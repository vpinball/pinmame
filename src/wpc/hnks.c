#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "hnks.h"

#define HNK_MEMREG_SCPU	REGION_CPU2
/*----------------------------------------
/ Hankin Sound System
/ cpu: 6802
/ 
/ 0x0000 - 0x007F: RAM
/ 0x0080 - 0x0083: PIA I/O
/ 0x1000 - 0x17FF: ROM
/ 
/ (the wavetable ROM is mapped to f000-f1ff 
   and the main rom is repeated at f800-ffff
   for the interupt vectors)
/-----------------------------------------*/

#define BASE_FREQUENCY 16000
/*
  - can't say if the waveform is correct
    (the waveform is build by a comparator chip and I'm not sure how it really 
	works)
  - base frequency for the counter is set to 16000 Hz, not sure if this is ok
*/

#define SP_PIA0	2

static struct {
  struct sndbrdData brdData;
  int sndCmd;

  int     div2;
  int     counterSpeed;
  int     counterReset;
  int     actSamples;
  double  volume;

  // some cached values
  int pia0a_r;
  int pia0b_w;

  int   channel;
} hnks_locals;

MEMORY_READ_START(hnks_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SP_PIA0) },
  { 0x1000, 0x17ff, MRA_ROM },
  { 0xf800, 0xffff, MRA_ROM }, /* reset vector */
MEMORY_END

MEMORY_WRITE_START(hnks_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SP_PIA0) },
  { 0x1000, 0x17ff, MWA_ROM },
  { 0xf800, 0xffff, MWA_ROM }, /* reset vector */
MEMORY_END

static void hnks_irq(int state) {
  // logerror("irq\r\n");
  cpu_set_irq_line(hnks_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INT8 samples[2][32];
static int iBuffer = 0;

void start_samples(void)
{
	int i;

	unsigned char* adr = memory_region(HNK_MEMREG_SCPU) + 0xf000 + (hnks_locals.actSamples*0x20);

	iBuffer = 1-iBuffer;
	for (i=0; i<0x20; i++)
		samples[iBuffer][i] = (((*adr++)<<4)-0x80) * hnks_locals.volume; 

	// looks strange? (cond?32:1), thats ok!
	mixer_play_sample(
		hnks_locals.channel, 
		samples[iBuffer],
		!hnks_locals.counterReset?0x20:1, 
		BASE_FREQUENCY/(hnks_locals.counterSpeed/((1-hnks_locals.div2)+1)+1),
		1
	);
}

/*
  PA4-PA7 : output: select waveform (0-15, upper 4 bits of wave table ROM IC3)
*/
static WRITE_HANDLER(pia0a_w)
{
  int samp = (data&0xf0)>>4;
  // logerror("pia0a_w: %02x\n", data);

  if ( samp == hnks_locals.actSamples )
	  return;

  hnks_locals.actSamples = samp;
  start_samples();
}

/*
  PB0-PB3 : output: counterSpeed
  PB4-PB7 : output: set volume
*/
static WRITE_HANDLER(pia0b_w)
{
  // logerror("pia0b_w: %02x\n", data);
  if ( hnks_locals.pia0b_w==data )
	  return;
  hnks_locals.pia0b_w = data;

  hnks_locals.counterSpeed = data&0x0f;
  hnks_locals.volume = ((data&0xf0)>>4) / 15.0;
  
  // logerror("counter speed : %02x\n", hnks_locals.counterSpeed);
  // logerror("volume: %f\n", hnks_locals.volume);
  start_samples();
}

/* enables the counter to the wave table */
static WRITE_HANDLER(pia0ca2_w)
{
  // logerror("pia0ca2_w: %02x\n", data);
  if ( hnks_locals.div2==data )
	  return;

  hnks_locals.div2 = data;
  start_samples();
}

/* resets the counter to the wave table 
/ (counter will stay zero as long as this signal is high) */
static WRITE_HANDLER(pia0cb2_w)
{
  // logerror("pia0cb2_w: %02x\n", data);
  if ( hnks_locals.counterReset==data )
	  return;

  hnks_locals.counterReset = data;
  start_samples();
}

static READ_HANDLER(pia0a_r)
{
  // logerror("pia0_r: %02x\n", data);
  return hnks_locals.pia0a_r;
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
  /*i: A/B,CA/B1,CA/B2 */ pia0a_r, 0, PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0), 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ hnks_irq,hnks_irq
};

/* sound data */
static WRITE_HANDLER(hnks_data_w)
{
  // logerror("hnks_data_w: 0x%02x\r\n", data);
  hnks_locals.pia0a_r = data&0x0f;
}

/* sound strobe */
static WRITE_HANDLER(hnks_ctrl_w)
{
  // logerror("hnks_ctrl_w: 0x%02d\r\n", data);
  pia_set_input_ca1(SP_PIA0, data);
}

static WRITE_HANDLER(hnks_manCmd_w) {
  hnks_ctrl_w(0,0); hnks_data_w(0, data); hnks_ctrl_w(0,1);
}

static void hnks_init(struct sndbrdData *brdData)
{
  memset(&hnks_locals, 0x00, sizeof(hnks_locals));
  hnks_locals.brdData = *brdData;

  hnks_locals.pia0b_w = 0xff;

  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
}

static int hnks_sh_start(const struct MachineSound *msound) {
  hnks_locals.channel = mixer_allocate_channel(15);
  mixer_set_volume(hnks_locals.channel,0xff);
  return 0;
}

static void hnks_sh_stop(void) {
  mixer_stop_sample(hnks_locals.channel);
}

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf hankinIntf = {
  "HNK", hnks_init, NULL, NULL, hnks_manCmd_w, hnks_data_w, NULL, hnks_ctrl_w, NULL
};

struct CustomSound_interface hnks_custInt = {hnks_sh_start, hnks_sh_stop};

MACHINE_DRIVER_START(hnks)
  MDRV_CPU_ADD_TAG("scpu", M6802, 900000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(hnks_readmem, hnks_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(CUSTOM, hnks_custInt)
MACHINE_DRIVER_END
