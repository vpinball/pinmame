#include "driver.h"
#include "memory.h"
#include "cpu/m6502/m6502.h"
#include "machine/6530riot.h"
#include "machine/6532riot.h"
#include "sound/2151intf.h"
#include "sound/dac.h"
#include "sound/votrax.h"
#include "core.h"
#include "sndbrd.h"
#include "snd_cmd.h"

#include "gts80.h"
#include "gts80s.h"
#include "gts3.h"

/* 
    Gottlieb System 80 Sound Boards
   
    - System 80/80A Sound Board 

    - System 80/80A Sound & Speech Board

    - System 80A Sound Board with a PiggyPack installed
	  (thanks goes to Peter Hall for providing some very usefull information)
   
    - System 80B Sound Board (3 generations)

	- System 3 sound boards

*/


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound Board
/-----------------------------------------*/

#define GTS80S_BUFFER_SIZE 8192

static struct {
	struct sndbrdData boardData;

	int    stream;
	INT16  buffer[GTS80S_BUFFER_SIZE+1];
	double clock[GTS80S_BUFFER_SIZE+1];
	int    buf_pos;

	int    dips;
	UINT8* pRIOT6530_0_ram;
	int	   IRQEnabled;
} GTS80S_locals;

/* digital sound data output */
static WRITE_HANDLER(gts80s_riot6530_0a_w) {
//	logerror("riot6530_0a_w: 0x%02x\n", data);
	if ( GTS80S_locals.buf_pos>=GTS80S_BUFFER_SIZE )
		return;

	GTS80S_locals.clock[GTS80S_locals.buf_pos] = timer_get_time();
	GTS80S_locals.buffer[GTS80S_locals.buf_pos++] = ((data<<7)-0x4000)*2;
}

static WRITE_HANDLER(gts80s_riot6530_0b_w) { 
//	logerror("riot6530_0b_w: 0x%02x\n", data);
	/* reset the interupt on the PiggyPack board */
	if ( GTS80S_locals.boardData.subType==1 )
		GTS80S_locals.IRQEnabled = data&0x40;
}

static struct riot6530_interface GTS80S_riot6530_intf = {
 /* 6530RIOT 0 (0x200) Chip U2 */
 /* PA0 - PA7 Digital sound data */
 /* PB0 - PB7 Sound latch & Dip-Switches */
 /* in  : A/B, */ NULL, NULL,
 /* out : A/B, */ gts80s_riot6530_0a_w, gts80s_riot6530_0b_w,
 /* irq :      */ NULL
};

static WRITE_HANDLER(gts80s_riot6530_0_ram_w)
{
	UINT8 *pMem = GTS80S_locals.pRIOT6530_0_ram + (offset%0x40);

	pMem[0x0000] = pMem[0x0040] = pMem[0x0080] = pMem[0x00c0] = \
	pMem[0x0100] = pMem[0x0140] = pMem[0x0180] = pMem[0x01c0] = \
	pMem[0x1000] = pMem[0x1040] = pMem[0x1080] = pMem[0x10c0] = \
	data;
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80S_readmem)
{ 0x0000, 0x01ff, MRA_RAM},
{ 0x0200, 0x03ff, riot6530_0_r},
{ 0x0400, 0x0fff, MRA_ROM},
{ 0x1000, 0x10ff, MRA_RAM},  
{ 0xf800, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80S_writemem)
{ 0x0000, 0x01ff, gts80s_riot6530_0_ram_w},
{ 0x0200, 0x03ff, riot6530_0_w},
{ 0x0400, 0x0fff, MWA_ROM},
{ 0x1000, 0x10ff, gts80s_riot6530_0_ram_w},
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
	GTS80S_locals.pRIOT6530_0_ram = memory_region(GTS80S_locals.boardData.cpuNo);

	/* init dips */
	GTS80S_locals.dips =
		((core_getDip(5)&0x01) ? 0x00:0x80) | /* S1: Sound/Tones        */
		((core_getDip(5)&0x02) ? 0x00:0x10);  /* S2: Attrach Mode Tunes */

	/* init sound buffer */
	GTS80S_locals.clock[0]  = 0;
	GTS80S_locals.buffer[0] = 0;
	GTS80S_locals.buf_pos   = 1;

	if ( GTS80S_locals.boardData.subType==0 ) {
		/* clear the upper 4 bits, some ROM images aren't 0 */ 
		/* the 6530 RIOT ROM is not used by the boards which have a PiggyPack installed */
		pMem = memory_region(GTS80S_locals.boardData.cpuNo)+0x0400;
		for(i=0x0400; i<0x0bff; i++)
			*pMem++ = (*pMem&0x0f);

		memcpy(memory_region(GTS80S_locals.boardData.cpuNo)+0x1000, memory_region(GTS80S_locals.boardData.cpuNo)+0x0700, 0x100);
	}


	/* 
		Init RAM, i.e. set base of all bank to the base of bank 1, 
		the memory repeats ever 64 bytes; haven't found another way to 
		tell the MAME core this situation; the problem is that the cpu *will*
		execute code in this area, so a usually read/writer handler fails
	*/
	for (i=1;i<=12;i++)
		cpu_setbank(i, memory_region(STATIC_BANK1));

	/* init the RIOT */
    riot6530_config(0, &GTS80S_riot6530_intf);
	riot6530_set_clock(0, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);
	riot6530_reset();

	GTS80S_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80S_Update);
	set_RC_filter(GTS80S_locals.stream, 270000, 15000, 0, 33000);
}

/*--------------
/  exit
/---------------*/

void gts80s_exit(int boardNo)
{
	riot6530_unconfig();
}

/*--------------
/  sound cmd
/---------------*/

static WRITE_HANDLER(gts80s_data_w)
{
//	logerror("sound latch: 0x%02x\n", data);
	data &= 0x0f;
	riot6530_set_input_b(0, GTS80S_locals.dips | 0x20 | data);

	/* the PiggyPack board is really firing an interrupt; sound board layout
	   for theses game is different, because the IRQ line is connected */
	if ( (GTS80S_locals.boardData.subType==1) && data && GTS80S_locals.IRQEnabled )
		cpu_set_irq_line(GTS80S_locals.boardData.cpuNo, 0, PULSE_LINE);
}

/* only in at the moment for the pinmame startup information */
struct CustomSound_interface GTS80S_customsoundinterface = {
	NULL, NULL, NULL
};

const struct sndbrdIntf gts80sIntf = {
	gts80s_init, gts80s_exit, NULL, gts80s_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gts80s_s)
  MDRV_CPU_ADD_TAG("scpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80S_readmem, GTS80S_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, GTS80S_customsoundinterface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END


/*----------------------------------------
/ Gottlieb Sys 80/80A Sound Board
/-----------------------------------------*/

#define GTS80SS_BUFFER_SIZE 4096

static struct {
	struct sndbrdData boardData;

	int    dips;
	UINT8* pRIOT6532_3_ram;

	int    stream;
	INT16  buffer[GTS80SS_BUFFER_SIZE+1];
	double clock[GTS80SS_BUFFER_SIZE+1];
	int	   buf_pos;

	void   *timer;
} GTS80SS_locals;

static void GTS80SS_irq(int state) {
//	logerror("IRQ: %i\n",state);
	cpu_set_irq_line(GTS80SS_locals.boardData.cpuNo, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static void GTS80SS_nmi(int state)
{
	// logerror("NMI: %i\n",state);
	if ( !state )
		cpu_set_nmi_line(GTS80SS_locals.boardData.cpuNo, PULSE_LINE);
}

static WRITE_HANDLER(GTS80SS_riot3a_w) { logerror("riot3a_w: 0x%02x\n", data);}

/* Switch settings, test switch and NMI */
READ_HANDLER(GTS80SS_riot3b_r)  {
	// 0x40: test switch SW1
	return (votraxsc01_status_r(0)?0x80:0x00) | 0x40 | (GTS80SS_locals.dips^0x3f);
}

static WRITE_HANDLER(GTS80SS_riot3b_w) { logerror("riot3b_w: 0x%02x\n", data);}

/* D/A converters */
static WRITE_HANDLER(GTS80SS_da1_latch_w) {
//	logerror("da1_w: 0x%02x\n", data);

	if ( GTS80SS_locals.buf_pos>=GTS80SS_BUFFER_SIZE )
		return;

	GTS80SS_locals.clock[GTS80SS_locals.buf_pos] = timer_get_time();
	GTS80SS_locals.buffer[GTS80SS_locals.buf_pos++] = ((data<<7)-0x4000)*2;
}

static WRITE_HANDLER(GTS80SS_da2_latch_w) {
//	logerror("da2_w: 0x%02x\n", data); 
}

/* expansion board */
static READ_HANDLER(GTS80SS_ext_board_1_r) {
	logerror("ext_board_1_r\n"); 
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_1_w) {
	logerror("ext_board_1_w: 0x%02x\n", data);
}

static READ_HANDLER(GTS80SS_ext_board_2_r) {
	logerror("ext_board_2_r\n"); 
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_2_w) {
	logerror("ext_board_2_w: 0x%02x\n", data);
}

static READ_HANDLER(GTS80SS_ext_board_3_r) {
	logerror("ext_board_3_r\n"); 
	return 0xff;
}

static WRITE_HANDLER(GTS80SS_ext_board_3_w) {
	logerror("ext_board_3_w: 0x%02x\n", data);
}

/* voice synt latch */
static WRITE_HANDLER(GTS80SS_vs_latch_w) {
	votraxsc01_w(0, data^0xff);
}

static struct riot6532_interface GTS80SS_riot6532_intf = {
 /* 6532RIOT 3: Sound/Speech board Chip U15 */
 /* in  : A/B, */ NULL, GTS80SS_riot3b_r,
 /* out : A/B, */ GTS80SS_riot3a_w, GTS80SS_riot3b_w,
 /* irq :      */ GTS80SS_irq
};

static WRITE_HANDLER(GTS80SS_riot6532_3_ram_w)
{
	UINT8 *pMem = GTS80SS_locals.pRIOT6532_3_ram + (offset%0x80);

	pMem[0x0000] = pMem[0x0080] = pMem[0x0100] = pMem[0x0180] = data;
}

/*--------------
/  Memory map
/---------------*/
MEMORY_READ_START(GTS80SS_readmem)
{ 0x0000, 0x01ff, MRA_RAM}, 
{ 0x0200, 0x027f, riot6532_3_r},
{ 0x4000, 0x4fff, GTS80SS_ext_board_1_r},
{ 0x5000, 0x5fff, GTS80SS_ext_board_2_r},
{ 0x6000, 0x6fff, GTS80SS_ext_board_3_r},
{ 0x7000, 0x7fff, MRA_ROM},
{ 0x8000, 0xffff, MRA_ROM},
MEMORY_END

MEMORY_WRITE_START(GTS80SS_writemem)
{ 0x0000, 0x01ff, GTS80SS_riot6532_3_ram_w}, 
{ 0x0200, 0x027f, riot6532_3_w},
{ 0x1000, 0x1fff, GTS80SS_da1_latch_w},
{ 0x2000, 0x2fff, GTS80SS_vs_latch_w},
{ 0x3000, 0x3fff, GTS80SS_da2_latch_w},
{ 0x4000, 0x4fff, GTS80SS_ext_board_1_w},
{ 0x5000, 0x5fff, GTS80SS_ext_board_2_w},
{ 0x6000, 0x6fff, GTS80SS_ext_board_3_w},
{ 0x7000, 0xffff, MWA_NOP}, // the soundboard does fake writes to the ROM area (used for a delay function)
MEMORY_END

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

/*--------------
/  init
/---------------*/

void gts80ss_init(struct sndbrdData *brdData) {
	int i;

	memset(&GTS80SS_locals, 0x00, sizeof GTS80SS_locals);
	GTS80SS_locals.boardData = *brdData;

	GTS80SS_locals.pRIOT6532_3_ram = memory_region(GTS80SS_locals.boardData.cpuNo);

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

	/* 
		Init RAM, i.e. set base of all banks to the base of bank 1, 
		the memory repeats ever 128 bytes; haven't found another way to 
		tell the MAME core this situation; the problem is that the cpu *may*
		execute code in this area, so a usually read/writer handler fails
	*/
	for (i=1;i<=4;i++)
		cpu_setbank(i, memory_region(STATIC_BANK1));

	/* init RIOT */
    riot6532_config(3, &GTS80SS_riot6532_intf);
	riot6532_set_clock(3, Machine->drv->cpu[GTS80S_locals.boardData.cpuNo].cpu_clock);

	GTS80SS_locals.clock[0]  = 0;
	GTS80SS_locals.buffer[0] = 0;
	GTS80SS_locals.buf_pos   = 1;

	for(i = 0; i<8; i++)
		memcpy(memory_region(GTS80SS_locals.boardData.cpuNo)+0x8000+0x1000*i, memory_region(GTS80SS_locals.boardData.cpuNo)+0x7000, 0x1000);

	GTS80SS_locals.stream = stream_init("SND DAC", 100, 11025, 0, GTS80_ss_Update); 
	set_RC_filter(GTS80SS_locals.stream, 270000, 15000, 0, 10000);
}

/*--------------
/  exit
/---------------*/

void gts80ss_exit(int boardNo)
{
	if ( GTS80SS_locals.timer ) {
		timer_remove(GTS80SS_locals.timer);
		GTS80SS_locals.timer = 0;
	}
}

/*--------------
/  sound cmd
/---------------*/

WRITE_HANDLER(gts80ss_data_w)
{
	data = (data&0x3f);

//	logerror("sound_latch: 0x%02x\n", data);
	riot6532_set_input_a(3, (data&0x0f?0x80:0x00) | 0x40 | data);
}

/* only in at the moment for the pinmame startup information */
struct CustomSound_interface GTS80SS_customsoundinterface = {
	NULL, NULL, NULL
};

struct VOTRAXSC01interface GTS80SS_votrax_sc01_interface = {															
	1,						/* 1 chip */
	{ 100 },				/* master volume */
	{ 7000 },				/* dynamically changing this is currently not supported */
	{ &GTS80SS_nmi }		/* set NMI when busy signal get's low */
};

const struct sndbrdIntf gts80ssIntf = {
  gts80ss_init, gts80ss_exit, NULL, gts80ss_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gts80s_ss)
  MDRV_CPU_ADD_TAG("scpu", M6502, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80SS_readmem, GTS80SS_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, GTS80SS_customsoundinterface)
  MDRV_SOUND_ADD(VOTRAXSC01, GTS80SS_votrax_sc01_interface)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

/*----------------------------------------
/ Gottlieb Sys 80B Sound Board
/-----------------------------------------*/

/*----------------
/  Local varibles
/-----------------*/
static struct {
  int    ay_latch;			// Data Latch to AY-8913 chips
  int    ym2151_port;		// Bit determines if Registor or Data is written to YM2151
  int    nmi_rate;			// Programmable NMI rate
  void   *nmi_timer;		// Timer for NMI
  UINT8  dac_volume;
  UINT8  dac_data;

  int adpcm;
  int enable_cs;
  int u3;
  int enable_w;
  int rom_cs;
} GTS80BS_locals;

// Latch data for AY chips
WRITE_HANDLER(s80bs_ay8910_latch_w)
{
	GTS80BS_locals.ay_latch = data;
}

//Setup NMI timer and triggering code: Timed NMI occurs for the Y-CPU. Y-CPU triggers D-CPU NMI
void nmi_generate(int param)
{
	cpu_set_nmi_line(1, PULSE_LINE);
}

static void nmi_callback(int param)
{
	cpu_set_nmi_line(cpu_gettotalcpu()-1, PULSE_LINE);
}

WRITE_HANDLER(s80bs_nmi_rate_w)
{
	GTS80BS_locals.nmi_rate = data;
}

WRITE_HANDLER(s80bs_cause_dac_nmi_w)
{
	cpu_set_nmi_line(cpu_gettotalcpu()-2, PULSE_LINE);
}

READ_HANDLER(s80bs_cause_dac_nmi_r)
{
	s80bs_cause_dac_nmi_w(offset, 0); 
	return 0; 
}

//Latch a command into the Sound Latch and generate the IRQ interrupts
WRITE_HANDLER(s80bs_sh_w)
{
	data &= 0x3f;			//Not sure if this is needed for ALL generations.
	if ((data&0x0f) != 0xf) /* interrupt trigered by four low bits (not all 1's) */
	{
		soundlatch_w(offset,data);
		cpu_set_irq_line(cpu_gettotalcpu()-1, 0, PULSE_LINE);
		cpu_set_irq_line(cpu_gettotalcpu()-2, 0, PULSE_LINE);
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
		GTS80BS_locals.nmi_timer = timer_alloc(nmi_callback);

		timer_adjust(GTS80BS_locals.nmi_timer, interval, 0, interval);
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
	snd_cmd_log(data^0xff);
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

/* System 3 Sound:

   Far as I know there is only 1 generation.

   Hardware is almost the same as Generation 3 System 80b boards, except for the OKI chip.

   CPU: 2x(6502): DAC: 2x(AD7528): DSP: 1x(YM2151): OTHER: OKI6295 (Speech)
*/   

/*--------------
/  Memory map
/---------------
  CPU #1 (Y-CPU):
  ---------------

  S2 - LS139
  A11 A12
  -------
  0    0 = (0x6000) = Y0 = S5 LS374 Chip Select
  1    0 = (0x6800) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x7000) = Y2 = CPU #2 - Trigger NMI
  1    1 = (0x7800) = Y3 = S4-13 = Latch Data to 6295
  
  T4 - F138
  A13 A14 A15
  -----------
    0   0   0 = Y0 = RAM Enable (0x1fff)
    0   1   0 = Y2 = S4-14 = 2151 Enable  (0x4000)
	1   1   0 = Y3 = S2-LS139 Enable(0x6000)
	1   0   1 = Y5 = Enable G3-LS377 (0xA000)

  CPU #2 (D-CPU):
  ---------------

  S2 - LS139
  A14 A15
  -------
  0    0 = (<0x4000) = Y0 = RAM Enable
  1    0 = (0x4000) = Y1 = A4-LS74 - Clear IRQ & Enable Latch
  0    1 = (0x8000) = Y2 = Enable DAC (E2 - AD7528)
  
*/

// Send data to 6295, but only if Chip Selected and Write Enabled!
static void oki6295_w(void)
{
	if ( GTS80BS_locals.enable_cs && GTS80BS_locals.enable_w ) {
		OKIM6295_data_0_w(0, GTS80BS_locals.adpcm);
		logerror("OKI Data = %x\n", GTS80BS_locals.adpcm);
	}
	else {
		logerror("NO OKI: cs=%x w=%x\n", GTS80BS_locals.enable_cs, GTS80BS_locals.enable_w);
	}
}

static WRITE_HANDLER(adpcm_w)
{
	GTS80BS_locals.adpcm = data^0xff;	//Data is inverted from cpu
	logerror("adpcm: %x\n", GTS80BS_locals.adpcm);
	if ( GTS80BS_locals.adpcm&0x80 )
		logerror("START OF SAMPLE!\n");
	//Trigger a command to 6295
	oki6295_w();
}

/* G3 - LS377
   ----------
D0 = Enable CPU #1 NMI - In conjunction with programmable timer circuit
D1 = CPU #1 Diag LED
D2-D4 = NA?
D5 = S4-11 = DCLCK2 = Clock in 6295 Data from Latch to U3 Latch
D6 = S4-12 = ~WR = 6295 Write Enabled
D7 = S4-15 = YM2151 - A0

   U3 - LS374
   ----------
D0 = VUP/DOWN??
D1 = VSTEP??
D2 = 6295 Chip Select (Active Low)
D3 = ROM Select (Active Low)
D4 = 6295 - SS (Data = 1 = 8Khz; Data = 0 = 6.4Khz frequency)
D5 = LED
D6 = SRET1
D7 = SRET2
*/
static WRITE_HANDLER(sound_control_w)
{
	int hold_enable_w, hold_enable_cs, hold_rom_cs, hold_u3;
	hold_enable_w = GTS80BS_locals.enable_w;
	hold_enable_cs = GTS80BS_locals.enable_cs;
	hold_rom_cs = GTS80BS_locals.rom_cs;
	hold_u3 = GTS80BS_locals.u3;

	//Proc the YM2151 & Other Stuff
	s80bs3_sound_control_w(offset,data);
	// UpdateSoundLEDS(0,(data>>1)&1);

	//Is 6295 Enabled for Writing(Active Low)?
	if (data & 0x40) GTS80BS_locals.enable_w = 0; else GTS80BS_locals.enable_w = 1;
	// logerror("Enable W = %x\n", GTS80BS_locals.enable_w);

	//Trigger Command on Positive Edge
	if (GTS80BS_locals.enable_w && !hold_enable_w) oki6295_w();

	//Handle U3 Latch - On Positive Edge
	GTS80BS_locals.u3 = (data & 0x20);
	if( GTS80BS_locals.u3 && !hold_u3 ) {
		// UpdateSoundLEDS(1,(GTS80BS_locals.adpcm>>5)&1);
		OKIM6295_set_frequency(0,((GTS80BS_locals.adpcm>>4)&1)?8000:6400);
		if (GTS80BS_locals.adpcm & 0x04) GTS80BS_locals.enable_cs = 0; else GTS80BS_locals.enable_cs = 1;
		//Select either Arom1 or Arom2 when Rom Select Changes
		GTS80BS_locals.rom_cs = (GTS80BS_locals.adpcm>>3)&1;
		if (GTS80BS_locals.rom_cs != hold_rom_cs )
			OKIM6295_set_bank_base(0, 0x40000-(GTS80BS_locals.rom_cs*0x40000));

		logerror("SS = %x\n",(GTS80BS_locals.adpcm>>4)&1);
		logerror("ROM CS = %x\n",GTS80BS_locals.rom_cs);
		logerror("Enable CS = %x\n",GTS80BS_locals.enable_cs);

		//Trigger Command on Positive Edge of 6295 Chip Enabled
		if (GTS80BS_locals.enable_cs && !hold_enable_cs) oki6295_w();
	}
}

/*********/
/* Y-CPU */
/*********/
MEMORY_READ_START(GTS3_sreadmem)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x6800, 0x6800, soundlatch_r},
{ 0x7000, 0x7000, s80bs_cause_dac_nmi_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_swritemem)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x4000, 0x4000, s80bs_ym2151_w },
{ 0x6000, 0x6000, s80bs_nmi_rate_w},
{ 0x7800, 0x7800, adpcm_w},
{ 0xa000, 0xa000, sound_control_w },
MEMORY_END
/*********/
/* D-CPU */
/*********/
MEMORY_READ_START(GTS3_sreadmem2)
{ 0x0000, 0x07ff, MRA_RAM },
{ 0x4000, 0x4000, soundlatch_r},
{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(GTS3_swritemem2)
{ 0x0000, 0x07ff, MWA_RAM },
{ 0x8000, 0x8000, s80bs_dac_vol_w },
{ 0x8001, 0x8001, s80bs_dac_data_w},
MEMORY_END

struct OKIM6295interface GTS3_okim6295_interface = {															
	1,						/* 1 chip */
	{ 8000 },				/* 8000Hz frequency */
	{ GTS3_MEMREG_SROM1 },	/* memory region */
	{ 100 }
};

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

const struct sndbrdIntf gts80bIntf = {
  gts80b_init, gts80b_exit, NULL, gts80b_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gts80s_b1)
  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS1_readmem2, GTS80BS1_writemem2)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)

  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS1_readmem, GTS80BS1_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(AY8910, GTS80BS_ay8910Int)

  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80s_b2)
  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS2_readmem2, GTS80BS2_writemem2)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)

  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS2_readmem, GTS80BS2_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(AY8910, GTS80BS_ay8910Int)

  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80s_b3)
  MDRV_CPU_ADD_TAG("d-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS3_readmem2, GTS80BS3_writemem2)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)

  MDRV_CPU_ADD_TAG("y-cpu", M6502, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS80BS3_readmem, GTS80BS3_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(YM2151, GTS80BS_ym2151Int)

  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80s_s3)
  MDRV_CPU_ADD_TAG("d-cpu", M65C02, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_sreadmem2, GTS3_swritemem2)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, GTS80BS_dacInt)

  MDRV_CPU_ADD_TAG("y-cpu", M65C02, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(GTS3_sreadmem, GTS3_swritemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(YM2151,  GTS80BS_ym2151Int)
  MDRV_SOUND_ADD(OKIM6295,GTS3_okim6295_interface)

  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END
