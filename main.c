#include <avr/io.h>
#include "keypad.h"
#include "display.h"
#include "temp.h"
#include "servo.h"
#include "ssr.h"
#include "timer.h"

static enum {
	TEA_STATUS_UNKNOWN,
	TEA_STATUS_ON,
	TEA_STATUS_OFF
} tea_status;

static void tea_display_temp(void) {
	display_puti(thermistor_read_temp());
}

static inline void tea_off(void) {
	//do fun stuff while no tea is brewing
	if (tea_status == TEA_STATUS_OFF)
		return;

	del_timer(tea_display_temp);
	//add_timer(display_test, TIMER_HZ/4, TIMER_RUN_UNLIMITED);
	tea_status = TEA_STATUS_OFF;
	ssr_off();
	servo_set_angle(0);
}

static inline void tea_on(void) {
	if (tea_status == TEA_STATUS_ON)
		return;

	//update the temperature display once per second
	add_timer(tea_display_temp, TIMER_HZ/4, TIMER_RUN_UNLIMITED);
	//del_timer(display_test);

	tea_status = TEA_STATUS_ON;
	ssr_on();
	servo_set_angle(180);
}

#include <util/delay.h>
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

	while (1) {
		char key;
		if ((key = keypad_getc())) {
			// GREG: Why are interrupts being disabled here?
			// test without disabling (should work) !!!!!
			keypad_int_disable();
			display_putchar(key);

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
