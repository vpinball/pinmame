/******************************************************************************************
  MAC / CICPlay (Spain)
*******************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "cpu/z80/z80.h"

static struct {
  int    i8279cmd;
  int    i8279reg;
  UINT8  i8279ram[16];
  UINT8  i8279data;
  int strobe;
  int cycle;
  int isCic;
  UINT16 msmData;
  UINT16 msmOffset;
  int    msmToggle;
} locals;

static INTERRUPT_GEN(mac_vblank) {
  if (locals.isCic)
    core_updateSw(core_getSol(9));
  else
    core_updateSw(core_getSol(9) && core_getSol(15));
}

static void mac_nmi(int data) {
  if (!(coreGlobals.swMatrix[0] & 0x40))
    cpu_set_nmi_line(0, PULSE_LINE);
}

static MACHINE_RESET(MAC) {
  memset(&locals, 0x00, sizeof(locals));
}

static SWITCH_UPDATE(MAC) {
  int i;
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT] >> 8, 0xc0, 0);
    CORE_SETKEYSW(inports[CORE_COREINPORT], locals.isCic ? 0x1f : 0x7f, 1);
  }
  for (i = 1; i < 7; i++) {
    if (coreGlobals.swMatrix[i]) {
      cpu_set_irq_line(0, 0, ASSERT_LINE);
      return;
    }
  }
}

static WRITE_HANDLER(ay8910_0_porta_w)	{
  locals.strobe = data;
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x0ffff) | (((data >> 4) ^ 0x0f) << 16);
}
static WRITE_HANDLER(ay8910_0_portb_w)	{
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xfff00) | (data ^ 0xff);
}

static READ_HANDLER(ay8910_1_portb_r)   { return coreGlobals.swMatrix[0] & 0x80; }
static WRITE_HANDLER(ay8910_1_porta_w)	{
  UINT8 col = locals.strobe & 0x0f;
  if (col == 0x0e)
    coreGlobals.lampMatrix[7] = data;
  else if (col == 0x0f)
    coreGlobals.lampMatrix[0] = data;
  else if (col > 7)
    coreGlobals.lampMatrix[1 + col - 8] = data;
  else if (data)
    printf("s %02x=%02x\n", locals.strobe, data);
}
static WRITE_HANDLER(ay8910_1_portb_w)	{
  if (GET_BIT4) {
    cpu_set_irq_line(0, 0, ASSERT_LINE);
  }
  coreGlobals.solenoids = (coreGlobals.solenoids & 0xff9ff) | ((data & 0x60) << 4);
}

struct AY8910interface MAC_ay8910Int = {
	2,			/* 1 chip */
	1900000,	/* 2 MHz */
	{ 30, 50 },		/* Volume */
	{ 0, 0 },
	{ 0, ay8910_1_portb_r },
	{ ay8910_0_porta_w, ay8910_1_porta_w },
	{ ay8910_0_portb_w, ay8910_1_portb_w },
};
static WRITE_HANDLER(ay8910_0_ctrl_w)   { AY8910Write(0,0,data); }
static WRITE_HANDLER(ay8910_0_data_w)   { AY8910Write(0,1,data); }
static READ_HANDLER (ay8910_0_r)        { return AY8910Read(0); }
static WRITE_HANDLER(ay8910_1_ctrl_w)   { AY8910Write(1,0,data); }
static WRITE_HANDLER(ay8910_1_data_w)   { AY8910Write(1,1,data); }
static READ_HANDLER (ay8910_1_r)        { return AY8910Read(1); }

static void mac_msmIrq(int data) {
  UINT8 mdata = *(memory_region(REGION_USER1) + locals.msmData + locals.msmOffset);
  MSM5205_data_w(0, locals.msmToggle ? mdata & 0x0f : mdata >> 4);
  locals.msmToggle = !locals.msmToggle;
  if (!locals.msmToggle) locals.msmOffset++;
  if (locals.msmOffset > 0x7ff) {
    locals.msmOffset = 0;
    locals.msmToggle = 0;
    MSM5205_reset_w(0, 1);
  }
}

static struct MSM5205interface MAC_msm5205Int = {
  1,					//# of chips
  384000,				//384Khz Clock Frequency
  {mac_msmIrq},		//VCLK Int. Callback
  {MSM5205_S48_4B},	//Sample Mode
  {100}				//Volume
};

static UINT16 mac_bcd2seg(UINT8 data) {
  switch (data & 0x0f) {
    case 0x0a: return 0x79; // E
    case 0x0b: return 0x50; // r
    case 0x0c: return 0x5c; // o
  }
  return locals.isCic ? core_bcd2seg9[data & 0x0f] : core_bcd2seg7[data & 0x0f];
}

// handles the 8279 keyboard / display interface chip
static READ_HANDLER(i8279_r) {
  int row;
//logerror("i8279 r%d (cmd %02x, reg %02x)\n", offset, locals.i8279cmd, locals.i8279reg);
  if ((locals.i8279cmd & 0xe0) == 0x40) {
    row = locals.i8279reg & 0x07;
    if (row < 2) locals.i8279data = core_getDip(row); // read dips
    else locals.i8279data = coreGlobals.swMatrix[row - 1]; // read switches
  } else if ((locals.i8279cmd & 0xe0) == 0x60)
    locals.i8279data = locals.i8279ram[locals.i8279reg]; // read display ram
  else logerror("i8279 r:%02x\n", locals.i8279cmd);
  if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  return locals.i8279data;
}
static WRITE_HANDLER(i8279_w) {
  if (offset) { // command
    locals.i8279cmd = data;
    if ((locals.i8279cmd & 0xe0) == 0x40)
      logerror("I8279 read switches: %x\n", data & 0x07);
    else if ((locals.i8279cmd & 0xe0) == 0x80)
      logerror("I8279 write display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x60)
      logerror("I8279 read display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x20)
      logerror("I8279 scan rate: %02x\n", data & 0x1f);
    else if ((locals.i8279cmd & 0xe0) == 0xa0)
      logerror("I8279 blank: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0xc0) {
      logerror("I8279 clear: %x, %x\n", (data & 0x1c) >> 2, data & 0x03);
      if (data & 0x03) {
        locals.i8279data = 0;
        cpu_set_irq_line(0, 0, CLEAR_LINE);
      }
    } else if ((locals.i8279cmd & 0xe0) == 0xe0) {
      logerror("I8279 end interrupt\n");
      locals.i8279data = 0;
      cpu_set_irq_line(0, 0, CLEAR_LINE);
    } else if ((locals.i8279cmd & 0xe0) == 0)
      logerror("I8279 set modes: display %x, keyboard %x\n", (data >> 3) & 0x03, data & 0x07);
    else printf("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = data & 0x0f; // reset data for auto-increment
  } else { // data
    if ((locals.i8279cmd & 0xe0) == 0x80) { // write display ram
      locals.i8279ram[locals.i8279reg] = data;
      if (locals.cycle < 16) {
        if (!locals.i8279reg) {
          coreGlobals.lampMatrix[8] = data;
          coreGlobals.solenoids = (coreGlobals.solenoids & 0xffeff) | ((~data & 1) << 8);
        } else if (locals.i8279reg == 1) {
          coreGlobals.lampMatrix[9] = data;
          coreGlobals.solenoids = (coreGlobals.solenoids & 0xf0fff) | ((~data & 0x70) << 8);
        }
      } else if (locals.i8279reg < 12) {
        coreGlobals.segments[locals.i8279reg].w = mac_bcd2seg(data);
        coreGlobals.segments[16 + locals.i8279reg].w = mac_bcd2seg(data >> 4);
      } else {
        coreGlobals.segments[locals.i8279reg].w = core_bcd2seg7[data & 0x0f];
        coreGlobals.segments[16 + locals.i8279reg].w = core_bcd2seg7[data >> 4];
      }
      locals.cycle = (locals.cycle + 1) % 32;
    } else printf("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  }
}

static WRITE_HANDLER(msm_w) {
  logerror("SND: %x:%02x\n", offset, data);
  if (offset && !(data & 0x80)) {
    locals.msmData = (data & 0x0f) << 11;
    locals.msmOffset = 0;
    locals.msmToggle = 0;
    MSM5205_reset_w(0, 0);
  }
}

static MEMORY_READ_START(mac_readmem)
  {0x0000, 0x7fff, MRA_ROM},
  {0xc000, 0xc7ff, MRA_RAM},
MEMORY_END

static MEMORY_WRITE_START(mac_writemem)
  {0x0000, 0x7fff, MWA_NOP},
  {0xc000, 0xc7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static PORT_READ_START(mac_readport)
  {0x09,0x09, ay8910_0_r},
  {0x29,0x29, ay8910_1_r},
  {0x40,0x40, i8279_r},
PORT_END

static PORT_WRITE_START(mac_writeport)
  {0x08,0x08, ay8910_0_ctrl_w},
  {0x0a,0x0a, ay8910_0_data_w},
  {0x28,0x28, ay8910_1_ctrl_w},
  {0x2a,0x2a, ay8910_1_data_w},
  {0x40,0x41, i8279_w},
  {0x60,0x61, msm_w},
PORT_END

static MACHINE_DRIVER_START(mac)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 3800000)
  MDRV_CPU_MEMORY(mac_readmem, mac_writemem)
  MDRV_CPU_PORTS(mac_readport, mac_writeport)
  MDRV_CPU_VBLANK_INT(mac_vblank, 1)
  MDRV_TIMER_ADD(mac_nmi, 120)
  MDRV_CORE_INIT_RESET_STOP(NULL, MAC, NULL)
  MDRV_DIPS(16)
  MDRV_SWITCH_UPDATE(MAC)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SOUND_ADD(AY8910, MAC_ay8910Int)
MACHINE_DRIVER_END

static MEMORY_READ_START(mac0_readmem)
  {0x0000, 0x3fff, MRA_ROM},
  {0x4000, 0x47ff, MRA_RAM},
  {0x6000, 0x6000, i8279_r},
MEMORY_END

static MEMORY_WRITE_START(mac0_writemem)
  {0x0000, 0x3fff, MWA_NOP},
  {0x4000, 0x47ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x6000, 0x6001, i8279_w},
MEMORY_END

static PORT_READ_START(mac0_readport)
  {0x09,0x09, ay8910_0_r},
  {0x19,0x19, ay8910_1_r},
PORT_END

static PORT_WRITE_START(mac0_writeport)
  {0x08,0x08, ay8910_0_ctrl_w},
  {0x0a,0x0a, ay8910_0_data_w},
  {0x18,0x18, ay8910_1_ctrl_w},
  {0x1a,0x1a, ay8910_1_data_w},
PORT_END

MACHINE_DRIVER_START(mac0)
  MDRV_IMPORT_FROM(mac)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(mac0_readmem, mac0_writemem)
  MDRV_CPU_PORTS(mac0_readport, mac0_writeport)
MACHINE_DRIVER_END

static MACHINE_RESET(MACMSM) {
  memset(&locals, 0x00, sizeof(locals));
  MSM5205_reset_w(0, 1);
}

MACHINE_DRIVER_START(macmsm)
  MDRV_IMPORT_FROM(mac)
  MDRV_CORE_INIT_RESET_STOP(NULL, MACMSM, NULL)
  MDRV_SOUND_ADD(MSM5205, MAC_msm5205Int)
MACHINE_DRIVER_END

#define INITGAME(name, disp, flip, lamps) \
static core_tGameData name##GameData = {0,disp,{flip,0,lamps}}; \
static void init_##name(void) { core_gameData = &name##GameData; }

#define MAC_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0001, "Start",     KEYCODE_1) \
    COREPORT_BITIMP(0x0002, "Coin 1",    KEYCODE_3) \
    COREPORT_BITIMP(0x0004, "Coin 2",    KEYCODE_5) \
    COREPORT_BIT   (0x0020, "Coin 3",    KEYCODE_9) \
    COREPORT_BIT   (0x0008, "Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT   (0x0010, "Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT   (0x0040, "Reset RAM", KEYCODE_0) \
    COREPORT_BIT   (0x4000, "SW1",       KEYCODE_DEL) \
    COREPORT_BITTOG(0x8000, "40V Line",  KEYCODE_END) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "Balls per Game") \
      COREPORT_DIPSET(0x0001, "3" ) \
      COREPORT_DIPSET(0x0000, "5" ) \
    COREPORT_DIPNAME( 0x0006, 0x0006, "Credits f. sm. / big Coins") \
      COREPORT_DIPSET(0x0000, "1/2 / 3" ) \
      COREPORT_DIPSET(0x0002, "4/6 / 4" ) \
      COREPORT_DIPSET(0x0006, "5/4 / 5" ) \
      COREPORT_DIPSET(0x0004, "3/2 / 6" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "3-Ball Multiball") \
      COREPORT_DIPSET(0x0008, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0010, 0x0000, DEF_STR(Unused)) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0010, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0020, 0x0020, "In-Game Music") \
      COREPORT_DIPSET(0x0000, DEF_STR(No) ) \
      COREPORT_DIPSET(0x0020, DEF_STR(Yes) ) \
    COREPORT_DIPNAME( 0x0040, 0x0040, "Attract Sound") \
      COREPORT_DIPSET(0x0000, DEF_STR(No) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(Yes) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Unused, always off") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
    COREPORT_DIPNAME( 0x0f00, 0x0600, "Ball/RP1/2/HSTD") \
      COREPORT_DIPSET(0x0000, "300K/450K/630K/1M" ) \
      COREPORT_DIPSET(0x0100, "325K/500K/690K/1.1M" ) \
      COREPORT_DIPSET(0x0200, "350K/570K/740K/1.15M" ) \
      COREPORT_DIPSET(0x0300, "375K/610K/790K/1.25M" ) \
      COREPORT_DIPSET(0x0400, "400K/650K/850K/1.35M" ) \
      COREPORT_DIPSET(0x0500, "425K/690K/900K/1.4M" ) \
      COREPORT_DIPSET(0x0600, "450K/730K/950K/1.5M" ) \
      COREPORT_DIPSET(0x0700, "500K/810K/1.05M/1.65M" ) \
      COREPORT_DIPSET(0x0800, "550K/890K/1.15M/1.8M" ) \
      COREPORT_DIPSET(0x0900, "600K/950K/1.25M/2M" ) \
      COREPORT_DIPSET(0x0a00, "650K/1.05M/1.35M/2.15M" ) \
      COREPORT_DIPSET(0x0b00, "700K/1.15M/1.5M/2.3M" ) \
      COREPORT_DIPSET(0x0c00, "750K/1.25M/1.6M/2.5M" ) \
      COREPORT_DIPSET(0x0d00, "800K/1.3M/1.7M/2.7M" ) \
      COREPORT_DIPSET(0x0e00, "900K/1.45M/1.9M/3M" ) \
      COREPORT_DIPSET(0x0f00, "1M/1.65M/2.15M/3.3M" ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, "Auto-adjust") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x2000, "Multiple Extra Balls") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x4000, "Multiple Replays") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x4000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Unused, always off") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
  INPUT_PORTS_END

static core_tLCDLayout dispMAC[] = {
  {0, 0, 0,6,CORE_SEG8}, {0,16,16,6,CORE_SEG8},
  {3, 0, 6,6,CORE_SEG8}, {3,16,22,6,CORE_SEG8},
  {6, 0,15,1,CORE_SEG8}, {6, 4,14,1,CORE_SEG8}, {6, 8,13,1,CORE_SEG8}, {6,10,12,1,CORE_SEG8},
  {6,16,28,4,CORE_SEG8}, {6,24,31,1,CORE_SEG8}, {6,26,31,1,CORE_SEG8},
  {6, 2,32,1,CORE_SEG8}, {6, 6,32,1,CORE_SEG8},
  {0}
};

ROM_START(macgalxy)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("galaxy1.bin", 0x0000, 0x2000, CRC(00c71e67) SHA1(c1ad1dacae2b90f516c732bfdf8244908f67e15a))
    ROM_LOAD("galaxy2.bin", 0x2000, 0x2000, CRC(f0efb723) SHA1(697b3c9f3ebedca1087354eda5dfe9719d497045))
ROM_END
INITGAME(macgalxy,dispMAC,FLIP_SW(FLIP_L),2)
INPUT_PORTS_START(macgalxy)
CORE_PORTS
SIM_PORTS(1)
PORT_START /* 0 */
  COREPORT_BIT   (0x0001, "Start",     KEYCODE_1)
  COREPORT_BITIMP(0x0002, "Coin 1",    KEYCODE_3)
  COREPORT_BITIMP(0x0004, "Coin 2",    KEYCODE_5)
  COREPORT_BIT   (0x0020, "Coin 3",    KEYCODE_9)
  COREPORT_BIT   (0x0008, "Tilt",      KEYCODE_INSERT)
  COREPORT_BIT   (0x0010, "Slam Tilt", KEYCODE_HOME)
  COREPORT_BIT   (0x0040, "Reset RAM", KEYCODE_0)
  COREPORT_BIT   (0x4000, "SW1",       KEYCODE_DEL)
  COREPORT_BITTOG(0x8000, "40V Line",  KEYCODE_END)
PORT_START /* 1 */
  COREPORT_DIPNAME( 0x0001, 0x0001, "Balls per Game")
    COREPORT_DIPSET(0x0001, "3" )
    COREPORT_DIPSET(0x0000, "5" )
  COREPORT_DIPNAME( 0x0006, 0x0006, "Credits f. sm. / big Coins")
    COREPORT_DIPSET(0x0000, "1/2 / 3" )
    COREPORT_DIPSET(0x0002, "4/6 / 4" )
    COREPORT_DIPSET(0x0006, "5/4 / 5" )
    COREPORT_DIPSET(0x0004, "3/2 / 6" )
  COREPORT_DIPNAME( 0x0008, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0008, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0030, 0x0010, "Replay 1/2 thresholds")
    COREPORT_DIPSET(0x0000, "500K/650K" )
    COREPORT_DIPSET(0x0020, "500K/750K" )
    COREPORT_DIPSET(0x0010, "550K/750K" )
    COREPORT_DIPSET(0x0030, "600K/800K" )
  COREPORT_DIPNAME( 0x0040, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0040, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0080, 0x0000, "Unused, always off")
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
  COREPORT_DIPNAME( 0x0300, 0x0000, "Extra Ball threshold")
    COREPORT_DIPSET(0x0000, "350K" )
    COREPORT_DIPSET(0x0200, "400K" )
    COREPORT_DIPSET(0x0100, "450K" )
    COREPORT_DIPSET(0x0300, "-" )
  COREPORT_DIPNAME( 0x0400, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0400, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0800, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0800, DEF_STR(On) )
  COREPORT_DIPNAME( 0x1000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x1000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x2000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x2000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x4000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x4000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x8000, 0x0000, "Unused, always off")
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
INPUT_PORTS_END
CORE_GAMEDEFNV(macgalxy, "MAC's Galaxy", 1986, "MAC S.A.", mac0, 0)

ROM_START(macjungl)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("jungle1.bin", 0x0000, 0x2000, CRC(461a3e1b) SHA1(96981b4d8db0412c474169eaf5e5386be5006ffe))
    ROM_LOAD("jungle2.bin", 0x2000, 0x2000, CRC(26b53e6e) SHA1(e588787b2381c0e6a42590f0e7d18d2a74ebf5f0))
ROM_END
INITGAME(macjungl,dispMAC,FLIP_SW(FLIP_L),2)
INPUT_PORTS_START(macjungl)
CORE_PORTS
SIM_PORTS(1)
PORT_START /* 0 */
  COREPORT_BIT   (0x0001, "Start",     KEYCODE_1)
  COREPORT_BITIMP(0x0002, "Coin 1",    KEYCODE_3)
  COREPORT_BITIMP(0x0004, "Coin 2",    KEYCODE_5)
  COREPORT_BIT   (0x0020, "Coin 3",    KEYCODE_9)
  COREPORT_BIT   (0x0008, "Tilt",      KEYCODE_INSERT)
  COREPORT_BIT   (0x0010, "Slam Tilt", KEYCODE_HOME)
  COREPORT_BIT   (0x0040, "Reset RAM", KEYCODE_0)
  COREPORT_BIT   (0x4000, "SW1",       KEYCODE_DEL)
  COREPORT_BITTOG(0x8000, "40V Line",  KEYCODE_END)
PORT_START /* 1 */
  COREPORT_DIPNAME( 0x0001, 0x0001, "Balls per Game")
    COREPORT_DIPSET(0x0001, "3" )
    COREPORT_DIPSET(0x0000, "5" )
  COREPORT_DIPNAME( 0x0006, 0x0006, "Credits f. sm. / big Coins")
    COREPORT_DIPSET(0x0006, "5/4 / 5" )
    COREPORT_DIPSET(0x0004, "3/2 / 6" )
    COREPORT_DIPSET(0x0002, "9/4 / 9" )
    COREPORT_DIPSET(0x0000, "5/2 / 10" )
  COREPORT_DIPNAME( 0x0008, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0008, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0030, 0x0010, "Replay 1/2 thresholds")
    COREPORT_DIPSET(0x0000, "500K/650K" )
    COREPORT_DIPSET(0x0020, "500K/750K" )
    COREPORT_DIPSET(0x0010, "550K/750K" )
    COREPORT_DIPSET(0x0030, "600K/800K" )
  COREPORT_DIPNAME( 0x0040, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0040, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0080, 0x0000, "Unused, always off")
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
  COREPORT_DIPNAME( 0x0300, 0x0000, "Extra Ball threshold")
    COREPORT_DIPSET(0x0000, "350K" )
    COREPORT_DIPSET(0x0200, "400K" )
    COREPORT_DIPSET(0x0100, "450K" )
    COREPORT_DIPSET(0x0300, "-" )
  COREPORT_DIPNAME( 0x0400, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0400, DEF_STR(On) )
  COREPORT_DIPNAME( 0x0800, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x0800, DEF_STR(On) )
  COREPORT_DIPNAME( 0x1000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x1000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x2000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x2000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x4000, 0x0000, DEF_STR(Unknown) )
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
    COREPORT_DIPSET(0x4000, DEF_STR(On) )
  COREPORT_DIPNAME( 0x8000, 0x0000, "Unused, always off")
    COREPORT_DIPSET(0x0000, DEF_STR(Off) )
INPUT_PORTS_END
CORE_GAMEDEFNV(macjungl, "MAC Jungle", 1986, "MAC S.A.", mac0, 0)

ROM_START(spctrain)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("mbm27128.25", 0x0000, 0x4000, CRC(d65c5c36) SHA1(6f350b48daaecd36b3086e682ec6ee174f297a34))
ROM_END
INITGAME(spctrain,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(spctrain, 1)
CORE_GAMEDEFNV(spctrain, "Space Train", 1987, "MAC S.A.", mac, 0)

ROM_START(spctrai0)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("1717-c2.bin", 0x0000, 0x2000, CRC(ca8be787) SHA1(4091921013429c6104da698625391c575e30b8e1))
    ROM_LOAD("1717-c8.bin", 0x2000, 0x2000, CRC(c7f499f5) SHA1(6564cab0c70fb66a95b24c05b427239b4b886f1e))
ROM_END
INITGAME(spctrai0,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(spctrai0, 1)
CORE_CLONEDEFNV(spctrai0, spctrain, "Space Train (old hardware)", 1987, "MAC S.A.", mac0, 0)

ROM_START(spcpnthr)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("sp_game.bin", 0x0000, 0x8000, CRC(0428563c) SHA1(45b9daf12f8384101450f1e529491812f73d88bd))
  NORMALREGION(0x10000, REGION_USER1)
    ROM_LOAD("mac_snd.bin", 0x0000, 0x8000, CRC(d7aedbac) SHA1(4b59028e08b2d7ff8f19596022ba6e830cf736e2))
ROM_END
INITGAME(spcpnthr,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(spcpnthr, 1)
CORE_GAMEDEFNV(spcpnthr, "Space Panther", 1988, "MAC S.A.", macmsm, 0)

ROM_START(mac_1808)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("mac_1808.cpu", 0x0000, 0x8000, CRC(29126585) SHA1(b24c4f0f17f3ef7de5348cb06ec3b305c6ca7373))
  NORMALREGION(0x10000, REGION_USER1)
    ROM_LOAD("mac_snd.bin", 0x0000, 0x8000, CRC(d7aedbac) SHA1(4b59028e08b2d7ff8f19596022ba6e830cf736e2))
ROM_END
INITGAME(mac_1808,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(mac_1808, 1)
CORE_GAMEDEFNV(mac_1808, "Unknown Game (MAC #1808)", 19??, "MAC S.A.", macmsm, 0)

ROM_START(macjungn)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("juego1.bin", 0x0000, 0x8000, CRC(0b2d9b64) SHA1(22602b79c8b178793b447783bca59dcb49e4525f))
  NORMALREGION(0x10000, REGION_USER1)
    ROM_LOAD("mac_snd.bin", 0x0000, 0x8000, CRC(d7aedbac) SHA1(4b59028e08b2d7ff8f19596022ba6e830cf736e2))
ROM_END
INITGAME(macjungn,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(macjungn, 1)
CORE_GAMEDEFNV(macjungn, "MAC Jungle (New version)", 1995, "MAC S.A.", macmsm, 0)

ROM_START(nbamac)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("nba_mac.cpu", 0x0000, 0x8000, CRC(0c430988) SHA1(71126d9caf10ac27056b8bf28d300775062dc693))
  NORMALREGION(0x10000, REGION_USER1)
    ROM_LOAD("mac_snd.bin", 0x0000, 0x8000, CRC(d7aedbac) SHA1(4b59028e08b2d7ff8f19596022ba6e830cf736e2))
ROM_END
INITGAME(nbamac,dispMAC,FLIP_SW(FLIP_L),2)
MAC_COMPORTS(nbamac, 1)
CORE_GAMEDEFNV(nbamac, "NBA MAC", 1996, "MAC S.A.", macmsm, 0)

// CICPlay changes & games below

static MACHINE_RESET(CIC) {
  memset(&locals, 0x00, sizeof(locals));
  locals.isCic = 1;
  coreGlobals.segments[32].w = core_bcd2seg9[0];
}

static WRITE_HANDLER(i8279_w_cic) {
  if (offset) { // command
    locals.i8279cmd = data;
    if ((locals.i8279cmd & 0xe0) == 0x40)
      logerror("I8279 read switches: %x\n", data & 0x07);
    else if ((locals.i8279cmd & 0xe0) == 0x80)
      logerror("I8279 write display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x60)
      logerror("I8279 read display: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0x20)
      logerror("I8279 scan rate: %02x\n", data & 0x1f);
    else if ((locals.i8279cmd & 0xe0) == 0xa0)
      logerror("I8279 blank: %x\n", data & 0x0f);
    else if ((locals.i8279cmd & 0xe0) == 0xc0) {
      logerror("I8279 clear: %x, %x\n", (data & 0x1c) >> 2, data & 0x03);
      if (data & 0x03) {
        locals.i8279data = 0;
        cpu_set_irq_line(0, 0, CLEAR_LINE);
      }
    } else if ((locals.i8279cmd & 0xe0) == 0xe0) {
      logerror("I8279 end interrupt\n");
      locals.i8279data = 0;
      cpu_set_irq_line(0, 0, CLEAR_LINE);
    } else if ((locals.i8279cmd & 0xe0) == 0)
      logerror("I8279 set modes: display %x, keyboard %x\n", (data >> 3) & 0x03, data & 0x07);
    else printf("i8279 w%d:%02x\n", offset, data);
    locals.i8279reg = data & 0x0f; // set the data register even if the reset flag was not set!?
  } else { // data
    if ((locals.i8279cmd & 0xe0) == 0x80) { // write display ram
      locals.i8279ram[locals.i8279reg] = data;
      if (locals.i8279reg == 6 || locals.i8279reg > 12) {
        coreGlobals.segments[16 + locals.i8279reg].w = core_bcd2seg7[data & 0x0f];
        coreGlobals.segments[locals.i8279reg].w = core_bcd2seg7[data >> 4];
      } else {
        coreGlobals.segments[16 + locals.i8279reg].w = mac_bcd2seg(data);
        coreGlobals.segments[locals.i8279reg].w = mac_bcd2seg(data >> 4);
        if (locals.i8279reg == 0 || locals.i8279reg == 3 || locals.i8279reg == 7 || locals.i8279reg == 10) {
          if (coreGlobals.segments[locals.i8279reg].w) coreGlobals.segments[locals.i8279reg].w |= 0x80;
          if (coreGlobals.segments[16 + locals.i8279reg].w) coreGlobals.segments[16 + locals.i8279reg].w |= 0x80;
        }
      }
      if (locals.i8279reg == 6) {
        if ((data & 0x0f) > 7) {
          coreGlobals.lampMatrix[8] = 0;
          coreGlobals.lampMatrix[9] = (1 << ((data & 0x0f) - 8)) & 0x7f;
        } else {
          coreGlobals.lampMatrix[8] = 1 << (data & 0x0f);
          coreGlobals.lampMatrix[9] = 0;
        }
      } else if (locals.i8279reg == 15) {
        coreGlobals.lampMatrix[10] = (coreGlobals.lampMatrix[10] & 0xf0) | (data & 0x0f);
      } else if (locals.i8279reg == 14) {
        coreGlobals.lampMatrix[10] = (coreGlobals.lampMatrix[10] & 0x0f) | ((data & 0x0f) << 4);
        coreGlobals.solenoids = (coreGlobals.solenoids & 0xffeff) | (data & 0x03 ? 0 : 0x100);
      } else if (locals.i8279reg == 13) {
        coreGlobals.solenoids = (coreGlobals.solenoids & 0xf0fff) | ((data & 0x0f) << 12);
      }
    } else printf("i8279 w%d:%02x\n", offset, data);
    if (locals.i8279cmd & 0x10) locals.i8279reg = (locals.i8279reg+1) % 16; // auto-increase if register is set
  }
}

static MEMORY_WRITE_START(cic_writemem)
  {0x0000, 0x3fff, MWA_NOP},
  {0x4000, 0x47ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
  {0x6000, 0x6001, i8279_w_cic},
MEMORY_END

MACHINE_DRIVER_START(cic)
  MDRV_IMPORT_FROM(mac)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CPU_MEMORY(mac0_readmem, cic_writemem)
  MDRV_CPU_PORTS(mac0_readport, mac0_writeport)
  MDRV_CORE_INIT_RESET_STOP(NULL, CIC, NULL)
MACHINE_DRIVER_END

#define CIC_COMPORTS(game, balls) \
  INPUT_PORTS_START(game) \
  CORE_PORTS \
  SIM_PORTS(balls) \
  PORT_START /* 0 */ \
    COREPORT_BIT   (0x0001, "Start",     KEYCODE_1) \
    COREPORT_BITIMP(0x0002, "Coin 1",    KEYCODE_3) \
    COREPORT_BITIMP(0x0004, "Coin 2",    KEYCODE_5) \
    COREPORT_BIT   (0x0010, "Tilt",      KEYCODE_INSERT) \
    COREPORT_BIT   (0x0008, "Slam Tilt", KEYCODE_HOME) \
    COREPORT_BIT   (0x4000, "SW1",       KEYCODE_DEL) \
    COREPORT_BITTOG(0x8000, "40V Line",  KEYCODE_END) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "Balls per Game") \
      COREPORT_DIPSET(0x0001, "3" ) \
      COREPORT_DIPSET(0x0000, "5" ) \
    COREPORT_DIPNAME( 0x0006, 0x0006, "Credits f. sm. / big Coins") \
      COREPORT_DIPSET(0x0006, "5/4 / 5" ) \
      COREPORT_DIPSET(0x0004, "3/2 / 6" ) \
      COREPORT_DIPSET(0x0002, "9/4 / 9" ) \
      COREPORT_DIPSET(0x0000, "5/2 / 10" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "Arrows reward") \
      COREPORT_DIPSET(0x0000, "30K" ) \
      COREPORT_DIPSET(0x0008, "50K" ) \
    COREPORT_DIPNAME( 0x0030, 0x0010, "Replay 1/2 thresholds") \
      COREPORT_DIPSET(0x0000, "500K/650K" ) \
      COREPORT_DIPSET(0x0020, "500K/750K" ) \
      COREPORT_DIPSET(0x0010, "550K/750K" ) \
      COREPORT_DIPSET(0x0030, "600K/800K" ) \
    COREPORT_DIPNAME( 0x0040, 0x0000, DEF_STR(Unknown) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0040, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0080, 0x0000, "Unused, always off") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
    COREPORT_DIPNAME( 0x0300, 0x0000, "Extra Ball threshold") \
      COREPORT_DIPSET(0x0000, "350K" ) \
      COREPORT_DIPSET(0x0200, "400K" ) \
      COREPORT_DIPSET(0x0100, "450K" ) \
      COREPORT_DIPSET(0x0300, "-" ) \
    COREPORT_DIPNAME( 0x0400, 0x0000, DEF_STR(Unknown) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0400, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x0800, 0x0000, DEF_STR(Unknown) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x0800, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x1000, 0x0000, DEF_STR(Unknown) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x1000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x2000, 0x0000, DEF_STR(Unknown) ) \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x2000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x4000, 0x0000, "Reset RAM") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
      COREPORT_DIPSET(0x4000, DEF_STR(On) ) \
    COREPORT_DIPNAME( 0x8000, 0x0000, "Unused, always off") \
      COREPORT_DIPSET(0x0000, DEF_STR(Off) ) \
  INPUT_PORTS_END

static core_tLCDLayout dispCIC[] = {
  {0, 0, 0,1,CORE_SEG10},{0, 2, 1,2,CORE_SEG9}, {0, 6, 3,1,CORE_SEG10},{0, 8, 4,2,CORE_SEG9}, {0,12,32,1,CORE_SEG9},
  {0,16,16,1,CORE_SEG10},{0,18,17,2,CORE_SEG9}, {0,22,19,1,CORE_SEG10},{0,24,20,2,CORE_SEG9}, {0,28,32,1,CORE_SEG9},
  {3, 0, 7,1,CORE_SEG10},{3, 2, 8,2,CORE_SEG9}, {3, 6,10,1,CORE_SEG10},{3, 8,11,2,CORE_SEG9}, {3,12,32,1,CORE_SEG9},
  {3,16,23,1,CORE_SEG10},{3,18,24,2,CORE_SEG9}, {3,22,26,1,CORE_SEG10},{3,24,27,2,CORE_SEG9}, {3,28,32,1,CORE_SEG9},
  {6, 0,14,2,CORE_SEG7S},{6, 6, 6,1,CORE_SEG7S},{6,10,13,1,CORE_SEG7S},
  {0}
};

ROM_START(glxplay)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("1083-1.cpu", 0x0000, 0x2000, CRC(3df33169) SHA1(657720aab4cccf3364f013acb3f5dbc46fe0e05c))
    ROM_LOAD("1083-2.cpu", 0x2000, 0x2000, CRC(47b4f49e) SHA1(59853ac56bb9e2dc7b848dc46ebd27c21b9d2e82))
ROM_END
INITGAME(glxplay,dispCIC,FLIP_SW(FLIP_L),3)
CIC_COMPORTS(glxplay, 1)
CORE_GAMEDEFNV(glxplay, "Galaxy Play", 1985, "CICPlay", cic, 0)

ROM_START(kidnap)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("kidnap_1.bin", 0x0000, 0x2000, CRC(4b8f9bb1) SHA1(16672c1a5e55ba5963fbd8834443dbead9bdff10) BAD_DUMP)
    ROM_LOAD("kidnap_2.bin", 0x2000, 0x2000, CRC(4333d9ba) SHA1(362bcc9caaf37ad7efc116c3bee9b99cbbfa0563))
ROM_END
INITGAME(kidnap,dispCIC,FLIP_SW(FLIP_L),3)
CIC_COMPORTS(kidnap, 1)
CORE_GAMEDEFNV(kidnap, "Kidnap", 1986, "CICPlay", cic, 0)

ROM_START(glxplay2)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("1382-1.cpu", 0x0000, 0x2000, CRC(da43b0b9) SHA1(b13b260c61b3bd0b7632aabcdbcf4cdd5cbe4b22))
    ROM_LOAD("1382-2.cpu", 0x2000, 0x2000, CRC(945c90fd) SHA1(8367992f8db8b402d82e4a3f02a35b796756ce0f))
ROM_END
INITGAME(glxplay2,dispCIC,FLIP_SW(FLIP_L),3)
CIC_COMPORTS(glxplay2, 1)
CORE_GAMEDEFNV(glxplay2, "Galaxy Play 2", 1987, "CICPlay", cic, 0)
