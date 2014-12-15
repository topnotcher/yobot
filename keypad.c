#include <util/atomic.h>
#include <stdint.h>
#include "keypad.h"
#include "timer.h"

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
static void keypad_scan(void);
static void keypad_scan_end(void);

volatile uint8_t keymask = 0;

typedef struct {
	uint8_t colmask;
	uint8_t rowmask;
	enum {
		KEYPAD_STATE_IDLE,
		KEYPAD_STATE_ROWSCAN,
		KEYPAD_STATE_COLSCAN,
	} state;
	uint8_t samples;
} keypad_scan_t;

static keypad_scan_t keypad_scanner;

static void keypad_int_enable(void) {
	KEYPAD_PORT.INTCTRL |= PORT_INT0LVL_MED_gc;
}

static void keypad_int_disable(void) {
	KEYPAD_PORT.INTCTRL &= ~PORT_INT0LVL_MED_gc;
}

/**
 * Set cols as inputs to scan cols
 */
// col scan is initial. We are outputting 0 on the rows, and have the cols set
// to inputs with pullup. When button is pressed, the corresponding col pin is
// pulled low, following which the input/outputs are reversed and the rows are
// scanned.
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
	keypad_scanner.state = KEYPAD_STATE_IDLE;
	keypad_init_colscan();
	keypad_int_enable();
}

static void keypad_scan_end(void) {
	del_timer(keypad_scan);
	keypad_init_colscan();
	keypad_int_enable();
}


/**
 * This handles the actual reading of the keypresses.
 *
 * When a pin change is detected by hardware, the KEYPAD_ISR interrupt
 * triggers. The interrupt initializes the keypad_scanner state machine and
 * schedules keypad_scan to run 2*KEYPAD_SCAN_SAMPLES times at
 * KEYPAD_SCAN_DELAY intervals.
 *
 * Half of the KEYPAD_SCAN_SAMPLES try to read the column while the other half
 * try to read the row. The columns are read first. When a column is
 * successfully read, the rows are configured for reading. If, after
 * KEYPAD_SCAN_SAMPLES attempts, no column is read, key scanning is aborted.
 * When a row is successfully read, the keymask of the pressed key is stored in
 * the keymask variable and the KEYPAD_REPEAT_RATE timer is activated. If no
 * row is read, scanning is aborted.
 * 
 * TL;DR: read the col, read the row, set the result, turn it all off for a bit.
 */
static void keypad_scan(void) {
	
	if (keypad_scanner.state == KEYPAD_STATE_COLSCAN) {
		//cols are already setup to scan.
		keypad_scanner.colmask = ~KEYPAD_PORT.IN & KEYPAD_COLMASK;

		//col has been read or KEYPAD_SCAN_SAMPLES tries have been made
		if (keypad_scanner.colmask != 0 || keypad_scanner.samples == KEYPAD_SCAN_SAMPLES-1) {
			//col was not read - abort
			if (keypad_scanner.colmask == 0) {
				keypad_scan_end();
			//col was read - try rows
			} else {
				keypad_scanner.samples = 0;
				keypad_scanner.state = KEYPAD_STATE_ROWSCAN;
				keypad_init_rowscan();
			}
		} 

	} else {
		keypad_scanner.rowmask = ~KEYPAD_PORT.IN & KEYPAD_ROWMASK;

		if (keypad_scanner.rowmask != 0 || keypad_scanner.samples == KEYPAD_SCAN_SAMPLES-1) {
			if (keypad_scanner.rowmask == 0) {
				keypad_scan_end();
			} else {
				keymask = keypad_scanner.colmask | keypad_scanner.rowmask;

				//keypad_int_enable();
				//implement key repeat rate.
				add_timer(keypad_scan_end, KEYPAD_REPEAT_RATE, 1);
			}
		} 
	}

	keypad_scanner.samples++;
}

/**
 * Interrupt Service Routine that triggers when a PIN value changes.
 */
KEYPAD_ISR {
	keypad_scanner.colmask = 0;
	keypad_scanner.rowmask = 0;
	keypad_scanner.state = KEYPAD_STATE_COLSCAN;
	keypad_scanner.samples = 0;
	keypad_int_disable();
	add_timer(keypad_scan, KEYPAD_SCAN_DELAY, KEYPAD_SCAN_SAMPLES*2);
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
