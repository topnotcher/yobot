#ifndef CONFIG_H
#define CONFIG_H

/***************************************
 * Thermistor configuration
 **************************************/

// average this many readings to reduce noise
#define CONFIG_TEMPERATURE_READINGS 5

// use this ADC (exclusive access)
#define CONFIG_TEMPERATURE_ADC ADCA

//@TODO channel is hard coded

// resistance at 25 celsius  (for use in steinhart-hart beta paremeter)
// NOTE: The code currently _assumes_ the series resistor is 10k
#define CONFIG_THERMISTOR_R25 10000

// beta value
#define CONFIG_THERMISTOR_BETA 3435

/***************************************
 * Solid state relay configuration
 **************************************/
#define CONFIG_SSR_PORT PORTB
#define CONFIG_SSR_PIN 2

/***************************************
 * Display configuration
 **************************************/
#define DISPLAY_PORT PORTC
#define DISPLAY_SPI SPIC

// These mappings must correspond to the SPI hardware
#define DISPLAY_SCLK_PIN 7
#define DISPLAY_SOUT_PIN 5
#define DISPLAY_XLAT_PIN 4
#define DISPLAY_SPI_vect SPIC_INT_vect
#define DISPLAY_SIZE 3

/***************************************
 * Keypad configuration
 **************************************/

// PORT and interrupt vector for keypad pins.
#define KEYPAD_PORT PORTA
#define KEYPAD_ISR ISR(PORTA_INT0_vect)
/*
 * Map the port pins to the keypad rows columns.
 * "KEYPAD_PIN_R2 6" means row 2 on the keypad is 
 * connected to PIN6 on the port (PA6). 
 */
#define KEYPAD_PIN_R1 1
#define KEYPAD_PIN_R2 6
#define KEYPAD_PIN_R3 5
#define KEYPAD_PIN_R4 3
#define KEYPAD_PIN_C1 2
#define KEYPAD_PIN_C2 0
#define KEYPAD_PIN_C3 4


#endif
