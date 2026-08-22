// license:BSD-3-Clause

/************************************************************************************************
  Midway Pinball 2000 (Revenge From Mars, Star Wars Episode I)

  The machine itself lives in src/p2k/ (see src/p2k/README.md): a MediaGX PC
  imported from MAME with its own bus and device tree. This file is only what PinMAME needs to
  instantiate that CPU - a machine driver and a game definition - so that PinMAME's own
  facilities, the remote debugger above all, can attach to it.

  It is deliberately not the finished driver: the P2K machine still assembles itself through
  p2k_state rather than through PinMAME's machine driver
 ************************************************************************************************/
#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "wmssnd.h"
#include "p2k_names.h"
#include "../p2k/p2k_public.h"

#if HAS_MEDIAGX

/* Everything in this driver that only exists to debug it - the watches, the frame dumps and the
   stand-in playfield, and the P2K_* environment variables that drive them. See src/p2k/p2k_debug.h,
   which says the same for the subsystem side and is where the switch is documented */
#ifndef P2K_DEBUG
#define P2K_DEBUG 0
#endif

#if P2K_DEBUG
static int p2k_bringupFrame; /* SCAFFOLDING: shared clock, see the block below */
static int p2k_keyLaunch;    /* Launch Ball key, shared with the scaffolding - see there */
#endif
static int video_mode_announced = 0;

/* The MediaGX side's ROMs. They are declared as regions like any other game's so that a version
   is picked, audited and zipped exactly the way PinMAME does it everywhere else; MACHINE_INIT
   hands them to the subsystem, which never opens a file of its own. REGION_USER4 is the sound
   board's banked SRAM (DCS_P2K_SRAMREGION), hence 1 and 2 here */
#define P2K_PRISMREGION (REGION_USER1) /* four 16 MB Prism banks, u100-u107 interleaved */
#define P2K_UPDREGION   (REGION_USER2) /* the 8 MB update flash - this is the game version */

/* implemented in src/p2k/p2k_pinmame.cpp */
extern void p2k_pinmame_start(const unsigned char *prism, unsigned prismLen, const unsigned char *updates, unsigned updatesLen);
extern void p2k_pinmame_stop(void);
extern void p2k_pinmame_nvram_set(int which, const unsigned char *data, unsigned size);
extern unsigned p2k_pinmame_nvram_get(int which, unsigned char *data, unsigned size);
extern UINT32 p2k_pinmame_read(offs_t address, UINT32 mem_mask);
extern void p2k_pinmame_write(offs_t address, UINT32 data, UINT32 mem_mask);
extern unsigned p2k_pinmame_frame(UINT32 *dest, unsigned capacity, unsigned *width, unsigned *height, const unsigned fast_15bpp_path, unsigned *fast_15bpp_path_success);
extern void p2k_pinmame_push_switches(const unsigned char *matrix, unsigned count);
extern void p2k_pinmame_set_dips(unsigned char dips);
/* Takes the machine's clock from the host, the way a battery-backed RTC that kept running would.
   keep_year for a machine that already has a clock of its own: the firmware reads the year register
   as a number of years to add rather than a date, so giving it the host year twice makes the displayed year climb */
extern void p2k_pinmame_clock_from_host(int keep_year);
extern void p2k_pinmame_pull_outputs(unsigned char *lamps, unsigned lamp_columns, UINT32 *solenoids, UINT32 *solenoids2);

static READ32_HANDLER(p2k_r)  { return p2k_pinmame_read(offset * 4, ~mem_mask); }
static WRITE32_HANDLER(p2k_w) { p2k_pinmame_write(offset * 4, data, ~mem_mask); }

/* The subsystem decodes addresses itself, but PinMAME's memory system wants the regions spelled
   out - a single entry spanning the whole 32-bit space keeps the CPU from being set up at all.
   These are the same ranges p2k_state::mem_r handles */
#define P2K_RANGES(handler) \
	{ 0x00000000, 0x000FFFFF, handler }, /* low memory: RAM, CGA, expansion ROM, BIOS RAM */ \
	{ 0x00100000, 0x0FFFFFFF, handler }, /* extended RAM                                  */ \
	{ 0x10000000, 0x1FFFFFFF, handler }, /* PLX EEPROM, NVRAM, update flash, DCS, prism   */ \
	{ 0x40000000, 0x40BFFFFF, handler }, /* MediaGX registers, SMM, frame buffer          */ \
	{ 0xC0000000, 0xC000FFFF, handler }, /* alias of the MediaGX control registers        */ \
	{ 0xF00C0000, 0xF00C7FFF, handler }, /* second window on the expansion ROM            */ \
	{ 0xFFFD0000, 0xFFFFFFFF, handler }, /* system BIOS, holds the reset vector           */

static MEMORY_READ32_START(p2k_readmem)
	P2K_RANGES(p2k_r)
MEMORY_END

static MEMORY_WRITE32_START(p2k_writemem)
	P2K_RANGES(p2k_w)
MEMORY_END

/* The DCS2 sound board (src/wpc/wmssnd.c, docs/pin2k_sound.md). On real hardware it
   lives in the Prism card's BAR4 window at 0x13000000:

     byte offset 0, word access - command write / response read
     byte offset 0, byte access - echo, the host's liveness probe
     byte offset 2              - status flags

   The subsystem calls these two for anything in that window; they are weak there, so the
   standalone harness links without a sound board. P2K_DCSLOG=1 writes down every word it sends */
UINT32 p2k_dcs_read(UINT32 offset, UINT32 mem_mask);
void p2k_dcs_write(UINT32 offset, UINT32 data, UINT32 mem_mask);

/* Which of the games this is. The subsystem needs to know because the boot-ROM patch it
   applies sits at a different address in each, and the driver name carries the answer */
static const char *p2k_romPrefix(void) {
  const char *name = (Machine && Machine->gamedrv) ? Machine->gamedrv->name : "rfm";
  if (strncmp(name, "rfm", 3) == 0) return "rfm";       /* "rfm_160" and the rest */
  return "swep1";                                       /* "swep1_150" and the rest are the Episode I sets */
}

/* Which game's device-name tables to use: P2K_GAME_* from p2k_names.h */
static int p2k_gameIndex(void) {
  const char *p = p2k_romPrefix();
  return strncmp(p, "swep1", 5) == 0 ? P2K_GAME_SWEP1 : P2K_GAME_RFM;
}

#if P2K_DEBUG
/* P2K_SOLWATCH / P2K_LAMPWATCH / P2K_SWWATCH =1: change logs for the three kinds of device, named
   out of p2k_names.h, so walking a game's own test menu reads as names rather than bit numbers.
   Flashers come out of the solenoid watch.

   Change logs rather than periodic samples: a power stroke lasts about three frames and a sample
   walks straight past it. Sampled once per frame, in p2k_sync_io - what that does and does not
   catch, and how to read the output, is in src/p2k/README.md */
static int p2k_watch(const char *var, int *cache) {
  if (*cache < 0) *cache = getenv(var) ? 1 : 0;
  return *cache;
}
static int p2k_solwatch(void)  { static int on = -1; return p2k_watch("P2K_SOLWATCH",  &on); }
static int p2k_lampwatch(void) { static int on = -1; return p2k_watch("P2K_LAMPWATCH", &on); }
static int p2k_swwatch(void)   { static int on = -1; return p2k_watch("P2K_SWWATCH",   &on); }

/* Diff a bit array against the last call's copy and print what changed, by name. Bit b of byte i
   is device number base + i * stride + b, which is the only thing that differs between the three:
   lamps run 0,8,16.. (base 0, stride 8), solenoids 1,9,17.. (base 1, stride 8), and switches are
   PinMAME's column*10 + row (base 1, stride 10). All three are the inverse of what the core does
   with the number - core_getSw() reads (n/10)*8 + (n%10-1) as a flat bit index, and lamp n is
   physicOutputState[CORE_MODOUT_LAMP0 + n] - so a name that does not match the machine's own test
   menu means the table is wrong, not the arithmetic. Devices with no table entry print as
   "(unnamed)" rather than being skipped: an unnamed one firing is itself worth seeing */
static void p2k_watch_bits(const char *tag, const UINT8 *bits, int nbytes, UINT8 *prev, const p2k_name_t *names, int base, int stride) {
  int i, any = 0;
  for (i = 0; i < nbytes; i++) {
    const unsigned diff = (unsigned)(bits[i] ^ prev[i]);
    int b;
    if (!diff) continue;
    for (b = 0; b < 8; b++) {
      const int num = base + i * stride + b;
      const char *nm;
      if (!((diff >> b) & 1)) continue;
      nm = p2k_lookup(names, num);
      printf("[p2k %s] frame %d: %3d %-30s %s\n", tag, p2k_bringupFrame, num, nm ? nm : "(unnamed)", ((bits[i] >> b) & 1) ? "on" : "off");
      any = 1;
    }
    prev[i] = bits[i];
  }
  if (any) fflush(stdout);
}

static int p2k_dcs_log(void) {
  static int on = -1;
  if (on < 0) on = getenv("P2K_DCSLOG") ? 1 : 0;
  return on;
}
#else
#define p2k_solwatch()  0
#define p2k_lampwatch() 0
#define p2k_swwatch()   0
#define p2k_dcs_log()   0
#endif

/* The subsystem hands out dword-aligned addresses with the byte lanes in the mask, so the byte
   offset and the access width both have to be recovered from it: the lane of the first active
   byte gives the offset, the number of active bytes gives the width. Reading `offset & 3`
   directly puts every status access - byte offset 2, mask 0xffff0000 - into the echo path */
static void p2k_dcs_decode(UINT32 offset, UINT32 mem_mask, unsigned *byte_off, unsigned *width, unsigned *shift) {
  unsigned lane = 0, n = 0, i;
  while (lane < 4 && !((mem_mask >> (lane * 8)) & 0xff)) lane++;
  for (i = 0; i < 4; i++) if ((mem_mask >> (i * 8)) & 0xff) n++;
  if (lane > 3) lane = 0;
  *byte_off = ((offset & ~3u) + lane) & 3u;
  *width = n;
  *shift = lane * 8;
}

UINT32 p2k_dcs_read(UINT32 offset, UINT32 mem_mask) {
  unsigned byte_off, width, shift;
  UINT32 value;
  const char *what;
  p2k_dcs_decode(offset, mem_mask, &byte_off, &width, &shift);
  if (byte_off == 2)   { value = dcs_p2k_status_r(); what = "status"; }
  else if (width >= 2) { value = dcs_p2k_data_r();   what = "data"; }
  else                 { value = dcs_p2k_echo_r();   what = "echo"; }
  if (p2k_dcs_log())
    fprintf(stderr, "[p2k dcs] r %s -> %04x\n", what, value);
  return value << shift;
}

void p2k_dcs_write(UINT32 offset, UINT32 data, UINT32 mem_mask) {
  unsigned byte_off, width, shift;
  UINT32 value;
  const char *what;
  p2k_dcs_decode(offset, mem_mask, &byte_off, &width, &shift);
  value = (data >> shift) & (width >= 2 ? 0xffffu : 0xffu);
  what = (byte_off == 2) ? "status" : (width >= 2 ? "data" : "echo");
  if (p2k_dcs_log())
    fprintf(stderr, "[p2k dcs] w %s <- %04x\n", what, value);
  if (byte_off == 2)   dcs_p2k_status_w((UINT16)value);
  else if (width >= 2) dcs_p2k_data_w((UINT16)value);
  else                 dcs_p2k_echo_w((UINT8)value);
}

/* The pinball I/O meets PinMAME's core model here: the switch matrix goes down to the driver
   board, the lamp columns and coil bits come back. Once per frame is enough for lamps for now(!) - the core
   will integrate them anyway; if coils turn out to need finer timing, adopt here.

   //!! PWM missing

   p2k_names.h carries all tables for switch/coil/lamp mappings and every one has been stepped through the games' own test menus */
static void p2k_sync_io(void) {
  UINT8 lamps[16]; /* eight driven columns, two row banks each */
  UINT32 solenoids = 0, solenoids2 = 0;
  int i;

  /* Let the core read the keyboard into its switch columns first - it is what calls our
     SWITCH_UPDATE handler. Nothing else in this driver drives it, so without this the cabinet
     and coin door keys never reach the machine. The flipper argument enables the core's
     end-of-stroke simulation, which this board does not report. //!! ??

     Order matters here: core_updateSw() rewrites bits 0x02 and 0x08 of the flipper column from
     the flipper keys, and on this board those are the coin door switch and an unused position.
     It calls our handler afterwards, so our values win - but only in that order */
#if P2K_DEBUG
  /* The only tick per frame: SWITCH_UPDATE below reads it rather than advancing it,
     so the number means the same thing to every watch and to P2K_PLAY. It used to be advanced
     down there, behind that block's own early-out, which left it stuck at 0 for anyone who asked
     for a watch and nothing else */
  p2k_bringupFrame++;
#endif

  core_updateSw(FALSE);

  p2k_pinmame_set_dips((unsigned char)core_getDip(0));
  p2k_pinmame_push_switches((const unsigned char *)coreGlobals.swMatrix, CORE_MAXSWCOL);
  p2k_pinmame_pull_outputs(lamps, sizeof(lamps), &solenoids, &solenoids2);

  for (i = 0; i < (int)sizeof(lamps); i++)
    coreGlobals.lampMatrix[i] = lamps[i];
  coreGlobals.solenoids = solenoids;
  coreGlobals.solenoids2 = solenoids2;

#if P2K_DEBUG
  /* The watches sit here, right after this driver publishes everything, because it is the one
     place all three kinds are in hand at once and the values are the ones the rest of PinMAME
     will see. Coils in particular have to be read here and not from inside SWITCH_UPDATE: that
     runs during core_updateSw() above, before p2k_pinmame_pull_outputs() has fetched the board's
     own state, so what it sees is the core's "fake solenoids if not CPU controlled" rather than
     the machine's. pull_outputs overwrites solenoids2 whole, so the faking never reaches here */
  if (p2k_solwatch() || p2k_lampwatch() || p2k_swwatch()) {
    const int game = p2k_gameIndex();

    /* Coils and flashers. All six driver registers, which is drivers 1-48: solenoids is 1-32 and
       solenoids2's sixteen bits are 33-48, the top byte being the ones register 0x0e brings in.
       Watching that top byte is also how to tell whether the register really is those drivers -
       fit a shaker on Episode I 2.10, turn its adjustment on, and driver 43 should follow it. */
    if (p2k_solwatch()) {
      static UINT8 prev[6];
      UINT8 now[6];
      now[0] = (UINT8) (solenoids        & 0xff); now[1] = (UINT8)((solenoids  >>  8) & 0xff);
      now[2] = (UINT8)((solenoids >> 16) & 0xff); now[3] = (UINT8)((solenoids  >> 24) & 0xff);
      now[4] = (UINT8) (solenoids2       & 0xff); now[5] = (UINT8)((solenoids2 >>  8) & 0xff);
      p2k_watch_bits("sol", now, 6, prev, p2k_coil_names(game), 1, 8);
    }

    /* Lamps, sixteen bytes, but not two 8x8 matrices back to back: the board drives eight columns
       of sixteen in two row banks, handed over as bank A at byte 2c and bank B at 2c+1, so the
       manual's two matrices interleave per column rather than occupying a half each. p2k_names.h
       has the arithmetic and how it was measured */
    if (p2k_lampwatch()) {
      static UINT8 prev[16];
      p2k_watch_bits("lamp", lamps, (int)sizeof(lamps), prev, p2k_lamp_names(game), 0, 8);
    }

    /* Switches, as they went down to the board a moment ago. Optos read inverted from what the
       playfield is doing - they rest closed and open when a ball blocks them - so "off" on one of
       those is a ball arriving, not leaving */
    if (p2k_swwatch()) {
      static UINT8 prev[CORE_MAXSWCOL];
      p2k_watch_bits("sw", (const UINT8 *)coreGlobals.swMatrix, CORE_MAXSWCOL, prev, p2k_switch_names(game), 1, 10);
    }
  }
#endif
}

/* Per-channel average of two 0x00RRGGBB pixels, rounding down, without letting a channel carry
   into the one above it: the low bit of each channel is dropped by the mask before the shift.
   P2K_AVG2(a,a) == a, which is what makes the even output lines exact copies below */
#define P2K_AVG2(a,b) (((((a) ^ (b)) & 0xfefefeu) >> 1) + ((a) & (b)))

/* Per-channel average of two RGB555 pixels, rounding down.
   RGB555 layout: 0RRRRRGGGGGBBBBB. P2K_AVG2_555(a,a) == a. */
#define P2K_AVG2_555(a,b) (((((a) ^ (b)) & 0x7bdeu) >> 1) + ((a) & (b)))

/* Pinball 2000's output is the MediaGX frame buffer, 640x240 in RGB555 as the
   display controller is programmed here and what the analog monitor expects. What sits in video memory is rotated by 180 degrees
   against what a player sees - the cabinet reflects the monitor into the playfield through a
   half-silvered mirror, which accounts for one axis, and the frame buffer's row order for the
   other - so it is turned back here. What is on this screen is what a player sees.

   A PinMAME video renderer rather than a MAME VIDEO_UPDATE. The difference matters: with
   MDRV_VIDEO_UPDATE overridden, core_gen() never runs, and it is core_gen() that walks the
   display layout - which is what feeds libpinmame's display export. Declared in p2k_disp below
   as a CORE_VIDEO layout, exactly as byvidpin.c does e.g. for Baby Pac-Man */
static PINMAME_VIDEO_UPDATE(p2k_video) {
  static UINT32 frame[P2K_MAX_PIXELS];
  unsigned width, height, success, fast_path_success;

  p2k_sync_io();

  success = p2k_pinmame_frame(frame, P2K_MAX_PIXELS, &width, &height, (bitmap->depth != 32), &fast_path_success);
  if (!success || !width || !height) { fillbitmap(bitmap, 0, cliprect); return; }
  if (!video_mode_announced) {
    video_mode_announced = 1;
    fprintf(stderr, "[p2k video] %ux%u source into a %ux%u %d bpp bitmap (%s)\n",
            width, height, width, height * P2K_LINE_DOUBLE, bitmap->depth, P2K_LINE_INTERPOLATE ? "lines interpolated" : "lines doubled");
  }

  if (width > (unsigned)bitmap->width) width = bitmap->width;
  if (height * P2K_LINE_DOUBLE > (unsigned)bitmap->height)
    height = (unsigned)bitmap->height / P2K_LINE_DOUBLE;

  if (fast_path_success) // 15bpp on the input AND the output?
  {
  for (unsigned y = 0; y < height; y++) {
    const UINT32 * const __restrict src = &frame[(height - 1 - y) * width];
#if P2K_LINE_INTERPOLATE
    const UINT32 * const __restrict below = (y + 1 < height) ? &frame[(height - 2 - y) * width] : src;
#endif
    for (unsigned d = 0; d < P2K_LINE_DOUBLE; d++) {
      const unsigned ty = y * P2K_LINE_DOUBLE + d;
#if P2K_LINE_INTERPOLATE
      const UINT32 * const __restrict other = d ? below : src;
#endif
      for (unsigned x = 0; x < width; x++) {
#if P2K_LINE_INTERPOLATE
        const UINT32 srcx = src[x];
        const UINT32 otherx = other[x];
        const UINT16 rgb = P2K_AVG2_555(srcx, otherx);
#else
        const UINT16 rgb = src[x];
#endif
        ((UINT16 *)bitmap->line[ty])[x] = rgb;
      }
    }
  }
  }
  else // slow path / conversion
  for (unsigned y = 0; y < height; y++) {
    /* Video memory holds the picture upside down and the right way round, not mirrored:
       turning only the row order back makes the machine's own text (e.g. COIN DOOR IS OPEN /
       Revenge From Mars - 50070 - 1.5) legible. Measured by reading the bitmap back, and
       worth stating because it is the opposite of what the cabinet's half-silvered mirror would suggest */
    const UINT32 * const __restrict src = &frame[(height - 1 - y) * width];
#if P2K_LINE_INTERPOLATE
    /* The line below this one on screen, which - the row order being turned back - is the row
       before it in video memory. The bottom line of the frame has none, and pairs with itself:
       P2K_AVG2 then returns it unchanged, so that one line is doubled */
    const UINT32 * const __restrict below = (y + 1 < height) ? &frame[(height - 2 - y) * width] : src;
#endif
    for (unsigned d = 0; d < P2K_LINE_DOUBLE; d++) {
      const unsigned ty = y * P2K_LINE_DOUBLE + d;
#if P2K_LINE_INTERPOLATE
      /* d == 0 is the machine's own line - averaged with itself, so copied exactly. d == 1 is
         the new line, and averaging is all that separates the two cases */
      const UINT32 * const __restrict other = d ? below : src;
#endif
      for (unsigned x = 0; x < width; x++) {
#if P2K_LINE_INTERPOLATE
        const UINT32 srcx = src[x];
        const UINT32 otherx = other[x];
        const UINT32 rgb = P2K_AVG2(srcx, otherx);
#else
        const UINT32 rgb = src[x];
#endif
        if (bitmap->depth == 32)
          ((UINT32 *)bitmap->line[ty])[x] = rgb;
        else /* 15 bit direct: RGB555 */
          ((UINT16 *)bitmap->line[ty])[x] = (UINT16)((((rgb >> 19) & 0x1f) << 10) |
                                                     (((rgb >> 11) & 0x1f) << 5) |
                                                      ((rgb >> 3) & 0x1f));
      }
    }
  }
  height *= P2K_LINE_DOUBLE;          /* from here on, what is in the bitmap */

#if P2K_DEBUG
  /* P2K_VIDEO_PPM=<path>: read the bitmap back once and write it out. Proof that what PinMAME
     holds is a picture, and that the pixel format written into it is the one it expects */
  {
    static int frames = 0;
    static int lastAt = -1;
    const char *out = getenv("P2K_VIDEO_PPM");
    const char *when = getenv("P2K_VIDEO_PPM_AT");
    int hit = 0;
    char path[512];
    frames = p2k_bringupFrame ? p2k_bringupFrame : frames + 1;
    /* P2K_VIDEO_PPM_AT takes a comma separated list; each frame lands in <path>.<frame>.ppm so a sequence can be followed, not just one moment */
    if (out && when) {
      const char *q = when;
      while (*q) {
        const int at = (int)strtol(q, NULL, 10);
        if (at > lastAt && frames >= at) { hit = 1; lastAt = at; break; }
        q = strchr(q, ','); if (!q) break; q++;
      }
    } else if (out && frames >= 600 && lastAt < 0) { hit = 1; lastAt = 600; }
    if (hit) {
      FILE *f;
      snprintf(path, sizeof path, "%s.%d.ppm", out, lastAt);
      f = fopen(path, "wb");
      if (f) {
        fprintf(f, "P6\n%u %u\n255\n", width, height);
        for (unsigned y = 0; y < height; y++) {
          for (unsigned x = 0; x < width; x++) {
            unsigned r, g, b;
            if (bitmap->depth == 32) {
              const UINT32 c = ((UINT32 *)bitmap->line[y])[x];
              r = (c >> 16) & 0xff; g = (c >> 8) & 0xff; b = c & 0xff;
            } else {
              const UINT16 c = ((UINT16 *)bitmap->line[y])[x];
              r = ((c >> 10) & 0x1f) << 3; g = ((c >> 5) & 0x1f) << 3; b = (c & 0x1f) << 3;
            }
            fputc((int)r, f); fputc((int)g, f); fputc((int)b, f);
          }
        }
        fclose(f);
        fprintf(stderr, "[p2k video] wrote %s from the PinMAME bitmap\n", path);
      }
    }
  }
#endif
}

static SWITCH_UPDATE(p2k); /* defined with the input ports below */

/* The optos, per game, terminated by 0 - see the note at the top of SWITCH_UPDATE for where the
   lists come from. They rest closed, so anything that walks the matrix has to know which they are:
   driving an opto means taking it to 0, and every other switch to 1 */
static const int *p2k_optoList(void) {
  static const int rfmOptos[]   = {41,42,43,44,45,46,47,51,52,53,54,55,56,0};
  static const int swep1Optos[] = {41,42,43,44,45,46,47,48,51,52,58,0};
  return (strncmp(p2k_romPrefix(), "swep1", 5) == 0) ? swep1Optos : rfmOptos;
}
static int p2k_isOpto(int sw) {
  const int *o = p2k_optoList();
  int k;
  for (k = 0; o[k]; k++) if (o[k] == sw) return 1;
  return 0;
}

/* Switch numbering. PinMAME's base driver installs a *sequential* scheme (core_swSeq2m: matrix
   index = number + 7), but this machine numbers its switches by column and row - and so does the
   remote debugger, which emits num = column*10 + row + 1. Without this, the numbers in the game's
   own tables land somewhere else entirely: core_setSw(13) for the start button ended up at column
   2 row 5, which is Left Loop (Low). Measured, not guessed - the row the board handed out was
   0x10 on column 1 instead of 0x04 on column 0.

   Row is 0-based coming in from the matrix side and 1-based in a switch number, hence the +1 */
static int p2k_sw2m(int no)           { return (no / 10) * 8 + (no % 10) - 1; }
static int p2k_m2sw(int col, int row) { return col * 10 + row + 1; }

/* What survives a power cycle. The machine keeps three things: the CMOS it stores settings, audits
   and its error log in, the PLX EEPROM behind the PCI bridge, and the 8 MB update flash. The first
   two are persisted here; the update flash is not: it comes from the ROM set, and an 8 MB NVRAM
   file would quietly take precedence over it.

   PinMAME reads its NVRAM file before the machine is built and writes it after the machine is
   gone, so the bytes live in these buffers in between: the handler fills them on load, MACHINE_INIT
   pushes them into the machine, and the handler pulls them back out on save */

/* P2K_NV_CMOS_SIZE / P2K_NV_EEPROM_SIZE come from p2k_public.h - the subsystem allocates the
   blocks behind them, and the two sizes have to match */
static UINT8 p2k_nvCmos[P2K_NV_CMOS_SIZE];
static UINT8 p2k_nvEeprom[P2K_NV_EEPROM_SIZE];
static UINT8 p2k_nvRtc[P2K_NV_RTC_SIZE];
static int   p2k_nvLoaded = 0;

static NVRAM_HANDLER(p2k) {
  /* No harvesting here on the way out - by the time PinMAME saves, the machine is gone. See MACHINE_STOP below, which is where the bytes are taken */
  core_nvram(file, read_or_write, p2k_nvCmos,   P2K_NV_CMOS_SIZE,   0x00);
  core_nvram(file, read_or_write, p2k_nvEeprom, P2K_NV_EEPROM_SIZE, 0x00);
  core_nvram(file, read_or_write, p2k_nvRtc,    P2K_NV_RTC_SIZE,    0x00);
  /* Only adopt these if they really came from a file. With no file core_nvram() fills the
     buffers, and pushing those into the machine would wipe the PLX EEPROM defaults
     that reset() writes - which stops the machine booting at all */
  if (!read_or_write) p2k_nvLoaded = (file != NULL);
}

#if !P2K_DEBUG
#define p2k_dumpNames() ((void)0)
#else
/* P2K_NAMES=1 prints the machine's own device map once at startup: PinMAME switch number, coil
   driver number and lamp number against the names the game itself uses. PinMAME has nowhere to
   hang these, so this is the one place they are visible without a debugger that knows about them.

   All games' tables are in p2k_names.h and the running set picks between them */
static void p2k_dumpNames(void) {
  if (getenv("P2K_NAMES"))
  {
    const int game = p2k_gameIndex();
    const p2k_name_t *t;
    int i;
    printf("[p2k names] %s\n", game == P2K_GAME_SWEP1  ? "Star Wars Episode I" :
                               "Revenge From Mars");
    printf("[p2k names] switches (PinMAME number = column*10 + row):\n");
    for (t = p2k_switch_names(game), i = 0; t[i].name; i++)
      printf("   sw %3d  %s\n", t[i].num, t[i].name);
    printf("[p2k names] coils (driver number, solenoid bit = driver - 1):\n");
    for (t = p2k_coil_names(game), i = 0; t[i].name; i++)
      printf("   coil %2d  %s\n", t[i].num, t[i].name);
    printf("[p2k names] lamps:\n");
    for (t = p2k_lamp_names(game), i = 0; t[i].name; i++)
      printf("   lamp %2d  %s\n", t[i].num, t[i].name);
    fflush(stdout);
  }
}
#endif

static MACHINE_INIT(p2k) {
  video_mode_announced = 0;

  p2k_dumpNames();
  p2k_pinmame_start(memory_region(P2K_PRISMREGION), (unsigned int)memory_region_length(P2K_PRISMREGION),
                    memory_region(P2K_UPDREGION), (unsigned int)memory_region_length(P2K_UPDREGION));
  if (p2k_nvLoaded) { /* whatever PinMAME had on file, after the reset that fills in the EEPROM defaults */
    p2k_pinmame_nvram_set(P2K_NV_BLOCK_CMOS,   p2k_nvCmos,   P2K_NV_CMOS_SIZE);
    p2k_pinmame_nvram_set(P2K_NV_BLOCK_EEPROM, p2k_nvEeprom, P2K_NV_EEPROM_SIZE);
    p2k_pinmame_nvram_set(P2K_NV_BLOCK_RTC,    p2k_nvRtc,    P2K_NV_RTC_SIZE);
  }
  /* The clock comes from the host every start, so the machine shows real time rather than whatever
     it had when last switched off. A machine that already has a CMOS keeps its year register: the
     firmware adds that register to its own stored year, so refreshing it would climb! */
  p2k_pinmame_clock_from_host(p2k_nvLoaded);
  sndbrd_0_init(SNDBRD_DCSP2K, DCS_CPUNO, memory_region(DCS_ROMREGION), NULL, NULL);
}
/* Take the machine's persistent blocks before letting go of it.
/
/  run_machine_core() saves NVRAM *after* cpu_run() returns, and cpu_run()'s own cpu_post_run()
/  calls machine_stop first - so the driver's NVRAM handler runs when there is no machine left to
/  ask. Harvesting in the handler's save branch, which is the obvious place, wrote a file of
/  196828 zero bytes: nothing was saved, and restoring those zeros over the PLX EEPROM defaults
/  stopped the next boot dead at "STARTING UPDATE GAME CODE" */
static MACHINE_STOP(p2k) {
  p2k_pinmame_nvram_get(P2K_NV_BLOCK_CMOS,   p2k_nvCmos,   P2K_NV_CMOS_SIZE);
  p2k_pinmame_nvram_get(P2K_NV_BLOCK_EEPROM, p2k_nvEeprom, P2K_NV_EEPROM_SIZE);
  p2k_pinmame_nvram_get(P2K_NV_BLOCK_RTC,    p2k_nvRtc,    P2K_NV_RTC_SIZE);
  p2k_pinmame_stop();
}

MACHINE_DRIVER_START(p2k)
	MDRV_IMPORT_FROM(PinMAME)
	MDRV_CORE_INIT_RESET_STOP(p2k, NULL, p2k)
	MDRV_SWITCH_UPDATE(p2k)
	MDRV_SWITCH_CONV(p2k_sw2m, p2k_m2sw)
	MDRV_NVRAM_HANDLER(p2k)
	MDRV_CPU_ADD_TAG("mcpu", MEDIAGX, 233000000/3) //!! sync with p2k_pinmame.cpp
	MDRV_CPU_MEMORY(p2k_readmem, p2k_writemem)
	MDRV_IMPORT_FROM(wmssnd_dcs3)
	/* 480 rows, not the 240 the frame buffer holds: core_initDisplaySize() forces the visible
	   height to Machine->drv->screen_height, so the layout's row count alone does not size the window. See P2K_LINE_DOUBLE */
	MDRV_SCREEN_SIZE(640, 240 * P2K_LINE_DOUBLE)
	MDRV_VISIBLE_AREA(0, 639, 0, 240 * P2K_LINE_DOUBLE - 1)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
MACHINE_DRIVER_END

/* The cabinet and coin door, in the shape the pinball driver board reports them. The board keeps
   these apart from the playfield matrix: index 0x00 is the coin inputs, 0x01 the cabinet, 0x03 the
   coin door's diagnostic buttons - so they are carried in PinMAME's dedicated columns 10, 11 and a
   twelfth column for the diagnostics, and p2k_switch_update() below fills them.

   Bit assignments are MAME's (src/mame/drivers/pinball2k.cpp, its keyboard handler); keys follow
   PinMAME's WPC convention instead of MAME's, so 7/8/9/0 are Enter/Up/Down/Escape and END toggles
   the coin door. The board's own coin door bit reads "closed" when set - the machine reports COIN
   DOOR IS OPEN with the bit clear, which is the state it powers up */
INPUT_PORTS_START(rfm)
	CORE_PORTS
	/* SIM_PORTS(1) - PinMAME's built-in ball simulator, commented out rather than removed so it is
	   easy to put back. It does nothing here: p2kGameData passes NULL for simData, so the simulator
	   never runs, and this driver reads only CORE_COREINPORT. What it did do was list "Balls" and
	   "Spinner time" in the Dip Switches menu - COREPORT_DIPNAME is the same macro a real DIP uses,
	   so MAME cannot tell them apart from the country switches below - and bind Shoot Ball (SPACE),
	   Ignore Location, Switch/Simulator, Next Ball and Prev Ball to keys that answer to nothing.
	   Ball handling here is the P2K_TROUGH scaffolding in SWITCH_UPDATE instead. It adds bits to
	   the port CORE_PORTS already opened rather than starting one, so leaving it out does not move
	   CORE_COREINPORT or the DIP port after it */
	PORT_START /* 2: CORE_COREINPORT */
		COREPORT_BITDEF(  0x0001, IPT_COIN1,        IP_KEY_DEFAULT)
		COREPORT_BITDEF(  0x0002, IPT_COIN2,        IP_KEY_DEFAULT)
		COREPORT_BITDEF(  0x0004, IPT_COIN3,        KEYCODE_3)
		COREPORT_BITDEF(  0x0008, IPT_COIN4,        KEYCODE_4)
		COREPORT_BIT(     0x0010, "Enter",          KEYCODE_7)
		COREPORT_BIT(     0x0020, "Up",             KEYCODE_8)
		COREPORT_BIT(     0x0040, "Down",           KEYCODE_9)
		COREPORT_BIT(     0x0080, "Escape",         KEYCODE_0)
		COREPORT_BITTOG(  0x0100, "Coin Door",      KEYCODE_END)
		COREPORT_BITDEF(  0x0200, IPT_START1,       IP_KEY_DEFAULT)
		COREPORT_BIT(     0x0400, "Plumb Tilt",     KEYCODE_INSERT)
		COREPORT_BIT(     0x0800, "Slam Tilt",      KEYCODE_HOME)
		COREPORT_BIT(     0x1000, "Left Action",    KEYCODE_LCONTROL)
		COREPORT_BIT(     0x2000, "Right Action",   KEYCODE_RCONTROL)
		COREPORT_BIT(     0x4000, "Launch Ball",    KEYCODE_ENTER)
		COREPORT_BITTOG(  0x8000, "Balls In Trough",KEYCODE_B)
	/* The power driver board DIP switches. Only 1-4 are read: they form a 4 bit country code that
	   picks the pricing table, which is what the changelogs mean by "the country dipswitch setting".
	   Measured on rfm_160 by setting each combination and reading the machine's own DIP Switch Test:
	   a closed switch is a set bit, 1 being the low one. 5-8 do nothing. Values 5, 6 and 9-15 the
	   machine itself calls Unused, so they are not offered here.
	   USA/Canada is the default. A country that disagrees with the one in CMOS is what XINA 1.02 warns about */
	PORT_START /* 3: DIP switches, read back through core_getDip(0) */
		COREPORT_DIPNAME( 0x000f, 0x0000, "Country (DIP 1-4)")
			COREPORT_DIPSET(0x0000, "USA / Canada" )
			COREPORT_DIPSET(0x0001, "Germany" )
			COREPORT_DIPSET(0x0002, "France" )
			COREPORT_DIPSET(0x0003, "United Kingdom" )
			COREPORT_DIPSET(0x0004, "Spain" )
			COREPORT_DIPSET(0x0007, "Europe" )
			COREPORT_DIPSET(0x0008, "Japan" )
INPUT_PORTS_END

#define input_ports_swep1 input_ports_rfm

/*-- Switch numbers, read out of the game's own switch table: every handler record carries the
     number, and number = 100 + (column-1)*8 + (row-1). Columns 1-9 are the playfield matrix,
     column 10 the coin door's diagnostic buttons, column 11 the cabinet - which is exactly how
     the driver board reports them (registers 0x04, 0x03 and 0x01). Two independent checks that
     the scheme is right: the start button lands at column 1 row 3, the one coordinate MAME's
     driver spells out, and column 11 rows 5-8 are right flipper, left flipper, right action,
     left action - MAME's cabinet register bits 4 to 7. See src/p2k/README.md for the full list --*/
#define swStartButton      13
/* The launch button sits in the playfield matrix rather than the cabinet column, and both games'
   tables name it. Revenge From Mars acts on it: Episode I is fitted with a hand plunger and
   has no autoplunger coil, which is what P2K_HANDPLUNGE in the scaffolding below stands in for */
#define swLaunchButton     23
#define swEscape          101
#define swDown            102
#define swUp              103
#define swEnter           104
#define swSlamTilt        111
#define swCoinDoorClosed  112
#define swPlumbTilt       113
#define swRFlipperButton  115
#define swLFlipperButton  116
#define swRActionButton   117
#define swLActionButton   118

/* The trough as built: four balls on 42-45, the same on all games. Nothing here puts them there
   on its own - an empty machine is the honest rest state, and PinMAME leaves the trough to a
   simulator or to the operator elsewhere too. The Balls In Trough key is the way to load it
   for a test, and the ball model in the scaffolding is the way to make them move */
#define P2K_TROUGH_SW1     42
#define P2K_TROUGH_BALLS    4

/*-- the driver board's own bit numbering, from MAME's driver --*/
#define P2K_CAB_SLAMTILT   0x01
#define P2K_CAB_COINDOOR   0x02
#define P2K_CAB_PLUMBTILT  0x04
#define P2K_CAB_RFLIPPER   0x10
#define P2K_CAB_LFLIPPER   0x20
#define P2K_CAB_RACTION    0x40
#define P2K_CAB_LACTION    0x80

/* Up and Down are the other way round from MAME's header comment - measured on the running game
   (holding 0x02 walks the master volume down, 0x04 walks it up) and confirmed by the game's own //!!
   switch table, which names column 10 rows 1 to 4 Escape, Down, Up, Enter */
#define P2K_DIAG_ESCAPE    0x01
#define P2K_DIAG_DOWN      0x02
#define P2K_DIAG_UP        0x04
#define P2K_DIAG_ENTER     0x08

static SWITCH_UPDATE(p2k) {
	/* Every opto at rest, before anything else runs.
	/
	/  An opto reads 1 when nothing is blocking it, and a matrix position this driver never sets
	/  reads 0 - which is a blocked or broken one as far as the machine is concerned. So all of them
	/  start at 1 and the ball model below moves the few it owns.
	/
	/  Which ones they are comes from the operations manuals, which name them outright - Revenge
	/  From Mars page 3-21, "the individual playfield opto switches are", then switches 46, 47, 51
	/  and 52 by number, plus the ball trough assembly's own five on page 3-19:
	/
	/      Revenge From Mars  41-47 trough jam, trough 1-4, right popper, jet exit
	/                         51 right lockup, 52 left ramp entrance
	/                         53-56 as well from 2.60, the expansion trough and lock
	/      SW Episode I       41-47 trough jam, trough 1-4, left and right ramp enter
	/                         51 shield popper, 58 shield hit
	/                         48 and 52 as well from 2.10, its expansion trough
	/
	/  47 and 52 were missing before this - Jet Exit and Left Ramp Entrance sat at 0 on every RFM
	/  set, which is a jet exit and a ramp entrance permanently reporting a ball. The manual names
	/  both, so that was right.
	/
	/  Revenge From Mars has since been checked against a machine as well: rfm_260's own switch test
	/  reports every opto as "norm closed", and the ones it reports are the ones in the list above -
	/  including 53-56, which came from 2.60's switch table rather than a manual and had not been
	/  confirmed until then. Episode I has since been checked the same way, including the optos that
	/  only its later revisions have: every switch the list marks reports as closed in the machine's
	/  own switch test, and nothing else does. Both games' lists are now machine-verified rather than
	/  taken from paper. That test is the way to check any of them: an opto rests closed, so it names
	/  itself - and it only became possible to walk those menus on 2.00, 2.01 and 2.10 once the UART
	/  divisor latch bug was fixed, which had been wedging them a few seconds in (see src/p2k/README.md)
	/
	/  Note: the switch table was assumed to carry an opto flag at +0x0c bit
	/  0x800 and that it agreed with the manuals. It does not. That bit - +0x1c from the record
	/  start used elsewhere in this file - gives 38, 41-46 and 51 on rfm_160, which both misses 47
	/  and 52 and adds 38, Up/Down Ramp Up. On Episode I it flags 38 Right Saucer, which is a plain
	/  switch. So it marks something else - every game's flagged set is ball-in-device detectors,
	/  so a device or ball-tracking bit is the likely reading - and it cannot be used to derive an
	/  opto list */
	{
		const int *o = p2k_optoList();
		int k;
		for (k = 0; o[k]; k++) core_setSw(o[k], 1);

		/* Balls In Trough (B) loads the trough for a test, and is a toggle because that is what it
		   models - balls are in the machine or they are not. Both games hold four as built, on
		   42-45; the 6 ball kits' extra positions are left clear, a stock playfield not having them.

		   Worth knowing why this is a key and not a default. An opto rests at 1 and a matrix
		   position nobody sets reads 0, so before the optos above were brought up to rest the
		   trough read *full* by accident - four balls that were really four unset switches, which
		   is why a game could be started without asking for any. Resting them correctly empties the
		   machine, and an empty machine is the honest state to power up in.

		   Do not hold it down from frame zero: a full trough before the machine has initialised is
		   not four balls, it is four switches that broke during power-up, and it may hang. Let it boot
		   first. The ball model in the scaffolding owns 42-45 whenever it runs and overrides this */
		if (inports && (inports[CORE_COREINPORT] & 0x8000))
			for (k = 0; k < P2K_TROUGH_BALLS; k++) core_setSw(P2K_TROUGH_SW1 + k, 0);
	}

	if (inports) {
		const int in = inports[CORE_COREINPORT];
		const int flip = inports[CORE_FLIPINPORT];

		/* Coin inputs. The board does not put them in the low bits: MAME's driver has the three
		   coin keys on bits 1, 2 and 7 of register 0x00, and setting bit 0 does nothing at all -
		   which is exactly what a coin mapped there did here. The fourth port bit has no known
		   position, so it stays unmapped rather than guessed */
		coreGlobals.swMatrix[CORE_COINDOORSWCOL] =
			((in   & 0x0001) ? 0x02              : 0) |
			((in   & 0x0002) ? 0x04              : 0) |
			((in   & 0x0004) ? 0x80              : 0);

		coreGlobals.swMatrix[11] =
			((in   & 0x0800) ? P2K_CAB_SLAMTILT  : 0) |
			((in   & 0x0100) ? P2K_CAB_COINDOOR  : 0) |
			((in   & 0x0400) ? P2K_CAB_PLUMBTILT : 0) |
			((flip & CORE_LRFLIPKEY) ? P2K_CAB_RFLIPPER : 0) |
			((flip & CORE_LLFLIPKEY) ? P2K_CAB_LFLIPPER : 0) |
			((in   & 0x2000) ? P2K_CAB_RACTION   : 0) |
			((in   & 0x1000) ? P2K_CAB_LACTION   : 0);

		coreGlobals.swMatrix[10] =
			((in   & 0x0080) ? P2K_DIAG_ESCAPE   : 0) |
			((in   & 0x0020) ? P2K_DIAG_UP       : 0) |
			((in   & 0x0040) ? P2K_DIAG_DOWN     : 0) |
			((in   & 0x0010) ? P2K_DIAG_ENTER    : 0);

		core_setSw(swStartButton, in & 0x0200);
		core_setSw(swLaunchButton, (in & 0x4000) ? 1 : 0);
#if P2K_DEBUG
		p2k_keyLaunch = (in & 0x4000) ? 1 : 0;
#endif
	}

#if P2K_DEBUG
	/* ---------------------------------------------------------------------------------------
	   BRING-UP SCAFFOLDING - not part of the driver

	   A standalone build has no playfield, and the machine will not run without one: its ball
	   devices do not merely read switches, they kick and then wait for switches to change. With
	   nothing answering, a device process stays alive forever and MultiDevice::game_start_check
	   refuses to start a game around it. On real hardware this feedback is the playfield; under
	   VPinMAME/libpinmame it is the table. This is a stand-in good enough to satisfy the devices.

	   Two constraints, both measured and both easy to get wrong:

	   - Optos rest at a 1, and nothing inverts on the way in. Which switches those are is in the
	     game's own table - flag at +0x0c, bit 0x800 - and they are all brought up at rest at the
	     top of SWITCH_UPDATE, so only the trough is moved here. Switch 18, the shooter lane, is
	     not an opto.
	   - Start at rest and change nothing until the machine has initialised. Presenting a full
	     trough from frame zero is not "four balls in the machine", it is four switches that broke
	     during power-up, and it hangs the machine outright. P2K_BALLSAT (default 1500) is when
	     the balls appear.

	   P2K_TROUGH=<n>      n balls, placed in the trough once the machine is up
	   P2K_BALLSAT=<frame> when that happens
	   P2K_DRAIN=<frames>  how long a launched ball stays in play (default 300)
	   P2K_LANEHOLD=<f>    how long the lane switch stays closed after the autoplunger (default 20)
	   P2K_HANDPLUNGE=<f>  for a machine with no autoplunger coil - swep1 has none - let a ball
	                       leave the shooter lane by itself after f frames, as a person would
	   P2K_DOORCLOSE=<f>   hold the coin door open until here, then close it - the *edge* is what
	                       brings up high voltage (Game::m_sw_coin_door_callback_irq), a door that
	                       was never open produces none and no coil ever fires
	   P2K_PLAY="<frame>:<what>[:<hold>],..." with what one of coin, start, enter, up, down, esc,
	                       coinb<n> for one bit of the coin register on its own (coin is bit 1 -
	                       the three slots are not in the low bits, so this is how a position gets
	                       checked instead of assumed),
	                       or sw<number> for any switch in the matrix - "1200:sw37:10" closes 37 at
	                       frame 1200 for ten frames, so a target or a jet needs no code of its own.
	                       Sixteen sw entries at most. The per-event hold matters: a coin held too
	                       long is not counted at all (8 frames gives credits, 120 gives none), the
	                       start button wants more.
	   P2K_SWSWEEP=<first>-<last>[:<period>[:<hold>]]
	                       Walk the matrix, one switch at a time, for checking the names in
	                       p2k_names.h against the ones the game's own switch test puts on screen.
	                       Every <period> frames (default 120) it takes the next switch in the range
	                       to its active state for <hold> frames (default 30), skipping rows 0 and 9,
	                       which do not exist. Optos are inverted for you. Starts at P2K_SWSWEEPAT
	                       (default 1800, i.e. after the machine is up - drive one earlier and it
	                       reads as a switch that broke during power-up). Unlike P2K_PLAY there is no
	                       count limit, so "P2K_SWSWEEP=11-88" with P2K_SWWATCH=1 walks a whole
	                       playfield in one run. Navigate to the switch test by hand while it counts
	                       down; the sweep repeats from the start when it reaches the end.
	   --------------------------------------------------------------------------------------- */
	{
		enum { B_TROUGH, B_TO_LANE, B_LANE, B_LEAVING, B_PLAY };
		static int st[8], due[8], nballs = -1, placed = 0;
		static int ejectWas = 0, plungeWas = 0;
		static int laneSince[8];
		const char *tr = getenv("P2K_TROUGH");
		const char *seq = getenv("P2K_PLAY");
		const char *dc = getenv("P2K_DOORCLOSE");
		const char* sweep = getenv("P2K_SWSWEEP");
		int frame;
		/* Nothing here costs anything unless it is asked for. Everything below is behind one of
		   these three, and a release build simply never enters the block */
		if (!tr && !seq && !dc && !sweep) return;

		frame = p2k_bringupFrame;   /* advanced once per frame in p2k_sync_io */

		/* P2K_SWSWEEP: one switch at a time across the range, so the game's switch test can be
		   read against p2k_names.h without a keypress per switch. Rows 0 and 9 are skipped -
		   core_getSw() reads (n/10)*8 + (n%10-1), which has no bit for either. The previous one is
		   released before the next is driven, and an opto is driven the other way round, both
		   because core_setSw() latches and a switch left in its active state reads as stuck */
		if (sweep) {
			static int lastSw = 0;
			int lo = 11, hi = 88, period = 120, hold = 30, at = 1800;
			const char *p2 = strchr(sweep, '-');
			const char *c3;
			lo = (int)strtol(sweep, NULL, 10);
			if (p2) hi = (int)strtol(p2 + 1, NULL, 10);
			if ((c3 = strchr(sweep, ':')) != NULL) {
				period = (int)strtol(c3 + 1, NULL, 10);
				if ((c3 = strchr(c3 + 1, ':')) != NULL) hold = (int)strtol(c3 + 1, NULL, 10);
			}
			if (period < 1) period = 1;
			if (getenv("P2K_SWSWEEPAT")) at = (int)strtol(getenv("P2K_SWSWEEPAT"), NULL, 10);
			if (frame >= at) {
				/* how many valid numbers are in [lo,hi], and which one is this step's */
				const int step = (frame - at) / period;
				const int into = (frame - at) % period;
				int n, count = 0, want = 0;
				for (n = lo; n <= hi; n++) { if (n % 10 == 0 || n % 10 == 9) continue; count++; }
				if (count > 0) {
					int seen = 0;
					for (n = lo; n <= hi; n++) {
						if (n % 10 == 0 || n % 10 == 9) continue;
						if (seen == step % count) { want = n; break; }
						seen++;
					}
				}
				if (lastSw && lastSw != want) { core_setSw(lastSw, p2k_isOpto(lastSw) ? 1 : 0); lastSw = 0; }
				if (want && into < hold) { core_setSw(want, p2k_isOpto(want) ? 0 : 1); lastSw = want; }
				else if (want && lastSw == want) { core_setSw(want, p2k_isOpto(want) ? 1 : 0); lastSw = 0; }
			}
		}

		if (seq || dc) {
			const char *hs = getenv("P2K_PLAYHOLD");
			const int hold = hs ? (int)strtol(hs, NULL, 10) : 8;
			const char *p = seq;
			int coin = 0, diag = 0, start = 0, cab = 0, launch = 0;
			/* Any playfield switch can be pulsed as "sw<number>", so a target or a jet needs no
			   code of its own. They are collected once and cleared every frame before the active
			   ones are set - core_setSw() latches, and a switch left closed reads as stuck */
			static int swList[16], swCount = -1;
			int k;
			if (swCount < 0) {
				const char *q = seq;
				swCount = 0;
				while (q && *q && swCount < 16) {
					const char *c2 = strchr(q, ':');
					if (c2 && !strncmp(c2 + 1, "sw", 2)) {
						const int n = (int)strtol(c2 + 3, NULL, 10);
						if (n > 0) swList[swCount++] = n;
					}
					q = strchr(q, ','); if (q) q++;
				}
			}
			for (k = 0; k < swCount; k++) core_setSw(swList[k], 0);
			if (dc && frame >= (int)strtol(dc, NULL, 10)) cab |= P2K_CAB_COINDOOR;
			while (p && *p) {
				const int at = (int)strtol(p, NULL, 10);
				const char *c = strchr(p, ':');
				int thisHold = hold;
				if (c) { const char *h = strchr(c + 1, ':');
				         if (h && (!strchr(p, ',') || h < strchr(p, ','))) thisHold = (int)strtol(h + 1, NULL, 10); }
				if (c && frame >= at && frame < at + thisHold) {
					c++;
					/* coinb<n> drives one bit of the coin register on its own, which is how the
					   three known positions were checked - "coin" is bit 1, the one MAME uses */
					if      (!strncmp(c, "coinb",   5)) coin |= 1 << (int)strtol(c + 5, NULL, 10);
					else if (!strncmp(c, "coin",    4)) coin |= 0x02;
					else if (!strncmp(c, "start",   5)) start = 1;
					else if (!strncmp(c, "enter",   5)) diag |= P2K_DIAG_ENTER;
					else if (!strncmp(c, "up",      2)) diag |= P2K_DIAG_UP;
					else if (!strncmp(c, "down",    4)) diag |= P2K_DIAG_DOWN;
					else if (!strncmp(c, "esc",     3)) diag |= P2K_DIAG_ESCAPE;
					else if (!strncmp(c, "lflip",   5)) cab  |= P2K_CAB_LFLIPPER;
					else if (!strncmp(c, "rflip",   5)) cab  |= P2K_CAB_RFLIPPER;
					else if (!strncmp(c, "laction", 7)) cab  |= P2K_CAB_LACTION;
					else if (!strncmp(c, "raction", 7)) cab  |= P2K_CAB_RACTION;
					/* the launch button is a playfield switch, not a cabinet one - the game
					   fires the autoplunger off it once a ball waits in the shooter lane */
					else if (!strncmp(c, "launch",  6)) launch = 1;
					else if (!strncmp(c, "sw",      2)) core_setSw((int)strtol(c + 2, NULL, 10), 1);
					if (frame == at) { printf("[p2k play] %.7s at frame %d\n", c, frame); fflush(stdout); }
				}
				p = strchr(p, ','); if (p) p++;
			}
			coreGlobals.swMatrix[CORE_COINDOORSWCOL] = coin;
			coreGlobals.swMatrix[10] = diag;
			coreGlobals.swMatrix[11] = cab;
			core_setSw(swStartButton, start);
			/* OR rather than assign: this runs after the keyboard above and writes the
			   switch every frame, so assigning would make the Launch Ball key dead in exactly
			   the configuration that needs it - the ball model on, the player at the keyboard */
			core_setSw(swLaunchButton, launch || p2k_keyLaunch);
		}

		if (tr) {
			const char *ba = getenv("P2K_BALLSAT");
			const int ballsAt  = ba ? (int)strtol(ba, NULL, 10) : 1500;
			const char *dr = getenv("P2K_DRAIN");
			const int playtime = dr ? (int)strtol(dr, NULL, 10) : 300;
			const char *lh = getenv("P2K_LANEHOLD");
			const int lanehold = lh ? (int)strtol(lh, NULL, 10) : 20;
			const char *hp = getenv("P2K_HANDPLUNGE");
			const int handplunge = hp ? (int)strtol(hp, NULL, 10) : 0;
			const int eject  = (coreGlobals.solenoids & (1u << 8))  ? 1 : 0; /* driver 9  */
			const int plunge = (coreGlobals.solenoids & (1u << 14)) ? 1 : 0; /* driver 15 */
			int inTrough = 0, busy = 0, lane = 0;
			int i;

			if (nballs < 0) { nballs = (int)strtol(tr, NULL, 10);
			                  if (nballs < 0) nballs = 0; if (nballs > 8) nballs = 8; }
			if (!placed && frame >= ballsAt) {
				placed = 1;
				for (i = 0; i < nballs; i++) { st[i] = B_TROUGH; due[i] = 0; }
				printf("[p2k ball] %d balls into the trough at frame %d\n", nballs, frame);
				fflush(stdout);
			}
			for (i = 0; placed && i < nballs; i++) {
				if (st[i] == B_TO_LANE && frame >= due[i]) {
					st[i] = B_LANE;
					laneSince[i] = frame;
					printf("[p2k ball] %d in the shooter lane at %d\n", i, frame); fflush(stdout);
				}
				if (st[i] == B_LEAVING && frame >= due[i]) {
					st[i] = B_PLAY; due[i] = frame + playtime;
					printf("[p2k ball] %d left the lane at %d\n", i, frame); fflush(stdout);
				}
				if (st[i] == B_PLAY && frame >= due[i]) {
					st[i] = B_TROUGH;
					printf("[p2k ball] %d drained at %d\n", i, frame); fflush(stdout);
				}
			}
			for (i = 0; placed && i < nballs; i++) {
				if (st[i] == B_TROUGH)  inTrough++;
				if (st[i] == B_TO_LANE) busy++;
				if (st[i] == B_LANE || st[i] == B_LEAVING) lane++;
			}
			if (eject && !ejectWas && placed && !busy && !lane)
				for (i = 0; i < nballs; i++)
					if (st[i] == B_TROUGH) {
						st[i] = B_TO_LANE; due[i] = frame + 30;
						printf("[p2k ball] %d ejected at %d, %d left\n", i, frame, inTrough - 1);
						fflush(stdout);
						break;
					}
			ejectWas = eject;
			if (plunge && !plungeWas)
				for (i = 0; i < nballs; i++)
					if (st[i] == B_LANE) {
						st[i] = B_LEAVING; due[i] = frame + lanehold;
						printf("[p2k ball] %d launched at %d\n", i, frame); fflush(stdout);
						break;
					}
			plungeWas = plunge;

			/* A machine without an autoplunger. Revenge From Mars has one - driver 15, fired by
			   the game when the launch button is pressed - and the model waits for it. Star Wars
			   Episode 1 has no such coil: its own coil table lists a trough eject, slings, jets,
			   saucers and flippers, and nothing that launches a ball. It has a hand plunger, and
			   the ball leaves the lane because a person pulled it.

			   P2K_HANDPLUNGE=<frames> models that person: a ball that has sat in the shooter lane
			   that long leaves on its own. Without it a game on Episode 1 stops at the lane */
			if (handplunge)
				for (i = 0; i < nballs; i++)
					if (st[i] == B_LANE && frame - laneSince[i] >= handplunge) {
						st[i] = B_LEAVING; due[i] = frame + lanehold;
						printf("[p2k ball] %d plunged by hand at %d\n", i, frame); fflush(stdout);
						break;
					}

			/* --- the playfield ------------------------------------------------------------
			   While a ball is in play the model walks it over the playfield: every P2K_HITRATE
			   frames it closes the next switch in the table below for P2K_HITLEN frames. The
			   order is fixed rather than random so a run can be repeated exactly - the point is
			   to keep the game fed with events, not to be a physics engine.

			   The table is the game's own switch numbers (see p2k_names.h): the three jets, both
			   slingshots, the centre and martian targets, the top lanes, the loops and the ramps.
			   The outlanes (16, 27) are deliberately absent - draining is the drain timer's job,
			   so a ball's length in play stays under P2K_DRAIN */
			static const int pfSwitches[] = {
				63, 64, 65,     /* left, right, bottom jet  */
				61, 62,         /* slingshots               */
				36, 35, 34, 33, /* centre targets 1..4      */
				73, 72, 71,     /* martian targets 1..3     */
				85, 86, 87,     /* martian targets 7..5     */
				77, 76,         /* left and right top lane  */
				78, 25, 68, 67, /* left and right loops     */
				11, 28, 52, 12, /* ramp entrances and exits */
				74              /* centre loop rollover     */
			};
			static int pfIdx = 0, pfSw = 0, pfUntil = 0, pfNext = 0;
			if (getenv("P2K_PLAYFIELD")) {
				const char *hr = getenv("P2K_HITRATE");
				const char *hl = getenv("P2K_HITLEN");
				const int hitrate = hr ? (int)strtol(hr, NULL, 10) : 60;
				const int hitlen  = hl ? (int)strtol(hl, NULL, 10) : 5;
				int anyInPlay = 0;
				for (i = 0; i < nballs; i++) if (st[i] == B_PLAY) anyInPlay = 1;
				if (pfSw && frame >= pfUntil) { core_setSw(pfSw, 0); pfSw = 0; }
				if (!anyInPlay) { pfNext = frame + hitrate; }
				else if (!pfSw && frame >= pfNext) {
					pfSw = pfSwitches[pfIdx++ % (int)(sizeof pfSwitches / sizeof pfSwitches[0])];
					pfUntil = frame + hitlen;
					pfNext  = frame + hitrate;
					core_setSw(pfSw, 1);
				}
			}

			/* the optos, at rest until the balls arrive; the lane is not an opto.

			   This list is the stock machine's. myPinballs' opto expander piggybacks on the existing
			   opto board, takes over the 12-way switch matrix plug and brings out more matrix
			   positions - their 6 Ball Trough kit's J3 carries switch 53 (Wht-Org) and 54 (Wht-Yel),
			   four optos in all, two transmitter/receiver pairs, wired to trough 5 and 6. Those are
			   free on a stock playfield, p2k_names.h running 52 then 61, which is why the board uses
			   them. 2.22 is where the trough is "recognised once extra hardware installed".

			   What they sell now is one kit for both jobs, "Revenge From Mars Opto Expansion Upgrade
			   (For 6-Ball Trough & Physical Ball Locking)", built from WMS opto board parts and
			   listed as wanting software "v2.6 and above" - which is the lock half talking, the
			   trough half having worked since 2.22. It also says fitting it "automatically increases
			   the ball counts on some multiball modes and the maximum add-a-ball count", so the game
			   changes behaviour on finding it, rather than only gaining a lock.

			   2.60 is the set that requires the board, and for the other job: locking 3 balls
			   physically in the right lockup. It is the first with "Physical Lock Hardware
			   Installed", and it adds two switches for it. Those are 55 and 56, "Right Lockup 2"
			   and "Right Lockup 3", beside the lockup opto already at 51 - read out of 2.60's own
			   switch table, not guessed: the packages ship a symbol table, and the descriptive
			   switch table it points at reproduces every number in p2k_names.h exactly, with
			   "Not Used" in each gap. The same table names 53 and 54 "Trough Ball 5" and "Trough
			   Ball 6", which is the trough kit above and confirms it.

			   So the physical lock is not the trough board rewired - it wants two positions that
			   board does not bring out, and a machine with both mods fitted needs all four of
			   53-56. 2.60's table flags all four as optos, and they come up at rest with the rest of
			   them at the top of SWITCH_UPDATE. What is still missing is movement: a lock model for
			   55 and 56 and a six-ball trough for 53 and 54.

			   Only the trough is driven here, and only the four a stock machine has. Everything else
			   was set at rest further up, so 41, 46 and 51 no longer need repeating */
			for (i = 0; i < 4; i++) core_setSw(42 + i, (i < inTrough) ? 0 : 1);
			core_setSw(18, lane ? 1 : 0);
		}
	}
#endif /* P2K_DEBUG */
}

/* The ROM sets
/
/  Three things are declared here. The sound board's is one unified 0x600000-word address space
/  with the flash at word 0, U109 at 0x200000 and U110 at 0x400000; DCS_P2K_SNDREGION is
/  deliberately not declared, because with a single region the SDRC runs in its EPROM mode, which
/  is the page register this firmware programs. The two sample chips sit there flat, one after
/  the other, NOT interleaved at 32-bit stride as docs/pin2k_sound.md suggests - the firmware
/  settles it: sound command 0x001b walks a checksum table held in the sound flash (three regions:
/  flash, U109, U110) and byte-sums each one. For Star Wars Episode 1 the flat layout reproduces
/  the expected 0x25db/0x8e72 exactly, while the interleaved one gives 0xef29/0xc424. MAME's
/  driver loads them flat as well.
/
/  The MediaGX side's Prism ROMs are four 16 MB banks, each a pair of 8 MB chips interleaved as
/  32-bit words - which is what ROM_LOAD32_WORD does, so the region arrives ready to use.
/
/  And the update flash: 8 MB holding bootdata, the image, the game and its symbol table, laid
/  out at the offsets the boot ROM looks for them at. This is very nearly the only part that
/  differs between versions, which is why a clone's zip needs nothing but its own four update
/  files - except for some RFM and SWEP1 sets, which bring a sound flash of their own as well.
/
/  Basic file names and hashes from official releases as in the MAME & Encore sets (https://github.com/ThomazPom/Encore-Pinball2000),
/  which is what these were computed against. Both games' sample chips are good dumps.
/  More sets were added afterwards.
/
/  Beware a different, bad rfm_u109/rfm_u110 pair is in circulation, CRC32 A20B2ABB/095ABEC9: it
/  fails the board's own ROM checksum (sound command 0x001b wants 0x08f8/0x54ac and gets 0x50a8) */

/* Everything that does not change between versions of RFM: the DCS sound set and the four
   MediaGX/Prism banks. The sound flash is a parameter because it is the one part of this that a
   version can bring its own of - see P2K_COMMON_RFM below and the _sf.rom note above. It is loaded
   twice on purpose, once as the sound CPU's region and once into the DCS address space.

   Bank 0 is the only place the two Prism card revisions differ. r2 is a factory revision, not a
   game version: its bootstrap loader is V3.4 dated Apr 1 1999 where the r1 pair has V3.2 dated
   Jan 26 1999, and the fallback game copy it carries is 0.80 against the r1 pair's 0.1. Banks
   1-3 and the sound board are shared */
#define P2K_RFM_BANK0 \
		ROM_LOAD32_WORD("rfm_u100.rom", 0x0000000, 0x800000, CRC(b3548b1b) SHA1(874a16282bb778886cea2567d68ec7024dc5ed22)) \
		ROM_LOAD32_WORD("rfm_u101.rom", 0x0000002, 0x800000, CRC(8bef301d) SHA1(2eade00b1a4cd3f5e98ebe8ed8f549e328188e77))
#define P2K_RFM_BANK0_R2 \
		ROM_LOAD32_WORD("rfm_u100r2.rom", 0x0000000, 0x800000, CRC(d4278a9b) SHA1(ec07b97190acb6b34b9ed6cda505ee8fefd66fec)) \
		ROM_LOAD32_WORD("rfm_u101r2.rom", 0x0000002, 0x800000, CRC(e5d4c0ed) SHA1(cfc7d9d2324cc02c9eaf53fd674f7db24736699c))
#define P2K_COMMON_RFM_B(sfname, sfhash, bank0) \
	NORMALREGION(0x100000, REGION_CPU1) \
		ROM_LOAD(sfname, 0x0000, 0x100000, sfhash) \
	NORMALREGION(ADSP2100_SIZE, DCS_CPUREGION) \
	NORMALREGION(0x8000*2,      DCS_P2K_SRAMREGION) \
	SOUNDREGION(0xc00000, DCS_ROMREGION) \
		ROM_LOAD(sfname, 0x000000, 0x100000, sfhash) \
		ROM_LOAD("rfm_u109.bin",   0x400000, 0x400000, CRC(385f1255) SHA1(0a3be261cd35cd153eff95335597bca46b760568)) \
		ROM_LOAD("rfm_u110.bin",   0x800000, 0x400000, CRC(2258dbde) SHA1(0c9e62e45fa7cc03aedd43a6e06fee28b2f288a5)) \
	ROM_REGION32_LE(0x4000000, P2K_PRISMREGION, 0) \
		bank0 \
		ROM_LOAD32_WORD("rfm_u102.rom", 0x1000000, 0x800000, CRC(749f5c59) SHA1(2d8850e7f8ea3e07e8b444d7dd4dc4195a547ae7)) \
		ROM_LOAD32_WORD("rfm_u103.rom", 0x1000002, 0x800000, CRC(a9ec5e97) SHA1(ce7c38dcbf34ce10d6e204a3176cd2c7a83b525a)) \
		ROM_LOAD32_WORD("rfm_u104.rom", 0x2000000, 0x800000, CRC(0a1acd70) SHA1(dcca4de92eadeb82ac776953326410a9687838cb)) \
		ROM_LOAD32_WORD("rfm_u105.rom", 0x2000002, 0x800000, CRC(1ef31684) SHA1(141900a7426ad483384606cddb018d186952f439)) \
		ROM_LOAD32_WORD("rfm_u106.rom", 0x3000000, 0x800000, CRC(daf4e1dc) SHA1(0612495468fb962b833057e50f620c5f69cd5840)) \
		ROM_LOAD32_WORD("rfm_u107.rom", 0x3000002, 0x800000, CRC(e737ab39) SHA1(0e978923db19e2893fdb4aae69d6ed3c3f664a31))

/* Everything that does not change between versions of SWEP1: the DCS sound set and the four MediaGX/Prism banks. Sound flash parameterised as for RFM above */
#define P2K_COMMON_SWEP1_SF(sfname, sfhash) \
	NORMALREGION(0x100000, REGION_CPU1) \
		ROM_LOAD(sfname, 0x0000, 0x100000, sfhash) \
	NORMALREGION(ADSP2100_SIZE, DCS_CPUREGION) \
	NORMALREGION(0x8000*2,      DCS_P2K_SRAMREGION) \
	SOUNDREGION(0xc00000, DCS_ROMREGION) \
		ROM_LOAD(sfname, 0x000000, 0x100000, sfhash) \
		ROM_LOAD("swe1_u109.rom",   0x400000, 0x400000, CRC(cc08936b) SHA1(fc428393e8a0cf37b800dd475fd293a1a98c4bcf)) \
		ROM_LOAD("swe1_u110.rom",   0x800000, 0x400000, CRC(6011ecd9) SHA1(8575958c8942a6cbcb2ac18f291fcada6f8cbc09)) \
	ROM_REGION32_LE(0x4000000, P2K_PRISMREGION, 0) \
		ROM_LOAD32_WORD("swe1_u100.rom", 0x0000000, 0x800000, CRC(db2c9709) SHA1(14e8db2c0b09c4da6306a4a1f7fe54b2a334c5ed)) \
		ROM_LOAD32_WORD("swe1_u101.rom", 0x0000002, 0x800000, CRC(a039e80d) SHA1(8f63e8ab83e043232fc17ed3dff1f251396a178a)) \
		ROM_LOAD32_WORD("swe1_u102.rom", 0x1000000, 0x800000, CRC(c9feb7bc) SHA1(a34acd34c3f91f082b67e385b1f4da2e5b6e5087)) \
		ROM_LOAD32_WORD("swe1_u103.rom", 0x1000002, 0x800000, CRC(7a692466) SHA1(9adf5ae9c12bd5b6314913f6c01d4566ee453fe1)) \
		ROM_LOAD32_WORD("swe1_u104.rom", 0x2000000, 0x800000, CRC(76e2dd7e) SHA1(9bc20a1423b11c46eb2f5a514e985151defb5651)) \
		ROM_LOAD32_WORD("swe1_u105.rom", 0x2000002, 0x800000, CRC(87f2460c) SHA1(cdc05e017367f61280e3d5682096e67e4c200150)) \
		ROM_LOAD32_WORD("swe1_u106.rom", 0x3000000, 0x800000, CRC(84877e2f) SHA1(6dd8c761b2e26313ae9e159690b3a4a170cb3bd8)) \
		ROM_LOAD32_WORD("swe1_u107.rom", 0x3000002, 0x800000, CRC(dc433c89) SHA1(9f1273debc9168c04202078503cfc4f1ca8cb30b))

/* The stock sound flash, which is what all but some of the versions ship. The odd ones pass their own package's _sf.rom to the _SF forms above instead */
#define P2K_COMMON_RFM_SF(sfname, sfhash) \
	P2K_COMMON_RFM_B(sfname, sfhash, P2K_RFM_BANK0)
#define P2K_COMMON_RFM \
	P2K_COMMON_RFM_SF("rfm_28f800.rom",  CRC(a57c55ad) SHA1(60ee230b8978b7c5f1482b1b587d1c6db5fdd20e))
#define P2K_COMMON_RFM_R2 \
	P2K_COMMON_RFM_B( "rfm_28f800.rom",  CRC(a57c55ad) SHA1(60ee230b8978b7c5f1482b1b587d1c6db5fdd20e), P2K_RFM_BANK0_R2)
#define P2K_COMMON_SWEP1 \
	P2K_COMMON_SWEP1_SF("swe1_28f800.rom", CRC(5fc1fd2c) SHA1(0967db9b6e82d386d3a8415bbef40bcab5a06654))

/* The update flash, which is what actually differs between versions: four components laid out
   as the machine expects them, the rest of the 8 MB erased. bootdata is 32 KiB in every
   package, so im_flsh0 always starts at 0x8000 and the rest follows on */
#define P2K_UPDATE(game, ver, bdhash, imlen, imhash, gmlen, gmhash, sylen, syhash) \
	ROM_REGION(0x800000, P2K_UPDREGION, ROMREGION_ERASEFF) \
		ROM_LOAD("pin2000_" #game "_" #ver "_bootdata.rom", 0x000000,                 0x008000, bdhash) \
		ROM_LOAD("pin2000_" #game "_" #ver "_im_flsh0.rom", 0x008000,                 imlen,    imhash) \
		ROM_LOAD("pin2000_" #game "_" #ver "_game.rom",     0x008000+(imlen),         gmlen,    gmhash) \
		ROM_LOAD("pin2000_" #game "_" #ver "_symbols.rom",  0x008000+(imlen)+(gmlen), sylen,    syhash)

/* Revenge From Mars from 1.80 on, and Episode I's 1.6x onwards, are not Midway's but the community's/aftermarkets
   continuation - shipped as the same PKSFX update .exes with the same four files inside, so
   nothing but their hashes is new here. Those packages carry a fifth, pin2000_<game>_<ver>_
   pubboot.rom, 32 KiB and identical in all twelve that have it, which is not part of the update
   flash and is not loaded; MAME and Encore's loader ignores it too.

   The sixth, _sf.rom, does matter: 1 MiB in every package, and it is the DCS sound board's flash.
   17 of the 20 are byte for byte the rfm_28f800.rom / swe1_28f800.rom above, which is what lets
   P2K_COMMON_* carry one copy for nearly every version. Some are not, and that is why the sound
   flash is a macro parameter rather than baked in:

       rfm    pin2000_50070_0191_sf.rom  CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd)
       swep1  pin2000_50069_0210_sf.rom  CRC(f70cb335) SHA1(ca92287d7dd0b326b8ae20ba5e204074369fa271)

   Multiple packages, two images: 1.91 is where the RFM one comes from ("added some new sounds") and
   some 2.x ship that exact file again as pin2000_50070_0210_sf.rom.

   Both are real flashes rather than padding - same header as the stock pair, each using more of
   the part than it does, to 0xff570 and 0x817fc against 0xfe996 and 0x54ab2 - which is what added
   speech or effects needs. An update does write this to the sound board, so the copy a package
   carries is the one that version runs with, which is why those three sets take theirs from the
   package rather than the stock chip */
ROM_START(rfm_260)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0260, CRC(5442a347) SHA1(104657c96c73dd7597982c63b0bf7151ae2274e2),
	           0x06e02c, CRC(17e92432) SHA1(87f128836dd21f9c805fa7d745413749ac2d8750),
	           0x280c00, CRC(fd26d7ae) SHA1(669d13b92018f0015783e249684051612ddde3cf),
	           0x0c0a00, CRC(66c85132) SHA1(520f69d0701fbe5296fda62002fac619b3c95fbb))
ROM_END
ROM_START(rfm_250)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0250, CRC(15f140f4) SHA1(f0b074e3376f2a09121527c3289c9afc415f2086),
	           0x06e02c, CRC(17e92432) SHA1(87f128836dd21f9c805fa7d745413749ac2d8750),
	           0x27fe00, CRC(070302ab) SHA1(468f71b2eda12e11647de4f9a26b38aff7d10ec6),
	           0x0c0600, CRC(2a39d567) SHA1(d20ea3cb5817e9a38c2c98d969a50ae705b52dd5))
ROM_END

/* 2.24 went out three times under the same version number, so the sets are numbered by build
   rather than by version, and none holds a plain rfm_224 name:

     r1     ~12/21 the initial release
     r2   13/01/22 fixes a bug that awarded two extra balls instead of one
     r3   29/01/22 the final release, which adds a Lyman Sheats tribute.

   r2 and r3 share an im_flsh0 and differ in boot data, game and symbols, r2's game image being 1536 bytes shorter */
ROM_START(rfm_224r3)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0224, CRC(39a81ae6) SHA1(a81498991a5d70ea47565fa4bf857033d491521c),
	           0x06e02c, CRC(17e92432) SHA1(87f128836dd21f9c805fa7d745413749ac2d8750),
	           0x27e400, CRC(685fbd4f) SHA1(90196e256ccaf2d3095ffd70c6942add94411e47),
	           0x0bf600, CRC(26f84e44) SHA1(2bb4deb346960b646290683534f93bf35549a11f))
ROM_END
ROM_START(rfm_224r2)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0224, CRC(2f4811d0) SHA1(39da9f8f40787105f14dac8540ceed3ea2a94a03),
	           0x06e02c, CRC(17e92432) SHA1(87f128836dd21f9c805fa7d745413749ac2d8750),
	           0x27de00, CRC(4d5e4a88) SHA1(72e7783572d9a215b056e272815326bd11c849a3),
	           0x0bf400, CRC(f9abfc87) SHA1(dcf4854be45600187fff2ccbcee6d43acccb2d73))
ROM_END

ROM_START(rfm_223)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0223, CRC(ba906071) SHA1(f3ef4e45befc4956d2f72be0cb064a3a0d5f1154),
	           0x05fb1c, CRC(d26fb382) SHA1(29825ac8dc2c3370a689761eee5ee99866e4985e),
	           0x27dc00, CRC(f8f47f8f) SHA1(27a47024a12c434363a452e1f4ecbca3eb484c91),
	           0x0bf400, CRC(2f2232ac) SHA1(f7276712acb3bc05393766e7344e6503d89ed1f8))
ROM_END

ROM_START(rfm_222)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0222, CRC(bf3ad897) SHA1(b11b8fe9536962b44ef0b9e990eb0aee7ace4f17),
	           0x05fb1c, CRC(d26fb382) SHA1(29825ac8dc2c3370a689761eee5ee99866e4985e),
	           0x27d000, CRC(c3f8510f) SHA1(d3531a31006e177e69a687cc0c9b0abfcb7db922),
	           0x0bf000, CRC(44a2fb16) SHA1(6ec4654c0513578b0de15ec2a631c2cbf07ca475))
ROM_END

/* 2.20, 2.21, 2.10 and 2.00 carry the same sound flash as 1.91, rather than the stock one, so they
   name it the same way - the file is byte for byte 1.91's */
ROM_START(rfm_221)
	P2K_COMMON_RFM_SF("pin2000_50070_0191_sf.rom", CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd))
	P2K_UPDATE(50070, 0221, CRC(89273166) SHA1(c10e008b83e013b6c97e7edd754fbdf62d58d958),
	           0x07e29c, CRC(6effc654) SHA1(1ecbad2e8d478f32dcbf2e0f7ee1794326f5ae73),
	           0x27ac00, CRC(768a29dd) SHA1(437848495f754360220dd704b3f040eb6e0ea862),
	           0x0be000, CRC(25f65fdd) SHA1(b434dea77bf726596c11e4fe049a8e5c2b7f9d04))
ROM_END

ROM_START(rfm_220)
	P2K_COMMON_RFM_SF("pin2000_50070_0191_sf.rom", CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd))
	P2K_UPDATE(50070, 0220, CRC(26ca3c17) SHA1(9c72e7b171d1669c7031e6afb06aadf449aaa30d),
	           0x07e29c, CRC(6effc654) SHA1(1ecbad2e8d478f32dcbf2e0f7ee1794326f5ae73),
	           0x278c00, CRC(a9939f19) SHA1(6111cbe1b9f9ade162e15a613c62444eb7fe9409),
	           0x0bd000, CRC(3bd4d575) SHA1(0672836fecd4363ea5fa26ca70ff66cf0e5a5f1b))
ROM_END

ROM_START(rfm_210)
	P2K_COMMON_RFM_SF("pin2000_50070_0191_sf.rom", CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd))
	P2K_UPDATE(50070, 0210, CRC(ce6111dc) SHA1(fcce8430bac6bad9260ef86f3e37cc87eebb3896),
	           0x0650e4, CRC(9ffbdbe8) SHA1(e52064655db0f81bd3ec6836cc4b9eda5d3fa4ca),
	           0x273a00, CRC(49332443) SHA1(08285224d8eff072ba3b5520a245a314b3568758),
	           0x0bae00, CRC(d775e101) SHA1(94801c03fa03ad3dd56e66eaf233f3fbfa811cbd))
ROM_END
/* 2.00 is myPinballs' first, and it seems to take over from hemtoni in two ways the files show plainly: it
   keeps 1.91's sound flash, and it is the first to carry the im_flsh0 that 2.10 then ships
   unchanged - byte-identical, same CRC as rfm_210's below. Its own code is still older than that
   suggests, XINA 1.22 like the 1.9x sets rather than 2.10's 1.31, so a newer system image arrived
   one release before the game moved to it. None of 2.10's optional shaker and knocker adjustments
   yet either - and do not read the menu's "Knocker Test" as one. No Pinball 2000 ever shipped with
   a knocker coil; the entry is in the stock software all the same, 1.90 carrying it and official
   1.60 before that as "Test Knocker", so it tests hardware no machine has. What 2.10 adds is the
   aftermarket kit: "Knocker (Optional)"/"Shaker (Optional)" and the drives behind them */
ROM_START(rfm_200)
	P2K_COMMON_RFM_SF("pin2000_50070_0191_sf.rom", CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd))
	P2K_UPDATE(50070, 0200, CRC(51e47335) SHA1(c2224d94f519cea8925ce77f00529b003d981c50),
	           0x0650e4, CRC(9ffbdbe8) SHA1(e52064655db0f81bd3ec6836cc4b9eda5d3fa4ca),
	           0x26fa00, CRC(2a877688) SHA1(7fa5b4f55a014a9e866acfcf94ba5428c98bb2a2),
	           0x0b9600, CRC(c9a97659) SHA1(a08b4c5bb3bfe94476af6282eab6aba31725b6fc))
ROM_END

/* 1.95 is 1.90 with the German retranslated, and the middle of the three rather than the newest -
   1.91 is the last and went back to 1.90's wording. 1.91 is also the one that brings its own sound
   flash, which 2.00 and 2.10 then reuse */
ROM_START(rfm_195)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0195, CRC(9e33a421) SHA1(cc64d0fb50f9ed9655a830012c1c2082e8499fa0),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x259400, CRC(237bcbdb) SHA1(909f8f15463014ab29f388b6e8de6d5ae78571df),
	           0x0c6200, CRC(e96beaf3) SHA1(991053aac339346928a749b7df4ca9275bb32b0f))
ROM_END
ROM_START(rfm_191)
	P2K_COMMON_RFM_SF("pin2000_50070_0191_sf.rom", CRC(9870a651) SHA1(d16e3fc489f90677f9bf0666b4dc01a412e7dadd))
	P2K_UPDATE(50070, 0191, CRC(6c7d7239) SHA1(eb1693b320222fbf2dce567c3f90e69005a161c3),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x26aa00, CRC(129f7854) SHA1(667b173bd78635d71fd16eab2ce4ca4fc5de8d52),
	           0x0b7400, CRC(edaa9cec) SHA1(e934bcd10ff66fad9b8dad17f83323df65cf3902))
ROM_END
ROM_START(rfm_190)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0190, CRC(9a27bff0) SHA1(961370c09757aba6788fba086f28084ffbc12bb7),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x259600, CRC(382a4a63) SHA1(85e5f483a58080452d6656347aae7b72894b2e47),
	           0x0c6200, CRC(9d02d3d1) SHA1(dae66902b9a4c42ca5832687416eaf7062116806))
ROM_END
ROM_START(rfm_180)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0180, CRC(cdced51f) SHA1(7f781d72fde892fc5867a511f9eb1bc773b50316),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x26aa00, CRC(a736f81f) SHA1(8136a526879729b5d2b7a9a6f01191a2e9097efc),
	           0x0b6e00, CRC(69421a8b) SHA1(c09a276a3f285e8c140faa1017221b599a577517))
ROM_END

/* The other two fallback images, for the same reason rfm_080 is here: a set each so they are
   addressable and anyone working on them can just run one. These two need no ROMs of
   their own - they are their parent's Prism chips with the update package left out, so the loader
   starts the copy in the ROMs instead, and we find the files in the parent set. Both boot and
   run now, as do 0.80 and 1.20.

   The version each one prints through "Software version: %d.%d" comes from the major/minor pair in
   the boot-data header at bank 0 offset 0x8040: 0 and 1 here, 0 and 40 for Episode I. The minor is
   a plain number, so RFM's machine displays 0.1 rather than 0.10 - the description below carries
   what it prints, while the set name follows the three-digit style the rest of this file uses */
ROM_START(rfm_010)
	P2K_COMMON_RFM
ROM_END
ROM_START(swep1_040)
	P2K_COMMON_SWEP1
ROM_END
/* The factory image on a rev. 2(?) Prism card, reached by giving the machine no update flash at all:
   the loader then validates and starts the fallback copy in the Prism ROMs. Every other set here
   overrides that copy with an update, so this is the one version that is the card rather than a package.

   "We only had 60MB of ROM to store every art asset the game needed.
    The other 4MB was reserved for an old version of the game software so that it was always possible to boot the game and upload a newer version into the Flash memory."

   It used to halt at 0x1b355f/0x1b3568/0x2020c5 with no timer ever programmed, the same shape as
   the rev. 1 pair's 0.1 and Episode I's 0.40 on this path. That was the PLX serial EEPROM, whose
   write side was not modelled - see prism_1000_w in src/p2k/p2k_driver.cpp. All four boot and run
   now. src/p2k/README.md has the table.

   Only u100/u101 are rev. 2(?) - the MAME set carries no other r2 file - but the rest is very
   likely right rather than merely assumed. Diff the two revisions of this pair and they part
   only below 0x2fd3c0 of the interleaved bank: the loader and the fallback game image.
   The 13 MB of asset data above that is byte-identical. So in the one bank where both revisions
   exist, the revision changed code and left the art alone, which is the same reasoning that
   carries banks 1-3 - also asset data - over unchanged. The sound flash and the two sample chips
   are on the sound board, not the Prism card, so a card revision does not implicate them at all;
   rfm_28f800.rom is the flash the game shipped with. A sound board revision of its own would not
   show up in this dump, which is the one gap left, and it should not what the halt is about.

   The PCI bring-up patch moves with the image and is found by its five-byte signature rather than
   by game - see set_prism_roms() in src/p2k/p2k_driver.cpp, which cannot tell r1 from r2 by name,
   and where r2 holds a call whose displacement starts where the stock pair holds the immediate */
ROM_START(rfm_080)
	P2K_COMMON_RFM_R2
ROM_END

ROM_START(rfm_160)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0160, CRC(b8574f37) SHA1(339475365ecc4adf11c28b3432ea1f7da905745d),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x26ae00, CRC(70235c6d) SHA1(94ddfa0df6a0808726f25d5ea89828d1c7cb8b35),
	           0x0b6c00, CRC(04d39e96) SHA1(867344e07b3420d0e5f1fbfd0b7174d13958af6c))
ROM_END
ROM_START(rfm_150)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0150, CRC(3b7fa316) SHA1(d0221e7ed53f7383e4153eda8e5b04e9a48c6da2),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x26b000, CRC(4492bf61) SHA1(caae41d86a7a7410165c588dc87899b675a9306e),
	           0x0b6e00, CRC(a343a172) SHA1(4295d756d6892bc657021f389f657aefeaf756d6))
ROM_END
ROM_START(rfm_140)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0140, CRC(fb535e75) SHA1(23a544860cff86bfb4d901737d15d162f31d18e0),
	           0x0961f4, CRC(5c5f4b6d) SHA1(98416e05d5d5a316e2f6a21907fb96c09385d4c7),
	           0x26b000, CRC(c181bb31) SHA1(8232bd8ffd87c4331a4c5edd3c8ce0a8e2f872c7),
	           0x0b6e00, CRC(0761953d) SHA1(7e8ad5641a9485d63aa8baded5933b2753480736))
ROM_END
ROM_START(rfm_120)
	P2K_COMMON_RFM
	P2K_UPDATE(50070, 0120, CRC(235d3263) SHA1(3392687e152a2266ee2dd3c4c9eb9d22dda6d254),
	           0x096204, CRC(a6bcc255) SHA1(867eb792308c4e8c3559d31236eafee4cd35b95d),
	           0x211a00, CRC(fed4d69d) SHA1(7bab52d3cf91322a039593ca6b60933c4b5da838),
	           0x0aca00, CRC(51bbabda) SHA1(617f67a4997f52fcb6c1095da9723f1a2ca22bd2))
ROM_END
/* and the other one, likewise */
ROM_START(swep1_210)
	P2K_COMMON_SWEP1_SF("pin2000_50069_0210_sf.rom", CRC(f70cb335) SHA1(ca92287d7dd0b326b8ae20ba5e204074369fa271))
	P2K_UPDATE(50069, 0210, CRC(73415976) SHA1(4d9d5f97cb288a0ac8ee464a6e46ddf68cac949d),
	           0x0531c8, CRC(d7b56727) SHA1(c637dc5be6a1c2ae8d2086d27c17d308b78930e3),
	           0x244400, CRC(c7677f84) SHA1(5bd7ddfbac2911c7e7fc378e4f4f0f94a74f6391),
	           0x098c00, CRC(cd9d9863) SHA1(b46550816518e5cc476370e32085b840d5314ceb))
ROM_END
ROM_START(swep1_201)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0201, CRC(9a605f47) SHA1(6e0ff8c21aa1d2b24cf5d4c72c01b32fa4d4bfb0),
	           0x03c384, CRC(046555ec) SHA1(e54577be77ca6222a891e23734fee670e3b94601),
	           0x23f800, CRC(c1b0067b) SHA1(2f221db8a5d6b5fa44da581dcd42717412b694d6),
	           0x096800, CRC(e64e9243) SHA1(e9ce758412d0c8194aec2c0e79c6436f22d691f8))
ROM_END
ROM_START(swep1_200)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0200, CRC(7e2989af) SHA1(44caed94a2d939aa474bf44cd5e9b3cf7c6b2bec),
	           0x0387cc, CRC(44124d91) SHA1(06a8f2b3b6c25aad45a27ce0a617382e24d34dbe),
	           0x23f000, CRC(109ed1ff) SHA1(745f18680a5f6793dd72fa34f7ddd10edee4384f),
	           0x096600, CRC(3ba136cc) SHA1(96d7f6ae2fc205e137300ab1ddf3961d24e9f2ee))
ROM_END
ROM_START(swep1_166)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0166, CRC(6cf16c6b) SHA1(7ce292a9159266e3e8fbc06899a1889372b94379),
	           0x05d8f0, CRC(f2f4c4ff) SHA1(6692fc6743cb6bc91f36136635bf7226d39398a3),
	           0x23f200, CRC(37e67c3e) SHA1(041d280dd2ae4c7cd588d48195f1001d6c299d6f),
	           0x094800, CRC(75b7e70b) SHA1(a934ff7c124d953a4bb70123c0f9f01e6212fd30))
ROM_END
ROM_START(swep1_150)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0150, CRC(61923fc2) SHA1(0f0073d53f7bf3ddac06d9e73480629f0ff8dad9),
	           0x089d54, CRC(222061cd) SHA1(870923d68694886ece2100c804e3b3e4f3a80d8b),
	           0x23d800, CRC(bafaf010) SHA1(0fc6cd338b880c6b5f6eb05bce6fb93dfc61189d),
	           0x094000, CRC(b3b74fc6) SHA1(2cace149d8162b86d99f34fbcc9e4a72af7b4b4c))
ROM_END
ROM_START(swep1_140)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0140, CRC(aecdec71) SHA1(0800f7172dc45e3de8bc63715b7cea3d56d96776),
	           0x089d54, CRC(222061cd) SHA1(870923d68694886ece2100c804e3b3e4f3a80d8b),
	           0x23da00, CRC(78a76a9b) SHA1(20610611ed83f065cf42049d178551283892d2f0),
	           0x094200, CRC(c9eb6613) SHA1(1e1e327a99fc7435920cf0f857eea3b4eb1809f3))
ROM_END
ROM_START(swep1_130)
	P2K_COMMON_SWEP1
	P2K_UPDATE(50069, 0130, CRC(c997b2c7) SHA1(1f9db99cb12f47e896bab19d3b49d62bd18cf0e6),
	           0x089d64, CRC(92ba31c1) SHA1(6961be1af60ea364014daa20a6867307428475b1),
	           0x1ec600, CRC(b714f1fa) SHA1(a9ab539d14b727656bdecbcaa11c866871525156),
	           0x08cc00, CRC(3e973484) SHA1(4584ddf0b05f4c41029e2dfb89063939c697cc45))
ROM_END

/* PinMAME's core wants a game description before it will build the machine. Pinball 2000 has
   no alphanumeric or dot matrix display - its video comes from the MediaGX frame buffer - so
   the layout is empty for now */
static PINMAME_VIDEO_UPDATE(p2k_video);
/* 640x240, the machine's own screen. Declaring it as CORE_VIDEO is what tells the core there is a
   display at all: without it locals.hasDmdOrVideo stays clear and libpinmame invents a 128x32 DMD
   in its place, so a frontend would see a fake dot matrix and never the real picture */
static core_tLCDLayout p2k_disp[] = {
  {0, 0, 240 * P2K_LINE_DOUBLE, 640, CORE_VIDEO, (genf *)p2k_video, NULL}, {0}
};
/* What PinMAME's core can be told about this machine. The lamp matrix is sixteen columns rather
   than eight, because the board drives eight columns with two row banks each, so eight custom
   columns are declared on top of the standard ones. The common switches are the ones the core and
   the remote debugger ask for by name; their numbers come from the game's own switch table
   (see src/p2k/README.md): start 13, plumb tilt 113, slam tilt 111, coin door 112. This machine
   reports no shooter lane switch of its own, so that one stays unset.

/  The flippers are driven by the game, not faked by the core.
/
/  Without FLIP_SOL the core treats the machine as one whose flippers are not CPU controlled and
/  does this, once a frame (core.c, "fake solenoids if not CPU controlled"):
/
/      coreGlobals.solenoids2 &= 0xffffff00;
/      if (swFlip & CORE_SWLLFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LLFLIPSOLBITS;
/      if (swFlip & CORE_SWLRFLIPBUTBIT) coreGlobals.solenoids2 |= CORE_LRFLIPSOLBITS;
/
/  Those low eight bits of solenoids2 are exactly where this driver puts the pinball board's
/  register 0x0c - drivers 33-40 - so the game's own flipper coils were being wiped every frame
/  and replaced by a copy of the buttons. Earlier flipper measurements in src/p2k/README.md were
/  reading that copy, not the machine.
/
/  The game's numbering and the core's agree exactly, which is why nothing else has to move:
/
/      Treiber 33 Right Flipper Power   -> solenoids2 bit 0 = sLRFlipPow (45)
/      Treiber 34 Right Flipper Hold    -> solenoids2 bit 1 = sLRFlip    (46)
/      Treiber 35 Left  Flipper Power   -> solenoids2 bit 2 = sLLFlipPow (47)
/      Treiber 36 Left  Flipper Hold    -> solenoids2 bit 3 = sLLFlip    (48)
/
/  FLIP_SOL(x) also sets FLIP_EOS(x), and that part is deliberately masked off. EOS would put two
/  synthesised end-of-stroke switches into the flipper column at bits 0x01 and 0x04, and this
/  machine's cabinet column uses those two for slam tilt and plumb tilt - a held flipper would
/  read as a tilt.
/
/  The machine does have end-of-stroke switches of its own:
/  its table names 105 and 106 Right and Left Flipper EOS, in every version from 1.60 on. They are
/  in the coin door column, though, nowhere near the two bits the core would synthesise into, so
/  masking is still right - it is a matter of not writing over slam and plumb tilt, and nothing to
/  do with whether the board reports EOS */
#define P2K_FLIPPERS (FLIP_SW(FLIP_L) | (FLIP_SOL(FLIP_L) & ~FLIP_EOS(FLIP_L)))

/* Drivers 37-48 as custom solenoids 51-62.
/
/  The board has six solenoid registers and this driver decodes all six, but the core only carries
/  the first forty outputs in the two words it publishes: coreGlobals.solenoids is drivers 1-32,
/  and of solenoids2 core_getAllSol() takes only the low four bits, the flipper coils. Drivers
/  37-40 sit in the next four and were being dropped - Lock Diverter Power/Hold and Up/Down Ramp
/  Power/Hold on Revenge From Mars, Shield Power/Hold and the laser flashers on Episode I - and
/  41-48 had nowhere to go at all.
/
/  hw.custSol and hw.getSol are what the core provides for outputs past its generic set, the way
/  alvg.c and capcom.c use them, so both ranges go out that way: solenoid 51 is board driver 37,
/  and 62 is 48. That covers Episode I's knocker, shaker and topper - board 42, 43 and 44, so
/  solenoids 56, 57 and 58 - which is what wanted doing here, and fixes the 37-40 hole for both
/  games on the way. The alternative was to claim an S11 or SAM generation flag, since those are
/  what core_getAllSol() tests before it will emit solenoids2's upper bits, and that would have
/  dragged in behaviour from another machine entirely.
/
/  There is a GEN_P2K now, and it deliberately stays out of that test. Adding it to the
/  GEN_ALLS11|GEN_SAM|GEN_SPA group in core_getSol() and core_getAllSol() looks like the tidy thing
/  to do and is not: those emit solenoids2 bits 8-15 as solenoids 37-44, which is the same twelve
/  outputs this already publishes as 51-62, so the machine would report every one of them twice.
/
/  The 37-40 half is measured, not assumed. Revenge From Mars 1.60's own coil test walks drivers
/  33-40 one at a time, and under P2K_SOLWATCH they come through as:
/
/      driver 37 Lock Diverter Power -> solenoids2 bit 4 -> solenoid 51
/      driver 38 Lock Diverter Hold  ->                5 ->          52
/      driver 39 Up/Down Ramp Power  ->                6 ->          53
/      driver 40 Up/Down Ramp Hold   ->                7 ->          54
/
/  with 33-36 staying out of the custom range, as they must - those are the flipper coils and the
/  core already publishes them as 45-48. Those bits are exactly the ones core_getAllSol() masks off,
/  so until this the diverter and the ramp had never reached a frontend on any set.
/
/  The 41-48 half is measured too, and not on a 2.x set: Episode I has its neon on 41, stock
/  hardware named in the official 1.50 table and in the manual's solenoid list. Under P2K_SOLWATCH
/  swep1_150 pulses it in attract mode all by itself -
/
/      driver 41 Neon                -> solenoids2 bit 8 -> solenoid 55
/
/  - and bit 8 is the high byte, which is set only if the register 0x0e decode in p2k_driver.cpp
/  works. That set's own coil test then walked every driver and each one came up under the name the
/  machine put on screen, so the range and the swep1 coil table are both checked against the game
/  rather than against the manual alone. RFM has nothing above 40 bar the ticket dispenser on 48,
/  which is not normally fitted, so its test never writes the register - that is why this took an
/  Episode I set. Only the myPinballs drives - knocker 42, shaker 43, topper 44 - wait on 2.x.
/
/  STILL OUTSTANDING: the modulated outputs. This machine takes no part in PinMAME's PWM model at
/  all - no coil strength, no bulb fade, no GI brightness, just on and off - and nSolenoids is left
/  at 0 on purpose. That is not an oversight, and switching it on is not small:
/
/    * nSolenoids alone makes things worse, not better. core_getSol() reads physicOutputState[] as
/      soon as the count is non-zero and the option is set, so declaring the count without also
/      feeding the integrator would report every output as permanently off.
/    * The integrator wants writes when the hardware is written, not once a frame.
/      core_write_pwm_output*() integrates over the time an output was actually on, and
/      p2k_sync_io() above samples the whole machine once per video update - a coil pulsed for
/      20 ms between two 60 Hz samples can read as fully on, fully off or anything between, and no
/      care at this end recovers a duty cycle that was never captured.
/    * So it has to be pushed from the subsystem, where p2k_state writes the coil, lamp and GI
/      registers, and that means p2k_driver.cpp reaching into the core - which it deliberately does
/      not do. Either that boundary gains a narrow output-event callback, or the sync moves to
/      something finer than a frame.
/    * Then the straightforward part: core_set_pwm_output_type() per output, with the coil and bulb
/      models the manuals' solenoid tables already name - AE1-26-1500, #906, #89 and the rest - and
/      core_update_pwm_solenoids()/_lamps()/_gis() on a tick.
/
/  capcom.c is the smallest driver that does the whole thing, and the one to read first */
#define P2K_CUSTSOL_FIRSTDRIVER 37
#define P2K_CUSTSOL_COUNT       12

static int p2k_getSol(int solNo) {
  const int driver = P2K_CUSTSOL_FIRSTDRIVER + (solNo - CORE_FIRSTCUSTSOL);
  if (driver < P2K_CUSTSOL_FIRSTDRIVER || driver >= P2K_CUSTSOL_FIRSTDRIVER + P2K_CUSTSOL_COUNT)
    return 0;
  /* solenoids2 bit 0 is driver 33, so bit 4 is 37 and bit 15 is 48 */
  return (coreGlobals.solenoids2 & (1u << (driver - 33))) ? 1 : 0;
}

/* GEN_P2K: Nothing in the core tests the bit, so setting it changes no
   behaviour; what it buys is that the machine is identifiable from outside, the way every other generation is */
static core_tGameData p2kGameData = {
  GEN_P2K, p2k_disp,
  {P2K_FLIPPERS, 0, /*lampCol*/ 8, /*custSol*/ P2K_CUSTSOL_COUNT,
   /*soundBoard*/ 0, /*display*/ 0, /*gameSpecific1/2*/ 0, 0, p2k_getSol},
  NULL,
  {{0}, {0}, {swStartButton, swPlumbTilt, swSlamTilt, swCoinDoorClosed, 0}}
};

static void init_rfm(void)   { core_gameData = &p2kGameData; }
static void init_swep1(void) { core_gameData = &p2kGameData; }

/* Not here: the second revision of Revenge From Mars's bank-0 Prism pair, which the Encore set
/  carries alongside the ones above.
/
/      rfm_u100r2.rom  CRC(d4278a9b) SHA1(ec07b97190acb6b34b9ed6cda505ee8fefd66fec)
/      rfm_u101r2.rom  CRC(e5d4c0ed) SHA1(cfc7d9d2324cc02c9eaf53fd674f7db24736699c)
/
/  The "r2" name is MAME's. It carries them as rfmpbr2, "Pinball 2000: Revenge From Mars (rev. 2)",
/  a clone of rfmpb differing in u100/u101 and nothing else, and dates both 1999 - so this should be a
/  factory-era revision of the card. MAME has to make a set of it because its P2K driver loads no update
/  flash at all: the Prism pair is the only thing there that can tell two machines apart. Here the
/  update flash is loaded, so the same pair is not a version.
/
/  Nor is either the sole home of the game code. u100/u101 hold a fallback copy of it plus system
/  data, and the update flash overrides that - which is why a machine runs whatever was last
/  updated into it regardless of which Prism revision is underneath.
/
/  These are not a game version - the game version is the update flash, and none of the update
/  packages above pairs with a particular boot ROM. They are a revision of the MediaGX side's own
/  firmware: same 8 MB each, identical to the stock pair from 0x17e9e0 to the end, and rewritten
/  below that - about 15% of each chip, concentrated in the first 1.5 MB.
/
/  Adding them as a ninth set would need more than a ROM_START, which is why they are left out for now(!)
/  rather than guessed at. The subsystem patches two bytes of bank 0 on the way in (see
/  p2k_state::set_prism_roms), and one of the two moves:
/
/      0x191    both revisions have the same `retf` (cb) there, so that patch carries over
/               unchanged - as it happens Episode I has cb there too
/      0x419a   this is the immediate, not the instruction: the stock pair has `b8 f9 ff ff ff`,
/               `mov eax,0FFFFFFF9h`, starting one byte earlier at 0x4199, and the patch overwrites
/               the four immediate bytes with 1 to force a failing check to report success. The r2
/               pair has `e8 9e 27 00 00` at that same 0x4199 - a near `call`, +0x279e - so the
/               four bytes the patch would write are its displacement, and writing 1 into them
/               sends the call somewhere arbitrary.
/
/  That last one has since been chased down, and it is less of an obstacle than it looks. The
/  patched instruction is the "already initialised" early exit of the MediaGX PCI bring-up - the
/  routine that finds the Cyrix host and ISA bridges and programs their BARs - and the patch makes
/  that exit return 1, the same value the success path returns, instead of -7. The same routine is
/  in the r2 image, just moved: it starts at 0x4608 rather than 0x4184, and the immediate to patch
/  is 0x461b rather than 0x419a. p2k_state::set_prism_roms has the full disassembly and the search
/  strings for re-finding it in any image. So an r2 set needs that one address changed, not new
/  reverse engineering.
/
/  Whether it should be a set at all is the other open question, and it turns on when rev. 2
/  actually shipped and which version first left the factory on it - with no update package tied to
/  the pair, an r2 set would have to be a clone of one of the versions above, and which one is not
/  something the files answer. That question decides the shape of every set here, not just an r2
/  one, and is written up under "To investigate: when did rev. 2 ship" in src/p2k/README.md. */

/* For Revenge From Mars parent is 1.60, not 1.80, but that one is debatable. Both packages are named ..._09222003_...,
   but the name is the package's and not the code's: 1.60's game.rom was built 22 September 2003,
   1.80's 23 April 2006. Episode I's 1.50 carries that same September 2003 build and is the last
   official one there - the two are one release, with the same fix (a factory reset when the machine
   boots with the power driver board unplugged) and the same final XINA, the P2K system software,
   which the game.rom banner numbers 1.19. That banner independently checks the order of everything
   here: 1.16 for SWEP1 1.30, 1.17 for RFM 1.40, 1.18 for the July 2000 pair, 1.19 for this one,
   1.21 for 1.80, 1.22 for the 1.9x sets, 1.31 and up for 2.x. Both went out after WMS Pinball had
   closed, carried on a PUB card rather than as a serial update - which is where the packages' fifth
   file gets its name - and the .exes here are a later repackaging.

   Three lines follow, and none is the next one's parent.

   1.70/1.80 is Tom Uban's (maybe with Lyman Sheats), i.e. the original
   Pinball 2000 programmers, for the EPC 2006 tournament server. The only readable strings it adds
   over 1.60 are a card-swipe tournament client ("Swipe card to play.", divisions, qualified
   players, "Server not responding...") - no gameplay change at all. 1.80 is also the one version wanting 8 MB of SDRAM, sitting on a white screen
   without it; nothing else needs it, and it cannot reach this driver anyway, which always just maps 256 MB.

   The 1.9x sets are hemtoni's. All three are XINA 1.22, and 1.90 is where that 8 MB requirement
   goes - his note on it is that it no longer needs the memory, with English and German kept and
   Spanish and French dropped for room. His changelog dates them 1.90 to 21 November 2017, 1.95 to
   29 March 2018 and 1.91 to 31 May 2018, so 1.95 is the middle one despite the number. The sets
   follow those dates, which is why 1.90 is 2017 here while its package says 2018: all three were
   put up in 2018, and all three game.roms carry 1.90's November 2017 stamp because the later two
   never restamped.

   Of the three, 1.90 lasted and the other two are branches off it: everything after them carries
   1.90's German, 1.95's re-translation appears in nothing later, and 1.91's added sounds ride in a
   sound flash only 2.10 reuses. The string counts behind that are in src/p2k/README.md, under
   "1.95 is not the newest RFM 1.x".

   The 2.x line is myPinballs' ("applejuice"), and where their opto expander board comes in: usable
   from 2.22 for a 6 ball trough if fitted, required only by 2.60, which locks 3 balls physically
   on two positions of its own. So every set here bar 2.60 runs on an unmodified playfield.
   Supporting either needs no new emulation - they are spare switch matrix positions, 53-56 (the
   trough taking 53/54 and the lock 55/56), and push_switches() already carries the matrix whole, so
   it is switch data and a little ball model; the opto block in SWITCH_UPDATE(p2k) below has the
   details. 2.60 also carries 1.80's card-swipe code
   ("DeffSwipeAttractMode"), translated, so that much of the tournament line did get folded in.

   Shaker and knocker are the other mod, this one on the driver side, and the two games do not put
   them in the same place. RFM 2.10 starts it: the loom hangs them off two drives the stock game
   leaves unused, knocker on 18 (Q44, J111-2, 50 V) and shaker on 19 (Q45, J111-3, 12 V), taken
   from the flasher section, which is why the kit ships protection diodes. RFM's own 2.00 has
   neither, so mind which game a "2.00" refers to here. Episode I gets the pair from its 2.00, six
   years the later of the two, and a topper at 2.10, but needs a cabinet driver control PCB as well as the loom, plus
   a pigtail moving which driver inputs it sits on - and its own driver table says where they land:
   knocker 42, shaker 43, topper 44. Both games' tables are in p2k_names.h, read out of the
   game.roms.

   That board is sold as shaker and knocker "aswell as 3 additional drives for future add-on (2 x
   50v and 1 x 12v)", which is J4, J5 and J6 on it, and the numbers fit: the 12 V one is J4, the
   topper, which 2.10 took up. The two 50 V spares have no software yet and would land on the 45-47
   Episode I still lists as Not Used. Worth knowing before wondering what those are for.

   That difference matters less here than it used to. RFM's two are inside coil register 0x09,
   drivers 17-24, so pull_outputs() has always handed them over as ordinary solenoids - an output
   nobody drives is simply off, with no rest state to get wrong unlike an opto. Episode I's sit at
   42-44, which needed the sixth coil register, and that register has since been found: 0x0e carries
   drivers 41-48, into the high byte of m_solenoids2 (p2k_driver.cpp), so all of 1-48 now reach
   PinMAME. It was confirmed by Episode I's neon on driver 41, which appears as custom solenoid 55.
   All of it is optional hardware either way, so every set plays without it.

   The year on each set is the version's own, from its changelog or build stamp wherever the package
   name disagrees - which it does for 1.60, 1.80, Episode I's 1.50 and all three 1.9x.

   Most games boot. Some of them used not to, and what divided them was the XINA each game.rom
   names rather than the game: 1.12 to 1.31 came up, 1.34 to 1.38 did not, so rfm_222 stopped where
   rfm_210 ran and Episode I's 2.x were all on the far side. The cause was a blank CMOS, not the
   boot ROM and not this driver: the newer software reports a NonFatal during static construction,
   the reporter appends it to an error log whose header a fresh CMOS does not have, and the entry
   lands on address 0 - which the scheduler then reports as fatal, for ever. P2K_SEED_ERROR_LOG in
   src/p2k/p2k_driver.cpp builds that header; the whole chain is in src/p2k/README.md */

CORE_GAMEDEF (rfm, 160, "Pinball 2000: Revenge From Mars (1.60)", 2003, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 150, 160, "Pinball 2000: Revenge From Mars (1.50)", 2000, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 140, 160, "Pinball 2000: Revenge From Mars (1.40)", 2000, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 120, 160, "Pinball 2000: Revenge From Mars (1.20)", 1999, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 080, 160, "Pinball 2000: Revenge From Mars (0.80 prototype/factory, rev. 2(?) board)", 1999, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 010, 160, "Pinball 2000: Revenge From Mars (0.1 prototype/factory, rev. 1 board)", 1999, "Midway", p2k, 0)
CORE_CLONEDEF(rfm, 180, 160, "Pinball 2000: Revenge From Mars (1.80 unofficial MOD)", 2006, "Midway", p2k, 0) // debatable if this still counts as official
CORE_CLONEDEF(rfm, 190, 160, "Pinball 2000: Revenge From Mars (1.90 unofficial MOD)", 2017, "Midway / hemtoni", p2k, 0)
CORE_CLONEDEF(rfm, 191, 160, "Pinball 2000: Revenge From Mars (1.91 unofficial MOD)", 2018, "Midway / hemtoni", p2k, 0)
CORE_CLONEDEF(rfm, 195, 160, "Pinball 2000: Revenge From Mars (1.95 unofficial MOD)", 2018, "Midway / hemtoni", p2k, 0)
CORE_CLONEDEF(rfm, 200, 160, "Pinball 2000: Revenge From Mars (2.00 unofficial MOD)", 2018, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 210, 160, "Pinball 2000: Revenge From Mars (2.10 unofficial MOD)", 2019, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 220, 160, "Pinball 2000: Revenge From Mars (2.20 unofficial MOD)", 2019, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 221, 160, "Pinball 2000: Revenge From Mars (2.21 unofficial MOD)", 2020, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 222, 160, "Pinball 2000: Revenge From Mars (2.22 unofficial MOD)", 2020, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 223, 160, "Pinball 2000: Revenge From Mars (2.23 unofficial MOD)", 2021, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 224r2, 160, "Pinball 2000: Revenge From Mars (2.24 rev. 2 unofficial MOD)", 2022, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 224r3, 160, "Pinball 2000: Revenge From Mars (2.24 rev. 3 unofficial MOD)", 2022, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 250, 160, "Pinball 2000: Revenge From Mars (2.50 unofficial MOD)", 2022, "Midway / mypinballs", p2k, 0)
CORE_CLONEDEF(rfm, 260, 160, "Pinball 2000: Revenge From Mars (2.60 unofficial MOD)", 2024, "Midway / mypinballs", p2k, 0)
CORE_GAMEDEF (swep1, 150, "Pinball 2000: Star Wars Episode I (1.50)", 2003, "Williams", p2k, 0)
CORE_CLONEDEF(swep1, 140, 150, "Pinball 2000: Star Wars Episode I (1.40)", 2000, "Williams", p2k, 0)
CORE_CLONEDEF(swep1, 130, 150, "Pinball 2000: Star Wars Episode I (1.30)", 1999, "Williams", p2k, 0)
CORE_CLONEDEF(swep1, 040, 150, "Pinball 2000: Star Wars Episode I (0.40 prototype/factory)", 1999, "Williams", p2k, 0)
CORE_CLONEDEF(swep1, 166, 150, "Pinball 2000: Star Wars Episode I (1.66 unofficial MOD)", 2022, "Williams / hemtoni", p2k, 0)
CORE_CLONEDEF(swep1, 200, 150, "Pinball 2000: Star Wars Episode I (2.00 unofficial MOD)", 2025, "Williams / mypinballs", p2k, 0)
CORE_CLONEDEF(swep1, 201, 150, "Pinball 2000: Star Wars Episode I (2.01 unofficial MOD)", 2025, "Williams / mypinballs", p2k, 0)
CORE_CLONEDEF(swep1, 210, 150, "Pinball 2000: Star Wars Episode I (2.10 unofficial MOD)", 2025, "Williams / mypinballs", p2k, 0)

#endif /* HAS_MEDIAGX */
