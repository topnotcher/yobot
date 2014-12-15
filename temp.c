#include <math.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "adc.h"
#include "config.h"

// The number of readings to average together to help reduce noise/even out
// fluctuations
#ifndef CONFIG_TEMPERATURE_READINGS
#define CONFIG_TEMPERATURE_READINGS 5
#endif

static uint16_t readings[CONFIG_TEMPERATURE_READINGS] = {0};

void thermistor_init(void) {
	ADC_t *adc = &CONFIG_TEMPERATURE_ADC;

	adc_enable_freerun(adc,true);
	//xmegaA, 297: 0 indicates unsigned conversion (default)
	//12 bit resolution, right aligned is also default, but set anyway.
	adc_set_conmode(adc, 0);
	adc_set_resolution(adc, ADC_RESOLUTION_12BIT_gc);

	//ADC reference voltage is the AREF pin on PORTB
	adc_set_ref(adc, ADC_REFSEL_AREFB_gc);

	//xmegaA,299: read channel 0 in freerun
	adc_set_sweep(adc, ADC_SWEEP_0_gc);

	adc_set_prescaler(adc, ADC_PRESCALER_DIV512_gc);

	adc->CH0.CTRL |= ADC_CH_INPUTMODE_SINGLEENDED_gc;
	adc->CH0.INTCTRL |= ADC_CH_INTLVL_MED_gc;
	adc->CH0.MUXCTRL |= ADC_CH_MUXPOS_PIN1_gc;

	adc_enable(adc);
}

ISR(ADCB_CH0_vect) {
	//0...3 0->1, 1->2
	for (int8_t i = CONFIG_TEMPERATURE_READINGS-2; i >= 0; --i)
		readings[i+1] = readings[i];

	readings[0] = ADCB.CH0.RES;
}

uint8_t thermistor_read_temp(void) {
	double res = 0;
	for (uint8_t i = 0; i < CONFIG_TEMPERATURE_READINGS; ++i)
		res += readings[i];

	res /= CONFIG_TEMPERATURE_READINGS;

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

	//ASSUMPTION: series resistor is 10k
	res = -1 * ((10000*(5*res - 1024))/(5*res-21504));
	//steinhart-hart (beta parameter): 1/T = 1/T0+1/B*log(R/R0)
	//see: http://en.wikipedia.org/wiki/Thermistor#B_or_.CE.B2_parameter_equation
	double s = 1.0 / (1.0/(25.0+273.15) + 1.0/(CONFIG_THERMISTOR_BETA) * log(res/CONFIG_THERMISTOR_R25));

	//convert from kelvin to fahrenheit
	s = (s-273.15)*1.8+32.0;

	return (uint8_t)s;
}
