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
#define TAITO_LAMPSMOOTH	 2 /* Smooth the display over this number of VBLANKS */

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT8 lampMatrix[CORE_MAXLAMPCOL];
  int sndCmd, oldsndCmd;

  void* timer_irq;
  UINT8* pDisplayRAM;
  UINT8* pCommandsDMA;
} TAITOlocals;

static NVRAM_HANDLER(taito);

static int segMap[] = {
  4,0,-4,4,0,-4,4,0,-4,4,0,-4,0,0
};

static INTERRUPT_GEN(taito_vblank) {
	/*-------------------------------
	/  copy local data to interface
	/--------------------------------*/
	TAITOlocals.vblankCount += 1;

	/*-- solenoids --*/
	if ((TAITOlocals.vblankCount % TAITO_SOLSMOOTH) == 0) {
		coreGlobals.solenoids = TAITOlocals.solenoids;
	}

	/*-- lamps --*/
 	if ((TAITOlocals.vblankCount % TAITO_LAMPSMOOTH) == 0) {
 		memcpy(coreGlobals.lampMatrix, TAITOlocals.lampMatrix, sizeof(coreGlobals.lampMatrix));
 	}

  /*-- display --*/
  if ((TAITOlocals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
	memcpy(coreGlobals.segments, TAITOlocals.segments, sizeof coreGlobals.segments);
  }

  core_updateSw(core_getSol(18));
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
  memset(&TAITOlocals, 0, sizeof(TAITOlocals));

  TAITOlocals.pDisplayRAM  = memory_region(TAITO_MEMREG_CPU) + 0x4080;
  TAITOlocals.pCommandsDMA = memory_region(TAITO_MEMREG_CPU) + 0x4090;

  TAITOlocals.timer_irq = timer_alloc(timer_irq);
  timer_adjust(TAITOlocals.timer_irq, TIME_IN_HZ(TAITO_IRQFREQ), 0, TIME_IN_HZ(TAITO_IRQFREQ));

  sndbrd_0_init(core_gameData->hw.soundBoard, TAITO_SCPU, memory_region(TAITO_MEMREG_SCPU), NULL, NULL);

  TAITOlocals.vblankCount = 1;
}

static MACHINE_STOP(taito) {
  if ( TAITOlocals.timer_irq ) {
	  timer_remove(TAITOlocals.timer_irq);
	  TAITOlocals.timer_irq = NULL;
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

// display
static WRITE_HANDLER(dma_display)
{
	TAITOlocals.pDisplayRAM[offset] = data;

	if ( offset<12 ) {
		((int*) TAITOlocals.segments)[2*offset+segMap[offset]]   = core_bcd2seg[(data>>4)&0x0f];
		((int*) TAITOlocals.segments)[2*offset+segMap[offset]+1] = core_bcd2seg[data&0x0f];
	}
	else {
		switch ( offset ) {
		case 12:
			((int*) TAITOlocals.segments)[2*12+segMap[12]] = core_bcd2seg[data&0x0f];
			break;
		
		case 13:
			((int*) TAITOlocals.segments)[2*12+segMap[12]+1] = core_bcd2seg[data&0x0f];
			break;
		
		case 14:
			((int*) TAITOlocals.segments)[2*13+segMap[13]] = core_bcd2seg[data&0x0f];
			break;

		case 15:
			((int*) TAITOlocals.segments)[2*13+segMap[13]+1] = core_bcd2seg[data&0x0f];
			break;
		}
	}
}

// sols, sound and lamps
static WRITE_HANDLER(dma_commands)
{
	// upper nibbles of offset 0-1: solenoids
	TAITOlocals.pCommandsDMA[offset] = data;

	switch ( offset ) {
	case 0:
		// upper nibble: - solenoids 17-18 (mux relay and play relay) 
		//				 - solenoids 5-6 or 11-12 depending on mux relay
		TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xfffcffff) | ((data&0xc0)<<10);
		if ( TAITOlocals.solenoids&0x10000 )
			// 11-12
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffff33ff) | ((data&0x30)<<6);
		else
			// 5-6
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffff3fcf) | (data&0x30);
		break;

	case 1:
		// upper nibble: solenoids 1-4 or 7-10 depending on mux relay
		if ( TAITOlocals.solenoids&0x10000 )
			// 7-10
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffff3c3f) | ((data&0xf0)<<2);
		else
			// 1-4
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffff3ff0) | ((data&0xf0)>>4);
		break;

	// upper nibbles of offset 2-3: sound commands
	case 2:
		// upper nibble: sound command 1-4
		TAITOlocals.sndCmd = (TAITOlocals.sndCmd & 0xf0) | ((data&0xf0)>>4);
		break;

	case 3:
		// upper nibble: sound command 5-8
		TAITOlocals.sndCmd = (TAITOlocals.sndCmd & 0x0f) | (data&0xf0);
		break;

	}

	if ( TAITOlocals.oldsndCmd!=TAITOlocals.sndCmd ) {
		TAITOlocals.oldsndCmd = TAITOlocals.sndCmd;

		taito_sndCmd_w(0, TAITOlocals.sndCmd);
	}

	// lower nibbles: lamps, offset 0-f
	// Taito uses 16 rows and 4 cols, rows 9-16 are mapped to row 1-8, cols 5-8
	if ( offset<8 )
		TAITOlocals.lampMatrix[offset] = (TAITOlocals.lampMatrix[offset]&0xf0) | (data&0x0f);
	else
		TAITOlocals.lampMatrix[offset-8] = (TAITOlocals.lampMatrix[offset-8]&0x0f) | ((data&0x0f)<<4);
}

static int TAITO_lamp2m(int no) { 
	if ( (no/10)<8 )
		return (4-(no%10))*8 + (no/10);
	else
		return (8-(no%10))*8 + ((no/10)-8);
}

static int TAITO_m2lamp(int col, int row) { 
	if ( row<8 )
		return (4-col)*8 + row;
	else
		return (8-col)*8 + (row-8);
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
  MDRV_NVRAM_HANDLER(taito)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(taito)
  MDRV_LAMP_CONV(TAITO_lamp2m,TAITO_m2lamp)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintevox)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintevox)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintetizador)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintetizador)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintevoxpp)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintevoxpp)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x4000, 0x100, 0xff);
}
