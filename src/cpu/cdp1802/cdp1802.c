/*****************************************************************************
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
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"

#include "cdp1802.h"

typedef int bool;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#ifdef RUNTIME_LOADER
#define cdp1802_ICount cdp1802_icount
struct cpu_interface
cdp1802_interface=
CPU0(CDP1802,  cdp1802,  1,  0,1.00,CDP1802_INT_NONE,  CDP1802_IRQ,    -1,             8, 16,     0,16,BE,1, 3);

extern void cdp1802_runtime_loader_init(void)
{
	cpuintf[CPU_CDP1802]=cdp1802_interface;
}
#endif

enum {
	CDP1802_P=1,
	CDP1802_X,
	CDP1802_D,
	CDP1802_B,
	CDP1802_T,

	CDP1802_R0,
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

	CDP1802_DF,
	CDP1802_IE,
	CDP1802_Q,
	CDP1802_IRQ_STATE
};
/* Layout of the registers in the debugger */
static UINT8 cdp1802_reg_layout[] = {
	CDP1802_R0,
	CDP1802_R1,
	CDP1802_R2,
	CDP1802_R3,
	CDP1802_R4,
	CDP1802_R5,
	CDP1802_R6,
	CDP1802_R7,
	-1,

	CDP1802_R8,
	CDP1802_R9,
	CDP1802_Ra,
	CDP1802_Rb,
	CDP1802_Rc,
	CDP1802_Rd,
	CDP1802_Re,
	CDP1802_Rf,
	-1,

	CDP1802_P,
	CDP1802_X,
	CDP1802_D,
	CDP1802_B,
	CDP1802_T,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 cdp1802_win_layout[] = {
	17, 0,63, 3,	/* register window (top, right rows) */
	 0, 0,16,22,	/* disassembler window (left colums) */
	17, 4,63, 9,	/* memory #1 window (right, upper middle) */
	17,14,63, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
	CDP1802_CONFIG *config;

	PAIR reg[0x10];
	int p, x; // indices to reg, p program count, x data pointer

	UINT8 d,b,t; // xp after entering interrupt

	UINT16 oldpc;

	bool df,ie,q;
	bool irq_state;

	bool idle;
	int dma_cycles;
}	CDP1802_Regs;

int cdp1802_icount = 0;

static CDP1802_Regs cdp1802;

#define PC cdp1802.reg[cdp1802.p].w.l
#define X cdp1802.reg[cdp1802.x].w.l

void cdp1802_dma_write(UINT8 data)
{
	cpu_writemem16(cdp1802.reg[0].w.l++, data);
	cdp1802.idle=0;
	cdp1802_icount--;
}

int cdp1802_dma_read(void)
{
	cdp1802.idle=0;
	cdp1802_icount--;
	return cpu_readmem16(cdp1802.reg[0].w.l++);
}

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "1802tbl.c"

void cdp1802_reset(void *param)
{
	if (param) {
		cdp1802.config=(CDP1802_CONFIG *)param;
	}
	cdp1802.p=0;
	cdp1802.q=0;
	cdp1802.ie=1;
	cdp1802.df=0;
	cdp1802.reg[0].w.l=0;
	change_pc16(PC);

	cdp1802.idle=0;
	cdp1802.dma_cycles=0;
}

void cdp1802_exit(void)
{
	/* nothing to do yet */
}

unsigned cdp1802_get_context (void *dst)
{
	if( dst )
		*(CDP1802_Regs*)dst = cdp1802;
	return sizeof(CDP1802_Regs);
}

void cdp1802_set_context (void *src)
{
	if( src )
	{
		cdp1802 = *(CDP1802_Regs*)src;
		change_pc16(PC);
	}
}

unsigned cdp1802_get_reg (int regnum)
{
	switch( regnum )
	{
	case REG_PC: return PC;
	case REG_SP: return 0;
	case CDP1802_P: return cdp1802.p;
	case CDP1802_X: return cdp1802.x;
	case CDP1802_T: return cdp1802.t;
	case CDP1802_D: return cdp1802.d;
	case CDP1802_B: return cdp1802.b;
	case CDP1802_R0: return cdp1802.reg[0].w.l;
	case CDP1802_R1: return cdp1802.reg[1].w.l;
	case CDP1802_R2: return cdp1802.reg[2].w.l;
	case CDP1802_R3: return cdp1802.reg[3].w.l;
	case CDP1802_R4: return cdp1802.reg[4].w.l;
	case CDP1802_R5: return cdp1802.reg[5].w.l;
	case CDP1802_R6: return cdp1802.reg[6].w.l;
	case CDP1802_R7: return cdp1802.reg[7].w.l;
	case CDP1802_R8: return cdp1802.reg[8].w.l;
	case CDP1802_R9: return cdp1802.reg[9].w.l;
	case CDP1802_Ra: return cdp1802.reg[0xa].w.l;
	case CDP1802_Rb: return cdp1802.reg[0xb].w.l;
	case CDP1802_Rc: return cdp1802.reg[0xc].w.l;
	case CDP1802_Rd: return cdp1802.reg[0xd].w.l;
	case CDP1802_Re: return cdp1802.reg[0xe].w.l;
	case CDP1802_Rf: return cdp1802.reg[0xf].w.l;
	case CDP1802_DF: return cdp1802.df;
	case CDP1802_IE: return cdp1802.ie;
	case CDP1802_Q: return cdp1802.q;
	case REG_PREVIOUSPC: return cdp1802.oldpc;
	case CDP1802_IRQ_STATE: return cdp1802.irq_state;
	}
	return 0;
}

void cdp1802_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case REG_PC: PC=val;change_pc16(PC);break;
	case REG_SP: break;
	case CDP1802_P: cdp1802.p=val;break;
	case CDP1802_X: cdp1802.x=val;break;
	case CDP1802_T: cdp1802.t=val;break;
	case CDP1802_D: cdp1802.d=val;break;
	case CDP1802_B: cdp1802.b=val;break;
	case CDP1802_R0: cdp1802.reg[0].w.l=val;break;
	case CDP1802_R1: cdp1802.reg[1].w.l=val;break;
	case CDP1802_R2: cdp1802.reg[2].w.l=val;break;
	case CDP1802_R3: cdp1802.reg[3].w.l=val;break;
	case CDP1802_R4: cdp1802.reg[4].w.l=val;break;
	case CDP1802_R5: cdp1802.reg[5].w.l=val;break;
	case CDP1802_R6: cdp1802.reg[6].w.l=val;break;
	case CDP1802_R7: cdp1802.reg[7].w.l=val;break;
	case CDP1802_R8: cdp1802.reg[8].w.l=val;break;
	case CDP1802_R9: cdp1802.reg[9].w.l=val;break;
	case CDP1802_Ra: cdp1802.reg[0xa].w.l=val;break;
	case CDP1802_Rb: cdp1802.reg[0xb].w.l=val;break;
	case CDP1802_Rc: cdp1802.reg[0xc].w.l=val;break;
	case CDP1802_Rd: cdp1802.reg[0xd].w.l=val;break;
	case CDP1802_Re: cdp1802.reg[0xe].w.l=val;break;
	case CDP1802_Rf: cdp1802.reg[0xf].w.l=val;break;
	case CDP1802_DF: cdp1802.df=val;break;
	case CDP1802_IE: cdp1802.ie=val;break;
	case CDP1802_Q: cdp1802.q=val;break;
	case REG_PREVIOUSPC: cdp1802.oldpc=val;break;
	case CDP1802_IRQ_STATE: cdp1802.irq_state=val;break;
	}
}

#if 0
INLINE void cdp1802_take_irq(void)
{
}
#endif

int cdp1802_execute(int cycles)
{
	int ref=cycles;
	cdp1802_icount = cycles;

	change_pc16(PC);

	do
	{
		cdp1802.oldpc = PC;

		CALL_MAME_DEBUG;

		if (!cdp1802.idle) cdp1802_instruction();
		else cdp1802_icount--;

		if (cdp1802.config && cdp1802.config->dma) {
			cdp1802.config->dma(ref-cdp1802_icount);ref=cdp1802_icount;
		}
	} while (cdp1802_icount > 0);


	return cycles - cdp1802_icount;
}

void cdp1802_set_nmi_line(int state)
{
}

void cdp1802_set_irq_line(int irqline, int state)
{
	cdp1802.idle=0;
	if (cdp1802.ie) {
		cdp1802.ie=0;
		cdp1802.t=(cdp1802.x<<4)|cdp1802.p;
		cdp1802.p=1;
		cdp1802.x=2;
		change_pc16(PC);
	}
}

void cdp1802_set_irq_callback(int (*callback)(int))
{
}

void cdp1802_state_save(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file,"sc61860",cpu,"PC",&sc61860.pc,2);
	state_save_UINT16(file,"sc61860",cpu,"DP",&sc61860.dp,2);
	state_save_UINT8(file,"sc61860",cpu,"P",&sc61860.p,1);
	state_save_UINT8(file,"sc61860",cpu,"Q",&sc61860.q,1);
	state_save_UINT8(file,"sc61860",cpu,"R",&sc61860.r,1);
//	state_save_UINT8(file,"sc61860",cpu,"C",&sc61860.carry,1);
//	state_save_UINT8(file,"sc61860",cpu,"Z",&sc61860.zero,1);
#endif
}

void cdp1802_state_load(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file,"sc61860",cpu,"PC",&sc61860.pc,2);
	state_load_UINT16(file,"sc61860",cpu,"DP",&sc61860.dp,2);
	state_load_UINT8(file,"sc61860",cpu,"P",&sc61860.p,1);
	state_load_UINT8(file,"sc61860",cpu,"Q",&sc61860.q,1);
	state_load_UINT8(file,"sc61860",cpu,"R",&sc61860.r,1);
//	state_load_UINT8(file,"sc61860",cpu,"C",&sc61860.carry,1);
//	state_load_UINT8(file,"sc61860",cpu,"Z",&sc61860.zero,1);
#endif
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *cdp1802_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
    CDP1802_Regs *r = context;

	which = (which + 1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &cdp1802;

	switch( regnum )
	{
	case CPU_INFO_REG+CDP1802_R0: sprintf(buffer[which],"R0:%.4x",r->reg[0].w.l);break;
	case CPU_INFO_REG+CDP1802_R1: sprintf(buffer[which],"R1:%.4x",r->reg[1].w.l);break;
	case CPU_INFO_REG+CDP1802_R2: sprintf(buffer[which],"R2:%.4x",r->reg[2].w.l);break;
	case CPU_INFO_REG+CDP1802_R3: sprintf(buffer[which],"R3:%.4x",r->reg[3].w.l);break;
	case CPU_INFO_REG+CDP1802_R4: sprintf(buffer[which],"R4:%.4x",r->reg[4].w.l);break;
	case CPU_INFO_REG+CDP1802_R5: sprintf(buffer[which],"R5:%.4x",r->reg[5].w.l);break;
	case CPU_INFO_REG+CDP1802_R6: sprintf(buffer[which],"R6:%.4x",r->reg[6].w.l);break;
	case CPU_INFO_REG+CDP1802_R7: sprintf(buffer[which],"R7:%.4x",r->reg[7].w.l);break;
	case CPU_INFO_REG+CDP1802_R8: sprintf(buffer[which],"R8:%.4x",r->reg[8].w.l);break;
	case CPU_INFO_REG+CDP1802_R9: sprintf(buffer[which],"R9:%.4x",r->reg[9].w.l);break;
	case CPU_INFO_REG+CDP1802_Ra: sprintf(buffer[which],"Ra:%.4x",r->reg[0xa].w.l);break;
	case CPU_INFO_REG+CDP1802_Rb: sprintf(buffer[which],"Rb:%.4x",r->reg[0xb].w.l);break;
	case CPU_INFO_REG+CDP1802_Rc: sprintf(buffer[which],"Rc:%.4x",r->reg[0xc].w.l);break;
	case CPU_INFO_REG+CDP1802_Rd: sprintf(buffer[which],"Rd:%.4x",r->reg[0xd].w.l);break;
	case CPU_INFO_REG+CDP1802_Re: sprintf(buffer[which],"Re:%.4x",r->reg[0xe].w.l);break;
	case CPU_INFO_REG+CDP1802_Rf: sprintf(buffer[which],"Rf:%.4x",r->reg[0xf].w.l);break;
	case CPU_INFO_REG+CDP1802_P: sprintf(buffer[which],"P:%x",r->p);break;
	case CPU_INFO_REG+CDP1802_X: sprintf(buffer[which],"X:%x",r->x);break;
	case CPU_INFO_REG+CDP1802_D: sprintf(buffer[which],"D:%.2x",r->d);break;
	case CPU_INFO_REG+CDP1802_B: sprintf(buffer[which],"B:%.2x",r->b);break;
	case CPU_INFO_REG+CDP1802_T: sprintf(buffer[which],"T:%.2x",r->t);break;
	case CPU_INFO_REG+CDP1802_DF: sprintf(buffer[which],"DF:%x",r->df);break;
	case CPU_INFO_REG+CDP1802_IE: sprintf(buffer[which],"IE:%x",r->ie);break;
	case CPU_INFO_REG+CDP1802_Q: sprintf(buffer[which],"Q:%x",r->q);break;
	case CPU_INFO_FLAGS: sprintf(buffer[which], "%s%s%s", r->df?"DF":"..",
	r->ie ? "IE":"..", r->q?"Q":"."); break;
	case CPU_INFO_NAME: return "CDP1802";
	case CPU_INFO_FAMILY: return "CDP1802";
	case CPU_INFO_VERSION: return "1.0alpha";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (c) 2000 Peter Trauner, all rights reserved.";
	case CPU_INFO_REG_LAYOUT: return (const char*)cdp1802_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char*)cdp1802_win_layout;
	}
	return buffer[which];
}

#ifndef MAME_DEBUG
unsigned cdp1802_dasm(char *buffer, unsigned pc)
{
	sprintf( buffer, "$%X", cpu_readop(pc) );
	return 1;
}
#endif

void cdp1802_init(void){ return; }

