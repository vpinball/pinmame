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

#define RECEL_SENSE_OPEN  0x0f  /* both comparators high: nothing drawing */
#define RECEL_SENSE_NORM  0x0d  /* return B drops: normal coil consumption */

static struct {
  int vblankCount;
  UINT8 accu;
  int cmd;
  int strobe;
  UINT8 coilSense;  /* returns read at strobe 10; §5.4. Recomputed from PIO
                        state by pio_set() -- see the comment there. */
  /* GPKD (10788) state: two independent 16 x 4-bit registers, scanned in
     lockstep. */
  UINT8 dispA[16], dispB[16];
  int ptrA, ptrB;
  int blankA, blankB;
  /* 11696 PIO (device 0xD): 24 outputs in six nibble groups A-F (pio[0..5]).
     Outputs 0-5 sound, 6-15 coils, 16-19 bonus BCD, 20-23 indicators.
     solenoids mirrors every output bit-for-bit (bit N = PIO output N).
     Sound additionally decodes its own slice, since recel_snd_w() needs a
     6-bit value rather than individual bits; the bonus BCD nibble has no
     such consumer and is read straight off ((solenoids >> 16) & 0xF). */
  UINT8 pio[6];
  UINT8 pioPrevWrite;  /* group write returns the pre-write value; §5 table */
  UINT8 sound;
  UINT32 solenoids;
  /* A17xx RRIOT I/O (B1 and B2): one holding F/F per line. 1 = released, so
     the pin floats; 0 = driven to +5V. a17_pin() turns that into what a read
     of the line sees. ioPrev holds the pin state sampled before the current
     access, which is what SES/SOS return (§7.1's command table). */
  UINT16 a17Rel[2];
  int ioPrev;
  /* HM6508 NVRAM (1024x1 bit = 128 bytes), addressed serially by a CD4040
     counter driven from B1 (device 0x4) -- see b1_w(). */
  UINT16 nvAddr;
  UINT8 nvData[128];
  int nvClk, nvEnab, nvRset, nvDin, nvWr;
} locals;

/* The game PROM is read through the BICs, which invert both the address and the
   data bus because the PPS-4 uses negative logic and the EPROM's TTL side does
   not. Undo both to get executable code. */
static void recel_decode_prom(void) {
  UINT8 *raw = memory_region(RECEL_MEMREG_PROM);
  UINT8 *cpu = memory_region(RECEL_MEMREG_CPU);
  int size, i;
  /* The `recel` BIOS parent set (RECEL_BIOS_ROMSTART) ships no game PROM, so
     it has no REGION_USER1 at all. Nothing to decode. */
  if (!raw || !cpu) return;
  size = (core_gameData->hw.gameSpecific1 == 2) ? 0x800 : 0x100;
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
  /*-- the counters' hardwired x1 digit (recel.h, RECEL_SEG_UNITS) --*/
  coreGlobals.segments[RECEL_SEG_UNITS].w = core_bcd2seg7a[0];
  core_updateSw(TRUE);
}

static MACHINE_INIT(RECEL) {
  /* core_nvram() loads NVRAM into locals.nvData before cpu_run() starts, but
     cpu_run() then calls machine_init on every (re)start -- so the blanket
     clear below would erase what was just loaded. Round-trip the one field
     core_nvram() actually persists (NVRAM_HANDLER(RECEL) below serializes
     only nvData, not nvAddr) across it.
     nvAddr is reset to 0, not round-tripped: a warm reset releases RSET,
     which asserts the CD4040's reset and forces the counter to 0 on real
     hardware, so carrying the old address forward would be *less* faithful.
     nvRset/nvClk/nvWr start released (1), matching every A17xx line after
     reset -- memset()'ing them to 0 ("driven") would let b1_w()'s edge
     detectors see a first re-release as a spurious rising edge. */
  UINT8 nvData[sizeof locals.nvData];
  memcpy(nvData, locals.nvData, sizeof nvData);
  memset(&locals, 0, sizeof locals);
  memcpy(locals.nvData, nvData, sizeof nvData);
  locals.nvAddr = 0;
  locals.nvRset = locals.nvClk = locals.nvWr = 1;
  locals.ptrA = locals.ptrB = 15;
  locals.blankA = locals.blankB = 1;
  locals.coilSense = RECEL_SENSE_OPEN;
  locals.a17Rel[0] = locals.a17Rel[1] = 0xffff;
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

/* Which of a group's 16 columns are digits. Columns 2, 8, 9 and A are
   latched in a 7475 rather than clocked with the digit scan, but latched is
   not undecoded: 2/A are a player's status LEDs and 8 the ball/tilt/game-over
   block, both lamps, while 9 is the match number, a decoded digit on the
   095-108 unit. Group B's 8/9 drive no indicator on a real machine. Returns
   GPKD_LAMP with the custom column in *lampcol, else GPKD_DIGIT/UNUSED. */
enum { GPKD_DIGIT, GPKD_LAMP, GPKD_UNUSED };
static int gpkd_kind(int group, int col, int *lampcol) {
  switch (col) {
    case 2:
      *lampcol = group ? RECEL_LAMPCOL_P4STATUS : RECEL_LAMPCOL_P1STATUS;
      return GPKD_LAMP;
    case 10:
      *lampcol = group ? RECEL_LAMPCOL_P3STATUS : RECEL_LAMPCOL_P2STATUS;
      return GPKD_LAMP;
    case 8:
      if (group) return GPKD_UNUSED;
      *lampcol = RECEL_LAMPCOL_GAMESTATE;
      return GPKD_LAMP;
    case 9:
      return group ? GPKD_UNUSED : GPKD_DIGIT;
    default:
      return GPKD_DIGIT;
  }
}

/* Column 8's nibble -> the named indicator bits (recel.h, RECEL_IND_*).
   A 7445 lights exactly one of BALL 1..5 / GAME OVER at a time. */
static UINT8 gamestate_lamps(UINT8 nibble) {
  const int code = nibble & 0x07;
  UINT8 out = (nibble & 0x08) ? RECEL_IND_TILT : 0;
  if (code <= 4)       out |= (UINT8)(RECEL_IND_BALL1 << code);
  else if (code == 7)  out |= RECEL_IND_GAMEOVER;
  return out;
}

/* 10788 GPKD, device 0xF. Push one group's 16 scan-time nibbles into
   coreGlobals.segments for genuine digit columns, or coreGlobals.tmpLampMatrix
   for latched columns; canonical position = 16*group + scan time, group A at
   base 0, group B at base 16. core_bcd2seg7a[]
   already reads 0 for nibble 0xF, so per-digit blanking (7448, §7) falls out
   without a special case. Latched columns get no such treatment: a 7475 has
   no blanking input, so they always show the last nibble written regardless
   of blankA/blankB. */
static void gpkd_refresh(int group) {
  const UINT8 *disp  = group ? locals.dispB : locals.dispA;
  const int    blank = group ? locals.blankB : locals.blankA;
  const int    base  = group ? 16 : 0;
  int i, lampcol;
  for (i = 0; i < 16; i++) {
    switch (gpkd_kind(group, i, &lampcol)) {
      case GPKD_LAMP:
        coreGlobals.tmpLampMatrix[lampcol] =
          (lampcol == RECEL_LAMPCOL_GAMESTATE) ? gamestate_lamps(disp[i])
                                               : disp[i];
        coreGlobals.segments[base + i].w = 0;
        break;
      case GPKD_UNUSED:
        coreGlobals.segments[base + i].w = 0;
        break;
      default: /* GPKD_DIGIT */
        coreGlobals.segments[base + i].w = blank ? 0 : core_bcd2seg7a[disp[i]];
        break;
    }
  }
}

/* Command table (Rockwell 10788). KAF/KBF blank a group
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

/* Power-play / driver / coil sense, read as returns A and B during strobe 10
   (the System III Operation and Maintenance manual §5.7): the K relay's contact
   is bridged by a 39 ohm resistor so the CPU can measure how much the coil
   chain draws with power play off. The manual's own table gives three states,
   and the self-check at ROM 0x7C0 masks the reading with 3 and demands
   exactly those values:
     3 = IN > 15V, no consumption   -> idle, or "coil X open"  (code X.4.7)
     1 = 4V < IN < 15V, normal draw -> the coil answered       (loop continues)
     0 = IN < 4V, short             -> "short in coil X"       (code X.4.4)
   The self-check sets one output, reads the sense eight times over ~260ms
   (0x7EA insists on eight identical samples), then clears it again.
   Registers #6-#F are the ten coil drivers; #0-#5 drive the discrete sound
   section, which is not on power play and so reports X.4.7 for each.

   Simplification: all ten drivers count as loaded. A game that wires fewer
   (Fair Fight uses #6-#B) would report the rest as "coil open" on real
   hardware; that needs per-game coil data the driver does not carry. */
#define RECEL_COIL_LO 6
#define RECEL_COIL_HI 15
static void update_coil_sense(void) {
  const UINT32 coilMask = ((2u << RECEL_COIL_HI) - (1u << RECEL_COIL_LO));
  locals.coilSense = (locals.solenoids & coilMask) ? RECEL_SENSE_NORM
                                                   : RECEL_SENSE_OPEN;
}

/* 11696 PIO output -> subsystem state. `out` is the factory register number
   from the System III Operation and Maintenance manual §7.2.2 (solenoid N = register #N):
   #0-#5 sound, #6-#F coils, 16-19 bonus BCD, 20-23 indicators. solenoids
   mirrors every output as a flat bitmask for /api/info; sound additionally
   gets its own decoded field, since recel_snd_w() wants a 6-bit value. */
static void pio_set(int out, int on) {
  if (on) locals.solenoids |=  (1u << out);
  else    locals.solenoids &= ~(1u << out);
  if (out < 6) {                                     /* sound */
    locals.sound = (UINT8)((locals.sound & ~(1 << out)) | (on << out));
    recel_snd_w(locals.sound);
  }
  update_coil_sense();
}

/* Group A-D bit -> register number. The 11696's own output index and Recel's
   register numbering run in opposite directions: group A bit 1 is IO1, which
   the factory calls #F, down to group D bit 8 = IO16 = #0
   (the System III Operation and Maintenance manual §7.2.2). Groups E and F are not
   bit-addressable and keep the flat 16-23 numbering. */
static int pio_reg(int group, int bit) {
  return (group < 4) ? 0x0f - (group * 4 + bit) : group * 4 + bit;
}

/* Command table (Rockwell 11696).
   Group write D0-D5 covers all 24 outputs four at a time; set/reset D6/DB
   address one of the 16 lines in groups A-D by accumulator, and the
   accumulator value *is* the factory register number -- #F down to #0, which
   is why the self-check's coil loop at 0x7C5 walks the accumulator 0..F and
   the manual reads the same digit back as "coil X open / test sound when
   X<5" (the System III Operation and Maintenance manual §3.2 step 5; on that bound see
   update_coil_sense above). pio_reg() converts a group write's bit position
   into the same numbering. */
static void pio_w(int cmd, int accu) {
  int i;
  if (cmd <= 0x05) {                        /* write group A..F */
    locals.pioPrevWrite = locals.pio[cmd];
    for (i = 0; i < 4; i++) pio_set(pio_reg(cmd, i), (accu >> i) & 1);
    locals.pio[cmd] = (UINT8)accu;
  } else if (cmd == 0x06 || cmd == 0x0b) {  /* set / reset one bit */
    const int line = 0x0f - accu;
    const int group = line / 4, bit = line % 4;
    pio_set(accu, cmd == 0x06);
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
    /* A mutation here (returning locals.pio[cmd], the post-write value,
       instead) survives the test suite -- but this is grounded in the
       documented "returns the port's previous state in A" (§5's command
       table), and no observable behaviour depends on which one runs, so it
       is left as specified rather than as whatever the tests happen to
       pin down. */
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
   the A17xx's own I/O0..I/O15 pin index (the System III Operation and Maintenance manual 7.1.1's pin table).

   Signal sense. An A17xx line's holding F/F either releases the pin (it
   floats to -12V, which the part reads back as 1) or drives it to +5V (read
   back as 0); SOS with A4=1 releases. Two statements in §7.1.1 fix that
   direction independently -- "IO2 = 0 => output +5V => HM6508 enabled" and
   "IO5 = 1 => pin Hi-Z"; the ROM agrees, since its write-a-1 helper (0x5B2)
   loads A4=0 and its write-a-0 helper (0x5B1) loads A4=1. That is the
   inversion §7.1.1 flags on IO1, and it is now applied.

     IO0 RDAT  data in.  Left released; the HM6508's data output drives it
                         while the chip is enabled.
     IO1 WTOU  data out. HM6508 sees the *complement* of the F/F.
     IO2 ENAB  chip enable, asserted when the pin is driven.
     IO3 RWRT  write strobe. Driven low then released around the data bit
                (ROM 0x5B0 and 0x5C0); the trailing edge latches, as on any
                SRAM /WE, which also keeps step 4's line-by-line I/O test
                from writing through a half-set-up bus.
     IO4 CLCK  clocks the CD4040 one address forward per pulse.
     IO5 RSET  holds the CD4040 at 0 while released.
   IO6/IO7 (STPR/RDPR, mini-printer) are touched by the NVRAM routines but
   drive nothing here, and IO8-IO15 are playfield outputs; all of them still
   need their released/driven state tracked, because step 4 of the self-check
   reads every line back. */
#define RECEL_NV_RDAT 0
#define RECEL_NV_WTOU 1
#define RECEL_NV_ENAB 2
#define RECEL_NV_RWRT 3
#define RECEL_NV_CLCK 4
#define RECEL_NV_RSET 5

#define A17IDX(dev) ((dev) == RECEL_DEV_B1 ? 0 : 1)

static int nv_bit(void) {
  return (locals.nvData[(locals.nvAddr >> 3) & 0x7f] >> (locals.nvAddr & 7)) & 1;
}

/* B2's IO15 is hard-tied to +5V on the board, so it reads back as driven
   however its holding F/F is set (spider.pps4.fr). That is not cosmetic. Step 4 of the self-check reads
   every A1762 output back one at a time and stops at the first that does
   not answer, so it ends on the minor fault 2.4.F -- which §10.3 notes
   displays as the documented "2.4." pass indication, because nibble F
   blanks. Stopping there is what leaves BM=4 and X=2 in the CPU for step 5
   to run on, and those are exactly the registers the manual's own coil
   codes X.4.7 / X.4.4 and power-play codes 2.4.5 / 2.4.6 spell out. Were
   all 16 lines to read clean, step 4 would instead fall through 0x75C's
   "lbl 00", step 5's per-coil scratch would land in the SAG page at
   M[00..0F], the 33ms delay at 0x1D2 would overwrite M[00]/M[01] on every
   call, and the eight-identical-samples loop at 0x7EA could never settle
   for register #1. */
#define RECEL_B2_TIEDHIGH 0x8000

/* What a read of one A17xx line sees. A driven line reads back as its own
   +5V; a released one reads the external signal, which is -12V (= 1) on
   every Recel line except B1's IO0 while the HM6508 is answering and the
   one tied-high B2 output (IO15) above. */
static int a17_pin(int device, int line) {
  if (!(locals.a17Rel[A17IDX(device)] & (1 << line))) return 0x00;
  if (device == RECEL_DEV_B1 && line == RECEL_NV_RDAT && locals.nvEnab)
    return nv_bit() ? 0x0f : 0x00;
  if (device == RECEL_DEV_B2 && (RECEL_B2_TIEDHIGH & (1 << line))) return 0x00;
  return 0x0f;
}

static void b1_w(int line, int cmd, int accu) {
  const int rel = (accu & 0x08) ? 1 : 0;
  if (!(cmd & 0x1)) return;   /* SES: global enable, not a per-line value */
  if (rel) locals.a17Rel[0] |=  (UINT16)(1 << line);
  else     locals.a17Rel[0] &= (UINT16)~(1 << line);
  switch (line) {
    case RECEL_NV_RSET:
      locals.nvRset = rel;         /* released = reset asserted, level-held */
      if (rel) locals.nvAddr = 0;
      break;
    case RECEL_NV_ENAB:
      locals.nvEnab = !rel;
      break;
    case RECEL_NV_CLCK:
      if (rel && !locals.nvClk && !locals.nvRset)
        locals.nvAddr = (locals.nvAddr + 1) & 0x3ff;
      locals.nvClk = rel;
      break;
    case RECEL_NV_WTOU:
      locals.nvDin = !rel;
      break;
    case RECEL_NV_RWRT:
      if (rel && !locals.nvWr && locals.nvEnab) {
        const int byte = (locals.nvAddr >> 3) & 0x7f, bit = locals.nvAddr & 7;
        if (locals.nvDin) locals.nvData[byte] |=  (UINT8)(1 << bit);
        else              locals.nvData[byte] &= (UINT8)~(1 << bit);
      }
      locals.nvWr = rel;
      break;
    default: break;
  }
}

/* CMOS nibble n backs RAM cell n (restore loop at 0x540), so an adjustment
   in RAM cell c lives in CMOS byte c/2, low half for even c. A0/A1/B0 are
   the three coin tables, B1 the mode of play.

   A blank CMOS reads 0 throughout, which the ROM takes as chute 1 "2 coins,
   1 play", chute 3 "0 plays per coin" (a dead chute) and 5 balls per game.
   Faithful to an unprogrammed board, so only a first run is seeded -- with
   1 coin = 1 play and the 3 balls Fair Fight's instruction card specifies. */
#define RECEL_NV_CHUTE1 (0xa0 / 2)   /* value 4 = 4*(1 coin) + (plays-1) */
#define RECEL_NV_CHUTE3 (0xb0 / 2)   /* low nibble: plays per coin.  The high
                                        nibble of the same byte is cell B1,
                                        the mode of play. */
#define RECEL_NV_3BALLS 0x80         /* B1 bit 3, in that high nibble */

static NVRAM_HANDLER(RECEL) {
  const int firstRun = !read_or_write && !file;
  core_nvram(file, read_or_write, locals.nvData, sizeof locals.nvData, 0x00);
  if (firstRun) {
    locals.nvData[RECEL_NV_CHUTE1] = 0x04;
    locals.nvData[RECEL_NV_CHUTE3] = 0x01 | RECEL_NV_3BALLS;
  }
}

/* A17xx RRIOT command (the System III Operation and Maintenance manual §7.1): cmd bit 0 is c --
   c=0 SES sets the shared enable F/F for all 16 lines (A4=1 enable, A4=0
   disable/float; disable is never seen in the traced ROM and unmodelled
   here); c=1 SOS loads the addressed line's holding F/F from accu bit 3
   (A4), unchanged.

   F/F=1 is OFF, confirmed against a gameplay recording of a real cabinet:
   the two SPECIAL lamps (lines 3 and 7), which this reading leaves dark
   during play, are lit in ~1-12% of frames. */
static void b2_w(int line, int cmd, int accu) {
  if (!(cmd & 0x1)) return;   /* SES: global enable, not a per-line value */
  if (accu & 0x08) {
    locals.a17Rel[1] |= (UINT16)(1 << line);
    coreGlobals.tmpLampMatrix[line / 8] &= ~(1 << (line % 8));
  } else {
    locals.a17Rel[1] &= (UINT16)~(1 << line);
    coreGlobals.tmpLampMatrix[line / 8] |=  (1 << (line % 8));
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
    /* SES and SOS both answer with the addressed pin's state *before* the
       access (§7.1's command table); the ROM leans on that hard, e.g. to
       turn "drive low, then write back what you read" into a strobe pulse. */
    case RECEL_DEV_B1:   locals.ioPrev = a17_pin(device, line);
                         b1_w(line, locals.cmd, locals.accu); break;
    case RECEL_DEV_B2:   locals.ioPrev = a17_pin(device, line);
                         b2_w(line, locals.cmd, locals.accu); break;
    case RECEL_DEV_PIO:  pio_w(locals.cmd, locals.accu); break;
    case RECEL_DEV_GPKD: gpkd_w(locals.cmd, locals.accu); break;
    default: break;
  }
}

static READ_HANDLER(recel_port_r) {
  /* GPKD does not drive I/D for any of its commands; the bus floats to
     all-ones. */
  const int device = offset >> 4;
  if (device == RECEL_DEV_GPKD) return 0x0f;
  if (device == RECEL_DEV_PIO) return pio_r(locals.cmd);
  if (device == RECEL_DEV_B1 || device == RECEL_DEV_B2) return locals.ioPrev;
  /* Unreachable today (devices 0x2/0x4/0xD/0xF cover every case above), but
     this is the exact shape of the bug that gated the self-check for most
     of this project: returning the just-written accumulator instead of a
     floating bus. Return 0x0f, matching every other undriven read here. */
  return 0x0f;
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
   7445; that second nibble reaches no hardware here
   and is intentionally ignored. */
/* Traced as "RECELSW", not "RECEL ": DOA/DIA are dedicated CPU output and
   input instructions, not an IOL device access, so they do not belong in the
   device-keyed IOL trace. */
static WRITE_HANDLER(sw_w) {
  TRACE(("RECELSW PC=%03x doa=%x offset=%x\n", activecpu_get_pc(), data, offset));
  if (!offset) locals.strobe = data & 0x0f;
}

/* Contact node is +5V open / 0V closed, and each of the four data bits goes
   through a pair of 2N4291s on the way to DIA (the System III Operation and Maintenance manual §5.7, which does not give the resulting polarity
   at the DIA pin). The ROM settles it: a closed contact reads 1. Only then
   is the resting machine all-zero, does ball home let the start button fire
   the ball-return coil, and do playfield contacts score.

   Strobe 10 is the power-play sense, not a contact group, and is unaffected. */
static READ_HANDLER(sw_r) {
  UINT8 value;
  if (offset) return 0x0f;
  if (locals.strobe == 10) value = locals.coilSense;
  else if (locals.strobe > 9) value = 0x0f;
  else value = coreGlobals.swMatrix[locals.strobe + 1] & 0x0f;
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
  MDRV_DIPS(8)  /* Recel has no physical DIP switches; used purely as an inport allocator */
  MDRV_SWITCH_UPDATE(RECEL)
  MDRV_SWITCH_CONV(recel_sw2m, recel_m2sw)
  MDRV_IMPORT_FROM(recel_snd)
MACHINE_DRIVER_END
