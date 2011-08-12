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

static int  atari_sh_start(const struct MachineSound *msound);
static void atari_sh_stop (void);
static WRITE_HANDLER(atari_gen1_w);
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
  "ATARI1", atari_init, NULL, NULL, atari_gen1_w, atari_gen1_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf atari2sIntf = {
  "ATARI2", atari_init, NULL, NULL, atari_data_w, atari_data_w, NULL, atari_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct CustomSound_interface atari1s_custInt = {atari_sh_start, atari_sh_stop};
static struct CustomSound_interface atari2s_custInt = {atari_sh_start, atari_sh_stop};

MACHINE_DRIVER_START(atari1s)
  MDRV_SOUND_ADD(CUSTOM, atari1s_custInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(atari2s)
  MDRV_SOUND_ADD(CUSTOM, atari2s_custInt)
MACHINE_DRIVER_END

#define ATARI_SNDFREQ 50000 /* audio base frequency in Hz */

/* waveform for the audio hardware */
static UINT8 romWave[32];

static const UINT8 sineWave[] = {
  0x00, 0x18, 0x30, 0x46, //   ***
  0x5a, 0x69, 0x75, 0x7d, //  *   *
  0x7f, 0x7d, 0x75, 0x69, // *     *
  0x5a, 0x46, 0x30, 0x18, // *     *
  0x00, 0xe7, 0xcf, 0xb9, //        *     *
  0xa5, 0x96, 0x8a, 0x82, //        *     *
  0x80, 0x82, 0x8a, 0x96, //         *   *
  0xa5, 0xb9, 0xcf, 0xe7, //          ***
};

static const UINT8 triangleWave[] = {
  0x00, 0x0f, 0x1f, 0x2f, //    *
  0x3f, 0x4f, 0x5f, 0x6f, //   * *
  0x7f, 0x6f, 0x5f, 0x4f, //  *   *
  0x3f, 0x2f, 0x1f, 0x0f, // *     *
  0x00, 0xf1, 0xe1, 0xd1, //        *     *
  0xc1, 0xb1, 0xa1, 0x91, //         *   *
  0x81, 0x91, 0xa1, 0xb1, //          * *
  0xc1, 0xd1, 0xe1, 0xf1, //           *
};

static const UINT8 sawtoothWave[] = {
  0x02, 0x0a, 0x12, 0x1a, //       *
  0x22, 0x2a, 0x32, 0x3a, //     * *
  0x42, 0x4a, 0x52, 0x5b, //   *   *
  0x63, 0x6b, 0x73, 0x7b, // *     *
  0x81, 0x89, 0x91, 0x99, //       *     *
  0xa1, 0xa9, 0xb2, 0xba, //       *   *
  0xc2, 0xca, 0xd2, 0xda, //       * *
  0xe2, 0xea, 0xf2, 0xfa, //       *
};

static const UINT8 squareWave[] = {
  0x7f, 0x7f, 0x7f, 0x7f, // *******
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x7f, 0x7f, 0x7f, 0x7f, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80, //       *
  0x80, 0x80, 0x80, 0x80  //       *******
};

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
      ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
  }
  if (atarilocals.sound & 0x01) { // wave on
    int i;
    UINT8 byte;
    mixer_set_volume(atarilocals.channel, atarilocals.volume*4);
    for (i=0; i < 32; i++) { // copy in the correct waveform from the sound PROM
      byte = memory_region(REGION_SOUND1)[32 * atarilocals.waveform + i] & 0x0f;
      romWave[i] = 0x80 + (byte | (byte << 4));
    }
    mixer_play_sample(atarilocals.channel, (signed char *)romWave, sizeof(romWave),
      ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
  }
}

static int atari_sh_start(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }

  atarilocals.channel = mixer_allocate_channel(60);
  atarilocals.noisechannel = mixer_allocate_channel(30);
  return 0;
}

static void atari_sh_stop(void) {
}

static WRITE_HANDLER(atari_gen1_w) {
  if (data) {
    atarilocals.frequency = data & 0x0f;
    atarilocals.waveform = data >> 4;
    if (atarilocals.waveform < 4) {
      mixer_play_sample(atarilocals.channel, (signed char *)squareWave, sizeof(squareWave),
        2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
    } else if (atarilocals.waveform < 8) {
      mixer_play_sample(atarilocals.channel, (signed char *)triangleWave, sizeof(triangleWave),
        2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
    } else if (atarilocals.waveform < 12) {
      mixer_play_sample(atarilocals.channel, (signed char *)sineWave, sizeof(sineWave),
        2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
    } else {
      mixer_play_sample(atarilocals.channel, (signed char *)sawtoothWave, sizeof(sawtoothWave),
        2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
    }
  } else if (mixer_is_sample_playing(atarilocals.channel)) {
    mixer_stop_sample(atarilocals.channel);
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
