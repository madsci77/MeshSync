/*

	MC9S08QG8 MCU, clocked from RFM12B module
	LEDs on PTB0 and PTB1
	Relay on PTA3 - Max turnon time 3 ms, turnoff 0.5 ms
	RFM12B on SPI
	SPI CS on PTB5 (NSEL)
	DCLK on PTA1 (FFIT) - FIFO full indication
	FSK on PTA2
	NIRQ on PTA0/KB0
	MOSI on PTB3
	MISO on PTB4
	SPSCK on PTB2
	
	On RX - NIRQ goes low, stays low until cleared
	FFIT goes high to indicate FIFO full
	

*/

#define reset_watchdog {__asm sta 0x1800;}

#define ticks_per_sec 244 // 2441 @ 10 MHz
#define ticks_per_slot 7
#define packet_latency_ticks 2	// 2.05044 = 8.4 msec

#define SS_HI {PTBD |= 0x20;}
#define SS_LOW {PTBD &= ~0x20;}
#define MOSI_HI {PTBD |= 0x08;}
#define MOSI_LOW {PTBD &= ~0x08;}
#define SPSCK_HI {PTBD |= 0x04;}
#define SPSCK_LOW {PTBD &= ~0x04;}
#define MISO (PTBD & 0x10)
#define FFIT (PTAD & 0x02)

//#define RELAY_ON {PTAD |= 0x08;}
//#define RELAY_OFF {PTAD &= ~0x08;}

#define RELAY_ON {PTAD |= 0x08; PTBD |= 2;}
#define RELAY_OFF {PTAD &= ~0x08; PTBD &= ~2;}
#define RED_ON
#define RED_OFF

#define GREEN_ON {PTBD |= 1;}
#define GREEN_OFF {PTBD &= ~1;}
//#define RED_ON {PTBD |= 2;}
//#define RED_OFF {PTBD &= ~2;}

// General processor setup
void mcu_setup(void);

// Flash routines
void flash_erase(void *addr);
void flash_program(void *dest, void *src, unsigned int len);

// SPI routines

void spi_setup(void);
char spi_poll(void);
unsigned short spi16(unsigned short out);

void sci_outc(unsigned char c);
unsigned char sci_getc(void);

unsigned char spi_byte(unsigned char outb);

#include "mc9s08qg8.h"
#include "hidef.h"