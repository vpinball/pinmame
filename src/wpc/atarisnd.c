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
/  the same sixteen-channel ramp engine that fades the lamps.  That is why the
/  sound looked like a lamp to a pulse-width test: the CPU writes a divisor and
/  a volume a few times a second, and the audio itself is made inside the chip.
/
/  Only the audio half of POKEY is modelled -- four channels, the frequency and
/  control registers and AUDCTL.  The potentiometer, keyboard and serial halves
/  are not wired to anything on this board.  The polynomial counters and the
/  dividers are clocked at the chip clock, which is the same 12.096 MHz / 8 as
/  the CPU.
/
/  $2700 is left alone.  The blit writes eight more bytes there and only one
/  CO12294 is visible in the board photograph, so it is either a mirror of the
/  same chip with A8 undecoded -- this board leaves address lines undecoded
/  elsewhere -- or a second POKEY that the photograph does not show.  Driving a
/  second chip on a guess would invent sound that may not exist.
/---------------------------------------------------------------------------*/

#define ATARI4_POKEY_CLK 1512000
#define ATARI4_POKEY_RATE 44100

static struct {
  UINT8  audf[4], audc[4], audctl;
  UINT32 div[4];        /* divider countdown, in chip clocks */
  UINT8  ff[4];         /* channel output, 0 or 1 */
  UINT32 p4, p5, p17;   /* polynomial counters */
  int    channel;
  double acc;           /* chip clocks owed to the current output sample */
} pk;

/*-- Divisor in chip clocks.  A channel normally counts the 64 kHz or 15 kHz
/   base clock, which is the chip clock divided by 28 or 114; AUDCTL can clock
/   channels 1 and 3 directly instead, and can join 1+2 and 3+4 into sixteen
/   bit counters whose output comes out of the higher channel. --*/
static UINT32 pokey_divisor(int ch) {
  const UINT32 base = (pk.audctl & 0x01) ? 114 : 28;
  if (ch == 1 && (pk.audctl & 0x10))        /* 1+2 joined, output on 2 */
    return ((pk.audc[0] & 0x00) + pk.audf[0] + 256 * pk.audf[1] + ((pk.audctl & 0x40) ? 7 : 1))
           * ((pk.audctl & 0x40) ? 1 : base);
  if (ch == 3 && (pk.audctl & 0x08))        /* 3+4 joined, output on 4 */
    return (pk.audf[2] + 256 * pk.audf[3] + ((pk.audctl & 0x20) ? 7 : 1))
           * ((pk.audctl & 0x20) ? 1 : base);
  if (ch == 0 && (pk.audctl & 0x40)) return pk.audf[0] + 4;
  if (ch == 2 && (pk.audctl & 0x20)) return pk.audf[2] + 4;
  return (pk.audf[ch] + 1) * base;
}

static int pokey_silenced(int ch) {
  /* the low half of a joined pair makes no sound of its own */
  if (ch == 0 && (pk.audctl & 0x10)) return 1;
  if (ch == 2 && (pk.audctl & 0x08)) return 1;
  return 0;
}

static void pokey_tick(void) {
  int ch;
  /*-- 4, 5 and 17 bit maximal length shift registers, taps as in the part --*/
  pk.p4  = (pk.p4  >> 1) | ((((pk.p4  ^ (pk.p4  >> 1)) & 1) ^ 1) << 3);
  pk.p5  = (pk.p5  >> 1) | ((((pk.p5  ^ (pk.p5  >> 2)) & 1) ^ 1) << 4);
  if (pk.audctl & 0x80)   /* nine bit poly instead of seventeen */
    pk.p17 = (pk.p17 >> 1) | (((pk.p17 ^ (pk.p17 >> 5)) & 1) << 8);
  else
    pk.p17 = (pk.p17 >> 1) | (((pk.p17 ^ (pk.p17 >> 5)) & 1) << 16);

  for (ch = 0; ch < 4; ch++) {
    if (pk.div[ch] == 0) pk.div[ch] = pokey_divisor(ch);
    if (--pk.div[ch]) continue;
    pk.div[ch] = pokey_divisor(ch);
    /*-- the five bit poly gates whether the channel changes at all --*/
    if (!(pk.audc[ch] & 0x20) && !(pk.p5 & 1)) continue;
    if      (pk.audc[ch] & 0x80) pk.ff[ch] ^= 1;          /* pure tone */
    else if (pk.audc[ch] & 0x40) pk.ff[ch] = pk.p4 & 1;   /* four bit poly */
    else                         pk.ff[ch] = pk.p17 & 1;  /* long poly */
  }
}

static void pokey_update(int num, INT16 *buffer, int length) {
  const double step = (double)ATARI4_POKEY_CLK / (double)ATARI4_POKEY_RATE;
  int i, ch;
  for (i = 0; i < length; i++) {
    int sample = 0;
    pk.acc += step;
    while (pk.acc >= 1.0) { pk.acc -= 1.0; pokey_tick(); }
    for (ch = 0; ch < 4; ch++) {
      const int vol = pk.audc[ch] & 0x0f;
      if (pokey_silenced(ch) || !vol) continue;
      /*-- bit 4 forces the output high so the volume itself is the signal,
      /   which is how POKEY plays sampled sound --*/
      if ((pk.audc[ch] & 0x10) || pk.ff[ch]) sample += vol;
    }
    buffer[i] = (INT16)(sample * 480);   /* 4 channels * 15 * 480 fits */
  }
}

static int atari4_sh_start(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) return -1;
  memset(&pk, 0, sizeof(pk));
  pk.p4 = pk.p5 = pk.p17 = 1;
  pk.channel = stream_init("POKEY", 100, ATARI4_POKEY_RATE, 0, pokey_update);
  return pk.channel < 0;
}

static void atari4_sh_stop(void) { }

static void atari4_sh_reset(void) {
  memset(pk.audf, 0, sizeof(pk.audf));
  memset(pk.audc, 0, sizeof(pk.audc));
  pk.audctl = 0;
}

/*-- Called from the 4x4 driver for every write in $2600-$260F. --*/
void atari4snd_w(int offset, int data) {
  if (pk.channel >= 0) stream_update(pk.channel, 0);
  switch (offset & 0x0f) {
    case 0x00: case 0x02: case 0x04: case 0x06:
      pk.audf[(offset & 0x0f) >> 1] = data; break;
    case 0x01: case 0x03: case 0x05: case 0x07:
      pk.audc[(offset & 0x0f) >> 1] = data; break;
    case 0x08: pk.audctl = data; break;
    default: break;   /* STIMER, SKRES, POTGO, SEROUT, IRQEN, SKCTL: unused here */
  }
}

static struct CustomSound_interface atari4s_custInt =
  {atari4_sh_start, atari4_sh_stop, NULL, atari4_sh_reset};

MACHINE_DRIVER_START(atari4s)
  MDRV_SOUND_ADD(CUSTOM, atari4s_custInt)
MACHINE_DRIVER_END
