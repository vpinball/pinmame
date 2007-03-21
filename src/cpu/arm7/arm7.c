/*****************************************************************************
 *
 *	 arm7.c
 *	 Portable ARM7TDMI CPU Emulator
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

/******************************************************************************
 *  Notes:

    ** This is a plain vanilla implementation of an ARM7 cpu which incorporates my ARM7 core.
	   It can be used as is, or used to demonstrate how to utilize the arm7 core to create a cpu
	   that uses the core, since there are numerous different mcu packages that incorporate an arm7 core.

	   See the notes in the arm7core.c file itself regarding issues/limitations of the arm7 core.
	**
*****************************************************************************/
#include <stdio.h>
#include "arm7.h"
#include "state.h"
#include "mamedbg.h"
#include "arm7core.h"	//include arm7 core

/* Example for showing how Co-Proc functions work */
#define TEST_COPROC_FUNCS 1

/*prototypes*/
#if TEST_COPROC_FUNCS
static WRITE32_HANDLER(test_do_callback);
static READ32_HANDLER(test_rt_r_callback);
static WRITE32_HANDLER(test_rt_w_callback);
static void test_dt_r_callback (data32_t insn, data32_t* prn, data32_t (*read32)(int addr));
static void test_dt_w_callback (data32_t insn, data32_t* prn, void (*write32)(int addr, data32_t data));
#ifdef MAME_DEBUG
char *Spec_RT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
char *Spec_DT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
char *Spec_DO( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0);
#endif
#endif

/* Macros that can be re-defined for custom cpu implementations - The core expects these to be defined */
/* In this case, we are using the default arm7 handlers - but simply changes these to use your own */
#define READ8(addr)			arm7_cpu_read8(addr)
#define WRITE8(addr,data)	arm7_cpu_write8(addr,data)
#define READ16(addr)		arm7_cpu_read16(addr)
#define WRITE16(addr,data)	arm7_cpu_write16(addr,data)
#define READ32(addr)		arm7_cpu_read32(addr)
#define WRITE32(addr,data)	arm7_cpu_write32(addr,data)
#define PTR_READ32			&arm7_cpu_read32
#define PTR_WRITE32			&arm7_cpu_write32

/* Macros that need to be defined according to the cpu implementation specific need */
#define ARMREG(reg)			arm7.sArmRegister[reg]
#define ARM7				arm7
#define ARM7_ICOUNT			arm7_ICount
#define RESET_ICOUNT		ARM7_ICOUNT = cycles;
#define CAPTURE_NUM_CYCLES
#define BEFORE_OPCODE_EXEC_HOOK
#define AFTER_OPCODE_EXEC_HOOK

/* CPU Registers */
typedef struct
{
	ARM7CORE_REGS				//these must be included in your cpu specific register implementation
} ARM7_REGS;

static ARM7_REGS arm7;
int ARM7_ICOUNT;

/* include the arm7 core */
#include "arm7core.c"

/***************************************************************************
 * CPU SPECIFIC IMPLEMENTATIONS
 **************************************************************************/
void arm7_reset(void *param)
{
	//must call core reset
	arm7_core_reset(param);
}

void arm7_exit(void)
{
	/* nothing to do here */
}

int arm7_execute( int cycles )
{
/*include the arm7 core execute code*/
#include "arm7exec.c"
} /* arm7_execute */


unsigned arm7_get_context(void *dst)
{
	if( dst )
	{
		memcpy( dst, &ARM7, sizeof(ARM7) );
	}
	return sizeof(ARM7);
}

void arm7_set_context(void *src)
{
	if (src)
	{
		memcpy( &ARM7, src, sizeof(ARM7) );
	}
}

unsigned arm7_get_pc(void)
{
	return R15;
}

void arm7_set_pc(unsigned val)
{
	R15 = val;
}

unsigned arm7_get_sp(void)
{
	return GET_REGISTER(13);
}

void arm7_set_sp(unsigned val)
{
	SET_REGISTER(13,val);
}

unsigned arm7_get_reg(int regnum)
{
	switch( regnum )
	{
	case ARM732_R0: return ARMREG(0);
	case ARM732_R1: return ARMREG(1);
	case ARM732_R2: return ARMREG(2);
	case ARM732_R3: return ARMREG(3);
	case ARM732_R4: return ARMREG(4);
	case ARM732_R5: return ARMREG(5);
	case ARM732_R6: return ARMREG(6);
	case ARM732_R7: return ARMREG(7);
	case ARM732_R8: return ARMREG(8);
	case ARM732_R9: return ARMREG(9);
	case ARM732_R10: return ARMREG(10);
	case ARM732_R11: return ARMREG(11);
	case ARM732_R12: return ARMREG(12);
	case ARM732_R13: return ARMREG(13);
	case ARM732_R14: return ARMREG(14);
	case ARM732_R15: return ARMREG(15);
	case ARM732_CPSR: return ARMREG(eCPSR);

	case ARM732_FR8: return	ARMREG(eR8_FIQ);
	case ARM732_FR9:	return ARMREG(eR9_FIQ);
	case ARM732_FR10: return ARMREG(eR10_FIQ);
	case ARM732_FR11: return ARMREG(eR11_FIQ);
	case ARM732_FR12: return ARMREG(eR12_FIQ);
	case ARM732_FR13: return ARMREG(eR13_FIQ);
	case ARM732_FR14: return ARMREG(eR14_FIQ);
    case ARM732_FSPSR: return ARMREG(eSPSR_FIQ);
	case ARM732_IR13: return ARMREG(eR13_IRQ);
	case ARM732_IR14: return ARMREG(eR14_IRQ);
    case ARM732_ISPSR: return ARMREG(eSPSR_IRQ);
	case ARM732_SR13: return ARMREG(eR13_SVC);
	case ARM732_SR14: return ARMREG(eR14_SVC);
	case ARM732_SSPSR: return ARMREG(eSPSR_SVC);
	case REG_PC: return ARMREG(15);
	}

	return 0;
}

void arm7_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
	case ARM732_R0: ARMREG(0)= val; break;
	case ARM732_R1: ARMREG( 1)= val; break;
	case ARM732_R2: ARMREG( 2)= val; break;
	case ARM732_R3: ARMREG( 3)= val; break;
	case ARM732_R4: ARMREG( 4)= val; break;
	case ARM732_R5: ARMREG( 5)= val; break;
	case ARM732_R6: ARMREG( 6)= val; break;
	case ARM732_R7: ARMREG( 7)= val; break;
	case ARM732_R8: ARMREG( 8)= val; break;
	case ARM732_R9: ARMREG( 9)= val; break;
	case ARM732_R10: ARMREG(10)= val; break;
	case ARM732_R11: ARMREG(11)= val; break;
	case ARM732_R12: ARMREG(12)= val; break;
	case ARM732_R13: ARMREG(13)= val; break;
	case ARM732_R14: ARMREG(14)= val; break;
	case ARM732_R15: ARMREG(15)= val; break;
	case ARM732_CPSR: SET_CPSR(val); break;
	case ARM732_FR8: ARMREG(eR8_FIQ) = val; break;
	case ARM732_FR9: ARMREG(eR9_FIQ) = val; break;
	case ARM732_FR10: ARMREG(eR10_FIQ) = val; break;
	case ARM732_FR11: ARMREG(eR11_FIQ) = val; break;
	case ARM732_FR12: ARMREG(eR12_FIQ) = val; break;
	case ARM732_FR13: ARMREG(eR13_FIQ) = val; break;
	case ARM732_FR14: ARMREG(eR14_FIQ) = val; break;
	case ARM732_FSPSR: ARMREG(eSPSR_FIQ) = val; break;
	case ARM732_IR13: ARMREG(eR13_IRQ) = val; break;
	case ARM732_IR14: ARMREG(eR14_IRQ) = val; break;
	case ARM732_ISPSR: ARMREG(eSPSR_IRQ) = val; break;
	case ARM732_SR13: ARMREG(eR13_SVC) = val; break;
	case ARM732_SR14: ARMREG(eR14_SVC) = val; break;
	case ARM732_SSPSR: ARMREG(eSPSR_SVC)= val; break;
	case ARM732_AR13: ARMREG(eR13_ABT) = val; break;
	case ARM732_AR14: ARMREG(eR14_ABT) = val; break;
	case ARM732_ASPSR: ARMREG(eSPSR_ABT) = val; break;
	case ARM732_UR13: ARMREG(eR13_UND) = val; break;
	case ARM732_UR14: ARMREG(eR14_UND) = val; break;
	case ARM732_USPSR: ARMREG(eSPSR_UND) = val; break;
	}
}

void arm7_set_nmi_line(int state)
{
}

void arm7_set_irq_line(int irqline, int state)
{
	//must call core
	arm7_core_set_irq_line(irqline,state);
}

void arm7_set_irq_callback(int (*callback)(int irqline))
{
}

static const data8_t arm7_reg_layout[] =
{
	-1,
	ARM732_R0,  ARM732_IR13, -1,
	ARM732_R1,  ARM732_IR14, -1,
	ARM732_R2,  ARM732_ISPSR, -1,
	ARM732_R3,  -1,
	ARM732_R4,  ARM732_FR8,  -1,
	ARM732_R5,  ARM732_FR9,  -1,
	ARM732_R6,  ARM732_FR10, -1,
	ARM732_R7,  ARM732_FR11, -1,
	ARM732_R8,  ARM732_FR12, -1,
	ARM732_R9,  ARM732_FR13, -1,
	ARM732_R10, ARM732_FR14, -1,
	ARM732_R11, ARM732_FSPSR, -1,
	ARM732_R12, -1,
	ARM732_R13, ARM732_AR13, -1,
	ARM732_R14, ARM732_AR14, -1,
	ARM732_R15, ARM732_ASPSR, -1,
	-1,
	ARM732_SR13, ARM732_UR13, -1,
	ARM732_SR14, ARM732_UR14, -1,
	ARM732_SSPSR, ARM732_USPSR, 0
};


static const UINT8 arm7_win_layout[] = {
	 0, 0,30,17,	/* register window (top rows) */
	31, 0,49,17,	/* disassembler window (left colums) */
	 0,18,48, 4,	/* memory #1 window (right, upper middle) */
	49,18,31, 4,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

const char *arm7_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;

	ARM7_REGS *pRegs = context;
	if( !context )
		pRegs = &ARM7;

	which = (which + 1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
	case CPU_INFO_REG + ARM732_R0: sprintf( buffer[which], "R0  :%08x", pRegs->sArmRegister[ 0] );  break;
	case CPU_INFO_REG + ARM732_R1: sprintf( buffer[which], "R1  :%08x", pRegs->sArmRegister[ 1] );  break;
	case CPU_INFO_REG + ARM732_R2: sprintf( buffer[which], "R2  :%08x", pRegs->sArmRegister[ 2] );  break;
	case CPU_INFO_REG + ARM732_R3: sprintf( buffer[which], "R3  :%08x", pRegs->sArmRegister[ 3] );  break;
	case CPU_INFO_REG + ARM732_R4: sprintf( buffer[which], "R4  :%08x", pRegs->sArmRegister[ 4] );  break;
	case CPU_INFO_REG + ARM732_R5: sprintf( buffer[which], "R5  :%08x", pRegs->sArmRegister[ 5] );  break;
	case CPU_INFO_REG + ARM732_R6: sprintf( buffer[which], "R6  :%08x", pRegs->sArmRegister[ 6] );  break;
	case CPU_INFO_REG + ARM732_R7: sprintf( buffer[which], "R7  :%08x", pRegs->sArmRegister[ 7] );  break;
	case CPU_INFO_REG + ARM732_R8: sprintf( buffer[which], "R8  :%08x", pRegs->sArmRegister[ 8] );  break;
	case CPU_INFO_REG + ARM732_R9: sprintf( buffer[which], "R9  :%08x", pRegs->sArmRegister[ 9] );  break;
	case CPU_INFO_REG + ARM732_R10:sprintf( buffer[which], "R10 :%08x", pRegs->sArmRegister[10] );  break;
	case CPU_INFO_REG + ARM732_R11:sprintf( buffer[which], "R11 :%08x", pRegs->sArmRegister[11] );  break;
	case CPU_INFO_REG + ARM732_R12:sprintf( buffer[which], "R12 :%08x", pRegs->sArmRegister[12] );  break;
	case CPU_INFO_REG + ARM732_R13:sprintf( buffer[which], "R13 :%08x", pRegs->sArmRegister[13] );  break;
	case CPU_INFO_REG + ARM732_R14:sprintf( buffer[which], "R14 :%08x", pRegs->sArmRegister[14] );  break;
	case CPU_INFO_REG + ARM732_R15:sprintf( buffer[which], "R15 :%08x", pRegs->sArmRegister[15] );  break;
	case CPU_INFO_REG + ARM732_CPSR:sprintf( buffer[which], "R16 :%08x", pRegs->sArmRegister[eCPSR] );  break;
	case CPU_INFO_REG + ARM732_FR8: sprintf( buffer[which], "FR8 :%08x", pRegs->sArmRegister[eR8_FIQ] );  break;
	case CPU_INFO_REG + ARM732_FR9: sprintf( buffer[which], "FR9 :%08x", pRegs->sArmRegister[eR9_FIQ] );  break;
	case CPU_INFO_REG + ARM732_FR10:sprintf( buffer[which], "FR10:%08x", pRegs->sArmRegister[eR10_FIQ] );  break;
	case CPU_INFO_REG + ARM732_FR11:sprintf( buffer[which], "FR11:%08x", pRegs->sArmRegister[eR11_FIQ]);  break;
	case CPU_INFO_REG + ARM732_FR12:sprintf( buffer[which], "FR12:%08x", pRegs->sArmRegister[eR12_FIQ] );  break;
	case CPU_INFO_REG + ARM732_FR13:sprintf( buffer[which], "FR13:%08x", pRegs->sArmRegister[eR13_FIQ] );  break;
	case CPU_INFO_REG + ARM732_FR14:sprintf( buffer[which], "FR14:%08x", pRegs->sArmRegister[eR14_FIQ] );  break;
    case CPU_INFO_REG + ARM732_FSPSR:sprintf( buffer[which], "FR16:%08x", pRegs->sArmRegister[eSPSR_FIQ] );  break;
	case CPU_INFO_REG + ARM732_IR13:sprintf( buffer[which], "IR13:%08x", pRegs->sArmRegister[eR13_IRQ] );  break;
	case CPU_INFO_REG + ARM732_IR14:sprintf( buffer[which], "IR14:%08x", pRegs->sArmRegister[eR14_IRQ] );  break;
    case CPU_INFO_REG + ARM732_ISPSR:sprintf( buffer[which], "IR16:%08x", pRegs->sArmRegister[eSPSR_IRQ] );  break;
	case CPU_INFO_REG + ARM732_SR13:sprintf( buffer[which], "SR13:%08x", pRegs->sArmRegister[eR13_SVC] );  break;
	case CPU_INFO_REG + ARM732_SR14:sprintf( buffer[which], "SR14:%08x", pRegs->sArmRegister[eR14_SVC] );  break;
	case CPU_INFO_REG + ARM732_SSPSR:sprintf( buffer[which], "SR16:%08x", pRegs->sArmRegister[eSPSR_SVC] );  break;
	case CPU_INFO_REG + ARM732_AR13:sprintf( buffer[which], "AR13:%08x", pRegs->sArmRegister[eR13_ABT] );  break;
	case CPU_INFO_REG + ARM732_AR14:sprintf( buffer[which], "AR14:%08x", pRegs->sArmRegister[eR14_ABT] );  break;
	case CPU_INFO_REG + ARM732_ASPSR:sprintf( buffer[which], "AR16:%08x", pRegs->sArmRegister[eSPSR_ABT] );  break;
	case CPU_INFO_REG + ARM732_UR13:sprintf( buffer[which], "UR13:%08x", pRegs->sArmRegister[eR13_UND] );  break;
	case CPU_INFO_REG + ARM732_UR14:sprintf( buffer[which], "UR14:%08x", pRegs->sArmRegister[eR14_UND] );  break;
	case CPU_INFO_REG + ARM732_USPSR:sprintf( buffer[which], "UR16:%08x", pRegs->sArmRegister[eSPSR_UND] );  break;

	case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c%c%c%c%c%c%c %s (%08x)",
			(pRegs->sArmRegister[eCPSR] & N_MASK) ? 'N' : '-',
			(pRegs->sArmRegister[eCPSR] & Z_MASK) ? 'Z' : '-',
			(pRegs->sArmRegister[eCPSR] & C_MASK) ? 'C' : '-',
			(pRegs->sArmRegister[eCPSR] & V_MASK) ? 'V' : '-',
			(pRegs->sArmRegister[eCPSR] & I_MASK) ? 'I' : '-',
			(pRegs->sArmRegister[eCPSR] & F_MASK) ? 'F' : '-',
			(pRegs->sArmRegister[eCPSR] & T_MASK) ? 'T' : '-',
			GetModeText(pRegs->sArmRegister[eCPSR]),
			pRegs->sArmRegister[eCPSR]);
		break;
	case CPU_INFO_NAME: 		return "ARM7";
	case CPU_INFO_FAMILY:		return "Acorn Risc Machine";
	case CPU_INFO_VERSION:		return "1.2";
	case CPU_INFO_FILE: 		return __FILE__;
	case CPU_INFO_CREDITS:		return "Copyright 2004 Steve Ellenoff, sellenoff@hotmail.com";
	case CPU_INFO_REG_LAYOUT:	return (const char*)arm7_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char*)arm7_win_layout;
	}

	return buffer[which];
}

unsigned arm7_dasm(char *buffer, unsigned int pc)
{
#ifdef MAME_DEBUG
	arm7_disasm( buffer, pc, READ32(pc)); //&ADDRESS_MASK) );
	return 4;
#else
	sprintf(buffer, "$%08x", READ32(pc));
	return 4;
#endif
}

void arm7_init(void)
{
	//must call core 
	arm7_core_init("arm7");

#if TEST_COPROC_FUNCS
	//setup co-proc callbacks example
	arm7_coproc_do_callback = test_do_callback;
	arm7_coproc_rt_r_callback = test_rt_r_callback;
	arm7_coproc_rt_w_callback = test_rt_w_callback;
	arm7_coproc_dt_r_callback = test_dt_r_callback;
	arm7_coproc_dt_w_callback = test_dt_w_callback;
#ifdef MAME_DEBUG
	//setup dasm callbacks - direct method example
	arm7_dasm_cop_dt_callback = Spec_DT;
	arm7_dasm_cop_rt_callback = Spec_RT;
	arm7_dasm_cop_do_callback = Spec_DO;
#endif
#endif

	return;
}


//*TEST COPROC CALLBACK HANDLERS - Used for example on how to implement only *//
#if TEST_COPROC_FUNCS

static WRITE32_HANDLER(test_do_callback)
{
	LOG(("test_do_callback opcode=%x, =%x\n",offset,data));
}
static READ32_HANDLER(test_rt_r_callback)
{
	data32_t data=0;
	LOG(("test_rt_r_callback opcode=%x\n",offset));
	return data;
}
static WRITE32_HANDLER(test_rt_w_callback)
{
	LOG(("test_rt_w_callback opcode=%x, data from ARM7 register=%x\n",offset,data));
}
static void test_dt_r_callback (data32_t insn, data32_t* prn, data32_t (*read32)(int addr))
{
	LOG(("test_dt_r_callback: insn = %x\n",insn));
}
static void test_dt_w_callback (data32_t insn, data32_t* prn, void (*write32)(int addr, data32_t data))
{
	LOG(("test_dt_w_callback: opcode = %x\n",insn));
}

//Custom Co-proc DASM handlers
#ifdef MAME_DEBUG
char *Spec_RT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
	pBuf += sprintf( pBuf, "SPECRT");
	return pBuf;
}
char *Spec_DT( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
	pBuf += sprintf( pBuf, "SPECDT");
	return pBuf;
}
char *Spec_DO( char *pBuf, data32_t opcode, char *pConditionCode, char *pBuf0)
{
	pBuf += sprintf( pBuf, "SPECDO");
	return pBuf;
}
#endif
#endif

