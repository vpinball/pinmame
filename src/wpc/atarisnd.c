/*----------------------------------------
/              Atari sound
/-----------------------------------------*/

//!! Remaining issue: The 1000 point sound on Airborne Avenger sounds quite different on most videos I found.
//   It's not a dump issue at least, since the sound itself uses waveform 0 (sine wave) and that waveform is used for a bunch of other sounds, which are good.
//   I think that there's some filtering and/or clipping going on at the DAC output section.
//   My guess is that it's a side effect from having the volume pot turned up, the manual itself actually mentions distortion.

#include <stdlib.h>
#include "driver.h"
#include "core.h"
#include "sndbrd.h"

#define GEN1_SND_CLK 125000

static struct {
  struct sndbrdData brdData;
  int    sound, volume, octave, frequency, waveform;
  int    channel, noisechannel;
} atarilocals;

static int  atari_sh_start1(const struct MachineSound *msound);
static int  atari_sh_start2(const struct MachineSound *msound);
static void atari_sh_stop (void);
static void atari_sh_reset1(void);
static void atari_sh_reset2(void);
static WRITE_HANDLER(atari_data1_w);
static WRITE_HANDLER(atari_ctrl1_w);
static WRITE_HANDLER(atari_data_w);
static WRITE_HANDLER(atari_ctrl_w);

static void atari_init(struct sndbrdData *brdData) {
  memset(&atarilocals, 0, sizeof(atarilocals));
  atarilocals.brdData = *brdData;
}

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf atari1sIntf = {
  "ATARI1", atari_init, NULL, NULL, atari_data1_w, atari_data1_w, NULL, atari_ctrl1_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf atari2sIntf = {
  "ATARI2", atari_init, NULL, NULL, atari_data_w, atari_data_w, NULL, atari_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct CustomSound_interface atari1s_custInt = {atari_sh_start1, atari_sh_stop, NULL, atari_sh_reset1};
static struct CustomSound_interface atari2s_custInt = {atari_sh_start2, atari_sh_stop, NULL, atari_sh_reset2};

MACHINE_DRIVER_START(atari1s)
  MDRV_SOUND_ADD(CUSTOM, atari1s_custInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(atari2s)
  MDRV_SOUND_ADD(CUSTOM, atari2s_custInt)
MACHINE_DRIVER_END

/* waveform for the audio hardware */
static UINT8 romWave[32];

/* memory for noise */
static UINT8 noiseWave[2000];

static void stopSound(void) {
  if (mixer_is_sample_playing(atarilocals.channel)) {
    mixer_stop_sample(atarilocals.channel);
  }
}

static void stopNoise(void) {
  if (mixer_is_sample_playing(atarilocals.noisechannel)) {
    mixer_stop_sample(atarilocals.noisechannel);
  }
}

static void playSound(void) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return;
  }
  if (atarilocals.sound & 0x02) { // noise on
    int i;
    for (i=0; i < sizeof(noiseWave); i++) {
      noiseWave[i] = (UINT8)(rand() % 256);
    }
    mixer_set_volume(atarilocals.noisechannel, atarilocals.volume*4);
    mixer_play_sample(atarilocals.noisechannel, (signed char *)noiseWave, sizeof(noiseWave),
      4000 + 26000 / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
  }
  if (atarilocals.sound & 0x01) { // wave on
    int i;
    mixer_set_volume(atarilocals.channel, atarilocals.volume*4);
    for (i=0; i < 32; i++) { // copy in the correct waveform from the sound PROM
      const UINT8 byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
      romWave[i] = 0x80 + (byte | (byte << 4));
    }
    if (mixer_is_sample_playing(atarilocals.channel)) {
      mixer_set_sample_frequency(atarilocals.channel,
        4000 + 26000 / (16-atarilocals.frequency) * (1 << atarilocals.octave));
    } else {
      mixer_play_sample(atarilocals.channel, (signed char *)romWave, sizeof(romWave),
        4000 + 26000 / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
    }
  }
}

static void playSound1(void) {
	if (atarilocals.sound & 0x01) { // wave on
		int i;
		mixer_set_volume(atarilocals.channel, atarilocals.volume * 4);
		for (i = 0; i < 32; i++) { // copy in the correct waveform from the sound PROM
			UINT8 byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
			romWave[i] = 0x80 + (byte | (byte << 4));
		}
		if (mixer_is_sample_playing(atarilocals.channel)) {
			mixer_set_sample_frequency(atarilocals.channel,
				GEN1_SND_CLK / (16 - atarilocals.frequency));
		}
		else {
			mixer_play_sample(atarilocals.channel, (signed char *)romWave, sizeof(romWave),
				GEN1_SND_CLK / (16 - atarilocals.frequency), 1);
		}
	}
}

static int atari_sh_start1(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }

  atarilocals.channel = mixer_allocate_channel(20);
  atarilocals.sound = 0;
  return 0;
}

static int atari_sh_start2(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }

  atarilocals.channel = mixer_allocate_channel(50);
  atarilocals.noisechannel = mixer_allocate_channel(25);
  return 0;
}

static void atari_sh_stop(void) {
}

static void atari_sh_reset1(void) {
  stopSound();
}

static void atari_sh_reset2(void) {
  stopSound();
  stopNoise();
}

static WRITE_HANDLER(atari_data1_w) {
	// frequency and volume
	UINT8 Freq = data & 0xF;
	UINT8 Volume = (data & 0xF0) >> 4;

	if (Freq != atarilocals.frequency)
	{
		atarilocals.frequency = Freq;
	}
	if (Volume != atarilocals.volume)
	{
		atarilocals.volume = Volume;
	}
	playSound1();
}

static WRITE_HANDLER(atari_ctrl1_w) {
	// waveform and enable
	UINT8 Wave = data & 0xF;
	int Enable = (data & 0x10) ? 1 : 0;
	if (Wave != atarilocals.waveform)
	{
		atarilocals.waveform = Wave;
	}
	if (atarilocals.sound != Enable)
	{
		atarilocals.sound = Enable;
		if (Enable == 0) stopSound();
	}
	playSound1();
}

static WRITE_HANDLER(atari_ctrl_w) {
  int noise = data & 0x80;
  int sound = data & 0x40;
  if (noise && !(atarilocals.sound & 0x02)) {
    logerror("noise on\n");
  }
  if (!noise && (atarilocals.sound & 0x02)) {
    logerror("noise off\n");
    stopNoise();
  }
  if (sound && !(atarilocals.sound & 0x01)) {
    logerror("wave on\n");
  }
  if (!sound && (atarilocals.sound & 0x01)) {
    logerror("wave off\n");
    stopSound();
  }
  if (atarilocals.octave != ((data >> 4) & 0x03)) {
    atarilocals.octave = (data >> 4) & 0x03;
    logerror("octave=%d\n", atarilocals.octave);
  }
  if (atarilocals.waveform != (data & 0x0f)) {
    atarilocals.waveform = data & 0x0f;
    logerror("waveform=%d\n", atarilocals.waveform);
  }
  if (atarilocals.sound != (data >> 6)) {
    atarilocals.sound = data >> 6;
    playSound();
  }
}

static WRITE_HANDLER(atari_data_w) {
  if (atarilocals.frequency != (data >> 4)) {
    atarilocals.frequency = data >> 4;
    logerror("freq.div=%d\n", atarilocals.frequency);
  }

  if (atarilocals.volume != (data & 0x0f)) {
    atarilocals.volume = data & 0x0f;
    logerror("amplitude=%d\n", atarilocals.volume);
  }
  playSound();
}

/*----------------------------------------------------------------------------
/  4x4: POKEY
/
/  The CPU board carries an Atari CO12294 -- POKEY -- at position C, date code
/  8151, and the game writes its first eight registers at $2600-$2607 through
/  the same sixteen-channel ramp engine that fades the lamps: the CPU writes a
/  divisor and a volume a few times a second, and the audio itself is made
/  inside the chip.
/
/  Only the audio half of POKEY is modelled -- four channels, the frequency and
/  control registers and AUDCTL.  The potentiometer, keyboard and serial halves
/  are not wired to anything on this board.  The polynomial counters and the
/  dividers are clocked at the chip clock, which is the same 1.5 MHz as the
/  CPU, the figure given by one of the two owners.
/
/  There are two of them, $2600 and $2700, confirmed by one of the two owners;
/  only one is legible in the board photograph.  Eight channels, and the ramp
/  engine's sixteen slots map one to one onto their sixteen registers.
/---------------------------------------------------------------------------*/

#define ATARI4_POKEY_CLK 1500000
#define ATARI4_POKEY_RATE 44100

#define ATARI4_POKEYS 2

static struct pokey_chip {
  UINT8  audf[4], audc[4], audctl, skctl;
  UINT32 div[4];        /* divider countdown, in chip clocks */
  UINT8  ff[4];         /* channel output, 0 or 1 */
  UINT32 p4, p5, p17;   /* polynomial counters, one set per chip */
  UINT32 r9, r17;       /* RANDOM: positions in rand9/rand17, see atari4snd_r */
  double rtime;         /* time those positions are as of */
} pk[ATARI4_POKEYS];

static struct {
  int    channel;
  double acc;           /* chip clocks owed to the current output sample */
  double dcx, dcy;      /* DC blocker state, see pokey_update */
} pkmix;

/*-- What this game asks of a POKEY.
/
/   The sound image is sixteen bytes at $31D6 and $961E blits it to registers
/   0-7 of each chip -- AUDF1-4 and AUDC1-4 and nothing else.  Everything
/   outside that is written directly, and there are only three places:
/
/     $DF8B   zeroes all sixteen registers of both chips, then writes $03 to
/             SKCTL.  This is the only write AUDCTL ever gets.
/     $93D3   POTGO, which starts a pot scan; the DIP switches are read back
/             through ALLPOT, see slice2_r in atari.c
/     $992F   POTGO again, on the second chip as well
/
/   AUDCTL therefore stays 0 for the life of the machine.  The joined
/   sixteen-bit channels, the 1.79 MHz channel clocking, the 15 kHz base and
/   the nine-bit polynomial are all implemented below but unreachable in this
/   game; they are here because they are what the part does.  The two
/   high-pass filters are AUDCTL bits 1 and 2 and so are unreachable too, and
/   are not implemented.  Two-tone mode is SKCTL bit 3, and the only value
/   SKCTL is given is $03.
/
/   SKCTL's reset does apply: while bits 0 and 1 are both zero the chip is held
/   with its polynomial counters stopped --*/

/*-- Divisor in chip clocks.  A channel normally counts the 64 kHz or 15 kHz
/   base clock, which is the chip clock divided by 28 or 114; AUDCTL can clock
/   channels 1 and 3 directly instead, and can join 1+2 and 3+4 into sixteen
/   bit counters whose output comes out of the higher channel. --*/
static UINT32 pokey_divisor(const struct pokey_chip *c, int ch) {
  const UINT32 base = (c->audctl & 0x01) ? 114 : 28;
  if (ch == 1 && (c->audctl & 0x10))        /* 1+2 joined, output on 2 */
    return (c->audf[0] + 256 * c->audf[1] + ((c->audctl & 0x40) ? 7 : 1))
           * ((c->audctl & 0x40) ? 1 : base);
  if (ch == 3 && (c->audctl & 0x08))        /* 3+4 joined, output on 4 */
    return (c->audf[2] + 256 * c->audf[3] + ((c->audctl & 0x20) ? 7 : 1))
           * ((c->audctl & 0x20) ? 1 : base);
  if (ch == 0 && (c->audctl & 0x40)) return c->audf[0] + 4;
  if (ch == 2 && (c->audctl & 0x20)) return c->audf[2] + 4;
  return (c->audf[ch] + 1) * base;
}

static int pokey_silenced(const struct pokey_chip *c, int ch) {
  /* the low half of a joined pair makes no sound of its own */
  if (ch == 0 && (c->audctl & 0x10)) return 1;
  if (ch == 2 && (c->audctl & 0x08)) return 1;
  return 0;
}

/*-- A channel at volume zero cannot be heard, so its divider and flip-flop
/   need not be advanced; when the volume comes back the phase it resumes on
/   is inaudible.  A chip with all four silent needs no polynomial stepping
/   either.  Most of the time most channels are quiet, and the chips tick at
/   1.5 MHz, so this is most of the cost. --*/
static int pokey_audible(const struct pokey_chip *c) {
  /* SKCTL bits 0 and 1 both zero hold the whole chip, polynomials included,
     in reset; it comes out of power-up that way until the game writes $03 */
  if (!(c->skctl & 0x03)) return 0;
  return (c->audc[0] | c->audc[1] | c->audc[2] | c->audc[3]) & 0x0f;
}

static void pokey_tick(struct pokey_chip *c) {
  int ch;
  /*-- 4, 5 and 17 bit maximal length shift registers, taps as in the part --*/
  c->p4 = (c->p4 >> 1) | ((((c->p4 ^ (c->p4 >> 1)) & 1) ^ 1) << 3);
  c->p5 = (c->p5 >> 1) | ((((c->p5 ^ (c->p5 >> 2)) & 1) ^ 1) << 4);
  if (c->audctl & 0x80)   /* nine bit poly instead of seventeen */
    c->p17 = (c->p17 >> 1) | (((c->p17 ^ (c->p17 >> 5)) & 1) << 8);
  else
    c->p17 = (c->p17 >> 1) | (((c->p17 ^ (c->p17 >> 5)) & 1) << 16);

  for (ch = 0; ch < 4; ch++) {
    if (!(c->audc[ch] & 0x0f)) continue;
    if (c->div[ch] == 0) c->div[ch] = pokey_divisor(c, ch);
    if (--c->div[ch]) continue;
    c->div[ch] = pokey_divisor(c, ch);
    /*-- unless AUDC bit 7 is set, the five bit poly gates whether the channel changes at all --*/
    if (!(c->audc[ch] & 0x80) && !(c->p5 & 1)) continue;
    if      (c->audc[ch] & 0x20) c->ff[ch] ^= 1;          /* pure tone */
    else if (c->audc[ch] & 0x40) c->ff[ch] = c->p4 & 1;   /* four bit poly */
    else                         c->ff[ch] = c->p17 & 1;  /* long poly */
  }
}

static void pokey_update(int num, INT16 *buffer, int length) {
  const double step = (double)ATARI4_POKEY_CLK / (double)ATARI4_POKEY_RATE;
  int i, ch, n;
  for (i = 0; i < length; i++) {
    int sample = 0;
    pkmix.acc += step;
    while (pkmix.acc >= 1.0) {
      pkmix.acc -= 1.0;
      for (n = 0; n < ATARI4_POKEYS; n++)
        if (pokey_audible(&pk[n])) pokey_tick(&pk[n]);
    }
    for (n = 0; n < ATARI4_POKEYS; n++)
      for (ch = 0; ch < 4; ch++) {
        const int vol = pk[n].audc[ch] & 0x0f;
        if (pokey_silenced(&pk[n], ch) || !vol) continue;
        /*-- bit 4 forces the output high so the volume itself is the signal,
        /   which is how POKEY plays sampled sound --*/
        if ((pk[n].audc[ch] & 0x10) || pk[n].ff[ch]) sample += vol;
      }
    /*-- Eight channels at volume 15 is 120, and both chips feed the one
    /   amplifier.  POKEY sums current, so its output only ever swings one
    /   way; on the board a coupling capacitor sits between it and the
    /   amplifier and the speaker never sees the offset.  The one-pole
    /   high-pass here does the same, at about 3 Hz. --*/
    { const double in = sample * 240.0;
      pkmix.dcy = in - pkmix.dcx + 0.9995 * pkmix.dcy;
      pkmix.dcx = in;
      buffer[i] = (INT16)(pkmix.dcy < -32768.0 ? -32768.0
                        : pkmix.dcy >  32767.0 ?  32767.0 : pkmix.dcy); }
  }
}

static int atari4_sh_start(const struct MachineSound *msound) {
  int n;
  if (Machine->gamedrv->flags & GAME_NO_SOUND) return -1;
  memset(&pk, 0, sizeof(pk));
  memset(&pkmix, 0, sizeof(pkmix));
  for (n = 0; n < ATARI4_POKEYS; n++) pk[n].p4 = pk[n].p5 = pk[n].p17 = 1;
  pkmix.channel = stream_init("POKEY", 100, ATARI4_POKEY_RATE, 0, pokey_update);
  return pkmix.channel < 0;
}

static void atari4_sh_stop(void) { }

static void atari4_sh_reset(void) {
  int n;
  for (n = 0; n < ATARI4_POKEYS; n++) {
    memset(pk[n].audf, 0, sizeof(pk[n].audf));
    memset(pk[n].audc, 0, sizeof(pk[n].audc));
    pk[n].audctl = 0;
  }
}

/*-- Called from the 4x4 driver for every write in $2600-$260F and
/   $2700-$270F; chip is 0 or 1. --*/
void atari4snd_w(int chip, int offset, int data) {
  struct pokey_chip *c = &pk[chip & (ATARI4_POKEYS - 1)];
  if (pkmix.channel >= 0) stream_update(pkmix.channel, 0);
  switch (offset & 0x0f) {
    case 0x00: case 0x02: case 0x04: case 0x06:
      c->audf[(offset & 0x0f) >> 1] = data; break;
    case 0x01: case 0x03: case 0x05: case 0x07:
      c->audc[(offset & 0x0f) >> 1] = data; break;
    case 0x08: c->audctl = data; break;
    case 0x0f:
      /* SKCTL.  Bits 0 and 1 both zero hold the chip in reset: the
         polynomial counters stop and nothing is heard.  The game writes 0 to
         all sixteen registers at $DF8B and then $03 here, which is the
         ordinary release; it never sets bit 3, so two-tone mode never comes on */
      c->skctl = data;
      if (!(data & 0x03)) {
        c->p4 = c->p5 = c->p17 = 1;
        memset(c->div, 0, sizeof(c->div));
        memset(c->ff, 0, sizeof(c->ff));
        c->r9 = c->r17 = 0;
        c->rtime = timer_get_time();
      }
      break;
    default: break;   /* STIMER, SKRES, POTGO, SEROUT, IRQEN: not written */
  }
}

/*-- The read side.  POT0-7 and ALLPOT carry the DIP switches and are answered
/   by atari.c; the keyboard and serial halves are not wired, so KBCODE and
/   SERIN read 0, and IRQST and SKSTAT read $FF (all active low: nothing
/   pending, no error, no key).
/
/   RANDOM reads eight bits of the 17 bit polynomial (9 bit with AUDCTL bit 7),
/   which runs at the chip clock whether or not anything is audible, so its
/   position is worked out from the time elapsed rather than stepped.  Like
/   MAME's POKEY the bits come from tables of the counter's sequence, and the
/   read is their complement. --*/
static UINT8 rand9[0x1ff], rand17[0x1ffff];

static void rand_init(UINT8 *rng, int size, int left, int right, int add) {
  const int mask = (1 << size) - 1;
  int i, x = 0;
  for (i = 0; i < mask; i++) {
    *rng++ = (size == 17) ? (UINT8)(x >> 6) : (UINT8)x;  /* bits 6-13, or 0-7 */
    x = ((x << left) + (x >> right) + add) & mask;
  }
}

int atari4snd_r(int chip, int offset) {
  static int rand_ready = 0; //!! ??
  struct pokey_chip *c = &pk[chip & (ATARI4_POKEYS - 1)];
  switch (offset & 0x0f) {
    case 0x0a: { /* RANDOM */
      const double now = timer_get_time();
      if (!rand_ready) {
        rand_init(rand9, 9, 8, 1, 0x00180);
        rand_init(rand17, 17, 16, 1, 0x1c000);
        rand_ready = 1;
      }
      if (!(c->skctl & 0x03) || now < c->rtime) { /* in reset, or a new run */
        c->r9 = c->r17 = 0;
        c->rtime = now;
      }
      else {
        const UINT32 adjust = (UINT32)((now - c->rtime) * ATARI4_POKEY_CLK);
        c->r9  = (c->r9  + adjust) % 0x1ff;
        c->r17 = (c->r17 + adjust) % 0x1ffff;
        c->rtime += adjust / (double)ATARI4_POKEY_CLK; /* keep the part of a clock not yet counted */
      }
      return ((c->audctl & 0x80) ? rand9[c->r9] : rand17[c->r17]) ^ 0xff;
    }
    case 0x0e: case 0x0f: return 0xff; /* IRQST, SKSTAT */
    default: return 0;                 /* KBCODE, SERIN, unused */
  }
}

static struct CustomSound_interface atari4s_custInt =
  {atari4_sh_start, atari4_sh_stop, NULL, atari4_sh_reset};

MACHINE_DRIVER_START(atari4s)
  MDRV_SOUND_ADD(CUSTOM, atari4s_custInt)
MACHINE_DRIVER_END
