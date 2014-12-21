#include <stdlib.h>
#include <stdio.h>

#include <util/atomic.h>

#include "temp.h"
#include "timer.h"
#include "tasks.h"
#include "threads.h"
#include "ds2483.h"
#include "ds18b20.h"
#include "debug.h"
#include "config.h"

#define _CONCAT3(a,b,c) a##b##c
#define _PIN(id) _CONCAT3(PIN,id,_bm)

static ds2483_dev_t *onewiredev;
static int16_t temp;

static void onewire_schedule(void);
static void onewire_block(void) __attribute__((naked));
static void onewire_resume(void) __attribute__((naked));
static inline void onewire_sleep(uint16_t);
static void onewire_init(void);

void temp_init(void) {
	onewire_init();
	add_timer(onewire_schedule, TEMP_SECONDS*1000, 1);
}

/**
 * Temperature monitoring thread.
 */
void temp_run(void) {
	ds2483_rst(onewiredev);

	while(1) {
		ds18b20_start_conversion(onewiredev);

		//@TODO: failsafe if ds18b20_* returns error

		//note: manual indicates max 750ms per conversion 
		onewire_sleep(TEMP_SECONDS*TIMER_HZ);

		//16 bit operations are not atomic
		int16_t tmp_temp;
		if (!ds18b20_read_temp(onewiredev,&tmp_temp)) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				temp = tmp_temp;
			}
		}
	}
}

static void onewire_init(void) {
	twi_master_t *twim = twi_master_init(&ONEWIRE_TWI.MASTER, ONEWIRE_TWI_BAUD, NULL, NULL);
	twi_master_set_blocking(twim, onewire_block, onewire_schedule);
	onewiredev = ds2483_init(twim,&ONEWIRE_SLPZ_PORT,_PIN(ONEWIRE_SLPZ_PIN));
}

static inline void onewire_sleep(uint16_t ms) {
	add_timer(onewire_schedule, ms, 1);
	onewire_block();
}

static void onewire_schedule(void) {
	task_schedule(onewire_resume);
}

static void onewire_resume(void) {
	threads_switchto(1);
}

static void onewire_block(void) {
	threads_switchto(0);
}

//@TODO!!!
DS2483_INTERRUPT_HANDLER(ISR(TWIC_TWIM_vect), onewiredev)
