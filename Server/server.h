#ifndef __SERVER_H__
#define  __SERVER_H__

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
char password[20];	//array for a password upto 20 chars

//-----------------------------------------------------------------------------
// Command interface
typedef struct {
  char opcode[8];       // There are many opcodes (always 7 chars), see documentation
  char* args[5][100];   // Explicit support for up to 5 args of size 100 chars each
} command;

int server_fd, new_socket, valread;
struct sockaddr_in address;

//-----------------------------------------------------------------------------
// Main operations
/* Sets up an easy-to-use interface for command access */
command * getCommand(char * buffer);
/* Decodes and processes the command by calling the correct function with the correct args*/
int processCommand(command * cmd);

//-----------------------------------------------------------------------------
// Opcode actuators
/* Handles opcode FAN_AON and FAN_OFF */
void OPCODEsetFan(int mode);
/* Handles opcode SET_SCH */
void OPCODEsetSchedule(int start, int end);
/* Handles logging in, sends TOK to client */
void OPCODEacceptUser(bool tok);

//-----------------------------------------------------------------------------
// Utilities
/* Converts a string of time into a workable format */
int strToTime(char* str);
/* Turns the fan to mode: 1 = ON, 0 = OFF */
void setFan(int mode);

#endif
