#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "core.h"
#include "sndbrd.h"
#include "taito.h"
#include "taitos.h"

// Taito Pinball System
// cpu: 8080 @ 1.888888 MHz (17 MHz / 9)
//
// Switch matrix: (D0-D7)*10 + V0-V7
// Lamp matrix:   (ST0-ST15)*10 + L3-L0
// Solenoids:     1-6 (first column), 7-12 (second column)
//                17: mux relay, 18: game play relay
//			      7-12 are automatically activated by sol 17
//				  flip sols as usual: right 45-46, left 47-48, active if sol 18 is on
//
// Many thanks to Alexandre Souza and Newton Pessoa

#define TAITO_IRQFREQ        0.53 // NE555 astable, C=3.3u, R1=820k, R2=2.2k, tOn=1880ms, tOff=5ms

#define TAITO_SOLSMOOTH      2 // Smooth the sols over this number of VBLANKS
#define TAITO_DISPLAYSMOOTH  2 // Smooth the display over this number of VBLANKS
#define TAITO_LAMPSMOOTH	 2 // Smooth the lamps over this number of VBLANKS

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT8 lampMatrix[CORE_MAXLAMPCOL];
  int sndCmd, oldsndCmd;

  void* timer_irq;
  UINT8* pDisplayRAM;
  UINT8* pCommandsDMA;
} TAITOlocals;

static NVRAM_HANDLER(taito);
static NVRAM_HANDLER(taito_old);

static int segMap[] = {
	4,0,-4,4,0,-4,4,0,-4,4,0,-4,0,0
};

static INTERRUPT_GEN(taito_vblank) {
	//-------------------------------
	//  copy local data to interface
	//-------------------------------
	TAITOlocals.vblankCount += 1;

	// -- solenoids --
	if ((TAITOlocals.vblankCount % TAITO_SOLSMOOTH) == 0) {
		coreGlobals.solenoids = TAITOlocals.solenoids;
	}

	// -- lamps --
 	if ((TAITOlocals.vblankCount % TAITO_LAMPSMOOTH) == 0) {
 		memcpy(coreGlobals.lampMatrix, TAITOlocals.lampMatrix, sizeof(coreGlobals.lampMatrix));
 	}

    // -- display --
	if ((TAITOlocals.vblankCount % TAITO_DISPLAYSMOOTH) == 0) {
		memcpy(coreGlobals.segments, TAITOlocals.segments, sizeof coreGlobals.segments);
	}

	// sol 18 is the play relay
	core_updateSw(core_getSol(18));
}

static SWITCH_UPDATE(taito) {
	if (inports) {
    CORE_SETKEYSW(inports[TAITO_COMINPORT]>>8, 0x80, 0);
    CORE_SETKEYSW(inports[TAITO_COMINPORT],    0xff, 1);
    CORE_SETKEYSW(inports[TAITO_COMINPORT]>>8, 0x1f, 8);
    if (inports[TAITO_COMINPORT] & 0x8000) sndbrd_0_diag(1);
	}
}

static INTERRUPT_GEN(taito_irq) {
	// logerror("irq:\n");
	cpu_set_irq_line(TAITO_CPU, 0, CLEAR_LINE);
	cpu_set_irq_line(TAITO_CPU, 0, HOLD_LINE);
}

static void timer_irq(int data) { taito_irq(); }

static MACHINE_INIT(taito) {
	memset(&TAITOlocals, 0, sizeof(TAITOlocals));

	TAITOlocals.pDisplayRAM  = memory_region(TAITO_MEMREG_CPU) + 0x4080;
	TAITOlocals.pCommandsDMA = memory_region(TAITO_MEMREG_CPU) + 0x4090;

	TAITOlocals.timer_irq = timer_alloc(timer_irq);
	timer_adjust(TAITOlocals.timer_irq, TIME_IN_HZ(TAITO_IRQFREQ), 0, TIME_IN_HZ(TAITO_IRQFREQ));
	if (core_gameData->hw.soundBoard)
		sndbrd_0_init(core_gameData->hw.soundBoard, TAITO_SCPU, memory_region(TAITO_MEMREG_SCPU), NULL, NULL);

	TAITOlocals.vblankCount = 1;
}

static MACHINE_INIT(taito_old) {
	memset(&TAITOlocals, 0, sizeof(TAITOlocals));

	TAITOlocals.pDisplayRAM  = memory_region(TAITO_MEMREG_CPU) + 0x1000;
	TAITOlocals.pCommandsDMA = memory_region(TAITO_MEMREG_CPU) + 0x1010;

	TAITOlocals.timer_irq = timer_alloc(timer_irq);
	timer_adjust(TAITOlocals.timer_irq, TIME_IN_HZ(5.293/4.0), 0, TIME_IN_HZ(5.293/4.0));
	if (core_gameData->hw.soundBoard)
		sndbrd_0_init(core_gameData->hw.soundBoard, TAITO_SCPU, memory_region(TAITO_MEMREG_SCPU), NULL, NULL);

	TAITOlocals.vblankCount = 1;
}

static MACHINE_STOP(taito) {
	if ( TAITOlocals.timer_irq ) {
		timer_remove(TAITOlocals.timer_irq);
		TAITOlocals.timer_irq = NULL;
	}
	if (core_gameData->hw.soundBoard)
		sndbrd_0_exit();
}

static WRITE_HANDLER(taito_sndCmd_w) {
	// logerror("sound cmd: 0x%02x\n", data);
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

	if (core_gameData->hw.soundBoard)
		sndbrd_0_data_w(0, data);
}

static READ_HANDLER(switches_r) {
	if ( offset==6 )
		return core_getDip(0)^0xff;
	if (offset == 8) offset = 6;
	return coreGlobals.swMatrix[offset+1]^0xff;
}

static WRITE_HANDLER(switches_w) {
	logerror("switch write: %i %i\n", offset, data);
}

// display
static WRITE_HANDLER(dma_display)
{
	TAITOlocals.pDisplayRAM[offset] = data;

	if ( offset<12 ) {
		// player 1-4, 6 digits per player
		TAITOlocals.segments[2*offset+segMap[offset]].w   = core_bcd2seg7e[(data>>4)&0x0f];
		TAITOlocals.segments[2*offset+segMap[offset]+1].w = core_bcd2seg7e[data&0x0f];
	}
	else {
		switch ( offset ) {
		case 12:
			// balls in play
			TAITOlocals.segments[2*12+segMap[12]].w = core_bcd2seg7e[data&0x0f];
			break;

		case 13:
			// credits
			TAITOlocals.segments[2*12+segMap[12]+1].w = core_bcd2seg7e[data&0x0f];
			break;

		case 14:
			// match
			TAITOlocals.segments[2*13+segMap[13]].w = core_bcd2seg7e[data&0x0f];
			break;

		case 15:
			// active player
			TAITOlocals.segments[2*13+segMap[13]+1].w = core_bcd2seg7e[data&0x0f];
			break;
		}
	}
}

// sols, sound and lamps
static WRITE_HANDLER(dma_commands)
{
	// upper nibbles of offset 0-1: solenoids
	TAITOlocals.pCommandsDMA[offset] = data;

	switch ( offset ) {
	case 0:
		// upper nibble: - solenoids 17-18 (mux relay and play relay)
		//				 - solenoids 5-6 or 11-12 depending on mux relay
		TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xfffcffff) | ((data&0xc0)<<10);
		if ( TAITOlocals.solenoids&0x10000 )
			// 11-12
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xfffff3ff) | ((data&0x30)<<6);
		else
			// 5-6
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffffffcf) | (data&0x30);
		break;

	case 1:
		// upper nibble: solenoids 1-4 or 7-10 depending on mux relay
		if ( TAITOlocals.solenoids&0x10000 )
			// 7-10
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xfffffc3f) | ((data&0xf0)<<2);
		else
			// 1-4
			TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xfffffff0) | ((data&0xf0)>>4);
		break;

	case 2:
		// upper nibble: sound command bits 1-4, sound enable
		TAITOlocals.sndCmd = (TAITOlocals.sndCmd & 0xf0) | (((data>>4)^core_getDip(1)) & 0x0f);
		if ( TAITOlocals.oldsndCmd!=TAITOlocals.sndCmd ) {
			TAITOlocals.oldsndCmd = TAITOlocals.sndCmd;
			taito_sndCmd_w(0, TAITOlocals.sndCmd);
		}
		break;

	case 3:
		// upper nibble: sound command bits 5-8, solenoids 13-14
		TAITOlocals.sndCmd = (TAITOlocals.sndCmd & 0x0f) | ((data ^ core_getDip(1)) & 0xf0);
		TAITOlocals.solenoids = (TAITOlocals.solenoids & 0xffffcfff) | ((data & 0xc0) << 6);
		break;

	}

	// lower nibbles: lamps, offset 0-f
	// Taito uses 16 rows and 4 cols, rows 9-16 are mapped to row 1-8, cols 5-8

	{
		int col = (offset<8)?0:4;
		int rowBit  = (1 << (offset%8));
		int rowMask = rowBit^0xff;
		int i = 0;

		for (i=0;i<4;i++) {
			TAITOlocals.lampMatrix[col] = (TAITOlocals.lampMatrix[col]&rowMask) | ((data&0x08)?rowBit:0);
			col++;
			data <<= 1;
		}
	}
}

// strobe (0-15)*10 + L3-L0
// example: 123 is strobe 12, L3 is the first lamp in row number 12
//
static int TAITO_lamp2m(int no) {
	if ( (no/10)<8 )
		return (4-(no%10))*8 + (no/10);
	else
		return (8-(no%10))*8 + ((no/10)-8);
}

static int TAITO_m2lamp(int col, int row) {
	if ( col<4 )
		return (row*10) + (3-col);
	else
		return ((row+8)*10) + (7-col);
}

static int TAITO_sw2m(int no) {
	return ((no%10)+1)*8 + (no/10);
}

static int TAITO_m2sw(int col, int row) {
	return (row*10) + (col-1);
}

static MEMORY_READ_START(taito_readmem)
  { 0x0000, 0x27ff, MRA_ROM },
  { 0x2800, 0x2808, switches_r }, /* some games use different locations */
  { 0x2880, 0x2887, switches_r }, /* -"- */
  { 0x28d8, 0x28df, switches_r }, /* -"- */
  { 0x3e00, 0x3fff, MRA_RAM },
  { 0x4000, 0x40ff, MRA_RAM },
  { 0x4800, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(taito_writemem)
  { 0x0000, 0x3dff, MWA_NOP },
  { 0x3e00, 0x3fff, MWA_RAM },
  { 0x4000, 0x407f, MWA_RAM },
  { 0x4080, 0x408f, dma_display },
  { 0x4090, 0x409f, dma_commands },
  { 0x40a0, 0x40ff, MWA_RAM },
MEMORY_END

static MEMORY_READ_START(taito_readmem_old)
  { 0x0000, 0x0fff, MRA_ROM },
  { 0x1000, 0x10ff, MRA_RAM },
  { 0x1400, 0x1408, switches_r },
  { 0x14d8, 0x14df, switches_r },
  { 0x1800, 0x1bff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(taito_writemem_old)
  { 0x0000, 0x0fff, MWA_NOP },
  { 0x1000, 0x100f, dma_display },
  { 0x1010, 0x101f, dma_commands },
  { 0x1020, 0x10ff, MWA_RAM },
  { 0x1400, 0x1408, switches_w },
MEMORY_END

MACHINE_DRIVER_START(taito)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(taito,NULL,taito)
  MDRV_CPU_ADD_TAG("mcpu", 8080, 17000000/9)
  MDRV_CPU_MEMORY(taito_readmem, taito_writemem)
  MDRV_CPU_VBLANK_INT(taito_vblank, 1)
  MDRV_NVRAM_HANDLER(taito)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(taito)
  MDRV_SWITCH_CONV(TAITO_sw2m,TAITO_m2sw)
  MDRV_LAMP_CONV(TAITO_lamp2m,TAITO_m2lamp)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintetizador)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintetizador)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintetizadorpp)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintetizadorpp)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintevox)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintevox)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_sintevoxpp)
  MDRV_IMPORT_FROM(taito)

  MDRV_IMPORT_FROM(taitos_sintevoxpp)
  MDRV_SOUND_CMD(taito_sndCmd_w)
  MDRV_SOUND_CMDHEADING("taito")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taito_old)
  MDRV_IMPORT_FROM(taito_sintetizador)

  MDRV_CORE_INIT_RESET_STOP(taito_old,NULL,taito)
  MDRV_CPU_REPLACE("mcpu", 8080, 19000000/9)
  MDRV_CPU_MEMORY(taito_readmem_old, taito_writemem_old)
  MDRV_NVRAM_HANDLER(taito_old)
MACHINE_DRIVER_END

//-----------------------------------------------
// Load/Save static ram
//-----------------------------------------------
static NVRAM_HANDLER(taito) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x4000, 0x100, 0x00);
}

static NVRAM_HANDLER(taito_old) {
  core_nvram(file, read_or_write, memory_region(TAITO_MEMREG_CPU)+0x1000, 0x100, 0x00);
}



//-----------------------------------------------
// Taito Z80 section (only used on same Mr. Black machines)
//
// Lots of guess work here due to a lack of manual and schematics.
// Sound is just as bad as in the regular Mr. Black game,
// so I assume the sound roms might be bad (or different ones).
//-----------------------------------------------

static struct {
  int vblankCount;
  int swCol;
  int solNo;
  UINT32 solenoids;
} z80locals;

static WRITE_HANDLER(col_w) {
  z80locals.solNo = data >> 4;
  z80locals.swCol = data & 0x0f;
}

static READ_HANDLER(sw_r) { // 16 switch colums with 4 bits each
  if (z80locals.swCol % 2)
    return ~(coreGlobals.swMatrix[1+(z80locals.swCol / 2)] >> 4);
  else
    return ~(coreGlobals.swMatrix[1+(z80locals.swCol / 2)] & 0x0f);
}

static WRITE_HANDLER(disp_w) { // there's room for 5 player displays, but only 4 are used.
  coreGlobals.segments[31-2*z80locals.swCol].w = core_bcd2seg7e[data&0x0f];
  coreGlobals.segments[30-2*z80locals.swCol].w = core_bcd2seg7e[data >> 4];
}

static WRITE_HANDLER(lamp_w) {
  coreGlobals.lampMatrix[z80locals.swCol] = data;
}

static WRITE_HANDLER(sol_w) {
  coreGlobals.lampMatrix[16] = data;
  if (z80locals.solNo) // how to activate sols 1 and 17 without turning them on all the time???
    z80locals.solenoids |= 1 << (z80locals.solNo + ((data & 0x80) >> 3));
  z80locals.solenoids |= (data & 0x10) << 12; // mapping game on sol to 17
}

static WRITE_HANDLER(p12_w) {
  logerror("Port 0x1%d write = %02x\n", offset+2, data);
}
static WRITE_HANDLER(p22_w) {
  logerror("Port 0x2%d write = %02x\n", offset+2, data);
}

static MEMORY_READ_START(z80_readmem)
  { 0x0000, 0x1fff, MRA_ROM },
  { 0x4800, 0x4bff, MRA_RAM },
MEMORY_END
static MEMORY_WRITE_START(z80_writemem)
  { 0x4800, 0x4bff, MWA_RAM, &generic_nvram, &generic_nvram_size },
  { 0x5800, 0x5800, col_w },
MEMORY_END

static PORT_READ_START(z80_readport)
  { 0x21, 0x21, sw_r },
PORT_END
static PORT_WRITE_START(z80_writeport)
  { 0x10, 0x10, disp_w },
  { 0x11, 0x11, taito_sndCmd_w },
  { 0x12, 0x13, p12_w },
  { 0x20, 0x20, lamp_w },
  { 0x21, 0x21, sol_w },
  { 0x22, 0x23, p22_w },
PORT_END

static INTERRUPT_GEN(z80_nmi) {
  cpu_set_nmi_line(0, PULSE_LINE);
}

static MACHINE_INIT(taitoz80) {
  memset(&z80locals, 0, sizeof(z80locals));
  sndbrd_0_init(core_gameData->hw.soundBoard, TAITO_SCPU, memory_region(TAITO_MEMREG_SCPU), NULL, NULL);
}
static MACHINE_STOP(taitoz80) {
  sndbrd_0_exit();
}

static INTERRUPT_GEN(taitoz80_vblank) {
  z80locals.vblankCount++;

  coreGlobals.solenoids = z80locals.solenoids;
  if (z80locals.vblankCount % 4 == 0)
    z80locals.solenoids = 0;

  core_updateSw(core_getSol(17));
}

static SWITCH_UPDATE(taitoz80) {
  if (inports) {
    CORE_SETKEYSW(inports[TAITO_COMINPORT],    0x7f, 1);
  }
}

MACHINE_DRIVER_START(taitoz80)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 5000000)
  MDRV_CPU_MEMORY(z80_readmem, z80_writemem)
  MDRV_CPU_PORTS(z80_readport, z80_writeport)
  MDRV_CPU_VBLANK_INT(taitoz80_vblank, 1)
  MDRV_CPU_PERIODIC_INT(z80_nmi, 1000)
  MDRV_CORE_INIT_RESET_STOP(taitoz80,NULL,taitoz80)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_SWITCH_UPDATE(taitoz80)

  MDRV_IMPORT_FROM(taitos_sintetizadorpp)
MACHINE_DRIVER_END

static core_tLCDLayout disp[] = {
  {0, 0,26,6,CORE_SEG7},
  {3, 0,20,6,CORE_SEG7},
  {6, 0,14,6,CORE_SEG7},
  {9, 0, 8,6,CORE_SEG7},
  {13,0, 7,1,CORE_SEG7}, {13,10,6,1,CORE_SEG7},
  {0}
};
static core_tGameData mrblkz80GameData = {0, disp, {FLIP_SW(FLIP_L),0,9,0,SNDBRD_TAITO_SINTETIZADORPP,0}};
static void init_mrblkz80(void) {
  core_gameData = &mrblkz80GameData;
}

TAITO_ROMSTART2222(mrblkz80, "mb01z80.dat", CRC(7f883a70) SHA1(848783123b55ade769cac3c1b3d4a2c759a6c5b6),
                             "mb02z80.dat", CRC(68de8f50) SHA1(7076297060e927da1aefae8bf75c8cda18031660),
                             "mb03z80.dat", CRC(5a8e55e8) SHA1(b93102254004d258998bd6ab7d7b333361b37830),
                             "mb04z80.dat", CRC(ecf30c2f) SHA1(404c891bc420cfe540e829a1cd05ced10ea5a09c))
TAITO_SOUNDROMS444("mrb_s1.bin", CRC(ff28b2b9) SHA1(3106811740e0206ad4ba7845e204e721b0da70e2),
                   "mrb_s2.bin", CRC(34d52449) SHA1(bdd5db5e58ca997d413d18f291928ad1a45c194e),
                   "mrb_s3.bin", CRC(276fb897) SHA1(b1a4323a4d921e3ae4beefaa04cd95e18cc33b9d))
TAITO_ROMEND

INPUT_PORTS_START(mrblkz80)
  CORE_PORTS
  SIM_PORTS(3)
  PORT_START /* 0 */
    /* Switch Column 1 */
    COREPORT_BIT(   0x0001, "Coin",         KEYCODE_5)
    COREPORT_BIT(   0x0002, "Start",        KEYCODE_1)
    COREPORT_BIT(   0x0004, "Slam",         KEYCODE_HOME)
    COREPORT_BIT(   0x0008, "Tilt",         KEYCODE_INSERT)
    COREPORT_BIT(   0x0010, "Test / Reset", KEYCODE_7)
    COREPORT_BITTOG(0x0020, "Coin Door",    KEYCODE_8)
    COREPORT_BIT(   0x0040, "Statistics",   KEYCODE_9)
INPUT_PORTS_END

CORE_CLONEDEFNV(mrblkz80, mrblack, "Mr. Black (Z-80 CPU)", 198?, "Taito", taitoz80, GAME_IMPERFECT_SOUND)
