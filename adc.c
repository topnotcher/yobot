#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stddef.h>
#include "adc.h"

#define max_readings 5
static uint16_t readings[max_readings] = {0};

static uint8_t adc_read_calibration_byte(const uint8_t offset) {
	NVM.CMD = NVM_CMD_READ_CALIB_ROW_gc;
	uint8_t data = pgm_read_byte(offset);
	NVM.CMD = NVM_CMD_NO_OPERATION_gc;

	return data;
}

static void adc_calibrate(ADC_t *adc) {
	//xmegaA, 38: required to load the calibration at runtime.

	if (adc == &ADCA) {
		adc->CAL = adc_read_calibration_byte(PRODSIGNATURES_ADCACAL0) | adc_read_calibration_byte(PRODSIGNATURES_ADCACAL1)<<8;
	} else {
		adc->CAL = adc_read_calibration_byte(PRODSIGNATURES_ADCBCAL0) | adc_read_calibration_byte(PRODSIGNATURES_ADCBCAL1)<<8;
	}
}

void adc_init(void) {
	ADC_t *adc = &ADCB;

	adc_calibrate(adc);

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

void adc_set_conmode(ADC_t *adc, uint8_t mode) {
	adc->CTRLB = (adc->CTRLB&(~ADC_CONMODE_bm)) | mode;
}

void adc_set_resolution(ADC_t *adc, uint8_t res) {
	adc->CTRLB = (adc->CTRLB&(~ADC_RESOLUTION_gm)) | res;
}

void adc_set_ref(ADC_t *adc, uint8_t ref) {
	adc->REFCTRL = ref;
}

void adc_set_sweep(ADC_t *adc, uint8_t sweep) {
	adc->EVCTRL = (adc->EVCTRL&(~ADC_SWEEP_gm)) | sweep;
}

void adc_set_prescaler(ADC_t *adc, uint8_t prescaler) {
	adc->PRESCALER = prescaler;
}

void adc_enable_freerun(ADC_t *adc, bool enable) {
	adc->CTRLB = (adc->CTRLB&(~ADC_FREERUN_bm)) | (enable ? ADC_FREERUN_bm : 0);
}

void adc_enable(ADC_t *adc) {
	adc->CTRLA |= ADC_ENABLE_bm;
}

void adc_disable(ADC_t *adc) {
	adc->CTRLA &= ~ADC_ENABLE_bm;
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
