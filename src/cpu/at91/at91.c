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

    See ARM7CORE.C for notes regarding the ARM7TDMI Core

	Todo:
	Most of the AT91 specific functionality is not really implemented or just
	enough to allow functionality we need. Someday it really should be implemented
	fully & properly.
*****************************************************************************/

#include <stdio.h>
#include "at91.h"
#include "../arm7/arm7core.h"
#include "state.h"
#include "mamedbg.h"

/* Prototypes */
INLINE void at91_cpu_write32( int addr, data32_t data );
INLINE void at91_cpu_write16( int addr, data16_t data );
INLINE void at91_cpu_write8( int addr, data8_t data );
INLINE data32_t at91_cpu_read32( int addr );
INLINE data16_t at91_cpu_read16( int addr );
INLINE data8_t at91_cpu_read8( offs_t addr );

/* Macros that can be re-defined for custom cpu implementations */
/* Here we setup to use custom handlers specifically made for the AT91 */
#define READ8(addr)			at91_cpu_read8(addr)
#define WRITE8(addr,data)	at91_cpu_write8(addr,data)
#define READ16(addr)		at91_cpu_read16(addr)
#define WRITE16(addr,data)	at91_cpu_write16(addr,data)
#define READ32(addr)		at91_cpu_read32(addr)
#define WRITE32(addr,data)	at91_cpu_write32(addr,data)
#define PTR_READ32			&at91_cpu_read32
#define PTR_WRITE32			&at91_cpu_write32

/* Macros that need to be defined according to the cpu implementation specific need */
#define ARMREG(reg)			at91.sArmRegister[reg]
#define ARM7				at91
#define ARM7_ICOUNT			at91_ICount

/* Private Data */

/* sArmRegister defines the CPU state */
typedef struct
{
	ARM7CORE_REGS				//these must be included in your cpu specific register implementation
} AT91_REGS;

static AT91_REGS at91;
int at91_ICount;

/*static vars*/
//These all should be eventually moved into the cpu structure, so that multi cpu support can be handled properly
static int remap = 0;							//flag if remap of ram occurred
static data32_t *page0_ram_ptr;					//holder for the pointer set by the driver to ram
static data32_t *reset_ram_ptr;					//""
static READ32_HANDLER((*cs_r_callback));		//holder for the cs_r callback
static WRITE32_HANDLER((*cs_w_callback));		//holder for the cs_w callback
static offs_t cs_r_start, cs_r_end, cs_w_start, cs_w_end;
static WRITE32_HANDLER((*at91_ready_irq_cb));

/* include the arm7 core */
#include "../arm7/arm7core.c"

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

INLINE void at91_cpu_write32( int addr, data32_t data )
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
	#ifdef AT91_DEBUG_CORE
		if(addr&3)
			LOG(("%08x: Unaligned write %08x\n",R15,addr));
	#endif
}


INLINE void at91_cpu_write16( int addr, data16_t data )
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

INLINE void at91_cpu_write8( int addr, data8_t data )
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

INLINE data32_t at91_cpu_read32( int addr )
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
		#ifdef AT91_DEBUG_CORE
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

INLINE data16_t at91_cpu_read16( int addr )
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

INLINE data8_t at91_cpu_read8( offs_t addr )
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


/***************************************************************************/

void at91_reset(void *param)
{
	//must call core...
	arm7_core_reset(param);
}

void at91_exit(void)
{
	/* nothing to do here */
}

int at91_execute( int cycles )
{
	//must call core...
	return arm7_core_execute(cycles);
}


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
	return GET_REGISTER(13);
}

void at91_set_sp(unsigned val)
{
	SET_REGISTER(13,val);
}

unsigned at91_get_reg(int regnum)
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

void at91_set_reg(int regnum, unsigned val)
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

void at91_set_nmi_line(int state)
{
}

void at91_set_irq_line(int irqline, int state)
{
	//must call core...
	arm7_core_set_irq_line(irqline,state);	
}

void at91_set_irq_callback(int (*callback)(int irqline))
{
}

static const data8_t at91_reg_layout[] =
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

static const UINT8 at91_win_layout[] = {
	 0, 0,30,17,	/* register window (top rows) */
	31, 0,49,17,	/* disassembler window (left colums) */
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
	case CPU_INFO_NAME: 		return "AT91";
	case CPU_INFO_FAMILY:		return "Atmel 91 - Acorn Risc Machine";
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
	arm7_disasm( buffer, pc, READ32(pc));
	return 4;
#else
	sprintf(buffer, "$%08x", READ32(pc));
	return 4;
#endif
}

void at91_init(void)
{
	//must call core...
	arm7_core_init("at91");
	return;
}

