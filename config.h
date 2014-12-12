#ifndef CONFIG_H
#define CONFIG_H
/**
 * Thermistor configuration
 */

// average this many readings to reduce noise
#define CONFIG_TEMPERATURE_READINGS 5

// use this ADC (exclusive access)
#define CONFIG_TEMPERATURE_ADC ADCB

// resistance at 25 celsius  (for use in steinhart-hart beta paremeter)
#define CONFIG_THERMISTOR_R25 10000

// beta value
#define CONFIG_THERMISTOR_BETA 3435
#endif
