#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "core.h"

#include "s80sound1.h"

static int s80ss_sndCPUNo;

#define BUFFER_SIZE 1024

struct {
	int	NMIState;
	int soundInput;

	int stream;
	INT16 buffer[BUFFER_SIZE];
	int	buf_pos;
} S80sound1_locals;

void S80SS_irq(int state) {
  // logerror("set sound irq\n");
  S80sound1_locals.NMIState = 1;
  cpu_set_irq_line(s80ss_sndCPUNo, M6502_INT_IRQ, state ? ASSERT_LINE : CLEAR_LINE);
}

void S80SS_nmi(int state)
{
	logerror("NMI\n");
	cpu_set_irq_line(s80ss_sndCPUNo, M6502_INT_NMI, PULSE_LINE);
	S80sound1_locals.NMIState = 1;
}

unsigned char riot_3_ram[256];

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
//	logerror("riot3a_r\n"); 
	return (S80sound1_locals.soundInput&0x0f?0x80:0x00) | 0x40 | S80sound1_locals.soundInput;
}
WRITE_HANDLER(riot3a_w) { logerror("riot3a_w: 0x%02x\n", data);}


/* Switch settings, test switch and NMI */
READ_HANDLER(riot3b_r)  { 
	// logerror("riot3b_r\n"); 
	return (S80sound1_locals.NMIState?0x00:0x80) | 0x54; 
}

WRITE_HANDLER(riot3b_w) { logerror("riot3b_w: 0x%02x\n", data);}

/* D/A converters */
WRITE_HANDLER(da1_latch_w) { 
	/* logerror("da1_w: 0x%02x\n", data); */
	if ( S80sound1_locals.buf_pos>=BUFFER_SIZE )
		return;

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
	queue[pos++] = data & 0x3f;

	S80sound1_locals.NMIState = 0;
	
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
	else
		timer_set(TIME_IN_USEC(50),1,S80SS_nmi);
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
//	int i = 0;
	// logerror("%i  %i\n", length, S80sound1_locals.buf_pos);
	memset(buffer,0x00,length*sizeof(INT16));

	if ( S80sound1_locals.buf_pos ) {
		memset(buffer,S80sound1_locals.buffer[0],length*sizeof(INT16));
		if ( S80sound1_locals.buf_pos<length )
			length = S80sound1_locals.buf_pos;

		memcpy(buffer, &S80sound1_locals.buffer, 2*length);
		S80sound1_locals.buf_pos = 0;
	}
}

static void RIOT_clock(int param)
{
	riot_clk(3);
}

/*--------------
/  init
/---------------*/
void S80SS_sinit(int num) {
	s80ss_sndCPUNo = num;

	memset(&S80sound1_locals, 0x00, sizeof S80sound1_locals);

	S80sound1_locals.stream = stream_init("SND DAC", 100, 6950, 0, s80_ss_Update);
//	timer_pulse(TIME_IN_HZ(200000), 0, RIOT_clock);
}



