#include "driver.h"
#include "sim.h"
#include "sndbrd.h"
#include "recel.h"

/* Lite box layout. System III Operation and Maintenance manual §7.4.

   coreGlobals.segments keeps the 10788's scan-time index: group A at 0-15,
   group B at 16-31. The display runs the other way -- the write pointer
   starts at time 15 and walks down -- so the higher scan time is a field's
   leftmost digit, and each field is spelled out descending because PinMAME
   layouts only run left to right.

     row 1   player 1  A7..A3   free play A0, extra ball A1 (two 1-digit units)
     row 2   player 2  AF..AB   match number A9
     row 3   player 3  BF..BB   --
     row 4   player 4  B7..B3   credit: tens B0, units B1 (one 2-digit unit)

   Columns 2 and A (player status LEDs) and A8 (ball/tilt/game over) are
   latched lamps rather than digits and are not laid out here; see
   gpkd_kind() in recel.c. B8/B9 drive no indicator on a real machine. */
#define RECEL_D(row, col, pos) {row, col, pos, 1, CORE_SEG7},
/* One counter: the five GPKD-multiplexed digits, MSD first, then the x1
   digit. base = the x10 digit's GPKD position. The x1 is not multiplexed --
   the 095-105 unit's sixth 7448 position is wired to a permanent 0 -- so it
   comes from RECEL_SEG_UNITS. */
#define RECEL_COUNTER(row, col, base) \
  RECEL_D(row, col,    (base)+4) RECEL_D(row, (col)+2,  (base)+3) \
  RECEL_D(row, (col)+4,(base)+2) RECEL_D(row, (col)+6,  (base)+1) \
  RECEL_D(row, (col)+8,(base))   RECEL_D(row, (col)+10, RECEL_SEG_UNITS)

static core_tLCDLayout recel_disp[] = {
  RECEL_COUNTER(0, 0,  3) RECEL_D(0, 14, 0) RECEL_D(0, 18, 1)
  RECEL_COUNTER(2, 0, 11) RECEL_D(2, 14, 9)
  RECEL_COUNTER(4, 0, 27)
  RECEL_COUNTER(6, 0, 19) RECEL_D(6, 14, 16) RECEL_D(6, 16, 17)
  {0}
};

#define INIT_RECEL(name, dsp, hwver) \
RECEL_INPUT_PORTS_START(name, 1) RECEL_INPUT_PORTS_END \
static core_tGameData name##GameData = { \
  GEN_RECEL, dsp, {FLIP_SW(FLIP_L),0,RECEL_LAMPCOLS,0,SNDBRD_NONE,0,hwver}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

/*-------------------------------------------------------------------
/ Recel System III shared program (not a game)
/-------------------------------------------------------------------*/
INIT_RECEL(recel, recel_disp, 1)
RECEL_BIOS_ROMSTART(recel)
RECEL_ROMEND
GAMEX(1978,recel,0,RECEL,recel,recel,ROT0,"Recel","System III",NOT_A_DRIVER)

/*-------------------------------------------------------------------
/ Fair Fight (1978) - model 1.053, doc 035-623
/-------------------------------------------------------------------*/
INIT_RECEL(r_fairfght, recel_disp, 1)
RECEL_ROMSTART(r_fairfght, "fa.c5", 0x0100, CRC(5d3694da) SHA1(4d0a8033acb6ef2e2af107f76540fd19b4a39b12))
RECEL_ROMEND
CORE_CLONEDEFNV(r_fairfght,recel,"Fair Fight",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Mr. Evil (1978) - model 1.055, doc 035-626
/-------------------------------------------------------------------*/
INIT_RECEL(r_mrevil, recel_disp, 1)
RECEL_ROMSTART(r_mrevil, "me.c5", 0x0100, CRC(53ce24a0) SHA1(42d376e3e7a4e94a09db2f974af8d4869579d0f5))
RECEL_ROMEND
CORE_CLONEDEFNV(r_mrevil,recel,"Mr. Evil",1978,"Recel",gl_mRECEL,0)

/*-------------------------------------------------------------------
/ Mr. Doom (1979) - model 1.060, doc 035-635. Mr. Evil re-themed: same
/ switch matrix and driver table, different ROM and rules.
/-------------------------------------------------------------------*/
INIT_RECEL(r_mrdoom, recel_disp, 1)
RECEL_ROMSTART(r_mrdoom, "md.c5", 0x0100, CRC(ca679a69) SHA1(f08f0cfe646f08882473dcd5d23889fffe4a03c8))
RECEL_ROMEND
CORE_CLONEDEFNV(r_mrdoom,recel,"Mr. Doom",1979,"Recel",gl_mRECEL,0)
