#ifndef I4004_H
#define I4004_H

#include "osd_cpu.h"

enum {
	I4004_PC=1, I4004_SP, I4004_AF ,I4004_BC, I4004_DE, I4004_HL,
	I4004_HALT, I4004_IM, I4004_IREQ, I4004_ISRV, I4004_VECTOR,
	I4004_TRAP_STATE, I4004_INTR_STATE,
	I4004_RST55_STATE, I4004_RST65_STATE, I4004_RST75_STATE};

#define I4004_INTR_LINE     0
#define I4004_RST55_LINE	1
#define I4004_RST65_LINE	2
#define I4004_RST75_LINE	3

extern int i4004_ICount;

extern void i4004_set_SID(int state);
extern void i4004_set_SOD_callback(void (*callback)(int state));
extern void i4004_init(void);
extern void i4004_reset(void *param);
extern void i4004_exit(void);
extern int i4004_execute(int cycles);
extern unsigned i4004_get_context(void *dst);
extern void i4004_set_context(void *src);
extern unsigned i4004_get_reg(int regnum);
extern void i4004_set_reg(int regnum, unsigned val);
extern void i4004_set_irq_line(int irqline, int state);
extern void i4004_set_irq_callback(int (*callback)(int irqline));
extern const char *i4004_info(void *context, int regnum);
extern unsigned i4004_dasm(char *buffer, unsigned pc);

#ifdef	MAME_DEBUG
extern unsigned Dasm4004(char *buffer, unsigned pc);
#endif

#endif
