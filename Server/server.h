#ifndef __SERVER_H__
#define  __SERVER_H__
#include <stdbool.h>
#include <sys/socket.h>

//-----------------------------------------------------------------------------
// Defines for sockets
#define PORT 8080
#define SO_REUSEPORT 15
#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

//-----------------------------------------------------------------------------
// Defines for readability
#define FAN_ON 1
#define FAN_OFF 0

//-----------------------------------------------------------------------------
// Global variables
/******** TODO: This should be done using more permenant vars in a meta file ********/
bool SCH_ON;
int SCH_START;
int SCH_END;
int T_THRESH;
char* password;	//array
int server_fd, new_socket, valread;
struct sockaddr_in address;
pthread_mutex_t mutex;

//-----------------------------------------------------------------------------
// Command interface
typedef struct {
  char opcode[8];       // There are many opcodes (always 7 chars), see documentation
  char* args[5][100];   // Explicit support for up to 5 args of size 100 chars each
} command;


//-----------------------------------------------------------------------------
// Main operations
/* Sets up an easy-to-use interface for command access */
command * getCommand(char * buffer);
/* Decodes and processes the command by calling the correct function with the correct args*/
int processCommand(command * cmd);
/* Sends current fan information to the client to be displayed in the app*/
int transmitData();

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
/* Handles logging in, sends TOK to client */
void OPCODEacceptUser(bool tok);

//-----------------------------------------------------------------------------
// Transmission to the client
/* Function for transmitting current temperature */
int SENDtemp();
/* Function for transmitting current fan mode*/
int SENDmode();
/* Function for transmitting current uptime*/
int SENDuptime();
/* Function for transmitting current threshold */
int SENDthreshold();
/* Function for transmitting current schedule */
int SENDschedule();

//-----------------------------------------------------------------------------
// Utilities
/* Converts a string of time into a workable format */
int strToTime(char* str);
/* Turns the fan to mode: 1 = ON, 0 = OFF */
void setFan(int mode);
/* Checks if the fan should be on or off according to the schedule */
void *checkSchedule();
/* Transmits 1 message across the socket */
int transmitCommand(char* message);

#endif
