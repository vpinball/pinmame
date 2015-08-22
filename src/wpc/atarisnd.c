/*----------------------------------------
/              Atari sound
/-----------------------------------------*/
#include <stdlib.h>
#include "driver.h"
#include "core.h"
#include "sndbrd.h"

static struct {
  struct sndbrdData brdData;
  int    sound, volume, octave, frequency, waveform;
  int    channel, noisechannel;
} atarilocals;

static int  atari_sh_start1(const struct MachineSound *msound);
static int  atari_sh_start2(const struct MachineSound *msound);
static void atari_sh_stop (void);
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
static struct CustomSound_interface atari1s_custInt = {atari_sh_start1, atari_sh_stop};
static struct CustomSound_interface atari2s_custInt = {atari_sh_start2, atari_sh_stop};

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

static int atari_sh_start1(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }

  atarilocals.channel = mixer_allocate_channel(20);
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

static WRITE_HANDLER(atari_data1_w) {
  static int delay; // used to delay the mixer_stop_sample call by 1 iteration (makes sound smoother)
  if (data) {
    delay = 0;
    atarilocals.frequency = data < 0x10 ? 0x10 | data : data;
    if (mixer_is_sample_playing(atarilocals.channel)) {
      mixer_set_sample_frequency(atarilocals.channel, 500 * atarilocals.frequency);
    } else {
      mixer_play_sample(atarilocals.channel, (signed char *)romWave, sizeof(romWave), 500 * atarilocals.frequency, 1);
    }
  } else {
    if (!delay) {
      delay = 1;
    } else {
      delay = 0;
      mixer_stop_sample(atarilocals.channel);
    }
  }
}

static WRITE_HANDLER(atari_ctrl1_w) {
  int i;
  UINT8 byte;
  atarilocals.waveform = data & 0x0f;
  for (i=0; i < 32; i++) { // copy in the correct waveform from the sound PROM
    byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
    romWave[i] = 0x80 + (byte | (byte << 4));
  }
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
