#ifndef INC_CORE
#define INC_CORE
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <assert.h>

#include "wpcsam.h"
#include "gen.h"
#include "sim.h"

/*-- some convenience macros --*/
#ifndef FALSE
  #define FALSE (0)
#endif
#ifndef TRUE
  #define TRUE (1)
#endif

#ifdef _WIN32
  #define strncasecmp _strnicmp
#endif

#ifdef MAME_DEBUG
  #define DBGLOG(x) logerror x
#else
  #define DBGLOG(x)
#endif

  #define NORMALREGION(size, reg)  ROM_REGION(size, reg, 0)
  #define NORMALREGIONE(size, reg) ROM_REGION(size, reg, ROMREGION_ERASE)
  #define SOUNDREGION(size ,reg)   ROM_REGION(size, reg, ROMREGION_SOUNDONLY)
  #define SOUNDREGIONE(size ,reg)  ROM_REGION(size, reg, ROMREGION_SOUNDONLY|ROMREGION_ERASE)

/*-- convenience macro for handling bits --*/
#define GET_BIT0 (data & 0x01) >> 0
#define GET_BIT1 (data & 0x02) >> 1
#define GET_BIT2 (data & 0x04) >> 2
#define GET_BIT3 (data & 0x08) >> 3
#define GET_BIT4 (data & 0x10) >> 4
#define GET_BIT5 (data & 0x20) >> 5
#define GET_BIT6 (data & 0x40) >> 6
#define GET_BIT7 (data & 0x80) >> 7


/*-- default screen size */
#ifdef VPINMAME
#  define CORE_SCREENX 640
#  define CORE_SCREENY 400
#else /* VPINMAME */
#  define CORE_SCREENX 320
#  define CORE_SCREENY 256
#endif /* VPINMAME */
/*-----------------
/  define the game
/------------------*/
#define CORE_GAMEDEF(name, ver, longname, year, manuf, machine, flag) \
  GAMEX(year,name##_##ver,0,machine,name,name,ROT0,manuf,longname,flag)
#define CORE_GAMEDEFNV(name, longname, year, manuf, machine, flag) \
  GAMEX(year,name,0,machine,name,name,ROT0,manuf,longname,flag)
#define CORE_CLONEDEF(name, ver, clonever, longname, year, manuf, machine,flag) \
  GAMEX(year,name##_##ver,name##_##clonever,machine,name,name,ROT0,manuf,longname,flag)
#define CORE_CLONEDEFNV(name, cl, longname, year, manuf, machine,flag) \
  GAMEX(year,name,cl,machine,name,name,ROT0,manuf,longname,flag)
#define CORE_GAMEDEFNVR90(name, longname, year, manuf, machine, flag) \
  GAMEX(year,name,0,machine,name,name,ROT90,manuf,longname,flag)

/*--------------
/  Input ports
/---------------*/
/* strange but there are no way to define IMP and TOG with key without using BITX */
#define COREPORT_BIT(mask, name, key) \
   PORT_BITX(mask,IP_ACTIVE_HIGH,IPT_BUTTON1,name,key,IP_JOY_NONE)
#define COREPORT_BITIMP(mask, name, key) \
   PORT_BITX(mask,IP_ACTIVE_HIGH,IPT_BUTTON1 | IPF_IMPULSE | (1<<8),name,key,IP_JOY_NONE)
#define COREPORT_BITTOG(mask, name, key) \
   PORT_BITX(mask,IP_ACTIVE_HIGH,IPT_BUTTON1 | IPF_TOGGLE,name,key,IP_JOY_NONE)
#define COREPORT_DIPNAME(mask,default,name) \
   PORT_DIPNAME(mask,default,name)
#define COREPORT_DIPSET(mask,name) \
   PORT_DIPSETTING(mask,name)

/*-- only used in standard inport --*/
#define COREPORT_BITDEF(mask, type, key) \
   PORT_BITX(mask,IP_ACTIVE_HIGH, type, IP_NAME_DEFAULT, key, IP_JOY_DEFAULT)

/*----------------
/  Common inports
/-----------------*/
#define CORE_PORTS \
  PORT_START /* 0 */ \
    COREPORT_BIT(0x0001, "Column 1",  KEYCODE_Q) \
    COREPORT_BIT(0x0002, "Column 2",  KEYCODE_W) \
    COREPORT_BIT(0x0004, "Column 3",  KEYCODE_E) \
    COREPORT_BIT(0x0008, "Column 4",  KEYCODE_R) \
    COREPORT_BIT(0x0010, "Column 5",  KEYCODE_T) \
    COREPORT_BIT(0x0020, "Column 6",  KEYCODE_Y) \
    COREPORT_BIT(0x0040, "Column 7",  KEYCODE_U) \
    COREPORT_BIT(0x0080, "Column 8",  KEYCODE_I) \
    COREPORT_BIT(0x0100, "Row 1",     KEYCODE_A) \
    COREPORT_BIT(0x0200, "Row 2",     KEYCODE_S) \
    COREPORT_BIT(0x0400, "Row 3",     KEYCODE_D) \
    COREPORT_BIT(0x0800, "Row 4",     KEYCODE_F) \
    COREPORT_BIT(0x1000, "Row 5",     KEYCODE_G) \
    COREPORT_BIT(0x2000, "Row 6",     KEYCODE_H) \
    COREPORT_BIT(0x4000, "Row 7",     KEYCODE_J) \
    COREPORT_BIT(0x8000, "Row 8",     KEYCODE_K) \
  PORT_START /* 1 */ \
    COREPORT_BIT(0x0001, "Left Flipper",   KEYCODE_LSHIFT)  \
    COREPORT_BIT(0x0002, "Right Flipper",  KEYCODE_RSHIFT) \
    COREPORT_BIT(0x0004, "U Left Flipper",  KEYCODE_A)  \
    COREPORT_BIT(0x0008, "Y Right Flipper", KEYCODE_L)

/*-----------------------
/ Access to common ports
/------------------------*/
/*-- manual switch keys --*/
#define CORE_MANSWINPORT    0
#define CORE_MANSWCOLUMNS   0x00ff
#define CORE_MANSWROWS      0xff00

/*-- common keys (start, tilt etc) --*/
#define CORE_FLIPINPORT     1
#define CORE_LLFLIPKEY      0x0001
#define CORE_LRFLIPKEY      0x0002
#define CORE_ULFLIPKEY      0x0004
#define CORE_URFLIPKEY      0x0008

#define CORE_SIMINPORT      1  /* Inport for simulator */
#define CORE_COREINPORT     2  /* Inport for core use */

// Macro to ease switch assignment
#define CORE_SETKEYSW(value, mask, swcol) \
  coreGlobals.swMatrix[(swcol)] = (coreGlobals.swMatrix[(swcol)] & ~(mask)) | ((value) & (mask))

/*------------------------------------------------------
/ Flipper hardware is described with the following macros
/  (macros use FLIP_LL, FLIP_LR, FLIP_UL, FLIP_UR)
/ FLIP_SW()      Flipper switches available
/ FLIP_SWNO(L,R) Flipper switch numbers if other than default (Pre-fliptronics)
/ FLIP_SOL()     CPU controlled flippers
/ Example: CPU controlled upper right flipper
/   FLIP_SW(FLIP_LL | FLIP_LR | FLIP_UR) + FLIP_SOL(FLIP_LL | FLIP_LR | FLIP_UR)
/ Example: Flippers not controlled by CPU
/   FLIP_SWNO(swFlipL, swFlipR)
/--------------------------------------------------------------*/
/*-- flipper names --*/
#define FLIP_LR        (0x1)
#define FLIP_LL        (0x2)
#define FLIP_UR        (0x4)
#define FLIP_UL        (0x8)
#define FLIP_L         (FLIP_LL | FLIP_LR)
#define FLIP_U         (FLIP_UL | FLIP_UR)

/*-- definition macros --*/
#define FLIP_BUT(x)    ((x)<<16)
#define FLIP_SW(x)     ((x)<<20)
#define FLIP_SWNO(l,r) (((l)<<8)|(r)|FLIP_SW(FLIP_L))
#define FLIP_EOS(x)    ((x)<<28)
#define FLIP_SOL(x)    (((x)<<24)|FLIP_EOS(x))

#define FLIP_SWL(x)    (((x)>>8)&0xff)
#define FLIP_SWR(x)    ((x)&0xff)

/*---------------------
/  Exported variables
/----------------------*/
#define CORE_FLIPSTROKETIME 2 /* Timer for flipper to reach top VBLANKs */

/*-----------------------------
/  Generic Display layout data
/------------------------------*/
/* The different kind of display units */
#define CORE_SEG16    0 // 16 segments
#define CORE_SEG16R   1 // 16 segments with comma and period reversed
#define CORE_SEG10    2 // 9  segments and comma
#define CORE_SEG9     3 // 9  segments
#define CORE_SEG8     4 // 7  segments and comma
#define CORE_SEG8D    5 // 7  segments and period
#define CORE_SEG7     6 // 7  segments
#define CORE_SEG87    7 // 7  segments, comma every three
#define CORE_SEG87F   8 // 7  segments, forced comma every three
#define CORE_SEG98    9 // 9  segments, comma every three
#define CORE_SEG98F  10 // 9  segments, forced comma every three
#define CORE_SEG7S   11 // 7  segments, small
#define CORE_SEG7SC  12 // 7  segments, small, with comma
#define CORE_SEG16S  13 // 16 segments with split top and bottom line
#define CORE_DMD     14 // DMD Display
#define CORE_VIDEO   15 // VIDEO Display
#define CORE_SEG16N  16 // 16 segments without commas
#define CORE_SEG16D  17 // 16 segments with periods only

#define CORE_SEGALL   0x1f // maximum segment definition number
#define CORE_IMPORT   0x20 // Link to another display layout
#define CORE_SEGMASK  0x3f // Note that CORE_IMPORT must be part of the segmask as well!

#define CORE_SEGHIBIT 0x40
#define CORE_SEGREV   0x80
#define CORE_DMDNOAA  0x100
#define CORE_NODISP   0x200
#define CORE_DMDSEG   0x400

#define CORE_SEG8H    (CORE_SEG8  | CORE_SEGHIBIT)
#define CORE_SEG7H    (CORE_SEG7  | CORE_SEGHIBIT)
#define CORE_SEG87H   (CORE_SEG87 | CORE_SEGHIBIT)
#define CORE_SEG87FH  (CORE_SEG87F| CORE_SEGHIBIT)
#define CORE_SEG7SH   (CORE_SEG7S | CORE_SEGHIBIT)
#define CORE_SEG7SCH  (CORE_SEG7SC| CORE_SEGHIBIT)

#define DMD_MAXX 256
#define DMD_MAXY 64

/* Shortcuts for some common display sizes */
#define DISP_SEG_16(row,type)    {4*(row), 0, 20*(row), 16, type}
#define DISP_SEG_7(row,col,type) {4*(row),16*(col),(row)*20+(col)*8+1,7,type}
#define DISP_SEG_CREDIT(no1,no2,type) {2,2,no1,1,type},{2,4,no2,1,type}
#define DISP_SEG_BALLS(no1,no2,type)  {2,8,no1,1,type},{2,10,no2,1,type}
#define DISP_SEG_IMPORT(x) {0,0,0,1,CORE_IMPORT,NULL,x}
/* display layout structure */
/* Don't know how the LCD got in there. Should have been LED but now it
   handles all kinds of displays so we call it dispLayout.
   Keep the typedef of core_tLCDLayout for some time. */
struct core_dispLayout {
  UINT16 top, left, start, length, type;
  genf *fptr;
  const struct core_dispLayout *lptr;	// for DISP_SEG_IMPORT(x) with flag CORE_IMPORT
};
typedef struct core_dispLayout core_tLCDLayout, *core_ptLCDLayout;
// Overall alphanumeric display layout. Used externally by VPinMame's dmddevice. Don't change order
typedef enum {
   CORE_SEGLAYOUT_None,
   CORE_SEGLAYOUT_2x16Alpha,
   CORE_SEGLAYOUT_2x20Alpha,
   CORE_SEGLAYOUT_2x7Alpha_2x7Num,
   CORE_SEGLAYOUT_2x7Alpha_2x7Num_4x1Num,
   CORE_SEGLAYOUT_2x7Num_2x7Num_4x1Num,
   CORE_SEGLAYOUT_2x7Num_2x7Num_10x1Num,
   CORE_SEGLAYOUT_2x7Num_2x7Num_4x1Num_gen7,
   CORE_SEGLAYOUT_2x7Num10_2x7Num10_4x1Num,
   CORE_SEGLAYOUT_2x6Num_2x6Num_4x1Num,
   CORE_SEGLAYOUT_2x6Num10_2x6Num10_4x1Num,
   CORE_SEGLAYOUT_4x7Num10,
   CORE_SEGLAYOUT_6x4Num_4x1Num,
   CORE_SEGLAYOUT_2x7Num_4x1Num_1x16Alpha,
   CORE_SEGLAYOUT_1x16Alpha_1x16Num_1x7Num,
   CORE_SEGLAYOUT_1x7Num_1x16Alpha_1x16Num,
   CORE_SEGLAYOUT_1x16Alpha_1x16Num_1x7Num_1x4Num,
   CORE_SEGLAYOUT_Invalid
} core_segOverallLayout_t;


#define PINMAME_VIDEO_UPDATE(name) int (name)(struct mame_bitmap *bitmap, const struct rectangle *cliprect, const struct core_dispLayout *layout)
typedef int (*ptPinMAMEvidUpdate)(struct mame_bitmap *bitmap, const struct rectangle *cliprect, const struct core_dispLayout *layout);

/*---------------------------------------
/ WPC driver constants
/----------------------------------------*/
/*      Solenoid numbering               */
/*                                       */
/*       WPC                             */
/*  1-28 Standard                        */
/* 29-31 GameOn (3 highest bit of GI reg)*/
/* 32    Unused (always 0)               */
/* 33-36 Upper flipper solenoids         */
/* 37-40 LPDC WPC95 controlled outputs   */
/* 41-44 Copy of 37-40 for backward comp.*/
/* 45-48 Lower flipper solenoids         */
/* 49    Fake solenoid for ball shooter  */
/* 50    Reserved                        */
/* 51-58 8 drivers Aux Board or custom   */
/* 59-64 custom                          */
/*                                       */
/*       S9/S11                          */
/*  1- 8 Standard 'A'-side               */
/*  9-16 Standard                        */
/* 17-22 Special                         */
/* 23    Flipper & Switched Sol. Enable  */
/* 25-32 Standard 'C'-side               */
/* 37-41 Sound overlay board             */
/*                                       */
/*       S7                              */
/*  1-16 Standard                        */
/* 17-24 Special                         */
/* 25    Flipper & SS Enabled Sol (fake) */
/*                                       */
/*       BY17/BY35                       */
/*  1-15 Standard Pulse                  */
/* 17-20 Standard Hold                   */
/*                                       */
/*       GTS80                           */
/*  1- 9 Standard                        */
/* 10    GameOn (fake)                   */
/* 11    Tilt (for GI) (fake)            */
#define CORE_FIRSTEXTSOL   37
#define CORE_FIRSTUFLIPSOL 33
#define CORE_FIRSTCUSTSOL  51
#define CORE_FIRSTLFLIPSOL 45
#define CORE_FIRSTSIMSOL   49
#define CORE_MAXSOL        64

#define CORE_SSFLIPENSOL  23
#define CORE_FIRSTSSSOL   17

#define CORE_SOLBIT(x) (1<<((x)-1))

/*  Flipper Solenoid numbers */
#define sLRFlip     (CORE_FIRSTLFLIPSOL+1)
#define sLRFlipPow  (CORE_FIRSTLFLIPSOL+0)
#define sLLFlip     (CORE_FIRSTLFLIPSOL+3)
#define sLLFlipPow  (CORE_FIRSTLFLIPSOL+2)
#define sURFlip     (CORE_FIRSTUFLIPSOL+1)
#define sURFlipPow  (CORE_FIRSTUFLIPSOL+0)
#define sULFlip     (CORE_FIRSTUFLIPSOL+3)
#define sULFlipPow  (CORE_FIRSTUFLIPSOL+2)

/*-- Flipper solenoid bits --*/
#define CORE_LRFLIPSOLBITS 0x03
#define CORE_LLFLIPSOLBITS 0x0c
#define CORE_URFLIPSOLBITS 0x30
#define CORE_ULFLIPSOLBITS 0xc0

/*-- create a custom solenoid number --*/
/* example: #define swCustom CORE_CUSTSOLNO(1)  // custom solenoid 1 */
#define CORE_CUSTSOLNO(n) (CORE_FIRSTCUSTSOL-1+(n))

#define CORE_COINDOORSWCOL                0 /* internal array number */
#define CORE_MAXSWCOL                    16 /* switch columns (0-9=sw matrix, 10=coin door, 11=cabinet/flippers) */
#define CORE_FLIPPERSWCOL                11 /* internal array number */
#define CORE_STDSWCOLS                   12 /* Base number of switch matrix columns */
#define CORE_CUSTSWCOL       CORE_STDSWCOLS /* first custom (game specific) switch column */
#define CORE_MAXLAMPCOL                  78 /* lamp column (0-7=std lamp matrix 8- custom) */
#define CORE_STDLAMPCOLS                  8 /* Base number of lamp matrix columns */
#define CORE_CUSTLAMPCOL   CORE_STDLAMPCOLS /* first custom lamp column */
#define CORE_MAXPORTS                     8 /* Maximum input ports */
#define CORE_MAXGI                        5 /* Maximum GI strings */
#define CORE_MAXNVRAM                131118 /* Maximum number of NVRAM bytes, only used for get_ChangedNVRAM so far */
#define CORE_SEGCOUNT                   128 /* Maximum number of alpha numeric display (16 segments each) */

/*-- create a custom switch number --*/
/* example: #define swCustom CORE_CUSTSWNO(1,2)  // custom column 1 row 2 */
#define CORE_CUSTSWNO(c,r) ((CORE_CUSTSWCOL-1+(c))*10+(r))

/*-------------------
/  Flipper Switches
/ in column FLIPPERSWCOL
/--------------------*/
#define CORE_SWLRFLIPEOSBIT 0x01
#define CORE_SWLRFLIPBUTBIT 0x02
#define CORE_SWLLFLIPEOSBIT 0x04
#define CORE_SWLLFLIPBUTBIT 0x08
#define CORE_SWURFLIPEOSBIT 0x10
#define CORE_SWURFLIPBUTBIT 0x20
#define CORE_SWULFLIPEOSBIT 0x40
#define CORE_SWULFLIPBUTBIT 0x80

#define CORE_FIRSTSIMROW   80 /* first free row on display */

/*-- Colours --*/
#define COL_DMD        1
#define COL_DMDOFF     (COL_DMD+0)
#define COL_DMD33      (COL_DMD+1)
#define COL_DMD66      (COL_DMD+2)
#define COL_DMDON      (COL_DMD+3)
#define COL_DMDCOUNT   4
#define COL_LAMP       (COL_DMD+COL_DMDCOUNT)
#define COL_LAMPCOUNT  8
#define COL_SHADE(x)   (COL_LAMPCOUNT+(x))
#define COL_DMDAA      (COL_LAMP+COL_LAMPCOUNT*2)
#define COL_DMDAACOUNT 7
#define COL_SEGAAON1   (COL_DMDAA+COL_DMDAACOUNT)
#define COL_SEGAAON2   (COL_SEGAAON1+1)
#define COL_SEGAAOFF1  (COL_SEGAAON1+2)
#define COL_SEGAAOFF2  (COL_SEGAAON1+3)
#define COL_SEGAACOUNT 4
#define COL_COUNT      (COL_SEGAAON1+COL_SEGAACOUNT)

/* Lamp Colors */
#define BLACK       (COL_LAMP+0)
#define WHITE       (COL_LAMP+1)
#define GREEN       (COL_LAMP+2)
#define RED         (COL_LAMP+3)
#define ORANGE      (COL_LAMP+4)
#define YELLOW      (COL_LAMP+5)
#define LBLUE       (COL_LAMP+6)
#define LPURPLE     (COL_LAMP+7)

/*-- Physical devices on binary outputs --*/

#define CORE_MODOUT_ENABLE_MODSOL            0x01 /* Bitmask for options.usemodsol to enable legacy behavior (simple solenoid linear integration for WPC/SAM)  */
#define CORE_MODOUT_ENABLE_PHYSOUT_LAMPS     0x04
#define CORE_MODOUT_ENABLE_PHYSOUT_GI        0x08
#define CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS 0x10
#define CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS 0x20
#define CORE_MODOUT_FORCE_ON                 0x80 /* Bitmask for options.usemodsol for drivers that needs PWM integration to be performed whatever the user settings are. Note that this is internal to the driver. The exchanged value depends only on the user settings. */
#define CORE_MODOUT_ENABLE_PHYSOUT_ALL (CORE_MODOUT_ENABLE_PHYSOUT_LAMPS|CORE_MODOUT_ENABLE_PHYSOUT_GI|CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS|CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS) /* Bitmask for options.usemodsol to enable physics output for solenoids/Lamp/GI/AlphaSegments */

#define CORE_MODOUT_LAMP_MAX                 (CORE_MAXLAMPCOL * 8) /* Maximum number of modulated outputs for lamps */
#define CORE_MODOUT_SOL_MAX                                     72 /* Maximum number of modulated outputs for solenoids */
#define CORE_MODOUT_GI_MAX                                       8 /* Maximum number of modulated outputs for GI */
#define CORE_MODOUT_SEG_MAX                   (CORE_SEGCOUNT * 16) /* Maximum number of modulated outputs for alphanumeric segments */
#define CORE_MODOUT_LAMP0                                        0 /* Index of first lamp output */
#define CORE_MODOUT_SOL0                      CORE_MODOUT_LAMP_MAX /* Index of first solenoid output */
#define CORE_MODOUT_GI0   (CORE_MODOUT_SOL0 + CORE_MODOUT_SOL_MAX) /* Index of first GI output */
#define CORE_MODOUT_SEG0  (CORE_MODOUT_GI0  + CORE_MODOUT_GI_MAX ) /* Index of first alphanumeric segment output */
#define CORE_MODOUT_MAX   (CORE_MODOUT_SEG0 + CORE_MODOUT_SEG_MAX) /* Maximum number of modulated outputs */

#define CORE_MODOUT_NONE                   0 /* just don't do anything: value defined by driver is kept unchanged by integrator */
#define CORE_MODOUT_PULSE                  1 /* No integration, just the raw pulse state */
#define CORE_MODOUT_SOL_2_STATE            2 /* Simple 2 state solenoid implementation: ON as soon as it is pulsed, OFF if no high state has been seen for the last 60ms */
#define CORE_MODOUT_SOL_CUSTOM             3 /* Call drivers's custom solenoid 'getSol' implementation */
#define CORE_MODOUT_CUSTOM_INTEGRATOR      4 /* Driver's custom integrator definition */
#define CORE_MODOUT_LEGACY_SOL_2_STATE    50 /* Simple 2 state solenoid implementation: ON as soon as it is pulsed, OFF if no high state has been seen for the last 60ms, waits for a few 'VBlank' before being reported */
#define CORE_MODOUT_BULB_44_6_3V_AC      100 /* Incandescent #44/555 Bulb connected to 6.3V, commonly used for GI */
#define CORE_MODOUT_BULB_47_6_3V_AC      101 /* Incandescent #47 Bulb connected to 6.3V, commonly used for (darker) GI with less heat */
#define CORE_MODOUT_BULB_86_6_3V_AC      102 /* Incandescent #86 Bulb connected to 6.3V, seldom use: TZ, CFTBL,... */
#define CORE_MODOUT_BULB_44_5_7V_AC      103 /* Incandescent #44/555 Bulb connected to 5.7V, used for GI on Stern/Sega Whitestar as well as Stern SAM hardware (slower fading up and 78% dimmer light emission) */
#define CORE_MODOUT_BULB_44_6_3V_AC_REV  104 /* Incandescent #44/555 Bulb connected to 6.3V, commonly used for GI, through a relay inverter: 1 is off, 0 is on */
#define CORE_MODOUT_BULB_44_18V_DC_WPC   201 /* Incandescent #44/555 Bulb connected to 18V, commonly used for lamp matrix with short strobing */
#define CORE_MODOUT_BULB_44_20V_DC_GTS3  202 /* Incandescent #44/555 Bulb connected to 20V, commonly used for lamp matrix with short strobing */
#define CORE_MODOUT_BULB_44_18V_DC_S11   203 /* Incandescent #44/555 Bulb connected to 18V, commonly used for lamp matrix with short strobing */
#define CORE_MODOUT_BULB_44_18V_DC_SE    204 /* Incandescent #44/555 Bulb connected to 18V, commonly used for lamp matrix with short strobing */
#define CORE_MODOUT_BULB_44_20V_DC_CC    205 /* Incandescent #44/555 Bulb connected to 18V, commonly used for lamp matrix with short strobing */
#define CORE_MODOUT_BULB_89_20V_DC_WPC   301 /* Incandescent #89 Bulb connected to 20V, commonly used for flashers */
#define CORE_MODOUT_BULB_89_20V_DC_GTS3  302 /* Incandescent #89 Bulb connected to 20V, commonly used for flashers */
#define CORE_MODOUT_BULB_89_32V_DC_S11   303 /* Incandescent #89 Bulb connected to 32V, used for flashers on S11 with output strobing */
#define CORE_MODOUT_BULB_89_25V_DC_S11   304 /* Incandescent #89 Bulb connected to 25V, used for flashers on S11 with output strobing */
#define CORE_MODOUT_BULB_906_20V_DC_WPC  311 /* Incandescent #906 Bulb connected to 20V, commonly used for flashers */
#define CORE_MODOUT_BULB_906_20V_DC_GTS3 312 /* Incandescent #906 Bulb connected to 20V, commonly used for flashers */
#define CORE_MODOUT_BULB_906_32V_DC_S11  313 /* Incandescent #906 Bulb connected to 32V, used for flashers on S11 with output strobing */
#define CORE_MODOUT_BULB_906_25V_DC_S11  314 /* Incandescent #906 Bulb connected to 25V, used for flashers on S11 with output strobing */
#define CORE_MODOUT_LED                  400 /* LED PWM (in fact mostly human eye reaction, since LED are nearly instantaneous) */
#define CORE_MODOUT_LED_STROBE_1_10MS    401 /* LED Strobed 1ms over 10ms for full power */
#define CORE_MODOUT_LED_STROBE_1_5MS     402 /* LED Strobed 1ms over 5ms for full power */
#define CORE_MODOUT_LED_STROBE_8_16MS    403 /* LED Strobed 8ms over 16ms for full power */
#define CORE_MODOUT_VFD_STROBE_05_20MS   450 /* Vacuum Fluorescent Display used for alpha numeric segment displays */
#define CORE_MODOUT_VFD_STROBE_1_16MS    451 /* Vacuum Fluorescent Display used for alpha numeric segment displays */

/*-------------------------------------------
/  Draw data. draw lamps,switches,solenoids
/  in this way instead of a matrix
/--------------------------------------------*/
typedef struct {
  UINT8 x,y, color;
} core_tDrawData;

typedef struct {
  UINT8 totnum;	 	 /*Total # of lamp positions defined - Up to 4 Max*/
  core_tDrawData lamppos[4];      /*Can support up to 4 lamp positions for each lamp matrix entry!*/
} core_tLampData;		 /*This means, one lamp matrix entry can share up to 4 bulbs on the playfield*/

typedef struct {
  core_tDrawData startpos;	/*Starting Coordinates to draw matrix*/
  core_tDrawData size;		/*Size of lamp matrix*/
  core_tLampData lamps[CORE_MAXLAMPCOL*8];      /*Can support up to 160 lamps!*/
} core_tLampDisplay;

typedef void (*core_tPhysOutputIntegrator)(const double, const int, const int, const int);

#define FLIP_BUFFER_SIZE 32               /* Number of state considered for PWM integration. Must be even (and should a power of 2 for performance reason) */
typedef struct {
   int type;                              /* Type of modulation from CORE_MODOUT_ definitions */
   float value;                           /* Last computed output main physical characteristic (relative brightness for bulbs, strength for solenoids,...) */
   core_tPhysOutputIntegrator integrator; /* Integrator function used to update the output state */
   union { /* Last PWM integration result (not taking in account elapsedInStateTime periods) */
      struct
      {
         int bulb;                        /* bulb model */
         int isAC;                        /* AC or DC ? */ // bool
         int isReversed;                  /* is input reversed ? */ // bool
         float U;                         /* voltage (Volts) */
         float serial_R;                  /* serial resistor (Ohms) */
         float relative_brightness;       /* Relative brightness scale */
         double integrationTimestamp;     /* last integration timestamp */
         double prevIntegrationTimestamp; /* prev integration timestamp */
         float prevIntegrationValue;      /* prev integration value */
         float filament_temperature;      /* actual filament temperature */
         float eye_integration[3];        /* flicker/fusion eye model state */
         float eye_emission_old;          /* prev emission value in the eye model state */
      } bulb; // Physical model of a bulb / LED / VFD
      struct
      {
         int fastOn;
         double lastFlipTimestamp;
         float switchDownLatency;
      } sol; // Physical model of a solenoid
   } state;
   double flipTimeStamps[FLIP_BUFFER_SIZE];
   unsigned int flipBufferPos;
   unsigned int lastIntegrationFlipPos;
} core_tPhysicOutput;

#ifdef LSB_FIRST
typedef union { struct { UINT8 lo, hi; } b; UINT16 w; } core_tWord;
typedef core_tWord core_tSeg[CORE_SEGCOUNT];
#else /* LSB_FIRST */
typedef union { struct { UINT8 hi, lo; } b; UINT16 w; } core_tWord;
typedef core_tWord core_tSeg[CORE_SEGCOUNT];
#endif /* LSB_FIRST */
typedef struct {
  /*-- Switches --*/
  volatile UINT8  swMatrix[CORE_MAXSWCOL];
  volatile UINT8  invSw[CORE_MAXSWCOL];                         /* Active low switches */
  /*-- Strobed lamp matrix --*/
  volatile UINT8  lampMatrix[CORE_MAXLAMPCOL];                  /* Strobed lamp matrix binary state as integrated during last strobe to On/Off state */
  volatile UINT8  tmpLampMatrix[CORE_MAXLAMPCOL];               /* Strobed lamp matrix binary state being currently integrated (intermediate state until next end of strobe) */
  /*-- Alphanumeric driver --*/
  core_tSeg segments;                                           /* Segments data from driver */
  UINT16 drawSeg[CORE_SEGCOUNT];                                /* Segments drawn */
  /*-- DMD --*/
  UINT8 dmdDotRaw[DMD_MAXY * DMD_MAXX];                         /* DMD: 'raw' dots, that is to say frame built up from rasterized frames (depends on each driver), stable result that can be used for post processing (colorization, ...) */
  UINT8 dmdDotLum[DMD_MAXY * DMD_MAXX];                         /* DMD: perceived linear luminance computed from PWM frames, for rendering (result may change and can't be considered as stable) */
  /*-- Solenoids --*/
  volatile UINT32 pulsedSolState;                               /* Current pulse binary value of solenoids on driver board */
  volatile UINT32 solenoids;                                    /* Current integrated binary On/Off value of solenoids on driver board (not pulsed, averaged over a period depending on the driver) */
  volatile UINT32 solenoids2;                                   /* Current integrated binary On/Off value of additional solenoids, including flipper solenoids (not pulsed, averaged over a period depending on the driver) */
  UINT64 flipperCoils;                                          /* Coil mapping of flipper power/hold coils => TODO move to core_gameData */
  /*-- GI --*/
  volatile int   gi[CORE_MAXGI];                                /* WPC, Whitestar and SAM GI strings state */
  /*-- Generalized outputs --*/
  int nSolenoids, nLamps, nGI, nAlphaSegs;                      /* Number of physical outputs the driver handles */
  double lastACZeroCrossTimeStamp;                              /* Last time AC did cross 0 as reported by the driver (should be 120Hz) */
  UINT8 binaryOutputState[CORE_MODOUT_MAX / 8];                 /* Pulsed binary state */
  core_tPhysicOutput physicOutputState[CORE_MODOUT_MAX];        /* Output state, taking in account the physical device wired to the binary output */
  float lastPhysicOutputReportedValue[CORE_MODOUT_MAX];         /* Last state value reported for each of the physic outputs */
  /*-- Miscellaneous --*/
  int    simAvail;                                              /* Simulator (keys) available */
  int    soundEn;                                               /* Sound enabled ? */
  volatile int    diagnosticLed;                                /* Data relating to diagnostic led(s)*/
#ifdef PROC_SUPPORT
  int    p_rocEn;         /* P-ROC support enable */
  int    isKickbackLamp[255];
#endif
} core_tGlobals;
extern core_tGlobals coreGlobals;
/* shortcut for coreGlobals */
#define cg coreGlobals
extern volatile struct pinMachine *coreData;
/*Exported variables*/
/*-- There are no custom fields in the game driver --*/
/*-- so I have to invent some by myself. Each driver --*/
/*-- fills in one of these in the game_init function --*/
typedef struct {
  UINT64  gen;                /* Hardware Generation */
  const struct core_dispLayout *lcdLayout; /* LCD display layout */
  struct {
    UINT32  flippers;      /* flippers installed (see defines below) */
    int     swCol, lampCol, custSol; /* Custom switch columns, lamp columns and solenoids */
    UINT32  soundBoard, display;
    UINT32  gameSpecific1, gameSpecific2;
    /*-- custom functions --*/
    int  (*getSol)(int solNo);        /* get state of custom solenoid */
    void (*handleMech)(int mech);     /* update switches depending on playfield mechanics */
    int  (*getMech)(int mechNo);      /* get status of mechanics */
    void (*drawMech)(BMTYPE **line);  /* draw game specific hardware */
    core_tLampDisplay *lampData;      /* lamp layout */
#ifdef ENABLE_MECHANICAL_SAMPLES
    wpc_tSamSolMap   *solsammap;      /* solenoids samples */
#endif
  } hw;
  const void *simData;
  struct { /* WPC specific stuff */
    char serialNo[21];  /* Security chip serial number */
    UINT8 invSw[CORE_MAXSWCOL]; /* inverted switches (e.g. optos) */
    /* common switches */
    struct { int start, tilt, sTilt, coinDoor, shooter; } comSw;
  } wpc;
  struct { /* S3-S11 specific stuff (incl DE) */
    int muxSol;  /* S11 Mux solenoid */
    int ssSw[8]; /* Special solenoid switches */
  } sxx;
  /* simulator data */
} core_tGameData;
extern const core_tGameData *core_gameData;

extern const int core_bcd2seg9[];  /* BCD to 9 segment display */
extern const int core_bcd2seg9a[]; /* BCD to 9 segment display, missing 6 top line */
extern const int core_bcd2seg7[];  /* BCD to 7 segment display */
extern const int core_bcd2seg7a[]; /* BCD to 7 segment display, missing 6 top line */
extern const int core_bcd2seg7e[]; /* BCD to 7 segment display with A to E letters */
extern const UINT16 core_ascii2seg16[];  /* BCD to regular 16 segment display */
extern const UINT16 core_ascii2seg16s[]; /* BCD to 16 segment display with split top / botom lines */
#define core_bcd2seg  core_bcd2seg7

/*-- Exported Display handling functions--*/
void core_updateSw(int flipEn);

/*-- text output functions --*/
void core_textOut(const char *buf, int length, int x, int y, int color);
void CLIB_DECL core_textOutf(int x, int y, int color, const char *text, ...);

/*-- lamp handling --*/
void core_setLamp(volatile UINT8 *lampMatrix, int col, int row);
void core_setLampBlank(volatile UINT8 *lampMatrix, int col, int row);

/*-- switch handling --*/
extern void core_setSw(int swNo, int value);
extern int core_getSw(int swNo);
extern void core_updInvSw(int swNo, int inv);

/*-- get a switch column. (colEn=bits) --*/
extern int core_getSwCol(int colEn);

/*-- solenoid handling --*/
extern int core_getSol(int solNo);
extern int core_getPulsedSol(int solNo);
extern UINT64 core_getAllSol(void);
extern void core_getAllPhysicSols(float* const state);

/*-- AC sync and PWM integration --*/
extern void core_update_pwm_outputs(const int startIndex, const int count);
INLINE void core_update_pwm_gis(void) { if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_GI)) core_update_pwm_outputs(CORE_MODOUT_GI0, coreGlobals.nGI); }
INLINE void core_update_pwm_solenoids(void) { if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)) core_update_pwm_outputs(CORE_MODOUT_SOL0, coreGlobals.nSolenoids); }
INLINE void core_update_pwm_segments(void) { if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS)) core_update_pwm_outputs(CORE_MODOUT_SEG0, coreGlobals.nAlphaSegs); }
INLINE void core_update_pwm_lamps(void) { if (options.usemodsol & (CORE_MODOUT_FORCE_ON | CORE_MODOUT_ENABLE_PHYSOUT_LAMPS)) core_update_pwm_outputs(CORE_MODOUT_LAMP0, coreGlobals.nLamps); }
extern void core_set_pwm_output_type(int startIndex, int count, int type);
extern void core_set_pwm_output_types(int startIndex, int count, int* outputTypes);
extern void core_set_pwm_output_bulb(int startIndex, int count, int bulb, float U, int isAC, float serial_R, float relative_brightness);
extern void core_write_pwm_output(int startIndex, int count, UINT8 bitStates); // Write binary state of count outputs, taking care of PWM integration based on physical model of connected device
extern void core_write_pwm_output_8b(int startIndex, UINT8 bitStates);
extern void core_write_masked_pwm_output_8b(int startIndex, UINT8 bitStates, UINT8 bitMask);
extern void core_write_pwm_output_lamp_matrix(int startIndex, UINT8 columns, UINT8 rows, int nCols);
INLINE void core_zero_cross(void) { coreGlobals.lastACZeroCrossTimeStamp = timer_get_time(); }

/*-- DMD PWM integration --*/
typedef struct {
  // Definition initialized at startup using 'core_dmd_pwm_init' then unmutable
  int     width;              // DMD width
  int     height;             // DMD height
  int     revByte;            // Is bitset reversed ?
  int     frameSize;          // Size of a DMD frame in bytes (width * height)
  int     rawFrameSize;       // Size of a raw DMD frame in bytes (width * height / 8)
  int     nFrames;            // Number of frames to store and consider to create shades (depends on hardware refresh frequency and used PWM patterns)
  int     raw_combiner;       // CORE_DMD_PWM_COMBINER_... enum that defines how to combine bitplanes to create multi plane raw frame for colorization plugin
  int     fir_size;           // Selected filter (depends on hardware refresh frequency and number of stored frames)
  const UINT32* fir_weights;  // Selected filter (depends on hardware refresh frequency and number of stored frames)
  UINT32  fir_sum;            // Sum of filter weights
  // Data acquisition, feeded by the driver through 'core_dmd_submit_frame'
  UINT8*  rawFrames;          // Buffer for incoming raw frames
  int     nextFrame;          // Position in circular buffer to store next raw frame
  UINT32* shadedFrame;        // Shaded frame computed from raw frames
  unsigned int frame_index;   // Raw frame index
  // Integrated data, computed by 'core_dmd_update_pwm'
  UINT8*  bitplaneFrame;      // DMD: bitplane frame built up from raw rasterized frames (depends on each driver, stable result that can be used for post processing like colorization, ...)
  UINT8*  luminanceFrame;     // DMD: linear luminance computed from PWM frames, for rendering (result may change and can't be considered as stable accross PinMame builds)
} core_tDMDPWMState;

#define CORE_DMD_PWM_FILTER_DE_128x16   0
#define CORE_DMD_PWM_FILTER_DE_128x32   1
#define CORE_DMD_PWM_FILTER_DE_192x64   2
#define CORE_DMD_PWM_FILTER_GTS3        3
#define CORE_DMD_PWM_FILTER_WPC         4
#define CORE_DMD_PWM_FILTER_WPC_PH      5
#define CORE_DMD_PWM_FILTER_ALVG1       6
#define CORE_DMD_PWM_FILTER_ALVG2       7

#define CORE_DMD_PWM_COMBINER_GTS3_4C_A 0
#define CORE_DMD_PWM_COMBINER_GTS3_4C_B 1
#define CORE_DMD_PWM_COMBINER_GTS3_5C   2
#define CORE_DMD_PWM_COMBINER_SUM_2     3
#define CORE_DMD_PWM_COMBINER_SUM_3     4
#define CORE_DMD_PWM_COMBINER_SUM_2_1   5
#define CORE_DMD_PWM_COMBINER_SUM_1_2   6
#define CORE_DMD_PWM_COMBINER_SUM_4     7

extern void core_dmd_pwm_init(core_tDMDPWMState* dmd_state, const int width, const int height, const int filter, const int raw_combiner);
extern void core_dmd_pwm_exit(core_tDMDPWMState* dmd_state);
extern void core_dmd_submit_frame(core_tDMDPWMState* dmd_state, const UINT8* frame, const int ntimes);
extern void core_dmd_update_pwm(core_tDMDPWMState* dmd_state);
extern void core_dmd_video_update(struct mame_bitmap *bitmap, const struct rectangle *cliprect, const struct core_dispLayout *layout, core_tDMDPWMState* dmd_state);

extern void core_sound_throttle_adj(int sIn, int *sOut, int buffersize, double samplerate);

/*-- nvram handling --*/
extern void core_nvram(void *file, int write, void *mem, size_t length, UINT8 init);

/* makes it easier to swap bits */
#if defined(_M_ARM64) || defined(__aarch64__)
#define core_revnyb __brevnyb
#define core_revbyte __brevc
#else
extern const UINT8 core_swapNyb[16];
INLINE UINT8 core_revnyb(UINT8 x)  { return core_swapNyb[x]; }
INLINE UINT8 core_revbyte(UINT8 x) { return (core_swapNyb[x & 0xf] << 4) | (core_swapNyb[x >> 4]); }
#endif
#define core_revword __brevs

/*-- core DIP handling --*/
//  Get the status of a DIP bank (8 dips)
extern int core_getDip(int dipBank);

/*-- Easy Bit Column to Number conversion
 *   Convert Bit Column Data to corresponding #, ie, if Bit 3=1, return 3 - Zero Based (Bit1=1 returns 0)
 *   Assumes only 1 bit is set at a time. --*/
INLINE int core_BitColToNum(int tmp)
{
	int data=0, i=0;
	do {
		if (tmp & 1) data += i;
		i++;
	} while (tmp >>= 1);
	return data;
}

extern MACHINE_DRIVER_EXTERN(PinMAME);
#endif /* INC_CORE */
