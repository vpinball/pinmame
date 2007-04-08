/* Game Plan Pinball
   -----------------
   by Steve Ellenoff (01/22/2002)

   Hardware:

   CPU: Z80 & Z80CTC (Controls Interrupt Generation & Zero Cross Detection)
   I/O: 8255

   Note: Although the mpu schematics indicate a 33rd Dip switch, it's actually the
         Accounting Reset Switch, which is read as Switch 1 in the matrix.

   Issues:
		a) Sometimes the display seems to not get erased properly, ie, it retains displayed data
		   (at startup (for vegasgp), notice how credit display = 0 while others are blank)
		   (occurs especially in the ball/credit areas, but also occurs during lamp test for other digits)
		   Update TomB Feb'05: That's the roms. The display buffers are located in nvram and
		   aren't always initialized.
		b) No idea how solenoids 17 & 18 (display on/off) get triggered. Not at all, perhaps?
		c) Solenoid ordering not adjusted for all games. But then, Bally suffers from the same problem...
*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "machine/8255ppi.h"
#include "core.h"
#include "sndbrd.h"
#include "gp.h"
#include "gpsnd.h"

#define GP_ZCFREQ   120

static WRITE_HANDLER(GP_soundCmd)  { }

static struct {
  int p0_a;
  int bcd[5];
  int lampaddr;
  int diagnosticLed;
  int vblankCount;
  int swCol;
  int disp_enable;		//Is Display Enabled
  int disp_col;			//Current Display Column/Digit Selected
  int last_clk;			//Last Digit Clock Line to go low
  UINT32 solenoids;
  core_tSeg segments,pseg;
  WRITE_HANDLER((*PORTA_WRITE));
} locals;

static void GP_dispStrobe(int mask) {
  int digit = locals.disp_col;
  int ii,jj;
//  logerror("mask = %x, digit = %x (%x,%x,%x,%x,%x)\n",mask, digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4]);
  ii = (digit < 7)? digit : 0;
  jj = mask;
  locals.segments[jj*8+ii].w |= locals.pseg[jj*8+ii].w = core_bcd2seg7a[locals.bcd[jj]];
}

static void GP_lampStrobe1(int lampadr, int lampdata) {
  UINT8 *matrix1, *matrix2;
  int bit, select;
  lampdata &= 0x07;	//3 Enable Lines
  select = lampdata>>1;
  matrix1 = &coreGlobals.tmpLampMatrix[select];
  matrix2 = &coreGlobals.tmpLampMatrix[select+1];
  bit = 1<<lampadr;
  while (lampdata) {
    if (lampdata & 0x01) { *matrix1 |= bit; *matrix2 |= (bit>>8); }
    lampdata >>= 1; matrix1++; matrix2++;
  }
}

static void GP_lampStrobe2(int lampadr, int lampdata) {
  UINT8 *matrix1, *matrix2;
  int bit, select;
  lampdata &= 0x0f;	//4 Enable Lines
  select = lampdata>>1;
  if (select > 3) select = 3;
  matrix1 = &coreGlobals.tmpLampMatrix[select];
  matrix2 = &coreGlobals.tmpLampMatrix[select+1];
  bit = 1<<lampadr;
  while (lampdata) {
    if (lampdata & 0x01) { *matrix1 |= bit; *matrix2 |= (bit>>8); }
    lampdata >>= 1; matrix1++; matrix2++;
  }
}

static void GP_UpdateSolenoids (int bank, int data) {
  static UINT32 mask = 0xC0008000;
  UINT32 sols = 0;
  int soldata = data & 0x0f;
  if (bank == 0) { // solenoids 1-15
    if (soldata != 0x0f) {
      if (core_gameData->hw.soundBoard == SNDBRD_GPSSU1 && (soldata < 2 || (soldata > 4 && soldata < 7)))
        sndbrd_0_data_w(GP_SCPUNO, soldata);
      sols = 1 << soldata;
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
      locals.solenoids |= sols;
    } else { // until we find another way to turn the solenoids back off...
	  if (core_gameData->hw.soundBoard == SNDBRD_GPSSU1) sndbrd_0_data_w(0, soldata);
      coreGlobals.pulsedSolState &= mask;
      locals.solenoids &= mask;
    }
  } else { // sound commands, solenoids 17-30
//  if ((data & 0x0f) != 0x0f) printf("snd = %02x\n", data);
    if (core_gameData->hw.soundBoard != SNDBRD_GPSSU1)
      sndbrd_0_data_w(GP_SCPUNO, data);
    if (soldata != 0x0f) {
      sols = 0x10000 << soldata;
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
      locals.solenoids |= sols;
    } else { // until we find another way to turn the solenoids back off...
      coreGlobals.pulsedSolState &= mask;
      locals.solenoids &= mask;
    }
  }
}

static INTERRUPT_GEN(GP_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % GP_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % GP_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % GP_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
  }
  core_updateSw(core_getSol(16));
}

static SWITCH_UPDATE(GP) {
  static UINT8 val = 0;
  if (inports) {
	coreGlobals.swMatrix[0] = (inports[GP_COMINPORT]>>9) & 0x03;
	coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0xf7)) |
                              ((inports[GP_COMINPORT] & 0xff) & 0xf7);
	coreGlobals.swMatrix[4] = (coreGlobals.swMatrix[4] & (~0x02)) |
                              ((inports[GP_COMINPORT]>>7) & 0x02);
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(GP_SWTEST)) generic_nvram[0xac] = val++;
  if (core_getSw(GP_SWSOUNDDIAG) && core_gameData->hw.soundBoard == SNDBRD_GPMSU3)
    cpu_set_nmi_line(GP_SCPUNO, PULSE_LINE);
}

/*
PORT B READ
(in) P0-P7: Switch & Dip Returns
*/
static READ_HANDLER(ppi0_pb_r) {
	int dipstrobe = locals.p0_a>>4;
	if (dipstrobe == 12) return core_getDip(0); // DIP#1 1-8
	if (dipstrobe == 13) return core_getDip(1); // DIP#2 9-16
	if (dipstrobe == 14) return core_getDip(2); // DIP#3 17-24
	if (dipstrobe == 15) return core_getDip(3); // DIP#4 25-32
	return core_getSwCol(locals.swCol);
}

/*PORT A WRITE - Generation Specific*/
static WRITE_HANDLER(ppi0_pa_w) {
	locals.PORTA_WRITE(offset,data);
}

/************************  MPU-1 GENERATION ********************************
PORT A WRITE
(out) P0-P3:
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)??? Not sure

(out) P4-P7P: Address Data
  (1-16 Demultiplexed - Output are all active low)
	0) = NA
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-5) = Lamp Enable/Data 1-3
	6) = NA
	7) = Switch Strobe 0
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Display Clock 4
	12-15) Dip Column Strobes
*/
static WRITE_HANDLER(mpu1_pa_w) {
	int addrdata = data & 0x0f;	//Take P0-P3
	int tmpdata =  data>>4;		//Take P4-P7
	int disp_clk = tmpdata - 8;	//Track Display Clock line
	locals.p0_a = data;

	/*Pin 1 not connected, but is used after a display clock line goes low
	  to trigger the lo->hi transition, and load in bcd data*/
	if(tmpdata == 0 && (locals.last_clk ^ 0x80)) {
		if(locals.disp_enable) {
				locals.bcd[locals.last_clk] = addrdata;
				GP_dispStrobe(locals.last_clk);
				locals.last_clk = 0x80;		//Set flag so that this routine only runs AFTER a valid display clock line was set!
		}
	}

	/*Enabling Solenoids? (Address Line 1,2)*/
	if(tmpdata>0 && tmpdata<3) {
		//No solenoid is wired for 15, so we use it for releasing the sols.
//		logerror("%x: sol en: %x addr %x \n",activecpu_get_previouspc(),tmpdata-1,addrdata);
		GP_UpdateSolenoids(tmpdata-1,addrdata);
	}

	/*Updating Lamps? (Address Line 3-5)*/
	if(tmpdata>2 && tmpdata<6)
		GP_lampStrobe1(addrdata,1<<(tmpdata-3));

	/*Strobing Switches? (Address Line 7-10)*/
	if(tmpdata>6 && tmpdata<11)
		locals.swCol = 1<<(tmpdata-7);

	/*Strobing Digit Displays? (Address Line 8-11)
	  ------------------------
	  Clock lines go low on selection, but are clocked in on lo->hi transition...
	  So we track the last clock line to go lo, and when it changes, we write in bcd data
	*/
	if(tmpdata>7 && tmpdata<12) {
		if(locals.disp_enable) {
			locals.last_clk = disp_clk;
//			logerror("%x: disp clock #%x, data=%x\n",activecpu_get_previouspc(),disp_clk,addrdata);
		}
	}

//	logerror("PA_W: P4-7 %x = %d\n",tmpdata,tmpdata);
//	logerror("PA_W: P0-3 %x = %d\n",addrdata,addrdata);
}

/************************  MPU-2 GENERATION ********************************
PORT A WRITE
(out) P0-P3:
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)??? Not sure

(out) P4-P7P: Address Data
  (1-16 Demultiplexed - Output are all active low)
	0) = NA
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-6) = Lamp Data 1-4
	7) = Switch Strobe 0 AND BDU - Clock 1
	8) = Switch Strobe 1 AND Display Clock 1 AND BDU - Clock 2
	9) = Switch Strobe 2 AND Display Clock 2 AND BDU - Clock 3
	10) = Switch Strobe 3 AND Display Clock 3 AND BDU - Clock 4
	11) = Switch Strobe 4 AND Display Clock 4 AND BDU - Clock 5
	12-15) Dip Column Strobes
*/
static WRITE_HANDLER(mpu2_pa_w) {
	int addrdata = data & 0x0f;	//Take P0-P3
	int tmpdata =  data>>4;		//Take P4-P7
	int disp_clk = tmpdata - 7;	//Track Display Clock line
	locals.p0_a = data;

	/*Pin 1 not connected, but is used after a display clock line goes low
	  to trigger the lo->hi transition, and load in bcd data*/
	if(tmpdata == 0 && (locals.last_clk ^ 0x80)) {
		if(locals.disp_enable) {
				locals.bcd[locals.last_clk] = addrdata;
				GP_dispStrobe(locals.last_clk);
				locals.last_clk = 0x80;		//Set flag so that this routine only runs AFTER a valid display clock line was set!
		}
	}

	/*Enabling Solenoids? (Address Line 1,2)*/
	if(tmpdata>0 && tmpdata<3) {
		//No solenoid is wired for 15, so we use it for releasing the sols.
//		logerror("%x: sol en: %x addr %x \n",activecpu_get_previouspc(),tmpdata-1,addrdata);
		GP_UpdateSolenoids(tmpdata-1,addrdata);
	}

	/*Updating Lamps? (Address Line 3-6)*/
	if(tmpdata>2 && tmpdata<7)
		GP_lampStrobe2(addrdata,1<<(tmpdata-3));

	/*Strobing Switches? (Address Line 7-11)*/
	if(tmpdata>6 && tmpdata<12)
		locals.swCol = 1<<(tmpdata-7);

	/*Strobing Digit Displays? (Address Line 7-11)
	  ------------------------
	  Clock lines go low on selection, but are clocked in on lo->hi transition...
	  So we track the last clock line to go lo, and when it changes, we write in bcd data
	*/
	if(tmpdata>6 && tmpdata<12) {
		if(locals.disp_enable) {
			locals.last_clk = disp_clk;
//			logerror("%x: disp clock #%x, data=%x\n",activecpu_get_previouspc(),disp_clk,addrdata);
		}
	}

//	logerror("PA_W: P4-7 %x = %d\n",tmpdata,tmpdata);
//	logerror("PA_W: P0-3 %x = %d\n",addrdata,addrdata);
}

static WRITE_HANDLER(ppi0_pb_w) {
	logerror("PPI0 Port B Write: %02x\n", data);
}

/*
PORT C WRITE
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Display Enable (Active Low)
(out) P5 : Spare output (J7-7) - Mapped as solenoid #31 ==> Coin Lockout
(out) P6 : Chuck-a-luck (J7-9) - What is this? (mapped as solenoid #32 now);
(out) P7 : Flipper enable (J7-8) - Mapped as solenoid #16
*/
static WRITE_HANDLER(ppi0_pc_w) {
	int col = data & 0x07;			//Display column in bits 0-2
	//Active Display Digit Select not changed when value is 111 (0x07).
	if (col < 0x07) locals.disp_col = 7-col;
	coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x02) | ((data>>3) & 1);
	//Display Enabled only when this value is 0
	locals.disp_enable = !((data>>4)&1);

	if ((data>>7)&1) locals.solenoids |= 0x8000; else locals.solenoids &= ~0x8000;
	if ((data>>5)&1) locals.solenoids |= 0x40000000; else locals.solenoids &= ~0x40000000;
	if ((data>>6)&1) locals.solenoids |= 0x80000000; else locals.solenoids &= ~0x80000000;
}

/*
8255 PPI
U17
---
Port A:
-------
(out) P0-P3:
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)

***  MPU-1 GENERATION ***
(out) P4-P7P: Address Data
	  (1-16 Demultiplexed - Output are all active low)
	0) = NA
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-5) = Lamp Data 1-3
	6) = NA
	7) = Swicht Strobe 0
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Display Clock 4
	12-15) Dip Column Strobes

***  MPU-2 GENERATION***
Differences Summary: Extra Lamp & Switch Strobe, and 5 Strobes for BDU
Differences Details:
(out) P4-P7P: Address Data
	  (1-16 Demultiplexed - Output are all active low)
	3-6) = Lamp Data 1-4
	7) = Swicht Strobe 0 AND BDU - Clock 1
	8) = Switch Strobe 1 AND Display Clock 1 AND BDU - Clock 2
	9) = Switch Strobe 2 AND Display Clock 2 AND BDU - Clock 3
	10) = Switch Strobe 3 AND Display Clock 3 AND BDU - Clock 4
	11) = Switch Strobe 4 AND Display Clock 4 AND BDU - Clock 5

Port B:
-------
(in) P0-P7: Switch & Dip Returns

Port C:
-------
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Display Enable (Active Low)
(out) P5 : Spare output (J7-7)
(out) P6 : Chuck-a-luck (J7-9)
(out) P7 : Flipper enable (J7-8)
*/
static ppi8255_interface ppi8255_intf =
{
	1, 			/* 1 chip */
	{0},			/* Port A read */
	{ppi0_pb_r},	/* Port B read */
	{0},			/* Port C read */
	{ppi0_pa_w},	/* Port A write */
	{ppi0_pb_w},	/* Port B write */
	{ppi0_pc_w},	/* Port C write */
};

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpu_set_irq_line_and_vector(0, 0, state, Z80_VECTOR(0, state));
}

static z80ctc_interface ctc_intf =
{
	1,								/* 1 chip */
	{ 0 },							/* clock (filled in from the CPU 0 clock */
	{ NOTIMER_1 | NOTIMER_3 },		/* timer disables */
	{ ctc_interrupt },				/* interrupt handler */
	{ 0 },							/* ZC/TO0 callback */
	{ 0 },							/* ZC/TO1 callback */
	{ 0 }							/* ZC/TO2 callback */
};

static void GP_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
//  logerror("Zero cross\n");
  z80ctc_0_trg2_w(0, 1);
  z80ctc_0_trg2_w(0, 0);
}

static void GP_common_init(void) {
  /* init sound */
  sndbrd_0_init(core_gameData->hw.soundBoard, GP_SCPUNO, memory_region(GP_MEMREG_SCPU), NULL, NULL);
  memset(&locals, 0, sizeof(locals));

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /* init CTC */
  ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);

  locals.vblankCount = 1;
}

/*MPU-1 Generation Init*/
static MACHINE_INIT(GP1) {
	GP_common_init();
	locals.PORTA_WRITE = mpu1_pa_w;
}
/*MPU-2 Generation Init*/
static MACHINE_INIT(GP2) {
	GP_common_init();
	locals.PORTA_WRITE = mpu2_pa_w;
}

static MACHINE_STOP(GP) {
  /* exit sound */
  sndbrd_0_exit();
}

static Z80_DaisyChain GP_DaisyChain[] =
{
    {z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0},
    {0,0,0,-1}
};

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
static MEMORY_READ_START(GP_readmem)
	{ 0x0000, 0x2fff, MRA_ROM },
	{ 0x8c00, 0x8cff, MRA_RAM }, /*256B CMOS RAM - Battery Backed*/
	{ 0x8d00, 0x8d7f, MRA_RAM }, /*128B NMOS RAM*/
MEMORY_END

static MEMORY_WRITE_START(GP_writemem)
	{ 0x0000, 0x2fff, MWA_ROM },
	{ 0x8c00, 0x8cff, MWA_RAM, &generic_nvram, &generic_nvram_size}, /*256B CMOS RAM - Battery Backed*/
	{ 0x8d00, 0x8d7f, MWA_RAM }, /*128B NMOS RAM*/
MEMORY_END

static PORT_READ_START( GP_readport )
	{0x04,0x07, ppi8255_0_r },
	{0x08,0x0b, z80ctc_0_r  },
PORT_END

static PORT_WRITE_START( GP_writeport )
	{0x04,0x07, ppi8255_0_w },
	{0x08,0x0b, z80ctc_0_w  },
PORT_END

MACHINE_DRIVER_START(GP1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD_TAG("mcpu", Z80, 2457600)
  MDRV_CPU_CONFIG(GP_DaisyChain)
  MDRV_CPU_MEMORY(GP_readmem, GP_writemem)
  MDRV_CPU_PORTS(GP_readport,GP_writeport)
  MDRV_CPU_VBLANK_INT(GP_vblank, 1)
  MDRV_TIMER_ADD(GP_zeroCross, GP_ZCFREQ)
  MDRV_CORE_INIT_RESET_STOP(GP1,NULL,GP)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_DIPS(32)
  MDRV_SWITCH_UPDATE(GP)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SOUND_CMD(GP_soundCmd)
  MDRV_SOUND_CMDHEADING("GP")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP1S1)
  MDRV_IMPORT_FROM(GP1)
  MDRV_IMPORT_FROM(gpSSU1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2)
  MDRV_IMPORT_FROM(GP1)
  MDRV_CORE_INIT_RESET_STOP(GP2,NULL,GP)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2S1)
  MDRV_IMPORT_FROM(GP2)
  MDRV_IMPORT_FROM(gpSSU1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2S2)
  MDRV_IMPORT_FROM(GP2)
  MDRV_IMPORT_FROM(gpSSU2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2S4)
  MDRV_IMPORT_FROM(GP2)
  MDRV_IMPORT_FROM(gpSSU4)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2SM)
  MDRV_IMPORT_FROM(GP2)
  MDRV_IMPORT_FROM(gpMSU1)
  MDRV_DIAGNOSTIC_LEDH(2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(GP2SM3)
  MDRV_IMPORT_FROM(GP2)
  MDRV_IMPORT_FROM(gpMSU3)
  MDRV_DIAGNOSTIC_LEDH(2)
MACHINE_DRIVER_END
