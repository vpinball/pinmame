#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "machine/6530riot.h"
#include "machine/6532riot.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "core.h"
#include "sndbrd.h"

#include "gts80.h"
#include "gts80s.h"

/* 
    Gottlieb System 80 Sound Boards
   
    - System 80/80A Sound Board 

    - System 80/80A Sound & Speech Board

    - System 80A Sound Board with a PiggyPack installed
	  (thanks goes to Peter Hall for providing some very usefull information)
   
    - System 80B Sound Board (3 generations)

*/


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound Board
/-----------------------------------------*/

#define GTS80S_BUFFER_SIZE 8192

extern void sh_votrax_start(int Channel);
extern void sh_votrax_stop(void);
extern void votrax_w(int data);

struct {
	struct sndbrdData boardData;

	int   stream;
	INT16 buffer[GTS80S_BUFFER_SIZE+1];
	double clock[GTS80S_BUFFER_SIZE+1];
	int   buf_pos;

	int   dips;
	int	IRQEnabled;
} GTS80S_locals;

WRITE_HANDLER(gts80s_data_w)
{
//	logerror("sound latch: 0x%02x\n", data);
	data &= 0x0f;
	riot6530_set_input_b(0, GTS80S_locals.dips | 0x20 | data);

	/* the PiggyPack board is really firing an interrupt; sound board layout
	   for theses game is different, because the IRQ line is connected */
	if ( (GTS80S_locals.boardData.subType==1) && data && GTS80S_locals.IRQEnabled )
		cpu_set_irq_line(GTS80S_locals.boardData.cpuNo, M6502_INT_IRQ, PULSE_LINE);
}

/* configured as output, shouldn't be read at all */
READ_HANDLER(riot6530_0a_r)  {
	logerror("riot6530_0a_r\n");
	return 0x00;
}

/* digital sound data output */
WRITE_HANDLER(riot6530_0a_w) {
//	logerror("riot6530_0a_w: 0x%02x\n", data);

	if ( GTS80S_locals.buf_pos>=GTS80S_BUFFER_SIZE )
		return;

	GTS80S_locals.clock[GTS80S_locals.buf_pos] = timer_get_time();
	GTS80S_locals.buffer[GTS80S_locals.buf_pos++] = (0x80-data)<<8;
}

WRITE_HANDLER(riot6530_0b_w) { 
//	logerror("riot6530_0b_w: 0x%02x\n", data);
	/* reset the interupt on the PiggyPack board */
	if ( GTS80S_locals.boardData.subType==1 )
		GTS80S_locals.IRQEnabled = data&0x40;
}

static UINT8 RIOT6532_RAM[0x0200]; 

static READ_HANDLER(riot6530_0_ram_r) {
	return RIOT6532_RAM[offset];
}

static WRITE_HANDLER(riot6530_0_ram_w) {
	RIOT6532_RAM[offset] = data;
}

struct riot6530_interface GTS80S_riot6530_intf = {
 /* 6530RIOT 0 (0x200) Chip U2 */
 /* PA0 - PA7 Digital sound data */
 /* PB0 - PB7 Sound latch & Dip-Switches */
 /* in  : A/B, */ riot6530_0a_r, NULL,
 /* out : A/B, */ riot6530_0a_w, riot6530_0b_w,
 /* irq :      */ NULL
};


/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80S_readmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x02ff, riot6530_0_r},
{ 0x0400, 0x0fff, MRA_ROM},
{ 0xf800, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80S_writemem)
{ 0x0000, 0x01ff, MWA_RAM},
{ 0x0200, 0x02ff, riot6530_0_w},
{ 0x0400, 0x0fff, MWA_ROM},
{ 0xf800, 0xffff, MWA_ROM},
MEMORY_END

static void GTS80S_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;
		
	dCurrentClock = GTS80S_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80S_locals.clock[0]) / length;

	i = 0;
	GTS80S_locals.clock[GTS80S_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80S_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80S_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80S_locals.clock[0] = dActClock;
	GTS80S_locals.buffer[0] = GTS80S_locals.buffer[GTS80S_locals.buf_pos-1];
	GTS80S_locals.buf_pos = 1;
}

/*--------------
/  init
/---------------*/

void gts80s_init(struct sndbrdData *brdData) {
	int i = 0;
	UINT8 *pMem;

	memset(&GTS80S_locals, 0x00, sizeof GTS80S_locals);

	GTS80S_locals.boardData = *brdData;

	/* init dips */
	GTS80S_locals.dips =
		((core_getDip(5)&0x01) ? 0x00:0x80) | /* S1: Sound/Tones        */
		((core_getDip(5)&0x02) ? 0x00:0x10);  /* S2: Attrach Mode Tunes */

	/* init sound buffer */
	GTS80S_locals.clock[0]  = 0;
	GTS80S_locals.buffer[0] = 0;
	GTS80S_locals.buf_pos   = 1;

	/* init RAM */
	memset(RIOT6532_RAM, 0x00, sizeof RIOT6532_RAM);

	if ( GTS80S_locals.boardData.subType==0 ) {
		/* clear the upper 4 bits, some ROM images aren't 0 */ 
		/* the 6530 RIOT ROM is not used by the boards which have a PiggyPack installed */
		pMem = memory_region(GTS80S_locals.boardData.cpuNo)+0x0400;
		for(i=0x0400; i<0x0bff; i++)
			*pMem++ = (*pMem&0x0f);
	}

	/* init the RIOT */
    riot6530_config(0, &GTS80S_riot6530_intf);
	riot6530_set_clock(0, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);
	riot6530_reset();

	GTS80S_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80S_Update);
	set_RC_filter(GTS80S_locals.stream, 270000, 15000, 0, 33000);
}

void gts80s_exit(int boardNo)
{
	riot6530_unconfig();
}

const struct sndbrdIntf gts80sIntf = {
	gts80s_init, gts80s_exit, NULL, gts80s_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound Board
/-----------------------------------------*/

#define GTS80SS_BUFFER_SIZE 4096

struct {
	struct sndbrdData boardData;

	int dips;
	int NMIState;

	int stream;
	INT16  buffer[GTS80SS_BUFFER_SIZE+1];
	double clock[GTS80SS_BUFFER_SIZE+1];
	int	buf_pos;

	void *timer;
} GTS80SS_locals;

void GTS80SS_irq(int state) {
//	logerror("IRQ: %i\n",state);
	cpu_set_irq_line(GTS80SS_locals.boardData.cpuNo, M6502_INT_IRQ, state ? ASSERT_LINE : CLEAR_LINE);
}

void GTS80SS_nmi(int state)
{
	if ( !GTS80SS_locals.NMIState && state ) {
//		logerror("NMI: %i\n",state);
/*		at last Devils Dare isn't working if the NMI is really fired */
/*		cpu_set_irq_line(GTS80SS_locals.boardData.cpuNo, M6502_INT_NMI, PULSE_LINE); */
	}
	GTS80SS_locals.NMIState = state;
}

UINT8 RIOT6532_3_RAM[256];

READ_HANDLER( riot6532_3_ram_r )
{
    return RIOT6532_3_RAM[offset&0x7f];
}

WRITE_HANDLER( riot6532_3_ram_w )
{
	RIOT6532_3_RAM[offset&0x7f] = data;
}

WRITE_HANDLER(riot3a_w) { logerror("riot3a_w: 0x%02x\n", data);}

/* Switch settings, test switch and NMI */
READ_HANDLER(riot3b_r)  {
	// 0x40: test switch SW1
	return (GTS80SS_locals.NMIState?0x00:0x80) | 0x40 | (GTS80SS_locals.dips^0x3f);
}

WRITE_HANDLER(riot3b_w) { logerror("riot3b_w: 0x%02x\n", data);}

/* D/A converters */
WRITE_HANDLER(da1_latch_w) {
//	logerror("da1_w: 0x%02x\n", data);
	if ( GTS80SS_locals.buf_pos>=GTS80SS_BUFFER_SIZE )
		return;

	GTS80SS_locals.clock[GTS80SS_locals.buf_pos] = timer_get_time();
	GTS80SS_locals.buffer[GTS80SS_locals.buf_pos++] = (0x80-data)*0xf0;
}

WRITE_HANDLER(da2_latch_w) {
/*	logerror("da2_w: 0x%02x\n", data); */
}

/* expansion board */
READ_HANDLER(ext_board_1_r) {
	logerror("ext_board_1_r\n"); 
	return 0xff;
}

WRITE_HANDLER(ext_board_1_w) {
	logerror("ext_board_1_w: 0x%02x\n", data);
}


READ_HANDLER(ext_board_2_r) {
	logerror("ext_board_2_r\n"); 
	return 0xff;
}

WRITE_HANDLER(ext_board_2_w) {
	logerror("ext_board_2_w: 0x%02x\n", data);
}


READ_HANDLER(ext_board_3_r) {
	logerror("ext_board_3_r\n"); 
	return 0xff;
}

WRITE_HANDLER(ext_board_3_w) {
	logerror("ext_board_3_w: 0x%02x\n", data);
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

static const int PhonemeDurationMS[65] =
{
  59,  71, 121,  47,  47,  71, 103,  90,  
  71,  55,  80, 121, 103,  80,  71,  71,
  71, 121,  71, 146, 121, 146, 103, 185,
  103, 80,  47,  71,  71, 103,  55,  90,
  185, 65,  80,  47, 250, 103, 185, 185,
  185, 103, 71,  90, 185,  80, 185, 103,
   90, 71, 103, 185,  80, 121,  59,  90,
   80, 71, 146, 185, 121, 250, 185,  47
};

void GTS80SS_speachtimeout(int state)
{
//	logerror("votrax timer timeout\n");
	GTS80SS_locals.timer = 0;
	GTS80SS_nmi(1);
}

/* voice synt latch */
WRITE_HANDLER(vs_latch_w) {
	static int queue[100],pos;

	votrax_w(data^0xff);
	data = (data^0xff) & 0x3f;
	if ( pos<100 )
		queue[pos++] = data;

//	logerror("Votrax: intonation %d, phoneme %02x %s\n",data >> 6,data & 0x3f,PhonemeTable[data & 0x3f]);

	if ( data==0x3f ) {
		if ( pos>1 ) {
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

	/* start working and set a timer when output is done */
	GTS80SS_nmi(0);
	if ( GTS80SS_locals.timer ) {
		timer_remove(GTS80SS_locals.timer);
		GTS80SS_locals.timer = 0;
	}
	GTS80SS_locals.timer = timer_set(TIME_IN_USEC(PhonemeDurationMS[data]*1000),1,GTS80SS_speachtimeout);
}

struct riot6532_interface GTS80SS_riot6532_intf = {
 /* 6532RIOT 3: Sound/Speech board Chip U15 */
 /* in  : A/B, */ NULL, riot3b_r,
 /* out : A/B, */ riot3a_w, riot3b_w,
 /* irq :      */ GTS80SS_irq
};

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80SS_readmem)
{ 0x0000, 0x01ff, riot6532_3_ram_r},
{ 0x0200, 0x027f, riot6532_3_r},
{ 0x4000, 0x4fff, ext_board_1_r},
{ 0x5000, 0x5fff, ext_board_2_r},
{ 0x6000, 0x6fff, ext_board_3_r},
{ 0x7000, 0x7fff, MRA_ROM},
{ 0x8000, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80SS_writemem)
{ 0x0000, 0x01ff, riot6532_3_ram_w},
{ 0x0200, 0x027f, riot6532_3_w},
{ 0x1000, 0x1fff, da1_latch_w},
{ 0x2000, 0x2fff, vs_latch_w},
{ 0x3000, 0x3fff, da2_latch_w},
{ 0x4000, 0x4fff, ext_board_1_w},
{ 0x5000, 0x5fff, ext_board_2_w},
{ 0x6000, 0x6fff, ext_board_3_w},
{ 0x7000, 0x7fff, MWA_ROM},
{ 0x8000, 0xfdff, MWA_ROM},
{ 0xff00, 0xffff, MWA_RAM},
MEMORY_END

WRITE_HANDLER(gts80ss_data_w)
{
	data = (data&0x3f);

//	logerror("sound_latch: 0x%02x\n", data);
	riot6532_set_input_a(3, (data&0x0f?0x80:0x00) | 0x40 | data);
}

static void GTS80_ss_Update(int num, INT16 *buffer, int length)
{
	double dActClock, dInterval, dCurrentClock;
	int i;
		
	dCurrentClock = GTS80SS_locals.clock[0];

	dActClock = timer_get_time();
	dInterval = (dActClock-GTS80SS_locals.clock[0]) / length;

	if ( GTS80SS_locals.buf_pos>1 )
		GTS80SS_locals.buf_pos = GTS80SS_locals.buf_pos;

	i = 0;
	GTS80SS_locals.clock[GTS80SS_locals.buf_pos] = 9e99;
	while ( length ) {
		*buffer++ = GTS80SS_locals.buffer[i];
		length--;
		dCurrentClock += dInterval;

		while ( (GTS80SS_locals.clock[i+1]<=dCurrentClock) )
			i++;
	}

	GTS80SS_locals.clock[0] = dActClock;
	GTS80SS_locals.buffer[0] = GTS80SS_locals.buffer[GTS80SS_locals.buf_pos-1];
	GTS80SS_locals.buf_pos = 1;
}

void gts80ss_init(struct sndbrdData *brdData) {
	int i;

	memset(&GTS80SS_locals, 0x00, sizeof GTS80SS_locals);
	GTS80SS_locals.boardData = *brdData;

	/* int dips */
	GTS80SS_locals.dips =
		  ((core_getDip(4)&0x01) ? 0x08:0x00) /* S1: Self-Test */
/*		| ((core_getDip(4)&0x02) ? 0x00:0x00)    S2: not used (goes to the expansion board, pin J1-1) */
		| ((core_getDip(4)&0x04) ? 0x20:0x00) /* S3: speech in attrach mode (0) */
		| ((core_getDip(4)&0x08) ? 0x10:0x00) /* S4: speech in attrach mode (1) */
		| ((core_getDip(4)&0x10) ? 0x04:0x00) /* S5: background music enabled */
		| ((core_getDip(4)&0x20) ? 0x02:0x00) /* S6: speech enabled */
		| ((core_getDip(4)&0x40) ? 0x01:0x00) /* S7: connected, usage unknown */
/*		| ((core_getDip(4)&0x80) ? 0x08:0x00)    S8: not used (goes to the expansion board, pin J1-I7)*/
    ;


	/* init RAM */
	memset(RIOT6532_3_RAM, 0x00, sizeof RIOT6532_3_RAM);
	
	/* init RIOT */
    riot6532_config(3, &GTS80SS_riot6532_intf);
	riot6532_set_clock(3, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);

	GTS80SS_locals.clock[0]  = 0;
	GTS80SS_locals.buffer[0] = 0;
	GTS80SS_locals.buf_pos   = 1;

	for(i = 0; i<8; i++)
		memcpy(memory_region(GTS80SS_locals.boardData.cpuNo)+0x8000+0x1000*i, memory_region(GTS80SS_locals.boardData.cpuNo)+0x7000, 0x1000);

	GTS80SS_nmi(1);
	GTS80SS_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80_ss_Update); 
	set_RC_filter(GTS80SS_locals.stream, 270000, 15000, 0, 10000);
	sh_votrax_start(mixer_allocate_channel(15));
}

void gts80ss_exit(int boardNo)
{
	if ( GTS80SS_locals.timer ) {
		timer_remove(GTS80SS_locals.timer);
		GTS80SS_locals.timer = 0;
	}
	sh_votrax_stop();
}

const struct sndbrdIntf gts80ssIntf = {
  gts80ss_init, gts80ss_exit, NULL, gts80ss_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

/*----------------------------------------
/ Gottlieb Sys 80B Sound Board
/-----------------------------------------*/

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    ay_latch;			//Data Latch to AY-8913 chips
  int    ym2151_port;		//Bit determines if Registor or Data is written to YM2151
  int    nmi_rate;			//Programmable NMI rate
  void   *nmi_timer;		//Timer for NMI
  UINT8  dac_volume;
  UINT8  dac_data;
} GTS80BS_locals;

// Latch data for AY chips
WRITE_HANDLER(s80bs_ay8910_latch_w) { GTS80BS_locals.ay_latch = data;}

// Init 
void gts80b_init(struct sndbrdData *brdData) {
	GTS80BS_locals.nmi_timer = NULL; 
	memset(&GTS80BS_locals, 0, sizeof(GTS80BS_locals));
}

// Cleanup
void gts80b_exit(int boardNo)
{
	if(GTS80BS_locals.nmi_timer)
		timer_remove(GTS80BS_locals.nmi_timer);
	GTS80BS_locals.nmi_timer = NULL;
}


//Setup NMI timer and triggering code: Timed NMI occurs for the Y-CPU. Y-CPU triggers D-CPU NMI
void nmi_generate(int param) { cpu_cause_interrupt(1,M6502_INT_NMI); }
static void nmi_callback(int param) { cpu_cause_interrupt(cpu_gettotalcpu()-1, M6502_INT_NMI); }
WRITE_HANDLER(s80bs_nmi_rate_w) { GTS80BS_locals.nmi_rate = data; }
WRITE_HANDLER(s80bs_cause_dac_nmi_w) { cpu_cause_interrupt(cpu_gettotalcpu()-2, M6502_INT_NMI); }
READ_HANDLER(s80bs_cause_dac_nmi_r) { s80bs_cause_dac_nmi_w(offset, 0); return 0; }

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(s80bs_sh_w)
{
	data &= 0x3f;			//Not sure if this is needed for ALL generations.
	if ((data&0x0f) != 0xf) /* interrupt trigered by four low bits (not all 1's) */
	{
		soundlatch_w(offset,data);
		cpu_cause_interrupt(cpu_gettotalcpu()-1,M6502_INT_IRQ);
		cpu_cause_interrupt(cpu_gettotalcpu()-2,M6502_INT_IRQ);
	}
}

//Generation 1 Specific
READ_HANDLER(s80bs1_sound_input_r)
{
	/* bits 0-3 are probably unused (future expansion) */
	/* bits 4 & 5 are two dip switches. Unused? */
	/* bit 6 is the test switch. When 0, the CPU plays a pulsing tone. */
	/* bit 7 comes from the speech chip DATA REQUEST pin */
	return 0xc0;
}

//Common to All Generations - Set NMI Timer parameters
static WRITE_HANDLER( common_sound_control_w )
{
	/* Bit 0 enables and starts NMI timer */
	if (GTS80BS_locals.nmi_timer)
	{
		timer_remove(GTS80BS_locals.nmi_timer);
		GTS80BS_locals.nmi_timer = 0;
	}

	if (data & 0x01)
	{
		/* base clock is 250kHz divided by 256 */
		double interval = TIME_IN_HZ(250000.0/256/(256-GTS80BS_locals.nmi_rate));
		GTS80BS_locals.nmi_timer = timer_pulse(interval, 0, nmi_callback);
	}

	/* Bit 1 controls a LED on the sound board */
}

//Generation 1 sound control
WRITE_HANDLER( s80bs1_sound_control_w )
{
	static int last;

	common_sound_control_w(offset, data);

	/* bit 2 goes to 8913 BDIR pin  */
	if ((last & 0x04) == 0x04 && (data & 0x04) == 0x00)
	{
		/* bit 3 selects which of the two 8913 to enable */
		if (data & 0x08)
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_0_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_0_w(0,GTS80BS_locals.ay_latch);
		}
		else
		{
			/* bit 4 goes to the 8913 BC1 pin */
			if (data & 0x10)
				AY8910_control_port_1_w(0,GTS80BS_locals.ay_latch);
			else
				AY8910_write_port_1_w(0,GTS80BS_locals.ay_latch);
		}
	}

	/* bit 5 goes to the speech chip DIRECT DATA TEST pin */

	/* bit 6 = speech chip DATA PRESENT pin; high then low to make the chip read data */
	if ((last & 0x40) == 0x40 && (data & 0x40) == 0x00)
	{
	}

	/* bit 7 goes to the speech chip RESET pin */

	last = data & 0x44;
}


//Generation 3 sound control
WRITE_HANDLER( s80bs3_sound_control_w )
{
	common_sound_control_w(offset, data);
	/* Bit 7 selects YM2151 register or data port */
	GTS80BS_locals.ym2151_port = data & 0x80;
}

//Determine whether to write data to YM2151 Registers or Data Port
WRITE_HANDLER( s80bs_ym2151_w )
{
	if (GTS80BS_locals.ym2151_port)
		YM2151_data_port_0_w(offset, data);
	else
		YM2151_register_port_0_w(offset, data);
}

//DAC Handling.. Set volume
WRITE_HANDLER( s80bs_dac_vol_w )
{
	GTS80BS_locals.dac_volume = data ^ 0xff;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
}
//DAC Handling.. Set data to send
WRITE_HANDLER( s80bs_dac_data_w )
{
	GTS80BS_locals.dac_data = data;
	DAC_data_16_w(0, GTS80BS_locals.dac_volume * GTS80BS_locals.dac_data);
}

//Process command from Main CPU
WRITE_HANDLER(gts80b_data_w)
{
//	logerror("Sound Command %x\n",data);
	s80bs_sh_w(0,data^0xff);	/*Data is inverted from main cpu*/
}

/***************************/
/* GENERATION 1 MEMORY MAP */
/***************************/
//Y-CPU
MEMORY_READ_START( GTS80BS1_readmem )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x6000, 0x6000, s80bs1_sound_input_r },
	{ 0xa800, 0xa800, soundlatch_r },
	{ 0xc000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x2000, 0x2000, MWA_NOP },	/* speech chip. The game sends strings */
									/* of 15 bytes (clocked by 4000). The chip also */
									/* checks a DATA REQUEST bit in 6000. */
	{ 0x4000, 0x4000, s80bs1_sound_control_w },
	{ 0x8000, 0x8000, s80bs_ay8910_latch_w },
	{ 0xa000, 0xa000, s80bs_nmi_rate_w },	   /* set Y-CPU NMI rate */
	{ 0xb000, 0xb000, s80bs_cause_dac_nmi_w }, /*Trigger D-CPU NMI*/
	{ 0xc000, 0xffff, MWA_ROM },
MEMORY_END
//D-CPU
MEMORY_READ_START( GTS80BS1_readmem2 )
	{ 0x0000, 0x03ff, MRA_RAM },
	{ 0x8000, 0x8000, soundlatch_r },
	{ 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START( GTS80BS1_writemem2 )
	{ 0x0000, 0x03ff, MWA_RAM },
	{ 0x4000, 0x4001, DAC_0_data_w },	/*Not sure if this shouldn't use s80bs_dac_vol_w & s80bs_dac_data_w*/
	{ 0xe000, 0xffff, MWA_ROM },
MEMORY_END
/***************************/
/* GENERATION 2 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x2000, 0x2000, MWA_NOP },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x8000, 0x8000, s80bs_ay8910_latch_w},
{ 0xa000, 0xa000, s80bs1_sound_control_w},
{ 0xa001,0xffff, MWA_ROM},
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS2_readmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS2_writemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

/***************************/
/* GENERATION 3 MEMORY MAP */
/***************************/

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_readmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_writemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0xa000, 0xa000, s80bs3_sound_control_w },
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS80BS3_readmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS80BS3_writemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

const struct sndbrdIntf gts80bIntf = {
  gts80b_init, gts80b_exit, NULL, gts80b_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};


/*----------------
/ Sound interface
/-----------------*/
struct DACinterface GTS80BS_dacInt =
{ 
  2,			/*2 Chips - but it seems we only access 1?*/
 {25,25}		/* Volume */
};

struct AY8910interface GTS80BS_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 25, 25 }, /* Volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

struct YM2151interface GTS80BS_ym2151Int =
{
	1,			/* 1 chip */
	4000000,	/* 4 MHz */
	{ YM3012_VOL(50,MIXER_PAN_LEFT,50,MIXER_PAN_RIGHT) },
	{ 0 }
};
