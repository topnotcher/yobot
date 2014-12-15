#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "config.h"

#ifndef KEYPAD_H
#define KEYPAD_H

/**
 * No configuration required beyond this point.
 */

#define KEYPAD_PORT_CONCAT(a,b) a ## b
#define KEYPAD_PORT_CONCAT3(a,b,c) a##b##c
#define KEYPAD_PIN(id) KEYPAD_PORT_CONCAT3(PIN,id,_bm)
#define KEYPAD_PORT_PINCTRL(id) KEYPAD_PORT_CONCAT3(PIN,id,CTRL)

#define KEYPAD_ROWMASK ( KEYPAD_PIN(KEYPAD_PIN_R1) | KEYPAD_PIN(KEYPAD_PIN_R2) | KEYPAD_PIN(KEYPAD_PIN_R3) | KEYPAD_PIN(KEYPAD_PIN_R4) )
#define KEYPAD_COLMASK ( KEYPAD_PIN(KEYPAD_PIN_C1) | KEYPAD_PIN(KEYPAD_PIN_C2) | KEYPAD_PIN(KEYPAD_PIN_C3) )

#define KEYPAD_PINCTRL(key) KEYPAD_PORT.KEYPAD_PORT_PINCTRL(KEYPAD_PORT_CONCAT(KEYPAD_PIN_,key))

#define KEY_PINS(r,c) (KEYPAD_PIN(KEYPAD_PIN_R##r) | KEYPAD_PIN(KEYPAD_PIN_C##c))

/**
 * Map the matrix to the actual keys #define KEY_1 KEY_PINS(1,1) means the 1
 * key is at position (1,1) (row,col) on the keypad.
 */

#define KEY_1 		 KEY_PINS(1,1)
#define KEY_2		 KEY_PINS(1,2)
#define KEY_3		 KEY_PINS(1,3)

#define KEY_4 		 KEY_PINS(2,1)
#define KEY_5		 KEY_PINS(2,2)
#define KEY_6		 KEY_PINS(2,3)

#define KEY_7 		 KEY_PINS(3,1)
#define KEY_8		 KEY_PINS(3,2)
#define KEY_9		 KEY_PINS(3,3)

#define KEY_ASTERISK KEY_PINS(4,1)
#define KEY_0 		 KEY_PINS(4,2)
#define KEY_POUND 	 KEY_PINS(4,3)


void keypad_init(void);
char keypad_getc(void);

#endif
