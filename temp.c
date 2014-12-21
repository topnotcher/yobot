#include <stdlib.h>
#include <stdio.h>

#include <util/atomic.h>

#include "temp.h"
#include "timer.h"
#include "tasks.h"
#include "threads.h"
#include "ds2483.h"
#include "debug.h"
#include "config.h"

#define _CONCAT3(a,b,c) a##b##c
#define _PIN(id) _CONCAT3(PIN,id,_bm)

//applicable when there is only ONE one wire device on the bus!
#define TEMP_CMD_SKIP_ROM 0xCC
#define TEMP_CMD_READ_SCRATCHPAD 0xBE
#define TEMP_CMD_CONVERT_T 0x44

static ds2483_dev_t *onewiredev;
static int16_t temp;

static void onewire_schedule(void);
static void onewire_block(void) __attribute__((naked));
static void onewire_resume(void) __attribute__((naked));
static inline void onewire_sleep(uint16_t);
static void onewire_init(void);
static void temp_read(int16_t*);
static void temp_start_conversion(void);

void temp_init(void) {
	onewire_init();
	add_timer(onewire_schedule, TEMP_SECONDS*1000, 1);
}

void temp_run(void) {
	ds2483_rst(onewiredev);

	while(1) {
		temp_start_conversion();

		//note: manual indicates max 750ms per conversion 
		onewire_sleep(TEMP_SECONDS*TIMER_HZ);

		temp_read(&temp);
	}
}

static void temp_read(int16_t *temp) {
	if (!ds2483_1w_rst(onewiredev))
		return; //no device

	ds2483_1w_write(onewiredev, TEMP_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, TEMP_CMD_READ_SCRATCHPAD);

	uint8_t low = ds2483_1w_read_byte(onewiredev);
	uint8_t high = ds2483_1w_read_byte(onewiredev);

	//note: the low bits in the low byte are fractional
	//16 bit operations are not atomic
	int16_t tmp = (low>>4) | (high<<4);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		*temp = tmp;
	}
}

static void temp_start_conversion() {
	if (!ds2483_1w_rst(onewiredev))
		return; //no device

	ds2483_1w_write(onewiredev, TEMP_CMD_SKIP_ROM);
	ds2483_1w_write(onewiredev, TEMP_CMD_CONVERT_T);
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
