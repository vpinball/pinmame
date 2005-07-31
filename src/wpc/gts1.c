/************************************************************************************************
 Gottlieb System 1
 -----------------

   Hardware:
   ---------
		CPU:     PPS/4 @ 198.864 kHz
		IO:      PPS/4 Ports
		DISPLAY: 4 x 6 Digit 9 segment panels, 1 x 4 Digit 7 segment panels
		SOUND:	 Chimes & System80 sound only board on later games

WARNING: This driver is currently nothing but a huge, gigantic hack!

We weren't able to get a read off the integrated ROMs inside the A1752 and A1753 chips,
so I tried and reverse-engineered the PGOL language. It only consists of 16 opcodes,
a few of which are still unknown in purpose (treated as NOP instructions right now).
The entire game logic is provided by a single PROM only 1024 x 4bit in size!
To overcome this pretty slim codebase, Rockwell came up with an interesting language
called "PGOL" (Pinball Game Oriented Language).
It basically does only very few things, like: check if lamp x-y is on or off, and set a flag
depending on the lamp state. The actions taken based on this are conditional, so every lamp
that's supposed to be become lit or unlit after this check will check the flag if it's
supposed to be executed at all.
Also, the game identification PROM will only apply to the current player, and the current
ball, and it is unknown how data (like bonus lamps and such) is being stored for all players,
because of the missing basic PPS4 code.
PGOL interpretation is based on assumptions *only*, so it's pretty surely still way off!
But in time, we'll iron out as many bugs as possible...
************************************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "gts1.h"
#include "gts80.h"
#include "sndbrd.h"
#include "gts80s.h"

#if 0
#define TRACE(x) printf x
#else
#define TRACE(x) logerror x
#endif

#define GTS1_VBLANKFREQ  60 /* VBLANK frequency in HZ*/

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    solCount;
  int    diagnosticLed;
  int    tmpSwCol;
  UINT32 solenoids;
  UINT8  tmpLampData;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  mame_timer* sleepTimer;
} locals;

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(GTS1_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % GTS1_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((++locals.solCount % GTS1_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

  /*-- display --*/
  if ((locals.vblankCount % GTS1_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(1);
}

static int codeOffset;

/* enable the execution timer */
static void exec_pgol(int address) {
  codeOffset = address;
  timer_adjust(locals.sleepTimer, 0.005, 0, 0.005);
}

/* This routine will be called whenever the execution timer is active */
static int accu;
static int ifActive;
static int score;
static void sleepTimer(int data) {
  UINT8* mem = memory_region(GTS1_MEMREG_CPU);
  int data0 = mem[codeOffset] & 0x0f;
  int data1 = mem[codeOffset+1] & 0x0f;
  int data2 = mem[codeOffset+2] & 0x0f;
  int data3 = mem[codeOffset+3] & 0x0f;
  switch (data0) {
    case 0x00: // check lamp and increment
      if (!ifActive) accu = 1;
      ifActive = 1;
      accu &= ((locals.lampMatrix[data2] & (1 << (7 & data1))) > 0 == (data1 >> 3));
      TRACE(("%03x: 0%x%x   andif %s %x-%x  (ac=%x)\n", codeOffset, data1, data2, (data1 & 8 ? "on ":"off"), data2, (data1 & 7), accu));
      exec_pgol(codeOffset + 3);
      break;
    case 0x01: // score (should be called in its own timed subroutine, as it's also producing sound FX)
      TRACE(("%03x: 1%x%x   score %x + %x  (if=%x,ac=%x)\n", codeOffset, data1, data2, data1, data2, ifActive, accu));
      if (!ifActive || accu) {
        int i, mult = 1;
        for (i=data1; i < 0x0d; i++)
          mult *= 10;
        score += mult * data2;
        score %= 1000000;
        locals.segments[0].w = core_bcd2seg7[score / 100000];
        locals.segments[1].w = core_bcd2seg7[(score % 100000) / 10000];
        locals.segments[2].w = core_bcd2seg7[(score % 10000) / 1000];
        locals.segments[3].w = core_bcd2seg7[(score % 1000) / 100];
        locals.segments[4].w = core_bcd2seg7[(score % 100) / 10];
        locals.segments[5].w = core_bcd2seg7[score %10];
      }
      exec_pgol(codeOffset + 3);
      break;
    case 0x02: // lamps & solenoids
      TRACE(("%03x: 2%x%x   data  %s %x-%x  (if=%x,ac=%x)\n", codeOffset, data1, data2, (data1 & 8 ? "on ":"off"), data2, (data1 & 7), ifActive, accu));
      if (!ifActive || accu) {
        if (8 & data1) {
          if (data2 > 0xb) {
            locals.solenoids |= (1 << (7 & data1)) << (4 * (data2 - 0x0c));
            locals.solCount = 0;
          } else
            locals.lampMatrix[data2] |= 1 << (7 & data1);
        } else
          if (data2 < 0xc) locals.lampMatrix[data2] &= ~(1 << (7 & data1));
      }
      exec_pgol(codeOffset + 3);
      break;
    case 0x03: // conditional jump
      TRACE(("%03x: 3%x%x%x  jcs   %x%x%x  (ac=%x)\n", codeOffset, data1, data2, data3, data1, data2, data3, accu));
      if (accu)
        exec_pgol((data1 << 8) | (data2 << 4) | data3);
      else
        exec_pgol(codeOffset + 4);
      break;
    case 0x06: // pause
      TRACE(("%03x: 6     pause\n", codeOffset));
      timer_adjust(locals.sleepTimer, 0.05, 0, 0.05);
      exec_pgol(codeOffset + 1);
      break;
    case 0x07: // setif?
      TRACE(("%03x: 7%x    setif %x  (ac=%x)\n", codeOffset, data1, data1, accu));
      ifActive = 1;
      if (accu == data1)
        accu = 1;
      else
        accu = 0;
      exec_pgol(codeOffset + 2);
      break;
    case 0x0a: // check lamp and decrement
      if (!ifActive) accu = 1;
      ifActive = 1;
      accu |= ((locals.lampMatrix[data2] & (1 << (7 & data1))) > 0 == (data1 >> 3));
      TRACE(("%03x: a%x%x   orif  %s %x-%x  (ac=%x)\n", codeOffset, data1, data2, (data1 & 8 ? "on ":"off"), data2, (data1 & 7), accu));
      exec_pgol(codeOffset + 3);
      break;
    case 0x0b: // turn off ifActive
      TRACE(("%03x: b     noif\n", codeOffset));
      ifActive = accu = 0;
      exec_pgol(codeOffset + 1);
      break;
    case 0x0c: // return to switch scanning
      TRACE(("%03x: c     ret\n", codeOffset));
      ifActive = accu = 0;
      timer_adjust(locals.sleepTimer, TIME_NEVER, 0, TIME_NEVER);
      break;
    case 0x0d: // inverse accu
      TRACE(("%03x: d     not\n", codeOffset));
      accu = accu ? 0 : 1;
      exec_pgol(codeOffset + 1);
      break;
    case 0x0e: // unconditional jump
      TRACE(("%03x: e%x%x%x  jump  %x%x%x\n", codeOffset, data1, data2, data3, data1, data2, data3));
      exec_pgol((data1 << 8) | (data2 << 4) | data3);
      break;
    case 0x0f: // do nothing (small delay)
      TRACE(("%03x: f     nop\n", codeOffset));
      exec_pgol(codeOffset + 1);
      break;
    default: // no idea so far
      TRACE(("%03x: %x     opc   #%x\n", codeOffset, data0, data0));
      exec_pgol(codeOffset + 1);
  }
}

static SWITCH_UPDATE(GTS1) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[GTS1_COMINPORT] & 0xff;
  }
  if (!locals.swMatrix[0] && coreGlobals.swMatrix[0] & 0x03)
    exec_pgol(0x100 + 4 * core_BitColToNum(coreGlobals.swMatrix[0] & 0x03));
  if (!locals.swMatrix[0] && coreGlobals.swMatrix[0] & 0x04)
    locals.lampMatrix[6]++;
  if (!locals.swMatrix[0] && coreGlobals.swMatrix[0] & 0x08)
    locals.lampMatrix[6] = 2;
  if (!locals.swMatrix[1] && coreGlobals.swMatrix[1] & 0x1f)
    exec_pgol(0x120 + 4 * core_BitColToNum(coreGlobals.swMatrix[1] & 0x1f));
  if (!locals.swMatrix[2] && coreGlobals.swMatrix[2] & 0x1f)
    exec_pgol(0x140 + 4 * core_BitColToNum(coreGlobals.swMatrix[2] & 0x1f));
  if (!locals.swMatrix[3] && coreGlobals.swMatrix[3] & 0x1f)
    exec_pgol(0x160 + 4 * core_BitColToNum(coreGlobals.swMatrix[3] & 0x1f));
  if (!locals.swMatrix[4] && coreGlobals.swMatrix[4] & 0x1f)
    exec_pgol(0x180 + 4 * core_BitColToNum(coreGlobals.swMatrix[4] & 0x1f));
  if (!locals.swMatrix[5] && coreGlobals.swMatrix[5] & 0x1f)
    exec_pgol(0x1a0 + 4 * core_BitColToNum(coreGlobals.swMatrix[5] & 0x1f));
  if (!locals.swMatrix[6] && coreGlobals.swMatrix[6] & 0x1f)
    exec_pgol(0x1c0 + 4 * core_BitColToNum(coreGlobals.swMatrix[6] & 0x1f));
  if (!locals.swMatrix[7] && coreGlobals.swMatrix[7] & 0x1f)
    exec_pgol(0x1e0 + 4 * core_BitColToNum(coreGlobals.swMatrix[7] & 0x1f));
  // manually performing switch debouncing
  memcpy(locals.swMatrix, coreGlobals.swMatrix, sizeof(coreGlobals.swMatrix));
}

static int GTS1_sw2m(int no) {
	return no + 8;
}

static int GTS1_m2sw(int col, int row) {
	return col*8 + row - 8;
}

/* game switches */
static READ_HANDLER(port_r) {
	return coreGlobals.swMatrix[offset];
}

/* lamps & solenoids */
static WRITE_HANDLER(port_0x_w) {
	switch (offset) {
		case 0:
			locals.tmpLampData = data; // latch lamp data for strobe_w call
			break;
		case 1:
			locals.solenoids |= data;
			break;
		case 2:
			locals.solenoids |= (data << 8);
			break;
	}
}

/* sound maybe? */
static WRITE_HANDLER(port_1x_w) {
	// Deposit the output as solenoids until we know what it's for.
	// So in case the sounds are just solenoid chimes, we're already done.
	if (offset == 1)
		locals.solenoids = (locals.solenoids & 0xff00ffff) | (data << 16);
	else if (data != 0 && !((offset == 0 && data == 0x47) || (offset == 6 && data == 0x0f)))
		logerror("Unexpected output on port %02x = %02x\n", offset, data);
}

/* display data */
static WRITE_HANDLER(disp_w) {
	((int *)locals.pseg)[2*offset] = core_bcd2seg[data >> 4];
	((int *)locals.pseg)[2*offset + 1] = core_bcd2seg[data & 0x0f];
}

/* this handler updates lamps, switch columns & displays - all at the same time!!! */
static WRITE_HANDLER(strobe_w) {
	if (data < 7) { // only 7 columns are used
		int ii;
		for (ii = 0; ii < 6; ii++) {
			((int *)locals.segments)[ii*7 + data] = ((int *)locals.pseg)[ii];
		}
		locals.lampMatrix[data] = locals.tmpLampData;
		locals.tmpSwCol = data + 1;
	}
}

/* port read / write */
PORT_READ_START( GTS1_readport )
	{ 0x00, 0x01, port_r },
PORT_END

PORT_WRITE_START( GTS1_writeport )
PORT_END

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *GTS1_CMOS;
static WRITE_HANDLER(GTS1_CMOS_w) {
  GTS1_CMOS[offset] = data;
}

static NVRAM_HANDLER(GTS1) {
	core_nvram(file, read_or_write, GTS1_CMOS, 256, 0x00);
}

/*-----------------------------------------
/  Memory map for System1 CPU board
/------------------------------------------*/
static READ_HANDLER(boot_r) {
  return 0x80; // Jump into an endless loop to keep the CPU busy
}

static MEMORY_READ_START(GTS1_readmem)
  {0x0000,0x00ff,	boot_r},	/* hack */
  {0x0100,0x04ff,	MRA_ROM},	/* game PROM */
MEMORY_END

static MEMORY_WRITE_START(GTS1_writemem)
MEMORY_END

static MACHINE_INIT(GTS1) {
  memset(&locals, 0, sizeof locals);
  locals.sleepTimer = timer_alloc(sleepTimer);
  timer_adjust(locals.sleepTimer, TIME_NEVER, 0, TIME_NEVER);
  if (core_gameData->hw.soundBoard)
    sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);
  accu = ifActive = score = 0;
}

static MACHINE_STOP(GTS1) {
  timer_adjust(locals.sleepTimer, TIME_NEVER, 0, TIME_NEVER);
}

static MACHINE_DRIVER_START(GTS1NS)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", PPS4, 198864)
  MDRV_CPU_MEMORY(GTS1_readmem, GTS1_writemem)
//  MDRV_CPU_PORTS(GTS1_readport,GTS1_writeport)
  MDRV_CPU_VBLANK_INT(GTS1_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(GTS1,NULL,GTS1)
//  MDRV_NVRAM_HANDLER(GTS1)
  MDRV_DIPS(24)
  MDRV_SWITCH_UPDATE(GTS1)
//  MDRV_DIAGNOSTIC_LEDH(4)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GTS1)
  MDRV_IMPORT_FROM(GTS1NS)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GTS1S80)
  MDRV_IMPORT_FROM(GTS1NS)
  MDRV_IMPORT_FROM(gts80s_s)
MACHINE_DRIVER_END
