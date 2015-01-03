#include <util/delay.h>
#include "temp.h"
#include "ssr.h"
#include "tasks.h"
#include "timer.h"
#include "yogurt.h"
#include "display.h"
#include "keypad.h"
#include <stdio.h>

//stage 1 heats to 185F for 10 minutes to sterilize
//stage 2 cools the milk to 110F and holds for 10 minutes
//stage 3 incubates at 100F for 8 hours
typedef struct {
	//target temperature in C
	int16_t temperature;
	uint16_t minutes;
} yogurt_cycle_t;

typedef struct {
	yogurt_cycle_t cycle;

	//each cycle starts by attaining the target temperature.
	//when the target is achieved, it is maintained.
	enum {
		YOGURT_STATE_IDLE,
		YOGURT_STATE_ATTAIN,
		YOGURT_STATE_MAINTAIN
	} state;

	int16_t last_temp;

	//counters in current state
	//these only start when
	uint16_t minutes;
	uint8_t seconds;
} yogurt_state_t;
static yogurt_state_t control;

static inline uint8_t temp_in_interval(int16_t temp, int16_t a, int16_t b);
static int8_t yogurt_maintain_temperature(int16_t maintain_temp, int16_t *cur_temp);
static void yogurt_start(void);
static void yogurt_run_upper(void);
static void yogurt_run_lower(void);
static void yogurt_keyhandler(void);
static int8_t yogurt_get_temp(int16_t *temp);

static void yogurt_keyhandler_idle(char);

void yogurt_init() {
	display_init();
	keypad_init();
	temp_init();
	ssr_init();
	register_keyhandler(yogurt_keyhandler);
	control.state = YOGURT_STATE_IDLE;
}

static void yogurt_start() {
	int8_t err = yogurt_get_temp(&control.last_temp);
	// keep retrying until a valid temperature is read
	if (err) {
		task_schedule(yogurt_start);
	} else {
		control.state = YOGURT_STATE_ATTAIN;
		add_timer(yogurt_run_upper, 1*TIMER_HZ, TIMER_RUN_UNLIMITED);
	}
}

/**
 * This runs via timer
 */
static void yogurt_run_upper() {
	if (control.state == YOGURT_STATE_MAINTAIN) {
		if (++control.seconds == 60) {
			control.minutes++;
			control.seconds = 0;
		}
	}
	task_schedule(yogurt_run_lower);
}

/**
 * This runs after yogurt_run_upper() -- outside of timer context
 */
static void yogurt_run_lower() {
	if (control.state == YOGURT_STATE_IDLE) {
		del_timer(yogurt_run_upper);
		ssr_off();
		clear();
		return;
	}

	int16_t temp;
	int8_t error = yogurt_maintain_temperature(control.cycle.temperature, &temp);

	//@TODO
	if (error) {
		printf("Err");
		return;
	}


	printf("%3d",(int)(temp*9.0/80.0+32.5));
	
	if (control.state == YOGURT_STATE_MAINTAIN) {
		//increment happens in upper
		if (control.minutes >= control.cycle.minutes) {
				ssr_off();
				control.state = YOGURT_STATE_IDLE;
		}

	} else if (control.state == YOGURT_STATE_ATTAIN) {
		//temperature could be increasing or decreasing, so we check only to
		//see if the temperature *crosses* the threshold:
		//target is in the interval: [last_temp,temp] OR [temp,last_temp]
		if (temp_in_interval(control.cycle.temperature,control.last_temp,temp)) {
			control.state = YOGURT_STATE_MAINTAIN;
			control.minutes = 0;
			control.seconds = 0;
		}
	}

	control.last_temp = temp;
}

static inline uint8_t temp_in_interval(int16_t temp, int16_t a, int16_t b) {
	//since temperatures are in 1/16ths, +-8 is +-1/2 
	return (temp >= a-8 && temp <= b+8) || (temp >= b-8 && temp <= a+8);
}

static int8_t yogurt_get_temp(int16_t *temp) {
	//Temp is in degrees C times 16 (1/16th degree)
	return get_temp(temp);
}

static int8_t yogurt_maintain_temperature(int16_t maintain_temp, int16_t *cur_temp) {
	int8_t err;
	err = yogurt_get_temp(cur_temp);

	//always shut the relay off in the event of an error
	if (err) {
		ssr_off();
		return err;
	}

	if (*cur_temp < maintain_temp)
		ssr_on();
	else if (*cur_temp >= maintain_temp)
		ssr_off();

	return err;
}

static void yogurt_keyhandler(void) {
	char key = keypad_getc();
	if (!key)
		return;

	if (control.state == YOGURT_STATE_IDLE)
		yogurt_keyhandler_idle(key);
	else if (key == '#') {
		control.state = YOGURT_STATE_IDLE;
		clear();
	}
}

static void yogurt_keyhandler_idle(char key) {
	/**
	 * ... * to enter temperature
	 * ... * again to enter time
	 * ... * again to begin.
	 *
	 * (0) doing nothing...
	 * 
	 * (1) Enter temperature
	 * 		- d1
	 * 		- d2
	 * 		- d3 1-3? digits.
	 * 		- * to move to entering time.
	 * (2) Enter time:
	 * 		- HH:MM
	 * 		- * to start the cycle.
	 */

	static uint8_t step = 0;
	static uint8_t digits[4] = {0};
	static int num = 0;
	static const uint8_t max_digits = 3;

	if (key == '*') {
		if (step == 0) {
			step = 1;
			for (uint8_t i = 0; i < max_digits; ++i)
				digits[i] = 0;
			printf("---");
		} else if (step == 1) {
			step = 2;
			//convert to 16th degrees C
			control.cycle.temperature = (num-32)*80.0/9;
			for (uint8_t i = 0; i < max_digits; ++i)
				digits[i] = 0;
			printf("---");
		} else if (step == 2) {
			step = 0;
			control.cycle.minutes = num;
			yogurt_start();
		}
	} else if (key == '#') {
		step = 0;
		clear();
	} else if (key >= '0' && key <= '9' && step > 0) {
		for (uint8_t i = max_digits-1; i > 0; --i)
			digits[i] = digits[i-1];

		digits[0] = key - '0';

		
		num = 0;
		int x = 1;
		for (uint8_t i = 0; i < max_digits; ++i) {
			num += digits[i]*x;
			x *= 10;
		}

		printf("%3d", num);
	}
}
