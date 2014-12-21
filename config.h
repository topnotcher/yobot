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
#define DISPLAY_SPI SPIC

// These mappings must correspond to the SPI hardware
#define DISPLAY_SCLK_PIN 7
#define DISPLAY_SOUT_PIN 5
#define DISPLAY_XLAT_PIN 4
#define DISPLAY_SPI_vect SPIC_INT_vect
#define DISPLAY_SIZE 3


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
#define TEMP_SECONDS 2

#endif
