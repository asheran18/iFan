#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )


void HexSetDec(int value) {
        if (value < 0 || value > 999999) {
                return;
        }
        int i;
        for (i = 0; i < 6; i++) {
                HexSet(i, value%10);
                value /= 10;
        }
}


//#define ADDR_JP1PORT ((volatile char *) 0xFF200060)
//#define ADDR_JP2PORT ((volatile char *) 0xFF200070)

int main()
{ 
	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	*(m_gpio_base + 4) = 0xFFFFFFFF; 
	while(1) {
		
		//*((uint32_t *)m_gpio_base) = 0x1;
		*((uint32_t *)m_gpio_base) = 0x2;
		*((uint32_t *)m_gpio_base) = 0x0;
	}
	
	/*
	// init interface directions
	*(ADDR_JP2PORT+4) = 0; //set every JP2 bit direction to input
	*(ADDR_JP1PORT+4) = 0xffffffff; //set every JP1 bit dir to output

	while (1)
	{
		*ADDR_JP1PORT = *ADDR_JP2PORT; // Read the value from J2 and set to J1
		*ADDR_JP1PORT = 0xffffffff; // Every port on J1 set to 1
	}
	*/
	return 0;
}




/*int main(int argc, char *argv[])
{

	if(!FPGAInit()){
		printf("can't initialize fpga");
		return 0;
	}

	// some declarations
	uint32_t adcValue;
	int address = 0;
	WriteADC(1,0); // initialize the ADC
	int counter = 0; 	
	
	// main loop
	while(1){
		ReadADC(&adcValue, address);
		if (adcValue >= 2500) {
			if (counter > 0) {
				counter--;
			}
		}
		else if (adcValue <= 2400) {
			if (counter < 999999) {
				counter++;	
			}
		}
		HexSetDec(counter);
		//printf("The sensor reads: %d\n", adcValue);	
		usleep(1000*100);
	}



	return 1;
}*/

// This function will print a value in binary when given
//  a size in bytes and a pointer to the value
//  it is to be used for debugging purposes
// 	correct use of printing 2 byte binary value should look this:
//		printBits(2,&value);
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;

    for (i=size-1;i>=0;i--)
    {
        for (j=7;j>=0;j--)
        {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}
