#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "zacsnd.h"

/*----------------------------------------
/  Zaccaria Sound Board (similar to BY61, but 2x 68xx CPUs)
/-----------------------------------------*/
/*
/  RAM ???
/  ROM C-H 8000-ffff
/  ROM 4-6 0000-ffff
*/
#define ZAC_PIA0 0
#define ZAC_PIA1 1

static void zac_init(struct sndbrdData *brdData);
static void zac_diag(int button);
static WRITE_HANDLER(zac_data_w);
static WRITE_HANDLER(zac_ctrl_w);
static READ_HANDLER(zac_8910a_r);

const struct sndbrdIntf zacIntf = {
  zac_init, NULL, zac_diag, zac_data_w, NULL, zac_ctrl_w, NULL, 0 //SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct DACinterface     zac_dacInt = { 1, { 20 }};
static struct AY8910interface  zac_ay8910Int = { 1, 3580000/4, {25}, {zac_8910a_r}};

static MEMORY_READ_START(zac_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(ZAC_PIA0) },
  { 0x0090, 0x0093, pia_r(ZAC_PIA1) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(zac_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(ZAC_PIA0) },
  { 0x0090, 0x0093, pia_w(ZAC_PIA1) },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START(zac_readmem2)
  { 0x0000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(zac_writemem2)
  { 0x0000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(zac)
  MDRV_CPU_ADD_TAG("snd1", M6800, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zac_readmem, zac_writemem)

  MDRV_CPU_ADD_TAG("snd2", M6800, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zac_readmem2, zac_writemem2)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(DAC,     zac_dacInt)
  MDRV_SOUND_ADD(AY8910,  zac_ay8910Int)
MACHINE_DRIVER_END

static READ_HANDLER(zac_pia0a_r);
static WRITE_HANDLER(zac_pia0a_w);
static WRITE_HANDLER(zac_pia0b_w);
static WRITE_HANDLER(zac_pia1a_w);
static WRITE_HANDLER(zac_pia1b_w);
static WRITE_HANDLER(zac_pia0ca2_w);
static void zac_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b;
  int cmd[2], lastcmd, cmdin, cmdout, lastctrl;
} locals;

static const struct pia6821_interface zac_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ zac_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ zac_pia0a_w, zac_pia0b_w, zac_pia0ca2_w, 0,
  /*irq: A/B           */ zac_irq, zac_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ zac_pia1a_w, zac_pia1b_w, 0, 0,
  /*irq: A/B           */ zac_irq, zac_irq
}};
static void zac_init(struct sndbrdData *brdData) {
  locals.brdData = *brdData;
  pia_config(ZAC_PIA0, PIA_STANDARD_ORDERING, &zac_pia[0]);
  pia_config(ZAC_PIA1, PIA_STANDARD_ORDERING, &zac_pia[1]);
  locals.cmdin = locals.cmdout = 2;
}
static void zac_diag(int button) {
  cpu_set_nmi_line(locals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}
static READ_HANDLER(zac_pia0a_r) {
  if (locals.brdData.subType == 1)   return zac_8910a_r(0); // B type
  if ((locals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(zac_pia0a_w) {
  locals.pia0a = data;
  if (locals.pia0b & 0x02) AY8910Write(0, locals.pia0b ^ 0x01, locals.pia0a);
}
static WRITE_HANDLER(zac_pia0b_w) {
  locals.pia0b = data;
  if (locals.pia0b & 0x02) AY8910Write(0, locals.pia0b ^ 0x01, locals.pia0a);
}
static WRITE_HANDLER(zac_pia1a_w) { locals.pia1a = data; }
static WRITE_HANDLER(zac_pia1b_w) {
  if (locals.pia1b & ~data & 0x02) { // write
    pia_pulse_ca2(ZAC_PIA1, 1);
  }
  else if (locals.pia1b & ~data & 0x01) { // read
    pia_pulse_ca2(ZAC_PIA1, 1);
  }
  locals.pia1b = data;
}

static WRITE_HANDLER(zac_data_w) {
  locals.lastcmd = (locals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(zac_ctrl_w) {
  locals.lastcmd = (locals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(ZAC_PIA0, ~data & 0x01);
}
static READ_HANDLER(zac_8910a_r) { return ~locals.lastcmd; }

static WRITE_HANDLER(zac_pia0ca2_w) { sndbrd_ctrl_cb(locals.brdData.boardNo,data); } // diag led

static void zac_irq(int state) {
  cpu_set_irq_line(locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
