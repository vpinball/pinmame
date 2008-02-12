/************************************************************************************************
  Gottlieb Pinball - System 3

  Hardware from 1989-1996

  Earlier games used 2 - 20 column 16 segment alphanumeric displays.
  Later games used a 128x32 DMD.

  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET
  //Cueball: CPU0: RST = FEF0, IRQ=462F, NMI=477A

  GV 07/20/05: Finally found the typo that prevented the extra displays from working perfectly!
  GV 07/27/05: Fixed the missing sound in Strikes And Spares
  SE 07/27/05: Finally got that DAMN 2nd DMD working

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "machine/6522via.h"
#include "core.h"
#include "sndbrd.h"
#include "gts3.h"
#include "vidhrdw/crtc6845.h"
#include "gts3dmd.h"
#include "gts80s.h"

UINT8 DMDFrames[GTS3DMD_FRAMES][0x200];
UINT8 DMDFrames2[GTS3DMD_FRAMES][0x200];		//2nd DMD Display for Strikes N Spares

#define GTS3_VBLANKFREQ      60 /* VBLANK frequency*/
#define GTS3_IRQFREQ       1500 /* IRQ Frequency (Guessed)*/
#define GTS3_ALPHANMIFREQ  1000 /* Alpha NMI Frequency (Guessed)*/

#define GTS3_CPUNO	0
#define GTS3_DCPUNO 1
#define GTS3_SCPUNO 2
#define GTS3_DCPUNO2 2

#if 1
#define logerror1 logerror
#else
#define logerror1 printf
#endif

/* FORCE The 16 Segment Layout to match the output order expected by core.c */
static const int alpha_adjust[16] =   {0,1,2,3,4,5,9,10,11,12,13,14,6,8,15,7};

static WRITE_HANDLER(display_control);

/*Alpha Display Generation Specific*/
static WRITE_HANDLER(alpha_u4_pb_w);
static READ_HANDLER(alpha_u4_pb_r);
static WRITE_HANDLER(alpha_display);
static WRITE_HANDLER(alpha_aux);
static void alpha_update(void);
/*DMD Generation Specific*/
static void dmdswitchbank(int which);
static WRITE_HANDLER(dmd_u4_pb_w);
static READ_HANDLER(dmd_u4_pb_r);
static WRITE_HANDLER(dmd_display);
static WRITE_HANDLER(dmd_aux);
static void dmd_vblank(int which);
static void dmd_update(void);

/*----------------
/ Local variables
/-----------------*/
struct {
  int    alphagen;
  core_tSeg segments, pseg;
  int    vblankCount;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    diagnosticLeds1;
  int    diagnosticLeds2;
  int    swCol;
  int    ssEn;
  int    mainIrq;
  int	 swDiag;
  int    swTilt;
  int    swSlam;
  int    swPrin;
  int    acol;
  int    u4pb;
  WRITE_HANDLER((*U4_PB_W));
  READ_HANDLER((*U4_PB_R));
  WRITE_HANDLER((*DISPLAY_CONTROL));
  WRITE_HANDLER((*AUX_W));
  void (*UPDATE_DISPLAY)(void);
  UINT8  ax[7], cx1, cx2, ex1;
  char   extra16led;
  int    sound_data;
  UINT8  prn[8];
} GTS3locals;

typedef struct {
  int    version;
  int	 pa0;
  int	 pa1;
  int	 pa2;
  int	 pa3;
  int	 a18;
  int	 q3;
  int	 dmd_latch;
  int	 diagnosticLed;
  int	 status1;
  int	 status2;
  int    dstrb;
  UINT8  dmd_visible_addr;
  int    nextDMDFrame;
} GTS3_DMDlocals;

//We need 2 structures, since Strikes N Spares has 2 DMD Displays
GTS3_DMDlocals GTS3_dmdlocals[2];

/* U4 */

//PA0-7 Switch Rows/Returns (Switches are inverted)
static READ_HANDLER( xvia_0_a_r ) { return ~core_getSwCol(GTS3locals.lampColumn); }

//PB0-7 Varies on Alpha or DMD Generation!
static READ_HANDLER( xvia_0_b_r ) { return GTS3locals.U4_PB_R(offset); }

/* ALPHA GENERATION
   ----------------
   PB0-2: Output only
   PB3:  Slam Switch (NOTE: Test Switch on later Generations!)
   PB4:  Tilt Switch
   PB5-7: Output only
*/
static READ_HANDLER(alpha_u4_pb_r)
{
	int data = 0;
	//Gen 1 checks Slam switch here
	if(GTS3locals.alphagen==1)
		data |= (GTS3locals.swSlam <<3);	//Slam Switch (NOT INVERTED!)
	else
		data |= (GTS3locals.swDiag << 3);	//Diag Switch (NOT INVERTED!)
	data |= (GTS3locals.swTilt << 4);   //Tilt Switch (NOT INVERTED!)
	return data;
}

/* DMD GENERATION
   ----------------
   PB0-2: Output only
   PB3:  Test Switch
   PB4:  Tilt Switch
   PB5-  A1P3-3 - Display Controller - Status1 (Not labeld on Schematic)
   PB6-  A1P3-1 - Display Controller - DMD Display Strobe (DSTB) - Output only, but might be read!
   PB7-  A1P3-2 - Display Controller - Status2 (Not labeld on Schematic)
*/
static READ_HANDLER(dmd_u4_pb_r)
{
	int data = 0;
	data |= (GTS3locals.swDiag << 3);	//Diag Switch (NOT INVERTED!)
	data |= (GTS3locals.swTilt << 4);   //Tilt Switch (NOT INVERTED!)
	data |= (GTS3_dmdlocals[0].status1 << 5);
	data |= (GTS3_dmdlocals[0].dstrb << 6);
	data |= (GTS3_dmdlocals[0].status2 << 7);
	return data;
}

//CA2:  To A1P6-12 & A1P7-6 Auxiliary (INPUT???)
static READ_HANDLER( xvia_0_ca2_r )
{
	logerror1("READ: NA?: via_0_ca2_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb1_r )
{
	// logerror1("READ: NA?: via_0_cb1_r\n");
	return 0;
}
static READ_HANDLER( xvia_0_cb2_r )
{
	// logerror1("READ: NA?: via_0_cb2_r\n");
	return 0;
}

static WRITE_HANDLER( xvia_0_a_w )
{
	// logerror1("WRITE:NA?: via_0_a_w: %x\n",data);
}

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER( xvia_0_b_w ) { GTS3locals.U4_PB_W(offset,data); }

/* ALPHA GENERATION
   ----------------
  PB0:  Lamp Data      (LDATA)
  PB1:  Lamp Strobe    (LSTRB)
  PB2:  Lamp Clear     (LCLR)
  PB5:  Display Data   (DDATA)
  PB6:  Display Strobe (DSTRB)
  PB7:  Display Blank  (DBLNK)
*/
#define LDATA 0x01
#define LSTRB 0x02
#define LCLR  0x04
#define DDATA 0x20
#define DSTRB 0x40
#define DBLNK 0x80

static WRITE_HANDLER(alpha_u4_pb_w) {
	int dispBits = data & 0xf0;

	//logerror("lampcolumn=%4x STRB=%d LCLR=%d\n",GTS3locals.lampColumn,data&LSTRB,data&LCLR);
	if (data & ~GTS3locals.u4pb & LSTRB) { // Positive edge
		if ((data & LCLR) && (data & LDATA))
			GTS3locals.lampColumn = 1;
		else
			GTS3locals.lampColumn = ((GTS3locals.lampColumn << 1) & 0x0fff);
		core_setLampBlank(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);
	} else
		core_setLamp(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);

	if (dispBits == 0xe0) { GTS3locals.acol = 0; }
	else if (dispBits == 0xc0) { GTS3locals.acol++; }

	GTS3locals.u4pb = data;
}

/* DMD GENERATION
   ----------------
  PB0:  Lamp Data      (LDATA)
  PB1:  Lamp Strobe    (LSTRB)
  PB2:  Lamp Clear     (LCLR)
  PB6:  Display Strobe (DSTRB)
*/
static WRITE_HANDLER(dmd_u4_pb_w) {
	static int bitSet = 0;

	if (data & ~GTS3locals.u4pb & LSTRB) { // Positive edge
		if ((data & LCLR) && (data & LDATA))
			GTS3locals.lampColumn = 1;
		else
			GTS3locals.lampColumn = ((GTS3locals.lampColumn << 1) & 0x0fff);
		core_setLampBlank(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);
	} else
		core_setLamp(coreGlobals.tmpLampMatrix, GTS3locals.lampColumn, GTS3locals.lampRow);

	GTS3_dmdlocals[0].dstrb = (data & DSTRB) != 0;
	if (GTS3_dmdlocals[0].version) { // probably wrong, but the only way to show *any* display
		if (GTS3_dmdlocals[0].dstrb) {
			bitSet++;
			if (bitSet == 4)
			{
				dmd_vblank(0);
				if(GTS3_dmdlocals[0].version == 2)
					dmd_vblank(1);
			}
		} else
			bitSet = 0;
	}

	GTS3locals.u4pb = data;
}

//AUX DATA? See ca2 above!
static WRITE_HANDLER( xvia_0_ca2_w )
{
	logerror1("EX1: via_0_ca2_w %x\n",data);
	GTS3locals.ex1 = data;
}

//CB2:  NMI to Main CPU
static WRITE_HANDLER( xvia_0_cb2_w )
{
	//logerror1("NMI: via_0_cb2_w: %x\n",data);
	cpu_set_nmi_line(GTS3_CPUNO, PULSE_LINE);
}

/* U5 */
//Not used?
static READ_HANDLER( xvia_1_a_r )
{
	// logerror1("via_1_a_r\n");
	return 0;
}

//Data to A1P6 Auxilary - Not Used
//Strikes & Spares reads the DMD #2 status line as the 8th bit.
static READ_HANDLER( xvia_1_b_r )
{
	// logerror1("via_1_b_r\n");
	int data = 0 ;
	if(GTS3_dmdlocals[0].version == 2)
		data |= (GTS3_dmdlocals[1].status1 << 7);
	return data;
}
//CA1:   Sound Return/Status
static READ_HANDLER( xvia_1_ca1_r )
{
	// logerror1("SOUND RET READ: via_1_ca1_r\n");
	return 0;
}
//Should be NA!
static READ_HANDLER( xvia_1_ca2_r )
{
	// logerror1("via_1_ca2_r\n");
	return 0;
}
//CB1:   CX1 - A1P6 (Auxilary)
static READ_HANDLER( xvia_1_cb1_r )
{
	// logerror1("via_1_cb1_r\n");
	return 0;
}
//CB2:   CX2 - A1P6 (Auxilary)
static READ_HANDLER( xvia_1_cb2_r )
{
	// logerror1("via_1_cb2_r\n");
	return 0;
}

//PA0-7: SD0-7 - A1P4 (Sound data)
static WRITE_HANDLER( xvia_1_a_w )
{
	// logerror1("Sound Command: WRITE:via_1_a_w: %x\n",data);
	// SJE - data commands to sound cpu need to be inverted.
	GTS3locals.sound_data = data^0xff;

	//Unless it's Strikes N Spares, send the sound command now!
    if (GTS3_dmdlocals[0].version != 2)
		sndbrd_0_data_w(0, GTS3locals.sound_data);
}

/* Data to A1P6 (extra LED Display or Flashers board)
   ------------
		DX0-DX3 = BCD Data / flasher lines
		DX4-DX7 = Column
		AX4     = Latch the Data (set by aux write handler)
*/
static WRITE_HANDLER( xvia_1_b_w ) {
	if (GTS3locals.extra16led) { // used for alpha display digits on Vegas only
		if (data > 0 && data < 4 && GTS3locals.ax[4] == GTS3locals.ax[6])
			GTS3locals.segments[39 + data].w = GTS3locals.pseg[39 + data].w =
			(GTS3locals.ax[6] & 0x3f) | ((GTS3locals.ax[6] & 0xc0) << 3)
			| ((GTS3locals.ax[5] & 0x0f) << 11) | ((GTS3locals.ax[5] & 0x10) << 2) | ((GTS3locals.ax[5] & 0x20) << 3);
	} else if (core_gameData->hw.lampCol > 4) { // flashers
		coreGlobals.tmpLampMatrix[12] = data;
	} else if (!(GTS3locals.ax[4] & 1)) { // LEDs
		if (GTS3locals.alphagen)
			GTS3locals.segments[40 + (data >> 4)].w = GTS3locals.pseg[40 + (data >> 4)].w = core_bcd2seg[data & 0x0f];
		else
			coreGlobals.segments[data >> 4].w = core_bcd2seg[data & 0x0f];
	}
}

//Should be not used!
static WRITE_HANDLER( xvia_1_ca1_w )
{
	logerror1("NOT USED!: via_1_ca1_w %x\n",data);
}
//CPU LED
static WRITE_HANDLER( xvia_1_ca2_w )
{
	GTS3locals.diagnosticLed = data;
}

static WRITE_HANDLER( xvia_1_cb1_w ) {
	logerror1("CX1: via_1_cb1_w %x\n",data);
	GTS3locals.cx1 = data;
}

static WRITE_HANDLER( xvia_1_cb2_w ) {
	logerror1("CX2: via_1_cb2_w %x\n",data);
	GTS3locals.cx2 = data;
}

//IRQ:  IRQ to Main CPU
static void GTS3_irq(int state) {
	// logerror("IN VIA_IRQ - STATE = %x\n",state);
	static int irq = 0;
	cpu_set_irq_line(GTS3_CPUNO, 0, irq?ASSERT_LINE:CLEAR_LINE);
	irq = !irq;
}

/*

DMD Generation Listed Below (for different hardware see code)

U4:
---
(I)PA0-7 Switch Rows/Returns
(I)PB3:  Test Switch
(I)PB4:  Tilt Switch
(I)PB5-  A1P3-3 - Display Controller - Status1
(I)PB6-  A1P3-1 - Display Controller - Not used
(I)PB7-  A1P3-2 - Display Controller - Status2
(I)CA1: Slam Switch
(I)CB1: N/A
(O)PB0:  Lamp Data    (LDATA)
(O)PB1:  Lamp Strobe  (LSTRB)
(O)PB2:  Lamp Clear   (LCLR)
(O)PB6:  Display Strobe (DSTRB)
(O)PB7:  Display Blanking (DBLNK) - Alpha Generation Only!
(O)CA2: To A1P6-12 & A1P7-6 Auxiliary
(O)CB2: NMI to Main CPU
IRQ:  IRQ to Main CPU

U5:
--
(O)PA0-7: SD0-7 - A1P4 (Sound data)
(O)PB0-7: DX0-7 - A1P6 (Auxilary) - DMD #2 Data on Strikes & Spares
(I)CA1:   Sound Return/Status
(O)CA2:   LED
(I)CB1:   CX1 - A1P6 (Auxilary)
(O)CB2:   CX2 - A1P6 (Auxilary)
IRQ:  IRQ to Main CPU
*/
static struct via6522_interface via_0_interface =
{
	/*inputs : A/B         */ xvia_0_a_r, xvia_0_b_r,
	/*inputs : CA1/B1,CA2/B2 */ 0, xvia_0_cb1_r, xvia_0_ca2_r, xvia_0_cb2_r,
	/*outputs: A/B,CA2/B2   */ xvia_0_a_w, xvia_0_b_w, xvia_0_ca2_w, xvia_0_cb2_w,
	/*irq                  */ 0 /* GTS3_irq */
};
static struct via6522_interface via_1_interface =
{
	/*inputs : A/B         */ xvia_1_a_r, xvia_1_b_r,
	/*inputs : CA1/B1,CA2/B2 */ xvia_1_ca1_r, xvia_1_cb1_r, xvia_1_ca2_r, xvia_1_cb2_r,
	/*outputs: A/B,CA2/B2   */ xvia_1_a_w, xvia_1_b_w, xvia_1_ca2_w, xvia_1_cb2_w,
	/*irq                  */ 0 /* GTS3_irq */
};

static INTERRUPT_GEN(GTS3_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  GTS3locals.vblankCount += 1;

  /*-- lamps --*/
  if ((GTS3locals.vblankCount % GTS3_LAMPSMOOTH) == 0) {
	memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = GTS3locals.solenoids;
  if ((GTS3locals.vblankCount % GTS3_SOLSMOOTH) == 0) {
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
    /*Update alpha or dmd display*/
    GTS3locals.UPDATE_DISPLAY();

	/*update leds*/

	//Strikes N Spares has 2 DMD LED, but no Sound Board LED
	if (GTS3_dmdlocals[0].version == 2) {
		coreGlobals.diagnosticLed = GTS3locals.diagnosticLed |
									(GTS3_dmdlocals[0].diagnosticLed << 1) |
									(GTS3_dmdlocals[1].diagnosticLed << 2);
	} else {
		coreGlobals.diagnosticLed = (GTS3locals.diagnosticLeds2<<3) |
									(GTS3locals.diagnosticLeds1<<2) |
									(GTS3_dmdlocals[0].diagnosticLed<<1) |
									GTS3locals.diagnosticLed;
	}
  }
  core_updateSw(GTS3locals.solenoids & 0x80000000);
}

static SWITCH_UPDATE(GTS3) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[GTS3_COMINPORT] & 0x0f00)>>8;
	if (inports[GTS3_COMINPORT] & 0x8000) // DMD games with tournament mode
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x80) | (inports[GTS3_COMINPORT] & 0x7f);
    else if (inports[GTS3_COMINPORT] & 0x4000) // DMD games without tournament mode
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xe0) | (inports[GTS3_COMINPORT] & 0x1f);
    else // alpha games
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xc0) | (inports[GTS3_COMINPORT] & 0x3f);
  }
  GTS3locals.swPrin = (core_getSw(GTS3_SWPRIN)>0?1:0);
  GTS3locals.swDiag = (core_getSw(GTS3_SWDIAG)>0?1:0);
  GTS3locals.swTilt = (core_getSw(GTS3_SWTILT)>0?1:0);
  GTS3locals.swSlam = (core_getSw(GTS3_SWSLAM)>0?1:0);

  //Force CA1 to read our input!
  /*Alpha Gen 1 returns TEST Switch here - ALL others Slam Switch*/
  if(GTS3locals.alphagen==1)
	via_set_input_ca1(0,GTS3locals.swDiag);
  else
	via_set_input_ca1(0,GTS3locals.swSlam);
}

static int gts3_sw2m(int no) {
  if (no % 10 > 7) return -1;
  return (no / 10 + 1) * 8 + (no % 10);
}

static int gts3_m2sw(int col, int row) {
  return (col - 1) * 10 + row;
}


/*Alpha Numeric First Generation Init*/
static void GTS3_alpha_common_init(void) {
  memset(&GTS3locals, 0, sizeof(GTS3locals));
  memset(&GTS3_dmdlocals[0], 0, sizeof(GTS3_DMDlocals));

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  GTS3locals.U4_PB_W  = alpha_u4_pb_w;
  GTS3locals.U4_PB_R  = alpha_u4_pb_r;
  GTS3locals.DISPLAY_CONTROL = alpha_display;
  GTS3locals.UPDATE_DISPLAY = alpha_update;
  GTS3locals.AUX_W = alpha_aux;

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);
}

/*Alpha Numeric First Generation Init*/
static MACHINE_INIT(gts3) {
	GTS3_alpha_common_init();
	GTS3locals.alphagen = 1;
}

/*Alpha Numeric Second Generation Init*/
static MACHINE_INIT(gts3b) {
	GTS3_alpha_common_init();
	GTS3locals.alphagen = 2;
}

/*DMD Generation Init*/
static void gts3dmd_init(void) {
  memset(&GTS3locals, 0, sizeof(GTS3locals));
  memset(&GTS3_dmdlocals[0], 0, sizeof(GTS3_DMDlocals));
  memset(&DMDFrames, 0, sizeof(DMDFrames));

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  //Init 6845
  crtc6845_init(0);

  /*DMD*/
  /*copy last 32K of ROM into last 32K of CPU region*/
  /*Setup ROM Swap so opcode will work*/
  if(memory_region(GTS3_MEMREG_DCPU1))
  {
    memcpy(memory_region(GTS3_MEMREG_DCPU1)+0x8000,
    memory_region(GTS3_MEMREG_DROM1) +
     (memory_region_length(GTS3_MEMREG_DROM1) - 0x8000), 0x8000);
  }

  GTS3locals.U4_PB_W  = dmd_u4_pb_w;
  GTS3locals.U4_PB_R  = dmd_u4_pb_r;
  GTS3locals.DISPLAY_CONTROL = dmd_display;
  GTS3locals.UPDATE_DISPLAY = dmd_update;
  GTS3locals.AUX_W = dmd_aux;

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);
}

/*DMD Generation Init*/
static MACHINE_INIT(gts3dmd) {
	gts3dmd_init();
}

static MACHINE_INIT(gts3dmda) {
	gts3dmd_init();
	GTS3_dmdlocals[0].version = 1;
}

/* Strikes n' Spares: this game uses TWO complete DMD boards! */
static MACHINE_INIT(gts3dmd2) {
  gts3dmd_init();
  memset(&GTS3_dmdlocals[1], 0, sizeof(GTS3_DMDlocals));
  memset(&DMDFrames2, 0, sizeof(DMDFrames2));
  GTS3_dmdlocals[0].version = 2;

  //Init 2nd 6845
  crtc6845_init(1);

  /*copy last 32K of DMD ROM into last 32K of CPU region*/
  if (memory_region(GTS3_MEMREG_DCPU2)) {
    memcpy(memory_region(GTS3_MEMREG_DCPU2) + 0x8000,
      memory_region(GTS3_MEMREG_DROM2) + (memory_region_length(GTS3_MEMREG_DROM2) - 0x8000), 0x8000);
  }
}

static MACHINE_STOP(gts3) {
  sndbrd_0_exit();
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	//logerror1("SS Write: Offset: %x Data: %x\n",offset,data);
	switch(offset){
		case 0:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
            break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
            break;
		case 3:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
			break;
		default:
			logerror1("Solenoid_W Logic Error\n");
	}
}

/*DMD Bankswitching - can handle two DMD displays*/
static void dmdswitchbank(int which)
{
	int	addr =	(GTS3_dmdlocals[which].pa0 *0x04000)+
				(GTS3_dmdlocals[which].pa1 *0x08000)+
				(GTS3_dmdlocals[which].pa2 *0x10000)+
 				(GTS3_dmdlocals[which].pa3 *0x20000)+
				(GTS3_dmdlocals[which].a18 *0x40000);
	cpu_setbank(which ? STATIC_BANK2 : STATIC_BANK1,
		memory_region(which ? GTS3_MEMREG_DROM2 : GTS3_MEMREG_DROM1) + addr);
}

/*Common DMD Data Latch Read routine*/
static READ_HANDLER(dmdlatch_read)
{
	int data = GTS3_dmdlocals[offset].dmd_latch;
//	if(offset)
//		logerror1("reading dmd #%x latch = %x\n",offset+1,data);
	return data;
}

/* Each specific DMD read latch routine */
static READ_HANDLER(dmdlatch_r) { return dmdlatch_read(0); }
static READ_HANDLER(dmdlatch2_r) { return dmdlatch_read(1); }

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER(display_control) { GTS3locals.DISPLAY_CONTROL(offset,data); }

/* ALPHA GENERATION
   ----------------
   Alpha Strobe:
		DS0 = enable a,b,c,d,e,f,g,h of Bottom Segment
		DS1 = enable i,j,k,l,m,n,dot,comma of Bottom Segment
		DS2 = enable a,b,c,d,e,f,g,h of Top Segment
		DS3 = enable i,j,k,l,m,n,dot,comma of Top Segment
*/
static WRITE_HANDLER(alpha_display){
	if ((GTS3locals.u4pb & ~DBLNK) && (GTS3locals.acol < 20)) {
		switch ( offset ) {
		case 0:
			GTS3locals.segments[20+GTS3locals.acol].b.lo |= GTS3locals.pseg[20+GTS3locals.acol].b.lo = data;
/*          // commented out segments dimming as it needs more work
			if (GTS3locals.pseg[20+GTS3locals.acol].w) {
				coreGlobals.segDim[20+GTS3locals.acol] /=2;
			} else {
				if (coreGlobals.segDim[20+GTS3locals.acol] < 15)
				  coreGlobals.segDim[20+GTS3locals.acol] +=3;
				else
				  GTS3locals.segments[20+GTS3locals.acol].w = 0;
				GTS3locals.pseg[20+GTS3locals.acol].w = GTS3locals.segments[20+GTS3locals.acol].w;
			}
*/
			break;

		case 1:
			GTS3locals.segments[20+GTS3locals.acol].b.hi |= GTS3locals.pseg[20+GTS3locals.acol].b.hi = data;
			break;

		case 2:
			GTS3locals.segments[GTS3locals.acol].b.lo |= GTS3locals.pseg[GTS3locals.acol].b.lo = data;
/*          // commented out segments dimming as it needs more work
			if (GTS3locals.pseg[GTS3locals.acol].w) {
				coreGlobals.segDim[GTS3locals.acol] /=2;
			} else {
				if (coreGlobals.segDim[GTS3locals.acol] < 15)
				  coreGlobals.segDim[GTS3locals.acol] +=3;
				else
				  GTS3locals.segments[GTS3locals.acol].w = 0;
				GTS3locals.pseg[GTS3locals.acol].w = GTS3locals.segments[GTS3locals.acol].w;
			}
*/
			break;

		case 3:
			GTS3locals.segments[GTS3locals.acol].b.hi |= GTS3locals.pseg[GTS3locals.acol].b.hi = data;
			break;
		}
	}
}

/* DMD GENERATION
   ----------------
   DMD Strobe & Reset:
		DS0 = Pulse the DMD CPU IRQ Line
		DS1 = Reset the DMD CPU
		DS2 = Not Used
		DS3 = Not Used
*/
static WRITE_HANDLER(dmd_display){
	//Latch DMD Data from U7
	GTS3_dmdlocals[0].dmd_latch = data;
	if (offset == 0) {
		cpu_set_irq_line(GTS3_DCPUNO, 0, HOLD_LINE);
	} else if (offset == 1) {
		cpu_set_irq_line(GTS3_DCPUNO, 0, CLEAR_LINE);
		cpu_set_reset_line(GTS3_DCPUNO, PULSE_LINE);
	} else
		logerror("DMD Signal: Offset: %x Data: %x\n",offset,data);
}

/*Chip U14 - LS273:
  D0=Q0=PA0=A14 of DMD Eprom
  D1=Q1=PA1=A15 of DMD Eprom
  D2=Q2=PA2=A16 of DMD Eprom (Incorrectly identified as D5,Q5 on the schematic!)
  D3=Q3=PA3=A17 of DMD Eprom (Incorrectly identified as D4,Q4 on the schematic!)
  D4=Q4=Fed to GAL16V8(A18?) (Incorrectly identified as D3,Q3 on the schematic!)
  D5=Q5=PB5(U4)=DMD Status 1 (Incorrectly identified as D2,Q2 on the schematic!)
  D6=Q6=PB7(U4)=DMD Status 2
  D7=DMD LED
*/

//Common DMD Output Port Handler
static WRITE_HANDLER(dmd_outport)
{
	GTS3_dmdlocals[offset].pa0=(data>>0)&1;
	GTS3_dmdlocals[offset].pa1=(data>>1)&1;
	GTS3_dmdlocals[offset].pa2=(data>>2)&1;
	GTS3_dmdlocals[offset].pa3=(data>>3)&1;
	GTS3_dmdlocals[offset].q3 =(data>>4)&1;
	GTS3_dmdlocals[offset].a18=GTS3_dmdlocals[offset].q3;
	GTS3_dmdlocals[offset].status1=(data>>5)&1;
	GTS3_dmdlocals[offset].status2=(data>>6)&1;
	GTS3_dmdlocals[offset].diagnosticLed = data>>7;
	dmdswitchbank(offset);
}

//DMD Output port handling for up to 2 DMD Displays
static WRITE_HANDLER(dmdoport) { dmd_outport(0,data); }
static WRITE_HANDLER(dmdoport2) { dmd_outport(1,data); }

//This should never be called!
static READ_HANDLER(display_r){ /* logerror("DISPLAY_R\n"); */ return 0;}

//Writes Lamp Returns
static WRITE_HANDLER(lds_w)
{
	//logerror1("LDS Write: Data: %x\n",data);
    GTS3locals.lampRow = data;
}

//PB0-7 Varies on Alpha or DMD Generation!
static WRITE_HANDLER(aux_w) {
	//logerror1("aux_w: %x %x\n",offset,data);
	GTS3locals.AUX_W(offset,data);
}
/* ALPHA GENERATION
   ----------------
   LED Board Digit Strobe
*/
static WRITE_HANDLER(alpha_aux) {
	GTS3locals.ax[4+offset] = data;
	if (!GTS3locals.extra16led && offset) GTS3locals.extra16led = 1;
}
/* DMD GENERATION
   ----------------
   Auxilary Data
   AX4 = (offset = 0)
   AX5 = (offset = 1)
   AX6 = (offset = 2)
*/
static WRITE_HANDLER(dmd_aux) {
	logerror1("dmd_aux: offset=%02x, data=%02x\n", offset, data);

	//Store it since the data is referred elsewhere
	GTS3locals.ax[4+offset] = data;

	//Generate an dmd vblank?
	if (!offset) {
		dmd_vblank(0);
		//2nd DMD for Strikes N Spares
		if(GTS3_dmdlocals[0].version == 2)
			dmd_vblank(1);
	}

	//Strikes N Spares Stuff
	if (GTS3_dmdlocals[0].version == 2)
	{
		//AX4 Line - Clocks in a new DMD command for Display #2
		if (offset == 0) {
			//logerror1("Sending DMD #2 Command = %x\n",data);
			GTS3_dmdlocals[1].dmd_latch = data;
			cpu_set_irq_line(GTS3_DCPUNO2, 0, HOLD_LINE);
		}
		// AX5 Line - Triggers data to OKI chip
		if (offset == 1) {
			OKIM6295_set_bank_base(0, (GTS3locals.ax[6] & 0x40) << 12);
			OKIM6295_data_0_w(0, data);
		}
	}
}

//Update the DMD Frames during a simulated DMD VBLANK - Supports 2 DMD Displays
static void dmd_vblank(int which) {
	int offset = crtc6845_start_address_r(which) >> 2;
	if (which)
		memcpy(DMDFrames2[GTS3_dmdlocals[1].nextDMDFrame],memory_region(GTS3_MEMREG_DCPU2)+0x1000+offset,0x200);
	else
		memcpy(DMDFrames[GTS3_dmdlocals[0].nextDMDFrame],memory_region(GTS3_MEMREG_DCPU1)+0x1000+offset,0x200);
	cpu_set_nmi_line(which ? GTS3_DCPUNO2 : GTS3_DCPUNO, PULSE_LINE);
	GTS3_dmdlocals[which].nextDMDFrame = (GTS3_dmdlocals[which].nextDMDFrame + 1) % GTS3DMD_FRAMES;
}

/* Printer connector */
static WRITE_HANDLER(aux1_w)
{
    static void *printfile;
	static UINT8 printdata[] = {0};
	if (printfile == NULL) {
		char filename[13];
		sprintf(filename,"%s.prt", Machine->gamedrv->name);
		printfile = mame_fopen(Machine->gamedrv->name,filename,FILETYPE_PRINTER,2); // APPEND write mode
	}
	GTS3locals.ax[1+(offset>>4)] = data;
	if (offset < 8) {
		GTS3locals.prn[offset] = data;
		if (!offset) {
			printdata[0] = data;
			mame_fwrite(printfile, printdata, 1);
		}
	}
	logerror1("Aux1 Write: Offset: %x Data: %x\n",offset,data);
}

/* Communication adapter (printer) status */
static READ_HANDLER(aux1_r) {
	if (GTS3locals.swPrin) {
		GTS3locals.prn[1]=0xbf;
	} else {
		GTS3locals.prn[1]=0x40;
		GTS3locals.prn[3]=0;
	}
	logerror("Aux1 Read: %04x:%x:%02x\n", activecpu_get_previouspc(), offset, GTS3locals.prn[offset]);
	return GTS3locals.prn[offset];
}

static INTERRUPT_GEN(alphanmi) {
	xvia_0_cb2_w(0,0);
}

static void alpha_update(){
    /* FORCE The 16 Segment Layout to match the output order expected by core.c */
	// There's got to be a better way than this junky code!
	UINT16 segbits, tempbits;
	int i,j,k;
	for (i=0; i<2; i++) {
		for (j=0; j<20; j++){
			segbits = GTS3locals.segments[20*i+j].w;
			tempbits = 0;
			if (segbits > 0) {
				for (k=0; k<16; k++) {
					if ( (segbits>>k)&1 )
						tempbits |= (1<<alpha_adjust[k]);
				}
			}
			GTS3locals.segments[20*i+j].w = tempbits;
		}
	}
	memcpy(coreGlobals.segments, GTS3locals.segments, sizeof(coreGlobals.segments));
    memcpy(GTS3locals.segments, GTS3locals.pseg, sizeof(GTS3locals.segments));
}
static void dmd_update() {}

//Show Sound Diagnostic LEDS
void UpdateSoundLEDS(int num,int data)
{
	if(num==0)
		GTS3locals.diagnosticLeds1 = data;
	else
		GTS3locals.diagnosticLeds2 = data;
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(gts3) {
  core_nvram(file, read_or_write, memory_region(GTS3_MEMREG_CPU), 0x2000, 0x00);
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_readmem)
{0x0000,0x1fff,MRA_RAM},
{0x2000,0x200f,via_0_r},
{0x2010,0x201f,via_1_r},
{0x2020,0x2023,display_r},
{0x2050,0x2057,aux1_r},
{0x4000,0xffff,MRA_ROM},
MEMORY_END


static MEMORY_WRITE_START(GTS3_writemem)
{0x0000,0x1fff,MWA_RAM},
{0x2000,0x200f,via_0_w},
{0x2010,0x201f,via_1_w},
{0x2020,0x2023,display_control},
{0x2030,0x2033,solenoid_w},
{0x2040,0x2040,lds_w},
{0x2041,0x2043,aux_w},
{0x2050,0x2070,aux1_w},
{0x4000,0xffff,MWA_ROM},
MEMORY_END


/*---------------------------
/  Memory map for DMD CPU
/----------------------------*/
static MEMORY_READ_START(GTS3_dmdreadmem)
{0x0000,0x1fff, MRA_RAM},
{0x2000,0x2000, crtc6845_register_0_r},
{0x3000,0x3000, dmdlatch_r}, /*Input Enable*/
{0x4000,0x7fff, MRA_BANK1},
{0x8000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem)
{0x0000,0x0fff, MWA_RAM},
{0x1000,0x1fff, MWA_RAM},    /*DMD Display RAM*/
{0x2800,0x2800, crtc6845_address_0_w},
{0x2801,0x2801, crtc6845_register_0_w},
{0x3800,0x3800, dmdoport},   /*Output Enable*/
{0x4000,0x7fff, MWA_BANK1},
{0x8000,0xffff, MWA_ROM},
MEMORY_END

//NOTE: DMD #2 for Strikes N Spares - Identical to DMD #1 hardware & memory map
static MEMORY_READ_START(GTS3_dmdreadmem2)
{0x0000,0x1fff, MRA_RAM},
{0x2000,0x2000, crtc6845_register_1_r},
{0x3000,0x3000, dmdlatch2_r}, /*Input Enable*/
{0x4000,0x7fff, MRA_BANK2},
{0x8000,0xffff, MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem2)
{0x0000,0x0fff, MWA_RAM},
{0x1000,0x1fff, MWA_RAM},    /*DMD Display RAM*/
{0x2800,0x2800, crtc6845_address_1_w},
{0x2801,0x2801, crtc6845_register_1_w},
{0x3800,0x3800, dmdoport2},   /*Output Enable*/
{0x4000,0x7fff, MWA_BANK2},
{0x8000,0xffff, MWA_ROM},
MEMORY_END

MACHINE_DRIVER_START(gts3)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M65C02, 2000000)
  MDRV_CPU_MEMORY(GTS3_readmem, GTS3_writemem)
  MDRV_CPU_VBLANK_INT(GTS3_vblank, GTS3_VBLANKFREQ)
  MDRV_CPU_PERIODIC_INT(alphanmi, GTS3_ALPHANMIFREQ)
  MDRV_TIMER_ADD(GTS3_irq, GTS3_IRQFREQ)
  MDRV_NVRAM_HANDLER(gts3)

  MDRV_SWITCH_UPDATE(GTS3)
  MDRV_DIAGNOSTIC_LEDH(4)
  MDRV_SWITCH_CONV(gts3_sw2m,gts3_m2sw)
  MDRV_LAMP_CONV(gts3_sw2m,gts3_m2sw)
  MDRV_SOUND_CMD(GTS3_sndCmd_w)
  MDRV_SOUND_CMDHEADING("GTS3")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1a)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CORE_INIT_RESET_STOP(gts3,NULL,gts3)
  MDRV_SCREEN_SIZE(320, 200)
  MDRV_VISIBLE_AREA(0, 319, 0, 199)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1as_no)
  MDRV_IMPORT_FROM(gts3_1a)
  MDRV_IMPORT_FROM(gts80s_s3_no)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1as)
  MDRV_IMPORT_FROM(gts3_1a)
  MDRV_IMPORT_FROM(gts80s_s3)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1as80b2)
  MDRV_IMPORT_FROM(gts3_1a)
  MDRV_IMPORT_FROM(gts80s_b2)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1as80b3)
  MDRV_IMPORT_FROM(gts3_1a)
  MDRV_IMPORT_FROM(gts80s_b3)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1b)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CORE_INIT_RESET_STOP(gts3b,NULL,gts3)
  MDRV_SCREEN_SIZE(320, 200)
  MDRV_VISIBLE_AREA(0, 319, 0, 199)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_1bs)
  MDRV_IMPORT_FROM(gts3_1b)
  MDRV_IMPORT_FROM(gts80s_s3)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_2)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CPU_ADD(M65C02, 3579000/2)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem, GTS3_dmdwritemem)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd,NULL,gts3)
  MDRV_IMPORT_FROM(gts80s_s3)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gts3_2a)
  MDRV_IMPORT_FROM(gts3_2)
  MDRV_CORE_INIT_RESET_STOP(gts3dmda,NULL,gts3)
MACHINE_DRIVER_END


//Sound Interface for Strikes N Spares
static struct OKIM6295interface sns_okim6295_interface = {
	1,					/* 1 chip */
	{ 15151.51 },		/* sampling frequency at 2MHz chip clock */
	{ REGION_USER3 },	/* memory region */
	{ 75 }				/* volume */
};

// 2nd DMD CPU for Strikes n' Spares
MACHINE_DRIVER_START(gts3_22)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CPU_ADD(M65C02, 3579000/2)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem, GTS3_dmdwritemem)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd,NULL,gts3)
  MDRV_CPU_ADD(M65C02, 3579000/2)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem2, GTS3_dmdwritemem2)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd2,NULL,gts3)
  MDRV_SOUND_ADD(OKIM6295, sns_okim6295_interface)
  MDRV_DIAGNOSTIC_LEDH(3)
MACHINE_DRIVER_END
