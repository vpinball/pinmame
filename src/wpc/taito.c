#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "taito.h"
#include "taitos.h"

#define TAITO_VBLANKFREQ     60  // VBLANK frequency
#define TAITO_IRQFREQ        0.2

#define TAITO_SOLSMOOTH      2 /* Smooth the Solenoids over this numer of VBLANKS */
#define TAITO_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS */

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  int sndCmd, oldsndCmd;

  void* timer_irq;
  UINT8* pDisplayRAM;
  UINT8* pCommandsDMA;
} locals;

static NVRAM_HANDLER(taito);

static int segMap[] = {
  4,0,-4,4,0,-4,4,0,-4,4,0,-4,0,0
};

static INTERRUPT_GEN(taito_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- solenoids --*/
  if ((locals.vblankCount % TAITO_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
  }

  /*-- display --*/
  if ((locals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
	memcpy(coreGlobals.segments, locals.segments, sizeof coreGlobals.segments);
/*
	int i = 0;

	for (i=0; i<14; i++)
	{
		((int*) coreGlobals.segments)[2*i+segMap[i]]   = core_bcd2seg[(*(locals.pDisplayRAM+i)>>4)&0x0f];
		((int*) coreGlobals.segments)[2*i+segMap[i]+1] = core_bcd2seg[*(locals.pDisplayRAM+i)&0x0f];
	}
*/
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

  locals.pDisplayRAM  = memory_region(TAITO_MEMREG_CPU) + 0x4080;
  locals.pCommandsDMA = memory_region(TAITO_MEMREG_CPU) + 0x4090;

  locals.timer_irq = timer_alloc(timer_irq);
  timer_adjust(locals.timer_irq, TIME_IN_HZ(TAITO_IRQFREQ), 0, TIME_IN_HZ(TAITO_IRQFREQ));

  sndbrd_0_init(core_gameData->hw.soundBoard, TAITO_SCPU, memory_region(TAITO_MEMREG_SCPU), NULL, NULL);

  locals.vblankCount = 1;
}

static MACHINE_STOP(taito) {
  if ( locals.timer_irq ) {
	  timer_remove(locals.timer_irq);
	  locals.timer_irq = NULL;
  }
  sndbrd_0_exit();
}

static WRITE_HANDLER(taito_sndCmd_w) {
	// logerror("sound cmd: 0x%02x\n", data);
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

    sndbrd_0_data_w(0, data);
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

static WRITE_HANDLER(dma_display)
{
	locals.pDisplayRAM[offset] = data;

	((int*) locals.segments)[2*offset+segMap[offset]]   = core_bcd2seg[(data>>4)&0x0f];
	((int*) locals.segments)[2*offset+segMap[offset]+1] = core_bcd2seg[data&0x0f];
}

static WRITE_HANDLER(dma_commands)
{
	// upper nibbles of offset 0-1: solenoids
	// upper nibbles of offset 2-3: sound commands
	// lower nibbles: lamps, offset 0-?
	locals.pCommandsDMA[offset] = data;

	switch ( offset ) {
	case 0:
		// upper nibble: - solenoids 13-14 (mux relay and play relay) 
		//				 - solenoids 5-6 or 11-12 depending on mux relay
		locals.solenoids = (locals.solenoids & 0xfff) | ((data&0xc0)<<6);
		if ( locals.solenoids&0x1000 )
			// 11-12
			locals.solenoids = (locals.solenoids & 0x33ff) | ((data&0x30)<<6);
		else
			// 5-6
			locals.solenoids = (locals.solenoids & 0x3fcf) | (data&0x30);
		break;

	case 1:
		// upper nibble: solenoids 1-4 or 7-10 depending on mux relay
		if ( locals.solenoids&0x1000 )
			// 7-10
			locals.solenoids = (locals.solenoids & 0x3c3f) | ((data&0xf0)<<2);
		else
			// 1-4
			locals.solenoids = (locals.solenoids & 0x3ff0) | ((data&0xf0)>>4);
		break;

	case 2:
		// upper nibble: sound command 1-4
		locals.sndCmd = (locals.sndCmd & 0xf0) | ((data&0xf0)>>4);
		break;

	case 3:
		// upper nibble: sound command 5-8
		locals.sndCmd = (locals.sndCmd & 0x0f) | (data&0xf0);
		break;

	}

	if ( locals.oldsndCmd!=locals.sndCmd ) {
		locals.oldsndCmd = locals.sndCmd;

		taito_sndCmd_w(0, locals.sndCmd);
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
  { 0x4000, 0x407f, MWA_RAM },
  { 0x4080, 0x408f, dma_display },
  { 0x4090, 0x409f, dma_commands },
  { 0x40a0, 0x40ff, MWA_RAM },
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

  MDRV_IMPORT_FROM(taitos)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x4000, 0x100, 0xff);
}
