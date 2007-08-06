#ifndef INC_VPINTF
#define INC_VPINTF

#include "snd_cmd.h"

#ifdef VPINMAME
	#define GAME_NOCRC 0x2000	//This should allow mame team to add at least a few more flags to gamedrv before it's a problem
#else
	#define GAME_NOCRC 0
#endif

typedef struct { int lampNo, currStat; } vp_tChgLamps[CORE_MAXLAMPCOL*8];
typedef struct { int solNo,  currStat; } vp_tChgSols[64];
typedef struct { int giNo,   currStat; } vp_tChgGIs[CORE_MAXGI];
typedef struct { int ledNo,  chgSeg, currStat; } vp_tChgLED[60];
typedef struct { int sndNo; } vp_tChgSound[MAX_CMD_LOG];

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
INLINE int vp_getSolenoid(int solNo) { return core_getSol(solNo); }

/*-------------------------------------------
/  get status of a GIString (0=off, 1=on)
/ (WPC games only)
/-------------------------------------*/
INLINE int vp_getGI(int giNo) { return coreGlobals.gi[giNo]; }

/*-------------------------------------------
/  get all lamps changed since last call
/  returns number of canged lamps
/-------------------------------------*/
int vp_getChangedLamps(vp_tChgLamps chgStat);

/*-------------------------------------------
/  get all solenoids changed since last call
/  returns number of canged solenoids
/-------------------------------------*/
int vp_getChangedSolenoids(vp_tChgSols chgStat);

/*-------------------------------------------
/  get all GIstrings changed since last call
/  returns number of canged GIstrings
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
int vp_getChangedLEDs(vp_tChgLED chgStat, UINT64 mask);

#endif /* INC_VPINTF */
