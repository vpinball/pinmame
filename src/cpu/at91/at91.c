/*****************************************************************************
 *
 *	 at91.c
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

/******************************************************************************
 *  Notes:

	Todo:
	Lots of stuff is still missing. 
	No Thumb mode support, Co-processor & Swap instructions not implemented.
	Data Processing opcodes need cycle count adjustments (see page 194 of ARM7TDMI manual for instruction timing summary)
	System Mode, Abort Mode, Undefined Mode not implemented. Multi-cpu support untested, may not work.

	Differences from Arm 2/3 (6 also?)
	-Full 32 bit address support
	-PC no longer contains CPSR information, CPSR is own register now
	-New opcodes for CPSR transfer, Long Multiplication, Co-Processor support, and some others
	-New operation modes? (unconfirmed)

	Based heavily on arm core from MAME 0.76:
    *****************************************
	ARM 2/3/6 Emulation

	Todo:
	Software interrupts unverified (nothing uses them so far, but they should be ok)
	Timing - Currently very approximated, nothing relies on proper timing so far.
	IRQ timing not yet correct (again, nothing is affected by this so far).

	By Bryan McPhail (bmcphail@tendril.co.uk) and Phil Stroffolino
*****************************************************************************/

#include <stdio.h>
#include "at91.h"
#include "state.h"
#include "mamedbg.h"

#define READ8(addr)			cpu_read8(addr)
#define WRITE8(addr,data)	cpu_write8(addr,data)
#define READ16(addr)		cpu_read16(addr)
#define WRITE16(addr,data)	cpu_write16(addr,data)
#define READ32(addr)		cpu_read32(addr)
#define WRITE32(addr,data)	cpu_write32(addr,data)

#define AT91_DEBUG_CORE 0

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

enum
{
	eARM_MODE_USER	= 0x0,
	eARM_MODE_FIQ	= 0x1,
	eARM_MODE_IRQ	= 0x2,
	eARM_MODE_SVC	= 0x3,

	kNumModes
};

/* There are 36 Unique - 32 bit processor registers */
/* Each mode has 17 registers (except user, which has 16) */
/* This is a list of each unique register */
/* todo: add missing modes registers */
enum
{
	/* All modes have the following */
	eR0=0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
	eR8,eR9,eR10,eR11,eR12,
	eR13, /* Stack Pointer */
	eR14, /* Link Register (holds return address) */
	eR15, /* Program Counter */
	eCPSR, /* Current Status Program Register */

	/* Fast Interrupt - Bank switched registers */
	eR8_FIQ,eR9_FIQ,eR10_FIQ,eR11_FIQ,eR12_FIQ,eR13_FIQ,eR14_FIQ,eSPSR_FIQ,

	/* IRQ - Bank switched registers*/
	eR13_IRQ,eR14_IRQ,eSPSR_IRQ,

	/* Supervisor/Service Mode - Bank switched registers*/
	eR13_SVC,eR14_SVC,eSPSR_SVC,

	kNumRegisters
};

/* 17 processor registers are visible at any given time,
 * banked depending on processor mode.
 */
static const int sRegisterTable[kNumModes][18] =
{
	{ /* USR */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13,eR14,
		eR15,eCPSR	//No SPSR in this mode
	},
	{ /* FIQ */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8_FIQ,eR9_FIQ,eR10_FIQ,eR11_FIQ,eR12_FIQ,
		eR13_FIQ,eR14_FIQ,
		eR15,eCPSR,eSPSR_FIQ
	},
	{ /* IRQ */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13_IRQ,eR14_IRQ,
		eR15,eCPSR,eSPSR_IRQ
	},
	{ /* SVC */
		eR0,eR1,eR2,eR3,eR4,eR5,eR6,eR7,
		eR8,eR9,eR10,eR11,eR12,
		eR13_SVC,eR14_SVC,
		eR15,eCPSR,eSPSR_SVC
	}
};

#define N_BIT	31
#define Z_BIT	30
#define C_BIT	29
#define V_BIT	28
#define I_BIT	7
#define F_BIT	6
#define T_BIT	5	//Thumb mode

#define N_MASK	((data32_t)(1<<N_BIT)) /* Negative flag */
#define Z_MASK	((data32_t)(1<<Z_BIT)) /* Zero flag */
#define C_MASK	((data32_t)(1<<C_BIT)) /* Carry flag */
#define V_MASK	((data32_t)(1<<V_BIT)) /* oVerflow flag */
#define I_MASK	((data32_t)(1<<I_BIT)) /* Interrupt request disable */
#define F_MASK	((data32_t)(1<<F_BIT)) /* Fast interrupt request disable */

#define N_IS_SET(pc)	((pc) & N_MASK)
#define Z_IS_SET(pc)	((pc) & Z_MASK)
#define C_IS_SET(pc)	((pc) & C_MASK)
#define V_IS_SET(pc)	((pc) & V_MASK)
#define I_IS_SET(pc)	((pc) & I_MASK)
#define F_IS_SET(pc)	((pc) & F_MASK)

#define N_IS_CLEAR(pc)	(!N_IS_SET(pc))
#define Z_IS_CLEAR(pc)	(!Z_IS_SET(pc))
#define C_IS_CLEAR(pc)	(!C_IS_SET(pc))
#define V_IS_CLEAR(pc)	(!V_IS_SET(pc))
#define I_IS_CLEAR(pc)	(!I_IS_SET(pc))
#define F_IS_CLEAR(pc)	(!F_IS_SET(pc))

#define R15						at91.sArmRegister[eR15]
#define SPSR					17	//SPSR is the 17th register in our array
//#define GET_CPSR				(GetRegister(eCPSR))
#define GET_CPSR				at91.sArmRegister[eCPSR]
//#define SET_CPSR(v)				(SetRegister(eCPSR,v))
#define SET_CPSR(v)				(GET_CPSR = (v))
//#define MODE					(modevar & 0x03)
#define MODE					(GET_CPSR & 0x03)
#define MODE_FLAG				0x3
#define SIGN_BIT				((data32_t)(1<<31))
#define SIGN_BITS_DIFFER(a,b)	(((a)^(b)) >> 31)

/* Deconstructing an instruction */

#define INSN_COND			((data32_t) 0xf0000000u)
#define INSN_SDT_L			((data32_t) 0x00100000u)
#define INSN_SDT_W			((data32_t) 0x00200000u)
#define INSN_SDT_B			((data32_t) 0x00400000u)
#define INSN_SDT_U			((data32_t) 0x00800000u)
#define INSN_SDT_P			((data32_t) 0x01000000u)
#define INSN_BDT_L			((data32_t) 0x00100000u)
#define INSN_BDT_W			((data32_t) 0x00200000u)
#define INSN_BDT_S			((data32_t) 0x00400000u)
#define INSN_BDT_U			((data32_t) 0x00800000u)
#define INSN_BDT_P			((data32_t) 0x01000000u)
#define INSN_BDT_REGS		((data32_t) 0x0000ffffu)
#define INSN_SDT_IMM		((data32_t) 0x00000fffu)
#define INSN_MUL_A			((data32_t) 0x00200000u)
#define INSN_MUL_RM			((data32_t) 0x0000000fu)
#define INSN_MUL_RS			((data32_t) 0x00000f00u)
#define INSN_MUL_RN			((data32_t) 0x0000f000u)
#define INSN_MUL_RD			((data32_t) 0x000f0000u)
#define INSN_I				((data32_t) 0x02000000u)
#define INSN_OPCODE			((data32_t) 0x01e00000u)
#define INSN_S				((data32_t) 0x00100000u)
#define INSN_BL				((data32_t) 0x01000000u)
#define INSN_BRANCH			((data32_t) 0x00ffffffu)
#define INSN_SWI			((data32_t) 0x00ffffffu)
#define INSN_RN				((data32_t) 0x000f0000u)
#define INSN_RD				((data32_t) 0x0000f000u)
#define INSN_OP2			((data32_t) 0x00000fffu)
#define INSN_OP2_SHIFT		((data32_t) 0x00000f80u)
#define INSN_OP2_SHIFT_TYPE	((data32_t) 0x00000070u)
#define INSN_OP2_RM			((data32_t) 0x0000000fu)
#define INSN_OP2_ROTATE		((data32_t) 0x00000f00u)
#define INSN_OP2_IMM		((data32_t) 0x000000ffu)
#define INSN_OP2_SHIFT_TYPE_SHIFT	4
#define INSN_OP2_SHIFT_SHIFT		7
#define INSN_OP2_ROTATE_SHIFT		8
#define INSN_MUL_RS_SHIFT			8
#define INSN_MUL_RN_SHIFT			12
#define INSN_MUL_RD_SHIFT			16
#define INSN_OPCODE_SHIFT			21
#define INSN_RN_SHIFT				16
#define INSN_RD_SHIFT				12
#define INSN_COND_SHIFT				28

enum
{
	OPCODE_AND,	/* 0000 */
	OPCODE_EOR,	/* 0001 */
	OPCODE_SUB,	/* 0010 */
	OPCODE_RSB,	/* 0011 */
	OPCODE_ADD,	/* 0100 */
	OPCODE_ADC,	/* 0101 */
	OPCODE_SBC,	/* 0110 */
	OPCODE_RSC,	/* 0111 */
	OPCODE_TST,	/* 1000 */
	OPCODE_TEQ,	/* 1001 */
	OPCODE_CMP,	/* 1010 */
	OPCODE_CMN,	/* 1011 */
	OPCODE_ORR,	/* 1100 */
	OPCODE_MOV,	/* 1101 */
	OPCODE_BIC,	/* 1110 */
	OPCODE_MVN	/* 1111 */
};

enum
{
	COND_EQ = 0,	/* Z: equal */
	COND_NE,		/* ~Z: not equal */
	COND_CS, COND_HS = 2,	/* C: unsigned higher or same */
	COND_CC, COND_LO = 3,	/* ~C: unsigned lower */
	COND_MI,		/* N: negative */
	COND_PL,		/* ~N: positive or zero */
	COND_VS,		/* V: overflow */
	COND_VC,		/* ~V: no overflow */
	COND_HI,		/* C && ~Z: unsigned higher */
	COND_LS,		/* ~C || Z: unsigned lower or same */
	COND_GE,		/* N == V: greater or equal */
	COND_LT,		/* N != V: less than */
	COND_GT,		/* ~Z && (N == V): greater than */
	COND_LE,		/* Z || (N != V): less than or equal */
	COND_AL,		/* always */
	COND_NV			/* never */
};

#define LSL(v,s) ((v) << (s))
#define LSR(v,s) ((v) >> (s))
#define ROL(v,s) (LSL((v),(s)) | (LSR((v),32u - (s))))
#define ROR(v,s) (LSR((v),(s)) | (LSL((v),32u - (s))))

/* Private Data */

/* sArmRegister defines the CPU state */
typedef struct
{
	data32_t sArmRegister[kNumRegisters];
	data8_t pendingIrq;
	data8_t pendingFiq;
} AT91_REGS;

static AT91_REGS at91;

int at91_ICount;

/* Prototypes */
static void HandleHalfWordDT(data32_t insn);
static void HandleSwap(data32_t insn);
static void HandlePSRTransfer( data32_t insn );
static void HandleALU( data32_t insn);
static void HandleMul( data32_t insn);
static void HandleUMulLong( data32_t insn);
static void HandleSMulLong( data32_t insn);
static void HandleBranch( data32_t insn);
static void HandleMemSingle( data32_t insn);
static void HandleMemBlock( data32_t insn);
static data32_t decodeShift( data32_t insn, data32_t *pCarry);
INLINE void SwitchMode( int );

/*static vars*/

//These all should be eventually moved into the cpu structure, so that multi cpu support can be handled properly
//static int modevar = 0;							//hold current operating mode
static int remap = 0;							//flag if remap of ram occurred
static data32_t *page0_ram_ptr;					//holder for the pointer set by the driver to ram
static data32_t *reset_ram_ptr;					//""
static READ32_HANDLER((*cs_r_callback));		//holder for the cs_r callback
static WRITE32_HANDLER((*cs_w_callback));		//holder for the cs_w callback
static offs_t cs_r_start, cs_r_end, cs_w_start, cs_w_end;
static WRITE32_HANDLER((*at91_ready_irq_cb));	

/* external interfaces */
void at91_set_ram_pointers(data32_t *reset_ram_ptrx, data32_t *page0_ram_ptrx)
{
	page0_ram_ptr = page0_ram_ptrx;
	reset_ram_ptr = reset_ram_ptrx;
}

/* for multi chip support - at cpu reset - these handlers should be copied to the appropriate place */
void at91_cs_callback_r(offs_t start, offs_t end, READ32_HANDLER((*callback)))
{
	cs_r_callback = callback;
	cs_r_start = start;
	cs_r_end = end;
}
void at91_cs_callback_w(offs_t start, offs_t end, WRITE32_HANDLER((*callback)))
{
	cs_w_callback = callback;
	cs_w_start = start;
	cs_w_end = end;
}

/* crappy hack to setup callback so core can notify driver when the timer/interrupt has been enabled */
/* totally implementation specific, and should be removed */
void at91_ready_irq_callback_w(WRITE32_HANDLER((*callback)))
{
	at91_ready_irq_cb = callback;
}

//Help debug the timer register data
static char temp[256];
char *GetTimerOffset(int addr)
{
	int offset = addr & 0x3f;
	switch(offset)
	{
		case 0x00:
		sprintf(temp,"Channel Control Register");
		break;
		case 0x04:
		sprintf(temp,"Channel Mode Register");
		break;
		case 0x08:
		case 0x0c:
		sprintf(temp,"Reserved");
		break;
		case 0x10:
		sprintf(temp,"Counter value");
		break;
		case 0x14:
		sprintf(temp,"Register A");
		break;
		case 0x18:
		sprintf(temp,"Register B");
		break;
		case 0x1c:
		sprintf(temp,"Register C");
		break;
		case 0x20:
		sprintf(temp,"Status Register");
		break;
		case 0x24:
		sprintf(temp,"Interrupt Enable");
		break;
		case 0x28:
		sprintf(temp,"Interrupt Disable");
		break;
		case 0x2c:
		sprintf(temp,"Interrupt Mask");
		break;
		default:
			sprintf(temp,"UNKNOWN");
	}
	return temp;
}

//WRITE TO  - Atmel AT91 CPU On Chip Periperhals
INLINE void internal_write (int addr, data32_t data)
{
	//EBI - External Bus Interface
	if(addr >= 0xffe00000 && addr <=0xffe03fff)
	{
		switch(addr) {
			//EBI_RCR - Remap Control Register
			case 0xffe00020:
				if(data & 1)
					{
						//Copy Reset RAM Contents into Page 0 RAM Address
						memcpy(page0_ram_ptr, reset_ram_ptr, 0x100000);
						remap = 1;
					}
				else
					LOG(("AT91-EBI_RCR = 0 (no effect)!\n"));
				break;
			default:
				LOG(("AT91-EBI WRITE: %08x = %08x\n",addr,data));
		}
	}
	else
	//Special Function
	if (addr >= 0xfff00000 && addr <=0xfff03fff)
	{
	}
	else
	//USART1
	if (addr >= 0xfffcc000 && addr <=0xfff0cfff)
	{
	}
	else
	//USART2
	if (addr >= 0xfffd0000 && addr <=0xfffd3fff)
	{
	}
	else
	//TC - Timer Counter
	if (addr >= 0xfffe0000 && addr <=0xfffe3fff)
	{
		int offset = addr & 0xff;
		if(offset < 0x40)
			LOG(("AT91-TIMER WRITE (TC Channel 0): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		else
		if(offset < 0x80)
			LOG(("AT91-TIMER WRITE (TC Channel 1): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		else
		if(offset < 0xc0)
			LOG(("AT91-TIMER WRITE (TC Channel 2): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		else
		if(offset == 0xc0)
			LOG(("AT91-TIMER WRITE (TC BLOCK CONTROL REGISTER): %08x = %08x\n",addr,data));
		else
		if(offset == 0xc4)
			LOG(("AT91-TIMER WRITE (TC BLOCK MODE REGISTER): %08x = %08x\n",addr,data));
		else
			LOG(("AT91-TIMER WRITE: %08x = %08x\n",addr,data));


		//hack to know that the timer 0 has started 
		if(addr == 0xfffe0000 &&  data == 0x04 && at91_ready_irq_cb)
		{
			at91_ready_irq_cb(0,1,0);
		}
	}
	else
	//PIO - Parallel I/O Controller
	if (addr >= 0xffff0000 && addr <=0xffff3fff)
	{
		switch(addr) {
			case 0xffff0000:
				LOG(("AT91-PER (PIO ENABLE REGISTER) WRITE: %08x = %08x\n",addr,data));
				break;
			case 0xffff0010:
				LOG(("AT91-OER (PIO OUTPUT ENABLE REGISTER) WRITE: %08x = %08x\n",addr,data));
				break;
			case 0xffff0034:
				LOG(("AT91-CIODR (PIO CLEAR OUTPUT DATA REGISTER) WRITE: %08x = %08x\n",addr,data));
				break;
			default:
				LOG(("AT91-PIO WRITE: %08x = %08x\n",addr,data));
		}
	}
	else
	//PS - Power Saving
	if (addr >= 0xffff4000 && addr <=0xffff7fff)
	{
		LOG(("AT91-POWER SAVER WRITE: %08x = %08x\n",addr,data));
	}
	else
	//WD - Watchdog Timer
	if (addr >= 0xffff8000 && addr <=0xffffbfff)
	{
		LOG(("AT91-WATCHDOG WRITE: %08x = %08x\n",addr,data));
	}
	else
	//AIC - Advanced Interrupt Controller
	if (addr >= 0xfffff000 && addr <=0xffffffff)
	{
		if(addr != 0xfffff100 && addr != 0xfffff130)
		LOG(("AT91-AIC WRITE: %08x = %08x\n",addr,data));
	}
	else
		LOG(("AT91-OCP WRITE: %08x = %08x\n",addr,data));
}

//READ FROM  - Atmel AT91 CPU On Chip Periperhals
INLINE data32_t internal_read (int addr)
{
	data32_t data = 0;
	//TC - Timer Counter
	if (addr >= 0xfffe0000 && addr <=0xfffe3fff)
	{
		int offset = addr & 0xff;
		if(offset < 0x40){
			if(addr !=0xfffe0020)
			LOG(("AT91-TIMER READ (TC Channel 0): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		}
		else
		if(offset < 0x80)
			LOG(("AT91-TIMER READ (TC Channel 1): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		else
		if(offset < 0xc0)
			LOG(("AT91-TIMER READ (TC Channel 2): %s (%08x) = %08x\n",GetTimerOffset(addr),addr,data));		
		else
		if(offset == 0xc0)
			LOG(("AT91-TIMER READ (TC BLOCK CONTROL REGISTER): %08x = %08x\n",addr,data));
		else
		if(offset == 0xc4)
			LOG(("AT91-TIMER READ (TC BLOCK MODE REGISTER): %08x = %08x\n",addr,data));
		else
			LOG(("AT91-TIMER READ: %08x = %08x\n",addr,data));
	}
	else
	//AIC - Advanced Interrupt Controller
	if (addr >= 0xfffff000 && addr <=0xffffffff)
	{
		//AIC Interrupt Vector Register - Based on current IRQ source 0-31, returns value of Source Vector 
		if(addr == 0xfffff100){
			//harded coded for timer 1 - totally cheating here
			data = 0x3af8;
			//LOG(("AT91-AIC_IVR READ: %08x = %08x\n",addr,data));
		}
		else
			LOG(("AT91-AIC READ: %08x = %08x\n",addr,data));
	}
	else
		LOG(("AT91-OCP READ: %08x = %08x\n",addr,data));
	return data;
}

/***************************************************************************/

INLINE void cpu_write32( int addr, data32_t data )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
	{
		internal_write(addr,data);
		return;
	}

	//If callback defined and remap has occurred - call it!
	if(cs_w_callback && remap && addr >= cs_w_start && addr <= cs_w_end)
	{
		cs_w_callback(addr,data,0xffffffff);
		return;
	}

	//Call normal 32 bit handler
	cpu_writemem32ledw_dword(addr,data);

	/* Unaligned writes are treated as normal writes */
	#if AT91_DEBUG_CORE
		if(addr&3)
			LOG(("%08x: Unaligned write %08x\n",R15,addr));
	#endif
}


INLINE void cpu_write16( int addr, data16_t data )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
	{
		internal_write(addr,(data32_t)data);
		return;
	}

	//If callback defined and remap has occurred - call it!
	if(cs_w_callback && remap && addr >= cs_w_start && addr <= cs_w_end)
	{
		cs_w_callback(addr,data,0x0000ffff);
		return;
	}

	//Call normal 16 bit handler ( for 32 bit cpu )
	cpu_writemem32ledw_word(addr,data);
}

INLINE void cpu_write8( int addr, data8_t data )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
	{
		internal_write(addr,(data32_t)data);
		return;
	}

	//If callback defined and remap has occurred - call it!
	if(cs_w_callback && remap && addr >= cs_w_start && addr <= cs_w_end)
	{
		cs_w_callback(addr,data,0x000000ff);
		return;
	}

	//Call normal 8 bit handler ( for 32 bit cpu )
	cpu_writemem32ledw(addr,data);
}

INLINE data32_t cpu_read32( int addr )
{
	data32_t result = 0;

	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
		result = internal_read(addr);
	else
	//If callback defined and remap has occurred - call it!
	if(cs_r_callback && remap && addr >= cs_r_start && addr <= cs_r_end)
		result = cs_r_callback(addr,0xffffffff);
	else
	//Handle through normal 32 bit handler
	result = cpu_readmem32ledw_dword(addr);

	/* Unaligned reads rotate the word, they never combine words */
	if (addr&3) {
		#if AT91_DEBUG_CORE
			if(addr&1)
				LOG(("%08x: Unaligned byte read %08x\n",R15,addr));
		#endif

		if ((addr&3)==3)
			return ((result&0x000000ff)<<24)|((result&0xffffff00)>> 8);
		if ((addr&3)==2)
			return ((result&0x0000ffff)<<16)|((result&0xffff0000)>>16);
		if ((addr&3)==1)
			return ((result&0x00ffffff)<< 8)|((result&0xff000000)>>24);
	}
	return result;
}

INLINE data16_t cpu_read16( int addr )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
		return (data16_t)internal_read(addr);

	//If callback defined and remap has occurred - call it!
	if(cs_r_callback && remap && addr >= cs_r_start && addr <= cs_r_end)
		return cs_r_callback(addr,0x0000ffff);

	if(addr&3)
	{
		int val = addr & 3;
		if(val != 2)
			LOG(("%08x: MISALIGNED half word read @ %08x:\n",R15,addr));
	}

	//Handle through normal 32 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw_word(addr);
}

INLINE data8_t cpu_read8( offs_t addr )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
		return (data8_t)internal_read(addr);

	//If callback defined and remap has occurred - call it!
	if(cs_r_callback && remap && addr >= cs_r_start && addr <= cs_r_end)
		return cs_r_callback(addr,0x000000ff);

	//Handle through normal 8 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw(addr);
}

INLINE data32_t GetRegister( int rIndex )
{
	return at91.sArmRegister[sRegisterTable[MODE][rIndex]];
}

INLINE void SetRegister( int rIndex, data32_t value )
{
	at91.sArmRegister[sRegisterTable[MODE][rIndex]] = value;
}

INLINE void SwitchMode (int mode)
{
	data32_t cspr = GET_CPSR & ~MODE_FLAG;
	SET_CPSR(cspr | mode);
}

/***************************************************************************/

void at91_reset(void *param)
{
	memset(&at91, 0, sizeof(at91));

	/* start up in SVC mode with interrupts disabled. */
	SET_CPSR(eARM_MODE_SVC|I_MASK|F_MASK);
	R15 = 0;
}

void at91_exit(void)
{
	/* nothing to do here */
}

int at91_execute( int cycles )
{
	data32_t pc;
	data32_t insn;

	at91_ICount = cycles;
	do
	{

#ifdef MAME_DEBUG
		if (mame_debug)
			MAME_Debug();
#endif

		/* load 32 bit instruction */
		pc = R15;
		insn = READ32( pc );

		//debug only
#ifdef MAME_DEBUG
		if(pc == 0x33d8)
			pc = pc;
#endif

		/* process condition codes for this instruction */
		switch (insn >> INSN_COND_SHIFT)
		{
		case COND_EQ:
			if (Z_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_NE:
			if (Z_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_CS:
			if (C_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_CC:
			if (C_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_MI:
			if (N_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_PL:
			if (N_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_VS:
			if (V_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_VC:
			if (V_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_HI:
			if (C_IS_CLEAR(GET_CPSR) || Z_IS_SET(GET_CPSR)) goto L_Next;
			break;
		case COND_LS:
			if (C_IS_SET(GET_CPSR) && Z_IS_CLEAR(GET_CPSR)) goto L_Next;
			break;
		case COND_GE:
			if (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK)) goto L_Next; /* Use x ^ (x >> ...) method */
			break;
		case COND_LT:
			if (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK)) goto L_Next;
			break;
		case COND_GT:
			if (Z_IS_SET(GET_CPSR) || (!(GET_CPSR & N_MASK) != !(GET_CPSR & V_MASK))) goto L_Next;
			break;
		case COND_LE:
			if (Z_IS_CLEAR(GET_CPSR) && (!(GET_CPSR & N_MASK) == !(GET_CPSR & V_MASK))) goto L_Next;
			break;
		case COND_NV:
			goto L_Next;
		}
		
		/*******************************************************************/
		/* If we got here - condition satisfied, so decode the instruction */
		/*******************************************************************/

		/* Branch and Exchange (BX) */
		if( (insn&0x0ffffff0)==0x012fff10 )		//bits 27-4 == 000100101111111111110001
		{ 
			R15 = GetRegister(insn & 0x0f);
			//If new PC address has A0 set, switch to Thumb mode
			if(R15 & 1) {
				//SET_CPSR(GET_CPSR|T_BIT);
				LOG(("%08x: Setting Thumb Mode due to R15 change to %08x - but not supported\n",pc,R15));
			}
		}
		else
		/* Multiply OR Swap OR Half Word Data Transfer */
		if( (insn & 0x0e000000)==0 && (insn & 0x80) && (insn & 0x10) )	//bits 27-25 == 000, bit 7=1, bit 4=1
		{
			/* Half Word Data Transfer */
			if(insn & 0x60)			//bits = 6-5 != 00
			{
				HandleHalfWordDT(insn);
			}
			else 
			/* Swap */
			if(insn & 0x01000000)	//bit 24 = 1
			{
				HandleSwap(insn);
			}
			/* Multiply Or Multiply Long */
			else
			{
				/* multiply long */
				if( insn&0x800000 )	//Bit 23 = 1 for Multiply Long
				{
					/* Signed? */
					if( insn&0x00400000 )
						HandleSMulLong(insn);
					else
						HandleUMulLong(insn);
				}
				/* multiply */
				else
				{
					HandleMul(insn);
				}
				R15 += 4;
			}
		}
		else
		/* Data Processing OR PSR Transfer */
		if( (insn & 0x0c000000) ==0 )	//bits 27-26 == 00 - This check can only exist properly after Multiplication check above
		{
			/* PSR Transfer (MRS & MSR) */
			if( ((insn&0x0100000)==0) && ((insn&0x01800000)==0x01000000) ) //( S bit must be clear, and bit 24,23 = 10 )
			{
				HandlePSRTransfer(insn);
				at91_ICount += 2;		//PSR only takes 1 - S Cycle, so we add + 2, since at end, we -3..
				R15 += 4;
			}
			/* Data Processing */
			else
			{
				HandleALU(insn);
			}
		}
		else
		/* Data Transfer - Single Data Access */
		if( (insn & 0x0c000000) == 0x04000000 )		//bits 27-26 == 01
		{
			HandleMemSingle(insn);
			R15 += 4;
		}
		else
		/* Block Data Transfer/Access */
		if( (insn & 0x0e000000) == 0x08000000 )		//bits 27-25 == 100
		{
			HandleMemBlock(insn);
			R15 += 4;
		}
		else
		/* Branch */
		if ((insn & 0x0e000000) == 0x0a000000)		//bits 27-25 == 101
		{
			HandleBranch(insn);
		}
		else
		/* Co-Processor Data Transfer */
		if( (insn & 0x0e000000) == 0x0c000000 )		//bits 27-25 == 110
		{
			LOG(("%08x: Co-Processer Data Transfer instructions not emulated yet!\n",R15));
			R15 += 4;
		}
		else
		/* Co-Processor Data Operation or Register Transfer */
		if( (insn & 0x0f000000) == 0x0e000000 )		//bits 27-24 == 1110
		{
			if(insn & 0x10)
				LOG(("%08x: Co-Processor Register Transfer instructions not emulated yet!\n",R15));
			else
				LOG(("%08x: Co-Processor Data Operation instructions not emulated yet!\n",R15));
			R15 += 4;
		}
		else
		/* Software Interrupt */
		if( (insn & 0x0f000000) == 0x0f000000 )	//bits 27-24 == 1111
		{
			pc=R15+4;
			SwitchMode(eARM_MODE_SVC);		/* Set SVC mode so PC is saved to correct R14 bank */
			SetRegister( 14, pc );			/* save PC */
			SetRegister( SPSR, GET_CPSR );	/* Save current CPSR */
			R15 = 0x8;
		}
		else
		/* Undefined */
		{
			LOG(("%08x:  Undefined instruction\n",R15));

			L_Next:
				at91_ICount -= 1;	//undefined takes 4 cycles
				R15 += 4;
		}

		at91_ICount -= 3;

	} while( at91_ICount > 0 );

	return cycles - at91_ICount;
} /* at91_execute */


unsigned at91_get_context(void *dst)
{
	if( dst )
	{
		memcpy( dst, &at91, sizeof(at91) );
	}
	return sizeof(at91);
}

void at91_set_context(void *src)
{
	if (src)
	{
		memcpy( &at91, src, sizeof(at91) );
	}
}

unsigned at91_get_pc(void)
{
	return R15;
}

void at91_set_pc(unsigned val)
{
	R15 = val;
}

unsigned at91_get_sp(void)
{
	return GetRegister(13);
}

void at91_set_sp(unsigned val)
{
	SetRegister(13,val);
}

unsigned at91_get_reg(int regnum)
{
	switch( regnum )
	{
	case AT9132_R0: return at91.sArmRegister[ 0];
	case AT9132_R1: return at91.sArmRegister[ 1];
	case AT9132_R2: return at91.sArmRegister[ 2];
	case AT9132_R3: return at91.sArmRegister[ 3];
	case AT9132_R4: return at91.sArmRegister[ 4];
	case AT9132_R5: return at91.sArmRegister[ 5];
	case AT9132_R6: return at91.sArmRegister[ 6];
	case AT9132_R7: return at91.sArmRegister[ 7];
	case AT9132_R8: return at91.sArmRegister[ 8];
	case AT9132_R9: return at91.sArmRegister[ 9];
	case AT9132_R10: return at91.sArmRegister[10];
	case AT9132_R11: return at91.sArmRegister[11];
	case AT9132_R12: return at91.sArmRegister[12];
	case AT9132_R13: return at91.sArmRegister[13];
	case AT9132_R14: return at91.sArmRegister[14];
	case AT9132_R15: return at91.sArmRegister[15];
	case AT9132_CPSR: return at91.sArmRegister[eCPSR];

	case AT9132_FR8: return	at91.sArmRegister[eR8_FIQ];
	case AT9132_FR9:	return at91.sArmRegister[eR9_FIQ];
	case AT9132_FR10: return at91.sArmRegister[eR10_FIQ];
	case AT9132_FR11: return at91.sArmRegister[eR11_FIQ];
	case AT9132_FR12: return at91.sArmRegister[eR12_FIQ];
	case AT9132_FR13: return at91.sArmRegister[eR13_FIQ];
	case AT9132_FR14: return at91.sArmRegister[eR14_FIQ];
    case AT9132_FSPSR: return at91.sArmRegister[eSPSR_FIQ];
	case AT9132_IR13: return at91.sArmRegister[eR13_IRQ];
	case AT9132_IR14: return at91.sArmRegister[eR14_IRQ];
    case AT9132_ISPSR: return at91.sArmRegister[eSPSR_IRQ];
	case AT9132_SR13: return at91.sArmRegister[eR13_SVC];
	case AT9132_SR14: return at91.sArmRegister[eR14_SVC];
	case AT9132_SSPSR: return at91.sArmRegister[eSPSR_SVC];
	case REG_PC: return at91.sArmRegister[15];
	}

	return 0;
}

void at91_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
	case AT9132_R0: at91.sArmRegister[ 0]= val; break;
	case AT9132_R1: at91.sArmRegister[ 1]= val; break;
	case AT9132_R2: at91.sArmRegister[ 2]= val; break;
	case AT9132_R3: at91.sArmRegister[ 3]= val; break;
	case AT9132_R4: at91.sArmRegister[ 4]= val; break;
	case AT9132_R5: at91.sArmRegister[ 5]= val; break;
	case AT9132_R6: at91.sArmRegister[ 6]= val; break;
	case AT9132_R7: at91.sArmRegister[ 7]= val; break;
	case AT9132_R8: at91.sArmRegister[ 8]= val; break;
	case AT9132_R9: at91.sArmRegister[ 9]= val; break;
	case AT9132_R10: at91.sArmRegister[10]= val; break;
	case AT9132_R11: at91.sArmRegister[11]= val; break;
	case AT9132_R12: at91.sArmRegister[12]= val; break;
	case AT9132_R13: at91.sArmRegister[13]= val; break;
	case AT9132_R14: at91.sArmRegister[14]= val; break;
	case AT9132_R15: at91.sArmRegister[15]= val; break;
	case AT9132_CPSR: SET_CPSR(val); break;
	case AT9132_FR8: at91.sArmRegister[eR8_FIQ] = val; break;
	case AT9132_FR9: at91.sArmRegister[eR9_FIQ] = val; break;
	case AT9132_FR10: at91.sArmRegister[eR10_FIQ] = val; break;
	case AT9132_FR11: at91.sArmRegister[eR11_FIQ] = val; break;
	case AT9132_FR12: at91.sArmRegister[eR12_FIQ] = val; break;
	case AT9132_FR13: at91.sArmRegister[eR13_FIQ] = val; break;
	case AT9132_FR14: at91.sArmRegister[eR14_FIQ] = val; break;
	case AT9132_FSPSR: at91.sArmRegister[eSPSR_FIQ] = val; break;
	case AT9132_IR13: at91.sArmRegister[eR13_IRQ] = val; break;
	case AT9132_IR14: at91.sArmRegister[eR14_IRQ] = val; break;
	case AT9132_ISPSR: at91.sArmRegister[eSPSR_IRQ] = val; break;
	case AT9132_SR13: at91.sArmRegister[eR13_SVC] = val; break;
	case AT9132_SR14: at91.sArmRegister[eR14_SVC] = val; break;
	case AT9132_SSPSR: at91.sArmRegister[eSPSR_SVC] = val; break;
	}
}

static void at91_check_irq_state(void)
{
	data32_t cpsr = GET_CPSR;	/* save current CPSR */
	data32_t pc = R15+4;		/* save old pc (already incremented in pipeline) */;

	/* Exception priorities:

		Reset
		Data abort
		FIRQ
		IRQ
		Prefetch abort
		Undefined instruction
		Software Interrupt
	*/

	if (at91.pendingFiq && (cpsr & F_MASK)==0) {
		SwitchMode(eARM_MODE_FIQ);				/* Set FIQ mode so PC is saved to correct R14 bank */
		SetRegister( 14, pc );					/* save PC to R14 */
		SetRegister( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK | F_MASK);	/* Mask both IRQ & FIRQ*/
		R15 = 0x1c;								/* IRQ Vector address */
		return;
	}

	if (at91.pendingIrq && (cpsr & I_MASK)==0) {
		SwitchMode(eARM_MODE_IRQ);				/* Set IRQ mode so PC is saved to correct R14 bank */
		SetRegister( 14, pc );					/* save PC to R14 */
		SetRegister( SPSR, cpsr );				/* Save current CPSR */
		SET_CPSR(GET_CPSR | I_MASK);			/* Mask IRQ */
		R15 = 0x18;								/* IRQ Vector address */
		return;
	}
}

void at91_set_nmi_line(int state)
{
}

void at91_set_irq_line(int irqline, int state)
{
	switch (irqline) {

	case AT91_IRQ_LINE: /* IRQ */
		if (state)
			at91.pendingIrq=1;
		else
			at91.pendingIrq=0;
		break;

	case AT91_FIRQ_LINE: /* FIRQ */
		if (state)
			at91.pendingFiq=1;
		else
			at91.pendingFiq=0;
		break;
	}

	at91_check_irq_state();
}

void at91_set_irq_callback(int (*callback)(int irqline))
{
}

static const data8_t at91_reg_layout[] =
{
	AT9132_R0, AT9132_SR13, -1,
	AT9132_R1, AT9132_SR14, -1,
	AT9132_R2, AT9132_SSPSR, -1,
	AT9132_R3, -2,  -1,
	AT9132_R4, AT9132_IR13,  -1,
	AT9132_R5, AT9132_IR14,  -1,
	AT9132_R6, AT9132_ISPSR, -1,
	AT9132_R7, -2,  -1,
	AT9132_R8, AT9132_FR8,   -1,
	AT9132_R9, AT9132_FR9,   -1,
	AT9132_R10,AT9132_FR10,  -1,
	AT9132_R11,AT9132_FR11,  -1,
	AT9132_R12,AT9132_FR12,  -1,
	AT9132_R13,AT9132_FR13,  -1,
	AT9132_R14,AT9132_FR14,  -1,
	AT9132_R15,AT9132_FSPSR,  -1,
	AT9132_CPSR, -1,
};

static const UINT8 at91_win_layout[] = {
	 0, 0,29,17,	/* register window (top rows) */
	30, 0,50,17,	/* disassembler window (left colums) */
	 0,18,48, 4,	/* memory #1 window (right, upper middle) */
	49,18,31, 4,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

const char *at91_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;

	AT91_REGS *pRegs = context;
	if( !context )
		pRegs = &at91;

	which = (which + 1) % 32;
	buffer[which][0] = '\0';

	switch( regnum )
	{
	case CPU_INFO_REG + AT9132_R0: sprintf( buffer[which], "R0  :%08x", pRegs->sArmRegister[ 0] );  break;
	case CPU_INFO_REG + AT9132_R1: sprintf( buffer[which], "R1  :%08x", pRegs->sArmRegister[ 1] );  break;
	case CPU_INFO_REG + AT9132_R2: sprintf( buffer[which], "R2  :%08x", pRegs->sArmRegister[ 2] );  break;
	case CPU_INFO_REG + AT9132_R3: sprintf( buffer[which], "R3  :%08x", pRegs->sArmRegister[ 3] );  break;
	case CPU_INFO_REG + AT9132_R4: sprintf( buffer[which], "R4  :%08x", pRegs->sArmRegister[ 4] );  break;
	case CPU_INFO_REG + AT9132_R5: sprintf( buffer[which], "R5  :%08x", pRegs->sArmRegister[ 5] );  break;
	case CPU_INFO_REG + AT9132_R6: sprintf( buffer[which], "R6  :%08x", pRegs->sArmRegister[ 6] );  break;
	case CPU_INFO_REG + AT9132_R7: sprintf( buffer[which], "R7  :%08x", pRegs->sArmRegister[ 7] );  break;
	case CPU_INFO_REG + AT9132_R8: sprintf( buffer[which], "R8  :%08x", pRegs->sArmRegister[ 8] );  break;
	case CPU_INFO_REG + AT9132_R9: sprintf( buffer[which], "R9  :%08x", pRegs->sArmRegister[ 9] );  break;
	case CPU_INFO_REG + AT9132_R10:sprintf( buffer[which], "R10 :%08x", pRegs->sArmRegister[10] );  break;
	case CPU_INFO_REG + AT9132_R11:sprintf( buffer[which], "R11 :%08x", pRegs->sArmRegister[11] );  break;
	case CPU_INFO_REG + AT9132_R12:sprintf( buffer[which], "R12 :%08x", pRegs->sArmRegister[12] );  break;
	case CPU_INFO_REG + AT9132_R13:sprintf( buffer[which], "R13 :%08x", pRegs->sArmRegister[13] );  break;
	case CPU_INFO_REG + AT9132_R14:sprintf( buffer[which], "R14 :%08x", pRegs->sArmRegister[14] );  break;
	case CPU_INFO_REG + AT9132_R15:sprintf( buffer[which], "R15 :%08x", pRegs->sArmRegister[15] );  break;
	case CPU_INFO_REG + AT9132_CPSR:sprintf( buffer[which], "R16 :%08x", pRegs->sArmRegister[eCPSR] );  break;
	case CPU_INFO_REG + AT9132_FR8: sprintf( buffer[which], "FR8 :%08x", pRegs->sArmRegister[eR8_FIQ] );  break;
	case CPU_INFO_REG + AT9132_FR9: sprintf( buffer[which], "FR9 :%08x", pRegs->sArmRegister[eR9_FIQ] );  break;
	case CPU_INFO_REG + AT9132_FR10:sprintf( buffer[which], "FR10:%08x", pRegs->sArmRegister[eR10_FIQ] );  break;
	case CPU_INFO_REG + AT9132_FR11:sprintf( buffer[which], "FR11:%08x", pRegs->sArmRegister[eR11_FIQ]);  break;
	case CPU_INFO_REG + AT9132_FR12:sprintf( buffer[which], "FR12:%08x", pRegs->sArmRegister[eR12_FIQ] );  break;
	case CPU_INFO_REG + AT9132_FR13:sprintf( buffer[which], "FR13:%08x", pRegs->sArmRegister[eR13_FIQ] );  break;
	case CPU_INFO_REG + AT9132_FR14:sprintf( buffer[which], "FR14:%08x", pRegs->sArmRegister[eR14_FIQ] );  break;
    case CPU_INFO_REG + AT9132_FSPSR:sprintf( buffer[which], "FR16:%08x", pRegs->sArmRegister[eSPSR_FIQ] );  break;
	case CPU_INFO_REG + AT9132_IR13:sprintf( buffer[which], "IR13:%08x", pRegs->sArmRegister[eR13_IRQ] );  break;
	case CPU_INFO_REG + AT9132_IR14:sprintf( buffer[which], "IR14:%08x", pRegs->sArmRegister[eR14_IRQ] );  break;
    case CPU_INFO_REG + AT9132_ISPSR:sprintf( buffer[which], "IR16:%08x", pRegs->sArmRegister[eSPSR_IRQ] );  break;
	case CPU_INFO_REG + AT9132_SR13:sprintf( buffer[which], "SR13:%08x", pRegs->sArmRegister[eR13_SVC] );  break;
	case CPU_INFO_REG + AT9132_SR14:sprintf( buffer[which], "SR14:%08x", pRegs->sArmRegister[eR14_SVC] );  break;
	case CPU_INFO_REG + AT9132_SSPSR:sprintf( buffer[which], "SR16:%08x", pRegs->sArmRegister[eSPSR_SVC] );  break;

	case CPU_INFO_FLAGS:
		sprintf(buffer[which], "%c%c%c%c%c%c",
			(pRegs->sArmRegister[eCPSR] & N_MASK) ? 'N' : '-',
			(pRegs->sArmRegister[eCPSR] & Z_MASK) ? 'Z' : '-',
			(pRegs->sArmRegister[eCPSR] & C_MASK) ? 'C' : '-',
			(pRegs->sArmRegister[eCPSR] & V_MASK) ? 'V' : '-',
			(pRegs->sArmRegister[eCPSR] & I_MASK) ? 'I' : '-',
			(pRegs->sArmRegister[eCPSR] & F_MASK) ? 'F' : '-');
		//todo: adjust method for getting mode here
		switch (pRegs->sArmRegister[eCPSR] & 3)
		{
		case 0:
			strcat(buffer[which], " USER");
			break;
		case 1:
			strcat(buffer[which], " FIRQ");
			break;
		case 2:
			strcat(buffer[which], " IRQ ");
			break;
		default:
			strcat(buffer[which], " SVC ");
			break;
		}
		break;
	case CPU_INFO_NAME: 		return "AT91";
	case CPU_INFO_FAMILY:		return "Acorn Risc Machine";
	case CPU_INFO_VERSION:		return "1.0";
	case CPU_INFO_FILE: 		return __FILE__;
	case CPU_INFO_CREDITS:		return "Copyright 2004 Steve Ellenoff, sellenoff@hotmail.com";
	case CPU_INFO_REG_LAYOUT:	return (const char*)at91_reg_layout;
	case CPU_INFO_WIN_LAYOUT:	return (const char*)at91_win_layout;
	}

	return buffer[which];
}

unsigned at91_dasm(char *buffer, unsigned int pc)
{
#ifdef MAME_DEBUG
	at91_disasm( buffer, pc, cpu_read32(pc)); //&ADDRESS_MASK) );
	return 4;
#else
	sprintf(buffer, "$%08x", READ32(pc));
	return 4;
#endif
}

void at91_init(void)
{
	int cpu = cpu_getactivecpu(),i;
	char buf[8];

	for (i=0; i<kNumRegisters; i++) {
		sprintf(buf,"R%d",i);
		state_save_register_UINT32("at91", cpu, buf, &at91.sArmRegister[i], 4);
	}
	state_save_register_UINT8("at91", cpu, "IRQ", &at91.pendingIrq, 1);
	state_save_register_UINT8("at91", cpu, "FIQ", &at91.pendingFiq, 1);

	return;
}

/***************************************************************************/

static void HandleBranch(  data32_t insn )
{
	data32_t off = (insn & INSN_BRANCH) << 2;

	/* Save PC into LR if this is a branch with link */
	if (insn & INSN_BL)
	{
		SetRegister(14,R15 + 4);
	}

	/* Sign-extend the 24-bit offset in our calculations */
	if (off & 0x2000000u)
	{
		R15 -= ((~(off | 0xfc000000u)) + 1) - 8;
	}
	else
	{
		R15 += off + 8;
	}
}

static void HandleMemSingle( data32_t insn )
{
	data32_t rn, rnv, off, rd;

	/* Fetch the offset */
	if (insn & INSN_I)
	{
		/* Register Shift */
		off = decodeShift(insn, NULL);
	}
	else
	{
		/* Immediate Value */
		off = insn & INSN_SDT_IMM;
	}

	/* Calculate Rn, accounting for PC */
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;

	if (insn & INSN_SDT_P)
	{
		/* Pre-indexed addressing */
		if (insn & INSN_SDT_U)
		{
			rnv = (GetRegister(rn) + off);
		}
		else
		{
			rnv = (GetRegister(rn) - off);
		}

		if (insn & INSN_SDT_W)
		{
			SetRegister(rn,rnv);

	//check writeback???
		}
		else if (rn == eR15)
		{
			rnv = rnv + 8;
		}
	}
	else
	{
		/* Post-indexed addressing */
		if (rn == eR15)
		{
			rnv = R15 + 8;
		}
		else
		{
			rnv = GetRegister(rn);
		}
	}

	/* Do the transfer */
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if (insn & INSN_SDT_L)
	{
		/* Load */
		if (insn & INSN_SDT_B)
		{
			SetRegister(rd,(data32_t) READ8(rnv));
		}
		else
		{
			if (rd == eR15)
			{
				R15 = READ32(rnv);
				R15 -= 4;
				//LDR, PC takes 2S + 2N + 1I (5 total cycles)
				at91_ICount -= 2;
			}
			else
			{
				SetRegister(rd,READ32(rnv));
			}
		}
	}
	else
	{
		/* Store */
		if (insn & INSN_SDT_B)
		{
			#if AT91_DEBUG_CORE
				if(rd==eR15)
					LOG(("Wrote R15 in byte mode\n"));
			#endif

			WRITE8(rnv, (data8_t) GetRegister(rd) & 0xffu);
		}
		else
		{
			#if AT91_DEBUG_CORE
				if(rd==eR15)
					LOG(("Wrote R15 in 32bit mode\n"));
			#endif

			//WRITE32(rnv, rd == eR15 ? R15 + 8 : GetRegister(rd));
			WRITE32(rnv, rd == eR15 ? R15 + 8 + 4 : GetRegister(rd)); //manual says STR rd = PC, +12
		}
		//Store takes only 2 N Cycles, so add + 1
		at91_ICount += 1;
	}

	/* Do post-indexing writeback */
	if (!(insn & INSN_SDT_P)/* && (insn&INSN_SDT_W)*/)
	{
		if (insn & INSN_SDT_U)
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
				//todo: check for offs... ?
			}
			else {

				if ((insn&INSN_SDT_W)!=0)
					LOG(("%08x:  RegisterWritebackIncrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));

				SetRegister(rn,(rnv + off));
			}
		}
		else
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
			}
			else {
				SetRegister(rn,(rnv - off));

				if ((insn&INSN_SDT_W)!=0)
				LOG(("%08x:  RegisterWritebackDecrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			}
		}
	}

//	at91_check_irq_state()

} /* HandleMemSingle */

/* Set NZCV flags for ADDS / SUBS */

#define HandleALUAddFlags(rd, rn, op2) \
  if (insn & INSN_S) \
    SET_CPSR( \
      ((GET_CPSR &~ (N_MASK | Z_MASK | V_MASK | C_MASK)) \
      | (((!SIGN_BITS_DIFFER(rn, op2)) && SIGN_BITS_DIFFER(rn, rd)) \
          << V_BIT) \
      | (((~(rn)) < (op2)) << C_BIT) \
      | HandleALUNZFlags(rd))); \
  R15 += 4;

#define HandleALUSubFlags(rd, rn, op2) \
  if (insn & INSN_S) \
    SET_CPSR( \
      ((GET_CPSR &~ (N_MASK | Z_MASK | V_MASK | C_MASK)) \
      | ((SIGN_BITS_DIFFER(rn, op2) && SIGN_BITS_DIFFER(rn, rd)) \
          << V_BIT) \
      | (((op2) <= (rn)) << C_BIT) \
      | HandleALUNZFlags(rd))); \
  R15 += 4;

/* Set NZC flags for logical operations. */

//This macro (which I didn't write) - doesn't make it obvious that the SIGN BIT = 31, just as the N Bit does,
//therfore, N is set by default
#define HandleALUNZFlags(rd) \
  (((rd) & SIGN_BIT) | ((!(rd)) << Z_BIT))


//Long ALU Functions use bit 63 
#define HandleLongALUNZFlags(rd) \
  ((((rd) & ((UINT64)1<<63))>>32) | ((!(rd)) << Z_BIT))

#define HandleALULogicalFlags(rd, sc) \
  if (insn & INSN_S) \
    SET_CPSR( ((GET_CPSR &~ (N_MASK | Z_MASK | C_MASK)) \
                     | HandleALUNZFlags(rd) \
                     | (((sc) != 0) << C_BIT)));\
  R15 += 4;


static void HandleHalfWordDT(data32_t insn)
{
	data32_t rn, rnv, off, rd;

	//Immediate or Register Offset?
	if(insn & 0x400000) {				//Bit 22 - 1 = immediate, 0 = register
		//imm. value in high nibble (bits 8-11) and lo nibble (bit 0-3)
		off = (((insn>>8)&0x0f)<<4) | (insn&0x0f);
	}
    else {
		//register
		off = GetRegister(insn & 0x0f);
	}

	/* Calculate Rn, accounting for PC */
	rn = (insn & INSN_RN) >> INSN_RN_SHIFT;

	if (insn & INSN_SDT_P)
	{
		/* Pre-indexed addressing */
		if (insn & INSN_SDT_U)
		{
			rnv = (GetRegister(rn) + off);
		}
		else
		{
			rnv = (GetRegister(rn) - off);
		}

		if (insn & INSN_SDT_W)
		{
			SetRegister(rn,rnv);

		//check writeback???
		}
		else if (rn == eR15)
		{
			rnv = rnv + 8;
		}
	}
	else
	{
		/* Post-indexed addressing */
		if (rn == eR15)
		{
			rnv = R15 + 8;
		}
		else
		{
			rnv = GetRegister(rn);
		}
	}

	/* Do the transfer */
	rd = (insn & INSN_RD) >> INSN_RD_SHIFT;

	/* Load */
	if (insn & INSN_SDT_L)
	{
		//Signed?
		if(insn & 0x40)
		{
			data32_t newval = 0;

			//Signed Half Word?
			if(insn & 0x20) {
				data16_t signbyte,databyte;
				databyte = READ16(rnv) & 0xFFFF;
				signbyte = (databyte & 0x8000) ? 0xffff : 0;
				newval = (data32_t)(signbyte<<16)|databyte;
			}
			//Signed Byte
			else {
				data8_t databyte;
				data32_t signbyte;
				databyte = READ8(rnv) & 0xff;
				signbyte = (databyte & 0x80) ? 0xffffff : 0;
				newval = (data32_t)(signbyte<<8)|databyte;
			}

			//PC?
			if(rd == eR15)
			{
				R15 = newval + 8;
				//LDR(H,SH,SB) PC takes 2S + 2N + 1I (5 total cycles)
				at91_ICount -= 2;

			}
			else
			{
				SetRegister(rd,newval);
				R15 += 4;
			}
		}
		//Unsigned Half Word
		else
		{
			if (rd == eR15)
			{
				R15 = READ16(rnv) + 8;
			}
			else
			{
				SetRegister(rd,READ16(rnv));
				R15 += 4;
			}
		}
	}
	/* Store */
	else
	{
		//WRITE16(rnv, rd == eR15 ? R15 + 8 : GetRegister(rd));
		WRITE16(rnv, rd == eR15 ? R15 + 8 + 4 : GetRegister(rd)); //manual says STR RD=PC, +12 of address
		if(rn != eR15)
			R15 += 4;
		//STRH takes 2 cycles, so we add + 1
		at91_ICount += 1;
	}

	//SJE: No idea if this writeback code works or makes sense here..

	/* Do post-indexing writeback */
	if (!(insn & INSN_SDT_P)/* && (insn&INSN_SDT_W)*/)
	{
		if (insn & INSN_SDT_U)
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
				//todo: check for offs... ?
			}
			else {

				if ((insn&INSN_SDT_W)!=0)
					LOG(("%08x:  RegisterWritebackIncrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));

				SetRegister(rn,(rnv + off));
			}
		}
		else
		{
			/* Writeback is applied in pipeline, before value is read from mem,
				so writeback is effectively ignored */
			if (rd==rn) {
				SetRegister(rn,GetRegister(rd));
			}
			else {
				SetRegister(rn,(rnv - off));

				if ((insn&INSN_SDT_W)!=0)
					LOG(("%08x:  RegisterWritebackDecrement %d %d %d\n",R15,(insn & INSN_SDT_P)!=0,(insn&INSN_SDT_W)!=0,(insn & INSN_SDT_U)!=0));
			}
		}
	}
}

static void HandleSwap(data32_t insn)
{
	LOG(("%08x: Swap not implemented!\n",R15));
	R15 += 4;
}

static void HandlePSRTransfer( data32_t insn )
{
	int reg = (insn & 0x400000)?SPSR:eCPSR;	//Either CPSR or SPSR
	int val = 0;

	//MSR ( bit 21 set ) - Copy value to CPSR/SPSR
	if( (insn & 0x00200000) ) {
		//If in non-privelge mode - mask off control bits (which cannot be changed)
		//Todo -

		//Flag Bits Only? (Bit 16 Clear)
		if( (insn & 0x10000)==0) {
		//Todo - 
			LOG(("%08x: MSR - Flag bits only - not implemented!\n",R15));
		}

		//Immediate Value?
		if(insn & 0x02000000) {
			//Todo
			LOG(("%08x: MSR - Immediate value not implemented!\n",R15));
		}
		//Value from Register
		else {
			val = GetRegister(insn & 0x0f);
		}
		SetRegister(reg,val);
	}
	//MRS ( bit 21 clear ) - Copy CPSR or SPSR to specified Register
	else {
		SetRegister( (insn>>12)& 0x0f ,GetRegister(reg));
	}
}

static void HandleALU( data32_t insn )
{
	data32_t op2, sc=0, rd, rn, opcode;
	data32_t by, rdn;
//	data32_t oldMode=GET_CPSR&3;

	opcode = (insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT;

	rd = 0;
	rn = 0;

	/* --------------*/
	/* Construct Op2 */
	/* --------------*/

	/* Immediate constant */
	if (insn & INSN_I)
	{
		by = (insn & INSN_OP2_ROTATE) >> INSN_OP2_ROTATE_SHIFT;
		if (by)
		{
			op2 = ROR(insn & INSN_OP2_IMM, by << 1);
			sc = op2 & SIGN_BIT;
		}
		else
		{
			op2 = insn & INSN_OP2;
			sc = GET_CPSR & C_MASK;
		}
	}
	/* Op2 = Register Value */
	else
	{
		op2 = decodeShift(insn, (insn & INSN_S && (opcode & 4) == 4)? &sc : NULL);

		if (!(insn & INSN_S && (opcode & 4) == 4))
			sc=0;
	}

	/* Calculate Rn to account for pipelining */
	if ((opcode & 0xd) != 0xd) /* No Rn in MOV */
	{
		if ((rn = (insn & INSN_RN) >> INSN_RN_SHIFT) == eR15)
		{
			#if AT91_DEBUG_CORE
				LOG(("%08x:  Pipelined R15 (Shift %d)\n",R15,(insn&INSN_I?8:insn&0x10u?12:12)));
			#endif
			rn=R15+8;
		}
		else
		{
			rn = GetRegister(rn);
		}
	}

	/* Perform the operation */
	switch ((insn & INSN_OPCODE) >> INSN_OPCODE_SHIFT)
	{
	/* Arithmetic operations */
	case OPCODE_SBC:
		rd = (rn - op2 - (GET_CPSR & C_MASK ? 0 : 1));
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_CMP:
	case OPCODE_SUB:
		rd = (rn - op2);
		HandleALUSubFlags(rd, rn, op2);
		break;
	case OPCODE_RSC:
		rd = (op2 - rn - (GET_CPSR & C_MASK ? 0 : 1));
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_RSB:
		rd = (op2 - rn);
		HandleALUSubFlags(rd, op2, rn);
		break;
	case OPCODE_ADC:
		rd = (rn + op2 + ((GET_CPSR & C_MASK) >> C_BIT));
		HandleALUAddFlags(rd, rn, op2);
		break;
	case OPCODE_CMN:
	case OPCODE_ADD:
		rd = (rn + op2);
		HandleALUAddFlags(rd, rn, op2);
		break;

	/* Logical operations */
	case OPCODE_AND:
	case OPCODE_TST:
		rd = rn & op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_BIC:
		rd = rn &~ op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_TEQ:
	case OPCODE_EOR:
		rd = rn ^ op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_ORR:
		rd = rn | op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MOV:
		rd = op2;
		HandleALULogicalFlags(rd, sc);
		break;
	case OPCODE_MVN:
		rd = (~op2);
		HandleALULogicalFlags(rd, sc);
		break;
	}

	/* Put the result in its register if not a test */
	rdn = (insn & INSN_RD) >> INSN_RD_SHIFT;
	if ((opcode & 0xc) != 0x8)
	{
		if (rdn == eR15 && !(insn & INSN_S))
		{
			R15 = rd;

			/* Retain old mode regardless */
			#if AT91_DEBUG_CORE
				LOG(("%08x:  Suspected R15 mode change\n",R15));
			#endif		
		}
		else
		{
			if (rdn==eR15) {

				//Update CPSR from SPSR
				SET_CPSR(GetRegister(SPSR));

				SetRegister(rdn,rd);

				/* IRQ masks may have changed in this instruction */
//				at91_check_irq_state();
			}
			else
				/* S Flag is set - update PSR & mode */
				SetRegister(rdn,rd);
		}
	/* TST & TEQ can affect R15 (the condition code register) with the S bit set */
	} else if (rdn==eR15) {
		if (insn & INSN_S) {
			#if AT91_DEBUG_CORE
				LOG(("%08x: TST class on R15 s bit set\n",R15));
			#endif
			R15 = rd;

			/* IRQ masks may have changed in this instruction */
//			at91_check_irq_state();
		} else {
			#if	AT91_DEBUG_CORE
				LOG(("%08x: TST class on R15 no s bit set\n",R15));
			#endif
		}
	}
}

static void HandleMul( data32_t insn)
{
	data32_t r;

	/* Do the basic multiply of Rm and Rs */
	r =	GetRegister( insn&INSN_MUL_RM ) *
	  	GetRegister( (insn&INSN_MUL_RS)>>INSN_MUL_RS_SHIFT );

	#if AT91_DEBUG_CORE
	if( 
	    ((insn&INSN_MUL_RM)==0xf) || 
		(((insn&INSN_MUL_RS)>>INSN_MUL_RS_SHIFT )==0xf) ||
		(((insn&INSN_MUL_RN)>>INSN_MUL_RN_SHIFT)==0xf)
	   )
		LOG(("%08x:  R15 used in mult\n",R15));
	#endif

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		r += GetRegister((insn&INSN_MUL_RN)>>INSN_MUL_RN_SHIFT);
	}

	/* Write the result */
	SetRegister((insn&INSN_MUL_RD)>>INSN_MUL_RD_SHIFT,r);

	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleALUNZFlags(r));
	}
}

//todo: add proper cycle counts
static void HandleSMulLong( data32_t insn)
{
	INT32 rm, rs;
	data32_t rhi,rlo;
	INT64 res=0;

	rm  = (INT32)GetRegister(insn&0xf);
	rs  = (INT32)GetRegister(((insn>>8)&0xf));
	rhi = (insn>>16)&0xf;
	rlo = (insn>>12)&0xf;

	/* todo: check for use of R15 and log it to error as it's not allowed*/

	/* Perform the multiplication */
	res = (INT64)rm * rs;

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		INT64 acum = (INT64)((((INT64)(rhi))<<32) | rlo);
		res += acum;
	}

	/* Write the result (upper dword goes to RHi, lower to RLo) */
	SetRegister(rhi, res>>32);
	SetRegister(rlo, res & 0xFFFFFFFF);
	
	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleLongALUNZFlags(res));
	}
}

//todo: add proper cycle counts
static void HandleUMulLong( data32_t insn)
{
	UINT32 rm, rs;
	data32_t rhi,rlo;
	UINT64 res=0;

	rm  = (INT32)GetRegister(insn&0xf);
	rs  = (INT32)GetRegister(((insn>>8)&0xf));
	rhi = (insn>>16)&0xf;
	rlo = (insn>>12)&0xf;

	/* todo: check for use of R15 and log it to error as it's not allowed */

	/* Perform the multiplication */
	res = (UINT64)rm * rs;

	/* Add on Rn if this is a MLA */
	if (insn & INSN_MUL_A)
	{
		UINT64 acum = (UINT64)((((UINT64)(rhi))<<32) | rlo);
		res += acum;
	}

	/* Write the result (upper dword goes to RHi, lower to RLo) */
	SetRegister(rhi, res>>32);
	SetRegister(rlo, res & 0xFFFFFFFF);
	
	/* Set N and Z if asked */
	if( insn & INSN_S )
	{
		SET_CPSR ( (GET_CPSR &~ (N_MASK | Z_MASK)) | HandleLongALUNZFlags(res));
	}
}

static int loadInc ( data32_t pat, data32_t rbv, data32_t s)
{
	int i,result;

	result = 0;
	for( i=0; i<16; i++ )
	{
		if( (pat>>i)&1 )
		{
			if (i==15) {
				if (s) /* Pull full contents from stack */
					SetRegister( 15, READ32(rbv+=4) );
				else /* Pull only address, preserve mode & status flags */
					SetRegister( 15, READ32(rbv+=4) );
			} else
				SetRegister( i, READ32(rbv+=4) );

			result++;
		}
	}
	return result;
}

static int loadDec( data32_t pat, data32_t rbv, data32_t s)
{
	int i,result;

	result = 0;
	for( i=15; i>=0; i-- )
	{
		if( (pat>>i)&1 )
		{
			if (i==15) {
				if (s) /* Pull full contents from stack */
					SetRegister( 15, READ32(rbv-=4) );
				else /* Pull only address, preserve mode & status flags */
					SetRegister( 15, READ32(rbv-=4) );
			}
			else
				SetRegister( i, READ32(rbv -=4) );
			result++;
		}
	}
	return result;
}

static int storeInc( data32_t pat, data32_t rbv)
{
	int i,result;

	result = 0;
	for( i=0; i<16; i++ )
	{
		if( (pat>>i)&1 )
		{
			#if AT91_DEBUG_CORE
				if(i==15) /* R15 is plus 12 from address of STM */
					LOG(("%08x: StoreInc on R15\n",R15));
			#endif
			WRITE32( rbv += 4, GetRegister(i) );
			result++;
		}
	}
	return result;
} /* storeInc */

static int storeDec( data32_t pat, data32_t rbv)
{
	int i,result;

	result = 0;
	for( i=15; i>=0; i-- )
	{
		if( (pat>>i)&1 )
		{
			#if AT91_DEBUG_CORE
				if(i==15) /* R15 is plus 12 from address of STM */
					LOG(("%08x: StoreDec on R15\n",R15));
			#endif
			WRITE32( rbv -= 4, GetRegister(i) );
			result++;
		}
	}
	return result;
} /* storeDec */

static void HandleMemBlock( data32_t insn)
{
	data32_t rb = (insn & INSN_RN) >> INSN_RN_SHIFT;
	data32_t rbp = GetRegister(rb);
	int result;

	if(rbp & 3)
		LOG(("%08x: Unaligned Mem Transfer @ %08x\n",R15,rbp));

	//We will specify the cycle count for each case, so remove the -3 that occurs at the end
	at91_ICount +=3;

	if (insn & INSN_BDT_L)
	{
		/* Loading */
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp + (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				//todo: implement user bank transfer
				LOG(("%08x: User Bank Transfer not implemented!\n",R15));
			}

			result = loadInc( insn & 0xffff, rbp, insn&INSN_BDT_S );

			if (insn & INSN_BDT_W)
			{
				#if AT91_DEBUG_CORE
					if(rb==15)
						LOG(("%08x:  Illegal LDRM writeback to r15\n",R15));
				#endif
				SetRegister(rb,GetRegister(rb)+result*4);
			}

			//R15 included? (NOTE: CPSR restore must occur LAST otherwise wrong registers restored!)
			if (insn & 0x8000) {
				R15-=4;
				//S - Flag Set? Signals transfer of current mode SPSR->CPSR
				//todo: flag if current mode is not a priveldegd mode
				if(insn & INSN_BDT_S)
					SET_CPSR(GetRegister(SPSR));
			}
			//LDM PC - takes 1 extra cycle
			at91_ICount -=1;
		}
		else
		{
			/* Decrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				//todo: implement user bank transfer
				LOG(("%08x: User Bank Transfer not implemented!\n",R15));
			}

			result = loadDec( insn&0xffff, rbp, insn&INSN_BDT_S );

			if (insn & INSN_BDT_W)
			{
				if (rb==0xf)
					LOG(("%08x:  Illegal LDRM writeback to r15\n",R15));
				SetRegister(rb,GetRegister(rb)-result*4);
			}
			
			//R15 included? (NOTE: CPSR restore must occur LAST otherwise wrong registers restored!)
			if (insn & 0x8000) {
				R15-=4;
				//S - Flag Set? Signals transfer of current mode SPSR->CPSR
				//todo: flag if current mode is not a priveldegd mode
				if(insn & INSN_BDT_S)
					SET_CPSR(GetRegister(SPSR));
				//LDM PC - takes 1 extra cycle
				at91_ICount -=1;
			}

			//LDM (NO PC) takes nS + 1n + 1I cycles (n = # of register transfers)
			at91_ICount -= (result+1+1);
		}
	} /* Loading */
	else
	{
		/* Storing */
		if (insn & (1<<eR15))
		{
			#if AT91_DEBUG_CORE
				LOG(("%08x: Writing R15 in strm\n",R15));
			#endif
			/* special case handling if writing to PC */
			R15 += 12;
		}
		if (insn & INSN_BDT_U)
		{
			/* Incrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp + (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				//todo: implement user bank transfer
				LOG(("%08x: User Bank Transfer not implemented!\n",R15));
			}

			result = storeInc( insn&0xffff, rbp );

			if( insn & INSN_BDT_W )
			{
				SetRegister(rb,GetRegister(rb)+result*4);
			}
		}
		else
		{
			/* Decrementing */
			if (!(insn & INSN_BDT_P))
			{
				rbp = rbp - (- 4);
			}

			//S Flag Set, but R15 not in list = User Bank Transfer
			if(insn & INSN_BDT_S && ((insn & 0x8000)==0))
			{
				//todo: implement user bank transfer
				LOG(("%08x: User Bank Transfer not implemented!\n",R15));
			}

			result = storeDec( insn&0xffff, rbp );

			if( insn & INSN_BDT_W )
			{
				SetRegister(rb,GetRegister(rb)-result*4);
			}
		}
		if( insn & (1<<eR15) )
			R15 -= 12;

		//STM takes (n+1)S+2N+1I cycles (n = # of register transfers)
		at91_ICount -= ((result+1)+2+1);
	}
} /* HandleMemBlock */



/* Decodes an Op2-style shifted-register form.  If @carry@ is non-zero the
 * shifter carry output will manifest itself as @*carry == 0@ for carry clear
 * and @*carry != 0@ for carry set.

   SJE: Rules: 
   IF RC = 256, Result = no shift.
   LSL   0   = Result = RM, Carry = Old Contents of CPSR C Bit
   LSL(0,31) = Result shifted, least significant bit is in carry out
   LSL  32   = Result of 0, Carry = Bit 0 of RM
   LSL >32   = Result of 0, Carry out 0
   LSR   0   = LSR 32 (see below)
   LSR  32   = Result of 0, Carry = Bit 31 of RM
   LSR >32   = Result of 0, Carry out 0
   ASR >=32  = ENTIRE Result = bit 31 of RM
   ROR  32   = Result = RM, Carry = Bit 31 of RM
   ROR >32   = Same result as ROR n-32 until amount in range of 1-32 then follow rules
*/

static data32_t decodeShift( data32_t insn, data32_t *pCarry)
{
	data32_t k	= (insn & INSN_OP2_SHIFT) >> INSN_OP2_SHIFT_SHIFT;	//Bits 11-7
	data32_t rm	= GetRegister( insn & INSN_OP2_RM );
	data32_t t	= (insn & INSN_OP2_SHIFT_TYPE) >> INSN_OP2_SHIFT_TYPE_SHIFT;

	if ((insn & INSN_OP2_RM)==0xf) {
		/* If hardwired shift, then PC is 8 bytes ahead, else if register shift
		is used, then 12 bytes - TODO?? */
		rm+=8;
	}

	/* All shift types ending in 1 are Rk, not #k */
	if( t & 1 )
	{
//		LOG(("%08x:  RegShift %02x %02x\n",R15, k>>1,GetRegister(k >> 1)));
		#if AT91_DEBUG_CORE
			if((insn&0x80)==0x80)
				LOG(("%08x:  RegShift ERROR (p36)\n",R15));
		#endif

		//see p35 for check on this
		//k = GetRegister(k >> 1)&0x1f;

		//Keep only the bottom 8 bits for a Register Shift
		k = GetRegister(k >> 1)&0xff;

		if( k == 0 ) /* Register shift by 0 is a no-op */
		{
//			LOG(("%08x:  NO-OP Regshift\n",R15));
			if (pCarry) *pCarry = GET_CPSR & C_MASK;
			return rm;
		}
	}
	/* Decode the shift type and perform the shift */
	switch (t >> 1)
	{
	case 0:						/* LSL */
		//LSL  32   = Result of 0, Carry = Bit 0 of RM
		//LSL >32   = Result of 0, Carry out 0
		if(k>=32) 
		{
			if(pCarry)	*pCarry = (k==32)?rm&1:0;
			return 0;
		}
		else
		{
			if (pCarry)
			{
			//LSL      0   = Result = RM, Carry = Old Contents of CPSR C Bit
			//LSL (0,31)   = Result shifted, least significant bit is in carry out
			*pCarry = k ? (rm & (1 << (32 - k))) : (GET_CPSR & C_MASK);
			}
			return k ? LSL(rm, k) : rm;
		}
		break;

	case 1:			       			/* LSR */
		if (k == 0 || k == 32)
		{
			if (pCarry) *pCarry = rm & SIGN_BIT;
			return 0;
		}
		else if (k > 32)
		{
			if (pCarry) *pCarry = 0;
			return 0;
		}
		else
		{
			if (pCarry) *pCarry = (rm & (1 << (k - 1)));
			return LSR(rm, k);
		}
		break;

	case 2:						/* ASR */
		if (k == 0 || k > 32)
			k = 32;

		if (pCarry) *pCarry = (rm & (1 << (k - 1)));
		if (k >= 32)
			return rm & SIGN_BIT ? 0xffffffffu : 0;
		else
		{
			if (rm & SIGN_BIT)
				return LSR(rm, k) | (0xffffffffu << (32 - k));
			else
				return LSR(rm, k);
		}
		break;

	case 3:						/* ROR and RRX */
		if (k)
		{
			while (k > 32) k -= 32;
			if (pCarry) *pCarry = rm & SIGN_BIT;
			return ROR(rm, k);
		}
		else
		{
			if (pCarry) *pCarry = (rm & 1);
			return LSR(rm, 1) | ((GET_CPSR & C_MASK) << 2);
		}
		break;
	}

	LOG(("%08x: Decodeshift error\n",R15));
	return 0;
} /* decodeShift */
