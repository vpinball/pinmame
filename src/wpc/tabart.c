#include "driver.h"
#include "core.h"
#include "sndbrd.h"

/* Sound section for C. Tabart games */

static struct {
  int nmi, outhole, swStrobe;
  UINT8 sndCmd, manCmd;
} sndlocals;

static void tabart_init(struct sndbrdData *brdData) {
	memset(&sndlocals, 0, sizeof sndlocals);
}

static WRITE_HANDLER(tabart_manCmd_w) {
  static int toggle;
  sndlocals.swStrobe = 0;
  if (!toggle)
    sndlocals.manCmd = data;
  else {
    sndlocals.sndCmd = data;
    sndlocals.outhole = (data & 0x20) ? 1 : 0;
    sndlocals.nmi = 1;
    cpu_set_nmi_line(1, PULSE_LINE);
  }
  toggle = !toggle;
}

// This is needed to determine the correct switch row
static WRITE_HANDLER(tabart_ctrl_w) {
  sndlocals.swStrobe = data;
  sndlocals.outhole = core_getSw(66) ? 1 : 0;
  if (sndlocals.nmi != (data == 1)) {
    sndlocals.nmi = (data == 1);
    cpu_set_nmi_line(1, sndlocals.nmi ? ASSERT_LINE : CLEAR_LINE);
  }
}

// GTS1 snd lines order: Dip2, Q, (NC), Dip1, T, Snd3, Snd2, Snd1
static WRITE_HANDLER(tabart_data_w) {
  sndlocals.sndCmd = data ^ 0x87;
  sndlocals.outhole = core_getSw(66) ? 1 : 0;
}

const struct sndbrdIntf tabartIntf = {
  "TABART", tabart_init, NULL, NULL, tabart_manCmd_w, tabart_data_w, NULL, tabart_ctrl_w
};

// Tabart cmd order: NMI, Dip2, Dip1, T+Outhole, Q, Snd3, Snd2, Snd1
static READ_HANDLER(ym2203_port_a_r) {
  UINT8 cmd = (sndlocals.nmi ? 0 : 0x80) | (sndlocals.outhole ? 0 : 0x10) | ((sndlocals.sndCmd & 0x80) >> 1)
    | ((sndlocals.sndCmd & 0x40) >> 3) | ((sndlocals.sndCmd & 0x18) << 1) | (sndlocals.sndCmd & 0x07);
  sndlocals.sndCmd |= 0x4f;  // pull up lines for the next read
  return cmd;
}

static READ_HANDLER(ym2203_port_b_r) {
  return sndlocals.swStrobe ? ~coreGlobals.swMatrix[sndlocals.swStrobe] : ~sndlocals.manCmd;
}

static void tabart_irq(int state) {
	cpu_set_irq_line(1, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2203interface tabart_ym2203Int = {
	1,
	3579545,
	{ 50 | (30 << 16) }, // uses high 16 bits for YM2203 FM volume!
	{ ym2203_port_a_r }, { ym2203_port_b_r },
	{ NULL }, { NULL },
	{ &tabart_irq }
};

static struct YM3526interface tabart_ym3526Int =  {
  1,
	3579545,
	{ 30 }
};

extern READ_HANDLER(YM3526_read_port_0_r);
MEMORY_READ_START(tabart1_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x407f, MRA_RAM},
  {0x5000,0x5000, YM2203_status_port_0_r},
  {0x5001,0x5001, YM2203_read_port_0_r},
  {0x6000,0x6000, YM3526_status_port_0_r},
  {0x6001,0x6001, YM3526_read_port_0_r},
MEMORY_END

MEMORY_WRITE_START(tabart1_writemem)
  {0x0000,0x3fff, MWA_NOP},
  {0x4000,0x407f, MWA_RAM},
  {0x5000,0x5000, YM2203_control_port_0_w},
  {0x5001,0x5001, YM2203_write_port_0_w},
  {0x6000,0x6000, YM3526_control_port_0_w},
  {0x6001,0x6001, YM3526_write_port_0_w},
MEMORY_END

MACHINE_DRIVER_START(TABART1)
  MDRV_CPU_ADD_TAG("scpu", Z80, 3579545) /* NTSC quartz */
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(tabart1_readmem, tabart1_writemem)
  MDRV_SOUND_ADD(YM2203, tabart_ym2203Int)
  MDRV_SOUND_ADD(YM3526, tabart_ym3526Int)
MACHINE_DRIVER_END
