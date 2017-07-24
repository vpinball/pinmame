#define VERBOSE 0

#include "driver.h"
#include "state.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "scamp.h"

#if VERBOSE
#include <stdio.h>
#include "driver.h"
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

/* Layout of the registers in the debugger */
static UINT8 SCAMP_reg_layout[] = {
	SCAMP_PC, SCAMP_P1, SCAMP_P2, SCAMP_P3, SCAMP_AC, SCAMP_EX, SCAMP_ST, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 SCAMP_win_layout[] = {
	25, 0,55, 1,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 2,55,10,	/* memory #1 window (right, upper middle) */
	25,13,55, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

typedef struct {
	PAIR	PC, P1, P2, P3;
	UINT8	accu, ereg, sreg;
}	SCAMP_Regs;

static SCAMP_Regs I;
int SCAMP_ICount = 0;

/* Cycles count for all opcodes */
static int op_cycles[] = {
	8, 7, 5, 5, 6, 6, 5, 6, 5, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 1, 1, 5, 5, 5, 5,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	8, 8, 8, 8, 8, 8, 8, 8, 1, 1, 1, 1, 7, 7, 7, 7,
	6, 1, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1,
	6, 1, 1, 1, 1, 1, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1,
	6, 1, 1, 1, 1, 1, 1, 1,11, 1, 1, 1, 1, 1, 1, 1,
	7, 1, 1, 1, 1, 1, 1, 1, 8, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,13,
	9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
	1, 1, 1, 1, 1, 1, 1, 1,22,22,22,22, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,22,22,22,22, 1, 1, 1, 1,
   10,10,10,10,18,18,18,18,10,10,10,10,18,18,18,18,
   10,10,10,10,18,18,18,18,10,10,10,10,18,18,18,18,
   10,10,10,10,18,18,18,18,15,15,15,15,23,23,23,23,
   11,11,11,11,19,19,19,19,12,12,12,12,20,20,20,20
};

static UINT8 ROP(void)
{
	I.PC.w.l = ((I.PC.w.l + 1) & 0x0fff) | (I.PC.w.l & 0xf000);
	return cpu_readop(I.PC.w.l);
}

static UINT8 ARG(void)
{
	I.PC.w.l = ((I.PC.w.l + 1) & 0x0fff) | (I.PC.w.l & 0xf000);
	return cpu_readop_arg(I.PC.w.l);
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
	UINT16 pc = I.PC.w.l;
	LOG(("SC/MP illegal instruction %04X $%02X\n", pc, cpu_readop(pc)));
#endif
}

INLINE void execute_one(int opcode)
{
	PAIR   tmpPC;
	UINT32 tmp32;
	UINT16 tmp16;
	UINT8  tmp = 0;

	if (opcode > 0x8e)
		tmp = ARG();
	if (opcode > 0x8f && (opcode & 0x0f) != 0x0c && (opcode & 0x0f) != 0x04)
		tmp = (tmp == 0x80) ? I.ereg : tmp;

	switch (opcode)
	{
		case 0x00: /* HALT */
			// pulse H Output... what pin is that one?
			cpu_writeport16(SCAMP_H_PORT, 1);
			cpu_writeport16(SCAMP_H_PORT, 0);
			break;
		case 0x01: /* XAE */
			tmp = I.accu;
			I.accu = I.ereg;
			I.ereg = tmp;
			break;
		case 0x02: /* CCL */
			I.sreg &= 0x7f;
			break;
		case 0x03: /* SCL */
			I.sreg |= 0x80;
			break;
		case 0x04: /* DINT */
			I.sreg &= 0xf7;
			break;
		case 0x05: /* IEN */
			I.sreg |= 0x08;
			break;
		case 0x06: /* CSA */
			I.accu = I.sreg;
			break;
		case 0x07: /* CSA */
			tmp = (I.sreg & 0x30) | (I.accu & 0xcf);
			if ((tmp & 0x07) != (I.sreg & 0x07))
				cpu_writeport16(SCAMP_FLAG_PORT, tmp & 0x07);
			I.sreg = tmp;
			break;
		case 0x08: /* NOP */
			break;

		case 0x19: /* SIO */
			cpu_writeport16(SCAMP_SERIAL_PORT, I.ereg & 0x01);
			I.ereg = (I.ereg >> 1) | ((cpu_readport16(SCAMP_SERIAL_PORT) > 0) << 7);
			break;
		case 0x1c: /* SR */
			I.accu >>= 1;
			break;
		case 0x1d: /* SRL */
			I.accu = (I.accu >> 1) | (I.sreg & 0x80);
			break;
		case 0x1e: /* RR */
			I.accu = ((I.accu & 0x01) << 7) | (I.accu >> 1);
			break;
		case 0x1f: /* RRL */
			tmp = I.accu & 0x01;
			I.accu = (I.accu >> 1) | (I.sreg & 0x80);
			I.sreg = (I.sreg & 0x7f) | (tmp << 7);
			break;

		case 0x30: /* XPAL PC */
			tmp = I.PC.w.l & 0xff;
			I.PC.w.l = (I.PC.w.l & 0xff00) | I.accu;
			I.accu = tmp;
			break;
		case 0x31: /* XPAL P1 */
			tmp = I.P1.w.l & 0xff;
			I.P1.w.l = (I.P1.w.l & 0xff00) | I.accu;
			I.accu = tmp;
			break;
		case 0x32: /* XPAL P2 */
			tmp = I.P2.w.l & 0xff;
			I.P2.w.l = (I.P2.w.l & 0xff00) | I.accu;
			I.accu = tmp;
			break;
		case 0x33: /* XPAL P3 */
			tmp = I.P3.w.l & 0xff;
			I.P3.w.l = (I.P3.w.l & 0xff00) | I.accu;
			I.accu = tmp;
			break;

		case 0x34: /* XPAH PC */
			tmp = I.PC.w.l >> 8;
			I.PC.w.l = (I.PC.w.l & 0x00ff) | (I.accu << 8);
			I.accu = tmp;
			break;
		case 0x35: /* XPAH P1 */
			tmp = I.P1.w.l >> 8;
			I.P1.w.l = (I.P1.w.l & 0x00ff) | (I.accu << 8);
			I.accu = tmp;
			break;
		case 0x36: /* XPAH P2 */
			tmp = I.P2.w.l >> 8;
			I.P2.w.l = (I.P2.w.l & 0x00ff) | (I.accu << 8);
			I.accu = tmp;
			break;
		case 0x37: /* XPAH P3 */
			tmp = I.P3.w.l >> 8;
			I.P3.w.l = (I.P3.w.l & 0x00ff) | (I.accu << 8);
			I.accu = tmp;
			break;

		case 0x3c: /* XPPC PC */
		    // does nothing.
			break;
		case 0x3d: /* XPPC P1 */
			tmpPC = I.PC;
			I.PC = I.P1;
			I.P1 = tmpPC;
			break;
		case 0x3e: /* XPPC P2 */
			tmpPC = I.PC;
			I.PC = I.P2;
			I.P2 = tmpPC;
			break;
		case 0x3f: /* XPPC P3 */
			tmpPC = I.PC;
			I.PC = I.P3;
			I.P3 = tmpPC;
			break;

		case 0x40: /* LDE */
			I.accu = I.ereg;
			break;
		case 0x48: /* STE ? */
			I.ereg = I.accu;
			break;

		case 0x50: /* ANDE */
			I.accu &= I.ereg;
			break;
		case 0x58: /* ORE */
			I.accu |= I.ereg;
			break;

		case 0x60: /* XORE */
			I.accu ^= I.ereg;
			break;
		case 0x68: /* DADE */
			tmp16 = I.accu + I.ereg + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;

		case 0x70: /* ADDE */
			tmp16 = I.accu + I.ereg + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0x78: /* CADE */
			tmp16 = I.accu + ~I.ereg + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;

		case 0x8f: /* DLY */
			tmp32 = tmp;
			SCAMP_ICount -= ((tmp32 << 9) | (I.accu << 1)) + (tmp32 << 1);
			I.accu = 0xff;
			break;

		case 0x90: /* JMP */
			I.PC.w.l += (INT8)tmp;
			break;
		case 0x91: /* JMP P1 */
			I.PC.w.l = I.P1.w.l + (INT8)tmp;
			break;
		case 0x92: /* JMP P2 */
			I.PC.w.l = I.P2.w.l + (INT8)tmp;
			break;
		case 0x93: /* JMP P3 */
			I.PC.w.l = I.P3.w.l + (INT8)tmp;
			break;
		case 0x94: /* JP */
			if (!(I.accu & 0x80)) {
				I.PC.w.l += (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x95: /* JP P1 */
			if (!(I.accu & 0x80)) {
				I.PC.w.l = I.P1.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x96: /* JP P2 */
			if (!(I.accu & 0x80)) {
				I.PC.w.l = I.P2.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x97: /* JP P3 */
			if (!(I.accu & 0x80)) {
				I.PC.w.l = I.P3.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x98: /* JZ */
			if (!I.accu) {
				I.PC.w.l += (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x99: /* JZ P1 */
			if (!I.accu) {
				I.PC.w.l = I.P1.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9a: /* JZ P2 */
			if (!I.accu) {
				I.PC.w.l = I.P2.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9b: /* JZ P3 */
			if (!I.accu) {
				I.PC.w.l = I.P3.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9c: /* JNZ */
			if (I.accu) {
				I.PC.w.l += (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9d: /* JNZ P1 */
			if (I.accu) {
				I.PC.w.l = I.P1.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9e: /* JNZ P2 */
			if (I.accu) {
				I.PC.w.l = I.P2.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;
		case 0x9f: /* JNZ P3 */
			if (I.accu) {
				I.PC.w.l = I.P3.w.l + (INT8)tmp;
				SCAMP_ICount -= 2;
			}
			break;

		case 0xa8: /* ILD PC */
			tmpPC.w.l = I.PC.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) + 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xa9: /* ILD P1 */
			tmpPC.w.l = I.P1.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) + 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xaa: /* ILD P2 */
			tmpPC.w.l = I.P2.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) + 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xab: /* ILD P3 */
			tmpPC.w.l = I.P3.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) + 1;
			WM(tmpPC.w.l, I.accu);
			break;

		case 0xb8: /* DLD PC */
			tmpPC.w.l = I.PC.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) - 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xb9: /* DLD P1 */
			tmpPC.w.l = I.P1.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) - 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xba: /* DLD P2 */
			tmpPC.w.l = I.P2.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) - 1;
			WM(tmpPC.w.l, I.accu);
			break;
		case 0xbb: /* DLD P3 */
			tmpPC.w.l = I.P3.w.l + (INT8)tmp;
			I.accu = RM(tmpPC.w.l) - 1;
			WM(tmpPC.w.l, I.accu);
			break;

		case 0xc0: /* LD PC */
			I.accu = RM(I.PC.w.l + (INT8)tmp);
			break;
		case 0xc1: /* LD P1 */
			I.accu = RM(I.P1.w.l + (INT8)tmp);
			break;
		case 0xc2: /* LD P2 */
			I.accu = RM(I.P2.w.l + (INT8)tmp);
			break;
		case 0xc3: /* LD P3 */
			I.accu = RM(I.P3.w.l + (INT8)tmp);
			break;
		case 0xc4: /* LDI */
			I.accu = tmp;
			break;
		case 0xc5: /* LDA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				I.accu = RM(I.P1.w.l);
			} else {
				I.accu = RM(I.P1.w.l);
				I.P1.w.l += tmp;
			}
			break;
		case 0xc6: /* LDA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				I.accu = RM(I.P2.w.l);
			} else {
				I.accu = RM(I.P2.w.l);
				I.P2.w.l += tmp;
			}
			break;
		case 0xc7: /* LDA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				I.accu = RM(I.P3.w.l);
			} else {
				I.accu = RM(I.P3.w.l);
				I.P3.w.l += tmp;
			}
			break;
		case 0xc8: /* ST PC */
			WM(I.PC.w.l + (INT8)tmp, I.accu);
			break;
		case 0xc9: /* ST P1 */
			WM(I.P1.w.l + (INT8)tmp, I.accu);
			break;
		case 0xca: /* ST P2 */
			WM(I.P2.w.l + (INT8)tmp, I.accu);
			break;
		case 0xcb: /* ST P3 */
			WM(I.P3.w.l + (INT8)tmp, I.accu);
			break;
		case 0xcc: /* STA PC ? */
			if (tmp & 0x80) {
				I.PC.w.l += (INT8)tmp;
				WM(I.PC.w.l, I.accu);
			} else {
				WM(I.PC.w.l, I.accu);
				I.PC.w.l += tmp;
			}
			break;
		case 0xcd: /* STA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				WM(I.P1.w.l, I.accu);
			} else {
				WM(I.P1.w.l, I.accu);
				I.P1.w.l += tmp;
			}
			break;
		case 0xce: /* STA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				WM(I.P2.w.l, I.accu);
			} else {
				WM(I.P2.w.l, I.accu);
				I.P2.w.l += tmp;
			}
			break;
		case 0xcf: /* STA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				WM(I.P3.w.l, I.accu);
			} else {
				WM(I.P3.w.l, I.accu);
				I.P3.w.l += tmp;
			}
			break;

		case 0xd0: /* AND PC */
			I.accu &= RM(I.PC.w.l + (INT8)tmp);
			break;
		case 0xd1: /* AND P1 */
			I.accu &= RM(I.P1.w.l + (INT8)tmp);
			break;
		case 0xd2: /* AND P2 */
			I.accu &= RM(I.P2.w.l + (INT8)tmp);
			break;
		case 0xd3: /* AND P3 */
			I.accu &= RM(I.P3.w.l + (INT8)tmp);
			break;
		case 0xd4: /* ANDI */
			I.accu &= tmp;
			break;
		case 0xd5: /* ANDA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				I.accu &= RM(I.P1.w.l);
			} else {
				I.accu &= RM(I.P1.w.l);
				I.P1.w.l += tmp;
			}
			break;
		case 0xd6: /* ANDA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				I.accu &= RM(I.P2.w.l);
			} else {
				I.accu &= RM(I.P2.w.l);
				I.P2.w.l += tmp;
			}
			break;
		case 0xd7: /* ANDA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				I.accu &= RM(I.P3.w.l);
			} else {
				I.accu &= RM(I.P3.w.l);
				I.P3.w.l += tmp;
			}
			break;
		case 0xd8: /* OR PC */
			I.accu |= RM(I.PC.w.l + (INT8)tmp);
			break;
		case 0xd9: /* OR P1 */
			I.accu |= RM(I.P1.w.l + (INT8)tmp);
			break;
		case 0xda: /* OR P2 */
			I.accu |= RM(I.P2.w.l + (INT8)tmp);
			break;
		case 0xdb: /* OR P3 */
			I.accu |= RM(I.P3.w.l + (INT8)tmp);
			break;
		case 0xdc: /* ORI */
			I.accu |= tmp;
			break;
		case 0xdd: /* ORA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				I.accu |= RM(I.P1.w.l);
			} else {
				I.accu |= RM(I.P1.w.l);
				I.P1.w.l += tmp;
			}
			break;
		case 0xde: /* ORA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				I.accu |= RM(I.P2.w.l);
			} else {
				I.accu |= RM(I.P2.w.l);
				I.P2.w.l += tmp;
			}
			break;
		case 0xdf: /* ORA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				I.accu |= RM(I.P3.w.l);
			} else {
				I.accu |= RM(I.P3.w.l);
				I.P3.w.l += tmp;
			}
			break;

		case 0xe0: /* XOR PC */
			I.accu ^= RM(I.PC.w.l + (INT8)tmp);
			break;
		case 0xe1: /* XOR P1 */
			I.accu ^= RM(I.P1.w.l + (INT8)tmp);
			break;
		case 0xe2: /* XOR P2 */
			I.accu ^= RM(I.P2.w.l + (INT8)tmp);
			break;
		case 0xe3: /* XOR P3 */
			I.accu ^= RM(I.P3.w.l + (INT8)tmp);
			break;
		case 0xe4: /* XORI */
			I.accu ^= tmp;
			break;
		case 0xe5: /* XORA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				I.accu ^= RM(I.P1.w.l);
			} else {
				I.accu ^= RM(I.P1.w.l);
				I.P1.w.l += tmp;
			}
			break;
		case 0xe6: /* XORA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				I.accu ^= RM(I.P2.w.l);
			} else {
				I.accu ^= RM(I.P2.w.l);
				I.P2.w.l += tmp;
			}
			break;
		case 0xe7: /* XORA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				I.accu ^= RM(I.P3.w.l);
			} else {
				I.accu ^= RM(I.P3.w.l);
				I.P3.w.l += tmp;
			}
			break;
		case 0xe8: /* DAD PC */
			tmp16 = I.accu + RM(I.PC.w.l + (INT8)tmp) + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xe9: /* DAD P1 */
			tmp16 = I.accu + RM(I.P1.w.l + (INT8)tmp) + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xea: /* DAD P2 */
			tmp16 = I.accu + RM(I.P2.w.l + (INT8)tmp) + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xeb: /* DAD P3 */
			tmp16 = I.accu + RM(I.P3.w.l + (INT8)tmp) + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xec: /* DADI */
			tmp16 = I.accu + tmp + (I.sreg >> 7);
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xed: /* DADA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P1.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P1.w.l) + (I.sreg >> 7);
				I.P1.w.l += tmp;
			}
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xee: /* DADA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P2.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P2.w.l) + (I.sreg >> 7);
				I.P2.w.l += tmp;
			}
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;
		case 0xef: /* DADA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P3.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P3.w.l) + (I.sreg >> 7);
				I.P3.w.l += tmp;
			}
			if ((tmp16 & 0x0f) > 9) tmp16 += 6;
			I.accu = tmp16 % 0xa0;
			I.sreg = (I.sreg & 0x7f) | (tmp16 > 0x99 ? 0x80 : 0);
			break;

		case 0xf0: /* ADD PC */
			tmp16 = I.accu + RM(I.PC.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf1: /* ADD P1 */
			tmp16 = I.accu + RM(I.P1.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf2: /* ADD P2 */
			tmp16 = I.accu + RM(I.P2.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf3: /* ADD P3 */
			tmp16 = I.accu + RM(I.P3.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf4: /* ADDI */
			tmp16 = I.accu + tmp + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf5: /* ADDA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P1.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P1.w.l) + (I.sreg >> 7);
				I.P1.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf6: /* ADDA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P2.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P2.w.l) + (I.sreg >> 7);
				I.P2.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf7: /* ADDA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				tmp16 = I.accu + RM(I.P3.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + RM(I.P3.w.l) + (I.sreg >> 7);
				I.P3.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf8: /* CAD PC */
			tmp16 = I.accu + ~RM(I.PC.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xf9: /* CAD P1 */
			tmp16 = I.accu + ~RM(I.P1.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xfa: /* CAD P2 */
			tmp16 = I.accu + ~RM(I.P2.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xfb: /* CAD P3 */
			tmp16 = I.accu + ~RM(I.P3.w.l + (INT8)tmp) + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xfc: /* CADI */
			tmp16 = I.accu + ~tmp + (I.sreg >> 7);
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xfd: /* CADA P1 */
			if (tmp & 0x80) {
				I.P1.w.l += (INT8)tmp;
				tmp16 = I.accu + ~RM(I.P1.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + ~RM(I.P1.w.l) + (I.sreg >> 7);
				I.P1.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xfe: /* CADA P2 */
			if (tmp & 0x80) {
				I.P2.w.l += (INT8)tmp;
				tmp16 = I.accu + ~RM(I.P2.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + ~RM(I.P2.w.l) + (I.sreg >> 7);
				I.P2.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;
		case 0xff: /* CADA P3 */
			if (tmp & 0x80) {
				I.P3.w.l += (INT8)tmp;
				tmp16 = I.accu + ~RM(I.P3.w.l) + (I.sreg >> 7);
			} else {
				tmp16 = I.accu + ~RM(I.P3.w.l) + (I.sreg >> 7);
				I.P3.w.l += tmp;
			}
			I.accu = tmp16 & 0xff;
			I.sreg = (I.sreg & 0x3f) | ((tmp16 & 0x100) ? 0xc0 : 0);
			break;

		default:
			illegal();
	}
	SCAMP_ICount -= op_cycles[opcode];
}

int SCAMP_execute(int cycles)
{
	PAIR   tmpPC;
	if ((I.sreg & 0x18) == 0x18) { // do the interrupt routine
		tmpPC = I.PC;
		I.PC = I.P3;
		I.P3 = tmpPC;
		I.sreg &= 0xf7;
	}

	SCAMP_ICount = cycles;
	do
	{
		CALL_MAME_DEBUG;
		/* here we go... */
		execute_one(ROP());

	} while (SCAMP_ICount > 0);

	return cycles - SCAMP_ICount;
}

/****************************************************************************
 * Init the SC/MP emulation
 ****************************************************************************/
void SCAMP_init(void)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT16("SCAMP", cpu, "PC", &I.PC.w.l, 1);
	state_save_register_UINT16("SCAMP", cpu, "P1", &I.P1.w.l, 1);
	state_save_register_UINT16("SCAMP", cpu, "P2", &I.P2.w.l, 1);
	state_save_register_UINT16("SCAMP", cpu, "P3", &I.P3.w.l, 1);
	state_save_register_UINT8("SCAMP", cpu, "ACCU", &I.accu, 1);
	state_save_register_UINT8("SCAMP", cpu, "EXT", &I.ereg, 1);
	state_save_register_UINT8("SCAMP", cpu, "STATUS", &I.sreg, 1);
}

/****************************************************************************
 * Reset the SC/MP emulation
 ****************************************************************************/
void SCAMP_reset(void *param)
{
	memset(&I, 0, sizeof(SCAMP_Regs));
	change_pc16(I.PC.d);
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void SCAMP_exit(void)
{
	/* nothing to do */
}

/****************************************************************************
 * Get the current SC/MP context
 ****************************************************************************/
unsigned SCAMP_get_context(void *dst)
{
	if( dst )
		*(SCAMP_Regs*)dst = I;
	return sizeof(SCAMP_Regs);
}

/****************************************************************************
 * Set the current SC/MP context
 ****************************************************************************/
void SCAMP_set_context(void *src)
{
	if( src )
	{
		I = *(SCAMP_Regs*)src;
		change_pc16(I.PC.d);
	}
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned SCAMP_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return I.PC.d;
		case SCAMP_PC: return I.PC.w.l;
		case SCAMP_P1: return I.P1.w.l;
		case SCAMP_P2: return I.P2.w.l;
		case SCAMP_P3: return I.P3.w.l;
		case SCAMP_AC: return I.accu;
		case SCAMP_EX: return I.ereg;
		case SCAMP_ST: return I.sreg;
		case REG_PREVIOUSPC: return 0; /* previous pc not supported */
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void SCAMP_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: I.PC.w.l = val; change_pc16(I.PC.d); break;
		case SCAMP_PC: I.PC.w.l = val; break;
		case SCAMP_P1: I.P1.w.l = val; break;
		case SCAMP_P2: I.P2.w.l = val; break;
		case SCAMP_P3: I.P3.w.l = val; break;
		case SCAMP_AC: I.accu = val; break;
		case SCAMP_EX: I.ereg = val; break;
		case SCAMP_ST: I.sreg = val; break;
	}
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *SCAMP_info(void *context, int regnum)
{
	static char buffer[12][47+1];
	static int which = 0;
	SCAMP_Regs *r = context;

	which = (which+1) % 12;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+SCAMP_PC: sprintf(buffer[which], "PC:%04X", r->PC.w.l); break;
		case CPU_INFO_REG+SCAMP_P1: sprintf(buffer[which], "P1:%04X", r->P1.w.l); break;
		case CPU_INFO_REG+SCAMP_P2: sprintf(buffer[which], "P2:%04X", r->P2.w.l); break;
		case CPU_INFO_REG+SCAMP_P3: sprintf(buffer[which], "P3:%04X", r->P3.w.l); break;
		case CPU_INFO_REG+SCAMP_AC: sprintf(buffer[which], "ACCU:%02X", r->accu); break;
		case CPU_INFO_REG+SCAMP_EX: sprintf(buffer[which], "EXTR:%02X", r->ereg); break;
		case CPU_INFO_REG+SCAMP_ST: sprintf(buffer[which], "STAT:%02X", r->sreg); break;
		case CPU_INFO_FLAGS: sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
		  (I.sreg & 0x80) ? 'C' : '.',
		  (I.sreg & 0x40) ? 'V' : '.',
		  (I.sreg & 0x20) ? 'B' : '.',
		  (I.sreg & 0x10) ? 'A' : '.',
		  (I.sreg & 0x08) ? 'I' : '.',
		  (I.sreg & 0x04) ? '2' : '.',
		  (I.sreg & 0x02) ? '1' : '.',
		  (I.sreg & 0x01) ? '0' : '.');
		  break;
		case CPU_INFO_NAME: return "SC/MP";
		case CPU_INFO_FAMILY: return "National Semiconductor SC/MP";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "written 2004 by G. Volkenborn";
		case CPU_INFO_REG_LAYOUT: return (const char *)SCAMP_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)SCAMP_win_layout;
	}
	return buffer[which];
}

void SCAMP_set_sense_a(int state) {
	if (state)
		I.sreg |= 0x10;
	else
		I.sreg &= 0xef;
}

void SCAMP_set_sense_b(int state) {
	if (state)
		I.sreg |= 0x20;
	else
		I.sreg &= 0xdf;
}

void SCAMP_set_irq_line(int irqline, int state) {
	SCAMP_set_sense_a(state);
}

void SCAMP_set_irq_callback(int (*callback)(int)) {
}

unsigned SCAMP_dasm(char *buffer, unsigned pc)
{
	pc++;
#ifdef MAME_DEBUG
	return DasmScamp(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
