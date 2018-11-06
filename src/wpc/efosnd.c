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
#include "machine/z80fmly.h"

static struct {
  UINT8 sndCmd;
  UINT8 msmData;
  int clockDiv;
  UINT8 vector;
  int irqs[3];
} sndlocals;

WRITE_HANDLER(zc0_0_w) {
  if (data) {
    z80ctc_0_trg0_w(0, 1);
    z80ctc_0_trg0_w(0, 0);
  }
}

WRITE_HANDLER(zc1_0_w) {
//  printf("zc1_0_w: %d\n", data);
}

WRITE_HANDLER(zc2_0_w) {
//  printf("zc2_0_w: %d\n", data);
}

WRITE_HANDLER(zc0_1_w) {
//  printf("zc0_1_w: %d\n", data);
}

WRITE_HANDLER(zc1_1_w) {
//  printf("zc1_1_w: %d\n", data);
}

WRITE_HANDLER(zc2_1_w) {
//  printf("zc2_1_w: %d\n", data);
}

void ctc_interrupt_0(int state) {
  sndlocals.irqs[0] = state;
  sndlocals.vector = Z80_VECTOR(0, state);
  logerror("ctc_irq_0: %d %02x\n", state, sndlocals.vector);
//  if (!sndlocals.vector) sndlocals.vector = 0xff;
  cpu_set_irq_line_and_vector(1, 0, state, sndlocals.vector);
}

void ctc_interrupt_1(int state) {
  sndlocals.irqs[1] = state;
  logerror("ctc_irq_1: %d %02x\n", state, sndlocals.vector);
  sndlocals.vector = 0xf6;
  cpu_set_irq_line_and_vector(1, 0, state, sndlocals.vector);
}

static z80ctc_interface ctc_intf = {
  2,
  { 4000000, 4000000 },
  { 0, 0 },
  { ctc_interrupt_0, ctc_interrupt_1 },
  { zc0_0_w, zc0_1_w },
  { zc1_0_w, zc1_1_w },
  { zc2_0_w, zc2_1_w }
};

static void clock_pulse(int dummy) {
  static int mod2, modDiv, mod32;
  z80ctc_0_trg1_w(0, 1); // 2 MHz
  z80ctc_0_trg1_w(0, 0);
  z80ctc_0_trg2_w(0, modDiv >= sndlocals.clockDiv); // 250 kHz, 500 hHz, or 1 MHz
  modDiv = (modDiv + 1) % (2 * sndlocals.clockDiv);
  z80ctc_0_trg3_w(0, mod32 > 31); // 31.25 kHz
  mod32 = (mod32 + 1) % 64;
  z80ctc_1_trg0_w(0, mod2); // 1 MHz
  z80ctc_1_trg1_w(0, mod2);
  z80ctc_1_trg2_w(0, mod2);
  mod2 = (mod2 + 1) % 2;
}

static void zsu_init(struct sndbrdData *brdData) {
  memset(&sndlocals, 0, sizeof sndlocals);
  sndlocals.vector = 0xff;
  cpu_irq_line_vector_w(1, 0, sndlocals.vector);
  sndlocals.sndCmd = 0xff; // pulled high if no data present
  sndlocals.clockDiv = 4;
  z80ctc_init(&ctc_intf);
//  timer_pulse(TIME_IN_HZ(2000000),0,clock_pulse);
}

static WRITE_HANDLER(zsu_data_w) {
  logerror("--> SND CMD: %02x\n", data);
  sndlocals.sndCmd = data;
  sndlocals.irqs[2] = 1;
  sndlocals.vector = 0xff;
	cpu_set_irq_line_and_vector(1, 0, HOLD_LINE, sndlocals.vector);
}

const struct sndbrdIntf zsuIntf = {
  "ZSU", zsu_init, NULL, NULL, zsu_data_w, zsu_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

static WRITE_HANDLER(ay8910_0_a_w)	{
  logerror("BANK: %x\n", data & 3);
  cpu_setbank(1, memory_region(REGION_USER1) + 0x8000 * (data & 3));
}

static WRITE_HANDLER(ay8910_1_a_w)	{
  logerror("SPEED: %x\n", data >> 4);
  MSM5205_reset_w(0, GET_BIT0);
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

static void zsu_msmIrq(int data) {
  static int intr;
  MSM5205_data_w(0, intr ? sndlocals.msmData & 0x0f : sndlocals.msmData >> 4);
  intr = !intr;
}

static struct MSM5205interface zsu_msm5205Int = {
  1,
  500000,
  {zsu_msmIrq},
  {MSM5205_S48_4B},
  {100}
};

static WRITE_HANDLER(msm_w) {
  logerror("MSM:%02x\n", data);
  sndlocals.msmData = data;
}

static READ_HANDLER(snd_r) {
  logerror("<-- SND READ %02x %02x\n", sndlocals.sndCmd, sndlocals.vector);
  sndlocals.irqs[2] = 0;
  cpu_set_irq_line(1, 0, CLEAR_LINE);
  return sndlocals.sndCmd;
}

static WRITE_HANDLER(ay0_w) {
  logerror("AY0:%04x:%x:%02x\n", activecpu_get_previouspc(), offset, data);
  AY8910Write(0, offset % 2, data);
}

static WRITE_HANDLER(ay1_w) {
  logerror("AY1:%04x:%x:%02x\n", activecpu_get_previouspc(), offset, data);
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
  {0x08, 0x08, msm_w},
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
