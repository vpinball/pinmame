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
  UINT8 coilSense;  /* returns read at strobe 10; §5.4 */
  /* GPKD (10788) state: two independent 16 x 4-bit registers, scanned in
     lockstep. docs/gpkd-protocol.md §1, §8. */
  UINT8 dispA[16], dispB[16];
  int ptrA, ptrB;
  int blankA, blankB;
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
  locals.ptrA = locals.ptrB = 15;
  locals.blankA = locals.blankB = 1;
  locals.coilSense = 0x0f;
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

/* 10788 GPKD, device 0xF. Push one group's 16 scan-time nibbles into
   coreGlobals.segments; canonical position = 16*group + scan time, group A
   at base 0, group B at base 16. docs/gpkd-protocol.md §4. core_bcd2seg7a[]
   already reads 0 for nibble 0xF, so per-digit blanking (7448, §7) falls out
   without a special case. */
static void gpkd_refresh(int group) {
  const UINT8 *disp  = group ? locals.dispB : locals.dispA;
  const int    blank = group ? locals.blankB : locals.blankA;
  const int    base  = group ? 16 : 0;
  int i;
  for (i = 0; i < 16; i++)
    coreGlobals.segments[base + i].w = blank ? 0 : core_bcd2seg7a[disp[i]];
}

/* Command table: docs/gpkd-protocol.md §1, §3. KAF/KBF blank a group
   without touching its contents; KLA/KLB write at the current pointer and
   then move it down one position. */
static void gpkd_w(int cmd, int accu) {
  switch (cmd) {
    case 0xe: /* KLA */
      locals.dispA[locals.ptrA] = accu;
      locals.ptrA = (locals.ptrA - 1) & 0x0f;
      gpkd_refresh(0);
      break;
    case 0xd: /* KLB */
      locals.dispB[locals.ptrB] = accu;
      locals.ptrB = (locals.ptrB - 1) & 0x0f;
      gpkd_refresh(1);
      break;
    case 0xb: /* KAF */
      locals.ptrA = 15;
      locals.blankA = 1;
      gpkd_refresh(0);
      break;
    case 0x7: /* KBF */
      locals.ptrB = 15;
      locals.blankB = 1;
      gpkd_refresh(1);
      break;
    case 0x3: /* KDN */
      locals.blankA = locals.blankB = 0;
      gpkd_refresh(0);
      gpkd_refresh(1);
      break;
    default:
      TRACE(("RECEL unhandled GPKD cmd=%x\n", cmd));
      break;
  }
}

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
    case RECEL_DEV_GPKD: gpkd_w(locals.cmd, locals.accu); break;
    default: break;
  }
}

static READ_HANDLER(recel_port_r) {
  /* GPKD does not drive I/D for any of its commands; the bus floats to
     all-ones. docs/gpkd-protocol.md §3.2. */
  if ((offset >> 4) == RECEL_DEV_GPKD) return 0x0f;
  return locals.accu;
}

/* Factory numbering: switch = strobe*10 + bit index (A..D = 1..4). Column 0 of
   swMatrix is reserved for dedicated switches (not part of Recel's strobed
   matrix), so strobe S lives in column S+1, bit (index-1). A plain
   "(col-1)*10 + row" / "(no/10+1)*8 + no%10" pair round-trips correctly for
   every real switch but is off by one bit within the column, and also fails
   core.c's MACHINE_INIT self-check loop at column 0 (C truncates negative
   division so (0-1)*10+row never maps back to column 0). Column 0 is folded
   into the disjoint negative range below instead, since no real switch number
   is negative. */
static int recel_sw2m(int no) {
  return (no / 10 + 1) * 8 + (no % 10 - 1);
}
static int recel_m2sw(int col, int row) {
  return col ? (col - 1) * 10 + (row + 1) : row - 7;
}

/* Cabinet switches (strobes 8-9) come from the input port, not the JC matrix.
   RECEL_COMPORTS packs strobe 8 in the low nibble and strobe 9 in the high
   nibble; column = strobe+1 as above. */
static SWITCH_UPDATE(RECEL) {
  if (inports) {
    CORE_SETKEYSW(inports[RECEL_COMINPORT], 0x0f, 9);
    CORE_SETKEYSW(inports[RECEL_COMINPORT] >> 4, 0x0f, 10);
  }
}

/* DOA -> 7404 -> 7445: 10 strobes, latched from the accumulator (port 0x100).
   The PPS-4/2's DOA also writes the X register to port 0x101 (pps4.c, the DOA
   case) but the Recel board wires only the four accumulator lines to the
   7445 [hardware notes §11.14]; that second nibble reaches no hardware here
   and is intentionally ignored. */
/* Traced as "RECELSW", not "RECEL ", so tests/test_ports.py's IOL-device
   trace parser (which requires every "RECEL "-prefixed line to be a strict
   key=value list keyed by IOL device id) does not try to parse it: DOA/DIA
   are dedicated CPU output/input instructions, not an IOL device access. */
static WRITE_HANDLER(sw_w) {
  TRACE(("RECELSW PC=%03x doa=%x offset=%x\n", activecpu_get_pc(), data, offset));
  if (!offset) locals.strobe = data & 0x0f;
}

static READ_HANDLER(sw_r) {
  UINT8 value;
  if (offset) return 0x0f;
  if (locals.strobe == 10) value = locals.coilSense;
  else if (locals.strobe > 9) value = 0x0f;
  /* Returns are active-low: +5V open, 0V closed. */
  else value = ~coreGlobals.swMatrix[locals.strobe + 1] & 0x0f;
  TRACE(("RECELSW PC=%03x strobe=%x dia=%x\n", activecpu_get_pc(), locals.strobe, value));
  return value;
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
  MDRV_SWITCH_UPDATE(RECEL)
  MDRV_SWITCH_CONV(recel_sw2m, recel_m2sw)
MACHINE_DRIVER_END
