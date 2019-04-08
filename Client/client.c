// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#define PORT 8080
#define OUTPORT 8081

#define MAX_COMMAND_LENGTH 100

int main(int argc, char const *argv[])
{
/* Socket1  */
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "131.128.49.175", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

/* Socket2 */
    struct sockaddr_in address2;
    int sockOUT = 0, valreadOUT;
    struct sockaddr_in serv_addr2;
    
    char bufferOUT[MAX_COMMAND_LENGTH] = {0};
    if ((sockOUT = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr2, '0', sizeof(serv_addr2));

    serv_addr2.sin_family = AF_INET;

    serv_addr2.sin_port = htons(OUTPORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "131.128.49.175", &serv_addr2.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sockOUT, (struct sockaddr *)&serv_addr2, sizeof(serv_addr2)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    //int message = 0xffffffff;
    //uint32_t *message;
    char message[MAX_COMMAND_LENGTH] = {0};
	//send(sock, message, sizeof(message), 0);
    while(1){
        //scanf("%s", message);
	//valread = read(sock , buffer, MAX_COMMAND_LENGTH);
        //send(sock, message, sizeof(message), 0);
	valreadOUT = read(sockOUT, bufferOUT, MAX_COMMAND_LENGTH);
       	printf("%s\n",bufferOUT);
    }

	//close(sock);
    return 0;
}
