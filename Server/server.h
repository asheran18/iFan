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

//-----------------------------------------------------------------------------
// Opcode actuators
/* Handles opcode FAN_AON and FAN_OFF */
void OPCODEsetFan(int mode);
/* Handles opcode SET_SCH */
void OPCODEsetSchedule(int start, int end);

//-----------------------------------------------------------------------------
// Utilities
/* Converts a string of time into a workable format */
int strToTime(char* str);
/* Turns the fan to mode: 1 = ON, 0 = OFF */
void setFan(int mode);
/* Checks if the fan should be on or off according to the schedule */
void checkSchedule();

#endif
