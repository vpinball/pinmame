/*----------------------------------------
/              Atari sound
/-----------------------------------------*/
#include "driver.h"
#include "core.h"
#include "sndbrd.h"

/*-------------------
/ exported interface
/--------------------*/
static int  atari_sh_start(const struct MachineSound *msound);
static void atari_sh_stop (void);

struct CustomSound_interface atari_custInt = {atari_sh_start, atari_sh_stop};

#define ATARI_DECAYTIME   130 /* decay for sound in milliseconds */
#define ATARI_SNDFREQ   62500 /* audio base frequency in Hz */

/* waveform for the audio hardware */
static const UINT8 sineWave[] = {
  204,188,163,214,252,188,115,125,136,63,0,37,88,63,47,125
};

static struct {
  int    sound, volume, octave, frequency, waveform;
  int    channel;
  void   *vtimer;
} atarilocals;

static void setSoundMode(int mode)    { atarilocals.sound = mode; }
static void setVolume(int volume)     { atarilocals.volume = volume; }
static void setOctave(int octave)     { atarilocals.octave = octave; }
static int  getOctave(void)           { return atarilocals.octave; }
static void setFrequency(int freq)    { atarilocals.frequency = freq; }
static void setWaveform(int waveform) { atarilocals.waveform = waveform; }

static void atari_decay(int dummy) {
	if (mixer_is_sample_playing(atarilocals.channel)) mixer_stop_sample(atarilocals.channel);
}

static void playSound(void) {
	if (Machine->gamedrv->flags & GAME_NO_SOUND)
		return;

	if (atarilocals.sound == 0) {
		mixer_stop_sample(atarilocals.channel);
		return;
	}

	if (mixer_is_sample_playing(atarilocals.channel)) mixer_stop_sample(atarilocals.channel);
	mixer_set_volume(atarilocals.channel, atarilocals.volume * 15);
    mixer_play_sample(atarilocals.channel, (signed char *)sineWave, sizeof(sineWave), 1000, 1);
    mixer_set_sample_frequency(atarilocals.channel,
	  ATARI_SNDFREQ / (16-atarilocals.frequency) * (atarilocals.octave+1));
    atarilocals.vtimer = timer_set(TIME_IN_MSEC(ATARI_DECAYTIME),0,atari_decay);
}

static int atari_sh_start(const struct MachineSound *msound) {
    if (!atarilocals.channel) atarilocals.channel = mixer_allocate_channel(15);
    return 0;
}

static void atari_sh_stop(void) {
}

void atari_snd0_w(int data) {
	int play = 0;
	if (data & 0x80) {
		logerror("noise on\n");
		setSoundMode(0);
	}
	if (data & 0x40) {
		logerror("wave on\n");
		setSoundMode(1);
	}
	if (getOctave() != ((data >> 4) & 0x03)) {
		setOctave((data >> 4) & 0x03);
		logerror("octave=%d\n", getOctave());
		play = 1;
	}
	if (data & 0x0f) {
		setWaveform(data & 0x0f);
		logerror("waveform=%d\n", data & 0x0f);
		play = 1;
	}
	if (play) playSound();
}

void atari_snd1_w(int data) {
	if (data & 0xf0) {
		logerror("freq.div=%d\n", data >> 4);
		setFrequency(data >> 4);
	}

	if (data & 0x0f) {
		logerror("amplitude=%d\n", data & 0x0f);
		setVolume(data & 0x0f);
	}
	playSound();
}
