/*

	MeshSync

*/

#include "hardware.h"
#include "status.h"
#include "utility.h"
#include "events.h"
#include "rf12b.h"
#include "meshsync.h"

#define HISTCOUNT	5					// History levels to keep for membership
#define CYCLE_SECONDS 1				// Seconds per net cycle
#define GROUP_TIMEOUT 6				// Timeout before reverting to solo operation
#define MAXNODE 31					// Max node number
#define MAX_PROGRAM 10

unsigned volatile int events;		// Pending system-wide events
volatile unsigned char debounce;

unsigned short clock_ticks,			// Incremented every tick, resets every second
					tempo_ticks,			// Ticks for lighting sequence tempo
					pattern_seconds,		// Seconds pattern has been running
				 	second;					// Second-of-hour for timeslotting

char ctemp[10];				// Multi-use temp shared with GPS code

void send_data(void *data, char length);
void lighting_program(void);

const char MyID = 2;				// Start numbering at 2 - 0 and 1 reserved
unsigned char MyGroup = MyID;
unsigned char GroupTimeout = 0;
unsigned char RelativePosition = 0;			// Relative position within group
unsigned char NodesInGroup = 1;
unsigned long DirectHeard[HISTCOUNT], CurrentMembers[HISTCOUNT];
unsigned char PatternProgram = 0;
unsigned char PatternSequence = 0;
unsigned char PatternTempo = 61;				// 1 pattern tick = 1/15.25 second
unsigned short lfsr = 0xace1, prng;					// For PRNG


message_type msg, incoming_msg;

// Min, max, tempo, duration
pattern_type patterns[] = {
	0, 255, 0, 10,							// Always on
	2, 255, 61, 15,						// Linear 1 by 1
	3, 255, 61,	15, 						// Bouncing 1 by 1
	2, 255, 61,	15,						// Linear 1 by 1 inverted
	3, 255, 61,	15,						// Bouncing 1 by 1 inverted
	4, 255, 61,	15,						// Linear 2 by 2 from opposite ends
	3, 255, 61,	10,						// Odds and evens
	3, 255, 61,	15,						// Thirds
	0, 255, 30,	8,							// Sparkle
	2, 255, 61,	10,						// Linear / Flash
	0, 255, 61, 10							// All on / off
};

/*

TODO

Simulation reverses sequence of time slots each cycle to deal with long lines
May need to duplicate

*/

unsigned long OrHistory(unsigned long *pl, unsigned char count)
{
	unsigned char c;
	unsigned long l;
	l = 0;
	for (c = 0; c < count; c++)
	{
		l |= *pl;
		pl++;
	}
	return l;
}

// Main program entry point
void main(void)
{
	mcu_setup();
	wait(ticks_per_sec / 6);
	rf12b_init();
	EnableInterrupts;

	RED_ON;
	GREEN_ON;
	wait(ticks_per_sec / 2);
	RED_OFF;
	GREEN_OFF;
	
	// SRS sources - POR, pin, COP, ILOP, ILAD, reserved, LVD, reserved
	if ((SRS & 0x80) == 0)
	{
		// Not a POR - signal error code
		wait(ticks_per_sec / 2);
		if (SRS & 0x40)	// Pin reset
		{
			RED_ON;
		}
		else if (SRS & 0x20)	// COP reset
		{
			GREEN_ON;
		}
		else if (SRS & 0x10)	// ILOP reset
		{
			RED_ON;
			GREEN_ON;
		}
		else if (SRS & 0x08)	// ILAD reset
		{
			RED_ON;
			wait(ticks_per_sec/2);
			GREEN_ON;		
		}
		else if (SRS & 0x02)	// LVD reset
		{
			GREEN_ON;
			wait(ticks_per_sec/2);
			RED_ON;			
		}
		wait(ticks_per_sec/2);
		GREEN_OFF;
		RED_OFF;		
	}
	
	// Test routines for RF12B transmitter

	// Main program loop.  Only exit is to bootloader on 'R' command.
	for (;;)
	{

		reset_watchdog;

		// Save power by waiting for an interrupt
		while (!CheckEvent(EVENT_ANY))
		{
//			Wait;
			reset_watchdog;
		}

		// Process incoming packets
		if (CheckEvent(EVENT_PACKET_RECEIVED))
		{
			unsigned char n;
			
			ClearEvent(EVENT_PACKET_RECEIVED);
			GREEN_ON;

			// Sync clock to lower IDs in group, or any group if we're solo to avoid collisions
			if (incoming_msg.Group == MyGroup || (MyID == MyGroup && NodesInGroup == 1))
			{
				clock_ticks = (incoming_msg.Source * ticks_per_slot) + packet_latency_ticks;			
			}
			
			if (incoming_msg.Group == MyGroup)
			{
				// Synchronize tick clock and program to any sender in group with a lower ID
				if (incoming_msg.Source < MyID)
				{
					PatternProgram = incoming_msg.Program;
					PatternSequence = incoming_msg.Sequence;
					PatternTempo = incoming_msg.Tempo;
				}

				// If received group ID is our group ID, update timeout
				GroupTimeout = GROUP_TIMEOUT;
				
				// Record that we heard this station direct
				DirectHeard[0] |= 1L << incoming_msg.Source;
				
				// Record those heard by sending station
				DirectHeard[0] |= incoming_msg.DirectHeard;
				
				// If we're not group leader, accept membership list
				if (MyGroup != MyID)
				{
					CurrentMembers[0] |= incoming_msg.CurrentMembers;
				}
			}
			
			if (incoming_msg.Group < MyGroup)
			{
				// Heard from a group with a lower ID than ours - join it
				MyGroup = incoming_msg.Group;
				// Throw out our known station lists and accept new ones
				DirectHeard[0] = incoming_msg.DirectHeard;
				CurrentMembers[0] = incoming_msg.CurrentMembers;
				// Clear history for previous group
				for (n = 1; n < HISTCOUNT; n++)
				{
					CurrentMembers[n] = 0;
					DirectHeard[n] = 0;
				}
			}
		}
		
		// Process net cycle events
		if (CheckEvent(EVENT_SECOND))
		{
			ClearEvent(EVENT_SECOND);
			second++;
			
			pattern_seconds++;
#ifndef HAS_SWITCH
			// If we're group master, handle program selection
			if (MyID == MyGroup && NodesInGroup > 1 && pattern_seconds > patterns[PatternProgram].duration)
			{
				PatternSequence = 0;
				PatternProgram++;
				if (PatternProgram > MAX_PROGRAM) PatternProgram = 0;
				// Find a valid pattern for the number of nodes we have
				while (patterns[PatternProgram].min_nodes > NodesInGroup)
				{
					if (PatternProgram > MAX_PROGRAM) PatternProgram = 0;
					PatternProgram++;
				}
				PatternTempo = patterns[PatternProgram].tempo;
				pattern_seconds = 0;									
			}
#endif
			if (second >= CYCLE_SECONDS)
			{
				unsigned char h, cnt, pos;
				second = 0;
				// New net cycle
				if (GroupTimeout)
				{
					if (--GroupTimeout == 0)
					{
						// Haven't heard from our group in too long, fall back to solo operation
						MyGroup = MyID;						
						PatternProgram = 0;
					}
				}
				
				if (MyID != MyGroup)
				{
					// Not group leader - check to see if we've lost our leader
					if ((OrHistory(CurrentMembers, HISTCOUNT) & (1L << MyGroup)) == 0)
					{
						// Fall back to our own group and null program
						MyGroup = MyID;						
						PatternProgram = 0;						
						// Simulator searches for next-lowest member - we'll just wait for this to happen in normal cycle
					}				
				}
				
				// Cycle history lists
				for (h = HISTCOUNT-1; h; h--)
				{
					CurrentMembers[h] = CurrentMembers[h-1];
				}
				
				// If we're not group leader, erase current member list
				if (MyID != MyGroup)
				{
					CurrentMembers[0] = 0;
				}
				else
				{
					// Otherwise aggregate all heard stations in history
					CurrentMembers[0] = 0;
					for (h = 1; h < HISTCOUNT; h++)
					{
						CurrentMembers[0] |= DirectHeard[h];					
					}
					// Include ourselves
					CurrentMembers[0] |= 1L << MyID;
					// Cycle direct heard history
					for (h = HISTCOUNT-1; h; h--)
					{
						DirectHeard[h] = DirectHeard[h-1];
					}				
				}
				DirectHeard[0] = 0;		// Clear direct heard list for this cycle
				
				// Find our relative position in the group based on updated membership for this cycle
				cnt = 0;
				pos = 0;
				for (h = 2; h < MAXNODE; h++)	// Ignore reserved controller IDs 0 and 1
				{
					if (OrHistory(CurrentMembers, HISTCOUNT) & (1L << h))
					{
						// Count all members of group
						cnt++;
						// Count lower IDs
						if (h < MyID) pos++;
					}
				}
				RelativePosition = pos;
				NodesInGroup = cnt;			
			}
		}
		
		if (CheckEvent(EVENT_SLOT))
		{
			ClearEvent(EVENT_SLOT);
			msg.Source = MyID;
			msg.Group = MyGroup;
			msg.Program = PatternProgram;
			msg.Sequence = PatternSequence;
			msg.Tempo = PatternTempo;
			msg.DirectHeard = DirectHeard[0];
			msg.CurrentMembers = CurrentMembers[0];

			RED_ON;
			DisableInterrupts;
			rf12b_start_tx();
			send_data(&msg, sizeof(msg));
			rf12b_end_tx();
			EnableInterrupts;	
		}
		
		if (CheckEvent(EVENT_PATTERN_TICK))
		{
			ClearEvent(EVENT_PATTERN_TICK);
			lighting_program();		
		}

	}
}


// Handle actual lighting sequences
void lighting_program(void)
{
	unsigned char x;
	x = 0;
	
	switch (PatternProgram)
	{
		case 0:
			// Always on (only program available if solo)
			x = 1;
			break;
			
		case 1:
			// Linear 1 by 1
			if (PatternSequence >= NodesInGroup) PatternSequence = 0;
			if (RelativePosition == PatternSequence) x = 1;
			break;
			
		case 2:
			// Bouncing 1 by 1
			if (PatternSequence >= NodesInGroup * 2) PatternSequence = 0;
			if (RelativePosition == PatternSequence) x = 1;
			if (PatternSequence - NodesInGroup == NodesInGroup - RelativePosition) x = 1;
			break;
			
		case 3:
			// Linear 1 by 1 inverted (normally on)
			x = 1;
			if (PatternSequence >= NodesInGroup) PatternSequence = 0;
			if (RelativePosition == PatternSequence) x = 0;
			break;

		case 4:
			// Bouncing 1 by 1 inverted (normally on)
			x = 1;
			if (PatternSequence >= NodesInGroup * 2) PatternSequence = 0;
			if (RelativePosition == PatternSequence) x = 0;
			if (PatternSequence - NodesInGroup == NodesInGroup - RelativePosition) x = 0;
			break;
		
		case 5:
			// Linear 2 by 2 from opposite ends
			if (PatternSequence >= NodesInGroup - 1) PatternSequence = 0;
			if (RelativePosition == PatternSequence) x = 1;
			if (RelativePosition == NodesInGroup - 1 - PatternSequence) x = 1;
			break;
		
		case 6:
			// Odds / evens
			if ((PatternSequence ^ RelativePosition) & 1) x = 1;
			break;
			
		case 7:
			// Thirds
			switch (PatternSequence)
			{
				case 0:
					if (RelativePosition % 3 == 0) x = 1;
					break;
					
				// 1 is off
				
				case 2:
					if (RelativePosition % 3 == 0) x = 1;
					if (RelativePosition % 3 == 1) x = 1;
					break;
					
				// 3 is off
				
				case 4:
					x = 1;
					break;
					
				// 5 is off
				
				case 6:
					if (RelativePosition % 3 == 1) x = 1;
					if (RelativePosition % 3 == 2) x = 1;
					break;
				
				// 7 is off
				
				case 8:
					if (RelativePosition % 3 == 2) x = 1;
					break;
					
				case 9:
					x = 1;
					PatternSequence = 255;
					break;
			}
			
		case 8:
			// Random sparkle
			if (prng & 1) x = 1;
			break;
			
		case 9:
			// Alternating linear / flash
			if (PatternSequence >= NodesInGroup * 4) PatternSequence = 0;
			switch (PatternSequence % 4)
			{
				case 0:
					if (PatternSequence/4 == RelativePosition) x = 1;
					break;
				
				// 1 is off
				
				case 2:
					x = 1;
					break;
					
			 	// 3 is fof
								
			}
			break;
		
		case 10:
			// All on/off
			if (PatternSequence & 1) x = 1;
			break;		
	}
		
	if (x)
	{
		RELAY_ON;
	}
	else
	{
		RELAY_OFF;
	}
	
	PatternSequence++;	
}



#pragma mark -

// --- Interrupt Handlers ---

// Null handler, to catch any stray interrupts

interrupt void NullHandler(void)
{
}


// Timebase interrupt handler - called (ticks_per_sec) times per second

interrupt void tick(void)
{
	unsigned char bit;
	SRTISC |= 0x40;

	// Update random number generator (x^16 + x^14 + x^13 + x^11 + 1)
	bit  = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ) & 1;
	lfsr =  (lfsr >> 1) | (bit << 15);
	prng <<= 1;
	if (lfsr & 0x8000) prng |= 1;

#ifdef HAS_SWITCH
	if (debounce) debounce--;	
	if ((PTBD & 1) == 0 && !debounce)
	{
		debounce = 255;
		PatternProgram++;
		if (PatternProgram > MAX_PROGRAM) PatternProgram = 0;
		PatternSequence = 0;
		PatternTempo = patterns[PatternProgram].tempo;
	}
#endif

	clock_ticks++;
	// If we're in wait mode, decrement wait counter
	if (wait_ticks)
	{
		wait_ticks--;
	}
	else
	{
		RED_OFF;
		GREEN_OFF;	
	}

	// Lighting sequence tempo
//	tempo_ticks++;
	if (clock_ticks % PatternTempo == 0)
	{
//		tempo_ticks = 0;
		RaiseEvent(EVENT_PATTERN_TICK);	
	}

	// Poll FIFO full interrupt in case we missed one
//	if (PTAD & 0x02) rf12b_interrupt();
	
	// Our TX time slot occurs once per second at fixed time based on ID
	if (clock_ticks == (ticks_per_slot * MyID))
		RaiseEvent(EVENT_SLOT);
	
	// May overrun past ticks_per_sec if busy transmitting
	if (clock_ticks >= ticks_per_sec)
	{
		RaiseEvent(EVENT_SECOND);
		clock_ticks = 0;
	}

} 
