/************************************************************************************************
 Recel System III
 ----------------
   CPU:     Rockwell PPS-4/2 (11660) @ 198.864 kHz
   ROM:     A1761/A1762 RRIOT (1KB each) + 256B or 2KB game PROM behind two 10738 BICs
   IO:      11696 PIO (24 out), 10788 GPKD (displays), RRIOT I/O (lamps, NVRAM, printer)
   SOUND:   discrete TTL driven by PIO outputs 0-5
************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "sim.h"
#include "recel.h"
#include "cpu/pps4/pps4.h"

static struct {
  int vblankCount;
} locals;

/* The game PROM is read through the BICs, which invert both the address and the
   data bus because the PPS-4 uses negative logic and the EPROM's TTL side does
   not. Undo both to get executable code. */
static void recel_decode_prom(void) {
  UINT8 *raw = memory_region(RECEL_MEMREG_PROM);
  UINT8 *cpu = memory_region(RECEL_MEMREG_CPU);
  const int size = (core_gameData->hw.gameSpecific1 == 2) ? 0x800 : 0x100;
  int i;
  for (i = 0; i < size; i++)
    cpu[0x800 + i] = ~raw[size - 1 - i];
  /* Hardware version 2: the upper half of the decoded image replaces the
     A1762 ROM section, which the board disables. */
  if (size == 0x800)
    memcpy(cpu + 0x400, cpu + 0xC00, 0x400);
}

static INTERRUPT_GEN(RECEL_vblank) {
  locals.vblankCount++;
  core_updateSw(TRUE);
}

static MACHINE_INIT(RECEL) {
  memset(&locals, 0, sizeof locals);
  recel_decode_prom();
}

static MACHINE_STOP(RECEL) { }

static MEMORY_READ_START(RECEL_readmem)
  {0x0000,0x0fff, MRA_ROM},
  {0x1000,0x10ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(RECEL_writemem)
  {0x1000,0x10ff, MWA_RAM},
MEMORY_END

MACHINE_DRIVER_START(RECEL)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", PPS4, 3579545./18.)
  MDRV_CPU_MEMORY(RECEL_readmem, RECEL_writemem)
  MDRV_CPU_VBLANK_INT(RECEL_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(RECEL,NULL,RECEL)
  MDRV_DIPS(8)
MACHINE_DRIVER_END
