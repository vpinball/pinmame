/************************************************************************************************
 Juegos Populares (Spain)
 ------------------------
   NOTES:
   This system doesn't seem to have any kind of diagnostic tests, which makes emulation a pain in the a..
   Also, the display board is just being fed 3 serial bits, and everything else goes from there.
   I still haven't found where the external sound board gets its commands from either...
   The Aqualand schematics unfortunately aren't very detailed; at least all the chips are visible.

   Hardware:
   ---------
		CPU:     Z80 @ 4 MHz
			INT: IRQ @ 488 Hz (4MHz/4096/2)
		IO:      DMA, AY8910 input ports
		DISPLAY: 7-digit 8-segment panels with direct segment access, driven by 4x 4094 serial controllers.
		SOUND:	 AY8910 @ 2 MHz on CPU board,
		         Z80 CPU with DAC? on separate board.
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "jp.h"
#include "sound/ay8910.h"
#include "sndbrd.h"
#include "machine/4094.h"

#define JP_VBLANKFREQ   60 /* VBLANK frequency */
#define JP_IRQFREQ     488 /* IRQ frequency */
#define JP_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  core_tSeg segments;
  UINT32 dispData;
  UINT8  sndCmd;
  int    swCol;
} locals;

static INTERRUPT_GEN(JP_irq) {
  cpu_set_irq_line(JP_CPU, 0, PULSE_LINE);
}

// bits 24 to 27 decide about the lit segment (4 bits decoded by a 4028 chip),
// the rest of the bits enable the single digits (therefore 28 digits possible).
static void dispStrobe(void) {
  static UINT8 segs[16] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0,0,0,0,0,0,0,0 };
  int seg = segs[(locals.dispData >> 24) & 0x0f];
  int i,j;
  for (j=0; j < 4; j++) {
	// bits 28 - 31: scores 1M points per player
    if (locals.dispData & (0x10000000 << j)) locals.segments[j*7].w |= seg;
    // bits 4 - 23: scores 100k - 100 points per player
    for (i=0; i < 5; i++)
      if (locals.dispData & (1 << (j*6+i)))  locals.segments[j*7+i+1].w |= seg;

    // bits 0 - 3: match / credits
    if (locals.dispData & (1 << j)) locals.segments[28+j].w |= seg;
    // fake single points zero digit
    locals.segments[j*7 + 6].w = (locals.segments[j*7 + 5].w) ? core_bcd2seg7[0] : 0;
    // number of balls: data is bit 6 - 13 reversed, LEDs: strobed by bits 14 - 23
    if ((locals.dispData >> 24) == 9) {
      locals.segments[32].w |= core_revbyte((locals.dispData >> 6) & 0xff);
      coreGlobals.tmpLampMatrix[13] |= (locals.dispData >> 14) & 0xff;
      coreGlobals.tmpLampMatrix[14] |= (locals.dispData >> 22) & 0x03;
    }
  }
}

static WRITE_HANDLER(parallel_0_out) {
  locals.dispData = (locals.dispData & 0xffffff00) | data;
  dispStrobe();
}
static WRITE_HANDLER(parallel_1_out) {
  locals.dispData = (locals.dispData & 0xffff00ff) | (data << 8);
  dispStrobe();
}
static WRITE_HANDLER(parallel_2_out) {
  locals.dispData = (locals.dispData & 0xff00ffff) | (data << 16);
  dispStrobe();
}
static WRITE_HANDLER(parallel_3_out) {
  locals.dispData = (locals.dispData & 0x00ffffff) | (data << 24);
  dispStrobe();
}
static WRITE_HANDLER(qs1pin_0_out) {
  HC4094_data_w(1, data);
}
static WRITE_HANDLER(qs1pin_1_out) {
  HC4094_data_w(2, data);
}
static WRITE_HANDLER(qs1pin_2_out) {
  HC4094_data_w(3, data);
}

static HC4094interface hc4094jp = {
  4, // 4 chips
  { parallel_0_out, parallel_1_out, parallel_2_out, parallel_3_out },
  { 0 },
  { qs1pin_0_out, qs1pin_1_out, qs1pin_2_out }
};

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(JP_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % JP_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  if ((locals.vblankCount % JP_SOLSMOOTH) == 0)
    coreGlobals.solenoids = locals.solenoids;
  /*-- display --*/
  if ((locals.vblankCount % JP_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));
    memset(locals.segments, 0, sizeof(locals.segments));
  }

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(JP) {
  if (inports) {
//    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xfb, 1);
  }
}

static READ_HANDLER(dip_r) {
  return core_getDip(3-offset);
}

// No idea which bit does what here...
// The schematics won't tell, but the order should be:
// DATA, CLOCK, LATCH, X0, X1, X2, X3, X4
static WRITE_HANDLER(disp_w) {
  locals.swCol = core_BitColToNum((~data >> 4) & 0x0f);
//  logerror("disp: %02x\n", data);
  HC4094_data_w (0, GET_BIT0);
  HC4094_clock_w(0, GET_BIT1);
  HC4094_clock_w(1, GET_BIT1);
  HC4094_clock_w(2, GET_BIT1);
  HC4094_clock_w(3, GET_BIT1);
  HC4094_strobe_w(0, GET_BIT2);
  HC4094_strobe_w(1, GET_BIT2);
  HC4094_strobe_w(2, GET_BIT2);
  HC4094_strobe_w(3, GET_BIT2);
  HC4094_oe_w(0, 1);
  HC4094_oe_w(1, 1);
  HC4094_oe_w(2, 1);
  HC4094_oe_w(3, 1);
}

static WRITE_HANDLER(lamp1_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(lamp2_w) {
  coreGlobals.tmpLampMatrix[5+offset] = data;
}

static WRITE_HANDLER(sol1_w) {
  locals.solenoids = (locals.solenoids & 0xffffff00) | data;
}

static WRITE_HANDLER(sol2_w) {
  locals.solenoids = (locals.solenoids & 0xffff00ff) | (data << 8);
}

static WRITE_HANDLER(ay8910_ctrl_w) { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_data_w) { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_r)      { return AY8910Read(0); }

// Funny: they use the ports of the AY8910 sound chip as switch inputs! :)
static READ_HANDLER (ay8910_portA_r) {
	if (locals.swCol)
		return ~core_getDip(locals.swCol - 1);
	else
		return ~coreGlobals.swMatrix[5];
}
static READ_HANDLER (ay8910_portB_r) {
	return ~coreGlobals.swMatrix[1+locals.swCol];
}

struct AY8910interface JP_ay8910Int = {
	1,					/* 1 chip */
	2000000,			/* 2 MHz */
	{ 30 },				/* Volume */
	{ ay8910_portA_r },	/* Input Port A callback */
	{ ay8910_portB_r },	/* Input Port B callback */
};

static MEMORY_READ_START(JP_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4300,0x43ff, MRA_RAM},
  {0x4400,0x47ff, MRA_RAM},
  {0x6001,0x6001, ay8910_r},
MEMORY_END

static MEMORY_WRITE_START(JP_writemem)
  {0x4300,0x43ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x4400,0x47ff, MWA_RAM},
  {0x6000,0x6000, ay8910_ctrl_w},
  {0x6002,0x6002, ay8910_data_w},
  {0xa000,0xa000, sol1_w},
  {0xa001,0xa001, disp_w},
  {0xa002,0xa002, sol2_w},
  {0xa003,0xa007, lamp1_w},
  {0xc000,0xc007, lamp2_w},
MEMORY_END

static MACHINE_INIT(JP) {
  memset(&locals, 0, sizeof locals);
  HC4094_init(&hc4094jp);
}

MACHINE_DRIVER_START(JP)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, JP_CPUFREQ)
  MDRV_CPU_MEMORY(JP_readmem, JP_writemem)
  MDRV_CPU_VBLANK_INT(JP_vblank, 1)
  MDRV_CPU_PERIODIC_INT(JP_irq, JP_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(JP,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(24)
  MDRV_SWITCH_UPDATE(JP)
  MDRV_SOUND_ADD(AY8910, JP_ay8910Int)
MACHINE_DRIVER_END


// external sound board section

static MACHINE_INIT(JPS) {
  memset(&locals, 0, sizeof locals);
  HC4094_init(&hc4094jp);
  /* init sound */
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(JP_MEMREG_SND),NULL,NULL);
}

static MACHINE_STOP(JPS) {
  sndbrd_0_exit();
}

static INTERRUPT_GEN(JPS_irq) {
  cpu_set_irq_line(1, 0, PULSE_LINE);
}

static struct DACinterface JP_dacInt = { 1, { 25 }};

static WRITE_HANDLER(bank_w) {
  cpu_setbank(1, memory_region(REGION_SOUND1) + (0x8000 * data & 0x07));
}

static MEMORY_READ_START(jpsnd_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x47fc,0x47ff, MRA_RAM},
  {0x8000,0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(jpsnd_writemem)
  {0x47fc,0x47ff, MWA_RAM},
  {0x5000,0x5000, bank_w  },
  {0x7000,0x7000, DAC_0_data_w },
MEMORY_END

MACHINE_DRIVER_START(JPS)
  MDRV_IMPORT_FROM(JP)
  MDRV_CPU_ADD_TAG("scpu", Z80, 4000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(jpsnd_readmem, jpsnd_writemem)
  MDRV_CPU_PERIODIC_INT(JPS_irq, JP_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(JPS,NULL,JPS)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, JP_dacInt)
MACHINE_DRIVER_END
