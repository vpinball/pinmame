#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "taito.h"

#define TAITO_VBLANKFREQ    60 /* VBLANK frequency */
#define TAITO_IRQFREQ        1 /* IRQ (via PIA) frequency*/

#define TAITO_SOLSMOOTH      2 /* Smooth the Solenoids over this numer of VBLANKS */
#define TAITO_LAMPSMOOTH     2 /* Smooth the lamps over this number of VBLANKS */
#define TAITO_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS */

static struct {
  int bcd[7];
  int lampadr1, lampadr2;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
} locals;

static NVRAM_HANDLER(taito);

static void taito_dispStrobe(int mask) {
  int ii,jj;
  int digit = 0;
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
    }
}

static void taito_lampStrobe(int board, int lampadr) {
}

static INTERRUPT_GEN(taito_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  /*
  if ((locals.vblankCount % TAITO_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  */

  /*-- solenoids --*/
  if ((locals.vblankCount % TAITO_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
//	  for (i=0; i<14; i++) {
//		  b = *(memory_region(TAITO_MEMREG_CPU)+0x4040 + i);
//		  ((int *) coreGlobals.segments)[i*2] = core_bcd2seg[(int) (b&0x0f)];
//		  ((int *) coreGlobals.segments)[i*2+1] = core_bcd2seg[(b & 0xf0)>>4];
//	  }

    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(taito) {
  if (inports) {
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xEF) |
							  (((inports[TAITO_COMINPORT]>>5)^0x10)&0x10);
  }

  // Diagnostic buttons on CPU board 
  // cpu_set_nmi_line(0, core_getSw(TAITO_SWCPUDIAG) ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(taito_irq) {
	// logerror("irq:\n");
	cpu_set_irq_line(TAITO_CPU, 0, ASSERT_LINE);
}

static int irq_callback(int irqline)
{
  cpu_set_irq_line(TAITO_CPU, 0, CLEAR_LINE);
  return 0xff;
}

static MACHINE_INIT(taito) {
  memset(&locals, 0, sizeof(locals));
  cpu_set_irq_callback(TAITO_CPU, irq_callback);

  locals.vblankCount = 1;
}

static WRITE_HANDLER(lamps) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static sw0 = 0x10;

static READ_HANDLER(switches_r) {
	if ( offset == 6 )
		return 0xff;
	else
	    return coreGlobals.swMatrix[offset+1];
}

static WRITE_HANDLER(switches_w) {
	logerror("switch write: %i %i\n", offset, data);
}

static WRITE_HANDLER(cmd_w) {
	logerror("command write: %i %i\n", offset, data);
	switch ( offset ) {
	case 0:
		((int*) coreGlobals.segments)[38] = core_bcd2seg[(data>>4)&0x0f];
		((int*) coreGlobals.segments)[39] = core_bcd2seg[data&0x0f];
		break;

	case 1:
		((int*) coreGlobals.segments)[46] = core_bcd2seg[(data>>4)&0x0f];
		((int*) coreGlobals.segments)[47] = core_bcd2seg[data&0x0f];
		break;

	}
}

static MEMORY_READ_START(taito_readmem)
  { 0x0000, 0x1fff, MRA_ROM },
  { 0x3000, 0x3eff, MRA_ROM },
  { 0x2800, 0x2808, switches_r },
  { 0x4000, 0x40ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(taito_writemem)
  { 0x0000, 0x1fff, MWA_ROM },
  { 0x3000, 0x3eff, MWA_ROM },
  { 0x3f00, 0x3fff, cmd_w },
  { 0x4000, 0x40ff, MWA_RAM },
MEMORY_END

MACHINE_DRIVER_START(taito)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(taito,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", 8080, 4000000)
  MDRV_CPU_MEMORY(taito_readmem, taito_writemem)
  MDRV_CPU_VBLANK_INT(taito_vblank, TAITO_VBLANKFREQ)
  MDRV_CPU_PERIODIC_INT(taito_irq, TAITO_IRQFREQ)
  MDRV_NVRAM_HANDLER(taito)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(taito)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x4000, 0x100, 0xff);
}
#if 0
static core_tData taitoData = {
  32, /* 32 Dips */
  taito_updSw, 1, sndbrd_0_data_w, "taito",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};
#endif
