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
			INT: IRQ @ 250 Hz (4 ms) for the games up to Lap by Lap, slightly less for later games
		IO:      DMA for earlier games,
		         PIAs for later ones.
		DISPLAY: 6-digit or 7-digit 7-segment panels with direct segment access
		SOUND:	 TI76489 @ 2 MHz for Brave Team
				 AY8910 @ 2 MHz for Canasta86, 2x AY8910 on separate Z80 CPU for Lap By Lap,
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

  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % INDER_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

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
    coreGlobals.segments[8*i + 7-(data >> 3)].w = locals.dispSeg[i];
}
// secondary display (Lap By Lap and Moon Light)
static WRITE_HANDLER(disp2_w) {
  if (data & 0xf0) {
    coreGlobals.segments[51 - core_BitColToNum(data >> 4)].w = core_bcd2seg7[data & 0x0f];
  }
}

// external sound
static WRITE_HANDLER(snd2_w) {
  locals.sndCmd = data;
  cpu_set_nmi_line(INDER_SND_CPU, PULSE_LINE);
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.lampMatrix[offset] = data;
}

// It's hard to draw the line between solenoids and lamps, as they share the outputs...
static WRITE_HANDLER(sol_w) {
  locals.solenoids |= data;
}

static READ_HANDLER(sndcmd_r) {
  return locals.sndCmd;
}

/*-------------------------------------------------------
/ Brave Team: Using a TI76489 chip, similar to 76496.
/--------------------------------------------------------*/
struct SN76489interface INDER_ti76489Int = {
	1,	/* total number of chips in the machine */
	{ 2000000 },	/* base clock 2 MHz */
	{ 75 }	/* volume */
};
static WRITE_HANDLER(ti76489_0_w)	{ SN76489_0_w(0, core_revbyte(data)); }

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
	locals.solenoids |= data << 16;
}
// (always ff?)
static WRITE_HANDLER(ci20_portc_w) {
	coreGlobals.lampMatrix[7] = data ^ 0xff;
}

// solenoids
static WRITE_HANDLER(ci21_porta_w) {
	locals.solenoids |= data ^ 0xee;
}
// lamps
static WRITE_HANDLER(ci21_portb_w) {
	coreGlobals.lampMatrix[0] = data;
}
// lamps and solenoids, heavily mixed between games! Sorry, it won't get any better than this.
static WRITE_HANDLER(ci21_portc_w) {
	coreGlobals.lampMatrix[1] = data;
  locals.solenoids |= ((data & 0xf0) ^ 0xf0) << 4;
}

// lamps
static WRITE_HANDLER(ci22_porta_w) {
	coreGlobals.lampMatrix[2] = data;
}
// lamps
static WRITE_HANDLER(ci22_portb_w) {
	coreGlobals.lampMatrix[6] = data;
}
// lamps
static WRITE_HANDLER(ci22_portc_w) {
	coreGlobals.lampMatrix[3] = data;
}

// lamps
static WRITE_HANDLER(ci23_porta_w) {
	coreGlobals.lampMatrix[4] = data;
}
// lamps
static WRITE_HANDLER(ci23_portb_w) {
	coreGlobals.lampMatrix[5] = data;
}
// display strobes
static WRITE_HANDLER(ci23_portc_w) {
  int i;
  if ((data & 0x0f) < 8)
    for (i=0; i < 7; i++)
      coreGlobals.segments[8*i + (data & 0x07)].w = locals.dispSeg[i];
}

// display data
static WRITE_HANDLER(disp16_w) {
  locals.dispSeg[offset] = data;
}

// sound
static WRITE_HANDLER(snd_w) {
  locals.sndCmd = data;
}
// unknown, 0x21 on game start, 0x20 on game over
static WRITE_HANDLER(X6ce0_w) {
  if (data) logerror("Unknown write to 0x6ce0: %02x\n", data);
  coreGlobals.diagnosticLed = data & 1;
}

static ppi8255_interface ppi8255_intf1 =
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
//{0x4900,0x4900, MWA_NOP}, // unknown stuff here
  {0x6000,0x6003, ppi8255_0_w},
  {0x6400,0x6403, ppi8255_1_w},
  {0x6800,0x6803, ppi8255_2_w},
  {0x6c00,0x6c03, ppi8255_3_w},
  {0x6c20,0x6c20, snd_w},
  {0x6c60,0x6c66, disp16_w},
  {0x6ce0,0x6ce0, X6ce0_w},
MEMORY_END

static MACHINE_INIT(INDER) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_DRIVER_START(INDER)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 2500000)
  MDRV_CPU_MEMORY(INDER_readmem, INDER_writemem)
  MDRV_CPU_VBLANK_INT(INDER_vblank, 1)
  MDRV_CPU_PERIODIC_INT(INDER_irq, 250)
  MDRV_CORE_INIT_RESET_STOP(INDER,NULL,NULL)
  MDRV_NVRAM_HANDLER(INDER)
  MDRV_DIPS(24)
  MDRV_SWITCH_CONV(INDER_sw2m,INDER_m2sw)
  MDRV_SWITCH_UPDATE(INDER)
  MDRV_DIAGNOSTIC_LEDH(1)
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
//{0x4800,0x4805, MWA_NOP}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4b00,0x4b00, ti76489_0_w},
MEMORY_END

MACHINE_DRIVER_START(INDER0)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER0_readmem, INDER0_writemem)
  MDRV_SOUND_ADD(SN76489, INDER_ti76489Int)
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
//{0x4800,0x4805, MWA_NOP}, // unknown stuff here
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
//{0x4800,0x4805, MWA_NOP}, // unknown stuff here
  {0x4806,0x480a, sw_w},
  {0x4900,0x4900, sol_w},
  {0x4901,0x4907, lamp_w},
  {0x4a00,0x4a00, snd2_w},
  {0x4a01,0x4a01, disp2_w},
//{0x4b00,0x4b00, MWA_NOP}, // unknown stuff here
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

static MACHINE_INIT(INDER2) {
	memset(&locals, 0, sizeof locals);

	/* init sound */
	sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(INDER_MEMREG_SND),NULL,NULL);
	sndbrd_setManCmd(0, snd2_w);
}

static MACHINE_STOP(INDER2) {
	sndbrd_0_exit();
}

MACHINE_DRIVER_START(INDER2)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDER0_readmem, INDER2_writemem)
  MDRV_CORE_INIT_RESET_STOP(INDER2,NULL,INDER2)

  MDRV_CPU_ADD_TAG("scpu", Z80, 2000000)
  MDRV_CPU_PERIODIC_INT(inder2_snd_irq, 250) // not really accurate
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(inder2_snd_readmem, inder2_snd_writemem)

  MDRV_SOUND_ADD(AY8910, INDER_ay8910Int2)
MACHINE_DRIVER_END


// MSM sound board section

static struct {
	UINT8 ALO;
	UINT8 AHI;
	UINT8 CS;
	int PC0;
	int Reset;
	int Strobe;
} sndlocals;

static READ_HANDLER(INDERS_MSM5205_READROM) {
	UINT32 addr = (sndlocals.CS << 16) | (sndlocals.AHI << 8) | sndlocals.ALO;
	return *(memory_region(REGION_USER1) + addr);
}

/* MSM5205 interrupt callback */
static void INDER_msmIrq(int data) {
	//Write data
	if (!sndlocals.Reset) {
		//Read Data from ROM & Write Data To MSM Chip
		UINT8 mdata = INDERS_MSM5205_READROM(0) >> (4*sndlocals.PC0);	//PC0 determines if lo or hi nibble is fed
		MSM5205_data_w(0, mdata & 0x0f);
	}

	//Flip it..
	if (!sndlocals.Strobe)
		sndlocals.PC0 = !sndlocals.PC0;
}

/* MSM5205 ADPCM CHIP INTERFACE */
static struct MSM5205interface INDER_msm5205Int = {
	1,					//# of chips
	384000,				//384Khz Clock Frequency?
	{INDER_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B},	//Sample Mode
	{100}				//Volume
};

static READ_HANDLER(snd_porta_r) { logerror(("SND1_PORTA_R\n")); return 0; }
static READ_HANDLER(snd_portb_r) { logerror(("SND1_PORTB_R\n")); return 0; }
static READ_HANDLER(snd_portc_r) {
	return !sndlocals.PC0;
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
//	MSM5205_reset_w(0, GET_BIT6);
	MSM5205_playmode_w(0, GET_BIT5 ? MSM5205_S48_4B : MSM5205_S64_4B);

	if (GET_BIT6)
		sndlocals.PC0 = 1;

	sndlocals.Strobe = GET_BIT7;

	//Store reset value
	sndlocals.Reset = GET_BIT6;
}

static WRITE_HANDLER(sndctrl_w) {
	sndlocals.CS = core_BitColToNum(data^0xff);
}

static void init_common(void) {
	memset(&locals, 0, sizeof locals);
	memset(&sndlocals, 0, sizeof sndlocals);

	/* init sound */
	sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(INDER_MEMREG_SND),NULL,NULL);
	sndbrd_setManCmd(0, snd_w);
}

static MACHINE_INIT(INDERS1) {
	init_common();

	/* init PPI */
	ppi8255_init(&ppi8255_intf1);
}

static MEMORY_READ_START(indersnd_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x2fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_r},
	{ 0x8000, 0x8000, sndcmd_r },
MEMORY_END

static MEMORY_WRITE_START(indersnd_writemem)
	{ 0x2000, 0x2fff, MWA_RAM },
	{ 0x4000, 0x4900, ppi8255_4_w},
	{ 0x6000, 0x6000, sndctrl_w},
MEMORY_END

MACHINE_DRIVER_START(INDERS1)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(INDER_irq, 180) // any higher, and switches behave erratic

  MDRV_CPU_ADD_TAG("scpu", Z80, 2500000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CORE_INIT_RESET_STOP(INDERS1,NULL,INDER2)
  MDRV_CPU_MEMORY(indersnd_readmem, indersnd_writemem)

  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, INDER_msm5205Int)
MACHINE_DRIVER_END


/* Moon Light has slightly different PIA mappings, and an extra display */
static WRITE_HANDLER(ci21_porta_w0) {
  disp2_w(0, data);
}
static WRITE_HANDLER(ci21_portb_w0) {
  coreGlobals.lampMatrix[2] = data;
}
static WRITE_HANDLER(ci21_portc_w0) {
  coreGlobals.lampMatrix[3] = data;
}
static WRITE_HANDLER(ci22_porta_w0) {
  coreGlobals.lampMatrix[5] = ~data;
}
static WRITE_HANDLER(ci22_portb_w0) {
	locals.solenoids |= data ^ 0xee;
}
static WRITE_HANDLER(ci22_portc_w0) {
  coreGlobals.lampMatrix[4] = data ^ 0x0e;
	locals.solenoids |= (~data & 0x0c) << 6;
}
static WRITE_HANDLER(ci23_porta_w0) {
  coreGlobals.lampMatrix[1] = data ^ 0x80;
}
static WRITE_HANDLER(ci23_portb_w0) {
  coreGlobals.lampMatrix[0] = data;
}
static WRITE_HANDLER(ci23_portc_w0) {
  int i;
  if ((data & 0x0f) > 8)
    for (i=0; i < 5; i++)
      coreGlobals.segments[8*i + (data & 0x07)].w = locals.dispSeg[i];
}

static ppi8255_interface ppi8255_intf0 =
{
	4+1,					/* 4 chips for CPU board + 1 chip for sound */
	{ci20_porta_r, ci23_porta_r, ci22_porta_r, ci21_porta_r, snd_porta_r},	/* Port A read */
	{ci20_portb_r, ci23_portb_r, ci22_portb_r, ci21_portb_r, snd_portb_r},	/* Port B read */
	{ci20_portc_r, ci23_portc_r, ci22_portc_r, ci21_portc_r, snd_portc_r},	/* Port C read */
	{ci20_porta_w, ci23_porta_w0,ci22_porta_w0,ci21_porta_w0,snd_porta_w},	/* Port A write */
	{ci20_portb_w, ci23_portb_w0,ci22_portb_w0,ci21_portb_w0,snd_portb_w},	/* Port B write */
	{ci20_portc_w, ci23_portc_w0,ci22_portc_w0,ci21_portc_w0,snd_portc_w},	/* Port C write */
};

static MACHINE_INIT(INDERS0) {
  init_common();

	/* init PPI */
	ppi8255_init(&ppi8255_intf0);
}

MACHINE_DRIVER_START(INDERS0)
  MDRV_IMPORT_FROM(INDERS1)
  MDRV_CORE_INIT_RESET_STOP(INDERS0,NULL,INDER2)
MACHINE_DRIVER_END


// two MSM sound boards, just like on Bushido and the Spinball machines

static struct {
  int    S1_ALO;
  int    S1_AHI;
  int    S1_CS0;
  int    S1_CS1;
  int    S1_A16;
  int    S1_A17;
  int    S1_PC0;
  int    S1_MSMDATA;
  int    S1_Reset;

  int    S2_ALO;
  int    S2_AHI;
  int    S2_CS0;
  int    S2_CS1;
  int    S2_A16;
  int    S2_A17;
  int    S2_PC0;
  int    S2_MSMDATA;
  int    S2_Reset;
} sndlocals2;

static void INDER_S1_msmIrq(int data);
static void INDER_S2_msmIrq(int data);
static READ_HANDLER(INDER_S1_MSM5205_READROM);
static READ_HANDLER(INDER_S2_MSM5205_READROM);
static WRITE_HANDLER(INDER_S1_MSM5205_w);
static WRITE_HANDLER(INDER_S2_MSM5205_w);

/* MSM5205 ADPCM CHIP INTERFACE */
static struct MSM5205interface inder_msm5205Int2 = {
	2,										//# of chips (effects / music)
	640000,									//384Khz Clock Frequency according to schematic, but speech needs faster clock!
	{INDER_S1_msmIrq, INDER_S2_msmIrq},		//VCLK Int. Callback
	{MSM5205_S64_4B, MSM5205_S96_4B},		//Sample Mode
	{80,50}								//Volume
};

// (always ff? ignore it for now)
static WRITE_HANDLER(ci20_portb_w2) {
}

// returns the sound ready bit for Metal Man, no idea where it comes from, so always return HI bit
static READ_HANDLER(ci20_portb_r2) {
  return 0x02;
}

// display strobes are reversed on Metal Man
static WRITE_HANDLER(ci23_portc_w2) {
  int i;
  if ((data & 0x0f) < 7) {
    for (i=0; i < 7; i++)
      coreGlobals.segments[8*i + 7 - (data & 0x07)].w = locals.dispSeg[i];
  }
}

/*
SND CPU #1 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
    (IN)(P0)    - Detects nibble feeds to MSM5205
		(P1-P3) - Not Used?
		(P4)    - Ready Status to Main CPU?
		(P5)    - S1 Pin on MSM5205 (Sample Rate Select 1)
		(P6)    - Reset on MSM5205
		(P7)    - Not Used?
*/
static READ_HANDLER(snd1_porta_r) { /* LOGSND(("SND1_PORTA_R\n")); */ return 0; }
static READ_HANDLER(snd1_portb_r) { /* LOGSND(("SND1_PORTB_R\n")); */ return 0; }
static READ_HANDLER(snd1_portc_r) {
	int data = sndlocals2.S1_PC0;
  //LOGSND(("SND1_PORTC_R = %x\n",data));
	return data;
}

static WRITE_HANDLER(snd1_porta_w) { sndlocals2.S1_AHI = data; }
static WRITE_HANDLER(snd1_portb_w) { sndlocals2.S1_ALO = data; }
static WRITE_HANDLER(snd1_portc_w) {
  //LOGSND(("SND1_PORTC_W = %02x\n",data));

	//Set Reset Line on the chip
	MSM5205_reset_w(0, GET_BIT6);

	//PC0 = 1 on Reset
	if (GET_BIT6) {
		sndlocals2.S1_PC0 = 1;
	} else {
    //Read Data from ROM & Write Data To MSM Chip
		int msmdata = INDER_S1_MSM5205_READROM(0);
		INDER_S1_MSM5205_w(0,msmdata);
	}

	//Store reset value
	sndlocals2.S1_Reset = GET_BIT6;
}

/*
SND CPU #2 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
    (IN)(P0)    - Detects nibble feeds to MSM5205
		(P1-P3) - Not Used?
		(P4)    - Ready Status to Main CPU?
		(P5)    - S1 Pin on MSM5205 (Sample Rate Select 1)
		(P6)    - Reset on MSM5205
		(P7)    - Not Used?
*/
static READ_HANDLER(snd2_porta_r) { /* LOGSND(("SND2_PORTA_R\n")); */ return 0; }
static READ_HANDLER(snd2_portb_r) { /* LOGSND(("SND2_PORTB_R\n")); */ return 0; }
static READ_HANDLER(snd2_portc_r) {
	int data = sndlocals2.S2_PC0;
	//LOGSND(("SND2_PORTC_R = %x\n",data));
	return data;
}

static WRITE_HANDLER(snd2_porta_w) { sndlocals2.S2_AHI = data; }
static WRITE_HANDLER(snd2_portb_w) { sndlocals2.S2_ALO = data; }
static WRITE_HANDLER(snd2_portc_w) {
	//LOGSND(("SND2_PORTC_W = %02x\n",data));

	//Set Reset Line on the chip
	MSM5205_reset_w(1, GET_BIT6);

	//PC0 = 1 on Reset
	if (GET_BIT6) {
		sndlocals2.S2_PC0 = 1;
	} else {
    //Read Data from ROM & Write Data To MSM Chip
		int msmdata = INDER_S2_MSM5205_READROM(0);
		INDER_S2_MSM5205_w(0,msmdata);
	}

	//Store reset value
	sndlocals2.S2_Reset = GET_BIT6;
}

//Sound Control - CPU #1 & CPU #2 (identical)
//FROM MANUAL (AND TOTALLY WRONG!)
//Bit 0 = Chip Select Data Rom 1
//Bit 1 = Chip Select Data Rom 2
//Bit 2 = A16 Select  Data Roms
//Bit 3 = NC
//Bit 4 = A17 Select  Data Roms
//Bit 5 = NC
//Bit 6 = A18 Select  Data Roms
//Bit 7 = NC

//GUESSED FROM WATCHING EMULATION
//Bit 0 = A16 Select  Data Roms
//Bit 1 = A17 Select  Data Roms? Not used on Metal Man, but maybe on Moon Light?
//Bit 6 = Chip Select Data Rom 1
//Bit 7 = Chip Select Data Rom 2
// all other bits are HI all the time

static WRITE_HANDLER(sndctrl_1_w) {
	sndlocals2.S1_A16 = GET_BIT0;
	sndlocals2.S1_A17 = GET_BIT1;
	sndlocals2.S1_CS0 = GET_BIT6;
	sndlocals2.S1_CS1 = GET_BIT7;
}

static WRITE_HANDLER(sndctrl_2_w) {
	sndlocals2.S2_A16 = GET_BIT0;
	sndlocals2.S2_A17 = GET_BIT1;
	sndlocals2.S2_CS0 = GET_BIT6;
	sndlocals2.S2_CS1 = GET_BIT7;
}

static READ_HANDLER(INDER_S1_MSM5205_READROM) {
	int addr, data;
	addr = (sndlocals2.S1_CS1 << 18) | (sndlocals2.S1_A17 << 17) | (sndlocals2.S1_A16 << 16)
	  | (sndlocals2.S1_AHI << 8) | sndlocals2.S1_ALO;
	data = (UINT8)*(memory_region(REGION_USER1) + addr);
	return data;
}

static READ_HANDLER(INDER_S2_MSM5205_READROM) {
	int addr, data;
	addr = (sndlocals2.S2_CS1 << 18) | (sndlocals2.S2_A17 << 17) | (sndlocals2.S2_A16 << 16)
	  | (sndlocals2.S2_AHI << 8) | sndlocals2.S2_ALO;
	data = (UINT8)*(memory_region(REGION_USER2) + addr);
	return data;
}

static WRITE_HANDLER(INDER_S1_MSM5205_w) {
  sndlocals2.S1_MSMDATA = data;
}
static WRITE_HANDLER(INDER_S2_MSM5205_w) {
  sndlocals2.S2_MSMDATA = data;
}

/* MSM5205 interrupt callback */
static void INDER_S1_msmIrq(int data) {
  //Write data
  if (!sndlocals2.S1_Reset) {
    int mdata = sndlocals2.S1_MSMDATA>>(4*sndlocals2.S1_PC0);	//PC0 determines if lo or hi nibble is fed
    MSM5205_data_w(0, mdata&0x0f);
  }
  //Flip it..
  sndlocals2.S1_PC0 = !sndlocals2.S1_PC0;
}

/* MSM5205 interrupt callback */
static void INDER_S2_msmIrq(int data) {
  //Write data
  if (!sndlocals2.S2_Reset) {
    int mdata = sndlocals2.S2_MSMDATA>>(4*sndlocals2.S2_PC0);	//PC0 determines if lo or hi nibble is fed
    MSM5205_data_w(1, mdata&0x0f);
  }
  //Flip it..
  sndlocals2.S2_PC0 = !sndlocals2.S2_PC0;
}

static ppi8255_interface ppi8255_intf2 = {
	6, /* 4 chips for CPU board + 2 chips for sound */
	{ci20_porta_r, ci23_porta_r, ci22_porta_r, ci21_porta_r, snd1_porta_r, snd2_porta_r},	/* Port A read */
	{ci20_portb_r2,ci23_portb_r, ci22_portb_r, ci21_portb_r, snd1_portb_r, snd2_portb_r},	/* Port B read */
	{ci20_portc_r, ci23_portc_r, ci22_portc_r, ci21_portc_r, snd1_portc_r, snd2_portc_r},	/* Port C read */
	{ci20_porta_w, ci23_porta_w, ci22_porta_w, ci21_porta_w, snd1_porta_w, snd2_porta_w},	/* Port A write */
	{ci20_portb_w2,ci23_portb_w, ci22_portb_w, ci21_portb_w, snd1_portb_w, snd2_portb_w},	/* Port B write */
	{ci20_portc_w, ci23_portc_w2,ci22_portc_w, ci21_portc_w ,snd1_portc_w, snd2_portc_w},	/* Port C write */
};

static MACHINE_INIT(INDERS2) {
	memset(&locals, 0, sizeof locals);
	memset(&sndlocals2, 0, sizeof sndlocals2);

	/* init PPI */
	ppi8255_init(&ppi8255_intf2);

	/* init sound */
	sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(REGION_CPU2),NULL,NULL);
	sndbrd_setManCmd(0, snd_w);
}

// extra outputs, map to lamps
static WRITE_HANDLER(extra_w) {
	coreGlobals.lampMatrix[8 + offset] = data;
}

static MEMORY_READ_START(INDERS2_readmem)
  {0x0000,0x3fff, MRA_ROM},
  {0x4000,0x45ff, MRA_RAM},
  {0x6000,0x6003, ppi8255_0_r},
  {0x6400,0x6403, ppi8255_1_r},
  {0x6800,0x6803, ppi8255_2_r},
  {0x6c00,0x6c03, ppi8255_3_r},
MEMORY_END

static MEMORY_WRITE_START(INDERS2_writemem)
  {0x4000,0x43ff, MWA_RAM},
  {0x4400,0x45ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
//{0x4900,0x4900, MWA_NOP}, // unknown stuff here
  {0x6000,0x6003, ppi8255_0_w},
  {0x6400,0x6403, ppi8255_1_w},
  {0x6800,0x6803, ppi8255_2_w},
  {0x6c00,0x6c03, ppi8255_3_w},
  {0x6c20,0x6c20, snd_w},
  {0x6c40,0x6c45, extra_w},
  {0x6c60,0x6c66, disp16_w},
  {0x6ce0,0x6ce0, X6ce0_w},
MEMORY_END

//CPU #1 - SOUND EFFECTS SAMPLES
static MEMORY_READ_START(indersnd1_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_r},
	{ 0x8000, 0x8000, sndcmd_r},
MEMORY_END
static MEMORY_WRITE_START(indersnd1_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_w},
//{ 0x4900, 0x4900, MWA_NOP}, // unknown stuff here
	{ 0x6000, 0x6000, sndctrl_1_w},
MEMORY_END

//CPU #2 - MUSIC SAMPLES
static MEMORY_READ_START(indersnd2_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_5_r},
	{ 0x8000, 0x8000, sndcmd_r},
MEMORY_END
static MEMORY_WRITE_START(indersnd2_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x4003, ppi8255_5_w},
//{ 0x4900, 0x4900, MWA_NOP}, // unknown stuff here
	{ 0x6000, 0x6000, sndctrl_2_w},
MEMORY_END

MACHINE_DRIVER_START(INDERS2)
  MDRV_IMPORT_FROM(INDER)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(INDERS2_readmem, INDERS2_writemem)
  MDRV_CPU_PERIODIC_INT(INDER_irq, 180) // adjustable on real machine
  MDRV_CORE_INIT_RESET_STOP(INDERS2,NULL,INDER2)
  MDRV_NVRAM_HANDLER(generic_0fill)

  MDRV_CPU_ADD_TAG("scpu1", Z80, 5000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(indersnd1_readmem, indersnd1_writemem)

  MDRV_CPU_ADD_TAG("scpu2", Z80, 5000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(indersnd2_readmem, indersnd2_writemem)

  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, inder_msm5205Int2)
MACHINE_DRIVER_END
