#define VERBOSE 0

#include "driver.h"
#include "state.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "pps4.h"
#include "pps4cpu.h"

#if VERBOSE
#include <stdio.h>
#include "driver.h"
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

/* Layout of the registers in the debugger */
static UINT8 PPS4_reg_layout[] = {
	PPS4_PC, PPS4_SA, PPS4_SB, PPS4_BX, PPS4_AB, PPS4_DB, PPS4_A, PPS4_X,
	/* PPS4_C, PPS4_F1, PPS4_F2, PPS4_SK, */ 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 PPS4_win_layout[] = {
	25, 0,55, 1,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 2,55,10,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct {
	int 	cputype;	/* 0 = PPS-4 (10660), 1 = PPS-4/2 (11660) */
	PAIR	PC, SA, SB, BX, AB;
	UINT8   DB;
	INT8	accu, xreg, carry, ff1, ff2, skip;
}	PPS4_Regs;

static PPS4_Regs I;

int PPS4_ICount = 0;
int wasLB = 0;
int wasLDI = 0;

/* Word count for all opcodes */
static int words[] = {
	2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static UINT8 ROP(void)
{
#if INVERT_DATA
	return (~cpu_readop(I.PC.w.l++)) & 0xff;
#else
	return cpu_readop(I.PC.w.l++);
#endif
}

static UINT8 ARG(void)
{
#if INVERT_DATA
	return (~cpu_readop_arg(I.PC.w.l++)) & 0xff;
#else
	return cpu_readop_arg(I.PC.w.l++);
#endif
}

static UINT8 RM(UINT32 a)
{
	return cpu_readmem16(a) & 0x0f;
}

static void WM(UINT32 a, UINT8 v)
{
if (a > 0x10ff) logerror("%03x: Write to memory @%04x:%x\n", activecpu_get_pc(), a, v);
	cpu_writemem16(a, v & 0x0f);
}

INLINE void execute_one(UINT8 opcode)
{
	PAIR tmpPair;
	int tmp;

	PPS4_ICount -= words[opcode];
	if (wasLB) wasLB--;
	if (wasLDI) wasLDI--;
	I.skip = 0;

	switch (opcode)
	{
		case 0x00: /* LBL */
			if (!wasLB) {
				I.BX.b.h = 0;
				I.BX.b.l = ~ARG() & 0xff;
				I.AB.b.l = I.BX.b.l;
				I.DB = I.BX.b.l;
			} else {
				I.PC.w.l++;
				change_pc16(I.PC.d);
			}
			wasLB = 2;
			break;
		/* TML */
		case 0x01: case 0x02: case 0x03:
			tmpPair = I.PC;
			tmpPair.w.l++;
			I.SB = I.SA;
			I.SA = tmpPair;
			M_JMP(opcode)
			break;
		case 0x04: /* LBUA */
			I.BX.b.h = I.accu;
			I.accu = RM(0x1000 | I.AB.w.l);
			I.AB = I.BX;
			break;
		case 0x05: /* RTN */
			tmpPair = I.SA;
			I.PC = tmpPair;
			I.SA = I.SB;
			I.SB = tmpPair;
			change_pc16(I.PC.d);
			break;
		case 0x06: /* XS */
			tmpPair = I.SA;
			I.SA = I.SB;
			I.SB = tmpPair;
			break;
		case 0x07: /* RTNSK */
			tmpPair = I.SA;
			I.PC = tmpPair;
			I.SA = I.SB;
			I.SB = tmpPair;
			change_pc16(I.PC.d); // manual says P = P + 1, but this won't work for TML!
			I.skip = 1;
			break;
		case 0x08: /* ADCSK */
			I.accu += I.carry + RM(0x1000 | I.AB.w.l);
			I.skip = I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			I.AB = I.BX;
			break;
		case 0x09: /* ADSK */
			I.accu += I.carry + RM(0x1000 | I.AB.w.l); // This is exactly the same as ADCSK, so where's the difference?
//			I.accu += RM(0x1000 | I.AB.w.l); // This should actually be correct, but apparently it is not!?
			I.skip = I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			I.AB = I.BX;
			break;
		case 0x0a: /* ADC */
			I.accu += I.carry + RM(0x1000 | I.AB.w.l);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			I.AB = I.BX;
			break;
		case 0x0b: /* AD */
			I.accu += RM(0x1000 | I.AB.w.l);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			I.AB = I.BX;
			break;
		case 0x0c: /* EOR */
			I.accu ^= RM(0x1000 | I.AB.w.l);
			I.AB = I.BX;
			break;
		case 0x0d: /* AND */
			I.accu &= RM(0x1000 | I.AB.w.l);
			I.AB = I.BX;
			break;
		case 0x0e: /* COMP */
			I.accu ^= 0x0f;
			break;
		case 0x0f: /* OR */
			I.accu |= RM(0x1000 | I.AB.w.l);
			I.AB = I.BX;
			break;

		case 0x10: /* LBMX */
			I.BX.b.l = (I.BX.b.l & 0x0f) | (I.xreg << 4);
			I.AB.b.l = (I.AB.b.l & 0x0f) | (I.BX.b.l & 0xf0);
			I.DB = I.BX.b.l;
			break;
		case 0x11: /* LABL */
			I.accu = I.BX.b.l & 0x0f;
			break;
		case 0x12: /* LAX */
			I.accu = I.xreg;
			break;
		case 0x13: /* SAG */
			I.AB.b.h = 0;
			I.AB.b.l = I.BX.b.l & 0x0f;
			break;
		case 0x14: /* SKF2 */
			I.skip = I.ff2;
			break;
		case 0x15: /* SKC */
			I.skip = I.carry;
			break;
		case 0x16: /* SKF1 */
			I.skip = I.ff1;
			break;
		case 0x17: /* INCB */
			if ((I.BX.b.l & 0x0f) == 0x0f) {
				I.BX.b.l &= 0xf0;
				I.skip = 1;
			} else
				I.BX.b.l++;
			I.AB.b.l = (I.AB.b.l & 0xf0) | (I.BX.b.l & 0x0f);
			I.DB = I.BX.b.l;
			break;
		case 0x18: /* XBMX */
			tmp = I.xreg;
			I.xreg = (I.BX.b.l >> 4) & 0x0f;
			I.BX.b.l = (I.BX.b.l & 0x0f) | (tmp << 4);
			I.AB.b.l = (I.AB.b.l & 0x0f) | (I.BX.b.l & 0xf0);
			I.DB = I.BX.b.l;
			break;
		case 0x19: /* XABL */
			tmp = I.accu;
			I.accu = I.BX.b.l & 0x0f;
			I.BX.b.l = (I.BX.b.l & 0xf0) | tmp;
			I.AB.b.l = (I.AB.b.l & 0xf0) | (I.BX.b.l & 0x0f);
			I.DB = I.BX.b.l;
			break;
		case 0x1a: /* XAX */
			tmp = I.accu;
			I.accu = I.xreg;
			I.xreg = tmp;
			break;
		case 0x1b: /* LXA */
			I.xreg = I.accu;
			break;
		case 0x1c: /* IOL */
			tmp = ARG();
			M_OUT((tmp & 0xf0) | (I.DB & 0x0f), ((tmp & 0x0f) << 4) | I.accu);
			M_IN((tmp & 0xf0) | (I.DB & 0x0f));
			break;
		case 0x1d: /* DOA */
			M_OUT(0x100, I.accu)
			if (I.cputype)
				M_OUT(0x101, I.xreg) // PPS-4/2 only!
			break;
		case 0x1e: /* SKZ */
			I.skip = (!I.accu);
			break;
		case 0x1f: /* DECB */
			if (!(I.BX.b.l & 0x0f)) {
				I.BX.b.l |= 0x0f;
				I.skip = 1;
			} else
				I.BX.b.l--;
			I.AB.b.l = (I.AB.b.l & 0xf0) | (I.BX.b.l & 0x0f);
			I.DB = I.BX.b.l;
			break;

		case 0x20: /* SC */
			I.carry = 1;
			break;
		case 0x21: /* SF2 */
			I.ff2 = 1;
			break;
		case 0x22: /* SF1 */
			I.ff1 = 1;
			break;
		case 0x23: /* DIB */
			M_IN(0x101)
			break;
		case 0x24: /* RC */
			I.carry = 0;
			break;
		case 0x25: /* RF2 */
			I.ff2 = 0;
			break;
		case 0x26: /* RF1 */
			I.ff1 = 0;
			break;
		case 0x27: /* DIA */
			M_IN(0x100)
			break;
		/* EXD */
		case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f:
			tmp = I.accu;
			I.accu = RM(0x1000 | I.AB.w.l);
			WM(0x1000 | I.AB.w.l, tmp);
			I.BX.b.l ^= (~opcode << 4) & 0x70;
			if (!(I.BX.b.l & 0x0f)) {
				I.BX.b.l |= 0x0f;
				I.skip = 1;
			} else
				I.BX.b.l--;
			I.AB.b.l = (I.AB.b.l & 0x80) | (I.BX.b.l & 0x7f);
			I.DB = I.BX.b.l;
			break;

		/* LD */
		case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			I.accu = RM(0x1000 | I.AB.w.l);
			I.BX.b.l ^= (~opcode << 4) & 0x70;
			I.AB.b.l = (I.AB.b.l & 0x8f) | (I.BX.b.l & 0x70);
			I.DB = I.BX.b.l;
			break;
		/* EX */
		case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f:
			tmp = I.accu;
			I.accu = RM(0x1000 | I.AB.w.l);
			WM(0x1000 | I.AB.w.l, tmp);
			I.BX.b.l ^= (~opcode << 4) & 0x70;
			I.AB.b.l = (I.AB.b.l & 0x8f) | (I.BX.b.l & 0x70);
			I.DB = I.BX.b.l;
			break;

		/* SKBI */
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
			I.skip = ((I.BX.b.l & 0x0f) == (opcode & 0x0f));
			break;

		/* TL */
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
			M_JMP(opcode & 0x0f);
			break;

		/* ADI */
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x66: case 0x67:
		case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e:
			I.accu += ~opcode & 0x0f;
			I.skip = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x65: /* DC */
			I.accu = (I.accu + 0x0a) & 0x0f;
			break;
		case 0x6f: /* CYS */
			tmp = ~I.accu & 0x0f;
			I.accu = ~I.SA.w.l & 0x0f;
			I.SA.w.l = (I.SA.w.l >> 4) | (tmp << 8);
			break;

		/* LDI */
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			if (!wasLDI) {
				I.accu = ~opcode & 0x0f;
			}
			wasLDI = 2;
			break;

		/* T */
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b: case 0x8c: case 0x8d: case 0x8e: case 0x8f:

		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b: case 0x9c: case 0x9d: case 0x9e: case 0x9f:

		case 0xa0: case 0xa1: case 0xa2: case 0xa3: case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab: case 0xac: case 0xad: case 0xae: case 0xaf:

		case 0xb0: case 0xb1: case 0xb2: case 0xb3: case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb: case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			I.PC.w.l = (I.PC.w.l & 0xfc0) | (opcode & 0x3f);
			change_pc16(I.PC.d);
			break;

		/* LB */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
			if (!wasLB) {
				I.SB = I.SA;
				I.SA = I.PC;
				I.PC.w.l = opcode;
				I.BX.b.h = 0;
				I.BX.b.l = ~ARG() & 0xff;
				I.AB.b.l = I.BX.b.l;
				I.DB = I.BX.b.l;
				tmpPair = I.SA;
				I.PC = tmpPair;
				I.SA = I.SB;
				I.SB = tmpPair;
			} else {
				I.PC.w.l++;
				change_pc16(I.PC.d);
			}
			wasLB = 2;
			break;

		/* TM */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:

		case 0xe0: case 0xe1: case 0xe2: case 0xe3: case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb: case 0xec: case 0xed: case 0xee: case 0xef:

		case 0xf0: case 0xf1: case 0xf2: case 0xf3: case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb: case 0xfc: case 0xfd: case 0xfe: case 0xff:
			I.SB = I.SA;
			I.SA = I.PC;
			I.PC.w.l = opcode;
			M_JMP(0x01)
			break;
	}
	if (I.skip) {
		opcode = ROP();
		I.PC.w.l += words[opcode] - 1;
		PPS4_ICount -= words[opcode];
	}
}

int PPS4_execute(int cycles)
{
	PPS4_ICount = cycles;
	do
	{
		CALL_MAME_DEBUG;
		/* here we go... */
		execute_one(ROP());

	} while (PPS4_ICount > 0);

	return cycles - PPS4_ICount;
}

/****************************************************************************
 * Init the PPS-4 emulation
 ****************************************************************************/
void PPS4_init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("PPS4", cpu, "PC", &I.PC.w.l, 1);
	state_save_register_UINT16("PPS4", cpu, "SA", &I.SA.w.l, 1);
	state_save_register_UINT16("PPS4", cpu, "SB", &I.SB.w.l, 1);
	state_save_register_UINT16("PPS4", cpu, "BX", &I.BX.w.l, 1);
	state_save_register_UINT16("PPS4", cpu, "AB", &I.AB.w.l, 1);
	state_save_register_UINT8("PPS4", cpu, "DB", &I.DB, 1);
	state_save_register_INT8("PPS4", cpu, "ACCU", &I.accu, 1);
	state_save_register_INT8("PPS4", cpu, "CARRY", &I.carry, 1);
	state_save_register_INT8("PPS4", cpu, "X", &I.xreg, 1);
	state_save_register_INT8("PPS4", cpu, "FF1", &I.ff1, 1);
	state_save_register_INT8("PPS4", cpu, "FF2", &I.ff2, 1);
	state_save_register_INT8("PPS4", cpu, "SKIP", &I.skip, 1);
}

/****************************************************************************
 * Reset the PPS-4 emulation
 ****************************************************************************/
void PPS4_reset(void *param)
{
	memset(&I, 0, sizeof(PPS4_Regs));
	I.cputype = 1; // we use the PPS-4/2 by default for enhanced I/O capability!
	PPS4_ICount = 0;
	wasLB = 0;
	wasLDI = 0;
	change_pc16(I.PC.d);
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void PPS4_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get the current PPS-4 context
 ****************************************************************************/
unsigned PPS4_get_context(void *dst)
{
	if( dst )
		*(PPS4_Regs*)dst = I;
	return sizeof(PPS4_Regs);
}

/****************************************************************************
 * Set the current PPS-4 context
 ****************************************************************************/
void PPS4_set_context(void *src)
{
	if( src )
	{
		I = *(PPS4_Regs*)src;
		change_pc16(I.PC.d);
	}
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned PPS4_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return I.PC.d;
		case PPS4_PC: return I.PC.w.l;
		case REG_SP: return I.SA.d;
		case PPS4_SA: return I.SA.w.l;
		case PPS4_SB: return I.SB.w.l;
		case PPS4_BX: return I.BX.w.l;
		case PPS4_AB: return I.AB.w.l;
		case PPS4_DB: return I.DB;
		case PPS4_A: return I.accu;
		case PPS4_C: return I.carry;
		case PPS4_X: return I.xreg;
		case PPS4_F1: return I.ff1;
		case PPS4_F2: return I.ff2;
		case PPS4_SK: return I.skip;
		case REG_PREVIOUSPC: return 0; /* previous pc not supported */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.SA.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RM( offset ) + ( RM( offset+1 ) << 8 );
			}
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void PPS4_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: I.PC.w.l = val; change_pc16(I.PC.d); break;
		case PPS4_PC: I.PC.w.l = val; break;
		case REG_SP: I.SA.w.l = val; break;
		case PPS4_SA: I.SA.w.l = val; break;
		case PPS4_SB: I.SB.w.l = val; break;
		case PPS4_BX: I.BX.w.l = val; break;
		case PPS4_AB: I.AB.w.l = val; break;
		case PPS4_DB: I.DB = val & 0xff; break;
		case PPS4_A: I.accu = val & 0x0f; break;
		case PPS4_X: I.xreg = val & 0x0f; break;
		case PPS4_C: I.carry = val > 0; break;
		case PPS4_F1: I.ff1 = val > 0; break;
		case PPS4_F2: I.ff2 = val > 0; break;
		case PPS4_SK: I.skip = val > 0; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.SA.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, val );
					WM( offset+1, val>>8 );
				}
			}
	}
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *PPS4_info(void *context, int regnum)
{
	static char buffer[12][47+1];
	static int which = 0;
	PPS4_Regs *r = context;

	which = (which+1) % 12;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+PPS4_PC: sprintf(buffer[which], "PC:%03X", r->PC.w.l); break;
		case CPU_INFO_REG+PPS4_SA: sprintf(buffer[which], "SA:%03X", r->SA.w.l); break;
		case CPU_INFO_REG+PPS4_SB: sprintf(buffer[which], "SB:%03X", r->SB.w.l); break;
		case CPU_INFO_REG+PPS4_BX: sprintf(buffer[which], "BX:%03X", r->BX.w.l); break;
		case CPU_INFO_REG+PPS4_AB: sprintf(buffer[which], "AB:%03X", r->AB.w.l); break;
		case CPU_INFO_REG+PPS4_DB: sprintf(buffer[which], "DB:%02X", I.DB); break;
		case CPU_INFO_REG+PPS4_A: sprintf(buffer[which], "A:%X", I.accu); break;
		case CPU_INFO_REG+PPS4_C: sprintf(buffer[which], "C:%X", I.carry); break;
		case CPU_INFO_REG+PPS4_X: sprintf(buffer[which], "X:%X", I.xreg); break;
		case CPU_INFO_REG+PPS4_F1: sprintf(buffer[which], "FF1:%X", I.ff1); break;
		case CPU_INFO_REG+PPS4_F2: sprintf(buffer[which], "FF2:%X", I.ff2); break;
		case CPU_INFO_REG+PPS4_SK: sprintf(buffer[which], "SKP:%X", I.skip); break;
		case CPU_INFO_FLAGS: sprintf(buffer[which], "%c%c%c%c",
		  (I.carry ? 'C' : '.'), (I.ff1 ? '1' : '.'), (I.ff2 ? '2' : '.'), (I.skip ? 'S' : '.')); break;
		case CPU_INFO_NAME: return (I.cputype ? "PPS-4/2" : "PPS-4");
		case CPU_INFO_FAMILY: return (I.cputype ? "Rockwell PPS-4/2" : "Rockwell PPS-4");
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "written 2002 by G. Volkenborn";
		case CPU_INFO_REG_LAYOUT: return (const char *)PPS4_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)PPS4_win_layout;
	}
	return buffer[which];
}

void PPS4_set_irq_line(int irqline, int state) {
	/* no IRQ line on the PPS-4 */
}

void PPS4_set_irq_callback(int (*callback)(int)) {
	/* no IRQ line on the PPS-4 */
}

unsigned PPS4_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmPPS4(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
