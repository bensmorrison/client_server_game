/* Benjamin Morrison
Victoria Schimansky
Jared Orr
Jake Decker*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define PORTNUM 5011     /* the port number that the server is listening to*/
#define DEFAULT_PROTOCOL 0 /*constant for default protocol*/
#define BUFF_SIZE 512
#define BYTES_WRIT 511

struct args
{
    int sock;
};

void doprocessing();
void *serverRecv(void *arg);
int socketid; /* will hold the id of the socket created*/


int main()
{
    int port;
    int status;   /* error status holder*/
    struct sockaddr_in serv_addr;
    struct hostent *server;

    /* this creates the socket*/
    socketid = socket(AF_INET, SOCK_STREAM, DEFAULT_PROTOCOL);
    if (socketid < 0)
    {
        printf("error in creating client socket.\n");
        exit(-1);
    }
    printf("Created client socket successfully.\n");

    /* before connecting the socket we need to set up the right values in
  	 thedifferent fields of the structure server_addryou can check the
	 definition of this structure on your own*/
    server = gethostbyname("osnode10");
    if (server == NULL)
    {
        printf("Error trying to identify the machine where the server is running.\n");
        exit(0);
    }
    port = PORTNUM;

    /*This function is used to initialize the socket structures with null values. */
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);

    /* connecting the client socket to the server socket */
    status = connect(socketid, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (status < 0)
    {
        printf("Error in connecting client socket with server.\n");
        exit(-1);
    }
    printf("Connected client socket to the server socket.\n");

    printf("\n\nWelcome to the game! \n");
    /*printf("Enter \"help\" to learn how to play the game\n");*/
    printf("Enter \"ready to play\" to start\n");

    // function for client side
    pthread_t thread_id;
    struct args targs = {socketid};
    pthread_create(&thread_id, NULL, serverRecv, (void *)&targs);

    doprocessing(socketid);

    // close the socket when doprocessing is done
    close(socketid);
    return 0;
}

void *serverRecv(void *arg)
{
    char buffer[BUFF_SIZE]; /* the message buffer*/
    int i;
    int status;

    while (socketid > 0)
    {
        bzero(buffer, sizeof(buffer));
        status = read(socketid, buffer, sizeof(buffer));
        if(status == 16)
        {
            system("clear");
            for (i = 0; i < 16; i += 4)
                printf("%c\t%c\t%c\t%c\n", buffer[i], buffer[i + 1], buffer[i + 2], buffer[i + 3]);
        }
        else if (status > 1)
        {
            printf("%s\n", buffer);
        }  
    }
    return 0;
}

void doprocessing()
{
    char buffer[BUFF_SIZE]; /* the message buffer*/
    int status, n;          /* error status holder*/

    /* now lets send a message to the server. the message will be
       whatever the user wants to write to the server.*/
    while (socketid > 0)
    {
        // clear out the buffer
        bzero(buffer, sizeof(buffer));

        // get input and send to the server
        n = 0;

        while ((buffer[n++] = getchar()) != '\n')
            ;
        status = write(socketid, buffer, sizeof(buffer));

        // error check
        if (status < 0)
        {
            printf("error while sending client message to server\n");
            exit(0);
        }

        if ((strncmp(buffer, "exit", 4)) == 0)
        {
            printf("Client Exit...\n");
            close(socketid);
            exit(0);
        }
    }
}
