#include <avr/io.h>
#include "keypad.h"
#include "display.h"
#include "adc.h"

inline void servo_set_angle(uint8_t angle);
static inline void tea_off(void) {
	PORTB.OUTCLR = PIN2_bm;
	servo_set_angle(0);
}

static inline void tea_on(void) {
	PORTB.OUTSET = PIN2_bm;
	servo_set_angle(180);
}

int main(void) {
	//xmegaA, pp160. In frequency generation mode, freq = clk/(2N(CCA+1)), N = prescaler
	//also pp160, freq = clk/(N(PER+1))
	// /1024 = 1953.125
	TCC0.CTRLA |= TC_CLKSEL_DIV64_gc /*TC_CLKSEL_DIV256*/;
	TCC0.CTRLB |= TC0_CCAEN_bm | TC_WGMODE_DSBOTH_gc ;
	TCC0.PER = 312;

	//dirset is required
	PORTC.DIRSET = PIN0_bm;
	PORTC.OUTCLR = PIN0_bm;

	//LED
	PORTC.DIRSET = PIN1_bm;
	PORTC.OUTSET = PIN1_bm;

	keypad_init();
	display_init();
	adc_init();
	
	/** TEA INIT
	 */

	PORTB.DIRSET = PIN2_bm;
	tea_off();

	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	sei();

	uint8_t k = 0;
	while (1) {
		char key;
		if ((key = keypad_getc())) {
			keypad_int_disable();
			//display_putchar(key);

			if (key == '*') {
				tea_off();
			} else if (key == '0') {
				servo_set_angle(90);
			} else if (key == '#') {
				tea_on();
			}
			keypad_int_enable();
		}

		uint8_t temp = adc_get_temp();
		if (temp >= 190) {
			tea_off();
		}

		if (++k == 255) {
			char foo[] = {0,0,0};
			foo[2] = temp % 10 + 0x30;
			temp /= 10;
			foo[1] = temp % 10 + 0x30;
			temp /= 10;
			foo[0] = temp % 10 + 0x30;
			display_puts(foo); 
		}
	}
}

inline void servo_set_angle(uint8_t angle) {
	const uint8_t cca_min = 10;
	const uint8_t cca_max = 38;

	//map [0,180] -> [cca_min, cca_max]
	TCC0.CCA = cca_min + ((float)(cca_max-cca_min))/180 * angle;
}
