#ifndef PPS4_H
#define PPS4_H

#include "osd_cpu.h"

#define INVERT_DATA 0

enum {
	PPS4_PC=1, PPS4_SA, PPS4_SB, PPS4_BX, PPS4_AB,
	PPS4_DB, PPS4_A, PPS4_C, PPS4_X, PPS4_F1, PPS4_F2, PPS4_SK
};

extern int PPS4_ICount;

extern void PPS4_init(void);
extern void PPS4_reset(void *param);
extern void PPS4_exit(void);
extern int PPS4_execute(int cycles);
extern unsigned PPS4_get_context(void *dst);
extern void PPS4_set_context(void *src);
extern unsigned PPS4_get_reg(int regnum);
extern void PPS4_set_reg(int regnum, unsigned val);
extern void PPS4_set_irq_line(int irqline, int state);
extern void PPS4_set_irq_callback(int (*callback)(int irqline));
extern const char *PPS4_info(void *context, int regnum);
extern unsigned PPS4_dasm(char *buffer, unsigned pc);

#ifdef	MAME_DEBUG
extern unsigned DasmPPS4(char *buffer, unsigned pc);
#endif

#endif
