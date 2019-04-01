#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "hps_0.h"
#include <stdbool.h>
#include "fpga.h"


#define PORT 8080
#define SO_REUSEPORT 15 
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )


int main(int argc, char const *argv[])
{

	// Init memory map of the FPGA	
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}


	// Setup for sever
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	//uint32_t buffer;
	char message[256];

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}

	
	// Set the GPIO pins to write
	*(m_gpio_base + 4) = 0xFFFFFFFF; 

	// Process the incomming messages
	printf("Server Ready...\n");
	while(1){
		valread = read(new_socket, buffer, sizeof(buffer));
				
		if (strlen(buffer) == 1) {
			printf("Buffer Rcvd: %s\n", buffer);
			printf("valread status: %d\n", valread);
			if (buffer[0] == '1') *((uint32_t *)m_gpio_base) = 0xFFFFFFFF;
			else if (buffer[0] == '0') *((uint32_t *)m_gpio_base) = 0x0;
		}
		
		//scanf("%s", message);
		//send(new_socket, message, strlen(message), 0);

	}

    	return 0;
}

