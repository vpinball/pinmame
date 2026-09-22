// license:BSD-3-Clause

#include "driver.h"
#include "gen.h"
#include "sim.h"
#include "sleic.h"

#define INITGAME(name, disptype, balls) \
	SLEIC_INPUT_PORTS_START(name, balls) SLEIC_INPUT_PORTS_END \
	static core_tGameData name##GameData = {GEN_SLEIC,disptype,{FLIP_SW(FLIP_L)}}; \
	static void init_##name(void) { \
		core_gameData = &name##GameData; \
	}

/* Dot-Matrix display (128 x 32) */
core_tLCDLayout sleic_dispDMD[] = {
  {0,0,32,128,CORE_DMD,NULL,NULL}, {0}
};

/*-------------------------------------------------------------------
/ Bike Race (1992)
/-------------------------------------------------------------------*/
/* "Balls" is 0 on all three Bike Race sets, and for this family that is not a ball count
   -- it is the OFF position of the driver's optional ball-present model
   (sleic3_ball_update in sleic.c).  Off is the PinMAME convention: swMatrix[5]'s COL4
   optos are ordinary switches, and closing them is the frontend's job -- a VPinMAME table
   script's, or standalone the matrix test keys 8 and '-'.  Io Moon does the same and sits
   on "FALTA 1 BOLA" until its trough contacts close.

   Set "Balls" to any non-zero value to turn the model on for standalone desktop play,
   where nothing else is going to close them.  Unlike Io Moon there is no meaningful
   number here: the firmware answers a ball-PRESENT query rather than counting a trough,
   so the model presents the whole complement or none of it, and the cabinet port's "Ball
   out of trough" key (Backspace) lifts it while held */
INITGAME(bikerace, sleic_dispDMD, 0)
SLEIC_ROMSTART7(bikerace,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "bkcpu04.bin", CRC(ce745e89) SHA1(04ba97a9ef1e60a7609c87cf6d8fcae2d0e32621),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bkio07.bin",  CRC(b52a9d4f) SHA1(726a4d9b354729d7390d2a4f877dc480701ec795))
SLEIC_ROMEND
CORE_GAMEDEFNV(bikerace,"Bike Race",1992,"Sleic (Spain)",gl_mSLEIC3,0)

INITGAME(bikerac2, sleic_dispDMD, 0)
SLEIC_ROMSTART7(bikerac2,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bksnd03.bin", CRC(b6d00245) SHA1(f7da6f2ca681fbe62ea9cab7f92d3e501b7e867d),
						 "04.bin",      CRC(aaaa4a8a) SHA1(ff579041575da4060615da2ff634f3aa91537751),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "07.bin",      CRC(0b763a89) SHA1(8952d7b13674e1599e53cce96e57c2783899a90a))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac2,bikerace,"Bike Race (2-ball play)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* V4.1 -- the newest of the three known Bike Race sets.  Three chips differ from
/  the parent: the OKI sample ROM 03 (by 229 bytes), the game code 04 and the Z80
/  I/O code 07, both full rebuilds.  Chips 01, 02, 05 and 06 are the parent's.
/
/  For 02 and 05 that inheritance is verified against a
/  complete six-chip pull off a V4.1 machine.  01 was never dumped.  06 IS
/  INHERITED because a re-dump confirmed it is the parent's: the first V4.1 read
/  of ROM 06, CRC ad48a30a, was a BAD DUMP, and a re-read came back CRC 9db436d4,
/  byte-identical to bkcpu06. So V4.1 is a genuine three-chip clone,
/  03/04/07 over the parent, and inheriting bkcpu06 is correct rather than inferred.
/  The bad image (ad48a30a) and this analysis are archived at
/  sleic-iomoon/roms/related-machines/bike-race/v4.1/ */
INITGAME(bikerac3, sleic_dispDMD, 0)
SLEIC_ROMSTART7(bikerac3,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04.bin",    CRC(33fd212e) SHA1(9471e34fc4280741816d65f88590febc9e8629a7),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerac3,bikerace,"Bike Race (V4.1)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/* MOD of the V4.1 set carrying two patches on chip 04, both in caves in the ROM's zero
/  padding at F9163 (which nothing in the ROM targets):
/    FREE PLAY   -- the machine always has a credit standing, so START begins a game with
/                   no coin.  Hook at E059F, the triplicated-credit read.
/    PRESS START -- the end of a game holds the finished game's SCORES on the panel until
/                   START is pressed.  TWO hooks into one shared hold, because two
/                   different things take the scores down: F000:10F2, the screen loader
/                   that follows a one-, two- or three-player game, and E8C5D, the panel
/                   blank ahead of E8C67's "Partida" overlay, which is what a FOUR-player
/                   game reaches first.  Both are guarded on the state byte [0116:0099]
/                   (4 = a game is running) and the player count [0116:0072], which keeps
/                   the hold out of mid-game resets and out of the power-on path, where an
/                   unguarded wait swallows the first START press and no game can be
/                   started at all.  [0010:1030], a byte the firmware never touches, is the
/                   latch that gives a game exactly one hold.  The hold releases the way
/                   the firmware's own START waits do: one press can queue more than one
/                   36h, so it settles for 16 steps of the free-running digit [0010:000B]
/                   (~320 ms) and then calls E9C7:2F47 to scrub the rest, as EC2FA and
/                   E9486 do.  Only chip
/  04 changes; the rest are V4.1's.  Bike Race has no free-play adjustment of its own --
/  its CREDITOS page is coin pricing only, and no ROM in the set contains GRATIS, LIBRE
/  or FREE.
/
/  Bypassing the START handler's credit test at EC3B4 does NOT work: the mode byte
/  [0116:0076] is promoted 0 -> 1 by the stock code only BECAUSE a credit appeared, and
/  the handler refuses at its first instruction in mode 0.  So the machine is given a
/  real credit instead, at the one place every credit value comes from -- E000:059F
/  reads the triplicated credit byte (0083/0105/0278 in the store window at segment
/  1040) and the cache [0116:0070] is recomputed from it, so flooring that read at 1
/  leaves every downstream path stock.  The end-of-game match award still banks its
/  credit through that same path.
/
/  "Balls" is 3 rather than the other sets' 0: this is a play mod, so the optional
/  ball-present model is ON and a desktop START press begins a game with nothing else
/  to close the trough optos.
/
/  Generated by sleic-iomoon/scripts/bike_race_free_play_patch.py and
/  bike_race_press_start_patch.py */
INITGAME(bikerc3f, sleic_dispDMD, 3)
SLEIC_ROMSTART7(bikerc3f,"bkdsp01.bin", CRC(9b220fcb) SHA1(54e82705d8ce8a26d9e1b5f0fe382ded1f2070c3),
						 "bksnd02.bin", CRC(d67b3883) SHA1(712022b9b24c6ab559d020ab8e2106f68b4d7896),
						 "bk03.bin",    CRC(74c10536) SHA1(43a2a63494b044fe2326ee09831ef90f37d3b432),
						 "bk04f.bin",   CRC(7626564e) SHA1(d69c5dd794c55d91d4dde5c6c07009bb1ebe2c41),
						 "bkcpu05.bin", CRC(072ce879) SHA1(4f6fb044592feb4c72bbdcbe5f19e063c0e49d0d),
						 "bkcpu06.bin", CRC(9db436d4) SHA1(3869524c0490e0a019d2f8ab46546ff42727665e),
						 "bk07.bin",    CRC(200ff3fc) SHA1(96fc8561b078c5306b15e260436e3d3ba562c51d))
SLEIC_ROMEND
CORE_CLONEDEFNV(bikerc3f,bikerace,"Bike Race (V4.1, free play + press start)",1992,"Sleic (Spain)",gl_mSLEIC3,0)

/*-------------------------------------------------------------------
/ Sleic Pin-Ball (1993)
/-------------------------------------------------------------------*/
INITGAME(sleicpin, sleic_dispDMD, 1)
SLEIC_ROMSTART4(sleicpin,"sp01-1_1.rom", CRC(240015bb) SHA1(0e647718173ad59dafbf3b5bc84bef3c33886e23),
						 "sp02-1_1.rom", CRC(0e4851a0) SHA1(0692ee2df0b560e2013db9c03fd27c6eb12e618d),
						 "sp03-1_1.rom", CRC(261b0ae4) SHA1(e7d9d1c2cab7776afb732701b0b8697b62a8d990),
						 "sp04-1_1.rom", CRC(84514cfa) SHA1(6aa87b86892afa534cf963821f08286c126b4245))
SLEIC_ROMEND
CORE_GAMEDEFNV(sleicpin,"Sleic Pin-Ball",1993,"Sleic (Spain)",gl_mSLEIC1,0)
