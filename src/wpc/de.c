/*DE.C - Includes support for
         Alpha Numeric Games And 1st Sound Board Generations ONLY */

/************************************************************************************************
  Data East Pinball -
  Hardware from 1987-1994
  CPU hardware is very similar to Williams System 11 Hardware!

  CPU Boards:
	1) CPU Rev 1 : Ram size = 2k (0x0800)	(Early Laser War Only)
	2) CPU Rev 2 : Ram size = 8k (0x2000)	(Later Laser War to Phantom of the Opera)
	3) CPU Rev 3 : CPU Controlled solenoids	(Back to the Future to Jurassic Park)
	4) CPU Rev 3b: Printer option			(Last Action Hero to Batman Forever)

  Display Boards:
	1) 520-5004-00: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Numeric), 1 X 4 Digit (7 Seg. Numeric)
	   (Used in Laser War Only)

	2) 520-5014-01: 2 X 7 Digit (16 Seg. Alphanumeric), 2 X 7 Digit (7 Seg. Alphanumeric)
	   (Secret Service to Playboy)
	
	3) 520-5030-00: 2 X 16 Digit (16 Seg Alphanumeric)
		(MNF to Simpsons)
	
	4) 520-5042-00: 128X16 DMD - z80 CPU + integrated controller.
	   (Checkpoint to Hook)

	5) 520-5505 Series: 128X32 DMD - m6809 CPU + separate controller board
		a) -00 generation: (Lethal Weapon to Last Action Hero)
		b) -01 generation: (Tales From the Crypt to Guns N Roses)

	6) 520-5092-01: 192X64 DMD - 68000 CPU + separate controller board
	   (Maveric to Batman Forever)

   Sound Board Revisions: 
	1) 520-5002 Series: M6809 cpu, YM2151, MSM5205, hc4020 for stereo decoding.
		a) -00 generation, used 27256 eproms (only Laser War)
	    b) -02 generation, used 27256 & 27512 eproms (Laser War - Back to the Future)
		c) -03 generation, used 27010 voice eproms (Simpsons - Checkpoint)

	2) 520-5050-01 Series:	M6809 cpu, BSMT2000 16 bit stereo synth+dac, 2 custom PALS
		a) -01 generation,	used 27020 voice eproms (Batman - Lethal Weapon 3)
		b) -02 generation,	used 27040 voice eproms (Star Wars - J.Park)
		c) -03 generation,	similar to 02, no more info known (LAH - Maverick)
	3) 520-5077-00 Series:	??  (Tommy to Frankenstein)
	4) 520-5126-xx Series:	??	(Baywatch to Batman Forever)

*************************************************************************************************/
#include <stdarg.h>
//#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "machine/6821pia.h"
#include "core.h"
#include "dedmd.h"
#include "de1sound.h"
#include "de.h"
#include "snd_cmd.h"

#define DE_SCPU1			1   /*Sound CPU Always #1*/
#define DE_VBLANKFREQ      50   /* VBLANK frequency */
#define DE_IRQFREQ       1075   /* IRQ Frequency (Measured on real game)*/

static int UsingSound = 0;

static void de_init(void);
static void de_exit(void);
static void de_nvram(void *file, int write);

/*----------------
/  Local varibles
/-----------------*/
struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  int    lampRow, lampColumn;
  int    digSel;
  int    diagnosticLed;
  int    swCol;
  int    ssEn;
  int    mainIrq;
} delocals;

static void de_piaMainIrq(int state) {
  delocals.mainIrq = state;
  cpu_set_irq_line(DE_CPUNO, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
  /*What is there here for, TOM?*/
  pia_set_input_ca1(2, state?0:!core_getSwSeq(DE_SWADVANCE));
  pia_set_input_cb1(2, state?0:!core_getSwSeq(DE_SWUPDN));
}

static int de_irq(void) {
  /*What is there here for, TOM?*/
  pia_set_input_ca1(2, 1);
  pia_set_input_cb1(2, 1);

  if (delocals.mainIrq == 0) /* Don't send IRQ if already active */
    cpu_set_irq_line(DE_CPUNO, M6808_IRQ_LINE, PULSE_LINE);

  return 0;
}

static int de_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  delocals.vblankCount += 1;
  /*-- lamps --*/
  if ((delocals.vblankCount % DE_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, delocals.lampMatrix, sizeof(delocals.lampMatrix));
    memset(delocals.lampMatrix, 0, sizeof(delocals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((delocals.vblankCount % DE_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = delocals.solenoids;

	/*If Mux. Solenoid 10 is firing, adjust solenoid # to 24!*/
    if((delocals.solenoids & CORE_SOLBIT(10)))
      coreGlobals.solenoids = (delocals.solenoids & 0x00ffff00) | (delocals.solenoids<<24);

    if (delocals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    delocals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((delocals.vblankCount % DE_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, delocals.segments, sizeof(coreGlobals.segments));
    memcpy(delocals.segments, delocals.pseg, sizeof(delocals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = delocals.diagnosticLed;
    delocals.diagnosticLed = 0;
  }
  core_updateSw(delocals.ssEn); /* Is flipper enabled? */
  return 0;
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia1a_w) { core_setLamp(delocals.lampMatrix, delocals.lampColumn, delocals.lampRow = ~data); }
static WRITE_HANDLER(pia1b_w) { core_setLamp(delocals.lampMatrix, delocals.lampColumn = data, delocals.lampRow); }

/*-- Jumper J8 --*/
static READ_HANDLER (pia2a_r)   { return 1; }

/*-----------------
/ Display handling
/-----------------*/
static WRITE_HANDLER(pia2a_w) {
  delocals.digSel = data & 0x0f;
  delocals.diagnosticLed = (data>>4);	/*LED = PA_4*/
}

static WRITE_HANDLER(pia2b_w) {
  if (core_gameData->gen & (DE_ALPHA1 | DE_ALPHA2))
    delocals.segments[1][delocals.digSel].lo |= delocals.pseg[1][delocals.digSel].lo = data;
  else
    delocals.segments[1][delocals.digSel].hi |= delocals.pseg[1][delocals.digSel].hi = data;
}
static WRITE_HANDLER(pia5a_w) {
  if (core_gameData->gen & DE_ALPHA3)
    delocals.segments[1][delocals.digSel].lo |= delocals.pseg[1][delocals.digSel].lo = data;
}


static WRITE_HANDLER(pia3a_w) {
	/*Alpha Segments*/
	delocals.segments[0][delocals.digSel].lo |= delocals.pseg[0][delocals.digSel].lo = data;
}
static WRITE_HANDLER(pia3b_w) {
  /*Alpha Segments*/  
  delocals.segments[0][delocals.digSel].hi |= delocals.pseg[0][delocals.digSel].hi = data;
  //logerror("[0].hi = %x\n",data);
}

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (delocals.ssEn & (~data & 1)) { coreGlobals.pulsedSolState |= bit;  delocals.solenoids |= bit; }
  else                               coreGlobals.pulsedSolState &= ~bit;
}
static WRITE_HANDLER(pia0b_w)   {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
  delocals.solenoids |= (data<<8);
}
static WRITE_HANDLER(latch2200) {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
  delocals.solenoids |= data;
}

static WRITE_HANDLER(pia0cb2_w) { delocals.ssEn = !data;}
static WRITE_HANDLER(pia1ca2_w) { setSSSol(data, 3); }		// Solenoid #20
static WRITE_HANDLER(pia1cb2_w) { setSSSol(data, 4); }		// Solenoid #21
static WRITE_HANDLER(pia3ca2_w) { setSSSol(data, 5); }		// Solenoid #22
static WRITE_HANDLER(pia3cb2_w) { setSSSol(data, 1); }		// Solenoid #18
static WRITE_HANDLER(pia4ca2_w) { setSSSol(data, 0); }		// Solenoid #17
static WRITE_HANDLER(pia4cb2_w) { setSSSol(data, 2); }		// Solenoid #19

/*---------------
/ Switch reading
/----------------*/
static READ_HANDLER(pia4a_r)  { return core_getSwCol(delocals.swCol); }
static WRITE_HANDLER(pia4b_w) { delocals.swCol = data; }

/*----------------
/Sound Commands
/-----------------*/
static WRITE_HANDLER(pia5b_w)  {
	//logerror("soundlatch_w %x\n",data);
	if(UsingSound) {
		soundlatch_w(0,data);
		snd_cmd_log(data);
	}
}
static WRITE_HANDLER(pia5cb2_w) {
//	logerror("FIRQ Enable?:pia5cb2_w %x\n",data);
	if(UsingSound)
		if((data&0x01)==0)
			cpu_cause_interrupt(DE_SCPU1,M6809_INT_FIRQ);
}

/*SHOULD be Unsused*/
static WRITE_HANDLER(pia0ca2_w) {}
//static WRITE_HANDLER(pia3ca2_w) {logerror("pia3ca2_w\n");}
//static WRITE_HANDLER(pia3cb2_w) {logerror("pia3cb2_w\n");}

static READ_HANDLER (pia3ca1_r) {;return 0x00;}
static READ_HANDLER (pia3cb1_r) {return 0x00;}
static READ_HANDLER (pia3ca2_r) {return 0x00;}
static READ_HANDLER (pia3cb2_r) {return 0x00;}
static WRITE_HANDLER(pia2ca2_w) {}
static WRITE_HANDLER(pia2cb2_w) {}
static WRITE_HANDLER(pia5ca2_w) {}
static READ_HANDLER (pia5a_r)  {return 0x00;}

static READ_HANDLER (pia2ca1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? 0:!core_getSwSeq(DE_SWADVANCE) ; }
static READ_HANDLER (pia2cb1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? 0:!core_getSwSeq(DE_SWUPDN)    ; }

struct pia6821_interface de_pia_intf[] = {
{/* PIA 0 (2100) Chip 5F*/
 /* PA0 - PA7 Not Used */
 /* PB0 - PB7 Solenoid 9-16*/
 /* CA2       Not Used */
 /* CB2       Enable Special Solenoids? */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia0b_w, pia0ca2_w, pia0cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 1 (2400) Chip 11D */
 /* PA0 - PA7 Lamp Matrix Strobe */
 /* PB0 - PB7 Lamp Matrix Return */
 /* CA2       SS3  */
 /* CB2       SS5  */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 2 (2800) Chip 11B */
 /* PA0 - PA3 Digit Select 1-16 */
 /* PA4       Diagnostic LED */
 /* PA5 - PA6 NC */
 /* PA7       (I) Jumper J8 */
 /* PB0 - PB7 Digit BCD */
 /* CA1       (I) Diagnostic Advance */
 /* CB1       (I) Diagnostic Up/dn */
 /* CA2       N/A */
 /* CB2       N/A */
 /* in  : A/B,CA/B1,CA/B2 */ pia2a_r, 0, pia2ca1_r, pia2cb1_r, 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 3 (2c00) Chip 9B*/
 /* PA0 - PA7 Display Data (h,j,k,m,n,p,r,dot) */
 /* PB0 - PB7 Display Data (a,b,c,d,e,f,g,com) */
 /* CA1       CN25-Pin 15-Not Used*/
 /* CB1       CN25-Pin 11-Not Used*/
 /* CA2       SS4 */
 /* CB2       SS1 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, pia3ca1_r, pia3cb1_r, pia3ca2_r, pia3cb2_r,
 /* out : A/B,CA/B2       */ pia3a_w, pia3b_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 4 (3000) Chip 8H*/
 /* PA0 - PA7 Switch Input (row) */
 /* PB0 - PB7 Switch Drivers (column) */
 /* CA1/CB1   GND */
 /* CA2       SS6 */
 /* CB2       SS2 */
 /* in  : A/B,CA/B1,CA/B2 */ pia4a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia4b_w, pia4ca2_w, pia4cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 5 (3400) Chip 7B*/
 /* PA0 - PA7 Display Data' (h,j,k ,m,n,p,r,dot) (Alpha Type 3 Only!) */
 /* PB0 - PB7 Sound Data */
 /* CA1       CN25-Pin 11-Not Used*/
 /* CB1       CN25-Pin 12-YM2151 - CT2 (Handled by YM2151 port handler)*/
 /* CA2       CN25-Pin 18-Not Used*/
 /* CB2       CN25-Pin 13-ls74->FIRQ 6809*/
 /* in  : A/B,CA/B1,CA/B2 */ pia5a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia5a_w, pia5b_w, pia5ca2_w, pia5cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq
}};

static void de_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[DE_COMINPORT] & 0x7f00)>>8;
    coreGlobals.swMatrix[1] = inports[DE_COMINPORT];
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSwSeq(DE_SWCPUDIAG))   cpu_set_nmi_line(DE_CPUNO, PULSE_LINE);
  if(UsingSound)
	  if (core_getSwSeq(DE_SWSOUNDDIAG)) cpu_set_nmi_line(DE_SCPU1, PULSE_LINE);

  /*-- coin door switches --*/
  pia_set_input_ca1(2, !core_getSwSeq(DE_SWADVANCE));
  pia_set_input_cb1(2, !core_getSwSeq(DE_SWUPDN));

  if(core_getSwSeq(DE_SWADVANCE))
          core_textOutf(40, 20, BLACK, "%-7s","A-Up");
  else
          core_textOutf(40, 20, BLACK, "%-7s","A-Down");

  /* Show Status of Auto/Manual Switch */
  if(core_getSwSeq(DE_SWUPDN))
          core_textOutf(40, 30, BLACK, "%-7s","Up");
  else
          core_textOutf(40, 30, BLACK, "%-7s","Down");
}


static WRITE_HANDLER(de_sndCmd_w) {
	pia5b_w(offset,data);
	pia5cb2_w(offset,data);
}

static core_tData deData = {
  0, /* No DIPs */
  de_updSw,
  1,
  de_sndCmd_w,
  "de"
};

static void de_init(void) {
  /* init PIAs */
  int ii;
  if (delocals.initDone)
    de_exit();
  delocals.initDone = TRUE;

  for (ii = 0; ii < sizeof(de_pia_intf)/sizeof(de_pia_intf[0]); ii++)
    pia_config(ii, PIA_STANDARD_ORDERING, &de_pia_intf[ii]);

  if (core_init(&deData))
	  return;

  /*Sound Enabled?*/
  if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate)
  {
	  UsingSound=1;
	  DE_sinit(DE_SCPU1);
  }
  else
	  UsingSound=0;

  pia_reset();
}

static void de_exit(void) {
  core_exit();
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(de_readmem)
  { 0x0000, 0x1fff, MRA_RAM},
  { 0x2100, 0x2103, pia_0_r},
  { 0x2400, 0x2403, pia_1_r},
  { 0x2800, 0x2803, pia_2_r},
  { 0x2c00, 0x2c03, pia_3_r},
  { 0x3000, 0x3003, pia_4_r},
  { 0x3400, 0x3403, pia_5_r},
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(de_writemem)
  { 0x0000, 0x07ff, MWA_RAM }, /* CMOS */
  { 0x0800, 0x1fff, MWA_RAM }, /* CMOS */
  { 0x2100, 0x2103, pia_0_w},
  { 0x2200, 0x2200, latch2200},
  { 0x2400, 0x2403, pia_1_w},
  { 0x2800, 0x2803, pia_2_w},
  { 0x2c00, 0x2c03, pia_3_w},
  { 0x3000, 0x3003, pia_4_w},
  { 0x3400, 0x3403, pia_5_w},
MEMORY_END

struct MachineDriver machine_driver_DE_AS = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1,
      de_irq, DE_IRQFREQ
    },
    DE_SOUNDCPU
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de_init, CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{ DE_SOUND },
  de_nvram
};

/*This driver uses NO Display - Usde Only for the DE Test Chip*/
struct MachineDriver machine_driver_DE_NO = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1,
      de_irq, DE_IRQFREQ
    }
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de_init, CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0,{{0}},
  de_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/ The RAM size was changed from 2K to 8K starting with
/ CPU Rev #2
/-------------------------------------------------*/
void de_nvram(void *file, int write) {
  int length = (core_gameData->gen & DE_CPUREV1)?0x0800:0x2000;
  if (write)  /* save nvram */
    osd_fwrite(file, memory_region(DE_MEMREG_CPU), length);
  else if (file) /* load nvram */
    osd_fread(file, memory_region(DE_MEMREG_CPU), length);
  else        /* first time */
    memset(memory_region(DE_MEMREG_CPU), 0xff, length);
}
