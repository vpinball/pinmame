/* Zaccaria Pinball*/
/* CPU: Signetics 2650 (32K Addressable CPU Space)
   Switch strobing & reading performed via the 2650 "fake" output ports (ie D/~C line)
   Lamps, Solenoids, and Display Data is accessed via 256x4 RAM, using
   DMA (Direct Memory Access) with timers to generate the enable bits for each hardware.
*/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/s2650/s2650.h"
#include "core.h"
#include "zac.h"

#define ZAC_VBLANKFREQ    60 /* VBLANK frequency */
#define ZAC_IRQFREQ      366 /* IRQ frequency (can someone confirm this?)*/

static WRITE_HANDLER(ZAC_soundCmd) { }
static void ZAC_soundInit(void) {}
static void ZAC_soundExit(void) {}

static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2;
  int swCol;
  int bcd[5];	//There are 5 Displays
  int lampadr1;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
  int initDone;
  int refresh;
} locals;

static void ZAC_dispStrobe(int mask) {
  int digit = locals.p1_a & 0xfc; //PA2-7 Selects Digits 1-6
  int ii,jj;
  logerror("digit = %x (%x,%x,%x,%x,%x)\n",digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4]);
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
		if (dispMask & 0x01) {
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg9[locals.bcd[jj]];
		}
    }
}

static INTERRUPT_GEN(ZAC_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % ZAC_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    //memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % ZAC_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % ZAC_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    //locals.diagnosticLed = 0;
  }
  core_updateSw(TRUE);
}

static SWITCH_UPDATE(ZAC) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[ZAC_COMINPORT];
  }
  /*-- Diagnostic buttons on CPU board --*/
  //if (core_getSw(ZAC_SWSOUNDDIAG)) cpu_set_nmi_line(ZAC_SCPU1NO, PULSE_LINE);
}

static int irq_callback(int int_level) {
	logerror("callback!\n");
	cpu_set_irq_line(ZAC_CPUNO, 0, 0);
	return 0xbf;
}

//Generate the IRQ
static INTERRUPT_GEN(ZAC_irq) {
	logerror("%x: IRQ\n",activecpu_get_previouspc());
	cpu_set_irq_line(ZAC_CPUNO, 0, 1);
//	return S2650_INT_IRQ;
}

static MACHINE_INIT(ZAC) {
//  if (locals.initDone) CORE_DOEXIT(ZAC_exit);

//  if (core_init(&ZACData)) return;
  memset(&locals, 0, sizeof(locals));

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback);

  if (coreGlobals.soundEn) ZAC_soundInit();

  locals.vblankCount = 1;
//  locals.initDone = TRUE;
}

static MACHINE_STOP(ZAC) {
  if (coreGlobals.soundEn) ZAC_soundExit();
  // core_exit();
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *ZAC_CMOS;

static NVRAM_HANDLER(ZAC) {
  //core_nvram(file, read_or_write, ZAC_CMOS, 0x100,0xff);
}

//4 top bits
static WRITE_HANDLER(ZAC_CMOS_w) {
  data |= 0x0f;
  ZAC_CMOS[offset] = data;
}


/*   CTRL PORT : READ = D0-D7 = Switch Returns 0-7 */
static READ_HANDLER(ctrl_port_r)
{
	//logerror("%x: Ctrl Port Read\n",activecpu_get_previouspc());
//	logerror("%x: Switch Returns Read: Col = %x\n",activecpu_get_previouspc(),locals.swCol);
	return core_getSwCol(locals.swCol);
}

/*   DATA PORT : READ = D0-D3 = Dip Switch Read D0-D3 & Program Switch 1 on D3
						D4-D7 = ActSnd & ActSpk? Pin 20,19,18,17 of CN?
*/
static READ_HANDLER(data_port_r)
{
	logerror("%x: Dip & ActSnd/Spk Read - Dips=%x\n",activecpu_get_previouspc(),core_getDip(0)&0x0f);
	//logerror("%x: Data Port Read\n",activecpu_get_previouspc());
	return core_getDip(0)&0x0f; // 4Bit Dip Switch;
}

/*
   SENSE PORT: READ = Read Serial Input (What is this hooked to?)
*/
static READ_HANDLER(sense_port_r)
{
	logerror("%x: Sense Port Read\n",activecpu_get_previouspc());
	return 0;
}

/*
   ------------------
   CTRL PORT : WRITE =	D0-D2 = Switch Strobes (3-8 Demultiplexed)
						D3=REFRESH Strobe??
						D4=Pin 5 of CN? & /RUNEN = !D4
						D5=LED
						D6=Pin 15 of CN?
						D7=Pin 11 of CN?
*/
static WRITE_HANDLER(ctrl_port_w)
{
	//logerror("%x: Ctrl Port Write=%x\n",activecpu_get_previouspc(),data);
	//logerror("%x: Switch Strobe & LED Write=%x\n",activecpu_get_previouspc(),data);
	locals.refresh = (data>>3)&1;
	locals.diagnosticLed = (data>>5)&1;
	locals.swCol = data & 0x07;
#if 0
	int tmp;
	locals.swCol = 0;
	//3-8 Demultiplexed
	for(tmp = 0; tmp < 3; tmp++) {
		if((data>>tmp)&1) {
			locals.swCol |= 1<<tmp;
		}
	}
	locals.swCol+=1;
#endif
//	logerror("strobe data = %x, swcol = %x\n",data&0x07,locals.swCol);
//	logerror("refresh=%x\n",locals.refresh);
}

/*   DATA PORT : WRITE = Sound Data 0-7 */
static WRITE_HANDLER(data_port_w)
{
	//logerror("%x: Data Port Write=%x\n",activecpu_get_previouspc(),data);
	logerror("%x: Sound Data Write=%x\n",activecpu_get_previouspc(),data);
}

/*   SENSE PORT: WRITE = ??*/
static WRITE_HANDLER(sense_port_w)
{
	logerror("%x: Sense Port Write=%x\n",activecpu_get_previouspc(),data);
}

static UINT8 *ram, *ram1;

static READ_HANDLER(ram_r) {
//	if (offset > 0xff) logerror("ram_r: offset = %4x\n", offset);
	if (offset == 0x05a6) // locks up some games if 0
		ram[offset] = 0xff;
	return ram[offset];
}

static WRITE_HANDLER(ram_w) {
	ram[offset] = data;
	if (offset > 0x46f && offset < 0x4c0) {
		if (data)
			coreGlobals.tmpLampMatrix[(offset-0x470) / 8] |= 1 << (offset % 8);
		else
			coreGlobals.tmpLampMatrix[(offset-0x470) / 8] &= ~(1 << (offset % 8));
	} // else if (offset > 0xff) logerror("ram_w: offset = %4x, data = %02x\n", offset, data);
}

static WRITE_HANDLER(ram1_w) {
	ram1[offset] = data;
}

static READ_HANDLER(ram1_r) {
	if (offset > 0x2d && offset < 0x34)
		ram1[offset] = ~coreGlobals.swMatrix[offset - 0x2d];
	return ram1[offset];
}

static WRITE_HANDLER(lamp_w) {
	ram1[offset + 0x80] = data;
	if (data)
		coreGlobals.tmpLampMatrix[offset / 8] |= 1 << (offset % 8);
	else
		coreGlobals.tmpLampMatrix[offset / 8] &= ~(1 << (offset % 8));
}

static WRITE_HANDLER(ram11_w) {
	ram1[offset + 0xc0] = data;
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*
Chip Select Decoding:
---------------------
A13 A12 A11 VMA
  L   L   L   L   L  H  H  H  H  H  H  H  = CS0  = (<0x800) & (0x4000 mirrored)
  L   L   H   L   H  L  H  H  H  H  H  H  = CS1  = (0x800)  & (0x4800 mirrored)
  L   H   L   L   H  H  L  H  H  H  H  H  = CS2  = (0x1000) & (0x5000 mirrored)
  L   H   H   L   H  H  H  L  H  H  H  H  = CS3  = (0x1800) & (0x5800 mirrored)
  H   L   L   L   H  H  H  H  L  H  H  H  = CS0* = (0x2000) & (0x6000 mirrored)
  H   L   H   L   H  H  H  H  H  L  H  H  = CS1* = (0x2800) & (0x6800 mirrored)
  H   H   L   L   H  H  H  H  H  H  L  H  = CS2* = (0x3000) & (0x7000 mirrored)
  H   H   H   L   H  H  H  H  H  H  H  L  = CS3* = (0x3800) & (0x7800 mirrored)

  *= Mirrored by hardware design

  CS0 = EPROM1
  CS1 = EPROM2
  CS2 = EPROM3 (OR EPROM2 IF 2764 EPROMS USED)
  CS3 = RAM

  2 Types of RAM:
  a) DMA WRITE ONLY RAM:
	 2104 (256x4)
     Read:	N/A			(because Data Outputs are not on Data Bus)
	 Write:	1800-18ff	(and mirrored addresses)
  b) BATTERY BACKED GAME RAM:
	 2114 & 6514 (1024x4) - Effective 8 bit RAM.
	 Read:	1800 - 1bff							(and mirrored addresses)
	 Write: 1800 - 18b0 WHEN DIP.SW4 NOT SET!	(and mirrored addresses)
	 Write: 1800 - 1bff WHEN DIP.SW4 SET!		(and mirrored addresses)
*/
static MEMORY_READ_START(ZAC_readmem)
{ 0x0000, 0x17ff, MRA_ROM },
{ 0x1800, 0x1bff, ram1_r },		/* RAM */
{ 0x1c00, 0x1fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem)
{ 0x0000, 0x13ff, MWA_ROM },
{ 0x1800, 0x187f, ram1_w },		/* RAM */
{ 0x1880, 0x18bf, lamp_w },
{ 0x18c0, 0x1bff, ram11_w },	/* RAM */
{ 0x1c00, 0x1fff, MWA_ROM },
{ 0x3800, 0x3bff, ram1_w, &ram1 },	/* RAM Mirror & Init*/
MEMORY_END

/* bigger RAM area from 0x1800 to 0x1fff */
static MEMORY_READ_START(ZAC_readmem2)
{ 0x0000, 0x17ff, MRA_ROM },
{ 0x1800, 0x1fff, ram_r },		/* RAM */
{ 0x2000, 0x37ff, MRA_ROM },
{ 0x3800, 0x3fff, ram_r },		/* RAM Mirror  */
{ 0x4000, 0x57ff, MRA_ROM },
{ 0x5800, 0x5fff, ram_r },		/* RAM Mirror  */
{ 0x6000, 0x77ff, MRA_ROM },
{ 0x7800, 0x7fff, ram_r },		/* RAM Mirror */
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem2)
{ 0x0000, 0x17ff, MWA_ROM },
{ 0x1800, 0x1fff, ram_w, &ram },/* RAM */
{ 0x2000, 0x37ff, MWA_ROM },
{ 0x3800, 0x3fff, ram_w },		/* RAM Mirror  */
{ 0x4000, 0x57ff, MWA_ROM },
{ 0x5800, 0x5fff, ram_w },		/* RAM Mirror  */
{ 0x6000, 0x77ff, MWA_ROM },
{ 0x7800, 0x7fff, ram_w },		/* RAM Mirror */
MEMORY_END

/* bigger RAM area from 0x1400 to 0x1bff */
static MEMORY_READ_START(ZAC_readmem0)
{ 0x0000, 0x13ff, MRA_ROM },
{ 0x1400, 0x17ff, ram_r },		/* RAM */
{ 0x1800, 0x1bff, ram1_r },		/* RAM */
{ 0x1c00, 0x1fff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem0)
{ 0x0000, 0x13ff, MWA_ROM },
{ 0x1400, 0x17ff, ram_w, &ram },	/* RAM */
{ 0x1800, 0x187f, ram1_w },		/* RAM */
{ 0x1880, 0x18bf, lamp_w },
{ 0x18c0, 0x1bff, ram11_w },	/* RAM */
{ 0x1c00, 0x1fff, MWA_ROM },
{ 0x3800, 0x3bff, ram1_w, &ram1 },	/* RAM Mirror & Init*/
MEMORY_END

/* PORT MAPPING
   ------------
   CTRL PORT : READ = D0-D7 = Switch Returns 0-7
   DATA PORT : READ = D0-D3 = Dip Switch Read D0-D3 & Program Switch 1 on D3
					  D4-D7 = ActSnd & ActSpk? Pin 20,19,18,17 of CN?
   SENSE PORT: READ = Read Serial Input (What is this hooked to?)
   ------------------
   CTRL PORT : WRITE =	D0-D3 = Switch Strobes (4-8 Demultiplexed)
						D4=Pin 5 of CN? & /RUNEN = !D4
						D5=LED
						D6=Pin 15 of CN?
						D7=Pin 11 of CN?
   DATA PORT : WRITE = Sound Data 0-7
   SENSE PORT: WRITE = ??
*/
static PORT_WRITE_START( ZAC_writeport )
	{ S2650_CTRL_PORT,  S2650_CTRL_PORT,  ctrl_port_w },
	{ S2650_DATA_PORT,  S2650_DATA_PORT,  data_port_w },
	{ S2650_SENSE_PORT, S2650_SENSE_PORT, sense_port_w },
PORT_END

static PORT_READ_START( ZAC_readport )
	{ S2650_CTRL_PORT, S2650_CTRL_PORT, ctrl_port_r },
	{ S2650_DATA_PORT, S2650_DATA_PORT, data_port_r },
	{ S2650_SENSE_PORT, S2650_SENSE_PORT, sense_port_r },
PORT_END

MACHINE_DRIVER_START(ZAC1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ZAC,NULL,ZAC)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 600000/4)
  MDRV_CPU_MEMORY(ZAC_readmem, ZAC_writemem)
  MDRV_CPU_PORTS(ZAC_readport, ZAC_writeport)
  MDRV_CPU_VBLANK_INT(ZAC_vblank, 1)
//  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ)
  MDRV_NVRAM_HANDLER(ZAC)
  MDRV_DIPS(4)
  MDRV_SWITCH_UPDATE(ZAC)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(ZAC_soundCmd)
  MDRV_SOUND_CMDHEADING("ZAC")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2)
  MDRV_IMPORT_FROM(ZAC1)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(ZAC_readmem2, ZAC_writemem2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC0)
  MDRV_IMPORT_FROM(ZAC1)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(ZAC_readmem0, ZAC_writemem0)
MACHINE_DRIVER_END

#if 0
static core_tData ZACData = {
  4, /* 4 Dips */
  ZAC_updSw, 1, ZAC_soundCmd, "ZAC",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

/* 3 x 2532 EPROM Configuration */
struct MachineDriver machine_driver_ZAC1 = {
  {{  CPU_S2650, 6000000/4, /* 6.00/4 = 1.5Mhz */
      ZAC_readmem, ZAC_writemem, ZAC_readport, ZAC_writeport,
      ZAC_vblank, 1, ZAC_irq, ZAC_IRQFREQ
  }},
  ZAC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, ZAC_init, CORE_EXITFUNC(ZAC_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  ZAC_nvram
};

/* 2 x 2764 EPROM Configuration */
struct MachineDriver machine_driver_ZAC2 = {
  {{  CPU_S2650, 6000000/4, /* 6.00/4 = 1.5Mhz */
      ZAC_readmem2, ZAC_writemem2, ZAC_readport, ZAC_writeport,
      ZAC_vblank, 1, ZAC_irq, ZAC_IRQFREQ
  }},
  ZAC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, ZAC_init2, CORE_EXITFUNC(ZAC_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  ZAC_nvram
};
#endif
