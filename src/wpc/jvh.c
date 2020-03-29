/************************************************************************************************
 Jac Van Ham (trade name: Royal)
 -------------------------------
 Rare Dutch manufacturer.
 The TMS9980 emulation in MAME 0.76 is a bit complicated.
 However, the games work fine, according to all sources. :)

 Hardware:
 ---------
 CPU:   TMS9980A
 IO:    CPU ports
 SOUND: M6802 CPU, AY8912, 6522 VIA
************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "machine/6522via.h"

static struct {
  core_tSeg segments;
  UINT8 digit, latch, snd;
  int irq, irqenable, irqlevel, zc, zcenable;
  int bcd, col;
} locals;

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));

  core_updateSw(core_getSol(17));
}

static INTERRUPT_GEN(irq) {
  locals.irq = !locals.irq;
  if (locals.irqenable) {
    locals.irqlevel = 1;
    cpu_set_irq_line(0, 0, locals.irq ? ASSERT_LINE : CLEAR_LINE);
  } else
    cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static void zc(int data) {
  locals.zc = !locals.zc;
  if (locals.zcenable) {
    locals.irqlevel = 2;
    cpu_set_irq_line(0, 0, locals.zc ? ASSERT_LINE : CLEAR_LINE);
  } else
    cpu_set_irq_line(0, 0, CLEAR_LINE);
}

static int irq_callback(int irqline) {
  int level = locals.irqlevel ? 2 + locals.irqlevel : 7; // set IC0-2 pins according to level
  locals.irqlevel = 0; // clear level after interrupt
  return level;
}

static WRITE_HANDLER(irq_enable) {
  locals.irqenable = data;
}

static WRITE_HANDLER(zc_enable) {
  locals.zcenable = data;
}

static WRITE_HANDLER(latch_w) {
  locals.latch = locals.digit;
}

static WRITE_HANDLER(col_w) {
  locals.col = (locals.col & ~(1 << offset)) | ((data & 1) << offset);
}

static WRITE_HANDLER(digit_w) {
  locals.digit = (locals.digit & ~(1 << offset)) | ((data & 1) << offset);
}

static WRITE_HANDLER(bcd_w) {
  locals.bcd = (locals.bcd & ~(1 << offset)) | ((data & 1) << offset);
}

static WRITE_HANDLER(panel_w) {
  if (data) locals.segments[offset*8 + core_BitColToNum(~locals.latch & 0xff)].w = core_bcd2seg7[0x0f ^ locals.bcd];
}

static WRITE_HANDLER(snd_w) {
  locals.snd = (locals.snd & ~(1 << offset)) | ((data & 1) << offset);
  logerror("snd:%02x\n", locals.snd);
  if (offset == 5) {
    sndbrd_0_data_w(0, locals.snd);
  }
}

static WRITE_HANDLER(enable_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xfffeffff) | (data << 16);
}

static WRITE_HANDLER(out1a_w) {
  coreGlobals.tmpLampMatrix[0] = (coreGlobals.tmpLampMatrix[0] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out1b_w) {
  coreGlobals.tmpLampMatrix[0] = (coreGlobals.tmpLampMatrix[0] & ~(1 << (offset+3))) | ((data & 1) << (offset+3));
}
static WRITE_HANDLER(out2a_w) {
  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out2b_w) {
  coreGlobals.tmpLampMatrix[2] = (coreGlobals.tmpLampMatrix[2] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out3a_w) {
  coreGlobals.tmpLampMatrix[3] = (coreGlobals.tmpLampMatrix[3] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out5b_w) {
  coreGlobals.tmpLampMatrix[4] = (coreGlobals.tmpLampMatrix[4] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out6a_w) {
  coreGlobals.tmpLampMatrix[5] = (coreGlobals.tmpLampMatrix[5] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out6b_w) {
  coreGlobals.tmpLampMatrix[6] = (coreGlobals.tmpLampMatrix[6] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out7a_w) {
  coreGlobals.tmpLampMatrix[7] = (coreGlobals.tmpLampMatrix[7] & ~(1 << offset)) | ((data & 1) << offset);
}

static WRITE_HANDLER(out0a2_w) {
  coreGlobals.tmpLampMatrix[0] = (coreGlobals.tmpLampMatrix[0] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out0b2_w) {
  coreGlobals.tmpLampMatrix[1] = (coreGlobals.tmpLampMatrix[1] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out1a2_w) {
  coreGlobals.tmpLampMatrix[2] = (coreGlobals.tmpLampMatrix[2] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out1b2_w) {
  coreGlobals.tmpLampMatrix[3] = (coreGlobals.tmpLampMatrix[3] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out2a2_w) {
  coreGlobals.tmpLampMatrix[4] = (coreGlobals.tmpLampMatrix[4] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out2b2_w) {
  coreGlobals.tmpLampMatrix[5] = (coreGlobals.tmpLampMatrix[5] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out3a2_w) {
  coreGlobals.tmpLampMatrix[7] = (coreGlobals.tmpLampMatrix[7] & 0xf7) | ((data & 1) << 3);
}
static WRITE_HANDLER(out4a2_w) {
  coreGlobals.tmpLampMatrix[6] = (coreGlobals.tmpLampMatrix[6] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out4b2_w) {
  coreGlobals.tmpLampMatrix[7] = (coreGlobals.tmpLampMatrix[7] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out4c2_w) {
  coreGlobals.tmpLampMatrix[7] = (coreGlobals.tmpLampMatrix[7] & ~(1 << (offset+4))) | ((data & 1) << (offset+4));
}
static WRITE_HANDLER(out6b2_w) {
  coreGlobals.tmpLampMatrix[8] = (coreGlobals.tmpLampMatrix[8] & ~(1 << offset)) | ((data & 1) << offset);
}
static WRITE_HANDLER(out7a2_w) {
  coreGlobals.tmpLampMatrix[9] = (coreGlobals.tmpLampMatrix[9] & ~(1 << offset)) | ((data & 1) << offset);
}

static WRITE_HANDLER(sol_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & ~(1 << offset)) | ((data & 1) << offset);
}

static READ_HANDLER(sw1_r) {
  return coreGlobals.swMatrix[offset];
}
static READ_HANDLER(sw6_r) {
  return offset ? 0 : coreGlobals.swMatrix[2 + core_BitColToNum(~locals.col & 0x1f)];
}
static READ_HANDLER(dip_r) {
  return core_getDip(offset);
}

static MEMORY_READ_START(readmem)
  { 0x0000, 0x3bff, MRA_ROM },
  { 0x3c00, 0x3fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(writemem)
  { 0x3c00, 0x3fff, MWA_RAM, &generic_nvram, &generic_nvram_size },
MEMORY_END

static PORT_WRITE_START(writeport)
  { 0x10, 0x15, snd_w },
  { 0x16, 0x16, latch_w },
  { 0x17, 0x19, out1a_w },
  { 0x1a, 0x1a, enable_w },
  { 0x1b, 0x1f, out1b_w },
  { 0x20, 0x27, out2a_w },
  { 0x28, 0x2f, out2b_w },
  { 0x30, 0x37, out3a_w },
  { 0x3e, 0x3e, irq_enable },
  { 0x3f, 0x3f, zc_enable },
  { 0x40, 0x47, digit_w },
  { 0x48, 0x4b, bcd_w },
  { 0x4c, 0x50, panel_w },
  { 0x51, 0x55, col_w },
  { 0x58, 0x5f, out5b_w },
  { 0x60, 0x67, out6a_w },
  { 0x68, 0x6f, out6b_w },
  { 0x70, 0x74, out7a_w },
  { 0x75, 0x7f, sol_w },
PORT_END

static PORT_WRITE_START(writeport2)
  { 0x00, 0x07, out0a2_w },
  { 0x08, 0x0f, out0b2_w },
  { 0x10, 0x17, out1a2_w },
  { 0x18, 0x1f, out1b2_w },
  { 0x20, 0x27, out2a2_w },
  { 0x28, 0x2f, out2b2_w },
  { 0x30, 0x30, out3a2_w },
  { 0x31, 0x36, snd_w },
  { 0x37, 0x37, latch_w },
  { 0x3e, 0x3e, irq_enable },
  { 0x3f, 0x3f, zc_enable },
  { 0x40, 0x47, out4a2_w },
  { 0x48, 0x4a, out4b2_w },
  { 0x4b, 0x4b, enable_w },
  { 0x4c, 0x4f, out4c2_w },
  { 0x50, 0x55, col_w },
  { 0x57, 0x5a, bcd_w },
  { 0x5b, 0x5f, panel_w },
  { 0x60, 0x67, digit_w },
  { 0x68, 0x6f, out6b2_w },
  { 0x70, 0x74, out7a2_w },
  { 0x75, 0x7f, sol_w },
PORT_END

static PORT_READ_START(readport)
  { 0x01, 0x02, sw1_r },
  { 0x03, 0x05, dip_r },
  { 0x06, 0x07, sw6_r }, // Escape
  { 0x08, 0x09, sw6_r }, // Movie Masters
PORT_END

static MACHINE_INIT(jvh) {
  memset(&locals, 0, sizeof(locals));
  cpu_set_irq_callback(0, irq_callback);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, NULL, NULL, NULL);
}

static MACHINE_RESET(jvh) {
  memset(&locals, 0, sizeof(locals));
}

static SWITCH_UPDATE(jvh) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],    0x07, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x0f, 1);
  }
}

// sound section

static struct {
  struct sndbrdData brdData;
  UINT8 cmd, via_b, cmd2;
  mame_timer *timer;
  int ignore;
} sndlocals;

static READ_HANDLER(jvh_via_a_r) {
  UINT8 cmd = sndlocals.cmd ^ 0x3f;
  return cmd ? cmd : 0xff; // avoid passing in 0x00 as a command because it stops all sound forever
}
static WRITE_HANDLER(jvh_via_a_w) {
  if (data >> 7)
    AY8910Write(0, (data ^ 0x40) >> 6, sndlocals.via_b);
}
static WRITE_HANDLER(jvh_via_b_w) {
  sndlocals.via_b = data;
}

static WRITE_HANDLER(jvh_data_w) {
  UINT8 cmd;

  if (sndlocals.brdData.subType) { // Formula 1
    sndlocals.cmd = data;
    sndlocals.ignore = 2;
    cpu_set_irq_line(sndlocals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
    cpu_set_irq_line(sndlocals.brdData.cpuNo, M6809_IRQ_LINE, PULSE_LINE);
    return;
  }

  sndlocals.cmd = data & 0x3f;
  cmd = sndlocals.cmd ^ 0x3f;
  via_set_input_a(0, cmd ? cmd : 0xff); // avoid passing in 0x00 as a command because it stops all sound forever
}

static void jvh_irq(int state) {
  cpu_set_irq_line(sndlocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void jvh_firq(int state) {
  cpu_set_irq_line(sndlocals.brdData.cpuNo + 1, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(vol2_w) {
  static UINT8 last;
  if (last != data) {
    mixer_set_volume(4, (data & 0x0f) * 8);
    last = data;
  }
}

static const struct via6522_interface jvh_via[] = {{
  /*i: A/B,CA/B1,CA/B2 */ jvh_via_a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ jvh_via_a_w, jvh_via_b_w, 0, 0,
  /*irq                */ jvh_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ DAC_0_data_w, vol2_w, 0, 0,
  /*irq                */ jvh_firq
}};

static void jvh_init(struct sndbrdData *brdData) {
  via_reset();
  via_config(0, &jvh_via[brdData->subType ? 1 : 0]);
  memset(&sndlocals, 0, sizeof(sndlocals));
  sndlocals.brdData = *brdData;
}

const struct sndbrdIntf jvhIntf = {
  "JVH", jvh_init, NULL, NULL, jvh_data_w, jvh_data_w
};

static struct AY8910interface jvh_ay8912Int  = { 1, 1000000, {25} };

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x008f, via_0_r },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x008f, via_0_w },
MEMORY_END

// driver section

static int jvh_sw2m(int no) {
  if (no < 1) return no + 7;
  return 8 * (1 + no/10) + no%10 - 1;
}
static int jvh_m2sw(int col, int row) {
  if (col < 1) return row-7;
  return (col-1)*10 + row;
}

MACHINE_DRIVER_START(jvh)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(jvh,jvh,NULL)
  MDRV_SCREEN_SIZE(640,400)
  MDRV_VISIBLE_AREA(0, 639, 0, 399)

  MDRV_CPU_ADD_TAG("mcpu", TMS9980, 1000000) // ~8MHz, divided by 8?
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_PORTS(readport, writeport)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_CPU_PERIODIC_INT(irq, 150) // should be clock divided by 1024, but game only works between zc/2 and zc!?
  MDRV_TIMER_ADD(zc, 200) // European zc frequency * 2
  MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_UPDATE(jvh)
  MDRV_SWITCH_CONV(jvh_sw2m, jvh_m2sw)
  MDRV_DIPS(24)
  MDRV_NVRAM_HANDLER(generic_1fill)

  MDRV_CPU_ADD_TAG("scpu", M6802, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(AY8910, jvh_ay8912Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(jvh2)
  MDRV_IMPORT_FROM(jvh)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PORTS(readport, writeport2)
MACHINE_DRIVER_END

static core_tLCDLayout dispJVH0[] = {
  {0, 0, 0,7,CORE_SEG7}, {0,27, 8,7,CORE_SEG7},
  {3, 0,16,7,CORE_SEG7}, {3,27,24,7,CORE_SEG7},
  {3,16,33,2,CORE_SEG7}, {3,21,35,2,CORE_SEG7}, {0}
};

static core_tLCDLayout dispJVH[] = {
  {0, 0, 7,1,CORE_SEG7}, {0, 2, 0,7,CORE_SEG7}, {0,29,15,1,CORE_SEG7}, {0,31, 8,7,CORE_SEG7},
  {3, 0,23,1,CORE_SEG7}, {3, 2,16,7,CORE_SEG7}, {3,29,31,1,CORE_SEG7}, {3,31,24,7,CORE_SEG7},
  {3,18,33,2,CORE_SEG7}, {3,23,35,2,CORE_SEG7}, {0}
};

static core_tLCDLayout dispJVH2[] = {
  {0, 0, 7,1,CORE_SEG7}, {0, 2, 0,7,CORE_SEG7}, {0,29,15,1,CORE_SEG7}, {0,31, 8,7,CORE_SEG7},
  {3, 0,23,1,CORE_SEG7}, {3, 2,16,7,CORE_SEG7}, {3,29,31,1,CORE_SEG7}, {3,31,24,7,CORE_SEG7},
  {3,18,34,2,CORE_SEG7}, {3,23,37,2,CORE_SEG7}, {0}
};

#define INITGAME_JVH(name, lamps, disp, sb) \
static core_tGameData name##GameData = {GEN_ZAC1, disp, {FLIP_SW(FLIP_L), 0, lamps, 0, sb}}; \
static void init_##name(void) { \
  core_gameData = &name##GameData; \
}

INPUT_PORTS_START(jvh) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0001, IPT_COIN1,  IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0002, IPT_COIN2,  IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0004, IPT_COIN3,  KEYCODE_3) \
    COREPORT_BITDEF(0x0200, IPT_START1, IP_KEY_DEFAULT)  \
    COREPORT_BIT   (0x0400,"Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT   (0x0800,"Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT   (0x0100,"Self Test", KEYCODE_7) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x6000, 0x2000, "Balls per game") \
      COREPORT_DIPSET(0x0000, "2" ) \
      COREPORT_DIPSET(0x2000, "3" ) \
      COREPORT_DIPSET(0x4000, "4" ) \
      COREPORT_DIPSET(0x6000, "5" ) \
    COREPORT_DIPNAME( 0x0060, 0x0060, "Credit limit") \
      COREPORT_DIPSET(0x0000, "10" ) \
      COREPORT_DIPSET(0x0020, "15" ) \
      COREPORT_DIPSET(0x0040, "25" ) \
      COREPORT_DIPSET(0x0060, "40" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Enable clear bookkeeping") \
      COREPORT_DIPSET(0x0080, DEF_STR(No) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Yes) ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Background music") \
      COREPORT_DIPSET(0x8000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x001f, 0x0000, "Left Coin Chute (credits/coins)") \
      COREPORT_DIPSET(0x0000, "1/1" ) \
      COREPORT_DIPSET(0x0001, "2/1" ) \
      COREPORT_DIPSET(0x0002, "3/1" ) \
      COREPORT_DIPSET(0x0003, "4/1" ) \
      COREPORT_DIPSET(0x0004, "5/1" ) \
      COREPORT_DIPSET(0x0005, "6/1" ) \
      COREPORT_DIPSET(0x0006, "7/1" ) \
      COREPORT_DIPSET(0x0007, "8/1" ) \
      COREPORT_DIPSET(0x0008, "9/1" ) \
      COREPORT_DIPSET(0x0009, "10/1" ) \
      COREPORT_DIPSET(0x000a, "1/2" ) \
      COREPORT_DIPSET(0x000b, "2/2" ) \
      COREPORT_DIPSET(0x000c, "3/2" ) \
      COREPORT_DIPSET(0x000d, "4/2" ) \
      COREPORT_DIPSET(0x000e, "5/2" ) \
      COREPORT_DIPSET(0x000f, "6/2" ) \
      COREPORT_DIPSET(0x0010, "7/2" ) \
      COREPORT_DIPSET(0x0011, "8/2" ) \
      COREPORT_DIPSET(0x0012, "9/2" ) \
      COREPORT_DIPSET(0x0013, "10/2" ) \
      COREPORT_DIPSET(0x0014, "1/3" ) \
      COREPORT_DIPSET(0x0015, "2/3" ) \
      COREPORT_DIPSET(0x0016, "1/4" ) \
      COREPORT_DIPSET(0x0017, "3/4 (0,0,0,3)" ) \
      COREPORT_DIPSET(0x0018, "1/5" ) \
      COREPORT_DIPSET(0x0019, "3/2 (1,2)" ) \
      COREPORT_DIPSET(0x001a, "3/4 (0,1,1,1)" ) \
      COREPORT_DIPSET(0x001b, "3/4 (0,1,0,2)" ) \
      COREPORT_DIPSET(0x001c, "5/4 (1,1,1,2)" ) \
      COREPORT_DIPSET(0x001d, "7/4 (1,2,1,3)" ) \
      COREPORT_DIPSET(0x001e, "7/4 (1,2,2,2)" ) \
      COREPORT_DIPSET(0x001f, "2/5 (0,0,1,0,1)" ) \
    COREPORT_DIPNAME( 0x1f00, 0x0000, "Center Coin Chute (credits/coins)") \
      COREPORT_DIPSET(0x0000, "1/1" ) \
      COREPORT_DIPSET(0x0100, "2/1" ) \
      COREPORT_DIPSET(0x0200, "3/1" ) \
      COREPORT_DIPSET(0x0300, "4/1" ) \
      COREPORT_DIPSET(0x0400, "5/1" ) \
      COREPORT_DIPSET(0x0500, "6/1" ) \
      COREPORT_DIPSET(0x0600, "7/1" ) \
      COREPORT_DIPSET(0x0700, "8/1" ) \
      COREPORT_DIPSET(0x0800, "9/1" ) \
      COREPORT_DIPSET(0x0900, "10/1" ) \
      COREPORT_DIPSET(0x0a00, "1/2" ) \
      COREPORT_DIPSET(0x0b00, "2/2" ) \
      COREPORT_DIPSET(0x0c00, "3/2" ) \
      COREPORT_DIPSET(0x0d00, "4/2" ) \
      COREPORT_DIPSET(0x0e00, "5/2" ) \
      COREPORT_DIPSET(0x0f00, "6/2" ) \
      COREPORT_DIPSET(0x1000, "7/2" ) \
      COREPORT_DIPSET(0x1100, "8/2" ) \
      COREPORT_DIPSET(0x1200, "9/2" ) \
      COREPORT_DIPSET(0x1300, "10/2" ) \
      COREPORT_DIPSET(0x1400, "1/3" ) \
      COREPORT_DIPSET(0x1500, "2/3" ) \
      COREPORT_DIPSET(0x1600, "1/4" ) \
      COREPORT_DIPSET(0x1700, "3/4 (0,0,0,3)" ) \
      COREPORT_DIPSET(0x1800, "1/5" ) \
      COREPORT_DIPSET(0x1900, "3/2 (1,2)" ) \
      COREPORT_DIPSET(0x1a00, "3/4 (0,1,1,1)" ) \
      COREPORT_DIPSET(0x1b00, "3/4 (0,1,0,2)" ) \
      COREPORT_DIPSET(0x1c00, "5/4 (1,1,1,2)" ) \
      COREPORT_DIPSET(0x1d00, "7/4 (1,2,1,3)" ) \
      COREPORT_DIPSET(0x1e00, "7/4 (1,2,2,2)" ) \
      COREPORT_DIPSET(0x1f00, "2/5 (0,0,1,0,1)" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x001f, 0x0000, "Right Coin Chute (credits/coins)") \
      COREPORT_DIPSET(0x0000, "1/1" ) \
      COREPORT_DIPSET(0x0001, "2/1" ) \
      COREPORT_DIPSET(0x0002, "3/1" ) \
      COREPORT_DIPSET(0x0003, "4/1" ) \
      COREPORT_DIPSET(0x0004, "5/1" ) \
      COREPORT_DIPSET(0x0005, "6/1" ) \
      COREPORT_DIPSET(0x0006, "7/1" ) \
      COREPORT_DIPSET(0x0007, "8/1" ) \
      COREPORT_DIPSET(0x0008, "9/1" ) \
      COREPORT_DIPSET(0x0009, "10/1" ) \
      COREPORT_DIPSET(0x000a, "1/2" ) \
      COREPORT_DIPSET(0x000b, "2/2" ) \
      COREPORT_DIPSET(0x000c, "3/2" ) \
      COREPORT_DIPSET(0x000d, "4/2" ) \
      COREPORT_DIPSET(0x000e, "5/2" ) \
      COREPORT_DIPSET(0x000f, "6/2" ) \
      COREPORT_DIPSET(0x0010, "7/2" ) \
      COREPORT_DIPSET(0x0011, "8/2" ) \
      COREPORT_DIPSET(0x0012, "9/2" ) \
      COREPORT_DIPSET(0x0013, "10/2" ) \
      COREPORT_DIPSET(0x0014, "1/3" ) \
      COREPORT_DIPSET(0x0015, "2/3" ) \
      COREPORT_DIPSET(0x0016, "1/4" ) \
      COREPORT_DIPSET(0x0017, "3/4 (0,0,0,3)" ) \
      COREPORT_DIPSET(0x0018, "1/5" ) \
      COREPORT_DIPSET(0x0019, "3/2 (1,2)" ) \
      COREPORT_DIPSET(0x001a, "3/4 (0,1,1,1)" ) \
      COREPORT_DIPSET(0x001b, "3/4 (0,1,0,2)" ) \
      COREPORT_DIPSET(0x001c, "5/4 (1,1,1,2)" ) \
      COREPORT_DIPSET(0x001d, "7/4 (1,2,1,3)" ) \
      COREPORT_DIPSET(0x001e, "7/4 (1,2,2,2)" ) \
      COREPORT_DIPSET(0x001f, "2/5 (0,0,1,0,1)" ) \
    COREPORT_DIPNAME( 0x0060, 0x0060, "Replays for beating HSTD") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPSET(0x0040, "2" ) \
      COREPORT_DIPSET(0x0060, "3" ) \
    COREPORT_DIPNAME( 0x0080, 0x0080, "Match") \
      COREPORT_DIPSET(0x0000, DEF_STR(No) ) \
      COREPORT_DIPSET(0x0080, DEF_STR(Yes) ) \
INPUT_PORTS_END

INITGAME_JVH(icemania, 0, dispJVH0, SNDBRD_JVH)
ROM_START(icemania) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("1c1_ic1.bin", 0x0000, 0x2000, CRC(eae16d52) SHA1(9151e0ccec938d268a157cea30b4a23b69e194b8)) \
    ROM_LOAD("2b1_ic7.bin", 0x2000, 0x2000, CRC(7b5fc604) SHA1(22c1c1a030877df99c3db50e7dd41dffe6c21dec)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("im_snd.bin",  0xc000, 0x2000, CRC(0d861f65) SHA1(6c17d1f674c95a4c877b42100ab6e0ae8213264b)) \
    ROM_RELOAD(0xe000, 0x2000) \
ROM_END
#define input_ports_icemania input_ports_jvh
CORE_GAMEDEFNV(icemania,"Ice Mania",1986,"Jac Van Ham (Royal)",jvh,0)

INITGAME_JVH(escape, 0, dispJVH, SNDBRD_JVH)
ROM_START(escape) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("cpu_ic1.bin", 0x0000, 0x2000, CRC(fadb8f9a) SHA1(b7e7ea8e33847c14a3414f5e367e304f12c0bc00)) \
    ROM_LOAD("cpu_ic7.bin", 0x2000, 0x2000, CRC(2f9402b4) SHA1(3d3bae7e4e5ad40e3c8019d55392defdffd21cc4)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("snd.bin",     0xc000, 0x2000, CRC(2477bbe2) SHA1(f636952822153f43e9d09f8211edde1057249203)) \
    ROM_RELOAD(0xe000, 0x2000) \
ROM_END
#define input_ports_escape input_ports_jvh
CORE_GAMEDEFNV(escape,"Escape",1987,"Jac Van Ham (Royal)",jvh,0)

INITGAME_JVH(movmastr, 2, dispJVH2, SNDBRD_JVH)
ROM_START(movmastr) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("mm_ic1.764", 0x0000, 0x2000, CRC(fb59920d) SHA1(05536c4c036a8d73516766e14f4449665b2ec180)) \
    ROM_LOAD("mm_ic7.764", 0x2000, 0x2000, CRC(9b47af41) SHA1(ae795c22aa437d6c71312d93de8a87f43ee500fb)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("snd.bin",    0xc000, 0x2000, NO_DUMP) \
    ROM_RELOAD(0xe000, 0x2000) \
ROM_END
#define input_ports_movmastr input_ports_jvh
CORE_GAMEDEFNV(movmastr,"Movie Masters",19??,"Jac Van Ham (Royal)",jvh2,0)


/*
 Formula 1 uses different hardware:
 ---------
 CPU:   TMS9995
 IO:    CPU ports, only display seems to use DMA
 SOUND: 2x M6809 CPU, AY2203 including its FM channel, AY3812, DAC for percussion samples
*/
static MACHINE_INIT(jvh3) {
  memset(&locals, 0, sizeof(locals));
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, NULL, NULL, NULL);
}

static MACHINE_RESET(jvh3) {
  sndlocals.cmd = 0xff;
  sndlocals.ignore = 2;
}

static INTERRUPT_GEN(vblank3) {
  static int dispCnt;
  static int killFastSols;
  UINT16 segs;
  int i;

  if (killFastSols && !(--killFastSols)) {
    coreGlobals.solenoids &= 0xfffffff8;
  }
  if (!killFastSols && coreGlobals.solenoids & 0x7) {
    killFastSols = 3;
  }

  core_updateSw(core_getSol(4));

  for (i = 0; i < 40; i++) {
    segs = 0xf8a0 + 4 * (i / 8) + (dispCnt < 8 ? 0 : 2);
    segs = (memory_region(REGION_CPU1)[segs] << 8) | memory_region(REGION_CPU1)[segs + 1];
    segs = core_ascii2seg16[0x7f & memory_region(REGION_CPU1)[segs + i % 8]];
    coreGlobals.segments[i].w = i < 16 ? segs : (segs & 0x7f) | ((segs & 0x200) >> 8) | ((segs & 0x2000) >> 11);
  }
  dispCnt = (dispCnt + 1) % 16;
}

static INTERRUPT_GEN(irq3) {
  cpu_set_irq_line(0, 1, PULSE_LINE);
}

static SWITCH_UPDATE(jvh3) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],    0x3f, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT],    0x80, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x80, 2);
    cpu_set_irq_line(0, 0, coreGlobals.swMatrix[0] & 0x20 ? ASSERT_LINE : CLEAR_LINE);
  }
}

static MEMORY_WRITE_START(writemem3)
  { 0xf800, 0xffff, MWA_RAM, &generic_nvram, &generic_nvram_size },
MEMORY_END

static MEMORY_READ_START(readmem3)
  { 0x0000, 0x7fff, MRA_ROM },
  { 0xf800, 0xffff, MRA_RAM },
MEMORY_END

static WRITE_HANDLER(port_w) {
  static UINT8 latch;
  if (offset <= 0x07)
    latch = (latch & ~(1 << offset)) | (data << offset);
  else if (offset == 0x08 || offset == 0x09)
    logerror("Port %02x: %x latch %02x\n", offset, data, latch);
  else if (offset >= 0x11 && offset <= 0x13) // very short pulses, maybe flashers?
    coreGlobals.solenoids |= data << (offset - 0x11);
  else if (offset >= 0x20 && offset <= 0x5f)
    coreGlobals.lampMatrix[(offset - 0x20) / 8] = (coreGlobals.lampMatrix[(offset - 0x20) / 8] & ~(1 << (offset % 8))) | (data << (offset % 8));
  else if (offset >= 0x63 && offset <= 0x7f)
    coreGlobals.solenoids = (coreGlobals.solenoids & ~(1 << (offset - 0x60))) | (data << (offset - 0x60));
  else if (offset >= 0x80 && offset <= 0x82)
    coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & ~(1 << (offset - 0x80))) | (data << (offset - 0x80));
  else if (offset == 0x85) {
    if (data) sndbrd_0_data_w(0, locals.snd);
  } else if (offset >= 0x88 && offset <= 0x8f)
    locals.snd = (locals.snd & ~(1 << (offset - 0x88))) | (data << (offset - 0x88));
  else if (offset >= 0x90 && offset <= 0x96)
    locals.col = (locals.col & ~(1 << (offset - 0x90))) | (data << (offset - 0x90));
  else
    logerror("Port %02x: %x\n", offset, data);
}

static READ_HANDLER(port_r) {
  switch (offset) {
    case 4: return coreGlobals.swMatrix[0];
    case 5: return coreGlobals.swMatrix[1 + core_BitColToNum(~locals.col & 0x7f)];
    case 6: return 0; // no idea about this one, maybe reserved for future use / more switches?
    default: return core_getDip(offset);
  }
}

static PORT_WRITE_START(writeport3)
  { 0x00, 0xff, port_w },
PORT_END

static PORT_READ_START(readport3)
  { 0x00, 0x06, port_r },
PORT_END

// sound section

static READ_HANDLER(sndcmd_r) {
  return sndlocals.cmd;
}

static READ_HANDLER(ready_r) {
  return YM2203_status_port_0_r(0) | YM3812_status_port_0_r(0);
}

static MEMORY_WRITE_START(snd_writemem3)
  { 0x2000, 0x2000, YM2203_control_port_0_w },
  { 0x2001, 0x2001, YM2203_write_port_0_w },
  { 0x4000, 0x4000, YM3812_control_port_0_w },
  { 0x4001, 0x4001, YM3812_write_port_0_w },
  { 0x6000, 0x63ff, MWA_RAM },
  { 0x8000, 0xffff, MWA_NOP },
MEMORY_END

static MEMORY_READ_START(snd_readmem3)
  { 0x0000, 0x0000, sndcmd_r },
  { 0x2000, 0x2000, YM2203_status_port_0_r },
  { 0x4000, 0x4000, YM3812_status_port_0_r },
  { 0x6000, 0x63ff, MRA_RAM },
  { 0xc000, 0xc000, ready_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

// keep code for nostalgic / documentary reasons: volume is determined by a 16-bit value composed by VIA registers
/*
static WRITE_HANDLER(vol_w) {
  static UINT16 vol;
  if (offset) vol = (vol & 0x00ff) | (data << 8); else vol = (vol & 0xff00) | data;
  mixer_set_volume(4, vol * 2 / 9);
}
*/
static READ_HANDLER(sndcmd2_r) {
  UINT8 cmd = sndlocals.cmd2;
  cpu_set_irq_line(sndlocals.brdData.cpuNo + 1, M6809_IRQ_LINE, CLEAR_LINE);
  sndlocals.cmd2 = 0xff;
  return cmd;
}

static MEMORY_WRITE_START(snd_writemem4)
  { 0x0000, 0x000f, via_0_w },
/*
  { 0x0004, 0x0005, vol_w },
  { 0x000f, 0x000f, DAC_0_data_w },
*/
  { 0x0000, 0x00ff, MWA_RAM },
  { 0x2000, 0xffff, MWA_NOP },
MEMORY_END

static MEMORY_READ_START(snd_readmem4)
  { 0x0000, 0x000f, via_0_r },
  { 0x0010, 0x0010, sndcmd2_r },
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x2000, 0xffff, MRA_ROM },
MEMORY_END

static WRITE_HANDLER(ym2203_a_w) {
  sndlocals.cmd2 = data;
  cpu_set_irq_line(sndlocals.brdData.cpuNo + 1, M6809_IRQ_LINE, ASSERT_LINE);
}

static void snd_stop(int param) {
  discrete_sound_w(2, 0);
}

static WRITE_HANDLER(ym2203_b_w) {
  static int ignore = 1;
  static UINT8 old;
  if (old != data) {
    if (!ignore && !sndlocals.ignore) {
      discrete_sound_w(1, data);
      discrete_sound_w(2, 1);
      if (!sndlocals.timer) sndlocals.timer = timer_alloc(snd_stop);
      timer_reset(sndlocals.timer, 0.05);
    } else {
      if (ignore) ignore--;
      else if (sndlocals.ignore) sndlocals.ignore--;
    }
  }
  old = data;
}

static void ym2203_irq(int state) {
  cpu_set_irq_line(sndlocals.brdData.cpuNo, M6809_FIRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface ym2203_interface = {
  1, 2000000, { YM2203_VOL(30,30) },
  { NULL },	{ NULL },
  { ym2203_a_w }, { ym2203_b_w },
  { ym2203_irq }
};

static struct YM3812interface ym3812_interface = { 1, 1000000, { 60 }};

static struct DACinterface dac_interface = { 1, { 30 }};

static DISCRETE_SOUND_START(discInt)
	DISCRETE_INPUT(NODE_01,1,0x0003,0) // tone
	DISCRETE_INPUT(NODE_02,2,0x0003,0) // enable
	DISCRETE_MULTADD(NODE_03,1,NODE_01,1,200)
	DISCRETE_TRIANGLEWAVE(NODE_04,NODE_02,NODE_03,20000,10000,0)
	DISCRETE_GAIN(NODE_05,NODE_04,1)
	DISCRETE_OUTPUT(NODE_05,20)
DISCRETE_SOUND_END

// driver section

MACHINE_DRIVER_START(jvh3)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(jvh3,jvh3,NULL)

  MDRV_CPU_ADD_TAG("mcpu", TMS9995, 12000000)
  MDRV_CPU_MEMORY(readmem3, writemem3)
  MDRV_CPU_PORTS(readport3, writeport3)
  MDRV_CPU_VBLANK_INT(vblank3, 1)
  MDRV_CPU_PERIODIC_INT(irq3, 100)
  MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_UPDATE(jvh3)
  MDRV_DIPS(32)
  MDRV_NVRAM_HANDLER(generic_1fill)

  MDRV_CPU_ADD_TAG("scpu", M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem3, snd_writemem3)
  MDRV_SOUND_ADD(YM2203, ym2203_interface)
  MDRV_SOUND_ADD(YM3812, ym3812_interface)
  MDRV_SOUND_ADD(DISCRETE, discInt)

  MDRV_CPU_ADD_TAG("scpu2", M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem4, snd_writemem4)
  MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END

INPUT_PORTS_START(formula1) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0004, IPT_COIN1,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0002, IPT_COIN2,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0001, IPT_COIN3,   KEYCODE_3) \
    COREPORT_BITDEF(0x0010, IPT_START1,  IP_KEY_DEFAULT)  \
    COREPORT_BIT   (0x0020, "Reset",     KEYCODE_0) \
    COREPORT_BIT   (0x0008, "Self Test", KEYCODE_7) \
    COREPORT_BIT   (0x0080, "LDIR",      KEYCODE_8) \
    COREPORT_BIT   (0x8000, "RDIR",      KEYCODE_9) \
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
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

static core_tLCDLayout dispJVH3[] = {
  {0, 0, 0,8,CORE_SEG16}, {0,18, 8,8,CORE_SEG16},
  {3, 0,16,8,CORE_SEG7},  {3,18,24,8,CORE_SEG7},
#ifdef MAME_DEBUG
  {6, 0,32,8,CORE_SEG7},
#endif
  {0}
};

INITGAME_JVH(formula1, 0, dispJVH3, SNDBRD_JVH2)
ROM_START(formula1)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("f1_ic4.bin", 0x0000, 0x4000, CRC(6ca345da) SHA1(5532c5786d304744a69c7e892924edd03abe8209))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("f1_snd.bin", 0x8000, 0x8000, CRC(00562594) SHA1(3325ad4c0ec04457f4d5b78b9ac48218d6c7aef3))
  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("f1_samples.bin", 0x0000, 0x10000, CRC(ecb7ff04) SHA1(1c11c8ce62ec2f0a4f7dae3b1661baddb04ad55a))
ROM_END
CORE_GAMEDEFNV(formula1,"Formula 1",1988,"Jac Van Ham (Royal)",jvh3,0)
