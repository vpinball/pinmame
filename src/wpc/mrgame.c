/************************************************************************************************
  Mr. Game (Italy)
  ----------------
  by Steve Ellenoff (08/23/2004)
  
  Thanks to Gerrit for helping out with Solenoid smoothing (better now) and getting those damn matrix
  out of the way of the video display..


  Main CPU Board:

  CPU: Motorola M68000
  Clock: 6 Mhz
  Interrupt: Tied to a Fixed System Timer
  I/O: DMA

  Issues/Todo:
  #0) Z80 WAIT line is involved in the real schematic somehow, maybe this needs to be emulated?
  #1) Adding NMI pulse seems to have helped, but still something is quite wrong
  #2) Watching output of video commands from cpu->video, motor cross definitely loses some commands..
      (this occurs after drag race graphic)
  #3) Have not figured out how object ram assigns colors to different areas of the screen
  #4) There are sprites - these are not handled yet!
  #5) I can see screen scroll registers, so the screen must scroll somehow..
  #6) Sound not done yet
  #7) Mr. Game Logo not correct color in Motor Show ( Bad Color Prom Read? )
************************************************************************************************/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "core.h"
#include "sim.h"

//use this to comment out video and sound cpu for quicker debugging of main cpu
//#define TEST_MAIN_CPU
#define NOSOUND

#define MRGAME_DISPLAYSMOOTH 2
#define MRGAME_SOLSMOOTH 4
#define MRGAME_CPUFREQ 6000000

//Jumper on board shows 200Hz hard wired ( should double check it's actually 200Hz and not a divide by 200)
#define MRGAME_IRQ_FREQ TIME_IN_HZ(200)

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
  int vid_data;
  int vid_strb;
  int vid_a11;
  int vid_a12;
  int vid_a13;
} locals;

data8_t *mrgame_videoram;
data8_t *mrgame_objectram;

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

  //setup IRQ timer for Main CPU
  timer_pulse(MRGAME_IRQ_FREQ,0,mrgame_irq);

  //pull video registers out of ram space
  install_mem_write_handler(1,0x6800, 0x68ff, vid_registers_w);
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
static WRITE16_HANDLER(video_w) { locals.vid_data = (data>>8) & 0xff; } //printf("viddata=%x\n",data>>8);}

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
		//S0-S5 = CN14 (Not Used!)
		//S6-S7 = CN14A 
		case 0:
			locals.vid_strb = (data>>7);
			//LOG(("S0-S7 = %04x\n",data));
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
static UINT16 *NVRAM;
static NVRAM_HANDLER(mrgame_nvram) {
  core_nvram(file, read_or_write, NVRAM, 0x10000, 0x00);
}

/***************************************************************************/
/************************** VIDEO HANDLING *********************************/
/***************************************************************************/

//Read D0-D7 from cpu
static READ_HANDLER(i8255_porta_r) { 
	//LOG(("i8255_porta_r=%x\n",locals.vid_data)); 

	//title
	//if(locals.vid_data == 0x18) locals.vid_data = 0x13;

	//Racecars
    //if(locals.vid_data == 0x18) locals.vid_data = 0x1a;

	//Bikes
    //if(locals.vid_data == 0x18) locals.vid_data = 0x19;
	
	//printf("i8255_porta_r=%x\n",locals.vid_data);

	return locals.vid_data; 
}
static READ_HANDLER(i8255_portb_r) { LOG(("UNDOCUMENTED: i8255_portb_r\n")); return 0; }

//Bits 0-3 = Video Dips (NOT INVERTED)
//Bits   4 = Video Strobe from CPU
static int lastr = 0;
READ_HANDLER(i8255_portc_r) { 
	int data = core_getDip(1) | (locals.vid_strb<<4);
#if 0
	if(lastr != data) {
		LOG(("i8255_portc_r=%x\n",data));
		lastr = data;
	}
#endif
	return data;
}

static WRITE_HANDLER(i8255_porta_w) { LOG(("i8255_porta_w=%x\n",data)); }

static WRITE_HANDLER(i8255_portb_w) {
	//cpu_interrupt_enable(1,data&1);
	//LOG(("i8255_portb_w=%x\n",data)); 
}
static WRITE_HANDLER(i8255_portc_w) { LOG(("i8255_portc_w=%x\n",data)); }

static WRITE_HANDLER(vid_registers_w) {
	switch(offset) {
		//Graphics rom - address line 11 pin
		case 0:
			locals.vid_a11 = data & 1;
			break;
		//?
		case 1:
			//cpu_interrupt_enable(1,data&1);
 			break;
		//Graphics rom - address line 12 pin
		case 3:
			locals.vid_a12 = data & 1;
			break;
		//Graphics rom - address line 13 pin
		case 4:
			locals.vid_a13 = data & 1;
			break;
	}
	LOG(("vid_register[%02x]_w=%x\n",offset,data)); 
}

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

static VIDEO_START(mrgame) {
  tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
  return (tmpbitmap == 0);
}

static int charoff = 0;
PINMAME_VIDEO_UPDATE(mrgame_update) {
	int offs = 0;
	int color = 0;
	int tile = 0;

#ifdef MAME_DEBUG

if(!debugger_focus) {
  if(keyboard_pressed_memory_repeat(KEYCODE_Z,2))
	  charoff+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_X,2))
	  charoff-=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_C,2))
	  charoff++;
  if(keyboard_pressed_memory_repeat(KEYCODE_V,2))
	  charoff--;
}
#endif

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < videoram_size - 1; offs++)
	{
		if (1) //dirtybuffer[offs])
		{
			int sx,sy;

//			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			//this works, but is wrong, as the object ram can vary the color in several places.
#if 1
			color = mrgame_objectram[1];
#else
			//This might be correct, but i'm not sure..
			if(offs%2==0)
				color = mrgame_objectram[(offs%32)+1];
			else
				color = mrgame_objectram[offs%32];
#endif

			tile = mrgame_videoram[offs]+
                   (locals.vid_a11*0x100)+(locals.vid_a12*0x200)+(locals.vid_a13*0x400)+charoff;

			drawgfx(tmpbitmap,Machine->gfx[0],
					tile,
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
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
MEMORY_END
static MEMORY_WRITE_START(videog1_writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x47ff, MWA_RAM },
  { 0x4800, 0x4be7, MWA_RAM, &mrgame_videoram, &videoram_size },
  { 0x4be8, 0x4fff, MWA_RAM },
  { 0x5000, 0x50ff, MWA_RAM, &mrgame_objectram },
  { 0x5100, 0x7fff, MWA_RAM },
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
  { 0x8800, 0x8be7, MWA_RAM, &mrgame_videoram, &videoram_size },
  { 0x8bef, 0x8fff, MWA_RAM },
  { 0x9000, 0x90ff, MWA_RAM, &mrgame_objectram },
  { 0x9100, 0xbfff, MWA_RAM },
  { 0xc000, 0xc003, ppi8255_0_w},
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

PALETTE_INIT( mrgame )
{
	int i;
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;
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

static struct GfxLayout charlayout_g2 =
{
	8,8,						/* 8*8 characters */
	4096,						/* 4096 characters = (32768 Bytes / 8 bits per byte)  */
	5,							/* 5 bits per pixel */
	{ 0, 0x8000*8*1, 0x8000*8*2, 0x8000*8*3, 0x8000*8*4},		/* the bitplanes are separated across the 5 roms*/
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo_g1[] =
{
	{ REGION_GFX1, 0, &charlayout_g1,   0, 32 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo gfxdecodeinfo_g2[] =
{
	{ REGION_GFX1, 0, &charlayout_g2,   0, 32 },
	{ -1 } /* end of array */
};


/* VIDEO GENERATION 1 DRIVER */
MACHINE_DRIVER_START(mrgame_vid1)
  MDRV_CPU_ADD(Z80,18432000/6)	/* 3.072 MHz */
  MDRV_CPU_MEMORY(videog1_readmem, videog1_writemem)
  MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)
  MDRV_FRAMES_PER_SECOND(60)
  MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
MACHINE_DRIVER_END

/* VIDEO GENERATION 2 DRIVER */
MACHINE_DRIVER_START(mrgame_vid2)
  MDRV_CPU_ADD(Z80, 3000000)	/*3 Mhz?*/
  MDRV_CPU_MEMORY(videog2_readmem, videog2_writemem)
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
#ifndef NOSOUND
  MDRV_IMPORT_FROM(mrgame_snd1)
#endif
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* Gen 2 - Vid & Sound */
MACHINE_DRIVER_START(mrgame_vidsnd_g2)
  MDRV_IMPORT_FROM(mrgame_vid2)
#ifndef NOSOUND
  MDRV_IMPORT_FROM(mrgame_snd1)
#endif
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
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
  MDRV_PALETTE_LENGTH(32)
  MDRV_PALETTE_INIT(mrgame)
  MDRV_VIDEO_START(mrgame)
MACHINE_DRIVER_END


//Generation 1 
MACHINE_DRIVER_START(mrgame1)
	MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
	MDRV_IMPORT_FROM(mrgame_vidsnd_g1)
#endif
	MDRV_IMPORT_FROM(mrgame_video_common)
	MDRV_GFXDECODE(gfxdecodeinfo_g1)
MACHINE_DRIVER_END

//Generation 2
MACHINE_DRIVER_START(mrgame2)
  MDRV_IMPORT_FROM(mrgame_cpu)
#ifndef TEST_MAIN_CPU
  MDRV_IMPORT_FROM(mrgame_vidsnd_g2)
#endif
  MDRV_IMPORT_FROM(mrgame_video_common)
  MDRV_GFXDECODE(gfxdecodeinfo_g2)
MACHINE_DRIVER_END
