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
  UINT8 coilSense;  /* returns read at strobe 10; §5.4. Recomputed from PIO
                        state by pio_set() -- see the comment there. */
  /* GPKD (10788) state: two independent 16 x 4-bit registers, scanned in
     lockstep. docs/gpkd-protocol.md §1, §8. */
  UINT8 dispA[16], dispB[16];
  int ptrA, ptrB;
  int blankA, blankB;
  /* 11696 PIO (device 0xD): 24 outputs in six nibble groups A-F (pio[0..5]).
     Outputs 0-5 sound, 6-15 coils, 16-19 bonus BCD, 20-23 indicators --
     docs/driver-notes.md §7 "Full PIO output allocation". solenoids mirrors
     every output bit-for-bit (bit N = PIO output N); sound/bonus additionally
     decode their own slice for other subsystems to read directly. */
  UINT8 pio[6];
  UINT8 pioPrevWrite;  /* group write returns the pre-write value; §5 table */
  UINT8 sound;
  UINT8 bonus;
  UINT32 solenoids;
  /* HM6508 NVRAM (1024x1 bit = 128 bytes), addressed serially by a CD4040
     counter driven from B1 (device 0x4) -- see b1_w(). */
  UINT16 nvAddr;
  UINT8 nvData[128];
  int nvClk, nvEnab, nvRset;
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
  /*-- lamps --*/
  if ((locals.vblankCount % RECEL_LAMPSMOOTH) == 0)
    memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix,
           sizeof(coreGlobals.lampMatrix));
  /*-- solenoids (PIO outputs 0-23, see locals.pio) --*/
  coreGlobals.solenoids = locals.solenoids;
  core_updateSw(TRUE);
}

static MACHINE_INIT(RECEL) {
  /* core_nvram() loads NVRAM into locals.nvData/nvAddr before cpu_run()
     starts, but cpu_run() then calls machine_init on every (re)start -- so
     the blanket clear below would erase what was just loaded. Round-trip
     the two persisted fields across it. */
  UINT8 nvData[sizeof locals.nvData];
  const UINT16 nvAddr = locals.nvAddr;
  memcpy(nvData, locals.nvData, sizeof nvData);
  memset(&locals, 0, sizeof locals);
  memcpy(locals.nvData, nvData, sizeof nvData);
  locals.nvAddr = nvAddr;
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

/* Coils sit on PIO outputs 6-15 (10 drivers, docs/driver-notes.md §7); 0-5 are
   sound and have no coil behind them. The self-check's coil test sets one
   output, immediately reads the power-play sense on switch strobe 10 (sw_r),
   then clears the output again (see the traced D6/tml-0x36E/DB sequence at
   ROM 0x7C5-0x7CD) -- so coilSense should follow whether *any* coil output is
   presently energized. Bit weights and the open-vs-normal split below are
   inferred from recel-system3-hardware.md's item 14 ("the manual describes
   the concept but ... that path is not identified") plus the one point that
   is measured: the constant 0x0F this replaces reliably renders the
   self-check's "coil open" digit, so 0x0F must mean open and is kept as the
   baseline. Flag as inference if this needs revisiting, same as the lamp
   polarity note above. */
#define RECEL_COIL_LO 6
#define RECEL_COIL_HI 15
static void update_coil_sense(void) {
  const UINT32 coilMask = ((2u << RECEL_COIL_HI) - (1u << RECEL_COIL_LO));
  locals.coilSense = (locals.solenoids & coilMask) ? 0x0e : 0x0f;
}

/* 11696 PIO output line -> subsystem state. `line` is the 0-based PIO output
   number from the table in docs/driver-notes.md §7 (solenoid N = PIO output
   N). solenoids mirrors every line as a flat bitmask for /api/info; sound and
   bonus additionally get their own decoded fields. */
static void pio_set(int line, int on) {
  if (on) locals.solenoids |=  (1u << line);
  else    locals.solenoids &= ~(1u << line);
  if (line < 6) {                                    /* sound (Task 12) */
    locals.sound = (UINT8)((locals.sound & ~(1 << line)) | (on << line));
  } else if (line >= 16 && line < 20) {               /* bonus BCD nibble */
    const int bit = line - 16;
    locals.bonus = (UINT8)((locals.bonus & ~(1 << bit)) | (on << bit));
  }
  update_coil_sense();
}

/* Command table: docs/driver-notes.md §5 "11696 PIO command encoding".
   Group write D0-D5 covers outputs 0-23 four at a time (group*4+bit); set/
   reset D6/DB address one of the 16 bit-addressable lines in groups A-D
   (outputs 0-15) by accumulator, datasheet-numbered IO1 (accu F) down to
   IO16 (accu 0) -- so output = 15-accu (accu F -> output 0, accu 0 ->
   output 15), algebraically the only formula fitting both given endpoints.
   Confirmed against the running ROM: the self-check's coil-test loop at
   0x7C5 drives the accumulator through 0..F in order (traced via -log,
   "dev=d cmd=6 accu=0".."accu=f"), which under this formula pulses outputs
   15 down to 0 -- the 10 coil outputs first (accu 0-9), then the 6 sound
   outputs (accu A-F), matching the "10 coils + 6 sound = 16 bit-addressable
   lines" allocation exactly with no leftover or overlap. */
static void pio_w(int cmd, int accu) {
  int i;
  if (cmd <= 0x05) {                        /* write group A..F */
    locals.pioPrevWrite = locals.pio[cmd];
    for (i = 0; i < 4; i++) pio_set(cmd * 4 + i, (accu >> i) & 1);
    locals.pio[cmd] = (UINT8)accu;
  } else if (cmd == 0x06 || cmd == 0x0b) {  /* set / reset one bit */
    const int line = 0x0f - accu;
    const int group = line / 4, bit = line % 4;
    pio_set(line, cmd == 0x06);
    if (cmd == 0x06) locals.pio[group] |= (UINT8)(1 << bit);
    else             locals.pio[group] &= (UINT8)~(1 << bit);
  } else if (cmd == 0x07 || cmd == 0x0e) {
    TRACE(("RECEL unhandled PIO cmd=%x\n", cmd));
  }
  /* else: cmd is a read-group opcode (8,9,a,c,d,f). IOL always writes then
     reads the same port with the same command byte, so every PIO read also
     reaches here first; there is nothing to do on the write side of a read. */
}

static int pio_r(int cmd) {
  switch (cmd) {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
      return locals.pioPrevWrite;   /* group write returns the pre-write value */
    case 0x08: return locals.pio[0];
    case 0x09: return locals.pio[1];
    case 0x0a: return locals.pio[2];
    case 0x0f: return locals.pio[3];  /* group D reads with DF, not DB */
    case 0x0c: return locals.pio[4];
    case 0x0d: return locals.pio[5];
  }
  return 0x0f;  /* set/reset/undocumented: bus floats high */
}

/* B1 (A1761, device 0x4) drives the HM6508 NVRAM (1024x1 bit = 128 bytes)
   bit-serially through an external CD4040 address counter. Line numbers are
   the A17xx's own I/O0..I/O15 pin index (docs/recel-system3-hardware.md
   §7.1.1's pin table, sourced from the disassembly), NOT the 0/1/2/3 first
   guessed for this task -- confirmed by disassembling the two RRIOT ROMs
   (`cat A1761-13_1K.bin A1762-13_1K.bin | unidasm -arch pps4`) and matching
   the result against the pin table:
     - RSET = line 5 ("RESET of the CD4040", active high). Both NVRAM entry
       points -- 0x2F4 "transfer RAM to HM6508" and 0x510 "test CMOS RAM and
       write it to working RAM" -- open with the identical `lbl 05; ldi 0;
       iol 41`, releasing reset exactly once at the start of the transfer.
     - ENAB = line 2 ("ENABLE HM6508", active low). 0x528 ("read one bit at
       current address") asserts it (accu=0) as its first action and
       deasserts it (accu=8) as its last, bracketing the whole access.
     - CLCK = line 4 ("CLOCK -- advances the CD4040"). Touched exactly once
       per call to 0x528, immediately after ENAB's deassert and right before
       return -- the structurally right spot to advance to the next address
       between accesses. The traced run only ever completes one partial pass
       (device 0x4 sees the same 36 accesses whether run for 900 or 3000
       frames, then goes silent), so a clean assert/deassert pulse pair was
       never observed here; the edge convention below follows the datasheet.
     - WTOU = line 1 ("Data out to HM6508"). The only line whose accu value
       differs between two otherwise-identical calls to the same PC (0x5B7)
       in different loop passes -- the signature of a serial data line, not
       a control strobe.
   Lines 0 (data in), 3 (read/write select) and 6/7 (STPR/RDPR, mini-printer)
   are outside this task's interface and are not modelled. */
#define RECEL_NV_WTOU 1
#define RECEL_NV_ENAB 2
#define RECEL_NV_CLCK 4
#define RECEL_NV_RSET 5

static void b1_w(int line, int cmd, int accu) {
  const int on = (accu & 0x08) ? 1 : 0;
  if (!(cmd & 0x1)) return;   /* SES: global enable, not a per-line value */
  switch (line) {
    case RECEL_NV_RSET:
      locals.nvRset = on;          /* active high, level-held */
      if (on) locals.nvAddr = 0;
      break;
    case RECEL_NV_ENAB:
      locals.nvEnab = !on;         /* active low */
      break;
    case RECEL_NV_CLCK:
      /* rising edge advances the counter, unless RSET is holding it at 0 */
      if (on && !locals.nvClk && !locals.nvRset)
        locals.nvAddr = (locals.nvAddr + 1) & 0x3ff;
      locals.nvClk = on;
      break;
    case RECEL_NV_WTOU:
      if (locals.nvEnab) {
        const int byte = locals.nvAddr >> 3, bit = locals.nvAddr & 7;
        if (on) locals.nvData[byte] |=  (1 << bit);
        else    locals.nvData[byte] &= ~(1 << bit);
      }
      break;
    default: break;
  }
}

static NVRAM_HANDLER(RECEL) {
  core_nvram(file, read_or_write, locals.nvData, sizeof locals.nvData, 0x00);
}

/* A17xx RRIOT command (docs/recel-system3-hardware.md §7.1): cmd bit 0 is c --
   c=0 SES sets the shared enable F/F for all 16 lines (A4=1 enable, A4=0
   disable/float; disable is never seen in the traced ROM and unmodelled
   here); c=1 SOS loads the addressed line's holding F/F from accu bit 3
   (A4), unchanged.

   On/off sense is an inference, not a measurement -- see docs/driver-notes.md
   "Lamp polarity (unresolved)". Device 0x2 is written 17 times total in the
   traced run (16x SOS then 1x SES, all at boot, never again); if that preset
   meant "all on", every lamp would stay lit while the self-check sits
   reporting a coil fault, which is implausible, so F/F=1 is treated as OFF
   here. The competing physical-chain reading (4050 buffer, MC140 sink, pin
   high => lamp on => F/F=1 is ON) skips the negative-logic bus between F/F
   and pin (§3.3: PPS-4 logic 1 = -12V) and so does not settle it either. */
static void b2_w(int line, int cmd, int accu) {
  if (!(cmd & 0x1)) return;   /* SES: global enable, not a per-line value */
  if (accu & 0x08) coreGlobals.tmpLampMatrix[line / 8] &= ~(1 << (line % 8));
  else             coreGlobals.tmpLampMatrix[line / 8] |=  (1 << (line % 8));
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
    case RECEL_DEV_B1:   b1_w(line, locals.cmd, locals.accu); break;
    case RECEL_DEV_B2:   b2_w(line, locals.cmd, locals.accu); break;
    case RECEL_DEV_PIO:  pio_w(locals.cmd, locals.accu); break;
    case RECEL_DEV_GPKD: gpkd_w(locals.cmd, locals.accu); break;
    default: break;
  }
}

static READ_HANDLER(recel_port_r) {
  /* GPKD does not drive I/D for any of its commands; the bus floats to
     all-ones. docs/gpkd-protocol.md §3.2. */
  if ((offset >> 4) == RECEL_DEV_GPKD) return 0x0f;
  if ((offset >> 4) == RECEL_DEV_PIO) return pio_r(locals.cmd);
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
  MDRV_NVRAM_HANDLER(RECEL)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(RECEL)
  MDRV_SWITCH_CONV(recel_sw2m, recel_m2sw)
MACHINE_DRIVER_END
