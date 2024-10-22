#ifndef INC_GTS3
#define INC_GTS3
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define GTS3_ROMEND	ROM_END

/*-- Common Inports for System 3 --*/
#define GTS3_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0008, IPT_START1,         IP_KEY_DEFAULT) \
	COREPORT_BIT(     0x0010, "Left Advance",     KEYCODE_7) \
	COREPORT_BIT(     0x0020, "Right Advance",    KEYCODE_8) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BIT(     0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME)

/*-- Standard input ports --*/
#define GTS3_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS3_COMPORTS

#define GTS3_INPUT_PORTS_END INPUT_PORTS_END

#define GTS31_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BIT(     0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME) \
    PORT_BITX(0x4000,IP_ACTIVE_LOW,IPT_UNUSED,"",0,IP_JOY_NONE) // flag 0x4000

#define GTS31_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS31_COMPORTS

#define GTS32_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    COREPORT_BITTOG(  0x0020, "Tournament",       KEYCODE_2) \
    COREPORT_BITTOG(  0x0040, "Coin Door",        KEYCODE_END) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Diagnostic",       KEYCODE_0) \
    COREPORT_BIT(     0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BITTOG(  0x0800, "Printer",          KEYCODE_9) \
    PORT_BITX(0x8000,IP_ACTIVE_LOW,IPT_UNUSED,"",0,IP_JOY_NONE) // flag 0x8000

#define GTS32_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls)\
    GTS32_COMPORTS

#define GTS3_IC_COMPORTS \
  PORT_START /* 0 */ \
      /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0100, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BIT(     0x0200, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BIT(     0x0400, "Diagnostic",       KEYCODE_0) \
    PORT_BITX(0x4000,IP_ACTIVE_LOW,IPT_UNUSED,"",0,IP_JOY_NONE) // flag 0x4000

#define GTS3_IC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls)\
    GTS3_IC_COMPORTS


#define GTS3_COMINPORT       CORE_COREINPORT

#define GTS3_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
#define GTS3_LEDSMOOTH       1 /* Smooth the LED display over this number of VBLANKS */

/*-- To access C-side multiplexed solenoid/flasher --*/
#define GTS3_CSOL(x) ((x)+24)

/*-- S80 switch numbers --*/
#define GTS3_SWDIAG     -8
#define GTS3_SWTILT     -7
#define GTS3_SWSLAM     -6
#define GTS3_SWPRIN     -5


/*-- Memory regions --*/
#define GTS3_MEMREG_DROM1	REGION_USER1
#define GTS3_MEMREG_DROM2	REGION_USER2

#define GTS3_MEMREG_CPU		REGION_CPU1
#define GTS3_MEMREG_DCPU1	REGION_CPU2
#define GTS3_MEMREG_DCPU2	REGION_CPU3

#define GTS3_MEMREG_SCPU1	REGION_CPU3
#define GTS3_MEMREG_SCPU2	REGION_CPU4
#define GTS3_MEMREG_SROM1	REGION_SOUND1


/*-- Main CPU regions and ROM --*/

/*-- 32K CPU ROM --*/
#define GTS3ROMSTART32(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS3_MEMREG_CPU) \
       ROM_LOAD(n1, 0x8000, 0x8000, chk1)

/*-- 64K CPU ROM --*/
#define GTS3ROMSTART(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS3_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0x10000, chk1)

/*-- unknown CPU ROM --*/
#define GTS3ROMSTARTX(name, n1, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, GTS3_MEMREG_CPU) \
       ROM_LOAD(n1, 0x0000, 0, chk1)

/**************************************/
/** DMD (128x32) ROM 256K            **/
/**************************************/
#define GTS3_DMD256_ROMSTART(n1,chk1) \
     NORMALREGION(0x10000, GTS3_MEMREG_DCPU1)\
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x40000, chk1)\
       ROM_RELOAD(  0x40000, 0x40000)

/**************************************/
/** DMD (128x32) ROM 256K (2 boards) **/
/**************************************/
#define GTS3_DMD256_ROMSTART2(n1,chk1) \
     NORMALREGION(0x10000, GTS3_MEMREG_DCPU1) \
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x40000, chk1) \
       ROM_RELOAD(  0x40000, 0x40000) \
     NORMALREGION(0x10000, GTS3_MEMREG_DCPU2) \
     NORMALREGION(0x80000, GTS3_MEMREG_DROM2) \
       ROM_LOAD(n1, 0x00000, 0x40000, chk1) \
       ROM_RELOAD(  0x40000, 0x40000)

/**************************************/
/** DMD (128x32) ROM 512K            **/
/**************************************/
#define GTS3_DMD512_ROMSTART(n1,chk1) \
     NORMALREGION(0x10000, GTS3_MEMREG_DCPU1)\
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0x80000, chk1)

/**************************************/
/** DMD (128x32) unknown ROM         **/
/**************************************/
#define GTS3_DMD_ROMSTARTX(n1,chk1) \
     NORMALREGION(0x10000, GTS3_MEMREG_DCPU1)\
     NORMALREGION(0x80000, GTS3_MEMREG_DROM1) \
       ROM_LOAD(n1, 0x00000, 0, chk1)

extern void UpdateSoundLEDS(int num,UINT8 bit);
extern PINMAME_VIDEO_UPDATE(gts3_dmd128x32a);
extern PINMAME_VIDEO_UPDATE(gts3_dmd128x32b);

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(gts3_1a);
extern MACHINE_DRIVER_EXTERN(gts3_1as);
extern MACHINE_DRIVER_EXTERN(gts3_1as_no);
extern MACHINE_DRIVER_EXTERN(gts3_1as80b2);
extern MACHINE_DRIVER_EXTERN(gts3_1as80b3);
extern MACHINE_DRIVER_EXTERN(gts3_1b);
extern MACHINE_DRIVER_EXTERN(gts3_1bs);
extern MACHINE_DRIVER_EXTERN(gts3_21);
extern MACHINE_DRIVER_EXTERN(gts3_22);

#define mGTS3           gts3_1a
#define mGTS3S          gts3_1as
#define mGTS3SNO        gts3_1as_no
#define mGTS3S80B2      gts3_1as80b2
#define mGTS3S80B3      gts3_1as80b3
#define mGTS3B          gts3_1b
#define mGTS3BS         gts3_1bs
#define mGTS3DMDS       gts3_21
#define mGTS3DMDS2      gts3_22

typedef struct {
  int   has2DMD;
  UINT8 pa0; // bool
  UINT8 pa1; // bool
  UINT8 pa2; // bool
  UINT8 pa3; // bool
  UINT8 a18; // bool
  UINT8 q3; // bool
  int   dmd_latch;
  int   diagnosticLed;
  UINT8 status1; // bool
  UINT8 status2; // bool
  UINT8 dstrb; // bool
  UINT8 dmd_visible_addr;
  core_tDMDPWMState pwm_state;
} GTS3_DMDlocals;

typedef struct {
  UINT8  alphagen; //0,1,2
  int    alphaNumCol, alphaNumColShiftRegister;
  core_tWord activeSegments[2]; // Realtime active alphanum segments
  int    interfaceUpdateCount;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  UINT8  diagnosticLed;  // bool
  UINT8  diagnosticLed1; // bool
  UINT8  diagnosticLed2; // bool
  UINT8  swDiag; // bool
  UINT8  swTilt; // bool
  UINT8  swSlam; // bool
  UINT8  swPrin; // bool
  UINT8  u4pb;
  READ_HANDLER((*U4_PB_R));
  WRITE_HANDLER((*DISPLAY_CONTROL));
  WRITE_HANDLER((*AUX_W));
  UINT8  ax[7], cx1, cx2, ex1;
  UINT8  extra16led; // bool
  UINT8  sound_data;
  UINT8  prn[8];

  UINT8 irq;

  int modsol_rate_counter;
} tGTS3locals;

#endif /* INC_GTS3 */
