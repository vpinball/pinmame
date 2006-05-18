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
    locals.solenoids &= 0x10000;
  }
  /*-- display --*/
  if ((locals.vblankCount % LTD_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(17));
}

static SWITCH_UPDATE(LTD) {
  if (inports) {
    CORE_SETKEYSW(inports[LTD_COMINPORT],    0x01, 0);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x01, 1);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x20, 4);
  }
  cpu_set_nmi_line(LTD_CPU, coreGlobals.swMatrix[0] & 1);
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
  if (offset < 10) {
    locals.segments[18 - 2*offset].w = core_bcd2seg[data >> 4];
    locals.segments[19 - 2*offset].w = core_bcd2seg[data & 0x0f];
  } else if (offset >= 0x10 && offset < 0x16) {
    coreGlobals.tmpLampMatrix[offset - 0x10] = data;
    // map flippers enable to sol 17
    if (offset == 0x10) {
      locals.solenoids = (locals.solenoids & 0x0ffff) | ((coreGlobals.tmpLampMatrix[0] & 0x40) ? 0 : 0x10000);
      locals.diagnosticLed = coreGlobals.tmpLampMatrix[0] >> 7;
    }
  } else if (offset == 0x16) {
    locals.solenoids = (locals.solenoids & 0x100ff) | (data << 8);
  } else if (offset == 0x17) {
    locals.solenoids = (locals.solenoids & 0x1ff00) | data;
  }
}

static WRITE_HANDLER(auxlamp1_w) {
  coreGlobals.tmpLampMatrix[7] = data;
}

static WRITE_HANDLER(auxlamp2_w) {
  coreGlobals.tmpLampMatrix[8] = data;
}

static WRITE_HANDLER(auxlamp3_w) {
  coreGlobals.tmpLampMatrix[9] = data;
}

static WRITE_HANDLER(auxlamp4_w) {
  coreGlobals.tmpLampMatrix[10] = data;
}

static WRITE_HANDLER(auxlamp5_w) {
  coreGlobals.tmpLampMatrix[11] = data;
}

static WRITE_HANDLER(auxlamp6_w) {
  coreGlobals.tmpLampMatrix[12] = data;
}

static WRITE_HANDLER(auxlamp7_w) {
  coreGlobals.tmpLampMatrix[13] = data;
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x60 && offset < 0x78) peri_w(offset-0x60, data);
}

static READ_HANDLER(ram_r) {
  return generic_nvram[offset];
}

static READ_HANDLER(sw_r) {
  return ~(coreGlobals.swMatrix[offset + 1] & 0x3f);
}

static MACHINE_INIT(LTD) {
  memset(&locals, 0, sizeof locals);
  locals.irqtimer = timer_alloc(timer_callback);
  locals.irqfreq = 200;
  timer_adjust(locals.irqtimer, 1.0/(double)locals.irqfreq, 0, 1.0/(double)locals.irqfreq);
}

/*-----------------------------------------
/  Memory map for system III CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem)
  {0x0000,0x007f, ram_r},
  {0x0080,0x00ff, sw_r},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem)
  {0x0000,0x007f, ram_w, &generic_nvram, &generic_nvram_size},
  {0x0800,0x0800, auxlamp1_w},
  {0x0c00,0x0c00, auxlamp2_w},
  {0x1800,0x1800, auxlamp3_w},
  {0x1c00,0x1c00, auxlamp4_w},
  {0x2800,0x2800, auxlamp5_w},
  {0x2c00,0x2c00, auxlamp6_w},
  {0xb000,0xb000, auxlamp7_w},
  {0xf000,0xffff, MWA_NOP},
MEMORY_END

MACHINE_DRIVER_START(LTD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6802, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem, LTD_writemem)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_UPDATE(LTD)
MACHINE_DRIVER_END

static WRITE_HANDLER(peri4_w) {
  if (offset == 4) {
    locals.solenoids = (locals.solenoids & 0x1ff00) | data;
  } else if (offset > 4 && offset < 0x17) {
    locals.segments[offset-5].w = data;
  } else if (offset == 0x17) {
    locals.segments[18].w = core_bcd2seg[data >> 4];
    locals.segments[19].w = core_bcd2seg[data & 0x0f];
  } else if (offset == 0x18) {
    locals.segments[20].w = core_bcd2seg[data >> 4];
    locals.segments[21].w = core_bcd2seg[data & 0x0f];
  } else if (offset == 0x19) {
    locals.segments[22].w = core_bcd2seg[data >> 4];
    locals.segments[23].w = core_bcd2seg[data & 0x0f];
  } else if (offset >= 0x20 && offset < 0x2c) {
    coreGlobals.tmpLampMatrix[offset - 0x20] = data;
    // map flippers enable to sol 17
    if (offset == 0x10) {
      locals.solenoids = (locals.solenoids & 0x0ffff) | ((coreGlobals.tmpLampMatrix[0] & 0x40) ? 0 : 0x10000);
      locals.diagnosticLed = coreGlobals.tmpLampMatrix[0] >> 7;
    }
  }
}

static READ_HANDLER(sw4_r) {
  UINT8 sw = coreGlobals.swMatrix[offset + 1];
  if (offset == 1) sw ^= 0x01;
  return sw;
}

static WRITE_HANDLER(ram4_w) {
  generic_nvram[offset] = data;
  if (offset >= 0xd0 && offset < 0xfc) peri4_w(offset-0xd0, data);
}

static MACHINE_INIT(LTD4) {
  memset(&locals, 0, sizeof locals);
  locals.irqtimer = timer_alloc(timer_callback);
  locals.irqfreq = 100;
  timer_adjust(locals.irqtimer, 1.0/(double)locals.irqfreq, 0, 1.0/(double)locals.irqfreq);
}

/*-----------------------------------------
/  Memory map for system 4 CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem2)
  {0x0000,0x01f7, ram_r},
  {0x01f8,0x01ff, sw4_r},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem2)
  {0x0000,0x01ff, ram4_w, &generic_nvram, &generic_nvram_size},
  {0x0800,0x0800, auxlamp6_w},
  {0x0c00,0x0c00, auxlamp7_w},
  {0x1400,0x1400, auxlamp6_w},
  {0x1800,0x1800, auxlamp7_w},
  {0x1c00,0x1c00, auxlamp6_w},
  {0x2800,0x2800, auxlamp7_w},
MEMORY_END

MACHINE_DRIVER_START(LTD4)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6803, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem2, LTD_writemem2)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(LTD4,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_UPDATE(LTD)
MACHINE_DRIVER_END
