#include <avr/io.h>
#include "keypad.h"
#include "display.h"
#include "temp.h"
#include "servo.h"
#include "ssr.h"
#include "timer.h"

static inline void tea_off(void) {
	ssr_off();
	servo_set_angle(0);
}

static inline void tea_on(void) {
	ssr_on();
	servo_set_angle(180);
}
static void tea_display_temp(void) {
	display_puti(thermistor_read_temp());
}

int main(void) {
	servo_init();
	keypad_init();
	display_init();
	thermistor_init();
	ssr_init();
	init_timers();
	

	tea_off();

	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	//update the temperature display once per second
	add_timer(tea_display_temp, TIMER_HZ, TIMER_RUN_UNLIMITED);

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
	}
}
