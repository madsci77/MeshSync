/*

	Processor-specific code for MC9S08QG8
	
*/


#include "hardware.h"
#include "utility.h"
#include "status.h"

void mcu_setup(void)
{
	unsigned int i = 0;

	// Set ICS to external reference - BUSCLK is 1/2 clock from RFM12B
//	ICSC2 = 0b00100010;
//	ICSC1 = 0b10000000;

	SOPT1 = 0b01110011;

	ICSC2 = 0b00100010;
	ICSC1 = 0b00101000;		// External reference/32 * 512 = 8 MHz bus
	

   // Set data direction registers

   PTBD  = 0b00101100;		// LEDs are outputs
#ifndef HAS_SWITCH  
   PTBDD = 0b00101111;
#else
   PTBDD = 0b00101110;
   PTBPE = 0b00000001;
#endif
   PTBDS = 0b00000011;
   
	// Timebase setup - 244.140625 ticks/second with 1 MHz clock
	SRTISC = 0b00110100;
	
	// Set up SPI
	spi_setup();
	
	// KBI setup - for NIRQ input
	PTAPE = 0x00;
	KBIPE = 0x01;				// Enable keyboard pins
	KBISC = 2;					// Enable KBI interrupt

   PTADD = 8;		// Relay is output
   PTADS = 8;

}

void spi_setup(void)
{
	SS_HI;
	SPIC1 = 0b01010000;					// Module enable, no interrupts, CPOL=0, CPHA=0, MSB first
	SPIC2 = 0b00000000;					// 16-bit mode
	SPIBR = 1;								// BUSCLK / 2
}

#pragma inline
unsigned char spi_byte(unsigned char outb)
{
	// Wait for transmitter empty
	while (!(SPIS & 0x20));
	SPID = outb;
	// Wait for receiver full
	while (!(SPIS & 0x80));
	return SPID;
}

unsigned short spi16(unsigned short outb)
{
	unsigned short inb;
	SS_LOW;
	// Wait for transmitter empty
	while (!(SPIS & 0x20));
	SPID = outb >> 8;
	// Wait for receiver full
	while (!(SPIS & 0x80));
	inb = SPID << 8;
	// Wait for transmitter empty
	while (!(SPIS & 0x20));
	SPID = outb & 0xff;
	// Wait for receiver full
	while (!(SPIS & 0x80));
	inb |= SPID;
	SS_HI;
	return inb;
}

// Poll RFM12B for TX complete status
char spi_poll(void)
{
	char stat;
	stat = 0;
	SS_LOW;
	MOSI_LOW;
	SPSCK_HI;
	SPIC1 = 0b00010000; // Disable SPI
	if (MISO) stat = 1;
	SPSCK_LOW;
	MOSI_HI;
	SS_HI;
	SPIC1 = 0b01010000; // Enable SPI
	return stat;
}

// Send character to serial port
void sci_outc(unsigned char c)
{
	while (!(SCIS1 & 0x80)) reset_watchdog;
	SCID = c;
}


