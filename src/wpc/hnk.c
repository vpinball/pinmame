/* HANKIN Pinball*/
/* 
   Hardare is EXTREMELY SIMILAR to Bally MPU-35!
   Display, however, uses 9 Segments, rather than Bally's 7 and works *slightly* different
   There are only 3 x 8 Dip Banks (Dips 1-24), rather than Bally's 4 x 8 = 32.
*/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "machine/6821pia.h"
#include "core.h"
#include "sndbrd.h"
#include "hnk.h"
#include "hnks.h"

#define HNK_VBLANKFREQ    60 /* VBLANK frequency */
#define HNK_IRQFREQ      150 /* IRQ (via PIA) frequency*/
#define HNK_ZCFREQ        85 /* Zero cross frequency */

static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2;
  int bcd[5];	//There are 5 Displays
  int lampadr1;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
  int initDone;
  void *zctimer;
} locals;

static void hnk_exit(void);
static void hnk_nvram(void *file, int write);

static void piaIrq(int state) {
  //DBGLOG(("IRQ = %d\n",state));
  cpu_set_irq_line(0, M6800_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void hnk_dispStrobe(int mask) {
  int digit = locals.p1_a & 0xfc; //PA2-7 Selects Digits 1-6
  int ii,jj;
//  logerror("digit = %x (%x,%x,%x,%x,%x)\n",digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4]);
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
		if (dispMask & 0x01) {
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg9[locals.bcd[jj]];
		}
    }
}

static void hnk_lampStrobe(int board, int lampadr) {
  if (lampadr != 0x0f) {
    int lampdata = (locals.p0_a>>4)^0x0f;
    UINT8 *matrix = &coreGlobals.tmpLampMatrix[(lampadr>>3)+8*board];
    int bit = 1<<(lampadr & 0x07);

    //DBGLOG(("adr=%x data=%x\n",lampadr,lampdata));
    while (lampdata) {
      if (lampdata & 0x01) *matrix |= bit;
      lampdata >>= 1; matrix += 2;
    }
  }
}

static WRITE_HANDLER(hnk_sndCmd_w) {
	// logerror("sound cmd: 0x%02x\n", data);
	if ( Machine->gamedrv->flags & GAME_NO_SOUND )
		return;

    sndbrd_0_data_w(0, data);
}

/* PIA0:A-W  Control what is read from PIA0:B 
-----------------------------------------------
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-4: Switch Strobe(Columns)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-3: Display Latch Strobe/Enable #1-4
(out) PA5:   Dip Bank Enable 1 (S01-S08)
(out) PA6:   Dip Bank Enable 2 (S09-S16)
(out) PA7:   Dip Bank Enable 3 (S17-S24)
(out) PA4-7: BCD Lamp Data & BCD Display Data
*/
static WRITE_HANDLER(pia0a_w) {
  if (!locals.p0_ca2) {		// If Display Blanking cleared
    int bcdLoad = locals.p0_a & ~data & 0x0f;	//Positive Edge
    int ii;
    for (ii = 0; bcdLoad; ii++, bcdLoad>>=1)
      if (bcdLoad & 0x01) locals.bcd[ii] = data>>4;
    hnk_dispStrobe(data&0x0f);
  }
  locals.p0_a = data;
//  logerror("bcd data=%x\n",data);
  hnk_lampStrobe(0,locals.lampadr1);
}
/* PIA1:A-W  0   Display Latch #5
			 1   N/A
			 2-7 Display Digit Select 1-6
*/
static WRITE_HANDLER(pia1a_w) {
  int tmp = locals.p1_a;
//  logerror("digit_w: %x\n",data);
  locals.p1_a = data;
//  logerror("setting digit to %x\n",locals.p1_a);
  return;
  //if (!locals.p0_ca2) {		  // If Display Blanking cleared
    if (tmp & ~data & 0x01) { // Positive Edge
//	  logerror("Latch #5: ca2 = %x, p0_a = %x\n",locals.p0_ca2,locals.p0_a);
      locals.bcd[4] = locals.p0_a>>4;
      hnk_dispStrobe(0x10);
    }
//  }
}

/* PIA0:B-R  Get Data depending on PIA0:A */
static READ_HANDLER(pia0b_r) {
  if (locals.p0_a & 0x20) return core_getDip(0); // DIP#1 1-8
  if (locals.p0_a & 0x40) return core_getDip(1); // DIP#2 9-16
  if (locals.p0_a & 0x80) return core_getDip(2); // DIP#3 17-24

  if ( (locals.p1_b & 0x80)>>2 )
	locals.p1_b = locals.p1_b;
  return core_revbyte(core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x80)>>2)));
}

/* PIA0:CB2-W Lamp Strobe #1 */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}

/* PIA1:CA2-W Diagnostic LED & Audio Strobe */
static WRITE_HANDLER(pia1ca2_w) {
  locals.diagnosticLed = data;
  hnk_sndCmd_w(0, locals.p1_b & 0x0f);
}

/* PIA0:CA2-W Display Blanking */
static WRITE_HANDLER(pia0ca2_w) {
  //DBGLOG(("PIA0:CA2=%d\n",data));
  locals.p0_ca2 = data;
  if (!data) hnk_dispStrobe(0x1f);
}

/* PIA1:B-W Solenoid Data & Sound Data (PB0-3) */
static WRITE_HANDLER(pia1b_w) {
  locals.p1_b = data;
  coreGlobals.pulsedSolState = 0;
  if (!locals.p1_cb2)
    locals.solenoids |= coreGlobals.pulsedSolState = (1<<(data & 0x0f)) & 0x7fff;
  data ^= 0xf0;
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xfff0ffff) | ((data & 0xf0)<<12);
  locals.solenoids |= (data & 0xf0)<<12;
}

/* PIA1:CB2-W Solenoid Bank Select */
static WRITE_HANDLER(pia1cb2_w) {
  //DBGLOG(("PIA1:CB2=%d\n",data));
  locals.p1_cb2 = data;
}

static int hnk_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % HNK_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % HNK_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % HNK_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, locals.segments, sizeof(coreGlobals.segments));
    memcpy(locals.segments, locals.pseg, sizeof(locals.segments));
    memset(locals.pseg,0,sizeof(locals.pseg));
    /*update leds*/
    coreGlobals.diagnosticLed = locals.diagnosticLed;
    locals.diagnosticLed = 0;
  }
  core_updateSw(core_getSol(19));
  return 0;
}

static void hnk_updSw(int *inports) {
  if (inports) {
      coreGlobals.swMatrix[0] = (inports[HNK_COMINPORT]>>5) & 0x07;
      coreGlobals.swMatrix[1] =   (coreGlobals.swMatrix[1] & 0xf9)
								| ((inports[HNK_COMINPORT]<<1) & 0x06);
      coreGlobals.swMatrix[2] =   (coreGlobals.swMatrix[2] & 0x7e)
								| ((inports[HNK_COMINPORT]>>2) & 0x01)
								| ((inports[HNK_COMINPORT]<<4) & 0x80);
      coreGlobals.swMatrix[5] =   (coreGlobals.swMatrix[5] & 0x7e)
								| ((inports[HNK_COMINPORT]<<3) & 0x80);

//	  coreGlobals.swMatrix[0] = (inports[HNK_COMINPORT]>>10) & 0x07;
//    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x60)) |
//                              ((inports[HNK_COMINPORT]<<5) & 0x60);
//    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0x87)) |
//                                ((inports[HNK_COMINPORT]>>2) & 0x87);
  }
  /*-- Diagnostic buttons on CPU board --*/
  if (core_getSw(HNK_SWCPUDIAG))  cpu_set_nmi_line(0, PULSE_LINE);
  /*-- coin door switches --*/
  pia_set_input_ca1(0, !core_getSw(HNK_SWSELFTEST));
}

/*PIA 1*/
/*PIA U10:
(in)  PB0-7: Switch Returns/Rows and Cabinet Switch Returns/Rows 
(in)  PB0-7: Dip Returns
(in)  CA1:   Self Test Switch
(in)  CB1:   Zero Cross Detection
(in)  CA2:   N/A
(out) PA0-1: Cabinet Switches Strobe (shared below)
(out) PA0-4: Switch Strobe(Columns)
(out) PA0-3: Lamp Address (Shared with Switch Strobe)
(out) PA0-3: Display Latch Strobe/Enable #1-4
(out) PA5:   Dip Bank Enable 1 (S01-S08)
(out) PA6:   Dip Bank Enable 2 (S09-S16)
(out) PA7:   Dip Bank Enable 3 (S17-S24)
(out) PA4-7: BCD Lamp Data & BCD Display Data
(out) CA2:   Display Blanking/(Select?)
(out) CB2:   Lamp Strobe
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/
/*PIA 2*/
/*PIA U11:
(in)  PA0-7	 N/A?
(in)  PB0-7  N/A?
(in)  CA1:   N/A
(in)  CB1:	 TEST?
(in)  CA2:	 N/A?
(in)  CB2:	 N/A?
(out) PA0:	 Display Latch Strobe/Enable #5
(out) PA1:	 N/A
(out) PA2-7: Display Digit Select
(out) PB0-3: Momentary Solenoid/Sound Data
(out) PB4-7: Continuous Solenoid (PB-7 Marked N/U on schem, but no harm to leave it in)
(out) CA2:	 Diag LED + Audio Strobe
(out) CB2:   Solenoid Bank Select
	  IRQ:	 Wired to Main 6800 CPU IRQ.*/
static struct pia6821_interface piaIntf[] = {{
/* I:  A/B,CA1/B1,CA2/B2 */  0, pia0b_r, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia0a_w,0, pia0ca2_w,pia0cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
},{
/* I:  A/B,CA1/B1,CA2/B2 */  0,0, 0,0, 0,0,
/* O:  A/B,CA2/B2        */  pia1a_w,pia1b_w,pia1ca2_w,pia1cb2_w,
/* IRQ: A/B              */  piaIrq,piaIrq
}};

static int hnk_irq(void) { return 0; }

static core_tData hnkData = {
  24, /* 24 Dips */
  hnk_updSw, 1, hnk_sndCmd_w, "hnk",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

static void hnk_zeroCross(int data) {
  /*- toggle zero/detection circuit-*/
  pia_set_input_cb1(0, 0); pia_set_input_cb1(0, 1);
}
static void hnk_init(void) {
  if (locals.initDone) CORE_DOEXIT(hnk_exit);

  if (core_init(&hnkData)) return;
  memset(&locals, 0, sizeof(locals));

  /* init PIAs */
  pia_config(0, PIA_STANDARD_ORDERING, &piaIntf[0]);
  pia_config(1, PIA_STANDARD_ORDERING, &piaIntf[1]);
  
  if (coreGlobals.soundEn) 
	  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(HNK_MEMREG_SCPU), NULL, NULL);

  pia_reset();
  locals.vblankCount = 1;
  locals.zctimer = timer_pulse(TIME_IN_HZ(HNK_ZCFREQ),0,hnk_zeroCross);

  locals.initDone = TRUE;
}

static void hnk_exit(void) {
#ifdef PINMAME_EXIT
  if (locals.zctimer) { timer_remove(locals.zctimer); locals.zctimer = NULL; }
#endif
  if (coreGlobals.soundEn) 
    sndbrd_0_exit();

  core_exit();
}
static UINT8 *hnk_CMOS;
//4 top bits
static WRITE_HANDLER(hnk_CMOS_w) {
  data |= 0x0f;
  hnk_CMOS[offset] = data;
}
/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
static MEMORY_READ_START(hnk_readmem)
{ 0x0000, 0x0080, MRA_RAM }, /* U7 128 Byte Ram*/
{ 0x0200, 0x02ff, MRA_RAM }, /* CMOS Battery Backed*/
{ 0x0088, 0x008b, pia_0_r }, /* U10 PIA: Switchs + Display + Lamps*/
{ 0x0090, 0x0093, pia_1_r }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
{ 0x1000, 0x1fff, MRA_ROM },
{ 0xff00, 0xffff, MRA_ROM }, /* Reset & IRQ Vectors */
MEMORY_END

static MEMORY_WRITE_START(hnk_writemem)
{ 0x0000, 0x0080, MWA_RAM }, /* U7 128 Byte Ram*/
{ 0x0200, 0x02ff, hnk_CMOS_w, &hnk_CMOS }, /* CMOS Battery Backed*/
{ 0x0088, 0x008b, pia_0_w }, /* U10 PIA: Switchs + Display + Lamps*/
{ 0x0090, 0x0093, pia_1_w }, /* U11 PIA: Solenoids/Sounds + Display Strobe */
{ 0x1000, 0x1fff, MWA_ROM },
MEMORY_END

struct MachineDriver machine_driver_HNK = {
  {
    {
	  CPU_M6800, 3580000/4, /* 3.58/4 = 900hz */
      hnk_readmem, hnk_writemem, NULL, NULL,
      hnk_vblank, 1, hnk_irq, HNK_IRQFREQ
	},
	HNK_SOUND_CPU
  },
  HNK_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, hnk_init, CORE_EXITFUNC(hnk_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {HNK_SOUND},
  hnk_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void hnk_nvram(void *file, int write) {
  core_nvram(file, write, hnk_CMOS, 0x100,0xff);
}
