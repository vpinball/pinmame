// license:BSD-3-Clause

#ifndef PIC32MXCPU_H
#define PIC32MXCPU_H

#include "pic32mx.h"

enum {
	PIC32CPU_PC = 1, PIC32CPU_R0, PIC32CPU_R31 = PIC32CPU_R0 + 31,
	PIC32CPU_HI, PIC32CPU_LO, PIC32CPU_STATUS, PIC32CPU_CAUSE, PIC32CPU_EPC
};

extern int pic32cpu_ICount;

void pic32cpu_init(void);
void pic32cpu_reset(void *param);
void pic32cpu_exit(void);
int pic32cpu_execute(int cycles);
unsigned pic32cpu_get_context(void *dst);
void pic32cpu_set_context(void *src);
unsigned pic32cpu_get_reg(int regnum);
void pic32cpu_set_reg(int regnum, unsigned val);
void pic32cpu_set_irq_line(int irqline, int state);
void pic32cpu_set_irq_callback(int (*callback)(int irqline));
const char *pic32cpu_info(void *context, int regnum);
unsigned pic32cpu_dasm(char *buffer, unsigned pc);

void pic32cpu_set_board(const pic32mx_board *board);
pic32mx *pic32cpu_soc(void);

#endif
