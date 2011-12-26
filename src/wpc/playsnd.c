#include "driver.h"
#include "core.h"
#include "play.h"
#include "sndbrd.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/ay8910.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT8  sndCmd;
  int    enSn;
  int    volume;
  int    channel;
  mame_timer *timer;
} sndlocals;

static void play1s_timer_callback(int n) {
  if (sndlocals.volume) mixer_set_volume(0, (--sndlocals.volume));
  else timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
}

static void play1s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.timer = timer_alloc(play1s_timer_callback);
  sndlocals.volume = 100;
}

static WRITE_HANDLER(play1s_data_w) {
  static UINT8 oldData;
  static int timer_on;
  if (data & 0x0f) {
    if (oldData != data) sndlocals.volume = 100;
    oldData = data;
    discrete_sound_w(8, data & 0x01);
    discrete_sound_w(4, data & 0x02);
    discrete_sound_w(2, data & 0x04);
    discrete_sound_w(1, data & 0x08);
    if (~data & 0x10) { // start fading
      timer_adjust(sndlocals.timer, 0.02, 0, 0.02);
      timer_on = 1;
    } else { // no fading used
      timer_adjust(sndlocals.timer, TIME_NEVER, 0, 0);
      timer_on = 0;
      mixer_set_volume(0, sndlocals.volume);
    }
  } else if (!timer_on) { // no fading going on, so stop sound
    discrete_sound_w(8, 0);
    discrete_sound_w(4, 0);
    discrete_sound_w(2, 0);
    discrete_sound_w(1, 0);
  }
}

static int play2s_start(const struct MachineSound *msound) {
  if (Machine->gamedrv->flags & GAME_NO_SOUND) {
    return -1;
  }
  sndlocals.channel = mixer_allocate_channel(15);
  return 0;
}

static void play2s_stop(void) {
}

static struct CustomSound_interface play2s_custInt = {play2s_start, play2s_stop};

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

static void play2s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
}

static WRITE_HANDLER(play2s_data_w) {
  if (mixer_is_sample_playing(sndlocals.channel)) {
    mixer_set_sample_frequency(sndlocals.channel, 2950000.0 / 4 / (data + 1));
  } else {
    mixer_play_sample(sndlocals.channel, (signed char *)squareWave, sizeof(squareWave), 2950000.0 / 4 / (data + 1), 1);
  }
}

static WRITE_HANDLER(play2s_ctrl_w) {
  sndlocals.enSn = data & 1;
  if (sndlocals.enSn) {
    sndlocals.volume = 150;
  } else if (sndlocals.volume) {
    sndlocals.volume--;
  }
  mixer_set_volume(0, sndlocals.volume * 2 / 3);
}

static WRITE_HANDLER(play2s_man_w) {
  play2s_ctrl_w(0, 1);
  play2s_data_w(0, data);
}

static void play3s_timer_callback(int n) {
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
  cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, ASSERT_LINE);
}

static void play3s_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.timer = timer_alloc(play3s_timer_callback);
  timer_adjust(sndlocals.timer, TIME_IN_HZ(3496), 0, TIME_IN_HZ(3496)); // ought to be correct according to 1863 datasheet
}

static WRITE_HANDLER(play3s_data_w) {
  logerror("snd data: %02x\n", data);
  sndlocals.sndCmd = data;
}

static WRITE_HANDLER(play3s_ctrl_w) {
  logerror("snd en: %x\n", data);
  sndlocals.enSn = data & 1;
}

static WRITE_HANDLER(play3s_man_w) {
  play3s_data_w(0, data);
  play3s_ctrl_w(0, 0);
}

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf play1sIntf = {
  "PLAY1", play1s_init, NULL, NULL, play1s_data_w, play1s_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play2sIntf = {
  "PLAY2", play2s_init, NULL, NULL, play2s_man_w, play2s_data_w, NULL, play2s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play3sIntf = {
  "PLAY3", play3s_init, NULL, NULL, play3s_man_w, play3s_data_w, NULL, play3s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf play4sIntf = {
  "PLAY4", play3s_init, NULL, NULL, play3s_man_w, play3s_data_w, NULL, play3s_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

struct AY8910interface play_ay8910_1 = {
	1,			/* 1 chip */
	2950000.0/2,	/* 1.475 MHz */
	{ 30 }	/* Volume */
};

static WRITE_HANDLER(ay8910_0_porta_w)	{
  if (data & 0x80) { // mute all
    AY8910_set_volume(0, 0, 0);
    AY8910_set_volume(0, 1, 0);
    AY8910_set_volume(0, 2, 0);
  } else {
    // TODO bits 1 to 6 control a slight reverb effect!?
    AY8910_set_volume(0, 0, data & 0x40 ? 60 : 100);
    AY8910_set_volume(0, 1, data & 0x40 ? 60 : 100);
    AY8910_set_volume(0, 2, data & 0x40 ? 60 : 100);
  }
}
static WRITE_HANDLER(ay8910_1_porta_w)	{
  if (data & 0x80) { // mute all
    AY8910_set_volume(1, 0, 0);
    AY8910_set_volume(1, 1, 0);
    AY8910_set_volume(1, 2, 0);
  } else {
    // TODO bits 1 to 6 control a slight reverb effect!?
    AY8910_set_volume(1, 0, data & 0x40 ? 60 : 100);
    AY8910_set_volume(1, 1, data & 0x40 ? 60 : 100);
    AY8910_set_volume(1, 2, data & 0x40 ? 60 : 100);
  }
}
struct AY8910interface play_ay8910_2 = {
	2,			/* 2 chips */
	3579545.0/2,	/* 1.79 MHz */
	{ MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) },	/* Volume */
	{ 0, 0 },
	{ 0, 0 },
	{ ay8910_0_porta_w, ay8910_1_porta_w }
};

static WRITE_HANDLER(ay0_w) {
	AY8910Write(0,offset % 2,data);
}

static WRITE_HANDLER(ay1_w) {
	AY8910Write(1,offset % 2,data);
}

static READ_HANDLER(in_snd) {
  sndlocals.enSn = 1;
  return sndlocals.sndCmd;
}

static WRITE_HANDLER(clk_snd) {
  logerror("snd clk: %02x\n", data);
  timer_adjust(sndlocals.timer, TIME_IN_HZ((3579545 >> 7) / (data + 1)), 0, TIME_IN_HZ((3579545 >> 7) / (data + 1))); // too fast? but sounds right!
}

static MEMORY_READ_START(playsound_readmem3)
  {0x0000,0x1fff, MRA_ROM},
  {0x2000,0x20ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writemem3)
  {0x2000,0x20ff, MWA_RAM},
MEMORY_END

static MEMORY_READ_START(playsound_readmem4)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x5fff, AY8910_read_port_0_r},
  {0x6000,0x7fff, AY8910_read_port_1_r},
  {0x8000,0x80ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writemem4)
  {0x4000,0x5fff, ay0_w},
  {0x6000,0x7fff, ay1_w},
  {0x8000,0x80ff, MWA_RAM},
MEMORY_END

static PORT_READ_START(playsound_readport)
  {0x02, 0x02, in_snd},
MEMORY_END

static PORT_WRITE_START(playsound_writeport)
  {0x01, 0x01, clk_snd},
MEMORY_END

static UINT8 snd_mode(void) { return CDP1802_MODE_RUN; }

static UINT8 snd_ef(void) {
  return sndlocals.enSn;
}

static void snd_sc(int data) {
  if (data & 2) {
    cpu_set_irq_line(PLAYMATIC_SCPU, CDP1802_INPUT_LINE_INT, CLEAR_LINE);
    timer_enable(sndlocals.timer, 0);
  } else {
    timer_enable(sndlocals.timer, 1);
  }
}

static CDP1802_CONFIG playsound_config =
{
	snd_mode,	// MODE
	snd_ef,		// EF
	snd_sc,		// SC
	NULL,		// Q
	NULL,		// DMA read
	NULL		// DMA write
};

static DISCRETE_SOUND_START(play_tones)
	DISCRETE_INPUT(NODE_01,1,0x000f,0)                         // Input handlers, mostly for enable
	DISCRETE_INPUT(NODE_02,2,0x000f,0)
	DISCRETE_INPUT(NODE_04,4,0x000f,0)
	DISCRETE_INPUT(NODE_08,8,0x000f,0)

	DISCRETE_TRIANGLEWAVE(NODE_10,NODE_01,523,50000,10000,0) // C' note
	DISCRETE_TRIANGLEWAVE(NODE_20,NODE_02,659,50000,10000,0) // E' note
	DISCRETE_TRIANGLEWAVE(NODE_30,NODE_04,784,50000,10000,0) // G' note
	DISCRETE_TRIANGLEWAVE(NODE_40,NODE_08,988,50000,10000,0) // H' note

	DISCRETE_ADDER4(NODE_50,1,NODE_10,NODE_20,NODE_30,NODE_40) // Mix all four sound sources

	DISCRETE_OUTPUT(NODE_50, 75)                               // Take the output from the mixer
DISCRETE_SOUND_END

MACHINE_DRIVER_START(PLAYMATICS1)
  MDRV_SOUND_ADD(DISCRETE, play_tones)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(PLAYMATICS2)
  MDRV_SOUND_ADD(CUSTOM, play2s_custInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(PLAYMATICS3)
  MDRV_CPU_ADD_TAG("scpu", CDP1802, 2950000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_CONFIG(playsound_config)
  MDRV_CPU_MEMORY(playsound_readmem3, playsound_writemem3)
  MDRV_CPU_PORTS(playsound_readport, playsound_writeport)
  MDRV_SOUND_ADD(AY8910, play_ay8910_1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(PLAYMATICS4)
  MDRV_CPU_ADD_TAG("scpu", CDP1802, 3579545)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_CONFIG(playsound_config)
  MDRV_CPU_MEMORY(playsound_readmem4, playsound_writemem4)
  MDRV_CPU_PORTS(playsound_readport, playsound_writeport)
  MDRV_SOUND_ADD(AY8910, play_ay8910_2)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END
