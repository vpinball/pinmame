#ifndef INC_GTS80
#define INC_GTS80

#include "core.h"
#include "wpcsam.h"
#include "sim.h"

/*-------------------------
/ Machine driver constants
/--------------------------*/
/*-- Common Inports for GTS80Games --*/
#define GTS80_COMPORTS \
  PORT_START /* 0 */ \
    /* switch column 8 */ \
    COREPORT_BIT(     0x0001, "Test",             KEYCODE_8) \
    COREPORT_BITDEF(  0x0002, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN2,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0008, IPT_COIN3,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0020, IPT_TILT,           KEYCODE_INSERT) \
    /* These are put in switch column 0 */ \
    COREPORT_BIT(     0x8000, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BIT(     0x1000, "Sound Test",       KEYCODE_9)

#define GTS80VID_COMPORTS \
    COREPORT_BIT(     0x0f00, "Video Test",       KEYCODE_7) \
    COREPORT_BIT(     0x0100, "Joystick Right",   KEYCODE_RIGHT) \
    COREPORT_BIT(     0x0200, "Joystick Left",    KEYCODE_LEFT) \
    COREPORT_BIT(     0x0400, "Joystick Up",      KEYCODE_UP) \
    COREPORT_BIT(     0x0800, "Joystick Down",    KEYCODE_DOWN)

#define GTS80_DIPS \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0001, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S2") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0002, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0004, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0008, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0010, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0020, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0080, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "S9") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0100, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "S10") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0200, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0400, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0800, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x4000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "S16") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x8000, DEF_STR(On) ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S17") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0001, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "S18") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0002, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0004, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "S20") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0008, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "S21") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0010, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0020, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "S23") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S24") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0080, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0100, 0x0100, "S25") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0100, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0200, 0x0200, "S26") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0200, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0400, 0x0400, "S27") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0400, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0800, 0x0800, "S28") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0800, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x1000, 0x1000, "S29") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x2000, "S30") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "S31") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x4000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x8000, 0x8000, "S32") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x8000, DEF_STR(On) ) \
  PORT_START /* 3 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "SS:S1: Self Test") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0001, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "SS:S2: Not Used") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0002, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x000c, 0x0004, "SS:S3-S4: Attract Mode Speech") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0004, "10s" ) \
      COREPORT_DIPSET(0x0008, "2m" ) \
      COREPORT_DIPSET(0x000c, "4m" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "SS:S5: Background Sound ") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x0010, "Enabled" ) \
    COREPORT_DIPNAME( 0x0020, 0x0020, "SS:S6: Speech") \
      COREPORT_DIPSET(0x0000, "Disabled" ) \
      COREPORT_DIPSET(0x0020, "Enabled" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "SS:S7: Not Used") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "SS:S8: Not Used") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0080, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S:S1") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0100, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S:S2") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0200, DEF_STR(On) )

// 07:Test, 17:Coin1, etc.

/*-- Standard input ports --*/
#define GTS80_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS80_COMPORTS \
    GTS80_DIPS

#define GTS80VID_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    GTS80_COMPORTS \
    GTS80VID_COMPORTS \
    GTS80_DIPS

#define GTS80_INPUT_PORTS_END INPUT_PORTS_END

#define GTS80_COMINPORT       CORE_COREINPORT

/*-- GTS80 switch numbers --*/
#define GTS80_SWSLAMTILT	  -1

/*-- Memory regions --*/
#define GTS80_MEMREG_CPU	REGION_CPU1
#define GTS80_MEMREG_SCPU1	REGION_CPU2 /* used for sound, sound & speech and s3 sound board */
#define GTS80_MEMREG_SCPU2	REGION_CPU3 /* used for s80b and s3 sound board */
#define GTS80_MEMREG_SCPU3	REGION_CPU4 /* used for s80b sound board with extra DAC */
#define GTS80_MEMREG_VIDCPU	REGION_CPU3 /* Caveman Video CPU */

/* CPUs */
#define GTS80_CPU		0
#define GTS80_SCPU1		1
#define GTS80_SCPU2		2
#define GTS80_SCPU3		3
#define GTS80_VIDCPU	2

/*-- GTS80/GTS80A Main CPU regions and ROM, 2 game PROM version --*/
#define GTS80_2_ROMSTART(n1, chk1, n2, chk2) \
      ROM_LOAD(n1, 0x1000, 0x0200, chk1) \
        ROM_RELOAD(0x1400, 0x0200)       \
        ROM_RELOAD(0x5000, 0x0200)       \
        ROM_RELOAD(0x5400, 0x0200)       \
        ROM_RELOAD(0x9000, 0x0200)       \
        ROM_RELOAD(0x9400, 0x0200)       \
        ROM_RELOAD(0xd000, 0x0200)       \
        ROM_RELOAD(0xd400, 0x0200)       \
      ROM_LOAD(n2, 0x1200, 0x0200, chk2) \
        ROM_RELOAD(0x1600, 0x0200)       \
        ROM_RELOAD(0x5000, 0x0200)       \
        ROM_RELOAD(0x5600, 0x0200)       \
        ROM_RELOAD(0x9000, 0x0200)       \
        ROM_RELOAD(0x9600, 0x0200)       \
        ROM_RELOAD(0xd000, 0x0200)       \
        ROM_RELOAD(0xd600, 0x0200)

/*-- GTS80/GTS80A Main CPU regions and ROM, 1 game PROM version --*/
#define GTS80_1_ROMSTART(n1, chk1) \
      ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
        ROM_RELOAD(0x5000, 0x0800)       \
        ROM_RELOAD(0x9000, 0x0800)       \
        ROM_RELOAD(0xd000, 0x0800)

/*-- Video roms for Caveman, they are copied to their right place by the driver --*/
#define VIDEO_ROMSTART(n1,chk1,n2,chk2,n3,chk3,n4,chk4,n5,chk5,n6,chk6,n7,chk7,n8,chk8) \
     NORMALREGION(0x1000000, GTS80_MEMREG_VIDCPU) \
       ROM_LOAD16_BYTE(n1, 0x08000, 0x1000, chk1) \
         ROM_RELOAD(       0xf8000, 0x1000) \
       ROM_LOAD16_BYTE(n2, 0x08001, 0x1000, chk2) \
         ROM_RELOAD(       0xf8001, 0x1000) \
       ROM_LOAD16_BYTE(n3, 0x0a000, 0x1000, chk3) \
         ROM_RELOAD(       0xfa000, 0x1000) \
       ROM_LOAD16_BYTE(n4, 0x0a001, 0x1000, chk4) \
         ROM_RELOAD(       0xfa001, 0x1000) \
       ROM_LOAD16_BYTE(n5, 0x0c000, 0x1000, chk5) \
         ROM_RELOAD(       0xfc000, 0x1000) \
       ROM_LOAD16_BYTE(n6, 0x0c001, 0x1000, chk6) \
         ROM_RELOAD(       0xfc001, 0x1000) \
       ROM_LOAD16_BYTE(n7, 0x0e000, 0x1000, chk7) \
         ROM_RELOAD(       0xfe000, 0x1000) \
       ROM_LOAD16_BYTE(n8, 0x0e001, 0x1000, chk8) \
         ROM_RELOAD(       0xfe001, 0x1000)
#if 0
       ROM_LOAD(n1, 0x08000, 0x1000, chk1) \
       ROM_LOAD(n2, 0x09000, 0x1000, chk2) \
       ROM_LOAD(n3, 0x0a000, 0x1000, chk3) \
       ROM_LOAD(n4, 0x0b000, 0x1000, chk4) \
       ROM_LOAD(n5, 0x0c000, 0x1000, chk5) \
       ROM_LOAD(n6, 0x0d000, 0x1000, chk6) \
       ROM_LOAD(n7, 0x0e000, 0x1000, chk7) \
       ROM_LOAD(n8, 0x0f000, 0x1000, chk8)
#endif
/*-- GTS80B Main CPU regions and ROM, 8K single game PROM --*/
#define GTS80B_8K_ROMSTART(name, n1, chk1) \
  ROM_START(name) \
    NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
      ROM_LOAD(n1, 0x2000, 0x2000, chk1) \
        ROM_RELOAD(0x6000, 0x2000)       \
        ROM_RELOAD(0xa000, 0x2000)       \
        ROM_RELOAD(0xe000, 0x2000)

/*-- GTS80B Main CPU regions and ROM, 8K & 2K game PROM --*/
#define GTS80B_2K_ROMSTART(name, n1, chk1, n2, chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
      ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
        ROM_RELOAD(0x5000, 0x0800)       \
        ROM_RELOAD(0x9000, 0x0800)       \
        ROM_RELOAD(0xd000, 0x0800)       \
      ROM_LOAD(n2, 0x2000, 0x2000, chk2) \
        ROM_RELOAD(0x6000, 0x2000)       \
        ROM_RELOAD(0xa000, 0x2000)       \
        ROM_RELOAD(0xe000, 0x2000)

/*-- GTS80B Main CPU regions and ROM, 8K & 4K game PROM --*/
#define GTS80B_4K_ROMSTART(name, n1, chk1, n2, chk2) \
  ROM_START(name) \
    NORMALREGION(0x10000, GTS80_MEMREG_CPU) \
      ROM_LOAD(n1, 0x1000, 0x0800, chk1) \
        ROM_CONTINUE(0x9000, 0x0800)     \
        ROM_RELOAD(0x5000, 0x0800)       \
        ROM_CONTINUE(0xd000, 0x0800)     \
      ROM_LOAD(n2, 0x2000, 0x2000, chk2) \
        ROM_RELOAD(0x6000, 0x2000)       \
        ROM_RELOAD(0xa000, 0x2000)       \
        ROM_RELOAD(0xe000, 0x2000)

#define GTS80_ROMEND	ROM_END

/*-- These are only here so the game structure can be in the game file --*/
extern MACHINE_DRIVER_EXTERN(gts80s);
extern MACHINE_DRIVER_EXTERN(gts80ss);
extern MACHINE_DRIVER_EXTERN(gts80vid);
extern MACHINE_DRIVER_EXTERN(gts80b);
extern MACHINE_DRIVER_EXTERN(gts80bs1);
extern MACHINE_DRIVER_EXTERN(gts80bs2);
extern MACHINE_DRIVER_EXTERN(gts80bs3);
extern MACHINE_DRIVER_EXTERN(gts80bs3a);

#define gl_mGTS80S		gts80s
#define gl_mGTS80SS		gts80ss
#define gl_mGTS80VID	gts80vid
#define gl_mGTS80B		gts80b
#define gl_mGTS80BS1	gts80bs1
#define gl_mGTS80BS2	gts80bs2
#define gl_mGTS80BS3	gts80bs3
#define gl_mGTS80BS3A	gts80bs3a

/* Display types */
#define GTS80_DISPBCD   0x00
#define GTS80_DISPALPHA 0x01
#define GTS80_DISPVIDEO 0x02

extern core_tLCDLayout GTS80_dispCaveman[];

#endif /* INC_GTS80 */
