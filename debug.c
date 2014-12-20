#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>
#include <string.h>

#include "mempool.h"
#include "debug.h"
#include "queue.h"

#define QUEUE_SIZE 5
#define DEBUG_MAX_LEN 32

static void uart_begin_tx(void);

static void uart_tx_interrupt_enable(void) {
	USARTC1.CTRLA |= USART_DREINTLVL_LO_gc;
}

static void uart_tx_interrupt_disable(void) {
	USARTC1.CTRLA &= ~USART_DREINTLVL_LO_gc;
}

typedef struct {
	volatile enum {
		UART_STATUS_IDLE,
		UART_STATUS_BUSY
	} status;

	mempool_t *pool;
	queue_t *queue;

	uint8_t buf_size;
	uint8_t buf_pos;
	uint8_t *buf;
} uart_t;

static uart_t uart;

void debug_init(void) {

	uint16_t bsel = 1666;
	int8_t bscale = -7;

	//BSEL
	USARTC1.BAUDCTRLA = (uint8_t)( bsel & 0x00FF );
	USARTC1.BAUDCTRLB = (bscale<<USART_BSCALE_gp) | (uint8_t)( (bsel>>8) & 0x0F ) ;

	USARTC1.CTRLC |= USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTC1.CTRLB |= USART_TXEN_bm;

	//xmegaA, p237
	PORTC.OUTSET = PIN7_bm;
	PORTC.DIRSET = PIN7_bm;

	uart.pool = init_mempool(DEBUG_MAX_LEN,QUEUE_SIZE);
	uart.queue = queue_create(QUEUE_SIZE);
}

//call with interrupts disabled
static void uart_begin_tx(void) {
	uint8_t *buf = queue_poll(uart.queue);	

	if (buf == NULL) {
		uart.status = UART_STATUS_IDLE;
		return;
	}

	uart.status = UART_STATUS_BUSY;
	uart.buf = buf;
	uart.buf_size = strlen((char*)buf);
	uart.buf_pos = 0;
	uart_tx_interrupt_enable();
}

void debug_write(char *str) {
	void *buf = mempool_alloc(uart.pool);
	strcpy(buf,str);
	queue_offer(uart.queue,buf);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (uart.status == UART_STATUS_IDLE)
			uart_begin_tx();
	}
}

ISR(USARTC1_DRE_vect) {
	USARTC1.DATA = uart.buf[uart.buf_pos];

	if (++uart.buf_pos >= uart.buf_size) {
		mempool_putref(uart.buf);
		uart_tx_interrupt_disable();

		//begin another transfer is there is one queued
		uart_begin_tx();
	}
}
