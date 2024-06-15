#ifndef INC_VPINTF
#define INC_VPINTF
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "snd_cmd.h"

#ifdef VPINMAME
	#define GAME_NOCRC 0x2000	//This should allow mame team to add at least a few more flags to gamedrv before it's a problem
#else
	#define GAME_NOCRC 0
#endif

typedef struct { int lampNo, currStat; } vp_tChgLamps[CORE_MODOUT_LAMP_MAX];
typedef struct { int solNo,  currStat; } vp_tChgSols[CORE_MODOUT_SOL_MAX];
typedef struct { int giNo,   currStat; } vp_tChgGIs[CORE_MODOUT_GI_MAX];
typedef struct { int ledNo, chgSeg, currStat; } vp_tChgLED[CORE_SEGCOUNT];
typedef struct { int sndNo; } vp_tChgSound[MAX_CMD_LOG];
typedef struct { int nvramNo, oldStat, currStat; } vp_tChgNVRAMs[CORE_MAXNVRAM];

#define VP_OUT_SOLENOID          0 /* Solenoid output type */
#define VP_OUT_LAMP              1 /* Lamp output type */
#define VP_OUT_GI                2 /* Global Illumination output type */
#define VP_OUT_ALPHASEG          3 /* Alpha Numeric segment output type */

#define VP_MAXDIPBANKS 10
/*----------------------------------------------------
/ Switches/Lamps are numbered differently in WPCgames
/----------------------------------------------------*/
#define WPCNUMBERING (core_gameData->gen & GEN_ALLWPC)
#define S80NUMBERING (core_gameData->gen & GEN_ALLS80)

/*-------------------------------
/  Initialise/reset the VP interface
/--------------------------------*/
void vp_init(void);

/*-------------------------------
/  Check if WPCNumbering is used
/--------------------------------*/
INLINE int vp_getWPCNumbering(void) { return WPCNUMBERING; }

/*------------------------------------
/  get status of a lamp (0=off, 1=on)
/-------------------------------------*/
int vp_getLamp(int lampNo);

/*------------------------------------
/  set status of a switch (0=off, !0=on)
/-------------------------------------*/
INLINE void vp_putSwitch(int swNo, int newStat) { core_setSw(swNo, newStat); }

/*------------------------------------
/  get status of a switch (0=off, !0=on)
/-------------------------------------*/
INLINE int vp_getSwitch(int swNo) { return core_getSw(swNo); }

/*------------------------------------
/  get status of a solenoid (0=off, !0=on)
/-------------------------------------*/
int vp_getSolenoid(int solNo);

/*-------------------------------------------
/  get status of a GIString (0=off, !0=on)
/ (WPC games only)
/-------------------------------------*/
int vp_getGI(int giNo);

/*-------------------------------------------
/  get all lamps changed since last call
/  returns number of changed lamps
/-------------------------------------*/
int vp_getChangedLamps(vp_tChgLamps chgStat);

/*-------------------------------------------
/  get all solenoids changed since last call
/  returns number of changed solenoids
/-------------------------------------*/
int vp_getChangedSolenoids(vp_tChgSols chgStat);

/*-------------------------------------------
/  get all GIstrings changed since last call
/  returns number of changed GIstrings
/-------------------------------------*/
int vp_getChangedGI(vp_tChgGIs chgStat);

/*------------------------------------
/  get status of a game specific mechanic
/-------------------------------------*/
int vp_getMech(int mechNo);

/*-----------
/  DIPs
/-----------*/
void vp_setDIP(int dipBank, int value);
int vp_getDIP(int dipBank);

/*-----------
/  set Solenoid Mask
/-----------*/
void vp_setSolMask(int low, int mask);

/*-----------
/  get Solenoid Mask
/-----------*/
int vp_getSolMask(int low);

/*-----------
/  set Output Modulation Type
/-----------*/
void vp_setModOutputType(int output, int no, int type);

/*-----------
/  get Output Modulation Type
/-----------*/
int vp_getModOutputType(int output, int no);

/*-----------
/  set a time fence where emulation is suspended when reached
/-----------*/
void vp_setTimeFence(double timeInS);

/*-----------
/ Mechanics
/------------*/
void vp_setMechData(int para, int data);

/*-- used from core.c --*/
UINT64 vp_getSolMask64(void);

/*-------------------------------------------------
/  get all sound commands issued since last call
/------------------------------------------------*/
int vp_getNewSoundCommands(vp_tChgSound chgSound);

/*-------------------------------------------------
/ get alpha digit value
/-------------------------------------------------*/
int vp_getChangedLEDs(vp_tChgLED chgStat, UINT64 mask, UINT64 mask2);

#endif /* INC_VPINTF */
