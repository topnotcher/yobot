#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stddef.h>
#include "display.h"

#define max_readings 5
static uint16_t readings[max_readings] = {0};

void adc_init(void) {
	//xmegaA, 38: required to load the calibration at runtime.
	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
	ADCB.CAL = pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
	ADCB.CAL |= pgm_read_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0))>>8;
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;


	ADCB.CTRLB |= ADC_FREERUN_bm;
	//xmegaA, 297: 0 indicates unsigned conversion (default)
	//12 bit resolutio, right aligned is also default, but set anyway.
	ADCB.CTRLB &= ~ADC_CONMODE_bm & ~ADC_RESOLUTION_gm;
	ADCB.CTRLB |= ADC_RESOLUTION_12BIT_gc;

	//ADC reference voltage is the AREF pin on PORTB
	ADCB.REFCTRL |= ADC_REFSEL_AREFB_gc;

	//xmegaA,299: read channel 0 in freerun
	ADCB.EVCTRL |= ADC_SWEEP_0_gc;

	ADCB.PRESCALER |= ADC_PRESCALER_DIV512_gc;

	ADCB.CH0.CTRL |= ADC_CH_INPUTMODE_SINGLEENDED_gc;
	ADCB.CH0.INTCTRL |= ADC_CH_INTLVL_MED_gc;
	ADCB.CH0.MUXCTRL |= ADC_CH_MUXPOS_PIN1_gc;

	ADCB.CTRLA |= ADC_ENABLE_bm;
}

ISR(ADCB_CH0_vect) {
	//0...3 0->1, 1->2
	for (int8_t i = max_readings-2; i >= 0; --i)
		readings[i+1] = readings[i];

	readings[0] = ADCB.CH0.RES;
}

uint8_t adc_get_temp(void) {
	double res = 0;
	for (uint8_t i = 0; i < max_readings; ++i)
		res += readings[i];

	res /= max_readings;

	/*
	ADC reading gives us a voltage (res). We want a resistance. 
	per avr1300:
	res=(vi+.05*vref)/vref*4096
	voltage divider: vi = R/(R+10k)*vref (series resistor is 10k)
	substitute (R/(R+10k)*vref+dV)/vref*4096
	also per AVR1300, dV = .05*vref, thus 
	res =  (R/(R+10k)*vref+.05*vref)/vref*4096 = vref*(R/(R+10k)+.05)/vref*4096
	now the vrefs cancel out
	res = (R/(R+10k)+.05)*4096
	we get: R = -(10k(5*res-1024))/(5*res-21504)
	*/

	res = -1 * ((10000*(5*res - 1024))/(5*res-21504));
	//steinhart-hart: 1/T = 1/T0+1/B*log(R/R0)
	double s = 1.0 / (1.0/(25.0+273.15) + 1.0/3435.0 * log(res/10000));

	//convert from kelvin to fahrenheit
	s = (s-273.15)*1.8+32.0;
	

	return (uint8_t)s;
}
