#include "driver.h"
#include "sim.h"
#include "de.h"
#include "de2.h"
#include "de1sound.h"
#include "de2sound.h"

#define INITGAME(name, gen, disptype, flippers, balls) \
	static core_tGameData name##GameData = { gen, disptype, {flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
DE_INPUT_PORTS_START(name, balls) DE_INPUT_PORTS_END

//Used for games after Frankenstein
#define INITGAME2(name, gen, disptype, flippers, balls) \
	static core_tGameData name##GameData = { gen, disptype, {flippers}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
} \
DE2_INPUT_PORTS_START(name, balls) DE_INPUT_PORTS_END


/*Common Flipper Switch Settings*/
#define FLIP4746    FLIP_SWNO(DE_SWNO(47),DE_SWNO(46))
#define FLIP3031	FLIP_SWNO(DE_SWNO(30),DE_SWNO(31))
#define FLIP1516	FLIP_SWNO(DE_SWNO(15),DE_SWNO(16))
#define FLIP6364	FLIP_SWNO(DE_SWNO(63),DE_SWNO(64))

/* NO OUTPUT */
core_tLCDLayout de_NoOutput[] = {{0}};

/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows, 1 X 4 Numeric*/
core_tLCDLayout de_dispAlpha1[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7),  DISP_SEG_7(1,1, CORE_SEG7),
  DISP_SEG_CREDIT(20,28,CORE_SEG7),DISP_SEG_BALLS(0,8,CORE_SEG7H),
  {0}
};

/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows */
core_tLCDLayout de_dispAlpha2[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7),  DISP_SEG_7(1,1, CORE_SEG7),
  {0}
};

/* 2 X 16 AlphaNumeric Rows */
core_tLCDLayout de_dispAlpha3[] = {
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

/* 128x16 DMD OUTPUT */
core_tLCDLayout de_128x16DMD[] = {
	{0,0,16,128,CORE_DMD}, {0}
};

/* 192x64 DMD OUTPUT */
core_tLCDLayout de_192x64DMD[] = {
	{0,0,64,192,CORE_DMD}, {0}
};


/***************************************************/
/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */
/***************************************************/

/*-------------------------------------------------------------------
/ Laser War - CPU Rev 1 /Alpha Type 1 - 32K ROM - 32/64K Sound Roms
/-------------------------------------------------------------------*/
INITGAME(lwar,DE_CPUREV1 | DE_ALPHA1, de_dispAlpha1, FLIP4746, 3/*?*/)
DE32_ROMSTART(lwar,		"lwar.c5",0xeee158ee)
DESOUND3264_ROMSTART(	"lwar_e9.snd",0x9a6c834d,	//F7 on schem (sound)
						"lwar_e6.snd",0x7307d795,	//F6 on schem (voice1)
						"lwar_e7.snd",0x0285cff9)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(lwar,"Laser War",1987,"Data East",de_mDE_AS,0)


/*-------------------------------------------------------------------------
/ Secret Service - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32K/64K Sound Roms
/-------------------------------------------------------------------------*/
INITGAME(ssvc,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP3031, 3/*?*/)
DE3232_ROMSTART(ssvc,	"ssvc4-6.b5",0xe5eab8cd,
						"ssvc4-6.c5",0x171b97ae)
DESOUND3264_ROMSTART(	"sssndf7.rom",0x980778d0,	//F7 on schem (sound)
						"ssv1f6.rom",0xccbc72f8,	//F6 on schem (voice1)
						"ssv2f4.rom",0x53832d16)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(ssvc,"Secret Service",1988,"Data East",de_mDE_AS,0)

/*-----------------------------------------------------------------------
/ Torpedo Alley - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/------------------------------------------------------------------------*/
INITGAME(torpe,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE3232_ROMSTART(torpe,	"torpe2-1.b5",0xac0b03e3,
						"torpe2-1.c5",0x9ad33882)
DESOUND3264_ROMSTART(	"torpef7.rom",0x26f4c33e,	//F7 on schem (sound)
						"torpef6.rom",0xb214a7ea,	//F6 on schem (voice1)
						"torpef4.rom",0x83a4e7f3)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(torpe,"Torpedo Alley",1988,"Data East",de_mDE_AS,0)

/*--------------------------------------------------------------------------
/ Time Machine - CPU Rev 2 /Alpha Type 2 16/32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------*/
INITGAME(tmach,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE1632_ROMSTART(tmach,	"tmach2-4.b5",0x6ef3cf07,
                        "tmach2-4.c5",0xb61035f5)
DESOUND3264_ROMSTART(	"tmachf7.rom",0x5d4994bb,	//F7 on schem (sound)
						"tmachf6.rom",0xc04b07ad,	//F6 on schem (voice1)
						"tmachf4.rom",0x70f70888)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(tmach,"Time Machine",1988,"Data East",de_mDE_AS,0)


/*-----------------------------------------------------------------------------------
/ Playboy 35th Anniversary - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------------------------*/
INITGAME(play,DE_CPUREV2 | DE_ALPHA2, de_dispAlpha2, FLIP1516, 3/*?*/)
DE3232_ROMSTART(play,	"play2-4.b5",0xbc8d7b32,
						"play2-4.c5",0x47c30bc2)
DESOUND3264_ROMSTART(	"pbsnd7.dat",0xc2cf2cc5,	//F7 on schem (sound)
						"pbsnd6.dat",0xc2570631,	//F6 on schem (voice1)
						"pbsnd5.dat",0x0fd30569)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(play,"Playboy 35th Anniversary",1989,"Data East",de_mDE_AS,0)

/*-----------------------------------------------------------------------------------
/ Monday Night Football - CPU Rev 2 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/----------------------------------------------------------------------------------*/
INITGAME(mnfb,DE_CPUREV2 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(mnfb,	"mnfb2-7.b5",0x995eb9b8,
						"mnfb2-7.c5",0x579d81df)
DESOUND3264_ROMSTART(	"mnf-f7.256",0xfbc2d6f6,	//F7 on schem (sound)
						"mnf-f5-6.512",0x0c6ea963,	//F6 on schem (voice1)
						"mnf-f4-5.512",0xefca5d80)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(mnfb,"Monday Night Football",1989,"Data East",de_mDE_AS,0)
/*------------------------------------------------------------------
/ Robocop - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------*/
INITGAME(robo,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE3232_ROMSTART(robo,	"robob5.a34",0x5a611004,
						"roboc5.a34",0xc8705f47)
DESOUND3264_ROMSTART(	"robof7.rom",0xfa0891bd,	//F7 on schem (sound)
						"robof6.rom",0x9246e107,	//F6 on schem (voice1)
						"robof4.rom",0x27d31df3)	//F4 on schem (voice2)
DE_ROMEND
CORE_GAMEDEFNV(robo,"Robocop",1989,"Data East",de_mDE_AS,0)

/*-------------------------------------------------------------------------------
/ Phantom of the Opera - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/-------------------------------------------------------------------------------*/
INITGAME(poto,DE_CPUREV2 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(poto,	"potob5.3-2",0xbdc39205,
						"potoc5.3-2",0xe6026455)
DESOUND3264_ROMSTART(	"potof7.rom",0x2e60b2e3,	//7f
						"potof6.rom",0x62b8f74b,	//6f
						"potof5.rom",0x5a0537a8)	//4f
DE_ROMEND
CORE_GAMEDEFNV(poto,"The Phantom of the Opera",1990,"Data East",de_mDE_AS,0)

/*--------------------------------------------------------------------------------
/ Back To the Future - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------------*/
INITGAME(bttf,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE3232_ROMSTART(bttf,	"bttfb5.2-0",0xc0d4df6b,
						"bttfc5.2-0",0xa189a189)
DESOUND3264_ROMSTART(	"bttfsf7.rom",0x7673146e,	//7f
						"bttfsf6.rom",0x468a8d9c,	//6f
						"bttfsf5.rom",0x37a6f6b8)	//4f
DE_ROMEND
CORE_GAMEDEFNV(bttf,"Back To the Future",1990,"Data East",de_mDE_AS,0)

/*------------------------------------------------------------------------
/ The Simpsons - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/128K Sound Roms
/------------------------------------------------------------------------*/
INITGAME(simp,DE_CPUREV3 | DE_ALPHA3, de_dispAlpha3, FLIP1516, 3/*?*/)
DE1632_ROMSTART(simp,	"simpb5.2-7",0x701c4a4b,
						"simpc5.2-7",0x400a98b2)
DESOUND32128_ROMSTART(	"simpf7.rom",0xa36febbc,	//7f
						"simpf6.rom",0x2eb32ed0,	//6f
						"simpf5.rom",0xbd0671ae)	//4f
DE_ROMEND
CORE_GAMEDEFNV(simp,"The Simpsons",1990,"Data East",de_mDE_AS,0)


/***********************************************************************/
/*************** GAMES USING 128X16 DMD DISPLAY ************************/
/***********************************************************************/

/*------------------------------------------------------------
/ Checkpoint - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAME(chkpnt,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE1632_ROMSTART(chkpnt,	"chkpntb5.107",0x9fbae8e3,
						"chkpntc5.107",0x082dc283)
DE_DMD64_ROMSTART(		"chkpntds.512",0x14d9c6d6)
//DE_DMD64_ROMSTART(		"chkpntds.80",0x14d9c6d6)	&& Doesn't work in mame for some reason. Rename to .512!
DESOUND32128D_ROMSTART(	"chkpntf7.rom",0xe6f6d716,	//7f
						"chkpntf6.rom",0x2d08043e,	//6f
						"chkpntf5.rom",0x167daa2c)	//4f

DE_ROMEND
CORE_GAMEDEFNV(chkpnt,"Checkpoint",1991,"Data East",de_mDE_DMD1S1,0)


/*-----------------------------------------------------------------------------
/ Teenage Mutant Ninja Turtles - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/-----------------------------------------------------------------------------*/
INITGAME(tmnt,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE1632_ROMSTART(tmnt,	"tmntb5a.104",0xf508eeee,
						"tmntc5a.104",0xa33d18d4)
DE_DMD64_ROMSTART(		"tmntdsp.104",0x545686b7)
DESOUND32128D_ROMSTART(	"tmntf7.rom",0x59ba0153,	//7f
						"tmntf6.rom",0x5668d45a,	//6f
						"tmntf4.rom",0xf96b0539)	//4f

DE_ROMEND
CORE_GAMEDEFNV(tmnt,"Teenage Mutant Ninja Turtles",1991,"Data East",de_mDE_DMD1S1,0)


/***************************************************************************/
/** ALL FOLLOWING GAMES BELOW STARTED USING NEW SOUND BOARD WITH BSMT2000 **/
/***************************************************************************/

/*-------------------------------------------------------------
/ Batman - CPU Rev 3 /DMD Type 1 128K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAME(batmn,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE3232_ROMSTART(batmn,	"batcpub5.101",0xa7f5754e,
						"batcpuc5.101",0x1fcb85ca)
DE_DMD128_ROMSTART(		"batdsp.106",0x4c4120e7)
DES_SOUNDROM021(		"batman.u7" ,0xb2e88bf5,
						"batman.u17" ,0xb84914dd,
						"batman.u21" ,0x42dab6ac)
DE_ROMEND
CORE_GAMEDEFNV(batmn,"Batman",1992,"Data East",de_mDE_DMD1S2,0)

/*-------------------------------------------------------------
/ Star Trek - CPU Rev 3 /DMD Type 1 128K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(trek,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE64_ROMSTART(trek,		"trekcpuu.201",0xea0681fe)
DE_DMD128_ROMSTART(		"trekdspa.109",0xa7e7d44d)
DES_SOUNDROM022(		"trek.u7"  ,0xf137abbb,
						"trek.u17" ,0x531545da,
						"trek.u21" ,0x6107b004)
DE_ROMEND
CORE_GAMEDEFNV(trek,"Star Trek 25th Anniversary",1992,"Data East",de_mDE_DMD1S2,0)

/*-------------------------------------------------------------
/ Hook - CPU Rev 3 /DMD  Type 1 128K Rom - CPU Rom
/------------------------------------------------------------*/
INITGAME(hook,DE_CPUREV3, de_128x16DMD, FLIP1516, 3/*?*/)
DE64_ROMSTART(hook,		"hokcpua.408",0x46477fc7)
DE_DMD128_ROMSTART(		"hokdspa.401",0x59a07eb5)
DES_SOUNDROM022(		"hooksnd.u7" ,0x642f45b3,
						"hook-voi.u17" ,0x6ea9fcd2,
						"hook-voi.u21" ,0xb5c275e2)
DE_ROMEND
CORE_GAMEDEFNV(hook,"Hook",1992,"Data East",de_mDE_DMD1S2,0)


/***********************************************************************/
/*************** GAMES USING 128X32 DMD DISPLAY ************************/
/***********************************************************************/

/*----------------------------------------------------------------
/ Lethal Weapon 3 - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/---------------------------------------------------------------*/
INITGAME(lw3,DE_CPUREV3, 0, FLIP1516, 3)
DE64_ROMSTART(lw3,		"lw3cpuu.208",0xa3041f8a)
DE_DMD256_ROMSTART(		"lw3drom1.a26",0x44a4cf81,
						"lw3drom0.a26",0x22932ed5)
DES_SOUNDROM022(		"lw3u7.dat"  ,0xba845ac3,
						"lw3u17.dat" ,0xe34cf2fc,
						"lw3u21.dat" ,0x82bed051)
DE_ROMEND
CORE_GAMEDEFNV(lw3,"Lethal Weapon 3",1992,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Star Wars - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(stwarde,DE_CPUREV3, 0, FLIP1516, 3)
DE64_ROMSTART(stwarde,	"starcpua.103",0x318085ca)
DE_DMD512_ROMSTART(		"sw4mrom.a15",0x00c87952)
DES_SOUNDROM042(		"s-wars.u7"  ,0xcefa19d5,
						"s-wars.u17" ,0x7950a147,
						"s-wars.u21" ,0x7b08fdf1)
DE_ROMEND
CORE_GAMEDEFNV(stwarde,"Star Wars",1992,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Rocky & Bullwinkle - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(rab,DE_CPUREV3b, 0, FLIP1516, 3)
DE64_ROMSTART(rab,	"rabcpua.130",0xf59b1a53)
DE_DMD512_ROMSTART(	"rbdspa.130",0xb6e2176e)
DES_SOUNDROM142(	"rab.u7"  ,0xb232e630,
					"rab.u17" ,0x7f2b53b8,
					"rab.u21" ,0x3de1b375)
DE_ROMEND
CORE_GAMEDEFNV(rab,"Rocky & Bullwinkle",1993,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Jurassic Park - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(jurpark,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(jurpark,"jpcpua.513",0x9f70a937)
DE_DMD512_ROMSTART(	  "jpdspa.510",0x9ca61e3c)
DES_SOUNDROM142(	"jpu7.dat"  ,0xf3afcf13,
					"jpu17.dat" ,0x38135a23,
					"jpu21.dat" ,0x6ac1554c)
DE_ROMEND
CORE_GAMEDEFNV(jurpark,"Jurassic Park",1993,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Last Action Hero - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(lah,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(lah,	"lahcpua.112",0xe7422236)
DE_DMD512_ROMSTART(	"lahdispa.106",0xca6cfec5)
DES_SOUNDROM142(	"lahsnd.u7"  ,0x0279c45b,
					"lahsnd.u17" ,0xd0c15fa6,
					"lahsnd.u21" ,0x4571dc2e)
DE_ROMEND
CORE_GAMEDEFNV(lah,"Last Action Hero",1993,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Tales From the Crypt - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(tftc,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(tftc,	"tftccpua.303",0xe9bec98e)
DE_DMD512_ROMSTART(	"tftcdspa.301",0x3888d06f)
DES_SOUNDROM144(	"sndu7.dat"    ,0x7963740e,
					"sndu17.dat" ,0x5c5d009a,
					"sndu21.dat" ,0xa0ae61f7)
DE_ROMEND
CORE_GAMEDEFNV(tftc,"Tales From the Crypt",1993,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Tommy - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(tommy,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(tommy,"tomcpua.400",0xd0310a1a)
DE_DMD512_ROMSTART(	"tommydva.400",0x9e640d09)
DES_SOUNDROM14444(		"tommysnd.u7"  ,0xab0b4626,
                        "tommysnd.u17" ,0x11bb2aa7,
                        "tommysnd.u21" ,0xbb4aeec3,
                        "tommysnd.u36" ,0x208d7aeb,
                        "tommysnd.u37" ,0x46180085)
DE_ROMEND
CORE_GAMEDEFNV(tommy,"Tommy",1994,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ WWF Royal Rumble - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(wwfrumb,DE_CPUREV3b, 0, FLIP6364, 3)
DE64_ROMSTART(wwfrumb,	"wfcpuc5.512"  ,0x7e9ead89)
DE_DMD512_ROMSTART(		"wfdisp0.400"  ,0xe190b90f)
DES_SOUNDROM1444(		"wfsndu7.512"  ,0xeb01745c,
                        "wfsndu17.400" ,0x7d9c2ca8,
                        "wfsndu21.400" ,0x242dcdcb,
                        "wfsndu36.400" ,0x39db8d85)
DE_ROMEND
CORE_GAMEDEFNV(wwfrumb,"WWF Royal Rumble",1994,"Data East",de_mDE_DMD2S1,0)

/*-------------------------------------------------------------
/ Guns N Roses - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
//INITGAME(gnr,DE_CPUREV3b, 0, FLIP6364, 3)

extern int de_data;
#define sLMagnet   CORE_CUSTSOLNO(1) /* 33 */
#define sTMagnet   CORE_CUSTSOLNO(2) /* 34 */
#define sRMagnet   CORE_CUSTSOLNO(3) /* 35 */

static int  gnr_getSol(int solNo);
/*-- return status of custom solenoids --*/
static int gnr_getSol(int solNo) {
  if (solNo == sLMagnet)    return (de_data & 0x01) > 0;
  if (solNo == sTMagnet)    return (de_data & 0x02) > 0;
  if (solNo == sRMagnet)	return (de_data & 0x04) > 0;
  return 0;
}

static core_tGameData gnrGameData = {
	DE_CPUREV3b, 0, {
    FLIP6364,
    0,0,3,				//We need 3 custom solenoids!
    gnr_getSol,NULL, NULL, NULL,
    NULL,NULL
  },
  NULL,
  {{0}},
  {0}
};
static void init_gnr(void) {
  core_gameData = &gnrGameData;
}
DE_INPUT_PORTS_START(gnr, 6) DE_INPUT_PORTS_END
DE64_ROMSTART(gnr,		"gnrcpua.300",0xfaf0cc8c)
DE_DMD512_ROMSTART(		"gnrdispa.300",0x4abf29e3)
DES_SOUNDROM14444(		"gnru7.snd"  ,0x3b9de915,
                        "gnru17.snd" ,0x3d3219d6,
                        "gnru21.snd" ,0xd2ca17ab,
                        "gnru36.snd" ,0x5b32396e,
                        "gnru37.snd" ,0x4930e1f2)
DE_ROMEND
CORE_GAMEDEFNV(gnr,"Guns N Roses",1994,"Data East",de_mDE_DMD2S1,0)



/***********************************************************************/
/*************** GAMES USING 192X64 DMD DISPLAY ************************/
/***********************************************************************/

//Snd Works (Voices might be messed not sure)
/*-------------------------------------------------------------
/ Maverick - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(maverick,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(maverick,	"mavcpua.404",0x9f06bd8d)
DE_DMD1024_ROMSTART(	"mavdsar0.401",0x35b811af,
						"mavdsar3.401",0xc4c126ae)
DES_SOUNDROM144(		"mavu7.dat"  ,0x427e6ab9,
                        "mavu17.dat" ,0xcba377b8,
                        "mavu21.dat" ,0xbe0c6a6f)
DE_ROMEND
CORE_GAMEDEFNV(maverick,"Maverick",1994,"Data East",de_mDE_DMD3S1,0)

/*****************************************************************************************************************************/
/* NOTE: SEGA Began Distribution of the following games, although they run on Data East Hardware, so they stay in this file! */
/*****************************************************************************************************************************/


/*-------------------------------------------------------------
/ Frankenstein - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME(frankst,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(frankst,	"franka.103",0xa9aba9be)
DE_DMD1024_ROMSTART(	"frdspr0a.103",0x9dd09c7d,
						"frdspr3a.103",0x73b538bb)
DES_SOUNDROM1444(		"frsnd.u7"  ,0x084f856c,
                        "frsnd.u17" ,0x0da904d6,
                        "frsnd.u21" ,0x14d4bc12,
                        "frsnd.u36" ,0x9964d721)
DE_ROMEND
CORE_GAMEDEFNV(frankst,"Mary Shelley's Frankenstein",1994,"Sega",de_mDE_DMD3S1,0)


//Start of the Portals Diagnostic Menu System

/*-------------------------------------------------------------
/ Baywatch - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(baywatch,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(baywatch,	"baycpua.400",0x89facfda)
DE_DMD1024_ROMSTART(	"bayrom0a.400",0x43d615c6,
						"bayrom3a.400",0x41bcb66b)
DES_SOUNDROM144(		"bayw.u7"  ,0x90d6d8a8,
                        "bayw.u17" ,0xb20fde56,
                        "bayw.u21" ,0xb7598881)
DE_ROMEND
CORE_GAMEDEFNV(baywatch,"Baywatch",1995,"Sega",de_mDE_DMD3S1,0)


/*-------------------------------------------------------------
/ Batman Forever 4.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(batmanf,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(batmanf, "batnova.401", 0x4e62df4e)
DE_DMD1024_ROMSTART(   "bfdrom0a.401",0x8a3c20ad,
                       "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_GAMEDEFNV(batmanf,"Batman Forever (4.0)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever 3.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(batmanf3,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(batmanf3, "batcpua.302", 0x5ae7ce69)
DE_DMD1024_ROMSTART(    "bmfrom0a.300",0x764bb217,
                        "bmfrom3a.300",0xb4e3b515)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(batmanf3,batmanf,"Batman Forever (3.0)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (UK) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_uk,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_uk, "batnove.401",0x80f6e4af)
DE_DMD1024_ROMSTART(  "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_uk,batmanf,"Batman Forever (English)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (CN) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_cn,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_cn, "batnovc.401",0x99936537)
DE_DMD1024_ROMSTART(  "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_cn,batmanf,"Batman Forever (Canadian)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (NO) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_no,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_no, "batnovn.401",0x79dd48b4)
DE_DMD1024_ROMSTART(  "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_no,batmanf,"Batman Forever (Norwegian)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (SV) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_sv,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_sv, "batnovt.401",0x854029ab)
DE_DMD1024_ROMSTART(  "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_sv,batmanf,"Batman Forever (Swedish)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (AT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_at,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_at, "batnovh.401",0xacba13d7)
DE_DMD1024_ROMSTART(  "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_at,batmanf,"Batman Forever (Austrian)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (CH) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_ch,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_ch, "batnovs.401",0x4999d5f9)
DE_DMD1024_ROMSTART(  "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_ch,batmanf,"Batman Forever (Swiss)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (DE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_de,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_de, "batnovg.401",0xdd37e99a)
DE_DMD1024_ROMSTART(  "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_de,batmanf,"Batman Forever (German)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (BE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_be,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_be, "batnovb.401",0x21309873)
DE_DMD1024_ROMSTART(  "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_be,batmanf,"Batman Forever (Belgian)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (FR) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_fr,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_fr, "batnovf.401",0x4baa793d)
DE_DMD1024_ROMSTART(  "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_fr,batmanf,"Batman Forever (French)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (NL) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_nl,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_nl, "batnovd.401",0x6ae4570c)
DE_DMD1024_ROMSTART(  "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_nl,batmanf,"Batman Forever (Dutch)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (IT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_it,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_it, "batnovi.401",0x7053ef9e)
DE_DMD1024_ROMSTART(  "bfdrom0i.401",0x23051253,
                      "bfdrom3i.401",0x82b61a41)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_it,batmanf,"Batman Forever (Italian)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (SP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_sp,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_sp, "batnova.401",0x4e62df4e)
DE_DMD1024_ROMSTART(  "bfdrom0l.401",0xb22b10d9,
                      "bfdrom3l.401",0x016b8666)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_sp,batmanf,"Batman Forever (Spanish)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (JP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_jp,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_jp, "batnovj.401",0xeef9bef0)
DE_DMD1024_ROMSTART(  "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_jp,batmanf,"Batman Forever (Japanese)",1995,"Sega",de_mDE_DMD3S1,0)

/*-------------------------------------------------------------
/ Batman Forever (Timed Version) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAME2(bmf_time,DE_CPUREV3b, de_192x64DMD, FLIP6364, 3)
DE64_ROMSTART(bmf_time, "batnova.401", 0x4e62df4e)
DE_DMD1024_ROMSTART(    "bfdrom0t.401",0xb83b8d28,
                        "bfdrom3t.401",0xa024b1a5)
DES_SOUNDROM144(		"bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
CORE_CLONEDEFNV(bmf_time,batmanf,"Batman Forever (Timed Play)",1995,"Sega",de_mDE_DMD3S1,0)


/***********************************************************************/
/*************** SPECIAL TEST CHIP - NO DISPLAY ************************/
/***********************************************************************/
/*-------------------------------------------------------------
/ Data East Test Chip 64K ROM
/------------------------------------------------------------*/
INITGAME(detest,DE_CPUREV3, de_NoOutput, FLIP1516, 3/*?*/)
DE64_ROMSTART(detest,"de_test.512",0xbade8ca8)
DE_ROMEND
CORE_GAMEDEFNV(detest,"Data East Test Chip",1998,"Data East",de_mDE_NO,0)

