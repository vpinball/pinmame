/*Game Plan Pinball*/
#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "machine/z80fmly.h"
#include "machine/8255ppi.h"
#include "core.h"
#include "gp.h"

#define mlogerror printf

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
} locals;

static void GP_exit(void);
static void GP_nvram(void *file, int write);


static void GP_dispStrobe(int mask) {
  int digit = locals.p0_c & 0x07;
  int ii,jj;
  //DBGLOG(("digit = %x (%x,%x,%x,%x,%x,%x)\n",digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4],locals.bcd[5]));
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
        if (dispMask & 0x01)
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg[locals.bcd[jj]];
    }
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
    coreGlobals.swMatrix[0] = (inports[GP_COMINPORT]>>10) & 0x07;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x60)) |
                              ((inports[GP_COMINPORT]<<5) & 0x60);
      coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x87)) |
                                ((inports[GP_COMINPORT]>>2) & 0x87);
  }
  /*-- Diagnostic buttons on CPU board --*/
  //if (core_getSw(GP_SWCPUDIAG))  cpu_set_nmi_line(0, PULSE_LINE);
  //if (core_getSw(GP_SWSOUNDDIAG)) cpu_set_nmi_line(GP_SCPU1NO, PULSE_LINE);
  /*-- coin door switches --*/
  //pia_set_input_ca1(0, !core_getSw(GP_SWSELFTEST));
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
	if (locals.p0_a == 0x07) return core_getDip(4); // DIP#5 33
	return core_getSwCol(locals.swCol);
}

/*
PORT A WRITE
(out) P0-P3: 
	  (4-16 Demultiplexed - Output are all active low)
	0) = NA?
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-6) = Lamp Data 1-4
	7) = Swicht Strobe 0 AND Dip Switch #33 Enable
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Display Clock 4
	12-15) Dip Column Strobes
(out) P4-P7P: Address Data 
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)
*/
static WRITE_HANDLER(ppi0_pa_w) {
	int tmpdata = data & 0x0f;	//Take P0-P3
	int addrdata = data & 0xf0; //Take P4-P7
	locals.p0_a = data;

	/*Enabling Solenoids?*/
	if(tmpdata>0 && tmpdata<3) {
		GP_UpdateSolenoids(tmpdata-1,addrdata);
	}

	/*Updating Lamps?*/
	if(tmpdata>2 && tmpdata<6) {
		GP_lampStrobe(addrdata,tmpdata);
	}

	/*Strobing Switches?*/
	if(tmpdata>6 && tmpdata<11) {	
		locals.swCol = tmpdata-7;
	}

	/*Strobing Digit Displays?*/
	if(tmpdata>7 && tmpdata<12) {	
		int bcdLoad = ~addrdata & 0x0f;
		int ii;
		for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
			if (bcdLoad & 0x01) locals.bcd[ii] = addrdata>>4;
		GP_dispStrobe(tmpdata);
	}
	logerror("PA_W: %x\n",data);
}

/*
PORT C WRITE
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Enable (J3-22)?
(out) P5 : NA (J7-7)
(out) P6 : Chuck-a-luck? (J7-9)
(out) P7 : Flipper? (J7-8)
*/
static WRITE_HANDLER(ppi0_pc_w) {
	locals.p0_c = data;
	logerror("PC_W: %x\n",data);
	locals.diagnosticLed = (data>>3)&1;
}

/*
8255 PPI
U17
---
Port A:
-------
(out) P0-P3: 
	  (4-16 Demultiplexed - Output are all active low)
	0) = NA?
	1) = Solenoid Address 1-4 Enable
	2) = Solenoid Address 5-8 Enable
	3-6) = Lamp Data 1-4
	7) = Swicht Strobe 0 AND Dip Switch #33 Enable
	8) = Switch Strobe 1 AND Display Clock 1
	9) = Switch Strobe 2 AND Display Clock 2
	10) = Switch Strobe 3 AND Display Clock 3
	11) = Display Clock 4
	12-15) Dip Column Strobes
(out) P4-P7P: Address Data 
	a) Lamp Address 1-4
	b) Display BCD Data? (Shared with Lamp Address 1-4)
	c) Solenoid Address 1-4 (must be enabled from above)
	d) Solenoid Address 5-8 (must be enabled from above)

Port B:
-------
(in) P0-P7: Switch & Dip Returns

Port C:
-------
(out) P0-P2 : 3-8 Demultiplexed Digit Selects (1-7)
(out) P3 : LED
(out) P4 : Enable (J3-22)?
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
static int eatfirstirq=1;
static void ctc_interrupt (int state)
{
	if(eatfirstirq)
		eatfirstirq=0;
	else {
		mlogerror("%x: IRQ: state=%x\n", cpu_getpreviouspc(),state);
		cpu_cause_interrupt (0, Z80_VECTOR(0,state) );
	}
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
#if 0
  /*- toggle zero/detection circuit-*/
  logerror("Zero cross\n",);
  z80ctc_0_trg2_w(0, 1);
  z80ctc_0_trg2_w(0, 0);
#endif
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
