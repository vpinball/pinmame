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

/* logerror() only writes when the emulator was started with -log <file>, so
   this is free in normal runs and is how the tests capture I/O. */
#define RECEL_TRACE 1
#if RECEL_TRACE
#define TRACE(x) logerror x
#else
#define TRACE(x)
#endif

static struct {
  int vblankCount;
  UINT8 accu;
  int cmd;
  int strobe;
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

/* IOL issues a write then a read on the same port. The command nibble only
   travels with the write, so the read handler recovers it from locals.cmd. */
static WRITE_HANDLER(recel_port_w) {
  const int device = offset >> 4;
  const int line   = offset & 0x0f;
  locals.cmd  = data >> 4;
  locals.accu = data & 0x0f;
  TRACE(("RECEL PC=%03x dev=%x cmd=%x accu=%x b=%x\n",
         activecpu_get_pc(), device, locals.cmd, locals.accu, line));
  switch (device) {
    case RECEL_DEV_B1:   break;
    case RECEL_DEV_B2:   break;
    case RECEL_DEV_PIO:  break;
    case RECEL_DEV_GPKD: break;
    default: break;
  }
}

static READ_HANDLER(recel_port_r) {
  return locals.accu;
}

static WRITE_HANDLER(sw_w) {
  if (!offset) locals.strobe = data & 0x0f;
}

static READ_HANDLER(sw_r) {
  return 0x0f;  /* all returns open until Task 6 */
}

static PORT_READ_START(RECEL_readport)
  {0x000,0x0ff, recel_port_r},
  {0x100,0x101, sw_r},
PORT_END

static PORT_WRITE_START(RECEL_writeport)
  {0x000,0x0ff, recel_port_w},
  {0x100,0x101, sw_w},
PORT_END

MACHINE_DRIVER_START(RECEL)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", PPS4, 3579545./18.)
  MDRV_CPU_MEMORY(RECEL_readmem, RECEL_writemem)
  MDRV_CPU_PORTS(RECEL_readport, RECEL_writeport)
  MDRV_CPU_VBLANK_INT(RECEL_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(RECEL,NULL,RECEL)
  MDRV_DIPS(8)
MACHINE_DRIVER_END
