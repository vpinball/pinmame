// license:BSD-3-Clause

/************************************************************************************************
  Stern SPIKE 1 (2012-2020: Game of Thrones, KISS, Wrestlemania, Ghostbusters, ...)

  The game is a statically linked ARMv5TE Linux program on an ARM926-class CPU board. It drives a
  128x32 DMD with 16 shades and talks to the node boards - switches, coils, LEDs, motors, a game's
  LCD insert - over an RS-485 node bus. src/spike1 runs that program: MAME's ARM9 core under Linux
  emulated at the system-call level, with the board's devices modelled from the game's own driver
  code. A title's ROMs are its game folder from Stern's update image; this driver hands them over
  and connects switches, coils, LEDs, displays, sound and NVRAM to PinMAME.

  PinMAME numbers are the factory manual's, which the game's own tables give:
    switches 1-95           the Switch Reference numbers. The flipper buttons and EOS switches
                            (each title's own numbers, spike1_tSwitches) sit in PinMAME's flipper
                            column, so the flipper keys reach them
    switches 101-116        the CPU board's C1-C16: 101-108 its DIP switches, set from the DIP
                            settings, 109-112 the service buttons, 116 the door's power sense
    switches 121-131        an optional topper's own switches, which it numbers 1 on (Game of
                            Thrones' TOPPER DRAGON is 121)
    solenoids 1-32          the coils by their Driver Reference number
    solenoids 51-72         a coil numbered 0 or above 32, in number order
    solenoids 46, 48        the flipper outputs: the coils the game names RIGHT FLIPPER and
                            LEFT FLIPPER, or RIGHT and LEFT FLIPPER POWER (and their HOLD coils
                            where a title has them), which the node board fires on its own when
                            the button closes. Upper flippers are their coils' solenoids only
    lamps 1-n               the LED channels by their Light Reference number (an RGB LED is three;
                            GI strings and flashers are LED channels too)
    mech 0-n                the motors the boards run on their own (Ghostbusters' Slimer), then
                            their steppers (Whoa Nellie's reels, Game of Thrones' dragons), then a
                            limit motor (KISS's Starchild): a motor's position in the game's
                            units, a stepper's step within its turn, a limit motor's way from home
                            (0) to away (100), through Controller.GetMech(n)
  Coils and LEDs are levels (a coil driver's PWM duty, an LED channel's brightness): modulated
  outputs hold them as 0-1, the solenoid and lamp bits say whether they are on at all.

  The display is the DMD, and below it the LCD insert when the title has one (Ghostbusters' Ecto
  goggles) or the CPU board's own LCD (WWE's playfield screen, 320x240), drawn in direct colour
  through 32768 palette pens after the core's own.

  Transformers The Pin and Avengers The Pin run Stern's earlier ELG framework: one node board, a
  16 character alphanumeric display of 16 segments (CORE_SEG16S, each segment at one of 16
  levels) and the game's sounds as WAV files. The game counts its switches, coils and lamps from 0,
  as its test menu shows them; their PinMAME numbers are one more.
************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpuintrf.h"
#include "usrintrf.h"
#include "spike1.h"
#include "../spike1/spike1_public.h"
#include "../spike1/spike1_debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAS_SPIKE1

#define SPIKE1_REGION     REGION_USER1 /* the game folder's files, mapped from the set on disk (ROMREGION_MAPPED) */
#define SPIKE1_AUDIO_RATE 44100        /* the stream's rate until the game sets /dev/i2s to its own */
#define SPIKE1_PEN0       (COL_COUNT + 48 + 48 + 48) /* the core's palette, then 32768 pens of RGB555 */
#define SPIKE1_MAXCOILS   64
#define SPIKE1_MAXLEDS    256
#define SPIKE1_MAXMECHS   8            /* Whoa Nellie's five reels are the most a title has */
#define SPIKE1_MAXFILES   512          /* Avengers The Pin's program and its 365 sounds are the most a title has */
#define SPIKE1_ALNUM_DIGITS 16         /* the ELG titles' display: the game keeps its text to 16 of the frame's 32 positions */
#define SPIKE1_FIRSTCUSTSOL CORE_FIRSTCUSTSOL /* solenoid 51 */
#define SPIKE1_NCUSTSOLS  (CORE_MODOUT_SOL_MAX - SPIKE1_FIRSTCUSTSOL + 1)
#define SPIKE1_GAMEONSOL  33           /* GAME ON, as SAM's: the flippers enabled - their reflexes armed */
#define SPIKE1_GAMEONBIT  0x10         /* its bit in solenoids2, where the core takes SAM's */
#define SPIKE1_IOFREQ     1000         /* the I/O exchange: the node boards scan their switches far faster than a frame */

/* The numbers a title's manual gives the switches PinMAME's keys reach; they differ between titles */
typedef struct {
  int flip[8]; /* left button, right button, left EOS, right EOS, then the same for the upper flippers (0: none) */
  int coin[4]; /* left, right, center, fourth */
  int tilt, slam;
} spike1_tSwitches;
#define SPIKE1_SWITCHES(lbutton, rbutton, leos, reos, coin1, coin2, coin3, coin4, tilt, slam) \
  { { lbutton, rbutton, leos, reos }, { coin1, coin2, coin3, coin4 }, tilt, slam }
/* a title with upper flippers, worked by their own leaf on each flipper button (Game of Thrones) */
#define SPIKE1_SWITCHES_UPPER(lbutton, rbutton, leos, reos, ulbutton, urbutton, coin1, coin2, coin3, coin4, tilt, slam) \
  { { lbutton, rbutton, leos, reos, ulbutton, urbutton }, { coin1, coin2, coin3, coin4 }, tilt, slam }

/* A stepper whose home switch is wired to another board (Game of Thrones' dragons): the stepper's
   board, by its node address, and its index there, and the switch's number; node 0 for none */
typedef struct { int node, stepper, sw; } spike1_tHome;
#define SPIKE1_NOHOMES { { 0 } }
#define SPIKE1_HOMES(node1, stepper1, sw1, node2, stepper2, sw2) { { node1, stepper1, sw1 }, { node2, stepper2, sw2 } }

/* A motor the game runs between two limit switches through two LED channels (KISS's Starchild):
   the lamp numbers of the channels that drive it toward the away switch and toward home, the two
   switches, and its travel time between them in ms; forward lamp 0 for none */
typedef struct { int forward, backward, home, away, travelMs; } spike1_tLimitMotor;
#define SPIKE1_NOLIMIT { 0 }
#define SPIKE1_LIMIT_MOTOR(forward, backward, home, away, travelMs) { forward, backward, home, away, travelMs }

/* The mechanism behind a board's encoder motor (Ghostbusters' Slimer), which the board runs on its
   own: the board's node address and motor index, its end stops in encoder counts from the edge of
   its home zone (the zone reaches from there down to the lower stop), and the counts a second it
   makes at full drive; node 0 for none */
typedef struct { int node, motor, lowStop, highStop, countsPerS; } spike1_tEncoderMotor;
#define SPIKE1_ENCODER_MOTOR(node, motor, lowStop, highStop, countsPerS) { node, motor, lowStop, highStop, countsPerS }

/* What a title adds to core_tGameData: its game folder's name (the program runs as
   /games/<folder>/game), its switch numbers, its steppers' home switches on other boards, its
   limit motor, for a game program shipped without its symbol table the addresses of the tables
   the subsystem reads (NULL when the program has its own), for a title whose game picks its
   display by the machine's /usr/local/spike/display.hex link that link's target (NULL: none), the
   mechanism of its encoder motor, and the program the machine runs when it is not the folder's
   "game" (NULL: "game") */
typedef struct {
  core_tGameData core;
  const char *folder;
  spike1_tSwitches sw;
  spike1_tHome home[2];
  spike1_tLimitMotor limit;
  const spike1_symbol *symbols;
  const char *displayLink;
  spike1_tEncoderMotor encoder;
  const char *program;
} spike1_tGameData;

static const spike1_tGameData *spike1_game(void) { return (const spike1_tGameData *)core_gameData; }

#if SPIKE1_DEBUG
/* SPIKE1_LOG set in the environment: once every five seconds, what the sound stream asked for and
   got, and the switch changes passed to the machine. Only in SPIKE1_DEBUG builds (spike1_debug.h) */
static struct { int on; unsigned frames, asked, got, switches, calls, peak, late, shortfalls; double cycles, run_s; cycles_t since; } diag;
#endif

static struct {
  int running, warned;
  int sw[256];         /* the title's switch numbers, and how many */
  int nsw;
  UINT8 swState[256];  /* each switch as last handed to the machine */
  int swM[256];        /* its matrix entry */
  UINT32 swSeen[256];  /* the count of changes to it as of the last look */
  UINT8 swAgain[256];  /* the last look found a change half made: the next one takes what it finds */
  int coilSol[SPIKE1_MAXCOILS]; /* the PinMAME solenoid of coil k, 0 if the game has no coil k */
  int coilOf[CORE_MODOUT_SOL_MAX + 1]; /* and back: the coil of solenoid s, -1 for none */
  UINT8 coilLevel[SPIKE1_MAXCOILS]; /* coil k's output as last passed on (spike1_pinmame_coil_output) */
  double coilOnAt[SPIKE1_MAXCOILS]; /* when coil k's output last came on */
  int flipCoil[2][2];  /* left, right: the flipper's coil and its separate hold coil (Whoa Nellie), -1 for none */
  int startSw;
  unsigned dmdFrames;
  unsigned nLeds;
  unsigned audioRate;  /* the sound stream's rate: the game's, once it has set one */
  int ledLamp[SPIKE1_MAXLEDS]; /* the lamp number of LED channel k, 0 for none */
  volatile int mech[SPIKE1_MAXMECHS]; /* each mech's position as of the last I/O exchange, for GetMech */
} locals;

static int spike1_stream = -1; /* the sound stream's first channel, the left; the right follows */

/* The machine's NVRAM block between the NVRAM handler and the machine: PinMAME reads the file
   before MACHINE_INIT and writes it after MACHINE_STOP, when the machine is gone */
static UINT8 *nvBlock;
static unsigned nvSize;

/*-------------------------------------------------
/  the CPU: game program, run for PinMAME's cycles
/-------------------------------------------------*/
int spike1cpu_ICount;

void spike1cpu_init(void) {}
void spike1cpu_reset(void *param) { (void)param; }
void spike1cpu_exit(void) {}

int spike1cpu_execute(int cycles)
{
  spike1cpu_ICount = cycles;
  if (locals.running) {
#if SPIKE1_DEBUG
    const cycles_t t0 = diag.on ? osd_cycles() : 0;
    spike1_pinmame_run(cycles);
    if (diag.on) { diag.calls++; diag.cycles += cycles; diag.run_s += (double)(osd_cycles() - t0) / (double)osd_cycles_per_second(); }
#else
    spike1_pinmame_run(cycles);
#endif
  }
  spike1cpu_ICount = 0;
  return cycles;
}

/* cpuintrf_init_cpu() refuses a CPU without a context, though this one keeps its own */
unsigned spike1cpu_get_context(void *dst) { (void)dst; return sizeof(void *); }
void spike1cpu_set_context(void *src) { (void)src; }
unsigned spike1cpu_get_reg(int regnum) { (void)regnum; return 0; }
void spike1cpu_set_reg(int regnum, unsigned val) { (void)regnum; (void)val; }
void spike1cpu_set_irq_line(int irqline, int state) { (void)irqline; (void)state; }
void spike1cpu_set_irq_callback(int (*callback)(int irqline)) { (void)callback; }

const char *spike1cpu_info(void *context, int regnum)
{
  (void)context;
  switch (regnum) {
  case CPU_INFO_NAME: return "SPIKE 1";
  case CPU_INFO_FAMILY: return "ARM9 (ARMv5TE) Linux";
  case CPU_INFO_VERSION: return "1.0";
  case CPU_INFO_FILE: return __FILE__;
  case CPU_INFO_CREDITS: return "MAME ARM7/ARM9 core; Linux at the system-call level";
  }
  return "";
}

unsigned spike1cpu_dasm(char *buffer, unsigned pc) { (void)pc; strcpy(buffer, "???"); return 4; }

static MEMORY_READ32_START(spike1_readmem)
  { 0x00000000, 0x00000003, MRA32_NOP },
MEMORY_END

static MEMORY_WRITE32_START(spike1_writemem)
  { 0x00000000, 0x00000003, MWA32_NOP },
MEMORY_END

/*-------------------------------------------------
/  switch numbers <-> PinMAME's matrix (16 columns of 8)
/    0-95     matrix entries 0-95, except that the title's flipper switches and the numbers 88-95
/             trade places: the flipper switches go to column 11, PinMAME's flipper column
/             (88 right EOS, 89 right button, 90 left EOS, 91 left button, 92-95 the same for the
/             upper flippers, as the core's CORE_SW*FLIP* bits lay them out), 88-95 to the entries they leave free
/    101-132  columns 12-15: the CPU board's C1-C16 as 101-116, an optional topper's switches
/             as 121 on; 132 also takes any number the game does not have
/-------------------------------------------------*/
static int spike1_flipSlot(int k)
{
  const int *flip = spike1_game()->sw.flip;
  static const int slot[8] = { 3, 1, 2, 0, 7, 5, 6, 4 }; /* entry 88 + k holds flip[slot[k]] */
  return flip[slot[k]];
}

static int spike1_swap(int n)
{
  int k;
  for (k = 0; k < 8; k++) {
    const int flip = spike1_flipSlot(k);
    if (!flip) continue;
    if (n == flip) return 88 + k;
    if (n == 88 + k) return flip;
  }
  return n;
}

static int spike1_sw2m(int no)
{
  if (no >= 0 && no <= 95) return spike1_swap(no);
  if (no >= 101 && no <= 132) return 96 + (no - 101);
  return 127;
}

static int spike1_m2sw(int col, int row)
{
  const int m = col * 8 + row;
  return m < 96 ? spike1_swap(m) : 101 + (m - 96);
}

/* lamp n <-> matrix entry n - 1; the core passes and expects matrix columns counted from 1 */
static int spike1_lamp2m(int no) { return no + 7; }
static int spike1_m2lamp(int col, int row) { return (col - 1) * 8 + row + 1; }

/*-------------------------------------------------
/  the cabinet keys
/-------------------------------------------------*/
#define SPIKE1_COMPORTS \
  PORT_START /* 2: CORE_COREINPORT */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,   IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,   KEYCODE_3) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,   KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Back",      KEYCODE_7) \
    COREPORT_BIT(     0x0020, "Minus",     KEYCODE_8) \
    COREPORT_BIT(     0x0040, "Plus",      KEYCODE_9) \
    COREPORT_BIT(     0x0080, "Select",    KEYCODE_0) \
    COREPORT_BITTOG(  0x0100, "Coin Door", KEYCODE_END) \
    COREPORT_BITDEF(  0x0200, IPT_START1,  IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0400, "Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT(     0x0800, "Slam Tilt", KEYCODE_HOME) \
  PORT_START /* 3: the CPU board's DIP switches (switches 101-108), read through core_getDip(0) */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "DIP 1") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0001, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "DIP 2") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0002, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "DIP 3") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0004, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "DIP 4") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0008, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "DIP 5") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0010, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "DIP 6") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0020, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "DIP 7") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0040, DEF_STR(On)) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "DIP 8") COREPORT_DIPSET(0x0000, DEF_STR(Off)) COREPORT_DIPSET(0x0080, DEF_STR(On))

/* the CPU board's own switches, which every title numbers alike (the cabinet's are in spike1_tSwitches) */
#define SPIKE1_SW_DIP1    101 /* C1-C8 */
#define SPIKE1_SW_BACK    112 /* C12, then MINUS C11, PLUS C10, SELECT C9 */
#define SPIKE1_SW_DCSENSE 116 /* C16, the coin door's power sense: closed while the door is shut */

static SWITCH_UPDATE(spike1)
{
  const spike1_tSwitches *sw = &spike1_game()->sw;
  int i;
  if (!inports) return;
  for (i = 0; i < 4; i++) if (sw->coin[i]) core_setSw(sw->coin[i], inports[CORE_COREINPORT] & (0x0001 << i));
  for (i = 0; i < 4; i++) core_setSw(SPIKE1_SW_BACK - i, inports[CORE_COREINPORT] & (0x0010 << i)); /* back, minus, plus, select */
  core_setSw(SPIKE1_SW_DCSENSE, !(inports[CORE_COREINPORT] & 0x0100));
  if (locals.startSw >= 0) core_setSw(locals.startSw, inports[CORE_COREINPORT] & 0x0200);
  if (sw->tilt) core_setSw(sw->tilt, inports[CORE_COREINPORT] & 0x0400);
  if (sw->slam) core_setSw(sw->slam, inports[CORE_COREINPORT] & 0x0800); /* a free-play title (Spider-Man) has no coin or slam switches */
}

/*-------------------------------------------------
/  the I/O, every millisecond (SPIKE1_IOFREQ): the game reads a board's switches every 2 to 5 ms,
/  at the node bus wire's pace, so a change is there for its next read. Coils, LEDs and the DMD go
/  out as soon as the game sets them. The core reports solenoid changes to its hosts once a frame
/  (core_updateSw), so a coil stays on for a frame at least: a pulse that came and went between two
/  frames still shows. Shared state the host's thread reads is built here first and then stored once
/-------------------------------------------------*/

/* A table sets switches from its own thread, and VPX runs a frame's physics in a burst: a fast ball
   through a trigger closes and opens it within a fraction of a millisecond, between two looks here.
   The core counts each change it makes (core_swChangedHook), so such a closure still reaches the
   machine - closed, then open */
static volatile UINT32 spike1_swChanges[128];

static void spike1_swChanged(int m)
{
  if (m >= 0 && m < 128) spike1_swChanges[m]++;
}

static void spike1_sync_io(void)
{
  int i, gameOn = 0;
  UINT32 sols = 0, sols2 = coreGlobals.solenoids2 & ~(UINT32)(CORE_LLFLIPSOLBITS | CORE_LRFLIPSOLBITS | SPIKE1_GAMEONBIT);
  UINT8 lamps[CORE_MAXLAMPCOL];
  const double now = timer_get_time(), frame = 1.0 / Machine->drv->frames_per_second;
  /* switches that changed, the DIP switches from the DIP settings */
  const int dips = core_getDip(0);
  for (i = 0; i < 8; i++) core_setSw(SPIKE1_SW_DIP1 + i, (dips >> i) & 1);
  for (i = 0; i < locals.nsw; i++) {
    /* the count, then the level. core_setSw sets the level (clearing the bit first) and then counts,
       so a level that disagrees with the count's parity is a change half made, taken at the next
       look - as is a change made without core_setSw (the core's flipper keys), taken then anyway */
    const UINT32 changes = spike1_swChanges[locals.swM[i]];
    const UINT8 closed = core_getSw(locals.sw[i]) ? 1 : 0;
    const int moved = closed != locals.swState[i];
    if (moved != (int)((changes - locals.swSeen[i]) & 1) && !locals.swAgain[i]) {
      locals.swAgain[i] = 1;
      continue;
    }
    locals.swAgain[i] = 0;
    if (moved) {
      locals.swState[i] = closed;
      spike1_pinmame_set_switch(locals.sw[i], closed);
      SPIKE1_DEBUG_ONLY(diag.switches++;
      if (diag.on) fprintf(stderr, "[spike1 driver] switch %d %s\n", locals.sw[i], closed ? "closed" : "opened");)
    } else if (changes != locals.swSeen[i]) { /* changed and changed back since the last look */
      spike1_pinmame_set_switch(locals.sw[i], !closed);
      spike1_pinmame_set_switch(locals.sw[i], closed);
      SPIKE1_DEBUG_ONLY(diag.switches += 2;
      if (diag.on) fprintf(stderr, "[spike1 driver] switch %d %s and back\n", locals.sw[i], closed ? "opened" : "closed");)
    }
    locals.swSeen[i] = changes;
  }
  /* coils */
  for (i = 0; i < SPIKE1_MAXCOILS; i++) {
    const int sol = locals.coilSol[i];
    if (sol) {
      unsigned level = spike1_pinmame_coil_output(i);
      if (level && !locals.coilLevel[i]) locals.coilOnAt[i] = now;
      else if (!level && now - locals.coilOnAt[i] <= frame) level = locals.coilLevel[i];
      locals.coilLevel[i] = (UINT8)level;
      coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + sol - 1].value = level / 255.0f;
      if (level && sol <= 32) sols |= CORE_SOLBIT(sol);
    }
  }
  /* the flipper outputs, and GAME ON: the flippers enabled. The game arms the flippers' reflexes on
     the boards - with the slings' and pops' - while a ball is in play, and disarms them when it
     tilts and when a ball or the game ends, so a table switches its own slings and pops with it */
  for (i = 0; i < 4; i++) {
    const int coil = locals.flipCoil[i >> 1][i & 1]; /* a flipper coil always has a solenoid, so its level is taken above */
    if (coil >= 0 && locals.coilLevel[coil]) sols2 |= (i >> 1) ? CORE_LRFLIPSOLBITS : CORE_LLFLIPSOLBITS;
    if (coil >= 0 && spike1_pinmame_coil_reflex_armed(coil)) gameOn = 1;
  }
  if (gameOn) sols2 |= SPIKE1_GAMEONBIT;
  coreGlobals.solenoids = coreGlobals.pulsedSolState = sols;
  coreGlobals.solenoids2 = sols2;
  coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + SPIKE1_GAMEONSOL - 1].value = gameOn ? 1.0f : 0.0f;
  /* LEDs */
  memset(lamps, 0, sizeof(lamps));
  for (i = 0; i < (int)locals.nLeds; i++) {
    const int lamp = locals.ledLamp[i] - 1;
    if (lamp >= 0) {
      const unsigned level = spike1_pinmame_led_level(i);
      coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + lamp].value = level / 255.0f;
      if (level) lamps[lamp / 8] |= 1 << (lamp % 8);
    }
  }
  for (i = 0; i < CORE_MAXLAMPCOL; i++) coreGlobals.lampMatrix[i] = lamps[i];
  /* the mechs, for GetMech */
  for (i = 0; i < SPIKE1_MAXMECHS; i++) {
    int position = 0;
    locals.mech[i] = spike1_pinmame_motor((unsigned)i, &position, NULL) ? position : 0;
  }
  /* the DMD: a new frame when the game has sent one (a title with the RGB matrix instead draws it as video, spike1_rgb_video) */
  if ((core_gameData->lcdLayout->type & CORE_SEGALL) == CORE_DMD && spike1_pinmame_dmd(NULL) != locals.dmdFrames) {
    static UINT8 dots[SPIKE1_DMD_WIDTH * SPIKE1_DMD_HEIGHT];
    locals.dmdFrames = spike1_pinmame_dmd(dots);
    core_dmd_submit_frame(core_gameData->lcdLayout, dots, 1);
  }
  /* the alphanumeric display (the ELG titles): the game's segment k is CORE_SEG16S bit 15 - k, and
     its level (0-15) the segment's modulated output */
  else if ((core_gameData->lcdLayout->type & CORE_SEGALL) == CORE_SEG16S && spike1_pinmame_alnum(NULL) != locals.dmdFrames) {
    static UINT8 levels[SPIKE1_ALNUM_CHARS * SPIKE1_ALNUM_SEGMENTS];
    int digit, k;
    locals.dmdFrames = spike1_pinmame_alnum(levels);
    for (digit = 0; digit < SPIKE1_ALNUM_DIGITS; digit++) {
      UINT16 lit = 0;
      for (k = 0; k < SPIKE1_ALNUM_SEGMENTS; k++) {
        const UINT8 level = levels[digit * SPIKE1_ALNUM_SEGMENTS + k];
        if (level) lit |= 1 << (15 - k);
        coreGlobals.physicOutputState[CORE_MODOUT_SEG0 + digit * 16 + 15 - k].value = level / 15.0f;
      }
      coreGlobals.segments[digit].w = lit;
    }
  }
}

static void spike1_io(int param)
{
  (void)param;
  if (locals.running) spike1_sync_io();
}

static INTERRUPT_GEN(spike1_vblank)
{
  char reason[128];
  if (locals.running) {
    /* the sound stream at the game's rate */
    const unsigned rate = spike1_pinmame_audio_rate();
    if (spike1_stream >= 0 && rate && rate != locals.audioRate) {
      stream_set_sample_rate(spike1_stream, rate);
      stream_set_sample_rate(spike1_stream + 1, rate);
      locals.audioRate = rate;
    }
#if SPIKE1_DEBUG
    if (diag.on && ++diag.frames % 300 == 0) {
      const cycles_t now = osd_cycles();
      const double wall = diag.since ? (double)(now - diag.since) / (double)osd_cycles_per_second() : 0.;
      fprintf(stderr, "[spike1 driver] %.1f s wall: CPU %u slices of %.0f cycles on average, %.1f s in the machine; "
              "sound: stream asked %u frames, got %u, late %u times, short %u, peak %u; the game's rate %u Hz; %u switch changes\n",
              wall, diag.calls, diag.calls ? diag.cycles / diag.calls : 0., diag.run_s,
              diag.asked, diag.got, diag.late, diag.shortfalls, diag.peak, spike1_pinmame_audio_rate(), diag.switches);
      diag.asked = diag.got = diag.calls = diag.peak = diag.late = diag.shortfalls = 0;
      diag.cycles = diag.run_s = 0.;
      diag.since = now;
    }
#endif
    if (!locals.warned && !spike1_pinmame_running(reason, sizeof(reason))) {
      locals.warned = 1;
      usrintf_showmessage_secs(30, "SPIKE 1: the game program stopped: %s", reason);
    }
  }
  core_updateSw(0); /* the flippers are the node boards' own (FLIP_SOL), see spike1_sync_io */
}

/* mech n: the n-th motor's position, as the last I/O exchange took it (spike1_sync_io): a host asks
   from its own thread, and the machine's motors run on the machine's */
static int spike1_getMech(int mechNo)
{
  return mechNo >= 0 && mechNo < SPIKE1_MAXMECHS ? locals.mech[mechNo] : 0;
}

/* the coils PinMAME numbers 51 on: a coil numbered 0 or above 32 */
static int spike1_getSol(int solNo)
{
  if (solNo < SPIKE1_FIRSTCUSTSOL || solNo > CORE_MODOUT_SOL_MAX || locals.coilOf[solNo] < 0) return 0;
  return locals.coilLevel[locals.coilOf[solNo]];
}

/*-------------------------------------------------
/  the LCD insert, drawn below the DMD
/-------------------------------------------------*/
static PINMAME_VIDEO_UPDATE(spike1_insert_video)
{
  static UINT16 rgb[SPIKE1_INSERT_WIDTH * SPIKE1_INSERT_HEIGHT];
  unsigned backlight = 0;
  const int shown = spike1_pinmame_insert(rgb, &backlight);
  int x, y;
  (void)cliprect;
  for (y = 0; y < SPIKE1_INSERT_HEIGHT && layout->top + y < bitmap->height; y++) {
    UINT16 * const line = (UINT16 *)bitmap->line[layout->top + y] + layout->left;
    for (x = 0; x < SPIKE1_INSERT_WIDTH && layout->left + x < bitmap->width; x++) {
      UINT32 c = 0;
      if (shown) { /* RGB565 to RGB555, dimmed with the backlight */
        const UINT32 p = rgb[y * SPIKE1_INSERT_WIDTH + x];
        const UINT32 r = ((p >> 11) & 0x1f) * backlight / 255, g = ((p >> 6) & 0x1f) * backlight / 255, b = (p & 0x1f) * backlight / 255;
        c = (r << 10) | (g << 5) | b;
      }
      line[x] = (UINT16)Machine->pens[SPIKE1_PEN0 + c];
    }
  }
}

/*-------------------------------------------------
/  the CPU board's LCD (WWE's playfield screen), drawn below the DMD
/-------------------------------------------------*/
static PINMAME_VIDEO_UPDATE(spike1_lcd_video)
{
  static UINT16 rgb[SPIKE1_LCD_WIDTH * SPIKE1_LCD_HEIGHT];
  const int shown = spike1_pinmame_lcd(rgb);
  int x, y;
  (void)cliprect;
  for (y = 0; y < SPIKE1_LCD_HEIGHT && layout->top + y < bitmap->height; y++) {
    UINT16 * const line = (UINT16 *)bitmap->line[layout->top + y] + layout->left;
    for (x = 0; x < SPIKE1_LCD_WIDTH && layout->left + x < bitmap->width; x++) {
      UINT32 c = 0;
      if (shown) { /* RGB565 to RGB555 */
        const UINT32 p = rgb[y * SPIKE1_LCD_WIDTH + x];
        c = ((p >> 1) & 0x7fe0) | (p & 0x1f);
      }
      line[x] = (UINT16)Machine->pens[SPIKE1_PEN0 + c];
    }
  }
}

/*-------------------------------------------------
/  the RGB LED matrix (Spider-Man's display, in the DMD's place)
/-------------------------------------------------*/
static PINMAME_VIDEO_UPDATE(spike1_rgb_video)
{
  static UINT16 rgb[SPIKE1_RGB_WIDTH * SPIKE1_RGB_HEIGHT];
  const unsigned frames = spike1_pinmame_rgb(rgb);
  int x, y;
  (void)cliprect;
  for (y = 0; y < SPIKE1_RGB_HEIGHT && layout->top + y < bitmap->height; y++) {
    UINT16 * const line = (UINT16 *)bitmap->line[layout->top + y] + layout->left;
    for (x = 0; x < SPIKE1_RGB_WIDTH && layout->left + x < bitmap->width; x++) {
      const UINT32 p = frames ? rgb[y * SPIKE1_RGB_WIDTH + x] : 0;
      line[x] = (UINT16)Machine->pens[SPIKE1_PEN0 + (((p >> 1) & 0x7fe0) | (p & 0x1f))]; /* RGB565 to RGB555 */
    }
  }
}

/* The key the encrypted LCD clips (WWE's .spv files) are RC4-encrypted with, which each board's
   factory key store holds and the ROM set does not: the user puts it in <set>.txt beside the set
   (wwe_135h.txt) as 32 hex digits. Spaces, commas and 0x prefixes are allowed. 1 when the file
   holds a key, 0 when there is no file, -1 when the file holds no key */
static int spike1_hex_digit(char c)
{
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

static int spike1_read_video_key(const char *name, unsigned char key[16])
{
  mame_file *file = mame_fopen(NULL, name, FILETYPE_ROM, 0);
  char text[512];
  UINT32 length, i;
  int digits = 0;

  if (!file) return 0;
  length = mame_fread(file, text, sizeof(text));
  mame_fclose(file);
  i = length >= 3 && memcmp(text, "\xef\xbb\xbf", 3) == 0 ? 3 : 0; /* a UTF-8 byte order mark */
  for (; i < length; i++) {
    const char c = text[i];
    const int value = spike1_hex_digit(c);
    if (value < 0) {
      /* the 0 of a 0x prefix, which starts a byte, was taken for a digit */
      if ((c == 'x' || c == 'X') && digits % 2 == 1 && text[i - 1] == '0' && (i < 2 || spike1_hex_digit(text[i - 2]) < 0))
        digits--;
      else if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != ',')
        return -1;
      continue;
    }
    if (digits == 32) return -1;
    key[digits / 2] = (unsigned char)(digits % 2 ? key[digits / 2] | value : value << 4);
    digits++;
  }
  return digits == 32 ? 1 : -1;
}

/* Gives a title with encrypted LCD clips the key from its key file, or says what is missing */
static void spike1_provide_video_key(void)
{
  char name[64];
  unsigned char key[16];
  int found;

  if (!spike1_pinmame_needs_video_key()) return;
  sprintf(name, "%s.txt", Machine->gamedrv->name);
  found = spike1_read_video_key(name, key);
  if (found > 0 && spike1_pinmame_video_key(key)) return;
  if (found < 0)
    usrintf_showmessage_secs(30, "SPIKE 1: %s does not hold a video key (32 hex digits)", name);
  else
    usrintf_showmessage_secs(30, "SPIKE 1: the LCD videos need their key: put it in %s beside the ROM set", name);
}

static core_tLCDLayout spike1_dmd[] = {
  {0, 0, SPIKE1_DMD_HEIGHT, SPIKE1_DMD_WIDTH, CORE_DMD, NULL, NULL},
  {0}
};

/* The insert starts 68 pixels down: below the DMD at its double size (64) as well as at its single */
static core_tLCDLayout spike1_dmd_insert[] = {
  {0, 0, SPIKE1_DMD_HEIGHT, SPIKE1_DMD_WIDTH, CORE_DMD, NULL, NULL},
  {68, 0, SPIKE1_INSERT_HEIGHT, SPIKE1_INSERT_WIDTH, CORE_VIDEO, (genf *)spike1_insert_video, NULL},
  {0}
};

/* The CPU board's LCD in the same place */
static core_tLCDLayout spike1_dmd_lcd[] = {
  {0, 0, SPIKE1_DMD_HEIGHT, SPIKE1_DMD_WIDTH, CORE_DMD, NULL, NULL},
  {68, 0, SPIKE1_LCD_HEIGHT, SPIKE1_LCD_WIDTH, CORE_VIDEO, (genf *)spike1_lcd_video, NULL},
  {0}
};

/* The RGB matrix alone, a title's only display */
static core_tLCDLayout spike1_rgb[] = {
  {0, 0, SPIKE1_RGB_HEIGHT, SPIKE1_RGB_WIDTH, CORE_VIDEO, (genf *)spike1_rgb_video, NULL},
  {0}
};

/* The ELG titles' alphanumeric display, their only one: a line of 16 characters */
static core_tLCDLayout spike1_alnum[] = {
  {0, 0, 0, SPIKE1_ALNUM_DIGITS, CORE_SEG16S, NULL, NULL},
  {0}
};

/*-------------------------------------------------
/  sound: the game's DAC, 16-bit stereo, at the rate the game sets (Ghostbusters 44.1 kHz,
/  Whoa Nellie 24 kHz): spike1_sync_io() moves the stream to it, the mixer resamples
/-------------------------------------------------*/

/* The machine makes its samples in time slices, and the stream can ask for a frame's worth before
   the slice that makes them has run. So the samples pass through a FIFO that starts playing only
   once 1/30 s is waiting; should it run dry anyway, the last sample is held - a gap of zeros would
   click - and the FIFO fills to its mark again */
#define SPIKE1_FIFO 16384 /* stereo frames */
static struct { INT16 s[2 * SPIKE1_FIFO]; int head, count, primed; INT16 last[2]; } fifo;

static void spike1_snd_update(int num, INT16 **buffer, int length)
{
  static INT16 tmp[2 * 1024];
  const int mark = (int)(locals.audioRate ? locals.audioRate : SPIKE1_AUDIO_RATE) / 30;
  int i, got = 0;
  (void)num;
  while (locals.running && fifo.count < SPIKE1_FIFO) {
    const int room = SPIKE1_FIFO - fifo.count, want = room > 1024 ? 1024 : room;
    const int n = (int)spike1_pinmame_audio(tmp, want);
    for (i = 0; i < n; i++) {
      const int at = (fifo.head + fifo.count + i) % SPIKE1_FIFO;
      fifo.s[2 * at    ] = tmp[2 * i];
      fifo.s[2 * at + 1] = tmp[2 * i + 1];
    }
    SPIKE1_DEBUG_ONLY(for (i = 0; i < 2 * n; i++) { const unsigned a = (unsigned)(tmp[i] < 0 ? -tmp[i] : tmp[i]); if (a > diag.peak) diag.peak = a; })
    fifo.count += n;
    got += n;
    if (n < want) break;
  }
  SPIKE1_DEBUG_ONLY(diag.got += got;
  diag.asked += length;
  if (got < length) diag.late++;) /* the machine had less ready than the stream asked: without the FIFO, a gap */
  if (!fifo.primed && fifo.count >= length + mark) fifo.primed = 1;
  for (i = 0; i < length; i++) {
    if (fifo.primed && fifo.count) {
      fifo.last[0] = fifo.s[2 * fifo.head];
      fifo.last[1] = fifo.s[2 * fifo.head + 1];
      fifo.head = (fifo.head + 1) % SPIKE1_FIFO;
      fifo.count--;
    } else if (fifo.primed) { /* ran dry: hold, and wait for the mark again */
      fifo.primed = 0;
      SPIKE1_DEBUG_ONLY(diag.shortfalls++;)
    }
    buffer[0][i] = fifo.last[0];
    buffer[1][i] = fifo.last[1];
  }
}

static int spike1_sh_start(const struct MachineSound *msound)
{
  static const char *names[] = { "SPIKE 1 Left", "SPIKE 1 Right" };
  const int vol[2] = { MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) };
  (void)msound;
  spike1_stream = stream_init_multi(2, names, vol, SPIKE1_AUDIO_RATE, 0, spike1_snd_update);
  return spike1_stream < 0;
}

static void spike1_sh_stop(void) { spike1_stream = -1; }

static struct CustomSound_interface spike1_sndInt = { spike1_sh_start, spike1_sh_stop, 0 };

/*-------------------------------------------------
/  start and stop
/-------------------------------------------------*/
static void spike1_map_outputs(void)
{
  int i, number, kind, cust = 0;
  const char *name;
  for (i = 0; i < SPIKE1_MAXCOILS; i++) locals.coilSol[i] = 0;
  for (i = 0; i <= CORE_MODOUT_SOL_MAX; i++) locals.coilOf[i] = -1;
  locals.flipCoil[0][0] = locals.flipCoil[0][1] = locals.flipCoil[1][0] = locals.flipCoil[1][1] = -1;
  for (i = 0; spike1_pinmame_coil(i, &number, &name); i++) {
    int sol;
    if (number < 0 || number >= SPIKE1_MAXCOILS) continue;
    if (number >= 1 && number <= 32) sol = number;
    else if (cust < SPIKE1_NCUSTSOLS) sol = SPIKE1_FIRSTCUSTSOL + cust++;
    else continue;
    locals.coilSol[number] = sol;
    locals.coilOf[sol] = number;
    {
      /* Supreme writes LT and RT (RT FLIPPER POWER) */
      char full[64];
      if (!strncmp(name, "LT ", 3)) snprintf(full, sizeof(full), "LEFT %s", name + 3);
      else if (!strncmp(name, "RT ", 3)) snprintf(full, sizeof(full), "RIGHT %s", name + 3);
      else snprintf(full, sizeof(full), "%s", name);
      if (!strcmp(full, "LEFT FLIPPER") || !strcmp(full, "LEFT FLIPPER POWER")) locals.flipCoil[0][0] = number;
      if (!strcmp(full, "LEFT FLIPPER HOLD")) locals.flipCoil[0][1] = number;
      if (!strcmp(full, "RIGHT FLIPPER") || !strcmp(full, "RIGHT FLIPPER POWER")) locals.flipCoil[1][0] = number;
      if (!strcmp(full, "RIGHT FLIPPER HOLD")) locals.flipCoil[1][1] = number;
    }
  }
  /* the core counts lamps in whole columns, up to the highest Light Reference number of the title;
     a motor drive is no lamp, and Light Reference 0 has no PinMAME number */
  coreGlobals.nLamps = 8 * (CORE_CUSTLAMPCOL + core_gameData->hw.lampCol);
  locals.nLeds = 0;
  for (i = 0; i < SPIKE1_MAXLEDS && spike1_pinmame_led(i, &number, &kind, &name); i++) {
    locals.ledLamp[i] = number >= 1 && number <= coreGlobals.nLamps && !(kind & SPIKE1_LED_MOTOR) ? number : 0;
    locals.nLeds = i + 1;
  }
  /* through the lower flippers (45-48) at least, so GAME ON (33) and they are physical outputs too */
  coreGlobals.nSolenoids = cust ? SPIKE1_FIRSTCUSTSOL - 1 + cust : CORE_FIRSTLFLIPSOL + 3;
  coreGlobals.nGI = 0;
  /* the alphanumeric display's segments are modulated outputs too (spike1_sync_io) */
  coreGlobals.nAlphaSegs = (core_gameData->lcdLayout->type & CORE_SEGALL) == CORE_SEG16S ? SPIKE1_ALNUM_DIGITS * 16 : 0;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_NONE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_NONE);
}

static void spike1_map_switches(void)
{
  int i, number, closed;
  const char *name;
  locals.nsw = 0;
  for (i = 0; locals.nsw < 256 && spike1_pinmame_switch(i, &number, &closed, &name); i++) {
    locals.sw[locals.nsw] = number;
    locals.swState[locals.nsw] = (UINT8)closed;
    locals.nsw++;
    /* what is closed at rest - the trough with its balls, the door's power sense - starts closed in PinMAME too */
    core_setSw(number, closed);
  }
  for (i = 0; i < locals.nsw; i++) {
    locals.swM[i] = spike1_sw2m(locals.sw[i]);
    locals.swSeen[i] = spike1_swChanges[locals.swM[i]];
  }
  core_swChangedHook = spike1_swChanged;
  locals.startSw = spike1_pinmame_find_switch("START BUTTON");
  if (locals.startSw < 0) locals.startSw = spike1_pinmame_find_switch("START"); /* the ELG titles' name */
}

static MACHINE_INIT(spike1)
{
  spike1_file files[SPIKE1_MAXFILES];
  unsigned count = 0;
  const struct RomModule *region, *rom;
  const char *missing = NULL, *inMemory = NULL;
  char error[256];
  int c;

  memset(&locals, 0, sizeof(locals));
  memset(&fifo, 0, sizeof(fifo));
#if SPIKE1_DEBUG
  memset(&diag, 0, sizeof(diag));
  diag.on = getenv("SPIKE1_LOG") != NULL;
#endif
  locals.startSw = -1;
  /* the insert's colours: the pens after the core's palette are RGB555 */
  for (c = 0; c < 32768; c++)
    palette_set_color(SPIKE1_PEN0 + c, (c >> 7 & 0xf8) | (c >> 12), (c >> 2 & 0xf8) | (c >> 7 & 7), (c << 3 & 0xf8) | (c >> 2 & 7));
  if ((core_gameData->lcdLayout->type & CORE_SEGALL) == CORE_DMD)
    core_dmd_pwm_init(core_gameData->lcdLayout, CORE_DMD_PWM_PREINTEGRATED_LINEAR_16, CORE_DMD_PWM_PREINTEGRATED_LINEAR_16, 0);

  /* the title's files, which the ROM loader keeps mapped from the set on disk for the machine's
     life: a stored zip entry or a file in the set's folder. One it holds in memory instead
     (a compressed entry) costs its size in RAM */
  for (region = rom_first_region(Machine->gamedrv); region; region = rom_next_region(region)) {
    if (ROMREGION_GETTYPE(region) != SPIKE1_REGION) continue;
    for (rom = rom_first_file(region); rom && count < sizeof(files) / sizeof(files[0]); rom = rom_next_file(rom)) {
      UINT32 length = 0;
      int mapped = 0;
      files[count].name = ROM_GETNAME(rom);
      files[count].data = get_mapped_rom(rom, &length, &mapped);
      files[count].size = length;
      if (!files[count].data) missing = files[count].name;
      else if (!mapped && !inMemory) inMemory = files[count].name;
      count++;
    }
  }
  if (missing) {
    usrintf_showmessage_secs(30, "SPIKE 1: the ROM set's %s is not open", missing);
    return;
  }
  if (!spike1_pinmame_start(spike1_game()->folder, spike1_game()->program, files, count, spike1_game()->symbols, nvBlock, nvSize, error, sizeof(error))) {
    usrintf_showmessage_secs(30, "SPIKE 1: the game program does not start: %s", error);
    return;
  }
  locals.running = 1;
  if (inMemory) {
    logerror("SPIKE 1: %s is held in memory, not mapped from disk: a set's zip maps only what it stores uncompressed\n", inMemory);
    usrintf_showmessage_secs(10, "SPIKE 1: %s is held in memory, not mapped from disk (a compressed zip?)", inMemory);
  }
  spike1_provide_video_key();
  if (spike1_game()->displayLink) spike1_pinmame_display_link(spike1_game()->displayLink);
  for (c = 0; c < 2; c++) {
    const spike1_tHome *home = &spike1_game()->home[c];
    if (home->node) spike1_pinmame_stepper_home(home->node, home->stepper, home->sw);
  }
  {
    const spike1_tLimitMotor *limit = &spike1_game()->limit;
    if (limit->forward) spike1_pinmame_limit_motor(limit->forward, limit->backward, limit->home, limit->away, (unsigned)limit->travelMs);
  }
  {
    const spike1_tEncoderMotor *encoder = &spike1_game()->encoder;
    if (encoder->node) spike1_pinmame_encoder_motor((unsigned)encoder->node, (unsigned)encoder->motor, encoder->lowStop, encoder->highStop, (unsigned)encoder->countsPerS);
  }
  spike1_map_switches();
  spike1_map_outputs();
}

static MACHINE_STOP(spike1)
{
  if (locals.running) {
    /* switched off like the machine: the game commits what it keeps in memory (settings, audits,
       high scores) to its NVRAM files when the power fails, and only then is the block complete */
    spike1_pinmame_power_down(5000);
    /* the NVRAM handler saves after the machine is gone: take its block now */
    const unsigned size = spike1_pinmame_nvram(NULL, 0);
    UINT8 *block = (UINT8 *)malloc(size ? size : 1);
    if (block && spike1_pinmame_nvram(block, size) == size) {
      free(nvBlock);
      nvBlock = block;
      nvSize = size;
    } else
      free(block);
  }
  locals.running = 0;
  core_swChangedHook = NULL;
  spike1_pinmame_stop();
}

/* The block is the machine's own (see spike1_linux::nvram()) and its size changes as the game
   writes, so it is saved as it is rather than through core_nvram()'s fixed sizes */
static NVRAM_HANDLER(spike1)
{
  if (read_or_write) {
    if (file && nvBlock && nvSize) mame_fwrite(file, nvBlock, nvSize);
    return;
  }
  free(nvBlock);
  nvBlock = NULL;
  nvSize = 0;
  if (file) {
    const UINT64 size = mame_fsize(file);
    if (size && size < (64u << 20) && (nvBlock = (UINT8 *)malloc((size_t)size)) != NULL)
      nvSize = mame_fread(file, nvBlock, (size_t)size);
  }
}

MACHINE_DRIVER_START(spike1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(spike1, NULL, spike1)
  MDRV_CPU_ADD_TAG("mcpu", SPIKE1, SPIKE1_CPU_HZ)
  MDRV_CPU_MEMORY(spike1_readmem, spike1_writemem)
  MDRV_CPU_VBLANK_INT(spike1_vblank, 1)
  MDRV_TIMER_ADD(spike1_io, SPIKE1_IOFREQ)
  MDRV_NVRAM_HANDLER(spike1)
  MDRV_SWITCH_UPDATE(spike1)
  MDRV_SWITCH_CONV(spike1_sw2m, spike1_m2sw)
  MDRV_LAMP_CONV(spike1_lamp2m, spike1_m2lamp)
  MDRV_SOUND_ADD(CUSTOM, spike1_sndInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_PALETTE_LENGTH(SPIKE1_PEN0 + 32768)
MACHINE_DRIVER_END

/* A title with the CPU board's LCD (WWE): the DMD with the LCD below it is taller than PinMAME's default screen outside VPinMAME (320x256) */
MACHINE_DRIVER_START(spike1_lcd)
  MDRV_IMPORT_FROM(spike1)
#if CORE_SCREENY < 68 + SPIKE1_LCD_HEIGHT
  MDRV_SCREEN_SIZE(SPIKE1_LCD_WIDTH, 68 + SPIKE1_LCD_HEIGHT)
  MDRV_VISIBLE_AREA(0, SPIKE1_LCD_WIDTH - 1, 0, 68 + SPIKE1_LCD_HEIGHT - 1)
#endif
MACHINE_DRIVER_END

/*-------------------------------------------------
/  the games
/-------------------------------------------------*/
#define SPIKE1_INPUT_PORTS(name) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SPIKE1_COMPORTS \
  INPUT_PORTS_END

/* layout: spike1_dmd, or spike1_dmd_insert for a title with an LCD insert; lamps: its highest Light
   Reference number - the lamps take the 8 standard lamp columns and as many custom ones as they need;
   switches: SPIKE1_SWITCHES(...) with the manual's numbers. The flipper switches need no FLIP_SWNO:
   spike1_sw2m() puts them in the flipper column */
#define SPIKE1_LAMPCOLS(lamps) (((lamps) + 7) / 8 > CORE_CUSTLAMPCOL ? ((lamps) + 7) / 8 - CORE_CUSTLAMPCOL : 0)
#define SPIKE1_CORE(layout, lamps, flippers) \
  { GEN_SPIKE1, layout, \
    { FLIP_SW(flippers) | FLIP_SOL(FLIP_L), 4, SPIKE1_LAMPCOLS(lamps), SPIKE1_NCUSTSOLS, 0, 0, 0, 0, spike1_getSol, NULL, spike1_getMech } }
#define SPIKE1_INIT(name, folder, layout, lamps, switches) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(layout, lamps, FLIP_L), folder, switches, SPIKE1_NOHOMES, SPIKE1_NOLIMIT }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* the same for a title with upper flippers (flippers FLIP_L | FLIP_U, switches SPIKE1_SWITCHES_UPPER),
   home switches on other boards (homes SPIKE1_HOMES) or a limit motor (limit SPIKE1_LIMIT_MOTOR) */
#define SPIKE1_INIT_EX(name, folder, layout, lamps, flippers, switches, homes, limit) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(layout, lamps, flippers), folder, switches, homes, limit }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* the same as SPIKE1_INIT for a game program without a symbol table: symbols lists the addresses */
#define SPIKE1_INIT_SYMBOLS(name, folder, layout, lamps, switches, symbols) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(layout, lamps, FLIP_L), folder, switches, SPIKE1_NOHOMES, SPIKE1_NOLIMIT, symbols }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* the same for a title with the RGB matrix as its display (Spider-Man): displayLink is the file the
   machine's /usr/local/spike/display.hex points at, whose name tells the game its display type */
#define SPIKE1_INIT_RGB(name, folder, lamps, switches, displayLink) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(spike1_rgb, lamps, FLIP_L), folder, switches, SPIKE1_NOHOMES, SPIKE1_NOLIMIT, NULL, displayLink }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* the same as SPIKE1_INIT for a title with an encoder motor (encoder SPIKE1_ENCODER_MOTOR) */
#define SPIKE1_INIT_ENCODER(name, folder, layout, lamps, switches, encoder) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(layout, lamps, FLIP_L), folder, switches, SPIKE1_NOHOMES, SPIKE1_NOLIMIT, NULL, NULL, encoder }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* the same for a title on Stern's ELG framework (Transformers, Avengers The Pin): the alphanumeric
   display as its only one, and program the game folder's program the machine runs, the target of
   its /usr/local/games/game link */
#define SPIKE1_INIT_ALNUM(name, folder, program, lamps, switches) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { SPIKE1_CORE(spike1_alnum, lamps, FLIP_L), folder, switches, SPIKE1_NOHOMES, SPIKE1_NOLIMIT, NULL, NULL, { 0 }, program }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }
/* Another edition of a title (a Pro beside its LE): the set name_ver, a clone of the set parent,
   with its own game data from SPIKE1_INIT*(data, ...). Its files are the edition's own; those it
   shares with the parent (the asset image, the node firmware) can come from the parent's ROM set */
#define SPIKE1_CLONEDEF(name, ver, parent, data, longname, year, machine) \
  GAMEX(year, name##_##ver, parent, machine, data, data, ROT0, "Stern", longname, 0)

/*-------------------------------------------------------------------
/ Transformers The Pin
/-------------------------------------------------------------------*/
/* Stern's ELG framework (game folder "tf-elg", from the update image transformers_pin-1.0.18.iso): one
   node board (node 8: 46 switches, 8 coils, 64 lamps), the 16 character alphanumeric display, and
   its sounds as WAV files. The machine runs the folder's "gamer", 1.0.18 (its /usr/local/games/game
   link; the folder's "game" is an older build, v0.0.123). A home model: no coin door. The numbers
   are the game's own plus one */
SPIKE1_INIT_ALNUM(tfcp, "tf-elg", "gamer", 64, SPIKE1_SWITCHES(2, 3, 4, 5, 0, 0, 0, 0, 46, 0))
ROM_START(tfcp_1018)
  ROM_REGION(0x02c39000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("gamer", 0x00000000, 0x00104148, CRC(14587823) SHA1(42647a7dc47d596a0c455a483fa2c350eb2da139))
    ROM_LOAD("Award_02_A.wav", 0x00105000, 0x0002f37e, CRC(6041b02e) SHA1(10075e7cd245d2d933e9c7314ab508ede99cbc17))
    ROM_LOAD("Award_02_A_short.wav", 0x00135000, 0x00004f5a, CRC(c56c4639) SHA1(7356bc41f8aa5fbea84c506809c8bed84965ef5f))
    ROM_LOAD("BG_AutoBotBonus_Intro.wav", 0x0013a000, 0x0005296e, CRC(24b9d750) SHA1(5e8371bb547ba5d83fc7949e7d06954a0f875165))
    ROM_LOAD("BG_AutoBotBonus_loop.wav", 0x0018d000, 0x0001f25e, CRC(af67a81f) SHA1(7fcb341483fe4c659102e5703d7855ce31ebe64d))
    ROM_LOAD("BG_AutoBotMainPlay_loop.wav", 0x001ad000, 0x00103978, CRC(4149659f) SHA1(3cc4599dc4d91ed0bb119053dfa7ed514b9b0cfb))
    ROM_LOAD("BG_AutoBotMainPlay_Opening.wav", 0x002b1000, 0x00103b48, CRC(f34d0ffe) SHA1(c829f12e9381f819a277337e7368e901d3e0ae84))
    ROM_LOAD("BG_AutobotSelect_intro.wav", 0x003b5000, 0x0000aefc, CRC(9fcf9ead) SHA1(7fbc2ea0a113c36ad4c82cbb25ba97d152bb4067))
    ROM_LOAD("BG_AutobotSelect_loop.wav", 0x003c0000, 0x000567ec, CRC(d437e830) SHA1(8f88463a5fba2bec85d71a6f778dfd16820adb62))
    ROM_LOAD("BG_Blackout_Loop.wav", 0x00417000, 0x00103a82, CRC(3c1375d0) SHA1(51933719fac8b6c71f731cfcb2f5563009d43bbd))
    ROM_LOAD("BG_BumbleBee_Intro.wav", 0x0051b000, 0x0000535c, CRC(2b7284e1) SHA1(48c7464ecf9d7ba62307489651bdd15bb896f733))
    ROM_LOAD("BG_BumbleBee_Loop.wav", 0x00521000, 0x000d96f0, CRC(a5db75cc) SHA1(222d7e760a027bd2d765bee9b3def05e3c50f1ef))
    ROM_LOAD("BG_DecepticonBonus_loop.wav", 0x005fb000, 0x00016306, CRC(0abda797) SHA1(a41a4a386d3a3466f73bc090c1fac2cf2c6200c9))
    ROM_LOAD("BG_DecepticonBonus_Start.wav", 0x00612000, 0x0003f98e, CRC(7ec3a726) SHA1(a53d4921568b6905bab27a5b0b3d08708c684a77))
    ROM_LOAD("BG_DecepticonMainPlay.wav", 0x00652000, 0x001780a2, CRC(997fbc2a) SHA1(26c1f75937fa53a5b89a171852ffe84be3af85d8))
    ROM_LOAD("BG_DecepticonSelect_Intro.wav", 0x007cb000, 0x0001785e, CRC(556b0589) SHA1(441ce71a75d30e460d7d8bf3a8813f4fd0c0d530))
    ROM_LOAD("BG_DecepticonSelect_Loop.wav", 0x007e3000, 0x0005dbb4, CRC(5a96c5eb) SHA1(4ed7bca4ec6ecebe9753265f573012874ed2feb1))
    ROM_LOAD("BG_Devastator_Intro.wav", 0x00841000, 0x0006d174, CRC(d023a7bf) SHA1(7cf90eb402a133b8b89a63c2b0229c9ac7677c91))
    ROM_LOAD("BG_Devastator_loop.wav", 0x008af000, 0x00110312, CRC(84048cb0) SHA1(080f85c1980d0037cadec23473a7dee3a048caf3))
    ROM_LOAD("BG_FastScoring_loop.wav", 0x009c0000, 0x000f11c4, CRC(683c97b2) SHA1(c1bdf7e5551d3e35a1e3a0a7ddb600f245c58e59))
    ROM_LOAD("BG_Ironhide2a Intro.wav", 0x00ab2000, 0x0003ea0e, CRC(77f8474d) SHA1(1bc739f66b773d53090db4b7aaef05e061c5e48c))
    ROM_LOAD("BG_Ironhide2a loop.wav", 0x00af1000, 0x000cb29c, CRC(1308eb92) SHA1(8e228b37c7a435633e91d8740e9de5552db0db10))
    ROM_LOAD("BG_MandS_Twins_intro.wav", 0x00bbd000, 0x00029f2a, CRC(4df7dadd) SHA1(c8ce4dc10561c5ed5f57b4c7ce1fb64f61f705ba))
    ROM_LOAD("BG_MandS_Twins_loop.wav", 0x00be7000, 0x0014d1b0, CRC(4cbaafcd) SHA1(ab85ea6059e1c3797e1a9768ce995aed45e40726))
    ROM_LOAD("BG_Megatron_Multiballintro.wav", 0x00d35000, 0x0005b8ba, CRC(92278335) SHA1(e31af7fbd38d6b99598d6f721b13c112d7f13fdc))
    ROM_LOAD("BG_Megatron_Multiballloop.wav", 0x00d91000, 0x00157586, CRC(9c8162ab) SHA1(b60cf65e0e08659aeaacaba081c22508a11db7c5))
    ROM_LOAD("BG_MiniWizard_Intro.wav", 0x00ee9000, 0x00025a82, CRC(6ac24c7c) SHA1(06448860cbdc2297cca097dd50aeb9ebb5a90ebb))
    ROM_LOAD("BG_MiniWizard_loop.wav", 0x00f0f000, 0x000e1826, CRC(fd36f10d) SHA1(51c30a37ec0bb36755eb83b6ae1a6342226ce5f2))
    ROM_LOAD("BG_OptimusMultiball3_intro.wav", 0x00ff1000, 0x0006bf2c, CRC(ac11c6c4) SHA1(90487a3f235b24a0c812c16a1f7326a1bb43fc52))
    ROM_LOAD("BG_OptimusMultiball3_loop.wav", 0x0105d000, 0x00103efe, CRC(0f530c57) SHA1(d99ed4a9e065c67d313465b7cfb0687de64a7287))
    ROM_LOAD("BG_PillarMultiball_Intro.wav", 0x01161000, 0x000280d6, CRC(ba846720) SHA1(9c0669e8db1f1acc6e296a6ffc799998f9314dc7))
    ROM_LOAD("BG_PillarMultiball_loop.wav", 0x0118a000, 0x00169d28, CRC(555490cc) SHA1(a16454025d93454525aac71c7bd9f7fc8bc57201))
    ROM_LOAD("BG_Sam.wav", 0x012f4000, 0x0016aa48, CRC(b491cdae) SHA1(4f9c66335a83606270692ef00a8f5bcaa122d60d))
    ROM_LOAD("BG_Shockwave_Intro.wav", 0x0145f000, 0x00029f1c, CRC(115cfdbe) SHA1(43c668770442f46bd426c558e79b6b49e842d8a5))
    ROM_LOAD("BG_Shockwave_loop.wav", 0x01489000, 0x000d204e, CRC(daa94981) SHA1(f1d3cf8a300a0513286b27598e0a3c996f648124))
    ROM_LOAD("BG_Starscream_Intro.wav", 0x0155c000, 0x0005ccb8, CRC(b0679b3f) SHA1(f3452aaa9fa4f1578ddadea7bb0b42e5b7b20d3b))
    ROM_LOAD("BG_Starscream_loop.wav", 0x015b9000, 0x000fce2e, CRC(f08fa138) SHA1(f2eede6e1f5b21388e3c0607461335f77bb975e0))
    ROM_LOAD("BG_TCT_Heavy_intro.wav", 0x016b6000, 0x00013164, CRC(956184a6) SHA1(343de90a0d25c27c6e32fdfd7366713b44d0c4cb))
    ROM_LOAD("BG_TCT_Heavy_loop.wav", 0x016ca000, 0x0015a72c, CRC(6632f5ee) SHA1(7059b70c8694c0e4995de72223c7f9818f168347))
    ROM_LOAD("FX_AllsparkLites.wav", 0x01825000, 0x0002f108, CRC(d66be657) SHA1(c45d4eba880e4fb11c66e90efbe5d2d269745129))
    ROM_LOAD("FX_AllSparkMysteryAward.wav", 0x01855000, 0x00030c12, CRC(9f433359) SHA1(3a30a4ffcd181617f95164adb3ed3881339dfb53))
    ROM_LOAD("FX_AllSparkMysteryAward2_Locked.wav", 0x01886000, 0x00022926, CRC(01fc45ab) SHA1(9e3af1eb2df02ca49801b00a8019ebc5ce91d2e0))
    ROM_LOAD("FX_AllSparkMysteryAward2_Rise.wav", 0x018a9000, 0x0001a29c, CRC(e53b0e7f) SHA1(c9d7f0be138326ee6241ca985d862a20a6b48e1a))
    ROM_LOAD("FX_AutoBotFires_01.wav", 0x018c4000, 0x0001b308, CRC(9cbd2e9f) SHA1(f23b839c98bcd9fe43a6e9659b7de23612c1c3ec))
    ROM_LOAD("FX_AutoBotLitLane.wav", 0x018e0000, 0x0000fa26, CRC(48c7ddc7) SHA1(0012d4c6419eb1360459ffa3dbf53e4b409f3c3f))
    ROM_LOAD("FX_AutoTalk_01.wav", 0x018f0000, 0x00017704, CRC(928fda25) SHA1(016576d56229f82a2e277a8f5878544a1372b01c))
    ROM_LOAD("FX_AutoTalk_02.wav", 0x01908000, 0x0000b586, CRC(9d3571e2) SHA1(a2a073f520426e1c7e11076996be03dcee32b62d))
    ROM_LOAD("FX_AutoTalk_03_FX001.wav", 0x01914000, 0x000063e0, CRC(b460234e) SHA1(ede3cf19a997fe3ac4bdcad045b5d770484e9dc3))
    ROM_LOAD("FX_AutoTalk_03_FX002.wav", 0x0191b000, 0x000076ca, CRC(c779b540) SHA1(673334ea696a4138311e1b85edd5a5f02168a790))
    ROM_LOAD("FX_Bang01.wav", 0x01923000, 0x00018108, CRC(1bc4b26a) SHA1(6a74a83948e040e38f74809be3b359b75b50a096))
    ROM_LOAD("FX_BeamWeapon_01.wav", 0x0193c000, 0x0001346c, CRC(e3ec2d78) SHA1(ca0df4902169e2cc319e065d857434cbcc9879f3))
    ROM_LOAD("FX_Bung_01.wav", 0x01950000, 0x0001307e, CRC(49fce7c5) SHA1(c7bc08813fdb232647e2e5e268235ccafdf4e031))
    ROM_LOAD("FX_CarRevs_FX001.wav", 0x01964000, 0x0000d228, CRC(dffb4b49) SHA1(a8f0470370939c39323c9ba01ae8a50e3ec108ec))
    ROM_LOAD("FX_CarRevs_FX002.wav", 0x01972000, 0x0000cb08, CRC(bf37df1b) SHA1(a3977f567923e1a2ffc5fb36852ff4a179172981))
    ROM_LOAD("FX_CarRevs_FX003.wav", 0x0197f000, 0x0001e8c6, CRC(803d5924) SHA1(f48d2afa521529b08d8bffcfa43cfcaef985ecf2))
    ROM_LOAD("FX_CarSkidsFX001.wav", 0x0199e000, 0x00022e0c, CRC(7629f7dd) SHA1(c63c65a2971b6598388f9941e15cd45561741868))
    ROM_LOAD("FX_CarSkidsFX002.wav", 0x019c1000, 0x000134e0, CRC(33f07b30) SHA1(f370cf03c32b77cf20357e41d311222ae3505dcb))
    ROM_LOAD("FX_CarSkidsFX003.wav", 0x019d5000, 0x00039c66, CRC(13caa210) SHA1(a3b3dd4cfbf96b114fe2ddd9230960e9571042c9))
    ROM_LOAD("FX_CarSkidsFX004.wav", 0x01a0f000, 0x0002f004, CRC(07fd25ac) SHA1(f1b60f12e72e3c242ad2f563a2b291b4d538833c))
    ROM_LOAD("FX_CleanHit_FX001.wav", 0x01a3f000, 0x00009aec, CRC(e797a04c) SHA1(d3719415c728d2ba09aa1ef1a71ea20c2d6982f6))
    ROM_LOAD("FX_CleanHit_FX002.wav", 0x01a49000, 0x00009966, CRC(09779ad8) SHA1(50fcab216011e69726a5018b96e98df18a026707))
    ROM_LOAD("FX_CleanHit_FX003.wav", 0x01a53000, 0x00009966, CRC(59f483cd) SHA1(49bf8c60e0c7c7227a2d2b5d428931d723fcec82))
    ROM_LOAD("FX_CleanHit_FX004.wav", 0x01a5d000, 0x00009966, CRC(8a5ef724) SHA1(8918a2d8fd4eebdcdbae6e22966c2ffb950fb619))
    ROM_LOAD("FX_DecepPops_02.wav", 0x01a67000, 0x00015d3a, CRC(96c45979) SHA1(13928622eb39e26191d814606349c0932e2882d6))
    ROM_LOAD("FX_DecepPops_03.wav", 0x01a7d000, 0x00016b0a, CRC(3dd3a9b5) SHA1(ee20fda90f17335d2b76798003783034eaab8191))
    ROM_LOAD("FX_DecepThing.wav", 0x01a94000, 0x00017154, CRC(ddfa434d) SHA1(d142f2acbb082c64f4889b63e5b03275574b871a))
    ROM_LOAD("FX_Diesel01.wav", 0x01aac000, 0x0002b490, CRC(bbf46d01) SHA1(31deffbd68be20086a0dd15c3f777520c1699fd1))
    ROM_LOAD("FX_Energon_FX001.wav", 0x01ad8000, 0x0000e766, CRC(5194210f) SHA1(b598ac6ed3b7ae6c25cf8c5f7895b88ca24d7e5f))
    ROM_LOAD("FX_Energon_FX002.wav", 0x01ae7000, 0x0000d4ae, CRC(ae45b77c) SHA1(a12bc2b56c297516a8426fd941d586e84012be47))
    ROM_LOAD("FX_EnergyBeam_01.wav", 0x01af5000, 0x00019d44, CRC(b61b9006) SHA1(1a30eec239b9be997820abac50f5a30976898f93))
    ROM_LOAD("FX_EnergyBeam_02.wav", 0x01b0f000, 0x00018814, CRC(15427c47) SHA1(921aefa9a2c1df227ae50dd689f76171ab68cb48))
    ROM_LOAD("FX_EnergyBeam_03.wav", 0x01b28000, 0x0001901a, CRC(c26352a2) SHA1(3391e433772a5c82d2614863776a49ba420b37f7))
    ROM_LOAD("FX_EnergyShot_01.wav", 0x01b42000, 0x0001ed70, CRC(3bc671d1) SHA1(ac514bfe420a2b368480768e74c43ef1f21396ec))
    ROM_LOAD("FX_EnergyShot_02.wav", 0x01b61000, 0x0001960a, CRC(03f1f707) SHA1(1363c4780be4a122009e7cc7d71eb73ac0722c56))
    ROM_LOAD("FX_EnergyShot_03.wav", 0x01b7b000, 0x0001f58a, CRC(aadce90e) SHA1(58817fb61eaa202f6cdd8248ad17ba9d2120a598))
    ROM_LOAD("FX_FragmentedRiseFlanged2_SweptDown.wav", 0x01b9b000, 0x00023754, CRC(1d04ee64) SHA1(754e3b59f02cf7b78194bc6f931662fbd0a30846))
    ROM_LOAD("FX_GoodAward_01_FX001.wav", 0x01bbf000, 0x000104c8, CRC(9b73ff1c) SHA1(72f624e23e11459c7c70773c9db03be8cb95e031))
    ROM_LOAD("FX_GoodAward_01_FX002.wav", 0x01bd0000, 0x000104c8, CRC(8f1d4dde) SHA1(87f3a279294d6b680867534b70ad1c01ee5547bd))
    ROM_LOAD("FX_GoodThing_02_FX001.wav", 0x01be1000, 0x00010ad4, CRC(7ad03e17) SHA1(852a070af0c9de77e11c8fcea1cd52db1e0a4679))
    ROM_LOAD("FX_GoodThing_02_FX003.wav", 0x01bf2000, 0x00010842, CRC(87d5295b) SHA1(260df9917d471a7e1700c7d89a62eefd4bc1b541))
    ROM_LOAD("FX_HitDecepThing_FX001.wav", 0x01c03000, 0x0000bf0c, CRC(c9dd2c91) SHA1(cb0400b7cf6cf43e0222af34b558de2cb2217564))
    ROM_LOAD("FX_HitDecepThing_FX002.wav", 0x01c0f000, 0x0000d376, CRC(38a29278) SHA1(832d1d146f6d93aa9fd6d6c52d86a2e136ac6457))
    ROM_LOAD("FX_HitDecepThing_FX003.wav", 0x01c1d000, 0x0000b916, CRC(9f11a7b8) SHA1(df78b7eb597543a6d7e56be16983b91c6c0ac431))
    ROM_LOAD("FX_HitLow_FX001.wav", 0x01c29000, 0x00011e42, CRC(1da41783) SHA1(ef54711a70e8fc3e8af1f59ebf940419977f948e))
    ROM_LOAD("FX_HitLow_FX002.wav", 0x01c3b000, 0x000105fc, CRC(794954bc) SHA1(5ecae609b7ac34d058434f77f43aa893550f11c3))
    ROM_LOAD("FX_HitLow_FX003.wav", 0x01c4c000, 0x0000f432, CRC(6a54b178) SHA1(9e046644388aa1eeee4d7cc6e612264752bf6729))
    ROM_LOAD("FX_HitLow_FX004.wav", 0x01c5c000, 0x0001247c, CRC(1c7d2dc5) SHA1(6033e51349fdc844449816128fdbba5cf44665fb))
    ROM_LOAD("FX_HitSomethingGood_01.wav", 0x01c6f000, 0x0000f638, CRC(05df45c4) SHA1(570993f4fe09d929efdf24f15732628975fdee8b))
    ROM_LOAD("FX_ImpactMetal_FX001.wav", 0x01c7f000, 0x0000e936, CRC(a549a333) SHA1(17ac583ac416fc8c8f85502a52c45c2374b22491))
    ROM_LOAD("FX_ImpactMetal_FX002.wav", 0x01c8e000, 0x0000e506, CRC(a1ba1b08) SHA1(ad3acf60a5225f6f2cb78bf660e16d9f5424da8a))
    ROM_LOAD("FX_ImpactMetal_FX003.wav", 0x01c9d000, 0x0000e50e, CRC(b5881c61) SHA1(8e0d62ffd71be146a403bc9cbef011f6e3d2bec3))
    ROM_LOAD("FX_ImpactMetal_FX004.wav", 0x01cac000, 0x0000e50e, CRC(69a86da0) SHA1(c969cc4a8c0d21221ea5aa7146174d37b1b27cd6))
    ROM_LOAD("FX_LeftEjectWindUp_01.wav", 0x01cbb000, 0x00022efc, CRC(83972a28) SHA1(36ea48c3f470c4940e637f580d59277b07aa556c))
    ROM_LOAD("FX_LookingForTheAllSpark.wav", 0x01cde000, 0x00015c78, CRC(24a29ee6) SHA1(4cc3add63439a8db177a2549d299980d34d96618))
    ROM_LOAD("FX_MagicFlyIn_01.wav", 0x01cf4000, 0x000253d8, CRC(dbfe8225) SHA1(9179507a74e94b5633104967073353c4231e8401))
    ROM_LOAD("FX_MegaAward_01.wav", 0x01d1a000, 0x00023eee, CRC(10df610a) SHA1(83789e41503b9bcc8f379d9dc1559972df21f009))
    ROM_LOAD("FX_MegatronFires_01_FX001.wav", 0x01d3e000, 0x00022724, CRC(a68e7b4f) SHA1(5d4d4246a7d867b3cea4b3f57a4b14ac1db3c9ba))
    ROM_LOAD("FX_MegatronFires_01_FX002.wav", 0x01d61000, 0x00022724, CRC(07bbf92b) SHA1(7721c3fd7e4f9ce62905ff4515d725a4145fed54))
    ROM_LOAD("FX_MegatronFires_01_FX003.wav", 0x01d84000, 0x00022724, CRC(8e3fc9df) SHA1(969afc421e3aaef1cfe64207e48bcadcebb4ebf3))
    ROM_LOAD("FX_MegatronLock_FX001.wav", 0x01da7000, 0x000166e6, CRC(fe2346c8) SHA1(9212bb8026aa33ea536e82cdf7f56ba484e91c6a))
    ROM_LOAD("FX_MegatronLock_FX002.wav", 0x01dbe000, 0x00017ef2, CRC(11397ede) SHA1(9b153436a96e3d7d4c6ff5753dd8daf20c842640))
    ROM_LOAD("FX_MegatronLock_FX003.wav", 0x01dd6000, 0x0001f49c, CRC(1e90de5c) SHA1(5ad759a0ec7db4d4d8ec37a39d72968082509417))
    ROM_LOAD("FX_ModeStartDevastator_01a.wav", 0x01df6000, 0x000279ce, CRC(160426e3) SHA1(9694101484b5532d17e27f2db36dbd8f92aff516))
    ROM_LOAD("FX_ModeStartDevastator_01b.wav", 0x01e1e000, 0x00022a90, CRC(78598d65) SHA1(d28aa58543c0bc72d50d1aa1cc24b357676873f5))
    ROM_LOAD("FX_ModeStartDevastator_01c.wav", 0x01e41000, 0x0001d9ee, CRC(c8d8b155) SHA1(45de0f98c81abd27c119d2bc85410422293e182d))
    ROM_LOAD("FX_ModeStartDevastator_01d.wav", 0x01e5f000, 0x0002ef48, CRC(08065027) SHA1(104705d6c904237938a3babed812fa6cf5092fb5))
    ROM_LOAD("FX_ModeStartDevastator_01e.wav", 0x01e8e000, 0x0002eee0, CRC(e1e3d3d4) SHA1(56944f802273d822055892ce7e4cf630ae8e5432))
    ROM_LOAD("FX_ModeStartMandS.wav", 0x01ebd000, 0x0002bf5a, CRC(4ea65a46) SHA1(e8b5fa2e4f15cfe4f5c33f6997a9ee809b4cefe5))
    ROM_LOAD("FX_ModeStartRatchet_01.wav", 0x01ee9000, 0x0003299a, CRC(fcd67b69) SHA1(1148fdc9f1a0a4262918ecc3002b7560fa7f8ff3))
    ROM_LOAD("FX_ModeStartRatchet_02.wav", 0x01f1c000, 0x0002919a, CRC(77af4ea1) SHA1(5930bbcf936241a9c528296dbae9516d7da35983))
    ROM_LOAD("FX_Modifier_01.wav", 0x01f46000, 0x0000fa5e, CRC(4a7a1f57) SHA1(d4e089934bf97675381bd28631f5393d10c41827))
    ROM_LOAD("FX_Modifier_02.wav", 0x01f56000, 0x0000f8ea, CRC(72dce911) SHA1(066fd10082a6b069870bf3563c82e5e95eda20e5))
    ROM_LOAD("FX_MoreThanMeetsTheEye.wav", 0x01f66000, 0x000289c6, CRC(8007f4cf) SHA1(d53feae08129e93b852875e425c3f95de49a0be7))
    ROM_LOAD("FX_Null_02.wav", 0x01f8f000, 0x000129fc, CRC(42d581ef) SHA1(148a822965e91ebcf86440cfc6a4b8a690e8f010))
    ROM_LOAD("FX_OptimusOrbit.wav", 0x01fa2000, 0x00033b9a, CRC(5886349b) SHA1(71e2452a2b5fb71f3bfe2b833d9cfc7f615aeaad))
    ROM_LOAD("FX_OrbitScore_FX002.wav", 0x01fd6000, 0x00018662, CRC(56610077) SHA1(6e75edee4827b195728afb212a0f0c51352289ae))
    ROM_LOAD("FX_Plunge_01.wav", 0x01fef000, 0x000271da, CRC(479354ea) SHA1(ed45fbce858bdbef64df3b1034c8c33c9885f8ec))
    ROM_LOAD("FX_Plunge_02.wav", 0x02017000, 0x000252d6, CRC(200a896e) SHA1(1db67c84bb0272aa12383610daa3312a2b1c716c))
    ROM_LOAD("FX_RampEntry_01_FX001.wav", 0x0203d000, 0x00014c28, CRC(49681a5a) SHA1(0e43dcb4d6ac6f4119284033b7e256cf6dae69ee))
    ROM_LOAD("FX_RampEntry_01_FX002.wav", 0x02052000, 0x000156a4, CRC(cfe239f1) SHA1(ec35c2ab05bca003cc84e98771376c393a6fd40a))
    ROM_LOAD("FX_Spinner_01_FX001.wav", 0x02068000, 0x00014b24, CRC(e8455417) SHA1(c2e047684f95c9a0da9ea52adb814d86613b083f))
    ROM_LOAD("FX_Spinner_01_FX004.wav", 0x0207d000, 0x000133ee, CRC(8f420a3e) SHA1(ad2dc72a7be128433d8dfdeaa4d76c0d1f3c561a))
    ROM_LOAD("FX_StingerAutoAward02.wav", 0x02091000, 0x000234be, CRC(f61cb6a6) SHA1(54884e51309de77b4b9a433947115fe98dc58cb3))
    ROM_LOAD("FX_StingerDecepticon_01_FX001.wav", 0x020b5000, 0x00030856, CRC(4beb5dc1) SHA1(f4e40b7d267354e58ee284abad80b69954cbe066))
    ROM_LOAD("FX_StingerDecepticon_01_FX002.wav", 0x020e6000, 0x00030856, CRC(2d0964a8) SHA1(daf5e15320d90f760d217e46aa556c34dd91cb80))
    ROM_LOAD("FX_StingerDecepticon_01_FX003.wav", 0x02117000, 0x0002f11c, CRC(5c6f8301) SHA1(13aee84558e5a60d7e388dcf093bdd5743b9d182))
    ROM_LOAD("FX_StingerScore_01.wav", 0x02147000, 0x0001c66e, CRC(0fd296ab) SHA1(438d83b2625eec8d45ec79b511ef24b95d788274))
    ROM_LOAD("FX_StingerScore_a_01.wav", 0x02164000, 0x0001ab0c, CRC(7866be28) SHA1(cd97f28ea6c51583ef0d5d53f888bd2d02d9b388))
    ROM_LOAD("FX_StingerScore_B_01.wav", 0x0217f000, 0x0002b774, CRC(7958681e) SHA1(84a497ae40a0a7b804b882c548425e40c401fed1))
    ROM_LOAD("FX_StingerScore_Bb_01.wav", 0x021ab000, 0x0001aa4e, CRC(431795c1) SHA1(17c48f1b9eb7bbb110dc9e36f4c7a49e522ac8b8))
    ROM_LOAD("FX_StingerScore_C_01.wav", 0x021c6000, 0x0002b980, CRC(15fc30a9) SHA1(366e088fb9b804722a56b5531b8826e6ef5fed34))
    ROM_LOAD("FX_StingerScore_Db_01.wav", 0x021f2000, 0x0002b96a, CRC(0da31496) SHA1(58fea4652afefad1b22fb14ad7c430ef7f7393a2))
    ROM_LOAD("FX_SuperSkillShot.wav", 0x0221e000, 0x0001dd96, CRC(48909b97) SHA1(0196208c568b771437b71e6d8a2d45f0c8211113))
    ROM_LOAD("FX_SynthFlyBy.wav", 0x0223c000, 0x0002d4d0, CRC(1b2ccfeb) SHA1(797d5c8deb9c82b5a4f5a36dc88454f91a457ba1))
    ROM_LOAD("FX_T1_AutobotEMIPulse.wav", 0x0226a000, 0x00013a48, CRC(c7e7e9d4) SHA1(a1f7f4a9fc7645b49792df1cfafb33ad6e86dac2))
    ROM_LOAD("FX_T1_BlackOutLands.wav", 0x0227e000, 0x0002741c, CRC(b93dc17c) SHA1(cb9255ef0b05519f70adaf51c0d01a1fb809f46d))
    ROM_LOAD("FX_T1_DecepticonHelicopterFlyover.wav", 0x022a6000, 0x0003f050, CRC(d317a338) SHA1(a6e8bf1da338cf406685845f3227b64552cb7709))
    ROM_LOAD("FX_T1_DecepticonLanesComplete.wav", 0x022e6000, 0x0000f29a, CRC(467c4a6e) SHA1(83a38a3063e9ffb5ff6e469f72a50d489a27ac95))
    ROM_LOAD("FX_T1_Explo.wav", 0x022f6000, 0x0000df8e, CRC(09d2a519) SHA1(cf67492682f199294485dc03d0160df20dddf1a5))
    ROM_LOAD("FX_T1_ExplosionGunsClose.wav", 0x02304000, 0x00015094, CRC(9333251f) SHA1(ffb3d3f6977952ab97e0f9d169bc7f22d749d42e))
    ROM_LOAD("FX_T1_ExplosionGunsClose_02.wav", 0x0231a000, 0x0000ea5c, CRC(44271f64) SHA1(74d865c31b4525d855f3b820e51b29b33c0fa024))
    ROM_LOAD("FX_T1_ExplosionGunsClose_04.wav", 0x02329000, 0x000175ee, CRC(dbd5e64e) SHA1(47ed333d3de16b17c5ac3ee71297904fac095055))
    ROM_LOAD("FX_T1_MegatronShouts_01.wav", 0x02341000, 0x0000e368, CRC(f349b51b) SHA1(03abab9071e402b7d5c5c3d0db688529e6f6ede3))
    ROM_LOAD("FX_T1_MisslesHitMegatron.wav", 0x02350000, 0x00016b8c, CRC(7247744f) SHA1(9580546f5ca2a876d74a0a28bc22539e8f9a609b))
    ROM_LOAD("FX_T1_OptimusDeploysEnergonSword.wav", 0x02367000, 0x0000c4e8, CRC(dde57461) SHA1(de32446fe1b2bff030721a3252da5bc86a0929d8))
    ROM_LOAD("FX_T1_OptimusHitsBuilding.wav", 0x02374000, 0x000290c4, CRC(ada7be0b) SHA1(aa9cb5e3d7b22338f82aadc2958729bf8330528d))
    ROM_LOAD("FX_T1_OptimusSheathssEnergonSword.wav", 0x0239e000, 0x00009df2, CRC(70a5e475) SHA1(ab064af443fb49e833113255d857c60da13a179c))
    ROM_LOAD("FX_T1_OptimusTransforms_FX001.wav", 0x023a8000, 0x00021d54, CRC(0273d4f3) SHA1(705e94149f2992d64288c12a4556c7d6d2d4b170))
    ROM_LOAD("FX_T1_OptimusTransforms_FX002.wav", 0x023ca000, 0x0001b6e4, CRC(ec1ee8ae) SHA1(07e9e7b8c4f75abad30927e08255c3ced7f37d79))
    ROM_LOAD("FX_T1_OptimusTransforms_FX003.wav", 0x023e6000, 0x0001b69e, CRC(c6a5dd46) SHA1(074244228f2c1902a2469ee068501e1cd31b2682))
    ROM_LOAD("FX_T1_OptimusTransforms_FX008.wav", 0x02402000, 0x000122c8, CRC(07026eff) SHA1(75e852e31bcb0602edf5300a4227ac0e84393c1b))
    ROM_LOAD("FX_T1_OptimusTransforms_FX010.wav", 0x02415000, 0x00018f3e, CRC(cc314442) SHA1(c1d43bc3c483de6effcd13fa753338b35bc0bc9b))
    ROM_LOAD("FX_T1_OptimusTransforms_FX011.wav", 0x0242e000, 0x0001f83e, CRC(5ae47165) SHA1(9505766e3381813a3cee1952fee2eaeeb196b559))
    ROM_LOAD("FX_T1_OptimusTransforms_FX012.wav", 0x0244e000, 0x0001c318, CRC(10cb2d5a) SHA1(c3f3a7fc99058cc4a1f4a07f1ce2ca413fc2a32c))
    ROM_LOAD("FX_T1_StarscreamFlyBy_01.wav", 0x0246b000, 0x0001a26c, CRC(ee66db34) SHA1(8fee2972dfa58b5bc68923b7e4ab74f75d09b492))
    ROM_LOAD("FX_T1_StarscreamFlyBy_02.wav", 0x02486000, 0x00029252, CRC(46e6be76) SHA1(4caede27e86c03d48c42343836c21c747e85fef1))
    ROM_LOAD("FX_T1_StarscreamFlyOver.wav", 0x024b0000, 0x00015d24, CRC(3a3a2dfc) SHA1(7cd9dc81e107bbd62c634af0b848f0891b6a4261))
    ROM_LOAD("FX_T1_StarscreamFlyOver_02.wav", 0x024c6000, 0x00024f5a, CRC(e969a189) SHA1(83d64e29a23d46bda88fdf67e854911a0f149703))
    ROM_LOAD("FX_T1_StarscreamFlyOver_03.wav", 0x024eb000, 0x0002523e, CRC(dadbc4a2) SHA1(4a34be623c8207bf5ee1f0e55e758cfe20a93783))
    ROM_LOAD("FX_T1_WindingUpCubePower.wav", 0x02511000, 0x0001bef4, CRC(70c68eef) SHA1(f7aef7295c83d670bd695c9f48d516b51153c2c2))
    ROM_LOAD("FX_T2_OptimusLandsPunch_02.wav", 0x0252d000, 0x00014cfa, CRC(4bc001bd) SHA1(fc1abe18fbcb4b775ad1e1605209253f983591a6))
    ROM_LOAD("FX_T2_OptimusLandsPunch_07.wav", 0x02542000, 0x00013530, CRC(22af841d) SHA1(64a5eb13422296ebc1dd018e90f77ec62f3bc088))
    ROM_LOAD("FX_T2_OptimusLandsSloMoPunch_01.wav", 0x02556000, 0x000131fe, CRC(d41a1bc6) SHA1(965e355d1c9a1a07c17a637e25657c81eaaeed4e))
    ROM_LOAD("FX_T2_OptimusWacksGrindor_01.wav", 0x0256a000, 0x00017c3e, CRC(7d909730) SHA1(1c05194a7ea5bec5555b0b5b41bbe6b67fca9fe4))
    ROM_LOAD("FX_TCT_CommercialBumper_01.wav", 0x02582000, 0x00028fb8, CRC(4897c740) SHA1(b4b85ee34a8d09294176bc6757fa5f2f959a2b2f))
    ROM_LOAD("FX_TiltWarning_01.wav", 0x025ab000, 0x000157b8, CRC(c3187c2e) SHA1(d8f3ca533a551991e2600a7ea5cdc83a3c97fac2))
    ROM_LOAD("FX_UnlitDecepticonLane_01.wav", 0x025c1000, 0x0000bc60, CRC(13507fad) SHA1(80ef5fe76a77b56080f47025a7d549b390680d42))
    ROM_LOAD("FX_UnlitDecepticonLane_02.wav", 0x025cd000, 0x0000f4a0, CRC(f83bbfb5) SHA1(f03e16c4e5b85a702db3af034479b2827730f69e))
    ROM_LOAD("FX_UnlitDecepticonLane_03.wav", 0x025dd000, 0x00010c92, CRC(0fdea2c5) SHA1(4285f53202f1ba7995b7b7ac93f205c0a536b7bf))
    ROM_LOAD("FX_Warning_04b.wav", 0x025ee000, 0x00009cfc, CRC(d1864ca2) SHA1(441f456e2d78ab15f572a0ee397a571beada6b77))
    ROM_LOAD("FXT1SomeThingEnabled.wav", 0x025f8000, 0x00023806, CRC(9a657fc9) SHA1(a5d8e25a550662e28fa95661ddbc08cd37e8df43))
    ROM_LOAD("VO_IH_001a_It_s_gonna_take_more_than_you_rust_buckets_to_stop_me.wav", 0x0261c000, 0x00012d32, CRC(4af9444a) SHA1(fdb3e956750f61dc5f05f82f40957e4e4e8cd084))
    ROM_LOAD("VO_IH_002a_Your_days_are_numbered_now_decepi_creeps.wav", 0x0262f000, 0x00010ab2, CRC(640165f4) SHA1(aef45f139313cf9636054479ab38ae5b43d60739))
    ROM_LOAD("VO_IH_007a_Lets_blast_their_tail_rudders_out_of_there.wav", 0x02640000, 0x0000f82e, CRC(d24eb5c8) SHA1(8da5db41481de900fa1de8c36dfc0388ffcbe2b5))
    ROM_LOAD("VO_IH_012a_Say_your_prayers_Decepti_crumbs.wav", 0x02650000, 0x0000e332, CRC(c32e7329) SHA1(1451ec74285c946c7a8b9e2ed9b44690112231c8))
    ROM_LOAD("VO_IH_014a_You_Decepti_jerks_fell_lucky.wav", 0x0265f000, 0x0000f4b2, CRC(ea8dbc3e) SHA1(700787046b2176d4f0b4b7df6a305738a7f9c522))
    ROM_LOAD("VO_IH_016a_Decepticon_punk.wav", 0x0266f000, 0x00009364, CRC(adbaa25b) SHA1(4ca122d36d65b99c91e3eed9752a67465f1b6c30))
    ROM_LOAD("VO_IH_Let_s_bust_Decepti_chops_1_add_on.wav", 0x02679000, 0x0000c368, CRC(b6adf630) SHA1(59e758570a381853a7e2ac510d92aa4632fc6d80))
    ROM_LOAD("VO_IH_Your_Days_Are_Numbered_1_add_on.wav", 0x02686000, 0x000107b2, CRC(93a63cc9) SHA1(4755c4692c670fdc2b6fa7bdd089c0fdf891e854))
    ROM_LOAD("VO_M_015c_Feel_my_power.wav", 0x02697000, 0x0000c50c, CRC(eca58d29) SHA1(c6f20ea6c3fce453fead89ca7ce90397f1efaef5))
    ROM_LOAD("VO_M_037a_You_cannot_stop_me_no_one_stops_Megatron.wav", 0x026a4000, 0x00022132, CRC(38dfab2e) SHA1(9d281ffaf9c66f84751c45bb4f508d85a1b0bc47))
    ROM_LOAD("VO_M_056a_Optimus.wav", 0x026c7000, 0x0000cb32, CRC(8b837f52) SHA1(39cd2f4abad9692ee7dcfe5440c6dd68a2a7964a))
    ROM_LOAD("VO_M_076a_You_re_runnin_out_of_time.wav", 0x026d4000, 0x00011332, CRC(eb13870c) SHA1(2772e715696fa4fd2da012086abe6b0494eb0626))
    ROM_LOAD("VO_Mfx_001a_Wait_I_still_function.wav", 0x026e6000, 0x00015340, CRC(7c1c9510) SHA1(29f38bdb72ad79911158201a4e4f415ec2fb5ed5))
    ROM_LOAD("VO_Mfx_003a_Devastator.wav", 0x026fc000, 0x0000824a, CRC(d5fe14a3) SHA1(2a4b1eb913c5cb5ae3b2b4229c16f79ed701628b))
    ROM_LOAD("VO_Mfx_004a_Shockwave.wav", 0x02705000, 0x00008284, CRC(e6e752fd) SHA1(6d528c9de654aeb0de8ba146e172d1950def7f26))
    ROM_LOAD("VO_Mfx_005a_Megatron.wav", 0x0270e000, 0x00009f26, CRC(95fb455d) SHA1(bc5de7fc452c1f1cdf6d0d6efb1e578aefc2e898))
    ROM_LOAD("VO_Mfx_008a_Blackout.wav", 0x02718000, 0x00007ba0, CRC(171f7760) SHA1(77d64108fc8353f38cec81eb38f89ac22a6c37d6))
    ROM_LOAD("VO_Mfx_009a_Starscream.wav", 0x02720000, 0x0000aa80, CRC(ea916830) SHA1(56ad08c3d553bc91eb1793b2b47206853ebc2fc5))
    ROM_LOAD("VO_Mfx_013b_This_is_my_planet.wav", 0x0272b000, 0x0000ec20, CRC(da65d252) SHA1(e625bf516ab360f88c0432cdbf991ec261feddb0))
    ROM_LOAD("VO_Mfx_015c_Feel_my_power.wav", 0x0273a000, 0x0000bd40, CRC(863cc746) SHA1(bae42c42c46f6d95ebcc5d2c2c2382f8fbc2844f))
    ROM_LOAD("VO_Mfx_016b_Worthless.wav", 0x02746000, 0x00005f80, CRC(e9ef7c87) SHA1(81d50d6d962c7f0332882847c8630ddc7d89ac7b))
    ROM_LOAD("VO_Mfx_017b_Pathetic.wav", 0x0274c000, 0x00005f80, CRC(97d1367b) SHA1(53d36ec3057f73b02c2095a61090e691a673b172))
    ROM_LOAD("VO_Mfx_018b_Pitiful.wav", 0x02752000, 0x00005620, CRC(134f3bbe) SHA1(501eadd64a7f5f6b611cf8bac7cd944b8373bc68))
    ROM_LOAD("VO_Mfx_019a_you_re_so_weak.wav", 0x02758000, 0x0000d960, CRC(3354af6c) SHA1(2cdef3c5876e2a48b4b40afc60bcdbf590305f4a))
    ROM_LOAD("VO_Mfx_032a_Decepticons_Scramble.wav", 0x02766000, 0x0000ec20, CRC(3bd5a1ed) SHA1(b911ffbe13d4b445304d59371ea46cf96bb6990d))
    ROM_LOAD("VO_Mfx_033b_None_shall_stand_in_my_way.wav", 0x02775000, 0x00012dc0, CRC(d9b2fff4) SHA1(02dfc2b5128d84151327905a922b5a9182fc697c))
    ROM_LOAD("VO_Mfx_041a_We_are_indestructible.wav", 0x02788000, 0x0000fee0, CRC(46d77b30) SHA1(4630770e701ac1a8e6e3fd6f20fa755ccf34f864))
    ROM_LOAD("VO_Mfx_045a_I_bring_chaos.wav", 0x02798000, 0x0000ec20, CRC(e1f0da2e) SHA1(1a80623a271aab45b224c113fb6470248ec7fe62))
    ROM_LOAD("VO_Mfx_054a_I_ll_crush_you_with_my_bare_hands.wav", 0x027a7000, 0x00016f60, CRC(32bb66f3) SHA1(c0c6ac39a59127054603b71fab40a3bcd48b7fad))
    ROM_LOAD("VO_Mfx_085_Collected.wav", 0x027be000, 0x000056cc, CRC(673bf561) SHA1(7be19a97bd1ac243f2535f7bbaf07bb7ffa3c2ab))
    ROM_LOAD("VO_Mfx_085c_Collected.wav", 0x027c4000, 0x00005f00, CRC(1085e86e) SHA1(aae37652bb4ff553249c781dcc25b09c70c02f06))
    ROM_LOAD("VO_Mfx_Imbecile_added_line.wav", 0x027ca000, 0x00007ba0, CRC(6a19d1a5) SHA1(62cece20af54a4d6b42f57f3ddd747fb205c0e65))
    ROM_LOAD("VO_Mfx_Time_is_Running_Out_Prime_3_added_line.wav", 0x027d2000, 0x000111a0, CRC(f1119c8b) SHA1(a1d9c7dc91f08403fbae28c9c812a338bd6e6e06))
    ROM_LOAD("VO_OP_001a_Bumblebee.wav", 0x027e4000, 0x000065f4, CRC(5220ad71) SHA1(db273a84c9df102fa553eb4aa7bebe0bdb28cdb1))
    ROM_LOAD("VO_OP_001b_Bumblebee.wav", 0x027eb000, 0x00006a32, CRC(af362403) SHA1(d51568c14c63560813591dd41c3153e684884c46))
    ROM_LOAD("VO_OP_001c_Bumblebee.wav", 0x027f2000, 0x0000795a, CRC(9ffc87ae) SHA1(01bb673108fbd4e0b8ccf68fe6a9003c96a66233))
    ROM_LOAD("VO_OP_001d_Bumblebee.wav", 0x027fa000, 0x00007532, CRC(3e3cecfa) SHA1(e5ce35c7d31c28b2d52407d6018140e61deca3f0))
    ROM_LOAD("VO_OP_003a_collected.wav", 0x02802000, 0x00007932, CRC(16fbf082) SHA1(3ce2cfb97d8542721eb0dafe3fd831b068cf3baa))
    ROM_LOAD("VO_OP_004a_Optimus_Prime.wav", 0x0280a000, 0x00009df2, CRC(c6f9b8e3) SHA1(9aeea59f02aa080708db26ad05cb86019dd3cecf))
    ROM_LOAD("VO_OP_005a_Ironhide.wav", 0x02814000, 0x000070d6, CRC(027be7b6) SHA1(61290df02402c4b03c038040192c12a0c624cdef))
    ROM_LOAD("VO_OP_006a_Mudflap_and_Skids.wav", 0x0281c000, 0x0000c5b2, CRC(669165bc) SHA1(18705e6b9b998fd8e8db74373db60064fa740c0f))
    ROM_LOAD("VO_OP_007a_Ratchet.wav", 0x02829000, 0x0000562c, CRC(85e58fc5) SHA1(94e1a85d39c528a27d912a78fa57828998cb8071))
    ROM_LOAD("VO_OP_042a_Multiball.wav", 0x0282f000, 0x00008e8a, CRC(15fb3bd1) SHA1(15282ee6edf1ac0e6cfdc5d320218fa307289eb2))
    ROM_LOAD("VO_OP_042b_Multiball.wav", 0x02838000, 0x000080f0, CRC(825c02b6) SHA1(e031410a8eece634628826618131f95bb47b67dd))
    ROM_LOAD("VO_OP_042c_Multiball.wav", 0x02841000, 0x00007ec0, CRC(120b4fcf) SHA1(79ecfd1a6e1b87d5b071624b069d464db58c4868))
    ROM_LOAD("VO_OP_048b_Field_is_hot_Move_move.wav", 0x02849000, 0x0000eaf4, CRC(f013bdd2) SHA1(5115c101a5770dd620a11b1df17b0cfbeb3dc7f6))
    ROM_LOAD("VO_OP_050a_It_is_time_for_battle.wav", 0x02858000, 0x0000c668, CRC(8a0013c5) SHA1(fd8529991e87056447ccd3b370e2e0e7722fbc15))
    ROM_LOAD("VO_OP_056a_Keep_it_steady.wav", 0x02865000, 0x00006b92, CRC(02e00194) SHA1(f110afffee6ef568e267e0b72cb633bf55c6bda2))
    ROM_LOAD("VO_OP_088a_Super_Jackpot.wav", 0x0286c000, 0x00008154, CRC(ea88f3ac) SHA1(f340561123671e6efe42993b5e874438dc336227))
    ROM_LOAD("VO_OP_Optimus_Prime_Multiball_added_line.wav", 0x02875000, 0x0000f410, CRC(a220921d) SHA1(93ebff0cf8b7cfd4df1450dfa478b73b1672c363))
    ROM_LOAD("VO_SS_001a_AHHHH_you_flesh_creatures_will_pay_for_that.wav", 0x02885000, 0x00019a32, CRC(20b87e6d) SHA1(c09b77471f3ff761f0a0b057388f9965a0184dfa))
    ROM_LOAD("VO_SS_002a_Pathetic_fools_there_s_no_escape.wav", 0x0289f000, 0x0001b632, CRC(97c382e5) SHA1(4b283992bfecf5fcab348dc7bb68c3aae7303c8d))
    ROM_LOAD("VO_SS_Decepticons_Attack_added_line.wav", 0x028bb000, 0x0000c672, CRC(08a0b751) SHA1(2c14c3628ec50b80c8329a9ab995468b323b3c07))
    ROM_LOAD("VO_SS_You_re_No_Match_for_Lord_Megatron_added_line.wav", 0x028c8000, 0x00015d32, CRC(854fc2bb) SHA1(8a17a2b84f47c8a19b3ac3fb89ddaa65e17d69bd))
    ROM_LOAD("VO_T1_00054_OP_Before_time_began_there_was_the_Cube.wav", 0x028de000, 0x00019632, CRC(e40c6909) SHA1(01d791c56c6b89aee508c5acaa2ea7a6d9f886a0))
    ROM_LOAD("VO_T1_00059_OP_001_WeKnowNotWhereItComesFrom.wav", 0x028f8000, 0x0000b84a, CRC(e9c62e67) SHA1(4c10345743ffa6499defcea5732fc178f2c5e265))
    ROM_LOAD("VO_T1_00059_OP_002_OnlyThatItHoldsThePowerToCreateWorlds.wav", 0x02904000, 0x0001595c, CRC(d4dca73a) SHA1(d0b7f8cf328c794b05720e7d19db1ccd57a32969))
    ROM_LOAD("VO_T1_00059_OP_003_AndFillThem.wav", 0x0291a000, 0x00007e12, CRC(3f2f105a) SHA1(2d8dcb4b37a972036ff852192bc7cbb3174b6b79))
    ROM_LOAD("VO_T1_00059_OP_004_WithLife.wav", 0x02922000, 0x00004b28, CRC(9fae996e) SHA1(56e2fcddf925bb75b4d74349d2cfe98f1265c3fa))
    ROM_LOAD("VO_T1_00111_OP_001_ForATimeWeLivedInHarmony.wav", 0x02927000, 0x0000c9ca, CRC(4de832a4) SHA1(fec4d982a65f12bdde3b876e710787b205f3e613))
    ROM_LOAD("VO_T1_00111_OP_002_ButLikeAllGreatPower.wav", 0x02934000, 0x0000b12e, CRC(fe663028) SHA1(f75b8728ab6e5f06242921d163dc62748c389886))
    ROM_LOAD("VO_T1_00111_OP_003_SomeWantedItForGood.wav", 0x02940000, 0x0000bef0, CRC(b0188d8d) SHA1(6c468f7ffa88478c9d4587affcd7c59d9a018dfa))
    ROM_LOAD("VO_T1_00111_OP_004_OthersForEvil.wav", 0x0294c000, 0x0000913c, CRC(275f3ceb) SHA1(0417545e5c60ebf65de496dec30d4d5584e4e096))
    ROM_LOAD("VO_T1_00121_OP_And_so_began_001_AndSoBeganTheWar.wav", 0x02956000, 0x0000ebe8, CRC(6437d156) SHA1(44f289806bdfa5fcebb3c54f9071f45ab8856c0c))
    ROM_LOAD("VO_T1_00121_OP_And_so_began_002_AWarThatRavagedOurPlanet.wav", 0x02965000, 0x0001d0e8, CRC(f0cc42f6) SHA1(4a411833df8c42fd285270ddb5f84e9b0b20ba4f))
    ROM_LOAD("VO_T1_00121_OP_And_so_began_003_AndTheCube.wav", 0x02983000, 0x00007534, CRC(4c77cec8) SHA1(de1636068cb43c00e9ec1249df22b7710e0f1a4b))
    ROM_LOAD("VO_T1_00121_OP_And_so_began_004_WasLostToTheFarReachesOfSpace.wav", 0x0298b000, 0x0000fb7c, CRC(f575f4fd) SHA1(074b17e7b64589f0f364f2dbf40f0df70ab76c83))
    ROM_LOAD("VO_T1_00134_OP_001_weScatteredAcrossTheGalaxy.wav", 0x0299b000, 0x0000d54c, CRC(499f8e1b) SHA1(3c0dbc62560f0a422a5f46ce756bd0b0336ae72c))
    ROM_LOAD("VO_T1_00134_OP_002_HopingToFindIt.wav", 0x029a9000, 0x000080ca, CRC(1e7dd828) SHA1(fe7951ba02ea428ef606686d35ef7ba31f51c76e))
    ROM_LOAD("VO_T1_00134_OP_003_AndRebuildOurHome.wav", 0x029b2000, 0x00009658, CRC(ff5eb538) SHA1(dbe6a7f1057de64682a79fbd0662e32ebefe3b9e))
    ROM_LOAD("VO_T1_00134_OP_004_SearchingEveryStar.wav", 0x029bc000, 0x0000b18a, CRC(a75aa223) SHA1(fed395b2561b307b7eda154cd80a070a21b21364))
    ROM_LOAD("VO_T1_00134_OP_005_EveryWorld.wav", 0x029c8000, 0x00006f84, CRC(83ac252f) SHA1(b6a77a64792207053c5eab535257f13a839690e6))
    ROM_LOAD("VO_T1_00146_OP_001_andJustWhenAllHopeSeemedLost.wav", 0x029cf000, 0x0000fb08, CRC(7e0a475d) SHA1(1df748802a2c24b6cea7af9ea10eb5c315ae2883))
    ROM_LOAD("VO_T1_00146_OP_002_MessageOfANewDiscovery.wav", 0x029df000, 0x0000b02a, CRC(d115b0a0) SHA1(8a40c8a2b1f40a018214ea848f04201ad749c1cf))
    ROM_LOAD("VO_T1_00146_OP_003_DrewUsToAnUnknownPlanetCalled.wav", 0x029eb000, 0x00010eb0, CRC(8eee0da6) SHA1(54187f3727755fcbfbd566eafb503abd6a99c26c))
    ROM_LOAD("VO_T1_00146_OP_004_Earth.wav", 0x029fc000, 0x0000433a, CRC(81b0a5f6) SHA1(3f3de8ebbda988c6206e81c25a9f047e59f78e63))
    ROM_LOAD("VO_T1_00203_OP_But_we_were_already_too_late.wav", 0x02a01000, 0x0000d248, CRC(f5b94b39) SHA1(c541553bd94a81a5c18e61c6929b24e50177bd78))
    ROM_LOAD("VO_T1_10643_MEG_I_am_Megatron.wav", 0x02a0f000, 0x0001295a, CRC(11af8828) SHA1(4e9c51abb2ea107bc8b5a5c469014e96d40682da))
    ROM_LOAD("VO_T1_12651_MEG_Oh_so_unwise.wav", 0x02a22000, 0x000109ca, CRC(c422c9d3) SHA1(2757968cf69c0c409be369f717ca978b48fadece))
    ROM_LOAD("VO_T2_00631_MUD_This_is_combat_man.wav", 0x02a33000, 0x00006056, CRC(687e2eda) SHA1(3ff0aeb0f9a4a9ccb7748f5996463dc74bb3319b))
    ROM_LOAD("VO_T2_06352_MEG_Its_time_for_the_world_to_know_of_our_presence.wav", 0x02a3a000, 0x0000fba4, CRC(9d56ab05) SHA1(7da9374033972fb9113c19dce3b9846300145aa0))
    ROM_LOAD("VO_T2_06356_MEG_No_more_disguises_No_mercy.wav", 0x02a4a000, 0x0001295c, CRC(ea4fc443) SHA1(0ffc8c48961af8a9c1b83a4bfc9d558770c6af05))
    ROM_LOAD("VO_T2_10227_MUD_Now_thats_rude.wav", 0x02a5d000, 0x00004d7c, CRC(365d7234) SHA1(9fa862f3e4675c6da81ccf017ab937aa06cba863))
    ROM_LOAD("VO_T2_11754_MUD_You_aint_getting_me.wav", 0x02a62000, 0x0000bf3c, CRC(9ebe32bc) SHA1(5145d9f5cd6df99b384ee39e010e97ada6e7b58a))
    ROM_LOAD("VO_T2_11844_MUD_Nobody_messes_with_the_twins.wav", 0x02a6e000, 0x0000872e, CRC(0a3580e7) SHA1(c133946c332c12f6bb33e7f403f5fe78d9b15f15))
    ROM_LOAD("VO_T3_opt_get_down_here_sentinel.wav", 0x02a77000, 0x0000c5ea, CRC(d06285ee) SHA1(25f40870009b6563ea863c9626539a635dc8415a))
    ROM_LOAD("VOfx_d_LookingForTheAllSpark.wav", 0x02a84000, 0x000137de, CRC(83f4e78b) SHA1(edf54fdb044d7bd9e1176687a3c894d290581e8f))
    ROM_LOAD("VOfx_E_LookingForTheAllSpark.wav", 0x02a98000, 0x000138f6, CRC(f138b24b) SHA1(6ba15e83875f7b6bc1c7f443e858d286aa36ef69))
    ROM_LOAD("VOfx_OP_010a_Extra_ball.wav", 0x02aac000, 0x00008e06, CRC(a2778def) SHA1(e2d0fdbbc4ec7181f82a9ba91de6cb73852eb63b))
    ROM_LOAD("VOfx_OP_013a_Autobots_roll_out.wav", 0x02ab5000, 0x0000c4aa, CRC(b285b106) SHA1(b2b08664726ca9a9b9b397ad9b5c114aec721386))
    ROM_LOAD("VOfx_OP_017a_Player_2.wav", 0x02ac2000, 0x000074d6, CRC(7b831323) SHA1(bcfa92a4e342b012c6d76bfff05cd63bc066a94d))
    ROM_LOAD("VOfx_OP_020a_Megatron_you_re_a_bucket_of_rust.wav", 0x02aca000, 0x00011cea, CRC(01c9eb2e) SHA1(98f153b00eadd8e8a234b2ac3e15979d5d518a90))
    ROM_LOAD("VOfx_OP_024a_Stick_it_in_neutral_Megatron.wav", 0x02adc000, 0x0000f314, CRC(68dfe30e) SHA1(446054e679ea99be565834631b02fed8c588f73a))
    ROM_LOAD("VOfx_OP_026b_Megatron_must_be_stopped_no_matter_the_cost.wav", 0x02aec000, 0x0000b59a, CRC(b24f8bc7) SHA1(4e2ef0be6cb874c4d2107ceb8bca7763a2d75c00))
    ROM_LOAD("VOfx_OP_037a_We_are_one_step_closer_to_stopping_the_Decepticons.wav", 0x02af8000, 0x00019030, CRC(518cc3d3) SHA1(1ffa32fba31be56eaa87f81148ae5c26e8df23d1))
    ROM_LOAD("VOfx_OP_045a_Protect_the_humans.wav", 0x02b12000, 0x0000b118, CRC(5548f8e4) SHA1(9130ce694edec1e4ce7f07853aa432047ac25fb7))
    ROM_LOAD("VOfx_OP_046c_Scan_your_perimeter.wav", 0x02b1e000, 0x0000985c, CRC(e23f6e59) SHA1(8faea29fc4b49cb1eaecbfddc436074986c69e25))
    ROM_LOAD("VOfx_OP_049a_Always_believe_that_good_will_triumph.wav", 0x02b28000, 0x000121be, CRC(9626a295) SHA1(0901976cfa368fb703a11ce82c4bb816cb2f8993))
    ROM_LOAD("VOfx_OP_050a_It_is_time_for_battle.wav", 0x02b3b000, 0x0000c7e8, CRC(8f071633) SHA1(f2e01eef1989ba0fdd80e34c357f78c1402f93ed))
    ROM_LOAD("VOfx_OP_051f_Autobots_transform_and_roll_out.wav", 0x02b48000, 0x00010cce, CRC(a30bf3d1) SHA1(16a9c7d8998f7057924b4cb86be3352dc50ecea3))
    ROM_LOAD("VOfx_OP_053a_Trust_your_instincts.wav", 0x02b59000, 0x00008a44, CRC(9e9c6dea) SHA1(efd452671518f37913b97b93b8d97887a24892a7))
    ROM_LOAD("VOfx_OP_056a_Keep_it_steady.wav", 0x02b62000, 0x00006d08, CRC(62a60a34) SHA1(adf4baf3b6fe88fd21685fac0858b2757519bc17))
    ROM_LOAD("VOfx_OP_057a_Autobots_This_is_going_to_be_a_very_dangerous_mission.wav", 0x02b69000, 0x000186a2, CRC(e972efd3) SHA1(fe48e5e46e2725d348338a16488113e6f04e8681))
    ROM_LOAD("VOfx_OP_058b_We_re_counting_on_you_don_t_drop_the_ball.wav", 0x02b82000, 0x0000fdfc, CRC(2ca1b0ed) SHA1(fad384eb8bae40f0f2f79dbd10e8018aaed9e274))
    ROM_LOAD("VOfx_OP_080a_Megatron_it_s_over.wav", 0x02b92000, 0x0000c91c, CRC(e99f0746) SHA1(ecd125b9b915a24f6fbb4ef52dd247af6f3eea6f))
    ROM_LOAD("VOfx_OP_084b_Shoot_Again.wav", 0x02b9f000, 0x00007a1a, CRC(ffae0810) SHA1(bb71265a04d0c173b6b9c9e28e54cfcedaef66f8))
    ROM_LOAD("VOfx_OP_088d_Super_Jackpot.wav", 0x02ba7000, 0x000092c6, CRC(32217901) SHA1(c8a60928c76b8ba5c6759bd88b0af252b9037e42))
    ROM_LOAD("VOfx_OP_090a_Three.wav", 0x02bb1000, 0x00005d7e, CRC(4a899303) SHA1(200fdf1f9aa328afeb509f60dce6951a80fb2c9a))
    ROM_LOAD("VOfx_OP_091a_Two.wav", 0x02bb7000, 0x000055a6, CRC(e76eda5b) SHA1(eda0d8019facf83643c149fdb0c587b0f74925b7))
    ROM_LOAD("VOfx_OP_092a_Four.wav", 0x02bbd000, 0x00006534, CRC(7732b177) SHA1(a165129938b937912f52765a45270b1b194c952c))
    ROM_LOAD("VOfx_OP_102b_Battle_for_Cybertron_is_ready.wav", 0x02bc4000, 0x0000feac, CRC(8ecd68f4) SHA1(031d2ced41fe28007378c963c840086fef051456))
    ROM_LOAD("VOfx_OP_Face_Me_Megatron_2_added_line.wav", 0x02bd4000, 0x0000be98, CRC(fbde0ae1) SHA1(c7f9da915fa812b0e282cfdb9e04abb83b2e405d))
    ROM_LOAD("VOfx_OP_jack_pot_1_added_line.wav", 0x02be0000, 0x00005f16, CRC(bd024f18) SHA1(0d64ca4f81e8e676e7534b4a888e192bb3e1c33a))
    ROM_LOAD("VOfx_OP_jack_pot_2_added_line.wav", 0x02be6000, 0x00005dae, CRC(5c8d05da) SHA1(838d8cc651f23b7196a755dffec4df7a19dd41da))
    ROM_LOAD("VOfx_OP_One_added_line.wav", 0x02bec000, 0x000057c2, CRC(b8896ac1) SHA1(afe1cd874f6001f90cbad88c080f9346bf699c55))
    ROM_LOAD("VOfx_OP_Optimus_Prime_Multiball_added_line.wav", 0x02bf2000, 0x0000ec6e, CRC(36af267a) SHA1(feb0e613b4331b6a4212fbf63d7133121edc59b3))
    ROM_LOAD("VOfx_OP_robot_JackPOT_.wav", 0x02c01000, 0x00004ef4, CRC(4599896b) SHA1(cd9d4bfce8a82c33c2769fa3e8dec6da198b57e9))
    ROM_LOAD("windDown02.wav", 0x02c06000, 0x00032c74, CRC(3c706081) SHA1(7567d990c9d3d5fa0a19853abf7110f111acf30a))
ROM_END
CORE_GAMEDEF(tfcp, 1018, "Transformers The Pin (1.0.18)", 2012, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Avengers The Pin
/-------------------------------------------------------------------*/
/* Transformers The Pin's hardware (game folder "avengers-elg", from a raw dump of its card,
   avengers.bin): node 8 with 49 switches, 8 coils and 64 lamps, the alphanumeric display, the sounds
   as WAV files. The machine runs the folder's "game", 1.0.3 ("game0" beside it is a copy). A home
   model: no coin door. The numbers are the game's own plus one */
SPIKE1_INIT_ALNUM(avscp, "avengers-elg", "game", 64, SPIKE1_SWITCHES(2, 3, 4, 5, 0, 0, 0, 0, 49, 0))
ROM_START(avscp_103)
  ROM_REGION(0x03687000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x0010c319, CRC(78eecb7e) SHA1(9d753220125a14654b36ba3eb10fe0e9eb181a8a))
    ROM_LOAD("BG_BattleForEarth.wav", 0x0010d000, 0x001c202e, CRC(d6b9dd97) SHA1(d72b727fe43f4748a15531f0bc4a602305184226))
    ROM_LOAD("BG_BlackWidowLoop.wav", 0x002d0000, 0x00493770, CRC(4d8586ff) SHA1(4f9515d00c782d8bc5c8b407b72c94254c119c58))
    ROM_LOAD("BG_BlackWidowShooterGroove.wav", 0x00764000, 0x000bb82e, CRC(0116a36b) SHA1(d08432cb630f996076c58877a07f3c4fc63fa8c4))
    ROM_LOAD("BG_BonusStart.wav", 0x00820000, 0x00010e2e, CRC(540f8393) SHA1(36039bc161054ef7f6832145468e7aeab99725f1))
    ROM_LOAD("BG_BonusTotal.wav", 0x00831000, 0x00026628, CRC(51e87953) SHA1(801132b398d3d454808fba69038c484398789bf4))
    ROM_LOAD("BG_CaptainAmericaLoop.wav", 0x00858000, 0x002d9f90, CRC(94a4c43c) SHA1(79e47c6c87a9876f3204c4550ff81cd3bef8f5db))
    ROM_LOAD("BG_CaptainAmericaShooterGroove.wav", 0x00b32000, 0x000abc44, CRC(86f4259a) SHA1(a81efc781eb1531f8d380a81976d739e46811239))
    ROM_LOAD("BG_HawkeyeLoop.wav", 0x00bde000, 0x00282de4, CRC(014425aa) SHA1(32f37fe634d65a04e607dbe440be032004557dc5))
    ROM_LOAD("BG_HawkeyeShooterGroove.wav", 0x00e61000, 0x000a0b9a, CRC(7d2707ca) SHA1(429fe5627a3b20bc44dc6364c1cedc42de19fc4b))
    ROM_LOAD("BG_HulkLoop.wav", 0x00f02000, 0x0017702e, CRC(77835be7) SHA1(9bb16cd16ba4fbf98d7657940ff697a90a23cfc8))
    ROM_LOAD("BG_HulkShooterGroove.wav", 0x0107a000, 0x0009602e, CRC(33389b03) SHA1(0e22645202fcce052a31d60a736bb969d0c3dfb3))
    ROM_LOAD("BG_IronManLoop.wav", 0x01111000, 0x00218f14, CRC(bf1c49ae) SHA1(0050df08f1300a2af94609b3f1d2d13e845af664))
    ROM_LOAD("BG_IronManShooterGroove.wav", 0x0132a000, 0x0006648a, CRC(541db5e1) SHA1(e1990efbe1b4abbd3072f2b107ac77a085b583e2))
    ROM_LOAD("BG_LokiLoop.wav", 0x01391000, 0x0026162e, CRC(82474d0f) SHA1(6ee05ed50b5e31c464e1734e512f5a7cf8f7a982))
    ROM_LOAD("BG_MainLoop.wav", 0x015f3000, 0x002b4518, CRC(6ccf9865) SHA1(8a5750bc98873fa2328dc3556c18166328c1fb06))
    ROM_LOAD("BG_MainShooterGrooveLoop.wav", 0x018a8000, 0x000ad168, CRC(bbe82027) SHA1(54a74441dc919e6786f3c8f0aeddf3deb1da6634))
    ROM_LOAD("BG_Tesseract_01.wav", 0x01956000, 0x0011942e, CRC(df559de4) SHA1(b99042dee16fc0b6b046102b7d0452a4e6a756dd))
    ROM_LOAD("BG_Tesseract_Terminate.wav", 0x01a70000, 0x000639ee, CRC(8a8e3004) SHA1(b89b32ae597b08ea885ea8de2def8f4dd4c31f21))
    ROM_LOAD("BG_ThorLoop.wav", 0x01ad4000, 0x0027602c, CRC(5bac4b69) SHA1(6629d7a6d0d0d0efc8f77993e2003fb3c428ba87))
    ROM_LOAD("BG_ThorShooterGroove.wav", 0x01d4b000, 0x000b402e, CRC(156fbac2) SHA1(385ccb144e921cab86eaae18f621ed2f7010d19d))
    ROM_LOAD("FX_Add_A_Player.wav", 0x01e00000, 0x00020a32, CRC(1cb1f8b7) SHA1(180825f77c20277c519c9c179c040f97fd3ab8e9))
    ROM_LOAD("FX_Advance_CAmerica_001.wav", 0x01e21000, 0x000262ae, CRC(f10948d5) SHA1(4b5bd9464730ee94144ed55e091a4114bee315c2))
    ROM_LOAD("FX_Advance_Hawkeye_1.wav", 0x01e48000, 0x0001413a, CRC(4ffd7a75) SHA1(910d695f31591b7c8044c37431e687b9ec4791fc))
    ROM_LOAD("FX_Advance_Hawkeye_2_3.wav", 0x01e5d000, 0x0002176e, CRC(e480145d) SHA1(6c6ee3ea23ff2f4db99c9d43e2f30d881ba02b09))
    ROM_LOAD("FX_Advance_IronMan_1.wav", 0x01e7f000, 0x0002f7f2, CRC(2bb92cfc) SHA1(126b2067c3d8899a8f44eda83b4296dc0c93063a))
    ROM_LOAD("FX_Advance_IronMan_2_4.wav", 0x01eaf000, 0x0002f434, CRC(5766151b) SHA1(3d488b0d54ec4fbab9a0ed1f96c46e2ec2c77acc))
    ROM_LOAD("FX_Advance_Thor_001.wav", 0x01edf000, 0x000271ae, CRC(cdadb2e9) SHA1(72aaea3bd5d216622a31899454122c4b9c6037ee))
    ROM_LOAD("FX_Advance_Thor_2_4.wav", 0x01f07000, 0x00014ff2, CRC(e8e54a3f) SHA1(a03de5cfe776c37b80e5de5ca7bbcc0111e8731c))
    ROM_LOAD("FX_AutoPlunge.wav", 0x01f1c000, 0x00022c96, CRC(59a0ec77) SHA1(e89495560be9aca1e9a6aca0785b74efa0e757e8))
    ROM_LOAD("FX_Ball_Save.wav", 0x01f3f000, 0x000098ca, CRC(e28f9ac3) SHA1(1b6231b8cb72f9960f40cf635f4ecf8f60c2e630))
    ROM_LOAD("FX_Bang01.wav", 0x01f49000, 0x00018108, CRC(1bc4b26a) SHA1(6a74a83948e040e38f74809be3b359b75b50a096))
    ROM_LOAD("FX_BonusSeqMult.wav", 0x01f62000, 0x00011a4c, CRC(e6fa7523) SHA1(2daec560f865a6070f90738fd66258cc4ee8aacd))
    ROM_LOAD("FX_Bottom_Lane_Lit.wav", 0x01f74000, 0x00015314, CRC(56df9790) SHA1(5f0312af0e74037b707f3eb3dbd80520c031b2aa))
    ROM_LOAD("FX_Bottom_Lanes_Complete.wav", 0x01f8a000, 0x00024f72, CRC(cd0a8f93) SHA1(a4d0d85b4899af91e1710eee97ac9dfb3a93a44d))
    ROM_LOAD("FX_BottomLane_Unlit.wav", 0x01faf000, 0x0002686c, CRC(02b2a928) SHA1(c23f8b4ffaee66304a9ab705abd64d97134c8093))
    ROM_LOAD("FX_Bung_01.wav", 0x01fd6000, 0x0001307e, CRC(49fce7c5) SHA1(c7bc08813fdb232647e2e5e268235ccafdf4e031))
    ROM_LOAD("FX_BWidow_Advance_001.wav", 0x01fea000, 0x0001c788, CRC(80e075f3) SHA1(918e32804e6620403fd7959bdb0eeeddf69af9ab))
    ROM_LOAD("FX_BWidow_Advance_002_4.wav", 0x02007000, 0x000233b8, CRC(a6235ee2) SHA1(45956f096d13e980660517f8ce68df4daa1335ab))
    ROM_LOAD("FX_BWidow_Mode_Start.wav", 0x0202b000, 0x0002b71a, CRC(69dfc830) SHA1(19de278fdeae4b820ce1821a0b67f45de71fa880))
    ROM_LOAD("FX_BWidow_Scores_Letter.wav", 0x02057000, 0x00022d76, CRC(d17ff301) SHA1(83e7894c4e6dc21cc41d18bb2bec908323ba609a))
    ROM_LOAD("FX_CA_PopBumperHits_001.wav", 0x0207a000, 0x0000fe76, CRC(aa70c195) SHA1(736da832d246bb73315b8ef7fe67e2cc15d8ea6f))
    ROM_LOAD("FX_CA_PopBumperHits_002.wav", 0x0208a000, 0x0000e176, CRC(e76345e9) SHA1(9ad7a4503de8a804d6d873f7dfb4ecee7573057b))
    ROM_LOAD("FX_CA_PopBumperHits_003.wav", 0x02099000, 0x0001b136, CRC(bda896c6) SHA1(92c607c637999c1783ac82232ffbb8537f74eec0))
    ROM_LOAD("FX_CA_PopBumperHits_004.wav", 0x020b5000, 0x0000cc66, CRC(4fa8740a) SHA1(577c469336ccaa6f5bb6b908814bed9d31a2d2b2))
    ROM_LOAD("FX_CAFights_001.wav", 0x020c2000, 0x00008e1e, CRC(bc14702e) SHA1(16a1af782dbb5e72cec79cc2e4aaae447a6d5263))
    ROM_LOAD("FX_CAmerica_Mode_Start.wav", 0x020cb000, 0x00025448, CRC(fa7c81f7) SHA1(cbc4ebfcbbb6489ba97ccc84f0b4be2ff71b4d97))
    ROM_LOAD("FX_Captain_Bonus.wav", 0x020f1000, 0x0002b140, CRC(f1c349fd) SHA1(8bd1aff4f291b8aa35f65ed689bc5de362618963))
    ROM_LOAD("FX_Captain_Bonus_2.wav", 0x0211d000, 0x0002cc40, CRC(b69dd80f) SHA1(fc02a5c083b38f372b2e3a6a72dad30e962fb8c0))
    ROM_LOAD("FX_DE_BW_FacePunch.wav", 0x0214a000, 0x0001b506, CRC(d59956dc) SHA1(45e339d1e181f0c861ad73c96272964b14504a8a))
    ROM_LOAD("FX_DE_BW_Guns.wav", 0x02166000, 0x0001724c, CRC(4914678d) SHA1(9563b98168f40e2c56e1724f27e819e2c16a9385))
    ROM_LOAD("FX_DE_IM_ChestWeapon.wav", 0x0217e000, 0x00017e90, CRC(946c0dd0) SHA1(1e47072308494725838841c81cdafe3b56e59580))
    ROM_LOAD("FX_DE_IronManFlyBy.wav", 0x02196000, 0x00010a72, CRC(bff2cb79) SHA1(ae2ed13632a17cb686d7e33abb038cdf11a98b6a))
    ROM_LOAD("FX_DE_SlideBang_01.wav", 0x021a7000, 0x0000c3cc, CRC(078c9ab0) SHA1(caba3a6423cbdf9c5200e918c6cb75626a7c684a))
    ROM_LOAD("FX_dink.wav", 0x021b4000, 0x00001598, CRC(56fbd13f) SHA1(431d9acd7f19816d9d585f60b8bf9a93fef97de0))
    ROM_LOAD("FX_Electricity_003.wav", 0x021b6000, 0x000118dc, CRC(4d747d33) SHA1(4af9711970ad2224da82ae458df6ed2a95cba286))
    ROM_LOAD("FX_Electricity_004.wav", 0x021c8000, 0x000150ec, CRC(4f3a9101) SHA1(a83b859c24532b8cd94c2185133f1fe549d97902))
    ROM_LOAD("FX_Enter_Initials_completed.wav", 0x021de000, 0x0002b9d6, CRC(d3542d37) SHA1(19196d3ff8dcd3407008a59920111d69e6bf057c))
    ROM_LOAD("FX_Extra_Ball_Get.wav", 0x0220a000, 0x0002d4b6, CRC(8f23dc8c) SHA1(d946ea3430df0c2addd34ea6a2c24aff1ada82a3))
    ROM_LOAD("FX_Extra_Ball_Lit.wav", 0x02238000, 0x0001e606, CRC(846700ac) SHA1(735638e683ed89c6af90ef1d013b9e57b0934358))
    ROM_LOAD("FX_Hawkeye_Bonus_2.wav", 0x02257000, 0x000114ec, CRC(2926d14d) SHA1(17b4fbb3cbbdc7a9f9940b25a18c619fffbd4d46))
    ROM_LOAD("FX_Hawkeye_Mode_Start.wav", 0x02269000, 0x0002cbf8, CRC(4099b82e) SHA1(ee8cc969f25546f815794e869be2cebac222508c))
    ROM_LOAD("FX_Hawkeye_Scores_Letter.wav", 0x02296000, 0x0001e6ec, CRC(022a39c5) SHA1(8e78b1aeb7f2073d0487a60441ae74db651f5e7e))
    ROM_LOAD("FX_Heli_Score.wav", 0x022b5000, 0x0001dc5c, CRC(adb64e0f) SHA1(9f98308b40b3846a85b3dd253875217312d2c47e))
    ROM_LOAD("FX_Hulk_Bonus_2.wav", 0x022d3000, 0x0001e9d8, CRC(bd299d73) SHA1(6fcce80680d0994a14bdf988743f77583f82e828))
    ROM_LOAD("FX_Hulk_Multiball.wav", 0x022f2000, 0x00049324, CRC(98b05a10) SHA1(a56e2f110f25c9be56a82bfd1f6b2890afd20c07))
    ROM_LOAD("FX_Hulk_Not_Avail_001.wav", 0x0233c000, 0x0001566e, CRC(ea97de96) SHA1(fa04eb1870844a7c41e358170c6997a392d93b99))
    ROM_LOAD("FX_Hulk_Not_Avail_002.wav", 0x02352000, 0x00015622, CRC(08058099) SHA1(28e44294fdb2c91d22b4a1cb1aea8dee7a0a8ece))
    ROM_LOAD("FX_Hulk_Not_Avail_003.wav", 0x02368000, 0x00012656, CRC(9b0dfd97) SHA1(dca17155bb0487b46f1b35ff68d6bc7bf88ecf18))
    ROM_LOAD("FX_Hulk_Scores_Letters_H.wav", 0x0237b000, 0x0001442a, CRC(b526f78c) SHA1(d9178fb4271db5ccb3c4eae85b4fa95498f46a92))
    ROM_LOAD("FX_Hulk_Scores_Letters_L.wav", 0x02390000, 0x00014b3a, CRC(b3d2156e) SHA1(a93ddc1ad1637dd8304c30e7218c2f019c5c3c9b))
    ROM_LOAD("FX_Hulk_Scores_Letters_U.wav", 0x023a5000, 0x00014b3a, CRC(efbc230b) SHA1(7c2868372a16a4deacaeb0803db56ad2e9e2683c))
    ROM_LOAD("FX_Hulk_Smash_001.wav", 0x023ba000, 0x00013198, CRC(320a60fa) SHA1(24254b653bafdec01391a9471e1c7877018d3191))
    ROM_LOAD("FX_Hulk_Smash_002.wav", 0x023ce000, 0x0001508c, CRC(7df7b19c) SHA1(72068652e5babfffbd932ddf86516fd4fb67adc3))
    ROM_LOAD("FX_Hulk_Smash_003.wav", 0x023e4000, 0x000166ae, CRC(a3c179b1) SHA1(f0c868e41b790a4a4f88b786a27acd29128f0113))
    ROM_LOAD("FX_IronMan_Bonus_2.wav", 0x023fb000, 0x000114ec, CRC(c1f69cbe) SHA1(d6c50cdf686399ff028d1f5ccee16381b1092331))
    ROM_LOAD("FX_IronMan_Mode_Start.wav", 0x0240d000, 0x0001848c, CRC(ee7b5ffc) SHA1(9f8a22781139b68a03977ae2e19821c4d4e01be0))
    ROM_LOAD("FX_IronManDot.wav", 0x02426000, 0x0002a862, CRC(922d93ac) SHA1(0f3631a6fbc255146d80c7814018db383adbd9b3))
    ROM_LOAD("FX_Loki_Lock_001.wav", 0x02451000, 0x000265a8, CRC(63b0debd) SHA1(2162de1047b8e3044ad5b4aa171ac3580d6bfe83))
    ROM_LOAD("FX_Loki_Lock_002.wav", 0x02478000, 0x0002f8bc, CRC(18710cac) SHA1(5ec2dad50229ca57548931dd321fdb7e600b526f))
    ROM_LOAD("FX_Loki_Multiball_Start.wav", 0x024a8000, 0x0004d3b6, CRC(c5e6c58a) SHA1(a6824e5a8fea5504f5c9c29556d5af4ed3b8e911))
    ROM_LOAD("FX_Mystery_.wav", 0x024f6000, 0x000369d8, CRC(38544e46) SHA1(76bfb4499297a9daab8625919c4b3b832f059e48))
    ROM_LOAD("FX_Plunger_Ball_Launch.wav", 0x0252d000, 0x0003527a, CRC(37a80d4d) SHA1(583cdc9bcc08f3a2b961955c3934c2117e0bcc52))
    ROM_LOAD("FX_SHIELD_Completed.wav", 0x02563000, 0x0001882a, CRC(47f37ed5) SHA1(4da55d84c18e14ffd462158c48f900fddcf6e15c))
    ROM_LOAD("FX_SHIELD_null.wav", 0x0257c000, 0x0000af88, CRC(e3737ccf) SHA1(96ae00bf87ab81cdd404f48195c268d7d5dd3389))
    ROM_LOAD("FX_SHIELD_unlit.wav", 0x02587000, 0x0001dfd2, CRC(99a97c40) SHA1(e81c69dfb1b99d1db2849e43f631ff7028bab6c8))
    ROM_LOAD("FX_ShootAgain.wav", 0x025a5000, 0x0002ba6e, CRC(8c7c2412) SHA1(1c8a3e84d84b6f1bea9b78cc36bca92a6131d9d8))
    ROM_LOAD("FX_SILENCE_0400_MS.wav", 0x025d1000, 0x000089fc, CRC(1f069d20) SHA1(adc283986743497e128c4887d5f6216a3df36e9e))
    ROM_LOAD("FX_Skill_Shot_Award.wav", 0x025da000, 0x000248e4, CRC(28021d5f) SHA1(0649d888e621a17e842b555a2612e640939c3ab5))
    ROM_LOAD("FX_Slingshots_001.wav", 0x025ff000, 0x00023322, CRC(d54b275e) SHA1(3a9171f04098ac46fa03e98aee0da689c551922b))
    ROM_LOAD("FX_Slingshots_002.wav", 0x02623000, 0x00023322, CRC(4d7ea891) SHA1(ed93f48ea818a5d39067acc44cb395192e2c7e0e))
    ROM_LOAD("FX_Slingshots_003.wav", 0x02647000, 0x00023322, CRC(58d835ae) SHA1(0d1b7d5dd372dbb8403d47c7a6f2370a7f9fab6c))
    ROM_LOAD("FX_Tess_Seg_000.wav", 0x0266b000, 0x0001ce7e, CRC(c8f00370) SHA1(89ca690c220b30c32047ff75a5403dad7d5912e0))
    ROM_LOAD("FX_Tess_Seg_001.wav", 0x02688000, 0x0001d83e, CRC(578b9a86) SHA1(ee563ebe3f647a468982d21bc10f1c1d788b01b2))
    ROM_LOAD("FX_Thor_Bonus_2.wav", 0x026a6000, 0x00029cb4, CRC(8389edf5) SHA1(1140ce893618d6a386c557e20040f32f45386b5e))
    ROM_LOAD("FX_Thor_Hammer_Returns.wav", 0x026d0000, 0x0000b44e, CRC(4842503a) SHA1(79710bdf41be552b47a71c9d2cfdc4cdfd382fab))
    ROM_LOAD("FX_Thor_Hit_Null_001.wav", 0x026dc000, 0x0000a300, CRC(7a5ee36a) SHA1(62a82be6c6eada375315fd077f0cc54d17e6c313))
    ROM_LOAD("FX_Thor_Hit_Null_002.wav", 0x026e7000, 0x0000a300, CRC(daa186d7) SHA1(e49dc3a6dd9750d4e000129dc05c940f33835ce6))
    ROM_LOAD("FX_Thor_Mode_Start.wav", 0x026f2000, 0x00026b9a, CRC(e443f5e7) SHA1(a2d3d0c71f46e2589881d7ba3ad995ccca75323c))
    ROM_LOAD("FX_Thor_Score_001.wav", 0x02719000, 0x00022252, CRC(e6b13f14) SHA1(3a23a99b43998a0f62da9bcf2483b2557ae61b04))
    ROM_LOAD("FX_Thor_Thunder_001.wav", 0x0273c000, 0x0003f4fa, CRC(8475f5fd) SHA1(f1a6e22bc5de4278d7c7d30c4342c10b8d910e61))
    ROM_LOAD("FX_Thor_Thunder_002.wav", 0x0277c000, 0x00013bf0, CRC(41ec94b4) SHA1(6222314a26b8b788599b2a920e58ed7bc2a7fa2a))
    ROM_LOAD("FX_Thor_Thunder_003.wav", 0x02790000, 0x00025b9c, CRC(9ee0b809) SHA1(a88db7c455083d6dbce07d932094a1b3c902cca8))
    ROM_LOAD("FX_Thor_Thunder_004.wav", 0x027b6000, 0x0000a666, CRC(33eac99e) SHA1(b4c3520fa5a6ecae4c3daa15655ea34e006e16dd))
    ROM_LOAD("FX_Tilt_Tilt.wav", 0x027c1000, 0x00034776, CRC(bff9386d) SHA1(a03ef9c5d1f0ffe288966ae0dfe5ed3d0eda23ea))
    ROM_LOAD("FX_Tilt_Warning.wav", 0x027f6000, 0x0001668c, CRC(eef13fd4) SHA1(4f3369cef3b43a3d2b7b19aa0b1d7439d87dcfe7))
    ROM_LOAD("FX_Top_Lanes_Lit.wav", 0x0280d000, 0x00015cc2, CRC(34a8b99c) SHA1(e0141edd9d1862630dd388073879f44711193145))
    ROM_LOAD("FX_Top_Lanes_Unlit.wav", 0x02823000, 0x00016bd2, CRC(02c4fd0e) SHA1(f7dadbeb82b2c4c36e09a1877828ba68139f5699))
    ROM_LOAD("VO_001_AN_BLACK_WIDOW.wav", 0x0283a000, 0x00007dea, CRC(543fa6d7) SHA1(3e2d79a763739cb768b75491d5f03c6ee43d8a21))
    ROM_LOAD("VO_002_AN_CAPTAIN_AMERICA.wav", 0x02842000, 0x00006e58, CRC(c61cae1d) SHA1(dff2cf50b8761687705464325e50163875bb4e76))
    ROM_LOAD("VO_004_AN_HAWKEYE.wav", 0x02849000, 0x000046da, CRC(d108f6cb) SHA1(4cef9ec31bf4ba6255a3e42d626bb360905f1778))
    ROM_LOAD("VO_005_AN_HULK.wav", 0x0284e000, 0x0000458e, CRC(5f47d8e1) SHA1(485756a0209549558b9e60a1c3e687579a74887a))
    ROM_LOAD("VO_006_AN_IRONMAN.wav", 0x02853000, 0x00005f06, CRC(ff877b4e) SHA1(1816e18a0859460481c06d96d24dc922895aca59))
    ROM_LOAD("VO_007_AN_THOR.wav", 0x02859000, 0x00005488, CRC(a1e52eb6) SHA1(b3e2eb085c4abceb42ffddcdb04247212ed30dc3))
    ROM_LOAD("VO_020_AN_BLACK_WIDOW_COLLECTED.wav", 0x0285f000, 0x00009dd6, CRC(2346c7d6) SHA1(5d524b63f4420e356b741535c697cb0ece2bd29e))
    ROM_LOAD("VO_020_AN_BLACK_WIDOW_COMPLETED.wav", 0x02869000, 0x0000b1b4, CRC(fe68485e) SHA1(a0998c8173bc16a08e165fb20fc35ddbb6e62672))
    ROM_LOAD("VO_021_AN_CAPTAIN_AMERICA_COLLECTED.wav", 0x02875000, 0x0000d6c6, CRC(80907aa2) SHA1(a578936de3e9d564ccd4829324e335927f89bf5b))
    ROM_LOAD("VO_021_AN_CAPTAIN_AMERICA_COMPLETED.wav", 0x02883000, 0x0000ca18, CRC(bc1d3f8d) SHA1(9acbe3a5a56a6fc71124b10375eaf84c8f7dc099))
    ROM_LOAD("VO_022_AN_HAWKEYE_COMPLETED.wav", 0x02890000, 0x0000b178, CRC(ba990573) SHA1(326529d3cfaed3969cc83d623c3088040dfc96b3))
    ROM_LOAD("VO_022_AN_HAWKEYE_COMPLETED_A.wav", 0x0289c000, 0x0000b39e, CRC(90090ea9) SHA1(67a04d8ff29f015b8faa0d209c9f73a06caa3a6f))
    ROM_LOAD("VO_023_AN_JACKPOT_1.wav", 0x028a8000, 0x00004f58, CRC(77d5f877) SHA1(69a992734afc3e085e175fa80dba69ed4106da22))
    ROM_LOAD("VO_023_AN_JACKPOT_2.wav", 0x028ad000, 0x000054ea, CRC(d28647ca) SHA1(81177e906b1c94fdf9a0da1098310925d2effa11))
    ROM_LOAD("VO_023_AN_JACKPOT_3.wav", 0x028b3000, 0x00007540, CRC(6df32a2b) SHA1(502324cd5c4024688784b1b43edcd9e6ec800609))
    ROM_LOAD("VO_023_AN_JACKPOT_4.wav", 0x028bb000, 0x0000747a, CRC(177b1e46) SHA1(b497ec8c35f60420dd796c80edabae81ae248189))
    ROM_LOAD("VO_023_AN_JACKPOT_5.wav", 0x028c3000, 0x00005a48, CRC(6fd81945) SHA1(b4069376a9ad81b848f2fc728a67722c6350b378))
    ROM_LOAD("VO_024_AN_IRONMAN_COLLECTED.wav", 0x028c9000, 0x0000abfa, CRC(e32c8ba3) SHA1(388bc6f20d7cc18d4b8db3a5aa21aa963630c0cf))
    ROM_LOAD("VO_024_AN_IRONMAN_COMPLETED.wav", 0x028d4000, 0x00008fa2, CRC(d2b73f1f) SHA1(68311e0cd7e54db2d71afc40b52690b922076824))
    ROM_LOAD("VO_025_AN_THOR_COLLECTED.wav", 0x028dd000, 0x0000b17c, CRC(8ed1d7fd) SHA1(6b4bb941562afa4e3b511e4e03681479730b10f1))
    ROM_LOAD("VO_025_AN_THOR_COMPLETED.wav", 0x028e9000, 0x000096ae, CRC(dd670932) SHA1(db922c73ee754584c4ee4493e827477d275cf747))
    ROM_LOAD("VO_026_AN_HULK_COLLECTED.wav", 0x028f3000, 0x00009dda, CRC(6a3d2c3d) SHA1(23b5cec7c9cd7962db12f8635aec3884e88b98cd))
    ROM_LOAD("VO_026_AN_HULK_COMPLETED.wav", 0x028fd000, 0x0000c6ce, CRC(70c933d9) SHA1(4b57c070fc81ea2d29f9bcfc6315b078459960b1))
    ROM_LOAD("VO_035_BW_NEVER_UNDERESTIMATE_ME.wav", 0x0290a000, 0x0000a4ac, CRC(33dea6e3) SHA1(3f4d2cdf6c9c298f827268e88516f0bc18106f8b))
    ROM_LOAD("VO_036_BW_MY_MAIN_WEAPON_IS_MY_CHARM.wav", 0x02915000, 0x0000c8b2, CRC(14e525e3) SHA1(9c6cc62ef9e66857a9d8df57adfe2f926ed565dc))
    ROM_LOAD("VO_037_BW_MY_NAME_IS_NATASHA_BUT_YOU_CAN_CALL_ME_THE_BLA.wav", 0x02922000, 0x00017550, CRC(f904fd81) SHA1(788f8fc6c3a1d8152c7430cb5dadc1d97a7912a9))
    ROM_LOAD("VO_038_BW_CROSS_ME_AND_YOULL_FIND_OUT_HOW_I_GOT_MY_NAMES.wav", 0x0293a000, 0x000171a2, CRC(4cce96c5) SHA1(5015a43ec2d1e68dd4ee80a8e1df37c783d05ab0))
    ROM_LOAD("VO_039_BW_DID_I_MENTION_IM_AN_EXPERT_GYMNAST.wav", 0x02952000, 0x0000e966, CRC(7bb13ed8) SHA1(6930d5a75094cc34d9d748ae2251514d6403aeae))
    ROM_LOAD("VO_040_BW_IVE_DONE_THINGS_IM_NOT_PROUD_OF_ID_LIKE_TO_ATO.wav", 0x02961000, 0x00014e96, CRC(a494ee40) SHA1(75beb0a37f3f1d9ac54168c0715b57c6ef44b7ba))
    ROM_LOAD("VO_041_BW_AGENT_BARTON_WAS_SENT_TO_KILL_ME_HE_MADE_A_DIF.wav", 0x02976000, 0x00013d1c, CRC(24827f2e) SHA1(d152524eaeee7c48f44cb94088de0d2d54fc32bd))
    ROM_LOAD("VO_042_BW_AVENGERS_ASSEMBLE.wav", 0x0298a000, 0x0000973c, CRC(cdfc7dce) SHA1(58efdb55bc0ca9b84e0b63dd872d130f53691d28))
    ROM_LOAD("VO_043_BW_REGIMES_FALL_EVERYDAY_I_TEND_NOT_TO_WEEP_OVER_.wav", 0x02994000, 0x00012ce2, CRC(437e1369) SHA1(6cf5875924ae6def300889983032e9f744a3cdbf))
    ROM_LOAD("VO_080_CA_THEY_CALL_ME_THE_FIRST_AVENGER_NOW_ILL.wav", 0x029a7000, 0x00014942, CRC(b7770311) SHA1(9cf7cbc6f0da301f87173be1a5dba505ca93702f))
    ROM_LOAD("VO_081_CA_WE_AVENGERS_WILL_ALWAYS_FIGHT_THE_GOOD.wav", 0x029bc000, 0x0000d8e8, CRC(e8d44788) SHA1(ac69630c842f24fe0dfc41c89e906b0b930a075b))
    ROM_LOAD("VO_083_CA_AVENGERS_ASSEMBLE_1.wav", 0x029ca000, 0x0000965e, CRC(e2879a36) SHA1(afb51d3d822fce8563d36da793fb5bdce01d368c))
    ROM_LOAD("VO_085_CA_I_DONT_LIKE_BULLIES_NO_MATTER_WHERE_TH.wav", 0x029d4000, 0x0001227c, CRC(e7d8c3bf) SHA1(d4862b2bae5df2a34c9e43de76ce8c0d8b48e490))
    ROM_LOAD("VO_086_CA_IM_JUST_A_KID_FROM_BROOKLYN.wav", 0x029e7000, 0x00008844, CRC(602cdafb) SHA1(74ab837afb5a1095577b921ca43d52b73df99153))
    ROM_LOAD("VO_087_CA_IF_YOU_CHOOSE_TO_STAND_IN_THE_WAY_OF_F.wav", 0x029f0000, 0x0002501a, CRC(06c66d69) SHA1(2c5355cdc96f508605f98756b5df86c40ca5f3da))
    ROM_LOAD("VO_088_CA_WE_HAVE_A_MISSION_TO_CARRY_OUT_PEOPLE_.wav", 0x02a16000, 0x0001250e, CRC(f18f5553) SHA1(cedbccd27af2e25d058c020cf4224e5768e93955))
    ROM_LOAD("VO_089_CA_YOU_HAVE_YOUR_DEFINITION_OF_A_HERO_AND.wav", 0x02a29000, 0x0001e4d2, CRC(0b2a6797) SHA1(dc8d5882e2aa32ea0bf5b6b7761735d82fbe2b1b))
    ROM_LOAD("VO_090_CA_THIS_SOLDIER_FIGHTS_FOR_FREEDOM_AND_JU.wav", 0x02a48000, 0x0000de52, CRC(b8866ec0) SHA1(39a71ebcab0f94a64579aaf7a3e40e9b68e6a4b6))
    ROM_LOAD("VO_091_CA_I_FIGHT_FOR_THE_FLAG.wav", 0x02a56000, 0x0000854e, CRC(11a6342b) SHA1(e2a3154999523c3c3bc3414fa852fd885a1ee859))
    ROM_LOAD("VO_092_CA_MISTER_YOU_JUST_MADE_THE_GREATEST_MIST.wav", 0x02a5f000, 0x0000ffa0, CRC(dc03e33e) SHA1(c6afede4bae26e27c02de2ecec8af94ae406e9bb))
    ROM_LOAD("VO_093_CA_YOU_THINK_YOURE_TOUGH_WELL_LETS_SEE_YO.wav", 0x02a6f000, 0x0001249c, CRC(5398b53a) SHA1(3844c9e56e3e2491d16e368b8f3070ea76a3c61b))
    ROM_LOAD("VO_095_CA_WE_HAVE_NO_CHOICE_SO_WE_FIGHT_AND_WE_W.wav", 0x02a82000, 0x0001fd12, CRC(744d4b3c) SHA1(50505f012db31ec57204a69657cf8e6cf179735e))
    ROM_LOAD("VO_096_CA_NO_ONE_DIES_ON_MY_WATCH.wav", 0x02aa2000, 0x0000a58e, CRC(4ad44f3d) SHA1(db845410716dd3797290250f4a2b3c8df6e1f1a7))
    ROM_LOAD("VO_098_CA_THE_BATTLE_IS_NEVER_ENDED_THERE_WILL_B.wav", 0x02aad000, 0x000161c2, CRC(9f7db054) SHA1(0bcc09648f310a0c027dc00560648e5f870664cf))
    ROM_LOAD("VO_099_CA_FORGET_WHAT_SETS_US_APART_AND_REMEMBER.wav", 0x02ac4000, 0x000126ca, CRC(977517c1) SHA1(73060dc4f3ccd7cd8367561409c9a2ecaa4c1704))
    ROM_LOAD("VO_100_CA_I_WILL_ALWAYS_PROTECT_THOSE_WHO_CANNOT.wav", 0x02ad7000, 0x0001246a, CRC(38f16ba7) SHA1(bd5ec592b943f62e88ab1d3f700530cffd69f917))
    ROM_LOAD("VO_102_CA_FREEDOM_WILL_PREVAIL.wav", 0x02aea000, 0x0000a242, CRC(5f984848) SHA1(004c0bf3c82dc2edaa017ed1c40092e8e3d515c9))
    ROM_LOAD("VO_137_CA_SON_OF_A_GUN.wav", 0x02af5000, 0x00004fda, CRC(efd09e1f) SHA1(9c53758d79bc87d166208a047dbc9ff7a610d661))
    ROM_LOAD("VO_140_CA_GET_BACK_IN_THERE_SOLDIER.wav", 0x02afa000, 0x00006f18, CRC(fd1a4808) SHA1(1a852286d3e308c51c4c3a0385a4f54eb6198066))
    ROM_LOAD("VO_141_CA_NEVER_GIVE_UP_FREEDOM_WILL_PREVAIL.wav", 0x02b01000, 0x0000e648, CRC(7c0d6b2d) SHA1(156d3df4ddcc59e272aaea54ae6304fcdb3555b4))
    ROM_LOAD("VO_142_CA_GOOD_WORK_SOLDIER.wav", 0x02b10000, 0x000076ca, CRC(ddd220d0) SHA1(080eb7dd1f18976b2cebae5d5d62c1fa128c6752))
    ROM_LOAD("VO_156_HE_PIECE_OF_CAKE.wav", 0x02b18000, 0x00008470, CRC(6806c102) SHA1(90b1312642d2da8a926689f46113862a0f40de82))
    ROM_LOAD("VO_158_HE_AVENGERS_ASSEMBLE.wav", 0x02b21000, 0x000080dc, CRC(2040fa9b) SHA1(6803a68c7e242db9720d2031a1094fb6ccb5501f))
    ROM_LOAD("VO_162_HE_I_SEE_BETTER_FROM_A_DISTANCE.wav", 0x02b2a000, 0x0000a3b6, CRC(2e827140) SHA1(2468c0d467fb17388139c0513a958eaecd3ea6e7))
    ROM_LOAD("VO_163_HE_ITS_NOT_BOASTING_IF_YOU_CAN_BACK_IT_UP.wav", 0x02b35000, 0x0000cab8, CRC(02644939) SHA1(cbdaadaab4603045268b0f464f9f9ca4ad5faf35))
    ROM_LOAD("VO_164_HE_BULLSEYE.wav", 0x02b42000, 0x00004870, CRC(7abafdc1) SHA1(162e89290528649c8a57eb764c3fefd4c8befc9e))
    ROM_LOAD("VO_166_HE_ALWAYS_ON_THE_MARK.wav", 0x02b47000, 0x000099be, CRC(2b697749) SHA1(3f72bfa09a9f4a54819e565d24c4e5c0bf8ab729))
    ROM_LOAD("VO_167_HE_THIS_TAKES_REAL_SKILL_YA_KNOW.wav", 0x02b51000, 0x0000a66a, CRC(cf63c4ce) SHA1(a84fea999ee756f7942adda38d6bca848b68d643))
    ROM_LOAD("VO_169_HE_WORLDS_GREATEST_MARKSMAN_AT_YOUR_SERVICE.wav", 0x02b5c000, 0x0000d516, CRC(e8aeef8d) SHA1(bba23570f370917d45cf02c19b2fcfd439095384))
    ROM_LOAD("VO_170_HE_THIS_IS_HOW_ITS_DONE.wav", 0x02b6a000, 0x00009164, CRC(3f0cc150) SHA1(38d7fc8d674abc301f03f60f29b69bcabee7dec2))
    ROM_LOAD("VO_171_HE_I_MAKE_ANY_TEAM_LOOK_GOOD.wav", 0x02b74000, 0x0000a468, CRC(b682a615) SHA1(f96ec3e33a56029acb525b84f8c10252dc1100ce))
    ROM_LOAD("VO_172_HE_I_GOT_THIS.wav", 0x02b7f000, 0x00007642, CRC(37874fa5) SHA1(601626b8126dbda70dfc3ccba2e9b089736f8d3d))
    ROM_LOAD("VO_173_HE_I_NEVER_MISS.wav", 0x02b87000, 0x000075b8, CRC(dea10b7c) SHA1(9f4e79bff98d570240b5adce28d56c74aad93241))
    ROM_LOAD("VO_193_HL_WHY_EVERYBODY_ALWAYS_PICK_ON_HULK.wav", 0x02b8f000, 0x0000f736, CRC(2db9b92b) SHA1(685d7987737aa58adfd4f8e9bae73a70240fc32e))
    ROM_LOAD("VO_195_HL_LEAVE_HULK_ALONE.wav", 0x02b9f000, 0x0000af7c, CRC(459750d7) SHA1(eef929f4fbb4ca7810fd3e5631f625c2f35295cb))
    ROM_LOAD("VO_196_HL_WHEN_IT_HULKS_TURN_TO_PLAY.wav", 0x02baa000, 0x0000c90c, CRC(b4e5f6bc) SHA1(8dbcf70611150e3b8a63dd1a73608474baf1f08b))
    ROM_LOAD("VO_197_HL_DONT_MAKE_HULK_COME_OVER_THERE.wav", 0x02bb7000, 0x0000d4fe, CRC(2e9c320d) SHA1(e4f14f4359b20a05a20deb2866cff633a33ced85))
    ROM_LOAD("VO_199_HL_LAST_TIME_HULK_IN_NEW_YORK_HULK_BREAK_HARLEM.wav", 0x02bc5000, 0x0001701c, CRC(408a168e) SHA1(bab51a4d987eb932edcfca0c201b2b1a99e445b9))
    ROM_LOAD("VO_200_HL_WHY_EVERYONE_ALWAYS_LAUGH_WHEN_HULK_TALK.wav", 0x02bdd000, 0x00014852, CRC(b1cbe402) SHA1(d732d576fd5a1477db583bdace676c4818c53b73))
    ROM_LOAD("VO_208_HL_LEAVE_HULK_ALONE.wav", 0x02bf2000, 0x00012e58, CRC(e5cc0a94) SHA1(d7ba72a872fea380537c5200ba3060ccb04cfcdf))
    ROM_LOAD("VO_223_HL_YOU_WANT_BALL_HULK_GIVE_YOU_BALL.wav", 0x02c05000, 0x00013d88, CRC(73e9889c) SHA1(843462ebd113c00d19acdea01ae0f9bd04dfeb12))
    ROM_LOAD("VO_227_HL_METAL_BALL_STAND_STILL_SO_HULK_CAN_SMASH.wav", 0x02c19000, 0x00019182, CRC(59f0d05d) SHA1(a5882a0e48d6aeda94f1b9a417685571e960f6d3))
    ROM_LOAD("VO_230_HL_CRUSH.wav", 0x02c33000, 0x00006564, CRC(414c88a5) SHA1(c36468827676a82583af4c72eed77f8a9e09df5c))
    ROM_LOAD("VO_239_HL_HULK.wav", 0x02c3a000, 0x000066a0, CRC(dbc8a0dc) SHA1(f7a59238e4da469a48f2d23da4c91065c81f883c))
    ROM_LOAD("VO_242_HL_BASH.wav", 0x02c41000, 0x0000d788, CRC(a56c3df7) SHA1(47a27fbcc1ad8f22b57b06b356b5a54681c56d4c))
    ROM_LOAD("VO_253_HL_ANGRY.wav", 0x02c4f000, 0x0000b006, CRC(de80f7f8) SHA1(c3abf195e6767a766e2d59a4424df022cffac3a2))
    ROM_LOAD("VO_256_HL_SMASH.wav", 0x02c5b000, 0x00009e7a, CRC(5fae5883) SHA1(8035d20b4d098ee27d0d569765b48058d4611d1d))
    ROM_LOAD("VO_259_HL_GIVE_HULK_BALL.wav", 0x02c65000, 0x00015fc2, CRC(221708e4) SHA1(fce472527eaa2b751bc0c26bb8389e1590213468))
    ROM_LOAD("VO_266_HL_DONT_GET_HULK_ANGRY_YOU_NOT_LIKE_WHEN_HULK_ANGRY.wav", 0x02c7b000, 0x0001ce82, CRC(2132394a) SHA1(afbe4436934d6159ad4637eaf92eddadca8c7c55))
    ROM_LOAD("VO_267_HL_YOU_MAKE_HULK_ANGRY.wav", 0x02c98000, 0x0000c244, CRC(f9024bbb) SHA1(6e5a561bbe89ce0b4e17874ab214ff165271c3a8))
    ROM_LOAD("VO_293_HL_WHAT_NUMBERS_FOR_NUMBERS_MAKE_HULK_DIZZY.wav", 0x02ca5000, 0x00015a60, CRC(301189eb) SHA1(80567e126e1f4f469d094d226ca50b9479fc02d4))
    ROM_LOAD("VO_302_HL_HULK_STRONGEST_ONE_THERE_IS.wav", 0x02cbb000, 0x0000e604, CRC(5379d50c) SHA1(b77d77685222ca0d51dea9f1244a310339035351))
    ROM_LOAD("VO_303_HL_HULK_SAY_GET_BACK_HERE.wav", 0x02cca000, 0x0000a2d2, CRC(322b348b) SHA1(93a2088e265bed2a1ea38ef266add506f3730424))
    ROM_LOAD("VO_323_HL_HULK_SMASH_PUNY_PLAYER.wav", 0x02cd5000, 0x0000e24c, CRC(bf04cb6f) SHA1(69ae614b661c53058df27ca9a4c93029721b4736))
    ROM_LOAD("VO_325_HL_WHAT_DOES_PUNY_PLAYER_WANT.wav", 0x02ce4000, 0x0000ca8a, CRC(49bb4375) SHA1(d64c741d3b1258453dc6c7294922e7549de414dd))
    ROM_LOAD("VO_341_IM_JARVIS_TRANSFER_40_MORE_POWER_TO_THE_THRUSTERS.wav", 0x02cf1000, 0x0000f17a, CRC(590af916) SHA1(bd6610c4c8513ca6f4b4a42eb7a3363b4f5ecfaf))
    ROM_LOAD("VO_342_IM_OFFICIAL_CONSULTING_HOURS_ARE_BETWEEN_8_AND_5_EVERY_OT.wav", 0x02d01000, 0x00014c0c, CRC(d737568c) SHA1(d92f5ab346258a7aff8ec092288b0bd3530f6c3d))
    ROM_LOAD("VO_343_IM_YOU_HAVE_REACHED_THE_LIFE_MODEL_DECOY_OF_TONY_STARK_PL.wav", 0x02d16000, 0x00017b90, CRC(0279755b) SHA1(4c6c7179aef96afd963012214832dd0722d3178c))
    ROM_LOAD("VO_344_IM_GENIUS_TAKES_TIME_UNLESS_YOURE_ME.wav", 0x02d2e000, 0x0000b718, CRC(70b554cf) SHA1(ae2ed7c12803d24086dbef41a22e4e40331093d2))
    ROM_LOAD("VO_345_IM_HOW_DOES_IT_FEEL_TO_BE_A_GENIUS_ONLY_I_KNOW_THE_ANSWER.wav", 0x02d3a000, 0x00019d58, CRC(e1253263) SHA1(c4a18d039435c2e562f7708e41ab8f5af990ffbc))
    ROM_LOAD("VO_347_IM_JARVIS_FULL_POWER_TO_THE_REPULSERS.wav", 0x02d54000, 0x0000ba48, CRC(fa09b89e) SHA1(9ee9175dfa49d348d669596352c9db86cc6e2e94))
    ROM_LOAD("VO_348_IM_AVENGERS_ASSEMBLE.wav", 0x02d60000, 0x00009740, CRC(2e2de3c7) SHA1(59eb8d96d15d69d966fc9997b578bf3e184a3e71))
    ROM_LOAD("VO_349_IM_IM_MORE_OF_A_LONE_WOLF.wav", 0x02d6a000, 0x00008498, CRC(713bf51b) SHA1(83e9af28d701d4cb94a651df7b83bd6077023de9))
    ROM_LOAD("VO_350_IM_SUPERIOR_TECH_SUPERIOR_INTELLIGENCE_WHAT_CHANCE_DOES_A.wav", 0x02d73000, 0x00019f22, CRC(d34d854c) SHA1(0e74368bc58109f8ef7c40268a3e3acd88f1cffb))
    ROM_LOAD("VO_351_IM_WEAPONS_CHARGING.wav", 0x02d8d000, 0x00006532, CRC(d4be9e69) SHA1(dd6ca96e97cd710c3cfd02d04d158bf8e9898615))
    ROM_LOAD("VO_352_IM_I_MAKE_ANY_SUIT_LOOK_GOOD_ESPECIALLY_THIS_ONE.wav", 0x02d94000, 0x00011ed2, CRC(81358c90) SHA1(bdbaddf89ae51ed446a101369a5cff0732e9d8d8))
    ROM_LOAD("VO_353_IM_YEAH_APPARENTLY_IM_VOLATILE_SELF_OBSESSED_DONT_PLAY_WE.wav", 0x02da6000, 0x00019de6, CRC(6ee22924) SHA1(178c89818c67b6c90ada1bf0519c2111293cc25a))
    ROM_LOAD("VO_354_IM_JARVIS_CANCEL_MY_AFTERNOON_MEETINGS_THIS_IS_TOO_MUCH_F.wav", 0x02dc0000, 0x00011786, CRC(8594279c) SHA1(fe0f9e6f0d6bbd1b8b65aa89fee3fb7e68a0c527))
    ROM_LOAD("VO_356_IM_JARVIS_ACTIVATE_THE_UNIBEAM.wav", 0x02dd2000, 0x00008fb4, CRC(c9bb9365) SHA1(627be5e630006d15b668d25b3e667ea4999793e2))
    ROM_LOAD("VO_357_IM_I_TOLD_YOU_I_DONT_WANT_TO_JOIN_YOUR_SUPER_SECRET_BOY_B.wav", 0x02ddb000, 0x0000d21c, CRC(41c60809) SHA1(2032f64057109179550e77fb4b9bb40cafab50db))
    ROM_LOAD("VO_358_IM_THERES_NOTHING_LIKE_FLYING_AT_THE_SPEED_OF_SOUND_EY_JA.wav", 0x02de9000, 0x0000d3f4, CRC(6ffd8975) SHA1(cf44a66ff962a542e5c15150be9774c7c6a011bc))
    ROM_LOAD("VO_359_IM_JARVIS_ARC_REACTOR_FULL_POWER.wav", 0x02df7000, 0x0000c224, CRC(3416effd) SHA1(31161decd446b19acbacb39140924962e67fde06))
    ROM_LOAD("VO_360_IM_OK_IM_HERE_THE_PARTY_HAS_NOW_OFFICIALLY_STARTED.wav", 0x02e04000, 0x00010688, CRC(39baba0e) SHA1(0ae42331dccdc5eed248bc80e0639a8fa6714296))
    ROM_LOAD("VO_417_IM_GENIUS_BILLIONAIRE_PLAYBOY_PHILANTHROPIST.wav", 0x02e15000, 0x0000dad4, CRC(8e65612f) SHA1(52f9c77f3c0e98474aa5b7ddb4b79b6d81fa9ef6))
    ROM_LOAD("VO_418_LO_YOU_ARE_BUT_AN_INSECT_TO_ME_BEGONE.wav", 0x02e23000, 0x0000feaa, CRC(102a0d0d) SHA1(eb200e0c0d2416143ab8db56db6c90977bd09faf))
    ROM_LOAD("VO_426_LO_YOU_WERE_MADE_TO_BE_RULED.wav", 0x02e33000, 0x000127f6, CRC(d903a058) SHA1(83eb57dc100b0e7077f3133a4c0460e54e8381ce))
    ROM_LOAD("VO_428_LO_I_AM_LOKI_OF_ASGARD_AND_I_AM_BURDENED_WITH_GLORIOUS_PURPOSE.wav", 0x02e46000, 0x0001b9f4, CRC(776cd006) SHA1(513dcdd597fc32ff64f05d27e4883bc13529eb4f))
    ROM_LOAD("VO_430_LO_AN_ANT_HAS_NO_QUARREL_WITH_A_BOOT.wav", 0x02e62000, 0x0000eb98, CRC(e18cc93a) SHA1(88dfa03dc3fcaae872b1358cba6048727b5ced07))
    ROM_LOAD("VO_434_LO_HOW_WILL_YOU_CONTINUE_TO_PLAY_YOUR_SILLY_GAME_WITHOUT_THIS.wav", 0x02e71000, 0x0001782a, CRC(ddbcdbac) SHA1(b5a2a207c1a0f924aad1dab7e81da68328ec0b45))
    ROM_LOAD("VO_435_LO_YOU_ARE_BUT_A_HUMAN_WHAT_HOPE_DO_YOU_HAVE_AGAINST_ME.wav", 0x02e89000, 0x000144b2, CRC(33544dbc) SHA1(e9a700a7a9725aef75ec9bd4c272ec7fc7592aba))
    ROM_LOAD("VO_437_LO_DO_YOU_POSSES_THE_TESSERACT_IF_NOT_THEN_GET_OUT_OF_MY_S.wav", 0x02e9e000, 0x0001db76, CRC(f38c68ae) SHA1(45ad6aabf05e0c819c40d937e1d48645bd02061c))
    ROM_LOAD("VO_438_LO_WHY_DONT_YOU_GO_BOTHER_MY_BROTHER_INSTEAD.wav", 0x02ebc000, 0x000137b2, CRC(c94b57c4) SHA1(a06625431e39160086f534def85508b727c0b49b))
    ROM_LOAD("VO_441_LO_I_WAS_A_KING_THE_RIGHTFUL_KING_OF_ASGARD_BETRAYED.wav", 0x02ed0000, 0x00018da0, CRC(da7ee96d) SHA1(7e31aee4e2f25c0bc1a240c03397bf5db69c5fb0))
    ROM_LOAD("VO_442_LO_IT_IS_I_LOKI_YOUR_NEXT_RULER.wav", 0x02ee9000, 0x00015408, CRC(3398c313) SHA1(15939ba07f3b87c4f6c610e7b8f42d9d9f30d06a))
    ROM_LOAD("VO_446_LO_KNEEL_BEFORE_ME.wav", 0x02eff000, 0x000079b0, CRC(4be08f89) SHA1(e04d5a800e91ec63bf09d4509494afd1d2d1f762))
    ROM_LOAD("VO_447_LO_IT_IS_MY_WILL_NOT_YOURS_THAT_THIS_PINBALL_MACHINE_OBEYS.wav", 0x02f07000, 0x000174c8, CRC(caaa38e2) SHA1(8bc49193192072015cd75be624b32df0e941de8e))
    ROM_LOAD("VO_448_LO_YET_ANOTHER_OF_EARTHS_ARTIFACTS_IN_MY_POSITION.wav", 0x02f1f000, 0x00017906, CRC(8b4167bc) SHA1(0a1329dd915e0fb51d7f747c3bbf7679fdc1c9af))
    ROM_LOAD("VO_449_LO_WHAT_MANNER_OF_THING_IS_THIS_IT_IS_NOT_FROM_ASGARD.wav", 0x02f37000, 0x000155fa, CRC(a9cb6c91) SHA1(ee7d12706a0b41ab49667aa56006a951448d13e6))
    ROM_LOAD("VO_450_LO_ANYTHING_I_CAN_TAKE_AWAY_FROM_YOU_BRINGS_A_SMILE_TO_MY_.wav", 0x02f4d000, 0x0001bace, CRC(3b6c6e02) SHA1(f36363f575bec58cc25d3d0ff62e70f824d418f2))
    ROM_LOAD("VO_451_LO_ILL_TAKE_THAT.wav", 0x02f69000, 0x00008320, CRC(2e8c837d) SHA1(15a2fac2d5dbe6d3f55de5d07e3713dc24dc5184))
    ROM_LOAD("VO_452_LO_OH_WAS_THAT_YOUR_BALL_MY_APOLOGIES.wav", 0x02f72000, 0x00017da4, CRC(d6c9d501) SHA1(f72e9e5bf4298b5a71a44ef5232c6c67f62da0ba))
    ROM_LOAD("VO_456_LO_I_GROW_TIRED_OF_YOUR_DRIBBLE_ITS_TIME_TO_END_THIS.wav", 0x02f8a000, 0x00016f10, CRC(e97063a2) SHA1(d8738b53b70727c088217338ff93f4a562fd69cf))
    ROM_LOAD("VO_457_LO_YOU_REALLY_HAVE_NO_IDEA_WITH_WHOM_YOU_SPEAK_DO_YOU_MORTAL.wav", 0x02fa1000, 0x0001732e, CRC(a017b341) SHA1(d713f9afdb14a58cdd3782fc575d17707efe5bda))
    ROM_LOAD("VO_458_LO_YOU_THINK_TO_THREATEN_ME.wav", 0x02fb9000, 0x000090b6, CRC(4171dbd3) SHA1(8231d0b0bd876223cc1291b29b0110f07684e13a))
    ROM_LOAD("VO_466_LO_IM_IMPRESSED_I_DIDNT_THINK_YOU_HUMANS_HAD_THE_MENTAL_CA.wav", 0x02fc3000, 0x00023370, CRC(fe6b628d) SHA1(3fd04c3c22ea3fc46e80d89cb6ac5a18caeccaf0))
    ROM_LOAD("VO_471_LO_NO_IT_IS_I_WHO_SHOULD_BE_PINBALL_KING.wav", 0x02fe7000, 0x00019958, CRC(3517845c) SHA1(39578adb03aa3cfe5d632f7578e8c76850ba14f0))
    ROM_LOAD("VO_480_LO_YOU_LACK_THE_SKILL_AND_FINESSE_TO_WIN.wav", 0x03001000, 0x0000d9c4, CRC(493d48d4) SHA1(d12a23b6e33e2e4aa2fb835b2217d76b49829456))
    ROM_LOAD("VO_481_LO_YOURE_MOST_ASSUREDLY_NO_AVENGER.wav", 0x0300f000, 0x00012218, CRC(b1ca0997) SHA1(fd1dcff0daca2d8e24bd47118402c44401e123b9))
    ROM_LOAD("VO_482_LAUGH_7.wav", 0x03022000, 0x000058de, CRC(afb894b3) SHA1(00da45c0d321ee97d4d95bd0ed8d5c61fd7df853))
    ROM_LOAD("VO_483_LO_I_AM_A_GOD_AND_YOU_ARE_BUT_A_HUMAN_WHAT_COULD_YOU_POSSIBLY_HOPE_T.wav", 0x03028000, 0x0001b02c, CRC(5df9edf3) SHA1(fa40e73bbd36ea78c596388a78bcb8b014c9c85d))
    ROM_LOAD("VO_486_LO_OOPS_I_AM_SORRY_WAS_THAT_IN_YOUR_WAY.wav", 0x03044000, 0x0001194a, CRC(904af4eb) SHA1(8a6ebfb2d9f748327f736a40a46093d3e2194b09))
    ROM_LOAD("VO_489_LO_HOW_CAN_YOU_HOPE_TO_DEFEAT_ME.wav", 0x03056000, 0x0000d442, CRC(bbd17a9d) SHA1(28c3e1a1b0fde97ce775796497829e1510f5c934))
    ROM_LOAD("VO_491_LO_YOU_HAVE_WORN_OUT_YOUR_WELCOME.wav", 0x03064000, 0x0000abd4, CRC(e70e7528) SHA1(f9e5d9dc470cb86b6b24ce72ef91c467136e025e))
    ROM_LOAD("VO_497_LO_THIS_IS_LAUGHABLE.wav", 0x0306f000, 0x0000a0fc, CRC(05611137) SHA1(72dad6c47a5396663bc9da04fb6810583099202c))
    ROM_LOAD("VO_501_LO_I_SAID_KNEEEEEEEEEEELLLLLLLLLLLLLLLLL.wav", 0x0307a000, 0x0000e8e2, CRC(66b704fa) SHA1(ea0007f0cb83a7400114bea13750df4e64c2b74a))
    ROM_LOAD("VO_506_LO_NOOOOOOOOOOO.wav", 0x03089000, 0x00006fb8, CRC(a52c44f1) SHA1(a4fd618ca8d2182a3b9954e608474ae2db3d7ab2))
    ROM_LOAD("VO_512_LO_ARE_YOU_EVER_NOT_GOING_TO_FALL_FOR_THAT.wav", 0x03090000, 0x0000c8ea, CRC(31a33de2) SHA1(ab1efda81a293a156d402d71953c3f1e1e04a92c))
    ROM_LOAD("VO_513_LO_OH_TOO_BAD_HOW_AWFUL_IT_MUST_BE_TO_BE_HUMAN.wav", 0x0309d000, 0x00019e06, CRC(5d255d1f) SHA1(8d8f20672828acc48cebfcba6918ac77cfbfa0b7))
    ROM_LOAD("VO_514_LO_THIS_PLEASES_ME.wav", 0x030b7000, 0x0000c4d8, CRC(64e118ec) SHA1(1993108708cb62ee327a7bd1b560afa15eb5677a))
    ROM_LOAD("VO_529_NF_INTELLIGENCE.wav", 0x030c4000, 0x0000966c, CRC(787f7f61) SHA1(1893973339a3dce9be4fbe90cc3fe0f53f2911d0))
    ROM_LOAD("VO_530_NF_PHYSICAL_PROWESS.wav", 0x030ce000, 0x00009b14, CRC(a03d6707) SHA1(11c2526b16f32b9cae4467b3818b06023fed2da6))
    ROM_LOAD("VO_531_NF_POWER_SOURCE.wav", 0x030d8000, 0x00008114, CRC(e7b6706e) SHA1(c6822a5fbfdae755e579434194ee40c0a43435f9))
    ROM_LOAD("VO_532_NF_WEAPONS.wav", 0x030e1000, 0x00006906, CRC(61b1da01) SHA1(71c364841c8a12e0a314056968d8224c1433cf56))
    ROM_LOAD("VO_534_NF_EXPERTISE.wav", 0x030e8000, 0x0000891c, CRC(a4c4458a) SHA1(effb8d8b49d921a8c272ee494f5613709330068d))
    ROM_LOAD("VO_535_NF_NATASHA_ROMANOFF_CODENAME_BLACK_WIDOW.wav", 0x030f1000, 0x000123fe, CRC(3ba23920) SHA1(113a402be324c7013f8a4f4e8d7c51174f0146f2))
    ROM_LOAD("VO_536_NF_SAVATE.wav", 0x03104000, 0x00004480, CRC(14d0aedf) SHA1(b5cccbc05be07282512e1027b39d5f9fa6bca33e))
    ROM_LOAD("VO_537_NF_JUDO.wav", 0x03109000, 0x000048fe, CRC(855240c4) SHA1(80d726a389473923d16c6a5927e3a0359cdbce6a))
    ROM_LOAD("VO_538_NF_NINJUTSU.wav", 0x0310e000, 0x000068a8, CRC(36bd1233) SHA1(cf13d4d240c73e7de340a62940603c3ebb80e235))
    ROM_LOAD("VO_539_NF_AIKIDO.wav", 0x03115000, 0x000067de, CRC(7555d9cd) SHA1(c683dbf0de772f674ed4915e3ff746db17f1c38c))
    ROM_LOAD("VO_540_NF_ESPIONAGE.wav", 0x0311c000, 0x00008a06, CRC(36bf4fd8) SHA1(e649fc7cf8fdcee18eb3506aff5e2ccbb0bcd463))
    ROM_LOAD("VO_541_NF_KARATE.wav", 0x03125000, 0x0000719e, CRC(80838312) SHA1(91b6ad391cf053d457df345b548be48cec536f7b))
    ROM_LOAD("VO_542_NF_BOXING.wav", 0x0312d000, 0x00004eae, CRC(3c25015a) SHA1(da4bbb96d0f653170f57c066106144d8331b1dc2))
    ROM_LOAD("VO_543_NF_KUNG_FU.wav", 0x03132000, 0x00006008, CRC(33be7c54) SHA1(37274f2be5e9b5acd1dd9d779a0432add372e330))
    ROM_LOAD("VO_544_NF_WIDOWS_LINE.wav", 0x03139000, 0x000065de, CRC(b7710b21) SHA1(d57c527092d68900695aa352e5ed99ddf9e17256))
    ROM_LOAD("VO_545_NF_WIDOWS_KISS.wav", 0x03140000, 0x00007902, CRC(153f5a1c) SHA1(14f26e87b99e0d4189b5d4bbb51673daf0c47ac7))
    ROM_LOAD("VO_546_NF_HIGH.wav", 0x03148000, 0x0000355c, CRC(03cff140) SHA1(9fbd20c2bb6e216e182dc4d615dffc2eaf342c88))
    ROM_LOAD("VO_547_NF_ATHLETIC.wav", 0x0314c000, 0x00006c06, CRC(85f3d338) SHA1(62a1fa1ca9b0d12fa8b2950eaaa0e4a0bb3697ef))
    ROM_LOAD("VO_548_NF_WIDOWS_BITE.wav", 0x03153000, 0x0000786e, CRC(19e8afcf) SHA1(11568ac45e9b8d4f6d0d377acb42da820a83054e))
    ROM_LOAD("VO_549_NF_A_TROUBLED_AGENT_WITH_DIVIDED_LOYALTIES.wav", 0x0315b000, 0x00013e74, CRC(afdde698) SHA1(0c152f2829aaa47b501d1b1f4d66617d04f04d65))
    ROM_LOAD("VO_550_NF_RUSSIAN_SPY_TURNED_SHIELD_AGENT.wav", 0x0316f000, 0x0000ef86, CRC(5f7c5e4f) SHA1(6f89a2bc505e9691aee66fcbbc6f8df5379ce5d5))
    ROM_LOAD("VO_551_NF_BLACK_WIDOW.wav", 0x0317e000, 0x000056c6, CRC(bcc16eeb) SHA1(15825cd376b0904b73afdc009a532218f0de4280))
    ROM_LOAD("VO_552_NF_THE_BEAUTIFUL_DEADLY_ASSASSIN.wav", 0x03184000, 0x0000fc7a, CRC(2b6e985c) SHA1(79fde84b2083afcefe8c81b4da81ba3f4b92dbcc))
    ROM_LOAD("VO_553_NF_PEAK_HUMAN_STRENGTH.wav", 0x03194000, 0x00009234, CRC(9c528e77) SHA1(7600217ab63d1837c7ad802a8b43b33c02ec3c76))
    ROM_LOAD("VO_554_NF_STEVE_ROGERS_CODENAME_CAPTAIN_AMERICA.wav", 0x0319e000, 0x00015af4, CRC(51416f48) SHA1(f3926765afbe3b8b28eb0639416092d138734e51))
    ROM_LOAD("VO_556_NF_VAST_MILITARY_KNOWLEDGE.wav", 0x031b4000, 0x0000c84a, CRC(789d53b9) SHA1(87c07c7b6c1740cef7af84883a109f5723b59abf))
    ROM_LOAD("VO_557_NF_COVERT_OPS.wav", 0x031c1000, 0x000092be, CRC(67e657e2) SHA1(bc70f414a670164f6a175c4d48061b9f5a4bb7b4))
    ROM_LOAD("VO_558_NF_SUPERIOR_ENDURANCE.wav", 0x031cb000, 0x0000c840, CRC(863be5b6) SHA1(53fa8d01e2ec9338b7434f9c6e7583d1fa728a07))
    ROM_LOAD("VO_559_NF_CUSTOM_MOTORCYCLE.wav", 0x031d8000, 0x0000a248, CRC(5d3ef4bd) SHA1(769aacdb4408123513e20938ac85f89561934e61))
    ROM_LOAD("VO_561_NF_ABOVE_AVERAGE.wav", 0x031e3000, 0x0000b008, CRC(7d775e82) SHA1(23f25758663d61300d35a1473d194fb4b4d554ef))
    ROM_LOAD("VO_562_NF_INCREDIBLE_AGILITY.wav", 0x031ef000, 0x00009f88, CRC(f203d582) SHA1(c997fda555fb3313af0538cd3c9175115413ee9c))
    ROM_LOAD("VO_563_NF_CUSTOM_BATTLE_VAN.wav", 0x031f9000, 0x0000bc78, CRC(1b5c12b9) SHA1(22044d55a8317e62f31f2e0f40bc653a2a7db5cd))
    ROM_LOAD("VO_564_NF_VIBRANIUM_SHIELD.wav", 0x03205000, 0x0000a6f0, CRC(8026a56f) SHA1(d0d52e8ec9ca6fd6f7335d055bc31c46ff145e75))
    ROM_LOAD("VO_565_NF_CAPTAIN_AMERICA.wav", 0x03210000, 0x00009d6a, CRC(01d8d217) SHA1(3c5a567f46680f2e786f3a20557d395e2cd2b4eb))
    ROM_LOAD("VO_566_NF_WORLD_WAR_2_HERO_DISPLACED_IN_TIME.wav", 0x0321a000, 0x00012e2c, CRC(e6d200bd) SHA1(36a8c3cb10c98c4c3db15174ed156f99379a4c38))
    ROM_LOAD("VO_567_NF_A_BRILLIANT_BATTLEFIELD_TACTICIAN.wav", 0x0322d000, 0x0000efd4, CRC(899b0c48) SHA1(76a3a1fad27ab26d920f94e96b33363ab237360c))
    ROM_LOAD("VO_568_NF_THE_LIVING_LEGEND.wav", 0x0323c000, 0x00009060, CRC(88c5aadc) SHA1(cf709fa21e2e024f332c48e7bf02b2ceb3860505))
    ROM_LOAD("VO_569_NF_THE_SUPER_SOLDIER.wav", 0x03246000, 0x00007f92, CRC(a9036c6d) SHA1(b899191ca7d845e9045a8b1a9160611621952096))
    ROM_LOAD("VO_571_NF_CLINT_BARTON_CODENAME_HAWKEYE.wav", 0x0324e000, 0x00010f1c, CRC(b41c2b82) SHA1(696753f1d358e96920086cd5eda1571ab40471a2))
    ROM_LOAD("VO_572_NF_NUMEROUS_TRICK_ARROWS.wav", 0x0325f000, 0x0000c38e, CRC(19ae696b) SHA1(0a5ec89e100b526c7be0d28983571058be90e411))
    ROM_LOAD("VO_573_NF_GRANDMASTER_MARKSMAN.wav", 0x0326c000, 0x0000893e, CRC(849f82c0) SHA1(740c4327e291ff3f96ec0ec8913ddedbb7728c04))
    ROM_LOAD("VO_574_NF_DRAW_WEIGHT_BOW.wav", 0x03275000, 0x0000788e, CRC(3edf6cdf) SHA1(d1ff4125c022bb5117709ed05171307f236723be))
    ROM_LOAD("VO_575_NF_COVERT_OPS.wav", 0x0327d000, 0x00008a92, CRC(28cf7bf8) SHA1(adf9d3e102321de39c4efa23233b24a501e17b84))
    ROM_LOAD("VO_577_NF_AVERAGE.wav", 0x03286000, 0x00006b78, CRC(2d6ae9b1) SHA1(c16c7056821ac01b73d3d7307dde50b1251dc716))
    ROM_LOAD("VO_578_NF_NEVER_MISSES.wav", 0x0328d000, 0x00007eae, CRC(3505d7c9) SHA1(5b151a09184577411de52d4c20a32858ad8b4bb4))
    ROM_LOAD("VO_579_NF_A_SOLITARY_AGENT_WHO_HAS_TROUBLE_FOLLOWING_ORDERS.wav", 0x03295000, 0x000152c2, CRC(da3aca9d) SHA1(1c207d99f327b66e128acaa1b9df3acd5f3dba30))
    ROM_LOAD("VO_580_NF_THE_EVER_VIGILANT_MARKSMAN.wav", 0x032ab000, 0x0000ac7e, CRC(cd541fbb) SHA1(0e80e4e03119679f60a5c910bd6197fa7451a9ed))
    ROM_LOAD("VO_581_NF_HAWKEYE.wav", 0x032b6000, 0x000056a4, CRC(01043d73) SHA1(19f8b7c81743da0f24bec6b24a5bed4cfb087d76))
    ROM_LOAD("VO_582_NF_A_SPECIALIST_IN_MARKSMANSHIP_AND_MARTIAL_ARTS.wav", 0x032bc000, 0x00010bfc, CRC(e801b8dc) SHA1(c6a903fa56d5fa6b2a940ed84f6334623ba336c2))
    ROM_LOAD("VO_583_NF_GENIUS_BILLIONAIRE_PLAYBOY_PHILANTHROPIST.wav", 0x032cd000, 0x00013e14, CRC(9f6b0be1) SHA1(cde52b627e0eb0f57306f6ca2079806f38ada1c7))
    ROM_LOAD("VO_585_NF_TONY_STARK_CODENAME_IRONMAN.wav", 0x032e1000, 0x00011a90, CRC(03e55cff) SHA1(a5e8bf995684fa66256083825a97c2e4c485a719))
    ROM_LOAD("VO_586_NF_ADVANCED_WEAPONRY.wav", 0x032f3000, 0x00009950, CRC(5253188c) SHA1(becf77b79d87a7b19694ad1245169a0595e76b5b))
    ROM_LOAD("VO_587_NF_ROBOTICS.wav", 0x032fd000, 0x00006ac8, CRC(6cfccbd8) SHA1(dee67e2d14a7283abaa1da88585fea70b05c3bcf))
    ROM_LOAD("VO_588_NF_ARTIFICIAL_INTELLIGENCE.wav", 0x03304000, 0x0000b97a, CRC(d825ce1d) SHA1(1d66884ff629ebe68b0f709a748039c9df96b849))
    ROM_LOAD("VO_589_NF_CHEMISTRY.wav", 0x03310000, 0x000053fa, CRC(8384dd7d) SHA1(24a01fb06eb8ba30b38cd34fcbf1ac4d04cf401a))
    ROM_LOAD("VO_590_NF_PHYSICS.wav", 0x03316000, 0x00005cb0, CRC(4119cb68) SHA1(7e71c4ac47f6d664339fb2f6518ef270393fcdc4))
    ROM_LOAD("VO_591_NF_ENGINEERING.wav", 0x0331c000, 0x00005a5c, CRC(ec8a341f) SHA1(317689664f30de0a5018afb46a1fb2a0840604e6))
    ROM_LOAD("VO_592_NF_GENIUS_LEVEL.wav", 0x03322000, 0x0000687a, CRC(9c235757) SHA1(fbbe984fa74669f9e8c01fad3bbfa0ee26626eeb))
    ROM_LOAD("VO_593_NF_UNI_BEAM.wav", 0x03329000, 0x00007e42, CRC(6ef099d1) SHA1(fa9a35d2412aedb78f5ea389b7d9479d1940f963))
    ROM_LOAD("VO_594_NF_ARC_REACTOR_POWERED.wav", 0x03331000, 0x0000ad14, CRC(f7487c4b) SHA1(3f9681d2e9f9bb2f8138c202e20ff61fd8f26d3a))
    ROM_LOAD("VO_596_NF_REPULSER_BLASTS.wav", 0x0333c000, 0x0000852c, CRC(4e700bab) SHA1(2a47634e6eb69dc23b5078c07ca0397a90e12c7e))
    ROM_LOAD("VO_597_NF_HE_DOESNT_PLAY_WELL_WITH_OTHERS.wav", 0x03345000, 0x0000bbaa, CRC(9144201b) SHA1(8639998ef960d4e73f8979b871451d1c9733dbaf))
    ROM_LOAD("VO_598_NF_A_VOLATILE_SELF_ABSORBED_RICH_BOY_WITH_THE_MOST_P.wav", 0x03351000, 0x00020bcc, CRC(8efdbd0a) SHA1(d8916aaf945e972a1a13f57f84852754a610337e))
    ROM_LOAD("VO_599_NF_A_LEADER_IN_THE_FIELD_OF_ARTIFICIAL_ROBOTICS.wav", 0x03372000, 0x00012548, CRC(392de204) SHA1(acaeeb0c94cffac6ac51ca25745a07831e6636b6))
    ROM_LOAD("VO_600_NF_A_VISIONARY_IN_WEAPONS.wav", 0x03385000, 0x0000a464, CRC(cf4ad2a3) SHA1(d43c0670b7c4b6fad08f0888a84df173f3814e6c))
    ROM_LOAD("VO_601_NF_A_VISIONARY_IN_ENGINEERING.wav", 0x03390000, 0x0000b44c, CRC(3895bce4) SHA1(a8d692e8e8b804239437db7768524b2194ab1ffe))
    ROM_LOAD("VO_602_NF_A_LEADER_IN_THE_FIELD_OF_ARTIFICIAL_INTELLIGENCE.wav", 0x0339c000, 0x0000fd64, CRC(0d0c1b55) SHA1(d496608bd2b025d1005d7943877e23eeb7466882))
    ROM_LOAD("VO_603_NF_TONY_STARK.wav", 0x033ac000, 0x00006644, CRC(7a63f7a7) SHA1(57a40d87852b76fdf1a77a6533875751526620c5))
    ROM_LOAD("VO_624_NF_UNARMED_COMBAT.wav", 0x033b3000, 0x00009344, CRC(72315fe5) SHA1(0e7f7ec8e9fd3b9ba730deff8a9c851da10b47f4))
    ROM_LOAD("VO_625_NF_MYSTICAL_ARTS.wav", 0x033bd000, 0x00009504, CRC(86323b19) SHA1(890803d27172bda33ef3500bfef5293de810f17d))
    ROM_LOAD("VO_626_NF_MELEE_WEAPONS.wav", 0x033c7000, 0x0000819c, CRC(369a88c1) SHA1(0879276f951c323fea42026a66f688500ab97ba5))
    ROM_LOAD("VO_626_NF_MYSTICAL_WEAPONS.wav", 0x033d0000, 0x00009fde, CRC(57c12aaa) SHA1(178347db0a2b5d0e81e6d96c203385b58cee481c))
    ROM_LOAD("VO_627_NF_WIND.wav", 0x033da000, 0x00004228, CRC(728b9014) SHA1(720280254d3358c0b18eb1b7665b4ff3bd9dd6c0))
    ROM_LOAD("VO_630_NF_STORMS.wav", 0x033df000, 0x000075f2, CRC(2e3a8a54) SHA1(a1bf2426f22803f5bedb04223571e3432b52f61e))
    ROM_LOAD("VO_631_NF_LIGHTNING.wav", 0x033e7000, 0x00007112, CRC(032c69ac) SHA1(562da7351c953412307332bda3f371291df5dc49))
    ROM_LOAD("VO_632_NF_ENORMOUS_STRENGTH.wav", 0x033ef000, 0x00009496, CRC(99d31948) SHA1(fc6bd7f25478101f289c19fafcebafa1bba63201))
    ROM_LOAD("VO_634_NF_THOR.wav", 0x033f9000, 0x00003d0a, CRC(27a3aee9) SHA1(e8b87cbe01132857bd1056e7162ad821979c430c))
    ROM_LOAD("VO_636_NF_AN_ASGARDIAN_WITH_UNPARALLELED_STRENGTH.wav", 0x033fd000, 0x00011298, CRC(1796cacb) SHA1(b6e5b292ba85c68974dd28075f8d12abedd147b5))
    ROM_LOAD("VO_650_NF_SUPER_JACKPOT.wav", 0x0340f000, 0x000097b6, CRC(69aa3841) SHA1(b3de467fe3f1f60b743a24429840158f7a0c26ec))
    ROM_LOAD("VO_667_NF_AVENGERS_ASSEMBLE.wav", 0x03419000, 0x00007d5e, CRC(6cb99862) SHA1(b3067ecd2a96cdfb8b2d6c92f63e93dae8180da6))
    ROM_LOAD("VO_687_NF_SKILL_SHOT.wav", 0x03421000, 0x00006b32, CRC(459d5116) SHA1(b713b4f0b382a9827788d1c402205029c129b448))
    ROM_LOAD("VO_718_TO_THE_CHITAURI_GROW_RESTLESS.wav", 0x03428000, 0x00010dde, CRC(c941b787) SHA1(1cf23d6b9dddae1bb6d6f9599246f922756567ff))
    ROM_LOAD("VO_745_TR_THE_TESSERACT_BELONGS_ON_ASGARD.wav", 0x03439000, 0x0000b22c, CRC(4e11b836) SHA1(7e30d42c08c82caa78a914b2b92488680c08d419))
    ROM_LOAD("VO_747_TR_YOU_SPEAK_OF_CONTROL_YET_YOU_COURT_CHAOS.wav", 0x03445000, 0x000108c4, CRC(908a932c) SHA1(40e75eff7c41d8e3590c10076cedc9220182cfbc))
    ROM_LOAD("VO_748_TR_I_THOUGHT_HUMANS_WERE_MORE_EVOLVED_THEN_THIS.wav", 0x03456000, 0x0000fb04, CRC(7132d163) SHA1(738a1f48d2972703fa8ef6fd97a17c6306e37bfc))
    ROM_LOAD("VO_751_TR_FOR_ASGARD_1.wav", 0x03466000, 0x00006eda, CRC(e4fcdba7) SHA1(88ea66b57618ad55c15b92e2c5b04ccea92fceec))
    ROM_LOAD("VO_752_TR_AVENGERS_ASSEMBLE.wav", 0x0346d000, 0x0000d104, CRC(bd3fb320) SHA1(37b77f545214fd0985f5da1cf704ef878390d107))
    ROM_LOAD("VO_779_TR_WHAT_MANNER_OF_TRICKERY_OR_SORCERY_IS_THIS.wav", 0x0347b000, 0x000125c8, CRC(baca1eb6) SHA1(47c6f8f4dff1062e0ea53882d8317ae2b9513f4b))
    ROM_LOAD("VO_781_TR_BY_ODINS_BEARD_WHAT_IS_THAT_FLYING_CONTRAPTION.wav", 0x0348e000, 0x00016bf8, CRC(733362af) SHA1(4d22632f42f48ff94851d5760b7bd4104ee2da2c))
    ROM_LOAD("VO_786_TR_I_WILL_DEAL_WITH_THESE_VILLAINS.wav", 0x034a5000, 0x0000a7c4, CRC(b46c3d91) SHA1(7100706e1e48ec34a492973fba4256c39c88172d))
    ROM_LOAD("VO_787_TR_I_HAVE_UNFINISHED_BUSINESS_WITH_LOKI.wav", 0x034b0000, 0x0000d50c, CRC(bc3c7ec0) SHA1(5010279e626f75d0a6cf8e1af7a0db4e850c7f72))
    ROM_LOAD("VO_790_TR_MIGHTY_BONUS.wav", 0x034be000, 0x000080ca, CRC(7f3ab6fc) SHA1(0113bee56e1afad4df98b1944c484027d6153bb9))
    ROM_LOAD("VO_791_TR_TO_ME_MY_BONUS.wav", 0x034c7000, 0x000096b4, CRC(1ea7c1a6) SHA1(224e6649394a5942a756aad654aacc79715c4615))
    ROM_LOAD("VO_BW_FX_03.wav", 0x034d1000, 0x00003b84, CRC(56ad352d) SHA1(576ddb09845052daa03d8c3b12ae9c7e795bc51f))
    ROM_LOAD("VO_HL_FX__05.wav", 0x034d5000, 0x0000486a, CRC(7adb2fee) SHA1(9076dff89785fcd7bf3120ef2a5f81eb7b5730bc))
    ROM_LOAD("VO_HL_FX__06.wav", 0x034da000, 0x000043f6, CRC(8fdfb843) SHA1(c406117795707fdf379fd069fb9289f145201e9f))
    ROM_LOAD("VO_HL_FX__13.wav", 0x034df000, 0x00003ffe, CRC(2d231500) SHA1(19de32da4ec3fd2c76bf85e2ef2a7e8f138c4b0d))
    ROM_LOAD("VO_TO_COSMIC_1.wav", 0x034e3000, 0x000078bc, CRC(0de4296a) SHA1(ecd9d68877c03f94b713fa98e92a80bd6a980af6))
    ROM_LOAD("VO_TO_COSMIC_CUBE_2.wav", 0x034eb000, 0x0000a938, CRC(ec103116) SHA1(bc9e5137360591eff43d60cd67b975455c92e5e1))
    ROM_LOAD("VO_TO_IT_AWAKENS_BUT_HOW.wav", 0x034f6000, 0x00014ae0, CRC(3f46ba96) SHA1(6ee4a0875cdce843991e916ed8b27974c476a980))
    ROM_LOAD("VO_TO_NOOOOOOO.wav", 0x0350b000, 0x000079f6, CRC(a2720699) SHA1(3271f8968a73e056b226c834796a3b8955e1010a))
    ROM_LOAD("VO_TO_PATIENCE_CHITAURI_YOUR_TIME_WILL_COME.wav", 0x03513000, 0x0001b874, CRC(c7e604e6) SHA1(7aa54e648f937cdcc681d6f33394db29dfba070d))
    ROM_LOAD("VO_TO_SOON_THE_TESSERACTS_POWER_WILL_BE_OURS.wav", 0x0352f000, 0x0001e06e, CRC(75624f35) SHA1(94faa37a3270a9917c0d1b15d15777b7000b6adb))
    ROM_LOAD("VO_TO_THAT_WAS_ALL_LUCK_HUMAN.wav", 0x0354e000, 0x0001602a, CRC(5df61b63) SHA1(b363cc49a9eac9862f75bdee58fd6877df86242f))
    ROM_LOAD("VO_TO_THATS_IMPOSSIBLE_NO_HUMAN_CAN_CONTROL_THE_TES.wav", 0x03565000, 0x00020cd0, CRC(e6ccd122) SHA1(386fa7b505c2b67f55964d9e98e4128ea236703a))
    ROM_LOAD("VO_TO_THE_HUMANS_HAVE_ACTIVATED_THE_TESSERACT.wav", 0x03586000, 0x00011d5a, CRC(5b16926c) SHA1(016f91e6f13db3cd0e4d9fde54586bf5e1524b95))
    ROM_LOAD("VO_TO_THE_HUMANS_WILL_NEVER_CONTROL_THE_TESSERACTS_.wav", 0x03598000, 0x0001a65e, CRC(a9a546b9) SHA1(86fb1e7ed128e08b28c9e4a821a335939f6de4a4))
    ROM_LOAD("VO_TO_THE_INVASION_WILL_PROCEED_MY_LORD.wav", 0x035b3000, 0x00018fb6, CRC(e2059dbf) SHA1(948c0901a8d0e3298e2f780de3d0d7fd47da49f9))
    ROM_LOAD("VO_TO_THE_TESSERACT_2.wav", 0x035cc000, 0x0000a2ec, CRC(7a866b20) SHA1(47cbed21d0df6be5be8559d5db6edf42ce00fc6f))
    ROM_LOAD("VO_TO_THE_TESSERACT_IS_ACTIVE.wav", 0x035d7000, 0x0000ee38, CRC(f72b8459) SHA1(9f8852963d716aac79b9d842dcd08e84bcd88cba))
    ROM_LOAD("VO_TO_THIS_DOES_NOT_BODE_WELL.wav", 0x035e6000, 0x00011a24, CRC(894d62aa) SHA1(9338e105a2d7d5c3fab39b9d7ca5098422655207))
    ROM_LOAD("VO_TO_WE_MUST_STRIKE_NOW_BEFORE_THE_HUMANS_HARNESS_.wav", 0x035f8000, 0x0001ef06, CRC(79778c36) SHA1(db45455f694ca093dbb594e0a10cc9fecccdec7e))
    ROM_LOAD("VO_TO_WE_WILL_NOT_ALLOW_YOU_TO_CONTROL_ITS_POWER.wav", 0x03617000, 0x00019eb0, CRC(0b04c774) SHA1(004dcdef62cffd2780897e4b74204c39ae1e1d24))
    ROM_LOAD("VO_TO_WHAT_HOW_DID_YOU_ACTIVATE_IT.wav", 0x03631000, 0x0001c44a, CRC(135a5019) SHA1(08c75d10d81f14c043235f9332dff1baf5e67f0b))
    ROM_LOAD("VO_TO_YOU_ARE_NOT_THE_WEAKLING_WE_EXPECTED.wav", 0x0364e000, 0x0001f2e6, CRC(3628a2c3) SHA1(e65050cf0b19f84944bdbcf8b68b6c4cc3c87048))
    ROM_LOAD("VO_TO_YOU_KNOW_NOTHING_OF_THE_TESSERACTS_TRUE_POWER.wav", 0x0366e000, 0x00018068, CRC(3b90c007) SHA1(43abfa0ecdd93afe0a35ea0cbb8ae785afd72ada))
ROM_END
CORE_GAMEDEF(avscp, 103, "Avengers The Pin (1.0.3)", 2013, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ (Legends of) Wrestlemania
/-------------------------------------------------------------------*/
/* The oldest SPIKE 1 SDK. Its playfield LCD is the CPU board's own 320x240 screen, the second
   display; the clips it plays are the folder's video files (.spv, RC4-encrypted), which the set
   holds beside the others and the subsystem puts in video/. Each flipper button has a second leaf
   (74, 75) for the upper flipper (solenoid 22) */
SPIKE1_INIT_EX(wwe, "WWE_LE", spike1_dmd_lcd, 102, FLIP_L | FLIP_U, SPIKE1_SWITCHES_UPPER(10, 11, 65, 66, 74, 75, 67, 68, 69, 70, 72, 77), SPIKE1_NOHOMES, SPIKE1_NOLIMIT)
ROM_START(wwe_135h)
  ROM_REGION(0xaae47000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x005685c2, CRC(edc2c801) SHA1(23e2936810e3869d52d39bc3ff7863d9de59951b))
    ROM_LOAD("image.bin", 0x00569000, 0x547598dc, CRC(aa528057) SHA1(f513c5afd07cee4bba97b38822f98943857dc824))
    ROM_LOAD("AJ.spv", 0x54cc3000, 0x008ca1fc, CRC(6fe0fc26) SHA1(2bc3a9b2d149416c421b9778f8f981e96be0b9b9))
    ROM_LOAD("BELLA_TWINS.spv", 0x5558e000, 0x008ca1fc, CRC(5ca08b63) SHA1(0250a7b67af1143643f2235ebb613f52b1687c3c))
    ROM_LOAD("BLANK_FRAME.spv", 0x55e59000, 0x00025824, CRC(19340b39) SHA1(a1dced3d232eefa7b5ee355de58c117a584c2601))
    ROM_LOAD("CENA_BG_FPS20.spv", 0x55e7f000, 0x01588cb4, CRC(c516caca) SHA1(4a0e80d34b34ecebf032fa2e508e1fa50e145dc1))
    ROM_LOAD("CENA_INTRO_FPS20.spv", 0x57408000, 0x059a733c, CRC(fefaf73d) SHA1(eb7413d6d4bfc3f499caa2ffe80531f594076540))
    ROM_LOAD("coil4node-LPC1112_101-0_18_4.hex", 0x5cdb0000, 0x000086b3, CRC(0db5fef9) SHA1(a1d1be1ab95c07ffa1bde5295047b1e184b358e8))
    ROM_LOAD("coil4node-LPC1112_201-0_18_4.hex", 0x5cdb9000, 0x000086b3, CRC(0d97d36b) SHA1(63d1740e475faf5b1a6edd082713f71797d8518f))
    ROM_LOAD("coil4node-LPC1313-0_18_4.hex", 0x5cdc2000, 0x00009910, CRC(eda97e7a) SHA1(c25649ea0a0d832f259c97a0cace84d22668eabe))
    ROM_LOAD("DB_BG_FPS20.spv", 0x5cdcc000, 0x01588cb4, CRC(dadcc332) SHA1(a283a50b9b135f57c73582ecbca89a5e0f9a5f5f))
    ROM_LOAD("DB_INTRO_FPS20.spv", 0x5e355000, 0x0a95843c, CRC(1c0764df) SHA1(f32d322fe7b7c540b33d9fad9f00cf66ee44c67b))
    ROM_LOAD("FIREWORKS_FPS20.spv", 0x68cae000, 0x008ca1fc, CRC(4a85e806) SHA1(218ed67c6604322c6a1f58c903a67da3ff26b5fe))
    ROM_LOAD("HBK_BG_FPS20.spv", 0x69579000, 0x01588cb4, CRC(32e76255) SHA1(87d06190e814e1c5cb5dd1328f3360e58f12372e))
    ROM_LOAD("HBK_INTRO_FPS20.spv", 0x6ab02000, 0x0b9c07bc, CRC(cf51844f) SHA1(df10d2f302f0a0f1a5cf223907900579a36035aa))
    ROM_LOAD("HHH_BG_FPS20.spv", 0x764c3000, 0x01588cb4, CRC(a535703f) SHA1(7b8f9fd3d9179a079920dbf1c48368ef8fd10d55))
    ROM_LOAD("HHH_INTRO_FPS20.spv", 0x77a4c000, 0x064c959c, CRC(2ba4c0e4) SHA1(84849d6b4faafdf23546eb243bd3e5c24f25e630))
    ROM_LOAD("HOGAN_BG_FPS20.spv", 0x7df16000, 0x01588cb4, CRC(7fd2b05f) SHA1(8c0ea36071553a06eccaef4651c229822468f5c4))
    ROM_LOAD("HOGAN_INTRO_FPS20.spv", 0x7f49f000, 0x099d10ec, CRC(bfffb34e) SHA1(a175c2e1b8daaa19765b874ce8ca384d043c51f0))
    ROM_LOAD("KANE.spv", 0x88e71000, 0x005dc15c, CRC(d0d04329) SHA1(ffeafbc4bec80367c4d4ca90c727cc59f6080870))
    ROM_LOAD("lcdnode-LPC1113_302-0_18_4.hex", 0x8944e000, 0x00009783, CRC(b9e6387e) SHA1(cc5b609d967d2b587a2fe297b2409c368696e7f0))
    ROM_LOAD("LEGENDS_FPS20.spv", 0x89458000, 0x0070819c, CRC(d45c415f) SHA1(893358a533fecd9b0da79335048cb1d32bdf8fb5))
    ROM_LOAD("LEGION_OF_DOOM.spv", 0x89b61000, 0x008ca1fc, CRC(3031bab7) SHA1(2d85188ca39b67d4c08ffd53aedb6564019f7ae6))
    ROM_LOAD("MAIN_EVENT.spv", 0x8a42c000, 0x008ca1fc, CRC(9386f7d6) SHA1(5f162af60eb8ba6ff441c91027df3607ccdd9d21))
    ROM_LOAD("pinnode-LPC1112_101-0_18_4.hex", 0x8acf7000, 0x00008005, CRC(72dd8621) SHA1(66e6693e1bdd570db5eee4218f36a9153da760ff))
    ROM_LOAD("pinnode-LPC1112_201-0_18_4.hex", 0x8ad00000, 0x0000802a, CRC(8a0e089c) SHA1(e41ade3256e4cc7cfcaece06a5e1035c7ff2d026))
    ROM_LOAD("pinnode-LPC1313-0_18_4.hex", 0x8ad09000, 0x0000a082, CRC(0e9790a7) SHA1(9bafcdde92f5eecccd18e43f4eaf0a20a813a611))
    ROM_LOAD("RAW.spv", 0x8ad14000, 0x008ca1fc, CRC(7c19cd49) SHA1(43b25425622cb269798bc4d12a3fa600647e1216))
    ROM_LOAD("ROCK_BG_FPS20.spv", 0x8b5df000, 0x01588cb4, CRC(8ed98eb7) SHA1(d2877ea40166bdc163746e65af091cc3fbe4312d))
    ROM_LOAD("ROCK_INTRO_FPS20.spv", 0x8cb68000, 0x06c6773c, CRC(3a667503) SHA1(76d1f34eb816e6e759ca2877316659cc7b6a998e))
    ROM_LOAD("ROYAL_RUMBLE.spv", 0x937d0000, 0x00a4124c, CRC(d549883a) SHA1(765a859973d7b9c2036d2c55288c842615b8e793))
    ROM_LOAD("SCSA_BG_FPS20.spv", 0x94212000, 0x01588cb4, CRC(9230922c) SHA1(5a9a303b41dd74164ff55b09d8463e257f3052ab))
    ROM_LOAD("SCSA_INTRO_FPS20.spv", 0x9579b000, 0x0668b5fc, CRC(42a33cce) SHA1(e5828f2656c2057b602df50f329bcb4364c6fe5c))
    ROM_LOAD("TAG_TEAM.spv", 0x9be27000, 0x005dc15c, CRC(f90ff90f) SHA1(c1c5d5b77d2f17131a394d2d25faa6a2153c7dfe))
    ROM_LOAD("UNDERTAKER_BG_FPS20.spv", 0x9c404000, 0x01588cb4, CRC(69dd7575) SHA1(d90b1da92106d2947eafcd8072250ffd7f2753e7))
    ROM_LOAD("UNDERTAKER_INTRO_FPS20.spv", 0x9d98d000, 0x070f2034, CRC(f513bc4f) SHA1(fedb1a69f0b4d9a7b28b0b5aef23228f1c7f7fe3))
    ROM_LOAD("US_CHAMPIONS.spv", 0xa4a80000, 0x011943dc, CRC(7a004855) SHA1(d056ebe16513e8ff2a1f4a5bdcaa8d6386329e71))
    ROM_LOAD("WORLD_HEAVYWEIGHT.spv", 0xa5c15000, 0x011943dc, CRC(4fd67f81) SHA1(3bb7366a4e2908782a5f34301758b20ede7c6826))
    ROM_LOAD("WORLD_INTERCONTINENTAL.spv", 0xa6daa000, 0x0148247c, CRC(72ea6cc2) SHA1(e66895e0a9c335567c685535a4d5108002af0f43))
    ROM_LOAD("WRESTLEMANIALOGO.spv", 0xa822d000, 0x00d2f2ec, CRC(e387938a) SHA1(449fe9ac64c59886af50f8f285294eedb74be401))
    ROM_LOAD("WWE_SHATTERLOGO_FPS20.spv", 0xa8f5d000, 0x015f94cc, CRC(8be9e928) SHA1(eaa6fda36d57396639cc4a25deabd39c25292cc2))
    ROM_LOAD("WWELOGO_FPS20.spv", 0xaa557000, 0x008efa04, CRC(0b72c2aa) SHA1(031082beaae636e21e5adb69fa642b9a7ec27bcf))
ROM_END
CORE_GAMEDEF(wwe, 135h, "Legends of Wrestlemania Limited Edition (1.35.0)", 2015, "Stern", spike1_lcd, 0)

/*-------------------------------------------------------------------
/ Whoa Nellie! Big Juicy Melons
/-------------------------------------------------------------------*/
/* The DMD output drives the small LCD in the apron. The score and credit reels in the backbox are
   steppers the node board runs: mech 0-4 are the 1000s, 100s, 10s and 1s reels and the credit reel,
   each as its step (0-199, 20 steps a digit; the credit reel 10 a step). The bells and the knocker are coils */
SPIKE1_INIT(wnbjm, "WN", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(wnbjm_155)
  ROM_REGION(0x10bb7000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x004115ac, CRC(67df0775) SHA1(fe018c240e4834abd07b3efed1d47d67017910c5))
    ROM_LOAD("image.bin", 0x00412000, 0x10756444, CRC(9c10415b) SHA1(74fc69297540ae4a49e90663293c9c867150eea3))
    ROM_LOAD("coil4node-LPC1112_101-0_28_0.hex", 0x10b69000, 0x00008487, CRC(29d48577) SHA1(2cef71c04de9d2ff8c4664fafaeab077532f6178))
    ROM_LOAD("coil4node-LPC1112_201-0_28_0.hex", 0x10b72000, 0x00008487, CRC(19aac926) SHA1(c53e6f2fe65d68a841b23a79ad35dc3422da3e6b))
    ROM_LOAD("coil4node-LPC1313-0_28_0.hex", 0x10b7b000, 0x0000c23a, CRC(e019995b) SHA1(7d09cd36e12e79c9545e8f32e94f738951e45468))
    ROM_LOAD("lcdnode-LPC1113_302-0_28_0.hex", 0x10b88000, 0x0000b484, CRC(d139cac7) SHA1(e4a57e712a599041f7ba7dfcf08f07a724ccbd9f))
    ROM_LOAD("pinnode-LPC1112_101-0_28_0.hex", 0x10b94000, 0x00007f1c, CRC(83ca12dc) SHA1(2ac8f8c9384e8c5e0bbc1ca2d9f2ddd8bb831d3d))
    ROM_LOAD("pinnode-LPC1112_201-0_28_0.hex", 0x10b9c000, 0x00007f66, CRC(cee92fdc) SHA1(048abb5d42962204595d7c311dd351c9cb25fd6c))
    ROM_LOAD("pinnode-LPC1313-0_28_0.hex", 0x10ba4000, 0x0000c859, CRC(18c17d1d) SHA1(6b2f232c0b6c2389c59f998d0487081491e9d177))
    ROM_LOAD("ws2812node-LPC1313-0_28_0.hex", 0x10bb1000, 0x00005590, CRC(921dc0f7) SHA1(2c49842bb5163b75a19ebac37f0379ef72c64d4a))
ROM_END
CORE_GAMEDEF(wnbjm, 155, "Whoa Nellie! Big Juicy Melons (1.55.0)", 2015, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Pabst Can Crusher, The
/-------------------------------------------------------------------*/
/* Same as Whoa Nellie's hardware - the apron LCD, the score and credit reels as mechs 0-4, the bells - and its switch numbers */
SPIKE1_INIT(pabst, "can_crusher", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(pabst_101)
  ROM_REGION(0x39d5f000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x004122e1, CRC(0a114c42) SHA1(f1179042ab49190e506d93d66028bd5404ba19f8))
    ROM_LOAD("image.bin", 0x00413000, 0x398fd2c4, CRC(424bb457) SHA1(4289990fc4991803fa72cb76508caa3e1f6402f4))
    ROM_LOAD("coil4node-LPC1112_101-0_28_0.hex", 0x39d11000, 0x00008487, CRC(29d48577) SHA1(2cef71c04de9d2ff8c4664fafaeab077532f6178))
    ROM_LOAD("coil4node-LPC1112_201-0_28_0.hex", 0x39d1a000, 0x00008487, CRC(19aac926) SHA1(c53e6f2fe65d68a841b23a79ad35dc3422da3e6b))
    ROM_LOAD("coil4node-LPC1313-0_28_0.hex", 0x39d23000, 0x0000c23a, CRC(e019995b) SHA1(7d09cd36e12e79c9545e8f32e94f738951e45468))
    ROM_LOAD("lcdnode-LPC1113_302-0_28_0.hex", 0x39d30000, 0x0000b484, CRC(d139cac7) SHA1(e4a57e712a599041f7ba7dfcf08f07a724ccbd9f))
    ROM_LOAD("pinnode-LPC1112_101-0_28_0.hex", 0x39d3c000, 0x00007f1c, CRC(83ca12dc) SHA1(2ac8f8c9384e8c5e0bbc1ca2d9f2ddd8bb831d3d))
    ROM_LOAD("pinnode-LPC1112_201-0_28_0.hex", 0x39d44000, 0x00007f66, CRC(cee92fdc) SHA1(048abb5d42962204595d7c311dd351c9cb25fd6c))
    ROM_LOAD("pinnode-LPC1313-0_28_0.hex", 0x39d4c000, 0x0000c859, CRC(18c17d1d) SHA1(6b2f232c0b6c2389c59f998d0487081491e9d177))
    ROM_LOAD("ws2812node-LPC1313-0_28_0.hex", 0x39d59000, 0x00005590, CRC(921dc0f7) SHA1(2c49842bb5163b75a19ebac37f0379ef72c64d4a))
ROM_END
CORE_GAMEDEF(pabst, 101, "Pabst Can Crusher, The (1.01.0)", 2016, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Primus
/-------------------------------------------------------------------*/
/* Same as Whoa Nellie's hardware - the apron LCD, the score and credit reels as mechs 0-4, the bells - and its switch numbers */
SPIKE1_INIT(primus, "primus", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(primus_103)
  ROM_REGION(0x2a4d2000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x0041372a, CRC(017178e5) SHA1(eae72f6251af2f4c7381c93c3ee1d48388c37967))
    ROM_LOAD("image.bin", 0x00414000, 0x2a06f114, CRC(cb499566) SHA1(d0a4b05a93ea9b73d6796393ce128df407766787))
    ROM_LOAD("coil4node-LPC1112_101-0_28_0.hex", 0x2a484000, 0x00008487, CRC(29d48577) SHA1(2cef71c04de9d2ff8c4664fafaeab077532f6178))
    ROM_LOAD("coil4node-LPC1112_201-0_28_0.hex", 0x2a48d000, 0x00008487, CRC(19aac926) SHA1(c53e6f2fe65d68a841b23a79ad35dc3422da3e6b))
    ROM_LOAD("coil4node-LPC1313-0_28_0.hex", 0x2a496000, 0x0000c23a, CRC(e019995b) SHA1(7d09cd36e12e79c9545e8f32e94f738951e45468))
    ROM_LOAD("lcdnode-LPC1113_302-0_28_0.hex", 0x2a4a3000, 0x0000b484, CRC(d139cac7) SHA1(e4a57e712a599041f7ba7dfcf08f07a724ccbd9f))
    ROM_LOAD("pinnode-LPC1112_101-0_28_0.hex", 0x2a4af000, 0x00007f1c, CRC(83ca12dc) SHA1(2ac8f8c9384e8c5e0bbc1ca2d9f2ddd8bb831d3d))
    ROM_LOAD("pinnode-LPC1112_201-0_28_0.hex", 0x2a4b7000, 0x00007f66, CRC(cee92fdc) SHA1(048abb5d42962204595d7c311dd351c9cb25fd6c))
    ROM_LOAD("pinnode-LPC1313-0_28_0.hex", 0x2a4bf000, 0x0000c859, CRC(18c17d1d) SHA1(6b2f232c0b6c2389c59f998d0487081491e9d177))
    ROM_LOAD("ws2812node-LPC1313-0_28_0.hex", 0x2a4cc000, 0x00005590, CRC(921dc0f7) SHA1(2c49842bb5163b75a19ebac37f0379ef72c64d4a))
ROM_END
CORE_GAMEDEF(primus, 103, "Primus (1.03.0)", 2018, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ KISS
/-------------------------------------------------------------------*/
/* The Starchild goes between its MOTOR HOME and MOTOR AWAY switches (53, 54) on the two channels
   of an LED driver, MOTOR - BALL LEFT toward away and MOTOR - BALL RIGHT toward home (lamps 169
   and 170): mech 0, from 0 at home to 100 away. Its travel time is not measured: 1 s, well inside
   the game's own limit of about 5 s. The spinning disc runs on lamp 172 */
SPIKE1_INIT_EX(kiss15, "KISS_LE", spike1_dmd, 172, FLIP_L, SPIKE1_SWITCHES(10, 11, 65, 66, 67, 68, 69, 70, 72, 77), SPIKE1_NOHOMES, SPIKE1_LIMIT_MOTOR(169, 170, 53, 54, 1000))
ROM_START(kiss15_141h)
  ROM_REGION(0x3ee2e000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x004ee9ca, CRC(1c55a059) SHA1(9467d0dd243b61707223e1723882652cd46cb926))
    ROM_LOAD("image.bin", 0x004ef000, 0x3e8f02ec, CRC(d96ccf4c) SHA1(ac04430ffc184e2bdfe268f0f6ba91fa3fcc899e))
    ROM_LOAD("coil4node-LPC1112_101-0_28_0.hex", 0x3ede0000, 0x00008487, CRC(29d48577) SHA1(2cef71c04de9d2ff8c4664fafaeab077532f6178))
    ROM_LOAD("coil4node-LPC1112_201-0_28_0.hex", 0x3ede9000, 0x00008487, CRC(19aac926) SHA1(c53e6f2fe65d68a841b23a79ad35dc3422da3e6b))
    ROM_LOAD("coil4node-LPC1313-0_28_0.hex", 0x3edf2000, 0x0000c23a, CRC(e019995b) SHA1(7d09cd36e12e79c9545e8f32e94f738951e45468))
    ROM_LOAD("lcdnode-LPC1113_302-0_28_0.hex", 0x3edff000, 0x0000b484, CRC(d139cac7) SHA1(e4a57e712a599041f7ba7dfcf08f07a724ccbd9f))
    ROM_LOAD("pinnode-LPC1112_101-0_28_0.hex", 0x3ee0b000, 0x00007f1c, CRC(83ca12dc) SHA1(2ac8f8c9384e8c5e0bbc1ca2d9f2ddd8bb831d3d))
    ROM_LOAD("pinnode-LPC1112_201-0_28_0.hex", 0x3ee13000, 0x00007f66, CRC(cee92fdc) SHA1(048abb5d42962204595d7c311dd351c9cb25fd6c))
    ROM_LOAD("pinnode-LPC1313-0_28_0.hex", 0x3ee1b000, 0x0000c859, CRC(18c17d1d) SHA1(6b2f232c0b6c2389c59f998d0487081491e9d177))
    ROM_LOAD("ws2812node-LPC1313-0_28_0.hex", 0x3ee28000, 0x00005590, CRC(921dc0f7) SHA1(2c49842bb5163b75a19ebac37f0379ef72c64d4a))
ROM_END
CORE_GAMEDEF(kiss15, 141h, "KISS Limited Edition (1.41.0)", 2015, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Game of Thrones
/-------------------------------------------------------------------*/
/* The upper flippers have their own leaf on each flipper button (switches 12 and 13), which the
   flipper keys close with the lower ones; their coils are solenoids 16 and 18. The dragon's wings
   and the optional topper dragon's are steppers turning on through their home switch (88, and the
   topper's 121): mech 0 and 1, each as its step (0-199) */
SPIKE1_INIT_EX(got, "GOT_LE", spike1_dmd, 265, FLIP_L | FLIP_U, SPIKE1_SWITCHES_UPPER(10, 11, 16, 5, 12, 13, 66, 67, 68, 69, 71, 75), SPIKE1_HOMES(10, 0, 88, 12, 0, 121), SPIKE1_NOLIMIT)
ROM_START(got_137h)
  ROM_REGION(0x2f27d000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x005fa263, CRC(bd9d74e9) SHA1(41b028146e53e546dc6e88b31642ae45f6e266b7))
    ROM_LOAD("image.bin", 0x005fb000, 0x2ec129dc, CRC(8dbe0aa4) SHA1(ca03000c6a53cf9980774e2739c0277ab12b74b0))
    ROM_LOAD("accbridgenode-LPC1313-0_49_0.hex", 0x2f20e000, 0x0000529b, CRC(5f3ae3d6) SHA1(24a00f23fa0fbee9cf38ddf50e4cede0c96b4137))
    ROM_LOAD("coil4node-LPC1112_101-0_49_0.hex", 0x2f214000, 0x00008595, CRC(6f4a25ae) SHA1(a7f49114a399e96879d5e58e3137c89243385287))
    ROM_LOAD("coil4node-LPC1112_201-0_49_0.hex", 0x2f21d000, 0x00008595, CRC(43f9bc24) SHA1(b921dc5082b87828fdb376ec183caa08f41816bf))
    ROM_LOAD("coil4node-LPC1313-0_49_0.hex", 0x2f226000, 0x0000d28b, CRC(551c8bb3) SHA1(7baef81e9d8157fbea84bbfcebd835731dbbd021))
    ROM_LOAD("lcdnode-LPC1113_302-0_49_0.hex", 0x2f234000, 0x0000b70a, CRC(762a4010) SHA1(d76aa881edf11f9820590ee2eccabe171c45f566))
    ROM_LOAD("netbridge-LPC1313-0_49_0.hex", 0x2f240000, 0x0000e89c, CRC(0179e13b) SHA1(e1f0768ae71b8e723d3cf567a0aa29a6230d3ce6))
    ROM_LOAD("nodebusanalyzer-LPC1313-0_49_0.hex", 0x2f24f000, 0x0000548a, CRC(7925a13c) SHA1(9a6171b0dc26e49496366b8043a420f16c6e3b70))
    ROM_LOAD("pinnode-LPC1112_101-0_49_0.hex", 0x2f255000, 0x0000801a, CRC(66b91e74) SHA1(0f0025758753b046b6e0a117a9668dab51d66180))
    ROM_LOAD("pinnode-LPC1112_201-0_49_0.hex", 0x2f25e000, 0x00008057, CRC(42ca3b82) SHA1(32c08bf5e2aee47ab875e91abc3ee9a1fedeb4ec))
    ROM_LOAD("pinnode-LPC1313-0_49_0.hex", 0x2f267000, 0x0000ddae, CRC(a31b7afa) SHA1(23670fe8e7f3d469a84fa74750c35b15deed7b7b))
    ROM_LOAD("ws2812node-LPC1313-0_49_0.hex", 0x2f275000, 0x0000777d, CRC(00f38c82) SHA1(77dcfd9bc27a86458ce275ec79ccac69c4ba72ff))
ROM_END
CORE_GAMEDEF(got, 137h, "Game of Thrones Limited Edition (1.37.0)", 2015, "Stern", spike1, 0)

/* The same asset image and node firmware as the Limited Edition, its own game program (folder GOT).
   No upper playfield: no upper flippers, the right flipper's EOS is 17, and the dragon is a
   coil (solenoid 18) instead of a stepper. The optional topper dragon is the only stepper: mech 0, homing through switch 121 */
SPIKE1_INIT_EX(gotp, "GOT", spike1_dmd, 265, FLIP_L, SPIKE1_SWITCHES(10, 11, 16, 17, 66, 67, 68, 69, 71, 75), SPIKE1_HOMES(12, 0, 121, 0, 0, 0), SPIKE1_NOLIMIT)
ROM_START(got_137)
  ROM_REGION(0x2f269000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x005e6954, CRC(8e5d0f49) SHA1(3b9f9d429b0406fb6488a5bcc3e47092c340768a))
    ROM_LOAD("image.bin", 0x005e7000, 0x2ec129dc, CRC(8dbe0aa4) SHA1(ca03000c6a53cf9980774e2739c0277ab12b74b0))
    ROM_LOAD("accbridgenode-LPC1313-0_49_0.hex", 0x2f1fa000, 0x0000529b, CRC(5f3ae3d6) SHA1(24a00f23fa0fbee9cf38ddf50e4cede0c96b4137))
    ROM_LOAD("coil4node-LPC1112_101-0_49_0.hex", 0x2f200000, 0x00008595, CRC(6f4a25ae) SHA1(a7f49114a399e96879d5e58e3137c89243385287))
    ROM_LOAD("coil4node-LPC1112_201-0_49_0.hex", 0x2f209000, 0x00008595, CRC(43f9bc24) SHA1(b921dc5082b87828fdb376ec183caa08f41816bf))
    ROM_LOAD("coil4node-LPC1313-0_49_0.hex", 0x2f212000, 0x0000d28b, CRC(551c8bb3) SHA1(7baef81e9d8157fbea84bbfcebd835731dbbd021))
    ROM_LOAD("lcdnode-LPC1113_302-0_49_0.hex", 0x2f220000, 0x0000b70a, CRC(762a4010) SHA1(d76aa881edf11f9820590ee2eccabe171c45f566))
    ROM_LOAD("netbridge-LPC1313-0_49_0.hex", 0x2f22c000, 0x0000e89c, CRC(0179e13b) SHA1(e1f0768ae71b8e723d3cf567a0aa29a6230d3ce6))
    ROM_LOAD("nodebusanalyzer-LPC1313-0_49_0.hex", 0x2f23b000, 0x0000548a, CRC(7925a13c) SHA1(9a6171b0dc26e49496366b8043a420f16c6e3b70))
    ROM_LOAD("pinnode-LPC1112_101-0_49_0.hex", 0x2f241000, 0x0000801a, CRC(66b91e74) SHA1(0f0025758753b046b6e0a117a9668dab51d66180))
    ROM_LOAD("pinnode-LPC1112_201-0_49_0.hex", 0x2f24a000, 0x00008057, CRC(42ca3b82) SHA1(32c08bf5e2aee47ab875e91abc3ee9a1fedeb4ec))
    ROM_LOAD("pinnode-LPC1313-0_49_0.hex", 0x2f253000, 0x0000ddae, CRC(a31b7afa) SHA1(23670fe8e7f3d469a84fa74750c35b15deed7b7b))
    ROM_LOAD("ws2812node-LPC1313-0_49_0.hex", 0x2f261000, 0x0000777d, CRC(00f38c82) SHA1(77dcfd9bc27a86458ce275ec79ccac69c4ba72ff))
ROM_END
SPIKE1_CLONEDEF(got, 137, got_137h, gotp, "Game of Thrones (1.37.0)", 2015, spike1)

/*-------------------------------------------------------------------
/ Ghostbusters
/-------------------------------------------------------------------*/
/* Slimer: node 8 motor 0. The game's own Slimer calibration drives him to -25 and to 125 and passes
   only when he stopped at -6 (+/-4) and 102 (+/-5) counts further on, so his end stops are at -6
   and 96. His speed is not in the game or the firmware, which sets a PWM duty: 50 counts a second
   at full drive is an estimate from the manual's 10 RPM gearhead (041-5132-01), not a measurement */
SPIKE1_INIT_ENCODER(gbust, "ghostbusters_le", spike1_dmd_insert, 165, SPIKE1_SWITCHES(9, 10, 11, 12, 81, 82, 83, 84, 86, 89),
                    SPIKE1_ENCODER_MOTOR(8, 0, -6, 96, 50))
ROM_START(gbust_117h)
  ROM_REGION(0x47c44000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x006c311a, CRC(0b6958ba) SHA1(6f94dbcdaa85e95073534032d74a2dab390b7a4a))
    ROM_LOAD("image.bin", 0x006c4000, 0x462dcb94, CRC(26796bf5) SHA1(fe9ddeb83ff128f2af13ef5db48c0d12dcc93d35))
    ROM_LOAD("accbridgenode-LPC1313-0_52_0.hex", 0x469a1000, 0x0000527e, CRC(c4a99441) SHA1(a87fb68db17a757c4cbd7edfc401a7dbdb7f0722))
    ROM_LOAD("coil4node-LPC1112_101-0_52_0.hex", 0x469a7000, 0x00008543, CRC(98264c96) SHA1(13a1401393ba2eee5c5d22f64cdc9657f658ceba))
    ROM_LOAD("coil4node-LPC1112_201-0_52_0.hex", 0x469b0000, 0x00008543, CRC(41a33f10) SHA1(2c9388aef25694a16dfe4688b81bb191de80322f))
    ROM_LOAD("coil4node-LPC1313-0_52_0.hex", 0x469b9000, 0x0000df1e, CRC(42b641b3) SHA1(d6d18652993b0fcb8def0fbc8504de8cdc3735da))
    ROM_LOAD("lcdinsert.bin", 0x469c7000, 0x0122a020, CRC(e77842db) SHA1(28bdfdceaa27f55a56ba0577d3c1d46634a1c13f))
    ROM_LOAD("lcdnode-LPC1113_302-0_52_0.hex", 0x47bf2000, 0x0000b6fa, CRC(118be40f) SHA1(4f9f2b32229e386517c439dba27527f48632d3dc))
    ROM_LOAD("netbridge-LPC1313-0_52_0.hex", 0x47bfe000, 0x0000f53c, CRC(29323686) SHA1(6c7efc3cacfd4247402969c65dc6d489ae9c8cb3))
    ROM_LOAD("node4-LPC1124_303-0_52_0.hex", 0x47c0e000, 0x000065d4, CRC(13217fb3) SHA1(1eb461cedc2059e46e8c35cfb90599440642dcd6))
    ROM_LOAD("nodebusanalyzer-LPC1313-0_52_0.hex", 0x47c15000, 0x0000546d, CRC(69c0666c) SHA1(12557754046833b136199d24cada86df96fbb8a5))
    ROM_LOAD("pinnode-LPC1112_101-0_52_0.hex", 0x47c1b000, 0x00007fc8, CRC(9f0f6f3f) SHA1(bc95697e1da01e2b6345385ca7ee1118feb50c22))
    ROM_LOAD("pinnode-LPC1112_201-0_52_0.hex", 0x47c23000, 0x00008005, CRC(a2bc8685) SHA1(da95f7350eca021d7accf3ee8f1b2dabe1a800e4))
    ROM_LOAD("pinnode-LPC1313-0_52_0.hex", 0x47c2c000, 0x0000ea4e, CRC(ac00f191) SHA1(3072a56c8d3f8b69da5b76c93ea9706716f9265c))
    ROM_LOAD("ws2812node-LPC1313-0_52_0.hex", 0x47c3b000, 0x00008676, CRC(d0008f92) SHA1(ea5d965703e4244399d3f6012ddfc7b358654d80))
ROM_END
CORE_GAMEDEF(gbust, 117h, "Ghostbusters Limited Edition (1.17.0)", 2016, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Spider-Man The Pin
/-------------------------------------------------------------------*/
/* Node firmware 0.19.1: two boards, four balls, free play (no coin or slam switches). Its display is
   a 64x32 RGB LED matrix in the DMD's place, which the game picks by the machine's
   /usr/local/spike/display.hex link (rgbdotmatrix-0_1_1.hex). The set comes from a card image (the
   game folder "spiderman_cp"); the cabinet's numbers are Supreme's */
SPIKE1_INIT_RGB(smancp, "spiderman_cp", 130, SPIKE1_SWITCHES(8, 10, 9, 11, 0, 0, 0, 0, 14, 0), "rgbdotmatrix-0_1_1.hex")
ROM_START(smancp_101)
  ROM_REGION(0x10b03000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x00504aef, CRC(11a8c4e4) SHA1(015a8155da2179725bde3b91f131b74386654ee7))
    ROM_LOAD("image.bin", 0x00505000, 0x105bafbc, CRC(10f7f532) SHA1(b22fb9d40dd0ab71aaae4dbb96d4f99e9e068dc9))
    ROM_LOAD("coil4node-LPC1112_101-0_19_1.hex", 0x10ac0000, 0x00008705, CRC(4f7ee588) SHA1(8ae7da77e6725bed415b7209a0ab9559d7a6d0cb))
    ROM_LOAD("coil4node-LPC1112_201-0_19_1.hex", 0x10ac9000, 0x00008705, CRC(5dd30f62) SHA1(0a488ab4cb90e118eb0842a7ab66ad018c7728c0))
    ROM_LOAD("coil4node-LPC1313-0_19_1.hex", 0x10ad2000, 0x00009a09, CRC(0b2c7089) SHA1(c1999b6b0a82776239e50bdc57c745a6f2e27054))
    ROM_LOAD("lcdnode-LPC1113_302-0_19_1.hex", 0x10adc000, 0x00009be8, CRC(8efe9b44) SHA1(790bfb0c8865d42c973ac47132ff74219c017343))
    ROM_LOAD("pinnode-LPC1112_101-0_19_1.hex", 0x10ae6000, 0x00008057, CRC(221c95fd) SHA1(7bf6eb85483ecbd1bf95ee75f2f0d309e1d77c1a))
    ROM_LOAD("pinnode-LPC1112_201-0_19_1.hex", 0x10aef000, 0x0000807c, CRC(2769520c) SHA1(380f4755b683b69e59af172000c9300350bfab41))
    ROM_LOAD("pinnode-LPC1313-0_19_1.hex", 0x10af8000, 0x0000a188, CRC(feb117d1) SHA1(453a0de75a62aef43bfaa1edea1634eada328190))
ROM_END
CORE_GAMEDEF(smancp, 101, "Spider-Man The Pin (1.01)", 2016, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Supreme
/-------------------------------------------------------------------*/
/* Node firmware 0.22.0: two boards, four balls. The set comes from a card image (the game folder
   "supreme"). Its start button is the switch the game names START BUTTON (13); the coin door has
   another, COINDOOR START B. (73). The coils name the right flipper RT FLIPPER POWER */
SPIKE1_INIT(supreme, "supreme", spike1_dmd, 130, SPIKE1_SWITCHES(8, 10, 9, 11, 76, 77, 78, 79, 14, 84))
ROM_START(supreme_101)
  ROM_REGION(0x07454000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x00462e0b, CRC(681a3c00) SHA1(dc9cbf0a4204a3bbaecede3a5f5f9b1f9a2269c2))
    ROM_LOAD("image.bin", 0x00463000, 0x06fa11ec, CRC(3eb2c19d) SHA1(23cf9140f30c15367c214503025962619a5937f6))
    ROM_LOAD("coil4node-LPC1112_101-0_22_0.hex", 0x07405000, 0x000082cd, CRC(22c8518e) SHA1(3961762f04c7ac0d5dfa12eb8e5f2a5c75a05bef))
    ROM_LOAD("coil4node-LPC1112_201-0_22_0.hex", 0x0740e000, 0x000082cd, CRC(01d69520) SHA1(f1b2f65b19ff88fd9fa3874b8373a1fc30fd8f88))
    ROM_LOAD("coil4node-LPC1313-0_22_0.hex", 0x07417000, 0x0000bcec, CRC(290c5e17) SHA1(ace1e1a6e37f370f4f49aa61624a95215566843a))
    ROM_LOAD("lcdnode-LPC1113_302-0_22_0.hex", 0x07423000, 0x0000b484, CRC(0e222d2c) SHA1(ae62c5331e7393b80e1ff0f946c41652f7d20a25))
    ROM_LOAD("pinnode-LPC1112_101-0_22_0.hex", 0x0742f000, 0x000080fb, CRC(0be8403d) SHA1(ff3f421ad4fcd06cdbc22936f378e3df123977cc))
    ROM_LOAD("pinnode-LPC1112_201-0_22_0.hex", 0x07438000, 0x00008138, CRC(b3646ae2) SHA1(1f5bda3ebbe5b8e2699f30bf946ea19e6a2ac6c0))
    ROM_LOAD("pinnode-LPC1313-0_22_0.hex", 0x07441000, 0x0000c313, CRC(f6416a86) SHA1(17a26b7800fba83936314fde8cac2191de4042c7))
    ROM_LOAD("ws2812node-LPC1313-0_22_0.hex", 0x0744e000, 0x00005590, CRC(9a090388) SHA1(2f92d8c86bb2c86026cda36ff256b2fcd813cf0e))
ROM_END
CORE_GAMEDEF(supreme, 101, "Supreme (1.01.0)", 2018, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Heavy Metal
/-------------------------------------------------------------------*/
/* The last known SPIKE 1 SDK (node firmware 0.67.0, its images encrypted; the game decrypts them).
   Stern ships it only as an update package (heavy_metal-1_02_0.spk), whose game group the ROM set
   builder reads, and its game program without a symbol table: the addresses below were found by
   matching its code and data against Ghostbusters 1.17.0's (the same compiler and libraries) and
   checked in a running machine */
static const spike1_symbol heavym20_102Symbols[] = {
  { "node_board_table",                                         0x00205d20, 12 },
  { "node_board_device_table",                                  0x00205d74, 12 },
  { "node_board_device_led_table",                              0x00205d44, 12 },
  { "node_board_device_sw_table",                               0x00205d68, 12 },
  { "node_board_device_cl_table_data",                          0x002289ec, 14 * 56 },
  { "NODE_BOARD_DEVICE_CL_TABLE_DATA_ENTRY_COUNT",              0x00205d3c, 4 }, /* the count in the coil table's descriptor */
  { "switch_dedicated_table",                                   0x00205e04, 12 },
  { "switch_table",                                             0x00205e10, 12 },
  { "hook_balls_installed_in_game",                             0x0020620c, 1 },
  { "node_board_runtime_hex_image_list_head",                   0x0037b298, 4 },
  { "_Z33sys_node_board_get_next_block_ptrh",                   0x00035984, 88 },
  { "_Z32node_update_runtime_hex_image_idP18node_board_block_s", 0x0004ccb4, 112 },
  { "_Z50node_board_runtime_hex_image_manager_get_image_pdijj", 0x0004573c, 64 },
  { "_Z23NODEBUS_SetLEDMultiple2hhPhS_S_",                      0x000c145c, 0 },
  { NULL }
};
SPIKE1_INIT_SYMBOLS(heavym20, "heavy_metal", spike1_dmd, 105, SPIKE1_SWITCHES(8, 10, 9, 11, 76, 77, 78, 79, 81, 84), heavym20_102Symbols)
ROM_START(heavym20_102)
  ROM_REGION(0x71fdb000, SPIKE1_REGION, ROMREGION_MAPPED)
    ROM_LOAD("game", 0x00000000, 0x00364e7c, CRC(89dcf06c) SHA1(84e70db1ae9bdb54541c19afbcd960665af13b8a))
    ROM_LOAD("image.bin", 0x00365000, 0x71c08cbc, CRC(378ea9e2) SHA1(488d399d0d16fde42701014952ff5dbc9ea4c150))
    ROM_LOAD("coil4node-LPC1112_101-0_67_0.hex", 0x71f6e000, 0x00008784, CRC(3694f8a6) SHA1(056276be4ce185c4cfaf1dfcfe6b3adf028632af))
    ROM_LOAD("coil4node-LPC1112_201-0_67_0.hex", 0x71f77000, 0x00008784, CRC(9760b3eb) SHA1(0af9c5a27ca63641f9b5db6b90622e153d8c6fb1))
    ROM_LOAD("coil4node-LPC1313-0_67_0.hex", 0x71f80000, 0x0000ecf5, CRC(0795b75b) SHA1(927b5b4a74c727ac71230502e6c4dc0383b865d6))
    ROM_LOAD("lcdnode-LPC1113_302-0_67_0.hex", 0x71f8f000, 0x0000b494, CRC(5486eb88) SHA1(09a9f3a07094c01b91cfe0ecb177aa111af8b6d9))
    ROM_LOAD("node4-LPC1124_303-0_67_0.hex", 0x71f9b000, 0x00009e0c, CRC(219e0f03) SHA1(ef3d00758269197f9b2388d314f8552fa4face7a))
    ROM_LOAD("pinnode-LPC1112_101-0_67_0.hex", 0x71fa5000, 0x00008784, CRC(6c0eadc3) SHA1(af04b077a2df3aaa5b7a2963d14bb897822c082c))
    ROM_LOAD("pinnode-LPC1112_201-0_67_0.hex", 0x71fae000, 0x00008784, CRC(7b8accae) SHA1(ff4fc9fdc8fcf21ec45b4594e5e8b5b40bd284b8))
    ROM_LOAD("pinnode-LPC1313-0_67_0.hex", 0x71fb7000, 0x0000f839, CRC(0ee1afa2) SHA1(c30a73ef171732f33fda0b3685e1832fb2bfa3cb))
    ROM_LOAD("tmc2590node-LPC1313-0_67_0.hex", 0x71fc7000, 0x0000bfe5, CRC(a48f352c) SHA1(01527df261b74753ac2e1fc57b976e0c87be06c0))
    ROM_LOAD("ws2812node-LPC1313-0_67_0.hex", 0x71fd3000, 0x00007c40, CRC(514046cf) SHA1(a3a595cef1bf9058bba46d8d61eb5bc6b43e790f))
ROM_END
CORE_GAMEDEF(heavym20, 102, "Heavy Metal (1.02.0)", 2020, "Stern", spike1, 0)

#endif /* HAS_SPIKE1 */
