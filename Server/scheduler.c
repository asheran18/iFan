#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "server.h"

/* Checks if the fan should be on or off according to the schedule */
void checkSchedule();

int main() {
	while (1) {
		/* Check if things are on schedule or if action is required */
		if (SCH_ON) {
			printf("Checking scheduling...\n");
			checkSchedule();
		}
	}	
	return 0;
}

/* Checks if the fan should be on or off according to the schedule */
void checkSchedule() {
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
