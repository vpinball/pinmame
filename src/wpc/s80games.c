#include "driver.h"
#include "sim.h"
#include "s80.h"
#include "s80sound0.h"
#include "s80sound1.h"

//	static core_tGameData name##GameData = {gen, disptype, {FLIP_SWNO(S80_SWNO(8), S80_SWNO(18))}};

#define INITGAME(name, gen, disptype, balls) \
	static core_tGameData name##GameData = {gen, disptype}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
S80_INPUT_PORTS_START(name, balls) S80_INPUT_PORTS_END

#define DISP_SEG_6(row,col,type) {4*row,20*col,row*16+col*8+2,6,type}

/* 4 X 7 (6) AlphaNumeric Rows, 1 X 4 Numeric */
core_tLCDLayout gottlieb_dispNumeric1[] = {
  DISP_SEG_6(0,0, CORE_SEG16), DISP_SEG_6(0,1, CORE_SEG16),
  DISP_SEG_6(1,0, CORE_SEG16), DISP_SEG_6(1,1, CORE_SEG16),
  DISP_SEG_CREDIT(16,24,CORE_SEG16),DISP_SEG_BALLS(0,8,CORE_SEG16),
  {0}
};

/* 4 X 6 AlphaNumeric Rows, 1 X 4 AlphaNumeric, 1 X 6  AlphaNumeric */
core_tLCDLayout gottlieb_dispNumeric2[] = {
  DISP_SEG_6(0,0, CORE_SEG16), DISP_SEG_6(0,1, CORE_SEG16),
  DISP_SEG_6(1,0, CORE_SEG16), DISP_SEG_6(1,1, CORE_SEG16),
  DISP_SEG_CREDIT(16,24,CORE_SEG16),DISP_SEG_BALLS(0,8,CORE_SEG16),
  {7, 10, 34, 6, CORE_SEG16},
  {0}
};

/* 4 X 7 AlphaNumeric Rows, 1 X 4 AlphaNumeric, 2 X 6  AlphaNumeric */
core_tLCDLayout gottlieb_dispNumeric3[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG16), DISP_SEG_7(1,1, CORE_SEG16),
  DISP_SEG_CREDIT(16,24,CORE_SEG16),DISP_SEG_BALLS(0,8,CORE_SEG16),
  {7, 0, 34, 6, CORE_SEG16}, {7, 18, 42, 6, CORE_SEG16},
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

/*-------------------------------------------------------------------
/ Spiderman
/-------------------------------------------------------------------*/
INITGAME(spidermn,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(spidermn, "653-1.cpu",0x674ddc58,
						 "653-2.cpu",0xff1ddfd7,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("653.snd", 0xf5650c46, 
			        "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(spidermn,"Spiderman",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Panthera
/-------------------------------------------------------------------*/
INITGAME(panthera,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(panthera, "652.cpu",0x5386e5fb,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("652.snd", 0x4d0cf2c0, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(panthera,"Panthera",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Circus
/-------------------------------------------------------------------*/
INITGAME(circus,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(circus, "654-1.cpu",0x0eeb2731,
					   "654-2.cpu",0x01e23569,
					   "u2_80.bin",0x4f0bc7b1,
					   "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("654.snd", 0x00000000, 
					"6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(circus,"Circus",1980,"Gottlieb",gl_mS80,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Counterforce
/-------------------------------------------------------------------*/
INITGAME(cntforce,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(cntforce, "656-1.cpu",0x42baf51d,
					     "656-2.cpu",0x0e185c30,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("656.snd", 0x0be2cbe9, 
				    "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(cntforce,"Counterforce",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Star Race
/-------------------------------------------------------------------*/
INITGAME(starrace,GEN_S80,gottlieb_dispNumeric1, 1)
S80_2_ROMSTART(starrace, "657-1.cpu",0x27081372,
					     "657-2.cpu",0xc56e31c8,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("657.snd", 0x3a1d3995, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(starrace,"Star Race",1980,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ James Bond
/-------------------------------------------------------------------*/
INITGAME(jamesb,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(jamesb, "658-1.cpu",0xb841ad7a,
					   "u2_80.bin",0x4f0bc7b1,
					   "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("658.snd", 0x00000000, 
				    "6530sy80.bin", 0xc8ba951d)
S80_ROMEND
CORE_GAMEDEFNV(jamesb,"James Bond",1980,"Gottlieb",gl_mS80,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Time Line
/-------------------------------------------------------------------*/
INITGAME(timeline,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(timeline, "659.cpu",0x0d6950e3b,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("659.snd", 0x00000000, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(timeline,"Time Line",1980,"Gottlieb",gl_mS80,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Force II
/-------------------------------------------------------------------*/
INITGAME(forceii,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(forceii, "661-1.cpu",0x00000000,
					    "u2_80.bin",0x4f0bc7b1,
					    "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("661.snd", 0x650158a7, 
				    "6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(forceii,"Force II",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Pink Panther
/-------------------------------------------------------------------*/
INITGAME(pnkpnthr,GEN_S80,gottlieb_dispNumeric1, 3)
S80_1_ROMSTART(pnkpnthr, "664-1.cpu",0xa0d3e69a,
					     "u2_80.bin",0x4f0bc7b1,
					     "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("664.snd", 0x18f4abfd, 
				    "6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(pnkpnthr,"Pink Panther",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Mars - God of War
/-------------------------------------------------------------------*/
INITGAME(mars,GEN_S80SS,gottlieb_dispNumeric1, 3)
S80_1_ROMSTART(mars, "666-1.cpu",0xbb7d476a,
				     "u2_80.bin",0x4f0bc7b1,
				     "u3_80.bin",0x1e69f9d0) 
S80SOUND22_ROMSTART("666-s1.snd", 0xd33dc8a5,
				    "666-s2.snd", 0xe5616f3e)
S80_ROMEND
CORE_GAMEDEFNV(mars,"Mars - God of War",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Volcano (Sound and Speech)
/-------------------------------------------------------------------*/
INITGAME(vlcno_ax,GEN_S80SS,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(vlcno_ax, "667-a-x.cpu",0x1f51c351,
						 "u2_80.bin",0x4f0bc7b1,
						 "u3_80.bin",0x1e69f9d0) 
S80SOUND22_ROMSTART("667-s1.snd", 0xba9d40b7,
					"667-s2.snd", 0xb54bd123) 
S80_ROMEND
CORE_GAMEDEFNV(vlcno_ax,"Volcano",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Volcano (Sound Only) 
/-------------------------------------------------------------------*/
INITGAME(vlcno_1b,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(vlcno_1b,	"667-1b.cpu" ,0xa422d862,
							"u2_80.bin",0x4f0bc7b1,
							"u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("667-a-s.snd",    0x894b4e2e, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(vlcno_1b,"Volcano (Sound Only)",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Black Hole
/-------------------------------------------------------------------*/
INITGAME(blckhole,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(blckhole, "668-2.cpu",0xdf03ffea,
						 "u2_80.bin",0x4f0bc7b1,
						 "u3_80.bin",0x1e69f9d0) 
S80SOUND1K_ROMSTART("668.snd", 0x5175f307, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(blckhole,"Black Hole",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Haunted House
/-------------------------------------------------------------------*/
INITGAME(hh,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(hh, "669.cpu",0x96e72b93,
				   "u2_80.bin",0x4f0bc7b1,
				   "u3_80.bin",0x1e69f9d0) 
S80SOUND22_ROMSTART("669-s1.snd", 0x52ec7335,
					"669-s2.snd", 0xa3317b4b)
S80_ROMEND
CORE_GAMEDEFNV(hh,"Haunted House",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Haunted House, since serial no. 5000
/-------------------------------------------------------------------*/
INITGAME(hh_2,GEN_S80SS,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(hh_2, "669-2.cpu",0xf3085f77,
				     "u2_80.bin",0x4f0bc7b1,
				     "u3_80.bin",0x1e69f9d0) 
S80SOUND22_ROMSTART("669-s1.snd", 0x52ec7335,
					"669-s2.snd", 0xa3317b4b)
S80_ROMEND
CORE_GAMEDEF(hh,2,"Haunted House (V2)",1982,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Eclipse
/-------------------------------------------------------------------*/
INITGAME(eclipse,GEN_S80,gottlieb_dispNumeric2, 1)
S80_1_ROMSTART(eclipse, "671-a.cpu",0xefad7312,
				        "u2_80.bin",0x4f0bc7b1,
				        "u3_80.bin",0x1e69f9d0) 
SOUND_ROMS_NOT_AVAILABLE
S80SOUND1K_ROMSTART("671-a-s.snd", 0x5175f307, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(eclipse,"Eclipse",1981,"Gottlieb",gl_mS80,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ Devil's Dare
/-------------------------------------------------------------------*/
INITGAME(dvlsdre,GEN_S80SS,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(dvlsdre, "670.cpu", 0x6318bce2,
			            "u2_80a.bin", 0x241de1d4,
				        "u3_80a.bin", 0x2d77ccdc) 
S80SOUND22_ROMSTART(	"670-s1.snd", 0x506bc22a,
						"670-s2.snd", 0xf662ee4b)
S80_ROMEND
CORE_GAMEDEFNV(dvlsdre,"Devil's Dare",1981,"Gottlieb",gl_mS80SS,GAME_IMPERFECT_SOUND)

/*-------------------------------------------------------------------
/ The Games
/-------------------------------------------------------------------*/
INITGAME(thegames,GEN_S80,gottlieb_dispNumeric1, 1)
S80_1_ROMSTART(thegames, "691.cpu", 0x50f620ea,
			             "u2_80a.bin", 0x241de1d4,
				         "u3_80a.bin", 0x2d77ccdc) 
S80SOUND1K_ROMSTART("691.snd", 0xd7011a31, 
					"6530sy80.bin", 0xc8ba951d) 
S80_ROMEND
CORE_GAMEDEFNV(thegames,"The Games",1984,"Gottlieb",gl_mS80,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Caveman
/-------------------------------------------------------------------*/
/* forget it at the moment */

/*-------------------------------------------------------------------
/ Spirit
/-------------------------------------------------------------------*/
INITGAME(spirit,GEN_S80,gottlieb_dispNumeric3, 1)
S80_1_ROMSTART(spirit, "673-2.cpu",0xa7dc2207,
		               "u2_80a.bin",0x241de1d4,
				       "u3_80a.bin",0x2d77ccdc) 
SOUND_ROMS_NOT_AVAILABLE
S80SOUND22_ROMSTART("673-s1.snd", 0x00000000,
					"673-s2.snd", 0x00000000) 
S80_ROMEND
CORE_GAMEDEFNV(spirit, "Spirit",1982,"Gottlieb",gl_mS80,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Raven
/-------------------------------------------------------------------*/
INITGAME(raven,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(raven, "prom2.cpu",0x481f3fb8,
 				        "prom1.cpu",0xedc88561)
S80_ROMEND
CORE_GAMEDEFNV(raven, "Raven",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Hollywood Heat
/-------------------------------------------------------------------*/
INITGAME(hlywoodh,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(hlywoodh, "prom2.cpu",0xa465e5f3,
 				           "prom1.cpu",0x0493e27a)
S80_ROMEND
CORE_GAMEDEFNV(hlywoodh, "Hollywood Heat",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Genesis
/-------------------------------------------------------------------*/
INITGAME(genesis,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(genesis, "prom2.cpu",0x06a42eaf4,
				          "prom1.cpu",0x0e724db90)
S80_ROMEND
CORE_GAMEDEFNV(genesis, "Genesis",1986,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Monte Carlo
/-------------------------------------------------------------------*/
INITGAME(mntecrlo,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(mntecrlo, "prom2.cpu",0x6860e315,
 				           "prom1.cpu",0x0fbf15a3)
S80_ROMEND
CORE_GAMEDEFNV(mntecrlo, "Monte Carlo",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Victory
/-------------------------------------------------------------------*/
INITGAME(victory,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(victory, "prom2.cpu",0x6a42eaf4,
				          "prom1.cpu",0xe724db90)
S80_ROMEND
CORE_GAMEDEFNV(victory, "Victory",1987,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ TX-Sector 
/-------------------------------------------------------------------*/
INITGAME(txsector,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(txsector, "prom2.cpu",0xf12514e6,
 				           "prom1.cpu",0xe51d39da)
S80_ROMEND
CORE_GAMEDEFNV(txsector, "TX-Sector",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Robo-War
/-------------------------------------------------------------------*/
INITGAME(robowars,GEN_S80B2K,gottlieb_dispAlpha, 1)
S80B_2K_ROMSTART(robowars, "prom2.cpu",0x893177ed,
 				           "prom1.cpu",0xcd1587d8)
S80_ROMEND
CORE_GAMEDEFNV(robowars, "Robo-War",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Bad Girls
/-------------------------------------------------------------------*/
INITGAME(badgirls,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(badgirls, "prom2.cpu",0x583933ec,
				           "prom1.cpu",0x956aeae0)
S80_ROMEND
CORE_GAMEDEFNV(badgirls, "Bad Girls",1988,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Big House
/-------------------------------------------------------------------*/
INITGAME(bighouse,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(bighouse, "prom2.cpu",0x047c8ef5,
 				           "prom1.cpu",0x0ecef900)
S80_ROMEND
CORE_GAMEDEFNV(bighouse, "Big House",1989,"Gottlieb",gl_mS80B,GAME_NO_SOUND)

/*-------------------------------------------------------------------
/ Bone Busters
/-------------------------------------------------------------------*/
INITGAME(bonebstr,GEN_S80B4K,gottlieb_dispAlpha, 1)
S80B_4K_ROMSTART(bonebstr, "prom2.cpu",0x681643df,
 				           "prom1.cpu",0x052f97be)
S80_ROMEND
CORE_GAMEDEFNV(bonebstr, "Bone Busters",1989,"Gottlieb",gl_mS80B,GAME_NO_SOUND)


