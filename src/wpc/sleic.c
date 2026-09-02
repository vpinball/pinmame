// license:BSD-3-Clause

/************************************************************************************************
 Sleic (Spain)
 -------------

   Hardware:
   ---------
		CPU:     I80C188 for game & sound (drives YM3812 + OKI MSM6376 directly),
		         DMD coprocessor: I80C39 (Bike Race / Sleic Pin-Ball),
		         Z80 for I/O (switches/lamps/solenoids; forwards sound cmds over J1)
		DISPLAY: DMD 128x32
		SOUND:   YM3812 (OPL2 FM music) + OKI MSM6376 (ADPCM speech/FX),
		         both driven by the 80188
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/i8039/i8039.h"
#include "sound/adpcm.h"
#include "sound/3812intf.h"
#include "sleic.h"
#include <stdlib.h>

//#define DEBUG_SLEIC // enable for environment var support (see getenv's), etc

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  //UINT8  sndCmd;
  UINT8  swCol;
  UINT8  lampCol;
  UINT8  rawDMD[128 * 32];

  /* Bike Race (SLEIC3) interrupt-rate accumulators, see sleic3_irq_gen */
  double int0Acc, t0Acc;

  /* DMD: how the two raster fields are weighted, and which submit path is used;
   * both in MACHINE_INIT (see notes at sleic3_build_dmd_frame and sleic_submit_dmd_frame) */
  int    dmdEqualFields;
  int    dmdFromPtr;

  /* completed-frame latch for the I8039 machines, see SLEIC3_DMD_LATCH_TICKS */
  UINT8  dmdLatch[128 * 32];
  int    dmdLatchTtl;

  /* OKI MSM6376: phrase latch (PCS6 0xA0300), /OKCS strobe shadow (PCS0 0xA0000) and whether a real phrase number is armed */
  UINT8  okiLatch;
  UINT8  okiPrevStrobe;
  int    okiPending;

  /* J1 link state, see the PCS2 notes at sleic_periph_r */
  UINT8  j1Inbound;  /* PCS2 0xA0100 latch: last byte strobed by the Z80  */
  UINT8  j1Fresh;    /* a new Z80 byte is waiting to be read by the 80188 */
  UINT8  j1PrevCtrl; /* Z80 port 0x81 shadow for the bit-2 strobe edge    */
  UINT8  cmd188;     /* the byte the 80188 wrote to PCS1 0xA0080, latched for
                      * the Z80 to read via IN port 0x00 in its NMI handler */

  /* YM3812 A0 line: 0 = register/index port, 1 = data port.  SLEIC1 drives it
   * from PCS0 bit 1, Bike Race toggles it per write (these use different peripheral write handlers, so serves both) */
  UINT8  ymA0;
} locals;

#ifdef DEBUG_SLEIC
/* env-gated DMD frame capture for headless verification.
 * Set SLEIC_DMD_DUMP=/path/prefix to append one PGM (P5, 128x32) per submitted
 * frame as /path/prefix.<seq>.pgm. No env var set = no-op */
static void sleic_dmd_dump(const UINT8 *frame) {
  const char *pfx = getenv("SLEIC_DMD_DUMP");
  static int seq = 0;
  char fn[512];
  FILE *fp;
  int i;
  if (!pfx) return;
  snprintf(fn, sizeof fn, "%s.%05d.pgm", pfx, seq++);
  if (!(fp = fopen(fn, "wb"))) return;
  fprintf(fp, "P5\n128 32\n255\n");
  for (i = 0; i < 128 * 32; i++) {
    unsigned v = frame[i];                 /* brightness 0..3 */
    fputc((int)(v > 3 ? v : v * 85), fp);  /* 0..3 -> 0,85,170,255 */
  }
  fclose(fp);
}
#endif

// switches start at 50 for column 1, and each column adds 10
static int SLEIC_sw2m(int no) { return (no/10 - 4)*8 + no%10; }
static int SLEIC_m2sw(int col, int row) { return 40 + col*10 + row; }

static INTERRUPT_GEN(SLEIC_irq_i80188) {
  cpu_set_irq_line(SLEIC_MAIN_CPU, 0, PULSE_LINE);
}

/* Sleic Pin-Ball (SLEIC1) 80C188 interrupt: the firmware enables ONLY the internal
 * Timer0 interrupt (reset PCB init at F000:FEAC: T0CON=0xE003 enable+int, T0CMPA/B=
 * 0x4000, TCUCON=0x0003 unmasked; INT0-3/DMA0-1 all masked).  Timer0 clocks at
 * CPU/4 = 2 MHz, so it fires ~122 Hz, vector type 0x08 -> ISR F000:DF7F, which
 * decrements the firmware's delay counters ([0x4DF] etc.).  The i188 core
 * does not emulate the internal timer, so we must inject the Timer0 vector here.
 * The base SLEIC_irq_i80188 pulses IRQ0 WITHOUT a vector, so the ISR never runs and
 * the post-boot delay loops (e.g. E000:0050 'mov [0x4DF],0x12C; spin until 0') hang
 * forever -> blank DMD.  Deliver vector 0x08 like the working Bike Race path */
static INTERRUPT_GEN(sleic1_irq_gen) {
  cpu_set_irq_line_and_vector(SLEIC_MAIN_CPU, 0, HOLD_LINE, 0x08);
}

/* Bike Race (SLEIC3): Timer0 (type 0x08) = 99.18 Hz (T0CON/T0CMP); INT0
 * (type 0x0C) = 72.5 Hz frame strobe (the 145 Hz DMD wire rate / 2 bitplanes);
 * NMI (type 0x02) is the J1-byte-arrival interrupt, strobe-driven from
 * sleic3_z80_write (not periodic). No IVT memcpy here: bkcpu04 copies its own
 * IVT (CS:00C4) to physical 0 before STI, so physical 0 must not be clobbered */
static INTERRUPT_GEN(sleic3_irq_gen) {
  locals.t0Acc   += 99.18 / 244.0;
  locals.int0Acc += 72.5 / 244.0;
  if (locals.t0Acc >= 1.0)        { locals.t0Acc   -= 1.0; cpu_set_irq_line_and_vector(SLEIC_MAIN_CPU, 0, HOLD_LINE, 0x08); }
  else if (locals.int0Acc >= 1.0) { locals.int0Acc -= 1.0; cpu_set_irq_line_and_vector(SLEIC_MAIN_CPU, 0, HOLD_LINE, 0x0C); }
}

/* How the panel's two raster fields are weighted, which differs per machine because the
 * two games ship different I8039 display ROMs:
 *
 *   Sleic Pin-Ball (sp01-1_1.rom): both fields hold the row lit for essentially the same
 *     time -- P1.7 is raised for MOV R6,#30 / DJNZ (98 cycles) in field 1 against
 *     MOV R6,#2E / DJNZ (94 cycles) in field 2, a ratio of 1.04:1.  Two equal fields give
 *     THREE levels, not four: off, one plane (either one, ~50%), both planes (100%).  The
 *     artwork agrees -- in the source images the field-1 plane is a strict subset of the
 *     field-2 plane (e.g. at F000:9943: 0 pixels in field 1 only, 769 in field 2 only,
 *     872 in both), so the pair encodes "lit" plus "lit brightly", not a 2-bit number.
 *     Both single-plane states do occur in the frame buffer during play (measured over a
 *     scripted game: 7.9% of pixels field-1-only, 7.4% field-2-only), and the real panel
 *     shows them at the SAME brightness -- rendering them two levels apart is what made
 *     the shading look wrong against the machine.
 *
 *   Bike Race (bkdsp01.bin): field 1 carries an extra MOV R6,#20 / DJNZ dwell (66 cycles)
 *     that field 2 does not have at all, so the fields are strongly asymmetric and the
 *     pair really is a weighted 2-bit value.  Keep the 4-level mapping there.
 *
 * Set in MACHINE_INIT (locals.dmdEqualFields); SLEIC1 (Sleic Pin-Ball) overrides it */

/* Decode the 2-bitplane frame buffer at 0x60410 into a 128x32 brightness grid.
 * See the plane/weighting notes in SLEIC_irq_i8039 below */
static void sleic3_build_dmd_frame(UINT8 *dst) {
  int ii;
  const UINT8 * const buf = memory_region(SLEIC_MEMREG_CPU) + 0x60410;
  for (ii = 0; ii < 32; ii++) {
    UINT8 *line = dst + ii * 128;
    const UINT8 * const src = buf + ii * 32;
    int jj;
    for (jj = 0; jj < 16; jj++) {
      UINT8 f1 = src[jj];         /* plane lit by raster field 1 (rows 0x00-0x1F) */
      UINT8 f2 = src[jj + 0x800]; /* plane lit by raster field 2 (rows 0x20-0x3F) */
      int kk;
      for (kk = 7; kk >= 0; kk--) {
        int a = (f1 >> kk) & 1, b = (f2 >> kk) & 1;
        *line++ = locals.dmdEqualFields ? (a | b ? (a & b ? 3 : 2) : 0)  /* 3 levels  */
                                        : ((a << 1) | b);                /* 4 levels  */
      }
    }
  }
}

/* Bike Race V4.1 rebuilds the panel from scratch every frame: it strobes PCS4 bit 3 to
 * announce a finished frame, then takes the DMD lock (PCS0 bit 0) to blank-clear all
 * 0x2FFF bytes, drops it, and redraws.  Sampling the buffer freely -- as the I8039 tick
 * below does -- therefore catches cleared and half-drawn states, which is exactly the
 * garbage that set showed.  So latch a copy at the strobe and keep displaying it: the
 * I8039 tick still submits at the panel rate (which the DMD PWM integrator wants), it
 * just submits complete frames.
 *
 * The latch has to lapse rather than stay on for good.  The 1992 sets redraw the panel
 * incrementally and are perfectly stable to sample, and they strobe bit 3 exactly once,
 * during init -- latching on that one strobe and holding it freezes them on an empty
 * frame.  So a strobe only claims the next second of ticks: if the strobes keep coming
 * the latch stays in charge (V4.1 strobes every ~90 ms), and if they stop we fall back
 * to sampling the buffer directly, which is safe precisely because a redraw is always
 * followed by a strobe */
#define SLEIC3_DMD_LATCH_TICKS 244 /* ~1 s at the 244 Hz I8039 tick */

static INTERRUPT_GEN(SLEIC_irq_i8039) {
  cpu_set_irq_line(SLEIC_DISPLAY_CPU, 0, PULSE_LINE);

  /* The I8039 is a pure timing generator (drives only the panel scan), so we read
   * the 80188's DMD frame buffer directly.  The panel is a 2-bitplane PWM display
   * with 4 brightness levels; both planes live in the frame buffer at 0x60410:
   *
   *   plane 0 : 0x60410 + row*0x20, 16 bytes (= 128 px, MSB first, 1 = lit)
   *   plane 1 : same, +0x800
   *
   * (the 16 bytes following each row's plane 0 are unused padding).  The layout is
   * written by the 80188 frame blitter at F000:08AF in bkcpu04:
   *     mov di,0410h / mov cx,20h / mov bx,0800h
   *     mov al,ds:[bp+si] / mov es:[bx+di],al   ; plane 1 -> +0x800
   *     lodsb / stosb                           ; plane 0 -> +0x000
   *     add di,20h                              ; row stride 32
   *
   * That there are two differently weighted fields (and not just two copies of one
   * image) comes from the I8039 raster loop in bkdsp01 -- the whole program is 116
   * bytes: it scans the row counter twice per refresh, 0x00-0x1F then 0x20-0x3F, and
   * the first field carries an extra dwell (MOV R6,#20 / DJNZ R6,$ at 0x0032) that
   * the second does not, so row-counter bit 5 picks the plane and the two fields are
   * lit for different lengths of time.  Bit 5 is the long-dwell field when clear, which
   * makes the +0x000 plane the bright one -- the MSB.
   *
   * That cannot be read off the raster loop alone, because the 80188 sees the panel RAM
   * through a scrambled decode (the buffer base 0x410 holds A10 and A4 high, rows step
   * A5-A9, the plane is A11), so bit 5 cannot be traced to A11 without the board's PAL.
   * What settles it is how Sleic Pin-Ball actually uses the two planes in play: of 822
   * distinct in-game frames captured over a scripted game, 613 are drawn entirely into
   * the +0x000 plane and none of the rest use +0x800 on its own.  The whole ordinary
   * display -- score, JUGADOR/BOLA, the text screens -- is that one plane, and on the
   * real machine it reads as a normally bright display with only the highlights (both
   * planes) hotter.  Making +0x000 the LSB would run the entire game at one third
   * brightness and leave the two brighter levels almost unused, which is not what the
   * hardware does.  This also agrees with the blitter, where +0x000 is the primary
   * lodsb/stosb stream, and with Io Moon's decode above, where the base plane is the
   * MSB.  (A statistic over the attract animation appears to favour the opposite
   * assignment by counting single-level pixel steps; it does not, because it assumes
   * fades are done by toggling the LSB when this firmware fades by toggling the main
   * plane, which is a two-level step) */
  if (locals.dmdLatchTtl > 0) { /* firmware is announcing completed frames */
    locals.dmdLatchTtl--;
    memcpy(locals.rawDMD, locals.dmdLatch, sizeof locals.rawDMD);
  }
  else
    sleic3_build_dmd_frame(locals.rawDMD);
#ifdef DEBUG_SLEIC
  sleic_dmd_dump(locals.rawDMD);
#endif
  core_dmd_submit_frame(core_gameData->lcdLayout->importedLayout ? core_gameData->lcdLayout->importedLayout : core_gameData->lcdLayout, locals.rawDMD, 1);
}

static INTERRUPT_GEN(SLEIC_irq_z80) {
  cpu_set_irq_line(SLEIC_IO_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(SLEIC_interface_update) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % SLEIC_LAMPSMOOTH) == 0)
    memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));

  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;

  core_updateSw(TRUE);
}

#ifdef MAME_DEBUG
static void showData(int data) {
  static char s[3];
  sprintf(s, "%02x", data);
  core_textOut(s, 6, 25, 2, 5);
}
#endif /* MAME_DEBUG */

/* env-gated headless test harness.  Nothing below has any effect unless the
 * matching environment variable is set -- in normal use the frontend (Visual Pinball)
 * supplies ball and switch state, and standalone play uses the key maps further down.
 *
 *   SLEIC_INJECT_CAB=<hex>  hold cabinet bits in swMatrix[9]
 *   SLEIC_INJECT_COL=<n> + SLEIC_INJECT_BIT=<hex>   hold one matrix position
 *   SLEIC_TROUGH[=<hex>]    close this game's ball-trough optos once the machine is up;
 *                           the optional value overrides the per-game default mask
 *   SLEIC_BALLSAT=<frame>   when that happens (default 400).  A full trough from frame 0
 *                           is not "balls in the machine", it is optos that failed during
 *                           power-up, which the firmware reports as a fault.
 *   SLEIC_PLAY="f:what,..." scripted cabinet presses at frame f; what is one of
 *                           coin, start, test, lflip, rflip, tilt
 *   SLEIC_HOLD=<frames>     how long each scripted press is held (default 10)
 */
#ifdef DEBUG_SLEIC
static void sleic_debug_switches(int troughCol, UINT8 troughDefault) {
  static int frame = 0;
  const char *e, *col, *bit;
  frame++;

  if ((e = getenv("SLEIC_INJECT_CAB")))
    coreGlobals.swMatrix[9] |= (UINT8)strtol(e, NULL, 0);
  col = getenv("SLEIC_INJECT_COL"); bit = getenv("SLEIC_INJECT_BIT");
  if (col && bit)
    coreGlobals.swMatrix[strtol(col, NULL, 0) & 0xf] |= (UINT8)strtol(bit, NULL, 0);

  if ((e = getenv("SLEIC_TROUGH"))) {
    const char *at  = getenv("SLEIC_BALLSAT");
    const char *off = getenv("SLEIC_TROUGHOFF");
    const UINT8 mask = (e[0] && e[0] != '1') ? (UINT8)strtol(e, NULL, 0) : troughDefault;
    const int from = at ? (int)strtol(at, NULL, 10) : 400;
    /* The optos are only closed between BALLSAT and TROUGHOFF: a ball that never leaves
     * the trough is a ball that never reaches play, so holding them shut for the whole
     * run keeps the game out of ball-in-play (and therefore silent) */
    const int to = off ? (int)strtol(off, NULL, 10) : 0;
    if (frame >= from && (!to || frame < to))
      coreGlobals.swMatrix[troughCol] |= mask;
  }

  /* SLEIC_SWEEP=<col-lo>-<col-hi> pulses each matrix position in turn from SLEIC_SWEEPAT
   * (default 1500), SLEIC_SWEEPHOLD frames each (default 6): playfield activity that a
   * cabinet button alone cannot produce, so the game actually scores */
  if ((e = getenv("SLEIC_SWEEP"))) {
    const char *at = getenv("SLEIC_SWEEPAT"), *hd = getenv("SLEIC_SWEEPHOLD");
    const int from = at ? (int)strtol(at, NULL, 10) : 1500;
    const int hold = hd ? (int)strtol(hd, NULL, 10) : 6;
    int lo = 1, hi = 8; char *q;
    lo = (int)strtol(e, &q, 10); if (*q == '-') hi = (int)strtol(q + 1, NULL, 10); else hi = lo;
    if (frame >= from) {
      const int slot = (frame - from) / hold;          /* which position we are on   */
      const int ncol = (hi - lo + 1);
      const int idx  = slot % (ncol * 8);
      coreGlobals.swMatrix[lo + idx / 8] |= (UINT8)(1u << (idx % 8));
    }
  }

  if ((e = getenv("SLEIC_PLAY"))) {
    const char *h = getenv("SLEIC_HOLD");
    const int hold = h ? (int)strtol(h, NULL, 10) : 10;
    const char *p = e;
    while (*p) {
      char *end;
      long at = strtol(p, &end, 10);
      p = end;
      if (*p == ':') p++;
      if (frame >= at && frame < at + hold) {
        UINT8 b = 0;
        if      (!strncmp(p, "coin",  4)) b = 0x20;
        else if (!strncmp(p, "start", 5)) b = 0x10;
        else if (!strncmp(p, "test",  4)) b = 0x02;
        else if (!strncmp(p, "lflip", 5)) b = 0x08;
        else if (!strncmp(p, "rflip", 5)) b = 0x04;
        else if (!strncmp(p, "tilt",  4)) b = 0x01;
        coreGlobals.swMatrix[9] |= b;
      }
      while (*p && *p != ',') p++;
      if (*p == ',') p++;
    }
  }
}
#endif

static SWITCH_UPDATE(SLEIC) {
#ifdef MAME_DEBUG
  static UINT8 data = 0;
#endif
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
#ifdef MAME_DEBUG
  if      (keyboard_pressed_memory_repeat(KEYCODE_Z, 2))
    showData(data -= 16);
  else if (keyboard_pressed_memory_repeat(KEYCODE_X, 2))
    showData(--data);
  else if (keyboard_pressed_memory_repeat(KEYCODE_C, 2))
    showData(++data);
  else if (keyboard_pressed_memory_repeat(KEYCODE_V, 2))
    showData(data += 16);
  else if (keyboard_pressed_memory_repeat(KEYCODE_SPACE, 2)) {
    OKIM6376_data_0_w(0, data);
    OKIM6376_data_0_w(0, 0x10);
  }
#endif /* MAME_DEBUG */
}

static WRITE_HANDLER(pic_w) {
#ifdef DEBUG_SLEIC
  if (getenv("SLEIC_TRACE_PW")) fprintf(stderr, "[188->periph] PCS%d off=%03x data=%02x\n", offset>>7, offset, data);
#endif
  logerror("PIC W(%03x->%2x) = %02x\n", offset, offset>>7, data);
}

/* Io Moon submits its DMD from the display pointer at 4000:1150 when the firmware
 * strobes PCS4 bit 3.  Bike Race and Sleic Pin-Ball instead have an I8039 that rasters
 * the panel out of the 0x60410 frame buffer, and their 4000:1150 is ordinary work RAM,
 * so the pointer-based submit must not run for them -- it would push whatever that RAM
 * happens to hold as a frame.  Set in MACHINE_INIT(SLEIC) from the presence of the
 * display CPU.  (Bike Race V4.1 made this visible: it clears and redraws the panel
 * every frame and strobes PCS4 bit 3 each time -- 114 strobes in a 600-frame run,
 * against a single one from the 1992 sets -- so the bogus frames swamped the real
 * ones and the DMD showed garbage) -> locals.dmdFromPtr */

/* Snapshot the 2-bitplane DMD frame buffer and submit the 128x32 brightness
 * grid to the DMD core. Each plane is 512 bytes (32 rows x 16 bytes,
 * MSB = leftmost pixel); plane 0 weighted x2, plane 1 x1 -> 4 grey levels.
 * cpu_readmem20 routes through the 80188 memory map (per docs/dmd_graphics.md) */
static void sleic_submit_dmd_frame(void) {
  int row;
  UINT8 * __restrict dst = locals.rawDMD;
  unsigned p1ptr, s1ptr, base;
  const core_tLCDLayout *layout = core_gameData->lcdLayout->importedLayout
                                ? core_gameData->lcdLayout->importedLayout
                                : core_gameData->lcdLayout;
  /* The 80188 draws the DMD into the buffer addressed by the display pointer at
   * 4000:1150 (offset p1, segment s1); the PIC rasters from there. seg 7000h is
   * only a clear/staging area (always zero). Plane 0 = base, plane 1 = base+0x200 */
  p1ptr = cpu_readmem20(0x41150) | (cpu_readmem20(0x41151) << 8);
  s1ptr = cpu_readmem20(0x41152) | (cpu_readmem20(0x41153) << 8);
  base  = (s1ptr << 4) + p1ptr;
  for (row = 0; row < 32; row++) {
    int row_offset = row * 16;
    int byte_idx;
    for (byte_idx = 0; byte_idx < 16; byte_idx++) {
      const UINT8 b0 = cpu_readmem20(base + row_offset + byte_idx);
      const UINT8 b1 = cpu_readmem20(base + 0x200 + row_offset + byte_idx);
      int bit;
      for (bit = 7; bit >= 0; bit--) {
        UINT8 p0 = (b0 >> bit) & 1;
        UINT8 p1 = (b1 >> bit) & 1;
        *dst++ = (p0 << 1) | p1;    /* plane 0 = MSB, plane 1 = LSB */
      }
    }
  }
#ifdef DEBUG_SLEIC
  sleic_dmd_dump(locals.rawDMD);
#endif
  core_dmd_submit_frame(layout, locals.rawDMD, 1);
}

/* PACS peripheral chip-select block at segment A000h (base 0xA0000,
 * 7 selects PCS0-PCS6 on 0x80 boundaries):
 *   0xA0000 PCS0  DMD control reg 1  AND the OKI /OKCS strobe (bit 5)
 *   0xA0080 PCS1  DMD control reg 2
 *   0xA0200 PCS4  DMD mode; bit 3 rising edge = swap-buffer / frame strobe
 *   0xA0280 PCS5  YM3812 register/index port (A0=0)
 *   0xA0281 PCS5  YM3812 data port          (A0=1)
 *   0xA0300 PCS6  DMD enable (init 0x80)  AND the OKI control latch
 *                 (phrase number in bits 0-6, channel select in bit 7)
 *
 * YM3812 (IC60) = in-game FM music; OKI MSM6376 (IC51) = speech/FX. OKI trigger
 * model (exact latch bits await the IC7 PAL dump): a non-zero phrase written to
 * 0xA0300 ARMS a phrase, the next /OKCS rising edge (0xA0000 bit 5) STARTS it
 * -> locals.okiLatch / locals.okiPrevStrobe / locals.okiPending */

/* J1 inbound byte latch (IC43 at 80188 PCS2 = 0xA0100): the last byte the Z80 strobed
 * across the J1 port. The Z80's port-0x81 bit-2 strobe latches the byte AND raises the
 * 80188 NMI; the NMI handler dmd_vblank_isr (D000:016D = IVT type 0x02) reads 0xA0100,
 * pushes the byte into the display command queue at 4000:1220 and sets the frame-pending
 * flag [4000:1147] that vsync_check (D000:5D1B) waits on -> locals.j1* */

static void sleic_oki_trigger(void) {
  UINT8 sample =  locals.okiLatch & 0x7f;              /* phrase number     */
  UINT8 voice  = (locals.okiLatch & 0x80) ? 0x1 : 0x2; /* ch A=v0 / ch B=v1 */
  locals.okiPending = 0;
  if (!sample) return;
#ifdef DEBUG_SLEIC
  if (getenv("SLEIC_TRACE_SND")) fprintf(stderr, "[oki] phrase %02x\n", sample);
#endif
  OKIM6376_data_0_w(0, 0x80 | sample); /* latch phrase number           */
  OKIM6376_data_0_w(0, voice << 4);    /* trigger playback on the voice */
}

static WRITE_HANDLER(sleic_periph_w) {
#ifdef DEBUG_SLEIC
  if (getenv("SLEIC_TRACE_PW")) fprintf(stderr, "[188->periph] PCS%d off=%03x data=%02x\n", offset>>7, offset, data);
#endif
  switch (offset) {
    case 0x280:                        /* PCS5: YM3812 port */
      /* Bike Race wires the YM3812 to the single address 0xA0280 and toggles A0 in hardware
       * per write, so it streams (register,value) pairs all to 0xA0280 */
      {
#ifdef DEBUG_SLEIC
        if (getenv("SLEIC_TRACE_SND")) fprintf(stderr, "[ym] %s %02x\n", locals.ymA0 ? "data":"reg ", data);
#endif
        if (locals.ymA0) YM3812_write_port_0_w(0, data); else YM3812_control_port_0_w(0, data);
        locals.ymA0 ^= 1;
      }
      return;
    case 0x300:                        /* PCS6: DMD enable + OKI ctrl latch */
      locals.okiLatch = data;
      if (data & 0x7f) locals.okiPending = 1; /* real phrase (not 0x80 DMD-enable) */
      break;
    case 0x000:                        /* PCS0: OKI /OKCS strobe (bit 4) */
      {
        /* /OKCS strobe: Bike Race pulses PCS0 bit 4 (0x10). (Exact decode awaits the IC7 PAL20L10 dump) */
        const UINT8 okcs = 0x10;
        if (locals.okiPending && (data & okcs) && !(locals.okiPrevStrobe & okcs))
          sleic_oki_trigger();
      }
      locals.okiPrevStrobe = data;
      break;
    case 0x080:                        /* PCS1: command byte the 80188 sends to the I/O side */
      /* Boot-init 3-gate handshake over the J1 queue:
       *   gate A: wait for 0x5F ("I/O ready", sent once by the Z80 at boot);
       *   gate B: send cmd 0xD4, wait for a byte >0xF0 (else trap "IMPOSIBLE SEGUIR");
       *   gate C: send cmd 0xD5, wait (no timeout) for ball-status 0x5B/5C/5D. */
      /* Bike Race: deliver the command to the real Z80 firmware (bkio07) and assert its NMI.
       * The Z80's NMI handler (0x0066) reads it via IN 0x00; cmd 0xD4 -> reply IN(0x04)|0xF0
       * (gate B), cmd 0xD5 -> reply ball-status 0x5B/5C/5D (gate C), via the Z80's normal
       * send path (port 0x80 + strobe -> 0xA0100) */
      locals.cmd188 = data;
      cpu_set_irq_line(SLEIC_IO_CPU, IRQ_LINE_NMI, PULSE_LINE);
      break;
    case 0x200:                        /* PCS4: DMD mode; bit 3 = frame swap */
      if (data & 0x08) {
        if (locals.dmdFromPtr)
          sleic_submit_dmd_frame();    /* Io Moon: buffer-swap ack is emitted by the PIC phase machine */
        else {                         /* I8039 machines: latch the finished 0x60410 frame */
          sleic3_build_dmd_frame(locals.dmdLatch);
          locals.dmdLatchTtl = SLEIC3_DMD_LATCH_TICKS;
        }
      }
      break;
    default:
      logerror("SLEIC periph A000:%03X = %02x\n", offset, data);
      break;
  }
}

static READ_HANDLER(sleic_periph_r) {
  if (offset == 0x100) {              /* PCS2: Z80->J1 inbound byte latch (IC43 74LS244) */
    /* Consume-on-read: return the fresh Z80 byte if one was strobed over J1, else the
     * idle value 0x37 (Bike Race's NMI treats 0x37 as "no event") */
    { UINT8 v;
      if (locals.j1Fresh) { locals.j1Fresh = 0; v = locals.j1Inbound; } /* a freshly strobed Z80 byte */
      else v = 0x37;                                                    /* idle: "no event" */
      return v;
    }
  }
  if (offset == 0x180)                /* PCS1: DMD controller status -- bit 0 = ready for a command */
    return 0x01;
  if (offset == 0x280)                /* PCS5: YM3812 status */
    return YM3812_status_port_0_r(0);
  return 0;                           /* PCS3 /OKBUSY etc. -- report not-busy */
}

/* handler called by the 3812 when the internal timers cause an IRQ */
static void ym3812_irq(int irq) {
//  cpu_set_irq_line(SLEIC_MAIN_CPU, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Interfaces*/
static struct YM3812interface SLEIC_ym3812_intf =
{
	1,					/* 1 chip */
	4000000,			/* 4 MHz */
	{ 100 },			/* volume */
	{ ym3812_irq },		/* IRQ Callback */
};
static struct OKIM6295interface SLEIC_okim6376_intf =
{
	0,					/* 1 chip (but use 0 to indicate 6376 chip) */
	{ 2000000./132. },	/* sampling frequency at 2MHz chip clock */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};
static struct OKIM6295interface SLEIC_okim6376_intf2 =
{
	0,					/* 1 chip (but use 0 to indicate 6376 chip) */
	{ 4000000./132. },	/* sampling frequency at 4MHz chip clock */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};
static struct DACinterface SLEIC_dac_intf = { 1, { 25 }};

static READ_HANDLER(read_0) {
  return 0;
}


static MEMORY_READ_START(SLEIC_80188_readmem)
  {0x00000,0x01fff, MRA_RAM},
  {0x10100,0x10900, read_0 /* MRA_RAM */},
  {0x60410,0x6340f, MRA_RAM},
  {0x80000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_80188_writemem)
  {0x00000,0x01fff,	MWA_RAM},
  {0x10100,0x10900, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xa0000,0xa07ff, pic_w},
  {0x60410,0x6340f, MWA_RAM},
MEMORY_END

/* Sleic Pin-Ball (SLEIC1) NVRAM. The base SLEIC map above maps NVRAM reads to
 * read_0 (returns 0) and writes to a separate generic_nvram buffer, so the
 * firmware's boot-time self-repair can never be read back -> the boot's signature
 * re-validation always fails -> "Memoria EEPROM en mal estado / Imposible Seguir".
 * (Boot: validate F000:80F5, on fail call factory-init F000:818D, re-validate,
 *  still-fail -> error F000:49E8 + halt)
 * Fix: one coherent buffer for both reads and writes, persisted by NVRAM_HANDLER.
 * The 80188 accesses NVRAM in segment 0x1000; the factory-init clears offset
 * 0x000-0xFFF and writes signature/config blocks, so map the full 28C64A-class
 * 8 KB window 0x10000-0x11FFF.  No embedded factory image is needed: on a fresh
 * boot the firmware seeds valid defaults itself, then core_nvram persists them */
#define SLEIC1_NVRAM_BASE 0x10000
#define SLEIC1_NVRAM_SIZE 0x2000
/* Not part of locals: NVRAM_HANDLER(SLEIC1) below fills this from the .nv file (or
 * zero-fills it) at every machine start, and it must survive locals' memset */
static UINT8 sleic1_nvram[SLEIC1_NVRAM_SIZE];
static READ_HANDLER(sleic1_nvram_r)  { return sleic1_nvram[offset]; }
static WRITE_HANDLER(sleic1_nvram_w) { sleic1_nvram[offset] = data; }
static NVRAM_HANDLER(SLEIC1) {
  core_nvram(file, read_or_write, sleic1_nvram, sizeof sleic1_nvram, 0x00);
}

/* Sleic Pin-Ball (SLEIC1) YM3812 A0 latch (PCS0 0xA0000 bit 1): 0 = register/index
 * port, 1 = data port.  sleicpin streams (register,value) pairs all to ONE address
 * 0xA0280 and selects register-vs-data via PCS0 bit 1 (set by the FM write primitive
 * at sp03 file 0x1E5E just before each 0xA0280 write) -- NOT 0x280/0x281 (IO Moon) and
 * NOT simple per-write alternation (Bike Race). Held in locals.ymA0 */

/* Sleic Pin-Ball (SLEIC1) 80188 peripheral write.  Two roles confirmed against sp03:
 *
 *  PCS1 (0xA0080) = reverse path: the command byte the 80188 sends to the Z80 I/O
 *    side.  It latches to Z80 port 0x00 and raises the Z80 NMI (handler 0x0066), whose
 *    buffered commands set the Z80's menu-mode flag [0xC051] (which gates the flipper
 *    nav codes in the service menu) AND the lamp/solenoid data.  Without this the Z80
 *    never learns it is in menu mode (flippers fire coils instead of scrolling) and
 *    gets no lamp data (dark matrix).
 *
 *  SOUND (sp03 0x1AEF/0x1E05): the 80188 drives BOTH chips.
 *    PCS5 (0xA0280)  = YM3812 register/data (A0 from PCS0 bit 1; see locals.ymA0).
 *    PCS6 (0xA0300)  = OKI MSM6376 phrase latch (the phrase number, sp03 0x1B36/0x1B47).
 *    PCS0 (0xA0000)  = shared control shadow [0x4da]: bit1 = YM3812 A0, bit4 (0x10) =
 *                      OKI /OKCS strobe (rising edge fires the latched phrase, sp03
 *                      0x1B6B), bit3 (0x08) = OKI channel.
 *
 * PCS4 (0xA0200) is the DMD frame strobe (shadow [0x4de]) -- left alone, since the
 * I8039 renders sleicpin's DMD from 0x60410 (calling sleic_submit_dmd_frame would corrupt it) */
static WRITE_HANDLER(sleic1_periph_w) {
  switch (offset) {
    case 0x080:                       /* PCS1: 80188 -> Z80 command (reverse path) */
      locals.cmd188 = data;
      cpu_set_irq_line(SLEIC_IO_CPU, IRQ_LINE_NMI, PULSE_LINE);
      return;
    case 0x280:                       /* PCS5: YM3812 (FM music) register or data */
#ifdef DEBUG_SLEIC
      if (getenv("SLEIC_TRACE_SND")) fprintf(stderr, "[ym] %s %02x\n", locals.ymA0 ? "data":"reg ", data);
#endif
      if (locals.ymA0) YM3812_write_port_0_w(0, data);
      else             YM3812_control_port_0_w(0, data);
      return;
    case 0x300:                       /* PCS6: OKI MSM6376 phrase latch */
      locals.okiLatch = data;
      if (data & 0x7f) locals.okiPending = 1;
      return;
    case 0x000:                       /* PCS0: shared control (bit1 YM A0, bit4 /OKCS) */
      locals.ymA0 = (data >> 1) & 1;
      if (locals.okiPending && (data & 0x10) && !(locals.okiPrevStrobe & 0x10)) {
        sleic_oki_trigger();
      }
      locals.okiPrevStrobe = data;
      return;
    default:                          /* PCS3 / PCS4 DMD strobe (I8039 renders) / etc. */
#ifdef DEBUG_SLEIC
      if (getenv("SLEIC_TRACE_PW")) fprintf(stderr, "[188->periph] PCS%d off=%03x data=%02x\n", offset>>7, offset, data);
#endif
      return;
  }
}

static MEMORY_READ_START(SLEIC1_80188_readmem)
  {0x00000,0x01fff, MRA_RAM},
  {SLEIC1_NVRAM_BASE, SLEIC1_NVRAM_BASE+SLEIC1_NVRAM_SIZE-1, sleic1_nvram_r}, /* 28C64A NVRAM (seg 0x1000) */
  {0xa0000,0xa0fff, sleic_periph_r},                                          /* PACS peripheral read; PCS2 0xA0100 = J1 inbound switch latch (NMI F000:DF20) */
  {0x60410,0x6340f, MRA_RAM},
  {0x80000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC1_80188_writemem)
  {0x00000,0x01fff, MWA_RAM},
  {SLEIC1_NVRAM_BASE, SLEIC1_NVRAM_BASE+SLEIC1_NVRAM_SIZE-1, sleic1_nvram_w}, /* 28C64A NVRAM (seg 0x1000) */
  {0xa0000,0xa0fff, sleic1_periph_w},                                         /* PACS write; PCS1 0xA0080 = 80188->Z80 cmd (Z80 NMI reverse path) */
  {0x60410,0x6340f, MWA_RAM},
MEMORY_END

/*----------------------------------------------------------------------------------
/  Io Moon (SLEIC2) 80188 memory map.
/
/  PinMAME's i86/i188 core does not emulate the 80188's internal peripheral control
/  block, so the firmware's OUT DX,AX writes to 0xFFA0.. are inert (they land in
/  i80188_write_port, a no-op) and the windows below are hard-wired to exactly the
/  values the boot table programs -- the 30-entry table at D000:0041, findings F1:
/
/    UMCS  FFA0 = C03C   0xC0000-0xFFFFF   ROM1 code half + the reset vector (FFFF0)
/    LMCS  FFA2 = 3FFC   0x00000-0x3FFFF   ROM1 low half: the IVT is RESIDENT IN ROM at
/                                          physical 0 (nothing copies it), followed by the
/                                          animation data the F5183 far-pointer table
/                                          addresses.  Not banked (F2)
/    PACS  FFA4 = A03C   0xA0000           peripheral block, PCS0..PCS6 on 0x80 spacing
/    MMCS  FFA6 = 41FC   mid-range memory based at 0x40000, and MPCS FFA8 = A0FC makes
/    MPCS  FFA8 = A0FC   that four 64 KB blocks MCS0-MCS3:
/                          MCS0 0x40000  work RAM (UM62256 IC12; stack SS:SP = 4152:0205)
/                          MCS1 0x50000  non-volatile store, window at 0x50400 (F10)
/                          MCS2 0x60000  ONE 64 KB page of the graphics ROM (F2)
/                          MCS3 0x70000  DMD staging buffer 7000:0000-03FF (F13)
/
/  Segments the firmware actually loads confirm the map is complete: 0000/1000/3000
/  (LMCS), 4000/4130/4134/4137/413C (MCS0), 5040 (MCS1), 7000 (MCS3), A000 (PACS) --
/  and 6000 never as an immediate, only through the far pointers F2 describes.
/---------------------------------------------------------------------------------*/

/* Io Moon non-volatile store (F10): a window at segment 5040 = flat 0x50400 inside
 * MCS1, gated by the complementary PCS0 bits 3/4 (pcs0_window_open D057E /
 * pcs0_window_close D059F).  Every access in the ROM is bracketed by that pair, so the
 * window is mapped unconditionally here -- modelling the gate could only ever turn a
 * correctly bracketed access into a lost one.  The part is inferred to be the 28C64A
 * (8 KB) on the board inventory; the firmware's highest offset is 0x31D, so 8 KB backs
 * everything it touches.  Zero-filled on a fresh boot: sub_D05C0 finds no signature,
 * sub_D622C writes the factory defaults, and core_nvram persists them from then on */
#define IOMOON_NVRAM_BASE 0x50400
#define IOMOON_NVRAM_SIZE 0x2000
static UINT8 iomoon_nvram[IOMOON_NVRAM_SIZE]; //!!
static READ_HANDLER(iomoon_nvram_r)  { return iomoon_nvram[offset]; }
static WRITE_HANDLER(iomoon_nvram_w) { iomoon_nvram[offset] = data; }
static NVRAM_HANDLER(SLEIC2) {
  core_nvram(file, read_or_write, iomoon_nvram, sizeof iomoon_nvram, 0x00);
}

/* Io Moon graphics bank (F2).  PCS0 bits 0-2 are a 3-bit page register (sub_F00A0 at
 * F00A0, shadow [4000:1134], 17 call sites all pushing an immediate 0..6) that pages
 * one 64 KB page of V1 3_02.bin into segment 6000.  All seven populated pages start
 * with the same 32-row / 16-byte / 0x200-stride header anim_stream_open reads, and
 * page 7 is blank, which is what makes page = file offset >> 16 the natural reading.
 *
 * That page->offset mapping is CONFIRMED; the BIT ORDER of the selector (whether bit 0
 * is A16 or A18) is INFERRED -- only the IC7 PAL20L10 or a scope can settle the wiring.
 * It therefore lives in this table: if the attract animations come out wrong, permuting
 * these seven bases is the one-line fix, and nothing else has to change */
#define IOMOON_GFX_BANK 1
static const UINT32 iomoon_gfx_page_base[8] = {
  0x00000, 0x10000, 0x20000, 0x30000, 0x40000, 0x50000, 0x60000,
  0x70000  /* page 7: blank in V1 3_02.bin and never selected */
};
static UINT8 iomoon_pcs0; /* PCS0 (0xA0000) output shadow; boot leaves it at 0x28 */ //!!

static void iomoon_set_gfx_bank(UINT8 pcs0) {
  cpu_setbank(IOMOON_GFX_BANK, memory_region(SLEIC_MEMREG_GFX) + iomoon_gfx_page_base[pcs0 & 0x07]);
}

/* Io Moon 80188 peripheral write.  Only PCS0's page-select side effect is wired up
 * here; the rest of the block (PCS1 J1 outbound, PCS2, PCS4, PCS5 YM3812, PCS6 OKI +
 * NVRAM gate) is the subject of the following driver work and is only logged for now */
static WRITE_HANDLER(sleic2_periph_w) {
  switch (offset) {
    case 0x000: /* PCS0: bits 0-2 graphics page (F2), 3/4 NVRAM window gate (F10), 5 = OKI /OKCS (F9) */
      if ((data ^ iomoon_pcs0) & 0x07) iomoon_set_gfx_bank(data);
      iomoon_pcs0 = data;
      return;
    default:
#ifdef DEBUG_SLEIC
      if (getenv("SLEIC_TRACE_PW")) fprintf(stderr, "[188->periph] PCS%d off=%03x data=%02x\n", offset>>7, offset, data);
#endif
      logerror("iomoon periph A000:%03X = %02x\n", offset, data);
      return;
  }
}

/* Io Moon 80188 peripheral read.  Nothing in the block is modelled yet: the J1 inbound
 * byte at PCS2 0xA0100 (F4/F6) and the YM3812 status at PCS5 0xA0280 (F8) come with the
 * link and sound work.  Deliberately NOT the base sleic_periph_r -- its 0x37 "no event"
 * idle and 0xA0180 ready bit are Bike Race conventions that do not apply here */
static READ_HANDLER(sleic2_periph_r) {
  return 0;
}

static MEMORY_READ_START(SLEIC2_80188_readmem)
  {0x00000,0x3ffff, MRA_ROM},         /* LMCS: ROM1 low half -- resident IVT + animation data */
  {0x40000,0x4ffff, MRA_RAM},         /* MCS0: work RAM (UM62256 IC12)                        */
  {IOMOON_NVRAM_BASE,IOMOON_NVRAM_BASE+IOMOON_NVRAM_SIZE-1, iomoon_nvram_r}, /* MCS1: seg 5040 store */
  {0x60000,0x6ffff, MRA_BANK1},       /* MCS2: graphics ROM page (IOMOON_GFX_BANK, PCS0 bits 0-2) */
  {0x70000,0x7ffff, MRA_RAM},         /* MCS3: DMD staging (7000:0000-03FF)                   */
  {0xa0000,0xa0fff, sleic2_periph_r}, /* PACS: PCS0-PCS6                                      */
  {0xc0000,0xfffff, MRA_ROM},         /* UMCS: ROM1 code half + reset vector                  */
MEMORY_END

static MEMORY_WRITE_START(SLEIC2_80188_writemem)
  {0x40000,0x4ffff, MWA_RAM},         /* MCS0: work RAM                                       */
  {IOMOON_NVRAM_BASE,IOMOON_NVRAM_BASE+IOMOON_NVRAM_SIZE-1, iomoon_nvram_w}, /* MCS1: seg 5040 store */
  {0x70000,0x7ffff, MWA_RAM},         /* MCS3: DMD staging                                    */
  {0xa0000,0xa0fff, sleic2_periph_w}, /* PACS: PCS0-PCS6                                      */
  /* 0x00000-0x3FFFF and 0x60000-0x6FFFF are ROM and are deliberately left unmapped for
   * writes: no instruction in the decoded ROM writes either window (F2), so a write
   * showing up there is a bug worth seeing in the log rather than silently absorbing */
MEMORY_END

/* Bike Race (SLEIC3) 80188 map. MMCS=0x01FF / MPCS=0xC0FC decode a 512 KB
 * mid-range block based at 0, split into four 128 KB chip-selects MCS0-3:
 *   MCS0 0x00000-0x1FFFF : work RAM (boot stack 012F:0203; the boot copies its IVT
 *                          image from CS:00C4 to physical 0 before STI, so the
 *                          driver must NOT overwrite the IVT)
 *   MCS1 0x20000-0x3FFFF : graphics ROM bkcpu05
 *   MCS2 0x40000-0x5FFFF : graphics ROM bkcpu06 (read via ES=0x5000)
 *   MCS3 0x60000-0x7FFFF : DMD / video frame buffer RAM (panel staging at 0x60410)
 * PACS=0xA03C -> peripheral block at 0xA0000, so sleic_periph_r/w are used.
 * UMCS -> bkcpu04 code at 0xE0000-0xFFFFF (reset EA F000:0000). */

/* Bike Race NVRAM (28C64A, 8 KB at seg 0x1040): read/written through a dedicated buffer
 * that NVRAM_HANDLER(SLEIC3) persists via core_nvram, which zero-fills it on a fresh boot
 * exactly like every other driver here.
 *
 * Nothing seeds a factory image.  On a blank NVRAM the firmware's own reset routine
 * (bkcpu04 E97DB, reached at E520C) puts up "ESTABLECIENDO VALORES FABRICA / PULSE START"
 * and waits (E9425/E9478) for an operator START press (J1 event 0x36), then writes the
 * coin/credit (0x230-0x23B), high-score and audit tables itself (E9805+).  That is what a
 * real machine does with a new battery-backed chip, so it is what the driver does: press
 * START once at the prompt and the game seeds itself, persisting to the .nv from then on.
 * (Sleic Pin-Ball differs -- its boot repairs a blank NVRAM silently at F000:818D) */
/* Not part of locals, same as sleic1_nvram above: NVRAM_HANDLER(SLEIC3) fills this
 * at every machine start and it must survive locals' memset */
static UINT8 sleic3_nvram[0x2000];
static READ_HANDLER(sleic3_nvram_r)  { return sleic3_nvram[offset]; }
static WRITE_HANDLER(sleic3_nvram_w) { sleic3_nvram[offset] = data; }
static NVRAM_HANDLER(SLEIC3) {
  core_nvram(file, read_or_write, sleic3_nvram, sizeof sleic3_nvram, 0x00);
}

static MEMORY_READ_START(SLEIC3_80188_readmem)
  {0x00000,0x103ff, MRA_RAM},        /* MCS0: work RAM (IVT@0, stack, data)      */
  {0x10400,0x123ff, sleic3_nvram_r}, /* MCS0: 28C64A 8KB NVRAM (persisted by NVRAM_HANDLER(SLEIC3)) */
  {0x12400,0x1ffff, MRA_RAM},        /* MCS0: work RAM (rest)                    */
  {0x20000,0x5ffff, MRA_ROM},        /* MCS1/MCS2: graphics ROM (bkcpu06/05)     */
  {0x60000,0x7ffff, MRA_RAM},        /* MCS3: DMD / video frame buffer (0x60410) */
  {0xa0000,0xa0fff, sleic_periph_r}, /* PACS peripheral block: J1+OKI+YM3812     */
  {0xe0000,0xfffff, MRA_ROM},        /* bkcpu04 game/sound code (E000/F000)      */
MEMORY_END

static MEMORY_WRITE_START(SLEIC3_80188_writemem)
  {0x00000,0x103ff, MWA_RAM},        /* MCS0: work RAM (IVT@0, stack, data)  */
  {0x10400,0x123ff, sleic3_nvram_w}, /* MCS0: 28C64A 8KB NVRAM (seg 0x1040)  */
  {0x12400,0x1ffff, MWA_RAM},        /* MCS0: work RAM (rest)                */
  {0x60000,0x7ffff, MWA_RAM},        /* MCS3: DMD / video frame buffer       */
  {0xa0000,0xa0fff, sleic_periph_w}, /* PACS peripheral block: J1+OKI+YM3812 */
MEMORY_END

static MEMORY_READ_START(SLEIC_8039_readmem)
  {0x0000,0x3fff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_8039_writemem)
MEMORY_END

static MEMORY_READ_START(SLEIC_Z80_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0xc000,0xc7ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_Z80_writemem)
  {0xc000,0xc7ff, MWA_RAM},
MEMORY_END

static WRITE_HANDLER(i80188_write_port) {
  /* 80188 internal Peripheral Control Block writes (chip-selects at 0xFFA0+, timer/
   * interrupt-controller config, EOI at 0xFF2C). The I188 core does not model these;
   * they are no-ops here. The interrupt sources are generated by sleic3_irq_gen */
}

static READ_HANDLER(i8039_read_test) {
//  logerror("8039 read port T1\n");
  return 0;
}

static WRITE_HANDLER(i8039_write_port) {
/*
  static UINT8 pos = 1; //!!
  UINT8 *line;
  if (!offset)
    pos = data;
  else {
    line = &dotCol[1+(pos >> 4)][8*(pos & 0x0f)];
    *line++ = data & 0x80 ? 3 : 0;
    *line++ = data & 0x40 ? 3 : 0;
    *line++ = data & 0x20 ? 3 : 0;
    *line++ = data & 0x10 ? 3 : 0;
    *line++ = data & 0x08 ? 3 : 0;
    *line++ = data & 0x04 ? 3 : 0;
    *line++ = data & 0x02 ? 3 : 0;
    *line++ = data & 0x01 ? 3 : 0;
  }
*/
  logerror("8039 write port P%d = %02x\n", offset+1, data);
}

static READ_HANDLER(z80_read_port) {
  switch (offset) {
    case 1: return core_getDip(0);
    case 2: return ~coreGlobals.swMatrix[1 + locals.swCol];
    case 3: return ~coreGlobals.tmpLampMatrix[locals.lampCol];
    case 4: return coreGlobals.swMatrix[0];
    default: logerror("Z80 read port %02x\n", offset);
  }
  return 0;
}

static WRITE_HANDLER(z80_write_port) {
  switch (offset) {
    case 1: coreGlobals.diagnosticLed = data >> 3; break;
    case 2: locals.swCol = core_BitColToNum(data); break;
    case 3: locals.lampCol = core_BitColToNum(data); break;
    case 4: coreGlobals.tmpLampMatrix[locals.lampCol] = data; break;
    case 5: locals.solenoids = (locals.solenoids & 0xff00ff) | ((data ^ 0xff) << 8); break;
    case 6: locals.solenoids = (locals.solenoids & 0xffff00) | (data ^ 0xff); break;
    case 7: break;
    default: logerror("Z80 write port %2x = %02x\n", 0x80 + offset, data);
  }
}

/* Z80 I/O processor state shadow (Bike Race port handlers below) */
static struct {
  UINT8 swStrobe; /* port 0x82: switch matrix column strobe (0..5 after decode) */
  UINT8 lampCol;  /* lamp matrix column (after decode) */
  UINT8 lampRow;  /* port 0x84: lamp row byte, latched until the 0x83 column strobe */
  UINT8 ctrl;     /* port 0x81: control register shadow */
  UINT8 sndData;  /* port 0x80: last byte written */
} sleic_io;

/* Bike Race (SLEIC3) Z80 I/O processor ports.  Port map from the bkio07
 * disassembly.  Three boot-critical details: the J1 status bit is port-0x01
 * *bit 5*; port 0x04 *bit 7* must read 1 or main_init diverts to the service
 * loop at Z80 0x29AE and never announces readiness; the switch-matrix column
 * strobe is port 0x82 (lamp rows on 0x83/0x84, the IRQ-timed row strobe on
 * 0x87).  At boot (Z80 0x012A) the Z80 sends an 0x5F "I/O board ready" byte
 * over J1, which is what the 80188's "ESPERANDO" (waiting) attract poll is waiting to receive */
static READ_HANDLER(sleic3_z80_read) {
  switch (offset) {
    case 0x00:                                                      /* J1 inbound: 80188 command byte (NMI reads it) */
      return locals.cmd188;
    case 0x01: return 0x20;                                         /* status: bit 5 = J1 "80188 ready" (bkio07 polls bit 5); always-ready */
    case 0x02: return ~coreGlobals.swMatrix[1 + sleic_io.swStrobe]; /* switch-matrix column return (active-low)  */
    case 0x03: return ~coreGlobals.swMatrix[9];                     /* all 6 direct/cabinet buttons (active-low); see SWITCH_UPDATE */
    case 0x04: return 0xff;                                         /* idle: bit7=1 normal boot, bit4=1 trough-query enable      */
    default:   logerror("bikerace Z80 read port %02x\n", offset);
  }
  return 0;
}

static WRITE_HANDLER(sleic3_z80_write) {
  switch (offset) {
    case 0x00:  /* port 0x80: byte onto the J1 data lines toward the 80188 */
      sleic_io.sndData = data;
      locals.j1Inbound = data;
      break;
    case 0x01:  /* port 0x81: control; bit-2 rising edge latches the J1 byte into 80188 PCS2 (0xA0100) */
      if ((data & 0x04) && !(locals.j1PrevCtrl & 0x04)) {
        locals.j1Inbound = sleic_io.sndData;
        locals.j1Fresh = 1;
        /* The port-0x81 bit-2 strobe latches the byte into PCS2 (0xA0100) AND raises the
         * 80188 NMI. The NMI handler (E000:0272) consumes exactly ONE J1 byte per assertion,
         * so the NMI MUST be strobe-driven here, not periodic (see sleic3_irq_gen) */
        cpu_set_irq_line(SLEIC_MAIN_CPU, IRQ_LINE_NMI, PULSE_LINE);
      }
      locals.j1PrevCtrl = data;
      sleic_io.ctrl = data;
      coreGlobals.diagnosticLed = (data >> 4) & 1;
      break;
    case 0x02:  /* port 0x82: switch-matrix column strobe (one-hot) */
      if (data) sleic_io.swStrobe = core_BitColToNum(data & -data);
      break;
    case 0x03:  /* port 0x83: lamp-matrix COLUMN strobe (one-hot 0x01..0x80 = COL0..COL7).
                 * The lamp refresh writes the row byte to 0x84 first, then pulses the column
                 * here, so commit on this strobe with the row from the preceding 0x84. 8x8 = 64 */
      if (data) coreGlobals.tmpLampMatrix[core_BitColToNum(data & -data)] = sleic_io.lampRow;
      break;
    case 0x04:  /* port 0x84: lamp-matrix ROW data (bit b = FILA b); latched, committed on 0x83 */
      sleic_io.lampRow = data;
      break;
    case 0x05:  /* port 0x85: solenoid bank 1 (active-low) */
      /* Write the driver-local shadow, not coreGlobals.solenoids: the VBLANK handler
       * (SLEIC_interface_update) copies locals.solenoids -> coreGlobals.solenoids each frame */
      locals.solenoids = (locals.solenoids & 0xffff00) | (data ^ 0xff);
      break;
    case 0x06:  /* port 0x86: solenoid bank 2 (active-low) */
      locals.solenoids = (locals.solenoids & 0xff00ff) | ((data ^ 0xff) << 8);
      break;
    case 0x07:  /* port 0x87: switch-matrix row strobe / Z80 control bits (NOT the lamp column).
                 * Switches are read via the 0x82 column strobe + port-0x02 data, so no action here */
      break;
    default:
      logerror("bikerace Z80 write port %02x = %02x\n", 0x80 + offset, data);
  }
}

static PORT_READ_START(SLEIC_80188_readport)
MEMORY_END

static PORT_WRITE_START(SLEIC_80188_writeport)
  {0xff00,0xffff, i80188_write_port},
MEMORY_END

static PORT_READ_START(SLEIC_8039_readport)
  {I8039_t1,I8039_t1, i8039_read_test},
MEMORY_END

static PORT_WRITE_START(SLEIC_8039_writeport)
  {I8039_p1,I8039_p2, i8039_write_port},
MEMORY_END

static PORT_READ_START(SLEIC_Z80_readport)
  {0x00,0x07, z80_read_port},
MEMORY_END

static PORT_WRITE_START(SLEIC_Z80_writeport)
  {0x80,0x87, z80_write_port},
MEMORY_END

static PORT_READ_START(SLEIC3_Z80_readport)
  {0x00,0x07, sleic3_z80_read},
MEMORY_END

static PORT_WRITE_START(SLEIC3_Z80_writeport)
  {0x80,0x87, sleic3_z80_write},
MEMORY_END

/* Sleic Pin-Ball (SLEIC1) Z80 I/O processor ports.  Verified against the sp04
 * disassembly: identical I/O conventions to Bike Race.  Switch matrix is 8 comun
 * (port-0x82 one-hot strobe) x 4 retorno (port-0x02 read, active-low/CPL'd); the
 * Z80 sends each switch code over J1 via port 0x80 + a port-0x81 bit-2 strobe
 * (sub_082a: out 0x80; (0x81|0x04); 0x81) which latches the byte at the 80188 PCS2
 * (0xA0100) and raises the 80188 NMI.  Boot gate (Z80 0x1744) spins until port-0x04
 * bit 7 = 1; port-0x01 bit 5 is the J1 ready status */
static READ_HANDLER(sleic1_z80_read) {
  switch (offset) {
    case 0x00: return locals.cmd188;                                /* J1 inbound: 80188->Z80 cmd (Z80 NMI reads it) */
    case 0x01: return 0x20;                                         /* status: bit 5 = J1 ready (sp04 0x03f6 tests bit 5) */
    case 0x02: return ~coreGlobals.swMatrix[1 + sleic_io.swStrobe]; /* matrix retorno data for the selected comun (active-low) */
    case 0x03: return ~coreGlobals.swMatrix[9];                     /* direct/cabinet buttons C31-C36 (active-low, CPL'd at 0x1757) */
    case 0x04: return 0xff;                                         /* bit 7 = 1 (boot gate 0x1744), bit 0 = 1 */
    default:   logerror("sleicpin Z80 read port %02x\n", offset);
  }
  return 0;
}

static WRITE_HANDLER(sleic1_z80_write) {
  switch (offset) {
    case 0x00: /* port 0x80: byte onto the J1 data lines toward the 80188 */
      sleic_io.sndData = data;
      locals.j1Inbound = data;
      break;
    case 0x01: /* port 0x81: control; bit-2 rising edge latches the J1 byte into 80188 PCS2 (0xA0100) and raises the NMI */
      if ((data & 0x04) && !(locals.j1PrevCtrl & 0x04)) {
        locals.j1Inbound = sleic_io.sndData;
        locals.j1Fresh = 1;
#ifdef DEBUG_SLEIC
        if (getenv("SLEIC_TRACE_SW")) fprintf(stderr, "[SW->188] code=%02x\n", locals.j1Inbound);
#endif
        cpu_set_irq_line(SLEIC_MAIN_CPU, IRQ_LINE_NMI, PULSE_LINE); /* NMI handler F000:DF20 reads 0xA0100 */
      }
      locals.j1PrevCtrl = data;
      sleic_io.ctrl = data;
      coreGlobals.diagnosticLed = (data >> 4) & 1;                  /* bit 4 = NMI-ack / diag LED */
      break;
    case 0x02: /* port 0x82: switch-matrix comun strobe (one-hot 0x01..0x80 = comun 0..7) */
      if (data) sleic_io.swStrobe = core_BitColToNum(data & -data);
      break;
    case 0x03: /* port 0x83: lamp-matrix column strobe; commit the row byte from the preceding 0x84 */
      if (data) {
#ifdef DEBUG_SLEIC
        if (getenv("SLEIC_TRACE_LAMP") && sleic_io.lampRow) fprintf(stderr, "[lamp] col=%d row=%02x\n", core_BitColToNum(data & -data), sleic_io.lampRow);
#endif
        coreGlobals.tmpLampMatrix[core_BitColToNum(data & -data)] = sleic_io.lampRow;
      }
      break;
    case 0x04: /* port 0x84: lamp-matrix row data; latched, committed on the 0x83 strobe */
      sleic_io.lampRow = data;
      break;
    case 0x05: /* port 0x85: flipper coil windings (active-low, fired bit-cleared).
                * Verified vs sp04: bit0=bobina 01 Flipper Izq Fuerza (sub_024a),
                * bit1=02 Flipper Izq Mantenimiento, bit2=03 Flipper Der Fuerza (sub_0266),
                * bit3=04 Flipper Der Mantenimiento; bits 4-7 unused.  -> solenoids 1-4 */
      locals.solenoids = (locals.solenoids & ~(UINT32)0x00f) | ((UINT32)(data ^ 0xff) & 0x0f);
      break;
    case 0x06: /* port 0x86: playfield coils (active-low).  Verified vs sp04 + the service
                * manual BOBINAS list (FIGURA 4): bit0=05 Bancada Izquierda, bit1=06 Bancada
                * Derecha, bit2=07 Bumper Izquierdo, bit3=08 Bumper Derecho, bit4=09 Expulsor
                * Izquierdo, bit5=10 Expulsor Derecho, bit6=11 Salida Bolas, bit7=12 Taca.
                * Mapped to solenoids 5-12 so PinMAME sol# == the manual's bobina #
                * (Bike Race used bits 8-15; sleicpin's 12-coil layout differs) */
      locals.solenoids = (locals.solenoids & ~(UINT32)0xff0) | (((UINT32)(data ^ 0xff) & 0xff) << 4);
      break;
    case 0x07: /* port 0x87: VDB (coil-current watchdog) scan / Z80 control bits; not a coil
                * output and not the matrix column (matrix uses 0x82) */
      break;
    default:
      logerror("sleicpin Z80 write port %02x = %02x\n", 0x80 + offset, data);
  }
}

static PORT_READ_START(SLEIC1_Z80_readport)
  {0x00,0x07, sleic1_z80_read},
MEMORY_END

static PORT_WRITE_START(SLEIC1_Z80_writeport)
  {0x80,0x87, sleic1_z80_write},
MEMORY_END

static MACHINE_INIT(SLEIC) {
  /* The memset covers everything in locals -- the DMD latch and its TTL, the OKI
   * phrase/strobe state, the J1 link latches, the YM3812 A0 select and the SLEIC3
   * interrupt accumulators all start at zero on every machine start */
  memset(&locals, 0, sizeof locals);
  memset(&sleic_io, 0, sizeof sleic_io);
  /* Only Io Moon lacks the I8039 display CPU, and only Io Moon uses the 4000:1150 display-pointer submit on the PCS4 bit-3 strobe */
  locals.dmdFromPtr = (Machine->drv->cpu[SLEIC_DISPLAY_CPU].cpu_type == CPU_DUMMY);
  /* locals.dmdEqualFields stays 0 here (Bike Race weighting); SLEIC1 overrides it below */
  core_dmd_pwm_init(core_gameData->lcdLayout, CORE_DMD_PWM_PREINTEGRATED_LINEAR_4, CORE_DMD_PWM_PREINTEGRATED_LINEAR_4, 0);
  /* Ball trough / ball-detect optos live on matrix COL4 (swMatrix[5]). The Z80 cmd-0xD5
   * ball-status query strobes COL4 (out 0x82 = 0x10), reads it into 0xC0DB and replies
   * over J1. The set of monitored optos and reply codes DIFFERS BY VERSION -- the Z80 I/O
   * ROM (bkio07.bin vs 07.bin) is one of the two ROMs that differ between the games:
   *   bikerace (bkio07, handler 0x0B1D -> sub 0x0B31, replies 0x0B7C/0x0B9D): monitors
   *     COL4 bits 0x04 (code 0x2C, key 3), 0x20 (C7 "Bola Retenida", key 8) and
   *     0x80 (C8 "Bola fuera", key 9). Reply 0x5D "BOLAS OK" / 0x5B "FALTA 1 BOLA";
   *     a ball at C7 OR C8 -> BOLAS OK, so standalone-test by holding key 8 (or 9).
   *   bikerac2 (07.bin, sub 0x0B72, replies 0x0C0B/0x0C16/0x0C37): monitors the same three
   *     PLUS bit 0x40 (code 0x30, key 7) and counts missing balls -- reply adds
   *     0x5C "FALTA 2 BOLAS". "BOLAS OK" needs two optos INCLUDING the key-7 sensor
   *     (combos 0x20+0x40 or 0x40+0x80), so standalone-test by holding key 7 with 8 or 9;
   *     holding only 8+9 (no 7) never reports OK. (Verified against the disassembly of both
   *     Z80 ROMs; the key bindings already exist in sleic3_pf_keys below.)
   * The driver does not fabricate the ball complement -- the frontend (e.g. VPX)
   * supplies trough state; the keys above are only for standalone testing */
}

/* Sleic Pin-Ball's display ROM lights both raster fields for the same time, so its panel
 * has three levels rather than four -- see locals.dmdEqualFields above */
static MACHINE_INIT(SLEIC1) {
  machine_init_SLEIC();
  locals.dmdEqualFields = 1;
}

/* Io Moon: point the segment-6000 graphics bank somewhere valid before the first
 * instruction runs.  0x28 is the value boot_init leaves in the PCS0 shadow [4000:1134]
 * (bit 3 = NVRAM window closed, bit 5 = OKI /OKCS idle high, page bits clear) */
static MACHINE_INIT(SLEIC2) {
  machine_init_SLEIC();
  iomoon_pcs0 = 0x28;
  iomoon_set_gfx_bank(iomoon_pcs0);
}

/* Bike Race (SLEIC3) playfield-matrix test keys. The 40 matrix positions (Z80 switch
 * codes 0x0A-0x31) are read by the 80188 through swMatrix[1..5] (COL0..COL4, selected by
 * the port-0x82 one-hot column strobe); code = 0x0A + 8*(col-1) + row. Mapping every
 * position to a key lets the CONTACTOS self-test verify each contact. COL4 (swMatrix[5])
 * is the trough column. The Z80 cmd-0xD5 ball-status handler monitors COL4 bits 0x04
 * (code 0x2C, key 3), 0x20 = C7 (key 8) and 0x80 = C8 (key 9) on BOTH versions, plus
 * bit 0x40 (code 0x30, key 7) on bikerac2 only; see the per-version breakdown in the
 * MACHINE_INIT trough comment above */
static const struct { int key; UINT8 col; UINT8 bit; } sleic3_pf_keys[] = {
  {KEYCODE_Q,1,0x01},{KEYCODE_W,1,0x02},{KEYCODE_E,1,0x04},{KEYCODE_R,1,0x08}, /* COL0 0x0A-0x0D */
  {KEYCODE_Y,1,0x10},{KEYCODE_U,1,0x20},{KEYCODE_I,1,0x40},{KEYCODE_O,1,0x80}, /* COL0 0x0E-0x11 */
  {KEYCODE_A,2,0x01},{KEYCODE_S,2,0x02},{KEYCODE_D,2,0x04},{KEYCODE_F,2,0x08}, /* COL1 0x12-0x15 */
  {KEYCODE_G,2,0x10},{KEYCODE_H,2,0x20},{KEYCODE_J,2,0x40},{KEYCODE_K,2,0x80}, /* COL1 0x16-0x19 */
  {KEYCODE_Z,3,0x01},{KEYCODE_X,3,0x02},{KEYCODE_C,3,0x04},{KEYCODE_V,3,0x08}, /* COL2 0x1A-0x1D */
  {KEYCODE_B,3,0x10},{KEYCODE_N,3,0x20},{KEYCODE_M,3,0x40},{KEYCODE_L,3,0x80}, /* COL2 0x1E-0x21 */
  {KEYCODE_0_PAD,4,0x01},{KEYCODE_1_PAD,4,0x02},{KEYCODE_2_PAD,4,0x04},{KEYCODE_3_PAD,4,0x08}, /* COL3 0x22-0x25 */
  {KEYCODE_4_PAD,4,0x10},{KEYCODE_5_PAD,4,0x20},{KEYCODE_6_PAD,4,0x40},{KEYCODE_7_PAD,4,0x80}, /* COL3 0x26-0x29 */
  {KEYCODE_0,5,0x01},{KEYCODE_2,5,0x02},{KEYCODE_3,5,0x04},{KEYCODE_4,5,0x08}, /* COL4 0x2A-0x2D */
  {KEYCODE_6,5,0x10},{KEYCODE_8,5,0x20},{KEYCODE_7,5,0x40},{KEYCODE_9,5,0x80}, /* COL4 0x2E; trough optos: 0x2F=C7(key8) 0x31=C8(key9) both versions, 0x30=key7 bikerac2-only (+0x2C=key3); see trough notes above */
};

static SWITCH_UPDATE(SLEIC3) {
  unsigned i;
  if (inports) {
    /* Cabinet/direct buttons are all on Z80 port 0x03 (swMatrix[9]), bit -> contact:
     *   bit0 = C17 Tilt        (code 0x32, also menu ENTER)
     *   bit1 = C4  Test        (code 0x33 = menu ENTER)
     *   bit2 = C5  Right flipper(code 0x34, menu SELECT)
     *   bit3 = C1  Left flipper (code 0x35, menu scroll DOWN)
     *   bit4 = C2  Start        (code 0x36)
     *   bit5 = C3  Coin/Monedero(code 0x37/0x39) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x01, 9); /* TILT(T)   0x400 -> bit0 (C17 tilt, code 0x32) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x02, 9); /* TEST(End) 0x800 -> bit1 (C4 Test, code 0x33 = menu ENTER) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] << 1,  0x04, 9); /* R-Shift 0x002 -> bit2 (C5 right flipper) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] << 3,  0x08, 9); /* L-Shift 0x001 -> bit3 (C1 left flipper)  */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 4,  0x10, 9); /* START 0x100 -> bit4 (C2) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 4,  0x20, 9); /* COIN  0x200 -> bit5 (C3) */
  }
  /* playfield matrix: closed (bit set) while the key is held */
  for (i = 0; i < sizeof(sleic3_pf_keys)/sizeof(sleic3_pf_keys[0]); i++) {
    if (keyboard_pressed(sleic3_pf_keys[i].key))
      coreGlobals.swMatrix[sleic3_pf_keys[i].col] |=  sleic3_pf_keys[i].bit;
    else
      coreGlobals.swMatrix[sleic3_pf_keys[i].col] &= ~sleic3_pf_keys[i].bit;
  }
#ifdef DEBUG_SLEIC
  sleic_debug_switches(5, 0xE0); /* COL4: C7 0x20 | key-7 sensor 0x40 (bikerac2) | C8 0x80 */
#endif
}

/* Sleic Pin-Ball (SLEIC1) playfield-matrix test keys.  The 8 comun x 4 retorno
 * matrix (FIGURA 24 of the service manual) -> swMatrix[1..8] (comun 0..7), bit r = retorno r.
 * Mapping each populated position to a key lets the CONTACTOS self-test verify every contact */
static const struct { int key; UINT8 col; UINT8 bit; } sleic1_pf_keys[] = {
  {KEYCODE_Q,1,0x01},{KEYCODE_W,1,0x02},{KEYCODE_E,1,0x04},{KEYCODE_R,1,0x08}, /* comun0: C1, C28,C29,C6  */
  {KEYCODE_A,2,0x01},{KEYCODE_S,2,0x02},{KEYCODE_D,2,0x04},{KEYCODE_F,2,0x08}, /* comun1: C2, C23,C27,C18 */
  {KEYCODE_Z,3,0x01},{KEYCODE_X,3,0x02},{KEYCODE_C,3,0x04},{KEYCODE_V,3,0x08}, /* comun2: C14,C24,C10,C15 */
  {KEYCODE_Y,4,0x01},{KEYCODE_U,4,0x02},{KEYCODE_I,4,0x04},{KEYCODE_O,4,0x08}, /* comun3: C17,C25,C11,C13 */
  {KEYCODE_G,5,0x01},{KEYCODE_H,5,0x02},{KEYCODE_J,5,0x04},{KEYCODE_K,5,0x08}, /* comun4: C16,C7, C19,C30 */
                     {KEYCODE_B,6,0x02},{KEYCODE_N,6,0x04},{KEYCODE_M,6,0x08}, /* comun5: C8, C20,C3      */
                     {KEYCODE_1_PAD,7,0x02},{KEYCODE_2_PAD,7,0x04},{KEYCODE_3_PAD,7,0x08}, /* comun6: C9, C21,C4 */
                     {KEYCODE_4_PAD,8,0x02},{KEYCODE_5_PAD,8,0x04},{KEYCODE_6_PAD,8,0x08}, /* comun7: C26,C22,C5 */
};

static SWITCH_UPDATE(SLEIC1) {
  unsigned i;
  if (inports) {
    /* Cabinet/direct buttons on Z80 port 0x03 (swMatrix[9]).  port-0x03 bit -> code ->
     * contact CONFIRMED against the sp04 cabinet dispatcher (sub_0978/sub_09fe..0a70)
     * and the sp03 code handlers:
     *   bit0 -> code 0x01 = C35 Pendulo de Falta (tilt): handler acts only in-game
     *           ([0x103]!=0) with a warning counter.
     *   bit1 -> code 0x02 = C36 Pulsador de Test: handler enters the service menu
     *           (F000:567F sets the [0x4d1] menu-active flag).
     *   bit2 -> code 0x03 = C32 Flipper Derecho: fires coil 03 (Flipper Der Fuerza).
     *   bit3 -> code 0x04 = C31 Flipper Izquierdo: fires coil 01 (Flipper Izq Fuerza).
     *   bit4 -> code 0x05 = C33 Pulsador Start (start key '1').
     *   bit5 -> code 0x06 = C34 Monedero / coin (coin key '5').
     * (Flippers also fire their coils in real time; codes 0x03/0x04 are only sent in
     * menu mode for navigation) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x01, 9); /* TILT  -> bit0 (C35 Falta)       */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x02, 9); /* TEST  -> bit1 (C36 Test)        */
    CORE_SETKEYSW(inports[CORE_COREINPORT] << 1,  0x04, 9); /* R-flip-> bit2 (C32 Flipper Der) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] << 3,  0x08, 9); /* L-flip-> bit3 (C31 Flipper Izq) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 4,  0x10, 9); /* START -> bit4 (C33 Start)       */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 4,  0x20, 9); /* COIN  -> bit5 (C34 Monedero)    */
  }
  for (i = 0; i < sizeof(sleic1_pf_keys)/sizeof(sleic1_pf_keys[0]); i++) {
    if (keyboard_pressed(sleic1_pf_keys[i].key))
      coreGlobals.swMatrix[sleic1_pf_keys[i].col] |=  sleic1_pf_keys[i].bit;
    else
      coreGlobals.swMatrix[sleic1_pf_keys[i].col] &= ~sleic1_pf_keys[i].bit;
  }
#ifdef DEBUG_SLEIC
  sleic_debug_switches(1, 0x04); /* comun0 bit2 = C29 Salida Bolas (ball trough) */
#endif
}

static MACHINE_DRIVER_START(SLEIC)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(SLEIC,NULL,NULL)
  MDRV_SWITCH_CONV(SLEIC_sw2m,SLEIC_m2sw)
  MDRV_SWITCH_UPDATE(SLEIC)
  MDRV_DIPS(8)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_NVRAM_HANDLER(generic_0fill)

  // game & sound section
  MDRV_CPU_ADD_TAG("mcpu", I188, 8000000)
  MDRV_CPU_MEMORY(SLEIC_80188_readmem, SLEIC_80188_writemem)
  MDRV_CPU_PORTS(SLEIC_80188_readport, SLEIC_80188_writeport)
  MDRV_CPU_VBLANK_INT(SLEIC_interface_update, 1)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i80188, 120)

  // I/O section
  MDRV_CPU_ADD_TAG("icpu", Z80, 2500000)
  MDRV_CPU_MEMORY(SLEIC_Z80_readmem, SLEIC_Z80_writemem)
  MDRV_CPU_PORTS(SLEIC_Z80_readport, SLEIC_Z80_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_z80, 2500000/2048.)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC1)
  MDRV_IMPORT_FROM(SLEIC)
  MDRV_CORE_INIT_RESET_STOP(SLEIC1,NULL,NULL)

  // Sleic Pin-Ball cabinet/direct switches (C31-C36) -> swMatrix[9]; playfield
  // matrix via sleic1_pf_keys -> swMatrix[1..8]
  MDRV_SWITCH_UPDATE(SLEIC1)

  // Battery-backed NVRAM (28C64A): persisted by NVRAM_HANDLER(SLEIC1) and mapped
  // read==write to one buffer so the firmware's boot-time self-repair sticks and
  // the signature re-validation passes (clears "Memoria EEPROM en mal estado").
  // REPLACE wipes the inherited mcpu map/ports/IRQ, so all are re-stated; only the
  // 80188 memory map changes vs. the base SLEIC (SLEIC2/iomoon keep the base map)
  MDRV_NVRAM_HANDLER(SLEIC1)
  MDRV_CPU_REPLACE("mcpu", I188, 8000000)
  MDRV_CPU_MEMORY(SLEIC1_80188_readmem, SLEIC1_80188_writemem)
  MDRV_CPU_PORTS(SLEIC_80188_readport, SLEIC_80188_writeport)
  MDRV_CPU_VBLANK_INT(SLEIC_interface_update, 1)
  MDRV_CPU_PERIODIC_INT(sleic1_irq_gen, 122) // internal Timer0 (vector 0x08) ~122 Hz

  // I/O Z80: drive the switch matrix / cabinet / lamps / solenoids and the J1
  // byte-port (port-0x81 bit-2 strobe latches a switch byte + raises the 80188 NMI)
  MDRV_CPU_MODIFY("icpu")
  MDRV_CPU_PORTS(SLEIC1_Z80_readport, SLEIC1_Z80_writeport)

  // display section
  MDRV_CPU_ADD_TAG("dcpu", I8039, 2000000)
  MDRV_CPU_MEMORY(SLEIC_8039_readmem, SLEIC_8039_writemem)
  MDRV_CPU_PORTS(SLEIC_8039_readport, SLEIC_8039_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i8039, 2000000/8192.) // DMD VSYNC at 244.14Hz

  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC_okim6376_intf)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC2)
  MDRV_IMPORT_FROM(SLEIC)
  MDRV_CORE_INIT_RESET_STOP(SLEIC2,NULL,NULL)

  // Io Moon 80188 map: LMCS ROM at 0, UMCS ROM at 0xC0000, the four MMCS blocks
  // (work RAM / non-volatile store / banked graphics page / DMD staging) and the
  // PACS peripheral block.  See the SLEIC2_80188_readmem comment block for the
  // chip-select values this hard-wires and where they come from
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(SLEIC2_80188_readmem, SLEIC2_80188_writemem)

  // Non-volatile store at segment 5040 (F10), persisted by NVRAM_HANDLER(SLEIC2) and
  // zero-filled on a fresh boot, which is what makes the firmware seed its own factory
  // defaults.  Replaces the base driver's generic_0fill, whose generic_nvram buffer
  // this map does not reference
  MDRV_NVRAM_HANDLER(SLEIC2)

  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC_okim6376_intf2)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC3)
  MDRV_IMPORT_FROM(SLEIC)
  MDRV_SWITCH_UPDATE(SLEIC3) // Bike Race cabinet/direct switches -> swMatrix[9]/[10]

  // Battery-backed NVRAM (28C64A): persisted by NVRAM_HANDLER(SLEIC3), zero-filled by
  // core_nvram on a fresh boot.  A blank chip makes the firmware ask for an operator
  // START at its FABRICA prompt and then seed itself, as on real hardware
  MDRV_NVRAM_HANDLER(SLEIC3)

  // Bike Race main CPU: 80C188 at 10 MHz (work RAM at seg 0, peripherals at
  // 0xA0000, code at 0xE0000). REPLACE wipes the inherited
  // map/IRQ, so memory, ports and the IRQ generator are all re-stated
  MDRV_CPU_REPLACE("mcpu", I188, 10000000)
  MDRV_CPU_MEMORY(SLEIC3_80188_readmem, SLEIC3_80188_writemem)
  MDRV_CPU_PORTS(SLEIC_80188_readport, SLEIC_80188_writeport)
  MDRV_CPU_VBLANK_INT(SLEIC_interface_update, 1)
  MDRV_CPU_PERIODIC_INT(sleic3_irq_gen, 244)

  // Bike Race I/O CPU: Z80 with the bkio07 port map (J1 byte-port link to the
  // 80188).  The base SLEIC map's generic z80_read_port returns 0 for port 0x04,
  // whose bit 7 must be 1 or the Z80 hangs in its service loop and never sends
  // the 0x5F "ready" byte the 80188 waits for ("ESPERANDO")
  MDRV_CPU_MODIFY("icpu")
  MDRV_CPU_PORTS(SLEIC3_Z80_readport, SLEIC3_Z80_writeport)

  // display section
  MDRV_CPU_ADD_TAG("dcpu", I8039, 2000000)
  MDRV_CPU_MEMORY(SLEIC_8039_readmem, SLEIC_8039_writemem)
  MDRV_CPU_PORTS(SLEIC_8039_readport, SLEIC_8039_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i8039, 2000000/8192.)

  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC_okim6376_intf2)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)
MACHINE_DRIVER_END
