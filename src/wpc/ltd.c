/************************************************************************************************
 LTD (Brasil)
 ------------
 by Gaston

 Emulating LTD was a wee little difficult as information is relatively sparse.

 There are some very good schematics for system III redrawn by Newton Pessoa (thanks!)
 but somehow they wouldn't align to what is happening on the CPU side.
 Still, I somehow came up with a DMA solution for all of the inputs and outputs.
 Maybe one day I'll find out how they really did it, and rework the driver,
 but for the moment it's working OK...

 System 4 was hell at first, but some terribly good fun in the end. :)
 LTD switched over from a simple 6802 CPU to the 6803 MCU, and had me sitting and wondering
 where all the inputs and outputs were connected, as seemingly NOTHING was going on!
 Of course I had to hook up the internal register read/write handlers, and voila,
 CPU ports were suddenly available (imagine me slapping my forehead at that point)! ;)

 I had a manual for system 4 written in Portuguese that was a bit of a help,
 but I still had to guess most of the data; it seems to work fairly good now,
 only whenever a sound is played, this seems to interfere with the displays, which is
 odd because they run on completely different outputs!?

 Fun fact: LTD obviously didn't use any IRQ or NMI timing on system 4 at all!
 They also drive the flipper coils directly by two pulsed solenoid outputs.

   Hardware:
   ---------
		CPU:	 M6802 for system III, M6803 for system 4 with NTSC quartz
		DISPLAY: 7-segment LED panels, direct segment access on system 4
		SOUND:	 - discrete (4 tones, like Zaccaria's 1311) on system III, maybe something better on Zephy
				 - 2 x AY8910 (separated for left & right speaker) for system 4 games
 ************************************************************************************************/

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "sound/ay8910.h"
#include "core.h"
#include "ltd.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  int vblankCount;
  int diagnosticLed;
  UINT32 solenoids;
  UINT16 solenoids2;
  core_tSeg segments;
  int swCol, lampCol, cycle, solBank;
  UINT8 port2, dispData[2], auxData;
} locals;

#define LTD_CPUFREQ	3579545/4

static WRITE_HANDLER(ay8910_0_ctrl_w) { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w) { AY8910Write(0,1,data); }
static WRITE_HANDLER(ay8910_0_reset)  { AY8910_reset(0); }
static READ_HANDLER (ay8910_0_r)      { return AY8910Read(0); }

static WRITE_HANDLER(ay8910_1_ctrl_w) { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w) { AY8910Write(1,1,data); }
static WRITE_HANDLER(ay8910_1_reset)  { AY8910_reset(1); }
static READ_HANDLER (ay8910_1_r)      { return AY8910Read(1); }

struct AY8910interface LTD_ay8910Int = {
	2,					/* 2 chips */
	LTD_CPUFREQ,		/* 895 kHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },	/* Volume */
};


// System III

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(LTD_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % LTD_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((locals.vblankCount % LTD_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids &= 0x10000;
  }
  /*-- display --*/
  if ((locals.vblankCount % LTD_DISPLAYSMOOTH) == 0)
  {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }
  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(17));
}

static INTERRUPT_GEN(LTD_irq) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, PULSE_LINE);
}

static SWITCH_UPDATE(LTD) {
  if (inports) {
    CORE_SETKEYSW(inports[LTD_COMINPORT],    0x01, 0);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x01, 1);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x20, 4);
  }
  cpu_set_nmi_line(LTD_CPU, coreGlobals.swMatrix[0] & 1);
}

/* Activate periphal write:
   0-6 : INT1-INT7 - Lamps & flipper enable
   7   : INT8      - Solenoids
   8   : INT9      - Display data
   9   : INT10     - Display strobe & sound
   15  : CLR       - resets all output
 */
static WRITE_HANDLER(peri_w) {
  if (offset < 10) {
    locals.segments[18 - 2*offset].w = core_bcd2seg[data >> 4];
    locals.segments[19 - 2*offset].w = core_bcd2seg[data & 0x0f];
  } else if (offset >= 0x10 && offset < 0x16) {
    coreGlobals.tmpLampMatrix[offset - 0x10] = data;
    // map flippers enable to sol 17
    if (offset == 0x10) {
      locals.solenoids = (locals.solenoids & 0x0ffff) | ((coreGlobals.tmpLampMatrix[0] & 0x40) ? 0 : 0x10000);
      locals.diagnosticLed = coreGlobals.tmpLampMatrix[0] >> 7;
    }
  } else if (offset == 0x16) {
    locals.solenoids = (locals.solenoids & 0x100ff) | (data << 8);
  } else if (offset == 0x17) {
    locals.solenoids = (locals.solenoids & 0x1ff00) | data;
  }
}

static WRITE_HANDLER(auxlamp1_w) {
  coreGlobals.tmpLampMatrix[8] = data;
}

static WRITE_HANDLER(auxlamp2_w) {
  coreGlobals.tmpLampMatrix[9] = data;
}

static WRITE_HANDLER(auxlamp3_w) {
  coreGlobals.tmpLampMatrix[10] = data;
}

static WRITE_HANDLER(auxlamp4_w) {
  coreGlobals.tmpLampMatrix[11] = data;
}

static WRITE_HANDLER(auxlamp5_w) {
  coreGlobals.tmpLampMatrix[12] = data;
}

static WRITE_HANDLER(auxlamp6_w) {
  coreGlobals.tmpLampMatrix[13] = data;
}

static WRITE_HANDLER(auxlamp7_w) {
  coreGlobals.tmpLampMatrix[7] = data;
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x60 && offset < 0x78) peri_w(offset-0x60, data);
}

static READ_HANDLER(sw_r) {
  return ~(coreGlobals.swMatrix[offset + 1] & 0x3f);
}

static MACHINE_INIT(LTD) {
  memset(&locals, 0, sizeof locals);
}

/*-----------------------------------------
/  Memory map for system III CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem)
  {0x0000,0x007f, MRA_RAM},
  {0x0080,0x00ff, sw_r},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem)
  {0x0000,0x007f, ram_w, &generic_nvram, &generic_nvram_size},
  {0x0800,0x0800, auxlamp1_w},
  {0x0c00,0x0c00, auxlamp4_w},
  {0x1800,0x1800, auxlamp2_w},
  {0x1c00,0x1c00, auxlamp5_w},
  {0x2800,0x2800, auxlamp3_w},
  {0x2c00,0x2c00, auxlamp6_w},
MEMORY_END

MACHINE_DRIVER_START(LTD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6802, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem, LTD_writemem)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CPU_PERIODIC_INT(LTD_irq, 200)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_UPDATE(LTD)
MACHINE_DRIVER_END


// System 4

static INTERRUPT_GEN(LTD4_vblank) {
  locals.vblankCount++;
  /*-- lamps --*/
  if ((locals.vblankCount % LTD_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((locals.vblankCount % LTD4_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    coreGlobals.solenoids2 = locals.solenoids2;
    locals.solenoids = locals.solenoids2 = 0;
  }
  /*-- display --*/
  if ((locals.vblankCount % LTD_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
  }

  core_updateSw(0);
}

static SWITCH_UPDATE(LTD4) {
  if (inports) {
    CORE_SETKEYSW(inports[LTD_COMINPORT], 0x13, 2);
  }
}

static READ_HANDLER(sw4_r) {
  return (~locals.port2 & 0x10) ? ~coreGlobals.swMatrix[locals.swCol+1] : 0xff;
}

/* convert the segments to fit the usual core.c layout */
static UINT8 convDisp(UINT8 data) {
  return (data & 0x88) |
    ((data & 1) << 6) |
    ((data & 2) << 4) |
    ((data & 4) << 2) |
    ((data & 0x10) >> 2) |
    ((data & 0x20) >> 4) |
    ((data & 0x40) >> 6);
}

static WRITE_HANDLER(peri4_w) {
  static int clear = 0;
  if (locals.port2 & 0x10) {
    switch (locals.cycle) {
      case  0: if (data != 0xff) locals.lampCol = (1 + core_BitColToNum(data)) % 8; clear = (data != 0xff); break;
      case  1: locals.solBank = core_BitColToNum(data); break;
      case  2: locals.solenoids |= (data >> 4) << (locals.solBank * 4);
               locals.solenoids2 |= (data & 0x0f) << 4;
               coreGlobals.solenoids = locals.solenoids; coreGlobals.solenoids2 = locals.solenoids2; break;
      case  6: if (clear) locals.swCol = data >> 4;
               locals.segments[31-(data & 0x0f)].w = locals.dispData[0];
               locals.segments[15-(data & 0x0f)].w = locals.dispData[1]; break;
      case  7: if (clear) locals.dispData[0] = convDisp(data); break;
      case  8: if (clear) locals.dispData[1] = convDisp(data); break;
      case  9: coreGlobals.tmpLampMatrix[(locals.lampCol % 2 ? 8 : 12) + locals.lampCol / 2] = locals.auxData; break;
      case 10: coreGlobals.tmpLampMatrix[locals.lampCol] = data; break;
      default: logerror("peri_%d_w = %02x\n", locals.cycle, data);
    }
  }
}

static READ_HANDLER(unknown_r) {
  return locals.port2;
}

static WRITE_HANDLER(cycle_w) {
  if (~locals.port2 & data & 0x10) locals.cycle++;
  locals.port2 = data;
}

static WRITE_HANDLER(cycle_reset_w) {
  locals.cycle = 0;
}

static WRITE_HANDLER(auxlamps_w) {
  locals.auxData = data;
}

/*-----------------------------------------
/  Memory map for system 4 CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD4_readmem)
  {0x0000,0x001f, m6803_internal_registers_r},
  {0x0080,0x00ff, MRA_RAM},
  {0x0100,0x01ff, MRA_RAM},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD4_writemem)
  {0x0000,0x001f, m6803_internal_registers_w},
  {0x0080,0x00ff, MWA_RAM},
  {0x0100,0x01ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x0800,0x0800, cycle_reset_w},
  {0x2800,0x2800, auxlamps_w},
  {0x0c00,0x0c00, ay8910_1_reset},
  {0x1000,0x1000, ay8910_0_ctrl_w},
  {0x1400,0x1400, ay8910_0_reset},
  {0x1800,0x1800, ay8910_1_ctrl_w},
  {0x3000,0x3000, ay8910_0_data_w},
  {0x3800,0x3800, ay8910_1_data_w},
MEMORY_END

static PORT_READ_START(LTD4_readport)
  { M6803_PORT1, M6803_PORT1, sw4_r },
  { M6803_PORT2, M6803_PORT2, unknown_r },
PORT_END

static PORT_WRITE_START(LTD4_writeport)
  { M6803_PORT1, M6803_PORT1, peri4_w },
  { M6803_PORT2, M6803_PORT2, cycle_w },
PORT_END

MACHINE_DRIVER_START(LTD4)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6803, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD4_readmem, LTD4_writemem)
  MDRV_CPU_PORTS(LTD4_readport, LTD4_writeport)
  MDRV_CPU_VBLANK_INT(LTD4_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(LTD4)
  MDRV_SOUND_ADD(AY8910, LTD_ay8910Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END
