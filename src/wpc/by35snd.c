#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "by35.h"
#include "snd_cmd.h"
#include "by35snd.h"

static struct pia6821_interface snt_pia[2];
static struct pia6821_interface sp_pia;

static void snt_cmd(int data);
static void s32_cmd(int data);
static void sp51_cmd(int data);
static void sp56_cmd(int data);
/*------ */
void by35_soundInit(void) {
  if (coreGlobals.soundEn) {
    switch (core_gameData->gen) {
      case GEN_BY35_32:
      case GEN_BY35_50:
        break;
      case GEN_BY35_61:
      case GEN_BY35_61B:
      case GEN_BY35_81:
        pia_config(2, PIA_STANDARD_ORDERING, &snt_pia[0]);
        pia_config(3, PIA_STANDARD_ORDERING, &snt_pia[1]);
        break;
      case GEN_BY35_51:
      case GEN_BY35_56:
        pia_config(2, PIA_STANDARD_ORDERING, &sp_pia);
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
        s32_cmd(data); break;
      case GEN_BY35_61:
      case GEN_BY35_61B:
      case GEN_BY35_81:
        snt_cmd(data); break;
      case GEN_BY35_51:
        sp51_cmd(data); break;
      case GEN_BY35_56:
        sp56_cmd(data); break;
    }
  }
}

/*----------------------------------------
/              -32, -50 sound
/-----------------------------------------*/
#define S32_DECAYFREQ   50
#define S32_EXPFACTOR   95   // %
#define S32_PITCH      100   // 0-100%
/* waveform for the audio hardware */
static UINT8 sineWave[] = {
  204,188,163,214,252,188,115,125,136,63,0,37,88,63,47,125
/*
128,148,166,181,193,202,206,208,207,206,205,206,209,215,222,231,240,
248,254,255,254,248,237,223,207,191,174,160,148,140,134,130,128,126,
122,116,108,96 ,82 ,65 ,49 ,33 ,19 ,8  ,2  ,0  ,2  ,8  ,16 ,25 ,34 ,
41 ,47 ,50 ,51 ,50 ,49 ,48 ,50 ,54 ,63 ,75 ,90 ,108
*/
};

static struct {
  int volume;
  int lastCmd;
  int channel;
  void *vTimer;
} s32locals;

static void s32_decay(int param) {
  mixer_set_volume(s32locals.channel, s32locals.volume/10);
  if (s32locals.volume < 100) s32locals.volume = 0;
  else s32locals.volume = s32locals.volume * S32_EXPFACTOR / 100;
}

static int s32_sh_start(const struct MachineSound *msound) {
  s32locals.channel = mixer_allocate_channel(100);
  mixer_set_volume(s32locals.channel,0);
  mixer_play_sample(s32locals.channel, (signed char *)sineWave, sizeof(sineWave), 1, 1);
  s32locals.vTimer = timer_pulse(TIME_IN_HZ(S32_DECAYFREQ),0,s32_decay);
  return 0;
}

static void s32_sh_stop(void) {
  mixer_stop_sample(s32locals.channel);
  if (s32locals.vTimer) { timer_remove(s32locals.vTimer); s32locals.vTimer = NULL; }
}

static void s32_cmd(int data) {
  data ^= 0x10;
  if (data == s32locals.lastCmd) return; /* Nothing has changed */

  if (data & 0x20) { /* sound is enabled -> change frequency */
    if ((data & 0x0f) != 0x0f) {
      UINT8 sData; int f;
      sData = *((UINT8 *)(memory_region(BY35_MEMREG_SROM) + (data & 0x1f)));
      sData = CORE_SWAPBYTE(sData);
      f= sizeof(sineWave)/((1.1E-6+S32_PITCH*1E-8)*sData)/8;

      mixer_set_sample_frequency(s32locals.channel, f);
    }
    if ((s32locals.lastCmd & 0x20) == 0) s32locals.volume = 1000; /* positive edge */
  }
  else s32locals.volume = 0; /* Sound is disabled */
  s32locals.lastCmd = data;
}

struct CustomSound_interface s32_custInt = {s32_sh_start, s32_sh_stop};

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
/  U2  PIA 0080-0083
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
/  Vocalizer board
*/
static struct {
  int lastcmd, currcmd, a ,b, cmdout, cmd[2];
} splocals;
static READ_HANDLER(sp_8910a_r);
static WRITE_HANDLER(sp_pia2a_w);
static WRITE_HANDLER(sp_pia2b_w);
static void sp_irq(int state);

struct AY8910interface sp_ay8910Int = {
  1, 3580000/4, {10}, {sp_8910a_r}
};
struct hc55516_interface sp_hc55516Int = { 1, {100}};

MEMORY_READ_START(sp51_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_2_r },
  { 0x1000, 0x1fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_READ_START(sp56_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x00ff, pia_2_r },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(sp_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x00ff, pia_2_w },
  { 0x1000, 0x1fff, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static READ_HANDLER(sp_8910r);
static struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ sp_8910r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ sp_pia2a_w, sp_pia2b_w, 0, 0,
  /*irq: A/B           */ sp_irq, sp_irq
};
static READ_HANDLER(sp_8910r) {
  if ((splocals.b & 0x03) == 0x01) return AY8910Read(0);
  return 0;
}

static WRITE_HANDLER(sp_pia2a_w) {
  splocals.a = data;
  if (splocals.b & 0x02) {
    AY8910Write(0, splocals.b ^ 0x01, splocals.a);
  }
}

static WRITE_HANDLER(sp_pia2b_w) {
  splocals.b = data;
  if (core_gameData->gen & GEN_BY35_56) {
    hc55516_digit_w(0,(data & 0x80)>0);
    hc55516_clock_w(0,(data & 0x40)>0);
  }
  if (splocals.b & 0x02) {
    AY8910Write(0, splocals.b ^ 0x01, splocals.a);
  }
}

static void sp51_cmd_sync(int data) {
  pia_set_input_ca1(2, data & 0x20);
  splocals.currcmd = ~data & 0x1f;
}
static void sp51_cmd(int data) {
  if ((data ^ splocals.lastcmd) & 0x20)
    timer_set(TIME_NOW, data, sp51_cmd_sync);
  splocals.lastcmd = data;
}

static void sp56_cmd_sync(int data) {
  splocals.cmdout = 0;
  pia_set_input_ca1(2, 0); pia_set_input_ca1(2, 1);
}

static void sp56_cmd(int data) {
  if (data & ~splocals.lastcmd & 0x20)
    splocals.currcmd = 0;
  else if ((data & 0x20) && splocals.currcmd < 2) {
    splocals.cmd[splocals.currcmd++] = ~data & 0x1f; snd_cmd_log(data);
    if (splocals.currcmd == 2) /* two commands received */
      timer_set(TIME_NOW, data, sp56_cmd_sync);
  }
  splocals.lastcmd = data;
}

static READ_HANDLER(sp_8910a_r) {
  if (core_gameData->gen & GEN_BY35_56) {
    switch (splocals.cmdout) {
      case 1 : //pia_set_input_ca1(2, 0);
      case 0 : return splocals.cmd[splocals.cmdout++];
      default: return splocals.lastcmd;
    }
  }
  return splocals.currcmd;
}

static void sp_irq(int state) {
  cpu_set_irq_line(BY35_SCPU1NO, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
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
/ PIA2
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
/ PIA3: 0090
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

static READ_HANDLER(snt_pia2a_r);
static WRITE_HANDLER(snt_pia2a_w);
static WRITE_HANDLER(snt_pia2b_w);
static WRITE_HANDLER(snt_pia3a_w);
static WRITE_HANDLER(snt_pia3b_w);
static READ_HANDLER(snt_8910a_r);
static WRITE_HANDLER(snt_pia2ca2_w);
static void snt_irq(int state);
static void snt_5220Irq(int state);

struct TMS5220interface snt_tms5220Int = { 640000, 100, snt_5220Irq };
struct DACinterface snt_dacInt = { 1, { 100 }};
struct AY8910interface snt_ay8910Int = {
  1, 3580000/4, {100}, {snt_8910a_r}
};

static struct {
  int a2,b2,a3,b3,cmd[2],lastcmd,cmdsync,cmdout;
} sntlocals;
static struct pia6821_interface snt_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ snt_pia2a_r, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ snt_pia2a_w, snt_pia2b_w, snt_pia2ca2_w, 0,
  /*irq: A/B           */ snt_irq, snt_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ snt_pia3a_w, snt_pia3b_w, 0, 0,
  /*irq: A/B           */ snt_irq, snt_irq
}};
static READ_HANDLER(snt_pia2a_r) {
  if (core_gameData->gen & GEN_BY35_61B) return snt_8910a_r(0);
  if ((sntlocals.b2 & 0x03) == 0x01)     return AY8910Read(0);
  return 0;
}
static WRITE_HANDLER(snt_pia2a_w) {
  sntlocals.a2 = data;
  //DBGLOG(("8910w %d %x\n",sntlocals.b2, sntlocals.a2));
  if (sntlocals.b2 & 0x02) {
    AY8910Write(0, sntlocals.b2 ^ 0x01, sntlocals.a2);
  }
}
static WRITE_HANDLER(snt_pia2b_w) {
  sntlocals.b2 = data;
  //DBGLOG(("8910w %d %x\n",sntlocals.b2, sntlocals.a2));
  if (sntlocals.b2 & 0x02) {
    AY8910Write(0, sntlocals.b2 ^ 0x01, sntlocals.a2);
  }
}
static WRITE_HANDLER(snt_pia3a_w) {
  sntlocals.a3 = data;
}
static WRITE_HANDLER(snt_pia3b_w) {
  if (sntlocals.b3 & ~data & 0x02) { // write
    tms5220_data_w(0, sntlocals.a3);
    pia_set_input_ca2(3, 1); pia_set_input_ca2(3, 0);
  }
  else if (sntlocals.b3 & ~data & 0x01) { // read
    pia_set_input_a(3,tms5220_status_r(0));
    pia_set_input_ca2(3, 1); pia_set_input_ca2(3, 0);
  }
  sntlocals.b3 = data;
}

/*-- pass command from main cpu to sound cpu --*/
/* The main CPU causes an IRQ and then sends two commands
   To sync the CPUs we wait until both commands has been sent
   before causing the IRQ. If the command is read outside
   the IRQ the result is undefined.
   I read somewhere that the main CPU creates sounds at the same
   time as solenoids are activated. This will probably not work.
*/
static void snt_cmd_sync(int data) {
  sntlocals.cmdout = 0; /* start reading commands */
  pia_set_input_cb1(2, 0); pia_set_input_cb1(2,1); /* generate IRQ */
}

/* Collect two commands after rising edge */
static void snt_cmd(int data) {
  if (data & ~sntlocals.lastcmd & 0x20)
    sntlocals.cmdsync = 0;
  else if ((data & 0x20) && sntlocals.cmdsync < 2) {
    sntlocals.cmd[sntlocals.cmdsync++] = data; snd_cmd_log(data);
    if (sntlocals.cmdsync == 2) /* two commands received */
      timer_set(TIME_NOW, data, snt_cmd_sync);
  }
  sntlocals.lastcmd = data;
}

static READ_HANDLER(snt_8910a_r) {
  int tmp = (sntlocals.cmdout < 2) ? sntlocals.cmd[sntlocals.cmdout++] :
                                     sntlocals.lastcmd;
  return ~tmp & 0x1f;
}

static WRITE_HANDLER(snt_pia2ca2_w) { /* diagnostic LED */ }

static void snt_irq(int state) {
  cpu_set_irq_line(BY35_SCPU1NO, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}
static void snt_5220Irq(int state) {
  pia_set_input_cb1(3, !state);
}

MEMORY_READ_START(snt_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_2_r },
  { 0x0090, 0x0093, pia_3_r },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(snt_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_2_w },
  { 0x0090, 0x0093, pia_3_w },
  { 0x1000, 0x1000, DAC_0_data_w },
  { 0xc000, 0xffff, MWA_ROM },
MEMORY_END


