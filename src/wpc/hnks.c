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

  int   channel;
  INT8  wavetable[16][32];
} hnks_locals;

#define SP_PIA0  2

MEMORY_READ_START(hnks_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x1000, 0x17ff, MRA_ROM },
  { 0xf000, 0xf200, MRA_ROM },
  { 0xf800, 0xffff, MRA_ROM }, /* reset vector */
MEMORY_END

MEMORY_WRITE_START(hnks_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_w(SP_PIA0) },
  { 0x1000, 0x17ff, MWA_ROM },
  { 0xf000, 0xf200, MWA_ROM },
  { 0xf800, 0xffff, MWA_ROM }, /* reset vector */
MEMORY_END

/*
  PA4-PA7 : output: select waveform (0-15, upper bits of wave table ROM)
*/
static WRITE_HANDLER(pia0a_w)
{
  mixer_play_sample(hnks_locals.channel, (char*) &hnks_locals.wavetable[(data&0xf0)>>4], 32, 1000, 1);
}

/*
  PB0-PB3 : output: replay speed
  PB4-PB7 : output: set volume
*/
static WRITE_HANDLER(pia0b_w)
{
  // pitch is not supported yet
  // logerror("speed : %02x\n", data&0x0f);
  
  mixer_set_volume(hnks_locals.channel, ((data&0xf0)>>4)*0x10);
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
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, 0, 0,
  /*irq: A/B           */ 0,0
};

static WRITE_HANDLER(hnks_data_w)
{
    pia_set_input_a(SP_PIA0, data&0x0f);
    cpu_set_irq_line(hnks_locals.brdData.cpuNo, M6802_IRQ_LINE, PULSE_LINE);
}

static void hnks_init(struct sndbrdData *brdData)
{
  int i,j;
  memset(&hnks_locals, 0x00, sizeof(hnks_locals));
  hnks_locals.brdData = *brdData;

  /* load the "wave table" (from the ROM, it's mapped to 0xf000-0xf1ff */
  for (i=0; i<16; i++) {
	  unsigned char* adr = memory_region(HNK_MEMREG_SCPU) + 0xf000 + (i*32);
	  for(j=0; j<32; j++)
		hnks_locals.wavetable[i][j] = (*adr++)<<4;
  }

  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
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
  hnks_init, NULL, NULL, hnks_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

struct CustomSound_interface hnks_custInt = {hnks_sh_start, hnks_sh_stop};

