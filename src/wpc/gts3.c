/************************************************************************************************
  Gottlieb Pinball - System 3

  Hardware from 1992??-1996


  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET
*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "machine/6522via.h"
#include "core.h"
#include "dedmd.h"
#include "gts3.h"
#include "vidhrdw/crtc6845.h"

#define GTS3_VBLANKFREQ      60 /* VBLANK frequency */
#define GTS3_IRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define GTS3_DMDFIRQFREQ	    125 /* FIRQ Handler (Guessed, but seems to show all animations)*/

#define GTS3_CPUNO	0

static void GTS3_init(void);
static void GTS3_exit(void);
static void GTS3_nvram(void *file, int write);
static void dmdswitchbank(void);
static void dmdfirq(int data);
static WRITE_HANDLER(dmd_w);
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
} GTS3locals;

struct {
  int	 pa0;
  int	 pa1;
  int	 pa2;
  int	 pa3;
  int	 a18;
  int	 dmd_latch;
  
} GTS3_dmdlocals;

/* U4 */

//PA0-7 Switch Rows/Returns
static READ_HANDLER( xvia_0_a_r )
{
	logerror("SWITCH ROWS: via_0_a_r\n");
	return core_getSwCol(GTS3locals.swCol);
	//return 0;
}

/*
PB3:  Test Switch
PB4:  Tilt Switch
PB5-  A1P3-3 - Display Controller - Status1??
PB6-  A1P3-1 - Display Controller - Not used?
PB7-  A1P3-2 - Display Controller - Status2??
*/
static READ_HANDLER( xvia_0_b_r )
{
	logerror("TEST,TILT,& DMD Status: via_0_b_r\n");
	return 0;
}
//CA1:  Slam Switch
static READ_HANDLER( xvia_0_ca1_r )
{
	logerror("SLAM: via_0_ca1_r\n");
	return 0;
}

//CA2:  To A1P6-12 & A1P7-6 Auxiliary (INPUT???)
static READ_HANDLER( xvia_0_ca2_r )
{
	logerror("NA?: via_0_ca2_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb1_r )
{
	logerror("NA?: via_0_cb1_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb2_r )
{
	logerror("NA?: via_0_cb2_r\n");
	return 0;
}

static WRITE_HANDLER( xvia_0_a_w )
{
	logerror("NA?: via_0_a_w: %x\n",data);
}
static WRITE_HANDLER( xvia_0_b_w )
{
	logerror("LAMP DATA: via_0_b_w: %x\n",data);
}
//AUX DATA? See ca2 above!
static WRITE_HANDLER( xvia_0_ca2_w )
{
	logerror("AUX W??:via_0_ca2_w: %x\n",data);
}

//CB2:  NMI to Main CPU
static WRITE_HANDLER( xvia_0_cb2_w )
{
	logerror("NMI: via_0_cb2_w: %x\n",data);
	cpu_cause_interrupt(0,M65C02_INT_NMI);
}

/* U5 */
static READ_HANDLER( xvia_1_a_r )
{
	logerror("via_1_a_r\n");
	return 0;
}
static READ_HANDLER( xvia_1_b_r )
{
	logerror("via_1_b_r\n");
	return 0;
}
static READ_HANDLER( xvia_1_ca1_r )
{
	logerror("via_1_ca1_r\n");
	return 0;
}
static READ_HANDLER( xvia_1_ca2_r )
{
	logerror("via_1_ca2_r\n");
	return 0;
}
static READ_HANDLER( xvia_1_cb1_r )
{
	logerror("via_1_cb1_r\n");
	return 0;
}
static READ_HANDLER( xvia_1_cb2_r )
{
	logerror("via_1_cb2_r\n");
	return 0;
}

static WRITE_HANDLER( xvia_1_a_w )
{
	logerror("via_1_a_w: %x\n",data);
}
static WRITE_HANDLER( xvia_1_b_w )
{
	logerror("via_1_b_w: %x\n",data);
}
static WRITE_HANDLER( xvia_1_ca2_w )
{
	logerror("via_1_ca2_w: %x\n",data);
}
static WRITE_HANDLER( xvia_1_cb2_w )
{
	logerror("via_1_cb2_w: %x\n",data);
}

//IRQ:  IRQ to Main CPU
static void via_irq(int state) {
	logerror("via_irq\n");
//    cpu_set_irq_line(0, M6502_INT_IRQ, PULSE_LINE);
	cpu_cause_interrupt(0,M65C02_INT_IRQ);
}

/*

U4:
---
PA0-7 Switch Rows/Returns
CA1:  Slam Switch
CA2:  To A1P6-12 & A1P7-6 Auxiliary
PB0:  Lamp Data
PB1:  Lamp Strobe
PB2:  Lamp Clear 
PB3:  Test Switch
PB4:  Tilt Switch
PB5-  A1P3-3 - Display Controller - Status1??
PB6-  A1P3-1 - Display Controller - Not used?
PB7-  A1P3-2 - Display Controller - Status2??
CB2:  NMI to Main CPU
IRQ:  IRQ to Main CPU

U5:
--
PA0-7: SD0-7 - A1P4 (Sound data)
CA1:   Sound Return?
CA2:   LED
PB0-7: DX0-7 - A1P6 (Auxilary)
CB1:   CX1 - A1P6 (Auxilary)
CB2:   CX2 - A1P6 (Auxilary)

*/
static struct via6522_interface via_0_interface =
{
	/*inputs : A/B         */ xvia_0_a_r, xvia_0_b_r,
	/*inputs : CA/B1,CA/B2 */ xvia_0_ca1_r, xvia_0_cb1_r, xvia_0_ca2_r, xvia_0_cb2_r,
	/*outputs: A/B,CA/B2   */ xvia_0_a_w, xvia_0_b_w, xvia_0_ca2_w, xvia_0_cb2_w,
	/*irq                  */ via_irq
};
static struct via6522_interface via_1_interface =
{
	/*inputs : A/B         */ xvia_1_a_r, xvia_1_b_r,
	/*inputs : CA/B1,CA/B2 */ xvia_1_ca1_r, xvia_1_cb1_r, xvia_1_ca2_r, xvia_1_cb2_r,
	/*outputs: A/B,CA/B2   */ xvia_1_a_w, xvia_1_b_w, xvia_1_ca2_w, xvia_1_cb2_w,
	/*irq                  */ via_irq
};


static int com = 0;

static int GTS3_irq(void) {
  //if (GTS3locals.mainIrq == 0) /* Don't send IRQ if already active */
  //  cpu_set_irq_line(GTS3_CPUNO, M6809_IRQ_LINE, HOLD_LINE);
  //return 0;
  //logerror("firq?\n");
//  cpu_set_irq_line(GTS3_CPUNO, M6809_FIRQ_LINE,PULSE_LINE);
	//cpu_set_irq_line(GTS3_CPUNO, M6809_FIRQ_LINE,HOLD_LINE);
  logerror("STEVE\n");
  dmd_w(0,++com);
  return ignore_interrupt();	//NO INT OR NMI GENERATED!
}

static int GTS3_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  GTS3locals.vblankCount += 1;
  /*-- lamps --*/
  if ((GTS3locals.vblankCount % GTS3_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, GTS3locals.lampMatrix, sizeof(GTS3locals.lampMatrix));
    memset(GTS3locals.lampMatrix, 0, sizeof(GTS3locals.lampMatrix));
  }
  /*-- solenoids --*/
  if ((GTS3locals.vblankCount % GTS3_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = GTS3locals.solenoids;
    if (GTS3locals.ssEn) {
      int ii;
      coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
      /*-- special solenoids updated based on switches --*/
      for (ii = 0; ii < 6; ii++)
        if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
          coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
    }
    GTS3locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((GTS3locals.vblankCount % GTS3_DISPLAYSMOOTH) == 0) {
    memcpy(coreGlobals.segments, GTS3locals.segments, sizeof(coreGlobals.segments));
    memcpy(GTS3locals.segments, GTS3locals.pseg, sizeof(GTS3locals.segments));

    /*update leds*/
    coreGlobals.diagnosticLed = GTS3locals.diagnosticLed;
    GTS3locals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* assume flipper enabled */
  return 0;
}

static void GTS3_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[1] = inports[GTS3_COMINPORT] & 0x00ff;
    coreGlobals.swMatrix[0] = (inports[GTS3_COMINPORT] & 0xff00)>>8;
  }
}

static WRITE_HANDLER(GTS3_sndCmd_w) {
}

static core_tData SEData = {
  0, /* No DIPs */
  GTS3_updSw,
  1,
  GTS3_sndCmd_w,
  "SE"
};

static void GTS3_init(void) {
  if (GTS3locals.initDone)
    GTS3_exit();
  GTS3locals.initDone = TRUE;

  memset(&GTS3_dmdlocals, 0, sizeof(GTS3_dmdlocals));

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  /*Set DMD to point to our DMD memory*/
  coreGlobals_dmd.DMDFrames[0] = memory_region(GTS3_MEMREG_DCPU1);

  /*DMD*/
  /*copy last 32K of ROM into last 32K of CPU region*/
  /*Setup ROM Swap so opcode will work*/
  if(memory_region(GTS3_MEMREG_DCPU1))
  {
  memcpy(memory_region(GTS3_MEMREG_DCPU1)+0x8000,
  memory_region(GTS3_MEMREG_DROM1) + 
  (memory_region_length(GTS3_MEMREG_DROM1) - 0x8000), 0x8000);
  }
  GTS3_dmdlocals.pa0 = GTS3_dmdlocals.pa1 = GTS3_dmdlocals.pa2 = GTS3_dmdlocals.pa3 = 
  GTS3_dmdlocals.a18 = 1;
  dmdswitchbank();

  //Manually call the DMD FIRQ at the specified rate
  timer_pulse(TIME_IN_HZ(GTS3_DMDFIRQFREQ), 0, dmdfirq);

  if (core_init(&SEData))
	  return;
}

static void GTS3_exit(void) {
  core_exit();
}


/*Switchs*/
static READ_HANDLER(switch_r)	{ return core_getSwCol(GTS3locals.swCol); }
static WRITE_HANDLER(switch_w)	{ GTS3locals.swCol = data; }

/*Need Info on Dedicated Switches*/
static READ_HANDLER(dedswitch_r)
{
	return 0xff;
}
/*Need Info on Dip Switches*/
static READ_HANDLER(dipswitch_r)
{
	return 0xff;
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	switch(offset){
		case 0:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
            break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
            break;
		case 3:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
			break;
		default:
			logerror("Solenoid_W Logic Error\n");
	}
}
static WRITE_HANDLER(auxlamp_w)
{
	logerror("Aux Lamp Write: Offset: %x Data: %x\n",offset,data);
}

/*AUX BOARDS*/
static WRITE_HANDLER(auxboard_w)
{
	logerror("Aux Board Write: Offset: %x Data: %x\n",offset,data);
}
static WRITE_HANDLER(giaux_w)
{
	logerror("GI/Aux Board Write: Offset: %x Data: %x\n",offset,data);
}

/*DMD*/
static READ_HANDLER(dmdie_r)
{
	logerror("DMD Input Enable Read: Offset: %x\n",offset);
	return 0x00;
}
static WRITE_HANDLER(dmdoe_w)
{
	logerror("DMD Output Enable Write: Offset: %x Data: %x\n",offset,data);
}
static READ_HANDLER(status_r)
{
	return 0x01;
}

/*LAMPS*/
static WRITE_HANDLER(lampdriv_w) { core_setLamp(GTS3locals.lampMatrix, GTS3locals.lampColumn = data, GTS3locals.lampRow);}
static WRITE_HANDLER(lampstrb_w) { core_setLamp(GTS3locals.lampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow = data);}

static void dmdswitchbank(void)
{
	int	addr =	(GTS3_dmdlocals.pa0 *0x04000)+
				(GTS3_dmdlocals.pa1 *0x08000)+ 
				(GTS3_dmdlocals.pa2 *0x10000)+
 				(GTS3_dmdlocals.pa3 *0x20000)+
				(GTS3_dmdlocals.a18 *0x40000);
	if(addr>0)
		logerror("switchbank: addr=%x\n",addr);
	cpu_setbank(1, memory_region(GTS3_MEMREG_DROM1) + addr);
}

static READ_HANDLER(dmdlatch_r)
{
	logerror("Reading dmdlatch %x\n",GTS3_dmdlocals.dmd_latch);
	return GTS3_dmdlocals.dmd_latch;
}

static WRITE_HANDLER(lds_w)
{
	logerror("LDS Write: Data: %x\n",data);
}
static WRITE_HANDLER(ss_w)
{
	logerror("SS Write: Offset: %x Data: %x\n",offset,data);
}

static WRITE_HANDLER(aux_w)
{
	logerror("AUX Write: Offset: %x Data: %x\n",offset,data);
}

static WRITE_HANDLER(dmd_w)
{
	logerror("DMD Write: Offset: %x Data: %x\n",offset,data);
        GTS3_dmdlocals.dmd_latch = data;
        cpu_cause_interrupt(1,M65C02_INT_IRQ);
}

static WRITE_HANDLER(aux1_w)
{
	logerror("Aux1 Write: Offset: %x Data: %x\n",offset,data);
}

static void dmdfirq(int data)
{
logerror("dmd irq\n");
cpu_cause_interrupt(1,M65C02_INT_NMI);
}


/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_readmem)
{0x0000,0x1fff,MRA_RAM},
{0x2000,0x200f,via_0_r},
{0x2010,0x201f,via_1_r},
{0x4000,0xffff,MRA_ROM},
MEMORY_END

//Writes occur
static MEMORY_WRITE_START(GTS3_writemem)
{0x0000,0x1fff,MWA_RAM},
{0x2000,0x200f,via_0_w},
{0x2010,0x201f,via_1_w},
{0x2020,0x2023,dmd_w},
{0x2030,0x2033,ss_w},
{0x2040,0x2040,lds_w},
{0x2041,0x2043,aux_w},
{0x2050,0x2070,aux1_w},
{0x4000,0xffff,MWA_ROM},
MEMORY_END


/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_dmdreadmem)
//{0x0000,0xffff, MRA_RAM},
//{0x0000,0x00ff, MRA_RAM},
//{0x0100,0x1fff, MRA_RAM},	/*What is this?*/
//{0x2000,0x2fff, MRA_RAM},
{0x3000,0x3000, dmdlatch_r},
//{0x3001,0x3001, crtc6845_register_r},
//{0x8000,0xbfff, MRA_BANK1},
//{0xc000,0xffff, MRA_ROM},
{0x8000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem)
//{0x0000,0xffff, MWA_RAM},
//{0x0000,0x00ff, MWA_RAM},
//{0x0100,0x1fff, MWA_RAM},	/*What is this?*/
//{0x2000,0x23ff, MWA_RAM},	/*DMD RAM PAGE 0? 1K*/
//{0x2400,0x27ff, MWA_RAM},	/*DMD RAM PAGE 1? 1K*/
//{0x2800,0x2bff, MWA_RAM},	/*DMD RAM PAGE 2? 1K*/
//{0x2c00,0x2fff, MWA_RAM},
//{0x3008,0x3fff, MWA_RAM},
{0x2800,0x2800, crtc6845_address_w},
{0x2801,0x2801, crtc6845_register_w},
//{0x3002,0x3002, xa_w},
//{0x8000,0xbfff, MWA_BANK1},
//{0xc000,0xffff, MWA_ROM},
MEMORY_END

struct MachineDriver machine_driver_GTS3_1 = {
  {
    {
      CPU_M65C02, 2000000, /* 2 Mhz */
      GTS3_readmem, GTS3_writemem, NULL, NULL,
      GTS3_vblank, 1,
      GTS3_irq, GTS3_IRQFREQ
    },
	{
      CPU_M65C02, 3579000/2, /* 1.76? Mhz*/
      GTS3_dmdreadmem, GTS3_dmdwritemem, NULL, NULL,
      ignore_interrupt,0    /*IRQ & NMI are generated by other devices*/
    }
  },
  GTS3_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50,
  GTS3_init,CORE_EXITFUNC(NULL)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER,
  0,
  NULL, NULL, de_dmd128x32_refresh,
  0,0,0,0,{{ 0 }},
  GTS3_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
void GTS3_nvram(void *file, int write) {
  if (write)  /* save nvram */
    osd_fwrite(file, memory_region(GTS3_MEMREG_CPU), 0x2000);
  else if (file) /* load nvram */
    osd_fread(file, memory_region(GTS3_MEMREG_CPU), 0x2000);
  else        /* first time */
    memset(memory_region(GTS3_MEMREG_CPU), 0xff, 0x2000);
}
