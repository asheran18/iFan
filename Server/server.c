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
	/* Init globals */
	SCH_ON = false;
	SCH_START = 0;
	SCH_END = 0;
	T_THRESH = -1; //-1 represents threshold not set

	/* Init memory map of the FPGA*/
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	/* Setup for sever */
/*	int server_fd, new_socket, valread;
	struct sockaddr_in address;
*/	int opt = 1;
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


	/* Let's start the scheduler to handle user-set fan schedules */
	pthread_t scheduler;
	int t = pthread_create(&scheduler, NULL, checkSchedule, NULL);
	if (t != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* Process the incomming commands */
	printf("Server Ready...\n");

	/* This is where login functionality should be immplemented*/
	/* Pseudocode:
		while(not logged in && password is set){
			read_from_socket(password)
			if(password = correct){
				login = yes		//should then exit from while loop
			}
		}
	*/

	while(1){
		/* Get the socket and make sure it is valid */
		valread = read(new_socket, buffer, sizeof(buffer));
		if(valread != -1) {
			printf("Command received from client\n");
			/* Parse the command and get it ready for processing */
			command * cmd = getCommand(buffer);
			/* Process it */
			processStatus = processCommand(cmd);
			if (processStatus) printf("Invalid command detected and ignored...\n");
		}

		/* Begin Data to Send */
		transmitData();


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
	printf("OPCODE: %s received from client\n", cmd->opcode);

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

//-----------------------------------------------------------------------------
// Opcode actuators

void OPCODEsetFan(int mode){
	printf("Setting fan to mode: %d\n", mode);
	setFan(mode);
}

void OPCODEsetSchedule(int start, int end){
	printf("Setting fan schedule start time to %d and stop time to %d\n", start, end);
	pthread_mutex_lock(&mutex);
	SCH_START = start;
	SCH_END = end;
	SCH_ON = true;
	pthread_mutex_unlock(&mutex);
}

void OPCODEclrSch(){
	pthread_mutex_lock(&mutex);
	SCH_ON = false;
	SCH_START = 0;
	SCH_END = 0;
	pthread_mutex_unlock(&mutex);
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
// Transmission OPCODE Actuators

/* function for transmitting current temperature */
int SENDtemp(){
	char* msg = "AMB_TMP,";

	/* Read temperature from ADC */
	int temp = ;

	/* Convert temperature to string */
	char* tempStr = itoa(temp);


	/* Set msg*/
	strcat(msg, tempStr);

	/* Transmit msg*/
	send(server_fd, msg, sizeof(msg), 0);

	return 0;
}

/* function for transmitting current fan mode*/
int SENDmode(){


	return 0;
}

/* function for transmitting current uptime*/
int SENDuptime(){

	return 0;
}
/* function for transmitting current threshold */
int SENDthreshold(){

	return 0;
}

int SENDschedule(){

	return 0;
}


//-----------------------------------------------------------------------------
// Data Transmission Function
/* main data transmission function*/
int transmitData(){
	/* Send Temperature */
	int error = SENDtemp();

	if(error == 0){
		printf("Temperature sent...\n");
	}
	else {
		printf("Error Code: %d\n", error);
	}

	/* Send Current Fan Mode */
	error = SENDmode();

	if(error == 0){
		printf("Fan Mode sent...\n");
	}
	else {
		printf("Error Code: %d\n", error);
	}

	/* Send Fan Uptime (How long its been on/off) */
	error = SENDuptime();

	if(error == 0){
		printf("Uptime sent...\n");
	}
	else {
		printf("Error Code: %d\n", error);
	}

	/* Send Current Threshold */
	error = SENDthreshold();

	if(error == 0){
		printf("Current Threshold sent...\n");
	}
	else {
		printf("Error Code: %d\n", error);
	}

	/* Send Current Schedule */
	error = SENDschedule();

	if(error == 0){
		printf("Current Schedule sent...\n");
	}
	else {
		printf("Error Code: %d\n", error);
	}

	/* Return 0 if no errors */
	return 0;
}

//-----------------------------------------------------------------------------
// Utilities

/* transmits 1 message across the socket */
int transmitCommand(char* message){
	/* sets socket stream */
	send(server_fd, message, sizeof(message), 0);
	/* Returns 0 if no errors */
}

/* Sets fan to on or off */
void setFan(int mode){
	if(mode == FAN_ON){
		*((uint32_t *)m_gpio_base) = 0xFFFFFFFF;
	} else {
		*((uint32_t *)m_gpio_base) = 0x0;
	}
}

/* Converts a string to time in seconds (parsed as: "hh:mm" in 24 hour time */
int strToTime(char* str){
  	int time = 0;
  	time += 60*atoi(&str[0]);
  	time += atoi(&str[3]);
  	return time;
}

/* Checks if the fan should be on or off according to the schedule */
void *checkSchedule() {
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
		pthread_mutex_lock(&mutex);
		if (SCH_ON) {
			if(currTime > SCH_START && currTime < SCH_END){
				printf("Within scheduled time...Setting fan to ON\n");
				setFan(FAN_ON);
			}
			else {
				printf("Outside of schedule...Setting fan to OFF\n");
				setFan(FAN_OFF);
			}
		}
		pthread_mutex_unlock(&mutex);
		sleep(3);
	}
	pthread_exit(NULL);
}
