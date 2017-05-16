// RF12B transceiver support code

#include "hardware.h"
#include "utility.h"
#include "rf12b.h"
#include "events.h"
#include "string.h"
#include "meshsync.h"

#define FEND 0xC0
#define FESC 0xDB
#define TFEND 0xDC
#define TFESC 0xDD

//#define USE_HAMMING

#define RX_BUFFER_SIZE 64
#define MIN_PACKET_SIZE 4

//unsigned char *tx_next_packet, *tx_head, *tx_tail, txbyte;
unsigned char rx_buffer[RX_BUFFER_SIZE];

#define PTT_ON {spi16(0x8228); spi16(0x8238);}
#define PTT_OFF {spi16(0x8228); spi16(0x82d8);}

volatile static unsigned short fcs, txcount;
volatile static unsigned char rxcount;
static char escape;

unsigned short crc_calc(unsigned char c, unsigned short f)
{
	unsigned char bit;
	for (bit=8; bit; bit--)
	{
		if ((f ^ c) & 1) f = (f >> 1) ^ 0x8408;
		else f = f >> 1;
		c = c >> 1;
	}
	return f;
}


// Hamming ECC codeword table, encode most significant nybble first
const unsigned char hamming_codeword[16] = {
	0b00000000,	0b01010001,	0b01110010,	0b00100011,	0b00110100,	0b01100101,	0b01000110,	0b00010111,
	0b01101000,	0b00111001,	0b00011010,	0b01001011,	0b01011100,	0b00001101,	0b00101110,	0b01111111
};

// Transposed table for decoding
const unsigned char hamming_transpose[3] = {
	0b01001011,	0b00101110,	0b00010111
};

const unsigned char coset[3] = {
	0b00000100,	0b00000010,	0b00000001
};

const unsigned char coset2[8] = {
	0b00000000,	0b00010000,	0b00100000,	0b00000100,	0b01000000,	0b00000001,	0b00001000,	0b00000010
};

unsigned char tx_interleave[8];
unsigned char rx_interleave[8];
unsigned char tx_i_cnt, rx_i_cnt;

void rf12b_send_raw(unsigned char c);
void rf12b_send(unsigned char c);
void rf12b_interrupt(void);


// Starts packet
void rf12b_start_tx(void)
{
	fcs = 0xffff;
	PTT_ON;
	// Send preamble without Hamming encoding, low effective baud rate for AFC adjustment time
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0x0f);
	rf12b_send_raw(0xaa);
	rf12b_send_raw(0xaa);	
	rf12b_send_raw(0x2d);
	rf12b_send_raw(0xd4);
	tx_i_cnt = 0;		// Reset interleave buffer count
}

// Ends packet
void rf12b_end_tx(void)
{
	fcs ^= 0xffff;	

//FIXME: this won't handle CRCs that come out to escape sequences
	rf12b_send(fcs & 0xff);
	rf12b_send(fcs >> 8);
	rf12b_send(FEND);
	rf12b_send(FEND);
	rf12b_send(FEND);
	rf12b_send(FEND);
	// Pad interleaving matrix with FEND characters
#ifdef USE_HAMMING
	while (tx_i_cnt) 	rf12b_send(FEND);
#endif
	while (spi_poll() == 0) reset_watchdog;

	PTT_OFF;
}


// Transmit data without interleaving or Hamming encoding
void rf12b_send_raw(unsigned char c)
{
//	while (spi_poll() == 0) reset_watchdog;
	// Wait for SDO to go high
	SPSCK_LOW;
	SS_LOW;
	SPIC1 = 0b00010000; // Disable SPI
	while (!MISO);
	SPIC1 = 0b01010000; // Enable SPI
	spi16(0xb800 | c);
}


// Send with interleaving and Hamming encoding
void rf12b_send(unsigned char c)
{
#ifdef USE_HAMMING
	// Look up Hamming codewords and add to interleave matrix
	tx_interleave[tx_i_cnt++] = hamming_codeword[c >> 4];
	tx_interleave[tx_i_cnt++] = hamming_codeword[c & 0x0f];
	if (tx_i_cnt == 8)
	{
		unsigned char n;
		// We have 8 codewords to be transmitted (7 bits each)
		for (n = 0; n < 7; n++)
		{
			c = 0;
			for (tx_i_cnt = 0; tx_i_cnt < 8; tx_i_cnt++)
			{
				c <<= 1;
				if (tx_interleave[tx_i_cnt] & 0x40) c |= 1;
				tx_interleave[tx_i_cnt] <<= 1;
			}
			// 7 bytes get sent
			rf12b_send_raw(c);
		}
		tx_i_cnt = 0;
	}
#else
	rf12b_send_raw(c);
#endif
}

#pragma inline
unsigned char rf12b_read(void)
{
	char c;
	SS_LOW;
	// Read and discard 16 bits
	while (!(SPIS & 0x20));
	SPID = 0;
	while (!(SPIS & 0x80));
	c = SPID;
	while (!(SPIS & 0x20));
	SPID = 0;
	while (!(SPIS & 0x80));
	c = SPID;
	// Get FIFO data
	while (!(SPIS & 0x20));
	SPID = 0;
	// Wait for receiver full
	while (!(SPIS & 0x80));
	c = SPID;
	SS_HI;
	return c;
}

void rf12b_rx_fifo_reset(void)
{
	spi16(CMD_FIFO_MODE);				// FIFO interrupt level, sync pattern, start on sync word
	spi16(CMD_FIFO_MODE | FIFO_FF | FIFO_DR);		// Enable FIFO fill
	rx_i_cnt = 0;
	rxcount = 0;
	escape = 0;
}

void rf12b_init(void)
{
	tx_i_cnt = 0;		// Reset interleave buffer count
	rx_i_cnt = 0;
	rf12b_read();
//	spi16(CMD_CONFIG | CONFIG_EF | CONFIG_EL | CONFIG_BAND_915);		// 915 MHz mode (use 80d8 for 433), enable TX register, enable RX FIFO, 12.5 pF caps


// Test with FIFO disabled
	spi16(CMD_CONFIG | CONFIG_EL | CONFIG_BAND_915);		// 915 MHz mode (use 80d8 for 433), enable TX register, enable RX FIFO, 12.5 pF caps

	
	spi16(CMD_POWER | POWER_ER | POWER_EBB | POWER_ES | POWER_EX);		// Enable crystal oscillator + RX and MCU clock output
	
	// Low 12 bits set frequency. Base is 900 MHz, tuning steps are 7.5 kHz
	spi16(CMD_FREQUENCY + ((915000000 - 900000000) / 7500));				// Set frequency to 915 MHz
	spi16(0xcc77);		// MCU clock speed, dithering disabled, PLL bandwidth

//	spi16(CMD_DATA_RATE | DATA_RATE_57600);									// Set data rate
//	spi16(CMD_RX_CTL | RX_BW_134K | RX_GAIN_0 | RX_RSSI_103);			// Fast VDI response, set baseband bandwidth, gain 0, threshold -103 dBm
//	spi16(CMD_TX_CTL | TX_DEV_90K | TX_POWER_0);								// 90 kHz deviation, power 0 dB
/*
	// Production MeshSync
	spi16(CMD_DATA_RATE | DATA_RATE_38400);									// Set data rate
	spi16(CMD_RX_CTL | RX_BW_134K | RX_GAIN_0 | RX_RSSI_103);			// Fast VDI response, set baseband bandwidth, gain 0, threshold -103 dBm
	spi16(CMD_TX_CTL | TX_DEV_90K | TX_POWER_0);								// 90 kHz deviation, power 0 dB
*/
	spi16(CMD_DATA_RATE | DATA_RATE_4800);									// Set data rate
	spi16(CMD_RX_CTL | RX_BW_67K | RX_GAIN_0 | RX_RSSI_103);			// Fast VDI response, set baseband bandwidth, gain 0, threshold -103 dBm
	spi16(CMD_TX_CTL | TX_DEV_45K | TX_POWER_0);								// 90 kHz deviation, power 0 dB


//	spi16(CMD_DATA_FILTER);											// Clock recovery auto-lock disabled, digital filter, DQD threshold

// Analog filter test
	spi16(CMD_ANALOG_FILTER);											// Clock recovery auto-lock disabled, analog filter, DQD threshold


	rf12b_rx_fifo_reset();
	spi16(0xc4f7);		// AFC config
	spi16(0xe000);		// Wake-up timer disabled
	spi16(0xc80e);		// Low duty cycle disabled
	spi16(0xc000);		// Low battery threshold 2.2v, MCU clock 1 MHz
//	spi16(0xc0e0);		// Low battery threshold 2.2v, MCU clock 10 MHz
	wait(2);
	escape = 0;
	rxcount = 0;
//	*(unsigned short*)tx_next_packet = 0;						// Clear next buffer size
}

// Hamming decode - multiplies by transposed table to find syndrome and corrects
unsigned char hamming_decode(unsigned char codeword)
{
	unsigned char col, syndrome, c, parity, bit;

	for (col = 0, syndrome = 0; col < 3; col++)
	{
		c = hamming_transpose[col] & codeword;
		parity = 0;
		for (bit = 0; bit < 8; bit++)
		{
			if (c & 0x80) parity ^= 0xff;
			c <<= 1;
		}
		if (parity)
		{
			// Syndrome is non-zero
			syndrome |= coset[col];
		}
	}
	c = codeword ^ coset2[syndrome];			// Correct modified bit
	return c & 0x0f;
}


// Send data with KISS-style framing
void send_data(void *data, char length)
{
	char *p;
	p = data;
	while (length)
	{
		// Escape frame end to escape and transposed frame end
		if (*p == FEND)
		{
			rf12b_send(FESC);
			rf12b_send(TFEND);
		}
		// Escape escape to escape and transposed escape
		else if (*p == FESC)
		{
			rf12b_send(FESC);
			rf12b_send(TFESC);
		}
		else rf12b_send(*p);
		fcs = crc_calc(*p, fcs);
		p++;
		length--;
	}
}


void rf12b_rx(void)
{
	unsigned char c, b;
	
	c = rf12b_read();
	// Ignore extra synchronization patterns (not valid for start of packet anyway)
//	if (!rxcount && (c == 0x2d || c == 0xd4)) return;
	
#ifdef USE_HAMMING
	rx_interleave[rx_i_cnt++] = c;
	
	if (rx_i_cnt == 7)
	{
		// Got full interleave matrix, will decode to 8 codewords / 4 bytes
		unsigned char temp_matrix[8];
		unsigned char bit_cnt, word_cnt;

		rx_i_cnt = 0;

		for (bit_cnt = 0; bit_cnt < 8; bit_cnt++)
		{
			b = 0;
			for (word_cnt = 0; word_cnt < 7; word_cnt++)
			{
				b <<= 1;
				if (rx_interleave[word_cnt] & 0x80) b |= 1;
				rx_interleave[word_cnt] <<= 1;
			}
			temp_matrix[bit_cnt] = b;
		}
		
		// We now have 4 bytes encoded as 8 codewords
		
		for (word_cnt = 0; word_cnt < 8; word_cnt++)
		{
			b = 0;
			b = hamming_decode(temp_matrix[word_cnt++]) << 4;
			b |= hamming_decode(temp_matrix[word_cnt]);
#else
			b = c;
#endif
			if (rxcount >= RX_BUFFER_SIZE-1)
			{
				// Overflow, reset
				rf12b_rx_fifo_reset();
				return;
			}
			
			if (b == FEND)
			{
				unsigned char rxc;
				rxc = rxcount;
				rf12b_rx_fifo_reset();
				// End of frame
				if (rxc > MIN_PACKET_SIZE)		// Drop 'runt' packets
				{
					unsigned short calc_fcs;
					unsigned char i;
					calc_fcs = 0xffff;

					for (i=0; i < rxc-2; i++)
					{
						calc_fcs = crc_calc(rx_buffer[i], calc_fcs);
					}
					calc_fcs ^= 0xffff;					
					fcs = rx_buffer[rxc-2];
					fcs |= rx_buffer[rxc-1] << 8;
					rxc -= 2;

					if (fcs == calc_fcs)
					{
						// Packet is OK - process
						if (rxc == sizeof(message_type))
						{	
							RaiseEvent(EVENT_PACKET_RECEIVED);
							memcpy(&incoming_msg, rx_buffer, sizeof(message_type));
						}
					}
				}
				return;
			}
			if (escape)
			{
				if (b == TFESC) b = FESC;
				if (b == TFEND) b = FEND;
				escape = 0;
			}
			else if (b == FESC)
			{
				escape = 1;
				return;
			}
			rx_buffer[rxcount++] = b;
#ifdef USE_HAMMING
		}
	}
#endif
}

interrupt void rf12b_interrupt(void)
{
	// Loop while RX FIFO is full (Check DCLK)
	while (FFIT)
	{
		reset_watchdog;
		rf12b_rx();
	}
	KBISC |= 4;
}

