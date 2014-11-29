#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <stddef.h>
#include "display.h"


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

	double res = ADCB.CH0.RES;
	res = 4095.0/(res-205)-1;
	res = 10000/res;
	double s;

	s = log(res/10000); 
	s /= 3950;
	s += 1.0 / (25+273.15);
	s = 1.0/s;
	s = (s-273.15)*1.8+32;
	uint8_t temp = s;

	//uint16_t k = ((double)res-205) /(4095.0) * 100;

	char foo[] = {0,0,0};
	foo[2] = temp % 10 + 0x30;
	temp /= 10;
	foo[1] = temp % 10 + 0x30;
	temp /= 10;
	foo[0] = temp % 10 + 0x30;
	display_puts(foo); 

	/*
	if (ADCB.CH1.RES > 2000 && ADCB.CH1.RES <= 4095)
		display_putchar('1');
	else 
		display_putchar('0');
		*/
}
