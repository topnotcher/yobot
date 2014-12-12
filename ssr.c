#include <avr/io.h>
#include "ssr.h"
#include "config.h"

#define _SSR_bm SSR_PIN(CONFIG_SSR_PIN)

void ssr_init() {
	CONFIG_SSR_PORT.DIRSET = _SSR_bm;
	ssr_off();
}

void ssr_on(void) {
	CONFIG_SSR_PORT.OUTSET = _SSR_bm;
}

void ssr_off(void) {
	CONFIG_SSR_PORT.OUTCLR = _SSR_bm;
}
