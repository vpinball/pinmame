#ifndef I4004_H
#define I4004_H

#include "osd_cpu.h"

enum {
	I4004_PC=1, I4004_S1, I4004_S2, I4004_S3, I4004_RAM,
	I4004_01 ,I4004_23, I4004_45, I4004_67, I4004_89, I4004_AB, I4004_CD, I4004_EF,
	I4004_A, I4004_C, I4004_T
};

extern int i4004_ICount;

extern void i4004_init(void);
extern void i4004_reset(void *param);
extern void i4004_exit(void);
extern int i4004_execute(int cycles);
extern unsigned i4004_get_context(void *dst);
extern void i4004_set_context(void *src);
extern unsigned i4004_get_reg(int regnum);
extern void i4004_set_reg(int regnum, unsigned val);
extern void i4004_set_TEST(int state);
extern void i4004_set_irq_line(int irqline, int state);
extern void i4004_set_irq_callback(int (*callback)(int irqline));
extern const char *i4004_info(void *context, int regnum);
extern unsigned i4004_dasm(char *buffer, unsigned pc);

#ifdef	MAME_DEBUG
extern unsigned Dasm4004(char *buffer, unsigned pc);
#endif

#endif
