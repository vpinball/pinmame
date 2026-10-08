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
    solenoids 1-32          the coils by their Driver Reference number
    solenoids 51-72         a coil numbered 0 or above 32, in number order
    solenoids 46, 48        the flipper outputs: the coils the game names RIGHT FLIPPER and
                            LEFT FLIPPER (and their HOLD coils where a title has them), which the
                            node board fires on its own when the button closes
    lamps 1-n               the LED channels by their Light Reference number (an RGB LED is three;
                            GI strings and flashers are LED channels too)
    mech 0-n                the motors the boards run on their own (Ghostbusters' Slimer), then
                            their steppers (Whoa Nellie's reels): a motor's position in the game's
                            units, a stepper's step within its turn, through Controller.GetMech(n)
  Coils and LEDs are levels (a coil driver's PWM duty, an LED channel's brightness): modulated
  outputs hold them as 0-1, the solenoid and lamp bits say whether they are on at all.

  The display is the DMD, and below it the LCD insert when the title has one (Ghostbusters' Ecto
  goggles), drawn in direct colour through 32768 palette pens after the core's own.
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
#define SPIKE1_AUDIO_RATE 44100          /* what the games set /dev/i2s to */
#define SPIKE1_PEN0       (COL_COUNT + 48 + 48 + 48) /* the core's palette, then 32768 pens of RGB555 */
#define SPIKE1_MAXCOILS   64
#define SPIKE1_MAXLEDS    256
#define SPIKE1_FIRSTCUSTSOL CORE_FIRSTCUSTSOL /* solenoid 51 */
#define SPIKE1_NCUSTSOLS  (CORE_MODOUT_SOL_MAX - SPIKE1_FIRSTCUSTSOL + 1)

/* The numbers a title's manual gives the switches PinMAME's keys reach; they differ between titles */
typedef struct {
  int flip[4];   /* left button, right button, left EOS, right EOS */
  int coin[4];   /* left, right, center, fourth */
  int tilt, slam;
} spike1_tSwitches;
#define SPIKE1_SWITCHES(lbutton, rbutton, leos, reos, coin1, coin2, coin3, coin4, tilt, slam) \
  { { lbutton, rbutton, leos, reos }, { coin1, coin2, coin3, coin4 }, tilt, slam }

/* What a title adds to core_tGameData: its game folder's name (the program runs as
   /games/<folder>/game) and its switch numbers */
typedef struct {
  core_tGameData core;
  const char *folder;
  spike1_tSwitches sw;
} spike1_tGameData;

static const spike1_tGameData *spike1_game(void) { return (const spike1_tGameData *)core_gameData; }

/* SPIKE1_LOG set in the environment: once every five seconds, what the sound stream asked for and
   got, and the switch changes passed to the machine */
static struct { int on; unsigned frames, asked, got, switches, calls, peak; double cycles, run_s; cycles_t since; } diag;

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
  int ledLamp[SPIKE1_MAXLEDS]; /* the lamp number of LED channel k, 0 for none */
} locals;

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
/    0-95     matrix entries 0-95, except that the title's four flipper switches and the numbers
/             88-91 trade places: the flipper switches go to column 11, PinMAME's flipper column
/             (88 right EOS, 89 right button, 90 left EOS, 91 left button, as the core's
/             CORE_SW*FLIP* bits lay them out), 88-91 to the entries they leave free
/    101-132  columns 12-15: the CPU board's C1-C16 as 101-116, then numbers no title uses;
/             132 also takes any number the game does not have
/-------------------------------------------------*/
static int spike1_flipSlot(int k)
{
  const int *flip = spike1_game()->sw.flip;
  static const int slot[4] = { 3, 1, 2, 0 }; /* entry 88 + k holds flip[slot[k]] */
  return flip[slot[k]];
}

static int spike1_swap(int n)
{
  int k;
  for (k = 0; k < 4; k++) {
    if (n == spike1_flipSlot(k)) return 88 + k;
    if (n == 88 + k) return spike1_flipSlot(k);
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
              "sound: stream asked %u frames, got %u, peak %u; the game's rate %u Hz; %u switch changes\n",
              wall, diag.calls, diag.calls ? diag.cycles / diag.calls : 0., diag.run_s,
              diag.asked, diag.got, diag.peak, spike1_pinmame_audio_rate(), diag.switches);
      diag.asked = diag.got = diag.calls = diag.peak = 0;
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

/*-------------------------------------------------
/  sound: the game's DAC, 16-bit stereo
/-------------------------------------------------*/
static int spike1_sound_started;

static void spike1_snd_update(int num, INT16 **buffer, int length)
{
  static INT16 tmp[2 * 1024];
  int done = 0;
  (void)num;
  while (done < length) {
    const int want = length - done > 1024 ? 1024 : length - done;
    const int n = locals.running ? (int)spike1_pinmame_audio(tmp, want) : 0;
    int i;
    diag.asked += want;
    diag.got += n;
    for (i = 0; i < 2 * n; i++) { const unsigned a = (unsigned)(tmp[i] < 0 ? -tmp[i] : tmp[i]); if (a > diag.peak) diag.peak = a; }
    for (i = 0; i < n; i++) {
      buffer[0][done + i] = tmp[2 * i];
      buffer[1][done + i] = tmp[2 * i + 1];
    }
    if (n < want) { /* the machine is behind: silence for the rest of this buffer */
      for (i = done + n; i < length; i++) buffer[0][i] = buffer[1][i] = 0;
      return;
    }
    done += n;
  }
}

static int spike1_sh_start(const struct MachineSound *msound)
{
  const char *names[] = { "Spike 1 Left", "Spike 1 Right" };
  const int vol[2] = { MIXER(100, MIXER_PAN_LEFT), MIXER(100, MIXER_PAN_RIGHT) };
  (void)msound;
  spike1_sound_started = 1;
  return stream_init_multi(2, names, vol, SPIKE1_AUDIO_RATE, 0, spike1_snd_update) < 0;
}

static void spike1_sh_stop(void) { spike1_sound_started = 0; }

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
    if (!strcmp(name, "LEFT FLIPPER")) locals.flipCoil[0][0] = number;
    if (!strcmp(name, "LEFT FLIPPER HOLD")) locals.flipCoil[0][1] = number;
    if (!strcmp(name, "RIGHT FLIPPER")) locals.flipCoil[1][0] = number;
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
  if (!spike1_pinmame_start(spike1_game()->folder, files, count, nvBlock, nvSize, error, sizeof(error))) {
    usrintf_showmessage_secs(30, "Spike 1: the game program does not start: %s", error);
    return;
  }
  locals.running = 1;
  spike1_map_switches();
  spike1_map_outputs();
}

static MACHINE_STOP(spike1)
{
  if (locals.running) {
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
#define SPIKE1_INIT(name, folder, layout, lamps, switches) \
  SPIKE1_INPUT_PORTS(name) \
  static spike1_tGameData name##GameData = { \
    { GEN_SPIKE1, layout, \
      { FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 4, SPIKE1_LAMPCOLS(lamps), SPIKE1_NCUSTSOLS, 0, 0, 0, 0, spike1_getSol, NULL, spike1_getMech } }, \
    folder, switches }; \
  static void init_##name(void) { core_gameData = &name##GameData.core; }

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

#endif /* HAS_SPIKE1 */
