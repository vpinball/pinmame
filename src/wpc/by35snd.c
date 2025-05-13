// license:BSD-3-Clause

#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6809/m6809.h"
#include "sound/discrete.h"
#include "sound/tms5220.h" // uses TMS5200
#include "core.h"
#include "sndbrd.h"
#include "by35.h"
#include "by35snd.h"

#if 0 // port from MAME, to be wired up
static const struct discrete_mixer_desc as2888_digital_mixer_info =
{
	DISC_MIXER_IS_RESISTOR,                       /* type */
	{RES_K(33), RES_K(3.9)},                      /* r{} */
	{0, 0, 0, 0},                                 /* r_node */
	{0, 0},                                       /* c{} */
	0,                                            /* rI  */
//	RES_VOLTAGE_DIVIDER(RES_K(10), RES_R(360)),   /* rF  */
	RES_K(10),                                    /* rF  */   // not really
	CAP_U(0.01),                                  /* cF  */
	0,                                            /* cAmp */
	0,                                            /* vRef */
	0.00002                                       /* gain */
};

static const struct discrete_op_amp_filt_info as2888_preamp_info = {
	RES_K(10), 0, RES_R(470), 0,      /* r1 .. r4 */
	RES_K(10),                        /* rF */
	CAP_U(1),                         /* C1 */
	0,                                /* C2 */
	0,                                /* C3 */
	0.0,                              /* vRef */
	12.0,                             /* vP */
	-12.0,                            /* vN */
};

static DISCRETE_SOUND_START(as2888_discrete)

	DISCRETE_INPUT_DATA(NODE_08)        // Start Sustain Attenuation from 555 circuit
	DISCRETE_INPUT_LOGIC(NODE_01)       // Binary Counter B output (divide by 1) T2
	DISCRETE_INPUT_LOGIC(NODE_04)       // Binary Counter D output (divide by 4) T3

	DISCRETE_DIVIDE(NODE_11, 1, NODE_01, 1) // 2
	DISCRETE_DIVIDE(NODE_14, 1, NODE_04, 1)


	DISCRETE_RCFILTER(NODE_06, 1, NODE_14, RES_K(15), CAP_U(0.1))      // T4 filter
#if 0
	DISCRETE_RCFILTER(NODE_05, 1, NODE_11, RES_K(33), CAP_U(0.01))     // T1 filter
	DISCRETE_ADDER2(NODE_07, 1, NODE_05, NODE_06)
#else

	DISCRETE_MIXER2(NODE_07, 1, NODE_11, NODE_06, &as2888_digital_mixer_info)   // Mix and filter T1 and T4 together
#endif
	DISCRETE_RCDISC5(NODE_87, 1, NODE_08, RES_K(150), CAP_U(1.0))

	DISCRETE_RCFILTER_VREF(NODE_88,1,NODE_87,RES_M(1),CAP_U(0.01),2)
	DISCRETE_MULTIPLY(NODE_09, 1, NODE_07, NODE_88)    // Apply sustain

	DISCRETE_OP_AMP_FILTER(NODE_20, 1, NODE_09, 0, DISC_OP_AMP_FILTER_IS_HIGH_PASS_1, &as2888_preamp_info)

	DISCRETE_CRFILTER(NODE_25, NODE_20, RES_M(100), CAP_U(0.05))    // Resistor is fake. Capacitor in series between pre-amp and output amp.

	DISCRETE_GAIN(NODE_30, NODE_25, 50) // Output amplifier LM380 fixed inbuilt gain of 50

	DISCRETE_OUTPUT(NODE_30, 10000000)  //  17000000
DISCRETE_SOUND_END
#endif

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
#if 0
  MDRV_SOUND_ADD(DISCRETE, as2888_discrete)
#else
  MDRV_SOUND_ADD(CUSTOM, by32_custInt)
#endif
#ifdef ENABLE_MECHANICAL_SAMPLES
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
#endif
MACHINE_DRIVER_END

#define BY32_DECAYFREQ   50
#define BY32_EXPFACTOR   90
#define BY32_PITCH      400

/* waveform for the audio hardware */
static const INT16 sineWave[] = {
/*
    6836,  16956,  20464,  22477,  24174,  24988,  25600,  25699,
   25710,  11529,   6486,   6222,   6587,   6805,   6835,   6574,
    6347,   6026,  21288,  23102,  22166,  20086,  18564,  17068,
   15433,  14130,  12733,  -4363,  -7370,  -7842,  -7468,  -7275,
   -7076,  -6897,  -6630,  -2875,   6858,   5115,    482,  -2879,
   -5526,  -8070,  -9501, -10671, -16040, -27830, -29562, -28556,
  -27184, -25808, -24037, -22682, -20848, -14203,  -2291,    513,
     879,    880,    959,   1074,   1244,   1380,  -4861, -14419,
  -15251, -14156, -12504, -11270, -10166,  -9080,  -8086,    497
*/
    -411,   9756,  21784,  27153,  29033,  30258,  30725,  30748,
   30328,  29687,  28821,  27547,  26363,  25106,  23445,  22274,
   20969,  19560,   4032,  -2111,  -3506,  -3407,  -3325,  -3334,
   -3311,  -3336,  -3373,  -3400,  -3446,  -3459,  -3514,  -3547,
   -3793,  -3949,  -2817,  12083,  14726,  13756,  12298,  10663,
    9447,   8297,   6956,   5936,   4985,   3869,   3060,   2291,
    1416,    891,    219,  -5349, -17793, -19641, -19026, -18017,
  -16879, -15996, -15140, -14122, -13342, -12609, -11688, -11009,
  -10354,  -9703,  -9137,  -8405,   1461,   6115,   2565,  -1128,
   -4224,  -7153,  -8915, -10243, -11378, -11950, -12304, -12431,
  -12364, -12180, -11539, -11005, -10365, -23724, -27447, -26336,
  -24325, -22384, -20117, -18338, -16665, -14691, -13182, -11771,
  -10091,  -8838,  -7650,  -6468,  -5566,  -4913,  12261,  15587,
   15601,  14905,  14239,  13449,  12861,  12276,  11609,  11068,
   10570,   9945,   9493,   9060,   8629,   8188,   2540,  -9151,
  -10863, -10051,  -8964,  -8013,  -6870,  -6009,  -5199,  -4240,
   -3520,  -2882,  -2103,  -1550,  -1051,   -630
};

static struct {
  struct sndbrdData brdData;
  int volume, lastCmd, channel, strobe;
} by32locals;

static void by32_decay(int param) {
  mixer_set_volume(by32locals.channel, by32locals.volume/10);
  if (by32locals.volume < 50) by32locals.volume = 0;
  else by32locals.volume = by32locals.volume * BY32_EXPFACTOR / 100;
}

static int by32_sh_start(const struct MachineSound *msound) {
  by32locals.channel = mixer_allocate_channel(30);
  mixer_play_sample_16(by32locals.channel, (INT16 *)sineWave, sizeof(sineWave), 0, 1);
  timer_pulse(TIME_IN_HZ(BY32_DECAYFREQ),0,by32_decay);
  return 0;
}

static void by32_sh_stop(void) {
  mixer_stop_sample(by32locals.channel);
}

static void setfreq(int cmd) {
  if ((cmd != by32locals.lastCmd) && ((cmd & 0x0f) != 0x0f)) {
    UINT8 sData = core_revbyte(*(by32locals.brdData.romRegion + (cmd ^ 0x10)));
    double f = sizeof(sineWave)/((1.1E-6+BY32_PITCH*1E-8)*sData)/8;
    mixer_set_sample_frequency(by32locals.channel, f);
    by32locals.volume = 1000;
    mixer_set_volume(by32locals.channel, 100);
  }
  by32locals.lastCmd = cmd;
}

static WRITE_HANDLER(by32_data_w) {
  if (~by32locals.strobe & 0x01) {
    by32locals.lastCmd = (by32locals.lastCmd & 0x10) | (data & 0x0f);
  } else {
    setfreq((by32locals.lastCmd & 0x10) | (data & 0x0f));
  }
}

static WRITE_HANDLER(by32_ctrl_w) {
  if (~by32locals.strobe & data & 0x01) {
    setfreq((by32locals.lastCmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00));
  } else if (by32locals.strobe & ~data & 0x01) {
    by32locals.volume = 0;
    by32locals.lastCmd = (by32locals.lastCmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  }
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
/U1-U8 ROM 8000-ffff (vocalizer board)
/  U10 RAM 0000-007f
/  U2  PIA 0080-0083 (PIA0)
/      A  : 8910 DA
/      B0 : 8910 BC1
/      B1 : 8910 BDIR
/      B6 : Speech clock
/      B7 : Speech data
/      CA1: SoundEnable
/      CB1: fed by 555 timer (not equipped?)
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
static void sp_diag(int button);
static WRITE_HANDLER(sp51_data_w);
static WRITE_HANDLER(sp51_ctrl_w);
static WRITE_HANDLER(sp51_manCmd_w);
static READ_HANDLER(sp_8910a_r);

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by51Intf = {
  "BY51", sp_init, NULL, sp_diag, sp51_manCmd_w, sp51_data_w, NULL, sp51_ctrl_w, NULL,
};

static struct AY8910interface   sp_ay8910Int  = { 1, 3579545./4., {20}, {sp_8910a_r} };

static MEMORY_READ_START(sp51_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x1000, 0x1fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sp_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_w(SP_PIA0) },
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(by51)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sp51_readmem, sp_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(AY8910, sp_ay8910Int)
MACHINE_DRIVER_END

static struct DACinterface sp_dacInt = { 1, { 20 }};

static MEMORY_READ_START(sp51N_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sp51N_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_w(SP_PIA0) },
  { 0x1000, 0x1000, DAC_0_data_w },
MEMORY_END

MACHINE_DRIVER_START(by51N)
  MDRV_IMPORT_FROM(by51)
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(sp51N_readmem, sp51N_writemem)
  MDRV_SOUND_ADD(DAC, sp_dacInt)
MACHINE_DRIVER_END

static struct mc3417_interface sp_mc3417Int = { 1, {100}};

static MEMORY_READ_START(sp56_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MACHINE_DRIVER_START(by56)
  MDRV_IMPORT_FROM(by51)
  MDRV_CPU_MODIFY("scpu")
  MDRV_CPU_MEMORY(sp56_readmem, sp_writemem)
  MDRV_SOUND_ADD(MC3417, sp_mc3417Int)
MACHINE_DRIVER_END

static READ_HANDLER(sp_8910r);
static READ_HANDLER(sp_pia0b_r);
static WRITE_HANDLER(sp_pia0a_w);
static WRITE_HANDLER(sp_pia0b_w);
static WRITE_HANDLER(sp_pia0cb2_w);
static void sp_irq(int state);

static const struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ sp_8910r, sp_pia0b_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, 0,
  /*o: A/B,CA/B2       */ sp_pia0a_w, sp_pia0b_w, 0, sp_pia0cb2_w,
  /*irq: A/B           */ sp_irq, sp_irq
};

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b;
  UINT8 lastcmd, cmd[2], lastctrl;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  int i;
  splocals.brdData = *brdData;
  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
  //if (splocals.brdData.subType == 1) { // -56 board
  //  mc3417_set_gain(0, 0.92);
  //}
  for (i=0; i < 0x80; i++) memory_region(BY51_CPUREGION)[i] = 0xff;
}
static void sp_diag(int button) {
  cpu_set_nmi_line(splocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}
static READ_HANDLER(sp_8910r) {
  if (splocals.brdData.subType == 2) return ~splocals.lastcmd; // -51N
  if ((splocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(sp_pia0a_w) {
  splocals.pia0a = data;
  if (splocals.pia0b & 0x02) AY8910Write(0, splocals.pia0b ^ 0x01, splocals.pia0a);
}
static READ_HANDLER(sp_pia0b_r) {
  return splocals.pia0b;
}
static WRITE_HANDLER(sp_pia0b_w) {
  splocals.pia0b = data;
  if (splocals.brdData.subType == 1) { // -56 board
    mc3417_digit_w(0,(data & 0x80)>0);
    mc3417_clock_w(0,(data & 0x40)>0);
  }
  if (splocals.pia0b & 0x02) AY8910Write(0, splocals.pia0b ^ 0x01, splocals.pia0a);
}
static WRITE_HANDLER(sp_pia0cb2_w) {
  logerror("Mute sound: %d\n", data);
  // spaceinv seems to use this feature at game start time but not anymore afterwards!?
  mixer_set_volume(0, data ? 75 : 100);
}

static WRITE_HANDLER(sp51_data_w) {
  if (splocals.brdData.subType == 2) { // -51N
    splocals.lastcmd = data & 0x0f;
    if ((splocals.lastctrl & 1) && data != 0x0f) {
      sp_irq(1); sp_irq(0);
    }
  } else {
    splocals.lastcmd = (splocals.lastcmd & 0x10) | (data & 0x0f);
  }
}
static WRITE_HANDLER(sp51_ctrl_w) {
  if (splocals.brdData.subType != 2) { // not -51N
    splocals.lastcmd = (splocals.lastcmd & 0x0f) | ((data & 0x02) << 3);
  }
  pia_set_input_ca1(SP_PIA0, data & 0x01);
  splocals.lastctrl = data;
}
static WRITE_HANDLER(sp51_manCmd_w) {
  splocals.lastcmd = data;  pia_set_input_ca1(SP_PIA0, 1); pia_set_input_ca1(SP_PIA0, 0);
  if (splocals.brdData.subType == 2) { // -51N
    sp_irq(1); sp_irq(0);
  }
}

static READ_HANDLER(sp_8910a_r) {
  return (0x1f & ~splocals.lastcmd) | 0x20;
}

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
/ CB2:    ?
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
#define SNT2_PIA0 4
#define SNT2_PIA1 5

static void snt_init(struct sndbrdData *brdData);
static void snt_diag(int button);
static WRITE_HANDLER(snt_data_w);
static WRITE_HANDLER(snt_ctrl_w);
static WRITE_HANDLER(snt_manCmd_w);
static void snt_5220Irq(int state);
static void snt_5220Rdy(int state);
READ_HANDLER(snt_8910a_r);

const struct sndbrdIntf by61Intf = {
  "BYSNT", snt_init, NULL, snt_diag, snt_manCmd_w, snt_data_w, NULL, snt_ctrl_w, NULL, 0//SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};
static struct TMS5220interface snt_tms5220Int = { 640000, 75, snt_5220Irq, snt_5220Rdy };
static struct DACinterface     snt_dacInt = { 1, { 20 }};
static struct AY8910interface  snt_ay8910Int = { 1, 3579545./4., {25}, {snt_8910a_r}};

static MEMORY_READ_START(snt_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNT_PIA0) },
  { 0x0090, 0x0093, pia_r(SNT_PIA1) },
  { 0x1000, 0x1000, MRA_NOP },
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
  MDRV_CPU_ADD(M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snt_readmem, snt_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, snt_tms5220Int)
  MDRV_SOUND_ADD(DAC,     snt_dacInt)
  MDRV_SOUND_ADD(AY8910,  snt_ay8910Int)
MACHINE_DRIVER_END

static struct DACinterface     snt_dacInt2 = { 2, { 20, 20 }};

static MEMORY_READ_START(snt_readmem2)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNT2_PIA0) },
  { 0x0090, 0x0093, pia_r(SNT2_PIA1) },
  { 0x1000, 0x1000, MRA_NOP },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snt_writemem2)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNT2_PIA0) },
  { 0x0090, 0x0093, pia_w(SNT2_PIA1) },
  { 0x1000, 0x1000, DAC_1_data_w },
  { 0xc000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(by61x2)
  MDRV_CPU_ADD(M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snt_readmem, snt_writemem)

  MDRV_CPU_ADD(M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snt_readmem2, snt_writemem2)
  MDRV_INTERLEAVE(500)

  MDRV_SOUND_ADD(TMS5220, snt_tms5220Int)
  MDRV_SOUND_ADD(DAC,     snt_dacInt2)
  MDRV_SOUND_ADD(AY8910,  snt_ay8910Int)
MACHINE_DRIVER_END

static READ_HANDLER(snt_pia0a_r);
static READ_HANDLER(snt_pia0b_r);
static WRITE_HANDLER(snt_pia0a_w);
static WRITE_HANDLER(snt_pia0b_w);
static WRITE_HANDLER(snt_pia0ca2_w);

static READ_HANDLER(snt_pia1a_r);
static READ_HANDLER(snt_pia1ca2_r);
static READ_HANDLER(snt_pia1cb1_r);
static WRITE_HANDLER(snt_pia1a_w);
static WRITE_HANDLER(snt_pia1b_w);

static READ_HANDLER(snt2_pia0a_r);
static WRITE_HANDLER(snt2_pia0ca2_w);

static void snt_irq(int state);
static void snt2_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b, pia1cb1, pia1ca2;
  UINT8 cmd[2], lastcmd, lastctrl;
} sntlocals;

static const struct pia6821_interface snt_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia0a_r, snt_pia0b_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snt_pia0a_w, snt_pia0b_w, snt_pia0ca2_w, 0,
  /*irq: A/B           */ snt_irq, snt_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia1a_r, 0, PIA_UNUSED_VAL(1), snt_pia1cb1_r, snt_pia1ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snt_pia1a_w, snt_pia1b_w, 0, 0,
  /*irq: A/B           */ snt_irq, snt_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snt2_pia0a_r, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, snt2_pia0ca2_w, 0,
  /*irq: A/B           */ snt2_irq, snt2_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, 0, 0, 0,
  /*irq: A/B           */ snt2_irq, snt2_irq
}};

static void snt_init(struct sndbrdData *brdData) {
  int i;
  sntlocals.brdData = *brdData;
  pia_config(SNT_PIA0, PIA_STANDARD_ORDERING, &snt_pia[0]);
  pia_config(SNT_PIA1, PIA_STANDARD_ORDERING, &snt_pia[1]);
  if (brdData->subType == 2) { // Mysterian, uses a 2nd board without any AY8910 or TMS speech chip, just the DAC is used
    pia_config(SNT2_PIA0, PIA_STANDARD_ORDERING, &snt_pia[2]);
    pia_config(SNT2_PIA1, PIA_STANDARD_ORDERING, &snt_pia[3]);
  }
  tms5220_reset();
  tms5220_set_variant(TMS5220_IS_5200);
  for (i=0; i < 0x80; i++) memory_region(BY61_CPUREGION)[i] = 0xff;
  if (core_gameData->hw.gameSpecific1 & BY35GD_REVERB) {
    tms5220_set_reverb_filter(0.25f, (float)core_getDip(4) * 0.05f);
    AY8910_set_reverb_filter(0, 0.25f, (float)core_getDip(4) * 0.05f);
    DAC_set_reverb_filter(0, 0.25f, (float)core_getDip(4) * 0.05f);
  }
}
static void snt_diag(int button) {
  cpu_set_nmi_line(sntlocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(snt_pia0a_r) {
  if (sntlocals.brdData.subType && sntlocals.brdData.subType < 3) return snt_8910a_r(0); // -61B
  if ((sntlocals.pia0b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}
static READ_HANDLER(snt_pia0b_r) {
  return sntlocals.pia0b;
}
static WRITE_HANDLER(snt_pia0a_w) {
  sntlocals.pia0a = data;
  if (sntlocals.pia0b & 0x02) AY8910Write(0, sntlocals.pia0b ^ 0x01, sntlocals.pia0a);
}
static WRITE_HANDLER(snt_pia0b_w) {
  sntlocals.pia0b = data;
  if (sntlocals.pia0b & 0x02) AY8910Write(0, sntlocals.pia0b ^ 0x01, sntlocals.pia0a);
}
static READ_HANDLER(snt_pia1a_r) { return sntlocals.pia1a; }
static WRITE_HANDLER(snt_pia1a_w) { sntlocals.pia1a = data; }
static WRITE_HANDLER(snt_pia1b_w) {
  if (sntlocals.pia1b & ~data & 0x01) { // read, overrides write command!
    sntlocals.pia1a = tms5220_status_r(0);
  } else if (sntlocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, sntlocals.pia1a);
  }
  sntlocals.pia1b = data;
  pia_set_input_ca2(SNT_PIA1, tms5220_ready_r());
}
static READ_HANDLER(snt_pia1ca2_r) {
  return sntlocals.pia1ca2;
}
static READ_HANDLER(snt_pia1cb1_r) {
  return sntlocals.pia1cb1;
}

static READ_HANDLER(snt2_pia0a_r) {
  return snt_8910a_r(0);
}

static WRITE_HANDLER(snt_data_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x10) | (data & 0x0f);
}
static WRITE_HANDLER(snt_ctrl_w) {
  sntlocals.lastcmd = (sntlocals.lastcmd & 0x0f) | ((data & 0x02) ? 0x10 : 0x00);
  pia_set_input_cb1(SNT_PIA0, ~data & 0x01);
  switch (sntlocals.brdData.subType) {
    case 2:
      pia_set_input_cb1(SNT2_PIA0, ~data & 0x01);
      break;
    case 3: // Cosmic Flash needs IRQ triggered depending on LSB
      if (data & 0x01) cpu_set_irq_line(sntlocals.brdData.cpuNo, M6802_IRQ_LINE, PULSE_LINE);
  }
}

static int manualSoundcmd = 0; // only for sound command mode
static WRITE_HANDLER(snt_manCmd_w) {
  manualSoundcmd = 1;
  sntlocals.lastcmd = data;
  pia_set_input_cb1(SNT_PIA0, 1);
  pia_set_input_cb1(SNT_PIA0, 0);
  switch (sntlocals.brdData.subType) {
    case 2:
      pia_set_input_cb1(SNT2_PIA0, 1);
      pia_set_input_cb1(SNT2_PIA0, 0);
      break;
    case 3:
      cpu_set_irq_line(sntlocals.brdData.cpuNo, M6802_IRQ_LINE, PULSE_LINE);
  }
}

READ_HANDLER(snt_8910a_r) {
  if (!manualSoundcmd)
    return ~sntlocals.lastcmd;
  else // S&T needs special handling for sound command mode
  {
    static UINT8 first = 1;
    if (first) // first nibble/least significant 4-bits
    {
      const UINT8 cmd = sntlocals.lastcmd & 0x0f;
      first = 0;
      return ~cmd;
    }
    else // second nibble/most significant 4-bits
    {
      const UINT8 cmd = sntlocals.lastcmd >> 4;
      first = 1;
      manualSoundcmd = 0; // manual soundcommand finished
      return ~cmd;
    }
  }
}

static WRITE_HANDLER(snt_pia0ca2_w) { sndbrd_ctrl_cb(sntlocals.brdData.boardNo,data); } // diag led
static WRITE_HANDLER(snt2_pia0ca2_w) { sndbrd_ctrl_cb(sntlocals.brdData.boardNo, data << 1); } // diag led of 2nd board

static void snt_irq(int state) {
  cpu_set_irq_line(sntlocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void snt2_irq(int state) {
  cpu_set_irq_line(sntlocals.brdData.cpuNo + 1, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void snt_5220Irq(int state) { pia_set_input_cb1(SNT_PIA1, (sntlocals.pia1cb1 = !state)); }
static void snt_5220Rdy(int state) { pia_set_input_ca2(SNT_PIA1, (sntlocals.pia1ca2 = state)); }

/*----------------------------------------
/    Cheap Squeak  -45
/-----------------------------------------*/
static void cs_init(struct sndbrdData *brdData);
static void cs_diag(int button);
static WRITE_HANDLER(cs_cmd_w);
static WRITE_HANDLER(cs_ctrl_w);
static READ_HANDLER(cs_port2_r);
static WRITE_HANDLER(cs_port2_w);

const struct sndbrdIntf by45Intf = {
  "BY45", cs_init, NULL, cs_diag, NULL, cs_cmd_w, NULL, cs_ctrl_w, NULL, 0
};
static struct DACinterface cs_dacInt = { 1, { 20 }};
static MEMORY_READ_START(cs_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },	/*Internal RAM*/
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
static MEMORY_WRITE_START(cs_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },	/*Internal RAM*/
  { 0x8000, 0xffff, MWA_NOP },
MEMORY_END
static PORT_READ_START(cs_readport)
  { M6803_PORT2, M6803_PORT2, cs_port2_r },
PORT_END
static PORT_WRITE_START(cs_writeport)
  { M6803_PORT1, M6803_PORT1, DAC_0_data_w },
  { M6803_PORT2, M6803_PORT2, cs_port2_w },
PORT_END

MACHINE_DRIVER_START(by45)
  MDRV_CPU_ADD(M6803, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(cs_readmem, cs_writemem)
  MDRV_CPU_PORTS(cs_readport, cs_writeport)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(DAC, cs_dacInt)
MACHINE_DRIVER_END

static struct {
  struct sndbrdData brdData;
  int cmd, ctrl, p21;
} cslocals;

static void cs_init(struct sndbrdData *brdData) {
  cslocals.brdData = *brdData;
}

static void cs_diag(int button) {
  cpu_set_nmi_line(cslocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(cs_cmd_w) { cslocals.cmd = data; }
static WRITE_HANDLER(cs_ctrl_w) {
  cslocals.ctrl = ((data & 1) == cslocals.brdData.subType);
  cpu_set_irq_line(cslocals.brdData.cpuNo, M6803_TIN_LINE, (data & 1) ? ASSERT_LINE : CLEAR_LINE);
}

void by45_p21_w(UINT8 data)
{
	cslocals.p21 = data ? 1 : 0;
}

static READ_HANDLER(cs_port2_r) {
	//static int last = 0xff;
	int data = cslocals.ctrl | (cslocals.cmd << 1);
	if (cslocals.p21) data |= 0x02;
#if 0
	if(last != data)
		printf("cs_port2_r = %x\n",data);
	last = data;
#endif
	return data;
}

static WRITE_HANDLER(cs_port2_w) {
	//printf("MPU: port write = %x\n",data);
	sndbrd_ctrl_cb(cslocals.brdData.boardNo,data & 0x01);
} // diag led

/*----------------------------------------
/    Turbo Cheap Squeak
/-----------------------------------------*/
#define TCS_PIA0  2
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
  /*i: A/B,CA/B1,CA/B2 */ 0, tcs_pia0b_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, 0,
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
#define SD_PIA0 2
static void sd_init(struct sndbrdData *brdData);
static void sd_diag(int button);
static WRITE_HANDLER(sd_cmd_w);
static WRITE_HANDLER(sd_ctrl_w);
static WRITE_HANDLER(sd_man_w);
static READ_HANDLER(sd_status_r);

const struct sndbrdIntf bySDIntf = {
  "BYSD", sd_init, NULL, sd_diag, sd_man_w, sd_cmd_w, sd_status_r, sd_ctrl_w, NULL, 0//SNDBRD_NODATASYNC|SNDBRD_NOCBSYNC
};
static struct DACinterface sd_dacInt = { 1, { 80 }};
static MEMORY_READ16_START(sd_readmem)
  {0x00000000, 0x0003ffff, MRA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_r(SD_PIA0) },	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x00070fff, MRA16_RAM},		/*RAM*/
MEMORY_END
static MEMORY_WRITE16_START(sd_writemem)
  {0x00000000, 0x0003ffff, MWA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_w(SD_PIA0)},	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x00070fff, MWA16_RAM},		/*RAM*/
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
  UINT16 dacdata;
  int status, ledcount;
  UINT8 cmd, latch;
} sdlocals;

static const struct pia6821_interface sd_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, sd_pia0b_r, PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(1), 0, 0,
  /*o: A/B,CA/B2       */ sd_pia0a_w, sd_pia0b_w, 0, sd_pia0cb2_w,
  /*irq: A/B           */ sd_pia0irq, sd_pia0irq
};
static void sd_init(struct sndbrdData *brdData) {
  sdlocals.brdData = *brdData;
  sdlocals.ledcount = 0;
  pia_config(SD_PIA0, PIA_ALTERNATE_ORDERING, &sd_pia);
}
static WRITE_HANDLER(sd_pia0cb2_w) {
  if (!data) {
    sdlocals.ledcount++;
    logerror("SD LED: %d\n", sdlocals.ledcount);
    if (core_gameData->hw.gameSpecific1 & 1) { // hack for Blackwater 100 (main CPU boots up too fast)
      if (sdlocals.ledcount == 5) // suspend main cpu, soundboard not ready yet
        cpu_set_halt_line(0, 1);
      else if (sdlocals.ledcount == 6) // resume main cpu, soundboard ready
        cpu_set_halt_line(0, 0);
    }
  }
  sndbrd_ctrl_cb(sdlocals.brdData.boardNo,data);
}
static void sd_diag(int button) {
  cpu_set_irq_line(sdlocals.brdData.cpuNo, MC68000_IRQ_3, button ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(sd_man_w) {
  sd_cmd_w(0, data);
  sd_ctrl_w(0, 0);
  sd_ctrl_w(0, 1);
  sd_cmd_w(0, data >> 4);
}
static WRITE_HANDLER(sd_cmd_w) {
  logerror("SD cmd: %02x\n", data);
  sdlocals.latch = data;
}
static WRITE_HANDLER(sd_ctrl_w) {
  logerror("SD ctrl:%d\n", data);
  if (!(data & 0x01)) sdlocals.cmd = sdlocals.latch;
  pia_set_input_ca1(SD_PIA0, data & 0x01);
}
static READ_HANDLER(sd_status_r) { return sdlocals.status; }

static READ_HANDLER(sd_pia0b_r) {
  UINT8 val = 0x30 | (sdlocals.cmd & 0x0f);
  logerror("SD read:%02x\n", val);
  sdlocals.cmd >>= 4;
  return val;
}

static WRITE_HANDLER(sd_pia0a_w) {
  sdlocals.dacdata = (sdlocals.dacdata & 0x00ff) | (data << 8);
}
static WRITE_HANDLER(sd_pia0b_w) {
  sdlocals.dacdata = (sdlocals.dacdata & 0xff00) | (data & 0xc0);
  DAC_signed_data_16_w(0, sdlocals.dacdata);
  sdlocals.status = (data>>4) & 0x03;
}
static void sd_pia0irq(int state) {
  cpu_set_irq_line(sdlocals.brdData.cpuNo, MC68000_IRQ_4, state ? ASSERT_LINE : CLEAR_LINE);
}
