#ifndef INC_RECEL
#define INC_RECEL

#include "core.h"
#include "sim.h"

/* Recel System III (Rockwell PPS-4/2). Object numbering follows the factory
   service manuals: switch = strobe*10 + bit index (A..D = 1..4), solenoid =
   PIO output number, lamp = printed register code. */

#define RECEL_SOLSMOOTH     4
#define RECEL_LAMPSMOOTH    1
#define RECEL_DISPLAYSMOOTH 1

#define RECEL_MEMREG_CPU  REGION_CPU1
#define RECEL_MEMREG_PROM REGION_USER1

#define RECEL_CPU 0

/* IOL device ids */
#define RECEL_DEV_B2   0x2  /* A1762 - playfield lamp registers */
#define RECEL_DEV_B1   0x4  /* A1761 - NVRAM and printer control */
#define RECEL_DEV_PIO  0xD  /* 11696 */
#define RECEL_DEV_GPKD 0xF  /* 10788 */

/* Inport for the cabinet switches (strobes 8-9), read by SWITCH_UPDATE(RECEL).
   Bit layout matches the MAIN SWITCH CODE table (platform-level, same on every
   machine): low nibble = strobe 8 (A=Fault,B=Coin3,C=Coin1,D=Coin2), high
   nibble = strobe 9 (A=Tilt/Door,B=Replays,C=Button2,D=Button1). */
#define RECEL_COMINPORT CORE_COREINPORT

#define RECEL_COMPORTS \
  PORT_START /* 2 */ \
    COREPORT_BIT(   0x0001, "Fault",       KEYCODE_7) \
    COREPORT_BIT(   0x0002, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0004, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0008, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0010, "Tilt/Door",   KEYCODE_DEL) \
    COREPORT_BIT(   0x0020, "Replays",     KEYCODE_6) \
    COREPORT_BIT(   0x0040, "Button 2",    KEYCODE_2) \
    COREPORT_BIT(   0x0080, "Button 1",    KEYCODE_1)

#define RECEL_INPUT_PORTS_START(name, balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    RECEL_COMPORTS

#define RECEL_INPUT_PORTS_END INPUT_PORTS_END

/* The two spider chips are 2KB dumps whose upper half is unprogrammed (A11 is
   tied to VSS on the board, so only the first 1KB is reachable). Both are
   loaded whole and overlapping: a2362 lands on a2361's zero upper half, and
   its own zero upper half falls in 0x800-0xBFF, which recel_decode_prom()
   fills afterwards. Order matters. The raw game PROM goes to REGION_USER1. */
#define RECEL_ROMSTART(name, promfile, promsize, promhash) \
  ROM_START(name) \
    NORMALREGION(0x1000, RECEL_MEMREG_CPU) \
      ROM_LOAD("a2361.b1", 0x0000, 0x0800, CRC(d0c4695d) SHA1(4846adb3f6c292626840ba5255ffc5e788a69301)) \
      ROM_LOAD("a2362.b2", 0x0400, 0x0800, CRC(39a70611) SHA1(8545e168a5f256150bcff12d1e6d8efffd08c3cd)) \
    NORMALREGION(0x0800, RECEL_MEMREG_PROM) \
      ROM_LOAD(promfile, 0x0000, promsize, promhash)

#define RECEL_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(RECEL);
#define gl_mRECEL RECEL

/* recelsnd.c: discrete sound, PIO outputs 0-5 */
extern MACHINE_DRIVER_EXTERN(recel_snd);
void recel_snd_w(int bits);

#endif /* INC_RECEL */
