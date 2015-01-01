#include <stdlib.h>
#include <stdio.h>

#include <util/atomic.h>

#include "temp.h"
#include "timer.h"
#include "tasks.h"
#include "threads.h"
#include "ds2483.h"
#include "ds18b20.h"
#include "error.h"
#include "config.h"

#define _CONCAT3(a,b,c) a##b##c
#define _PIN(id) _CONCAT3(PIN,id,_bm)

static ds2483_dev_t *onewiredev;

static int8_t temp_error;
static double temp;

static void onewire_schedule(void);
static void onewire_resume(void) __attribute__((naked));
static inline void onewire_sleep(uint16_t);
static void onewire_init(void);

void temp_init(void) {
	temp_error = -EINVAL;
	onewire_init();
	task_schedule(onewire_schedule);
}

/**
 * Temperature monitoring thread.
 */
void temp_run(void) {
	ds2483_rst(onewiredev);

	while(1) {
		int8_t error;
		error = ds18b20_start_conversion(onewiredev);

		if (error) {
			temp_error = error;
			continue;
		}

	
		//@TODO: failsafe if ds18b20_* returns error

		//note: manual indicates max 750ms per conversion 
		onewire_sleep(TEMP_SECONDS*TIMER_HZ);

		//double operations are not atomic
		double tmp_temp;
		error = ds18b20_read_temp(onewiredev,&tmp_temp);
		if (!error) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				temp = tmp_temp;
			}

	
			temp_error = 0;
		} else {
			temp_error = error;
		}
	}
}

int8_t get_temp(double *temp_ret) {
	*temp_ret = temp;
	return temp_error;
}

static void onewire_init(void) {
	twi_master_t *twim = twi_master_init(&ONEWIRE_TWI.MASTER, ONEWIRE_TWI_BAUD, NULL, NULL);
	twi_master_set_blocking(twim, block, onewire_schedule);
	onewiredev = ds2483_init(twim,&ONEWIRE_SLPZ_PORT,_PIN(ONEWIRE_SLPZ_PIN));
}

static inline void onewire_sleep(uint16_t ms) {
	add_timer(onewire_schedule, ms, 1);
	block();
}

static void onewire_schedule(void) {
	task_schedule(onewire_resume);
}

static void onewire_resume(void) {
	threads_switchto(1);
}

//@TODO!!!
DS2483_INTERRUPT_HANDLER(ONEWIRE_TWI_ISR, onewiredev)
