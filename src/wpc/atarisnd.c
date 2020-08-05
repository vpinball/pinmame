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
    UINT8 byte;
    mixer_set_volume(atarilocals.channel, atarilocals.volume*4);
    for (i=0; i < 32; i++) { // copy in the correct waveform from the sound PROM
      byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
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
		UINT8 byte;
		mixer_set_volume(atarilocals.channel, atarilocals.volume * 4);
		for (i = 0; i < 32; i++) { // copy in the correct waveform from the sound PROM
			byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
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
