/*****************************************************************************
 *
 *	 arm7.h
 *	 Portable ARM7TDMI Core Emulator
 *
 *	 Copyright (c) 2004 Steve Ellenoff, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   sellenoff@hotmail.com
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *	This work is based on:
 *  #1) 'Atmel Corporation ARM7TDMI (Thumb) Datasheet - January 1999'
 *  #2) Arm 2/3/6 emulator By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino (MAME CORE 0.76)
 *
 *****************************************************************************/

#ifndef ARM7_H
#define ARM7_H

#include "driver.h"

/****************************************************************************************************
 *	PUBLIC GLOBALS
 ***************************************************************************************************/

extern int arm7_ICount;

/****************************************************************************************************
 *	PUBLIC FUNCTIONS
 ***************************************************************************************************/

extern void arm7_init(void);
extern void arm7_reset(void *param);
extern void arm7_exit(void);
extern int arm7_execute(int cycles);
extern unsigned arm7_get_context(void *dst);
extern void arm7_set_context(void *src);
extern unsigned arm7_get_pc(void);
extern void arm7_set_pc(unsigned val);
extern unsigned arm7_get_sp(void);
extern void arm7_set_sp(unsigned val);
extern unsigned arm7_get_reg(int regnum);
extern void arm7_set_reg(int regnum, unsigned val);
extern void arm7_interrupt( int type );
extern void arm7_set_nmi_line(int state);
extern void arm7_set_irq_line(int irqline, int state);
extern void arm7_set_irq_callback(int (*callback)(int irqline));
extern const char *arm7_info(void *context, int regnum);
extern unsigned arm7_dasm(char *buffer, unsigned pc);

#endif /* ARM7_H */
