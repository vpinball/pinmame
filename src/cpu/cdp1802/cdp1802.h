/*****************************************************************************
 *
 *	 cdp1802.h
 *	 portable cosmac cdp1802 emulator interface
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
#ifndef _CDP1802_H
#define _CDP1802_H

/* missing mark */

/* processor takes 8 external clocks to do something
   so specify /8 in mame's  machine structure */
#include "cpuintrf.h"
#include "osd_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef RUNTIME_LOADER
	extern void cdp1802_runtime_loader_init(void);
#endif


typedef struct {
	/* called after execution of an instruction with cycles,
	   return cycles taken by dma hardware */
	void (*dma)(int cycles);
	void (*out_n)(int data, int n);
	int (*in_n)(int n);
	void (*out_q)(int level);
	int (*in_ef)(void);
} CDP1802_CONFIG;

void cdp1802_dma_write(UINT8 data);
int cdp1802_dma_read(void);

#define CDP1802_INT_NONE 0
#define CDP1802_IRQ 1

extern int cdp1802_icount;				/* cycle count */

extern void cdp1802_init(void);
extern void cdp1802_reset(void *param);
extern void cdp1802_exit(void);
extern int cdp1802_execute(int cycles);
extern unsigned cdp1802_get_context(void *dst);
extern void cdp1802_set_context(void *src);
extern unsigned cdp1802_get_reg(int regnum);
extern void cdp1802_set_reg(int regnum, unsigned val);
extern void cdp1802_set_nmi_line(int state);
extern void cdp1802_set_irq_line(int irqline, int state);
extern void cdp1802_set_irq_callback(int (*callback)(int irqline));
extern void cdp1802_state_save(void *file);
extern void cdp1802_state_load(void *file);
extern const char *cdp1802_info(void *context, int regnum);
extern unsigned cdp1802_dasm(char *buffer, unsigned pc);

#ifdef __cplusplus
}
#endif

#endif
