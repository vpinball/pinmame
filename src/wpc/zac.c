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
#define ZAC_IRQFREQ       2150 /* IRQ frequency (can someone confirm this?) */
#define ZAC_IRQFREQ_A     1850 /* IRQ frequency (can someone confirm this?) */

static WRITE_HANDLER(ZAC_soundCmd) { }
static void ZAC_soundInit(void) {}
static void ZAC_soundExit(void) {}

static struct {
  int swCol;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int diagnosticLed;
  int refresh;
  int gen;
} locals;

static INTERRUPT_GEN(ZAC_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids2 = locals.sols2;
  coreGlobals.solenoids = locals.solenoids;
  /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    //locals.diagnosticLed = 0;
  core_updateSw(core_getSol(8));
}

static SWITCH_UPDATE(ZAC) {
  if (inports) {
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[ZAC_COMINPORT] & 0xff);
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xfd) | (inports[ZAC_COMINPORT] >> 8);
  }
}

static int irq_callback(int int_level) {
//	logerror("callback!\n");
	cpu_set_irq_line(ZAC_CPUNO, 0, 0);
	return 0xbf;
}

//Generate the IRQ
static INTERRUPT_GEN(ZAC_irq) {
//	logerror("%x: IRQ\n",activecpu_get_previouspc());
	cpu_set_irq_line(ZAC_CPUNO, 0, PULSE_LINE);
//	return S2650_INT_IRQ;
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *ram1;

static NVRAM_HANDLER(ZAC1) {
    core_nvram(file, read_or_write, ram1, 0x400, 0x0f);
}

static UINT8 *ram;
static NVRAM_HANDLER(ZAC2) {
    core_nvram(file, read_or_write, ram, 0x800, 0x00);
}

static MACHINE_INIT(ZAC1) {
  memset(&locals, 0, sizeof(locals));
  locals.gen = 1;
  memset(ram1, 0xff, 0x400);

  if (coreGlobals.soundEn) ZAC_soundInit();
}

static MACHINE_INIT(ZAC2) {
  memset(&locals, 0, sizeof(locals));
  locals.gen = 2;

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback);

  if (coreGlobals.soundEn) ZAC_soundInit();
}

static MACHINE_INIT(ZAC2A) {
  memset(&locals, 0, sizeof(locals));
  locals.gen = 3;

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback);

  if (coreGlobals.soundEn) ZAC_soundInit();
}

static MACHINE_STOP(ZAC) {
  if (coreGlobals.soundEn) ZAC_soundExit();
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
	return ~(core_getDip(0)&0x0f); // 4Bit Dip Switch;
}

/*
   SENSE PORT: READ = Read Serial Input (hooked to printer)
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

/*   SENSE PORT: WRITE = Write Serial Output (hooked to printer) */
static WRITE_HANDLER(sense_port_w)
{
	logerror("%x: Sense Port Write=%x\n",activecpu_get_previouspc(),data);
}

static READ_HANDLER(ram_r) {
	if (offset == 0x05a6) // locks up some games if 0
		ram[offset] = 0xff;
	else {
		if (locals.gen < 3) {
			if (offset > 0x508 && offset < 0x549) {
				offset -= 0x509;
				ram[offset] = (coreGlobals.swMatrix[offset/8+1] & (1 << offset%8)) ? 0x0f : 0x00;
			}
		} else {
			if (offset > 0x51c && offset < 0x55d) {
				offset -= 0x51d;
				ram[offset] = (coreGlobals.swMatrix[offset/8+1] & (1 << offset%8)) ? 0x0f : 0x00;
			}
		}
	}
//	else logerror("ram_r: offset = %4x\n", offset);
	return ram[offset];
}

static WRITE_HANDLER(ram_w) {
	ram[offset] = data;
	if (offset > 0x43f && offset < 0x460) {
		UINT32 sol = 1 << (offset - 0x440);
		if (data)
			locals.solenoids |= sol;
		else
			locals.solenoids &= ~sol;
	} else if (offset > 0x45f && offset < 0x470) {
		UINT16 sol2 = 1 << (offset - 0x460);
		if (data)
			locals.sols2 |= sol2;
		else
			locals.sols2 &= ~sol2;
	} else if (offset > 0x46f && offset < 0x4c0) {
		offset -= 0x470;
		if (data)
			coreGlobals.tmpLampMatrix[offset/8] |= 1 << (offset%8);
		else
			coreGlobals.tmpLampMatrix[offset/8] &= ~(1 << (offset%8));
	} else if (offset > 0x4bf && offset < 0x4e8) {
		locals.segments[0x4e7 - offset].w = core_bcd2seg7[data & 0x0f];
		// handle comma in 5th display
		if (locals.segments[4].w > 0)
			locals.segments[4].w |= 0x80;
		else
			locals.segments[4].w &= 0x7f;
		if (locals.segments[2].w > 0)
			locals.segments[1].w |= 0x80;
		else
			locals.segments[1].w &= 0x7f;
	}
//	else logerror("ram_w: offset = %4x, data = %02x\n", offset, data);
}

static WRITE_HANDLER(ram1_w) {
	ram1[offset] = data;
	if (offset < 0x2e)
		locals.segments[offset].w = core_bcd2seg7[data & 0x0f];
}

static READ_HANDLER(ram1_r) {
	if (offset > 0x2d && offset < 0x34)
		ram1[offset] = ~coreGlobals.swMatrix[offset - 0x2d];
	return ram1[offset];
}

static WRITE_HANDLER(lamp_w) {
	ram1[offset + 0x80] = data;
	if (data)
		coreGlobals.tmpLampMatrix[offset/8] |= 1 << (offset%8);
	else
		coreGlobals.tmpLampMatrix[offset/8] &= ~(1 << (offset%8));
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
{ 0x0000, 0x17ff, MWA_ROM },
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
{ 0x3800, 0x3fff, ram_r },		/* RAM Mirror */
{ 0x4000, 0x57ff, MRA_ROM },
{ 0x5800, 0x5fff, ram_r },		/* RAM Mirror  */
{ 0x6000, 0x77ff, MRA_ROM },
{ 0x7800, 0x7fff, ram_r },		/* RAM Mirror */
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem2)
{ 0x0000, 0x17ff, MWA_ROM },
{ 0x1800, 0x1fff, ram_w },		/* RAM */
{ 0x2000, 0x37ff, MWA_ROM },
{ 0x3800, 0x3fff, ram_w, &ram },/* RAM Mirror */
{ 0x4000, 0x57ff, MWA_ROM },
{ 0x5800, 0x5fff, ram_w },		/* RAM Mirror */
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
   SENSE PORT: READ = Read Serial Input (hooked to printer)
   ------------------
   CTRL PORT : WRITE =	D0-D3 = Switch Strobes (4-8 Demultiplexed)
						D4=Pin 5 of CN? & /RUNEN = !D4
						D5=LED
						D6=Pin 15 of CN?
						D7=Pin 11 of CN?
   DATA PORT : WRITE = Sound Data 0-7
   SENSE PORT: WRITE = Write Serial Output (hooked to printer)
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
  MDRV_CORE_INIT_RESET_STOP(ZAC1,NULL,ZAC)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 6000000/4)
  MDRV_CPU_MEMORY(ZAC_readmem, ZAC_writemem)
  MDRV_CPU_PORTS(ZAC_readport, ZAC_writeport)
  MDRV_CPU_VBLANK_INT(ZAC_vblank, 1)
  MDRV_NVRAM_HANDLER(ZAC1)
  MDRV_DIPS(4)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(ZAC_soundCmd)
  MDRV_SOUND_CMDHEADING("ZAC")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC0)
  MDRV_IMPORT_FROM(ZAC1)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(ZAC_readmem0, ZAC_writemem0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ZAC2,NULL,ZAC)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 6000000/4)
  MDRV_CPU_MEMORY(ZAC_readmem2, ZAC_writemem2)
  MDRV_CPU_PORTS(ZAC_readport, ZAC_writeport)
  MDRV_CPU_VBLANK_INT(ZAC_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ)
  MDRV_NVRAM_HANDLER(ZAC2)
  MDRV_DIPS(4)
  MDRV_SWITCH_UPDATE(ZAC)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(ZAC_soundCmd)
  MDRV_SOUND_CMDHEADING("ZAC")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2A)
  MDRV_IMPORT_FROM(ZAC2)
  MDRV_CORE_INIT_RESET_STOP(ZAC2A,NULL,ZAC)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_A)
MACHINE_DRIVER_END
