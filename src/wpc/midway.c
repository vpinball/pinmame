/************************************************************************************************
 Midway Pinball games
 --------------------

   Hardware:
   ---------
   Flicker: (Customized version of Bally EM machine by Dave Nutting; no production run.
             The SS patent was sold to Midway in 1977, and a few home pinball models
             were built using the same hardware design)

		CPU:     I4004 @ 750 kHz
		IO:      4004 Ports
		DISPLAY: 2 x 6 Digit, 2 x 2 Digit, 1 x 1 Digit 7-Segment panels
		SOUND:	 Chimes

   Rotation VIII:
		CPU:     Z80 @ 1.65 MHz ?
			INT: NMI @ 1350Hz ?
		IO:      Z80 Ports
		DISPLAY: 5 x 6 Digit 7-Segment panels
		SOUND:	 ?
************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/i4004/i4004.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "midway.h"

#define MIDWAY_VBLANKFREQ   60 /* VBLANK frequency in HZ */
#define MIDWAY_NMIFREQ    1350 /* NMI frequency in HZ - at this rate, bumpers seems OK. */

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

static int MIDWAY_sw2m(int no) {
  return no + 8;
}

static int MIDWAY_m2sw(int col, int row) {
  return col*8 + row - 8;
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static UINT8 *MIDWAY_CMOS;
static WRITE_HANDLER(MIDWAY_CMOS_w) {
  MIDWAY_CMOS[offset] = data;
}

static NVRAM_HANDLER(MIDWAY) {
  core_nvram(file, read_or_write, MIDWAY_CMOS, 256, 0x00);
}

static MACHINE_INIT(MIDWAY) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_STOP(MIDWAY) {
}



/* ROTATION VIII */

static INTERRUPT_GEN(MIDWAY_nmihi) {
  cpu_set_nmi_line(MIDWAY_CPU, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(MIDWAY_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % MIDWAY_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % MIDWAY_SOLSMOOTH) == 0)
  	locals.solenoids = 0;
  /*-- display --*/
  if ((locals.vblankCount % MIDWAY_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0x00, sizeof locals.segments);
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(12));
}

static SWITCH_UPDATE(MIDWAY) {
  if (inports) {
    coreGlobals.swMatrix[0] = inports[MIDWAY_COMINPORT] & 0xff;
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0x0f) | ((inports[MIDWAY_COMINPORT] & 0x0f00) >> 4);
    coreGlobals.swMatrix[3] = (coreGlobals.swMatrix[3] & 0x2f) | ((inports[MIDWAY_COMINPORT] & 0xd000) >> 8);
    coreGlobals.swMatrix[4] = (coreGlobals.swMatrix[4] & 0xbf) | ((inports[MIDWAY_COMINPORT] & 0x2000) >> 7);
  }
}

/* if this read operation returns 0xc3, more rom code exists, starting at address 0x1800 */
static READ_HANDLER(mem1800_r) {
  return 0x00; /* 0xc3 */
}

/* game switches */
static READ_HANDLER(port_2x_r) {
  return coreGlobals.swMatrix[offset ? 0 : locals.tmpSwCol];
}

/* lamps & solenoids */
static WRITE_HANDLER(port_0x_w) {
  if (offset == 0)
    locals.tmpLampData = data; // latch lamp data for strobe_w call
  else
    locals.solenoids |= (data << ((offset-1) * 8));
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
  locals.pseg[2*offset].w = core_bcd2seg[data >> 4];
  locals.pseg[2*offset + 1].w = core_bcd2seg[data & 0x0f];
}

/* this handler updates lamps, switch columns & displays - all at the same time!!! */
static WRITE_HANDLER(port_2x_w) {
  switch (offset) {
    case 3:
      if (data < 7) { // only 7 columns are used
        int ii;
        for (ii = 0; ii < 6; ii++)
          locals.segments[ii*7 + data].w = locals.pseg[ii].w;
        locals.lampMatrix[data] = locals.tmpLampData;
        locals.tmpSwCol = data + 1;
      } else
        locals.tmpSwCol = 8;
      break;
    default:
      logerror("Write to port 2%x = %02x\n", offset, data);
  }
}

/* port read / write for Rotation VIII */
PORT_READ_START( midway_readport )
{ 0x21, 0x22, port_2x_r },
PORT_END

PORT_WRITE_START( midway_writeport )
{ 0x00, 0x02, disp_w },
{ 0x03, 0x07, port_0x_w },
{ 0x10, 0x17, port_1x_w },
{ 0x20, 0x25, port_2x_w },
PORT_END

/*-----------------------------------------
/  Memory map for Rotation VIII CPU board
/------------------------------------------
0000-17ff  3 x 2K ROM
c000-c0ff  RAM
e000-e0ff  NVRAM
*/
static MEMORY_READ_START(MIDWAY_readmem)
{0x0000,0x17ff,	MRA_ROM},	/* ROM */
{0x1800,0x1800,	mem1800_r}, /* Possible code extension. More roms to come? */
{0xc000,0xc0ff, MRA_RAM},   /* RAM */
{0xe000,0xe0ff,	MRA_RAM},	/* NVRAM */
MEMORY_END

static MEMORY_WRITE_START(MIDWAY_writemem)
{0x0000,0x17ff,	MWA_ROM},	/* ROM */
{0xc000,0xc0ff, MWA_RAM},   /* RAM */
{0xe000,0xe0ff,	MIDWAY_CMOS_w, &MIDWAY_CMOS},	/* NVRAM */
MEMORY_END

MACHINE_DRIVER_START(MIDWAY)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 1650000)
  MDRV_CPU_MEMORY(MIDWAY_readmem, MIDWAY_writemem)
  MDRV_CPU_PORTS(midway_readport,midway_writeport)
  MDRV_CPU_VBLANK_INT(MIDWAY_vblank, 1)
  MDRV_CPU_PERIODIC_INT(MIDWAY_nmihi, MIDWAY_NMIFREQ)
  MDRV_CORE_INIT_RESET_STOP(MIDWAY,NULL,MIDWAY)
  MDRV_NVRAM_HANDLER(MIDWAY)
  MDRV_DIPS(0) // no dips!
  MDRV_SWITCH_UPDATE(MIDWAY)
MACHINE_DRIVER_END



/* FLICKER */

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(MIDWAYP_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % MIDWAY_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, locals.lampMatrix, sizeof(locals.lampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % (MIDWAY_SOLSMOOTH)) == 0)
    locals.solenoids &= 0x8000;
  /*-- display --*/
  if ((locals.vblankCount % MIDWAY_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0x00, sizeof locals.segments);
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(MIDWAYP) {
  if (inports) {
    coreGlobals.swMatrix[5] = (coreGlobals.swMatrix[5] & ~0xe2) | (inports[MIDWAY_COMINPORT] & 0xff);
    coreGlobals.swMatrix[6] = (coreGlobals.swMatrix[6] & ~0x9f) | (inports[MIDWAY_COMINPORT] >> 8);
  }
}

/* "dips" (just connectable wires IRL) & normal switches */
static READ_HANDLER(rom2_r) {
  UINT8 sw;
  if (offset > 7)
    sw = core_getDip(offset/2 - 4);
  else
    sw = coreGlobals.swMatrix[offset/2 + 1];
  return (offset % 2) ? sw >> 4 : sw & 0x0f;
}

/* display (16 digits) */
static WRITE_HANDLER(rom1_w) {
  locals.segments[15 - offset].w = core_bcd2seg[data];
  // These next lines make up for a flaw in the rom code:
  // when you first make a 10 pt target in the game, and a
  // 1000 pt target thereafter, the 100s digit in between
  // will remain off! I'm sure Dave & Jeff did it similarily
  // on the prototype by wiring the segments accordingly;
  // maybe they also rewrote some of the code to fix it later on?
  if (locals.segments[2].w && !locals.segments[3].w)
    locals.segments[3].w = core_bcd2seg[0];
  if (locals.segments[9].w && !locals.segments[10].w)
    locals.segments[10].w = core_bcd2seg[0];
}

/* lamps (up to 64 are possible) */
static WRITE_HANDLER(rom2_w) {
  UINT8 lamp = locals.lampMatrix[offset/2];
  locals.lampMatrix[offset/2] = (offset % 2) ? (lamp & 0x0f) | (data << 4) : (lamp & 0xf0) | data;
  // map game on to solenoid #16
  locals.solenoids = (locals.solenoids & 0x7fff) | (((locals.lampMatrix[0] & 0x05) == 0x05) << 15);
  // Display the "0" at the single match units.
  if (locals.lampMatrix[5] & 0x01)
    locals.segments[16].w = core_bcd2seg[0];
}

/* solenoids 1-15 & test switch reading */
static WRITE_HANDLER(ram_w) {
  if (data && data != offset)
    locals.solenoids |= (1 << (data-1));
  else {
	UINT16 specialSw = (coreGlobals.swMatrix[6] << 8) | coreGlobals.swMatrix[5];
    // The 4004's test line is used to gain an extra switch row.
    i4004_set_TEST((specialSw >> offset) & 1);
  }
}

/* port read / write for Flicker */
PORT_READ_START( midway_readport2 )
{ 0x20, 0x2f, rom2_r },
PORT_END

PORT_WRITE_START( midway_writeport2 )
{ 0x00, 0x0f, rom1_w },
{ 0x10, 0x1f, rom2_w },
{ 0x110,0x11f,ram_w },
PORT_END

/*-----------------------------------------
/  Memory map for Flicker CPU board
/------------------------------------------
0000-03ff  1K ROM
1000-10ff  NVRAM (planned)
2000-20ff  RAM
*/
static MEMORY_READ_START(MIDWAYP_readmem)
{0x0000,0x03ff,	MRA_ROM},	/* ROM */
{0x1000,0x10ff, MRA_RAM},   /* NVRAM */
{0x2000,0x20ff, MRA_RAM},   /* RAM */
MEMORY_END

static MEMORY_WRITE_START(MIDWAYP_writemem)
{0x0000,0x03ff,	MWA_ROM},	/* ROM */
{0x1000,0x10ff,	MIDWAY_CMOS_w, &MIDWAY_CMOS},	/* NVRAM */
{0x2000,0x20ff, MWA_RAM},   /* RAM */
MEMORY_END

MACHINE_DRIVER_START(MIDWAYProto)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", 4004, 750000)
  MDRV_CPU_MEMORY(MIDWAYP_readmem, MIDWAYP_writemem)
  MDRV_CPU_PORTS(midway_readport2,midway_writeport2)
  MDRV_CPU_VBLANK_INT(MIDWAYP_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(MIDWAY,NULL,MIDWAY)
  MDRV_NVRAM_HANDLER(MIDWAY) // not used so far, but there'll be a bootleg that does!
  MDRV_DIPS(32) // 32 dips
  MDRV_SWITCH_UPDATE(MIDWAYP)
MACHINE_DRIVER_END
