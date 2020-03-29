#include "driver.h"
#include "core.h"
#include "machine/6821pia.h"
#include "cpu/m6809/m6809.h"
#include "sound/ay8910.h"

static struct {
  int    cb10, ca11;
  int    timerSt;
  int    i8279cmd;
  int    i8279reg;
  UINT8  i8279ram[16];
  UINT8  sndCtrl, sndData;
} locals;

static READ_HANDLER(dip_r) {
  return core_getDip(0);
}

static READ_HANDLER(stick_r) {
  return ~coreGlobals.swMatrix[9];
}

static READ_HANDLER(snd_r) {
  if (locals.sndCtrl & 0x02)
    return AY8910Read(0);
  else if (locals.sndCtrl & 0x10)
    return AY8910Read(1);
  return 0;
}

static WRITE_HANDLER(snd_ctrl_w) {
  if (locals.sndCtrl & ~data & 0x04) {
    AY8910Write(0, (locals.sndCtrl & 0x01) ? 0 : 1, locals.sndData);
    pia_read(1, 0); // force a pia port A read to reset the IRQ!
  }
  if (locals.sndCtrl & ~data & 0x20) {
    AY8910Write(1, (locals.sndCtrl & 0x08) ? 0 : 1, locals.sndData);
    pia_read(1, 0); // force a pia port A read to reset the IRQ!
  }
  locals.sndCtrl = data;
}

static WRITE_HANDLER(snd_data_w) {
  locals.sndData = data;
}

static void ice_irq(int state) {
  cpu_set_irq_line(0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void ice_firq(int state) {
  cpu_set_irq_line(0, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct pia6821_interface ice_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ stick_r, dip_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, 0, 0,
  /*irq: A/B           */ ice_irq, ice_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snd_r, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snd_data_w, snd_ctrl_w, 0, 0,
  /*irq: A/B           */ ice_firq, ice_firq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, 0, 0,
  /*irq: A/B           */ ice_irq, ice_irq
}};

static void sirq(int data) {
  pia_set_input_cb1(0, locals.cb10 = data);
}

// handles the 8279 keyboard / display interface chip
static READ_HANDLER(i8279_r) {
  static UINT8 lastData;
  logerror("i8279 r%d (cmd %02x, reg %02x)\n", offset, locals.i8279cmd, locals.i8279reg);
  if (offset) {
    sirq(0);
    return 0xfb & coreGlobals.swMatrix[1];
  }
  if ((locals.i8279cmd & 0xe0) == 0x60)
    lastData = locals.i8279ram[locals.i8279reg]; // read display ram
  else {
    sirq(0);
    lastData = coreGlobals.swMatrix[(locals.i8279cmd & 0x03) + 1]; // read switches
  }
  if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  return lastData;
}
static WRITE_HANDLER(i8279_w) {
  if (offset) { // command
    locals.i8279cmd = data;
    if ((locals.i8279cmd & 0xe0) == 0x40)
      logerror("I8279 read switches: %x\n", data & 0x07);
    else if ((locals.i8279cmd & 0xe0) == 0x80)
      logerror("I8279 write display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x60)
      logerror("I8279 read display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x20)
      logerror("I8279 scan rate: %02x\n", data & 0x1f);
    else if ((locals.i8279cmd & 0xe0) == 0)
      logerror("I8279 set modes: display %x, keyboard %x\n", (data >> 3) & 0x03, data & 0x07);
    else logerror("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = data & 0x0f; // reset data for auto-increment
  } else { // data
    if ((locals.i8279cmd & 0xe0) == 0x80) { // write display ram
      locals.i8279ram[locals.i8279reg] = data;
      coreGlobals.segments[locals.i8279reg].w = data;
    } else logerror("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  }
}

static WRITE_HANDLER(motor_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x1ff00) | (data ^ 0xff);
}

static READ_HANDLER(ay_r) {
#if 0
  printf("r%d ", offset);
#endif
  return 0;
}

static WRITE_HANDLER(ay_w) {
#if 0
  printf("w%d:%02x ", offset, data);
#endif
}

static MEMORY_WRITE_START(ice_writemem)
  { 0x0000, 0x07ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x4000, 0x4007, ay_w },
  { 0x4010, 0x4013, pia_w(0) },
  { 0x4020, 0x4023, pia_w(1) },
  { 0x4040, 0x4043, pia_w(2) }, // unused
  { 0x4080, 0x4081, i8279_w },
  { 0x4100, 0x4100, motor_w },
  { 0xa000, 0xffff, MWA_NOP },
MEMORY_END

static MEMORY_READ_START(ice_readmem)
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x4000, 0x4007, ay_r },
  { 0x4010, 0x4013, pia_r(0) },
  { 0x4020, 0x4023, pia_r(1) },
  { 0x4040, 0x4043, pia_r(2) }, // unused
  { 0x4080, 0x4081, i8279_r },
  { 0xa000, 0xffff, MRA_ROM },
MEMORY_END

static WRITE_HANDLER(ay8910_0_portb_w) {
  coreGlobals.lampMatrix[0] = data;
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x0ffff) | ((data & 0x80) << 9);
}
static READ_HANDLER(ay8910_0_porta_r)	{
  return core_getDip(1);
}
static WRITE_HANDLER(ay8910_1_porta_w) {
  coreGlobals.lampMatrix[1] = data;
  locals.timerSt = data >> 7;
  if (!locals.timerSt) locals.cb10 = 0;
}
static WRITE_HANDLER(ay8910_1_portb_w) {
  coreGlobals.lampMatrix[2] = data;
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x100ff) | (data << 8);
}

struct AY8910interface ice_ay8910Int = {
	2,			/* 2 chips */
	1500000,	/* 1.5 MHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },		/* Volume */
	{ ay8910_0_porta_r, 0 },
	{ 0, 0 },
	{ 0, ay8910_1_porta_w },
	{ ay8910_0_portb_w, ay8910_1_portb_w },
};

static MACHINE_INIT(ICE) {
  int i; for (i=0; i < 3; i++) pia_config(i, PIA_STANDARD_ORDERING, &ice_pia[i]);
  pia_reset();
  pia_set_input_ca1(1, 1);
  AY8910_set_volume(0, 2, 0); // AY chip #0 channel C is used as a 30Hz clock generator, so it needs to be muted!
}
static MACHINE_RESET(ICE) {
  memset(&locals, 0, sizeof(locals));
}
static MACHINE_STOP(ICE) {
  pia_unconfig();
}

static SWITCH_UPDATE(ICE) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x01, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>12,0x03, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT],    0xff, 9);
  }
  cpu_set_nmi_line(0, coreGlobals.swMatrix[0] ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(ice_vblank) {
  core_updateSw(TRUE);
  if (coreGlobals.swMatrix[1] || coreGlobals.swMatrix[2] || coreGlobals.swMatrix[3]) sirq(1);
}
static void ice_30hz(int data) {
  locals.ca11 = !locals.ca11;
  pia_set_input_ca1(1, locals.ca11);
}

MACHINE_DRIVER_START(icecold)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ICE,ICE,ICE)
  MDRV_CPU_ADD_TAG("mcpu", M6809, 6000000/4)
  MDRV_CPU_MEMORY(ice_readmem, ice_writemem)
  MDRV_CPU_VBLANK_INT(ice_vblank, 1)
  MDRV_TIMER_ADD(ice_30hz, 30)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(ICE)
  MDRV_DIPS(16)
  MDRV_SOUND_ADD(AY8910, ice_ay8910Int)
MACHINE_DRIVER_END

#define INITGAME(name, disptype) \
  INPUT_PORTS_START(name) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BIT(     0x0040, "Left Up Limit", KEYCODE_Z) \
    COREPORT_BIT(     0x0080, "Left Joystick Up", KEYCODE_X) \
    COREPORT_BIT(     0x0008, "Left Joystick Down", KEYCODE_C) \
    COREPORT_BIT(     0x0004, "Left Down Limit", KEYCODE_V) \
    COREPORT_BIT(     0x0010, "Right Up Limit", KEYCODE_B) \
    COREPORT_BIT(     0x0020, "Right Joystick Up", KEYCODE_N) \
    COREPORT_BIT(     0x0002, "Right Joystick Down", KEYCODE_M) \
    COREPORT_BIT(     0x0001, "Right Down Limit", KEYCODE_COMMA) \
    COREPORT_BIT(     0x0100, "Test", KEYCODE_0) \
    COREPORT_BITDEF(  0x1000, IPT_COIN1, IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x2000, IPT_START1, IP_KEY_DEFAULT) \
  PORT_START /* 1 */ \
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
  INPUT_PORTS_END \
  static core_tGameData name##GameData = {0,icb_disp,{FLIP_SW(FLIP_L),0,-5}}; \
  static void init_##name(void) { \
    core_gameData = &name##GameData; \
  }

core_tLCDLayout icb_disp[] = {
  {0, 0, 7, 1, CORE_SEG7},
  {3, 0, 4, 3, CORE_SEG7},
  {6, 0, 0, 4, CORE_SEG7}, {0}
};

INITGAME(icecold, icb_disp)
ROM_START(icecold)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("icb23b.bin", 0xe000, 0x2000, CRC(b5b69d0a) SHA1(86f5444700adebb7b2d9da702b6d5425c8d682e3))
    ROM_LOAD("icb24.bin",  0xc000, 0x2000, CRC(2d1e7282) SHA1(6f170e24f71d1504195face5f67176b55c933eef))
ROM_END
CORE_GAMEDEFNV(icecold,"Ice Cold Beer",1983,"Taito",icecold,GAME_NOT_WORKING)

INITGAME(icecoldf, icb_disp)
ROM_START(icecoldf)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("icb23b_f.bin", 0xe000, 0x2000, CRC(6fe73c9d) SHA1(24b60da1fc791844601bd9a7628fde195e9e9644))
    ROM_LOAD("icb24.bin",  0xc000, 0x2000, CRC(2d1e7282) SHA1(6f170e24f71d1504195face5f67176b55c933eef))
ROM_END
CORE_CLONEDEFNV(icecoldf,icecold,"Ice Cold Beer (Free Play)",1983,"Taito",icecold,GAME_NOT_WORKING)

INITGAME(zekepeak, icb_disp)
ROM_START(zekepeak)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("zp23.bin", 0xe000, 0x2000, CRC(ef959586) SHA1(7f8a4787b340bfa34180164806b181b5fb4e5cfa))
    ROM_LOAD("zp24.bin", 0xc000, 0x2000, CRC(ee90c8f5) SHA1(27a513000e90536e485ccdf43786b415b3c95bd7))
ROM_END
CORE_CLONEDEFNV(zekepeak,icecold,"Zeke's Peak",1984,"Taito",icecold,GAME_NOT_WORKING)
