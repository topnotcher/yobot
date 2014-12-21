#include <avr/io.h>
#include "ssr.h"
#include "timer.h"
#include "debug.h"
#include "threads.h"
#include "temp.h"
#include "tasks.h"

#define CLKSYS_Enable( _oscSel ) ( OSC.CTRL |= (_oscSel) )
#define CLKSYS_IsReady( _oscSel ) ( OSC.STATUS & (_oscSel) )
static void sysclk_set_internal_32mhz(void);
static void main_thread(void);

int main(void) {
	sysclk_set_internal_32mhz();

	ssr_init();
	tasks_init();
	init_timers();
	debug_init();
	temp_init();


	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm | PMIC_HILVLEN_bm;
	//interrupts will get enabled when process starts
	//sei();

	threads_init_stack();
	thread_create("main",main_thread);
	thread_create("temp",temp_run);
	threads_start_main();
	return 0;
}

static void main_thread(void) {
	static int i = 0;
	debug_write("hello");
	while (1) {
		tasks_run();
		++i;
	}
}


static void sysclk_set_internal_32mhz(void) {
	// Enable 32 MHz Osc. 
	CLKSYS_Enable( OSC_RC32MEN_bm ); 
	// Wait for 32 MHz Osc. to stabilize 
	while ( CLKSYS_IsReady( OSC_RC32MRDY_bm ) == 0 ); 
	// was 8  PLL mult. factor ( 2MHz x8 ) and set clk source to PLL. 
	OSC_PLLCTRL = 16;  

	//enable PLL
	OSC_CTRL = 0x10;

	//wait for PLL clk to be ready
	while( !( OSC_STATUS & 0x10 ) );

	//Unlock seq. to access CLK_CTRL
	CCP = 0xD8; 

	// Select PLL as sys. clk. These 2 lines can ONLY go here to engage the PLL ( reverse of what manual A pg 81 says )
	CLK_CTRL = 0x04; 

}
