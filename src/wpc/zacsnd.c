#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/i8085/i8085.h"
#include "sound/discrete.h"
#include "sound/sn76477.h"
#include "core.h"
#include "sndbrd.h"
#include "zacsnd.h"

extern void UpdateZACSoundLED(int data);
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
static int zac1125_sh_start(const struct MachineSound *msound);
static void zac1125_sh_stop(void);

const struct SN76477interface zac1125_sn76477Int = { 1,	{ 25 }, /* mixing level */
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
  zac1125_init, NULL, NULL, zac1125_data_w, NULL, zac1125_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct CustomSound_interface zac1125_custInt = {zac1125_sh_start, zac1125_sh_stop};

MACHINE_DRIVER_START(zac1125)
//  MDRV_SOUND_ADD(SN76477, zac1125_sn76477Int)
  MDRV_SOUND_ADD(CUSTOM, zac1125_custInt)
MACHINE_DRIVER_END

static WRITE_HANDLER(zac1125_data_w) {
  logerror("data=%2x\n", data);
//  SN76477_sh_update();
}

static WRITE_HANDLER(zac1125_ctrl_w) {
  logerror("ctrl=%2x\n", data);
}

static void zac1125_init(struct sndbrdData *brdData) {
}

static int zac1125_sh_start(const struct MachineSound *msound) {
//  SN76477_sh_start(msound);
  return 0;
}

static void zac1125_sh_stop(void) {
//  SN76477_sh_stop();
}

/*----------------------------------------
/ Zaccaria Sound Board 1346
/ i8085 CPU (8035 on Locomotion?), no PIAs
/-----------------------------------------*/
/*
/ CPU I8085
/ ROM 0000-07ff
*/
static void sp_init(struct sndbrdData *brdData);
static WRITE_HANDLER(sp1346_data_w);
static WRITE_HANDLER(sp1346_ctrl_w);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1346Intf = {
  sp_init, NULL, NULL, sp1346_data_w, NULL, sp1346_ctrl_w, NULL, 0
};

static struct {
  struct sndbrdData brdData;
  int lastcmd, cmdin, cmdout, cmd[2], lastctrl;
  int p20, p21, p22, wr;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.p20 = splocals.p21 = splocals.p22 = 0;
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
  cpu_set_irq_line(splocals.brdData.cpuNo, I8085_INTR_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

READ_HANDLER(rom_r) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  UINT16 address = (offset & 0x7f) | ((offset & 0x100) >> 1)
   | (splocals.p20 << 8) | (splocals.p21 << 9) | (splocals.p22 << 10);
  splocals.wr = (offset & 0x80) ? 0 : 1;
  return rom[address];
}

static MEMORY_READ_START(i8085_readmem)
  { 0x0000, 0x07ff, rom_r },
MEMORY_END

static MEMORY_WRITE_START(i8085_writemem)
MEMORY_END

MACHINE_DRIVER_START(zac1346)
  MDRV_CPU_ADD_TAG("scpu", 8085A, 6000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(i8085_readmem, i8085_writemem)

  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

/*----------------------------------------
/ Zaccaria Sound & Speech Board 1B1370
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
static READ_HANDLER(sns_data_r);

const struct sndbrdIntf zac1370Intf = {
  sns_init, NULL, sns_diag, sns_data_w, NULL, sns_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface sns_tms5220Int = { 640000, 50, sns_5220Irq };
static struct DACinterface     sns_dacInt = { 1, { 20 }};
static struct AY8910interface  sns_ay8910Int = { 1, 3580000/4, {25}, {sns_8910a_r}};

static MEMORY_READ_START(sns_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x2000, 0x2000, sns_data_r },
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
  cpu_set_nmi_line(1, button ? ASSERT_LINE : CLEAR_LINE);
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
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  else if (snslocals.pia1b & ~data & 0x01) { // read
    pia_set_input_a(SNS_PIA1, tms5220_status_r(0));
    pia_set_input_ca2(SNS_PIA1, 1); pia_set_input_ca2(SNS_PIA1, 0);
  }
  snslocals.pia1b = data;
  UpdateZACSoundACT((data>>2)&0x3);	//ACTSND & ACTSPK on bits 2 & 3
}

static WRITE_HANDLER(sns_data_w) {
  //snslocals.lastcmd = (snslocals.lastcmd & 0x10) | (data & 0x0f);
	snslocals.lastcmd = data & 0x7f;
	pia_set_input_cb1(0, data & 0x80 ? 1 : 0);
	//pia_set_input_cb1(SNS_PIA0, ~data & 0x01);
}

static WRITE_HANDLER(sns_ctrl_w) {
  snslocals.lastcmd = (snslocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(SNS_PIA0, ~data & 0x01);
}

static READ_HANDLER(sns_8910a_r) { return ~snslocals.lastcmd; }

static WRITE_HANDLER(sns_pia0ca2_w) {
	//sndbrd_ctrl_cb(snslocals.brdData.boardNo,data);
	UpdateZACSoundLED(data);
} // diag led

static void sns_irq(int state) {
  logerror("sns_irq: state=%x\n",state);
  cpu_set_irq_line(1, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_5220Irq(int state) { pia_set_input_cb1(SNS_PIA1, !state); }

static READ_HANDLER(sns_data_r)
{
	//logerror("%x: reading cmd = %x\n",activecpu_get_previouspc(),~snslocals.lastcmd);
	return ~snslocals.lastcmd;
}

/*--------------------------------------------
/ Zaccaria Sound & Speech Board 1B13136
/ (like 2 x BY61, but only 1 x TMS5220 speech)
//It's reported that the sound board is half populated, so in reality it acts exactly like the older
//1370 board, but with a larger eprom capacity and of course a slightly different memory map
/---------------------------------------------*/
#define ZAC_PIA0 2

static void zac_init(struct sndbrdData *brdData);
static void zac_diag(int button);
static WRITE_HANDLER(zac_data_w);
static WRITE_HANDLER(zac_ctrl_w);
static READ_HANDLER(zac_8910a_r);

const struct sndbrdIntf zac13136Intf = {
  zac_init, NULL, zac_diag, zac_data_w, NULL, zac_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct DACinterface     zac_dacInt = {2, {20, 20}};
static struct AY8910interface  zac_ay8910Int = {2, 3580000/4, {25, 25}, {sns_8910a_r, zac_8910a_r}};

//Seems only difference in memory map from 1370 board is the sound latch read!
static MEMORY_READ_START(zac_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0084, 0x0087, pia_r(ZAC_PIA0) },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x1800, 0x1800, sns_data_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(zac_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNS_PIA0) },
  { 0x0084, 0x0087, pia_w(ZAC_PIA0) },
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x1000, 0x1000, DAC_1_data_w },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(zac13136)
  MDRV_CPU_ADD(M6802, 3580000/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zac_readmem, zac_writemem)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     zac_dacInt)
  MDRV_SOUND_ADD(AY8910,  zac_ay8910Int)
MACHINE_DRIVER_END

static READ_HANDLER(zac_pia0a_r);
static WRITE_HANDLER(zac_pia0a_w);
static WRITE_HANDLER(zac_pia0b_w);
static WRITE_HANDLER(zac_pia0ca2_w);
static void zac_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b;
  int lastcmd, lastctrl;
} locals;

static const struct pia6821_interface zac_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ zac_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ zac_pia0a_w, zac_pia0b_w, zac_pia0ca2_w, 0,
  /*irq: A/B           */ zac_irq, zac_irq
}};

static void zac_init(struct sndbrdData *brdData) {
  locals.brdData = *brdData;
  pia_config(SNS_PIA0, PIA_STANDARD_ORDERING, &sns_pia[0]);
  pia_config(SNS_PIA1, PIA_STANDARD_ORDERING, &sns_pia[1]);
  pia_config(ZAC_PIA0, PIA_STANDARD_ORDERING, &zac_pia[0]);
}

static void zac_diag(int button) {
  cpu_set_nmi_line(2, button ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(zac_pia0a_r) {
  if ((locals.pia0b & 0x03) == 0x01) return AY8910Read(1);
  return 0;
}
static WRITE_HANDLER(zac_pia0a_w) {
  locals.pia0a = data;
  if (locals.pia0b & 0x02) AY8910Write(1, locals.pia0b ^ 0x01, locals.pia0a);
}
static WRITE_HANDLER(zac_pia0b_w) {
  locals.pia0b = data;
  if (locals.pia0b & 0x02) AY8910Write(1, locals.pia0b ^ 0x01, locals.pia0a);
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
  cpu_set_irq_line(2, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
