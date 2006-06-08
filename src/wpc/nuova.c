/************************************************************************************************
  Nuova Bell Games
  ----------------
  by Steve Ellenoff, with fixes / hacks by Gerrit Volkenborn

  Main CPU Board:

  CPU: Motorola M6802
  Clock: Unknown (1Mhz?)
  Interrupt: IRQ - Via the 6821 chips, NMI - Push Button?
  I/O: 2 X 6821

  Issues/Todo:
  Sound,
  Lamps: Lamp Addr is 1-16 data, Lamp Data 0-3 (each bit selects different 1-16 mux) - Strobe 2 used for Aux Lamps
  But the lamp data never changes???

  Game is done with testing @ 14A3?
  143E - Display some digits?
  152e - CLI - Clear Interrupt Disable
  RAM - 680 - Contains text for display driver
  DP @ 1098 for display out routine (loop begins @ 1089?) - similar @ 10c9 (starts @ 10ba?)
************************************************************************************************/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sim.h"
#include "sndbrd.h"

#define NUOVA_SOLSMOOTH		4
#define NUOVA_CPUFREQ		1000000		//1 Mhz ??
#define NOUVA_ZCFREQ		240			//120 Hz * 2 ??
#define NOUVA_555TIMER_FREQ	10			//??

#define NOUVA_SWSNDDIAG		-2
#define NOUVA_SWCPUDIAG		-1
#define NOUVA_SWCPUBUTT		0

#if 0
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#if 1
#define LOGSND(x) logerror x
#else
#define LOGSND(x)
#endif

static struct {
  int vblankCount;
  core_tSeg segments;
  UINT32 solenoids;
  UINT16 sols2;
  int diagnosticLed;
  int piaIrq;
  int SwCol;
  int dispCol[4];
  int LampCol;
  int zero_cross;
  int timer_555;
  int last_nmi_state;
  int pia0_a;
  int pia1_a;
  int pia0_cb2;
  int pia1_cb2;
  int pia0_da_enable;
  UINT8 sndCmd;
} locals;

/***************/
/* ZERO CROSS? */
/***************/
static void nuova_zeroCross(int data) {
	 locals.zero_cross = !locals.zero_cross;
	 pia_set_input_cb1(0,locals.zero_cross);
}

/********************/
/* 555 Timer CROSS? */
/********************/
static void nuova_555timer(int data) {
	 locals.timer_555 = !locals.timer_555;
	 pia_set_input_ca1(1,locals.timer_555);
}

static INTERRUPT_GEN(nuova_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));

  coreGlobals.solenoids = locals.solenoids;
  if ((locals.vblankCount % NUOVA_SOLSMOOTH) == 0) {
	locals.solenoids = coreGlobals.pulsedSolState;
  }

  coreGlobals.diagnosticLed = locals.diagnosticLed;

  core_updateSw(core_getSol(18));
}

static SWITCH_UPDATE(nuova) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT]<<4,0xe0,0);
    CORE_SETKEYSW(inports[CORE_COREINPORT],   0x60,1);
    CORE_SETKEYSW(inports[CORE_COREINPORT]>>8,0x87,2);
  }
  // CPU DIAG SWITCH
  if (core_getSw(NOUVA_SWCPUBUTT))
  {
	  if(!locals.last_nmi_state)
	  {
		  locals.last_nmi_state = 1;
		  cpu_set_nmi_line(0, ASSERT_LINE);
	  }
  }
  else
  {
	  if(locals.last_nmi_state)
	  {
		  locals.last_nmi_state = 0;
		  cpu_set_nmi_line(0, CLEAR_LINE);
	  }
  }
  pia_set_input_ca1(0,core_getSw(NOUVA_SWCPUDIAG));
  if (core_getSw(NOUVA_SWSNDDIAG)) {
    cpu_set_nmi_line(1, ASSERT_LINE);
    cpu_set_irq_line(1, M6803_IRQ_LINE, ASSERT_LINE);
  } else {
    cpu_set_nmi_line(1, CLEAR_LINE);
    cpu_set_irq_line(1, M6803_IRQ_LINE, CLEAR_LINE);
  }
}

static void nuova_irqline(int state) {
  cpu_set_irq_line(0, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void nuova_piaIrq(int state) {
  nuova_irqline(locals.piaIrq = state);
}

/* ----- */
/* PIA 0 */
/* ----- */

/*  i  PB0-7:  Dips  1-32 Read & Switch Read 0-7 & Cabinet Switch Read 0-7 */
static READ_HANDLER(pia0_b_r)
{
	int data = 0;
	if(locals.pia0_a & 0xE0 || locals.pia0_cb2 )
	{
		int dipcol = ((locals.pia0_a)>>5) | (locals.pia0_cb2<<3);
		if(dipcol < 9)
		{
			dipcol = core_BitColToNum(dipcol);
			data = core_getDip(dipcol);
			LOG(("%04x: DIP SWITCH BANK #%d - READ: pia0_b_r =%x\n",activecpu_get_previouspc(),dipcol,data));
		}
	}
	else
	{
		data = coreGlobals.swMatrix[locals.SwCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
		LOG(("%04x: SWITCH COL #%d - READ: pia0_b_r =%x\n",activecpu_get_previouspc(),locals.SwCol,data));
	}
	return data;
}
/*
  o  PA0-PA1 Switch Strobe 0 - 1 & Cabinet Switch Strobe 0 - 1 & Lamp Addr 0 - 1 & Disp Strobe 0 - 1
  o  PA2-PA3 Switch Strobe 2 - 3 & Lamp Addr 2 - 3 & Disp Strobe 2 - 3
  o  PA4:    Switch Strobe 4   & Lamp Data 0 & Disp Data 0
  o  PA5:    Dips  1-8 Strobe  & Lamp Data 1 & Disp Data 1
  o  PA6:    Dips  9-16 Strobe & Lamp Data 2 & Disp Data 2
  o  PA7:    Dips 17-24 Strobe & Lamp Data 3 & Disp Data 3
*/
static WRITE_HANDLER(pia0_a_w)
{
	locals.pia0_a = data;
	locals.SwCol = (data & 0x1f) ? core_BitColToNum(data & 0x1f) : 5;
	LOG(("%04x: EVERYTHING: pia0_a_w = %x \n",activecpu_get_previouspc(),data));
}
/*  o  CA2:    Display Blank */
static WRITE_HANDLER(pia0_ca2_w)
{
	locals.pia0_da_enable = data & 1;
	LOG(("%04x: DISP BLANK: pia0_ca2_w = %x \n",activecpu_get_previouspc(),data));
}
/*  o  CB2:    Dips 25-32 Strobe & Lamp Strobe #1 */
static WRITE_HANDLER(pia0_cb2_w)
{
	UINT8 col = locals.pia0_a & 0x0f;
	locals.pia0_cb2 = data & 1;
	LOG(("%04x: DIP25 STR & LAMP STR 1: pia0_cb2_w = %x \n",activecpu_get_previouspc(),data));
	if (col % 2 == 0)
		coreGlobals.tmpLampMatrix[col/2] = (coreGlobals.tmpLampMatrix[col/2] & 0xf0) | (locals.pia0_a >> 4);
	else
		coreGlobals.tmpLampMatrix[col/2] = (coreGlobals.tmpLampMatrix[col/2] & 0x0f) | (locals.pia0_a & 0xf0);
}

 /* -----------*/
 /* PIA 1 (U10)*/
 /* -----------*/

/* i  CB1:    Marked FE */
static READ_HANDLER(pia1_cb1_r)
{
	LOG(("%04x: FE?: pia1_cb1_r \n",activecpu_get_previouspc()));
	return 0;
}

static const UINT16 core_ascii2seg[] = {
  /* 0x00-0x07 */ 0x0000, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x08-0x0f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x10-0x17 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x18-0x1f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x20-0x27 */ 0x0000, 0x0309, 0x0220, 0x2A4E, 0x2A6D, 0x5d64, 0x135D, 0x0200, //  !"#$%&'
  /* 0x28-0x2f */ 0x1400, 0x4100, 0x7F40, 0x2A40, 0x0080, 0x0840, 0x8000, 0x4400, // ()*+,-./
  /* 0x30-0x37 */ 0x003f, 0x2200, 0x085B, 0x084f, 0x0866, 0x086D, 0x087D, 0x0007, // 01234567
  /* 0x38-0x3f */ 0x087F, 0x086F, 0x0800, 0x8080, 0x1400, 0x0848, 0x4100, 0x2803, // 89:;<=>?
  /* 0x40-0x47 */ 0x205F, 0x0877, 0x2A0F, 0x0039, 0x220F, 0x0079, 0x0071, 0x083D, // @ABCDEFG
  /* 0x48-0x4f */ 0x0876, 0x2209, 0x001E, 0x1470, 0x0038, 0x0536, 0x1136, 0x003f, // HIJKLMNO
  /* 0x50-0x57 */ 0x0873, 0x103F, 0x1873, 0x086D, 0x2201, 0x003E, 0x4430, 0x5036, // PRQSTUVW
  /* 0x58-0x5f */ 0x5500, 0x2500, 0x4409, 0x0039, 0x1100, 0x000f, 0x0402, 0x0008, // XYZ[\]^_
  /* 0x60-0x67 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x68-0x6f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x70-0x77 */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  /* 0x78-0x7f */ 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
};

/*
  o  PA0     Display Strobe 5
  o  PA1-7   Display Digit 1-7 (note: diagram shows PA7 =Digit 1, PA6 = Digit 2, etc..)
*/
static WRITE_HANDLER(pia1_a_w)
{
	static int counter = 0;
	LOG(("%04x: DISPLAY: pia1_a_w = %x \n",activecpu_get_previouspc(),data));
	if (data & 0x80) { // 1st display strobe
		if (locals.dispCol[0] < 20)
			locals.segments[locals.dispCol[0]++].w = core_ascii2seg[locals.pia0_a & 0x7f];
		counter++;
		if (counter > 7) {
			counter = 0;
			locals.dispCol[0] = 0;
			locals.dispCol[1] = 0;
		}
	} else if (data & 0x40) { // 2nd display strobe
		counter = 0;
		if (locals.dispCol[1] < 12)
			locals.segments[20+(locals.dispCol[1]++)].w = core_ascii2seg[locals.pia0_a & 0x7f];
	}
	locals.pia1_a = data;
}
/*
  o  PB0:    A Solenoid
  o  PB1:    B Solenoid
  o  PB2:    C Solenoid
  o  PB3:    D Solenoid & AUX STS Signal
  o  PB4:    ? (sound?)
  o  PB5:    ? (sound?)
  o  PB6:    ? (sound?)
  o  PB7:    ? (sound?)
*/
static WRITE_HANDLER(pia1_b_w)
{
	if (locals.pia1_cb2) { // sound
		locals.sndCmd = data & 0x0f;
		if ((data & 0x0f) < 0x0f) LOGSND(("snd data w = %x\n", data & 0x0f));
	} else if ((data & 0x20) == 0) { // activates any solenoid
		locals.solenoids = 0xf7fff & ((core_revbyte(~data & 0xd0) << 16) | (1 << (data & 0x0f)));
		LOG(("%04x: SOLS & (SOUND?): pia1_b_w = %x \n",activecpu_get_previouspc(),data));
	}
}
/* o  CA2:    LED & Lamp Strobe #2 */
static WRITE_HANDLER(pia1_ca2_w)
{
	UINT8 col = locals.pia0_a & 0x0f;
	locals.diagnosticLed = (locals.diagnosticLed & 2) | (data & 1);
	LOG(("%04x: LED & Lamp Strobe #2: pia1_ca2_w = %x \n",activecpu_get_previouspc(),data));
	if (col % 2 == 0)
		coreGlobals.tmpLampMatrix[8 + col/2] = (coreGlobals.tmpLampMatrix[8 + col/2] & 0xf0) | (locals.pia0_a >> 4);
	else
		coreGlobals.tmpLampMatrix[8 + col/2] = (coreGlobals.tmpLampMatrix[8 + col/2] & 0x0f) | (locals.pia0_a & 0xf0);
}
/* o  CB2:    Solenoid Bank Select */
static WRITE_HANDLER(pia1_cb2_w)
{
	locals.pia1_cb2 = data & 1;
	LOG(("%04x: SOL BANK SELECT: pia1_cb2_w = %x \n",activecpu_get_previouspc(),data));
}

static const struct pia6821_interface nuova_pia[] = {
{ /* PIA 0 (U9)
  -------------
  o  PA0-PA1 Switch Strobe 0 - 1 & Cabinet Switch Strobe 0 - 1 & Lamp Addr 0 - 1 & Disp Addr 0 - 1
  o  PA2-PA3 Switch Strobe 2 - 3 & Lamp Addr 2 - 3 & Disp Addr 2 - 3
  o  PA4:    Switch Strobe 4   & Lamp Data 0 & Disp Data 0
  o  PA5:    Dips  1-8 Strobe  & Lamp Data 1 & Disp Data 1
  o  PA6:    Dips  9-16 Strobe & Lamp Data 2 & Disp Data 2
  o  PA7:    Dips 17-24 Strobe & Lamp Data 3 & Disp Data 3
  i  PB0-7:  Dips  1-32 Read & Swtich Read 0-7 & Cabinet Switch Read 0-7
  i  CA1:    Diagnostic Switch?
  i  CB1:    Tied to 43V line (Some kind of zero cross detection?)
  o  CA2:    Activates Disp Addr Data
  o  CB2:    Dips 25-32 Strobe & Lamp Strobe #1 */
 /* in  : A/B,CA1/B1,CA2/B2 */ 0, pia0_b_r, 0, 0, 0, 0,
 /* out : A/B,CA2/B2        */ pia0_a_w, 0, pia0_ca2_w, pia0_cb2_w,
 /* irq : A/B               */ nuova_piaIrq, nuova_piaIrq
},{
 /* PIA 1 (U10)
 -------------
  o  PA0     Display Blanking
  o  PA1-7   Display Digit 1-7 (note: diagram shows PA7 =Digit 1, PA6 = Digit 2, etc..)
  o  PB0:    A Solenoid
  o  PB1:    B Solenoid
  o  PB2:    C Solenoid
  o  PB3:    D Solenoid & AUX STS Signal
  o  PB4:    ? (sound?)
  o  PB5:    ? (sound?)
  o  PB6:    ? (sound?)
  o  PB7:    ? (sound?)
  i  CA1:    Tied to a 555 timer
  i  CB1:    Marked FE
  o  CA2:    LED & Lamp Strobe #2
  o  CB2:    Solenoid Bank Select */
 /* in  : A/B,CA1/B1,CA2/B2 */ 0, 0, 0, pia1_cb1_r, 0, 0,
 /* out : A/B,CA2/B2        */ pia1_a_w, pia1_b_w, pia1_ca2_w, pia1_cb2_w,
 /* irq : A/B               */ nuova_piaIrq, nuova_piaIrq
}
};

static MACHINE_INIT(nuova) {
  memset(&locals, 0, sizeof(locals));
  pia_config(0, PIA_STANDARD_ORDERING, &nuova_pia[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &nuova_pia[1]);
  sndbrd_0_init(SNDBRD_ZAC1346, 1, memory_region(REGION_CPU2), NULL, NULL);
}

static MACHINE_RESET(nuova) {
  pia_reset();
}

static MACHINE_STOP(nuova) {
  //sndbrd_0_exit();
}

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static data8_t *s6_CMOS;
static NVRAM_HANDLER(nuova) {
  core_nvram(file, read_or_write, s6_CMOS, 0x0800, 0x00); // 2K of RAM, battery-backed
}
static WRITE_HANDLER(s6_CMOS_w_0) {
	s6_CMOS[offset] = data;
}
static READ_HANDLER(s6_CMOS_r_0) {
	return s6_CMOS[offset];
}
static WRITE_HANDLER(s6_CMOS_w_1) {
	s6_CMOS[0x8c + offset] = data;
}
static READ_HANDLER(s6_CMOS_r_1) {
	return s6_CMOS[0x8c + offset];
}
static WRITE_HANDLER(s6_CMOS_w_2) {
	s6_CMOS[0x94 + offset] = data;
}
static READ_HANDLER(s6_CMOS_r_2) {
	return s6_CMOS[0x94 + offset];
}
/*-----------------------------------
/  Memory map for Main CPU board
/------------------------------------*/
static MEMORY_READ_START(cpu_readmem)
  { 0x0000, 0x0087, s6_CMOS_r_0 },
  { 0x0088, 0x008b, pia_r(0)},
  { 0x008c, 0x008f, s6_CMOS_r_1 },
  { 0x0090, 0x0093, pia_r(1)},
  { 0x0094, 0x07ff, s6_CMOS_r_2 },
  { 0x1000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(cpu_writemem)
  { 0x0000, 0x0087, s6_CMOS_w_0 },
  { 0x0088, 0x008b, pia_w(0)},
  { 0x008c, 0x008f, s6_CMOS_w_1 },
  { 0x0090, 0x0093, pia_w(1)},
  { 0x0094, 0x07ff, s6_CMOS_w_2 },
  { 0x0800, 0x0fff, MWA_RAM, &s6_CMOS }, // initialization of the RAM
MEMORY_END

static WRITE_HANDLER(bank_w) {
  data = ~data;
  LOGSND(("m8000w = %x\n",data));
  locals.diagnosticLed = (locals.diagnosticLed & 1) | ((data >> 6) & 2);
  if (data & 0x0f)
    cpu_setbank(1, memory_region(REGION_SOUND1) + (0x8000 * data & 0x07));
  else
    cpu_setbank(1, memory_region(REGION_SOUND1));
}

static READ_HANDLER(snd_cmd_r) {
  return locals.sndCmd;
}

static WRITE_HANDLER(snd_enable) {
  LOGSND(("sound P2w: %02x\n", data));
}

static MEMORY_READ_START(snd_readmem)
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x8000, 0xffff, MRA_BANKNO(1) },
MEMORY_END

static MEMORY_WRITE_START(snd_writemem)
  { 0x0000, 0x00ff, MWA_RAM },
  { 0xc000, 0xc000, bank_w  },
MEMORY_END

static PORT_READ_START(snd_readport)
  { M6803_PORT2, M6803_PORT2, snd_cmd_r },
PORT_END

static PORT_WRITE_START(snd_writeport)
  { M6803_PORT1, M6803_PORT1, DAC_0_data_w },
  { M6803_PORT2, M6803_PORT2, snd_enable },
PORT_END

static core_tLCDLayout disp[] = {
  {0, 0, 0,16,CORE_SEG16},
  {3, 0,16,16,CORE_SEG16},
  {0}
};
static core_tGameData nuovaGameData = {GEN_ZAC2, disp, {FLIP_SWNO(48, 0), 0, 2}};
static void init_nuova(void) {
  core_gameData = &nuovaGameData;
}
static struct DACinterface nuova_dacInt = { 1, { 25 }};

MACHINE_DRIVER_START(nuova)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(nuova,nuova,nuova)
  MDRV_CPU_ADD_TAG("mcpu", M6802, NUOVA_CPUFREQ)
  MDRV_CPU_MEMORY(cpu_readmem, cpu_writemem)
  MDRV_CPU_VBLANK_INT(nuova_vblank, 1)
  MDRV_NVRAM_HANDLER(nuova)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(nuova)
  MDRV_DIAGNOSTIC_LEDH(2)
  MDRV_TIMER_ADD(nuova_zeroCross, NOUVA_ZCFREQ)
  MDRV_TIMER_ADD(nuova_555timer,  NOUVA_555TIMER_FREQ)

  MDRV_CPU_ADD_TAG("scpu", M6803, 1000000)
  MDRV_CPU_MEMORY(snd_readmem, snd_writemem)
  MDRV_CPU_PORTS(snd_readport, snd_writeport)
  MDRV_SOUND_ADD(DAC, nuova_dacInt)
  MDRV_INTERLEAVE(500)
MACHINE_DRIVER_END

INPUT_PORTS_START(nuova) \
  CORE_PORTS \
  SIM_PORTS(4) \
  PORT_START /* 0 */ \
  /* Switch Column 1 */ \
    COREPORT_BITDEF(  0x0020, IPT_START1,         IP_KEY_DEFAULT) \
    COREPORT_BIT(     0x0040, "Ball Tilt",        KEYCODE_INSERT) \
    COREPORT_BITDEF(  0x0100, IPT_COIN1,          IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0200, IPT_COIN2,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0400, IPT_COIN3,          KEYCODE_4) \
    COREPORT_BIT   (  0x8000, "Slam Tilt",        KEYCODE_HOME) \
  /* These are put in switch column 0 since they are not read in the regular switch matrix */ \
    COREPORT_BIT(     0x0008, "CPU Button",       KEYCODE_7) \
	COREPORT_BIT(     0x0004, "Self Test",        KEYCODE_8) \
	COREPORT_BIT(     0x0002, "Sound Test",       KEYCODE_0) \
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

//U6 Ram Notes
// A0-A10 Used (0x800) size - accessible during E-cycle at any address?

//U7 Rom Notes
//
// The schematic has got to be wrong for a variety of reasons, but luckily, I found the startup
//
// After finally finding the right map to get the cpu started
// -Watching the rom boot up it actually tests different sections of rom, which is a huge help!
// - I have no idea how this actually makes any sense!
// Check - Data @ 0x1000 = 0x1A (found @ 0000)
// Check - Data @ 0x5000 = 0x55 (found @ 1000) (A14->A12? or not used?)
// Check - Data @ 0x7000 = 0x7A (found @ 5000) (A13 = 0?)
// Check - Data @ 0x9000 = 0x95 (found @ 2000) (A15 = 0, A12 -> A13)
// Check - Data @ 0xB000 = 0xBA (found @ 6000) (A15->A13, A12->A14, A14->A12)
// Check - Data @ 0xD000 = 0xD5 (found @ 3000) (A15->A13, A14=0)
// Check - Data @ 0xF000 = 0xFA (found @ 7000) (A15->A13, A12->A14, A14->A12)

ROM_START(f1gp)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("cpu_u7", 0x8000, 0x8000, CRC(2287dea1) SHA1(5438752bf63aadaa6b6d71bbf56a72d8b67b545a))
  ROM_COPY(REGION_CPU1, 0x8000, 0x1000,0x1000)
  ROM_COPY(REGION_CPU1, 0x9000, 0x5000,0x1000)
  ROM_COPY(REGION_CPU1, 0xd000, 0x7000,0x1000)
  ROM_COPY(REGION_CPU1, 0xa000, 0x9000,0x1000)
  ROM_COPY(REGION_CPU1, 0xb000, 0xd000,0x1000)
  ROM_COPY(REGION_CPU1, 0xe000, 0xb000,0x1000)

  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8b", 0x0000, 0x8000, CRC(14cddb29) SHA1(667b54174ad5dd8aa45037574916ecb4ee996a94))
    ROM_LOAD("snd_u8a", 0x8000, 0x8000, CRC(3a2af90b) SHA1(f6eeae74b3bfb1cfd9235c5214f7c029e0ad14d6))
    ROM_LOAD("snd_u9b", 0x10000,0x8000, CRC(726920b5) SHA1(002e7a072a173836c89746cceca7e5d2ac26356d))
    ROM_LOAD("snd_u9a", 0x18000,0x8000, CRC(681ee99c) SHA1(955cd782073a1ce0be7a427c236d47fcb9cccd20))
    ROM_LOAD("snd_u10b",0x20000,0x8000, CRC(9de359fb) SHA1(ce75a78dc4ed747421a386d172fa0f8a1369e860))
    ROM_LOAD("snd_u10a",0x28000,0x8000, CRC(4d3fc9bb) SHA1(d43cd134f399e128a678b86e57b1917fad70df76))
    ROM_LOAD("snd_u11b",0x30000,0x8000, CRC(2394b498) SHA1(bf0884a6556a27791e7e801051be5975dd6b95c4))
    ROM_LOAD("snd_u11a",0x38000,0x8000, CRC(884dc754) SHA1(b121476ea621eae7a7ba0b9a1b5e87051e1e9e3d))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

#define init_f1gp init_nuova
#define input_ports_f1gp input_ports_nuova
CORE_GAMEDEFNV(f1gp, "F1 Grand Prix", 1987, "Nuova Bell Games", nuova, GAME_NOT_WORKING)

// Rom areas are not determined yet.
ROM_START(futrquen)
  NORMALREGION(0x10000, REGION_CPU1)
    ROM_LOAD("mpu_u2.bin", 0xc000, 0x4000, CRC(bc66b636) SHA1(65f3e6461a1eca8542bbbc5b8c7cd1fca1b3011f))
  ROM_COPY(REGION_CPU1, 0xc000, 0x1000,0x1000)
  NORMALREGION(0x40000, REGION_SOUND1)
    ROM_LOAD("snd_u8.bin", 0x0000, 0x8000, CRC(3d254d89) SHA1(2b4aa3387179e2c0fbf18684128761d3f778dcb2))
    ROM_LOAD("snd_u9.bin", 0x10000,0x8000, CRC(9560f2c3) SHA1(3de6d074e2a3d3c8377fa330d4562b2d266bbfff))
    ROM_LOAD("snd_u10.bin",0x20000,0x8000, CRC(70f440bc) SHA1(9fa4d33cc6174ce8f43f030487171bfbacf65537))
    ROM_LOAD("snd_u11.bin",0x30000,0x8000, CRC(71d98d17) SHA1(9575b80a91a67b1644e909f70d364e0a75f73b02))
  NORMALREGION(0x10000, REGION_CPU2)
  ROM_COPY(REGION_SOUND1, 0x0000, 0x8000,0x8000)
ROM_END

#define init_futrquen init_nuova
#define input_ports_futrquen input_ports_nuova
CORE_GAMEDEFNV(futrquen, "Future Queen", 198?, "Nuova Bell Games", nuova, GAME_NOT_WORKING)
