/************************************************************************************************
 Gottlieb System 1
 -----------------

   Hardware:
   ---------
		CPU:     PPS/4 @ 198.864 kHz
		IO:      PPS/4 Ports
		DISPLAY: 4 x 6 Digit 9 segment panels, 1 x 4 Digit 7 segment panels
		SOUND:	 Chimes & System80 sound only board on later games
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "gts1.h"

#define GTS1_VBLANKFREQ  60 /* VBLANK frequency in HZ*/

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  int    diagnosticLed;
  int    tmpSwCol;
  UINT32 solenoids;
  UINT8  tmpLampData;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
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
  if ((locals.vblankCount % GTS1_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

  /*-- display --*/
  if ((locals.vblankCount % GTS1_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
	memset(locals.segments, 0x00, sizeof locals.segments);
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(1);
}

static SWITCH_UPDATE(GTS1) {
	if (inports) {
		coreGlobals.swMatrix[0] = inports[GTS1_COMINPORT] & 0xff;
	}
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
/------------------------------------------
0000-0fff  ROM
1000-10ff  RAM
*/
static MEMORY_READ_START(GTS1_readmem)
{0x0000,0x0fff,	MRA_ROM},	/* ROM */
{0x1000,0x10ff, MRA_RAM},   /* RAM */
MEMORY_END

static MEMORY_WRITE_START(GTS1_writemem)
{0x0000,0x0fff,	MWA_ROM},	/* ROM */
{0x1000,0x10ff, MWA_RAM},   /* RAM */
MEMORY_END

static MACHINE_INIT(GTS1) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_STOP(GTS1) {
}

MACHINE_DRIVER_START(GTS1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", PPS4, 198864)
  MDRV_CPU_MEMORY(GTS1_readmem, GTS1_writemem)
  MDRV_CPU_PORTS(GTS1_readport,GTS1_writeport)
  MDRV_CPU_VBLANK_INT(GTS1_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(GTS1,NULL,GTS1)
//  MDRV_NVRAM_HANDLER(GTS1)
  MDRV_DIPS(24)
  MDRV_SWITCH_UPDATE(GTS1)
//  MDRV_DIAGNOSTIC_LEDH(4)
MACHINE_DRIVER_END
