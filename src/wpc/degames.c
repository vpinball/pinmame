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
		b) -01 generation: (Tales from the Crypt to Guns N' Roses)

	6) 520-5092-01: 192X64 DMD - 68000 CPU + separate controller board
	   (Maverick to Batman Forever)

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

static struct core_dispLayout de_dispAlpha1[] = { /* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows, 1 X 4 Numeric*/
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG8),  DISP_SEG_7(1,1, CORE_SEG8),
  DISP_SEG_CREDIT(20,28,CORE_SEG7),DISP_SEG_BALLS(0,8,CORE_SEG7), {0}
};

static struct core_dispLayout de_dispAlpha2[] = { /* 2 X 7 AlphaNumeric Rows, 2 X 7 Numeric Rows */
  DISP_SEG_7(0,0, CORE_SEG16), DISP_SEG_7(0,1, CORE_SEG16),
  DISP_SEG_7(1,0, CORE_SEG8),  DISP_SEG_7(1,1, CORE_SEG8), {0}
};

static struct core_dispLayout de_dispAlpha3[] = { /* 2 X 16 AlphaNumeric Rows */
  DISP_SEG_16(0,CORE_SEG16),DISP_SEG_16(1,CORE_SEG16),{0}
};

static struct core_dispLayout de_128x16DMD[] = { /* 128x16 DMD OUTPUT */
  {0,0,16,128,CORE_DMD,(genf *)dedmd16_update,NULL}, {0}
};

static struct core_dispLayout de_128x32DMD[] = { /* 128x32 DMD OUTPUT */
  {0,0,32,128,CORE_DMD,(genf *)dedmd32_update,NULL}, {0}
};

static struct core_dispLayout de_192x64DMD[] = { /* 192x64 DMD OUTPUT */
  {0,0,64,192,CORE_DMD,(genf *)dedmd64_update,NULL}, {0}
};

/***************************************************/
/* GAMES APPEAR IN PRODUCTION ORDER (MORE OR LESS) */
/***************************************************/
/*-------------------------------------------------------------------
/ Laser War - CPU Rev 1 /Alpha Type 1 - 32K ROM - 32/64K Sound Roms
/-------------------------------------------------------------------*/
INITGAMES11(lwar, GEN_DE, de_dispAlpha1, FLIP4746, SNDBRD_DE1S, 0, 0)
DE_ROMSTARTx8(lwar_a83,"lwar8-3.c5",CRC(eee158ee) SHA1(54db2342bdd15b16fee906dc65f183a957fd0012))
DE1S_SOUNDROM244(   "lwar_e9.snd",CRC(9a6c834d) SHA1(c6e2c4658db4bd8dfcbb0351793837cdff30ba28),   //F7 on schem (sound)
                    "lwar_e6.snd",CRC(7307d795) SHA1(5d88b8d883a2f17ca9fa30c7e7ac29c9f236ac4d),   //F6 on schem (voice1)
                    "lwar_e7.snd",CRC(0285cff9) SHA1(2c5e3de649e419ec7944059f2a226aaf58fe2af5))   //F4 on schem (voice2)
DE_ROMEND
#define input_ports_lwar input_ports_des11
CORE_GAMEDEF(lwar,a83,"Laser War (8.3)",1987,"Data East",de_mDEAS1,0)

DE_ROMSTARTx8(lwar_e90,"lwar9-0.e5",CRC(b596151f) SHA1(10dade79ded71625770ec7e21ea50b7aa64023d0))
DE1S_SOUNDROM244(   "lwar_e9.snd",CRC(9a6c834d) SHA1(c6e2c4658db4bd8dfcbb0351793837cdff30ba28),   //F7 on schem (sound)
                    "lwar_e6.snd",CRC(7307d795) SHA1(5d88b8d883a2f17ca9fa30c7e7ac29c9f236ac4d),   //F6 on schem (voice1)
                    "lwar_e7.snd",CRC(0285cff9) SHA1(2c5e3de649e419ec7944059f2a226aaf58fe2af5))   //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(lwar,e90,a83,"Laser War (9.0 Europe)",1987,"Data East",de_mDEAS1,0)

DE_ROMSTARTx8(lwar_a81,"c100_g8.256",CRC(fe63ef04) SHA1(edab4b7fab4a016e653a546110a4bc8c563e7cb7))
DE1S_SOUNDROM244(   "lwar_e9.snd",CRC(9a6c834d) SHA1(c6e2c4658db4bd8dfcbb0351793837cdff30ba28),   //F7 on schem (sound)
                    "lwar_e6.snd",CRC(7307d795) SHA1(5d88b8d883a2f17ca9fa30c7e7ac29c9f236ac4d),   //F6 on schem (voice1)
                    "lwar_e7.snd",CRC(0285cff9) SHA1(2c5e3de649e419ec7944059f2a226aaf58fe2af5))   //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(lwar,a81,a83,"Laser War (8.1)",1987,"Data East",de_mDEAS1,0)

/*-------------------------------------------------------------------------
/ Secret Service - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32K/64K Sound Roms
/-------------------------------------------------------------------------*/
INITGAMES11(ssvc, GEN_DE, de_dispAlpha2, FLIP3031, SNDBRD_DE1S, 0, 0)
DE_ROMSTART88(ssvc_a26,"ssvc2-6.b5",CRC(e5eab8cd) SHA1(63cb678084d4fb2131ba64ed9de1294830057960),
                   "ssvc2-6.c5", CRC(171b97ae) SHA1(9d678b7b91a5d50ea3cf4f2352094c2355f917b2))
DE1S_SOUNDROM244(  "sssndf7.rom",CRC(980778d0) SHA1(7c1f14d327b6d0e6d0fef058f96bb1cb440c9330),      //F7 on schem (sound)
                   "ssv1f6.rom", CRC(ccbc72f8) SHA1(c5c13fb8d05d7fb4005636655073d88b4d12d65e),       //F6 on schem (voice1)
                   "ssv2f4.rom", CRC(53832d16) SHA1(2227eb784e0221f1bf2bdf7ea48ecd122433f1ea))       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_ssvc input_ports_des11
CORE_GAMEDEF(ssvc,a26,"Secret Service (2.6)",1988,"Data East",de_mDEAS1, 0)

DE_ROMSTART88(ssvc_b26,"ssvc2-6.b5",CRC(e5eab8cd) SHA1(63cb678084d4fb2131ba64ed9de1294830057960),
                   "ssvc2-6.c5", CRC(171b97ae) SHA1(9d678b7b91a5d50ea3cf4f2352094c2355f917b2))
DE1S_SOUNDROM244(  "sssndf7b.rom",CRC(4bd6b16a) SHA1(b9438a16cd35820628fe6eb82287b2c39fe4b1c6),      //F7 on schem (sound)
                   "ssv1f6.rom", CRC(ccbc72f8) SHA1(c5c13fb8d05d7fb4005636655073d88b4d12d65e),       //F6 on schem (voice1)
                   "ssv2f4.rom", CRC(53832d16) SHA1(2227eb784e0221f1bf2bdf7ea48ecd122433f1ea))       //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(ssvc,b26,a26,"Secret Service (2.6, alternate sound)",1988,"Data East",de_mDEAS1, 0)

DE_ROMSTART88(ssvc_a42,"ss-b5.256",CRC(e7d27ea1) SHA1(997412f62c95cffc0cf9eba065fbc020574c7ad5),
                   "ss-c5.256", CRC(eceab834) SHA1(d946adac7ec8688709fd75108674a82f2f5c7b53))
DE1S_SOUNDROM244(  "sssndf7b.rom",CRC(4bd6b16a) SHA1(b9438a16cd35820628fe6eb82287b2c39fe4b1c6),      //F7 on schem (sound)
                   "ssv1f6.rom", CRC(ccbc72f8) SHA1(c5c13fb8d05d7fb4005636655073d88b4d12d65e),       //F6 on schem (voice1)
                   "ssv2f4.rom", CRC(53832d16) SHA1(2227eb784e0221f1bf2bdf7ea48ecd122433f1ea))       //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(ssvc,a42,a26,"Secret Service (4.2, alternate sound)",1988,"Data East",de_mDEAS1, 0)

/*-----------------------------------------------------------------------
/ Torpedo Alley - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/------------------------------------------------------------------------*/
INITGAMES11(torp, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, 0)
DE_ROMSTART88(torp_e21,"torpe2-1.b5",CRC(ac0b03e3) SHA1(0ac57b2fec29cdc90ab35cba49844f0cf545d959),
                    "torpe2-1.c5",CRC(9ad33882) SHA1(c4504d8e136f667652f79b54d4e8d775169c6ac3))
DE1S_SOUNDROM244(   "torpef7.rom",CRC(26f4c33e) SHA1(114f85e93e7b699c4cd6ce1298f95228d439deba),      //F7 on schem (sound)
                    "torpef6.rom",CRC(b214a7ea) SHA1(d972148395581844e3eaed08f755f3e2217dbbc0),      //F6 on schem (voice1)
                    "torpef4.rom",CRC(83a4e7f3) SHA1(96deac9251fe68cc0319ac009becd424c4e444c5))      //F4 on schem (voice2)
DE_ROMEND
#define input_ports_torp input_ports_des11
CORE_GAMEDEF(torp,e21,"Torpedo Alley (2.1 Europe)",1988,"Data East",de_mDEAS1, 0)

DE_ROMSTART88(torp_a16,"b5.256",CRC(89711a7c) SHA1(b976b32b287d6cbaf4c448697f8aa12452db1f0b),
                    "c5.256",CRC(3b3d754f) SHA1(c5d4a09f4daf92af78d778148377fa0d2a550761))
DE1S_SOUNDROM244(   "torpef7.rom",CRC(26f4c33e) SHA1(114f85e93e7b699c4cd6ce1298f95228d439deba),      //F7 on schem (sound)
                    "torpef6.rom",CRC(b214a7ea) SHA1(d972148395581844e3eaed08f755f3e2217dbbc0),      //F6 on schem (voice1)
                    "torpef4.rom",CRC(83a4e7f3) SHA1(96deac9251fe68cc0319ac009becd424c4e444c5))      //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(torp,a16,e21,"Torpedo Alley (1.6)",1988,"Data East",de_mDEAS1, 0)

/*--------------------------------------------------------------------------
/ Time Machine - CPU Rev 2 /Alpha Type 2 16/32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------*/
INITGAMES11(tmac, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, 0)
DE_ROMSTART48(tmac_a24,"tmach2-4.b5",CRC(6ef3cf07) SHA1(3fabfbb2166273bf5bfab06d92fff094d3331d1a),
                    "tmach2-4.c5",CRC(b61035f5) SHA1(08436b68f37323f50c1fec86aba303a1690af653))
DE1S_SOUNDROM244(   "tmachf7.rom",CRC(0f518bd4) SHA1(05e24ca0e76d576c65d9d2a01417f1ad2aa984bb),      //F7 on schem (sound)
                    "tmachf6.rom",CRC(47e61641) SHA1(93cd946ebc9f69d82512429a9ae5f2754499b00a),      //F6 on schem (voice1)
                    "tmachf4.rom",CRC(51e3aade) SHA1(38fc0f3a9c727bfd07fbcb16c3ca6d0560dc65c3))      //F4 on schem (voice2)
DE_ROMEND
#define input_ports_tmac input_ports_des11
CORE_GAMEDEF(tmac,a24,"Time Machine (2.4)",1988,"Data East",de_mDEAS1, 0)

DE_ROMSTART48(tmac_a18,"tmach1-8.b5",CRC(5dabdc4c) SHA1(67fe261888ddaa088abe2f8a331eaa5ac34be92e),
                    "tmach1-8.c5",CRC(5a348def) SHA1(bf2b9a69d516d38e6f87c5886e0ba768c2dc28ab))
DE1S_SOUNDROM244(   "tmachf7.rom",CRC(0f518bd4) SHA1(05e24ca0e76d576c65d9d2a01417f1ad2aa984bb),      //F7 on schem (sound)
                    "tmachf6.rom",CRC(47e61641) SHA1(93cd946ebc9f69d82512429a9ae5f2754499b00a),      //F6 on schem (voice1)
                    "tmachf4.rom",CRC(51e3aade) SHA1(38fc0f3a9c727bfd07fbcb16c3ca6d0560dc65c3))      //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(tmac,a18,a24,"Time Machine (1.8)",1988,"Data East",de_mDEAS1, 0)

DE_ROMSTART48(tmac_g18,"tmachg18.b5",CRC(513d70ad) SHA1(dacdfc77956b1b5fb9bebca59fdb705aefa1b5b2),
                    "tmachg18.c5",CRC(5a348def) SHA1(bf2b9a69d516d38e6f87c5886e0ba768c2dc28ab))
DE1S_SOUNDROM244(   "tmachf7.rom",CRC(0f518bd4) SHA1(05e24ca0e76d576c65d9d2a01417f1ad2aa984bb),      //F7 on schem (sound)
                    "tmachf6.rom",CRC(47e61641) SHA1(93cd946ebc9f69d82512429a9ae5f2754499b00a),      //F6 on schem (voice1)
                    "tmachf4.rom",CRC(51e3aade) SHA1(38fc0f3a9c727bfd07fbcb16c3ca6d0560dc65c3))      //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(tmac,g18,a24,"Time Machine (1.8 German)",1988,"Data East",de_mDEAS1, 0)

/*-----------------------------------------------------------------------------------
/ Playboy 35th Anniversary - CPU Rev 2 /Alpha Type 2 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------------------------*/
INITGAMES11(play, GEN_DE, de_dispAlpha2, FLIP1516, SNDBRD_DE1S, 0, S11_MUXDELAY)
DE_ROMSTART88(play_a24,"play2-4.b5",CRC(bc8d7b32) SHA1(3b57dea2feb12315586283548e0bffdc8173b8fb),
                   "play2-4.c5",CRC(47c30bc2) SHA1(c62e192ec01f4884226e9628baa2cad10cc57bd9))
DE1S_SOUNDROM244(  "pbsnd7.dat",CRC(c2cf2cc5) SHA1(1277704b1b38558c341b52da5e06ffa9f07942ad),       //F7 on schem (sound)
                   "pbsnd6.dat",CRC(c2570631) SHA1(135db5b923689884c73aa5ce48f566db7f1cf831),       //F6 on schem (voice1)
                   "pbsnd5.dat",CRC(0fd30569) SHA1(0bf53fe4b5dffb5e15212c3371f51e98ad14e258))       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_play input_ports_des11
CORE_GAMEDEF(play,a24,"Playboy 35th Anniversary (2.4)",1989,"Data East",de_mDEAS1,0)

/*-----------------------------------------------------------------------------------
/ Monday Night Football - CPU Rev 2 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/----------------------------------------------------------------------------------*/
INITGAMES11(mnfb,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(mnfb_c29,"mnfb2-9.b5",CRC(2d6805d1) SHA1(f222cbf30d07975279eea210738f7d4f73b3fcf4) BAD_DUMP, // checksum needed to be patched on that one, so maybe bad dump
                   "mnfb2-9.c5",  CRC(98d50cf5) SHA1(59d3b16f8195ab95cece71a12dab3349dfeb2c2b))
DE1S_SOUNDROM244(  "mnf-f7.256",  CRC(fbc2d6f6) SHA1(33173c081de776d32e926481e94b265ec48d770b),     //F7 on schem (sound)
                   "mnf-f5-6.512",CRC(0c6ea963) SHA1(8c88fa588222ef8a6c872b8c5b49639b108384d4),     //F6 on schem (voice1)
                   "mnf-f4-5.512",CRC(efca5d80) SHA1(9655c885dd64aa170205170b6a0c052bd9367379))     //F4 on schem (voice2)
DE_ROMEND
#define input_ports_mnfb input_ports_des11
CORE_GAMEDEF(mnfb,c29,"Monday Night Football (2.9, 50cts)",1989,"Data East",de_mDEAS1,0)

DE_ROMSTART48(mnfb_c27,"mnfb2-7.b5",CRC(995eb9b8) SHA1(d05d74393fda59ffd8d7b5546313779cdb10d23e),
                   "mnfb2-7.c5",  CRC(579d81df) SHA1(9c96da34d37d3369513003e208222bd6e8698638))
DE1S_SOUNDROM244(  "mnf-f7.256",  CRC(fbc2d6f6) SHA1(33173c081de776d32e926481e94b265ec48d770b),     //F7 on schem (sound)
                   "mnf-f5-6.512",CRC(0c6ea963) SHA1(8c88fa588222ef8a6c872b8c5b49639b108384d4),     //F6 on schem (voice1)
                   "mnf-f4-5.512",CRC(efca5d80) SHA1(9655c885dd64aa170205170b6a0c052bd9367379))     //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(mnfb,c27,c29,"Monday Night Football (2.7, 50cts)",1989,"Data East",de_mDEAS1,0)

/*------------------------------------------------------------------
/ Robocop - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/-----------------------------------------------------------------*/
INITGAMES11(robo,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART88(robo_a34,"robob5.a34",CRC(5a611004) SHA1(08722f8f4386bbc467cfbe8854f0d45c4537bdc6),
                   "roboc5.a34",CRC(c8705f47) SHA1(a29ad9e4e0269ab19dae77b1e70ff84c8c8d9e85))
DE1S_SOUNDROM244(  "robof7.rom",CRC(fa0891bd) SHA1(332d03c7802989abf717564230993b54819ebc0d),       //F7 on schem (sound)
                   "robof6.rom",CRC(9246e107) SHA1(e8e72c0d099b17ea9e59ea7794011bad4c072c5e),       //F6 on schem (voice1)
                   "robof4.rom",CRC(27d31df3) SHA1(1611a508ce74eb62a07296d69782ea4fa14503fc))       //F4 on schem (voice2)
DE_ROMEND
#define input_ports_robo input_ports_des11
CORE_GAMEDEF(robo,a34,"Robocop (3.4)",1989,"Data East",de_mDEAS1,0)

DE_ROMSTART88(robo_a29,"robob5.a29",CRC(72497d0b) SHA1(8a970c879cd0aaef5970a77778f71c0f3d6da049),
                   "roboc5.a29",CRC(b251b0b6) SHA1(3d340070494b102703e282ae3a7970f6f8aaede9))
DE1S_SOUNDROM244(  "robof7.rom",CRC(fa0891bd) SHA1(332d03c7802989abf717564230993b54819ebc0d),       //F7 on schem (sound)
                   "robof6.rom",CRC(9246e107) SHA1(e8e72c0d099b17ea9e59ea7794011bad4c072c5e),       //F6 on schem (voice1)
                   "robof4.rom",CRC(27d31df3) SHA1(1611a508ce74eb62a07296d69782ea4fa14503fc))       //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(robo,a29,a34,"Robocop (2.9)",1989,"Data East",de_mDEAS1,0)

DE_ROMSTART88(robo_a30,"b5.256",CRC(6870f3ae) SHA1(f02cace5f1d1922aed52c84efe60a46e5297865c),
                   "c5.256",CRC(f2de58cf) SHA1(0b5dd14761b4c64c1b01faad923ab671573499c5))
DE1S_SOUNDROM244(  "robof7.rom",CRC(fa0891bd) SHA1(332d03c7802989abf717564230993b54819ebc0d),       //F7 on schem (sound)
                   "robof6.rom",CRC(9246e107) SHA1(e8e72c0d099b17ea9e59ea7794011bad4c072c5e),       //F6 on schem (voice1)
                   "robof4.rom",CRC(27d31df3) SHA1(1611a508ce74eb62a07296d69782ea4fa14503fc))       //F4 on schem (voice2)
DE_ROMEND
CORE_CLONEDEF(robo,a30,a34,"Robocop (3.0)",1989,"Data East",de_mDEAS1,0)

/*-------------------------------------------------------------------------------
/ Phantom of the Opera - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/-------------------------------------------------------------------------------*/
INITGAMES11(poto,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(poto_a32,"potob5.3-2",CRC(bdc39205) SHA1(67b3f56655ef2cc056912ab6e351cf83352abaa9),
                   "potoc5.3-2",CRC(e6026455) SHA1(c1441fda6181e9014a8a6f93b7405998a952f508))
DE1S_SOUNDROM244(  "potof7.rom",CRC(2e60b2e3) SHA1(0be89fc9b2c6548392febb35c1ace0eb912fc73f),       //7f
                   "potof6.rom",CRC(62b8f74b) SHA1(f82c706b88f49341bab9014bd83371259eb53b47),       //6f
                   "potof5.rom",CRC(5a0537a8) SHA1(26724441d7e2edd7725337b262d95448499151ad))       //4f
DE_ROMEND
#define input_ports_poto input_ports_des11
CORE_GAMEDEF(poto,a32,"Phantom of the Opera, The (3.2)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(poto_a31,"potob5.3-1",CRC(b7be6fa8) SHA1(3ef77daafaf31e2388ac207275aa060f854bd4b9),
                   "potoc5.3-1",CRC(4ce1d254) SHA1(4d24a230ae3a37674cc25ab5ae40c57acbdf5f04))
DE1S_SOUNDROM244(  "potof7.rom",CRC(2e60b2e3) SHA1(0be89fc9b2c6548392febb35c1ace0eb912fc73f),       //7f
                   "potof6.rom",CRC(62b8f74b) SHA1(f82c706b88f49341bab9014bd83371259eb53b47),       //6f
                   "potof5.rom",CRC(5a0537a8) SHA1(26724441d7e2edd7725337b262d95448499151ad))       //4f
DE_ROMEND
CORE_CLONEDEF(poto,a31,a32,"Phantom of the Opera, The (3.1)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(poto_a29,"potob5.2-9",CRC(f01b5510) SHA1(90c632ee74a2dbf877cfe013a69067b1771f1d67),
                   "potoc5.2-9",CRC(c34975b3) SHA1(c9c57126a5da6d78b4066b1d316ffc840660689d))
DE1S_SOUNDROM244(  "potof7.rom",CRC(2e60b2e3) SHA1(0be89fc9b2c6548392febb35c1ace0eb912fc73f),       //7f
                   "potof6.rom",CRC(62b8f74b) SHA1(f82c706b88f49341bab9014bd83371259eb53b47),       //6f
                   "potof5.rom",CRC(5a0537a8) SHA1(26724441d7e2edd7725337b262d95448499151ad))       //4f
DE_ROMEND
CORE_CLONEDEF(poto,a29,a32,"Phantom of the Opera, The (2.9)",1990,"Data East",de_mDEAS1,0)

/*-------------------------------------------------------------------------------
/ King Kong - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/64K Sound Roms
/-------------------------------------------------------------------------------*/
INITGAMES11(kiko,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(kiko_a10,"kkcpu_b5.bin",CRC(97b80fd2) SHA1(a704bda771bd44676a0de2f698a713d10feb01f3),
                   "kkcpu_c5.bin",CRC(d42cab64) SHA1(ca4ceac34384804395b3e3035a430560f194846b))
DE1S_SOUNDROM244(  "kksnd_f7.bin",CRC(fb1b3e11) SHA1(3c9a6958749d7e4dc5a1a57d6683e3cb3dc34890),       //7f
                   "kkvoi_f5.bin",CRC(415f814c) SHA1(27e5b6b7f7ce2e5548ee9bf30966fa4f276bdc4d),       //5f
                   "kkvoi_f4.bin",CRC(bbdc836c) SHA1(825a02b4f058d9dbc387035eb6533547d1766396))       //4f
DE_ROMEND
#define input_ports_kiko input_ports_des11
CORE_GAMEDEF(kiko,a10,"King Kong (1.0)",1990,"Data East",de_mDEAS1,0)

/*--------------------------------------------------------------------------------
/ Back to the Future - CPU Rev 3 /Alpha Type 3 - 32K Roms - 32/64K Sound Roms
/--------------------------------------------------------------------------------*/
INITGAMES11(bttf,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(bttf_a28,"bttfb5.2-8",CRC(a7dafa3c) SHA1(a29b8986d1886aa7bb7dea2521c3d7143ab75320),
                   "bttfc5.2-8",CRC(5dc9928f) SHA1(03de05ed7b04ba86d695f03b1a3d65788faf2d4f))
DE1S_SOUNDROM244(  "bttfsf7.rom",CRC(7673146e) SHA1(d6bd7cf39c78c8aff0b1a0b6cfd46a2a8ce9e086),      //7f
                   "bttfsf6.rom",CRC(468a8d9c) SHA1(713cf84cc5f0531e2e9f7aaa58ebeb53c28ba395),      //6f
                   "bttfsf5.rom",CRC(37a6f6b8) SHA1(ebd603d36527a2af25dcda1fde5cdf9a34d1f9cd))      //4f
DE_ROMEND

#define input_ports_bttf input_ports_des11
CORE_GAMEDEF(bttf,a28,"Back to the Future (2.8)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART88(bttf_a20,"bttfb5.2-0",CRC(c0d4df6b) SHA1(647d0d0a5af04f4255a588da41a6cdb2cf522875),
                   "bttfc5.2-0",CRC(a189a189) SHA1(9669653280c78c811931ea3944817c717f3b5b77))
DE1S_SOUNDROM244(  "bttfsf7.rom",CRC(7673146e) SHA1(d6bd7cf39c78c8aff0b1a0b6cfd46a2a8ce9e086),      //7f
                   "bttfsf6.rom",CRC(468a8d9c) SHA1(713cf84cc5f0531e2e9f7aaa58ebeb53c28ba395),      //6f
                   "bttfsf5.rom",CRC(37a6f6b8) SHA1(ebd603d36527a2af25dcda1fde5cdf9a34d1f9cd))      //4f
DE_ROMEND
CORE_CLONEDEF(bttf,a20,a28,"Back to the Future (2.0)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(bttf_a21,"bktofutr.b5",CRC(a651f867) SHA1(99cff09a06a99abac505c7732bb4ed985f0946e4),
                   "bktofutr.c5",CRC(118ae58e) SHA1(a17e4cc3c12ca770e6e0674cfbeb55482739f735))
DE1S_SOUNDROM244(  "bttfsf7.rom",CRC(7673146e) SHA1(d6bd7cf39c78c8aff0b1a0b6cfd46a2a8ce9e086),      //7f
                   "bttfsf6.rom",CRC(468a8d9c) SHA1(713cf84cc5f0531e2e9f7aaa58ebeb53c28ba395),      //6f
                   "bttfsf5.rom",CRC(37a6f6b8) SHA1(ebd603d36527a2af25dcda1fde5cdf9a34d1f9cd))      //4f
DE_ROMEND
CORE_CLONEDEF(bttf,a21,a28,"Back to the Future (2.1)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(bttf_a27,"bttfb5.2-7",CRC(24b53174) SHA1(00a5e47e70ce4244873980c946479f0bbc414f2e),
                   "bttfc5.2-7",CRC(c4d85d7e) SHA1(88bb91f9ed50335fc402b68983b49319c7dd4e99))
DE1S_SOUNDROM244(  "bttfsf7.rom",CRC(7673146e) SHA1(d6bd7cf39c78c8aff0b1a0b6cfd46a2a8ce9e086),      //7f
                   "bttfsf6.rom",CRC(468a8d9c) SHA1(713cf84cc5f0531e2e9f7aaa58ebeb53c28ba395),      //6f
                   "bttfsf5.rom",CRC(37a6f6b8) SHA1(ebd603d36527a2af25dcda1fde5cdf9a34d1f9cd))      //4f
DE_ROMEND
CORE_CLONEDEF(bttf,a27,a28,"Back to the Future (2.7)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(bttf_g27,"bttfb5g.2-7",CRC(5e3e3cfa) SHA1(2d489c48463c7d28614d56aa566ffbc745bf6a8b),
                   "bttfc5g.2-7",CRC(31dec6d0) SHA1(b0f9323ace3f6d96790be7fe2df67b974c291a29))
DE1S_SOUNDROM244(  "bttfsf7.rom",CRC(7673146e) SHA1(d6bd7cf39c78c8aff0b1a0b6cfd46a2a8ce9e086),      //7f
                   "bttfsf6.rom",CRC(468a8d9c) SHA1(713cf84cc5f0531e2e9f7aaa58ebeb53c28ba395),      //6f
                   "bttfsf5.rom",CRC(37a6f6b8) SHA1(ebd603d36527a2af25dcda1fde5cdf9a34d1f9cd))      //4f
DE_ROMEND
CORE_CLONEDEF(bttf,g27,a28,"Back to the Future (2.7 German)",199?,"Data East",de_mDEAS1,0)

/*------------------------------------------------------------------------
/ The Simpsons - CPU Rev 3 /Alpha Type 3 16/32K Roms - 32/128K Sound Roms
/------------------------------------------------------------------------*/
INITGAMES11(simp,GEN_DE, de_dispAlpha3, FLIP1516, SNDBRD_DE1S, S11_LOWALPHA, 0)
DE_ROMSTART48(simp_a27,"simpb5.2-7",CRC(701c4a4b) SHA1(2a19e2340d119e8813df27a9455aefb599c20a61),
                    "simpc5.2-7",CRC(400a98b2) SHA1(8d11063712dd718ff8badc29586c700208e7442c))
DE1S_SOUNDROM288(   "simpf7.rom",CRC(a36febbc) SHA1(3b96e05f797dd0dc0d4d52544ed995d477991a9f),       //7f
                    "simpf6.rom",CRC(2eb32ed0) SHA1(e7bc3291cb88bf70010865f64496a3ca393257e7),       //6f
                    "simpf5.rom",CRC(bd0671ae) SHA1(b116a23db956a3dd9fc138ec25af250885ba4ef5))       //4f
DE_ROMEND
#define input_ports_simp input_ports_des11
CORE_GAMEDEF(simp,a27,"Simpsons, The (2.7)",1990,"Data East",de_mDEAS1,0)

DE_ROMSTART48(simp_a20,"simpa2-0.b5",CRC(e67038d1) SHA1(f3eae2ed45caca97a1eb53d847366c52ea68bbee),
                   "simpa2-0.c5",CRC(43662bc3) SHA1(d8171a5c083eb8bffa61353b74db6b3ebab96923))
DE1S_SOUNDROM288(   "simpf7.rom",CRC(a36febbc) SHA1(3b96e05f797dd0dc0d4d52544ed995d477991a9f),       //7f
                    "simpf6.rom",CRC(2eb32ed0) SHA1(e7bc3291cb88bf70010865f64496a3ca393257e7),       //6f
                    "simpf5.rom",CRC(bd0671ae) SHA1(b116a23db956a3dd9fc138ec25af250885ba4ef5))       //4f
DE_ROMEND
CORE_CLONEDEF(simp,a20,a27,"Simpsons, The (2.0)",1990,"Data East",de_mDEAS1,0)

/***********************************************************************/
/*************** GAMES USING 128X16 DMD DISPLAY ************************/
/***********************************************************************/
/*------------------------------------------------------------
/ Checkpoint - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAMES11(ckpt,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE1S, SNDBRD_DEDMD16, 0)
DE_ROMSTART48(ckpt_a17,"chkpntb5.107",CRC(9fbae8e3) SHA1(a25b9dcba2a3f84394972bf36930c0f0344eccbd),
                     "chkpntc5.107",CRC(082dc283) SHA1(cc3038e0999d2c403fe1863e649b8029376b0387))
DE_DMD16ROM1(        "chkpntds.512",CRC(14d9c6d6) SHA1(5470a4ebe7bc4a056f75aa1fffe3a4e3e24457c6))
DE1S_SOUNDROM288(    "chkpntf7.rom",CRC(e6f6d716) SHA1(a034eb94acb174f7dbe192a55cfd00715ca85a75),      //7f
                     "chkpntf6.rom",CRC(2d08043e) SHA1(476c9945354e733bfc9a854760ca8cfa3bc62294),      //6f
                     "chkpntf5.rom",CRC(167daa2c) SHA1(458781726c73a09da2b8e8313e1d359cb795a744))      //4f
DE_ROMEND
#define input_ports_ckpt input_ports_des11
CORE_GAMEDEF(ckpt,a17,"Checkpoint (1.7)",1991,"Data East",de_mDEDMD16S1,0)


/*-----------------------------------------------------------------------------
/ Teenage Mutant Ninja Turtles - CPU Rev 3 /DMD Type 1 64K Rom 16/32K CPU Roms
/-----------------------------------------------------------------------------*/
INITGAMES11(tmnt,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE1S, SNDBRD_DEDMD16, 0)
DE_ROMSTART48(tmnt_104,"tmntb5a.104",CRC(f508eeee) SHA1(5e67fde49f6e7d5d563645df9036d5691be076cf),
                   "tmntc5a.104",CRC(a33d18d4) SHA1(41cf815c1f3d117efe0ddd14ad84076dcb80318a))
DE_DMD16ROM1(      "tmntdsp.104",CRC(545686b7) SHA1(713df7820d024db3406f5e171f62a53e34474f70))
DE1S_SOUNDROM288(  "tmntf7.rom",CRC(59ba0153) SHA1(e7b02a656c67a0d866020a60ee90e30bef77f67f),        //7f
                   "tmntf6.rom",CRC(5668d45a) SHA1(65766cb47791ec0a2243015d487f1156a2819fe6),        //6f
                   "tmntf4.rom",CRC(6c38cd84) SHA1(bbe8797fe1622cb8f0842c4d7159760fed080880))        //4f
DE_ROMEND
#define input_ports_tmnt input_ports_des11
CORE_GAMEDEF(tmnt,104,"Teenage Mutant Ninja Turtles (1.04)",1991,"Data East",de_mDEDMD16S1,0)

DE_ROMSTART48(tmnt_104g,"tmntb5a.104",CRC(f508eeee) SHA1(5e67fde49f6e7d5d563645df9036d5691be076cf),
                  "tmntc5g.104",CRC(d7f2fd8b) SHA1(b80f6201ca2981ec4a3869688963884948a6bd72))
DE_DMD16ROM1(     "tmntdsp.104",CRC(545686b7) SHA1(713df7820d024db3406f5e171f62a53e34474f70))
DE1S_SOUNDROM288(  "tmntf7.rom",CRC(59ba0153) SHA1(e7b02a656c67a0d866020a60ee90e30bef77f67f),        //7f
                   "tmntf6.rom",CRC(5668d45a) SHA1(65766cb47791ec0a2243015d487f1156a2819fe6),        //6f
                   "tmntf4.rom",CRC(6c38cd84) SHA1(bbe8797fe1622cb8f0842c4d7159760fed080880))        //4f
DE_ROMEND
CORE_CLONEDEF(tmnt,104g,104,"Teenage Mutant Ninja Turtles (1.04 German)",1991,"Data East",de_mDEDMD16S1,0)

DE_ROMSTART48(tmnt_103,"tmntb5.103",CRC(fcc6c5b0) SHA1(062bbc93de0f8bb1921da4d756a13923f23cf5d9),
                   "tmntc5.103",CRC(46b68ecc) SHA1(cb94041017c0856f1e15de05c70369cb4f8756cd))
DE_DMD16ROM1(     "tmntdsp.104",CRC(545686b7) SHA1(713df7820d024db3406f5e171f62a53e34474f70))
DE1S_SOUNDROM288(  "tmntf7.rom",CRC(59ba0153) SHA1(e7b02a656c67a0d866020a60ee90e30bef77f67f),        //7f
                   "tmntf6.rom",CRC(5668d45a) SHA1(65766cb47791ec0a2243015d487f1156a2819fe6),        //6f
                   "tmntf4.rom",CRC(6c38cd84) SHA1(bbe8797fe1622cb8f0842c4d7159760fed080880))        //4f
DE_ROMEND
CORE_CLONEDEF(tmnt,103,104,"Teenage Mutant Ninja Turtles (1.03)",1991,"Data East",de_mDEDMD16S1,0)

/***************************************************************************/
/** ALL FOLLOWING GAMES BELOW STARTED USING NEW SOUND BOARD WITH BSMT2000 **/
/***************************************************************************/

/*-------------------------------------------------------------
/ Batman - CPU Rev 3 /DMD Type 1 128K Rom 16/32K CPU Roms
/------------------------------------------------------------*/
INITGAMES11(btmn,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD16, 0)
DE_ROMSTART48(btmn_103,"batcpub5.103",CRC(6f160581) SHA1(0f2d6c396324fbf116309a872cf95d9a05446cea),
                    "batcpuc5.103",CRC(8588c5a8) SHA1(41b159c9e4ca523b37f0b893e57f166c85e812e9))
DE_DMD16ROM2(       "batdsp.102",CRC(4c4120e7) SHA1(ba7d78c933f6709b3db4efcca5e7bb9099074550))
DE2S_SOUNDROM021(   "batman.u7" ,CRC(b2e88bf5) SHA1(28f814ea73f8eefd1bb5499a599e67a6850c92c0),
                    "batman.u17" ,CRC(b84914dd) SHA1(333d88033428705cbd0a40d70d938c0021bb0015),
                    "batman.u21" ,CRC(42dab6ac) SHA1(facf993db2ce240c9e825ca9a21ac65a0fbba188))
DE_ROMEND
#define input_ports_btmn input_ports_des11
CORE_GAMEDEF(btmn,103,"Batman (1.03)",1991,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTART88(btmn_101,"batcpub5.101",CRC(a7f5754e) SHA1(2c24cab4cc5f1e05539d2843a49b4b1a8d507630),
                    "batcpuc5.101",CRC(1fcb85ca) SHA1(daf1e1297975b9b577c796d50b973885f925508e))
DE_DMD16ROM2(       "batdsp.102",CRC(4c4120e7) SHA1(ba7d78c933f6709b3db4efcca5e7bb9099074550))
DE2S_SOUNDROM021(   "batman.u7" ,CRC(b2e88bf5) SHA1(28f814ea73f8eefd1bb5499a599e67a6850c92c0),
                    "batman.u17" ,CRC(b84914dd) SHA1(333d88033428705cbd0a40d70d938c0021bb0015),
                    "batman.u21" ,CRC(42dab6ac) SHA1(facf993db2ce240c9e825ca9a21ac65a0fbba188))
DE_ROMEND
CORE_CLONEDEF(btmn,101,103,"Batman (1.01)",1991,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTART48(btmn_f13,"batcpub5.103",CRC(6f160581) SHA1(0f2d6c396324fbf116309a872cf95d9a05446cea),
                    "batccpuf.103",CRC(6f654fb4) SHA1(4901326f92aab1f5a2cdf9032511bef8b197f7e4))
DE_DMD16ROM2(       "bat_dspf.103",CRC(747be2e6) SHA1(47ac64b91eabc24be57e376035ef8da95259587d))
DE2S_SOUNDROM021(   "batman.u7" ,CRC(b2e88bf5) SHA1(28f814ea73f8eefd1bb5499a599e67a6850c92c0),
                    "batman.u17" ,CRC(b84914dd) SHA1(333d88033428705cbd0a40d70d938c0021bb0015),
                    "batman.u21" ,CRC(42dab6ac) SHA1(facf993db2ce240c9e825ca9a21ac65a0fbba188))
DE_ROMEND
CORE_CLONEDEF(btmn,f13,103,"Batman (1.03 French)",1991,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTART48(btmn_g13,"batcpub5.103",CRC(6f160581) SHA1(0f2d6c396324fbf116309a872cf95d9a05446cea),
                    "batccpug.103",CRC(a199ab0f) SHA1(729dab10fee708a18b7be5a2b9b904aa211b233a))
DE_DMD16ROM2(       "bat_dspg.104",CRC(1581819f) SHA1(88facfad2e74dd44b71fd19df685a4c2378d26de))
DE2S_SOUNDROM021(   "batman.u7" ,CRC(b2e88bf5) SHA1(28f814ea73f8eefd1bb5499a599e67a6850c92c0),
                    "batman.u17" ,CRC(b84914dd) SHA1(333d88033428705cbd0a40d70d938c0021bb0015),
                    "batman.u21" ,CRC(42dab6ac) SHA1(facf993db2ce240c9e825ca9a21ac65a0fbba188))
DE_ROMEND
CORE_CLONEDEF(btmn,g13,103,"Batman (1.03 German)",1991,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTART48(btmn_106,"b5_a106.128",CRC(5aa7fbe3) SHA1(587be4fd18ad730e675e720923e00d1775a4560e),
                    "c5_a106.256",CRC(79e86ccd) SHA1(430ac436bd1c8841950986af80747285a7d25942))
DE_DMD16ROM2(       "batdsp.106",CRC(4c4120e7) SHA1(ba7d78c933f6709b3db4efcca5e7bb9099074550))
DE2S_SOUNDROM021(   "batman.u7" ,CRC(b2e88bf5) SHA1(28f814ea73f8eefd1bb5499a599e67a6850c92c0),
                    "batman.u17" ,CRC(b84914dd) SHA1(333d88033428705cbd0a40d70d938c0021bb0015),
                    "batman.u21" ,CRC(42dab6ac) SHA1(facf993db2ce240c9e825ca9a21ac65a0fbba188))
DE_ROMEND
CORE_CLONEDEF(btmn,106,103,"Batman (1.06)",1991,"Data East",de_mDEDMD16S2A,0)

/*-------------------------------------------------------------
/ Star Trek - CPU Rev 3 /DMD Type 1 128K Rom - 64K CPU Rom
/------------------------------------------------------------*/
#define sChase1   CORE_CUSTSOLNO(1) /* 33 */
#define sChase2   CORE_CUSTSOLNO(2) /* 34 */
#define sChase3   CORE_CUSTSOLNO(3) /* 35 */

/*-- return status of custom solenoids --*/
static int trek_getSol(int solNo) {
  if ((solNo == sChase1) || (solNo == sChase2) || (solNo == sChase3)) {
    return core_getSol(CORE_FIRSTEXTSOL + solNo - CORE_FIRSTCUSTSOL);
  }
  return 0;
}

static core_tGameData trekGameData = {
  GEN_DEDMD16, de_128x16DMD,
  { FLIP1516, 0,0,3, //We need 3 custom solenoids!
    SNDBRD_DE2S,SNDBRD_DEDMD16,S11_PRINTERLINE,0, trek_getSol
  }, NULL, {{0}}, {10}
};
static void init_trek(void) { core_gameData = &trekGameData; }
DE_ROMSTARTx0(trek_201,"trekcpuu.201",CRC(ea0681fe) SHA1(282c8181e60da6358ef320358575a538aa4abe8c))
DE_DMD16ROM2(   "trekdspa.109",CRC(a7e7d44d) SHA1(d26126310b8b316ca161d4202645de8fb6359822))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
#define input_ports_trek input_ports_des11
CORE_GAMEDEF(trek,201,"Star Trek 25th Anniversary (2.01)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_200,"trekcpuu.200",CRC(4528e803) SHA1(0ebb16ab8b95f04a19fa4510e58c01493393d48c))
DE_DMD16ROM2(   "trekdspa.109",CRC(a7e7d44d) SHA1(d26126310b8b316ca161d4202645de8fb6359822))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,200,201,"Star Trek 25th Anniversary (2.00)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_120,"trekcpu.120",CRC(2cac0731) SHA1(abf68c358c50bdeb36714cca0a9848e398a6f9fc))
DE_DMD16ROM2(    "trekdsp.106",CRC(dc3bf312) SHA1(3262d6604d1dcd1dc738bc3f919a3319b783fd73))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,120,201,"Star Trek 25th Anniversary (1.20)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_117,"trekcpu.117",CRC(534ebb09) SHA1(96f343fcc7b0f39e0a8ec7df47cea433ad2c9119))
DE_DMD16ROM2(   "trekdspa.109",CRC(a7e7d44d) SHA1(d26126310b8b316ca161d4202645de8fb6359822))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,117,201,"Star Trek 25th Anniversary (1.17)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_110,"trekcpu.110",CRC(06e0f87b) SHA1(989d70e067cd322351768550549a4e2c8923132c))
DE_DMD16ROM2(    "trekdsp.106",CRC(dc3bf312) SHA1(3262d6604d1dcd1dc738bc3f919a3319b783fd73))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,110,201,"Star Trek 25th Anniversary (1.10)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_11a,"trekcpu.110",CRC(06e0f87b) SHA1(989d70e067cd322351768550549a4e2c8923132c))
DE_DMD16ROM2(   "trekadsp.bin",CRC(54681627) SHA1(4251fa0568d2e869b44358471a3d4a4e88443954))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,11a,201,"Star Trek 25th Anniversary (1.10, Alpha Display)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(trek_300,"trekcpuu.300",CRC(3859939e) SHA1(a453208a7640a39cb96947e8d3d3b2f71f85e613))
DE_DMD16ROM2(   "trekdspa.300",CRC(d312f92e) SHA1(eebcc697b89bbe62b0450fdec6226a8396308f37))
DE2S_SOUNDROM022(  "trek.u7"  ,CRC(f137abbb) SHA1(11731170ed4f04dd8af05d8f79ad727b0e0104d7),
                   "trek.u17" ,CRC(531545da) SHA1(905f34173db0e04eaf5236191186ea209b8a0a34),
                   "trek.u21" ,CRC(6107b004) SHA1(1f9bed9b06d5b19fbc0cc0bef2e493eb1a3f1aa4))
DE_ROMEND
CORE_CLONEDEF(trek,300,201,"Star Trek 25th Anniversary (3.00 unofficial MOD)",2020,"Data East",de_mDEDMD16S2A,0)

/*-------------------------------------------------------------
/ Hook - CPU Rev 3 /DMD  Type 1 128K Rom - CPU Rom
/------------------------------------------------------------*/
INITGAMES11(hook,GEN_DEDMD16, de_128x16DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD16, 0)
DE_ROMSTARTx0(hook_408,"hokcpua.408",CRC(46477fc7) SHA1(ce6228fd9ab4b6c774e128d291f50695746da358))
DE_DMD16ROM2(       "hokdspa.401",CRC(59a07eb5) SHA1(d1ca41ce417f1772fe4da1eb37077f924b66ad36))
DE2S_SOUNDROM022(   "hooksnd.u7" ,CRC(642f45b3) SHA1(a4b2084f32e52a596547384906281d04424332fc),
                    "hook-voi.u17" ,CRC(6ea9fcd2) SHA1(bffc66df542e06dedddaa403b5513446d9d6fc8c),
                    "hook-voi.u21" ,CRC(b5c275e2) SHA1(ff51c2007132a1310ac53b5ab2a4af7d0ab15948))
DE_ROMEND
#define input_ports_hook input_ports_des11
CORE_GAMEDEF(hook,408,"Hook (4.08)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(hook_500,"hokcpua.500",CRC(93e25b12) SHA1(5ec37ad3358f1f093be771f5978f1ad8a7a7c61a))
DE_DMD16ROM2(       "hokdspa.500",CRC(ed6a134b) SHA1(775975b629312f7f1151d878d40723ad4a79928c))
DE2S_SOUNDROM022(   "hooksnd.u7" ,CRC(642f45b3) SHA1(a4b2084f32e52a596547384906281d04424332fc),
                    "hook-voi.u17" ,CRC(6ea9fcd2) SHA1(bffc66df542e06dedddaa403b5513446d9d6fc8c),
                    "hook-voi.u21" ,CRC(b5c275e2) SHA1(ff51c2007132a1310ac53b5ab2a4af7d0ab15948))
DE_ROMEND
CORE_CLONEDEF(hook,500,408,"Hook (5.00 unofficial MOD)",2016,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(hook_501,"hokcpua.501",CRC(9aa504a5) SHA1(fa5d09b513916f7f738b88eb83cc1e9e62c7864a))
DE_DMD16ROM2(       "hokdspa.500",CRC(ed6a134b) SHA1(775975b629312f7f1151d878d40723ad4a79928c))
DE2S_SOUNDROM022(   "hooksnd.u7" ,CRC(642f45b3) SHA1(a4b2084f32e52a596547384906281d04424332fc),
                    "hook-voi.u17" ,CRC(6ea9fcd2) SHA1(bffc66df542e06dedddaa403b5513446d9d6fc8c),
                    "hook-voi.u21" ,CRC(b5c275e2) SHA1(ff51c2007132a1310ac53b5ab2a4af7d0ab15948))
DE_ROMEND
CORE_CLONEDEF(hook,501,408,"Hook (5.01 unofficial MOD)",2018,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(hook_401p,"hokcpua.401",CRC(20223298) SHA1(a8063765db947b059eadaad6654ed0c5cad9198d))
DE_DMD16ROM2(       "hokdspa.401",CRC(59a07eb5) SHA1(d1ca41ce417f1772fe4da1eb37077f924b66ad36))
DE2S_SOUNDROM022(   "hooksnd_p.u7" ,CRC(20091293) SHA1(fdfc4eadef0bf1915c7c72c1fd8dafaa429b3c44),
                    "hook-voi_p.u17" ,CRC(667cf0fb) SHA1(dd12a7fa280384381ebc5c3d8add652eddb294fb),
                    "hook-voi_p.u21" ,CRC(04775416) SHA1(5675aea39b76178ff476b0f627223a1c75a3d6b7))
DE_ROMEND
CORE_CLONEDEF(hook,401p,408,"Hook (4.01 with Prototype Sound)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(hook_401,"hokcpua.401",CRC(20223298) SHA1(a8063765db947b059eadaad6654ed0c5cad9198d))
DE_DMD16ROM2(       "hokdspa.401",CRC(59a07eb5) SHA1(d1ca41ce417f1772fe4da1eb37077f924b66ad36))
DE2S_SOUNDROM022(   "hooksnd.u7" ,CRC(642f45b3) SHA1(a4b2084f32e52a596547384906281d04424332fc),
                    "hook-voi.u17" ,CRC(6ea9fcd2) SHA1(bffc66df542e06dedddaa403b5513446d9d6fc8c),
                    "hook-voi.u21" ,CRC(b5c275e2) SHA1(ff51c2007132a1310ac53b5ab2a4af7d0ab15948))
DE_ROMEND
CORE_CLONEDEF(hook,401,408,"Hook (4.01)",1992,"Data East",de_mDEDMD16S2A,0)

DE_ROMSTARTx0(hook_404,"hokcpua.404",CRC(53357d8b) SHA1(4e8f5f4376418fbac782065c602da82acab06ef3))
DE_DMD16ROM2(       "hokdspa.401",CRC(59a07eb5) SHA1(d1ca41ce417f1772fe4da1eb37077f924b66ad36))
DE2S_SOUNDROM022(   "hooksnd.u7" ,CRC(642f45b3) SHA1(a4b2084f32e52a596547384906281d04424332fc),
                    "hook-voi.u17" ,CRC(6ea9fcd2) SHA1(bffc66df542e06dedddaa403b5513446d9d6fc8c),
                    "hook-voi.u21" ,CRC(b5c275e2) SHA1(ff51c2007132a1310ac53b5ab2a4af7d0ab15948))
DE_ROMEND
CORE_CLONEDEF(hook,404,408,"Hook (4.04)",1992,"Data East",de_mDEDMD16S2A,0)


/***********************************************************************/
/*************** GAMES USING 128X32 DMD DISPLAY ************************/
/***********************************************************************/
/*----------------------------------------------------------------
/ Lethal Weapon 3 - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/---------------------------------------------------------------*/
INITGAMES11(lw3,GEN_DEDMD32, de_128x32DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, 0)
DE_ROMSTARTx0(lw3_208,"lw3cpuu.208",CRC(a3041f8a) SHA1(3c5b8525b8e9b924590648429c56aaf97adee460))
DE_DMD32ROM44(     "lw3drom1.a26",CRC(44a4cf81) SHA1(c7f3e3d5fbe930650e48423c8ba0ac484ce0640c),
                   "lw3drom0.a26",CRC(22932ed5) SHA1(395aa376cd8562de7956a6e34b8747e7cf81f935))
DE2S_SOUNDROM022(  "lw3u7.dat"   ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17.dat"  ,CRC(e34cf2fc) SHA1(417c83ded6637f891c8bb42b32d6898c90a0e5cf),
                   "lw3u21.dat"  ,CRC(82bed051) SHA1(49ddc4190762d9b473fda270e0d6d88a4422d5d7))
DE_ROMEND
#define input_ports_lw3 input_ports_des11
CORE_GAMEDEF(lw3,208,"Lethal Weapon 3 (2.08)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(lw3_208p,"lw3cpuu.208",CRC(a3041f8a) SHA1(3c5b8525b8e9b924590648429c56aaf97adee460))
DE_DMD32ROM44(     "lw3drom1.a26",CRC(44a4cf81) SHA1(c7f3e3d5fbe930650e48423c8ba0ac484ce0640c),
                   "lw3drom0.a26",CRC(22932ed5) SHA1(395aa376cd8562de7956a6e34b8747e7cf81f935))
DE2S_SOUNDROM022(  "lw3u7.dat"   ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17_vm.dat",CRC(5168dbbd) SHA1(e5f91650e613350c542ac93d0d4be64b25333186),
                   "lw3u21_vm.dat",CRC(7ec96750) SHA1(13e41833d43396e370b817928618f72f928d9ba0))
DE_ROMEND
CORE_CLONEDEF(lw3,208p,208,"Lethal Weapon 3 (2.08p, Voices Mod)",2013,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(lw3_207,"lw3gc5.207",CRC(27aeaea9) SHA1(f8c40cbc37edac20187ac880be281dd45d8ad614))
DE_DMD32ROM44(     "lw3drom1.a26",CRC(44a4cf81) SHA1(c7f3e3d5fbe930650e48423c8ba0ac484ce0640c),
                   "lw3drom0.a26",CRC(22932ed5) SHA1(395aa376cd8562de7956a6e34b8747e7cf81f935))
DE2S_SOUNDROM022(  "lw3u7.dat"  ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17.dat" ,CRC(e34cf2fc) SHA1(417c83ded6637f891c8bb42b32d6898c90a0e5cf),
                   "lw3u21.dat" ,CRC(82bed051) SHA1(49ddc4190762d9b473fda270e0d6d88a4422d5d7))
DE_ROMEND
CORE_CLONEDEF(lw3,207,208,"Lethal Weapon 3 (2.07 Canadian)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(lw3_205,"lw3gc5.205",CRC(5ad8ff4a) SHA1(6a01a2195543c0c57ce4ce78703c91500835a2da))
DE_DMD32ROM44(     "lw3dsp1.205",CRC(9dfeffb4) SHA1(f62f2a884da68b4dbfe7da071058dc8cd1766c36),
                   "lw3dsp0.205",CRC(bd8156f1) SHA1(b18214af1b79cca79bdc634c175c3bf7d0052843))
DE2S_SOUNDROM022(  "lw3u7.dat"  ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17.dat" ,CRC(e34cf2fc) SHA1(417c83ded6637f891c8bb42b32d6898c90a0e5cf),
                   "lw3u21.dat" ,CRC(82bed051) SHA1(49ddc4190762d9b473fda270e0d6d88a4422d5d7))
DE_ROMEND
CORE_CLONEDEF(lw3,205,208,"Lethal Weapon 3 (2.05)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(lw3_203,"lw3cpuu.203",CRC(0cfa38d4) SHA1(11d2e101a574c2dfec49ec701f480173b84c842e))
DE_DMD32ROM44(     "lw3dsp1.204",CRC(1ba79363) SHA1(46d489a1190533c73370acd8a48cef60d12f87ce),
                   "lw3dsp0.204",CRC(c74d3cf2) SHA1(076ee9b2e3cad0b8058ac0c70f5ffe7e29f3eff5))
DE2S_SOUNDROM022(  "lw3u7.dat"  ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17.dat" ,CRC(e34cf2fc) SHA1(417c83ded6637f891c8bb42b32d6898c90a0e5cf),
                   "lw3u21.dat" ,CRC(82bed051) SHA1(49ddc4190762d9b473fda270e0d6d88a4422d5d7))
DE_ROMEND
CORE_CLONEDEF(lw3,203,208,"Lethal Weapon 3 (2.03)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(lw3_200,"lw3cpu.200",CRC(ddb6e7a7) SHA1(d48309e1984ef9a7682dfde190cf457632044657))
DE_DMD32ROM44(     "lw3dsp1.204",CRC(1ba79363) SHA1(46d489a1190533c73370acd8a48cef60d12f87ce),
                   "lw3dsp0.204",CRC(c74d3cf2) SHA1(076ee9b2e3cad0b8058ac0c70f5ffe7e29f3eff5))
DE2S_SOUNDROM022(  "lw3u7.dat"  ,CRC(ba845ac3) SHA1(bb50413ace1885870cb3817edae478904b0eefb8),
                   "lw3u17.dat" ,CRC(e34cf2fc) SHA1(417c83ded6637f891c8bb42b32d6898c90a0e5cf),
                   "lw3u21.dat" ,CRC(82bed051) SHA1(49ddc4190762d9b473fda270e0d6d88a4422d5d7))
DE_ROMEND
CORE_CLONEDEF(lw3,200,208,"Lethal Weapon 3 (2.00)",1992,"Data East",de_mDEDMD32S2A_hack,0)

/*-------------------------------------------------------------
/ Aaron Spelling - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(aar,GEN_DEDMD32, de_128x32DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, 0)
DE_ROMSTARTx0(aar_101,"as512cpu.bin",CRC(03c70e67) SHA1(3093e217943ae80c842a1d893cff5330ac90bc30))
DE_DMD32ROM44(        "asdspu12.bin",CRC(5dd81be6) SHA1(20e5ec19550e3795670c5ee4e8e92fae0499fdb8),
                      "asdspu14.bin",CRC(3f2204ca) SHA1(69523d6c5555d391ab24912f4c4c78aa09a400c1))
DE2S_SOUNDROM144(     "assndu7.bin" ,CRC(f0414a0d) SHA1(b1f940be05426a39f4e5ea0802fd03a7ce055ebc),
                      "assndu17.bin",CRC(e151b1fe) SHA1(d7d97499d93885a4f7ebd7bb302731bc5bc456ff),
                      "assndu21.bin",CRC(7d69e917) SHA1(73e21e65bc194c063933288cb617127b41593466))
DE_ROMEND
#define input_ports_aar input_ports_des11
CORE_GAMEDEF(aar,101,"Aaron Spelling (1.01)",1992,"Data East",de_mDEDMD32S2A_hack,0)

/*-------------------------------------------------------------
/ Star Wars - CPU Rev 3 /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(stwr,GEN_DEDMD32, de_128x32DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, 0)
DE_ROMSTARTx0(stwr_104,"starcpua.104",CRC(12b87cfa) SHA1(12e0ab52f6784beefce8291d29b8aff01b2f2818))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
#define input_ports_stwr input_ports_des11
CORE_GAMEDEF(stwr,104,"Star Wars (1.04 20th Anniversary)",2012,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_106,"starcpua.106",CRC(35d3cfd9) SHA1(14d8960f3657d7cd977b0a749e995aadb3fd4c7c))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,106,104,"Star Wars (1.06 20th Anniversary)",2016,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_107s,"starcpua.107",CRC(1a801b7e) SHA1(fef567126dff87a2cb31401b029c3050438072b7))
DE_DMD32ROM8x(        "sw4mrom.s15",CRC(158867b9) SHA1(45a0f4d26c21e2259aeb2a726a1eac23744213a2))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,107s,104,"Star Wars (1.07 20th Anniversary, Spanish)",2016,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_107,"starcpua.107",CRC(1a801b7e) SHA1(fef567126dff87a2cb31401b029c3050438072b7))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,107,104,"Star Wars (1.07 20th Anniversary)",2016,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_103,"starcpua.103",CRC(318085ca) SHA1(7c35bdee52e8093fe05f0624615baabe559a1917))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,103,104,"Star Wars (1.03, Display 1.05)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_a14,"starcpua.103",CRC(318085ca) SHA1(7c35bdee52e8093fe05f0624615baabe559a1917))
DE_DMD32ROM44(        "swrom1.a14", CRC(4d577828) SHA1(8b1f302621fe2ee13a067b9c97e3dc33f4519cea),
                      "swrom0.a14", CRC(104e5a6b) SHA1(b6a9e32f8aec078665faf2ba9ba4f9f51f68cea8))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,a14,104,"Star Wars (1.03, Display 1.04)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_a046,"starcpua.103",CRC(318085ca) SHA1(7c35bdee52e8093fe05f0624615baabe559a1917))
DE_DMD32ROM44(        "swrom1.a046",CRC(5ceac219) SHA1(76b7acf378f83bacf6c4adb020d6e544eacbac7a),
                      "swrom0.a046",CRC(305e45be) SHA1(fbdc90175467a9ee59dc11c5ccbe83130b3644c8))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,a046,104,"Star Wars (1.03, Display A0.46)",1992,"Data East",de_mDEDMD32S2A_hack,0)

/* USA CPU 1.02 (11/20/92) */
DE_ROMSTARTx0(stwr_102,"starcpua.102",CRC(8b9d90d6) SHA1(2fb7594e6f4aae1dc3a07192546fabd2901acbed))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,102,104,"Star Wars (1.02)",1992,"Data East",de_mDEDMD32S2A_hack,0)

/* England CPU 1.02 (11/20/92) */
DE_ROMSTARTx0(stwr_e12,"starcpue.102",CRC(b441abd3) SHA1(42cab6e16be8e25a68b2db30f53ba516bbb8741d))
DE_DMD32ROM8x(        "sw4mrom.a15",CRC(00c87952) SHA1(cd2f491f03fcb3e3ceff7ee7f678aa1957a5d14b))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,e12,104,"Star Wars (1.02 English)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_101,"starcpu.101",CRC(6efc7b14) SHA1(f669669fbd8733d06b386ea352fdb2041bf98362))
DE_DMD32ROM44(        "stardisp_u14.102", CRC(f8087364) SHA1(4cd66b72cf430018cfb7ac8306b96a8499d41896),
                      "stardisp_u12.102", CRC(fde126c6) SHA1(0a3eacfd4589ee0f26c4212ba9948dff061f3338))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,101,104,"Star Wars (1.01)",1992,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(stwr_g11,"starcpug.101",CRC(c74b4576) SHA1(67db9294cd802be8d62102fe756648f750821960))
DE_DMD32ROM8x(        "swdsp_g.102",CRC(afdfbfc4) SHA1(1c3cd90b9cd4f88ee2b556abef863a0ae9a10056))
DE2S_SOUNDROM042(     "s-wars.u7"  ,CRC(cefa19d5) SHA1(7ddf9cc85ab601514305bc46083a07a3d087b286),
                      "s-wars.u17" ,CRC(7950a147) SHA1(f5bcd5cf6b35f9e4f14d62b084495c3a743d92a1),
                      "s-wars.u21" ,CRC(7b08fdf1) SHA1(489d21a10e97e886f948d81dedd7f8de3acecd2b))
DE_ROMEND
CORE_CLONEDEF(stwr,g11,104,"Star Wars (1.01 German)",1992,"Data East",de_mDEDMD32S2A_hack,0)

/*-------------------------------------------------------------
/ The Adventures of Rocky and Bullwinkle and Friends - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(rab,GEN_DEDMD32, de_128x32DMD, FLIP1516, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(rab_320,"rabcpua.320",CRC(21a2d518) SHA1(42123dca519034ecb740e5cb493b1b0b6b44e3be))
DE_DMD32ROM8x(     "rbdspa.300",CRC(a5dc2f72) SHA1(60bbb4914ff56ad48c86c3550e094a3d9d70c700))
DE2S_SOUNDROM142(  "rab.u7"  ,CRC(b232e630) SHA1(880fffc395d7c24bdea4e7e8000afba7ea71c094),
                   "rab.u17" ,CRC(7f2b53b8) SHA1(fd4f4ed1ed343069ffc534fe4b20026fe7403220),
                   "rab.u21" ,CRC(3de1b375) SHA1(a48bb80483ca03cd7c3bf0b5f2930a6ee9cc448d))
DE_ROMEND
#define input_ports_rab input_ports_des11
CORE_GAMEDEF(rab,320,"Adventures of Rocky and Bullwinkle and Friends, The (3.20)",1993,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(rab_130,"rabcpua.130",CRC(f59b1a53) SHA1(046cd0eaee6e646286f3dfa73eeacfd93c2be273))
DE_DMD32ROM8x(     "rbdspa.130",CRC(b6e2176e) SHA1(9ccbb30dc0f386fcf5e5255c9f80c720e601565f))
DE2S_SOUNDROM142(  "rab.u7"  ,CRC(b232e630) SHA1(880fffc395d7c24bdea4e7e8000afba7ea71c094),
                   "rab.u17" ,CRC(7f2b53b8) SHA1(fd4f4ed1ed343069ffc534fe4b20026fe7403220),
                   "rab.u21" ,CRC(3de1b375) SHA1(a48bb80483ca03cd7c3bf0b5f2930a6ee9cc448d))
DE_ROMEND
CORE_CLONEDEF(rab,130,320,"Adventures of Rocky and Bullwinkle and Friends, The (1.30)",1993,"Data East",de_mDEDMD32S2A_hack,0)

DE_ROMSTARTx0(rab_103,"rabcpu.103",CRC(d5fe3184) SHA1(dc1ca938f15240d1c15ee5724d29a3538418f8de))
DE_DMD32ROM8x(     "rabdspsp.103",CRC(02624948) SHA1(069ef69d6ce193d73954935b378230c05b83b8fc))
DE2S_SOUNDROM142(  "rab.u7"  ,CRC(b232e630) SHA1(880fffc395d7c24bdea4e7e8000afba7ea71c094),
                   "rab.u17" ,CRC(7f2b53b8) SHA1(fd4f4ed1ed343069ffc534fe4b20026fe7403220),
                   "rab.u21" ,CRC(3de1b375) SHA1(a48bb80483ca03cd7c3bf0b5f2930a6ee9cc448d))
DE_ROMEND
CORE_CLONEDEF(rab,103,320,"Adventures of Rocky and Bullwinkle and Friends, The (1.03 Spanish)",1993,"Data East",de_mDEDMD32S2A_hack,0)

/*-------------------------------------------------------------
/ Jurassic Park - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(jupk,GEN_DEDMD32, de_128x32DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(jupk_513,"jpcpua.513",CRC(9f70a937) SHA1(cdea6c6e852982eb5e800db138f7660d51b6fdc8))
DE_DMD32ROM8x(         "jpdspa.510",CRC(9ca61e3c) SHA1(38ae472f38e6fc33671e9a276313208e5ccd8640))
DE2S_SOUNDROM142(      "jpu7.dat"  ,CRC(f3afcf13) SHA1(64e12f9d42c00ae08a4584b2ebea475566b90c13),
                       "jpu17.dat" ,CRC(38135a23) SHA1(7c284c17783269824a3d3e83c4cd8ead27133309),
                       "jpu21.dat" ,CRC(6ac1554c) SHA1(9a91ce836c089f96ad9c809bb66fcddda1f3e456))
DE_ROMEND
#define input_ports_jupk input_ports_des11
CORE_GAMEDEF(jupk,513,"Jurassic Park (5.13)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(jupk_600, "jpcpua.600",CRC(b1cfc259) SHA1(a856531abf7e94b961a84bde340e8d3f33a5a6a3))
DE_DMD32ROM8x(          "jpdspa.600",CRC(b591b297) SHA1(83bd9dc08320ed4bde862a5cd63f30f0367c446e))
DE2S_SOUNDROM142(       "jpu7.dat"  ,CRC(f3afcf13) SHA1(64e12f9d42c00ae08a4584b2ebea475566b90c13),
                        "jpu17.dat" ,CRC(38135a23) SHA1(7c284c17783269824a3d3e83c4cd8ead27133309),
                        "jpu21.dat" ,CRC(6ac1554c) SHA1(9a91ce836c089f96ad9c809bb66fcddda1f3e456))
DE_ROMEND
CORE_CLONEDEF(jupk,600,513,"Jurassic Park (6.00 unofficial MOD)",2015,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(jupk_501,"jpcpua.501",CRC(d25f09c4) SHA1(a12ace496352002685b0415515f5f5ce4fc95bdb))
DE_DMD32ROM8x(         "jpdspa.501",CRC(04a87d42) SHA1(e13df9a63ec77ec6f97b681ed99216ef3f3af691))
DE2S_SOUNDROM142(      "jpu7.dat"  ,CRC(f3afcf13) SHA1(64e12f9d42c00ae08a4584b2ebea475566b90c13),
                       "jpu17.dat" ,CRC(38135a23) SHA1(7c284c17783269824a3d3e83c4cd8ead27133309),
                       "jpu21.dat" ,CRC(6ac1554c) SHA1(9a91ce836c089f96ad9c809bb66fcddda1f3e456))
DE_ROMEND
CORE_CLONEDEF(jupk,501,513,"Jurassic Park (5.01)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(jupk_g51,"jpcpua.501",CRC(d25f09c4) SHA1(a12ace496352002685b0415515f5f5ce4fc95bdb))
DE_DMD32ROM8x(         "jpdspg.501",CRC(3b524bfe) SHA1(ea6ae6f8fc8379f311fd7ef456f0d6711c4e35c5))
DE2S_SOUNDROM142(      "jpu7.dat"  ,CRC(f3afcf13) SHA1(64e12f9d42c00ae08a4584b2ebea475566b90c13),
                       "jpu17.dat" ,CRC(38135a23) SHA1(7c284c17783269824a3d3e83c4cd8ead27133309),
                       "jpu21.dat" ,CRC(6ac1554c) SHA1(9a91ce836c089f96ad9c809bb66fcddda1f3e456))
DE_ROMEND
CORE_CLONEDEF(jupk,g51,513,"Jurassic Park (5.01 German)",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Last Action Hero - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(lah,GEN_DEDMD32, de_128x32DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(lah_112,"lahcpua.112",CRC(e7422236) SHA1(c0422fa6d29fe615cb718056bea00eb9a80ce803))
DE_DMD32ROM8x(      "lahdispa.106",CRC(ca6cfec5) SHA1(5e2081387d76bed17c14120cd347d6aaf435276b))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
#define input_ports_lah input_ports_des11
CORE_GAMEDEF(lah,112,"Last Action Hero (1.12)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(lah_l104,"lahcpua.104",CRC(49b9e5e9) SHA1(cf6198e4c93ce839dc6e5231090d4ca56e9bdea2))
DE_DMD32ROM8x(      "lahdispl.102",CRC(3482c349) SHA1(8f03ba28132ea5159d3193b3adb7b4a6a43046c6))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
CORE_CLONEDEF(lah,l104,112,"Last Action Hero (1.04 Spanish)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(lah_c106,"lahcpuc.106",CRC(d4be4178) SHA1(ea2d9c780f6636a8768164d3a1bb33b050c3a2a7))
DE_DMD32ROM8x(      "lahdispa.104",CRC(baf4e7b3) SHA1(78924d992c0e206bfbf4a6fcc62ea7f91e995260))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
CORE_CLONEDEF(lah,c106,112,"Last Action Hero (1.06 Canadian)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(lah_l108,"lahcpua.108",CRC(8942794b) SHA1(f023ca040d6d4c6da80b58a162f1d217e571ed81))
DE_DMD32ROM8x(      "lahdispl.104",CRC(6b1e51a7) SHA1(ad17507b63f2da8aa0651401ccb8d449c15aa46c))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
CORE_CLONEDEF(lah,l108,112,"Last Action Hero (1.08 Spanish)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(lah_110,"lahcpua.110",CRC(d1861dc2) SHA1(288bd06b6ae346d1f6a17a642d5533f1a9a3bf5e))
DE_DMD32ROM8x(      "lahdispa.106",CRC(ca6cfec5) SHA1(5e2081387d76bed17c14120cd347d6aaf435276b))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
CORE_CLONEDEF(lah,110,112,"Last Action Hero (1.10)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(lah_113,"lahcpua.113",CRC(d05fd307) SHA1(49c9e5ac509498a521f5dae062df5442d4e954ac))
DE_DMD32ROM8x(      "lahdispa.106",CRC(ca6cfec5) SHA1(5e2081387d76bed17c14120cd347d6aaf435276b))
DE2S_SOUNDROM142(   "lahsnd.u7"  ,CRC(0279c45b) SHA1(14daf6b711d1936352209e90240f51812ebe76e0),
                    "lahsnd.u17" ,CRC(d0c15fa6) SHA1(5dcd13b578fa53c82353cda5aa774ca216c5ddfe),
                    "lahsnd.u21" ,CRC(4571dc2e) SHA1(a1068cb080c30dbc07d164eddfc5dfd0afd52d3b))
DE_ROMEND
CORE_CLONEDEF(lah,113,112,"Last Action Hero (1.13 unofficial MOD)",2014,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Tales from the Crypt - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(tftc,GEN_DEDMD32, de_128x32DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(tftc_303,"tftccpua.303",CRC(e9bec98e) SHA1(02643805d596017c88d9a534b94b2075bb2ab101))
DE_DMD32ROM8x(         "tftcdspa.301",CRC(3888d06f) SHA1(3d276df436a76c6e9bed6629114204dacd88245b))
DE2S_SOUNDROM144(      "sndu7.dat"   ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat"  ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat"  ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
#define input_ports_tftc input_ports_des11
CORE_GAMEDEF(tftc,303,"Tales from the Crypt (3.03)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tftc_400,"tftccpua.400",CRC(766c144e) SHA1(c99c0eb0017784c61b09a0e5ed4056b6f0cdb4e4))
DE_DMD32ROM8x(         "tftcdspa.400",CRC(015d24e0) SHA1(91e8a42dda4a8359ebf244418c873744a499430e))
DE2S_SOUNDROM144(      "sndu7.dat"   ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat"  ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat"  ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
CORE_CLONEDEF(tftc,400,303,"Tales from the Crypt (4.00 unofficial MOD)",2015,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tftc_302,"tftccpua.302",CRC(a194fe0f) SHA1(b83e048300f7e072f76672d72cdf43e43fab2e9e))
DE_DMD32ROM8x(         "tftcdspa.301",CRC(3888d06f) SHA1(3d276df436a76c6e9bed6629114204dacd88245b))
DE2S_SOUNDROM144(      "sndu7.dat"   ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat"  ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat"  ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
CORE_CLONEDEF(tftc,302,303,"Tales from the Crypt (3.02 Dutch)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tftc_300,"tftccpua.300",CRC(3d275152) SHA1(0aa6df629c27d9265cf35ca0724e241d9820e56b))
DE_DMD32ROM8x(         "tftcdspa.300",CRC(bf5c812b) SHA1(c10390b6cad0ad457fb83241c7ee1d6b109cf5be))
DE2S_SOUNDROM144(      "sndu7.dat"   ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat"  ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat"  ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
CORE_CLONEDEF(tftc,300,303,"Tales from the Crypt (3.00)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tftc_200,"tftcgc5.a20",CRC(94b61f83) SHA1(9f36353a06cacb8ad67f70cd8d9d8ac698905ba3))
DE_DMD32ROM8x(         "tftcdot.a20",CRC(16b3968a) SHA1(6ce91774fc60187e4b0d8874a14ef64e2805eb3f))
DE2S_SOUNDROM144(      "sndu7.dat"  ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat" ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat" ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
CORE_CLONEDEF(tftc,200,303,"Tales from the Crypt (2.00)",1993,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tftc_104,"tftccpua.104",CRC(efb3c0d0) SHA1(df1505947732704171e31dbace4c263723c8342b))
DE_DMD32ROM8x(         "tftcdspl.103",CRC(98f3b13e) SHA1(909c373b1a27b5aeebad2535ae4fb9bba71e9b5c))
DE2S_SOUNDROM144(      "sndu7.dat"   ,CRC(7963740e) SHA1(fc1f150dcbab8af865a8ea624dfdcc03301f05e6),
                       "sndu17.dat"  ,CRC(5c5d009a) SHA1(57d0307ea682eca5a57957e4f61fd92bb7f40e17),
                       "sndu21.dat"  ,CRC(a0ae61f7) SHA1(c7b5766fda64642f77bdc03b2025cd84f29f4495))
DE_ROMEND
CORE_CLONEDEF(tftc,104,303,"Tales from the Crypt (1.04 Spanish)",1993,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Tommy - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
//INITGAME(tommy,DE_CPUREV3b, 0, FLIP6364, 3)

#define sBlinderMotor   CORE_CUSTSOLNO(1) /* 33 */
/*-- return status of custom solenoids --*/
static int tommy_getSol(int solNo) { return (solNo == sBlinderMotor) ? (core_getSol(44) > 0) : 0; }
static core_tGameData tommyGameData = {
  GEN_DEDMD32, de_128x32DMD,
  { FLIP6364, 0,0,1, // We need 1 custom solenoids!
    SNDBRD_DE2S,SNDBRD_DEDMD32,S11_PRINTERLINE,0,
    tommy_getSol
  }, NULL, {{0}}, {10}
};
static void init_tomy(void) { core_gameData = &tommyGameData; }
DE_ROMSTARTx0(tomy_400,"tomcpua.400", CRC(d0310a1a) SHA1(5b14f5d6e271676b4ec93b64f1cde9607844b677))
DE_DMD32ROM8x(      "tommydva.400",CRC(9e640d09) SHA1(d921fadeb728cf929c6bae2e79bd4d140192a4d2))
DE2S_SOUNDROM14444( "tommysnd.u7" ,CRC(ab0b4626) SHA1(31237b4f5e866710506f1336e3ca2dbd6a89385a),
                    "tommysnd.u17",CRC(11bb2aa7) SHA1(57b4867c109996861f45ead1ceedb7153aff852e),
                    "tommysnd.u21",CRC(bb4aeec3) SHA1(2ac6cd25b79584fa6ad2c8a36c3cc58ab8ec0206),
                    "tommysnd.u36",CRC(208d7aeb) SHA1(af8af2094d1a91c7b4ef8ac6d4f594728e97450f),
                    "tommysnd.u37",CRC(46180085) SHA1(f761c27532180de313f23b41f02341783be8938b))
DE_ROMEND
#define input_ports_tomy input_ports_des11
CORE_GAMEDEF(tomy,400,"Tommy (4.00)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tomy_500,"tomcpua.500", CRC(902cfaa7) SHA1(0f2931d43d8effb7c8ab4e430dcb57a2ea8364c6))
DE_DMD32ROM8x(      "tommydva.500",CRC(9ade47a0) SHA1(16b5600afc4dd22cf8a04997225cdac9edb685d8))
DE2S_SOUNDROM14444( "tommysnd.u7" ,CRC(ab0b4626) SHA1(31237b4f5e866710506f1336e3ca2dbd6a89385a),
                    "tommysnd.u17",CRC(11bb2aa7) SHA1(57b4867c109996861f45ead1ceedb7153aff852e),
                    "tommysnd.u21",CRC(bb4aeec3) SHA1(2ac6cd25b79584fa6ad2c8a36c3cc58ab8ec0206),
                    "tommysnd.u36",CRC(208d7aeb) SHA1(af8af2094d1a91c7b4ef8ac6d4f594728e97450f),
                    "tommysnd.u37",CRC(46180085) SHA1(f761c27532180de313f23b41f02341783be8938b))
DE_ROMEND
CORE_CLONEDEF(tomy,500,400,"Tommy (5.00 unofficial MOD)",2016,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tomy_h30,"tomcpuh.300", CRC(121b5932) SHA1(e7d7bf8a78baf1c00c8bac908d4646586b8cf1f5))
DE_DMD32ROM8x(      "tommydva.300",CRC(1f2d0896) SHA1(50c617e30bb843c69a6ca8afeeb751c886f5e6bd))
DE2S_SOUNDROM14444( "tommysnd.u7" ,CRC(ab0b4626) SHA1(31237b4f5e866710506f1336e3ca2dbd6a89385a),
                    "tommysnd.u17",CRC(11bb2aa7) SHA1(57b4867c109996861f45ead1ceedb7153aff852e),
                    "tommysnd.u21",CRC(bb4aeec3) SHA1(2ac6cd25b79584fa6ad2c8a36c3cc58ab8ec0206),
                    "tommysnd.u36",CRC(208d7aeb) SHA1(af8af2094d1a91c7b4ef8ac6d4f594728e97450f),
                    "tommysnd.u37",CRC(46180085) SHA1(f761c27532180de313f23b41f02341783be8938b))
DE_ROMEND
CORE_CLONEDEF(tomy,h30,400,"Tommy (3.00 Dutch)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(tomy_102,"tomcpua.102", CRC(e470b78e) SHA1(9d358e9d87469cdefb5c373f16c51774bbd390ea))
DE_DMD32ROM8x(      "tommydva.300",CRC(1f2d0896) SHA1(50c617e30bb843c69a6ca8afeeb751c886f5e6bd))
DE2S_SOUNDROM14444( "tommysnd.u7" ,CRC(ab0b4626) SHA1(31237b4f5e866710506f1336e3ca2dbd6a89385a),
                    "tommysnd.u17",CRC(11bb2aa7) SHA1(57b4867c109996861f45ead1ceedb7153aff852e),
                    "tommysnd.u21",CRC(bb4aeec3) SHA1(2ac6cd25b79584fa6ad2c8a36c3cc58ab8ec0206),
                    "tommysnd.u36",CRC(208d7aeb) SHA1(af8af2094d1a91c7b4ef8ac6d4f594728e97450f),
                    "tommysnd.u37",CRC(46180085) SHA1(f761c27532180de313f23b41f02341783be8938b))
DE_ROMEND
CORE_CLONEDEF(tomy,102,400,"Tommy (1.02)",1994,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ WWF Royal Rumble - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(wwfr,GEN_DEDMD32, de_128x32DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD32, S11_PRINTERLINE)
DE_ROMSTARTx0(wwfr_106,"wwfcpua.106", CRC(5f1c7da2) SHA1(9188e0b9c26e4b6c92c63a58b52ee42bd3b77ca0))
DE_DMD32ROM8x(         "wwfdispa.102",CRC(4b629a4f) SHA1(c301d0c785f7bc4d3c23cbda76ff955c742eaeef))
DE2S_SOUNDROM1444(     "wfsndu7.512", CRC(eb01745c) SHA1(7222e39c52ed298b737aadaa5b57d2068d39287e),
                       "wfsndu17.400",CRC(7d9c2ca8) SHA1(5d84559455fe7e27634b28bcab81d54f2676390e),
                       "wfsndu21.400",CRC(242dcdcb) SHA1(af7220e14b0956ef40f75b2749eb1b9d715a1af0),
                       "wfsndu36.400",CRC(39db8d85) SHA1(a55dd88fd4d9154b523dca9160bf96119af1f94d))
DE_ROMEND
#define input_ports_wwfr input_ports_des11
CORE_GAMEDEF(wwfr,106,"WWF Royal Rumble (1.06)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(wwfr_103f,"wwfcpuf.103"  ,CRC(0e211494) SHA1(c601a075636f84ad12ec0693772a8759049077d5))
DE_DMD32ROM8x(          "wwfdspf.101"  ,CRC(4c39bda9) SHA1(2ea61a2020a4a4e3f23853ab8780d6999053e8ae))
DE2S_SOUNDROM1444(      "wfsndu7.512"  ,CRC(eb01745c) SHA1(7222e39c52ed298b737aadaa5b57d2068d39287e),
                        "wfsndu17.400" ,CRC(7d9c2ca8) SHA1(5d84559455fe7e27634b28bcab81d54f2676390e),
                        "wfsndu21.400" ,CRC(242dcdcb) SHA1(af7220e14b0956ef40f75b2749eb1b9d715a1af0),
                        "wfsndu36.400" ,CRC(39db8d85) SHA1(a55dd88fd4d9154b523dca9160bf96119af1f94d))
DE_ROMEND
CORE_CLONEDEF(wwfr,103f,106,"WWF Royal Rumble (1.03 French)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(wwfr_103, "wfcpuc5.512"  ,CRC(7e9ead89) SHA1(6cfd64899128b5f9b4ccc37b7bfdbb0a2a75a3a5))
DE_DMD32ROM8x(          "wfdisp0.400"  ,CRC(e190b90f) SHA1(a0e73ce0b241a81e935e6790e04ea5e1fccf3742))
DE2S_SOUNDROM1444(      "wfsndu7.512"  ,CRC(eb01745c) SHA1(7222e39c52ed298b737aadaa5b57d2068d39287e),
                        "wfsndu17.400" ,CRC(7d9c2ca8) SHA1(5d84559455fe7e27634b28bcab81d54f2676390e),
                        "wfsndu21.400" ,CRC(242dcdcb) SHA1(af7220e14b0956ef40f75b2749eb1b9d715a1af0),
                        "wfsndu36.400" ,CRC(39db8d85) SHA1(a55dd88fd4d9154b523dca9160bf96119af1f94d))
DE_ROMEND
CORE_CLONEDEF(wwfr,103,106,"WWF Royal Rumble (1.03)",1994,"Data East",de_mDEDMD32S2A,0)

/*-------------------------------------------------------------
/ Guns N' Roses - CPU Rev 3b /DMD  Type 2 512K Rom - 64K CPU Rom
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
  GEN_DEDMD32, de_128x32DMD,
  { FLIP6364, 0,0,3, //We need 3 custom solenoids!
    SNDBRD_DE2S,SNDBRD_DEDMD32,S11_PRINTERLINE,0, gnr_getSol
  }, NULL, {{0}}, {10}
};
static void init_gnr(void) { core_gameData = &gnrGameData; }
DE_ROMSTARTx0(gnr_300,"gnrcpua.300",CRC(faf0cc8c) SHA1(0e889ad6eed832d4ccdc6e379f9e4e58ae0e0b83))
DE_DMD32ROM8x(     "gnrdispa.300",CRC(4abf29e3) SHA1(595328e0f92a6e1972d71c56505a5dd07a373ef5))
DE2S_SOUNDROM14444("gnru7.snd"  ,CRC(3b9de915) SHA1(a901a1f37bf5433c819393c4355f9d13164b32ce),
                   "gnru17.snd" ,CRC(3d3219d6) SHA1(ac4a6d3eff0cdd02b8c79dddcb8fec2e22faa9b9),
                   "gnru21.snd" ,CRC(d2ca17ab) SHA1(db7c4f74a2e2c099fe14f38de922fdc851bd4a6b),
                   "gnru36.snd" ,CRC(5b32396e) SHA1(66462a6a929c869d668968e057fac199d05df267),
                   "gnru37.snd" ,CRC(4930e1f2) SHA1(1569d0c7fea1af008acbdc492c3677ace7d1897a))
DE_ROMEND
#define input_ports_gnr input_ports_des11
CORE_GAMEDEF(gnr,300,"Guns N' Roses (3.00)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(gnr_300f,"gnrcpuf.300",CRC(7f9006b2) SHA1(429d90fa27ea39176b94d1293a313ec3d1033dbc))
DE_DMD32ROM8x(     "gnrdispf.300",CRC(63e9761a) SHA1(05e5a61b66148da7728779d8e5fa14a489e09441))
DE2S_SOUNDROM14444("gnru7.snd"  ,CRC(3b9de915) SHA1(a901a1f37bf5433c819393c4355f9d13164b32ce),
                   "gnru17.snd" ,CRC(3d3219d6) SHA1(ac4a6d3eff0cdd02b8c79dddcb8fec2e22faa9b9),
                   "gnru21.snd" ,CRC(d2ca17ab) SHA1(db7c4f74a2e2c099fe14f38de922fdc851bd4a6b),
                   "gnru36.snd" ,CRC(5b32396e) SHA1(66462a6a929c869d668968e057fac199d05df267),
                   "gnru37.snd" ,CRC(4930e1f2) SHA1(1569d0c7fea1af008acbdc492c3677ace7d1897a))
DE_ROMEND
#define input_ports_gnrf input_ports_gnr
CORE_CLONEDEF(gnr,300f,300,"Guns N' Roses (3.00 French)",1994,"Data East",de_mDEDMD32S2A,0)

DE_ROMSTARTx0(gnr_300d,"gnrcpud.300",CRC(ae35f830) SHA1(adf853f50ed01c3261d7ce4064c45f834934b5e2))
DE_DMD32ROM8x(     "gnrdispd.300",CRC(4abf29e3) SHA1(595328e0f92a6e1972d71c56505a5dd07a373ef5))
DE2S_SOUNDROM14444("gnru7.snd"  ,CRC(3b9de915) SHA1(a901a1f37bf5433c819393c4355f9d13164b32ce),
                   "gnru17.snd" ,CRC(3d3219d6) SHA1(ac4a6d3eff0cdd02b8c79dddcb8fec2e22faa9b9),
                   "gnru21.snd" ,CRC(d2ca17ab) SHA1(db7c4f74a2e2c099fe14f38de922fdc851bd4a6b),
                   "gnru36.snd" ,CRC(5b32396e) SHA1(66462a6a929c869d668968e057fac199d05df267),
                   "gnru37.snd" ,CRC(4930e1f2) SHA1(1569d0c7fea1af008acbdc492c3677ace7d1897a))
DE_ROMEND
#define input_ports_gnrf input_ports_gnr
CORE_CLONEDEF(gnr,300d,300,"Guns N' Roses (3.00 Dutch)",1994,"Data East",de_mDEDMD32S2A,0)

/*****************************************************************************************************************************/
/* NOTE: SEGA Began Distribution of the following games, although they run on Data East Hardware, so they stay in this file! */
/*****************************************************************************************************************************/

/***********************************************************************/
/*************** GAMES USING 192X64 DMD DISPLAY ************************/
/***********************************************************************/
//Snd Works (Voices might be messed not sure)
/*-------------------------------------------------------------
/ Maverick - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(mav, GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64, S11_PRINTERLINE)
DE_ROMSTARTx0(mav_402, "mavcpua.404",CRC(9f06bd8d) SHA1(3b931af5455ed9c40f2b6c884427a326bba8f75a))
DE_DMD64ROM88(   "mavdisp0.402",CRC(4e643525) SHA1(30b91c91c2f1295cdd018023c5ac783570a0aeea),
                 "mavdisp3.402",CRC(8c5f9460) SHA1(6369b4c98ec6fd5e769275b44631b2b6dd5c411b))
DE2S_SOUNDROM144("mavu7.dat"  , CRC(427e6ab9) SHA1(6ad9295097f3d498383c91adf4ca667f797f29b1),
                 "mavu17.dat" , CRC(cba377b8) SHA1(b7551b6cb64357cdacf1a96cedfccbabf4bd070a),
                 "mavu21.dat" , CRC(be0c6a6f) SHA1(4fee912d9f0d4b196dbfacf06a4202b2fa3037b1))
DE_ROMEND
#define input_ports_mav input_ports_des11
CORE_GAMEDEF(mav,402,"Maverick (4.04, Display 4.02)",1994,"Sega",de_mDEDMD64S2A,0)

DE_ROMSTARTx0(mav_401, "mavcpua.404",CRC(9f06bd8d) SHA1(3b931af5455ed9c40f2b6c884427a326bba8f75a))
DE_DMD64ROM88(   "mavdsar0.401",CRC(35b811af) SHA1(1e235a0f16ef0eecca5b6ec7a2234ed1dc4e4440),
                 "mavdsar3.401",CRC(c4c126ae) SHA1(b4841e83ec075bddc919217b65afaac97709e69b))
DE2S_SOUNDROM144("mavu7.dat"  , CRC(427e6ab9) SHA1(6ad9295097f3d498383c91adf4ca667f797f29b1),
                 "mavu17.dat" , CRC(cba377b8) SHA1(b7551b6cb64357cdacf1a96cedfccbabf4bd070a),
                 "mavu21.dat" , CRC(be0c6a6f) SHA1(4fee912d9f0d4b196dbfacf06a4202b2fa3037b1))
DE_ROMEND
CORE_CLONEDEF(mav,401,402,"Maverick (4.04, Display 4.01)",1994,"Sega",de_mDEDMD64S2A,0)

DE_ROMSTARTx0(mav_400, "mavgc5.400",CRC(e2d0a88b) SHA1(d1571edba47aecc871ac0cfdaabca31774f70fa1))
DE_DMD64ROM88(   "mavdisp0.400",CRC(b6069484) SHA1(2878d9a0151194bd4a0e12e2f75b02a5d7316b68),
                 "mavdisp3.400",CRC(149f871f) SHA1(e29a8bf149b77bccaeed202786cf76d9a4fd51df))
DE2S_SOUNDROM144("mavu7.dat"  , CRC(427e6ab9) SHA1(6ad9295097f3d498383c91adf4ca667f797f29b1),
                 "mavu17.dat" , CRC(cba377b8) SHA1(b7551b6cb64357cdacf1a96cedfccbabf4bd070a),
                 "mavu21.dat" , CRC(be0c6a6f) SHA1(4fee912d9f0d4b196dbfacf06a4202b2fa3037b1))
DE_ROMEND
CORE_CLONEDEF(mav,400,402,"Maverick (4.00)",1994,"Sega",de_mDEDMD64S2A,0)

DE_ROMSTARTx0(mav_100, "mavcpu.100",CRC(13fdc959) SHA1(f8155f0fe5d4c3fe55000ab3b57f298fd9229fef))
DE_DMD64ROM88(   "mavdsp0.100", CRC(3e01f5c8) SHA1(8e40f399c77aa17bebbefe04742ff2ff95508323),
                 "mavdsp3.100", CRC(e2b623f2) SHA1(7b5a6d0db30f3deedb8fe0e1731c81ec836a66f5))
DE2S_SOUNDROM144("mavu7.dat"  , CRC(427e6ab9) SHA1(6ad9295097f3d498383c91adf4ca667f797f29b1),
                 "mavu17.dat" , CRC(cba377b8) SHA1(b7551b6cb64357cdacf1a96cedfccbabf4bd070a),
                 "mavu21.dat" , CRC(be0c6a6f) SHA1(4fee912d9f0d4b196dbfacf06a4202b2fa3037b1))
DE_ROMEND
CORE_CLONEDEF(mav,100,402,"Maverick (1.00)",1994,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Mary Shelley's Frankenstein - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(frankst,GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64, S11_PRINTERLINE)
DE_ROMSTARTx0(frankst,"franka.103",  CRC(a9aba9be) SHA1(1cc22fcbc0f51a17037637c04e606579956c9cba))
DE_DMD64ROM88(        "frdspr0a.103",CRC(9dd09c7d) SHA1(c5668e53d6c914667a59538f82222ec2efc6f187),
                      "frdspr3a.103",CRC(73b538bb) SHA1(07d7ae21f062d15711d72af03bfcd52608f75a5f))
DE2S_SOUNDROM1444(    "frsnd.u7"  ,  CRC(084f856c) SHA1(c91331a32b565c2ed3f96156f44143dc22009e8e),
                      "frsnd.u17" ,  CRC(0da904d6) SHA1(e190f1a35147b2f39224832969ca7b1d4a30f6cc),
                      "frsnd.u21" ,  CRC(14d4bc12) SHA1(9e7005c5bd0afe7f9c9215b39878496640cdea77),
                      "frsnd.u36" ,  CRC(9964d721) SHA1(5ea0bc051d1909bee80d3feb6b7350b6307b6dcb))
DE_ROMEND
#define input_ports_frankst input_ports_des11
CORE_GAMEDEFNV(frankst,"Frankenstein, Mary Shelley's",1994,"Sega",de_mDEDMD64S2A,0)

#define init_frankstg init_frankst
DE_ROMSTARTx0(frankstg,"franka.103",  CRC(a9aba9be) SHA1(1cc22fcbc0f51a17037637c04e606579956c9cba))
DE_DMD64ROM88(         "frdspr0g.101",CRC(5e27ec02) SHA1(351d6f1b7d72e415f2bf5780b6533dbd67579261),
                       "frdspr3g.101",CRC(d6c607b5) SHA1(876d4bd2a5b89f1a28ff7cd45494c7245f147d27))
DE2S_SOUNDROM1444(     "frsnd.u7"  ,  CRC(084f856c) SHA1(c91331a32b565c2ed3f96156f44143dc22009e8e),
                       "frsnd.u17" ,  CRC(0da904d6) SHA1(e190f1a35147b2f39224832969ca7b1d4a30f6cc),
                       "frsnd.u21" ,  CRC(14d4bc12) SHA1(9e7005c5bd0afe7f9c9215b39878496640cdea77),
                       "frsnd.u36" ,  CRC(9964d721) SHA1(5ea0bc051d1909bee80d3feb6b7350b6307b6dcb))
DE_ROMEND
#define input_ports_frankstg input_ports_frankst
CORE_CLONEDEFNV(frankstg,frankst,"Frankenstein, Mary Shelley's (German)",1995,"Sega",de_mDEDMD64S2A,0)

//Start of the Portals Diagnostic Menu System
/*-------------------------------------------------------------
/ Baywatch - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(baywatch,GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64, S11_PRINTERLINE)
DE_ROMSTARTx0(baywatch, "baycpua.400", CRC(89facfda) SHA1(71720b1da227752b0e276390abd08c742bca9090))
DE_DMD64ROM88(          "bayrom0a.400",CRC(43d615c6) SHA1(7c843b6d5215305b02a55c9fa1d62375ef0766ea),
                        "bayrom3a.400",CRC(41bcb66b) SHA1(e6f0a9236e14c2e919881ca1ffe3356aaa121730))
DE2S_SOUNDROM144(       "bayw.u7"  ,   CRC(90d6d8a8) SHA1(482c5643453f21a078257aa13398845ef19cab3c),
                        "bayw.u17" ,   CRC(b20fde56) SHA1(2f2db49245e4a6a8251cbe896b2437fcec88d42d),
                        "bayw.u21" ,   CRC(b7598881) SHA1(19d1dde1cb6634a7c7b5cdb4fa01cd09cc7d7777))
DE_ROMEND
#define input_ports_baywatch input_ports_des112
CORE_GAMEDEFNV(baywatch,"Baywatch (4.00)",1995,"Sega",de_mDEDMD64S2A,0)

#define init_bay_401 init_baywatch
DE_ROMSTARTx0(bay_401,  "baycpua.401", CRC(0c69e71e) SHA1(ccc75dac14e0b0b5e6b8258bab2adbdf284dde4b))
DE_DMD64ROM88(          "bayrom0a.400",CRC(43d615c6) SHA1(7c843b6d5215305b02a55c9fa1d62375ef0766ea),
                        "bayrom3a.400",CRC(41bcb66b) SHA1(e6f0a9236e14c2e919881ca1ffe3356aaa121730))
DE2S_SOUNDROM144(       "bayw.u7"  ,   CRC(90d6d8a8) SHA1(482c5643453f21a078257aa13398845ef19cab3c),
                        "bayw.u17" ,   CRC(b20fde56) SHA1(2f2db49245e4a6a8251cbe896b2437fcec88d42d),
                        "bayw.u21" ,   CRC(b7598881) SHA1(19d1dde1cb6634a7c7b5cdb4fa01cd09cc7d7777))
DE_ROMEND
#define input_ports_bay_401 input_ports_baywatch
CORE_CLONEDEFNV(bay_401,baywatch,"Baywatch (4.01 unofficial MOD)",2017,"Sega",de_mDEDMD64S2A,0)

#define init_bay_d400 init_baywatch
DE_ROMSTARTx0(bay_d400, "baycpud.400", CRC(45019616) SHA1(5a1e04cdfa00f179f010c09fae52d090553cd82e))
DE_DMD64ROM88(          "bayrom0d.300",CRC(3f195829) SHA1(a10a1b7f125f239b0eff87ee6667c8250b7ffc87),
                        "bayrom3d.300",CRC(ae3d8585) SHA1(28b38ebc2755ffb3859f8091a9bf50d868794a3e))
DE2S_SOUNDROM144(       "bayw.u7"  ,   CRC(90d6d8a8) SHA1(482c5643453f21a078257aa13398845ef19cab3c),
                        "bayw.u17" ,   CRC(b20fde56) SHA1(2f2db49245e4a6a8251cbe896b2437fcec88d42d),
                        "bayw.u21" ,   CRC(b7598881) SHA1(19d1dde1cb6634a7c7b5cdb4fa01cd09cc7d7777))
DE_ROMEND
#define input_ports_bay_d400 input_ports_baywatch
CORE_CLONEDEFNV(bay_d400,baywatch,"Baywatch (4.00 Dutch)",1995,"Sega",de_mDEDMD64S2A,0)

#define init_bay_e400 init_baywatch
DE_ROMSTARTx0(bay_e400, "baycpua2.400", CRC(07b77fe2) SHA1(4f81a5b3d821907e06d6b547117ad39c238a900c))
DE_DMD64ROM88(          "bayrom0a.400",CRC(43d615c6) SHA1(7c843b6d5215305b02a55c9fa1d62375ef0766ea),
                        "bayrom3a.400",CRC(41bcb66b) SHA1(e6f0a9236e14c2e919881ca1ffe3356aaa121730))
DE2S_SOUNDROM1444(      "bw-u7.bin"  , CRC(a5e57557) SHA1(a884c1118331b8724507b0a916127ce5df309fe4),
                        "bw-u17.bin" , CRC(660e7f5d) SHA1(6dde294e728e596a6c455326793b65254139620e),
                        "bw-u21.bin" , CRC(5ec3a889) SHA1(f355f742de137344e6e4b5d3a4b2380a876c8cc3),
                        "bw-u36.bin" , CRC(1877abc5) SHA1(13ca231a486495a83cc1d9c6dde558a57eb4abe1))
DE_ROMEND
#define input_ports_bay_e400 input_ports_baywatch
CORE_CLONEDEFNV(bay_e400,baywatch,"Baywatch (4.00 English)",1995,"Sega",de_mDEDMD64S2A,0)

#define init_bay_d300 init_baywatch
DE_ROMSTARTx0(bay_d300, "baycpud.300", CRC(c160f045) SHA1(d1f75d5ba292b25278539b01e0f4908276d34e34))
DE_DMD64ROM88(          "bayrom0d.300",CRC(3f195829) SHA1(a10a1b7f125f239b0eff87ee6667c8250b7ffc87),
                        "bayrom3d.300",CRC(ae3d8585) SHA1(28b38ebc2755ffb3859f8091a9bf50d868794a3e))
DE2S_SOUNDROM144(       "bayw.u7"  ,   CRC(90d6d8a8) SHA1(482c5643453f21a078257aa13398845ef19cab3c),
                        "bayw.u17" ,   CRC(b20fde56) SHA1(2f2db49245e4a6a8251cbe896b2437fcec88d42d),
                        "bayw.u21" ,   CRC(b7598881) SHA1(19d1dde1cb6634a7c7b5cdb4fa01cd09cc7d7777))
DE_ROMEND
#define input_ports_bay_d300 input_ports_baywatch
CORE_CLONEDEFNV(bay_d300,baywatch,"Baywatch (3.00 Dutch)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever 4.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
INITGAMES11(batmanf, GEN_DEDMD64, de_192x64DMD, FLIP6364, SNDBRD_DE2S, SNDBRD_DEDMD64, S11_PRINTERLINE)
DE_ROMSTARTx0(batmanf, "batnova.401", CRC(4e62df4e) SHA1(6c3be65fc8825f47cd08755b58fdcf3652ede702))
DE_DMD64ROM88(         "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                       "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(      "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                       "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                       "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_batmanf input_ports_des112
CORE_GAMEDEFNV(batmanf,"Batman Forever (4.0)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever 3.0 - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(batmanf3, "batcpua.302", CRC(5ae7ce69) SHA1(13409c7c993bd9940f3a72f3bac8c8c57a665b3f))
DE_DMD64ROM88(          "bmfrom0a.300",CRC(764bb217) SHA1(2923d2d2924faa4bdc6e67087fb8ce694d27809a),
                        "bmfrom3a.300",CRC(b4e3b515) SHA1(0f8bf08bc480eed575da54bfc0135f38a86302d4))
DE2S_SOUNDROM144(       "bmfu7.bin"  , CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                        "bmfu17.bin" , CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                        "bmfu21.bin" , CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_batmanf3 input_ports_batmanf
#define init_batmanf3        init_batmanf
CORE_CLONEDEFNV(batmanf3,batmanf,"Batman Forever (3.0)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (2.02) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(batmanf2, "batcpua.202", CRC(3e2fe40b) SHA1(afacbbc8af319110149b25c35ef03dcf019ca8da))
DE_DMD64ROM88(          "bmfrom0.200",CRC(17086824) SHA1(37f2d463d7cc15739fb18000c81dbc1e79c1549a),
                        "bmfrom3.200",CRC(9c8a9a8f) SHA1(8dce048cac657da66478ae0b6bd000a2648a118a))
DE2S_SOUNDROM144(       "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                        "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                        "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_batmanf2 input_ports_batmanf
#define init_batmanf2        init_batmanf
CORE_CLONEDEFNV(batmanf2,batmanf,"Batman Forever (2.02)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (1.02) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(batmanf1, "batcpua.102", CRC(aafba427) SHA1(485fa3b76569a8c9ed640e9fa8fd714fdd2268b8))
DE_DMD64ROM88(          "bmfrom0.100",CRC(4d65a45c) SHA1(b4a112f8a70ad887e1a23291bcec1d55bd7277c1),
                        "bmfrom3.100",CRC(b4b774d1) SHA1(5dacfb5cedc597dbb2d72e83de4979eb19b19d72))
DE2S_SOUNDROM144(       "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                        "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                        "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_batmanf1 input_ports_batmanf
#define init_batmanf1        init_batmanf
CORE_CLONEDEFNV(batmanf1,batmanf,"Batman Forever (1.02)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (UK) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_uk,"batnove.401",CRC(80f6e4af) SHA1(dd233d2150dcb50b74a70e6ff89c74a3f0d8fae1))
DE_DMD64ROM88(       "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                     "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(    "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                     "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                     "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_uk input_ports_batmanf
#define init_bmf_uk        init_batmanf
CORE_CLONEDEFNV(bmf_uk,batmanf,"Batman Forever (4.0 English)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (CN) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_cn, "batnovc.401",CRC(99936537) SHA1(08ff9c6a1fcb3f198190d24bbc75ea1178427fda))
DE_DMD64ROM88(        "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                      "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_cn input_ports_batmanf
#define init_bmf_cn        init_batmanf
CORE_CLONEDEFNV(bmf_cn,batmanf,"Batman Forever (4.0 Canadian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (NO) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_no, "batnovn.401",CRC(79dd48b4) SHA1(eefdf423f9638e293e51bd31413de898ec4eb83a))
DE_DMD64ROM88(        "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                      "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_no input_ports_batmanf
#define init_bmf_no        init_batmanf
CORE_CLONEDEFNV(bmf_no,batmanf,"Batman Forever (4.0 Norwegian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (SV) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_sv, "batnovt.401",CRC(854029ab) SHA1(044c2fff6f3e8995c48344f727c1cd9079f7e232))
DE_DMD64ROM88(        "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                      "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_sv input_ports_batmanf
#define init_bmf_sv        init_batmanf
CORE_CLONEDEFNV(bmf_sv,batmanf,"Batman Forever (4.0 Swedish)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (AT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_at, "batnovh.401",CRC(acba13d7) SHA1(b5e5dc5ffc926612ea3d592b6d4e8e02f6290bc7))
DE_DMD64ROM88(        "bfdrom0g.401",CRC(3a2d7d53) SHA1(340107290d58bfb8b9a6613215eb556626fe2461),
                      "bfdrom3g.401",CRC(94e424f1) SHA1(3a6daf9cbd38e21e2c6447ff1fb0e86b4c03f971))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_at input_ports_batmanf
#define init_bmf_at        init_batmanf
CORE_CLONEDEFNV(bmf_at,batmanf,"Batman Forever (4.0 Austrian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (CH) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_ch, "batnovs.401",CRC(4999d5f9) SHA1(61a9220da38e05360a9496504fa7b11aff14515d))
DE_DMD64ROM88(        "bfdrom0g.401",CRC(3a2d7d53) SHA1(340107290d58bfb8b9a6613215eb556626fe2461),
                      "bfdrom3g.401",CRC(94e424f1) SHA1(3a6daf9cbd38e21e2c6447ff1fb0e86b4c03f971))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_ch input_ports_batmanf
#define init_bmf_ch        init_batmanf
CORE_CLONEDEFNV(bmf_ch,batmanf,"Batman Forever (4.0 Swiss)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (DE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_de, "batnovg.401",CRC(dd37e99a) SHA1(7949ed43df38849d927f6ed0afa8c3f77cd74b6a))
DE_DMD64ROM88(        "bfdrom0g.401",CRC(3a2d7d53) SHA1(340107290d58bfb8b9a6613215eb556626fe2461),
                      "bfdrom3g.401",CRC(94e424f1) SHA1(3a6daf9cbd38e21e2c6447ff1fb0e86b4c03f971))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_de input_ports_batmanf
#define init_bmf_de        init_batmanf
CORE_CLONEDEFNV(bmf_de,batmanf,"Batman Forever (4.0 German)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (BE) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_be, "batnovb.401",CRC(21309873) SHA1(cebd0c5c05dc5c0a2eb8563ad5c4759f78d6a4b9))
DE_DMD64ROM88(        "bfdrom0f.401",CRC(e7473f6f) SHA1(f5951a9b6a8776073adf10e38b9d68d6d444240a),
                      "bfdrom3f.401",CRC(f7951709) SHA1(ace5b374d1e382d6f612b2bafc0e9fdde9e21014))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_be input_ports_batmanf
#define init_bmf_be        init_batmanf
CORE_CLONEDEFNV(bmf_be,batmanf,"Batman Forever (4.0 Belgian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (FR) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_fr, "batnovf.401",CRC(4baa793d) SHA1(4ba258d11f1bd7a2078ae6cd823a11e10ca96627))
DE_DMD64ROM88(        "bfdrom0f.401",CRC(e7473f6f) SHA1(f5951a9b6a8776073adf10e38b9d68d6d444240a),
                      "bfdrom3f.401",CRC(f7951709) SHA1(ace5b374d1e382d6f612b2bafc0e9fdde9e21014))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_fr input_ports_batmanf
#define init_bmf_fr        init_batmanf
CORE_CLONEDEFNV(bmf_fr,batmanf,"Batman Forever (4.0 French)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (NL) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_nl, "batnovd.401",CRC(6ae4570c) SHA1(e863d6d0963910a993f2a0b8ddeefba48d304ca6))
DE_DMD64ROM88(        "bfdrom0f.401",CRC(e7473f6f) SHA1(f5951a9b6a8776073adf10e38b9d68d6d444240a),
                      "bfdrom3f.401",CRC(f7951709) SHA1(ace5b374d1e382d6f612b2bafc0e9fdde9e21014))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_nl input_ports_batmanf
#define init_bmf_nl        init_batmanf
CORE_CLONEDEFNV(bmf_nl,batmanf,"Batman Forever (4.0 Dutch)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (IT) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_it, "batnovi.401",CRC(7053ef9e) SHA1(918ab3e250b5965998ca0a38e1b8ba3cc012083f))
DE_DMD64ROM88(        "bfdrom0i.401",CRC(23051253) SHA1(155669a3fecd6e67838b10e71a57a6b871c8762a),
                      "bfdrom3i.401",CRC(82b61a41) SHA1(818c8fdbf44e29fe0ec5362a34ac948e98002efa))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_it input_ports_batmanf
#define init_bmf_it        init_batmanf
CORE_CLONEDEFNV(bmf_it,batmanf,"Batman Forever (4.0 Italian)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (SP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_sp, "batnova.401",CRC(4e62df4e) SHA1(6c3be65fc8825f47cd08755b58fdcf3652ede702))
DE_DMD64ROM88(        "bfdrom0l.401",CRC(b22b10d9) SHA1(c8f5637b00b0701d47a3b6bc0fdae08ae1a8df64),
                      "bfdrom3l.401",CRC(016b8666) SHA1(c10b7fc2c1e5b8382ff5b021a6b70f3a550b190e))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_sp input_ports_batmanf
#define init_bmf_sp        init_batmanf
CORE_CLONEDEFNV(bmf_sp,batmanf,"Batman Forever (4.0 Spanish)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (JP) - CPU Rev 3b /DMD  Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_jp, "batnovj.401",CRC(eef9bef0) SHA1(ac37ae12673351be939a969ecbc5b68c3995dca0))
DE_DMD64ROM88(        "bfdrom0a.401",CRC(8a3c20ad) SHA1(37415ac7ba178981dffce3a17502f39ab29d90ea),
                      "bfdrom3a.401",CRC(5ef46847) SHA1(a80f241db3d309f0bcb455051e33fc2b74e2ddcd))
DE2S_SOUNDROM144(     "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                      "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                      "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_jp input_ports_batmanf
#define init_bmf_jp        init_batmanf
CORE_CLONEDEFNV(bmf_jp,batmanf,"Batman Forever (4.0 Japanese)",1995,"Sega",de_mDEDMD64S2A,0)

/*-------------------------------------------------------------
/ Batman Forever (Timed Version) - CPU Rev 3b / DMD Type 3 2x512K Rom - 64K CPU Rom
/------------------------------------------------------------*/
DE_ROMSTARTx0(bmf_time, "batnova.401", CRC(4e62df4e) SHA1(6c3be65fc8825f47cd08755b58fdcf3652ede702))
DE_DMD64ROM88(          "bfdrom0t.401",CRC(b83b8d28) SHA1(b90e6a6fa55dadbf0e752745b87d1e8e9d7ccfa7),
                        "bfdrom3t.401",CRC(a024b1a5) SHA1(2fc8697fa98b7de7a844ca4d6a162b96cc751447))
DE2S_SOUNDROM144(       "bmfu7.bin"  ,CRC(58c0d144) SHA1(88a404d3625c7c154892282598b4949ac97de12b),
                        "bmfu17.bin" ,CRC(edcd5c10) SHA1(561f22fb7817f64e09ef6adda646f58f31b80bf4),
                        "bmfu21.bin" ,CRC(e41a516d) SHA1(9c41803a01046e57f8bd8759fe5e62ad6abaa80c))
DE_ROMEND
#define input_ports_bmf_time input_ports_batmanf
#define init_bmf_time        init_batmanf
CORE_CLONEDEFNV(bmf_time,batmanf,"Batman Forever (4.0 Timed Play)",1995,"Sega",de_mDEDMD64S2A,0)


/***********************************************************************/
/*************** SPECIAL TEST CHIP - NO DISPLAY ************************/
/***********************************************************************/
/* NO OUTPUT */
static struct core_dispLayout de_NoOutput[] = {{0}};
/*-------------------------------------------------------------
/ Data East Test Chip 64K ROM
/------------------------------------------------------------*/
INITGAMES11(detest, GEN_DE, de_NoOutput, FLIP1516, 0, 0, 0)
DE_ROMSTARTx0(detest,"de_test.512",CRC(bade8ca8) SHA1(e7e9d6622b9c9b9381ba2793297f87f102214972))
DE_ROMEND
#define input_ports_detest input_ports_des11
CORE_GAMEDEFNV(detest,"Data East Test Chip",1998,"Data East",de_mDEA,0)
