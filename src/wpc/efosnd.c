/***************************************************************************

   Copied from https://github.com/mamedev/mame/blob/master/src/mame/audio/efo_zsu.cpp:

   ZSU Sound Control Unit (Proyectado 21/4/86 J. Gamell)

   The ZSU board is a component of the Z-Pinball hardware developed by
   E.F.O. (Electrónica Funcional Operativa) S.A. of Barcelona, Spain. Its
   sound generators are 2 AY-3-8910As and 1 OKI MSM5205, and 2 MF10s and
   1 HC4066 are used to mix their outputs. The timing circuits are rather
   intricate, using Z80-CTCs, HC74s and HC393s and various other gates to
   drive both the 5205 and the SGS HCF40105BE (equivalent to CD40105B)
   through which its samples are funneled.

   ZSU's memory map consists primarily of a bank of up to five 27256
   EPROMs switched from two output lines of the first 8910, overlaid with
   a mere 2K of RAM.

   irq vectors

   0xe6 - from ctc0 channel 3 (vector = E0) used to drive MSM5205 through FIFO
   0xee - from ctc0 channel 3 (vector = E8) ^^
   0xf6 - drive AY (once per frame?) triggered by ctc1 channel 3 (vector = F0)
   0xff - read sound latch (triggered by write from master board; default vector set by 5K/+5 pullups on D0-D7)

***************************************************************************/
#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"

static struct {
  UINT8 sndCmd;
  int clockDiv, initDone, fifoSize, timerCnt, trg3;
  UINT8 fifo[16];
} sndlocals;

void ctc_interrupt_0(int state);
void ctc_interrupt_1(int state);
/*
WRITE_HANDLER(zc0_0_w) {
  // resets CA of HC4066 chip
}
WRITE_HANDLER(zc1_0_w) {
  // sets CKA of 1st MF10 chip
}
*/
WRITE_HANDLER(zc2_0_w) {
  if (data) {
    sndlocals.timerCnt = sndlocals.trg3 = 0;
    z80ctc_0_trg3_w(0, sndlocals.fifoSize > 0);
  }
}
/*
WRITE_HANDLER(zc0_1_w) {
  // sets CKB of 1st MF10 chip
}
WRITE_HANDLER(zc1_1_w) {
  // sets CKA of 2nd MF10 chip
}
WRITE_HANDLER(zc2_1_w) {
  // sets CKB of 2nd MF10 chip
}
*/
static z80ctc_interface ctc_intf = {
  2,
  { 4000000, 4000000 },
  { 0, 0 },
  { ctc_interrupt_0, ctc_interrupt_1 },
  { 0 /*zc0_0_w*/, 0 /*zc0_1_w*/ },
  { 0 /*zc1_0_w*/, 0 /*zc1_1_w*/ },
  { zc2_0_w, 0 /*zc2_1_w*/ }
};

static void init(void) {
  if (!sndlocals.initDone) {
    memset(&sndlocals, 0x00, sizeof(sndlocals));
    sndlocals.initDone = 1;
    sndlocals.clockDiv = 1;
    sndlocals.sndCmd = 0xff; // pulled high if no data present
    z80ctc_init(&ctc_intf);
  }
}

// TODO: for some odd reason, sometimes the ZTC irq state is all high bits, causes issues with next IRQ state
void fix_irq_state(void) {
  UINT8 irqstate = cpunum_get_reg(1, Z80_DC0);
  if (irqstate == 0xff) {
    cpunum_set_reg(1, Z80_DC0, 0);
  }
}

void ctc_interrupt_0(int state) {
  init();
  fix_irq_state();
  cpu_set_irq_line_and_vector(1, 0, ASSERT_LINE, Z80_VECTOR(1, state));
}

void ctc_interrupt_1(int state) {
  init();
  fix_irq_state();
  cpu_set_irq_line_and_vector(1, 0, ASSERT_LINE, Z80_VECTOR(0, state));
}

static void shi(UINT8 data) {
  int i;
  if (sndlocals.fifoSize < 16) sndlocals.fifoSize++;
  for (i = sndlocals.fifoSize - 1; i > 0; i--) sndlocals.fifo[i] = sndlocals.fifo[i-1];
  sndlocals.fifo[0] = data;
}

static UINT8 sho(void) {
  if (sndlocals.fifoSize) sndlocals.fifoSize--;
  return sndlocals.fifo[sndlocals.fifoSize];
}

static void clock_pulse(int dummy) {
  static UINT8 msmData;
  z80ctc_0_trg2_w(0, !(sndlocals.timerCnt % sndlocals.clockDiv)); // controls data fetch rate to MSM chip
  z80ctc_0_trg2_w(0, 0);
  sndlocals.trg3 = (sndlocals.timerCnt % 8) > 3;
  z80ctc_0_trg3_w(0, sndlocals.fifoSize || sndlocals.trg3);
  if (sndlocals.timerCnt == 4) msmData = sho();
  if (sndlocals.fifoSize && sndlocals.timerCnt == 64) {
    MSM5205_data_w(0, msmData);
    MSM5205_vclk_w(0, 1);
    MSM5205_vclk_w(0, 0);
  }
  sndlocals.timerCnt = (sndlocals.timerCnt + 1) % 512;
}

static void zsu_init(struct sndbrdData *brdData) {
  sndlocals.initDone = 0;
  init();
  timer_pulse(TIME_IN_HZ(1000000),0,clock_pulse);
}

static WRITE_HANDLER(zsu_data_w) {
  logerror("--> SND CMD: %02x\n", data);
  sndlocals.sndCmd = data;
  cpu_set_irq_line_and_vector(1, 0, ASSERT_LINE, 0xff);
}

const struct sndbrdIntf zsuIntf = {
  "ZSU", zsu_init, NULL, NULL, zsu_data_w, zsu_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static WRITE_HANDLER(ay8910_0_a_w) {
  static int bank;
  if ((data & 3) != bank) {
    bank = data & 3;
    cpu_setbank(1, memory_region(REGION_USER1) + 0x8000 * bank);
  }
}

static WRITE_HANDLER(ay8910_1_a_w) {
  static UINT8 old;
  if (old != data) logerror("MSM %d, SPEED: %x\n", !GET_BIT0, data >> 4);
  old = data;

  MSM5205_reset_w(0, GET_BIT0);
  if (GET_BIT0) {
    sndlocals.fifoSize = 0;
    z80ctc_0_trg3_w(0, sndlocals.trg3);
  }
  if (GET_BIT4) sndlocals.clockDiv = 4; // 250 kHz
  if (GET_BIT5) sndlocals.clockDiv = 2; // 500 kHz
  if (GET_BIT6) sndlocals.clockDiv = 1; // 1 MHz
}

struct AY8910interface zsu_8910Int = {
  2,
  2000000,
  { 30, 30 },
  { NULL }, { NULL },
  { ay8910_0_a_w, ay8910_1_a_w }
};

static struct MSM5205interface zsu_msm5205Int = {
  1,
  0, //500000, // connected but not needed because VCLK pin is being used, saves a timer
  {NULL},
  {MSM5205_SEX_4B},
  {50}
};

static WRITE_HANDLER(fifo_w) {
  shi(data);
  z80ctc_0_trg3_w(0, 1);
}

static READ_HANDLER(snd_r) {
  cpu_set_irq_line(1, 0, CLEAR_LINE);
  return sndlocals.sndCmd;
}

static WRITE_HANDLER(ay0_w) {
  AY8910Write(0, offset % 2, data);
}

static WRITE_HANDLER(ay1_w) {
  AY8910Write(1, offset % 2, data);
}

static MEMORY_READ_START(zsu_readmem)
  {0x0000, 0x6fff, MRA_ROM},
  {0x7000, 0x77ff, MRA_RAM},
  {0x8000, 0xffff, MRA_BANKNO(1)},
MEMORY_END

static MEMORY_WRITE_START(zsu_writemem)
  {0x0000, 0x6fff, MWA_NOP},
  {0x7000, 0x77ff, MWA_RAM},
MEMORY_END

static PORT_READ_START(zsu_readport)
  {0x00, 0x03, z80ctc_0_r},
  {0x04, 0x07, z80ctc_1_r},
  {0x14, 0x14, snd_r},
MEMORY_END

static PORT_WRITE_START(zsu_writeport)
  {0x00, 0x03, z80ctc_0_w},
  {0x04, 0x07, z80ctc_1_w},
  {0x08, 0x08, fifo_w},
  {0x0c, 0x0d, ay0_w},
  {0x10, 0x11, ay1_w},
MEMORY_END

static Z80_DaisyChain zsu_DaisyChain[] = {
  {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 1},
  {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
  {0, 0, 0, -1}
};

MACHINE_DRIVER_START(ZSU)
  MDRV_CPU_ADD_TAG("scpu", Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(zsu_readmem, zsu_writemem)
  MDRV_CPU_PORTS(zsu_readport, zsu_writeport)
  MDRV_CPU_CONFIG(zsu_DaisyChain)
  MDRV_SOUND_ADD(AY8910, zsu_8910Int)
  MDRV_SOUND_ADD(MSM5205, zsu_msm5205Int)
MACHINE_DRIVER_END
