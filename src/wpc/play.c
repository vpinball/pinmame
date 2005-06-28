/************************************************************************************************
 Playmatic (Spain)
 -----------------

   Hardware:
   ---------
		CPU:	 CDP1802 @ 2.95 MHz or 3.579545 Mhz (NTSC quartz) for later games
			INT: 360 Hz
		DISPLAY: 5 rows of 7-segment LED panels
		SOUND:	 - discrete (4 tones, like Zaccaria's 1311)
				 - CDP1802 @ NTSC clock with 2 x AY8910 @ NTSC/2 for later games
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "play.h"
#include "cpu/cdp1802/cdp1802.h"
#include "sound/ay8910.h"

#define PLAYMATIC_VBLANKFREQ   60 /* VBLANK frequency */
#define NTSC_QUARTZ 3579545.0 /* 3.58 MHz quartz */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  sndCmd;
  int    ef[5];
  UINT8  databus;
} locals;

static INTERRUPT_GEN(PLAYMATIC_irq) {
  static int state = 0;
  locals.ef[1] = state;
  locals.ef[2] = !state;
  cpu_set_irq_line(PLAYMATIC_CPU, 0, state? ASSERT_LINE : CLEAR_LINE);
  state = !state;
}

static void PLAYMATIC_zeroCross(int data) {
  locals.ef[3] = data;
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(PLAYMATIC_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % PLAYMATIC_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % PLAYMATIC_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(PLAYMATIC) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
  locals.ef[4] = coreGlobals.swMatrix[0] & 1; // test button
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[offset+1];
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(disp_w) {
  coreGlobals.segments[2*offset].w = core_bcd2seg7[data >> 4];
  coreGlobals.segments[2*offset+1].w = core_bcd2seg7[data & 0x0f];
}

static WRITE_HANDLER(sol_w) {
  locals.solenoids |= data;
}

static WRITE_HANDLER(databus_w) {
  locals.databus = data;
}

static WRITE_HANDLER(out_w) {
  logerror("out_w %x:%02x:%02x\n", offset, data, locals.databus);
  lamp_w(offset, data);
}

static int bitColToNum(int tmp)
{
	int i, data;
	i = data = 0;
	while(tmp)
	{
		if(tmp&1) data=i;
		tmp = tmp>>1;
		i++;
	}
	return data;
}

static READ_HANDLER(in_r) {
//  logerror("in_r  %x:%02x\n", offset, locals.databus);
  if (offset == 4) return sw_r(bitColToNum(locals.databus));
  else             return coreGlobals.swMatrix[0];
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *CMOS;
static NVRAM_HANDLER(PLAYMATIC) { core_nvram(file, read_or_write, CMOS, 0x100, 0x00); }
static WRITE_HANDLER(CMOS_w) { CMOS[offset] = data; databus_w(offset, data); }

static MEMORY_READ_START(PLAYMATIC_readmem)
  {0x0000,0x1fff, MRA_ROM},
  {0x2000,0x20ff, MRA_RAM},
  {0x4000,0x5fff, MRA_ROM},
  {0x8000,0x9fff, MRA_ROM},
  {0xc000,0xdfff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(PLAYMATIC_writemem)
  {0x0000,0x1fff, databus_w},
  {0x2000,0x20ff, MWA_RAM, &CMOS},
  {0x2100,0xffff, databus_w},
MEMORY_END

static void dma(int cycles) { /* not used */ }
static void out_n(int data, int n) { out_w(n, data); }
static int in_n(int n) { return in_r(n); }
static void out_q(int level) { /* connected to RST1 pin of flip flop U2 */ }
static int in_ef(void) { return locals.ef[1] | (locals.ef[2] << 1) | (locals.ef[3] << 2) | (locals.ef[4] << 3); }

static CDP1802_CONFIG play1802_config= { dma, out_n, in_n, out_q, in_ef };

static MACHINE_INIT(PLAYMATIC) {
  memset(&locals, 0, sizeof locals);
}

DISCRETE_SOUND_START(play_tones)
	DISCRETE_INPUT(NODE_01,1,0x000f,0)                         // Input handlers, mostly for enable
	DISCRETE_INPUT(NODE_02,2,0x000f,0)
	DISCRETE_INPUT(NODE_04,4,0x000f,0)
	DISCRETE_INPUT(NODE_08,8,0x000f,0)

	DISCRETE_SAWTOOTHWAVE(NODE_10,NODE_01,523,50000,10000,0,0) // C' note
	DISCRETE_SAWTOOTHWAVE(NODE_20,NODE_02,659,50000,10000,0,0) // E' note
	DISCRETE_SAWTOOTHWAVE(NODE_30,NODE_04,784,50000,10000,0,0) // G' note
	DISCRETE_SAWTOOTHWAVE(NODE_40,NODE_08,988,50000,10000,0,0) // H' note

	DISCRETE_ADDER4(NODE_50,1,NODE_10,NODE_20,NODE_30,NODE_40) // Mix all four sound sources

	DISCRETE_OUTPUT(NODE_50, 50)                               // Take the output from the mixer
DISCRETE_SOUND_END

MACHINE_DRIVER_START(PLAYMATIC)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", CDP1802, 2950000/8)
  MDRV_CPU_MEMORY(PLAYMATIC_readmem, PLAYMATIC_writemem)
  MDRV_CPU_CONFIG(play1802_config)
  MDRV_CPU_PERIODIC_INT(PLAYMATIC_irq, 2950000.0/4096.0)
  MDRV_TIMER_ADD(PLAYMATIC_zeroCross, 100)
  MDRV_CPU_VBLANK_INT(PLAYMATIC_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(PLAYMATIC,NULL,NULL)
  MDRV_SWITCH_UPDATE(PLAYMATIC)
  MDRV_NVRAM_HANDLER(PLAYMATIC)
  MDRV_DIPS(0)
  MDRV_SOUND_ADD(DISCRETE, play_tones)
MACHINE_DRIVER_END

// electronic sound section

static READ_HANDLER(ay8910_0_porta_r)	{ return 0; }
static READ_HANDLER(ay8910_1_porta_r)	{ return 0; }

struct AY8910interface play_ay8910 = {
	2,			/* 2 chips */
	NTSC_QUARTZ/2,	/* 1.79 MHz */
	{ 30, 30 },		/* Volume */
	{ ay8910_0_porta_r, ay8910_1_porta_r }
};

static MEMORY_READ_START(playsound_readmem)
  {0x0000,0x1fff, MRA_ROM},
  {0x2000,0x2fff, MRA_ROM},
  {0x8000,0x80ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(playsound_writemem)
  {0x8000,0x80ff, MWA_RAM},
MEMORY_END

MACHINE_DRIVER_START(PLAYMATIC2)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", CDP1802, NTSC_QUARTZ/8)
  MDRV_CPU_MEMORY(PLAYMATIC_readmem, PLAYMATIC_writemem)
  MDRV_CPU_CONFIG(play1802_config)
  MDRV_CPU_PERIODIC_INT(PLAYMATIC_irq, NTSC_QUARTZ/4096.0)
  MDRV_TIMER_ADD(PLAYMATIC_zeroCross, 100)
  MDRV_CPU_VBLANK_INT(PLAYMATIC_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(PLAYMATIC,NULL,NULL)
  MDRV_SWITCH_UPDATE(PLAYMATIC)
  MDRV_NVRAM_HANDLER(PLAYMATIC)
  MDRV_DIPS(0)
  MDRV_CPU_ADD_TAG("scpu", CDP1802, NTSC_QUARTZ/8)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(playsound_readmem, playsound_writemem)
  MDRV_SOUND_ADD(AY8910, play_ay8910)
MACHINE_DRIVER_END
