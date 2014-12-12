#include <avr/io.h>
#include "keypad.h"
#include "display.h"
#include "temp.h"
#include "servo.h"

static inline void tea_off(void) {
	PORTB.OUTCLR = PIN2_bm;
	servo_set_angle(0);
}

static inline void tea_on(void) {
	PORTB.OUTSET = PIN2_bm;
	servo_set_angle(180);
}

int main(void) {
	servo_init();
	keypad_init();
	display_init();
	thermistor_init();
	
	/** TEA INIT
	 */

	PORTB.DIRSET = PIN2_bm;
	tea_off();

	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	uint8_t k = 0;
	while (1) {
		char key;
		if ((key = keypad_getc())) {
			keypad_int_disable();
			//display_putchar(key);

			if (key == '*') {
				tea_off();
			} else if (key == '0') {
				servo_set_angle(90);
			} else if (key == '#') {
				tea_on();
			}
			keypad_int_enable();
		}

		uint8_t temp = thermistor_read_temp();
		if (temp >= 190) {
			tea_off();
		}

		if (++k == 255) {
			char foo[] = {0,0,0};
			foo[2] = temp % 10 + 0x30;
			temp /= 10;
			foo[1] = temp % 10 + 0x30;
			temp /= 10;
			foo[0] = temp % 10 + 0x30;
			display_puts(foo); 
		}
	}
}
