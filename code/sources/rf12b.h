// RF12B transceiver header

void rf12b_init(void);
unsigned char rf12b_read(void);
void rf12b_send(unsigned char c);
void rf12b_interrupt(void);
void rf12b_start_tx(void);
void rf12b_end_tx(void);

// Configuration setting command (fixed at default load capacitance)
#define CMD_CONFIG	0x8008
// el enables internal data register
#define CONFIG_EL		0x0080
// ef enables FIFI mode - if ef=0 DATA and DCLK are used for data and clock output
#define CONFIG_EF		0x0040
// Frequency band selection
#define CONFIG_BAND_433 0x0010
#define CONFIG_BAND_868 0x0020
#define CONFIG_BAND_915 0x0030

// Power management command
#define CMD_POWER		0x8200
// er enables receiver chain
#define POWER_ER		0x0080
// ebb switches baseband circuit on separately
#define POWER_EBB		0x0040
// et switches on PLL and PA and starts transmission
#define POWER_ET		0x0020
// es turns on synthesizer
#define POWER_ES		0x0010
// ex turns on crystal oscillator
#define POWER_EX		0x0008
// eb turns on low battery detector
#define POWER_EB		0x0004
// ew enables wake-up timer
#define POWER_EW		0x0002
// dc disables clock output
#define POWER_DC		0x0001

// Frequency setting command
#define CMD_FREQUENCY	0xA000

// Data rate command
#define CMD_DATA_RATE	0xC600
//BR = 10000 / 29 / (R+1) / (1+cs · 7) [kbps]
#define DATA_RATE_CS		0x0080
#define DATA_RATE_600	DATA_RATE_CS | 81
#define DATA_RATE_1200	DATA_RATE_CS | 40
#define DATA_RATE_2400	DATA_RATE_CS | 20
#define DATA_RATE_4800	71
#define DATA_RATE_9600	35
#define DATA_RATE_19200	17
#define DATA_RATE_28800	11
#define DATA_RATE_38400	8
#define DATA_RATE_57600	5
#define DATA_RATE_115200 2

// Receiver control command
#define CMD_RX_CTL	0x9000
// Change pin 16 from interrupt to VDI
#define RX_CTL_VDI	0x0400
// VDI speeed
#define RX_VDI_FAST	0x0000
#define RX_VDI_MED	0x0100
#define RX_VDI_SLOW	0x0200
#define RX_VDI_ON		0x0300
// RX bandwidth
#define RX_BW_400K	0x0020
#define RX_BW_340K	0x0040
#define RX_BW_270K	0x0060
#define RX_BW_200K	0x0080
#define RX_BW_134K	0x00A0
#define RX_BW_67K		0x00C0
// RX LNA gain select (- dB)
#define RX_GAIN_0		0x0000
#define RX_GAIN_6	0x0008
#define RX_GAIN_14	0x0010
#define RX_GAIN_20	0x0018
// RSSI threshold (- dB)
#define RX_RSSI_103	0x0000
#define RX_RSSI_97	0x0001
#define RX_RSSI_91	0x0002
#define RX_RSSI_85	0x0003
#define RX_RSSI_79	0x0004
#define RX_RSSI_73	0x0005

// Data filter command
#define CMD_DATA_FILTER	0xC22C
#define CMD_ANALOG_FILTER 0xC23C

// Auto lock
#define FILTER_AL		0x0080
// Clock recovery lock control (if auto lock = 0)
#define FILTER_ML		0x0040
// S: 0 = digital filter, 1 = analog RC filter
#define FILTER_S		0x0010
// DQD threshold (not implemented)

// FIFO and reset mode command - interrupt level fixed at 8 bits
#define CMD_FIFO_MODE	0xCA80
// sp - sync pattern: 0 = 0x2DD4, 1 = 0xD4
#define FIFO_SP			0x0008
// al - 0 = fill FIFO on sync pattern, 1 = always fill
#define FIFO_AL			0x0004
// 1 = FIFO fill enable
#define FIFO_FF			0x0002
// 1 = non-sensitive reset
#define FIFO_DR			0x0001

// Transmit control command
#define CMD_TX_CTL		0x9800
// mp = modulation polarity
#define TX_MP				0x0100
// Third nybble is deviation, M+1 * 15 kHz
#define TX_DEV_15K		0x0000
#define TX_DEV_30K		0x0010
#define TX_DEV_45K		0x0020
#define TX_DEV_60K		0x0030
#define TX_DEV_75K		0x0040
#define TX_DEV_90K		0x0050
#define TX_DEV_105K		0x0060
#define TX_DEV_120K		0x0070
#define TX_DEV_135K		0x0080
#define TX_DEV_150K		0x0090
#define TX_DEV_165K		0x00A0
#define TX_DEV_180K		0x00B0
#define TX_DEV_195K		0x00C0
#define TX_DEV_210K		0x00D0
#define TX_DEV_225K		0x00E0
#define TX_DEV_240K		0x00F0
// Power setting, - dB
#define TX_POWER_0		0x0000
#define TX_POWER_2_5		0x0001
#define TX_POWER_5		0x0002
#define TX_POWER_7_5		0x0003
#define TX_POWER_10		0x0004
#define TX_POWER_12_5	0x0005
#define TX_POWER_15		0x0006
#define TX_POWER_17_5	0x0007

/* Optimum BW and deviation settings:

Rate		BW		Dev
1200		67		45
2400		67		45
4800		67		45
9600		67		45
19200		67		45
38400		134	90
57600		134	90
115200	200	120

*/