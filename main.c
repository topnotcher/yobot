#include <avr/io.h>
#include <util/delay.h>
#include "keypad.h"
#include "display.h"
#include "temp.h"
#include "servo.h"
#include "ssr.h"
#include "timer.h"
#include "mp3.h"


static uint8_t tea_set_point = 0;

/**
 * Whether or not the device is brewing tea
 */
static enum {
	DEV_STATE_UNKNOWN,
	DEV_STATE_BREW,
	DEV_STATE_IDLE
} device_state;


/**
 * Progress in the tea brewing cycle
 */
static enum {
	TEA_STATE_HEAT,
	TEA_STATE_STEEP
} tea_state;
static uint16_t tea_ticks;

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
static void tea_tick(void);
static void tea_off(void);
static void tea_on(void);

static void clear_temp_state(void) {
	temp_entry.state = TEMP_STATE_NONE;
	display_puts("---");
}

static void tea_tick(void) {
	if (tea_state == TEA_STATE_HEAT) {
		uint8_t temp = thermistor_read_temp();
		display_puti(temp);

		if (temp >= tea_set_point) {
			ssr_off();
			servo_set_angle(180);
			tea_state = TEA_STATE_STEEP;
			tea_ticks = TEA_STEEP_TIME;
		}

	} else if (tea_state == TEA_STATE_STEEP) {
		//tea done
		display_puti(tea_ticks);
		if (tea_ticks == 0) {
			tea_off();
			mp3_play(6);
		} else {
			--tea_ticks;
		}
	}
}

static void tea_off(void) {

	del_timer(tea_tick);
	//add_timer(display_test, TIMER_HZ/4, TIMER_RUN_UNLIMITED);
	device_state = DEV_STATE_IDLE;
	ssr_off();
	servo_set_angle(0);
	display_puts("---");
}

static void tea_set_temperature(uint16_t temp) {
	//001_180-190.mp3  002_190-200.mp3  003_200-212.mp3  004_212+.mp3  005_buttons.mp3  006_done.mp3  007_stop.mp3  008_success.mp3  009_under180.mp3      010_welcome.mp3
	//
	if (temp < TEA_TEMP_MIN || temp > TEA_TEMP_MAX) {
		display_puts("Err");
	
		if (temp < 180) {
			mp3_play(9);
		} else if (temp < 190) {
			mp3_play(1);
		} else if (temp < 200) {
			mp3_play(2);
		} else if (temp < 212) {
			mp3_play(3);
		} else if (temp >= 212) {
			mp3_play(4);
		}
		
	} else {
		tea_set_point = temp;
		display_puti(temp);
		mp3_play(8);
	}
}

static void tea_on(void) {
	if (device_state == DEV_STATE_BREW)
		return;

	//update the temperature display once per second
	add_timer(tea_tick, TIMER_HZ, TIMER_RUN_UNLIMITED);
	//del_timer(display_test);

	device_state = DEV_STATE_BREW;
	tea_state = TEA_STATE_HEAT;
	ssr_on();
}

int main(void) {
	//allow everything to settle/boot/etc
	//(mainly the mp3 chip takes a while to boot up)
	_delay_ms(2000);

	servo_init();
	keypad_init();
	display_init();
	thermistor_init();
	ssr_init();
	init_timers();
	mp3_init();

	tea_off();

	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	//another 100 for things to settle
	_delay_ms(100);

	//welcome
	mp3_play(10);

	while (1) {
		char key;
		if ((key = keypad_getc()))
			handle_key(key);
	}
}

/**
 * Handle all key presses
 */
static void handle_key(const char key) {
	keypad_int_disable();
	if (device_state == DEV_STATE_BREW) {
		if (key == '*')
			tea_off();
		mp3_play(5);
	} else if (device_state == DEV_STATE_IDLE) {
		handle_key_idle(key);
	} else {
		//What? there is no else!
	}
	keypad_int_enable();
}

/**
 * Handle keys when tea is not being brewed
 */
static void handle_key_idle(const char key) {
	//digit
	if (key >= 0x30 && key <= 0x39) {
		handle_temperature_digit(key - 0x30);
	} else if (key == '#') {
		if (tea_set_point == 0)
			display_puts("Err");
		else
			tea_on();
	} else if (key == '*') {
		display_puts("---");
		del_timer(clear_temp_state);
		clear_temp_state();
	}
}

/**
 * Handle keys to set temperature digits
 */
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
