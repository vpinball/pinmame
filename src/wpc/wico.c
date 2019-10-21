/************************************************************************************************
 Wico Af-Tor (Wico's sole commercial game)
 -----------

   Hardware:
   ---------
		CPU:	 2 x 6809 for game and sound, they share one RAM chip!
			INT: IRQ @ ZC-speed for 1st CPU,
			     NE555 chip for 2nd CPU
		DISPLAY: 9-segment LCD
		SOUND:	 SN76494 (76496-compatible)
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "gen.h"
#include "cpu/m6809/m6809.h"
#include "sound/sn76496.h"

#define WICO_CLOCK_FREQ	10000000
#define HOUSEKEEPING 0
#define COMMAND 1

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    dispBlank;
  int    vblankCount;
  int    firqtimer;
  UINT32 solenoids, solenoids2;
  UINT8	 gtIRQEnable;
  UINT8	 diagnosticLed;
} locals;

static UINT8 *shared_ram;

static INTERRUPT_GEN(WICO_irq_housekeeping) {
  cpu_set_irq_line(HOUSEKEEPING, M6809_IRQ_LINE, HOLD_LINE);
}

static void WICO_firq_housekeeping(int data) {
  if (!locals.gtIRQEnable)
    cpu_set_irq_line(HOUSEKEEPING, M6809_FIRQ_LINE, PULSE_LINE);

  // Gen. timer irq of command CPU kicks in every 4 interrupts of this timer
  locals.firqtimer++;
  if (locals.firqtimer > 4) {
    cpu_set_irq_line(COMMAND, M6809_IRQ_LINE, PULSE_LINE);
    locals.firqtimer = 0;
  }
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(WICO_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  coreGlobals.solenoids = locals.solenoids;
  /*-- diag. LED --*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(8));
}

static SWITCH_UPDATE(WICO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x0b, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x07, 11);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x80, 0);
  }
}

static READ_HANDLER(io_r) {
  UINT8 swCol, dispCol, i, ret = 0;
  switch (offset) {
    case 0x0a:
//      locals.gtIRQEnable = 1;
      ret = 0xff;
      break;
    case 0x0b: // LAMPST, reads display digits from RAM
      dispCol = shared_ram[0x0096] & 7;
      for (i = 0; i < 5; i++) {
        if (locals.dispBlank) {
          coreGlobals.segments[8 * i + dispCol].w = 0;
        } else {
          coreGlobals.segments[8 * i + dispCol].w = (shared_ram[0x7f9 + i] & 0x7f) | ((shared_ram[0x7f9 + i] & 0x80) ? 0x300 : 0);
        }
      }
      ret = 0xff;
      break;
    case 0x0e:
      locals.gtIRQEnable = 0;
      ret = 0xff;
      break;
    case 0x0f:
      swCol = shared_ram[0x0095] % 16;
      if (swCol > 11) {
        ret = core_getDip(swCol - 12);
        if (swCol == 15) {
          ret |= coreGlobals.swMatrix[0] & 0x80; // include self test button (same input line as dip #32)
        }
      } else {
        ret = coreGlobals.swMatrix[1 + swCol];
      }
      break;
  }
  logerror("io_r: CPU %d PC=%04X offset %x ret %02x RAM VAL %02x\n", cpu_getactivecpu(), activecpu_get_previouspc(), offset, ret, shared_ram[0x0096]);
  return ret;
}

static WRITE_HANDLER(io_w) {
  switch (offset) {
    case 0: // fire NMI? marked MUXLD, enables write to 0x1fe1 on cpu #1
      cpu_set_nmi_line(COMMAND, PULSE_LINE);
      break;
    case 1: // STORE, enables NVRAM
      break;
    case 2: // diagnostic 7-seg digit, display blanking
      locals.diagnosticLed = core_bcd2seg7[data >> 4];
      locals.dispBlank = !(data & 1);
      break;
    case 3: // continuous solenoids
      locals.solenoids = (locals.solenoids & 0xffffff00) | data;
      break;
    case 4: // matrix solenoids, 42 in total!
      if (!data) {
        locals.solenoids &= 0x000000ff;
        locals.solenoids2 = coreGlobals.tmpLampMatrix[16] = coreGlobals.tmpLampMatrix[17] = coreGlobals.tmpLampMatrix[18] = 0;
      } else if (data < 0x10) {
        locals.solenoids = (locals.solenoids & 0xffffc0ff) | (0x100 << (data - 0x09));
      } else if (data < 0x18) {
        locals.solenoids = (locals.solenoids & 0xfff03fff) | (0x4000 << (data - 0x11));
      } else if (data < 0x20) {
        locals.solenoids = (locals.solenoids & 0xfc0fffff) | (0x100000 << (data - 0x19));
      } else if (data < 0x28) {
        locals.solenoids = (locals.solenoids & 0x03ffffff) | (0x4000000 << (data - 0x21));
      } else if (data < 0x30) {
        coreGlobals.tmpLampMatrix[16] = 1 << (data - 0x29);
        locals.solenoids2 = (locals.solenoids2 & 0xffffffc0) | (1 << (data - 0x29));
      } else if (data < 0x38) {
        coreGlobals.tmpLampMatrix[17] = 1 << (data - 0x31);
        locals.solenoids2 = (locals.solenoids2 & 0xfffff03f) | (0x40 << (data - 0x31));
      } else {
        coreGlobals.tmpLampMatrix[18] = 1 << (data - 0x39);
        locals.solenoids2 = (locals.solenoids2 & 0xfffc0fff) | (0x1000 << (data - 0x39));
      }
      break;
    case 5: // sound
      SN76494_0_w(0, data);
      break; 
    case 6: // watchdog housekeeping cpu reset line
      if (data == 0xff) {
        cpunum_set_reset_line(HOUSEKEEPING, CLEAR_LINE); // release reset line so housekeeping (cpu0) starts
      } else {
        if (data) logerror("io_w: offset %x, data %02x not handled\n", offset, data);
      }
      break;
    case 7: // zero crossing interrupt reset
      cpu_set_irq_line(HOUSEKEEPING, M6809_IRQ_LINE, CLEAR_LINE);
      logerror("io_w: ZC INT RESET offset %x, data %02x\n", offset, data);
      break;
    case 9: // enable/disable general timing interrupt
      locals.gtIRQEnable = data;
      logerror("io_w: INFO GT IRQ   offset %x, data %02x\n", offset, data);
      break;
  }
  if (offset != 6) logerror("io_w: CPU %d PC=%04X offset %x, data %02x\n", cpu_getactivecpu(), activecpu_get_previouspc(), offset, data);
}

static READ_HANDLER(shared_ram_r) {
  return shared_ram[offset];
}
static WRITE_HANDLER(shared_ram_w) {
  shared_ram[offset] = data;

  if (offset > 0x45 && offset < 0x56) {
  	coreGlobals.tmpLampMatrix[offset - 0x46] = data;
  }
}

static UINT8 *nvram;
// nvram uses lower nibble only
static NVRAM_HANDLER(WICO) {
  core_nvram(file, read_or_write, nvram, 0x100, 0);
}
static READ_HANDLER(nvram_r) {
  return nvram[offset];
}
static WRITE_HANDLER(nvram_w) {
  nvram[offset] = data & 0x0f;
}

static MEMORY_READ_START(WICO_0_readmem)
  {0x0000,0x07ff, shared_ram_r},
  {0x1fe0,0x1fef, io_r},
  {0xf000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(WICO_0_writemem)
  {0x0000,0x07ff, shared_ram_w, &shared_ram},
  {0x1fe0,0x1fef, io_w},
MEMORY_END

static MEMORY_READ_START(WICO_1_readmem)
  {0x0000,0x07ff, shared_ram_r},
  {0x1fe0,0x1fef, io_r},
  {0x4000,0x40ff, nvram_r},
  {0x8000,0x9fff, MRA_ROM},
  {0xe000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(WICO_1_writemem)
  {0x0000,0x07ff, shared_ram_w},
  {0x1fe0,0x1fef, io_w},
  {0x4000,0x40ff, nvram_w, &nvram},
MEMORY_END

static MACHINE_INIT(WICO) {
  memset(&locals, 0, sizeof locals);  
  memset(shared_ram, 0x12, 0x800);  
  cpunum_set_reset_line(HOUSEKEEPING, ASSERT_LINE);
  cpu_set_irq_line(HOUSEKEEPING, M6809_IRQ_LINE, CLEAR_LINE);
}

static MACHINE_RESET(WICO) {
  memset(&locals, 0, sizeof locals);  
  memset(shared_ram, 0x12, 0x800);  
  cpunum_set_reset_line(HOUSEKEEPING, ASSERT_LINE);
  cpu_set_irq_line(HOUSEKEEPING, M6809_IRQ_LINE, CLEAR_LINE);
}

struct SN76494interface WICO_sn76494Int = {
  1, /* total number of chips in the machine */
  { WICO_CLOCK_FREQ/8 }, /* base clock */
  { 75 } /* volume */
};

MACHINE_DRIVER_START(aftor)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(WICO,WICO,NULL)
  MDRV_SWITCH_UPDATE(WICO)
  MDRV_DIPS(32)
  MDRV_NVRAM_HANDLER(WICO)
  MDRV_DIAGNOSTIC_LED7

  // housekeeping cpu: displays, switches
  MDRV_CPU_ADD_TAG("mcpu housekeeping", M6809, WICO_CLOCK_FREQ/8)
  MDRV_CPU_MEMORY(WICO_0_readmem, WICO_0_writemem)  
  MDRV_CPU_PERIODIC_INT(WICO_irq_housekeeping, 120) // zero crossing
  MDRV_TIMER_ADD(WICO_firq_housekeeping, 750) // time generator

  // command cpu: sound, solenoids
  MDRV_CPU_ADD_TAG("scpu command", M6809, WICO_CLOCK_FREQ/8)
  MDRV_CPU_MEMORY(WICO_1_readmem, WICO_1_writemem)
  MDRV_CPU_VBLANK_INT(WICO_vblank, 1)
  MDRV_SOUND_ADD(SN76494, WICO_sn76494Int)
MACHINE_DRIVER_END

INPUT_PORTS_START(aftor) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
  COREPORT_BITDEF(0x0008, IPT_START1, IP_KEY_DEFAULT) \
  COREPORT_BITDEF(0x0001, IPT_COIN1, IP_KEY_DEFAULT) \
  COREPORT_BITDEF(0x0002, IPT_COIN2, IP_KEY_DEFAULT) \
  COREPORT_BIT   (0x0100, "Slam Tilt", KEYCODE_HOME) \
  COREPORT_BIT   (0x0200, "Playfield Tilt", KEYCODE_INSERT) \
  COREPORT_BIT   (0x0400, "Pendulum Tilt", KEYCODE_DEL) \
  COREPORT_BITTOG(0x8000, "Service", KEYCODE_9) \

  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S5") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" ) \
  PORT_START /* 2 */ \
    COREPORT_DIPNAME( 0x0001, 0x0000, "S17") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0000, "S18") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "S19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "S20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, "S21") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0010, "1" ) \
    COREPORT_DIPNAME( 0x0020, 0x0000, "S22") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, "S23") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "S24") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" ) \
    COREPORT_DIPNAME( 0x0100, 0x0000, "S25") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0100, "1" ) \
    COREPORT_DIPNAME( 0x0200, 0x0000, "S26") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0200, "1" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, "S27") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0400, "1" ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, "S28") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0800, "1" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "S29") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x1000, "1" ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, "S30") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x2000, "1" ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "S31") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x4000, "1" ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "S32") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END

ROM_START(aftor) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("u25.bin", 0xf000, 0x1000, CRC(d66e95ff) SHA1(f7e8c51f1b37e7ef560406f1968c12a2043646c5)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("u52.bin", 0x8000, 0x2000, CRC(8035b446) SHA1(3ec59015e259c315bf09f4e2046f9d98e2d7a732)) \
    ROM_LOAD("u48.bin", 0xe000, 0x2000, CRC(b4406563) SHA1(6d1a9086eb1f6f947eae3a92ccf7a9b7375d85d3)) \
ROM_END

static core_tLCDLayout dispAftor[] = {
  {0, 0, 8,7,CORE_SEG9},
  {0,24,24,7,CORE_SEG9},
  {2, 0,16,7,CORE_SEG9},
  {2,24,32,7,CORE_SEG9},
  {1,21, 0,2,CORE_SEG7S},
  {1,26, 2,2,CORE_SEG7S},
  {0}
};

static int aftor_getSol(int solNo) {
  return locals.solenoids2 & (1 << (solNo - 51));
}
static core_tGameData aftorGameData = {GEN_WICO,dispAftor,{FLIP_SW(FLIP_L),4,11,18,0,0,0,0,aftor_getSol}};
static void init_aftor(void) { core_gameData = &aftorGameData; }

CORE_GAMEDEFNV(aftor,"Af-Tor",1984,"Wico",aftor,0)
