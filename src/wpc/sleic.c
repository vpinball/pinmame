// license:BSD-3-Clause

/************************************************************************************************
 Sleic (Spain)
 -------------

   Hardware:
   ---------
		CPU:     I80C188 @ ??? for game & sound,
		         I80C39 @ ??? for DMD display,
		         Z80 @ ??? for I/O
		    INT: IRQ @ ???
		DISPLAY: DMD
		SOUND:   YM3812 @ ???,
		         DAC,
		         OKI6376 @ 2 or 4 MHz for speech / FX
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/i8039/i8039.h"
#include "sound/adpcm.h"
#include "sound/3812intf.h"
#include "sleic.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  sndCmd;
  UINT8  swCol;
  UINT8  lampCol;
} locals;

// switches start at 50 for column 1, and each column adds 10.
static int SLEIC_sw2m(int no) { return (no/10 - 4)*8 + no%10; }
static int SLEIC_m2sw(int col, int row) { return 40 + col*10 + row; }

static INTERRUPT_GEN(SLEIC_irq_i80188) {
  cpu_set_irq_line(SLEIC_MAIN_CPU, 0, PULSE_LINE);
}

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

  core_updateSw(TRUE);
}

#ifdef MAME_DEBUG
static void showData(int data) {
  static char s[2];
  sprintf(s, "%02x", data);
  core_textOut(s, 6, 25, 2, 5);
}
#endif /* MAME_DEBUG */

static SWITCH_UPDATE(SLEIC) {
#ifdef MAME_DEBUG
  static UINT8 data = 0;
#endif
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
#ifdef MAME_DEBUG
  if      (keyboard_pressed_memory_repeat(KEYCODE_Z, 2))
    showData(data -= 16);
  else if (keyboard_pressed_memory_repeat(KEYCODE_X, 2))
    showData(--data);
  else if (keyboard_pressed_memory_repeat(KEYCODE_C, 2))
    showData(++data);
  else if (keyboard_pressed_memory_repeat(KEYCODE_V, 2))
    showData(data += 16);
  else if (keyboard_pressed_memory_repeat(KEYCODE_SPACE, 2)) {
	  OKIM6376_data_0_w(0, data);
	  OKIM6376_data_0_w(0, 0x10);
	}
#endif /* MAME_DEBUG */
}

static WRITE_HANDLER(pic_w) {
  logerror("PIC W(%03x->%2x) = %02x\n", offset, offset>>7, data);
}

/* handler called by the 3812 when the internal timers cause an IRQ */
static void ym3812_irq(int irq) {
//  cpu_set_irq_line(SLEIC_MAIN_CPU, 0, irq ? ASSERT_LINE : CLEAR_LINE);
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
	0,					/* 1 chip (but use 0 to indicate 6374 chip) */
	{ 2000000./132. },	/* sampling frequency at 2MHz chip clock */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};
static struct OKIM6295interface SLEIC_okim6376_intf2 =
{
	0,					/* 1 chip (but use 0 to indicate 6374 chip)  */
	{ 4000000./132. },  /* sampling frequency at 4MHz chip clock */
	{ REGION_USER1 },	/* memory region */
	{ 75 }				/* volume */
};
static struct DACinterface SLEIC_dac_intf = { 1, { 25 }};

static READ_HANDLER(read_0) {
  return 0;
}

static MEMORY_READ_START(SLEIC_80188_readmem)
  {0x00000,0x01fff, MRA_RAM},
  {0x10100,0x10900, read_0 /* MRA_RAM */},
  {0x60410,0x6340f, MRA_RAM},
  {0x80000,0xfffff, MRA_ROM},
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

static WRITE_HANDLER(i80188_write_port) {
  logerror("80188 internal w %02x:%x %02x\n", offset / 2, offset % 2, data);
}

static READ_HANDLER(i8039_read_test) {
//  logerror("8039 read port T1\n");
  return 0;
}

static WRITE_HANDLER(i8039_write_port) {
/*
  static UINT8 pos = 1;
  UINT8 *line;
  if (!offset)
    pos = data;
  else {
    line = &dotCol[1+(pos >> 4)][8*(pos & 0x0f)];
    *line++ = data & 0x80 ? 3 : 0;
    *line++ = data & 0x40 ? 3 : 0;
    *line++ = data & 0x20 ? 3 : 0;
    *line++ = data & 0x10 ? 3 : 0;
    *line++ = data & 0x08 ? 3 : 0;
    *line++ = data & 0x04 ? 3 : 0;
    *line++ = data & 0x02 ? 3 : 0;
    *line++ = data & 0x01 ? 3 : 0;
  }
*/
  logerror("8039 write port P%d = %02x\n", offset+1, data);
}

static READ_HANDLER(z80_read_port) {
  switch (offset) {
    case 1: return core_getDip(0);
    case 2: return ~coreGlobals.swMatrix[1 + locals.swCol];
    case 3: return ~coreGlobals.tmpLampMatrix[locals.lampCol];
    case 4: return coreGlobals.swMatrix[0];
    default: logerror("Z80 read port %02x\n", offset);
  }
  return 0;
}

static WRITE_HANDLER(z80_write_port) {
  switch (offset) {
    case 1: coreGlobals.diagnosticLed = data >> 3; break;
    case 2: locals.swCol = core_BitColToNum(data); break;
    case 3: locals.lampCol = core_BitColToNum(data); break;
    case 4: coreGlobals.tmpLampMatrix[locals.lampCol] = data; break;
    case 5: locals.solenoids = (locals.solenoids & 0xff00ff) | ((data ^ 0xff) << 8); break;
    case 6: locals.solenoids = (locals.solenoids & 0xffff00) | (data ^ 0xff); break;
    case 7: break;
    default: logerror("Z80 write port %2x = %02x\n", 0x80 + offset, data);
  }
}

static PORT_READ_START(SLEIC_80188_readport)
MEMORY_END

static PORT_WRITE_START(SLEIC_80188_writeport)
  {0xff00,0xffff, i80188_write_port},
MEMORY_END

static PORT_READ_START(SLEIC_8039_readport)
  {I8039_t1,I8039_t1, i8039_read_test},
MEMORY_END

static PORT_WRITE_START(SLEIC_8039_writeport)
  {I8039_p1,I8039_p2, i8039_write_port},
MEMORY_END

static PORT_READ_START(SLEIC_Z80_readport)
  {0x00,0x07, z80_read_port},
MEMORY_END

static PORT_WRITE_START(SLEIC_Z80_writeport)
  {0x80,0x87, z80_write_port},
MEMORY_END

static MACHINE_INIT(SLEIC) {
  memset(&locals, 0, sizeof locals);
}

static MACHINE_DRIVER_START(SLEIC)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(SLEIC,NULL,NULL)
  MDRV_SWITCH_CONV(SLEIC_sw2m,SLEIC_m2sw)
  MDRV_SWITCH_UPDATE(SLEIC)
  MDRV_DIPS(8)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_NVRAM_HANDLER(generic_0fill)

  // game & sound section
  MDRV_CPU_ADD_TAG("mcpu", I188, 8000000)
  MDRV_CPU_MEMORY(SLEIC_80188_readmem, SLEIC_80188_writemem)
  MDRV_CPU_PORTS(SLEIC_80188_readport, SLEIC_80188_writeport)
  MDRV_CPU_VBLANK_INT(SLEIC_vblank, 1)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i80188, 120)

  // I/O section
  MDRV_CPU_ADD_TAG("icpu", Z80, 2500000)
  MDRV_CPU_MEMORY(SLEIC_Z80_readmem, SLEIC_Z80_writemem)
  MDRV_CPU_PORTS(SLEIC_Z80_readport, SLEIC_Z80_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_z80, 2500000/2048.)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC1)
  MDRV_IMPORT_FROM(SLEIC)

  // display section
  MDRV_CPU_ADD_TAG("dcpu", I8039, 2000000)
  MDRV_CPU_MEMORY(SLEIC_8039_readmem, SLEIC_8039_writemem)
  MDRV_CPU_PORTS(SLEIC_8039_readport, SLEIC_8039_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i8039, 2000000/8192.)

  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC_okim6376_intf)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC2)
  MDRV_IMPORT_FROM(SLEIC)

  MDRV_SOUND_ADD(YM3812, SLEIC_ym3812_intf)
  MDRV_SOUND_ADD(OKIM6295, SLEIC_okim6376_intf2)
  MDRV_SOUND_ADD(DAC, SLEIC_dac_intf)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(SLEIC3)
  MDRV_IMPORT_FROM(SLEIC2)

  // display section
  MDRV_CPU_ADD_TAG("dcpu", I8039, 2000000)
  MDRV_CPU_MEMORY(SLEIC_8039_readmem, SLEIC_8039_writemem)
  MDRV_CPU_PORTS(SLEIC_8039_readport, SLEIC_8039_writeport)
  MDRV_CPU_PERIODIC_INT(SLEIC_irq_i8039, 2000000/8192.)
MACHINE_DRIVER_END

PINMAME_VIDEO_UPDATE(sleic_dmd_update) {
  int ii, jj, kk;
  UINT16 *RAM;

  RAM = (void *)(memory_region(SLEIC_MEMREG_CPU) + 0x60410);
  for (ii = 0; ii < 32; ii++) {
    UINT8 *line = &coreGlobals.dmdDotRaw[ii * layout->length];
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

  core_dmd_video_update(bitmap, cliprect, layout, NULL);
  return 0;
}
