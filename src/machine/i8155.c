// license:BSD-3-Clause
// copyright-holders:Curt Coder,AJR
/**********************************************************************

    Intel 8155/8156 - 2048-Bit Static MOS RAM with I/O Ports and Timer

    The timer primarily functions as a square-wave generator, but can
    also be programmed for a single-cycle low pulse on terminal count.

    The only difference between 8155 and 8156 is that pin 8 (CE) is
    active low on the former device and active high on the latter.

    National's NSC810 RAM-I/O-Timer is pin-compatible with the Intel
    8156, but has different I/O registers (including a second timer)
    with incompatible mapping.

**********************************************************************/

/*

    TODO:

    - ALT 3 and ALT 4 strobed port modes

*/

#include "driver.h"
#include "core.h"
#include "timer.h"
#include "machine/i8155.h"

/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

enum
{
	I8155_REGISTER_COMMAND = 0,
	I8155_REGISTER_STATUS = 0,
	I8155_REGISTER_PORT_A,
	I8155_REGISTER_PORT_B,
	I8155_REGISTER_PORT_C,
	I8155_REGISTER_TIMER_LOW,
	I8155_REGISTER_TIMER_HIGH
};

enum
{
	I8155_PORT_A = 0,
	I8155_PORT_B,
	I8155_PORT_C,
	I8155_PORT_COUNT
};

enum
{
	I8155_PORT_MODE_INPUT = 0,
	I8155_PORT_MODE_OUTPUT,
	I8155_PORT_MODE_STROBED_PORT_A,	/* not supported */
	I8155_PORT_MODE_STROBED			/* not supported */
};

#define I8155_COMMAND_PA					0x01
#define I8155_COMMAND_PB					0x02
#define I8155_COMMAND_PC_MASK				0x0c
#define I8155_COMMAND_PC_ALT_1				0x00
#define I8155_COMMAND_PC_ALT_2				0x0c
#define I8155_COMMAND_PC_ALT_3				0x04	/* not supported */
#define I8155_COMMAND_PC_ALT_4				0x08	/* not supported */
#define I8155_COMMAND_IEA					0x10	/* not supported */
#define I8155_COMMAND_IEB					0x20	/* not supported */
#define I8155_COMMAND_TM_MASK				0xc0
#define I8155_COMMAND_TM_NOP				0x00
#define I8155_COMMAND_TM_STOP				0x40
#define I8155_COMMAND_TM_STOP_AFTER_TC		0x80
#define I8155_COMMAND_TM_START				0xc0

#define I8155_STATUS_INTR_A					0x01	/* not supported */
#define I8155_STATUS_A_BF					0x02	/* not supported */
#define I8155_STATUS_INTE_A					0x04	/* not supported */
#define I8155_STATUS_INTR_B					0x08	/* not supported */
#define I8155_STATUS_B_BF					0x10	/* not supported */
#define I8155_STATUS_INTE_B					0x20	/* not supported */
#define I8155_STATUS_TIMER					0x40

#define I8155_TIMER_MODE_MASK				0xc0
#define I8155_TIMER_MODE_LOW				0x00
#define I8155_TIMER_MODE_SQUARE_WAVE		0x40
#define I8155_TIMER_MODE_SINGLE_PULSE		0x80
#define I8155_TIMER_MODE_AUTOMATIC_RELOAD	0xc0

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct
{
	mem_read_handler		in_port_func[I8155_PORT_COUNT];
	mem_write_handler		out_port_func[I8155_PORT_COUNT];
	mem_write_handler	out_to_func;

	/* registers */
	UINT8 command;				/* command register */
	UINT8 status;				/* status register */
	UINT8 output[3];			/* output latches */

	/* counter */
	UINT16 count_length;		/* count length register */
	UINT16 counter;				/* counter register */
	int to;						/* timer output */

	/* timer */
	mame_timer *timer;			/* counter timer */
} i8155_t;

static i8155_t chips[MAX_8155];

/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

INLINE UINT8 get_timer_mode(i8155_t *i8155)
{
	return (i8155->count_length >> 8) & I8155_TIMER_MODE_MASK;
}

INLINE void timer_output(i8155_t *i8155)
{
	(i8155->out_to_func)(0, i8155->to);

	if (LOG) logerror("8155 Timer Output: %u\n", i8155->to);
}

INLINE void pulse_timer_output(i8155_t *i8155)
{
	i8155->to = 0; timer_output(i8155);
	i8155->to = 1; timer_output(i8155);
}

INLINE int get_port_mode(i8155_t *i8155, int port)
{
	int mode = -1;

	switch (port)
	{
	case I8155_PORT_A:
		mode = (i8155->command & I8155_COMMAND_PA) ? I8155_PORT_MODE_OUTPUT : I8155_PORT_MODE_INPUT;
		break;

	case I8155_PORT_B:
		mode = (i8155->command & I8155_COMMAND_PB) ? I8155_PORT_MODE_OUTPUT : I8155_PORT_MODE_INPUT;
		break;

	case I8155_PORT_C:
		switch (i8155->command & I8155_COMMAND_PC_MASK)
		{
		case I8155_COMMAND_PC_ALT_1: mode = I8155_PORT_MODE_INPUT;			break;
		case I8155_COMMAND_PC_ALT_2: mode = I8155_PORT_MODE_OUTPUT;			break;
		case I8155_COMMAND_PC_ALT_3: mode = I8155_PORT_MODE_STROBED_PORT_A; break;
		case I8155_COMMAND_PC_ALT_4: mode = I8155_PORT_MODE_STROBED;		break;
		}
		break;
	}

	return mode;
}

INLINE UINT8 read_port(i8155_t *i8155, int port)
{
	UINT8 data = 0;

	switch (port)
	{
	case I8155_PORT_A:
	case I8155_PORT_B:
		switch (get_port_mode(i8155, port))
		{
		case I8155_PORT_MODE_INPUT:
			data = i8155->in_port_func[port] ? (i8155->in_port_func[port])(0) : 0;
			break;

		case I8155_PORT_MODE_OUTPUT:
			data = i8155->output[port];
			break;
		}
		break;

	case I8155_PORT_C:
		switch (get_port_mode(i8155, I8155_PORT_C))
		{
		case I8155_PORT_MODE_INPUT:
			data = i8155->in_port_func[port] ? (i8155->in_port_func[port])(0) & 0x3f : 0;
			break;

		case I8155_PORT_MODE_OUTPUT:
			data = i8155->output[port] & 0x3f;
			break;

		default:
			logerror("8155 Unsupported Port C mode!\n");
		}
		break;
	}

	return data;
}

INLINE void write_port(i8155_t *i8155, int port, UINT8 data)
{
	switch (get_port_mode(i8155, port))
	{
	case I8155_PORT_MODE_OUTPUT:
		i8155->output[port] = data;
		if (i8155->out_port_func[port]) (i8155->out_port_func[port])(0, i8155->output[port]);
		break;
	}
}

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    TIMER_CALLBACK( counter_tick )
-------------------------------------------------*/

static void counter_tick(int n)
{
	i8155_t *i8155 = &chips[n];

	/* count down */
	i8155->counter--;

	if (get_timer_mode(i8155) == I8155_TIMER_MODE_LOW)
	{
		/* pulse on every count */
		pulse_timer_output(i8155);
	}

	if (i8155->counter == 0)
	{
		if (LOG) logerror("8155 #%x Timer Count Reached\n", n);

		switch (i8155->command & I8155_COMMAND_TM_MASK)
		{
		case I8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop timer */
			if (i8155->timer) timer_enable(i8155->timer, 0);
			if (LOG) logerror("8155 #%x Timer Stopped\n", n);
			break;
		}

		switch (get_timer_mode(i8155))
		{
		case I8155_TIMER_MODE_SQUARE_WAVE:
			/* toggle timer output */
			i8155->to = !i8155->to;
			timer_output(i8155);
			break;

		case I8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			pulse_timer_output(i8155);

			/* clear timer mode setting */
			i8155->command &= ~I8155_COMMAND_TM_MASK;
			break;

		case I8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			pulse_timer_output(i8155);
			break;
		}

		/* set timer flag */
		i8155->status |= I8155_STATUS_TIMER;

		/* reload timer counter */
		i8155->counter = i8155->count_length & 0x3fff;
	}
}

/*-------------------------------------------------
    i8155_r - register read
-------------------------------------------------*/

int i8155_r(int which, int offset)
{
	i8155_t *i8155 = &chips[which];

	UINT8 data = 0;

	switch (offset & 0x03)
	{
	case I8155_REGISTER_STATUS:
		data = i8155->status;

		/* clear timer flag */
		i8155->status &= ~I8155_STATUS_TIMER;
		break;

	case I8155_REGISTER_PORT_A:
		data = read_port(i8155, I8155_PORT_A);
		break;

	case I8155_REGISTER_PORT_B:
		data = read_port(i8155, I8155_PORT_B);
		break;

	case I8155_REGISTER_PORT_C:
		data = read_port(i8155, I8155_PORT_C);
		break;
	}

	return data;
}

/*-------------------------------------------------
    i8155_w - register write
-------------------------------------------------*/

void i8155_w(int which, int offset, UINT8 data)
{
	i8155_t *i8155 = &chips[which];
	
	switch (offset & 0x07)
	{
	case I8155_REGISTER_COMMAND:
		i8155->command = data;

		if (LOG) logerror("8155 #%x Port A Mode: %s\n", which, (data & I8155_COMMAND_PA) ? "output" : "input");
		if (LOG) logerror("8155 #%x Port B Mode: %s\n", which, (data & I8155_COMMAND_PB) ? "output" : "input");

		if (LOG) logerror("8155 #%x Port A Interrupt: %s\n", which, (data & I8155_COMMAND_IEA) ? "enabled" : "disabled");
		if (LOG) logerror("8155 #%x Port B Interrupt: %s\n", which, (data & I8155_COMMAND_IEB) ? "enabled" : "disabled");

		switch (data & I8155_COMMAND_PC_MASK)
		{
		case I8155_COMMAND_PC_ALT_1:
			if (LOG) logerror("8155 #%x Port C Mode: Alt 1\n", which);
			break;

		case I8155_COMMAND_PC_ALT_2:
			if (LOG) logerror("8155 #%x Port C Mode: Alt 2\n", which);
			break;

		case I8155_COMMAND_PC_ALT_3:
			if (LOG) logerror("8155 #%x Port C Mode: Alt 3\n", which);
			break;

		case I8155_COMMAND_PC_ALT_4:
			if (LOG) logerror("8155 #%x Port C Mode: Alt 4\n", which);
			break;
		}

		switch (data & I8155_COMMAND_TM_MASK)
		{
		case I8155_COMMAND_TM_NOP:
			/* do not affect counter operation */
			break;

		case I8155_COMMAND_TM_STOP:
			/* NOP if timer has not started, stop counting if the timer is running */
			if (LOG) logerror("8155 #%x Timer Command: Stop\n", which);
			if (i8155->timer) timer_enable(i8155->timer, 0);
			break;

		case I8155_COMMAND_TM_STOP_AFTER_TC:
			/* stop immediately after present TC is reached (NOP if timer has not started) */
			if (LOG) logerror("8155 #%x Timer Command: Stop after TC\n", which);
			break;

		case I8155_COMMAND_TM_START:
			if (LOG) logerror("%04x: 8155 #%x Timer Command: Start\n", activecpu_get_pc(), which);

			if (i8155->timer) {
				if (timer_enabled(i8155->timer))
				{
					/* if timer is running, start the new mode and CNT length immediately after present TC is reached */
				}
				else
				{
					/* load mode and CNT length and start immediately after loading (if timer is not running) */
					i8155->counter = i8155->count_length;
					timer_adjust(i8155->timer, 0, which, TIME_IN_HZ(Machine->drv->cpu[0].cpu_clock));
				}
			}
			break;
		}
		break;

	case I8155_REGISTER_PORT_A:
		write_port(i8155, I8155_PORT_A, data);
		break;

	case I8155_REGISTER_PORT_B:
		write_port(i8155, I8155_PORT_B, data);
		break;

	case I8155_REGISTER_PORT_C:
		write_port(i8155, I8155_PORT_C, data);
		break;

	case I8155_REGISTER_TIMER_LOW:
		i8155->count_length = (i8155->count_length & 0xff00) | data;
		if (LOG) logerror("8155 #%x Count Length Low: %04x\n", which, i8155->count_length);
		break;

	case I8155_REGISTER_TIMER_HIGH:
		i8155->count_length = (data << 8) | (i8155->count_length & 0xff);
		if (LOG) logerror("8155 #%x Count Length High: %04x\n", which, i8155->count_length);

		switch (data & I8155_TIMER_MODE_MASK)
		{
		case I8155_TIMER_MODE_LOW:
			/* puts out LOW during second half of count */
			if (LOG) logerror("8155 #%x Timer Mode: LOW\n", which);
			break;

		case I8155_TIMER_MODE_SQUARE_WAVE:
			/* square wave, i.e. the period of the square wave equals the count length programmed with automatic reload at terminal count */
			if (LOG) logerror("8155 #%x Timer Mode: Square wave\n", which);
			break;

		case I8155_TIMER_MODE_SINGLE_PULSE:
			/* single pulse upon TC being reached */
			if (LOG) logerror("8155 #%x Timer Mode: Single pulse\n", which);
			break;

		case I8155_TIMER_MODE_AUTOMATIC_RELOAD:
			/* automatic reload, i.e. single pulse every time TC is reached */
			if (LOG) logerror("8155 #%x Timer Mode: Automatic reload\n", which);
			break;
		}
		break;
	}
}

void i8155_reset( int which )
{
	i8155_t *i8155 = &chips[which];

	/* clear output registers */
	i8155->output[I8155_PORT_A] = 0;
	i8155->output[I8155_PORT_B] = 0;
	i8155->output[I8155_PORT_C] = 0;

	/* set ports to input mode */
	i8155_w(which, I8155_REGISTER_COMMAND, i8155->command & ~(I8155_COMMAND_PA | I8155_COMMAND_PB | I8155_COMMAND_PC_MASK));

	/* clear timer flag */
	i8155->status &= ~I8155_STATUS_TIMER;

	/* clear timer output */
	i8155->to = 1;

	/* stop counting */
	if (i8155->timer) {
		i8155->counter = i8155->count_length = 0;
		timer_enable(i8155->timer, 0);
		timer_output(i8155);
	}

}

void i8155_init( i8155_interface *intfce )
{
	int i;

	int num = intfce->num;

	for (i = 0; i < num; i++)
	{
		chips[i].in_port_func[I8155_PORT_A] = intfce->in_pa_func[i];
		chips[i].in_port_func[I8155_PORT_B] = intfce->in_pb_func[i];
		chips[i].in_port_func[I8155_PORT_C] = intfce->in_pc_func[i];
		chips[i].out_port_func[I8155_PORT_A] = intfce->out_pa_func[i];
		chips[i].out_port_func[I8155_PORT_B] = intfce->out_pb_func[i];
		chips[i].out_port_func[I8155_PORT_C] = intfce->out_pc_func[i];
		chips[i].out_to_func = intfce->out_to_func[i];

		/* create the timers */
		if (chips[i].out_to_func) {
			chips[i].timer = timer_alloc(counter_tick);
		}
	}
}

READ_HANDLER( i8155_0_r ) { return i8155_r( 0, offset ); }
READ_HANDLER( i8155_1_r ) { return i8155_r( 1, offset ); }
READ_HANDLER( i8155_2_r ) { return i8155_r( 2, offset ); }
READ_HANDLER( i8155_3_r ) { return i8155_r( 3, offset ); }

WRITE_HANDLER( i8155_0_w ) { i8155_w( 0, offset, data ); }
WRITE_HANDLER( i8155_1_w ) { i8155_w( 1, offset, data ); }
WRITE_HANDLER( i8155_2_w ) { i8155_w( 2, offset, data ); }
WRITE_HANDLER( i8155_3_w ) { i8155_w( 3, offset, data ); }
