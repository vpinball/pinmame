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
#include "cpu/m6502/m6502.h"
#include "machine/6532riot.h"
#include "core.h"
#include "s80.h"
#include "s80sound0.h"
#include "s80sound1.h"
#include "s80sound2.h"

#define S80_VBLANKFREQ      60 /* VBLANK frequency */

#define S80_CPUNO	0
#define S80SS_SCPU	1

static void S80_init(void);
static void S80_exit(void);
static void S80_nvram(void *file, int write);

/* operator adjustable switches
	s1 is the msb of the first byte, s32 the lsb of the last byte :

	s1  s2  s3  s4  s5  s6  s7  s8
	s9  s10 s11 s12 s13 s14 s15 s16
	s17 s18 s19 s20 s21 s22 s23 s24
	s25 s26 s27 s28 s29 s30 s31 s32
*/
#if 0
static UINT8 opSwitches[4] = {0x02, 0x83, 0xd3, 0xfc};
#endif
int core_bcd2seg16[16] = {0x3f00,0x0022,0x5b08,0x4f08,0x6608,0x6d08,0x7d08,0x0700,0x7f08,0x6f08,
#ifdef MAME_DEBUG
/*
0x3700, 0x7C08, 0x3900, 0x5E08, 0x7908, 0x7108
*/
#endif /* MAME_DEBUG */
};


static int core_ascii2seg[] = {
	/* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x20-0x27 */ 0x0000, 0x0903, 0x2002, 0x4E2A, 0x6D2A, 0x656E, 0x5D13, 0x0004,
	/* 0x28-0x2f */ 0x0014, 0x0041, 0x407F, 0x402A, 0x0000, 0x4008, 0x0000, 0x0044,
	/* 0x30-0x37 */ 0x3f00, 0x0022, 0x5B08, 0x4F08, 0x6608, 0x6D08, 0x7D08, 0x0700,
	/* 0x38-0x3f */ 0x7F08, 0x6F08, 0x0900, 0x0140, 0x0844, 0x4808, 0x0811, 0x0328,
	/* 0x40-0x47 */ 0x5F20, 0x7708, 0x0F2A, 0x3900, 0x0F22, 0x7900, 0x7100, 0x3D08,
	/* 0x48-0x4f */ 0x7608, 0x0922, 0x1E00, 0x7014, 0x3800, 0x3605, 0x3611, 0x3f00,
	/* 0x50-0x57 */ 0x7308, 0x3F10, 0x7318, 0x6D08, 0x0122, 0x3E00, 0x3044, 0x3650,
	/* 0x58-0x5f */ 0x0055, 0x0025, 0x0944, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
	/* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
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
} S80locals;

static void S80_irq(int state) {
  cpu_set_irq_line(S80_CPUNO, M6502_INT_IRQ, state ? ASSERT_LINE : CLEAR_LINE);
}

static int S80_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/

  S80locals.vblankCount += 1;
  /*-- lamps --*/
  if ((S80locals.vblankCount % S80_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, S80locals.lampMatrix, sizeof(S80locals.lampMatrix));
//    memset(S80locals.lampMatrix, 0, sizeof(S80locals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((S80locals.vblankCount % S80_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = S80locals.solenoids;
    if (S80locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    S80locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((S80locals.vblankCount % S80_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, S80locals.segments, sizeof(coreGlobals.segments));
	memset(S80locals.segments, 0x00, sizeof S80locals.segments);
//    memcpy(S80locals.segments, S80locals.pseg, sizeof(S80locals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* assume flipper enabled */
  return 0;
}

static void S80_updSw(int *inports) {
	if (inports) {
		coreGlobals.swMatrix[0] = ((inports[S80_COMINPORT] & 0xff00)>>8);
        coreGlobals.swMatrix[8] = (coreGlobals.swMatrix[8]&0xc0) | (inports[S80_COMINPORT] & 0x003f);
	}

  /*-- slam tilt --*/
	riot_set_input_a(1, (core_gameData->gen&GEN_S80B4K) ? (core_getSw(S80_SWSLAMTILT)?0x80:0x00) : (core_getSw(S80_SWSLAMTILT)?0x00:0x80));
}

static WRITE_HANDLER(S80_sndCmd_w) {
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	if ( core_gameData->gen & (GEN_S80|GEN_S80B) ) {
		// sys80 sound board
		sys80_sound_latch_s(data);
	}
	if ( core_gameData->gen & GEN_S80SS ) {
		// sys80 sound & speech board
		sys80_sound_latch_ss(data|((S80locals.lampMatrix[1]&0x02)?0x10:0x00));
	}
	if ( core_gameData->gen & (GEN_S80B2K|GEN_S80B4K) ) {
		// sys80b sound board (all generations)
		S80Bs_soundlatch(data);
	}
	// logerror("sound cmd: 0x%02x\n", data);
}

/* S80 switch numbering, row and column is swapped */
static int S80_sw2m(int no) {
	if ( no>=96 )
		return (no/10)*8+(no%10-1);
	else {
		no += 1;
		return (no%10)*8 + no/10;
	}
}

static int S80_m2sw(int col, int row) {
	if (col > 9 || (col == 9 && row >= 6))
		return col*8+row;
	else
		return row*10+col-1;
}
static int S80_m2lamp(int no) { return no+8; }
static int S80_lamp2m(int col, int row) { return (col-1)*8+row; }

static core_tData S80Data = {
  32, /* 32 DIPs */
  S80_updSw,
  1,
  S80_sndCmd_w, "S80",
  S80_sw2m, S80_m2lamp, S80_m2sw, S80_lamp2m
};

static int S80_getSwRow(int row) {
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
static READ_HANDLER(riot0a_r)  { return (S80locals.OpSwitchEnable) ? core_revbyte(core_getDip(S80locals.swColOp)) : (S80_getSwRow(S80locals.swRow)&0xff);}
static WRITE_HANDLER(riot0a_w) { logerror("riot0a_w: 0x%02x\n", data); }

static READ_HANDLER(riot0b_r)  { /* logerror("riot0b_r\n"); */ return 0x7f; }
static WRITE_HANDLER(riot0b_w) { /* logerror("riot0b_w: 0x%02x\n", data); */  S80locals.swRow = data; }

/*---------------------------
/  Display
/----------------------------*/

static READ_HANDLER(riot1a_r)  { /* logerror("riot1a_r\n"); */ return core_gameData->gen&GEN_S80B4K ? (core_getSw(S80_SWSLAMTILT)?0x80:0x00) : (core_getSw(S80_SWSLAMTILT)?0x00:0x80); }
static WRITE_HANDLER(riot1a_w)
{
	int segSel = ((data>>4)&0x07);
	if ( core_gameData->gen & (GEN_S80B|GEN_S80B2K|GEN_S80B4K) ) {
		if ( (segSel&0x01) && !(S80locals.segSel&0x01) )
			S80locals.data = (S80locals.data&0xf0) | (S80locals.disData&0x0f);
		if ( (segSel&0x02) && !(S80locals.segSel&0x02) )
			S80locals.data = (S80locals.data&0x0f) | ((S80locals.disData&0x0f)<<4);
	}
	else {
		int strobe = (data&0x0f);

		if ( (segSel&0x01) && !(S80locals.segSel&0x01) )
			S80locals.seg1 = core_bcd2seg9[S80locals.disData&0x0f];
		if ( !(S80locals.disData&0x10) )
			S80locals.seg1 = core_bcd2seg9[0x01];

		if ( (segSel&0x02) && !(S80locals.segSel&0x02) )
			S80locals.seg2 = core_bcd2seg9[S80locals.disData&0x0f];
		if ( !(S80locals.disData&0x20) )
			S80locals.seg2 = core_bcd2seg9[0x01];

		if ( (segSel&0x04) && !(S80locals.segSel&0x04) )
			S80locals.seg3 = core_bcd2seg9[S80locals.disData&0x0f];
		if ( !(S80locals.disData&0x40) )
			S80locals.seg3 = core_bcd2seg9[0x01];

		if ( strobe<=5 ) {
			if ( S80locals.seg1 ) {
				S80locals.segments[0][7-strobe].lo = S80locals.seg1 & 0x00ff;
				S80locals.segments[0][7-strobe].hi = S80locals.seg1 >> 8;
			}
			if ( S80locals.seg2 ) {
				S80locals.segments[1][7-strobe].lo = S80locals.seg2 & 0x00ff;
				S80locals.segments[1][7-strobe].hi = S80locals.seg2 >> 8;
			}
			if ( S80locals.seg3 ) {
				S80locals.segments[2][7-strobe].lo = S80locals.seg3 & 0x00ff;
				S80locals.segments[2][7-strobe].hi = S80locals.seg3 >> 8;
			}
		}
		else if ( strobe<=11 ) {
			if ( S80locals.seg1 ) {
				S80locals.segments[0][21-strobe].lo = S80locals.seg1 & 0x00ff;
				S80locals.segments[0][21-strobe].hi = S80locals.seg1 >> 8;
			}
			if ( S80locals.seg2 ) {
				S80locals.segments[1][21-strobe].lo = S80locals.seg2 & 0x00ff;
				S80locals.segments[1][21-strobe].hi = S80locals.seg2 >> 8;
			}
			if ( S80locals.seg3 ) {
				S80locals.segments[2][21-strobe].lo = S80locals.seg3 & 0x00ff;
				S80locals.segments[2][21-strobe].hi = S80locals.seg3 >> 8;
			}
		}
		else {
			// 7th digits in score display for s80a
			if ( S80locals.seg1 ) {
				if ( strobe==12 ) {
					S80locals.segments[0][9].lo = S80locals.seg1 & 0x00ff;
					S80locals.segments[0][9].hi = S80locals.seg1 >> 8;
				}
				else if ( strobe==15 ) {
					S80locals.segments[0][1].lo = S80locals.seg1 & 0x00ff;
					S80locals.segments[0][1].hi = S80locals.seg1 >> 8;
				}
			}
			if ( S80locals.seg2 ) {
				if ( strobe==12 ) {
					S80locals.segments[1][9].lo = S80locals.seg2 & 0x00ff;
					S80locals.segments[1][9].hi = S80locals.seg2 >> 8;
				}
				else if ( strobe==15 ) {
					S80locals.segments[1][1].lo = S80locals.seg2 & 0x00ff;
					S80locals.segments[1][1].hi = S80locals.seg2 >> 8;
				}
			}
			if ( S80locals.seg3 ) {
				switch ( strobe ) {
				case 12:
					S80locals.segments[0][8].lo = S80locals.seg3 & 0x00ff;
					S80locals.segments[0][8].hi = S80locals.seg3 >> 8;
					break;

				case 13:
					S80locals.segments[0][0].lo = S80locals.seg3 & 0x00ff;
					S80locals.segments[0][0].hi = S80locals.seg3 >> 8;
					break;

				case 14:
					S80locals.segments[1][8].lo = S80locals.seg3 & 0x00ff;
					S80locals.segments[1][8].hi = S80locals.seg3 >> 8;
					break;

				case 15:
					S80locals.segments[1][0].lo = S80locals.seg3 & 0x00ff;
					S80locals.segments[1][0].hi = S80locals.seg3 >> 8;
					break;
				}
			}
		}
	}

	S80locals.segSel = segSel;
	return;
}

static READ_HANDLER(riot1b_r)  { /* logerror("riot1b_r\n"); */ return 0x00; }
static WRITE_HANDLER(riot1b_w)
{
//	logerror("riot1b_w: 0x%02x\n", data);
	S80locals.OpSwitchEnable = (data&0x80);
	if ( core_gameData->gen & (GEN_S80B|GEN_S80B2K|GEN_S80B4K) ) {
		int value;

		S80locals.disData = (data&0x3f);
		if ( data&0x40 ) {
			S80locals.segPos1 = 0;
			S80locals.segPos2 = 0;
			/* logerror("display reset\n"); */
			S80locals.disCmdMode1 = 0;
			S80locals.disCmdMode2 = 0;
		}
		/* LD1 */
		if ( !(S80locals.disData&0x10) ) {
			if ( S80locals.disCmdMode1 ) {
				S80locals.disCmdMode1 = 0;
				S80locals.segPos1 = 0;
			}
			else if ( S80locals.data==0x01 ) {
				S80locals.disCmdMode1 = 1;
			}
			else {
				value = core_ascii2seg[S80locals.data&0x7f] | (S80locals.data&0x80?0x8080:0x0000);
				S80locals.segments[0][S80locals.segPos1].lo = value&0x00ff;
				S80locals.segments[0][S80locals.segPos1].hi = value>>8;
				S80locals.segPos1 = (S80locals.segPos1+1)%20;
			}
		}
		/* LD2 */
		if ( !(S80locals.disData&0x20) ) {
			if ( S80locals.disCmdMode2 ) {
				S80locals.disCmdMode2 = 0;
				S80locals.segPos2 = 0;
			}
			else if ( S80locals.data==0x01 ) {
				S80locals.disCmdMode2 = 1;
			}
			else {
				value = core_ascii2seg[S80locals.data&0x7f] | (S80locals.data&0x80?0x8080:0x0000);
				S80locals.segments[1][S80locals.segPos2].lo = value&0x00ff;
				S80locals.segments[1][S80locals.segPos2].hi = value>>8;
				S80locals.segPos2 = (S80locals.segPos2+1)%20;
			}
		}
	}
	else
		S80locals.disData = (data&0x7f);

	return;
}

/*---------------------------
/  Solenoids, Lamps and Sound
/----------------------------*/

static int S80_bitsLo[] = { 0x01, 0x02, 0x04, 0x08 };
static int S80_bitsHi[] = { 0x10, 0x20, 0x40, 0x80 };

/* solenoids */
static READ_HANDLER(riot2a_r)  { /* logerror("riot2a_r\n"); */ return 0xff; }
static WRITE_HANDLER(riot2a_w) {
	/* solenoids 1-4 */
	if ( !(data&0x20) ) S80locals.solenoids |= S80_bitsLo[~data&0x03];
	/* solenoids 1-4 */
	if ( !(data&0x40) ) S80locals.solenoids |= S80_bitsHi[(~data>>2)&0x03];
	/* solenoid 9    */
	S80locals.solenoids |= ((~data&0x80)<<1);

	/* sound */
	S80_sndCmd_w(0,!(data&0x10)?(~data)&0x0f:0x00);
	
//	logerror("riot2a_w: 0x%02x\n", data);
}

static READ_HANDLER(riot2b_r)  { logerror("riot2b_r\n"); return 0xff; }
static WRITE_HANDLER(riot2b_w) {
	/* logerror("riot2b_w: 0x%02x\n", data); */
	if ( data&0xf0 ) {
		int col = ((data&0xf0)>>4)-1;
		if ( col%2 ) {
			S80locals.lampMatrix[col/2] = (S80locals.lampMatrix[col/2]&0x0f)|((data&0x0f)<<4);
			if (col==11)
				S80locals.lampMatrix[6] = (S80locals.lampMatrix[5]>>4)^0x0f;
		}
		else {
                        //int iOld = S80locals.lampMatrix[1]&0x02;
			S80locals.lampMatrix[col/2] = (S80locals.lampMatrix[col/2]&0xf0)|(data&0x0f);
		}
	}
	S80locals.swColOp = (data>>4)&0x03;
}

/* Sound/Speak RIOT 3 interface, can be found in sound1.c */
extern void S80SS_irq(int state);
extern READ_HANDLER(riot3a_r);
extern WRITE_HANDLER(riot3a_w);
extern READ_HANDLER(riot3b_r);
extern WRITE_HANDLER(riot3b_w);

struct riot6532_interface s80_riot_intf[] = {
{/* RIOT 0 (0x200) Chip U4 (SWITCH MATRIX)*/
 /* PA0 - PA7 Switch Return  (ROW) */
 /* PB0 - PB7 Switch Strobes (COL) */
 /* in  : A/B, */ riot0a_r, riot0b_r,
 /* out : A/B, */ riot0a_w, riot0b_w,
 /* irq :      */ S80_irq
},

{/* RIOT 1 (0x280) Chip U5 (DISPLAY & SWITCH ENABLE)*/
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
 /* in  : A/B, */ riot1a_r, riot1b_r,
 /* out : A/B, */ riot1a_w, riot1b_w,
 /* irq :      */ S80_irq
},

{/* RIOT 2 (0x300) Chip U6*/
 /* PA0-6: FEED Z28(LS139) (SOL1-8) & SOUND 1-4 */
 /* PA7:   SOL.9 */
 /* PB0-3: LD1-4 */
 /* PB4-7: FEED Z33:LS154 (LAMP LATCHES) + PART OF SWITCH ENABLE */
 /* in  : A/B, */ riot2a_r, riot2b_r,
 /* out : A/B, */ riot2a_w, riot2b_w,
 /* irq :      */ S80_irq
},

{/* RIOT 3: Sound/Speak board Chip U15 */
 /* in  : A/B, */ riot3a_r, riot3b_r,
 /* out : A/B, */ riot3a_w, riot3b_w,
 /* irq :      */ S80SS_irq
}};

static UINT8 RAM_256[0x100];
static READ_HANDLER(ram_256r) {
	return RAM_256[offset%0x100];
}

static WRITE_HANDLER(ram_256w) {
	RAM_256[offset%0x100] = (data&0x0f);
}

static UINT8 RIOT0_RAM[0x80];
static UINT8 RIOT1_RAM[0x80];
static UINT8 RIOT2_RAM[0x80];

static READ_HANDLER(riot0_ram_r) {
	return RIOT0_RAM[offset%0x80];
}

static WRITE_HANDLER(riot0_ram_w) {
	RIOT0_RAM[offset%0x80] = data;
}

static READ_HANDLER(riot1_ram_r) {
	return RIOT1_RAM[offset%0x80];
}

static WRITE_HANDLER(riot1_ram_w) {
	RIOT1_RAM[offset%0x80] = data;
}

static READ_HANDLER(riot2_ram_r) {
	return RIOT2_RAM[offset%0x80];
}

static WRITE_HANDLER(riot2_ram_w) {
	RIOT2_RAM[offset%0x80] = data;
}


/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(S80_readmem)
{0x0000,0x007f,	riot0_ram_r},	/*U4 - 6532 RAM*/
{0x0080,0x00ff,	riot1_ram_r},	/*U5 - 6532 RAM*/
{0x0100,0x017f,	riot2_ram_r},	/*U6 - 6532 RAM*/
{0x0200,0x027f, riot_0_r},		/*U4 - I/O*/
{0x0280,0x02ff, riot_1_r},		/*U5 - I/O*/
{0x0300,0x037f, riot_2_r},		/*U6 - I/O*/
{0x1000,0x17ff, MRA_ROM},		/*Game Prom(s)*/
{0x1800,0x1fff, ram_256r},		
{0x2000,0x2fff, MRA_ROM},		/*U2 ROM*/
{0x3000,0x3fff, MRA_ROM},		/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	riot0_ram_r},	/*U4 - 6532 RAM*/
{0x4080,0x40ff,	riot1_ram_r},	/*U5 - 6532 RAM*/
{0x4100,0x417f,	riot2_ram_r},	/*U6 - 6532 RAM*/
{0x4200,0x427f, riot_0_r},		/*U4 - I/O*/
{0x4280,0x42ff, riot_1_r},		/*U5 - I/O*/
{0x4300,0x437f, riot_2_r},		/*U6 - I/O*/
{0x5000,0x57ff, MRA_ROM},		/*Game Prom(s)*/
{0x5800,0x5fff, ram_256r},		
{0x6000,0x6fff, MRA_ROM},		/*U2 ROM*/
{0x7000,0x7fff, MRA_ROM},		/*U3 ROM*/

{0x8000,0x807f,	riot0_ram_r},	/*U4 - 6532 RAM*/
{0x8080,0x80ff,	riot1_ram_r},	/*U5 - 6532 RAM*/
{0x8100,0x817f,	riot2_ram_r},	/*U6 - 6532 RAM*/
{0x8200,0x827f, riot_0_r},		/*U4 - I/O*/
{0x8280,0x82ff, riot_1_r},		/*U5 - I/O*/
{0x8300,0x837f, riot_2_r},		/*U6 - I/O*/
{0x9000,0x97ff, MRA_ROM},		/*Game Prom(s)*/
{0x9800,0x9fff, ram_256r},		
{0xa000,0xafff, MRA_ROM},		/*U2 ROM*/
{0xb000,0xbfff, MRA_ROM},		/*U3 ROM*/

{0xc000,0xc07f,	riot0_ram_r},	/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	riot1_ram_r},	/*U5 - 6532 RAM*/
{0xc100,0xc17f,	riot2_ram_r},	/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot_0_r},		/*U4 - I/O*/
{0xc280,0xc2ff, riot_1_r},		/*U5 - I/O*/
{0xc300,0xc37f, riot_2_r},		/*U6 - I/O*/
{0xd000,0xd7ff, MRA_ROM},		/*Game Prom(s)*/
{0xd800,0xdfff, ram_256r},		
{0xe000,0xefff, MRA_ROM},		/*U2 ROM*/
{0xf000,0xffff, MRA_ROM},		/*U3 ROM*/
MEMORY_END

static MEMORY_WRITE_START(S80_writemem)
{0x0000,0x007f,	riot0_ram_w},	/*U4 - 6532 RAM*/
{0x0080,0x00ff,	riot1_ram_w},	/*U5 - 6532 RAM*/
{0x0100,0x017f,	riot2_ram_w},	/*U6 - 6532 RAM*/
{0x0200,0x027f, riot_0_w},		/*U4 - I/O*/
{0x0280,0x02ff, riot_1_w},		/*U5 - I/O*/
{0x0300,0x037f, riot_2_w},		/*U6 - I/O*/
{0x1000,0x17ff, MWA_ROM},		/*Game Prom(s)*/
{0x1800,0x1fff, ram_256w},		/*RAM - 4x the same 256 Bytes*/
{0x2000,0x2fff, MWA_ROM},		/*U2 ROM*/
{0x3000,0x3fff, MWA_ROM},		/*U3 ROM*/

/* A14 & A15 aren't used) */
{0x4000,0x407f,	riot0_ram_w},	/*U4 - 6532 RAM*/
{0x4080,0x40ff,	riot1_ram_w},	/*U5 - 6532 RAM*/
{0x4100,0x417f,	riot2_ram_w},	/*U6 - 6532 RAM*/
{0x4200,0x427f, riot_0_w},		/*U4 - I/O*/
{0x4280,0x42ff, riot_1_w},		/*U5 - I/O*/
{0x4300,0x437f, riot_2_w},		/*U6 - I/O*/
{0x5000,0x57ff, MWA_ROM},		/*Game Prom(s)*/
{0x5800,0x5fff, ram_256w},		/*RAM - 4x the same 256 Bytes*/
{0x6000,0x6fff, MWA_ROM},		/*U2 ROM*/
{0x7000,0x7fff, MWA_ROM},		/*U3 ROM*/

{0x8000,0x807f,	riot0_ram_w},	/*U4 - 6532 RAM*/
{0x8080,0x80ff,	riot1_ram_w},	/*U5 - 6532 RAM*/
{0x8100,0x817f,	riot2_ram_w},	/*U6 - 6532 RAM*/
{0x8200,0x827f, riot_0_w},		/*U4 - I/O*/
{0x8280,0x82ff, riot_1_w},		/*U5 - I/O*/
{0x8300,0x837f, riot_2_w},		/*U6 - I/O*/
{0x9000,0x97ff, MWA_ROM},		/*Game Prom(s)*/
{0x9800,0x9fff, ram_256w},		/*RAM - 4x the same 256 Bytes*/
{0xa000,0xafff, MWA_ROM},		/*U2 ROM*/
{0xb000,0xbfff, MWA_ROM},		/*U3 ROM*/

{0xc000,0xc07f,	riot0_ram_w},	/*U4 - 6532 RAM*/
{0xc080,0xc0ff,	riot1_ram_w},	/*U5 - 6532 RAM*/
{0xc100,0xc17f,	riot2_ram_w},	/*U6 - 6532 RAM*/
{0xc200,0xc27f, riot_0_w},		/*U4 - I/O*/
{0xc280,0xc2ff, riot_1_w},		/*U5 - I/O*/
{0xc300,0xc37f, riot_2_w},		/*U6 - I/O*/
{0xd000,0xd7ff, MWA_ROM},		/*Game Prom(s)*/
{0xd800,0xdfff, ram_256w},		/*RAM - 4x the same 256 Bytes*/
{0xe000,0xefff, MWA_ROM},		/*U2 ROM*/
{0xf000,0xffff, MWA_ROM},		/*U3 ROM*/
MEMORY_END

struct MachineDriver machine_driver_S80 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	},
	S80S_SOUNDCPU,
  },
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  S80_nvram
};

/* s80/80a with sound/speech board */
struct MachineDriver machine_driver_S80SS = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	},
	S80SS_SOUNDCPU,
  },
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  S80_nvram
};

/* s80b - System80a sound only hardware with 2K sound rom */
struct MachineDriver machine_driver_S80B = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	},
	S80S_SOUNDCPU
  },
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  S80_nvram
};

/* s80b - Gen 1 Sound Hardware*/
struct MachineDriver machine_driver_S80BS1 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	}
	S80BS1_SOUNDCPU2
    S80BS1_SOUNDCPU1},
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{S80BS1_SOUND},
  S80_nvram
};

/* s80b - Gen 2 Sound Hardware*/
struct MachineDriver machine_driver_S80BS2 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	}
	S80BS2_SOUNDCPU2
    S80BS2_SOUNDCPU1},
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{S80BS2_SOUND},
  S80_nvram
};

/* s80b - Gen 3 Sound Hardware*/
struct MachineDriver machine_driver_S80BS3 = {
  {
    {
      CPU_M6502, 850000, /* 0.85 Mhz */
      S80_readmem, S80_writemem, NULL, NULL,
	  S80_vblank, 1,
	  NULL, 0
	}
	S80BS3_SOUNDCPU2
    S80BS3_SOUNDCPU1},
  S80_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  S80_init,CORE_EXITFUNC(S80_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{S80BS3_SOUND},
  S80_nvram
};

static void S80_init(void) {
  int ii;

  if (S80locals.initDone)
    S80_exit();
  S80locals.initDone = TRUE;

  memset(&S80locals.initDone, 0, sizeof S80locals.initDone);

  /* init ROM */
  for(ii = 1; ii<4; ii++) {
	memcpy(memory_region(S80_MEMREG_CPU)+0x1000+0x4000*ii, memory_region(S80_MEMREG_CPU)+0x1000, 0x0800);
	memcpy(memory_region(S80_MEMREG_CPU)+0x2000+0x4000*ii, memory_region(S80_MEMREG_CPU)+0x2000, 0x2000);
	if ( core_gameData->gen & GEN_S80B4K ) {
		memcpy(memory_region(S80_MEMREG_CPU)+0x9000, memory_region(S80_MEMREG_CPU)+0x1800, 0x800);
		memcpy(memory_region(S80_MEMREG_CPU)+0xd000, memory_region(S80_MEMREG_CPU)+0x1800, 0x800);
	}
  }

  /* init RAM */
  memset(RIOT0_RAM, 0x00, sizeof RIOT0_RAM);
  memset(RIOT1_RAM, 0x00, sizeof RIOT1_RAM);
  memset(RIOT1_RAM, 0x00, sizeof RIOT1_RAM);

  /* init RIOTS */
  for (ii = 0; ii < sizeof(s80_riot_intf)/sizeof(s80_riot_intf[0]); ii++)
    riot_config(ii, &s80_riot_intf[ii]);

  if (core_init(&S80Data))
	  return;

  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	  if ( core_gameData->gen & (GEN_S80|GEN_S80B) )
		S80S_sinit(S80SS_SCPU);
	  else if ( core_gameData->gen & GEN_S80SS )
		S80SS_sinit(S80SS_SCPU);
	  else if ( core_gameData->gen & (GEN_S80B2K|GEN_S80B4K) )
	    S80Bs_sound_init();
  }

  riot_reset();
}

static void S80_exit(void) {
  /* Sound Enabled? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND)==0) && Machine->sample_rate)
  {
	  if ( core_gameData->gen & GEN_S80SS )
		S80SS_sexit();
	  else if ( core_gameData->gen & (GEN_S80B2K|GEN_S80B4K) )
	    S80Bs_sound_exit();
  }

  riot_unconfig();
  core_exit();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void S80_nvram(void *file, int write) {
	core_nvram(file, write, RAM_256, sizeof RAM_256, 0x00);
#if 0
	/* no NVRAM file ? */
	if ( !file ) {
		int  i;
		for(i=0;i<4;i++) {
			if ( core_getDip(i) )
				break;
		}

		/* no dips are defined by the user, so use default values */
		if (i==4) {
			for(i=0;i<4;i++)
				core_setDip(i, revertByte(opSwitches[i]));
		}
	}
#endif
}
