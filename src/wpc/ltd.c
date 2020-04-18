/************************************************************************************************
 LTD (Brasil)
 ------------
 by Gaston

 Emulating LTD was a wee little difficult as information is relatively sparse.

 There are some very good schematics for system III redrawn by Newton Pessoa (thanks!)
 but I failed to see how the DMA access to the peripherals works for the longest time.
 Now it's understood, and there is sound for the early games as well.

 System 4 was hell at first, but some terribly good fun in the end. :)
 LTD switched over from a simple 6802 CPU to the 6803 MCU, and had me sitting and wondering
 where all the inputs and outputs were connected, as seemingly NOTHING was going on!
 Of course I had to hook up the internal register read/write handlers, and voila,
 CPU ports were suddenly available (imagine me slapping my forehead at that point)! ;)

 I had a manual for system 4 written in Portuguese that was a bit of a help,
 but I still had to guess most of the data; it seems to work fairly good now.

 Fun fact: LTD obviously didn't use any IRQ or NMI timing on system 4 at all!
 They also drive the flipper coils directly by two pulsed solenoid outputs.

   Hardware:
   ---------
		CPU:	 M6802 for system III, M6803 for system 4 with NTSC quartz
		DISPLAY: 7-segment LED panels, direct segment access on system 4
		SOUND:	 - discrete (tones) on early system III games
				 - unknown sound hardware on Force, Space Poker, and Black Hole
				 - 2 x AY8910 (separated for left & right speaker) for Cowboy, Zephy, and system 4 games
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
  int swCol, cycle;
  UINT8 port2, auxData;
  int isHH;
} locals;

#define LTD_CPUFREQ	3579545/4

static WRITE_HANDLER(ay8910_0_ctrl_w) { AY8910_set_volume(0, ALL_8910_CHANNELS, 50); AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w) { AY8910_set_volume(0, ALL_8910_CHANNELS, 50); AY8910Write(0,1,data); }
static WRITE_HANDLER(ay8910_0_mute) { AY8910_set_volume(0, ALL_8910_CHANNELS, 0);  }
static WRITE_HANDLER(ay8910_1_ctrl_w) { AY8910_set_volume(1, ALL_8910_CHANNELS, 50); AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w) { AY8910_set_volume(1, ALL_8910_CHANNELS, 50); AY8910Write(1,1,data); }
static WRITE_HANDLER(ay8910_1_mute) { AY8910_set_volume(1, ALL_8910_CHANNELS, 0); }
static WRITE_HANDLER(ay8910_01_ctrl_w) { ay8910_0_ctrl_w(offset, data); ay8910_1_ctrl_w(offset, data); }
static WRITE_HANDLER(ay8910_01_data_w) { ay8910_0_data_w(offset, data); ay8910_1_data_w(offset, data); }
static WRITE_HANDLER(ay8910_01_reset) { AY8910_reset(0); AY8910_reset(1); }

static WRITE_HANDLER(port_0a_w) { logerror("AY#0 port A: %02x\n", data); }
static WRITE_HANDLER(port_0b_w) { logerror("AY#0 port B: %02x\n", data); }
static WRITE_HANDLER(port_1a_w) { locals.auxData = data; }
static WRITE_HANDLER(port_1b_w) { logerror("AY#1 port B: %02x\n", data); }

struct AY8910interface LTD_ay8910Int = {
	2,					/* 2 chips */
	LTD_CPUFREQ,		/* 895 kHz */
	{ MIXER(50,MIXER_PAN_LEFT), MIXER(50,MIXER_PAN_RIGHT) },	/* Volume */
	{ NULL }, { NULL },
	{ port_0a_w, port_1a_w }, { port_0b_w, port_1b_w },
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
  cpu_set_irq_line(LTD_CPU, M6800_IRQ_LINE, PULSE_LINE);
}

static SWITCH_UPDATE(LTD) {
  if (inports) {
    CORE_SETKEYSW(inports[LTD_COMINPORT],    0x01, 0);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x01, 1);
    CORE_SETKEYSW(inports[LTD_COMINPORT]>>8, 0x20, 4);
  }
  cpu_set_nmi_line(LTD_CPU, coreGlobals.swMatrix[0] & 1);
}

DISCRETE_SOUND_START(ltd3_discInt)
	DISCRETE_INPUT(NODE_01,1,0x0003,0) // tone
	DISCRETE_INPUT(NODE_02,2,0x0003,0) // enable
	DISCRETE_MULTADD(NODE_03,1,NODE_01,10,200)
	DISCRETE_TRIANGLEWAVE(NODE_04,NODE_02,NODE_03,20000,10000,0)
	DISCRETE_GAIN(NODE_05,NODE_04,12)
	DISCRETE_OUTPUT(NODE_05,50)
DISCRETE_SOUND_END

static void snd_stop(int param) {
  discrete_sound_w(param, 0);
}

/* Activate periphal write:
   0-6 : INT1-INT7 - Lamps & flipper enable
   7   : INT8      - Solenoids
   8   : INT9      - Display data
   9   : INT10     - Display strobe & sound
   15  : CLR       - resets all output
 */
static WRITE_HANDLER(peri_w) {
  static int freq[] = { 105, 70, 40, 30, 21, 15, 10, 5 };
  static UINT8 strobe;
  static int lampStrobe;
  int seg;
  if (offset < 0x06) {
    if (!strncasecmp(Machine->gamedrv->name, "spcpoker", 8)) {
      if (offset == 5) { // extra strobe for more lamps
        if (data & 0x08) lampStrobe = 0;
        else if (data & 0x80) lampStrobe = 1;
        coreGlobals.tmpLampMatrix[5] = data & 0x77;
      } else {
        coreGlobals.tmpLampMatrix[offset + 12 * lampStrobe] = data;
      }
    } else {
      coreGlobals.tmpLampMatrix[offset] = data;
    }
    // map flippers enable to sol 17
    if (!offset) {
      locals.solenoids = (locals.solenoids & 0x0ffff) | ((data & 0x40) || (~data & 0x10) ? 0 : 0x10000);
      locals.diagnosticLed = data >> 7;
    }
  } else if (offset == 0x06) { // either lamps or solenoids, or a mix of both!
    coreGlobals.tmpLampMatrix[6] = data;
    locals.solenoids = (locals.solenoids & 0x100ff) | (data << 8);
  } else if (offset == 0x07) {
    locals.solenoids = (locals.solenoids & 0x1ff00) | data;
  } else if (offset == 0x08) {
    seg = 9 - (strobe & 0x0f);
    locals.segments[seg].w = core_bcd2seg7a[data >> 4];
    locals.segments[10 + seg].w = core_bcd2seg7a[data & 0x0f];
    if (core_gameData->hw.gameSpecific1 & (1 << seg)) {
      if (seg & 1)
        coreGlobals.tmpLampMatrix[8 + seg / 2 * 2] = (coreGlobals.tmpLampMatrix[8 + seg / 2 * 2] & 0xf0) | (data >> 4);
      else
        coreGlobals.tmpLampMatrix[8 + seg / 2 * 2] = (coreGlobals.tmpLampMatrix[8 + seg / 2 * 2] & 0x0f) | (data & 0xf0);
    }
    if (core_gameData->hw.gameSpecific2 & (1 << seg)) {
      if (seg & 1)
        coreGlobals.tmpLampMatrix[9 + seg / 2 * 2] = (coreGlobals.tmpLampMatrix[9 + seg / 2 * 2] & 0xf0) | (data & 0x0f);
      else
        coreGlobals.tmpLampMatrix[9 + seg / 2 * 2] = (coreGlobals.tmpLampMatrix[9 + seg / 2 * 2] & 0x0f) | (data << 4);
    }
  } else if (offset == 0x09) {
    strobe = data;
    if (data & 0x10) {
      discrete_sound_w(1, freq[data >> 5]);
      discrete_sound_w(2, 1);
      timer_set(0.1, 2, snd_stop);
    }
  } else if (offset == 0x0d && !strncasecmp(Machine->gamedrv->name, "force", 5)) { // drop target single reset solenoids
    locals.solenoids = (locals.solenoids & 0x1ffff) | (0x10000 << data);
  } else if (offset == 0x0f) {
    logerror("CLR\n");
  } else {
    logerror("INT%d:%02x\n", offset + 1, data);
  }
}

static WRITE_HANDLER(ram_w) {
  generic_nvram[offset] = data;
  if (offset >= 0x70 && offset < 0x80) peri_w(offset-0x70, data);
}

static READ_HANDLER(sw_r) {
  return ~(coreGlobals.swMatrix[offset + 1] & 0x3f);
}

static READ_HANDLER(ff_r) {
  return 0xff;
}

static MACHINE_INIT(LTD) {
  memset(&locals, 0, sizeof locals);
}

/*-----------------------------------------
/  Memory map for system III CPU board
/------------------------------------------*/
static MEMORY_READ_START(LTD_readmem)
  {0x0000,0x007f, MRA_RAM},
  {0x0080,0x0087, sw_r},
  {0x00ff,0x00ff, ff_r}, // spcpoker code flaw: lda $00FF instead of lda #$FF, read from unmapped space -> all bits should be high
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(LTD_writemem)
  {0x0000,0x007f, ram_w, &generic_nvram, &generic_nvram_size},
  {0x0800,0x0800, ay8910_01_data_w},
  {0x0c00,0x0c00, ay8910_01_ctrl_w},
  {0x1800,0x1800, ay8910_0_data_w},
  {0x1c00,0x1c00, ay8910_0_ctrl_w},
  {0x2800,0x2800, ay8910_1_data_w},
  {0x2c00,0x2c00, ay8910_1_ctrl_w},
  {0xb000,0xb000, ay8910_01_reset},
MEMORY_END

MACHINE_DRIVER_START(LTD3)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6802, LTD_CPUFREQ)
  MDRV_CPU_MEMORY(LTD_readmem, LTD_writemem)
  MDRV_CPU_VBLANK_INT(LTD_vblank, 1)
  MDRV_CPU_PERIODIC_INT(LTD_irq, 260)
  MDRV_CORE_INIT_RESET_STOP(LTD,NULL,NULL)
  MDRV_NVRAM_HANDLER(generic_1fill)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_UPDATE(LTD)
  MDRV_SOUND_ADD(DISCRETE, ltd3_discInt)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(LTD3A)
  MDRV_IMPORT_FROM(LTD3)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_PERIODIC_INT(LTD_irq, 1111)

  MDRV_SOUND_ADD(AY8910, LTD_ay8910Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END


// System 4

static MACHINE_INIT(LTDHH) {
  memset(&locals, 0, sizeof locals);
  locals.isHH = 1;
}

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
  static UINT8 dispData[2];
  static int clear, lampCol, solBank;
  if (locals.port2 & 0x10) {
    switch (locals.cycle) {
      case  0: clear = data != 0xff; if (clear) lampCol = (1 + core_BitColToNum(data)) % 8; break;
      case  1: if (locals.isHH) {
                 if ((data & 0x0f) != 0x0f) locals.solenoids |= 0x10 << (data & 0x0f);
                 if ((data & 0xf0) != 0xf0) locals.solenoids |= 0x4000 << (data >> 4);
                 coreGlobals.solenoids = locals.solenoids;
               } else
                 solBank = core_BitColToNum(data);
               break;
      case  2: locals.solenoids |= (data >> 4) << (solBank * 4);
               locals.solenoids2 |= (data & 0x0f) << 4;
               coreGlobals.solenoids = locals.solenoids; coreGlobals.solenoids2 = locals.solenoids2; break;
      case  6: if (clear) locals.swCol = data >> 4;
               locals.segments[31-(data & 0x0f)].w = dispData[0];
               locals.segments[15-(data & 0x0f)].w = dispData[1]; break;
      case  7: if (clear) dispData[0] = convDisp(data); break;
      case  8: if (clear) dispData[1] = convDisp(data); break;
      case  9: if (clear) coreGlobals.tmpLampMatrix[(lampCol % 2 ? 8 : 12) + lampCol / 2] = locals.auxData; break;
      case 10: if (clear) coreGlobals.tmpLampMatrix[lampCol] = data; break;
      default: logerror("peri_%d_w = %02x\n", locals.cycle, data);
    }
  }
}

static READ_HANDLER(unknown_r) {
  return locals.port2;
}

static WRITE_HANDLER(cycle_w) {
  if (~locals.port2 & data & 0x10) locals.cycle++;
  if (locals.port2 == 0x04) locals.cycle = 0;
  locals.port2 = data;
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
  {0x0800,0x0800, ay8910_1_ctrl_w},
  {0x0c00,0x0c00, ay8910_1_mute},
  {0x1000,0x1000, ay8910_0_ctrl_w},
  {0x1400,0x1400, ay8910_0_mute},
  {0x1800,0x1800, ay8910_01_ctrl_w},
  {0x1c00,0x1c00, ay8910_01_reset},
  {0x2800,0x2800, ay8910_1_data_w},
  {0x3000,0x3000, ay8910_0_data_w},
  {0x3800,0x3800, ay8910_01_data_w},
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

MACHINE_DRIVER_START(LTD4HH)
  MDRV_IMPORT_FROM(LTD4)
  MDRV_CORE_INIT_RESET_STOP(LTDHH,NULL,NULL)
MACHINE_DRIVER_END
