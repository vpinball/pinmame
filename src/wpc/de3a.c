/*#include <windows.h>*/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "machine/6821pia.h"
#include "core.h"
#include "de1sound.h"
#include "dedmd.h"
#include "de.h"
#include "de2.h"
#include "vidhrdw/crtc6845.h"
#include "snd_cmd.h"

#define DE_DCPU1			1	/*DMD CPU IS #1*/
#define DE_SDCPU1			2   /*DMD SOUND GEN 1 CPU IS #2*/

#define DE_VBLANKDIV		4
#define DE_VBLANKFREQ     900 //50   /* VBLANK frequency */
#define DE_IRQFREQ       1000   /* IRQ Frequency */
#define DE_DMDFIRQFREQ	  113   /* FIRQ Handler */

static void de_init(void);
static void de2_init(void);
static void de_exit(void);
static void de_nvram(void *file, int write);
static void dmdfirq(int data);

static int UsingSound = 0;

/*----------------
/  Local varibles
/-----------------*/
static struct {
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
  void   *dmdfirqtimer;
} delocals;

static struct {
  int	 busy;		//Busy Signal back to main cpu
  int    status;	//Status line back to main cpu
  int    dmd_latch;	//DMD Command Latch from main cpu
  int	 za0;		//Bank Switch Lines
  int	 xa0;		// ""
  int	 xa1;		// ""
  int	 xa2;		// ""
  int	 xa3;		// ""
  int	 xa4;		// ""
  int	 xa5;		// ""
  int	 xa6;		// ""
  int	 xa7;		// ""
} de_dmdlocals;


static void de_piaMainIrq(int state) {
  delocals.mainIrq = state;
  cpu_set_irq_line(DE_CPUNO, M6808_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);

  //pia_set_input_ca1(2, state?0:!core_getSw(DE_SWADVANCE));
  //pia_set_input_cb1(2, state?0:!core_getSw(DE_SWUPDN));
}

static int de_irq(void) {
  //pia_set_input_ca1(2, 1);
  //pia_set_input_cb1(2, 1);

  if (delocals.mainIrq == 0) /* Don't send IRQ if already active */
    cpu_set_irq_line(DE_CPUNO, M6808_IRQ_LINE, PULSE_LINE);

  return 0;
}

static int de_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/

  delocals.vblankCount = (delocals.vblankCount+1) % 16;

  if ((delocals.vblankCount % (DE_VBLANKDIV+4)) == 0) {
	/*Fire an FIRQ - We are approximating this since, a custom PAL actually fires it*/
	cpu_set_irq_line(DE_DCPU1, M6809_FIRQ_LINE,PULSE_LINE);	
	//logerror("firq\n");
  }

  //delocals.vblankCount += 1;

  /*-- lamps --*/
  if ((delocals.vblankCount % (DE_VBLANKDIV*DE_LAMPSMOOTH)) == 0) {
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
  if ((delocals.vblankCount % (DE_VBLANKDIV*DE_DISPLAYSMOOTH)) == 0) {
    memcpy(coreGlobals.segments, delocals.segments, sizeof(coreGlobals.segments));
    memcpy(delocals.segments, delocals.pseg, sizeof(delocals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = delocals.diagnosticLed;
    delocals.diagnosticLed = 0;
  }
  core_updateSw(delocals.ssEn); /* Is flipper Enabled? */
  return 0;
}

/*---------------
/  Lamp handling
/----------------*/
static WRITE_HANDLER(pia1a_w) { core_setLamp(delocals.lampMatrix, delocals.lampColumn, delocals.lampRow = ~data); }
static WRITE_HANDLER(pia1b_w) { core_setLamp(delocals.lampMatrix, delocals.lampColumn = data, delocals.lampRow); }

/*------------
/  Solenoids
/-------------*/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (delocals.ssEn & (~data & 1)) { coreGlobals.pulsedSolState |= bit;  delocals.solenoids |= bit; }
  else                               coreGlobals.pulsedSolState &= ~bit;
}
/*Solenoids 9-16*/
static WRITE_HANDLER(pia0b_w)   {
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
  delocals.solenoids |= (data<<8);
}
/*Solenoids 1-8*/
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

/*-- Jumper J8 --*/
static READ_HANDLER (pia2a_r)   { logerror("pia2a_r\n"); return 1; }

/* - Feeds Chip 4H - 74154 1-16 Active Low output multiplexer
     (Also, PA-4 feeds PIA LED - it's inverted)
     data = 0x01, CN1-Pin 7 (Magnets) goes low
     data = 0x04, CN2-Pin 1 (Enable) goes low    */
static WRITE_HANDLER(pia2a_w) {
	delocals.diagnosticLed = ~(data>>4);	/*LED = PA_4*/
	logerror("pia2a_w: %x\n",data);
}
static READ_HANDLER(pia2b_r) { logerror("pia2b_r\n"); return 0; }

/* - CN3 Printer Data Lines (& Magnets on GNR)
	 data = 0x01, CN3-Pin 9 (Magnet 3?)
	 data = 0x02, CN3-Pin 8 (Magnet 2?)
	 data = 0x04, CN3-Pin 7 (Magnet 1?)       */
static WRITE_HANDLER(pia2b_w) { logerror("pia2b_w: %x\n",data); }
static WRITE_HANDLER(pia5a_w) { logerror("pia5a_w\n"); }

/*----------------
/Sound Commands
/-----------------*/
static WRITE_HANDLER(pia5b_w)  {
	//logerror("soundlatch_w %x\n",data);
	//if(UsingSound)
	{
	//	soundlatch_w(0,data);
		snd_cmd_log(data);
	}
}
static WRITE_HANDLER(pia5cb2_w) {
//	logerror("FIRQ Enable?:pia5cb2_w %x\n",data);
	if(UsingSound)
		if((data&0x01)==0)
			cpu_cause_interrupt(DE_SDCPU1,M6809_INT_FIRQ);
}

/*SHOULD be Unsused*/
static WRITE_HANDLER(pia0ca2_w) {logerror("pia0ca2_w\n");}
static READ_HANDLER (pia3ca1_r) {return 0x00;}
static READ_HANDLER (pia3cb1_r) {return 0x00;}
static READ_HANDLER (pia3ca2_r) {return 0x00;}
static READ_HANDLER (pia3cb2_r) {return 0x00;}
static WRITE_HANDLER(pia2ca2_w) { logerror("Comma 3+4 %d\n",data); }
static WRITE_HANDLER(pia2cb2_w) { logerror("Comma 1+2 %d\n",data); }
static WRITE_HANDLER(pia5ca2_w) { logerror("pia5ca2_w %x\n",data); }

static READ_HANDLER (pia2ca1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSw(DE_SWADVANCE) : 0; }
static READ_HANDLER (pia2cb1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSw(DE_SWUPDN)    : 0; }

/************************************************/
/*********** DMD HANDLING BELOW *****************/
/************************************************/

/*Read Status & Busy from DMD (128x16 Only!)*/
static READ_HANDLER(pia5a_r)
{
	logerror("pia5a_r - should not be called!\n");
	return 0;
}

static void switchbank(void)
{
	int	addr =	(de_dmdlocals.za0 *0x04000)+
				(de_dmdlocals.xa1 *0x08000)+
				(de_dmdlocals.xa2 *0x10000)+
 				(de_dmdlocals.xa3 *0x20000)+
				(de_dmdlocals.xa4 *0x40000);
	cpu_setbank(1, memory_region(DE_MEMREG_DROM1) + addr);
}

static WRITE_HANDLER(bank_w)
{
	de_dmdlocals.xa0 = (data>>0)&1;
	de_dmdlocals.xa1 = (data>>1)&1;
	de_dmdlocals.xa2 = (data>>2)&1;
	de_dmdlocals.xa3 = (data>>3)&1;
	de_dmdlocals.xa4 = (data>>4)&1;
	de_dmdlocals.xa5 = (data>>5)&1;
	de_dmdlocals.xa6 = (data>>6)&1;
	de_dmdlocals.xa7 = (data>>7)&1;
	de_dmdlocals.za0=de_dmdlocals.xa0;
//	logerror("xa_w %x %x %x %x %x %x %x %x: za0=%x\n",de_dmdlocals.xa0,
//			de_dmdlocals.xa1,de_dmdlocals.xa2,de_dmdlocals.xa3,de_dmdlocals.xa4,
//			de_dmdlocals.xa5,de_dmdlocals.xa6,de_dmdlocals.xa7,de_dmdlocals.za0);
	switchbank();
}


static WRITE_HANDLER(pia3a_w) {
  /*DMD Data - Send DMD Command (PA0-PA7)*/
  de_dmdlocals.dmd_latch = data;
//  logerror("dmd_latch written (%x)\n", data);
}

static WRITE_HANDLER(pia3b_w) {
    /*DMD Data - Clock & Reset signal (PB0-PB1)*/
//	logerror("DMD Clock & Reset %x\n", data);

	/*PB0 - DMD Clock Triggers an IRQ!*/
	if(data&1) {
		de_dmdlocals.busy = 1;
		cpu_set_irq_line(DE_DCPU1, M6809_IRQ_LINE, HOLD_LINE);
	}

	/*PB1 - Trigger a Reset*/
	if((data>>1)&1)
	{
		cpu_set_reset_line(DE_DCPU1, PULSE_LINE);
		//Set all vars to 0!
		memset(&de_dmdlocals, 0, sizeof(de_dmdlocals));
		switchbank();
	}
}

static READ_HANDLER(pia3b_r)
{
//	return de_dmdlocals.status | (de_dmdlocals.busy?0x0f:0x00);
//	return de_dmdlocals.status | (de_dmdlocals.busy<<7);
	return (de_dmdlocals.busy?0x80:0x00) + (de_dmdlocals.status<<3);
}

struct pia6821_interface dedmd2_pia_intf[] = {
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
 /* CA2       (I) Switched Solenoid Driver 3 */
 /* CB2       (I) Switched Solenoid Driver 5 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 2 (2800) Chip 11B */
 /* PA0 - PA3 Feed 74154 - Chip 4H - Feeding CN1 & CN2 */
 /* PA4       Diagnostic LED */
 /* PA5 - PA6 NC */
 /* PA7       (I) Jumper J8 */
 /* PB0 - PB7 Data Line for Printer  (Also - Magnet control for GNR)*/
 /* CA1       (I) Diagnostic Advance */
 /* CB1       (I) Diagnostic Up/dn */
 /* CA2       N/A */
 /* CB2       N/A */
 /* in  : A/B,CA/B1,CA/B2 */ pia2a_r, pia2b_r, pia2ca1_r, pia2cb1_r, 0, 0,
 /* out : A/B,CA/B2       */ pia2a_w, pia2b_w, pia2ca2_w, pia2cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 3 (2c00) Chip 9B*/
 /* PA0 - PA7 Display Data (h,j,k,m,n,p,r,dot) - ALPHA Games Only*/
 /* PB0 - PB7 Display Data (a,b,c,d,e,f,g,com) - ALPHA Games Only */
 /* PB0       DMD Clock Strobe		- DMD Games Only
    PB1 	  DMD Reset				- DMD Games Only
	PB2		  Not Used
	PB3 - PB6 (I) DMD Status 0-3 Flag	- DMD Games Only
	PB7       (I) DMD Busy Flag			- DMD Games Only*/
 /* CA1       CN25-Pin 15-Not Used*/
 /* CB1       CN25-Pin 11-Not Used*/
 /* CA2       (I) Switched Solenoid Driver 2 */
 /* CB2       (I) Switched Solenoid Driver 6 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, pia3b_r, pia3ca1_r, pia3cb1_r, pia3ca2_r, pia3cb2_r,
 /* out : A/B,CA/B2       */ pia3a_w, pia3b_w, pia3ca2_w, pia3cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 4 (3000) Chip 8H*/
 /* PA0 - PA7 Switch Input (row) */
 /* PB0 - PB7 Switch Drivers (column) */
 /* CA1/CB1   GND */
 /* CA2       (I) Switched Solenoid Driver 1 */
 /* CB2       (I) Switched Solenoid Driver 4 */
 /* in  : A/B,CA/B1,CA/B2 */ pia4a_r, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */ 0, pia4b_w, pia4ca2_w, pia4cb2_w,
 /* irq : A/B             */ de_piaMainIrq, de_piaMainIrq


},{ /* PIA 5 (3400) Chip 7B*/
 /* PA0 - PA7 Not connected for DMD128x16 */
 /* PB0 - PB7 Sound Data */
 /* CA1       CN25-Pin 16-BUSY?*/
 /* CB1       CN25-Pin 12-SST0?*/
 /* CA2       CN25-Pin 18-Reset?*/
 /* CB2       CN25-Pin 13-STB?*/
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
  if (core_getSw(DE_SWCPUDIAG))   cpu_set_nmi_line(DE_CPUNO, PULSE_LINE);
  if(UsingSound)
	if (core_getSw(DE_SWSOUNDDIAG)) cpu_set_nmi_line(DE_SDCPU1, PULSE_LINE);

  /*-- coin door switches --*/
  pia_set_input_ca1(2, core_getSw(DE_SWADVANCE));
  pia_set_input_cb1(2, core_getSw(DE_SWUPDN));

  if(core_getSw(DE_SWADVANCE))
          core_textOutf(40, 20, BLACK, "%-7s","A-Up");
  else
          core_textOutf(40, 20, BLACK, "%-7s","A-Down");

  /* Show Status of Auto/Manual Switch */
  if(core_getSw(DE_SWUPDN))
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
  de_sndCmd_w, "de",
  core_swSeq2m, core_swSeq2m,core_m2swSeq,core_m2swSeq
};

static void de_init(void) {
  /* init PIAs */
  int ii;
  if (delocals.initDone)
    de_exit();
  delocals.initDone = TRUE;

  /*Set DMD to point to our DMD memory*/
  coreGlobals_dmd.DMDFrames[0] = memory_region(DE_MEMREG_DCPU1);

  //Set all vars to 0!
  memset(&de_dmdlocals, 0, sizeof(de_dmdlocals));

  for (ii = 0; ii < sizeof(dedmd2_pia_intf)/sizeof(dedmd2_pia_intf[0]); ii++)
    pia_config(ii, PIA_STANDARD_ORDERING, &dedmd2_pia_intf[ii]);

  if (core_init(&deData))
	  return;

  /*copy last 16K of ROM into last 16K of CPU region*/
  /*Setup ROM Swap so opcode will work*/
  if(memory_region(DE_MEMREG_DCPU1))
  {
  memcpy(memory_region(DE_MEMREG_DCPU1)+0x8000,
         memory_region(DE_MEMREG_DROM1) +
	     (memory_region_length(DE_MEMREG_DROM1) - 0x8000), 0x8000);
  }

  /*Sound Enabled?*/
  if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate)
  {
	  UsingSound=1;
	  DE_sinit(DE_SDCPU1);
  }
  else
	  UsingSound=0;

  logerror("Using Sound = %s\n",UsingSound?"Yes":"No");

  pia_reset();

  delocals.dmdfirqtimer = NULL;
  //delocals.dmdfirqtimer = timer_pulse(TIME_IN_HZ(DE_DMDFIRQFREQ),0,dmdfirq);
}

static void de2_init(void) {
  /* init PIAs */
  int ii;
  if (delocals.initDone)
    de_exit();
  delocals.initDone = TRUE;

  /*Set DMD to point to our DMD memory*/
  coreGlobals_dmd.DMDFrames[0] = memory_region(DE_MEMREG_DCPU1);

  //Set all vars to 0!
  memset(&de_dmdlocals, 0, sizeof(de_dmdlocals));

  for (ii = 0; ii < sizeof(dedmd2_pia_intf)/sizeof(dedmd2_pia_intf[0]); ii++)
    pia_config(ii, PIA_STANDARD_ORDERING, &dedmd2_pia_intf[ii]);

  if (core_init(&deData))
	  return;

  /*Sound Enabled?*/
  if (((Machine->gamedrv->flags & GAME_NO_SOUND) == 0) && Machine->sample_rate)
  {
	  UsingSound=1;
	  DE_sinit(DE_SDCPU1);
  }
  else
	  UsingSound=0;

  logerror("Using Sound = %s\n",UsingSound?"Yes":"No");

  pia_reset();
}


static void de_exit(void) {
#ifdef PINMAME_EXIT
  if (delocals.dmdfirqtimer) {
	  timer_remove(delocals.dmdfirqtimer);
	  delocals.dmdfirqtimer = NULL;
  }
#endif
  core_exit();
}

static READ_HANDLER(dmdlatch_r)
{
//	logerror("%x: Reading dmdlatch %x\n",cpu_get_pc(),de_dmdlocals.dmd_latch);
	de_dmdlocals.busy = 0;
	cpu_set_irq_line(DE_DCPU1, M6809_IRQ_LINE, CLEAR_LINE);
	return de_dmdlocals.dmd_latch;
}

static WRITE_HANDLER(status_w)
{
//	logerror("Status Write %x\n",data);
	//de_dmdlocals.status = data<<4;
	de_dmdlocals.status = data;
}

//static int nTest = 0;
static int de_dmdfirq(void)
{
//	nTest = (nTest+1) % 4;
//	return !nTest ? M6809_INT_FIRQ : ignore_interrupt();
//	return M6809_INT_FIRQ; //ignore_interrupt();
	return ignore_interrupt();
}

static int de2_dmdfirq(void)
{
	//nTest = (nTest+1) % 4;
	//return !nTest ? M6809_INT_FIRQ : ignore_interrupt();
	//return M6809_INT_FIRQ; //ignore_interrupt();
	return MC68000_INT_NONE;
}

static void dmdfirq(int data){
cpu_set_irq_line(DE_DCPU1, M6809_FIRQ_LINE,PULSE_LINE);
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

/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(de_dmdreadmem)
{0x0000,0x1fff, MRA_RAM},	/*RAM*/
{0x2000,0x21ff, MRA_RAM},	/*DMD RAM PAGE 0 512K*/
{0x2200,0x23ff, MRA_RAM},	/*DMD RAM PAGE 1 512K*/
{0x2400,0x25ff, MRA_RAM},	/*DMD RAM PAGE 2 512K*/
{0x2600,0x27ff, MRA_RAM},	/*DMD RAM PAGE 3 512K*/
{0x2800,0x29ff, MRA_RAM},	/*512K*/
{0x2a00,0x2bff, MRA_RAM},	/*512K*/
{0x2c00,0x2dff, MRA_RAM},	/*512K*/
{0x2e00,0x2fff, MRA_RAM},	/*512K*/
{0x3000,0x3000, crtc6845_register_r},
{0x3003,0x3003, dmdlatch_r},
{0x4000,0x7fff, MRA_BANK1},	/*ROM BANKING*/
{0x8000,0xffff, MRA_ROM},	/*ROM CODE*/
MEMORY_END

static MEMORY_WRITE_START(de_dmdwritemem)
{0x0000,0x1fff, MWA_RAM},	/*RAM*/
{0x2000,0x21ff, MWA_RAM},	/*DMD RAM PAGE 0 512K*/
{0x2200,0x23ff, MWA_RAM},	/*DMD RAM PAGE 1 512K*/
{0x2400,0x25ff, MWA_RAM},	/*DMD RAM PAGE 2 512K*/
{0x2600,0x27ff, MWA_RAM},	/*DMD RAM PAGE 3 512K*/
{0x2800,0x29ff, MWA_RAM},	/*512K*/
{0x2a00,0x2bff, MWA_RAM},	/*512K*/
{0x2c00,0x2dff, MWA_RAM},	/*512K*/
{0x2e00,0x2fff, MWA_RAM},	/*512K*/
{0x3000,0x3000, crtc6845_address_w},
{0x3001,0x3001, crtc6845_register_w},
{0x3002,0x3002, bank_w},	/*DMD Bank Switching*/
{0x4000,0x4000, status_w},	/*DMD Status*/
MEMORY_END

static MEMORY_READ16_START(de2_dmdreadmem)
{0x000000,0x7fffff, MRA16_ROM},	/*RAM*/
{0x000000,0x8fffff, MRA16_RAM},	/*RAM*/
MEMORY_END

static MEMORY_WRITE16_START(de2_dmdwritemem)
{0x000000,0x7fffff, MWA16_ROM},	/*RAM*/
{0x800000,0x8fffff, MWA16_RAM},	/*RAM*/
MEMORY_END

struct MachineDriver machine_driver_DE_DMD2 = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1, //DE_VBLANKDIV,
      de_irq, DE_IRQFREQ
    },
	{
      CPU_M6809, 4000000, /* 4 Mhz*/
      de_dmdreadmem, de_dmdwritemem, NULL, NULL,
	  de_dmdfirq, DE_DMDFIRQFREQ
    }
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de_init,CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, de_dmd128x32_refresh,
  0,0,0,0, {{0}},
  de_nvram
};

struct MachineDriver machine_driver_DE_DMD3 = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1, //DE_VBLANKDIV,
      de_irq, DE_IRQFREQ
    },
	{
      CPU_M68000,   12000000, /* 12 Mhz*/
      de2_dmdreadmem, de2_dmdwritemem, NULL, NULL,
	  de2_dmdfirq, DE_DMDFIRQFREQ
    }
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de2_init,CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, de_dmd128x32_refresh,
  0,0,0,0, {{0}},
  de_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void de_nvram(void *file, int write) {
  if (write)  /* save nvram */
    osd_fwrite(file, memory_region(DE_MEMREG_CPU), 0x2000);
  else if (file) /* load nvram */
    osd_fread(file, memory_region(DE_MEMREG_CPU), 0x2000);
  else        /* first time */
    memset(memory_region(DE_MEMREG_CPU), 0xff, 0x2000);
}
