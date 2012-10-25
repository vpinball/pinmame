/************************************************************************************************
  Tecnoplay (San Marino)
  ----------------------
  by Gerrit Volkenborn & Steve Ellenoff

  Main CPU Board:

  CPU: Motorola M68000
  Clock: 8 Mhz
  Interrupt: Tied to a Fixed System Timer
  I/O: DMA
  Sound: TKY-2016 music chip @NTSC clock (probably a Y8950), DAC for f/x

  Issues/Todo:
  #0) Display is going too fast @ 8Mhz cpu
  #1) Not 100% sure of the IRQ timing, although I think it should be correct from the schematics.
  #2) Seems the display might go too fast in places, ie, doesn't scroll enough sometimes
  #3) In relation to #2 - not sure if calculation of the display column is always 100% correct

  NOTE ON TIMING: The manual claims the "Display Message" will appear every 15 seconds
************************************************************************************************/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms7000/tms7000.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"

#define TECNO_SOLSMOOTH 4
//#define TECNO_CPUFREQ 8000000		//As written in manual
#define TECNO_CPUFREQ 4000000			//Seems to work better

// Crystal from CPU feeds an LS393 - Q1 (acts as divide by 2) - Feeds 4040 which divides by 128 - Feeds 7474 (divide by 2)
#define TECNO_IRQ_FREQ TIME_IN_HZ(TECNO_CPUFREQ/2/128/2)		//Not 100% sure on this one..

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int irq_count;
  int DispNoWait;
  int DispCol;
  int LampCol;
  UINT8 sndCmd;
  int sndAck;
} locals;

/* Each time an IRQ is fired, the Vector # is incremented (since the IRQ generation is via a 4040 timer)

   Bit 7, 3 are always 1, and 5, 6 are always 0!
   Therefore, the sequence is:
   0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f (and repeat)

   The very first time, 0x88 is skipped, since an IRQ is only triggered from a 1->0 transition */
static int tecno_irq_callback(int x)
{
	int vector_num = 0;
	if(locals.irq_count < 0x08)
		vector_num = 0x88+locals.irq_count;
	else
		vector_num = (0x98-0x08)+locals.irq_count;

	//LOG(("servicing irq vector - time #%x - returning %x!\n",locals.irq_count,vector_num));

	locals.irq_count = (locals.irq_count+1) % 0x10;
	return vector_num;
}

//Generate a level 1 IRQ - IP0,IP1,IP2 = 0
static void tecno_irq(int data)
{
  cpu_set_irq_line(0, MC68000_IRQ_1, PULSE_LINE);
}

static INTERRUPT_GEN(vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));

  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % TECNO_SOLSMOOTH) == 0) {
	locals.solenoids = coreGlobals.pulsedSolState;
  }
  core_updateSw(1);
}

static SWITCH_UPDATE(tecno) {
  if (inports) {
	  coreGlobals.swMatrix[1] = (inports[CORE_COREINPORT] & 0x00ff);		//Column 0 Switches
	  coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xfc) | (inports[CORE_COREINPORT] & 0x0300)>>8;     //Column 1 Switches
  }
}

static MACHINE_INIT(tecno) {
  memset(&locals, 0, sizeof(locals));

  //setup 68000 IRQ callback to generate IRQ Vector #
  cpu_set_irq_callback(0, tecno_irq_callback);

  //setup IRQ timer
  timer_pulse(TECNO_IRQ_FREQ, 0, tecno_irq);

  //start count on 1 - because 1st time through 0x89 is the vector done.
  locals.irq_count = 1;

  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(REGION_CPU2), NULL, NULL);
}

//Input Key - Return Switches (uses Lamp Column Strobe)
static READ16_HANDLER(input_key_r) {
	UINT8 switches = coreGlobals.swMatrix[locals.LampCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
	return (UINT16)core_revbyte(switches);	//Reverse bits to align with switch matrix from manual
}

//Return Sound Status?
static READ16_HANDLER(input_sound_r) {
	LOG(("input_sound_r\n"));
	return locals.sndAck ? 0 : 0xffff;
}

//The value here is read, which is tied to the ls74 flip generating the blanking signal.
static READ16_HANDLER(rtrg_r) {
	//LOG(("%08x: rtrg_r\n",activecpu_get_pc()));
	return 0xffff;
}

//This value is read but unused according to schematics.
static READ16_HANDLER(rtrg2_r) {
	//LOG(("%08x: rtrg2_r\n",activecpu_get_pc()));
	return 0;
}

//Lamp Rows (actually columns) 1-8
static WRITE16_HANDLER(lamp1_w) { locals.LampCol = core_BitColToNum(data >> 8); }
//Lamp Cols (actually rows) 1-8
static WRITE16_HANDLER(lamp2_w) { 	coreGlobals.tmpLampMatrix[locals.LampCol] = data>>8; }

//Solenoids 1-16
static WRITE16_HANDLER(sol1_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF0000) | data; }
//Solenoids 17-32
static WRITE16_HANDLER(sol2_w) { coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x0000FFFF) | (data<<16); }


//*****************************
//D0-D7   = Sound Data 0-7
//D8      = Strobe
//D9      = Reset
//D10     = Display Data Clock
//D11-D15 = AUX 1-5
//******************************
static WRITE16_HANDLER(sound_w) {
	int dclk = (data & 0x400) >> 10;

	//LOG(("sound_w = %04x\n",data));
	sndbrd_data_w(0, data & 0xff);

	//LOG(("%08x: dclk_w = %04x\n",activecpu_get_pc(),dclk));

	//Increment Display Column on if not waiting for transition!
	if(locals.DispNoWait) {
//		locals.DispCol = (locals.DispCol + 1) % 16;
		locals.DispCol++;
	}
	if(dclk) {
		locals.DispNoWait = 1;
		locals.DispCol = 0;
	}
	//LOG(("DispCol = %x\n",locals.DispCol));
}

/*********************************************************************************************
	Tecno Layout ( this was guessed by me since not shown in schematics, but seems correct )


     a1   a2
    ---- ----
   |\   |   /|
 f |h\ j| k/ | b
   |  \ | /  |
    ---- ----
   |q / |\ m |
 e | / p| \  | c
   |/r  | n\ |
    ---- ----
     d2   d1


This is now handled by core.c
******************************/

static WRITE16_HANDLER(disp1_w) {
	//LOG(("%08x: disp1_w = %04x\n",activecpu_get_pc(),data));
  if (locals.DispCol > 0x0f) {
    locals.segments[(locals.DispCol & 0x0f)+32].w = data;
  } else {
    locals.segments[locals.DispCol].w = data;
  }
}
static WRITE16_HANDLER(disp2_w) {
	//LOG(("%08x: disp2_w = %04x\n",activecpu_get_pc(),data));
  if (locals.DispCol > 0x0f) {
    locals.segments[(locals.DispCol & 0x0f)+48].w = data;
  } else {
    locals.segments[locals.DispCol+16].w = data;
  }
}

//Like rtrg - setout is connected to the same ls74 flip flop - not sure of it's purpose.
static WRITE16_HANDLER(setout_w) {
	//LOG(("%08x: setout_w = %04x\n",activecpu_get_pc(),data));
}

//NVRAM
static UINT16 *NVRAM;
static NVRAM_HANDLER(tecno_nvram) {
  core_nvram(file, read_or_write, NVRAM, 0x2000, 0x00);
}

//Memory Map for Main CPU
static MEMORY_READ16_START(readmem)
  { 0x000000, 0x003fff, MRA16_ROM },
  { 0x004000, 0x005fff, MRA16_RAM },
  { 0x006000, 0x00ffff, MRA16_ROM },
  { 0x014000, 0x014001, input_key_r },
  { 0x014800, 0x014801, input_sound_r },
  { 0x015000, 0x015001, rtrg_r },
  { 0x015800, 0x015801, rtrg2_r },
MEMORY_END

static MEMORY_WRITE16_START(writemem)
  { 0x000000, 0x003fff, MWA16_ROM },
  { 0x004000, 0x005fff, MWA16_RAM, &NVRAM },
  { 0x014000, 0x014001, lamp1_w },
  { 0x014800, 0x014801, lamp2_w },
  { 0x015000, 0x015001, sol1_w },
  { 0x015800, 0x015801, sol2_w },
  { 0x016000, 0x016001, sound_w },
  { 0x016800, 0x016801, disp1_w },
  { 0x017000, 0x017001, disp2_w },
  { 0x017800, 0x017801, setout_w },
MEMORY_END

/* Manual starts with a switch # of 0 */
static int tecno_sw2m(int no) { return no+7+1; }
static int tecno_m2sw(int col, int row) { return col*8+row-7-1; }

static WRITE_HANDLER(m7000_w) {
  coreGlobals.diagnosticLed = data;
  // bit 2 controls the X2 output, but it's connected nowhere?
}

static MEMORY_READ_START(snd_readmem)
  { 0x3000, 0x3000, Y8950_status_port_0_r },
  { 0x3001, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x5800, 0x5800, Y8950_control_port_0_w },
  { 0x5801, 0x5801, Y8950_write_port_0_w },
  { 0x6800, 0x6800, DAC_0_data_w },
  { 0x7000, 0x7000, m7000_w },
MEMORY_END

static READ_HANDLER(tms_port_r) {
  LOG(("p_in %c\n", 'A' + offset));
  return offset ? 0 : locals.sndCmd;
}
static WRITE_HANDLER(tms_port_w) {
  LOG(("p_out %c: %02x\n", 'A' + offset, data));
  if (offset == 1) {
    locals.sndAck = (data & 2) >> 1;
  }
}

static PORT_READ_START(snd_readport)
  { TMS7000_PORTA, TMS7000_PORTD, tms_port_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { TMS7000_PORTA, TMS7000_PORTD, tms_port_w },
PORT_END

static void y8950_irq(int data) {
  cpu_set_irq_line(1, TMS7000_IRQ1_LINE, data ? ASSERT_LINE : CLEAR_LINE);
}
static WRITE_HANDLER(key_w) {
  LOG(("Y8910 key_w %02x\n", data));
}
static struct Y8950interface tecno_y8950Intf =
{
	1,						/* 1 chip */
	3579545,				/* 3.58 MHz */
	{ 100 },				/* volume */
	{ y8950_irq },			/* IRQ Callback */
	{ REGION_USER1 }, /* ROM region */
	{ NULL }, { key_w },
};

struct DACinterface tecno_dacInt =
{
	1,			/* 1 chip */
	{50}		/* Volume */
};

MACHINE_DRIVER_START(tecno)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(tecno, NULL, NULL)
  MDRV_CPU_ADD_TAG("mcpu", M68000, TECNO_CPUFREQ)
  MDRV_CPU_MEMORY(readmem, writemem)
  MDRV_CPU_VBLANK_INT(vblank, 1)
  MDRV_SWITCH_UPDATE(tecno)
  MDRV_SWITCH_CONV(tecno_sw2m, tecno_m2sw)
  MDRV_NVRAM_HANDLER(tecno_nvram)
  MDRV_DIAGNOSTIC_LEDH(2)

  MDRV_CPU_ADD_TAG("scpu", TMS7000, 4000000)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_SOUND_ADD(Y8950, tecno_y8950Intf )
  MDRV_SOUND_ADD(DAC, tecno_dacInt)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
MACHINE_DRIVER_END

static WRITE_HANDLER(tecsnd_data_w) {
  if (data) {
    locals.sndCmd = data;
    cpu_set_irq_line(1, TMS7000_IRQ3_LINE, ASSERT_LINE);
  } else {
    cpu_set_irq_line(1, TMS7000_IRQ3_LINE, CLEAR_LINE);
  }
}
const struct sndbrdIntf tecnoplayIntf = {
  "TECNOPLAY", NULL, NULL, NULL, tecsnd_data_w, tecsnd_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

INPUT_PORTS_START(tecno) \
  CORE_PORTS \
  SIM_PORTS(4) \
  PORT_START /* 0 */ \

    /* Switch Column 1 */ \
    COREPORT_BIT(     0x0001, "Test Up",		  KEYCODE_7)\
	COREPORT_BIT(     0x0002, "Test Down",		  KEYCODE_8)\
	COREPORT_BIT(     0x0004, "Ball Tilt",        KEYCODE_INSERT)\
	COREPORT_BIT(     0x0008, "Service",          KEYCODE_9)\
	COREPORT_BITDEF(  0x0010, IPT_COIN1,          KEYCODE_3)\
	COREPORT_BITDEF(  0x0020, IPT_COIN2,          KEYCODE_4)\
	COREPORT_BITDEF(  0x0040, IPT_COIN3,          KEYCODE_5)\
	COREPORT_BIT(     0x0080, "Letter Sel ->",    KEYCODE_PGUP)\
	/* Switch Column 2 */ \
	COREPORT_BIT(     0x0100, "Letter Sel <-",    KEYCODE_PGDN)\
	COREPORT_BITDEF(  0x0200, IPT_START1,         IP_KEY_DEFAULT)\
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
      COREPORT_DIPSET(0x0080, "1" )
INPUT_PORTS_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG16S},
  {3, 0,16,16,CORE_SEG16S},
  {0}
};
static core_tGameData xforceGameData = {0,disp,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_TECNOPLAY}};
static void init_xforce(void) {
  core_gameData = &xforceGameData;
}
ROM_START(xforce)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD16_BYTE("ic15", 0x000001, 0x8000, CRC(fb8d2853) SHA1(0b0004abfe32edfd3ac15d66f90695d264c97eba))
    ROM_LOAD16_BYTE("ic17", 0x000000, 0x8000, CRC(122ef649) SHA1(0b425f81869bc359841377a91c39f44395502bff))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("sound.bin", 0x8000, 0x8000, NO_DUMP)
    ROM_RELOAD(0, 0x8000)
  NORMALREGION(0x40000, REGION_USER1)
ROM_END
#define input_ports_xforce input_ports_tecno
CORE_GAMEDEFNV(xforce, "X Force", 1987, "Tecnoplay", tecno, GAME_IMPERFECT_SOUND)

static core_tLCDLayout disp2[] = {
  {0, 0, 0, 1,CORE_SEG16S}, {0, 2,32,15,CORE_SEG16S},
  {3, 0,16, 1,CORE_SEG16S}, {3, 2,48,15,CORE_SEG16S},
  {0}
};
static core_tGameData spcteamGameData = {0,disp2,{FLIP_SW(FLIP_L),0,0,0,SNDBRD_TECNOPLAY}};
static void init_spcteam(void) {
  core_gameData = &spcteamGameData;
}
ROM_START(spcteam)
  NORMALREGION(0x1000000, REGION_CPU1)
    ROM_LOAD16_BYTE("cpu_top.bin", 0x000001, 0x8000, CRC(b11dcf1f) SHA1(084eb98ee4c9f32d5518897a891ad1a601850d80))
    ROM_LOAD16_BYTE("cpu_bot.bin", 0x000000, 0x8000, CRC(892a5592) SHA1(c30dce37a5aae2834459179787f6c99353aadabb))
  NORMALREGION(0x10000, REGION_CPU2)
    ROM_LOAD("sound.bin", 0x8000, 0x8000, CRC(6a87370f) SHA1(51e055dcf23a30e337ff439bba3c40e5c51c490a))
    ROM_RELOAD(0, 0x8000)
  NORMALREGION(0x40000, REGION_USER1)
ROM_END
#define input_ports_spcteam input_ports_tecno
CORE_GAMEDEFNV(spcteam, "Space Team", 1988, "Tecnoplay", tecno, GAME_IMPERFECT_SOUND)
