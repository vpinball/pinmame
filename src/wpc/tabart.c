#include "driver.h"
#include "core.h"
#include "sndbrd.h"

/* Sound section for C. Tabart games */

static struct {
  int nmi, outhole, swStrobe;
  UINT8 sndCmd, manCmd;
  int subtype;
} sndlocals;

static void tabart_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  memset(memory_region(REGION_CPU2) + 0x4000, 0xff, 0x80); // sahalove needs a clean RAM for sound 05 to work correctly
  sndlocals.subtype = brdData->subType;
}

static WRITE_HANDLER(tabart_manCmd_w) {
  static int toggle;

  if (sndlocals.subtype) {
    sndlocals.sndCmd = data;
    return;
  }

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
  if (sndlocals.subtype || !data) return;

  sndlocals.swStrobe = data;
  sndlocals.outhole = core_getSw(66) ? 1 : 0;
  sndlocals.nmi = data == 1;
  cpu_set_nmi_line(1, sndlocals.nmi ? ASSERT_LINE : CLEAR_LINE);
}

// GTS1 snd lines order: Dip2, Q, (NC), Dip1, T, Snd3, Snd2, Snd1
static WRITE_HANDLER(tabart_data_w) {
  if (sndlocals.subtype & 1) { // Sahara Love: Q (and / or Dip4?), Dip3, Dip2, Dip1, T, Snd3, Snd2, Snd1 ???
    sndlocals.sndCmd = ((data & 0x40 ? 0 : 0x80) | (~data & 0x0f)) ^ ((~core_getDip(3) & 0x0f) << 4);
    return;
  }

  if (sndlocals.subtype & 2) { // Le Grand 8: Q (and / or Dip4?), Dip3, Dip2, Dip1, T, Snd3, Snd2, Snd1 ???
    sndlocals.sndCmd = ((data & 0x40 ? 0 : 0x80) | (~data & 0x0f)) ^ ((~core_getDip(4) & 0x0f) << 4);
    return;
  }

  sndlocals.sndCmd = data ^ 0xc7;
  sndlocals.outhole = core_getSw(66) ? 1 : 0;
}

const struct sndbrdIntf tabartIntf = {
  "TABART", tabart_init, NULL, NULL, tabart_manCmd_w, tabart_data_w, NULL, tabart_ctrl_w
};

// hexagone cmd order: NMI, Dip2, Dip1, Outhole, Q, Snd3, Snd2, Snd1
static READ_HANDLER(ym2203_port_a_r) {
  UINT8 cmd = (sndlocals.nmi ? 0 : 0x80) | (sndlocals.sndCmd & 0x80 ? 0x40 : 0) | (sndlocals.sndCmd & 0x10 ? 0x20 : 0)
    | (sndlocals.outhole ? 0 : 0x10) | (sndlocals.sndCmd & 0x40 ? 0x08 : 0) | (sndlocals.sndCmd & 0x07);
  return cmd;
}

static READ_HANDLER(ym2203_port_b_r) {
  return sndlocals.swStrobe ? coreGlobals.swMatrix[sndlocals.swStrobe] : ~sndlocals.manCmd;
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


// Sahara Love / Le Grand 8 sound board

static READ_HANDLER(ay8912a_r) {
  sndlocals.nmi = 0;
  return sndlocals.sndCmd;
}

static WRITE_HANDLER(ay8912a_w) {
  logerror("AY W %02x\n", data);
}

static struct AY8910interface tabart_ay8912Int = {
  1,
  19660800./16.,
  { 33 },
  { ay8912a_r }, { NULL },
  { ay8912a_w }
};

static MEMORY_READ_START(tabart2_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x407f, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(tabart2_writemem)
  {0x4000,0x407f, MWA_RAM},
  {0x8000,0x8000, MWA_NOP}, // watchdog: resets CPU if binary counters are not working
MEMORY_END

static PORT_READ_START(tabart2_readport)
  {4,4, AY8910_read_port_0_r},
PORT_END

static PORT_WRITE_START(tabart2_writeport)
  {0,0, AY8910_control_port_0_w},
  {1,1, AY8910_write_port_0_w},
PORT_END

static INTERRUPT_GEN(tabart2_irq) {
  cpu_set_nmi_line(1, PULSE_LINE);
  cpu_set_irq_line(1, 0, PULSE_LINE); // not actually needed because NMI wins, but still connected
}

MACHINE_DRIVER_START(TABART2)
  MDRV_CPU_ADD_TAG("scpu", Z80, 19660800./8.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(tabart2_readmem, tabart2_writemem)
  MDRV_CPU_PORTS(tabart2_readport, tabart2_writeport)
  MDRV_CPU_PERIODIC_INT(tabart2_irq, 19660800./131072.) // 19660800/4/16/16/16/8 = 150 Hz
  MDRV_SOUND_ADD(AY8910, tabart_ay8912Int)
MACHINE_DRIVER_END
