/************************************************************************************************
  Mr. Game (Italy)
  ----------------
  by Steve Ellenoff (08/23/2004)

  Main CPU Board:

  CPU: Motorola M68000
  Clock: 6 Mhz
  Interrupt: Tied to a Fixed System Timer
  I/O: DMA

  Issues/Todo:
  #1) Sound & Video are 90% not done yet..
  #2) Video commands to video board are treated as raw ascii and sent to pinmame display for debugging (remove when real video emulation done)
************************************************************************************************/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "core.h"
#include "sim.h"

//use this to comment out video and sound cpu for quicker debugging of main cpu
#define TEST_MAIN_CPU

#define MRGAME_DISPLAYSMOOTH 2
#define MRGAME_SOLSMOOTH 4
#define MRGAME_CPUFREQ 6000000

//Jumper on board shows 200Hz hard wired ( should double check it's actually 200Hz and not a divide by 200)
#define MRGAME_IRQ_FREQ TIME_IN_HZ(200)

#if 1
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

//Declarations
READ_HANDLER(i8255_porta_r);
READ_HANDLER(i8255_portb_r);
READ_HANDLER(i8255_portc_r);
WRITE_HANDLER(i8255_porta_w);
WRITE_HANDLER(i8255_portb_w);
WRITE_HANDLER(i8255_portc_w);

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
  int row_pin5;
  int a0a2;
  int d0d1;
} locals;

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

#if 0
/* MSM6585 ADPCM CHIP INTERFACE */
static struct MSM5205interface SPINB_msm6585Int = {
	2,										//# of chips
	640000,									//640Khz Clock Frequency
	{SPINB_S1_msmIrq, SPINB_S2_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B, MSM5205_S48_4B},		//Sample Mode
	{100,75}								//Volume
};
/* Sound board */
const struct sndbrdIntf spinbIntf = {
   "SPINB", NULL, NULL, NULL, spinb_sndCmd_w, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
};
#endif


//Generate a level 1 IRQ - IP0,IP1,IP2 = 0
static void mrgame_irq(int data)
{
  cpu_set_irq_line(0, MC68000_IRQ_1, PULSE_LINE);
}

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));

#if 1
  coreGlobals.solenoids = locals.solenoids;
//  locals.solenoids = coreGlobals.pulsedSolState;
  if ((locals.vblankCount % MRGAME_SOLSMOOTH) == 0) {
    locals.solenoids = 0;
  }
#else
  coreGlobals.solenoids = coreGlobals.pulsedSolState;
#endif

  /*-- display --*/
  if ((locals.vblankCount % MRGAME_DISPLAYSMOOTH) == 0) {
	/*update leds*/
	coreGlobals.diagnosticLed = locals.diagnosticLED;
  }

  core_updateSw(1);
}

static SWITCH_UPDATE(mrgame) {
  if (inports) {
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[CORE_COREINPORT] & 0x007f);		//Column 1 Switches
	  coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xf9) | (inports[CORE_COREINPORT] & 0x0180)>>6;  //Column 2 Switches
  }
}

static MACHINE_INIT(mrgame) {
  memset(&locals, 0, sizeof(locals));

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  //setup IRQ timer
  timer_pulse(MRGAME_IRQ_FREQ,0,mrgame_irq);
}


//Reads current switch column (really row) - Inverted
static READ16_HANDLER(col_r) {
	UINT8 switches = coreGlobals.swMatrix[locals.SwCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
	return switches^0xff;
}

/*
Return Dip Switches & Sound Ack (Inverted)
Bits 0-3 Dips (Inverted)
Bits 4   Ack Sound
Bits 5   Ack Spk
Bits 6-7 (Always 0?)
*/
static READ16_HANDLER(rsw_ack_r) {
    int data = core_getDip(0) & 0x0f;
	data |= (locals.acksnd << 4);
	data |= (locals.ackspk << 5);
	return data^0x3f;	//Bit 6&7 always 0!
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	int sol = locals.a0a2;				//Controls which individual solenoid we're updating
	int msk = (~(1<<sol)) & 0xff;		//Mask it off..
	data &= msk;
	data |= (locals.d0d1&1)<<sol;

	switch(offset){
		case 0:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			locals.solenoids |= data;
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
			locals.solenoids |= data << 8;
            break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
			locals.solenoids |= data << 16;
            break;
		case 3:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
			locals.solenoids |= data << 24;
			break;
		default:
			LOG(("Solenoid_W Logic Error\n"));
	}
}

static WRITE16_HANDLER(sound_w) {
	//LOG(("%08x: sound_w = %04x\n",activecpu_get_pc(),data));
}

//8 bit data to this latch comes from D8-D15 (ie, upper bits only)
//TEMP HACK TO DISPLAY SOME TEXT ON SCREEN
static int xpos = 0x20;
static int ypos = 0;
static WRITE16_HANDLER(video_w) {
	char tmp[2];
	int mdata = (data>>8)^0xff;
	//LOG(("%08x: video_w = %04x (%c)\n",activecpu_get_pc(),mdata,mdata));
#if 0
	if(mdata == 0xef) printf("\n");
	else	printf("%c",mdata);
#else
	if(mdata == 0xef) {
		xpos = 0x20;
		ypos = (ypos + 10);
		if(ypos > 350) {
			fillbitmap(Machine->scrbitmap, get_black_pen(), NULL);
			schedule_full_refresh();
			ypos = 0;
		}
	}
	else {
	xpos = (xpos + 8);
	if(xpos > 250) xpos = 0x20;
	sprintf(tmp,"%c",mdata);
	core_textOut(tmp, 1,xpos , ypos, 5);
	}
#endif
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
	int sol = locals.a0a2;
	int lmpmsk = (~(1<<sol)) & 0xff;

	switch(bank) {
		//IC01 - Data Bits 3 = 0, 4 = 0 -> S0-S7 of CN14 & CN14A - Bit 7 is sent to S0-S7 via D0-D2
		case 0:
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
					locals.LampCol = output-3;
					coreGlobals.tmpLampMatrix[locals.LampCol] &= lmpmsk;
					coreGlobals.tmpLampMatrix[locals.LampCol] |= (locals.d0d1&1)<<sol;
			}
		break;

		//IC37 - Data Bits 3 = 0, 4 = 1 -> S20,21,22,23,24 of CN12 (D0-D2 generate 0-7 bits)
		//LAMP COLUMN 6,7,8,9,10
		case 2:
			//bits 5,6,7 not connected
			if(output < 5) {
				locals.LampCol = 5+output;
				coreGlobals.tmpLampMatrix[locals.LampCol] &= lmpmsk;
				coreGlobals.tmpLampMatrix[locals.LampCol] |= (locals.d0d1&1)<<sol;
			}
		break;
	}
}

/*
Bits 0 - Bit 1 = D0-D1 to CN12 (i/o) AND CN14 (video)
Bits 2 - Bit 7 = D2-D7 to CN14 (video board)
*/
static WRITE16_HANDLER(data_w) {
	locals.d0d1 = data & 0x03;
#if 0
	if(locals.d0d1 > 0)
		LOG(("* * * %08x: data_w = %04x, aoa2 = %x\n",activecpu_get_pc(),data,locals.a0a2));
//	else
//		LOG(("%08x: data_w = %04x\n",activecpu_get_pc(),data));
#endif
}

/*
Bits 0-2 = A0-A2 of CN12 & CN14
Bits 3-7 = NC
*/
static WRITE16_HANDLER(extadd_w) {
	locals.a0a2 = data & 0x07;
#if 0
	if(locals.d0d1)
		LOG(("%08x: extadd_w = %04x\n",activecpu_get_pc(),data));
#endif
}

/*
Bit 0 - Bit 2 => Rows 0->7
Bit 3 = RFSH Line
Bit 4 = LED
Bit 5 = Pin 5 - CN10 & Pin 18 - CN11 (??)
Bit 6 = NA
Bit 7 = /RUNEN Line
*/
static WRITE16_HANDLER(row_w) {
	locals.SwCol = core_BitColToNum(data & 0x7);
	locals.diagnosticLED = GET_BIT4;
	locals.row_pin5 = GET_BIT5;
//	LOG(("%08x: row_w = %04x\n",activecpu_get_pc(),data));
}

//NVRAM
//static UINT16 *NVRAM;
static NVRAM_HANDLER(mrgame_nvram) {
  //core_nvram(file, read_or_write, NVRAM, 0x2000, 0x00);
}

/***************************************************************************/
/************************** VIDEO HANDLING *********************************/
/***************************************************************************/
READ_HANDLER(i8255_porta_r) { LOG(("i8255_porta_r\n")); return 0; }
READ_HANDLER(i8255_portb_r) { LOG(("i8255_portb_r\n")); return 0; }
READ_HANDLER(i8255_portc_r) { LOG(("i8255_portc_r\n")); return 0; }
WRITE_HANDLER(i8255_porta_w) { LOG(("i8255_porta_w=%x\n",data)); }
WRITE_HANDLER(i8255_portb_w) { LOG(("i8255_portb_w=%x\n",data)); }
WRITE_HANDLER(i8255_portc_w) { LOG(("i8255_portc_w=%x\n",data)); }

static READ_HANDLER(soundg1_1_port_r) {
	return 0;
}
static WRITE_HANDLER(soundg1_1_port_w) {
}
static READ_HANDLER(soundg1_2_port_r) {
	return 0;
}
static WRITE_HANDLER(soundg1_2_port_w) {
}
static READ_HANDLER(videog1_port_r) {
	return 0;
}
static WRITE_HANDLER(videog1_port_w) {
}
static READ_HANDLER(videog2_port_r) {
	return 0;
}
static WRITE_HANDLER(videog2_port_w) {
}

static VIDEO_START(mrgame) {
  tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
  return (tmpbitmap == 0);
}

static WRITE_HANDLER(vram_w) {
}

PINMAME_VIDEO_UPDATE(mrgame_update) {
//  copybitmap(bitmap,tmpbitmap,0,0,0,0,&GTS80locals.vidClip,TRANSPARENCY_NONE,0);
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
  { 0x020000, 0x02ffff, MWA16_RAM },
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
  //{ 0xa800, 0xa805, misc_r},
MEMORY_END
static MEMORY_WRITE_START(videog1_writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x7fff, MWA_RAM },
  { 0x8100, 0x8103, ppi8255_0_w},
  //{ 0xa800, 0xa805, misc_w},
MEMORY_END
static PORT_READ_START(videog1_readport)
  { 0x00, 0xff, videog1_port_r },
MEMORY_END
static PORT_WRITE_START(videog1_writeport)
  { 0x00, 0xff, videog1_port_w },
MEMORY_END

/*******************************/
/* Video CPU Gen #2 Memory Map */
/*******************************/
static MEMORY_READ_START(videog2_readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0xbfff, MRA_RAM },
  { 0xc000, 0xc003, ppi8255_0_r},
  //{ 0xa800, 0xa805, misc_r},
MEMORY_END
static MEMORY_WRITE_START(videog2_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0xbfff, MWA_RAM },
  { 0xc000, 0xc003, ppi8255_0_w},
  //{ 0xa800, 0xa805, misc_w},
MEMORY_END
static PORT_READ_START(videog2_readport)
  { 0x00, 0xff, videog2_port_r },
MEMORY_END
static PORT_WRITE_START(videog2_writeport)
  { 0x00, 0xff, videog2_port_w },
MEMORY_END
/**********************************/
/* Sound CPU #1 Gen #1 Memory Map */
/**********************************/
static MEMORY_READ_START(soundg1_1_readmem)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0x8000, 0xfaff, MRA_ROM },
  { 0xfb00, 0xffff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START(soundg1_1_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0xfaff, MWA_ROM },
  { 0xfb00, 0xffff, MWA_RAM },
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
  { 0x8000, 0xfaff, MRA_ROM },
  { 0xfb00, 0xffff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START(soundg1_2_writemem)
  { 0x0000, 0x7fff, MWA_ROM },
  { 0x8000, 0xfaff, MWA_ROM },
  { 0xfb00, 0xffff, MWA_RAM },
MEMORY_END
static PORT_READ_START(soundg1_2_readport)
  { 0x00, 0xff, soundg1_2_port_r },
MEMORY_END
static PORT_WRITE_START(soundg1_2_writeport)
  { 0x00, 0xff, soundg1_2_port_w },
MEMORY_END

/* Manual starts with a switch # of 0 */
static int mrgame_sw2m(int no) { return no+7+1; }
static int mrgame_m2sw(int col, int row) { return col*8+row-7-1; }

/* MAIN CPU DRIVER */


/* VIDEO GENERATION 1 DRIVER */
MACHINE_DRIVER_START(mrgame_vid1)
  MDRV_CPU_ADD(Z80, 3000000)	/*3 Mhz?*/
  MDRV_CPU_MEMORY(videog1_readmem, videog1_writemem)
  MDRV_CPU_PORTS(videog1_readport, videog1_writeport)
MACHINE_DRIVER_END

/* VIDEO GENERATION 2 DRIVER */
MACHINE_DRIVER_START(mrgame_vid2)
  MDRV_CPU_ADD(Z80, 3000000)	/*3 Mhz?*/
  MDRV_CPU_MEMORY(videog2_readmem, videog2_writemem)
  MDRV_CPU_PORTS(videog2_readport, videog2_writeport)
MACHINE_DRIVER_END

/* SOUND GENERATION 1 DRIVER */
MACHINE_DRIVER_START(mrgame_snd1)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg1_1_readmem, soundg1_1_writemem)
  MDRV_CPU_PORTS(soundg1_1_readport, soundg1_1_writeport)
  MDRV_CPU_ADD(Z80, 4000000)	/*4 Mhz*/
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(soundg1_2_readmem, soundg1_2_writemem)
  MDRV_CPU_PORTS(soundg1_2_readport, soundg1_2_writeport)
MACHINE_DRIVER_END

/* Gen 1 - Vid & Sound */
MACHINE_DRIVER_START(mrgame_vidsnd_g1)
  MDRV_IMPORT_FROM(mrgame_vid1)
  MDRV_IMPORT_FROM(mrgame_snd1)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* Gen 2 - Vid & Sound */
MACHINE_DRIVER_START(mrgame_vidsnd_g2)
  MDRV_IMPORT_FROM(mrgame_vid2)
  MDRV_IMPORT_FROM(mrgame_snd1)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* Main CPU - Common among all */
MACHINE_DRIVER_START(mrgame_cpu)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(mrgame, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", M68000, MRGAME_CPUFREQ)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(mrgame)
  MDRV_SWITCH_CONV(mrgame_sw2m,mrgame_m2sw)
  MDRV_DIAGNOSTIC_LEDH(1)
//  MDRV_NVRAM_HANDLER(mrgame_nvram)
MACHINE_DRIVER_END

/* Video Board - Common among all */
MACHINE_DRIVER_START(mrgame_video_common)
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 255, 0, 399)
  MDRV_GFXDECODE(0)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE)
  MDRV_VIDEO_START(mrgame)
MACHINE_DRIVER_END


//Generation 1
MACHINE_DRIVER_START(mrgame1)
	MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
	MDRV_IMPORT_FROM(mrgame_vidsnd_g1)
#endif
	MDRV_IMPORT_FROM(mrgame_video_common)
MACHINE_DRIVER_END

//Generation 2
MACHINE_DRIVER_START(mrgame2)
  MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
  MDRV_IMPORT_FROM(mrgame_vidsnd_g2)
#endif
  MDRV_IMPORT_FROM(mrgame_video_common)
MACHINE_DRIVER_END
