#include "driver.h"
#include "core.h"
#include "ltd.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  int vblankCount;
  int diagnosticLed;
  UINT8 db;
  UINT32 solenoids;
  core_tSeg segments;
  UINT8 pseg1, pseg2;
  mame_timer *irqtimer;
  int irqfreq;
} locals;

#define LTD_CPUFREQ	3579545/4
#define LTD_IRQFREQ 500

#ifdef MAME_DEBUG
static void adjust_timer(int offset) {
  static char s[4];
  locals.irqfreq += offset;
  if (locals.irqfreq < 1) locals.irqfreq = 1;
  sprintf(s, "%4d", locals.irqfreq);
  core_textOut(s, 4, 25, 5, 5);
  timer_adjust(locals.irqtimer, 1.0/(double)locals.irqfreq, 0, 1.0/(double)locals.irqfreq);
}
#endif /* MAME_DEBUG */

static void timer_callback(int n) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(LTD_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % LTD_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((locals.vblankCount % LTD_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
  }
  /*-- display --*/
  if ((locals.vblankCount % LTD_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(LTD) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[LTD_COMINPORT] & 1;
    cpu_set_nmi_line(LTD_CPU, coreGlobals.swMatrix[0] & 1);
  }
#ifdef MAME_DEBUG
  if      (keyboard_pressed_memory_repeat(KEYCODE_Z, 2))
    adjust_timer(-10);
  else if (keyboard_pressed_memory_repeat(KEYCODE_X, 2))
    adjust_timer(-1);
  else if (keyboard_pressed_memory_repeat(KEYCODE_C, 2))
    adjust_timer(1);
  else if (keyboard_pressed_memory_repeat(KEYCODE_V, 2))
    adjust_timer(10);
#endif /* MAME_DEBUG */
}

/* Activate periphal write:
   0-6 : INT1-INT7 - Lamps & flipper enable
   7   : INT8      - Solenoids
   8   : INT9      - Display data
   9   : INT10     - Display strobe & sound
   15  : CLR       - resets all output
 */
static WRITE_HANDLER(peri_w) {
  switch (offset) {
    case 0: case 2: case 4:
      locals.segments[4 - offset].w = core_bcd2seg[data >> 4];
      locals.segments[5 - offset].w = core_bcd2seg[data & 0x0f];
      break;
    case 1: case 3: case 5:
      locals.segments[11 - offset].w = core_bcd2seg[data >> 4];
      locals.segments[12 - offset].w = core_bcd2seg[data & 0x0f];
      break;
    case 8:
      locals.segments[12].w = core_bcd2seg[data >> 4];
      locals.segments[13].w = core_bcd2seg[data & 0x0f];
      break;
    case 9:
      locals.segments[14].w = core_bcd2seg[data >> 4];
      locals.segments[15].w = core_bcd2seg[data & 0x0f];
      break;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
      coreGlobals.tmpLampMatrix[offset - 0x10] = data;
      break;
    case 0x16:
      locals.solenoids = (locals.solenoids &= 0x00ff) | (data << 8);
      break;
    case 0x17:
      locals.solenoids = (locals.solenoids &= 0xff00) | data;
      break;
  }
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x60 && offset < 0x78) peri_w(offset-0x60, data);
}

static READ_HANDLER(ram_r) {
  static UINT8 sw;
  if (offset > 0x4f && offset < 0x60) { // switches
    sw = coreGlobals.swMatrix[1 + offset % 8];
  } else
    sw = 0;
  return generic_nvram[offset] ^ sw;
}

/*-----------------------------------------
/  Memory map for CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem)
  {0x0000,0x01ff, ram_r},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem)
  {0x0000,0x01ff, ram_w, &generic_nvram, &generic_nvram_size},
  {0xf000,0xffff, MWA_NOP},
MEMORY_END

static MACHINE_INIT(LTD) {
  memset(&locals, 0, sizeof locals);
  locals.irqtimer = timer_alloc(timer_callback);
  locals.irqfreq = LTD_IRQFREQ;
  timer_adjust(locals.irqtimer, 1.0/(double)locals.irqfreq, 0, 1.0/(double)locals.irqfreq);
}

MACHINE_DRIVER_START(LTD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6803, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem, LTD_writemem)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(LTD)
MACHINE_DRIVER_END
