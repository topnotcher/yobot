#include <stdbool.h>
#ifndef ADC_H
#define ADC_H
void adc_init(void);
uint8_t adc_get_temp(void);


void adc_set_conmode(ADC_t *adc, uint8_t mode);
void adc_set_resolution(ADC_t *adc, uint8_t res);
void adc_set_ref(ADC_t *adc, uint8_t ref);
void adc_set_sweep(ADC_t *adc, uint8_t sweep);
void adc_set_prescaler(ADC_t *adc, uint8_t prescaler);
void adc_enable(ADC_t *adc);
void adc_disable(ADC_t *adc);
void adc_enable_freerun(ADC_t *adc, bool enable);
#endif
