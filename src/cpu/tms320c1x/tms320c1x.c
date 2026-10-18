// license:BSD-3-Clause
// copyright-holders:Tony La Porta
/**************************************************************************

    Texas Instruments TMS320C1x DSP Emulator

    Copyright Tony La Porta

    Notes:
    * The term 'DMA' within this document, is in reference to Direct
      Memory Addressing, and NOT the usual term of Direct Memory Access.
    * This is a word based microcontroller, with addressing architecture
      based on the Harvard addressing scheme.

    C port for PinMAME of MAME's src/devices/cpu/tms320c1x/tms320c1x.cpp
    (the instruction semantics and cycle counts are kept exactly as in
    MAME, including its documented quirks). See tms320c1x.h.

**************************************************************************/

#include <string.h>
#include "driver.h"
#include "tms320c1x.h" /* also compiled through src/sound/bsmt2000.c, see there */

/*********  The following is the Status (Flag) register definition.  *********/
/* 15 | 14  |  13  | 12 | 11 | 10 | 9 |  8  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0  */
/* OV | OVM | INTM |  1 |  1 |  1 | 1 | ARP | 1 | 1 | 1 | 1 | 1 | 1 | 1 | DP */
#define OV_FLAG     0x8000  /* OV   (Overflow flag) 1 indicates an overflow */
#define OVM_FLAG    0x4000  /* OVM  (Overflow Mode bit) 1 forces ACC overflow to greatest positive or negative saturation value */
#define INTM_FLAG   0x2000  /* INTM (Interrupt Mask flag) 0 enables maskable interrupts */
#define ARP_REG     0x0100  /* ARP  (Auxiliary Register Pointer) */
#define DP_REG      0x0001  /* DP   (Data memory Pointer (bank) bit) */

#define OV      (c->STR & OV_FLAG)
#define OVM     (c->STR & OVM_FLAG)
#define INTM    (c->STR & INTM_FLAG)
#define ARP     ((c->STR & ARP_REG) >> 8)
#define DP      ((c->STR & DP_REG) << 7)

#define OPL     (c->opcode & 0xff)          /* low byte of the opcode */
#define OPH     (c->opcode >> 8)            /* high byte of the opcode */

#define DMA_DP  (DP | (OPL & 0x7f))         /* address used in direct memory access operations */
#define DMA_DP1 (0x80 | OPL)                /* address used in direct memory access operations for sst instruction */
#define IND     (c->AR[ARP] & 0xff)         /* address used in indirect memory access operations */

#define ACC_H   ((UINT16)(c->ACC >> 16))
#define ACC_L   ((UINT16)c->ACC)
#define SET_ACC_H(v) (c->ACC = (c->ACC & 0x0000ffff) | ((UINT32)(UINT16)(v) << 16))
#define SET_ACC_L(v) (c->ACC = (c->ACC & 0xffff0000) | (UINT16)(v))

#define RDRAM(A)    (c->ram[(A) & 0xff])
#define WRRAM(A,V)  (c->ram[(A) & 0xff] = (UINT16)(V))
#define RDOP(A)     (c->rom[(A) & c->addr_mask])
#define RDROM(A)    (c->rom[(A) & c->addr_mask])
#define P_IN(P)     (c->io_r ? c->io_r(c->param, (P)) : 0)
#define P_OUT(P,V)  do { if (c->io_w) c->io_w(c->param, (P), (V)); } while (0)
#define BIO         (c->bio_r ? c->bio_r(c->param) : 0)

typedef struct tms320c1x_state tms_t;

/************************************************************************
 *  Shortcuts
 ************************************************************************/

INLINE void CLR(tms_t *c, UINT16 flag) { c->STR &= ~flag; c->STR |= 0x1efe; }
INLINE void SET_FLAG(tms_t *c, UINT16 flag) { c->STR |= flag; c->STR |= 0x1efe; }

INLINE void CALCULATE_ADD_OVERFLOW(tms_t *c, INT32 addval)
{
	if ((INT32)(~(c->oldacc ^ (UINT32)addval) & (c->oldacc ^ c->ACC)) < 0) {
		SET_FLAG(c, OV_FLAG);
		if (OVM)
			c->ACC = ((INT32)c->oldacc < 0) ? 0x80000000 : 0x7fffffff;
	}
}
INLINE void CALCULATE_SUB_OVERFLOW(tms_t *c, INT32 subval)
{
	if ((INT32)((c->oldacc ^ (UINT32)subval) & (c->oldacc ^ c->ACC)) < 0) {
		SET_FLAG(c, OV_FLAG);
		if (OVM)
			c->ACC = ((INT32)c->oldacc < 0) ? 0x80000000 : 0x7fffffff;
	}
}

INLINE UINT16 POP_STACK(tms_t *c)
{
	const UINT16 data = c->STACK[3];
	c->STACK[3] = c->STACK[2];
	c->STACK[2] = c->STACK[1];
	c->STACK[1] = c->STACK[0];
	return data & c->addr_mask;
}
INLINE void PUSH_STACK(tms_t *c, UINT16 data)
{
	c->STACK[0] = c->STACK[1];
	c->STACK[1] = c->STACK[2];
	c->STACK[2] = c->STACK[3];
	c->STACK[3] = data & c->addr_mask;
}

INLINE void UPDATE_AR(tms_t *c)
{
	if (OPL & 0x30) {
		UINT16 tmpAR = c->AR[ARP];
		if (OPL & 0x20) tmpAR++;
		if (OPL & 0x10) tmpAR--;
		c->AR[ARP] = (c->AR[ARP] & 0xfe00) | (tmpAR & 0x01ff);
	}
}
INLINE void UPDATE_ARP(tms_t *c)
{
	if (~OPL & 0x08) {
		if (OPL & 0x01) SET_FLAG(c, ARP_REG);
		else CLR(c, ARP_REG);
	}
}

INLINE void getdata(tms_t *c, UINT8 shift, UINT8 signext)
{
	if (OPL & 0x80)
		c->memaccess = IND;
	else
		c->memaccess = DMA_DP;

	c->ALU = (UINT16)RDRAM(c->memaccess);
	if (signext) c->ALU = (UINT32)(INT32)(INT16)c->ALU;
	c->ALU <<= shift;
	if (OPL & 0x80) {
		UPDATE_AR(c);
		UPDATE_ARP(c);
	}
}

INLINE void putdata(tms_t *c, UINT16 data)
{
	if (OPL & 0x80)
		c->memaccess = IND;
	else
		c->memaccess = DMA_DP;

	if (OPL & 0x80) {
		UPDATE_AR(c);
		UPDATE_ARP(c);
	}
	WRRAM(c->memaccess, data);
}
INLINE void putdata_sar(tms_t *c, UINT8 n)
{
	if (OPL & 0x80)
		c->memaccess = IND;
	else
		c->memaccess = DMA_DP;

	if (OPL & 0x80) {
		UPDATE_AR(c);
		UPDATE_ARP(c);
	}
	WRRAM(c->memaccess, c->AR[n]);
}
INLINE void putdata_sst(tms_t *c, UINT16 data)
{
	if (OPL & 0x80)
		c->memaccess = IND;
	else
		c->memaccess = DMA_DP1;  /* Page 1 only */

	if (OPL & 0x80) {
		UPDATE_AR(c);
	}
	WRRAM(c->memaccess, data);
}

/* taken conditional branches cost one more cycle (as in MAME) */
#define BRANCH_TAKEN() do { c->PC = RDOP(c->PC); c->icount -= 1; } while (0)
#define BRANCH_IF(cond) do { if (cond) BRANCH_TAKEN(); else c->PC++; } while (0)

/************************************************************************
 *  Instructions
 ************************************************************************/

static void exec_7f(tms_t *c)
{
	switch (OPL & 0x1f)
	{
		case 0x00: /* nop */
			c->icount -= 1;
			break;
		case 0x01: /* dint */
			c->icount -= 1;
			SET_FLAG(c, INTM_FLAG);
			break;
		case 0x02: /* eint */
			c->icount -= 1;
			CLR(c, INTM_FLAG);
			break;
		case 0x08: /* abst */
			c->icount -= 1;
			if ((INT32)c->ACC < 0) {
				c->ACC = (UINT32)(-(INT32)c->ACC);
				if (OVM && (c->ACC == 0x80000000)) c->ACC--;
			}
			break;
		case 0x09: /* zac */
			c->icount -= 1;
			c->ACC = 0;
			break;
		case 0x0a: /* rovm */
			c->icount -= 1;
			CLR(c, OVM_FLAG);
			break;
		case 0x0b: /* sovm */
			c->icount -= 1;
			SET_FLAG(c, OVM_FLAG);
			break;
		case 0x0c: /* cala */
			c->icount -= 2;
			PUSH_STACK(c, c->PC);
			c->PC = ACC_L & c->addr_mask;
			break;
		case 0x0d: /* ret */
			c->icount -= 2;
			c->PC = POP_STACK(c);
			break;
		case 0x0e: /* pac */
			c->icount -= 1;
			c->ACC = c->Preg;
			break;
		case 0x0f: /* apac */
			c->icount -= 1;
			c->oldacc = c->ACC;
			c->ACC += c->Preg;
			CALCULATE_ADD_OVERFLOW(c, (INT32)c->Preg);
			break;
		case 0x10: /* spac */
			c->icount -= 1;
			c->oldacc = c->ACC;
			c->ACC -= c->Preg;
			CALCULATE_SUB_OVERFLOW(c, (INT32)c->Preg);
			break;
		case 0x1c: /* push */
			c->icount -= 2;
			PUSH_STACK(c, ACC_L);
			break;
		case 0x1d: /* pop */
			c->icount -= 2;
			c->ACC = POP_STACK(c);
			break;
		default: /* illegal: 0 cycles in MAME, 1 here so that a stray PC cannot hang the host */
			c->icount -= 1;
			logerror("TMS320C1x:  PC=%04x,  Illegal opcode = %04x\n", (c->PC - 1) & 0xffff, c->opcode);
			break;
	}
}

static void exec_one(tms_t *c)
{
	const UINT8 hi = OPH;

	if (hi < 0x30)
	{
		c->icount -= 1;
		switch (hi >> 4)
		{
			case 0: /* add_sh */
				c->oldacc = c->ACC;
				getdata(c, hi & 0x0f, 1);
				c->ACC += c->ALU;
				CALCULATE_ADD_OVERFLOW(c, (INT32)c->ALU);
				break;
			case 1: /* sub_sh */
				c->oldacc = c->ACC;
				getdata(c, hi & 0x0f, 1);
				c->ACC -= c->ALU;
				CALCULATE_SUB_OVERFLOW(c, (INT32)c->ALU);
				break;
			default: /* lac_sh */
				getdata(c, hi & 0x0f, 1);
				c->ACC = c->ALU;
				break;
		}
		return;
	}
	if (hi >= 0x80 && hi < 0xa0) /* mpyk */
	{
		c->icount -= 1;
		c->Preg = (UINT32)((INT32)(INT16)c->Treg * (INT32)((INT16)(c->opcode << 3) >> 3));
		return;
	}
	if (hi >= 0x40 && hi < 0x48) /* in_p */
	{
		c->icount -= 2;
		c->ALU = (c->ALU & 0xffff0000) | P_IN(hi & 7);
		putdata(c, (UINT16)c->ALU);
		return;
	}
	if (hi >= 0x48 && hi < 0x50) /* out_p */
	{
		c->icount -= 2;
		getdata(c, 0, 0);
		P_OUT(hi & 7, (UINT16)c->ALU);
		return;
	}
	if (hi >= 0x58 && hi < 0x60) /* sach_sh */
	{
		c->icount -= 1;
		c->ALU = c->ACC << (hi & 7);
		putdata(c, (UINT16)(c->ALU >> 16));
		return;
	}

	switch (hi)
	{
		case 0x30: c->icount -= 1; putdata_sar(c, 0); break;               /* sar_ar0 */
		case 0x31: c->icount -= 1; putdata_sar(c, 1); break;               /* sar_ar1 */
		case 0x38: c->icount -= 1; getdata(c, 0, 0); c->AR[0] = (UINT16)c->ALU; break; /* lar_ar0 */
		case 0x39: c->icount -= 1; getdata(c, 0, 0); c->AR[1] = (UINT16)c->ALU; break; /* lar_ar1 */
		case 0x50: c->icount -= 1; putdata(c, ACC_L); break;               /* sacl */

		case 0x60: /* addh */
		{
			UINT16 oldh, newh;
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 0, 0);
			SET_ACC_H(ACC_H + (UINT16)c->ALU);
			oldh = (UINT16)(c->oldacc >> 16);
			newh = ACC_H;
			if ((INT16)(~(oldh ^ (UINT16)(c->ALU >> 16)) & (oldh ^ newh)) < 0) {
				SET_FLAG(c, OV_FLAG);
				if (OVM)
					SET_ACC_H(((INT16)oldh < 0) ? 0x8000 : 0x7fff);
			}
			break;
		}
		case 0x61: /* adds */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 0, 0);
			c->ACC += c->ALU;
			CALCULATE_ADD_OVERFLOW(c, (INT32)c->ALU);
			break;
		case 0x62: /* subh */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 16, 0);
			c->ACC -= c->ALU;
			CALCULATE_SUB_OVERFLOW(c, (INT32)c->ALU);
			break;
		case 0x63: /* subs */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 0, 0);
			c->ACC -= c->ALU;
			CALCULATE_SUB_OVERFLOW(c, (INT32)c->ALU);
			break;
		case 0x64: /* subc */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 15, 0);
			c->ALU = (UINT32)((INT32)c->ACC - (INT32)c->ALU);
			if ((INT32)((c->oldacc ^ c->ALU) & (c->oldacc ^ c->ACC)) < 0)
				SET_FLAG(c, OV_FLAG);
			if ((INT32)c->ALU >= 0)
				c->ACC = (c->ALU << 1) + 1;
			else
				c->ACC = c->ACC << 1;
			break;
		case 0x65: /* zalh */
			c->icount -= 1;
			getdata(c, 0, 0);
			c->ACC = (UINT32)(UINT16)c->ALU << 16;
			break;
		case 0x66: /* zals */
			c->icount -= 1;
			getdata(c, 0, 0);
			c->ACC = (UINT16)c->ALU;
			break;
		case 0x67: /* tblr */
			c->icount -= 3;
			c->ALU = RDROM(ACC_L & c->addr_mask);
			putdata(c, (UINT16)c->ALU);
			c->STACK[0] = c->STACK[1];
			break;
		case 0x68: /* larp_mar */
			c->icount -= 1;
			if (OPL & 0x80) {
				UPDATE_AR(c);
				UPDATE_ARP(c);
			}
			break;
		case 0x69: /* dmov */
			c->icount -= 1;
			getdata(c, 0, 0);
			WRRAM(c->memaccess + 1, (UINT16)c->ALU);
			break;
		case 0x6a: /* lt */
			c->icount -= 1;
			getdata(c, 0, 0);
			c->Treg = (UINT16)c->ALU;
			break;
		case 0x6b: /* ltd */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 0, 0);
			c->Treg = (UINT16)c->ALU;
			WRRAM(c->memaccess + 1, (UINT16)c->ALU);
			c->ACC += c->Preg;
			CALCULATE_ADD_OVERFLOW(c, (INT32)c->Preg);
			break;
		case 0x6c: /* lta */
			c->icount -= 1;
			c->oldacc = c->ACC;
			getdata(c, 0, 0);
			c->Treg = (UINT16)c->ALU;
			c->ACC += c->Preg;
			CALCULATE_ADD_OVERFLOW(c, (INT32)c->Preg);
			break;
		case 0x6d: /* mpy */
			c->icount -= 1;
			getdata(c, 0, 0);
			c->Preg = (UINT32)((INT32)(INT16)c->ALU * (INT32)(INT16)c->Treg);
			if (c->Preg == 0x40000000) c->Preg = 0xc0000000;
			break;
		case 0x6e: /* ldpk */
			c->icount -= 1;
			if (OPL & 1) SET_FLAG(c, DP_REG);
			else CLR(c, DP_REG);
			break;
		case 0x6f: /* ldp */
			c->icount -= 1;
			getdata(c, 0, 0);
			if (c->ALU & 1) SET_FLAG(c, DP_REG);
			else CLR(c, DP_REG);
			break;
		case 0x70: c->icount -= 1; c->AR[0] = OPL; break;                 /* lark_ar0 */
		case 0x71: c->icount -= 1; c->AR[1] = OPL; break;                 /* lark_ar1 */
		case 0x78: /* xor */
			c->icount -= 1;
			getdata(c, 0, 0);
			SET_ACC_L(ACC_L ^ (UINT16)c->ALU);
			break;
		case 0x79: /* and */
			c->icount -= 1;
			getdata(c, 0, 0);
			c->ACC &= c->ALU;
			break;
		case 0x7a: /* or */
			c->icount -= 1;
			getdata(c, 0, 0);
			SET_ACC_L(ACC_L | (UINT16)c->ALU);
			break;
		case 0x7b: /* lst */
			c->icount -= 1;
			if (OPL & 0x80)
				c->opcode |= 0x08; /* In Indirect Addressing mode, next ARP is not supported here so mask it */
			getdata(c, 0, 0);
			c->ALU = (c->ALU & 0xffff0000) | ((UINT16)c->ALU & (UINT16)~INTM_FLAG); /* Must not affect INTM */
			c->STR &= INTM_FLAG;
			c->STR |= (UINT16)c->ALU;
			c->STR |= 0x1efe;
			break;
		case 0x7c: c->icount -= 1; putdata_sst(c, c->STR); break;         /* sst */
		case 0x7d: /* tblw: the program memory is a mask ROM here, so the write goes nowhere */
			c->icount -= 3;
			getdata(c, 0, 0);
			c->STACK[0] = c->STACK[1];
			break;
		case 0x7e: c->icount -= 1; c->ACC = OPL; break;                   /* lack */
		case 0x7f: exec_7f(c); break;

		case 0xf4: /* banz */
		{
			UINT16 v;
			c->icount -= 1;
			BRANCH_IF(c->AR[ARP] & 0x01ff);
			v = (UINT16)(c->AR[ARP] - 1);
			c->ALU = (c->ALU & 0xffff0000) | v;
			c->AR[ARP] = (c->AR[ARP] & 0xfe00) | (v & 0x01ff);
			break;
		}
		case 0xf5: /* bv */
			c->icount -= 1;
			if (OV) {
				CLR(c, OV_FLAG);
				BRANCH_TAKEN();
			}
			else
				c->PC++;
			break;
		case 0xf6: c->icount -= 1; BRANCH_IF(BIO != 0); break;            /* bioz */
		case 0xf8: /* call */
			c->icount -= 2;
			c->PC++;
			PUSH_STACK(c, c->PC);
			c->PC = RDOP((UINT16)(c->PC - 1));
			break;
		case 0xf9: c->icount -= 2; c->PC = RDOP(c->PC); break;            /* br */
		case 0xfa: c->icount -= 1; BRANCH_IF((INT32)c->ACC <  0); break;  /* blz */
		case 0xfb: c->icount -= 1; BRANCH_IF((INT32)c->ACC <= 0); break;  /* blez */
		case 0xfc: c->icount -= 1; BRANCH_IF((INT32)c->ACC >  0); break;  /* bgz */
		case 0xfd: c->icount -= 1; BRANCH_IF((INT32)c->ACC >= 0); break;  /* bgez */
		case 0xfe: c->icount -= 1; BRANCH_IF(c->ACC != 0); break;         /* bnz */
		case 0xff: c->icount -= 1; BRANCH_IF(c->ACC == 0); break;         /* bz */

		default: /* illegal: 0 cycles in MAME, 1 here so that a stray PC cannot hang the host */
			c->icount -= 1;
			logerror("TMS320C1x:  PC=%04x,  Illegal opcode = %04x\n", (c->PC - 1) & 0xffff, c->opcode);
			break;
	}
}

/****************************************************************************
 *  Public interface
 ****************************************************************************/

void tms320c1x_init(struct tms320c1x_state *c, const UINT16 *rom, int addr_bits, void *param,
	UINT16 (*io_r)(void *param, int port), void (*io_w)(void *param, int port, UINT16 data), int (*bio_r)(void *param))
{
	memset(c, 0, sizeof(*c));
	c->rom = rom;
	c->addr_mask = (UINT16)((1u << addr_bits) - 1);
	c->param = param;
	c->io_r = io_r;
	c->io_w = io_w;
	c->bio_r = bio_r;
}

void tms320c1x_reset(struct tms320c1x_state *c)
{
	c->PC   = 0;
	c->ACC  = 0;
	c->INTF = 0;
	/* Setup Status Register : 7efe */
	CLR(c, (OV_FLAG | ARP_REG | DP_REG));
	SET_FLAG(c, (OVM_FLAG | INTM_FLAG));
}

void tms320c1x_set_irq(struct tms320c1x_state *c)
{
	c->INTF = 1;
}

void tms320c1x_abort(struct tms320c1x_state *c)
{
	c->abort = 1;
}

int tms320c1x_execute(struct tms320c1x_state *c, int cycles)
{
	c->icount = cycles;
	c->abort = 0;
	do
	{
		if (c->INTF) {
			/* Don't service INT if previous instruction was MPY, MPYK or EINT */
			if ((OPH != 0x6d) && ((OPH & 0xe0) != 0x80) && (c->opcode != 0x7f82) && !INTM) {
				c->INTF = 0;
				SET_FLAG(c, INTM_FLAG);
				PUSH_STACK(c, c->PC);
				c->PC = 0x0002;
				c->icount -= 3; /* 3 cycles used due to PUSH and DINT operation ? */
			}
		}

		c->PREVPC = c->PC;
		c->opcode = RDOP(c->PC);
		c->PC++;
		exec_one(c);
	} while (c->icount > 0 && !c->abort);

	return cycles - c->icount;
}
