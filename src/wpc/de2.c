/*DE2.C - Includes support for
         DMD 128x16 Games And 1st Sound Board Generations ONLY */

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
#include "cpu/z80/z80.h"
#include "machine/6821pia.h"
#include "core.h"
#include "de1sound.h"
#include "dedmd.h"
#include "de.h"
#include "de2.h"
#include "snd_cmd.h"

#define DE_DCPU1			1	/*DMD CPU IS #1*/
#define DE_SDCPU1			2   /*DMD SOUND GEN 1 CPU IS #2*/

#define DE_VBLANKFREQ      50   /* VBLANK frequency */
#define DE_IRQFREQ       1075   /* IRQ Frequency (Measured on real game)*/
//#define DE_DMDIRQFREQ	 7810*4 /* NMI occurs at 7810 kHz (4Mhz/512) */
#define DE_DMDIRQFREQ	 2000   /* As measured on my Hook */

static void de_init(void);
static void de_exit(void);
static void de_nvram(void *file, int write);

UINT32 hv5408_shift, hv5408_latch;
UINT32 hv5308_shift, hv5308_latch;
UINT32 hv5222;

static unsigned char dmdRAM[0x2000] = {0};

static int UsingSound = 0;

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

static struct {
  int    bs0;		//BS0-2 Rom Bank Switching bits
  int    bs1;
  int    bs2;
  int	 busy;
  int    status;	//Status line back to main cpu.. doesn't show as hooked up in cpu schem
  int    test;		//Not sure what this is for.. (used with IDAT to schedule an INT)
  int    rowclk;	//Row Clock
  int    rowdata;	//Row Data
  int    coclk;		//Column Clock
  int    clatch;	//Column Latch
  int    pd0;		//Display Data 1
  int    pd1;		//Display Data 2
  int    idat;		//Interrupt Request Line?
  int    blank;		//Blanking Line to DMD Controller
  int    dmd_latch;	//DMD Command Latch from Main CPU
  int	 dmd_clock;	//Clock signal from Main CPU.
  int	 dmd_reset;	//Reset signal from Main CPU.
  int	 colnum;	//Track Column #
  int	 rownum;	//Track Row #
  int	 nmi_freq;	//Track NMI Frequency

  int	 hc74_q;
  int	 hc74_nq;
  int    hc74_clk;

  int    nTest;
} de_dmdlocals;

int  de_dmd128x16[16][128] = {{0}};

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
static READ_HANDLER (pia2a_r)   { return 1; }
static WRITE_HANDLER(pia2a_w) { delocals.diagnosticLed = (data>>4);	/*LED = PA_4*/ }
static WRITE_HANDLER(pia2b_w) { logerror("pia2b_w\n"); }
static WRITE_HANDLER(pia5a_w) { logerror("pia5a_w\n"); }

/*----------------
/Sound Commands
/-----------------*/
static WRITE_HANDLER(pia5b_w)  {
	//logerror("soundlatch_w %x\n",data);
//	if(UsingSound){
//		soundlatch_w(0,data);
		snd_cmd_log(data);
//	}
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

static READ_HANDLER (pia2ca1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSwSeq(DE_SWADVANCE) : 0; }
static READ_HANDLER (pia2cb1_r) { return cpu_get_reg(M6808_IRQ_STATE) ? core_getSwSeq(DE_SWUPDN)    : 0; }

/************************************************/
/*********** DMD HANDLING BELOW *****************/
/************************************************/

/*Read Status & Busy from DMD*/
static READ_HANDLER(pia5a_r)
{
//	logerror("busy=%x status=%x\n",de_dmdlocals.busy,de_dmdlocals.status);
	int busy = (de_dmdlocals.idat == 0 && de_dmdlocals.test == 1);
	logerror("busy=%x status=%x\n",busy,de_dmdlocals.status);
	return busy
		 + de_dmdlocals.status    ? 0x02:0x00;
		 //+ de_dmdlocals.nTest     ? 0x00:0x80;
	
}

/*DMD HC74 Logic */
static void HC7474Logic(int clock, int clear, int preset)
{
	int nTemp = de_dmdlocals.hc74_nq;
	if ( !clear || !preset ) {
		if ( !clear && !preset ) {
			de_dmdlocals.hc74_q  = 0; // 1;
			de_dmdlocals.hc74_nq = 1;
		}
		else if ( !clear ) {
			de_dmdlocals.hc74_q  = 0;
			de_dmdlocals.hc74_nq = 1;
		}
		else {
			de_dmdlocals.hc74_q  = 1;
			de_dmdlocals.hc74_nq = 0;
		}
	}
	else {
		if ( clock && !de_dmdlocals.hc74_clk) {
			de_dmdlocals.hc74_q  = !de_dmdlocals.hc74_q;
			de_dmdlocals.hc74_nq = !de_dmdlocals.hc74_q;
		}
		de_dmdlocals.hc74_clk = clock;
	}
	if ( nTemp!=de_dmdlocals.hc74_nq ) {
		if ( !de_dmdlocals.hc74_nq ) {
			cpu_set_irq_line(DE_DCPU1, Z80_INT_REQ, ASSERT_LINE); 
			logerror("z80 int\n");
		}
		else {
			cpu_set_irq_line(DE_DCPU1, Z80_INT_REQ, CLEAR_LINE); 
		}
	}
	de_dmdlocals.busy = de_dmdlocals.hc74_q;
}

static void switchbank(void)
{
	int addr =   (de_dmdlocals.bs0 *0x04000) 
			   + (de_dmdlocals.bs1 *0x08000)
 			   + (de_dmdlocals.bs2 *0x10000);

	//logerror("switchbank: addr=%x\n",addr);
	cpu_setbank(2, memory_region(DE_MEMREG_DROM1) + addr);
}

static WRITE_HANDLER(pia3a_w) {
  /*DMD Data - Send DMD Command (PA0-PA7)*/
  de_dmdlocals.dmd_latch = data;

  logerror("dmd_latch written (%x)\n", data);
}
static WRITE_HANDLER(pia3b_w) {
	int nTemp;
    /*DMD Data - Clock & Reset signal (PB0-PB1)*/
	nTemp = (data>>1) & 0x01;
	logerror("pia3b_w %x\n", data);

	if ( nTemp != de_dmdlocals.dmd_reset ) {
		de_dmdlocals.dmd_reset = nTemp;
		if ( de_dmdlocals.dmd_reset ) {
			de_dmdlocals.bs0 = 0;
			de_dmdlocals.bs1 = 0;
			de_dmdlocals.bs2 = 0;
			de_dmdlocals.test = 0;
			de_dmdlocals.dmd_clock = 0;
			de_dmdlocals.blank = 0;
			de_dmdlocals.status = 0;
			de_dmdlocals.idat = 1;
			HC7474Logic(de_dmdlocals.dmd_clock, de_dmdlocals.idat, de_dmdlocals.test);
			switchbank();
			cpu_set_reset_line(DE_DCPU1, PULSE_LINE);
			logerror("reset\n");
		}
	}
	nTemp = data & 0x01;
	if ( de_dmdlocals.dmd_clock != nTemp ) {
		de_dmdlocals.dmd_clock = nTemp;
			logerror("clock set to %x\n",de_dmdlocals.dmd_clock);
		HC7474Logic(de_dmdlocals.dmd_clock, de_dmdlocals.idat, de_dmdlocals.test);
	}
}

struct pia6821_interface dedmd_pia_intf[] = {
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
 /* CA2       (I) Switched Solenoid Driver 2 */
 /* CB2       (I) Switched Solenoid Driver 6 */
 /* in  : A/B,CA/B1,CA/B2 */ 0, 0, pia3ca1_r, pia3cb1_r, pia3ca2_r, pia3cb2_r,
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
	if (core_getSwSeq(DE_SWSOUNDDIAG)) cpu_set_nmi_line(DE_SDCPU1, PULSE_LINE);

  /*-- coin door switches --*/
  pia_set_input_ca1(2, core_getSwSeq(DE_SWADVANCE));
  pia_set_input_cb1(2, core_getSwSeq(DE_SWUPDN));

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

  /*Clear DMD RAM*/
  memset(dmdRAM, 0x00, sizeof dmdRAM);
  memset(de_dmd128x16,0, sizeof de_dmd128x16);

  //Set all vars to 0!
  memset(&de_dmdlocals, 0, sizeof(de_dmdlocals));
  //These are active high!
  de_dmdlocals.bs0 = de_dmdlocals.bs1 = de_dmdlocals.bs2 =
  de_dmdlocals.coclk = de_dmdlocals.idat = 1;

  //Set dmd vars to 0!
  hv5408_shift = hv5408_latch = 0;
  hv5308_shift = hv5308_latch = 0;
  hv5222 = 0;
  de_dmdlocals.dmd_clock = 0;

  for (ii = 0; ii < sizeof(dedmd_pia_intf)/sizeof(dedmd_pia_intf[0]); ii++)
    pia_config(ii, PIA_STANDARD_ORDERING, &dedmd_pia_intf[ii]);

  if (core_init(&deData))
	  return;

  /*Load 64K/128K DMD Chip rom into 0x0000 memory in Z80 CPU */
  /*copy last 16K of ROM into 1st 16K of CPU region*/
  if(memory_region(DE_MEMREG_DCPU1))
  {
  memcpy(memory_region(DE_MEMREG_DCPU1),
         memory_region(DE_MEMREG_DROM1) + 
	     (memory_region_length(DE_MEMREG_DROM1) - 0x4000), 0x4000);
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
}

static void de_exit(void) {
  core_exit();
}

static void showstatus(void){
	logerror("col=%d: row=%d: ",de_dmdlocals.colnum,de_dmdlocals.rownum);
	logerror("coclk=%x: clatch=%x: rowdata=%x : ",
			  de_dmdlocals.coclk,de_dmdlocals.clatch,de_dmdlocals.rowdata);
	logerror("rowclk=%x: blank=%x : status=%x: idat=%x: test=%x: ",
			  de_dmdlocals.rowclk,de_dmdlocals.blank,de_dmdlocals.status,de_dmdlocals.idat,
			  de_dmdlocals.test);
	logerror("latch=%x : clock=%x: reset=%x: pd0=%x: pd1=%x\n",
			  de_dmdlocals.dmd_latch,de_dmdlocals.dmd_clock,de_dmdlocals.dmd_reset,de_dmdlocals.pd0,de_dmdlocals.pd1);
}

//DMD Z80 - Timed IRQ Event.. NMI is generated
static int de_dmdirq(void) {
/*
	int nTemp;
	if ( de_dmdlocals.dmd_reset ) {
		de_dmdlocals.nmi_freq = 0;
		return ignore_interrupt();	//Otherwise an IRQ_INT is asserted via mame core!
	}
*/
	cpu_set_nmi_line(DE_DCPU1, PULSE_LINE);
#if 0
	nTemp = de_dmdlocals.nmi_freq;
	de_dmdlocals.nmi_freq = (de_dmdlocals.nmi_freq + 1) % 16;

	if ( nTemp==0 ) {
		cpu_set_nmi_line(DE_DCPU1, ASSERT_LINE);
	}
	else {
		cpu_set_nmi_line(DE_DCPU1, CLEAR_LINE);
	}
#endif
	return ignore_interrupt();
}

static void coldriver_w(int data1, int data2)
{
	/*Shift Data & add new bit to end!*/
	hv5408_shift = (hv5408_shift<<1) | (data1 ? 0x0000001 : 0x00000000);
	hv5308_shift = (hv5308_shift<<1) | (data2 ? 0x0000001 : 0x00000000);
}

static void rowdriver_w(int data)
{
	/*Shift Data & add new bit to end!*/
	hv5222 = (hv5222<<1) + (data&0x01);
}

#if 0
static void dmd_w(void)
{
	int curRow, curCol, colOffset;

	UINT32 tmphv5222 = hv5222;

	if ( !hv5222 ) {
		// memset(de_dmd128x16, 0, sizeof de_dmd128x16);
		return;
	}

	/* this code is able to handle multiple lines written at once     */
	/* the game never does this, see below for a slighly faster way */

	curRow = 0;
	while ( tmphv5222 ) {
		/* Copy Column Drivers to current row!                               */

		/* 5408 writes the even # dots, 5308 writes the odd # dots           */
		/* Load the DMD data from the right to left side, ie 128 first..to 1 */

		/* odd lines driving the columns on the right site, i.e. 64...127 */
		if ( tmphv5222 & 0x01 ) {
			colOffset = 64;
			for (curCol=0; curCol<64; curCol+=2)
			{
				if ( ((hv5408_latch>>(curCol/2))&0x01) )
					de_dmd128x16[curRow][(127-colOffset)-curCol] += ((hv5408_latch>>(curCol/2))&0x01);
				else
					de_dmd128x16[curRow][(127-colOffset)-curCol] = 0;

				if ( ((hv5308_latch>>(curCol/2))&0x01) )
					de_dmd128x16[curRow][((127-colOffset)-curCol)-1] += ((hv5308_latch>>(curCol/2))&0x01);
				else
					de_dmd128x16[curRow][((127-colOffset)-curCol)-1] = 0;
			}
		}

		/* even lines driving the columns on the left site, i.e. 0..63 */
		if ( tmphv5222 & 0x02 ) {
			colOffset = 0;
			for (curCol=0; curCol<64; curCol+=2)
			{
				if ( ((hv5408_latch>>(curCol/2))&0x01) )
					de_dmd128x16[curRow][(127-colOffset)-curCol] += ((hv5408_latch>>(curCol/2))&0x01);
				else
					de_dmd128x16[curRow][(127-colOffset)-curCol] = 0;

				if ( ((hv5308_latch>>(curCol/2))&0x01) )
					de_dmd128x16[curRow][((127-colOffset)-curCol)-1] += ((hv5308_latch>>(curCol/2))&0x01);
				else
					de_dmd128x16[curRow][((127-colOffset)-curCol)-1] = 0;
			}
		}

		curRow++;
		tmphv5222 >>= 2;
	}
	return;

	/* this code can only handle one line at a time, this is what the game actually does */
	/* you you want a very slightly speedup, use this code                               */

	/* Determine which row */

	if ( hv5222 & 0x55555555 ) {
		/* odd lines, right site */
		colOffset = 64;

		/* at last we have on line */
		tmphv5222 = hv5222>>2;
	}
	else {
		/* even lines, left site */
		colOffset = 0;

		/* at last we have on line, but now even and odd indexes are the same */
		tmphv5222 = hv5222>>3;
	}

	curRow = 0;
	while ( tmphv5222 ) {
		curRow++;

		/* skip the next odd/even combination */
		tmphv5222>>=2;
	}

	/*Copy Column Drivers to current row!*/
	/*If Row # ie ODD - Left 64 Dots are selected, otherwise Right Side*/

	/*5408 writes the even # dots, 5308 writes the odd # dots*/
	/*Load the DMD data from the right to left side, ie 128 first..to 1*/

	for (curCol=0; curCol<64; curCol+=2)
	{
		de_dmd128x16[curRow][(127-colOffset)-curCol]     = ((hv5408_latch>>(curCol/2))&0x01);
		de_dmd128x16[curRow][((127-colOffset)-curCol)-1] = ((hv5308_latch>>(curCol/2))&0x01);
	}
}
#endif

static int read=0;
//This chip feeds the DMD data to the DMD controller logic.
static WRITE_HANDLER(dmd_hc153){
	if (read)
		return;
	if ( data )
		data = data;
	switch(offset&0x03)
	{
	case 0:
		de_dmdlocals.pd0 = ((data>>0)&0x01);	//PD 0 = D0
		de_dmdlocals.pd1 = ((data>>1)&0x01);	//PD 1 = D1
		break;
	case 1:
		de_dmdlocals.pd0 = ((data>>2)&0x01);	//PD 0 = D2
		de_dmdlocals.pd1 = ((data>>3)&0x01);	//PD 1 = D3
		break;
	case 2:
		de_dmdlocals.pd0 = ((data>>4)&0x01);	//PD 0 = D4
		de_dmdlocals.pd1 = ((data>>5)&0x01);	//PD 1 = D5
		break;
	case 3:
		de_dmdlocals.pd0 = ((data>>6)&0x01);	//PD 0 = D6
		de_dmdlocals.pd1 = ((data>>7)&0x01);	//PD 1 = D7
		break;
	default:
		break;
	}
}

static int dmd_hc139(int a, int b, data8_t data)
{
	// a2, a7
	int lastcoclk  = 1; // de_dmdlocals.coclk;
	int lastclatch = 0; // de_dmdlocals.clatch;
	int lastidat   = 1; // de_dmdlocals.idat;

	if (!a & !b)						//COCLK Signal Goes LOW
	{
		de_dmdlocals.coclk  = 0;
		de_dmdlocals.clatch = 0;
		de_dmdlocals.idat   = 1;
//		logerror("pd0=%x pd1=%x\n",de_dmdlocals.pd0,de_dmdlocals.pd1);
		coldriver_w(de_dmdlocals.pd0,de_dmdlocals.pd1);
	}
	else
	{
		if (a & !b)						//CLATCH Signal Goes HIGH!
		{
			de_dmdlocals.coclk  = 1;
			de_dmdlocals.clatch = 1;	//CLATCH signal is inverted from active low
			de_dmdlocals.idat   = 1;

			hv5408_latch = hv5408_shift;
			hv5308_latch = hv5308_shift;
		}
		else
		{
			if(!a & b)						//IDAT Signal Goes LOW
			{
				de_dmdlocals.coclk = 1;
				de_dmdlocals.clatch = 0;

				/* on the end of the I/O operation idat will return to high */
				de_dmdlocals.idat = 1;
				/* Read DMD Latch */
				data = de_dmdlocals.dmd_latch;
				
				///*HC74 @ U11 Logic*/
				HC7474Logic(de_dmdlocals.dmd_clock, 0, de_dmdlocals.test);
			}
			else
			{
				if(a & b)						//HC259 Enabled
				{
				de_dmdlocals.coclk = 1;
				de_dmdlocals.clatch = 0;
				de_dmdlocals.idat = 1;
				}
				else
					logerror("DMD_HC139: Control path failed!\n");
			}
		}
	}

	/*Flag what changed
	  ------------------*/

	/*Did column clock change?*/
	if(lastcoclk != de_dmdlocals.coclk)
	{
//		logerror("coclk set to %x\n", de_dmdlocals.coclk);
	}

	/*Did column latch change?*/
	if(lastclatch != de_dmdlocals.clatch)
	{
//		logerror("clatch set to %x\n", de_dmdlocals.clatch);
	}

	/*Did idat change?*/
	if (lastidat != de_dmdlocals.idat)
	{
//		logerror("IDAT set to %x\n", de_dmdlocals.idat);
	}
	return data;
}

/* All variables to the gates must be updated on both a read and a write!*/
static int dmd_portlogic(offs_t offset,data8_t data)
{
	int lastrowclk = de_dmdlocals.rowclk;
	int lastblank = de_dmdlocals.blank;
	int a0, a1, a2, a3, a4, a6, a7, d0;
	a0 = (offset)&0x01;
	a1 = (offset>>1)&0x01;
	a2 = (offset>>2)&0x01;
	a3 = (offset>>3)&0x01;
	a4 = (offset>>4)&0x01;
	a6 = (offset>>6)&0x01;
	a7 = (offset>>7)&0x01;
	d0 = (data & 0x01);

	/*Process HC153 Logic*/
	if ( offset<=0x03 )
		dmd_hc153(offset,data);

	/*Process HC139 Logic*/
	data = dmd_hc139(a2,a7,data);		

	//Only if A2 and A7 are active, is the HC259 Addressable Latch Active
	if (a2 & a7) {
		/*If Address Line 6 is low, then do bank switch*/
		/*NOTE: We assume A14 is high for the switch to occur, but it's ok, because
		        a read by the CPU to 0x4000-0x7fff means that a14 is high!*/
		/*NOTE2: BS0-BS2 are inverted */
		if (!a6) {		
			if(!a4 & !a3){		//000		//SET BANK SWITCH BIT 0 (0x84)
				de_dmdlocals.bs0 = !d0;
				//logerror("*bs0=%x : bs1=%x: bs2=%x, data=%x, d0=%x\n", 
				//		  de_dmdlocals.bs0,de_dmdlocals.bs1,de_dmdlocals.bs2,data,d0);
				switchbank();
			}
			if(!a4 & a3){		//001		//SET BANK SWITCH BIT 1	(0x8c)
				de_dmdlocals.bs1 = !d0;
				//logerror("bs0=%x : *bs1=%x: bs2=%x, data=%x, d0=%x\n", 
				//		  de_dmdlocals.bs0,de_dmdlocals.bs1,de_dmdlocals.bs2,data,d0);
				switchbank();
			}
			if(a4 & !a3){		//010		//SET BANK SWITCH BIT 2	(0x94)
				de_dmdlocals.bs2 = !d0;
				//logerror("bs0=%x : bs1=%x: *bs2=%x, data=%x, d0=%x\n", 
				//		  de_dmdlocals.bs0,de_dmdlocals.bs1,de_dmdlocals.bs2,data,d0);
				switchbank();
			}
			if(a4 & a3){		//011		//SET BLANKING LINE
				de_dmdlocals.blank = d0;
				if(lastblank != de_dmdlocals.blank && de_dmdlocals.blank==1)
					;
					//dmd_w();
			}
		}
		else {
			if(!a4 & !a3){  //100		//SET STATUS LINE
				de_dmdlocals.status = d0;
				logerror("status set to %x\n",de_dmdlocals.status);
			}
			if(!a4 & a3){   //101		//SET ROW DATA LINE
				de_dmdlocals.rowdata = d0;
				// logerror("rowdata set to %x\n",de_dmdlocals.rowdata);
			}
			if(a4 & !a3){   //110		//SET ROW CLOCK LINE
				de_dmdlocals.rowclk = d0;
				// logerror("rowclock set to %x\n",de_dmdlocals.rowclk);

				/*Send data to column drivers*/
				if ( lastrowclk != de_dmdlocals.rowclk && de_dmdlocals.rowclk==1 )
					rowdriver_w(de_dmdlocals.rowdata);
			}
			if(a4 & a3)    //111		//SET TEST LINE
			{
				de_dmdlocals.test = d0;
				logerror("test set to %x\n",de_dmdlocals.test);
				HC7474Logic(de_dmdlocals.dmd_clock, de_dmdlocals.idat, de_dmdlocals.test);
			}
		}
	}

	return data;
}

static READ_HANDLER(de_dmdportread) {
	int data;
	read = 1;
	data=dmd_portlogic(offset,0);	//Assume data bus 0 on a read
	logerror("Read Port #%x returned %x\n",offset,data);

	if ( de_dmdlocals.dmd_latch==1 )
		de_dmdlocals.nTest = 1;

	return data;
}

static WRITE_HANDLER(de_dmdportwrite){
	//logerror("Write Port #%x - data:%x\n",offset,data);
	read = 0;
	dmd_portlogic(offset,data);
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

/*We'll handle RAM ourselves*/
static READ_HANDLER(de_dmdmemread) {
	return dmdRAM[offset]; 
}
static WRITE_HANDLER(de_dmdmemwrite) {
	if ( offset<0x2000 )
		dmdRAM[offset] = data;
}

/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(de_dmdreadmem)
	{ 0x0000, 0x3fff, MRA_ROM},			/*Z80 ROM CODE*/
	{ 0x4000, 0x7fff, MRA_BANK2},		/*ROM BANK*/
{0x8000,0x9fff, MRA_RAM},
//	{ 0x8000, 0x9fff, de_dmdmemread},	/*RAM REGION*/
//	{ 0xa000, 0xbfff, de_dmdmemread},	/*Mirrored*/
//	{ 0xc000, 0xdfff, de_dmdmemread},	/*Mirrored*/
//	{ 0xe000, 0xffff, de_dmdmemread},	/*Mirrored*/
MEMORY_END

static MEMORY_WRITE_START(de_dmdwritemem)
	{ 0x0000, 0x3fff, MWA_ROM},			/*Z80 ROM CODE*/
	{ 0x4000, 0x7fff, MWA_ROM},			/*ROM BANK*/
{0x8000,0x9fff, MWA_RAM},
//	{ 0x8000, 0x9fff, de_dmdmemwrite},
//	{ 0xa000, 0xbfff, de_dmdmemwrite},
//	{ 0xc000, 0xdfff, de_dmdmemwrite},
//	{ 0xe000, 0xffff, de_dmdmemwrite},
MEMORY_END

static PORT_READ_START( de_dmdreadport )
	{0x00,0xff, de_dmdportread},
PORT_END

static PORT_WRITE_START( de_dmdwriteport )
	{0x00,0xff, de_dmdportwrite},
PORT_END

/*------------------------------------
/  Memory map for BSMT2000 SOUN BOARD
/------------------------------------*/
static MEMORY_READ_START(de_snd2readmem)
//	{ 0x0000, 0x1fff, MRA_RAM},			
//	{ 0x4000, 0x7fff, MRA_BANK2},		
    { 0x8000, 0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(de_snd2writemem)
// 	{ 0x0000, 0x1fff, MWA_RAM},			
//	{ 0x4000, 0x7fff, MWA_ROM},			
    { 0x8000, 0xffff, MWA_ROM},
MEMORY_END


/*DMD 128x16 Games w/o Emulated Sound*/
struct MachineDriver machine_driver_DE_DMD1 = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1,
      de_irq, DE_IRQFREQ
    },
	{
      CPU_Z80,   4000000, /* 4 Mhz*/
      de_dmdreadmem, de_dmdwritemem, de_dmdreadport, de_dmdwriteport,
	  NULL, 0,
	  de_dmdirq, DE_DMDIRQFREQ
    }
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de_init, CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, de2_dmd128x16_refresh,
  0,0,0,0, {{0}},
  de_nvram
};

#if 0
/*DMD 128x16 Games w/o Emulated Sound*/
struct MachineDriver machine_driver_DE_DMD1 = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1,
      de_irq, DE_IRQFREQ
    },
	{
      CPU_Z80,   4000000, /* 4 Mhz*/
      de_dmdreadmem, de_dmdwritemem, de_dmdreadport, de_dmdwriteport,
	  NULL, 0,
	  de_dmdirq, DE_DMDIRQFREQ
    },
	{
      CPU_M6809 | CPU_AUDIO_CPU,   2000000, /* 2 Mhz*/
      de_snd2readmem, de_snd2writemem, 0,0,
      NULL, 0,0,0
	}
  },
  DE_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  de_init, CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, de2_dmd128x16_refresh,
  0,0,0,0, {{0}},
  de_nvram
};
#endif


/*DMD 128x16 Games with Generation 1 Emulated Sound*/
struct MachineDriver machine_driver_DE_DMD1S = {
  {
    {
      CPU_M6808, 1000000, /* 1 Mhz */
      de_readmem, de_writemem, NULL, NULL,
      de_vblank, 1,
      de_irq, DE_IRQFREQ
    },
	{
      CPU_Z80,   4000000, /* 4 Mhz */
      de_dmdreadmem, de_dmdwritemem, de_dmdreadport, de_dmdwriteport,
	  NULL,0,
	  de_dmdirq, DE_DMDIRQFREQ
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
  NULL, NULL, de2_dmd128x16_refresh,
  0,0,0,0, {DE_SOUND},
  de_nvram
};

/*This driver uses NO Display - Used for Test Chip*/
struct MachineDriver machine_driver_DE_DMDNO = {
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
/-------------------------------------------------*/
void de_nvram(void *file, int write) {
  if (write)  /* save nvram */
    osd_fwrite(file, memory_region(DE_MEMREG_CPU), 0x2000);
  else if (file) /* load nvram */
    osd_fread(file, memory_region(DE_MEMREG_CPU), 0x2000);
  else        /* first time */
    memset(memory_region(DE_MEMREG_CPU), 0xff, 0x2000);
}
