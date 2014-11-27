#include <util/atomic.h>
#include <stdint.h>
#include "keypad.h"

/**
 * 3x4 matrix keypad driver.
 *
 * How this works:
 *
 * The keypad is configured in a matrix, meaning reach row and each column is a
 * separate pin (3x4: 7 pins total). Each key has a location represented by
 * (row,col) (top->right). When the key at (R,C) is pressed, it connects the
 * row R pin to the col C pin.
 * 
 * Since any given pin on the AVR can only be an input or an output at a point
 * in time, the row and the column must be read separately. This requires
 * switching the outputs to inputs and the inputs to outputs. Initially the
 * rows are outputs and the columns inputs. When the column is read, the
 * columns are switched to outputs and the row is read. The distinct
 * combination of (R,C) determines a unique key.
 */

static void keypad_init_rowscan(void);
static void keypad_init_colscan(void);

volatile uint8_t keymask = 0;

void keypad_int_enable(void) {
	KEYPAD_PORT.INTCTRL |= PORT_INT0LVL_MED_gc;
}

void keypad_int_disable(void) {
	KEYPAD_PORT.INTCTRL &= ~PORT_INT0LVL_MED_gc;
}

/**
 * Set cols as inputs to scan cols
 */
static void keypad_init_colscan(void) {
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
static void keypad_init_rowscan(void) {
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

void keypad_init(void) {

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

char keypad_getc(void) {
	uint8_t tmp;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		tmp = keymask; 
		keymask = 0;
	}

	switch (tmp) {
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
			return '\0';
	}
}
