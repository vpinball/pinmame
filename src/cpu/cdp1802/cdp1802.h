#ifndef _CDP1802_H
#define _CDP1802_H

#include "mame.h"
#include "cpuintrf.h"

extern int cdp1802_icount;

extern void cdp1802_init(void);
extern void cdp1802_reset(void *param);
extern void cdp1802_exit(void);
extern int cdp1802_execute(int cycles);
extern unsigned cdp1802_get_context(void *dst);
extern void cdp1802_set_context(void *src);
extern unsigned cdp1802_get_reg(int regnum);
extern void cdp1802_set_reg(int regnum, unsigned val);
extern const char *cdp1802_info(void *context, int regnum);
extern void cdp1802_set_irq_line(int irqline, int state);
extern void cdp1802_set_irq_callback(int (*callback)(int irqline));

extern unsigned cdp1802_dasm(char *buffer, unsigned pc);

enum {
	CDP1802_INPUT_LINE_INT = 0,
	CDP1802_INPUT_LINE_DMAIN,
	CDP1802_INPUT_LINE_DMAOUT
};

enum {
	EF1 = 0x01,
	EF2 = 0x02,
	EF3 = 0x04,
	EF4 = 0x08
};

enum _cdp1802_control_mode {
	CDP1802_MODE_LOAD = 0,
	CDP1802_MODE_RESET,
	CDP1802_MODE_PAUSE,
	CDP1802_MODE_RUN
};

typedef enum _cdp1802_control_mode cdp1802_control_mode;

enum _cdp1802_state {
	CDP1802_STATE_CODE_S0_FETCH = 0,
	CDP1802_STATE_CODE_S1_EXECUTE,
	CDP1802_STATE_CODE_S2_DMA,
	CDP1802_STATE_CODE_S3_INTERRUPT
};
typedef enum _cdp1802_state cdp1802_state;

// CDP1802 Registers

enum {
	CDP1802_P = 1,	// Designates which register is Program Counter
	CDP1802_X,		// Designates which register is Data Pointer
	CDP1802_D,		// Data Register (Accumulator)
	CDP1802_B,		// Auxiliary Holding Register
	CDP1802_T,		// Holds old X, P after Interrupt (X is high nibble)

	CDP1802_R0,		// 1 of 16 Scratchpad Registers
	CDP1802_R1,
	CDP1802_R2,
	CDP1802_R3,
	CDP1802_R4,
	CDP1802_R5,
	CDP1802_R6,
	CDP1802_R7,
	CDP1802_R8,
	CDP1802_R9,
	CDP1802_Ra,
	CDP1802_Rb,
	CDP1802_Rc,
	CDP1802_Rd,
	CDP1802_Re,
	CDP1802_Rf,

	CDP1802_DF,		// Data Flag (ALU Carry)
	CDP1802_IE,		// Interrupt Enable
	CDP1802_Q,		// Output Flip-Flop
	CDP1802_N,		// Holds Low-Order Instruction Digit
	CDP1802_I,		// Holds High-Order Instruction Digit
};

typedef struct
{
	UINT8 (*mode_r)(void);
	UINT8 (*ef_r)(void);
	void (*sc_w)(int state);
	void (*q_w)(int level);
	UINT8 (*dma_r)(int offset);
	void (*dma_w)(int offset, UINT8 data);
} CDP1802_CONFIG;

#endif
