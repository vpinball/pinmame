// license:BSD-3-Clause

#pragma once

#include "core.h"
#include "sim.h"

#define PINHECK_CPUREGION  REGION_CPU1
#define PINHECK_PROPREGION REGION_USER1
#define PINHECK_BIOSREGION REGION_USER2
#define PINHECK_HEXREGION  REGION_USER3 /* a PIC32 image as Intel HEX, converted into PINHECK_CPUREGION */

/* drawn 2x2 per dot in PinMAME and VPinMAME; libpinmame hosts get the panel as sent */
#ifdef LIBPINMAME
#define PINHECK_VIDEO_SCALE 1
#else
#define PINHECK_VIDEO_SCALE 2
#endif
#define PINHECK_VIDEO_W   (128 * PINHECK_VIDEO_SCALE)
#define PINHECK_VIDEO_H   (32  * PINHECK_VIDEO_SCALE)
#define PINHECK_VIDEO_H64 (64  * PINHECK_VIDEO_SCALE) /* the 128x64 module */

#define PINHECK_SWLFLIP  4
#define PINHECK_SWRFLIP  3
#define PINHECK_CUSTSOLS 14

#define PINHECK_COMPORTS \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0001, IPT_TILT,    KEYCODE_INSERT) \
    COREPORT_BITDEF(0x0002, IPT_COIN1,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0004, IPT_START1,  IP_KEY_DEFAULT) \
    COREPORT_BITTOG(0x0008, "Coin Door", KEYCODE_END) \
    COREPORT_BIT(   0x0010, "Back",      KEYCODE_7) \
    COREPORT_BIT(   0x0020, "Enter",     KEYCODE_0) \
    COREPORT_BIT(   0x0040, "User",      KEYCODE_9)

#define PINHECK_INPUT_PORTS_START(name, balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    PINHECK_COMPORTS

#define PINHECK_INPUT_PORTS_END INPUT_PORTS_END

#define PINHECK_BIOS_ROMSTART(name) \
  ROM_START(name) \
    ROM_REGION(0x8000, PINHECK_BIOSREGION, 0) \
      ROM_LOAD("p8x32a.rom", 0x0000, 0x8000, CRC(f99b3070) SHA1(b7b4fdf4f096db7d18bda6355725cb42ae4a9378))

#define PINHECK_ROMSTART(name, prg, prgsize, prghash, prp, prphash) \
  PINHECK_BIOS_ROMSTART(name) \
    ROM_REGION(0x80000, PINHECK_CPUREGION, ROMREGION_ERASEFF) \
      ROM_LOAD(prg, 0x0000, prgsize, prghash) \
    ROM_REGION(0x8000, PINHECK_PROPREGION, 0) \
      ROM_LOAD(prp, 0x0000, 0x8000, prphash)

/* a PIC32 image in Intel HEX, programmed into flash at start (pinheck_flash_hex) */
#define PINHECK_HEX_ROMSTART(name, hex, hexsize, hexhash, prp, prphash) \
  PINHECK_BIOS_ROMSTART(name) \
    ROM_REGION(0x80000, PINHECK_CPUREGION, ROMREGION_ERASEFF) \
    ROM_REGION(hexsize, PINHECK_HEXREGION, 0) \
      ROM_LOAD(hex, 0x0000, hexsize, hexhash) \
    ROM_REGION(0x8000, PINHECK_PROPREGION, 0) \
      ROM_LOAD(prp, 0x0000, 0x8000, prphash)

#define PINHECK_ROMEND ROM_END

/* set name_ver as a clone of the pinheck BIOS; all versions share input_ports_name and init_name */
#define PINHECK_GAMEDEF(name, ver, longname, year, manuf, machine, flag) \
  GAMEX(year,name##_##ver,pinheck,machine,name,name,ROT0,manuf,longname,flag)

/* per-game data; core_gameData points at its core member */
typedef struct {
  core_tGameData core;
  int width, height;      /* display module in dots: 128 x 32 or 128 x 64, drawn in the look its config packet sets */
  int aligned;            /* the POSITION the look draws unshifted: the game's factory POSITION */
  int servoMin, servoMax; /* servo pulse widths in us drawn as servo levels 0 and 255 */
  int rgbInverted;        /* WS2801 lines inverted on the board; the driver supports 0 */
  int inService;          /* PINHECK_INSERVICE seeds the update record (version: core.hw.gameSpecific1) */
  int dmdHub;             /* a raw 128 x 32 DMD scanned by a Propeller cog: hub address of its 4 bpp frame; 0: the display link */
  int bootHold;           /* ms the bootloader stand-in holds the PIC32 after a reset without a sign-on */
  int onbLed2;            /* 1: a third on-board WS2801 LED, on outputs 62-64 (the external chain's LED 0 then has none) */
  /* the CPU-driven flipper coils (coil numbers 1-24, 0 none): right power, right hold, left power, left hold, then the
     same for upper right and upper left. pinheck.c mirrors them as PinMAME's flipper outputs 45-48 and 33-36; the
     game data declares them with FLIP_SOL. Games without them get 45-48 from the flipper buttons */
  int flipSols[8];
} pinheck_tGameData;

/* Domino's values (128 x 32, POSITION 340, servos 1.0-2.0 ms, display link, 3 s boot hold), also used by the system set */
#define PINHECK_DOMINOS_DATA 128, 32, 340, 1000, 2000, 0, 1, 0, 3000, 0

extern PINMAME_VIDEO_UPDATE(pinheck_video);
extern int pinheck_getsol(int solNo);
extern int pinheck_servo(int servo);
extern void pinheck_flash_hex(void);
extern MACHINE_DRIVER_EXTERN(PINHECK);
extern MACHINE_DRIVER_EXTERN(PINHECKDMD);
#define gl_mPINHECK PINHECK
#define gl_mPINHECKDMD PINHECKDMD
