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

#define ATARI_SNDFREQ   62500 /* audio base frequency in Hz */

/* waveform for the audio hardware */
static const UINT8 sineWave[] = {
//	204,188,163,214,252,188,115,125,136,63,0,37,88,63,47,125
	0x00, 0x0c, 0x18, 0x24, 0x30, 0x3b, 0x46, 0x50, //   ***
	0x5a, 0x62, 0x69, 0x70, 0x75, 0x79, 0x7d, 0x7e, //  *   *
	0x7f, 0x7e, 0x7d, 0x79, 0x75, 0x70, 0x69, 0x62, // *     *
	0x5a, 0x50, 0x46, 0x3b, 0x30, 0x24, 0x18, 0x0c, // *     *
	0x00, 0xf3, 0xe7, 0xdb, 0xcf, 0xc4, 0xb9, 0xaf, //        *     *
	0xa5, 0x9d, 0x96, 0x8f, 0x8a, 0x86, 0x82, 0x81, //        *     *
	0x80, 0x81, 0x82, 0x86, 0x8a, 0x8f, 0x96, 0x9d, //         *   *
	0xa5, 0xaf, 0xb9, 0xc4, 0xcf, 0xdb, 0xe7, 0xf3  //          ***
};

static const UINT8 triangleWave[] = {
	0x00, 0x07, 0x0f, 0x17, 0x1f, 0x27, 0x2f, 0x37, //    *
	0x3f, 0x47, 0x4f, 0x57, 0x5f, 0x67, 0x6f, 0x77, //   * *
	0x7f, 0x77, 0x6f, 0x67, 0x5f, 0x57, 0x4f, 0x47, //  *   *
	0x3f, 0x37, 0x2f, 0x27, 0x1f, 0x17, 0x0f, 0x07, // *     *
	0x00, 0xf9, 0xf1, 0xe9, 0xe1, 0xd9, 0xd1, 0xc9, //        *     *
	0xc1, 0xb9, 0xb1, 0xa9, 0xa1, 0x99, 0x91, 0x89, //         *   *
	0x81, 0x89, 0x91, 0x99, 0xa1, 0xa9, 0xb1, 0xb9, //          * *
	0xc1, 0xc9, 0xd1, 0xd9, 0xe1, 0xe9, 0xf1, 0xf9  //           *
};

static const UINT8 sawtoothWave[] = {
	0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e, //       *
	0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e, //     * *
	0x42, 0x46, 0x4a, 0x4e, 0x52, 0x57, 0x5b, 0x5f, //   *   *
	0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f, // *     *
	0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d, //       *     *
	0xa1, 0xa5, 0xa9, 0xae, 0xb2, 0xb6, 0xba, 0xbe, //       *   *
	0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd6, 0xda, 0xde, //       * *
	0xe2, 0xe6, 0xea, 0xee, 0xf2, 0xf6, 0xfa, 0xfe  //       *
};

static const UINT8 squareWave[] = {
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, // *******
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, //       *
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, //       *
	0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, //       *
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, //       *
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, //       *
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, //       *
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80  //       *******
};

/* memory for noise */
static UINT8 noiseWave[2000];

static void stopSound(void) {
	if (mixer_is_sample_playing(atarilocals.channel))
		mixer_stop_sample(atarilocals.channel);
}

static void stopNoise(void) {
	if (mixer_is_sample_playing(atarilocals.noisechannel))
		mixer_stop_sample(atarilocals.noisechannel);
}

static void playSound(void) {
	if (Machine->gamedrv->flags & GAME_NO_SOUND)
		return;

	if (atarilocals.sound & 0x02) { // noise on
		int i;
		for (i=0; i < sizeof(noiseWave); i++)
			noiseWave[i] = (UINT8)(rand() % 256);
		stopNoise();
		mixer_set_volume(atarilocals.noisechannel, atarilocals.volume*4);
		mixer_play_sample(atarilocals.noisechannel, (signed char *)noiseWave, sizeof(noiseWave),
		  ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
	}
	if (atarilocals.sound & 0x01) { // wave on
		stopSound();
		mixer_set_volume(atarilocals.channel, atarilocals.volume*4);
		if (atarilocals.waveform < 4)
			mixer_play_sample(atarilocals.channel, (signed char *)squareWave, sizeof(squareWave),
			  2 * ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
		else if (atarilocals.waveform < 8)
 			mixer_play_sample(atarilocals.channel, (signed char *)triangleWave, sizeof(triangleWave),
			  2 * ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
		else if (atarilocals.waveform < 12)
			mixer_play_sample(atarilocals.channel, (signed char *)sineWave, sizeof(sineWave),
			  2 * ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
		else
			mixer_play_sample(atarilocals.channel, (signed char *)sawtoothWave, sizeof(sawtoothWave),
			  2 * ATARI_SNDFREQ / (16-atarilocals.frequency) * (1 << atarilocals.octave), 1);
	}
}

static int atari_sh_start(const struct MachineSound *msound) {
	if (Machine->gamedrv->flags & GAME_NO_SOUND)
		return -1;

   	atarilocals.channel = mixer_allocate_channel(50);
   	atarilocals.noisechannel = mixer_allocate_channel(25);
    return 0;
}

static void atari_sh_stop(void) {
}

static WRITE_HANDLER(atari_gen1_w) {
	if (data) {
		atarilocals.frequency = data & 0x0f;
		atarilocals.waveform = data >> 4;
		if (atarilocals.waveform < 4)
			mixer_play_sample(atarilocals.channel, (signed char *)squareWave, sizeof(squareWave),
			  2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
		else if (atarilocals.waveform < 8)
 			mixer_play_sample(atarilocals.channel, (signed char *)triangleWave, sizeof(triangleWave),
			  2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
		else if (atarilocals.waveform < 12)
			mixer_play_sample(atarilocals.channel, (signed char *)sineWave, sizeof(sineWave),
			  2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
		else
			mixer_play_sample(atarilocals.channel, (signed char *)sawtoothWave, sizeof(sawtoothWave),
			  2 * ATARI_SNDFREQ / (16 - atarilocals.frequency), 1);
	} else {
		if (mixer_is_sample_playing(atarilocals.channel))
			mixer_stop_sample(atarilocals.channel);
	}
}

static WRITE_HANDLER(atari_ctrl_w) {
	int noise = data & 0x80;
	int sound = data & 0x40;
	if (noise && !(atarilocals.sound & 0x02))
		logerror("noise on\n");
	if (!noise && (atarilocals.sound & 0x02)) {
		logerror("noise off\n");
		stopNoise();
	}
	if (sound && !(atarilocals.sound & 0x01))
		logerror("wave on\n");
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
