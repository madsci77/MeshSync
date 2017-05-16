// Status flags

typedef struct {
	unsigned rxbuffer_lock		: 1;
} status_flags;

extern status_flags flags;
