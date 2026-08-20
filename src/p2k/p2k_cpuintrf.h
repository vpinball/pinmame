// license:BSD-3-Clause

/* PinMAME P2K subsystem - C view of the MediaGX bridge (see p2k_cpuintrf.cpp).
   Included by src/cpuintrf.c so the CPU table can reference the adapter */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern int mediagx_ICount;

void mediagx_init(void);
void mediagx_reset(void *param);
void mediagx_exit(void);
int mediagx_execute(int cycles);
unsigned mediagx_get_context(void *dst);
void mediagx_set_context(void *src);
unsigned mediagx_get_reg(int regnum);
void mediagx_set_reg(int regnum, unsigned val);
void mediagx_set_irq_line(int irqline, int linestate);
void mediagx_set_irq_callback(int (*callback)(int irqline));
const char *mediagx_info(void *context, int regnum);
unsigned mediagx_dasm(char *buffer, unsigned pc);

#ifdef __cplusplus
}
#endif
