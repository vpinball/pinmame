/*****************************************************************************
 *
 *	 at91.h
 *	 Portable ATMEL 91 ARM Thumb MCU Family Emulator ( ARM7TDMI Core )
 *
 *   Chips in the family:
 *   (AT91M40800, AT91R40807,AT91M40807,AT91R40008)
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
 *	#1) 'AT91 ARM Thumb Microcontrollers Users Manual & Summary Sheet'
 *  #2) 'Atmel Corporation ARM7TDMI (Thumb) Datasheet - January 1999'
 *  #3) Arm 2/3/6 emulator By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino (MAME CORE 0.76)
 *
 *****************************************************************************/
#ifndef AT91_H
#define AT91_H

#include "driver.h"

/****************************************************************************************************
 *	INTERRUPT CONSTANTS
 ***************************************************************************************************/

#define AT91_IRQ_LINE	0
#define AT91_FIRQ_LINE	1

/****************************************************************************************************
 *	PUBLIC GLOBALS
 ***************************************************************************************************/

extern int at91_ICount;

/****************************************************************************************************
 *	PUBLIC FUNCTIONS
 ***************************************************************************************************/

extern void at91_init(void);
extern void at91_reset(void *param);
extern void at91_exit(void);
extern int at91_execute(int cycles);
extern unsigned at91_get_context(void *dst);
extern void at91_set_context(void *src);
extern unsigned at91_get_pc(void);
extern void at91_set_pc(unsigned val);
extern unsigned at91_get_sp(void);
extern void at91_set_sp(unsigned val);
extern unsigned at91_get_reg(int regnum);
extern void at91_set_reg(int regnum, unsigned val);
extern void at91_interrupt( int type );
extern void at91_set_nmi_line(int state);
extern void at91_set_irq_line(int irqline, int state);
extern void at91_set_irq_callback(int (*callback)(int irqline));
extern const char *at91_info(void *context, int regnum);
extern unsigned at91_dasm(char *buffer, unsigned pc);

//interfaces to the drivers using the chip
extern void at91_set_ram_pointers(data32_t *reset_ram_ptr, data32_t *page0_ram_ptr);
extern void at91_cs_callback_r(offs_t start, offs_t end, READ32_HANDLER((*callback)));
extern void at91_cs_callback_w(offs_t start, offs_t end, WRITE32_HANDLER((*callback)));
extern void at91_ready_irq_callback_w(WRITE32_HANDLER((*callback)));

#ifdef MAME_DEBUG
extern void at91_disasm( char *pBuf, data32_t pc, data32_t opcode );
#endif

enum
{
	AT9132_R0=1, AT9132_R1, AT9132_R2, AT9132_R3, AT9132_R4, AT9132_R5, AT9132_R6, AT9132_R7,
	AT9132_R8, AT9132_R9, AT9132_R10, AT9132_R11, AT9132_R12, AT9132_R13, AT9132_R14, AT9132_R15,
	AT9132_FR8, AT9132_FR9, AT9132_FR10, AT9132_FR11, AT9132_FR12, AT9132_FR13, AT9132_FR14,
	AT9132_IR13, AT9132_IR14, AT9132_SR13, AT9132_SR14, AT9132_FSPSR, AT9132_ISPSR, AT9132_SSPSR,
	AT9132_CPSR
};

#endif /* AT91_H */
