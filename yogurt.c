#include <util/delay.h>
#include "temp.h"
#include "ssr.h"
#include "tasks.h"
#include "timer.h"
#include "yogurt.h"
#include "display.h"
//#include "debug.h"
#include <stdio.h>

/*
void display_puts(char *s) {
	debug_write(s);
}
*/

//stage 1 heats to 185F for 10 minutes to sterilize
//stage 2 cools the milk to 110F and holds for 10 minutes
//stage 3 incubates at 100F for 8 hours
typedef struct {
	//target temperature in C
	int16_t temperature;
	uint16_t minutes;
} yogurt_cycle_t;

static yogurt_cycle_t yogurt_cycles[] = {
	{ .temperature = 85, .minutes = 10 },
	{ .temperature = 110, .minutes = 10 },
	{ .temperature = 85, .minutes = 480 },

	//0,0 acts as a dummy value to end the list
	{ .temperature = 0, .minutes = 0 },
};

typedef struct {
	yogurt_cycle_t *cycle;

	//each cycle starts by attaining the target temperature.
	//when the target is achieved, it is maintained.
	enum {
		YOGURT_STEP_IDLE,
		YOGURT_STEP_ATTAIN,
		YOGURT_STEP_MAINTAIN
	} step;

	int16_t last_temp;

	//counters in current state
	//these only start when
	uint16_t minutes;
	uint8_t seconds;
} yogurt_state_t;
static yogurt_state_t state;

static inline uint8_t temp_in_interval(int16_t temp, int16_t a, int16_t b);
static int8_t yogurt_maintain_temperature(int16_t maintain_temp, int16_t *cur_temp);
static void yogurt_start(void);
static void yogurt_run_upper(void);
static void yogurt_run_lower(void);

void yogurt_init() {
	display_init();
	temp_init();
	ssr_init();
	//debug_init();
	task_schedule(yogurt_start);
}

static void yogurt_start() {
	int8_t err = get_temp(&state.last_temp);
	// keep retrying until a valid temperature is read
	if (err) {
		task_schedule(yogurt_start);
	} else {
		state.cycle = &yogurt_cycles[0];
		state.step = YOGURT_STEP_ATTAIN;
		add_timer(yogurt_run_upper, 1*TIMER_HZ, TIMER_RUN_UNLIMITED);
	}
}

/**
 * This runs via timer
 */
static void yogurt_run_upper() {
	if (state.step == YOGURT_STEP_MAINTAIN) {
		if (++state.seconds == 60) {
			state.minutes++;
			state.seconds = 0;
		}
	}
	task_schedule(yogurt_run_lower);
}

/**
 * This runs after yogurt_run_upper() -- outside of timer context
 */
static void yogurt_run_lower() {
	if (state.step == YOGURT_STEP_IDLE) {
		del_timer(yogurt_run_upper);
		ssr_off();
		display_puts("---");
		return;
	}

	int16_t temp;
	int8_t error = yogurt_maintain_temperature(state.cycle->temperature, &temp);

	//@TODO
	if (error) {
		display_puts("Err");
		return;
	}


	char buf[4] = {0};
	sprintf(buf,"%0 d",(int)(temp*1.8+32));
	display_puts(buf);
	
	if (state.step == YOGURT_STEP_MAINTAIN) {
		//increment happens in upper
		if (state.minutes >= state.cycle->minutes) {
			state.cycle++;

			if (state.cycle->temperature == 0 && state.cycle->minutes == 0) {
				ssr_off();
				state.step = YOGURT_STEP_IDLE;
			} else {
				state.step = YOGURT_STEP_ATTAIN;
			}
		}

	} else if (state.step == YOGURT_STEP_ATTAIN) {
		//temperature could be increasing or decreasing, so we check only to
		//see if the temperature *crosses* the threshold:
		//target is in the interval: [last_temp,temp] OR [temp,last_temp]
		if (temp_in_interval(state.cycle->temperature,state.last_temp,temp)) {
			state.step = YOGURT_STEP_MAINTAIN;
			state.minutes = 0;
			state.seconds = 0;
		}
	}

	state.last_temp = temp;
}

static inline uint8_t temp_in_interval(int16_t temp, int16_t a, int16_t b) {
	return (temp >= a && temp <= b) || (temp >= b && temp <= a);
}

static int8_t yogurt_maintain_temperature(int16_t maintain_temp, int16_t *cur_temp) {
	int8_t err;
	err = get_temp(cur_temp);

	//always shut the relay off in the event of an error
	if (err) {
		ssr_off();
		return err;
	}

	if (*cur_temp < maintain_temp)
		ssr_on();
	else if (*cur_temp > maintain_temp)
		ssr_off();

	return err;
}
