#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "hnk.h"
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
   and the main rom is repeated at f800-ffff
   for the interupt vector)
/-----------------------------------------*/

/*
  - pitch handling isn't implemented
  - can't say if the waveform is correct
  - base pitch is set to 1000Hz, not sure if this is ok
  - counter reset/hold not implemented
*/

static struct {
  struct sndbrdData brdData;
  int sndCmd;

  int   soundEnabled1;
  int   soundEnabled2;
  int   actSamples;
  int   pitch;
  int   channel;
  INT8  wavetable[16][32];
} hnks_locals;

#define SP_PIA0  2

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
  cpu_set_irq_line(hnks_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

void start_samples(void) {
//  if ( hnks_locals.soundEnabled1 && hnks_locals.soundEnabled2 )
    mixer_play_sample(hnks_locals.channel, (INT8 *) &hnks_locals.wavetable[hnks_locals.actSamples], 32, (0x4000/(hnks_locals.pitch+1)), 1);
//  else
//	mixer_stop_sample(hnks_locals.channel);
}

/*
  PA4-PA7 : output: select waveform (0-15, upper 4 bits of wave table ROM IC3)
*/
static WRITE_HANDLER(pia0a_w)
{
  hnks_locals.actSamples = (data&0xf0)>>4;
  start_samples();
}

/*
  PB0-PB3 : output: replay pitch
  PB4-PB7 : output: set volume
*/
static WRITE_HANDLER(pia0b_w)
{
  hnks_locals.pitch = data&0x0f;
  
  //logerror("pitch : %02x\n", hnks_locals.pitch);
  // logerror("volume: %02x\n", (data&0xf0)>>4);

  mixer_set_volume(hnks_locals.channel, data&0xf0);
}

static WRITE_HANDLER(pia0ca2_w)
{
  logerror("pia0ca2_w: %02x\n", data);
  hnks_locals.soundEnabled1 = data;
  start_samples();
}

static WRITE_HANDLER(pia0cb2_w)
{
  logerror("pia0cb2_w: %02x\n", data);
  hnks_locals.soundEnabled2 = data;
  start_samples();
}

/*
  CA1     : input: sound strobe
  PA0-PA3 : input: sound command
  PA4-PA7 : output: select waveform (0-15, upper bits of wave from ROM)
  CA2     : output: start/enabled timer ???
  PB0-PB3 : output: timer startup value ???
  PB4-PB7 : output: set volume level
*/

static const struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ hnks_irq,hnks_irq
};

static WRITE_HANDLER(hnks_data_w)
{
    pia_set_input_a(SP_PIA0, data&0x0f);
}

static WRITE_HANDLER(hnks_ctrl_w)
{
	pia_set_input_ca1(SP_PIA0, data);
}

static void hnks_init(struct sndbrdData *brdData)
{
  int i,j;
  memset(&hnks_locals, 0x00, sizeof(hnks_locals));
  hnks_locals.brdData = *brdData;

  /* load the "wave table" (from the ROM, it's mapped to 0xf000-0xf1ff */
  for (i=0; i<=0x0f; i++) {
	  unsigned char* adr = memory_region(HNK_MEMREG_SCPU) + 0xf000 + (i*0x20);
	  for(j=0; j<32; j++)
		hnks_locals.wavetable[i][j] = ((*adr++)<<4)-0x80;
  }

  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);

  hnks_locals.soundEnabled1 = 1;

}

static int hnks_sh_start(const struct MachineSound *msound) {
  hnks_locals.channel = mixer_allocate_channel(15);
  mixer_set_volume(hnks_locals.channel,0x00);
  return 0;
}

static void hnks_sh_stop(void) {
  mixer_stop_sample(hnks_locals.channel);
}

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf hankinIntf = {
  hnks_init, NULL, NULL, hnks_data_w, NULL, hnks_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

struct CustomSound_interface hnks_custInt = {hnks_sh_start, hnks_sh_stop};

