// license:BSD-3-Clause

#ifndef PIC32MX_H
#define PIC32MX_H

#include "../mips32/mips32.h"

#define PIC32MX_RAM_SIZE   0x20000u
#define PIC32MX_FLASH_SIZE 0x80000u
#define PIC32MX_BOOT_SIZE  0x3000u
#define PIC32MX_SFR_BASE   0x1F800000u
#define PIC32MX_SFR_SIZE   0x90000u
#define PIC32MX_SYSCLK     80000000u
#define PIC32MX_UARTS      6
#define PIC32MX_RXFIFO     8
#define PIC32MX_IRQS       76
#define PIC32MX_VECTORS    64
#define PIC32MX_I2CS       5

enum { PIC32MX_PORTA, PIC32MX_PORTB, PIC32MX_PORTC, PIC32MX_PORTD, PIC32MX_PORTE, PIC32MX_PORTF, PIC32MX_PORTG, PIC32MX_PORTS };

typedef struct pic32mx_board {
	void *ctx;
	void (*port_write)(void *ctx, int port, uint32_t lat, uint32_t tris, uint64_t cycle);
	uint32_t (*port_read)(void *ctx, int port, uint64_t cycle);
	void (*uart_tx)(void *ctx, int uart, uint8_t byte, uint64_t cycle);
	int (*i2c_pins)(void *ctx, int module, int scl, int sda, uint64_t cycle);
	void (*unmapped)(void *ctx, uint32_t pa, int write);
	void (*exception)(void *ctx, int code, uint32_t pc);
	uint64_t (*hold)(void *ctx, uint64_t cycle); /* cycles the core must stay held, 0 = run */
	/* the true bits of a port read that pic32mx_uncertain marked, in port_read's value; 1 when known (always with wait) */
	int (*port_settle)(void *ctx, uint32_t token, int wait, uint32_t *bits);
} pic32mx_board;

typedef struct pic32mx_timer {
	uint32_t tmr, frac;
	uint64_t last;
} pic32mx_timer;

typedef struct pic32mx_i2c {
	int pending;
	uint64_t done_at;
	uint32_t con_clear, stat_set, stat_clear, rcv;
} pic32mx_i2c;

typedef struct pic32mx_uart {
	uint8_t rx[PIC32MX_RXFIFO];
	int rx_head, rx_count;
} pic32mx_uart;

typedef struct pic32mx {
	mips32_state cpu;
	pic32mx_board board;
	const uint8_t *flash;
	uint32_t flash_size;
	uint8_t ram[PIC32MX_RAM_SIZE];
	uint8_t boot[PIC32MX_BOOT_SIZE];
	uint32_t sfr[PIC32MX_SFR_SIZE / 4];
	pic32mx_timer timer[5];
	pic32mx_uart uart[PIC32MX_UARTS];
	pic32mx_i2c i2c[PIC32MX_I2CS];
	uint64_t vec_count[PIC32MX_VECTORS];
	uint64_t exc_count;
	uint8_t logged[PIC32MX_SFR_SIZE / 16];
	int *icount;      /* host cycle counter, kept current across board callbacks; NULL = none */
	uint64_t run_end; /* cycle at which the current pic32mx_run ends */
	uint32_t unc, unc_tok; /* port_read's bits not known yet (pic32mx_uncertain) */
} pic32mx;

void pic32mx_init(pic32mx *p, const pic32mx_board *board, const uint8_t *flash, uint32_t flash_size);
void pic32mx_reset(pic32mx *p);
int pic32mx_run(pic32mx *p, int cycles);
void pic32mx_uart_rx(pic32mx *p, int uart, uint8_t byte);
void pic32mx_set_irq(pic32mx *p, int irq);
void pic32mx_uncertain(pic32mx *p, uint32_t mask, uint32_t token); /* from port_read: these bits are settled later */
int pic32mx_irq_vector(int irq);
uint32_t pic32mx_sfr_peek(const pic32mx *p, uint32_t va);

#endif
