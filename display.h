#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_PORT PORTC

//LE
#define XLAT 4
#define SCLK 7
#define SOUT 5


#define DISPLAY_PORT_CONCAT3(a,b,c) a##b##c
#define DISPLAY_PIN(id) DISPLAY_PORT_CONCAT3(PIN,id,_bm)
#define DISPLAY_PIN_HIGH(pin) DISPLAY_PORT.OUTSET = DISPLAY_PIN(pin)
#define DISPLAY_PIN_LOW(pin) DISPLAY_PORT.OUTCLR = DISPLAY_PIN(pin)


//#define DISPLAY_CHARMAP_SIZE 28


//three digits.
#define DISPLAY_SIZE 3


void sge_on(uint8_t n);
void seg_off(uint8_t n);
void xlat_trigger(void);
void sclk_trigger(void);
void display_write(void);
void display_init(void);
void display_putchar(char);
uint8_t get_mapped_char(char);
void display_test(void);
#endif
