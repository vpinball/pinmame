/* Game Plan Pinball
   -----------------

   Hardware: 

   CPU: Z80 & Z80CTC (Controls Interrupt Generation & Zero Cross Detection)
   I/O: 8255

*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "machine/8255ppi.h"
#include "core.h"
#include "gp.h"

#if 0
#define mlogerror printf
#else
#define mlogerror logerror
#endif

#define GP_VBLANKFREQ    60 /* VBLANK frequency */
#define GP_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define GP_ZCFREQ        85 /* Zero cross frequency */

static WRITE_HANDLER(GP_soundCmd) {}

static struct {
  int p0_a,p0_c;
  int bcd[6];
  int lampaddr;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
  int initDone;
  void *zctimer;
  int swCol;
  int disp_enable;
  int disp_col;
  int last_clk;
} locals;

static void GP_exit(void);
static void GP_nvram(void *file, int write);


static void GP_dispStrobe(int mask) {
  int digit = locals.disp_col;
  int ii,jj;
  logerror("mask = %x, digit = %x (%x,%x,%x,%x,%x,%x)\n",mask, digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4],locals.bcd[5]);
  ii = digit;
  jj = mask;
  ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
}

static void GP_lampStrobe(int lampadr, int lampdata) {
  UINT8 *matrix;
  int bit;
  if (lampadr != 0x0f) {
    lampdata ^= 0x0f;
    matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)];
    bit = 1<<(lampadr & 0x07);
    //DBGLOG(("adr=%x data=%x\n",lampadr,lampdata));
    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}


static void GP_UpdateSolenoids (int bank, int soldata) {
  coreGlobals.pulsedSolState = 0;
  if (!bank)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(soldata & 0x0f)) & 0x7fff;
  soldata ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((soldata & 0xf0)<<12);
  locals.solenoids |= (soldata & 0xf0)<<12;
}

static int GP_vblank(void) {
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
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
  return ignore_interrupt();
}

static void GP_updSw(int *inports) {
  if (inports) {
	coreGlobals.swMatrix[0] = (inports[GP_COMINPORT]>>9) & 0x01;
	coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0xf7)) |
                              ((inports[GP_COMINPORT] & 0xff) & 0xf7);
	coreGlobals.swMatrix[4] = (coreGlobals.swMatrix[4] & (~0x02)) |
                              ((inports[GP_COMINPORT]>>7) & 0x02);
  }
  /*-- Diagnostic buttons on CPU board --*/
  //if (core_getSw(GP_SWSOUNDDIAG)) cpu_set_nmi_line(GP_SCPU1NO, PULSE_LINE);
}

/*
PORT B READ
(in) P0-P7: Switch & Dip Returns
*/
static READ_HANDLER(ppi0_pb_r) {
	logerror("PB_R: \n");
	if (locals.p0_a == 0x0c) return core_getDip(0); // DIP#1 1-8
	if (locals.p0_a == 0x0d) return core_getDip(1); // DIP#2 9-16
	if (locals.p0_a == 0x0e) return core_getDip(2); // DIP#3 17-24
	if (locals.p0_a == 0x0f) return core_getDip(3); // DIP#4 25-32
	//If Strobe 0 - Return switches plus Dip #5 - #33
//	if (locals.p0_a == 0x07) 
//		return ~(core_getSwCol(locals.swCol) | (core_getDip(4) & 1)); // DIP#5 33
//	else
		logerror("(%x): read sw col: %x = %x\n",cpu_getpreviouspc(),locals.swCol, core_getSwCol(locals.swCol));
		return core_getSwCol(locals.swCol);
}

/*
PORT A WRITE
(out) P0-P3: 
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)

(out) P4-P7P: Address Data 
  (1-16 Demultiplexed - Output are all active low)
	0) = NA
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-6) = Lamp Data 1-4
	7) = Switch Strobe 0 AND Dip Switch #33 Enable
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Display Clock 4
	12-15) Dip Column Strobes
*/
static WRITE_HANDLER(ppi0_pa_w) {
	int addrdata = data & 0x0f;	//Take P0-P3
	int tmpdata =  data>>4;		//Take P4-P7
	int disp_clk = tmpdata - 8;	//Track Display Clock line
	locals.p0_a = data;

	/*Enabling Solenoids?*/
	if(tmpdata>0 && tmpdata<3) {
		if(addrdata !=0xf)
			mlogerror("sol en: %x addr %x \n",tmpdata-1,addrdata);
		GP_UpdateSolenoids(tmpdata-1,~addrdata);
	}

	/*Updating Lamps?*/
	if(tmpdata>2 && tmpdata<6) {
		GP_lampStrobe(~addrdata,tmpdata);
	}

	/*Strobing Switches?*/
	if(tmpdata>6 && tmpdata<12) {	
		locals.swCol = 1<<(tmpdata-7);
	}

#if 1
	/*Strobing Digit Displays?
	  ------------------------
	  Clock lines go low on selection, but are clock data in, on lo->hi transition
	  So we track the last clock line to go lo, and when it changes, we write in bcd data
	*/
	if(disp_clk != locals.last_clk) {
		locals.bcd[locals.last_clk] = addrdata;
		if(locals.disp_enable)
			GP_dispStrobe(locals.last_clk);
		//locals.last_clk = disp_clk;
	}
#endif

	if(tmpdata>7 && tmpdata<12) {
		locals.last_clk = disp_clk;
	}

	logerror("PA_W: P4-7 %x = %d\n",tmpdata,tmpdata);
	logerror("PA_W: P0-3 %x = %d\n",addrdata,addrdata);
}

/*
PORT C WRITE
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Display Enable (Active Low)
(out) P5 : NA (J7-7)
(out) P6 : Chuck-a-luck? (J7-9)
(out) P7 : Flipper? (J7-8)
*/
static int ct = 0;
static WRITE_HANDLER(ppi0_pc_w) {
	int tmp = locals.disp_enable;
	int col = locals.p0_c & 0x07;
	locals.p0_c = data;
	locals.disp_enable = !((data>>4)&1);
	//Active Display Digit Select not changed when value is 111 (0x07).
	if(col < 7)
		locals.disp_col = 6-col;
	logerror("disp_enable = %x\n",locals.disp_enable);
	logerror("col: %x \n",col);
	logerror("PC_W: %x\n",data);
	locals.diagnosticLed = (data>>3)&1;
	if((data>>3)&1) 
	{	
		ct++;
		logerror("LED #%x\n",ct);
	}
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

(out) P4-P7P: Address Data 
	  (1-16 Demultiplexed - Output are all active low)
	0) = NA?
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-6) = Lamp Data 1-4
	7) = Swicht Strobe 0 AND Dip Switch #33 Enable
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Switch Strobe 4 AND Display Clock 4
	12-15) Dip Column Strobes

Port B:
-------
(in) P0-P7: Switch & Dip Returns

Port C:
-------
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Display Enable (Active Low)
(out) P5 : NA (J7-7)
(out) P6 : Chuck-a-luck? (J7-9)
(out) P7 : Flipper? (J7-8)
*/
static ppi8255_interface ppi8255_intf =
{
	1, 			/* 1 chip */
	{0},			/* Port A read */
	{ppi0_pb_r},	/* Port B read */
	{0},			/* Port C read */
	{ppi0_pa_w},	/* Port A write */
	{0},			/* Port B write */
	{ppi0_pc_w},	/* Port C write */
};

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpu_cause_interrupt (0, Z80_VECTOR(0,state) );
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

static int GP_irq(void) {
  return ignore_interrupt();
}

static core_tData GPData = {
  33, /* 33 Dips */
  GP_updSw, 1, GP_soundCmd, "GP",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void GP_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
  logerror("Zero cross\n");
  z80ctc_0_trg2_w(0, 1);
  z80ctc_0_trg2_w(0, 0);
}

static void GP_init(void) {
  if (locals.initDone) CORE_DOEXIT(GP_exit);

  if (core_init(&GPData)) return;
  memset(&locals, 0, sizeof(locals));

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /* init CTC */
  ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
  z80ctc_init(&ctc_intf);

  //if (coreGlobals.soundEn) GP_soundInit();
  locals.vblankCount = 1;
  locals.zctimer = timer_pulse(TIME_IN_HZ(GP_ZCFREQ),0,GP_zeroCross);

  locals.initDone = TRUE;
}

static void GP_exit(void) {
#ifdef PINMAME_EXIT
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
#endif
  //if (coreGlobals.soundEn) GP_soundExit();
  core_exit();
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
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0xffff, MRA_RAM }, /*256K RAM - BUT MIRRORED ALL OVER THE PLACE*/
MEMORY_END

static MEMORY_WRITE_START(GP_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM }, /*256K RAM - BUT MIRRORED ALL OVER THE PLACE*/
MEMORY_END

static PORT_READ_START( GP_readport )
	{0x04,0x07, ppi8255_0_r },
	{0x08,0x0b, z80ctc_0_r  },
PORT_END

static PORT_WRITE_START( GP_writeport )
	{0x04,0x07, ppi8255_0_w },
	{0x08,0x0b, z80ctc_0_w  },
PORT_END


/*MPU-1*/
struct MachineDriver machine_driver_GP1 = {
  {{  CPU_Z80, 2000000, /* 2Mhz */
      GP_readmem, GP_writemem, GP_readport, GP_writeport,
      GP_vblank, 1, GP_irq, GP_IRQFREQ, GP_DaisyChain
  }},
  GP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, GP_init, CORE_EXITFUNC(GP_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  GP_nvram
};

/*MPU-2*/
struct MachineDriver machine_driver_GP2 = {
  {{  CPU_Z80, 2000000, /* 2Mhz */
      GP_readmem, GP_writemem, NULL, NULL,
      GP_vblank, 1, GP_irq, GP_IRQFREQ
  }},
  GP_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, GP_init, CORE_EXITFUNC(GP_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  GP_nvram
};


/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void GP_nvram(void *file, int write) {
  //core_nvram(file, write, GP_CMOS, 0x100,0xff);
}
