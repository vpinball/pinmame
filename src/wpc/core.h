#ifndef INC_CORE
#define INC_CORE

#include "wpcsam.h"
#include "gen.h"

/* 240201 Corrected solenoid handling once again */
/* 090501 Added CORE_GAMEDEFNV for games without version number */
/* 080801 Moved generations to gen.h (SJE) */

/*-- some convenience macros --*/
#ifndef FALSE
  #define FALSE (0)
#endif
#ifndef TRUE
  #define TRUE (1)
#endif

#ifdef MAME_DEBUG
  #define DBGLOG(x) logerror x
#else
  #define DBGLOG(x)
#endif

#if defined(MAMEVER) && MAMEVER >= 3709
  #define NORMALREGION(size, reg)  ROM_REGION(size, reg, 0)
  #define NORMALREGIONE(size, reg) ROM_REGION(size, reg, ROMREGION_ERASE)
  #define SOUNDREGION(size ,reg)   ROM_REGION(size, reg, ROMREGION_SOUNDONLY)
  #define SOUNDREGIONE(size ,reg)  ROM_REGION(size, reg, ROMREGION_SOUNDONLY|ROMREGION_ERASE)
#else /* MAMEVER */
  #define NORMALREGION(size, reg)  ROM_REGION(size, reg)
  #define NORMALREGIONE(size, reg) ROM_REGION(size, reg)
  #define SOUNDREGION(size ,reg)   ROM_REGION(size, reg | REGIONFLAG_SOUNDONLY)
  #define SOUNDREGIONE(size ,reg)  ROM_REGION(size, reg | REGIONFLAG_SOUNDONLY)
  #define memory_set_opbase_handler(a,b) cpu_setOPbaseoverride(a,b)
#endif /* MAMEVER */

#ifdef PINMAME_EXIT
  #define CORE_EXITFUNC(x) x,
  #define CORE_DOEXIT(x)
#else
  #define CORE_EXITFUNC(x)
  #define CORE_DOEXIT(x) x()
#endif


/*-- no of DMD frames to add together to create shades --*/
/*-- (hardcoded, do not change)                        --*/
#define DMD_FRAMES         3

/*-- default screen size */
#ifdef VPINMAME
#  define CORE_SCREENX 640
#  define CORE_SCREENY 320
#else /* VPINMAME */
#  define CORE_SCREENX 256
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
    COREPORT_BIT(0x0001, "Left Flipper",   KEYCODE_LCONTROL)  \
    COREPORT_BIT(0x0002, "Right Flipper",  KEYCODE_RCONTROL) \
    COREPORT_BIT(0x0004, "U Left Flipper", KEYCODE_LEFT)  \
    COREPORT_BIT(0x0008, "U Right Flipper",KEYCODE_RIGHT)

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
#if 0
#define CORE_SEG16  0                 // 16 seg alphanumeric
#define CORE_SEG8   1                 //  7 seg + comma
#define CORE_SEG7   2                 //  7 seg
#define CORE_SEG7S  3                 //  7 seg small
#define CORE_SEG87  4                 //  7 seg + comma every 3
#define CORE_SEG8H  (CORE_SEG8+8)     //  7 seg + comma, use high bits
#define CORE_SEG7H  (CORE_SEG7+8)     //  7 seg use high bits
#define CORE_SEG87H (CORE_SEG87+8)    //  7 seg + comma every 3, use high bits
#define CORE_SEG7SH (CORE_SEG7S+8)    //  7 seg small, use high bits
#else
#define CORE_DMD      7 // DMD Display
#define CORE_SEG16    0 // 16 segements
#define CORE_SEG10    1 // 10 segments
#define CORE_SEG8     2 // 8  segments
#define CORE_SEG7     3 // 7  segments
#define CORE_SEG87    4 // 7  segments, comma every three
#define CORE_SEG87F   5 // 7  segments, forced comma every three
#define CORE_SEG7S    6 // 7  segements, small

#define CORE_SEG10H   (CORE_SEG10 +8)
#define CORE_SEG8H    (CORE_SEG8  +8)
#define CORE_SEG7H    (CORE_SEG7  +8)
#define CORE_SEG87H   (CORE_SEG87 +8)
#define CORE_SEG87FH  (CORE_SEG87F+8)
#define CORE_SEG7SH   (CORE_SEG7S +8)

#define CORE_DUMMYZERO(x) ((x)*16)

#define DMD_MAXX 192
#define DMD_MAXY 64


#endif
/* Shortcuts for some common display sizes */
#define DISP_SEG_16(row,type)    {4*row, 0, 20*row, 16, type}
#define DISP_SEG_7(row,col,type) {4*row,16*col,row*20+col*8+1,7,type}
#define DISP_SEG_CREDIT(no1,no2,type) {2,2,no1,1,type},{2,4,no2,1,type}
#define DISP_SEG_BALLS(no1,no2,type)  {2,8,no1,1,type},{2,10,no2,1,type}
/* display layout structure */
typedef struct {
  UINT8 top, left, start, length, type;
} core_tLCDLayout, *core_ptLCDLayout;

typedef UINT8 tDMDDot[DMD_MAXY+2][DMD_MAXX+2];
void dmd_draw(struct mame_bitmap *bitmap, tDMDDot dotCol, core_ptLCDLayout layout);
/* Generic display handler. requires LCD layout in GameData structure */
extern void gen_refresh(struct mame_bitmap *bitmap, int fullRefresh);

/*----------------------
/ WPC driver constants
/-----------------------*/
/* Solenoid numbering */
/*       WPC          */
/*  1-28 Standard                */
/* 33-36 Upper flipper solenoids */
/* 37-40 Standard (WPC95 only)   */
/* 41-44 - "" - Copy of above    */
/* 45-48 Lower flipper solenoids */
/* 49-50 Simulated               */
/* 51-44                         */
/*       S9/S11            */
/*  1- 8 Standard 'A'-side */
/*  9-16 Standard          */
/* 17-22 Special           */
/* 23    Flipper & SS Enabled Sol (fake) */
/* 25-32 Standard 'C'-side */
/* 37-41 Sound overlay board */
/*       S7 */
/*  1-16 Standard */
/* 17-24 Special */
/* 25    Flipper & SS Enabled Sol (fake) */
/*       BY17/BY35         */
/*  1-15 Standard Pulse */
/* 17-20 Standard Hold */
#define CORE_STDSOLS       28

#define CORE_FIRSTEXTSOL   37
#define CORE_FIRSTUFLIPSOL 33
#define CORE_FIRSTCUSTSOL  51
#define CORE_FIRSTLFLIPSOL 45
#define CORE_FIRSTSIMSOL   49

#define CORE_LASTSTDSOL   28
#define CORE_LASTEXTSOL   44
#define CORE_LASTUFLIPSOL 36
#define CORE_LASTLFLIPSOL 48
#define CORE_LASTSIMSOL   50

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

#define CORE_STDLAMPCOLS   8
#define CORE_STDSWCOLS    12

#define CORE_COINDOORSWCOL   0   /* internal array number */
#define CORE_MAXSWCOL       16   /* switch columns (0-9=sw matrix, 10=coin door, 11=cabinet/flippers) */
#define CORE_FLIPPERSWCOL   11   /* internal array number */
#define CORE_CUSTSWCOL     CORE_STDSWCOLS  /* first custom (game specific) switch column */
#define CORE_MAXLAMPCOL     20   /* lamp column (0-7=std lamp matrix 8- custom) */
#define CORE_CUSTLAMPCOL   CORE_STDLAMPCOLS  /* first custom lamp column */
#define CORE_MAXPORTS        8   /* Maximum input ports */
#define CORE_MAXGI           5   /* Maximum GI strings */

/*-- create a custom switch number --*/
/* example: #define swCustom CORE_CUSTSWNO(1,2)  // custom column 1 row 2 */
#define CORE_CUSTSWNO(c,r) ((CORE_CUSTSWCOL-1+c)*10+r)

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

#define SEQ_SWNO(x) (x)

#define CORE_FIRSTSIMROW   80 /* first free row on display */
#define CORE_COLOR(x)      Machine->pens[(x)]
#define DMD_DOTOFF  1
#define DMD_DOT33   2
#define DMD_DOT66   3
#define DMD_DOTON   4

/*-- Colours --*/
#define DMD_COLORS  5   /* Includes Background color, DMD Dot Off, On, 33% & 66% */
#define LAMP_COLORS 8   /* # of Colors available for use in Playfield Lamps*/
                        /* Includes BLACK as 1 of the colors */

/* Marks where the antialias palette entries begin */
#define START_ANTIALIAS (DMD_COLORS+(LAMP_COLORS*2))

/* Colors Begin where DMD Ends */
#define BLACK       (DMD_COLORS+0)
#define WHITE       (DMD_COLORS+1)
#define GREEN       (DMD_COLORS+2)
#define RED         (DMD_COLORS+3)
#define ORANGE      (DMD_COLORS+4)
#define YELLOW      (DMD_COLORS+5)
#define LBLUE       (DMD_COLORS+6)
#define LPURPLE     (DMD_COLORS+7)
#define SHADE(x)    ((x)+LAMP_COLORS)     /*Mark where the colors shade entry is*/

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
  core_tLampData lamps[88];      /*Can support up to 88 lamps!*/
} core_tLampDisplay;

/*-- Interface data --*/
typedef struct {
  UINT8 *DMDFrames[DMD_FRAMES];
  int    nextDMDFrame;
  int    dmdOnly;         /* draw only the dmd (or alpha segemnts) */
  int    DMDsize;         /* 1=compact, 2=Normal */
} core_tGlobals_dmd;
extern core_tGlobals_dmd coreGlobals_dmd;

#ifdef LSB_FIRST
typedef struct { UINT8 lo, hi, dmy1, dmy2; } core_tSeg[3][20];
#else /* LSB_FIRST */
typedef struct { UINT8 dmy2, dmy1, hi, lo; } core_tSeg[3][20];
#endif /* LSB_FIRST */
typedef struct {
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  invSw[CORE_MAXSWCOL];   /* Active low switches */
  UINT8  lampMatrix[CORE_MAXLAMPCOL], tmpLampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments;
  UINT32 solenoids;       /* on power driver bord */
  UINT32 solenoids2;      /* flipper solenoids */
  UINT32 pulsedSolState;  /* current pulse value of solenoids on driver board */
  UINT64 lastSol;         /* last state of all solenoids */
  int    gi[CORE_MAXGI];  /* WPC gi strings */
  int    simAvail;        /* simulator (keys) available */
  int    soundEn;         /* Sound enabled ? */
  int    flipTimer[4];    /* time since flipper was activated (used for EOS simulation) */
  int    diagnosticLed;	  /* data relating to diagnostic led(s)*/
} core_tGlobals;
extern core_tGlobals coreGlobals;
/* shortcut for coreGlobals */
#define cg coreGlobals

/*Exported variables*/

extern unsigned char core_palette[(DMD_COLORS+(LAMP_COLORS*2)+7)][3];

/*-- There are no custom fields in the game driver --*/
/*-- so I have to invent some by myself. Each driver --*/
/*-- fills in one of these in the game_init function --*/
typedef struct {
  UINT64  gen;                /* Hardware Generation */
  core_ptLCDLayout lcdLayout; /* LCD display layout */
  struct {
    UINT32  flippers;      /* flippers installed (see defines below) */
    int     swCol, lampCol, custSol; /* Custom switch columns, lamp columns and solenoids */
    /*-- custom functions --*/
    int  (*getSol)(int solNo);        /* get state of custom solenoid */
    void (*handleMech)(int mech);     /* update switches depending on playfield mechanics */
    int  (*getMech)(int mechNo);      /* get status of mechanics */
    void (*drawMech)(BMTYPE **line); /* draw game specific hardware */
    core_tLampDisplay *lampData;      /* lamp layout */
    wpc_tSamSolMap   *solsammap;      /* solenoids samples */
  } hw;
  void *simData;
  struct { /* WPC specific stuff */
    char serialNo[21];  /* Securty chip serial number */
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
extern core_tGameData *core_gameData;

/*-- each core must fill in one of these --*/
typedef struct {
  int  coreDips;               /* Number of core DIPs */
  void (*updSw)(int *inport);  /* update core specific switches */
  int  diagLEDs;               /* number of diagnostic LEDs */
  mem_write_handler sndCmd;    /* send a sound command */
  char sndHead[10];            /* heading in sound.dat */
  int (*sw2m)(int no);         /* conversion function for switch */
  int (*lamp2m)(int no);       /* conversion function for lamps */
  int (*m2sw)(int col, int row);
  int (*m2lamp)(int col, int row);
} core_tData;
#define CORE_DIAG7SEG           0xff
#define DIAGLED_VERTICAL	0x100	/*Flag indicated DIAG LEDS are Vertically Positioned*/
extern core_tData coreData;
extern const int core_bcd2seg[]; /* BCD to 7 segment display */

/*-- Exported Display handling functions--*/
void core_initpalette(unsigned char *game_palette, unsigned short *game_colortable,
                     const unsigned char *color_prom);
void drawStatus(struct mame_bitmap *bitmap, int fullRefresh);
void core_updateSw(int flipEn);

/*-- text output functions --*/
void core_textOut(char *buf, int length, int x, int y, int color);
void CLIB_DECL core_textOutf(int x, int y, int color, char *text, ...);

/*-- lamp handling --*/
void core_setLamp(UINT8 *lampMatrix, int col, int row);

/*-- switch handling --*/
extern int core_swSeq2m(int no);
extern int core_m2swSeq(int col, int row);
extern void core_setSw(int swNo, int value);
extern int core_getSw(int swNo);
extern void core_updInvSw(int swNo, int inv);

/*-- get a switch column. (colEn=bits) --*/
extern int core_getSwCol(int colEn);

/*-- solenoid handling --*/
extern int core_getSol(int solNo);
extern int core_getPulsedSol(int solNo);
extern UINT64 core_getAllSol(void);

/*-- nvram handling --*/
extern void core_nvram(void *file, int write, void *mem, int length, UINT8 init);

/* makes it easier to swap bits */
extern const UINT8 core_swapNyb[16];
#define CORE_SWAPBYTE(x) ((core_swapNyb[(x)&0xf]<<4)|(core_swapNyb[(x)>>4]))
#define CORE_SWAPNYB(x)  core_swapNyb[(x)]
/*-- core DIP handling --*/
/*---------------------------------------
/  Get the status of a DIP bank (8 dips)
/-----------------------------------------*/
extern int core_getDip(int dipBank);

/*-- startup/shutdown --*/
int core_init(core_tData *cd);
void core_exit(void);

#endif /* INC_CORE */
