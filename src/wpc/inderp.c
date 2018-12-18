/************************************************************************************************
 Inder (early games, "Indertronic B-1")
 --------------------------------------

   Hardware:
   ---------
		CPU:     M6502, R/C combo with 100kOhms and 10pF for clock, manual says 4 microseconds min. instruction execution
			INT:   ? (somewhere in between 200 .. 250 Hz seems likely)
		IO:      DMA only
		DISPLAY: 6-digit, both 9-segment & 7-segment panels with direct segment access
		SOUND:	 simple tones, needs comparison with real machine
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "inder.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
	UINT16 digit[5];
  int vblankCount, sndTimer;
} locals;

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(INDERP_vblank) {
  locals.vblankCount++;
	if (locals.vblankCount % 4 == 0) {
		coreGlobals.solenoids &= 0xffff00;
	}
  locals.sndTimer++;
	if (locals.sndTimer % 6 == 0) {
		coreGlobals.solenoids &= 0x000fff;
    discrete_sound_w(0x01, 0);
    discrete_sound_w(0x02, 0);
    discrete_sound_w(0x04, 0);
    discrete_sound_w(0x08, 0);
    discrete_sound_w(0x10, 0);
    discrete_sound_w(0x20, 0);
    discrete_sound_w(0x40, 0);
    discrete_sound_w(0x80, 0);
  }
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));

  core_updateSw(core_getSol(9));
}

static INTERRUPT_GEN(INDERP_irq) {
  cpu_set_irq_line(INDER_CPU, 0, PULSE_LINE);
}

static SWITCH_UPDATE(INDERP) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x4f, 3);
  }
}

static READ_HANDLER(sws_r) {
  return ~coreGlobals.swMatrix[offset + 1];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(2 - offset);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
  if (!offset) {
  	coreGlobals.solenoids = (coreGlobals.solenoids & 0xfffcff) | ((data & 0x03) << 8);
  }
}

static WRITE_HANDLER(strobe_w) {
	static int posMap[10] = { 0, 6, 7, 8, 5, 4, 2, 9, 3, 1 };
	int pos;
	memory_region(REGION_CPU1)[0xab] = data;
  pos = data ? 1 + core_BitColToNum(data) : 0;
  coreGlobals.segments[posMap[pos]].w = locals.digit[0];
  coreGlobals.segments[10 + posMap[pos]].w = locals.digit[1];
  coreGlobals.segments[20 + posMap[pos]].w = locals.digit[2];
  coreGlobals.segments[30 + posMap[pos]].w = locals.digit[3];
  coreGlobals.segments[40 + posMap[pos]].w = locals.digit[4];
}

static WRITE_HANDLER(disp_w) {
  locals.digit[offset] = (data & 0x7f) | ((data & 0x80) << 1) | ((data & 0x80) << 2);
}

static WRITE_HANDLER(sol0_w) {
	locals.vblankCount = 0;
  coreGlobals.solenoids |= 1 << data;
}

static WRITE_HANDLER(sol1_w) {
	locals.sndTimer = 0;
	if (data < 8) {
	  coreGlobals.solenoids = (coreGlobals.solenoids & 0xffff) | (0x10000 << data);
	  discrete_sound_w(0x01, 0);
	  discrete_sound_w(0x02, 0);
	  discrete_sound_w(0x04, 0);
	  discrete_sound_w(0x08, 0);
	  discrete_sound_w(0x10, 0);
	  discrete_sound_w(0x20, 0);
	  discrete_sound_w(0x40, 0);
	  discrete_sound_w(0x80, 0);
	  discrete_sound_w(1 << data, 1);
	} else {
    coreGlobals.solenoids |= 0x1000; // knocker (sol. 13)
	}
}

static MEMORY_READ_START(INDERP_readmem)
  {0x0000,0x00ff, MRA_RAM},
  {0x0180,0x01ff, MRA_RAM},
  {0x0300,0x0302, dip_r},
  {0x0303,0x0309, sws_r},
  {0x8000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(INDERP_writemem)
  {0x00ab,0x00ab, strobe_w},
  {0x0000,0x00ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x0180,0x01ff, MWA_RAM},
  {0x0200,0x0204, disp_w},
  {0x0205,0x030f, MWA_NOP},
  {0x0310,0x0310, sol0_w},
  {0x0320,0x0320, sol1_w},
  {0x0330,0x0337, lamp_w},
MEMORY_END

static MACHINE_INIT(INDERP) {
  memset(&locals, 0, sizeof locals);
}

static DISCRETE_SOUND_START(inder_tones)
	DISCRETE_INPUT(NODE_01,0x01,0xff,0)
	DISCRETE_INPUT(NODE_02,0x02,0xff,0)
	DISCRETE_INPUT(NODE_03,0x04,0xff,0)
	DISCRETE_INPUT(NODE_04,0x08,0xff,0)
	DISCRETE_INPUT(NODE_05,0x10,0xff,0)
	DISCRETE_INPUT(NODE_06,0x20,0xff,0)
	DISCRETE_INPUT(NODE_07,0x40,0xff,0)
	DISCRETE_INPUT(NODE_08,0x80,0xff,0)

	DISCRETE_TRIANGLEWAVE(NODE_10,NODE_01,262,50000,10000,0) // C'
	DISCRETE_TRIANGLEWAVE(NODE_20,NODE_02,294,50000,10000,0) // D'
	DISCRETE_TRIANGLEWAVE(NODE_30,NODE_03,330,50000,10000,0) // E'
	DISCRETE_TRIANGLEWAVE(NODE_40,NODE_04,349,50000,10000,0) // F'
	DISCRETE_TRIANGLEWAVE(NODE_50,NODE_05,392,50000,10000,0) // G'
	DISCRETE_TRIANGLEWAVE(NODE_60,NODE_06,440,50000,10000,0) // A'
	DISCRETE_TRIANGLEWAVE(NODE_70,NODE_07,494,50000,10000,0) // H'
	DISCRETE_TRIANGLEWAVE(NODE_80,NODE_08,523,50000,10000,0) // C"

	DISCRETE_ADDER4(NODE_90,1,NODE_10,NODE_20,NODE_30,NODE_40)
	DISCRETE_ADDER4(NODE_91,1,NODE_50,NODE_60,NODE_70,NODE_80)

	DISCRETE_OUTPUT_STEREO(NODE_90,NODE_91,75)
DISCRETE_SOUND_END

// switches start at 30 for column 1, and each column adds 10.
static int INDERP_sw2m(int no) { return (no/10 - 2)*8 + no%10; }
static int INDERP_m2sw(int col, int row) { return 20 + col*10 + row; }

MACHINE_DRIVER_START(INDERP)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6502, 1000000) // guessed
  MDRV_CPU_MEMORY(INDERP_readmem, INDERP_writemem)
  MDRV_CPU_VBLANK_INT(INDERP_vblank, 1)
  MDRV_CPU_PERIODIC_INT(INDERP_irq, 220) // guessed, seems to work OK for sound
  MDRV_CORE_INIT_RESET_STOP(INDERP,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_CONV(INDERP_sw2m,INDERP_m2sw)
  MDRV_SWITCH_UPDATE(INDERP)
  MDRV_DIPS(24)
  MDRV_SOUND_ADD(DISCRETE, inder_tones)
MACHINE_DRIVER_END

static core_tLCDLayout inderDispP1[] = {
  {0, 0,10,6,CORE_SEG9},
  {1,24,20,2,CORE_SEG7S},{1,30,22,2,CORE_SEG7S},
  {0}
};

static core_tLCDLayout inderDispP4[] = {
  {0, 0, 0,6,CORE_SEG9},
  {3, 0,10,6,CORE_SEG9},
  {6, 0,20,6,CORE_SEG9},
  {9, 0,30,6,CORE_SEG9},
  {1,24,40,2,CORE_SEG7S},{1,30,44,2,CORE_SEG7S},
  {0}
};

static core_tLCDLayout inderDispP4a[] = {
  {0, 0, 0,6,CORE_SEG9}, {2, 0,10,6,CORE_SEG9},
  {7, 0,20,6,CORE_SEG9}, {9, 0,30,6,CORE_SEG9},
  {5, 3,40,2,CORE_SEG7S},{5, 9,44,2,CORE_SEG7S},
  {0}
};

#define INDERP_INPUT_PORTS(name) \
INPUT_PORTS_START(name) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0008, IPT_START1,    IP_KEY_DEFAULT) \
    PORT_BITX      (0x0001, IP_ACTIVE_LOW, IPT_BUTTON1, "Coin 1", KEYCODE_3, IP_JOY_NONE) \
    PORT_BITX      (0x0002, IP_ACTIVE_LOW, IPT_BUTTON1, "Coin 2", KEYCODE_4, IP_JOY_NONE) \
    PORT_BITX      (0x0004, IP_ACTIVE_LOW, IPT_BUTTON1, "Coin 3", KEYCODE_5, IP_JOY_NONE) \
    COREPORT_BIT   (0x0040, "Tilt",        KEYCODE_INSERT) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0008, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0004, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0010, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
INPUT_PORTS_END

/*-------------------------------------------------------------------
/ Centaur (1979)
/-------------------------------------------------------------------*/
static core_tGameData centauriGameData = {0,inderDispP1,{FLIP_SW(FLIP_L)}};
static void init_centauri(void) {
	core_gameData = &centauriGameData;
}
INDERP_INPUT_PORTS(centauri)
INDER_ROMSTARTP(centauri,	"cent2.bin", CRC(4f1ad0bc) SHA1(0622c16520275e629eac27ae2575e4e433de56ed),
						"cent3.bin", CRC(f87abd63) SHA1(c3f48ffd46fad076fd064cbc0fdcc31641f5b1b6),
						"cent4.bin", CRC(b69e95b6) SHA1(2f053a5848110d084239e1fc960198b247b3b98e))
INDER_ROMEND
CORE_GAMEDEFNV(centauri,"Centaur (Inder)",1979,"Inder (Spain)",INDERP,0)

#define init_centaurj init_centauri
INDERP_INPUT_PORTS(centaurj)
INDER_ROMSTARTP(centaurj,	"cent2.bin", CRC(4f1ad0bc) SHA1(0622c16520275e629eac27ae2575e4e433de56ed),
						"cent3a.bin", CRC(7b8215b1) SHA1(7cb6c18ad88060b56785bbde398bff157d8417cd),
						"cent4a.bin", CRC(7ee64ea6) SHA1(b751b757faab7e3bb56625e4d72c3aeeb84a3f28))
INDER_ROMEND
CORE_CLONEDEFNV(centaurj,centauri,"Centaur (Inder, alternate set)",1979,"Inder (Spain)",INDERP,0)

/*-------------------------------------------------------------------
/ Topaz (1979)
/-------------------------------------------------------------------*/
static core_tGameData topaziGameData = {0,inderDispP4,{FLIP_SW(FLIP_L)}};
static void init_topazi(void) {
	core_gameData = &topaziGameData;
}
INDERP_INPUT_PORTS(topazi)
ROM_START(topazi)
  NORMALREGION(0x10000, INDER_MEMREG_CPU)
    ROM_LOAD("topaz0.bin", 0x8400, 0x0400, CRC(d047aee0) SHA1(b2bc2e9fb088006fd3b7eb080feaa1eac479af58))
    ROM_LOAD("topaz1.bin", 0x8800, 0x0400, CRC(72a423c2) SHA1(e3ba5d581739fc0871901f861a7692fd86e0f6aa))
    ROM_LOAD("topaz2.bin", 0x8c00, 0x0400, CRC(b8d2e7c6) SHA1(e19bec04fab15536fea51c4298c6a4cb3817630c))
      ROM_RELOAD(0xfc00, 0x0400)
INDER_ROMEND
CORE_GAMEDEFNV(topazi,"Topaz (Inder)",1979,"Inder (Spain)",INDERP,0)

/*-------------------------------------------------------------------
/ Skate Board (1980)
/-------------------------------------------------------------------*/
static core_tGameData skatebrdGameData = {0,inderDispP4a,{FLIP_SW(FLIP_L)}};
static void init_skatebrd(void) {
	core_gameData = &skatebrdGameData;
}
INDERP_INPUT_PORTS(skatebrd)
INDER_ROMSTARTP(skatebrd,	"skate2.bin", CRC(ee9b4c4c) SHA1(1a8b2ef8dfead18bfc62e85474dab2838b73ce08),
						"skate3.bin", CRC(58e181fe) SHA1(f54c8099100d0c96dc2ddbae8db9293f8581d459),
						"skate4.bin", CRC(fcdccffe) SHA1(a2db53f7bc555d705aa894e62307590fd74067dd))
INDER_ROMEND
CORE_GAMEDEFNV(skatebrd,"Skate Board",1980,"Inder (Spain)",INDERP,0)
