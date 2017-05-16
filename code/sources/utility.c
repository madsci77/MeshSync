// Utility routines
// void sleep(unsigned int ticks);

#include "utility.h"
#include "hardware.h"
#include "status.h"
#include "ctype.h"

volatile unsigned short wait_ticks;

// Wait specified number of clock ticks
void wait(unsigned int ticks)
{
	wait_ticks = ticks;
	while (wait_ticks)
	{
		Wait;
		reset_watchdog;
	}		
}

