/************************************************************************************************
 Sleic (Spain)
 -------------

   Hardware:
   ---------
		CPU:	 I80C188 @ ??? for game & sound,
				 I80C39 @ ??? for DMD display,
				 Z80 @ ??? for I/O
			INT: IRQ @ ???
		DISPLAY: DMD
		SOUND:	 YM3812 @ ,
				 DAC,
		         OKI6376 @ for speech
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/i8039/i8039.h"
#include "sound/adpcm.h"
#include "sound/3812intf.h"
#include "machine/pic8259.h"
#include "sleic.h"

#define SLEIC_VBLANKFREQ   60 /* VBLANK frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  sndCmd;
} locals;

// switches start at 50 for column 1, and each column adds 10.
static int SLEIC_sw2m(int no) { return (no/10 - 4)*8 + no%10; }
static int SLEIC_m2sw(int col, int row) { return 40 + col*10 + row; }

static INTERRUPT_GEN(SLEIC_irq_i8039) {
  cpu_set_irq_line(SLEIC_DISPLAY_CPU, 0, PULSE_LINE);
}

static INTERRUPT_GEN(SLEIC_irq_z80) {
  cpu_set_irq_line(SLEIC_IO_CPU, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(SLEIC_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  if ((locals.vblankCount % SLEIC_LAMPSMOOTH) == 0)
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % SLEIC_SOLSMOOTH) == 0)
  	locals.solenoids = 0;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(SLEIC) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
}

static READ_HANDLER(dip_r) {
  return core_getDip(offset);
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[offset];
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(sol_w) {
  locals.solenoids |= data;
}

static WRITE_HANDLER(pic_w) {
  logerror("PIC W(%03x->%2x) = %02x\n", offset, offset>>7, data);
  pic8259_0_w(offset>>7, data);
}

/* handler called by the 3812 when the internal timers cause an IRQ */
static void ym3812_irq(int irq) {
  cpu_set_irq_line(SLEIC_MAIN_CPU, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*Interfaces*/
static struct YM3812interface SLEIC_ym3812_intf =
{
	1,						/* 1 chip */
	4000000,				/* 4 MHz */
	{ 100 },				/* volume */
	{ ym3812_irq },			/* IRQ Callback */
};
static struct OKIM6295interface SLEIC_okim6376_intf =
{
	1,						/* 1 chip */
	{ 8000 },				/* 8000Hz playback */
	{ SLEIC_MEMREG_CPU },	/* ROM REGION */
	{ 50 }					/* Volume */
};
static struct DACinterface SLEIC_dac_intf = { 1, { 25 }};

static WRITE_HANDLER(snd_w) {
  locals.sndCmd = data;
}

static MEMORY_READ_START(SLEIC_80188_readmem)
  {0x00000,0x01fff, MRA_RAM},
  {0x10100,0x10900, MRA_RAM},
  {0x60410,0x6340f, MRA_RAM},
  {0xe0000,0xfffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_80188_writemem)
  {0x00000,0x01fff,	MWA_RAM},
  {0x10100,0x10900, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0xa0000,0xa07ff, pic_w},
  {0x60410,0x6340f, MWA_RAM},
MEMORY_END

static MEMORY_READ_START(SLEIC_8039_readmem)
  {0x0000,0x3fff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_8039_writemem)
MEMORY_END

static MEMORY_READ_START(SLEIC_Z80_readmem)
  {0x0000,0x7fff, MRA_ROM},
  {0xc000,0xc7ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(SLEIC_Z80_writemem)
  {0xc000,0xc7ff, MWA_RAM},
MEMORY_END

static WRITE_HANDLER(i80188_write_port3) {
  logerror("80188 write port %2x = %02x\n", 0x30 + offset, data);
}

static WRITE_HANDLER(i80188_write_port5) {
  logerror("80188 write port %2x = %02x\n", 0x50 + offset, data);
}

static WRITE_HANDLER(i80188_write_port6) {
  logerror("80188 write port %2x = %02x\n", 0x60 + offset, data);
}

static WRITE_HANDLER(i80188_write_porta) {
  logerror("80188 write port %2x = %02x\n", 0xa0 + offset, data);
}

static READ_HANDLER(i8039_read_test) {
//  logerror("8039 read port T1\n");
  return 0;
}

static WRITE_HANDLER(i8039_write_port1) {
  logerror("8039 write port P1 = %02x\n", data);
}

static WRITE_HANDLER(i8039_write_port2) {
  logerror("8039 write port P2 = %02x\n", data);
}

static READ_HANDLER(z80_read_port) {
//  logerror("Z80 read port %02x\n", offset);
  return coreGlobals.swMatrix[offset];
}

static WRITE_HANDLER(z80_write_port) {
//  logerror("Z80 write port %2x = %02x\n", 0x80 + offset, data);
  coreGlobals.tmpLampMatrix[offset] = data;
}

static PORT_READ_START(SLEIC_80188_readport)
MEMORY_END

static PORT_WRITE_START(SLEIC_80188_writeport)
  {0xff30,0xff3f, i80188_write_port3},
  {0xff50,0xff5f, i80188_write_port5},
  {0xff66,0xff67, i80188_write_port6},
  {0xffa0,0xffa9, i80188_write_porta},
MEMORY_END

static PORT_READ_START(SLEIC_8039_readport)
  {I8039_t1,I8039_t1, i8039_read_test},
MEMORY_END

static PORT_WRITE_START(SLEIC_8039_writeport)
  {I8039_p1,I8039_p1, i8039_write_port1},
  {I8039_p2,I8039_p2, i8039_write_port2},
MEMORY_END

static PORT_READ_START(SLEIC_Z80_readport)
  {0x00,0x07, z80_read_port},
MEMORY_END

static PORT_WRITE_START(SLEIC_Z80_writeport)
  {0x80,0x87, z80_write_port},
MEMORY_END

static MACHINE_INIT(SLEIC) {
  memset(&locals, 0, sizeof locals);
  pic8259_0_config(SLEIC_MAIN_CPU, 0);
}

MACHINE_DRIVER_START(SLEIC)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(SLEIC,NULL,NULL)
//  MDRV_SWITCH_CONV(SLEIC_sw2m,SLEIC_m2sw)
  MDRV_SWITCH_UPDATE(SLEIC)
  MDRV_DIPS(2)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(DAC, SLEIC_okim6376_intf)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)

  // game & sound section
  MDRV_CPU_ADD_TAG("mcpu", I188, 8000000)
  MDRV_CPU_MEMORY(SLEIC_80188_readmem, SLEIC_80188_writemem)
  MDRV_CPU_PORTS(SLEIC_80188_readport, SLEIC_80188_writeport)
  MDRV_CPU_VBLANK_INT(SLEIC_vblank, 1)

  // display section
  MDRV_CPU_ADD_TAG("dcpu", I8039, 2000000)
  MDRV_CPU_MEMORY(SLEIC_8039_readmem, SLEIC_8039_writemem)
  MDRV_CPU_PORTS(SLEIC_8039_readport, SLEIC_8039_writeport)
//  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i8039, 250)

  // I/O section
  MDRV_CPU_ADD_TAG("mcpu", Z80, 2500000)
  MDRV_CPU_MEMORY(SLEIC_Z80_readmem, SLEIC_Z80_writemem)
  MDRV_CPU_PORTS(SLEIC_Z80_readport, SLEIC_Z80_writeport)
//  MDRV_CPU_PERIODIC_INT(SLEIC_irq_z80, 250)
MACHINE_DRIVER_END

PINMAME_VIDEO_UPDATE(sleic_dmd_update) {
  tDMDDot dotCol;
  int ii, jj, kk;
  UINT16 *RAM;

  RAM = (void *)(memory_region(SLEIC_MEMREG_CPU) + 0x60410);
  for (ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 16; jj++) {
      for (kk = 7; kk >= 0; kk--) {
        *line++ = (RAM[0]>>kk) & 1 ? 3 : 0;
	  }
      for (kk = 15; kk > 7; kk--) {
        *line++ = (RAM[0]>>kk) & 1 ? 3 : 0;
	  }
      RAM++;
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
