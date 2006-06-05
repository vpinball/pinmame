/************************************************************************************************
  Rowamet
  ---------
  by Gerrit Volkenborn

  Main CPU board:
  ---------------
  CPU: Z80
  Clock: 2 Mhz

  Sound board:
  ------------
  CPU: Z80
  Clock: 2 Mhz
  DAC
************************************************************************************************/
#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "sim.h"

#define ROWAMET_SOLSMOOTH 4
#define ROWAMET_CPUFREQ 2000000

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT8 sndCmd;
} locals;

static INTERRUPT_GEN(rowamet_irq) {
  static int irq = 0;
  irq++;
  if (irq == 5) {
    cpu_set_irq_line(0, 0, ASSERT_LINE);
    cpu_set_irq_line(0, 0, CLEAR_LINE);
    irq = 0;
  }
}

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));

  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % ROWAMET_SOLSMOOTH) == 0) {
    locals.solenoids = 0;
  }
  core_updateSw(core_getSol(1));
}

static SWITCH_UPDATE(rowamet) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[CORE_COREINPORT] & 0xff);		//Column 0 Switches
  }
}

static MACHINE_INIT(rowamet) {
  memset(&locals, 0, sizeof(locals));
}

static READ_HANDLER(sw_r) {
  UINT8 sw = coreGlobals.swMatrix[offset];
  return sw ^ 0xff;
}

static WRITE_HANDLER(lamp_w) {
  int oldData = coreGlobals.tmpLampMatrix[offset / 2];
  coreGlobals.tmpLampMatrix[offset / 2] = offset % 2 ? (oldData & 0x0f) | ((data & 0x0f) << 4) : (oldData & 0xf0) | (data & 0x0f);
}

static WRITE_HANDLER(sol_w) {
  UINT16 d = data;
  locals.solenoids |= d << offset;
}

static WRITE_HANDLER(disp_w) {
  locals.segments[2*offset+1].w = core_bcd2seg[data & 0x0f];
  locals.segments[2*offset].w = core_bcd2seg[data >> 4];
}

static WRITE_HANDLER(peri_w) {
  if (offset <= 0x0f) {
    disp_w(0x0f - offset, data);
  } else {
    lamp_w(offset - 0x10, data);
    if (offset == 0x12) {
      if (((locals.sndCmd & 0xf0) | (data >> 4)) != locals.sndCmd) {
        locals.sndCmd = (locals.sndCmd & 0xf0) | (data >> 4);
        cpu_set_nmi_line(1, PULSE_LINE);
      }
    }
    else if (offset == 0x13) locals.sndCmd = (locals.sndCmd & 0x0f) | (data & 0xf0);
    else sol_w(offset - 0x10, data & 0x80 ? 1 : 0);
    LOG(("peri_w %02x = %x\n", offset, data >> 4));
  }
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x80 && offset < 0xa0) peri_w(offset-0x80, data);
}

//Memory map for main CPU
static MEMORY_READ_START(cpu_readmem)
  { 0x0000, 0x1fff, MRA_ROM },
  { 0x2800, 0x2808, sw_r },
  { 0x4000, 0x40ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  { 0x4000, 0x40ff, ram_w, &generic_nvram, &generic_nvram_size },
MEMORY_END

//Memory map for sound CPU
static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x0fff, MRA_ROM },
  { 0x1000, 0x17ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x1000, 0x17ff, MWA_RAM },
MEMORY_END

static READ_HANDLER(snd_data_r) {
  return locals.sndCmd;
}

//Port map for sound CPU
static PORT_READ_START(snd_readport)
  { 0x00, 0x00, snd_data_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { 0x01, 0x01, DAC_0_data_w },
PORT_END

static struct DACinterface rowamet_dac_intf = { 1, { 25 }};

MACHINE_DRIVER_START(rowamet)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(rowamet, NULL, NULL)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_CPU_ADD_TAG("mcpu", Z80, ROWAMET_CPUFREQ)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_CPU_PERIODIC_INT(rowamet_irq, 1)
  MDRV_SWITCH_UPDATE(rowamet)
  MDRV_NVRAM_HANDLER(generic_0fill)

  MDRV_CPU_ADD_TAG("scpu", Z80, 1500000)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_SOUND_ADD(DAC, rowamet_dac_intf)
MACHINE_DRIVER_END

static core_tLCDLayout disp[] = {
  {0, 2, 5, 1,CORE_SEG7},
  {3, 0,26, 6,CORE_SEG7},
  {5, 0,20, 6,CORE_SEG7},
  {7, 0,14, 6,CORE_SEG7},
  {9, 0, 8, 6,CORE_SEG7},
  {12,2, 7, 1,CORE_SEG7},
  {0}
};
static core_tGameData heavymtlGameData = {GEN_ZAC2, disp};
static void init_heavymtl(void) {
  core_gameData = &heavymtlGameData;
}

INPUT_PORTS_START(heavymtl) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0001, "Key 1",     KEYCODE_1) \
    COREPORT_BIT(     0x0002, "Key 2",     KEYCODE_2) \
    COREPORT_BIT(     0x0004, "Key 3",     KEYCODE_3) \
    COREPORT_BIT(     0x0008, "Key 4",     KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Key 5",     KEYCODE_5) \
    COREPORT_BIT(     0x0020, "Key 6",     KEYCODE_6) \
    COREPORT_BIT(     0x0040, "Key 7",     KEYCODE_7) \
    COREPORT_BIT(     0x0080, "Key 8",     KEYCODE_8) \
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
      COREPORT_DIPSET(0x0080, "1" )
INPUT_PORTS_END

ROM_START(heavymtl) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("hvymtl_c.bin", 0x0000, 0x1000, CRC(9dacb8c8) SHA1(2eaeeb84ff466adcd05830d6a68815a054adb27d)) \
    ROM_LOAD("hvymtl_b.bin", 0x1000, 0x1000, CRC(7aa7228d) SHA1(261ecad7348c64938c8ddf2ebbb8e8307c3a52b9)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("hvymtl_s.bin", 0x0000, 0x1000, CRC(b8406c85) SHA1(a18fbc2f088be89ded3ed15a149fb7414b859ce1)) \
ROM_END

CORE_GAMEDEFNV(heavymtl, "Heavy Metal", 198?, "Rowamet", rowamet, 0)
