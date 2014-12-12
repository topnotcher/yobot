#include <avr/pgmspace.h>
#include <avr/io.h>
#include "adc.h"

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

void adc_init(ADC_t *adc) {
	adc_calibrate(adc);
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
