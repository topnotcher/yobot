#include <avr/io.h>
#include "ssr.h"
#include "timer.h"
#include "debug.h"

int main(void) {
	ssr_init();
	init_timers();
	debug_init();


	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	debug_write("hello");

	while (1) {
	}
}
