#include "driver.h"
#include "cpu/adsp2100/adsp2100.h"
#include "wpc.h"
#include "dcs.h"

/* 310101 Added dcs_speedup to do sound decompression in C */
/* 280101 Corrected bug which caused garbled sound at regular intervals. */
/*        (It was a stupid cut & paste bug '>=' instead of '>') */
/*        changed handling of ROM and RAM banks (slightly faster by caching */
/*        bank address but real reason is that the new speedup code needs the pointer */
/* 041200 Added adsp_init to handle restarts (MAME32) */
/* 281100 Updated to MAME 37b9 */
/*        Removed FORCESOUNDCPU */
/* 261100 Added a speedup hack within WPCDCSSPEEDUP */
/*        requires additional code in ADSP2100.c */
/* 261000 Buffer is now tranmitted in DCS_IRQSTEPS parts */
/*        Hopefully this will correct som garbled sound */
/*        The problem is that the ADSP uses the same buffer */
/*        all the time and fills it in while playing */
/*        The program waits until the transmission passes */
/*        a certain point and then starts to fill the buffer */
/*        with new data. */
/*-- ADSP core functions --*/
static void adsp_init(UINT32 *(*getBootROM)(void),
               void (*txData)(UINT16 start, UINT16 size, UINT16 memStep, int sRate));
static void adsp_boot(void);
static void adsp_txCallback(int port, INT32 data);
static WRITE16_HANDLER(adsp_control_w);

/*-- bank handlers --*/
static WRITE16_HANDLER(dcs1_ROMbankSelect1_w);
static WRITE16_HANDLER(dcs2_ROMbankSelect1_w);
static WRITE16_HANDLER(dcs2_ROMbankSelect2_w);
static WRITE16_HANDLER(dcs2_RAMbankSelect_w);
static READ16_HANDLER(dcs2_RAMbankSelect_r);
static READ16_HANDLER(dcs_ROMbank_r);
static READ16_HANDLER(dcs2_RAMbank_r);
static WRITE16_HANDLER(dcs2_RAMbank_w);

/*-- sound interface handlers --*/
/* once the ADSP core is updated to handle PM mapping */
/* these can be static */
/*static*/ READ16_HANDLER(dcs_latch_r);
/*static*/ WRITE16_HANDLER(dcs_latch_w);

/*-- sound generation --*/
static int dcs_custStart(const struct MachineSound *msound);
static void dcs_custStop(void);
static void dcs_dacUpdate(int num, INT16 *buffer, int length);
static void dcs_txData(UINT16 start, UINT16 size, UINT16 memStep, int sRate);

/*-- local data --*/
#define DCS_BUFFER_SIZE	  4096
#define DCS_BUFFER_MASK	  (DCS_BUFFER_SIZE - 1)

static struct {
 int     enabled;
 UINT32  sOut, sIn, sStep;
 INT16  *buffer;
 int     stream;
} dcs_dac;

static struct {
 UINT16  ROMbank1;
 UINT16  ROMbank2;
 UINT16  RAMbank;
 UINT8  *ROMbankPtr;
 UINT16 *RAMbankPtr;
 int     replyAvail;
} locals;

/*--------------
/  Memory maps
/---------------*/
MEMORY_READ16_START(dcs2_readmem)
  { ADSP_DATA_ADDR_RANGE(0x0000, 0x07ff), dcs_ROMbank_r },
  { ADSP_DATA_ADDR_RANGE(0x1000, 0x1fff), MRA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs2_RAMbank_r },
  { ADSP_DATA_ADDR_RANGE(0x3200, 0x3200), dcs2_RAMbankSelect_r },
  { ADSP_DATA_ADDR_RANGE(0x3300, 0x3300), dcs_latch_r },
  { ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },
  { ADSP_PGM_ADDR_RANGE (0x0000, 0x0800), MRA16_RAM }, /* Internal boot RAM */
  { ADSP_PGM_ADDR_RANGE (0x1000, 0x3fff), MRA16_RAM }, /* External RAM */
MEMORY_END

MEMORY_WRITE16_START(dcs2_writemem)
  { ADSP_DATA_ADDR_RANGE(0x0000, 0x07ff), MWA16_ROM },
  { ADSP_DATA_ADDR_RANGE(0x1000, 0x1fff), MWA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs2_RAMbank_w },
  { ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs2_ROMbankSelect1_w },
  { ADSP_DATA_ADDR_RANGE(0x3100, 0x3100), dcs2_ROMbankSelect2_w },
  { ADSP_DATA_ADDR_RANGE(0x3200, 0x3200), dcs2_RAMbankSelect_w },
  { ADSP_DATA_ADDR_RANGE(0x3300, 0x3300), dcs_latch_w },
  { ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), adsp_control_w },
  { ADSP_PGM_ADDR_RANGE (0x0000, 0x0800), MWA16_RAM }, /* Internal boot RAM */
  { ADSP_PGM_ADDR_RANGE (0x1000, 0x3fff), MWA16_RAM }, /* External RAM */
MEMORY_END

MEMORY_READ16_START(dcs1_readmem)
  { ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MRA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), dcs_ROMbank_r },
  { ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MRA16_RAM },
  { ADSP_PGM_ADDR_RANGE (0x0000, 0x0800), MRA16_RAM }, /* Internal boot RAM */
  { ADSP_PGM_ADDR_RANGE (0x1000, 0x2fff), MRA16_RAM }, /* External RAM */
  { ADSP_PGM_ADDR_RANGE (0x3000, 0x3000), dcs_latch_r },
MEMORY_END

MEMORY_WRITE16_START(dcs1_writemem)
  { ADSP_DATA_ADDR_RANGE(0x0000, 0x1fff), MWA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x2000, 0x2fff), MWA16_ROM },
  { ADSP_DATA_ADDR_RANGE(0x3000, 0x3000), dcs1_ROMbankSelect1_w },
  { ADSP_DATA_ADDR_RANGE(0x3800, 0x39ff), MWA16_RAM },
  { ADSP_DATA_ADDR_RANGE(0x3fe0, 0x3fff), adsp_control_w },
  { ADSP_PGM_ADDR_RANGE (0x0000, 0x0800), MWA16_RAM },
  { ADSP_PGM_ADDR_RANGE (0x1000, 0x2fff), MWA16_RAM },
  { ADSP_PGM_ADDR_RANGE (0x3000, 0x3000), dcs_latch_w },
MEMORY_END

/*---------------
/  Bank handlers
/----------------*/
#define DCS1_ROMBANKBASE(bank) \
  ((UINT8 *)(memory_region(WPC_MEMREG_SROM) + (((bank) & 0x7ff)<<12)))
#define DCS2_ROMBANKBASE(bankH, bankL) \
  ((UINT8 *)(memory_region(WPC_MEMREG_SROM) + \
             (((bankH) & 0x1c)<<18) + (((bankH) & 0x01)<<19) + (((bankL) & 0xff)<<11)))
#define DCS2_RAMBANKBASE(bank) \
  ((UINT16 *)(((bank) & 0x08) ? memory_region(WPC_MEMREG_SBANK) : \
              (memory_region(WPC_MEMREG_SCPU) + ADSP2100_DATA_OFFSET + (0x2000<<1))))

static WRITE16_HANDLER(dcs1_ROMbankSelect1_w) {
  locals.ROMbank1 = data;
  locals.ROMbankPtr = DCS1_ROMBANKBASE(locals.ROMbank1);
}
static WRITE16_HANDLER(dcs2_ROMbankSelect1_w) {
  locals.ROMbank1 = data;
  locals.ROMbankPtr = DCS2_ROMBANKBASE(locals.ROMbank2,locals.ROMbank1);
}
static WRITE16_HANDLER(dcs2_ROMbankSelect2_w) {
  locals.ROMbank2 = data;
  locals.ROMbankPtr = DCS2_ROMBANKBASE(locals.ROMbank2,locals.ROMbank1);
}
static WRITE16_HANDLER(dcs2_RAMbankSelect_w) {
  locals.RAMbank  = data;
  locals.RAMbankPtr = DCS2_RAMBANKBASE(locals.RAMbank);
}
static READ16_HANDLER (dcs2_RAMbankSelect_r) {
  return locals.RAMbank;
}

static READ16_HANDLER(dcs_ROMbank_r)
  { return locals.ROMbankPtr[offset]; }

static READ16_HANDLER(dcs2_RAMbank_r)
  { return locals.RAMbankPtr[offset]; }

static WRITE16_HANDLER(dcs2_RAMbank_w)
  { locals.RAMbankPtr[offset] = data; }

static UINT32 *dcs_getBootROM(void) {
  return (UINT32 *)(memory_region(WPC_MEMREG_SROM) + ((locals.ROMbank1 & 0xff)<<12));
}

/*----------------------
/ Other memory handlers
/-----------------------*/
/* These should be static but the patched ADSP core requires them */

/*static*/ READ16_HANDLER(dcs_latch_r) {
  cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ2, CLEAR_LINE);
  return soundlatch_r(0);
}

/*static*/ WRITE16_HANDLER(dcs_latch_w) {
  DBGLOG(("dcs_latch_w: %4x\n",data));
  soundlatch2_w(0, data);
  locals.replyAvail = TRUE;
}

/*----------------
/ Sound interface
/-----------------*/
struct CustomSound_interface dcs_custInt = {
  dcs_custStart, dcs_custStop, 0
};

static int dcs_custStart(const struct MachineSound *msound) {
  /*-- clear DAC data --*/
  memset(&dcs_dac,0,sizeof(dcs_dac));

  /*-- allocate a DAC stream --*/
  dcs_dac.stream = stream_init("DCS DAC", 100, 32000, 0, dcs_dacUpdate);

  /*-- allocate memory for our buffer --*/
  dcs_dac.buffer = malloc(DCS_BUFFER_SIZE * sizeof(INT16));

  dcs_dac.sStep = 0x10000;

  return (dcs_dac.buffer == 0);
}

static void dcs_custStop(void) {
  if (dcs_dac.buffer)
    { free(dcs_dac.buffer); dcs_dac.buffer = NULL; }
}

static void dcs_dacUpdate(int num, INT16 *buffer, int length) {
  if (!dcs_dac.enabled)
    memset(buffer, 0, length * sizeof(INT16));
  else {
    int ii;
    /* fill in with samples until we hit the end or run out */
    for (ii = 0; ii < length; ii++) {
      if (dcs_dac.sOut == dcs_dac.sIn) break;
      buffer[ii] = dcs_dac.buffer[dcs_dac.sOut];
      dcs_dac.sOut = (dcs_dac.sOut + 1) & DCS_BUFFER_MASK;
    }
#if 0
    if (ii < length)
      logerror("DCS: %d samples of %d missing \n",length-ii, length);
    else {
      int sampleLeft = ((dcs_dac.sIn + DCS_BUFFER_SIZE) - dcs_dac.sOut) & DCS_BUFFER_MASK;
      logerror("Samples left = %d after %d\n",sampleLeft,length);
    }
#endif
    /* fill the rest with the last sample */
    for ( ; ii < length; ii++)
      buffer[ii] = dcs_dac.buffer[(dcs_dac.sOut - 1) & DCS_BUFFER_MASK];
  }
}

/*-------------------
/ Exported interface
/---------------------*/
READ_HANDLER(dcs_data_r) {
  locals.replyAvail = FALSE;
  return soundlatch2_r(0) & 0xff;
}

WRITE_HANDLER(dcs_data_w) {
  soundlatch_w(0, data);
  cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ2, ASSERT_LINE);
}

READ_HANDLER(dcs_ctrl_r) {
  return locals.replyAvail ? 0x80 : 0x00;
}

WRITE_HANDLER(dcs_ctrl_w) {
}

/*----------------------------
/ Checksum in DCS
/ GEN_SECURITY: Page 0x0004
/ WPC95:        Page 0x000c
/
/ Pages, StartPage, ChkSum
/
-----------------------------*/

/*-- handle bug in ADSP core */
static OPBASE_HANDLER(opbaseoveride) { return -1; }

void dcs_init(void) {
  memory_set_opbase_handler(WPC_SCPUNO, opbaseoveride);
  /*-- initialize our structure --*/
  memset(&locals, 0, sizeof(locals));
  locals.ROMbankPtr = (UINT8  *)memory_region(WPC_MEMREG_SROM);
  locals.RAMbankPtr = (UINT16 *)memory_region(WPC_MEMREG_SBANK);

  adsp_init(dcs_getBootROM, dcs_txData);

  /*-- clear all interrupts --*/
  cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ0, CLEAR_LINE );
  cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ1, CLEAR_LINE );
  cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ2, CLEAR_LINE );

  /*-- speed up startup by disable checksum --*/
#if 0
  if (options.cheat) {
    if (core_gameData->gen & GEN_WPC95)
      *((UINT16 *)(memory_region(WPC_MEMREG_SROM) + 0x6000)) = 0x0000;
    else
      *((UINT16 *)(memory_region(WPC_MEMREG_SROM) + 0x4000)) = 0x0000;
  }
#endif
  /*-- boot ADSP2100 --*/
  adsp_boot();
}

/*-----------------
/  local functions
/------------------*/
/*-- autobuffer SPORT transmission  --*/
/*-- copy data to transmit into dac buffer --*/
static void dcs_txData(UINT16 start, UINT16 size, UINT16 memStep, int sRate) {
  UINT16 *mem = ((UINT16 *)(memory_region(WPC_MEMREG_SCPU) + ADSP2100_DATA_OFFSET)) + start;
  int idx;

  stream_update(dcs_dac.stream, 0);
  if (size == 0) /* No data, stop playing */
    { dcs_dac.enabled = FALSE; return; }
  /*-- For some reason can't the sample rate of streams be changed --*/
  /*-- The DCS samplerate is now hardcoded to 32K. Seems to work with all games */
  // if (!dcs_dac.enabled) stream_set_sample_frequency(dcs_dac.stream,sRate);
  /*-- size is the size of the buffer not the number of samples --*/
#if MAMEVER >= 3716
  for (idx = 0; idx < size; idx += memStep) {
    dcs_dac.buffer[dcs_dac.sIn] = mem[idx];
    dcs_dac.sIn = (dcs_dac.sIn + 1) & DCS_BUFFER_MASK;
  }
#else /* MAMEVER */
  size /= memStep;
  sStep = sRate * 65536.0 / (double)(Machine->sample_rate);

  /*-- copy samples into buffer --*/
  while ((idx>>16) < size) {
    dcs_dac.buffer[dcs_dac.sIn] = mem[(idx>>16)*memStep];
    dcs_dac.sIn = (dcs_dac.sIn + 1) & DCS_BUFFER_MASK;
    idx += sStep;
  }
#endif /* MAMEVER */
  /*-- enable the dac playing --*/
  dcs_dac.enabled = TRUE;
}
#define DCS_IRQSTEPS 4
/*--------------------------------------------------*/
/*-- This should actually be part of the CPU core --*/
/*--------------------------------------------------*/
enum {
  S1_AUTOBUF_REG = 15, S1_RFSDIV_REG, S1_SCLKDIV_REG, S1_CONTROL_REG,
  S0_AUTOBUF_REG, S0_RFSDIV_REG, S0_SCLKDIV_REG, S0_CONTROL_REG,
  S0_MCTXLO_REG, S0_MCTXHI_REG, S0_MCRXLO_REG, S0_MCRXHI_REG,
  TIMER_SCALE_REG, TIMER_COUNT_REG, TIMER_PERIOD_REG, WAITSTATES_REG,
  SYSCONTROL_REG
};

static struct {
 UINT16  ctrlRegs[32];
 void   *irqTimer;
 UINT32 *(*getBootROM)(void);
 void   (*txData)(UINT16 start, UINT16 size, UINT16 memStep, int sRate);
} adsp; /* = {{0},NULL,dcs_getBootROM,dcs_txData};*/

static void adsp_init(UINT32 *(*getBootROM)(void),
                     void (*txData)(UINT16 start, UINT16 size, UINT16 memStep, int sRate)) {
  /*-- reset control registers etc --*/
  memset(&adsp, 0, sizeof(adsp));
  adsp.getBootROM = getBootROM;
  adsp.txData = txData;
  /*-- initialize the ADSP Tx callback --*/
  adsp2105_set_tx_callback(adsp_txCallback);
}

static void adsp_boot(void) {
  UINT32 *src = adsp.getBootROM();
  UINT32 *dst = (UINT32 *)(memory_region(WPC_MEMREG_SCPU) + ADSP2100_PGM_OFFSET);
  UINT32  data = src[0];
  UINT32  size;
  UINT32  ii;

  data = src[0];
#ifdef LSB_FIRST // ************** not really tested yet ****************
  data = ((data & 0xff)<<24) | ((data & 0xff00)<<8) | ((data>>8) & 0xff00) | ((data>>24) & 0xff);
#endif /* LSB_FIRST */
  size = ((data & 0xff) + 1) * 8;

  for (ii = 0; ii < size; ii++) {
    data = src[ii];
#ifdef LSB_FIRST // ************** not really tested yet ****************
    data = ((data & 0xff)<<24) | ((data & 0xff00)<<8) | ((data>>8) & 0xff00) | ((data>>24) & 0xff);
#endif
    data >>= 8;
    ADSP2100_WRPGM(&dst[ii], data);
  }
  if (adsp.irqTimer)
    { timer_remove(adsp.irqTimer); adsp.irqTimer = NULL; }
}

static WRITE16_HANDLER(adsp_control_w) {

  adsp.ctrlRegs[offset] = data;
  switch (offset) {
    case SYSCONTROL_REG:
      if (data & 0x0200) {
        /* boot force */
        DBGLOG(("boot force\n"));
        cpu_set_reset_line(WPC_SCPUNO, PULSE_LINE);
        adsp_boot();
        adsp.ctrlRegs[SYSCONTROL_REG] &= ~0x0200;
      }
      /* see if SPORT1 got disabled */
      if ((data & 0x0800) == 0) {
        dcs_txData(0, 0, 0, 0);
        /* nuke the timer */
        if (adsp.irqTimer)
          { timer_remove(adsp.irqTimer); adsp.irqTimer = NULL; }
      }
      break;
    case S1_AUTOBUF_REG:
      /* autobuffer off: nuke the timer */
      if ((data & 0x0002) == 0) {
        adsp.txData(0, 0, 0, 0);
        /* nuke the timer */
        if (adsp.irqTimer)
          { timer_remove(adsp.irqTimer); adsp.irqTimer = NULL; }
      }
      break;
    case S1_CONTROL_REG:
      if (((data>>4) & 3) == 2)
	DBGLOG(("Oh no!, the data is compresed with u-law encoding\n"));
      if (((data>>4) & 3) == 3)
	DBGLOG(("Oh no!, the data is compresed with A-law encoding\n"));
      break;
  } /* switch */
}

static struct {
  UINT16 start;
  UINT16 size, step;
  int    sRate;
  int    iReg;
  int    last;
  int    irqCount;
} adsp_aBufData;

static void adsp_irqGen(int dummy) {
  int next;

  if (adsp_aBufData.irqCount < DCS_IRQSTEPS) {
    adsp_aBufData.irqCount += 1;
#ifdef WPCDCSSPEEDUP
    /* wake up suspended cpu by simulating an interrupt trigger */
    cpu_trigger(-2000 + WPC_SCPUNO);
#endif /* WPCDCSSPEEDUP */
  }
  else {
    adsp_aBufData.irqCount = 1;
    adsp_aBufData.last = 0;
    cpu_set_irq_line(WPC_SCPUNO, ADSP2105_IRQ1, PULSE_LINE);
  }

  next = (adsp_aBufData.size / adsp_aBufData.step * adsp_aBufData.irqCount /
          DCS_IRQSTEPS - 1) * adsp_aBufData.step;

  cpunum_set_reg(WPC_SCPUNO, ADSP2100_I0 + adsp_aBufData.iReg,
                 adsp_aBufData.start + next);

  adsp.txData(adsp_aBufData.start + adsp_aBufData.last, (next - adsp_aBufData.last),
              adsp_aBufData.step, adsp_aBufData.sRate);

  adsp_aBufData.last = next;

}

static void adsp_txCallback(int port, INT32 data) {
  if (port != 1)
    { DBGLOG(("tx0 not handled\n")); return; };
  /*-- remove any pending timer --*/
  if (adsp.irqTimer)
    { timer_remove(adsp.irqTimer); adsp.irqTimer = NULL; }
  if ((adsp.ctrlRegs[SYSCONTROL_REG] & 0x0800) == 0)
    DBGLOG(("tx1 without SPORT1 enabled\n"));
  else if ((adsp.ctrlRegs[S1_AUTOBUF_REG] & 0x0002) == 0)
    DBGLOG(("SPORT1 without autobuffer"));
  else {
    int ireg, mreg;

    ireg = (adsp.ctrlRegs[S1_AUTOBUF_REG]>>9) & 7;
    mreg = ((adsp.ctrlRegs[S1_AUTOBUF_REG]>>7) & 3) | (ireg & 0x04);

    /* start = In, size = Ln, step = Mn */
    adsp_aBufData.step  = cpu_get_reg(ADSP2100_M0 + mreg);
    adsp_aBufData.size  = cpu_get_reg(ADSP2100_L0 + ireg);
    /*-- assume that the first sample comes from the memory position before --*/
    adsp_aBufData.start = cpu_get_reg(ADSP2100_I0 + ireg) - adsp_aBufData.step;
    adsp_aBufData.sRate = Machine->drv->cpu[WPC_SCPUNO].cpu_clock /
                          (2 * (adsp.ctrlRegs[S1_SCLKDIV_REG] + 1)) / 16;
    adsp_aBufData.iReg = ireg;
    adsp_aBufData.irqCount = adsp_aBufData.last = 0;
    adsp_irqGen(0); /* first part, rest is handled via the timer */
    /*-- fire the irq timer --*/
    adsp.irqTimer = timer_pulse(TIME_IN_HZ(adsp_aBufData.sRate) *
                      adsp_aBufData.size / adsp_aBufData.step / DCS_IRQSTEPS,
                                0, adsp_irqGen);
    logerror("DCS size=%d,step=%d,rate=%d\n",adsp_aBufData.size,adsp_aBufData.step,adsp_aBufData.sRate);
    return;
  }
  /*-- if we get here, something went wrong. Disable transmission --*/
  adsp.txData(0, 0, 0, 0);
}

#ifdef WPCDCSSPEEDUP

UINT32 dcs_speedup(UINT32 pc) {
  UINT16 *ram1source, *ram2source, volume;
  int ii;

  /* DCS and DCS95 uses different buffers */
  if (pc > 0x2000) {
    UINT32 volumeOP = *(UINT32 *)&OP_ROM[ADSP2100_PGM_OFFSET + ((pc+0x2b84-0x2b44)<<2)];

    ram1source = (UINT16 *)(memory_region(WPC_MEMREG_SCPU) + ADSP2100_DATA_OFFSET + (0x1000<<1));
    ram2source = locals.RAMbankPtr;
    volume = ram1source[((volumeOP>>4)&0x3fff)-0x1000];
    /*DBGLOG(("OP=%6x addr=%4x V=%4x\n",volumeOP,(volumeOP>>4)&0x3fff,volume));*/
  }
  else {
    UINT32 volumeOP = *(UINT32 *)&OP_ROM[ADSP2100_PGM_OFFSET + ((pc+0x2b84-0x2b44)<<2)];

    ram1source = (UINT16 *)(memory_region(WPC_MEMREG_SCPU) + ADSP2100_DATA_OFFSET + (0x0700<<1));
    ram2source = (UINT16 *)(memory_region(WPC_MEMREG_SCPU) + ADSP2100_DATA_OFFSET + (0x3800<<1));
    volume = ram2source[((volumeOP>>4)&0x3fff)-0x3800];
    /*DBGLOG(("OP=%6x addr=%4x V=%4x\n",volumeOP,(volumeOP>>4)&0x3fff,volume));*/
  }

  {
    UINT16 *i0, *i2;
			/* 2B44     I0 = $2000 >>> (3800) <<< */
    i0 = &ram2source[0];
			/* 2B45:    I2 = $2080 >>> (3880) <<< */
    i2 = &ram2source[0x0080];
			/* 2B46     M2 = $3FFF */
			/* 2B47     M3 = $3FFD */
			/* 2B48     CNTR = $0040 */
			/* 2B49     DO $2B53 UNTIL CE */
    /* M0 = 0, M1 = 1, M2 = -1 */
    for (ii = 0; ii < 0x0040; ii++) {
      INT16 ax0 , ay0, ax1, ay1, ar;
			/* 2B4A       AX0 = DM(I0,M1) */
      ax0 = *i0++;
			/* 2B4B       AY0 = DM(I2,M0) */
      ay0 = *i2;
			/* 2B4C       AR = AX0 + AY0, AX1 = DM(I0,M2) */
      ax1 = *i0--;
      ar = ax0 + ay0;
			/* 2B4D       AR = AX0 - AY0, DM(I0,M1) = AR */
      *i0++ = ar;
      ar = ax0 - ay0;
			/* 2B4E       DM(I2,M1) = AR */
      *i2++ = ar;
			/* 2B4F       AY1 = DM(I2,M0) */
      ay1 = *i2;
			/* 2B50       AR = AX1 + AY1 */
      ar = ax1 + ay1;
			/* 2B51       DM(I0,M1) = AR */
      *i0++ = ar;
			/* 2B52       AR = AX1 - AY1 */
      ar = ax1 - ay1;
  			/* 2B53       DM(I2,M1) = AR */
      *i2++ = ar;
    }
  }
  {
    int mem63d, mem63e, mem63f;
    int jj,kk;
  			/* 2B54     AR = $0002 */
			/* 2B55     DM($15EB) = AR (063d) */
    mem63d = 2;
			/* 2B56     SI = $0040 */
			/* 2B57     DM($15ED) = SI (063e) */
    mem63e = 0x40;
			/* 2B58     SR = LSHIFT SI BY -1 (LO) */
			/* 2B59     DM($15EF) = SR0 (063f) */
    mem63f = mem63e >> 1;
			/* 2B5A     M0 = $3FFF */
			/* 2B5B     CNTR = $0006 */
			/* 2B5C     DO $2B80 UNTIL CE */

    /* M0 = -1, M1 = 1, M5 = 1 */
    for (ii = 0; ii < 6; ii++) {
      UINT16 *i0, *i1, *i2, *i4, *i5;
      INT16 m2, m3;
			/* 2B5D       I4 = $1080 >>> (0780) <<< */
      i4 = &ram1source[0x0080];
			/* 2B5E       I5 = $1000 >>> (0700) <<< */
      i5 = &ram1source[0x0000];
			/* 2B5F       I0 = $2000 >>> (3800) <<< */
      i0 = &ram2source[0x0000];
			/* 2B60       I1 = $2000 >>> (3800) <<< */
      i1 = &ram2source[0x0000];
			/* 2B61       AY0 = DM($15ED) (063e) */
			/* 2B62       M2 = AY0 */
      m2 = mem63e;
			/* 2B63       MODIFY (I1,M2) */
      i1 += m2;
			/* 2B64       I2 = I1 */
      i2 = i1;
			/* 2B65       AR = AY0 - 1 */
			/* 2B66       M3 = AR */
      m3 = mem63e - 1;
			/* 2B67       CNTR = DM($15EB) (063d) */
			/* 2B68       DO $2B79 UNTIL CE */

      for (jj = 0; jj < mem63d; jj++) {
        INT16 mx0, mx1, my0, my1;
			/* 2B6A         MY0 = DM(I4,M5) */
        my0 = *i4++;
			/* 2B6B         MY1 = DM(I5,M5) */
        my1 = *i5++;
			/* 2B6C         MX0 = DM(I1,M1) */
        mx0 = *i1++;
			/* 2B69         CNTR = DM($15EF) (063f) */
			/* 2B6D         DO $2B76 UNTIL CE */
        for (kk = 0; kk < mem63f; kk++) {
          INT16 ax0, ay0, ay1, ar;
          INT32 tmp, mr;
			/* 2B6E           MR = MX0 * MY0 (SS), MX1 = DM(I1,M1) */
          mx1 = *i1++;
          tmp = ((mx0 * my0)<<1);
          mr = tmp;
			/* 2B6F           MR = MR - MX1 * MY1 (RND), AY0 = DM(I0,M1) */
          ay0 = *i0++;
          tmp = ((mx1 * my1)<<1);
          mr = (mr - tmp + 0x8000) & (((tmp & 0xffff) == 0x8000) ? 0xfffeffff : 0xffffffff);
			/* 2B70           MR = MX1 * MY0 (SS), AX0 = MR1 */
          ax0 = mr>>16;
          tmp = (mx1 * my0)<<1;
          mr = tmp;
			/* 2B71           MR = MR + MX0 * MY1 (RND), AY1 = DM(I0,M0) */
          ay1 = *i0--; /* M0 = -1 */
          tmp = ((mx0 * my1)<<1);
          mr = (mr + tmp + 0x8000) & (((tmp & 0xffff) == 0x8000) ? 0xfffeffff : 0xffffffff);
			/* 2B72           AR = AY0 - AX0, MX0 = DM(I1,M1) */
          mx0 = *i1++;
          ar = ay0 - ax0;
			/* 2B73           AR = AX0 + AY0, DM(I0,M1) = AR */
          *i0++ = ar;
          ar = ax0 + ay0;
			/* 2B74           AR = AY1 - MR1, DM(I2,M1) = AR */
          *i2++ = ar;
          ar = ay1 - (mr>>16);
			/* 2B75           AR = MR1 + AY1, DM(I0,M1) = AR */
          *i0++ = ar;
          ar = (mr>>16) + ay1;
			/* 2B76           DM(I2,M1) = AR */
          *i2++ = ar;
        }
			/* 2B77         MODIFY (I2,M2) */
        i2 += m2;
			/* 2B78         MODIFY (I1,M3) */
        i1 += m3;
			/* 2B79         MODIFY (I0,M2) */
        i0 += m2;
      }
			/* 2B7A       SI = DM($15EB) (063d) */
			/* 2B7B:      SR = LSHIFT SI BY 1 (LO) */
			/* 2B7C:      DM($15EB) = SR0 (063d) */
      mem63d <<= 1;
			/* 2B7D       SI = DM($15EF) (063f) */
			/* 2B7E       DM($15ED) = SI (063e) */
      mem63e = mem63f;
			/* 2B7F       SR = LSHIFT SI BY -1 (LO) */
			/* 2B80       DM($15EF) = SR0 (063f) */
      mem63f >>= 1;
    }
  }
  { /* Volume scaling */
    UINT16 *i0;
    UINT16 my0;
			/* 2B81     M0 = $0000 */
			/* 2B82     I0 = $2000 >>> (3800) <<< */
    i0 = &ram2source[0x0000];
			/* 2B84     MY0 = DM($15FD) (390e) */
    my0 = volume;
			/* 2B83     CNTR = $0100 */
			/* 2B85     DO $2B89 UNTIL CE */
    /* M0 = 0, M1 = 1 */
    for (ii = 0; ii < 0x0100; ii++) {
      INT16 mx0;
      INT32 mr;
			/* 2B86       MX0 = DM(I0,M0) */
      mx0 = *i0;
			/* 2B87       MR = MX0 * MY0 (SU) */
      mr = (mx0 * my0)<<1;
			/* 2B88       IF MV SAT MR */
      /* This instruction limits MR to 32 bits */
      /* In reality the volume will never be higher than 0x8000 so */
      /* this is not needed */
			/* 2B89       DM(I0,M1) = MR1 */
      *i0++ = mr>>16;
    }
  }
  cpu_set_reg(ADSP2100_PC, pc + 0x2b89 - 0x2b44);
  return 0; /* execute a NOP */
}

#endif /* WPCDCSSPEEDUP */
