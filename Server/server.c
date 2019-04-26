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
#include "hwlib.h"
#include "soc_cv_av/socal/socal.h"
#include "soc_cv_av/socal/hps.h"
#include "soc_cv_av/socal/alt_gpio.h"
#include "hps_0.h"
#include "fpga.h"
#include "server.h"

int main(int argc, char const *argv[]) {
	/* Some lock initialization */
	pthread_mutex_init (&mutex_mbox, NULL);
	pthread_mutex_init (&mutex_sch, NULL);
	pthread_mutex_init (&mutex_thr, NULL);

	/* Some global initialization */
	FAN_IS_ON = false;
	T_THRESH = 1000; //1000 is default threshold: you will not survive in these conditions anyway
	SCH_ON = false;
	SCH_START = 0;
	SCH_END = 0;
	SCH_START_STR = "00:00";
	SCH_END_STR = "00:00";
	startTime = 0;
	wasAutoCooling = false;
	simTemp = 65; // Start the simulated temperature at 65 deg F

	/* Init memory map of the FPGA*/
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	/* Set the GPIO pins to write and prep the ADC*/
	*(m_gpio_base + 4) = 0xFFFFFFFF;
	WriteADC(1,0);

	/* Set up a TCP Connection */
	int * socket = malloc(sizeof(int));
	setupTCPConnection(socket);

	/* Let's start the incoming command mailbox to handle user commands */
	pthread_t mailbox;
	int m = pthread_create(&mailbox, NULL, checkMailbox, (void *)socket);
	if (m != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* Let's start the sender to give the client the fan data */
	pthread_t sender;
	int t = pthread_create(&sender, NULL, transmitData, (void *)socket);
	if (t != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* Let's start the scheduler to handle user-set fan schedules */
	pthread_t scheduler;
	int s = pthread_create(&scheduler, NULL, checkSchedule, NULL);
	if (s != 0) {
		printf("FATAL SERVER ERROR - THREAD CREATION FAILED\n");
	}

	/* Let's start the monitor to handle user-set temperature thresholds */
	pthread_t tempMonitor;
	int tm = pthread_create(&tempMonitor, NULL, checkThreshold, NULL);
	if (tm != 0) {
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
		SCH_START_STR = (char*)cmd->args[0]; // Save the strings for easy return back to the client
		SCH_END_STR = (char*)cmd->args[1];
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
		OPCODEclrThr();
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
// Threaded operations

void * transmitData(void * new_socket){
	/* When the connection has been established, start sending data */
	printf("Server sending thread ready...\n");
	while(1) {
		SENDmode(*((int *)new_socket));
		printf("Sent Mode...\n");
		SENDtemp(*((int *)new_socket));
		printf("Sent Temp...\n");
		SENDuptime(*((int *)new_socket));
		printf("Sent uptime...\n");
		SENDthreshold(*((int *)new_socket));
		printf("Sent Threshold...\n");
		SENDschedule(*((int *)new_socket));
		printf("Sent Schedule...\n");
		// This esentially defines the data refresh rate on the client side - can adjust
		sleep(10);
	}
	pthread_exit(NULL);
}

void * checkMailbox(void * new_socket) {
	/* When the connection has been established, start receiving data */
	printf("Server receiving thread ready...\n");

	/* Setup for command execution */
	int enqueueSuccess;
	int valread;
	char buffer[MAX_COMMAND_LENGTH] = {0};

	/* Process the incomming commands */
	while(1){
		/* Get the socket and make sure it is valid */
		valread = read(*((int *)new_socket), buffer, sizeof(buffer));
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
				// Only set the pin if it is not already set
				if (FAN_IS_ON == false) {
					printf("Within scheduled time...Setting fan to ON\n");
					setFan(FAN_ON);
				}
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
		else if (SCH_ON == false && schWasOn && FAN_IS_ON) {
			printf("Schedule cleared during scheduled ON time...Setting fan to OFF\n");
			setFan(FAN_OFF);
			schWasOn = false;
		}
		pthread_mutex_unlock(&mutex_sch);
		sleep(3);
	}
	pthread_exit(NULL);
}

void * checkThreshold() {
	while(1) {
		pthread_mutex_lock(&mutex_thr);
		float currTemp = getCurrentTemperature();
		if (currTemp > T_THRESH) {
			/* If the temperature rises above the threshold, the fan goes on */
			if (FAN_IS_ON == false) {
				printf("Temperature threshold has been exceeded, beginning auto-cooling...\n");
				setFan(FAN_ON);
			}
			wasAutoCooling = true;
		}
		/* Add some hysteresis to the threshold so we don't turn back on right away */
		else if (currTemp < (T_THRESH - TEMP_HYST) && wasAutoCooling) {
			/* Only Turn the fan off it is was on because the threshold was exceeded */
			printf("Desirable temperature reached, turning off auto-cooling...\n");
			setFan(FAN_OFF);
			wasAutoCooling = false;
		}
		pthread_mutex_unlock(&mutex_thr);
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
	SCH_START_STR = "00:00";
	SCH_END_STR = "00:00";
	pthread_mutex_unlock(&mutex_sch);
}

void OPCODEsetThr(int temperature){
	T_THRESH = temperature;
}

void OPCODEclrThr() {
	T_THRESH = 1000;
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

void SENDmode(int socket){
	char buffer[MAX_COMMAND_LENGTH] = {0};
	sprintf(buffer, "FAN_MOD,%d", FAN_IS_ON);
	send(socket, buffer, sizeof(buffer), 0);
}

void SENDtemp(int socket){
	float Temperature = getCurrentTemperature();
	char buffer[MAX_COMMAND_LENGTH] = {0};
	sprintf(buffer, "AMB_TMP,%f", Temperature);
	send(socket, buffer, sizeof(buffer), 0);
}

void SENDuptime(int socket){
	float upTime = 0;
	if (startTime != 0) {
		/* Compute how long it has been running by taking the difference between now and the global variable */
		struct tm * timeinfo;
		time_t rawtime;
		time(&rawtime);
		/* Boilerplate to get the time, ignore */
		timeinfo = localtime(&rawtime);
		int hours = timeinfo->tm_hour;
		int minutes = timeinfo->tm_min;
		float currTime = (60*hours+minutes) - (4*60);
		upTime = currTime - startTime;
		/* Minutes to hours */
		upTime = upTime / 60.0;
	}
	char buffer[MAX_COMMAND_LENGTH] = {0};
	sprintf(buffer, "FAN_UPT,%f", upTime);
	send(socket, buffer, sizeof(buffer), 0);
}

void SENDthreshold(int socket){
	char buffer[MAX_COMMAND_LENGTH] = {0};
	sprintf(buffer, "CUR_THR,%d", T_THRESH);
	send(socket, buffer, sizeof(buffer), 0);
}

void SENDschedule(int socket){
	char buffer[MAX_COMMAND_LENGTH] = {0};
	sprintf(buffer, "CUR_SCH,%d,%s,%s", SCH_ON, SCH_START_STR, SCH_END_STR);
	send(socket, buffer, sizeof(buffer), 0);
}

//-----------------------------------------------------------------------------
// Utilities

void setupTCPConnection(int * ret_socket) {
	/* Setup for sever */
	int server_fd, new_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

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
	*ret_socket = new_socket;
}

void setFan(int mode){
	if(mode == FAN_ON){
		/* Start counting the Up Time */
		struct tm * timeinfo;
		time_t rawtime;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
	  	int hours = timeinfo->tm_hour;
		int minutes = timeinfo->tm_min;
		startTime = (60*hours+minutes) - (4*60);
		/* Turn the pin on */
		*((uint32_t *)m_gpio_base) = 0xFFFFFFFF;
		FAN_IS_ON = true;
	} else {
		/* Stop counting the Up Time */
		startTime = 0;
		*((uint32_t *)m_gpio_base) = 0x0;
		FAN_IS_ON = false;
	}
}

int strToTime(char* str){
  	int time = 0;
  	time += 60*atoi(&str[0]);
  	time += atoi(&str[3]);
  	return time;
}

float getCurrentTemperature(char* str) {

	/*
	*
	* The following code is meant to translate the reading from the ADC header
	* temperature sensor or thermosister into a temperature reading in degrees
	* F. However, the sensor given in the lab does not produce consistent or
	* accurate enough data for this application.
	*
	* A better sensor is required for the product to be used as intended, but for
	* the purposes of demonstrating all features under realistic conditions, we
	* simulate the temperature by randomly increasing, decreasing, or not changing
	* the simulated value at every time this function is called.
	*
	*/

	// uint32_t adcValue;
	// int address = 0;
	// ReadADC(&adcValue, address);
	// // Some computation is necessary to get the temperature
	// float res = (4095.0/(float)adcValue - 1.0);
	// res = 5.0 * 9960.0 - res + 9960.0;
	// res = res/5.0;
	// float tmp = res/2200.0;
	// tmp = log(tmp);
	// float tBeta = 1.0/(float)BETA;
	// tmp = tBeta*tmp;
	// Temperature = 1.0/298.15 + tmp;
	// Temperature = 1.0/Temperature;
	// Temperature = Temperature - 273.15;
	// //fprintf(stderr, "ADC = %d, Temperature = %f, R = %f\n", adcValue, Temperature, res);
	// return Temperature;

	return simTemp;
}
