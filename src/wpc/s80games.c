#include "driver.h"
#include "sim.h"
#include "s80.h"
#include "s80sound0.h"
#include "s80sound1.h"
#include "s80sound2.h"

//	static core_tGameData name##GameData = {gen, disptype, {FLIP_SWNO(S80_SWNO(8), S80_SWNO(18))}};

#define INITGAME(name, gen, disptype, balls) \
	S80_INPUT_PORTS_START(name, balls) S80_INPUT_PORTS_END \
	static core_tGameData name##GameData = {gen, disptype}; \
	static void init_##name(void) { \
	  core_gameData = &name##GameData; \
	}

#define DISP_SEG_6(row,col,type) {4*row,20*col,row*16+col*8+2,6,type}

/* 4 X 7 (6) AlphaNumeric Rows, 2 X 2 AlphaNumeric */
core_tLCDLayout gottlieb_dispNumeric1[] = {
  DISP_SEG_6(0,0, CORE_SEG16), DISP_SEG_6(0,1, CORE_SEG16),
  DISP_SEG_6(1,0, CORE_SEG16), DISP_SEG_6(1,1, CORE_SEG16),
  DISP_SEG_CREDIT(16,24,CORE_SEG16),DISP_SEG_BALLS(0,8,CORE_SEG16),
  {0}
};

/* 4 X 6 AlphaNumeric Rows, 2 X 2 AlphaNumeric, 1 X 6 AlphaNumeric */
core_tLCDLayout gottlieb_dispNumeric2[] = {
  DISP_SEG_6(0,0, CORE_SEG16), DISP_SEG_6(0,1, CORE_SEG16),
  DISP_SEG_6(1,0, CORE_SEG16), DISP_SEG_6(1,1, CORE_SEG16),
  DISP_SEG_CREDIT(16,24,CORE_SEG16),DISP_SEG_BALLS(0,8,CORE_SEG16),
  {6, 10, 34, 6, CORE_SEG16},
  {0}
};

/* 4 X 6+1 AlphaNumeric Rows, 2 X 2 AlphaNumeric */
core_tLCDLayout gottlieb_dispNumeric3[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},
  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),
  {0}
};

#define DISP_SEG_20(row,type) {2*row, 0, 20*row, 20, type}

/* 2 X 20 AlphaNumeric Rows */
core_tLCDLayout gottlieb_dispAlpha[] = {
	DISP_SEG_20(0,CORE_SEG16),
	DISP_SEG_20(1,CORE_SEG16),
	{0}
};

/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */

// System80

/*-------------------------------------------------------------------
/ Spiderman
/-------------------------------------------------------------------*/
INITGAME(spidermn,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(spidermn, "653-1.cpu",    0x674ddc58,
                         "653-2.cpu",    0xff1ddfd7,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "653.snd",      0xf5650c46,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(spidermn,"Spiderman",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Panthera
/-------------------------------------------------------------------*/
INITGAME(panthera,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(panthera, "652.cpu",      0x5386e5fb,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "652.snd",      0x4d0cf2c0,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(panthera,"Panthera",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Circus
/-------------------------------------------------------------------*/
INITGAME(circus,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(circus, "654-1.cpu",    0x0eeb2731,
                       "654-2.cpu",    0x01e23569,
                       "u2_80.bin",    0x4f0bc7b1,
                       "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(   "654.snd",      0x75c3ad67,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(circus,"Circus",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Counterforce
/-------------------------------------------------------------------*/
INITGAME(cntforce,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(cntforce, "656-1.cpu",    0x42baf51d,
                         "656-2.cpu",    0x0e185c30,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "656.snd",      0x0be2cbe9,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(cntforce,"Counterforce",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Star Race
/-------------------------------------------------------------------*/
INITGAME(starrace,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(starrace, "657-1.cpu",    0x27081372,
                         "657-2.cpu",    0xc56e31c8,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "657.snd",      0x3a1d3995,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(starrace,"Star Race",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ James Bond (Timed Play)
/-------------------------------------------------------------------*/
INITGAME(jamesb,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(jamesb, "658-1.cpu",    0xb841ad7a,
                       "u2_80.bin",    0x4f0bc7b1,
                       "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(   "658.snd",      0x962c03df,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(jamesb,"James Bond (Timed Play)",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ James Bond (3/5 Ball Play)
/-------------------------------------------------------------------*/
INITGAME(jamesb2,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(jamesb2, "658-x.cpu",    0xe7e0febf,
                        "u2_80.bin",    0x4f0bc7b1,
                        "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(    "658.snd",      0x962c03df,
                        "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_CLONEDEFNV(jamesb2,jamesb,"James Bond (3/5-Ball)",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Time Line
/-------------------------------------------------------------------*/
INITGAME(timeline,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(timeline, "659.cpu",      0x0d6950e3b,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "659.snd",      0x28185568,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(timeline,"Time Line",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Force II
/-------------------------------------------------------------------*/
INITGAME(forceii,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(forceii, "661-2.cpu",    0xa4fa42a4,
                        "u2_80.bin",    0x4f0bc7b1,
                        "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(    "661.snd",      0x650158a7,
                        "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(forceii,"Force II",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Pink Panther
/-------------------------------------------------------------------*/
INITGAME(pnkpnthr,GEN_S80,gottlieb_dispNumeric1, 3)
S80_1_ROMSTART(pnkpnthr, "664-1.cpu",    0xa0d3e69a,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "664.snd",      0x18f4abfd,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(pnkpnthr,"Pink Panther",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Mars - God of War
/-------------------------------------------------------------------*/
INITGAME(mars,GEN_S80SS,gottlieb_dispNumeric1, 3)
S80_1_ROMSTART(mars, "666-1.cpu",  0xbb7d476a,
                     "u2_80.bin",  0x4f0bc7b1,
                     "u3_80.bin",  0x1e69f9d0)
S80SOUND22_ROMSTART( "666-s1.snd", 0xd33dc8a5,
                     "666-s2.snd", 0xe5616f3e)
S80_ROMEND
CORE_GAMEDEFNV(mars,"Mars - God of War",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Volcano (Sound and Speech)
/-------------------------------------------------------------------*/
INITGAME(vlcno_ax,GEN_S80SS,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(vlcno_ax, "667-a-x.cpu", 0x1f51c351,
                         "u2_80.bin",   0x4f0bc7b1,
                         "u3_80.bin",   0x1e69f9d0)
S80SOUND22_ROMSTART(     "667-s1.snd",  0xba9d40b7,
                         "667-s2.snd",  0xb54bd123)
S80_ROMEND
CORE_GAMEDEFNV(vlcno_ax,"Volcano",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Volcano (Sound Only) 
/-------------------------------------------------------------------*/
INITGAME(vlcno_1b,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(vlcno_1b,"667-1b.cpu" ,  0xa422d862,
                        "u2_80.bin",    0x4f0bc7b1,
                        "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(    "667-a-s.snd",  0x894b4e2e,
                        "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_CLONEDEFNV(vlcno_1b,vlcno_ax,"Volcano (Sound Only)",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 4)
/-------------------------------------------------------------------*/
INITGAME(blckhole,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(blckhole, "668-4.cpu",  0x01b53045,
                         "u2_80.bin",  0x4f0bc7b1,
                         "u3_80.bin",  0x1e69f9d0)
S80SOUND22_ROMSTART(     "668-s1.snd", 0x23d5045d,
                         "668-s2.snd", 0xd63da498)
S80_ROMEND
CORE_GAMEDEFNV(blckhole,"Black Hole",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Black Hole (Rev. 2)
/-------------------------------------------------------------------*/
INITGAME(blkhole2,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(blkhole2, "668-2.cpu",  0xdf03ffea,
                         "u2_80.bin",  0x4f0bc7b1,
                         "u3_80.bin",  0x1e69f9d0)
S80SOUND22_ROMSTART(     "668-s1.snd", 0x23d5045d,
                         "668-s2.snd", 0xd63da498)
S80_ROMEND
CORE_CLONEDEFNV(blkhole2,blckhole,"Black Hole (Rev. 2)",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Black Hole (Sound Only) 
/-------------------------------------------------------------------*/
INITGAME(blkholea,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(blkholea, "668-a2.cpu" ,  0xdf56f896,
                         "u2_80.bin",    0x4f0bc7b1,
                         "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(     "668-a-s.snd",  0x5175f307,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_CLONEDEFNV(blkholea,blckhole,"Black Hole (Sound Only)",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Haunted House, since serial no. 5000
/-------------------------------------------------------------------*/
INITGAME(hh,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(hh,  "669-2.cpu",  0xf3085f77,
                    "u2_80.bin",  0x4f0bc7b1,
                    "u3_80.bin",  0x1e69f9d0)
S80SOUND22_ROMSTART("669-s1.snd", 0x52ec7335,
                    "669-s2.snd", 0xa3317b4b)
S80_ROMEND
CORE_GAMEDEFNV(hh,"Haunted House (Rev 2)",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Haunted House up to serial no. 4999
/-------------------------------------------------------------------*/
INITGAME(hh_1,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(hh_1, "669-1.cpu",  0x96e72b93,
                     "u2_80.bin",  0x4f0bc7b1,
                     "u3_80.bin",  0x1e69f9d0)
S80SOUND22_ROMSTART( "669-s1.snd", 0x52ec7335,
                     "669-s2.snd", 0xa3317b4b)
S80_ROMEND
CORE_CLONEDEFNV(hh_1,hh,"Haunted House (Rev 1)",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Eclipse
/-------------------------------------------------------------------*/
INITGAME(eclipse,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(eclipse, "671-a.cpu",    0xefad7312,
                        "u2_80.bin",    0x4f0bc7b1,
                        "u3_80.bin",    0x1e69f9d0)
S80SOUND1K_ROMSTART(    "671-a-s.snd",  0x5175f307,
                        "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(eclipse,"Eclipse",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ System 80 Test Fixture
/-------------------------------------------------------------------*/
INITGAME(s80tst,GEN_S80SS,gottlieb_dispNumeric1, 3)
S80_1_ROMSTART(s80tst, "80tst.cpu",    0xa0f9e56b,
                       "u2_80.bin",    0x4f0bc7b1,
                       "u3_80.bin",    0x1e69f9d0)
S80SOUND22_ROMSTART(   "80tst-s1.snd", 0xb9dbdd21,
                       "80tst-s2.snd", 0x1a4b1e9d)
S80_ROMEND
CORE_GAMEDEFNV(s80tst,"System 80 Test",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

// System 80a

/*-------------------------------------------------------------------
/ Devil's Dare (Sound and Speech)
/-------------------------------------------------------------------*/
core_tLCDLayout dispDevilsdare[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {6, 9,34,6,CORE_SEG16}, {0}
};
INITGAME(dvlsdre,GEN_S80SS,dispDevilsdare, 1)
S80_1_ROMSTART(dvlsdre, "670-1.cpu",  0x6318bce2,
                        "u2_80a.bin", 0x241de1d4,
                        "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(    "670-s1.snd", 0x506bc22a,
                        "670-s2.snd", 0xf662ee4b)
S80_ROMEND
CORE_GAMEDEFNV(dvlsdre,"Devil's Dare",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Devil's Dare (Sound Only)
/-------------------------------------------------------------------*/
INITGAME(dvlsdre2,GEN_S80,dispDevilsdare, 1)
S80_1_ROMSTART(dvlsdre2, "670-a.cpu",    0x353b2e18,
                         "u2_80a.bin",   0x241de1d4,
                         "u3_80a.bin",   0x2d77ccdc)
S80SOUND1K_ROMSTART(     "670-a-s.snd",  0xf141d535,
                         "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_CLONEDEFNV(dvlsdre2,dvlsdre,"Devil's Dare (Sound Only)",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Rocky
/-------------------------------------------------------------------*/
core_tLCDLayout dispRocky[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {6, 10,38,2,CORE_SEG16}, {6,16,34,2,CORE_SEG16}, {0}
};
INITGAME(rocky,GEN_S80SS,dispRocky, 1)
S80_1_ROMSTART(rocky, "672-2x.cpu", 0x8e2f0d39,
                      "u2_80a.bin", 0x241de1d4,
                      "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(  "672-s1.snd", 0x10ba523c,
                      "672-s2.snd", 0x5e77117a)
S80_ROMEND
CORE_GAMEDEFNV(rocky,"Rocky",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Caveman
/-------------------------------------------------------------------*/
INITGAME(caveman,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(caveman, "pv810-1.cpu",  0xdd8d516c,
                        "u2_80a.bin",   0x241de1d4,
                        "u3_80a.bin",   0x2d77ccdc)
S80SOUND22_ROMSTART(    "pv810-s1.snd", 0xa491664d,
                        "pv810-s2.snd", 0xd8654e6e)
S80_ROMEND
CORE_GAMEDEFNV(caveman,"Caveman",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Spirit
/-------------------------------------------------------------------*/
core_tLCDLayout dispSpirit[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {6, 9,34,6,CORE_SEG16}, {0}
};
INITGAME(spirit,GEN_S80SS,dispSpirit, 1)
S80_1_ROMSTART(spirit, "673-2.cpu",  0xa7dc2207,
                       "u2_80a.bin", 0x241de1d4,
                       "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(   "673-s1.snd", 0xfd3062ae,
                       "673-s2.snd", 0x7cf923f1)
S80_ROMEND
CORE_GAMEDEFNV(spirit,"Spirit",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Striker
/-------------------------------------------------------------------*/
core_tLCDLayout dispStriker[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,21,18,6,CORE_SEG16}, {0,33,17,1,CORE_SEG16},
  {4, 0,10,6,CORE_SEG16}, {4,12, 9,1,CORE_SEG16},
  {4,21,26,6,CORE_SEG16}, {4,33,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {0,15,36,2,CORE_SEG16}, {0,36,46,2,CORE_SEG16},
  {4,15,34,2,CORE_SEG16}, {4,36,44,2,CORE_SEG16}, {0}
};
INITGAME(striker,GEN_S80SS,dispStriker, 1)
S80_1_ROMSTART(striker, "675.cpu",    0x06b66ce8,
                        "u2_80a.bin", 0x241de1d4,
                        "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(    "675-s1.snd", 0xcc11c487,
                        "675-s2.snd", 0xec30a3d9)
S80_ROMEND
CORE_GAMEDEFNV(striker,"Striker",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Punk!
/-------------------------------------------------------------------*/
INITGAME(punk,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(punk, "674.cpu",    0x70cccc57,
                     "u2_80a.bin", 0x241de1d4,
                     "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART( "674-s1.snd", 0xb75f79d5,
                     "674-s2.snd", 0x005d123a)
S80_ROMEND
CORE_GAMEDEFNV(punk,"Punk!",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Goin' Nuts
/-------------------------------------------------------------------*/
core_tLCDLayout dispGoinNuts[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {6,12,37,3,CORE_SEG16}, {0}
};
INITGAME(goinnuts,GEN_S80SS,dispGoinNuts, 1)
S80_1_ROMSTART(goinnuts, "682.cpu",    0x51c7c6de,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(     "682-s1.snd", 0xf00dabf3,
                         "682-s2.snd", 0x3be8ac5f)
S80_ROMEND
CORE_GAMEDEFNV(goinnuts,"Goin' Nuts",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Krull
/-------------------------------------------------------------------*/
core_tLCDLayout dispKrull[] = {
  {0, 0, 2,6,CORE_SEG16}, {0,12, 1,1,CORE_SEG16},
  {0,16,10,6,CORE_SEG16}, {0,28, 9,1,CORE_SEG16},
  {4, 0,18,6,CORE_SEG16}, {4,12,17,1,CORE_SEG16},
  {4,16,26,6,CORE_SEG16}, {4,28,25,1,CORE_SEG16},

  DISP_SEG_CREDIT(16,24,CORE_SEG16), DISP_SEG_BALLS(0,8,CORE_SEG16),

  {6, 8,34,3,CORE_SEG16}, {6,16,37,3,CORE_SEG16}, {0}
};
INITGAME(krull,GEN_S80SS,dispKrull, 1)
S80_1_ROMSTART(krull, "676-3.cpu",  0x71507430,
                      "u2_80a.bin", 0x241de1d4,
                      "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(  "676-s1.snd", 0xb1989d8f,
                      "676-s2.snd", 0x05fade11)
S80_ROMEND
CORE_GAMEDEFNV(krull,"Krull",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Q*Bert's Quest
/-------------------------------------------------------------------*/
INITGAME(qbquest,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(qbquest, "677.cpu",    0xfd885874,
                        "u2_80a.bin", 0x241de1d4,
                        "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(    "677-s1.snd", 0xaf7bc8b7,
                        "677-s2.snd", 0x820aa26f)
S80_ROMEND
CORE_GAMEDEFNV(qbquest,"Q*Bert's Quest",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Super Orbit
/-------------------------------------------------------------------*/
INITGAME(sorbit,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(sorbit, "680.cpu",    0xdecf84e6,
                       "u2_80a.bin", 0x241de1d4,
                       "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(   "680-s1.snd", 0xfccbbbdd,
                       "680-s2.snd", 0xd883d63d)
S80_ROMEND
CORE_GAMEDEFNV(sorbit,"Super Orbit",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Royal Flush Deluxe
/-------------------------------------------------------------------*/
INITGAME(rflshdlx,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(rflshdlx, "681-2.cpu",  0x0b048658,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(     "681-s1.snd", 0x33455bbd,
                         "681-s2.snd", 0x639c93f9)
S80_ROMEND
CORE_GAMEDEFNV(rflshdlx,"Royal Flush Deluxe",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Amazon Hunt
/-------------------------------------------------------------------*/
INITGAME(amazonh,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(amazonh, "684-2.cpu",  0xb0d0c4af,
                        "u2_80a.bin", 0x241de1d4,
                        "u3_80a.bin", 0x2d77ccdc)
S80SOUND22_ROMSTART(    "684-s1.snd", 0x86d239df,
                        "684-s2.snd", 0x4d8ea26c)
S80_ROMEND
CORE_GAMEDEFNV(amazonh,"Amazon Hunt",1983,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Rack 'Em Up
/-------------------------------------------------------------------*/
INITGAME(rackemup,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(rackemup, "685.cpu",    0x4754d68d,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "685-s.snd",  0xd4219987,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(rackemup,"Rack 'Em Up",1983,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Ready...Aim...Fire!
/-------------------------------------------------------------------*/
INITGAME(raimfire,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(raimfire, "686.cpu",    0xd1e7a0de,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "686-s.snd",  0x09740682,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(raimfire,"Ready...Aim...Fire!",1983,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Jacks To Open
/-------------------------------------------------------------------*/
INITGAME(jack2opn,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(jack2opn, "687.cpu",    0x0080565e,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "687-s.snd",  0xf9d10b7a,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(jack2opn,"Jacks to Open",1984,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Alien Star
/-------------------------------------------------------------------*/
INITGAME(alienstr,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(alienstr, "689.cpu",    0x4262006b,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "689-s.snd",  0xe1e7a610,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(alienstr,"Alien Star",1984,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Games
/-------------------------------------------------------------------*/
INITGAME(thegames,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(thegames, "691.cpu",    0x50f620ea,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "691-s.snd",  0xd7011a31,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(thegames,"The Games",1984,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Touchdown
/-------------------------------------------------------------------*/
INITGAME(touchdn,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(touchdn, "688.cpu",    0xe531ab3f,
                        "u2_80a.bin", 0x241de1d4,
                        "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(    "688-s.snd",  0x5e9988a6,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(touchdn,"Touchdown",1984,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ El Dorado City of Gold
/-------------------------------------------------------------------*/
INITGAME(eldorado,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(eldorado, "692-2.cpu",  0x4ee6d09b,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "692-s.snd",  0xd5a10e53,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(eldorado,"El Dorado City of Gold",1984,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Ice Fever
/-------------------------------------------------------------------*/
INITGAME(icefever,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(icefever, "695.cpu",    0x2f6e9caf,
                         "u2_80a.bin", 0x241de1d4,
                         "u3_80a.bin", 0x2d77ccdc)
S80SOUND2K_ROMSTART(     "695-s.snd",  0xdaededc2,
                       "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(icefever,"Ice Fever",1985,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

// System 80b

/*-------------------------------------------------------------------
/ Chicago Cubs' Triple Play
/-------------------------------------------------------------------*/
INITGAME(triplay,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(triplay, "prom2.cpu", 0xdeedea61,
                          "prom1.cpu", 0x42b29b01)
S80_ROMEND
CORE_GAMEDEFNV(triplay, "Triple Play",1985,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Bounty Hunter
/-------------------------------------------------------------------*/
INITGAME(bountyh,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(bountyh, "prom2.cpu", 0xa0383e41,
                          "prom1.cpu", 0xe8190df7)
S80_ROMEND
CORE_GAMEDEFNV(bountyh, "Bounty Hunter",1985,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Rock
/-------------------------------------------------------------------*/
INITGAME(rock,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_8K_ROMSTART(rock, "prom1.cpu", 0x1146c1d3)
S80_ROMEND
CORE_GAMEDEFNV(rock, "Rock",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Raven
/-------------------------------------------------------------------*/
INITGAME(raven,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(raven, "prom2.cpu", 0x481f3fb8,
                        "prom1.cpu", 0xedc88561)
S80_ROMEND
CORE_GAMEDEFNV(raven, "Raven",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Hollywood Heat
/-------------------------------------------------------------------*/
INITGAME(hlywoodh,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(hlywoodh, "prom2.cpu", 0xa465e5f3,
                           "prom1.cpu", 0x0493e27a)
S80_ROMEND
CORE_GAMEDEFNV(hlywoodh, "Hollywood Heat",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Genesis
/-------------------------------------------------------------------*/
INITGAME(genesis,GEN_S80B2K,gottlieb_dispAlpha, 1)
// GAME_ROMS_NOT_AVAILABLE
S80B_2K_ROMSTART(genesis, "prom2.cpu", 0x00000000,
                          "prom1.cpu", 0x00000000)
S80_ROMEND
CORE_GAMEDEFNV(genesis, "Genesis",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Gold Wings
/-------------------------------------------------------------------*/
INITGAME(goldwing,GEN_S80B2K,gottlieb_dispAlpha, 1)
// GAME_ROMS_NOT_AVAILABLE
S80B_2K_ROMSTART(goldwing, "prom2.cpu", 0x00000000,
                           "prom1.cpu", 0xbf242185)
S80_ROMEND
CORE_GAMEDEFNV(goldwing, "Gold Wings",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Monte Carlo
/-------------------------------------------------------------------*/
INITGAME(mntecrlo,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(mntecrlo, "prom2.cpu", 0x6860e315,
                           "prom1.cpu", 0x0fbf15a3)
S80_ROMEND
CORE_GAMEDEFNV(mntecrlo, "Monte Carlo",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Spring Break
/-------------------------------------------------------------------*/
INITGAME(sprbreak,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(sprbreak, "prom2.cpu", 0x47171062,
                           "prom1.cpu", 0x53ed608b)
S80_ROMEND
CORE_GAMEDEFNV(sprbreak, "Spring Break",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Arena
/-------------------------------------------------------------------*/
INITGAME(arena,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(arena, "prom2.cpu", 0x4783b689,
                        "prom1.cpu", 0x8c9f8ee9)
S80_ROMEND
CORE_GAMEDEFNV(arena, "Arena",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Victory
/-------------------------------------------------------------------*/
INITGAME(victory,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(victory, "prom2.cpu", 0x6a42eaf4,
                          "prom1.cpu", 0xe724db90)
S80_ROMEND
CORE_GAMEDEFNV(victory, "Victory",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Diamond Lady
/-------------------------------------------------------------------*/
INITGAME(diamond,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(diamond, "prom2.cpu", 0x862951dc,
                          "prom1.cpu", 0x7a011757)
S80_ROMEND
CORE_GAMEDEFNV(diamond, "Diamond Lady",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ TX-Sector 
/-------------------------------------------------------------------*/
INITGAME(txsector,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(txsector, "prom2.cpu", 0xf12514e6,
                           "prom1.cpu", 0xe51d39da)
S80_ROMEND
CORE_GAMEDEFNV(txsector, "TX-Sector",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Robo-War
/-------------------------------------------------------------------*/
INITGAME(robowars,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(robowars, "prom2.cpu", 0x893177ed,
                           "prom1.cpu", 0xcd1587d8)
S80_ROMEND
CORE_GAMEDEFNV(robowars, "Robo-War",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Excalibur
/-------------------------------------------------------------------*/
S80_INPUT_PORTS_START(excalibr, 3) S80_INPUT_PORTS_END \
  static core_tGameData excalibrGameData = { \
    GEN_S80B4K, gottlieb_dispAlpha, { 0 }, NULL, { "", \
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */ \
    { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, \
    { 0 }, \
  } \
};
static void init_excalibr(void) {
  core_gameData = &excalibrGameData;
}
S80B_4K_ROMSTART(excalibr, "prom2.cpu", 0x499e2e41,
                           "prom1.cpu", 0xed1083d7)
S80_ROMEND
CORE_GAMEDEFNV(excalibr, "Excalibur",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Bad Girls
/-------------------------------------------------------------------*/
INITGAME(badgirls,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(badgirls, "prom2.cpu", 0x583933ec,
                           "prom1.cpu", 0x956aeae0)
S80_ROMEND
CORE_GAMEDEFNV(badgirls, "Bad Girls",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Big House
/-------------------------------------------------------------------*/
INITGAME(bighouse,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(bighouse, "prom2.cpu", 0x047c8ef5,
                           "prom1.cpu", 0x0ecef900)
S80_ROMEND
CORE_GAMEDEFNV(bighouse, "Big House",1989,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Bone Busters
/-------------------------------------------------------------------*/
INITGAME(bonebstr,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(bonebstr, "prom2.cpu", 0x681643df,
                           "prom1.cpu", 0x052f97be)
S80_ROMEND
CORE_GAMEDEFNV(bonebstr, "Bone Busters",1989,"Gottlieb",gl_mS80B,GAME_NO_SOUND)
