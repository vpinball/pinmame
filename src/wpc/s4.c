#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "s4.h"
#include "s67s.h"

#define S4_VBLANKFREQ    60 /* VBLANK frequency */
#define S4_IRQFREQ    1000  /* IRQ Frequency*/

#define CONFIGURE_PIAS4(a,b,c,d) \
	pia_unconfig();\
	pia_config(0, PIA_STANDARD_ORDERING, &a);\
	pia_config(1, PIA_STANDARD_ORDERING, &b);\
	pia_config(2, PIA_STANDARD_ORDERING, &c);\
	pia_config(3, PIA_STANDARD_ORDERING, &d);

static void s4_exit(void);

static struct {
  int	 alphapos;
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  swMatrix[CORE_MAXSWCOL];
  UINT8  lampMatrix[CORE_MAXLAMPCOL];
  core_tSeg segments, pseg;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int    dipcol;
  int    ssEn;
} s4locals;

static data8_t *s4_CMOS;

/*
  6 Digit Structure, Alpha Position, and Layout
  ----------------------------------------------
  (00)(01)(02)(03)(04)(05) (06)(07)(08)(09)(10)(11)

  (16)(17)(18((19)(20)(21) (22)(23)(24)(25)(26)(27)

	   (12)(13)   (14)(15)

  0-5 =		Player 1
  6-11 =	Player 2
  12-15 =   Credits/Balls In Play
  16-21 =	Player 3
  22-27 =	Player 4
*/

//Structure is: top, left, start, length, type
core_tLCDLayout s4_disp[] = {
  // Player 1            Player 2
  {0, 0, 0,6,CORE_SEG7}, {0,18, 6,6,CORE_SEG7},
  // Player 3            Player 4
  {2, 0,16,6,CORE_SEG7}, {2,18,22,6,CORE_SEG7},
  // Right Side          Left Side
  {4, 9,12,2,CORE_SEG7}, {4,14, 14,2,CORE_SEG7}, {0}
};

static void s4_nvram(void *file, int write);

static READ_HANDLER(s4_dips_r);
static READ_HANDLER(s4_diagsw1_r);
static READ_HANDLER(s4_diagsw2_r);
static READ_HANDLER(s4_entersw_r);
static READ_HANDLER(s4_swcol_r);
static READ_HANDLER(s4_swrow_r);
static WRITE_HANDLER(s4_swcol_w);
static WRITE_HANDLER(s4_alpha_w);
static WRITE_HANDLER(s4_pa_w);
static WRITE_HANDLER(s4_specsol6_w);
static WRITE_HANDLER(s4_specsol4_w);
static WRITE_HANDLER(s4_specsol3_w);
static WRITE_HANDLER(s4_lampcol_w);
static WRITE_HANDLER(s4_lamprow_w);
static WRITE_HANDLER(s4_specsol2_w);
static WRITE_HANDLER(s4_specsol1_w);
static WRITE_HANDLER(s4_sol1_8_w);
static WRITE_HANDLER(s4_sol9_16_w);
static WRITE_HANDLER(s4_specsol5_w);
static WRITE_HANDLER(s4_gameon_w);

/*********************/
/*DIPS SWITCHES?     */
/*********************/
/*Dips are returned to a4-7 and are read by pulsing of a0-3 (same as alpapos).
  Dips are matrixed.. Column 0 = Data Switches 1-4, Column 1 = Data Switches 5-8..
  		      Column 2 = Function Switches 1-4, Column 3 = Function Switches 5-8
 */
READ_HANDLER(s4_dips_r)
{
  /* 0x01=0, 0x02=1, 0x04=2, 0x08=3 */
  int dipcol = (s4locals.dipcol & 0x0c) ? ((s4locals.dipcol>>2)+1) :
	                                         s4locals.dipcol>>1;
  return (core_getDip(dipcol/2)<<(4*(1-(dipcol&0x01)))) & 0xf0;
}

/*********************/
/*DIAGNOSTIC SWITCHES*/
/*********************/
/*ADVANCE SWITCH - (CA1)*/
READ_HANDLER(s4_diagsw1_r)
{
int data = cpu_get_reg(M6800_IRQ_STATE) ?(core_getSw(S4_SWADVANCE)) : 0; 
//logerror("s4_diagsw1_r: data = %x \n", data);
return data;
}

/*AUTO/MANUAL SWITCH - (CB1)*/
READ_HANDLER(s4_diagsw2_r)
{ 
int data = cpu_get_reg(M6800_IRQ_STATE) ?(core_getSw(S4_SWUPDN)) : 0;
//logerror("s4_diagsw2_r: data = %x \n", data);
return data;
}

/***********************************/
/*ENTER                            */
/***********************************/
READ_HANDLER(s4_entersw_r)
{
int data = core_getSw(S4_ENTER);
logerror("s4_entersw_r: data = %x \n", data);
return data;
}

/********************/
/*SWITCH MATRIX     */
/********************/
READ_HANDLER(s4_swcol_r)
{
logerror("s4_swcol_r\n");
return 0;
}
READ_HANDLER(s4_swrow_r) { return core_getSwCol(s4locals.swCol); }
WRITE_HANDLER(s4_swcol_w)
{
//logerror("s4_swcol_w: data = %x \n", data);
s4locals.swCol = data;
}



/*****************/
/*DISPLAY ALPHA  */
/*****************/
WRITE_HANDLER(s4_alpha_w)
{
s4locals.segments[0][s4locals.alphapos].lo |=
s4locals.pseg[0][s4locals.alphapos].lo = core_bcd2seg[data>>4];
s4locals.segments[1][s4locals.alphapos].lo |=
s4locals.pseg[1][s4locals.alphapos].lo = core_bcd2seg[data&0x0f];
}

WRITE_HANDLER(s4_pa_w)
{
char led1, led2;
/*DIAG LED IS PA-4 & PA-5*/
led2 = (data>>4)&0x01;
led1 = (data>>5)&0x01;
s4locals.diagnosticLed = ((led2<<1) | led1);

/*Blanking? PA-2*/

/*ALPHA POSITION - PA0-PA3*/
data &= 0x0f;
//logerror("alphapos: %x blanking?:%x \n",data,(data>>2)&0x01);

/*Store Dips Column Enable = Same as Digit Select PA0-PA3*/
s4locals.dipcol = data;

/*Adjust alpha digit select for proper positioning!:
  We need to modify the strobe from it's current ordering:
  1,2,3,4,5,6,9,10,11,12,13,14,15,16,7,8 to regular 1..16 order!*/
if(data >= 6  && data <= 7)
 data+=8;
else
 if(data >= 8)
   data-=2;
s4locals.alphapos = data;
}

/*****************/
/* LAMP HANDLING */
/*****************/
READ_HANDLER(s4_lamprow_r);
READ_HANDLER(s4_lamprow_r)
{
logerror("s4_lamprow_r\n");
return 0;
}

READ_HANDLER(s4_lampcol_r);
READ_HANDLER(s4_lampcol_r)
{
logerror("s4_lampcol_r\n");
return 0;
}

WRITE_HANDLER(s4_lampcol_w) { core_setLamp(s4locals.lampMatrix, s4locals.lampColumn = data, s4locals.lampRow); }
WRITE_HANDLER(s4_lamprow_w) { core_setLamp(s4locals.lampMatrix, s4locals.lampColumn, s4locals.lampRow = ~data); }

/**************************/
/* REGULAR SOLENOIDS #1-8 */
/**************************/
WRITE_HANDLER(s4_sol1_8_w)
{
//logerror("s4_sol1_8_w: data = %x \n", data);
coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffffff00) | data;
s4locals.solenoids |= data;
}

/***********************************************************/
/* REGULAR SOLENOIDS #9-16 - USED AS SOUND COMMANDS! (9-14)*/
/***********************************************************/
WRITE_HANDLER(s4_sol9_16_w)
{
//logerror("s4_sol9_16_w: data = %x \n", data);
//snd_cmd_w(0,data&0x1f);		/*Sound Command is bits 1-5!*/
//logerror("snd_cmd_w: data = %x \n", data&0x1f);
s67s_cmd(0, ~data); data &= 0xe0; /* mask of sound command bits */
coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xffff00ff) | (data<<8);
s4locals.solenoids |= (data<<8);
}


/**********************************/
/* SPECIAL SOLENOID CONTROL #1-#6 */
/**********************************/
static void setSSSol(int data, int solNo) {
  int bit = CORE_SOLBIT(CORE_FIRSTSSSOL + solNo);
  if (s4locals.ssEn & (~data & 1)) { coreGlobals.pulsedSolState |= bit;  s4locals.solenoids |= bit; }
  else                               coreGlobals.pulsedSolState &= ~bit;
}
WRITE_HANDLER(s4_specsol1_w)
{
//logerror("s4_specsol1_w: data = %x \n", data);
setSSSol(data, 0);
}
WRITE_HANDLER(s4_specsol2_w)
{
//logerror("s4_specsol2_w: data = %x \n", data);
setSSSol(data, 1);
}
WRITE_HANDLER(s4_specsol3_w)
{
//logerror("s4_specsol3_w: data = %x \n",data);
setSSSol(data, 2);
}
WRITE_HANDLER(s4_specsol4_w)
{
//logerror("s4_specsol4_w: data = %x \n", data);
setSSSol(data, 3);
}
WRITE_HANDLER(s4_specsol5_w)
{
//logerror("s4_specsol5_w: data = %x \n", data);
setSSSol(data, 4);
}
WRITE_HANDLER(s4_specsol6_w)
{
//logerror("s4_specsol6_w: data = %x \n", data);
setSSSol(data, 5);
}


/*******************/
/* GAME ON SIGNAL? */
/*******************/
WRITE_HANDLER(s4_gameon_w)
{
logerror("s4_gameon_w: data = %x \n", data);
s4locals.ssEn = data;
}

/* UNHANDLED - NOT DOCUMENTED */
static READ_HANDLER(pia1_b_r);
static READ_HANDLER(pia1_b_r)
{
logerror("pia1_b_r \n");
return 0;
}
static READ_HANDLER(pia1_cb2_r);
static READ_HANDLER(pia1_cb2_r)
{
logerror("pia1_cb2_r \n");
return 0;
}

static WRITE_HANDLER(pia1_ca2_w);
static WRITE_HANDLER(pia1_ca2_w)
{
logerror("pia1_ca2_w: data = %x \n",data);
}

static READ_HANDLER(pia4_a_r);
static READ_HANDLER(pia4_a_r)
{
	logerror("pia4_a_r \n");
return 0;
}
static READ_HANDLER(pia4_b_r);
static READ_HANDLER(pia4_b_r)
{
	logerror("pia4_b_r \n");
return 0;
}

static WRITE_HANDLER(pia2_a_w);
static WRITE_HANDLER(pia2_a_w)
{
	logerror("pia2_a_w: data %x \n",data);
}

static void pia1_irqa(int state);
static void pia1_irqa(int state)
{
logerror("pia1_irqa \n");
}
static void pia1_irqb(int state);
static void pia1_irqb(int state)
{
logerror("pia1_irqb \n");
}

static void s4s_piaIrq(int state);
static void s4s_piaIrq(int state)
{
	/* IRQ to the sound CPU */
	cpu_set_irq_line(1, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
//	logerror("irq\n");
}

/*PIA 1 - Display and other*/
/*PIA I:
(out)	PA0-3: 	16 Digit Strobing.. 1 # activates a digit.
(out)	PA4-5:  Output LED's - When CA2 Output = 1
(in)	PA4-7: 	16 Functional Switches (Dips?)
(out)	PB0-7: 	BCD1&BCD2 Outputs..
(in)	CA1:	Diagnostic Switch - On interrupt: Advance Switch
(in)	CB1:	Diagnostic Switch - On interrupt: Auto/Manual Switch
(in)	CA2:   	Read Master Command Enter Switch
(out)	CB2:   	Special Solenoid #6

	Display: 5 x (7 Segment LEDS) (Player 1-4 & Status)
	BCD1: Feeds Player 1, 2, and Status
	BCD2: Feeds Player 3, 4
	Alpha Pos (L-R): 1,2,3,4,5,6
	Strobing: 1-6 = (P1&3)1-6
		  7,8,16 = (Status)5,6,(2&3)
		  9-14 = (P2&4)1-6
*/
static struct pia6821_interface pia_1_intf =
{
	/*inputs : A/B,CA1/B1,CA2/B2 */ s4_dips_r, pia1_b_r, s4_diagsw1_r, s4_diagsw2_r, s4_entersw_r, pia1_cb2_r,
	/*outputs: A/B,CA2/B2       */  s4_pa_w, s4_alpha_w, pia1_ca2_w, s4_specsol6_w,
	/*irqs   : A/B             */ pia1_irqa, pia1_irqb
};
/*PIA 2 - Switch Matrix*/
/*PIA II:
(in)	PA0-7:	Switch Rows
(in)	PB0-7:	Switch Columns??
(out)	PB0-7:  Switch Column Enable
	CA1:	None
	CB1:	None
(out)	CA2:	Special Solenoid #4
(out)	CB2:	Special Solenoid #3
*/
static struct pia6821_interface pia_2_intf =
{
	/*inputs : A/B,CA1/B1,CA2/B2 */ s4_swrow_r, s4_swcol_r, 0, 0, 0, 0,
	/*outputs: A/B,CA2/B2       */  pia2_a_w, s4_swcol_w, s4_specsol4_w,s4_specsol3_w,
	/*irqs   : A/B             */ 0, 0
};
/*PIA 3 - Lamp Matrix*/
/*
PIA III:
(out)	PA0-7:	Lamp Rows
(out)	PB0-7:	Lamp Columns
(in)    PB0-7:  Lamp Column (input????!!)
	CA1:	None
	CB1:	None
(out)	CA2:	Special Solenoid #2
(out)	CB2:	Special Solenoid #1
*/
static struct pia6821_interface pia_3_intf =
{
	/*inputs : A/B,CA1/B1,CA2/B2 */ s4_lamprow_r, s4_lampcol_r, 0, 0, 0, 0,
	/*outputs: A/B,CA2/B2       */  s4_lamprow_w, s4_lampcol_w, s4_specsol2_w,s4_specsol1_w,
	/*irqs   : A/B             */ 0, 0
};
/*PIA 4 - Standard Solenoids*/
/*
PIA IV:	
	PA0-7:	Solenoids 1-8  
	PB0-7:	Solenoids 9-16 - Sound Commands!
	CA1:	None
	CB1:	None
	CA2:	Special Solenoid #5
	CB2:	Game On Signal
*/
static struct pia6821_interface pia_4_intf =
{
	/*inputs : A/B,CA1/B1,CA2/B2 */ pia4_a_r, pia4_b_r, 0, 0, 0, 0,
	/*outputs: A/B,CA2/B2       */ s4_sol1_8_w, s4_sol9_16_w,s4_specsol5_w, s4_gameon_w,
	/*irqs   : A/B             */ 0, 0
};

/*PIA 5 Sound Board PIA*/
static struct pia6821_interface pia_5_intf = {
 /* PA0 -	DAC */
 /* PB0 -	N/A*/
 /* CA1     	N/A */
 /* CB1     	N/A */
 /* CA2     	CVSD*/
 /* CB2     	CLOCK */
 /* in  : A/B,CA/B1,CA/B2 */   0, 0, 0, 0, 0, 0,
 /* out : A/B,CA/B2       */   DAC_0_data_w, 0,hc55516_0_digit_w, hc55516_0_clock_w,
 /* irq : A/B             */   s4s_piaIrq, s4s_piaIrq
};


static int s4_irq(void) {
  cpu_set_irq_line(0, M6800_IRQ_LINE, PULSE_LINE);
  return 0;
}

static int s4_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  s4locals.vblankCount += 1;

  /*-- lamps --*/
  if ((s4locals.vblankCount % S4_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, s4locals.lampMatrix, sizeof(s4locals.lampMatrix));
    memset(s4locals.lampMatrix, 0, sizeof(s4locals.lampMatrix));
  }

  /*-- solenoids --*/
  if ((s4locals.vblankCount % S4_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = s4locals.solenoids;
    if (s4locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    s4locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((s4locals.vblankCount % S4_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, s4locals.segments, sizeof(coreGlobals.segments));
    memcpy(s4locals.segments, s4locals.pseg, sizeof(s4locals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = s4locals.diagnosticLed;
    s4locals.diagnosticLed = 0;

  }
  core_updateSw(s4locals.ssEn);
  return 0;
}


static void s4_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[1] = inports[S4_COMINPORT] & 0x00ff;
    coreGlobals.swMatrix[0] = (inports[S4_COMINPORT] & 0xff00)>>8;
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(S4_SWCPUDIAG))   cpu_set_nmi_line(0, PULSE_LINE);
  if (core_getSw(S4_SWSOUNDDIAG)) cpu_set_nmi_line(1, PULSE_LINE);

  /*-- coin door switches --*/
  pia_set_input_ca1(0, core_getSw(S4_SWADVANCE));
  pia_set_input_cb1(0, core_getSw(S4_SWUPDN));

  /* Show Status of Auto/Manual Switch */
  if(core_getSw(S4_SWUPDN))
          core_textOutf(40, 30, BLACK, "%-7s","Auto");
  else
          core_textOutf(40, 30, BLACK, "%-7s","Manual");
}



static core_tData s4Data = {
  16, /* 16 Dips */
  s4_updSw,
  2 | DIAGLED_VERTICAL,
  s67s_cmd,
  "s4"
};

static void s4_init(void) {
  if (s4locals.initDone)
    s4_exit();
  s4locals.initDone = TRUE;

  if (core_init(&s4Data)) return;
  memset(&s4locals, 0, sizeof(s4locals));

  /* init PIAs */
  CONFIGURE_PIAS4(pia_1_intf, pia_2_intf, pia_3_intf, pia_4_intf);
  pia_reset();

  s4locals.vblankCount = 1;

  if (coreGlobals.soundEn) s67s_init();
  pia_reset();

}

static void s4s_init(void){
	s4_init();
	pia_config(4, PIA_STANDARD_ORDERING, &pia_5_intf);
	pia_reset();
}

static void s4_exit(void) {
  if (coreGlobals.soundEn) s67s_exit();
  core_exit();
}

static WRITE_HANDLER(s4_CMOS_w) { s4_CMOS[offset] = data | 0xf0; }

/*-----------------------------------
/  Memory map for Sys 4 CPU board
/------------------------------------*/
static MEMORY_READ_START(s4_readmem)
  { 0x0000, 0x01ff, MRA_RAM},
  { 0x2200, 0x2203, pia_3_r },		/*Solenoids + Sound Commands*/
  { 0x2400, 0x2403, pia_2_r }, 		/*Lamps*/
  { 0x2800, 0x2803, pia_0_r },		/*Display + Other*/
  { 0x3000, 0x3003, pia_1_r },		/*Switches*/
  { 0x6000, 0x8000, MRA_ROM },
  { 0x8000, 0xffff, MRA_ROM },		/*Doubled ROM region since only 15 address pins used!*/
MEMORY_END

static MEMORY_WRITE_START(s4_writemem)
  { 0x0000, 0x00ff, MWA_RAM },
{ 0x0100, 0x01ff, s4_CMOS_w, &s4_CMOS },
  { 0x2200, 0x2203, pia_3_w },		/*Solenoids + Sound Commands*/
  { 0x2400, 0x2403, pia_2_w }, 		/*Lamps*/
  { 0x2800, 0x2803, pia_0_w },		/*Display + Other*/
  { 0x3000, 0x3003, pia_1_w },		/*Switches*/
  { 0x6000, 0x8000, MWA_ROM },
  { 0x8000, 0xffff, MWA_ROM },		/*Doubled ROM region since only 15 address pins used!*/
MEMORY_END

struct MachineDriver machine_driver_s4 = {
    {
     {
      CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      s4_readmem, s4_writemem, NULL, NULL,
      s4_vblank, 1,
      s4_irq, S4_IRQFREQ
     }
  },
  S4_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  s4_init,CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  s4_nvram
};

struct MachineDriver machine_driver_s4s= {
    {{
      CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      s4_readmem, s4_writemem, NULL, NULL,
      s4_vblank, 1,
      s4_irq, S4_IRQFREQ
     },S67S_SOUNDCPU
	},
  S4_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  s4s_init,CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, gen_refresh,
  0,0,0,0, { S67S_SOUND },
  s4_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
void s4_nvram(void *file, int write) {
  UINT8 *mem = s4_CMOS;
  if (write)     osd_fwrite(file, mem, 0x0100); /* save */
  else if (file) osd_fread( file, mem, 0x0100); /* load */
  else           memset(mem, 0xff, 0x0100);     /* first time */
}
