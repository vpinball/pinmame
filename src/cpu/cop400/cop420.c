/**************************************************************************
 *               National Semiconductor COP420 Emulator                   *
 *                                                                        *
 *                   Copyright (C) 2006 MAME Team                         *
 **************************************************************************/

/*

    TODO:

    - serial I/O

*/

#include "driver.h"
#include "core.h"
#include "state.h"
#include "osd_cpu.h"
#include "mamedbg.h"
#include "cop400.h"

/* Layout of the registers in the debugger */
static UINT8 cop420_reg_layout[] = {
	COP400_PC, COP400_A, COP400_B, COP400_G, COP400_EN, COP400_Q, 0xFF,
  COP400_SA, COP400_SB, COP400_SC, COP400_SIO, COP400_SKL, COP400_T, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 cop420_win_layout[] = {
	25, 0,55, 2,	/* register window (top, right rows) */
	 0, 0,24,22,	/* disassembler window (left colums) */
	25, 3,55,10,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/* The opcode table now is a combination of cycle counts and function pointers */
typedef struct {
	unsigned cycles;
	void (*function) (void);
}	s_opcode;

static COP420_Regs R;
int    cop420_ICount;

static int InstLen[256];
static int LBIops[256];
static int LBIops33[256];

static mame_timer *cop420_counter_timer;

#include "420ops.c"

static s_opcode cop420_opcode_op23[256]=
{
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, ldd			},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},{1, ldd		},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, xad			},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},{1, xad		},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},
	{1, illegal 	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	},{1, illegal	}
};

static s_opcode cop420_opcode_op33[256]=
{
	{1, inil 		},{1, skgbz0 	},{1, illegal 	},{1, skgbz2 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgbz1 	},{1, illegal 	},{1, skgbz3 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, skgz	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, inin 		},{1, illegal 	},{1, ing	 	},{1, illegal 	},{1, cqma	 	},{1, illegal 	},{1, inl	 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, omg	 	},{1, illegal 	},{1, camq	 	},{1, illegal 	},{1, obd	 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, ogi0	 	},{1, ogi1	 	},{1, ogi2	 	},{1, ogi3	 	},{1, ogi4	 	},{1, ogi5	 	},{1, ogi6	 	},{1, ogi7	 	},
	{1, ogi8	 	},{1, ogi9	 	},{1, ogi10	 	},{1, ogi11	 	},{1, ogi12	 	},{1, ogi13	 	},{1, ogi14	 	},{1, ogi15	 	},
	{1, lei0	 	},{1, lei1	 	},{1, lei2	 	},{1, lei3	 	},{1, lei4	 	},{1, lei5	 	},{1, lei6	 	},{1, lei7	 	},
	{1, lei8	 	},{1, lei9	 	},{1, lei10	 	},{1, lei11	 	},{1, lei12	 	},{1, lei13	 	},{1, lei14	 	},{1, lei15	 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, lbi0_1 	},{1, lbi0_2 	},{1, lbi0_3 	},{1, lbi0_4 	},{1, lbi0_5 	},{1, lbi0_6 	},{1, lbi0_7 	},
	{1, lbi0_8	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, lbi1_1 	},{1, lbi1_2 	},{1, lbi1_3 	},{1, lbi1_4 	},{1, lbi1_5 	},{1, lbi1_6 	},{1, lbi1_7 	},
	{1, lbi1_8	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, lbi2_1 	},{1, lbi2_2 	},{1, lbi2_3 	},{1, lbi2_4 	},{1, lbi2_5 	},{1, lbi2_6 	},{1, lbi2_7 	},
	{1, lbi2_8	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, lbi3_1 	},{1, lbi3_2 	},{1, lbi3_3 	},{1, lbi3_4 	},{1, lbi3_5 	},{1, lbi3_6 	},{1, lbi3_7 	},
	{1, lbi3_8	 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},
	{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	},{1, illegal 	}
};

static void cop420_op23(void)
{
	(*(cop420_opcode_op23[ROM(PC++)].function))();
}

static void cop420_op33(void)
{
	(*(cop420_opcode_op33[ROM(PC++)].function))();
}

static s_opcode cop420_opcode_main[256]=
{
	{1, clra		},{1, skmbz0	},{1, x_o_r		},{1, skmbz2		},{1, xis0		},{1, ld0		},{1, x0		},{1, xds0		},
	{1, lbi0_9		},{1, lbi0_10	},{1, lbi0_11	},{1, lbi0_12		},{1, lbi0_13	},{1, lbi0_14	},{1, lbi0_15	},{1, lbi0_0	},
	{1, casc		},{1, skmbz1	},{1, xabr		},{1, skmbz3		},{1, xis1		},{1, ld1		},{1, x1		},{1, xds1		},
	{1, lbi1_9		},{1, lbi1_10	},{1, lbi1_11	},{1, lbi1_12		},{1, lbi1_13	},{1, lbi1_14	},{1, lbi1_15	},{1, lbi1_0	},
	{1, skc			},{1, ske		},{1, sc		},{2, cop420_op23		},{1, xis2		},{1, ld2		},{1, x2		},{1, xds2 		},
	{1,	lbi2_9		},{1, lbi2_10	},{1, lbi2_11	},{1, lbi2_12		},{1, lbi2_13	},{1, lbi2_14	},{1, lbi2_15	},{1, lbi2_0	},
	{1, asc			},{1, add		},{1, rc		},{2, cop420_op33  	},{1, xis3		},{1, ld3		},{1, x3		},{1, xds3		},
	{1,	lbi3_9		},{1, lbi3_10	},{1, lbi3_11	},{1, lbi3_12		},{1, lbi3_13	},{1, lbi3_14	},{1, lbi3_15	},{1, lbi3_0	},
	{1, comp		},{1, skt		},{1, rmb2		},{1, rmb3			},{1, nop		},{1, rmb1		},{1, smb2		},{1, smb1		},
	{1,	ret			},{1, retsk		},{1, adt		},{1, smb3			},{1, rmb0		},{1, smb0		},{1, cba		},{1, xas		},
	{1, cab			},{1, aisc1		},{1, aisc2		},{1, aisc3			},{1, aisc4		},{1, aisc5		},{1, aisc6		},{1, aisc7		},
	{1, aisc8		},{1, aisc9		},{1, aisc10	},{1, aisc11		},{1, aisc12	},{1, aisc13	},{1, aisc14	},{1, aisc15	},
	{2, jmp0		},{2, jmp1		},{2, jmp2		},{2, jmp3			},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal   },
	{2, jsr0		},{2, jsr1		},{2, jsr2		},{2, jsr3			},{0, illegal	},{0, illegal	},{0, illegal	},{0, illegal	},
	{1, stii0		},{1, stii1		},{1, stii2		},{1, stii3			},{1, stii4		},{1, stii5		},{1, stii6		},{1, stii7		},
	{1, stii8		},{1, stii9		},{1, stii10	},{1, stii11		},{1, stii12	},{1, stii13	},{1, stii14	},{1, stii15	},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{2, lqid		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jp		},
	{1, jp			},{1, jp		},{1, jp		},{1, jp			},{1, jp		},{1, jp		},{1, jp		},{1, jid		}
};

/* 8 bit Binary Counter */

static void cop420_counter_tick(int n)
{
	R.counter++;

	if (R.counter > 1023)
	{
		R.counter = 0;
		R.timerlatch = 1;
	}
}

/* IN Latches */

static void cop420_inil_tick(int n)
{
	UINT8 in = IN_IN();
	int i;

	for (i = 0; i < 4; i++)
	{
		R.in[i] = (R.in[i] << 1) | BIT(in, i);

		if ((R.in[i] & 0x07) == 0x04) // 100
		{
			R.IL |= (1 << i);
		}
	}
}

/****************************************************************************
 * Initialize emulation
 ****************************************************************************/
void cop420_init(void)
{
	int i;
	int cpu = cpu_getactivecpu();

	memset(&R, 0, sizeof(COP420_Regs));
	R.G_mask = 0x0F;
	R.D_mask = 0x0F;

	cop420_counter_timer = timer_alloc(cop420_counter_tick);
	timer_adjust(cop420_counter_timer, TIME_IN_HZ(Machine->drv->cpu[cpu].cpu_clock), 0, TIME_IN_HZ(Machine->drv->cpu[cpu].cpu_clock));

	// serial and microbus timers not emulated

	for (i=0; i<256; i++) InstLen[i]=1;

	InstLen[0x60] = InstLen[0x61] = InstLen[0x62] = InstLen[0x63] =
	InstLen[0x68] = InstLen[0x69] = InstLen[0x6a] = InstLen[0x6b] =
	InstLen[0x33] = InstLen[0x23] = 2;

	for (i=0; i<256; i++) LBIops[i] = 0;
	for (i=0x08; i<0x10; i++) LBIops[i] = 1;
	for (i=0x18; i<0x20; i++) LBIops[i] = 1;
	for (i=0x28; i<0x30; i++) LBIops[i] = 1;
	for (i=0x38; i<0x40; i++) LBIops[i] = 1;

	for (i=0; i<256; i++) LBIops33[i] = 0;
	for (i=0x80; i<0xc0; i++) LBIops33[i] = 1;

	state_save_register_UINT16("cop410", cpu, "PC", &PC, 1);
	state_save_register_UINT16("cop410", cpu, "PREVPC", &prevPC, 1);
	state_save_register_UINT8("cop410", cpu, "A", &A, 1);
	state_save_register_UINT8("cop410", cpu, "B", &B, 1);
	state_save_register_UINT8("cop410", cpu, "C", &C, 1);
	state_save_register_UINT8("cop410", cpu, "EN", &EN, 1);
	state_save_register_UINT8("cop410", cpu, "G", &G, 1);
	state_save_register_UINT8("cop410", cpu, "Q", &Q, 1);
	state_save_register_UINT16("cop410", cpu, "SA", &SA, 1);
	state_save_register_UINT16("cop410", cpu, "SB", &SB, 1);
	state_save_register_UINT16("cop410", cpu, "SC", &SC, 1);
	state_save_register_UINT8("cop410", cpu, "SIO", &SIO, 1);
	state_save_register_UINT8("cop410", cpu, "SKL", &SKL, 1);
	state_save_register_UINT8("cop410", cpu, "skip", &skip, 1);
	state_save_register_UINT8("cop410", cpu, "skipLBI", &skipLBI, 1);
	state_save_register_UINT8("cop410", cpu, "timerlatch", &R.timerlatch, 1);
	state_save_register_UINT16("cop410", cpu, "counter", &R.counter, 1);
	state_save_register_UINT8("cop410", cpu, "G_mask", &R.G_mask, 1);
	state_save_register_UINT8("cop410", cpu, "D_mask", &R.D_mask, 1);
	state_save_register_UINT8("cop410", cpu, "RAM", R.R_RAM, 64);
}

/****************************************************************************
 * Reset registers to their initial values
 ****************************************************************************/
void cop420_reset(void *param)
{
	PC = 0;
	A = 0;
	B = 0;
	C = 0;
	OUT_D(0);
	EN = 0;
	WRITE_G(0);
	SKL = 1;

	R.counter = 0;
	R.timerlatch = 1;
}

/****************************************************************************
 * Shut down the CPU emulation
 ****************************************************************************/
void cop420_exit(void) {
	/* nothing to do */
}

void cop420_set_irq_line(int irqline, int state) {
	/* no IRQ line on the COP */
}

void cop420_set_irq_callback(int (*callback)(int)) {
	/* no IRQ line on the COP */
}

/****************************************************************************
 * Execute cycles CPU cycles. Return number of cycles really executed
 ****************************************************************************/
int cop420_execute(int cycles)
{
	cop420_ICount = cycles;

	do
	{
		unsigned opcode;

		prevPC = PC;

		CALL_MAME_DEBUG;

		opcode = ROM(PC);

		if (skipLBI == 1)
		{
			int is_lbi = 0;

			if (opcode == 0x33)
			{
				is_lbi = LBIops33[ROM(PC+1)];
			}
			else
			{
				is_lbi = LBIops[opcode];
			}

			if (is_lbi == 0)
			{
				skipLBI = 0;
			}
			else {
				cop420_ICount -= cop420_opcode_main[opcode].cycles;

				PC += InstLen[opcode];
			}
		}

		if (skipLBI == 0)
		{
			int inst_cycles = cop420_opcode_main[opcode].cycles;
			PC++;
			(*(cop420_opcode_main[opcode].function))();
			cop420_ICount -= inst_cycles;

			// check for interrupt
			if (BIT(EN, 1) && BIT(R.IL, 1))
			{
				opcode=ROM(PC);
				if (!((opcode >= 0x80 && opcode != 0xbf && opcode != 0xff) // jp
					|| (opcode >= 0x60 && opcode < 0x64) // jmp
					|| (opcode >= 0x68 && opcode < 0x6c))) { // jsr

					// store skip logic
					R.last_skip = skip;
					skip = 0;
	
					// push next PC
					PUSH(PC + 1);
	
					// jump to interrupt service routine
					PC = 0x0ff;
	
					// disable interrupt
					EN &= ~0x02;
				}

				R.IL &= ~2;
			}

			if (skip == 1) {
				opcode=ROM(PC);
				if (opcode == 0xbf || opcode == 0xff) // bqid or jid
				  cop420_ICount -= 1;
				else
				  cop420_ICount -= cop420_opcode_main[opcode].cycles;
				PC += InstLen[opcode];
				skip = 0;
			}
		}
		cop420_inil_tick(0);
	} while (cop420_ICount > 0);

	return cycles - cop420_ICount;
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned cop420_get_context (void *dst)
{
	if( dst )
		*(COP420_Regs*)dst = R;
	return sizeof(COP420_Regs);
}


/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void cop420_set_context (void *src)
{
	if( src ) {
		R = *(COP420_Regs*)src;
		change_pc16(PC);
	}
}

/****************************************************************************
 * Get a specific register
 ****************************************************************************/
unsigned cop420_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC: return PC;
		case REG_PREVIOUSPC: return prevPC;
		case COP400_PC: return PC;
		case COP400_A: return A;
		case COP400_B: return B;
		case COP400_C: return C;
		case COP400_G: return G;
		case COP400_EN: return EN;
		case COP400_Q: return Q;
		case COP400_SA: return SA;
		case COP400_SB: return SB;
		case COP400_SC: return SC;
		case COP400_SIO: return SIO;
		case COP400_SKL: return SKL;
		case COP400_T: return R.counter;
	}
	return 0;
}

/****************************************************************************
 * Set a specific register
 ****************************************************************************/
void cop420_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC: PC = val; change_pc16(PC); break;
		case REG_PREVIOUSPC: prevPC = val; break;
		case COP400_PC: PC = val; break;
		case COP400_A: A = val; break;
		case COP400_B: B = val; break;
		case COP400_C: C = val; break;
		case COP400_G: G = val; break;
		case COP400_EN: EN = val; break;
		case COP400_Q: Q = val; break;
		case COP400_SA: SA = val; break;
		case COP400_SB: SB = val; break;
		case COP400_SC: SC = val; break;
		case COP400_SIO: SIO = val; break;
		case COP400_SKL: SKL = val; break;
		case COP400_T: R.counter = val; break;
	}
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

const char *cop420_info(void *context, int regnum)
{
	static char buffer[12][47+1];
	static int which = 0;

	COP420_Regs *r = (COP420_Regs*)context;

	which = (which+1) % 12;
	buffer[which][0] = '\0';
	if( !context )
		r = &R;

	switch( regnum )
	{
		case CPU_INFO_REG + COP400_PC: sprintf(buffer[which], "PC:%03X", r->R_PC); break;
		case CPU_INFO_REG + COP400_A: sprintf(buffer[which], "A:%X", r->R_A); break;
		case CPU_INFO_REG + COP400_B: sprintf(buffer[which], "B:%02X", r->R_B); break;
		case CPU_INFO_REG + COP400_EN: sprintf(buffer[which], "EN:%X", r->R_EN); break;
		case CPU_INFO_REG + COP400_G: sprintf(buffer[which], "G:%X", r->R_G); break;
		case CPU_INFO_REG + COP400_Q: sprintf(buffer[which], "Q:%02X", r->R_Q); break;
		case CPU_INFO_REG + COP400_SA: sprintf(buffer[which], "SA:%03X", r->R_SA); break;
		case CPU_INFO_REG + COP400_SB: sprintf(buffer[which], "SB:%03X", r->R_SB); break;
		case CPU_INFO_REG + COP400_SC: sprintf(buffer[which], "SC:%03X", r->R_SC); break;
		case CPU_INFO_REG + COP400_SIO: sprintf(buffer[which], "SIO:%X", r->R_SIO); break;
		case CPU_INFO_REG + COP400_T: sprintf(buffer[which], "T:%03X", r->counter); break;

		case CPU_INFO_FLAGS: sprintf(buffer[which], "%c%c",
		  r->R_C ? 'C' : '.',
		  r->R_SKL ? 'S' : '.');
		  break;

		case CPU_INFO_NAME: return "COP420";
		case CPU_INFO_FAMILY: return "National Semiconductor COP420";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) 2006 MAME Team";
		case CPU_INFO_REG_LAYOUT: return (const char *)cop420_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char *)cop420_win_layout;
	}
	return buffer[which];
}

WRITE_HANDLER(cop420_internal_w) {
  if (offset < 64) {
    R.R_RAM[offset] = data;
  }
}

READ_HANDLER(cop420_internal_r) {
  return offset < 64 ? R.R_RAM[offset] : 0;
}

unsigned cop420_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmCOP420(buffer, pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
