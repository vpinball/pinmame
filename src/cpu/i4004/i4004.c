#define VERBOSE 0

#include "driver.h"
#include "state.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "i4004.h"
#include "i4004cpu.h"

#if VERBOSE
#include <stdio.h>
#include "driver.h"
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

/* Layout of the registers in the debugger */
static UINT8 i4004_reg_layout[] = {
	I4004_PC, I4004_S1, I4004_S2, I4004_S3, I4004_RAM,I4004_A, /* I4004_C, */ I4004_T, -1,
	I4004_01, I4004_23, I4004_45, I4004_67, I4004_89, I4004_AB, I4004_CD, I4004_EF, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i4004_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55,10,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct {
	int 	cputype;	/* 0 = 4004 */
	PAIR	PC, S1, S2, S3, ramaddr;
	UINT8	R01, R23, R45, R67, R89, RAB, RCD, REF;
	INT8	accu, carry, test;
}	i4004_Regs;

int i4004_ICount = 0;

static i4004_Regs I;

static UINT8 ROP(void)
{
	return cpu_readop(I.PC.w.l++);
}

static UINT8 ARG(void)
{
	return cpu_readop_arg(I.PC.w.l++);
}

static UINT8 RM(UINT32 a)
{
	return cpu_readmem16(a);
}

static void WM(UINT32 a, UINT8 v)
{
	cpu_writemem16(a, v);
}

static void illegal(void)
{
#if VERBOSE
	UINT16 pc = I.PC.w.l - 1;
	LOG(("i4004 illegal instruction %04X $%02X\n", pc, cpu_readop(pc)));
#endif
}

INLINE void execute_one(int opcode)
{
	int tmp, romno = I.PC.w.l >> 8;
	i4004_ICount -= 8;
	switch (opcode)
	{
		case 0x00: /* NOP */
			break;

		case 0x10: /* JCN (*) - jump always */
			M_JMP(romno, 1)
			break;
		case 0x11: /* JCN (~T) */
			M_JMP(romno, !I.test)
			break;
		case 0x12: /* JCN (C) */
			M_JMP(romno, I.carry)
			break;
		case 0x13: /* JCN (C~T) */
			M_JMP(romno, (I.carry && !I.test))
			break;
		case 0x14: /* JCN (~A) */
			M_JMP(romno, !I.accu)
			break;
		case 0x15: /* JCN (~A~T) */
			M_JMP(romno, (!I.accu && !I.test))
			break;
		case 0x16: /* JCN (C~A) */
			M_JMP(romno, (I.carry && !I.accu))
			break;
		case 0x17: /* JCN (C~A~T) */
			M_JMP(romno, (I.carry && !I.accu && !I.test))
			break;
		case 0x18: /* JCN (~*) - never jump */
			M_JMP(romno, 0)
			break;
		case 0x19: /* JCN (T) */
			M_JMP(romno, I.test)
			break;
		case 0x1a: /* JCN (~C) */
			M_JMP(romno, !I.carry)
			break;
		case 0x1b: /* JCN (T~C) */
			M_JMP(romno, (I.test && !I.carry))
			break;
		case 0x1c: /* JCN (A) */
			M_JMP(romno, I.accu)
			break;
		case 0x1d: /* JCN (AT) */
			M_JMP(romno, (I.accu && I.test))
			break;
		case 0x1e: /* JCN (A~C) */
			M_JMP(romno, (I.accu && !I.carry))
			break;
		case 0x1f: /* JCN (AT~C) */
			M_JMP(romno, (I.accu && I.test && !I.carry))
			break;

		case 0x20: /* FIM */
			i4004_ICount -= 8;
			I.R01 = ARG();
			break;
		case 0x22: /* FIM */
			i4004_ICount -= 8;
			I.R23 = ARG();
			break;
		case 0x24: /* FIM */
			i4004_ICount -= 8;
			I.R45 = ARG();
			break;
		case 0x26: /* FIM */
			i4004_ICount -= 8;
			I.R67 = ARG();
			break;
		case 0x28: /* FIM */
			i4004_ICount -= 8;
			I.R89 = ARG();
			break;
		case 0x2a: /* FIM */
			i4004_ICount -= 8;
			I.RAB = ARG();
			break;
		case 0x2c: /* FIM */
			i4004_ICount -= 8;
			I.RCD = ARG();
			break;
		case 0x2e: /* FIM */
			i4004_ICount -= 8;
			I.REF = ARG();
			break;

		case 0x21: /* SRC */
			I.ramaddr.w.l = I.R01;
			break;
		case 0x23: /* SRC */
			I.ramaddr.w.l = I.R23;
			break;
		case 0x25: /* SRC */
			I.ramaddr.w.l = I.R45;
			break;
		case 0x27: /* SRC */
			I.ramaddr.w.l = I.R67;
			break;
		case 0x29: /* SRC */
			I.ramaddr.w.l = I.R89;
			break;
		case 0x2b: /* SRC */
			I.ramaddr.w.l = I.RAB;
			break;
		case 0x2d: /* SRC */
			I.ramaddr.w.l = I.RCD;
			break;
		case 0x2f: /* SRC */
			I.ramaddr.w.l = I.REF;
			break;

		case 0x30: /* FIN */
			I.R01 = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x32: /* FIN */
			I.R23 = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x34: /* FIN */
			I.R45 = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x36: /* FIN */
			I.R67 = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x38: /* FIN */
			I.R89 = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x3a: /* FIN */
			I.RAB = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x3c: /* FIN */
			I.RCD = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;
		case 0x3e: /* FIN */
			I.REF = cpu_readop((I.PC.w.l & 0xf00) | I.R01);
			break;

		case 0x31: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.R01;
			change_pc16(I.PC.d);
			break;
		case 0x33: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.R23;
			change_pc16(I.PC.d);
			break;
		case 0x35: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.R45;
			change_pc16(I.PC.d);
			break;
		case 0x37: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.R67;
			change_pc16(I.PC.d);
			break;
		case 0x39: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.R89;
			change_pc16(I.PC.d);
			break;
		case 0x3b: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.RAB;
			change_pc16(I.PC.d);
			break;
		case 0x3d: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.RCD;
			change_pc16(I.PC.d);
			break;
		case 0x3f: /* JIN */
			I.PC.w.l = (I.PC.w.l & 0xf00) | I.REF;
			change_pc16(I.PC.d);
			break;

        /* JMS */
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f:
			I.S3 = I.S2;
			I.S2 = I.S1;
			I.S1 = I.PC;
			I.S1.w.l++;

		/* JUN */
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
		case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f:
			M_JMP(opcode & 0x0f, 1)
			break;

		case 0x60: /* INC */
			I.R01 = (I.R01 & 0x0f) | ((I.R01 + 0x10) & 0xf0);
			break;
		case 0x61: /* INC */
			I.R01 = (I.R01 & 0xf0) | ((I.R01 + 1) & 0x0f);
			break;
		case 0x62: /* INC */
			I.R23 = (I.R23 & 0x0f) | ((I.R23 + 0x10) & 0xf0);
			break;
		case 0x63: /* INC */
			I.R23 = (I.R23 & 0xf0) | ((I.R23 + 1) & 0x0f);
			break;
		case 0x64: /* INC */
			I.R45 = (I.R45 & 0x0f) | ((I.R45 + 0x10) & 0xf0);
			break;
		case 0x65: /* INC */
			I.R45 = (I.R45 & 0xf0) | ((I.R45 + 1) & 0x0f);
			break;
		case 0x66: /* INC */
			I.R67 = (I.R67 & 0x0f) | ((I.R67 + 0x10) & 0xf0);
			break;
		case 0x67: /* INC */
			I.R67 = (I.R67 & 0xf0) | ((I.R67 + 1) & 0x0f);
			break;
		case 0x68: /* INC */
			I.R89 = (I.R89 & 0x0f) | ((I.R89 + 0x10) & 0xf0);
			break;
		case 0x69: /* INC */
			I.R89 = (I.R89 & 0xf0) | ((I.R89 + 1) & 0x0f);
			break;
		case 0x6a: /* INC */
			I.RAB = (I.RAB & 0x0f) | ((I.RAB + 0x10) & 0xf0);
			break;
		case 0x6b: /* INC */
			I.RAB = (I.RAB & 0xf0) | ((I.RAB + 1) & 0x0f);
			break;
		case 0x6c: /* INC */
			I.RCD = (I.RCD & 0x0f) | ((I.RCD + 0x10) & 0xf0);
			break;
		case 0x6d: /* INC */
			I.RCD = (I.RCD & 0xf0) | ((I.RCD + 1) & 0x0f);
			break;
		case 0x6e: /* INC */
			I.REF = (I.REF & 0x0f) | ((I.REF + 0x10) & 0xf0);
			break;
		case 0x6f: /* INC */
			I.REF = (I.REF & 0xf0) | ((I.REF + 1) & 0x0f);
			break;

		case 0x70: /* ISZ */
			I.R01 = (I.R01 & 0x0f) | ((I.R01 + 0x10) & 0xf0);
			M_JMP(romno, (I.R01 & 0xf0))
			break;
		case 0x71: /* ISZ */
			I.R01 = (I.R01 & 0xf0) | ((I.R01 + 1) & 0x0f);
			M_JMP(romno, (I.R01 & 0x0f))
			break;
		case 0x72: /* ISZ */
			I.R23 = (I.R23 & 0x0f) | ((I.R23 + 0x10) & 0xf0);
			M_JMP(romno, (I.R23 & 0xf0))
			break;
		case 0x73: /* ISZ */
			I.R23 = (I.R23 & 0xf0) | ((I.R23 + 1) & 0x0f);
			M_JMP(romno, (I.R23 & 0x0f))
			break;
		case 0x74: /* ISZ */
			I.R45 = (I.R45 & 0x0f) | ((I.R45 + 0x10) & 0xf0);
			M_JMP(romno, (I.R45 & 0xf0))
			break;
		case 0x75: /* ISZ */
			I.R45 = (I.R45 & 0xf0) | ((I.R45 + 1) & 0x0f);
			M_JMP(romno, (I.R45 & 0x0f))
			break;
		case 0x76: /* ISZ */
			I.R67 = (I.R67 & 0x0f) | ((I.R67 + 0x10) & 0xf0);
			M_JMP(romno, (I.R67 & 0xf0))
			break;
		case 0x77: /* ISZ */
			I.R67 = (I.R67 & 0xf0) | ((I.R67 + 1) & 0x0f);
			M_JMP(romno, (I.R67 & 0x0f))
			break;
		case 0x78: /* ISZ */
			I.R89 = (I.R89 & 0x0f) | ((I.R89 + 0x10) & 0xf0);
			M_JMP(romno, (I.R89 & 0xf0))
			break;
		case 0x79: /* ISZ */
			I.R89 = (I.R89 & 0xf0) | ((I.R89 + 1) & 0x0f);
			M_JMP(romno, (I.R89 & 0x0f))
			break;
		case 0x7a: /* ISZ */
			I.RAB = (I.RAB & 0x0f) | ((I.RAB + 0x10) & 0xf0);
			M_JMP(romno, (I.RAB & 0xf0))
			break;
		case 0x7b: /* ISZ */
			I.RAB = (I.RAB & 0xf0) | ((I.RAB + 1) & 0x0f);
			M_JMP(romno, (I.RAB & 0x0f))
			break;
		case 0x7c: /* ISZ */
			I.RCD = (I.RCD & 0x0f) | ((I.RCD + 0x10) & 0xf0);
			M_JMP(romno, (I.RCD & 0xf0))
			break;
		case 0x7d: /* ISZ */
			I.RCD = (I.RCD & 0xf0) | ((I.RCD + 1) & 0x0f);
			M_JMP(romno, (I.RCD & 0x0f))
			break;
		case 0x7e: /* ISZ */
			I.REF = (I.REF & 0x0f) | ((I.REF + 0x10) & 0xf0);
			M_JMP(romno, (I.REF & 0xf0))
			break;
		case 0x7f: /* ISZ */
			I.REF = (I.REF & 0xf0) | ((I.REF + 1) & 0x0f);
			M_JMP(romno, (I.REF & 0x0f))
			break;

		case 0x80: /* ADD */
			I.accu += (I.R01 >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x81: /* ADD */
			I.accu += (I.R01 & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x82: /* ADD */
			I.accu += (I.R23 >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x83: /* ADD */
			I.accu += (I.R23 & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x84: /* ADD */
			I.accu += (I.R45 >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x85: /* ADD */
			I.accu += (I.R45 & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x86: /* ADD */
			I.accu += (I.R67 >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x87: /* ADD */
			I.accu += (I.R67 & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x88: /* ADD */
			I.accu += (I.R89 >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x89: /* ADD */
			I.accu += (I.R89 & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8a: /* ADD */
			I.accu += (I.RAB >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8b: /* ADD */
			I.accu += (I.RAB & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8c: /* ADD */
			I.accu += (I.RCD >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8d: /* ADD */
			I.accu += (I.RCD & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8e: /* ADD */
			I.accu += (I.REF >> 4);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0x8f: /* ADD */
			I.accu += (I.REF & 0x0f);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;

		case 0x90: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R01 >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x91: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R01 & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x92: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R23 >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x93: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R23 & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x94: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R45 >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x95: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R45 & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x96: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R67 >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x97: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R67 & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x98: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R89 >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x99: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.R89 & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9a: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.RAB >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9b: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.RAB & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9c: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.RCD >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9d: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.RCD & 0x0f)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9e: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.REF >> 4)) & 0x0f;
			I.carry = 0;
			break;
		case 0x9f: /* SUB */
			I.accu = (((I.carry << 4) | I.accu) - (I.REF & 0x0f)) & 0x0f;
			I.carry = 0;
			break;

		case 0xa0: /* LD */
			I.accu = I.R01 >> 4;
			break;
		case 0xa1: /* LD */
			I.accu = I.R01 & 0x0f;
			break;
		case 0xa2: /* LD */
			I.accu = I.R23 >> 4;
			break;
		case 0xa3: /* LD */
			I.accu = I.R23 & 0x0f;
			break;
		case 0xa4: /* LD */
			I.accu = I.R45 >> 4;
			break;
		case 0xa5: /* LD */
			I.accu = I.R45 & 0x0f;
			break;
		case 0xa6: /* LD */
			I.accu = I.R67 >> 4;
			break;
		case 0xa7: /* LD */
			I.accu = I.R67 & 0x0f;
			break;
		case 0xa8: /* LD */
			I.accu = I.R89 >> 4;
			break;
		case 0xa9: /* LD */
			I.accu = I.R89 & 0x0f;
			break;
		case 0xaa: /* LD */
			I.accu = I.RAB >> 4;
			break;
		case 0xab: /* LD */
			I.accu = I.RAB & 0x0f;
			break;
		case 0xac: /* LD */
			I.accu = I.RCD >> 4;
			break;
		case 0xad: /* LD */
			I.accu = I.RCD & 0x0f;
			break;
		case 0xae: /* LD */
			I.accu = I.REF >> 4;
			break;
		case 0xaf: /* LD */
			I.accu = I.REF & 0x0f;
			break;

		case 0xb0: /* XCH */
			tmp = I.accu;
			I.accu = I.R01 >> 4;
			I.R01 = (I.R01 & 0x0f) | (tmp << 4);
			break;
		case 0xb1: /* XCH */
			tmp = I.accu;
			I.accu = I.R01 & 0x0f;
			I.R01 = (I.R01 & 0xf0) | tmp;
			break;
		case 0xb2: /* XCH */
			tmp = I.accu;
			I.accu = I.R23 >> 4;
			I.R23 = (I.R23 & 0x0f) | (tmp << 4);
			break;
		case 0xb3: /* XCH */
			tmp = I.accu;
			I.accu = I.R23 & 0x0f;
			I.R23 = (I.R23 & 0xf0) | tmp;
			break;
		case 0xb4: /* XCH */
			tmp = I.accu;
			I.accu = I.R45 >> 4;
			I.R45 = (I.R45 & 0x0f) | (tmp << 4);
			break;
		case 0xb5: /* XCH */
			tmp = I.accu;
			I.accu = I.R45 & 0x0f;
			I.R45 = (I.R45 & 0xf0) | tmp;
			break;
		case 0xb6: /* XCH */
			tmp = I.accu;
			I.accu = I.R67 >> 4;
			I.R67 = (I.R67 & 0x0f) | (tmp << 4);
			break;
		case 0xb7: /* XCH */
			tmp = I.accu;
			I.accu = I.R67 & 0x0f;
			I.R67 = (I.R67 & 0xf0) | tmp;
			break;
		case 0xb8: /* XCH */
			tmp = I.accu;
			I.accu = I.R89 >> 4;
			I.R89 = (I.R89 & 0x0f) | (tmp << 4);
			break;
		case 0xb9: /* XCH */
			tmp = I.accu;
			I.accu = I.R89 & 0x0f;
			I.R89 = (I.R89 & 0xf0) | tmp;
			break;
		case 0xba: /* XCH */
			tmp = I.accu;
			I.accu = I.RAB >> 4;
			I.RAB = (I.RAB & 0x0f) | (tmp << 4);
			break;
		case 0xbb: /* XCH */
			tmp = I.accu;
			I.accu = I.RAB & 0x0f;
			I.RAB = (I.RAB & 0xf0) | tmp;
			break;
		case 0xbc: /* XCH */
			tmp = I.accu;
			I.accu = I.RCD >> 4;
			I.RCD = (I.RCD & 0x0f) | (tmp << 4);
			break;
		case 0xbd: /* XCH */
			tmp = I.accu;
			I.accu = I.RCD & 0x0f;
			I.RCD = (I.RCD & 0xf0) | tmp;
			break;
		case 0xbe: /* XCH */
			tmp = I.accu;
			I.accu = I.REF >> 4;
			I.REF = (I.REF & 0x0f) | (tmp << 4);
			break;
		case 0xbf: /* XCH */
			tmp = I.accu;
			I.accu = I.REF & 0x0f;
			I.REF = (I.REF & 0xf0) | tmp;
			break;

		/* BBL */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3: case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd: case 0xce: case 0xcf:
			I.PC = I.S1;
			I.S1 = I.S2;
			I.S2 = I.S3;
			I.S3.d = 0;
			change_pc16(I.PC.d);

		/* LDM */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3: case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb: case 0xdc: case 0xdd: case 0xde: case 0xdf:
			I.accu = opcode & 0x0f;
			break;

		case 0xe0: /* WRM */
			WM(0x1000 | I.ramaddr.d, I.accu);
			break;
		case 0xe1: /* WMP */
			M_OUT(1)
			break;
		case 0xe2: /* WRR */
			M_OUT(0)
			break;
		case 0xe3: /* WPM */
			M_OUT(2)
			break;
		case 0xe4: /* WR0 */
			WM(0x2000 | (I.ramaddr.w.l & 0xff0), I.accu);
			break;
		case 0xe5: /* WR1 */
			WM(0x2001 | (I.ramaddr.w.l & 0xff0), I.accu);
			break;
		case 0xe6: /* WR2 */
			WM(0x2002 | (I.ramaddr.w.l & 0xff0), I.accu);
			break;
		case 0xe7: /* WR3 */
			WM(0x2003 | (I.ramaddr.w.l & 0xff0), I.accu);
			break;
		case 0xe8: /* SBM */
			I.accu = (((I.carry << 4) | I.accu) - RM(0x1000 | I.ramaddr.d)) & 0x0f;
			I.carry = 0;
			break;
		case 0xe9: /* RDM */
			I.accu = RM(0x1000 | I.ramaddr.d);
			break;
		case 0xea: /* RDR */
			M_IN
			break;
		case 0xeb: /* ADM */
			I.accu += RM(0x1000 | I.ramaddr.d);
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0xec: /* RD0 */
			I.accu = RM(0x2000 | (I.ramaddr.w.l & 0xff0));
			break;
		case 0xed: /* RD1 */
			I.accu = RM(0x2001 | (I.ramaddr.w.l & 0xff0));
			break;
		case 0xee: /* RD2 */
			I.accu = RM(0x2002 | (I.ramaddr.w.l & 0xff0));
			break;
		case 0xef: /* RD3 */
			I.accu = RM(0x2003 | (I.ramaddr.w.l & 0xff0));
			break;

		case 0xf0: /* CLB */
			I.accu = 0;
		case 0xf1: /* CLC */
			I.carry = 0;
			break;
		case 0xf2: /* IAC */
			I.carry = (I.accu == 0x0f);
			I.accu = (I.accu + 1) & 0x0f;
			break;
		case 0xf3: /* CMC */
			I.carry = I.carry ? 0 : 1;
			break;
		case 0xf4: /* CMA */
			I.accu = ~I.accu & 0x0f;
			break;
		case 0xf5: /* RAL */
			I.accu = (I.accu << 1) | I.carry;
			I.carry = I.accu >> 4;
			I.accu &= 0x0f;
			break;
		case 0xf6: /* RAR */
			tmp = I.accu & 0x01;
			I.accu = (I.accu >> 1) | (I.carry << 3);
			I.carry = tmp;
			break;
		case 0xf7: /* TCC */
			I.accu = I.carry;
			I.carry = 0;
			break;
		case 0xf8: /* DAC */
			if (I.accu == 0) {
				I.accu = 0x0f;
				I.carry = 1;
			} else {
				I.accu--;
				I.carry = 0;
			}
			break;
		case 0xf9: /* TCS */
			I.accu = 9 + I.carry;
			I.carry = 0;
			break;
		case 0xfa: /* STC */
			I.carry = 1;
			break;
		case 0xfb: /* DAA */
			if (I.accu > 9 || I.carry) {
				I.accu += 6;
				if (I.accu & ~0x0f) I.carry = 1 - I.carry;
			}
			I.accu &= 0x0f;
			break;
		case 0xfc: /* KBP */
			if (I.accu > 2) {
				if (I.accu == 4) I.accu = 3;
				else if (I.accu == 8) I.accu = 4;
				else I.accu = 0x0f;
			}
			break;
		case 0xfd: /* DCL */
			I.ramaddr.b.h = I.accu;
			break;

		default:
			illegal();
	}
}

int i4004_execute(int cycles)
{
	i4004_ICount = cycles;
	do
	{
		CALL_MAME_DEBUG;
		/* here we go... */
		execute_one(ROP());

	} while (i4004_ICount > 0);

	return cycles - i4004_ICount;
}

/****************************************************************************
 * Init the 4004 emulation
 ****************************************************************************/
void i4004_init(void)
{
	int cpu = cpu_getactivecpu();
	I.cputype = 0;

	state_save_register_UINT8("i4004", cpu, "EF", &I.REF, 1);
	state_save_register_UINT8("i4004", cpu, "CD", &I.RCD, 1);
	state_save_register_UINT8("i4004", cpu, "AB", &I.RAB, 1);
	state_save_register_UINT8("i4004", cpu, "89", &I.R89, 1);
	state_save_register_UINT8("i4004", cpu, "67", &I.R67, 1);
	state_save_register_UINT8("i4004", cpu, "45", &I.R45, 1);
	state_save_register_UINT8("i4004", cpu, "23", &I.R23, 1);
	state_save_register_UINT8("i4004", cpu, "01", &I.R01, 1);
	state_save_register_UINT16("i4004", cpu, "S1", &I.S1.w.l, 1);
	state_save_register_UINT16("i4004", cpu, "S2", &I.S2.w.l, 1);
	state_save_register_UINT16("i4004", cpu, "S3", &I.S3.w.l, 1);
	state_save_register_UINT16("i4004", cpu, "PC", &I.PC.w.l, 1);
	state_save_register_UINT16("i4004", cpu, "RAM", &I.ramaddr.w.l, 1);
	state_save_register_INT8("i4004", cpu, "ACCU", &I.accu, 1);
	state_save_register_INT8("i4004", cpu, "CARRY", &I.carry, 1);
	state_save_register_INT8("i4004", cpu, "TEST", &I.test, 1);
}

/****************************************************************************
 * Reset the 4004 emulation
 ****************************************************************************/
void i4004_reset(void *param)
{
	int testSave = I.test;
	memset(&I, 0, sizeof(i4004_Regs));
	I.test = testSave;
	change_pc16(I.PC.d);
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void i4004_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get the current 4004 context
 ****************************************************************************/
unsigned i4004_get_context(void *dst)
{
	if( dst )
		*(i4004_Regs*)dst = I;
	return sizeof(i4004_Regs);
}

/****************************************************************************
 * Set the current 4004 context
 ****************************************************************************/
void i4004_set_context(void *src)
{
	if( src )
	{
		int testSave = I.test;
		I = *(i4004_Regs*)src;
		change_pc16(I.PC.d);
		I.test = testSave;
	}
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned i4004_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return I.PC.d;
		case I4004_PC: return I.PC.w.l;
		case REG_SP: return I.S1.d;
		case I4004_S1: return I.S1.w.l;
		case I4004_S2: return I.S2.w.l;
		case I4004_S3: return I.S3.w.l;
		case I4004_RAM: return I.ramaddr.w.l;
		case I4004_EF: return I.REF;
		case I4004_CD: return I.RCD;
		case I4004_AB: return I.RAB;
		case I4004_89: return I.R89;
		case I4004_67: return I.R67;
		case I4004_45: return I.R45;
		case I4004_23: return I.R23;
		case I4004_01: return I.R01;
		case I4004_A: return I.accu;
		case I4004_C: return I.carry;
		case I4004_T: return I.test;
		case REG_PREVIOUSPC: return 0; /* previous pc not supported */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.S1.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return RM( offset ) + ( RM( offset+1 ) << 8 );
			}
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void i4004_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: I.PC.w.l = val; change_pc16(I.PC.d); break;
		case I4004_PC: I.PC.w.l = val; break;
		case REG_SP: I.S1.w.l = val; break;
		case I4004_S1: I.S1.w.l = val; break;
		case I4004_S2: I.S2.w.l = val; break;
		case I4004_S3: I.S3.w.l = val; break;
		case I4004_RAM: I.ramaddr.w.l = val; break;
		case I4004_EF: I.REF = val; break;
		case I4004_CD: I.RCD = val; break;
		case I4004_AB: I.RAB = val; break;
		case I4004_89: I.R89 = val; break;
		case I4004_67: I.R67 = val; break;
		case I4004_45: I.R45 = val; break;
		case I4004_23: I.R23 = val; break;
		case I4004_01: I.R01 = val; break;
		case I4004_A: I.accu = (val & 0x0f); break;
		case I4004_C: I.carry = (val > 0); break;
		case I4004_T: I.test = (val > 0); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = I.S1.w.l + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, val&0xff );
					WM( offset+1, (val>>8)&0xff );
				}
			}
	}
}

/****************************************************************************/
/* Set TEST signal state													*/
/****************************************************************************/
void i4004_set_TEST(int state)
{
	LOG(("i4004: TEST %d\n", state));
	I.test = (state > 0);
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i4004_info(void *context, int regnum)
{
	static char buffer[17][47+1];
	static int which = 0;
	i4004_Regs *r = context;

	which = (which+1) % 17;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+I4004_PC: sprintf(buffer[which], "PC:%03X", r->PC.w.l); break;
		case CPU_INFO_REG+I4004_S1: sprintf(buffer[which], "S1:%03X", r->S1.w.l); break;
		case CPU_INFO_REG+I4004_S2: sprintf(buffer[which], "S2:%03X", r->S2.w.l); break;
		case CPU_INFO_REG+I4004_S3: sprintf(buffer[which], "S3:%03X", r->S3.w.l); break;
		case CPU_INFO_REG+I4004_EF: sprintf(buffer[which], "EF:%02X", r->REF); break;
		case CPU_INFO_REG+I4004_CD: sprintf(buffer[which], "CD:%02X", r->RCD); break;
		case CPU_INFO_REG+I4004_AB: sprintf(buffer[which], "AB:%02X", r->RAB); break;
		case CPU_INFO_REG+I4004_89: sprintf(buffer[which], "89:%02X", r->R89); break;
		case CPU_INFO_REG+I4004_67: sprintf(buffer[which], "67:%02X", r->R67); break;
		case CPU_INFO_REG+I4004_45: sprintf(buffer[which], "45:%02X", r->R45); break;
		case CPU_INFO_REG+I4004_23: sprintf(buffer[which], "23:%02X", r->R23); break;
		case CPU_INFO_REG+I4004_01: sprintf(buffer[which], "01:%02X", r->R01); break;
		case CPU_INFO_REG+I4004_RAM: sprintf(buffer[which], "RA:%03X", r->ramaddr.w.l); break;
		case CPU_INFO_REG+I4004_A: sprintf(buffer[which], "ACC:%X", I.accu); break;
		case CPU_INFO_REG+I4004_C: sprintf(buffer[which], "CRY:%X", I.carry); break;
		case CPU_INFO_REG+I4004_T: sprintf(buffer[which], "TST:%X", I.test); break;
		case CPU_INFO_FLAGS: sprintf(buffer[which], "%c", (I.carry ? 'C' : '.')); break;
		case CPU_INFO_NAME: return "4004";
		case CPU_INFO_FAMILY: return "Intel 4004";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "written 2002 by G. Volkenborn";
		case CPU_INFO_REG_LAYOUT: return (const char *)i4004_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)i4004_win_layout;
	}
	return buffer[which];
}

void i4004_set_irq_line(int irqline, int state) {
	/* no IRQ line on the 4004 */
}

void i4004_set_irq_callback(int (*callback)(int)) {
	/* no IRQ line on the 4004 */
}

unsigned i4004_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm4004(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
