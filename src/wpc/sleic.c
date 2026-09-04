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

   The F1..F15 citations throughout this file refer to the findings contract in the SLEIC
   IO Moon reverse-engineering repository (https://github.com/gerwout/sleic-iomoon, file
   asm/baseline-2026-09/findings.md), as do paths such as docs/dmd_wire_protocol.md and
   asm/pic16c57_annotated.asm.
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/i8039/i8039.h"
#include "cpu/i86/i86intf.h" /* I86_FLAGS: the IF bit gates Io Moon's interrupt delivery */
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

  /* DMD: how the two raster fields are weighted; set in MACHINE_INIT
   * (see the per-machine notes above sleic_build_dmd_frame) */
  int    dmdEqualFields;

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

  /* ---- Io Moon (SLEIC2) only, from here down ---------------------------------------
   * Io Moon runs its own interrupt generator, peripheral handlers and Z80 port map, so
   * none of the members above serve it; these are its equivalents.  They are in locals
   * for the same reason as everything else here: MACHINE_INIT's memset is what zeroes
   * them at every machine start, and MACHINE_INIT(SLEIC2) then states only the values
   * that are NOT zero (see it for the 0x28 PCS0 shadow and the trough/coin resets) */

  /* Interrupt-rate accumulators and the one-deep pending latch per source, see iomoon_irq_gen */
  double iomInt0Acc, iomT0Acc;
  int    iomInt0Pend, iomT0Pend;
  double iomPanelAcc;  /* the free-running IC23 panel raster rides the same tick */

  UINT8  iomPcs0;      /* PCS0 (0xA0000) output shadow; also selects the graphics page */
  UINT8  iomOkiLatch;  /* PCS6 (0xA0300): bit 7 = CH2, bits 0-6 = phrase number        */

  /* J1 byte port, both directions; see the block comment above iomoon_z80_write */
  struct {
    UINT8 to188;     /* Z80 port 0x80: the byte on the J1 data lines                     */
    UINT8 latch;     /* PCS2 0xA0100: what the port-0x81 strobe captured for the 80188   */
    UINT8 latchFull; /* set from the strobe until the NMI reads 0xA0100 (port-01 bit 1)  */
    UINT8 ctrl;      /* Z80 port 0x81 shadow, for the bit-2 / bit-5 strobe edges         */
    UINT8 toZ80;     /* PCS1 0xA0080: the byte the 80188 sent                            */
    UINT8 toZ80Full; /* set until the Z80 takes it (PCS3 0xA0180 bit 0 = receiver ready) */
    UINT8 pcs4;      /* PCS4 0xA0200 shadow, for the outbound bit-5 strobe edge          */
    UINT8 swCol;     /* Z80 port 0x82: switch-matrix column strobe, decoded to 0..5      */
  } iomJ1;

  /* Opt-in ball trough model, see the block comment above iomoon_ball_reset */
  struct {
    int inTrough;   /* balls resting on the trough contacts                              */
    int atExit;     /* a ball on the ball-exit contact                                    */
    int inPlay;     /* balls on the playfield                                             */
    int kick;       /* frames left before the served ball reaches the exit contact        */
    int dwell;      /* frames the ball has been sitting on the exit contact               */
    int drainHeld;  /* previous state of the drain input, so one press = one ball         */
    int seeded;     /* the model is ON and the complement has been taken from the port    */
  } iomBalls;

  /* Coin mechanism pulse train, see the block comment above iomoon_coin_reset */
  struct { int pending, phase; UINT16 lastKeys; } iomCoin;
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

/* The base machine's placeholder: a vector-less pulse of IRQ line 0.  Every concrete
 * machine below now overrides it (sleic1_irq_gen / iomoon_irq_gen / sleic3_irq_gen), so
 * nothing that ships reaches this */
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

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) interrupt model -- findings F1 (the timer programming) and F3 (the
/  vectors and what each handler does).
/
/  The IVT is RESIDENT IN ROM at physical 0, inside the LMCS window (F2/F3): the firmware
/  installs nothing, and the image contains no IVT copy loop at all (no MOVSW/MOVSB
/  anywhere).  So the driver must NOT seed a table -- the vectors are simply read out of
/  ROM1, and the three live ones are:
/
/    type 02  NMI     -> D000:016D  inbound J1 byte: reads PCS2 0xA0100, counts 0x32,
/                                   appends everything else to the 4000:1220 FIFO
/    type 08  timer 0 -> D000:024F  OKI duration counters, deferred triggers and the
/                                   general down-counters [1139]/[113B]/[113D]/[113F]/[1140]
/    type 0C  INT0    -> D000:0343  alternating half-frames ([4000:1142] toggles on entry):
/                                   even = DMD blit + animation dispatch, odd = DMD
/                                   composite + fm_player_tick + qout_service_pcs1
/
/  Only these two vectored sources exist, because the boot table unmasks only timer 0 and
/  INT0 (F1: IMASK/priority words FF32 = 0001 for the timer, FF38 = 0000 for INT0 --
/  edge-triggered and the HIGHER priority of the two, since 0 is the top level -- with
/  DMA0/DMA1/INT1/INT2/INT3 all masked and PRIMSK FF2A = 1) and nothing reprograms any of
/  it after boot.  The two handlers agree: the one entered from vector 08 clears INSERV
/  bit 0 (the timer) and the one entered from vector 0C clears INSERV bit 4 (INT0).
/
/  Timer 0's rate is fixed by that same boot table and is arithmetic, not a guess:
/    T0CON  FF56 = E003 = EN | INH | INT, CONT=1, ALT=1, and prescaler bit 3 = 0, so the
/                  counter is clocked at CLKOUT/4 and requests an interrupt on every
/                  terminal count (an assumption -- see IOMOON_T0_MAXCOUNT);
/    T0CMPA FF52 = T0CMPB FF54 = 0x6276 = 25206 -- both max counts are loaded with the
/                  same value, so alternating between them does not change the period;
/    timers 1 and 2 are disabled (FF5E = FF66 = 0).
/  =>  (10 MHz / 4) / 25206  =  2 500 000 / 25206  =  99.18 Hz.
/  The divisor and the count are confirmed.  The two things that are not are the CLKOUT
/  figure (see IOMOON_CPU_CLOCK) and the every-terminal-count reading of ALT mode (see
/  IOMOON_T0_MAXCOUNT); each is a clean factor on the result.
/
/  INT0's rate is the one genuinely open number in the model -- see IOMOON_INT0_HZ.
/
/  The NMI is deliberately NOT generated here.  It is not periodic: F4/F6 show the Z80
/  raises it by strobing port-0x81 bit 2, which latches one byte into PCS2 0xA0100 for the
/  handler to read, and 0xA0100 is read in exactly one place in the whole 80188 ROM (that
/  handler).  It is raised from iomoon_z80_write instead, where the real Z80 firmware
/  strobes it -- see the J1 byte-port block further down.
/-----------------------------------------------------------------------------------*/

/* CLKOUT.  INFERRED, not measured: IC1 is an AMD N80C188-10, and the -10 is the part's
 * speed GRADE -- the maximum clock it is rated for -- not a reading of the crystal fitted
 * on this board, and no ROM states the clock.  10 MHz is what the timer-0 arithmetic above
 * assumes, so the emulated CPU is given the same figure rather than the base driver's
 * 8 MHz, and this one constant moves both together if the crystal is ever measured */
#define IOMOON_CPU_CLOCK   10000000

/* 25206 = T0CMPA = T0CMPB, straight out of the boot table.  The period it implies rests on
 * one ASSUMPTION, which findings F1 makes too: that with ALT = 1 the timer requests an
 * interrupt on EVERY terminal count, so one max count is one period.  If the part instead
 * only interrupts on max count B, the rate halves to 49.59 Hz -- a factor of two, not a
 * detail.  Both max counts hold the same value here, so nothing else would change */
#define IOMOON_T0_MAXCOUNT 0x6276
#define IOMOON_TIMER0_HZ   ((IOMOON_CPU_CLOCK / 4.0) / (double)IOMOON_T0_MAXCOUNT) /* 99.18 */

/* INT0 (vector 0x0C) rate.  NOT CONFIRMED, and the one genuinely open number in the model
 * -- findings F3 lists the INT0 source as its unresolved gap: neither ROM says which line
 * feeds INT0, and the three candidates imply very different rates (IC23's per-plane pulse
 * ~290 Hz, its per-frame pulse ~145 Hz, or the Z80's port-0x81 bit-3 toggle, which is not
 * even free-running).  F3 recommends the per-plane pulse at ~290 Hz, because the handler's
 * two alternating bodies are exactly blit-then-composite -- one interrupt per DMD plane --
 * and because it is the only candidate whose implied outbound J1 byte rate (INT0/8, see
 * qout_service_pcs1) carries 64 lamps and 13 drivers at a playable rate.
 *
 * Neither of the two PIC rates can be run here, and the handler they would be driving is
 * why.  The composite branch sub_F08A5 is two 512-iteration byte loops of 7-9 instructions
 * each -- about 65 000 clocks, 6.5 ms at 10 MHz -- and the blit branch sub_F08EB is a
 * 512-iteration loop plus the animation dispatch; measured over a headless boot the
 * handler averages 7.5 ms.  A 290 Hz period is 3.45 ms and a 145 Hz period 6.9 ms, so
 * neither contains it, and because INT0 outranks timer 0 on the controller a
 * permanently-pending INT0 does not merely run late -- it starves the timer outright.  At
 * 290 Hz measurement shows timer 0 at zero interrupts per second and the firmware never
 * leaves its frame-delay loop at D5611: a hang, not a slow machine.
 *
 * So what ships here is 72.5 Hz, and it is important to be plain about what that number is
 * and is not.  It is an EMULATION-SERVICEABILITY figure that corresponds to NO F3
 * candidate and is not derived from the panel at all.  It cannot be: a per-plane pulse
 * MULTIPLIES the frame rate rather than dividing it, which is precisely why F3's
 * per-plane candidate is 290 Hz -- twice the 145 Hz wire rate -- and half of 145 is not a
 * rate any candidate produces.  72.5 Hz was chosen empirically, by sweeping the rate and
 * taking the highest one at which both handlers stay served: timer 0 falls to 66/s at
 * INT0 = 100 Hz, to 34/s at 145 Hz and to nothing at 290 Hz, whereas at 72.5 Hz INT0 is
 * serviced in full, timer 0 runs at 92 of its 99.18 Hz, the CPU spends 55% of its time in
 * ISRs, and the firmware boots through its intro to the J1 wait.
 *
 * The corollary points past the rate, and is the more interesting half.  If the handler
 * costs anything like 7 ms on silicon as well -- and the instruction count says it should,
 * since a real 80186 needs the same clocks and the 80188's 8-bit bus needs more -- then no
 * ~290 Hz source can be driving it, so the measurement weakens F3's SOURCE hypothesis (the
 * PIC per-plane pulse) and not just its number.  That is an emulation-side result, so it
 * does not close F3's gap: what would settle the source and the rate is an IC23 dump or a
 * scope on the line.  DMD refresh, FM tempo and the outbound queue rate all scale with
 * this constant, which is why it stays one named number */
#define IOMOON_INT0_HZ 72.5

/* I/O Z80 clock.  This is the INHERITED Bike Race / Sleic Pin-Ball figure, not an Io Moon
 * reading, and it is named here only so that it stops being invisible: the SLEIC2 block
 * used to reach the Z80 through MDRV_CPU_MODIFY, which leaves the base MDRV_CPU_ADD_TAG's
 * 2.5 MHz standing while the comment beside it talks about an 8 MHz crystal.
 *
 * The likely correction is 4 MHz, and the argument is short: IC1 on the I/O board is a
 * Goldstar Z8400A, i.e. a Z80A, whose speed GRADE is 4 MHz -- so the board inventory's
 * "8 MHz" cannot be the CPU clock.  It is X10, the board crystal, and halving it is the
 * ordinary arrangement; the same crystal divided by 8192 gives the ~977 Hz Z80 IRQ this
 * driver already states.  Neither ROM says anything about either figure.
 *
 * It is deliberately NOT changed yet.  Everything measured for the J1 link -- the switch
 * scan cadence, the handshake spins, the byte rates -- was measured at 2.5 MHz, and a 1.6x
 * change to the I/O board's speed re-times all of it at once.  Revisit it against the real
 * machine (or a scope on X10) as its own step, with switch-delivery testing re-run
 * either side of the change; that is an owner checkpoint, not a silent edit */
#define IOMOON_Z80_CLOCK 2500000

/* One periodic generator drives both sources, and it has to tick a good deal faster than
 * their sum: a request that comes due while the firmware is inside an ISR can only be
 * handed over on a later tick (see the note on the accumulators below), so the tick period
 * is also the delay between an ISR returning and the next interrupt being delivered.
 * 2 kHz = 0.5 ms of granularity against ISRs measured in milliseconds */
#define IOMOON_IRQ_TICK_HZ 2000.0

/* Panel visible-frame rate -- the third rate on that tick, and the only MEASURED one.
 * From the Saleae capture of the panel wires (docs/dmd_wire_protocol.md): DOTCLK ~599 kHz
 * over 128 x 32 dots x 2 bitplanes = 73.1 visible frames/s, and RCLK ~4.7 kHz over the 64
 * row scans the PIC performs per visible frame (32 rows x 2 planes) = 73.4 -- the same
 * figure from the other direction.  The capture's own "~145 Hz frame rate" line counts one
 * BITPLANE scan as a frame; the IC23 listing shows two of those per visible frame.  It is
 * NOT derived from IOMOON_INT0_HZ and must not be: the panel and the firmware's frame
 * pipeline are two independent clocks on this machine (F13), which is the whole point */
#define IOMOON_PANEL_HZ 73.0

/* YM3812 (IC60) master clock phi-M.  F8 settles the ports, the write primitive and the
 * sequencer, but it cannot settle this one: a clock is a wire, and no ROM mentions it.
 * What the board says (011-029A inventory, IC60 row, traced on sheet 011-029-06/07) is
 * that phi-M is fed by YACLK off the IC20 74LS393 divider in the board's 8 MHz timing
 * domain -- the same chain whose /8192 tap is the ~977 Hz Z80 IRQ this driver already
 * states.  A 74LS393 half offers /2 /4 /8 /16, so from 8 MHz the candidates are:
 *
 *     4.0 MHz (/2)   <- used here
 *     2.0 MHz (/4)
 *     1.0 MHz (/8), 0.5 MHz (/16)
 *
 * 4 MHz is picked because it is the YM3812's rated phi-M ceiling and the only tap that
 * lands anywhere near the part's 3.58 MHz nominal -- /4 and below would run the OPL2 at
 * half pitch or worse -- and because 4 MHz is the usual OPL2 clock in pinball (Alvin G.
 * uses it; PinMAME's own SLEIC interface already did).  It is a WIRING INFERENCE, not a
 * measurement: only a scope on IC60 pin 24 or the IC7 PAL dump can confirm which tap is
 * strapped.  If the music comes out an octave low, /4 is the next candidate and this line
 * is the whole fix.
 *
 * Pitch and envelope speed scale with it; musical TEMPO does not.  Tempo comes from the
 * sequencer's tick, which F8 puts on the INT0 handler's odd branch: IOMOON_INT0_HZ / 2 =
 * 36.25 Hz here, and that half is MEASURED (INT0 served at 72.5/s over a headless boot).
 * How far off hardware that is depends entirely on F3's open INT0 source, so it is stated
 * as a range and not as a number: 2x slow if INT0 is the per-FRAME candidate (~145 Hz),
 * 4x slow under F3's per-PLANE recommendation (~290 Hz).  Deliberately NOT derived from
 * IOMOON_PANEL_HZ -- the panel raster and the firmware's tick are independent clocks
 * (F13), and F3's addendum warns in as many words that 72.5 is a serviceability constant
 * and not "145 / 2 planes".  What will settle it is the OWNER'S SOUND CHECKPOINT: play a
 * track on the real machine beside this one and the tempo ratio names INT0's rate
 * directly, which makes that listening test a measurement rather than an opinion */
#define IOMOON_YM3812_CLOCK 4000000

/* Io Moon OKI MSM6376 (IC51) playback rate, in Hz -- the rate the driver hands the core as
 * the voice stream's sample rate, so it is what sets speech PITCH and duration.
 *
 * Unlike the YM3812's clock this one is MEASURED, out of the firmware and the sample ROMs,
 * and it does not need the OKI's crystal to be traced.  The firmware pairs every phrase
 * with a playing time: oki_trigger_a/b (F9) load the duration table CS:0C1F+2*(n-1) into
 * [12FD]/[1300] and the timer-0 ISR counts it down at IOMOON_TIMER0_HZ, freeing the channel
 * when it hits zero -- i.e. the table is the machine's own statement of how long phrase n
 * lasts.  The sample ROMs state the same thing in nibbles: phrase n's chained ADPCM blocks
 * (table entry n*4, see the region note in sleic.h) are 2 nibbles per byte.  Dividing one by
 * the other over ALL 28 phrases gives the rate the two agree on:
 *
 *     phrase 1   292 ticks = 2.944 s   93 496 nibbles  ->  31 757 Hz
 *     phrase 13  449 ticks = 4.527 s  143 818 nibbles  ->  31 768 Hz
 *     phrase 28  449 ticks = 4.527 s  143 690 nibbles  ->  31 740 Hz
 *     ... 28 of 28 inside 31 526 - 32 176 Hz; the long phrases (where a +-0.5 tick rounding
 *     is negligible) cluster at 31 700 - 31 770 Hz, aggregate 31 747 Hz.
 *
 * That is 0.8 % under 32 kHz and 1.6 % over 31.25 kHz, the two rates a 6376 is normally
 * strapped for (4.096 MHz and 4 MHz divided by 128), and the residual is inside the error
 * IOMOON_TIMER0_HZ carries from IOMOON_CPU_CLOCK -- a 10.08 MHz part instead of the assumed
 * 10 MHz makes the fit exactly 32 000.  So 32 kHz is the round value the evidence names,
 * and it is independently what the reverse-engineering side's sample extractor
 * (scripts/extract-oki-msm6376.py) defaults to.
 *
 * That ~1 % clock question is NOT the whole error budget, and the other branch is a factor
 * of two rather than a trim.  This rate is measured in timer-0 TICKS, so it inherits
 * IOMOON_T0_MAXCOUNT's open ALT-mode assumption as well: if the part interrupts only on max
 * count B, IOMOON_TIMER0_HZ halves to 49.59 Hz, every duration above doubles in seconds and
 * the derived rate lands at ~15.9 kHz -- i.e. 16 000, itself a standard 6376 strapping and
 * the neighbourhood Sleic Pin-Ball's interface already sits in (15 151).  The two branches
 * are audibly distinct and the owner's sound checkpoint separates them by ear:
 *     speech roughly right but slightly sharp   -> the clock branch; trim 32000 -> 31250
 *     speech an OCTAVE high (and half as long)  -> the ALT-mode branch; set ~16000 here AND
 *                                                  revisit IOMOON_TIMER0_HZ, which is then
 *                                                  wrong by the same factor everywhere else
 * That second outcome would be a finding about the timer, not about the OKI.
 *
 * It is stated here rather than reusing SLEIC_okim6376_intf2's 4000000/132: that divisor is
 * the OKIM6295's (pin 7) and belongs to Bike Race's interface, it is 4.7 % slow against the
 * measurement above, and sharing the struct would mean any pitch correction from the owner's
 * sound checkpoint silently retuned Bike Race as well */
#define IOMOON_OKI_SAMPLE_RATE 32000

/* Fractional tick accumulators, and one held request per source.  The request has to be
 * held because of how this i86 core delivers interrupts.  cpu_set_irq_line_and_vector does
 * not touch the CPU itself: it appends the request to a per-CPU event queue and schedules a
 * TIME_NOW timer, and cpu_empty_event_queue (src/cpuint.c) is what asserts the line --
 * still before the CPU executes another instruction, which is why testing IF in this
 * generator is a valid proxy for IF at delivery.  i86_set_irq_line then takes the interrupt
 * right there if IF is set, and otherwise does nothing at all: the execute loop never
 * re-examines irq_state, so HOLD_LINE does not actually hold anything and a request raised
 * while an ISR is running would just vanish.
 * That is not what the 80188's interrupt controller does: it latches the request and
 * serves it when the firmware re-enables interrupts.  Measured on Io Moon the difference
 * is not cosmetic -- the INT0 handler's DMD work is long enough that raising the two
 * sources blind loses about two thirds of the timer-0 ticks, which would stretch every
 * firmware timeout by the same factor.  So: integrate time every tick, latch at most one
 * request per source (as the hardware does -- a second one arriving before the first is
 * served is lost, not queued), and hand it over on the first tick where IF is set
 * -> locals.iomInt0Acc / locals.iomT0Acc and locals.iomInt0Pend / locals.iomT0Pend */

/* The panel raster rides on the same tick (F13): IC23 free-runs and sends the 80188 no
 * frame signal at all, so the DMD is sampled on a clock of its own rather than on
 * anything the firmware does -> locals.iomPanelAcc */
static void iomoon_submit_dmd_frame(void);

static INTERRUPT_GEN(iomoon_irq_gen) {
  /* The panel raster is not an interrupt source and is serviced first: IC23 scans segment
   * 7000 whatever the 80188 is doing, so the display stays live regardless of what the
   * interrupt model below does */
  locals.iomPanelAcc += IOMOON_PANEL_HZ / IOMOON_IRQ_TICK_HZ;
  if (locals.iomPanelAcc >= 1.0) { locals.iomPanelAcc -= 1.0; iomoon_submit_dmd_frame(); }

  locals.iomInt0Acc += IOMOON_INT0_HZ   / IOMOON_IRQ_TICK_HZ;
  locals.iomT0Acc   += IOMOON_TIMER0_HZ / IOMOON_IRQ_TICK_HZ;
  if (locals.iomInt0Acc >= 1.0) { locals.iomInt0Acc -= 1.0; locals.iomInt0Pend = 1; }
  if (locals.iomT0Acc   >= 1.0) { locals.iomT0Acc   -= 1.0; locals.iomT0Pend   = 1; }

  /* IF clear = the firmware is inside an ISR: keep both requests latched and try again */
  if (!(activecpu_get_reg(I86_FLAGS) & 0x200)) return;

  /* Only one vector can be delivered at a time, so when both are pending INT0 goes first:
   * it is the higher-priority source on the real controller (level 0 against the timer's
   * 1), and the timer takes the next tick */
  if (locals.iomInt0Pend) {
    locals.iomInt0Pend = 0;
    cpu_set_irq_line_and_vector(SLEIC_MAIN_CPU, 0, HOLD_LINE, 0x0c); /* INT0    -> D000:0343 */
  }
  else if (locals.iomT0Pend) {
    locals.iomT0Pend = 0;
    cpu_set_irq_line_and_vector(SLEIC_MAIN_CPU, 0, HOLD_LINE, 0x08); /* timer 0 -> D000:024F */
  }
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
 *   Io Moon (IC23 PIC16C57): a third display program, and the most lopsided of the three
 *     -- 200 row-hold counts for plane 0 against 30 for plane 1.  4 levels, and see
 *     MACHINE_INIT(SLEIC2) for what that ratio does and does not settle.
 *
 * Set in MACHINE_INIT (locals.dmdEqualFields); SLEIC1 (Sleic Pin-Ball) and SLEIC2 (Io Moon)
 * each state their own */

/* Decode one 128x32 two-bitplane frame -- 32 rows x 16 bytes per plane, MSB = leftmost
 * pixel, 1 = lit -- into the brightness grid core_dmd_submit_frame takes.  The two planes
 * arrive as pointers because the machines stage them differently: the I8039 games
 * interleave them in the panel buffer at 0x60410 (row stride 0x20, second plane +0x800),
 * Io Moon writes two flat 512-byte planes at 7000:0000 (row stride 0x10, second plane
 * +0x200, findings F13).  p0 is the MSB plane on all three -- see the per-machine
 * weighting note above and the PIC row-hold ratio in MACHINE_INIT(SLEIC2).  No machine
 * inverts: the firmware ANDs, ORs and copies these bytes and never NOTs or XORs them */
static void sleic_build_dmd_frame(UINT8 *dst, const UINT8 *p0, const UINT8 *p1, int rowStride) {
  int ii;
  for (ii = 0; ii < 32; ii++, p0 += rowStride, p1 += rowStride) {
    int jj;
    for (jj = 0; jj < 16; jj++) {
      const UINT8 f1 = p0[jj]; /* Bike Race / Sleic Pin-Ball: raster field 1 (rows 0x00-0x1F) */
      const UINT8 f2 = p1[jj]; /*                             raster field 2 (rows 0x20-0x3F) */
      int kk;
      for (kk = 7; kk >= 0; kk--) {
        const int a = (f1 >> kk) & 1, b = (f2 >> kk) & 1;
        *dst++ = locals.dmdEqualFields ? (a | b ? (a & b ? 3 : 2) : 0)  /* 3 levels  */
                                       : ((a << 1) | b);                /* 4 levels  */
      }
    }
  }
}

/* Decode the 2-bitplane frame buffer at 0x60410 into a 128x32 brightness grid.
 * See the plane/weighting notes in SLEIC_irq_i8039 below */
static void sleic3_build_dmd_frame(UINT8 *dst) {
  const UINT8 * const buf = memory_region(SLEIC_MEMREG_CPU) + 0x60410;
  sleic_build_dmd_frame(dst, buf, buf + 0x800, 0x20);
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

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) DMD frame path -- findings F13.
/
/  The firmware runs a four-stage pipeline in work RAM and ends it with a plain copy into
/  a 1 KB display buffer at segment 7000 (flat 0x70000), which is all the panel ever sees:
/
/    4000:0000-05FF   sprite planes 0/1, then a one-plane mask (0xFF = pass-all)
/    4000:0600-09FF   background planes 0/1 -- where the animation loader lands its frames
/    4000:0A00-0DFF   composite planes 0/1 = (background AND mask) OR sprite   sub_F08A5
/    7000:0000-01FF   display plane 0  \ straight copy of 4000:0A00/0C00,      sub_F08EB
/    7000:0200-03FF   display plane 1  / plane 1 first, then plane 0
/
/  Composite and blit are the two alternating branches of the INT0 ISR (F3), so the
/  display buffer is refreshed at INT0/2.  Four things follow from F13 and are what this
/  path implements:
/
/    * 32 rows x 16 bytes per plane, MSB = leftmost pixel -- the same geometry the
/      animation pages carry in their header (F2: rows 0x20, bytes/row 0x10, plane stride
/      0x200), so a full-screen background image reaches the panel byte-for-byte;
/    * plane 0 is the MSB, because the PIC scans it first and holds each of its rows far
/      longer (the 200:30 row-hold ratio -- see MACHINE_INIT(SLEIC2));
/    * NO inversion.  Nothing in the firmware NOTs or XORs these bytes, so an inversion
/      could only be a property of the panel, and nothing observed says it is one.  (The
/      old driver applied ^0xFF here; F13 rejects it.);
/    * NO frame strobe.  PCS4 0xA0200 bit 3 -- the old driver's "swap buffer", which is
/      what used to trigger the submit -- is pulsed exactly ONCE, from boot_init.  IC23 is
/      a free-running raster with no command interface (150 words, and the only input it
/      samples is the external dot clock), so the panel just displays whatever stands in
/      7000:0000-03FF at the moment it scans it.
/
/  So the driver does what the panel does: it samples the buffer at the panel's visible
/  frame rate and submits what it finds, rather than waiting for a signal the hardware
/  never sends.  Segment 7000 is not double-buffered, so a sample can catch a blit in
/  progress and show one plane a frame ahead of the other -- the real panel has exactly
/  the same race, for exactly the same reason.
/-----------------------------------------------------------------------------------*/
#define IOMOON_DMD_STAGE 0x70000 /* 7000:0000 = display plane 0 (MSB); plane 1 at +0x200 */

static void iomoon_submit_dmd_frame(void) {
  const UINT8 * const stage = memory_region(SLEIC_MEMREG_CPU) + IOMOON_DMD_STAGE;
  sleic_build_dmd_frame(locals.rawDMD, stage, stage + 0x200, 0x10);
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
      if (data & 0x08) {               /* latch the finished 0x60410 frame for the I8039 tick */
        sleic3_build_dmd_frame(locals.dmdLatch);
        locals.dmdLatchTtl = SLEIC3_DMD_LATCH_TICKS;
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
/* Io Moon (SLEIC2) YM3812 (IC60).  Same shape as the base interface, stated separately so
 * the clock is the named constant with its derivation (IOMOON_YM3812_CLOCK) instead of a
 * bare 4000000 shared with two machines whose boards were never traced for it.
 *
 * The IRQ callback stays disconnected, and for once that is a finding rather than a
 * shrug: the OPL2's only interrupt source is its two internal timers, and NO Io Moon
 * song stream writes registers 02, 03 or 04 -- all ten streams in the CS:0DE5 table were
 * walked through the sequencer's own opcode rules (each 0xDD loop taken once), 22 012
 * register writes, not one of them to a timer register -- so the chip can never assert
 * /IRQ on this machine.  That also removes it as a candidate source for the unidentified
 * INT0 line (F3) */
static struct YM3812interface SLEIC2_ym3812_intf =
{
	1,					/* 1 chip (IC60)          */
	IOMOON_YM3812_CLOCK,	/* phi-M -- see the constant */
	{ 100 },			/* volume                 */
	{ ym3812_irq },		/* never called: the firmware starts no OPL2 timer */
};

static struct OKIM6295interface SLEIC_okim6376_intf2 =
{
	0,					/* 1 chip (but use 0 to indicate 6376 chip) */
	{ 4000000./132. },	/* sampling frequency at 4MHz chip clock */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};
/* Io Moon (SLEIC2) OKI MSM6376 (IC51).  Same shape as the interface above and the same
 * region -- REGION_USER1 holds V1 3_03.bin at 0x00000 and V1 3_04.bin at 0x80000, which is
 * where the firmware's own phrase table says the samples are (sleic.h's SLEIC_ROMSTART5
 * note) -- but the stream rate is Io Moon's measured one rather than Bike Race's assumed
 * 4 MHz/132.  See IOMOON_OKI_SAMPLE_RATE for the derivation; it is the one knob the owner's
 * speech pitch/tempo A/B lands on, and having it here keeps that A/B off Bike Race.
 *
 * num = 0 is not "no chips": it is how PinMAME's OKIM6295 core is told this is a 6376 --
 * two voices instead of four, and generate_adpcm_6376's chained-block decoder (adpcm.c) */
static struct OKIM6295interface SLEIC2_okim6376_intf =
{
	0,								/* 6376 marker: 1 chip (IC51), 2 voices        */
	{ IOMOON_OKI_SAMPLE_RATE },		/* playback rate -- measured, see the constant */
	{ REGION_USER1 },				/* V1 3_03.bin + V1 3_04.bin, contiguous 1 MB  */
	{ 75 }							/* volume                                      */
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
 * I8039 renders sleicpin's DMD from the 0x60410 frame buffer, not from a strobe */
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
/* Not part of locals, same as sleic1_nvram and sleic3_nvram: NVRAM_HANDLER(SLEIC2) below
 * fills this from the .nv file (or zero-fills it) at every machine start, and it must
 * survive locals' memset -- mame.c loads NVRAM before cpu_run(), and MACHINE_INIT runs
 * from inside it */
static UINT8 iomoon_nvram[IOMOON_NVRAM_SIZE];
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
/* The PCS0 (0xA0000) output shadow that selects the page above is locals.iomPcs0;
 * boot leaves it at 0x28 (see MACHINE_INIT(SLEIC2)) */
static void iomoon_set_gfx_bank(UINT8 pcs0) {
  cpu_setbank(IOMOON_GFX_BANK, memory_region(SLEIC_MEMREG_GFX) + iomoon_gfx_page_base[pcs0 & 0x07]);
}

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) J1 byte port -- findings F6 (the port, both directions), F4 (what the
/  80188 does with an inbound byte) and F5 (the switch codes the Z80 sends over it).
/
/  J1 is an 8-bit bidirectional byte port with handshakes: no address bus, no shared
/  memory, no HOLD/HLDA.  Each direction is one latch plus one interrupt, and BOTH ends
/  here run their real firmware -- the driver only carries bytes.  Nothing is synthesised:
/  the boot 0x47, the 0x45/0x46 replies and every switch code come out of the Z80 ROM
/  (V1 3_05.bin), and F4 rejects the old branch's invented marker stream outright.
/
/  Z80 -> 80188.  Two outbound channels share the data port, and both strobe the same
/  latch (F6; 0xA0100 is the only inbound read in the whole 80188 ROM, so a second latch
/  would be unread by construction):
/
/    host_send_c0fc 0116  one-byte EVENT CODE (F5)       port-0x81 bit 2
/    host_send_c008 0144  8-bit STATE BITMASK            port-0x81 bit 5
/
/    IN A,($01) / BIT 1 / JR Z,self   spin until the port is free   <- port 0x01 bit 1
/    (C001) |= 0x02, OUT ($81)        data-valid
/    A = the byte,  OUT ($80)         onto the data lines
/    (C001) |= 0x04, OUT ($81)        strobe: latch + 80188 NMI
/
/  The strobe latches the port-0x80 byte at PCS2 0xA0100 and raises the 80188 NMI, whose
/  handler D000:016D reads that latch exactly ONCE (F4), counts the value 0x32 in
/  [4000:1144] and appends everything else to the FIFO at 4000:1220.  The driver models
/  the latch as a one-byte mailbox with flow control on port-0x01 bit 1: free while empty,
/  busy from the strobe until the NMI reads 0xA0100.  That is what stops the Z80
/  overrunning a latch the 80188 has not emptied yet -- and dropped switch bytes are
/  exactly the failure that flow control exists to prevent.
/
/  It cannot deadlock, and NOT because of anything on the Z80 side: host_send_c008, the
/  sibling sender at Z80 0144, is also reached from the IRQ handler (call sites at 0B49
/  and 0C7E), so its spin can run with IFF1 = 0 and no Z80 interrupt will break it.  What
/  clears the latch is the other CPU, on two mechanisms
/  that hold regardless: (a) the 80188 NMI is non-maskable, so the handler that reads
/  0xA0100 always runs; and (b) cpu_set_irq_line called from Z80 context queues the
/  request for CPU 0 and schedules it with timer_set(TIME_NOW), which calls
/  activecpu_abort_timeslice (src/timer.c) -- so the 80188 is given the CPU almost
/  immediately, long before the Z80 has spun for any meaningful time.
/
/  80188 -> Z80.  qout_service_pcs1 D01E5, called from the INT0 ISR, writes the byte to
/  PCS1 0xA0080 and then pulses PCS4 0xA0200 bit 6 and bit 5, releasing both after 13
/  NOPs.  On the Z80 that strobe is an NMI (0x0066): it gates port-0x81 bit 4 on, reads
/  IN ($00), stores into the ring at $C076 and lets host_cmd_dispatch 16D5 run the byte
/  through the 256-entry table at $2000 (F7).  Before writing, the ISR tests PCS3
/  0xA0180 bit 0 -- "receiver ready" -- and gives up for this call if it is clear, which
/  is the reverse direction's flow control and is modelled the same way.
/
/  The Z80 NMI is taken on the bit-5 edge rather than bit 6 for a reason that is worth
/  recording: the INBOUND NMI handler also drives PCS4, asserting bits 7, 6 and 4
/  (OR AL,0D0) around its latch read, so bit 6 rises on every Z80->80188 byte as well and
/  would fire a spurious Z80 NMI per inbound byte.  Bit 5 is touched by nothing but
/  qout_service_pcs1.  WHICH J1 line each PCS4 bit actually drives is F6's own open
/  clause (it needs the IC7/IC8 PALs); what the firmware pair needs, and what this
/  models, is one Z80 NMI per outbound byte, after the byte is latched.
/
/  The latches and shadows it needs are locals.iomJ1, declared with the locals struct.
/-----------------------------------------------------------------------------------*/

/* The ball trough (defined with the model further down): the 80188 -> Z80 strobe hands
 * every command byte here, and 0xE9 is the one that moves a ball; the reset is the
 * machine init's */
static void iomoon_ball_command(UINT8 cmd);
static void iomoon_ball_reset(void);

/* The coin mechanism's pulse train (model further down); the reset is the machine init's */
static void iomoon_coin_reset(void);

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) OKI MSM6376 speech and effects -- finding F9.
/
/  The whole chip is driven by two addresses, and F9 accounts for every write the ROM makes
/  to either of them (5 writes to PCS6, 8 to PCS0, all enumerated):
/
/    PCS6 0xA0300   an 8-bit latch on the OKI's data pins.  Bits 0-6 are the phrase number;
/                   bit 7 is the CHANNEL, and the driver reads it as the chip's CH2 pin.
/    PCS0 0xA0000   bit 5 is /OKCS, the chip's start (ST) input: idle HIGH, pulsed low then
/                   high by okcs_strobe D0CE2.  The other PCS0 bits belong to F2 and F10.
/
/  The three sequences, verbatim from F9:
/
/    oki_trigger_a D0C57   0xA0300 <- 0x80|n   then strobe            (bit 7 SET)
/    oki_trigger_b D0C84   0xA0300 <- n&0x7F   then strobe,           (bit 7 CLEAR)
/                          then 0xA0300 <- 0x80|n with NO second strobe
/    sub_D0CB8             0xA0300 <- 0        then bit 5 LOW and left low  (silence/reset)
/
/  Reading bit 7 as CH2 is what makes those three coherent, and it is the reading the code
/  argues for rather than an analogy with the sister machines: trigger_b drives bit 7 low
/  only across its strobe and RESTORES it immediately after, which is a level a pin is held
/  at for the duration of an event, not a data byte -- a data byte would have no reason to
/  be rewritten once latched.  Boot leaves the latch at 0x80 (D00C4/D00C6) and the shadow at
/  0x28, i.e. the idle state is "channel 2 selected, ST released".
/
/  PinMAME's core speaks the 6295-style two-byte protocol instead of ST/CH2 (adpcm.c
/  OKIM6376_data_w: a byte with bit 7 set latches the phrase, the next byte starts the
/  voices named by its bits 4-5), so a strobe is translated into that pair here.  Three
/  details of the translation are deliberate:
/
/   - WHICH EDGE.  The falling one.  For both trigger routines it makes no difference --
/     the latch holds the same value at both edges, because trigger_b's rewrite comes after
/     the pulse -- so the choice is settled by the third sequence: sub_D0CB8 drives ST low
/     and leaves it there, so only the falling edge sees the silence request at all.
/     A consequence worth stating: a strobe issued while ST is ALREADY low fires nothing
/     here.  That is not a hole in the model, it is the pin -- a level that does not move
/     is an edge the chip does not see either -- and the firmware never does it: after
/     sub_D0CB8 it sets [1303] = 0xFF, and the timer-0 ISR restores ST high through
/     pcs0_bit5_set_far (D0284) BEFORE it starts the parked sample.
/   - PHRASE 0 = STOP BOTH VOICES.  That is sub_D0CB8's whole purpose: it is called when a
/     priority sample arrives and both channels are busy (D0BBE), and it then parks the
/     sample for the timer-0 ISR to start a tick later.  Stopping only the CH2-selected
/     voice would leave the other one playing, and the core would then REFUSE the parked
/     sample ("requested to play sample on non-stopped voice") -- the priority path would
/     drop exactly the sound it exists to make room for.
/   - RETRIGGER STOPS FIRST.  A real ST pulse restarts a channel whatever it was doing; the
/     core instead refuses a start on a voice still playing.  The firmware's own busy model
/     is software (F9: the duration counters, no BUSY pin is read anywhere), and those
/     durations match the sample lengths to about 1 % -- see IOMOON_OKI_SAMPLE_RATE -- so a
/     re-trigger landing a few milliseconds early is expected, and without the explicit stop
/     it would be silently dropped instead of restarting.
/
/  Not modelled, and stated rather than hidden: WHICH physical channel bit 7 = 1 is.  Both
/  voices mix to the same output at the same volume and the firmware never reads the chip
/  back, so swapping them changes nothing audible -- only which voice number a phrase
/  occupies -- and no dump is needed to settle something with no observable difference.
/
/  The PCS6 0xA0300 latch itself (bit 7 = CH2, bits 0-6 = phrase number) is
/  locals.iomOkiLatch.
/-----------------------------------------------------------------------------------*/
static void iomoon_oki_strobe(void) {
  const UINT8 phrase = locals.iomOkiLatch & 0x7f;
  /* CH2 -> the core's voice bit: bit 4 = voice 0, bit 5 = voice 1 (adpcm.c, data >> 4) */
  const UINT8 voice  = (locals.iomOkiLatch & 0x80) ? 1 : 0;

  /* The test is on the PHRASE, not on the whole latch, and the two are the same test here:
   * the only latch value with phrase 0 that sub_D0CB8 can leave is 0x00 itself, because the
   * one other way phrase 0 could reach a trigger routine -- sound command 0 -- is intercepted
   * by the dispatcher at D0B92, which substitutes a random sample from CS:0C19 instead of
   * passing 0 through.  Testing the phrase is kept because it is also the right answer for
   * the value that cannot occur: a phrase-0 start is silence on the chip too, its table
   * entry being null */
  if (!phrase) {                      /* sub_D0CB8: latch 0, ST low -- silence both */
    OKIM6376_data_0_w(0, 0x18);       /* no command pending -> bits 3,4 = stop voices 0,1 */
    return;
  }
  OKIM6376_data_0_w(0, (UINT8)(0x08 << voice));   /* abort this channel first (see above) */
  OKIM6376_data_0_w(0, (UINT8)(0x80 | phrase));   /* phrase number on D0-D6              */
  OKIM6376_data_0_w(0, (UINT8)(0x10 << voice));   /* start it on the CH2-selected voice  */
}

/* Io Moon 80188 peripheral write.  PCS0's page select (F2) and /OKCS strobe (F9), the J1
 * outbound half (PCS1 data + the PCS4 bit-5 strobe, F6), PCS5's YM3812 pair (F8) and PCS6's
 * OKI latch (F9) are wired; the PCS0 NVRAM gate bits (F10) need no action -- the window
 * they open is mapped unconditionally, see IOMOON_NVRAM_BASE */
static WRITE_HANDLER(sleic2_periph_w) {
  switch (offset) {
    case 0x000: /* PCS0: bits 0-2 graphics page (F2), 3/4 NVRAM window gate (F10), 5 = OKI /OKCS (F9) */
      if ((data ^ locals.iomPcs0) & 0x07) iomoon_set_gfx_bank(data);
      /* bit 5 = the OKI's ST input, idle high (F9).  Only the three OKI routines ever move
       * it -- okcs_strobe D0CE2, pcs0_bit5_clear D0CFC and pcs0_bit5_set_far D0D09 -- while
       * the page select F00A0 and the NVRAM gate D0596/D05B7 write the same shadow byte
       * with bit 5 untouched, so an edge here is always an OKI event and never a side
       * effect of the other two fields sharing this register */
      if ((locals.iomPcs0 & 0x20) && !(data & 0x20)) iomoon_oki_strobe();
      locals.iomPcs0 = data;
      return;
    case 0x080: /* PCS1: the byte on the outbound J1 data lines (qout_service_pcs1 D020F).
                 * Writing it does NOT start a transfer -- the PCS4 bit-5 strobe below
                 * does.  That distinction is not academic: the boot's peripheral init
                 * writes 0x00 here long before any queue exists, and treating that as a
                 * byte in flight would leave "receiver ready" clear for ever and stall
                 * the whole outbound direction */
      locals.iomJ1.toZ80 = data;
      return;
    case 0x200: /* PCS4: J1 handshake lines.  Bit 5 is pulsed by qout_service_pcs1 alone
                 * (D022A/D0243) -- the inbound NMI handler asserts bits 7/6/4 but never
                 * bit 5 -- so its rising edge is the outbound strobe, and it arrives
                 * after the PCS1 write above.  Bit 3 (the old branch's "frame strobe")
                 * belongs to the DMD frame path (F13), not to this handshake.
                 *
                 * Bits 0-2 are deliberately ignored: sub_D03C0 / sub_D0401 bit-bang them
                 * as a three-wire serial port (CS, U/D, INC) for the X9C503P digital
                 * potentiometer IC63, which is the service menu's VOLUME page.  That part
                 * is an ANALOGUE attenuator on the YM3014B's output, not a register the
                 * sound core has: PinMAME has no device for it, and the mixer level here
                 * is the driver's own MDRV volume.  The page still works -- it draws its
                 * bar and stores the setting in NVRAM 0x1BC-0x1BE (sub_D098C) -- it just
                 * does not change what comes out, and modelling it would mean scaling the
                 * YM3812 channel by a wiper position the schematic has not been read for */
      if ((data & 0x20) && !(locals.iomJ1.pcs4 & 0x20)) {
        locals.iomJ1.toZ80Full = 1;
        iomoon_ball_command(locals.iomJ1.toZ80);         /* the ball trough, 0xE9 = serve */
        cpu_set_irq_line(SLEIC_IO_CPU, IRQ_LINE_NMI, PULSE_LINE); /* Z80 handler 0x0066 */
      }
      locals.iomJ1.pcs4 = data;
      return;
    case 0x280: /* PCS5 with A0 = 0: the YM3812's register/index port (F8).
                 *
                 * Io Moon uses TWO ADDRESSES, one per port -- ES:[0280] then ES:[0281]
                 * inside the one write primitive ym3812_write D0D99 -- so A0 comes off the
                 * address bus and the driver decodes it here.  Neither sister machine does
                 * it this way and both were checked before this was written: Bike Race
                 * streams every byte at 0xA0280 and toggles A0 in hardware (sleic_periph_w
                 * alternates), and Sleic Pin-Ball streams every byte at 0xA0280 with A0
                 * carried on PCS0 bit 1 (locals.ymA0).  Three machines, three conventions;
                 * this one is the only one the Io Moon ROM supports, and F8 confirms these
                 * are the only two accesses to either address in the whole ROM */
      YM3812_control_port_0_w(0, data);
      return;
    case 0x281: /* PCS5 with A0 = 1: the YM3812's data port (F8).  The firmware's settling
                 * delay between the two writes (ym3812_settle_delay D0DA9, 10 iterations)
                 * is a real-chip timing requirement with no emulated equivalent -- the core
                 * accepts the pair back to back -- so nothing here waits */
      YM3812_write_port_0_w(0, data);
      return;
    case 0x300: /* PCS6: the OKI MSM6376's data/CH2 latch (F9).  Latching alone starts
                 * nothing -- the PCS0 bit-5 pulse above does -- which is why the boot's
                 * 0x80 (D00C6) and trigger_b's trailing rewrite (D0CAE) are silent here */
      locals.iomOkiLatch = data;
      return;
    default:                          /* PCS2, PCS3 */
#ifdef DEBUG_SLEIC
      if (getenv("SLEIC_TRACE_PW")) fprintf(stderr, "[188->periph] PCS%d off=%03x data=%02x\n", offset>>7, offset, data);
#endif
      return;
  }
}

/* Io Moon 80188 peripheral read.  The J1 half is modelled (F4/F6) and the YM3812 status
 * port answers at PCS5 0xA0280 (F8).  Deliberately NOT the base sleic_periph_r
 * -- its 0x37 "no event" idle is a Bike Race convention that F4 rules out here: 0xA0100
 * carries Z80 bytes and nothing else */
static READ_HANDLER(sleic2_periph_r) {
  switch (offset) {
    case 0x100: /* PCS2: the J1 inbound latch, read exactly once per byte by the NMI
                 * handler D018C (F4).  The read is what frees the port for the Z80's
                 * next byte, so clear the busy flag and leave the latch itself standing
                 * -- a hardware latch holds its last value */
      locals.iomJ1.latchFull = 0;
      return locals.iomJ1.latch;
    case 0x180: /* PCS3 bit 0: "receiver ready".  qout_service_pcs1 tests it (D0206) and
                 * gives up for this call when it is clear, so it must report that the Z80
                 * has taken the previous byte */
      return locals.iomJ1.toZ80Full ? 0x00 : 0x01;
    case 0x280: /* PCS5: the YM3812 status port, which reads back on the same address as the
                 * index port (A0 = 0) exactly as on any OPL2.
                 *
                 * The Io Moon firmware never reads it -- F8 finds only the two writes in
                 * ym3812_write, and busy is handled by the software settling delay instead
                 * of by polling bit 7 -- so this line changes no behaviour today.  It is
                 * here because the hardware answers here: leaving the address to the
                 * function's default 0 would quietly invent a chip that reports "no timer
                 * overflow, not busy" for ever, and a patched or later ROM that does poll
                 * would then hang against a lie rather than run against the core */
      return YM3812_status_port_0_r(0);
  }
  return 0;
}

static MEMORY_READ_START(SLEIC2_80188_readmem)
  {0x00000,0x3ffff, MRA_ROM},         /* LMCS: ROM1 low half -- resident IVT + animation data */
  {0x40000,0x4ffff, MRA_RAM},         /* MCS0: work RAM.  The chip-select block is 64 KB; the
                                       * UM62256 at IC12 behind it is 32 KB, so this window is
                                       * a superset -- harmless, because the firmware's highest
                                       * work-RAM segment is 413C (flat 0x41496, F5)           */
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
 *   MCS1 0x20000-0x3FFFF : graphics ROM bkcpu06 (sprite table in its first 0x1104)
 *   MCS2 0x40000-0x5FFFF : graphics ROM bkcpu05 (read via ES=0x5000)
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

/* Z80 I/O processor state shadow, shared by the SLEIC1 / SLEIC2 / SLEIC3 port handlers
 * (all three I/O ROMs drive the same latch conventions; Io Moon uses lampRow and keeps
 * its own J1 and switch-column state in locals.iomJ1) */
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

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) I/O Z80 ports.  The J1 half is the block above; this is the rest of
/  the map the real V1 3_05.bin firmware drives, read out of that ROM.
/
/  IN  0x00  the J1 inbound byte (PCS1), taken by the NMI (0x0080, gate port-0x81 bit 4)
/            or by the polled host_read_byte 01B6 (gate bit 6).  One latch either way
/  IN  0x01  bit 1 = J1 port free (every host_send_* and host_read_byte spins on it,
/            0116/0144/017E/01B6), bit 5 = the selected direct input, active LOW
/            (direct_input_scan 0DF8/0E36)
/  IN  0x02  switch-matrix column return, active LOW: sw_read_col0..5 2ED3..2FB9 CPL it
/            into the per-column change masks.  6 columns x 8 = the 48 codes of F5
/  IN  0x03  cabinet inputs, active LOW (input_port03_read 2E54 CPLs it).  Bits and the
/            codes their handlers send: 0 -> 0x3E (sub_125B, with a 3000-tick lockout),
/            1 -> 0x3F (sub_1278; F14: this is the byte that OPENS the service menu),
/            4 -> 0x40 (sub_1285), 2 and 3 -> the two flipper inputs (sub_12D8 / sub_1292,
/            which fire the port-0x85 coil pairs directly and send 0x42 / 0x41 only in
/            test mode), 5 -> the auto-repeating 0x32 (0x33 in test mode, 0D3C/0D44)
/  IN  0x04  cabinet/config byte -- see IOMOON_PORT04_IDLE
/
/  OUT 0x80  J1 data lines            OUT 0x83  lamp column strobe, one-hot 0x01..0x80
/  OUT 0x81  J1 control (bit map F6)  OUT 0x84  lamp row data, active HIGH, latched
/  OUT 0x82  switch column strobe     OUT 0x85/0x86  driver latches, active LOW
/                                     OUT 0x87  direct-input index + flag bits
/
/  The three output groups are F7: 8 columns x 8 bits = 64 lamps (row byte on 0x84, then
/  the column strobe on 0x83), and TWO 8-bit driver latches = 16 driver bits -- not the
/  "7 columns x 16 rows" and "18 solenoids" of the service manual's summary, both of which
/  the I/O ROM contradicts.
/-----------------------------------------------------------------------------------*/

/* Port 0x04 with nothing pressed.  Two bits have to be right or the firmware misbehaves,
 * and both readings come from the ROM:
 *
 *   bit 7 = 1  selftest_wait_reset 2E42 spins on it (IN ($04) / BIT 7 / JP Z,self), so a 0
 *              here is a hang the moment the self-test path is taken.
 *   bit 0 = 1  disables direct_input_scan 0DBF (IN ($04) / BIT 0 / RET NZ).  That scan is
 *              the 16-way multiplexed input behind port-0x87's low nibble and port-0x01
 *              bit 5, sending codes 0x50-0x79; which physical contacts sit on it is part
 *              of F5's open gap, so the honest model is "not fitted" rather than a
 *              guessed wiring.  Turning it on is a one-bit change once that is known.
 *
 * Bits 1-3 and bit 5 are NOT in this constant: they are SW40 switches the firmware acts
 * on, so they come from the DIP bank -- see iomoon_port04() below.  What remains here is
 * bits 0, 4, 6 and 7, all high.
 *
 * Two structural notes, because the read is "IDLE & ~swMatrix[10]" and an AND can only
 * ever CLEAR bits:
 *   - swMatrix[10] cannot RAISE bit 5.  Modelling the trough therefore means changing
 *     this CONSTANT to 0xFF, not mapping a switch -- and that is the change to make once
 *     the trough contacts are identified (F5's open gap), since bit 5 = 1 is what puts
 *     the 0xED handler on its real "wait for the balls" path.
 *   - anything mapped into swMatrix[10] bit 7 would pull that bit low and hang
 *     selftest_wait_reset 2E42.  Nothing writes row 10 today; keep it that way unless
 *     the bit is understood. */
#define IOMOON_PORT04_IDLE 0xff

/* Bits 1-3 of port 0x04 are the SW40 country switches SW2-SW4, and they are NOT an idle
 * constant -- the firmware reads them and acts on them, so they are a DIP the user sets
 * (sleic.h, DIP bank 0).  The path is a full J1 round trip on every boot: sub_D5A8B
 * D5A8B pushes Z80 command 0xF9, handler 2D9D answers IN($04) | 0xF0, and the 80188's
 * reader turns bits 1-3 of that byte into a country number with
 *
 *     D5CA3: CMP [0006], 0F0 / JNB          ; a 0xF0-or-above byte is the DIP report
 *     D5CAC: AND AL, 00E / SUB AX, 2        ; country = (port04 >> 1) & 7
 *     D5CC2: JMP CS:W[BX + 02DE1]           ; seven-way table at D5D01 -> [4130:0020]
 *
 * and then acts on it twice: sub_D69CC applies that country's coin-value preset (one of
 * eight, D6D36/D6DDF/.../D7204) and saves it to NVRAM 0x1C4-0x1CF, and boot_init compares
 * the DIP-derived country with the stored NVRAM 0x1BF at D664D -- if they differ the DIP
 * wins, the NVRAM is rewritten and the preset re-applied.  Country 5 additionally selects
 * the Spanish string and menu tables (D3277, D8048, DD406); every other value is English.
 * So this one DIP sets both the coinage and the language, exactly as the service manual's
 * "country code" row implies.  Which of SW2/SW4 is the low bit is settled by the presets
 * themselves, not by the UK and Spain rows (both are palindromes and survive a reversal);
 * sleic.h carries the eight-row comparison and the three rows a reversal would break.
 *
 * Bit 5 is SW40-5, the manual's "servicio: no se dispensan bolas".  The 0xED handler
 * 2BEB reads it (IN ($04) / BIT 5 / JP Z,2C17) and with the bit LOW answers 0x45 without
 * looking at a contact -- which is exactly "do not dispense balls", i.e. the SERVICE
 * position.  Normal play is the bit HIGH, where 2BEB runs the real check: strobe column
 * 0, test the trough (sub_2C1F), and answer 0x45 through sub_2851 when the balls are
 * home or 0x46 and the eject sequence when they are not.  That branch used to be a hang,
 * which is why this bit was held low; with the trough modelled it is not, and the DIP
 * now selects it.  Measured both ways: bit high answers 0x45 in ONE frame at boot and
 * the whole coin/credit/START/serve chain runs identically to bit low. */
static UINT8 iomoon_port04(void) {
  /* DIP 0x10 is SW40-5 and its two settings are named for the switch, not for the bit:
   * "On" (DIP 0) = the service position = port-04 bit 5 LOW, "Off" (DIP 0x10) = normal
   * play = bit 5 HIGH.  Default is Off */
  return (UINT8)((IOMOON_PORT04_IDLE & ~0x2e) | (core_getDip(0) & 0x0e)
                 | ((core_getDip(0) & 0x10) ? 0x20 : 0x00));
}

static READ_HANDLER(iomoon_z80_read) {
  switch (offset) {
    case 0x00: /* J1 inbound data.  Reading is what frees the outbound latch, which is
                * what PCS3 0xA0180 bit 0 reports back to qout_service_pcs1 */
      locals.iomJ1.toZ80Full = 0;
      return locals.iomJ1.toZ80;
    case 0x01: /* bit 1 = J1 free (clear while the 80188 has not read the latch yet),
                * bit 5 = direct input, 1 = open.  The scan that reads bit 5 is gated off
                * by IOMOON_PORT04_IDLE bit 0 anyway */
      return (UINT8)((locals.iomJ1.latchFull ? 0x00 : 0x02) | 0x20);
    case 0x02: /* switch matrix: 6 columns -> swMatrix[1..6], active low */
      return (UINT8)~coreGlobals.swMatrix[1 + locals.iomJ1.swCol];
    case 0x03: /* cabinet inputs -> swMatrix[9], active low */
      return (UINT8)~coreGlobals.swMatrix[9];
    case 0x04: /* cabinet/config byte; swMatrix[10] pulls a bit low when closed */
      return (UINT8)(iomoon_port04() & ~coreGlobals.swMatrix[10]);
    default:
      logerror("iomoon Z80 read port %02x\n", offset);
  }
  return 0;
}

static WRITE_HANDLER(iomoon_z80_write) {
  switch (offset) {
    case 0x00: /* port 0x80: the byte onto the J1 data lines (written before the strobe) */
      locals.iomJ1.to188 = data;
      break;
    case 0x01: /* port 0x81: J1 control.  Bit 2 (event codes) and bit 5 (the C008 state
                * bitmask) are the two outbound strobes and reach the same latch (F6);
                * bit 1 is data-valid, bits 4/6 gate the two inbound read paths, bit 3 is
                * the free-running-ish toggle F3 lists as an INT0 candidate and bit 0 is
                * unproven -- none of those four need an action here */
      { const UINT8 rise = (UINT8)(data & ~locals.iomJ1.ctrl);
        if (rise & 0x24) {
          /* A strobe arriving on a latch the NMI has not read yet would overwrite an
           * undelivered byte -- a lost switch code, silently.  Port-0x01 bit 1 is what
           * stops the firmware getting here, so this is unreachable as the ROM stands;
           * it is a tripwire for any future change to the port-0x81 handling */
          if (locals.iomJ1.latchFull)
            logerror("iomoon J1: strobe %02x over undelivered byte %02x\n",
                     locals.iomJ1.to188, locals.iomJ1.latch);
          locals.iomJ1.latch = locals.iomJ1.to188;
          locals.iomJ1.latchFull = 1;
          cpu_set_irq_line(SLEIC_MAIN_CPU, IRQ_LINE_NMI, PULSE_LINE); /* handler D000:016D */
        }
      }
      locals.iomJ1.ctrl = data;
      break;
    case 0x02: /* port 0x82: switch-matrix column strobe, one-hot 0x01..0x20 = column 0..5 */
      if (data) { const unsigned col = core_BitColToNum(data & -data);
                  if (col < 6) locals.iomJ1.swCol = (UINT8)col; }
      break;
    case 0x03: /* port 0x83: lamp-matrix COLUMN strobe, one-hot 0x01..0x80 = column 0..7.
                * lamp_colN_out (Z80 3457..34C7, one column per lamp_scan_tick, so the whole
                * matrix is refreshed every 8 Z80 interrupts) writes the row byte to 0x84
                * FIRST and pulses the column here second, so the pair commits on this
                * strobe with the row latched by the preceding 0x84 */
      if (data) coreGlobals.tmpLampMatrix[core_BitColToNum(data & -data)] = sleic_io.lampRow;
      break;
    case 0x04: /* port 0x84: lamp-matrix ROW data, ACTIVE HIGH -- no inversion.  The byte is
                * one of C10F..C116, which sub_353A fills either verbatim from the "lit" bank
                * C0FF..C106 or as that bank ANDed with the "steady" bank C107..C10E on its
                * alternate phase (the blink model: lit+steady = on, lit only = blinking),
                * so a 1 bit is a lamp that is on; boot_port_init 043A starts it at 0x00.
                * Latched here, committed on the 0x83 strobe */
      sleic_io.lampRow = data;
      break;
    case 0x05: /* port 0x85: driver latch A -> solenoids 1-8.  ACTIVE LOW, so the byte is
                * inverted here and coreGlobals carries positive logic: boot_port_init 0427
                * writes 0xFF (everything off) and every driver routine CLEARS its bit to
                * fire (AND #~bit at 05D7, 05FD, 0623, ...) and SETs it to release (OR #bit).
                * Writing the driver-local shadow rather than coreGlobals.solenoids is
                * deliberate: SLEIC_interface_update copies it over once a VBLANK.
                *
                * Bits 0/1, 2/3 and 4/5 are three COMPLEMENTARY PAIRS, bits 6 and 7 two
                * singles (F7).  Each pair is one dual-wound flipper driven one winding at a
                * time -- the button handlers sub_1292 / sub_12D8 choose between the two on
                * that flipper's EOS contact (C0DB bit 6 / bit 7), i.e. one winding while it
                * is travelling and the other to hold it up, and sub_064D / sub_06C2 release
                * both.  WHICH physical coil sits on each bit is in neither ROM, so the
                * mapping is the plain one, driver bit b -> solenoid b+1, and no coil is
                * named here -- unlike Sleic Pin-Ball, whose manual numbering is verified */
      locals.solenoids = (locals.solenoids & ~(UINT32)0x00ff) | (UINT32)(data ^ 0xff);
      break;
    case 0x06: /* port 0x86: driver latch B -> solenoids 9-16.  Same active-low convention
                * (boot_port_init 042C also writes 0xFF), but all eight bits are independent:
                * fired at 0706-07D1, released at 081B-0892, plus the timed auto-release path
                * at 0ADA-0C51 inside the Z80's IRQ handler (F7) */
      locals.solenoids = (locals.solenoids & ~(UINT32)0xff00) | ((UINT32)(data ^ 0xff) << 8);
      break;
    case 0x07: /* port 0x87: NOT a driver output.  Low nibble = direct_input_scan's 16-way
                * mux index (0DEA-0E01; the selected input comes back on port-0x01 bit 5, and
                * the scan is gated off entirely by IOMOON_PORT04_IDLE bit 0), bits 4 and 5
                * are firmware flags (2851/2831 and port87_bit5_set/_clear), and
                * boot_port_init 0420 starts the port at 0x30.  Nothing to drive */
      break;
    default:
      logerror("iomoon Z80 write port %02x = %02x\n", 0x80 + offset, data);
  }
}

static PORT_READ_START(SLEIC2_Z80_readport)
  {0x00,0x07, iomoon_z80_read},
MEMORY_END

static PORT_WRITE_START(SLEIC2_Z80_writeport)
  {0x80,0x87, iomoon_z80_write},
MEMORY_END

static MACHINE_INIT(SLEIC) {
  /* The memset covers everything in locals -- the DMD latch and its TTL, the OKI
   * phrase/strobe state, the J1 link latches, the YM3812 A0 select and the SLEIC3
   * interrupt accumulators all start at zero on every machine start */
  memset(&locals, 0, sizeof locals);
  memset(&sleic_io, 0, sizeof sleic_io);
  /* locals.dmdEqualFields stays 0 here (Bike Race weighting); SLEIC1 and SLEIC2 each state their own below */
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
   *     PLUS bit 0x40 (code 0x30, key '-') and counts missing balls -- reply adds
   *     0x5C "FALTA 2 BOLAS". "BOLAS OK" needs two optos INCLUDING the '-' sensor
   *     (combos 0x20+0x40 or 0x40+0x80), so standalone-test by holding '-' with 8 or 9;
   *     holding only 8+9 (no '-') never reports OK. (Verified against the disassembly of both
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
  /* Everything Io Moon keeps in locals has just been zeroed by machine_init_SLEIC's
   * memset, so only the values that are NOT zero are stated here.  Three that the zero
   * IS the answer for, because they are easy to mistake for oversights:
   *   the OKI latch starts empty rather than at the 0x80 boot_init writes -- 0x28 below
   *     already has ST released, so nothing can be triggered before the firmware's own
   *     D00C6 write arrives, and phrase 0 would be a stop in any case;
   *   the interrupt accumulators and their pending latches start at rest;
   *   the J1 latches start empty, so port-0x01 bit 1 reads free for the Z80's very first
   *     send -- boot_port_init 041B calls host_send_c008_b with interrupts still off, and
   *     that routine spins on the bit before it does anything else */
  locals.iomPcs0 = 0x28;
  iomoon_set_gfx_bank(locals.iomPcs0);

  /* Panel weighting, from the IC23 dump (asm/pic16c57_annotated.asm) rather than by
   * analogy with the I8039 games: the raster's per-row hold delay is 200 counts for the
   * plane PORTB walks first (0x00-0x3F = 7000:0000, plane 0) against 30 counts for the
   * second (0x40-0x7F = 7000:0200, plane 1) -- 6.7:1.  Two consequences, one settled and
   * one approximated:
   *
   *   SETTLED: the planes are strongly asymmetric, so this panel really does show a
   *     weighted 2-bit value and not Sleic Pin-Ball's "lit / lit brightly" pair, and
   *     plane 0 is unambiguously the MSB.  That is what locals.dmdEqualFields = 0
   *     selects, and it is stated here rather than inherited from the base init so the
   *     PIC is on record as the reason.
   *
   *   APPROXIMATED: the DUTY CYCLES the ratio implies are 0, 30/230 = 13%, 200/230 = 87%
   *     and 100%, whereas MACHINE_INIT(SLEIC)'s CORE_DMD_PWM_PREINTEGRATED_LINEAR_4 maps
   *     the four levels this driver hands core_dmd_submit_frame onto 0, 1/3, 2/3 and 1.
   *     The ORDER is right and the ends are exact; the two middle levels are pulled
   *     toward each other.  The core takes no per-plane weight -- its filter/combiner
   *     pairs are either fixed FIR patterns for a named board or the two preintegrated
   *     LUTs -- so LINEAR_4 is the closest available model, and the alternative
   *     (PREINTEGRATED_SAM's 1/12-step LUT, whose indices 2 and 13 would land within 4%
   *     of 13% and 87%) is rejected because it would put values outside 0..3 into the raw
   *     frames every downstream consumer of a 4-shade DMD reads.  Revisit if the core
   *     ever grows a weighted 2-plane combiner */
  locals.dmdEqualFields = 0;
  /* The ball trough starts EMPTY, and at the default "Balls" = 0 it stays that way: the
   * model is opt-in and the frontend owns those contacts.  It is filled by the first
   * SWITCH_UPDATE only if the operator has asked for it, because the complement comes
   * from an input port and the ports are not readable this early */
  iomoon_ball_reset();
  /* No coin part-way through its pulse train, and no key remembered as already down */
  iomoon_coin_reset();
}

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) switch input.  Where each byte comes from is F5; which physical
/  contact sits behind each code is F5's own open gap, so what is claimed here is the
/  code map (exact, out of the Z80 ROM) and not a contact list.
/
/    swMatrix[1..6]  the 6 x 8 matrix, column c selected by the port-0x82 one-hot strobe
/                    and read back on port 0x02.  Code = 0x0A + 8c + b for c = 0..4 and
/                    0x34 + b for c = 5 -- exactly regular, 48 positions.
/    swMatrix[9]     the six port-0x03 cabinet inputs (bit -> code above iomoon_z80_read)
/    swMatrix[10]    the port-0x04 config byte (see IOMOON_PORT04_IDLE); nothing is mapped
/                    into it, because bit 7 must stay high and the rest is unknown
/
/  FIVE of the 48 matrix positions never produce a byte, and that is firmware, not a gap
/  in the driver: codes 0x13 and 0x38-0x3B dispatch to sub_1348, which is a bare RET.
/  (0x38-0x3B do reach the 80188, but only as command-table re-sends -- F5.)
/-----------------------------------------------------------------------------------*/

/* Playfield test keys, one per matrix position, so the service menu's contact test can
 * exercise every contact standalone.  Columns 0-4 keep the Bike Race key layout; column 5
 * only maps the four positions whose codes (0x34-0x37) the firmware actually sends.
 *
 * Two positions are deliberately NOT on the key their neighbours suggest, because those
 * two keys already belong to a cabinet button: 7 is the test/service key (sleic.h) and T
 * is MAME's IPT_TILT default, so col4 0x30 sits on '-' and col5 0x34 on '=' -- still the
 * same number row, just past the digits.  Everything else here is free of both the
 * cabinet block and the MAME UI keys */
static const struct { int key; UINT8 col; UINT8 bit; } iomoon_pf_keys[] = {
  {KEYCODE_Q,1,0x01},{KEYCODE_W,1,0x02},{KEYCODE_E,1,0x04},{KEYCODE_R,1,0x08}, /* col0 0x0A-0x0D */
  {KEYCODE_Y,1,0x10},{KEYCODE_U,1,0x20},{KEYCODE_I,1,0x40},{KEYCODE_O,1,0x80}, /* col0 0x0E-0x11 */
  {KEYCODE_A,2,0x01},{KEYCODE_S,2,0x02},{KEYCODE_D,2,0x04},{KEYCODE_F,2,0x08}, /* col1 0x12-0x15 (0x13 = RET) */
  {KEYCODE_G,2,0x10},{KEYCODE_H,2,0x20},{KEYCODE_J,2,0x40},{KEYCODE_K,2,0x80}, /* col1 0x16-0x19 */
  {KEYCODE_Z,3,0x01},{KEYCODE_X,3,0x02},{KEYCODE_C,3,0x04},{KEYCODE_V,3,0x08}, /* col2 0x1A-0x1D */
  {KEYCODE_B,3,0x10},{KEYCODE_N,3,0x20},{KEYCODE_M,3,0x40},{KEYCODE_L,3,0x80}, /* col2 0x1E-0x21 */
  {KEYCODE_0_PAD,4,0x01},{KEYCODE_1_PAD,4,0x02},{KEYCODE_2_PAD,4,0x04},{KEYCODE_3_PAD,4,0x08}, /* col3 0x22-0x25 */
  {KEYCODE_4_PAD,4,0x10},{KEYCODE_5_PAD,4,0x20},{KEYCODE_6_PAD,4,0x40},{KEYCODE_7_PAD,4,0x80}, /* col3 0x26-0x29 */
  {KEYCODE_0,5,0x01},{KEYCODE_2,5,0x02},{KEYCODE_3,5,0x04},{KEYCODE_4,5,0x08}, /* col4 0x2A-0x2D */
  {KEYCODE_6,5,0x10},{KEYCODE_8,5,0x20},{KEYCODE_MINUS,5,0x40},{KEYCODE_9,5,0x80}, /* col4 0x2E-0x31 */
  {KEYCODE_EQUALS,6,0x01},{KEYCODE_8_PAD,6,0x02},{KEYCODE_9_PAD,6,0x04},{KEYCODE_MINUS_PAD,6,0x08}, /* col5 0x34-0x37 */
};

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) ball trough -- OPT-IN, AND OFF BY DEFAULT.
/
/  READ THIS FIRST.  PinMAME's contract is that switches are switches: the FRONTEND closes
/  them.  Under VPinMAME a table script owns the trough opto/contact numbers and reports a
/  ball arriving or leaving; standalone, the matrix test keys do it by hand.  A driver that
/  seeds balls of its own invents playfield state the frontend then fights, so this model
/  is not the default and the sister machines have no equivalent -- Bike Race sits on
/  "FALTA 1 BOLA" until something closes its trough contacts, which is exactly right.
/
/  So the four contacts below are ORDINARY SWITCHES unless the operator asks for the
/  model.  The knob is the standard simulator port's "Balls" setting, which every PinMAME
/  driver already carries (sim.h, and SLEIC2_INPUT_PORTS_START in sleic.h):
/
/    Balls = 0   DEFAULT.  The model is off.  swMatrix[1] bits 0-3 are driven only by the
/                matrix inputs -- the Q/W/E/R test keys, or a VPinMAME table script.  No
/                seeding, no kicker, and the "Drain ball in play" and "Shoot Ball" inputs
/                do nothing at all (iomoon_ball_update returns before it reads them).
/    Balls = 3   The internal three-ball model below, for standalone desktop play where
/                nothing else is going to close those contacts.  1 and 2 clamp up to 3
/                (see the clamp comment); 4..7 also give 3, since the trough only has
/                three contacts.
/
/  Turning it on or off mid-session is safe: the off path drops the whole state, so a
/  serve caught half-way is not replayed into the first frame after it is switched back on.
/  Turning it OFF during a game does leave the firmware looking for balls that are now
/  nobody's job to present, which is the same stall a real cabinet has when the balls are
/  taken out mid-game -- switch it before you start, not during.
/
/  WHAT THE MODEL IS FOR, when it is on.  The firmware BLOCKS on these contacts.  Four of
/  the 48 matrix contacts are ball-handling contacts rather than
/  playfield events, and both CPUs stop dead until they read right:
/
/    swMatrix[1] bits 0-2   the three trough contacts, codes 0x0A-0x0C
/    swMatrix[1] bit 3      the ball-exit contact, code 0x0D
/
/  That these four are ball handling and not playfield is the ROM's statement, not an
/  inference from the manual: the in-game switch dispatcher sub_D7636 subtracts 0x0E
/  before its 55-entry table (D7652: SUB AX,0000E / CMP BX,00036), so codes 0x0A-0x0D
/  reach no playfield handler at all and exist only for the four commands below.  The
/  service manual's matrix list agreeing -- its first four entries are "contacto salida
/  bolas 1/2/3" and "bola fuera" -- is corroboration for the group, not for which contact
/  is which; the ROM settles that on its own further down.
/
/  The four commands, Z80 handlers out of the 256-entry table at $2000:
/
/    0xEA -> 2A45  count the trough.  Counts the CLOSED contacts among C0DB bits 0-2 and
/                  adds bit 3, then answers 0x48 (none) 0x38 (1) 0x39 (2) 0x3A (3).  The
/                  80188 stores that in [413C:00F9] through the reply table at CS:04EE4
/                  (sub_DC10D DC144-DC17D).
/    0xEB -> 2AB0  the same count on column 4 for the second ball device -> [413C:00F8].
/    0xE9 -> 2B03  SERVE.  If C0DB bit 3 is already closed it answers 0x45 at once;
/                  otherwise it reports driver state 0 busy (sub_08A7) and polls column 0
/                  until bit 3 closes, five tries of 0x3E8 ticks each, then gives up with
/                  0x4A.  So the machine expects the trough kicker to put a ball ON the
/                  ball-exit contact.
/    0xEF -> 2BC7  BALLS HOME.  A closed loop -- test the trough (sub_2C1F), run the
/                  ball-search coil sequence (sub_2CFB), wait, repeat -- whose ONLY exit
/                  is sub_2C1F returning 0, i.e. three adjacent trough contacts closed
/                  (2C2B: AND 007 / 2C35: AND 00E, either one all-zero).  It answers 0x45
/                  and returns only then.
/
/  and the ball-start path that drives them, sub_DC4C9:
/
/    DC4D0: CALL sub_DC410            ; 0xEA, count the trough -> [00F9]
/    DC4D4: CALL sub_DC2FC            ; 0xEB, count device 2   -> [00F8]
/    DC4DC: CMP ES:000F9,0 / JE DC4FE ; nothing in the trough -> ball search
/    DC4F9: CMP DX,3 / JNL DC522      ; [00F9]+[00F8] >= 3 -> skip the search
/    DC4FE: PUSH 0EF / CALL qout_push
/    DC507: CALL sub_DC075 / OR AX,AX / JE DC507   ; wait for 0x45 -- NO TIMEOUT
/    DC514: MOV ES:000F9, 003         ; after a search the trough IS the three balls
/    DC522: CALL sub_DC47E            ; 0xE9, serve one ball
/    DC52B: DEC [00F9]
/
/  With no ball modelled at all the trough answers 0x48, DC4DC takes the search branch,
/  and the two CPUs deadlock: the 80188 spins at DC507 for a reply the Z80 will never
/  send, and the Z80 never leaves 2BC7, so main_loop 0D4C stops running and with it
/  input_port03_read_tick 2E62 and the whole switch scan.  That is why cabinet buttons
/  and flippers die the moment a game starts, and it is why this model exists.
/
/  THE DRAIN.  The first trough contact is also the ball-over sensor, and that is what
/  fixes which end of the trough is which.  Its per-bit routine is not like the other
/  47: instead of sending its own code it sends 0x43, and only while the game has armed
/  it --
/
/    316D: LD A,#$0A / LD (C0FC),A / JP sub_161E
/    161E: LD A,(C068) / AND A / JP NZ,1642   ; test mode: send 0x0A like any contact
/    1625: LD A,(C054) / AND A / RET Z        ; not armed -> report NOTHING
/    162A: LD A,(C04B) / AND A / RET NZ       ; 200-tick lockout still running
/    162F: C049 = 0x00C8 / C04B = 0xFF        ; re-arm the lockout
/    163A: LD A,#$43 / LD (C0FC),A / JP host_send_c0fc
/
/  C054 is armed by 80188 command 0xF3 (Z80 2A3A) and disarmed by 0xF4 (2A40), and
/  sub_DC74B pushes 0xF3 immediately after serving the ball (DC779).  On the 80188 side
/  0x43 is not an ordinary event: BOTH in-game tables send it to sub_D92C0, the ball-over
/  routine -- the dispatch table at CS:0527 (sub_D7636, entry for 0x43 -> D7666) and the
/  ball-in-play wait table at CS:07AE (sub_D7A12, 0x43 -> D7A3E), where every other
/  playfield code merely returns 1.  Note also that 161E's normal-play path never calls
/  sub_3394, so unlike contacts 1-3 (sub_164A / sub_165A / sub_166A, which set the
/  reported mask C0E8 and go quiet) this contact re-reports for as long as it is closed.
/
/  So contact 0 is the trough ENTRY: a ball only rests on it when it is home, and a ball
/  arriving there is the drain.  Balls therefore fill the trough from contact 2 down, and
/  the contact a served ball vacates is contact 0.
/
/  What is modelled, and why each transition is here:
/
/    at rest            three balls on the trough contacts.  The complement comes from the
/                       "Balls" setting of the standard simulator input port, which the
/                       game already carries; the firmware's own number is 3 (DC514 and
/                       DC14D both store 3).  0 -- the DEFAULT -- turns the whole model
/                       off and hands these contacts back to the frontend.
/    serve              on command 0xE9 reaching the Z80, after a short mechanical delay,
/                       one ball leaves the trough and closes the ball-exit contact.  That
/                       is what 2B03 is waiting for and it is what makes it answer 0x45.
/    launch             the ball rolls off the exit contact into play.  "Shoot Ball" does
/                       it at once; otherwise it happens on its own after a delay, because
/                       the firmware demands it: with bit 3 held closed the ball-start path
/                       runs again every ~4 s (measured) instead of settling into play.
/                       WHICH of the two the real cabinet does -- a plunger the player
/                       pulls or a coil that launches by itself -- is not in either ROM,
/                       so both are offered and neither is claimed.
/    drain              the ball in play returns to the trough and closes contact 0, which
/                       is what the firmware reads as ball-over (above).  This is the ONE
/                       thing a ball does not do by itself, so it is an input: "Drain ball
/                       in play" on the cabinet port (sleic.h).  The driver does not
/                       announce the ball over -- it puts the ball back in the trough and
/                       lets the Z80's own 161E send the 0x43.
/
/  Nothing here writes firmware memory; it only presents contacts, and every contact it
/  presents is one the real machine has -- and with "Balls" at its default 0 it presents
/  none of them, so the frontend's own trough switches are the only thing driving them.
/-----------------------------------------------------------------------------------*/
#define IOMOON_TROUGH_COL   1     /* swMatrix index of Z80 switch column 0              */
#define IOMOON_TROUGH_BITS  0x07  /* bits 0-2, codes 0x0A-0x0C                          */
#define IOMOON_EXIT_BIT     0x08  /* bit 3, code 0x0D                                   */
#define IOMOON_TROUGH_MAX   3     /* three contacts, so three balls can rest on them     */

/* Frame counts at the 60 Hz VBLANK this runs on.  Both are the mechanical time a ball
 * takes, not a firmware requirement, so both are generous against the firmware's own
 * windows: the serve has 0x3E8 Z80 ticks ~ 1.0 s per try before 2B03 gives up, and the
 * ball-start path re-runs about every 4 s if the exit contact never clears */
#define IOMOON_KICK_FRAMES    8   /* kicker fires -> ball on the exit contact  (~0.13 s) */
#define IOMOON_LAUNCH_FRAMES 90   /* ball sits at the exit -> into play        (~1.5 s)  */

/* The model's own state is locals.iomBalls, declared with the locals struct */

/* Called from MACHINE_INIT.  Also the whole of the model-off path: zero here means the
 * model owns no contact and no ball, and nothing it presents can survive being switched
 * off.  "seeded" doubles as the on/off flag -- it is set only by iomoon_ball_update with
 * "Balls" > 0, and cleared again the moment that setting goes back to 0 */
static void iomoon_ball_reset(void) {
  memset(&locals.iomBalls, 0, sizeof locals.iomBalls);
}

/* Called from the 80188 -> Z80 strobe with the command byte, i.e. exactly when the Z80's
 * NMI takes it.  0xE9 is the only command that moves a ball; 0xEF cannot be served by
 * moving one, since its exit condition is the trough being FULL and the balls it is
 * looking for are the ones already in play.
 *
 * The "seeded" test is what keeps the kicker out of the way with the model off: with
 * "Balls" = 0 the serve command is simply not acted on, and whatever closes the trough
 * contacts -- a table script, or the matrix test keys -- answers it instead */
static void iomoon_ball_command(UINT8 cmd) {
  if (cmd == 0xe9 && locals.iomBalls.seeded && !locals.iomBalls.atExit && !locals.iomBalls.kick
      && locals.iomBalls.inTrough > 0)
    locals.iomBalls.kick = IOMOON_KICK_FRAMES;
}

/* Called once a frame from SWITCH_UPDATE(SLEIC2) AFTER the playfield key loop, and it ORs
 * its contacts in rather than assigning them: a key held on one of those four positions is
 * a contact stuck closed, which is exactly what the service menu's contact test wants to
 * see, and the model has no business overriding it.
 *
 * shoot = the simulator port's "Shoot Ball", drain = the cabinet port's "Drain ball in
 * play", balls = its "Balls" setting.  balls = 0 is the DEFAULT and means model off */
static void iomoon_ball_update(int balls, int shoot, int drain) {
  UINT8 bits;
  /* MODEL OFF -- the default, and the only behaviour a frontend ever sees.  Return before
   * anything is seeded, before the kicker is stepped and before "shoot" or "drain" is
   * looked at, so those two inputs are inert rather than half-live, and swMatrix[1] bits
   * 0-3 are left exactly as the matrix inputs set them.  Drop the whole state, not just
   * the seeded flag: a half-finished serve left behind would otherwise be applied to the
   * first frame after the setting is turned back on */
  if (balls <= 0) { iomoon_ball_reset(); return; }
  /* MODEL ON.  Fewer than three cannot work and is not a machine state a user should be
   * able to select by accident: the trough test sub_2C1F only ever clears with three
   * adjacent contacts closed, so a two-ball trough leaves command 0xEF (Z80 2BC7) looping
   * with no give-up path -- which is what a real cabinet missing a ball does, but a UI
   * setting should not brick the emulation.  Clamp up; 0 above is the deliberate way off */
  if (balls < IOMOON_TROUGH_MAX) balls = IOMOON_TROUGH_MAX;
  if (!locals.iomBalls.seeded) {
    locals.iomBalls.seeded   = 1;
    locals.iomBalls.inTrough = balls > IOMOON_TROUGH_MAX ? IOMOON_TROUGH_MAX : balls;
  }

  if (locals.iomBalls.kick && --locals.iomBalls.kick == 0 && locals.iomBalls.inTrough > 0) {
    locals.iomBalls.inTrough--;          /* the kicker has thrown it clear of the trough */
    locals.iomBalls.atExit = 1;
    locals.iomBalls.dwell  = 0;
  }
  if (locals.iomBalls.atExit) {
    locals.iomBalls.dwell++;
    if (shoot || locals.iomBalls.dwell >= IOMOON_LAUNCH_FRAMES) {
      locals.iomBalls.atExit = 0;
      locals.iomBalls.inPlay++;
    }
  }
  /* One press, one ball: the drain is an event, not a level */
  if (drain && !locals.iomBalls.drainHeld && locals.iomBalls.inPlay > 0) {
    locals.iomBalls.inPlay--;
    if (locals.iomBalls.inTrough < IOMOON_TROUGH_MAX) locals.iomBalls.inTrough++;
  }
  locals.iomBalls.drainHeld = drain ? 1 : 0;

  /* Balls stack AWAY from the entry, so contact 0 is the last to fill and the first to
   * empty: n balls close the TOP n of the three contacts.  That order is not a choice --
   * the Z80's per-bit routine for contact 0 is the one that reports a drained ball
   * (sub_161E, see above), so contact 0 has to be the one a returning ball closes and
   * therefore the one that is open while a ball is out on the playfield.  Filling from
   * the other end instead reports a drain the instant the game arms the monitor, and the
   * ball ends about a second after it starts -- measured, before this was understood. */
  bits = (UINT8)(((1u << locals.iomBalls.inTrough) - 1u)
                 << (IOMOON_TROUGH_MAX - locals.iomBalls.inTrough)) & IOMOON_TROUGH_BITS;
  if (locals.iomBalls.atExit) bits |= IOMOON_EXIT_BIT;
  coreGlobals.swMatrix[IOMOON_TROUGH_COL] |= bits;
}

/*-------------------------------------------------------------------------------------
/  Io Moon (SLEIC2) coin input: ONE line, and one coin is a PULSE TRAIN on it.
/
/  The cabinet has a single electronic coin mechanism -- one entry in the manual's board
/  list, designator N-50, on the door -- and it lands on the direct-input connector J7
/  (manual 7.2.2.2) as Z80 port 0x03 bit 5.  There is no second or third coin switch
/  anywhere in the Z80 ROM: a sweep of the whole listing for the code-send pattern finds
/  73 sites and exactly one of them is a coin (sub_0D15 0D15 -> 0D3C, code 0x32; 0x33 in
/  test mode), and the 80188 has no other credit source (F5, F11).  So "coin slot 1/2/3"
/  is NOT a wiring fact here and cannot be put on three keys as three switches.
/
/  What the three per-country "coin values" are instead is DENOMINATIONS on that one line,
/  told apart by HOW MANY pulses the validator emits.  The firmware never sees a coin --
/  it counts pulses and prices the running total:
/
/    D000:0190   the NMI tests the inbound byte for 0x32 and, alone among the codes, does
/                not queue it: it increments the pulse counter [4000:1144].
/    sub_D800A   folds that count into the accumulator and calls the per-country pricing
/                routine (sub_DCD9E for country 5, else sub_DD03D).
/    sub_DD03D   divides the accumulated pulses by each coin value in turn, LARGEST FIRST
/                (413C:00AF, then 00AE, then 00AD; the country-5 routine starts one higher
/                at 00B0), multiplies each whole coin it finds by that coin's credit value
/                (00AB / 00AA / 00A9, and 00AC), banks it through sub_DD1C1 and keeps the
/                remainder.
/
/  Cross-check the pulse counts against the manual's coin table and the unit is obvious:
/
/    country      coin values (pulses)          credits        one pulse =
/    UK        3 (30p)   5 (50p)  10 (GBP1)      1 / 2 / 5        10p
/    Germany   1 (1 DM)  2 (2 DM)  5 (5 DM)      1 / 3 / 8        1 DM
/    Italy     1 (500 L) 2 (1000L) 4 (2000 L)    1 / 3 / 7        500 L
/    Holland   2 (1 Fl)  5 (2,5Fl) 10 (5 Fl)     1 / 3 / 7        0,50 Fl
/    Spain     2 (50pta) 4 (100)   8 (200) 20 (500)  1/3/7/18     25 pta
/
/  which is an ordinary multi-coin validator with a value-scaled pulse output, and it
/  settles what the coin KEY has to mean.  A player cannot emit one pulse -- there is no
/  coin worth one pulse in the UK, Holland or Spain -- so a key that emits a single pulse
/  models something the cabinet cannot do, and under those countries it takes two or three
/  presses before the firmware sees a whole coin (owner report, and reproduced: under the
/  old UK default three presses banked one credit).  The key therefore inserts a COIN: it
/  starts a burst of as many pulses as that country's coin is worth, and the firmware
/  prices it exactly as it prices the real mechanism.  One press = one coin = at least one
/  credit under every one of the eight country settings.
/
/  The pulse count is read from the firmware's own live table rather than a copy of the
/  eight presets, because the table is what the machine is actually pricing against: it is
/  loaded from NVRAM 0x1C4-0x1CF at boot (sub_D6A36) and rewritten whenever the country
/  DIP changes or the operator edits the values in the service menu, and a driver-side copy
/  would go stale on both.  Before the boot load has run it reads zero, and one pulse is
/  the honest fallback (the machine cannot price anything at that point either -- the
/  pulses simply pile up in [4000:1144] until it can).
/
/  The BURST TIMING is the Z80's, not a guess.  sub_0D15 debounces the contact for 0x32
/  ticks of the C046 counter before it sends the code, and that counter is stepped from the
/  Z80's own periodic interrupt (SLEIC2 runs it at 8000000/8192 = 977 Hz), so a pulse must
/  hold the contact for 50/977 = 51 ms = about 3.1 frames.  Measured on this driver: a
/  3-frame hold sends NOTHING and a 4-frame hold sends every pulse, so 5 frames closed is
/  the shortest hold with real margin.  The contact must then open again before the next
/  pulse, because sub_33FA latches the input into the C0F8 mask on the way out and
/  input_port03_read only clears it once the contact physically opens (F5: one press, one
/  code, no auto-repeat); 3 frames open is comfortably enough.  8 frames per pulse is also
/  about what the mechanism itself does -- a validator of this class pulses at roughly
/  100 ms -- so the coin sound and the credit ticking up land at a plausible rate. */
#define IOMOON_COIN_VALUES 0x4146d /* 413C:00AD, the three coin values in PULSES (00AF is
                                    * the largest; country 5 has a fourth at 00B0)       */
#define IOMOON_COIN_HOLD   5       /* frames the contact is closed, per pulse            */
#define IOMOON_COIN_GAP    3       /* frames it is open again before the next pulse      */
#define IOMOON_CAB_COIN    0x0200  /* the COIN bit of the cabinet inport (sleic.h)       */

/* The train's own state is locals.iomCoin, declared with the locals struct */

static void iomoon_coin_reset(void) { memset(&locals.iomCoin, 0, sizeof locals.iomCoin); }

/* Pulses the mechanism emits for the coin the player just put in.  Only the smallest
 * denomination has a key (see SLEIC2_CABPORT in sleic.h): the firmware prices the running
 * pulse total rather than individual coins, so repeated small coins reach the same credit
 * levels a larger coin does */
static int iomoon_coin_pulses(void) {
  const int n = cpu_readmem20(IOMOON_COIN_VALUES);
  return (n > 0 && n <= 40) ? n : 1; /* 0 = the boot load has not run yet; 20 is the most
                                      * any preset asks for (Spain's 500 pta)            */
}

/* Called once a frame with the cabinet inport word.  Returns the state of the coin
 * contact for this frame: a fresh press queues a whole coin's worth of pulses, and the
 * queue is emitted one pulse at a time whatever else the player does meanwhile (pressing
 * again mid-burst adds a second coin, exactly like feeding the mechanism does) */
static int iomoon_coin_update(UINT16 keys) {
  int closed;
  if ((keys & IOMOON_CAB_COIN) && !(locals.iomCoin.lastKeys & IOMOON_CAB_COIN))
    locals.iomCoin.pending += iomoon_coin_pulses();
  locals.iomCoin.lastKeys = keys;
  if (locals.iomCoin.pending <= 0) { locals.iomCoin.phase = 0; return 0; }
  closed = (locals.iomCoin.phase < IOMOON_COIN_HOLD);
  if (++locals.iomCoin.phase >= IOMOON_COIN_HOLD + IOMOON_COIN_GAP) {
    locals.iomCoin.phase = 0;
    locals.iomCoin.pending--;
  }
  return closed;
}

static SWITCH_UPDATE(SLEIC2) {
  unsigned i;
  /* Model OFF unless the ports say otherwise, and inports == NULL is exactly the case
   * that must stay off: core.c passes NULL when the host owns the keyboard (VPinMAME),
   * i.e. when a table script is driving the trough switches itself */
  int balls = 0, shoot = 0, drain = 0;
  if (inports) {
    const UINT16 in = inports[CORE_COREINPORT];
    /* The ball trough's three inputs, acted on after the key loop below.  "Balls" and
     * "Shoot Ball" are the standard simulator port the game already carries (sim.h,
     * SLEIC2_INPUT_PORTS_START); "Drain ball in play" is Io Moon's own cabinet bit
     * 0x1000 (sleic.h), which is how a run gets through a whole ball.  "Balls" is the
     * opt-in: 0 (the DEFAULT) leaves the trough contacts to the frontend and makes the
     * other two inert */
    balls = SIM_BALLS(inports[CORE_SIMINPORT]);
    shoot = (inports[CORE_SIMINPORT] & SIM_SHOOTERKEY) ? 1 : 0;
    drain = (in & 0x1000) ? 1 : 0;
    /* Cabinet inputs -> swMatrix[9] = Z80 port 0x03, one bit per input.  The bit -> CODE
     * map is exact (F5/F14, see iomoon_z80_read).  The bit -> BUTTON map is no longer a
     * choice for four of the six: the 80188 firmware says outright what it does with each
     * code, and for COIN and TILT it says the OPPOSITE of what this driver assumed until
     * now.  What each code reaches, traced from the code that consumes it:
     *
     *   bit 5 -> COIN.  Its code 0x32 is the ONE code the NMI does not queue: D000:0190
     *            tests for it and counts it in [4000:1144] instead.  sub_D800A D800A then
     *            folds that count into the pulse accumulator 413C:00D5 and hands it to the
     *            per-country pricing routine (sub_DCD9E / sub_DD03D, chosen on the country
     *            byte [4000:1001]), which divides the pulses by the coin values at
     *            413C:00AD/00AE/00AF, multiplies by the credit values at 413C:00A9/00AA/
     *            00AB and calls sub_DCFAB / sub_DD1C1 to bank the result.  That is a
     *            single-line coin validator emitting a pulse train per coin, and it is the
     *            only path in the ROM that adds credits from a switch.  The award itself
     *            is nvstore_write_triple_83 (F10) with the running total cached in
     *            413C:00D4, and sub_D0B70(0x0A) is the coin sound on the partial.
     *            This bit is therefore NOT driven from the key directly: one press is one
     *            COIN, and iomoon_coin_update turns it into that coin's pulse train.
     *   bit 0 -> TILT ("falta").  Its code 0x3E dispatches through the in-game table at
     *            CS:0527 (entry 48) to sub_D9EBB, which counts down the warning counter
     *            [4134:0033] -- loaded at every ball start from NVRAM byte 0x42 minus one
     *            (DBE17, DBF7F), i.e. the FALTA adjustment -- and on the last one stops the
     *            music (fm_song_select(0)), clears both sprite and background planes,
     *            plays sound 0x0E and pushes the driver-disable command 0xF2.  The Z80's
     *            own 3000-tick lockout in sub_125B is the tilt debounce, not a coin
     *            lockout.  Bike Race puts its tilt on the same port-0x03 bit 0.
     *   bit 1 -> TEST, because its code 0x3F is the one that opens the service menu (F14).
     *   bit 4 -> START, because code 0x40 reaches sub_D8066, which is the start handler:
     *            it refuses in attract, refuses at 4 players, refuses on zero credits,
     *            then decrements the NVRAM credit triple and, on the first player, calls
     *            sub_D8154 -> mode 3 -> the in-game song.
     *   bits 3/2 -> left/right flipper: those two handlers (sub_1292 / sub_12D8) fire the
     *            port-0x85 coil pairs directly (sub_05C7 / sub_05ED), and Sleic Pin-Ball's
     *            verified map puts the left flipper on the first pair.  Which of the two
     *            is left is still INFERRED; that they are the flipper buttons is not.
     *            Their codes 0x41/0x42 are also the service menu's scroll and select
     *            (sub_DD480's dispatch at DD501), which is what a flipper button does in
     *            every pinball service menu. */
    CORE_SETKEYSW(in >> 10, 0x01, 9); /* TILT   0x0400 -> bit0 (code 0x3E) */
    CORE_SETKEYSW(in >> 10, 0x02, 9); /* TEST   0x0800 -> bit1 (code 0x3F) */
    CORE_SETKEYSW(in << 1,  0x04, 9); /* R-flip 0x0002 -> bit2 (code 0x42) */
    CORE_SETKEYSW(in << 3,  0x08, 9); /* L-flip 0x0001 -> bit3 (code 0x41) */
    CORE_SETKEYSW(in >> 4,  0x10, 9); /* START  0x0100 -> bit4 (code 0x40) */
    /* COIN 0x0200 -> bit5 (code 0x32), through the pulse-train generator rather than
     * straight from the key: one press is one COIN, which is several pulses on this one
     * line -- see iomoon_coin_update */
    if (iomoon_coin_update(in)) coreGlobals.swMatrix[9] |=  0x20;
    else                        coreGlobals.swMatrix[9] &= ~0x20;
  }
  for (i = 0; i < sizeof(iomoon_pf_keys)/sizeof(iomoon_pf_keys[0]); i++) {
    if (keyboard_pressed(iomoon_pf_keys[i].key))
      coreGlobals.swMatrix[iomoon_pf_keys[i].col] |=  iomoon_pf_keys[i].bit;
    else
      coreGlobals.swMatrix[iomoon_pf_keys[i].col] &= ~iomoon_pf_keys[i].bit;
  }
  /* After the key loop, because it ORs its four contacts in on top: a key held on one of
   * them is a contact stuck closed and the model must not override it */
  iomoon_ball_update(balls, shoot, drain);
}

/* Bike Race (SLEIC3) playfield-matrix test keys. The 40 matrix positions (Z80 switch
 * codes 0x0A-0x31) are read by the 80188 through swMatrix[1..5] (COL0..COL4, selected by
 * the port-0x82 one-hot column strobe); code = 0x0A + 8*(col-1) + row. Mapping every
 * position to a key lets the CONTACTOS self-test verify each contact. COL4 (swMatrix[5])
 * is the trough column. The Z80 cmd-0xD5 ball-status handler monitors COL4 bits 0x04
 * (code 0x2C, key 3), 0x20 = C7 (key 8) and 0x80 = C8 (key 9) on all three sets, plus
 * bit 0x40 (code 0x30, key '-') on bikerac2 AND bikerac3, which share the same F000
 * code revision; see the per-version breakdown in the MACHINE_INIT trough comment above.
 *
 * What that means for filling the trough by hand: bikerace clears with either 8+9 or
 * 8+'-', but bikerac2 and bikerac3 need 8+'-' -- 8+9 alone leaves them sitting on
 * "FALTAN n BOLAS" (measured: 5 and 6 distinct DMD frames against 110 and 111). The
 * SLEIC_TROUGH default mask 0xE0 closes 8, '-' and 9 together and serves all three */
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
  {KEYCODE_6,5,0x10},{KEYCODE_8,5,0x20},{KEYCODE_MINUS,5,0x40},{KEYCODE_9,5,0x80}, /* COL4 0x2E; trough optos: 0x2F=C7(key8) 0x31=C8(key9) both versions, 0x30=key'-' bikerac2-only (+0x2C=key3); 0x30 is on '-' rather than 7 because 7 is the test/service key (sleic.h); see trough notes above */
};

/*-------------------------------------------------------------------------------------
/  Bike Race (SLEIC3) ball-present model -- OPT-IN, AND OFF BY DEFAULT.
/
/  Same bargain as Io Moon's trough model: under a frontend a table script owns the ball
/  optos and reports them, so the driver must not fabricate them, and with "Balls" at its
/  default 0 it does not.  Standalone there is nothing to close them, and the machine sits
/  on "FALTA n BOLAS" until something does -- which is correct, but means holding two
/  matrix keys down for the whole session before a game can be started.  Setting "Balls"
/  to any non-zero value hands that job to the driver.
/
/  This is a SIMPLER model than Io Moon's, deliberately.  Io Moon's trough is three
/  contacts in a line with a kicker command (0xE9) and a drain sensor, so the state a ball
/  is in can be tracked and each contact driven from it.  What Bike Race exposes on COL4
/  is a ball-PRESENT check answered by the Z80's cmd-0xD5 handler, and no serve command
/  has been identified in either Z80 ROM.  So nothing here tracks a ball count or a
/  kicker: it presents the complement the firmware wants to see, and the cabinet port's
/  "Ball out of trough" key lifts it.  Inventing a serve would be inventing mechanics the
/  disassembly does not show.
/
/  THE MASK.  0xE0 closes all three of COL4's monitored optos at once, which satisfies
/  every version's rule without the driver having to know which set is running:
/    bikerace  wants a ball at 0x20 OR 0x80                         -> 0xE0 satisfies it
/    bikerac2  wants two optos INCLUDING 0x40 (0x20+0x40 or 0x40+0x80) -> 0xE0 satisfies it
/    bikerac3  shares bikerac2's Z80 code revision, so the same rule
/  Measured over 2000 headless frames with two coins and START, distinct DMD frames:
/  104 / 107 / 111 with the mask against 5-6 for the sets that need 0x40 without it.
/  See the per-version breakdown in the MACHINE_INIT trough comment above.
/-----------------------------------------------------------------------------------*/
#define SLEIC3_TROUGH_COL   5     /* swMatrix index of Z80 switch column 4 (COL4)       */
#define SLEIC3_TROUGH_BITS  0xE0  /* the three monitored optos, see the mask note above  */

/* Called from SWITCH_UPDATE(SLEIC3) AFTER the playfield key loop, and it ORs its bits in
 * rather than assigning them, for the same reason Io Moon's does: a matrix test key held
 * on one of these positions is a contact stuck closed, which is what the service menu's
 * contact test wants to see, and the model has no business overriding it.
 *
 * balls = the simulator port's "Balls" setting, 0 (the default) meaning model off.
 * out   = the cabinet port's "Ball out of trough", held while the ball is away. */
static void sleic3_ball_update(int balls, int out) {
  if (balls <= 0 || out) return;
  coreGlobals.swMatrix[SLEIC3_TROUGH_COL] |= SLEIC3_TROUGH_BITS;
}

static SWITCH_UPDATE(SLEIC3) {
  unsigned i;
  int balls = 0, out = 0;
  if (inports) {
    balls = SIM_BALLS(inports[CORE_SIMINPORT]);
    out   = (inports[CORE_COREINPORT] & 0x1000) ? 1 : 0;
    /* Cabinet/direct buttons are all on Z80 port 0x03 (swMatrix[9]), bit -> contact:
     *   bit0 = C17 Tilt        (code 0x32, also menu ENTER)
     *   bit1 = C4  Test        (code 0x33 = menu ENTER)
     *   bit2 = C5  Right flipper(code 0x34, menu SELECT)
     *   bit3 = C1  Left flipper (code 0x35, menu scroll DOWN)
     *   bit4 = C2  Start        (code 0x36)
     *   bit5 = C3  Coin/Monedero(code 0x37/0x39) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x01, 9); /* TILT(T)   0x400 -> bit0 (C17 tilt, code 0x32) */
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 10, 0x02, 9); /* TEST(7)   0x800 -> bit1 (C4 Test, code 0x33 = menu ENTER) */
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
  /* After the key loop, because it ORs its optos in on top -- see the comment on it */
  sleic3_ball_update(balls, out);
#ifdef DEBUG_SLEIC
  sleic_debug_switches(5, SLEIC3_TROUGH_BITS); /* COL4 optos, same mask the model uses */
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
  // REPLACE only re-states the CPU type and clock (driver.h: it assigns cpu_type and
  // cpu_clock and nothing else) -- the inherited map, ports and IRQ survive it.  They are
  // spelled out again below anyway, because only the 80188 memory map actually differs
  // from the base SLEIC (Bike Race and Io Moon each state their own map in the SLEIC3 /
  // SLEIC2 blocks below)
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

  // Io Moon switch input: the 6x8 matrix on swMatrix[1..6] (Z80 port-0x82 column strobe,
  // port 0x02 return) and the six cabinet inputs on swMatrix[9] (port 0x03).  Row 10 backs
  // the port-0x04 config byte but nothing is mapped into it -- see IOMOON_PORT04_IDLE for
  // why, and for what has to change instead.  Replaces the base handler, which only fills
  // swMatrix[0] -- a row nothing on this machine reads
  MDRV_SWITCH_UPDATE(SLEIC2)

  // Io Moon main CPU: the 80C188 at IC1 is an N80C188-10, and CLKOUT = 10 MHz is what
  // the timer-0 rate below is computed from, so the emulated clock matches it rather
  // than the base driver's 8 MHz.  REPLACE only re-states type and clock; the map is
  // set separately: LMCS ROM at 0, UMCS ROM at 0xC0000, the four MMCS blocks (work RAM /
  // non-volatile store / banked graphics page / DMD staging) and the PACS peripheral
  // block -- see the SLEIC2_80188_readmem comment block for the chip-select values this
  // hard-wires and where they come from.
  //
  // Interrupts: iomoon_irq_gen replaces the base driver's vector-less 120 Hz IRQ0 pulse
  // (which, having no vector, took an IVT filler entry and derailed the boot) with the
  // real model of findings F1/F3 -- timer 0 on vector 0x08 at 99.18 Hz and INT0 on
  // vector 0x0C at IOMOON_INT0_HZ, both interleaved on one tick.  The NMI is not
  // periodic and is not generated here; see the comment block above iomoon_irq_gen
  //
  // The base block's MDRV_CPU_VBLANK_INT(SLEIC_interface_update, 1) is DELIBERATELY left
  // inherited (REPLACE does not clear it): that handler is what copies tmpLampMatrix ->
  // lampMatrix and locals.solenoids -> coreGlobals.solenoids once a frame, so the lamp and
  // driver decode in iomoon_z80_write writes the driver-local shadows only, exactly as the
  // SLEIC1 and SLEIC3 port handlers do.  Do not re-state or drop it here
  MDRV_CPU_REPLACE("mcpu", I188, IOMOON_CPU_CLOCK)
  MDRV_CPU_MEMORY(SLEIC2_80188_readmem, SLEIC2_80188_writemem)
  MDRV_CPU_PERIODIC_INT(iomoon_irq_gen, IOMOON_IRQ_TICK_HZ)

  // Non-volatile store at segment 5040 (F10), persisted by NVRAM_HANDLER(SLEIC2) and
  // zero-filled on a fresh boot, which is what makes the firmware seed its own factory
  // defaults.  Replaces the base driver's generic_0fill, whose generic_nvram buffer
  // this map does not reference
  MDRV_NVRAM_HANDLER(SLEIC2)

  // I/O Z80 periodic IRQ, ~977 Hz.  The rate is not derived from the Z80's own clock: it
  // is the 8 MHz board crystal divided by 8192 in the IC20/IC21 (74LS393) + IC22 (74LS27)
  // chain on the 16-bit board, delivered to the Z80 board over J3, so it is stated as
  // 8000000/8192 to keep that derivation visible.  (Both ROMs are silent on it -- the
  // source is external to each -- so this is the board inventory's reading, not the
  // disassembly's.)  It replaces the inherited 2500000/2048 Bike Race / Sleic Pin-Ball rate
  //
  // The I/O Z80 also gets its own port map: the J1 byte-port link to the 80188 (port 0x80
  // data + the port-0x81 strobes in, port 0x00 + the PCS1/PCS4 pair out) and the switch
  // matrix.  The base map's z80_read_port returns 0 for port 0x04, whose bit 7 the Z80
  // spins on in selftest_wait_reset, and 0 for port 0x01, whose bit 1 every J1 send waits
  // for -- either alone hangs the I/O board before it can announce itself
  //
  // REPLACE rather than MODIFY, purely so the CLOCK is stated.  In this tree REPLACE sets
  // the CPU type and clock and nothing else (driver.h), so the map, ports and IRQ are
  // unaffected -- but MODIFY left the Z80 running on the base block's 2.5 MHz Bike Race
  // figure, silently, three lines under a comment about an 8 MHz crystal.  The value is
  // unchanged; IOMOON_Z80_CLOCK is where it now lives, with why 4 MHz is the likely
  // correction and why it is not being made in the same breath
  MDRV_CPU_REPLACE("icpu", Z80, IOMOON_Z80_CLOCK)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_z80, 8000000/8192.)
  MDRV_CPU_PORTS(SLEIC2_Z80_readport, SLEIC2_Z80_writeport)

  // Sound: two chips, both driven by the 80188 through the PACS block, and NO third
  // device.  The YM3812 (IC60) carries the FM music at PCS5 0xA0280/0xA0281 (F8, decoded
  // in sleic2_periph_w) and the OKI MSM6376 (IC51) the speech and effects at PCS6 (F9).
  //
  // The MDRV_SOUND_ADD(DAC, ...) line this block used to carry is gone, and it is dropped
  // on evidence rather than on tidiness.  (It was this block's own line, not something
  // inherited: MACHINE_DRIVER_START(SLEIC) adds no sound device at all -- each of SLEIC1,
  // SLEIC2 and SLEIC3 states its own set, which is why removing it here changes nothing
  // for the other two.)  Nothing on this machine is a CPU-written DAC: the
  // only converter on board 011-029A is IC61, a YM3014B, which is the YM3812's own
  // companion serial DAC (it takes the OPL2's serial output, not a bus byte) and is
  // therefore already inside PinMAME's YM3812 emulation; IC62 is an LM324-class op-amp
  // and IC63 an X9C503P digital pot for the FM-vs-OKI balance -- an output stage, not a
  // sound source.  From the software side F8 and F9 between them enumerate every sound
  // write the ROM makes -- 0xA0280/0xA0281, the 0xA0300 latch and the 0xA0000 bit-5
  // /OKCS strobe -- and there is no fourth address and no sample-rate write loop anywhere
  // for a DAC to be driven from.  Nothing in this file ever called DAC_data_w either, so
  // the device only ever added a silent mixer channel.  (SLEIC1 and SLEIC3 keep theirs:
  // their boards were not traced for this and are not this task's to change.)
  MDRV_SOUND_ADD(YM3812, SLEIC2_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC2_okim6376_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC3)
  MDRV_IMPORT_FROM(SLEIC)
  MDRV_SWITCH_UPDATE(SLEIC3) // Bike Race cabinet/direct switches -> swMatrix[9]/[10]

  // Battery-backed NVRAM (28C64A): persisted by NVRAM_HANDLER(SLEIC3), zero-filled by
  // core_nvram on a fresh boot.  A blank chip makes the firmware ask for an operator
  // START at its FABRICA prompt and then seed itself, as on real hardware
  MDRV_NVRAM_HANDLER(SLEIC3)

  // Bike Race main CPU: 80C188 at 10 MHz (work RAM at seg 0, peripherals at
  // 0xA0000, code at 0xE0000). REPLACE only re-states the CPU type and clock; the
  // memory, ports and IRQ generator below are stated because they all differ from
  // the base SLEIC, not because REPLACE cleared them
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
