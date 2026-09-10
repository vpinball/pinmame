#ifndef INC_RECEL
#define INC_RECEL

#include "core.h"
#include "sim.h"

/* Recel System III (Rockwell PPS-4/2). Switch and solenoid numbering follows
   the factory service manuals: switch = strobe*10 + bit index (A..D = 1..4),
   solenoid = PIO output number -- true of the raw bitmask in
   coreGlobals.solenoids, but PinMAME's own user-facing solenoid ids (via
   core_getSol(), the generic /api/monitor?type=sol endpoint) are one higher;
   see tests/test_solenoids.py. Lamps do NOT follow the factory register
   code: there is no MDRV_LAMP_CONV, so a lamp is exposed at its raw A1762
   (device 0x2) line index instead -- column 0 = lines 0-7 = factory codes
   51/52/54/58/41/42/44/48, column 1 = lines 8-15 = factory codes
   31/32/34/38/21/22/24/28 (RECEL_LAMP_CODES below, in line order). lamp2m/
   m2lamp are consumed only by src/wpc/vpintf.c (VPinMAME, a declared
   spec non-goal), and vpintf assumes lamps start at column 1 while Recel's
   live in columns 0-1, so implementing the conversion would only serve that
   out-of-scope consumer. See docs/driver-notes.md for the recorded
   deviation from spec §5.4. */

#define RECEL_LAMPSMOOTH    1

#define RECEL_MEMREG_CPU  REGION_CPU1
#define RECEL_MEMREG_PROM REGION_USER1

/* IOL device ids */
#define RECEL_DEV_B2   0x2  /* A1762 - playfield lamp registers */
#define RECEL_DEV_B1   0x4  /* A1761 - NVRAM and printer control */
#define RECEL_DEV_PIO  0xD  /* 11696 */
#define RECEL_DEV_GPKD 0xF  /* 10788 */

/* A1762 (device 0x2) line -> factory lamp code, index = line (see the header
   comment above). Kept as a single parseable list so tests/test_lamps.py can
   derive its code_to_line() mapping from here instead of duplicating it. */
#define RECEL_LAMP_CODES { 51,52,54,58,41,42,44,48, 31,32,34,38,21,22,24,28 }

/* GPKD (device 0xF) columns 2, 8, 9 and A are latched in a 7475, not decoded
   by a 7448 (docs/gpkd-protocol.md §6) -- they are not digits. Each is
   exposed as its raw 4-bit nibble (DA1 = bit 0 .. DA4 = bit 3) in a custom
   lamp column instead, one column per source. Group B's columns 8/9 drive no
   indicator on a real machine and get no column. Custom columns start at
   CORE_CUSTLAMPCOL (8) since Recel's real lamp driver (A1762) only ever uses
   columns 0-1. */
#define RECEL_LAMPCOL_P1STATUS  (CORE_CUSTLAMPCOL+0)  /* group A col 2 */
#define RECEL_LAMPCOL_GAMESTATE (CORE_CUSTLAMPCOL+1)  /* group A col 8: ball/tilt/game over */
#define RECEL_LAMPCOL_P2STATUS  (CORE_CUSTLAMPCOL+2)  /* group A col A */
#define RECEL_LAMPCOL_P4STATUS  (CORE_CUSTLAMPCOL+3)  /* group B col 2 */
#define RECEL_LAMPCOL_P3STATUS  (CORE_CUSTLAMPCOL+4)  /* group B col A */
/* Column 9 is the match number: a decoded digit, laid out in recel_disp, not
   a lamp. See gpkd_kind() in recel.c. */
/* hw.lampCol: core.c draws and counts CORE_CUSTLAMPCOL + lampCol columns, so
   without this the six columns above exist in coreGlobals.lampMatrix but are
   never rendered -- the ball-in-play/game-over indicator was invisible on
   screen, leaving a started game looking identical to attract. */
#define RECEL_LAMPCOLS 5

/* Inport for the cabinet switches (strobes 8-9), read by SWITCH_UPDATE(RECEL).
   Bit layout matches the MAIN SWITCH CODE table (platform-level, same on every
   machine): low nibble = strobe 8 (A=Fault,B=Coin3,C=Coin1,D=Coin2), high
   nibble = strobe 9 (A=Tilt/Door,B=Replays,C=Button2,D=Button1).

   The manual's "BUTTON 1"/"BUTTON 2" are S1/S2, the two adjustment buttons
   inside the door -- SELECT 1 and SELECT 2 (system3-operation-maintenance.md
   3.5). The player's button is the REPLAYS one: measured, it is the only one
   of the three that serves a ball, and it refuses to with no credit up. So
   that is what carries KEYCODE_1 and the name "Start", and the two door
   buttons move out of the way to 8 and 9. tests/test_cabinet.py. */
#define RECEL_COMINPORT CORE_COREINPORT

#define RECEL_COMPORTS \
  PORT_START /* 2 */ \
    COREPORT_BIT(   0x0001, "Fault",       KEYCODE_7) \
    COREPORT_BIT(   0x0002, "Coin 3",      KEYCODE_5) \
    COREPORT_BIT(   0x0004, "Coin 1",      KEYCODE_3) \
    COREPORT_BIT(   0x0008, "Coin 2",      KEYCODE_4) \
    COREPORT_BIT(   0x0010, "Tilt/Door",   KEYCODE_DEL) \
    COREPORT_BIT(   0x0020, "Start",       KEYCODE_1) \
    COREPORT_BIT(   0x0040, "Select 2",    KEYCODE_9) \
    COREPORT_BIT(   0x0080, "Select 1",    KEYCODE_8)

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
   fills afterwards. Order matters. This is the shared program -- the BIOS --
   common to every machine; it ships no game PROM of its own (roms/pinmame/
   recel.zip contains only these two dumps), so it is what the `recel`
   NOT_A_DRIVER parent set must be built from. Mirrors gts1.c's
   GTS1_2_ROMSTART pattern (src/wpc/gts1.h). */
/* The region has to span every address the PPS-4 core can put on the bus, not
   just the ROM: RM/WM mask to 0x1fff (pps4.c), and MRA_RAM/MWA_RAM at
   0x1000-0x10ff are backed by this region at that offset. Sizing it 0x1000
   left every RAM access one byte past the end, which corrupted the adjacent
   heap chunk and aborted in free() at exit. */
#define RECEL_BIOS_ROMSTART(name) \
  ROM_START(name) \
    NORMALREGION(0x2000, RECEL_MEMREG_CPU) \
      ROM_LOAD("a2361.b1", 0x0000, 0x0800, CRC(d0c4695d) SHA1(4846adb3f6c292626840ba5255ffc5e788a69301)) \
      ROM_LOAD("a2362.b2", 0x0400, 0x0800, CRC(39a70611) SHA1(8545e168a5f256150bcff12d1e6d8efffd08c3cd))

/* A real game: the BIOS plus its own game PROM, decoded through the BICs by
   recel_decode_prom() and loaded raw into REGION_USER1. */
#define RECEL_ROMSTART(name, promfile, promsize, promhash) \
  RECEL_BIOS_ROMSTART(name) \
    NORMALREGION(0x0800, RECEL_MEMREG_PROM) \
      ROM_LOAD(promfile, 0x0000, promsize, promhash)

#define RECEL_ROMEND ROM_END

extern MACHINE_DRIVER_EXTERN(RECEL);
#define gl_mRECEL RECEL

/* recelsnd.c: discrete sound, PIO outputs 0-5 */
extern MACHINE_DRIVER_EXTERN(recel_snd);
void recel_snd_w(int bits);

#endif /* INC_RECEL */
