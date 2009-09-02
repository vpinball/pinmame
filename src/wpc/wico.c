/************************************************************************************************
 Wico Af-Tor (Wico's sole commercial game)
 -----------

   Hardware:
   ---------
		CPU:	 2 x 6809 for game and sound, they share one RAM chip!
			INT: IRQ @ ZC-speed for 1st CPU,
			     NE555 chip for 2nd CPU
		DISPLAY: 9-segment LCD
		SOUND:	 SN76494 (76496-compatible?)
 ************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "gen.h"
#include "cpu/m6809/m6809.h"
#include "sound/sn76496.h"

#define WICO_CLOCK_FREQ	10000000

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  sndCmd;
  UINT8  swCol;
  UINT8  lampCol;
  UINT8	 zcIRQEnable;
  UINT8	 gtIRQEnable;
  UINT8	 diagnosticLed;
} locals;

static int wico_data2seg[0x60] = {
// 0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F
  0x3f,  0x300, 0x5b,  0x4f,  0x360, 0x6d,  0x7d,  0x07,  0x7f,  0x6f,  0x77,  0x34f, 0x39,  0x30f, 0x79,  0x71, // 0
  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,    // 1
  0,     0,     0x22,  0,     0,     0,     0,     0x02,  0x39,  0x0f,  0,     0x340, 0x10,  0x40,  0,     0x52, // 2   "    '() +,- /
  0x3f,  0x300, 0x5b,  0x4f,  0x360, 0x6d,  0x7d,  0x07,  0x7f,  0x6f,  0,     0,     0,     0x48,  0,     0x53, // 3 0123456789   = ?
  0,     0x77,  0x34f, 0x39,  0x30f, 0x79,  0x71,  0x3d,  0x76,  0x309, 0x1e,  0x374, 0x38,  0x337, 0x37,  0x3f, // 4  ABCDEFGHIJKLMNO
  0x73,  0x36b, 0x347, 0x6d,  0x301, 0x3e,  0x338, 0x33e, 0x364, 0x362, 0x5b,  0x39,  0x64,  0x0f,  0x23,  0x08  // 5 PQRSTUVWXYZ[\]^_
};

static void WICO_firq_0(void) {
	cpu_set_irq_line(0, M6809_FIRQ_LINE, ASSERT_LINE);
	cpu_set_irq_line(0, M6809_FIRQ_LINE, CLEAR_LINE);
}

static INTERRUPT_GEN(WICO_irq_0) {
  if (locals.zcIRQEnable) cpu_set_irq_line(0, M6809_IRQ_LINE, PULSE_LINE); 
}

static INTERRUPT_GEN(WICO_irq_1) {
  static int firqtimer = 0;

  if (!locals.gtIRQEnable)
  {
	  cpu_set_irq_line(1, M6809_IRQ_LINE, PULSE_LINE);
	  
	  // firq of CPU 0 kicks in every 4 interrupts of the general timer
	  firqtimer++;
	  if (firqtimer > 4) {
		  WICO_firq_0();
		  firqtimer = 0;
	  }
  }
}

static void WICO_firq_1(int data) {
  static int last;
  cpu_set_irq_line(1, M6809_FIRQ_LINE, (last = !last) ? ASSERT_LINE : CLEAR_LINE);
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

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(WICO) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xff, 0);
  }
//  cpu_set_nmi_line(0, coreGlobals.swMatrix[0] & 1 ? ASSERT_LINE : CLEAR_LINE);
//  if (coreGlobals.swMatrix[0] & 2) WICO_firq_0(0);
//  cpu_set_nmi_line(1, coreGlobals.swMatrix[0] & 4 ? ASSERT_LINE : CLEAR_LINE);
//  if (coreGlobals.swMatrix[0] & 8) WICO_firq_1(0);
}

static UINT8 *ram_01;
static WRITE_HANDLER(io0_w) {
  switch (offset) {
	case 0xe0: // matrix scan begin???
		break;

	case 0xe2: // segment display
		coreGlobals.segments[36 + (data & 0x0f)].w = wico_data2seg[data >> 4];
		if ((data >> 4) == 0x08) logerror("CPU0 SELF TEST DONE -------------------\n");
		break;

	case 0xe6: // watchdog keep alive??? 
		if (data == 0x00) coreGlobals.diagnosticLed = locals.diagnosticLed & 0x01;
		else logerror("io0_w: offset %x, data %02x not handled\n", offset, data);
		locals.diagnosticLed = ~locals.diagnosticLed; 
		break;

	case 0xe7: // zero crossing interrupt
		locals.zcIRQEnable = data; // enable/disable zero crossing interrupt
		logerror("io0_w: ZC INT ENABLE offset %x, data %02x\n", offset, data);
		break;
	
    default:
      logerror("io0_w: PC=%X offset %x, data %02x\n", cpunum_get_pc(0), offset, data);
  }
}

static READ_HANDLER(io0_r) {
  UINT8 ret = 0;

  switch (offset) {
    case 0xeb: case 0xec: case 0xed: case 0xee:
      ret = core_getDip(offset - 0xeb);
      break;
    case 0xef:
      ret = coreGlobals.swMatrix[0];
      break;
    default:
      //logerror("io0_r: offset %x\n", offset);
	  logerror("io0_r: PC=%X offset %x ret %02x RAM VAL %02x\n", cpunum_get_pc(0), offset, ret, ram_01[0x0096]);

  }

  return ret;
}
static WRITE_HANDLER(io1_w) {
  switch (offset) {
	case 0xe2: // segment display digit
		coreGlobals.segments[36 + (data & 0x0f)].w = wico_data2seg[data >> 4];
		if ((data >> 4) == 0x0c) logerror("CPU1 SELF TEST DONE -------------------\n");
		break;
	case 0xe3: // solenoids
		locals.solenoids = (locals.solenoids & 0xffffff00) | data;
		break;
	case 0xe4: // more solenoids
		locals.solenoids = (locals.solenoids & 0xffff00ff) | (data << 8);
		break;
	case 0xe5: // sound
		SN76496_0_w(0, data);
		break;
	case 0xe6: // watchdog housekeeping cpu reset line
		if (data == 0xff) cpunum_set_reset_line(0, CLEAR_LINE); // release reset line so housekeeping (cpu0) starts
		else 
		{
			cpunum_set_reset_line(0, ASSERT_LINE);
			logerror("watchdog assert line\n");
		}
	  break;
	case 0xe9: // enable/disable general timing interrupt
		locals.gtIRQEnable = data;
		logerror("io1_w: INFO GT IRQ offset %x, data %02x\n", offset, data);
	  break;
  default:
      logerror("io1_w: PC=%X offset %x, data %02x\n", cpunum_get_pc(1), offset, data);
  }
}
static READ_HANDLER(io1_r) {
	switch (offset) {
    default:
      logerror("io1_r: PC=%X offset %x\n", cpunum_get_pc(1), offset); return 0;
  }
}

static READ_HANDLER(ram_01_r) {
  return ram_01[offset];
}
static WRITE_HANDLER(ram_01_w) {
  ram_01[offset] = data;
  if (offset > 0x09 && offset < 0x2e) {
    coreGlobals.segments[offset - 0x0a].w = wico_data2seg[data];
  }
}

static UINT8 *nvram;
// nvram uses lower nibble only
static READ_HANDLER(nvram_r) {
  return nvram[offset];
}
static WRITE_HANDLER(nvram_w) {
  nvram[offset] = data & 0x0f;
}

static MEMORY_READ_START(WICO_0_readmem)
  {0x0000,0x07ff, ram_01_r},
  {0xf000,0xffff, MRA_ROM},
  {0x1f00,0x1fff, io0_r},
MEMORY_END

static MEMORY_WRITE_START(WICO_0_writemem)
  {0x0000,0x07ff, ram_01_w, &ram_01},
  {0x1f00,0x1fff, io0_w},
MEMORY_END

static MEMORY_READ_START(WICO_1_readmem)
  {0x0000,0x07ff, ram_01_r},
  {0x8000,0x9fff, MRA_ROM},
  {0xe000,0xffff, MRA_ROM},
  {0x1f00,0x1fff, io1_r},
  {0x4000,0x40ff, nvram_r},
MEMORY_END

static MEMORY_WRITE_START(WICO_1_writemem)
  {0x0000,0x07ff, ram_01_w},
  {0x1f00,0x1fff, io1_w},
  {0x4000,0x40ff, nvram_w, &nvram},
MEMORY_END

static MACHINE_INIT(WICO) {
  memset(&locals, 0, sizeof locals);  
  cpunum_set_reset_line(0, ASSERT_LINE);

//  coreGlobals.swMatrix[16] = 0x80; // DEBUG SELF-TEST
}

struct SN76496interface WICO_sn76494Int = {
	1,	/* total number of chips in the machine */
	{ WICO_CLOCK_FREQ/8 },	/* base clock */
	{ 75 }	/* volume */
};


MACHINE_DRIVER_START(aftor)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(WICO,NULL,NULL)
  MDRV_SWITCH_UPDATE(WICO)
  MDRV_DIPS(16)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIAGNOSTIC_LEDH(2)

  MDRV_CPU_ADD_TAG("mcpu housekeeping", M6809, WICO_CLOCK_FREQ/8)
//  MDRV_CPU_VBLANK_INT(WICO_vblank, 1)
  MDRV_CPU_MEMORY(WICO_0_readmem, WICO_0_writemem)  
  MDRV_CPU_PERIODIC_INT(WICO_irq_0, 120) // zero crossing  
//  MDRV_TIMER_ADD(WICO_firq_0, 30)

  // game & sound section
  MDRV_CPU_ADD_TAG("scpu command", M6809, WICO_CLOCK_FREQ/8)
  MDRV_CPU_VBLANK_INT(WICO_vblank, 1)
  MDRV_CPU_MEMORY(WICO_1_readmem, WICO_1_writemem)
  MDRV_CPU_PERIODIC_INT(WICO_irq_1, 1500) // time generator
  MDRV_SOUND_ADD(SN76496, WICO_sn76494Int)
//  MDRV_TIMER_ADD(WICO_firq_1, 1) // watchdog reset trigger
MACHINE_DRIVER_END

INPUT_PORTS_START(aftor) \
  CORE_PORTS \
  SIM_PORTS(1) \
  PORT_START /* 0 */ \
  COREPORT_BIT(     0x0001, "Test 1", KEYCODE_1) \
	COREPORT_BIT(     0x0002, "Test 2", KEYCODE_2) \
	COREPORT_BIT(     0x0004, "Test 3", KEYCODE_3) \
	COREPORT_BIT(     0x0008, "Test 4", KEYCODE_4) \
	COREPORT_BIT(     0x0010, "Test 5", KEYCODE_5) \
	COREPORT_BIT(     0x0020, "Test 6", KEYCODE_6) \
	COREPORT_BIT(     0x0040, "Test 7", KEYCODE_7) \
	COREPORT_BIT(     0x0080, "Test 8", KEYCODE_8) \

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
INPUT_PORTS_END

ROM_START(aftor) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD("u25.bin", 0xf000, 0x1000, CRC(d66e95ff) SHA1(f7e8c51f1b37e7ef560406f1968c12a2043646c5)) \
  NORMALREGION(0x10000, REGION_CPU2) \
    ROM_LOAD("u52.bin", 0x8000, 0x2000, CRC(8035b446) SHA1(3ec59015e259c315bf09f4e2046f9d98e2d7a732)) \
    ROM_LOAD("u48.bin", 0xe000, 0x2000, CRC(b4406563) SHA1(6d1a9086eb1f6f947eae3a92ccf7a9b7375d85d3)) \
ROM_END

static core_tLCDLayout dispAftor[] = {
  {0, 0, 1,7,CORE_SEG9},
  {0,16, 9,7,CORE_SEG9},
  {2, 0,17,7,CORE_SEG9},
  {2,16,25,7,CORE_SEG9},
  {4,10,32,2,CORE_SEG9},
  {4,16,34,2,CORE_SEG9},
  {4,26,36,2,CORE_SEG9},
  {0}
};
static core_tGameData aftorGameData = {GEN_ALVG,dispAftor,{FLIP_SW(FLIP_L),0,2}};
static void init_aftor(void) { core_gameData = &aftorGameData; }

CORE_GAMEDEFNV(aftor,"Af-Tor",1984,"Wico",aftor,GAME_NOT_WORKING)
