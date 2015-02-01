#include <avr/io.h>
#include "alarm.h"
#include "config.h"

void alarm_init() {
	ALARM_PORT.DIRSET = _BV(ALARM_PIN);
	ALARM_PORT.OUTCLR = _BV(ALARM_PIN);
}

void alarm_off() {
	ALARM_PORT.OUTCLR = _BV(ALARM_PIN);
}

void alarm_on() {
	ALARM_PORT.OUTSET = _BV(ALARM_PIN);
}
