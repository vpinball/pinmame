#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "by35.h"
#include "by35snd.h"

/*----------------------------------------
/              -32, -50 sound
/-----------------------------------------*/
static void by32_init(struct sndbrdData *brdData);
static WRITE_HANDLER(by32_data_w);
static WRITE_HANDLER(by32_ctrl_w);
static WRITE_HANDLER(by32_manCmd_w);
static int by32_sh_start(const struct MachineSound *msound);
static void by32_sh_stop(void);

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by32Intf = {
  "BY32", by32_init, NULL, NULL, by32_manCmd_w, by32_data_w, NULL, by32_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct CustomSound_interface by32_custInt = {by32_sh_start, by32_sh_stop};

MACHINE_DRIVER_START(by32)
  MDRV_SOUND_ADD(CUSTOM, by32_custInt)
MACHINE_DRIVER_END

#define BY32_DECAYFREQ   50
#define BY32_EXPFACTOR   95   // %
#define BY32_PITCH      100   // 0-100%

/* waveform for the audio hardware */
static const UINT8 sineWave[] = {
  204,188,163,214,252,188,115,125,136,63,0,37,88,63,47,125
};

static struct {
  struct sndbrdData brdData;
  int volume, lastCmd, channel, strobe;
} by32locals;

static void by32_decay(int param) {
  mixer_set_volume(by32locals.channel, by32locals.volume/10);
  if (by32locals.volume < 100) by32locals.volume = 0;
  else by32locals.volume = by32locals.volume * BY32_EXPFACTOR / 100;
}

static int by32_sh_start(const struct MachineSound *msound) {
  by32locals.channel = mixer_allocate_channel(15);
//  mixer_set_lowpass_frequency(by32locals.channel, 2000);
  mixer_set_volume(by32locals.channel,0);
  mixer_play_sample(by32locals.channel, (signed char *)sineWave, sizeof(sineWave), 1000, 1);
  timer_pulse(TIME_IN_HZ(BY32_DECAYFREQ),0,by32_decay);
  return 0;
}

static void by32_sh_stop(void) {
  mixer_stop_sample(by32locals.channel);
}

static void setfreq(int cmd) {
  UINT8 sData; int f;
  if ((cmd != by32locals.lastCmd) && ((cmd & 0x0f) != 0x0f)) {
    sData = core_revbyte(*(by32locals.brdData.romRegion + (cmd ^ 0x10)));
    f= sizeof(sineWave)/((1.1E-6+BY32_PITCH*1E-8)*sData)/8;
    mixer_set_sample_frequency(by32locals.channel, f);
  }
  by32locals.lastCmd = cmd;
}

static WRITE_HANDLER(by32_data_w) { setfreq((by32locals.lastCmd & 0x10) | (data & 0x0f)); }

static WRITE_HANDLER(by32_ctrl_w) {
  if (~by32locals.strobe & data & 0x01) by32locals.volume = 1000;
  else if (~data & 0x01)                by32locals.volume = 0;
  setfreq((by32locals.lastCmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00));
  by32locals.strobe = data;
}
static WRITE_HANDLER(by32_manCmd_w) {
  by32_data_w(0, data); by32_ctrl_w(0,0); by32_ctrl_w(0,((data & 0x10)>>3)|0x01);
}
static void by32_init(struct sndbrdData *brdData) {
  memset(&by32locals, 0, sizeof(by32locals));
  by32locals.brdData = *brdData;
}

/*----------------------------------------
/            Sounds Plus -51
/            Sounds Plus -56 & Vocalizer -57
/-----------------------------------------*/
/*
/  U3  CPU 6802/6808
/      3.58MHz
/
/  U4  ROM f000-ffff (8000-8fff)
/ U1-U8 ROM 8000-ffff (vocalizer board)
/  U10 RAM 0000-007f
/  U2  PIA 0080-0083 (PIA0)
/      A:  8910 DA
/      B0: 8910 BC1
/      B1: 8910 BDIR
/      B6: Speach clock
/      B7: Speach data
/      CA1: SoundEnable
/      CA2: ? (volume circuit)
/      CB2: ? (volume circuit)
/      IRQ: CPU IRQ
/  U1  AY-3-8910
/      IOA0-IOA4 = ~SoundA-E
/      CLK = E
/
*/
#define SP_PIA0  2

static void sp_init(struct sndbrdData *brdData);
static WRITE_HANDLER(sp51_data_w);
static WRITE_HANDLER(sp51_ctrl_w);
static WRITE_HANDLER(sp51_manCmd_w);
static READ_HANDLER(sp_8910a_r);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by51Intf = {
  "BY51", sp_init, NULL, NULL, sp51_manCmd_w, sp51_data_w, NULL, sp51_ctrl_w, NULL,
};

static struct AY8910interface   sp_ay8910Int  = { 1, 3580000/4, {20}, {sp_8910a_r} };
static struct hc55516_interface sp_hc55516Int = { 1, {100}};
static MEMORY_READ_START(sp51_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x1000, 0x1fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START(sp56_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sp_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_w(SP_PIA0) },
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(by51)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sp51_readmem, sp_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(AY8910, sp_ay8910Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(by56)
  MDRV_IMPORT_FROM(by51)
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(sp56_readmem, sp_writemem)
  MDRV_SOUND_ADD(HC55516, sp_hc55516Int)
MACHINE_DRIVER_END

static READ_HANDLER(sp_8910r);
static WRITE_HANDLER(sp_pia0a_w);
static WRITE_HANDLER(sp_pia0b_w);
static void sp_irq(int state);

static const struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ sp_8910r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sp_pia0a_w, sp_pia0b_w, 0, 0,
  /*irq: A/B           */ sp_irq, sp_irq
};

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b;
  int lastcmd, cmdin, cmdout, cmd[2], lastctrl;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
  splocals.cmdin = splocals.cmdout = 2;
}
static READ_HANDLER(sp_8910r) {
  if ((splocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(sp_pia0a_w) {
  splocals.pia0a = data;
  if (splocals.pia0b & 0x02) AY8910Write(0, splocals.pia0b ^ 0x01, splocals.pia0a);
}
static WRITE_HANDLER(sp_pia0b_w) {
  splocals.pia0b = data;
  if (splocals.brdData.subType == 1) { // -56 board
    hc55516_digit_w(0,(data & 0x80)>0);
    hc55516_clock_w(0,(data & 0x40)>0);
  }
  if (splocals.pia0b & 0x02) AY8910Write(0, splocals.pia0b ^ 0x01, splocals.pia0a);
}

static WRITE_HANDLER(sp51_data_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(sp51_ctrl_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_ca1(SP_PIA0, data & 0x01);
}
static WRITE_HANDLER(sp51_manCmd_w) {
  splocals.lastcmd = data;  pia_set_input_ca1(SP_PIA0, 1); pia_set_input_ca1(SP_PIA0, 0);
}

static READ_HANDLER(sp_8910a_r) { return ~splocals.lastcmd; }

static void sp_irq(int state) {
  cpu_set_irq_line(splocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*----------------------------------------
/          Squawk n Talk -61
/-----------------------------------------*/
/*
/  RAM U6 0000-007f
/  ROM U2 e000-e7ff (c000-Cfff)
/  ROM U3 e800-efff (d000-dfff)
/  ROM U4 f000-f7ff (e000-efff)
/  ROM U5 f800-ffff (f000-ffff)
/
/  0800  DAC (J-A)
/  1000  DAC (J-C)
/
/ PIA0
/  A: 0-7 AY8912 Data
/     0-4 Sound cmd (J-EE)
/
/  B: 0   AY8912 BC1
/     1   AY8912 R/W
/     2   Vocalizer Clock
/     3   Vocalizer Data
/     4-7 Sound volume
/ CA1:    NC
/ CA2:    Self-test LED (+5V)
/ CB1:    Sound interrupt (assume it starts high)
/ CB2:	  ?
/ IRQA, IRQB: CPU IRQ
/
/ PIA1: 0090
/  A: 0-7 TMS5200 D7-D0
/  B: 0   TMS5200 ReadStrobe
/     1   TMS5200 WriteStrobe
/     2-3 J38
/     4-7 Speech volume
/ CA1:    NC
/ CA2:    TMS5200 Ready
/ CB1:    TMS5200 Int
/ CB2:    NC
/ IRQA, IRQB: CPU IRQ
*/
#define SNT_PIA0 2
#define SNT_PIA1 3

static void snt_init(struct sndbrdData *brdData);
static void snt_diag(int button);
static WRITE_HANDLER(snt_data_w);
static WRITE_HANDLER(snt_ctrl_w);
static WRITE_HANDLER(snt_manCmd_w);
static void snt_5220Irq(int state);
static READ_HANDLER(snt_8910a_r);

const struct sndbrdIntf by61Intf = {
  "BYSNT", snt_init, NULL, snt_diag, snt_manCmd_w, snt_data_w, NULL, snt_ctrl_w, NULL, 0//SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface snt_tms5220Int = { 640000, 50, snt_5220Irq };
static struct DACinterface     snt_dacInt = { 1, { 20 }};
static struct AY8910interface  snt_ay8910Int = { 1, 3580000/4, {25}, {snt_8910a_r}};

static MEMORY_READ_START(snt_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNT_PIA0) },
  { 0x0090, 0x0093, pia_r(SNT_PIA1) },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snt_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNT_PIA0) },
  { 0x0090, 0x0093, pia_w(SNT_PIA1) },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0xc000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(by61)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snt_readmem, snt_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, snt_tms5220Int)
  MDRV_SOUND_ADD(DAC,     snt_dacInt)
  MDRV_SOUND_ADD(AY8910,  snt_ay8910Int)
MACHINE_DRIVER_END

static READ_HANDLER(snt_pia0a_r);
static WRITE_HANDLER(snt_pia0a_w);
static WRITE_HANDLER(snt_pia0b_w);
static READ_HANDLER(snt_pia1a_r);
static READ_HANDLER(snt_pia1ca2_r);
static READ_HANDLER(snt_pia1cb1_r);
static WRITE_HANDLER(snt_pia1a_w);
static WRITE_HANDLER(snt_pia1b_w);
static WRITE_HANDLER(snt_pia0ca2_w);
static void snt_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a_r, pia1a_w, pia1b;
  int cmd[2], lastcmd, cmdin, cmdout, lastctrl;
} sntlocals;
static const struct pia6821_interface snt_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia0a_r, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, 0,
  /*o: A/B,CA/B2       */ snt_pia0a_w, snt_pia0b_w, snt_pia0ca2_w, 0,
  /*irq: A/B           */ snt_irq, snt_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia1a_r, 0, PIA_UNUSED_VAL(1), snt_pia1cb1_r, snt_pia1ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snt_pia1a_w, snt_pia1b_w, 0, 0,
  /*irq: A/B           */ snt_irq, snt_irq
}};
static void snt_init(struct sndbrdData *brdData) {
  sntlocals.brdData = *brdData;
  pia_config(SNT_PIA0, PIA_STANDARD_ORDERING, &snt_pia[0]);
  pia_config(SNT_PIA1, PIA_STANDARD_ORDERING, &snt_pia[1]);
  sntlocals.cmdin = sntlocals.cmdout = 2;
}
static void snt_diag(int button) {
  cpu_set_nmi_line(sntlocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}
static READ_HANDLER(snt_pia0a_r) {
  if (sntlocals.brdData.subType == 1)   return snt_8910a_r(0); // -61B
  if ((sntlocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(snt_pia0a_w) {
  sntlocals.pia0a = data;
  if (sntlocals.pia0b & 0x02) AY8910Write(0, sntlocals.pia0b ^ 0x01, sntlocals.pia0a);
}
static WRITE_HANDLER(snt_pia0b_w) {
  sntlocals.pia0b = data;
  if (sntlocals.pia0b & 0x02) AY8910Write(0, sntlocals.pia0b ^ 0x01, sntlocals.pia0a);
}
static READ_HANDLER(snt_pia1a_r) { return sntlocals.pia1a_r; }
static WRITE_HANDLER(snt_pia1a_w) { sntlocals.pia1a_w = data; }
static WRITE_HANDLER(snt_pia1b_w) {
  if (~data & 0x02) // write out data to speech chip
    tms5220_data_w(0, sntlocals.pia1a_w);
  sntlocals.pia1a_r = tms5220_status_r(0);
  pia_set_input_ca2(SNT_PIA1, 1); // enable
  sntlocals.pia1b = data;
}
static READ_HANDLER(snt_pia1ca2_r) {
  return !tms5220_ready_r();
}
static READ_HANDLER(snt_pia1cb1_r) {
  return !tms5220_int_r();
}

static WRITE_HANDLER(snt_data_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(snt_ctrl_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(SNT_PIA0, ~data & 0x01);
}
static WRITE_HANDLER(snt_manCmd_w) {
  sntlocals.lastcmd = data;  pia_set_input_cb1(SNT_PIA0, 1); pia_set_input_cb1(SNT_PIA0, 0);
}
static READ_HANDLER(snt_8910a_r) { return ~sntlocals.lastcmd; }

static WRITE_HANDLER(snt_pia0ca2_w) { sndbrd_ctrl_cb(sntlocals.brdData.boardNo,data); } // diag led

static void snt_irq(int state) {
  if (core_gameData->gen & GEN_ALLBY35) tms5220_reset();
  cpu_set_irq_line(sntlocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void snt_5220Irq(int state) { pia_set_input_cb1(SNT_PIA1, !state); }

/*----------------------------------------
/    Cheap Squalk  -45
/-----------------------------------------*/
static void cs_init(struct sndbrdData *brdData);
static WRITE_HANDLER(cs_cmd_w);
static WRITE_HANDLER(cs_ctrl_w);
static READ_HANDLER(cs_port1_r);
static WRITE_HANDLER(cs_port2_w);

const struct sndbrdIntf by45Intf = {
  "BY45", cs_init, NULL, NULL, NULL, cs_cmd_w, NULL, cs_ctrl_w, NULL, 0
};
static struct DACinterface cs_dacInt = { 1, { 20 }};
static MEMORY_READ_START(cs_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },	/*Internal RAM*/
  { 0xb000, 0xdfff, MRA_ROM },
  { 0xe000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(cs_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },	/*Internal RAM*/
  { 0xb000, 0xdfff, MWA_ROM },
  { 0xe000, 0xffff, MWA_ROM },
MEMORY_END
static PORT_READ_START(cs_readport)
  { M6803_PORT2, M6803_PORT2, cs_port1_r },
PORT_END
static PORT_WRITE_START(cs_writeport)
  { M6803_PORT1, M6803_PORT1, DAC_0_data_w },
  { M6803_PORT2, M6803_PORT2, cs_port2_w },
PORT_END

MACHINE_DRIVER_START(by45)
  MDRV_CPU_ADD(M6803, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(cs_readmem, cs_writemem)
  MDRV_CPU_PORTS(cs_readport, cs_writeport)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(DAC, cs_dacInt)
MACHINE_DRIVER_END

static struct {
  struct sndbrdData brdData;
  int cmd, ctrl;
} cslocals;

static void cs_init(struct sndbrdData *brdData) {
  cslocals.brdData = *brdData;
}

static WRITE_HANDLER(cs_cmd_w) { cslocals.cmd = data; }
static WRITE_HANDLER(cs_ctrl_w) {
  cslocals.ctrl = ((data & 1) == cslocals.brdData.subType);
  cpu_set_irq_line(cslocals.brdData.cpuNo, M6803_TIN_LINE, (data & 1) ? ASSERT_LINE : CLEAR_LINE);
}
static READ_HANDLER(cs_port1_r) { return cslocals.ctrl | (cslocals.cmd << 1); }
static WRITE_HANDLER(cs_port2_w) { sndbrd_ctrl_cb(sntlocals.brdData.boardNo,data & 0x01); } // diag led

/*----------------------------------------
/    Turbo Cheap Squalk
/-----------------------------------------*/
#define TCS_PIA0  4
static void tcs_init(struct sndbrdData *brdData);
static void tcs_diag(int button);
static WRITE_HANDLER(tcs_cmd_w);
static WRITE_HANDLER(tcs_ctrl_w);
static READ_HANDLER(tcs_status_r);

const struct sndbrdIntf byTCSIntf = {
  "BYTCS", tcs_init, NULL, tcs_diag, NULL, tcs_cmd_w, tcs_status_r, tcs_ctrl_w, NULL, SNDBRD_NOCBSYNC
};
static struct DACinterface tcs_dacInt = { 1, { 20 }};
static MEMORY_READ_START(tcs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x6000, 0x6003, pia_r(TCS_PIA0) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(tcs_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x6000, 0x6003, pia_w(TCS_PIA0) },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END
static MEMORY_READ_START(tcs2_readmem)
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x0800, 0x0803, pia_r(TCS_PIA0) },
  { 0x0c00, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(tcs2_writemem)
  { 0x0000, 0x07ff, MWA_RAM },
  { 0x0800, 0x0803, pia_w(TCS_PIA0) },
  { 0x0c00, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(byTCS)
  MDRV_CPU_ADD_TAG("scpu", M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(tcs_readmem, tcs_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(DAC, tcs_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(byTCS2)
  MDRV_IMPORT_FROM(byTCS)
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(tcs2_readmem, tcs2_writemem)
MACHINE_DRIVER_END

static WRITE_HANDLER(tcs_pia0cb2_w);
static READ_HANDLER(tcs_pia0b_r);
static WRITE_HANDLER(tcs_pia0a_w);
static WRITE_HANDLER(tcs_pia0b_w);
static void tcs_pia0irq(int state);

static struct {
  struct sndbrdData brdData;
  int cmd, dacdata, status;
} tcslocals;
static const struct pia6821_interface tcs_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, tcs_pia0b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ tcs_pia0a_w, tcs_pia0b_w, 0, tcs_pia0cb2_w,
  /*irq: A/B           */ tcs_pia0irq, tcs_pia0irq
};
static void tcs_init(struct sndbrdData *brdData) {
  tcslocals.brdData = *brdData;
  pia_config(TCS_PIA0, PIA_ALTERNATE_ORDERING, &tcs_pia);
}
static WRITE_HANDLER(tcs_pia0cb2_w) { sndbrd_ctrl_cb(tcslocals.brdData.boardNo,data); }
static void tcs_diag(int button) { cpu_set_nmi_line(tcslocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE); }
static WRITE_HANDLER(tcs_cmd_w) { tcslocals.cmd = data; }
static WRITE_HANDLER(tcs_ctrl_w) { pia_set_input_ca1(TCS_PIA0, data & 0x01); }
static READ_HANDLER(tcs_status_r) { return tcslocals.status; }
static READ_HANDLER(tcs_pia0b_r) {
  int ret = tcslocals.cmd & 0x0f;
  tcslocals.cmd >>= 4;
  return ret;
}
static WRITE_HANDLER(tcs_pia0a_w) {
  tcslocals.dacdata = (tcslocals.dacdata & ~0x3fc) | (data << 2);
  DAC_signed_data_16_w(0, tcslocals.dacdata << 6);
}
static WRITE_HANDLER(tcs_pia0b_w) {
  tcslocals.dacdata = (tcslocals.dacdata & ~0x003) | (data >> 6);
  DAC_signed_data_16_w(0, tcslocals.dacdata << 6);
  tcslocals.status = (data>>4) & 0x03;
}
static void tcs_pia0irq(int state) {
  cpu_set_irq_line(tcslocals.brdData.cpuNo, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

/*----------------------------------------
/    Sounds Deluxe
/-----------------------------------------*/
#define SD_PIA0 5
static void sd_init(struct sndbrdData *brdData);
static void sd_diag(int button);
static WRITE_HANDLER(sd_cmd_w);
static WRITE_HANDLER(sd_ctrl_w);
static READ_HANDLER(sd_status_r);

const struct sndbrdIntf bySDIntf = {
  "BYSD", sd_init, NULL, sd_diag, NULL, sd_cmd_w, sd_status_r, sd_ctrl_w, NULL, 0//SNDBRD_NODATASYNC|SNDBRD_NOCBSYNC
};
static struct DACinterface sd_dacInt = { 1, { 80 }};
static MEMORY_READ16_START(sd_readmem)
  {0x00000000, 0x0003ffff, MRA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_r(SD_PIA0) },	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x0007ffff, MRA16_RAM},		/*RAM*/
MEMORY_END
static MEMORY_WRITE16_START(sd_writemem)
  {0x00000000, 0x0003ffff, MWA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_w(SD_PIA0)},	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x0007ffff, MWA16_RAM},		/*RAM*/
MEMORY_END

MACHINE_DRIVER_START(bySD)
  MDRV_CPU_ADD(M68000, 8000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sd_readmem, sd_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(DAC, sd_dacInt)
MACHINE_DRIVER_END

static WRITE_HANDLER(sd_pia0cb2_w);
static READ_HANDLER(sd_pia0b_r);
static WRITE_HANDLER(sd_pia0a_w);
static WRITE_HANDLER(sd_pia0b_w);
static void sd_pia0irq(int state);

static struct {
  struct sndbrdData brdData;
  int cmd[2], dacdata, status, cmdsync, irqnext;
} sdlocals;

static const struct pia6821_interface sd_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, sd_pia0b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sd_pia0a_w, sd_pia0b_w, 0, sd_pia0cb2_w,
  /*irq: A/B           */ sd_pia0irq, sd_pia0irq
};
static void sd_init(struct sndbrdData *brdData) {
  sdlocals.brdData = *brdData;
  pia_config(SD_PIA0, PIA_ALTERNATE_ORDERING, &sd_pia);
}
static WRITE_HANDLER(sd_pia0cb2_w) {
  sndbrd_ctrl_cb(sdlocals.brdData.boardNo,data);
}
static void sd_diag(int button) {
  cpu_set_irq_line(sdlocals.brdData.cpuNo, MC68000_IRQ_3, button ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(sd_cmd_w) {
  sdlocals.cmd[sdlocals.cmdsync ^= 1] = data;
  if (sdlocals.irqnext) {
    pia_set_input_ca1(SD_PIA0,0); pia_set_input_ca1(SD_PIA0,1);
    sdlocals.irqnext = 0;
  }
}
static WRITE_HANDLER(sd_ctrl_w) {
  if (!(data & 0x01)) sdlocals.irqnext = 1;
}
static READ_HANDLER(sd_status_r) { return sdlocals.status; }

static READ_HANDLER(sd_pia0b_r) {
  return sdlocals.cmd[sdlocals.cmdsync ^= 1];
}

static WRITE_HANDLER(sd_pia0a_w) {
  sdlocals.dacdata = (sdlocals.dacdata & ~0x3fc) | (((UINT16)data) << 2);
  DAC_signed_data_16_w(0, sdlocals.dacdata << 6);
}
static WRITE_HANDLER(sd_pia0b_w) {
  sdlocals.dacdata = (sdlocals.dacdata & ~0x003) | (data >> 6);
  DAC_signed_data_16_w(0, sdlocals.dacdata << 6);
  sdlocals.status = (data>>4) & 0x03;
}
static void sd_pia0irq(int state) {
  cpu_set_irq_line(sdlocals.brdData.cpuNo, MC68000_IRQ_4, state ? ASSERT_LINE : CLEAR_LINE);
}
