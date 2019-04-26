#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pthread.h>

#define PORT 8080
#define MAX_COMMAND_LENGTH 100

void setupTCPConnection(int * ret_socket);
void * checkMailbox(void * new_socket);

int main(int argc, char const *argv[])
{

	/* Init */
	char buffer[MAX_COMMAND_LENGTH] = {0};

	/* Set up a TCP Connection */
	int * socket = malloc(sizeof(int));
	setupTCPConnection(socket);

	/* Rcvr thread */
	pthread_t mailbox;
	int m = pthread_create(&mailbox, NULL, checkMailbox, (void *)socket);
	if (m != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}
		
	/* Send commands to the server - from CLA for testing */
	while(1){
		scanf("%s", buffer);
		send(*socket, buffer, sizeof(buffer), 0);
	}

    return 0;
}

void setupTCPConnection(int * ret_socket) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "131.128.49.175", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
    }

    *ret_socket = sock;
}

void * checkMailbox(void * new_socket) {
	/* When the connection has been established, start receiving data */
	printf("Client receiving thread ready...\n");

	/* Setup for command execution */
	int enqueueSuccess;
	int valread;
	char buffer[MAX_COMMAND_LENGTH*5] = {0};

	/* Process the incomming commands */
	while(1){
		/* Get the socket and make sure it is valid */
		valread = read(*((int *)new_socket), buffer, sizeof(buffer));
		if(valread > 0 && strcmp(buffer,"")) {
			printf("Command received from server : %s\n", buffer);
		}
	}
	pthread_exit(NULL);
}

