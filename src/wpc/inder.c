/************************************************************************************************
 Inder (Spain)
 -------------

 ISSUES:
 - Missing memory write handlers, esp. for 0x4805, which seems to be a timer of some sort?
 - Does the NVRAM really use 8 bits? It's 256 Bytes long, and has 2 5101 chips.

 NOTE:
 - According to the manual, these games could read 10 switch columns, but only 3 + 5 are used.

   Hardware:
   ---------
		CPU:     Z80 @ 2.5 MHz
			INT: IRQ @ 250 Hz (4 ms)
		IO:      DMA for earlier games,
		         PIAs for later ones.
		DISPLAY: 6-digit or 7-digit 7-segment panels with direct segment access
		SOUND:	 TI76489 @ 2 or 4 MHz for Brave Team
				 AY8910 @ 2 MHz for Canasta86,
		         MSM5205 @ 384 kHz on Z80 CPU for later games.
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "inder.h"
#include "sound/ay8910.h"
#include "sound/msm5205.h"
#include "sndbrd.h"
#include "machine/8255ppi.h"

#define INDER_VBLANKFREQ   60 /* VBLANK frequency */
#define INDER_IRQFREQ     250 /* IRQ frequency */
#define INDER_CPUFREQ 2500000 /* CPU clock frequency */

static READ_HANDLER(snd_porta_r);
static READ_HANDLER(snd_portb_r);
static READ_HANDLER(snd_portc_r);
static WRITE_HANDLER(snd_porta_w);
static WRITE_HANDLER(snd_portb_w);
static WRITE_HANDLER(snd_portc_w);

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  dispSeg[7];
  UINT8  swCol[5];
  core_tSeg segments;
  UINT8  sndCmd;
} locals;

// switches start at 50 for column 1, and each column adds 10.
static int INDER_sw2m(int no) { return (no/10 - 4)*8 + no%10; }
static int INDER_m2sw(int col, int row) { return 40 + col*10 + row; }

static INTERRUPT_GEN(INDER_irq) {
  cpu_set_irq_line(INDER_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(INDER_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % INDER_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % INDER_SOLSMOOTH) == 0)
  	locals.solenoids = 0;
  /*-- display --*/
  if ((locals.vblankCount % INDER_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));
  }

  core_updateSw(core_getSol(5));
}

static SWITCH_UPDATE(INDER) {
  int i;
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xfb, 1);
  }
  for (i=0; i < 5; i++) locals.swCol[i] = ~coreGlobals.swMatrix[i+1];
}

static READ_HANDLER(dip_r) {
  return ~core_getDip(3-offset);
}

static READ_HANDLER(sw_r) {
  logerror("sw_r(%d): %02x\n", offset, locals.swCol[offset]);
  return locals.swCol[offset];
}

// switch debouncing; I wonder if this is how it is really done?
static WRITE_HANDLER(sw_w) {
  locals.swCol[offset] = data;
  logerror("sw_w(%d): %02x\n", offset, data);
}

// Display uses direct segment addressing, so simple chars are possible.
static WRITE_HANDLER(disp_w) {
  int i;
  if (offset < 5) locals.dispSeg[offset] = data;
  else for (i=0; i < 5; i++)
    locals.segments[8*i + 7-(data >> 3)].w = locals.dispSeg[i];
}
// secondary display (Lap By Lap and Moon Light)
static WRITE_HANDLER(disp2_w) {
  if (data & 0xf0) {
    locals.segments[51 - core_BitColToNum(data >> 4)].w = core_bcd2seg7[data & 0x0f];
  }
}

// external sound
static WRITE_HANDLER(snd2_w) {
  locals.sndCmd = data;
  cpu_set_nmi_line(INDER_SND_CPU, PULSE_LINE);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

// It's hard to draw the line between solenoids and lamps, as they share the outputs...
static WRITE_HANDLER(sol_w) {
  locals.solenoids |= data;
}

static READ_HANDLER(sndcmd_r) {
  return locals.sndCmd;
}

/*-------------------------------------------------------
/ Brave Team: Using a TI76489 chip, equivalent to 76496.
/--------------------------------------------------------*/
struct SN76496interface INDER_ti76489Int = {
	1,	/* total number of chips in the machine */
	{ 2000000 },	/* base clock 2 MHz (or 4 MHz?) */
	{ 75 }	/* volume */
};
static WRITE_HANDLER(ti76489_0_w)	{ SN76496_0_w(0, core_revbyte(data)); }

/*--------------------------------------------------
/ Canasta 86: Using a AY8910 chip, no extra ROMs.
/---------------------------------------------------*/
struct AY8910interface INDER_ay8910Int = {
	1,			/* 1 chip */
	2000000,	/* 2 MHz */
	{ 30 },		/* Volume */
};
static WRITE_HANDLER(ay8910_0_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w)   { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_0_r)        { return AY8910Read(0); }

/*--------------------------------------------------
/ Lap By Lap: Using two AY8910 chips on separate Z80 CPU.
/---------------------------------------------------*/
struct AY8910interface INDER_ay8910Int2 = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 35, 35 },		/* Volume */
	{ 0, sndcmd_r },	/* Input Port A callback (sound command) */
};
static WRITE_HANDLER(ay8910_1_ctrl_w)   { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w)   { AY8910Write(1,1,data); }
static READ_HANDLER (ay8910_1_r)        { return AY8910Read(1); }

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *INDER_CMOS;

static NVRAM_HANDLER(INDER) {
  core_nvram(file, read_or_write, INDER_CMOS, 0x100, 0xff);
}
// 5101 RAM only uses 4 bits, but they used 2 chips for 8 bits?
static WRITE_HANDLER(INDER_CMOS_w) { INDER_CMOS[offset] = data; }
static READ_HANDLER(INDER_CMOS_r)  { return INDER_CMOS[offset]; }

/*------------------
/ PIA definitions
/-------------------*/
static READ_HANDLER(ci20_porta_r) { logerror("UNDOCUMENTED: ci20_porta_r\n"); return 0; }
static READ_HANDLER(ci20_portb_r) { logerror("UNDOCUMENTED: ci20_portb_r\n"); return 0; }
// switch & dip reading
static READ_HANDLER(ci20_portc_r) {
	if (locals.swCol[0] > 2)
		return coreGlobals.swMatrix[locals.swCol[0]-2];
	else
		return core_getDip(locals.swCol[0]);
}

static READ_HANDLER(ci21_porta_r) { logerror("UNDOCUMENTED: ci21_porta_r\n"); return 0; }
static READ_HANDLER(ci21_portb_r) { logerror("UNDOCUMENTED: ci21_portb_r\n"); return 0; }
static READ_HANDLER(ci21_portc_r) { logerror("UNDOCUMENTED: ci21_portc_r\n"); return 0; }

static READ_HANDLER(ci22_porta_r) { logerror("UNDOCUMENTED: ci22_porta_r\n"); return 0; }
static READ_HANDLER(ci22_portb_r) { logerror("UNDOCUMENTED: ci22_portb_r\n"); return 0; }
static READ_HANDLER(ci22_portc_r) { logerror("UNDOCUMENTED: ci22_portc_r\n"); return 0; }

static READ_HANDLER(ci23_porta_r) { logerror("UNDOCUMENTED: ci23_porta_r\n"); return 0; }
static READ_HANDLER(ci23_portb_r) { logerror("UNDOCUMENTED: ci23_portb_r\n"); return 0; }
static READ_HANDLER(ci23_portc_r) { logerror("UNDOCUMENTED: ci23_portc_r\n"); return 0; }

// define switch column
static WRITE_HANDLER(ci20_porta_w) {
	if (data < 0xff)
		locals.swCol[0] = core_BitColToNum(data);
	else
		locals.swCol[0] = 6;
}
// (always 00?)
static WRITE_HANDLER(ci20_portb_w) {
	locals.solenoids |= data << 8;
}
// (always ff?)
static WRITE_HANDLER(ci20_portc_w) {
	coreGlobals.tmpLampMatrix[7] = data ^ 0xff;
}

// sols
static WRITE_HANDLER(ci21_porta_w) {
	locals.solenoids |= data ^ 0xee;
}
// lamps
static WRITE_HANDLER(ci21_portb_w) {
	coreGlobals.tmpLampMatrix[0] = data;
}
// lamps
static WRITE_HANDLER(ci21_portc_w) {
	coreGlobals.tmpLampMatrix[1] = data;
}

// lamps
static WRITE_HANDLER(ci22_porta_w) {
	coreGlobals.tmpLampMatrix[2] = data;
}
// lamps
static WRITE_HANDLER(ci22_portb_w) {
	coreGlobals.tmpLampMatrix[6] = data;
}
// lamps
static WRITE_HANDLER(ci22_portc_w) {
	coreGlobals.tmpLampMatrix[3] = data;
}

// lamps
static WRITE_HANDLER(ci23_porta_w) {
	coreGlobals.tmpLampMatrix[4] = data;
}
// lamps
static WRITE_HANDLER(ci23_portb_w) {
	coreGlobals.tmpLampMatrix[5] = data;
}
// display strobes
static WRITE_HANDLER(ci23_portc_w) {
  int i;
  if ((data & 0x0f) < 8)
    for (i=0; i < 7; i++)
      locals.segments[8*i + (data & 0x07)].w = locals.dispSeg[i];
}

// display data
static WRITE_HANDLER(disp16_w) {
  locals.dispSeg[offset] = data;
}

// sound
static WRITE_HANDLER(snd_w) {
  locals.sndCmd = data;
}

static ppi8255_interface ppi8255_intf =
{
	4+1,					/* 4 chips for CPU board + 1 chip for sound */
	{ci20_porta_r, ci23_porta_r, ci22_porta_r, ci21_porta_r, snd_porta_r},	/* Port A read */
	{ci20_portb_r, ci23_portb_r, ci22_portb_r, ci21_portb_r, snd_portb_r},	/* Port B read */
	{ci20_portc_r, ci23_portc_r, ci22_portc_r, ci21_portc_r, snd_portc_r},	/* Port C read */
	{ci20_porta_w, ci23_porta_w, ci22_porta_w, ci21_porta_w, snd_porta_w},	/* Port A write */
	{ci20_portb_w, ci23_portb_w, ci22_portb_w, ci21_portb_w, snd_portb_w},	/* Port B write */
	{ci20_portc_w, ci23_portc_w, ci22_portc_w, ci21_portc_w, snd_portc_w},	/* Port C write */
};

static MEMORY_READ_START(INDER_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x43ff, MRA_RAM},
  {0x4400,0x44ff, INDER_CMOS_r},
  {0x6000,0x6003, ppi8255_0_r},
  {0x6400,0x6403, ppi8255_1_r},
  {0x6800,0x6803, ppi8255_2_r},
  {0x6c00,0x6c03, ppi8255_3_r},
MEMORY_END

static MEMORY_WRITE_START(INDER_writemem)
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
  {0x6000,0x6003, ppi8255_0_w},
  {0x6400,0x6403, ppi8255_1_w},
  {0x6800,0x6803, ppi8255_2_w},
  {0x6c00,0x6c03, ppi8255_3_w},
  {0x6c20,0x6c20, snd_w},
  {0x6c60,0x6c66, disp16_w},
//{0x6ce0,0x6ce0, ?}, // unknown stuff here
MEMORY_END

static MACHINE_INIT(INDER) {
  memset(&locals, 0, sizeof locals);
}

MACHINE_DRIVER_START(INDER)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, INDER_CPUFREQ)
  MDRV_CPU_MEMORY(INDER_readmem, INDER_writemem)
  MDRV_CPU_VBLANK_INT(INDER_vblank, 1)
  MDRV_CPU_PERIODIC_INT(INDER_irq, INDER_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(INDER,NULL,NULL)
  MDRV_NVRAM_HANDLER(INDER)
  MDRV_DIPS(24)
  MDRV_SWITCH_CONV(INDER_sw2m,INDER_m2sw)
  MDRV_SWITCH_UPDATE(INDER)
MACHINE_DRIVER_END

static MEMORY_READ_START(INDER0_readmem)
  {0x0000,0x1fff, MRA_ROM},
  {0x4000,0x43ff, MRA_RAM},
  {0x4400,0x44ff, INDER_CMOS_r},
  {0x4800,0x4802, dip_r},
  {0x4805,0x4809, sw_r},
MEMORY_END

static MEMORY_WRITE_START(INDER0_writemem)
  {0x2000,0x20ff, disp_w},
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
//{0x4800,0x4805, ?}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4b00,0x4b00, ti76489_0_w},
MEMORY_END

MACHINE_DRIVER_START(INDER0)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER0_readmem, INDER0_writemem)
  MDRV_SOUND_ADD(SN76496, INDER_ti76489Int)
MACHINE_DRIVER_END

static MEMORY_READ_START(INDER1_readmem)
  {0x0000,0x1fff, MRA_ROM},
  {0x4000,0x43ff, MRA_RAM},
  {0x4400,0x44ff, INDER_CMOS_r},
  {0x4800,0x4802, dip_r},
  {0x4805,0x4809, sw_r},
  {0x4b01,0x4b01, ay8910_0_r },
MEMORY_END

static MEMORY_WRITE_START(INDER1_writemem)
  {0x2000,0x20ff, disp_w},
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
//{0x4800,0x4805, ?}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4b00,0x4b00, ay8910_0_ctrl_w },
  {0x4b02,0x4b02, ay8910_0_data_w },
MEMORY_END

MACHINE_DRIVER_START(INDER1)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER1_readmem, INDER1_writemem)
  MDRV_SOUND_ADD(AY8910, INDER_ay8910Int)
MACHINE_DRIVER_END

static INTERRUPT_GEN(inder2_snd_irq) {
  cpu_set_irq_line(INDER_SND_CPU, 0, PULSE_LINE);
}

static MEMORY_WRITE_START(INDER2_writemem)
  {0x2000,0x20ff, disp_w},
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x44ff, INDER_CMOS_w, &INDER_CMOS},
//{0x4800,0x4805, ?}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4a00,0x4a00, snd2_w},
  {0x4a01,0x4a01, disp2_w},
MEMORY_END

static MEMORY_READ_START(inder2_snd_readmem)
  {0x0000, 0x1fff, MRA_ROM},
  {0x8000, 0x87ff, MRA_RAM},
  {0x9001, 0x9001, ay8910_0_r},
  {0xa001, 0xa001, ay8910_1_r},
MEMORY_END

static MEMORY_WRITE_START(inder2_snd_writemem)
  {0x8000,0x87ff, MWA_RAM},
  {0x9000,0x9000, ay8910_0_ctrl_w},
  {0x9002,0x9002, ay8910_0_data_w},
  {0xa000,0xa000, ay8910_1_ctrl_w},
  {0xa002,0xa002, ay8910_1_data_w},
MEMORY_END

MACHINE_DRIVER_START(INDER2)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER0_readmem, INDER2_writemem)
  MDRV_CPU_ADD_TAG("scpu", Z80, 2000000)
  MDRV_CPU_PERIODIC_INT(inder2_snd_irq, INDER_IRQFREQ)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(inder2_snd_readmem, inder2_snd_writemem)
  MDRV_SOUND_ADD(AY8910, INDER_ay8910Int2)
MACHINE_DRIVER_END


// MSM sound board section

static struct {
	int ALO;
	int AHI;
	int CS;
	int PC0;
	int MSMDATA;
} sndlocals;

/* MSM5205 interrupt callback */
static void INDER_msmIrq(int data) {
	//Write data
	int mdata = sndlocals.MSMDATA>>(4*sndlocals.PC0);	//PC0 determines if lo or hi nibble is fed
	MSM5205_data_w(0, mdata&0x0f);
	//Flip it..
	sndlocals.PC0 = !sndlocals.PC0;
}

/* MSM5205 ADPCM CHIP INTERFACE */
static struct MSM5205interface INDER_msm5205Int = {
	1,					//# of chips
	384000,				//384Khz Clock Frequency?
	{INDER_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B},	//Sample Mode
	{80}				//Volume
};

static READ_HANDLER(snd_porta_r) { logerror(("SND1_PORTA_R\n")); return 0; }
static READ_HANDLER(snd_portb_r) { logerror(("SND1_PORTB_R\n")); return 0; }
static READ_HANDLER(snd_portc_r) {
	int data = sndlocals.PC0;
	return data;
}

static WRITE_HANDLER(snd_porta_w) {
	sndlocals.ALO = data;
}
static WRITE_HANDLER(snd_portb_w) {
	sndlocals.AHI = data;
}
static WRITE_HANDLER(snd_portc_w) {
	//Set Reset Line on the chip. This is done totally silly in the real machine;
	//They even used an R/C timer to pulse the reset line to avoid static!
	if ((data & 0xc0) == 0x40) MSM5205_reset_w(0, 1);
	if ((data & 0xc0) == 0x80) MSM5205_reset_w(0, 0);

	if (GET_BIT6)
		sndlocals.PC0 = 1;
	if (!GET_BIT7)
		sndlocals.PC0 = 1;
}

static WRITE_HANDLER(sndctrl_w) {
	sndlocals.CS = core_BitColToNum(data^0xff);
	//Read Data from ROM & Write Data To MSM Chip
	sndlocals.MSMDATA = (UINT8)*(memory_region(REGION_USER1) +
	  ((sndlocals.CS<<16) | (sndlocals.AHI<<8) | sndlocals.ALO));
}

static MACHINE_INIT(INDERS) {
	memset(&locals, 0, sizeof locals);
	memset(&sndlocals, 0, sizeof sndlocals);

	/* init PPI */
	ppi8255_init(&ppi8255_intf);
	/* init sound */
	sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(INDER_MEMREG_SND),NULL,NULL);
}

static MACHINE_STOP(INDERS) {
	sndbrd_0_exit();
}

static MEMORY_READ_START(indersnd_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_r},
	{ 0x8000, 0x8000, sndcmd_r },
MEMORY_END

static MEMORY_WRITE_START(indersnd_writemem)
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_w},
	{ 0x6000, 0x6000, sndctrl_w},
MEMORY_END

MACHINE_DRIVER_START(INDERS)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_ADD_TAG("scpu", Z80, 2500000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CORE_INIT_RESET_STOP(INDERS,NULL,INDERS)
  MDRV_CPU_MEMORY(indersnd_readmem, indersnd_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, INDER_msm5205Int)
MACHINE_DRIVER_END
