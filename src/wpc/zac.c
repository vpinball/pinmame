/* Zaccaria Pinball*/
/* CPU: Signetics 2650 (32K Addressable CPU Space)
   Switch strobing & reading performed via the 2650 "fake" output ports (ie D/~C line)
   Lamps, Solenoids, and Display Data is accessed via 256x4 RAM, using
   DMA (Direct Memory Access) with timers to generate the enable bits for each hardware.
*/
#include <stdlib.h>
#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "zac.h"
#include "zacsnd.h"

#define ZAC_VBLANKFREQ    60 /* VBLANK frequency */
#define ZAC_OLDIRQFREQ   130 /* IRQ frequencies (guessed) */
#define ZAC_IRQFREQ     1466
#define ZAC_IRQFREQ_A   1833
#define ZAC_IRQFREQ_B   1933
#define ZAC_IRQFREQ_C   1866
#define ZAC_IRQFREQ_F   2066

static WRITE_HANDLER(ZAC_soundCmd) { }
static void ZAC_soundInit(void) {
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13136) {
    sndbrd_0_init(SNDBRD_ZAC1370, ZACSND_CPUA, memory_region(ZACSND_CPUAREGION), NULL, NULL);
    sndbrd_1_init(SNDBRD_ZAC13136, ZACSND_CPUB, memory_region(ZACSND_CPUBREGION), NULL, NULL);
  } else {
    sndbrd_0_init(core_gameData->hw.soundBoard, ZACSND_CPUA, memory_region(ZACSND_CPUAREGION), NULL, NULL);
  }
}
static void ZAC_soundExit(void) {
  sndbrd_0_exit();
  if (core_gameData->hw.soundBoard == SNDBRD_ZAC13136) {
    sndbrd_1_exit();
  }
}

static struct {
  int swCol;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int diagnosticLed;
  int diagnosticSLed;
  int actsnd;
  int actspk;
  int refresh;
  int gen;
  void *printfile;
} locals;

static INTERRUPT_GEN(ZAC_vblank_old) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids = locals.solenoids;
  /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    //locals.diagnosticLed = 0;
  core_updateSw(!core_getSol(26));
}

static INTERRUPT_GEN(ZAC_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  coreGlobals.solenoids2 = locals.sols2;
  coreGlobals.solenoids = locals.solenoids;
  /*update leds*/
    //coreGlobals.diagnosticLed = locals.diagnosticLed;
	coreGlobals.diagnosticLed = locals.diagnosticLed | (locals.diagnosticSLed<<1);
    //locals.diagnosticLed = 0;
  core_updateSw(coreGlobals.lampMatrix[2] & 0x08);
}

static SWITCH_UPDATE(ZAC1) {
  if (inports) {
    sndbrd_0_diag(core_getSw(-1));
    coreGlobals.swMatrix[0] = (inports[ZAC_COMINPORT] & 0x1000) >> 5;
    coreGlobals.swMatrix[1] = inports[ZAC_COMINPORT] & 0xff;
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0x7f) | ((inports[ZAC_COMINPORT] & 0xefff) >> 8);
  }
}

static SWITCH_UPDATE(ZAC2) {
  if (inports) {
    sndbrd_0_diag(core_getSw(-1));
    coreGlobals.swMatrix[0] = (inports[ZAC_COMINPORT] & 0x1000) >> 5;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[ZAC_COMINPORT] & 0xff);
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0x79) | ((inports[ZAC_COMINPORT] & 0xefff) >> 8);
  }
}

static int ZAC_sw2m(int no) {
	return no + 8;
}

static int ZAC_m2sw(int col, int row) {
	return col*8 + row - 8;
}

static int irq_callback(int int_level) {
	return 0xbf;
}

static int irq_callback_old(int int_level) {
	return 0x18;
}

//Generate the IRQ
static INTERRUPT_GEN(ZAC_irq) {
//	logerror("%x: IRQ\n",activecpu_get_previouspc());
	cpu_set_irq_line(ZAC_CPUNO, 0, PULSE_LINE);
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT8 *ram1;
static NVRAM_HANDLER(ZAC1) {
    core_nvram(file, read_or_write, ram1, 0x400, 0x00);
}

static UINT8 *ram;
static NVRAM_HANDLER(ZAC2) {
    core_nvram(file, read_or_write, ram, 0x800, 0x00);
}

static MACHINE_INIT(ZAC1) {
  static UINT8 presetRam[] = {
    0,0x03, 2,0x01, 3,0x0f, 5,0x01, 6,0x01,
    8,0x01, 10,0x02, 11,0x01, 12,0x01, 13,0x02, 15,0x03,
    17,0x05, 19,0x06, 21,0x06, 55,0x05, 56,0x0a
  };
  int i;

  memset(&locals, 0, sizeof(locals));
  locals.gen = 1;

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback_old);

  /* Preset RAM */
  if (ram1[0xf7] != 0x05 && ram1[0xf8] != 0x0a) {
	  for (i = 0; i < 16; i++) {
		ram1[0xc0 + presetRam[2*i]] = presetRam[2*i+1];
	  }
  }

  ZAC_soundInit();
}

static MACHINE_INIT(ZAC2) {
  memset(&locals, 0, sizeof(locals));
  locals.gen = 2;

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback);

  ZAC_soundInit();
}

static MACHINE_INIT(ZAC2A) {
  memset(&locals, 0, sizeof(locals));
  locals.gen = 3;

  /* Set IRQ Vector Routine */
  cpu_set_irq_callback(0, irq_callback);

  ZAC_soundInit();
}

static MACHINE_STOP(ZAC) {
  if (locals.printfile) {
    osd_fclose(locals.printfile);
    locals.printfile = NULL;
  }

  ZAC_soundExit();
}

/*   CTRL PORT : READ = D0-D7 = Switch Returns 0-7 */
static READ_HANDLER(ctrl_port_r)
{
	//logerror("%x: Ctrl Port Read\n",activecpu_get_previouspc());
//	logerror("%x: Switch Returns Read: Col = %x\n",activecpu_get_previouspc(),locals.swCol);
	return core_getSwCol(locals.swCol);
}

/*   DATA PORT : READ = D0-D3 = Dip Switch Read D0-D3 & Program Switch 1 on D3
						D4-D7 = ActSnd(D4) & ActSpk(D6) Pin 20,19,18,17 of CN?
*/
static READ_HANDLER(data_port_r)
{
	int data = core_getDip(0)&0x0f;
	data |= (locals.actsnd<<4);
	data |= (locals.actspk<<6);
	//logerror("%x: Data Port Read\n",activecpu_get_previouspc());
	//logerror("%x: Dip & ActSnd/Spk Read - Dips=%x\n",activecpu_get_previouspc(),core_getDip(0)&0x0f);
	//return 0xff;
	//return 0;
	//logerror("data_port_r: %x ~%x\n",data,~data);
	return ~data; // 4Bit Dip Switch;
}

/*
   SENSE PORT: READ = Read Serial Input (hooked to printer)
*/
static READ_HANDLER(sense_port_r)
{
	static UINT8 byte = 0xff;
	static int bitno = 7;
	if (++bitno > 7) {
		byte++;
		bitno = 0;
//		logerror("%x: Sense Port Read=%02x\n",activecpu_get_previouspc(),byte);
	}
	return (byte >> bitno) & 1;
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
	if(activecpu_get_previouspc() > 0x14a) {
		//logerror("%x: Sound Ctrl Write=%x\n",activecpu_get_previouspc(),data);
		//sndbrd_0_ctrl_w(ZACSND_CPUA, data);
	}
//	logerror("strobe data = %x, swcol = %x\n",data&0x07,locals.swCol);
//	logerror("refresh=%x\n",locals.refresh);
}

/*   DATA PORT : WRITE = Sound Data 0-7 */
static WRITE_HANDLER(data_port_w)
{
	sndbrd_0_data_w(ZACSND_CPUA, data);
	logerror("%x: Sound Data Write=%x\n",activecpu_get_previouspc(),data);
}

/*   SENSE PORT: WRITE = Write Serial Output (hooked to printer) */
static WRITE_HANDLER(sense_port_w)
{
	static UINT8 printdata[] = {0};
	static int bitno = 0;
	static int startbit = 0;
	if (locals.printfile == NULL) {
	  char filename[13];
	  sprintf(filename,"%s.prt", Machine->gamedrv->name);
	  locals.printfile = mame_fopen(Machine->gamedrv->name,filename,FILETYPE_PRINTER,2); // APPEND write mode
	}
	if (data && startbit < 1)
		startbit = 1;
	else if (data == 0 && startbit == 1)
		startbit = 2;
	else if (bitno < 8 && startbit > 1)
		printdata[0] |= (data << bitno++);
	if (bitno == 8 && startbit > 1)
    	if (locals.printfile) mame_fwrite(locals.printfile, printdata, 1);
    if (bitno > 7) {
		printdata[0] = 0;
		bitno = 0;
		startbit = 0;
	}
//	logerror("%x: Sense Port Write=%x\n",activecpu_get_previouspc(),data);
}

static READ_HANDLER(ram_r) {
	int off;
	static int key[64];
	if (offset == 0x05a6) // locks up some games if 0
		ram[offset] = 0xff;
	else {
		off = (locals.gen < 3) ? 0x508 : 0x51c;
		if (offset > off && offset < off + 0x41) {
			off = offset - off - 1;
			if (coreGlobals.swMatrix[off/8+1] & (1 << off%8))
				key[off]++;
			else
				key[off] = 0;
			if (!key[off]) ram[offset] = 0;
			else if (key[off] == 1) {
				if (!ram[offset]) ram[offset] = 0x01;
			} else if (key[off] == 2) {
				ram[offset] |= 0x02;
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
		if (data & 0x01)
			coreGlobals.tmpLampMatrix[offset/8] |= 1 << (offset%8);
		else
			coreGlobals.tmpLampMatrix[offset/8] &= ~(1 << (offset%8));
	} else if (offset > 0x4bf && offset < 0x4e8)
		locals.segments[0x4e7 - offset].w = core_bcd2seg7[data & 0x0f];
	else if (offset < 0x500) logerror("ram_w: offset = %4x, data = %02x\n", offset, data);
}

static WRITE_HANDLER(ram1_w) {
	ram1[offset] = data;
	if (offset < 0x2e)
		locals.segments[0x2f - offset].w = core_bcd2seg7[data & 0x0f];
	else if (offset > 0x3f && offset < 0x60) {
		UINT32 sol = 1 << (offset - 0x40);
		if (data)
			locals.solenoids |= sol;
		else
			locals.solenoids &= ~sol;
	} else if (offset > 0x7f && offset < 0xc0) {
		offset -= 0x80;
		if (data & 0x01)
			coreGlobals.tmpLampMatrix[offset/8] |= (1 << (offset%8));
		else
			coreGlobals.tmpLampMatrix[offset/8] &= ~(1 << (offset%8));
	}
}

static READ_HANDLER(ram1_r) {
	UINT8 value;
	int off = 0x2d;
	if (offset > off && offset < off + 0x07) {
		off = offset - off - 1;
		value = coreGlobals.swMatrix[off + 1];
		ram1[offset] = value;
	}
	return ram1[offset];
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
{ 0x1800, 0x1bff, ram1_w, &ram1 },	/* RAM */
{ 0x1c00, 0x1fff, MWA_ROM },
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
{ 0x1800, 0x1bff, ram1_w, &ram1 },	/* RAM */
{ 0x1c00, 0x1fff, MWA_ROM },
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

MACHINE_DRIVER_START(ZAC)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ZAC1,NULL,ZAC)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 6000000/4)
  MDRV_CPU_MEMORY(ZAC_readmem, ZAC_writemem)
  MDRV_CPU_PORTS(ZAC_readport, ZAC_writeport)
  MDRV_CPU_VBLANK_INT(ZAC_vblank_old, 1)
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_OLDIRQFREQ)
  MDRV_NVRAM_HANDLER(ZAC1)
  MDRV_DIPS(4)
  MDRV_SWITCH_UPDATE(ZAC1)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_SWITCH_CONV(ZAC_sw2m,ZAC_m2sw)
  MDRV_SOUND_CMD(ZAC_soundCmd)
  MDRV_SOUND_CMDHEADING("ZAC")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC1)
  MDRV_IMPORT_FROM(ZAC)
  MDRV_IMPORT_FROM(zac1346)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC0)
  MDRV_IMPORT_FROM(ZAC)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(ZAC_readmem0, ZAC_writemem0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC0S)
  MDRV_IMPORT_FROM(ZAC0)
  MDRV_IMPORT_FROM(zac1346)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2NS)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(ZAC2,NULL,ZAC)
  MDRV_CPU_ADD_TAG("mcpu", S2650, 6000000/4)
  MDRV_CPU_MEMORY(ZAC_readmem2, ZAC_writemem2)
  MDRV_CPU_PORTS(ZAC_readport, ZAC_writeport)
  MDRV_CPU_VBLANK_INT(ZAC_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ)
  MDRV_NVRAM_HANDLER(ZAC2)
  MDRV_DIPS(4)
  MDRV_SWITCH_UPDATE(ZAC2)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_SWITCH_CONV(ZAC_sw2m,ZAC_m2sw)
  MDRV_SOUND_CMD(ZAC_soundCmd)
  MDRV_SOUND_CMDHEADING("ZAC")
MACHINE_DRIVER_END

//Sound board 1370
MACHINE_DRIVER_START(ZAC2)
  MDRV_IMPORT_FROM(ZAC2NS)
  MDRV_IMPORT_FROM(zac1370)
MACHINE_DRIVER_END

//Sound board 13136
MACHINE_DRIVER_START(ZAC2X)
  MDRV_IMPORT_FROM(ZAC2NS)
  MDRV_IMPORT_FROM(zac13136)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2A)
  MDRV_IMPORT_FROM(ZAC2)
  MDRV_CORE_INIT_RESET_STOP(ZAC2A,NULL,ZAC)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_A)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2B)
  MDRV_IMPORT_FROM(ZAC2A)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_B)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2C)
  MDRV_IMPORT_FROM(ZAC2X)
  MDRV_CORE_INIT_RESET_STOP(ZAC2A,NULL,ZAC)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_C)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2F)
  MDRV_IMPORT_FROM(ZAC2C)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_F)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ZAC2FX)
  MDRV_IMPORT_FROM(ZAC2X)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(ZAC_irq, ZAC_IRQFREQ_F)
MACHINE_DRIVER_END

void UpdateZACSoundLED(int data)
{
	locals.diagnosticSLed = data;
}

void UpdateZACSoundACT(int data)
{
  locals.actspk = data & 0x01;
  locals.actsnd = (data>>1) & 0x01;
 // logerror("sound act = %x\n",data);
}