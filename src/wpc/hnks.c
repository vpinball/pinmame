#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "by35.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "hnks.h"

/*----------------------------------------
/ Hankin Sound System
/ cpu: 6802
/
/ 0x0000 - 0x007F: RAM
/ 0x0080 - 0x0083: PIA I/O
/ 0x1000 - 0x17FF: ROM
/
/ (the wavetable ROM is mapped to f000-f1ff
/  and the main rom is repeated at f800-ffff
/  for the interupt vectors)
/-----------------------------------------*/
#define BASE_FREQUENCY 16000
/*
  - can't say if the waveform is correct
    (the waveform is build by a comparator chip and I'm not sure how it really
	works)
  - base frequency for the counter is set to 16000 Hz, not sure if this is ok
*/
#define SP_PIA0  2

static struct {
  struct sndbrdData brdData;
  int    enabled, reset;
  int    volume, freq;
  INT8   buffer[32*16];
  int   channel;
} locals;
static UINT8 *waveRom;

static MEMORY_READ_START(hnks_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SP_PIA0) },
  { 0x1000, 0x17ff, MRA_ROM },
  { 0xf000, 0xf1ff, MRA_ROM },
  { 0xf800, 0xffff, MRA_ROM }, /* reset vector */
MEMORY_END

static MEMORY_WRITE_START(hnks_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SP_PIA0) },
  { 0x1000, 0x17ff, MWA_ROM },
  { 0xf000, 0xf1ff, MWA_ROM, &waveRom },
  { 0xf800, 0xffff, MWA_ROM }, /* reset vector */
MEMORY_END

static void hnks_irq(int state) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*
  PA4-PA7 : output: select waveform (0-15, upper 4 bits of wave table ROM IC3)
*/
static WRITE_HANDLER(pia0a_w) {
  mixer_play_sample(locals.channel, &locals.buffer[((UINT16)(data & 0xf0))<<1], 32, locals.freq,1);
}

/*
  PB0-PB3 : output: counterSpeed
  PB4-PB7 : output: set volume
*/
static WRITE_HANDLER(pia0b_w) {
  locals.freq = BASE_FREQUENCY/((data & 0x0f)+1);
  locals.volume = ((UINT16)(data & 0xf0)) * 100 / 0xf0;
  mixer_set_sample_frequency(locals.channel, locals.freq);
  if (locals.enabled && !locals.reset) mixer_set_volume(locals.channel, locals.volume);
}

/* enables the counter to the wave table */
static WRITE_HANDLER(pia0ca2_w) {
  locals.enabled = data;
  if (locals.enabled && !locals.reset) mixer_set_volume(locals.channel, locals.volume);
}

/* resets the counter to the wave table
/ (counter will stay zero as long as this signal is high) */
static WRITE_HANDLER(pia0cb2_w) {
  locals.reset = data;
  if (locals.enabled && !locals.reset) mixer_set_volume(locals.channel, locals.volume);
}

/*
  CA1     : input: sound strobe
  PA0-PA3 : input: sound command
  PA4-PA7 : output: select waveform (0-15, upper adress bits of wave ROM)
  CA2     : output: timer reset/enable
  PB0-PB3 : output: timer divider value
  PB4-PB7 : output: set volume level
*/

static const struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ hnks_irq,hnks_irq
};

/* sound data */
static WRITE_HANDLER(hnks_data_w) { pia_set_input_a(SP_PIA0, data&0x0f); }

/* sound strobe */
static WRITE_HANDLER(hnks_ctrl_w) { pia_set_input_ca1(SP_PIA0, data); }

static WRITE_HANDLER(hnks_manCmd_w) {
  hnks_data_w(0, data); hnks_ctrl_w(0,1); hnks_ctrl_w(0,0);
}
static void hnks_init(struct sndbrdData *brdData) {
  int ii;
  memset(&locals, 0x0, sizeof(locals));
  locals.brdData = *brdData;

  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
  for (ii = 0; ii < 16*32; ii++) // convert waverom to signed values
    locals.buffer[ii] = ((waveRom[ii]<<4)-0x80);
}

static int hnks_sh_start(const struct MachineSound *msound) {
  locals.channel = mixer_allocate_channel(15);
  mixer_set_volume(locals.channel,0);
  return 0;
}

static void hnks_sh_stop(void) {
  mixer_stop_sample(locals.channel);
}

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf hankinIntf = {
  "HNK", hnks_init, NULL, NULL, hnks_manCmd_w, hnks_data_w, NULL, hnks_ctrl_w, NULL
};

static struct CustomSound_interface hnks_custInt = {hnks_sh_start, hnks_sh_stop};

MACHINE_DRIVER_START(hnks)
  MDRV_CPU_ADD_TAG("scpu", M6802, 900000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(hnks_readmem, hnks_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, hnks_custInt)
MACHINE_DRIVER_END
