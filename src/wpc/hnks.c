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
/-----------------------------------------*/

static struct {
  struct sndbrdData brdData;
  int sndCmd;
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

static WRITE_HANDLER(pia0a_w)
{
}

static WRITE_HANDLER(pia0b_w)
{
}

static void pia0_irq(int state)
{
  cpu_set_irq_line(hnks_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
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
  /*irq: A/B           */ pia0_irq, pia0_irq
};

static WRITE_HANDLER(hnks_data_w)
{
	hnks_locals.sndCmd = data & 0x0f;
}

static void hnks_init(struct sndbrdData *brdData) {
  logerror("sound init\n");
  memset(&hnks_locals, 0x00, sizeof(hnks_locals));
  hnks_locals.brdData = *brdData;

  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
}

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf hankinIntf = {
  hnks_init, NULL, NULL, hnks_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

