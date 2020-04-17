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
	-Fuller AIC handling, including interrupt priorites and ensuring interrupts don't overlap
	-Serial Port Interfaces & proper timing
	-All Timer Modes - Only RC Compare for Capture Mode is supported at this time.
	-Watch Dog Circuit Handling
	-Special Function Handling
	-Find a way to avoid all the nested if/else for memory mapping
*****************************************************************************/

#include <stdio.h>
#include "at91.h"
#include "../arm7/arm7core.h"
#include "state.h"
#include "mamedbg.h"
#include <assert.h>

//needed to capture this cpu's operating frequency
extern int activecpu;

//Flags
#define	USE_MAME_TIMERS	1			//Set to 1 for performance but possibly less accurate timing, to 0 for more accuracy but huge performance penalty

//Flags for Logging
#define LOG_AT91 0					//Turn on/off logging of all AT91 debugging info
#define LOG_TOSCREEN 0				//Send Log data to screen, otherwise will go to log file

//Flags for specific Logging
#define LOG_TIMERS 0				//Turn on/off logging of Timer Info
#define LOG_TIMER_STAT_READ 0		//Turn on/off logging of Timer Status Read
#define LOG_TIMER_START 0			//Turn on/off logging of Timer Trigger Start
#define LOG_TIMER_ENABLE 0			//Turn on/off logging of Timer Enable Event
#define LOG_TIMER_DISABLE 0			//Turn on/off logging of Timer Disable Event
#define LOG_PIO 0					//Turn on/off logging of PIO Info
#define LOG_PIO_READ 0				//Turn on/off logging of PIO Read
#define LOG_PIO_WRITE 0				//Turn on/off logging of PIO Write
#define LOG_POWER 0					//Turn on/off logging of Power Saver Info
#define LOG_AIC 0					//Turn on/off logging of AIC Info
#define LOG_AIC_IVW 0				//Turn on/off logging of AIC - Interrupt Vector Write Command
#define LOG_AIC_EOI 0				//Turn on/off logging of AIC - End of Interrupt Command
#define LOG_AIC_VECTOR_READ 0		//Turn on/off logging of AIC - Vector Address Read
#define LOG_USART1_DATA_OUT 0

//Flags for Testing
#define INT_BLOCK_FIQ   0			//Turn on/off blocking of specified Interrupt (Leave OFF for non-test situation)
#define INT_BLOCK_IRQ0	0
#define INT_BLOCK_TC0   0
#define INT_BLOCK_TC1   0
#define INT_BLOCK_TC2   0

/* Prototypes */
INLINE void at91_cpu_write32( int addr, data32_t data );
INLINE void at91_cpu_write16( int addr, data16_t data );
INLINE void at91_cpu_write8( int addr, data8_t data );
INLINE data32_t at91_cpu_read32( int addr );
INLINE data16_t at91_cpu_read16( int addr );
INLINE data8_t at91_cpu_read8( int addr );
void at91_fire_irq(int irqline);
void at91_irq_assert_if_greater();
void (*at91_transmit_serial)(int usartno, data8_t *data, int size) = NULL;
void (*at91_serial_receive_ready)(int usartno) = NULL;

void at91_set_transmit_serial(void (*fp)(int usartno, data8_t *data, int size))
{
	at91_transmit_serial = fp;
}

void at91_set_serial_receive_ready(void (*fp)(int usartno))
{
	at91_serial_receive_ready = fp;
}

#if !USE_MAME_TIMERS
INLINE BeforeOpCodeHook(void);
INLINE AfterOpCodeHook(void);
#else
static void timer_trigger_event(int timer_num);
#endif

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
#define PTR_READ16          &arm7_cpu_read16
#define PTR_WRITE16         &arm7_cpu_write16
#define PTR_READ8           &arm7_cpu_read8
#define PTR_WRITE8          &arm7_cpu_write8

/* Macros that need to be defined according to the cpu implementation specific need */
#define ARMREG(reg)			at91.sArmRegister[reg]
#define ARM7				at91
#define ARM7_ICOUNT			at91_ICount
#define RESET_ICOUNT		ARM7_ICOUNT = cycles; at91.tot_prev_cycles = 0;
#define CAPTURE_NUM_CYCLES	at91.prev_cycles = cycles - ARM7_ICOUNT - at91.tot_prev_cycles;\
							at91.tot_prev_cycles += at91.prev_cycles;

#if !USE_MAME_TIMERS
	#define BEFORE_OPCODE_EXEC_HOOK	BeforeOpCodeHook();
	#define AFTER_OPCODE_EXEC_HOOK
#else
	#define BEFORE_OPCODE_EXEC_HOOK
	#define AFTER_OPCODE_EXEC_HOOK
#endif

//Define # of integrated timers
#define MAX_TIMER 3

//Helper macros
#define CLEAR_BITS(x,y)		((x) &= ((y)^0xffffffff));
#define SET_BITS(x,y)		((x) |= (y));

//Timer macros
#define PS_TIMER_CLOCK_ENABLED(x)	(at91.power_status & (0x10<<(x)))				//Power Enabled for Clock: TC0 = bit 4, TC1 = bit 5, TC2 = bit 6
#define TC_ENABLED(x)				(at91.tc_clock[(x)].tc_status & 0x10000)		//bit 16 of Status = 1 for Enabled
#define TC_RUNNING(x)				(at91.tc_clock[(x)].started && PS_TIMER_CLOCK_ENABLED((x)))
#define TC_RC_IRQ_ENABLED(x)		(at91.tc_clock[(x)].tc_irq_mask & 0x10)
#define TC_OVRFL_IRQ_ENABLED(x)		(at91.tc_clock[(x)].tc_irq_mask & 1)
#define TC_WAVEMODE(x)				(at91.tc_clock[(x)].tc_chan_mode & 0x8000)		//bit 15 of Channel Mode = 1 for Wave Mode
#define TC_RC_TRIGGER(x)			(at91.tc_clock[(x)].tc_chan_mode & 0x4000)		//bit 14 of Channel Mode = 1 for RC Compare Trigger
#define AT91_USART_IRQ(x)			(2+(1-(x)))
/* Private Data */

typedef struct
{
	int started;					//Flag if the timer has been started
	int halfticks;					//holds # of half ticks from last check
	data32_t tc_chan_mode;			//holds the channel mode data
	data32_t tc_counter;			//holds 16 bit counter (even though we declare it 32bit)
	data32_t tc_rega;				//holds Register A data
	data32_t tc_regb;				//holds Register B data
	data32_t tc_regc;				//holds Register C data
	data32_t tc_status;				//holds status of timer (overflow bits, etc)
	data32_t tc_irq_mask;			//holds irq mask of timer (0 = irq disabled for the pin, 1 = irq enabled for the pin)
	int tc_cycle_div;				//cycle divider to use for ticking the clock.
} AT91_CLOCK;

/* CPU Registers that change on a reset */
typedef struct
{
	ARM7CORE_REGS								//these must be included in your cpu specific register implementation
	int prev_cycles;							//# of previous cycles used since last opcode was performed.
	int tot_prev_cycles;						//# of total previous cycles
	int remap;	//flag if remap of ram occurred
	data32_t aic_vectors[32];						//holds vector address for each interrupt 0-31
	data32_t aic_smr[32];						//Holds IRQ priorities
	data32_t aic_irqmask;						//holds the irq enabled mask for each interrupt 0-31 (0 = disabled, 1 = enabled) - 1 bit per interrupt.
	data32_t aic_irqstatus;						//holds the status of the current interrupt # - 0-31 are valid #
	data32_t aic_irqpending;					//holds the status of any pending interupts
	data32_t aic_spurious;						//Spurious interrupt vector
	data32_t pio_enabled_status;				//holds status of each pio pin, 1 bit per pin. (0 = peripheral control, 1 = PIO control)
	data32_t pio_output_status;					//holds output status of each pio pin, 1 bit per pin. (0 = Input Pin, 1 = Output Pin)
	data32_t pio_output_data_status;			//holds output data status of each pio pin, 1 bit per pin. (0 = Pin Value is programmed as 0, 1 = Pin is programmed 1) - Only if pin is under PIO control and set as an output pin.
	data32_t pio_data_status;					//holds data status of each pio pin, 1 bit per pin. (0 = pin is low, 1 = pin is hi)
	data32_t pio_irq_mask;						//holds irq mask of each pio pin, 1 bit per pin. (0 = irq disabled for the pin, 1 = irq enabled for the pin)
	data32_t pio_irq_status;					//holds irq status of each pio pin, 1 bit per pin. (0 = no change since last read, 1 = change in value of pin since last read)
	data32_t power_status;						//holds enabled status of each peripheral clock, 1 bit per pin. (0 = irq disabled for the pin, 1 = irq enabled for the pin)
	AT91_CLOCK tc_clock[MAX_TIMER];				//holds the clock structure for each clock that is integrated in the cpu
	data32_t tc_block_mode;						//holds block mode data for all timer channels
} AT91_REGS;

//CPU Reset Protected Settings
typedef struct
{
	int cpu_freq;								//CPU Clock Frequency (as specified by the Machine Driver)
	int cpunum;									//CPU num
	data32_t *page0_ram_ptr;					//holder for the pointer set by the driver to ram @ page 0.
	data32_t *reset_ram_ptr;					//holder for the pointer set by the driver to ram swap location.
	data32_t page0[0x100000];					//Hold copy of original boot rom data @ page 0.
	#if USE_MAME_TIMERS
	mame_timer* timer[MAX_TIMER];				//handle to mame timer for each clock
	#endif
} AT91_REGS_RS;

static AT91_REGS at91;
static AT91_REGS_RS at91rs;
int at91_ICount;
int at91_priority_map[31];
int at91_irqstack[31];
int at91_irqstackpos = 0;

#define AT91_RECEIVE_BUFFER_SIZE 512

#define US_DMSI         (1<<10)
#define US_TXEMPTY      (1<<9)
#define US_TIMEOUT      (1<<8)
#define US_PARE         (1<<7)
#define US_FRAME        (1<<6)
#define US_OVRE         (1<<5)
#define US_ENDTX        (1<<4)
#define US_ENDRX        (1<<3)
#define US_RXBRK        (1<<2)
#define US_TXRDY        (1<<1)
#define US_RXRDY        (1)


typedef struct 
{ 	
	data32_t US_IER; 
	data32_t US_CSR;
	data32_t US_TPR;
	data32_t US_TCR;
	data32_t US_RPR;
	data32_t US_THR;
	data32_t US_RCR;
	int at91_rbuf_head;
	int at91_rbuf_tail;

	data8_t at91_receivebuf[AT91_RECEIVE_BUFFER_SIZE];

} AT91_REGS_USART;

static AT91_REGS_USART at91usart[2] = {{ 0, US_TXRDY | US_TXEMPTY | US_ENDTX,0,0,0,0,0,0 }, { 0, US_TXRDY | US_TXEMPTY | US_ENDTX,0,0,0,0,0,0 }};

/* include the arm7 core */
#include "../arm7/arm7core.c"

#undef LOG

#if LOG_AT91
#if LOG_TOSCREEN
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif
#else
#define LOG(x)
#endif

/* external interfaces */
void at91_set_ram_pointers(data32_t *reset_ram_ptr, data32_t *page0_ram_ptr)
{
	at91rs.page0_ram_ptr = page0_ram_ptr;
	at91rs.reset_ram_ptr = reset_ram_ptr;
}

void at91_init_jit(int min_addr, int max_addr)
{
	jit_create_map(at91.jit, min_addr, max_addr);
}

//used for debugging
static char temp[256];

//Help debug the Timer register data
static const char *timer_offset[] = {
	"CHANNEL CONTROL","CHANNEL MODE","RESERVED","RESERVED",
	"COUNTER VALUE","REGISTER A","REGISTER B","REGISTER C",
	"STATUS REGISTER","INTERRUPT ENABLE","INTERRUPT DISABLE","INTERRUPT MASK"
};
char *GetTimerOffset(int addr)
{
	int offset = addr & 0x3c;
	if(offset <= 0x2c)
		sprintf(temp,"%s",timer_offset[offset/4]);
	else
		sprintf(temp,"** Unknown **");
	return temp;
}

//Help debug the AIC register data
static const char *aic_offset[] = {
	"SOURCE MODE 0(FIQ)","SOURCE MODE 1(SWIRQ)","SOURCE MODE 2(UART0)","SOURCE MODE 3(UART1)","SOURCE MODE 4(TC0)",
	"SOURCE MODE 5(TC1)","SOURCE MODE 6(TC2)","SOURCE MODE 7(WDOG)","SOURCE MODE 8(PIO)","SOURCE MODE 9(RSVD)",
	"SOURCE MODE 10(RSVD)","SOURCE MODE 11(RSVD)","SOURCE MODE 12(RSVD)","SOURCE MODE 13(RSVD)","SOURCE MODE 14(RSVD)",
	"SOURCE MODE 15(RSVD)","SOURCE MODE 16(IRQ0)","SOURCE MODE 17(IRQ1)","SOURCE MODE 18(IRQ2)","SOURCE MODE 19(RSVD)",
	"SOURCE MODE 20(RSVD)","SOURCE MODE 21(RSVD)","SOURCE MODE 22(RSVD)","SOURCE MODE 23(RSVD)","SOURCE MODE 24(RSVD)",
	"SOURCE MODE 25(RSVD)","SOURCE MODE 26(RSVD)","SOURCE MODE 27(RSVD)","SOURCE MODE 28(RSVD)","SOURCE MODE 29(RSVD)",
	"SOURCE MODE 30(RSVD)","SOURCE MODE 31(RSVD)",
	"SOURCE VECTOR 0(FIQ)","SOURCE VECTOR 1(SWIRQ)","SOURCE VECTOR 2(UART0)","SOURCE VECTOR 3(UART1)","SOURCE VECTOR 4(TC0)",
	"SOURCE VECTOR 5(TC1)","SOURCE VECTOR 6(TC2)","SOURCE VECTOR 7(WDOG)","SOURCE VECTOR 8(PIO)","SOURCE VECTOR 9(RSVD)",
	"SOURCE VECTOR 10(RSVD)","SOURCE VECTOR 11(RSVD)","SOURCE VECTOR 12(RSVD)","SOURCE VECTOR 13(RSVD)","SOURCE VECTOR 14(RSVD)",
	"SOURCE VECTOR 15(RSVD)","SOURCE VECTOR 16(IRQ0)","SOURCE VECTOR 17(IRQ1)","SOURCE VECTOR 18(IRQ2)","SOURCE VECTOR 19(RSVD)",
	"SOURCE VECTOR 20(RSVD)","SOURCE VECTOR 21(RSVD)","SOURCE VECTOR 22(RSVD)","SOURCE VECTOR 23(RSVD)","SOURCE VECTOR 24(RSVD)",
	"SOURCE VECTOR 25(RSVD)","SOURCE VECTOR 26(RSVD)","SOURCE VECTOR 27(RSVD)","SOURCE VECTOR 28(RSVD)","SOURCE VECTOR 29(RSVD)",
	"SOURCE VECTOR 30(RSVD)","SOURCE VECTOR 31(RSVD)",
	"IRQ VECTOR","FIQ VECTOR","INTERRUPT STATUS","INTERRUPT PENDING","INTERRUPT MASK","CORE INTERRUPT STATUS",
	"RESERVED","RESERVED","INTERRUPT ENABLE CMD","INTERRUPT DISABLE CMD","INTERRUPT CLEAR CMD","INTERRUPT SET CMD",
	"END OF INTERRUPT CMD","SPURIOUS VECTOR"
};
char *GetAICOffset(int addr)
{
	int offset = addr & 0x1ff;
	if(offset <= 0x134)
		sprintf(temp,"%s",aic_offset[offset/4]);
	else
		sprintf(temp,"** Unknown **");
	return temp;
}

//Help debug the PIO register data
static const char *pio_offset[] = {
	"PIO ENABLE","PIO DISABLE","PIO STATUS","RESERVED",
	"OUTPUT ENABLE","OUTPUT DISABLE","OUTPUT STATUS","RESERVED",
	"FILTER ENABLE","FILTER DISABLE","FILTER STATUS","RESERVED",
	"SET OUTPUT DATA","CLEAR OUTPUT DATA","OUTPUT DATA STATUS","PIN DATA STATUS",
	"INTERRUPT ENABLE","INTERRUPT DISABLE","INTERRUPT MASK","INTERRUPT STATUS"
};
char *GetPIOOffset(int addr)
{
	int offset = addr & 0x1ff;
	if(offset <= 0x4c)
		sprintf(temp,"%s",pio_offset[offset/4]);
	else
		sprintf(temp,"** Unknown **");
	return temp;
}

//Help debug the PS register data
static const char *ps_offset[] = {
	"CONTROL REGISTER","PERIPHERAL CLOCK ENABLE","PERIPHERAL CLOCK DISABLE","PERIPHERAL CLOCK STATUS REGISTER"
};
char *GetPSOffset(int addr)
{
	int offset = addr & 0xf;
	if(offset <= 0xc)
		sprintf(temp,"%s",ps_offset[offset/4]);
	else
		sprintf(temp,"** Unknown **");
	return temp;
}

//Help debug the UART register data
static const char *uart_offset[] = {
	"CONTROL REGISTER","MODE REGISTER","INTERRUPT ENABLE","INTERRUPT DISABLE","INTERRUPT MASK","CHANNEL STATUS",
	"RECEIVER HOLDING REGISTER","TRANSMITTER HOLDING REGISTER","BAUD RATE GENERATOR","RECEIVER TIME-OUT",
	"TRANSMITTER TIME-GUARD","RESERVED","RECEIVE POINTER","RECEIVE COUNTER","TRANSMIT POINTER","TRANSMIT COUNTER"
};
char *GetUARTOffset(int addr)
{
	int offset = addr & 0xff;
	if(offset <= 0x3c)
		sprintf(temp,"%s",uart_offset[offset/4]);
	else
		sprintf(temp,"** Unknown **");
	return temp;
}

void at91_build_priority_map()
{
	int i, c=0, j;

	for(i=7;i>=0;i--)
	{
		for (j=1;j<32;j++)
		{
			if ((at91.aic_smr[j] & 0x07) == i)
			{
				at91_priority_map[c++]=j;
			}
		}
	}
	assert(c==31);
}

//Update the status of the pio pins (1 bit per pin)
void update_pio_pins(int data)
{
	int prev = at91.pio_data_status;
	int changed;
	int interrupt;
	//update pin status
	at91.pio_data_status = data;
	//check for changes in pin
	changed = data ^ prev;
	//see if this will trigger an interrupt (interrupt must be enabled)
	interrupt = changed & at91.pio_irq_mask;
	//update irq status
	at91.pio_irq_status |= interrupt;
	//flag a pending PIO interrupt
	if(interrupt)
		at91.aic_irqpending |= 0x100;
}

mame_timer* at91_serial_timer=NULL;

static void serial_timer_event(int timer_num);

void at91_irqcheck()
{
	arm7_check_irq_state();
}

void at91_pending_serial(int usartno, int delaytime)
{
	// We can't process serial output immediately, the code will get confused and loop.
	// So we set up a timer to process it a little bit later.
	// I'm picking 60 * 60, because it needs to be able to write 
	// 54 characters 60 times per second.  If this gets behind 
	// things lag.   If this is too fast then it starts blocking
	// timer interrupts
	// The longer serial multi-byte strings seem to get hung up unless there is more delay
	// so added a delaytime parameter.  This became more apparent when I added more
	// nimble IRQ handling.  *Sigh*. 
	int freq = delaytime * 60;  
	timer_adjust(at91_serial_timer, TIME_IN_HZ(freq), 0, TIME_IN_HZ(freq));
}

static void serial_timer_event(int timer_num)
{
	unsigned int usartno, i;
	timer_adjust(at91_serial_timer, TIME_NEVER, 0, TIME_NEVER);

	for (usartno = 0; usartno < 2; usartno++)
	{
		// Doing a buffered transmit?
		if (at91usart[usartno].US_TCR > 0 && (at91usart[usartno].US_CSR & US_ENDTX) == 0)
		{
			if (at91_transmit_serial)
			{
				// TODO: There ought to be a way to get to the memory pointer and just pass it. 
				// (memory_region?) 

				data8_t *pData = (data8_t *)malloc(sizeof(data8_t) * at91usart[usartno].US_TCR);
				for (i = 0; i < at91usart[usartno].US_TCR; i++)
				{
					pData[i] = cpu_readmem32ledw(at91usart[usartno].US_TPR + i);
				}
				at91_transmit_serial(usartno, pData, at91usart[usartno].US_TCR);
				free(pData);
			}

			// Move pointer forward
			at91usart[usartno].US_TPR += at91usart[usartno].US_TCR;
			// Clear counter
			at91usart[usartno].US_TCR = 0x0;
			// Channel status TX complete + TX Ready
			at91usart[usartno].US_CSR |= US_TXRDY | US_TXEMPTY | US_ENDTX;
			// if irq enbled fire.
			if (at91usart[usartno].US_IER & (US_TXRDY | US_ENDTX))
			{
				at91_fire_irq(AT91_USART_IRQ(usartno));
			}
		}
		else
		{
			// Doing a character-by-character transmit? 
			if (at91usart[usartno].US_TPR == 0 && (at91usart[usartno].US_CSR & US_TXEMPTY) == 0)
			{
				if (at91_transmit_serial)
				{
					at91_transmit_serial(usartno, (data8_t*)&(at91usart[usartno].US_THR), 1);
				}
				// Channel status TX complete + TX Ready
				at91usart[usartno].US_CSR |= US_TXRDY + US_TXEMPTY;

				// Fire AIC IRQ for USART1.
				if ((at91usart[usartno].US_IER & (US_TXEMPTY | US_TXRDY)) > 0)
				{
					at91_fire_irq(AT91_USART_IRQ(usartno));
				}
			}
		}
		// Are there characters in the output buffer waiting, and AT91 has set up a buffered receive pointer? 
		while (at91usart[usartno].at91_rbuf_tail != at91usart[usartno].at91_rbuf_head && at91usart[usartno].US_RPR != 0 && at91usart[usartno].US_RCR > 0)
		{
			cpu_writemem32ledw(at91usart[usartno].US_RPR++, at91usart[usartno].at91_receivebuf[at91usart[usartno].at91_rbuf_tail]);
			if (at91usart[usartno].at91_rbuf_tail == AT91_RECEIVE_BUFFER_SIZE - 1)
				at91usart[usartno].at91_rbuf_tail = 0;
			at91usart[usartno].US_RCR--;
			at91usart[usartno].US_CSR |= US_ENDRX;
		}
	}
}


int at91_receive_serial(int usartno, data8_t *buf, int size)
{
	int remaining = size;

	if (remaining==0)
		return 0;

	while(remaining)
	{	
		// Is buffer full? 
		if ((at91usart[usartno].at91_rbuf_tail == 0 && at91usart[usartno].at91_rbuf_head == AT91_RECEIVE_BUFFER_SIZE-1) ||
			(at91usart[usartno].at91_rbuf_tail-1 == at91usart[usartno].at91_rbuf_head))
		{
			break;
		}
		at91usart[usartno].at91_receivebuf[at91usart[usartno].at91_rbuf_head++] = *(buf++);
		if (at91usart[usartno].at91_rbuf_head == AT91_RECEIVE_BUFFER_SIZE-1)
			at91usart[usartno].at91_rbuf_head = 0;
		remaining--;
	}
	// Let the ROM know there's something pending. 
	// TODO: Implement PDC buffer mode, check if receive buffer configured 
	// TODO: This should be moved into its own block of code too. 
	if (!(at91usart[usartno].US_CSR & (US_RXRDY | US_ENDRX)))
	{
		at91usart[usartno].US_CSR |= (US_RXRDY | US_ENDRX);
		at91_fire_irq(AT91_USART_IRQ(usartno));
	}
	return remaining;
}


void at91_usart_read(int usartno, int addr, data32_t *pData)
{
	switch ((addr & 0xff) / 4)
	{
	case 0x0c: // Receive pointer
		if (at91_serial_receive_ready && at91usart[usartno].US_RPR == 0x00)
			at91_serial_receive_ready(usartno);
		*pData = at91usart[usartno].US_RPR;
		break;
	case 0x0d:  // Receive counter
		*pData = at91usart[usartno].US_RCR;
		break;
	case 0x0e:  // Transmit pointer
		*pData = at91usart[usartno].US_TPR;
		break;
	case 0x0f:  // Transmit counter
		*pData = at91usart[usartno].US_TCR;
		break;
	case 0x06:  // Receiver holding register 
		*pData = at91usart[usartno].at91_receivebuf[at91usart[usartno].at91_rbuf_tail++];
		if (at91usart[usartno].at91_rbuf_tail == AT91_RECEIVE_BUFFER_SIZE-1)
			at91usart[usartno].at91_rbuf_tail = 0;
		// No more characters left in buffer?  Clear rxready. 
		if (at91usart[usartno].at91_rbuf_head == at91usart[usartno].at91_rbuf_tail) 
			at91usart[usartno].US_CSR &= ~(US_RXRDY | US_ENDRX);
		break;
	case 0x05:  // Channel status 
		*pData = at91usart[usartno].US_CSR;
		break;
	case 0x04: // Interrupt mask
		*pData = at91usart[usartno].US_IER & at91usart[usartno].US_CSR;
		break;
	default:
		break;
	}
	LOG(("%08x: AT91-USART%d READ: %s (%08x) = %08x\n", activecpu_get_pc(), usartno+1, GetUARTOffset(addr), addr, *pData));

}


void at91_usart_write(int usartno, int addr, data32_t outdata)
{

	LOG(("%08x: AT91-USART%d WRITE: %s (%08x) = %08x\n", activecpu_get_pc(), usartno+1, GetUARTOffset(addr), addr, outdata));
	switch ((addr & 0xff) / 4)
	{
	case 0x02: // Interrupt Enable
		// If we just enabled interrupts on something that would fire, we appear to need to fire here.
		// Without this, the transmit loop never begins in some cases. 
		if ((~at91usart[usartno].US_IER & outdata) & at91usart[usartno].US_CSR)
		{
			at91_fire_irq(AT91_USART_IRQ(usartno));
		}
		at91usart[usartno].US_IER |= outdata;
		
		break;
	case 0x03:
		at91usart[usartno].US_IER &= (~outdata);
		break;
	case 0x07:
		at91usart[usartno].US_THR = outdata; // Transmit character
		at91usart[usartno].US_CSR &= ~(US_TXRDY | US_TXEMPTY); 
		at91_pending_serial(usartno, 60);
		break;
	case 0x0c:
		at91usart[usartno].US_RPR = outdata; // Receive pointer
		break;
	case 0x0d:
		at91usart[usartno].US_RCR = outdata; // Receive counter
		at91usart[usartno].US_CSR &= ~(US_ENDRX); 
		break;
	case 0x0e:  // Transmit pointer
		at91usart[usartno].US_TPR = outdata;
		break;
	case 0x0f:  // Transmit counter
		at91usart[usartno].US_TCR = outdata;
		at91usart[usartno].US_CSR &= ~(US_ENDTX); 
		at91_pending_serial(usartno, 40);
		break;
	}
}

//WRITE TO  - Atmel AT91 CPU On Chip Periperhals
INLINE void internal_write (int addr, data32_t data)
{
	int offset = (addr & 0xFF000) >> 12;

	//EBI - External Bus Interface
	switch(offset)
	{
		//EBI OR SPECIAL FUNCTION
		case 0:
			//Special Function
			if(addr & 0x100000)
			{
				LOG(("%08x: AT91-SPECIAL_FUNC WRITE: %08x = %08x\n",activecpu_get_pc(),addr,data));
			}
			//EBI - External Bus Interface
			else
			{
				switch((UINT32)addr)
				{
					//EBI_CSR0 - EBI_CSR7
					case 0xffe00000:
					case 0xffe00004:
					case 0xffe00008:
					case 0xffe0000c:
					case 0xffe00010:
					case 0xffe00014:
					case 0xffe00018:
					case 0xffe0001c:
					{
						int dbw, nws, wse, pages, tdf, bat, csen, ba;
						char tmp[1000],tmp1[50],tmp2[50];
						dbw = data & 3;
						nws = (data & 0x1c)>>2;
						wse = (data & 0x20)>>5;
						pages = (data & 0x180)>>7;
						tdf = (data & 0xe00)>>9;
						bat = (data & 0x1000)>>12;
						csen = (data & 0x2000)>>13;
						ba = (data & 0xFFF00000);
						switch(pages)
						{
							case 0:
								sprintf(tmp1,"1M");
								break;
							case 1:
								sprintf(tmp1,"4M");
								break;
							case 2:
								sprintf(tmp1,"16M");
								break;
							case 3:
								sprintf(tmp1,"64M");
								break;
						}
						if(bat)
							sprintf(tmp2,"BYTE_SELECT");
						else
							sprintf(tmp2,"BYTE_WRITE");

						sprintf(tmp,"ba=%8x,dbw=%d, nws=%d,wse=%d,tdf=%d,csen=%d,pages=%s,bat=%s",ba,(dbw==1?16:8),nws,wse,tdf,csen,tmp1,tmp2);
						LOG(("%08x: AT91-EBI_CSR%d = %8x (%s)\n",activecpu_get_pc(),(addr - 0xffe00000) / 4,data,tmp));
						break;
					}

					//EBI_RCR - Remap Control Register
					//All AT91 chips contain internal ram (max of 1MB Block) - always mapped @ 0x300000 prior to remap, and 0x0 afterwards!
					case 0xffe00020:
						if(data & 1)
							{
								//Copy Reset RAM Contents into Page 0 RAM Address
								memcpy(at91rs.page0_ram_ptr, at91rs.reset_ram_ptr, 0x100000);
								at91.remap = 1;
								LOG(("%08x: AT91-EBI_RCR = 1 (RAM @ 0x300000 remapped to 0x0)!\n",activecpu_get_pc()));

								//the final program is now loaded - enable JIT translation
								jit_enable(at91.jit);
							}
						else
							LOG(("%08x: AT91-EBI_RCR = 0 (no effect)!\n",activecpu_get_pc()));
						break;

					case 0xffe00024:
					{
						int drp = (data & 0x10)>>4;
						int ale = data & 7;
						char tmp[50],tmp1[50];
						if(drp)
							sprintf(tmp1,"EBI_DRP_EARLY");
						else
							sprintf(tmp1,"EBI_DRP_STANDARD");
						switch(ale)
						{
							case 0:
							case 1:
							case 2:
							case 3:
								sprintf(tmp,"EBI_ALE_16M - A20-A23, No CS!");
								break;
							case 4:
								sprintf(tmp,"EBI_ALE_8M - A20-A22, CS4!");
								break;
							case 5:
								sprintf(tmp,"EBI_ALE_4M - A20-A21 CS4-CS5!");
								break;
							case 6:
								sprintf(tmp,"EBI_ALE_2M - A20, CS4-CS6!");
								break;
							case 7:
								sprintf(tmp,"EBI_ALE_1M - No A20-A23, CS4-CS7!");
								break;
						}
						LOG(("%08x: AT91-EBI_MCR = %x (%s)(%s)\n",activecpu_get_pc(),data,tmp,tmp1));
						break;
					}

					default:
						LOG(("%08x: AT91-EBI WRITE: %08x = %08x\n",activecpu_get_pc(),addr,data));
				}
			}
			break;

		//USART1
		case 0xcc:
			at91_usart_write(0, addr, data);
			break;
		//USART2
		case 0xd0:
			at91_usart_write(1, addr, data);
			break;

		//TC - Timer Counter
		case 0xe0:
		{
			int offset2 = addr & 0xff;
			int toffset = offset2;
			int timer_num = (offset2 & 0xc0)/0x40;	//keep bits 6,7
			int logit = LOG_TIMERS;

			//One of the Timer registers?
			if(offset2 < 0xc0)
			{
				//Adjust register offset for given clock
				toffset -= (0x40 * timer_num);

				switch(toffset)
				{
					//Channel Control
					case 0:
					{
						int enabled = data & 1;
						int disabled = data & 2;
						int trigger = data & 4;

						//Clear All Overflow Status (don't clear Enabled Status)
						CLEAR_BITS(at91.tc_clock[timer_num].tc_status,0x600FF);


						//Enable Clock
						if(enabled)
						{
							logit = LOG_TIMER_ENABLE;
							SET_BITS(at91.tc_clock[timer_num].tc_status,0x10000);		//Set Enabled status
							//?Does this really happen on an enable?
							at91.tc_clock[timer_num].started = 0;
						}

						//Disable Clock
						if(disabled)
						{
							logit = LOG_TIMER_DISABLE;
							CLEAR_BITS(at91.tc_clock[timer_num].tc_status,0x10000);		//Clear Enabled status
							at91.tc_clock[timer_num].started = 0;
							#if USE_MAME_TIMERS
							//Stop the timer
							if(at91rs.timer[timer_num])
								timer_adjust(at91rs.timer[timer_num],TIME_NEVER,timer_num,TIME_NEVER);
							#endif
						}

						//Sofware Trigger - Start Clock & Reset Counter
						if(trigger)
						{
							logit = LOG_TIMER_START;
							at91.tc_clock[timer_num].tc_counter = 0;
							//Trigger only starts clock if enabled
							if(TC_ENABLED(timer_num))
							{
								at91.tc_clock[timer_num].started = 1;

								#if USE_MAME_TIMERS
								//Setup an RC Compare Timer?
								if(TC_RC_TRIGGER(timer_num) && TC_RC_IRQ_ENABLED(timer_num))
								{
									//Base clock is the CPU Clock Frequency
									int freq = at91rs.cpu_freq;
									//Account for clock divisor & Register C value
									freq /= at91.tc_clock[timer_num].tc_cycle_div;
									freq /= at91.tc_clock[timer_num].tc_regc;

									if(logit) LOG(("Timer TC%d starting up for RC Compare @ Freq: %d\n",timer_num,freq));

									//Start up the timer
									if(at91rs.timer[timer_num])
										timer_adjust(at91rs.timer[timer_num],TIME_IN_HZ(freq),timer_num,TIME_IN_HZ(freq));
								}
								else
									LOG(("TC%d setup as non-rc compare timer - not supported\n",timer_num));
								#endif
							}
						}
						break;
					}

					//Channel Mode
					case 0x04:
					{
						//see page 135 of AT91 Datasheet
						const int divs[]={2,8,32,128,1024,0,0,0};
						logit = LOG_TIMERS;		//disable the one later on..

						//store data
						at91.tc_clock[timer_num].tc_chan_mode = data;

						//log the raw data
						if(logit)	LOG(("%08x: AT91-TIMER WRITE (TC%d): %s (%08x) = %08x\n",activecpu_get_pc(),timer_num,GetTimerOffset(addr),addr,data));

						//Wave Mode
						if(TC_WAVEMODE(timer_num))
						{
							char tmp[1000];
							int clk, clki, burst, cpcstop, cpcdis, eevtedg, eevt, enetrg, cpctrg, wave,
								acpa, acpc, aeevt, aswtrg, bcpb, bcpc, beevt, bswtrg;
							int divr;
							clk = (data & 7);
							clki = (data & 8)>>3;
							burst = (data & 0x30) >> 4;
							cpcstop = (data & 0x40) >> 6;
							cpcdis = (data & 0x80) >> 7;
							eevtedg = (data & 0x300)>>8;
							eevt = (data & 0xc00)>>10;
							enetrg = (data & 0x1000) >> 12;
							cpctrg = (data & 0x4000)>>14;
							wave = (data & 0x8000)>>15;
							acpa = (data & 0x30000)>>16;
							acpc = (data & 0xc0000)>>18;
							aeevt = (data & 0x300000)>>20;
							aswtrg = (data & 0xc00000)>>22;
							bcpb = (data & 0x3000000)>>24;
							bcpc = (data & 0xc000000)>>26;
							beevt = (data & 0x30000000)>>28;
							bswtrg = (data & 0xc0000000)>>30;

							//Convert Clock Value to Divisor Value
							divr = divs[clk];
							at91.tc_clock[timer_num].tc_cycle_div = divr;

							sprintf(tmp,"clk=%d (mclk/%d),clki=%d,burst=%d,cpcstop=%d,cpcdis=%d,eevtedg=%d,eevt=%d,enetrg=%d,cpctrg=%d,wave=%d,acpa=%d, acpc=%d, aeevt=%d, aswtrg=%d, bcpb=%d, bcpc=%d, beevt=%d, bswtrg=%d \n",
										clk, divr, clki, burst, cpcstop, cpcdis, eevtedg, eevt, enetrg, cpctrg,wave,
										acpa, acpc, aeevt, aswtrg, bcpb, bcpc, beevt, bswtrg);

							if(logit)	LOG(("%s",tmp));

							LOG(("Timer TC%d - WAVE MODE NOT SUPPORTED!\n",timer_num));
						}
						//Capture Mode
						else
						{
							char tmp[1000];
							int clk, clki, burst, ldbstop, ldbdis, etrgedge, abetrg, cpctrg, wave, ldra, ldrb;
							int divr;
							clk = (data & 7);
							clki = (data & 8)>>3;
							burst = (data & 0x30) >> 4;
							ldbstop = (data & 0x40)>>6;
							ldbdis = (data & 0x80)>>7;
							etrgedge = (data & 0x300)>>8;
							abetrg = (data & 0x400)>>10;
							cpctrg = (data & 0x4000)>>14;
							wave = (data & 0x8000)>>15;
							ldra = (data & 0x30000)>>16;
							ldrb = (data & 0x60000)>>18;
							//Convert Clock Value to Divisor Value
							divr = divs[clk];
							at91.tc_clock[timer_num].tc_cycle_div = divr;

							sprintf(tmp,"clk=%d (mclk/%d),clki=%d,burst=%d,ldbstop=%d,ldbdis=%d,etrgedge=%d,abetrg=%d,cpctrg=%d,wave=%d,ldra=%d,ldrb=%d \n",
										clk, divr, clki, burst, ldbstop, ldbdis, etrgedge, abetrg,
										cpctrg, wave, ldra, ldrb);
							if(logit)	LOG(("%s",tmp));

						}
						break;
					}

					//Counter Value
					case 0x10:
						at91.tc_clock[timer_num].tc_counter = data;
						#if USE_MAME_TIMERS
						LOG(("Timer TC%d - Writing Counter Value not supported with MAME TIMERS!\n",timer_num));
						#endif
						break;

					//Register A
					case 0x14:
						at91.tc_clock[timer_num].tc_rega = data;
						break;

					//Register B
					case 0x18:
						at91.tc_clock[timer_num].tc_regb = data;
						break;

					//Register C
					case 0x1c:
						at91.tc_clock[timer_num].tc_regc = data;
						break;

					//IRQ Enable
					case 0x24:
						SET_BITS(at91.tc_clock[timer_num].tc_irq_mask,data);
						break;

					//IRQ Disable
					case 0x28:
						CLEAR_BITS(at91.tc_clock[timer_num].tc_irq_mask,data);
						break;
				}
				if(logit)	LOG(("%08x: AT91-TIMER WRITE (TC%d): %s (%08x) = %08x\n",activecpu_get_pc(),timer_num,GetTimerOffset(addr),addr,data));
			} else
			if (offset2 == 0xc0) {
				if(logit)	LOG(("%08x: AT91-TIMER WRITE (TC BLOCK CONTROL REGISTER): %08x = %08x\n",activecpu_get_pc(),addr,data));
			} else
			if (offset2 == 0xc4) {
				at91.tc_block_mode = data;
				if(logit)	LOG(("%08x: AT91-TIMER WRITE (TC BLOCK MODE REGISTER): %08x = %08x\n",activecpu_get_pc(),addr,data));
			}
			else
				if(logit)	LOG(("%08x: AT91-TIMER WRITE: %08x = %08x\n",activecpu_get_pc(),addr,data));
			break;
		}
		//PIO - Parallel I/O Controller
		case 0xf0:
		{
			int offset2 = addr & 0xff;
			int logit = LOG_PIO;
			if(logit) LOG(("%08x: AT91-PIO WRITE: %s (%08x) = %08x\n",activecpu_get_pc(),GetPIOOffset(addr),addr,data));
			switch(offset2)
			{
				//PIO Enable Register
				case 0x00:
					SET_BITS(at91.pio_enabled_status,data);
					break;
				//PIO Disable Register
				case 0x04:
					data &= (0x18F0000^0xffffffff);		//Pins 16,17,18,19,23,24 are Dedicated PIO lines, and cannot be cleared.
					CLEAR_BITS(at91.pio_enabled_status,data);
					break;
				//PIO Output Enable Register
				case 0x10:
					SET_BITS(at91.pio_output_status,data);
					break;
				//PIO Output Disable Register
				case 0x14:
					CLEAR_BITS(at91.pio_output_status,data);
					break;
				//PIO Input Filter Enable Register
				case 0x20:
					break;
				//PIO Input Filter Disable Register
				case 0x24:
					break;
				//PIO Set Output Data Register
				case 0x30:
					logit = LOG_PIO_WRITE;
					//disregard (mask) any pins not set as outputs
					data &= at91.pio_output_status;
					//disregard (mask) any pins not controlled by the PIO
					data &= at91.pio_enabled_status;
					//Update status
					SET_BITS(at91.pio_output_data_status,data);
					//if pins are still available, we can now post that they be set.
					if(data)
					{
						//add newly set bits from the old status
						int newdata = at91.pio_data_status | data;
						//post data to machine port handler (but only bits set as output)
						cpu_writeport32ledw_dword(0,newdata & at91.pio_output_status);
						//update pin data status
						update_pio_pins(newdata);
					}
					break;
				//PIO Clear Output Data Register
				case 0x34:
					logit = LOG_PIO_WRITE;
					//disregard (mask) any pins not set as outputs
					data &= at91.pio_output_status;
					//disregard (mask) any pins not controlled by the PIO
					data &= at91.pio_enabled_status;
					//Update status
					CLEAR_BITS(at91.pio_output_data_status,data);
					//if pins are still available, we can now post that they be cleared.
					if(data)
					{
						//mask off newly cleared bits from the old status
						int newdata = at91.pio_data_status & ~data;
						//post data to machine port handler (but only bits set as output)
						cpu_writeport32ledw_dword(0,newdata & at91.pio_output_status);
						//update pin data status
						update_pio_pins(newdata);
					}
					break;
				//PIO Interrupt Enable Register
				case 0x40:
					SET_BITS(at91.pio_irq_mask,data);
					break;
				//PIO Interrupt Disable Register
				case 0x44:
					CLEAR_BITS(at91.pio_irq_mask,data);
					break;
			}
	//		if(logit) LOG(("%08x: AT91-PIO WRITE: %s (%08x) = %08x\n",activecpu_get_pc(),GetPIOOffset(addr),addr,data));
			break;
		}

 		//PS - Power Saving
		case 0xf4:
		{
			int logit = LOG_POWER;
			switch(addr)
			{
				//Control Register
				case 0xffff4000:
					break;
				//Peripheral Clock Enable
				case 0xffff4004:
					SET_BITS(at91.power_status,data);
					//TODO: If using MAME Timers - must un-pause the timer - if paused (use timer_enable(1)?)
					#if USE_MAME_TIMERS
					if(data & 0x70)
						LOG(("Enable Clock from Power Saver Registers is not supported with MAME TIMERS!\n"));
					#endif
					break;
				//Peripheral Clock Disable
				case 0xffff4008:
					CLEAR_BITS(at91.power_status,data);
					//TODO: If using MAME Timers - must pause the timer (use timer_enable(0)?)
					#if USE_MAME_TIMERS
					if(data & 0x70)
						LOG(("Disable Clock from Power Saver Registers is not supported with MAME TIMERS!\n"));
					#endif
					break;
			}
			if(logit) LOG(("%08x: AT91-POWER WRITE: %s (%08x) = %08x\n",activecpu_get_pc(),GetPSOffset(addr),addr,data));
			break;
		}

		//WD - Watchdog Timer
		case 0xf8:
 			LOG(("%08x: AT91-WATCHDOG WRITE: %08x = %08x\n",activecpu_get_pc(),addr,data));
			break;

		//AIC - Advanced Interrupt Controller
		case 0xff:
		{
			int logit = LOG_AIC;
			int offset2 = addr & 0x1ff;
			switch(offset2)
			{
			case 0x00:
				case 0x04:
				case 0x08:
				case 0x0c:
				case 0x10:
				case 0x14:
				case 0x18:
				case 0x1c:
				case 0x20:
				case 0x24:
				case 0x28:
				case 0x2c:
				case 0x30:
				case 0x34:
				case 0x38:
				case 0x3c:
				case 0x40:
				case 0x44:
				case 0x48:
				case 0x4c:
				case 0x50:
				case 0x54:
				case 0x58:
				case 0x5c:
				case 0x60:
				case 0x64:
				case 0x68:
				case 0x6c:
				case 0x70:
				case 0x74:
				case 0x78:
				case 0x7c:
					at91.aic_smr[(addr-0xfffff000)/4] = data;
					at91_build_priority_map();
					break;
									//AIC Interrupt Vector Register - Based on current IRQ source 0-31, returns value of Source Vector

			case 0x80:
				case 0x84:
				case 0x88:
				case 0x8c:
				case 0x90:
				case 0x94:
				case 0x98:
				case 0x9c:
				case 0xa0:
				case 0xa4:
				case 0xa8:
				case 0xac:
				case 0xb0:
				case 0xb4:
				case 0xb8:
				case 0xbc:
				case 0xc0:
				case 0xc4:
				case 0xc8:
				case 0xcc:
				case 0xd0:
				case 0xd4:
				case 0xd8:
				case 0xdc:
				case 0xe0:
				case 0xe4:
				case 0xe8:
				case 0xec:
				case 0xf0:
				case 0xf4:
				case 0xf8:
				case 0xfc:
					at91.aic_vectors[(addr-0xfffff080)/4] = data;
					break;

				//Interrupt Vector Write
				case 0x100:
					logit = LOG_AIC_IVW;
					break;

				//AIC Interrupt Enable
				case 0x120:
					SET_BITS(at91.aic_irqmask,data);
					at91_irq_assert_if_greater();
					break;
				//AIC Interrupt Disable
				case 0x124:
					CLEAR_BITS(at91.aic_irqmask,data);
					break;
				// AIC Interrupt Clear
				case 0x128:
					CLEAR_BITS(at91.aic_irqpending,data);
					break;
				case 0x12C:
					SET_BITS(at91.aic_irqpending,data);
					break;

				//End of Interrupt
				case 0x130:
					logit = LOG_AIC_EOI;
					if (at91_irqstackpos > 0)
					{
						at91.aic_irqstatus = at91_irqstack[--at91_irqstackpos];
						at91_irq_assert_if_greater();
					}
					break;
				case 0x134:
					at91.aic_spurious = data ;
					break;
			}
			if(logit) LOG(("%08x: AT91-AIC WRITE: %s (%08x) = %08x\n",activecpu_get_pc(),GetAICOffset(addr),addr,data));
			break;
		}

 		default:
			LOG(("%08x: AT91-OCP WRITE: %08x = %08x\n",activecpu_get_pc(),addr,data));
	}
}


//READ FROM  - Atmel AT91 CPU On Chip Periperhals
INLINE data32_t internal_read (int addr)
{
	data32_t data = 0;
	int offset2 = (addr & 0xFF000) >> 12;

	//EBI - External Bus Interface
	switch(offset2)
	{
		//EBI OR SPECIAL FUNCTION
		case 0:
			//Special Function
			if(addr & 0x100000)
			{
				LOG(("%08x: AT91-SPECIAL_FUNC READ: %08x = %08x\n",activecpu_get_pc(),addr,data));
			}
			//EBI - External Bus Interface
			else
			{
				LOG(("%08x: AT91-EBI READ: %08x = %08x\n",activecpu_get_pc(),addr,data));
			}
			break;

		//USART1
		case 0xcc:
			at91_usart_read(0, addr, &data);
			break;
		//USART2
		case 0xd0:
			at91_usart_read(1, addr, &data);
			break;
		//TC - Timer Counter
		case 0xe0:
		{
			int offset3 = addr & 0xff;
			int toffset = offset3;
			int timer_num = (offset3 & 0xc0)/0x40;	//keep bits 6,7
			int logit = LOG_TIMERS;

			//One of the Timer registers?
			if(offset3 < 0xc0)
			{
				//Adjust register offset
				toffset -= (0x40 * timer_num);

				switch(toffset)
				{
					//Channel Mode
					case 0x04:
						data = at91.tc_clock[timer_num].tc_chan_mode;
						break;

					//Counter Value
					case 0x10:

#if USE_MAME_TIMERS
						LOG(("Timer TC%d - Reading Counter Value not supported with MAME TIMERS!\n",timer_num));
						data = at91.tc_clock[timer_num].tc_counter;// ARM7_ICOUNT;
#else
						data = at91.tc_clock[timer_num].tc_counter;
#endif

						break;

					//Register A
					case 0x14:
						data = at91.tc_clock[timer_num].tc_rega;
						break;

					//Register B
					case 0x18:
						data = at91.tc_clock[timer_num].tc_regb;
						break;

					//Register C
					case 0x1c:
						data = at91.tc_clock[timer_num].tc_regc;
						break;

					//Status
					case 0x20:
						data = at91.tc_clock[timer_num].tc_status;
						at91.tc_clock[timer_num].tc_status &= 0x10000;		//Reading the status, clears it (not the clock enabled bit though)
						logit = LOG_TIMER_STAT_READ;
						break;

					//IRQ Mask
					case 0x2c:
						data = at91.tc_clock[timer_num].tc_irq_mask;
						break;
				}
				if(logit)	LOG(("%08x: AT91-TIMER READ (TC%d): %s (%08x) = %08x\n",activecpu_get_pc(),timer_num,GetTimerOffset(addr),addr,data));
			} else
			if (offset3 == 0xc0) {
				//Timer Block Control
				if(logit)	LOG(("%08x: AT91-TIMER READ (TC BLOCK CONTROL REGISTER): %08x = %08x\n",activecpu_get_pc(),addr,data));
			} else
			if (offset3 == 0xc4) {
				//Timer Block Mode
				data = at91.tc_block_mode;
				if(logit)	LOG(("%08x: AT91-TIMER READ (TC BLOCK MODE REGISTER): %08x = %08x\n",activecpu_get_pc(),addr,data));
			} else
				//??
				LOG(("%08x: AT91-TIMER READ: %08x = %08x\n",activecpu_get_pc(),addr,data));
			break;
		}

		//PIO - Parallel I/O Controller
		case 0xf0:
		{
			int offset3 = addr & 0xff;
			int logit = LOG_PIO;

			switch(offset3)
			{
				//PIO (Enabled) Status Register
				case 0x08:
					data = at91.pio_enabled_status;
					break;
				//PIO Output Status Register
				case 0x18:
					data = at91.pio_output_status;
					break;
				//PIO Input Filter Status Register
				case 0x28:
					break;
				//PIO Output Data Status Register
				case 0x38:
					data = at91.pio_output_data_status;
					break;
				//PIO Pin Data Status Register
				case 0x3c:
					logit = LOG_PIO_READ;
					//pull data from machine port handler
					data = cpu_readport32ledw_dword(0);
					//disregard (mask) any pins not set as inputs
					data &= (at91.pio_output_status ^ 0xffffffff);
					//disregard (mask) any pins not controlled by the PIO
					data &= at91.pio_enabled_status;
					//update pin data status
					update_pio_pins(at91.pio_data_status | data);
					//read the full status of the pins
					data = at91.pio_data_status;
					break;
				//PIO Interrupt Mask Register
				case 0x48:
					data = at91.pio_irq_mask;
					break;
				//PIO Interrupt Status Register
				case 0x4c:
					data = at91.pio_irq_status;
					//cleared on a read
					at91.pio_irq_status = 0;
					break;
			}
			//must come last, otherwise, logged data read is not yet set.
			if(logit) LOG(("%08x: AT91-PIO READ: %s (%08x) = %08x\n",activecpu_get_pc(),GetPIOOffset(addr),addr,data));
			break;
		}

		//PS - Power Saving
		case 0xf4:
		{
			int logit = LOG_POWER;

			//Peripheral Clock Status
			if(addr == 0xffff400c)
				data = at91.power_status;

			if(logit) LOG(("%08x: AT91-POWER READ: %s (%08x) = %08x\n",activecpu_get_pc(),GetPSOffset(addr),addr,data));
			break;
		}

		//WD - Watchdog Timer
		case 0xf8:
			LOG(("%08x: AT91-WATCHDOG READ: %08x = %08x\n",activecpu_get_pc(),addr,data));
			break;

		//AIC - Advanced Interrupt Controller
		case 0xff:
		{
			int offset3 = addr & 0xfff;
			int logit = LOG_AIC;

			switch(offset3)
			{
				//IRQ Based on current IRQ source 0-31, returns value of Source Vector
				case 0x100: { // IVR
					// Find the highest ranked vector that's set in irqpending

					int i;
					for (i = 0; i < 31; i++)
					{
						if (( at91.aic_irqpending & at91.aic_irqmask) & (1 << at91_priority_map[i]))
						{
							at91_irqstack[at91_irqstackpos++] = at91.aic_irqstatus;
							if (at91_irqstackpos >= 32)
							{
								// something's wrong if we get here.
								at91_irqstackpos = 31;
							}
							at91.aic_irqstatus = at91_priority_map[i];
							data = at91.aic_vectors[at91_priority_map[i]];
							at91.aic_irqpending &= ~(1 << at91_priority_map[i]);
							break;
						}
					}
					if (i==31) 
					{
						at91_irqstack[at91_irqstackpos++] = 0;
						data = at91.aic_spurious;
					}
					ARM7.pendingIrq = 0;
//					arm7_core_set_irq_line(ARM7_IRQ_LINE, 0);
					}
					break;

				//FIQ - Has it's own register address
				case 0x104:
					logit = LOG_AIC_VECTOR_READ;
					data = at91.aic_vectors[0];
					ARM7.pendingFiq = 0;
					break;

				//Interrupt Status
				case 0x108:
					data = at91.aic_irqstatus;
					break;

				//Interrupt Pending
				case 0x10c:
					data = at91.aic_irqpending;
					break;

				//Interrupt Mask
				case 0x110:
					data = at91.aic_irqmask;
					break;

				//Pending status
				case 0x114:
					data = ARM7.pendingIrq << 1 | ARM7.pendingFiq;
					break;
			}
			if(logit)	LOG(("%08x: AT91-AIC READ: %s (%08x) = %08x\n",activecpu_get_pc(),GetAICOffset(addr),addr,data));
			break;
		}

		default:
			LOG(("%08x: AT91-OCP READ: %08x = %08x\n",activecpu_get_pc(),addr,data));
	}
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

	if(addr&3)
	{
		int val = addr & 3;
		if(val != 2)
			LOG(("%08x: MISALIGNED half word read @ %08x:\n",R15,addr));
	}

	//Handle through normal 32 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw_word(addr);
}

INLINE data8_t at91_cpu_read8( int addr )
{
	//Atmel AT91 CPU On Chip Periperhals Mapped here
	if(addr >= 0xFFC00000)
		return (data8_t)internal_read(addr);

	//Handle through normal 8 bit handler ( for 32 bit cpu )
	return cpu_readmem32ledw(addr);
}


/***************************************************************************/

void at91_reset(void *param)
{
	int i;
	//clear out timers
	for(i=0;i<MAX_TIMER;i++)
	{
		if(at91rs.timer[i])
			timer_adjust(at91rs.timer[i],TIME_NEVER,i,TIME_NEVER);
	}
	//if we've remapped, we must un-map - BEFORE we reset, but we do it from our copy, as real memory might have changed!
	if(at91.remap)
		memcpy(at91rs.page0_ram_ptr, &at91rs.page0, 0x100000);
	//otherwise copy our page0 memory region for the next time we reset.
	else
		memcpy(&at91rs.page0, at91rs.page0_ram_ptr, 0x100000);

	//must call core...
	arm7_core_reset(param);

	//reset state of certain things
	at91.pio_enabled_status = 0x1FFFFFF;
	at91.power_status = 0x17c;
}

void at91_exit(void)
{
	int i;
	//remove timers
	for(i=0;i<MAX_TIMER;i++)
	{
		if(at91rs.timer[i])
			timer_remove(at91rs.timer[i]);
	}
	timer_remove(at91_serial_timer);
	//core cleanup
	arm7_core_exit();
}

int at91_execute( int cycles )
{
	/*include the arm7 core execute code*/
	#include "../arm7/arm7exec.c"
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
	case ARM732_FR9: return ARMREG(eR9_FIQ);
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
	case ARM732_R0: ARMREG( 0)= val; break;
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

void at91_irq_assert_if_greater()
{
	if (at91.aic_irqstatus > 0)
	{
		// We only want to assert if this IRQ is higher in priority than what's currently being processed.
		int i;

		for (i = 0; i < 31; i++)
		{
			if (at91_priority_map[i] == at91.aic_irqstatus)
				break;
			if ((( at91.aic_irqpending & at91.aic_irqmask) & ( 1 << at91_priority_map[i])))
			{
				ARM7.pendingIrq = 1;
				break;
			}
		}
	} else {
		if (( at91.aic_irqpending & at91.aic_irqmask))
			ARM7.pendingIrq = 1;
	}
}

void at91_fire_irq(int irqline)
{
//	if ((at91.aic_irqmask & (1<<irqline)) == 0)
//	{
//		return;
//	}
//  Always mark as pending, but don't fire if masked. 

	if(irqline == AT91_FIQ_IRQ)
	{
		cpu_set_irq_line(at91rs.cpunum, ARM7_FIRQ_LINE, PULSE_LINE);
	}
	else
	{
		at91.aic_irqpending |= (1 << irqline);
		at91_irq_assert_if_greater();
	}
}

void at91_set_irq_line(int irqline, int state)
{
	arm7_core_set_irq_line(irqline,state);
}

void at91_set_irq_callback(int (*callback)(int irqline))
{
}

static const data8_t at91_reg_layout[] =
{
	0xFF,
	ARM732_R0,  ARM732_IR13, 0xFF,
	ARM732_R1,  ARM732_IR14, 0xFF,
	ARM732_R2,  ARM732_ISPSR, 0xFF,
	ARM732_R3,  0xFF,
	ARM732_R4,  ARM732_FR8,  0xFF,
	ARM732_R5,  ARM732_FR9,  0xFF,
	ARM732_R6,  ARM732_FR10, 0xFF,
	ARM732_R7,  ARM732_FR11, 0xFF,
	ARM732_R8,  ARM732_FR12, 0xFF,
	ARM732_R9,  ARM732_FR13, 0xFF,
	ARM732_R10, ARM732_FR14, 0xFF,
	ARM732_R11, ARM732_FSPSR, 0xFF,
	ARM732_R12, 0xFF,
	ARM732_R13, ARM732_AR13, 0xFF,
	ARM732_R14, ARM732_AR14, 0xFF,
	ARM732_R15, ARM732_ASPSR, 0xFF,
	0xFF,
	ARM732_SR13, ARM732_UR13, 0xFF,
	ARM732_SR14, ARM732_UR14, 0xFF,
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
	//arm7_disasm( buffer, pc, READ32(pc));
	arm7_disasm( buffer, pc, cpu_readop32(pc));
	return 4;
#else
	sprintf(buffer, "$%08x", READ32(pc));
	return 4;
#endif
}

void at91_init(void)
{
	int i;

	//clear out static data - since for VPM this code can be re-run with values from last game run which causes issues
	memset(&at91, 0, sizeof(at91));
	memset(&at91rs, 0, sizeof(at91rs));
	at91_ICount = 0;

	//must call core...
	arm7_core_init("at91");

	//allocate timers
	for(i=0;i<MAX_TIMER;i++)
	{
		at91rs.timer[i] = timer_alloc(timer_trigger_event);
	}
	at91_serial_timer = timer_alloc(serial_timer_event);

	//Store the cpu clock frequency (is there any easier way to get this?)
	at91rs.cpu_freq = Machine->drv->cpu[activecpu].cpu_clock;
	at91rs.cpunum = activecpu;
	at91_build_priority_map();
	return;
}

#if USE_MAME_TIMERS

// Certain programs have problems with the deferred timer interrupts, while most others
// work better. Expose a setting.
int at91_block_timers = 0;

static void timer_trigger_event(int timer_num)
{
	//reset counter and flag status
	at91.tc_clock[timer_num].tc_counter = 0;
	at91.tc_clock[timer_num].tc_status |= 0x10;

	//generate an interrupt?
	if(TC_RC_IRQ_ENABLED(timer_num) /* && */)
	{
		if (!at91_block_timers || (GET_CPSR & I_MASK) ==0)
			at91_fire_irq(AT91_TC0_IRQ+timer_num);	
	}
}
#else
INLINE BeforeOpCodeHook(void)
{
	//Update timers if running
	if( (TC_RUNNING(0) || TC_RUNNING(1) || TC_RUNNING(2)) )			//for speed we use this, but really it should loop and check using MAX_TIMER var
	{
		int i;
		int cycles = 0;

		//Check each timer
		for(i=0; i<MAX_TIMER; i++)
		{
			cycles = at91.prev_cycles;				//How many cycles have passed since last check?
			cycles+= at91.tc_clock[i].halfticks;	//Add half ticks since last time
			//Is this timer running?
			if(TC_RUNNING(i))
			{
				int divr = at91.tc_clock[i].tc_cycle_div;
				int ticks = cycles/divr;
				int half = cycles%divr;

				//increment counter by # of ticks & store # of half ticks
				at91.tc_clock[i].tc_counter+=ticks;
				at91.tc_clock[i].halfticks = half;

				//check for rc compare trigger if turned on
				if(TC_RC_TRIGGER(i))
				{
					//rc compare trigger?
					if(at91.tc_clock[i].tc_counter >= at91.tc_clock[i].tc_regc)
					{
						//reset counter and flag status
						at91.tc_clock[i].tc_counter = 0;
						at91.tc_clock[i].tc_status |= 0x10;

						//generate an interrupt?
						if(TC_RC_IRQ_ENABLED(i))
						{
							at91_set_irq_line(AT91_TC0_IRQ+i,1);
						}
					}
				}

				//check overflow condtion
				if(at91.tc_clock[i].tc_counter >= 0xFFFF)
				{
					//reset counter and flag status
					at91.tc_clock[i].tc_counter = 0;
					at91.tc_clock[i].tc_status |= 1;

					//generate an interrupt?
					if(TC_OVRFL_IRQ_ENABLED(i))
					{
						at91_set_irq_line(AT91_TC0_IRQ+i,1);
					}
				}
			}
		}
	}
}
INLINE AfterOpCodeHook(void)
{

}
#endif

//
// Include the ARM7 JIT
//
#include "../arm7/arm7jit.c"
