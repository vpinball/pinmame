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
#include "gts80.h"
#include "sndbrd.h"
#include "gts80s.h"
#include "cpu/pps4/pps4.h"

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
  int    pgolAddress;
  UINT8  accu;
  UINT8  strobe, swStrobe, lampData, ramE2, ramRW, ramAddr;
  UINT32 solenoids;
} locals;

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(GTS1_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % GTS1_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.lampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((++locals.solCount % GTS1_SOLSMOOTH) == 0) {
  	locals.solenoids = 0;
  }

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

static WRITE_HANDLER(led_w) {
	coreGlobals.diagnosticLed = data;
}

static WRITE_HANDLER(lamp_w) {
	data ^= 0x0f;
	if (offset % 2)
		coreGlobals.tmpLampMatrix[offset/2] = (coreGlobals.tmpLampMatrix[offset/2] & 0x0f) | (data << 4);
	else
		coreGlobals.tmpLampMatrix[offset/2] = (coreGlobals.tmpLampMatrix[offset/2] & 0xf0) | data;
}

static WRITE_HANDLER(disp_w) {
	coreGlobals.segments[offset].w = core_bcd2seg7[data];
}

static WRITE_HANDLER(port_w) {
	int device = offset >> 4;
	int ioport = offset & 0x0f;
	int rw = data & 0x40;
	int sos = data & 0x10;
	int enable = data & 0x08;
	int group = 0x07 ^ (((data & 0x80) >> 5) | ((data & 0x30) >> 4));
	locals.accu = data & 0x0f;
	switch (device) {
		case 0x02: // U5 RRIO A1752 - Switch matrix
			logerror("%03x: I/O on U5, port %x: %s %x\n", activecpu_get_pc(), ioport, sos ? "SOS" : "SES", enable ? 1 : 0);
			if (sos && ioport < 6)
				locals.swStrobe = ioport;
			if (ioport > 7) {
				locals.accu &= 0x07;
				locals.accu |= (coreGlobals.swMatrix[locals.swStrobe + 1] & (1 << (ioport - 8))) ? 0: 0x08;
			}
			break;
		case 0x03: // U3 GPIO 10696 - Lamps, strobe, dip switches, bits 8 & 9 of PGOL address
			if (rw) {
				if (group & 1) // outputs 1 - 4: lamp data
					locals.lampData = locals.accu;
				if (group & 2) // outputs 5 - 8: data strobe
					locals.strobe = locals.accu;
					if (locals.strobe) lamp_w(locals.strobe, locals.lampData);
				if (group & 4) { // outputs 9 & 10: PGOL address bits
					locals.pgolAddress = (locals.pgolAddress & 0x0ff) | ((locals.accu & 2) << 7) | ((locals.accu & 1) << 9);
				}
			} else {
				locals.accu = 0;
				if (group & 1) // inputs 1 - 4: dips
					locals.accu |= 0x0f ^ (core_getDip(locals.strobe) & 0x0f);
				if (group & 2) // inputs 5 - 8: dips
					locals.accu |= 0x0f ^ (core_getDip(locals.strobe) >> 4);
				if (group & 4) // inputs 9 - 12: special switches
					locals.accu |= 0x0e ^ (coreGlobals.swMatrix[0] >> 4);
			}
			logerror("%03x: I/O on U3: %s %x: %x\n", activecpu_get_pc(), rw ? "SET" : "READ", group, locals.accu);
			break;
		case 0x04: // U4 RRIO A1753 - Solenoids, NVRAM R/W & enable
			logerror("%03x: I/O on U4, port %x: %s %x\n", activecpu_get_pc(), ioport, sos ? "SOS" : "SES", enable ? 1 : 0);
			if (sos) {
				locals.solenoids = (locals.solenoids & ~(1 << ioport)) | (enable << ioport);
				if (ioport == 0x0d)
					locals.ramE2 = enable;
				if (ioport == 0x0e)
					locals.ramRW = enable;
			}
			break;
		case 0x06: // U2 GPIO 10696 - NVRAM in / out
			if (rw) {
				if (group & 1) // ram lo address
					locals.ramAddr = (locals.ramAddr & 0xf0) | locals.accu;
				if (group & 2) // ram hi address
					locals.ramAddr = (locals.ramAddr & 0x0f) | (locals.accu << 4);
				if (group & 4) // write to nvram
					memory_region(REGION_CPU1)[0x1100 + locals.ramAddr] = locals.accu;
			} else {
				locals.accu = 0;
				if (group & 1) // read from nvram
					locals.accu = memory_region(REGION_CPU1)[0x1100 + locals.ramAddr];
			}
			logerror("%03x: I/O on U2: %s %x: %x\n", activecpu_get_pc(), rw ? "SET" : "READ", group, locals.accu);
			break;
		case 0x0d: // U6 GPKD 10788 - Display
			data = PPS4_get_reg(PPS4_AB); // read display address from B register (that's how the real chip does it!)
			logerror("%03x: I/O on U6: %04x:%x\n", activecpu_get_pc(), data, locals.accu);
			switch (data >> 4) {
				case 0:
					disp_w(48 + (data & 0x0f), locals.accu);
					break;
				case 1: case 2: case 3: case 4: case 5:
					disp_w((data >> 4) * 6 + (data & 0x0f), locals.accu);
					break;
				case 7:
					disp_w(32 + (data & 0x0f), locals.accu);
					break;
				default:
					logerror("%03x: Write to unknown display %x: %02x\n", activecpu_get_pc(), data >> 4, locals.accu);
			}
			break;
		default:
			logerror("%03x: Write to unknown periphal chip %x: %02x\n", activecpu_get_pc(), device, data);
	}
}
static READ_HANDLER(port_r) {
	return locals.accu;
}

static WRITE_HANDLER(pgol_w) {
	data ^= 0x0f;
	if (offset) locals.pgolAddress = (locals.pgolAddress & 0x30f) | (data << 4);
	else        locals.pgolAddress = (locals.pgolAddress & 0x3f0) | data;
}
static READ_HANDLER(pgol_r) {
	UINT8 code = memory_region(REGION_USER1)[locals.pgolAddress] & 0x0f;
	logerror("Reading PGOL prom at address %03x: %x\n", locals.pgolAddress, code);
	return code;
}

/* port read / write */
PORT_READ_START( GTS1_readport )
  {0x000,0x0ff,	port_r},
  {0x100,0x100,	pgol_r},
PORT_END

PORT_WRITE_START( GTS1_writeport )
  {0x000,0x0ff,	port_w},
  {0x100,0x101,	pgol_w},
PORT_END

/*-----------------------------------------
/  Memory map for System1 CPU board
/------------------------------------------*/
static MEMORY_READ_START(GTS1_readmem)
  {0x0000,0x0fff,	MRA_ROM},
  {0x1000,0x11ff,	MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(GTS1_writemem)
  {0x1000,0x10ff,	MWA_RAM},
  {0x1100,0x11ff,	MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static MACHINE_INIT(GTS1) {
  memset(&locals, 0, sizeof locals);
  if (core_gameData->hw.soundBoard)
    sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS80_MEMREG_SCPU1), NULL, NULL);
}

static MACHINE_RESET(GTS1) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_STOP(GTS1) {
}

static MACHINE_DRIVER_START(GTS1NS)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", PPS4, 198864)
  MDRV_CPU_MEMORY(GTS1_readmem, GTS1_writemem)
  MDRV_CPU_PORTS(GTS1_readport,GTS1_writeport)
  MDRV_CPU_VBLANK_INT(GTS1_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(GTS1,GTS1,GTS1)
  MDRV_DIPS(24)
  MDRV_NVRAM_HANDLER(generic_0fill)
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
