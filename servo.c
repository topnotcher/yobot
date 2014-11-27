#include <avr/io.h>
#include <util/delay.h>

int main(void) {
	//xmegaA, pp160. In frequency generation mode, freq = clk/(2N(CCA+1)), N = prescaler
	//also pp160, freq = clk/(N(PER+1))
	// /1024 = 1953.125
	TCC0.CTRLA |= TC_CLKSEL_DIV64_gc /*TC_CLKSEL_DIV256*/;
	TCC0.CTRLB |= TC0_CCAEN_bm | TC_WGMODE_DSBOTH_gc ;
	TCC0.PER = 312;

	PORTC.DIRSET = PIN0_bm;
	PORTC.OUTCLR = PIN0_bm;
	//10-38
	TCC0.CCA = 24;
	while (1);
}
