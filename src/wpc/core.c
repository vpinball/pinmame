/***********************************************/
/* PINMAME - Interface function (Input/Output) */
/***********************************************/
#include <stdarg.h>
#include "driver.h"
#include "sim.h"
#include "snd_cmd.h"
#include "mech.h"
#include "core.h"

/* stuff to test VPINMAME */
#if 0
#define VPINMAME
int g_fHandleKeyboard = 1, g_fHandleMechanics = 1;
void OnSolenoid(int nSolenoid, int IsActive) {}
void OnStateChange(int nChange) {}
UINT64 vp_getSolMask64(void) { return -1; }
void vp_updateMech(void) {};
int vp_getDip(int bank) { return 0; }
void vp_setDIP(int bank, int value) { }
#endif

#ifdef VPINMAME
  #include "vpintf.h"
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
  #define vp_setDIP(x,y)
#endif /* VPINMAME */

/* PinMAME specific options */
tPMoptions pmoptions;

#define drawChar(bm,r,c,b,t) drawChar1(bm,r,c,*((UINT32 *)&b),t)
static void drawChar1(struct mame_bitmap *bitmap, int row, int col, UINT32 bits, int type);
static UINT32 core_initDisplaySize(const struct core_dispLayout *layout);
static VIDEO_UPDATE(core_status);

core_tGlobals     coreGlobals;
struct pinMachine *coreData;
const core_tGameData *core_gameData = NULL;  /* data about the running game */
const int core_bcd2seg7[16] = {
/* 0    1    2    3    4    5    6    7    8    9  */
  0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
const int core_bcd2seg9[16] = {
/* 0     1    2    3    4    5    6    7    8    9  */
  0x3f,0x80,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
#ifdef MAME_DEBUG
/* A    B    C    D    E */
 ,0x77,0x7c,0x39,0x5e,0x79
#endif /* MAME_DEBUG */
};
/* makes it easier to swap bits */
                              // 0  1  2  3  4  5  6  7  8  9 10,11,12,13,14,15
const UINT8 core_swapNyb[16] = { 0, 8, 4,12, 2,10, 6,14, 1, 9, 5,13, 3,11, 7,15};

typedef UINT32 tSegRow[17];
typedef struct { int rows, cols; tSegRow *segs; } tSegData;

static struct {
  core_tSeg lastSeg;
  int       firstSimRow;
  tSegData  *segData;
  void      *timers[5];
  int       displaySize; // 1=compact 2=normal
} locals;

/*--------
/ Palette
/---------*/
static unsigned char core_palette[(DMD_COLORS+(LAMP_COLORS*2)+7)][3] = {
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
static PALETTE_INIT(core) {
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
  if ((pmoptions.dmd_perc0 > 0) || (pmoptions.dmd_perc33 > 0) || (pmoptions.dmd_perc66 > 0)) {
    perc66 = pmoptions.dmd_perc66;
    perc33 = pmoptions.dmd_perc33;
    perc0  = pmoptions.dmd_perc0;
  }
#endif /* PINMAME_EXT */
  /*-- Copy the WPC palette to the MAME game palette --*/
  memcpy(palette, core_palette, sizeof(core_palette));

  /*-- Autogenerate DMD Color Shades--*/
  palette[DMD_DOTOFF*3+0] = rStart * perc0 / 100;
  palette[DMD_DOTOFF*3+1] = gStart * perc0 / 100;
  palette[DMD_DOTOFF*3+2] = bStart * perc0 / 100;
  palette[DMD_DOT33*3+0]  = rStart * perc33 / 100;
  palette[DMD_DOT33*3+1]  = gStart * perc33 / 100;
  palette[DMD_DOT33*3+2]  = bStart * perc33 / 100;
  palette[DMD_DOT66*3+0]  = rStart * perc66 / 100;
  palette[DMD_DOT66*3+1]  = gStart * perc66 / 100;
  palette[DMD_DOT66*3+2]  = bStart * perc66 / 100;
  palette[DMD_DOTON*3+0]  = rStart;
  palette[DMD_DOTON*3+1]  = gStart;
  palette[DMD_DOTON*3+2]  = bStart;

  /*-- Autogenerate Dark Playfield Lamp Colors --*/
  for (ii = 0; ii < LAMP_COLORS; ii++) {
    /* Reduce by 75% */
    palette[(DMD_COLORS+LAMP_COLORS+ii)*3+0] = (palette[(DMD_COLORS+ii)*3+0] * 25) / 100;
    palette[(DMD_COLORS+LAMP_COLORS+ii)*3+1] = (palette[(DMD_COLORS+ii)*3+1] * 25) / 100;
    palette[(DMD_COLORS+LAMP_COLORS+ii)*3+2] = (palette[(DMD_COLORS+ii)*3+2] * 25) / 100;
  }

  { /*-- Autogenerate antialias colours --*/
    int rStep, gStep, bStep;

    rStart = palette[DMD_DOTOFF*3+0];
    gStart = palette[DMD_DOTOFF*3+1];
    bStart = palette[DMD_DOTOFF*3+2];
    rStep = (palette[DMD_DOTON*3+0] * pmoptions.dmd_antialias / 100 - rStart) / 6;
    gStep = (palette[DMD_DOTON*3+1] * pmoptions.dmd_antialias / 100 - gStart) / 6;
    bStep = (palette[DMD_DOTON*3+2] * pmoptions.dmd_antialias / 100 - bStart) / 6;

    for (ii = START_ANTIALIAS+1; ii < (START_ANTIALIAS+1)+6; ii++) {
      palette[ii*3+0] = rStart;
      palette[ii*3+1] = gStart;
      palette[ii*3+2] = bStart;
      rStart += rStep; gStart += gStep; bStart += bStep;
    }
  }
}

/*-----------------------------------
/  Generic DMD display handler
/------------------------------------*/
void video_update_core_dmd(struct mame_bitmap *bitmap, const struct rectangle *cliprect, tDMDDot dotCol, const struct core_dispLayout *layout) {
  UINT32 *dmdColor = &CORE_COLOR(DMD_DOTOFF);
  UINT32 *aaColor  = &CORE_COLOR(START_ANTIALIAS);
  BMTYPE **lines = ((BMTYPE **)bitmap->line) + (layout->top*locals.displaySize);
  int ii, jj;

  memset(&dotCol[layout->start+1][0], 0, sizeof(dotCol[0][0])*layout->length+1);
  memset(&dotCol[0][0], 0, sizeof(dotCol[0][0])*layout->length+1); // clear above
  for (ii = 0; ii < layout->start+1; ii++) {
    BMTYPE *line = (*lines++) + (layout->left*locals.displaySize);
    dotCol[ii][layout->length] = 0;
    if (ii > 0) {
      for (jj = 0; jj < layout->length; jj++) {
        *line++ = dmdColor[dotCol[ii][jj]];
        if (locals.displaySize > 1)
          *line++ = aaColor[dotCol[ii][jj] + dotCol[ii][jj+1]];
      }
    }
    if (locals.displaySize > 1) {
      int col1 = dotCol[ii][0] + dotCol[ii+1][0];
      line = (*lines++) + (layout->left*locals.displaySize);
      for (jj = 0; jj < layout->length; jj++) {
        int col2 = dotCol[ii][jj+1] + dotCol[ii+1][jj+1];
        *line++ = aaColor[col1];
        *line++ = aaColor[2*(col1 + col2)/5];
        col1 = col2;
      }
    }
  }
  osd_mark_dirty(layout->left*locals.displaySize,layout->top*locals.displaySize,
                 (layout->left+layout->length)*locals.displaySize,(layout->top+layout->start)*locals.displaySize);
}

/*-----------------------------------
/  Generic segement display handler
/------------------------------------*/
static VIDEO_UPDATE(core_gen) {
  const struct core_dispLayout *layout = core_gameData->lcdLayout;
  if (layout == NULL) { DBGLOG(("gen_refresh without LCD layout\n")); return; }
  for (; layout->length; layout += 1) {
    if (layout->update) if (!layout->update(bitmap,cliprect,layout)) continue;
    {
      int zeros = layout->type/32; // dummy zeros
      int left  = layout->left * (locals.segData[layout->type & CORE_SEGMASK].cols + 1) / 2;
      int top   = layout->top  * (locals.segData[0].rows + 1) / 2;
      int ii    = layout->length + zeros;
      int *seg     = ((int *)coreGlobals.segments) + layout->start;
      int *lastSeg = ((int *)locals.lastSeg)       + layout->start;
      int step     = (layout->type & CORE_SEGREV) ? -1 : 1;

      if (step < 0) { seg += ii-1; lastSeg += ii-1; }
      while (ii--) {
        int tmpSeg = (ii < zeros) ? ((core_bcd2seg7[0]<<8) | (core_bcd2seg7[0])) : *seg;
        int tmpType = layout->type & CORE_SEGMASK;

        if (tmpSeg != *lastSeg) {
          tmpSeg >>= (layout->type & CORE_SEGHIBIT) ? 8 : 0;

          if ((tmpType == CORE_SEG87) || (tmpType == CORE_SEG87F)) {
            if ((ii > 0) && (ii % 3 == 0)) { // Handle Comma
              tmpType = CORE_SEG8;
              if ((tmpType == CORE_SEG87F) && tmpSeg) tmpSeg |= 0x80;
            }
            else
              tmpType = CORE_SEG7;
          }
          else if (tmpType == CORE_SEG9) tmpSeg |= (tmpSeg & 0x80)<<1;
          drawChar1(bitmap,  top, left, tmpSeg, tmpType);
        }
        left += locals.segData[layout->type & 0x0f].cols+1;
        seg += step; lastSeg += step;
      }
    }
  }
  memcpy(locals.lastSeg,coreGlobals.segments,sizeof(locals.lastSeg));
  video_update_core_status(bitmap,cliprect);
}

/*---------------------
/  Update all switches
/----------------------*/
void core_updateSw(int flipEn) {
  /*-- handle flippers--*/
  int flip = core_gameData->hw.flippers;
  int inports[CORE_MAXPORTS];
  UINT8 swFlip = coreGlobals.swMatrix[(core_gameData->gen & GEN_GTS3) ? 15 : CORE_FLIPPERSWCOL] &
                 (CORE_SWULFLIPBUTBIT|CORE_SWURFLIPBUTBIT|CORE_SWLLFLIPBUTBIT|CORE_SWLRFLIPBUTBIT);
  int ii;

  if (g_fHandleKeyboard) {
    for (ii = 0; ii < CORE_COREINPORT+(coreData->coreDips+31)/16; ii++)
      inports[ii] = readinputport(ii);

    /*-- buttons --*/
    swFlip = 0;
    if (inports[CORE_FLIPINPORT] & CORE_LLFLIPKEY) swFlip |= CORE_SWLLFLIPBUTBIT;
    if (inports[CORE_FLIPINPORT] & CORE_LRFLIPKEY) swFlip |= CORE_SWLRFLIPBUTBIT;
    if (flip & FLIP_SW(FLIP_UL)) {    /* have UL switch */
      if (flip & FLIP_BUT(FLIP_UL))
        { if (inports[CORE_FLIPINPORT] & CORE_ULFLIPKEY) swFlip |= CORE_SWULFLIPBUTBIT; }
      else
        { if (inports[CORE_FLIPINPORT] & CORE_LLFLIPKEY) swFlip |= CORE_SWULFLIPBUTBIT; }
    }
    if (flip & FLIP_SW(FLIP_UR)) {    /* have UL switch */
      if (flip & FLIP_BUT(FLIP_UR))
        { if (inports[CORE_FLIPINPORT] & CORE_URFLIPKEY) swFlip |= CORE_SWURFLIPBUTBIT; }
      else
        { if (inports[CORE_FLIPINPORT] & CORE_LRFLIPKEY) swFlip |= CORE_SWURFLIPBUTBIT; }
    }
  }

  /*-- set switches in matrix for non-fliptronic games --*/
  if (FLIP_SWL(flip)) core_setSw(FLIP_SWL(flip), swFlip & CORE_SWLLFLIPBUTBIT);
  if (FLIP_SWR(flip)) core_setSw(FLIP_SWR(flip), swFlip & CORE_SWLRFLIPBUTBIT);

  /*-- fake solenoids if not CPU controlled --*/
  if ((flip & FLIP_SOL(FLIP_L)) == 0) {
    coreGlobals.solenoids2 &= 0xffffff00;
    if (flipEn) {
      if (swFlip & CORE_SWLLFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LLFLIPSOLBITS;
      if (swFlip & CORE_SWLRFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LRFLIPSOLBITS;
    }
  }

  /*-- EOS switches --*/
  if (flip & FLIP_EOS(FLIP_UL)) {
    if (core_getSol(sULFlip)) coreGlobals.flipTimer[0] += 1;
    else                      coreGlobals.flipTimer[0] = 0;
    if (coreGlobals.flipTimer[0] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWULFLIPEOSBIT;
  }
  if (flip & FLIP_EOS(FLIP_UR)) {
    if (core_getSol(sURFlip)) coreGlobals.flipTimer[1] += 1;
    else                      coreGlobals.flipTimer[1] = 0;
    if (coreGlobals.flipTimer[1] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWURFLIPEOSBIT;
  }
  if (flip & FLIP_EOS(FLIP_LL)) {
    if (core_getSol(sLLFlip)) coreGlobals.flipTimer[2] += 1;
    else                      coreGlobals.flipTimer[2] = 0;
    if (coreGlobals.flipTimer[2] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWLLFLIPEOSBIT;
  }
  if (flip & FLIP_EOS(FLIP_LR)) {
    if (core_getSol(sLRFlip)) coreGlobals.flipTimer[3] += 1;
    else                      coreGlobals.flipTimer[3] = 0;
    if (coreGlobals.flipTimer[3] >= CORE_FLIPSTROKETIME) swFlip |= CORE_SWLRFLIPEOSBIT;
  }
  coreGlobals.swMatrix[(core_gameData->gen & GEN_GTS3) ? 15 : CORE_FLIPPERSWCOL] = swFlip;

  /*-- update core dependent switches --*/
  if (coreData->updSw)  coreData->updSw(g_fHandleKeyboard ? inports : NULL);

  /*-- update game dependent switches --*/
  if (g_fHandleMechanics) {
    if (core_gameData->hw.handleMech) core_gameData->hw.handleMech(g_fHandleMechanics);
  }
  /*-- Run simulator --*/
  if (coreGlobals.simAvail)
    sim_run(inports, CORE_COREINPORT+(coreData->coreDips+31)/16,
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
          if (!pmoptions.dmd_only && (allSol & 0x01))
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
        coreGlobals.swMatrix[col] ^= (1<<(row-1));
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
static VIDEO_UPDATE(core_status) {
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int firstRow = locals.firstSimRow;
  int ii, jj, bits;
  BMTYPE dotColor[2];


  /*-- anything to do ? --*/
  if ((pmoptions.dmd_only) ||
      (coreGlobals.soundEn && (!manual_sound_commands(bitmap))))
    return;

  dotColor[0] = CORE_COLOR(DMD_DOTOFF); dotColor[1] = CORE_COLOR(DMD_DOTON);
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
        line[0][ii*2] = dotColor[bits & 0x01];
        line += 2; bits >>= 1;
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
      line[0][ii*2] = dotColor[bits & 0x01];
      line += 2; bits >>= 1;
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
      line[(ii/8)*2][(ii%8)*2] = dotColor[allSol & 0x01];
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
    if (coreData->diagLEDs == 0xff) /* 7 SEG */
      drawChar1(bitmap, firstRow, 5, coreGlobals.diagnosticLed,2);
    else {
      bits = coreGlobals.diagnosticLed;

      // Draw LEDS Vertically
      if (coreData->diagLEDs & DIAGLED_VERTICAL) {
        for (ii = 0; ii < (coreData->diagLEDs & ~DIAGLED_VERTICAL); ii++) {
	  line[0][5] = dotColor[bits & 0x01];
	  line += 2; bits >>= 1;
	}
      }
      else { // Draw LEDS Horizontally
	for (ii = 0; ii < coreData->diagLEDs; ii++) {
	  line[0][5+ii*2] = dotColor[bits & 0x01];
	  bits >>= 1;
	}
      }
    }
  }
  osd_mark_dirty(5, firstRow, 21, firstRow+20);

  firstRow += 25;
  if (core_gameData->gen & GEN_ALLWPC) {
    for (ii = 0; ii < CORE_MAXGI; ii++)
      lines[firstRow][ii*2] = dotColor[coreGlobals.gi[ii]>0];
    osd_mark_dirty(0, firstRow, 2*CORE_MAXGI+1, firstRow+2);
  }
  if (coreGlobals.simAvail) sim_draw(locals.firstSimRow);
  /*-- draw game specific mechanics --*/
  if (core_gameData->hw.drawMech) core_gameData->hw.drawMech((void *)&bitmap->line[locals.firstSimRow]);
}

/*-------------------
/ display handling
/--------------------*/
/* alphanumeric display characters */
static tSegRow segSize1[3][20] = {{ /* alphanumeric display characters */
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
/*              x  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004}
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
},{ /* 9 segement LED characters */
/*  xxxxxxxxxxx    */{0x05555540,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000010,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00010000,0x00000000,0x00000000},
/*  xxxxxxxxxxx    */{0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x   */{0x10010010,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000000},
/* x     x     x x */{0x10010011,0x00000000,0x00000000,0x00000010,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00010000,0x00000001},
/*  xxxxxxxxxxx  x */{0x05555541,0x00000000,0x00000000,0x00000000,0x05555540,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000001},
/*              x  */{0x00000004,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000004}
}};

static tSegRow segSize2[3][12] = {{ /* alphanumeric display characters */
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
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400}
},{ /* 7 segement LED characters */
/* | | | | |          all        0001       0002       0004       0008       0010       0020       0040       0080 */
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
},{ /* 9 segement LED characters */
/* | | | | |          all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200 */
/*  xxxxxxx    */{0x05554000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00100000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00100000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00100000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00001000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00100000,0x00000000,0x00000000},
/*  xxxxxxx    */{0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x   */{0x10101000,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00100000,0x00000000},
/* x   x   x x */{0x10101100,0x00000000,0x00000000,0x00001000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x00100000,0x00000100},
/*  xxxxxxx  x */{0x05554100,0x00000000,0x00000000,0x00000000,0x05554000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000100},
/*          x  */{0x00000400,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000400}
}};
static tSegRow segSize3[3][10] = {{ /* alphanumeric display characters */
{0} /* not possible */
},{ /* 7 segement LED characters */
/* | | |          all        0001       0002       0004       0008       0010       0020       0040       0080 */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/* x   x   */{0x10100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00010000},
/*      x  */{0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000}
},{ /* 9 segement LED characters */
/* | | |          all        0001       0002       0004       0008       0010       0020       0040       0080       0100       0200 */
/*  xxx    */{0x05400000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x01000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x01000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00100000,0x00000000,0x00000000,0x00000000,0x10000000,0x00000000,0x01000000,0x00000000,0x00000000},
/*  xxx    */{0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000},
/* x x x   */{0x11100000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x01000000,0x00000000},
/* x x x x */{0x11110000,0x00000000,0x00000000,0x00100000,0x00000000,0x10000000,0x00000000,0x00000000,0x00000000,0x01000000,0x00010000},
/*  xxx  x */{0x05410000,0x00000000,0x00000000,0x00000000,0x05400000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00010000},
/*      x  */{0x00040000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00000000,0x00040000}
}};
static tSegData segData[2][9] = {{
  {20,15,&segSize1[0][0]}, /* SEG16 */
  {20,13,&segSize1[2][0]}, /* SEG10 */
  {20,15,&segSize1[2][0]}, /* SEG9 */
  {20,15,&segSize1[1][0]}, /* SEG8 */
  {20,13,&segSize1[1][0]}, /* SEG7 */
  {20,15,&segSize1[1][0]}, /* SEG87 */
  {20,15,&segSize1[1][0]}, /* SEG87F */
  {12, 9,&segSize2[1][0]}, /* SEG7S */
  { 2, 2,NULL}             /* DMD */
},{
  {12,11,&segSize2[0][0]}, /* SEG16 */
  {12, 9,&segSize2[2][0]}, /* SEG10 */
  {12,11,&segSize2[2][0]}, /* SEG9 */
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

/*-- "normal" switch/lamp numbering (1-64) --*/
int core_swSeq2m(int no) { return no+7; }
int core_m2swSeq(int col, int row) { return col*8+row-7; }

/*------------------------------------------
/  Read the current switch value
/
/  This function returns TRUE for active
/  switches even if the switch is active low.
/-------------------------------------------*/
int core_getSw(int swNo) {
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  return (coreGlobals.swMatrix[swNo/8] ^ coreGlobals.invSw[swNo/8]) & (1<<(swNo%8));
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
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  coreGlobals.swMatrix[swNo/8] &= ~(1<<(swNo%8)); /* clear the bit first */
  coreGlobals.swMatrix[swNo/8] |=  ((value ? 0xff : 0) ^ coreGlobals.invSw[swNo/8]) & (1<<(swNo%8));
}

/*-------------------------
/  update active low/high
/-------------------------*/
void core_updInvSw(int swNo, int inv) {
  int bit;
  if (coreData->sw2m) swNo = coreData->sw2m(swNo); else swNo = (swNo/10)*8+(swNo%10-1);
  bit = (1 << (swNo%8));

  if (inv)
    inv = bit;
  if ((coreGlobals.invSw[swNo/8] ^ inv) & bit) {
    coreGlobals.invSw[swNo/8] ^= bit;
    coreGlobals.swMatrix[swNo/8] ^= bit;
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

static MACHINE_INIT(core) {
  if (!coreData) { // first time
    /*-- init variables --*/
    memset(&coreGlobals, 0, sizeof(coreGlobals));
    memset(&locals, 0, sizeof(locals));
    memset(&locals.lastSeg, -1, sizeof(locals.lastSeg));
    coreData = (struct pinMachine *)&Machine->drv->pinmame;
    //-- initialise timers --
    if (coreData->timers[0].callback) {
      int ii;
      for (ii = 0; ii < 5; ii++) {
        if (coreData->timers[ii].callback) {
          locals.timers[ii] = timer_alloc(coreData->timers[ii].callback);
          timer_adjust(locals.timers[ii], TIME_IN_HZ(coreData->timers[ii].rate), 0, TIME_IN_HZ(coreData->timers[ii].rate));
        }
      }
    }
    /*-- init switch matrix --*/
    memcpy(&coreGlobals.invSw, core_gameData->wpc.invSw, sizeof(core_gameData->wpc.invSw));
    memcpy(coreGlobals.swMatrix, coreGlobals.invSw, sizeof(coreGlobals.invSw));

    /*-- command line options --*/
    locals.displaySize = pmoptions.dmd_compact ? 1 : 2;
    // Skip core_initDisplaySize if using CORE_VIDEO flag.. but this code must also run if NO layout defined
    if( !(core_gameData->lcdLayout) ||
  	   (core_gameData->lcdLayout && core_gameData->lcdLayout->type != CORE_VIDEO)
  	){
      UINT32 size = core_initDisplaySize(core_gameData->lcdLayout) >> 16;
  	  if ((size > CORE_SCREENX) && (locals.displaySize > 1)) {
  		/* force small display */
  		locals.displaySize = 1;
  		core_initDisplaySize(core_gameData->lcdLayout);
  	  }
    }
    /*-- Sound enabled ? */
    if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate) {
      coreGlobals.soundEn = TRUE;
      /*-- init sound commander --*/
      snd_cmd_init(coreData->sndCmd, coreData->sndHead);
    }
    else
      snd_cmd_init(NULL, NULL);

    /*-- init simulator --*/
    if (g_fHandleKeyboard && core_gameData->simData) {
      int inports[CORE_MAXPORTS];
      int ii;

      for (ii = 0; ii < CORE_COREINPORT+(coreData->coreDips+31)/16; ii++)
        inports[ii] = readinputport(ii);

      coreGlobals.simAvail = sim_init((sim_tSimData *)core_gameData->simData,
                                         inports,CORE_COREINPORT+(coreData->coreDips+31)/16);
    }
    /*-- finally init the core --*/
    if (coreData->init) coreData->init();
  }
  /*-- now reset everything --*/
  if (coreData->reset) coreData->reset();

  OnStateChange(1); /* We have a lift-off */

/* TOM: this causes to draw the static sim text */
  schedule_full_refresh();
}

static MACHINE_STOP(core) {
  int ii;
  if (coreData->stop) coreData->stop();
  snd_cmd_exit();
  for (ii = 0; ii < 5; ii++) {
    if (locals.timers[ii])
      timer_remove(locals.timers[ii]);
  }
  memset(locals.timers, 0, sizeof(locals.timers));
  coreData = NULL;
}

void machine_add_timer(struct InternalMachineDriver *machine, void (*func)(int), int rate) {
  int ii;
  for (ii = 0; machine->pinmame.timers[ii].callback; ii++)
    ;
  machine->pinmame.timers[ii].callback = func;
  machine->pinmame.timers[ii].rate = rate;
}

static UINT32 core_initDisplaySize(const struct core_dispLayout *layout) {
  int maxX = 0, maxY = 0;

  locals.segData = &segData[locals.displaySize == 1][0];
  if (layout) {
    while (layout->length) {
      int tmp;
      if (layout->type >= CORE_DMD) tmp = (layout->left + layout->length) * locals.segData[CORE_DMD].cols + 1;
      else tmp = (layout->left + 2*layout->length) * (locals.segData[layout->type & 0x07].cols + 1) / 2;
      if (tmp > maxX) maxX = tmp;
      if (layout->type >= CORE_DMD) tmp = (layout->top  + layout->start)  * locals.segData[CORE_DMD].rows + 1;
      else tmp = (layout->top + 2) * (locals.segData[0].rows + 1) / 2;
      if (tmp > maxY) maxY = tmp;
      layout += 1;
    }
  }
  else if (locals.displaySize > 1)
#ifndef VPINMAME
    { maxX = 256; maxY = 65; }
#else
    { maxX = 257; maxY = 65; }
#endif /* VPINMAME */
  else
    { maxX = 129; maxY = 33; }
  locals.firstSimRow = maxY + 5;
  if (!pmoptions.dmd_only) {
    maxY += 180;
    if (maxX < 256) maxX = 256;
  }
  if (maxY >= CORE_SCREENY) maxY = CORE_SCREENY-1;
  set_visible_area(0, maxX-1, 0, maxY);
#ifndef VPINMAME
  if (maxX == 257) maxX = 256;
#endif /* VPINMAME */
  return (maxX<<16) | maxY;
}

void core_nvram(void *file, int write, void *mem, int length, UINT8 init) {
  if (write)     osd_fwrite(file, mem, length); /* save */
  else if (file) osd_fread(file,  mem, length); /* load */
  else           memset(mem, init, length);     /* first time */
  mech_nv(file, write); /* save mech positions */
  { /*-- Load/Save DIP settings --*/
    UINT8 dips[6];
    int   ii;

    if (write) {
      for (ii = 0; ii < 6; ii++) dips[ii] = core_getDip(ii);
      osd_fwrite(file, dips, sizeof(dips));
    }
    else if (file) {
      /* set the defaults (for compabilty with older versions) */
      dips[0] = readinputport(CORE_COREINPORT+1) & 0xff;
      dips[1] = readinputport(CORE_COREINPORT+1)>>8;
      dips[2] = readinputport(CORE_COREINPORT+2) & 0xff;
      dips[3] = readinputport(CORE_COREINPORT+2)>>8;
      dips[4] = readinputport(CORE_COREINPORT+3) & 0xff;
      dips[5] = readinputport(CORE_COREINPORT+3)>>8;

      osd_fread(file, dips, sizeof(dips));
      for (ii = 0; ii < 6; ii++) vp_setDIP(ii, dips[ii]);

    }
    else { // always get the default from the inports
      /* coreData not initialised yet. Don't know exact number of DIPs */
      vp_setDIP(0, readinputport(CORE_COREINPORT+1) & 0xff);
      vp_setDIP(1, readinputport(CORE_COREINPORT+1)>>8);
      vp_setDIP(2, readinputport(CORE_COREINPORT+2) & 0xff);
      vp_setDIP(3, readinputport(CORE_COREINPORT+2)>>8);
      vp_setDIP(4, readinputport(CORE_COREINPORT+3) & 0xff);
      vp_setDIP(5, readinputport(CORE_COREINPORT+3)>>8);
    }
  }
}

MACHINE_DRIVER_START(PinMAME)
  MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY)
  MDRV_SCREEN_SIZE(CORE_SCREENX,CORE_SCREENY)
  MDRV_VISIBLE_AREA(0, CORE_SCREENX-1, 0, CORE_SCREENY-1)
  MDRV_PALETTE_INIT(core)
  MDRV_PALETTE_LENGTH(sizeof(core_palette)/sizeof(core_palette[0][0])/3)
  MDRV_FRAMES_PER_SECOND(60)
  MDRV_SWITCH_CONV(core_swSeq2m,core_m2swSeq)
  MDRV_LAMP_CONV(core_swSeq2m,core_m2swSeq)
  MDRV_MACHINE_INIT(core) MDRV_MACHINE_STOP(core)
  MDRV_VIDEO_UPDATE(core_gen)
MACHINE_DRIVER_END
