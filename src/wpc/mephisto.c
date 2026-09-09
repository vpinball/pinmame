// license:BSD-3-Clause

#include "driver.h"
#include "core.h"
#include "cpu/i86/i88intf.h"
#include "cpu/i8051/i8051.h"
#include "machine/i8256.h"
#include "machine/i8155.h"


/* Phase 0 tracing: set to 1 to log every I/O access with its decoded meaning. */
#define CIRSA_VERBOSE 0

static struct {
  int   lampCol;        /* IC20 PA4-6 -> IC29 (7445) -> lamp columns LC0-LC7 */
  int   lampSel;        /* a lamp column has just been selected and the row
                           byte for it has not arrived yet -- see ic20_pb_w */
  int   lampPrev;       /* last lamp column that actually received row data,
                           so the sweep wrap can be spotted in ic20_pb_w */
  int   swCol;          /* IC20 PA0-3 -> IC30 (7445) -> switch columns CC0-CC9 */
  UINT8 shiftFrame[8];  /* one pass through the 4094 display chain, 6 or 8
                           bytes long -- see cirsa_frameLen() */
  int   shiftPos;
  UINT8 lastKeys;       /* previous cabinet key state, for edge-only updates */
  UINT8 lastPlayKeys;   /* coin 1/2/3 + start, previous state */
  UINT8 lastTrough;     /* ball trough toggle, previous state */
  UINT8 troughPending;  /* seed the trough on the first frame -- see MACHINE_INIT */
  UINT8 lastFlipBut;    /* flipper buttons, previous state, as the two
                           CORE_SW*FLIPBUTBIT bits of swMatrix[11] */
  int   ppcero;         /* PPCERO, the mains zero-cross pulse: MUART P11,
                           and (through IC26/IC24) the MUART's EXTINT pin */
  /*-- phase 0 instrumentation state --*/
  int   lastrep;
  char  lastline[192];
  UINT8 qcState;        /* live quick-contact inputs, swMatrix[12] bits 0-7 */
  UINT8 qcLatch;        /* IC11 (74LS373) held value, read back on IC9 PB */
  int   qcTransparent;  /* IC9 PC5 high -> latch follows input */
  UINT8 sndToSnd;       /* last byte the MUART sent, latched for the 8051 */
  UINT8 p2Out;          /* last value written to MUART port 2, for edge detect */
  UINT8 inhLF;          /* P25 INH. LUCES FIJAS      -- 1 = fixed lights off  */
  UINT8 inhFlip;        /* P26 INH. FLIPPER          -- 1 = flipper power off */
  UINT8 inhLC;          /* P27 INH. LUCES CONTROLADAS-- 1 = lamp matrix off   */
  UINT8 muartP1Out;     /* MUART port 1 output latch; bit 6 is the 8051's P3.2 */
  UINT8 sndP1;          /* 8051 port 1 output latch = the AY-3-8910 data bus */
  UINT8 sndP3;          /* 8051 port 3 output latch; bits 4/5 = BDIR/BC1 */
  UINT8 sndP2;          /* 8051 port 2 output latch = XRAM address bits 8-15 */
  UINT8 ayPortA;        /* AY-3-8910 IOA output latch -- the audio board's own
                           column strobe and pulsed outputs, see ay8910_porta_w */
} locals;

/* I/O tracing, observation only: every handler still returns what it returned
   before.  Identical consecutive lines are collapsed, otherwise the watchdog
   kick (a single XOR on MUART P1.4) buries everything else. */
#if CIRSA_VERBOSE
static void iolog(const char *msg) {
  if (!strcmp(msg, locals.lastline)) { locals.lastrep++; return; }
  if (locals.lastrep) {
    logerror("        ... previous line repeated %d more time(s)\n", locals.lastrep);
    locals.lastrep = 0;
  }
  strncpy(locals.lastline, msg, sizeof(locals.lastline)-1);
  locals.lastline[sizeof(locals.lastline)-1] = '\0';
  logerror("%s", msg);
}

static const char *muart_regname(int reg) {
  static const char * const n[16] = {
    "CMD1", "CMD2", "CMD3", "MODE", "PORT1C", "SETINT", "RSTINT/INTADR",
    "BUFFER", "PORT1", "PORT2", "TIMER1", "TIMER2", "TIMER3", "TIMER4",
    "TIMER5", "STATUS/MODIF" };
  return n[reg & 15];
}

/* levels per the 8256AH datasheet; L1 and L3/L6/L7 depend on CMD1/MODE bits */
static void muart_levels(char *buf, int mask) {
  static const char * const src[8] = {
    "L0=Tmr1", "L1=Tmr2/P17", "L2=EXTINT", "L3=Tmr3", "L4=RX", "L5=TX",
    "L6=Tmr4", "L7=Tmr5" };
  int i; buf[0] = '\0';
  for (i = 0; i < 8; i++)
    if (mask & (1 << i)) { if (buf[0]) strcat(buf, ","); strcat(buf, src[i]); }
  if (!buf[0]) strcpy(buf, "none");
}

static void muart_decode(char *buf, int reg, int data) {
  char tmp[128];
  buf[0] = '\0';
  switch (reg) {
    case 0: /* CMD1 */
      sprintf(buf, "FRQ=%d(%s) 8086=%d BITI=%d(L1=%s) BRKI=%d stop=%d len=%d",
              data & 1, (data & 1) ? "1kHz" : "16kHz",
              (data >> 1) & 1, (data >> 2) & 1,
              ((data >> 2) & 1) ? "P17 edge" : "Timer2",
              (data >> 3) & 1, (data >> 4) & 3, 8 - ((data >> 6) & 3));
      break;
    case 1: /* CMD2 */
      sprintf(buf, "baud=%d clkdiv=%d parity=%s",
              data & 15, (data >> 4) & 3,
              (data & 0x80) ? ((data & 0x40) ? "even" : "odd") : "none");
      break;
    case 2: /* CMD3 -- set/reset register */
      sprintf(buf, "%s:", (data & 0x80) ? "SET" : "RESET");
      if (data & 0x40) strcat(buf, " RxE");
      if (data & 0x20) strcat(buf, " IAE");
      if (data & 0x10) strcat(buf, " NIE");
      if (data & 0x08) strcat(buf, " END(EOI)");
      if (data & 0x04) strcat(buf, " SBRK");
      if (data & 0x02) strcat(buf, " TBRK");
      if (data & 0x01) strcat(buf, " RST");
      break;
    case 3: /* MODE */
      sprintf(buf, "T35=%d T24=%d T5C=%d CT3=%d CT2=%d P2C=%d",
              (data >> 7) & 1, (data >> 6) & 1, (data >> 5) & 1,
              (data >> 4) & 1, (data >> 3) & 1, data & 7);
      break;
    case 4: /* PORT1C -- 1 = output */
      sprintf(buf, "P1 dir out=%02x in=%02x", data, (~data) & 0xff);
      break;
    case 5: muart_levels(tmp, data); sprintf(buf, "enable %s", tmp); break;
    case 6: muart_levels(tmp, data); sprintf(buf, "disable %s", tmp); break;
    default: break;
  }
}

#endif  /* CIRSA_VERBOSE */

static WRITE_HANDLER(ic4_w) {
#if CIRSA_VERBOSE
  char msg[192], dec[128];
  int reg, mode8086 = i8256_is_8086_mode();
  if (mode8086 && (offset & 1)) {
    sprintf(msg, "IC4  8256 W off=%02x  IGNORED (odd offset in 8086 mode)  PC=%05x\n",
            offset, activecpu_get_pc());
    iolog(msg);
  } else {
    reg = mode8086 ? ((offset >> 1) & 15) : (offset & 15);
    muart_decode(dec, reg, data);
    sprintf(msg, "IC4  8256 W off=%02x reg%-2d %-13s = %02x  %s%s  PC=%05x\n",
            offset, reg, muart_regname(reg), data,
            mode8086 ? "" : "[8085 mode] ", dec, activecpu_get_pc());
    iolog(msg);
  }
#endif
  i8256_w(offset, data);
}

static READ_HANDLER(ic4_r) {
  int val = i8256_r(offset);
#if CIRSA_VERBOSE
  {
    char msg[192];
    int mode8086 = i8256_is_8086_mode();
    int reg = mode8086 ? ((offset >> 1) & 15) : (offset & 15);
    if (!(mode8086 && (offset & 1))) {
      sprintf(msg, "IC4  8256 R off=%02x reg%-2d %-13s -> %02x  PC=%05x\n",
              offset, reg, muart_regname(reg), val, activecpu_get_pc());
      iolog(msg);
    }
  }
#endif
  return val;
}

#if CIRSA_VERBOSE
static const char *i8155_regname(int reg) {
  static const char * const n[8] = {
    "CMD/STATUS", "PA", "PB", "PC", "TIMER_LO", "TIMER_HI", "reg6", "reg7" };
  return n[reg & 7];
}

static void i8155_decode(char *buf, int reg, int data) {
  static const char * const tm[4] = { "NOP", "STOP", "STOP-AT-TC", "START" };
  /* Command bits 3-2 are PC2,PC1 and the four modes are NOT in numeric
     order: 00 = ALT1 (all input), 01 = ALT3, 10 = ALT4, 11 = ALT2 (all
     output) -- Intel's table, and what i8155.c's own decode implements. */
  static const char * const pc[4] = { "1", "3", "4", "2" };
  buf[0] = '\0';
  if (reg == 0)
    sprintf(buf, "PA=%s PB=%s PC=ALT%s intA=%d intB=%d timer=%s",
            (data & 1) ? "out" : "in", (data & 2) ? "out" : "in",
            pc[(data >> 2) & 3], (data >> 4) & 1, (data >> 5) & 1,
            tm[(data >> 6) & 3]);
  else if (reg == 5)
    sprintf(buf, "count_hi=%d mode=%d", data & 0x3f, (data >> 6) & 3);
}

static void log8155(const char *chip, int rw, int offset, int data) {
  char msg[192], dec[128];
  int reg = offset & 7;
  i8155_decode(dec, reg, data);
  if (rw)
    sprintf(msg, "%-4s 8155 W off=%x  %-10s = %02x  %s  PC=%05x\n",
            chip, offset, i8155_regname(reg), data, dec, activecpu_get_pc());
  else
    sprintf(msg, "%-4s 8155 R off=%x  %-10s       PC=%05x\n",
            chip, offset, i8155_regname(reg), activecpu_get_pc());
  iolog(msg);
}

#endif  /* CIRSA_VERBOSE */

#define I8155_IC9   0
#define I8155_IC20  1

static WRITE_HANDLER(ic20_w) {
#if CIRSA_VERBOSE
  log8155("IC20", 1, offset, data);
#endif
  i8155_w(I8155_IC20, offset, data);
}
static READ_HANDLER(ic20_r) {
  UINT8 val = i8155_r(I8155_IC20, offset);
#if CIRSA_VERBOSE
  log8155("IC20", 0, offset, val);
#endif
  return val;
}
static WRITE_HANDLER(ic9_w) {
#if CIRSA_VERBOSE
  log8155("IC9", 1, offset, data);
#endif
  i8155_w(I8155_IC9, offset, data);
}
static READ_HANDLER(ic9_r) {
  UINT8 val = i8155_r(I8155_IC9, offset);
#if CIRSA_VERBOSE
  log8155("IC9", 0, offset, val);
#endif
  return val;
}

/*-- Display: the 74LS165 at 0x2E000 feeds the 4094 chain -----------------
/  The CPU writes a byte to IC2 (74165); hardware shifts it out on QH,
/  clocked by CLK SHT (IC9's TIMER OUT via IC13), into the daisy-chained
/  4094s on the display boards.  One frame = one byte per 4094, and the two
/  games do not have the same number of them: 8 bytes on Sport 2000 (7
/  segment groups + the column select), 6 on Mephisto (5 + column).  Selected
/  per game by hw.gameSpecific1, same as the column-mask table below.
/
/  The last byte of a frame is the digit column select -- active low, exactly
/  one bit clear, walking across the seven TIP116 digit drivers; the rest is
/  segment data.  The first byte shifted in travels furthest down the chain,
/  so the last byte written sits in the 4094 nearest the CPU.
/
/  Which byte feeds which display group is set out in cirsa_shift_frame.
/  What is NOT established for either game: which physical row is upper or
/  lower on the cabinet.  A wrong row order still renders readable text, just
/  in the wrong place -- do not guess it, it needs a live display test.
/----------------------------------------------------------------------*/
/* The column-select mask table is per ROM set, and each game's own display
   routine reads it from its own address:

     Sport 2000        ROM 0xB730: FF FD FB F7 7F BF EF DF
                       col 1-7 -> bits 1,2,3,7,6,4,5 (bit 0 never used)
     Mephisto rev 1.2  ROM 0x0F82: FF FE FD FB F7 BF DF EF
                       col 1-7 -> bits 0,1,2,3,6,5,4 (bit 7 never used)

   mephist1 carries the identical table at 0x0D61, so it shares Mephisto's.
   The two tables below are those inverted: indexed by the cleared bit, giving
   the 1-7 ROM column, with 0 meaning "no column". */
static const UINT8 colFromBitSport2k[8]  = { 0, 1, 2, 3, 6, 7, 5, 4 };
static const UINT8 colFromBitMephisto[8] = { 1, 2, 3, 4, 7, 6, 5, 0 };

/* Bytes per pass through the chain -- one per 4094 on the display boards. */
static int cirsa_frameLen(void) {
  return core_gameData->hw.gameSpecific1 ? 6 : 8;
}

/*-- Segment bit-order remap ----------------------------------------------
/  The ROM's own segment bit order -- both the 7-segment font at 0xC143 and
/  the two halves of the 16-segment font at 0xBA08 -- is NOT PinMAME's, so
/  written raw it produced correct character data drawn with the wrong
/  strokes.  These tables translate on write, so coreGlobals.segments[] holds
/  PinMAME-order values throughout.
/
/  16-segment: the 16 ROM bits carry 14 independent strokes plus two
/  hard-wired duplicates -- A2 (low bit 2) mirrors A1 (high bit 3), D2 (high
/  bit 0) mirrors D1 (high bit 4), matching the manual's Plate 15 pin pairs.
/  Both members of a pair map to the same PinMAME bit, a harmless redundant
/  OR.  G (high bit 6) and R (low bit 7) are the two halves of the middle bar
/  -- Plate 15's pin list leaves only those two for it once the 8 outer-ring
/  and 6 inner-cross pins are accounted for -- and stay two independent
/  single-bit mappings: forcing G to also set PinMAME's bit 11 (R's target)
/  gives 'F' a right-hand nub PinMAME's own font lacks.
/
/  Two known font mismatches are NOT remap bugs -- do not chase them by
/  editing these tables.  The ROM's 'E' lights a full-width middle bar where
/  PinMAME's Rockwell-derived font draws a half-width one, and the ROM's 'K'
/  lights no outer-ring stroke at all; no permutation of the low byte could
/  create or hide an outer-ring segment.
/---------------------------------------------------------------------*/

/* Numeric font bit -> core_bcd2seg7 bit.  ROM order is a=3 b=7 c=5 d=4 e=1
   f=2 g=6 dp=0; core_bcd2seg7 (core.c:137) is the classic a=0 b=1 c=2 d=3
   e=4 f=5 g=6, with no dp bit. */
static const UINT8 cirsa_seg8dBit[8] = {
  0,      /* 0: dp -- dropped.  CORE_SEG8D has a period (bit 7), but the
             digit font never sets ROM bit 0 and in the alphanumeric high
             byte that bit is D2, not a dot */
  1 << 4, /* 1: e */
  1 << 5, /* 2: f */
  1 << 0, /* 3: a */
  1 << 3, /* 4: d */
  1 << 2, /* 5: c */
  1 << 6, /* 6: g */
  1 << 1, /* 7: b */
};

static UINT8 cirsa_seg8d(UINT8 v) {
  UINT8 out = 0;
  int b;
  for (b = 0; b < 8; b++)
    if (v & (1 << b)) out |= cirsa_seg8dBit[b];
  return out;
}

/* Alphanumeric font, high byte bit -> CORE_SEG16N bit.  Bits 1-7 are the same
   a-g outline as the numeric font, in the same positions; bit 0 is D2, the
   mirrored twin of D1 (bit 4) -- see the block comment above. */
static const UINT16 cirsa_seg16HiBit[8] = {
  1 << 3,  /* 0: D2 -> d  (mirrors D1) */
  1 << 4,  /* 1: E  -> e */
  1 << 5,  /* 2: F  -> f */
  1 << 0,  /* 3: A1 -> a */
  1 << 3,  /* 4: D1 -> d */
  1 << 2,  /* 5: C  -> c */
  1 << 6,  /* 6: G  -> g */
  1 << 1,  /* 7: B  -> b */
};

/* Alphanumeric font, low byte bit -> CORE_SEG16N bit.  Bit 2 is A2, the
   mirrored twin of A1 (hi bit 3); the rest are the six-stroke internal
   cross plus R (bit 7, settled -- see the block comment above). */
static const UINT16 cirsa_seg16LoBit[8] = {
  1 << 12, /* 0: M */
  1 << 14, /* 1: K */
  1 << 0,  /* 2: A2 -> a  (mirrors A1) */
  1 << 8,  /* 3: H */
  1 << 13, /* 4: L */
  1 << 10, /* 5: J */
  1 << 9,  /* 6: I */
  1 << 11, /* 7: R  -- settled, see block comment above */
};

static UINT16 cirsa_seg16(UINT8 lo, UINT8 hi) {
  UINT16 out = 0;
  int b;
  for (b = 0; b < 8; b++) {
    if (hi & (1 << b)) out |= cirsa_seg16HiBit[b];
    if (lo & (1 << b)) out |= cirsa_seg16LoBit[b];
  }
  return out;
}

static void cirsa_shift_frame(const UINT8 *f, int len) {
  const UINT8 *colFromBit = core_gameData->hw.gameSpecific1
                              ? colFromBitMephisto : colFromBitSport2k;
  UINT8 sel = (UINT8)~f[len - 1];
  int col, bit;

  /* Exactly one of the seven digit drivers is on at a time, so the column
     byte must have exactly one clear bit.  0xFF (nothing selected) and
     anything with two or more clear bits are both rejected: a byte that is
     not a valid column select means the frame is not a frame, and a blank
     display is a far better symptom of that than a plausible digit. */
  if (!sel || (sel & (sel - 1))) return;
  for (bit = 0; !(sel & (1 << bit)); bit++) ;
  if (colFromBit[bit] == 0) return;               /* bit unused as a column */
  col = colFromBit[bit] - 1;                      /* 0..6 digit position */

  /* Which shift-frame byte feeds which display group.  Sport 2000's f[0]/f[1]
     and f[3]/f[4] are low/high byte PAIRS driving the two 7-character
     LA8041R-11B alphanumeric rows, not four separate 7-digit displays:

       Sport 2000 (7 groups + col)        Mephisto (5 groups + col)
         f[3] lo / f[4] hi -> seg[0..6]     f[0] -> seg[0..6]   (Player 1)
         f[0] lo / f[1] hi -> seg[7..13]    f[1] -> seg[7..13]  (Player 2)
         f[5]              -> seg[14..20]   f[2] -> seg[14..20] (Player 3)
         f[2]              -> seg[21..27]   f[3] -> seg[21..27] (Player 4)
         f[6] cols 0-4     -> seg[28..32]   f[4] cols 0-4 -> seg[28..32]

     Mephisto's chain runs the credit board first and then players 4..1, which
     is what puts credit/column last in its frame.  Its f[4] column slice is
     carried over from Sport 2000's f[6] and is NOT independently established;
     Sport 2000's is (writers traced to offsets 42-46, none for 5-6). */
  if (core_gameData->hw.gameSpecific1) {
    coreGlobals.segments[0 * 7 + col].w = cirsa_seg8d(f[0]);
    coreGlobals.segments[1 * 7 + col].w = cirsa_seg8d(f[1]);
    coreGlobals.segments[2 * 7 + col].w = cirsa_seg8d(f[2]);
    coreGlobals.segments[3 * 7 + col].w = cirsa_seg8d(f[3]);
    if (col < 5) coreGlobals.segments[28 + col].w = cirsa_seg8d(f[4]);  /* credit/match/EB, cols 0-4 */
    return;
  }

  coreGlobals.segments[0 * 7 + col].w = cirsa_seg16(f[3], f[4]);
  coreGlobals.segments[1 * 7 + col].w = cirsa_seg16(f[0], f[1]);
  coreGlobals.segments[2 * 7 + col].w = cirsa_seg8d(f[5]);
  coreGlobals.segments[3 * 7 + col].w = cirsa_seg8d(f[2]);
  if (col < 5) coreGlobals.segments[28 + col].w = cirsa_seg8d(f[6]);  /* credit/match/EB, cols 0-4 */
}

static WRITE_HANDLER(shift_w) {
  int len = cirsa_frameLen();
  locals.shiftFrame[locals.shiftPos++] = data;
  if (locals.shiftPos == len) {
    locals.shiftPos = 0;
    cirsa_shift_frame(locals.shiftFrame, len);
  }
#if CIRSA_VERBOSE
  {
    char msg[192];
    sprintf(msg, "SHIFT W off=%x = %02x  PC=%05x\n", offset, data, activecpu_get_pc());
    iolog(msg);
  }
#endif
}

/* Read side of the same address (74LS165 parallel load).  The ROM has one
   such read at 0x6A88, on a path it does not currently take. */
static READ_HANDLER(shift_r) {
#if CIRSA_VERBOSE
  char msg[192];
  sprintf(msg, "SHIFT R off=%x  (74LS165)  PC=%05x\n", offset, activecpu_get_pc());
  iolog(msg);
#endif
  return 0;
}

static WRITE_HANDLER(diag_w) {
#if CIRSA_VERBOSE
  char msg[192];
  sprintf(msg, "DIAG W off=%x = %02x  PC=%05x\n", offset, data, activecpu_get_pc());
  iolog(msg);
#endif
}

static MEMORY_WRITE_START(mephisto_writemem)
  {0x10000,0x107ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x12000,0x1201f, ic4_w},
  {0x13000,0x130ff, MWA_RAM},
  {0x13800,0x13807, ic20_w},
  {0x14000,0x140ff, MWA_RAM},
  {0x14800,0x14807, ic9_w},
  {0x16000,0x16000, shift_w},
  {0x17000,0x17001, diag_w},
MEMORY_END

static MEMORY_READ_START(mephisto_readmem)
  {0x00000,0x0ffff, MRA_ROM},
  {0x10000,0x107ff, MRA_RAM},
  {0x12000,0x1201f, ic4_r},
  {0x13000,0x130ff, MRA_RAM},
  {0x13800,0x13807, ic20_r},
  {0x14000,0x140ff, MRA_RAM},
  {0x14800,0x14807, ic9_r},
  {0x16000,0x16000, shift_r},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(cirsa_writemem)
  {0x20000,0x21fff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x2a000,0x2a01f, ic4_w},
  {0x2b000,0x2b0ff, MWA_RAM},
  {0x2b800,0x2b807, ic20_w},
  {0x2c000,0x2c0ff, MWA_RAM},
  {0x2c800,0x2c807, ic9_w},
  {0x2e000,0x2e000, shift_w},
  {0x2f000,0x2f000, diag_w},
MEMORY_END

static MEMORY_READ_START(cirsa_readmem)
  {0x00000,0x0ffff, MRA_ROM},
  {0x20000,0x21fff, MRA_RAM},
  {0x2a000,0x2a01f, ic4_r},
  {0x2b000,0x2b0ff, MRA_RAM},
  {0x2b800,0x2b807, ic20_r},
  {0x2c000,0x2c0ff, MRA_RAM},
  {0x2c800,0x2c807, ic9_r},
  {0x2e000,0x2e000, shift_r},
  {0xf8000,0xfffff, MRA_ROM},
MEMORY_END

/*-- 8256 MUART wiring --*/
static void cirsa_muart_int(int state) {
  cpu_set_irq_line(0, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

/* The MUART answers the acknowledge cycle with its own vector (40H + level in
   8086 mode), which is why the whole machine is dead without it: the ROM's
   interrupt table only has real handlers at types 40H..47H. */
static int cirsa_irq_callback(int irqline) {
  return i8256_inta();
}

/*-- PPCERO, and what actually drives EXTINT -----------------------------
/  Same circuit on both boards (Sport 2000 plate 5, Mephisto plate 4): the
/  LM393 IC25 comparator's open-collector output goes to MUART Port 1 pin 38
/  -- P11, PPCERO, "paso por cero", the mains zero crossing -- and also into
/  74HC02 IC26 / 74LS14 IC24 -> 8256 pin 16 EXTINT, together with MUART P12
/  (an output).  So EXTINT = PPCERO OR NOT(P12).  It is NOT the quick-contact
/  line, which an earlier reading of this board claimed.
/
/  The NOT(P12) half is deliberately not modelled: both ROMs clear P12 before
/  their first sti and never set it again, so on hardware that term holds
/  EXTINT asserted for the whole run - and i8256.c requests only on a 0->1
/  transition, so a permanently asserted level would give exactly one level-2
/  interrupt per session.  Driving the pin from PPCERO reproduces the rate the
/  ROM is written around and leaves i8256.c's shared semantics alone.  Revisit
/  here if it ever grows a true level-sensitive EXTINT that re-requests on EOI.
/
/  100 Hz is 50 Hz Spanish mains, full-wave rectified.
/
/  PPCERO IS A NARROW HIGH PULSE, NOT A SQUARE WAVE -- do not "fix" it to 50%
/  duty.  R18/R19 divide 5 V to 0.877 V on the comparator's non-inverting
/  input, so the output is released high only in the sliver either side of the
/  crossing where the unsmoothed rail falls below that, ~0.33 ms of every 10.
/  CIRSA_ZC_PULSE_US is that sliver, and it gates Sport 2000's whole coil
/  pipeline: the four-phase round robin at 0x08B3 has its phase counter [0x31]
/  forced back to 0 by every pass that finds P11 high (0x08AA, 0x090C and the
/  level-2 ISR at 0x095A), so reaching phase 3 - 0xC3CA, the only writer of the
/  coil-frame bytes - needs ~8.3 ms of unbroken P11-low.  50 us to 1.5 ms of
/  high time all behave identically; 3 ms degrades, and the 5 ms this code once
/  emitted gives zero coils, ever.  If coils go quiet, check CIRSA_ZC_PULSE_US
/  and [0x31] before touching the coil bus.
/
/  Mephisto needs the pulse for the opposite reason: its level-2 ISR (0x143B)
/  updates INH LF / INH FLIP / INH L.C. only when it finds P11 HIGH, so resting
/  the pin low would freeze those outputs.  Its coils are not gated on P11 at
/  all, which is why the square wave was a Sport 2000-only bug.
/----------------------------------------------------------------------*/
#define CIRSA_ZC_HZ 100          /* mains zero crossings per second */
#define CIRSA_ZC_PULSE_US 300    /* PPCERO high time per crossing, see above */

static void cirsa_zc_off(int dummy) {
  locals.ppcero = 0;
  i8256_set_extint(0);
}

static void cirsa_zc_tick(int dummy) {
  locals.ppcero = 1;
  i8256_set_extint(1);
  timer_set(TIME_IN_USEC(CIRSA_ZC_PULSE_US), 0, cirsa_zc_off);
}

static UINT8 cirsa_p1_in(void) {
  /* Port 1 inputs (PORT1C = 5C leaves P10, P11, P15 and P17 as inputs):

       P10 S.C.MAT  the lamp-matrix fault sense.  Must rest HIGH = healthy:
                    each ROM samples it in its lamp scan and condemns a
                    column after 6-7 consecutive lows (sport2k 0x0B080,
                    mephisto 0x00FAD).  With the old `return 0` stub every
                    lamp failed -- Mephisto replayed FALLO LUCES n. 00..63
                    through its whole boot and Sport 2000's matrix went dark
                    a few seconds into attract.
       P11 PPCERO   mains zero crossing, see the block comment above.
       P15          the sound board's BUSY line.  It arrives through
                    i8256_set_p1_pin(5,...) from cirsa_sndp3_w, not from
                    here, so this callback must leave it alone --
                    i8256_port1_read ORs the two sources.
       P17 F.T.     "falta de tension", power failure.  Must rest LOW:
                    CMD1.BITI routes it to interrupt level 1 and a low-to-
                    high edge is what signals the fault, so a pin idling
                    high reads as a permanent power failure and the ROM
                    restarts. */
  return (UINT8)(0x01 | (locals.ppcero ? 0x02 : 0x00));
}

static void cirsa_p1_out(UINT8 data) {
  /* P14 = CL-WD (watchdog kick), P12/P13 not used.

     P16 is the sound link's byte handshake, and the sound board cannot run
     without it: it goes to the 8051's P3.2 (INT0), and the firmware's byte
     transmitter spins on `JNB P3.2` (sport2k 0x0253, Mephisto 0x0227) until
     the main CPU drives it high.  JNB reads the pin, not the latch, so that
     lands in cirsa_sndp3_r below.  The main ROM drives it per byte rather
     than as a level: 0x08EE raises it, 0x0915 lowers it, and ISR_SerialRX
     clears it at 0xC9A1 on every byte received.  i8256_port1_write() hands
     us the value already masked by PORT1C (0x5C, so P16 is an output). */
  locals.muartP1Out = data;
}

/*-- MUART Port 2 (plate 5) ---------------------------------------------
/  P20 EG1, P21 EG2, P22 TEST, P23 AVANCE are inputs, buffered through
/  74HC240s with 10K pull-ups, so a pressed button reads as 1.  P24 RST ASIN
/  (sound board reset), P25 INH LF, P26 INH FLIP, P27 INH L.C. are outputs --
/  the bits the ROM sets and clears around 0x0A29, not display strobes as
/  first assumed.
/
/  POLARITY: LOW enables, HIGH inhibits, and nothing inverts.  Each line
/  crosses exactly ONE buffer on the control board (a 7407: open collector,
/  non-inverting, no output bubble) and then drives either a PNP emitter
/  follower or an opto LED pulled up on the far side at the power supply, so
/  current - and the triac switching that transformer secondary - flows only
/  while the CPU pulls the line LOW:
/      P25/P26/P27 LOW  = supply present (NOT inhibited)
/      P25/P26/P27 HIGH = supply removed (inhibited)
/  The Mephisto manual agrees from the operator's side (p.9): the supply's own
/  LEDs sit in series with these lines, and DL15 "stays off, since it
/  corresponds to the flipper inhibit and therefore lights only when the
/  machine enters Game".
/
/  INH BOB is NOT a port-2 bit - Sport 2000 rides it in the coil frame
/  instead, bit 6 of [0x673], set and cleared at 0x0A53/0x0A5A.
/
/  LF is luces fijas, the 6.3/7 VAC general illumination; L.C. is luces
/  controladas, the V LUCES rail feeding the eight BDX34C lamp-COLUMN drivers
/  on plate 6.  Do not confuse either with the control board's own LC0-LC7 /
/  LF0-LF7, the lamp matrix's column and row lines - an unlucky collision.
/----------------------------------------------------------------------*/
static void cirsa_p2_out(UINT8 data) {
  /* P24 = RST ASIN, the sound board's reset line.  Both manuals draw the same
     circuit at the audio-board end: T1, a BC237 emitter follower, straight to
     the 8051's RST pin with no inverter anywhere in the path, and the 8051's
     RST is active high -- so RST ASIN HIGH holds the sound CPU in reset and
     LOW releases it.  R19 3K3 pulls it up, so the board sits in reset until
     something drives it low, which is what both ROMs' boot sequence assumes.

     Edge triggered on purpose.  The ROM writes port 2 for the three inhibit
     lines as well, and cpu_set_reset_line() schedules its work through
     timer_set(), so re-asserting on every write would keep re-suspending the
     8051.  scpu is CPU 1: mcpu is added first in MACHINE_DRIVER_START(mephisto). */
  if ((data ^ locals.p2Out) & 0x10)
    cpu_set_reset_line(1, (data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

  /* P25/P26/P27, decoded per the block comment above: 1 = that supply has
     been switched off at the power board. */
  locals.inhLF   = (data & 0x20) ? 1 : 0;
  locals.inhFlip = (data & 0x40) ? 1 : 0;

  /* INH LUCES FIJAS -> the general illumination string.  The one output on
     this machine that is neither a matrix lamp nor a coil, which is exactly
     what coreGlobals.gi[] is for (gts80.c:406 does the same for its tilt
     relay's GI).  MACHINE_INIT declares nGI so neither gi[] nor the PWM
     write is left unfed. */
  coreGlobals.gi[0] = locals.inhLF ? 0 : 8;
  core_write_masked_pwm_output_8b(CORE_MODOUT_GI0, locals.inhLF ? 0 : 1, 0x01);

  /* INH LUCES CONTROLADAS -> V LUCES, the rail the whole lamp matrix hangs
     off.  With it gone no column can be lit however the ROM strobes IC20, so
     blank the matrix the instant the line goes high rather than waiting for
     the current sweep to finish -- both games' tilt handlers exercise this. */
  if (locals.inhLC != ((data & 0x80) ? 1 : 0)) {
    locals.inhLC = (data & 0x80) ? 1 : 0;
    if (locals.inhLC) {
      memset((void *)coreGlobals.lampMatrix,    0, sizeof(coreGlobals.lampMatrix));
      memset((void *)coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
    }
  }
  locals.p2Out = data;
}

static UINT8 cirsa_p2_in(void) {
  UINT8 ded = coreGlobals.swMatrix[0];
  UINT8 v = 0;
  if (ded & 0x01) v |= 0x04;      /* TEST    -> P22 */
  if (ded & 0x02) v |= 0x08;      /* AVANCE  -> P23 */
  if (ded & 0x04) v |= 0x01;      /* EG1     -> P20 */
  if (ded & 0x08) v |= 0x02;      /* EG2     -> P21 */
  return v;
}

/*-- The MUART <-> 8051 serial link (plate 5, J23) -----------------------
/  Byte level, not bit level: the 8256's TxD callback hands us a byte, the
/  8051's RxD callback collects it, and anything the 8051 transmits goes back
/  through i8256_receive(), which raises interrupt level 4 -- ISR_SerialRX at
/  0x0C994, reading the buffer at [0xA00E].  The boot handshake at 0x05BF
/  compares [0x2A00E] against 0xA5; until this link existed the test could not
/  pass and the machine displayed "NO AUDIO".
/
/  i8051.c's I8051_RX_LINE case is the ONLY place serial_rx_callback is ever
/  invoked -- there is no polling path -- so a driver that installs the
/  callback without also asserting the line gets silence: the byte sits in
/  locals.sndToSnd forever.  The case does not look at the line state, so a
/  bare ASSERT_LINE per byte is enough and there is nothing to clear
/  afterwards.  scpu (the 8051) is cpu 1 here.
/----------------------------------------------------------------------*/
static void cirsa_txd_out(UINT8 data) {
  locals.sndToSnd = data;
  cpu_set_irq_line(1, I8051_RX_LINE, ASSERT_LINE);
}

static int cirsa_snd_rx(void) {
  return locals.sndToSnd;
}

static void cirsa_snd_tx(int data) {
  i8256_receive((UINT8)data);
}

/* .clock is filled in per game by MACHINE_INIT(CIRSA) -- see there. */
static I8256interface cirsa_i8256 = {
  cirsa_muart_int,
  cirsa_p1_in, cirsa_p1_out,
  cirsa_p2_in, cirsa_p2_out,
  cirsa_txd_out,
  0
};

/*-- IC20: the lamp and switch matrices (plate 6) ------------------------
/  PA0-3 select a switch column through IC30 (7445), PA4-6 select a lamp
/  column through IC29 (7445), and PA7 strobes the WD LUCES lamp watchdog.
/  PB drives the lamp rows through IC32 (UDN6118-A).  PC reads the switch rows
/  back through IC31, a 74HC14 -- ONE inverting stage, not two.
/
/  THE SWITCH ROWS MUST NOT BE INVERTED HERE, and that is the whole point of
/  IC31.  The six CF row lines rest high through AR9 (4K7 to +5V); IC30 pulls
/  the selected column LOW, a closed switch pulls its row low with it, and the
/  single inversion turns that into a HIGH at Port C.  So a closed switch
/  reads as 1.  The ROM agrees: 0x0CD31 copies the Port C byte into the
/  debounced level array at [0x72B] with no NOT of its own and latches an
/  event on each 0->1 transition, and the cabinet buttons (cirsa_p2_in) and
/  the quick contacts (ic9_pb_r) feed that same debouncer non-inverted.
/
/  Restore the `~` and every column reads 0x3F at rest, i.e. all 60 switches
/  permanently closed: every event moves to the release edge and the coin
/  debouncer sits in its jammed-coin path, so coins credit about 1 time in 12.
/----------------------------------------------------------------------*/
static WRITE_HANDLER(ic20_pa_w) {
  int col = (data >> 4) & 0x07;
  /* A change in PA4-6 re-points IC29, so the next Port B write is the row
     byte for the newly selected column.  ic20_pb_w consumes that flag; the
     sweep-wrap latch lives there too, because Mephisto passes through
     column 0 on its way to every column (see below) and latching here
     would fire eight times a sweep instead of once. */
  if (col != locals.lampCol) locals.lampSel = 1;
  locals.swCol   = data & 0x0f;
  locals.lampCol = col;
}

/*-- IC20 Port B: the lamp rows, and why only one write per column counts --
/  Both ROMs write Port B more than once per lamp column, and only the write
/  that follows the column select carries lamp data.  Sport 2000's lamp-fault
/  test (0xB06B) samples MUART P10 with the rows off and then drives every
/  not-yet-failed row for ~560 cycles -- on a healthy machine that mask is
/  0xFF, so taking it as data made core_setLamp report all 64 lamps
/  permanently on.  The transient is real on the board but it is not a lit
/  lamp: 560 cycles against the 8,220 a genuinely driven row gets in the same
/  slot.  This handler therefore takes the row byte the ROM sets up for the
/  column and ignores the re-writes.  It does NOT special-case 0xFF -- a
/  column legitimately driving all eight rows still reports all eight.
/
/  Mephisto (0x0FDE) writes Port B twice per column and again only the second
/  is data, but its read-modify-write of PA drops the column field to 0 on the
/  way to every column (0x0FF3).  That is why the sweep-wrap latch lives here
/  rather than in ic20_pa_w: a "col == 0" test there fires once per column,
/  clearing tmpLampMatrix eight times a sweep and leaving lampMatrix holding a
/  single column.  lampPrev tracks the last column that actually received
/  data, so the wrap is detected on real sweeps only.
/----------------------------------------------------------------------*/
static WRITE_HANDLER(ic20_pb_w) {
  if (!locals.lampSel) return;   /* a fault-test re-write, not lamp data */
  locals.lampSel = 0;
  /* The lamp columns are strobed 0..7 in order, so accumulate a whole sweep
     and latch it when the sweep wraps.  Latching on the video frame instead
     chops the sweep and drops whichever columns straddle the boundary. */
  if (locals.lampCol == 0 && locals.lampPrev != 0) {
    memcpy((void *)coreGlobals.lampMatrix, (void *)coreGlobals.tmpLampMatrix,
           sizeof(coreGlobals.tmpLampMatrix));
    memset((void *)coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  locals.lampPrev = locals.lampCol;
  /* MUART P27 (INH. LUCES CONTROLADAS) cuts V LUCES, the matrix's own
     supply -- see cirsa_p2_out.  The ROM goes on strobing IC20 while the
     rail is gone, so the gate belongs here, not in the ROM's data. */
  core_setLamp(coreGlobals.tmpLampMatrix, 1 << locals.lampCol,
               locals.inhLC ? 0 : data);
}

static READ_HANDLER(ic20_pc_r) {
  /* IC30 decodes 0-9 only; for 10-15 no column is pulled low, so no switch
     can pull a row low and every buffered row reads back as "open" -- 0
     under this active-high convention. */
  if (locals.swCol > 9) return 0x00;
  return coreGlobals.swMatrix[locals.swCol + 1] & 0x3f;
}

/*-- IC9 Port A: the coil bus (plate 9) -----------------------------------
/  One 8-byte frame carries all 24 coils.  PA0-2 select a position 0-7 shared
/  by the three 74HC259 addressable latches; PA3, PA4 and PA5 are the data bit
/  for J11 (coils 0-7), J12 (8-15) and J13 (16-23) respectively -- three
/  parallel data lines, not a one-hot block select, which is why 24 coils need
/  only eight transfers.  PA7 is the strobe and PA6 a global enable that is
/  only meaningful at position 0.
/
/  Coils are level-held: the ROM re-sends the whole frame about 280 times a
/  second and never issues an "off", so this must be idempotent -- it sets and
/  clears the three bits for the addressed position on every write.
/
/  THE TWO GAMES NUMBER THE POSITIONS IN OPPOSITE DIRECTIONS.  Same wires,
/  same connectors, same frame, but Sport 2000 puts coil N at position N%8 and
/  Mephisto at 7-(N%8), so a decode written for one mislabels 6 of every 8
/  coils on the other.  That is what each ROM does, not an inference from the
/  schematic: Sport 2000's SolenoidOn(N) (0xC7D8, table 0xC81E) sets mask
/  1<<(N%8), while Mephisto's COIL TEST element routine (0x3178, mephist1
/  0x2E01) does `mov al,0x80 / ror al,cl` with cl = N%8.  Both frame builders
/  then emit group-byte bit p at position p.  Verified against both ROMs' own
/  COIL TEST, all 24 coils, both Mephisto revisions.
/
/  The bus transaction is identical for both games, instruction for
/  instruction (Mephisto 0x130D, Sport 2000 0xC277), so both memory maps route
/  their IC9 through this one handler.
/
/  Not modelled: PA6, which the ROM reads back and tests but which never moves
/  a coil.  "Coil 24" (general illumination) is MUART Port 2 bit 7, not on this
/  bus at all.
/----------------------------------------------------------------------*/
static WRITE_HANDLER(ic9_pa_w) {
  const int pos = data & 0x07;
  int blk;

  for (blk = 0; blk < 3; blk++) {
    /* coil number == mask bit; Mephisto counts the position the other way
       round (7-pos), see the block comment. */
    const int coil = blk * 8 +
                     (core_gameData->hw.gameSpecific1 ? (7 - pos) : pos);
    const UINT32 bit = 1u << coil;
    if (data & (0x08 << blk)) coreGlobals.solenoids |=  bit;
    else                      coreGlobals.solenoids &= ~bit;
  }

  /* Coils 1-3 (SORTING RAMP EJECTOR, KICKBACK, BUMPER) are fired by the
     quick contacts in hardware, and the ROM re-derives their frame bits from
     the contact latch (0xC2AB, 0xC2A5, 0xC2B1/0xC2B6/0xC2BD) -- but only
     while a game is running: [0x663] stays constant through every attract
     closure.  The real machine's contact wiring does not care what mode the
     ROM is in, so OR the live contact state back in here.  Contact N's bit is
     coil N-60's bit, by hardware coincidence.

     One-directional on purpose: it can only ever ADD a bit, never block a
     CPU-commanded write.  Coils 0 and 4 (the two ball ejectors) are
     deliberately EXCLUDED -- the ROM drives those two itself on its own timed
     schedule (0xC3CA), and asserting them from raw contact state holds them
     on for as long as the contact is closed, which the ROM never does. */
  if (!core_gameData->hw.gameSpecific1)              /* Sport 2000 only */
    coreGlobals.solenoids |= (locals.qcState & 0x0e); /* coils 1-3 only  */

  /* Feed the PWM integrator with the same 24-bit state, quick-contact OR
     included -- this is what MACHINE_INIT(CIRSA)'s
     core_set_pwm_output_type(CORE_MODOUT_SOL0, 24, ...) is for.  Doing it on
     every hardware write (~280/s) rather than once a vblank is what keeps
     short pulses visible to the integrator; see p2k.c:2013-2018. */
  core_write_pwm_output_8b(CORE_MODOUT_SOL0,      (UINT8)(coreGlobals.solenoids      & 0xff));
  core_write_pwm_output_8b(CORE_MODOUT_SOL0 +  8, (UINT8)((coreGlobals.solenoids >> 8)  & 0xff));
  core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 16, (UINT8)((coreGlobals.solenoids >> 16) & 0xff));
}

/*-- IC9 PB/PC5: the quick-contact return path (plate 5) -----------------
/  The quick contacts (60 RAMP HOLE, 61 SORTING RAMP, 62 KICKBACK, 63 BUMPER,
/  64 BRIDGE ENTRY) fire their coils in hardware -- that is what makes them
/  quick -- and notify the CPU separately, through IC11, a 74LS373.
/  ISR_QuickContacts at 0x0931 drives IC9 PC5 high then low and reads Port B;
/  a '373 is transparent while its latch-enable is high and holds on the
/  falling edge.
/
/  coreGlobals.swMatrix[12] carries the live contact state, and which slot
/  that is is a free driver-side choice -- the contacts arrive over IC9 Port B
/  and never through the IC20 matrix scan.  It must NOT be column 11: that is
/  PinMAME's CORE_FLIPPERSWCOL (core.h:334) and core_updateSw() overwrites its
/  bits 0x02/0x08 from the flipper-button state every frame, which used to
/  stop contacts 61 and 63 reaching the ROM at all.  Column 12 is the first
/  CUSTOM column (CORE_STDSWCOLS == 12) and needs hw.swCol == 1 on both game
/  data blocks so core.c's custom-column loops stay inside CORE_MAXSWCOL.
/----------------------------------------------------------------------*/
static READ_HANDLER(ic9_pb_r) {
  return locals.qcLatch;
}

static WRITE_HANDLER(ic9_pc_w) {
  const int pc5 = (data & 0x20) ? 1 : 0;
  if (pc5) locals.qcLatch = locals.qcState;   /* transparent */
  locals.qcTransparent = pc5;                 /* falling edge freezes it */
}

static i8155_interface cirsa_i8155 = {
  2,                              /* IC9 = chip 0, IC20 = chip 1 */
  {0, 0}, {ic9_pb_r, 0}, {0, ic20_pc_r},
  {ic9_pa_w, ic20_pa_w}, {0, ic20_pb_w}, {ic9_pc_w, 0},
  {0, 0}
};

/* Not declared in any header; core.c defines it at file scope. */
extern int g_fHandleKeyboard;

static READ32_HANDLER(cirsa_eram_addr);   /* defined with the sound ports below */

/* The ball trough: a whole column's worth of switches held closed by the balls.
   Used both by MACHINE_INIT, which seeds them, and by SWITCH_UPDATE's toggle. */
static const UINT8 cirsaTrough[2] = {6, 0x3c};   /* BALL TROUGH 1-4  */
static const UINT8 mephTrough[2]  = {7, 0x0e};   /* elements 38,39,40 */

/*=========================================================================
/  CIRSA_SNDSWEEP -- the switch -> sound sweep harness.  Compiled out
/  entirely unless -DCIRSA_SNDSWEEP, and inert unless CIRSA_SWEEP=1 is set in
/  the environment.
/
/  Once per frame from cirsa_vblank it walks switch elements 0..67 in
/  fixed-length slots, PASSES times over (pass 1 closes nothing and is the
/  control), toggling each element for CLOSE_MS.  Every 8 element slots it
/  inserts a keep-alive slot -- fill the trough, drop a coin if the ROM's own
/  credit counter is low, press START -- so a game is live throughout.  It
/  records every byte the ROM pushes into its own sound-command queue, caught
/  by a write handler on the queue COUNTER so nothing can be enqueued and
/  drained between two samples, plus the ROM's switch level array for the
/  element under test.
/
/  Coins are gated on the credit counter (< 8) because Mephisto discards a
/  coin outright above 200 credits (0x1BDA), and a long run otherwise pins
/  itself at the ceiling with its own keep-alive coins.
/
/  Environment (all optional):
/    CIRSA_SWEEP=1        arm it; without this the binary behaves normally
/    SWEEP_CLOSE_MS=300   how long an element is held
/    SWEEP_COIN_MS=300    how long a coin chute is held in a keep-alive slot
/    SWEEP_SLOT_MS=1700   slot length
/    SWEEP_PASSES=5       passes over the element list
/    SWEEP_BOOT_MS=40000  quiet time before pass 1
/=========================================================================*/
#ifdef CIRSA_SNDSWEEP
#include <stdlib.h>

#define SWEEP_ELEMS   68
#define SWEEP_KAEVERY 8                    /* keep-alive slot every N elements */
#define SWEEP_SLOTS   (SWEEP_ELEMS + (SWEEP_ELEMS + SWEEP_KAEVERY - 1)/SWEEP_KAEVERY)

static struct {
  int   on;
  int   meph;
  int   ramBase;               /* linear address the main CPU's RAM starts at */
  int   bootFr, slotFr, closeFr, coinFr, passes;
  int   frame;
  int   curEl;                 /* element held in this slot, -1 = keep-alive */
  UINT8 curIdx, curMask;
  int   curWasSet;             /* level before we touched it: trough rests closed */
  int   qCount, qBase, qDepth; /* linear addresses of the ROM's sound queue */
  int   credAddr, levelBase;   /* -1 when not established for this revision */
  UINT8 *ram;                  /* generic_nvram, i.e. the main CPU's RAM */
  UINT8 coinIdx, coinMask, startIdx, startMask, trIdx, trMask, trServed;
} swp;

static int sweep_env(const char *k, int dflt) {
  const char *v = getenv(k);
  return v ? atoi(v) : dflt;
}

/* element -> {swMatrix index, bit}.  Sport 2000 numbers col*6+row, Mephisto
   col*6+(5-row) -- both the ROMs' own arithmetic; both read ROM column N out
   of swMatrix[N+1], and elements 60-67 are the quick contacts. */
static void sweep_elem(int el, UINT8 *idx, UINT8 *mask) {
  if (el >= 60) { *idx = 12; *mask = (UINT8)(1 << (el - 60)); return; }
  { int col = el / 6, row = el % 6;
    if (swp.meph) row = 5 - row;
    *idx = (UINT8)(col + 1); *mask = (UINT8)(1 << row); }
}

/* The queue counter's own write handler.  The ROM writes queue[count] and
   only then increments count, so every byte is already in RAM when the
   counter moves -- nothing can be enqueued and drained between samples. */
static WRITE_HANDLER(sweep_qcount_w) {
  int cOff = swp.qCount - swp.ramBase;
  int prev = swp.ram ? swp.ram[cOff] : 0;
  if (swp.ram) swp.ram[cOff] = data;
  /* SoundCmd increments the counter by exactly one per call, so accept only
     that.  The ROM's power-on RAM test walks patterns through this byte too,
     and a jump of 0 -> 0xFF would otherwise read as 255 queued commands. */
  if (swp.on && swp.ram && data == prev + 1 && prev < swp.qDepth)
    printf("SW cmd f=%d el=%d cmd=%02X\n", swp.frame, swp.curEl,
           swp.ram[swp.qBase - swp.ramBase + prev]);
}

static void sweep_init(void) {
  const char *g = Machine->gamedrv->name;
  swp.meph    = core_gameData->hw.gameSpecific1 ? 1 : 0;
  swp.on      = sweep_env("CIRSA_SWEEP", 0);
  swp.ramBase = swp.meph ? 0x10000 : 0x20000;
  if (!strcmp(g, "mephisto")) {
    swp.qCount = 0x10359; swp.qBase = 0x1035A; swp.qDepth = 20;
    swp.credAddr = 0x10356; swp.levelBase = 0x1009E;
  } else if (!strcmp(g, "mephist1")) {
    swp.qCount = 0x10352; swp.qBase = 0x10353; swp.qDepth = 20;
    swp.credAddr = 0x10350; swp.levelBase = -1;
  } else {                                        /* sport2k */
    swp.qCount = 0x206FD; swp.qBase = 0x206FE; swp.qDepth = 9;
    swp.credAddr = -1;    swp.levelBase = 0x2072B;
  }
  if (swp.meph) {
    /* The same cells mephCoinSw/cirsaCoinSw below carry; spelled out here
       because those tables are declared after this block. */
    swp.coinIdx = 6; swp.coinMask = 0x10;      /* chute 0, +1 credit */
    swp.startIdx= 4; swp.startMask= 0x04;      /* START, element 21  */
    swp.trIdx   = mephTrough[0];    swp.trMask   = mephTrough[1];
    swp.trServed= (UINT8)(mephTrough[1] & ~0x02);  /* one ball out of the trough */
  } else {
    swp.coinIdx = 7; swp.coinMask = 0x10;      /* centre chute, +1 credit */
    swp.startIdx= 7; swp.startMask= 0x04;      /* START, element 38       */
    swp.trIdx   = cirsaTrough[0];    swp.trMask   = cirsaTrough[1];
    swp.trServed= cirsaTrough[1];    /* sport2k drains on BALL OUT, not the trough */
  }
  swp.bootFr  = sweep_env("SWEEP_BOOT_MS", 40000) * 60 / 1000;
  swp.slotFr  = sweep_env("SWEEP_SLOT_MS", 1700)  * 60 / 1000;
  swp.closeFr = sweep_env("SWEEP_CLOSE_MS", 300)  * 60 / 1000;
  swp.coinFr  = sweep_env("SWEEP_COIN_MS", 300)   * 60 / 1000;
  swp.passes  = sweep_env("SWEEP_PASSES", 5);
  if (swp.closeFr < 1) swp.closeFr = 1;
  if (swp.coinFr  < 1) swp.coinFr  = 1;
  if (swp.closeFr > swp.slotFr - 6) swp.closeFr = swp.slotFr - 6;
  swp.frame = 0; swp.curEl = -1; swp.ram = NULL;
  if (swp.on) {
    install_mem_write_handler(0, swp.qCount, swp.qCount, sweep_qcount_w);
    printf("SW cfg game=%s meph=%d boot=%d slot=%d close=%d coin=%d passes=%d "
           "slots=%d qcount=%05X\n", g, swp.meph, swp.bootFr, swp.slotFr,
           swp.closeFr, swp.coinFr, swp.passes, SWEEP_SLOTS, swp.qCount);
  }
}

static void sweep_frame(void) {
  int t, pass, sp, fr, el;
  if (!swp.on) return;
  if (!swp.ram) swp.ram = generic_nvram;
  t = swp.frame++;
  if (t < swp.bootFr) {
    if (t == swp.bootFr - 1) printf("SW boot done f=%d\n", t);
    return;
  }
  t -= swp.bootFr;
  pass = t / (SWEEP_SLOTS * swp.slotFr);
  if (pass >= swp.passes) { printf("SW done f=%d\n", swp.frame); swp.on = 0; return; }
  sp = (t / swp.slotFr) % SWEEP_SLOTS;
  fr = t % swp.slotFr;
  el = (sp % (SWEEP_KAEVERY + 1) == 0) ? -1 : sp - sp/(SWEEP_KAEVERY+1) - 1;
  if (el >= SWEEP_ELEMS) el = -1;

  if (fr == 0) {                                   /* slot opens */
    int cred = (swp.credAddr >= 0 && swp.ram) ? swp.ram[swp.credAddr - swp.ramBase]
                                              : -1;
    swp.curEl = el;
    if (el < 0) {                                  /* keep-alive */
      /* Fill the trough first.  Mephisto will not take START with a ball
         missing, and the serve it does take is the trough going FULL ->
         SERVED afterwards -- the same cycle scripts/playtest-deep.sh uses. */
      coreGlobals.swMatrix[swp.trIdx] |= swp.trMask;
      if (cred < 0 || cred < 8)
        coreGlobals.swMatrix[swp.coinIdx] |= swp.coinMask;
    } else if (pass > 0) {                         /* pass 1 closes nothing */
      sweep_elem(el, &swp.curIdx, &swp.curMask);
      swp.curWasSet = (coreGlobals.swMatrix[swp.curIdx] & swp.curMask) ? 1 : 0;
      if (swp.curWasSet) coreGlobals.swMatrix[swp.curIdx] &= ~swp.curMask;
      else               coreGlobals.swMatrix[swp.curIdx] |=  swp.curMask;
    }
    printf("SW slot p=%d s=%d el=%d f=%d cred=%d\n", pass, sp, el, swp.frame, cred);
  }
  if (el < 0) {                                    /* keep-alive timing */
    if (fr == swp.coinFr) coreGlobals.swMatrix[swp.coinIdx]  &= ~swp.coinMask;
    if (fr == 20)         coreGlobals.swMatrix[swp.startIdx] |=  swp.startMask;
    if (fr == 38)         coreGlobals.swMatrix[swp.startIdx] &= ~swp.startMask;
    return;
  }
  if (pass == 0) return;
  if (fr == swp.closeFr - 1 && swp.levelBase >= 0 && swp.ram && el < 60) {
    /* Did the ROM's own switch level array see it?  Sport 2000 [0x72B],
       Mephisto rev 1.2 [0x9E]; one byte per column, bit = the row the
       element's own numbering gives.  Not established for rev 1.1. */
    int col = el / 6, row = swp.meph ? 5 - (el % 6) : (el % 6);
    UINT8 lv = swp.ram[swp.levelBase - swp.ramBase + col];
    printf("SW lvl el=%d seen=%d raw=%02X\n", el, (lv >> row) & 1, lv);
  }
  if (fr == swp.closeFr) {                         /* put it back */
    if (swp.curWasSet) coreGlobals.swMatrix[swp.curIdx] |=  swp.curMask;
    else               coreGlobals.swMatrix[swp.curIdx] &= ~swp.curMask;
  }
}
#endif /* CIRSA_SNDSWEEP */

static MACHINE_INIT(CIRSA) {
  memset(&locals, 0, sizeof(locals));

  /* A cabinet button physically held at power-on is already down when the ROM
     first reads MUART port 2, a few hundred instructions into the boot -- that
     is how the real machine enters its test mode -- so sample the inputs here
     rather than waiting for the first core_updateSw, which arrives a frame too
     late.  Only when the driver itself owns the keyboard: core_updateSw passes
     SWITCH_UPDATE a NULL input port array when g_fHandleKeyboard is clear
     (VPinMAME and libpinmame both clear it, see rfranco.c), and seeding from
     input ports the front end believes it owns would fight it. */
  if (g_fHandleKeyboard) {
    UINT8 keys = (UINT8)((readinputport(CORE_COREINPORT) >> 8) & 0x0f);
    coreGlobals.swMatrix[0] = (UINT8)((coreGlobals.swMatrix[0] & ~0x0f) | keys);
    locals.lastKeys = keys;
  }

  /* Put the balls in the trough, which is where they are on a machine someone
     just switched on.  These switches are a level held closed by the balls,
     not an event, and with them open both ROMs park in BALL WAITING (sport2k)
     or no bola (mephisto) -- a state that is not attract, does not poll the
     service check, and cannot be left by any cabinet button.

     The "Ball Trough" toggle in SWITCH_UPDATE still works: locals.lastTrough
     is 0 after the memset above and so is the port bit, so the first press
     sets what is already set and the second empties the trough. */
  {
    /* How many balls comes from the "Balls" dip that SIM_PORTS declares.  The
       trough switches are ordered so bit 0 of the mask is the first ball, so
       filling n is the low n set bits.  Sport 2000 holds four (elements
       32-35), Mephisto three (38/39/40); the dip offers up to seven, so
       clamp.  Zero is legitimate -- it is how BALL WAITING / no bola is
       reached deliberately. */
    const UINT8 *t = core_gameData->hw.gameSpecific1 ? mephTrough : cirsaTrough;
    UINT8 mask = t[1], seeded = 0;
    int want = 4, i;

    /* The dip cannot be read here.  MACHINE_INIT runs from cpu_run()
       (cpuexec.c:364) BEFORE the first frame, and input_port_value[] is only
       filled by update_input_ports() once a frame from the OSD loop, so
       readinputport() returns 0 at this point and SIM_BALLS(0) is 0 -- which
       seeds nothing at all and parks a bare `xpinmame sport2k` in BALL
       WAITING.  Seed the default here and let the first SWITCH_UPDATE, which
       does get a populated inports[], apply the dip. */
    if (g_fHandleKeyboard) locals.troughPending = 1;

    for (i = 0; i < 8 && want > 0; i++)
      if (mask & (1 << i)) { seeded |= (UINT8)(1 << i); want--; }

    coreGlobals.swMatrix[t[0]] |= seeded;
  }

  coreGlobals.nSolenoids = 24;
  core_set_pwm_output_type(CORE_MODOUT_SOL0, 24, CORE_MODOUT_SOL_2_STATE);

  /* One GI string: the "luces fijas" that MUART P25 switches through the
     power supply's TR4 triac -- 7 VAC on Sport 2000, 6.3 VAC on Mephisto,
     ordinary #44-class bulbs either way.  It starts ON because MUART port 2
     resets to 0x00 and 0 = not inhibited; both ROMs drive all three lines
     high a few hundred instructions later (sport2k 0x06F6). */
  coreGlobals.nGI = 1;
  core_set_pwm_output_type(CORE_MODOUT_GI0, 1, CORE_MODOUT_BULB_44_6_3V_AC);
  coreGlobals.gi[0] = 8;
  core_write_masked_pwm_output_8b(CORE_MODOUT_GI0, 1, 0x01);

  /* IC4's CLK pin (17) sits on the 8284-A's CLK output, the same net that
     clocks the 8088, with no divider between them -- Sport 2000's plate 5
     prints "6MHz" on that wire, Mephisto's plate 4 shows the same connection
     with a 15 MHz crystal, so 8284-A CLK = XTAL/3 = 5 MHz there.  IC17 (PAT
     036 / PAT 032) *receives* CLK on its pin 1 and does not generate it, and
     IC22 (4060) + IC15 (74LS393) are the reset watchdog divider, not a clock
     source.

     Both ROMs leave CMD2's prescaler on divide-by-5 and CMD1.FRQ on the /64
     tap, so the MUART timer base is CLK/320: 18 750 Hz on Sport 2000 and
     15 625 Hz on Mephisto.  It is NOT the 16 kHz i8256.c used to hard-code
     (MAME's i8256_device still does).  Mephisto's board was built to land on
     that nominal 16 kHz; Sport 2000 kept the circuit with a faster crystal
     and runs its MUART 17 % quicker, which is why the two ROMs reload timer 1
     with different counts -- 19 against 22 -- for nearly the same 1.2 ms.

     Sport 2000's 6 MHz has a second, independent derivation from the serial
     link: both ROMs set CMD2 = 0x02 (TxC/32) and both sound 8051s run 12 MHz
     with TL1 = TH1 = 0xFE and no PCON write, i.e. exactly 15 625 baud, and
     the clock reaches MUART pin 22 as PCLK -> 8155 IC20 timer (loaded with 6)
     -> CLKUS.  18 MHz -> PCLK 3.000 MHz -> /6 -> /32 = 15 625 closes exactly.
     It does NOT close on Mephisto (13 021 baud), and that is a genuine open
     question -- probably its net labelled PCLK is the PAT's own CLK output
     (15/5 = 3 MHz) rather than the 8284-A's pin 2.  It does not affect the
     timer base either way. */
  cirsa_i8256.clock = core_gameData->hw.gameSpecific1 ? 5000000  /* mephisto */
                                                      : 6000000; /* sport2k  */
  i8256_init(&cirsa_i8256);
  /* PPCERO: a narrow 100 Hz pulse on MUART P11 and, through IC26/IC24,
     on the MUART's EXTINT pin.  One timer per crossing; the falling edge
     is scheduled by cirsa_zc_tick itself.  See cirsa_p1_in's block
     comment for why the width matters. */
  timer_pulse(TIME_IN_HZ(CIRSA_ZC_HZ), 0, cirsa_zc_tick);
  i8155_init(&cirsa_i8155);
  cpu_set_irq_callback(0, cirsa_irq_callback);

  /* Setup serial line callbacks, needs to be set before CPU reset by
     design -- see nuova.c's uboat65 init for the same warning. */
  i8051_set_serial_tx_callback(cirsa_snd_tx);
  i8051_set_serial_rx_callback(cirsa_snd_rx);
  i8051_set_eram_iaddr_callback(cirsa_eram_addr);
#ifdef CIRSA_SNDSWEEP
  sweep_init();
#endif
}

/* Coin 1/2/3 and Start, as {swMatrix index, bit mask}, measured through each
   ROM's own SWITCH TEST.  Sport 2000 takes all three chutes and Start on one
   column; Mephisto spreads them, and its chute order comes from the ROM's own
   tables at 0x1C49 / 0x1C4C. */
static const UINT8 cirsaCoinSw[4][2] = {
  /* Coin 1 is the centre chute on purpose: the 25 chute takes TWO coins per
     credit, and key 5 has to be the one that visibly does something on a
     single press. */
  {7, 0x10},  /* Coin 1 -> centre chute, 1 credit      */
  {7, 0x08},  /* Coin 2 -> 25 chute, 2 coins = 1 credit */
  {7, 0x20},  /* Coin 3 -> 100 chute, 3 credits        */
  {7, 0x04}   /* Start                                 */
};
static const UINT8 mephCoinSw[4][2] = {
  {6, 0x10},  /* Coin 1 -> chute 0, 1 credit  */
  {4, 0x20},  /* Coin 2 -> chute 1, 2 credits */
  {7, 0x10},  /* Coin 3 -> chute 2, 5 credits */
  {4, 0x04}   /* Start                        */
};

/* The flipper BUTTONS, as {swMatrix index, bit mask} -- Sport 2000 only.
   Left = element 16, right = element 17, i.e. swMatrix[3] bits 4 and 5, both
   measured through the ROM's own SWITCH TEST.  On the real board CONT. MAND.
   forks: to the flipper coil driver, and through D6/R11/Q7/Q4 out to FILA and
   COL. on P38, which is the matrix report.  That fork runs off +12 V, not off
   V FLIP, which is why the ROM keeps seeing the button while INH. FLIPPER is
   asserted.

   MEPHISTO HAS NO SUCH CELLS and deliberately gets no entry here: its layout
   list groups the flipper buttons under "Switches NOT read by the matrix", and
   its own elements 16 and 17 are two scoring targets (handlers 0x4418 and
   0x42D1), so mirroring the keys onto them would pay out for flipping.

   NEITHER game's ROM reads the end-of-stroke switches -- Sport 2000's EOS opto
   does the local power-to-hold winding changeover on the flipper board and
   never reaches the CPU -- so core.c's FLIP_EOS machinery stays switched off
   and neither game declares it. */
static const UINT8 cirsaFlipSw[2][2] = {
  {3, 0x10},  /* left  flipper button -> element 16 */
  {3, 0x20}   /* right flipper button -> element 17 */
};

static SWITCH_UPDATE(CIRSA) {
  const int meph = core_gameData->hw.gameSpecific1;

  /* Write a bit only when the key behind it has actually changed.
     Rewriting the whole row every frame stamps out anything else that set
     one of these switches -- a front end, or the remote debugger -- before
     the ROM has had a chance to poll it.  Same reasoning as rfranco.c. */
  if (inports) {
    const UINT8 (*coin)[2] = meph ? mephCoinSw : cirsaCoinSw;
    const UINT8 *trough    = meph ? mephTrough : cirsaTrough;
    UINT8 keys    = (UINT8)((inports[CORE_COREINPORT] >> 8) & 0x0f);
    UINT8 changed = (UINT8)(keys ^ locals.lastKeys);
    UINT8 now, diff;
    int i;

    if (changed) {
      coreGlobals.swMatrix[0] = (UINT8)((coreGlobals.swMatrix[0] & ~changed) |
                                        (keys & changed));
      locals.lastKeys = keys;
    }

    /* Apply the "Balls" dip on the first frame -- MACHINE_INIT could not read
       it, see the block comment there. */
    if (locals.troughPending) {
      UINT8 mask = trough[1], seeded = 0;
      int want = SIM_BALLS(inports[CORE_SIMINPORT]), i;
      locals.troughPending = 0;
      /* Only ever ADD to what MACHINE_INIT seeded.  The dip's own DIPSET
         default does not reach this port on every front end -- headless,
         inports[CORE_SIMINPORT] reads 0x0010, so SIM_BALLS() is 0 -- and
         letting that overwrite the default is what emptied the trough and
         parked the machine in BALL WAITING. */
      if (want > 0) {
        for (i = 0; i < 8 && want > 0; i++)
          if (mask & (1 << i)) { seeded |= (UINT8)(1 << i); want--; }
        coreGlobals.swMatrix[trough[0]] =
          (UINT8)((coreGlobals.swMatrix[trough[0]] & ~mask) | seeded);
      }
    }

    /* Coin 1/2/3 and Start, same change-only discipline, one bit each. */
    now  = (UINT8)(inports[CORE_COREINPORT] & 0x0f);
    diff = (UINT8)(now ^ locals.lastPlayKeys);
    for (i = 0; i < 4; i++)
      if (diff & (1 << i)) {
        const UINT8 idx = coin[i][0], msk = coin[i][1];
        if (now & (1 << i)) coreGlobals.swMatrix[idx] |=  msk;
        else                coreGlobals.swMatrix[idx] &= ~msk;
      }
    locals.lastPlayKeys = now;

    /* Ball trough -- a toggle, so follow its level rather than its edge. */
    now  = (UINT8)((inports[CORE_COREINPORT] >> 4) & 1);
    if (now != locals.lastTrough) {
      if (now) coreGlobals.swMatrix[trough[0]] |=  trough[1];
      else     coreGlobals.swMatrix[trough[0]] &= ~trough[1];
      locals.lastTrough = now;
    }
  }

  /* The flipper buttons, into the matrix cells the real ones sit on.
     Sport 2000 only -- see cirsaFlipSw above for why Mephisto gets nothing.

     Read from swMatrix[CORE_FLIPPERSWCOL] rather than from the input ports,
     because that is the one place both front ends agree on: core_updateSw
     writes those two bits from the L/R Shift keys when the driver owns the
     keyboard and leaves them as the front end set them when it does not,
     both a few lines before it calls us.  inports[CORE_FLIPINPORT] would
     work for the keyboard and do nothing under VPinMAME or libpinmame.

     Change-only, and that is why FLIP_SWNO is NOT used instead even though it
     would be the idiomatic spelling: core.c:1740-1741 calls core_setSw for
     both flipper switches on EVERY frame whatever the keys are doing, and
     core_setSw clears the bit before writing it, so those two cells stop
     accepting writes from anything else -- which breaks the service menu's
     own flipper navigation and any switch sweep over elements 16/17.  (For
     the record, the numbers would be FLIP_SWNO(21,22), not (35,36):
     MDRV_IMPORT_FROM(PinMAME) installs core_swSeq2m, no+7, so the default
     (no/10)*8+(no%10-1) fallback never applies here.)

     Deliberately NOT gated on locals.inhFlip.  INH. FLIPPER takes the 50 V
     away from the coil, not the 12 V away from the button's matrix report, so
     a tilted machine still sees the button.  core_updateSw models the coil
     half on its own, from the !locals.inhFlip it is passed. */
  if (!meph) {
    const UINT8 butMask = (UINT8)(CORE_SWLLFLIPBUTBIT | CORE_SWLRFLIPBUTBIT);
    const UINT8 but     = (UINT8)((coreGlobals.swMatrix[CORE_FLIPPERSWCOL] ^
                                   coreGlobals.invSw[CORE_FLIPPERSWCOL]) & butMask);
    const UINT8 moved   = (UINT8)(but ^ locals.lastFlipBut);
    if (moved) {
      int i;
      for (i = 0; i < 2; i++) {
        const UINT8 bit = (UINT8)(i ? CORE_SWLRFLIPBUTBIT : CORE_SWLLFLIPBUTBIT);
        if (moved & bit) {
          const UINT8 idx = cirsaFlipSw[i][0], msk = cirsaFlipSw[i][1];
          if (but & bit) coreGlobals.swMatrix[idx] |=  msk;
          else           coreGlobals.swMatrix[idx] &= ~msk;
        }
      }
      locals.lastFlipBut = but;
    }
  }
}

static INTERRUPT_GEN(cirsa_vblank) {
#ifdef CIRSA_SNDSWEEP
  sweep_frame();
#endif
  /* MUART P26, INH. FLIPPER: the ROM's only control over the flippers.  The
     flipper board fires its own coil straight from the button, and the button
     contact forks -- one branch to the coil driver, one out to FILA/COL. on
     P38, the matrix report, which runs off +12 V and touches neither the 50 V
     nor GNDFLIP.  So the CPU can neither pulse a flipper nor stop seeing the
     button; all it can do is take away the 50 V, and that is what P26 does.

     core_updateSw's flipEn argument models exactly that: for a game without
     FLIP_SOL -- which is this one -- it clears the four synthesised
     flipper-coil bits in coreGlobals.solenoids2 and only sets them from the
     keys when flipEn is true.  It deliberately does NOT touch the flipper
     button switches, which the real machine still reads while tilted.  Same
     shape as gts1.c:112, gts80.c:91-108, s11.c:358, alvg.c:558, zac.c:78. */
  core_updateSw(!locals.inhFlip);
  {
    /* All EIGHT bits, and for BOTH games.  Each ROM numbers eight quick
       contacts as switch-test elements 60-67 and reads them off IC9 Port B
       (Sport 2000 through the event table at 0xD11E, Mephisto through [0xDA],
       filled at 0x103D), even though Sport 2000's manual lists only five
       populated ones.  A 0x1f mask, or a hw.gameSpecific1 gate, leaves
       elements 65-67 or all of Mephisto's unreachable.

       Nothing downstream widens with them: ic9_pa_w still ORs only bits 1-3
       back into the coil bus, and still only for Sport 2000. */
    const UINT8 qc = coreGlobals.swMatrix[12];
    if (qc != locals.qcState) {
      locals.qcState = qc;
      if (locals.qcTransparent) locals.qcLatch = qc;   /* '373 is transparent */
      /* The quick contacts do NOT drive EXTINT -- that carries PPCERO (see
         cirsa_p1_in's block comment); the contacts reach the CPU only through
         IC11, latched here, which ISR_QuickContacts reads back over IC9 Port
         B 100 times a second.  Re-reading an unchanged latch costs nothing:
         the ROM's consumer at 0xC8E7 is an edge detector on its own mirror of
         the byte. */
    }
  }
}

/*-- The AY-3-8910's own I/O ports (plate 11 / Mephisto plate 9) ---------
/  IOA is an OUTPUT and IOB an INPUT: the sequencer's register-7 special case
/  forces that on every write ((v & 0x3F) | 0x40, sport2k 0x0F50, mephisto
/  0x0862), and no score can override it.
/
/  They are NOT the playfield lamp or switch matrix -- they are the audio
/  board's own connectors, J17-J20 in both manuals: a 5x3 keypad, a 3x2 pad,
/  two counters and the three coin inputs with their inhibits.  Mephisto walks
/  a column bit across IOA bits 3..7 at 20 Hz per column and reads IOB back;
/  Sport 2000 writes IOA once at boot and never reads IOB at all, its coin door
/  and switch matrix being on the CPU board instead.
/
/  Do not restore the MAME placeholder that put IOA into
/  coreGlobals.tmpLampMatrix[0] and IOB from coreGlobals.swMatrix[]: neither
/  game's audio board drives a lamp, so on Mephisto the column strobe simply
/  overwrote lamp column 0 a hundred times a second, and feeding the playfield
/  matrix back in put phantom closures on the audio board's coin inputs.
/
/  IOB reads 0xFF, i.e. no closure: the rows sit on 10K pull-ups and the
/  columns are driven through IC6, a ULN2064, so a closure pulls a row LOW.
/  MAME's own mephisto driver returns 0xFF for the same reason.  [INFERRED] --
/  nothing in either firmware consumes the result, so no behaviour today
/  distinguishes 0x00 from 0xFF.
/----------------------------------------------------------------------*/
static READ_HANDLER(ay8910_porta_r)   { return locals.ayPortA; }
static READ_HANDLER(ay8910_portb_r)   { return 0xff; }
static WRITE_HANDLER(ay8910_porta_w)  { locals.ayPortA = data; }
static WRITE_HANDLER(ay8910_portb_w)  { }

static void ym3812_irq(int irq) {
  cpu_set_irq_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

static struct AY8910interface cirsa_ay8910Int = {
  1,			/* 1 chip */
  1500000,		/* 1.5 MHz */
  { 50 },		/* Volume */
  { ay8910_porta_r },
  { ay8910_portb_r },
  { ay8910_porta_w },
  { ay8910_portb_w },
};

static struct YM3812interface cirsa_ym3812Int = {
  1,						/* 1 chip */
  3579545,					/* NTSC clock */
  { 50 },					/* volume */
  { ym3812_irq },			/* IRQ Callback */
};

static struct DACinterface cirsa_dacInt = { 1, { 50 }};

/*-- Sound ROM banking (plate 11, PDF p.34) ------------------------------
/  IC20, a 74LS373, latches the bank byte.  Three of its outputs drive IC22
/  (74LS138) A/B/C, whose Y0-Y7 are CST0-CST7, the chip selects of the eight
/  EPROM sockets -- board order IC14, IC13, IC12, IC11, IC16, IC17, IC18,
/  IC19.  A fourth output leaves the latch as the net A15F and reaches pin 1
/  of every socket through a 10K pull-up, with no inverter: bit 3 is A15
/  directly, 0 = low half.
/
/  Pin 1 is Vpp on a 27256 and A15 on a 27512 -- the sockets are silkscreened
/  "27256-(27512)" for that reason -- and the two games populate them
/  differently, which is why the arithmetic cannot be shared:
/
/    Mephisto  eight 27256 (0x8000 each), CST0..CST7 -> 0x00000..0x38000.
/              A15F is Vpp here and does nothing to the address, so bit 3 must
/              be MASKED OFF: chip = data & 7.  Every bank byte in its PCM
/              descriptor table (ic15_02 0x073E) has bit 3 set for exactly
/              that reason, and MAME's own driver reaches the same conclusion
/              from the other side with `data & 0xf` over a 0x80000 region.
/    Sport2000 five 27512 (0x10000 each) on CST0..CST4.  Bits 0-2 pick the
/              chip, bit 3 picks the 32K half within it.
/
/  cirsa_cst[] is which physical chip sits in which socket, not a re-ordering
/  of anything the schematic says.  The five Sport 2000 27512s go in the same
/  descending sockets Mephisto uses (CST0 = IC14 ... CST4 = IC16) but their
/  dump names count the other way, so CST0..CST4 hold s411, s311, s211, s117,
/  s511.  ROM_START loads them in name order, which is upstream's and MAME's;
/  this table maps the CST index onto it.  It was derived by scoring all
/  5! x 2 candidate maps (chip order x bit-3 polarity) against the erased
/  pages of the 320K set: exactly one claims none of them, and under it the
/  DAC stops being fed erased EPROM at full scale.
/-----------------------------------------------------------------------*/
static WRITE_HANDLER(bank_w) {
  /* CST0..CST4 -> which 64K chip of REGION_SOUND1, in ROM_START's load order
     s117, s211, s311, s411, s511.  CST5..CST7 are unfitted on this board and
     never appear in the descriptor table; they fold onto chip 0. */
  static const UINT8 cirsa_cst[8] = { 3, 2, 1, 0, 4, 0, 0, 0 };
  UINT32 off;

  if (core_gameData->hw.gameSpecific1)          /* Mephisto: 8 x 27256 */
    off = (data & 0x07) * 0x8000;
  else                                          /* Sport 2000: 5 x 27512 */
    off = cirsa_cst[data & 0x07] * 0x10000 + (((data >> 3) & 1) * 0x8000);

  cpu_setbank(1, memory_region(REGION_SOUND1) + off);
#if CIRSA_VERBOSE
  /* Very hot: ~384k bank writes in 40 emulated seconds, i.e. ~9,600 log lines
     a second with -log on, which buries every other trace.  Same class as the
     "op-code execute on mapped I/O" flood already cleaned up in this tree. */
  logerror("SND BANK %x:%02x -> %05x\n", offset, data, off);
#endif
}

static READ_HANDLER(port_r) {
#if CIRSA_VERBOSE
  logerror("SND PORT %x READ\n", offset);
#endif
  return 0;
}

static WRITE_HANDLER(port_w) {
#if CIRSA_VERBOSE
  logerror("SND PORT %x:%02x\n", offset, data);
#endif
}

/*-- The sound CPU's ports 1 and 3 (plate 11 / Mephisto plate 9) ----------
/  Port 1 is the AY-3-8910's 8-bit data bus, and port 3 bit 4 is BDIR with bit
/  5 BC1.  Do NOT map port 1 to AY8910_control_port_0_w and port 3 to
/  AY8910_write_port_0_w as this driver once did -- those are the wrong pins,
/  and the AY then receives the four states of the handshake port (c7, cf, df,
/  ff) as its register data and never a music byte.
/
/  The firmware writes the AY's own BDIR/BC1 sequence out longhand (sport2k
/  0x0F40, Mephisto 0x0862): register number on P1, ORL P3,#30h to latch the
/  address, ANL P3,#CFh, the value on P1, ORL P3,#10h to write the data.
/  Mephisto's 0x05A6 -- the only port 1 *read* in either ROM -- pins the same
/  assignment down from the other side with ORL P3,#20h for BC1 alone = READ.
/
/  The strobes are acted on when the state changes, so the read-modify-write
/  P3 accesses the firmware makes for its two handshake bits cannot re-trigger
/  a latch: they leave bits 4-5 at 0.
/----------------------------------------------------------------------*/
static WRITE_HANDLER(cirsa_sndp1_w) {
  locals.sndP1 = data;                  /* just the data-bus latch */
}

static READ_HANDLER(cirsa_sndp1_r) {
  /* Only while BC1 alone is asserted does the AY drive the bus; the rest of
     the time the 8051's own quasi-bidirectional latch is what a read sees. */
  if ((locals.sndP3 & 0x30) == 0x20) return AY8910_read_port_0_r(0);
  return locals.sndP1;
}

static WRITE_HANDLER(cirsa_sndp3_w) {
  UINT8 prev = locals.sndP3;
  locals.sndP3 = data;
  if ((data ^ prev) & 0x30) {
    if ((data & 0x30) == 0x30)      AY8910_control_port_0_w(0, locals.sndP1);
    else if ((data & 0x30) == 0x10) AY8910_write_port_0_w(0, locals.sndP1);
  }
  /* P3.3 (INT1, an output here) is the sound board's BUSY line, raised on
     entry to the serial ISR at 0x0375 and the 100 Hz tick at 0x0450 and
     restored from flag 20h.6 on the way out.  It lands on MUART Port 1
     bit 5, which the main ROM tests at 0xCB41 before transmitting. */
  if ((data ^ prev) & 0x08)
    i8256_set_p1_pin(5, (data & 0x08) ? 1 : 0);
}

static WRITE_HANDLER(cirsa_sndp2_w) {
  locals.sndP2 = data;                  /* the XRAM page -- see cirsa_eram_addr */
}

/*-- MOVX @Ri needs Port 2 for the high address byte ---------------------
/  The firmware pages its 2K of XRAM and sets P2 explicitly every time: page 0
/  for the serial packet buffer, 1 for the FM voice state, 2 for the OPL2
/  register shadow, 3 for the note timers.  PinMAME's MOVX @Ri asks the driver
/  for the full address through this callback and, with none registered, falls
/  back to the bare 8-bit offset -- so all four pages land on page 0.
/
/  Two things this has to get right.  The callback is shared with MOVX @DPTR
/  (i8051ops.c:621,640), which already has all 16 bits and must be returned
/  untouched -- mem_mask tells the two apart, 0xFF for @Ri and 0xFFFF for
/  @DPTR -- and getting that wrong takes the DAC, the OPL2 and the ROM bank
/  latch off the map.  And P2 must come from a shadow kept here, not from
/  i8051_internal_r(0xA0): that routes through sfr_read(), which for a port
/  with RWM clear calls IN(2) instead of returning the latch, i.e. reads the
/  port back through this same driver.  spinb.c's dmd_eram_address keeps its
/  own P2 copy for the same reason.
/----------------------------------------------------------------------*/
static READ32_HANDLER(cirsa_eram_addr) {
  if (mem_mask > 0xff) return offset;   /* MOVX @DPTR -- already complete */
  return (UINT32)((locals.sndP2 << 8) | (offset & 0xff));
}

static READ_HANDLER(cirsa_sndp3_r) {
  /* P3.2 (INT0) is an input driven by MUART Port 1 bit 6 -- see
     cirsa_p1_out().  Every other pin of this port is either an output or
     unconnected, and reads low, exactly as it did through port_r before. */
  return (UINT8)((locals.muartP1Out & 0x40) ? 0x04 : 0x00);
}

/*-- 8051 sound-board memory maps ---------------------------------------
/  The two audio boards are identical but for the OPL2: Sport 2000's carries a
/  YM3812 with its own 14.318 MHz crystal, Mephisto's carries only the 8051, an
/  AY-3-8910 and a DAC-08.  MACHINE_DRIVER_START(mephisto) therefore adds no
/  YM3812, and its map must not name the chip's handlers -- with no chip
/  created chip_3812[0] is NULL and both handlers dereference it, which on the
/  default ymfm backend is a virtual call through a null pointer.  Mephisto's
/  firmware has not been seen to touch 0x11800/0x11801, but an empirical
/  negative is not a guarantee and splitting the map costs five repeated lines.
/  MACHINE_DRIVER_START(cirsa) swaps the OPL2 map in through
/  MDRV_CPU_MODIFY("scpu"), the same idiom it already uses for "mcpu". */

static MEMORY_READ_START(mephisto_readsnd)
  { 0x00000, 0x07fff, MRA_ROM },
  { 0x08000, 0x0ffff, MRA_BANKNO(1) },
  { 0x10000, 0x107ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(mephisto_writesnd)
  { 0x00000, 0x07fff, MWA_ROM },
  { 0x10000, 0x107ff, MWA_RAM },
  { 0x10800, 0x10800, bank_w },
  /* The same latch, reached through a second address.  The Timer 0 ISR
     services two independent PCM streams and writes a bank number
     immediately before each one's MOVC: stream 1 to 0x0F00, stream 2 to
     0x0800.  There is only one banked window, so there is only one latch to
     drive it, and a decode of the 0x0800-0x0FFF block that ignores A8-A10
     aliases it across the whole block.  [INFERRED] -- the trace has not been
     read off the plate; but each stream rewrites the bank immediately before
     its own MOVC, so one latch and two latches are indistinguishable by
     observing this firmware, and mapping 0x0F00 is right under either.
     Mephisto writes only 0x0800, so this line changes nothing for it. */
  { 0x10f00, 0x10f00, bank_w },
  { 0x11000, 0x11000, DAC_0_data_w },
MEMORY_END

/* Sport 2000 only: the two maps above plus the OPL2, an address/data pair with
   A0 selecting between them.  MACHINE_DRIVER_START(cirsa) has always added the
   chip, but until this branch nothing was mapped for the 8051 to reach it, so
   every register write the sound ROM made was silently discarded.  The pair of
   addresses is identified from the ROM's own behaviour: the unmapped writes to
   0x11800 carry exactly the OPL2 register map, gaps included, each followed by
   a write to 0x11801. */

static MEMORY_READ_START(cirsa_readsnd)
  { 0x00000, 0x07fff, MRA_ROM },
  { 0x08000, 0x0ffff, MRA_BANKNO(1) },
  { 0x10000, 0x107ff, MRA_RAM },
  { 0x11800, 0x11800, YM3812_status_port_0_r },   /* OPL2 status */
MEMORY_END

static MEMORY_WRITE_START(cirsa_writesnd)
  { 0x00000, 0x07fff, MWA_ROM },
  { 0x10000, 0x107ff, MWA_RAM },
  { 0x10800, 0x10800, bank_w },
  { 0x10f00, 0x10f00, bank_w },
  { 0x11000, 0x11000, DAC_0_data_w },
  { 0x11800, 0x11800, YM3812_control_port_0_w },
  { 0x11801, 0x11801, YM3812_write_port_0_w },
MEMORY_END

static PORT_READ_START(cirsa_readsndport)
  { 1, 1, cirsa_sndp1_r },
  { 3, 3, cirsa_sndp3_r },
  { 0, 3, port_r },
PORT_END

static PORT_WRITE_START(cirsa_writesndport)
  { 1, 1, cirsa_sndp1_w },
  { 2, 2, cirsa_sndp2_w },
  { 3, 3, cirsa_sndp3_w },
  { 0, 3, port_w },
PORT_END

MACHINE_DRIVER_START(mephisto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(CIRSA,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", I88, 6000000)
  MDRV_CPU_MEMORY(mephisto_readmem, mephisto_writemem)
  MDRV_CPU_VBLANK_INT(cirsa_vblank, 1)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(CIRSA)

  MDRV_CPU_ADD_TAG("scpu", I8051, 12000000)
  MDRV_CPU_MEMORY(mephisto_readsnd, mephisto_writesnd)
  MDRV_CPU_PORTS(cirsa_readsndport, cirsa_writesndport)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_SOUND_ADD(AY8910, cirsa_ay8910Int)
  MDRV_SOUND_ADD(DAC, cirsa_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(cirsa)
  MDRV_IMPORT_FROM(mephisto)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(cirsa_readmem, cirsa_writemem)
  /* Sport 2000's audio board has a YM3812 (OPL2); Mephisto's has none, and
     MAME agrees -- only its sport2k() config adds one.  The chip and the map
     that reaches it must be added together: mephisto_writesnd names neither
     handler, so Mephisto's 8051 cannot reach a chip that was never created. */
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(cirsa_readsnd, cirsa_writesnd)
  MDRV_SOUND_ADD(YM3812, cirsa_ym3812Int)
MACHINE_DRIVER_END

/*-- Input ports (CORE_COREINPORT, i.e. port 2) --------------------------
/  Bits 0x0100-0x0800 are the four cabinet buttons on the service bracket.
/  Bits 0x0001-0x0040 are coin, start and the ball trough, which are playfield
/  matrix switches on the real machine rather than cabinet wiring -- bound on
/  their own port and translated to the matrix in SWITCH_UPDATE, the same
/  pattern capcom.h, atari.h and gp.h use.  Which matrix bit each lands on
/  differs per game: see cirsaCoinSw/mephCoinSw above.
/
/  The trough is a BITTOG because it is a level, not an event: the balls hold
/  those switches closed for the whole game.
/
/  A coin, by contrast, is an event, and both ROMs refuse a long closure as a
/  jammed mech -- past ~0.58 s on sport2k (0x068EE compares the run length
/  against 0x0F) but only ~0.28 s on either Mephisto.  IPF_IMPULSE holds the
/  bit for a fixed number of frames however long the key is really held; 8
/  frames is 133 ms, above the ~50 ms the debouncer needs and below both jam
/  windows.  COREPORT_BITIMP would be 1 frame, under the debounce tick, and
/  can be missed entirely.  Start is left momentary -- it has no jam timeout.
/----------------------------------------------------------------------*/
#define CIRSA_COIN(mask, type) \
  PORT_BITX(mask, IP_ACTIVE_HIGH, (type) | IPF_IMPULSE | (8<<8), \
            IP_NAME_DEFAULT, IP_KEY_DEFAULT, IP_JOY_DEFAULT)

INPUT_PORTS_START(cirsa)
  CORE_PORTS
  SIM_PORTS(4)
  PORT_START /* CORE_COREINPORT */
    /* MAME's own input types, so they inherit its default keys and joystick
       codes and stay remappable: 5/6/7 for the chutes, 1 for start. */
    CIRSA_COIN(       0x0001, IPT_COIN1)
    CIRSA_COIN(       0x0002, IPT_COIN2)
    CIRSA_COIN(       0x0004, IPT_COIN3)
    COREPORT_BITDEF(  0x0008, IPT_START1, IP_KEY_DEFAULT)
    /* The trough has no MAME equivalent -- it is a level held by the balls, not
       a control -- so it keeps an explicit key and a descriptive name. */
    COREPORT_BITTOG(  0x0010, "Ball Trough", KEYCODE_B)
    /* The four buttons on the service bracket.  IPT_SERVICE1..4 keeps these
       clear of the coin keys, which they previously collided with (Test was
       on 7 = IPT_COIN3's default, Advance on 8 = IPT_COIN4's).  The names
       stay descriptive: "Service 1" says nothing about what the button is. */
    PORT_BITX(0x0100, IP_ACTIVE_HIGH, IPT_SERVICE1, "Test",    IP_KEY_DEFAULT, IP_JOY_DEFAULT)
    PORT_BITX(0x0200, IP_ACTIVE_HIGH, IPT_SERVICE2, "Advance", IP_KEY_DEFAULT, IP_JOY_DEFAULT)
    PORT_BITX(0x0400, IP_ACTIVE_HIGH, IPT_SERVICE3, "EG1",     IP_KEY_DEFAULT, IP_JOY_DEFAULT)
    PORT_BITX(0x0800, IP_ACTIVE_HIGH, IPT_SERVICE4, "EG2",     IP_KEY_DEFAULT, IP_JOY_DEFAULT)
INPUT_PORTS_END

/* Positions 0..6 and 7..13 are the two 7-character LA8041R-11B alphanumeric
   rows, CORE_SEG16N; 14..20 and 21..27 the two 7-digit LTS 3401 numeric rows,
   CORE_SEG8D.  CORE_SEG16N controls pixel geometry only, not bit
   interpretation -- the ROM's bit order is translated on write, see
   cirsa_seg16()/cirsa_seg8d().  core.c's segData[] gives both types the same
   {20,15} cell, so the existing {0,16} column spacing fits either way. */
static core_tLCDLayout cirsa_disp[] = {
  {0, 0, 0, 7,CORE_SEG16N}, {0,16, 7, 7,CORE_SEG16N},
  {3, 0,14, 7,CORE_SEG8D}, {3,16,21, 7,CORE_SEG8D},
  {6, 8,28, 2,CORE_SEG8D}, {6,14,30, 1,CORE_SEG8D}, {6,18,31, 2,CORE_SEG8D},
  {0}
};
/* Mephisto's panel is NOT Sport 2000's -- 33 identical LTS 3401 seven-segment
   digits, no alphanumeric units at all, grouped 7+7+7+7+5 across the four
   player boards and the credit/match board.  Same grid as cirsa_disp for
   visual consistency; every group stays CORE_SEG8D. */
static core_tLCDLayout mephisto_disp[] = {
  {0, 0, 0, 7,CORE_SEG8D}, {0,16, 7, 7,CORE_SEG8D},
  {3, 0,14, 7,CORE_SEG8D}, {3,16,21, 7,CORE_SEG8D},
  {6, 8,28, 2,CORE_SEG8D}, {6,14,30, 1,CORE_SEG8D}, {6,18,31, 2,CORE_SEG8D},
  {0}
};
/* hw.swCol counts CUSTOM switch columns beyond CORE_STDSWCOLS (12), NOT the
   game's hardware column count, and coreGlobals.swMatrix is only
   CORE_MAXSWCOL (16) entries -- a previous value of 10 here made core.c read
   22 entries from that 16-entry array.  Both games read swMatrix[1..10]
   through ic20_pc_r, which is inside the standard range and needs no custom
   column at all.

   Both are 1 for the quick contacts, which need a slot of their own at
   swMatrix[12], the first CUSTOM column: 12+1 = 13 keeps core.c's two
   CORE_STDSWCOLS+hw.swCol loops inside CORE_MAXSWCOL.  Mephisto has the same
   eight contacts on the same wires (its switch scan reads IC9 Port B into
   [0xDA] at 0x103D and its SWITCH TEST tests that byte for elements 60-67), so
   both games share the path. */
/* hw.gameSpecific1 (7th field of the hw sub-struct) is the
   Sport-2000-vs-Mephisto switch: 0 = Sport 2000, 1 = Mephisto/mephist1.  Its
   consumers are cirsa_frameLen(), cirsa_shift_frame()'s column-mask table and
   segment-group mapping, ic9_pa_w's coil-bus position order and its Sport
   2000-only quick-contact OR, and bank_w's sound-ROM arithmetic.  Every one is
   a selector between two implemented paths, not a "not established yet" gate. */
/*-- Ball simulator: not supplied ---------------------------------------
/  Both games pass NULL for core_tGameData.simData, so playfield switches are
/  closed by hand with the generic column/row keys, as in most of this tree.
/  The one thing that costs: Sport 2000's in-game music engine never runs,
/  because its rulebook waits for the served ball to close element 31 (BALL
/  OUT) and nothing here ever does.  Reachable by hand today; the proper fix is
/  a simData table using sim_tState's solSwNo -- taf.c:213 is the pattern --
/  with a trough state holding elements 32-35, released by coil 15, whose next
/  state closes element 31.
/----------------------------------------------------------------------*/
static core_tGameData cirsaGameData    = {0,cirsa_disp,{FLIP_SW(FLIP_L),1,8}};
static core_tGameData mephistoGameData = {0,mephisto_disp,{FLIP_SW(FLIP_L),1,8,0,0,0,1}};
static void init_cirsa(void) {
  core_gameData = &cirsaGameData;
}
static void init_mephisto(void) {
  core_gameData = &mephistoGameData;
}

ROM_START(mephisto)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("cpu_ver1.2", 0x00000, 0x8000, CRC(845c8eb4) SHA1(2a705629990950d4e2d3a66a95e9516cf112cc88))
      ROM_RELOAD(0x08000, 0x8000)
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("ic15_02", 0x00000, 0x8000, CRC(2accd446) SHA1(7297e4825c33e7cf23f86fe39a0242e74874b1e2))
  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("ic14_s0", 0x00000, 0x8000, CRC(7cea3018) SHA1(724fe7a4456cbf2ac01466d946668ee86f4410ae))
    ROM_LOAD("ic13_s1", 0x08000, 0x8000, CRC(5a9e0f1d) SHA1(dbfd307706c51f8809f4867a199b4b62beb64379))
    ROM_LOAD("ic12_s2", 0x10000, 0x8000, CRC(b3cc962a) SHA1(521376cab7e917a5d5f5f183bccb21bd13327c48))
    ROM_LOAD("ic11_s3", 0x18000, 0x8000, CRC(8aaa21ec) SHA1(29f17249cac62128fd8b0eee415ce399ee2ec672))
    ROM_LOAD("ic16_c",  0x20000, 0x8000, CRC(5f12b4f4) SHA1(73fbdb57fca0dbc918e6665a6cb949e741f2720a))
    ROM_LOAD("ic17_d",  0x28000, 0x8000, CRC(d17e18a8) SHA1(372eaf209ea5d26f3c096aadd7d028ef68bfb68e))
    ROM_LOAD("ic18_e",  0x30000, 0x8000, CRC(eac6dbba) SHA1(f4971c8b0aa3a72c396b943a0ee3094afb902ec1))
    ROM_LOAD("ic19_f",  0x38000, 0x8000, CRC(cc4bb629) SHA1(db46be2a8034bbd106b7dd80f50988c339684b5e))
ROM_END
/* init_mephisto is a real function (above) that selects mephistoGameData,
   not init_cirsa -- Mephisto uses its own column-mask table. */
#define input_ports_mephisto input_ports_cirsa
CORE_GAMEDEFNV(mephisto,"Mephisto (rev. 1.2)",1986,"Stargame",mephisto,GAME_IMPERFECT_SOUND)

ROM_START(mephist1)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("cpu_ver1.1", 0x00000, 0x8000, CRC(ce584902) SHA1(dd05d008bbd9b6588cb204e8d901537ffe7ddd43))
      ROM_RELOAD(0x08000, 0x8000)
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("ic15_02", 0x00000, 0x8000, CRC(2accd446) SHA1(7297e4825c33e7cf23f86fe39a0242e74874b1e2))
  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("ic14_s0", 0x00000, 0x8000, CRC(7cea3018) SHA1(724fe7a4456cbf2ac01466d946668ee86f4410ae))
    ROM_LOAD("ic13_s1", 0x08000, 0x8000, CRC(5a9e0f1d) SHA1(dbfd307706c51f8809f4867a199b4b62beb64379))
    ROM_LOAD("ic12_s2", 0x10000, 0x8000, CRC(b3cc962a) SHA1(521376cab7e917a5d5f5f183bccb21bd13327c48))
    ROM_LOAD("ic11_s3", 0x18000, 0x8000, CRC(8aaa21ec) SHA1(29f17249cac62128fd8b0eee415ce399ee2ec672))
    ROM_LOAD("ic16_c",  0x20000, 0x8000, CRC(5f12b4f4) SHA1(73fbdb57fca0dbc918e6665a6cb949e741f2720a))
    ROM_LOAD("ic17_d",  0x28000, 0x8000, CRC(d17e18a8) SHA1(372eaf209ea5d26f3c096aadd7d028ef68bfb68e))
    ROM_LOAD("ic18_e",  0x30000, 0x8000, CRC(eac6dbba) SHA1(f4971c8b0aa3a72c396b943a0ee3094afb902ec1))
    ROM_LOAD("ic19_f",  0x38000, 0x8000, CRC(cc4bb629) SHA1(db46be2a8034bbd106b7dd80f50988c339684b5e))
ROM_END
/* mephist1 (rev 1.1) has the identical column-mask table content as
   mephisto (rev 1.2), confirmed by a byte scan of its ROM -- see the
   comment above colFromBitMephisto -- so it reuses init_mephisto rather
   than init_cirsa. */
#define init_mephist1 init_mephisto
#define input_ports_mephist1 input_ports_cirsa
CORE_CLONEDEFNV(mephist1,mephisto,"Mephisto (rev. 1.1)",1986,"Stargame",mephisto,GAME_IMPERFECT_SOUND)

ROM_START(sport2k)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD("u1_256.bin", 0x00000, 0x8000, CRC(403f9000) SHA1(376dc17355c9569bd1ed9b19dbc322bfd69bf938))
    ROM_LOAD("u2_256.bin", 0x08000, 0x8000, CRC(4a88cc10) SHA1(591568dc60c40cc058f45a144c098faccb4e970c))
      ROM_RELOAD(0xf8000, 0x8000)
  NORMALREGION(0x20000, REGION_CPU2)
    ROM_LOAD("c541_256.bin", 0x00000, 0x8000, CRC(7ca4a952) SHA1(6b01f7f79fa88c4ae71a6a19341760fa256b9958))
  NORMALREGION(0x50000, REGION_SOUND1)
    ROM_LOAD("s117_512.bin", 0x00000, 0x10000, CRC(035d302e) SHA1(f207ea239e5a34839366cc19a569ab5f3d1e1a60))
    ROM_LOAD("s211_512.bin", 0x10000, 0x10000, CRC(61cf84f9) SHA1(4c5680fbf48f30fbe0e15f4194ab708955df7721))
    ROM_LOAD("s311_512.bin", 0x20000, 0x10000, CRC(162cd1ff) SHA1(4d9ad7a839cc16e74abfc77c92674608ccba8cc3))
    ROM_LOAD("s411_512.bin", 0x30000, 0x10000, CRC(4deffaa0) SHA1(98a20a01437ea060ac5c6fb52f4da892fee1fb75))
    ROM_LOAD("s511_512.bin", 0x40000, 0x10000, CRC(ca9afa80) SHA1(6f219bdc1ad06e340b2930610897b70369a43684))
ROM_END
#define init_sport2k init_cirsa
#define input_ports_sport2k input_ports_cirsa
CORE_GAMEDEFNV(sport2k,"Sport 2000",1988,"Cirsa",cirsa,GAME_IMPERFECT_SOUND)
