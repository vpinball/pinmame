/***********************************************/
/* PINMAME - Interface function (Input/Output) */
/* 310301 - Corrected core_getSol problem with <= */
/* 190401 - Corrected flipper hold coil problem */
/***********************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sim.h"
#include "snd_cmd.h"
#include "mech.h"
#include "core.h"
#ifdef VPINMAME
  #include "vpintf.h"
#endif /* VPINMAME */

/* stuff to test VPINMAME */
#if 0
#define VPINMAME
int g_fHandleKeyboard = 1, g_fHandleMechanics = 1;
void OnSolenoid(int nSolenoid, int IsActive) {}
void OnStateChange(int nChange) {}
UINT64 vp_getSolMask64(void) { return -1; }
void vp_updateMech(void) {};
int vp_getDip(int bank) return 0;
#endif

#ifdef VPINMAME
  extern int g_fHandleKeyboard, g_fHandleMechanics;
  extern void OnSolenoid(int nSolenoid, int IsActive);
  extern void OnStateChange(int nChange);
#else /* VPINMAME */
  #define g_fHandleKeyboard  (TRUE)
  #define g_fHandleMechanics (0xff)
  #define OnSolenoid(nSolenoid, IsActive)
  #define OnStateChange(nChange)
  #define vp_getSolMask64() ((UINT64)(-1))
  #define vp_updateMech()
  #define vp_dipnv(x,y)
#endif /* VPINMAME */

/* PinMAME specific options */
tPMoptions pmoptions;

#define drawChar(bm,r,c,b,t) drawChar1(bm,r,c,*((UINT32 *)&b),t)
static void drawChar1(struct mame_bitmap *bitmap, int row, int col, UINT32 bits, int type);
static UINT32 core_initDisplaySize(core_ptLCDLayout layout);

core_tGlobals coreGlobals;
core_tGlobals_dmd coreGlobals_dmd;
core_tGameData *core_gameData = NULL;  /* data about the running game */
int core_bcd2seg[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
static core_tData coreData;
/* makes it easier to swap bits */
                         // 0  1  2  3  4  5  6  7  8  9 10,11,12,13,14,15
const UINT8 core_swapNyb[16] = { 0, 8, 4,12, 2,10, 6,14, 1, 9, 5,13, 3,11, 7,15};

typedef UINT32 tSegRow[17];
typedef struct { int rows, cols; tSegRow *segs; } tSegData;

static struct {
  core_tSeg lastSeg;
  int       firstSimRow;
  tSegData  *segData;
} locals;

/*--------
/ Palette
/---------*/
unsigned char core_palette[(DMD_COLORS+(LAMP_COLORS*2)+7)][3] = {
{/*  0 */ 0x00,0x00,0x00}, /* Background */
/* -- DMD DOT COLORS-- */
{/*  1 */ 0x30,0x00,0x00}, /* "Black" Dot - DMD Background */
{/*  2 */ 0x00,0x00,0x00}, /* Intensity  33% - Filled in @ Run Time */
{/*  3 */ 0x00,0x00,0x00}, /* Intensity  66% - Filled in @ Run Time */
{/*  4 */ 255,104,53},     /* Intensity 100% - Changed @ Run Time to match config vars*/
/* -- PLAYFIELD LAMP COLORS -- */
{/*  5 */ 0x00,0x00,0x00}, /* Black */
{/*  6 */ 0xff,0xff,0xff}, /* White */
{/*  7 */ 0x40,0xff,0x00}, /* green */
{/*  8 */ 0xff,0x00,0x00}, /* Red */
{/*  9 */ 0xff,0x80,0x00}, /* orange */
{/* 10 */ 0xff,0xff,0x00}, /* yellow */
{/* 11 */ 0x00,0x80,0xff}, /* lblue */
{/* 12 */ 0x9f,0x40,0xff}  /* lpurple*/
};

/* Initialize the game palette */
void core_initpalette(unsigned char *game_palette, unsigned short *game_colortable,
                     const unsigned char *color_prom) {
  int rStart = 0xff;
  int gStart = 0xe0;
  int bStart = 0x20;
  int perc66 = 67;
  int perc33 = 33;
  int perc0  = 20;
  int ii;
#ifdef PINMAME_EXT
  if ((pmoptions.dmd_red > 0) || (pmoptions.dmd_green > 0) || (pmoptions.dmd_blue > 0)) {
    rStart = pmoptions.dmd_red;
    gStart = pmoptions.dmd_green;
    bStart = pmoptions.dmd_blue;
  }
  if ((pmoptions.dmd_perc1 > 0) || (pmoptions.dmd_perc2 > 0) || (pmoptions.dmd_perc3 > 0)) {
    perc66 = pmoptions.dmd_perc2;
    perc33 = pmoptions.dmd_perc3;
    perc0  = pmoptions.dmd_perc1;
  }
#endif /* PINMAME_EXT */
  /*-- Autogenerate DMD Color Shades--*/
  core_palette[DMD_DOTOFF][0] = rStart * perc0 / 100;
  core_palette[DMD_DOTOFF][1] = gStart * perc0 / 100;
  core_palette[DMD_DOTOFF][2] = bStart * perc0 / 100;
  core_palette[DMD_DOT33][0]  = rStart * perc33 / 100;
  core_palette[DMD_DOT33][1]  = gStart * perc33 / 100;
  core_palette[DMD_DOT33][2]  = bStart * perc33 / 100;
  core_palette[DMD_DOT66][0]  = rStart * perc66 / 100;
  core_palette[DMD_DOT66][1]  = gStart * perc66 / 100;
  core_palette[DMD_DOT66][2]  = bStart * perc66 / 100;
  core_palette[DMD_DOTON][0]  = rStart;
  core_palette[DMD_DOTON][1]  = gStart;
  core_palette[DMD_DOTON][2]  = bStart;

  /*-- Autogenerate Dark Playfield Lamp Colors --*/
  for (ii = 0; ii < LAMP_COLORS; ii++) {
    /* Reduce by 75% */
    core_palette[DMD_COLORS+LAMP_COLORS+ii][0] = (core_palette[DMD_COLORS+ii][0] * 25) / 100;
    core_palette[DMD_COLORS+LAMP_COLORS+ii][1] = (core_palette[DMD_COLORS+ii][1] * 25) / 100;
    core_palette[DMD_COLORS+LAMP_COLORS+ii][2] = (core_palette[DMD_COLORS+ii][2] * 25) / 100;
  }

  /*-- Autogenerate antialias colours --*/
#if MAMEVER < 3715
#  define vector_flicker flicker
#endif /* MAME_VER */
  {
    int rStep, gStep, bStep;

    rStart = core_palette[DMD_DOTOFF][0];
    gStart = core_palette[DMD_DOTOFF][1];
    bStart = core_palette[DMD_DOTOFF][2];
    rStep = (core_palette[DMD_DOTON][0] * options.vector_flicker / 0xff - rStart) / 6;
    gStep = (core_palette[DMD_DOTON][1] * options.vector_flicker / 0xff - gStart) / 6;
    bStep = (core_palette[DMD_DOTON][2] * options.vector_flicker / 0xff - bStart) / 6;

    for (ii = START_ANTIALIAS+1; ii < (START_ANTIALIAS+1)+6; ii++) {
      rStart += rStep; gStart += gStep; bStart += bStep;
      core_palette[ii][0] = rStart;
      core_palette[ii][1] = gStart;
      core_palette[ii][2] = bStart;
    }
  }
  /*-- Copy the WPC palette to the MAME game palette --*/
  memcpy(game_palette, core_palette, sizeof(core_palette));
}

/*-----------------------------------
/  Generic DMD display handler
/------------------------------------*/
void dmd_draw(struct mame_bitmap *bitmap, tDMDDot dotCol, core_ptLCDLayout layout) {
  UINT32 *dmdColor = &CORE_COLOR(DMD_DOTOFF);
  UINT32 *aaColor  = &CORE_COLOR(START_ANTIALIAS);
  BMTYPE **lines = ((BMTYPE **)bitmap->line) + layout->top;
  int ii, jj;

  memset(&dotCol[layout->start+1][0], 0, sizeof(UINT8)*layout->length);
  memset(&dotCol[0][0], 0, sizeof(UINT8)*layout->length);
  for (ii = 0; ii < layout->start+1; ii++) {
    BMTYPE *line = (*lines++) + layout->left;
    if (ii > 0) {
      for (jj = 0; jj < layout->length; jj++) {
        *line++ = dmdColor[dotCol[ii][jj]];
        if (coreGlobals_dmd.DMDsize > 1)
          *line++ = aaColor[dotCol[ii][jj] + dotCol[ii][jj+1]];
      }
    }
    if (coreGlobals_dmd.DMDsize > 1) {
      int col1 = dotCol[ii][0] + dotCol[ii+1][0];
      line = (*lines++) + layout->left;
      for (jj = 0; jj < layout->length; jj++) {
        int col2 = dotCol[ii][jj+1] + dotCol[ii+1][jj+1];
        *line++ = aaColor[col1];
        *line++ = aaColor[2*(col1 + col2)/5];
        col1 = col2;
      }
    }
  }
  osd_mark_dirty(0,0,layout->length*coreGlobals_dmd.DMDsize,layout->start*coreGlobals_dmd.DMDsize);
}

/*-----------------------------------
/  Generic segement display handler
/------------------------------------*/
void gen_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  core_ptLCDLayout layout = core_gameData->lcdLayout;
  if (layout == NULL) { DBGLOG(("gen_refresh without LCD layout\n")); return; }

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) {
    fillbitmap(bitmap,Machine->pens[0],NULL);
    memset(&locals.lastSeg, -1, sizeof(locals.lastSeg));
  }
  while (layout->length) {
    int zeros = layout->type/16; // dummy zeros
    int left  = layout->left * (locals.segData[layout->type & 0x07].cols + 1) / 2;
    int top   = layout->top  * (locals.segData[0].rows + 1) / 2;
    int ii    = layout->length + zeros;
    int *seg     = ((int *)coreGlobals.segments) + layout->start;
    int *lastSeg = ((int *)locals.lastSeg)       + layout->start;

    while (ii--) {
      int tmpSeg = (ii < zeros) ? ((core_bcd2seg[0]<<8) | (core_bcd2seg[0])) : *seg;
      int tmpType = layout->type & 0x07;

      if (tmpSeg != *lastSeg) {
        tmpSeg >>= layout->type & 0x08;


        if ((tmpType == CORE_SEG87) || (tmpType == CORE_SEG87F)) {
          if ((ii > 0) && (ii % 3 == 0)) { // Handle Comma
            tmpType = CORE_SEG8;
            if ((tmpType == CORE_SEG87F) && tmpSeg) tmpSeg |= 0x80;
          }
          else
            tmpType = CORE_SEG7;
        }
        drawChar1(bitmap,  top, left, tmpSeg, tmpType);
      }
      left += locals.segData[layout->type & 0x07].cols+1;
      seg += 1; lastSeg += 1;
    }
    layout += 1;
  }
  memcpy(locals.lastSeg,coreGlobals.segments,sizeof(locals.lastSeg));
  drawStatus(bitmap,fullRefresh);
}

/*---------------------
/  Update all switches
/----------------------*/
void core_updateSw(int flipEn) {
  /*-- handle flippers--*/
  int flip = core_gameData->hw.flippers;
  int inports[CORE_MAXPORTS];
  int ii;

  if (g_fHandleKeyboard) {
    for (ii = 0; ii < CORE_COREINPORT+(coreData.coreDips+31)/16; ii++)
      inports[ii] = readinputport(ii);

    /*-- buttons **/
    core_setSw(swLLFlip, inports[CORE_FLIPINPORT] & CORE_LLFLIPKEY);
    core_setSw(swLRFlip, inports[CORE_FLIPINPORT] & CORE_LRFLIPKEY);
    if (flip & FLIP_SW(FLIP_UL))     /* have UL switch */
      core_setSw(swULFlip,
                 inports[CORE_FLIPINPORT] & ((flip & FLIP_BUT(FLIP_UL)) ?
                 CORE_ULFLIPKEY : CORE_LLFLIPKEY));
    if (flip & FLIP_SW(FLIP_UR))     /* have UR swicth */
      core_setSw(swURFlip,
                 inports[CORE_FLIPINPORT] & ((flip & FLIP_BUT(FLIP_UR)) ?
                 CORE_URFLIPKEY : CORE_LRFLIPKEY));
  }
  /*-- set switches in matrix for non-fliptronic games --*/
  if (FLIP_SWL(flip))    /* have LL switch in matrix */
    core_setSw(FLIP_SWL(flip), core_getSw(swLLFlip));
  if (FLIP_SWR(flip))    /* have LR switch in matrix */
    core_setSw(FLIP_SWR(flip), core_getSw(swLRFlip));

  /*-- fake solenoids if not CPU controlled --*/
  if ((flip & FLIP_SOL(FLIP_L)) == 0) {
    coreGlobals.solenoids2 &= 0xffffff00;
    if (flipEn) {
      if (core_getSw(swLLFlip))
        coreGlobals.solenoids2 |= CORE_LLFLIPSOLBITS;
      if (core_getSw(swLRFlip))
        coreGlobals.solenoids2 |= CORE_LRFLIPSOLBITS;
	}
  }

  /*-- EOS switches --*/
  if (flip & FLIP_EOS(FLIP_UL)) {
    if (core_getSol(sULFlip)) coreGlobals.flipTimer[0] += 1;
    else                      coreGlobals.flipTimer[0] = 0;
    core_setSw(swULFlipEOS, (coreGlobals.flipTimer[0] >= CORE_FLIPSTROKETIME));
  }
  if (flip & FLIP_EOS(FLIP_UR)) {
    if (core_getSol(sURFlip)) coreGlobals.flipTimer[1] += 1;
    else                      coreGlobals.flipTimer[1] = 0;
    core_setSw(swURFlipEOS, (coreGlobals.flipTimer[1] >= CORE_FLIPSTROKETIME));
  }
  if (flip & FLIP_EOS(FLIP_LL)) {
    if (core_getSol(sLLFlip)) coreGlobals.flipTimer[2] += 1;
    else                      coreGlobals.flipTimer[2] = 0;
    core_setSw(swLLFlipEOS, (coreGlobals.flipTimer[2] >= CORE_FLIPSTROKETIME));
  }
  if (flip & FLIP_EOS(FLIP_LR)) {
    if (core_getSol(sLRFlip)) coreGlobals.flipTimer[3] += 1;
    else                      coreGlobals.flipTimer[3] = 0;
    core_setSw(swLRFlipEOS, (coreGlobals.flipTimer[3] >= CORE_FLIPSTROKETIME));
  }

  /*-- update core dependent switches --*/
  if (coreData.updSw)  coreData.updSw(g_fHandleKeyboard ? inports : NULL);

  /*-- update game dependent switches --*/
  if (g_fHandleMechanics) {
    if (core_gameData->hw.handleMech) core_gameData->hw.handleMech(g_fHandleMechanics);
  }
  /*-- Run simulator --*/
  if (coreGlobals.simAvail)
    sim_run(inports, CORE_COREINPORT+(coreData.coreDips+31)/16,
            (inports[CORE_SIMINPORT] & SIM_SWITCHKEY) == 0,
            (SIM_BALLS(inports[CORE_SIMINPORT])));
  { /*-- check changed solenoids --*/
    UINT64 allSol = core_getAllSol();
    UINT64 chgSol = (allSol ^ coreGlobals.lastSol) & vp_getSolMask64();

    if (chgSol) {
      coreGlobals.lastSol = allSol;
      for (ii = 1; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
        if (chgSol & 0x01) {
          /*-- solenoid has changed state --*/
          OnSolenoid(ii, allSol & 0x01);
          if (!coreGlobals_dmd.dmdOnly && (allSol & 0x01))
			core_textOutf(235,0,BLACK,"%2d",ii);

          if (coreGlobals.soundEn)
            proc_mechsounds(ii, allSol & 0x01);
        }
        chgSol >>= 1;
        allSol >>= 1;
      }
    }
  }

  /*-- check if we should use simulator keys --*/
  if (g_fHandleKeyboard &&
      (!coreGlobals.simAvail || inports[CORE_SIMINPORT] & SIM_SWITCHKEY)) {
    /*-- simulator keys disabled, use row+column keys --*/
    static int lastRow = 0, lastCol = 0;
    int row = 0, col = 0;

    if (((inports[CORE_MANSWINPORT] & CORE_MANSWCOLUMNS) == 0) ||
        ((inports[CORE_MANSWINPORT] & CORE_MANSWROWS) == 0))
      lastRow = lastCol = 0;
    else {
      int bit = 0x0101;

      for (ii = 0; ii < 8; ii++) {
        if (inports[CORE_MANSWINPORT] & CORE_MANSWCOLUMNS & bit) col = ii+1;
        if (inports[CORE_MANSWINPORT] & CORE_MANSWROWS    & bit) row = ii+1;
        bit <<= 1;
      }
      if ((col != lastCol) || (row != lastRow)) {
        core_setSw(col*10+row, !core_getSw(col*10+row));
        lastCol = col; lastRow = row;
      }
    }
  }

#ifdef MAME_DEBUG
  /* Press W and E at the same time to insert a mark in logfile */
  if (g_fHandleKeyboard && ((inports[CORE_MANSWINPORT] & 0x06) == 0x06))
    logerror("\nLogfile Mark\n");
#endif /* MAME_DEBUG */
}

/*--------------------------
/ Write text on the screen
/---------------------------*/
void core_textOut(char *buf, int length, int x, int y, int color) {
  int ii, l;

  l = strlen(buf);
  for (ii = 0; ii < length; ii++) {
    char c = (ii >= l) ? ' ' : buf[ii];

    drawgfx(Machine->scrbitmap, Machine->uifont, c, color-1, 0, 0,
            x + ii * Machine->uifont->width, y+locals.firstSimRow, 0,
            TRANSPARENCY_NONE, 0);
  }
}

/*-----------------------------------
/ Write formatted text on the screen
/------------------------------------*/
void CLIB_DECL core_textOutf(int x, int y, int color, char *text, ...) {
  va_list arg;
  char buf[100];
  char *bufPtr = buf;

  va_start(arg, text);
  vsprintf(buf, text, arg);
  va_end(arg);

  while (*bufPtr) {
    drawgfx(Machine->scrbitmap, Machine->uifont, *bufPtr++, color-1, 0, 0,
            x, y+locals.firstSimRow, 0, TRANSPARENCY_NONE, 0);
    x += Machine->uifont->width;
  }
}

/*--------------------------------------------
/ Draw status display
/ Lamps, Switches, Solenoids, Diagnostic LEDs
/---------------------------------------------*/
void drawStatus(struct mame_bitmap *bitmap, int fullRefresh) {
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int firstRow = locals.firstSimRow;
  int ii, jj, bits;

  /*-- anything to do ? --*/
  if ((coreGlobals_dmd.dmdOnly) ||
      (coreGlobals.soundEn && (!manual_sound_commands(bitmap, &fullRefresh))))
    return;

  /*-----------------
  /  Draw the lamps
  /------------------*/
  if (core_gameData->hw.lampData) {
    core_tLampDisplay *drawData = core_gameData->hw.lampData;
    int startx = drawData->startpos.x;
    int starty = drawData->startpos.y;
    int sizex = drawData->size.x;
    int sizey = drawData->size.y;
    BMTYPE **line = &lines[locals.firstSimRow+startx];
    int num = 0;
    int qq;
    /*For each lamp in the matrix*/
    for (ii = 0; ii < CORE_CUSTLAMPCOL+core_gameData->hw.lampCol; ii++) {
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
        /*For each defined lamp position for this lamp matrix lamp*/
	for (qq = 0; qq < drawData->lamps[num].totnum; qq++) {
	  int color = drawData->lamps[num].lamppos[qq].color;
	  int lampx = drawData->lamps[num].lamppos[qq].x;
	  int lampy = drawData->lamps[num].lamppos[qq].y;
	  line[lampx][starty + lampy] =
	          CORE_COLOR((bits & 0x01) ? color : SHADE(color));
	}
        bits >>= 1;
        num++;
      }
    }
    osd_mark_dirty(starty,  startx + locals.firstSimRow,
                   starty + sizey,
                   startx + sizex + locals.firstSimRow);
    firstRow = startx + sizex + locals.firstSimRow;
  }
  /*-- Normal square lamp matrix layout --*/
  else {
    for (ii = 0; ii < CORE_CUSTLAMPCOL + core_gameData->hw.lampCol; ii++) {
      BMTYPE **line = &lines[firstRow];
      bits = coreGlobals.lampMatrix[ii];

      for (jj = 0; jj < 8; jj++) {
        line[0][ii*2] = CORE_COLOR((bits & 0x01) ? DMD_DOTON : DMD_DOTOFF);
        line += 2;
        bits >>= 1;
      }
    }
    osd_mark_dirty(0,firstRow,16,firstRow+ii*2);
  } /* else */

  firstRow += 20;
  /*-------------------
  /  Draw the switches
  /--------------------*/
  for (ii = 0; ii < CORE_CUSTSWCOL+core_gameData->hw.swCol; ii++) {
    BMTYPE **line = &lines[firstRow];
    bits = coreGlobals.swMatrix[ii];

    for (jj = 0; jj < 8; jj++) {
      line[0][ii*2] = CORE_COLOR((bits & 0x01) ? DMD_DOTON : DMD_DOTOFF);
      line += 2;
      bits >>= 1;
    }
  }
  osd_mark_dirty(0,firstRow,16,firstRow+ii*2);
  firstRow += 20;

  /*------------------------------
  /  Draw Solenoids and Flashers
  /-------------------------------*/
  firstRow += 5;
  {
    BMTYPE **line = &lines[firstRow];
    UINT64 allSol = core_getAllSol();
    for (ii = 0; ii < CORE_FIRSTCUSTSOL+core_gameData->hw.custSol; ii++) {
      line[(ii/8)*2][(ii%8)*2] = CORE_COLOR((allSol & 0x01) ? DMD_DOTON : DMD_DOTOFF);
      allSol >>= 1;
    }
    osd_mark_dirty(0, firstRow, 16,
        firstRow+((CORE_FIRSTCUSTSOL+core_gameData->hw.custSol+7)/8)*2);
  }

  /*------------------------------*/
  /*-- draw diagnostic LEDs     --*/
  /*------------------------------*/
  firstRow += 20;
  {
    BMTYPE **line = &lines[firstRow];
    if (coreData.diagLEDs == CORE_DIAG7SEG)
      drawChar1(bitmap, firstRow, 5, coreGlobals.diagnosticLed,2);
    else {
      bits = coreGlobals.diagnosticLed;

      // Draw LEDS Vertically
      if (coreData.diagLEDs & DIAGLED_VERTICAL) {
        for (ii = 0; ii < (coreData.diagLEDs & ~DIAGLED_VERTICAL); ii++) {
	  line[0][5] = CORE_COLOR((bits & 0x01) ? DMD_DOTON : DMD_DOTOFF);
	  line += 2;
	  bits >>= 1;
	}
      }
      else { // Draw LEDS Horizontally
	for (ii = 0; ii < coreData.diagLEDs; ii++) {
	  line[0][5+ii*2] = CORE_COLOR((bits & 0x01) ? DMD_DOTON : DMD_DOTOFF);
	  bits >>= 1;
	}
      }
    }
  }
  osd_mark_dirty(5, firstRow, 21, firstRow+20);

  firstRow += 25;
  if (core_gameData->gen & GEN_ALLWPC) {
    for (ii = 0; ii < CORE_MAXGI; ii++)
      lines[firstRow][ii*2] = CORE_COLOR(coreGlobals.gi[ii] ? DMD_DOTON : DMD_DOTOFF);
    osd_mark_dirty(0, firstRow, 2*CORE_MAXGI+1, firstRow+2);
  }
  if (coreGlobals.simAvail) sim_draw(fullRefresh, locals.firstSimRow);
  /*-- draw game specific mechanics --*/
  if (core_gameData->hw.drawMech) core_gameData->hw.drawMech((void *)&bitmap->line[locals.firstSimRow]);
}

/*-------------------
/ display handling
/--------------------*/
/* alphanumeric display characters */
static tSegRow segSize1[2][20] = {{ /* alphanumeric display characters */
/*                       all        0001       0002       0004       0008       0010       0020       0040?      0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*  xxxxxxxxxxx    */{0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* xx    x    xx   */{0x14010050,0x04000000,0x00010000,0x00000040,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x x   x   x x   */{0x11010110,0x01000000,0x00010000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x x   x   x x   */{0x11010110,0x01000000,0x00010000,0x00000100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x  x  x  x  x   */{0x10410410,0x00400000,0x00010000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x  x  x  x  x   */{0x10410410,0x00400000,0x00010000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x x x   x   */{0x10111010,0x00100000,0x00010000,0x00001000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x x x   x   */{0x10111010,0x00100000,0x00010000,0x00001000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x    xxx    x   */{0x10054010,0x00040000,0x00010000,0x00004000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxx xxxxx    */{0x05545540,0x00000000,0x00000000,0x00000000,0x00005540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05540000,0x00000000},
/* x    xxx    x   */{0x10054010,0x00000000,0x00000000,0x00000000,0x00000000,0x00004000,0x00010000,0x00040000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x x x   x   */{0x10111010,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00010000,0x00100000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x x x   x   */{0x10111010,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00010000,0x00100000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x  x  x  x  x   */{0x10410410,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00010000,0x00400000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x  x  x  x  x   */{0x10410410,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00010000,0x00400000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x x   x   x x   */{0x11010110,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x00010000,0x01000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x x   x   x x   */{0x11010110,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100,0x00010000,0x01000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* xx    x    xx x */{0x14010051,0x00000000,0x00000000,0x00000000,0x00000000,0x00000040,0x00010000,0x04000000,0x00000000,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000001},
/*  xxxxxxxxxxx  x */{0x05555541,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000001},
/*              x  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 7 segement LED characters */
/*  xxxxxxxxxxx    */{0x05555540,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxxxxxxxx    */{0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x   */{0x10000010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x           x x */{0x10000011,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000001},
/*  xxxxxxxxxxx  x */{0x05555541,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000001},
/*              x  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004}
}};

static tSegRow segSize2[2][12] = {{ /* alphanumeric display characters */
/* | | | | |          all        0001       0002       0004       0008       0010       0020       0040?      0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* xx  x  xx   */{0x14105000,0x04000000,0x00100000,0x00004000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x01000000,0x00100000,0x00010000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00400000,0x00100000,0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxx xxx    */{0x05454000,0x00000000,0x00000000,0x00000000,0x00054000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000},
/* x  xxx  x   */{0x10541000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000,0x00100000,0x00400000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x x x x x   */{0x11111000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00100000,0x01000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* xx  x  xx x */{0x14105100,0x00000000,0x00000000,0x00000000,0x00000000,0x00004000,0x00100000,0x04000000,0x00000000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000100},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000}
},{ /* 7 segement LED characters */
/* | | | | |          all        0001       0002       0004       0008       0010       0020       0040?      0080       0100       0200       0400       0800       1000       2000       4000       8000 */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x   */{0x10001000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x       x x */{0x10001100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000100},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400}
}};
static tSegRow segSize3[2][20] = {{ /* alphanumeric display characters */
{0} /* not possible */
},{ /* 7 segement LED characters */
/* | | | | |     all        0001       0002       0004       0008       0010       0020       0040?      0080 */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00010000},
/*      x  */{0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000}
}};
tSegData segData[2][8] = {{
  {20,15,&segSize1[0][0]}, /* SEG16 */
  {20,15,&segSize1[1][0]}, /* SEG10 (using SEG8 for now) */
  {20,15,&segSize1[1][0]}, /* SEG8 */
  {20,13,&segSize1[1][0]}, /* SEG7 */
  {20,15,&segSize1[1][0]}, /* SEG87 */
  {20,15,&segSize1[1][0]}, /* SEG87F */
  {12, 9,&segSize2[1][0]}, /* SEG7S */
  { 2, 2,NULL}             /* DMD */
},{
  {12,11,&segSize2[0][0]}, /* SEG16 */
  {12,11,&segSize2[1][0]}, /* SEG10 (using SEG8 for now) */
  {12,11,&segSize2[1][0]}, /* SEG8 */
  {12, 9,&segSize2[1][0]}, /* SEG7 */
  {12,11,&segSize2[1][0]}, /* SEG87 */
  {12,11,&segSize2[1][0]}, /* SEG87F */
  { 8, 5,&segSize3[1][0]}, /* SEG7S */
  { 1, 1,NULL}             /* DMD */
}};

/*-- lamp handling --*/
void core_setLamp(UINT8 *lampMatrix, int col, int row) {
  while (col) {
    if (col & 0x01) *lampMatrix |= row;
    col >>= 1;
    lampMatrix += 1;
  }
}

/*------------------------------------------
/  Read the current switch value
/
/  This function returns TRUE for active
/  switches even if the switch is active low.
/-------------------------------------------*/
int core_getSw(int swNo) {
  return (coreGlobals.swMatrix[swNo/10] ^ coreGlobals.invSw[swNo/10]) & (1<<(swNo%10-1));
}

int core_getSwSeq(int swNo) {
  swNo += 7;
  return coreGlobals.swMatrix[swNo/8] & (1<<(swNo%8));
}

int core_getSwCol(int colEn) {
  int ii = 1;
  if (colEn) {
    while ((colEn & 0x01) == 0) {
      colEn >>= 1;
      ii += 1;
    }
  }
  return coreGlobals.swMatrix[ii];
}

/*----------------------
/  Set/reset a switch
/-----------------------*/
void core_setSw(int swNo, int value) {
  coreGlobals.swMatrix[swNo/10] &= ~(1<<(swNo%10-1)); /* clear the bit first */
  coreGlobals.swMatrix[swNo/10] |=  ((value ? 0xff : 0) ^ coreGlobals.invSw[swNo/10]) & (1<<(swNo%10-1));
}

void core_setSwSeq(int swNo, int value) {
  swNo += 7;
  if (value)
    coreGlobals.swMatrix[swNo/8] |= (1<<(swNo%8));
  else
    coreGlobals.swMatrix[swNo/8] &= ~(1<<(swNo%8));
}

/*-------------------------
/  update active low/high
/-------------------------*/
void core_updInvSw(int swNo, int inv) {
  int bit = (1 << (swNo%10-1));

  if (inv)
    inv = bit;
  if ((coreGlobals.invSw[swNo/10] ^ inv) & bit) {
    coreGlobals.invSw[swNo/10] ^= bit;
    coreGlobals.swMatrix[swNo/10] ^= bit;
  }
}

/*-------------------------------------
/  Read the status of a solenoid
/  For the standard solenoids this is
/  the "smoothed" value
/--------------------------------------*/
int core_getSol(int solNo) {
  if (solNo <= CORE_LASTSTDSOL)
    return coreGlobals.solenoids & CORE_SOLBIT(solNo);
  else if (solNo <= CORE_LASTUFLIPSOL) {
    if (core_gameData->gen & GEN_ALLS11)
      return coreGlobals.solenoids & CORE_SOLBIT(solNo);
    else if (core_gameData->gen & GEN_ALLWPC) {
      int mask = 1<<(solNo - CORE_FIRSTUFLIPSOL + 4);
      /*-- flipper coils --*/
      if      ((solNo == sURFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)))
        mask = CORE_URFLIPSOLBITS;
      else if ((solNo == sULFlip) && (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)))
        mask = CORE_ULFLIPSOLBITS;
      return coreGlobals.solenoids2 & mask;
    }
  }
  else if (solNo <= CORE_LASTEXTSOL) {
    if (core_gameData->gen & GEN_WPC95)
      return coreGlobals.solenoids & (1<<((solNo - 13)|4));
    if (core_gameData->gen & GEN_ALLS11)
      return coreGlobals.solenoids2 & (1<<((solNo - CORE_FIRSTEXTSOL)+8));
  }
  else if (solNo <= CORE_LASTLFLIPSOL) {
    int mask = 1<<(solNo - CORE_FIRSTLFLIPSOL);
    /*-- Game must have lower flippers but for symmetry we check anyway --*/
    if      ((solNo == sLRFlip) /*&& (core_gameData->hw.flippers & FLIP_SOL(FLIP_LR))*/)
      mask = CORE_LRFLIPSOLBITS;
    else if ((solNo == sLLFlip) /*&& (core_gameData->hw.flippers & FLIP_SOL(FLIP_LL))*/)
      mask = CORE_LLFLIPSOLBITS;
    return coreGlobals.solenoids2 & mask;
  }
  else if (solNo <= CORE_LASTSIMSOL)
    return sim_getSol(solNo); /* simulated */
  else if (core_gameData->hw.getSol)
    return core_gameData->hw.getSol(solNo);
  return 0;
}

int core_getPulsedSol(int solNo) {
  if (solNo <= CORE_LASTSTDSOL)
    return coreGlobals.pulsedSolState & CORE_SOLBIT(solNo);
  else if ((core_gameData->gen & GEN_WPC95) &&
           (solNo >= CORE_FIRSTEXTSOL) && (solNo <= CORE_LASTEXTSOL))
    return coreGlobals.pulsedSolState & (1<<((solNo-13)|4));
  return core_getSol(solNo); /* sol is not smoothed anyway */
}

/*-------------------------------------------------
/  Get the value of all solenoids in one variable
/--------------------------------------------------*/
UINT64 core_getAllSol(void) {
  UINT64 sol;

  /*-- standard solenoids --*/
  sol = coreGlobals.solenoids;
  if (core_gameData->gen & GEN_ALLWPC) sol &= 0x0fffffff;
  if (core_gameData->gen & GEN_WPC95) {
    UINT64 tmp = coreGlobals.solenoids & 0xf0000000;
    sol |= (tmp<<12)|(tmp<<8);
  }
  if (core_gameData->gen & GEN_ALLS11) sol |= ((UINT64)(coreGlobals.solenoids2 & 0xff00))<<28;
  { /*-- flipper solenoids --*/
    UINT8 lFlip = (coreGlobals.solenoids2 & (CORE_LRFLIPSOLBITS|CORE_LLFLIPSOLBITS));
    UINT8 uFlip = (coreGlobals.solenoids2 & (CORE_URFLIPSOLBITS|CORE_ULFLIPSOLBITS));
    // hold coil is set if either coil is set
    lFlip = lFlip | ((lFlip & 0x05)<<1);
    if (core_gameData->hw.flippers & FLIP_SOL(FLIP_UR))
      uFlip = uFlip | ((uFlip & 0x10)<<1);
    if (core_gameData->hw.flippers & FLIP_SOL(FLIP_UL))
      uFlip = uFlip | ((uFlip & 0x40)<<1);
    sol |= (((UINT64)lFlip)<<44) | (((UINT64)uFlip)<<28);
  }
  /*-- simulated --*/
  sol |= sim_getSol(CORE_FIRSTSIMSOL) ? (((UINT64)1)<<48) : 0;
  /*-- custom --*/
  if (core_gameData->hw.getSol) {
    UINT64 bit = ((UINT64)1)<<(CORE_FIRSTCUSTSOL-1);
    int ii;

    for (ii = 0; ii < core_gameData->hw.custSol; ii++) {
      sol |= core_getSol(CORE_FIRSTCUSTSOL+ii) ? bit : 0;
      bit <<= 1;
    }
  }
  return sol;
}

/*---------------------------------------
/  Get the status of a DIP bank (8 dips)
/-----------------------------------------*/
int core_getDip(int dipBank) {
#ifdef VPINMAME
  return vp_getDIP(dipBank);
#else /* VPINMAME */
  return (readinputport(CORE_COREINPORT+1+dipBank/2)>>((dipBank & 0x01)*8))&0xff;
#endif /* VPINMAME */
}

static void drawChar1(struct mame_bitmap *bitmap, int row, int col, UINT32 bits, int type) {
  tSegData *s = &locals.segData[type];
  UINT32 pixel[20], pen[4];
  int kk,ll;

  pen[0] = CORE_COLOR(0); pen[1] = CORE_COLOR(1); pen[2] = CORE_COLOR(4); pen[3] = CORE_COLOR(4);
  for (kk = 0; kk < s->rows; kk++)
    pixel[kk] = s->segs[kk][0];
  for (kk = 1; bits; kk++, bits >>= 1) {
    if (bits & 0x01)
      for (ll = 0; ll < s->rows; ll++)
        pixel[ll] += s->segs[ll][kk];
  }
  for (kk = 0; kk < s->rows; kk++) {
    BMTYPE *line = &((BMTYPE **)(bitmap->line))[row+kk][col];
    UINT32 p = pixel[kk];

    for (ll = 0; ll < s->cols; ll++) {
      *line++ = pen[(p >> 28) & 0x03];
      p <<= 2;
    }
  }
  osd_mark_dirty(col,row,col+s->cols,row+s->rows);
}

int core_init(core_tData *cd) {
  UINT32 size;
  /*-- init variables --*/
  memset(&coreGlobals, 0, sizeof(coreGlobals));
  memcpy(&coreData, cd, sizeof(coreData));
  memset(&locals.lastSeg, -1, sizeof(locals.lastSeg));

  /*-- init switch matrix --*/
  memcpy(&coreGlobals.invSw, core_gameData->wpc.invSw, sizeof(core_gameData->wpc.invSw));
  memcpy(coreGlobals.swMatrix, coreGlobals.invSw, sizeof(coreGlobals.invSw));

  /*-- command line options --*/
  coreGlobals_dmd.DMDsize = options.antialias ? 2 : 1;
  coreGlobals_dmd.dmdOnly = !options.translucency;
  size = core_initDisplaySize(core_gameData->lcdLayout) >> 16;
  if ((size > CORE_SCREENX) && (coreGlobals_dmd.DMDsize > 1)) {
    /* force small display */
    coreGlobals_dmd.DMDsize = 1;
    core_initDisplaySize(core_gameData->lcdLayout);
  }
  /*-- Sound enabled ? */
  if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate) {
    coreGlobals.soundEn = TRUE;
    /*-- init sound commander --*/
    snd_cmd_init(coreData.sndCmd, coreData.sndHead);
  }
  else
    snd_cmd_init(NULL, NULL);

  /*-- init simulator --*/
  if (g_fHandleKeyboard && core_gameData->simData) {
    int inports[CORE_MAXPORTS];
    int ii;

    for (ii = 0; ii < CORE_COREINPORT+(coreData.coreDips+31)/16; ii++)
      inports[ii] = readinputport(ii);

    coreGlobals.simAvail = sim_init((sim_tSimData *)core_gameData->simData,
                                       inports,CORE_COREINPORT+(coreData.coreDips+31)/16);
  }
  OnStateChange(1); /* We have a lift-off */

/* TOM: this causes to draw the static sim text */
  schedule_full_refresh();

  return 0;
}

void core_exit(void) {
  snd_cmd_exit();
//  mech_exit();
}
static UINT32 core_initDisplaySize(core_ptLCDLayout layout) {
  int maxX = 0, maxY = 0;

  locals.segData = &segData[coreGlobals_dmd.DMDsize == 1][0];
  if (layout) {
    while (layout->length) {
      int tmp;
      if (layout->type == CORE_DMD) tmp = layout->left + layout->length * locals.segData[CORE_DMD].cols + 1;
      else tmp = (layout->left + 2*layout->length) * (locals.segData[layout->type & 0x07].cols + 1) / 2;
      if (tmp > maxX) maxX = tmp;
      if (layout->type == CORE_DMD) tmp = layout->top  + layout->start  * locals.segData[CORE_DMD].rows + 1;
      else tmp = (layout->top + 2) * (locals.segData[0].rows + 1) / 2;
      if (tmp > maxY) maxY = tmp;
      layout += 1;
    }
  }
  else if (coreGlobals_dmd.DMDsize > 1)
#ifndef VPINMAME
    { maxX = 256; maxY = 65; }
#else
    { maxX = 257; maxY = 65; }
#endif /* VPINMAME */
  else
    { maxX = 129; maxY = 33; }
  locals.firstSimRow = maxY + 5;
  if (!coreGlobals_dmd.dmdOnly) {
    maxY += 180;
    if (maxX < 256) maxX = 256;
  }
  set_visible_area(0, maxX-1, 0, maxY);
#ifndef VPINMAME
  if (maxX == 257) maxX = 256;
#endif /* VPINMAME */
  return (maxX<<16) | maxY;
}

void core_nvram(void *file, int write, void *mem, int length) {
  if (write)     osd_fwrite(file, mem, length); /* save */
  else if (file) osd_fread(file,  mem, length); /* load */
  else           memset(mem, 0xff, length);     /* first time */
  mech_nv(file, write); /* save mech positions */
  vp_dipnv(file,write);
}


