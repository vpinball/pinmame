#include "driver.h"
#include "sim.h"
#include "wmssnd.h"
#include "s6.h"

#define INITGAME(name, display, balls) \
static core_tGameData name##GameData = { GEN_S6, display }; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S6_INPUT_PORTS_START(name, balls) S6_INPUT_PORTS_END

/*--------------------------------
/ Laser Ball - Sys.6 (Game #493)
/-------------------------------*/
INITGAME(lzbal,s6_6digit_disp,1/*?*/)
S6_ROMSTART(lzbal,l2, "gamerom.716", 0x9c5ffe2f,
                      "green1.716",  0x2145f8ab,
                      "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(      "sound1.716",  0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(lzbal,l2,"Laser Ball (L-2)",1979,"Williams",s6_mS6S,0)

/*-----------------------------
/ Scorpion - Sys.6 (Game #494)
/----------------------------*/
INITGAME(scrpn,s6_6digit_disp,1/*?*/)
S6_ROMSTART(scrpn,l1,  "gamerom.716", 0x881109a9,
                       "green1.716",  0x2145f8ab,
                       "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(       "sound1.716",  0xf4190ca3)
S6_ROMEND
CORE_GAMEDEF(scrpn,l1,"Scorpion (L-1)",1980,"Williams",s6_mS6S,0)


/*----------------------------
/ Blackout - Sys.6 (Game #495)
/---------------------------*/
INITGAME(blkou,s6_6digit_disp,1/*?*/)
S6_ROMSTART(blkou,l1,  "gamerom.716", 0x04b407ae2,
                       "green1.716",  0x2145f8ab,
                       "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(       "sound2.716",  0xc9103a68)
S67S_SPEECHROMS000x(   "v_ic7.532" ,  0x087864071,
                       "v_ic5.532" ,  0x0046a96d8,
                       "v_ic6.532" ,  0x00104e5c4)
S6_ROMEND
CORE_GAMEDEF(blkou,l1,"Blackout (L-1)",1979,"Williams",s6_mS6S,0)


/*--------------------------
/ Gorgar - Sys.6 (Game #496)
/-------------------------*/
INITGAME(grgar,s6_6digit_disp,1/*?*/)
S6_ROMSTART(grgar,l1,  "gamerom.716", 0x1c6f3e48,
                       "green1.716",  0x2145f8ab,
                       "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(       "sound2.716",  0xc9103a68)
S67S_SPEECHROMS000x(   "v_ic7.532" ,  0x0b1879e3,
                       "v_ic5.532" ,  0x0ceaef37,
                       "v_ic6.532" ,  0x218290b9)
S6_ROMEND
CORE_GAMEDEF(grgar,l1,"Gorgar (L-1)",1979,"Williams",s6_mS6S,0)


/*-------------------------------
/ Firepower - Sys.6 (Game #497)
/------------------------------*/
INITGAME(frpwr,s6_6digit_disp,1/*?*/)
S6_ROMSTARTPROM(frpwr,l2, "gamerom.716", 0xfdd3b983,
                          "green1.716",  0x2145f8ab,
                          "green2.716",  0x1c978a4a,
                          "prom1.474",   0xfbb7299f,
                          "prom2.474",   0xf75ade1a,
                          "prom3.474",   0x242ec687)
S67S_SOUNDROMS8(          "sound3.716",  0x55a10d13)
S67S_SPEECHROMS000x(      "v_ic7.532",   0x94c5c0a7,
                          "v_ic5.532",   0x1737fdd2,
                          "v_ic6.532",   0xe56f7aa2)
S6_ROMEND
CORE_GAMEDEF(frpwr,l2,"Firepower (L-2)",1980,"Williams",s6_mS6S,0)

/* Following games used a 7 segment display */

/*--------------------------
/ Algar - Sys.6 (Game #499)
/-------------------------*/
INITGAME(algar,s6_7digit_disp,1/*?*/)
S6_ROMSTART(algar,l1, "gamerom.716", 0x06711da23,
                      "green1.716",  0x2145f8ab,
                      "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(      "sound.716",   0x067ea12e7)
S6_ROMEND
CORE_GAMEDEF(algar,l1,"Algar (L-1)",1980,"Williams",s6_mS6S,0)

/*-------------------------------
/ Alien Poker - Sys.6 (Game #501)
/-------------------------------*/
INITGAME(alpok,s6_7digit_disp,1/*?*/)
S6_ROMSTART(alpok,l2, "gamerom.716", 0x020538a4a,
                      "green1.716",  0x2145f8ab,
                      "green2.716",  0x1c978a4a)
S67S_SOUNDROMS8(      "sound2.716",  0x055a10d13)
S67S_SPEECHROMS000x(  "v_ic7.532" ,  0x0a66c7ca6,
                      "v_ic5.532" ,  0x0f16a237a,
                      "v_ic6.532" ,  0x015a3cc85)
S6_ROMEND
CORE_GAMEDEF(alpok,l2,"Alien Poker (L-2)",1980,"Williams",s6_mS6S,0)
