#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m68000/m68000.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "by35.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "by35snd.h"

/*----------------------------------------
/              -32, -50 sound
/-----------------------------------------*/
static void by32_init(struct sndbrdData *brdData);
static WRITE_HANDLER(by32_cmd_w);
static int by32_sh_start(const struct MachineSound *msound);
static void by32_sh_stop(void);

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by32Intf = { by32_init, NULL, NULL, by32_cmd_w, NULL, NULL, NULL };
struct CustomSound_interface by32_custInt = {by32_sh_start, by32_sh_stop};

#define BY32_DECAYFREQ   50
#define BY32_EXPFACTOR   95   // %
#define BY32_PITCH      100   // 0-100%

/* waveform for the audio hardware */
static const UINT8 sineWave[] = {
  204,188,163,214,252,188,115,125,136,63,0,37,88,63,47,125
};

static struct {
  struct sndbrdData brdData;
  int volume, lastCmd, channel;
  void *vTimer;
} by32locals;

static void by32_decay(int param) {
  mixer_set_volume(by32locals.channel, by32locals.volume/10);
  if (by32locals.volume < 100) by32locals.volume = 0;
  else by32locals.volume = by32locals.volume * BY32_EXPFACTOR / 100;
}

static int by32_sh_start(const struct MachineSound *msound) {
  by32locals.channel = mixer_allocate_channel(15);
  mixer_set_volume(by32locals.channel,0);
  mixer_play_sample(by32locals.channel, (signed char *)sineWave, sizeof(sineWave), 1, 1);
  by32locals.vTimer = timer_pulse(TIME_IN_HZ(BY32_DECAYFREQ),0,by32_decay);
  return 0;
}

static void by32_sh_stop(void) {
  mixer_stop_sample(by32locals.channel);
  if (by32locals.vTimer) { timer_remove(by32locals.vTimer); by32locals.vTimer = NULL; }
}

static WRITE_HANDLER(by32_cmd_w) {
  data ^= 0x10;
  if (data == by32locals.lastCmd) return; /* Nothing has changed */

  if (data & 0x20) { /* sound is enabled -> change frequency */
    if ((data & 0x0f) != 0x0f) {
      UINT8 sData; int f;
      sData = core_revbyte(*(by32locals.brdData.romRegion + (data & 0x1f)));
      f= sizeof(sineWave)/((1.1E-6+BY32_PITCH*1E-8)*sData)/8;

      mixer_set_sample_frequency(by32locals.channel, f);
    }
    if ((by32locals.lastCmd & 0x20) == 0) by32locals.volume = 1000; /* positive edge */
  }
  else by32locals.volume = 0; /* Sound is disabled */
  by32locals.lastCmd = data;
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
static WRITE_HANDLER(sp51_cmd_w);
static WRITE_HANDLER(sp56_cmd_w);
static READ_HANDLER(sp_8910a_r);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by51Intf = { sp_init, NULL, NULL, sp51_cmd_w, NULL, NULL, NULL };
const struct sndbrdIntf by56Intf = { sp_init, NULL, NULL, sp56_cmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC };

struct AY8910interface sp_ay8910Int = { 1, 3580000/4, {10}, {sp_8910a_r} };
struct hc55516_interface sp_hc55516Int = { 1, {100}};
MEMORY_READ_START(sp51_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x1000, 0x1fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_READ_START(sp56_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_r(SP_PIA0) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(sp_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_w(SP_PIA0) },
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

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
  int lastcmd, currcmd, cmdout, cmd[2];
} splocals;

static void sp_init(struct sndbrdData *brdData) {
  splocals.brdData = *brdData;
  pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
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
static WRITE_HANDLER(sp51_cmd_w) {
  if ((data ^ splocals.lastcmd) & 0x20) {
    pia_set_input_ca1(SP_PIA0, data & 0x20);
    splocals.currcmd = ~data & 0x1f;
  }
  splocals.lastcmd = data;
}
/* collect two commands before passing to sound CPU */
static WRITE_HANDLER(sp56_cmd_w) {
  if (data & ~splocals.lastcmd & 0x20)
    splocals.currcmd = 0;
  else if ((data & 0x20) && splocals.currcmd < 2) {
    splocals.cmd[splocals.currcmd++] = ~data & 0x1f; snd_cmd_log(data);
    if (splocals.currcmd == 2) { /* two commands received */
      splocals.cmdout = 0;
      sndbrd_sync_w(pia_pulse_ca1, SP_PIA0, 0);
    }
  }
  splocals.lastcmd = data;
}

static READ_HANDLER(sp_8910a_r) {
  if (splocals.brdData.subType == 1) { // -56 board
    switch (splocals.cmdout) {
      case 1 : //pia_set_input_ca1(2, 0);
      case 0 : return splocals.cmd[splocals.cmdout++];
      default: return splocals.lastcmd;
    }
  }
  return splocals.currcmd;
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
/ CA2:    Self-test LED
/ CB1:    Sound interrupt
/ CB2:
/ IRQA, IRQB: CPU IRQ
/
/ PIA1: 0090
/  A: 0-7 TMS5200 D7-D0
/  B: 0   TMS5200 ReadStrobe
/     1   TMS5200 WriteStrobe
/     2-3 J3
/     4-7 Speech volume
/ CA1:    NC
/ CA2:    TMS5200 Ready
/ CB1:    TMS5200 Int
/ IRQA, IRQB: CPU IRQ
*/
#define SNT_PIA0 2
#define SNT_PIA1 3

static void snt_init(struct sndbrdData *brdData);
static WRITE_HANDLER(snt_cmd_w);
static void snt_5220Irq(int state);
static READ_HANDLER(snt_8910a_r);
/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf by61Intf = {
  snt_init, NULL, NULL, snt_cmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};
struct TMS5220interface snt_tms5220Int = { 640000, 50, snt_5220Irq };
struct DACinterface snt_dacInt = { 1, { 20 }};
struct AY8910interface snt_ay8910Int = { 1, 3580000/4, {25}, {snt_8910a_r}};

MEMORY_READ_START(snt_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(SNT_PIA0) },
  { 0x0090, 0x0093, pia_r(SNT_PIA1) },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(snt_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(SNT_PIA0) },
  { 0x0090, 0x0093, pia_w(SNT_PIA1) },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0xc000, 0xffff, MWA_ROM },
MEMORY_END

static READ_HANDLER(snt_pia0a_r);
static WRITE_HANDLER(snt_pia0a_w);
static WRITE_HANDLER(snt_pia0b_w);
static WRITE_HANDLER(snt_pia1a_w);
static WRITE_HANDLER(snt_pia1b_w);
static WRITE_HANDLER(snt_pia0ca2_w);
static void snt_irq(int state);

static struct {
  struct sndbrdData brdData;
  int pia0a, pia0b, pia1a, pia1b;
  int cmd[2], lastcmd, cmdsync, cmdout;
} sntlocals;
static const struct pia6821_interface snt_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia0a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ snt_pia0a_w, snt_pia0b_w, snt_pia0ca2_w, 0,
  /*irq: A/B           */ snt_irq, snt_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ snt_pia1a_w, snt_pia1b_w, 0, 0,
  /*irq: A/B           */ snt_irq, snt_irq
}};
static void snt_init(struct sndbrdData *brdData) {
  sntlocals.brdData = *brdData;
  pia_config(SNT_PIA0, PIA_STANDARD_ORDERING, &snt_pia[0]);
  pia_config(SNT_PIA1, PIA_STANDARD_ORDERING, &snt_pia[1]);
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
static WRITE_HANDLER(snt_pia1a_w) { sntlocals.pia1a = data; }
static WRITE_HANDLER(snt_pia1b_w) {
  if (sntlocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, sntlocals.pia1a);
    pia_pulse_ca2(SNT_PIA1, 1);
  }
  else if (sntlocals.pia1b & ~data & 0x01) { // read
    pia_set_input_a(SNT_PIA1, tms5220_status_r(0));
    pia_pulse_ca2(SNT_PIA1, 1);
  }
  sntlocals.pia1b = data;
}

/*-- pass command from main cpu to sound cpu --*/
/* The main CPU causes an IRQ and then sends two commands
   To sync the CPUs we wait until both commands has been sent
   before causing the IRQ. If the command is read outside
   the IRQ the result is undefined.
   I read somewhere that the main CPU creates sounds at the same
   time as solenoids are activated. This will probably not work.
*/
static WRITE_HANDLER(snt_cmd_w) {
  if (data & ~sntlocals.lastcmd & 0x20)
    sntlocals.cmdsync = 0;
  else if ((data & 0x20) && sntlocals.cmdsync < 2) {
    sntlocals.cmd[sntlocals.cmdsync++] = data; snd_cmd_log(data);
    if (sntlocals.cmdsync == 2) { /* two commands received */
      sntlocals.cmdout = 0; /* start reading commands */
      sndbrd_sync_w(pia_pulse_cb1, SNT_PIA0, 0);
    }
  }
  sntlocals.lastcmd = data;
}

static READ_HANDLER(snt_8910a_r) {
  int tmp = (sntlocals.cmdout < 2) ? sntlocals.cmd[sntlocals.cmdout++] :
                                     sntlocals.lastcmd;
  return ~tmp & 0x1f;
}

static WRITE_HANDLER(snt_pia0ca2_w) { /* diagnostic LED */ }

static void snt_irq(int state) {
  cpu_set_irq_line(sntlocals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void snt_5220Irq(int state) {
  pia_set_input_cb1(SNT_PIA1, !state);
}
/*----------------------------------------
/    Cheap Squalk  -45
/-----------------------------------------*/
static void cs_init(struct sndbrdData *brdData);
static WRITE_HANDLER(cs_cmd_w);
static READ_HANDLER(cs_port1_r);

const struct sndbrdIntf by45Intf = {
  cs_init, NULL, NULL, cs_cmd_w, NULL, NULL, NULL, 0
};
struct DACinterface cs_dacInt = { 1, { 20 }};
MEMORY_READ_START(cs_readmem)
  { 0x0000, 0x001f, m6803_internal_registers_r },
  { 0x0080, 0x00ff, MRA_RAM },	/*Internal RAM*/
  { 0xb000, 0xdfff, MRA_ROM },
  { 0xe000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(cs_writemem)
  { 0x0000, 0x001f, m6803_internal_registers_w },
  { 0x0080, 0x00ff, MWA_RAM },	/*Internal RAM*/
  { 0xb000, 0xdfff, MWA_ROM },
  { 0xe000, 0xffff, MWA_ROM },
MEMORY_END
PORT_READ_START(cs_readport )
  { M6803_PORT2, M6803_PORT2, cs_port1_r },
PORT_END
PORT_WRITE_START( cs_writeport )
  { M6803_PORT1, M6803_PORT1, DAC_0_data_w },
PORT_END

static struct {
  struct sndbrdData brdData;
  int cmd;
} cslocals;

static void cs_init(struct sndbrdData *brdData) {
  cslocals.brdData = *brdData;
}

static WRITE_HANDLER(cs_cmd_w) {
  cslocals.cmd = data;
  cpu_set_irq_line(cslocals.brdData.cpuNo, M6803_TIN_LINE, PULSE_LINE);
}
static READ_HANDLER(cs_port1_r) { return cslocals.cmd; }
/*----------------------------------------
/    Turbo Cheap Squalk
/-----------------------------------------*/
#define TCS_PIA0  4
static void tcs_init(struct sndbrdData *brdData);
static void tcs_diag(int button);
static WRITE_HANDLER(tcs_cmd_w);
static READ_HANDLER(tcs_status_r);

const struct sndbrdIntf byTCSIntf = {
  tcs_init, NULL, tcs_diag, tcs_cmd_w, tcs_status_r, NULL, NULL, SNDBRD_NOCTRLSYNC|SNDBRD_NOCBSYNC
};
struct DACinterface tcs_dacInt = { 1, { 20 }};
MEMORY_READ_START(tcs_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x6000, 0x6003, pia_r(TCS_PIA0) },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(tcs_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x6000, 0x6003, pia_w(TCS_PIA0) },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END
MEMORY_READ_START(tcs2_readmem)
  { 0x0000, 0x07ff, MRA_RAM },
  { 0x0800, 0x0803, pia_r(TCS_PIA0) },
  { 0x0c00, 0xffff, MRA_ROM },
MEMORY_END
MEMORY_WRITE_START(tcs2_writemem)
  { 0x0000, 0x07ff, MWA_RAM },
  { 0x0800, 0x0803, pia_w(TCS_PIA0) },
  { 0x0c00, 0xffff, MWA_ROM },
MEMORY_END

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
static WRITE_HANDLER(tcs_pia0cb2_w) {
  sndbrd_ctrl_cb(tcslocals.brdData.boardNo,data);
}
static void tcs_diag(int button) { cpu_set_nmi_line(tcslocals.brdData.cpuNo, button ? ASSERT_LINE : CLEAR_LINE); }
static WRITE_HANDLER(tcs_cmd_w) {
  tcslocals.cmd = data; pia_pulse_ca1(TCS_PIA0, 0);
}
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
static READ_HANDLER(sd_status_r);

const struct sndbrdIntf bySDIntf = {
  sd_init, NULL, sd_diag, sd_cmd_w, sd_status_r, NULL, NULL, SNDBRD_NOCTRLSYNC|SNDBRD_NOCBSYNC
};
struct DACinterface sd_dacInt = { 1, { 20 }};
MEMORY_READ16_START(sd_readmem)
  {0x00000000, 0x0003ffff, MRA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_r(SD_PIA0) },	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x0007ffff, MRA16_RAM},		/*RAM*/
MEMORY_END
MEMORY_WRITE16_START(sd_writemem)
  {0x00000000, 0x0003ffff, MWA16_ROM},		/*ROM (4 X 64K)*/
  {0x00060000, 0x00060007, pia_msb_w(SD_PIA0)},	/*PIA - CPU D8-15 connected to PIA D0-7*/
  {0x00070000, 0x0007ffff, MWA16_RAM},		/*RAM*/
MEMORY_END
static WRITE_HANDLER(sd_pia0cb2_w);
static READ_HANDLER(sd_pia0b_r);
static WRITE_HANDLER(sd_pia0a_w);
static WRITE_HANDLER(sd_pia0b_w);
static void sd_pia0irq(int state);

static struct {
  struct sndbrdData brdData;
  int cmd, dacdata, status;
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
static void sd_diag(int button) { cpu_set_irq_line(sdlocals.brdData.cpuNo, MC68000_IRQ_3, button ? ASSERT_LINE : CLEAR_LINE); }
static WRITE_HANDLER(sd_cmd_w) {
  sdlocals.cmd = data; pia_pulse_ca1(SD_PIA0, 0);
}
static READ_HANDLER(sd_status_r) { return sdlocals.status; }

static READ_HANDLER(sd_pia0b_r) {
  int ret = sdlocals.cmd & 0x0f;
  sdlocals.cmd >>= 4;
  return ret;
}
static WRITE_HANDLER(sd_pia0a_w) {
  sdlocals.dacdata = (sdlocals.dacdata & ~0x3fc) | (data << 2);
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
#if 0
void by35_soundInit(void) {
  if (coreGlobals.soundEn) {
    switch (core_gameData->gen) {
      case GEN_BY35_32:
      case GEN_BY35_50:
        sndbrd_1_init(SNDBRD_BY32, 0, memory_region(BY35_MEMREG_SROM),NULL,NULL);
        break;
      case GEN_BY35_61:
      case GEN_BY35_61B:
        sndbrd_1_init(SNDBRD_BY61, BY35_SCPU1NO, NULL, NULL, NULL);
        break;
      case GEN_BY35_45:
        sndbrd_1_init(SNDBRD_BY45, BY35_SCPU1NO, NULL, NULL, NULL);
        break;
      case GEN_BY35_81:
        sndbrd_1_init(SNDBRD_BY81, BY35_SCPU1NO, NULL, NULL, NULL);
        break;
      case GEN_BY35_51:
        sndbrd_1_init(SNDBRD_BY51, BY35_SCPU1NO, NULL,NULL,NULL);
        break;
      case GEN_BY35_56:
        sndbrd_1_init(SNDBRD_BY56, BY35_SCPU1NO, NULL,NULL,NULL);
        break;
    }
  }
}
void by35_soundExit(void) {}

/*-- note that the sound command includes the soundEnable (0x20) --*/
WRITE_HANDLER(by35_soundCmd) {
  if (coreGlobals.soundEn) {
    switch (core_gameData->gen) {
      case GEN_BY35_32:
      case GEN_BY35_50:
      case GEN_BY35_51:
      case GEN_BY35_56:
      case GEN_BY35_61:
      case GEN_BY35_61B:
      case GEN_BY35_81:
      case GEN_BY35_45:
        sndbrd_1_data_w(0,data); break;
    }
  }
}

#endif