#ifndef CONFIG_H
#define CONFIG_H

/***************************************
 * Solid state relay configuration
 **************************************/
#define CONFIG_SSR_PORT PORTC
#define CONFIG_SSR_PIN 2

/***************************************
 * Display configuration
 **************************************/
#define DISPLAY_PORT PORTC
#define DISPLAY_USART USARTC1

// These mappings must correspond to the SPI hardware
#define DISPLAY_SCLK_PIN 5
#define DISPLAY_SOUT_PIN 7
#define DISPLAY_XLAT_PIN 4
#define DISPLAY_DRE_vect USARTC1_DRE_vect
#define DISPLAY_TXC_vect USARTC1_TXC_vect
#define DISPLAY_BSEL 8
#define DISPLAY_SIZE 8

/**
 * 1WB to TWI bridge configuration
 */
#define ONEWIRE_TWI TWIC
#define ONEWIRE_TWI_BAUD 35
#define ONEWIRE_TWI_ISR ISR(TWIC_TWIM_vect)
#define ONEWIRE_SLPZ_PORT PORTC
#define ONEWIRE_SLPZ_PIN 6

/**
 * Temperature configuration
 */

//check the temperature every ... seconds
#define TEMP_SECONDS 1


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
#define KEYPAD_PIN_R1 3
#define KEYPAD_PIN_R2 5
#define KEYPAD_PIN_R3 1
#define KEYPAD_PIN_R4 7
#define KEYPAD_PIN_C1 2
#define KEYPAD_PIN_C2 4
#define KEYPAD_PIN_C3 6
#define KEYPAD_PIN_C4 0

/**
 * In order to kill contact bounce, when a keypress is detected, the keys are
 * scanned KEYPAD_SCAN_SAMPLES every KEYPAD_SCAN_DELAY milliseconds. This
 * happens twice: once for the rows, once for the cols. Thus delay=5, samples
 * = 3 => 30ms spent reading a key.
 */
#define KEYPAD_SCAN_DELAY 5
#define KEYPAD_SCAN_SAMPLES 3

//after keypress is recorded, ignore keypresses for this many milliseconds
#define KEYPAD_REPEAT_RATE 250

/**
 * Alarm Configuration
 */

#define ALARM_PORT PORTD
#define ALARM_PIN 0

#endif
