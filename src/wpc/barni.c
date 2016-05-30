/************************************************************************************************
 Barni Pinball
 -----------------

   Hardware:
		CPU: 2 x M6809, optional M6802 (what for?)
			 INT: IRQ on CPU 0, FIRQ on CPU 1
		IO: DMA (Direct Memory Access/Address)
		    2x PIA 6821
		    1x VIS 6522
		DISPLAY: 5x6 digit 7 or 16 segment display
		SOUND:	 basically the same as Bally's Squalk & Talk -61 board but missing AY8912 synth chip

************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "machine/6821pia.h"
#include "sound/tms5220.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT8 strobe1;
  UINT8 strobe2;
} locals;

static INTERRUPT_GEN(barni_irq) {
  static int state;
  cpu_set_irq_line(0, 0, (state = !state) ? ASSERT_LINE : CLEAR_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(barni_vblank) {
  core_updateSw(TRUE);
}

static SWITCH_UPDATE(barni) {
	if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x01, 9);
	}
  sndbrd_0_diag(coreGlobals.swMatrix[9]);
#ifdef MAME_DEBUG
  if (keyboard_pressed(KEYCODE_X)) {
    cpu_set_irq_line(0, 1, ASSERT_LINE);
  } else {
    cpu_set_irq_line(0, 1, CLEAR_LINE);
  }
  if (keyboard_pressed(KEYCODE_C)) {
    cpu_set_nmi_line(0, ASSERT_LINE);
  } else {
    cpu_set_nmi_line(0, CLEAR_LINE);
  }
#endif
}

static int BARNI_sw2m(int no) {
	return no + 8;
}

static int BARNI_m2sw(int col, int row) {
	return col*8 + row - 8;
}

static READ_HANDLER(dip0_r) {
  return core_getDip(0);
}

static READ_HANDLER(dip1_r) {
  return core_getDip(1);
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[locals.strobe1];
}

static const UINT16 core_ascii2seg[] = {
  /* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x20-0x27 */ 0x0000, 0x0309, 0x0220, 0x2A4E, 0x2A6D, 0x6E65, 0x135D, 0x0400,
  /* 0x28-0x2f */ 0x1400, 0x4100, 0x7F40, 0x2A40, 0x0000, 0x0840, 0x0000, 0x4400,
  /* 0x30-0x37 */ 0x003f, 0x2200, 0x085B, 0x084f, 0x0866, 0x086D, 0x087D, 0x0007,
  /* 0x38-0x3f */ 0x087F, 0x086F, 0x0009, 0x4001, 0x4408, 0x0848, 0x1108, 0x2803,
  /* 0x40-0x47 */ 0x205F, 0x0877, 0x2A0F, 0x0039, 0x220F, 0x0079, 0x0071, 0x083D,
  /* 0x48-0x4f */ 0x0876, 0x2209, 0x001E, 0x1470, 0x0038, 0x0536, 0x1136, 0x003f,
  /* 0x50-0x57 */ 0x0873, 0x103F, 0x1873, 0x086D, 0x2201, 0x003E, 0x4430, 0x5036,
  /* 0x58-0x5f */ 0x5500, 0x2500, 0x4409, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

static UINT8 ram[0xff];
static READ_HANDLER(ram_r) {
  if (offset == 0x7f) {
    ram[offset] = coreGlobals.swMatrix[0];
  } else if (offset == 0x7e) {
    ram[offset] = coreGlobals.swMatrix[1];
  } else if ((offset > 0x1f && offset < 0x63) || (offset > 0x6c && offset < 0xe1) || offset > 0xe1) {
    logerror("R %02x\n", offset);
  }
  return ram[offset];
}
static WRITE_HANDLER(ram_w) {
  memory_region(REGION_CPU1)[0xa000 + offset] = data;
  ram[offset] = data;
  if (offset < 0x20) {
    coreGlobals.segments[offset].w = core_ascii2seg[data];
  } else if (offset > 0x62 && offset < 0x6d) {
    coreGlobals.lampMatrix[offset - 0x63] = data;
  } else if (offset != 0xab) {
    locals.strobe1 = 1 + core_BitColToNum(data);
  } else if (offset != 0xac) {
    locals.strobe2 = data;
  } else if (offset != 0xe1 && offset != 0xe2 && offset != 0xe3) {
    logerror("W %02x: %02x\n", offset, data);
  }
}

static MEMORY_READ_START(barni_readmem1)
  {0x0000,0x00ff,	MRA_RAM}, // nvram?
  {0x4000,0x4000,	dip0_r},
  {0x6000,0x6000,	dip1_r},
  {0xa000,0xa0ff,	ram_r},
  {0xa100,0xa7ff,	MRA_RAM},
  {0xc000,0xffff,	MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(barni_writemem1)
  {0x0000,0x00ff,	MWA_RAM}, // nvram?
  {0xa000,0xa0ff,	ram_w},
  {0xa100,0xa7ff,	MWA_RAM},
  {0xc000,0xffff,	MWA_ROM},
MEMORY_END

static MEMORY_READ_START(barni_readmem2)
  {0x0000,0x03ff,	MRA_RAM},
  {0xe000,0xffff,	MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(barni_writemem2)
  {0x0000,0x03ff,	MWA_RAM},
  {0xe000,0xffff,	MWA_ROM},
MEMORY_END

static struct {
  int pia0a, pia0b, pia1a, pia1b, pia1cb1, pia1ca2;
  UINT8 cmd[2], lastcmd, lastctrl;
} sntlocals;

static WRITE_HANDLER(snd_pia0b_w) {
  logerror("snd PIA B: %02x\n", data);
}
static READ_HANDLER(snd_pia1a_r) { return sntlocals.pia1a; }
static WRITE_HANDLER(snd_pia1a_w) { sntlocals.pia1a = data; }
static WRITE_HANDLER(snd_pia1b_w) {
  if (sntlocals.pia1b & ~data & 0x01) { // read, overrides write command!
    sntlocals.pia1a = tms5220_status_r(0);
  } else if (sntlocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, sntlocals.pia1a);
  }
  sntlocals.pia1b = data;
  pia_set_input_ca2(3, tms5220_ready_r());
}
static READ_HANDLER(snd_pia1ca2_r) {
  return sntlocals.pia1ca2;
}
static READ_HANDLER(snd_pia1cb1_r) {
  return sntlocals.pia1cb1;
}

static void snd_irq(int state) {
  cpu_set_irq_line(2, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct pia6821_interface snd_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ PIA_UNUSED_VAL(0xff), PIA_UNUSED_VAL(0xff), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, snd_pia0b_w, 0, 0,
  /*irq: A/B           */ snd_irq, snd_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snd_pia1a_r, 0, PIA_UNUSED_VAL(1), snd_pia1cb1_r, snd_pia1ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snd_pia1a_w, snd_pia1b_w, 0, 0,
  /*irq: A/B           */ snd_irq, snd_irq
}};

static void snd_init(struct sndbrdData *brdData) {
  pia_config(2, PIA_STANDARD_ORDERING, &snd_pia[0]);
  pia_config(3, PIA_STANDARD_ORDERING, &snd_pia[1]);
  tms5220_set_variant(TMS5220_IS_5220); // schematics say TMS5200 but that sounds terrible!
}
static void snd_diag(int button) {
  cpu_set_nmi_line(2, button ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(snd_data_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(snd_ctrl_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(2, ~data & 0x01);
}
static WRITE_HANDLER(snd_manCmd_w) {
  sntlocals.lastcmd = data;  pia_set_input_cb1(2, 1); pia_set_input_cb1(2, 0);
}
const struct sndbrdIntf barniIntf = {
  "BARNI", snd_init, NULL, snd_diag, snd_manCmd_w, snd_data_w, NULL, snd_ctrl_w, NULL, 0
};
static READ_HANDLER(snd_cmd_r) {
  return sntlocals.lastcmd;
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(2) },
  { 0x0090, 0x0093, pia_r(3) },
  { 0x2000, 0x2000, snd_cmd_r },
  { 0x1000, 0x1000, MRA_NOP },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(2) },
  { 0x0090, 0x0093, pia_w(3) },
  { 0x1000, 0x1000, DAC_0_data_w },
MEMORY_END

static void snd_5220Irq(int state) { pia_set_input_cb1(3, (sntlocals.pia1cb1 = !state)); }
static void snd_5220Rdy(int state) { pia_set_input_ca2(3, (sntlocals.pia1ca2 = state)); }

static struct TMS5220interface snd_tms5220Int = { 639450, 75, snd_5220Irq, snd_5220Rdy };
static struct DACinterface     snd_dacInt = { 1, { 20 }};

static const struct pia6821_interface pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, 0, 0,
  /*irq: A/B           */ 0, 0
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, 0, 0,
  /*irq: A/B           */ 0, 0
}};

static MACHINE_INIT(barni) {
  memset(&locals, 0, sizeof(locals));
  pia_config(0, PIA_STANDARD_ORDERING, &pia[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &pia[1]);
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(REGION_SOUND1), NULL, NULL);
}

static MACHINE_STOP(barni) {
  sndbrd_0_exit();
}

MACHINE_DRIVER_START(barni)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6809, 2000000)
  MDRV_CPU_MEMORY(barni_readmem1, barni_writemem1)
  MDRV_CPU_PERIODIC_INT(barni_irq, 10)
  MDRV_CPU_ADD_TAG("hcpu", M6809, 2000000)
  MDRV_CPU_MEMORY(barni_readmem2, barni_writemem2)
  MDRV_CPU_VBLANK_INT(barni_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(barni,NULL,barni)
//  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(barni)

  MDRV_CPU_ADD_TAG("scpu", M6802, 3579545/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, snd_tms5220Int)
  MDRV_SOUND_ADD(DAC,     snd_dacInt)
MACHINE_DRIVER_END

/*--------------------------------
/ Red Baron
/-------------------------------*/
ROM_START(redbaron)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("redbaron.r1", 0xe000, 0x2000, CRC(fd401d3f) SHA1(33c0f178c798e16a9b4489a0e469f0a227882e79))
    ROM_LOAD("redbaron.r2", 0xc000, 0x2000, CRC(0506e53e) SHA1(a1eaa39181cb3e5a1c281d217d680a42e39c856a))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("redbaron.r3", 0xe000, 0x2000, CRC(45bca0b8) SHA1(77e2d6b04ea8d6fa7e30b59232696b9aa5307286))
  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("rbsnd1.732", 0xf000, 0x1000, CRC(674389ce) SHA1(595bbfe51dc3af266f4344e3865c0e48dd96acea))
    ROM_LOAD("rbsnd2.732", 0xe000, 0x1000, CRC(30e7da5e) SHA1(3054cf9b09e0f89c242e1ad35bb31d9bd77248e4))
    ROM_LOAD("rbsnd3.732", 0xd000, 0x1000, CRC(a4ba0f72) SHA1(e46148a2f5125914944973f37e73a62001c76aaa))
    ROM_LOAD("rbsnd4.732", 0xc000, 0x1000, CRC(fd8db899) SHA1(0978213f14f73ccc4a24eb42a39db00d9299c5d0))
ROM_END

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0, 6,CORE_SEG16}, {0,14, 6, 6,CORE_SEG16},
  {3, 0,12, 6,CORE_SEG16}, {3,14,18, 6,CORE_SEG16},
  {6, 0,24, 8,CORE_SEG16},
  {0}
};
static core_tGameData redbaronGameData = {0,dispAlpha,{FLIP_SWNO(0,0),0,0,0,SNDBRD_BARNI,0,0}};
static void init_redbaron(void) {
  core_gameData = &redbaronGameData;
}

INPUT_PORTS_START(barni)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0001, "Key 1",		KEYCODE_1)
    COREPORT_BIT(   0x0002, "Key 2",		KEYCODE_2)
    COREPORT_BIT(   0x0004, "Key 3",		KEYCODE_3)
    COREPORT_BIT(   0x0008, "Key 4",		KEYCODE_4)
    COREPORT_BIT(   0x0010, "Key 5",		KEYCODE_5)
    COREPORT_BIT(   0x0020, "Key 6",		KEYCODE_6)
    COREPORT_BIT(   0x0040, "Key 7",		KEYCODE_7)
    COREPORT_BIT(   0x0080, "Key 8",		KEYCODE_8)
    COREPORT_BIT(   0x0100, "Sound test",		KEYCODE_0)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
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
INPUT_PORTS_END

#define input_ports_redbaron input_ports_barni

CORE_GAMEDEFNV(redbaron, "Red Baron", 1985, "Barni", barni, GAME_NOT_WORKING)
