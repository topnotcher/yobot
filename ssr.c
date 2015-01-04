#include <avr/io.h>
#include "ssr.h"
#include "config.h"
#include "timer.h"

#define _SSR_bm SSR_PIN(CONFIG_SSR_PIN)

//xmegaA, 161:
//f_pwm_ss = clk/(N(PER+1))
//so if clk = 32000000, N=1024, PER=31249... then frequency = 1Hz...
//per same page: resolution = log(PER+1)/Log(2)
//
static int16_t level;

void ssr_init() {
	//xmegaA, p159: direction must be set to output
	CONFIG_SSR_PORT.DIRSET = _SSR_bm;
	CONFIG_SSR_PORT.OUTCLR = _SSR_bm;
	TCC0.CTRLB = TC_WGMODE_SS_gc;
	ssr_off();
}

void ssr_level(int16_t lvl) {
	if (lvl <= 0) {
		ssr_off();
		level = 0;
	} else {
		level = lvl;
		TCC0.INTCTRLB |= TC_CCCINTLVL_MED_gc;
		TCC0.CTRLB |= TC0_CCCEN_bm;
		TCC0.CTRLA = TC_CLKSEL_DIV1024_gc;
	}
}

void ssr_off(void) {
	level = 0;
	TCC0.CNT = 0;
	TCC0.PER = SSR_MAX_LEVEL;
	TCC0.CTRLA = TC_CLKSEL_OFF_gc;
	TCC0.CTRLB &= ~TC0_CCCEN_bm;
	TCC0.INTCTRLB &= ~TC_CCCINTLVL_MED_gc;
	CONFIG_SSR_PORT.OUTCLR = _SSR_bm;
}

ISR(TCC0_CCC_vect) {
	TCC0.CCC = level;
}
