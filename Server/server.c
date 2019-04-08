#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pthread.h>
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "hps_0.h"
#include "fpga.h"
#include "server.h"

int main(int argc, char const *argv[]) {
	/* Some initialization */
	pthread_mutex_init (&mutex_mbox, NULL);
	pthread_mutex_init (&mutex_sch, NULL);

	T_THRESH = -1; //-1 represents threshold not set
	SCH_ON = false;
	SCH_START = 0;
	SCH_END = 0;

	/* Init memory map of the FPGA*/
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	/* Set the GPIO pins to write */
	*(m_gpio_base + 4) = 0xFFFFFFFF;

	/* Let's start the incoming command mailbox to handle user commands */
	pthread_t mailbox;
	int m = pthread_create(&mailbox, NULL, checkMailbox, NULL);
	if (m != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}
		
	/* Let's start the sender to give the client the fan data */
	pthread_t sender;
	int t = pthread_create(&sender, NULL, transmitData, NULL);
	if (t != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* Let's start the scheduler to handle user-set fan schedules */
	pthread_t scheduler;
	int s = pthread_create(&scheduler, NULL, checkSchedule, NULL);
	if (s != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* This is where login functionality should be immplemented*/
	/* Pseudocode:
		while(not logged in && password is set){
			read_from_socket(password)
			if(password = correct){
				login = yes		//should then exit from while loop
			}
		}
	*/
	int processStatus;
	while(1){
		/* Check if there is a command in the queue */
		pthread_mutex_lock(&mutex_mbox);
		if(!queueEmpty()) {
			printf("Processing command at the front of the queue...\n");
			/* Parse the command and get it ready for processing */
			char * firstCommand = dequeueCommand();
			command * cmd = getCommand(firstCommand);
			/* Process it */
			if(cmd != NULL) {
				processStatus = processCommand(cmd);
				if (processStatus) printf("Invalid command detected and ignored...\n");
			}
			else {
				printf("WARNING - Invalid syntax or memory error results in ignored command\n");
			}
			
		}
		pthread_mutex_unlock(&mutex_mbox);
		sleep(1);
		/* Begin Data to Send */
		//transmitData();
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Main operations

command * getCommand(char * buffer) {
	/* A pointer to this will be returned */
	command * newCmd = malloc(sizeof(command));

  	if (newCmd == NULL) {
    		printf("FATAL SERVER ERROR - malloc failed\n");
    		return NULL;
  	}

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
			/* Grab the substring from the last comma to this comma */
			strncpy(newCmd->args[argnum], buffer + sub, (i - sub));
			newCmd->args[argnum][i-sub] = '\0';
      	argnum ++;
			sub = i + 1;
		}
		else if (buffer[i+1] == '\0') {
			/* Grab the substring from the last comma to the end */
			strncpy(newCmd->args[argnum], buffer + sub, (i + 1 - sub));
			newCmd->args[argnum][i + 1 -sub] = '\0';
		}
		i++;
	}

	/* Return the pointer */
	return newCmd;
}

int processCommand(command* cmd){
	printf("OPCODE: %s is being processed\n", cmd->opcode);

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

	} else if(strcmp(cmd->opcode, "CLR_SCH") == 0){
		OPCODEclrSch();
		return 0;

	} else if (strcmp(cmd->opcode, "SET_THR") == 0){
		int temp = atoi((char*)cmd->args[0]);
		OPCODEsetThr(temp);		//reads args[0] for what temp to set threshold as
		return 0;
	} else if(strcmp(cmd->opcode, "CLR_THR") == 0){
		OPCODEsetThr(-1);
		return 0;
	} else if(strcmp(cmd->opcode, "SET_PWD") == 0){
		password = (char*)cmd->args[0];
		return 0;
	} else if(strcmp(cmd->opcode, "TRY_PWD") == 0){
	/*
		if(hash(password) == args[0]){
			OPCODEacceptUser(true); //Password is correct and allows user to login
		} else {
			OPCODEacceptUser(false); //Password is incorrect and asks for a retry
	  	}
	*/
		return 0;
	}
	else {
	// Invalid command
		return -1;
	}
}

void * transmitData(){
	/* Setup for sever */
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

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
	address.sin_port = htons(OUTPORT);

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

	/* When the connection has been established, start sending data */
	printf("Server sending thread ready...\n");
	while(1) {
		char message[MAX_COMMAND_LENGTH] = "Data from server...";
		send((int)new_socket, message, sizeof(message), 0);
		sleep(4);
	}
	pthread_exit(NULL);	
}

void * checkMailbox() {
	/* Setup for sever */
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	/* Setup for command execution */
	char buffer[MAX_COMMAND_LENGTH] = {0};
	//int processStatus;

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

	/* When the connection has been established, start receiving data */
	printf("Server receiving thread ready...\n");
	
	/* Process the incomming commands */
	int enqueueSuccess;
	while(1){
		/* Get the socket and make sure it is valid */
		
		valread = read(new_socket, buffer, sizeof(buffer));
		if(valread > 0 && strcmp(buffer,"")) {
			printf("Command received from client\n");
			printf("Command plain text is : %s\n", buffer);
			/* Try to enqueue the command */
			// TODO: It may be a good idea to loop here until there is space
			pthread_mutex_lock(&mutex_mbox);
			enqueueSuccess = tryEnqueueCommand(buffer);
			if (enqueueSuccess < 0) {
				printf("WARNING - Could not enqueue command :: %s :: Command dropped!\n", buffer);
			}
			pthread_mutex_unlock(&mutex_mbox);
		}
		// TODO: may want to wait here if data is being sent (can used a shared variable to check?)
	}
	pthread_exit(NULL);
}

void * checkSchedule() {
	bool schWasOn = false;
	while(1) {
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
		pthread_mutex_lock(&mutex_sch);
		if (SCH_ON) {
			// Per the spec, turn on at <inclusive> and off at <exclusive>	
			if(currTime >= SCH_START && currTime < SCH_END){
				printf("Within scheduled time...Setting fan to ON\n");
				setFan(FAN_ON);
				schWasOn = true;
			}
			// We were on a schedule, but it has expired
			else if (schWasOn) {
				printf("Outside of schedule...Setting fan to OFF\n");
				setFan(FAN_OFF);
				schWasOn = false;
				SCH_ON = false;				
			}
		}
		pthread_mutex_unlock(&mutex_sch);
		sleep(3);
	}
	pthread_exit(NULL);
}

//-----------------------------------------------------------------------------
// FIFO queue operations

int tryEnqueueCommand(char * incomingCommand) {
	int i;
	/* Find a spot */
	for (i = 0; i < MAX_FIFO_SIZE; i++) {
		if (mailboxQueue[i] == 0) {
			mailboxQueue[i] = incomingCommand;
			return 0;
		}
	}
	/* If we have not returned yet, we do not have room */
	return -1;
}

bool queueEmpty() {
	if (mailboxQueue[0] == 0) return true;
	return false;
}

char * dequeueCommand() {
	char * retCommand = mailboxQueue[0];
	/* Shift the queue*/
  int i;
  for (i = 0; i < MAX_FIFO_SIZE - 1; i++) {
    mailboxQueue[i] = mailboxQueue[i + 1];
  }
  mailboxQueue[MAX_FIFO_SIZE-1] = 0; // Clear the back of the queue if it was previously full
  return retCommand;
}

//-----------------------------------------------------------------------------
// Opcode actuators

void OPCODEsetFan(int mode){
	printf("Setting fan to mode: %d\n", mode);
	setFan(mode);
}

void OPCODEsetSchedule(int start, int end){
	printf("Setting fan schedule start time to %d and stop time to %d\n", start, end);
	pthread_mutex_lock(&mutex_sch);
	SCH_START = start;
	SCH_END = end;
	SCH_ON = true;
	pthread_mutex_unlock(&mutex_sch);
}

void OPCODEclrSch(){
	pthread_mutex_lock(&mutex_sch);
	SCH_ON = false;
	SCH_START = 0;
	SCH_END = 0;
	pthread_mutex_unlock(&mutex_sch);
}

void OPCODEsetThr(int temperature){
	T_THRESH = temperature;
}

void OPCODEacceptUser(bool tok){
	if(tok == true){
		//send tok == yes (1) to client
		char ret_tok = "LOG_TOK,1";
	} else {
		//send tok == no (0) to client
		char ret_tok = "LOG_TOK,0";
	}

	//send(server_fd, ret_tok, sizeof(ret_tok), 0);

}

//-----------------------------------------------------------------------------
// Transmission to the client

void SENDtemp(){

}

void SENDmode(){

}

void SENDuptime(){

}

void SENDthreshold(){

}

void SENDschedule(){

}

//-----------------------------------------------------------------------------
// Utilities

int transmitCommand(char* message){
	/* sets socket stream */
	//send(server_fd, message, sizeof(message), 0);
	/* Returns 0 if no errors */
	return 0;
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
