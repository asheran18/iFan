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
#include "server.h"

/******** TODO: This should be done using more permenant vars in a meta file ********/
bool SCH_ON = false;
int SCH_START = 0;
int SCH_END = 0;
/*************************************************************************************/

int main(int argc, char const *argv[]) {

	/* Init memory map of the FPGA*/
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	/* Setup for sever */
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	/* Setup for command execution */
	char buffer[1024] = {0};
	int processStatus;

	/* Creating socket file descriptor */
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	/* Forcefully attaching socket to the port 8080 */
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	/* Forcefully attaching socket to the port 8080 */
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


	/* Set the GPIO pins to write */
	*(m_gpio_base + 4) = 0xFFFFFFFF;

	/* Process the incomming commands */
	printf("Server Ready...\n");
	while(1){
		/* 
		** TODO: read() polls the socket, and the instructions following it do not execute
		** until it returns, so we dont check the schedule until another command comes in
		*/		
		/* Get the socket and make sure it is valid */
		valread = read(new_socket, buffer, sizeof(buffer));
		if(valread != -1) {
			printf("Command recieved from client\n");
			/* Parse the command and get it ready for processing */
			command * cmd = getCommand(buffer);
			/* Process it */
			processStatus = processCommand(cmd);
			if (processStatus) printf("Invalid command detected and ignored...\n");
		}
		/* Check if things are on schedule or if action is required */
		if (SCH_ON) {
			printf("Checking scheduling...\n");
			checkSchedule();
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Main operations

command * getCommand(char * buffer) {
	/* A pointer to this will be returned */
	command * newCmd = malloc(sizeof(command));

	/* Check if there are enough chars for a valid opcode and get the opcode */
	if (strlen(buffer) < 7) {
		free(newCmd);
		 return NULL;
	}
	strncpy(newCmd->opcode, buffer, 7);
	newCmd->opcode[7] = '\0';

	/* Get the args if there are any */
	int sub = 8;
	int argnum = 0;
	int i = 8;
	while (buffer[i]) {
		if (buffer[i] == ',') {
			/* Grab the substring from the last comma to this comma*/
			strncpy((char*)newCmd->args[argnum], &buffer[sub], (i - sub));
			newCmd->args[argnum][i-sub] = '\0';
			argnum ++;
			sub = i + 1;
		}
		else if (buffer[i+1] == '\0') {
			/* Grab the substring from the last comma to the end */
			strncpy((char*)newCmd->args[argnum], &buffer[sub], (i + 1 - sub));
			newCmd->args[argnum][i + 1 -sub] = '\0';
		}
		i++;
	}

	/* Return the pointer */
	return newCmd;
}

int processCommand(command* cmd){
	printf("OPCODE: %s recieved from client\n", cmd->opcode);

	if(strcmp(cmd->opcode, "FAN_AON") == 0) {
		OPCODEsetFan(FAN_ON);
		return 0;

	} else if(strcmp(cmd->opcode, "FAN_OFF") == 0) {
		OPCODEsetFan(FAN_OFF);
		return 0;

	} else if(strcmp(cmd->opcode, "SET_SCH") == 0) {
		int start = strToTime((char*)cmd->args[0]);
		int end = strToTime((char*)cmd->args[1]);
		OPCODEsetSchedule(start,end);
		return 0;

	// Invalid command
	} else {
		return -1;
	}
}

//-----------------------------------------------------------------------------
// Opcode actuators

void OPCODEsetFan(int mode){
	printf("Setting fan to mode: %d\n", mode);
	setFan(mode);
}

void OPCODEsetSchedule(int start, int end){
	printf("Setting fan schedule start time to %d and stop time to %d\n", start, end);
	SCH_START = start;
	SCH_END = end;
	SCH_ON = true;
}

//-----------------------------------------------------------------------------
// Utilities

void checkSchedule(){
	//declaring a time struct
	time_t rawtime;
	struct tm * timeinfo;
	//retrieve current time
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	//get hours and minutes of current time
  	int hours = timeinfo->tm_hour;
	int minutes = timeinfo->tm_min;
	//converts minutes and hours into just minutes
	int currTime = (60*hours+minutes) - (4*60);
	if(currTime > SCH_START && currTime < SCH_END){
		printf("Within scheduled time...Setting fan to ON\n");
		setFan(FAN_ON);
	}
	else {
		printf("Outside of schedule...Setting fan to OFF\n");
		setFan(FAN_OFF);
	}
}

void setFan(int mode){
	if(mode == FAN_ON){
		*((uint32_t *)m_gpio_base) = 0xFFFFFFFF;
	} else {
		*((uint32_t *)m_gpio_base) = 0x0;
	}
}

int strToTime(char* str){
  int time = 0;
  time += 60*atoi(&str[0]);
  time += atoi(&str[3]);
  return time;
}
