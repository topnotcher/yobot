#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>
#include <string.h>

#include "mempool.h"
#include "debug.h"
#include "queue.h"

#define QUEUE_SIZE 5
#define DEBUG_MAX_LEN 32

typedef struct {
	uint8_t size;
	uint8_t data[DEBUG_MAX_LEN];
} uart_buf;

static void uart_begin_tx(void);

static void uart_tx_interrupt_enable(void) {
	USARTD0.CTRLA |= USART_DREINTLVL_LO_gc;
}

static void uart_tx_interrupt_disable(void) {
	USARTD0.CTRLA &= ~USART_DREINTLVL_LO_gc;
}

typedef struct {
	volatile enum {
		UART_STATUS_IDLE,
		UART_STATUS_BUSY
	} status;

	mempool_t *pool;
	queue_t *queue;

	uint8_t buf_pos;
	uart_buf *buf;
} uart_t;

static uart_t uart;

void debug_init(void) {

	uint16_t bsel = 3332;
	int8_t bscale = -4;

	//BSEL
	USARTD0.BAUDCTRLA = (uint8_t)( bsel & 0x00FF );
	USARTD0.BAUDCTRLB = (bscale<<USART_BSCALE_gp) | (uint8_t)( (bsel>>8) & 0x0F ) ;

	USARTD0.CTRLC |= USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	USARTD0.CTRLB |= USART_TXEN_bm;

	//xmegaA, p237
	PORTD.OUTSET = PIN3_bm;
	PORTD.DIRSET = PIN3_bm;

	uart.pool = init_mempool(sizeof(uart_buf),QUEUE_SIZE);
	uart.queue = queue_create(QUEUE_SIZE);
}

//call with interrupts disabled
static void uart_begin_tx(void) {
	uart_buf *buf = queue_poll(uart.queue);	

	if (buf == NULL) {
		uart.status = UART_STATUS_IDLE;
		return;
	}

	uart.status = UART_STATUS_BUSY;
	uart.buf = buf;
	uart.buf_pos = 0;
	uart_tx_interrupt_enable();
}

void debug_write(void *data,const uint8_t size) {
	uart_buf *buf = mempool_alloc(uart.pool);
	memcpy((void*)(buf->data),data,size);
	buf->size = size;
	queue_offer(uart.queue,buf);

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (uart.status == UART_STATUS_IDLE)
			uart_begin_tx();
	}
}

ISR(USARTD0_DRE_vect) {
	USARTD0.DATA = uart.buf->data[uart.buf_pos];

	if (++uart.buf_pos >= uart.buf->size) {
		mempool_putref(uart.buf);
		uart_tx_interrupt_disable();

		//begin another transfer is there is one queued
		uart_begin_tx();
	}
}
