#define EVENT_ANY 0xffffffff
#define EVENT_PACKET_RECEIVED 	0x1
#define EVENT_SECOND					0x2
#define EVENT_SLOT					0x4
#define EVENT_PATTERN_TICK			0x8

#define RaiseEvent(x) (events |= (x))
#define ClearEvent(x) (events &= ~(x))
#define CheckEvent(x) (events & (x))

extern unsigned volatile int events;
