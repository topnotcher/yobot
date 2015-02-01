#include <stdio.h>
#include <util/delay.h>
#include "temp.h"
#include "ssr.h"
#include "tasks.h"
#include "timer.h"
#include "yogurt.h"
#include "display.h"
#include "keypad.h"
#include "alarm.h"
#include "debug.h"

#define MIN(a,b) (((a) > (b))? (b) : (a))
#define MAX(a,b) (((a) > (b))? (a) : (b))

typedef struct {
	//target temperature in 1/16th C
	int16_t temperature;
	uint16_t minutes;
} yogurt_cycle_t;

typedef struct {
	yogurt_cycle_t cycle;
	int16_t next_target;

	//each cycle starts by attaining the target temperature.
	//when the target is attained, it is maintained.
	enum {
		YOGURT_STATE_IDLE,
		YOGURT_STATE_ATTAIN,
		YOGURT_STATE_MAINTAIN
	} state;

	int16_t last_temp;
	int16_t integral;

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

static void yogurt_print_status(int16_t temp, int16_t minutes, uint8_t seconds);
static inline void yogurt_print_status_down(int16_t temp, int16_t cycle_minutes, int16_t cur_minutes, uint8_t cur_seconds);
static void yogurt_keyhandler_idle(char);

void yogurt_init() {
	display_init();
	keypad_init();
	temp_init();
	ssr_init();
	debug_init();
	alarm_init();
	register_keyhandler(yogurt_keyhandler);
	control.state = YOGURT_STATE_IDLE;
}

static void yogurt_start() {
	int8_t err = yogurt_get_temp(&control.last_temp);
	// keep retrying until a valid temperature is read
	if (err) {
		task_schedule(yogurt_start);
	} else {
		control.integral = 0;
		if (control.last_temp < control.cycle.temperature) {
			if ((control.cycle.temperature - control.last_temp) > 134)
				control.next_target = control.cycle.temperature-134;
			else
				control.next_target = control.last_temp + 12;
		} else {
			control.next_target = 0;
		}

		control.state = YOGURT_STATE_ATTAIN;
		control.minutes = 0;
		control.seconds = 0;
		add_timer(yogurt_run_upper, 1*TIMER_HZ, TIMER_RUN_UNLIMITED);
	}
}

/**
 * This runs via timer
 */
static void yogurt_run_upper() {
	if (++control.seconds == 60) {
		control.minutes++;
		control.seconds = 0;
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

	if (error) {
		printf("Err");
		return;
	}

	if (control.state == YOGURT_STATE_MAINTAIN) {
		yogurt_print_status_down(temp,control.cycle.minutes,control.minutes,control.seconds);

		if (control.minutes >= control.cycle.minutes) {
				ssr_off();
				alarm_on();
				control.state = YOGURT_STATE_IDLE;
		}

	} else if (control.state == YOGURT_STATE_ATTAIN) {
		yogurt_print_status(temp,control.minutes,control.seconds);

		if (temp_in_interval(control.cycle.temperature,control.last_temp,temp)) {
			yogurt_print_status(temp,control.cycle.minutes,control.seconds);
			alarm_on();
			control.minutes = 0;
			control.seconds = 0;
			control.integral = 0;
			control.state = YOGURT_STATE_MAINTAIN;
		}
	}

	control.last_temp = temp;
	debug_write(&control, sizeof(control));
}

static inline uint8_t temp_in_interval(int16_t temp, int16_t a, int16_t b) {
	//since temperatures are in 1/16ths, +-8 is +-1/2
	return (temp >= a-8 && temp <= b+8) || (temp >= b-8 && temp <= a+8);
}

static int8_t yogurt_get_temp(int16_t *temp) {
	//Temp is in degrees C times 16 (1/16th degree)
	return get_temp(temp);
}

static int8_t yogurt_maintain_temperature(int16_t set_point, int16_t *cur_temp) {
	int8_t err;
	err = yogurt_get_temp(cur_temp);

	//cur_temp + diff = set_point
	int16_t diff = set_point - *cur_temp;

	//always shut the relay off in the event of an error
	if (err) {
		ssr_off();
		return err;
	}

	//fix this
	static int8_t diff_neg = 0;
	static int8_t diff_pos = 60;
	if (diff <= 0) {
		diff_neg = MIN(diff_neg+1, 60);
		diff_pos = MAX(diff_pos-1,0);
	} else { //diff > 0
		diff_pos = MIN(diff_pos+1, 60);
		diff_neg = MAX(diff_neg-1,0);
	}

	//initialize to 1/60th second -- theoretically the
	//minimum pulse the 0x SSR can output
	int32_t level = SSR_MAX_LEVEL/60+1;

	if (control.state == YOGURT_STATE_ATTAIN) {
		// 134 is about 15 degrees F - So proportional
		// control activates within 15 degrees of target
		level += (int32_t)diff*(SSR_MAX_LEVEL/134);

		// attempts to avoid integral windup.
		if (control.minutes >= 5 && diff < 134 && diff_pos == 60) {

			//every two minutes
			if (control.integral < SSR_MAX_LEVEL && (control.minutes&0x1) && control.seconds == 0) {

				// next_target reduces integral windup by raising the target
				// temperature by 12 at a time, so the diff used for the integral
				// can never exceed 12.
				if (control.next_target != 0) {
					control.integral += (control.next_target - *cur_temp)*160;
					control.next_target = *cur_temp + 12;
					if (control.next_target > set_point)
						control.next_target = set_point;
				}
			}
		} else if (diff_neg == 60) {
			control.integral = 0;
		}

		level += control.integral;

	//maintain mode - entered within 1 degree F of set point.
	} else {
		//proportional coefficient is much higher - hopefully will reduce
		//transients without significant increase in overshoot... hopefully.
		level += (int32_t)diff*(SSR_MAX_LEVEL/10);

		//when diff drops below 1 for 60 consecutive iterations, reset the
		//integral. This reduces oscillation.
		if (diff_neg == 60) {
			control.integral = 0;
		} else if (diff > 0) {
			int16_t add = 2*diff;
			if (control.integral <= SSR_MAX_LEVEL-add)
				control.integral += add;
		}

		//this will continue to add the integral term in until diff_neg == 30,
		level += control.integral;
	}

	if (level >= (int32_t)SSR_MAX_LEVEL)
		level = SSR_MAX_LEVEL;
	else if (level < 0)
		level = 0;

	ssr_level(level);

	return err;
}

static void yogurt_keyhandler(void) {
	char key = keypad_getc();
	if (!key)
		return;

	if (key == 'd') {
		alarm_off();
	} else if (control.state == YOGURT_STATE_IDLE) {
		yogurt_keyhandler_idle(key);
	} else if (key == '#') {
		control.state = YOGURT_STATE_IDLE;
		clear();
		alarm_off();
	}
}

static void yogurt_print_status(int16_t temp, int16_t minutes, uint8_t seconds) {
	int16_t n1,n2;
	if (minutes >= 60) {
		n1 = minutes/60;
		n2 = minutes-60*n1;
	} else {
		n1 = minutes;
		n2 = seconds;
	}

	printf("%4d%2d:%02d",(int)(temp*9.0/80.0+32.5),n1,n2);
}

static inline void yogurt_print_status_down(int16_t temp, int16_t cycle_minutes, int16_t cur_minutes, uint8_t cur_seconds) {
	int16_t minutes;
	uint8_t seconds;

	minutes = cycle_minutes - cur_minutes - 1;
	seconds = 60-cur_seconds;

	if (seconds == 60) {
		minutes++ ;
		seconds = 0;
	}

	yogurt_print_status(temp,minutes,seconds);
}

static void yogurt_keyhandler_idle(char key) {
	static uint8_t step = 0;
	static uint8_t digits[4] = {0};
	static int num = 0;
	static uint8_t max_digits = 3;

	if (key == '*') {
		if (step == 0) {
			step = 1;
			max_digits = 3;
			for (uint8_t i = 0; i < max_digits; ++i)
				digits[i] = 0;
			printf(" ---    ");
		} else if (step == 1) {
			step = 2;
			max_digits = 4;
			//convert to 16th degrees C
			control.cycle.temperature = (num-32)*80.0/9;
			for (uint8_t i = 0; i < max_digits; ++i)
				digits[i] = 0;
			printf("    --:--");
		} else if (step == 2) {
			step = 0;
			control.cycle.minutes = num;
			yogurt_start();
		}
	} else if (key == '#') {
		step = 0;
		clear();
		alarm_off();
	} else if (key >= '0' && key <= '9' && step > 0) {
		for (uint8_t i = max_digits-1; i > 0; --i)
			digits[i] = digits[i-1];

		digits[0] = key - '0';

		if (step == 1) {
			num = 0;
			int x = 1;
			for (uint8_t i = 0; i < max_digits; ++i) {
				num += digits[i]*x;
				x *= 10;
			}
			printf("%4d    ", num);
		} else if (step == 2) {
			uint8_t hours = digits[3]*10 + digits[2];
			uint8_t minutes = digits[1]*10 + digits[0];
			num = hours*60 + minutes;
			printf("    %02d:%02d",hours,minutes);
		}

	}
}
