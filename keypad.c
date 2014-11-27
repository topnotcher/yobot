#include "keypad.h"
#include <stdint.h>

volatile uint8_t keymask = 0;

inline void keypad_int_enable(void) {
	KEYPAD_PORT.INTCTRL |= PORT_INT0LVL_MED_gc;
}

inline void keypad_int_disable(void) {
	KEYPAD_PORT.INTCTRL &= ~PORT_INT0LVL_MED_gc;
}

/**
 * Set cols as inputs to scan cols
 */
inline void keypad_init_colscan(void) {
	//set rows to output 
	KEYPAD_PORT.DIRSET = KEYPAD_ROWMASK;
	//set value of rows to 0 
	KEYPAD_PORT.OUTCLR = KEYPAD_ROWMASK;

	//set cols to input 
	KEYPAD_PORT.DIRCLR = KEYPAD_COLMASK;

	//set pullup on cols (well this blows!)
	KEYPAD_PINCTRL(C1) = PORT_OPC_PULLUP_gc;
	KEYPAD_PINCTRL(C2) = PORT_OPC_PULLUP_gc;
	KEYPAD_PINCTRL(C3) = PORT_OPC_PULLUP_gc;
}

/**
 * Set rows as inputs to scan rows
 */
inline void keypad_init_rowscan(void) {
	//output on cols.
	KEYPAD_PORT.DIRSET = KEYPAD_COLMASK;
	//set cols to 0
	KEYPAD_PORT.OUTCLR = KEYPAD_COLMASK;
	KEYPAD_PORT.DIRCLR = KEYPAD_ROWMASK;

	KEYPAD_PINCTRL(R1) = PORT_OPC_PULLUP_gc;
	KEYPAD_PINCTRL(R2) = PORT_OPC_PULLUP_gc;
	KEYPAD_PINCTRL(R3) = PORT_OPC_PULLUP_gc;
	KEYPAD_PINCTRL(R4) = PORT_OPC_PULLUP_gc;
}

inline void keypad_init() {

	//initially, we set up to listen to changes
	//on the column pins.
	KEYPAD_PORT.INT0MASK = KEYPAD_COLMASK;
	keypad_init_colscan();
	keypad_int_enable();
}

KEYPAD_ISR {
	PORTC.OUT ^= PIN1_bm;
	//cols are already setup to scan.
	keymask = ~KEYPAD_PORT.IN & KEYPAD_COLMASK;	

	if ( keymask == 0 )
		return;

	keypad_init_rowscan();

	KEYPAD_SCAN_DELAY();

	keymask |= ~KEYPAD_PORT.IN & KEYPAD_ROWMASK;

	keypad_init_colscan();
}

inline uint8_t keypad_scan() {
	uint8_t tmp = keymask; 
	keymask = 0;
	return tmp;
}

char keypad_getchar(uint8_t keymask) {
	switch (keymask) {
		case KEY_1:
			return '1';
		case KEY_2:
			return '2';
		case KEY_3:
			return '3';
		case KEY_4:
			return '4';
		case KEY_5:
			return '5';
		case KEY_6:
			return '6';
		case KEY_7:
			return '7';
		case KEY_8:
			return '8';
		case KEY_9:
			return '9';
		case KEY_0:
			return '0';
		case KEY_ASTERISK:
			return '*';
		case KEY_POUND:
			return '#';
		default:
			return '?';
	}
}
