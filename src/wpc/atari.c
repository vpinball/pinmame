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
#include "cpu/m6502/m6502.h"
#include "sndbrd.h"
#include "atari.h"

#define ATARI_VBLANKFREQ      60 /* VBLANK frequency in HZ*/
#define ATARI_IRQFREQ        488 /* IRQ interval in HZ */
#define ATARI_NMIFREQ        244 /* NMI frequency in HZ */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT8  diagnosticLed;
  int    soldisable;
  UINT8  testSwBits;
  UINT32 solenoids;
  core_tSeg segments, pseg;
  int    snd1Enable;

  UINT8  dmaCount;
  UINT8  toggle;
  UINT8  enable_irq;
} locals;

static UINT8 latch[] = {
  0, 0, 0, 0, 0
};

static INTERRUPT_GEN(ATARI1_nmihi) {
	cpu_set_nmi_line(ATARI_CPU, PULSE_LINE);
	locals.dmaCount++;
	if (locals.dmaCount > 1) {
		locals.dmaCount = 0;
		locals.toggle = !locals.toggle;
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
    memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
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
	/* every second NMI pulse, a DMA interrupt is triggered.
	   Only if the 7th bit of dip switch 2/4 has changed when
	   that dip is read, the game code will proceed! */
	return locals.toggle ? (dipram[offset] | 0x40) : (dipram[offset] & 0xbf);
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
	if (offset == 0x80) return latch[0];
	if (offset == 0x84) return latch[1];
	if (offset == 0x88) return latch[2];
	if (offset == 0x8C) return latch[3];
	return ram[offset];
}

/*static READ_HANDLER(test_r) {
	logerror("read 0x%04x\n", offset);
	return 0xFF;
}*/

// Gen 2
static WRITE_HANDLER(disp0_w) {
	locals.pseg[offset].w = core_bcd2seg7a[data & 0x0f];
}

static WRITE_HANDLER(disp1_w) {
	static const int dispPos[] = { 49, 1, 9, 21, 29, 41, 61, 69 };
	int ii;
	data &= 0x07;
	for (ii = 0; ii < 7; ii++) {
		locals.segments[dispPos[ii]+data].w = locals.pseg[ii].w;
	}
}

/* solenoids */
// Gen 1
static void setLatch(int no, UINT8 data, UINT8 mask, int cnt) {
	/* table to adjust for original Atari sol numbers */
	static const UINT32 solTable[] = {
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
	sndbrd_0_ctrl_w(0, (latch[0] & 0xF) | ((locals.snd1Enable & 1) << 4)); // Control line gets waveform and enable
}
static WRITE_HANDLER(latch1084_w) {
	setLatch(1, data, 0xf0, 4);
	sndbrd_0_data_w(0, (latch[2] & 0xF) | ((latch[1] & 0xF) << 4)); // Data line gets frequency and volume
}
static WRITE_HANDLER(latch1088_w) {
	setLatch(2, data, 0xf0, 4);
	sndbrd_0_data_w(0, (latch[2] & 0xF) | ((latch[1] & 0xF) << 4)); // Data line gets frequency and volume
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
static WRITE_HANDLER(soundg1_w) { // enable
	if (core_gameData->hw.gameSpecific1 & 2) { // spcrider / midearth
		locals.snd1Enable = 1;
		sndbrd_0_ctrl_w(0, (latch[0] & 0xF) | 0x10);
	}
}

static WRITE_HANDLER(audiog1_w) { // reset
	locals.snd1Enable = (data) ? 1 : 0; 
	if (core_gameData->hw.gameSpecific1 & 2) // spcrider / midearth
		locals.snd1Enable = 0;
	sndbrd_0_ctrl_w(0, (latch[0] & 0xF) | ((locals.snd1Enable & 1) << 4));
}
// Gen 2
static WRITE_HANDLER(sound0_w) {
	locals.diagnosticLed = data & 0x0f; /* coupled with waveform select */
	sndbrd_ctrl_w(0, data);
}

static WRITE_HANDLER(sound1_w) {
	sndbrd_data_w(0, data);
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
{0x6000,0x6fff,	audiog1_w},		/* audio reset */
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
  locals.enable_irq = 1;
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_SOUND1), NULL, NULL);
}

static MACHINE_INIT(ATARI1) {
  init_common();
  locals.snd1Enable = 0;
}

/* Middle Earth uses an unusual way of polling the test switch. */
static MACHINE_INIT(ATARI1A) {
  init_common();
  locals.testSwBits = 0x0f;
  locals.snd1Enable = 0;
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
  MDRV_CPU_ADD_TAG("mcpu", M6800, 780000)
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

/*-----------------------------------------
/  4x4 custom I/O -- the Atari "Pinball Slice"
/
/  4x4 shares no I/O with generations 1-3.  It uses Atari custom gate arrays
/  silkscreened PINBALL SLICE (schematic 039032-01, designators 12E and 13E).
/  Each is a 40-pin part with a 4-bit data bus, A0-A3 selecting 16 registers,
/  8 switch inputs and 16 outputs in four groups; two of them share a chip
/  select and split the byte, one driving D0-D3 and the other D4-D7.  Five such
/  pairs sit at $2000-$2400 and two further 16-register devices at $2600/$2700
/  whose identity is not yet established.
/
/  Only the register roles the game code proves are decoded here.  Everything
/  else is stored and logged, because no datasheet for this part is known to
/  exist and the CPU-board sheet of the schematic has never surfaced.
/------------------------------------------*/
#define ATARI4_SLICE_DEVS 7  /* $2000,$2100,$2200,$2300,$2400,$2600,$2700 */

/*-- CPU: Atari's house standard for 6502 boards of this era is a 12.096 MHz
/   crystal divided by 8.  Centipede, Asteroids, Asteroids Deluxe, Tempest,
/   Black Widow and Food Fight all use exactly that.
/
/   IRQ: 60 Hz, the AC line frequency.  A mains zero-crossing interrupt is the
/   usual arrangement for a pinball of this period, and this is a US 60 Hz
/   machine (the wiring diagram's transformer has 120 V and 100 V primaries).
/   It is also what the game itself demands: the interrupt handler costs a
/   near-constant 3328 cycles, so at 369 Hz it consumed 81% of the CPU and the
/   Forth layer managed under 5000 words in a minute of emulated time.  At
/   60 Hz the handler takes 13% and the game executes 1.1 million words in the
/   same minute, with no illegal opcodes and no execution outside ROM.  The
/   earlier 575 kHz / 500 Hz and 1 MHz / 500 Hz pairs had no stated basis. --*/
#define ATARI4_CPUFREQ 1512000
#define ATARI4_IRQFREQ 60

/*-- The game kicks $230C on every interrupt and inside its power-up busy-wait,
/   so a gap of a second means it has stopped running and the real board would
/   have reset.  Modelling that matters while both clocks above are guesses: a
/   stalled machine has to announce itself rather than sit there looking idle.
/   logerror is compiled out of release builds, so the reset is the signal. --*/
#define ATARI4_WDOG_IRQS (ATARI4_IRQFREQ)  /* ~1 s without a kick */
#define ATARI4_TEST_HOOKS 0  /* 1 = log every slice access */

#if ATARI4_TEST_HOOKS
#define SLICE_LOG logerror
#else
#define SLICE_LOG(...) ((void)0)
#endif

static UINT8 *atari4_taskram;   /* $1000-$17FF: eight 256-byte task pages */
static UINT8 *atari4_CMOS;
static int atari4_taskbank;
static void atari4_setbank(int bank);

/*-- [dev][reg-0x0e] -> switch matrix column, or 0 for "not a switch port".
/   Matrix columns are 1-based, so column n here is the game's column n-1. --*/
static const UINT8 atari4_swcol[ATARI4_SLICE_DEVS][2] = {
  /* $200E, $200F */ { 2, 1 },
  /* $210E, $210F */ { 4, 3 },
  /* $220E, $220F */ { 6, 5 },
  /* $230E, $230F */ { 7, 0 },
  /* $240E, $240F */ { 0, 0 },
  /* $260E, $260F */ { 0, 0 },
  /* $270E, $270F */ { 0, 0 },
};

static struct {
  UINT8 reg[ATARI4_SLICE_DEVS][16];
  int   wdog;   /* interrupts since the last $230C kick */
} slice;

static UINT8 slice_read(int dev, int offset) {
  int reg = offset & 0x0f;
  UINT8 data;
  if (offset & 0xf0)
    SLICE_LOG("4x4 slice R dev=%d OFFPAGE off=%02X\n", dev, offset & 0xff);
  /*-- The debounce routine at $9248 does not scan the switch columns in device
  /   order.  It reads their addresses from a pointer table at $8834, and that
  /   table is
  /
  /       col 0  $200F    col 1  $200E
  /       col 2  $210F    col 3  $210E
  /       col 4  $220F    col 5  $220E
  /       col 6  $230E
  /
  /   -- register F before register E within each device, and a seventh column
  /   on device 3.  Seven ports of eight give the 56 switches SW00-SW37, and
  /   the switch number the game dispatches on is col*8+bit (the identity table
  /   at $875F).  Deriving the column arithmetically from dev and reg gets the
  /   order wrong and loses $230E altogether, which leaves column 6 returning
  /   an output latch: the debounce inverts it with EOR #$FF and every switch in
  /   that column reads permanently closed.
  /
  /   The inputs have 470R pull-ups to +5V and the switches pull to switch
  /   ground, so a closed switch reads low and $9248 inverts before storing. --*/
  if (reg >= 0x0e && atari4_swcol[dev][reg - 0x0e]) {
    int col = atari4_swcol[dev][reg - 0x0e];
    data = ~coreGlobals.swMatrix[col];
    SLICE_LOG("4x4 switchmap dev=%d reg=%X -> col=%d matrix=%02X data=%02X\n",
              dev, reg, col, coreGlobals.swMatrix[col], data);
  }
  else
    data = slice.reg[dev][reg];
  SLICE_LOG("4x4 slice R dev=%d reg=%X data=%02X\n", dev, reg, data);
  return data;
}

static void slice_write(int dev, int offset, UINT8 data) {
  int reg = offset & 0x0f;
  if (offset & 0xf0)
    SLICE_LOG("4x4 slice W dev=%d OFFPAGE off=%02X data=%02X\n", dev, offset & 0xff, data);
  if (dev == 3 && reg == 0x0f && ((data >> 4) & 7) != 7
      && ((data >> 4) & 7) != atari4_taskbank)
    atari4_setbank((data >> 4) & 7);     /* all ones = hold */
  if (reg == 0x0c) {
    if (dev == 2) cpu_set_irq_line(ATARI_CPU, 0, CLEAR_LINE);  /* $220C: IRQ ack */
    if (dev == 3) { slice.wdog = 0; SLICE_LOG("4x4 watchdog kick\n"); }  /* $230C */
  }
  /*-- $91B9 blits all 33 output ports every second interrupt, so logging every
  /   write floods the trace.  Only transitions carry information. --*/
  /*-- Registers 0-7 of the first two devices are the score displays: sixteen
  /   BCD digits, decoded to segments in hardware (the schematic runs the
  /   slice's OUT2x/OUT3x lines straight into a ULN2803 and on to SEG1/SEG2, and
  /   there is no font table anywhere in the ROM).  $F in a digit blanks it,
  /   which is what an unused player's display is filled with. --*/
  /*-- Displays.  Registers 0-7 of the first two devices hold BCD, decoded to
  /   segments inside the slice -- there is no font table anywhere in the ROM.
  /   Each device is a nibble-paired slice: 12E takes BD4-BD7 and drives SEG1
  /   through ULN2803A 12D/A, 13E takes BD0-BD3 and drives SEG2 through 13D.
  /   So one register byte carries two digits, one per segment bus, and the
  /   register index is the digit position: four buses of eight. --*/
  if (dev <= 1 && reg <= 7) {
    coreGlobals.segments[dev * 16 + reg].w     = core_bcd2seg7a[data >> 4];
    coreGlobals.segments[dev * 16 + 8 + reg].w = core_bcd2seg7a[data & 0x0f];
  }

  /*-- Registers $0A and $0B of device 0 are the solenoids.  The slices are
  /   nibble-paired: two four-bit parts share each address, one wired to
  /   BD0-BD3 and the other to BD4-BD7, so one byte drives both.  Each part
  /   sends OUT00-OUT03 and OUT10-OUT13 into a UDN2981, and the schematic
  /   names all sixteen drivers:
  /
  /     $200A low  OUT00-03 (13E -> 13EA)  right flipper, upper/mid/lower pop
  /     $200A high OUT00-03 (12E -> 12E/A) left flipper, SOL 5, SOL 6, SOL 7
  /     $200B low  OUT10-13 (13E -> 13EA)  hole, ball feed, reset L/R bank
  /     $200B high OUT10-13 (12E -> 12E/A) extra game, SOL D, SOL E, SOL F
  /
  /   Measured pulse widths agree: of all 33 ports only $200A, $200B and
  /   $210A ever go high for under a dozen frames, and $200B bit 1 -- the ball
  /   feed -- fires exactly once, at the start of a game. --*/
  /*-- $2600-$260F is the POKEY.  The CPU board carries a CO12294 at position
  /   C, and the game drives its frequency and control registers through the
  /   same ramp engine that fades the lamps. --*/
  if (dev == 5) { atari4snd_w(offset & 0x0f, data); return; }

  /*-- $2604 is the tone.  It is slot 4 of a sixteen-channel ramp engine: the
  /   game programs a value, a delta and a count into three parallel arrays at
  /   $31A6, $31B6 and $31C6, $9588 steps them every tick into an image at
  /   $31D6, and $961E blits that image to $2600-$2607 and $2700-$2707.
  /
  /   Of all 33 ports it is the only one that behaves like a frequency rather
  /   than a lamp: 148 writes in one game took 142 distinct values.  During
  /   attract its low nibble stays 0 and only the high nibble moves; when a
  /   sound plays, the low nibble sweeps -- F, D, B, 9, 7, 5, 3, 1 -- every two
  /   frames while the high nibble ramps slowly underneath, which is the
  /   nibble pairing doing its usual job of putting one slice's four tone bits
  /   and the other slice's four lamp bits in one byte.
  /
  /   It is not emulated.  Turning that four-bit value into a pitch needs the
  /   divider's clock, and the sheet carrying it is one of the ones missing
  /   from the schematic set -- the slice pages we have are sheet 4 of 7. --*/

  /*-- Devices 2, 3 and 4 appear in the blit table only at registers 0, 1, $0A
  /   and $0B -- the four output groups of a nibble-paired slice, with nothing
  /   driving a segment bus.  Every bit of them measures as a lamp: the
  /   shortest median on-time among them is 32 frames, against under a dozen
  /   for the solenoids on $200A/$200B.  Twelve columns, 96 lamps. --*/
  if (dev >= 2 && dev <= 4 && (reg <= 1 || reg == 0x0a || reg == 0x0b))
    coreGlobals.lampMatrix[(dev - 2) * 4 + (reg <= 1 ? reg : 2 + (reg - 0x0a))] = data;

  if (dev == 0 && (reg == 0x0a || reg == 0x0b)) {
    int shift = (reg == 0x0a) ? 0 : 8;
    coreGlobals.solenoids = (coreGlobals.solenoids & ~(0xff << shift))
                          | ((UINT32)data << shift);
  }

  if (slice.reg[dev][reg] != data)
    SLICE_LOG("4x4 out dev=%d reg=%X %02X -> %02X\n", dev, reg, slice.reg[dev][reg], data);
  slice.reg[dev][reg] = data;
  SLICE_LOG("4x4 slice W dev=%d reg=%X data=%02X\n", dev, reg, data);
}

static READ_HANDLER(slice_r)   { return slice_read(offset >> 8, offset); }
static WRITE_HANDLER(slice_w)  { slice_write(offset >> 8, offset, data); }
static READ_HANDLER(slice2_r)  { return slice_read(5 + (offset >> 8), offset); }
static WRITE_HANDLER(slice2_w) { slice_write(5 + (offset >> 8), offset, data); }

/*-- 4x4 stores scores and audits in a 256x4 CMOS at $2500.  Every read site in
/   the ROM normalises with ORA #$F0 and masks back with AND #$0F, which is the
/   idiom for a nibble-wide part whose upper bits float high; the arithmetic on
/   it runs in decimal mode.  Only the low nibble is real, so only it is kept. --*/
/*-- The Forth task records sit at $1nE9-$1nFF within the mirrored RAM.  Their
/   dispatch stubs ($1nEF-$1nFF) are never written by any instruction in the
/   ROM -- verified by exhaustive search and by runtime watch -- yet the task
/   switcher copies a stub out of them on every switch.  The only arrangement
/   under which that can work is one where every task's stub region reads back
/   the live zero page, so a switch into a task that has not run yet inherits
/   the current interpreter state instead of zeroing it.
/
/   Only the stub region behaves this way; the rest of $1000-$17FF is the plain
/   2K mirror, which is what the power-up RAM test at $DD80 walks for adjacency.
/   A read/write handler is enough here because the stub region is copied as
/   data and never executed -- unlike $1000-$1041, which the test at $DF10 runs
/   from, and which is why the mirror itself has to be a bank. --*/
/*-- Zero page $E0-$FF is the current task's working context, banked.
/
/   Each task owns a 23-byte record at TASK-SLOT(i) = $1iE9:
/
/       +$00  $1iE9  parameter stack base      (S0 = $DE)
/       +$01  $1iEA  parameter stack pointer   (the stack itself is in zero page, indexed by X)
/       +$02  $1iEB  return stack base         (R0 = $6F, the 6502 hardware stack)
/       +$03  $1iEC  return stack pointer
/       +$04  $1iED  sleep timer, 2 bytes, decremented by 8 per tick at $9541
/       +$06  $1iEF  17-byte interpreter context: IP at $F8/$F9, and $FA-$FC
/
/   No instruction anywhere in the ROM writes that context -- verified by an
/   exhaustive opcode scan, by an exhaustive search for the 17-byte copy loop
/   shape, and by watching every write at runtime.  The save is not missing: it
/   is free, because zero page IS the current task's record.  Simply running a
/   task writes that task's state, and $96B9 (the variant START-TASK uses) reads
/   $E9 and $EB to start the incoming task on its own stack bases -- reads that
/   only make sense through the bank.
/
/   The task number is latched from $230F bits 4-6.  The switcher computes
/   task*16 | (image & $8F) and stores it there immediately before it touches
/   the record, which is the only reason that store sits at that point in the
/   routine.  All ones means "hold", which is why the display refresh at $8FDC
/   writes (image & $0F) | $70 instead of the image: it has to drive the low
/   nibble without disturbing the task selection.  The other two writers, $9064
/   and $91DF, write the image back unchanged and so preserve it.
/
/   This has to be a bank rather than a read/write handler.  NEXT ends with
/   "jmp $00FA", and $00FA-$00FC hold a self-modifying JMP (cfa) whose operand
/   bytes ARE the code field address the inner interpreter just fetched.  MAME
/   reads those operand bytes with cpu_readop_arg, which bypasses memory
/   handlers, so a handler-based window would run every task through whatever
/   code field page zero happened to hold.
/
/   The eight pages live in the 2K at $1000-$17FF, which the ROM also addresses
/   directly: that is how the scheduler reads another task's sleep timer at
/   $1iED and how the switcher reads the outgoing task's context at $1iEF. --*/
static void atari4_setbank(int bank) {
  atari4_taskbank = bank;
  cpu_setbank(2, atari4_taskram + (bank << 8));   /* $0000-$00FF */
  cpu_setbank(3, atari4_taskram + (bank << 8));   /* $0100-$01FF, the same page */
}

/*-- 4x4 stores scores and audits in a 256x4 CMOS at $2500.  Every read site in
/   the ROM normalises with ORA #$F0 and masks back with AND #$0F, which is the
/   idiom for a nibble-wide part whose upper bits float high, and the arithmetic
/   on it runs in decimal mode.  Only the low nibble is real, so only it is
/   kept. --*/
static NVRAM_HANDLER(ATARI4) {
  core_nvram(file, read_or_write, atari4_CMOS, 0x100, 0x00);
}
static READ_HANDLER(nvram4_r)  { return atari4_CMOS[offset]; }
static WRITE_HANDLER(nvram4_w) { atari4_CMOS[offset] = data & 0x0f; }

/*-----------------------------------------
/  Memory map for 4x4
/------------------------------------------*/
static MEMORY_READ_START(ATARI4_readmem)
  {0x0000,0x00ff,	MRA_BANK2},	/* the current task's page */
  {0x0100,0x01ff,	MRA_BANK3},	/* the same page again: A8 is not decoded */
  {0x0200,0x07ff,	MRA_RAM},
  {0x1000,0x17ff,	MRA_RAM},	/* the eight task pages, addressed directly */
  {0x2000,0x24ff,	slice_r},	/* five Pinball Slice pairs */
  {0x2500,0x25ff,	nvram4_r},	/* 256x4 CMOS */
  {0x2600,0x27ff,	slice2_r},	/* two further 16-register devices */
  {0x3000,0x37ff,	MRA_RAM},
  {0x8000,0xffff,	MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(ATARI4_writemem)
  {0x0000,0x00ff,	MWA_BANK2},
  {0x0100,0x01ff,	MWA_BANK3},
  {0x0200,0x07ff,	MWA_RAM},
  {0x1000,0x17ff,	MWA_RAM, &atari4_taskram},
  {0x2000,0x24ff,	slice_w},
  {0x2500,0x25ff,	nvram4_w, &atari4_CMOS},
  {0x2600,0x27ff,	slice2_w},
  {0x3000,0x37ff,	MWA_RAM},
  /*-- Writes above $8000 hit ROM and do nothing on the real board.  They must
  /   be mapped: MAME's unmapped-write handler stores into the region for a
  /   16-bit address bus, so a stray write would silently patch the loaded ROM
  /   image.  The game does make two, through a generic pointer-store routine
  /   at $836D/$8377 that lands on $FFF8.  ATARI1_writemem has the same guard. --*/
  {0x8000,0xffff,	MWA_NOP},
MEMORY_END

/*-- The IRQ handler at $9665 acknowledges by writing $01 to $220C as its very
/   first act, and kicks $230C with $10 on the way out.  $230C is also poked
/   twice inside the power-up self-test's busy-wait at $DFD1/$DFDD, where
/   nothing else happens at all -- a register written inside a delay loop is a
/   watchdog, not an interrupt control.  The previous enable/disable latch here
/   was invented when neither was known. --*/
static INTERRUPT_GEN(ATARI4_irq) {
  if (++slice.wdog > ATARI4_WDOG_IRQS) {
    logerror("4x4 watchdog timeout: $230C not kicked for %d interrupts\n", slice.wdog);
    slice.wdog = 0;
    cpu_set_reset_line(ATARI_CPU, PULSE_LINE);
    return;
  }
  cpu_set_irq_line(ATARI_CPU, 0, ASSERT_LINE);
}

static MACHINE_INIT(ATARI4) {
  memset(&locals, 0, sizeof locals);
  memset(&slice, 0, sizeof slice);
  memset(memory_region(ATARI_MEMREG_CPU), 0, 0x2000);
  /*-- $1000-$17FF is the $0000-$07FF chip again, because the board does not
  /   decode A12.  It has to be a bank rather than a read/write handler: the
  /   power-up RAM test at $DF10 copies self-modifying code into $1000 and
  /   does JMP $1000, and an opcode fetch bypasses memory handlers. --*/
  atari4_taskram = memory_region(ATARI_MEMREG_CPU) + 0x1000;
  atari4_setbank(0);
}

/*-- The cabinet switches are SW28 start, SW29 tilt, SW2A left coin, SW2B right
/   coin and SW2C test, per the playfield diagram's legend -- so they live in
/   column 6, not column 1.  Generation 2's update, which this was copied from,
/   writes column 1; under the 4x4 switch map that column is SW00-SW07, real
/   playfield switches, and the 0x62 mask forced five of them open every frame.
/   Tilt has no key assigned, so bit 1 is preserved along with SW2D-SW2F. --*/
static SWITCH_UPDATE(ATARI4) {
	if (inports) {
		coreGlobals.swMatrix[6] = (coreGlobals.swMatrix[6] & 0xe2)
			| ((inports[ATARI_COMINPORT] & 0x0004) ? 0x01 : 0)   /* Game       -> SW28 */
			| ((inports[ATARI_COMINPORT] & 0x0008) ? 0x04 : 0)   /* Left coin  -> SW2A */
			| ((inports[ATARI_COMINPORT] & 0x0010) ? 0x08 : 0)   /* Right coin -> SW2B */
			| ((inports[ATARI_COMINPORT] & 0x0081) ? 0x10 : 0);  /* Test       -> SW2C */
	}
}

MACHINE_DRIVER_START(ATARI4)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6502, ATARI4_CPUFREQ)
  MDRV_CPU_MEMORY(ATARI4_readmem, ATARI4_writemem)
  MDRV_CPU_VBLANK_INT(ATARI2_vblank, 1)
  MDRV_CPU_PERIODIC_INT(ATARI4_irq, ATARI4_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(ATARI4,NULL,ATARI)
  MDRV_NVRAM_HANDLER(ATARI4)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(ATARI4)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_CONV(ATARI_sw2m,ATARI_m2sw)
  MDRV_IMPORT_FROM(atari4s)
MACHINE_DRIVER_END
