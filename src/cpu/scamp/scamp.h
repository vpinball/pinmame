#ifndef SCAMP_H
#define SCAMP_H

#include "osd_cpu.h"

enum {
	SCAMP_PC=1, SCAMP_P1, SCAMP_P2, SCAMP_P3, SCAMP_AC, SCAMP_EX, SCAMP_ST
};

/* Serial I/O Line */
#define SCAMP_SERIAL_PORT 0x100

/* F0, F1, F2 Lines */
#define SCAMP_FLAG_PORT 0x101

/* H Line (pulsed with halt instruction?) */
#define SCAMP_H_PORT 0x102

extern int SCAMP_ICount;

extern void SCAMP_init(void);
extern void SCAMP_reset(void *param);
extern void SCAMP_exit(void);
extern int SCAMP_execute(int cycles);
extern unsigned SCAMP_get_context(void *dst);
extern void SCAMP_set_context(void *src);
extern unsigned SCAMP_get_reg(int regnum);
extern void SCAMP_set_reg(int regnum, unsigned val);
extern void SCAMP_set_irq_line(int irqline, int state);
extern void SCAMP_set_sense_a(int state);
extern void SCAMP_set_sense_b(int state);
extern void SCAMP_set_irq_callback(int (*callback)(int irqline));
extern const char *SCAMP_info(void *context, int regnum);
extern unsigned SCAMP_dasm(char *buffer, unsigned pc);

#ifdef	MAME_DEBUG
extern unsigned DasmScamp(char *buffer, unsigned pc);
#endif

#endif
