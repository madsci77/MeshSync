#define Wait {__asm WAIT;}
#define Stop {__asm STOP;}

extern volatile unsigned short wait_ticks;

void wait(unsigned int ticks);
