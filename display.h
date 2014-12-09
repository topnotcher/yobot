#ifndef DISPLAY_H
#define DISPLAY_H

#define DISPLAY_PORT PORTC
#define DISPLAY_SPI SPIC
#define DISPLAY_SCLK_PIN 7
#define DISPLAY_SOUT_PIN 5
#define DISPLAY_XLAT_PIN 4
#define DISPLAY_SPI_vect SPIC_INT_vect





#define DISPLAY_PORT_CONCAT3(a,b,c) a##b##c
#define DISPLAY_PIN(id) DISPLAY_PORT_CONCAT3(PIN,id,_bm)
#define DISPLAY_PIN_HIGH(pin) DISPLAY_PORT.OUTSET = DISPLAY_PIN(pin)
#define DISPLAY_PIN_LOW(pin) DISPLAY_PORT.OUTCLR = DISPLAY_PIN(pin)


//#define DISPLAY_CHARMAP_SIZE 28


//three digits.
#define DISPLAY_SIZE 3


void sge_on(uint8_t n);
void seg_off(uint8_t n);
void display_write(void);
void display_init(void);
void display_putchar(char);
void display_puts(char str[]);
uint8_t get_mapped_char(char);
void display_test(void);
#endif
