#include <util/delay.h>
#include <avr/sleep.h>
#include <stdint.h>
#include <avr/io.h>
//#include <avr/interrupt.h>
//#include <stdlib.h>

#include "display.h"

const uint8_t display_charmap[][2] = {

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



uint8_t display_buffer[DISPLAY_SIZE] = {0};

uint8_t get_mapped_char(char c) {

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


void display_putchar(char c) {
	display_buffer[0] = get_mapped_char(c);
	display_buffer[1] = get_mapped_char(c);
	display_buffer[2] = get_mapped_char(c);

	display_write();
}

inline void xlat_trigger() {
	DISPLAY_PIN_HIGH(XLAT);
	DISPLAY_PIN_LOW(XLAT);	
}

inline void sclk_trigger() {
	DISPLAY_PIN_HIGH(SCLK);
	DISPLAY_PIN_LOW(SCLK);
}

void display_write() {	
	//each element of the regsiter contains 12 bits...
	//^^what does that comment mean???
	
	//foreach 7-segment display...
	for ( uint8_t c = 0; c < DISPLAY_SIZE; c++ ) {
		for ( uint8_t bit = 0; bit < 8; bit++ ) {
			if ( (display_buffer[c]>>bit)&0x01 )
				DISPLAY_PIN_HIGH(SOUT);
			else
				DISPLAY_PIN_LOW(SOUT);
		
			sclk_trigger();
		}
	}

	xlat_trigger();
}




inline void display_init() {

	DISPLAY_PORT.DIRSET = DISPLAY_PIN(SCLK) | DISPLAY_PIN(SOUT) | DISPLAY_PIN(XLAT);

	//Data is read from SOUT on rising edge
	DISPLAY_PIN_LOW(SCLK);

	//Data is shifted when XLAT is high
	
	DISPLAY_PIN_LOW(XLAT);

	//set everything high.
	DISPLAY_PIN_HIGH(SOUT);
	
	for ( uint8_t i = 0; i < DISPLAY_SIZE; ++i ) 
		display_buffer[i] = 0;

	display_write();

	_delay_ms(1);
}
