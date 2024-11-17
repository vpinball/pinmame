/************************************************************************************************
  Gottlieb Pinball - System 3

  Hardware from 1989-1996

  Earlier games used 2 - 20 column 16 segment alphanumeric displays.
  Later games used a 128x32 DMD.

  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET
  //Cueball: CPU0: RST = FEF0, IRQ=462F, NMI=477A

  GV 07/20/05: Finally found the typo that prevented the extra displays from working perfectly!
  GV 07/27/05: Fixed the missing sound in Strikes N Spares
  SE 07/27/05: Finally got that DAMN 2nd DMD working

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
//#include "cpu/m6502/m65ce02.h"
#include "machine/6522via.h"
#include "core.h"
#include "sndbrd.h"
#include "gts3.h"
#include "vidhrdw/crtc6845.h"
#include "gts3dmd.h"
#include "gts80s.h"

#define GTS3_INTERFACE_UPD_PER_FRAME    1 /* interface update frequency per 60Hz frame */
#define GTS3_IRQFREQ                 1500 /* IRQ Frequency (Guessed) */
#define GTS3_ALPHANMIFREQ            1000 /* Alpha NMI Frequency (Guessed)*/

#define GTS3_CPUNO   0
#define GTS3_DCPUNO  1
#define GTS3_SCPUNO  2
#define GTS3_DCPUNO2 2

#if 1
#define logerror1 logerror
#else
#define logerror1 printf
#endif

static WRITE_HANDLER(display_control);

/*Alpha Display Generation Specific*/
static READ_HANDLER(alpha_u4_pb_r);
static WRITE_HANDLER(alpha_display);
static WRITE_HANDLER(alpha_aux);
/*DMD Generation Specific*/
static void dmdswitchbank(int which);
static READ_HANDLER(dmd_u4_pb_r);
static WRITE_HANDLER(dmd_display);
static WRITE_HANDLER(dmd_aux);
static void dmd_vblank(int which);

/*----------------
/ Local variables
/-----------------*/
tGTS3locals GTS3locals;

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
		data |= (GTS3locals.swSlam << 3); //Slam Switch (NOT INVERTED!)
	else
		data |= (GTS3locals.swDiag << 3); //Diag Switch (NOT INVERTED!)
	data |= (GTS3locals.swTilt << 4);     //Tilt Switch (NOT INVERTED!)
	return data;
}

/* DMD GENERATION
   ----------------
   PB0-2: Output only
   PB3:  Test Switch
   PB4:  Tilt Switch
   PB5-  A1P3-3 - Display Controller - Status1 (Not labeled on Schematic, toggled by the DMD CPU when data is received to tell it is ready to get another byte)
   PB6-  A1P3-1 - Display Controller - DMD Display Strobe (DSTB) - Output only, but might be read!
   PB7-  A1P3-2 - Display Controller - Status2 (Not labeled on Schematic)
*/
static READ_HANDLER(dmd_u4_pb_r)
{
	int data = 0;
	data |= (GTS3locals.swDiag << 3);   //Diag Switch (NOT INVERTED!)
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

/* U4 VIA Parralel Out B
   ---------------------
  PB0:  Lamp Data      (LDATA)
  PB1:  Lamp Strobe    (LSTRB)
  PB2:  Lamp Clear     (LCLR)
  PB5:  Display Data   (DDATA) Alpha generation only, unused for DMD generation
  PB6:  Display Strobe (DSTRB)
  PB7:  Display Blank  (DBLNK) Alpha generation only, unused for DMD generation
*/
#define LDATA 0x01
#define LSTRB 0x02
#define LCLR  0x04
#define DDATA 0x20
#define DSTRB 0x40
#define DBLNK 0x80

static WRITE_HANDLER( xvia_0_b_w ) {
	// From schematics:
	// - Rows are a simple latch (74HC273) with overcurrent comparators
	// - Columns use two 74HS164 (8 bit shift register), when LSTRB is raised (low to high edge), column output are shifted (<<) and LDATA is used for lowest bit (forming a 12 bit shift sequence)
	// - LCLR creates a short pulse (through a 555, with R=4.7k and C=0.1uF so 50us pulse) that will reset the overcurrent comparators on the row outputs (likely to cover the latching between row & column and avoid ghosting)
	// This results in the following write sequence for strobing (observed on Cue Ball Wizard):
	// - Set lampRow to 0 (turn off all lamps)
	// - Set LDATA (only if needed, to start strobe on first column) / LSTRB H->L / LSTRB L->H / Clear LDATA
	// - Set lampRow to expected output (turn on expected lamps of the new column)
	// - LCLR H->L / LCLR L->H
	// We do not simulate the LCLR signals since the pulse is too short (50us) for the output resolution
	//printf("t=%8.5f Col=%3x STRB=%d DATA=%d LCLR=%d\n", timer_get_time(), GTS3locals.lampColumn, data & LSTRB, data & LDATA, data & LCLR);
	if (~GTS3locals.u4pb & data & LSTRB) { // Positive edge on LSTRB: shift 12bit register and set bit0 to LDATA
		GTS3locals.lampColumn = ((GTS3locals.lampColumn << 1) & 0x0ffe) | (data & LDATA);
		if (GTS3locals.lampColumn == 0x001) { // Simple strobe emulation: accumulate lamp matrix until strobe restarts from first column
			memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
			memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
		}
	}
	const UINT8 lampRow = GTS3locals.lampRow; // data & LCLR ? 0 : GTS3locals.lampRow; // Not emulated as it does not provide any benefit
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0     ,  GTS3locals.lampColumn       & 0xFF, lampRow, 8);
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (GTS3locals.lampColumn >> 8) & 0x0F, lampRow, 4);


	if (GTS3locals.alphagen) { // Alpha generation
		if (~GTS3locals.u4pb & data & DSTRB) { // Positive edge on DSTRB: shift 20bit register and set bit0 to DDATA
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16, 0);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16 + 8, 0);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16    , 0);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16 + 8, 0);
			GTS3locals.alphaNumColShiftRegister = ((GTS3locals.alphaNumColShiftRegister << 1) & 0x0ffffe) | ((data & DDATA) >> 5);
			GTS3locals.alphaNumCol = GTS3locals.alphaNumColShiftRegister == 0 ? 20 : core_BitColToNum(GTS3locals.alphaNumColShiftRegister);
			// This should never happens but you can drive the hardware to it (multiple resets,...), this will lead to incorrect rendering
			// assert((GTS3locals.alphaNumColShiftRegister == 0) || (GTS3locals.alphaNumColShiftRegister == (1 << GTS3locals.alphaNumCol)));
		}
		if (~GTS3locals.u4pb & data & DBLNK) { // DBlank start (positive edge)
			for (int i = 0; i < 20 * 2 * 2; i++)
				core_write_pwm_output_8b(CORE_MODOUT_SEG0 + i * 8, 0);
		}
		else if (GTS3locals.u4pb & ~data & DBLNK && GTS3locals.alphaNumCol < 20) { // DBlank end (negative edge)
			// Non dimmed segments emulation: use the column and value defined during DBLNK
			coreGlobals.segments[GTS3locals.alphaNumCol + 20].w = GTS3locals.activeSegments[0].w;
			coreGlobals.segments[GTS3locals.alphaNumCol     ].w = GTS3locals.activeSegments[1].w;
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16    , GTS3locals.activeSegments[0].b.lo);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16 + 8, GTS3locals.activeSegments[0].b.hi);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16    , GTS3locals.activeSegments[1].b.lo);
			core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16 + 8, GTS3locals.activeSegments[1].b.hi);
		}
	}
	else { // DMD generation
		GTS3_dmdlocals[0].dstrb = (data & DSTRB) != 0;
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
	if(GTS3_dmdlocals[0].has2DMD)
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
	if (GTS3_dmdlocals[0].has2DMD == 0)
		sndbrd_0_data_w(0, GTS3locals.sound_data);
}

/* Data to A1P6 (extra LED Display or Flashers board)
   ------------
		DX0-DX3 = BCD Data / flasher lines
		DX4-DX7 = Column
		AX4     = Latch the Data (set by aux write handler)
*/
static WRITE_HANDLER( xvia_1_b_w ) {
	// FIXME this looks somewhat wrong to me: I think the 6522 VIA is supposed to latch the data on its programmable inpout/output parallel port,
	// then the CPU trigger AX4/5/6 for the aux board to handle it. Here we are doing the opposite
	if (GTS3locals.extra16led) { // used for the 3 playfield alpha digit displays on Vegas only
		// TODO implement full aux board logic
		if (data > 0 && data < 4 && GTS3locals.ax[4] == GTS3locals.ax[6])
			coreGlobals.segments[39 + data].w = (GTS3locals.ax[6] & 0x3f) 
			                                 | ((GTS3locals.ax[6] & 0xc0) <<  3) 
			                                 | ((GTS3locals.ax[5] & 0x0f) << 11) 
			                                 | ((GTS3locals.ax[5] & 0x10) <<  2) 
			                                 | ((GTS3locals.ax[5] & 0x20) <<  3);
	} else if (core_gameData->hw.lampCol > 4) { // flashers drived by auxiliary board (for example backbox lights in SF2)
		// FIXME From the schematics, I would say that the latch only happens if ax[4] is raised up (not checked here)
		coreGlobals.lampMatrix[12] = coreGlobals.tmpLampMatrix[12] = data;
		core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 12 * 8, data);
	} else if (!(GTS3locals.ax[4] & 1)) { // LEDs
		if (GTS3locals.alphagen)
			coreGlobals.segments[40 + (data >> 4)].w = core_bcd2seg[data & 0x0f];
		else
			coreGlobals.segments[data >> 4].w = core_bcd2seg[data & 0x0f];
	}
}

//Should be not used!
#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER( xvia_1_ca1_w )
{
	logerror1("NOT USED!: via_1_ca1_w %x\n",data);
}
#endif

//CPU LED
static WRITE_HANDLER( xvia_1_ca2_w )
{
	GTS3locals.diagnosticLed = data;
}

#ifndef PINMAME_NO_UNUSED	// currently unused function (GCC 3.4)
static WRITE_HANDLER( xvia_1_cb1_w ) {
	logerror1("CX1: via_1_cb1_w %x\n",data);
	GTS3locals.cx1 = data;
}
#endif

static WRITE_HANDLER( xvia_1_cb2_w ) {
	logerror1("CX2: via_1_cb2_w %x\n",data);
	GTS3locals.cx2 = data;
}

//IRQ:  IRQ to Main CPU
static void GTS3_irq(int state) {
	// logerror("IN VIA_IRQ - STATE = %x\n",state);
	cpu_set_irq_line(GTS3_CPUNO, 0, GTS3locals.irq?ASSERT_LINE:CLEAR_LINE);
	GTS3locals.irq = !GTS3locals.irq;
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
(O)PB5:  Display Data   (DDATA) - Alpha Generation Only!
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

static INTERRUPT_GEN(GTS3_interface_update) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  GTS3locals.interfaceUpdateCount++;

  /*-- solenoids --*/
  coreGlobals.solenoids = GTS3locals.solenoids;
  if ((GTS3locals.interfaceUpdateCount % GTS3_SOLSMOOTH) == 0) {
	// FIXME ssEn is never set. Special solenoids are handled by core_updateSw triggered from GameOn directly read from solenoid #31 state
	// Note that the code here would have a bug because it sets solenoids #23 as GameOn but this solenoid is used for other purposes by GTS3 hardware
//	if (GTS3locals.ssEn) { 
//	  int ii;
//	  coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
//	  /*-- special solenoids updated based on switches --*/
//	  for (ii = 0; ii < 6; ii++)
//		if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
//		  coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
//	}
	GTS3locals.solenoids = coreGlobals.pulsedSolState;
  }

  /*-- diagnostic leds --*/
  if ((GTS3locals.interfaceUpdateCount % GTS3_LEDSMOOTH) == 0) { // TODO it seems that diag LEDs are PWMed => move to a lamp
	if (GTS3_dmdlocals[0].has2DMD) { //Strikes N Spares has 2 DMD LED, but no Sound Board LED
		coreGlobals.diagnosticLed = GTS3locals.diagnosticLed |
									(GTS3_dmdlocals[0].diagnosticLed << 1) |
									(GTS3_dmdlocals[1].diagnosticLed << 2);
	} else {
		coreGlobals.diagnosticLed = GTS3locals.diagnosticLed |
			                        (GTS3_dmdlocals[0].diagnosticLed << 1) |
			                        (GTS3locals.diagnosticLed1 << 2) |
			                        (GTS3locals.diagnosticLed2 << 3);
	}
  }

  core_updateSw(GTS3locals.solenoids & 0x80000000); // GameOn is solenoid #31 for all tables
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

  GTS3locals.U4_PB_R  = alpha_u4_pb_r;
  GTS3locals.DISPLAY_CONTROL = alpha_display;
  GTS3locals.AUX_W = alpha_aux;

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, 1, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);

  /* Initialize outputs */
  coreGlobals.nGI = 0; // in fact there are 2 GI relays (playfield / backbox) controlled by low power solenoid outputs
  coreGlobals.nAlphaSegs = 20 * 16 * 2;
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_BULB_44_20V_DC_GTS3);
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25, 1, CORE_MODOUT_BULB_44_6_3V_AC); // 'A' relay: lightbox insert (backbox), note that schematics read 6V AC, not 6.3
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30, 1, CORE_MODOUT_BULB_44_6_3V_AC); // 'T' relay: GI, note that schematics read 6V AC, not 6.3
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31, 1, CORE_MODOUT_PULSE);           // 'Q' relay: GameOn
  // VFD powered through 8.6V AC for the filaments, and 47V DC for grids and anodes, 0.5ms pulse every 20ms (dimmable)
  core_set_pwm_output_type(CORE_MODOUT_SEG0, coreGlobals.nAlphaSegs, CORE_MODOUT_VFD_STROBE_05_20MS);
  // Game specific hardware
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
	  rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
  if (strncasecmp(gn, "bellring", 8) == 0) { // Bell Ringer
	  // TODO No manual found
  }
  else if (strncasecmp(gn, "cactjack", 8) == 0) { // Cactus Jack
	  // Made from an incomplete manual without schematics. Needs to be checked more thoroughly
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 12, 12, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
  }
  else if (strncasecmp(gn, "carhop", 6) == 0) { // Car Hop
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14, 11, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7, 1, CORE_MODOUT_BULB_44_6_3V_AC); // 'B' relay: Right GI, note that schematics read 6V AC, not 6.3
  }
  else if (strncasecmp(gn, "clas1812", 8) == 0) { // Class of 1812
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 12, 13, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 1 * 8 + 0, 5, CORE_MODOUT_LED_STROBE_1_10MS); // Battometer
  }
  else if (strncasecmp(gn, "ccruise", 7) == 0) { // Caribbean Cruise
	  // TODO No manual found
  }
  else if (strncasecmp(gn, "deadweap", 8) == 0) { // Deadly Weapon
	  coreGlobals.nAlphaSegs = 20 * 16 * 2 + 8 * 16;
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 10, 15, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  for (int i = 0; i < 8; i++) // TODO check strobe timings for LED power
		core_set_pwm_output_type(CORE_MODOUT_SEG0 + 20 * 16 * 2 + i * 16, 7, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
  else if (strncasecmp(gn, "hoops", 5) == 0) { // Hoops
	  coreGlobals.nAlphaSegs = 20 * 16 * 2 + 12 * 16;
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 8, 6, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16, 6, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  for (int i = 0; i < 12; i++) // TODO check strobe timings for LED power
		core_set_pwm_output_type(CORE_MODOUT_SEG0 + 20 * 16 * 2 + i * 16, 7, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
  else if (strncasecmp(gn, "lca", 3) == 0) { // Light Camera Action
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17, 6, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
  }
  else if (strncasecmp(gn, "nudgeit", 7) == 0) { // Nudge It
	  // TODO No manual found
  }
  else if (strncasecmp(gn, "opthund", 7) == 0) { // Operation Thunder
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13, 10, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Aux board flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // SPECIAL
  }
  else if (strncasecmp(gn, "silvslug", 8) == 0) { // Silver Slugger
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 8, 15, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  for (int i = 0; i < 12; i++) // TODO check strobe timings for LED power
		core_set_pwm_output_type(CORE_MODOUT_SEG0 + 20 * 16 * 2 + i * 16, 7, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
  else if (strncasecmp(gn, "surfnsaf", 8) == 0) { // Surf'n Safari
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 10, 15, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 4 * 8 + 4, 4, CORE_MODOUT_LED_STROBE_1_10MS); // Left Billboard
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 5 * 8 + 2, 4, CORE_MODOUT_LED_STROBE_1_10MS); // Right Billboard
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 6 * 8 + 7, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Monster Nostrils
  }
  else if (strncasecmp(gn, "tfight", 6) == 0) { // Title Fight
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 6, 10, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18, 6, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
  }
  else if (strncasecmp(gn, "vegas", 5) == 0) { // Vegas
	  coreGlobals.nAlphaSegs = 20 * 16 * 2 + 3 * 16;
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7, 18, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SEG0 + 20 * 16 * 2, 3 * 16, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
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

  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  //Init 6845
  crtc6845_init(0);
  crtc6845_set_vsync(0, 3579545. / 2., dmd_vblank);

  // Setup PWM shading, with a backward compatible combiner
  if ((strncasecmp(Machine->gamedrv->name, "smb", 3) == 0) || (strncasecmp(Machine->gamedrv->name, "cueball", 7) == 0))
	  core_dmd_pwm_init(&GTS3_dmdlocals[0].pwm_state, 128, 32, CORE_DMD_PWM_FILTER_GTS3, CORE_DMD_PWM_COMBINER_GTS3_5C); // Used to be '_5C' games: SMB, SMB Mushroom, Cueball
  else if ((strncasecmp(Machine->gamedrv->name, "stargat", 7) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "bighurt", 7) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "waterwl", 7) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "andrett", 7) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "barbwire", 8) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "brooks", 6) == 0)
	  || (strncasecmp(Machine->gamedrv->name, "snspare", 7) == 0))
	  core_dmd_pwm_init(&GTS3_dmdlocals[0].pwm_state, 128, 32, CORE_DMD_PWM_FILTER_GTS3, CORE_DMD_PWM_COMBINER_GTS3_4C_B); // Used to be '_4C_b' games
  else
	  core_dmd_pwm_init(&GTS3_dmdlocals[0].pwm_state, 128, 32, CORE_DMD_PWM_FILTER_GTS3, CORE_DMD_PWM_COMBINER_GTS3_4C_A); // Used to be '_4C_a' games

  /*DMD*/
  /*copy last 32K of ROM into last 32K of CPU region*/
  /*Setup ROM Swap so opcode will work*/
  if(memory_region(GTS3_MEMREG_DCPU1))
  {
    memcpy(memory_region(GTS3_MEMREG_DCPU1)+0x8000,
    memory_region(GTS3_MEMREG_DROM1) +
     (memory_region_length(GTS3_MEMREG_DROM1) - 0x8000), 0x8000);
  }

  GTS3locals.U4_PB_R  = dmd_u4_pb_r;
  GTS3locals.DISPLAY_CONTROL = dmd_display;
  GTS3locals.AUX_W = dmd_aux;

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, 2, memory_region(GTS3_MEMREG_SCPU1), NULL, NULL);

  // Initialize outputs
  coreGlobals.nGI = 0; // in fact there are 2 GI relays (playfield / backbox) controlled by low power solenoid outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_BULB_44_20V_DC_GTS3);
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25, 15, CORE_MODOUT_BULB_44_6_3V_AC); // 'A' relay: lightbox insert (backbox), note that schematics read 6V AC, not 6.3
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30, 15, CORE_MODOUT_BULB_44_6_3V_AC); // 'T' relay: GI, note that schematics read 6V AC, not 6.3
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31, 15, CORE_MODOUT_PULSE);           // 'Q' relay: GameOn
  // Game specific hardware
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
	  rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
  // Missing definitions:
  // - Brooks & Dunn
  // - Nudge It
  // - Bell Ringer
  if (strncasecmp(gn, "andretti", 8) == 0) { // Mario Andretti
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19, 6, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  coreGlobals.nAlphaSegs = 6 * 16;
	  for (int i = 0; i < 6; i++) // TODO check strobe timings for LED power
		core_set_pwm_output_type(CORE_MODOUT_SEG0 + i * 16, 7, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
  else if (strncasecmp(gn, "barbwire", 8) == 0) { // Barbwire
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13, 2, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16, 9, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
  }
  else if (strncasecmp(gn, "bighurt", 7) == 0) { // Big Hurt
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17, 5, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Aux board flashers
  }
  else if (strncasecmp(gn, "brooks", 6) == 0) { // Brooks & Dunn
	  // TODO No manual found
  }
  else if (strncasecmp(gn, "cueball", 7) == 0) { // Cueball Wizard
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'DOUBLE' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 1 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'WIZARD' LEDs
  }
  else if (strncasecmp(gn, "freddy", 6) == 0) { // Freddy: A Nightmare on Elm Street
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16, 7, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'AWAKE' LEDs
  }
  else if (strncasecmp(gn, "gladiatr", 8) == 0) { // Gladiator
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13, 9, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23, 1, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7, 2, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield & Backbox flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'WEAPON' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 1 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'DOUBLE' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 9 * 8 + 4, 4, CORE_MODOUT_LED_STROBE_1_10MS); // Left Billboard LEDs
  }
  else if (strncasecmp(gn, "rescu911", 8) == 0) { // Rescue 911
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13, 7, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Backbox flasher (from aux board)
  }
  else if (strncasecmp(gn, "shaqattq", 8) == 0) { // Shaq Attaq
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 12, 10, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  coreGlobals.nAlphaSegs = 12 * 16;
	  for (int i = 0; i < 12; i++) // TODO check strobe timings for LED power
		core_set_pwm_output_type(CORE_MODOUT_SEG0 + i * 16, 7, CORE_MODOUT_VFD_STROBE_05_20MS); // Additional VFD display
  }
  else if (strncasecmp(gn, "smbmush", 7) == 0) { // Super Mario Bros. Mushroom World
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20, 3, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Flashers from aux board
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 Wand LEDs
  }
  else if (strncasecmp(gn, "smb", 3) == 0) { // Super Mario Bros.
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 12, 12, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 Castle LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 Billboard LEDs
  }
  else if (strncasecmp(gn, "stargate", 8) == 0) { // Stargate
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21, 1, CORE_MODOUT_LED); // 'Rope Lights', circle of leds around Ra in backbox
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Flashers from aux board
  }
  else if (strncasecmp(gn, "sfight2", 7) == 0) { // Street Fighter 2
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23, 15, CORE_MODOUT_PULSE);           // 'S' relay: Lower playfield GameOn
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14, 9, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 6 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 'FIGHTER' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Backbox flasher (from aux board)
  }
  else if (strncasecmp(gn, "teedoff", 7) == 0) { // Tee'd Off
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 10, 13, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'SKINS!' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 1 * 8 + 5, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'GOPHER' LEDs
  }
  else if (strncasecmp(gn, "waterwld", 8) == 0) { // Waterworld
	  // Made from an incomplete manual without schematics. Needs to be checked more thoroughly
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 11, 2, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16, 7, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Backbox flasher (from aux board)
  }
  else if (strncasecmp(gn, "wcsoccer", 8) == 0) { // World Challenge Soccer
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18, 7, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 12 * 8, 8, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Flashers from aux board
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 'SOCCER' LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 11 * 8 + 1, 6, CORE_MODOUT_LED_STROBE_1_10MS); // Challenge LEDs
  }
  else if (strncasecmp(gn, "wipeout", 7) == 0) { // Wipe Out
	  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13, 11, CORE_MODOUT_BULB_89_20V_DC_GTS3); // Playfield flashers
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 0 * 8 + 2, 6, CORE_MODOUT_LED_STROBE_1_10MS); // 6 LEDs
	  core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 1 * 8 + 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 'SKIERS' LEDs
  }
}

/*DMD Generation Init*/
static MACHINE_INIT(gts3dmd) {
	gts3dmd_init();
}

/* Strikes n' Spares: this game uses TWO complete DMD boards! */
static MACHINE_INIT(gts3dmd2) {
  gts3dmd_init();
  memset(&GTS3_dmdlocals[1], 0, sizeof(GTS3_DMDlocals));
  GTS3_dmdlocals[0].has2DMD = 1;

  //Init 2nd 6845
  crtc6845_init(1);
  crtc6845_set_vsync(1, 3579545. / 2., dmd_vblank);

  // Setup PWM shading
  core_dmd_pwm_init(&GTS3_dmdlocals[1].pwm_state, 128, 32, CORE_DMD_PWM_FILTER_GTS3, CORE_DMD_PWM_COMBINER_GTS3_4C_B); // Used to be '_4C_b' games

  /*copy last 32K of DMD ROM into last 32K of CPU region*/
  if (memory_region(GTS3_MEMREG_DCPU2)) {
    memcpy(memory_region(GTS3_MEMREG_DCPU2) + 0x8000,
      memory_region(GTS3_MEMREG_DROM2) + (memory_region_length(GTS3_MEMREG_DROM2) - 0x8000), 0x8000);
  }
}

static MACHINE_STOP(gts3) {
  sndbrd_0_exit();
}

static MACHINE_STOP(gts3dmd) {
  sndbrd_0_exit();
  core_dmd_pwm_exit(&GTS3_dmdlocals[0].pwm_state);
}

static MACHINE_STOP(gts3dmd2) {
  sndbrd_0_exit();
  core_dmd_pwm_exit(&GTS3_dmdlocals[0].pwm_state);
  core_dmd_pwm_exit(&GTS3_dmdlocals[1].pwm_state);
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	//logerror1("SS Write: Offset: %x Data: %x\n",offset,data);
	switch(offset){
		case 0:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			core_write_pwm_output_8b(CORE_MODOUT_SOL0, data);
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
			core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8, data);
			break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
			core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 16, data);
			break;
		case 3:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
			core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 24, data);
			break;
		default:
			logerror1("Solenoid_W Logic Error\n");
	}
}

/*DMD Bankswitching - can handle two DMD displays*/
static void dmdswitchbank(int which)
{
	int	addr =	(GTS3_dmdlocals[which].pa0 ? 0x04000 : 0) |
				(GTS3_dmdlocals[which].pa1 ? 0x08000 : 0) |
				(GTS3_dmdlocals[which].pa2 ? 0x10000 : 0) |
				(GTS3_dmdlocals[which].pa3 ? 0x20000 : 0) |
				(GTS3_dmdlocals[which].a18 ? 0x40000 : 0);
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
	/* Adjust the 16 Segment Layout to match the output order expected by core.c */
	if (offset & 1) { // Hi byte (8..15)
		GTS3locals.activeSegments[offset >> 1].w &= 0x063F;                // Remove bits 6..8 and 11..15
		GTS3locals.activeSegments[offset >> 1].w |= ((data & 0x0F) << 11)  /* 8..11 => 11..14 */
		                                         |  ((data & 0x10) <<  2)  /*    12 =>      6 */
		                                         |  ((data & 0x20) <<  3)  /*    13 =>      8 */
		                                         |  ((data & 0x40) <<  9)  /*    14 =>     15 */
		                                         |  ((data & 0x80)      ); /*    15 =>      7 */
	}
	else { // Lo byte (0..7)
		GTS3locals.activeSegments[offset >> 1].w &= 0xF9C0;               // Remove bits 0..5 and 9..10
		GTS3locals.activeSegments[offset >> 1].w |= ((data & 0x3F)     )  /* 0.. 5 => 0.. 5 */
		                                         |  ((data & 0xC0) << 3); /* 6.. 7 => 9..10 */
	}
	if (((GTS3locals.u4pb & DBLNK) == 0) && (GTS3locals.alphaNumCol < 20)) { // This should never happen since character pattern is loaded to the latch registers while DBLNK is raised
		core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16    , GTS3locals.activeSegments[0].b.lo);
		core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol + 20) * 16 + 8, GTS3locals.activeSegments[0].b.hi);
		core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16    , GTS3locals.activeSegments[1].b.lo);
		core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (GTS3locals.alphaNumCol     ) * 16 + 8, GTS3locals.activeSegments[1].b.hi);
	}
}

/* DMD GENERATION
   ----------------
   DMD Strobe & Reset:
		DS0 = Pulse the DMD CPU IRQ Line (data ready)
		DS1 = Reset the DMD CPU
		DS2 = Not Used
		DS3 = Not Used
*/
static WRITE_HANDLER(dmd_display){
	/*
	 CPU board - DMD communication:
	  - CPU sends data by writing bytes to to 0x2020, triggering DS0 (DMD board latch data and set IRQ to process it)
	  - DMD board acq received data by toggling PB5 pin
	  Note that the written data is latched on the CPU board on all CPU writes, not just the ones to dmd, but the dmd
	  board only latches it when DS0 is fired (so the emulation here is not perfect).
	*/
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
	GTS3_dmdlocals[offset].diagnosticLed = 1 - (data>>7); // DMD LED polarity : negative
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
	//printf("t=%8.5f LDS Write Data=%02x\n", timer_get_time(), data);
	GTS3locals.lampRow = data;
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0     ,  GTS3locals.lampColumn       & 0xFF, GTS3locals.lampRow, 8);
	core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (GTS3locals.lampColumn >> 8) & 0x0F, GTS3locals.lampRow, 4);
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

	//Strikes N Spares Stuff
	if (GTS3_dmdlocals[0].has2DMD)
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

//Update the DMD Frames during the DMD VBLANK generated by the CRTC6845 - Supports 2 DMD Displays
static void dmd_vblank(int which) {
	const UINT8* RAM = memory_region(which ? GTS3_MEMREG_DCPU2 : GTS3_MEMREG_DCPU1) + 0x1000 + (crtc6845_start_address_r(which) >> 2);
	//static double prev; printf("DMD VBlank %6.2fHz: %02x %02x %02x %02x\n", 1. / (timer_get_time() - prev), RAM[20 * 16 + 2], RAM[20 * 16 + 6], RAM[20 * 16 + 10], RAM[20 * 16 + 14]); prev = timer_get_time();
	core_dmd_submit_frame(&GTS3_dmdlocals[which].pwm_state, RAM, 1);
	cpu_set_nmi_line(which ? GTS3_DCPUNO2 : GTS3_DCPUNO, PULSE_LINE);
}

/* Printer connector */
static WRITE_HANDLER(aux1_w)
{
	static void *printfile;
	if (printfile == NULL) {
		char filename[64];
		sprintf(filename,"%s.prt", Machine->gamedrv->name);
		printfile = mame_fopen(Machine->gamedrv->name,filename,FILETYPE_PRINTER,2); // APPEND write mode
	}
	GTS3locals.ax[1+(offset>>4)] = data;
	if (offset < 8) {
		GTS3locals.prn[offset] = data;
		if (!offset) {
			UINT8 printdata = data;
			mame_fwrite(printfile, &printdata, 1);
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

//Show Sound Diagnostic LEDS
void UpdateSoundLEDS(int num,UINT8 bit)
{
	if(num==0)
		GTS3locals.diagnosticLed1 = bit;
	else
		GTS3locals.diagnosticLed2 = bit;
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
{0x0000,0x1fff, MRA_RAM},               /* DMD RAM         */
{0x2801,0x2801, crtc6845_register_0_r}, /* CRTC index      */
{0x3000,0x3000, dmdlatch_r},            /* Input Port      */
{0x4000,0x7fff, MRA_BANK1},             /* Paginated ROM   */
{0x8000,0xffff, MRA_ROM},               /* ROM             */
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem)
{0x0000,0x0fff, MWA_RAM},               /* DMD RAM         */
{0x1000,0x1fff, MWA_RAM},               /* DMD Display RAM */
{0x2800,0x2800, crtc6845_address_0_w},  /* CRTC index      */
{0x2801,0x2801, crtc6845_register_0_w}, /* CRTC registers  */
{0x3800,0x3800, dmdoport},              /* Output Port     */
{0x4000,0x7fff, MWA_BANK1},             /* Paginated ROM   */
{0x8000,0xffff, MWA_ROM},               /* ROM             */
MEMORY_END

//NOTE: DMD #2 for Strikes N Spares - Identical to DMD #1 hardware & memory map
static MEMORY_READ_START(GTS3_dmdreadmem2)
{0x0000,0x1fff, MRA_RAM},               /* DMD RAM         */
{0x2801,0x2801, crtc6845_register_1_r}, /* CRTC index      */
{0x3000,0x3000, dmdlatch2_r},           /* Input Port      */
{0x4000,0x7fff, MRA_BANK2},             /* Paginated ROM   */
{0x8000,0xffff, MRA_ROM},               /* ROM             */
MEMORY_END

static MEMORY_WRITE_START(GTS3_dmdwritemem2)
{0x0000,0x0fff, MWA_RAM},               /* DMD RAM         */
{0x1000,0x1fff, MWA_RAM},               /* DMD Display RAM */
{0x2800,0x2800, crtc6845_address_1_w},  /* CRTC index      */
{0x2801,0x2801, crtc6845_register_1_w}, /* CRTC registers  */
{0x3800,0x3800, dmdoport2},             /* Output Port     */
{0x4000,0x7fff, MWA_BANK2},             /* Paginated ROM   */
{0x8000,0xffff, MWA_ROM},               /* ROM             */
MEMORY_END

MACHINE_DRIVER_START(gts3)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M65C02, 2000000) // XTAL(4'000'000) / 2
  MDRV_CPU_MEMORY(GTS3_readmem, GTS3_writemem)
  MDRV_CPU_VBLANK_INT(GTS3_interface_update, GTS3_INTERFACE_UPD_PER_FRAME)
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

MACHINE_DRIVER_START(gts3_21)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CPU_ADD(M65C02, 3579545./2.)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem, GTS3_dmdwritemem)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd,NULL,gts3dmd)
  MDRV_IMPORT_FROM(gts80s_s3)
MACHINE_DRIVER_END

//Sound Interface for Strikes N Spares
static struct OKIM6295interface sns_okim6295_interface = {
	1,					/* 1 chip */
	{ 2000000./132. },	/* sampling frequency at 2MHz chip clock */
	{ REGION_USER3 },	/* memory region */
	{ 75 }				/* volume */
};

// 2nd DMD CPU for Strikes n' Spares
MACHINE_DRIVER_START(gts3_22)
  MDRV_IMPORT_FROM(gts3)
  MDRV_CPU_ADD(M65C02, 3579545./2.)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem, GTS3_dmdwritemem)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd,NULL,gts3)
  MDRV_CPU_ADD(M65C02, 3579545./2.)
  MDRV_CPU_MEMORY(GTS3_dmdreadmem2, GTS3_dmdwritemem2)
  MDRV_CORE_INIT_RESET_STOP(gts3dmd2,NULL,gts3dmd2)
  MDRV_SOUND_ADD(OKIM6295, sns_okim6295_interface)
  MDRV_DIAGNOSTIC_LEDH(3)
MACHINE_DRIVER_END
