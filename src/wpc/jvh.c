/************************************************************************************************
 Jac Van Ham (trade name: Royal)
 -------------------------------
 Rare Dutch manufacturer.
 The TMS9980 emulation in MAME 0.76 is a bit complicated.
 However, the game works fine, according to all sources. :)

 Hardware:
 ---------
 CPU:   TMS9980A
 IO:    CPU ports
 SOUND:	M6802 CPU, AY8912, 6522 VIA
************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"
#include "cpu/m6800/m6800.h"
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
static WRITE_HANDLER(out7b_w) {
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
  { 0x75, 0x7f, out7b_w },
PORT_END

static PORT_READ_START(readport)
  { 0x01, 0x02, sw1_r },
  { 0x03, 0x05, dip_r },
  { 0x06, 0x07, sw6_r },
PORT_END

static MACHINE_INIT(jvh) {
  memset(&locals, 0, sizeof(locals));
  cpu_set_irq_callback(0, irq_callback);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_SOUND1), NULL, NULL);
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
  UINT8 cmd, via_b;
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

  sndlocals.cmd = data & 0x3f;
  cmd = sndlocals.cmd ^ 0x3f;
  via_set_input_a(0, cmd ? cmd : 0xff); // avoid passing in 0x00 as a command because it stops all sound forever
}

static void jvh_irq(int state) {
  cpu_set_irq_line(sndlocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct via6522_interface jvh_via = {
  /*i: A/B,CA/B1,CA/B2 */ jvh_via_a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ jvh_via_a_w, jvh_via_b_w, 0, 0,
  /*irq                */ jvh_irq
};

static void jvh_init(struct sndbrdData *brdData) {
  int i;
  for (i=0; i < 0x80; i++) memory_region(REGION_CPU2)[i] = 0xff;
  via_reset();
  via_config(0, &jvh_via);
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


// Escape (10/87)

static core_tLCDLayout dispJVH[] = {
  {0, 0, 7,1,CORE_SEG7}, {0, 2, 0,7,CORE_SEG7}, {0,18,15,1,CORE_SEG7}, {0,20, 8,7,CORE_SEG7},
  {3, 0,23,1,CORE_SEG7}, {3, 2,16,7,CORE_SEG7}, {3,18,31,1,CORE_SEG7}, {3,20,24,7,CORE_SEG7},
  {6,12,33,2,CORE_SEG7}, {6,18,35,2,CORE_SEG7}, {0}
};
static core_tGameData jvhGameData = {GEN_ZAC1, dispJVH, {FLIP_SW(FLIP_L), 0, 0, 0, SNDBRD_JVH}};
static void init_jvh(void) {
  core_gameData = &jvhGameData;
}

INPUT_PORTS_START(escape) \
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

ROM_START(escape) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("cpu_ic1.bin", 0x0000, 0x2000, CRC(fadb8f9a) SHA1(b7e7ea8e33847c14a3414f5e367e304f12c0bc00)) \
    ROM_LOAD("cpu_ic7.bin", 0x2000, 0x2000, CRC(2f9402b4) SHA1(3d3bae7e4e5ad40e3c8019d55392defdffd21cc4)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("snd.bin",     0xc000, 0x2000, CRC(2477bbe2) SHA1(f636952822153f43e9d09f8211edde1057249203)) \
    ROM_RELOAD(0xe000, 0x2000) \
ROM_END
#define init_escape init_jvh
CORE_GAMEDEFNV(escape,"Escape",1987,"Jac Van Ham (Royal)",jvh,0)
