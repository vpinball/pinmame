/************************************************************************************************
 Gottlieb System 80 Pinball

 CPU: 6502
 I/O: 6532


 6532:
 ----
 Port A:
 Port B:
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i86/i86.h"
#include "machine/6532riot.h"
#include "sound/votrax.h"
#include "core.h"
#include "sndbrd.h"
#include "gts80.h"
#include "gts80s.h"

static int core_ascii2seg[] = {
	/* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x20-0x27 */ 0x0000, 0x0309, 0x0220, 0x2A4E, 0x2A6D, 0x6E65, 0x135D, 0x0400,
	/* 0x28-0x2f */ 0x1400, 0x4100, 0x7F40, 0x2A40, 0x0000, 0x0840, 0x0000, 0x4400,
	/* 0x30-0x37 */ 0x003f, 0x2200, 0x085B, 0x084f, 0x0866, 0x086D, 0x087D, 0x0007,
	/* 0x38-0x3f */ 0x087F, 0x086F, 0x0009, 0x4001, 0x4408, 0x0848, 0x1108, 0x2803,
	/* 0x40-0x47 */ 0x205F, 0x0877, 0x2A0F, 0x0039, 0x220F, 0x0079, 0x0071, 0x083D,
	/* 0x48-0x4f */ 0x0876, 0x2209, 0x001E, 0x1470, 0x0038, 0x0536, 0x1136, 0x003f,
	/* 0x50-0x57 */ 0x0873, 0x103F, 0x1873, 0x086D, 0x2201, 0x003E, 0x4430, 0x5036,
	/* 0x58-0x5f */ 0x5500, 0x2500, 0x4409, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments;
  int    swColOp;
  int    swRow;
  int    ssEn;
  int    OpSwitchEnable;
  int    disData;
  int    segSel;
  int    seg1;
  int    seg2;
  int    seg3;
  int    data;
  int    segPos1;
  int    segPos2;
  int    sndCmd;
  int    disCmdMode1;
  int    disCmdMode2;
  int	 sound_data;
  int	 video_data; // needed for video hardware
  int    hasVideo; // indicates video hardware present
  UINT8* pRAM;
} GTS80locals;

static void GTS80_irq(int state) {
  cpu_set_irq_line(GTS80_CPU, M6502_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(GTS80_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  int active;
  GTS80locals.vblankCount += 1;
  /*-- lamps --*/
  if ((GTS80locals.vblankCount % GTS80_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, GTS80locals.lampMatrix, sizeof(GTS80locals.lampMatrix));
//    memset(GTS80locals.lampMatrix, 0, sizeof(GTS80locals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((GTS80locals.vblankCount % GTS80_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = GTS80locals.solenoids;
    if (GTS80locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    GTS80locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((GTS80locals.vblankCount % GTS80_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, GTS80locals.segments, sizeof(coreGlobals.segments));
	memset(GTS80locals.segments, 0x00, sizeof GTS80locals.segments);

    /*update leds*/
    coreGlobals.diagnosticLed = 0;
  }
  /* Lamp 0 controls game enable */
  active = coreGlobals.lampMatrix[0] & 1;
  /* Lamp 1 controls tilt on System80 & System80a */
  if (!(core_gameData->gen & (GEN_GTS80B2K|GEN_GTS80B4K|GEN_GTS80B8K)))
	  active = active && !(coreGlobals.lampMatrix[0] & 2);
  core_updateSw(active);
}

/* GTS80 switch numbering, row and column is swapped */
static int GTS80_sw2m(int no) {
	if ( no>=96 )
		return (no/10)*8+(no%10-1);
	else if (no < 0)
		return no + 8;
	else {
		no += 1;
		return (no%10)*8 + no/10;
	}
}

static int GTS80_m2sw(int col, int row) {
	if (col > 9 || (col == 9 && row >= 6))
		return col*8+row;
	else if (col < 1)
		return -9 + row;
	else
		return row*10+col-1;
}
static int GTS80_lamp2m(int no) { return no+8; }
static int GTS80_m2lamp(int col, int row) { return (col-1)*8+row; }

static SWITCH_UPDATE(GTS80) {
	if (inports) {
		coreGlobals.swMatrix[0] = ((inports[GTS80_COMINPORT] & 0xff00)>>8);
		coreGlobals.swMatrix[8] = (coreGlobals.swMatrix[8]&0xc0) | (inports[GTS80_COMINPORT] & 0x003f);
	}

  /*-- slam tilt --*/
	riot6532_set_input_a(1, (core_gameData->gen&GEN_GTS80B4K) ? (core_getSw(GTS80_SWSLAMTILT)?0x80:0x00) : (core_getSw(GTS80_SWSLAMTILT)?0x00:0x80));
}

static SWITCH_UPDATE(GTS80VID) {
	int i;
	for (i=1; i < 8; i++) {
		coreGlobals.swMatrix[i] = (coreGlobals.swMatrix[i] & 0xfc) | (core_getSw(GTS80_m2sw(i, 0)) > 0) | (core_getSw(GTS80_m2sw(i, 1)) > 0) << 1;
	}
	if (!(GTS80locals.vblankCount % 5)) {
		for (i=1; i < 8; i++) {
			core_setSw(GTS80_m2sw(i, 0), 0);
			core_setSw(GTS80_m2sw(i, 1), 0);
		}
	}
	if (inports) {
		coreGlobals.swMatrix[0] = ((inports[GTS80_COMINPORT] & 0xff00)>>8);
		coreGlobals.swMatrix[8] = (coreGlobals.swMatrix[8]&0xc0) | (inports[GTS80_COMINPORT] & 0x003f);
	}

  /*-- slam tilt --*/
	riot6532_set_input_a(1, (core_gameData->gen&GEN_GTS80B4K) ? (core_getSw(GTS80_SWSLAMTILT)?0x80:0x00) : (core_getSw(GTS80_SWSLAMTILT)?0x00:0x80));
	/* i86 NMI line triggers Slam on the video board */
    if (core_getSw(GTS80_SWSLAMTILT)) cpu_set_irq_line(GTS80_VIDCPU, IRQ_LINE_NMI, PULSE_LINE);
}

static WRITE_HANDLER(GTS80_sndCmd_w) {
	// logerror("sound cmd: 0x%02x\n", data);
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	sndbrd_0_data_w(0, data|((GTS80locals.lampMatrix[1]&0x02)?0x10:0x00));
}

static int GTS80_getSwRow(int row) {
	int value = 0;
	int ii;

	for (ii=8; ii>=1; ii--)
		value = (value<<1) | (coreGlobals.swMatrix[ii]&row?0x01:0x00);

	return value;
}

static int revertByte(int value) {
	int retVal = 0;
	int ii;

	for (ii=0; ii<8; ii++) {
		retVal = (retVal<<1) | (value&0x01);
		value >>= 1;
	}

	return retVal;
}

/*---------------
/ Switch reading
/----------------*/
static READ_HANDLER(riot6532_0a_r)  { return (GTS80locals.OpSwitchEnable) ? core_revbyte(core_getDip(GTS80locals.swColOp)) : (GTS80_getSwRow(GTS80locals.swRow)&0xff);}
static WRITE_HANDLER(riot6532_0a_w) { logerror("riot6532_0a_w: 0x%02x\n", data); }

static READ_HANDLER(riot6532_0b_r)  { /* logerror("riot6532_0b_r\n"); */ return 0x7f; }
static WRITE_HANDLER(riot6532_0b_w) { /* logerror("riot6532_0b_w: 0x%02x\n", data); */  GTS80locals.swRow = data; }

/*---------------------------
/  Display
/----------------------------*/

static READ_HANDLER(riot6532_1a_r)  { /* logerror("riot6532_1a_r\n"); */ return core_gameData->gen&GEN_GTS80B4K ? (core_getSw(GTS80_SWSLAMTILT)?0x80:0x00) : (core_getSw(GTS80_SWSLAMTILT)?0x00:0x80); }
static WRITE_HANDLER(riot6532_1a_w)
{
	int segSel = ((data>>4)&0x07);
	if ( core_gameData->gen & (GEN_GTS80B2K|GEN_GTS80B4K|GEN_GTS80B8K) ) {
		if ( (segSel&0x01) && !(GTS80locals.segSel&0x01) )
			GTS80locals.data = (GTS80locals.data&0xf0) | (GTS80locals.disData&0x0f);
		if ( (segSel&0x02) && !(GTS80locals.segSel&0x02) )
			GTS80locals.data = (GTS80locals.data&0x0f) | ((GTS80locals.disData&0x0f)<<4);
	}
	else {
		int strobe = (data&0x0f);

		if ( (segSel&0x01) && !(GTS80locals.segSel&0x01) )
			GTS80locals.seg1 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x10) )
			GTS80locals.seg1 = core_bcd2seg9[0x01];

		if ( (segSel&0x02) && !(GTS80locals.segSel&0x02) )
			GTS80locals.seg2 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x20) )
			GTS80locals.seg2 = core_bcd2seg9[0x01];

		if ( (segSel&0x04) && !(GTS80locals.segSel&0x04) )
			GTS80locals.seg3 = core_bcd2seg9[GTS80locals.disData&0x0f];
		if ( !(GTS80locals.disData&0x40) )
			GTS80locals.seg3 = core_bcd2seg9[0x01];

		if ( strobe<=5 ) {
			if ( GTS80locals.seg1 ) 
				GTS80locals.segments[7-strobe].w = GTS80locals.seg1;
			
			if ( GTS80locals.seg2 ) 
				GTS80locals.segments[20 + 7-strobe].w = GTS80locals.seg2;
			
			if ( GTS80locals.seg3 ) 
				GTS80locals.segments[40 + 7-strobe].w = GTS80locals.seg3;
		}
		else if ( strobe<=11 ) {
			if ( GTS80locals.seg1 ) 
				GTS80locals.segments[21-strobe].w = GTS80locals.seg1;

			if ( GTS80locals.seg2 ) 
				GTS80locals.segments[20 + 21-strobe].w = GTS80locals.seg2;

			if ( GTS80locals.seg3 ) 
				GTS80locals.segments[40 + 21-strobe].w = GTS80locals.seg3;
		}
		else {
			// 7th digits in score display for GTS80a
			if ( GTS80locals.seg1 ) {
				if ( strobe==12 ) 
					GTS80locals.segments[9].w = GTS80locals.seg1;
				else if ( strobe==15 ) 
					GTS80locals.segments[1].w = GTS80locals.seg1;
			}
			if ( GTS80locals.seg2 ) {
				if ( strobe==12 ) 
					GTS80locals.segments[29].w = GTS80locals.seg2;
				else if ( strobe==15 ) 
					GTS80locals.segments[21].w = GTS80locals.seg2;
			}
			if ( GTS80locals.seg3 ) {
				switch ( strobe ) {
				case 12:
					GTS80locals.segments[8].w = GTS80locals.seg3;
					break;

				case 13:
					GTS80locals.segments[0].w = GTS80locals.seg3;
					break;

				case 14:
					GTS80locals.segments[28].w = GTS80locals.seg3;
					break;

				case 15:
					GTS80locals.segments[20].w = GTS80locals.seg3;
					break;
				}
			}
		}
	}

	GTS80locals.segSel = segSel;
	return;
}

static READ_HANDLER(riot6532_1b_r)  { /* logerror("riot6532_1b_r\n"); */ return 0x00; }
static WRITE_HANDLER(riot6532_1b_w)
{
//	logerror("riot6532_1b_w: 0x%02x\n", data);
	GTS80locals.OpSwitchEnable = (data&0x80);
	if ( core_gameData->gen & (GEN_GTS80B2K|GEN_GTS80B4K|GEN_GTS80B8K) ) {
		GTS80locals.disData = (data&0x3f);
		if ( data&0x40 ) {
			GTS80locals.segPos1 = 0;
			GTS80locals.segPos2 = 0;
			/* logerror("display reset\n"); */
			GTS80locals.disCmdMode1 = 0;
			GTS80locals.disCmdMode2 = 0;
		}
		/* LD1 */
		if ( !(GTS80locals.disData&0x10) ) {
			if ( GTS80locals.disCmdMode1 ) {
				GTS80locals.disCmdMode1 = 0;
				GTS80locals.segPos1 = 0;
			}
			else if ( GTS80locals.data==0x01 ) {
				GTS80locals.disCmdMode1 = 1;
			}
			else {
				GTS80locals.segments[GTS80locals.segPos1].w = core_ascii2seg[GTS80locals.data&0x7f] | (GTS80locals.data&0x80?0x8080:0x0000);
				GTS80locals.segPos1 = (GTS80locals.segPos1+1)%20;
			}
		}
		/* LD2 */
		if ( !(GTS80locals.disData&0x20) ) {
			if ( GTS80locals.disCmdMode2 ) {
				GTS80locals.disCmdMode2 = 0;
				GTS80locals.segPos2 = 0;
			}
			else if ( GTS80locals.data==0x01 ) {
				GTS80locals.disCmdMode2 = 1;
			}
			else {
				GTS80locals.segments[20+GTS80locals.segPos2].w = core_ascii2seg[GTS80locals.data&0x7f] | (GTS80locals.data&0x80?0x8080:0x0000);
				GTS80locals.segPos2 = (GTS80locals.segPos2+1)%20;
			}
		}
	}
	else
		GTS80locals.disData = (data&0x7f);

	return;
}

/*---------------------------
/  Solenoids, Lamps and Sound
/----------------------------*/

static int GTS80_bitsLo[] = { 0x01, 0x02, 0x04, 0x08 };
static int GTS80_bitsHi[] = { 0x10, 0x20, 0x40, 0x80 };

/* solenoids */
static READ_HANDLER(riot6532_2a_r)  { /* logerror("riot6532_2a_r\n"); */ return 0xff; }
static WRITE_HANDLER(riot6532_2a_w) {
	/* solenoids 1-4 */
	if ( !(data&0x20) ) GTS80locals.solenoids |= GTS80_bitsLo[~data&0x03];
	/* solenoids 1-4 */
	if ( !(data&0x40) ) GTS80locals.solenoids |= GTS80_bitsHi[(~data>>2)&0x03];
	/* solenoid 9    */
	GTS80locals.solenoids |= ((~data&0x80)<<1);

	/* sound */
	GTS80_sndCmd_w(0,!(data&0x10)?(~data)&0x0f:0x00);

//	logerror("riot6532_2a_w: 0x%02x\n", data);
}

static void video_irq(int irqline);

static READ_HANDLER(riot6532_2b_r)  { logerror("riot6532_2b_r\n"); return 0xff; }
static WRITE_HANDLER(riot6532_2b_w) {
	/* logerror("riot6532_2b_w: 0x%02x\n", data); */
	if ( data&0xf0 ) {
		int col = ((data&0xf0)>>4)-1;
		if ( col%2 ) {
			GTS80locals.lampMatrix[col/2] = (GTS80locals.lampMatrix[col/2]&0x0f)|((data&0x0f)<<4);
			if (col==11)
				GTS80locals.lampMatrix[6] = (GTS80locals.lampMatrix[5]>>4)^0x0f;
		}
		else {
                        //int iOld = GTS80locals.lampMatrix[1]&0x02;
			GTS80locals.lampMatrix[col/2] = (GTS80locals.lampMatrix[col/2]&0xf0)|(data&0x0f);
		}
		if (GTS80locals.hasVideo) {
			/* The right game command is determined by a specific lamp combination. Weird! */
			if (GTS80locals.lampMatrix[2] & 0x01) {
				UINT8 val = (GTS80locals.lampMatrix[0] & 0xf0) | (GTS80locals.lampMatrix[1] >> 4);
				GTS80locals.video_data = val;
				video_irq(0);
			}
		}

	}
	GTS80locals.swColOp = (data>>4)&0x03;
}

static struct riot6532_interface GTS80_riot6532_intf[] = {
{/* 6532RIOT 0 (0x200) Chip U4 (SWITCH MATRIX)*/
 /* PA0 - PA7 Switch Return  (ROW) */
 /* PB0 - PB7 Switch Strobes (COL) */
 /* in  : A/B, */ riot6532_0a_r, riot6532_0b_r,
 /* out : A/B, */ riot6532_0a_w, riot6532_0b_w,
 /* irq :      */ GTS80_irq
},

{/* 6532RIOT 1 (0x280) Chip U5 (DISPLAY & SWITCH ENABLE)*/
 /* PA0-3:  DIGIT STROBE */
 /* PA4:    Write to PL.1&2 */
 /* PA5:    Write to PL.3&4 */
 /* PA6:    Write to Ball/Credit */
 /* PA7(I): SLAM SWITCH */
 /* PB0-3:  DIGIT DATA */
 /* PB4:    H LINE (1&2) */
 /* PB5:    H LINE (3&4) */
 /* PB6:    H LINE (5&6) */
 /* PB7:    SWITCH ENABLE */
 /* in  : A/B, */ riot6532_1a_r, riot6532_1b_r,
 /* out : A/B, */ riot6532_1a_w, riot6532_1b_w,
 /* irq :      */ GTS80_irq
},

{/* 6532RIOT 2 (0x300) Chip U6*/
 /* PA0-6: FEED Z28(LS139) (SOL1-8) & SOUND 1-4 */
 /* PA7:   SOL.9 */
 /* PB0-3: LD1-4 */
 /* PB4-7: FEED Z33:LS154 (LAMP LATCHES) + PART OF SWITCH ENABLE */
 /* in  : A/B, */ riot6532_2a_r, riot6532_2b_r,
 /* out : A/B, */ riot6532_2a_w, riot6532_2b_w,
 /* irq :      */ GTS80_irq
}};


static WRITE_HANDLER(ram_256w) {
	UINT8 *pMem =  GTS80locals.pRAM + (offset%0x100);
	pMem[0x1800] = pMem[0x1900] = pMem[0x1a00] = pMem[0x1b00] = \
	pMem[0x1c00] = pMem[0x1d00] = pMem[0x1e00] = pMem[0x1f00] = \
		data;
}

static WRITE_HANDLER(riot6532_0_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0000] = pMem[0x4000] = pMem[0x8000] = pMem[0xc000] = data;
}

static WRITE_HANDLER(riot6532_1_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0080] = pMem[0x4080] = pMem[0x8080] = pMem[0xc080] = data;
}

static WRITE_HANDLER(riot6532_2_ram_w) {
	UINT8 *pMem = GTS80locals.pRAM + (offset%0x80);
	pMem[0x0100] = pMem[0x4100] = pMem[0x8100] = pMem[0xc100] = data;
}

/* for Caveman only */
static READ_HANDLER(in_1cb_r);
static READ_HANDLER(in_1e4_r);

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS80_readmem)
{0x0000,0x007f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x0080,0x00ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x0100,0x017f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x01cb,0x01cb, in_1cb_r},
{0x01e4,0x01e4, in_1e4_r},
{0x0200,0x027f, riot6532_0_r},	/*U4 - I/O*/
{0x0280,0x02ff, riot6532_1_r},	/*U5 - I/O*/
{0x0300,0x037f, riot6532_2_r},	/*U6 - I/O*/
{0x1000,0x17ff, MRA_ROM},		/*Game Prom(s)*/
{0x1800,0x1fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0x2000,0x2fff, MRA_ROM},		/*U2 ROM*/
{0x3000,0x3fff, MRA_ROM},		/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x4080,0x40ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x4100,0x417f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x4200,0x427f, riot6532_0_r},	/*U4 - I/O*/
{0x4280,0x42ff, riot6532_1_r},	/*U5 - I/O*/
{0x4300,0x437f, riot6532_2_r},	/*U6 - I/O*/
{0x5000,0x57ff, MRA_ROM},		/*Game Prom(s)*/
{0x5800,0x5fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0x6000,0x6fff, MRA_ROM},		/*U2 ROM*/
{0x7000,0x7fff, MRA_ROM},		/*U3 ROM*/

{0x8000,0x807f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0x8080,0x80ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0x8100,0x817f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0x8200,0x827f, riot6532_0_r},	/*U4 - I/O*/
{0x8280,0x82ff, riot6532_1_r},	/*U5 - I/O*/
{0x8300,0x837f, riot6532_2_r},	/*U6 - I/O*/
{0x9000,0x97ff, MRA_ROM},		/*Game Prom(s)*/
{0x9800,0x9fff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0xa000,0xafff, MRA_ROM},		/*U2 ROM*/
{0xb000,0xbfff, MRA_ROM},		/*U3 ROM*/

{0xc000,0xc07f,	MRA_RAM},		/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	MRA_RAM},		/*U5 - 6532 RAM*/
{0xc100,0xc17f,	MRA_RAM},		/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot6532_0_r},	/*U4 - I/O*/
{0xc280,0xc2ff, riot6532_1_r},	/*U5 - I/O*/
{0xc300,0xc37f, riot6532_2_r},	/*U6 - I/O*/
{0xd000,0xd7ff, MRA_ROM},		/*Game Prom(s)*/
{0xd800,0xdfff, MRA_RAM},		/*RAM - 8x the same 256 Bytes*/
{0xe000,0xefff, MRA_ROM},		/*U2 ROM*/
{0xf000,0xffff, MRA_ROM},		/*U3 ROM*/
MEMORY_END

static MEMORY_WRITE_START(GTS80_writemem)
{0x0000,0x007f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x0080,0x00ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x0100,0x017f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x0200,0x027f, riot6532_0_w},		/*U4 - I/O*/
{0x0280,0x02ff, riot6532_1_w},		/*U5 - I/O*/
{0x0300,0x037f, riot6532_2_w},		/*U6 - I/O*/
{0x1000,0x17ff, MWA_ROM},			/*Game Prom(s)*/
{0x1800,0x1fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0x2000,0x2fff, MWA_ROM},			/*U2 ROM*/
{0x3000,0x3fff, MWA_ROM},			/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x4080,0x40ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x4100,0x417f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x4200,0x427f, riot6532_0_w},		/*U4 - I/O*/
{0x4280,0x42ff, riot6532_1_w},		/*U5 - I/O*/
{0x4300,0x437f, riot6532_2_w},		/*U6 - I/O*/
{0x5000,0x57ff, MWA_ROM},			/*Game Prom(s)*/
{0x5800,0x5fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0x6000,0x6fff, MWA_ROM},			/*U2 ROM*/
{0x7000,0x7fff, MWA_ROM},			/*U3 ROM*/

{0x8000,0x807f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0x8080,0x80ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0x8100,0x817f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0x8200,0x827f, riot6532_0_w},		/*U4 - I/O*/
{0x8280,0x82ff, riot6532_1_w},		/*U5 - I/O*/
{0x8300,0x837f, riot6532_2_w},		/*U6 - I/O*/
{0x9000,0x97ff, MWA_ROM},			/*Game Prom(s)*/
{0x9800,0x9fff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0xa000,0xafff, MWA_ROM},			/*U2 ROM*/
{0xb000,0xbfff, MWA_ROM},			/*U3 ROM*/

{0xc000,0xc07f,	riot6532_0_ram_w},	/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	riot6532_1_ram_w},	/*U5 - 6532 RAM*/
{0xc100,0xc17f,	riot6532_2_ram_w},	/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot6532_0_w},		/*U4 - I/O*/
{0xc280,0xc2ff, riot6532_1_w},		/*U5 - I/O*/
{0xc300,0xc37f, riot6532_2_w},		/*U6 - I/O*/
{0xd000,0xd7ff, MWA_ROM},			/*Game Prom(s)*/
{0xd800,0xdfff, ram_256w},			/*RAM - 8x the same 256 Bytes*/
{0xe000,0xefff, MWA_ROM},			/*U2 ROM*/
{0xf000,0xffff, MWA_ROM},			/*U3 ROM*/
MEMORY_END

static void init_common(void) {
  int ii;

  memset(&GTS80locals, 0, sizeof GTS80locals);

  /* init ROM */
  for(ii = 1; ii<4; ii++)
	memcpy(memory_region(GTS80_MEMREG_CPU)+0x2000+0x4000*ii, memory_region(GTS80_MEMREG_CPU)+0x2000, 0x2000);

  if ( core_gameData->gen & GEN_GTS80B4K ) {
	memcpy(memory_region(GTS80_MEMREG_CPU)+0x5000, memory_region(GTS80_MEMREG_CPU)+0x1000, 0x800);
	memcpy(memory_region(GTS80_MEMREG_CPU)+0xd000, memory_region(GTS80_MEMREG_CPU)+0x9000, 0x800);
  }
  else {
	for(ii = 1; ii<4; ii++)
	  memcpy(memory_region(GTS80_MEMREG_CPU)+0x1000+0x4000*ii, memory_region(GTS80_MEMREG_CPU)+0x1000, 0x0800);
  }

  /* init RAM pointer */
  GTS80locals.pRAM = memory_region(GTS80_MEMREG_CPU);

  /* init RIOTS */
  for (ii = 0; ii < sizeof(GTS80_riot6532_intf)/sizeof(GTS80_riot6532_intf[0]); ii++)
    riot6532_config(ii, &GTS80_riot6532_intf[ii]);

  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);

  riot6532_reset();

  GTS80locals.initDone = TRUE;
}

static MACHINE_INIT(gts80) {
  init_common();
}

static MACHINE_STOP(gts80)
{
  sndbrd_0_exit();

  riot6532_unconfig();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/

static NVRAM_HANDLER(gts80) {
	int i;
	core_nvram(file, read_or_write, memory_region(GTS80_MEMREG_CPU)+0x1800, 0x100, 0x00);
	if (!read_or_write)
		for(i=1;i<8;i++)
			memcpy(memory_region(GTS80_MEMREG_CPU)+0x1800+(i*0x100), memory_region(GTS80_MEMREG_CPU)+0x1800, 0x100);
}

MACHINE_DRIVER_START(gts80)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(gts80,NULL,gts80)
  MDRV_CPU_ADD(M6502, 850000)
  MDRV_CPU_MEMORY(GTS80_readmem, GTS80_writemem)
  MDRV_CPU_VBLANK_INT(GTS80_vblank, 1)
  MDRV_NVRAM_HANDLER(gts80)
  MDRV_DIPS(42) /* 42 DIPs (32 for the controller board, 8 for the SS- and 2 for the S-Board*/
  MDRV_SWITCH_UPDATE(GTS80)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_CONV(GTS80_sw2m,GTS80_m2sw)
  MDRV_LAMP_CONV(GTS80_lamp2m,GTS80_m2lamp)
  MDRV_SOUND_CMD(GTS80_sndCmd_w)
  MDRV_SOUND_CMDHEADING("GTS80")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80s)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80ss)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_ss)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80b)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs1)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs2)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs3)
  MDRV_IMPORT_FROM(gts80)
  MDRV_IMPORT_FROM(gts80s_b3)
MACHINE_DRIVER_END



/*
	Additional code for Caveman below. Some notes:

	The memory from 0x0000 to 0x00ff holds the 8086
	IRQ vectors. I found only 4 vectors are filled
	with any useful values: the NMI vector and
	3 others, namely the ones at memory locations:

	0x0008 -> #0x02 (fixed to NMI according to i86.h)
	0x0080 -> #0x20
	0x0084 -> #0x21
	0x009c -> #0x27 (same address as 0x21)

	I picked 0x20, because this way, the language selection
	as described below works. No idea what the other IRQs
	are used for yet...

	Next, the command to be transmitted to the video board
	is determined by a certain lamp pattern at strobe time.
	The command consists of the number of the active player
	in the high nibble and a state "code" in the low nibble.
	The video game state is depending on these codes as
	they are returned from Port 0x300.
	I found the meaning of some of these codes:
	(the alphabetic letter was taken from the test screen)

	A - 0x01 - reset game screen
	B - 0x02 - place Caveman on left hand side
	C - 0x03 - place Caveman on right hand side
	D - 0x04 - convert a Tyrannosaurus into a Pterodactyl
	F - 0x06 - show extra ball spot
	G - 0x07 - tilt
	H - 0x08 - exit to demo mode
	I - 0x09 - hide extra ball spot, also tilt reset
	K - 0x0b - display english language instructions
	L - 0x0c - display french language instructions
	M - 0x0d - display german language instructions (code default!)

	Game dip switches 6 to 8 control the language used.
	Setting them all to 1 enables german text.
	Setting either one to off sets english text.
	(Didn't find the setting for french text yet)
*/

static int callback = 0x20;

static void video_irq(int irqline)
{
  if (irqline) {
    callback = 0x21;
    GTS80locals.video_data = coreGlobals.swMatrix[0] >> 4;
  } else
    callback = 0x20;
  logerror("command to video board = %02X\n", GTS80locals.video_data);
  cpu_set_irq_line(GTS80_VIDCPU, 0, ASSERT_LINE);
}

/* The right i86 IRQ vector is determined by the return value of irq_callback(0)???
   Why's that??? Sounds funny to me... but what the heck. */
static int irq_callback(int irqline)
{
  cpu_set_irq_line(GTS80_VIDCPU, 0, CLEAR_LINE);
  return callback;
}

static int maxy = -1;

static MACHINE_INIT(gts80vid) {
  static int init_done = 0;
  init_common();
  GTS80locals.hasVideo = 1;
  maxy = -1;
  cpu_set_irq_callback(GTS80_VIDCPU, irq_callback);

  if (!init_done) { /* the following must happen only once! */
    int i;
    UINT32 off, j = 0xf8000;
    UINT8* pvrom = memory_region(GTS80_MEMREG_VIDCPU);

    /* copying video ROM contents to the right place (in the right byte order). */
    for (off = 0x08000; off < 0x10000; off += 0x2000) {
      for (i = 0; i < 0x1000; i++) {
        memcpy(pvrom + (j++), pvrom + off + i, 1);
        memcpy(pvrom + (j++), pvrom + off + i + 0x1000, 1);
      }
    }
    memcpy(pvrom + 0x08000, pvrom + 0xf8000, 0x08000);

    init_done = 1;
  }

}

static VIDEO_START(gts80vid)
{
	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;
	return 0;
}

static VIDEO_STOP(gts80vid)
{
}

static UINT8 *vram;

static WRITE_HANDLER(vram_w) {
	int y = offset / 64;
	int x = offset % 64;
	int p1 = 5 + (data >> 6);
	int p2 = 5 + ((data >> 4) & 0x03);
	int p3 = 5 + ((data >> 2) & 0x03);
	int p4 = 5 + (data & 0x03);

	plot_pixel(tmpbitmap, x*4,   maxy+y, p1);
	plot_pixel(tmpbitmap, x*4+1, maxy+y, p2);
	plot_pixel(tmpbitmap, x*4+2, maxy+y, p3);
	plot_pixel(tmpbitmap, x*4+3, maxy+y, p4);
	vram[offset] = data;
}

static READ_HANDLER(vram_r) {
	return vram[offset];
}

/* in order to use the routine from core.c,
   it has to be declared non-static there! */
extern VIDEO_UPDATE(core_gen);

/* clipping area if dmd_only is on */
const struct rectangle clip1 = { 0, 255, 70, 309 };

/* clipping area if dmd_only is off - shows the garbled screen bottom too */
const struct rectangle clip2 = { 0, 255, 140, 395 };

static VIDEO_UPDATE(gts80vid)
{
	/* Draw regular display, lamps, etc */
	video_update_core_gen(bitmap, cliprect);

	if (maxy < 0) {
		maxy = Machine->visible_area.max_y;
		if (maxy < 140) {
			maxy = 70;
			set_visible_area(0, 255, 0, 309);
		} else {
		    maxy = 140;
			set_visible_area(0, 255, 0, 395);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,(maxy < 100 ? &clip1 : &clip2),TRANSPARENCY_NONE,0);
}

/* we'll need a screenshot from the actual game to set the palette right! */
static int rgb[16*3] = {
	0, 0, 0,
	127, 255, 127,
	255, 127, 127,
	255, 255, 127,
	127, 127, 255,
	127, 255, 255,
	255, 127, 255,
	255, 255, 255,
	0, 0, 0,
	0, 255, 0,
	255, 0, 0,
	255, 255, 0,
	0, 0, 255,
	0, 255, 255,
	255, 0, 255,
	255, 255, 255
};

static int read1x = 0;

/* port 0xx = Interrupt Controller */
static WRITE_HANDLER(port00xw) {
	logerror("IRQ Controller write %x = %02x\n", offset/2, data);
}
static READ_HANDLER(port002r) {
	logerror("IRQ Controller read\n");
	return 0x00;
}
/* port 1xx = CRTC HD46505 */
static WRITE_HANDLER(port10xw) {
	if (offset == 0 && data == 0x10)
		read1x = 0;
	if (offset == 0 && data == 0x11)
		read1x = 1;
	logerror("HD46505 write %x = %02x\n", offset/2, data);
}
static READ_HANDLER(port102r) {
	logerror("HD46505 read\n");
	return 0x00;
}
/* output to game switches, row 0 */
static WRITE_HANDLER(port200w) {
	int i;
	if (data)
		for (i=1; i < 8; i++) {
			core_setSw(GTS80_m2sw(i, 0), data & 1);
			data >>= 1;
		}
}
static READ_HANDLER(port200r) {
/* no idea what it is for, but its high nibble
   has got to be all 1 after IRQ 0x20 is called,
   or else the video code enters an endless loop. */
	logerror("read  port 200\n");
	return 0xf0;
}
/* output to game switches, row 1 */
static WRITE_HANDLER(port300w) {
	int i;
	if (data)
		for (i=1; i < 8; i++) {
			core_setSw(GTS80_m2sw(i, 1), data & 1);
			data >>= 1;
		}
}
/* read game state and no. of active player */
static READ_HANDLER(port300r) {
	return GTS80locals.video_data;
}
/* joystick inports */
static READ_HANDLER(port400r) {
	return coreGlobals.swMatrix[0] & 0x0f;
}
/* set up game colors. 4 colors at a time only, out of 16 possible! */
static WRITE_HANDLER(port50xw) {
	palette_set_color(5+offset/2, rgb[data*3], rgb[data*3+1], rgb[data*3+2]);
}

static READ_HANDLER(in_1cb_r) {
	logerror("read Caveman input 1CB\n");
	return 0;
}
static READ_HANDLER(in_1e4_r) {
	logerror("read Caveman input 1E4\n");
	return 0;
}

PORT_READ_START(video_readport)
	{ 0x002, 0x002, port002r },
	{ 0x102, 0x102, port102r },
	{ 0x200, 0x200, port200r },
	{ 0x300, 0x300, port300r },
	{ 0x400, 0x400, port400r },
PORT_END

PORT_WRITE_START(video_writeport)
	{ 0x000, 0x002, port00xw },
	{ 0x100, 0x102, port10xw },
	{ 0x200, 0x200, port200w },
	{ 0x300, 0x300, port300w },
	{ 0x500, 0x506, port50xw },
PORT_END

static MEMORY_READ_START(video_readmem)
{0x00000,0x0058e, MRA_RAM},
{0x007cf,0x007fe, MRA_RAM},
{0x02000,0x05fff, vram_r},
{0x08000,0x0ffff, MRA_ROM},
{0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(video_writemem)
{0x00000,0x0058e, MWA_RAM},
{0x007cf,0x007fe, MWA_RAM},
{0x02000,0x05fff, vram_w, &vram},
{0x08000,0x0ffff, MWA_ROM},
{0xf8000,0xfffff, MWA_ROM},
MEMORY_END

MACHINE_DRIVER_START(gts80vid)
  MDRV_IMPORT_FROM(gts80ss)
  MDRV_CPU_ADD_TAG("vcpu", I86, 5000000)
  MDRV_CPU_MEMORY(video_readmem, video_writemem)
  MDRV_CPU_PORTS(video_readport, video_writeport)
  MDRV_CORE_INIT_RESET_STOP(gts80vid,NULL,gts80)
  MDRV_SWITCH_UPDATE(GTS80VID)
  /* video hardware */
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 255, 0, 399)
  MDRV_GFXDECODE(0)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE)
  MDRV_VIDEO_START(gts80vid)
  MDRV_VIDEO_STOP(gts80vid)
  MDRV_VIDEO_UPDATE(gts80vid)
MACHINE_DRIVER_END
