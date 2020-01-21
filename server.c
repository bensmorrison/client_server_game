/* Benjamin Morrison
 Victoria Schimansky
 Jared Orr
 Jake Decker*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define PORTNUM 5011       /* the port number the server will listen to*/
#define DEFAULT_PROTOCOL 0 /*constant for default protocol*/
#define BUFF_SIZE 512
#define BYTES_WRIT 511

char board[4][4];
int points[4][4];
int num_of_clients = 0;
int clients[5] = {0};
int player_points[5] = {0};
sem_t mutex;

void *doprocessing(void *sockInfo);
void initialize_matrix(void);
int status_error_checker(int status, int status2);
char write_to_buffer(char buffer1);
void print_board(void);
int index_selector(char buffer);

struct args
{
    int sock;
};

int main(int argc, char *argv[])
{
    // call function to initilize board and points
    initialize_matrix();

    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int status, i;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = PORTNUM;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    char hostname[128];
    gethostname(hostname, sizeof hostname);
    printf("My hostname: %s\n", hostname);

    /* Now bind the host address using bind() call.*/

    status = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    if (status < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Now Server starts listening clients wanting to connect. No more than 5 clients allowed */

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    sem_init(&mutex, 0, 1);
    while (1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        for (i = 0; i < 5; i++)
            if (clients[i] == 0)
            {
                clients[i] = newsockfd;
                num_of_clients++;
                break;
            }

        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        /* Create child process */
        //pid = fork();
        pthread_t thread_id;
        struct args targs = {newsockfd};
        pthread_create(&thread_id, NULL, doprocessing, (void *)&targs);

    } /* end of while */
    sem_destroy(&mutex);
    return 0;
}

void *doprocessing(void *sockInfo)
{
    int status, status2 = 0, a, b;
    int sock = ((struct args *)sockInfo)->sock;
    char buffer[256];
    int clientWinner = 0;
    int done = 0;

    // while loop for back and forth between S and C
    while (sock)
    {
        int i;
        bzero(buffer, sizeof(buffer));               // erase buffer
        status = read(sock, buffer, sizeof(buffer)); // reads message from client and copies into buffer

        // check the status and exit if error or client close
        status_error_checker(status, status2);

        // input validation on lower case characters
        if (buffer[0] >= 'a' && buffer[0] <= 'p' && buffer[1] == '\n')
            buffer[0] = buffer[0] - 32;

        //printf("server: got connection from %s port %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        // begins the game
        if (0 == strcmp(buffer, "ready to play\n"))
        {
            status2 = write(sock, board, sizeof(board));
            print_board();
            continue;
        }

        // if message from client is "exit", server stops
        if (strncmp("exit", buffer, 4) == 0)
        {
            printf("Server Exit...\n");
            for (i = 0; i < 5; i++)
                if (clients[i] == sock)
                {
                    clients[i] = 0;
                }
        }

        // checks for index selection
        if (buffer[0] >= 'A' && buffer[0] <= 'P' && buffer[1] == '\n')
        {
            for (a = 0; a < 4; a++)
            {
                for (b = 0; b < 4; b++)
                {
                    sem_wait(&mutex);
                    // sets selection to '-' to indicate it has been chosen
                    if (buffer[0] == board[a][b])
                    {
                        board[a][b] = '-';
                        for (i = 0; i < 5; i++)
                        {
                            if (sock == clients[i])
                                player_points[i] += points[a][b];
                        }
                    }
                    sem_post(&mutex);
                }
            }

            // send updated board to active clients
            for (i = 0; i < 5; i++)
            {
                if (clients[i] != 0)
                    status2 = write(clients[i], board, sizeof(board));
            }
            
            print_board();
            
            done = 1;
            for (a = 0; a < 4; a++)
                for (b = 0; b < 4; b++)
                    if (board[a][b] != '-')
                        done = 0;
        }

        //Checks the flag for if the game is finished

        else if (done)
        {
            int max = 0;
            // Finding the winner
            for (i = 0; i < 5; i++)
            {
                if (clients[i] != 0)
                {
                    if (player_points[i] > max)
                    {
                        max = player_points[i];
                        clientWinner = i;
                    }
                }
            }
            // Printing the winner
            // Asking if they want to play again
            for (i = 0; i < 5; i++)
            {
                if (clients[i] != 0)
                {
                    char str[256];
                    snprintf(str, 256, "Player %d won with %d points!\n Type 'exit' to exit, or 'play again' to play again.", clientWinner+1, player_points[clientWinner]);
                    write(clients[i], str, strlen(str));
                }
            }
            done = 0;
        }

        // Another error check for writing to the socket
        status_error_checker(status, status2);

        if (strncmp("play again", buffer, 10) == 0)
        {
            initialize_matrix();
            printf("Playing again... \n");
            for (i = 0; i < 5; i++)
            {
                player_points[i] = 0;

                if (clients[i] != 0)
                {
                    // need condition here to add points to correct player
                    write(clients[i], board, sizeof(board));
                }
            }
        }
    }
    return 0;
}

/* initializes the matrix with board and values */
void initialize_matrix()
{
    srand(time(0));
    int a, b;
    char c = 'A';

    /* creating random points to assign to board */
    for (a = 0; a < 4; a++)
        for (b = 0; b < 4; b++)
            points[a][b] = rand() % 9;

    /* creating the board */
    for (a = 0; a < 4; a++)
        for (b = 0; b < 4; b++)
            board[a][b] = c++;
}

/* helper to check for read/write socket errors*/
int status_error_checker(int status, int status2)
{
    if (status == 0)
    {
        printf("Player has exited\n");
        int q = 1;
        pthread_exit(&q);
    }

    if (status < 0)
    {
        perror("ERROR reading from socket");
    }

    if (status2 < 0)
    {
        perror("ERROR writing to socket");
    }

    return 0;
}

/* helper to print the board*/
void print_board()
{
    int i, j, k;

    printf("Matrix: \n\n");

    for (i = 0; i < num_of_clients; i++)
    {
        printf("Player %d score: %d\n", i + 1, player_points[i]);
    }

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            printf("%c\t", board[i][j]);
        }
        printf("\n");

        for (k = 0; k < 4; k++)
        {
            printf("%d\t", points[i][k]);
        }
        printf("\n");
        printf("\n");
    }
}
