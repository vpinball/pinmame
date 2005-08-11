/************************************************************************************************
  Mr. Game (Italy)
  ----------------
  by Steve Ellenoff (08/23/2004)

  Huge Thanks to Gerrit for:
  a) helping out with Solenoid smoothing (better now)
  b) Fixing switches and enabling the flippers to work
  c) SCROLLING! Tracking why the scrolling was so much off was hell!

 ************************************************************************************************
  Main CPU Board:

  CPU: Motorola M68000
  Clock: 6 Mhz
  Interrupt: Tied to a Fixed System Timer - Hard wired & Jumpered to 200Hz
  I/O: DMA

  Video Board:

  CPU: Z80
  Clock: 3 Mhz
  Interrupt: Some funky timing & control generates an NMI, IRQ doesn't appear to be used
  I/O: 8255
  32 Color Fixed Palette rom
  Generation #1 - 2 roms for characters & sprites ( 2 bits per pixel )
  Generation #2 - 5 roms for characters & sprites ( 5 bits per pixel ) - Manual calls it '32 Color Video Board'

  Sound Board:
  CPU: 2 x Z80
  Clock: 4 Mhz
  Interrupt: IRQ via a timer - hardwired & jumpered to 60Hz, NMI via the 7th bit of the sound command
  Generation #1 - Audio: 3 X DAC (1 used to drive volume), 1 X TMS 5220 Speech Chip, 1 X M114S Digital Wave Table Synth Chip
  Generation #2 - Audio: 3 X DAC (1 used to drive volume), 1 X M114S Digital Wave Table Synth Chip

  Issues/Todo:
  #2) Timing of animations might be too slow..
  #3) M114S Sound chip emulated but needs to be improved for better accuracy
  #4) Generation #2 Video - some corrupt sprites appear on the soccer screen (right hand side)

************************************************************************************************/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "sound/5220intf.h"
#include "sound/m114s.h"
#include "vidhrdw/generic.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "mrgame.h"

//use this to comment out video and sound cpu for quicker debugging of main cpu
//#define NO_M114S
//#define TEST_MAIN_CPU
//#define TEST_MOTORSHOW
//#define NOSOUND
#define DISABLE_INTST

#define FIX_CMOSINIT 1		// fix nvram-initialization

//Define Total # of Mixing Channels Used ( 2 for the DAC, 1 for the TMS5220, and whatever else for the M114S )
#define MRGAME_TOTCHANNELS 3 + M114S_OUTPUT_CHANNELS

#define MRGAME_CPUFREQ 6000000

//not sure where these are defined, but those are the correct values
#define Z80_IRQ 0
#define Z80_NMI 127

//Jumper on board shows 200Hz hard wired - 6Mhz clock feeds 74393 as / 16 -> 4040 to Q11 as / 2048 = ~183Hz
#define MRGAME_IRQ_FREQ (6000000/16)/2048
//Jumper on sound board shows 60Hz hard wired - 4Mhz clock feeds 74393 as /16 -> 4040 to Q12 as / 4096 = ~61Hz
#define MRGAME_SIRQ_FREQ (4000000/16)/4096

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

//Declarations
static READ_HANDLER(i8255_porta_r);
static READ_HANDLER(i8255_portb_r);
static READ_HANDLER(i8255_portc_r);
static WRITE_HANDLER(i8255_porta_w);
static WRITE_HANDLER(i8255_portb_w);
static WRITE_HANDLER(i8255_portc_w);
static WRITE_HANDLER(vid_registers_w);
static WRITE16_HANDLER(sound_w);
static WRITE_HANDLER(mrgame_sndcmd);

static struct {
  int vblankCount;
  UINT32 solenoids;
  UINT16 sols2;
  int irq_count;
  int DispNoWait;
  int SwCol;
  int LampCol;
  int diagnosticLED;
  int acksnd;
  int ackspk;
  int flipRead;
  int a0a2;
  int d0d1;
  int vid_stat;
  int vid_data;
  int vid_strb;
  int vid_a11;
  int vid_a12;
  int vid_a13;
  int vid_a14;
  int sndstb;
  int sndcmd;
  int gameOn;
} locals;

static data8_t *mrgame_videoram;
static data8_t *mrgame_objectram;

/* -------------------*/
/* --- Interfaces --- */
/* -------------------*/

/* I8255 CHIPS */
static ppi8255_interface ppi8255_intf =
{
	1,					/* 1 chip */
	{i8255_porta_r},	/* Port A read */
	{i8255_portb_r},	/* Port B read */
	{i8255_portc_r},	/* Port C read */
	{i8255_porta_w},	/* Port A write */
	{i8255_portb_w},	/* Port B write */
	{i8255_portc_w},	/* Port C write */
};

/*----------------
/ Sound interface
/-----------------*/
struct DACinterface mrgame_dacInt =
{
  2,									/*2 Chips - Even though there's 3, only 2 used for actual output */
 { MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) }		/* Volume */
};
struct TMS5220interface mrgame_tms5220Int = { 640000, 100, 0 };
struct M114Sinterface mrgame_m114sInt = {
	1,					/* # of chips */
	{4000000},			/* Clock Frequency 4Mhz */
	{REGION_USER1},		/* ROM Region for samples */
	{100},				/* Volume Level */
	{2},				/* cpu # controlling M114S */
};

/* Sound board */
const struct sndbrdIntf mrgameIntf = {
   "MRGAME", NULL, NULL, NULL, mrgame_sndcmd, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

//Generate a level 1 IRQ - IP0,IP1,IP2 = 0
static INTERRUPT_GEN(mrgame_irq) { cpu_set_irq_line(0, MC68000_IRQ_1, PULSE_LINE); }

//Generate an IRQ on the sound chip's z80
static INTERRUPT_GEN(snd_irq) {
	cpu_set_irq_line(2,Z80_IRQ,PULSE_LINE);
    cpu_set_irq_line(3,Z80_IRQ,PULSE_LINE);
}

//Generate an NMI on the sound chip's z80
static void snd_nmi(int state) {
	cpu_set_irq_line(2,Z80_NMI,PULSE_LINE);
    cpu_set_irq_line(3,Z80_NMI,PULSE_LINE);
}


static INTERRUPT_GEN(vblank) {

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % MRGAME_LAMPSMOOTH) == 0) {
    locals.gameOn = (coreGlobals.tmpLampMatrix[2]>>4) & 1;	// lamp 21 is game on
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % MRGAME_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids | (locals.gameOn<<24);
    locals.solenoids      = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % MRGAME_DISPLAYSMOOTH) == 0) {
	/*update leds*/
	coreGlobals.diagnosticLed = locals.diagnosticLED;
  }

  core_updateSw(locals.gameOn);
}

static SWITCH_UPDATE(mrgame) {
  if (inports) {
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[CORE_COREINPORT] & 0x007f);		//Column 1 Switches
	  coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xf9) | (inports[CORE_COREINPORT] & 0x0180)>>6;  //Column 2 Switches
  }
}

static MACHINE_INIT(mrgame) {

  memset(&locals, 0, sizeof(locals));

  locals.sndstb = 1;
  locals.acksnd = 1;
  locals.ackspk = 1;

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  sndbrd_0_init(core_gameData->hw.soundBoard,   2, memory_region(MRGAME_MEMREG_SND1),NULL,NULL);
}


//Reads current switch column (really row) - Inverted
static READ16_HANDLER(col_r) {
	UINT8 switches;
	// 8th column is feedback byte from video cpu
	if (locals.SwCol==7) return locals.vid_stat;
	// +1 so we begin by reading column 1 of input matrix instead of 0
	// which is used for special switches in many drivers
	switches = coreGlobals.swMatrix[locals.SwCol+1];
	return switches^0xff;
}

/*
Return Dip Switches (Inverted) & Sound Ack
Bits 0-3 Dips (Inverted)
Bits 4   Ack Sound
Bits 5   Ack Spk
Bits 6-7 Opto flipper switches
*/
static READ16_HANDLER(rsw_ack_r) {
    int data = core_getDip(0) ^ 0x0f;
	data |= (locals.acksnd << 4);
	data |= (locals.ackspk << 5);
	data |= (coreGlobals.swMatrix[9] << 6);
	//printf("%08x: rsw_ack_r = %04x\n",activecpu_get_pc(),data);
	return data;
}
/*Solenoids*/
static WRITE_HANDLER(solenoid_w)
{
	if (offset < 4) {
		int    sol = offset*8+locals.a0a2;	// sol #
		UINT32 msk = ~(1<<sol);			// mask this bit
		UINT32 bit = (locals.d0d1&1)<<sol;	// data bit

	        locals.solenoids |=
	        coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & msk) | bit;
	}
	else {
		LOG(("Solenoid_W Logic Error\n"));
	}
}

//Sound Commander Write
static WRITE_HANDLER(mrgame_sndcmd)
{
	//Simulate the 3 commands the real cpu would generate by toggling the 7th Bit
	sound_w(0,data|0x80,0xff);
	sound_w(0,data,0xff);
	sound_w(0,data|0x80,0xff);
}

//Bit 7 of the data triggers NMI of the sound cpus
static WRITE16_HANDLER(sound_w) {
//	LOG(("%08x: sound_w = %04x\n",activecpu_get_pc(),data));

	locals.sndcmd = data & 0xff;

	//Main CPU sends 3 commands, Bit 7 set->cleared->set
	//We'll trigger the NMI on the cleared->set transition
	if(!locals.sndstb && GET_BIT7)
		snd_nmi(0);
	locals.sndstb = GET_BIT7;
}

//8 bit data to this latch comes from D8-D15 (ie, upper bits only)
static WRITE16_HANDLER(video_w) {
	locals.vid_data = (data>>8) & 0xff;
}

static WRITE_HANDLER(lampdata_w) {
	int lmpmsk;
	data = locals.a0a2;
	lmpmsk = (~(1<<data)) & 0xff;
	locals.LampCol = offset;
	coreGlobals.tmpLampMatrix[locals.LampCol] &= lmpmsk;					//Mask off the lamp we're updating
	coreGlobals.tmpLampMatrix[locals.LampCol] |= (locals.d0d1&1)<<data;		//Set it based on d0d1 value
}

/*
Bits 0-2 = D0-D2 selecting 0-7 output of selected bank (see below)
Bits 3-4 = Selects 1 of the 3 banks (s0-7, s9-19, s20-24)
Bits 5-6 = ??
Bits 7   = Controls what value is sent to S0-S7 of CN14 & CN14A

IC01 - Data Bits 3 = 0, 4 = 0 -> S0-S7 of CN14 & CN14A - Bit 7 is sent to S0-S7 via D0-D2
IC36 - Data Bits 3 = 1, 4 = 0 -> S9,10,11,15,16,17,18,19 of CN12 (D0-D2 generate 0-7 bits)
IC37 - Data Bits 3 = 0, 4 = 1 -> S20,21,22,23,24 of CN12         (D0-D2 generate 0-7 bits)

S9,S10,S11 = Solenoid bank 1,2,3 X 8 = 24 Sols
S15-S24 = Lamp Col 1-10 = 80 Lamps
*/
static WRITE16_HANDLER(ic35b_w) {
	int output = data & 0x07;
	int bank = (data & 0x18)>>3;
	switch(bank) {
		//IC01 - Data Bits 3 = 0, 4 = 0 -> S0-S7 of CN14 & CN14A - Bit 7 is sent to S0-S7 via D0-D2
		//S0-S5 = CN14 (Not Used!)
		//S6-S7 = CN14A
		case 0:
			locals.vid_strb = (data>>7);
			break;
		//IC36 - Data Bits 3 = 1, 4 = 0 -> S9,10,11,15,16,17,18,19 of CN12 (D0-D2 generate 0-7 bits)
		case 1:
			switch(output) {
				//S9 - Solenoids 1-8
				case 0:
					solenoid_w(0,0);
					break;
				//S10 - Solenoids 9-16
				case 1:
					solenoid_w(1,0);
					break;
				//S11 - Solenoids 17-24
				case 2:
					solenoid_w(2,0);
					break;
				//S15 - S19 (LAMP COLUMN 1,2,3,4,5)
				default:
					lampdata_w(output-3,0);
			}
			break;

		//IC37 - Data Bits 3 = 0, 4 = 1 -> S20,21,22,23,24 of CN12 (D0-D2 generate 0-7 bits)
		//LAMP COLUMN 6,7,8,9,10
		case 2:
			//bits 5,6,7 not connected
			if(output < 5) lampdata_w(5+output,0);
			break;
	}
}

/*
Bits 0 - Bit 1 = D0-D1 to CN12 (i/o) -- AND CN14 (video) - NOT USED!
Bits 2 - Bit 3 = D0-D1 to CN14A (video board) - NOT USED?!
Bits 4 - Bit 7 = D4-D7 to CN14A (video board) - NOT USED?!
*/
static WRITE16_HANDLER(data_w) { locals.d0d1 = data & 0x03; }

/*
Bits 0-2 = A0-A2 of CN12 --- AND CN14 (NOT USED!)
Bits 3-7 = NC
*/
static WRITE16_HANDLER(extadd_w) { locals.a0a2 = data & 0x07; }

/*
Bit 0 - Bit 2 => Rows 0->7
Bit 3 = RFSH Line
Bit 4 = LED
Bit 5 = Pin 5 - CN10 & Pin 18 - CN11 (??)
        CN10-5 = enable flipper buttons readout? but is always 0, strange!
Bit 6 = NA
Bit 7 = /RUNEN Line
*/
static WRITE16_HANDLER(row_w) {
	locals.SwCol = data & 7;
	locals.diagnosticLED = GET_BIT4;
	locals.flipRead = GET_BIT5;
//	LOG(("%08x: row_w = %04x\n",activecpu_get_pc(),data));
}

//NVRAM
static UINT16 *NVRAM;
static NVRAM_HANDLER(mrgame_nvram) {
  core_nvram(file, read_or_write, NVRAM, 0x10000, 0x00);
#if FIX_CMOSINIT
  if (*NVRAM==0) *NVRAM=0xFFFF;
#endif
}

/***************************************************************************/
/************************** VIDEO HANDLING *********************************/
/***************************************************************************/

//Read D0-D7 from cpu
static READ_HANDLER(i8255_porta_r) {
	return locals.vid_data;
}

static READ_HANDLER(i8255_portb_r) { LOG(("UNDOCUMENTED: i8255_portb_r\n")); return 0; }

//Bits 0-3 = Video Dips (NOT INVERTED)
//Bits   4 = Video Strobe from CPU
//static READ_HANDLER(i8255_portc_r) { return core_getDip(1) | (locals.vid_strb<<4); }
static READ_HANDLER(i8255_portc_r) {
	int data = core_getDip(2); // core_getDip(1) only works if the 16 bits of the 1st dip switch row was used.
	int strobe = (locals.vid_strb<<4);
	return  data | strobe;
}

//Connected to monitor! (?)
static WRITE_HANDLER(i8255_portb_w) {
	locals.vid_stat = data;
}

//These don't make sense to me - they're read lines, so no idea what writes to here would do!
static WRITE_HANDLER(i8255_porta_w) {
	//LOG(("i8255_porta_w=%x\n",data));
}
static WRITE_HANDLER(i8255_portc_w) {
	//LOG(("i8255_portc_w=%x\n",data));
}


//Video Registers
static WRITE_HANDLER(vid_registers_w) {
	int bitval = data & 1;
	switch(offset) {
		//Graphics rom - address line 11 pin
		case 0:
			locals.vid_a11 = bitval;
			break;
		//Interrupt Enable or Strobe? INTST/ on schems
		case 1:
#ifndef DISABLE_INTST
			//printf("setting intst/ to value of %x\n",bitval);
			cpu_interrupt_enable(1,bitval);
#endif
			break;
		//Graphics rom - address line 14 pin
		case 2:
			locals.vid_a14 = bitval;
			break;
		//Graphics rom - address line 12 pin
		case 3:
			locals.vid_a12 = bitval;
			break;
		//Graphics rom - address line 13 pin
		case 4:
			locals.vid_a13 = bitval;
			break;

		//?? - POUT2 on schems for Generation #2 board
		case 5:
			break;

		//Not used?
		case 6:
		case 7:
			//printf("vid_register[%02x]_w=%x\n",offset,data);
			break;
	}

#ifdef MAME_DEBUG
	if(offset != 1)
		LOG(("vid_register[%02x]_w=%x\n",offset,data));
#endif

}

/* Sound CPU 1 Ports
   Port 0 (W) - CSD/A1 -> DAC driving an OP AMP to control Volume
   Port 1 (R) - CSINS1 -> Read Main CPU Command
   Port 2 (W) - CSAKL1 -> Set Status line back to Main CPU - Data on D0
   Port 3 (W) - CSSGS  -> Data to M114S Chip - STB on D6
*/
static READ_HANDLER(soundg1_1_port_r) {
	int data = 0;
	switch(offset)
	{
		case 1:
			data = locals.sndcmd;		//Data is inverted
			LOG(("SOUND CPU #1 - Reading data: %02x\n",data));
			break;
		//Should not read at this port, but it does.. The value read is immediately discarded though..
		case 2:
			break;
		default:
			LOG(("%04x: Unhandled port read on Sound CPU #1 - Port %02x\n",activecpu_get_pc(),offset));
	}
	return data;
}

static WRITE_HANDLER(soundg1_1_port_w) {
	switch(offset)
	{
		case 0:
		{
			//Adjust volume on all channels
			int i;
			for(i=0;i<MRGAME_TOTCHANNELS;i++)
				mixer_set_volume(i,data);
			break;
		}
		case 2:
			//Write Status bit for main cpu to read
			locals.ackspk = GET_BIT0;
			break;
		case 3:
#ifndef NO_M114S
			//LOG(("%04x: Data to M114S = %02x\n",activecpu_get_pc(),data));
			M114S_data_w(0,data);
#endif
			break;
		default:
			LOG(("%04x: Unhandled port write on Sound CPU #1 - Port %02x - Data %02x\n",activecpu_get_pc(),offset,data));
	}
}

/* Sound CPU 2 Ports
   Port 0 (W)  - CSD/A2   -> DAC #2
   Port 1 (R)  - CSINS2   -> Read Main CPU Command
   Port 2 (W)  - CSAKL2   -> Set Status line back to Main CPU - Data on D0
   Port 3 (RW) - CSSPEECH -> TMS5220
   Port 4 (W)  - CSD/A3   -> DAC #3
*/
static READ_HANDLER(soundg1_2_port_r) {
	int data = 0;
	switch(offset)
	{
		case 1:
			data = locals.sndcmd;
			LOG(("SOUND CPU #2 - Reading data: %02x\n",data));
			break;
		case 3:
			data = tms5220_status_r(0);
			break;
		default:
			LOG(("Unhandled port read on Sound CPU #2 - Port %02x\n",offset));
	}
	return data;
}

static WRITE_HANDLER(soundg1_2_port_w) {
	switch(offset)
	{
		case 0:
			//Data to DAC #1
			DAC_data_w(0,data);
			break;
		case 2:
			//Write Status bit for main cpu to read
			locals.acksnd = GET_BIT0;
			break;
		case 3:
			//Data to TMS5220
			tms5220_data_w(0,data);
			break;
		case 4:
			//Data to DAC #2
			DAC_data_w(1,data);
			break;
		default:
			LOG(("Unhandled port write on Sound CPU #2 - Port %02x - Data %02x\n",offset,data));
	}
}

/* Sound Generation #2 */

/* Sound CPU 2 Ports
   Port 0 (W)  - CSD/A2   -> DAC #2
   Port 1 (R)  - CSINS2   -> Read Main CPU Command
   Port 2 (W)  - CSAKL2   -> Set Status line back to Main CPU - Data on D0
   Port 3 (W)  - ROM BANK SELECT
   Port 4 (W)  - CSD/A3   -> DAC #3
*/
static READ_HANDLER(soundg2_2_port_r) {
	int data = 0;
	switch(offset)
	{
		case 1:
			data = locals.sndcmd;
			LOG(("SOUND CPU #2 - Reading data: %02x\n",data & 0x7f));
			break;
		case 3:
			LOG(("SOUND CPU #2 - Reading data on Port 3: %02x\n",data));
			break;
		default:
			LOG(("Unhandled port read on Sound CPU #2 - Port %02x\n",offset));
	}
	return data;
}

static WRITE_HANDLER(soundg2_2_port_w) {
	switch(offset)
	{
		case 0:
			//Data to DAC #1
			DAC_data_w(0,data);
			break;
		case 2:
			//Write Status bit for main cpu to read
			locals.acksnd = GET_BIT0;
			break;
		case 3: {
			//SOUND ROM BANKING
			int snd_a15 = GET_BIT0;
			int snd_csrom46 = !(GET_BIT2);
			cpu_setbank(1, memory_region(REGION_USER2) + (snd_a15*0x8000) + (snd_csrom46*0x10000));
			break;
		}
		case 4:
			//Data to DAC #2
			DAC_data_w(1,data);
			break;
		default:
			LOG(("Unhandled port write on Sound CPU #2 - Port %02x - Data %02x\n",offset,data));
	}
}

//Video Setup
static struct mame_bitmap *tmpbitmap2;
static VIDEO_START(mrgame) {
  tmpbitmap =  auto_bitmap_alloc(256, 256);
  tmpbitmap2 = auto_bitmap_alloc(256, 240);
  return (tmpbitmap == 0);
}

#ifdef MAME_DEBUG
static int charoff = 0;
#endif

static const struct rectangle screen_all_area =
{
	0, 255,
	0, 255,
};
static const struct rectangle screen_visible_area =
{
	0, 255,
	8, 247,
};

//Video Update - Generation #1
PINMAME_VIDEO_UPDATE(mrgame_update_g1) {
    static int scrollers[32];
	int offs = 0;
	int color = 0;
	int colorindex = 0;
	int tile = 0;
	int flipx=0;
	int flipy=0;
	int sx=0;
	int sy=0;

#ifdef MAME_DEBUG

if(1 || !debugger_focus) {
if(keyboard_pressed_memory_repeat(KEYCODE_Z,25)) {
#ifdef TEST_MOTORSHOW
	  fake_w(0,0);
#else
	charoff = 0;
	locals.acksnd = !locals.acksnd;
#endif
}
  //core_textOutf(50,20,1,"offset=%08x", charoff);
  if(keyboard_pressed_memory_repeat(KEYCODE_Z,4))
	  charoff+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_X,4))
	  charoff-=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_C,4))
	  charoff++;
  if(keyboard_pressed_memory_repeat(KEYCODE_V,4))
	  charoff--;
}
#endif

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < videoram_size; offs++)
	{
		if (1) //dirtybuffer[offs])
		{
//			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			colorindex = (colorindex+2);
			if(sx==0) colorindex=1;
			color = mrgame_objectram[colorindex];
			scrollers[sx] = -mrgame_objectram[colorindex-1];

			tile = mrgame_videoram[offs]+
                   (locals.vid_a11<<8)+(locals.vid_a12<<9)+(locals.vid_a13<<10)+(locals.vid_a14<<11);

			drawgfx(tmpbitmap,Machine->gfx[0],
					tile,
					color+2,						//+2 to offset from PinMAME palette entries
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
	/* copy the temporary bitmap to the screen with scolling */
	copyscrollbitmap(tmpbitmap2,tmpbitmap,0,0,32,scrollers,&screen_all_area,TRANSPARENCY_NONE,0);

	/* Draw Sprites - Not sure of total size here (this memory size allows 8 sprites on screen at once ) */
	for (offs = 0x40; offs < 0x60; offs += 4)
	{
		sx = mrgame_objectram[offs + 3] + 1;
		sy = 240 - mrgame_objectram[offs];
		flipx = mrgame_objectram[offs + 1] & 0x40;
		flipy = mrgame_objectram[offs + 1] & 0x80;
		tile = (mrgame_objectram[offs + 1] & 0x3f) +
				   (locals.vid_a11<<6) + (locals.vid_a12<<7) + (locals.vid_a13<<8);
		color = mrgame_objectram[offs + 2];	//Note: This byte may have upper bits also used for other things, but no idea what if/any!
		drawgfx(tmpbitmap2,Machine->gfx[1],
				tile,
				color+2,							//+2 to offset from PinMAME palette entries
				flipx,flipy,
				sx,sy,
				0,TRANSPARENCY_PEN,0);
	}
	copybitmap(bitmap,tmpbitmap2,0,0,0,-8,&screen_visible_area,TRANSPARENCY_NONE,0);
    return 0;
}

//Video Update - Generation #2
PINMAME_VIDEO_UPDATE(mrgame_update_g2) {
    static int scrollers[32];
	int offs = 0;
	int colorindex = 0;
	int tile = 0;
	int flipx=0;
	int flipy=0;
	int sx=0;
	int sy=0;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < videoram_size; offs++)
	{
		if (1) //dirtybuffer[offs])
		{
//			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			colorindex = (colorindex+2);
			if(sx==0) colorindex=1;
			scrollers[sx] = -mrgame_objectram[colorindex-1];

			tile = mrgame_videoram[offs]+
                   (locals.vid_a11<<8)+(locals.vid_a12<<9)+(locals.vid_a13<<10)+(locals.vid_a14<<11);
			drawgfx(tmpbitmap,Machine->gfx[0],
					tile,
					0,			//Always color 0 because there's no color data used
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
	/* copy the temporary bitmap to the screen with scolling */
	copyscrollbitmap(tmpbitmap2,tmpbitmap,0,0,32,scrollers,&screen_all_area,TRANSPARENCY_NONE,0);


	/* Draw Sprites - Not sure of total size here (this memory size allows 8 sprites on screen at once ) */
	/* NOTE: We loop backwards in sprite memory so that we draw the last sprites first so overlapping priority is correct */
	for (offs = 0x5f; offs > 0x3F; offs -= 4)
	{
		sx = mrgame_objectram[offs] + 1;
		sy = 240 - mrgame_objectram[offs-3];
		flipx = mrgame_objectram[offs - 2] & 0x40;
		flipy = mrgame_objectram[offs - 2] & 0x80;
		tile = (mrgame_objectram[offs - 2] & 0x3f) +
				   (locals.vid_a11<<6) + (locals.vid_a12<<7) + (locals.vid_a13<<8) + (locals.vid_a14<<9);
		//Draw it
		drawgfx(tmpbitmap2,Machine->gfx[1],
				tile,
				0,			//Always color 0 because there's no color data used
				flipx,flipy,
				sx,sy,
				0,TRANSPARENCY_PEN,0);
	}
	copybitmap(bitmap,tmpbitmap2,0,0,0,-8,&screen_visible_area,TRANSPARENCY_NONE,0);

    return 0;
}


/***********************/
/* Main CPU Memory Map */
/***********************/
static MEMORY_READ16_START(readmem)
  { 0x000000, 0x01ffff, MRA16_ROM },
  { 0x020000, 0x02ffff, MRA16_RAM },
  { 0x030000, 0x030001, rsw_ack_r },
  { 0x03000c, 0x03000d, col_r },
MEMORY_END
static MEMORY_WRITE16_START(writemem)
  { 0x000000, 0x01ffff, MWA16_ROM },
  { 0x020000, 0x02ffff, MWA16_RAM, &NVRAM },
  { 0x030002, 0x030003, sound_w },
  { 0x030004, 0x030005, video_w },
  { 0x030006, 0x030007, ic35b_w },
  { 0x030008, 0x030009, data_w },
  { 0x03000a, 0x03000b, row_w },
  { 0x03000e, 0x03000f, extadd_w },
MEMORY_END
/*******************************/
/* Video CPU Gen #1 Memory Map */
/*******************************/
static MEMORY_READ_START(videog1_readmem)
  { 0x0000, 0x3fff, MRA_ROM },
  { 0x4000, 0x7fff, MRA_RAM },
  { 0x8100, 0x8103, ppi8255_0_r},
MEMORY_END
static MEMORY_WRITE_START(videog1_writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x47ff, MWA_RAM },
  { 0x4800, 0x4bff, MWA_RAM, &mrgame_videoram, &videoram_size },
  { 0x4c00, 0x4fff, MWA_RAM },
  { 0x5000, 0x50ff, MWA_RAM, &mrgame_objectram },
  { 0x5100, 0x67ff, MWA_RAM },
  { 0x6800, 0x68ff, vid_registers_w },
  { 0x6900, 0x7fff, MWA_RAM },
  { 0x8100, 0x8103, ppi8255_0_w},
MEMORY_END

/*******************************/
/* Video CPU Gen #2 Memory Map */
/*******************************/
static MEMORY_READ_START(videog2_readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0xbfff, MRA_RAM },
  { 0xc000, 0xc003, ppi8255_0_r},
MEMORY_END
static MEMORY_WRITE_START(videog2_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0x87ff, MWA_RAM },
  { 0x8800, 0x8bff, MWA_RAM, &mrgame_videoram, &videoram_size },
  { 0x9000, 0x90ff, MWA_RAM, &mrgame_objectram },
  { 0xa800, 0xa8ff, vid_registers_w },
  { 0xc000, 0xc003, ppi8255_0_w},
MEMORY_END

/**********************************/
/* Sound CPU #1 Gen #1 Memory Map */
/**********************************/
static MEMORY_READ_START(soundg1_1_readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0xfbff, MRA_ROM },
  { 0xfc00, 0xffff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START(soundg1_1_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0xfbff, MWA_ROM },
  { 0xfc00, 0xffff, MWA_RAM },
MEMORY_END
static PORT_READ_START(soundg1_1_readport)
  { 0x00, 0xff, soundg1_1_port_r },
MEMORY_END
static PORT_WRITE_START(soundg1_1_writeport)
  { 0x00, 0xff, soundg1_1_port_w },
MEMORY_END
/**********************************/
/* Sound CPU #2 Gen #1 Memory Map */
/**********************************/
static MEMORY_READ_START(soundg1_2_readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0xfbff, MRA_ROM },
  { 0xfc00, 0xffff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START(soundg1_2_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0xfbff, MWA_ROM },
  { 0xfc00, 0xffff, MWA_RAM },
MEMORY_END
static PORT_READ_START(soundg1_2_readport)
  { 0x00, 0xff, soundg1_2_port_r },
MEMORY_END
static PORT_WRITE_START(soundg1_2_writeport)
  { 0x00, 0xff, soundg1_2_port_w },
MEMORY_END
/**********************************/
/* Sound CPU #2 Gen #2 Memory Map */
/**********************************/
static MEMORY_READ_START(soundg2_2_readmem)
  { 0x0000, 0x7bff, MRA_ROM },
  { 0x7c00, 0x7fff, MRA_RAM },
  { 0x8000, 0xfbff, MRA_BANK1 },
  { 0xfc00, 0xffff, MRA_RAM },			//this shouldn't really be ram according to my best guess of schematics, but code writes here and it seems to be ok
MEMORY_END
static MEMORY_WRITE_START(soundg2_2_writemem)
  { 0x0000, 0x7bff, MWA_ROM },
  { 0x7c00, 0x7fff, MWA_RAM },
  { 0x8000, 0xfbff, MWA_BANK1 },
  { 0xfc00, 0xffff, MWA_RAM },			//this shouldn't really be ram according to my best guess of schematics, but code writes here and it seems to be ok
MEMORY_END
static PORT_READ_START(soundg2_2_readport)
  { 0x00, 0xff, soundg2_2_port_r },
MEMORY_END
static PORT_WRITE_START(soundg2_2_writeport)
  { 0x00, 0xff, soundg2_2_port_w },
MEMORY_END

/* Manual starts with a switch # of 0 */
static int mrgame_sw2m(int no) { return no+7+1; }
static int mrgame_m2sw(int col, int row) { return col*8+row-7-1; }

//Video Generation #1 - Palette Init

PALETTE_INIT( mrgame_g1 )
{
	int bit0,bit1,bit2,i,r,g,b;

	//Add in PinMAME colors for the lamp,sw,sol matrix to display
	palette_set_color(0,0,0,0);
	palette_set_color(1,85,85,85);
	palette_set_color(2,170,170,170);
	for (i=3; i < 8; i++)
		palette_set_color(i,255,255,255);

	for (; i < Machine->drv->total_colors; i++)
	{
		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

//Video Generation #2 - Palette Init

PALETTE_INIT( mrgame_g2 )
{
	int bit0,bit1,bit2,i,r,g,b;
	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		palette_set_color(i,r,g,b);
		color_prom++;
	}
}

/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

// ******************************
// *** GENERATION 1 HARDWARE ****
// ******************************
static struct GfxLayout charlayout_g1 =
{
	8,8,						/* 8*8 characters */
	4096,						/* 4096 characters = (32768 Bytes / 8 bits per byte)  */
	2,							/* 2 bits per pixel */
	{ 0, 0x8000*8 },			/* the bitplanes are separated across the 2 roms*/
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout_g1 =
{
	16,16,						/* 16*16 characters */
	4096/4,						/* 4096/4 characters = (32768 Bytes / 8 bits per byte)  */
	2,							/* 2 bits per pixel */
	{ 0, 0x8000*8 },			/* the bitplanes are separated across the 2 roms*/
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8 /* every char takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_g1[] =
{
	{ REGION_GFX1, 0, &charlayout_g1,   0, 32 },
	{ REGION_GFX1, 0, &spritelayout_g1,   0, 32 },
	{ -1 } /* end of array */
};


// ******************************
// *** GENERATION 2 HARDWARE ****
// ******************************

static struct GfxLayout charlayout_g2 =
{
    8,8,						/* 8*8 characters */
    4096,						/* 4096 characters = (32768 Bytes / 8 bits per byte)  */
    5,							/* 5 bits per pixel */
    //3,2,1,5,4 Plane Ordering
    { 0x8000*8*2, 0x8000*8*1, 0x8000*8*0, 0x8000*8*4, 0x8000*8*3 },		/* the bitplanes are separated across the 5 roms */
    { 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
    { 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
    8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout_g2 =
{
	16,16,						/* 16*16 characters */
	4096/4,						/* 4096/4 characters = (32768 Bytes / 8 bits per byte)  */
	5,							/* 5 bits per pixel */
    //3,2,1,5,4 Plane Ordering
    { 0x8000*8*2, 0x8000*8*1, 0x8000*8*0, 0x8000*8*4, 0x8000*8*3 },		/* the bitplanes are separated across the 5 roms */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8 /* every char takes 32 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_g2[] =
{
	{ REGION_GFX1, 0, &charlayout_g2,   0, 1 },
	{ REGION_GFX1, 0, &spritelayout_g2, 0, 1 },
	{ -1 } /* end of array */
};

/* Main CPU - Common among all generations */
MACHINE_DRIVER_START(mrgame_cpu)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(mrgame, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", M68000, MRGAME_CPUFREQ)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_PERIODIC_INT(mrgame_irq, MRGAME_IRQ_FREQ)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_NVRAM_HANDLER(mrgame_nvram)
  MDRV_SWITCH_UPDATE(mrgame)
  MDRV_SWITCH_CONV(mrgame_sw2m,mrgame_m2sw)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

/* Video Board - Common among all generations */
MACHINE_DRIVER_START(mrgame_video_common)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 255, 0, 399)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
  MDRV_VIDEO_START(mrgame)
MACHINE_DRIVER_END

/* VIDEO GENERATION 1 DRIVER */
MACHINE_DRIVER_START(mrgame_vid1)
  MDRV_CPU_ADD(Z80,3000000)		/* 3 MHz */
  MDRV_CPU_MEMORY(videog1_readmem, videog1_writemem)
  MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)
  MDRV_FRAMES_PER_SECOND(60)
  MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
  MDRV_PALETTE_LENGTH(32+8)		//We need extra entries for some pinmame colors
  MDRV_PALETTE_INIT(mrgame_g1)
MACHINE_DRIVER_END

/* VIDEO GENERATION 2 DRIVER */
MACHINE_DRIVER_START(mrgame_vid2)
  MDRV_CPU_ADD(Z80, 3000000)	/*3 Mhz?*/
  MDRV_CPU_MEMORY(videog2_readmem, videog2_writemem)
  MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)
  MDRV_FRAMES_PER_SECOND(60)
  MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
  MDRV_PALETTE_LENGTH(32)
  MDRV_PALETTE_INIT(mrgame_g2)
MACHINE_DRIVER_END

/* SOUND GENERATION 1 DRIVER */
MACHINE_DRIVER_START(mrgame_snd1)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg1_1_readmem, soundg1_1_writemem)
  MDRV_CPU_PORTS(soundg1_1_readport, soundg1_1_writeport)
  MDRV_CPU_PERIODIC_INT(snd_irq, MRGAME_SIRQ_FREQ)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg1_2_readmem, soundg1_2_writemem)
  MDRV_CPU_PORTS(soundg1_2_readport, soundg1_2_writeport)
  MDRV_SOUND_ADD(DAC, mrgame_dacInt)
  MDRV_SOUND_ADD(TMS5220, mrgame_tms5220Int)
  MDRV_SOUND_ADD(M114S, mrgame_m114sInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

/* SOUND GENERATION 2 DRIVER */
MACHINE_DRIVER_START(mrgame_snd2)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg1_1_readmem, soundg1_1_writemem)		//1st Z80 is identical to Gen #1 sound
  MDRV_CPU_PORTS(soundg1_1_readport, soundg1_1_writeport)		//1st Z80 is identical to Gen #1 sound
  MDRV_CPU_PERIODIC_INT(snd_irq, MRGAME_SIRQ_FREQ)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg2_2_readmem, soundg2_2_writemem)
  MDRV_CPU_PORTS(soundg2_2_readport, soundg2_2_writeport)
  MDRV_SOUND_ADD(DAC, mrgame_dacInt)
  MDRV_SOUND_ADD(M114S, mrgame_m114sInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

/* -- Helper Drivers referred to by actual driver definitions below */

/* Gen 1 - Vid & Sound */
MACHINE_DRIVER_START(mrgame_vidsnd_g1)
  MDRV_IMPORT_FROM(mrgame_vid1)
#ifndef NOSOUND
  MDRV_IMPORT_FROM(mrgame_snd1)
#endif
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* Gen 2 - Vid & Gen 1 - Sound */
MACHINE_DRIVER_START(mrgame_vidg2_sndg1)
  MDRV_IMPORT_FROM(mrgame_vid2)
#ifndef NOSOUND
  MDRV_IMPORT_FROM(mrgame_snd1)
#endif
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* Gen 2 - Vid & Sound */
MACHINE_DRIVER_START(mrgame_vidsnd_g2)
  MDRV_IMPORT_FROM(mrgame_vid2)
#ifndef NOSOUND
  MDRV_IMPORT_FROM(mrgame_snd2)
#endif
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END


/* ---------------------------- */
/* Actual Drivers Used are Here */
/* ---------------------------- */

//Generation 1 - Video & Sound
MACHINE_DRIVER_START(mrgame_g1)
	MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
	MDRV_IMPORT_FROM(mrgame_vidsnd_g1)
#endif
	MDRV_IMPORT_FROM(mrgame_video_common)
	MDRV_GFXDECODE(gfxdecodeinfo_g1)
MACHINE_DRIVER_END

//Generation 2 - Video & Generation 1 - Sound
MACHINE_DRIVER_START(mrgame_vg2_sg1)
	MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
	MDRV_IMPORT_FROM(mrgame_vidg2_sndg1)
#endif
	MDRV_IMPORT_FROM(mrgame_video_common)
	MDRV_GFXDECODE(gfxdecodeinfo_g2)
MACHINE_DRIVER_END

//Generation 2 - Video & Sound
MACHINE_DRIVER_START(mrgame_g2)
  MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
  MDRV_IMPORT_FROM(mrgame_vidsnd_g2)
#endif
  MDRV_IMPORT_FROM(mrgame_video_common)
  MDRV_GFXDECODE(gfxdecodeinfo_g2)
MACHINE_DRIVER_END
