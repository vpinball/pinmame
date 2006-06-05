/************************************************************************************************
  Rowamet
  ---------
  by Gerrit Volkenborn

  Main CPU Board:

  CPU: Z80
  Clock: 2,5 Mhz
  Interrupt: 125 Hz
  Sound: Z80
************************************************************************************************/
#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "sim.h"

#define ROWAMET_SOLSMOOTH 4
#define ROWAMET_CPUFREQ 2500000
#define ROWAMET_IRQFREQ 125

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
} locals;

static INTERRUPT_GEN(rowamet_irq)
{
  cpu_set_irq_line(0, 0, PULSE_LINE);
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
	locals.solenoids = coreGlobals.pulsedSolState;
  }
  core_updateSw(1);
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
  return offset ? sw ^ 0xff : sw ^ 0xef;
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(sol_w) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00) | data;
}

static WRITE_HANDLER(sound_w) {
  LOG(("sound_w = %04x\n",data));
}

static WRITE_HANDLER(disp_w) {
  locals.segments[2*offset].w = core_bcd2seg[data & 0x0f];
  locals.segments[2*offset+1].w = core_bcd2seg[data >> 4];
}

static WRITE_HANDLER(peri_w) {
  if (offset <= 0x0d) {
    disp_w(offset, data);
  } else if (offset >= 0x10 && offset <= 0x1c) {
    lamp_w(offset - 0x10, data);
  } else if (offset == 0x80) { // correct this!
    sol_w(0, data);
  } else if (offset >= 0x20 && offset <= 0x31) {
    // ignore - used for switch debuffering (9 columns including col #0)
  } else
    LOG(("peri_w %02x = %02x\n", offset, data));
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x80) peri_w(offset-0x80, data);
}

//Memory Map for Main CPU
static MEMORY_READ_START(cpu_readmem)
  { 0x0000, 0x1fff, MRA_ROM },
  { 0x2800, 0x2808, sw_r },
  { 0x4000, 0x40ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  { 0x4000, 0x40ff, ram_w, &generic_nvram, &generic_nvram_size },
MEMORY_END

MACHINE_DRIVER_START(rowamet)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(rowamet, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", Z80, ROWAMET_CPUFREQ)
  MDRV_CPU_PERIODIC_INT(rowamet_irq, ROWAMET_IRQFREQ)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(rowamet)
  MDRV_NVRAM_HANDLER(generic_0fill)
MACHINE_DRIVER_END

static core_tLCDLayout disp[] = {
  {0, 0, 0, 6,CORE_SEG7},
  {2, 0, 6, 6,CORE_SEG7},
  {4, 0,12, 6,CORE_SEG7},
  {6, 0,18, 6,CORE_SEG7},
  {8, 0,24, 4,CORE_SEG7},
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
ROM_END

CORE_GAMEDEFNV(heavymtl, "Heavy Metal", 198?, "Rowamet", rowamet, GAME_NO_SOUND)
