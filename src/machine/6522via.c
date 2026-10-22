// license:BSD-3-Clause
// copyright-holders:Peter Trauner, Mathis Rosenhauer
/**********************************************************************

    Rockwell 6522 VIA interface and emulation

	This function emulates the functionality of up to 8 6522
	versatile interface adapters.

    This is based on the M6821 emulation in MAME.

    To do:

	T2 pulse counting mode
    Pulse mode handshake output
	Shift register

	The optional timing accurate core (VIA6522_ACCURATE in 6522via.h)
	follows MAME's current 6522via.cpp instead:
	timers reload after N+2 cycles, exact counter reads in all modes, T1
	PB7 output, T2 pulse counting and 8 bit mode, shift register, input
	latching, CA2 pulse output and the IRQ line only driven on changes.
	Ports keep the PinMAME conventions of the old core (outputs are passed
	masked with DDR, inputs come from the read handlers).

**********************************************************************/

/*
  1999-Dec-22 PeT
   vc20 random number generation only partly working
   (reads (uninitialized) timer 1 and timer 2 counter)
   timer init, reset, read changed
 */

#include "driver.h"
#include "6522via.h"

//#define TRACE_VIA

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

/******************* internal VIA data structure *******************/

struct via6522
{
	const struct via6522_interface *intf;

	UINT8 in_a;
	UINT8 in_ca1;
	UINT8 in_ca2;
	UINT8 out_a;
	UINT8 out_ca2;
	UINT8 ddr_a;

	UINT8 in_b;
	UINT8 in_cb1;
	UINT8 in_cb2;
	UINT8 out_b;
	UINT8 out_cb2;
	UINT8 ddr_b;

	UINT8 t1cl;
	UINT8 t1ch;
	UINT8 t1ll;
	UINT8 t1lh;
	UINT8 t2cl;
	UINT8 t2ch;
	UINT8 t2ll;
	UINT8 t2lh;

	UINT8 sr;
	UINT8 pcr;
	UINT8 acr;
	UINT8 ier;
	UINT8 ifr;

	mame_timer *t1;
	double time1;
	char t1_active;
	mame_timer *t2;
	double time2;
	char t2_active;

	double cycles_to_sec;
	double sec_to_cycles;

#if VIA6522_ACCURATE
	UINT8 latch_a;
	UINT8 latch_b;
	UINT8 out_cb1;
	UINT8 t1_pb7;
	UINT8 shift_done;
	UINT8 shift_counter;
	UINT16 t1_value;
	UINT16 t2_start;
	double t2_load;
	mame_timer *ca2_timer;
	mame_timer *cb2_timer;
	mame_timer *shift_timer;
	mame_timer *shift_irq_timer;
#endif
};


/******************* convenince macros and defines *******************/

#define V_CYCLES_TO_TIME(c) ((double)(c) * v->cycles_to_sec)
#define V_TIME_TO_CYCLES(t) ((int)((t) * v->sec_to_cycles + 0.5)) // round

/* Macros for PCR */
#define CA1_LOW_TO_HIGH(c)		(c & 0x01)
#define CA1_HIGH_TO_LOW(c)		(!(c & 0x01))

#define CB1_LOW_TO_HIGH(c)		(c & 0x10)
#define CB1_HIGH_TO_LOW(c)		(!(c & 0x10))

#define CA2_INPUT(c)			(!(c & 0x08))
#define CA2_LOW_TO_HIGH(c)		((c & 0x0c) == 0x04)
#define CA2_HIGH_TO_LOW(c)		((c & 0x0c) == 0x00)
#define CA2_IND_IRQ(c)			((c & 0x0a) == 0x02)

#define CA2_OUTPUT(c)			(c & 0x08)
#define CA2_AUTO_HS(c)			((c & 0x0c) == 0x08)
#define CA2_HS_OUTPUT(c)		((c & 0x0e) == 0x08)
#define CA2_PULSE_OUTPUT(c)		((c & 0x0e) == 0x0a)
#define CA2_FIX_OUTPUT(c)		((c & 0x0c) == 0x0c)
#define CA2_OUTPUT_LEVEL(c)		((c & 0x02) >> 1)

#define CB2_INPUT(c)			(!(c & 0x80))
#define CB2_LOW_TO_HIGH(c)		((c & 0xc0) == 0x40)
#define CB2_HIGH_TO_LOW(c)		((c & 0xc0) == 0x00)
#define CB2_IND_IRQ(c)			((c & 0xa0) == 0x20)

#define CB2_OUTPUT(c)			(c & 0x80)
#define CB2_AUTO_HS(c)			((c & 0xc0) == 0x80)
#define CB2_HS_OUTPUT(c)		((c & 0xe0) == 0x80)
#define CB2_PULSE_OUTPUT(c)		((c & 0xe0) == 0xa0)
#define CB2_FIX_OUTPUT(c)		((c & 0xc0) == 0xc0)
#define CB2_OUTPUT_LEVEL(c)		((c & 0x20) >> 5)

/* Macros for ACR */
#define PA_LATCH_ENABLE(c)		(c & 0x01)
#define PB_LATCH_ENABLE(c)		(c & 0x02)

#define SR_DISABLED(c)			(!(c & 0x1c))
#define SI_T2_CONTROL(c)		((c & 0x1c) == 0x04)
#define SI_O2_CONTROL(c)		((c & 0x1c) == 0x08)
#define SI_EXT_CONTROL(c)		((c & 0x1c) == 0x0c)
#define SO_T2_RATE(c)			((c & 0x1c) == 0x10)
#define SO_T2_CONTROL(c)		((c & 0x1c) == 0x14)
#define SO_O2_CONTROL(c)		((c & 0x1c) == 0x18)
#define SO_EXT_CONTROL(c)		((c & 0x1c) == 0x1c)

#define T1_SET_PB7(c)			(c & 0x80)
#define T1_CONTINUOUS(c)		(c & 0x40)
#define T2_COUNT_PB6(c)			(c & 0x20)

/* Interrupt flags */
#define INT_CA2	0x01
#define INT_CA1	0x02
#define INT_SR	0x04
#define INT_CB2	0x08
#define INT_CB1	0x10
#define INT_T2	0x20
#define INT_T1	0x40
#define INT_ANY	0x80

#define CLR_PA_INT(v, which)	via_clear_int (which, INT_CA1 | ((!CA2_IND_IRQ(v->pcr)) ? INT_CA2: 0))
#define CLR_PB_INT(v, which)	via_clear_int (which, INT_CB1 | ((!CB2_IND_IRQ(v->pcr)) ? INT_CB2: 0))

#define IFR_DELAY 3

#define TIMER1_VALUE(v) (v->t1ll+(v->t1lh<<8))
#define TIMER2_VALUE(v) (v->t2ll+(v->t2lh<<8))

/******************* static variables *******************/

static struct via6522 via[MAX_VIA];

#if VIA6522_ACCURATE
/* timing accurate core, see the end of this file */
static void acc_reset(int which);
static int acc_read(int which, int offset);
static void acc_write(int which, int offset, int data);
static void acc_set_input_ca1(int which, int data);
static void acc_set_input_ca2(int which, int data);
static void acc_set_input_b(int which, int data);
static void acc_set_input_cb1(int which, int data);
static void acc_set_input_cb2(int which, int data);
#endif

/******************* configuration *******************/

void via_set_clock(int which,double clock)
{
	via[which].sec_to_cycles = clock;
	via[which].cycles_to_sec = 1.0 / via[which].sec_to_cycles;
}

void via_config(int which, const struct via6522_interface *intf)
{
	if (which >= MAX_VIA) return;
	via[which].intf = intf;
	via[which].t1ll = 0xf3; /* via at 0x9110 in vic20 show these values */
	via[which].t1lh = 0xb5; /* ports are not written by kernel! */
	via[which].t2ll = 0xff; /* taken from vice */
	via[which].t2lh = 0xff;
	via[which].time2 = via[which].time1=timer_get_time();

	/* Default clock is from CPU1 */
	via_set_clock (which, Machine->drv->cpu[0].cpu_clock);
}


/******************* external interrupt check *******************/

static void via_set_int (int which, int data)
{
	struct via6522 *v = via + which;


	v->ifr |= data;
#ifdef TRACE_VIA
LOG(("6522VIA chip %d: IFR = %02X.  PC: %08X\n", which, v->ifr, activecpu_get_pc()));
#endif

	if (v->ier & v->ifr)
    {
		v->ifr |= INT_ANY;
		if (v->intf->irq_func)
			(*v->intf->irq_func)(ASSERT_LINE);
		else
			LOG(("6522VIA chip %d: Interrupt is asserted but there is no callback function.  PC: %08X\n", which, activecpu_get_pc()));
    }
}

static void via_clear_int (int which, int data)
{
	struct via6522 *v = via + which;


	v->ifr = (v->ifr & ~data) & 0x7f;
#ifdef TRACE_VIA
LOG(("6522VIA chip %d: IFR = %02X.  PC: %08X\n", which, v->ifr, activecpu_get_pc()));
#endif

	if (v->ifr & v->ier)
		v->ifr |= INT_ANY;
	else
	{
		if (v->intf->irq_func)
			(*v->intf->irq_func)(CLEAR_LINE);
//		else
//			LOG(("6522VIA chip %d: Interrupt is cleared but there is no callback function.  PC: %08X\n", which, activecpu_get_pc()));
	}
}

/******************* Timer timeouts *************************/
static void via_t1_timeout (int which)
{
	struct via6522 *v = via + which;


	if (T1_CONTINUOUS (v->acr))
    {
		if (T1_SET_PB7(v->acr))
			v->out_b ^= 0x80;
		timer_adjust (v->t1, V_CYCLES_TO_TIME(TIMER1_VALUE(v) + IFR_DELAY), which, 0);
    }
	else
    {
		if (T1_SET_PB7(v->acr))
			v->out_b |= 0x80;
		v->t1_active = 0;
		v->time1=timer_get_time();
    }
	if (v->ddr_b)
	{
		UINT8 write_data = v->out_b & v->ddr_b;

		if (v->intf->out_b_func)
			v->intf->out_b_func(0, write_data);
		else
			LOG(("6522VIA chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
	}

	if (!(v->ifr & INT_T1))
		via_set_int (which, INT_T1);
}

static void via_t2_timeout (int which)
{
	struct via6522 *v = via + which;

	if (v->intf->t2_callback)
		v->intf->t2_callback(timer_timeelapsed(v->t2));
	else
		LOG(("6522VIA chip %d: T2 timout occured but there is no callback.  PC: %08X\n", which, activecpu_get_pc()));

	v->t2_active = 0;
	v->time2=timer_get_time();

	if (!(v->ifr & INT_T2))
		via_set_int (which, INT_T2);
}

/******************* reset *******************/

void via_reset(void)
{
	int i;
	struct via6522 v;

	memset(&v, 0, sizeof(v));

	for (i = 0; i < MAX_VIA; i++)
    {
		v.intf = via[i].intf;
		v.t1ll = via[i].t1ll;
		v.t1lh = via[i].t1lh;
		v.t2ll = via[i].t2ll;
		v.t2lh = via[i].t2lh;
		v.time1 = via[i].time1;
		v.time2 = via[i].time2;
		v.sec_to_cycles = via[i].sec_to_cycles;
		v.cycles_to_sec = via[i].cycles_to_sec;

#if VIA6522_ACCURATE
		via[i] = v;
		acc_reset(i);
		continue;
#endif

		v.t1 = timer_alloc(via_t1_timeout);
		v.t1_active = 0;
		v.t2 = timer_alloc(via_t2_timeout);
		v.t2_active = 0;

		via[i] = v;
    }
}

/******************* CPU interface for VIA read *******************/

int via_read(int which, int offset)
{
	struct via6522 *v = via + which;
	int val = 0;

#if VIA6522_ACCURATE
	return acc_read(which, offset);
#endif

	offset &= 0xf;

	switch (offset)
    {
    case VIA_PB:
		/* update the input */
		if (PB_LATCH_ENABLE(v->acr) == 0)
		{
			if (v->intf->in_b_func)
				v->in_b = v->intf->in_b_func(0);
			else
				LOG(("6522VIA chip %d: Port B is being read but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
		}

		CLR_PB_INT(v, which);

		/* combine input and output values, hold DDRB bit 7 high if T1_SET_PB7 */
		if (T1_SET_PB7(v->acr))
			val = (v->out_b & (v->ddr_b | 0x80)) | (v->in_b & ~(v->ddr_b | 0x80));
		else
			val = (v->out_b & v->ddr_b) + (v->in_b & ~v->ddr_b);
		break;

    case VIA_PA:
		/* update the input */
		if (PA_LATCH_ENABLE(v->acr) == 0)
		{
			if (v->intf->in_a_func)
				v->in_a = v->intf->in_a_func(0);
			else
				LOG(("6522VIA chip %d: Port A is being read but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
		}

		/* combine input and output values */
		val = (v->out_a & v->ddr_a) + (v->in_a & ~v->ddr_a);

		CLR_PA_INT(v, which);

		/* If CA2 is configured as output and in pulse or handshake mode,
		   CA2 is set now */
		if (CA2_AUTO_HS(v->pcr))
		{
			if (v->out_ca2)
			{
				/* set CA2 */
				v->out_ca2 = 0;

				/* call the CA2 output function */
				if (v->intf->out_ca2_func)
					v->intf->out_ca2_func(0, 0);
				else
					LOG(("6522VIA chip %d: Port CA2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), 0));
			}
		}

		break;

    case VIA_PANH:
		/* update the input */
		if (PA_LATCH_ENABLE(v->acr) == 0)
		{
			if (v->intf->in_a_func)
				v->in_a = v->intf->in_a_func(0);
			else
				LOG(("6522VIA chip %d: Port A is being read but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
		}

		/* combine input and output values */
		val = (v->out_a & v->ddr_a) + (v->in_a & ~v->ddr_a);
		break;

    case VIA_DDRB:
		val = v->ddr_b;
		break;

    case VIA_DDRA:
		val = v->ddr_a;
		break;

    case VIA_T1CL:
		via_clear_int (which, INT_T1);
		if (v->t1_active)
			val = V_TIME_TO_CYCLES(timer_timeleft(v->t1)) & 0xff;
		else
		{
			if ( T1_CONTINUOUS(v->acr) )
			{
				val = (TIMER1_VALUE(v)-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time1)
						%TIMER1_VALUE(v))-1)&0xff;
			}
			else
			{
				val = (0x10000-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time1)&0xffff)
					   -1)&0xff;
			}
		}
		break;

    case VIA_T1CH:
		if (v->t1_active)
			val = V_TIME_TO_CYCLES(timer_timeleft(v->t1)) >> 8;
		else
		{
			if ( T1_CONTINUOUS(v->acr) )
			{
				val = (TIMER1_VALUE(v)-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time1)
						%TIMER1_VALUE(v))-1)>>8;
			}
			else
			{
				val = (0x10000-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time1)&0xffff)
					   -1)>>8;
			}
		}
		break;

    case VIA_T1LL:
		val = v->t1ll;
		break;

    case VIA_T1LH:
		val = v->t1lh;
		break;

    case VIA_T2CL:
		via_clear_int (which, INT_T2);
		if (v->t2_active)
			val = V_TIME_TO_CYCLES(timer_timeleft(v->t2)) & 0xff;
		else
		{
			if (T2_COUNT_PB6(v->acr))
			{
				val = v->t2cl;
			}
			else
			{
				val = (0x10000-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time2)&0xffff)
					   -1)&0xff;
			}
		}
		break;

    case VIA_T2CH:
		if (v->t2_active)
			val = V_TIME_TO_CYCLES(timer_timeleft(v->t2)) >> 8;
		else
		{
			if (T2_COUNT_PB6(v->acr))
			{
				val = v->t2ch;
			}
			else
			{
				val = (0x10000-
					   (V_TIME_TO_CYCLES(timer_get_time()-v->time2)&0xffff)
					   -1)>>8;
			}
		}
		break;

    case VIA_SR:
		val = v->sr;
		break;

    case VIA_PCR:
		val = v->pcr;
		break;

    case VIA_ACR:
		val = v->acr;
		break;

    case VIA_IER:
		val = v->ier | 0x80;
		break;

    case VIA_IFR:
		val = v->ifr;
		break;
    }
	return val;
}


/******************* CPU interface for VIA write *******************/

void via_write(int which, int offset, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_write(which, offset, data);
	return;
#endif

	offset &=0x0f;

	switch (offset)
    {
    case VIA_PB:
		if (T1_SET_PB7(v->acr))
			v->out_b = (v->out_b & 0x80) | (data  & 0x7f);
		else
			v->out_b = data;

		if (v->ddr_b)
		{
			UINT8 write_data = v->out_b & v->ddr_b;

			if (v->intf->out_b_func)
				v->intf->out_b_func(0, write_data);
			else
				LOG(("6522VIA chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
		}

		CLR_PB_INT(v, which);

		/* If CB2 is configured as output and in pulse or handshake mode,
		   CB2 is set now */
		if (CB2_AUTO_HS(v->pcr))
		{
			if (v->out_cb2)
			{
				/* set CB2 */
				v->out_cb2 = 0;

				/* call the CB2 output function */
				if (v->intf->out_cb2_func)
					v->intf->out_cb2_func(0, 0);
				else
					LOG(("6522VIA chip %d: Port CB2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), 0));
			}
		}
		break;

    case VIA_PA:
		v->out_a = data;

		if (v->ddr_a)
		{
			UINT8 write_data = v->out_a & v->ddr_a;

			if (v->intf->out_a_func)
				v->intf->out_a_func(0, write_data);
			else
				LOG(("6522VIA chip %d: Port A is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
		}

		CLR_PA_INT(v, which);

		/* If CA2 is configured as output and in pulse or handshake mode,
		   CA2 is set now */
		if (CA2_PULSE_OUTPUT(v->pcr))
		{
			/* call the CA2 output function */
			if (v->intf->out_ca2_func)
			{
				v->intf->out_ca2_func(0, 0);
				v->intf->out_ca2_func(0, 1);
			}
			else
			{
				LOG(("6522VIA chip %d: Port CA2 is being pulsed but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
			}

			/* set CA2 (shouldn't be needed) */
			v->out_ca2 = 1;
		}
		else if (CA2_AUTO_HS(v->pcr))
		{
			if (v->out_ca2)
			{
				/* set CA2 */
				v->out_ca2 = 0;

				/* call the CA2 output function */
				if (v->intf->out_ca2_func)
					v->intf->out_ca2_func(0, 0);
				else
					LOG(("6522VIA chip %d: Port CA2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), 0));
			}
		}

		break;

    case VIA_PANH:
		v->out_a = data;

		if (v->ddr_a)
		{
			UINT8 write_data = v->out_a & v->ddr_a;

			if (v->intf->out_a_func)
				v->intf->out_a_func(0, write_data);
			else
				LOG(("6522VIA chip %d: Port A is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
		}

		break;

    case VIA_DDRB:
    	/* EHC 03/04/2000 - If data direction changed, present output on the lines */
    	if ( data != v->ddr_b )
    	{
			v->ddr_b = data;

			if (v->ddr_b)
			{
				UINT8 write_data = v->out_b & v->ddr_b;

				if (v->intf->out_b_func)
					v->intf->out_b_func(0, write_data);
				else
					LOG(("6522VIA chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
			}
		}
		break;

    case VIA_DDRA:
    	/* EHC 03/04/2000 - If data direction changed, present output on the lines */
    	if ( data != v->ddr_a )
    	{
			v->ddr_a = data;

			if (v->ddr_a)
			{
				UINT8 write_data = v->out_a & v->ddr_a;

				if (v->intf->out_a_func)
					v->intf->out_a_func(0, write_data);
				else
					LOG(("6522VIA chip %d: Port A is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
			}
		}
		break;

    case VIA_T1CL:
    case VIA_T1LL:
		v->t1ll = data;
		break;

	case VIA_T1LH:
	    v->t1lh = data;
	    via_clear_int (which, INT_T1);
	    break;

    case VIA_T1CH:
		v->t1ch = v->t1lh = data;
		v->t1cl = v->t1ll;

		via_clear_int (which, INT_T1);

		if (T1_SET_PB7(v->acr))
		{
			v->out_b &= 0x7f;

			if (v->ddr_b)
			{
				UINT8 write_data = v->out_b & v->ddr_b;

				if (v->intf->out_b_func)
					v->intf->out_b_func(0, write_data);
				else
					LOG(("6522VIA chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
			}
		}
		timer_adjust (v->t1, V_CYCLES_TO_TIME(TIMER1_VALUE(v) + IFR_DELAY), which, 0);
		v->t1_active = 1;
		break;

    case VIA_T2CL:
		v->t2ll = data;
		break;

    case VIA_T2CH:
		v->t2ch = v->t2lh = data;
		v->t2cl = v->t2ll;

		via_clear_int (which, INT_T2);

		if (!T2_COUNT_PB6(v->acr))
		{
			if (v->intf->t2_callback)
				v->intf->t2_callback(timer_timeelapsed(v->t2));
			else
				LOG(("6522VIA chip %d: T2 timout occured but there is no callback.  PC: %08X\n", which, activecpu_get_pc()));

			timer_adjust (v->t2, V_CYCLES_TO_TIME(TIMER2_VALUE(v) + IFR_DELAY), which, 0);
			v->t2_active = 1;
		}
		else
		{
			v->time2=timer_get_time();
		}
		break;

    case VIA_SR:
		v->sr = data;
		if (SO_O2_CONTROL(v->acr))
		{
			if (v->intf->out_shift_func)
				v->intf->out_shift_func(data);
			else
				LOG(("6522VIA chip %d: Shift register is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), data));
		}

		/* kludge for Mac Plus (and 128k, 512k, 512ke) : */
		if (SO_EXT_CONTROL(v->acr))
		{
			if (v->intf->out_shift_func2)
			{
				v->intf->out_shift_func2(data);
				via_set_int(which, INT_SR);
			}
			else
				LOG(("6522VIA chip %d: Shift register (Mac Plus) is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), data));
		}
		break;

    case VIA_PCR:
		v->pcr = data;
#ifdef TRACE_VIA
LOG(("6522VIA chip %d: PCR = %02X.  PC: %08X\n", which, data, activecpu_get_pc()));
#endif

		if (CA2_FIX_OUTPUT(data) && CA2_OUTPUT_LEVEL(data) ^ v->out_ca2)
		{
			v->out_ca2 = CA2_OUTPUT_LEVEL(data);
			if (v->intf->out_ca2_func)
				v->intf->out_ca2_func(0, v->out_ca2);
			else
				LOG(("6522VIA chip %d: Port CA2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), v->out_ca2));
		}

		if (CB2_FIX_OUTPUT(data) && CB2_OUTPUT_LEVEL(data) ^ v->out_cb2)
		{
			v->out_cb2 = CB2_OUTPUT_LEVEL(data);
			if (v->intf->out_cb2_func)
				v->intf->out_cb2_func(0, v->out_cb2);
			else
				LOG(("6522VIA chip %d: Port CB2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), v->out_cb2));
		}
		break;

    case VIA_ACR:
		v->acr = data;
		if (T1_SET_PB7(v->acr))
		{
			if (v->t1_active)
				v->out_b &= ~0x80;
			else
				v->out_b |= 0x80;

			if (v->ddr_b)
			{
				UINT8 write_data = v->out_b & v->ddr_b;

				if (v->intf->out_b_func)
					v->intf->out_b_func(0, write_data);
				else
					LOG(("6522VIA chip %d: Port B is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), write_data));
			}
		}
		if (T1_CONTINUOUS(data))
		{
			timer_adjust (v->t1, V_CYCLES_TO_TIME(TIMER1_VALUE(v) + IFR_DELAY), which, 0);
			v->t1_active = 1;
		}
		/* kludge for Mac Plus (and 128k, 512k, 512ke) : */
		if (SI_EXT_CONTROL(data))
		{
			if (v->intf->si_ready_func)
				v->intf->si_ready_func();
			else
				LOG(("6522VIA chip %d: SI READY is being called to but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
		}
		break;

	case VIA_IER:
		if (data & 0x80)
			v->ier |= data & 0x7f;
		else
			v->ier &= ~(data & 0x7f);

		if (v->ifr & INT_ANY)
		{
			if (((v->ifr & v->ier) & 0x7f) == 0)
			{
				v->ifr &= ~INT_ANY;
				if (v->intf->irq_func)
					(*v->intf->irq_func)(CLEAR_LINE);
//				else
//					LOG(("6522VIA chip %d: Interrupt is cleared but there is no callback function.  PC: %08X\n", which, activecpu_get_pc()));
			}
		}
		else
		{
			if ((v->ier & v->ifr) & 0x7f)
			{
				v->ifr |= INT_ANY;
				if (v->intf->irq_func)
					(*v->intf->irq_func)(ASSERT_LINE);
				else
					LOG(("6522VIA chip %d: Interrupt is asserted but there is no callback function.  PC: %08X\n", which, activecpu_get_pc()));
			}
		}
		break;

	case VIA_IFR:
		if (data & INT_ANY)
			data = 0x7f;
		via_clear_int (which, data);
		break;
    }
}

/******************* interface setting VIA port A input *******************/

void via_set_input_a(int which, int data)
{
	struct via6522 *v = via + which;

	/* set the input, what could be easier? */
	v->in_a = data;
}

/******************* interface setting VIA port CA1 input *******************/

void via_set_input_ca1(int which, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_set_input_ca1(which, data);
	return;
#endif

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* handle the active transition */
	if (data != v->in_ca1)
    {
#ifdef TRACE_VIA
LOG(("6522VIA chip %d: CA1 = %02X.  PC: %08X\n", which, data, activecpu_get_pc()));
#endif
		if ((CA1_LOW_TO_HIGH(v->pcr) && data) || (CA1_HIGH_TO_LOW(v->pcr) && !data))
		{
			if (PA_LATCH_ENABLE(v->acr))
			{
				if (v->intf->in_a_func)
					v->in_a = v->intf->in_a_func(0);
				else
					LOG(("6522VIA chip %d: Port A is being read but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
			}

			via_set_int (which, INT_CA1);

			/* CA2 is configured as output and in pulse or handshake mode,
			   CA2 is cleared now */
			if (CA2_AUTO_HS(v->pcr))
			{
				if (!v->out_ca2)
				{
					/* clear CA2 */
					v->out_ca2 = 1;

					/* call the CA2 output function */
					if (v->intf->out_ca2_func)
						v->intf->out_ca2_func(0, 1);
					else
						LOG(("6522VIA chip %d: Port CA2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), 1));
				}
			}
		}

		v->in_ca1 = data;
    }
}

/******************* interface setting VIA port CA2 input *******************/

void via_set_input_ca2(int which, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_set_input_ca2(which, data);
	return;
#endif

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* CA2 is in input mode */
	if (CA2_INPUT(v->pcr))
    {
		/* the new state has caused a transition */
		if (v->in_ca2 != data)
		{
			/* handle the active transition */
			if ((data && CA2_LOW_TO_HIGH(v->pcr)) || (!data && CA2_HIGH_TO_LOW(v->pcr)))
			{
				/* mark the IRQ */
				via_set_int (which, INT_CA2);
			}
			/* set the new value for CA2 */
			v->in_ca2 = data;
		}
    }


}



/******************* interface setting VIA port B input *******************/

void via_set_input_b(int which, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_set_input_b(which, data);
	return;
#endif

	/* set the input, what could be easier? */
	v->in_b = data;
}



/******************* interface setting VIA port CB1 input *******************/

void via_set_input_cb1(int which, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_set_input_cb1(which, data);
	return;
#endif

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* handle the active transition */
	if (data != v->in_cb1)
    {
		if ((CB1_LOW_TO_HIGH(v->pcr) && data) || (CB1_HIGH_TO_LOW(v->pcr) && !data))
		{
			if (PB_LATCH_ENABLE(v->acr))
			{
				if (v->intf->in_b_func)
					v->in_b = v->intf->in_b_func(0);
				else
					LOG(("6522VIA chip %d: Port B is being read but has no handler.  PC: %08X\n", which, activecpu_get_pc()));
			}

			via_set_int (which, INT_CB1);

			/* CB2 is configured as output and in pulse or handshake mode,
			   CB2 is cleared now */
			if (CB2_AUTO_HS(v->pcr))
			{
				if (!v->out_cb2)
				{
					/* clear CB2 */
					v->out_cb2 = 1;

					/* call the CB2 output function */
					if (v->intf->out_cb2_func)
						v->intf->out_cb2_func(0, 1);
					else
						LOG(("6522VIA chip %d: Port CB2 is being written to but has no handler.  PC: %08X - %02X\n", which, activecpu_get_pc(), 1));
				}
			}
		}
		v->in_cb1 = data;
    }
}

/******************* interface setting VIA port CB2 input *******************/

void via_set_input_cb2(int which, int data)
{
	struct via6522 *v = via + which;

#if VIA6522_ACCURATE
	acc_set_input_cb2(which, data);
	return;
#endif

	/* limit the data to 0 or 1 */
	data = data ? 1 : 0;

	/* CB2 is in input mode */
	if (CB2_INPUT(v->pcr))
    {
		/* the new state has caused a transition */
		if (v->in_cb2 != data)
		{
			/* handle the active transition */
			if ((data && CB2_LOW_TO_HIGH(v->pcr)) || (!data && CB2_HIGH_TO_LOW(v->pcr)))
			{
				/* mark the IRQ */
				via_set_int (which, INT_CB2);
			}
			/* set the new value for CB2 */
			v->in_cb2 = data;
		}
    }
}

/******************* interface to shift data into VIA ***********************/

/* kludge for Mac Plus (and 128k, 512k, 512ke) : */

void via_set_input_si(int which, int data)
{
	struct via6522 *v = via + which;

	via_set_int(which, INT_SR);
	v->sr = data;
}

#if VIA6522_ACCURATE
/***************************************************************************

	Timing accurate core

	After MAME's 6522via.cpp (Peter Trauner, Mathis Rosenhauer, timers and
	shift register reworked by Curt Coder), BSD-3-Clause like this file.

***************************************************************************/

/* T2 runs as an 8 bit timer while it clocks the shift register */
#define T2_SHIFTER_CLOCKED(c)	((((c) & 0x0c) == 0x04) || (((c) & 0x1c) == 0x10))

/* elapsed whole VIA clocks since start; the small bias absorbs the error of
   converting the (cycle exact) CPU time to double seconds and back */
INLINE UINT64 acc_clocks_since(const struct via6522 *v, double start)
{
	const double c = (timer_get_time() - start) * v->sec_to_cycles;
	return c > 0. ? (UINT64)(c + 1e-4) : 0;
}

INLINE UINT64 acc_clocks_left(const struct via6522 *v, mame_timer *t)
{
	const double c = timer_timeleft(t) * v->sec_to_cycles;
	return c > 0. ? (UINT64)(c + 1e-4) : 0;
}

/* a timer that will not fire: stopped, or one-shot that already fired */
INLINE int acc_timer_idle(mame_timer *t)
{
	return !timer_enabled(t) || timer_timeleft(t) >= TIME_NEVER / 2;
}

INLINE void acc_timer_stop(mame_timer *t)
{
	timer_adjust(t, TIME_NEVER, 0, 0);
}

static void acc_output_irq(int which)
{
	struct via6522 *v = via + which;

	if (v->ier & v->ifr & 0x7f)
	{
		if ((v->ifr & INT_ANY) == 0)
		{
			v->ifr |= INT_ANY;
			if (v->intf->irq_func)
				(*v->intf->irq_func)(ASSERT_LINE);
		}
	}
	else if (v->ifr & INT_ANY)
	{
		v->ifr &= ~INT_ANY;
		if (v->intf->irq_func)
			(*v->intf->irq_func)(CLEAR_LINE);
	}
}

static void acc_set_int(int which, int data)
{
	struct via6522 *v = via + which;

	if (!(v->ifr & data))
	{
		v->ifr |= data;
		acc_output_irq(which);
	}
}

static void acc_clear_int(int which, int data)
{
	struct via6522 *v = via + which;

	if (v->ifr & data)
	{
		v->ifr &= ~data;
		acc_output_irq(which);
	}
}

#define ACC_CLR_PA_INT(v, which)	acc_clear_int(which, INT_CA1 | ((!CA2_IND_IRQ(v->pcr)) ? INT_CA2: 0))
#define ACC_CLR_PB_INT(v, which)	acc_clear_int(which, INT_CB1 | ((!CB2_IND_IRQ(v->pcr)) ? INT_CB2: 0))

static void acc_out_ca2(struct via6522 *v, int state)
{
	v->out_ca2 = state;
	if (v->intf->out_ca2_func)
		v->intf->out_ca2_func(0, state);
}

static void acc_out_cb2(struct via6522 *v, int state)
{
	v->out_cb2 = state;
	if (v->intf->out_cb2_func)
		v->intf->out_cb2_func(0, state);
}

/******************* ports *******************/

static UINT8 acc_input_pa(struct via6522 *v)
{
	if (v->intf->in_a_func)
		v->in_a = v->intf->in_a_func(0);
	return (v->out_a & v->ddr_a) | (v->in_a & ~v->ddr_a);
}

static UINT8 acc_input_pb(struct via6522 *v)
{
	UINT8 pb;

	if (v->intf->in_b_func)
		v->in_b = v->intf->in_b_func(0);
	pb = (v->out_b & v->ddr_b) | (v->in_b & ~v->ddr_b);
	if (T1_SET_PB7(v->acr))
		pb = (pb & 0x7f) | (v->t1_pb7 << 7);
	return pb;
}

static void acc_output_pa(struct via6522 *v)
{
	if (v->intf->out_a_func)
		v->intf->out_a_func(0, v->out_a & v->ddr_a);
}

static void acc_output_pb(struct via6522 *v)
{
	UINT8 pb = v->out_b & v->ddr_b;

	if (T1_SET_PB7(v->acr))
		pb = (pb & 0x7f) | (v->t1_pb7 << 7);
	if (v->intf->out_b_func)
		v->intf->out_b_func(0, pb);
}

/******************* timers *******************/

static UINT16 acc_get_counter1_value(int which)
{
	struct via6522 *v = via + which;
	const UINT32 period = TIMER1_VALUE(v) + 2;
	UINT64 elapsed;
	INT64 e, c;
	UINT32 phase;

	/* reloads in one-shot mode too: latch .. 1, 0, $ffff, latch
	   hold at 0 until the timeout has set the interrupt flag */
	if (v->t1_active && timer_firetime(v->t1) <= timer_get_time())
		return 0;

	elapsed = acc_clocks_since(v, v->time1);
	e = elapsed ? (INT64)(elapsed - 1) : 0;
	c = (v->t1_value == 0xffff) ? -1 : (INT64)v->t1_value;

	if (e <= c)
		return (UINT16)(c - e);
	if (e == c + 1)
		return 0xffff;

	phase = (UINT32)((UINT64)(e - c - 2) % period);
	return (phase <= (UINT32)TIMER1_VALUE(v)) ? (UINT16)(TIMER1_VALUE(v) - phase) : 0xffff;
}

/* restart the counter computation from its current value, so a new latch
   only takes effect at the next reload */
static void acc_reanchor_counter1(int which)
{
	struct via6522 *v = via + which;
	const int underflow_due = v->t1_active && (timer_firetime(v->t1) <= timer_get_time());
	const UINT16 value = underflow_due ? 0xffff : acc_get_counter1_value(which);

	v->time1 = timer_get_time() - V_CYCLES_TO_TIME(1);
	v->t1_value = value;
}

static UINT32 acc_t2_underflow_delay(const struct via6522 *v)
{
	if (!T2_SHIFTER_CLOCKED(v->acr))
		return TIMER2_VALUE(v) + 2;

	return (v->t2_start & 0xff) + (v->t2_start >> 8) * (v->t2ll + 2) + 2;
}

static UINT16 acc_get_counter2_value(int which)
{
	struct via6522 *v = via + which;

	if (T2_COUNT_PB6(v->acr))
		return v->t2cl | (v->t2ch << 8);

	/* 8 bit mode: the low byte reloads from T2LL via $ff, the high byte borrows */
	if (T2_SHIFTER_CLOCKED(v->acr))
	{
		const UINT32 lo0 = v->t2_start & 0xff;
		const UINT32 hi0 = v->t2_start >> 8;
		const UINT64 elapsed = acc_clocks_since(v, v->t2_load);
		const UINT64 t = elapsed ? elapsed - 1 : 0;
		UINT32 period, hi, r;
		UINT64 since;

		if (t <= lo0)
			return (UINT16)((hi0 << 8) | (UINT32)(lo0 - t));

		period = v->t2ll + 2;
		since = t - (lo0 + 1);
		hi = (hi0 - 1 - (UINT32)(since / period)) & 0xff;
		r = (UINT32)(since % period);

		return (UINT16)((hi << 8) | (r ? ((v->t2ll - (r - 1)) & 0xff) : 0xff));
	}

	if (v->t2_active && !acc_timer_idle(v->t2))
	{
		/* hold at 0 until the timeout sets IFR */
		const UINT64 remaining = acc_clocks_left(v, v->t2);
		return remaining ? (UINT16)(remaining - 1) : 0;
	}

	return (UINT16)(0x10000 - (acc_clocks_since(v, v->time2) & 0xffff) - 1);
}

static void acc_counter2_decrement(int which)
{
	struct via6522 *v = via + which;

	if (!T2_COUNT_PB6(v->acr))
		return;

	/* count down on T2CL */
	if (v->t2cl-- != 0)
		return;

	/* borrow from T2CH */
	if (v->t2ch-- != 0)
		return;

	/* underflow causes only one interrupt between T2CH writes */
	if (v->t2_active)
	{
		v->t2_active = 0;
		acc_set_int(which, INT_T2);
	}
}

static void acc_t1_tick(int which)
{
	struct via6522 *v = via + which;

	/* PB7 toggles on timeout in one-shot mode too, only while PB7 output is enabled */
	if (T1_CONTINUOUS(v->acr))
	{
		if (TIMER1_VALUE(v) > 0 && T1_SET_PB7(v->acr))
			v->t1_pb7 = !v->t1_pb7;
		timer_adjust(v->t1, V_CYCLES_TO_TIME(TIMER1_VALUE(v) + 2), which, 0);
	}
	else
	{
		if (T1_SET_PB7(v->acr))
			v->t1_pb7 = !v->t1_pb7;
		v->t1_active = 0;
	}

	if (T1_SET_PB7(v->acr))
		acc_output_pb(v);

	acc_set_int(which, INT_T1);
}

static void acc_t2_tick(int which)
{
	struct via6522 *v = via + which;

	v->t2_active = 0;
	v->time2 = timer_get_time();

	if (v->intf->t2_callback)
		v->intf->t2_callback(timer_timeelapsed(v->t2));

	acc_set_int(which, INT_T2);
}

static void acc_ca2_tick(int which)
{
	acc_out_ca2(via + which, 1);
}

static void acc_cb2_tick(int which)
{
	acc_out_cb2(via + which, 1);
}

/******************* shift register *******************/

static int acc_shift_blocked(const struct via6522 *v)
{
	return v->shift_done && !(SO_T2_RATE(v->acr) || SI_EXT_CONTROL(v->acr) || SO_EXT_CONTROL(v->acr));
}

static int acc_shift_clock_level(const struct via6522 *v)
{
	return (SI_EXT_CONTROL(v->acr) || SO_EXT_CONTROL(v->acr) || SR_DISABLED(v->acr)) ? v->in_cb1 : v->out_cb1;
}

static void acc_shift_out(int which)
{
	struct via6522 *v = via + which;

	/* only shift out msb on falling edge */
	if (!acc_shift_clock_level(v))
	{
		const int bit = (v->sr >> 7) & 1;
		v->sr = (v->sr << 1) | bit;
		v->shift_counter = (v->shift_counter - 1) & 7;

		acc_out_cb2(v, bit);

		if (v->shift_counter == 0 && SO_EXT_CONTROL(v->acr))
		{
			/* IRQ on last falling edge for external clock (mode 7) */
			v->shift_done = 1;
			acc_set_int(which, INT_SR);
		}
	}
	else if (!SO_T2_RATE(v->acr)) /* the T2 free running shifter doesn't do interrupts (mode 4) */
	{
		/* IRQ on last rising edge for internal clock (modes 5-6) */
		if (v->shift_counter == 0 && (SO_O2_CONTROL(v->acr) || SO_T2_CONTROL(v->acr)))
		{
			v->shift_done = 1;
			acc_set_int(which, INT_SR);
		}
	}
}

static void acc_shift_in(int which)
{
	struct via6522 *v = via + which;

	/* only shift in data on rising edge */
	if (acc_shift_clock_level(v))
	{
		v->sr = (v->sr << 1) | (v->in_cb2 & 1);
		v->shift_counter = (v->shift_counter - 1) & 7;

		if (v->shift_counter == 0 && !SR_DISABLED(v->acr))
		{
			v->shift_done = 1;
			if (SI_EXT_CONTROL(v->acr))
				acc_set_int(which, INT_SR);
			else /* delay the IRQ 2 edges for internal shift in (modes 1-2) */
				timer_adjust(v->shift_irq_timer, V_CYCLES_TO_TIME(1), which, 0);
		}
	}
}

static void acc_shift_irq_tick(int which)
{
	acc_set_int(which, INT_SR);
}

static void acc_shift_tick(int which)
{
	struct via6522 *v = via + which;

	/* CB1 parks once the eight bits are done while T2 keeps running */
	if (!acc_shift_blocked(v))
	{
		v->out_cb1 ^= 1;

		if (SO_T2_RATE(v->acr) || SO_T2_CONTROL(v->acr) || SO_O2_CONTROL(v->acr))
			acc_shift_out(which);
		else if (SI_T2_CONTROL(v->acr) || SI_O2_CONTROL(v->acr))
			acc_shift_in(which);
	}

	if (SO_T2_RATE(v->acr) || SO_T2_CONTROL(v->acr) || SI_T2_CONTROL(v->acr))
		timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(v->t2ll + 2), which, 0);
	else if ((SI_O2_CONTROL(v->acr) || SO_O2_CONTROL(v->acr)) && !v->shift_done)
		timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(1), which, 0);
	else
		acc_timer_stop(v->shift_timer);
}

/******************* reset *******************/

static void acc_reset(int which)
{
	struct via6522 *v = via + which;

	v->t1 = timer_alloc(acc_t1_tick);
	v->t2 = timer_alloc(acc_t2_tick);
	v->ca2_timer = timer_alloc(acc_ca2_tick);
	v->cb2_timer = timer_alloc(acc_cb2_tick);
	v->shift_timer = timer_alloc(acc_shift_tick);
	v->shift_irq_timer = timer_alloc(acc_shift_irq_tick);

	/* initial counter state unknown, but definitely not zero */
	v->t1cl = v->t1ch = v->t2cl = v->t2ch = 0xff;
	v->t1_value = 0;
	v->t2_start = 0xffff;
	v->t2_load = v->time2;

	/* the lines idle high; unlike MAME the handlers are not called, so a
	   driver sees no output edge at reset (like with the old core) */
	v->out_ca2 = 1;
	v->out_cb1 = 1;
	v->out_cb2 = 1;
	v->t1_pb7 = 1;
	v->shift_done = 1;
}

/******************* CPU interface *******************/

static int acc_read(int which, int offset)
{
	struct via6522 *v = via + which;
	int val = 0;

	offset &= 0xf;

	switch (offset)
	{
	case VIA_PB:
		if (PB_LATCH_ENABLE(v->acr) && (v->ifr & INT_CB1))
			val = v->latch_b;
		else
			val = acc_input_pb(v);

		ACC_CLR_PB_INT(v, which);
		break;

	case VIA_PA:
		if (PA_LATCH_ENABLE(v->acr) && (v->ifr & INT_CA1))
			val = v->latch_a;
		else
			val = acc_input_pa(v);

		ACC_CLR_PA_INT(v, which);

		if (v->out_ca2 && CA2_AUTO_HS(v->pcr))
			acc_out_ca2(v, 0);
		if (CA2_PULSE_OUTPUT(v->pcr))
			timer_adjust(v->ca2_timer, V_CYCLES_TO_TIME(1), which, 0);
		break;

	case VIA_PANH:
		if (PA_LATCH_ENABLE(v->acr) && (v->ifr & INT_CA1))
			val = v->latch_a;
		else
			val = acc_input_pa(v);
		break;

	case VIA_DDRB:
		val = v->ddr_b;
		break;

	case VIA_DDRA:
		val = v->ddr_a;
		break;

	case VIA_T1CL:
		acc_clear_int(which, INT_T1);
		val = acc_get_counter1_value(which) & 0xff;
		break;

	case VIA_T1CH:
		val = acc_get_counter1_value(which) >> 8;
		break;

	case VIA_T1LL:
		val = v->t1ll;
		break;

	case VIA_T1LH:
		val = v->t1lh;
		break;

	case VIA_T2CL:
		acc_clear_int(which, INT_T2);
		val = acc_get_counter2_value(which) & 0xff;
		break;

	case VIA_T2CH:
		val = acc_get_counter2_value(which) >> 8;
		break;

	case VIA_SR:
		val = v->sr;
		/* an access mid-shift neither restarts the count nor moves the clock */
		if (!(SI_EXT_CONTROL(v->acr) || SO_EXT_CONTROL(v->acr)))
		{
			if ((v->ifr & INT_SR) || (v->shift_done && !SR_DISABLED(v->acr)))
			{
				v->shift_counter = 8;
				v->shift_done = 0;
			}
		}
		else
		{
			v->shift_counter = 8;
			v->shift_done = 0;
		}

		acc_clear_int(which, INT_SR);
		if (SI_O2_CONTROL(v->acr) || SO_O2_CONTROL(v->acr))
		{
			if (acc_timer_idle(v->shift_timer))
				timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(2), which, 0);
		}
		else if (SI_T2_CONTROL(v->acr) || SO_T2_CONTROL(v->acr))
		{
			if (acc_timer_idle(v->shift_timer))
				timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(v->t2ll + 2), which, 0);
		}
		else if (!SO_T2_RATE(v->acr))
			acc_timer_stop(v->shift_timer);
		break;

	case VIA_PCR:
		val = v->pcr;
		break;

	case VIA_ACR:
		val = v->acr;
		break;

	case VIA_IER:
		val = v->ier | 0x80;
		break;

	case VIA_IFR:
		val = v->ifr;
		break;
	}
	return val;
}

static void acc_write(int which, int offset, int data)
{
	struct via6522 *v = via + which;

	offset &= 0x0f;

	switch (offset)
	{
	case VIA_PB:
		v->out_b = data;

		if (v->ddr_b)
			acc_output_pb(v);

		ACC_CLR_PB_INT(v, which);

		if (v->out_cb2 && CB2_AUTO_HS(v->pcr))
			acc_out_cb2(v, 0);
		if (CB2_PULSE_OUTPUT(v->pcr))
			timer_adjust(v->cb2_timer, V_CYCLES_TO_TIME(1), which, 0);
		break;

	case VIA_PA:
		v->out_a = data;

		if (v->ddr_a)
			acc_output_pa(v);

		ACC_CLR_PA_INT(v, which);

		if (v->out_ca2 && CA2_AUTO_HS(v->pcr))
			acc_out_ca2(v, 0);
		if (CA2_PULSE_OUTPUT(v->pcr))
			timer_adjust(v->ca2_timer, V_CYCLES_TO_TIME(1), which, 0);
		break;

	case VIA_PANH:
		v->out_a = data;

		if (v->ddr_a)
			acc_output_pa(v);
		break;

	case VIA_DDRB:
		if (data != v->ddr_b)
		{
			v->ddr_b = data;
			acc_output_pb(v);
		}
		break;

	case VIA_DDRA:
		if (data != v->ddr_a)
		{
			v->ddr_a = data;
			acc_output_pa(v);
		}
		break;

	case VIA_T1CL:
	case VIA_T1LL:
		acc_reanchor_counter1(which);
		v->t1ll = data;
		break;

	case VIA_T1LH:
		acc_reanchor_counter1(which);
		v->t1lh = data;
		acc_clear_int(which, INT_T1);
		break;

	case VIA_T1CH:
		v->t1ch = v->t1lh = data;
		v->t1cl = v->t1ll;

		acc_clear_int(which, INT_T1);

		v->t1_pb7 = T1_SET_PB7(v->acr) ? 0 : 1;
		if (T1_SET_PB7(v->acr))
			acc_output_pb(v);

		v->time1 = timer_get_time();
		v->t1_value = TIMER1_VALUE(v);
		timer_adjust(v->t1, V_CYCLES_TO_TIME(TIMER1_VALUE(v) + 2), which, 0);
		v->t1_active = 1;
		break;

	case VIA_T2CL:
		v->t2ll = data;
		break;

	case VIA_T2CH:
		v->t2ch = v->t2lh = data;
		v->t2cl = v->t2ll;

		acc_clear_int(which, INT_T2);

		if (!T2_COUNT_PB6(v->acr))
		{
			v->t2_start = TIMER2_VALUE(v);
			v->t2_load = timer_get_time();
			timer_adjust(v->t2, V_CYCLES_TO_TIME(acc_t2_underflow_delay(v)), which, 0);
		}
		else
		{
			acc_timer_stop(v->t2);
			v->time2 = timer_get_time();
		}
		v->t2_active = 1;
		break;

	case VIA_SR:
		v->sr = data;

		v->shift_counter = 8;
		v->shift_done = SR_DISABLED(v->acr) ? 1 : 0;

		acc_clear_int(which, INT_SR);
		if (SO_O2_CONTROL(v->acr) || SI_O2_CONTROL(v->acr))
		{
			if (acc_timer_idle(v->shift_timer))
				timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(2), which, 0);
		}
		else if (SO_T2_RATE(v->acr) || SO_T2_CONTROL(v->acr) || SI_T2_CONTROL(v->acr))
		{
			if (acc_timer_idle(v->shift_timer))
				timer_adjust(v->shift_timer, V_CYCLES_TO_TIME(v->t2ll + 2), which, 0);
		}
		else /* in case the mode changes before the counter expires */
			acc_timer_stop(v->shift_timer);

		/* kludge for Mac Plus (and 128k, 512k, 512ke) */
		if (SO_EXT_CONTROL(v->acr) && v->intf->out_shift_func2)
			v->intf->out_shift_func2(data);
		break;

	case VIA_PCR:
		v->pcr = data;

		if (CA2_FIX_OUTPUT(data) && v->out_ca2 != CA2_OUTPUT_LEVEL(data))
			acc_out_ca2(v, CA2_OUTPUT_LEVEL(data));

		if (CB2_FIX_OUTPUT(data) && v->out_cb2 != CB2_OUTPUT_LEVEL(data))
			acc_out_cb2(v, CB2_OUTPUT_LEVEL(data));
		break;

	case VIA_ACR:
		{
			UINT16 counter2 = acc_get_counter2_value(which);
			const int t2_was_pb6 = T2_COUNT_PB6(v->acr) ? 1 : 0;
			const int pb7_changed = (v->acr ^ data) & 0x80;

			v->acr = data;

			if (pb7_changed)
				acc_output_pb(v);

			if (SR_DISABLED(v->acr) || SI_EXT_CONTROL(v->acr) || SO_EXT_CONTROL(v->acr))
			{
				acc_timer_stop(v->shift_timer);
				if (SR_DISABLED(v->acr) && !v->out_cb1)
					v->out_cb1 = 1;
			}
			else if ((SO_T2_RATE(v->acr) || SO_T2_CONTROL(v->acr) || SI_T2_CONTROL(v->acr))
					&& acc_timer_idle(v->shift_timer))
			{
				/* CB1 edges follow the low byte's underflows, starting one period late */
				timer_adjust(v->shift_timer, V_CYCLES_TO_TIME((counter2 & 0xff) + 3), which, 0);
			}

			/* the counter carries its value across a clock source change */
			if ((T2_COUNT_PB6(v->acr) ? 1 : 0) != t2_was_pb6)
			{
				if (T2_COUNT_PB6(v->acr))
				{
					counter2 = (counter2 - 1) & 0xffff;
					v->t2cl = counter2 & 0xff;
					v->t2ch = counter2 >> 8;
					acc_timer_stop(v->t2);
				}
				else if (v->t2_active)
				{
					/* the new clock source takes effect one cycle after the write */
					timer_adjust(v->t2, V_CYCLES_TO_TIME(counter2 + 2), which, 0);
				}
				else
					v->time2 = timer_get_time() - V_CYCLES_TO_TIME(0xfffe - counter2);
			}

			if (SI_T2_CONTROL(v->acr) || SI_O2_CONTROL(v->acr) || SI_EXT_CONTROL(v->acr))
				acc_out_cb2(v, 1);

			/* kludge for Mac Plus (and 128k, 512k, 512ke) */
			if (SI_EXT_CONTROL(data) && v->intf->si_ready_func)
				v->intf->si_ready_func();
		}
		break;

	case VIA_IER:
		if (data & 0x80)
			v->ier |= data & 0x7f;
		else
			v->ier &= ~(data & 0x7f);

		acc_output_irq(which);
		break;

	case VIA_IFR:
		acc_clear_int(which, data & 0x7f);
		break;
	}
}

/******************* inputs *******************/

static void acc_set_input_ca1(int which, int data)
{
	struct via6522 *v = via + which;

	data = data ? 1 : 0;
	if (data != v->in_ca1)
	{
		v->in_ca1 = data;

		if ((data && CA1_LOW_TO_HIGH(v->pcr)) || (!data && CA1_HIGH_TO_LOW(v->pcr)))
		{
			if (PA_LATCH_ENABLE(v->acr))
				v->latch_a = acc_input_pa(v);

			acc_set_int(which, INT_CA1);

			if (!v->out_ca2 && CA2_AUTO_HS(v->pcr))
				acc_out_ca2(v, 1);
		}
	}
}

static void acc_set_input_ca2(int which, int data)
{
	struct via6522 *v = via + which;

	data = data ? 1 : 0;
	if (data != v->in_ca2)
	{
		v->in_ca2 = data;

		if (CA2_INPUT(v->pcr))
		{
			if ((data && CA2_LOW_TO_HIGH(v->pcr)) || (!data && CA2_HIGH_TO_LOW(v->pcr)))
				acc_set_int(which, INT_CA2);
		}
	}
}

static void acc_set_input_b(int which, int data)
{
	struct via6522 *v = via + which;

	/* T2 counts falling edges on PB6 */
	if (!(data & 0x40) && (v->in_b & 0x40))
		acc_counter2_decrement(which);

	v->in_b = data;
}

static void acc_set_input_cb1(int which, int data)
{
	struct via6522 *v = via + which;

	data = data ? 1 : 0;
	if (data != v->in_cb1)
	{
		v->in_cb1 = data;

		if ((data && CB1_LOW_TO_HIGH(v->pcr)) || (!data && CB1_HIGH_TO_LOW(v->pcr)))
		{
			if (PB_LATCH_ENABLE(v->acr))
				v->latch_b = acc_input_pb(v);

			acc_set_int(which, INT_CB1);

			if (!v->out_cb2 && CB2_AUTO_HS(v->pcr))
				acc_out_cb2(v, 1);
		}

		/* the shifter is not controlled by PCR */
		if (SO_EXT_CONTROL(v->acr))
			acc_shift_out(which);
		else if (SI_EXT_CONTROL(v->acr) || SR_DISABLED(v->acr))
			acc_shift_in(which);
	}
}

static void acc_set_input_cb2(int which, int data)
{
	struct via6522 *v = via + which;

	data = data ? 1 : 0;
	if (data != v->in_cb2)
	{
		v->in_cb2 = data;

		if (CB2_INPUT(v->pcr))
		{
			if ((data && CB2_LOW_TO_HIGH(v->pcr)) || (!data && CB2_HIGH_TO_LOW(v->pcr)))
				acc_set_int(which, INT_CB2);
		}
	}
}
#endif /* VIA6522_ACCURATE */

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ_HANDLER( via_0_r) { return via_read(0, offset); }
READ_HANDLER( via_1_r) { return via_read(1, offset); }
READ_HANDLER( via_2_r) { return via_read(2, offset); }
READ_HANDLER( via_3_r) { return via_read(3, offset); }
READ_HANDLER( via_4_r) { return via_read(4, offset); }
READ_HANDLER( via_5_r) { return via_read(5, offset); }
READ_HANDLER( via_6_r) { return via_read(6, offset); }
READ_HANDLER( via_7_r) { return via_read(7, offset); }

WRITE_HANDLER( via_0_w) { via_write(0, offset, data); }
WRITE_HANDLER( via_1_w) { via_write(1, offset, data); }
WRITE_HANDLER( via_2_w) { via_write(2, offset, data); }
WRITE_HANDLER( via_3_w) { via_write(3, offset, data); }
WRITE_HANDLER( via_4_w) { via_write(4, offset, data); }
WRITE_HANDLER( via_5_w) { via_write(5, offset, data); }
WRITE_HANDLER( via_6_w) { via_write(6, offset, data); }
WRITE_HANDLER( via_7_w) { via_write(7, offset, data); }

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER( via_0_porta_w) { via_set_input_a(0, data); }
WRITE_HANDLER( via_1_porta_w) { via_set_input_a(1, data); }
WRITE_HANDLER( via_2_porta_w) { via_set_input_a(2, data); }
WRITE_HANDLER( via_3_porta_w) { via_set_input_a(3, data); }
WRITE_HANDLER( via_4_porta_w) { via_set_input_a(4, data); }
WRITE_HANDLER( via_5_porta_w) { via_set_input_a(5, data); }
WRITE_HANDLER( via_6_porta_w) { via_set_input_a(6, data); }
WRITE_HANDLER( via_7_porta_w) { via_set_input_a(7, data); }

WRITE_HANDLER( via_0_portb_w) { via_set_input_b(0, data); }
WRITE_HANDLER( via_1_portb_w) { via_set_input_b(1, data); }
WRITE_HANDLER( via_2_portb_w) { via_set_input_b(2, data); }
WRITE_HANDLER( via_3_portb_w) { via_set_input_b(3, data); }
WRITE_HANDLER( via_4_portb_w) { via_set_input_b(4, data); }
WRITE_HANDLER( via_5_portb_w) { via_set_input_b(5, data); }
WRITE_HANDLER( via_6_portb_w) { via_set_input_b(6, data); }
WRITE_HANDLER( via_7_portb_w) { via_set_input_b(7, data); }

READ_HANDLER( via_0_porta_r) { return via[0].in_a; }
READ_HANDLER( via_1_porta_r) { return via[1].in_a; }
READ_HANDLER( via_2_porta_r) { return via[2].in_a; }
READ_HANDLER( via_3_porta_r) { return via[3].in_a; }
READ_HANDLER( via_4_porta_r) { return via[4].in_a; }
READ_HANDLER( via_5_porta_r) { return via[5].in_a; }
READ_HANDLER( via_6_porta_r) { return via[6].in_a; }
READ_HANDLER( via_7_porta_r) { return via[7].in_a; }

READ_HANDLER( via_0_portb_r) { return via[0].in_b; }
READ_HANDLER( via_1_portb_r) { return via[1].in_b; }
READ_HANDLER( via_2_portb_r) { return via[2].in_b; }
READ_HANDLER( via_3_portb_r) { return via[3].in_b; }
READ_HANDLER( via_4_portb_r) { return via[4].in_b; }
READ_HANDLER( via_5_portb_r) { return via[5].in_b; }
READ_HANDLER( via_6_portb_r) { return via[6].in_b; }
READ_HANDLER( via_7_portb_r) { return via[7].in_b; }

/******************* 1-bit CA1/CA2/CB1/CB2 port interfaces *******************/

WRITE_HANDLER( via_0_ca1_w) { via_set_input_ca1(0, data); }
WRITE_HANDLER( via_1_ca1_w) { via_set_input_ca1(1, data); }
WRITE_HANDLER( via_2_ca1_w) { via_set_input_ca1(2, data); }
WRITE_HANDLER( via_3_ca1_w) { via_set_input_ca1(3, data); }
WRITE_HANDLER( via_4_ca1_w) { via_set_input_ca1(4, data); }
WRITE_HANDLER( via_5_ca1_w) { via_set_input_ca1(5, data); }
WRITE_HANDLER( via_6_ca1_w) { via_set_input_ca1(6, data); }
WRITE_HANDLER( via_7_ca1_w) { via_set_input_ca1(7, data); }
WRITE_HANDLER( via_0_ca2_w) { via_set_input_ca2(0, data); }
WRITE_HANDLER( via_1_ca2_w) { via_set_input_ca2(1, data); }
WRITE_HANDLER( via_2_ca2_w) { via_set_input_ca2(2, data); }
WRITE_HANDLER( via_3_ca2_w) { via_set_input_ca2(3, data); }
WRITE_HANDLER( via_4_ca2_w) { via_set_input_ca2(4, data); }
WRITE_HANDLER( via_5_ca2_w) { via_set_input_ca2(5, data); }
WRITE_HANDLER( via_6_ca2_w) { via_set_input_ca2(6, data); }
WRITE_HANDLER( via_7_ca2_w) { via_set_input_ca2(7, data); }

WRITE_HANDLER( via_0_cb1_w) { via_set_input_cb1(0, data); }
WRITE_HANDLER( via_1_cb1_w) { via_set_input_cb1(1, data); }
WRITE_HANDLER( via_2_cb1_w) { via_set_input_cb1(2, data); }
WRITE_HANDLER( via_3_cb1_w) { via_set_input_cb1(3, data); }
WRITE_HANDLER( via_4_cb1_w) { via_set_input_cb1(4, data); }
WRITE_HANDLER( via_5_cb1_w) { via_set_input_cb1(5, data); }
WRITE_HANDLER( via_6_cb1_w) { via_set_input_cb1(6, data); }
WRITE_HANDLER( via_7_cb1_w) { via_set_input_cb1(7, data); }
WRITE_HANDLER( via_0_cb2_w) { via_set_input_cb2(0, data); }
WRITE_HANDLER( via_1_cb2_w) { via_set_input_cb2(1, data); }
WRITE_HANDLER( via_2_cb2_w) { via_set_input_cb2(2, data); }
WRITE_HANDLER( via_3_cb2_w) { via_set_input_cb2(3, data); }
WRITE_HANDLER( via_4_cb2_w) { via_set_input_cb2(4, data); }
WRITE_HANDLER( via_5_cb2_w) { via_set_input_cb2(5, data); }
WRITE_HANDLER( via_6_cb2_w) { via_set_input_cb2(6, data); }
WRITE_HANDLER( via_7_cb2_w) { via_set_input_cb2(7, data); }

READ_HANDLER( via_0_ca1_r) { return via[0].in_ca1; }
READ_HANDLER( via_1_ca1_r) { return via[1].in_ca1; }
READ_HANDLER( via_2_ca1_r) { return via[2].in_ca1; }
READ_HANDLER( via_3_ca1_r) { return via[3].in_ca1; }
READ_HANDLER( via_4_ca1_r) { return via[4].in_ca1; }
READ_HANDLER( via_5_ca1_r) { return via[5].in_ca1; }
READ_HANDLER( via_6_ca1_r) { return via[6].in_ca1; }
READ_HANDLER( via_7_ca1_r) { return via[7].in_ca1; }
READ_HANDLER( via_0_ca2_r) { return via[0].in_ca2; }
READ_HANDLER( via_1_ca2_r) { return via[1].in_ca2; }
READ_HANDLER( via_2_ca2_r) { return via[2].in_ca2; }
READ_HANDLER( via_3_ca2_r) { return via[3].in_ca2; }
READ_HANDLER( via_4_ca2_r) { return via[4].in_ca2; }
READ_HANDLER( via_5_ca2_r) { return via[5].in_ca2; }
READ_HANDLER( via_6_ca2_r) { return via[6].in_ca2; }
READ_HANDLER( via_7_ca2_r) { return via[7].in_ca2; }

READ_HANDLER( via_0_cb1_r) { return via[0].in_cb1; }
READ_HANDLER( via_1_cb1_r) { return via[1].in_cb1; }
READ_HANDLER( via_2_cb1_r) { return via[2].in_cb1; }
READ_HANDLER( via_3_cb1_r) { return via[3].in_cb1; }
READ_HANDLER( via_4_cb1_r) { return via[4].in_cb1; }
READ_HANDLER( via_5_cb1_r) { return via[5].in_cb1; }
READ_HANDLER( via_6_cb1_r) { return via[6].in_cb1; }
READ_HANDLER( via_7_cb1_r) { return via[7].in_cb1; }
READ_HANDLER( via_0_cb2_r) { return via[0].in_cb2; }
READ_HANDLER( via_1_cb2_r) { return via[1].in_cb2; }
READ_HANDLER( via_2_cb2_r) { return via[2].in_cb2; }
READ_HANDLER( via_3_cb2_r) { return via[3].in_cb2; }
READ_HANDLER( via_4_cb2_r) { return via[4].in_cb2; }
READ_HANDLER( via_5_cb2_r) { return via[5].in_cb2; }
READ_HANDLER( via_6_cb2_r) { return via[6].in_cb2; }
READ_HANDLER( via_7_cb2_r) { return via[7].in_cb2; }


#undef TRACE_VIA
