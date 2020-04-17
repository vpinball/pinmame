/*****************************************************************************
 *
 *	 i8085.c
 *	 Portable I8085A emulator V1.2
 *
 *	 Copyright (c) 1999 Juergen Buchmueller, all rights reserved.
 *	 Partially based on information out of Z80Em by Marcel De Kogel
 *
 * changes in V1.3
 *   - Added undocumented opcodes for the 8085A, based on a german
 *     book about microcomputers: "Mikrocomputertechnik mit dem
 *     Prozessor 8085A".
 *   - This book also suggest that INX/DCX should modify the X flag bit
 *     for a LSB to MSB carry and
 *   - that jumps take 10 T-states only when they're executed, 7 when
 *     they're skipped.
 *     Thanks for the info and a copy of the tables go to Timo Sachsenberg
 *     <timo.sachsenberg@student.uni-tuebingen.de>
 * changes in V1.2
 *	 - corrected cycle counts for these classes of opcodes
 *	   Thanks go to Jim Battle <frustum@pacbell.bet>
 *
 *					808x	 Z80
 *	   DEC A		   5	   4	\
 *	   INC A		   5	   4	 \
 *	   LD A,B		   5	   4	  >-- Z80 is faster
 *	   JP (HL)		   5	   4	 /
 *	   CALL cc,nnnn: 11/17	 10/17	/
 *
 *	   INC HL		   5	   6	\
 *	   DEC HL		   5	   6	 \
 *	   LD SP,HL 	   5	   6	  \
 *	   ADD HL,BC	  10	  11	   \
 *	   INC (HL) 	  10	  11		>-- 8080 is faster
 *	   DEC (HL) 	  10	  11	   /
 *	   IN A,(#) 	  10	  11	  /
 *	   OUT (#),A	  10	  11	 /
 *	   EX (SP),HL	  18	  19	/
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *
 * Revisions:
 *
 * xx-xx-2002 Acho A. Tang
 *
 * - 8085 emulation was in fact never used. It's been treated as a plain 8080.
 * - protected IRQ0 vector from being overwritten
 * - modified interrupt handler to properly process 8085-specific IRQ's
 * - corrected interrupt masking, RIM and SIM behaviors according to Intel's documentation
 *
 * 20-Jul-2002 Krzysztof Strzecha
 *
 * - SBB r instructions should affect parity flag.
 *   Fixed only for non x86 asm version (#define i8080_EXACT 1).
 *   There are probably more opcodes which should affect this flag, but don't.
 * - JPO nnnn and JPE nnnn opcodes in disassembler were misplaced. Fixed.
 * - Undocumented i8080 opcodes added:
 *   08h, 10h, 18h, 20h, 28h, 30h, 38h  -  NOP
 *   0CBh                               -  JMP
 *   0D9h                               -  RET
 *   0DDh, 0EDh, 0FDh                   -  CALL
 *   Thanks for the info go to Anton V. Ignatichev.
 *
 * 08-Dec-2002 Krzysztof Strzecha
 *
 * - ADC r instructions should affect parity flag.
 *   Fixed only for non x86 asm version (#define i8080_EXACT 1).
 *   There are probably more opcodes which should affect this flag, but don't.
 *
 * 05-Sep-2003 Krzysztof Strzecha
 *
 * - INR r, DCR r, ADD r, SUB r, CMP r instructions should affect parity flag.
 *   Fixed only for non x86 asm version (#define i8080_EXACT 1).
 * 
 * 23-Dec-2006 Tomasz Slanina
 * - SIM fixed
 *
 * 28-Jan-2007 Zsolt Vasvari
 * - Removed archaic i8080_EXACT flag.
 *
 * 08-June-2008 Miodrag Milanovic
 * - Flag setting fix for some instructions and cycle count update
 *
 * August 2009, hap
 * - removed DAA table
 * - fixed accidental double memory reads due to macro overuse
 * - fixed cycle deduction on unconditional CALL / RET
 * - added cycle tables and cleaned up big switch source layout (1 tab = 4 spaces)
 * - removed HLT cycle eating (earlier, HLT after EI could theoretically fail)
 * - fixed parity flag on add/sub/cmp
 * - renamed temp register XX to official name WZ
 * - renamed flags from Z80 style S Z Y H X V N C  to  S Z X5 H X3 P V C, and
 *   fixed X5 / V flags where accidentally broken due to flag names confusion
 *
 * 21-Aug-2009, Curt Coder
 * - added 8080A variant
 * - refactored callbacks to use devcb
 *
 * October 2012, hap
 * - fixed H flag on subtraction opcodes
 * - on 8080, don't push the unsupported flags(X5, X3, V) to stack
 * - 8080 passes on 8080/8085 CPU Exerciser, 8085 errors only on the DAA test
 *   (ref: http://www.idb.me.uk/sunhillow/8080.html - tests only 8080 opcodes)
 *
 *****************************************************************************/

/*int survival_prot = 0; */

#define VERBOSE 0

#include "driver.h"
#include "state.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "i8085.h"
#include "i8085cpu.h"

#if VERBOSE
#include <stdio.h>
#include "driver.h"
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#define I8085_INTR      0xff

/* Layout of the registers in the debugger */
static UINT8 i8085_reg_layout[] = {
	I8085_PC,I8085_SP,I8085_AF,I8085_BC,I8085_DE,I8085_HL, 0xFF,
	I8085_HALT,I8085_IM,I8085_IREQ,I8085_ISRV,I8085_VECTOR, 0xFF,
	I8085_TRAP_STATE,I8085_INTR_STATE,I8085_RST55_STATE,I8085_RST65_STATE,I8085_RST75_STATE,
	0 };

/* Layout of the debugger windows x,y,w,h */
static UINT8 i8085_win_layout[] = {
	25, 0,55, 3,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 4,55, 9,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};


/* cycles lookup */
const UINT8 i8085_lut_cycles_8080[256]={
/*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  */
/* 0 */ 4, 10,7, 5, 5, 5, 7, 4, 4, 10,7, 5, 5, 5, 7, 4,
/* 1 */ 4, 10,7, 5, 5, 5, 7, 4, 4, 10,7, 5, 5, 5, 7, 4,
/* 2 */ 4, 10,16,5, 5, 5, 7, 4, 4, 10,16,5, 5, 5, 7, 4,
/* 3 */ 4, 10,13,5, 10,10,10,4, 4, 10,13,5, 5, 5, 7, 4,
/* 4 */ 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
/* 5 */ 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
/* 6 */ 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5,
/* 7 */ 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 7, 5,
/* 8 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* 9 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* A */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* B */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* C */ 5, 10,10,10,11,11,7, 11,5, 10,10,10,11,11,7, 11,
/* D */ 5, 10,10,10,11,11,7, 11,5, 10,10,10,11,11,7, 11,
/* E */ 5, 10,10,18,11,11,7, 11,5, 5, 10,5, 11,11,7, 11,
/* F */ 5, 10,10,4, 11,11,7, 11,5, 5, 10,4, 11,11,7, 11 };
const UINT8 i8085_lut_cycles_8085[256]={
/*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  */
/* 0 */ 4, 10,7, 6, 4, 4, 7, 4, 10,10,7, 6, 4, 4, 7, 4,
/* 1 */ 7, 10,7, 6, 4, 4, 7, 4, 10,10,7, 6, 4, 4, 7, 4,
/* 2 */ 7, 10,16,6, 4, 4, 7, 4, 10,10,16,6, 4, 4, 7, 4,
/* 3 */ 7, 10,13,6, 10,10,10,4, 10,10,13,6, 4, 4, 7, 4,
/* 4 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* 5 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* 6 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* 7 */ 7, 7, 7, 7, 7, 7, 5, 7, 4, 4, 4, 4, 4, 4, 7, 4,
/* 8 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* 9 */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* A */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* B */ 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
/* C */ 6, 10,10,10,11,12,7, 12,6, 10,10,12,11,11,7, 12,
/* D */ 6, 10,10,10,11,12,7, 12,6, 10,10,10,11,10,7, 12,
/* E */ 6, 10,10,16,11,12,7, 12,6, 6, 10,5, 11,10,7, 12,
/* F */ 6, 10,10,4, 11,12,7, 12,6, 6, 10,4, 11,10,7, 12 };

/* special cases (partially taken care of elsewhere):
               base c    taken?   not taken?
M_RET  8080    5         +6(11)   -0            (conditional)
M_RET  8085    6         +6(12)   -0            (conditional)
M_JMP  8080    10        +0       -0
M_JMP  8085    10        +0       -3(7)
M_CALL 8080    11        +6(17)   -0
M_CALL 8085    11        +7(18)   -2(9)

*/


typedef struct {
	int 	cputype;	/* 0 8080, 1 8085A */
	PAIR	PC,SP,AF,BC,DE,HL,XX;
	UINT8	HALT;
	UINT8	IM; 		/* interrupt mask */
	UINT8	IREQ;		/* requested interrupts */
	UINT8	ISRV;		/* serviced interrupt */
	UINT32	INTR;		/* vector for INTR */
	UINT32	IRQ2;		/* scheduled interrupt address */
	UINT32	IRQ1;		/* executed interrupt address */
	INT8	nmi_state;
	INT8	irq_state[4];
	INT8	filler; /* align on dword boundary */
	int 	(*irq_callback)(int);
	void	(*sod_callback)(int state);
}	i8085_Regs;

int i8085_ICount = 0;

static i8085_Regs I;
static UINT8 ZS[256];
static UINT8 ZSP[256];
static UINT8 RIM_IEN = 0; //AT: IEN status latch used by the RIM instruction
static UINT8 ROP(void)
{
	return cpu_readop(I.PC.w.l++);
}

static UINT8 ARG(void)
{
	return cpu_readop_arg(I.PC.w.l++);
}

static UINT16 ARG16(void)
{
	UINT16 w;
	w  = cpu_readop_arg(I.PC.d);
	I.PC.w.l++;
	w += cpu_readop_arg(I.PC.d) << 8;
	I.PC.w.l++;
	return w;
}

static UINT8 RM(UINT32 a)
{
	return cpu_readmem16(a);
}

static void WM(UINT32 a, UINT8 v)
{
	cpu_writemem16(a, v);
}

#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static	void illegal(void)
{
#if VERBOSE
	UINT16 pc = I.PC.w.l - 1;
	LOG(("i8085 illegal instruction %04X $%02X\n", pc, cpu_readop(pc)));
#endif
}
#endif

INLINE void execute_one(int opcode)
{
	i8085_ICount -= I.cputype ? i8085_lut_cycles_8085[opcode] : i8085_lut_cycles_8080[opcode];

	switch (opcode)
	{
		case 0x00:	/* NOP	*/
			/* no op */
			break;
		case 0x01:      /* LXI	B,nnnn */
			I.BC.w.l = ARG16();
			break;
		case 0x02: 	/* STAX B */
			WM(I.BC.d, I.AF.b.h);
			break;
		case 0x03: 	/* INX	B */
			I.BC.w.l++;
			if( I.cputype ) { if (I.BC.w.l == 0x0000) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x04: 	/* INR	B */
			M_INR(I.BC.b.h);
			break;
		case 0x05: 	/* DCR	B */
			M_DCR(I.BC.b.h);
			break;
		case 0x06: 	/* MVI	B,nn */
			M_MVI(I.BC.b.h);
			break;
		case 0x07: 	/* RLC	*/
			M_RLC;
			break;
		case 0x08:
			if( I.cputype ) {
						/* DSUB */
				M_DSUB();
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x09: 	/* DAD	B */
			M_DAD(BC);
			break;
		case 0x0a: 	/* LDAX B */
			I.AF.b.h = RM(I.BC.d);
			break;
		case 0x0b: 	/* DCX	B */
			I.BC.w.l--;
			if( I.cputype ) { if (I.BC.w.l == 0xffff) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x0c: 	/* INR	C */
			M_INR(I.BC.b.l);
			break;
		case 0x0d: 	/* DCR	C */
			M_DCR(I.BC.b.l);
			break;
		case 0x0e: 	/* MVI	C,nn */
			M_MVI(I.BC.b.l);
			break;
		case 0x0f: 	/* RRC	*/
			M_RRC;
			break;

		case 0x10:
			if( I.cputype ) {
						/* ASRH */
				I.AF.b.l = (I.AF.b.l & ~CF) | (I.HL.b.l & CF);
				I.HL.w.l = (I.HL.w.l >> 1);
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x11: 	/* LXI	D,nnnn */
			I.DE.w.l = ARG16();
			break;
		case 0x12: 	/* STAX D */
			WM(I.DE.d, I.AF.b.h);
			break;
		case 0x13: 	/* INX	D */
			I.DE.w.l++;
			if( I.cputype ) { if (I.DE.w.l == 0x0000) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x14: 	/* INR	D */
			M_INR(I.DE.b.h);
			break;
		case 0x15: 	/* DCR	D */
			M_DCR(I.DE.b.h);
			break;
		case 0x16: 	/* MVI	D,nn */
			M_MVI(I.DE.b.h);
			break;
		case 0x17: 	/* RAL	*/
			M_RAL;
			break;

		case 0x18:
			if( I.cputype ) {
						/* RLDE */
				I.AF.b.l = (I.AF.b.l & ~(CF | VF)) | (I.DE.b.h >> 7);
				I.DE.w.l = (I.DE.w.l << 1) | (I.DE.w.l >> 15);
				if (0 != (((I.DE.w.l >> 15) ^ I.AF.b.l) & CF))
					I.AF.b.l |= VF;
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x19: 	/* DAD	D */
			M_DAD(DE);
			break;
		case 0x1a: 	/* LDAX D */
			I.AF.b.h = RM(I.DE.d);
			break;
		case 0x1b: 	/* DCX	D */
			I.DE.w.l--;
			if( I.cputype ) { if (I.DE.w.l == 0xffff) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x1c: 	/* INR	E */
			M_INR(I.DE.b.l);
			break;
		case 0x1d: 	/* DCR	E */
			M_DCR(I.DE.b.l);
			break;
		case 0x1e: 	/* MVI	E,nn */
			M_MVI(I.DE.b.l);
			break;
		case 0x1f: 	/* RAR	*/
			M_RAR;
			break;

		case 0x20:
			if( I.cputype ) { //!! misses fixes from newer MAME
						/* RIM	*/
				I.AF.b.h = I.IM;
				I.AF.b.h |= RIM_IEN; RIM_IEN = 0; //AT: read and clear IEN status latch
/*				survival_prot ^= 0x01; */
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x21: 	/* LXI	H,nnnn */
			I.HL.w.l = ARG16();
			break;
		case 0x22: 	/* SHLD nnnn */
			I.XX.w.l = ARG16();
			WM(I.XX.d, I.HL.b.l);
			I.XX.w.l++;
			WM(I.XX.d, I.HL.b.h);
			break;
		case 0x23: 	/* INX	H */
			I.HL.w.l++;
			if( I.cputype ) { if (I.HL.w.l == 0x0000) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x24: 	/* INR	H */
			M_INR(I.HL.b.h);
			break;
		case 0x25: 	/* DCR	H */
			M_DCR(I.HL.b.h);
			break;
		case 0x26: 	/* MVI	H,nn */
			M_MVI(I.HL.b.h);
			break;
		case 0x27: 	/* DAA	*/
			I.XX.b.h = I.AF.b.h;
					if (I.cputype && I.AF.b.l&VF) {
						if ((I.AF.b.l&HF) | ((I.AF.b.h&0xf)>9)) I.XX.b.h-=6;
						if ((I.AF.b.l&CF) | (I.AF.b.h>0x99)) I.XX.b.h-=0x60;
					}
					else {
						if ((I.AF.b.l&HF) | ((I.AF.b.h&0xf)>9)) I.XX.b.h+=6;
						if ((I.AF.b.l&CF) | (I.AF.b.h>0x99)) I.XX.b.h+=0x60;
					}

					I.AF.b.l=(I.AF.b.l&3) | (I.AF.b.h&0x28) | (I.AF.b.h>0x99) | ((I.AF.b.h^I.XX.b.h)&0x10) | ZSP[I.XX.b.h];
					I.AF.b.h=I.XX.b.h;
			break;

		case 0x28:
			if( I.cputype ) {
						/* LDEH nn */
				I.XX.d = ARG();
				I.DE.d = (I.HL.d + I.XX.d) & 0xffff;
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x29: 	/* DAD	H */
			M_DAD(HL);
			break;
		case 0x2a: 	/* LHLD nnnn */
			I.XX.d = ARG16();
			I.HL.b.l = RM(I.XX.d);
			I.XX.w.l++;
			I.HL.b.h = RM(I.XX.d);
			break;
		case 0x2b: 	/* DCX	H */
			I.HL.w.l--;
			if( I.cputype ) { if (I.HL.w.l == 0xffff) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x2c: 	/* INR	L */
			M_INR(I.HL.b.l);
			break;
		case 0x2d: 	/* DCR	L */
			M_DCR(I.HL.b.l);
			break;
		case 0x2e: 	/* MVI	L,nn */
			M_MVI(I.HL.b.l);
			break;
		case 0x2f: 	/* CMA	*/
			I.AF.b.h ^= 0xff;
			if( I.cputype ) { I.AF.b.l |= HF | VF; }
			break;

		case 0x30: //!! misses new port from MAME
			if( I.cputype ) {
						/* SIM	*/
				if (I.AF.b.h & 0x40) //SOE - only when bit 0x40 is set!
				{
					I.IM &=~IM_SOD;
					if (I.AF.b.h & 0x80) I.IM |= IM_SOD; //is it needed ?
					if (I.sod_callback) (*I.sod_callback)(I.AF.b.h >> 7); //SOD - data = bit 0x80
				}
//AT
				//I.IM &= (IM_SID + IM_IE + IM_TRAP);
				//I.IM |= (I.AF.b.h & ~(IM_SID + IM_SOD + IM_IE + IM_TRAP));

				// overwrite RST5.5-7.5 interrupt masks only when bit 0x08 of the accumulator is set
				if (I.AF.b.h & 0x08)
					I.IM = (I.IM & ~(IM_M55+IM_M65+IM_M75)) | (I.AF.b.h & (IM_M55+IM_M65+IM_M75));
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x31: 	/* LXI SP,nnnn */
			I.SP.w.l = ARG16();
			break;
		case 0x32: 	/* STAX nnnn */
			I.XX.d = ARG16();
			WM(I.XX.d, I.AF.b.h);
			break;
		case 0x33: 	/* INX	SP */
			I.SP.w.l++;
			if( I.cputype ) { if (I.SP.w.l == 0x0000) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x34: 	/* INR	M */
			I.XX.b.l = RM(I.HL.d);
			M_INR(I.XX.b.l);
			WM(I.HL.d, I.XX.b.l);
			break;
		case 0x35: 	/* DCR	M */
			I.XX.b.l = RM(I.HL.d);
			M_DCR(I.XX.b.l);
			WM(I.HL.d, I.XX.b.l);
			break;
		case 0x36: 	/* MVI	M,nn */
			I.XX.b.l = ARG();
			WM(I.HL.d, I.XX.b.l);
			break;
		case 0x37: 	/* STC	*/
			I.AF.b.l = (I.AF.b.l & 0xfe) | CF;
			break;

		case 0x38:
			if( I.cputype ) {
						/* LDES nn */
				I.XX.d = ARG();
				I.DE.d = (I.SP.d + I.XX.d) & 0xffff;
			} else {
						/* NOP undocumented */
			}
			break;
		case 0x39: 	/* DAD SP */
			M_DAD(SP);
			break;
		case 0x3a: 	/* LDAX nnnn */
			I.XX.d = ARG16();
			I.AF.b.h = RM(I.XX.d);
			break;
		case 0x3b: 	/* DCX	SP */
			I.SP.w.l--;
			if( I.cputype ) { if (I.SP.w.l == 0xffff) I.AF.b.l |= X5F; else I.AF.b.l &= ~X5F; }
			break;
		case 0x3c: 	/* INR	A */
			M_INR(I.AF.b.h);
			break;
		case 0x3d: 	/* DCR	A */
			M_DCR(I.AF.b.h);
			break;
		case 0x3e: 	/* MVI	A,nn */
			M_MVI(I.AF.b.h);
			break;
		case 0x3f: 	/* CMC	*/
			I.AF.b.l = (I.AF.b.l & 0xfe) | (~I.AF.b.l & CF);
			break;

		case 0x40: 	/* MOV	B,B */
			/* no op */
			break;
		case 0x41: 	/* MOV	B,C */
			I.BC.b.h = I.BC.b.l;
			break;
		case 0x42: 	/* MOV	B,D */
			I.BC.b.h = I.DE.b.h;
			break;
		case 0x43: 	/* MOV	B,E */
			I.BC.b.h = I.DE.b.l;
			break;
		case 0x44: 	/* MOV	B,H */
			I.BC.b.h = I.HL.b.h;
			break;
		case 0x45: 	/* MOV	B,L */
			I.BC.b.h = I.HL.b.l;
			break;
		case 0x46: 	/* MOV	B,M */
			I.BC.b.h = RM(I.HL.d);
			break;
		case 0x47: 	/* MOV	B,A */
			I.BC.b.h = I.AF.b.h;
			break;

		case 0x48: 	/* MOV	C,B */
			I.BC.b.l = I.BC.b.h;
			break;
		case 0x49: 	/* MOV	C,C */
			/* no op */
			break;
		case 0x4a: 	/* MOV	C,D */
			I.BC.b.l = I.DE.b.h;
			break;
		case 0x4b: 	/* MOV	C,E */
			I.BC.b.l = I.DE.b.l;
			break;
		case 0x4c: 	/* MOV	C,H */
			I.BC.b.l = I.HL.b.h;
			break;
		case 0x4d: 	/* MOV	C,L */
			I.BC.b.l = I.HL.b.l;
			break;
		case 0x4e: 	/* MOV	C,M */
			I.BC.b.l = RM(I.HL.d);
			break;
		case 0x4f: 	/* MOV	C,A */
			I.BC.b.l = I.AF.b.h;
			break;

		case 0x50: 	/* MOV	D,B */
			I.DE.b.h = I.BC.b.h;
			break;
		case 0x51: 	/* MOV	D,C */
			I.DE.b.h = I.BC.b.l;
			break;
		case 0x52: 	/* MOV	D,D */
			/* no op */
			break;
		case 0x53: 	/* MOV	D,E */
			I.DE.b.h = I.DE.b.l;
			break;
		case 0x54: 	/* MOV	D,H */
			I.DE.b.h = I.HL.b.h;
			break;
		case 0x55: 	/* MOV	D,L */
			I.DE.b.h = I.HL.b.l;
			break;
		case 0x56: 	/* MOV	D,M */
			I.DE.b.h = RM(I.HL.d);
			break;
		case 0x57: 	/* MOV	D,A */
			I.DE.b.h = I.AF.b.h;
			break;

		case 0x58: 	/* MOV	E,B */
			I.DE.b.l = I.BC.b.h;
			break;
		case 0x59: 	/* MOV	E,C */
			I.DE.b.l = I.BC.b.l;
			break;
		case 0x5a: 	/* MOV	E,D */
			I.DE.b.l = I.DE.b.h;
			break;
		case 0x5b: 	/* MOV	E,E */
			/* no op */
			break;
		case 0x5c: 	/* MOV	E,H */
			I.DE.b.l = I.HL.b.h;
			break;
		case 0x5d: 	/* MOV	E,L */
			I.DE.b.l = I.HL.b.l;
			break;
		case 0x5e: 	/* MOV	E,M */
			I.DE.b.l = RM(I.HL.d);
			break;
		case 0x5f: 	/* MOV	E,A */
			I.DE.b.l = I.AF.b.h;
			break;

		case 0x60: 	/* MOV	H,B */
			I.HL.b.h = I.BC.b.h;
			break;
		case 0x61: 	/* MOV	H,C */
			I.HL.b.h = I.BC.b.l;
			break;
		case 0x62: 	/* MOV	H,D */
			I.HL.b.h = I.DE.b.h;
			break;
		case 0x63: 	/* MOV	H,E */
			I.HL.b.h = I.DE.b.l;
			break;
		case 0x64: 	/* MOV	H,H */
			/* no op */
			break;
		case 0x65: 	/* MOV	H,L */
			I.HL.b.h = I.HL.b.l;
			break;
		case 0x66: 	/* MOV	H,M */
			I.HL.b.h = RM(I.HL.d);
			break;
		case 0x67: 	/* MOV	H,A */
			I.HL.b.h = I.AF.b.h;
			break;

		case 0x68: 	/* MOV	L,B */
			I.HL.b.l = I.BC.b.h;
			break;
		case 0x69: 	/* MOV	L,C */
			I.HL.b.l = I.BC.b.l;
			break;
		case 0x6a: 	/* MOV	L,D */
			I.HL.b.l = I.DE.b.h;
			break;
		case 0x6b: 	/* MOV	L,E */
			I.HL.b.l = I.DE.b.l;
			break;
		case 0x6c: 	/* MOV	L,H */
			I.HL.b.l = I.HL.b.h;
			break;
		case 0x6d: 	/* MOV	L,L */
			/* no op */
			break;
		case 0x6e: 	/* MOV	L,M */
			I.HL.b.l = RM(I.HL.d);
			break;
		case 0x6f: 	/* MOV	L,A */
			I.HL.b.l = I.AF.b.h;
			break;

		case 0x70:  	/* MOV	M,B */
			WM(I.HL.d, I.BC.b.h);
			break;
		case 0x71: 	/* MOV	M,C */
			WM(I.HL.d, I.BC.b.l);
			break;
		case 0x72: 	/* MOV	M,D */
			WM(I.HL.d, I.DE.b.h);
			break;
		case 0x73: 	/* MOV	M,E */
			WM(I.HL.d, I.DE.b.l);
			break;
		case 0x74: 	/* MOV	M,H */
			WM(I.HL.d, I.HL.b.h);
			break;
		case 0x75: 	/* MOV	M,L */
			WM(I.HL.d, I.HL.b.l);
			break;
		case 0x76: 	/* HALT */
			I.PC.w.l--;
			I.HALT = 1;
			if (i8085_ICount > 0) i8085_ICount = 0; //!!
			break;
		case 0x77: 	/* MOV	M,A */
			WM(I.HL.d, I.AF.b.h);
			break;

		case 0x78: 	/* MOV	A,B */
			I.AF.b.h = I.BC.b.h;
			break;
		case 0x79: 	/* MOV	A,C */
			I.AF.b.h = I.BC.b.l;
			break;
		case 0x7a: 	/* MOV	A,D */
			I.AF.b.h = I.DE.b.h;
			break;
		case 0x7b: 	/* MOV	A,E */
			I.AF.b.h = I.DE.b.l;
			break;
		case 0x7c: 	/* MOV	A,H */
			I.AF.b.h = I.HL.b.h;
			break;
		case 0x7d: 	/* MOV	A,L */
			I.AF.b.h = I.HL.b.l;
			break;
		case 0x7e: 	/* MOV	A,M */
			I.AF.b.h = RM(I.HL.d);
			break;
		case 0x7f: 	/* MOV	A,A */
			/* no op */
			break;

		case 0x80: 	/* ADD	B */
			M_ADD(I.BC.b.h);
			break;
		case 0x81: 	/* ADD	C */
			M_ADD(I.BC.b.l);
			break;
		case 0x82: 	/* ADD	D */
			M_ADD(I.DE.b.h);
			break;
		case 0x83: 	/* ADD	E */
			M_ADD(I.DE.b.l);
			break;
		case 0x84: 	/* ADD	H */
			M_ADD(I.HL.b.h);
			break;
		case 0x85:      /* ADD	L */
			M_ADD(I.HL.b.l);
			break;
		case 0x86: 	/* ADD	M */
			M_ADD(RM(I.HL.d));
			break;
		case 0x87: 	/* ADD	A */
			M_ADD(I.AF.b.h);
			break;

		case 0x88: 	/* ADC	B */
			M_ADC(I.BC.b.h);
			break;
		case 0x89: 	/* ADC	C */
			M_ADC(I.BC.b.l);
			break;
		case 0x8a: 	/* ADC	D */
			M_ADC(I.DE.b.h);
			break;
		case 0x8b: 	/* ADC	E */
			M_ADC(I.DE.b.l);
			break;
		case 0x8c: 	/* ADC	H */
			M_ADC(I.HL.b.h);
			break;
		case 0x8d: 	/* ADC	L */
			M_ADC(I.HL.b.l);
			break;
		case 0x8e: 	/* ADC	M */
			M_ADC(RM(I.HL.d));
			break;
		case 0x8f: 	/* ADC	A */
			M_ADC(I.AF.b.h);
			break;

		case 0x90: 	/* SUB	B */
			M_SUB(I.BC.b.h);
			break;
		case 0x91: 	/* SUB	C */
			M_SUB(I.BC.b.l);
			break;
		case 0x92: 	/* SUB	D */
			M_SUB(I.DE.b.h);
			break;
		case 0x93: 	/* SUB	E */
			M_SUB(I.DE.b.l);
			break;
		case 0x94: 	/* SUB	H */
			M_SUB(I.HL.b.h);
			break;
		case 0x95: 	/* SUB	L */
			M_SUB(I.HL.b.l);
			break;
		case 0x96: 	/* SUB	M */
			M_SUB(RM(I.HL.d));
			break;
		case 0x97: 	/* SUB	A */
			M_SUB(I.AF.b.h);
			break;

		case 0x98: 	/* SBB	B */
			M_SBB(I.BC.b.h);
			break;
		case 0x99: 	/* SBB	C */
			M_SBB(I.BC.b.l);
			break;
		case 0x9a: 	/* SBB	D */
			M_SBB(I.DE.b.h);
			break;
		case 0x9b: 	/* SBB	E */
			M_SBB(I.DE.b.l);
			break;
		case 0x9c: 	/* SBB	H */
			M_SBB(I.HL.b.h);
			break;
		case 0x9d: 	/* SBB	L */
			M_SBB(I.HL.b.l);
			break;
		case 0x9e: 	/* SBB	M */
			M_SBB(RM(I.HL.d));
			break;
		case 0x9f: 	/* SBB	A */
			M_SBB(I.AF.b.h);
			break;

		case 0xa0: 	/* ANA	B */
			M_ANA(I.BC.b.h);
			break;
		case 0xa1: 	/* ANA	C */
			M_ANA(I.BC.b.l);
			break;
		case 0xa2: 	/* ANA	D */
			M_ANA(I.DE.b.h);
			break;
		case 0xa3: 	/* ANA	E */
			M_ANA(I.DE.b.l);
			break;
		case 0xa4: 	/* ANA	H */
			M_ANA(I.HL.b.h);
			break;
		case 0xa5: 	/* ANA	L */
			M_ANA(I.HL.b.l);
			break;
		case 0xa6: 	/* ANA	M */
			M_ANA(RM(I.HL.d));
			break;
		case 0xa7: 	/* ANA	A */
			M_ANA(I.AF.b.h);
			break;

		case 0xa8: 	/* XRA	B */
			M_XRA(I.BC.b.h);
			break;
		case 0xa9: 	/* XRA	C */
			M_XRA(I.BC.b.l);
			break;
		case 0xaa: 	/* XRA	D */
			M_XRA(I.DE.b.h);
			break;
		case 0xab: 	/* XRA	E */
			M_XRA(I.DE.b.l);
			break;
		case 0xac: 	/* XRA	H */
			M_XRA(I.HL.b.h);
			break;
		case 0xad: 	/* XRA	L */
			M_XRA(I.HL.b.l);
			break;
		case 0xae: 	/* XRA	M */
			M_XRA(RM(I.HL.d));
			break;
		case 0xaf: 	/* XRA	A */
			M_XRA(I.AF.b.h);
			break;

		case 0xb0: 	/* ORA	B */
			M_ORA(I.BC.b.h);
			break;
		case 0xb1: 	/* ORA	C */
			M_ORA(I.BC.b.l);
			break;
		case 0xb2: 	/* ORA	D */
			M_ORA(I.DE.b.h);
			break;
		case 0xb3: 	/* ORA	E */
			M_ORA(I.DE.b.l);
			break;
		case 0xb4: 	/* ORA	H */
			M_ORA(I.HL.b.h);
			break;
		case 0xb5: 	/* ORA	L */
			M_ORA(I.HL.b.l);
			break;
		case 0xb6: 	/* ORA	M */
			M_ORA(RM(I.HL.d));
			break;
		case 0xb7: 	/* ORA	A */
			M_ORA(I.AF.b.h);
			break;

		case 0xb8: 	/* CMP	B */
			M_CMP(I.BC.b.h);
			break;
		case 0xb9: 	/* CMP	C */
			M_CMP(I.BC.b.l);
			break;
		case 0xba: 	/* CMP	D */
			M_CMP(I.DE.b.h);
			break;
		case 0xbb: 	/* CMP	E */
			M_CMP(I.DE.b.l);
			break;
		case 0xbc: 	/* CMP	H */
			M_CMP(I.HL.b.h);
			break;
		case 0xbd: 	/* CMP	L */
			M_CMP(I.HL.b.l);
			break;
		case 0xbe: 	/* CMP	M */
			M_CMP(RM(I.HL.d));
			break;
		case 0xbf: 	/* CMP	A */
			M_CMP(I.AF.b.h);
			break;

		case 0xc0: 	/* RNZ	*/
			M_RET( !(I.AF.b.l & ZF) );
			break;
		case 0xc1: 	/* POP	B */
			M_POP(BC);
			break;
		case 0xc2: 	/* JNZ	nnnn */
			M_JMP( !(I.AF.b.l & ZF) );
			break;
		case 0xc3: 	/* JMP	nnnn */
			M_JMP(1);
			break;
		case 0xc4: 	/* CNZ	nnnn */
			M_CALL( !(I.AF.b.l & ZF) );
			break;
		case 0xc5: 	/* PUSH B */
			M_PUSH(BC);
			break;
		case 0xc6:  	/* ADI	nn */
			I.XX.b.l = ARG();
			M_ADD(I.XX.b.l);
				break;
		case 0xc7: 	/* RST	0 */
			M_RST(0);
			break;

		case 0xc8: 	/* RZ	*/
			M_RET( I.AF.b.l & ZF );
			break;
		case 0xc9: 	/* RET	*/
			M_RET(1);
			break;
		case 0xca: 	/* JZ	nnnn */
			M_JMP( I.AF.b.l & ZF );
			break;
		case 0xcb:
			if( I.cputype ) {
				if (I.AF.b.l & VF) {
					M_RST(8);			/* call 0x40 */
				} else {
					i8085_ICount += 6;	/* RST  V */
				}
			} else {
					/* JMP	nnnn undocumented*/
				M_JMP(1);
			}
			break;
		case 0xcc: 	/* CZ	nnnn */
			M_CALL( I.AF.b.l & ZF );
			break;
		case 0xcd: 	/* CALL nnnn */
			M_CALL(1);
			break;
		case 0xce: 	/* ACI	nn */
			I.XX.b.l = ARG();
			M_ADC(I.XX.b.l);
			break;
		case 0xcf: 	/* RST	1 */
			M_RST(1);
			break;

		case 0xd0: 	/* RNC	*/
			M_RET( !(I.AF.b.l & CF) );
			break;
		case 0xd1: 	/* POP	D */
			M_POP(DE);
			break;
		case 0xd2: 	/* JNC	nnnn */
			M_JMP( !(I.AF.b.l & CF) );
			break;
		case 0xd3: 	/* OUT	nn */
			M_OUT;
			break;
		case 0xd4: 	/* CNC	nnnn */
			M_CALL( !(I.AF.b.l & CF) );
			break;
		case 0xd5: 	/* PUSH D */
			M_PUSH(DE);
			break;
		case 0xd6: 	/* SUI	nn */
			I.XX.b.l = ARG();
			M_SUB(I.XX.b.l);
			break;
		case 0xd7: 	/* RST	2 */
			M_RST(2);
			break;

		case 0xd8: 	/* RC	*/
			M_RET( I.AF.b.l & CF );
			break;
		case 0xd9:
			if( I.cputype ) {
						/* SHLX */
				I.XX.w.l = I.DE.w.l;
				WM(I.XX.d, I.HL.b.l);
				I.XX.w.l++;
				WM(I.XX.d, I.HL.b.h);
			} else {
					/* RET undocumented */
				M_POP(PC);
                                change_pc16(I.PC.d);									\
			}
			break;
		case 0xda: 	/* JC	nnnn */
			M_JMP( I.AF.b.l & CF );
			break;
		case 0xdb: 	/* IN	nn */
			M_IN;
			break;
		case 0xdc: 	/* CC	nnnn */
			M_CALL( I.AF.b.l & CF );
			break;
		case 0xdd:
			if( I.cputype ) {
						/* JNX  nnnn */
				M_JMP( !(I.AF.b.l & X5F) );
			} else {
					/* CALL nnnn undocumented */
				M_CALL(1);
			}
			break;
		case 0xde: 	/* SBI	nn */
			I.XX.b.l = ARG();
			M_SBB(I.XX.b.l);
			break;
		case 0xdf: 	/* RST	3 */
			M_RST(3);
			break;

		case 0xe0: 	/* RPO	  */
			M_RET( !(I.AF.b.l & PF) );
			break;
		case 0xe1: 	/* POP	H */
			M_POP(HL);
			break;
		case 0xe2: 	/* JPO	nnnn */
			M_JMP( !(I.AF.b.l & PF) );
			break;
		case 0xe3: 	/* XTHL */
			M_POP(XX);
			M_PUSH(HL);
			I.HL.d = I.XX.d;
			break;
		case 0xe4: 	/* CPO	nnnn */
			M_CALL( !(I.AF.b.l & PF) );
			break;
		case 0xe5: 	/* PUSH H */
			M_PUSH(HL);
			break;
		case 0xe6: 	/* ANI	nn */
			I.XX.b.l = ARG();
			M_ANA(I.XX.b.l);
			break;
		case 0xe7: 	/* RST	4 */
			M_RST(4);
			break;

		case 0xe8: 	/* RPO	*/
			M_RET( I.AF.b.l & PF );
			break;
		case 0xe9: 	/* PCHL */
			I.PC.d = I.HL.w.l;
			change_pc16(I.PC.d);
			break;
		case 0xea: 	/* JPE	nnnn */
			M_JMP( I.AF.b.l & PF );
			break;
		case 0xeb: 	/* XCHG */
			I.XX.d = I.DE.d;
			I.DE.d = I.HL.d;
			I.HL.d = I.XX.d;
			break;
		case 0xec: 	/* CPO	nnnn */
			M_CALL( I.AF.b.l & PF );
			break;
		case 0xed:
			if( I.cputype ) {
						/* LHLX */
				I.XX.w.l = I.DE.w.l;
				I.HL.b.l = RM(I.XX.d);
				I.XX.w.l++;
				I.HL.b.h = RM(I.XX.d);
			} else {
					/* CALL nnnn undocumented */
				M_CALL(1);
			}
			break;
		case 0xee: 	/* XRI	nn */
			I.XX.b.l = ARG();
			M_XRA(I.XX.b.l);
			break;
		case 0xef: 	/* RST	5 */
			M_RST(5);
			break;

		case 0xf0: 	/* RP	*/
			M_RET( !(I.AF.b.l&SF) );
			break;
		case 0xf1: 	/* POP	A */
			M_POP(AF);
			break;
		case 0xf2: 	/* JP	nnnn */
			M_JMP( !(I.AF.b.l & SF) );
			break;
		case 0xf3: 	/* DI	*/
			/* remove interrupt enable */
			I.IM &= ~IM_IE;
			break;
		case 0xf4: 	/* CP	nnnn */
			M_CALL( !(I.AF.b.l & SF) );
			break;
		case 0xf5: 	/* PUSH A */
                        //if (IS_8080()) I.AF.b.l = (I.AF.b.l&~(X3F|X5F))|VF; // on 8080, VF=1 and X3F=0 and X5F=0 always! (we don't have to check for it elsewhere)
			M_PUSH(AF);
			break;
		case 0xf6: 	/* ORI	nn */
			I.XX.b.l = ARG();
			M_ORA(I.XX.b.l);
			break;
		case 0xf7: 	/* RST	6 */
			M_RST(6);
			break;

		case 0xf8: 	/* RM	*/
			M_RET( I.AF.b.l & SF );
			break;
		case 0xf9: 	/* SPHL */
			I.SP.d = I.HL.d;
			break;
		case 0xfa: 	/* JM	nnnn */
			M_JMP( I.AF.b.l & SF );
			break;
		case 0xfb: 	/* EI */ //!! not ported from new MAME
			/* set interrupt enable */
			I.IM |= IM_IE;
			/* remove serviced IRQ flag */
			I.IREQ &= ~I.ISRV;
			/* reset serviced IRQ */
			I.ISRV = 0;
			if( I.irq_state[0] != CLEAR_LINE ) {
				LOG(("i8085 EI sets INTR\n"));
				I.IREQ |= IM_INTR;
				I.INTR = I8085_INTR;
			}
			if( I.cputype ) {
				if( I.irq_state[1] != CLEAR_LINE ) {
					LOG(("i8085 EI sets RST5.5\n"));
					I.IREQ |= IM_M55;
				}
				if( I.irq_state[2] != CLEAR_LINE ) {
					LOG(("i8085 EI sets RST6.5\n"));
					I.IREQ |= IM_M65;
				}
				if( I.irq_state[3] != CLEAR_LINE ) {
					LOG(("i8085 EI sets RST7.5\n"));
					I.IREQ |= IM_M75;
				}
				/* find highest priority IREQ flag with
				   IM enabled and schedule for execution */
				if( !(I.IM & IM_M75) && (I.IREQ & IM_M75) ) {
					I.ISRV = IM_M75;
					I.IRQ2 = ADDR_RST75;
				}
				else
				if( !(I.IM & IM_M65) && (I.IREQ & IM_M65) ) {
					I.ISRV = IM_M65;
					I.IRQ2 = ADDR_RST65;
				} else if( !(I.IM & IM_M55) && (I.IREQ & IM_M55) ) {
					I.ISRV = IM_M55;
					I.IRQ2 = ADDR_RST55;
				} else if( !(I.IM & IM_INTR) && (I.IREQ & IM_INTR) ) {
					I.ISRV = IM_INTR;
					I.IRQ2 = I.INTR;
				}
			} else {
				if( !(I.IM & IM_INTR) && (I.IREQ & IM_INTR) ) {
					I.ISRV = IM_INTR;
					I.IRQ2 = I.INTR;
				}
			}
			break;
		case 0xfc: 	/* CM	nnnn */
			M_CALL( I.AF.b.l & SF );
			break;
		case 0xfd:
			if( I.cputype ) {
						/* JX   nnnn */
				M_JMP( I.AF.b.l & X5F );
			} else {
					/* CALL nnnn undocumented */
				M_CALL(1);
			}
			break;
		case 0xfe: 	/* CPI	nn */
			I.XX.b.l = ARG();
			M_CMP(I.XX.b.l);
			break;
		case 0xff: 	/* RST	7 */
			M_RST(7);
			break;
	}
}

static void Interrupt(void)
{

	if( I.HALT )		/* if the CPU was halted */
	{
		I.PC.w.l++; 	/* skip HALT instr */
		I.HALT = 0;
	}
//AT
	I.IREQ &= ~I.ISRV; // remove serviced IRQ flag
	RIM_IEN = (I.ISRV==IM_TRAP) ? I.IM & IM_IE : 0; // latch general interrupt enable bit on TRAP or NMI
//ZT
	I.IM &= ~IM_IE;		/* remove general interrupt enable bit */

	if( I.ISRV == IM_INTR )
	{
		LOG(("Interrupt get INTR vector\n"));
		I.IRQ1 = (I.irq_callback)(0);
	}

	if( I.cputype )
	{
		if( I.ISRV == IM_M55 )
		{
			LOG(("Interrupt get RST5.5 vector\n"));
			//I.IRQ1 = (I.irq_callback)(1);
			I.irq_state[I8085_RST55_LINE] = CLEAR_LINE; //AT: processing RST5.5, reset interrupt line
		}

		if( I.ISRV == IM_M65	)
		{
			LOG(("Interrupt get RST6.5 vector\n"));
			//I.IRQ1 = (I.irq_callback)(2);
			I.irq_state[I8085_RST65_LINE] = CLEAR_LINE; //AT: processing RST6.5, reset interrupt line
		}

		if( I.ISRV == IM_M75 )
		{
			LOG(("Interrupt get RST7.5 vector\n"));
			//I.IRQ1 = (I.irq_callback)(3);
			I.irq_state[I8085_RST75_LINE] = CLEAR_LINE; //AT: processing RST7.5, reset interrupt line
		}
	}

	switch( I.IRQ1 & 0xff0000 )
	{
		case 0xcd0000:	/* CALL nnnn */
			i8085_ICount -= 7;
			M_PUSH(PC);
		case 0xc30000:	/* JMP	nnnn */
			i8085_ICount -= 10;
			I.PC.d = I.IRQ1 & 0xffff;
			change_pc16(I.PC.d);
			break;
		default:
			switch( I.ISRV )
			{
				case IM_TRAP:
				case IM_M75:
				case IM_M65:
				case IM_M55:
					M_PUSH(PC);
					if (I.IRQ1 != (1 << I8085_RST75_LINE))
						I.PC.d = I.IRQ1;
					else
						I.PC.d = 0x3c;
					change_pc16(I.PC.d);
					break;
				default:
					LOG(("i8085 take int $%02x\n", I.IRQ1));
					execute_one(I.IRQ1 & 0xff);
			}
	}
}

int i8085_execute(int cycles)
{

	i8085_ICount = cycles;
	do
	{
		CALL_MAME_DEBUG;
		/* interrupts enabled or TRAP pending ? */
		if ( (I.IM & IM_IE) || (I.IREQ & IM_TRAP) )
		{
			/* copy scheduled to executed interrupt request */
			I.IRQ1 = I.IRQ2;
			/* reset scheduled interrupt request */
			I.IRQ2 = 0;
			/* interrupt now ? */
			if (I.IRQ1) Interrupt();
		}

		/* here we go... */
		execute_one(ROP());

	} while (i8085_ICount > 0);

	return cycles - i8085_ICount;
}

/****************************************************************************
 * Initialise the various lookup tables used by the emulation code
 ****************************************************************************/
static void init_tables (void)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		UINT8 zs = 0;
		int p = 0;
		if (i==0) zs |= ZF;
		if (i&128) zs |= SF;
		if (i&1) ++p;
		if (i&2) ++p;
		if (i&4) ++p;
		if (i&8) ++p;
		if (i&16) ++p;
		if (i&32) ++p;
		if (i&64) ++p;
		if (i&128) ++p;
		ZS[i] = zs;
		ZSP[i] = zs | ((p&1) ? 0 : PF);
	}
}

/****************************************************************************
 * Init the 8085 emulation
 ****************************************************************************/
void i8085_init(void)
{
	int cpu = cpu_getactivecpu();
	init_tables();
	I.cputype = 1;

	state_save_register_UINT16("i8085", cpu, "AF", &I.AF.w.l, 1);
	state_save_register_UINT16("i8085", cpu, "BC", &I.BC.w.l, 1);
	state_save_register_UINT16("i8085", cpu, "DE", &I.DE.w.l, 1);
	state_save_register_UINT16("i8085", cpu, "HL", &I.HL.w.l, 1);
	state_save_register_UINT16("i8085", cpu, "SP", &I.SP.w.l, 1);
	state_save_register_UINT16("i8085", cpu, "PC", &I.PC.w.l, 1);
	state_save_register_UINT8("i8085", cpu, "HALT", &I.HALT, 1);
	state_save_register_UINT8("i8085", cpu, "IM", &I.IM, 1);
	state_save_register_UINT8("i8085", cpu, "IREQ", &I.IREQ, 1);
	state_save_register_UINT8("i8085", cpu, "ISRV", &I.ISRV, 1);
	state_save_register_UINT32("i8085", cpu, "INTR", &I.INTR, 1);
	state_save_register_UINT32("i8085", cpu, "IRQ2", &I.IRQ2, 1);
	state_save_register_UINT32("i8085", cpu, "IRQ1", &I.IRQ1, 1);
	state_save_register_INT8("i8085", cpu, "NMI_STATE", &I.nmi_state, 1);
	state_save_register_INT8("i8085", cpu, "IRQ_STATE", I.irq_state, 4);
}

/****************************************************************************
 * Reset the 8085 emulation
 ****************************************************************************/
void i8085_reset(void *param)
{
	int cputype_bak = I.cputype; //AT: backup cputype(0=8080, 1=8085)

	init_tables();
	memset(&I, 0, sizeof(i8085_Regs)); //AT: this also resets I.cputype so 8085 features were never ever used!
	change_pc16(I.PC.d);

	I.cputype = cputype_bak; //AT: restore cputype
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void i8085_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get the current 8085 context
 ****************************************************************************/
unsigned i8085_get_context(void *dst)
{
	if( dst )
		*(i8085_Regs*)dst = I;
	return sizeof(i8085_Regs);
}

/****************************************************************************
 * Set the current 8085 context
 ****************************************************************************/
void i8085_set_context(void *src)
{
	if( src )
	{
		I = *(i8085_Regs*)src;
		change_pc16(I.PC.d);
	}
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned i8085_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return I.PC.d;
		case I8085_PC: return I.PC.w.l;
		case REG_SP: return I.SP.d;
		case I8085_SP: return I.SP.w.l;
		case I8085_AF: return I.AF.w.l;
		case I8085_BC: return I.BC.w.l;
		case I8085_DE: return I.DE.w.l;
		case I8085_HL: return I.HL.w.l;
		case I8085_IM: return I.IM;
		case I8085_HALT: return I.HALT;
		case I8085_IREQ: return I.IREQ;
		case I8085_ISRV: return I.ISRV;
		case I8085_VECTOR: return I.INTR;
		case I8085_TRAP_STATE: return I.nmi_state;
		case I8085_INTR_STATE: return I.irq_state[I8085_INTR_LINE];
		case I8085_RST55_STATE: return I.irq_state[I8085_RST55_LINE];
		case I8085_RST65_STATE: return I.irq_state[I8085_RST65_LINE];
		case I8085_RST75_STATE: return I.irq_state[I8085_RST75_LINE];
		case REG_PREVIOUSPC: return 0; /* previous pc not supported */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.SP.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RM( offset ) + ( RM( offset+1 ) << 8 );
			}
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void i8085_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: I.PC.w.l = val; change_pc16(I.PC.d); break;
		case I8085_PC: I.PC.w.l = val; break;
		case REG_SP: I.SP.w.l = val; break;
		case I8085_SP: I.SP.w.l = val; break;
		case I8085_AF: I.AF.w.l = val; break;
		case I8085_BC: I.BC.w.l = val; break;
		case I8085_DE: I.DE.w.l = val; break;
		case I8085_HL: I.HL.w.l = val; break;
		case I8085_IM: I.IM = val; break;
		case I8085_HALT: I.HALT = val; break;
		case I8085_IREQ: I.IREQ = val; break;
		case I8085_ISRV: I.ISRV = val; break;
		case I8085_VECTOR: I.INTR = val; break;
		case I8085_TRAP_STATE: I.nmi_state = val; break;
		case I8085_INTR_STATE: I.irq_state[I8085_INTR_LINE] = val; break;
		case I8085_RST55_STATE: I.irq_state[I8085_RST55_LINE] = val; break;
		case I8085_RST65_STATE: I.irq_state[I8085_RST65_LINE] = val; break;
		case I8085_RST75_STATE: I.irq_state[I8085_RST75_LINE] = val; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.SP.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, val&0xff );
					WM( offset+1, (val>>8)&0xff );
				}
			}
	}
}

/****************************************************************************/
/* Set the 8085 SID input signal state										*/
/****************************************************************************/
void i8085_set_SID(int state)
{
	LOG(("i8085: SID %d\n", state));
	if (state)
		I.IM |= IM_SID;
	else
		I.IM &= ~IM_SID;
}

/****************************************************************************/
/* Set a callback to be called at SOD output change 						*/
/****************************************************************************/
void i8085_set_sod_callback(void (*callback)(int state))
{
	I.sod_callback = callback;
}

/****************************************************************************/
/* Set TRAP signal state													*/
/****************************************************************************/
void i8085_set_TRAP(int state)
{
	LOG(("i8085: TRAP %d\n", state));
	if (state)
	{
		I.IREQ |= IM_TRAP;
		if( I.ISRV & IM_TRAP ) return;	/* already servicing TRAP ? */
		I.ISRV = IM_TRAP;				/* service TRAP */
		I.IRQ2 = ADDR_TRAP;
	}
	else
	{
		I.IREQ &= ~IM_TRAP; 			/* remove request for TRAP */
	}
}

/****************************************************************************/
/* Set RST7.5 signal state													*/
/****************************************************************************/
void i8085_set_RST75(int state)
{
	LOG(("i8085: RST7.5 %d\n", state));
	if( state )
	{

		I.IREQ |= IM_M75; 			/* request RST7.5 */
		if( I.IM & IM_M75 ) return;	/* if masked, ignore it for now */
		if( !I.ISRV )					/* if no higher priority IREQ is serviced */
		{
			I.ISRV = IM_M75;			/* service RST7.5 */
			I.IRQ2 = ADDR_RST75;
		}
	}
	/* RST7.5 is reset only by SIM or end of service routine ! */
}

/****************************************************************************/
/* Set RST6.5 signal state													*/
/****************************************************************************/
void i8085_set_RST65(int state)
{
	LOG(("i8085: RST6.5 %d\n", state));
	if( state )
	{
		I.IREQ |= IM_M65; 			/* request RST6.5 */
		if( I.IM & IM_M65 ) return;	/* if masked, ignore it for now */
		if( !I.ISRV )					/* if no higher priority IREQ is serviced */
		{
			I.ISRV = IM_M65;			/* service RST6.5 */
			I.IRQ2 = ADDR_RST65;
		}
	}
	else
	{
		I.IREQ &= ~IM_M65;			/* remove request for RST6.5 */
	}
}

/****************************************************************************/
/* Set RST5.5 signal state													*/
/****************************************************************************/
void i8085_set_RST55(int state)
{
	LOG(("i8085: RST5.5 %d\n", state));
	if( state )
	{
		I.IREQ |= IM_M55; 			/* request RST5.5 */
		if( I.IM & IM_M55 ) return;	/* if masked, ignore it for now */
		if( !I.ISRV )					/* if no higher priority IREQ is serviced */
		{
			I.ISRV = IM_M55;			/* service RST5.5 */
			I.IRQ2 = ADDR_RST55;
		}
	}
	else
	{
		I.IREQ &= ~IM_M55;			/* remove request for RST5.5 */
	}
}

/****************************************************************************/
/* Set INTR signal															*/
/****************************************************************************/
void i8085_set_INTR(int state)
{
	LOG(("i8085: INTR %d\n", state));
	if( state )
	{
		I.IREQ |= IM_INTR;				/* request INTR */
		//I.INTR = state;
		I.INTR = I8085_INTR; //AT: I.INTR is supposed to hold IRQ0 vector(0x38) (0xff in this implementation)
		if( I.IM & IM_INTR ) return;	/* if masked, ignore it for now */
		if( !I.ISRV )					/* if no higher priority IREQ is serviced */
		{
			I.ISRV = IM_INTR;			/* service INTR */
			I.IRQ2 = I.INTR;
		}
	}
	else
	{
		I.IREQ &= ~IM_INTR; 			/* remove request for INTR */
	}
}

void i8085_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		I.nmi_state = state;
		if( state != CLEAR_LINE )
			i8085_set_TRAP(1);
	}
	else if (irqline < 4)
	{
		I.irq_state[irqline] = state;
		if (state == CLEAR_LINE)
		{
			if( !(I.IM & IM_IE) )
			{
				switch (irqline)
				{
					case I8085_INTR_LINE: i8085_set_INTR(0); break;
					case I8085_RST55_LINE: i8085_set_RST55(0); break;
					case I8085_RST65_LINE: i8085_set_RST65(0); break;
					case I8085_RST75_LINE: i8085_set_RST75(0); break;
				}
			}
		}
		else
		{
			if( I.IM & IM_IE )
			{
				switch( irqline )
				{
					case I8085_INTR_LINE: i8085_set_INTR(1); break;
					case I8085_RST55_LINE: i8085_set_RST55(1); break;
					case I8085_RST65_LINE: i8085_set_RST65(1); break;
					case I8085_RST75_LINE: i8085_set_RST75(1); break;
				}
			}
		}
	}
}

void i8085_set_irq_callback(int (*callback)(int))
{
	I.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i8085_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	i8085_Regs *r = (i8085_Regs*)context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+I8085_AF: sprintf(buffer[which], "AF:%04X", r->AF.w.l); break;
		case CPU_INFO_REG+I8085_BC: sprintf(buffer[which], "BC:%04X", r->BC.w.l); break;
		case CPU_INFO_REG+I8085_DE: sprintf(buffer[which], "DE:%04X", r->DE.w.l); break;
		case CPU_INFO_REG+I8085_HL: sprintf(buffer[which], "HL:%04X", r->HL.w.l); break;
		case CPU_INFO_REG+I8085_SP: sprintf(buffer[which], "SP:%04X", r->SP.w.l); break;
		case CPU_INFO_REG+I8085_PC: sprintf(buffer[which], "PC:%04X", r->PC.w.l); break;
		case CPU_INFO_REG+I8085_IM: sprintf(buffer[which], "IM:%02X", r->IM); break;
		case CPU_INFO_REG+I8085_HALT: sprintf(buffer[which], "HALT:%d", r->HALT); break;
		case CPU_INFO_REG+I8085_IREQ: sprintf(buffer[which], "IREQ:%02X", I.IREQ); break;
		case CPU_INFO_REG+I8085_ISRV: sprintf(buffer[which], "ISRV:%02X", I.ISRV); break;
		case CPU_INFO_REG+I8085_VECTOR: sprintf(buffer[which], "VEC:%02X", I.INTR); break;
		case CPU_INFO_REG+I8085_TRAP_STATE: sprintf(buffer[which], "TRAP:%X", I.nmi_state); break;
		case CPU_INFO_REG+I8085_INTR_STATE: sprintf(buffer[which], "INTR:%X", I.irq_state[I8085_INTR_LINE]); break;
		case CPU_INFO_REG+I8085_RST55_STATE: sprintf(buffer[which], "RST55:%X", I.irq_state[I8085_RST55_LINE]); break;
		case CPU_INFO_REG+I8085_RST65_STATE: sprintf(buffer[which], "RST65:%X", I.irq_state[I8085_RST65_LINE]); break;
		case CPU_INFO_REG+I8085_RST75_STATE: sprintf(buffer[which], "RST75:%X", I.irq_state[I8085_RST75_LINE]); break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				(r->AF.b.l & 0x80) ? 'S':'.',
				(r->AF.b.l & 0x40) ? 'Z':'.',
				(r->AF.b.l & 0x20) ? 'X':'.', // X5
				(r->AF.b.l & 0x10) ? 'H':'.',
				(r->AF.b.l & 0x08) ? '?':'.',
				(r->AF.b.l & 0x04) ? 'P':'.',
				(r->AF.b.l & 0x02) ? 'V':'.',
				(r->AF.b.l & 0x01) ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "8085A";
		case CPU_INFO_FAMILY: return "Intel 8080";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1999 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char *)i8085_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)i8085_win_layout;
	}
	return buffer[which];
}

unsigned i8085_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm8085(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}


/**************************************************************************
 * 8080 section
 **************************************************************************/
#if (HAS_8080)
/* Layout of the registers in the debugger */
static UINT8 i8080_reg_layout[] = {
	I8080_AF, I8080_BC, I8080_DE, I8080_HL, I8080_SP, I8080_PC, -1,
	I8080_HALT, I8080_IREQ, I8080_ISRV, I8080_VECTOR, I8080_TRAP_STATE, I8080_INTR_STATE,
	0 };

/* Layout of the debugger windows x,y,w,h */
static UINT8 i8080_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55,10,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void i8080_init(void)
{
	int cpu = cpu_getactivecpu();
	init_tables();
	I.cputype = 0;

	state_save_register_UINT16("i8080", cpu, "AF", &I.AF.w.l, 1);
	state_save_register_UINT16("i8080", cpu, "BC", &I.BC.w.l, 1);
	state_save_register_UINT16("i8080", cpu, "DE", &I.DE.w.l, 1);
	state_save_register_UINT16("i8080", cpu, "HL", &I.HL.w.l, 1);
	state_save_register_UINT16("i8080", cpu, "SP", &I.SP.w.l, 1);
	state_save_register_UINT16("i8080", cpu, "PC", &I.PC.w.l, 1);
	state_save_register_UINT8("i8080", cpu, "HALT", &I.HALT, 1);
	state_save_register_UINT8("i8080", cpu, "IREQ", &I.IREQ, 1);
	state_save_register_UINT8("i8080", cpu, "ISRV", &I.ISRV, 1);
	state_save_register_UINT32("i8080", cpu, "INTR", &I.INTR, 1);
	state_save_register_UINT32("i8080", cpu, "IRQ2", &I.IRQ2, 1);
	state_save_register_UINT32("i8080", cpu, "IRQ1", &I.IRQ1, 1);
	state_save_register_INT8("i8080", cpu, "nmi_state", &I.nmi_state, 1);
	state_save_register_INT8("i8080", cpu, "irq_state", I.irq_state, 1);
}

void i8080_reset(void *param) { i8085_reset(param); }
void i8080_exit(void) { i8085_exit(); }
int i8080_execute(int cycles) { return i8085_execute(cycles); }
unsigned i8080_get_context(void *dst) { return i8085_get_context(dst); }
void i8080_set_context(void *src) { i8085_set_context(src); }
unsigned i8080_get_reg(int regnum) { return i8085_get_reg(regnum); }
void i8080_set_reg(int regnum, unsigned val)  { i8085_set_reg(regnum,val); }
void i8080_set_irq_line(int irqline, int state)
{
	if (irqline == IRQ_LINE_NMI)
	{
		i8085_set_irq_line(irqline, state);
	}
	else
	{
		I.irq_state[irqline] = state;
		if (state == CLEAR_LINE)
		{
			if (!(I.IM & IM_IE))
				i8085_set_INTR(0);
		}
		else
		{
			if (I.IM & IM_IE)
				i8085_set_INTR(1);
		}
	}
}
void i8080_set_irq_callback(int (*callback)(int irqline)) { i8085_set_irq_callback(callback); }
const char *i8080_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "8080";
		case CPU_INFO_VERSION: return "1.2";
		case CPU_INFO_REG_LAYOUT: return (const char *)i8080_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)i8080_win_layout;
	}
	return i8085_info(context,regnum);
}

unsigned i8080_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm8085(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
#endif

