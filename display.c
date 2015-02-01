#include <util/delay.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#include <stdint.h>
#include <avr/io.h>
#include "display.h"
#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#define _SCLK_bm DISPLAY_PIN(DISPLAY_SCLK_PIN)
#define _SOUT_bm DISPLAY_PIN(DISPLAY_SOUT_PIN)
#define _XLAT_bm DISPLAY_PIN(DISPLAY_XLAT_PIN)

static inline void xlat_trigger(void);
static inline void display_write_byte(void);
static void display_puts(char str[]);
static void display_write(void);
static uint8_t get_mapped_char(char);

/**
 * This is a mapping of char => segments. The binary numbers indicate what
 * segments need to be turned on to display the given character
 * e.g. 0 requires all of the outer segments: ABCDEF = 0b11111100
 */
#define DP_BM _BV(0) // decimal point
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
	{ ' ', 0b00000000 },
	{ '-', 0b00000010 },
	{ 'E', 0b10011110 },
	{ 'F', 0b10001110 },
	{ 'c', 0b10011100 },
	{ 'L', 0b00011100 },
	{ 'H', 0b01101110 },
	{ 'P', 0b11001110 },
	{ 'U', 0b01111100 },
	{ 'a', 0b11101110 },
	{ 'S', 0b10110110 }, //==5
	{ 'b', 0b00111110 },
	{ 'd', 0b01111010 },
	{ 'O', 0b11111100 }, //== 0
	{ 't', 0b00011110 }, //looks stupid without context
	{ 'n', 0b00101010 },
	{ 'o', 0b00111010 },
	{ '_', 0b00010000 },
	{ 'r', 0b11001100 },

	//null: leave this last.
	{ '\0',0b00000000 },

};

static uint8_t display_buffer[DISPLAY_SIZE] = {0};

typedef struct {
	uint8_t bytes;
} display_state_t;

static display_state_t state;


static void uart_tx_interrupt_enable(void) {
	USARTC1.CTRLA |= USART_DREINTLVL_LO_gc;
}

static void uart_tx_interrupt_disable(void) {
	USARTC1.CTRLA &= ~USART_DREINTLVL_LO_gc;
}
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

int printf(const char *fmt, ...) {
	static char buf[DISPLAY_SIZE+5] = {0};
	va_list ap;
	va_start(ap,fmt);
	int ret = vsnprintf(buf, DISPLAY_SIZE+5, fmt, ap);
	display_puts(buf);
	return ret;
}


/**
 * Call repeatedly with a delay to test the segments
 */
void display_test() {
	static uint8_t bm = 1;
	for (uint8_t i = 0; i < DISPLAY_SIZE; ++i)
		display_buffer[i] = bm;
	display_write();
	if (bm == 1<<7)
		bm = 1;
	else
		bm <<= 1;
}

/**
 * Write a string to the display
 */
static void display_puts(char str[]) {
	uint8_t i,len,add_dp = 0;
	const uint8_t max = DISPLAY_SIZE-1;
	for (i = 0, len = 0; len < DISPLAY_SIZE && str[i]; ++i) {
		if (str[i] == '.' && len > 0) {
			display_buffer[max-(len-1)] |= DP_BM;
		} else if (str[i] == ':' && len > 0) {
			display_buffer[max-(len-1)] |= DP_BM;
			add_dp = 1;
		} else {
			display_buffer[max-len] = get_mapped_char(str[i]);

			if (add_dp) {
				display_buffer[max-len] |= DP_BM;
				add_dp = 0;
			}
		
			len++;
		}
	}

	for ( ; len < DISPLAY_SIZE; ++len)
		display_buffer[max-len] = 0;

	display_write();
}

void clear(void) {
	display_puts("");
}

static inline void xlat_trigger() {
	DISPLAY_PIN_HIGH(DISPLAY_XLAT_PIN);
	DISPLAY_PIN_LOW(DISPLAY_XLAT_PIN);
}

static inline void display_write_byte(void) {
	USARTC1.DATA = display_buffer[state.bytes++];
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
		uart_tx_interrupt_enable();
		state.bytes = 0;
	}
}

ISR(USARTC1_DRE_vect) {
	display_write_byte();

	if (state.bytes == DISPLAY_SIZE) {
		uart_tx_interrupt_disable();
	}
}
ISR(USARTC1_TXC_vect) {
	xlat_trigger();
}

/**
 * Perform initialization of state and hardware
 */
void display_init() {

/*#define DISPLAY_SCLK_PIN 7
#define DISPLAY_SOUT_PIN 5
#define DISPLAY_XLAT_PIN 4
	TXD1 = 7, RXD1 = 6, XCK1 = 5
	*/
/*
	DISPLAY_PORT.DIRSET = _SCLK_bm | _SOUT_bm | _XLAT_bm;
	DISPLAY_PORT.OUTSET = _SCLK_bm | _SOUT_bm;
	DISPLAY_PORT.OUTCLR = _XLAT_bm;
	
	//DISPLAY_SPI.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_PRESCALER_DIV128_gc | SPI_DORD_bm;
	DISPLAY_SPI.INTCTRL = SPI_INTLVL_LO_gc;
	*/

	//0 = fastest; 1 = 8mhz
	uint16_t bsel = 8;
	int8_t bscale = 0;

	//BSEL
	USARTC1.BAUDCTRLA = (uint8_t)( bsel & 0x00FF );
	USARTC1.BAUDCTRLB = (bscale<<USART_BSCALE_gp) | (uint8_t)( (bsel>>8) & 0x0F ) ;

	USARTC1.CTRLA |= USART_TXCINTLVL0_bm;
	USARTC1.CTRLC |= /*USART_CHSIZE_8BIT_gc |*/ USART_CMODE_MSPI_gc /*| _BV(2) | _BV(1)*/;
	USARTC1.CTRLB |= USART_TXEN_bm;

	//xmegaA, p237
	PORTC.DIRSET = PIN7_bm | _XLAT_bm | PIN5_bm;

	PORTC.OUTSET = PIN7_bm;
	PORTC.OUTCLR = _XLAT_bm;

	//PORTC.PIN7CTRL |= PORT_INVEN_bm;

	state.bytes = 0;
	
	for ( uint8_t i = 0; i < DISPLAY_SIZE; ++i ) 
		display_buffer[i] = 0;

	//this will not run until interrupts are enabled.
	display_write();
}
