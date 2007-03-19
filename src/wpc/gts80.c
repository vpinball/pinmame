/************************************************************************************************
/ Gottlieb System 80 Pinball
/
/ Switches:        Numbering
/   Col0  Col8      Col0 Col1  Col2  Col3  ... Col10 Col11
/ 0 Right Test     -8    00 79 01 80 02 81     101   111
/ 1 Left  COIN1    -7 09 10 89 11 90 12 91     102   112
/ 2 Up    COIN2    -6 19 20    21    22        103   113
/ 3 Down  COIN3    -5 29 30    31    32        104   114
/ 4       START    -4 39 40    41    42        105   115
/ 5       TILT     -3 49 50    51    52        106   116
/ 6                -2 59 60    61    62        107   117
/ 7 Slam           -1 69 70    71    72        108   118
/
/ GameOn    relay is solenoid 10
/ Tilt (GI) relay is solenoid 11
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i86/i86.h"
#include "machine/6532riot.h"
#include "machine/pic8259.h"
#include "core.h"
#include "sndbrd.h"
#include "gts80.h"
#include "gts80s.h"

#define GTS80_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
#define GTS80_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */

static const UINT16 core_ascii2seg[] = {
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
  int    riot0b, riot1a, riot1b, riot2b;
  UINT8  slamSw;
  int    alphaData;
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    seg1, seg2, seg3;
  int    segPos1, segPos2;
  int    vidCmd, vidPlayer;
  struct rectangle vidClip;
  int    buf8212int;
} GTS80locals;
/*----------
/ Pointers
/-----------*/
static UINT8 *GTS80_pRAM, *GTS80_riot0RAM, *GTS80_riot1RAM, *GTS80_riot2RAM;
static UINT8 *GTS80_vRAM;

static void GTS80_irq(int state) {
  cpu_set_irq_line(GTS80_CPU, M6502_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(GTS80_vblank) {
  static UINT32 gameOn;
  GTS80locals.vblankCount += 1;
  /*-- lamps are not strobed so no need to smooth --*/
  /*-- solenoids --*/
  if ((GTS80locals.vblankCount % GTS80_SOLSMOOTH) == 0) {
    // Lamp 0 controls GameOn relay (map as sol 10)
    // Lamp 1 controls Tilt relay (map as sol 11) for S80 & S80A
    gameOn = (coreGlobals.lampMatrix[0] & 0x03);
    if (core_gameData->gen & (GEN_GTS80 | GEN_GTS80A))
      gameOn &= (gameOn ^ 0x06)>>1; // combine tilt & GameOn
    else
      gameOn &= 1; // only GameOn
    coreGlobals.solenoids = GTS80locals.solenoids | (gameOn << 9);
    GTS80locals.solenoids = coreGlobals.pulsedSolState;
    coreGlobals.pulsedSolState = 0;
  }
  /*-- display --*/
  if ((GTS80locals.vblankCount % GTS80_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, GTS80locals.segments, sizeof(coreGlobals.segments));
    memcpy(GTS80locals.segments, GTS80locals.pseg, sizeof(GTS80locals.segments));
  }
  core_updateSw(gameOn & 0x01);
}

/* GTS80 switch numbering, row and column is swapped */
static int GTS80_sw2m(int no) {
  if (no >= 96)    return (no/10)*8+(no%10-1);
  else if (no < 0) return no + 8;
  else { no += 1;  return (no%10)*8 + no/10; }
}

static int GTS80_m2sw(int col, int row) {
  if (col > 9 || (col == 9 && row >= 6)) return col*8+row;
  else if (col < 1)                      return -9 + row;
  else                                   return row*10+col-1;
}
static int GTS80_lamp2m(int no)           { return no+8; }
static int GTS80_m2lamp(int col, int row) { return (col-1)*8+row; }

static SWITCH_UPDATE(GTS80) {
  int isSlammed;
  int invPattern = coreGlobals.invSw[0];
  if (inports) {
    CORE_SETKEYSW(inports[GTS80_COMINPORT], 0x3f, 8);
    // Sound test
    CORE_SETKEYSW(inports[GTS80_COMINPORT] >> 8, 0x10, 0);
    // Set slam switch
    CORE_SETKEYSW(((inports[GTS80_COMINPORT] >> 8) & 0x80) ^ invPattern ? 0x80 : 0,0x80,0);
    if (core_gameData->hw.display & GTS80_DISPVIDEO)
      CORE_SETKEYSW(inports[GTS80_COMINPORT]>>8,0x0f,0);
  }
  /*-- slam tilt --*/
  isSlammed = core_getSw(GTS80_SWSLAMTILT);
  GTS80locals.slamSw = isSlammed ? invPattern : (invPattern ^ 0xff);
  riot6532_set_input_a(1, GTS80locals.slamSw);
  if (core_gameData->hw.display & GTS80_DISPVIDEO) { // Also triggers NMI on video CPU
    cpu_set_irq_line(GTS80_VIDCPU, IRQ_LINE_NMI, isSlammed ? ASSERT_LINE : CLEAR_LINE);
  }
  // sound test
  sndbrd_0_diag(coreGlobals.swMatrix[0] & 0x10);
}

static WRITE_HANDLER(GTS80_sndCmd_w) {
  sndbrd_0_data_w(0, data);
}

/*---------------
/ Riot0 Switch reading
/----------------*/
static READ_HANDLER(riot6532_0a_r) {
  int bits = 0;
  if (GTS80locals.riot1b & 0x80) {
    bits = core_getDip((GTS80locals.riot2b>>4)&0x03);
    return core_revbyte(bits);
  }
  else {
    int ii;
    for (ii = 8; ii > 0; ii--) {
      bits <<= 1;
      if (coreGlobals.swMatrix[ii] & GTS80locals.riot0b) bits |= 1;
    }
    return bits;
  }
}
static WRITE_HANDLER(riot6532_0b_w) {
  GTS80locals.riot0b = data;
  GTS80locals.buf8212int |= (data & 0x03); // for caveman only
}

/*---------------------------
/  Riot1 Display
/----------------------------*/
/* Slam Tilt Switch Input */
static READ_HANDLER(slam_sw_r) {
  return GTS80locals.slamSw;
}

/* BCD version */
static WRITE_HANDLER(riot6532_1aBCD_w) {
  static int reorder[] = { 8, 0, 1, 15, 9, 10, 11, 12, 13, 14, 2, 3, 4, 5, 6, 7 };
  int dispdata = data & 0x0f;
  int pos = reorder[15 - dispdata];
  // Load buffers on rising edge 0x10,0x20,0x40
  if (data & ~GTS80locals.riot1a & 0x10)
    GTS80locals.seg1 = core_bcd2seg9a[GTS80locals.riot1b & 0x0f];
  if (data & ~GTS80locals.riot1a & 0x20)
    GTS80locals.seg2 = core_bcd2seg9a[GTS80locals.riot1b & 0x0f];
  if (data & ~GTS80locals.riot1a & 0x40)
    GTS80locals.seg3 = dispdata < 12 ? core_bcd2seg9a[GTS80locals.riot1b & 0x0f] : core_bcd2seg7a[GTS80locals.riot1b & 0x0f];
  // Middle segments are controlled directly by portb 0x10,0x20,0x40
  // but digit is changed via porta so we set the segs here
  if ((GTS80locals.riot1b & 0x10) == 0) GTS80locals.seg1 |= core_bcd2seg9a[1];
  if ((GTS80locals.riot1b & 0x20) == 0) GTS80locals.seg2 |= core_bcd2seg9a[1];
  if ((GTS80locals.riot1b & 0x40) == 0) GTS80locals.seg3 |= dispdata < 12 ? core_bcd2seg9a[1] : core_bcd2seg7a[1];
  // Set current digit to current value in buffers
  GTS80locals.segments[pos].w |= GTS80locals.seg1;
  GTS80locals.segments[20+pos].w |= GTS80locals.seg2;
  GTS80locals.segments[55-dispdata].w |= GTS80locals.seg3;
  GTS80locals.riot1a = data;
}
static WRITE_HANDLER(riot6532_1bBCD_w) { GTS80locals.riot1b = data; }

/* Alphanumeric */
static WRITE_HANDLER(riot6532_1a_w) {
  if (~data & GTS80locals.riot1a & 0x10)
    GTS80locals.alphaData = (GTS80locals.alphaData & 0xf0) | (GTS80locals.riot1b & 0x0f);
  if (data & ~GTS80locals.riot1a & 0x20)
    GTS80locals.alphaData = (GTS80locals.alphaData & 0x0f) | ((GTS80locals.riot1b & 0x0f)<<4);
  GTS80locals.riot1a = data;
}
static WRITE_HANDLER(riot6532_1b_w) {
  const int alpha = GTS80locals.alphaData  & 0x7f; // shortcut
  if (data & ~GTS80locals.riot1b & 0x10) { // LD1 (falling edge)
    GTS80locals.segments[GTS80locals.segPos1].w |= (GTS80locals.alphaData & 0x80);
    if (alpha == 0x01)
      GTS80locals.segPos1 = -2;
    else if (GTS80locals.segPos1 >= 0)
      GTS80locals.segments[GTS80locals.segPos1].w |= GTS80locals.pseg[GTS80locals.segPos1].w = core_ascii2seg[alpha];
    if (GTS80locals.segPos1 < 19) GTS80locals.segPos1 = (GTS80locals.segPos1 + 1) % 0x14;
  } else if (data & ~GTS80locals.riot1b & 0x20) { // LD2
    GTS80locals.segments[20+GTS80locals.segPos2].w |= (GTS80locals.alphaData & 0x80);
    if (alpha == 0x01)
      GTS80locals.segPos2 = -2;
    else if (GTS80locals.segPos2 >= 0)
      GTS80locals.segments[20+GTS80locals.segPos2].w |= GTS80locals.pseg[20+GTS80locals.segPos2].w = core_ascii2seg[alpha];
    GTS80locals.segPos2 = (GTS80locals.segPos2 + 1) % 0x14;
  }
  GTS80locals.riot1b = data;
}

/*---------------------------
/  Riot2 Solenoids, Lamps and Sound
/----------------------------*/
/* solenoids */
static WRITE_HANDLER(riot6532_2a_w) {
  data = ~data;
  if (data & 0x20)  /* solenoids 1-4 */
    GTS80locals.solenoids |= coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfffffff0) | (1<<(data & 0x03));
  if (data & 0x40)  /* solenoid 5-8 */
    GTS80locals.solenoids |= coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff0f) | (0x10 <<((data>>2) & 0x03));
  /* solenoid 9 */
  GTS80locals.solenoids |= coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfffffeff) | ((data & 0x80)<<1);

  if (core_gameData->hw.soundBoard == SNDBRD_GTS80B) {
    if (data&0x10) GTS80_sndCmd_w(0, (coreGlobals.lampMatrix[0]&0x10) | (data&0x0f));
  } else {
    GTS80_sndCmd_w(0, ((coreGlobals.lampMatrix[1]&0x02)?0x10:0x00) | ((data&0x10) ? (data&0x0f) : 0));
  }
}

static WRITE_HANDLER(riot6532_2b_w) {
  const int column = ((data & 0xf0)>>4)-1; // 4 lamps per column, 12 columns
  GTS80locals.riot2b = data;
  data &= 0x0f;
  if (column >= 0) {
    if (column & 1)
      coreGlobals.lampMatrix[column/2] = (coreGlobals.lampMatrix[column/2] & 0x0f) | (data<<4);
    else
      coreGlobals.lampMatrix[column/2] = (coreGlobals.lampMatrix[column/2] & 0xf0) | data;
    if (column == 11) coreGlobals.lampMatrix[6] = data ^ 0x0f;
    if (core_gameData->hw.display & GTS80_DISPVIDEO) {
      if (column == 1)      GTS80locals.vidPlayer = data;
      else if (column == 3) GTS80locals.vidCmd = data;
      else if ((column == 4) && (data & 0x01)) {
        soundlatch_w(0,GTS80locals.vidCmd); pic8259_0_issue_irq(0);
      }
    }
  }
}

static struct riot6532_interface GTS80_riot6532_intf[] = {
{/* 6532RIOT 0 (0x200) Chip U4 (SWITCH MATRIX)*/
 /* PA0 - PA7 Switch Return  (ROW) */
 /* PB0 - PB7 Switch Strobes (COL) */
 /* in  : A/B, */ riot6532_0a_r, 0,
 /* out : A/B, */ 0, riot6532_0b_w,
 /* irq :      */ GTS80_irq
},{
 /* 6532RIOT 1A (BCD) (0x280) Chip U5 (DISPLAY & SWITCH ENABLE) */
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
 /* in  : A/B, */ slam_sw_r, 0,
 /* out : A/B, */ riot6532_1aBCD_w, riot6532_1bBCD_w,
 /* irq :      */ GTS80_irq
}, {
 /* 6532RIOT 1B (ALPHA) (0x280) Chip U5 (DISPLAY & SWITCH ENABLE) */
 /* in  : A/B, */ slam_sw_r, 0,
 /* out : A/B, */ riot6532_1a_w, riot6532_1b_w,
 /* irq :      */ GTS80_irq
}, {
  /* 6532RIOT 2 (0x300) Chip U6*/
 /* PA0-6: FEED Z28(LS139) (SOL1-8) & SOUND 1-4 */
 /* PA7:   SOL.9 */
 /* PB0-3: LD1-4 */
 /* PB4-7: FEED Z33:LS154 (LAMP LATCHES) + PART OF SWITCH ENABLE */
 /* in  : A/B, */ 0, 0,
 /* out : A/B, */ riot6532_2a_w, riot6532_2b_w,
 /* irq :      */ GTS80_irq
}};
static WRITE_HANDLER(ram_256w) {
  UINT8 *const pMem =  GTS80_pRAM + (offset & 0xff);
  pMem[0x0000] = pMem[0x0100] = pMem[0x0200] = pMem[0x0300] =
  pMem[0x0400] = pMem[0x0500] = pMem[0x0600] = pMem[0x0700] =
  pMem[0x4000] = pMem[0x4100] = pMem[0x4200] = pMem[0x4300] =
  pMem[0x4400] = pMem[0x4500] = pMem[0x4600] = pMem[0x4700] =
  pMem[0x8000] = pMem[0x8100] = pMem[0x8200] = pMem[0x8300] =
  pMem[0x8400] = pMem[0x8500] = pMem[0x8600] = pMem[0x8700] =
  pMem[0xc000] = pMem[0xc100] = pMem[0xc200] = pMem[0xc300] =
  pMem[0xc400] = pMem[0xc500] = pMem[0xc600] = pMem[0xc700] = data;
}

static WRITE_HANDLER(riot6532_0_ram_w) {
  UINT8 *const pMem = GTS80_riot0RAM + (offset & 0x7f);
  pMem[0x0000] = pMem[0x4000] = pMem[0x8000] = pMem[0xc000] = data;
}

static WRITE_HANDLER(riot6532_1_ram_w) {
  UINT8 *const pMem = GTS80_riot1RAM + (offset & 0x7f);
  pMem[0x0000] = pMem[0x4000] = pMem[0x8000] = pMem[0xc000] = data;
}

static WRITE_HANDLER(riot6532_2_ram_w) {
  UINT8 *const pMem = GTS80_riot2RAM + (offset & 0x7f);
  pMem[0x0000] = pMem[0x4000] = pMem[0x8000] = pMem[0xc000] = data;
}

/* for Caveman only */
static READ_HANDLER(in_1cb_r);
static READ_HANDLER(in_1e4_r);

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS80_readmem)
  {0x0000,0x007f, MRA_RAM},    /*U4 - 6532 RAM*/
  {0x0080,0x00ff, MRA_RAM},    /*U5 - 6532 RAM*/
  {0x0100,0x017f, MRA_RAM},    /*U6 - 6532 RAM*/
  {0x01cb,0x01cb, in_1cb_r},
  {0x01e4,0x01e4, in_1e4_r},
  {0x0200,0x027f, riot6532_0_r},        /*U4 - I/O*/
  {0x0280,0x02ff, riot6532_1_r},        /*U5 - I/O*/
  {0x0300,0x037f, riot6532_2_r},        /*U6 - I/O*/
  {0x1000,0x17ff, MRA_ROM},             /*Game Prom(s)*/
  {0x1800,0x1fff, MRA_RAM},            /*RAM - 8x the same 256 Bytes*/
  {0x2000,0x2fff, MRA_ROM},             /*U2 ROM*/
  {0x3000,0x3fff, MRA_ROM},             /*U3 ROM*/

  /* A14 & A15 aren't used) */
  {0x4000,0x407f, MRA_RAM},    /*U4 - 6532 RAM*/
  {0x4080,0x40ff, MRA_RAM},    /*U5 - 6532 RAM*/
  {0x4100,0x417f, MRA_RAM},    /*U6 - 6532 RAM*/
  {0x4200,0x427f, riot6532_0_r},        /*U4 - I/O*/
  {0x4280,0x42ff, riot6532_1_r},        /*U5 - I/O*/
  {0x4300,0x437f, riot6532_2_r},        /*U6 - I/O*/
  {0x5000,0x57ff, MRA_ROM},             /*Game Prom(s)*/
  {0x5800,0x5fff, MRA_RAM},            /*RAM - 8x the same 256 Bytes*/
  {0x6000,0x6fff, MRA_ROM},             /*U2 ROM*/
  {0x7000,0x7fff, MRA_ROM},             /*U3 ROM*/

  {0x8000,0x807f, MRA_RAM},    /*U4 - 6532 RAM*/
  {0x8080,0x80ff, MRA_RAM},    /*U5 - 6532 RAM*/
  {0x8100,0x817f, MRA_RAM},    /*U6 - 6532 RAM*/
  {0x8200,0x827f, riot6532_0_r},        /*U4 - I/O*/
  {0x8280,0x82ff, riot6532_1_r},        /*U5 - I/O*/
  {0x8300,0x837f, riot6532_2_r},        /*U6 - I/O*/
  {0x9000,0x97ff, MRA_ROM},             /*Game Prom(s)*/
  {0x9800,0x9fff, MRA_RAM},            /*RAM - 8x the same 256 Bytes*/
  {0xa000,0xafff, MRA_ROM},             /*U2 ROM*/
  {0xb000,0xbfff, MRA_ROM},             /*U3 ROM*/

  {0xc000,0xc07f, MRA_RAM},    /*U4 - 6532 RAM*/
  {0xc080,0xc0ff, MRA_RAM},    /*U5 - 6532 RAM*/
  {0xc100,0xc17f, MRA_RAM},    /*U6 - 6532 RAM*/
  {0xc200,0xc27f, riot6532_0_r},        /*U4 - I/O*/
  {0xc280,0xc2ff, riot6532_1_r},        /*U5 - I/O*/
  {0xc300,0xc37f, riot6532_2_r},        /*U6 - I/O*/
  {0xd000,0xd7ff, MRA_ROM},             /*Game Prom(s)*/
  {0xd800,0xdfff, MRA_RAM},            /*RAM - 8x the same 256 Bytes*/
  {0xe000,0xefff, MRA_ROM},             /*U2 ROM*/
  {0xf000,0xffff, MRA_ROM},             /*U3 ROM*/
MEMORY_END

static MEMORY_WRITE_START(GTS80_writemem)
  {0x0000,0x007f, riot6532_0_ram_w, &GTS80_riot0RAM}, /*U4 - 6532 RAM*/
  {0x0080,0x00ff, riot6532_1_ram_w, &GTS80_riot1RAM}, /*U5 - 6532 RAM*/
  {0x0100,0x017f, riot6532_2_ram_w, &GTS80_riot2RAM}, /*U6 - 6532 RAM*/
  {0x0200,0x027f, riot6532_0_w},          /*U4 - I/O*/
  {0x0280,0x02ff, riot6532_1_w},          /*U5 - I/O*/
  {0x0300,0x037f, riot6532_2_w},          /*U6 - I/O*/
  {0x1800,0x1fff, ram_256w, &GTS80_pRAM}, /*RAM - 8x the same 256 Bytes*/

  /* A14 & A15 aren't used) */
  {0x4000,0x407f, riot6532_0_ram_w},      /*U4 - 6532 RAM*/
  {0x4080,0x40ff, riot6532_1_ram_w},      /*U5 - 6532 RAM*/
  {0x4100,0x417f, riot6532_2_ram_w},      /*U6 - 6532 RAM*/
  {0x4200,0x427f, riot6532_0_w},          /*U4 - I/O*/
  {0x4280,0x42ff, riot6532_1_w},          /*U5 - I/O*/
  {0x4300,0x437f, riot6532_2_w},          /*U6 - I/O*/
  {0x5800,0x5fff, ram_256w},              /*RAM - 8x the same 256 Bytes*/

  {0x8000,0x807f, riot6532_0_ram_w},      /*U4 - 6532 RAM*/
  {0x8080,0x80ff, riot6532_1_ram_w},      /*U5 - 6532 RAM*/
  {0x8100,0x817f, riot6532_2_ram_w},      /*U6 - 6532 RAM*/
  {0x8200,0x827f, riot6532_0_w},          /*U4 - I/O*/
  {0x8280,0x82ff, riot6532_1_w},          /*U5 - I/O*/
  {0x8300,0x837f, riot6532_2_w},          /*U6 - I/O*/
  {0x9800,0x9fff, ram_256w},              /*RAM - 8x the same 256 Bytes*/

  {0xc000,0xc07f, riot6532_0_ram_w},      /*U4 - 6532 RAM*/
  {0xc080,0xc0ff, riot6532_1_ram_w},      /*U5 - 6532 RAM*/
  {0xc100,0xc17f, riot6532_2_ram_w},      /*U6 - 6532 RAM*/
  {0xc200,0xc27f, riot6532_0_w},          /*U4 - I/O*/
  {0xc280,0xc2ff, riot6532_1_w},          /*U5 - I/O*/
  {0xc300,0xc37f, riot6532_2_w},          /*U6 - I/O*/
  {0xd800,0xdfff, ram_256w},              /*RAM - 8x the same 256 Bytes*/
MEMORY_END

static MACHINE_INIT(gts80) {
  memset(&GTS80locals, 0, sizeof GTS80locals);

  /* init RIOTS */
  riot6532_config(0, &GTS80_riot6532_intf[0]); // switches
  if (core_gameData->hw.display & GTS80_DISPALPHA)
    riot6532_config(1, &GTS80_riot6532_intf[2]); // ALPHA Seg
  else
    riot6532_config(1, &GTS80_riot6532_intf[1]); // BCD Seg
  riot6532_config(2, &GTS80_riot6532_intf[3]); // Lamp + Sol

  GTS80locals.slamSw = 0x80;

  // Interrupt controller
  if (core_gameData->hw.display & GTS80_DISPVIDEO) pic8259_0_config(GTS80_VIDCPU,0);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);

  riot6532_reset();
}

static MACHINE_STOP(gts80) {
  sndbrd_0_exit(); riot6532_unconfig();
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(gts80) {
  core_nvram(file, read_or_write, GTS80_pRAM, 0x100, 0x00);
  if (!read_or_write) {
    int ii;
    for (ii = 1; ii < 8; ii++)
      memcpy(GTS80_pRAM+(ii*0x100), GTS80_pRAM, 0x100);
  }
}

MACHINE_DRIVER_START(gts80)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M6502, 850000)
  MDRV_CPU_MEMORY(GTS80_readmem, GTS80_writemem)
  MDRV_CPU_VBLANK_INT(GTS80_vblank, 1)
  MDRV_SWITCH_UPDATE(GTS80)
  MDRV_CORE_INIT_RESET_STOP(gts80,NULL,gts80)
  MDRV_NVRAM_HANDLER(gts80)
  MDRV_DIPS(42) /* 42 DIPs (32 for the controller board, 8 for the SS- and 2 for the S-Board*/
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

static MACHINE_DRIVER_START(gts80b_ns)
  MDRV_IMPORT_FROM(gts80)
  MDRV_SCREEN_SIZE(320, 200)
  MDRV_VISIBLE_AREA(0, 319, 0, 199)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80b)
  MDRV_IMPORT_FROM(gts80b_ns)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs1)
  MDRV_IMPORT_FROM(gts80b_ns)
  MDRV_IMPORT_FROM(gts80s_b1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs2)
  MDRV_IMPORT_FROM(gts80b_ns)
  MDRV_IMPORT_FROM(gts80s_b2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs3)
  MDRV_IMPORT_FROM(gts80b_ns)
  MDRV_IMPORT_FROM(gts80s_b3)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts80bs3a)
  MDRV_IMPORT_FROM(gts80b_ns)
  MDRV_IMPORT_FROM(gts80s_b3a)
MACHINE_DRIVER_END

/*-------------------------------------------------------
        Additional code for Caveman below. Some notes:

        The memory from 0x0000 to 0x00ff holds the 8086
        IRQ vectors. I found only 4 vectors are filled
        with any useful values: the NMI vector and
        3 others, namely the ones at memory locations:

        0x0008 -> #0x02 (fixed to NMI according to i86.h)
        0x0080 -> #0x20
        0x0084 -> #0x21
        0x009c -> #0x27 (same address as 0x21)

        This is handled by the 8259 interrupt controller now,
        thanks to Martin, so we don't have to worry about it.

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
-----------------------------------------------------------------------*/
static VIDEO_START(gts80vid) {
  tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
  return (tmpbitmap == 0);
}

static WRITE_HANDLER(vram_w) {
  int y = offset / 64 + GTS80locals.vidClip.min_y;
  int x = (offset % 64)<<2;

  plot_pixel(tmpbitmap, x++, y, 5 + ((data>>6) & 0x03));
  plot_pixel(tmpbitmap, x++, y, 5 + ((data>>4) & 0x03));
  plot_pixel(tmpbitmap, x++, y, 5 + ((data>>2) & 0x03));
  plot_pixel(tmpbitmap, x,   y, 5 + (data & 0x03));

  GTS80_vRAM[offset] = data;
}

static PINMAME_VIDEO_UPDATE(caveman_update) {
  if (GTS80locals.vidClip.max_y == 0) {
    GTS80locals.vidClip.min_x = layout->left;
    GTS80locals.vidClip.max_x = layout->left + layout->length;
    GTS80locals.vidClip.min_y = layout->top;
    GTS80locals.vidClip.max_y = layout->top + layout->start;
  }
  copybitmap(bitmap,tmpbitmap,0,0,0,0,&GTS80locals.vidClip,TRANSPARENCY_NONE,0);
  return 0;
}

/* port 0xx = Interrupt Controller */
static WRITE_HANDLER(port00xw) { pic8259_0_w(offset>>1,data);   }
static READ_HANDLER(port002r)  { return pic8259_0_r(offset>>1); }

/* port 1xx = CRTC HD46505 */
static WRITE_HANDLER(port10xw) { DBGLOG(("HD46505 w %x = %02x\n", offset/2, data)); }
static READ_HANDLER(port102r)  { DBGLOG(("HD46505 read\n")); return 0; }

// port200 & port300 is connected to an 8212 chip
// The 8212 is buffer but it can generate an interrupt when read.
// In caveman it set up so that the interrupt pin is connected to D6 & D7 on port 200.
// Port 200 is connected to switch strobe 0 and D7
// Port 300 is connected to switch strobe 1 and D6
// Accessing the port clears the interrupt
// buf8212int contains port200 status in D0 and port300 status in D1
// (was easier to handle that way)

static void GTS80_vidStatus(int strobe, int data) {
  int ii;
  data <<= (strobe-1);
  for (ii = 1; ii < 8; ii++, data >>= 1)
    coreGlobals.swMatrix[ii] = (coreGlobals.swMatrix[ii] & ~strobe) | (data & strobe);
  GTS80locals.buf8212int &= ~strobe; // clear interrupt pin
}

/* output to game switches, row 0 */
static WRITE_HANDLER(port200w) { if (data) GTS80_vidStatus(1,data); }
/* output to game switches, row 1 */
static WRITE_HANDLER(port300w) { if (data) GTS80_vidStatus(2,data); }
// Check if the main CPU has read the data in the 8212s
static READ_HANDLER(port200r) {
  const int stat = ((GTS80locals.buf8212int & 0x01)<<7) |
                   ((GTS80locals.buf8212int & 0x02)<<5);
  // reset switch lines
  if (GTS80locals.buf8212int & 0x01) GTS80_vidStatus(1,0);
  if (GTS80locals.buf8212int & 0x02) GTS80_vidStatus(2,0);
  GTS80locals.buf8212int = 0; // reading clears int pin
  return stat;
}

/* joystick + player */
static READ_HANDLER(port400r) {
  return (coreGlobals.swMatrix[0] & 0x0f) | (GTS80locals.vidPlayer<<4);
}

/* set up game colors. 4 colors at a time only, out of 16 possible! */
static WRITE_HANDLER(port50xw) {
/* we'll need a screenshot from the actual game to set the palette right! */
  static const UINT32 rgb[16] = {       // R G B
    0x000000,0x9f0000,0x009f00,0x9f9f00,0x00009f,0x9f009f,0x009f9f,0x9f9f9f,
    0x000000,0xff0000,0x00ff00,0xffff00,0x0000ff,0xff00ff,0x00ffff,0xffffff
  };
  const UINT32 color = rgb[data];
  palette_set_color(5 + offset/2, color>>16, color>>8, color);
}

static READ_HANDLER(in_1cb_r) { DBGLOG(("read Caveman input 1CB\n")); return 0; }
static READ_HANDLER(in_1e4_r) { DBGLOG(("read Caveman input 1E4\n")); return 0; }

static PORT_READ_START(video_readport)
  { 0x002, 0x002, port002r },
  { 0x102, 0x102, port102r },
  { 0x200, 0x200, port200r },
  { 0x300, 0x300, soundlatch_r },
  { 0x400, 0x400, port400r },
PORT_END

static PORT_WRITE_START(video_writeport)
  { 0x000, 0x002, port00xw },
  { 0x100, 0x102, port10xw },
  { 0x200, 0x200, port200w },
  { 0x300, 0x300, port300w },
  { 0x500, 0x506, port50xw },
PORT_END

static MEMORY_READ_START(video_readmem)
  {0x00000,0x0058e, MRA_RAM},
  {0x007cf,0x007fe, MRA_RAM},
  {0x02000,0x05fff, MRA_RAM},
  {0x08000,0x0ffff, MRA_ROM},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(video_writemem)
  {0x00000,0x0058e, MWA_RAM},
  {0x007cf,0x007fe, MWA_RAM},
  {0x02000,0x05fff, vram_w, &GTS80_vRAM},
  {0x08000,0x0ffff, MWA_ROM},
  {0xf8000,0xfffff, MWA_ROM},
MEMORY_END

/* 4 x 7 BCD + Ball,Credit */
core_tLCDLayout GTS80_dispCaveman[] = {
  {0, 0, 2, 7,CORE_SEG98F}, {0,16, 9, 7,CORE_SEG98F},
  {4, 0,22, 7,CORE_SEG98F}, {4,16,29, 7,CORE_SEG98F},
  DISP_SEG_CREDIT(40,41,CORE_SEG9), DISP_SEG_BALLS(42,43,CORE_SEG9),
  {70,0,240,256,CORE_VIDEO,(void *)caveman_update},{0}
};

MACHINE_DRIVER_START(gts80vid)
  MDRV_IMPORT_FROM(gts80ss)
  MDRV_CPU_ADD_TAG("vcpu", I86, 5000000)
  MDRV_CPU_MEMORY(video_readmem, video_writemem)
  MDRV_CPU_PORTS(video_readport, video_writeport)
  /* video hardware */
  MDRV_SCREEN_SIZE(640, 400)
  MDRV_VISIBLE_AREA(0, 255, 0, 399)
  MDRV_GFXDECODE(0)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE)
  MDRV_VIDEO_START(gts80vid)
MACHINE_DRIVER_END
