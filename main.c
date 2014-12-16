#include <avr/io.h>
#include "keypad.h"
#include "display.h"
#include "temp.h"
#include "servo.h"
#include "ssr.h"
#include "timer.h"


static uint8_t tea_set_point = 0;

static enum {
	TEA_STATUS_UNKNOWN,
	TEA_STATUS_ON,
	TEA_STATUS_OFF
} tea_state;

typedef struct {
	enum {
		//not entering a temperature
		TEMP_STATE_NONE,
		//entering a temperature
		TEMP_STATE_ENTER,
	} state;

	uint8_t digits[3];

	//digit being entered: 0,1,2
	uint8_t digit;

} temp_entry_t;

static temp_entry_t temp_entry;


static void handle_key(const char key);
static void handle_key_idle(const char key);
static void handle_temperature_digit(const uint8_t digit);
static void tea_set_temperature(uint16_t temp);

static void clear_temp_state(void) {
	temp_entry.state = TEMP_STATE_NONE;
	display_puts("   ");
}

static void tea_display_temp(void) {
	display_puti(thermistor_read_temp());
}

static inline void tea_off(void) {
	//do fun stuff while no tea is brewing
	if (tea_state == TEA_STATUS_OFF)
		return;

	del_timer(tea_display_temp);
	//add_timer(display_test, TIMER_HZ/4, TIMER_RUN_UNLIMITED);
	tea_state = TEA_STATUS_OFF;
	ssr_off();
	servo_set_angle(0);
	if (tea_set_point != 0)
		display_puti(tea_set_point);
	else
		display_puts("---");

}

static void tea_set_temperature(uint16_t temp) {
	if (temp < TEA_TEMP_MIN || temp > TEA_TEMP_MAX)
		display_puts("Err");
	else
		display_puti(temp);
}

static inline void tea_on(void) {
	if (tea_state == TEA_STATUS_ON)
		return;

	//update the temperature display once per second
	add_timer(tea_display_temp, TIMER_HZ/4, TIMER_RUN_UNLIMITED);
	//del_timer(display_test);

	tea_state = TEA_STATUS_ON;
	ssr_on();
	servo_set_angle(180);
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

	while (1) {
		char key;
		if ((key = keypad_getc()))
			handle_key(key);

		uint8_t temp = thermistor_read_temp();
		if (temp >= 190) {
			tea_off();
		}
	}
}

static void handle_key(const char key) {
	keypad_int_disable();
	if (tea_state == TEA_STATUS_ON) {
		//tea should handle all these keys
	} else if (tea_state == TEA_STATUS_OFF) {
		handle_key_idle(key);
	} else {
		//What? there is no else!
	}
	keypad_int_enable();
}

static void handle_key_idle(const char key) {
	//digit
	if (key >= 0x30 && key <= 0x39) {
		handle_temperature_digit(key - 0x30);
	}
}

static void handle_temperature_digit(const uint8_t digit) {
	char buf[4] = "---";

	//this is the first digit
	if (temp_entry.state == TEMP_STATE_NONE) {
		temp_entry.state = TEMP_STATE_ENTER;
		temp_entry.digit = 0;
	}

	temp_entry.digits[temp_entry.digit] = digit;
	del_timer(clear_temp_state);

	for (uint8_t i = 0; i <= temp_entry.digit; ++i)
		buf[i] = temp_entry.digits[i]+0x30;

	display_puts(buf);

	if (temp_entry.digit == 2) {
		temp_entry.state = TEMP_STATE_NONE;
		uint16_t temp = temp_entry.digits[0]*100 + temp_entry.digits[1]*10 + temp_entry.digits[2];
		tea_set_temperature(temp);
	} else {
		temp_entry.digit++;
		add_timer(clear_temp_state, 5*TIMER_HZ, 1);
	}
}
