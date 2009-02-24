// Kiss 8035 prototype

#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/8255ppi.h"
#include "core.h"

static struct {
  int swCol, lampCol;
  UINT32 solenoids;
  core_tSeg segments;
  int vblankCount;
  int ramBank;
} locals;

static INTERRUPT_GEN(by8035_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % 1) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % 1) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- display --*/
  if ((locals.vblankCount % 1) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memset(locals.segments, 0, sizeof(locals.segments));
  }

  core_updateSw(TRUE);
}

static INTERRUPT_GEN(by8035_irq) {
  static int irq;
  cpu_set_irq_line(0, 0, (irq = !irq) ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(by8035_ram_r) {
  UINT8 *ram = memory_region(REGION_CPU1) + 0x5000 + 0x100 * locals.ramBank;
  if (locals.ramBank > 7) logerror("%04x: ram %03x read\n", activecpu_get_previouspc(), 0x100 * locals.ramBank + offset);
  if (offset == 2) return 0x55;
//  if (ram_bank == 0x0a) return ppi8255_0_r(offset & 3);
  return ram[offset];
}

static READ_HANDLER(by8035_port_r) {
  logerror("%04x: 8035 port %d read\n", activecpu_get_previouspc(), offset);
  if (offset == 2) return ~coreGlobals.swMatrix[1];
  return 0;
}

static WRITE_HANDLER(by8035_ram_w) {
  UINT8 *ram = memory_region(REGION_CPU1) + 0x5000 + 0x100 * locals.ramBank;
  ram[offset] = data;
  if (locals.ramBank == 0x0b) locals.segments[offset].w = (~data & 0x7f) | ((~data & 0x80) << 1) | ((~data & 0x80) << 2);
  if (locals.ramBank > 7) logerror("%04x: ram %03x write: %02x\n", activecpu_get_previouspc(), 0x100 * locals.ramBank + offset, data);
//  if (locals.ramBank == 0x0a) ppi8255_0_w(offset & 3, data);
}

static WRITE_HANDLER(by8035_port_w) {
  if (offset == 2) cpu_setbank(1, memory_region(REGION_CPU1) + 0x1000 + 0x100 * (data & 0x30));
  if (offset == 2) locals.ramBank = data & 0x0f;
  logerror("%04x: 8035 port %d write = %02x\n", activecpu_get_previouspc(), offset, data);
}

static MEMORY_READ_START(by8035_readmem)
  { 0x0000, 0x0fff, MRA_BANKNO(1) },
  { 0x1000, 0x4fff, MRA_ROM },
  { 0x5000, 0x5fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(by8035_writemem)
  { 0x0000, 0x0fff, MWA_BANKNO(1) },
  { 0x1000, 0x4fff, MWA_ROM },
  { 0x5000, 0x5fff, MWA_RAM },
MEMORY_END

static PORT_READ_START(by8035_readport)
  { 0x00, 0xff, by8035_ram_r },
  { 0x100, 0x107, by8035_port_r },
MEMORY_END

static PORT_WRITE_START(by8035_writeport)
  { 0x00, 0xff, by8035_ram_w },
  { 0x100, 0x107, by8035_port_w },
MEMORY_END

static READ_HANDLER(u8_porta_r) { logerror("%04x: 8255 A read\n", activecpu_get_previouspc()); return 0; }
static READ_HANDLER(u8_portb_r) { logerror("%04x: 8255 B read\n", activecpu_get_previouspc()); return 0; }
static READ_HANDLER(u8_portc_r) { logerror("%04x: 8255 C read\n", activecpu_get_previouspc()); return 0; }
static WRITE_HANDLER(u8_porta_w) { logerror("%04x: 8255 A write: %02x\n", activecpu_get_previouspc(), data); }
static WRITE_HANDLER(u8_portb_w) { logerror("%04x: 8255 B write: %02x\n", activecpu_get_previouspc(), data); }
static WRITE_HANDLER(u8_portc_w) { logerror("%04x: 8255 C write: %02x\n", activecpu_get_previouspc(), data); }

static ppi8255_interface kiss8355_intf =
{
  1,            /* 1 chip */
  {u8_porta_r}, /* Port A read */
  {u8_portb_r}, /* Port B read */
  {u8_portc_r}, /* Port C read */
  {u8_porta_w}, /* Port A write */
  {u8_portb_w}, /* Port B write */
  {u8_portc_w}, /* Port C write */
};

struct AY8910interface kiss_ay8910Int = {
  1,                  /* 1 chip */
  2000000,            /* 2 MHz */
  { 30 },             /* Volume */
};

static MACHINE_INIT(by8035) {
  memset(&locals, 0, sizeof(locals));
  ppi8255_init(&kiss8355_intf);
}

MACHINE_DRIVER_START(by8035)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", I8035, 6000000/15)
  MDRV_CPU_MEMORY(by8035_readmem, by8035_writemem)
  MDRV_CPU_PORTS(by8035_readport, by8035_writeport)
  MDRV_CORE_INIT_RESET_STOP(by8035,NULL,NULL)
  MDRV_CPU_VBLANK_INT(by8035_vblank, 1)
  MDRV_CPU_PERIODIC_INT(by8035_irq, 1000)
  MDRV_SOUND_ADD(AY8910, kiss_ay8910Int)
MACHINE_DRIVER_END

static const core_tLCDLayout dispBy6p[] = {
  {0, 0, 0,16,CORE_SEG10},
  {2, 0,16,16,CORE_SEG10},
  {4, 0,32,16,CORE_SEG10},
  {6, 0,48,16,CORE_SEG10},
  {0}
};

#define INITGAMEP0(name, flip, sb) \
static core_tGameData name##GameData = {GEN_BYPROTO,dispBy6p,{flip,0,7,0,sb}}; \
static void init_##name(void) { core_gameData = &name##GameData; } \
INPUT_PORTS_START(name) \
  CORE_PORTS \
  SIM_PORTS(1) \
INPUT_PORTS_END

#define BY8035_ROMSTART(name, n1, chk1, n2, chk2, n3, chk3, n4, chk4) \
ROM_START(name) \
  NORMALREGION(0x10000, REGION_CPU1) \
    ROM_LOAD( n1, 0x4000, 0x0800, chk1) \
      ROM_RELOAD( 0x0000, 0x0800) \
    ROM_LOAD( n2, 0x1000, 0x1000, chk2) \
    ROM_LOAD( n3, 0x2000, 0x1000, chk3) \
    ROM_LOAD( n4, 0x3000, 0x0800, chk4) \
      ROM_RELOAD( 0x4800, 0x0800)

INITGAMEP0(kissp,FLIP_SW(FLIP_L),0)
BY8035_ROMSTART(kissp,"kiss8755.bin",CRC(894c1052) SHA1(579ce3c8ec374f2cd17928ab92311f035ecee341),
                      "kissprot.u5", CRC(38a2ef5a) SHA1(4ffdb2e9aa30417d506af3bc4b6835ba1dc80e4f),
                      "kissprot.u6", CRC(bcdfaf1d) SHA1(d21bebbf702b400eb71f8c88be50a180a5ac260a),
                      "kissprot.u7", CRC(d97da1d3) SHA1(da771a08969a12105c7adc9f9e3cbd1677971e79))
ROM_END
CORE_CLONEDEFNV(kissp,kiss,"Kiss (prototype)",1979,"Bally",by8035,GAME_NOT_WORKING)
