/**********************************************************************

	6532 RIOT interface and emulation

	This function emulates all the functionality of up to 8 6532 RIOT
	peripheral interface adapters.

**********************************************************************/

#include <string.h>
#include <stdio.h>
#include "driver.h"
#include "6532riot.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif


/******************* internal RIOT data structure *******************/

#define RIOT6532_PORTA	0x00
#define RIOT6532_DDRA	0x01
#define RIOT6532_PORTB	0x02
#define RIOT6532_DDRB	0x03

#define RIOT6532_TIMER	0x00
#define RIOT6532_IRF	0x01

struct riot6532
{
	const struct riot6532_interface *intf;
	UINT8 inUse;

	UINT8 in_a;
	UINT8 out_a;
	UINT8 ddr_a;

	UINT8 in_b;
	UINT8 out_b;
	UINT8 ddr_b;

	UINT8 timer_start;
	UINT16 timer_divider;
	UINT8 timer_irq_enabled;

	UINT8 edge_detect_ctrl;

	UINT8 irq_state;
	UINT8 irq;

	void *t;
	double time;

	double cycles_to_sec;
	double sec_to_cycles;
};

#define V_CYCLES_TO_TIME(c) ((double)(c) * p->cycles_to_sec)
#define V_TIME_TO_CYCLES(t) ((int)((t) * p->sec_to_cycles))


/******************* convenince macros and defines *******************/

#define RIOT_TIMERIRQ			0x80
#define RIOT_PA7IRQ				0x40

#define IRQ1_ENABLED(c)			(c & 0x01)
#define IRQ1_DISABLED(c)		(!(c & 0x01))
#define C1_LOW_TO_HIGH(c)		(c & 0x02)
#define C1_HIGH_TO_LOW(c)		(!(c & 0x02))
#define OUTPUT_SELECTED(c)		(c & 0x04)
#define DDR_SELECTED(c)			(!(c & 0x04))
#define IRQ2_ENABLED(c)			(c & 0x08)
#define IRQ2_DISABLED(c)		(!(c & 0x08))
#define STROBE_E_RESET(c)		(c & 0x08)
#define STROBE_C1_RESET(c)		(!(c & 0x08))
#define SET_C2(c)				(c & 0x08)
#define RESET_C2(c)				(!(c & 0x08))
#define C2_LOW_TO_HIGH(c)		(c & 0x10)
#define C2_HIGH_TO_LOW(c)		(!(c & 0x10))
#define C2_SET_MODE(c)			(c & 0x10)
#define C2_STROBE_MODE(c)		(!(c & 0x10))
#define C2_OUTPUT(c)			(c & 0x20)
#define C2_INPUT(c)				(!(c & 0x20))

#define IFR_DELAY 3

/******************* static variables *******************/

static struct riot6532 riot[MAX_RIOT_6532];

/******************* un-configuration *******************/

static void riot_timeout(int which);

void riot6532_unconfig(void)
{
	int i;

	for (i = 0; i < MAX_RIOT_6532; i++)
	{
		if ( riot[i].t ) {
			timer_remove(riot[i].t);
			riot[i].t = 0;
		}
	}

	memset(&riot, 0, sizeof(riot));
}


/******************* configuration *******************/

void riot6532_set_clock(int which, int clock)
{
	riot[which].sec_to_cycles = clock;
	riot[which].cycles_to_sec = 1.0 / riot[which].sec_to_cycles;
}

void riot6532_config(int which, const struct riot6532_interface *intf)
{
	if (which >= MAX_RIOT_6532) return;

	memset(&riot[which], 0x00, sizeof riot[which]);
	riot[which].inUse = 0x01;

	riot[which].intf = intf;
	riot[which].t = NULL;

	riot[which].time = timer_get_time();
	// riot[which].edge_detect_ctrl = 0x00;

	/* Default clock is from CPU1 */
	riot6532_set_clock(which, Machine->drv->cpu[0].cpu_clock);
}


/******************* reset *******************/

void riot6532_reset(void)
{
	int i;

	/* zap each structure, preserving the interface and swizzle */
	for (i = 0; i < MAX_RIOT_6532; i++)
	{
		riot[i].timer_divider = 1;
		riot[i].timer_start   = 0x00;
		riot[i].timer_irq_enabled = 0;

		riot[i].time = timer_get_time();

		if ( riot[i].inUse ) {
			riot[i].t = timer_alloc(riot_timeout);
			timer_adjust(riot[i].t,IFR_DELAY*riot[i].cycles_to_sec, i, 0);
		}
	}
}


/******************* wire-OR for all interrupt handlers *******************/

static void update_shared_irq_handler(void (*irq_func)(int state))
{
	int i;

	/* search all RIOTs for this same IRQ function */
	for (i = 0; i < MAX_RIOT_6532; i++)
		if (riot[i].intf)
		{
			if (riot[i].intf->irq_func == irq_func && riot[i].irq)
			{
				(*irq_func)(1);
				return;
			}
		}

	/* if we found nothing, the state is off */
	(*irq_func)(0);
}


/******************* external interrupt check *******************/

static void update_6532_interrupts(struct riot6532 *p)
{
	int new_state;

	new_state = 0;
	if ( (p->irq_state & RIOT_TIMERIRQ) && p->timer_irq_enabled ) 
		new_state = 1;

	if ( (p->irq_state & RIOT_PA7IRQ) && (p->edge_detect_ctrl & 0x02)) 
		new_state = 1;

	if (new_state != p->irq)
	{
		p->irq = new_state;
		if (p->intf->irq_func) update_shared_irq_handler(p->intf->irq_func);
	}
}

static void check_pa7_interrupt(struct riot6532 *p, int old_porta, int new_porta)
{
	if ( (old_porta==new_porta) || (p->irq & RIOT_PA7IRQ) )
		return; /* forget it */

	if ( p->edge_detect_ctrl&0x01 )
		p->irq_state |= (!(old_porta&0x80) && (new_porta&0x80)) ? RIOT_PA7IRQ : 0x00;
	else
		p->irq_state |= ((old_porta&0x80) && !(new_porta&0x80)) ? RIOT_PA7IRQ : 0x00;

	if (p->irq_state & RIOT_PA7IRQ)
		update_6532_interrupts(p);
}

static void riot_timeout(int which)
{
	struct riot6532 *p = riot + which;

	if ( p->irq_state & RIOT_TIMERIRQ ) {
		LOG(("RIOT%d: Timeout reached.\n", which));
		timer_enable(p->t,0);
	}
	else {
		LOG(("RIOT%d: Timer IRQ reached.\n", which));
		timer_reset(p->t, V_CYCLES_TO_TIME(255));
		p->time = timer_get_time();

		p->irq_state |= RIOT_TIMERIRQ;
		update_6532_interrupts(p);
	}
}

/******************* CPU interface for RIOT read *******************/

int riot6532_read(int which, int offset)
{
	struct riot6532 *p = riot + which;
	int val = 0;
	int old_timer_enabled = 0;

	/* adjust offset for 16-bit */
	offset &= 0x1f;

	if ( !(offset & 0x04) ) {
		switch( offset & 0x03 ) {
		case RIOT6532_PORTA:
			/* update the input */
			if (p->intf->in_a_func) p->in_a = p->intf->in_a_func(0);

			/* combine input and output values */
			val = (p->out_a & p->ddr_a) + (p->in_a & ~p->ddr_a);

			// LOG(("RIOT%d read port A = %02X\n", which, val));
			break;

		case RIOT6532_DDRA:
			val = p->ddr_a;
			// LOG(("RIOT%d read DDR A = %02X\n", which, val));
			break;
		
		case RIOT6532_PORTB:
			/* update the input */
			if (p->intf->in_b_func) p->in_b = p->intf->in_b_func(0);

			/* combine input and output values */
			val = (p->out_b & p->ddr_b) + (p->in_b & ~p->ddr_b);

			// LOG(("RIOT%d read port B = %02X\n", which, val));
			break;

		case RIOT6532_DDRB:
			val = p->ddr_b;
			//  LOG(("RIOT%d read DDR B = %02X\n", which, val));
			break;
		}
	}
	else {
		switch ( offset & 0x01 ) {
		case RIOT6532_TIMER:
			old_timer_enabled = timer_enable(p->t, 0);
			if ( old_timer_enabled ) // was timer enabled? re-enable it
				timer_enable(p->t, 1);

			if ( old_timer_enabled ) {
				if ( p->irq_state & RIOT_TIMERIRQ ) {
					val = 255 - V_TIME_TO_CYCLES(timer_get_time() - p->time);
				}
				else
					val = p->timer_start - V_TIME_TO_CYCLES(timer_get_time() - p->time) / p->timer_divider;
			}
			else
				val = 0x00;

			p->irq_state &= ~RIOT_TIMERIRQ;
			p->timer_irq_enabled = offset&0x08;
			update_6532_interrupts(p);

			LOG(("RIOT%d read timer = %02X %02X\n", which, val, offset&0x08));
			break;

		case RIOT6532_IRF:
			val = p->irq_state;
			p->irq_state &= ~RIOT_PA7IRQ;
			update_6532_interrupts(p);

			LOG(("RIOT%d read interrupt flags = %02X\n", which, val));
			break;
		}
	}

	return val;
}


/******************* CPU interface for RIOT write *******************/

void riot6532_write(int which, int offset, int data)
{
	struct riot6532 *p = riot + which;
	int old_port_a = p->in_a;

	/* adjust offset for 16-bit */
	offset &= 0x1f;

	if ( !(offset & 0x04) ) {
		switch ( offset & 0x03 ) {
		case RIOT6532_PORTA:
			LOG(("RIOT%d port A write = %02X\n", which, data));

			/* update the output value */
			p->out_a = data;

			/* check for possible interrupts on PA7 */
			if (p->ddr_a & 0x80 )
				check_pa7_interrupt(p, old_port_a, data);

			/* send it to the output function */
			if (p->intf->out_a_func && p->ddr_a) p->intf->out_a_func(0, p->out_a & p->ddr_a);
			break;

		case RIOT6532_DDRA:
			LOG(("RIOT%d DDR A write = %02X\n", which, data));

			if (p->ddr_a != data)
			{
				/* if DDR changed, call the callback again */
				p->ddr_a = data;

				/* send it to the output function */
				if (p->intf->out_a_func && p->ddr_a) p->intf->out_a_func(0, p->out_a & p->ddr_a);
			}
			break;
		
		case RIOT6532_PORTB:
			LOG(("RIOT%d port B write = %02X\n", which, data));

			/* update the output value */
			p->out_b = data;

			/* send it to the output function */
			if (p->intf->out_b_func && p->ddr_b) p->intf->out_b_func(0, p->out_b & p->ddr_b);
			break;

		case RIOT6532_DDRB:
			LOG(("RIOT%d DDR B write = %02X\n", which, data));

			if (p->ddr_b != data)
			{
				/* if DDR changed, call the callback again */
				p->ddr_b = data;

				/* send it to the output function */
				if (p->intf->out_b_func && p->ddr_b) p->intf->out_b_func(0, p->out_b & p->ddr_b);
			}
			break;
		}
	}
	else {
		/* timer and interrupt releated stuff */
		if ( offset & 0x10 ) {
			/* timer stuff */
			p->timer_irq_enabled = offset&0x08;
			p->timer_start = data;

			switch ( offset & 0x03 ) {
			case 0:
				p->timer_divider = 1;
				break;

			case 1:
				p->timer_divider = 8;
				break;

			case 2:
				p->timer_divider = 64;
				break;

			case 3:
				p->timer_divider = 1024;
				break;
			}
			p->irq_state &= ~RIOT_TIMERIRQ;
			update_6532_interrupts(p);

			timer_reset(p->t, V_CYCLES_TO_TIME(p->timer_divider * p->timer_start + IFR_DELAY));
			p->time = timer_get_time();

			LOG(("RIOT%d write timer = %02X * %04X, %04X\n", which, data, p->timer_divider, offset&0x08));
		}
		else {
			// edge control (PA7) stuff */
			LOG(("RIOT%d write edge detect control = %02X\n", which, offset & 0x03));

			p->edge_detect_ctrl = offset & 0x03;
		}
	}
}


/******************* interface setting RIOT port A input *******************/

void riot6532_set_input_a(int which, int data)
{
	struct riot6532 *p = riot + which;
	int old_port_a = p->in_a;

	p->in_a = data;

	check_pa7_interrupt(p, old_port_a, data);
}

/******************* interface setting RIOT port B input *******************/

void riot6532_set_input_b(int which, int data)
{
	struct riot6532 *p = riot + which;

	/* set the input, what could be easier? */
	p->in_b = data;
}

/******************* Standard 8-bit CPU interfaces, D0-D7 *******************/

READ_HANDLER( riot6532_0_r ) { return riot6532_read(0, offset); }
READ_HANDLER( riot6532_1_r ) { return riot6532_read(1, offset); }
READ_HANDLER( riot6532_2_r ) { return riot6532_read(2, offset); }
READ_HANDLER( riot6532_3_r ) { return riot6532_read(3, offset); }
READ_HANDLER( riot6532_4_r ) { return riot6532_read(4, offset); }
READ_HANDLER( riot6532_5_r ) { return riot6532_read(5, offset); }
READ_HANDLER( riot6532_6_r ) { return riot6532_read(6, offset); }
READ_HANDLER( riot6532_7_r ) { return riot6532_read(7, offset); }

WRITE_HANDLER( riot6532_0_w ) { riot6532_write(0, offset, data); }
WRITE_HANDLER( riot6532_1_w ) { riot6532_write(1, offset, data); }
WRITE_HANDLER( riot6532_2_w ) { riot6532_write(2, offset, data); }
WRITE_HANDLER( riot6532_3_w ) { riot6532_write(3, offset, data); }
WRITE_HANDLER( riot6532_4_w ) { riot6532_write(4, offset, data); }
WRITE_HANDLER( riot6532_5_w ) { riot6532_write(5, offset, data); }
WRITE_HANDLER( riot6532_6_w ) { riot6532_write(6, offset, data); }
WRITE_HANDLER( riot6532_7_w ) { riot6532_write(7, offset, data); }

/******************* Standard 16-bit CPU interfaces, D0-D7 *******************/

READ16_HANDLER( riot6532_0_lsb_r ) { return riot6532_read(0, offset); }
READ16_HANDLER( riot6532_1_lsb_r ) { return riot6532_read(1, offset); }
READ16_HANDLER( riot6532_2_lsb_r ) { return riot6532_read(2, offset); }
READ16_HANDLER( riot6532_3_lsb_r ) { return riot6532_read(3, offset); }
READ16_HANDLER( riot6532_4_lsb_r ) { return riot6532_read(4, offset); }
READ16_HANDLER( riot6532_5_lsb_r ) { return riot6532_read(5, offset); }
READ16_HANDLER( riot6532_6_lsb_r ) { return riot6532_read(6, offset); }
READ16_HANDLER( riot6532_7_lsb_r ) { return riot6532_read(7, offset); }

WRITE16_HANDLER( riot6532_0_lsb_w ) { if (ACCESSING_LSB) riot6532_write(0, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_1_lsb_w ) { if (ACCESSING_LSB) riot6532_write(1, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_2_lsb_w ) { if (ACCESSING_LSB) riot6532_write(2, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_3_lsb_w ) { if (ACCESSING_LSB) riot6532_write(3, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_4_lsb_w ) { if (ACCESSING_LSB) riot6532_write(4, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_5_lsb_w ) { if (ACCESSING_LSB) riot6532_write(5, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_6_lsb_w ) { if (ACCESSING_LSB) riot6532_write(6, offset, data & 0xff); }
WRITE16_HANDLER( riot6532_7_lsb_w ) { if (ACCESSING_LSB) riot6532_write(7, offset, data & 0xff); }

/******************* Standard 16-bit CPU interfaces, D8-D15 *******************/

READ16_HANDLER( riot6532_0_msb_r ) { return riot6532_read(0, offset) << 8; }
READ16_HANDLER( riot6532_1_msb_r ) { return riot6532_read(1, offset) << 8; }
READ16_HANDLER( riot6532_2_msb_r ) { return riot6532_read(2, offset) << 8; }
READ16_HANDLER( riot6532_3_msb_r ) { return riot6532_read(3, offset) << 8; }
READ16_HANDLER( riot6532_4_msb_r ) { return riot6532_read(4, offset) << 8; }
READ16_HANDLER( riot6532_5_msb_r ) { return riot6532_read(5, offset) << 8; }
READ16_HANDLER( riot6532_6_msb_r ) { return riot6532_read(6, offset) << 8; }
READ16_HANDLER( riot6532_7_msb_r ) { return riot6532_read(7, offset) << 8; }

WRITE16_HANDLER( riot6532_0_msb_w ) { if (ACCESSING_MSB) riot6532_write(0, offset, data >> 8); }
WRITE16_HANDLER( riot6532_1_msb_w ) { if (ACCESSING_MSB) riot6532_write(1, offset, data >> 8); }
WRITE16_HANDLER( riot6532_2_msb_w ) { if (ACCESSING_MSB) riot6532_write(2, offset, data >> 8); }
WRITE16_HANDLER( riot6532_3_msb_w ) { if (ACCESSING_MSB) riot6532_write(3, offset, data >> 8); }
WRITE16_HANDLER( riot6532_4_msb_w ) { if (ACCESSING_MSB) riot6532_write(4, offset, data >> 8); }
WRITE16_HANDLER( riot6532_5_msb_w ) { if (ACCESSING_MSB) riot6532_write(5, offset, data >> 8); }
WRITE16_HANDLER( riot6532_6_msb_w ) { if (ACCESSING_MSB) riot6532_write(6, offset, data >> 8); }
WRITE16_HANDLER( riot6532_7_msb_w ) { if (ACCESSING_MSB) riot6532_write(7, offset, data >> 8); }

/******************* 8-bit A/B port interfaces *******************/

WRITE_HANDLER( riot6532_0_porta_w ) { riot6532_set_input_a(0, data); }
WRITE_HANDLER( riot6532_1_porta_w ) { riot6532_set_input_a(1, data); }
WRITE_HANDLER( riot6532_2_porta_w ) { riot6532_set_input_a(2, data); }
WRITE_HANDLER( riot6532_3_porta_w ) { riot6532_set_input_a(3, data); }
WRITE_HANDLER( riot6532_4_porta_w ) { riot6532_set_input_a(4, data); }
WRITE_HANDLER( riot6532_5_porta_w ) { riot6532_set_input_a(5, data); }
WRITE_HANDLER( riot6532_6_porta_w ) { riot6532_set_input_a(6, data); }
WRITE_HANDLER( riot6532_7_porta_w ) { riot6532_set_input_a(7, data); }

WRITE_HANDLER( riot6532_0_portb_w ) { riot6532_set_input_b(0, data); }
WRITE_HANDLER( riot6532_1_portb_w ) { riot6532_set_input_b(1, data); }
WRITE_HANDLER( riot6532_2_portb_w ) { riot6532_set_input_b(2, data); }
WRITE_HANDLER( riot6532_3_portb_w ) { riot6532_set_input_b(3, data); }
WRITE_HANDLER( riot6532_4_portb_w ) { riot6532_set_input_b(4, data); }
WRITE_HANDLER( riot6532_5_portb_w ) { riot6532_set_input_b(5, data); }
WRITE_HANDLER( riot6532_6_portb_w ) { riot6532_set_input_b(6, data); }
WRITE_HANDLER( riot6532_7_portb_w ) { riot6532_set_input_b(7, data); }

READ_HANDLER( riot6532_0_porta_r ) { return riot[0].in_a; }
READ_HANDLER( riot6532_1_porta_r ) { return riot[1].in_a; }
READ_HANDLER( riot6532_2_porta_r ) { return riot[2].in_a; }
READ_HANDLER( riot6532_3_porta_r ) { return riot[3].in_a; }
READ_HANDLER( riot6532_4_porta_r ) { return riot[4].in_a; }
READ_HANDLER( riot6532_5_porta_r ) { return riot[5].in_a; }
READ_HANDLER( riot6532_6_porta_r ) { return riot[6].in_a; }
READ_HANDLER( riot6532_7_porta_r ) { return riot[7].in_a; }

READ_HANDLER( riot6532_0_portb_r ) { return riot[0].in_b; }
READ_HANDLER( riot6532_1_portb_r ) { return riot[1].in_b; }
READ_HANDLER( riot6532_2_portb_r ) { return riot[2].in_b; }
READ_HANDLER( riot6532_3_portb_r ) { return riot[3].in_b; }
READ_HANDLER( riot6532_4_portb_r ) { return riot[4].in_b; }
READ_HANDLER( riot6532_5_portb_r ) { return riot[5].in_b; }
READ_HANDLER( riot6532_6_portb_r ) { return riot[6].in_b; }
READ_HANDLER( riot6532_7_portb_r ) { return riot[7].in_b; }
