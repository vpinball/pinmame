/*****************************************************************************
 *
 *	 tms7000.c
 *	 Portable TMS7000 emulator (Texas Instruments 7000)
 *
 *	 Copyright (c) 2001 tim lindner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   tlindner@ix.netcom.com
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

//SJE: Changed all references to ICount to icount (to match MAME requirements)
//SJE: Changed RM/WM macros to reference newly created tms7000 read/write handlers & removed unused SRM() macro

#include <stdio.h>
#include <stdlib.h>
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "tms7000.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/* Private prototypes */

static UINT16 bcd_add( UINT16 a, UINT16 b );
static UINT16 bcd_tencomp( UINT16 a );
static UINT16 bcd_sub( UINT16 a, UINT16 b);
INLINE READ_HANDLER(tms7000_readmem);
INLINE WRITE_HANDLER(tms7000_writemem);

void tms7000_starttimer1( void );
UINT8 tms7000_calculate_timer1_decrementator( void );
void tms7000_int2_callback( int	param );

/* Public globals */

int tms7000_icount;

static UINT8 tms7000_reg_layout[] = {
	TMS7000_PC, TMS7000_SP, TMS7000_ST, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 tms7000_win_layout[] = {
	27, 0,53, 1,	/* register window (top, right rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 2,53,10,	/* memory #1 window (right, upper middle) */
	27,13,53, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

void tms7000_check_IRQ_lines( void );

//SJE
//#define RM(Addr) ((unsigned)cpu_readmem16(Addr))
//#define WM(Addr,Value) (cpu_writemem16(Addr,Value))
//#define SRM(Addr) ((signed)cpu_readmem16(Addr))			//SJE: NOT USED?

#define RM(Addr) ((unsigned)tms7000_readmem(Addr))
#define WM(Addr,Value) (tms7000_writemem(Addr,Value))

UINT16 RM16( UINT32 mAddr );	/* Read memory (16-bit) */
UINT16 RM16( UINT32 mAddr )
{
	UINT32 result = RM(mAddr) << 8;
	return result | RM((mAddr+1)&0xffff);
}

void WM16( UINT32 mAddr, PAIR p );	/*Write memory file (16 bit) */
void WM16( UINT32 mAddr, PAIR p )
{
	WM( mAddr, p.b.h );
	WM( (mAddr+1)&0xffff, p.b.l );
}

UINT16 RRF16( UINT32 mAddr ); /*Read register file (16 bit) */
UINT16 RRF16( UINT32 mAddr )
{
	PAIR result;
	result.b.h = RM((mAddr-1)&0xffff);
	result.b.l = RM(mAddr);
//	return (result.b.h << 8) & result.b.l;
	return result.w.l;
}

void WRF16( UINT32 mAddr, PAIR p ); /*Write register file (16 bit) */
void WRF16( UINT32 mAddr, PAIR p )
{
	WM( (mAddr-1)&0xffff, p.b.h );
	WM( mAddr, p.b.l );
}

//SJE: Not used
//#define RPF(x)		tms7000_pf_r(x)
//#define WPF(x,y)	tms7000_pf_w(x,y)

#define IMMBYTE(b)	b = ((unsigned)cpu_readop_arg(pPC)); pPC++
#define SIMMBYTE(b)	b = ((signed)cpu_readop_arg(pPC)); pPC++
#define IMMWORD(w)	w.b.h = (unsigned)cpu_readop_arg(pPC++); w.b.l = (unsigned)cpu_readop_arg(pPC++)

#define PUSHBYTE(b) pSP++; WM(pSP,b)
#define PUSHWORD(w) pSP++; WM(pSP,w.b.h); pSP++; WM(pSP,w.b.l)
#define PULLBYTE(b) b = RM(pSP); pSP--
#define PULLWORD(w) w.b.l = RM(pSP); pSP--; w.b.h = RM(pSP); pSP--

typedef struct
{
	PAIR		pc; 			/* Program counter */
	UINT8		sp;				/* Stack Pointer */
	UINT8		sr;				/* Status Register */
	UINT8		irq_state[3];	/* State of the three IRQs */
	UINT8		rf[0x0ff];		/* Register file */				/*SJE*/
	UINT8		pf[0x100];		/* Perpherial file */
	int 		(*irq_callback)(int irqline);
	UINT8		idle_state;		/* Set after the execution of an idle instruction */
	void		*timer1;		/* Timer 1 (triggers int 2) */
	double		time_timer1;	/* Absloute time when timer 1 started */
	UINT8		timer1_decrementator,
				timer1_prescaler,
				timer1_capturelatch;
} tms7000_Regs;

static tms7000_Regs tms7000;

#define pPC		tms7000.pc.w.l
#define PC		tms7000.pc
#define pSP		tms7000.sp
#define pSR		tms7000.sr

#define RDA		RM(0x0000)
#define RDB		RM(0x0001)

//SJE: 
//#define WRA(Value) (cpu_writemem16(0x0000,Value))
//#define WRB(Value) (cpu_writemem16(0x0001,Value))
#define WRA(Value) (tms7000_writemem(0x0000,Value))
#define WRB(Value) (tms7000_writemem(0x0001,Value))

#define SR_C	0x80		/* Carry */
#define SR_N	0x40		/* Negative */
#define SR_Z	0x20		/* Zero */
#define SR_I	0x10		/* Interrupt */

#define CLR_NZC 	pSR&=~(SR_N|SR_Z|SR_C)
#define CLR_NZCI 	pSR&=~(SR_N|SR_Z|SR_C|SR_I)
#define SET_C8(a)	pSR|=((a&0x0100)>>1)
#define SET_N8(a)	pSR|=((a&0x0080)>>1)
#define SET_Z(a)	if(!a)pSR|=SR_Z
#define SET_Z8(a)	SET_Z((UINT8)a)
#define SET_Z16(a)	SET_Z((UINT8)a>>8)
#define GET_C		(pSR >> 7)

/* Not working */
#define SET_C16(a)	pSR|=((a&0x010000)>>9)

#define SETC		pSR |= SR_C
#define SETZ		pSR |= SR_Z
#define SETN		pSR |= SR_N

#define CHANGE_PC change_pc16(pPC)


/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/

unsigned tms7000_get_context(void *dst)
{
	if( dst )
		*(tms7000_Regs*)dst = tms7000;
	return sizeof(tms7000_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void tms7000_set_context(void *src)
{
	if( src )
		tms7000 = *(tms7000_Regs*)src;
}

unsigned tms7000_get_reg(int regnum)
{
	switch( regnum )
	{
		case REG_PC:
		case TMS7000_PC: return pPC;
		case REG_SP:
		case TMS7000_SP: return pSP;
		case TMS7000_ST: return pSR;
		case REG_PREVIOUSPC: return 0;
	}
	return 0;
}

void tms7000_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case REG_PC:
		case TMS7000_PC: pPC = val; break;
		case REG_SP:
		case TMS7000_SP: pSP = val; break;
		case TMS7000_ST: pSR = val; break;
	}
}

void tms7000_init(void)
{
	int cpu = cpu_getactivecpu();

	memset(tms7000.pf, 0, 0x100);
	timer_set(TIME_NEVER, 0, tms7000_int2_callback);
	tms7000.timer1_capturelatch = 0;

	state_save_register_UINT16("tms7000", cpu, "PC", &pPC, 1);
	state_save_register_UINT8("tms7000", cpu, "SP", &pSP, 1);
	state_save_register_UINT8("tms7000", cpu, "SR", &pSR, 1);
}

void tms7000_reset(void *param)
{
//	tms7000.architecture = (int)param;
	
	tms7000.idle_state = 0;
	tms7000.irq_state[ TMS7000_IRQ1_LINE ] = CLEAR_LINE;
	tms7000.irq_state[ TMS7000_IRQ2_LINE ] = CLEAR_LINE;
	tms7000.irq_state[ TMS7000_IRQ3_LINE ] = CLEAR_LINE;
	
	WM( 0x100 + 9, 0 );		/* Data direction regs are cleared */
	WM( 0x100 + 11, 0 );
	
//	if( tms7000.architecture == TMS7000_NMOS )
//	{
		WM( 0x100 + 4, 0xff );		/* Output 0xff on port A */
		WM( 0x100 + 8, 0xff );		/* Output 0xff on port C */
		WM( 0x100 + 10, 0xff );		/* Output 0xff on port D */
//	}
//	else
//	{
//		WM( 0x100 + 4, 0xff );		/* Output 0xff on port A */
//	}
		
	pSP = 0x01;				/* Set stack pointer to r1 */
	pSR = 0x00;				/* Clear status register (disabling interrupts */
	WM( 0x100 + 0, 0 );		/* Write a zero to IOCNT0 */
	
	/* On TMS70x2 and TMS70Cx2 IOCNT1 is zero */
	
	WRA( tms7000.pc.b.h );	/* Write previous PC to A:B */
	WRB( tms7000.pc.b.l );
	pPC = RM16(0xfffe);		/* Load reset vector */
	CHANGE_PC;
}

void tms7000_exit(void)
{
//	timer_remove( tms7000.timer1);
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *tms7000_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	tms7000_Regs *r = context;

	which = (which+1) % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &tms7000;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "TMS7000";
		case CPU_INFO_FAMILY: return "Texas Instruments TMS7000";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) tim lindner 2001";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms7000_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms7000_win_layout;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->sr & 0x80 ? 'C':'c',
				r->sr & 0x40 ? 'N':'n',
				r->sr & 0x20 ? 'Z':'z',
				r->sr & 0x10 ? 'I':'i',
				r->sr & 0x08 ? '?':'.',
				r->sr & 0x04 ? '?':'.',
				r->sr & 0x02 ? '?':'.',
				r->sr & 0x01 ? '?':'.' );
			break;
		case CPU_INFO_REG+TMS7000_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+TMS7000_SP: sprintf(buffer[which], "SP:%02X", r->sp); break;
		case CPU_INFO_REG+TMS7000_ST: sprintf(buffer[which], "ST:%02X", r->sr); break;
	}
	return buffer[which];
}

unsigned tms7000_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return Dasm7000(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

void tms7000_set_irq_line(int irqline, int state)
{
	tms7000.irq_state[ irqline ] = state;

	LOG(("TMS7000#%d set_irq_line %d, %d\n", cpu_getactivecpu(), irqline, state));

	if (state == CLEAR_LINE)
	{
		tms7000.pf[0] &= ~(0x02 << (irqline * 2));	/* Clear INTx flag in iocntl0 */
		return;
	}

	tms7000.pf[0] |= (0x02 << (irqline * 2));	/* Set INTx iocntl0 flag */
	
	if( irqline == TMS7000_IRQ3_LINE )
	{
		/* Set capture latch */
		if( tms7000.pf[ 0x03 ] & 0x40 ) /* Determine timer source */
		{
			/* Source A7/EC1 */
			tms7000.timer1_capturelatch = tms7000.timer1_decrementator;
		}
		else
		{
			/* Source internal timer */	
			tms7000.timer1_capturelatch = tms7000_calculate_timer1_decrementator();
		}
	}
	
	tms7000_check_IRQ_lines();
}

void tms7000_set_irq_callback(int (*callback)(int irqline))
{
	tms7000.irq_callback = callback;
}

void tms7000_check_IRQ_lines( void )
{
	UINT16	newPC;
	int		tms7000_state;
	
	if( pSR & SR_I ) /* Check Global Interrupt bit: Status register, bit 4 */
	{
		if( tms7000.irq_state[ TMS7000_IRQ1_LINE ] == ASSERT_LINE )
		{
			if( tms7000.pf[0] & 0x01 ) /* INT1 Enable bit */
			{
				newPC = RM16(0xfffc);
				tms7000_state = TMS7000_IRQ1_LINE;
				goto tms7000_interrupt;
			}
		}

		if( tms7000.irq_state[ TMS7000_IRQ2_LINE ] == ASSERT_LINE )
		{
			if( tms7000.pf[0] & 0x04 ) /* INT2 Enable bit */
			{
				newPC = RM16(0xfffa);
				tms7000_state = TMS7000_IRQ2_LINE;
				goto tms7000_interrupt;
			}
		}

		if( tms7000.irq_state[ TMS7000_IRQ3_LINE ] == ASSERT_LINE )
		{
			if( tms7000.pf[0] & 0x10 ) /* INT3 Enable bit */
			{
				newPC = RM16(0xfff8);
				tms7000_state = TMS7000_IRQ3_LINE;
				goto tms7000_interrupt;
			}
		}

		return;

tms7000_interrupt:
	
		PUSHBYTE( pSR );	/* Push Status register */
		PUSHWORD( PC );		/* Push Program Counter */
		pSR = 0;			/* Clear Status register */
		pPC = newPC;		/* Load PC with interrupt vector */
		CHANGE_PC;
		
		if( tms7000.idle_state != 0 )
			tms7000_icount -= 19;		/* 19 cycles used */
		else
		{
			tms7000_icount -= 17;		/* 17 if idled */
			tms7000.idle_state = 0;
		}
		
		(void)(*tms7000.irq_callback)(tms7000_state);
	}
}

#include "tms70op.c"
#include "tms70tb.c"

int tms7000_execute(int cycles)
{
	int op;
	
	tms7000_icount = cycles;

	do
	{
		CALL_MAME_DEBUG;
		op = cpu_readop(pPC);
		pPC++;

		opfn[op]();
	} while( tms7000_icount > 0 );
	
	return cycles - tms7000_icount;

}

/****************************************************************************
 * Starts/restarts the timer1
 ****************************************************************************/
void tms7000_starttimer1( void )
{
	if( tms7000.pf[ 0x03 ] & 0x40 ) /* Determine timer source */
	{
		/* Source: A7/EC1 */
		tms7000.timer1_prescaler = tms7000.pf[0x03] & 0x1f;
		tms7000.timer1_decrementator = tms7000.pf[0x02];
	}
	else
	{
		/* Source: internal clock */
		timer_reset(tms7000.timer1, TIME_IN_CYCLES( 16 * ((tms7000.pf[ 0x03 ] & 0x1f)+1) * ((tms7000.pf[ 0x02 ])+1),
															cpu_getactivecpu() ) );
		tms7000.time_timer1 = timer_get_time();
	}
}

/****************************************************************************
 * Trigger the event counter
 ****************************************************************************/
void tms7000_A6EC1( void )
{
	if( tms7000.pf[0x03] & 0x80 )	/* Only valid is timer enabled */
	{
		if( (--(tms7000.timer1_prescaler) ) == 0xff )
		{
			tms7000.timer1_prescaler = tms7000.pf[0x03] & 0x1f;
			if( (--(tms7000.timer1_decrementator)) == 0xff )
			{
				tms7000.timer1_decrementator = tms7000.pf[0x02];
				tms7000_set_irq_line( TMS7000_IRQ2_LINE, ASSERT_LINE);
			}
		}
	}
}

/****************************************************************************
 * Call when INT2 timer fires
 ****************************************************************************/

void tms7000_int2_callback( int	param )
{
	tms7000_set_irq_line( TMS7000_IRQ2_LINE, ASSERT_LINE);
	tms7000_starttimer1();
}

/****************************************************************************
 * Return the value of the decrementator
 ****************************************************************************/

UINT8 tms7000_calculate_timer1_decrementator( void )
{
	UINT8	result;
	
	/* Since we are not really decrementating the register every cycle,
	   I use this function to calculate the value if/when it is needed.
	   
	   This will return the incorrect result if the the absolute time
	   overflows.
	*/
	
	double prescalertimer = TIME_IN_CYCLES(16,cpu_getactivecpu()) *
													(tms7000.pf[0x03] & 0x1f);
	
	result = (tms7000.time_timer1 - timer_get_time()) / prescalertimer;

	return result;
}


WRITE_HANDLER( tms70x0_pf_w )	/* Perpherial file write */
{
	data8_t	temp1, temp2, temp3, temp4;
	
	switch( offset )
	{
		case 0x00:	/* IOCNT0, Input/Ouput control */
			temp1 = data & 0x2a;				/* Record which bits to clear */
			//SJE - Must be a mistake..
			//temp2 = tms7000.pf[0x03] & 0x2a;	/* Get copy of current bits */
			temp2 = tms7000.pf[0x00] & 0x2a;	/* Get copy of current bits */		
			temp3 = (~temp1) & temp2;			/* Clear the requested bits */
			temp4 = temp3 | (data & (~0x2a) );	/* OR in the remaining data */
			
			tms7000.pf[0x00] = temp4;
			break;

		case 0x03:	/* T1CTL, timer 1 control */
			/* stuff data in register */
			tms7000.pf[0x03] = data;
			
			/* Stop current counter */
			timer_reset( tms7000.timer1, TIME_NEVER );
			
			if ((data & 0x80) == 0x80)
				tms7000_starttimer1();

			break;
		
		case 0x04: /* Port A write */
			/* Port A is read only so this is a NOP */
			break;
			
		case 0x06: /* Port B write */
			cpu_writeport16( TMS7000_PORTB, data );
			tms7000.pf[ 0x06 ] = data;
			break;
		
		case 0x08: /* Port C write */
			temp1 = data & tms7000.pf[ 0x09 ];	/* Mask off input bits */
			cpu_writeport16( TMS7000_PORTC, temp1 );
			tms7000.pf[ 0x08 ] = temp1;
			break;
			
		case 0x0a: /* Port D write */
			temp1 = data & tms7000.pf[ 0x0b ];	/* Mask off input bits */
			cpu_writeport16( TMS7000_PORTD, temp1 );
			tms7000.pf[ 0x0a ] = temp1;
			break;
			
		default:
			/* Just stuff the other registers */
			tms7000.pf[ offset ] = data;
			break;
	}
}

READ_HANDLER( tms70x0_pf_r )	/* Perpherial file read */
{
	data8_t result;
	data8_t	temp1, temp2, temp3, temp4;
	
	switch( offset )
	{
		//SJE: Why was this commented out?
		case 0x00:	/* IOCNT0, Input/Ouput control */
			result = tms7000.pf[0x00];
			break;
		
		case 0x02:	/* T1DATA, timer 1 data */
			if( tms7000.pf[ 0x03 ] & 0x40 )
			{
				/* Source A7/EC1 */
				result = tms7000.timer1_decrementator;
			}
			else
			{
				/* Source: internal timer */
				result = tms7000_calculate_timer1_decrementator();
			}
			break;

		case 0x03:	/* T1CTL, timer 1 control */
			result = tms7000.timer1_capturelatch;
			break;

		case 0x04: /* Port A read */
			result = cpu_readport16( TMS7000_PORTA );
			break;
			
//SJE: Why was this commented out?
		case 0x06: /* Port B read */
			/* Port B is write only, return a previous written value */
			result = tms7000.pf[ 0x06 ];
			break;
		
		case 0x08: /* Port C read */
			temp1 = tms7000.pf[ 0x08 ] & tms7000.pf[ 0x09 ];	/* Get previous output bits */
			temp2 = cpu_readport16( TMS7000_PORTC );			/* Read port */
			temp3 = temp2 & (~tms7000.pf[ 0x09 ]);				/* Mask off output bits */
			temp4 = temp1 | temp3;								/* OR together */
			
			result = temp4;
			break;
			
		case 0x0a: /* Port D read */
			temp1 = tms7000.pf[ 0x0a ] & tms7000.pf[ 0x0b ];	/* Get previous output bits */
			temp2 = cpu_readport16( TMS7000_PORTD );			/* Read port */
			temp3 = temp2 & (~tms7000.pf[ 0x0b ]);				/* Mask off output bits */
			temp4 = temp1 | temp3;								/* OR together */
			
			result = temp4;
			break;
			
		default:
			/* Just unstuff the other registers */
			result = tms7000.pf[ offset ];
			break;
	}

	return result;
}

// BCD arthrimetic handling
static UINT16 bcd_add( UINT16 a, UINT16 b )
{
	UINT16	t1,t2,t3,t4,t5,t6;
	
	/* Sure it is a lot of code, but it works! */
	t1 = a + 0x0666;
	t2 = t1 + b;
	t3 = t1 ^ b;
	t4 = t2 ^ t3;
	t5 = ~t4 & 0x1110;
	t6 = (t5 >> 2) | (t5 >> 3);
	return t2-t6;
}

static UINT16 bcd_tencomp( UINT16 a )
{
	UINT16	t1,t2,t3,t4,t5,t6;
	
	t1 = 0xffff - a;
	t2 = -a;
	t3 = t1 ^ 0x0001;
	t4 = t2 ^ t3;
	t5 = ~t4 & 0x1110;
	t6 = (t5 >> 2)|(t5>>3);
	return t2-t6;
}

static UINT16 bcd_sub( UINT16 a, UINT16 b)
{
	return bcd_tencomp(b) - bcd_tencomp(a);
}


//SJE: Return state of the internal registers
WRITE_HANDLER( tms7000_internal_w ) {
	tms7000.rf[ offset ]=data;
}
READ_HANDLER( tms7000_internal_r ) {
	return tms7000.rf[ offset ]; 
}

//SJE: Note setup only works when in Microprocessor Mode (MC Pin = VCC)
//Todo: Implement regular read/write if not in MC mode..
INLINE READ_HANDLER(tms7000_readmem)
{
	//Internal RAM (0-0xff)
	if(offset < 0x100)
		return tms7000_internal_r(offset);
	else
	//PF Registers (0x100-0x10c)
		if(offset < 0x10c)
			return tms70x0_pf_r(offset&0xff);
		else
	//Everything else goes to external memory
			return cpu_readmem16(offset);
}
INLINE WRITE_HANDLER(tms7000_writemem)
{
	//Internal RAM (0-0xff)
	if(offset < 0x100)
		tms7000_internal_w(offset,data);
	else
	//PF Registers (0x100-0x10c)
		if(offset < 0x10c)
			tms70x0_pf_w(offset&0xff,data);
		else
	//Everything else goes to external memory
			cpu_writemem16(offset,data);
}

