#include <stdint.h>

#ifndef MP3_H
#define MP3_H
void mp3_init(void);
void uart_write(uint8_t *buf, uint8_t size);
void uart_flush(void);
void mp3_send_cmd(uint8_t,uint8_t,uint8_t);
void mp3_play(uint8_t num);
#endif
