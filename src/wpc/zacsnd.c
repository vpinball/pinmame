#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "zacsnd.h"

/*----------------------------------------
/ Zaccaria Sound Board 1346
/ (8085 & 6510 CPU, no PIA)
/-----------------------------------------*/
/*
/ CPU 6810
/     ???
/ CPU 8085
/     6 MHz
/ ROM 1000-18ff (f800-ffff)?
*/
static void sp_init(struct sndbrdData *brdData);
static WRITE_HANDLER(sp1346_data_w);
static WRITE_HANDLER(sp1346_ctrl_w);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1346Intf = {
  sp_init, NULL, NULL, sp1346_data_w, NULL, sp1346_ctrl_w, NULL,
};

static MEMORY_READ_START(i8085_readmem)
  { 0x0000, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(i8085_writemem)
  { 0x0000, 0x07ff, MWA_ROM },
MEMORY_END

static MEMORY_READ_START(m6510_readmem)
  { 0x1000, 0x17ff, MRA_ROM },
  { 0xf800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(m6510_writemem)
  { 0x1000, 0x17ff, MWA_ROM },
  { 0xf800, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(zac1346)
//  MDRV_CPU_ADD_TAG("scpu1", 8085A, 6000000)
//  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//  MDRV_CPU_MEMORY(i8085_readmem, i8085_writemem)

  MDRV_CPU_ADD_TAG("scpu2", M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(m6510_readmem, m6510_writemem)

  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

static void sp_irq(int state);

static struct {
  struct sndbrdData brdData;
  int lastcmd, cmdin, cmdout, cmd[2], lastctrl;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
  splocals.cmdin = splocals.cmdout = 2;
}

static WRITE_HANDLER(sp1346_data_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(sp1346_ctrl_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
}

static void sp_irq(int state) {
  cpu_set_irq_line(splocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*----------------------------------------
/ Zaccaria Sound & Speech Board 1370
/ (almost identical to BY61, TMS5200 chip)
/-----------------------------------------*/
#define SNS_PIA0 0
#define SNS_PIA1 1

static void sns_init(struct sndbrdData *brdData);
static void sns_diag(int button);
static WRITE_HANDLER(sns_data_w);
static WRITE_HANDLER(sns_ctrl_w);
static void sns_5220Irq(int state);
static READ_HANDLER(sns_8910a_r);

const struct sndbrdIntf zac1370Intf = {
  sns_init, NULL, sns_diag, sns_data_w, NULL, sns_ctrl_w, NULL, 0//SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface sns_tms5220Int = { 640000, 50, sns_5220Irq };
static struct DACinterface     sns_dacInt = { 1, { 20 }};
static struct AY8910interface  sns_ay8910Int = { 1, 3580000/4, {25}, {sns_8910a_r}};

static MEMORY_READ_START(sns_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNS_PIA0) },
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(zac1370)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns_readmem, sns_writemem)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     sns_dacInt)
  MDRV_SOUND_ADD(AY8910,  sns_ay8910Int)
MACHINE_DRIVER_END

static READ_HANDLER(sns_pia0a_r);
static WRITE_HANDLER(sns_pia0a_w);
static WRITE_HANDLER(sns_pia0b_w);
static WRITE_HANDLER(sns_pia1a_w);
static WRITE_HANDLER(sns_pia1b_w);
static WRITE_HANDLER(sns_pia0ca2_w);
static void sns_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b;
  int cmd[2], lastcmd, cmdin, cmdout, lastctrl;
} snslocals;

static const struct pia6821_interface sns_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia0a_w, sns_pia0b_w, sns_pia0ca2_w, 0,
  /*irq: A/B           */ sns_irq, sns_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia1a_w, sns_pia1b_w, 0, 0,
  /*irq: A/B           */ sns_irq, sns_irq
}};

static void sns_init(struct sndbrdData *brdData) {
  snslocals.brdData = *brdData;
  pia_config(SNS_PIA0, PIA_STANDARD_ORDERING, &sns_pia[0]);
  pia_config(SNS_PIA1, PIA_STANDARD_ORDERING, &sns_pia[1]);
  snslocals.cmdin = snslocals.cmdout = 2;
}

static void sns_diag(int button) {
  cpu_set_nmi_line(snslocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(sns_pia0a_r) {
  if ((snslocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(sns_pia0a_w) {
  snslocals.pia0a = data;
  if (snslocals.pia0b & 0x02) AY8910Write(0, snslocals.pia0b ^ 0x01, snslocals.pia0a);
}
static WRITE_HANDLER(sns_pia0b_w) {
  snslocals.pia0b = data;
  if (snslocals.pia0b & 0x02) AY8910Write(0, snslocals.pia0b ^ 0x01, snslocals.pia0a);
}
static WRITE_HANDLER(sns_pia1a_w) { snslocals.pia1a = data; }
static WRITE_HANDLER(sns_pia1b_w) {
  if (snslocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, snslocals.pia1a);
    pia_pulse_ca2(SNS_PIA1, 1);
  }
  else if (snslocals.pia1b & ~data & 0x01) { // read
    pia_set_input_a(SNS_PIA1, tms5220_status_r(0));
    pia_pulse_ca2(SNS_PIA1, 1);
  }
  snslocals.pia1b = data;
}

static WRITE_HANDLER(sns_data_w) {
  snslocals.lastcmd = (snslocals.lastcmd & 0x10) | (data & 0x0f);
}

static WRITE_HANDLER(sns_ctrl_w) {
  snslocals.lastcmd = (snslocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(SNS_PIA0, ~data & 0x01);
}

static READ_HANDLER(sns_8910a_r) { return ~snslocals.lastcmd; }

static WRITE_HANDLER(sns_pia0ca2_w) { sndbrd_ctrl_cb(snslocals.brdData.boardNo,data); } // diag led

static void sns_irq(int state) {
  cpu_set_irq_line(snslocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_5220Irq(int state) { pia_set_input_cb1(SNS_PIA1, !state); }

/*----------------------------------------
/ Zaccaria Sound & Speech Board 13136
/ (similar to BY61, but 2x 6802 CPUs!)
/-----------------------------------------*/
#define ZAC_PIA0 0
#define ZAC_PIA1 1

static void zac_init(struct sndbrdData *brdData);
static void zac_diag(int button);
static WRITE_HANDLER(zac_data_w);
static WRITE_HANDLER(zac_ctrl_w);
static void zac_5220Irq(int state);
static READ_HANDLER(zac_8910a_r);

const struct sndbrdIntf zac13136Intf = {
  zac_init, NULL, zac_diag, zac_data_w, NULL, zac_ctrl_w, NULL, 0 //SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface zac_tms5220Int = { 640000, 50, zac_5220Irq };
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

MACHINE_DRIVER_START(zac13136)
  MDRV_CPU_ADD_TAG("snd1", M6800, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zac_readmem, zac_writemem)

  MDRV_CPU_ADD_TAG("snd2", M6800, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zac_readmem2, zac_writemem2)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, zac_tms5220Int)
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
    tms5220_data_w(0, locals.pia1a);
    pia_pulse_ca2(ZAC_PIA1, 1);
  }
  else if (locals.pia1b & ~data & 0x01) { // read
    pia_set_input_a(ZAC_PIA1, tms5220_status_r(0));
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

static void zac_5220Irq(int state) { pia_set_input_cb1(ZAC_PIA1, !state); }
