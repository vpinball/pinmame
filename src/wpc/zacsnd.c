#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/i8039/i8039.h"
#include "sound/discrete.h"
#include "sound/sn76477.h"
#include "core.h"
#include "sndbrd.h"
#include "zacsnd.h"

extern WRITE_HANDLER(UpdateZACSoundLED);
extern void UpdateZACSoundACT(int data);

/*----------------------------------------
/ Zaccaria Sound-On-Board 1311
/ 4 simple tone generators, solenoid-controlled
/-----------------------------------------*/
DISCRETE_SOUND_START(zac1311_discInt)
	DISCRETE_INPUT(NODE_01,1,0x000f,0)                         // Input handlers, mostly for enable
	DISCRETE_INPUT(NODE_02,2,0x000f,0)
	DISCRETE_INPUT(NODE_04,4,0x000f,0)
	DISCRETE_INPUT(NODE_08,8,0x000f,0)

	DISCRETE_SAWTOOTHWAVE(NODE_10,NODE_01,349,50000,10000,0,0) // F note
	DISCRETE_SAWTOOTHWAVE(NODE_20,NODE_02,440,50000,10000,0,0) // A note
	DISCRETE_SAWTOOTHWAVE(NODE_30,NODE_04,523,50000,10000,0,0) // C' note
	DISCRETE_SAWTOOTHWAVE(NODE_40,NODE_08,698,50000,10000,0,0) // F' note

	DISCRETE_ADDER4(NODE_50,1,NODE_10,NODE_20,NODE_30,NODE_40) // Mix all four sound sources

	DISCRETE_OUTPUT(NODE_50, 50)                               // Take the output from the mixer
DISCRETE_SOUND_END

MACHINE_DRIVER_START(zac1311)
  MDRV_SOUND_ADD(DISCRETE, zac1311_discInt)
MACHINE_DRIVER_END

const struct sndbrdIntf zac1311Intf = {0};

/*----------------------------------------
/ Zaccaria Sound Board 1125
/ SN76477 sound chip, no CPU
/-----------------------------------------*/
static void zac1125_init(struct sndbrdData *brdData);
static WRITE_HANDLER(zac1125_data_w);
static WRITE_HANDLER(zac1125_ctrl_w);

static struct SN76477interface  zac1125_sn76477Int = { 1, { 25 }, /* mixing level */
/*                         pin description		 */
	{ RES_K(47)  },		/*	4  noise_res		 */
	{ RES_K(220) },		/*	5  filter_res		 */
	{ CAP_N(2.2) },		/*	6  filter_cap		 */
	{ RES_M(1.5) },		/*	7  decay_res		 */
	{ CAP_U(2.2) },		/*	8  attack_decay_cap  */
	{ RES_K(4.7) },		/* 10  attack_res		 */
	{ RES_K(47)  },		/* 11  amplitude_res	 */
	{ RES_K(320) },		/* 12  feedback_res 	 */
	{ 0	/* ??? */},		/* 16  vco_voltage		 */
	{ CAP_U(0.33)},		/* 17  vco_cap			 */
	{ RES_K(100) },		/* 18  vco_res			 */
	{ 5.0		 },		/* 19  pitch_voltage	 */
	{ RES_M(1)   },		/* 20  slf_res			 */
	{ CAP_U(2.2) },		/* 21  slf_cap			 */
	{ CAP_U(2.2) },		/* 23  oneshot_cap		 */
	{ RES_M(1.5) }		/* 24  oneshot_res		 */
};

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1125Intf = {
  "ZAC1125", zac1125_init, NULL, NULL, NULL, zac1125_data_w, NULL, zac1125_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(zac1125)
  MDRV_SOUND_ADD(SN76477, zac1125_sn76477Int)
MACHINE_DRIVER_END

static struct {
  UINT8 ctrl;
} s1125locals;

static WRITE_HANDLER(zac1125_data_w) {
  logerror("snd ctrl %d = %d\n", s1125locals.ctrl, data);
}

static WRITE_HANDLER(zac1125_ctrl_w) {
  s1125locals.ctrl = data;
}

static void zac1125_init(struct sndbrdData *brdData) {
  /* MIXER A & C = GND */
  SN76477_mixer_w(0, 0);
  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(0, 1);
  /* fake: pulse the enable line to get rid of the constant noise */
//  SN76477_enable_w(0, 1);
//  SN76477_enable_w(0, 0);
}

/*----------------------------------------
/ Zaccaria Sound Board 1346
/ i8035 MCU, no PIAs
/-----------------------------------------*/
/*
/ MCU I8035
/ ROM 0000-07ff
*/
static void sp_init(struct sndbrdData *brdData);
static WRITE_HANDLER(sp1346_data_w);
static WRITE_HANDLER(sp1346_ctrl_w);
static WRITE_HANDLER(sp1346_manCmd_w);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1346Intf = {
  "ZAC1346", sp_init, NULL, NULL, sp1346_manCmd_w, sp1346_data_w, NULL, sp1346_ctrl_w, NULL, 0
};

static struct {
  struct sndbrdData brdData;
  int lastcmd, lastctrl;
  int p20, p21, p22, wr;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
}

static WRITE_HANDLER(sp1346_data_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(sp1346_ctrl_w) {
  splocals.lastcmd = (splocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
}
static WRITE_HANDLER(sp1346_manCmd_w) {
  splocals.lastcmd = data;
}
static void sp_irq(int state) {
  cpu_set_irq_line(splocals.brdData.cpuNo, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER(rom_r) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  splocals.p20 = i8035_get_reg(I8039_P2) & 0x01;
  splocals.p21 = (i8035_get_reg(I8039_P2) >> 1) & 0x01;
  splocals.p22 = (i8035_get_reg(I8039_P2) >> 2) & 0x01;
  UINT16 address = (offset & 0x7f) | ((offset & 0x100) >> 1)
   | (splocals.p20 << 8) | (splocals.p21 << 9) | (splocals.p22 << 10);
  splocals.wr = (offset & 0x80) ? 0 : 1;
  return rom[address];
}

static MEMORY_READ_START(i8035_readmem)
  { 0x0000, 0x07ff, rom_r },
  { 0x0800, 0x08ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(i8035_writemem)
  { 0x0000, 0x07ff, MWA_ROM },
  { 0x0800, 0x08ff, MWA_RAM },
MEMORY_END

MACHINE_DRIVER_START(zac1346)
  MDRV_CPU_ADD_TAG("scpu", I8035, 6000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(i8035_readmem, i8035_writemem)

  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

/*----------------------------------------
/ Zaccaria Sound & Speech Board 1B1370
/ (almost identical to BY61, TMS5200 chip)
/ and Zaccaria Sound & Speech Board 1B13136
/ (like 1B1370, with additional 6802 CPU and AY8910)
/ It's reported that the sound board is missing the 2nd CPU & ROM chips,
/ so in reality it acts exactly like the older 1370 board
/ with a slightly different memory map (but it can be integrated).
/-----------------------------------------*/
#define SNS_PIA0 0
#define SNS_PIA1 1
#define SNS_PIA2 2

static void sns_init(struct sndbrdData *brdData);
static void sns_diag(int button);
static WRITE_HANDLER(sns_data_w);
static WRITE_HANDLER(sns_ctrl_w);
static void sns_5220Irq(int state);
static READ_HANDLER(sns_8910a_r);
static READ_HANDLER(sns2_8910a_r);

const struct sndbrdIntf zac1370Intf = {
  "ZAC1370", sns_init, NULL, sns_diag, NULL, sns_data_w, NULL, sns_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf zac13136Intf = {
  "ZAC13136", sns_init, NULL, sns_diag, NULL, sns_data_w, NULL, sns_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
const struct sndbrdIntf zac11178Intf = {
  "ZAC11178", sns_init, NULL, sns_diag, NULL, sns_data_w, NULL, sns_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface sns_tms5220Int = { 640000, 50, sns_5220Irq };
static struct DACinterface     sns_dacInt = { 1, { 20 }};
static struct AY8910interface  sns_ay8910Int = { 1, 3580000/4, {25}, {sns_8910a_r}};
static struct AY8910interface  sns2_ay8910Int = { 2, 3580000/4, {25, 25}, {sns_8910a_r, sns2_8910a_r}};

static WRITE_HANDLER(ram80_w) {
  logerror("CEM3374 offset=%d, data=%02x\n", offset, data);
}
static WRITE_HANDLER(rama0_w) {
  logerror("CEM3372 offset=%d, data=%02x\n", offset, data);
}

static MEMORY_READ_START(sns_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x2000, 0x2000, sns_8910a_r },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START(sns2_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0084, 0x0087, pia_r(SNS_PIA2) },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x1800, 0x1800, sns2_8910a_r },
  { 0x2000, 0x2000, sns_8910a_r },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_READ_START(sns3_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNS_PIA0) },
  { 0x0084, 0x0087, pia_w(SNS_PIA2) }, // 13136 only
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns3_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0087, ram80_w },
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x00a0, 0x00a7, rama0_w },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0x4000, 0xffff, MWA_ROM },
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

MACHINE_DRIVER_START(zac13136)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns2_readmem, sns_writemem)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     sns_dacInt)
  MDRV_SOUND_ADD(AY8910,  sns2_ay8910Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns3_readmem, sns3_writemem)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     sns_dacInt)
MACHINE_DRIVER_END

static READ_HANDLER(sns_pia0a_r);
static WRITE_HANDLER(sns_pia0a_w);
static WRITE_HANDLER(sns_pia0b_w);
static WRITE_HANDLER(sns_pia0ca2_w);
static WRITE_HANDLER(sns_pia1a_w);
static WRITE_HANDLER(sns_pia1b_w);
static READ_HANDLER(sns_pia2a_r);
static WRITE_HANDLER(sns_pia2a_w);
static WRITE_HANDLER(sns_pia2b_w);
static WRITE_HANDLER(sns_pia2ca2_w);

static void sns_irqa(int state);
static void sns_irqb(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b, pia2a, pia2b;
  int cmd[2], lastcmd, cmdin, cmdout, lastctrl;
} snslocals;

static const struct pia6821_interface sns_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia0a_w, sns_pia0b_w, sns_pia0ca2_w, 0,
  /*irq: A/B           */ sns_irqa, sns_irqb
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia1a_w, sns_pia1b_w, 0, 0,
  /*irq: A/B           */ sns_irqa, sns_irqb
},{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia2a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sns_pia2a_w, sns_pia2b_w, sns_pia2ca2_w, 0,
  /*irq: A/B           */ 0, 0
}};

static void sns_init(struct sndbrdData *brdData) {
  snslocals.brdData = *brdData;
  pia_config(SNS_PIA0, PIA_STANDARD_ORDERING, &sns_pia[0]);
  pia_config(SNS_PIA1, PIA_STANDARD_ORDERING, &sns_pia[1]);
  if (brdData->boardNo == SNDBRD_ZAC13136)
    pia_config(SNS_PIA2, PIA_STANDARD_ORDERING, &sns_pia[2]);
  snslocals.cmdin = snslocals.cmdout = 2;
}

static void sns_diag(int button) {
  cpu_set_nmi_line(ZACSND_CPUA, button ? ASSERT_LINE : CLEAR_LINE);
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
static WRITE_HANDLER(sns_pia0ca2_w) {
  //sndbrd_ctrl_cb(snslocals.brdData.boardNo,data);
  UpdateZACSoundLED(1, data);
} // diag led

static WRITE_HANDLER(sns_pia1a_w) { snslocals.pia1a = data; }
static WRITE_HANDLER(sns_pia1b_w) {
  if (snslocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, snslocals.pia1a);
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  else if (snslocals.pia1b & ~data & 0x01) { // read
    pia_set_input_a(SNS_PIA1, tms5220_status_r(0));
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  snslocals.pia1b = data;
  UpdateZACSoundACT((data>>2)&0x3);	//ACTSND & ACTSPK on bits 2 & 3
}

static READ_HANDLER(sns_pia2a_r) {
  if ((snslocals.pia2b & 0x03) == 0x01) return AY8910Read(1);
  return 0;
}
static WRITE_HANDLER(sns_pia2a_w) {
  snslocals.pia2a = data;
  if (snslocals.pia2b & 0x02) AY8910Write(1, snslocals.pia2b ^ 0x01, snslocals.pia2a);
}
static WRITE_HANDLER(sns_pia2b_w) {
  snslocals.pia2b = data;
  if (snslocals.pia2b & 0x02) AY8910Write(1, snslocals.pia2b ^ 0x01, snslocals.pia2a);
}
static WRITE_HANDLER(sns_pia2ca2_w) {
    UpdateZACSoundLED(2, data);
} // diag led

static WRITE_HANDLER(sns_data_w) {
  //snslocals.lastcmd = (snslocals.lastcmd & 0x10) | (data & 0x0f);
  snslocals.lastcmd = data & 0x7f;
  //pia_set_input_cb1(SNS_PIA0, ~data & 0x01);
  pia_set_input_cb1(SNS_PIA0, data & 0x80 ? 1 : 0);
  if (snslocals.brdData.boardNo == SNDBRD_ZAC13136)
    pia_set_input_cb1(SNS_PIA2, data & 0x80 ? 1 : 0);
}

static WRITE_HANDLER(sns_ctrl_w) {
  snslocals.lastcmd = (snslocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(SNS_PIA0, ~data & 0x01);
  if (snslocals.brdData.boardNo == SNDBRD_ZAC13136)
    pia_set_input_cb1(SNS_PIA2, ~data & 0x01);
}

static READ_HANDLER(sns_8910a_r) { return ~snslocals.lastcmd; }

static READ_HANDLER(sns2_8910a_r) { return ~snslocals.lastcmd; }

static void sns_irqa(int state) {
  logerror("sns_irqA: state=%x\n",state);
  if (snslocals.brdData.boardNo == SNDBRD_ZAC1370) {
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
  }
  if (snslocals.brdData.boardNo == SNDBRD_ZAC11178) {
    sns_diag(state);
  }
}
static void sns_irqb(int state) {
  logerror("sns_irqB: state=%x\n",state);
  cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_5220Irq(int state) { pia_set_input_cb1(SNS_PIA1, !state); }
