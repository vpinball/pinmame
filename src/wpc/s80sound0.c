#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "core.h"

#include "s80sound0.h"

static int s80s_sndCPUNo;

#define RIOT_TIMERIRQ			0x80
#define RIOT_PA7IRQ				0x40

/* switches on the board (to PB4/7) */
#define ATTRACT_MODE_TUNE		0x10
#define SOUND_TONES				0x80

#define ATTRACT_ENABLED	1
#define SOUND_ENABLED	1


#define BUFFER_SIZE 1024

typedef struct tagS80S0L {
  UINT16 timer_divider;
  UINT8 timer_value;
  UINT8 timer_irq_enabled;
  UINT8 timer;
  UINT8 irq_state;
  UINT8 irq;
  UINT8 edge_detect_ctrl;

  int soundLatch;
  
  int   stream;
  UINT8 buffer[BUFFER_SIZE];
  int   buf_pos;
} S80S0L;

static S80S0L S80sound0_locals;

void sys80_sound_latch_s(int data)
{
	// logerror("sound latch: 0x%02x\n", S80sound0_locals.soundLatch);
	S80sound0_locals.soundLatch = (data&0x0f);
}

static void update_6530_interrupts(void)
{
	S80S0L *p = &S80sound0_locals;
	int new_state;

	new_state = 0;
	if ( (p->irq_state & RIOT_TIMERIRQ) && p->timer_irq_enabled ) 
		new_state = 1;

	if ( (p->irq_state & RIOT_PA7IRQ) && (p->edge_detect_ctrl & 0x02)) 
		new_state = 1;

	if (new_state != p->irq)
	{
		p->irq = new_state;
/*		if (p->intf->irq_func) update_shared_irq_handler(p->intf->irq_func); */
	}
}


static READ_HANDLER(riot6530_r) {
	S80S0L *p = &S80sound0_locals;
	int val = 0;

	offset &= 0x0f;
	if ( !(offset&0x04) ) {
		// I/O
		switch ( offset&0x03 ) {
		case 0x00:
			logerror("read PA\n");
			break;

		case 0x01:
			logerror("read DDRA\n");
			break;

		case 0x02:
			val = S80sound0_locals.soundLatch | ATTRACT_ENABLED*ATTRACT_MODE_TUNE | SOUND_ENABLED*SOUND_TONES;
			// logerror("read PB (0x%02x)\n", val);
			break;

		case 0x03:
			logerror("read DDRB\n");
			break;
		}
	}
	else {
		if ( !(offset&0x01) ) {
			val = p->timer_value;

			p->irq_state &= ~RIOT_TIMERIRQ;
			p->timer_irq_enabled = offset&0x08;
			update_6530_interrupts();

//			logerror("RIOT6530 read timer = %02X %02X\n", val, offset&0x08);
		}
		else
			logerror("read IRQ\n");
	}

	return val;
}

static WRITE_HANDLER(riot6530_w) {
	S80S0L *p = &S80sound0_locals;

	offset &= 0x0f;
	if ( !(offset&0x04) ) {
		// I/O
		switch ( offset&0x03 ) {
		case 0x00:
			// logerror("write PA (0x%02x)\n", data);
			if ( S80sound0_locals.buf_pos>=BUFFER_SIZE )
				return;
			S80sound0_locals.buffer[S80sound0_locals.buf_pos++] = data;
			break;

		case 0x01:
			// logerror("write DDRA (0x%02x)\n", data);
			break;

		case 0x02:
			logerror("write PB (0x%02x)\n", data);
			break;

		case 0x03:
			logerror("write DDRB(0x%02x)\n", data);
			break;
		}
	}
	else {
//		logerror("write timer (0x%02x)\n", data);

		p->timer_irq_enabled = offset&0x08;
		p->timer_value = data;
		
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
		update_6530_interrupts();
	}
}

static UINT8 ram6530[0x0200]; 

static READ_HANDLER(ram_6530r) {
	return ram6530[offset];
}

static WRITE_HANDLER(ram_6530w) {
	ram6530[offset] = data;
}


/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(S80S_sreadmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x02ff, riot6530_r},
{ 0x0400, 0x07ff, MRA_ROM},
{ 0x0800, 0x0bff, MRA_ROM},
{ 0x0c00, 0x0fff, MRA_ROM},
{ 0xfc00, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(S80S_swritemem)
{ 0x0000, 0x01ff, MWA_RAM},
{ 0x0200, 0x02ff, riot6530_w},
{ 0x0400, 0x07ff, MWA_ROM},
{ 0x0800, 0x0bff, MWA_ROM},
{ 0x0c00, 0x0fff, MWA_ROM},
{ 0xfc00, 0xffff, MWA_ROM},
MEMORY_END

void riot6530_clk(int param)
{
	S80S0L *p = &S80sound0_locals;

	if ( p->irq_state & RIOT_TIMERIRQ ) {
		if ( p->timer_value )
			p->timer_value--;
	}
	else {
		p->timer--;
		if ( !p->timer ) {
			p->timer = p->timer_divider;

			p->timer_value--;
			if ( p->timer_value==0xff ) {
				p->irq_state |= RIOT_TIMERIRQ;
			}
		}
	}
}

static void s80_s_Update(int num, INT16 *buffer, int length)
{
	int i = 0;
	// logerror("%i  %i\n", length, S80sound0_locals.buf_pos);
	memset (buffer,0,length*sizeof(INT16));

	if ( S80sound0_locals.buf_pos ) {
		if ( S80sound0_locals.buf_pos<length )
			length = S80sound0_locals.buf_pos;

		while ( length ) {
			*buffer++ = (0x80-S80sound0_locals.buffer[i++])*0x100;
			length--;
		}

		S80sound0_locals.buf_pos = 0;
	}
}

/*--------------
/  init
/---------------*/
void S80S_sinit(int num) {
	s80s_sndCPUNo = num;

	memset(&S80sound0_locals, 0x00, sizeof S80sound0_locals);

	S80sound0_locals.timer_divider = 1;
	S80sound0_locals.timer_value   = 0x00;
	S80sound0_locals.timer_irq_enabled = 0;
	S80sound0_locals.timer = S80sound0_locals.timer_divider;

	S80sound0_locals.stream = stream_init("SND DAC", 100, 12000, 0, s80_s_Update);

	timer_pulse(TIME_IN_HZ(1000000/16), 0, riot6530_clk);
}
