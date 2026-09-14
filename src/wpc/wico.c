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
  UINT8  dispBlank; // bool
  int    vblankCount;
  int    gentimerCnt; // U2 LS393 divider state, see WICO_gentimer()
  UINT32 solenoids, solenoids2;
  UINT8  diagSegs;    // segments last latched by MUXEN, shown only while DLED1 is on
  UINT8  diagLedOn;   // bool
  UINT8  diagnosticLed;
} locals;

static UINT8 *shared_ram;

static INTERRUPT_GEN(WICO_irq_housekeeping) {
  cpu_set_irq_line(HOUSEKEEPING, M6809_IRQ_LINE, HOLD_LINE);
}

/* RFSHINT, the housekeeping CPU's FIRQ, comes off the display refresh chain (U49 LS175 /
   U44 LS74, clocked by the display scan), not off the U1 timer.  That chain is not
   modelled, so the rate stays at the one the driver has always used */
static void WICO_rfshint(int data) {
  cpu_set_irq_line(HOUSEKEEPING, M6809_FIRQ_LINE, PULSE_LINE);
}

/* U1 timer -> U2 LS393 QB (divide by 4) -> U44 LS74 -> the command CPU's IRQ */
static void WICO_gentimer(int data) {
  locals.gentimerCnt++;
  if (locals.gentimerCnt > 3) {
    /* The LS74 holds the line until the handler reads GENTMRCL ($1fea), so assert rather
       than pulse: the 6809 core samples the line, and a pulse that lands while the CPU
       still has I set is dropped */
    cpu_set_irq_line(COMMAND, M6809_IRQ_LINE, ASSERT_LINE);
    locals.gentimerCnt = 0;
  }
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(WICO_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
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
    // GENTMRCL: the command CPU's handler reads this to clear its timer irq.  Both CPUs
    // decode the same $1fe0-$1fef window here, so in theory a housekeeping read would
    // clear it too; no housekeeping ROM path touches $1fea, and the zero crossing reset in
    // io_w has always been equally caller-agnostic
    case 0x0a:
      cpu_set_irq_line(COMMAND, M6809_IRQ_LINE, CLEAR_LINE);
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
    /* SOLST1/SOLST0 gate U37/U38 (LS244) onto the bus with the solenoid driver status
       lines, which idle high through their pull-ups while a driver is off; U22 (LS133)
       watches the same lines to raise SOLTRIP.  The drivers are not modelled, so report
       the healthy state - all off, nothing tripped - which is what the ROMs test for
       ($1fee == $ff, $1fed & 3 == 3 in the housekeeping power-fail handler) */
    case 0x0d: // SOLST1
    case 0x0e: // SOLST0  (it is NOT a timer enable, as this used to assume)
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
    case 0: // MUXLD: resets the digit/scan counter.  It does NOT fire an NMI on the command
            // CPU, as this used to do: that ran the $f18b handler, whose 2048 iteration delay
            // loop burns ~13ms with I and F masked, so at ~47 MUXLD writes/s it ate 62% of the
            // CPU and halved the general timer rate - MAYBE also the cause of the previously too slow music/note tempo
      break;
    case 1: // STORE, enables NVRAM
      break;
    case 2: { // MUXEN: latches the diagnostic digit (MC14495) and the display enable
      // own table: core_bcd2seg7 only carries A-F in a MAME_DEBUG build
      static const UINT8 mc14495[16] = { 0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,
                                         0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71 };
      locals.diagSegs = mc14495[data >> 4];
      locals.diagnosticLed = locals.diagLedOn ? locals.diagSegs : 0;
      locals.dispBlank = !(data & 1);
      break;
    }
    case 3: // continuous solenoids
      locals.solenoids = (locals.solenoids & 0xffffff00) | data;
      break;
    case 4: // matrix solenoids, 42 in total!
      if (!data) {
        locals.solenoids &= 0x000000ff;
        locals.solenoids2 = coreGlobals.tmpLampMatrix[16] = coreGlobals.tmpLampMatrix[17] = coreGlobals.tmpLampMatrix[18] = 0;
      } else if (data < 0x10) {
        locals.solenoids = (locals.solenoids & 0xffffc0ff) | (0x100u << (data - 0x09));
      } else if (data < 0x18) {
        locals.solenoids = (locals.solenoids & 0xfff03fff) | (0x4000u << (data - 0x11));
      } else if (data < 0x20) {
        locals.solenoids = (locals.solenoids & 0xfc0fffff) | (0x100000u << (data - 0x19));
      } else if (data < 0x28) {
        locals.solenoids = (locals.solenoids & 0x03ffffff) | (0x4000000u << (data - 0x21));
      } else if (data < 0x30) {
        coreGlobals.tmpLampMatrix[16] = 1u << (data - 0x29);
        locals.solenoids2 = (locals.solenoids2 & 0xffffffc0) | (1u << (data - 0x29));
      } else if (data < 0x38) {
        coreGlobals.tmpLampMatrix[17] = 1u << (data - 0x31);
        locals.solenoids2 = (locals.solenoids2 & 0xfffff03f) | (0x40u << (data - 0x31));
      } else {
        coreGlobals.tmpLampMatrix[18] = 1u << (data - 0x39);
        locals.solenoids2 = (locals.solenoids2 & 0xfffc0fff) | (0x1000u << (data - 0x39));
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
    case 8: // DLED0: diagnostic LED off
      locals.diagLedOn = 0;
      locals.diagnosticLed = 0;
      break;
    case 9: // DLED1: diagnostic LED on
      locals.diagLedOn = 1;
      locals.diagnosticLed = locals.diagSegs;
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
}

struct SN76494interface WICO_sn76494Int = {
  1, /* total number of chips in the machine */
  { WICO_CLOCK_FREQ/64 }, /* base clock */ // pitch is okay, see https://www.youtube.com/watch?v=rwkggZ02r4E
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
  MDRV_CPU_ADD_TAG("mcpu housekeeping", M6809, WICO_CLOCK_FREQ/8.)
  MDRV_CPU_MEMORY(WICO_0_readmem, WICO_0_writemem)
  MDRV_CPU_PERIODIC_INT(WICO_irq_housekeeping, 120) // zero crossing
  /* U1 astable: R4 56K to +5V, C5 0.01uF to ground, DISCHARGE/THRESHOLD/TRIGGER commoned,
     so f = 1/(0.693*R*C) = ~2577Hz, and the divide by 4 in WICO_gentimer() puts the command
     CPU's IRQ at ~644Hz.  PinMAME and MAME both used to feed 750Hz in here and divide that,
     i.e. 187.5Hz, which ran the music/note tempo to slow.  ~2577Hz is calculated, not
     measured - a scope on the real board must settle it */
  MDRV_TIMER_ADD(WICO_gentimer, 2577) // note tempo also sounds fine with this now
  MDRV_TIMER_ADD(WICO_rfshint, 750)   // rate unknown, see WICO_rfshint()

  // command cpu: sound, solenoids
  MDRV_CPU_ADD_TAG("scpu command", M6809, WICO_CLOCK_FREQ/8.)
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
