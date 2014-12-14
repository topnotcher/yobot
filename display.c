#include <util/delay.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <stdint.h>
#include <avr/io.h>
#include "display.h"
#include "config.h"

#define _SCLK_bm DISPLAY_PIN(DISPLAY_SCLK_PIN)
#define _SOUT_bm DISPLAY_PIN(DISPLAY_SOUT_PIN)
#define _XLAT_bm DISPLAY_PIN(DISPLAY_XLAT_PIN)

static inline void xlat_trigger(void);
static inline void display_write_byte(void);
static void display_write(void);
static uint8_t get_mapped_char(char);

/**
 * This is a mapping of char => segments. The binary numbers indicate what
 * segments need to be turned on to display the given character
 * e.g. 0 requires all of the outer segments: ABCDEF = 0b11111100
 */
static const uint8_t display_charmap[][2] = {
	//ABCDEFG.
	{ '0', 0b11111100 },
	{ '1', 0b01100000 },
	{ '2', 0b11011010 },
	{ '3', 0b11110010 },
	{ '4', 0b01100110 },
	{ '5', 0b10110110 },
	{ '6', 0b10111110 }, 
	{ '7', 0b11100000 },
	{ '8', 0b11111110 },
	{ '9', 0b11100110 }, 
	{ '-', 0b00000010 },
	{ 'E', 0b10011110 },
	{ 'F', 0b10001110 },
	{ 'C', 0b10011100 },
	{ 'L', 0b00011100 },
	{ 'H', 0b01101110 },
	{ 'P', 0b11001110 },
	{ 'U', 0b01111100 },
	{ 'A', 0b11101110 },
	{ 'S', 0b10110110 }, //==5
	{ 'b', 0b00111110 },
	{ 'd', 0b01111010 },
	{ 'O', 0b11111100 }, //== 0
	{ 't', 0b00011110 }, //looks stupid without context
	{ 'n', 0b00101010 },
	{ 'o', 0b00111010 },
	{ '_', 0b00010000 },

	//null: leave this last.
	{ '\0',0b00000000 },

};

static uint8_t display_buffer[DISPLAY_SIZE] = {0};

typedef struct {
	//state machine for controlling the SPI
	enum {
		DISPLAY_STATUS_BUSY,
		DISPLAY_STATUS_IDLE
	} status;
	uint8_t bytes;
} display_state_t;

static display_state_t state;

/**
 * Given a character (c), return the segments required to display c
 */
static uint8_t get_mapped_char(char c) {

	for ( uint8_t i = 0; i <= 255; ++i ) {
		if ( display_charmap[i][0] == c )
			return display_charmap[i][1];

		//end of the map
		else if ( display_charmap[i][0] == '\0' ) break;
	}

	return 0;
}

void display_test() {
	for (uint8_t i = 0; i < 8; i++) {
		uint8_t val = 1<<i;
		display_buffer[0] = display_buffer[1] = display_buffer[2] = val;
		display_write();
		_delay_ms(250);
	}
}

/**
 * Write a string to the display
 */
void display_puts(char str[]) {
	for (uint8_t i = 0; i < DISPLAY_SIZE; ++i) 
		display_buffer[2-i] = get_mapped_char(str[i]);
	display_write();
}

/*
 * write a character to the display (fill all digits)
 */
void display_putchar(char c) {
	display_buffer[0] = get_mapped_char(c);
	display_buffer[1] = get_mapped_char(c);
	display_buffer[2] = get_mapped_char(c);

	display_write();
}

/**
 * write an 'integer' to the display
 */
void display_puti(uint8_t n) {
	char str[DISPLAY_SIZE] = {0};
	//least significant digit first
	for (int8_t i = DISPLAY_SIZE-1; i >= 0; --i) {
		uint8_t digit = n%10;

		//don't print leading zeroes
		if (digit == 0 && i < 2)
			break;

		str[i] = digit + 0x30;
		n /= 10;
	}
	display_puts(str);
}

static inline void xlat_trigger() {
	DISPLAY_PIN_HIGH(DISPLAY_XLAT_PIN);
	DISPLAY_PIN_LOW(DISPLAY_XLAT_PIN);
}

static inline void display_write_byte(void) {
	DISPLAY_SPI.DATA = display_buffer[state.bytes++];
}

static void display_write() {	
	//It is possible to call this while the previous data is still being
	//written to the display. However, since the display is a giant shift
	//register, this is irrelevant: any data already written and not latched
	//will be shifted off the end of the register and replaced with the new
	//data. Since the shift registers are buffered, impartial transfers will
	//never be displayed. It is, however, possible that some bytes from the
	//current buffer have been written, then the buffer is replaced, and then
	//the remaining byte is written out before this runs. This is unlikely.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		state.bytes = 0;
		state.status = DISPLAY_STATUS_BUSY;
		display_write_byte();
	}
}

ISR(DISPLAY_SPI_vect) {
	if (state.bytes == DISPLAY_SIZE) {
		xlat_trigger();
		state.status = DISPLAY_STATUS_IDLE;
	} else {
		display_write_byte();
	}
}

/**
 * Perform initialization of state and hardware
 */
void display_init() {

	DISPLAY_PORT.DIRSET = _SCLK_bm | _SOUT_bm | _XLAT_bm;
	DISPLAY_PORT.OUTSET = _SCLK_bm | _SOUT_bm;
	DISPLAY_PORT.OUTCLR = _XLAT_bm;
	
	DISPLAY_SPI.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER_DIV4_gc | SPI_DORD_bm;
	DISPLAY_SPI.INTCTRL = SPI_INTLVL_LO_gc;

	state.status = DISPLAY_STATUS_IDLE;
	state.bytes = 0;
	
	for ( uint8_t i = 0; i < DISPLAY_SIZE; ++i ) 
		display_buffer[i] = 0;

	//this will not run until interrupts are enabled.
	display_write();
}
