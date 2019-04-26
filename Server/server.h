#ifndef __SERVER_H__
#define  __SERVER_H__
#include <stdbool.h>
#include <sys/socket.h>

//-----------------------------------------------------------------------------
// Defines for sockets
#define PORT 8080
#define OUTPORT 8081
#define SO_REUSEPORT 15
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

//-----------------------------------------------------------------------------
// Defines for readability
#define FAN_ON 1
#define FAN_OFF 0
#define MAX_FIFO_SIZE 10
#define MAX_COMMAND_LENGTH 100
#define MAX_ARGS 5
#define MAX_ADC_VAL 4095
#define BETA 3977
#define TEMP_HYST 1

//-----------------------------------------------------------------------------
// Global variables
/******** TODO: Some of this should be done using more permenant vars in a meta file ********/
/* The global status of the fan */
bool FAN_IS_ON;
/* For fan scheduling */
bool SCH_ON;
int SCH_START;
int SCH_END;
char * SCH_START_STR;
char * SCH_END_STR;
/* For fan thresholding */
int T_THRESH;
bool wasAutoCooling;
/* For keeping track of up time */
float startTime;
/* For threading */
pthread_mutex_t mutex_mbox;
pthread_mutex_t mutex_sch;
pthread_mutex_t mutex_thr;
/* For authentication */
char * password;
/* For job processing */
char * mailboxQueue[MAX_FIFO_SIZE] = {0};
/* For simulating temperature */
int simTemp;

//-----------------------------------------------------------------------------
// Structures for easy interfacing
typedef struct {
  char opcode[8];                                 // There are many opcodes (always 7 chars), see documentation
  char args[MAX_ARGS][MAX_COMMAND_LENGTH];        // Explicit support for up to <MAX_ARGS> args
} command;

//-----------------------------------------------------------------------------
// Main operations
/* Sets up an easy-to-use interface for command access */
command * getCommand(char * buffer);
/* Decodes and processes the command by calling the correct function with the correct args*/
int processCommand(command * cmd);

//-----------------------------------------------------------------------------
// Threaded operations
/* Sends current fan information to the client to be displayed in the app */
void * transmitData(void * new_socket);
/* Watches for incoming packets and adds them to the processing queue */
void * checkMailbox();
/* Checks if the fan should be on or off according to the schedule */
void * checkSchedule();
/* Checks if the threshold has been surpassed and the fan should turn on */
void * checkThreshold();

//-----------------------------------------------------------------------------
// FIFO queue operations - assumed to be performed on gloabal variable "cmdQueue"
/* Tries to add a command to the back of the queue if possible, returns -1 if failed */
int tryEnqueueCommand(char * incomingCommand);
/* Returns true if the queue is empty*/
bool queueEmpty();
/* Returns the first command in the queue and removes it from the queue */
char * dequeueCommand();

//-----------------------------------------------------------------------------
// Opcode actuators
/* Handles opcode FAN_AON and FAN_OFF */
void OPCODEsetFan(int mode);
/* Handles opcode SET_SCH */
void OPCODEsetSchedule(int start, int end);
/* Handles opcode CLR_SCH */
void OPCODEclrSch();
/* Handles opcode SET_THR */
void OPCODEsetThr(int temperature);
/* Handles opcode CLR_THR */
void OPCODEclrThr();
/* Handles logging in, sends TOK to client */
void OPCODEacceptUser(bool tok);

//-----------------------------------------------------------------------------
// Transmission to the client
/* Function for transmitting if the fan is on or off */
char * SENDmode();
/* Function for transmitting current temperature */
char * SENDtemp();
/* Function for transmitting current uptime*/
char * SENDuptime();
/* Function for transmitting current threshold */
char * SENDthreshold();
/* Function for transmitting current schedule */
char * SENDschedule();

//-----------------------------------------------------------------------------
// Utilities
/* Sets up a TCP connection on <ret_socket> */
void setupTCPConnection(int * ret_socket);
/* Converts a string of time into a workable format */
int strToTime(char* str);
/* Turns the fan to mode: 1 = ON, 0 = OFF */
void setFan(int mode);
/* Returns the current temperature reading from the sensor in deg. F */
int getCurrentTemperature();

#endif
