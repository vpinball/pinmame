#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "taito.h"

#define TAITO_VBLANKFREQ     60  // VBLANK frequency
#define TAITO_IRQFREQ        0.2 

#define TAITO_SOLSMOOTH      2 /* Smooth the Solenoids over this numer of VBLANKS */
#define TAITO_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS */

static struct {
  int vblankCount;
  void* timer_irq;
  UINT8* pDisplayRAM;
} locals;

static NVRAM_HANDLER(taito);

static INTERRUPT_GEN(taito_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- display --*/
  if ((locals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
	int i = 0;

	for (i=0; i<14; i++)
	{
		((int*) coreGlobals.segments)[2*i]   = core_bcd2seg[(*(locals.pDisplayRAM+i)>>4)&0x0f];
		((int*) coreGlobals.segments)[2*i+1] = core_bcd2seg[*(locals.pDisplayRAM+i)&0x0f];
	}
  }

  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(taito) {
  if (inports) {
	coreGlobals.swMatrix[1] = (inports[TAITO_COMINPORT]&0xff)^0x40;
  }
}

static INTERRUPT_GEN(taito_irq) {
	// logerror("irq:\n");
	cpu_set_irq_line(TAITO_CPU, 0, HOLD_LINE);
}

static void timer_irq(int data) {
	taito_irq();
}

static MACHINE_INIT(taito) {
  memset(&locals, 0, sizeof(locals));

  locals.pDisplayRAM = memory_region(TAITO_MEMREG_CPU) + 0x4080;


  locals.timer_irq = timer_alloc(timer_irq);
  timer_adjust(locals.timer_irq, TIME_IN_HZ(TAITO_IRQFREQ), 0, TIME_IN_HZ(TAITO_IRQFREQ));

  locals.vblankCount = 1;
}

static MACHINE_STOP(taito) {
  if ( locals.timer_irq ) {
	  timer_remove(locals.timer_irq);
	  locals.timer_irq = NULL;
  }
}

static READ_HANDLER(switches_r) {
	if ( offset==7 )
		logerror("switch read: %i\n", offset);

	if ( offset==6 )
		return core_getDip(0)^0xff;
	else
		return coreGlobals.swMatrix[offset+1]^0xff;
}

static WRITE_HANDLER(switches_w) {
	logerror("switch write: %i %i\n", offset, data);
}

static WRITE_HANDLER(cmd_w) {
	logerror("command (?) write: %i %i\n", offset, data);
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
  MDRV_CORE_INIT_RESET_STOP(taito,NULL,taito)
  MDRV_CPU_ADD_TAG("mcpu", 8080, 4000000)
  MDRV_CPU_MEMORY(taito_readmem, taito_writemem)
  MDRV_CPU_VBLANK_INT(taito_vblank, TAITO_VBLANKFREQ)
  // MDRV_CPU_PERIODIC_INT(taito_irq, TAITO_IRQFREQ)
  MDRV_NVRAM_HANDLER(taito)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(taito)
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x4000, 0x100, 0xff);
}
