#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "core.h"

#include "s80sound1.h"

static int s80ss_sndCPUNo;

#define BUFFER_SIZE 4096

struct {
	int	NMIState;
	int soundInput;

	int stream;
	INT16  buffer[BUFFER_SIZE+1];
	double clock[BUFFER_SIZE+1];

	int	buf_pos;
	void *timer;
} S80sound1_locals;

void S80SS_irq(int state) {
	logerror("IRQ: %i\n",state);
	cpu_set_irq_line(s80ss_sndCPUNo, M6502_INT_IRQ, state ? ASSERT_LINE : CLEAR_LINE);
}

void S80SS_nmi(int state)
{
	logerror("NMI: %i/n\n",state);
	cpu_set_irq_line(s80ss_sndCPUNo, M6502_INT_NMI, state? ASSERT_LINE:CLEAR_LINE);
	S80sound1_locals.NMIState = state;
	if ( state ) 
		S80sound1_locals.timer = timer_set(TIME_IN_USEC(100),0,S80SS_nmi);
	else
		S80sound1_locals.timer = 0;
}

UINT8 riot_3_ram[256];

READ_HANDLER( riot_3_ram_r )
{
    return riot_3_ram[offset&0x7f];
}

WRITE_HANDLER( riot_3_ram_w )
{
	riot_3_ram[offset&0x7f]=data;
}


/* input */
READ_HANDLER(riot3a_r)  {
	logerror("riot3a_r\n");
	return (S80sound1_locals.soundInput&0x0f?0x80:0x00) | 0x40 | S80sound1_locals.soundInput;
}
WRITE_HANDLER(riot3a_w) { logerror("riot3a_w: 0x%02x\n", data);}


/* Switch settings, test switch and NMI */
READ_HANDLER(riot3b_r)  {
	if ( S80sound1_locals.timer )
		logerror("riot3b_r\n");
	return (S80sound1_locals.NMIState?0x80:0x00) | (0x22)^0x7f;
}

WRITE_HANDLER(riot3b_w) { logerror("riot3b_w: 0x%02x\n", data);}

/* D/A converters */
WRITE_HANDLER(da1_latch_w) {
	/* logerror("da1_w: 0x%02x\n", data); */
	if ( S80sound1_locals.buf_pos>=BUFFER_SIZE )
		return;

	S80sound1_locals.clock[S80sound1_locals.buf_pos] = timer_get_time();
	S80sound1_locals.buffer[S80sound1_locals.buf_pos++] = (0x80-data)*0x100;
}

WRITE_HANDLER(da2_latch_w) {
/*	logerror("da2_w: 0x%02x\n", data); */
}

static const char *PhonemeTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

/* voice synt latch */
WRITE_HANDLER(vs_latch_w) {
	static int queue[100],pos;

	data ^= 0xff;
	if ( pos<100)
		queue[pos++] = data & 0x3f;

	logerror("Votrax: intonation %d, phoneme %02x %s\n",data >> 6,data & 0x3f,PhonemeTable[data & 0x3f]);
	if ((data & 0x3f) == 0x3f) {
		if (pos > 1) {
			int i;
			char buf[200];

			buf[0] = 0;
			for (i = 0;i < pos-1;i++) {
				if (queue[i] == 0x03 || queue[i] == 0x3e)
					strcat(buf," ");
				else
					strcat(buf,PhonemeTable[queue[i]]);
			}

			usrintf_showmessage(buf);
		}
		pos = 0;
	}
	if ( !S80sound1_locals.timer )
		S80sound1_locals.timer = timer_set(TIME_IN_USEC(50000),1,S80SS_nmi);
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(S80SS_sreadmem)
{ 0x0000, 0x01ff, riot_3_ram_r},
{ 0x0200, 0x027f, riot_3_r},
/*{ 0x4000, 0x4fff, ext_board_1_r},*/
/*{ 0x5000, 0x5fff, ext_board_2_r},*/
/*{ 0x6000, 0x6fff, ext_board_3_r},*/
{ 0x7000, 0x7fff, MRA_ROM},
{ 0xf000, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(S80SS_swritemem)
{ 0x0000, 0x01ff, riot_3_ram_w},
{ 0x0200, 0x027f, riot_3_w},
{ 0x1000, 0x1fff, da1_latch_w},
{ 0x2000, 0x2fff, vs_latch_w},
{ 0x3000, 0x3fff, da2_latch_w},
/*{ 0x4000, 0x4fff, ext_board_1_w},*/
/*{ 0x5000, 0x5fff, ext_board_2_w},*/
/*{ 0x6000, 0x6fff, ext_board_3_w},*/
{ 0x7000, 0x7fff, MWA_ROM},
{ 0xf000, 0xfdff, MWA_ROM},
{ 0xff00, 0xffff, MWA_RAM},
MEMORY_END

void sys80_sound_latch_ss(int data)
{
	int old = S80sound1_locals.soundInput;
	S80sound1_locals.soundInput = (data & 0x3f);

	if ( S80sound1_locals.soundInput!=old ) {
		logerror("sound_latch: 0x%02x\n", S80sound1_locals.soundInput);
		riot_set_input_a(3, (S80sound1_locals.soundInput&0x3f) | (S80sound1_locals.soundInput&0x0f?0x80:0x00));
	}
}

static void s80_ss_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;
		
	dCurrentClock = S80sound1_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-S80sound1_locals.clock[0]) / length;

	if ( S80sound1_locals.buf_pos>1 )
		S80sound1_locals.buf_pos = S80sound1_locals.buf_pos;

	i = 0;
	S80sound1_locals.clock[S80sound1_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = S80sound1_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (S80sound1_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	S80sound1_locals.clock[0] = dActClock;
	S80sound1_locals.buffer[0] = S80sound1_locals.buffer[S80sound1_locals.buf_pos-1];
	S80sound1_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/
void S80SS_sinit(int num) {
	s80ss_sndCPUNo = num;

	memset(&S80sound1_locals, 0x00, sizeof S80sound1_locals);
	riot_set_clock(3, Machine->drv->cpu[s80ss_sndCPUNo].cpu_clock);

	S80sound1_locals.clock[0]  = 0;
	S80sound1_locals.buffer[0] = 0;
	S80sound1_locals.buf_pos   = 1;

	S80sound1_locals.stream = stream_init("SND DAC", 100, 11025, 0, s80_ss_Update); 
}

void S80SS_sexit()
{
	if ( S80sound1_locals.timer ) {
		timer_remove(S80sound1_locals.timer);
		S80sound1_locals.timer = 0;
	}
}




