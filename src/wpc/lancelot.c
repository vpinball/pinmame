/************************************************************************************************
 Lancelot (by Peyper, 1994)
 --------------------------
   Hardware:
   ---------
		CPU:     Z80 compatible (Z80B-8400BPS) @ 5 MHz
			INT: IRQ @ ~1059 Hz (fed by NE555 timer)
		IO:      Z80 ports
		DISPLAY: 7-segment panels, direct segment access
		SOUND:	 MSM6295, YMF262, YAC512, running off TMP91P640 microcontroller (with undumped ROM)
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/262intf.h"
#include "core.h"
#include "sndbrd.h"

#define LANCELOT_IRQFREQ    1059 /* IRQ frequency */
#define LANCELOT_CPUFREQ 5000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT8  dispData[6];
  UINT8  comma[3];
  UINT8  col;
  UINT32 sndCmd[20];
  int    collect;
  int    sndLevel;
  UINT16 musicStart;
  UINT16 musicStop;
  mame_timer *sndtimer;
  mame_timer *sndtimer2;
} locals;

static INTERRUPT_GEN(LANCELOT_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(LANCELOT_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));

  if ((locals.vblankCount % 4) == 0) {
    coreGlobals.solenoids &= 0xffff0000;
  }

  core_updateSw(core_getSol(17));
}

static WRITE_HANDLER(OKI_w) {
  if (data & 0x80) {
    OKIM6295_data_0_w(0, 0x40);
    OKIM6295_data_0_w(0, 0x80 | (data & 0x3f));
    OKIM6295_data_0_w(0, 0x80);
  } else if (data & 0x40) {
    OKIM6295_data_0_w(0, 0x20);
    OKIM6295_data_0_w(0, 0x80 | (data & 0x3f));
    OKIM6295_data_0_w(0, 0x40);
  } else if (data & 0x20) {
    OKIM6295_data_0_w(0, 0x10);
    OKIM6295_data_0_w(0, 0x80 | (data & 0x3f));
    OKIM6295_data_0_w(0, 0x20);
  } else {
    OKIM6295_data_0_w(0, 0x08);
    OKIM6295_data_0_w(0, 0x80 | (data & 0x3f));
    OKIM6295_data_0_w(0, 0x10);
  }
}

static void music_play(int n) {
  if (locals.musicStart >= locals.musicStop) {
    timer_adjust(locals.sndtimer2, TIME_NEVER, 0, TIME_NEVER);
    return;
  }
//printf("%04x\r\r\r\r", locals.musicStart);
  YMF262_data_A_0_w(0, memory_region(REGION_USER2)[locals.musicStart]);
  locals.musicStart++;
}

static void timer_callback(int n) {
  if (locals.sndLevel < 1) {
    timer_adjust(locals.sndtimer, TIME_NEVER, 0, TIME_NEVER);
    return;
  }
  if (!(OKIM6295_status_0_r(0) & 0x0f)) { // once OKI chip isn't playing anymore, continue
    OKI_w(0, locals.sndCmd[--locals.sndLevel]);
  }
}

static WRITE_HANDLER(snd_w) {
  if (!data || data == 0x80) return;
  if (data > 0x80 && data < 0x90) { // music, probably won't play at all without the microcontroller's ROM
    locals.musicStart = memory_region(REGION_USER2)[data - 0x80] + ((UINT16)memory_region(REGION_USER2)[1 + data - 0xf80] << 8);
    locals.musicStop = memory_region(REGION_USER2)[4 + data - 0x80] + ((UINT16)memory_region(REGION_USER2)[5 + data - 0x80] << 8);
    timer_adjust(locals.sndtimer2, 0, 0, 0.001);
    return;
  }
  if (data >= 0xf0) { // music, probably won't play at all without the microcontroller's ROM
    locals.musicStart = memory_region(REGION_USER2)[data - 0xf0] + ((UINT16)memory_region(REGION_USER2)[1 + data - 0xf0] << 8);
    locals.musicStop = memory_region(REGION_USER2)[4 + data - 0xf0] + ((UINT16)memory_region(REGION_USER2)[5 + data - 0xf0] << 8);
    timer_adjust(locals.sndtimer2, 0, 0, 0.001);
    return;
  }
  if (data == 0xe4) { // begin collecting sound commands
    locals.collect = 1;
    locals.sndLevel = 0;
    return;
  } else if (data == 0xe6) { // end collecting sound commands, play back sequence now
    locals.collect = 0;
    timer_adjust(locals.sndtimer, 0.15, 0, 0.15);
    return;
  } else if (data >= 0xe0 && data != 0xea) { // play single delayed command
    locals.collect = 0;
    locals.sndCmd[0] = data;
    locals.sndLevel = 1;
    timer_adjust(locals.sndtimer, 0.15, 0, 0.15);
    return;
  }
  if (locals.collect) { // add to sound command collection
    locals.sndCmd[locals.sndLevel++] = data;
    return;
  }
  OKI_w(0, data);
}

static SWITCH_UPDATE(LANCELOT) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xf0, 5);
  }
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[1 + offset];
}

static READ_HANDLER(sw5_r) {
  return ~coreGlobals.swMatrix[5];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(offset);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(lamp2_w) {
  coreGlobals.tmpLampMatrix[8 + offset] = data;
}

static WRITE_HANDLER(lamp3_w) {
  coreGlobals.tmpLampMatrix[12] = data;
  coreGlobals.segments[48].w = data & 0x10 ? core_bcd2seg7[0] : 0;
  coreGlobals.segments[49].w = data & 0x40 ? core_bcd2seg7[0] : 0;
  coreGlobals.segments[50].w = data & 0x80 ? core_bcd2seg7[0] : 0;
}

static WRITE_HANDLER(sol_w) {
  if (data & 0x0f) {
  	locals.vblankCount = 0;
    coreGlobals.solenoids |= 1 << ((data & 0x0f) - 1);
  }
}

static WRITE_HANDLER(sol2_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x000fffff) | (data << 20);
}

static WRITE_HANDLER(disp_w) {
  locals.dispData[offset] = data;
}

static WRITE_HANDLER(col_w) {
  int i, seg;
  locals.col = data & 0x0f;
  if (locals.col) {
    for (i = 0; i < 6; i++) {
    	seg = (5 - i) * 8 + 8 - locals.col;
      coreGlobals.segments[seg].w = locals.dispData[5 - i];
      // make up for a silly flaw in code; comma segment is contained in another digit!
      if (seg == 41 || seg == 44 || seg == 47) {
        locals.comma[(seg - 41) / 3] = coreGlobals.segments[seg].w & 0x80;
        coreGlobals.segments[seg].w &= 0x7f;
      } else if (seg == 40 || seg == 43 || seg == 46) {
        coreGlobals.segments[seg].w |= locals.comma[(seg - 40) / 3];
      }
    }
  }
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xfff0ffff) | ((data & 0xf0) << 12);
}

static WRITE_HANDLER(p50_w) {
//printf("%d:%02x ", locals.col, data); if (locals.col == 8) printf("\n");
}

static MEMORY_READ_START(LANCELOT_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0x8000,0x87ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(LANCELOT_writemem)
  {0x8000,0x87ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(LANCELOT_readport)
  {0x08,0x0b, sw_r},
  {0x21,0x22, dip_r},
  {0x29,0x29, sw5_r},
PORT_END

static PORT_WRITE_START(LANCELOT_writeport)
  {0x00,0x05, disp_w},
  {0x06,0x06, lamp3_w},
  {0x10,0x17, lamp_w},
  {0x18,0x18, sol_w},
  {0x2a,0x2a, sol2_w},
  {0x30,0x33, lamp2_w},
  {0x34,0x34, snd_w},
  {0x40,0x40, col_w},
  {0x50,0x50, p50_w},
PORT_END

static struct OKIM6295interface okim6295_interface = {
	1,					/* 1 chip */
	{ 1056000./132. },	/* base frequency, 1056 kHz / 132 */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};

static void ymf_irq(int irq) {
//printf("\nI%d\n", irq);
  if (irq)
    timer_adjust(locals.sndtimer2, TIME_NEVER, 0, TIME_NEVER);
}

static struct YMF262interface ymf262_interface = {
	1,
	14318180,
	{ YAC512_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) }, /* channels A,B output from DOAB pin (#21 on YMF262-M) */
	{ YAC512_VOL(0,MIXER_PAN_CENTER,0,MIXER_PAN_CENTER) },	/* channels C,D output from DOCD pin (#22 on YMF262-M) */
	{ ymf_irq }
};

static MACHINE_STOP(LANCELOT) {
  timer_remove(locals.sndtimer);
  timer_remove(locals.sndtimer2);
}

static MACHINE_INIT(LANCELOT) {
  int i;
  memset(&locals, 0, sizeof locals);
  locals.sndtimer = timer_alloc(timer_callback);
  timer_adjust(locals.sndtimer, TIME_NEVER, 0, TIME_NEVER);
  // preset YMF262 to some default values
  YMF262_register_B_0_w(0, 0x05);
  YMF262_data_B_0_w(0, 0x01);
  YMF262_register_B_0_w(0, 0x01);
  YMF262_data_B_0_w(0, 0x00);
  YMF262_register_B_0_w(0, 0x04);
  YMF262_data_B_0_w(0, 0x3f);
  YMF262_register_A_0_w(0, 0x01);
  YMF262_data_A_0_w(0, 0x00);
  YMF262_register_A_0_w(0, 0x02);
  YMF262_data_A_0_w(0, 0x30);
  YMF262_register_A_0_w(0, 0x03);
  YMF262_data_A_0_w(0, 0xcd);
  YMF262_register_A_0_w(0, 0x04);
  YMF262_data_A_0_w(0, 0x03);
  YMF262_register_A_0_w(0, 0x08);
  YMF262_data_A_0_w(0, 0x00);
  for (i = 0x80; i < 0x86; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x88; i < 0x8e; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x90; i < 0x96; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x40; i < 0x46; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x48; i < 0x4e; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x50; i < 0x56; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xff);
  }
  for (i = 0x80; i < 0x86; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0x88; i < 0x8e; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0x90; i < 0x96; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0x40; i < 0x46; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0x48; i < 0x4e; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0x50; i < 0x56; i++) {
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xff);
  }
  for (i = 0xc0; i < 0xc9; i++) {
    YMF262_register_A_0_w(0, i);
    YMF262_data_A_0_w(0, 0xf0);
    YMF262_register_B_0_w(0, i);
    YMF262_data_B_0_w(0, 0xf0);
  }
  locals.sndtimer2 = timer_alloc(music_play);
}

MACHINE_DRIVER_START(LANCELOT)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, LANCELOT_CPUFREQ)
  MDRV_CPU_MEMORY(LANCELOT_readmem, LANCELOT_writemem)
  MDRV_CPU_PORTS(LANCELOT_readport, LANCELOT_writeport)
  MDRV_CPU_VBLANK_INT(LANCELOT_vblank, 1)
  MDRV_CPU_PERIODIC_INT(LANCELOT_irq, LANCELOT_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(LANCELOT,NULL,LANCELOT)
  MDRV_DIPS(16)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_SWITCH_UPDATE(LANCELOT)

  MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
  MDRV_SOUND_ADD(YMF262, ymf262_interface)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,8,CORE_SEG8},
  {3, 0, 8,8,CORE_SEG8},
  {6, 0,16,8,CORE_SEG8},
  {9, 0,24,8,CORE_SEG8},
  {0,18,39,1,CORE_SEG8},
  {0,28,37,2,CORE_SEG7SC},
  {10,23,35,1,CORE_SEG7SC},{10,26,36,1,CORE_SEG7SC},
  {3,24,45,3,CORE_SEG7SC},{3,30,48,1,CORE_SEG7SC},{3,32,48,1,CORE_SEG7SC},{3,34,48,1,CORE_SEG7SC},{3,36,48,1,CORE_SEG7SC},{3,38,48,1,CORE_SEG7SC},
  {5,24,42,3,CORE_SEG7SC},{5,30,49,1,CORE_SEG7SC},{5,32,49,1,CORE_SEG7SC},{5,34,49,1,CORE_SEG7SC},{5,36,49,1,CORE_SEG7SC},{5,38,49,1,CORE_SEG7SC},
  {7,24,34,1,CORE_SEG7SC},{7,26,40,2,CORE_SEG7SC},{7,30,50,1,CORE_SEG7SC},{7,32,50,1,CORE_SEG7SC},{7,34,50,1,CORE_SEG7SC},{7,36,50,1,CORE_SEG7SC},{7,38,50,1,CORE_SEG7SC},
  {0}
};
INPUT_PORTS_START(lancelot)
  CORE_PORTS
  SIM_PORTS(3)
  PORT_START /* 0 */
    COREPORT_BITDEF(  0x0010, IPT_START1,         IP_KEY_DEFAULT)
    COREPORT_BITDEF(  0x0020, IPT_COIN1,          IP_KEY_DEFAULT)
    COREPORT_BITDEF(  0x0040, IPT_COIN2,          IP_KEY_DEFAULT)
    COREPORT_BITTOG(  0x0080, "Run / Test",       KEYCODE_0)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x8000, 0x0000, "SW1-1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x8000, "1" )
    COREPORT_DIPNAME( 0x4000, 0x4000, "SW1-2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x4000, "1" )
    COREPORT_DIPNAME( 0x2000, 0x0000, "SW1-3")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x2000, "1" )
    COREPORT_DIPNAME( 0x1000, 0x0000, "SW1-4")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x1000, "1" )
    COREPORT_DIPNAME( 0x0800, 0x0000, "SW1-5")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0800, "1" )
    COREPORT_DIPNAME( 0x0400, 0x0400, "SW1-6")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0400, "1" )
    COREPORT_DIPNAME( 0x0200, 0x0200, "SW1-7")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0200, "1" )
    COREPORT_DIPNAME( 0x0100, 0x0000, "SW1-8")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0100, "1" )
    COREPORT_DIPNAME( 0x0080, 0x0080, "SW2-1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0080, "1" )
    COREPORT_DIPNAME( 0x0040, 0x0040, "SW2-2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0040, "1" )
    COREPORT_DIPNAME( 0x0020, 0x0020, "SW2-3")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0020, "1" )
    COREPORT_DIPNAME( 0x0010, 0x0010, "SW2-4")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0010, "1" )
    COREPORT_DIPNAME( 0x0008, 0x0008, "SW2-5")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0008, "1" )
    COREPORT_DIPNAME( 0x0004, 0x0000, "SW2-6")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0004, "1" )
    COREPORT_DIPNAME( 0x0002, 0x0000, "SW2-7")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0002, "1" )
    COREPORT_DIPNAME( 0x0001, 0x0000, "SW2-8")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0001, "1" )
INPUT_PORTS_END
static core_tGameData lancelotGameData = {0,disp,{FLIP_SWNO(35,34),0,5,0,SNDBRD_NONE}};
static void init_lancelot(void) {
  core_gameData = &lancelotGameData;
}
ROM_START(lancelot)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("lancelot.bin", 0x0000, 0x8000, CRC(26c10926) SHA1(ad032b43c15b1d7a7f32a12ca09ea3344d75105b))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("tmp91640.rom", 0x0000, 0x4000, NO_DUMP)
  NORMALREGION(0x40000, REGION_USER1)
    ROM_LOAD("snd_u3.bin", 0x00000, 0x20000, CRC(db88c28d) SHA1(35a80509c4a1f931d07af2fc74adbafc11af5639))
    ROM_LOAD("snd_u4.bin", 0x20000, 0x20000, CRC(5cebed6e) SHA1(d11cc57fadee95f056fc65927fa1f6ff0f337446))
  NORMALREGION(0x20000, REGION_USER2)
    ROM_LOAD("snd_u5.bin", 0x00000, 0x20000, CRC(bf141441) SHA1(630b852bb3bba0fcdae13ae548b1e9810bc64d7d))
ROM_END
CORE_GAMEDEFNV(lancelot,"Sir Lancelot",1994,"Peyper (Spain)",LANCELOT,GAME_IMPERFECT_SOUND)
