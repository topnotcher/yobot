#include <avr/io.h>
#include "alarm.h"
#include "timer.h"

void alarm_init() {
	PORTD.DIRSET = _BV(0);
	PORTD.OUTCLR = _BV(0);
}

void alarm_off() {
	PORTD.OUTCLR = _BV(0);
}

void alarm_on() {
	PORTD.OUTSET = _BV(0);
}
