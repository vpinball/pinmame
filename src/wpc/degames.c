/**************************************************************************************
  Data East Pinball -
  Hardware from 1987-1994
  CPU hardware is very similar to Williams System 11 Hardware!

  CPU Boards:
	1) CPU Rev 1 : Ram size = 2k (0x0800)	(Early Laser War Only)
	2) CPU Rev 2 : Ram size = 8k (0x2000)	(Later Laser War to Phantom of the Opera)
	3) CPU Rev 3 : CPU Controlled solenoids	(Back to the Future to Jurassic Park)
	4) CPU Rev 3b: Printer option			(Last Action Hero to Batman Forever)

  Display Boards:
	1) 520-5004-00: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Numeric), 1 X 4 Digit (7 Seg. Numeric)
	   (Used in Laser War Only)

	2) 520-5014-01: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Alphanumeric)
	   (Secret Service to Playboy)
	
	3) 520-5030-00: 2 X 16 Digit (16 Seg Alphanumeric)
		(MNF to Simpsons)
	
	4) 520-5042-00: 128X16 DMD - z80 CPU + integrated controller.
	   (Checkpoint to Hook)

	5) 520-5505 Series: 128X32 DMD - m6809 CPU + separate controller board
		a) -00 generation: (Lethal Weapon to Last Action Hero)
		b) -01 generation: (Tales From the Crypt to Guns N Roses)

	6) 520-5092-01: 192X64 DMD - 68000 CPU + separate controller board
	   (Maveric to Batman Forever)

   Sound Board Revisions:
	1) 520-5002 Series: M6809 cpu, YM2151, MSM5205, hc4020 for stereo decoding.
		a) -00 generation, used 27256 eproms (only Laser War)
	    b) -02 generation, used 27256 & 27512 eproms (Laser War - Back to the Future)
		c) -03 generation, used 27010 voice eproms (Simpsons - Checkpoint)

	2) 520-5050-01 Series:	M6809 cpu, BSMT2000 16 bit stereo synth+dac, 2 custom PALS
		a) -01 generation,	used 27020 voice eproms (Batman - Lethal Weapon 3)
		b) -02 generation,	used 27040 voice eproms (Star Wars - J.Park)
		c) -03 generation,	similar to 02, no more info known (LAH - Maverick)
	3) 520-5077-00 Series:	??  (Tommy to Frankenstein)
	4) 520-5126-xx Series:	??	(Baywatch to Batman Forever)
*************************************************************************************/

/* Coin Door Buttons Operation
   ---------------------------
   Pre-"Portals" Menu (Games before Baywatch)
   Buttons are: Green(Up/Down) & Black(Momentary Switch)

   a) If Green = Up and Black is pressed, enter Audits Menu.
		1) If Green = Up and Black is pressed, Cycle to Next Audit Function
		2) If Green = Down and Black is pressed, Cycle to Previous Audit Function

   b) If Green = Down and Black is pressed, enter Diagnostics.
		1) Start button to start a test
		2) Black Button to cycle tests
		3) Flippers can operate settings within a test (such as the Speaker/Sound Test)

  Portals Menu System (Baywatch & Batman Forever)
  Buttons are: Green(Momentary) & Black(Momentary Switch)
  a) Pressing Black button brings up the Portals System
  b) Flippers move the icon left or right
  c) Start button or Black button will select an icon
  d) Green button will also move the cursor to the right.
*/

#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "desound.h"
#include "dedmd.h"
#include "s11.h"

#define INITGAMES11(name, gen, disptype, flippers, sb, db, gs1) \
static core_tGameData name##GameData = { \
  gen, disptype, {flippers,0,0,0,sb,db,gs1}, NULL, {{0}},{10}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

DE_INPUT_PORTS_START(des11, 1) DE_INPUT_PORTS_END
DE_INPUT_PORTS_START2(des112, 1) DE_INPUT_PORTS_END

/*Common Flipper Switch Settings*/
#define FLIP4746    FLIP_SWNO(47,46)
#define FLIP3031    FLIP_SWNO(30,31)
#define FLIP1516    FLIP_SWNO(15,16)
#define FLIP6364    FLIP_SWNO(63,64)


/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows, 1 X 4 Numeric*/
core_tLCDLayout de_dispAlpha1[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7),  DISP_SEG_7(1,1, CORE_SEG7),
  DISP_SEG_CREDIT(20,28,CORE_SEG7),DISP_SEG_BALLS(0,8,CORE_SEG7H), {0}
};

/* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows */
core_tLCDLayout de_dispAlpha2[] = {
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG7),  DISP_SEG_7(1,1, CORE_SEG7), {0}
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
INITGAMES11(lwar, GEN_DE, de_dispAlpha1, FLIP4746, SNDBRD_DE1S, 0, 0)
DE_ROMSTARTx8(lwar, "lwar.c5", 0xeee158ee)
DE1S_SOUNDROM244(   "lwar_e9.snd",0x9a6c834d,   //F7 on schem (sound)
                    "lwar_e6.snd",0x7307d795,   //F6 on schem (voice1)
                    "lwar_e7.snd",0x0285cff9)   //F4 on schem (voice2)
DE_ROMEND
#define input_ports_lwar input_ports_des11
CORE_GAMEDEFNV(lwar,"Laser War",1987,"Data East",de_mDEAS1,0)


/*-------------------------------------------------------------------------
/ Secret Service - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32K/64K Sound Roms
/-------------------------------------------------------------------------*/
INITGAMES11(ssvc, GEN_DE, de_dispAlpha2, FLIP3031, SNDBRD_DE1S, 0, 0)
DE_ROMSTART88(ssvc,"ssvc4-6.b5", 0xe5eab8cd,
                   "ssvc4-6.c5", 0x171b97ae)
DE1S_SOUNDROM244(  "sssndf7.rom",0x980778d0,      //F7 on schem (sound)
                   "ssv1f6.rom", 0xccbc72f8,       //F6 on schem (voice1)
                   "ssv2f4.rom", 0x53832d16)       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_ssvc input_ports_des11
CORE_GAMEDEFNV(ssvc,"Secret Service",1988,"Data East",de_mDEAS1, 0)

/*-----------------------------------------------------------------------
/ Torpedo Alley - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/------------------------------------------------------------------------*/
INITGAMES11(torpe, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, 0)
DE_ROMSTART88(torpe,"torpe2-1.b5",0xac0b03e3,
                    "torpe2-1.c5",0x9ad33882)
DE1S_SOUNDROM244(   "torpef7.rom",0x26f4c33e,      //F7 on schem (sound)
                    "torpef6.rom",0xb214a7ea,      //F6 on schem (voice1)
                    "torpef4.rom",0x83a4e7f3)      //F4 on schem (voice2)
DE_ROMEND
#define input_ports_torpe input_ports_des11
CORE_GAMEDEFNV(torpe,"Torpedo Alley",1988,"Data East",de_mDEAS1, 0)

/*--------------------------------------------------------------------------
/ Time Machine - CPU Rev 2 /Alpha Type 2 16/32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------*/
INITGAMES11(tmach, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, 0)
DE_ROMSTART48(tmach,"tmach2-4.b5",0x6ef3cf07,
                    "tmach2-4.c5",0xb61035f5)
DE1S_SOUNDROM244(   "tmachf7.rom",0x5d4994bb,      //F7 on schem (sound)
                    "tmachf6.rom",0xc04b07ad,      //F6 on schem (voice1)
                    "tmachf4.rom",0x70f70888)      //F4 on schem (voice2)
DE_ROMEND
#define input_ports_tmach input_ports_des11
CORE_GAMEDEFNV(tmach, "Time Machine",1988,"Data East",de_mDEAS1, 0)


/*-----------------------------------------------------------------------------------
/ Playboy 35th Anniversary - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------------------------*/
INITGAMES11(play, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, 0)
DE_ROMSTART88(play,"play2-4.b5",0xbc8d7b32,
                   "play2-4.c5",0x47c30bc2)
DE1S_SOUNDROM244(  "pbsnd7.dat",0xc2cf2cc5,       //F7 on schem (sound)
                   "pbsnd6.dat",0xc2570631,       //F6 on schem (voice1)
                   "pbsnd5.dat",0x0fd30569)       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_play input_ports_des11
CORE_GAMEDEFNV(play,"Playboy 35th Anniversary",1989,"Data East",de_mDEAS1,0)

/*-----------------------------------------------------------------------------------
/ Monday Night Football - CPU Rev 2 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/----------------------------------------------------------------------------------*/
INITGAMES11(mnfb,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(mnfb,"mnfb2-7.b5",  0x995eb9b8,
                   "mnfb2-7.c5",  0x579d81df)
DE1S_SOUNDROM244(  "mnf-f7.256",  0xfbc2d6f6,       //F7 on schem (sound)
                   "mnf-f5-6.512",0x0c6ea963,     //F6 on schem (voice1)
                   "mnf-f4-5.512",0xefca5d80)     //F4 on schem (voice2)
DE_ROMEND
#define input_ports_mnfb input_ports_des11
CORE_GAMEDEFNV(mnfb,"Monday Night Football",1989,"Data East",de_mDEAS1,0)
/*------------------------------------------------------------------
/ Robocop - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------*/
INITGAMES11(robo,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART88(robo,"robob5.a34",0x5a611004,
                   "roboc5.a34",0xc8705f47)
DE1S_SOUNDROM244(  "robof7.rom",0xfa0891bd,       //F7 on schem (sound)
                   "robof6.rom",0x9246e107,       //F6 on schem (voice1)
                   "robof4.rom",0x27d31df3)       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_robo input_ports_des11
CORE_GAMEDEFNV(robo,"Robocop",1989,"Data East",de_mDEAS1,0)

/*-------------------------------------------------------------------------------
/ Phantom of the Opera - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/-------------------------------------------------------------------------------*/
INITGAMES11(poto,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(poto,"potob5.3-2",0xbdc39205,
                   "potoc5.3-2",0xe6026455)
DE1S_SOUNDROM244(  "potof7.rom",0x2e60b2e3,       //7f
                   "potof6.rom",0x62b8f74b,       //6f
                   "potof5.rom",0x5a0537a8)       //4f
DE_ROMEND
#define input_ports_poto input_ports_des11
CORE_GAMEDEFNV(poto,"The Phantom of the Opera",1990,"Data East",de_mDEAS1,0)

/*--------------------------------------------------------------------------------
/ Back To the Future - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------------*/
INITGAMES11(bttf,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART88(bttf,"bttfb5.2-0",0xc0d4df6b,
                   "bttfc5.2-0",0xa189a189)
DE1S_SOUNDROM244(  "bttfsf7.rom",0x7673146e,      //7f
                   "bttfsf6.rom",0x468a8d9c,      //6f
                   "bttfsf5.rom",0x37a6f6b8)      //4f
DE_ROMEND
#define input_ports_bttf input_ports_des11
CORE_GAMEDEFNV(bttf,"Back To the Future",1990,"Data East",de_mDEAS1,0)

/*------------------------------------------------------------------------
/ The Simpsons - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/128K Sound Roms
/------------------------------------------------------------------------*/
INITGAMES11(simp,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(simp, "simpb5.2-7",0x701c4a4b,
                    "simpc5.2-7",0x400a98b2)
DE1S_SOUNDROM288(   "simpf7.rom",0xa36febbc,       //7f
                    "simpf6.rom",0x2eb32ed0,       //6f
                    "simpf5.rom",0xbd0671ae)       //4f
DE_ROMEND
#define input_ports_simp input_ports_des11
CORE_GAMEDEFNV(simp,"The Simpsons",1990,"Data East",de_mDEAS1,0)


/***********************************************************************/
/*************** GAMES USING 128X16 DMD DISPLAY ************************/
/***********************************************************************/

/*------------------------------------------------------------
/ Checkpoint - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAMES11(chkpnt,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE1S, SNDBRD_DEDMD16, 0)
DE_ROMSTART48(chkpnt,"chkpntb5.107",0x9fbae8e3,
                     "chkpntc5.107",0x082dc283)
DE_DMD16ROM1(        "chkpntds.512",0x14d9c6d6)
DE1S_SOUNDROM288(    "chkpntf7.rom",0xe6f6d716,      //7f
                     "chkpntf6.rom",0x2d08043e,      //6f
                     "chkpntf5.rom",0x167daa2c)      //4f
DE_ROMEND
#define input_ports_chkpnt input_ports_des11
CORE_GAMEDEFNV(chkpnt,"Checkpoint",1991,"Data East",de_mDEDMD16S1,0)


/*-----------------------------------------------------------------------------
/ Teenage Mutant Ninja Turtles - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/-----------------------------------------------------------------------------*/
INITGAMES11(tmnt,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE1S, SNDBRD_DEDMD16, 0)
DE_ROMSTART48(tmnt,"tmntb5a.104",0xf508eeee,
                   "tmntc5a.104",0xa33d18d4)
DE_DMD16ROM1(      "tmntdsp.104",0x545686b7)
DE1S_SOUNDROM288(   "tmntf7.rom",0x59ba0153,        //7f
                   "tmntf6.rom",0x5668d45a,        //6f
                   "tmntf4.rom",0x6c38cd84)        //4f
DE_ROMEND
#define input_ports_tmnt input_ports_des11
CORE_GAMEDEFNV(tmnt,"Teenage Mutant Ninja Turtles",1991,"Data East",de_mDEDMD16S1,0)


/***************************************************************************/
/** ALL FOLLOWING GAMES BELOW STARTED USING NEW SOUND BOARD WITH BSMT2000 **/
/***************************************************************************/

/*-------------------------------------------------------------
/ Batman - CPU Rev 3 /DMD Type 1 128K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAMES11(batmn,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD16, 0)
DE_ROMSTART88(batmn,"batcpub5.101",0xa7f5754e,
                    "batcpuc5.101",0x1fcb85ca)
DE_DMD16ROM2(       "batdsp.106",0x4c4120e7)
DE2S_SOUNDROM021(   "batman.u7" ,0xb2e88bf5,
                    "batman.u17" ,0xb84914dd,
                    "batman.u21" ,0x42dab6ac)
DE_ROMEND
#define input_ports_batmn input_ports_des11
CORE_GAMEDEFNV(batmn,"Batman",1992,"Data East",de_mDEDMD16S2A,0)

/*-------------------------------------------------------------
/ Star Trek - CPU Rev 3 /DMD Type 1 128K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(trek,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD16, 0)
DE_ROMSTARTx0(trek,"trekcpuu.201",0xea0681fe)
DE_DMD16ROM2(      "trekdspa.109",0xa7e7d44d)
DE2S_SOUNDROM022(  "trek.u7"  ,0xf137abbb,
                   "trek.u17" ,0x531545da,
                   "trek.u21" ,0x6107b004)
DE_ROMEND
#define input_ports_trek input_ports_des11
CORE_GAMEDEFNV(trek,"Star Trek 25th Anniversary",1992,"Data East",de_mDEDMD16S2A,0)

/*-------------------------------------------------------------
/ Hook - CPU Rev 3 /DMD  Type 1 128K Rom - CPU Rom
/------------------------------------------------------------*/
INITGAMES11(hook,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD16, 0)
DE_ROMSTARTx0(hook, "hokcpua.408",0x46477fc7)
DE_DMD16ROM2(       "hokdspa.401",0x59a07eb5)
DE2S_SOUNDROM022(   "hooksnd.u7" ,0x642f45b3,
                    "hook-voi.u17" ,0x6ea9fcd2,
                    "hook-voi.u21" ,0xb5c275e2)
DE_ROMEND
#define input_ports_hook input_ports_des11
CORE_GAMEDEFNV(hook,"Hook",1992,"Data East",de_mDEDMD16S2A,0)


/***********************************************************************/
/*************** GAMES USING 128X32 DMD DISPLAY ************************/
/***********************************************************************/

/*----------------------------------------------------------------
/ Lethal Weapon 3 - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/---------------------------------------------------------------*/
INITGAMES11(lw3,GEN_DEDMD32, 0, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, 0)
DE_ROMSTARTx0(lw3, "lw3cpuu.208",0xa3041f8a)
DE_DMD32ROM44(     "lw3drom1.a26",0x44a4cf81,
                   "lw3drom0.a26",0x22932ed5)
DE2S_SOUNDROM022(  "lw3u7.dat"  ,0xba845ac3,
                   "lw3u17.dat" ,0xe34cf2fc,
                   "lw3u21.dat" ,0x82bed051)
DE_ROMEND
#define input_ports_lw3 input_ports_des11
CORE_GAMEDEFNV(lw3,"Lethal Weapon 3",1992,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Star Wars - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(stwarde,GEN_DEDMD32, 0, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, 0)
DE_ROMSTARTx0(stwarde,"starcpua.103",0x318085ca)
DE_DMD32ROM8x(        "sw4mrom.a15",0x00c87952)
DE2S_SOUNDROM042(     "s-wars.u7"  ,0xcefa19d5,
                      "s-wars.u17" ,0x7950a147,
                      "s-wars.u21" ,0x7b08fdf1)
DE_ROMEND
#define input_ports_stwarde input_ports_des11
CORE_GAMEDEFNV(stwarde,"Star Wars",1992,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Rocky & Bullwinkle - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(rab,GEN_DEDMD32, 0, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(rab, "rabcpua.130",0xf59b1a53)
DE_DMD32ROM8x(     "rbdspa.130",0xb6e2176e)
DE2S_SOUNDROM142(  "rab.u7"  ,0xb232e630,
                   "rab.u17" ,0x7f2b53b8,
                   "rab.u21" ,0x3de1b375)
DE_ROMEND
#define input_ports_rab input_ports_des11
CORE_GAMEDEFNV(rab,"Rocky & Bullwinkle",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Jurassic Park - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(jurpark,GEN_DEDMD32, 0, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(jurpark,"jpcpua.513",0x9f70a937)
DE_DMD32ROM8x(        "jpdspa.510",0x9ca61e3c)
DE2S_SOUNDROM142(     "jpu7.dat"  ,0xf3afcf13,
                      "jpu17.dat" ,0x38135a23,
                      "jpu21.dat" ,0x6ac1554c)
DE_ROMEND
#define input_ports_jurpark input_ports_des11
CORE_GAMEDEFNV(jurpark,"Jurassic Park",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Last Action Hero - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(lah,GEN_DEDMD32, 0, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(lah,  "lahcpua.112",0xe7422236)
DE_DMD32ROM8x(      "lahdispa.106",0xca6cfec5)
DE2S_SOUNDROM142(   "lahsnd.u7"  ,0x0279c45b,
                    "lahsnd.u17" ,0xd0c15fa6,
                    "lahsnd.u21" ,0x4571dc2e)
DE_ROMEND
#define input_ports_lah input_ports_des11
CORE_GAMEDEFNV(lah,"Last Action Hero",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Tales From the Crypt - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(tftc,GEN_DEDMD32, 0, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(tftc, "tftccpua.303",0xe9bec98e)
DE_DMD32ROM8x(      "tftcdspa.301",0x3888d06f)
DE2S_SOUNDROM144(   "sndu7.dat"    ,0x7963740e,
                    "sndu17.dat" ,0x5c5d009a,
                    "sndu21.dat" ,0xa0ae61f7)
DE_ROMEND
#define input_ports_tftc input_ports_des11
CORE_GAMEDEFNV(tftc,"Tales From the Crypt",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Tommy - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
//INITGAME(tommy,DE_CPUREV3b, 0, FLIP6364, 3)

#define sBlinderMotor   CORE_CUSTSOLNO(1) /* 33 */
/*-- return status of custom solenoids --*/
static int tommy_getSol(int solNo) { return (solNo == sBlinderMotor) ? (core_getSol(44) > 0) : 0; }
static core_tGameData tommyGameData = {
  GEN_DEDMD32, 0,
  { FLIP6364, 0,0,1, // We need 1 custom solenoids!
    SNDBRD_DE2S,SNDBRD_DEDMD32,S11_PRINTERLINE,0,
    tommy_getSol
  }, NULL, {{0}}, {10}
};
static void init_tommy(void) { core_gameData = &tommyGameData; }
DE_ROMSTARTx0(tommy,"tomcpua.400", 0xd0310a1a)
DE_DMD32ROM8x(      "tommydva.400",0x9e640d09)
DE2S_SOUNDROM14444( "tommysnd.u7" ,0xab0b4626,
                    "tommysnd.u17",0x11bb2aa7,
                    "tommysnd.u21",0xbb4aeec3,
                    "tommysnd.u36",0x208d7aeb,
                    "tommysnd.u37",0x46180085)
DE_ROMEND
#define input_ports_tommy input_ports_des11
CORE_GAMEDEFNV(tommy,"Tommy",1994,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ WWF Royal Rumble - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(wwfrumb,GEN_DEDMD32, 0, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32,S11_PRINTERLINE)
DE_ROMSTARTx0(wwfrumb,  "wfcpuc5.512"  ,0x7e9ead89)
DE_DMD32ROM8x(          "wfdisp0.400"  ,0xe190b90f)
DE2S_SOUNDROM1444(      "wfsndu7.512"  ,0xeb01745c,
                        "wfsndu17.400" ,0x7d9c2ca8,
                        "wfsndu21.400" ,0x242dcdcb,
                        "wfsndu36.400" ,0x39db8d85)
DE_ROMEND
#define input_ports_wwfrumb input_ports_des11
CORE_GAMEDEFNV(wwfrumb,"WWF Royal Rumble",1994,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Guns N Roses - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
#define sLMagnet   CORE_CUSTSOLNO(1) /* 33 */
#define sTMagnet   CORE_CUSTSOLNO(2) /* 34 */
#define sRMagnet   CORE_CUSTSOLNO(3) /* 35 */

/*-- return status of custom solenoids --*/
static int gnr_getSol(int solNo) {
  if ((solNo == sLMagnet) || (solNo == sTMagnet) || (solNo == sRMagnet))
    return core_getSol(CORE_FIRSTEXTSOL + solNo - CORE_FIRSTCUSTSOL);
  return 0;
}

static core_tGameData gnrGameData = {
  GEN_DEDMD32, 0,
  { FLIP6364, 0,0,3, //We need 3 custom solenoids!
    SNDBRD_DE2S,SNDBRD_DEDMD32,S11_PRINTERLINE,0, gnr_getSol
  }, NULL, {{0}}, {10}
};
static void init_gnr(void) { core_gameData = &gnrGameData; }
DE_ROMSTARTx0(gnr, "gnrcpua.300",0xfaf0cc8c)
DE_DMD32ROM8x(     "gnrdispa.300",0x4abf29e3)
DE2S_SOUNDROM14444("gnru7.snd"  ,0x3b9de915,
                   "gnru17.snd" ,0x3d3219d6,
                   "gnru21.snd" ,0xd2ca17ab,
                   "gnru36.snd" ,0x5b32396e,
                   "gnru37.snd" ,0x4930e1f2)
DE_ROMEND
#define input_ports_gnr input_ports_des11
CORE_GAMEDEFNV(gnr,"Guns N Roses",1994,"Data East",de_mDEDMD32S2A,0)

/***********************************************************************/
/*************** GAMES USING 192X64 DMD DISPLAY ************************/
/***********************************************************************/
//Snd Works (Voices might be messed not sure)
/*-------------------------------------------------------------
/ Maverick - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(maverick, GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64,S11_PRINTERLINE)
DE_ROMSTARTx0(maverick, "mavcpua.404",0x9f06bd8d)
DE_DMD64ROM88(   "mavdsar0.401",0x35b811af,
                 "mavdsar3.401",0xc4c126ae)
DE2S_SOUNDROM144("mavu7.dat"  , 0x427e6ab9,
                 "mavu17.dat" , 0xcba377b8,
                 "mavu21.dat" , 0xbe0c6a6f)
DE_ROMEND
#define input_ports_maverick input_ports_des11
CORE_GAMEDEFNV(maverick,"Maverick",1994,"Data East",de_mDEDMD64S2A,0)

/*****************************************************************************************************************************/
/* NOTE: SEGA Began Distribution of the following games, although they run on Data East Hardware, so they stay in this file! */
/*****************************************************************************************************************************/
/*-------------------------------------------------------------
/ Frankenstein - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(frankst,GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64,S11_PRINTERLINE)
DE_ROMSTARTx0(frankst,"franka.103",  0xa9aba9be)
DE_DMD64ROM88(        "frdspr0a.103",0x9dd09c7d,
                      "frdspr3a.103",0x73b538bb)
DE2S_SOUNDROM1444(    "frsnd.u7"  ,  0x084f856c,
                      "frsnd.u17" ,  0x0da904d6,
                      "frsnd.u21" ,  0x14d4bc12,
                      "frsnd.u36" ,  0x9964d721)
DE_ROMEND
#define input_ports_frankst input_ports_des11
CORE_GAMEDEFNV(frankst,"Mary Shelley's Frankenstein",1994,"Sega",de_mDEDMD64S2A,0)

//Start of the Portals Diagnostic Menu System
/*-------------------------------------------------------------
/ Baywatch - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(baywatch,GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64,S11_PRINTERLINE)
DE_ROMSTARTx0(baywatch, "baycpua.400", 0x89facfda)
DE_DMD64ROM88(          "bayrom0a.400",0x43d615c6,
                        "bayrom3a.400",0x41bcb66b)
DE2S_SOUNDROM144(       "bayw.u7"  ,   0x90d6d8a8,
                        "bayw.u17" ,   0xb20fde56,
                        "bayw.u21" ,   0xb7598881)
DE_ROMEND
#define input_ports_baywatch input_ports_des112
CORE_GAMEDEFNV(baywatch,"Baywatch",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever 4.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(batmanf, GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64,S11_PRINTERLINE)
DE_ROMSTARTx0(batmanf, "batnova.401", 0x4e62df4e)
DE_DMD64ROM88(         "bfdrom0a.401",0x8a3c20ad,
                       "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(      "bmfu7.bin"  ,0x58c0d144,
                       "bmfu17.bin" ,0xedcd5c10,
                       "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_batmanf input_ports_des112
CORE_GAMEDEFNV(batmanf,"Batman Forever (4.0)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever 3.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(batmanf3, "batcpua.302", 0x5ae7ce69)
DE_DMD64ROM88(          "bmfrom0a.300",0x764bb217,
                        "bmfrom3a.300",0xb4e3b515)
DE2S_SOUNDROM144(       "bmfu7.bin"  , 0x58c0d144,
                        "bmfu17.bin" , 0xedcd5c10,
                        "bmfu21.bin" , 0xe41a516d)
DE_ROMEND
#define input_ports_batmanf3 input_ports_batmanf
#define init_batmanf3        init_batmanf
CORE_CLONEDEFNV(batmanf3,batmanf,"Batman Forever (3.0)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (UK) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_uk,"batnove.401",0x80f6e4af)
DE_DMD64ROM88(       "bfdrom0a.401",0x8a3c20ad,
                     "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(    "bmfu7.bin"  ,0x58c0d144,
                     "bmfu17.bin" ,0xedcd5c10,
                     "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_uk input_ports_batmanf
#define init_bmf_uk        init_batmanf
CORE_CLONEDEFNV(bmf_uk,batmanf,"Batman Forever (English)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (CN) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_cn, "batnovc.401",0x99936537)
DE_DMD64ROM88(        "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_cn input_ports_batmanf
#define init_bmf_cn        init_batmanf
CORE_CLONEDEFNV(bmf_cn,batmanf,"Batman Forever (Canadian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (NO) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_no, "batnovn.401",0x79dd48b4)
DE_DMD64ROM88(        "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_no input_ports_batmanf
#define init_bmf_no        init_batmanf
CORE_CLONEDEFNV(bmf_no,batmanf,"Batman Forever (Norwegian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (SV) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_sv, "batnovt.401",0x854029ab)
DE_DMD64ROM88(        "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_sv input_ports_batmanf
#define init_bmf_sv        init_batmanf
CORE_CLONEDEFNV(bmf_sv,batmanf,"Batman Forever (Swedish)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (AT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_at, "batnovh.401",0xacba13d7)
DE_DMD64ROM88(        "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_at input_ports_batmanf
#define init_bmf_at        init_batmanf
CORE_CLONEDEFNV(bmf_at,batmanf,"Batman Forever (Austrian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (CH) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_ch, "batnovs.401",0x4999d5f9)
DE_DMD64ROM88(        "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_ch input_ports_batmanf
#define init_bmf_ch        init_batmanf
CORE_CLONEDEFNV(bmf_ch,batmanf,"Batman Forever (Swiss)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (DE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_de, "batnovg.401",0xdd37e99a)
DE_DMD64ROM88(          "bfdrom0g.401",0x3a2d7d53,
                      "bfdrom3g.401",0x94e424f1)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_de input_ports_batmanf
#define init_bmf_de        init_batmanf
CORE_CLONEDEFNV(bmf_de,batmanf,"Batman Forever (German)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (BE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_be, "batnovb.401",0x21309873)
DE_DMD64ROM88(        "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_be input_ports_batmanf
#define init_bmf_be        init_batmanf
CORE_CLONEDEFNV(bmf_be,batmanf,"Batman Forever (Belgian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (FR) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_fr, "batnovf.401",0x4baa793d)
DE_DMD64ROM88(        "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_fr input_ports_batmanf
#define init_bmf_fr        init_batmanf
CORE_CLONEDEFNV(bmf_fr,batmanf,"Batman Forever (French)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (NL) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_nl, "batnovd.401",0x6ae4570c)
DE_DMD64ROM88(        "bfdrom0f.401",0xe7473f6f,
                      "bfdrom3f.401",0xf7951709)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_nl input_ports_batmanf
#define init_bmf_nl        init_batmanf
CORE_CLONEDEFNV(bmf_nl,batmanf,"Batman Forever (Dutch)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (IT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_it, "batnovi.401",0x7053ef9e)
DE_DMD64ROM88(          "bfdrom0i.401",0x23051253,
                      "bfdrom3i.401",0x82b61a41)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_it input_ports_batmanf
#define init_bmf_it        init_batmanf
CORE_CLONEDEFNV(bmf_it,batmanf,"Batman Forever (Italian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (SP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_sp, "batnova.401",0x4e62df4e)
DE_DMD64ROM88(        "bfdrom0l.401",0xb22b10d9,
                      "bfdrom3l.401",0x016b8666)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_sp input_ports_batmanf
#define init_bmf_sp        init_batmanf
CORE_CLONEDEFNV(bmf_sp,batmanf,"Batman Forever (Spanish)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (JP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_jp, "batnovj.401",0xeef9bef0)
DE_DMD64ROM88(       "bfdrom0a.401",0x8a3c20ad,
                      "bfdrom3a.401",0x5ef46847)
DE2S_SOUNDROM144(     "bmfu7.bin"  ,0x58c0d144,
                      "bmfu17.bin" ,0xedcd5c10,
                      "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_jp input_ports_batmanf
#define init_bmf_jp        init_batmanf
CORE_CLONEDEFNV(bmf_jp,batmanf,"Batman Forever (Japanese)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (Timed Version) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_time, "batnova.401", 0x4e62df4e)
DE_DMD64ROM88(          "bfdrom0t.401",0xb83b8d28,
                        "bfdrom3t.401",0xa024b1a5)
DE2S_SOUNDROM144(       "bmfu7.bin"  ,0x58c0d144,
                        "bmfu17.bin" ,0xedcd5c10,
                        "bmfu21.bin" ,0xe41a516d)
DE_ROMEND
#define input_ports_bmf_time input_ports_batmanf
#define init_bmf_time        init_batmanf
CORE_CLONEDEFNV(bmf_time,batmanf,"Batman Forever (Timed Play)",1995,"Sega",de_mDEDMD64S2A,0)


/***********************************************************************/
/*************** SPECIAL TEST CHIP - NO DISPLAY ************************/
/***********************************************************************/
/* NO OUTPUT */
static core_tLCDLayout de_NoOutput[] = {{0}};
/*-------------------------------------------------------------
/ Data East Test Chip 64K ROM
/------------------------------------------------------------*/
INITGAMES11(detest, GEN_DE, de_NoOutput, FLIP1516, 0, 0, 0)
DE_ROMSTARTx0(detest,"de_test.512",0xbade8ca8)
DE_ROMEND
#define input_ports_detest input_ports_des11
CORE_GAMEDEFNV(detest,"Data East Test Chip",1998,"Data East",de_mDEA,0)

