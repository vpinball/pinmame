// license:BSD-3-Clause

/************************************************************************************************
  Stern Spike 1 (2015-2018: Game of Thrones, KISS, WrestleMania, Ghostbusters, ...)

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
************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpuintrf.h"
#include "usrintrf.h"
#include "spike1.h"
#include "../spike1/spike1_public.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAS_SPIKE1

#define SPIKE1_REGION     REGION_USER1   /* the game folder's files, one after another */
#define SPIKE1_AUDIO_RATE 44100          /* the stream's rate until the game sets /dev/i2s to its own */
#define SPIKE1_PEN0       (COL_COUNT + 48 + 48 + 48) /* the core's palette, then 32768 pens of RGB555 */
#define SPIKE1_MAXCOILS   64
#define SPIKE1_MAXLEDS    256
#define SPIKE1_FIRSTCUSTSOL CORE_FIRSTCUSTSOL /* solenoid 51 */
#define SPIKE1_NCUSTSOLS  (CORE_MODOUT_SOL_MAX - SPIKE1_FIRSTCUSTSOL + 1)

/* The numbers a title's manual gives the switches PinMAME's keys reach; they differ between titles */
typedef struct {
  int flip[8];   /* left button, right button, left EOS, right EOS, then the same for the upper flippers (0: none) */
  int coin[4];   /* left, right, center, fourth */
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

/* What a title adds to core_tGameData: its game folder's name (the program runs as
   /games/<folder>/game), its switch numbers, its steppers' home switches on other boards, its
   limit motor, and for a game program shipped without its symbol table the addresses of the
   tables the subsystem reads (NULL when the program has its own) */
typedef struct {
  core_tGameData core;
  const char *folder;
  spike1_tSwitches sw;
  spike1_tHome home[2];
  spike1_tLimitMotor limit;
  const spike1_symbol *symbols;
} spike1_tGameData;

static const spike1_tGameData *spike1_game(void) { return (const spike1_tGameData *)core_gameData; }

/* SPIKE1_LOG set in the environment: once every five seconds, what the sound stream asked for and
   got, and the switch changes passed to the machine */
static struct { int on; unsigned frames, asked, got, switches, calls, peak, late, shortfalls; double cycles, run_s; cycles_t since; } diag;

static struct {
  int running, warned;
  int sw[256];            /* the title's switch numbers, and how many */
  int nsw;
  UINT8 swState[256];     /* each switch as last handed to the machine */
  int coilSol[SPIKE1_MAXCOILS]; /* the PinMAME solenoid of coil k, 0 if the game has no coil k */
  int coilOf[CORE_MODOUT_SOL_MAX + 1]; /* and back: the coil of solenoid s, -1 for none */
  int flipCoil[2][2];     /* left, right: the flipper's coil and its separate hold coil (Whoa Nellie), -1 for none */
  int startSw;
  unsigned dmdFrames;
  unsigned nLeds;
  unsigned audioRate;     /* the sound stream's rate: the game's, once it has set one */
  int ledLamp[SPIKE1_MAXLEDS]; /* the lamp number of LED channel k, 0 for none */
} locals;

static int spike1_stream = -1; /* the sound stream's first channel, the left; the right follows */

/* The machine's NVRAM block between the NVRAM handler and the machine: PinMAME reads the file
   before MACHINE_INIT and writes it after MACHINE_STOP, when the machine is gone */
static UINT8 *nvBlock;
static unsigned nvSize;

/*-------------------------------------------------
/  the CPU: the game program, run for PinMAME's cycles
/-------------------------------------------------*/
int spike1cpu_ICount;

void spike1cpu_init(void) {}
void spike1cpu_reset(void *param) { (void)param; }
void spike1cpu_exit(void) {}

int spike1cpu_execute(int cycles)
{
  spike1cpu_ICount = cycles;
  if (locals.running) {
    const cycles_t t0 = diag.on ? osd_cycles() : 0;
    spike1_pinmame_run(cycles);
    if (diag.on) { diag.calls++; diag.cycles += cycles; diag.run_s += (double)(osd_cycles() - t0) / (double)osd_cycles_per_second(); }
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
  case CPU_INFO_NAME: return "Spike 1";
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
/             upper flippers, as the core's CORE_SW*FLIP* bits lay them out), 88-95 to the
/             entries they leave free
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
    COREPORT_BITDEF(  0x0001, IPT_COIN1,        IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,        IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,        KEYCODE_3) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,        KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Back",           KEYCODE_7) \
    COREPORT_BIT(     0x0020, "Minus",          KEYCODE_8) \
    COREPORT_BIT(     0x0040, "Plus",           KEYCODE_9) \
    COREPORT_BIT(     0x0080, "Select",         KEYCODE_0) \
    COREPORT_BITTOG(  0x0100, "Coin Door",      KEYCODE_END) \
    COREPORT_BITDEF(  0x0200, IPT_START1,       IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0400, "Tilt",           KEYCODE_INSERT) \
    COREPORT_BIT(     0x0800, "Slam Tilt",      KEYCODE_HOME) \
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
  for (i = 0; i < 4; i++) core_setSw(sw->coin[i], inports[CORE_COREINPORT] & (0x0001 << i));
  for (i = 0; i < 4; i++) core_setSw(SPIKE1_SW_BACK - i, inports[CORE_COREINPORT] & (0x0010 << i)); /* back, minus, plus, select */
  core_setSw(SPIKE1_SW_DCSENSE, !(inports[CORE_COREINPORT] & 0x0100));
  if (locals.startSw >= 0) core_setSw(locals.startSw, inports[CORE_COREINPORT] & 0x0200);
  core_setSw(sw->tilt, inports[CORE_COREINPORT] & 0x0400);
  core_setSw(sw->slam, inports[CORE_COREINPORT] & 0x0800);
}

/*-------------------------------------------------
/  the I/O, once a frame
/-------------------------------------------------*/
static void spike1_sync_io(void)
{
  int i;
  UINT32 sols = 0;
  /* switches that changed, the DIP switches from the DIP settings */
  const int dips = core_getDip(0);
  for (i = 0; i < 8; i++) core_setSw(SPIKE1_SW_DIP1 + i, (dips >> i) & 1);
  for (i = 0; i < locals.nsw; i++) {
    const UINT8 closed = core_getSw(locals.sw[i]) ? 1 : 0;
    if (closed != locals.swState[i]) {
      locals.swState[i] = closed;
      spike1_pinmame_set_switch(locals.sw[i], closed);
      diag.switches++;
      if (diag.on) fprintf(stderr, "[spike1 driver] switch %d %s\n", locals.sw[i], closed ? "closed" : "opened");
    }
  }
  /* coils */
  for (i = 0; i < SPIKE1_MAXCOILS; i++) {
    const int sol = locals.coilSol[i];
    if (sol) {
      const unsigned level = spike1_pinmame_coil_level(i);
      coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + sol - 1].value = level / 255.0f;
      if (level && sol <= 32) sols |= CORE_SOLBIT(sol);
    }
  }
  coreGlobals.solenoids = coreGlobals.pulsedSolState = sols;
  coreGlobals.solenoids2 &= ~(UINT32)(CORE_LLFLIPSOLBITS | CORE_LRFLIPSOLBITS);
  for (i = 0; i < 4; i++) {
    const int coil = locals.flipCoil[i >> 1][i & 1];
    if (coil >= 0 && spike1_pinmame_coil_level(coil)) coreGlobals.solenoids2 |= (i >> 1) ? CORE_LRFLIPSOLBITS : CORE_LLFLIPSOLBITS;
  }
  /* LEDs */
  memset((void *)coreGlobals.lampMatrix, 0, sizeof(coreGlobals.lampMatrix));
  for (i = 0; i < (int)locals.nLeds; i++) {
    const int lamp = locals.ledLamp[i] - 1;
    if (lamp >= 0) {
      const unsigned level = spike1_pinmame_led_level(i);
      coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + lamp].value = level / 255.0f;
      if (level) coreGlobals.lampMatrix[lamp / 8] |= 1 << (lamp % 8);
    }
  }
  /* the sound stream at the game's rate */
  {
    const unsigned rate = spike1_pinmame_audio_rate();
    if (spike1_stream >= 0 && rate && rate != locals.audioRate) {
      stream_set_sample_rate(spike1_stream, rate);
      stream_set_sample_rate(spike1_stream + 1, rate);
      locals.audioRate = rate;
    }
  }
  /* the DMD: a new frame when the game has sent one */
  {
    static UINT8 dots[SPIKE1_DMD_WIDTH * SPIKE1_DMD_HEIGHT];
    const unsigned frames = spike1_pinmame_dmd(dots);
    if (frames != locals.dmdFrames) {
      locals.dmdFrames = frames;
      core_dmd_submit_frame(core_gameData->lcdLayout, dots, 1);
    }
  }
}

static INTERRUPT_GEN(spike1_vblank)
{
  char reason[128];
  if (locals.running) {
    spike1_sync_io();
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
    if (!locals.warned && !spike1_pinmame_running(reason, sizeof(reason))) {
      locals.warned = 1;
      usrintf_showmessage_secs(30, "Spike 1: the game program stopped: %s", reason);
    }
  }
  core_updateSw(0); /* the flippers are the node boards' own (FLIP_SOL), see spike1_sync_io */
}

/* mech n: the n-th motor's position */
static int spike1_getMech(int mechNo)
{
  int position = 0;
  return mechNo >= 0 && spike1_pinmame_motor((unsigned)mechNo, &position, NULL) ? position : 0;
}

/* the coils PinMAME numbers 51 on: a coil numbered 0 or above 32 */
static int spike1_getSol(int solNo)
{
  if (solNo < SPIKE1_FIRSTCUSTSOL || solNo > CORE_MODOUT_SOL_MAX || locals.coilOf[solNo] < 0) return 0;
  return (int)spike1_pinmame_coil_level(locals.coilOf[solNo]);
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
    UINT16 *line = (UINT16 *)bitmap->line[layout->top + y] + layout->left;
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
    UINT16 *line = (UINT16 *)bitmap->line[layout->top + y] + layout->left;
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

/* The key the encrypted LCD clips (WWE's .spv files) are RC4-encrypted with, which each board's
   factory key store holds: the subsystem hands it to a title that has such clips */
static const unsigned char spike1_video_key[16] = {
  0x8e, 0x1f, 0x55, 0x43, 0xc2, 0xf5, 0x4a, 0x11, 0x67, 0x3a, 0x28, 0x2a, 0x2f, 0x87, 0xc0, 0x06
};

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
      fifo.s[2 * at] = tmp[2 * i];
      fifo.s[2 * at + 1] = tmp[2 * i + 1];
    }
    for (i = 0; i < 2 * n; i++) { const unsigned a = (unsigned)(tmp[i] < 0 ? -tmp[i] : tmp[i]); if (a > diag.peak) diag.peak = a; }
    fifo.count += n;
    got += n;
    if (n < want) break;
  }
  diag.got += got;
  diag.asked += length;
  if (got < length) diag.late++; /* the machine had less ready than the stream asked: without the FIFO, a gap */
  if (!fifo.primed && fifo.count >= length + mark) fifo.primed = 1;
  for (i = 0; i < length; i++) {
    if (fifo.primed && fifo.count) {
      fifo.last[0] = fifo.s[2 * fifo.head];
      fifo.last[1] = fifo.s[2 * fifo.head + 1];
      fifo.head = (fifo.head + 1) % SPIKE1_FIFO;
      fifo.count--;
    } else if (fifo.primed) { /* ran dry: hold, and wait for the mark again */
      fifo.primed = 0;
      diag.shortfalls++;
    }
    buffer[0][i] = fifo.last[0];
    buffer[1][i] = fifo.last[1];
  }
}

static int spike1_sh_start(const struct MachineSound *msound)
{
  const char *names[] = { "Spike 1 Left", "Spike 1 Right" };
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
    if (!strcmp(name, "LEFT FLIPPER") || !strcmp(name, "LEFT FLIPPER POWER")) locals.flipCoil[0][0] = number;
    if (!strcmp(name, "LEFT FLIPPER HOLD")) locals.flipCoil[0][1] = number;
    if (!strcmp(name, "RIGHT FLIPPER") || !strcmp(name, "RIGHT FLIPPER POWER")) locals.flipCoil[1][0] = number;
    if (!strcmp(name, "RIGHT FLIPPER HOLD")) locals.flipCoil[1][1] = number;
  }
  /* the core counts lamps in whole columns, up to the highest Light Reference number of the title;
     a motor drive is no lamp, and Light Reference 0 has no PinMAME number */
  coreGlobals.nLamps = 8 * (CORE_CUSTLAMPCOL + core_gameData->hw.lampCol);
  locals.nLeds = 0;
  for (i = 0; i < SPIKE1_MAXLEDS && spike1_pinmame_led(i, &number, &kind, &name); i++) {
    locals.ledLamp[i] = number >= 1 && number <= coreGlobals.nLamps && !(kind & SPIKE1_LED_MOTOR) ? number : 0;
    locals.nLeds = i + 1;
  }
  coreGlobals.nSolenoids = cust ? SPIKE1_FIRSTCUSTSOL - 1 + cust : 32;
  coreGlobals.nGI = 0;
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
  locals.startSw = spike1_pinmame_find_switch("START BUTTON");
}

static MACHINE_INIT(spike1)
{
  spike1_file files[64];
  unsigned count = 0;
  const struct RomModule *region, *rom;
  char error[256];
  int c;

  memset(&locals, 0, sizeof(locals));
  memset(&fifo, 0, sizeof(fifo));
  memset(&diag, 0, sizeof(diag));
  diag.on = getenv("SPIKE1_LOG") != NULL;
  locals.startSw = -1;
  /* the insert's colours: the pens after the core's palette are RGB555 */
  for (c = 0; c < 32768; c++)
    palette_set_color(SPIKE1_PEN0 + c, (c >> 7 & 0xf8) | (c >> 12), (c >> 2 & 0xf8) | (c >> 7 & 7), (c << 3 & 0xf8) | (c >> 2 & 7));
  core_dmd_pwm_init(core_gameData->lcdLayout, CORE_DMD_PWM_PREINTEGRATED_LINEAR_16, CORE_DMD_PWM_PREINTEGRATED_LINEAR_16, 0);

  /* the title's files, as the ROM set lays them out in SPIKE1_REGION */
  for (region = rom_first_region(Machine->gamedrv); region; region = rom_next_region(region)) {
    if (ROMREGION_GETTYPE(region) != SPIKE1_REGION) continue;
    for (rom = rom_first_file(region); rom && count < sizeof(files) / sizeof(files[0]); rom = rom_next_file(rom)) {
      files[count].name = ROM_GETNAME(rom);
      files[count].data = memory_region(SPIKE1_REGION) + ROM_GETOFFSET(rom);
      files[count].size = ROM_GETLENGTH(rom);
      count++;
    }
  }
  if (!spike1_pinmame_start(spike1_game()->folder, files, count, spike1_game()->symbols, nvBlock, nvSize, error, sizeof(error))) {
    usrintf_showmessage_secs(30, "Spike 1: the game program does not start: %s", error);
    return;
  }
  locals.running = 1;
  spike1_pinmame_video_key(spike1_video_key); /* a no-op for a title without encrypted clips */
  for (c = 0; c < 2; c++) {
    const spike1_tHome *home = &spike1_game()->home[c];
    if (home->node) spike1_pinmame_stepper_home(home->node, home->stepper, home->sw);
  }
  {
    const spike1_tLimitMotor *limit = &spike1_game()->limit;
    if (limit->forward) spike1_pinmame_limit_motor(limit->forward, limit->backward, limit->home, limit->away, (unsigned)limit->travelMs);
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
  MDRV_NVRAM_HANDLER(spike1)
  MDRV_SWITCH_UPDATE(spike1)
  MDRV_SWITCH_CONV(spike1_sw2m, spike1_m2sw)
  MDRV_LAMP_CONV(spike1_lamp2m, spike1_m2lamp)
  MDRV_SOUND_ADD(CUSTOM, spike1_sndInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_PALETTE_LENGTH(SPIKE1_PEN0 + 32768)
MACHINE_DRIVER_END

/* A title with the CPU board's LCD (WWE): the DMD with the LCD below it is taller than PinMAME's
   default screen outside VPinMAME (320x256) */
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
/* Another edition of a title (a Pro beside its LE): the set name_ver, a clone of the set parent,
   with its own game data from SPIKE1_INIT*(data, ...). Its files are the edition's own; those it
   shares with the parent (the asset image, the node firmware) can come from the parent's ROM set */
#define SPIKE1_CLONEDEF(name, ver, parent, data, longname, year, machine) \
  GAMEX(year, name##_##ver, parent, machine, data, data, ROT0, "Stern", longname, 0)

/*-------------------------------------------------------------------
/ Ghostbusters (Stern, 2016) - Limited Edition
/-------------------------------------------------------------------*/
SPIKE1_INIT(gbust, "ghostbusters_le", spike1_dmd_insert, 165, SPIKE1_SWITCHES(9, 10, 11, 12, 81, 82, 83, 84, 86, 89))
ROM_START(gbust_117h)
  ROM_REGION(0x47c44000, SPIKE1_REGION, 0)
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
CORE_GAMEDEF(gbust, 117h, "Ghostbusters (Limited Edition 1.17.0)", 2016, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Whoa Nellie! Big Juicy Melons (Stern, 2015)
/ The DMD output drives the small LCD in the apron. The score and credit reels in the backbox are
/ steppers the node board runs: mech 0-4 are the 1000s, 100s, 10s and 1s reels and the credit reel,
/ each as its step (0-199, 20 steps a digit; the credit reel 10 a step). The bells and the knocker
/ are coils
/-------------------------------------------------------------------*/
SPIKE1_INIT(wnbjm, "WN", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(wnbjm_155)
  ROM_REGION(0x10bb7000, SPIKE1_REGION, 0)
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
/ Primus (Stern, 2018) and Pabst Can Crusher (Stern, 2016): Whoa Nellie's hardware - the apron LCD,
/ the score and credit reels as mechs 0-4, the bells - and its switch numbers
/-------------------------------------------------------------------*/
SPIKE1_INIT(primus, "primus", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(primus_103)
  ROM_REGION(0x2a4d2000, SPIKE1_REGION, 0)
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

SPIKE1_INIT(pabst, "can_crusher", spike1_dmd, 76, SPIKE1_SWITCHES(3, 4, 1, 2, 53, 54, 55, 56, 58, 60))
ROM_START(pabst_101)
  ROM_REGION(0x39d5f000, SPIKE1_REGION, 0)
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
CORE_GAMEDEF(pabst, 101, "Pabst Can Crusher (1.01.0)", 2016, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Game of Thrones (Stern, 2015) - Limited Edition
/ The upper flippers have their own leaf on each flipper button (switches 12 and 13), which the
/ flipper keys close with the lower ones; their coils are solenoids 16 and 18. The dragon's wings
/ and the optional topper dragon's are steppers turning on through their home switch (88, and the
/ topper's 121): mech 0 and 1, each as its step (0-199)
/-------------------------------------------------------------------*/
SPIKE1_INIT_EX(got, "GOT_LE", spike1_dmd, 265, FLIP_L | FLIP_U, SPIKE1_SWITCHES_UPPER(10, 11, 16, 5, 12, 13, 66, 67, 68, 69, 71, 75), SPIKE1_HOMES(10, 0, 88, 12, 0, 121), SPIKE1_NOLIMIT)
ROM_START(got_137h)
  ROM_REGION(0x2f27d000, SPIKE1_REGION, 0)
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
CORE_GAMEDEF(got, 137h, "Game of Thrones (Limited Edition 1.37.0)", 2015, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ Game of Thrones (Stern, 2015) - Pro
/ The same asset image and node firmware as the Limited Edition, its own game program (folder
/ GOT). No upper playfield: no upper flippers, the right flipper's EOS is 17, and the dragon is a
/ coil (solenoid 18) instead of a stepper. The optional topper dragon is the only stepper: mech 0,
/ homing through switch 121
/-------------------------------------------------------------------*/
SPIKE1_INIT_EX(gotp, "GOT", spike1_dmd, 265, FLIP_L, SPIKE1_SWITCHES(10, 11, 16, 17, 66, 67, 68, 69, 71, 75), SPIKE1_HOMES(12, 0, 121, 0, 0, 0), SPIKE1_NOLIMIT)
ROM_START(got_137)
  ROM_REGION(0x2f269000, SPIKE1_REGION, 0)
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
SPIKE1_CLONEDEF(got, 137, got_137h, gotp, "Game of Thrones (Pro 1.37.0)", 2015, spike1)

/*-------------------------------------------------------------------
/ KISS (Stern, 2015) - Limited Edition (kiss15: PinMAME's kiss is Bally's of 1979)
/ The Starchild goes between its MOTOR HOME and MOTOR AWAY switches (53, 54) on the two channels
/ of an LED driver, MOTOR - BALL LEFT toward away and MOTOR - BALL RIGHT toward home (lamps 169
/ and 170): mech 0, from 0 at home to 100 away. Its travel time is not measured: 1 s, well inside
/ the game's own limit of about 5 s. The spinning disc runs on lamp 172
/-------------------------------------------------------------------*/
SPIKE1_INIT_EX(kiss15, "KISS_LE", spike1_dmd, 172, FLIP_L, SPIKE1_SWITCHES(10, 11, 65, 66, 67, 68, 69, 70, 72, 77), SPIKE1_NOHOMES, SPIKE1_LIMIT_MOTOR(169, 170, 53, 54, 1000))
ROM_START(kiss15_141h)
  ROM_REGION(0x3ee2e000, SPIKE1_REGION, 0)
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
CORE_GAMEDEF(kiss15, 141h, "KISS (Limited Edition 1.41.0)", 2015, "Stern", spike1, 0)

/*-------------------------------------------------------------------
/ WWE WrestleMania (Stern, 2015) - Limited Edition
/ The oldest Spike 1 SDK. Its playfield LCD is the CPU board's own 320x240 screen, the second
/ display; the clips it plays are the folder's video files (.spv, RC4-encrypted), which the set
/ holds beside the others and the subsystem puts in video/. Each flipper button has a second leaf
/ (74, 75) for the upper flipper (solenoid 22)
/-------------------------------------------------------------------*/
SPIKE1_INIT_EX(wwe, "WWE_LE", spike1_dmd_lcd, 102, FLIP_L | FLIP_U, SPIKE1_SWITCHES_UPPER(10, 11, 65, 66, 74, 75, 67, 68, 69, 70, 72, 77), SPIKE1_NOHOMES, SPIKE1_NOLIMIT)
ROM_START(wwe_135h)
  ROM_REGION(0xaae47000, SPIKE1_REGION, 0)
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
CORE_GAMEDEF(wwe, 135h, "WWE WrestleMania (Limited Edition 1.35.0)", 2015, "Stern", spike1_lcd, 0)

/*-------------------------------------------------------------------
/ Heavy Metal (Stern, 2020)
/ The newest Spike 1 SDK (node firmware 0.67.0, its images encrypted; the game decrypts them).
/ Stern ships it only as an update package (heavy_metal-1_02_0.spk), whose game group the ROM set
/ builder reads, and its game program without a symbol table: the addresses below were found by
/ matching its code and data against Ghostbusters 1.17.0's (the same compiler and libraries) and
/ checked in a running machine. The PinMAME name heavymtl is taken (Heavy Metal, 198?), and
/ hvymetal is Bally's Heavy Metal Meltdown
/-------------------------------------------------------------------*/
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
  ROM_REGION(0x71fdb000, SPIKE1_REGION, 0)
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
