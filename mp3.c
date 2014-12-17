#include <avr/io.h>
#include <util/atomic.h>
#include <stdint.h>
#include <string.h>
#include <util/delay.h>

#include "timer.h"
#include "mempool.h"
#include "mp3.h"
#include "queue.h"

#define QUEUE_SIZE 3

#define CMD_PLAY 0x0D
#define CMD_VOLUME 0x06
#define CMD_SELECT 0x03


typedef struct {
	uint8_t start;
	uint8_t cmd;
	uint8_t feedback;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t chk1;
	uint8_t chk2;
	uint8_t end;
} mp3_cmd;

//001_180-190.mp3  002_190-200.mp3  003_200-212.mp3  004_212+.mp3  005_buttons.mp3  006_done.mp3  007_stop.mp3  008_success.mp3  009_under180.mp3  010_welcome.mp3

static void uart_write(mp3_cmd *cmd);
static void uart_begin_tx(void);

static void uart_tx_interrupt_enable(void) {
	USARTD0.CTRLA |= USART_DREINTLVL_MED_gc;
}

static void uart_tx_interrupt_disable(void) {
	USARTD0.CTRLA &= ~USART_DREINTLVL_MED_gc;
}


static inline void mp3_checksum(mp3_cmd *cmd) {
	uint16_t chk = 0xFFFF - (cmd->cmd + cmd->arg1 + cmd->arg2 + cmd->feedback) + 1;
	cmd->chk2 = chk&0xFF;
	cmd->chk1 = (chk>>8)&0xFF;
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

uart_t uart;

void mp3_init(void) {

	uint16_t bsel = 1666;
	int8_t bscale = -7;

	//BSEL
	USARTD0.BAUDCTRLA = (uint8_t)( bsel & 0x00FF );
	USARTD0.BAUDCTRLB = (bscale<<USART_BSCALE_gp) | (uint8_t)( (bsel>>8) & 0x0F ) ;

	USARTD0.CTRLC |= USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	//USARTD0.CTRLA |= USART_RXCINTLVL_MED_gc;
	USARTD0.CTRLB |= /*USART_RXEN_bm |*/ USART_TXEN_bm;

	//xmegaA, p237
	PORTD.OUTSET = PIN3_bm;
	PORTD.DIRSET = PIN3_bm;

	uart.pool = init_mempool(sizeof(mp3_cmd),QUEUE_SIZE);
	uart.queue = queue_create(QUEUE_SIZE);

	mp3_send_cmd(CMD_VOLUME, 0x00, 0x1E);
}

//call with interrupts disabled
static void uart_begin_tx(void) {
	mp3_cmd *buf = queue_poll(uart.queue);	

	if (buf == NULL) {
		uart.status = UART_STATUS_IDLE;
		return;
	}

	uart.status = UART_STATUS_BUSY;
	uart.buf = (uint8_t*)buf;
	uart.buf_size = sizeof(*buf);
	uart.buf_pos = 0;
	uart_tx_interrupt_enable();
}

static void uart_write(mp3_cmd *cmd) {
	queue_offer(uart.queue,cmd);

	//when the status is busy, the TX will begin 100ms after
	//the current tx finishes
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (uart.status == UART_STATUS_IDLE)
			uart_begin_tx();
	}
}

void mp3_send_cmd(uint8_t cmd_val, uint8_t arg1, uint8_t arg2) {
	mp3_cmd *cmd = mempool_alloc(uart.pool);
	
	if (cmd == NULL)
		return;

	cmd->start = 0x7E;
	cmd->feedback = 0x00;
	cmd->arg1 = 0x00;
	cmd->arg2 = arg2;
	cmd->end = 0xEF;
	cmd->cmd = cmd_val;
	mp3_checksum(cmd);
	uart_write(cmd);
}

void mp3_play(uint8_t num) {
	mp3_send_cmd(CMD_SELECT,0x01, num);
	mp3_send_cmd(CMD_PLAY,0x00, 0x00);
}

ISR(USARTD0_DRE_vect) {
	USARTD0.DATA = uart.buf[uart.buf_pos];

	if (++uart.buf_pos >= uart.buf_size) {
		mempool_putref(uart.buf);
		uart_tx_interrupt_disable();

		//NOT setting status busy here because the mp3 chip can't handle
		//commands as fast as we can send them (WHY WHY WHY???)
		add_timer(uart_begin_tx, 200, 1);
	}
}
