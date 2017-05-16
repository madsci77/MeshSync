typedef struct
{
	unsigned char Source;
	unsigned char Group;
	unsigned char Program;
	unsigned char Sequence;
	unsigned char Tempo;
	unsigned long DirectHeard;
	unsigned long CurrentMembers;
	unsigned char Reserved[5];
} message_type;		// 12 bytes

typedef struct
{
	unsigned char min_nodes;
	unsigned char max_nodes;
	unsigned char tempo;
	unsigned char duration;
} pattern_type;

extern message_type incoming_msg;
extern unsigned short clock_ticks;