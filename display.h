#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_PORT_CONCAT3(a,b,c) a##b##c
#define DISPLAY_PIN(id) DISPLAY_PORT_CONCAT3(PIN,id,_bm)
#define DISPLAY_PIN_HIGH(pin) DISPLAY_PORT.OUTSET = DISPLAY_PIN(pin)
#define DISPLAY_PIN_LOW(pin) DISPLAY_PORT.OUTCLR = DISPLAY_PIN(pin)


void display_init(void);
void display_putchar(char);
void display_puts(char str[]);
void display_test(void);
void display_puti(uint8_t n);
#endif
