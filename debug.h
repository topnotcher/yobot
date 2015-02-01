#ifndef DEBUG_H
#define DEBUG_H

void debug_init(void);
void __debug_write(void *str,const uint8_t size);

static inline void debug_write(void *data,const uint8_t size) {
#ifdef DEBUG
	__debug_write(data,size);
#endif
}

#endif
