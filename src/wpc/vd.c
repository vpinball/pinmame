/************************************************************************************************
 Video Dens (Spain)
 ------------------------

   Hardware:
   ---------
		CPU:     Z80 @ 4? MHz
			INT: IRQ @ 484*2 Hz (Uptime counter in RAM seems to run twice too fast but games seem ok)
		IO:      CPU ports
		DISPLAY: 7-digit 8-segment panels, direct segment access
		SOUND:	 2 x AY-8910
 ************************************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "core.h"
#include "sound/ay8910.h"
#include "sndbrd.h"

#define JP_IRQFREQ 484*2 /* IRQ frequency */
#define JP_CPUFREQ 4000000 /* CPU clock frequency */

/*----------------
/  Local variables
/-----------------*/
static struct {
  int    vblankCount, solCount;
  UINT32 solenoids;
  core_tSeg segments;
  int    col;
} locals;

static INTERRUPT_GEN(VD_irq) {
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(VD_vblank) {
  locals.vblankCount++;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  /*-- solenoids --*/
  if (locals.vblankCount) {
    coreGlobals.solenoids = locals.solenoids;
  }
  /*-- display --*/
  memcpy(coreGlobals.segments, locals.segments, sizeof(locals.segments));

  core_updateSw(core_getSol(17));
}

static SWITCH_UPDATE(VD) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8, 0x40, 1);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>1, 0x01, 2);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>2, 0x01, 3);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>3, 0x01, 4);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x41, 5);
  }
}

static READ_HANDLER(port_a_r) {
  return ~core_getDip(0);
}

static READ_HANDLER(port_b_r) {
  return ~core_getDip(1);
}

struct AY8910interface VD_ay8910Int = {
  2,         /* 2 chips ? */
  2000000,   /* 2 MHz ? */
  { MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) }, /* Volume */
  { port_a_r }, { port_b_r },
};

static READ_HANDLER(clr_r) {
//  cpu_set_irq_line(0, 0, CLEAR_LINE);
  return 0;
}

static READ_HANDLER(sw_r) {
  return coreGlobals.swMatrix[1+offset];
}

static WRITE_HANDLER(col_w) {
  if ((data & 0x0f) < 0x08) locals.col = data & 0x07;
  else locals.solenoids = (locals.solenoids & 0xffff) | ((data & 0xe0) << 11);
  coreGlobals.diagnosticLed = (data >> 4) & 1;
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.tmpLampMatrix[offset] = data;
}

static WRITE_HANDLER(disp_w) {
  int digit = offset*8 + locals.col;
  locals.segments[digit].w = (locals.segments[digit].w & 0x80) | (core_revbyte(data) & 0x7f);
  // comma segments are carried by the following digit!
  if (digit) locals.segments[digit-1].w = (locals.segments[digit-1].w & 0x7f) | ((data & 1) << 7);
}

static WRITE_HANDLER(sol_w) {
  locals.vblankCount = -1;
  if (data) {
    locals.solenoids = (locals.solenoids & 0x10000) | (1 << (data-1));
    coreGlobals.solenoids |= locals.solenoids;
  } else locals.solenoids = locals.solenoids & 0x10000;
}

static MEMORY_READ_START(VD_readmem)
  {0x0000,0x5fff, MRA_ROM},
  {0x6000,0x67ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(VD_writemem)
  {0x6000,0x67ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(VD_readport)
  {0x00,0x05, sw_r},
  {0x61,0x61, AY8910_read_port_0_r},
  {0xa0,0xa0, clr_r},
PORT_END

static PORT_WRITE_START(VD_writeport)
  {0x20,0x27, lamp_w},
  {0x28,0x28, sol_w},
  {0x40,0x44, disp_w},
  {0x60,0x60, AY8910_control_port_0_w},
  {0x62,0x62, AY8910_write_port_0_w},
  {0x80,0x80, AY8910_control_port_1_w},
  {0x82,0x82, AY8910_write_port_1_w},
  {0xc0,0xc0, col_w},
PORT_END

static MACHINE_INIT(VD) {
  memset(&locals, 0, sizeof locals);
}

#define VD_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BITDEF(0x0002, IPT_START1,      IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0008, IPT_COIN1,       IP_KEY_DEFAULT) \
    COREPORT_BITDEF(0x0001, IPT_COIN2,       IP_KEY_DEFAULT) \
    COREPORT_BIT   (0x0004, "Ball Tilt",     KEYCODE_INSERT) \
    COREPORT_BITTOG(0x0040, "Test",          KEYCODE_7) \
    COREPORT_BIT   (0x4000, "Reset",         KEYCODE_0)

static MACHINE_STOP(VD) {
  cpu_set_nmi_line(0, PULSE_LINE); // NMI routine makes sure the NVRAM is valid!
  run_one_timeslice(); // wait two timeslices before shutdown so the NMI routine can finish
  run_one_timeslice();
}

static MACHINE_DRIVER_START(VD)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, JP_CPUFREQ)
  MDRV_CPU_MEMORY(VD_readmem, VD_writemem)
  MDRV_CPU_PORTS(VD_readport, VD_writeport)
  MDRV_CPU_VBLANK_INT(VD_vblank, 1)
  MDRV_CPU_PERIODIC_INT(VD_irq, JP_IRQFREQ)
  MDRV_CORE_INIT_RESET_STOP(VD,NULL,VD)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(VD)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_DIPS(16)
  MDRV_SOUND_ADD(AY8910, VD_ay8910Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END


// Ator (1985) - using Peyper hardware

// Break (1986)

static const core_tLCDLayout dispVD[] = {
  { 0, 0, 0, 7, CORE_SEG87 },
  { 3, 0, 8, 7, CORE_SEG87 },
  { 6, 0,16, 7, CORE_SEG87 },
  { 9, 0,24, 7, CORE_SEG87 },
  {12, 1,32, 1, CORE_SEG7S}, {12, 4,33, 1, CORE_SEG7S}, {12, 7,34, 1, CORE_SEG7S}, {12,10,35, 2, CORE_SEG7S}, {12,15,37, 1, CORE_SEG7S},
  {0}
};
static core_tGameData breakGameData = {GEN_ZAC1,dispVD,{FLIP_SW(FLIP_L)}};
static void init_break(void) { core_gameData = &breakGameData; }

ROM_START(break)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("break1.cpu", 0x0000, 0x2000, CRC(c187d263) SHA1(1790566799ccc41cd5445936e86f945150e24e8a))
    ROM_LOAD("break2.cpu", 0x2000, 0x2000, CRC(ed8f84ab) SHA1(ff5d7e3c373ca345205e8b92c6ce7b02f36a3d95))
    ROM_LOAD("break3.cpu", 0x4000, 0x2000, CRC(3cdfedc2) SHA1(309fd04c81b8facdf705e6297c0f4d507957ae1f))
ROM_END
VD_INPUT_PORTS_START(break, 1)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0003, 0x0000, "XB/RP#1/RP#2/HSTD")
      COREPORT_DIPSET(0x0000, "800K/1.4M/2M/2.6M")
      COREPORT_DIPSET(0x0001, "1M/1.6M/2.2M/2.8M")
      COREPORT_DIPSET(0x0002, "1.2M/1.8M/2.4M/3M")
      COREPORT_DIPSET(0x0003, "1.4M/2M/2.6M/3.2M")
    COREPORT_DIPNAME( 0x0004, 0x0000, "Balls per game")
      COREPORT_DIPSET(0x0000, "3" )
      COREPORT_DIPSET(0x0004, "5" )
    COREPORT_DIPNAME( 0x0018, 0x0000, "Credits per coin (chute #1/#2)")
      COREPORT_DIPSET(0x0018, "0.5/3" )
      COREPORT_DIPSET(0x0000, "1/5" )
      COREPORT_DIPSET(0x0008, "1/6" )
      COREPORT_DIPSET(0x0010, "2/8" )
    COREPORT_DIPNAME( 0x0020, 0x0000, "Attract tune")
      COREPORT_DIPSET(0x0020, DEF_STR(Off))
      COREPORT_DIPSET(0x0000, DEF_STR(On))
    COREPORT_DIPNAME( 0x0040, 0x0000, "Match feature")
      COREPORT_DIPSET(0x0040, DEF_STR(Off))
      COREPORT_DIPSET(0x0000, DEF_STR(On))
    COREPORT_DIPNAME( 0x0080, 0x0000, "Extra Ball score award")
      COREPORT_DIPSET(0x0080, DEF_STR(Off))
      COREPORT_DIPSET(0x0000, DEF_STR(On))
    COREPORT_DIPNAME( 0x0100, 0x0000, "Accounting #1")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0100, DEF_STR(On))
    COREPORT_DIPNAME( 0x0200, 0x0000, "Accounting #2")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0200, DEF_STR(On))
    COREPORT_DIPNAME( 0x0400, 0x0000, "Accounting #3")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0400, DEF_STR(On))
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0800, DEF_STR(On))
    COREPORT_DIPNAME( 0x3000, 0x0000, "Bumper power")
      COREPORT_DIPSET(0x3000, "4")
      COREPORT_DIPSET(0x2000, "6")
      COREPORT_DIPSET(0x1000, "8")
      COREPORT_DIPSET(0x0000, "10")
    COREPORT_DIPNAME( 0xc000, 0x0000, "Bonus balls")
      COREPORT_DIPSET(0xc000, "6")
      COREPORT_DIPSET(0x8000, "8")
      COREPORT_DIPSET(0x4000, "10")
      COREPORT_DIPSET(0x0000, "12")
INPUT_PORTS_END
CORE_GAMEDEFNV(break, "Break", 1986, "Video Dens", VD, 0)

// Papillon (1986)

static const core_tLCDLayout dispVD2[] = {
  { 0, 0, 0, 7, CORE_SEG87 },
  { 3, 0, 8, 7, CORE_SEG87 },
  { 6, 0,16, 7, CORE_SEG87 },
  { 9, 0,24, 7, CORE_SEG87 },
  {12, 3,33, 2, CORE_SEG7S}, {12, 8,35, 2, CORE_SEG7S}, {12,13,37, 1, CORE_SEG7S},
  {0}
};
static core_tGameData papillonGameData = {GEN_ZAC1,dispVD2,{FLIP_SW(FLIP_L)}};
static void init_papillon(void) { core_gameData = &papillonGameData; }

ROM_START(papillon)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("u4.dat", 0x0000, 0x2000, CRC(e57bfcdd) SHA1(d0d5c798552a2436693dfee0e2ebf4b6f465b194))
    ROM_LOAD("u5.dat", 0x2000, 0x2000, CRC(6d2ef02a) SHA1(0b67b2edd85624531630c162ae31af8078be01e3))
    ROM_LOAD("u6.dat", 0x4000, 0x2000, CRC(6b2867b3) SHA1(720fe8a65b447e839b0eb9ea21e0b3cb0e50cf7a))
ROM_END
VD_INPUT_PORTS_START(papillon, 1)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x0003, 0x0000, "RP#1/RP#2/HSTD")
      COREPORT_DIPSET(0x0000, "1.75M/2.5M/2.6M")
      COREPORT_DIPSET(0x0001, "2M/2.75M/2.8M")
      COREPORT_DIPSET(0x0002, "2.25M/3M/3M")
      COREPORT_DIPSET(0x0003, "2.5M/3.25M/3.2M")
    COREPORT_DIPNAME( 0x0004, 0x0000, "S3")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0004, DEF_STR(On))
    COREPORT_DIPNAME( 0x0008, 0x0000, "Balls per game")
      COREPORT_DIPSET(0x0000, "3" )
      COREPORT_DIPSET(0x0008, "5" )
    COREPORT_DIPNAME( 0x0030, 0x0000, "Credits per coin (chute #1/#2)")
      COREPORT_DIPSET(0x0030, "0.5/3" )
      COREPORT_DIPSET(0x0000, "1/5" )
      COREPORT_DIPSET(0x0010, "1/6" )
      COREPORT_DIPSET(0x0020, "2/8" )
    COREPORT_DIPNAME( 0x0040, 0x0000, "S7")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0040, DEF_STR(On))
    COREPORT_DIPNAME( 0x0080, 0x0000, "S8")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0080, DEF_STR(On))
    COREPORT_DIPNAME( 0x0100, 0x0000, "Accounting #1")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0100, DEF_STR(On))
    COREPORT_DIPNAME( 0x0200, 0x0000, "Accounting #2")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0200, DEF_STR(On))
    COREPORT_DIPNAME( 0x0400, 0x0000, "Accounting #3")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0400, DEF_STR(On))
    COREPORT_DIPNAME( 0x0800, 0x0000, "S12")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0800, DEF_STR(On))
    COREPORT_DIPNAME( 0x3000, 0x0000, "Bumper power")
      COREPORT_DIPSET(0x3000, "4")
      COREPORT_DIPSET(0x2000, "6")
      COREPORT_DIPSET(0x1000, "8")
      COREPORT_DIPSET(0x0000, "10")
    COREPORT_DIPNAME( 0x4000, 0x0000, "S15")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x4000, DEF_STR(On))
    COREPORT_DIPNAME( 0x8000, 0x0000, "S16")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x8000, DEF_STR(On))
INPUT_PORTS_END
CORE_GAMEDEFNV(papillon, "Papillon", 1986, "Video Dens", VD, 0)
