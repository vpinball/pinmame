/************************************************************************************************
 Atari Pinball
 -----------------

   Generation 1: Hardware: (From Atarians -> Space Riders)
		CPU: M6800? (Fixed Frequency of 1MHz)
			 INT: NMI @ 250Hz

   Generation 2: Hardware: (Superman -> Hercules)
		CPU: M6800 (Alternating frequency of 1MHz & 0.667MHz)
			 INT: IRQ - Fixed @ 488Hz?
		IO: DMA (Direct Memory Access/Address)
		DISPLAY: 5x6 Digit 7 Segment Display
		SOUND:	 Frequency + Noise Generator, Programmable Timers

   Generation 3: Hardware: (Road Runner and beyond?)
		same as generation 2, but more dip switches (maybe related to prototype states)

************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "sndbrd.h"
#include "atari.h"

#define ATARI_VBLANKFREQ      60 /* VBLANK frequency in HZ*/
#define ATARI_IRQFREQ        488 /* IRQ interval in HZ */
#define ATARI_NMIFREQ        250 /* NMI frequency in HZ */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    diagnosticLed;
  int    soldisable;
  UINT8  testSwBits;
  UINT32 solenoids;
  core_tSeg segments, pseg;
} locals;

static int toggle = 0;
static INTERRUPT_GEN(ATARI1_nmihi) {
	static int dmaCount = 0;
	cpu_set_nmi_line(ATARI_CPU, PULSE_LINE);
	dmaCount++;
	if (dmaCount > 3) {
		dmaCount = 0;
		toggle = !toggle;
	}
}

static void ATARI2_irq(int state) {
  cpu_set_irq_line(ATARI_CPU, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static INTERRUPT_GEN(ATARI2_irqhi) {
  ATARI2_irq(1);
}

static WRITE_HANDLER(intack_w) {
  ATARI2_irq(0);
}

/* only needed for real hardware */
static WRITE_HANDLER(watchdog_w) {
//logerror("Watchdog reset!\n");
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(ATARI1_vblank) {
  locals.vblankCount++;

  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % ATARI_SOLSMOOTH) == 0) {
    locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % ATARI_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }

  //Flippers are activated differently for almost every pinball of this generation...
  core_updateSw(TRUE);
}

static INTERRUPT_GEN(ATARI2_vblank) {
  locals.vblankCount ++;

  /*-- lamps --*/
  if ((locals.vblankCount % ATARI_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % ATARI_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    if (locals.soldisable) locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % ATARI_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(ATARI0) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xc0) | (inports[ATARI_COMINPORT] & 0x0f) | ((inports[ATARI_COMINPORT] & 0x0300) >> 4);
		coreGlobals.swMatrix[0] = (inports[ATARI_COMINPORT] & 0x8000) >> 15;
	}
}

static SWITCH_UPDATE(ATARI1) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xf0) | (inports[ATARI_COMINPORT] & 0x0f);
		coreGlobals.swMatrix[3] = (coreGlobals.swMatrix[3] & 0xfc) | ((inports[ATARI_COMINPORT] & 0x0300) >> 8);
		coreGlobals.swMatrix[0] = (inports[ATARI_COMINPORT] & 0x8000) >> 15;
	}
}

static SWITCH_UPDATE(ATARI2) {
	if (inports) {
		coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x62) | (inports[ATARI_COMINPORT] & 0x9d);
	}
}

static int ATARI_sw2m(int no) {
	return no + 8;
}

static int ATARI_m2sw(int col, int row) {
	return col*8 + row - 8;
}

/* Switch reading */
// Gen 1
static READ_HANDLER(swg1_r) {
	/* the replay selector influences this readout */
	int i, dip2 = core_getDip(2);
	for (i=0; i < 4; i++) {
		core_setSw(ATARI_m2sw(8, 5+i), dip2 & (1 << i));
	}
	return (coreGlobals.swMatrix[1+(offset/8)] & (1 << (offset % 8))) ? 0xff : 0;
}

static READ_HANDLER(dipg1_r) {
	UINT8 *dipram = memory_region(ATARI_MEMREG_CPU) + 0x2000;
	dipram[offset] = (core_getDip(offset/8) & (1 << (offset % 8))) ? 0xff : 0;
	/* test switch and dip switch 1/1 are the same line */
	if (offset == 11) {
		dipram[offset] ^= locals.testSwBits;
		dipram[offset] = dipram[offset] ^ (coreGlobals.swMatrix[0] ? 0xff : 0x00);
	}
	if (offset)
		return dipram[offset];
	/* every fourth NMI pulse, a DMA interrupt is triggered.
	   Only if the 7th bit of dip switch 2/4 has changed when
	   that dip is read, the game code will proceed! */
	return toggle ? (dipram[offset] | 0x40) : (dipram[offset] & 0xbf);
}
static WRITE_HANDLER(dipg1_w) {
	/* In fact, some games try to do that bit toggling stuff by themselves,
	   only MAME has a problem handling this: The code does a ROL instruction,
	   so MAME will perform a read followed by a write to that address.
	   The read process will overwrite the data with the dip settings. */
	UINT8 *dipram = memory_region(ATARI_MEMREG_CPU) + 0x2000;
	dipram[offset] = data;
}

// Gen 2
static READ_HANDLER(sw_r) {
	return ~coreGlobals.swMatrix[offset+1];
}

static READ_HANDLER(dip_r)  {
	UINT8 *dipram = memory_region(ATARI_MEMREG_CPU) + 0x2000;
	dipram[offset] = ~core_getDip(offset);
	return dipram[offset];
}
static WRITE_HANDLER(dip_w)  {
	UINT8 *dipram = memory_region(ATARI_MEMREG_CPU) + 0x2000;
	dipram[offset] = data;
}
static READ_HANDLER(dip2_r)  {
	UINT8 *dipram2 = memory_region(ATARI_MEMREG_CPU) + 0x2008;
	dipram2[offset] = core_getDip(offset+4);
	return dipram2[offset];
}

/* display */
// Gen 1
static UINT8 *ram;

static WRITE_HANDLER(ram_w) {
	ram[offset] = data;
	if (offset < 0x10) {
		if (offset % 4 == 3) {
			/* 1up to 4up lights */
			if ((data & 0x0f) == 0x08)
				coreGlobals.lampMatrix[16] |= (0x08 >> (offset/4));
			else
				coreGlobals.lampMatrix[16] &= ~(0x08 >> (offset/4));
		} else {
			/* player score displays */
			locals.segments[30 - offset*2].w = core_bcd2seg7a[data >> 4];
			locals.segments[31 - offset*2].w = core_bcd2seg7a[data & 0x0f];
		}
	} else if (offset == 0x1c || offset == 0x1d) {
		/* match & credit display */
		offset -= 0x1c;
		locals.segments[32 + offset*2].w = core_bcd2seg7a[data >> 4];
		locals.segments[33 + offset*2].w = core_bcd2seg7a[data & 0x0f];
	} else if (offset > 0x2f && offset < 0x40) {
		/* lamps (128 of them!) */
		int col;
		offset -= 0x30;
		col = (offset%4)*2 + offset/8;
		if (offset % 8 < 4) {
			coreGlobals.lampMatrix[col] = (coreGlobals.lampMatrix[col] & 0xf0) | (data & 0x0f);
			coreGlobals.lampMatrix[col+8] = (coreGlobals.lampMatrix[col+8] & 0xf0) | (data >> 4);
		} else {
			coreGlobals.lampMatrix[col] = (coreGlobals.lampMatrix[col] & 0x0f) | (data << 4);
			coreGlobals.lampMatrix[col+8] = (coreGlobals.lampMatrix[col+8] & 0x0f) | (data & 0xf0);
		}
	}
}
static WRITE_HANDLER(ram_w1) {
	ram[offset+0x81] = data;
}
static WRITE_HANDLER(ram_w2) {
	ram[offset+0x85] = data;
}
static WRITE_HANDLER(ram_w3) {
	ram[offset+0x89] = data;
}
static WRITE_HANDLER(ram_w4) {
	ram[offset+0x8d] = data;
}

static READ_HANDLER(ram_r) {
	return ram[offset];
}
// Gen 2
static WRITE_HANDLER(disp0_w) {
	locals.pseg[offset].w = core_bcd2seg7a[data & 0x0f];
}

static WRITE_HANDLER(disp1_w) {
	static int dispPos[] = { 49, 1, 9, 21, 29, 41, 61, 69 };
	int ii;
	data &= 0x07;
	for (ii = 0; ii < 7; ii++) {
		locals.segments[dispPos[ii]+data].w = locals.pseg[ii].w;
	}
}

/* solenoids */
// Gen 1
static UINT8 latch[] = {
	0, 0, 0, 0, 0
};
static void setLatch(int no, UINT8 data, UINT8 mask, int cnt) {
	/* table to adjust for original Atari sol numbers */
	static UINT32 solTable[] = {
		0x00001, 0x00010, 0x04000, 0x00040,
		0x00002, 0x00020, 0x00008, 0x08000,
		0x00800, 0x00004, 0x00080, 0x02000,
		0x00400, 0x00200, 0x00100, 0x01000,
		0x20000, 0x10000, 0x40000, 0x80000
	};
	latch[no] = data;
	if (data & mask) {
		int i;
		for (i=0; i < cnt; i++) {
			if (data & (0x80 >> i))
				locals.solenoids |= solTable[4*no + i];
		}
	}
}
static WRITE_HANDLER(latch1080_w) {
	setLatch(0, data, 0xf0, 4);
}
static WRITE_HANDLER(latch1084_w) {
	setLatch(1, data, 0xf0, 4);
}
static WRITE_HANDLER(latch1088_w) {
	setLatch(2, data, 0xf0, 4);
}
static WRITE_HANDLER(latch108c_w) {
	setLatch(3, data, 0xff, 8);
}
/* Additional outputs on Time 2000 */
static WRITE_HANDLER(latch508c_w) {
	latch[4] = data;
	if (data) {
		locals.solenoids |= (data << 20);
	}
}

static READ_HANDLER(latch1080_r) {
	return latch[0];
}
static READ_HANDLER(latch1084_r) {
	return latch[1];
}
static READ_HANDLER(latch1088_r) {
	return latch[2];
}
static READ_HANDLER(latch108c_r) {
	return latch[3];
}
// Gen 2
static WRITE_HANDLER(sol0_w) {
	data &= 0x0f;
	if (data) {
		locals.solenoids |= 1 << (data-1);
		locals.soldisable = 0;
	} else locals.soldisable = 1;
}

static WRITE_HANDLER(sol1_w) {
	if (offset != 7) {
		if (data & 0x01) locals.solenoids |= (1 << (15 + offset));
		else locals.solenoids &= ~(1 << (15 + offset));
	}
}

/* lamps */
// Gen 1 (handled by display routine, see ram_w)
// Gen 2
static WRITE_HANDLER(lamp_w) {
	coreGlobals.tmpLampMatrix[offset] = data;
}

/* sound */
// Gen 1
static WRITE_HANDLER(soundg1_w) {
	logerror("Play sound %2x\n", data);
	sndbrd_0_data_w(0, data);
}

static WRITE_HANDLER(audiog1_w) {
//	logerror("Audio Reset %2x\n", data);
	sndbrd_0_data_w(0, 0);
}
// Gen 2
static WRITE_HANDLER(sound0_w) {
	locals.diagnosticLed = data & 0x0f; /* coupled with waveform select */
	sndbrd_0_ctrl_w(1, data);
}

static WRITE_HANDLER(sound1_w) {
	sndbrd_0_data_w(0, data);
}

/*-----------------------------------------
/  Memory map for CPU board (GENERATION 1)
/------------------------------------------*/
static MEMORY_READ_START(ATARI1_readmem)
{0x0000,0x01ff, ram_r},			/* RAM */
{0x0200,0x0200,	MRA_ROM},		/* fake NVRAM */
{0x1080,0x1080,	latch1080_r},	/* read latches */
{0x1084,0x1084,	latch1084_r},	/* read latches */
{0x1088,0x1088,	latch1088_r},	/* read latches */
{0x108c,0x108c,	latch108c_r},	/* read latches */
{0x2000,0x200f,	dipg1_r},		/* dips */
{0x2010,0x204f,	swg1_r},		/* inputs */
{0x7000,0x7fff,	MRA_ROM},		/* ROM */
{0xf000,0xffff,	MRA_ROM},		/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI1_writemem)
{0x0000,0x01ff, ram_w, &ram},	/* RAM */
{0x0200,0x0200,	MWA_RAM, &generic_nvram, &generic_nvram_size},	/* fake NVRAM */
{0x1000,0x107f, ram_w},			/* RAM mirror, on Middle Earth only */
{0x1080,0x1080,	latch1080_w},	/* solenoids */
{0x1081,0x1083, ram_w1},		/* RAM mirror, on Middle Earth only */
{0x1084,0x1084,	latch1084_w},	/* solenoids */
{0x1085,0x1087, ram_w2},		/* RAM mirror, on Middle Earth only */
{0x1088,0x1088,	latch1088_w},	/* solenoids */
{0x1089,0x108b, ram_w3},		/* RAM mirror, on Middle Earth only */
{0x108c,0x108c,	latch108c_w},	/* solenoids */
{0x108d,0x11ff, ram_w4},		/* RAM mirror, on Middle Earth only */
{0x2000,0x200f,	dipg1_w},		/* dip switch memory area is written to by code */
{0x3000,0x3000,	soundg1_w},		/* audio enable */
{0x4000,0x4000,	watchdog_w},	/* watchdog reset? */
{0x508c,0x508c,	latch508c_w},	/* additional solenoids, on Time 2000 only */
{0x6000,0x6000,	audiog1_w},		/* audio reset */
{0xffff,0xffff,	MWA_NOP},		/* Middle Earth writes here */
MEMORY_END

// Gen 2 memory handling - RAM and NVRAM is mirrored!
static UINT8 *atari_RAM, *atari_CMOS;
static NVRAM_HANDLER(ATARI) {
  core_nvram(file, read_or_write, atari_CMOS, 0x100, 0x00);
}
static READ_HANDLER(ram2_r) { return atari_RAM[offset]; }
static WRITE_HANDLER(ram2_w) { atari_RAM[offset] = data; }
// NVRAM only uses bottom nybble
static READ_HANDLER(nvram_r) { return atari_CMOS[offset]; }
static WRITE_HANDLER(nvram_w) { atari_CMOS[offset] = data & 0x0f; }

/*-----------------------------------------
/  Memory map for CPU board (GENERATION 2)
/------------------------------------------*/
static MEMORY_READ_START(ATARI2_readmem)
{0x0000,0x00ff,	ram2_r},	/* RAM */
{0x0100,0x01ff,	ram2_r},	/* mirrored RAM */
{0x0800,0x08ff,	nvram_r},	/* NVRAM */
{0x0900,0x09ff,	nvram_r},	/* mirrored NVRAM */
{0x1000,0x1007,	sw_r},		/* inputs */
{0x2000,0x2003,	dip_r},		/* dip switches */
{0x2008,0x200b,	dip2_r},	/* more dip switches??? Road Runner only! */
{0x2800,0x3fff,	MRA_ROM},	/* ROM */
{0xa800,0xbfff,	MRA_ROM},	/* ROM */
{0xe800,0xffff,	MRA_ROM},	/* reset vector */
MEMORY_END

static MEMORY_WRITE_START(ATARI2_writemem)
{0x0000,0x00ff,	ram2_w, &atari_RAM},	/* RAM */
{0x0100,0x01ff,	ram2_w},	/* mirrored RAM */
{0x0700,0x07ff,	ram2_w},	/* mirrored RAM */
{0x0800,0x08ff,	nvram_w, &atari_CMOS},	/* NVRAM */
{0x1800,0x1800,	sound0_w},	/* sound */
{0x1820,0x1820,	sound1_w},	/* sound */
{0x1840,0x1846,	disp0_w},	/* display data output */
{0x1847,0x1847,	disp1_w},	/* display digit enable & diagnostic LEDs */
{0x1860,0x1867,	lamp_w},	/* lamp output */
{0x1880,0x1880,	sol0_w},	/* solenoid output */
{0x18a0,0x18a7,	sol1_w},	/* solenoid enable & independent control output */
{0x18c0,0x18c1,	watchdog_w},/* watchdog reset */
{0x18e0,0x18e0,	intack_w},	/* interrupt acknowledge (resets IRQ state) */
{0x2000,0x2003,	dip_w},		/* dip switch memory area is written to by code */
MEMORY_END

static void init_common(void) {
  memset(&locals, 0, sizeof locals);
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_SOUND1), NULL, NULL);
}

static MACHINE_INIT(ATARI1) {
  init_common();
}

/* Middle Earth uses an unusual way of polling the test switch. */
static MACHINE_INIT(ATARI1A) {
  init_common();
  locals.testSwBits = 0x0f;
}

static MACHINE_INIT(ATARI2) {
  init_common();
}

static MACHINE_STOP(ATARI) {
  sndbrd_0_exit();
}

MACHINE_DRIVER_START(ATARI1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 1000000)
  MDRV_CPU_MEMORY(ATARI1_readmem, ATARI1_writemem)
  MDRV_CPU_VBLANK_INT(ATARI1_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ATARI1_nmihi, ATARI_NMIFREQ)
  MDRV_CORE_INIT_RESET_STOP(ATARI1,NULL,ATARI)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(20)
  MDRV_SWITCH_UPDATE(ATARI1)

  MDRV_IMPORT_FROM(atari1s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("ATARI1")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ATARI0)
  MDRV_IMPORT_FROM(ATARI1)
  MDRV_SWITCH_UPDATE(ATARI0)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ATARI1A)
  MDRV_IMPORT_FROM(ATARI1)
  MDRV_CORE_INIT_RESET_STOP(ATARI1A,NULL,ATARI)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ATARI2)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 1000000)
  MDRV_CPU_MEMORY(ATARI2_readmem, ATARI2_writemem)
  MDRV_CPU_VBLANK_INT(ATARI2_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ATARI2_irqhi, ATARI_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(ATARI2,NULL,ATARI)
  MDRV_NVRAM_HANDLER(ATARI)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(ATARI2)
  MDRV_DIAGNOSTIC_LEDH(4)
  MDRV_SWITCH_CONV(ATARI_sw2m,ATARI_m2sw)

  MDRV_IMPORT_FROM(atari2s)
  MDRV_SOUND_CMD(sndbrd_0_data_w)
  MDRV_SOUND_CMDHEADING("ATARI2")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(ATARI3)
  MDRV_IMPORT_FROM(ATARI2)
  MDRV_DIPS(64)
MACHINE_DRIVER_END
