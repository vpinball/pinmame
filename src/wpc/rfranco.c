// license:BSD-3-Clause

/************************************************************************************************
 Recreativos Franco (Spain)
 --------------------------
   Hardware (from the factory manual, CPU board ref. 53/3291, and confirmed
   against a disassembly of the game ROM):

     CPU:     Intel 8085A @ 5.0688 MHz (X1)
     ROM:     27128 (16K) at IC19, mapped 0x0000-0x3FFF.
              IC14, a second program socket, is unpopulated.
     RAM:     5517 (2K x 8) at IC11, mapped 0xC000-0xC7FF, battery backed
     SOUND:   Intel 8035 @ XTAL/2 (taken from 8085 pin 37, CLK OUT) with its own
              2532 (4K) at IC4, plus 2 x AY-3-8910 @ XTAL/6 (from 8035 pin 1, T0)
              and an LM380 output stage
     LATCH:   4 x Intel 8212. Two of them form the bidirectional command/ack
              path between the two CPUs at 0x8000.
     I/O:     serial. The 8085's SOD pin drives the display chain, its SID pin
              reads the playfield switches back, and any OUT instruction
              generates the shared shift clock.

   Interrupts:
     TRAP    - mains phase detection (zero cross), 100 Hz. Non-maskable. NOTE:
               this is load bearing at boot - see the comment on the reset path
               below.
     RST5.5  - 8212 latch, raised when the sound CPU has taken a command
     RST6.5  - falta (tilt), JD1
     RST7.5  - power fail / emergency stop. Not driven; its handler clears the
               display and halts.

   Switch numbering is col*10 + row + 1, and so is lamp numbering:

      1-2   the two operator switches on the door, in the pseudo column 0 that
            no hardware byte of this machine uses: 1 interruptor de ajuste,
            2 interruptor de test. Closed = the switch is up. This is where
            Williams System 4-11 keeps its coin door too; see RFRANCO_SWAJUSTE
            in rfranco.h.
     11-18  connector JG, read as a byte at 0x4000. In bus order (AD0 first)
            10 puntos, bumper dcho, diana izq, rampa especial izq, diana dcha,
            rampa especial dcha, 100 puntos, bumper izq.
            11 and 17 are each two contacts wired in parallel: 11 is the
            manual's 24+25, the two slingshot contacts, and 17 is 10+21, the
            two 100 puntos lanes. The ROM's contact test flags both pairs.
     21-28  cabinet inputs, returned by the sound CPU from AY IC2 port A.
            The ROM only ever looks at bits 4-7, so 21 is borrowed for falta
            (see SWITCH_UPDATE); 22-24 are unused.
            25 monedero 25, 26 monedero 100, 27 caida de bolas, 28 pulsador
            partidas.
     31-38  first 74165 byte: pasillo inferior dcho, pasillo inferior izq,
            diana izq 1, diana izq 3, diana izq 2, diana izq 4, diana izq 5,
            diana dcha 5.
     41-48  second 74165 byte: diana dcha 4, 3, 2, 1, pasillo superior dcho,
            pasillo superior izq, picabolas, (unwired - IC5's floating SER).

     The order is the ROM's own, taken from the zone 9 contact test table at
     0x34A2 and cross checked against the manual's contact list. It agrees with
     the manual's 74165 wiring at fifteen of the sixteen positions: the ROM has
     JM7 = diana izquierda 3 and JM8 = diana izquierda 2 where the manual's IC6
     table has them the other way round. The ROM wins, since it is what the
     machine's own contact test reports.

   Solenoid numbering is the IC7 4028 output + 1; see the coil decode in
   rfranco_scpu_movx_w. 17-20 are synthesised - the two bumpers and the two
   slingshots have no CPU connection at all.

   One caveat on solenoid 2. The ROM gates 4028 output 1 when it awards a
   replay, and nothing anywhere gates output 0. That much is measured: award a
   special and the coil decode reports output 1. What is on that output is
   another matter. The driver schematic (manual page 17), read by the 4028
   output pin numbers printed on it (pin 3 = Q0 ... pin 5 = Q9), assigns Q0 to
   JL6 TACA and Q1 to JL10 N.C., and the JL connector table on page 16 gives
   pin 10 no wire at all. Taken at face value that would mean the machine's
   knocker is on an output the program never drives, and the output it does
   drive goes nowhere.
   The driver takes TACA to be output 1 anyway: a replay that never bangs is
   not a credible machine, both firmware revisions gate output 1, and the same
   manual's own errata already transposes two adjacent rows of exactly this
   kind of table twice (JA reversed, IC5 pins 10/11 swapped). It is an
   inference, not a measurement, and only a real board can settle it.

   Status: playable, on both sets. A coin gives a credit, the start button
   starts a game and serves a ball, playfield contacts score, the drop target
   banks light their specials, collecting one awards a replay and fires the coil
   the ROM gates for it - taken here to be the knocker, an inference the caveat
   above sets out and only a real board can settle - each ball ends into the
   next with its bonus paid, the last ball ends
   the game and the final score is held; the score and credit displays read
   correctly, lamps and coils follow the ROM's own tables, the two mains phases
   are multiplexed, and all four operator modes on the door switches work.
   tools/rfranco_game.py plays that sequence and asserts on it for both sets.

   Sets:
     supstarf  "Super Star" set 1 (m31-a-01187.ic19). 9 operator adjustment
               zones; this is the revision the factory manual documents.
     supstarfa "Super Star" set 2 (27c128.ic19). The NEWER firmware despite the
               set ordering inherited from MAME: it carries 19 adjustment zones,
               set 1's nine unchanged plus ten more numbered 10-19, and reserves
               an extra 0x30 bytes of NVRAM (its stack base drops from C7FF to
               C7CF). Its jump table at 0x349D has 25 entries, but the zone
               counter at C01D is BCD - 0x33DD steps it and forces 0x0A to 0x10 -
               so entries 9..14 can never be selected and are filled with the
               address of the zone 9 handler. The ten new zones are listed in
               docs/vpx-table-reference.md; two change how the game plays out of
               the box, the 100 puntos lane paying 1000 rather than set 1's 100
               (zone 15) and a diana paying 30000 (zone 14, which set 1 has
               fixed at the same value).

               Both sets play the same game, but set 2 has one behaviour set 1
               does not, and it is worth knowing about because it looks like a
               display fault when it fires. See the note on the contact watchdog
               below.

   Set 2's stuck contact watchdog (there is no equivalent in set 1):

     The routine at 0x3ABF, called from the game loop at 0x0713, watches four
     momentary contacts - 11 (10 puntos), 12 (bumper dcho), 18 (bumper izq) and
     47 (picabolas) - and keeps a two byte counter for each in the NVRAM block at
     C7E0..C7E7, which is inside the C7CF..C7FF area that set 1 uses as stack.
     Each pass, a contact that reads OPEN resets its counter to 0x60 and a
     contact that reads CLOSED increments it (0x3B27). When a counter runs past
     0x7F the strike byte goes to 1 and the routine jumps to the falta handler at
     0x028E. Set 2 gates its reset path on the same four contacts as well, at
     0x0340 and 0x034A.

     So a contact held closed for about 128 consecutive passes - measured here as
     ~27 s from a cold zero filled NVRAM, ~7 s once the counters carry their 0x60
     idle value - faults the machine. That is correct behaviour: a welded contact
     on a bumper or a rollover would otherwise score for ever.

     What it looks like is a display bug. The falta handler latches C01C = 0xFF,
     then 0x031F calls 0x2A1A, which fills all sixteen 8279 display RAM bytes
     with 0xEE, and then 0x0330 waits for a ball to return to the trough. Every
     digit therefore shows the 7447's pattern for 14 and stays there. If set 2
     ever appears to stop updating the score, read C01C before suspecting the
     8279 model: 0xFF means the ROM faulted, and the first thing to check is
     whether the front end is holding one of those four contacts closed. Set 1
     tolerates the same stuck contact indefinitely, which is why this only ever shows up on set 2
 ************************************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "cpu/i8039/i8039.h"
#include "core.h"
#include "rfranco.h"
#include "sndbrd.h"

#define RFRANCO_CPUFREQ 5068800 /* 5.0688 MHz crystal */

/* The mains phase detector feeds TRAP. Spain runs at 50 Hz and the detector
   sees both half cycles, so the interrupt arrives at 100 Hz. */
#define RFRANCO_TRAPFREQ 100

/* RST 5.5 and RST 6.5 are real signals (the 8212 latch and the tilt contact on
   JD1). RST 7.5 is deliberately NOT driven: its handler at 0x0244 clears the
   display, sends sound command 0xCC and then spins forever at 0x026A, so it is
   the power-fail / emergency stop input, not a refresh tick. The ROM opens a
   one-instruction window for it on every TRAP pass (SIM #0B, EI, NOP, DI at
   0x182F) and resets its latch with SIM #1D at 0x194C. */

/* Triggers used to model the 8212's READY handshake - see rfranco_sound_w. One
   is not enough: the deadlock guard behind each byte is a timer that cannot be
   cancelled, so with a single trigger number a stale guard from an earlier byte
   releases a later stall early and the transfer slips. Rotating through a block
   of them keeps every guard bound to the byte that armed it. */
#define RFRANCO_SOUND_TRIGGER 1701
#define RFRANCO_SOUND_TRIGGERS 32
/* Derived bound, not a tuned number.  The guard races the 8035's INT-to-read
   latency: the external interrupt is taken at the first instruction boundary
   with no interrupt in progress and EN I in effect, and the handler reaches
   its MOVX read of the latch (sound ROM 0x02B) 12 machine cycles later.  The
   longest the ROM can run with that recognition blocked is one timer-ISR pass
   of the three-voice player at a chord boundary where all three voice
   countdowns expire in the same tick and voice A also consumes the envelope
   re-arm event - four event-pair fetches plus three full note programmings,
   tune 0x7CA (commands 0x01/0xB0).  With the ISR entry that is 468 machine
   cycles at 5.92us each = 2770us, computed by cycle-accurate emulation of the
   sound ROM over every command's complete run.  Runners-up: 0x11 = 2243us,
   0x00 = 2089us; every non-tune path is <= 953us (command dispatched inside
   the timer ISR's tune-end path), and a polled 0xDD frame byte is taken every
   12 cycles = 71us.  Verified against this driver live: 238k traced
   handshakes match the computed paths within 3 cycles (max observed 971us, on
   the 953us path), and the 0x00 tune's boundary pass measures 339 traced
   cycles of blocked execution - 341 with the untraced vector, exactly the
   computed figure.  Guard = 2770us x 1.44 margin ~= 4000us; the margin covers
   the interleave quantum (~33us/slice) plus model error measured at <=3
   cycles.

   Upper limit: the trigger numbers rotate through RFRANCO_SOUND_TRIGGERS = 32
   and cpu_triggertime cannot be cancelled, so every guard fires ~guard after
   its byte no matter what; it must no longer be pending when its number is
   reused 32 writes later, or it would release a live stall.  The longest
   burst the ROM produces is 23 writes (0xDD + 20 frame bytes + 0xAA + 0x99,
   once per TRAP pass, spanning ~2ms), so 32 consecutive writes always
   straddle at least one full 10ms TRAP period and the guard is safe below
   ~8ms.  (The naive per-byte form of that invariant, 32 x 71us = 2.3ms, is
   not the binding one - bursts are capped by the protocol, not by the byte
   time.)  It must also stay far below the 50ms that was measured to distort
   game timing when the sound CPU never answers.  4000us satisfies both with
   >= 2x headroom.

   The old value, 1000us, was empirical and sat inside the 0x00/0x01/0x11/
   0xB0 chord windows.  Tracing showed why it got away with it: the TRAP-burst
   commands are phase-locked away from the long passes (a timer tick pending
   during the burst's in-ISR chain is postponed past it, and the reload inside
   the ISR drags the tick phase with it every frame), and on the 0x196C path a
   prematurely released 8085 still HALTs for the RST5.5 reply before writing
   again.  Neither mechanism protects the main-loop bare STA sends, which were
   measured landing inside blocked passes, so the computed bound above is the one that holds */
#define RFRANCO_SOUND_GUARD_US 4000

/* Pseudo solenoids. The two bumpers and the two expulsores have no CPU
   connection at all: board 53/3311 ("CONTROL BUMPER Y EXPULSOR") fires each
   coil straight from its own playfield switch through an RC one-shot. They are
   synthesised here so the machine's most visible mechanics are observable.

   "Expulsor" is not a kickout hole - there is no hole on this playfield. The
   contact drawing (manual page 3) has contacts 24 and 25 inside the two
   triangular bodies at the bottom corners, i.e. the slingshots, and the parts
   list calls that mechanism the RECHAZADOR. 53/3311 drives exactly four coils:
   two bumpers and those two. See rfranco_pseudo_sol for the wiring */
#define RFRANCO_SOL_BUMPER_L 17
#define RFRANCO_SOL_BUMPER_R 18
#define RFRANCO_SOL_EJECT_L  19
#define RFRANCO_SOL_EJECT_R  20
#define RFRANCO_PSEUDO_FRAMES 6 /* vblanks a synthesised coil stays visible */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments;

  /* Serial switch chain: two 74165s on the driver board (IC5/IC6) are loaded
     with the playfield contacts and clocked out into SID one bit at a time.
     shiftIn holds the word still to be shifted; shiftPos counts bits */
  UINT16 swShift;
  int    swShiftPos;

  /* Serial display chain: SOD feeds a 74164 (IC1) on the display board which
     in turn drives the 8279. The game clocks 9 bits per frame */
  UINT16 dispShift;
  int    sodState;
  int    cbInstalled;

  /* 8212 command/ack latches between the two CPUs at 0x8000. */
  UINT8  soundCmd;     /* main -> sound, latched in IC6 */
  UINT8  soundReply;   /* sound -> main, latched in IC5 */
  int    soundTrigger; /* trigger the main CPU is currently stalled on */
  int    soundSeq;     /* rotating index into the trigger block */
  UINT8  scpuP2;       /* 8035 port 2 latch - selects latch / PSG1 / PSG2 */
  int    coinPulse;    /* TRAP ticks left on a coin one-shot */
  UINT8  coinBits;     /* which coin slot is pulsing */
  UINT8  coinDriven;   /* coin bits this driver last wrote into the matrix */
  UINT8  lastCoin;     /* previous key level, for edge detect */
  UINT8  lastStart;    /* previous start key level, for edge detect */
  UINT8  lastDoor;     /* previous door toggle positions, for edge detect */
  int    troughEdge;   /* a physical event wants the trough contact driven */
  int    lastTilt;     /* previous falta level, for edge detect */
  int    phaseT1;      /* mains half-cycle presented on the 8035's T1 pin */
  int    gatePhase;    /* phase the bytes arriving now were selected for */
  UINT8  lampAcc[CORE_STDLAMPCOLS]; /* lamps gated during the current half cycle */
  UINT8  lampPhase[2][CORE_STDLAMPCOLS]; /* last complete image of each phase */
  UINT32 solAcc;       /* coils gated during the current half cycle */
  UINT32 solPhase[2];
  UINT32 solSticky;    /* coils gated since the last vblank */
  int    ballInTrough; /* caida de bolas contact - see RFRANCO SWITCH_UPDATE */
  UINT8  lastJG;       /* previous JG contacts, for the pseudo coil one-shots */
  int    pseudoSol[4]; /* vblanks left on each synthesised coil */
  UINT8  i8279ram[16]; /* display RAM behind the 8279 */
  int    i8279addr;
  int    i8279autoinc;
  int    inhibitA, inhibitB;
} locals;

/*------------------------------------
/  Serial switch input on the SID pin
/-------------------------------------*/
/* The scan at 0x18A6 reads 16 bits, first bit first, inverting as it goes
   because the contacts are active low, and clocks the chain on with an OUT between each */
static int rfranco_sid_r(void) {
  /* Present the bit as-is: PinMAME's 1 = closed is what the hardware puts on
     SID, and the ROM's CMA at 0x18A9 turns it into its own 0 = closed. The
     register is filled by LOAD, not here - see rfranco_load_w */
  return locals.swShift & 1;
}

/* LOAD, the pulse the sound CPU puts on P1.5 when it is sent command 0xAA,
   does two jobs: it is the 8279's /WR on the display board (JA12) and it is
   the parallel load of the two 74165s on the driver board (JE-4). So the
   playfield contacts are captured at exactly this instant.

   That matters, because the clock the shift registers run on is the shared OUT
   strobe, which the display transfer pulses nine times per byte. Reloading
   when a bit count runs out instead - which is what this driver used to do -
   let those display clocks eat shift positions, and the switch scan then
   started somewhere in the middle of the word. The offset varied with how many
   display bytes a frame happened to send, so contacts read as their neighbours
   and the reading moved about. The ROM's own layout is what makes the hardware
   scheme work: 0x2437 ends by sending 0xAA, and the switch scan at 0x189C is
   the very next thing the TRAP handler does.

   Two 74165s cascaded IC6 -> IC5 -> SID. IC6 (connector JM) clocks out first
   and H leaves before A, so the firmware sees chain positions 2..17: IC6's H
   input (JM3) is gone before the first RIM and is invisible to the ROM. That is
   exactly why the manual's errata moves the picabolas contact from JM3 to JN2.
   Row 4 bit 7 is IC5's floating SER input and must stay clear, or the zone 9
   contact test reports a phantom closed switch */
static void rfranco_load_w(void) {
  /* Mask the SER bit rather than only asking callers to leave it alone: a front
     end doing vp_setSwitch(48,1), or the Tab menu's manual switch keys on
     column 4 row 8, would otherwise feed the ROM a sixteenth contact and its
     own zone 9 test would report one the machine does not have */
  locals.swShift = (UINT16)((coreGlobals.swMatrix[3] |
                            (coreGlobals.swMatrix[4] << 8)) & 0x7fff);
  locals.swShiftPos = 16;
}

/*-------------------------------------
/  Serial display output on the SOD pin
/--------------------------------------*/
static void rfranco_sod_w(int state) {
  locals.sodState = state ? 1 : 0;
}

/*-------------------------------------------------
/  Shared shift clock: any OUT instruction pulses it
/--------------------------------------------------*/
/* The 8085 has no dedicated clock output for the serial chains, so the game
   uses an OUT to any port as the strobe (0x00 inside the switch read loop at
   0x18B3, 0xFF inside the display write loop at 0x241C - the port number is
   irrelevant, the whole I/O space is one decode). Each pulse advances both the
   switch shift register and the display shift register */
static WRITE_HANDLER(rfranco_clk_w) {
  /* Belt and braces. This used to be the only install that stuck, because the
     reset handler runs before the CPU cores are reset (cpuexec.c:364 vs :372)
     and i8085_reset wiped the callbacks. Commit 8317f095 made the core preserve
     them, so MACHINE_RESET's install now survives and this one re-seats the
     same two pointers. Kept because it costs one branch and removing it would
     make the driver depend silently on that core fix */
  if (!locals.cbInstalled) {
    locals.cbInstalled = 1;
    i8085_set_SID_callback(rfranco_sid_r);
    i8085_set_SOD_callback(rfranco_sod_w);
  }
  /* Advance the switch chain. Every OUT is a clock edge for it, whichever
     serial chain the game thinks it is driving */
  if (locals.swShiftPos > 0) {
    locals.swShift >>= 1; /* LSB first - see rfranco_sid_r */
    locals.swShiftPos--;
  }
  /* The 74164 simply accumulates whatever SOD holds at each clock. Nine clocks
     per frame: the first shifts in the stale trailing level from the previous
     frame, which then falls off the end, leaving the register holding the
     payload byte exactly. Nothing needs to distinguish the two serial chains
     here - the switch scan's clocks disturb the register harmlessly, because it
     is reloaded from scratch every frame and only ever committed when LOAD pulses (see rfranco_sound_w) */
  locals.dispShift = (UINT16)((locals.dispShift << 1) | locals.sodState);
}

/*------------------------------
/  8212 latches at 0x8000
/-------------------------------*/
/* Writing sends a command to the sound CPU and raises its interrupt; reading
   takes the reply and clears RST5.5 on the main CPU. The game's handshake is
   at 0x196C: store the command, unmask RST5.5 with SIM, EI, then HALT until the latch answers */
/*------------------------------
/  8279 display controller
/-------------------------------*/
/* 16 x 8 display RAM. Each byte holds two digits: the high nibble goes to the
   7447 driving D15..D30 (OUT A) and the low nibble to the one driving D1..D14
   (OUT B), with the 74159 selecting one anode pair per RAM address. Digits are
   raw BCD; 0x0F blanks (the 7447 shows nothing for 15) */
static const INT8 rfranco_digit[16][2] = {  /* {OUT A, OUT B}, -1 = not wired */
  { 14,  6 }, { 30, 22 }, { 13,  5 }, { 29, 21 },
  { 12,  4 }, { 28, 20 }, { 11,  3 }, { 27, 19 },
  { 10,  2 }, { 26, 18 }, {  9,  1 }, { 25, 17 },
  {  8,  0 }, { 24, 16 }, { 33, -1 }, { 32, -1 },
};

static void rfranco_8279_refresh(int addr) {
  UINT8 d = locals.i8279ram[addr & 0x0f];
  INT8 a = rfranco_digit[addr & 0x0f][0];
  INT8 b = rfranco_digit[addr & 0x0f][1];
  if (a >= 0) locals.segments[a].w = core_bcd2seg7[d >> 4];
  if (b >= 0) locals.segments[b].w = core_bcd2seg7[d & 0x0f];
}

/* Called when LOAD pulses, which is the rising edge of the 8279's /WR. A0 is
   whatever level SOD was left at: 0x2417 finishes with D=0xC0 (high = command)
   and 0x2432 with D=0x40 (low = data) */
static void rfranco_8279_w(UINT8 data, int a0) {
  if (a0) {                                   /* command */
    switch (data & 0xe0) {
      case 0x80:                              /* write display RAM at addr */
        /* bit 4 is AI. The game uses both settings: 0x90 (address 0, AI set)
           ahead of the 16 byte fills at 0x2A46 and 0x24A8, and 0x8n with AI
           clear when it re-addresses before every single digit (0x25B9) */
        locals.i8279addr = data & 0x0f;
        locals.i8279autoinc = (data & 0x10) ? 1 : 0;
        break;
      case 0xa0:                              /* display write inhibit */
        /* 101x IWa IWb BLa BLb - bit 3 inhibits OUT A (the byte's high nibble)
           and bit 2 inhibits OUT B (the low nibble). The game sends 0xA8 for
           the odd players and 0xA4 for the even ones (0x2456), which is what
           puts players 1/3 on OUT B and 2/4 on OUT A */
        locals.inhibitA = (data & 0x08) ? 1 : 0;
        locals.inhibitB = (data & 0x04) ? 1 : 0;
        break;
      case 0xc0:                              /* clear */
      {
        int i;
        for (i = 0; i < 16; i++) {
          locals.i8279ram[i] = 0xff;          /* all ones = blank on a 7447 */
          rfranco_8279_refresh(i);
        }
        break;
      }
      default:                                /* mode set, clock, read FIFO */
        break;
    }
    return;
  }
  /* data write - the inhibit bits mask off one nibble so the two players
     sharing a RAM address can be written independently */
  {
    UINT8 val = locals.i8279ram[locals.i8279addr];
    if (!locals.inhibitA) val = (UINT8)((val & 0x0f) | (data & 0xf0));
    if (!locals.inhibitB) val = (UINT8)((val & 0xf0) | (data & 0x0f));
    locals.i8279ram[locals.i8279addr] = val;
    rfranco_8279_refresh(locals.i8279addr);
    if (locals.i8279autoinc)
      locals.i8279addr = (locals.i8279addr + 1) & 0x0f;
  }
}

static WRITE_HANDLER(rfranco_sound_w) {
  /* Command 0xAA is the LOAD strobe. The sound CPU's handler for it pulses
     P1.5, which reaches the display board as the 8279's /WR (JA12) and the
     driver board as the 74165 parallel load (JE-4), so it commits the display
     byte and captures the playfield contacts at the same instant. 0x2417 sends
     it at the end of every transfer and it is issued from nowhere else */
  if (data == 0xaa) {
    rfranco_8279_w((UINT8)(locals.dispShift & 0xff), locals.sodState);
    rfranco_load_w();
  }
  locals.soundCmd = data;
  cpu_set_irq_line(RFRANCO_SCPU, 0, ASSERT_LINE);

  /* The 8212 holds the 8085 in wait states through its READY input until the
     sound CPU has taken the byte. That flow control is not optional: the bulk
     transfer at 0x19F3 pushes 19 bytes back to back with no handshake of its
     own, and the 8035 - which polls the INT pin with JNI at 0x00FB rather than
     taking an interrupt - is far too slow to keep up. Without READY the 8085
     simply overwrites the latch and the transfer is lost.

     MAME's skeleton has the same wiring noted but commented out:
        //m_soundlatch[1]->int_wr_callback().append_inputline(maincpu, READY)

     PinMAME's 8085 core has no READY line, so stall the main CPU on a trigger
     instead and let the sound CPU release it when it reads the latch. The
     timed trigger is a safety net: if the sound CPU has masked its interrupt
     and will never read, we must not deadlock.

     Each byte takes its own trigger number, because cpu_triggertime cannot be
     cancelled. With one shared number the guards armed by earlier bytes fire
     during later stalls and release them before the sound CPU has read - the
     8085 then runs ahead and overwrites the latch. Measured with the transfer
     traced on both sides, that lost the whole second half of every frame:
     the 8085 sent ... F5 ... 2F ... 2F but the sound CPU forwarded the coil
     half as ten copies of the trailing 0xFF, so no coil and no IC3 lamp ever
     reached the driver board */
  locals.soundSeq = (locals.soundSeq + 1) % RFRANCO_SOUND_TRIGGERS;
  locals.soundTrigger = RFRANCO_SOUND_TRIGGER + locals.soundSeq;
  cpu_spinuntil_trigger(locals.soundTrigger);
  cpu_triggertime(TIME_IN_USEC(RFRANCO_SOUND_GUARD_US), locals.soundTrigger);
}

static READ_HANDLER(rfranco_sound_r) {
  cpu_set_irq_line(RFRANCO_CPU, I8085_RST55_LINE, CLEAR_LINE);
  return locals.soundReply;
}

/* 0x4000 is chip select CS1 from the 74S138 and carries eight playfield
   contacts straight off connector JG - MAME's skeleton omits it entirely. The
   ROM reads it once per pass at 0x18BD and, unlike the serial chain, applies no
   CMA, so the bus itself is active low */
static READ_HANDLER(rfranco_4000_r) {
  return ~coreGlobals.swMatrix[1];
}

/*-------------------
/  Sound CPU (8035)
/--------------------*/
/* The 8035 reaches everything through MOVX, which PinMAME's MCS-48 core routes
   into the port space with the 8 bit address taken from R0/R1. P2.7 picks the
   target: low selects the 8212 latch pair, high leaves the PSGs selected.

   From the sound ROM's external interrupt handler:
       0028: MOV A,#$7F / OUTL P2,A    ; P2.7 low - select the latches
       002B: MOVX A,@R1                ; read the command, clears INT35
       ...
       007E: ORL P2,#$FF / ANL P2,#$7F ; P2.7 low again
       0082: MOVX @R1,A                ; write the reply, raises INT5.5

   The 8212s are edge devices: strobing one asserts its INT, reading it clears
   it. IC6 carries main->sound (INT35), IC5 carries sound->main (RST5.5) */
#define RFRANCO_LATCH_SELECTED(p2) (((p2) & 0x80) == 0)

static READ_HANDLER(rfranco_scpu_movx_r) {
  if (RFRANCO_LATCH_SELECTED(locals.scpuP2)) {
    /* reading IC6 takes the command and drops the sound CPU's interrupt */
    cpu_set_irq_line(RFRANCO_SCPU, 0, CLEAR_LINE);
    /* releases the main CPU from its READY stall */
    cpu_trigger(locals.soundTrigger);
    return locals.soundCmd;
  }
  /* PSG read path. P2.5 low selects PCS2 = IC2 (PSG2), which is the input
     device: its register 7 is programmed 0x38 at sound ROM 0x00B1, making both
     ports inputs. The MOVX address is the AY register number, taken from R1.

     This is the only way the machine can see a coin. The 8085 asks for it with
     sound command 0x99 (0x18C3: MVI A,99 / CALL 196C), the 8035 answers from
     0x060F by selecting IC2 and reading register 0x0E, and the reply lands in
     C027. Port A carries the two coin slots, the ball drain and the start
     button; port B carries the two operator switches on the door */
  if (!(locals.scpuP2 & 0x20)) {
    switch (offset & 0x0f) {
      case 0x0e: /* port A - cabinet inputs, active low */
        return ~coreGlobals.swMatrix[2];
      case 0x0f: {
        /* Port B bits 7/6 are the ajuste and test switches on the door; 1 is
           switch down. The boot dispatch at 0x00BB masks them to 0xC0 and
           branches on the result, giving the manual's four positions - all
           four verified against what the machine actually does:
             0xC0 both down    juego, the normal game
             0x80              test de luces, which is also the way in to the
                               RAM audit zones (0x312C)
             0x40              borrado: zeroes the credits once and waits
                               (0x00C7)
             0x00 both up      ajustes, the adjustment zones (0x3255 in
                               both sets).
           The position is not only read at boot - the ajustes menu re-reads it
           on every pass; see RFRANCO_SWAJUSTE in rfranco.h for what the ROM
           does with it and why these are switches and not a setting.
           Resting position is down, so an untouched machine reads 0xC0 and boots into juego */
        UINT8 v = 0xc0;
        if (core_getSw(RFRANCO_SWAJUSTE)) v &= (UINT8)~0x80;
        if (core_getSw(RFRANCO_SWTEST))   v &= (UINT8)~0x40;
        return v;
      }
      default:
        return 0xff;
    }
  }
  if (!(locals.scpuP2 & 0x40)) {
    /* PSG1 is read back too: the REST opcode saves a voice's volume register before muting it (sound ROM 0x2A5) */
    AY8910Write(0, 0, offset & 0x0f);
    return AY8910Read(0);
  }
  return 0xff;
}

/*----------------------------------------------
/  Lamp matrix: three 4028s times two mains phases
/-----------------------------------------------*/
/* The game keeps its lamp image in NVRAM as eight pairs of bytes at C219,
   one pair per (decoder, code range), FASE A first and FASE B second:

       C219/C21C  IC1 codes 0-7     C21F/C222  IC1 codes 8-9 (bits 7,6)
       C225/C228  IC2 codes 0-7     C22B/C22E  IC2 codes 8-9
       C231/C234  IC7 codes 0-7     C237/C23A  IC7 codes 8-9   (coils)
       C23D/C240  IC3 codes 0-7     C243/C246  IC3 codes 8-9

   Within a byte bit 7 is code 0 and bit 0 is code 7, because the serialiser at
   0x1A02 rotates left and emits the carry with an index that counts up from 0.

   Each (decoder, phase, code) is one physical lamp, so keep the layout: codes
   0-7 of each decoder/phase pair get a column of their own, and the code 8/9
   bits - only IC2 has anything wired there - share the last two columns.

       col 0  IC1 FASE A   1 luz falta   2 jugador 3   3 jugador 4
                           4-8 loteria 90/80/70/60/50
       col 1  IC1 FASE B  11 luz falta  12 jugador 1  13 jugador 2
                          14-18 loteria 00/10/20/30/40
       col 2  IC2 FASE A  21-28 avance 10000..80000
       col 3  IC2 FASE B  31-35 bola 1..5  36 fin de juego
                          37 bola extra   38 especial picabolas
       col 4  IC3 FASE A  41 bumper dcho  42 especial dcho
                          43 bola extra diana dcha  44 pasillo dcho
                          45 pulsador partidas
       col 5  IC3 FASE B  51 bumper izq   52 especial izq
                          53 bola extra diana izq   54 pasillo izq
       col 6  codes 8/9   61-64 IC1 (unused)  65 avance 90000  66 avance 100000
                          67 avance doble     68 avance triple
       col 7  codes 8/9   71-74 IC3 (unused)

   The map is the manual's IC1/IC2/IC3 tables (page 17) read from the bottom up,
   which the ROM confirms at every point that can be checked: 0x16C4 lights IC2
   FASE B code (ball-1), 0x16A1 lights IC1 code 1/2 for the player number,
   0x042B sets IC3 FASE A code 4 (the start button lamp) when a credit is
   available, 0x0174 sets IC2 FASE B code 5 (fin de juego) on game over, and
   0x151B alternates IC3 code 3 between the two phases (pasillo dcho/izq) */
#define RFRANCO_IC1 0
#define RFRANCO_IC2 1
#define RFRANCO_IC3 2

static void rfranco_gate(int dec, int phase, int code) {
  if (code > 9) return; /* 10-15 select no output */
  if (code < 8)
    locals.lampAcc[dec * 2 + phase] |= (UINT8)(1 << code);
  else if (dec != RFRANCO_IC3)
    locals.lampAcc[6] |= (UINT8)(1 << (dec * 4 + phase * 2 + (code - 8)));
  else
    locals.lampAcc[7] |= (UINT8)(1 << (phase * 2 + (code - 8)));
}

static WRITE_HANDLER(rfranco_scpu_movx_w) {
  if (RFRANCO_LATCH_SELECTED(locals.scpuP2)) {
    /* strobing IC5 is the ack the main CPU is halted waiting for */
    locals.soundReply = data;
    cpu_set_irq_line(RFRANCO_CPU, I8085_RST55_LINE, ASSERT_LINE);
    return;
  }
  /* P2.6 low selects PCS1 = IC3 (PSG1), the output device: its register 7 is
     programmed 0xF8 by the sound ROM at 0x00DB, making both ports outputs.
     Registers 0x0E/0x0F are the two I/O ports, and each byte written there
     carries TWO 4028 select codes - high nibble and low nibble. Codes 0-9 pick
     an output, 10-15 select none.

         port A  high nibble -> driver IC1 : loteria / jugador / falta (JA)
                 low  nibble -> driver IC2 : 20 playfield lamps       (JQ)
         port B  high nibble -> driver IC7 : the coils                (JL)
                 low  nibble -> driver IC3 : 9 playfield lamps        (JP)

     Each code gates a BT106 thyristor which then conducts to the end of the
     mains half cycle, so a lamp selected for one ~85us slot stays lit for the
     rest of the frame. Accumulate here and commit at the frame boundary rather
     than sampling instantaneously */
  if (!(locals.scpuP2 & 0x40)) {
    int hi = data >> 4, lo = data & 0x0f;
    int ph = locals.gatePhase ? 0 : 1;    /* 0 = FASE A, 1 = FASE B */
    switch (offset & 0x0f) {
      case 0x0e:
        rfranco_gate(RFRANCO_IC1, ph, hi);
        rfranco_gate(RFRANCO_IC2, ph, lo);
        return;
      case 0x0f:
        /* IC7, connector JL. Solenoid number is the 4028 output + 1:
             1 (n.c.)   2 taca     3 bobina monedero   4 contador 25
             5 contador 100        6 relay flippers    7 bancada izquierda
             8 picabolas           9 bancada derecha  10 salida bolas
           Confirmed against the ROM: 0x055F fires 4 on a 25 pta coin, 0x15A6
           holds 3 through attract to enable the coin slot, 0x1639/0x1656 fire
           9 and 7 to reset whichever target bank is down, 0x1682 fires 10 to
           serve the ball and 0x1754 fires 2 when a replay is awarded */
        if (hi < 10) locals.solAcc |= 1u << hi; /* IC7 coils */
        rfranco_gate(RFRANCO_IC3, ph, lo);
        return;
      default:
        AY8910Write(0, 0, offset & 0x0f);
        AY8910Write(0, 1, data);
        break;
    }
  }
  /* Not "else": P2 = 0x9F pulls both selects low at once, which the sound ROM
     uses at 0x39B to zero registers 8/9/10 on both chips */
  if (!(locals.scpuP2 & 0x20)) { /* PCS2 = IC2, the input PSG */
    AY8910Write(1, 0, offset & 0x0f);
    AY8910Write(1, 1, data);
  }
}

/* The sound CPU samples the mains half cycle on T1 (JD-8, DETECCION FASE) and
   reports it to the 8085, which uses it to pick between the FASE A and FASE B
   copies of the lamp data. Exactly one JT1 exists in the whole sound ROM, at 0x00F8 */
static READ_HANDLER(rfranco_scpu_t1_r) {
  return locals.phaseT1;
}

static WRITE_HANDLER(rfranco_scpu_p1_w) {
  /* Nothing to latch: no output of P1 reaches anything this driver models, so
     the value was write-only state and is not kept.
     P1 does NOT carry BDIR/BC1 - those come from IC17 (7400) fed by /WR35,
     /RD35 and ALE, which is why a single MOVX both latches the AY register
     number and writes it. P1 drives the 74S138 (IC8) and the 7438 (IC15)
     display strobe gates; P1.6 is an independent latched output toggled only
     by sound commands 0x96 and 0x69 */
}

static WRITE_HANDLER(rfranco_scpu_p2_w) {
  /* P2.4 is the system /RESET net: it reaches both AY-3-8910s (pin 23), the
     8212 latches and the main board. The sound CPU asserts it at 0x0BA, holds
     it across its ~1.94s power-up timer delay, and releases it at 0x0C7 - so
     the 8035 holds the rest of the machine in reset while it starts up */
  if ((locals.scpuP2 ^ data) & 0x10)
    cpu_set_reset_line(RFRANCO_CPU, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
  locals.scpuP2 = data;
}

/*-- AY-3-8910 --*/
/* PSG1 (chip 0, CPU board IC3) is the output device and PSG2 (chip 1, IC2) the
   input device - see rfranco_scpu_movx_r/w, which intercept registers 0x0E/0x0F
   for both. Nothing is wired here: the I/O ports never reach AY8910Write */
static struct AY8910interface RFRANCO_ay8910Int = {
  2,                   /* 2 chips: 0 = PSG1/IC3 (outputs), 1 = PSG2/IC2 */
  RFRANCO_CPUFREQ / 6, /* clocked from 8035 T0 = XTAL/6 */
  { 30, 30 },          /* volume */
  { 0, 0 }, { 0, 0 },  /* I/O ports are handled in rfranco_scpu_movx_r/w */
  { 0, 0 }, { 0, 0 },
};

/*-------------------------------------------------------------
/  Who owns the ball: the driver, or the front end's own physics
/--------------------------------------------------------------*/
/* CAIDA DE BOLAS is an ordinary trough contact - manual contact 28, connector
   JO3, debounced on the driver board like any other input. The machine senses a
   ball in the outhole exactly the way every other pinball does, and the driver
   does not model the *sensing*. What it models is the ball MOVEMENT that opens
   and closes the contact, because standalone PinMAME has no ball: the kicker
   firing stands in for "the ball left" and the DRAIN key for "the ball came
   back".

   That is mechanical simulation, and PinMAME's convention is that a front end
   which owns the mechanics owns it. VPinMAME exposes the choice to the table as
   Controller.HandleMechanics, libpinmame defaults it to 0, and core.c gates the
   generic mech handler on the same flag (core.c:1783). With it off, a Visual
   Pinball table drives switch 27 from its own ball physics - ball rolls into
   the trough, table closes the contact; kicker fires, table opens it - and the
   driver must keep its hands off, or the two fight over the same bit.

   The standalone build defaults the flag to 0xff, so keyboard play is unaffected */
static int rfranco_ownsBall(void) {
  extern int g_fHandleMechanics;
  return g_fHandleMechanics != 0;
}

/*-----------
/  Interrupts
/------------*/
/* TRAP carries the mains zero cross. It is not optional: on a machine with
   invalid NVRAM the reset path at 0x0000 tests C000 for the magic byte 0x55
   and, failing it, executes RST 0 - which lands back on 0x0000. Nothing in
   that loop ever writes the magic. It is the TRAP handler at 0x1800 that
   detects the bad magic, sends sound command 0xBB, seeds C000 with 0x55 and
   resets. So without TRAP running the machine simply never comes up. */
static INTERRUPT_GEN(rfranco_trap) {
  /* One half cycle's worth of lamp/coil selects has been gated by now. Only the
     phase that was live carries any selects: the other phase's thyristors are
     conducting on their own supply and its lamps stay lit right through. So
     keep a snapshot per phase and publish the union, rather than replacing the
     whole matrix every pass - that made every lamp flicker on and off at 50 Hz
     and left whichever phase vblank happened to miss looking dark */
  int i, p = locals.gatePhase ? 0 : 1;
  memcpy(locals.lampPhase[p], locals.lampAcc, sizeof(locals.lampAcc));
  locals.solPhase[p] = locals.solAcc;
  for (i = 0; i < CORE_STDLAMPCOLS; i++)
    coreGlobals.tmpLampMatrix[i] = (UINT8)(locals.lampPhase[0][i] | locals.lampPhase[1][i]);
  locals.solSticky |= locals.solPhase[0] | locals.solPhase[1];

  /* SALIDA BOLAS (IC7 code 9) is the outhole kicker. Once it has been gated the
     ball is out of the trough, so the contact opens - and it stays open until
     the player drains, which is the DRAIN key since there is no ball model.
     Without this the trough reads "ball present" for ever and the game ends
     every ball the instant it starts one (0x0A2C -> 0x1121).

     Only when the mechanics are ours - see rfranco_ownsBall */
  if (rfranco_ownsBall() && (locals.solAcc & (1u << 9))) {
    locals.ballInTrough = 0;
    locals.troughEdge = 1;
  }

  memset(locals.lampAcc, 0, sizeof(locals.lampAcc));
  locals.solAcc = 0;
  if (locals.coinPulse) locals.coinPulse--;

  /* The bytes that will be gated during the half cycle starting now were built
     by 0x1996 one TRAP ago, from the phase the sound CPU reported to the 0xDD
     issued then - 0x19E6 sends 0xDD, stores the reply in C04F and immediately
     blasts the buffer prepared on the previous pass. So the phase that selected
     the data now arriving is the one before the current one */
  locals.gatePhase = locals.phaseT1;
  locals.phaseT1 ^= 1;
  cpu_set_irq_line(RFRANCO_CPU, IRQ_LINE_NMI, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
/* The bumpers and the two ball ejectors are fired by board 53/3311 straight
   from their own playfield contacts; the CPU never sees them as outputs and
   only reads the contacts for scoring. Synthesise a one-shot on each so they
   are visible - and so a front end has something to drive */
static void rfranco_pseudo_sol(void) {
  static const struct { UINT8 mask; int sol; } wired[4] = {
    { 0x80, RFRANCO_SOL_BUMPER_L }, /* JG7  AD7 contacto bumper izq   (sw 18) */
    { 0x02, RFRANCO_SOL_BUMPER_R }, /* JG6  AD1 contacto bumper dcho  (sw 12) */
    /* Both expulsores hang off the same bit. Their contacts are the manual's
       24 and 25, the two "10 PUNTOS" inside the slingshot bodies, and they are
       wired in parallel onto AD0 - which the ROM's own contact-test table
       states, by flagging AD0 as a paralleled pair and reporting it as the
       higher of the two. The CPU therefore cannot tell left from right and
       neither can this driver: both fire together. A front end that knows
       where the ball was should use that instead. */
    { 0x01, RFRANCO_SOL_EJECT_L },  /* JG8  AD0 contacto 10 puntos    (sw 11) */
    { 0x01, RFRANCO_SOL_EJECT_R },  /* JG8  AD0 - the same contact            */
  };
  UINT8 jg = coreGlobals.swMatrix[1], closed = (UINT8)(jg & ~locals.lastJG);
  int i;
  locals.lastJG = jg;
  for (i = 0; i < 4; i++) {
    if (closed & wired[i].mask) locals.pseudoSol[i] = RFRANCO_PSEUDO_FRAMES;
    if (locals.pseudoSol[i]) {
      locals.pseudoSol[i]--;
      locals.solenoids |= 1u << (wired[i].sol - 1);
    }
  }
}

static INTERRUPT_GEN(rfranco_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  /* TRAP runs at 100 Hz against a 60 Hz vblank, so take everything gated since
     the last frame rather than only the newest half cycle */
  locals.solenoids = locals.solSticky;
  locals.solSticky = 0;
  rfranco_pseudo_sol();
  coreGlobals.solenoids = locals.solenoids;
  /*-- display --*/
  if ((locals.vblankCount % RFRANCO_DISPLAYSMOOTH) == 0)
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));

  /* core_updateSw is the only caller of the SWITCH_UPDATE handler, so without
     this the cabinet inputs never reach swMatrix at all. Pass TRUE because the
     flippers are not CPU driven here - the ROM never energises the flipper
     supply relay on JL3, the buttons feed the coils directly through the
     interconnect board - so the flipper solenoids have to be synthesised */
  core_updateSw(TRUE);
}

/* Switch numbering. The driver keeps its four hardware bytes in swMatrix rows
   1-4, so expose them as the usual col*10 + row + 1 (see the table at the top
   of this file). Declaring this explicitly rather than relying on core.c's
   default keeps the numbering the driver's own */
static int rfranco_sw2m(int no) { return (no / 10) * 8 + (no % 10) - 1; }
static int rfranco_m2sw(int col, int row) { return col * 10 + row + 1; }

/* Lamps use the same col*10 + row + 1 scheme, so a lamp number and a switch
   number are read the same way and both agree with what the debug interface
   reports. Without this the base machine driver's sequential numbering would
   apply and column 0 - the whole IC1 group - would land on lamp numbers 0 and
   below, unreachable through vp_getLamp */
/* The two directions do NOT take the column the same way, which is core.c's
   convention rather than this driver's choice. lamp2m carries a +8 that
   vp_getLamp cancels with its own -8 (vpintf.c:34), and m2lamp is called with a
   ONE-BASED column (vpintf.c:74 and :96 pass ii+1), so it has to subtract that
   1 back off - exactly as gts80.c:127 does. Without the -1 the change list a
   VPX table reads reports every lamp a column high: matrix column 0, the IC1
   FASE A group documented as lamps 1-8, came back as 11-18, and column 7 came
   back as 81-88, numbers this machine does not have.
   Note core.c's own round-trip assert (core.c:2469) passes a ZERO-based column,
   so it disagrees with vpintf about the convention. It never fires here because
   hw.lampCol is 0, and it is not this driver's to fix */
static int rfranco_lamp2m(int no) { return (no / 10) * 8 + (no % 10) + 7; }
static int rfranco_m2lamp(int col, int row) { return (col - 1) * 10 + row + 1; }

static SWITCH_UPDATE(RFRANCO) {
  UINT8 v = 0, mask = 0;
  int tilt = 0;

  /* Everything here has to work with inports == NULL. VPinMAME clears
     m_fHandleKeyboard and libpinmame clears g_fHandleKeyboard, so under either
     of them core_updateSw passes no input ports at all - and the two things
     that keep this ROM out of its fault loops, the ball trough and the coin
     one-shot, both live here.

     Nothing below writes a bit unless that bit has just changed. Rewriting the
     whole cabinet row every frame - which this used to do - means anything
     else that sets one of those switches is stamped back out at the next
     vblank, before the ROM has had a chance to look: it polls the row through
     the sound CPU, so a switch that survives less than one frame is a coin
     toss. That made the start button unusable from outside the keyboard, and
     it is what made "insert coin, press start" work about half the time */
  if (inports) {
    UINT16 inp = inports[RFRANCO_COMINPORT];
    UINT8  coins = (UINT8)(inp & 0x30);
    UINT8  start = (UINT8)((inp & 0x0080) ? 0x80 : 0);
    UINT8  door  = (UINT8)(((inp & 0x0200) ? 0x01 : 0) | ((inp & 0x0400) ? 0x02 : 0));

    /* The coin contacts have to be a pulse, not a level. 0x0545 latches the
       coin, then waits for the contact to OPEN within 20 TRAP ticks; if it is
       still closed it falls through to 0x055C and jumps to the fault handler,
       which wedges the machine for good. Holding a coin key down is the
       obvious thing for a user to do, so turn the key press into a one-shot */
    if (coins & ~locals.lastCoin) {
      locals.coinBits = (UINT8)(coins & ~locals.lastCoin);
      locals.coinPulse = 10;
    }
    locals.lastCoin = coins;

    if (inp & 0x0040) { /* DRAIN key: ball returns */
      if (!locals.ballInTrough) locals.troughEdge = 1;
      locals.ballInTrough = 1;
    }
    if (start != locals.lastStart) { /* pulsador partidas */
      v |= start; mask |= 0x80;
      locals.lastStart = start;
    }
    if (inp & 0x0100) tilt = 1;

    /* The two door switches, into the pseudo column 0 (see RFRANCO_SWAJUSTE).
       Williams copies its whole door column out of the inport every frame
       (s11.c:784); this driver cannot, for the reason given above - it would
       stamp back over anything a front end or a test harness had set between
       vblanks, and these two are exactly the switches an operator menu walker
       drives from outside. So they get the same change-detection treatment as
       everything else here: the keyboard owns a bit only at the moment it moves it */
    if (door != locals.lastDoor) {
      CORE_SETKEYSW(door, 0x03, 0);
      locals.lastDoor = door;
    }
  }

  /* The coin one-shot, from either path: the keyboard arms it above, and a
     front end that drives the contact itself never does - but if one is armed
     it has to be run down here, or the two paths could leave a coin latched */
  {
    UINT8 coinNow = (UINT8)(locals.coinPulse ? locals.coinBits : 0);
    if (coinNow != locals.coinDriven) {
      v = (UINT8)((v & ~0x30) | coinNow); mask |= 0x30;
      locals.coinDriven = coinNow;
    }
  }

  /* Caida de bolas is closed whenever a ball is sitting in the outhole. That is
     the rest state, and both the start path at 0x0508 and the fault recovery at
     0x030F require it - left open, 0x030F/0x0331 ping-pong for ever and the
     foreground program is dead while the TRAP handler keeps ticking, so the
     machine looks alive.

     Two state model: the game empties the trough by firing SALIDA BOLAS (see
     rfranco_trap) and the DRAIN key refills it, which is what ends the ball.
     Only those two physical events drive the contact - the state is not
     reasserted every frame - so a front end that owns the contact itself keeps
     control of it in between.

     And when the front end owns the mechanics outright (rfranco_ownsBall), the
     driver does not drive the contact at all: the table's ball physics do, and
     an empty trough at game start is then a real "no ball" condition that the
     ROM handles on its own by waiting at 0x030F/0x0331 */
  if (locals.troughEdge) {
    locals.troughEdge = 0; /* consumed either way, so that a table which
                              turns HandleMechanics back on mid-run does not
                              inherit a stale edge from before */
    if (rfranco_ownsBall()) {
      if (locals.ballInTrough) v |= 0x40;
      mask |= 0x40;
    }
  }

  if (mask) CORE_SETKEYSW(v, mask, 2);

  /* Falta (tilt) is not a matrix switch on the real board - it reaches the CPU
     on JD1 and fires RST 6.5 (vector 0x0034 -> 0x0286). The ROM never reads
     bits 0-3 of the cabinet row, so switch 21 is borrowed as the front end's
     way in; it is edge triggered, like the contact */
  if (coreGlobals.swMatrix[2] & 0x01) tilt = 1;
  /* Level, not a pulse. RST 6.5 is a level sensitive input, and the ROM leaves
     it masked except for the single NOP at 0x194D, so a pulse is armed and
     disarmed again long before the CPU can look: the core's own request flag is
     cleared by the falling edge (i8085_set_RST65). Hold the line for as long as
     the contact is closed and the request survives to the next unmask window */
  if (tilt != locals.lastTilt)
    cpu_set_irq_line(RFRANCO_CPU, I8085_RST65_LINE, tilt ? ASSERT_LINE : CLEAR_LINE);
  locals.lastTilt = tilt;

  /* Where the two door switches are sitting, in the same place and the same
     way s11.c:800 shows Advance and Up/Down. A toggle has no on-screen
     position of its own, and on this machine the position decides which of the
     four modes the ROM took at boot, so it is worth being able to see it */
  core_textOutf(40, 20, BLACK, core_getSw(RFRANCO_SWAJUSTE) ? "Ajuste arriba" : "Ajuste abajo ");
  core_textOutf(40, 30, BLACK, core_getSw(RFRANCO_SWTEST)   ? "Test   arriba" : "Test   abajo ");
}

/*----------------
/  Memory handlers
/-----------------*/
static MEMORY_READ_START(rfranco_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x4000, 0x4000, rfranco_4000_r},
  {0x8000, 0x8000, rfranco_sound_r},
  {0xc000, 0xc7ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(rfranco_writemem)
  {0x0000, 0x3fff, MWA_ROM},
  {0x8000, 0x8000, rfranco_sound_w},
  {0xc000, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_WRITE_START(rfranco_writeport)
  {0x00, 0xff, rfranco_clk_w},
PORT_END

/*-- sound CPU --*/
static MEMORY_READ_START(rfranco_scpu_readmem)
  {0x0000, 0x0fff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(rfranco_scpu_writemem)
  {0x0000, 0x0fff, MWA_ROM},
MEMORY_END

static PORT_READ_START(rfranco_scpu_readport)
  {0x00, 0xff, rfranco_scpu_movx_r},
  {I8039_t1, I8039_t1, rfranco_scpu_t1_r},
PORT_END

static PORT_WRITE_START(rfranco_scpu_writeport)
  {0x00, 0xff, rfranco_scpu_movx_w},
  {I8039_p1, I8039_p1, rfranco_scpu_p1_w},
  {I8039_p2, I8039_p2, rfranco_scpu_p2_w},
MEMORY_END

/* The 2532 at IC4 has its data pins wired to the 8035's AD0-AD7 in reverse
   order: EPROM D7 lands on AD0 and D0 on AD7 (see the CPU board schematic,
   manual sheet 1 of ref. 53/3291). Read straight off the chip the image is
   meaningless as MCS-48 code - 5 RET opcodes in 2K, and no jump at either the
   reset or the external interrupt vector. Reversing each byte turns it into an
   ordinary program: 172 JMP, 88 CALL, 28 RET, and the expected vector layout
       000: DIS I / JMP $09F
       003: SEL RB0 / MOV R7,A / JMP $028   (external interrupt)
       007: STOP TCNT                       (timer)
   Do it once, after the ROMs are loaded. */
/* Call exactly once per game start, from DRIVER_INIT: the ROM regions are
   reloaded on every start, so a process-lifetime guard would leave the second
   game started inside one VPinMAME/libpinmame process running a still
   scrambled sound ROM - the 8035 would execute noise, never release P2.4 and
   hold the main CPU in reset for ever. MACHINE_INIT is not the place either,
   because it also runs on a soft reset and would reverse the image back */
void rfranco_unscramble_sound_rom(void) {
  UINT8 * const rom = memory_region(RFRANCO_MEMREG_SCPU);
  int i;
  if (!rom) return;
  for (i = 0; i < 0x1000; i++) {
    UINT8 b = rom[i];
    b = (UINT8)(((b & 0x01) << 7) | ((b & 0x02) << 5) | ((b & 0x04) << 3) | ((b & 0x08) << 1) |
                ((b & 0x10) >> 1) | ((b & 0x20) >> 3) | ((b & 0x40) >> 5) | ((b & 0x80) >> 7));
    rom[i] = b;
  }
}

/* This has to be the RESET handler, not the INIT one. core.c calls the driver's
   init from inside its own `if (!coreData)` first-time block and its reset on
   every pass, so an init-only driver keeps every byte of `locals` across a soft
   reset (F3, or "Reset Game" in the Tab menu). That was visible: the machine
   came back with the trough left wherever the last game had put it rather than
   with a ball in it, so the ROM's boot path could not complete, and the boot
   dispatch at 0x00BB - the only place the operator door switches are ever read -
   was never reached. Resetting into an operator menu did nothing at all */
static MACHINE_RESET(RFRANCO) {
  memset(&locals, 0, sizeof locals);
  /* The 8035 resets its ports high (i8039_reset sets P2 = 0xff), so start there
     rather than at 0: a zeroed P2 reads as "latch selected" and gives the P2.4
     edge detector a state the chip was never in. Harmless with this ROM - 0x0A5
     writes P2 before the first MOVX - but only by luck */
  locals.scpuP2 = 0xff;
  /* Blank, not zero. The 8279's own clear command fills its display RAM with
     0xff because that is what blanks a 7447 (see the 0xc0 case in rfranco_8279_w),
     so a memset to 0x00 would leave the RAM claiming "digit 0" while locals.segments
     is blank. Anything the ROM then wrote with one inhibit bit set would take the
     untouched nibble from that stale 0 and paint a digit the machine is not showing.
     The ROM clears before it draws either way, but it keeps the two agreeing */
  memset(locals.i8279ram, 0xff, sizeof locals.i8279ram);
  locals.ballInTrough = 1; /* a ball rests in the outhole at power up */
  locals.troughEdge = 1;   /* ... and the ROM has to be told so, unless the
                              front end owns the trough - see
                              rfranco_ownsBall, which gates the commit */
  /* RIM must sample SID at the instant it executes, because the switch data is
     being clocked in a bit at a time - a value pushed in ahead of time would be stale */
  i8085_set_SID_callback(rfranco_sid_r);
  i8085_set_SOD_callback(rfranco_sod_w);
}

MACHINE_DRIVER_START(RFRANCO)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", 8085A, RFRANCO_CPUFREQ / 2) /* core wants the internal clock */
  MDRV_CPU_MEMORY(rfranco_readmem, rfranco_writemem)
  MDRV_CPU_PORTS(NULL, rfranco_writeport)
  MDRV_CPU_VBLANK_INT(rfranco_vblank, 1)
  MDRV_CPU_PERIODIC_INT(rfranco_trap, RFRANCO_TRAPFREQ)

  /* PinMAME's MCS-48 core wants the machine cycle rate, not the pin frequency:
     the 8035 divides its clock by 15 internally. The sound CPU is fed from the
     8085's CLK OUT, i.e. XTAL/2 */
  MDRV_CPU_ADD_TAG("scpu", I8035, RFRANCO_CPUFREQ / 2 / 15)
  MDRV_CPU_MEMORY(rfranco_scpu_readmem, rfranco_scpu_writemem)
  MDRV_CPU_PORTS(rfranco_scpu_readport, rfranco_scpu_writeport)

  /* Retested empirically after the 8212 READY handshake was modelled (the old
     500 predates it, from when raw interleave was all that kept the two CPUs
     in step for the latch protocol).  Ladder tried, one rebuild per rung,
     rfranco_check/rfranco_game/rfranco_sound on both sets from cold NVRAM:
     500, 250, 150, 100, 50, 20, 10, 5, 1.  1-20 wedge both sets at the
     power-on handshake (the READY trigger only covers the per-byte stall in
     rfranco_sound_w, boot is still interleave-carried); at 50 supstarfa's
     TRAP handler enters ~4x more often than it completes and never settles;
     at 100 rfranco_game's end-of-ball bonus attribution fails on supstarf
     (same total score, a stimulus-phase effect of the machine speed, but a
     harness failure all the same).  150 and 250 pass every harness
     repeatedly, sound byte protocol intact.  250 = lowest fully passing rung
     x 1.7 and 5x the highest rfranco_check failure; headless it buys ~1.6x
     emulated throughput and ~35% less host CPU per emulated second than 500.
     The slice quantum at 250 is ~67us, far inside the sound guard's 1230us
     margin - see RFRANCO_SOUND_GUARD_US.  Details: docs/driver-notes.md
     (superstar repo) 7.3 */
  MDRV_INTERLEAVE(250)
  MDRV_CORE_INIT_RESET_STOP(NULL, RFRANCO, NULL)
  MDRV_DIPS(0) /* none. The machine has no DIP switches: the operator's settings are NVRAM, reached from
                  the ajustes menu, and the two door switches are switches (RFRANCO_SWAJUSTE). The empty
                  inport they used to occupy stays declared - see the comment in rfranco.h */
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(RFRANCO)
  MDRV_SWITCH_CONV(rfranco_sw2m, rfranco_m2sw)
  MDRV_LAMP_CONV(rfranco_lamp2m, rfranco_m2lamp)
  MDRV_SOUND_ADD(AY8910, RFRANCO_ay8910Int)
MACHINE_DRIVER_END
