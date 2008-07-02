#include <stdio.h>
#include "mame.h"
#include "driver.h"
#include "state.h"
#include "mamedbg.h"

#include "cdp1802.h"

#define CDP1802_CYCLES_RESET 		8
#define CDP1802_CYCLES_INIT			8 // really 9, but needs to be 8 to synchronize cdp1861 video timings
#define CDP1802_CYCLES_FETCH		8
#define CDP1802_CYCLES_EXECUTE		8
#define CDP1802_CYCLES_DMA			8
#define CDP1802_CYCLES_INTERRUPT	8

enum {
	CDP1802_STATE_0_FETCH = 0,
	CDP1802_STATE_1_RESET,
	CDP1802_STATE_1_INIT,
	CDP1802_STATE_1_EXECUTE,
	CDP1802_STATE_2_DMA_IN,
	CDP1802_STATE_2_DMA_OUT,
	CDP1802_STATE_3_INT
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

typedef struct
{
	CDP1802_CONFIG *intf;

	UINT8 p, x, d, b, t;
	UINT16 r[16];
	UINT8 df, ie, q, n, i;
    UINT16 oldpc;
	int state;
	int prevmode, mode;
	int irq, dmain, dmaout;
	int ef;

} CDP1802_Regs;

int cdp1802_icount;

static CDP1802_Regs cdp1802;

#define M	cpu_readmem16
#define MW	cpu_writemem16

#define P	cdp1802.p
#define X	cdp1802.x
#define D	cdp1802.d
#define B   cdp1802.b
#define T	cdp1802.t
#define R   cdp1802.r
#define DF	cdp1802.df
#define IE	cdp1802.ie
#define Q	cdp1802.q
#define N	cdp1802.n
#define I	cdp1802.i

void cdp1802_init(void)
{
//	cdp1802.intf = (CDP1802_CONFIG *) config;

	cdp1802.mode = CDP1802_MODE_RESET;
	cdp1802.prevmode = cdp1802.mode;
	cdp1802.irq = CLEAR_LINE;
	cdp1802.dmain = CLEAR_LINE;
	cdp1802.dmaout = CLEAR_LINE;
}

void cdp1802_reset(void *param)
{
	if (param) {
		cdp1802.intf = (CDP1802_CONFIG *) param;
		cdp1802.mode = CDP1802_MODE_RESET;
	}
	cdp1802.p=0;
	cdp1802.q=0;
	cdp1802.ie=1;
	cdp1802.df=0;
	cdp1802.r[0]=0;
	change_pc16(cdp1802.r[cdp1802.p]);
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
		change_pc16(cdp1802.r[cdp1802.p]);
	}
}

unsigned cdp1802_get_reg (int regnum)
{
	switch( regnum )
	{
	case REG_PC: return cdp1802.r[cdp1802.p];
	case REG_SP: return 0;
	case CDP1802_P: return cdp1802.p;
	case CDP1802_X: return cdp1802.x;
	case CDP1802_T: return cdp1802.t;
	case CDP1802_D: return cdp1802.d;
	case CDP1802_B: return cdp1802.b;
	case CDP1802_R0: return cdp1802.r[0];
	case CDP1802_R1: return cdp1802.r[1];
	case CDP1802_R2: return cdp1802.r[2];
	case CDP1802_R3: return cdp1802.r[3];
	case CDP1802_R4: return cdp1802.r[4];
	case CDP1802_R5: return cdp1802.r[5];
	case CDP1802_R6: return cdp1802.r[6];
	case CDP1802_R7: return cdp1802.r[7];
	case CDP1802_R8: return cdp1802.r[8];
	case CDP1802_R9: return cdp1802.r[9];
	case CDP1802_Ra: return cdp1802.r[0xa];
	case CDP1802_Rb: return cdp1802.r[0xb];
	case CDP1802_Rc: return cdp1802.r[0xc];
	case CDP1802_Rd: return cdp1802.r[0xd];
	case CDP1802_Re: return cdp1802.r[0xe];
	case CDP1802_Rf: return cdp1802.r[0xf];
	case CDP1802_DF: return cdp1802.df;
	case CDP1802_IE: return cdp1802.ie;
	case CDP1802_Q: return cdp1802.q;
	case REG_PREVIOUSPC: return cdp1802.oldpc;
	}
	return 0;
}

void cdp1802_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case REG_PC: cdp1802.r[cdp1802.p]=val;change_pc16(cdp1802.r[cdp1802.p]);break;
	case REG_SP: break;
	case CDP1802_P: cdp1802.p=val;break;
	case CDP1802_X: cdp1802.x=val;break;
	case CDP1802_T: cdp1802.t=val;break;
	case CDP1802_D: cdp1802.d=val;break;
	case CDP1802_B: cdp1802.b=val;break;
	case CDP1802_R0: cdp1802.r[0]=val;break;
	case CDP1802_R1: cdp1802.r[1]=val;break;
	case CDP1802_R2: cdp1802.r[2]=val;break;
	case CDP1802_R3: cdp1802.r[3]=val;break;
	case CDP1802_R4: cdp1802.r[4]=val;break;
	case CDP1802_R5: cdp1802.r[5]=val;break;
	case CDP1802_R6: cdp1802.r[6]=val;break;
	case CDP1802_R7: cdp1802.r[7]=val;break;
	case CDP1802_R8: cdp1802.r[8]=val;break;
	case CDP1802_R9: cdp1802.r[9]=val;break;
	case CDP1802_Ra: cdp1802.r[0xa]=val;break;
	case CDP1802_Rb: cdp1802.r[0xb]=val;break;
	case CDP1802_Rc: cdp1802.r[0xc]=val;break;
	case CDP1802_Rd: cdp1802.r[0xd]=val;break;
	case CDP1802_Re: cdp1802.r[0xe]=val;break;
	case CDP1802_Rf: cdp1802.r[0xf]=val;break;
	case CDP1802_DF: cdp1802.df=val;break;
	case CDP1802_IE: cdp1802.ie=val;break;
	case CDP1802_Q: cdp1802.q=val;break;
	case REG_PREVIOUSPC: cdp1802.oldpc=val;break;
	}
}

INLINE void cdp1802_add(int left, int right)
{
	int result = left + right;
	D = result & 0xff;
	DF = (result & 0x100) >> 8;
}

INLINE void cdp1802_add_carry(int left, int right)
{
	int result = left + right + DF;
	D = result & 0xff;
	DF = (result & 0x100) >> 8;
}

INLINE void cdp1802_sub(int left, int right)
{
	int result = left + (~right & 0xff) + 1;

	D = result & 0xff;
	DF = (result & 0x100) >> 8;
}

INLINE void cdp1802_sub_carry(int left, int right)
{
	int result = left + (~right & 0xff) + DF;

	D = result & 0xff;
	DF = (result & 0x100) >> 8;
}

INLINE void cdp1802_short_branch(int taken)
{
	if (taken)
	{
		R[P] = (R[P] & 0xff00) | cpu_readop(R[P]);
	}
	else
	{
		R[P] = R[P] + 1;
	}
}

INLINE void cdp1802_long_branch(int taken)
{
	if (taken)
	{
		// S1#1

		B = cpu_readop(R[P]);
		R[P] = R[P] + 1;

		// S1#2

		R[P] = (B << 8) | cpu_readop(R[P]);
	}
	else
	{
		// S1#1

		R[P] = R[P] + 1;

		// S1#2

		R[P] = R[P] + 1;
	}
}

INLINE void cdp1802_long_skip(int taken)
{
	if (taken)
	{
		// S1#1

		R[P] = R[P] + 1;

		// S1#2

		R[P] = R[P] + 1;
	}
}

static void cdp1802_sample_ef(void)
{
	if (cdp1802.intf && cdp1802.intf->ef_r)
	{
		cdp1802.ef = cdp1802.intf->ef_r() & 0x0f;
	}
	else
	{
		cdp1802.ef = 0x0f;
	}
}

static void cdp1802_output_state_code(void)
{
	if (cdp1802.intf && cdp1802.intf->sc_w)
	{
		cdp1802_state state_code = CDP1802_STATE_CODE_S0_FETCH;

		switch (cdp1802.state)
		{
		case CDP1802_STATE_0_FETCH:
			state_code = CDP1802_STATE_CODE_S0_FETCH;
			break;

		case CDP1802_STATE_1_EXECUTE:
			state_code = CDP1802_STATE_CODE_S1_EXECUTE;
			break;

		case CDP1802_STATE_2_DMA_IN:
		case CDP1802_STATE_2_DMA_OUT:
			state_code = CDP1802_STATE_CODE_S2_DMA;
			break;

		case CDP1802_STATE_3_INT:
			state_code = CDP1802_STATE_CODE_S3_INTERRUPT;
			break;
		}

		cdp1802.intf->sc_w(state_code);
	}
}

static void cdp1802_run(void)
{
	cdp1802_output_state_code();

	switch (cdp1802.state)
	{
	case CDP1802_STATE_1_RESET:

		I = 0;
		N = 0;
		Q = 0;
		IE = 1;

		cdp1802_icount -= CDP1802_CYCLES_RESET;

		CALL_MAME_DEBUG;

		break;

	case CDP1802_STATE_1_INIT:

		X = 0;
		P = 0;
		R[0] = 0;

		cdp1802_icount -= CDP1802_CYCLES_INIT;

		if (cdp1802.dmain)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_IN;
		}
		else if (cdp1802.dmaout)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_OUT;
		}
		else
		{
			cdp1802.state = CDP1802_STATE_0_FETCH;
		}

		CALL_MAME_DEBUG;

		break;

	case CDP1802_STATE_0_FETCH:
		{
		UINT8 opcode = cpu_readop(R[P]);

		I = opcode >> 4;
		N = opcode & 0x0f;
		R[P] = R[P] + 1;

		cdp1802_icount -= CDP1802_CYCLES_FETCH;

		cdp1802.state = CDP1802_STATE_1_EXECUTE;
		}
		break;

	case CDP1802_STATE_1_EXECUTE:

		cdp1802_sample_ef();

		switch (I)
		{
		case 0:
			if (N > 0)
			{
				D = M(R[N]);
			}
			break;

		case 1:
			R[N] = R[N] + 1;
			break;

		case 2:
			R[N] = R[N] - 1;
			break;

		case 3:
			switch (N)
			{
			case 0:
				cdp1802_short_branch(1);
				break;

			case 1:
				cdp1802_short_branch(Q == 1);
				break;

			case 2:
				cdp1802_short_branch(D == 0);
				break;

			case 3:
				cdp1802_short_branch(DF == 1);
				break;

			case 4:
				cdp1802_short_branch((cdp1802.ef & EF1) ? 0 : 1);
				break;

			case 5:
				cdp1802_short_branch((cdp1802.ef & EF2) ? 0 : 1);
				break;

			case 6:
				cdp1802_short_branch((cdp1802.ef & EF3) ? 0 : 1);
				break;

			case 7:
				cdp1802_short_branch((cdp1802.ef & EF4) ? 0 : 1);
				break;

			case 8:
				cdp1802_short_branch(0);
				break;

			case 9:
				cdp1802_short_branch(Q == 0);
				break;

			case 0xa:
				cdp1802_short_branch(D != 0);
				break;

			case 0xb:
				cdp1802_short_branch(DF == 0);
				break;

			case 0xc:
				cdp1802_short_branch((cdp1802.ef & EF1) ? 1 : 0);
				break;

			case 0xd:
				cdp1802_short_branch((cdp1802.ef & EF2) ? 1 : 0);
				break;

			case 0xe:
				cdp1802_short_branch((cdp1802.ef & EF3) ? 1 : 0);
				break;

			case 0xf:
				cdp1802_short_branch((cdp1802.ef & EF4) ? 1 : 0);
				break;
			}
			break;

		case 4:
			D = M(R[N]);
			R[N] = R[N] + 1;
			break;

		case 5:
			MW(R[N], D);
			break;

		case 6:
			switch (N)
			{
			case 0:
				R[X] = R[X] + 1;
				break;

			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
				cpu_writeport16(N, M(R[X]));
				R[X] = R[X] + 1;
				break;

			case 8:
				/*

                    A note about INP 0 (0x68) from Tom Pittman's "A Short Course in Programming":

                    If you look carefully, you will notice that we never studied the opcode "68".
                    That's because it is not a defined 1802 instruction. It has the form of an INP
                    instruction, but 0 is not a defined input port, so if you execute it (try it!)
                    nothing is input. "Nothing" is the answer to a question; it is data, and something
                    will be put in the accumulator and memory (so now you know what the computer uses
                    to mean "nothing").

                    However, since the result of the "68" opcode is unpredictable, it should not be
                    used in your programs. In fact, "68" is the first byte of a series of additional
                    instructions for the 1804 and 1805 microprocessors.

                    http://www.ittybittycomputers.com/IttyBitty/ShortCor.htm

                */
			case 9:
			case 0xa:
			case 0xb:
			case 0xc:
			case 0xd:
			case 0xe:
			case 0xf:
				{
				UINT8 data = cpu_readport16(N & 0x07);
				MW(R[X], data);
				D = data;
				}
				break;
			}
			break;

		case 7:
			switch (N)
			{
			case 0:
				{
				UINT8 data = M(R[X]);
				R[X] = R[X] + 1;
				P = data & 0xf;
				X = data >> 4;
				IE = 1;
				}
				break;

			case 1:
				{
				UINT8 data = M(R[X]);
				R[X] = R[X] + 1;
				P = data & 0xf;
				X = data >> 4;
				IE = 0;
				}
				break;

			case 2:
				D = M(R[X]);
				R[X] = R[X] + 1;
				break;

			case 3:
				MW(R[X], D);
				R[X] = R[X] - 1;
				break;

			case 4:
				cdp1802_add_carry(M(R[X]), D);
				break;

			case 5:
				cdp1802_sub_carry(M(R[X]), D);
				break;

			case 6:
				{
				int b = DF;
				DF = D & 1;
				D >>= 1;
				if (b) D |= 0x80;
				}
				break;

			case 7:
				cdp1802_sub_carry(D, M(R[X]));
				break;

			case 8:
				MW(R[X], T);
				break;

			case 9:
				{
				UINT8 result = (X << 4) | P;
				T = result;
				MW(R[2], result);
				X = P;
				R[2] = R[2] - 1;
				}
				break;

			case 0xa:
				Q = 0;

				if (cdp1802.intf && cdp1802.intf->q_w)
				{
					cdp1802.intf->q_w(Q);
				}
				break;

			case 0xb:
				Q = 1;

				if (cdp1802.intf && cdp1802.intf->q_w)
				{
					cdp1802.intf->q_w(Q);
				}
				break;

			case 0xc:
				cdp1802_add_carry(M(R[P]), D);
				R[P] = R[P] + 1;
				break;

			case 0xd:
				cdp1802_sub_carry(M(R[P]), D);
				R[P] = R[P] + 1;
				break;

			case 0xe:
				{
				int b = DF;
				DF = D & 0x80;
				D <<= 1;
				if (b) D |= 1;
				}
				break;

			case 0xf:
				cdp1802_sub_carry(D, M(R[P]));
				R[P] = R[P] + 1;
				break;
			}
			break;

		case 8:
			D = R[N] & 0xff;
			break;

		case 9:
			D = (R[N] >> 8) & 0xff;
			break;

		case 0xa:
			R[N] = (R[N] & 0xff00) | D;
			break;

		case 0xb:
			R[N] = (D << 8) | (R[N] & 0xff);
			break;

		case 0xc:
			cdp1802_output_state_code();

			switch (N)
			{
			case 0:
				cdp1802_long_branch(1);
				break;

			case 1:
				cdp1802_long_branch(Q == 1);
				break;

			case 2:
				cdp1802_long_branch(D == 0);
				break;

			case 3:
				cdp1802_long_branch(DF == 1);
				break;

			case 4:
				// NOP
				break;

			case 5:
				cdp1802_long_skip(Q == 0);
				break;

			case 6:
				cdp1802_long_skip(D != 0);
				break;

			case 7:
				cdp1802_long_skip(DF == 0);
				break;

			case 8:
				cdp1802_long_skip(1);
				break;

			case 9:
				cdp1802_long_branch(Q == 0);
				break;

			case 0xa:
				cdp1802_long_branch(D != 0);
				break;

			case 0xb:
				cdp1802_long_branch(DF == 0);
				break;

			case 0xc:
				cdp1802_long_skip(IE == 1);
				break;

			case 0xd:
				cdp1802_long_skip(Q == 1);
				break;

			case 0xe:
				cdp1802_long_skip(D == 0);
				break;

			case 0xf:
				cdp1802_long_skip(DF == 1);
				break;
			}

			cdp1802_icount -= CDP1802_CYCLES_EXECUTE;
			break;

		case 0xd:
			P = N;
			break;

		case 0xe:
			X = N;
			break;

		case 0xf:
			switch (N)
			{
			case 0:
				D = M(R[X]);
				break;

			case 1:
				D = M(R[X]) | D;
				break;

			case 2:
				D = M(R[X]) & D;
				break;

			case 3:
				D = M(R[X]) ^ D;
				break;

			case 4:
				cdp1802_add(M(R[X]), D);
				break;

			case 5:
				cdp1802_sub(M(R[X]), D);
				break;

			case 6:
				DF = D & 0x01;
				D = D >> 1;
				break;

			case 7:
				cdp1802_sub(D, M(R[X]));
				break;

			case 8:
				D = M(R[P]);
				R[P] = R[P] + 1;
				break;

			case 9:
				D = M(R[P]) | D;
				R[P] = R[P] + 1;
				break;

			case 0xa:
				D = M(R[P]) & D;
				R[P] = R[P] + 1;
				break;

			case 0xb:
				D = M(R[P]) ^ D;
				R[P] = R[P] + 1;
				break;

			case 0xc:
				cdp1802_add(M(R[P]), D);
				R[P] = R[P] + 1;
				break;

			case 0xd:
				cdp1802_sub(M(R[P]), D);
				R[P] = R[P] + 1;
				break;

			case 0xe:
				DF = (D & 0x80) >> 7;
				D = D << 1;
				break;

			case 0xf:
				cdp1802_sub(D, M(R[P]));
				R[P] = R[P] + 1;
				break;
			}
			break;
		}

		cdp1802_icount -= CDP1802_CYCLES_EXECUTE;

		if (cdp1802.dmain)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_IN;
		}
		else if (cdp1802.dmaout)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_OUT;
		}
		else if (IE && cdp1802.irq)
		{
			cdp1802.state = CDP1802_STATE_3_INT;
		}
		else if ((I > 0) || (N > 0)) // not idling
		{
			cdp1802.state = CDP1802_STATE_0_FETCH;
		}

		CALL_MAME_DEBUG;

		break;

    case CDP1802_STATE_2_DMA_IN:

		if (cdp1802.intf && cdp1802.intf->dma_r)
		{
			MW(R[0], cdp1802.intf->dma_r(R[0]));
		}

		R[0] = R[0] + 1;

        cdp1802_icount -= CDP1802_CYCLES_DMA;

        if (cdp1802.dmain)
        {
            cdp1802.state = CDP1802_STATE_2_DMA_IN;
        }
        else if (cdp1802.dmaout)
        {
            cdp1802.state = CDP1802_STATE_2_DMA_OUT;
        }
        else if (IE && cdp1802.irq)
        {
            cdp1802.state = CDP1802_STATE_3_INT;
        }
        else if (cdp1802.mode == CDP1802_MODE_LOAD)
        {
            cdp1802.state = CDP1802_STATE_1_EXECUTE;
        }
        else
        {
            cdp1802.state = CDP1802_STATE_0_FETCH;
        }
        break;

    case CDP1802_STATE_2_DMA_OUT:

		if (cdp1802.intf && cdp1802.intf->dma_w)
		{
	        cdp1802.intf->dma_w(R[0], M(R[0]));
		}

		R[0] = R[0] + 1;

        cdp1802_icount -= CDP1802_CYCLES_DMA;

        if (cdp1802.dmain)
        {
            cdp1802.state = CDP1802_STATE_2_DMA_IN;
        }
        else if (cdp1802.dmaout)
        {
            cdp1802.state = CDP1802_STATE_2_DMA_OUT;
        }
        else if (IE && cdp1802.irq)
        {
            cdp1802.state = CDP1802_STATE_3_INT;
        }
        else
        {
            cdp1802.state = CDP1802_STATE_0_FETCH;
        }
        break;

	case CDP1802_STATE_3_INT:

		T = (X << 4) | P;
		X = 2;
		P = 1;
		IE = 0;

		cdp1802_icount -= CDP1802_CYCLES_INTERRUPT;

		if (cdp1802.dmain)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_IN;
		}
		else if (cdp1802.dmaout)
		{
			cdp1802.state = CDP1802_STATE_2_DMA_OUT;
		}
		else
		{
			cdp1802.state = CDP1802_STATE_0_FETCH;
		}

		CALL_MAME_DEBUG;

		break;
	}
}

int cdp1802_execute(int cycles)
{
	cdp1802_icount = cycles;
	cdp1802.prevmode = cdp1802.mode;
	cdp1802.mode = cdp1802.intf && cdp1802.intf->mode_r ? cdp1802.intf->mode_r() : cdp1802.prevmode;

	do
	{
		switch (cdp1802.mode)
		{
		case CDP1802_MODE_LOAD:
			I = 0;
			N = 0;
			cdp1802.state = CDP1802_STATE_1_EXECUTE;
			cdp1802_run();
			break;

		case CDP1802_MODE_RESET:
			cdp1802.state = CDP1802_STATE_1_RESET;
			cdp1802_run();
			break;

		case CDP1802_MODE_PAUSE:
			cdp1802_icount -= 1;
			break;

		case CDP1802_MODE_RUN:
			switch (cdp1802.prevmode)
			{
			case CDP1802_MODE_LOAD:
				// RUN mode cannot be initiated from LOAD mode
				cdp1802.mode = CDP1802_MODE_LOAD;
				break;

			case CDP1802_MODE_RESET:
				cdp1802.prevmode = CDP1802_MODE_RUN;
				cdp1802.state = CDP1802_STATE_1_INIT;
				cdp1802_run();
				break;

			case CDP1802_MODE_PAUSE:
				cdp1802.prevmode = CDP1802_MODE_RUN;
				cdp1802.state = CDP1802_STATE_0_FETCH;
				cdp1802_run();
				break;

			case CDP1802_MODE_RUN:
				cdp1802_run();
				break;
			}
			break;
		}
	}
	while (cdp1802_icount > 0);

	return cycles - cdp1802_icount;
}

static void cdp1802_set_interrupt_line(int state)
{
	cdp1802.irq = state;
}

static void cdp1802_set_dmain_line(int state)
{
	cdp1802.dmain = state;
}

static void cdp1802_set_dmaout_line(int state)
{
	cdp1802.dmaout = state;
}

void cdp1802_set_irq_line(int irqline, int state) {
	switch (irqline) {
		case CDP1802_INPUT_LINE_INT:    cdp1802_set_interrupt_line(state); break;
		case CDP1802_INPUT_LINE_DMAIN:  cdp1802_set_dmain_line(state);     break;
		case CDP1802_INPUT_LINE_DMAOUT: cdp1802_set_dmaout_line(state);    break;
	}
}

void cdp1802_set_irq_callback(int (*callback)(int irqline)) {
	// no callbacks
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
	case CPU_INFO_REG+CDP1802_R0: sprintf(buffer[which],"R0:%.4x",r->r[0]);break;
	case CPU_INFO_REG+CDP1802_R1: sprintf(buffer[which],"R1:%.4x",r->r[1]);break;
	case CPU_INFO_REG+CDP1802_R2: sprintf(buffer[which],"R2:%.4x",r->r[2]);break;
	case CPU_INFO_REG+CDP1802_R3: sprintf(buffer[which],"R3:%.4x",r->r[3]);break;
	case CPU_INFO_REG+CDP1802_R4: sprintf(buffer[which],"R4:%.4x",r->r[4]);break;
	case CPU_INFO_REG+CDP1802_R5: sprintf(buffer[which],"R5:%.4x",r->r[5]);break;
	case CPU_INFO_REG+CDP1802_R6: sprintf(buffer[which],"R6:%.4x",r->r[6]);break;
	case CPU_INFO_REG+CDP1802_R7: sprintf(buffer[which],"R7:%.4x",r->r[7]);break;
	case CPU_INFO_REG+CDP1802_R8: sprintf(buffer[which],"R8:%.4x",r->r[8]);break;
	case CPU_INFO_REG+CDP1802_R9: sprintf(buffer[which],"R9:%.4x",r->r[9]);break;
	case CPU_INFO_REG+CDP1802_Ra: sprintf(buffer[which],"Ra:%.4x",r->r[0xa]);break;
	case CPU_INFO_REG+CDP1802_Rb: sprintf(buffer[which],"Rb:%.4x",r->r[0xb]);break;
	case CPU_INFO_REG+CDP1802_Rc: sprintf(buffer[which],"Rc:%.4x",r->r[0xc]);break;
	case CPU_INFO_REG+CDP1802_Rd: sprintf(buffer[which],"Rd:%.4x",r->r[0xd]);break;
	case CPU_INFO_REG+CDP1802_Re: sprintf(buffer[which],"Re:%.4x",r->r[0xe]);break;
	case CPU_INFO_REG+CDP1802_Rf: sprintf(buffer[which],"Rf:%.4x",r->r[0xf]);break;
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
	case CPU_INFO_VERSION: return "2.0";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (c) 2008 Peter Trauner, all rights reserved.";
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
