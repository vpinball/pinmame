/************************************************************************************************
 Barni Pinball
 -----------------

   Hardware:
		CPU: 2 x M6809, optional M6802 (what for?)
			 INT: IRQ from VIA on CPU 0, FIRQ on CPU 1
		IO: 2x PIA 6821 for switches / dips
		    1x VIA 6522 for displays / solenoids / lamps / sound data
		DISPLAY: 4x7 digit 7-segment displays
		SOUND:	 basically the same as Bally's Squalk & Talk -61 board but missing AY8912 synth chip

************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "core.h"
#include "sndbrd.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "machine/6522via.h"
#include "sound/tms5220.h"

/*----------------
/  Local variables
/-----------------*/
static struct {
  UINT8 strobe1;
  UINT8 strobe2;
  UINT8 via_a;
  int bitCount;
} locals;

/*-------------------------------
/  copy local data to interface
/--------------------------------*/
static INTERRUPT_GEN(barni_vblank) {
  core_updateSw(core_getSol(17));
}

static SWITCH_UPDATE(barni) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0xf0, 9);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x06, 8);
    CORE_SETKEYSW(inports[CORE_COREINPORT], 0x01, 0);
  }
  sndbrd_0_diag(coreGlobals.swMatrix[0] & 1);
#ifdef MAME_DEBUG
  if (keyboard_pressed(KEYCODE_N)) {
    cpu_set_nmi_line(0, PULSE_LINE);
  }
#endif
}

static WRITE_HANDLER(gram_w) {
  memory_region(REGION_CPU1)[offset] = memory_region(REGION_CPU2)[offset] = data;
}

static READ_HANDLER(gram_r) {
  return memory_region(REGION_CPU1)[offset];
}

static WRITE_HANDLER(pal_w) {
  // this is probably wrong but I didn't find any other output to control game enable
  coreGlobals.solenoids = (coreGlobals.solenoids & 0x0ffff) | (data & 0x40 ? 0x10000 : 0);
}

static READ_HANDLER(firq_set) {
  cpu_set_irq_line(1, M6809_FIRQ_LINE, ASSERT_LINE);
  return 0;
}

static READ_HANDLER(firq_clr) {
  cpu_set_irq_line(1, M6809_FIRQ_LINE, CLEAR_LINE);
  return 0;
}

static READ_HANDLER(cswd_r) {
  // watchdog, periodically resets zc circuit which would hang the machine by triggering the NMI line
  return 0;
}

static WRITE_HANDLER(pia0a_w) {
  locals.strobe1 = core_BitColToNum(data);
}

static WRITE_HANDLER(pia1a_w) {
  locals.strobe2 = core_BitColToNum(data & 0x0f);
}

static READ_HANDLER(pia0b_r) {
  return coreGlobals.swMatrix[1 + locals.strobe1];
}

static READ_HANDLER(pia1a_r) {
  logerror("---PIA1 A R\n");
  return 0;
}

static READ_HANDLER(pia1b_r) {
  static UINT8 lastSw9;
  UINT8 retVal;
  UINT8 sw9 = coreGlobals.swMatrix[9] & 0xf0;
  UINT8 newSw9 = sw9;
  UINT8 dips = core_getDip(locals.strobe2 / 2);
  // HACK that will close both coin switches at the same time so the credits don't keep adding up!
  // Unfortunately this means both coins are amounted to the credits for now.
  if (~lastSw9 & sw9 & 0x30) {
    newSw9 |= 0x30;
  }
  retVal = (newSw9 ^ 0x30) | (locals.strobe2 % 2 ? dips >> 4 : dips & 0x0f);
  lastSw9 = sw9;
  return retVal;
}

static WRITE_HANDLER(via0a_w) {
  locals.via_a = data;
  locals.bitCount = 0;
}

static void showSegment(int num, UINT8 data) {
  int seg = 32 + num / 6;
  UINT8 bit = 1 << (num % 6);
  coreGlobals.segments[num].w = data & 0x7f;
  if (data & 0x80) {
    coreGlobals.segments[seg].w |= bit;
  } else {
    coreGlobals.segments[seg].w &= 0x3f ^ bit;
  }
}

static int countBits(UINT8 data) {
  int cnt = 0;
  int i;
  for (i = 0; i < 8; i++) {
    if (data & (1 << i)) {
      cnt++;
    }
  }
  return cnt;
}

static WRITE_HANDLER(via0b_w) {
  static UINT8 lampData, lampRow, col4[3];
  static int colNum, segNum;
  switch (locals.via_a >> 4) {
    case 0:
      sndbrd_0_data_w(0, ~data);
      break;
    case 1:
      if (!(locals.bitCount % 8)) {
        segNum = 31 - (locals.bitCount ? 0 : 1) - 2 * (locals.via_a & 0x0f);
        showSegment(segNum, data);
      }
      locals.bitCount++;
      break;
    case 5:
      switch (~locals.via_a & 0x0f) {
        case 0:
          if (core_gameData->hw.lampCol) {
            // champion uses two solenoid outputs to control 12 extra lamps
            coreGlobals.solenoids = (coreGlobals.solenoids & 0x100ff) | ((~data & 0x9f) << 8);
            if (~data & 0x20) coreGlobals.lampMatrix[8] = lampRow;
            if (~data & 0x40) coreGlobals.lampMatrix[9] = lampRow;
          } else {
            coreGlobals.solenoids = (coreGlobals.solenoids & 0x100ff) | ((~data & 0xff) << 8);
          }
          break;
        case 1:
          coreGlobals.solenoids = (coreGlobals.solenoids & 0x1ff00) | (~data & 0xff);
          break;
        case 2:
          lampRow = ~data;
          if (countBits(lampRow) == 1) {
            colNum = core_BitColToNum(lampRow);
            coreGlobals.lampMatrix[colNum] = lampData;
            // column 4 is additionally fed with all 0 on champion, so buffer the previous value a while
            if (core_gameData->hw.lampCol && colNum == 4) {
              col4[0] = col4[1];
              col4[1] = col4[2];
              col4[2] = lampData;
              coreGlobals.lampMatrix[4] |= col4[0] | col4[1];
            }
          }
          break;
        case 3:
          lampData = ~data;
          break;
        default:
          logerror("VIA A/B: %02x/%02x\n", locals.via_a, data);
      }
      break;
    case 7:
      if (core_getDip(2) && !data) {
        showSegment(segNum-1, 0);
        showSegment(segNum, 0);
      }
      break;
    default:
      logerror("VIA A/B: %02x/%02x\n", locals.via_a, data);
  }
}

static WRITE_HANDLER(via0ca2_w) {
  // we can do without this signal
}

static WRITE_HANDLER(via0cb2_w) {
  // we can do without this signal
}

static void via_irq(int state) {
  cpu_set_irq_line(0, M6809_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct pia6821_interface pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ 0, pia0b_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ pia0a_w, 0, 0, 0,
  /*irq: A/B           */ 0, 0
},{
  /*i: A/B,CA/B1,CA/B2 */ pia1a_r, pia1b_r, PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ pia1a_w, 0, 0, 0,
  /*irq: A/B           */ 0, 0
}};

static struct via6522_interface via = {
  /*i: A/B,CA1/B1,CA2/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA2/B2        */ via0a_w, via0b_w, via0ca2_w, via0cb2_w, 
  /*irq:                 */ via_irq
};

static MEMORY_READ_START(barni_readmem1)
  {0x0000,0x03ff, gram_r},
  {0x4000,0x4000, firq_set},
  {0x6000,0x6000, cswd_r},
  {0x8000,0x800f, via_0_r},
  {0xa000,0xa7ff, MRA_RAM},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(barni_writemem1)
  {0x0000,0x03ff, gram_w},
  {0x2000,0x2000, pal_w},
  {0x8000,0x800f, via_0_w},
  {0xa000,0xa7ff, MWA_RAM, &generic_nvram, &generic_nvram_size},
MEMORY_END

static MEMORY_READ_START(barni_readmem2)
  {0x0000,0x03ff, gram_r},
  {0x2000,0x2000, firq_clr},
  {0x8000,0x8003, pia_r(0)},
  {0xa000,0xa003, pia_r(1)},
  {0xc000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(barni_writemem2)
  {0x0000,0x03ff, gram_w},
  {0x8000,0x8003, pia_w(0)},
  {0xa000,0xa003, pia_w(1)},
MEMORY_END

// sound section

static struct {
  int pia0a, pia0b, pia1a, pia1b, pia1cb1, pia1ca2;
  UINT8 lastcmd;
} sndlocals;

static WRITE_HANDLER(snd_pia0b_w) {
  logerror("snd PIA B: %02x\n", data);
}
static READ_HANDLER(snd_pia1a_r) { return sndlocals.pia1a; }
static WRITE_HANDLER(snd_pia1a_w) { sndlocals.pia1a = data; }
static WRITE_HANDLER(snd_pia1b_w) {
  if (sndlocals.pia1b & ~data & 0x01) { // read, overrides write command!
    sndlocals.pia1a = tms5220_status_r(0);
  } else if (sndlocals.pia1b & ~data & 0x02) { // write
    tms5220_data_w(0, sndlocals.pia1a);
  }
  sndlocals.pia1b = data;
  pia_set_input_ca2(3, tms5220_ready_r());
}
static READ_HANDLER(snd_pia1ca2_r) {
  return sndlocals.pia1ca2;
}
static READ_HANDLER(snd_pia1cb1_r) {
  return sndlocals.pia1cb1;
}

static void snd_irq(int state) {
  cpu_set_irq_line(2, 0, state ? ASSERT_LINE : CLEAR_LINE);
}

static const struct pia6821_interface snd_pia[] = {{
  /*i: A/B,CA/B1,CA/B2 */ PIA_UNUSED_VAL(0xff), PIA_UNUSED_VAL(0xff), PIA_UNUSED_VAL(1), PIA_UNUSED_VAL(1), 0, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ 0, snd_pia0b_w, 0, 0,
  /*irq: A/B           */ snd_irq, snd_irq
},{
  /*i: A/B,CA/B1,CA/B2 */ snd_pia1a_r, 0, PIA_UNUSED_VAL(1), snd_pia1cb1_r, snd_pia1ca2_r, PIA_UNUSED_VAL(0),
  /*o: A/B,CA/B2       */ snd_pia1a_w, snd_pia1b_w, 0, 0,
  /*irq: A/B           */ snd_irq, snd_irq
}};

static void snd_init(struct sndbrdData *brdData) {
  pia_config(2, PIA_STANDARD_ORDERING, &snd_pia[0]);
  pia_config(3, PIA_STANDARD_ORDERING, &snd_pia[1]);
  tms5220_reset();
  tms5220_set_variant(TMS5220_IS_5220C); // schematics say TMS5200 but 5220C is used - verified with real game
}
static void snd_diag(int button) {
  cpu_set_nmi_line(2, button ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(snd_data_w) {
  sndlocals.lastcmd = data;
  pia_set_input_cb1(2, 1); pia_set_input_cb1(2, 0);
}
const struct sndbrdIntf barniIntf = {
  "BARNI", snd_init, NULL, snd_diag, snd_data_w, snd_data_w, NULL, NULL, NULL, 0
};
static READ_HANDLER(snd_cmd_r) {
  return sndlocals.lastcmd;
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x0083, pia_r(2) },
  { 0x0090, 0x0093, pia_r(3) },
  { 0x2000, 0x2000, snd_cmd_r },
  { 0x1000, 0x1000, MRA_NOP },
  { 0xc000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0080, 0x0083, pia_w(2) },
  { 0x0090, 0x0093, pia_w(3) },
  { 0x1000, 0x1000, DAC_0_data_w },
MEMORY_END

static void snd_5220Irq(int state) { pia_set_input_cb1(3, (sndlocals.pia1cb1 = !state)); }
static void snd_5220Rdy(int state) { pia_set_input_ca2(3, (sndlocals.pia1ca2 = state)); }

static struct TMS5220interface snd_tms5220Int = { 640000, 75, snd_5220Irq, snd_5220Rdy };
static struct DACinterface     snd_dacInt = { 1, { 20 }};

// driver and games

static MACHINE_INIT(barni) {
  memset(&locals, 0, sizeof(locals));
  pia_config(0, PIA_STANDARD_ORDERING, &pia[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &pia[1]);
  via_config(0, &via);
  pia_reset();
  via_reset();
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(REGION_CPU3), NULL, NULL);
}

static MACHINE_STOP(barni) {
  sndbrd_0_exit();
}

MACHINE_DRIVER_START(barni)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", M6809, 1000000)
  MDRV_CPU_MEMORY(barni_readmem1, barni_writemem1)
  MDRV_CPU_ADD_TAG("hcpu", M6809, 1000000)
  MDRV_CPU_MEMORY(barni_readmem2, barni_writemem2)
  MDRV_CPU_VBLANK_INT(barni_vblank, 1)
  MDRV_CORE_INIT_RESET_STOP(barni,NULL,barni)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(17)
  MDRV_SWITCH_UPDATE(barni)

  MDRV_CPU_ADD_TAG("scpu", M6802, 3579545/4)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_INTERLEAVE(500)
  MDRV_SOUND_ADD(TMS5220, snd_tms5220Int)
  MDRV_SOUND_ADD(DAC,     snd_dacInt)
MACHINE_DRIVER_END

/*--------------------------------
/ Red Baron
/-------------------------------*/
ROM_START(redbaron)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("redbaron.r1", 0xe000, 0x2000, CRC(fd401d3f) SHA1(33c0f178c798e16a9b4489a0e469f0a227882e79))
    ROM_LOAD("redbaron.r2", 0xc000, 0x2000, CRC(0506e53e) SHA1(a1eaa39181cb3e5a1c281d217d680a42e39c856a))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("redbaron.r3", 0xe000, 0x2000, CRC(45bca0b8) SHA1(77e2d6b04ea8d6fa7e30b59232696b9aa5307286))
  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("rbsnd1.732", 0xf000, 0x1000, CRC(674389ce) SHA1(595bbfe51dc3af266f4344e3865c0e48dd96acea))
    ROM_LOAD("rbsnd2.732", 0xe000, 0x1000, CRC(30e7da5e) SHA1(3054cf9b09e0f89c242e1ad35bb31d9bd77248e4))
    ROM_LOAD("rbsnd3.732", 0xd000, 0x1000, CRC(a4ba0f72) SHA1(e46148a2f5125914944973f37e73a62001c76aaa))
    ROM_LOAD("rbsnd4.732", 0xc000, 0x1000, CRC(fd8db899) SHA1(0978213f14f73ccc4a24eb42a39db00d9299c5d0))
ROM_END

static core_tLCDLayout dispAlpha[] = {
  {0, 0, 0,6,CORE_SEG7},{0,12,32,1,CORE_SEG7}, {0,26,18,6,CORE_SEG7},{0,38,35,1,CORE_SEG7},
  {3, 0, 6,6,CORE_SEG7},{3,12,33,1,CORE_SEG7}, {3,26,24,6,CORE_SEG7},{3,38,36,1,CORE_SEG7},
  {2,20,12,2,CORE_SEG7S},{2,25,14,2,CORE_SEG7S},{2,30,16,2,CORE_SEG7S},
#ifdef MAME_DEBUG
  {4,25,30,2,CORE_SEG7S},
#endif
  {0}
};
static core_tGameData redbaronGameData = {0,dispAlpha,{FLIP_SWNO(0,0),0,0,0,SNDBRD_BARNI}};
static void init_redbaron(void) {
  core_gameData = &redbaronGameData;
}

INPUT_PORTS_START(barni)
  CORE_PORTS
  SIM_PORTS(1)
  PORT_START /* 0 */
    COREPORT_BIT(   0x0040, "Partida",	KEYCODE_1)
    COREPORT_BIT(   0x0010, "25 Ptas",	KEYCODE_3)
    COREPORT_BIT(   0x0020, "100 Ptas",	KEYCODE_5)
    COREPORT_BIT(   0x0080, "Tilt",	KEYCODE_DEL)
    COREPORT_BIT(   0x0002, "Test +",	KEYCODE_7)
    COREPORT_BIT(   0x0004, "Test -",	KEYCODE_8)
    COREPORT_BIT(   0x0001, "Sound test",	KEYCODE_0)
  PORT_START /* 1 */
    COREPORT_DIPNAME( 0x2000, 0x0000, "  800.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x2000, DEF_STR(On))
    COREPORT_DIPNAME( 0x0200, 0x0200, "1.000.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0200, DEF_STR(On))
    COREPORT_DIPNAME( 0x0020, 0x0000, "1.200.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0020, DEF_STR(On))
    COREPORT_DIPNAME( 0x0002, 0x0000, "1.400.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0002, DEF_STR(On))
    COREPORT_DIPNAME( 0x0001, 0x0000, "1.800.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0001, DEF_STR(On))
    COREPORT_DIPNAME( 0x0010, 0x0010, "2.200.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0010, DEF_STR(On))
    COREPORT_DIPNAME( 0x0100, 0x0000, "2.600.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0100, DEF_STR(On))
    COREPORT_DIPNAME( 0x1000, 0x1000, "3.000.000 puntos partida")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x1000, DEF_STR(On))
    COREPORT_DIPNAME( 0x8484, 0x0080, "Monedero")
      COREPORT_DIPSET(0x8000, "0,5/25 Ptas, 3/100 Ptas" )
      COREPORT_DIPSET(0x0004, "  1/25 Ptas, 4/100 Ptas" )
      COREPORT_DIPSET(0x0080, "  1/25 Ptas, 5/100 Ptas" )
      COREPORT_DIPSET(0x0400, "  2/25 Ptas, 8/100 Ptas" )
    COREPORT_DIPNAME( 0x0008, 0x0008, "Musica Fondo")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0008, DEF_STR(On))
    COREPORT_DIPNAME( 0x0040, 0x0000, "Loteria")
      COREPORT_DIPSET(0x0000, "Mas")
      COREPORT_DIPSET(0x0040, "Menos")
    COREPORT_DIPNAME( 0x0800, 0x0000, "Bolas")
      COREPORT_DIPSET(0x0000, "3")
      COREPORT_DIPSET(0x0800, "5")
    COREPORT_DIPNAME( 0x4000, 0x4000, "Voz Presentacion")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x4000, DEF_STR(On))
  PORT_START /* 2 */
    COREPORT_DIPNAME( 0x0001, 0x0000, "Display blanking")
      COREPORT_DIPSET(0x0000, DEF_STR(Off))
      COREPORT_DIPSET(0x0001, DEF_STR(On))
INPUT_PORTS_END

#define input_ports_redbaron input_ports_barni
CORE_GAMEDEFNV(redbaron, "Red Baron", 1985, "Barni", barni, 0)

/*--------------------------------
/ Champion
/-------------------------------*/
ROM_START(champion)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("che.bin", 0xe000, 0x2000, CRC(c5dc9228) SHA1(5306980a9c73118cfb843dbce0d56f516d054220))
    ROM_LOAD("chc.bin", 0xc000, 0x2000, CRC(6ab0f232) SHA1(0638d33f86c62ee93dff924a16a5b9309392d9e8))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("chan.bin", 0xe000, 0x2000, CRC(3f148587) SHA1(e44dc9cce15830f522dc781aaa13c659a43371f3))
  NORMALREGION(0x10000, REGION_CPU3)
    ROM_LOAD("voz1.bin", 0xf000, 0x1000, CRC(48665778) SHA1(c295dfe7f4a98756f508391eb326f37a5aac37ff))
    ROM_LOAD("voz2.bin", 0xe000, 0x1000, CRC(30e7da5e) SHA1(3054cf9b09e0f89c242e1ad35bb31d9bd77248e4))
    ROM_LOAD("voz3.bin", 0xd000, 0x1000, CRC(3cd8058e) SHA1(fa4fd0cf4124263d4021c5a86033af9e5aa66eed))
    ROM_LOAD("voz4.bin", 0xc000, 0x1000, CRC(0d00d8cc) SHA1(10f64d2fc3fc3e276bbd0e108815a3b395dcf0c9))
ROM_END

static core_tGameData championGameData = {0,dispAlpha,{FLIP_SWNO(0,0),0,2,0,SNDBRD_BARNI}};
static void init_champion(void) {
  core_gameData = &championGameData;
}

#define input_ports_champion input_ports_barni
CORE_GAMEDEFNV(champion, "Champion", 1985, "Barni", barni, 0)
