#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6502/m6502.h"
#include "cpu/i8039/i8039.h"
#include "sound/discrete.h"
#include "sound/sn76477.h"
#include "sound/tms5220.h" // TMS5200 & TMS5220CNL
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

static struct SN76477interface zac1125_sn76477Int = { 1, { 50 }, /* mixing level */
/*                       pin description      */
  { RES_K(47)   },    /*  4  noise_res        */
  { RES_K(220)  },    /*  5  filter_res       */
  { CAP_N(2.2)  },    /*  6  filter_cap       */
  { RES_M(1.5)  },    /*  7  decay_res        */
  { CAP_U(2.2)  },    /*  8  attack_decay_cap */
  { RES_K(4.7)  },    /* 10  attack_res       */
  { RES_K(47)   },    /* 11  amplitude_res    */
  { RES_K(320)  },    /* 12  feedback_res     */
  { 0 /* ??? */ },    /* 16  vco_voltage      */
  { CAP_U(0.33) },    /* 17  vco_cap          */
  { RES_K(100)  },    /* 18  vco_res          */
  { 5.0         },    /* 19  pitch_voltage    */
  { RES_M(1)    },    /* 20  slf_res          */
  { CAP_U(2.2)  },    /* 21  slf_cap          */
  { CAP_U(2.2)  },    /* 23  oneshot_cap      */
  { RES_M(1.5)  }     /* 24  oneshot_res      */
};

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1125Intf = {
  "ZAC1125", zac1125_init, NULL, NULL, zac1125_data_w, zac1125_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(zac1125)
  MDRV_SOUND_ADD(SN76477, zac1125_sn76477Int)
MACHINE_DRIVER_END

static struct {
  UINT8 ctrl;
  double ne555_voltage;
  mame_timer *ne555;
} s1125locals;

static void ne555_timer(int n) {
  s1125locals.ne555_voltage += 0.02;
  SN76477_set_vco_voltage(0, s1125locals.ne555_voltage);
  if (s1125locals.ne555_voltage > 4.99) // stop timer if maximum voltage is reached
    timer_adjust(s1125locals.ne555, TIME_NEVER, 0, TIME_NEVER);
}

static WRITE_HANDLER(zac1125_data_w) {
  static double states[8][5] = { // pins 7, 18, 20, 24, and optional timer interval
    { RES_K(468), RES_K(100),RES_M(1), RES_K(9.9) },
    { RES_K(4.7), RES_K(32), RES_M(1), RES_K(136) },
    { RES_K(468), RES_K(25), RES_M(1), RES_K(9.9) },
    { RES_K(468), RES_K(32), RES_M(1), RES_K(9.9) },
    { RES_K(192), RES_K(4.5),RES_M(1), RES_M(1.5), 0.015 },
    { RES_K(26.5),RES_K(4.5),RES_M(1), RES_K(9.9), 0.0003 },
    { RES_M(1.5), RES_K(100),RES_M(1), RES_K(358) },
    { RES_K(192), RES_K(9.1),RES_K(32),RES_K(94)  }
  };
  static int vco[8] = { 1, 1, 1, 1, 0, 0, 1, 1 };
  static int mix[8] = { 0, 0, 0, 0, 0, 0, 2, 0 };

  data &= 0x0f;
  if (data) {
    int state = data >> 1;
    SN76477_enable_w       (0, 1);
    logerror("Sound %x plays\n", state);
    SN76477_mixer_w        (0, mix[state]);
    SN76477_set_decay_res  (0, states[state][0]); /* 7 */
    SN76477_set_vco_res    (0, states[state][1]); /* 18 */
    SN76477_set_slf_res    (0, states[state][2]); /* 20 */
    SN76477_set_oneshot_res(0, states[state][3]); /* 24 */
    SN76477_vco_w          (0, vco[state]);
    if (!vco[state]) { // simulate the loading of the capacitors by using a timer
      SN76477_set_vco_voltage(0, 0);
      s1125locals.ne555_voltage = 0;
      timer_adjust(s1125locals.ne555, states[state][4], 0, states[state][4]);
    }
    SN76477_enable_w       (0, 0);
  }
}

static void zac1125_init(struct sndbrdData *brdData) {
  /* MIXER A & C = GND */
  SN76477_mixer_w(0, 0);
  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(0, 1);
  /* fake: pulse the enable line to get rid of the constant noise */
//  SN76477_enable_w(0, 1);
//  SN76477_enable_w(0, 0);
  s1125locals.ne555 = timer_alloc(ne555_timer);
}

/*----------------------------------------
/ Zaccaria Sound Board 1346
/ i8035 MCU, no PIAs
/ and sound board 1146
/ with additional SN76477 chip used on Locomotion only
/-----------------------------------------*/
static void sp_init(struct sndbrdData *brdData);
static void sp_diag(int button);
static WRITE_HANDLER(sp1346_data_w);

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf zac1346Intf = {
  "ZAC1346", sp_init, NULL, sp_diag, sp1346_data_w, sp1346_data_w, NULL, NULL, NULL, 0
};
static struct DACinterface sp1346_dacInt = { 1, { 25 }};

static struct {
  struct sndbrdData brdData;
  int lastcmd;
  int tc;
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
}

static void sp_diag(int button) {
  cpu_set_irq_line(ZACSND_CPUA, 0, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(sp1346_data_w) {
  logerror("Sound %x plays\n", data);
  splocals.tc = data ? -1 : 0;
  i8035_set_reg(I8035_TC, splocals.tc);

  if (Machine->drv->sound[1].sound_type) { // >0 means SN74677 & NE555 chips present
    static double res[8] = { 0, RES_M(4.7), RES_M(2), RES_K(820), RES_K(680), RES_K(560), RES_K(470), RES_K(330) };
    static int currentValue = 0;
    switch (data) {
      case 0:
        if (splocals.lastcmd == 0) {
          SN76477_mixer_w(0, 7);
          currentValue = 0;
        }
        break;
      case 1: // raise the frequency if value < 7
        if (currentValue < 7) currentValue++;
        break;
      case 2: // lower the frequency if value > 1
        if (currentValue > 1) currentValue--;
        break;
    }
    if (currentValue > 0) {
      SN76477_mixer_w(0, 4);
      SN76477_set_slf_res(0, res[currentValue]);
    }
    SN76477_enable_w(0, !currentValue);
    discrete_sound_w(1, data == 3); // enable the NE555 tone
  }

  splocals.lastcmd = data;
}

static MEMORY_READ_START(i8035_readmem)
  { 0x0000, 0x07ff, MRA_ROM },
  { 0x0800, 0x087f, MRA_RAM },
  { 0x0880, 0x08ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(i8035_writemem)
  { 0x0800, 0x087f, MWA_RAM },
MEMORY_END

static WRITE_HANDLER(ram_w) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  rom[0x800 + offset] = data;
}
static READ_HANDLER(ram_r) {
  UINT8 *rom = memory_region(ZACSND_CPUAREGION);
  return rom[0x800 + offset];
}
static READ_HANDLER(sp1346_data_r) { return (core_getDip(0) << 4) | splocals.lastcmd; }
static READ_HANDLER(test_r) { return splocals.tc; }
static WRITE_HANDLER(dac_w) { DAC_0_data_w(0, core_revbyte(data)); }

static PORT_READ_START(i8035_readport)
  { 0x00, 0x7f, ram_r },
  { 0x80, 0xff, sp1346_data_r },
  { I8039_t1, I8039_t1, test_r },
MEMORY_END

static PORT_WRITE_START(i8035_writeport)
  { 0x00, 0x7f, ram_w },
  { I8039_p1, I8039_p1, dac_w },
MEMORY_END

MACHINE_DRIVER_START(zac1346)
  MDRV_CPU_ADD_TAG("scpu", I8035, 6000000./15.) // 8035 has internal divider by 15!
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(i8035_readmem, i8035_writemem)
  MDRV_CPU_PORTS(i8035_readport, i8035_writeport)

  MDRV_SOUND_ADD(DAC, sp1346_dacInt)
  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

static struct SN76477interface zac1146_sn76477Int = { 1, { 30 }, /* mixing level */
/*                       pin description      */
  { RES_K(39)   },    /*  4  noise_res        */
  { RES_K(100)  },    /*  5  filter_res       */
  { CAP_P(470)  },    /*  6  filter_cap       */
  { RES_M(1)    },    /*  7  decay_res        */
  { CAP_U(22)   },    /*  8  attack_decay_cap */
  { RES_K(4.7)  },    /* 10  attack_res       */
  { RES_K(47)   },    /* 11  amplitude_res    */
  { RES_K(87.7) },    /* 12  feedback_res     */
  { 0           },    /* 16  vco_voltage      */
  { 0           },    /* 17  vco_cap          */
  { 0           },    /* 18  vco_res          */
  { 5.0         },    /* 19  pitch_voltage    */
  { 0           },    /* 20  slf_res          */
  { CAP_N(220)  },    /* 21  slf_cap          */
  { CAP_U(1)    },    /* 23  oneshot_cap      */
  { RES_K(330)  }     /* 24  oneshot_res      */
};

static int type[1] = {0};
DISCRETE_SOUND_START(zac1146_discInt)
  DISCRETE_INPUT(NODE_01,1,0x0003,0)
  DISCRETE_555_ASTABLE(NODE_10,NODE_01,12.0,RES_K(1),RES_K(56),CAP_N(10),NODE_NC,type)
  DISCRETE_GAIN(NODE_20,NODE_10,1250)
  DISCRETE_OUTPUT(NODE_20, 50)
DISCRETE_SOUND_END

MACHINE_DRIVER_START(zac1146)
  MDRV_IMPORT_FROM(zac1346)
  MDRV_SOUND_ADD(SN76477, zac1146_sn76477Int)
  MDRV_SOUND_ADD(DISCRETE, zac1146_discInt)
MACHINE_DRIVER_END

/*----------------------------------------
/ Zaccaria Sound & Speech Board 1B1370
/ (almost identical to BY61, TMS5200 chip)
/ and Zaccaria Sound & Speech Board 1B13136
/ (like 1B1370, with an additional 6802 CPU, DAC, and AY8910 chip).
/ It's reported that the sound board is missing the 2nd CPU & ROM chips,
/ so in reality it acts exactly like the older 1370 board
/ with a slightly different memory map (but it can be integrated).
/-----------------------------------------*/
#define SNS_PIA0 0
#define SNS_PIA1 1
#define SNS_PIA2 2

#define SW_TRUE 1

static void sns_init(struct sndbrdData *brdData);
static void sns_diag(int button);
static WRITE_HANDLER(sns_data_w);
static void sns_5220Irq(int state);
static void sns_5220Rdy(int state);
static READ_HANDLER(sns_8910a_r);
static WRITE_HANDLER(sns_8910b_w);
static READ_HANDLER(sns2_8910a_r);
static WRITE_HANDLER(sns_dac_w);

const struct sndbrdIntf zac1370Intf = {
  "ZAC1370", sns_init, NULL, sns_diag, sns_data_w, sns_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static struct TMS5220interface sns_tms5220Int = { 640000, 75, sns_5220Irq, sns_5220Rdy }; // the frequency may vary by up to 30 percent!!!
static struct DACinterface     sns_dacInt = { 1, { 20 }};
static struct DACinterface     sns2_dacInt = { 2, { 20, 20 }};
static struct AY8910interface  sns_ay8910Int = { 1, 3579545/4, {25}, {sns_8910a_r}, {0}, {0}, {sns_8910b_w}};
static struct AY8910interface  sns2_ay8910Int = { 2, 3579545/4, {25, 25}, {sns_8910a_r, sns2_8910a_r}, {0}, {0}, {sns_8910b_w}};

static MEMORY_READ_START(sns_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNS_PIA0) },
  { 0x0084, 0x0087, pia_r(SNS_PIA2) }, // 13136 only
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x1800, 0x1800, sns2_8910a_r }, // 13136 only
  { 0x2000, 0x2000, sns_8910a_r },
  { 0x7000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNS_PIA0) },
  { 0x0084, 0x0087, pia_w(SNS_PIA2) }, // 13136 only
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x1000, 0x1000, sns_dac_w },
  { 0x4000, 0x4000, DAC_1_data_w }, // 13136 only (never accessed)
MEMORY_END

static MACHINE_DRIVER_START(zac1370_nosound)
  MDRV_CPU_ADD(M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns_readmem, sns_writemem)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac1370)
  MDRV_IMPORT_FROM(zac1370_nosound)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(AY8910,  sns_ay8910Int)
  MDRV_SOUND_ADD(DAC,     sns_dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac13136)
  MDRV_IMPORT_FROM(zac1370_nosound)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(AY8910,  sns2_ay8910Int)
  MDRV_SOUND_ADD(DAC,     sns2_dacInt)
MACHINE_DRIVER_END

static READ_HANDLER(sns_pia0a_r);
static WRITE_HANDLER(sns_pia0a_w);
static WRITE_HANDLER(sns_pia0b_w);
static WRITE_HANDLER(sns_pia0ca2_w);
static WRITE_HANDLER(sns_pia0cb2_w);
static READ_HANDLER(sns_pia1a_r);
static WRITE_HANDLER(sns_pia1a_w);
static WRITE_HANDLER(sns_pia1b_w);
static READ_HANDLER(sns_pia1ca1_r);
static READ_HANDLER(sns_pia1ca2_r);
static READ_HANDLER(sns_pia1cb1_r);
static READ_HANDLER(sns_pia2a_r);
static WRITE_HANDLER(sns_pia2a_w);
static WRITE_HANDLER(sns_pia2b_w);
static WRITE_HANDLER(sns_pia2ca2_w);

static void sns_irq0a(int state);
static void sns_irq0b(int state);
static void sns_irq1a(int state);
static void sns_irq1b(int state);

static void startcem3374(int param);
static void stopcem3374(int param);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1cb1, pia1ca1, pia1ca2, pia2a, pia2b;
  UINT8 lastcmd, daclatch, dacbyte1, dacbyte2;
  int dacMute,dacinp,channel;
  UINT8 snot_ab1, snot_ab2, snot_ab3, snot_ab4; // output from ls139 2d
  UINT8 s_ensynca,s_ensawb,s_entrigb,s_enpwma,s_ensawa,s_entriga,s_refsel,s_dacsh; // output from ls259 3h
  UINT8 s_inh4,s_inh3,s_inh2,s_inh1,s_envca,s_ensyncb; // output from ls259 3I
  int vcrfreq,rescntl,levchb,pwmb,freqb,levcha,pwma,freqa,r500cmd;
  int tmsPitch;
  mame_timer *fadeTimer;
  int vola, volb;
} snslocals;

static READ_HANDLER(sns_pia1b_r) {
  return pia_1_portb_r(0);
}

static const struct pia6821_interface sns_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia0a_r, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ sns_pia0a_w, sns_pia0b_w, sns_pia0ca2_w, sns_pia0cb2_w,
  /*irq: A/B           */ sns_irq0a, sns_irq0b
},{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia1a_r, sns_pia1b_r, sns_pia1ca1_r, sns_pia1cb1_r, sns_pia1ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ sns_pia1a_w, sns_pia1b_w, 0, 0,
  /*irq: A/B           */ sns_irq1a, sns_irq1b
},{
  /*i: A/B,CA/B1,CA/B2 */ sns_pia2a_r, 0, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ sns_pia2a_w, sns_pia2b_w, sns_pia2ca2_w, 0,
  /*irq: A/B           */ 0, 0
}};

static void fade_timer(int param) {
  int dec = snslocals.rescntl / 16;
  if (!dec) dec = 25; // fade veeery slowly on 0 value :)
  else if (dec < 50) dec = 50; // slowest sensible fading speed, apart from 0
  if (snslocals.vola > dec) {
    snslocals.vola -= dec;
  } else {
    snslocals.vola = 0;
    // TODO: we need to tell the main board the sound has finished; no idea how it's really done (PIA output B never changes)!?
    if (!snslocals.vola && !snslocals.volb) UpdateZACSoundACT(0x03);
  }
  mixer_set_volume(snslocals.channel,   snslocals.vola * 100 / 0xfff);
  mixer_set_volume(snslocals.channel+1, snslocals.vola * 100 / 0xfff);
  if (snslocals.volb > dec) {
    snslocals.volb -= dec;
  } else {
    snslocals.volb = 0;
    if (!snslocals.vola && !snslocals.volb) UpdateZACSoundACT(0x03);
  }
  mixer_set_volume(snslocals.channel+2, snslocals.volb * 100 / 0xfff);
  mixer_set_volume(snslocals.channel+3, snslocals.volb * 100 / 0xfff);
//printf("\r\r\r\r\r\r\r\r\r\r\r%03x:%03x:%03x", snslocals.rescntl, snslocals.vola, snslocals.volb);
}

static void initTMS(void) {
  tms5220_reset();
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370
    || core_gameData->hw.soundBoard == SNDBRD_ZAC11178
    || !strncasecmp(Machine->gamedrv->name, "tmac", 4)
    || !strncasecmp(Machine->gamedrv->name, "zankor", 6)) {
    // Pinball Champ ('82), Soccer Kings, Time Machine, Pool Champion, Black Belt, Mexico 86, Zankor
    tms5220_set_variant(TMS5220_IS_5200);
  } else {
    // Farfalla, Devil Riders, Magic Castle, Robot, Spooky, (New) Star's Phoenix, Thunder Man
    tms5220_set_variant(TMS5220_IS_5220C);
  }
  if (core_gameData->hw.soundBoard & 0x02) {
    snslocals.fadeTimer = timer_alloc(fade_timer);
    timer_adjust(snslocals.fadeTimer, TIME_IN_MSEC(25), 0, TIME_IN_MSEC(25));
  }
}

static void sns_init(struct sndbrdData *brdData) {
  snslocals.brdData = *brdData;

  pia_config(SNS_PIA0, PIA_STANDARD_ORDERING, &sns_pia[0]);
  pia_config(SNS_PIA1, PIA_STANDARD_ORDERING, &sns_pia[1]);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13136) {
    snslocals.pia1a = 0xff;
  }
  pia_config(SNS_PIA2, PIA_STANDARD_ORDERING, &sns_pia[2]);
  // reset tms5220
  snslocals.tmsPitch = -1;
  initTMS();
}

static void sns_diag(int button) {
  if (core_gameData->hw.soundBoard & 0x02) return; // ignore 11178 - too bad subboards can't have their own sndbrdIntf!
  cpu_set_nmi_line(ZACSND_CPUA, button ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(sns_dac_w) {
  if (core_gameData->hw.soundBoard != SNDBRD_ZAC1370 || !snslocals.dacMute) DAC_0_data_w(offset, data);
}

static void writeToCEM(void) {
  int changed = 0;
  switch (pia_1_portb_r(0) >> 5) {
    case 0:
      changed = (snslocals.freqa != snslocals.dacinp);
      snslocals.freqa = snslocals.dacinp;
      break;
    case 1:
      changed = (snslocals.pwma != snslocals.dacinp);
      snslocals.pwma = snslocals.dacinp;
      break;
    case 2:
      snslocals.levcha = snslocals.dacinp;
      break;
    case 3:
      changed = (snslocals.freqb != snslocals.dacinp);
      snslocals.freqb = snslocals.dacinp;
      break;
    case 4:
      changed = (snslocals.pwmb != snslocals.dacinp);
      snslocals.pwmb = snslocals.dacinp;
      break;
    case 5:
      snslocals.levchb = snslocals.dacinp;
      break;
    case 6:
      snslocals.rescntl = snslocals.dacinp;
      break;
    case 7:
      changed = (snslocals.vcrfreq != snslocals.dacinp);
      snslocals.vcrfreq = snslocals.dacinp;
  }
  if (changed) startcem3374(0);
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
  if ((data & 0xf0) != (snslocals.pia0b & 0xf0)) logerror("DAC1408 modulation: %x\n", data >> 4);
  snslocals.pia0b = data;
  if (snslocals.pia0b & 0x02) AY8910Write(0, snslocals.pia0b ^ 0x01, snslocals.pia0a);
}
static WRITE_HANDLER(sns_pia0ca2_w) {
  UpdateZACSoundLED(1, data);
} // diag led
static WRITE_HANDLER(sns_pia0cb2_w) {
  snslocals.dacMute = data;
} // mute DAC

static READ_HANDLER(sns_pia1a_r) {
  logerror("%04x:sns_pia1a_r %02x\n", activecpu_get_previouspc(),snslocals.pia1a);
  return snslocals.pia1a;
}
static WRITE_HANDLER(sns_pia1a_w) {
  logerror("%04x:sns_pia1a_w %02x\n", activecpu_get_previouspc(),data);
  snslocals.pia1a = data;
}
static WRITE_HANDLER(sns_pia1b_w) {
  logerror("%04x:sns_pia1b_w %02x\n", activecpu_get_previouspc(),data);
  snslocals.r500cmd = 0;

  if (pia_1_portb_r(0) & ~data & 0x01) { // read, overrides write command!
    // Port A is assigned in sns_pia1ca2_r, otherwise speech may be garbled
    snslocals.r500cmd = 1;
  } else if (pia_1_portb_r(0) & ~data & 0x02) { // write
    tms5220_data_w(0, snslocals.pia1a);
  }
  if (!(core_gameData->hw.soundBoard & 0x02) && (data & 0xf0) != (pia_1_portb_r(0) & 0xf0)) logerror("TMS5200 modulation: %x\n", data >> 4);
  pia_set_input_b(SNS_PIA1, data);
  pia_set_input_ca2(SNS_PIA1, tms5220_ready_r());

  if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178) {
    UpdateZACSoundACT(data & 0x04 ? 0 : 0x03);  //both ACTSND & ACTSPK inverted on bit 2
  } else if (core_gameData->hw.soundBoard == SNDBRD_ZAC11178_13181) {
    UpdateZACSoundACT(data & 0x04 ? 0 : 0x02);  //only ACTSND inverted on bit 2, ACTSPK not used?
//    logerror("%04x:UpdateZACSoundACT %x\n", activecpu_get_previouspc(),data);
  } else {
    UpdateZACSoundACT((data>>2) & 0x03);    //ACTSPK & ACTSND on bits 2 & 3
  }
  // set ab1 to ab4 switches based on pb3 & pb4 IC 2d
  if (core_gameData->hw.soundBoard & 0x02) { // true for all 11178
    snslocals.snot_ab4 = snslocals.snot_ab3 = snslocals.snot_ab2 = snslocals.snot_ab1 = 0;
    if ((data & 0x08) && (data & 0x10)) snslocals.snot_ab4 = SW_TRUE;   // pb3 and pb4 set -> snot_ab4 true
    if ((~data & 0x08) && (data & 0x10)) snslocals.snot_ab3 = SW_TRUE;
    if ((data & 0x08) && (~data & 0x10)) snslocals.snot_ab2 = SW_TRUE;
    if ((~data & 0x08) && (~data & 0x10)) snslocals.snot_ab1 = SW_TRUE;
//    logerror("sns_pia1b_w: data %x snot_ab4 %x snot_ab3 %x snot_ab2 %x snot_ab1 %x \n", data,snslocals.snot_ab4,snslocals.snot_ab3,snslocals.snot_ab2,snslocals.snot_ab1);
  }
}
static READ_HANDLER(sns_pia1ca1_r) {
  return snslocals.pia1ca1;
}
static READ_HANDLER(sns_pia1ca2_r) {
//  logerror("sns_pia1ca2_r TMS5220 ready %x\n", snslocals.pia1ca2);
// oliver
  if (snslocals.r500cmd) snslocals.pia1a = tms5220_status_r(0);
  return snslocals.pia1ca2;
}
static READ_HANDLER(sns_pia1cb1_r) {
  if (core_gameData->hw.soundBoard & 0x02) // true for all 11178
    return snslocals.pia1cb1;
  else
    return tms5220_int_r();
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
  snslocals.lastcmd = data;

  if ((core_gameData->hw.gameSpecific2 & 1) && (data & 0x80)) // some of the speech would be garbled otherwise!
    initTMS();

  switch (core_gameData->hw.soundBoard) {
    case SNDBRD_ZAC1370:
      pia_set_input_cb1(SNS_PIA0, data & 0x80 ? 1 : 0);
      break;
    case SNDBRD_ZAC13136:
      cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
      break;
    case SNDBRD_ZAC11178:
      snslocals.pia1ca1 = data & 0x80 ? 1 : 0;
      pia_set_input_ca1(SNS_PIA1, snslocals.pia1ca1);
      break;
    case SNDBRD_ZAC11178_13181:
      snslocals.pia1ca1 = data & 0x80 ? 1 : 0;
      pia_set_input_ca1(SNS_PIA1, snslocals.pia1ca1); // CA1 should connect to GND according to schematics... or to DB7! :)
//      logerror("sns_data_w command stored %x\n", data);
      // cpu reads command from adress b0 after nmi !!!
      if (!(data & 0x40)) cpu_set_nmi_line(ZACSND_CPUB, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
      break;
    case SNDBRD_ZAC13181x3:
      if (!(data & 0x40)) cpu_set_nmi_line(ZACSND_CPUA, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
      if (!(data & 0x40)) cpu_set_nmi_line(ZACSND_CPUB, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
      if (data & 0x40) cpu_set_nmi_line(ZACSND_CPUC, data & 0x80 ? ASSERT_LINE : CLEAR_LINE);
  }
}

static READ_HANDLER(sns_8910a_r) { return ~snslocals.lastcmd; }

static WRITE_HANDLER(sns_8910b_w) {
  static UINT8 lastAy = 0;
  if ((lastAy & 0x0f) != (data & 0x0f)) logerror("AY8910  modulation: %x\n", data & 0x0f);
  lastAy = data;
}

static READ_HANDLER(sns2_8910a_r) { return ~snslocals.lastcmd; }

static void sns_irq0a(int state) {
//  logerror("sns_irq0a: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void sns_irq0b(int state) {
//  logerror("sns_irq0b: state=%x\n",state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static int timerSet;
static void fireNmi(int dummy) {
  if (timerSet) {
    cpu_set_nmi_line(ZACSND_CPUA, PULSE_LINE);
  }
}

static void sns_irq1a(int state) {
//  logerror("%d:%04x:sns_irq1a: state=%x\n",cpu_getactivecpu(),activecpu_get_previouspc(),state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
  if (core_gameData->hw.soundBoard & 0x02) { // true for 11178
  	if (state) { // IRQA is set to 1 and back to 0 within 6 CPU cycles, but the NMI line assertion must be delayed slightly and must be cancelable
      timerSet = 1;
      timer_set(TIME_IN_CYCLES(5, ZACSND_CPUA), 0, fireNmi);
    } else {
      timerSet = 0;
    }
  }
}
static void sns_irq1b(int state) {
//  logerror("%d:%04x:sns_irq1b: state=%x\n",cpu_getactivecpu(),activecpu_get_previouspc(),state);
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370 || core_gameData->hw.soundBoard & 0x02)
    cpu_set_irq_line(ZACSND_CPUA, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void sns_5220Irq(int state) {
  if ((core_getDip(0) >> 4) != snslocals.tmsPitch)
    tms5220_set_frequency((93 + (snslocals.tmsPitch = (core_getDip(0) >> 4))) * 6666.66666666666); // 0:620kHz .. 15:720kHz
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370 || core_gameData->hw.soundBoard == SNDBRD_ZAC13136)
    pia_set_input_cb1(SNS_PIA1, !state);
//  logerror("sns_5220Irq: state=%x\n",state);
}
static void sns_5220Rdy(int state) {
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC1370 || core_gameData->hw.soundBoard == SNDBRD_ZAC13136 || core_gameData->hw.soundBoard & 0x02)
    pia_set_input_ca2(SNS_PIA1, (snslocals.pia1ca2 = state));
//  logerror("sns_5220Rdy: state=%x\n",state);
}

/*----------------------------------------
/ Sound & Speech board 1B11178.
/ Uses only one PIA controlling the TMS chip and the rest of the board,
/ also carries a single DAC and some analog VCO components (CEM 3372/3374)
/ to replace the AY8910 chip.
/-----------------------------------------*/
// This is the E signal, divided by a 4040 chip's Q11, so 3.58 MHz / 4 / 2048, so ~437 Hz
#define SNS_11178_IRQFREQ 3579545./8192.

static INTERRUPT_GEN(sns3_irq);
static WRITE_HANDLER(dacxfer);
static WRITE_HANDLER(storelatch);
static WRITE_HANDLER(storebyte1);
static WRITE_HANDLER(storebyte2);
static READ_HANDLER(readlatch);
static READ_HANDLER(readcmd);
static WRITE_HANDLER(chip3h259);
static WRITE_HANDLER(chip3i259);

static const UINT8 triangleWave[] = {
  0x00, 0x07, 0x0f, 0x17, 0x1f, 0x27, 0x2f, 0x37, //    *
  0x3f, 0x47, 0x4f, 0x57, 0x5f, 0x67, 0x6f, 0x77, //   * *
  0x7f, 0x77, 0x6f, 0x67, 0x5f, 0x57, 0x4f, 0x47, //  *   *
  0x3f, 0x37, 0x2f, 0x27, 0x1f, 0x17, 0x0f, 0x07, // *     *
  0x00, 0xf9, 0xf1, 0xe9, 0xe1, 0xd9, 0xd1, 0xc9, //        *     *
  0xc1, 0xb9, 0xb1, 0xa9, 0xa1, 0x99, 0x91, 0x89, //         *   *
  0x81, 0x89, 0x91, 0x99, 0xa1, 0xa9, 0xb1, 0xb9, //          * *
  0xc1, 0xc9, 0xd1, 0xd9, 0xe1, 0xe9, 0xf1, 0xf9  //           *
};
static UINT8 triangleWaver[64];

static const UINT8 sawtoothWave[] = {
  0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e, //       *
  0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e, //     * *
  0x42, 0x46, 0x4a, 0x4e, 0x52, 0x57, 0x5b, 0x5f, //   *   *
  0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f, // *     *
  0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d, //       *     *
  0xa1, 0xa5, 0xa9, 0xae, 0xb2, 0xb6, 0xba, 0xbe, //       *   *
  0xc2, 0xc6, 0xca, 0xce, 0xd2, 0xd6, 0xda, 0xde, //       * *
  0xe2, 0xe6, 0xea, 0xee, 0xf2, 0xf6, 0xfa, 0xfe  //       *
};
static UINT8 sawtoothWaver[64];

static int sns_sh_start(const struct MachineSound *msound) {
  UINT8 i;
  int mixing_levels[4] = {MIXER(15,MIXER_PAN_LEFT),MIXER(15,MIXER_PAN_LEFT),MIXER(15,MIXER_PAN_RIGHT),MIXER(15,MIXER_PAN_RIGHT)};
  for (i=0; i < 64; i++) {  // reverse waves
    triangleWaver[63-i]=triangleWave[i];
    sawtoothWaver[63-i]=sawtoothWave[i];
  }
  UpdateZACSoundLED(1, 1);
  if (!snslocals.channel) {
    // allocate channels
    snslocals.channel = mixer_allocate_channels(4, mixing_levels);
    mixer_set_name  (snslocals.channel,   "CEM 3374 A TR");
    mixer_set_name  (snslocals.channel+1, "CEM 3374 A SA");
    mixer_set_name  (snslocals.channel+2, "CEM 3374 B TR");
    mixer_set_name  (snslocals.channel+3, "CEM 3374 B SA");
  }
  return 0;
}

struct CustomSound_interface sns_custInt = { sns_sh_start };

static MEMORY_READ_START(sns3_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0090, 0x0093, pia_r(SNS_PIA1) },
  { 0x00b0, 0x00bf, readcmd},
  { 0x00f0, 0x00ff, readlatch },
  { 0x5000, 0x5000, MRA_NOP },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(sns3_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0087, chip3h259 },
  { 0x0090, 0x0093, pia_w(SNS_PIA1) },
  { 0x00a0, 0x00a7, chip3i259 },
  { 0x00c0, 0x00cf, dacxfer },
  { 0x00d0, 0x00df, storebyte1 },
  { 0x00e0, 0x00ef, storebyte2 },
  { 0x00f0, 0x00ff, storelatch },
MEMORY_END

MACHINE_DRIVER_START(zac11178)
  MDRV_CPU_ADD(M6802, 3579545./4.)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(sns3_readmem, sns3_writemem)
  MDRV_CPU_PERIODIC_INT(sns3_irq, SNS_11178_IRQFREQ)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(CUSTOM,  sns_custInt)
//  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static void stopcem3374(int param) {
  if (mixer_is_sample_playing(snslocals.channel))
    mixer_stop_sample(snslocals.channel);
  if (mixer_is_sample_playing(snslocals.channel+1))
    mixer_stop_sample(snslocals.channel+1);
  if (mixer_is_sample_playing(snslocals.channel+2))
    mixer_stop_sample(snslocals.channel+2);
  if (mixer_is_sample_playing(snslocals.channel+3))
    mixer_stop_sample(snslocals.channel+3);
}

static void startcem3374(int param) {
  signed char *triWave = param ? (signed char *)triangleWaver : (signed char *)triangleWave;
  signed char *sawWave = param ? (signed char *)sawtoothWaver : (signed char *)sawtoothWave;
  int triSize = param ? sizeof(triangleWaver) : sizeof(triangleWave);
  int sawSize = param ? sizeof(sawtoothWaver) : sizeof(sawtoothWave);

  int freqa = snslocals.s_entriga || snslocals.s_ensawa || snslocals.pwma || snslocals.s_ensyncb ? ((snslocals.s_enpwma ? snslocals.pwma : 0) + snslocals.freqa * snslocals.freqa) / 125 + snslocals.vcrfreq / 2 : 0;
  int freqb = snslocals.s_entrigb || snslocals.s_ensawb || snslocals.pwmb || snslocals.s_ensynca ? (snslocals.pwmb + snslocals.freqb * snslocals.freqb) / 125 + snslocals.vcrfreq / 2 : 0;

  if (snslocals.s_entriga || snslocals.s_ensyncb) {
    if (mixer_is_sample_playing(snslocals.channel))
      mixer_set_sample_frequency(snslocals.channel, snslocals.s_ensyncb ? (freqa + freqb) / 2 : freqa);
    else
      mixer_play_sample(snslocals.channel, (signed char *)triangleWave, sizeof(triangleWave), snslocals.s_ensyncb ? (freqa + freqb) / 2 : freqa, 1);
  } else
    mixer_stop_sample(snslocals.channel);

  if (snslocals.s_ensawa || snslocals.s_enpwma || snslocals.s_ensyncb) {
    if (mixer_is_sample_playing(snslocals.channel+1))
      mixer_set_sample_frequency(snslocals.channel+1, snslocals.s_ensyncb ? (freqa + freqb) / 2 : freqa);
    else
      mixer_play_sample(snslocals.channel+1, (signed char *)sawtoothWave, sizeof(sawtoothWave), snslocals.s_ensyncb ? (freqa + freqb) / 2 : freqa, 1);
  } else
    mixer_stop_sample(snslocals.channel+1);

  if (snslocals.s_entrigb || snslocals.s_ensynca) {
    if (mixer_is_sample_playing(snslocals.channel+2))
      mixer_set_sample_frequency(snslocals.channel+2, snslocals.s_ensynca ? (freqa + freqb) / 2 : freqb);
    else
      mixer_play_sample(snslocals.channel+2, triWave, triSize, snslocals.s_ensynca ? (freqa + freqb) / 2 : freqb, 1);
  } else
    mixer_stop_sample(snslocals.channel+2);

  if (snslocals.s_ensawb || snslocals.pwmb || snslocals.s_ensynca) {
    if (mixer_is_sample_playing(snslocals.channel+3))
      mixer_set_sample_frequency(snslocals.channel+3, snslocals.s_ensynca ? (freqa + freqb) / 2 : freqb);
    else
      mixer_play_sample(snslocals.channel+3, sawWave, sawSize, snslocals.s_ensynca ? (freqa + freqb) / 2 : freqb, 1);
  } else
    mixer_stop_sample(snslocals.channel+3);
//printf("FREQA:%03x PWMA:%03x LEVA:%03x FREQB:%03x PWMB:%03x LEVB:%03x RESCTRL:%03x VCRFREQ:%03x ", snslocals.freqa, snslocals.pwma, snslocals.levcha, snslocals.freqb, snslocals.pwmb, snslocals.levchb, snslocals.rescntl, snslocals.vcrfreq);
//printf("DACSH:%x REFSEL:%x TRIGA:%x SAWA:%x PWMA:%x TRIGB:%x SAWB:%x SYNCA:%x SYNCB:%x VCA:%x ", snslocals.s_dacsh, snslocals.s_refsel, snslocals.s_entriga, snslocals.s_ensawa, snslocals.s_enpwma, snslocals.s_entrigb, snslocals.s_ensawb, snslocals.s_ensynca, snslocals.s_ensyncb, snslocals.s_envca);
//printf("INH1:%x INH2:%x INH3:%x INH4:%x\n", snslocals.s_inh1, snslocals.s_inh2, snslocals.s_inh3, snslocals.s_inh4);
}

static INTERRUPT_GEN(sns3_irq) {
  snslocals.pia1cb1 = !snslocals.pia1cb1;
  pia_set_input_cb1(SNS_PIA1, snslocals.pia1cb1);
#ifdef MAME_DEBUG
  if (keyboard_pressed_memory_repeat(KEYCODE_B, 30)) UpdateZACSoundACT(0x03);
#endif
}

// OK: the following addresses are only used by the 11178 sound board variants
static READ_HANDLER(readcmd) {
  logerror("%04x:readcmd: %02x\n",activecpu_get_previouspc(), snslocals.lastcmd ^ 0xff);
  return snslocals.lastcmd ^ 0xff;
}
static READ_HANDLER(readlatch) {
  return snslocals.daclatch;
}
static WRITE_HANDLER(storelatch) {
  snslocals.daclatch = data;
}
static WRITE_HANDLER(storebyte1) {
  snslocals.dacbyte1 = snslocals.daclatch;
}
static WRITE_HANDLER(storebyte2) {
  snslocals.dacbyte2 = snslocals.daclatch;
}
static WRITE_HANDLER(dacxfer) {
  static int ignoreNext;
// this dac uses 12 bits, so a 16 bit dac must be used...
// ok dac input: first byte (msb bit) second byte (only 4 bits are used and contains lsb bit)
  UINT16 val = (snslocals.dacbyte1 << 4) | (snslocals.dacbyte2 >> 4);
// TODO: find out why there is an erratic write between registers 0 and 7?!
//printf("%x%d:%03x ", snslocals.s_inh1, pia_1_portb_r(0) >> 5, val);
  if (ignoreNext) {
    ignoreNext = 0;
//printf("\n");
    if (!snslocals.s_inh1 || snslocals.s_inh2 || snslocals.s_inh3) {
      stopcem3374(0);
    }
    return;
  }
  if (pia_1_portb_r(0) >> 5 == 0) ignoreNext = 1;
  snslocals.dacinp = val;
  writeToCEM();
}

static WRITE_HANDLER(chip3h259) {
  int changed = 0;
  int flag = (data | ~pia_1_portb_r(0)) & 1;
  if (snslocals.snot_ab1) {     // Enable is logic low, latch adressable
    switch (offset) {
      case 0:
        changed = (snslocals.s_dacsh != flag);
        snslocals.s_dacsh = flag;
        break;
      case 1:
        changed = (snslocals.s_refsel != flag);
        snslocals.s_refsel = flag;
        break;
      case 2:
        changed = (snslocals.s_entriga != flag);
        if (changed) snslocals.vola = snslocals.levcha;
        snslocals.s_entriga = flag;
        break;
      case 3:
        changed = (snslocals.s_ensawa != flag);
        if (changed) snslocals.vola = snslocals.levcha;
        snslocals.s_ensawa = flag;
        break;
      case 4:
        changed = (snslocals.s_enpwma != flag);
        snslocals.s_enpwma = flag;
        break;
      case 5:
        changed = (snslocals.s_entrigb != flag);
        if (changed) snslocals.volb = snslocals.levchb;
        snslocals.s_entrigb = flag;
        break;
      case 6:
        changed = (snslocals.s_ensawb != flag);
        if (changed) snslocals.volb = snslocals.levchb;
        snslocals.s_ensawb = flag;
        break;
      case 7:
        changed = (snslocals.s_ensynca != flag);
        if (changed) snslocals.volb = snslocals.levchb;
        snslocals.s_ensynca = flag;
    }
    if (changed) startcem3374(0);
//    logerror("%04x:chip3h259: offset %x data %x s_dacsh %x s_refsel %x s_entriga %x s_ensawa %x s_enpwma %x s_entrigb %x s_ensawb %x s_ensynca %x \n",activecpu_get_previouspc(),offset,data,snslocals.s_dacsh,snslocals.s_refsel,snslocals.s_entriga,snslocals.s_ensawa,snslocals.s_enpwma ,snslocals.s_entrigb,snslocals.s_ensawb,snslocals.s_ensynca);
  }
}

static WRITE_HANDLER(chip3i259) {
  static int lastinh1;
  int changed = 0;
  int flag = (data | ~pia_1_portb_r(0)) & 1;
  if (snslocals.snot_ab1) {     // Enable is logic low, latch adressable
    switch (offset) {
      case 0:
        changed = (snslocals.s_ensyncb != flag);
        if (changed) snslocals.vola = snslocals.levcha;
        snslocals.s_ensyncb = flag;
        break;
      case 1:
        snslocals.s_envca = flag;
        break;
      case 2:
        snslocals.s_inh1 = flag;
        if (flag && lastinh1) {
          changed = 1;
          snslocals.vola = snslocals.levcha;
          snslocals.volb = snslocals.levchb;
        }
        lastinh1 = flag;
        break;
      case 3:
        changed = (snslocals.s_inh2 != flag);
        snslocals.s_inh2 = flag;
        if (changed) {
          snslocals.vola = snslocals.levcha;
          snslocals.volb = snslocals.levchb;
        }
        break;
      case 4:
        changed = (snslocals.s_inh3 != flag);
        snslocals.s_inh3 = flag;
        if (changed) {
          snslocals.vola = snslocals.levcha;
          snslocals.volb = snslocals.levchb;
        }
        break;
      case 5:
        changed = (snslocals.s_inh4 != flag);
        if (changed) {
          snslocals.vola = snslocals.levcha;
          snslocals.volb = snslocals.levchb;
        }
        snslocals.s_inh4 = flag;
    }
    if (changed) startcem3374(0);
//    logerror("%04x:chip3i259: offset %x data %x s_ensyncb %x s_envca %x s_inh1 %x s_inh2 %x s_inh3 %x s_inh4 %x \n",activecpu_get_previouspc(),offset,data,snslocals.s_ensyncb,snslocals.s_envca,snslocals.s_inh1,snslocals.s_inh2,snslocals.s_inh3 ,snslocals.s_inh4);
  }
}


/*----------------------------------------
/ Sound & Speech board 1B11178 with 1B13181 / 1B11181 daughter board.
/ This daughter board is equipped with a Z80 processor, and
/ plays background music (all done with DACs).
/ The earlier 1B13181/0 single DAC version was used on Zankor only,
/ Spooky already has the full 3 DAC board 1B11181/1.
/
/ Also Sound & Speech board 1B11183 with three Z80 CPUs, used on Star's Phoenix.
/ One CPU does the exact same stuff like on the 1B11181 daughter board,
/ the 2nd CPU handles speech and effects, and the 3rd one produces
/ some siren-like effects (not played back in sound test).
/ That's five DAC chips playing at the same time! :)
/-----------------------------------------*/
static struct DACinterface     z80_1dacInt = { 1, { 50 }};
static struct DACinterface     z80_3dacInt = { 3, { 20, 30, 30 }};
static struct DACinterface     z80_5dacInt = { 5, { 20, 30, 30, 20, 20 }};

static MEMORY_READ_START(z80_readmem)
  { 0x0000, 0xfbff, MRA_ROM },
  { 0xfc00, 0xffff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(z80_writemem)
  { 0xfc00, 0xffff, MWA_RAM },
MEMORY_END

static WRITE_HANDLER(DAC_2_signed_data_w) { DAC_signed_data_w(2, data); }
static WRITE_HANDLER(DAC_3_signed_data_w) { DAC_signed_data_w(3, data); }
static WRITE_HANDLER(DAC_4_signed_data_w) { DAC_signed_data_w(4, data); }

static WRITE_HANDLER(snd_act_w) {
//  logerror("cpu #%d ACT:%d:%02x\n", cpu_getexecutingcpu(), offset, data);
  // ACTSPK & ACTSND
  UpdateZACSoundACT(data & 0x01 ? 0 : 0x02);    // only ACTSND inverted on bit 0, ACTSPK not used?
}

static WRITE_HANDLER(snd_mod_w) {
  // sound modulation for different chips maybe?
//  logerror("cpu #%d MOD:%d:%02x\n", cpu_getexecutingcpu(), offset, data);
}

static PORT_READ_START(z80_readport)
  { 0x01, 0x01, readcmd },
PORT_END

static PORT_WRITE_START(z80_writeport_a)
  { 0x00, 0x00, DAC_4_signed_data_w },
  { 0x02, 0x02, snd_mod_w },
PORT_END
static PORT_WRITE_START(z80_writeport_b)
  { 0x00, 0x00, DAC_0_signed_data_w },
  { 0x02, 0x03, snd_mod_w },
  { 0x04, 0x04, DAC_1_signed_data_w },
  { 0x08, 0x08, DAC_2_signed_data_w },
PORT_END

static PORT_READ_START(z80_readport_c)
  { 0x01, 0x01, readcmd },
  { 0x03, 0x03, tms5220_status_r },
PORT_END
static PORT_WRITE_START(z80_writeport_c)
  { 0x00, 0x00, DAC_3_signed_data_w },
  { 0x02, 0x02, snd_act_w },
  { 0x03, 0x03, tms5220_data_w },
PORT_END

static INTERRUPT_GEN(cpu_b_irq) { cpu_set_irq_line(ZACSND_CPUB, 0, PULSE_LINE); }
static INTERRUPT_GEN(cpu_c_irq) { cpu_set_irq_line(ZACSND_CPUC, 0, PULSE_LINE); }

static MACHINE_DRIVER_START(zac11178_13181_nodac)
  MDRV_IMPORT_FROM(zac11178)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_b)
  MDRV_CPU_PERIODIC_INT(cpu_b_irq, 60)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178_13181)
  MDRV_IMPORT_FROM(zac11178_13181_nodac)
  MDRV_SOUND_ADD(DAC,     z80_1dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11178_11181)
  MDRV_IMPORT_FROM(zac11178_13181_nodac)
  MDRV_SOUND_ADD(DAC,     z80_3dacInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(zac11183)
  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_a)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport_b)

  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport_c, z80_writeport_c)
  MDRV_CPU_PERIODIC_INT(cpu_c_irq, 60)

  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, sns_tms5220Int)
  MDRV_SOUND_ADD(DAC,     z80_5dacInt)
MACHINE_DRIVER_END
