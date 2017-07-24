/******************************************************************************************
  Mirco: Spirit of 76
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"

static struct {
  int vblankCount;
  UINT8 piaa, piab;
  UINT32 solenoids;
  int sol5, sol7;
} locals;

static INTERRUPT_GEN(spirit_vblank) {
  core_updateSw(TRUE); // game enables flippers directly
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x8000) | locals.solenoids;
  if (!locals.vblankCount) {
    locals.solenoids = (locals.sol5 ? 0x10 : 0) | (locals.sol7 ? 0x40 : 0);
  }
  locals.vblankCount = (locals.vblankCount + 1) % 3;
}

static INTERRUPT_GEN(spirit_irq) {
	static int state;
  cpu_set_irq_line(0, M6800_IRQ_LINE, (state = !state) ? ASSERT_LINE : CLEAR_LINE);
}

static SWITCH_UPDATE(spirit) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x08, 4);
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0x80, 5);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x88, 6);
  }
}

static WRITE_HANDLER(piaa_w) {
	locals.piaa = data;
}

static WRITE_HANDLER(piab_w) {
  static int tick5, tick7;
	int off;
	locals.piab = data;
	if (data & 0x80) {
		coreGlobals.segments[data & 0x0f].w = core_bcd2seg[locals.piaa & 0x0f];
		// seg 16: zero for match digit
		coreGlobals.segments[16].w = coreGlobals.segments[6].w ? core_bcd2seg7[0] : 0;
		// seg 17: always zero
		coreGlobals.segments[17].w = core_bcd2seg7[0];
		off = data % 8;
		if (data & 0x08) {
		  coreGlobals.lampMatrix[off] = (coreGlobals.lampMatrix[off] & 0x0f) | (locals.piaa & 0xf0);
		} else {
		  coreGlobals.lampMatrix[off] = (coreGlobals.lampMatrix[off] & 0xf0) | (locals.piaa >> 4);
		  // mapping two digits to lamps
		  if (off == 5) coreGlobals.lampMatrix[8] = (coreGlobals.lampMatrix[8] & 0xf0) | (~locals.piaa & 0x0f);
		  if (off == 7) coreGlobals.lampMatrix[8] = (coreGlobals.lampMatrix[8] & 0x0f) | ((~locals.piaa & 0x0f) << 4);
		}
  } else if (data & 0x20) {
    off = data & 0x0f;
    tick5 = off == 5 ? 0 : tick5 + 1;
 	  locals.sol5 = tick5 < 4;
    tick7 = off == 7 ? 0 : tick7 + 1;
 	  locals.sol7 = tick7 < 4;
  	if (off) {
  	  locals.solenoids |= 1 << (off - 1);
      coreGlobals.solenoids = (coreGlobals.solenoids & 0x8000) | locals.solenoids;
  	}
  }
}

// no idea what this might be but it's definitely used, so mapping it as solenoid 16
static WRITE_HANDLER(piacb2_w) {
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x7fff) | (data << 15);
}

static struct pia6821_interface pia = {
/* I:  A/B,CA1/B1,CA2/B2 */  0, 0, 0, 0, 0, 0,
/* O:  A/B,CA2/B2        */  piaa_w, piab_w, 0, piacb2_w,
/* IRQ: A/B              */  0, 0
};

static MACHINE_INIT(spirit) {
  memset(&locals, 0, sizeof(locals));
  pia_config(0, PIA_STANDARD_ORDERING, &pia);
}

static READ_HANDLER(sw_r) {
	int row = (locals.piab & 0x0f) / 2;
	int shiftBits = locals.piab % 2 ? 4 : 0;
  if (row > 5) {
    return core_revbyte(core_getDip(row - 6) >> (4 - shiftBits)) >> 4;
  }
  return coreGlobals.swMatrix[row + 1] >> shiftBits;
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
static MEMORY_READ_START(spirit_readmem)
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x0600, 0x0fff, MRA_ROM },
  { 0x2200, 0x2203, pia_r(0) },
  { 0x2400, 0x2400, sw_r },
  { 0xfff0, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(spirit_writemem)
  { 0x0000, 0x00ff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x2200, 0x2203, pia_w(0) },
  { 0x2401, 0x2401, MWA_NOP },
MEMORY_END

static MACHINE_DRIVER_START(spirit)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(spirit,NULL,NULL)
  MDRV_CPU_ADD_TAG("mcpu", M6800, 500000)
  MDRV_CPU_MEMORY(spirit_readmem, spirit_writemem)
  MDRV_CPU_VBLANK_INT(spirit_vblank, 1)
  MDRV_CPU_PERIODIC_INT(spirit_irq, 500)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(spirit)
  MDRV_NVRAM_HANDLER(generic_0fill)
MACHINE_DRIVER_END

static const core_tLCDLayout disp[] = {
  {2, 0, 0,5,CORE_SEG7}, {2,10,17,1,CORE_SEG7},
  {2,18,10,6,CORE_SEG7},
  {0,13, 8,2,CORE_SEG7},
  {4,13, 6,1,CORE_SEG7}, {4,15,16,1,CORE_SEG7},
  {0}
};
static core_tGameData spirit76GameData = {0,disp,{FLIP_SWNO(32,36),0,1}};
static void init_spirit76(void) {
  core_gameData = &spirit76GameData;
}
ROM_START(spirit76)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD_NIB_LOW("1g.bin", 0x0600, 0x0200, CRC(57d7213c) SHA1(0897876f5c662b2518a680bcbfe282bb3a19a161))
    ROM_LOAD_NIB_HIGH("5g.bin", 0x0600, 0x0200, CRC(90e22786) SHA1(da9e0eae1e8576c6c8ac734a9557784d9e59c141))
    ROM_LOAD_NIB_LOW("2c.bin", 0x0800, 0x0200, CRC(4b996a52) SHA1(c73378e61598f84e20c1022b811780e300b01cd1))
    ROM_LOAD_NIB_HIGH("3c.bin", 0x0800, 0x0200, CRC(448626fa) SHA1(658b9589ba60ef62ff692192f743038d622776ba))
    ROM_LOAD_NIB_LOW("2e.bin", 0x0a00, 0x0200, CRC(faaa907e) SHA1(ee9227944911a7c068216dd7b1b8dec284f90e3b))
    ROM_LOAD_NIB_HIGH("3e.bin", 0x0a00, 0x0200, CRC(3463168e) SHA1(d98643179eac5ecbf1a559df59da620ea544bdee))
    ROM_LOAD_NIB_LOW("2f.bin", 0x0c00, 0x0200, CRC(4d1a71ec) SHA1(6d3aa8fc4f7cec27d7fae2ecc73425388f8d9d52))
    ROM_LOAD_NIB_HIGH("3f.bin", 0x0c00, 0x0200, CRC(bf23f0fd) SHA1(62e2ef7df0c057f25685a99e57cf95aae2e75cdb))
    ROM_LOAD_NIB_LOW("2g.bin", 0x0e00, 0x0200, CRC(6236f053) SHA1(6183c8fa7dbd32ec40c4668cab8010b5e8c49949))
      ROM_RELOAD(0xfe00, 0x0200)
    ROM_LOAD_NIB_HIGH("3g.bin", 0x0e00, 0x0200, CRC(ae7192cd) SHA1(9ba76e81b8603163c22f47f1a99da310b4325e84))
      ROM_RELOAD(0xfe00, 0x0200)
ROM_END
INPUT_PORTS_START(spirit76)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT   (0x0800, "Game start", KEYCODE_1)
    COREPORT_BIT   (0x0080, "Coin 1",     KEYCODE_3)
    COREPORT_BIT   (0x0008, "Coin 2",     KEYCODE_5)
    COREPORT_BIT   (0x8000, "Tilt",       KEYCODE_INSERT)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0001, 0x0000, "S1")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0001, "1" )
    COREPORT_DIPNAME( 0x0002, 0x0002, "S2")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0002, "1" )
    COREPORT_DIPNAME( 0x0004, 0x0004, "S3")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0004, "1" )
    COREPORT_DIPNAME( 0x0008, 0x0000, "S4")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0008, "1" )
    COREPORT_DIPNAME( 0x0010, 0x0010, "S5")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0010, "1" )
    COREPORT_DIPNAME( 0x0020, 0x0020, "S6")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0020, "1" )
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0040, "1" )
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0080, "1" )
    COREPORT_DIPNAME( 0x0100, 0x0000, "S9")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0100, "1" )
    COREPORT_DIPNAME( 0x0200, 0x0000, "S10")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0200, "1" )
    COREPORT_DIPNAME( 0x0400, 0x0000, "S11")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0400, "1" )
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x0800, "1" )
    COREPORT_DIPNAME( 0x1000, 0x0000, "S13")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x1000, "1" )
    COREPORT_DIPNAME( 0x2000, 0x0000, "S14")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x2000, "1" )
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x4000, "1" )
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16")
      COREPORT_DIPSET(0x0000, "0" )
      COREPORT_DIPSET(0x8000, "1" )
INPUT_PORTS_END
CORE_GAMEDEFNV(spirit76,"Spirit of 76",1975,"Mirco",spirit,GAME_USES_CHIMES)
