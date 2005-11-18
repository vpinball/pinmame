/************************************************************************************************
  Alvin G and Company
  -------------------
  by Steve Ellenoff (08/20/2003) and Gerrit (for Alpha Numeric Display in Generation 1)

  Hardware from 1991-1994

  CPU BOARD: (Might have been a slight revision between generations but otherwise functionally the same)
	CPU: 65c02 @ 2 Mhz?
	I/O: 2 x 6522 VIA, 3 X 8255

  Generation #1 (Games Up to Al's Garage Band?)
  ---------------------------------------------
  SOUND BOARD:
    CPU: 6809 @ 2 Mhz
	I/O: 6255 VIA
	SND: YM3812 (Music), OKI6295 (Speech)

  DISPLAY BOARD:
    ALPHA NUMERIC SEGMENTS ( 2 DISPLAYS OF 20 DIGIT 16 ALPHA/NUMERIC SEGMENTS )
	I/O: 8255

  Generation #2 (All remaining games)
  -----------------------------------
  SOUND BOARD:
	CPU: 68B09 @ 8 Mhz? (Sound best @ 1 Mhz)
	I/O: buffers
	SND: BSMT2000 @ 24Mhz

  DISPLAY BOARD:
    DMD CONTROLLER:
	CPU: 8031 @ 12 Mhz? (Displays properly @ 1-2 Mhz)


  All Generations
  ---------------
  Lamp Matrix     = 8x12 = 96 Lamps
  Switch Matrix   = 8X12 = 96 Switches
  Solenoids	= 32 Solenoids

  CPU Diag LED Flashes:
  Normal - 1 Flash/Sec
  2 Flashes and stops: ROM Error
  3 Flashes and stops: Switch returns or U7 - 6522
  4 Flashes and stops: 4 Direct switches or U7 - 6522
  5 Flashes and stops: U8 - 6522

  Misc
  ----
  Main CPU NMI - Controls DMD Commands (and Lamp Generation in Pistol Poker)
  Main CPU IRQ - Switch Strobing/Reading (and maybe other stuff)
  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET

   Hacks & Issues that need to be looked into:
   -------------------------------------------
   #1) Manually calling the VIA_IRQ	(VIA BUG?)
   #2) Manually calling the NMI		(VIA BUG?)
   #3) Clearing the Lamp Matrix at the appropriate time
   #4) Lamp and Switch Column handling needs to be improved (use core functions)
   #5) Not sure if I did the sw2m and m2sw functions properly
   #6) Solenoid handling needs to be cleaned up (ie, remove unnecessary code)
   #7) Sound board (gen #2) FIRQ freq. is set by a jumper (don't know which is used) nor what the value of E is.
   #8) There's probably more I can't think of at the moment
   #9) Look into error log message from VIA chip about no callback handler for Timer.
  #10) Hack used to get U8 test to pass on Generation #1 games

**************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6502/m65ce02.h"
#include "machine/6522via.h"
#include "machine/8255ppi.h"
#include "core.h"
#include "sndbrd.h"
#include "alvg.h"
#include "alvgs.h"
#include "alvgdmd.h"

#ifdef VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define ALVG_VBLANKFREQ      60 /* VBLANK frequency*/

WRITE_HANDLER(alvg_sndCmd_w);

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT16 lampColumn, swColumn;
  int    lampRow, swCol;
  int    diagnosticLed;
  int    diagnosticLeds1;
  int    diagnosticLeds2;
  int    ssEn;
  int    mainIrq;
  int    DMDAck;
  int    DMDClock;
  int    DMDEnable;
  int    DMDData;
  int	 swTest;
  int    swEnter;
  int    swAvail1;
  int    swAvail2;
  int    swTicket;
  int	 via_1_b;
  int    sound_strobe;
  int    dispCol;
} alvglocals;

static UINT16 segMapper(UINT16 value) {
	UINT16 result = value & 0x047f;
	result |= (value & 0x80) ? 0x800 : 0;
	result |= (value & 0x100) ? 0x200 : 0;
	result |= (value & 0x200) ? 0x2000 : 0;
	result |= (value & 0x800) ? 0x1000 : 0;
	result |= (value & 0x1000) ? 0x4000 : 0;
	result |= (value & 0x2000) ? 0x100 : 0;
	result |= (value & 0x4000) ? 0x8000 : 0;
	result |= (value & 0x8000) ? 0x80 : 0;
	return result;
}

/*Receive command from DMD
 Pins are wired as:
    PIN   CPU  - DMD      - Data Bus
	-----------------------------------
	16 -  ENA  - ACK2     - D3 - OR D3?
	17 -  ACK  - ACK1     - D0 - OR D2?
	18 -  CLK  - TOGGLE?  - D2 - OR D1?
	19 -  DATA - READY    - D1 - OR D0?
 */
static WRITE_HANDLER(data_from_dmd)
{
	data&=0x0f;
	alvglocals.DMDAck    = ((data & 1) >> 0);
	alvglocals.DMDData   = ((data & 2) >> 1);
	alvglocals.DMDClock  = ((data & 4) >> 2);
	alvglocals.DMDEnable = ((data & 8) >> 3);
}


/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
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
			LOG(("Solenoid_W Logic Error\n"));
	}
}

//See U7-PB Read for more info
READ_HANDLER(CoinDoorSwitches_Read)
{
//According to manual (but doesn't work)
#if 0
	int data = 0;
	data |= (alvglocals.DMDAck   << 7);	 //DMD Ack		(?)
	data |= (alvglocals.swTest   << 5);	 //Test Sw.		(Not Inverted)
	data |= (alvglocals.swEnter  << 4);  //Enter Sw.	(Not Inverted)
	data |= (alvglocals.swAvail1 << 3);  //Avail1		(Not Inverted)
	data |= (alvglocals.swAvail2 << 2);  //Avail2		(Not Inverted)
#else
//Seems to work, note that the ticket switch may only be set when the ticket solenoid is pulled in,
//as this input is also connected to a flasher lamp test!
	int data = 0;
	data |= (alvglocals.DMDAck   << 7);	 //DMD Ack		(?)				(8)
	data |= (alvglocals.swTicket << 5);	 //Ticket Sw.	(Not Inverted)	(6)
	data |= (alvglocals.swEnter  << 4);  //Enter Sw.	(Not Inverted)	(5)
	data |= (alvglocals.swAvail2 << 3);  //Avail2		(Not Inverted)	(4)
	data |= (alvglocals.swTest   << 2);	 //Test Sw.		(Not Inverted)	(3)
	data |= (alvglocals.swAvail1 << 1);  //Avail1		(Not Inverted)	(2)
#endif
//printf("%x:data = %x\n",activecpu_get_previouspc(),data);
return data;
}

/* U7 - 6522 */

//PA0-7 (IN) - Switch Return (Switches are inverted!)
static READ_HANDLER( xvia_0_a_r ) { return coreGlobals.swMatrix[alvglocals.swCol]^0xff; }

//PB0-7 (IN)
/*
PB7  (In)  = DMD ACK
PB6  (In)  = NU
PB5  (In)  = Test Switch - Coin Door Switch
PB4  (In)  = Enter Switch - Coin Door Switch
PB3  (In)  = AVAI1 - Coin Door Switch (Flasher Sense in Pistol Poker)
PB2  (In)  = AVAI2 - Coin Door Switch (Ticket Sense in Pistol Poker)
PB1  (In)  = NU
PB0  (In)  = NU
*/
static READ_HANDLER( xvia_0_b_r ) { return CoinDoorSwitches_Read(0); }
//CA1: (IN) - N.C.
static READ_HANDLER( xvia_0_ca1_r ) { LOG(("WARNING: N.C.: U7-CA1-R\n")); return 0; }
//CB1: (IN) - N.C.
static READ_HANDLER( xvia_0_cb1_r ) { LOG(("WARNING: N.C.: U7-CB1-R\n")); return 0; }
//CA2:  (IN)
static READ_HANDLER( xvia_0_ca2_r ) { LOG(("U7-CA2-R\n")); return 0; }
//CB2: (IN)
static READ_HANDLER( xvia_0_cb2_r ) { LOG(("U7-CB2-R\n")); return 0; }

//PA0-7: (OUT) - N.C.
static WRITE_HANDLER( xvia_0_a_w ) { LOG(("WARNING - N.C.: U7-A-W: data=%x\n",data)); }

//PB0-7: (OUT)
/*
PB7  (Out) = NU
PB6  (Out) = NAND Gate -> WD (WatchDog)
PB5  (Out) = NU
PB4  (Out) = NU
PB3  (Out) = NU
PB2  (Out) = NU
PB1  (out) = NAND Gate -> WD (WatchDog)
PB0  (out) = N/C
*/
static WRITE_HANDLER( xvia_0_b_w ) {
	//if(data & 0x42) watchdog_reset_w(0,0);
	if(data && !(data & 0x40 || data & 0x42)) LOG(("WARNING: U7-B-W: data=%x\n",data));
}

//CA2: (OUT) - N.C.
static WRITE_HANDLER( xvia_0_ca2_w ) { LOG(("%x:WARNING: N.C.: U7-CA2-W: data=%x\n",activecpu_get_previouspc(),data)); }

//CB2: (OUT) - NMI TO MAIN 65C02
static WRITE_HANDLER( xvia_0_cb2_w )
{
	//printf("NMI: U7-CB2-W: data=%x\n",data);
	cpu_set_nmi_line(ALVG_CPUNO, PULSE_LINE);
}

/* U8 - 6522 */

//PA0-7 (IN) - N/C
static READ_HANDLER( xvia_1_a_r ) { LOG(("WARNING: U8-PA-R\n")); return 0; }

//PB0-7 (IN)
/*
PB7        = NU
PB6        = NU
PB5  (In)  = DMD Data
PB4  (In)  = DMD Clock
PB3  (In)  = DMD Enable
PB2        = NU
PB1        = NU
PB0        = NU*/
static READ_HANDLER( xvia_1_b_r ) {
	int data = alvglocals.via_1_b;
	data = ((data&0xf7) | alvglocals.DMDEnable) +
		   ((data&0xef) | alvglocals.DMDClock)  +
		   ((data&0xdf) | alvglocals.DMDData);
//printf("%x:U8-PB-R: data = %x\n",activecpu_get_previouspc(),data);
	return data;
}
//CA1: (IN) - Sound Control
static READ_HANDLER( xvia_1_ca1_r ) { return sndbrd_0_ctrl_r(0); }
//CB1: (IN) - N.C.
static READ_HANDLER( xvia_1_cb1_r ) { LOG(("WARNING: N.C.: U8-CB1-R\n")); return 0; }
//CA2:  (IN) - N.C.
static READ_HANDLER( xvia_1_ca2_r ) { LOG(("WARNING: N.C.: U8-CA2-R\n")); return 0; }
//CB2: (IN) - N.C.
static READ_HANDLER( xvia_1_cb2_r ) { LOG(("WARNING: U8-CB2-R\n")); return 0; }

//PA0-7: (OUT) - Sound Data
static WRITE_HANDLER( xvia_1_a_w ) { sndbrd_0_data_w(0,data); }

//PB0-7: (OUT)
/*
PB7        = NU
PB6        = NU
PB5        = Display Data (Generation #1 Only)
PB4        = Display Clock (Generation #1 Only)
PB3		   = Display Enable (Generation #1 Only)
PB2        = NU
PB1  (Out) = Sound Clock
PB0        = NU
*/
static WRITE_HANDLER( xvia_1_b_w ) {
	//printf("%x:U8-PB-W: data = %x\n",activecpu_get_previouspc(),data);

	//On clock transition - write to sound latch
	if(!alvglocals.sound_strobe && (data & 0x02))	sndbrd_0_ctrl_w(0,0);
	alvglocals.sound_strobe = data&0x02;

	if (data & ~alvglocals.via_1_b & 0x10)
		alvglocals.dispCol = (alvglocals.dispCol + 1) % 20;
	if (data & 0x20)
		alvglocals.dispCol = 0;
	alvglocals.via_1_b = data;
}
//CA2: (OUT) - CPU DIAG LED
static WRITE_HANDLER( xvia_1_ca2_w ) { 	alvglocals.diagnosticLed = data; }
//CB2: (OUT) - N.C.
static WRITE_HANDLER( xvia_1_cb2_w ) { LOG(("WARNING: N.C.: U8-CB2-W: data=%x\n",data)); }

//IRQ:  IRQ to Main CPU
static void via_irq(int state) {
//	printf("IN VIA_IRQ - STATE = %x\n",state);
	cpu_set_irq_line(ALVG_CPUNO, 0, state?ASSERT_LINE:CLEAR_LINE);
//	cpu_set_irq_line(ALVG_CPUNO, 0, PULSE_LINE);
}

/*U12 - 8255*/
/*
PA0-PA7 (out) = Sol 01-08
PB0-PB7 (out) = Sol 09-16
PC0-PC7 (out) = Sol 17-24
*/
WRITE_HANDLER(u12_porta_w) { solenoid_w(0,data); }
WRITE_HANDLER(u12_portb_w) { solenoid_w(1,data); }
WRITE_HANDLER(u12_portc_w) { solenoid_w(2,data); }
/*U13 - 8255*/
/*
PA0-PA7 (out)  = Sol 25-32
PB0-PB7 (out)  = Switch Strobe 1-8					(inverted)
PC0-PC7 (out)  = Switch Strobe 9-12	(bits 4-7 nc)	(inverted)
*/
WRITE_HANDLER(u13_porta_w) { solenoid_w(3,data); }

void UpdateSwCol(void) {
	int i, tmp, data;
	i = data = 0;
	tmp = alvglocals.swColumn;
	while(tmp)
	{
		i++;
		if(tmp&1) data+=i;
		tmp = tmp>>1;
	}
	alvglocals.swCol = data;
	//printf("COL = %x SwColumn = %d\n",alvglocals.swColumn,data);
	//printf("SwColumn = %d\n",data);
}

WRITE_HANDLER(u13_portb_w) {
	alvglocals.swColumn = (alvglocals.swColumn&0xff00) | (data^0xff);
	UpdateSwCol();
	//printf("SWITCH_STROBE(1-8): data = %x\n",data);
}
WRITE_HANDLER(u13_portc_w) {
	alvglocals.swColumn = (alvglocals.swColumn&0x00ff) | ((data^0xff)<<8);
	UpdateSwCol();
	//printf("SWITCH_STROBE(9-12): data = %x\n",data);
}


/*U14 - 8255*/
/*
PA0-PA7 (out) = Lamp Strobe 1-7  (bits 7 nc)
PB0-PB7 (out) = Lamp Strobe 8-12 (bits 5-7 nc)
PC0-PC7 (out) = Lamp Return 1-8
*/

void UpdateLampCol(void) {
	int i, tmp, lmpCol;
	i = lmpCol = 0;
	tmp = alvglocals.lampColumn;
	while(tmp)
	{
		i++;
		if(tmp&1) lmpCol+=i;
		tmp = tmp>>1;
	}
	coreGlobals.tmpLampMatrix[lmpCol-1] =
		(coreGlobals.tmpLampMatrix[lmpCol-1]&0xff) | alvglocals.lampRow;
	//printf("COL = %x LampColumn = %d\n",alvglocals.lampColumn,data);
	//printf("LampColumn = %d\n",data);
}

/* - Lamp Handling -
   -----------------
   It seems Pistol Poker handles lamps differently from the other games.
   First, the lamp columns are not standard ordering, like the others.
   Second, it writes the lamp column data first, then the lamp data itself.
   The other games write the lamp data first, then the lamp column data. - */
WRITE_HANDLER(u14_porta_w) {
	if (core_gameData->hw.gameSpecific1)
		alvglocals.lampColumn = (alvglocals.lampColumn&0x0f01) | ((data & 0x7f)<<1);
	else {
		alvglocals.lampColumn = (alvglocals.lampColumn&0x0f80) | (data & 0x7f);
		UpdateLampCol();
	}
	//printf("LAMP STROBE(1-7):  data = %x\n",data&0x7f);
}
WRITE_HANDLER(u14_portb_w) {
	if (core_gameData->hw.gameSpecific1)
		alvglocals.lampColumn = (alvglocals.lampColumn&0x00fe) | ((data & 0x0f)<<8) | ((data & 0x10)>>4);
	else {
		alvglocals.lampColumn = (alvglocals.lampColumn&0x007f) | ((data & 0x1f)<<7);
		UpdateLampCol();
	}
	//printf("LAMP STROBE(8-12): data = %x\n",data);
}
WRITE_HANDLER(u14_portc_w) {
	alvglocals.lampRow = data;
	if (core_gameData->hw.gameSpecific1)
		UpdateLampCol();
	//printf("LAMP RETURN: data = %x\n",data);
}

/*
U7 - 6522

PA0-7(In)  = Switch Column 1-8 Return
PB7  (In)  = DMD ACK
PB6  (Out) = NAND Gate -> WD (WatchDog)
PB5  (In)  = Test Switch - Cabinet Switch
PB4  (In)  = Enter Switch - Cabinet Switch
PB3  (In)  = AVAI1 - Cabinet Switch
PB2  (In)  = AVAI2 - Cabinet Switch
PB1  (Out) = NAND Gate -> WD (WatchDog)
PB0        = N/C
CA1        = N/C
CA2        = N/C
CB1        = N/C
CB2  (Out) = 65C02 NMI
IRQ        = 65C02 IRQ

U8 - 6522

PA0-7(Out) = Sound Data
PB7        = N/C
PB6        = N/C
PB5  (In)  = DMD Data
PB4  (In)  = DMD Clock
PB3  (In)  = DMD Enable
PB2        = N/C
PB1  (Out) = Sound Clock
PB0        = N/C
CA1  (In)  = Sound Ctrl
CA2  (Out) = CPU LED
CB1        = N/C
CB2        = N/C
IRQ        = 65C02 IRQ

*/
static struct via6522_interface via_0_interface =
{
	/*inputs : A/B           */ xvia_0_a_r, xvia_0_b_r,
	/*inputs : CA1/B1,CA2/B2 */ xvia_0_ca1_r, xvia_0_cb1_r, xvia_0_ca2_r, xvia_0_cb2_r,
	/*outputs: A/B,CA2/B2    */ xvia_0_a_w, xvia_0_b_w, xvia_0_ca2_w, xvia_0_cb2_w,
	/*irq                    */ via_irq
};
static struct via6522_interface via_1_interface =
{
	/*inputs : A/B           */ xvia_1_a_r, xvia_1_b_r,
	/*inputs : CA1/B1,CA2/B2 */ xvia_1_ca1_r, xvia_1_cb1_r, xvia_1_ca2_r, xvia_1_cb2_r,
	/*outputs: A/B,CA2/B2    */ xvia_1_a_w, xvia_1_b_w, xvia_1_ca2_w, xvia_1_cb2_w,
	/*irq                    */ via_irq
};

/*
U12 - 8255

PA0-PA7 (out) = Sol 01-08
PB0-PB7 (out) = Sol 09-16
PC0-PC7 (out) = Sol 17-24

U13 - 8255

PA0-PA7 (out)  = Sol 25-32
PB0-PB7 (out)  = Switch Strobe 1-8					(inverted)
PC0-PC7 (out)  = Switch Strobe 9-12	(bits 4-7 nc)	(inverted)

U14 - 8255

PA0-PA7 (out) = Lamp Strobe 1-7  (bits 7 nc)
PB0-PB7 (out) = Lamp Strobe 8-12 (bits 5-7 nc)
PC0-PC7 (out) = Lamp Return 1-8
*/

// Low seg row A
static WRITE_HANDLER(disp_porta_w) {
  coreGlobals.segments[alvglocals.dispCol].w |= segMapper(data);
}

// Hi seg row A
static WRITE_HANDLER(disp_portb_w) {
  coreGlobals.segments[alvglocals.dispCol].w = segMapper(data << 8);
}

// Low seg row B
static WRITE_HANDLER(disp_portc_w) {
  coreGlobals.segments[20+alvglocals.dispCol].w |= segMapper(data);
}

static ppi8255_interface ppi8255_intf =
{
	4, 												/* 4 chips */
	{0, 0, 0, 0},										/* Port A read */
	{0, 0, 0, 0},										/* Port B read */
	{0, 0, 0, 0},										/* Port C read */
	{u12_porta_w, u13_porta_w, u14_porta_w, disp_porta_w},		/* Port A write */
	{u12_portb_w, u13_portb_w, u14_portb_w, disp_portb_w},		/* Port B write */
	{u12_portc_w, u13_portc_w, u14_portc_w, disp_portc_w},		/* Port C write */
};


static INTERRUPT_GEN(alvg_vblank) {
  //hack to improve the lamp display
  static int lclear=0;
  lclear++;

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  alvglocals.vblankCount += 1;

  /*-- lamps --*/
  if ((alvglocals.vblankCount % ALVG_LAMPSMOOTH) == 0) {
	memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
	//don't clear lamp display every time (this is a hack)
	if((lclear%25)==0){
		memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
		lclear=0;
	}
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = alvglocals.solenoids;
  if ((alvglocals.vblankCount % ALVG_SOLSMOOTH) == 0) {
	if (alvglocals.ssEn) {
	  int ii;
	  coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
	  /*-- special solenoids updated based on switches --*/
	  for (ii = 0; ii < 6; ii++)
		if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
		  coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
	}
	alvglocals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((alvglocals.vblankCount % ALVG_DISPLAYSMOOTH) == 0) {
	/*update leds*/
	coreGlobals.diagnosticLed = (alvglocals.diagnosticLeds2<<2) |
								(alvglocals.diagnosticLeds1<<1) |
								alvglocals.diagnosticLed;
	//alvglocals.diagnosticLed = 0;	//For some reason, LED won't work with this line in
	alvglocals.diagnosticLeds1 = 0;
	alvglocals.diagnosticLeds2 = 0;
  }
  core_updateSw(core_getSol(27));	//Flipper Enable Relay
}

static SWITCH_UPDATE(alvg) {
  via_irq(1);			//Why is this necessary? - Seems to help switch scanning..
  xvia_0_cb2_w(0,1);	//Force an NMI call - fixes lamps in Pistol Poker & sending data to DMD

  if (inports) {
    coreGlobals.swMatrix[0] = (inports[ALVG_COMINPORT] & 0x0700)>>8;								 	    //Column 0 Switches
	coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xe0) | (inports[ALVG_COMINPORT] & 0x1f);		    //Column 1 Switches
	coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0x3c) | ((inports[ALVG_COMINPORT] & 0x1860)>>5);	//Column 2 Switches
  }
  alvglocals.swTest = (core_getSw(ALVG_SWTEST)>0?1:0);
  alvglocals.swEnter = (core_getSw(ALVG_SWENTER)>0?1:0);
  alvglocals.swTicket = (core_getSw(ALVG_SWTICKET)?1:0);

  //Update Flasher Relay
  {
  int relay=(coreGlobals.solenoids & 0x4000)>>14;
  alvglocals.swAvail1 = relay;
  alvglocals.swAvail2 = relay;
  }

  //Not necessary it seems..
  //Force VIA to see the coin door switch values
  //via_0_portb_w(0,CoinDoorSwitches_Read(0));
}

//Send a sound command to the sound board
WRITE_HANDLER(alvg_sndCmd_w) {
	sndbrd_0_data_w(0, data);
	sndbrd_0_ctrl_w(0, 0);
}

static int alvg_sw2m(int no) {
	return no + 7;
}

static int alvg_m2sw(int col, int row) {
	return col*8 + row - 9;
}

//Send data to the DMD CPU
static WRITE_HANDLER(DMD_LATCH) {
	sndbrd_1_data_w(0,data);
	sndbrd_1_ctrl_w(0,0);
}

//Send data to the display segments
// Hi seg row B
static WRITE_HANDLER(LED_LATCH) {
  coreGlobals.segments[20+alvglocals.dispCol].w = segMapper(data << 8);
}
static WRITE_HANDLER(LED_DATA) {
  ppi8255_3_w(3-offset, data);
}

/*Machine Init*/
static void init_common(void) {
  memset(&alvglocals, 0, sizeof(alvglocals));

  /* init VIA */
  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /*watchdog*/
  //watchdog_reset_w(0,0);

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, ALVGS_CPUNO,   memory_region(ALVGS_ROMREGION)  ,NULL,NULL);
}
static MACHINE_INIT(alvg) {
  init_common();
  install_mem_write_handler(0, 0x2c00, 0x2c00, LED_LATCH);
  install_mem_write_handler(0, 0x2c80, 0x2c83, LED_DATA);
}
static MACHINE_INIT(alvgdmd) {
  init_common();
  /* Init the dmd board */
  install_mem_write_handler(0, 0x2c00, 0x2fff, DMD_LATCH);
  sndbrd_1_init(core_gameData->hw.display,    ALVGDMD_CPUNO, memory_region(ALVGDMD_ROMREGION),data_from_dmd,NULL);
}

static MACHINE_STOP(alvg) {
  sndbrd_0_exit();
  sndbrd_1_exit();
}
//Show Sound & DMD Diagnostic LEDS
void alvg_UpdateSoundLEDS(int num,int data)
{
	if(num==0)
		alvglocals.diagnosticLeds1 = data;
	else
		alvglocals.diagnosticLeds2 = data;
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(alvg) {
  core_nvram(file, read_or_write, memory_region(ALVG_MEMREG_CPU), 0x2000, 0x00);
}

//Hack to get Punchy & Other Generation #1 games to pass the U8 startup test..
//NOTE: LED 5 Flashes Test of U8 begins @ line 40FC in Punchy
READ_HANDLER(cust_via_1_r)
{
	if(offset==0)
	{
 		int data = via_1_r(offset);
		if (data == 0)
			return 0x10;
		else
			return data;
	}
	else
		return via_1_r(offset);
}

/*---------------------------
/  Memory map for main CPU
/----------------------------
0  0  0  = Y0 =<0x2400 = SOLENOIDS				(/SEL1) -> U12 - 8255
0  0  1  = Y1 = 0x2400 = SOLENOIDS/SWITCH ROWS  (/SEL2) -> U13 - 8255
0  1  0  = Y2 = 0x2800 = LAMPS    				(/SEL3) -> U14 - 8255
0  1  1  = Y3 = 0x2C00 = DISPLAYS 				(/SEL4) -> INPUT SELECT/LATCH ON DMD CONTROLLER
1  0  0  = Y4 = 0x3000 = N/C?
1  0  1  = Y5 = 0x3400 = N/C?
1  1  0  = Y6 = 0x3800 = U8 - 6255 Enable
1  1  1  = Y7 = 0x3C00 = U7 - 6255 Enable
*/

static MEMORY_READ_START(alvg_readmem)
{0x0000,0x1fff,MRA_RAM},
{0x2000,0x2003,ppi8255_0_r},
{0x2400,0x2403,ppi8255_1_r},
{0x2800,0x2803,ppi8255_2_r},
//{0x3800,0x380f,via_1_r},
{0x3800,0x380f,cust_via_1_r},
{0x3c00,0x3c0f,via_0_r},
{0x4000,0xffff,MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(alvg_writemem)
{0x0000,0x1fff,MWA_RAM},
{0x2000,0x2003,ppi8255_0_w},
{0x2400,0x2403,ppi8255_1_w},
{0x2800,0x2803,ppi8255_2_w},
{0x3800,0x380f,via_1_w},
{0x3c00,0x3c0f,via_0_w},
{0x4000,0xffff,MWA_ROM},
MEMORY_END

//Main Machine Driver (Main CPU Only)
MACHINE_DRIVER_START(alvg)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M65C02, 2000000)
  MDRV_CPU_MEMORY(alvg_readmem, alvg_writemem)
  MDRV_CPU_VBLANK_INT(alvg_vblank, ALVG_VBLANKFREQ)
  MDRV_NVRAM_HANDLER(alvg)
  MDRV_CORE_INIT_RESET_STOP(alvg,NULL,alvg)
  MDRV_SWITCH_UPDATE(alvg)
  MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_CONV(alvg_sw2m,alvg_m2sw)
MACHINE_DRIVER_END

//Main CPU, Sound hardware Driver (Generation #1)
MACHINE_DRIVER_START(alvgs1)
  MDRV_IMPORT_FROM(alvg)
  MDRV_IMPORT_FROM(alvg_s1)
  MDRV_SOUND_CMD(alvg_sndCmd_w)
  MDRV_SOUND_CMDHEADING("alvg")
MACHINE_DRIVER_END

//Main CPU, Sound hardware Driver (Generation #2)
MACHINE_DRIVER_START(alvgs2)
  MDRV_IMPORT_FROM(alvg)
  MDRV_IMPORT_FROM(alvg_s2)
  MDRV_SOUND_CMD(alvg_sndCmd_w)
  MDRV_SOUND_CMDHEADING("alvg")
MACHINE_DRIVER_END

//Main CPU, DMD, Sound hardware Driver (Generation #2)
MACHINE_DRIVER_START(alvgs2dmd)
  MDRV_IMPORT_FROM(alvgs2)
  MDRV_IMPORT_FROM(alvgdmd)
  MDRV_CORE_INIT_RESET_STOP(alvgdmd,NULL,alvg)
MACHINE_DRIVER_END

//Use only to test 8031 core
#ifdef MAME_DEBUG
MACHINE_DRIVER_START(alvg_test8031)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_IMPORT_FROM(test8031)
  MDRV_SWITCH_UPDATE(alvg)
MACHINE_DRIVER_END
#endif
