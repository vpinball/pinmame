/*Data East/Sega/Stern Sound Hardware:
  ------------------------------------
  Generation 1: YM2151 & MSM5205 (Games up to TMNT)
  Generation 2: BSMT 2000 (Games from Batman and beyond)
*/
#include "driver.h"
#include "core.h"
#include "cpu/m6809/m6809.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"
#include "desound.h"
#include "machine/6821pia.h"
#include "sndbrd.h"

#define DE1S_BANK0 1

static void de1s_init(struct sndbrdData *brdData);
static WRITE_HANDLER(de1s_data_w);
static WRITE_HANDLER(de1s_ctrl_w);
static READ_HANDLER(de1s_cmd_r);
static void de1s_msmIrq(int data);
static void de1s_ym2151IRQ(int state);
static WRITE_HANDLER(de1s_ym2151Port);
static WRITE_HANDLER(de1s_chipsel_w);
static WRITE_HANDLER(de1s_4052_w);
static WRITE_HANDLER(de1s_MSM5025_w);

const struct sndbrdIntf de1sIntf = {
  de1s_init, NULL, NULL, de1s_data_w, NULL, de1s_ctrl_w, NULL
};

static struct MSM5205interface de1s_msm5205Int = {
/* chip          interrupt */
     1, 384000,	{ de1s_msmIrq }, { MSM5205_S48_4B }, { 60 }
};

static struct YM2151interface de1s_ym2151Int = {
  1, 3579545, /* Hz */
  { YM3012_VOL(40,MIXER_PAN_LEFT,40,MIXER_PAN_RIGHT) },
  { de1s_ym2151IRQ }, { de1s_ym2151Port }
};

static MEMORY_READ_START(de1s_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2001, 0x2001, YM2151_status_port_0_r },
  { 0x2400, 0x2400, de1s_cmd_r },
  { 0x3800, 0x3800, watchdog_reset_r},
  { 0x4000, 0x7fff, MRA_BANKNO(DE1S_BANK0) },	/*Voice Samples*/
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(de1s_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2000, YM2151_register_port_0_w },
  { 0x2001, 0x2001, YM2151_data_port_0_w },
  { 0x2800, 0x2800, de1s_chipsel_w },
  { 0x2c00, 0x2c00, de1s_4052_w },
  { 0x3000, 0x3000, de1s_MSM5025_w },
  { 0x3800, 0x3800, watchdog_reset_w },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(de1s)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(de1s_readmem, de1s_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(YM2151,  de1s_ym2151Int)
  MDRV_SOUND_ADD(MSM5205, de1s_msm5205Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

static struct {
  struct sndbrdData brdData;
  int    msmread, nmiEn, cmd;
  UINT8  msmdata;
} de1slocals;

static void de1s_init(struct sndbrdData *brdData) {
  memset(&de1slocals, 0, sizeof(de1slocals));
  de1slocals.brdData = *brdData;
  cpu_setbank(DE1S_BANK0, de1slocals.brdData.romRegion);
  watchdog_reset_w(0,0);
  MSM5205_playmode_w(0,MSM5205_S96_4B); /* Start off MSM5205 at 4khz sampling */
}

static WRITE_HANDLER(de1s_data_w) {
  de1slocals.cmd = data;
}

static WRITE_HANDLER(de1s_ctrl_w) {
  if (~data&0x1) cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_FIRQ_LINE, ASSERT_LINE);
}

static READ_HANDLER(de1s_cmd_r) {
  cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_FIRQ_LINE, CLEAR_LINE);
  return de1slocals.cmd;
}

static void de1s_ym2151IRQ(int state) {
  cpu_set_irq_line(de1slocals.brdData.cpuNo, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(de1s_4052_w) { /*logerror("to4052 %x\n",data);*/ }

/*
Chip 5e: LS273: (0x2800)
---------------
bit 0 = 2 = Select A14 on 4f/6f
bit 1 = 3 = Select A15 on 4f/6f
bit 2 = 4 = Chip select 4f or 6f. (0=6F,1=4F)
bit 3 = N/U
bit 4 = 3 = S1 on MSM5205
bit 5 = 2 = S2 on MSM5205
bit 6 = 1 = RSE on MSM5205
bit 7 = 0 = CLEAR NMI
********************
6f is loaded from 0-1ffff
4f is loaded from 20000-3ffff
*/
static WRITE_HANDLER(de1s_chipsel_w) {
  static const int prescaler[] = { MSM5205_S96_4B, MSM5205_S48_4B, MSM5205_S64_4B, 0};
  int addr = (((data>>0)&0x01)*0x4000) +
	     (((data>>1)&0x01)*0x8000) +
	     (((data>>3)&0x01)*0x10000) +
	     (((data>>2)&0x01)*0x20000);

  cpu_setbank(DE1S_BANK0, de1slocals.brdData.romRegion+addr);
  MSM5205_playmode_w(0, prescaler[(data & 0x30)>>4]); /* bit 4&5 */
  MSM5205_reset_w(0,   (data & 0x40)); /* bit 6 */
  de1slocals.nmiEn =  (~data & 0x80); /* bit 7 */
}

static WRITE_HANDLER(de1s_MSM5025_w) {
  de1slocals.msmdata = data;
  de1slocals.msmread = 0;
}

/* MSM5205 interrupt callback */
static void de1s_msmIrq(int data) {
  MSM5205_data_w(0, de1slocals.msmdata>>4);

  de1slocals.msmdata <<= 4;

  // Are we done fetching both nibbles? Generate an NMI for more data!
  if (de1slocals.msmread && de1slocals.nmiEn)
    cpu_set_nmi_line(de1slocals.brdData.cpuNo, PULSE_LINE);
  de1slocals.msmread ^= 1;
}

/*Send CT2 data to Main CPU's PIA*/
static WRITE_HANDLER(de1s_ym2151Port) {
  sndbrd_ctrl_cb(de1slocals.brdData.boardNo, data & 0x02);
}


/****************************************/
/** GENERATION 2 - BSMT SOUND HARDWARE **/
/****************************************/

// Missing things
// When a sound command is written from the Main CPU it generates a BUF-FULL signal
// and latches the data into U5
// The BUF-FULL signal goes into the PAL. No idea what happens there
// When the CPU reads the sound commands (BIN) a FIRQ is generated and BUF-FULL
// is cleared.
// A FIRQ is also generated from the 24MHz signal via two dividers U3 & U2
// (I think it is 24MHz/4/4096=1536 Hz)
//
// The really strange thing is the PAL. It can generate the following output
// BUSY  - Not used on BSMT board. Sent back to CPU board to STATUS latch?
// OSTAT - If D7 from the CPU is high this sends a reset to the BSMT
//         D0 is sent to CPU board as SST0
// BROM  - ROM for CPU
// BRAM  - RAM for CPU
// DSP0  - Latches A0-A7 and D0-D7 (low) into BSMT
// DSP1  - Latches D0-D7 (high) into BSMT. Ack's IRQ
// BIN   - Gets sound command from LATCH, Clears BUF-FUL and Generates FIRQ
// Input to the PAL is
// A15,A14,A13,A2,A1
// BUF-FUL
// FIRQ
// IRQ
// (E, R/W)
static void de2s_init(struct sndbrdData *brdData);
static READ_HANDLER(de2s_bsmtready_r);
static WRITE_HANDLER(de2s_bsmtreset_w);
static WRITE_HANDLER(de2s_bsmtcmdHi_w);
static WRITE_HANDLER(de2s_bsmtcmdLo_w);
static INTERRUPT_GEN(de2s_firq);

const struct sndbrdIntf de2sIntf = {
  de2s_init, NULL, NULL, soundlatch_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

/* Older 11 Voice Style BSMT Chip */
//NOTE: Do not put a volume adjustment here, otherwise 128x16 games have audible junk played at the beggining
static struct BSMT2000interface de2s_bsmt2000aInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, {0000}
};
/* Newer 12 Voice Style BSMT Chip */
static struct BSMT2000interface de2s_bsmt2000bInt = {
  1, {24000000}, {12}, {DE2S_ROMREGION}, {100}, {2000}
};
/* Older 11 Voice Style BSMT Chip but needs large volume adjustment */
static struct BSMT2000interface de2s_bsmt2000cInt = {
  1, {24000000}, {11}, {DE2S_ROMREGION}, {100}, {4000}
};

static MEMORY_READ_START(de2s_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2002, 0x2003, soundlatch_r },
  { 0x2006, 0x2007, de2s_bsmtready_r },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(de2s_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2001, de2s_bsmtreset_w },
  { 0x2008, 0x5fff, MWA_ROM },
  { 0x6000, 0x6000, de2s_bsmtcmdHi_w },
  { 0x6001, 0x9fff, MWA_ROM },
  { 0xa000, 0xa0ff, de2s_bsmtcmdLo_w },
  { 0xa100, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(de2as)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(de2s_readmem, de2s_writemem)
  MDRV_CPU_PERIODIC_INT(de2s_firq, 489) /* Fixed FIRQ of 489Hz as measured on real machine*/
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD_TAG("bsmt", BSMT2000, de2s_bsmt2000aInt)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2bs)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000bInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(de2cs)
  MDRV_IMPORT_FROM(de2as)
  MDRV_SOUND_REPLACE("bsmt", BSMT2000, de2s_bsmt2000cInt)
MACHINE_DRIVER_END

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  int bsmtData;
} de2slocals;

static void de2s_init(struct sndbrdData *brdData) {
  memset(&de2slocals, 0, sizeof(de2slocals));
  de2slocals.brdData = *brdData;
}

static WRITE_HANDLER(de2s_bsmtcmdHi_w) { de2slocals.bsmtData = data; }
static WRITE_HANDLER(de2s_bsmtcmdLo_w) {
  BSMT2000_data_0_w((~offset & 0xff), ((de2slocals.bsmtData<<8)|data), 0);
  //NOTE: Odd that it will NOT WORK without HOLD_LINE - although we don't clear it anywaywhere!
  cpu_set_irq_line(de2slocals.brdData.cpuNo, M6809_IRQ_LINE, HOLD_LINE);
}

static READ_HANDLER(de2s_bsmtready_r) { return 0x80; } // BSMT is always ready
static WRITE_HANDLER(de2s_bsmtreset_w) { /* Writing 0x80 here resets BSMT ?*/ }

static INTERRUPT_GEN(de2s_firq) {
  //NOTE: Odd that it will NOT WORK without HOLD_LINE - although we don't clear it anywaywhere!
  cpu_set_irq_line(de2slocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}

