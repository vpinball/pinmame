/************************************************************************************************
  Alvin G and Company
  -----------------
  by Steve Ellenoff (08/20/2003)

  Hardware from 1992-1994?

  CPU BOARD: 
	CPU: 65c02 @ 2 Mhz?
	I/O: 2 x 6522 VIA, 3 X 8255

  SOUND BOARD:
	CPU: 68B09 @ 1 Mhz? (Sound best @ 1Mhz)
	I/O: buffers
	SND: BSMT2000 @ 24Mhz

  DMD BOARD:
	CPU: 8031 @ 12 Mhz?

  Lamp Matrix     = 8x12 = 96 Lamps
  Switch Matrix   = 8X12 = 96 Switches
  Solenoids	= 32 Solenoids

  CPU Diag LED Flashes:
  Normal - 1 Flash/Sec
  2 Flashes and stops: ROM Error
  3 Flashes and stops: Switch returns or U7 - 6522
  4 Flashes and stops: 4 Direct switches or U7 - 6522
  5 Flashes and stops: U8 - 6522

  65c02: Vectors: FFFE&F = IRQ, FFFA&B = NMI, FFFC&D = RESET
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

#define ALVG_VBLANKFREQ      60 /* VBLANK frequency*/

#define ALVG_CPUNO	0
//#define ALVG_DCPUNO 1
//#define ALVG_SCPUNO 2

WRITE_HANDLER(alvg_sndCmd_w);

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    diagnosticLeds1;
  int    diagnosticLeds2;
  int    swCol;
  int    ssEn;
  int    mainIrq;
  int    DMDAck;
  int	 swTest;
  int    swEnter;
  int    swVolUp;
  int    swVolDn;
  int	 via_1_b;
  int    sound_strobe;
  int    sound_data;
} alvglocals;

struct {
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
} alvg_dmdlocals;


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
			logerror("Solenoid_W Logic Error\n");
	}
}

/* U7 - 6522 */

//PA0-7 (IN) - Switch Return (Switches are inverted!)
static READ_HANDLER( xvia_0_a_r ) { 
	int data = core_getSwCol(alvglocals.swCol)^0xff;
	//logerror("%x: SWITCH READ: U7-PA-R = %x\n",activecpu_get_previouspc(),data); 
	return data;
}
//PB0-7 (IN)
/*
PB7  (In?) = DMD ACK
PB6  (In)  = NU
PB5  (In)  = Test Switch - Coin Door Switch			
PB4  (In)  = Enter Switch - Coin Door Switch
PB3  (In)  = AVAI1 (Volume Up?) - Coin Door Switch
PB2  (In)  = AVAI2 (Volume Down?) - Coin Door Switch
PB1  (In)  = NU
PB0  (In)  = NU
*/
static READ_HANDLER( xvia_0_b_r ) { 
	int data = 0;
	//alvglocals.DMDAck = !alvglocals.DMDAck;
	data |= (alvglocals.DMDAck  << 7);	 //DMD Ack?
	data |= (alvglocals.swTest  << 5);	 //Test Sw.		(Not Inverted)
	data |= (alvglocals.swEnter << 4);   //Enter Sw.	(Not Inverted)
	data |= (alvglocals.swVolUp << 3);   //Vol Up Sw.	(Not Inverted)
	data |= (alvglocals.swVolDn << 2);   //Vol Down Sw.	(Not Inverted)
	//logerror("%x: COIN DOOR SWITCH: U7-PB-R - reading data=%x\n",activecpu_get_previouspc(),data); 
	return data;
}
//CA1: (IN) - N.C.
static READ_HANDLER( xvia_0_ca1_r ) { logerror("WARNING: N.C.: U7-CA1-R\n"); return 0; }
//CB1: (IN) - N.C.
static READ_HANDLER( xvia_0_cb1_r ) { logerror("WARNING: N.C.: U7-CB1-R\n"); return 0; }
//CA2:  (IN)
static READ_HANDLER( xvia_0_ca2_r ) { logerror("U7-CA2-R\n"); return 0; }
//CB2: (IN)
static READ_HANDLER( xvia_0_cb2_r ) { logerror("U7-CB2-R\n"); return 0; }

//PA0-7: (OUT) - N.C.
static WRITE_HANDLER( xvia_0_a_w ) { 
	logerror("WARNING - N.C.: U7-A-W: data=%x\n",data);
}
//PB0-7: (OUT)
/*
PB7  (Out) = NU?
PB6  (Out) = NAND Gate -> WD (WatchDog)
PB5  (Out) = NU
PB4  (Out) = NU
PB3  (Out) = NU
PB2  (Out) = NU
PB1  (out) = NAND Gate -> WD (WatchDog)
PB0  (out) = N/C
*/
static WRITE_HANDLER( xvia_0_b_w ) { 
	if(!(data & 0x40 || data & 0x42))
		logerror("U7-B-W: data=%x\n",data);
}
//CA2: (OUT) - N.C.
static WRITE_HANDLER( xvia_0_ca2_w ) { logerror("%x:WARNING: N.C.: U7-CA2-W: data=%x\n",activecpu_get_previouspc(),data); }

//CB2: (OUT) - NMI TO MAIN 65C02
static WRITE_HANDLER( xvia_0_cb2_w ) 
{ 
	//logerror("NMI: U7-CB2-W: data=%x\n",data); 
	cpu_set_nmi_line(ALVG_CPUNO, PULSE_LINE);
}

/* U8 - 6522 */

//PA0-7 (IN) - N/C
static READ_HANDLER( xvia_1_a_r ) { logerror("WARNING: U8-PA-R\n"); return 0; }

//PB0-7 (IN)
/*
PB7        = N/C
PB6        = N/C
PB5  (I?)  = DMD Data
PB4  (O?)  = DMD Clock
PB3  (O?)  = DMD Enable?
PB2        = N/C
PB1        = N/C
PB0        = N/C*/
static READ_HANDLER( xvia_1_b_r ) { 
	//logerror("%x:U8-PB-R\n",activecpu_get_previouspc());
	return alvglocals.via_1_b; 
}
//CA1: (IN) - Sound Control
static READ_HANDLER( xvia_1_ca1_r ) { return 0; }//logerror("SOUND CONTROL: U8-CA1-R\n"); return 0; }
//CB1: (IN) - N.C.
static READ_HANDLER( xvia_1_cb1_r ) { logerror("WARNING: N.C.: U8-CB1-R\n"); return 0; }
//CA2:  (IN) - N.C.
static READ_HANDLER( xvia_1_ca2_r ) { logerror("WARNING: N.C.: U8-CA2-R\n"); return 0; }
//CB2: (IN) - N.C.
static READ_HANDLER( xvia_1_cb2_r ) { logerror("WARNING: U8-CB2-R\n"); return 0; }

//PA0-7: (OUT) - Sound Data
static WRITE_HANDLER( xvia_1_a_w ) { 
	alvglocals.sound_data = data;
	//logerror("SOUND DATA: U8-A-W: data=%x\n",data); 
}

//PB0-7: (OUT)
/*
PB7        = N/C
PB6        = N/C
PB5  (I?)  = DMD Data
PB4  (O?)  = DMD Clock
PB3  (O?)  = DMD Enable?
PB2        = N/C
PB1  (Out) = Sound Clock
PB0        = N/C
*/
static WRITE_HANDLER( xvia_1_b_w ) { 
	//logerror("U8-B-W: data=%x\n",data);
	alvglocals.via_1_b = data;

	//On clock transition - write to sound latch
	if(!alvglocals.sound_strobe && (data & 0x02))
	{
		if(alvglocals.sound_data)
			alvg_sndCmd_w(0,alvglocals.sound_data);
	}
	alvglocals.sound_strobe = data&0x02;

	//if(alvglocals.sound_strobe) logerror("SOUND CLOCK!\n");
	//else logerror("SOUND CLOCK RESET\n");
}
//CA2: (OUT) - CPU DIAG LED
static WRITE_HANDLER( xvia_1_ca2_w ) { 	alvglocals.diagnosticLed = data; }
//CB2: (OUT) - N.C.
static WRITE_HANDLER( xvia_1_cb2_w ) { logerror("WARNING: N.C.: U8-CB2-W: data=%x\n",data); }

//IRQ:  IRQ to Main CPU
static void via_irq(int state) {
	//logerror("IN VIA_IRQ - STATE = %x\n",state);
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
PB0-PB7 (out)  = Switch Strobe 1-8
PC0-PC7 (out)  = Switch Strobe 9-12	(bits 4-7 nc)
*/
WRITE_HANDLER(u13_porta_w) { solenoid_w(3,data); }
WRITE_HANDLER(u13_portb_w) { } //logerror("SWITCH_STROBE(1-8): data = %x\n",data^0xff); }
WRITE_HANDLER(u13_portc_w) { } //logerror("SWITCH_STROBE(9-12): data = %x\n",data^0xff); }
/*U14 - 8255*/
/*
PA0-PA7 (out) = Lamp Strobe 1-7  (bits 7 nc)
PB0-PB7 (out) = Lamp Strobe 8-12 (bits 5-7 nc)
PC0-PC7 (out) = Lamp Return 1-8
*/
WRITE_HANDLER(u14_porta_w) {} //logerror("LAMP STROBE(1-7):  data = %x\n",data^0xff); }
WRITE_HANDLER(u14_portb_w) {}//logerror("LAMP STROBE(8-12): data = %x\n",data^0xff); }
WRITE_HANDLER(u14_portc_w) {}//logerror("LAMP RETURN: data = %x\n",data^0xff); }







/*
U7 - 6522

PA0-7(In) = Switch Column 1-8 Return?
PB7  (In?) = DMD ACK
PB6  (Out) = NAND Gate -> WD (WatchDog)
PB5  (In)  = Test Switch - Cabinet Switch
PB4  (In)  = Enter Switch - Cabinet Switch
PB3  (In)  = AVAI1 - Cabinet Switch
PB2  (In)  = AVAI2 - Cabinet Switch
PB1  (out) = NAND Gate -> WD (WatchDog)
PB0        = N/C
CA1        = N/C
CA2        = N/C
CB1        = N/C
CB2  (Out) = 65C02 NMI
IRQ        = 65C02 IRQ

U8 - 6522

PA0-7(O)   = Sound Data
PB7        = N/C
PB6        = N/C
PB5  (I?)  = DMD Data
PB4  (O?)  = DMD Clock
PB3  (O?)  = DMD Enable?
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
PB0-PB7 (out)  = Switch Strobe 1-8
PC0-PC7 (out)  = Switch Strobe 9-12	(bits 4-7 nc)

U14 - 8255

PA0-PA7 (out) = Lamp Strobe 1-7  (bits 7 nc)
PB0-PB7 (out) = Lamp Strobe 8-12 (bits 5-7 nc)
PC0-PC7 (out) = Lamp Return 1-8
*/

static ppi8255_interface ppi8255_intf =
{
	3, 												/* 3 chips */
	{0, 0, 0},										/* Port A read */
	{0, 0, 0},										/* Port B read */
	{0, 0, 0},										/* Port C read */
	{u12_porta_w, u13_porta_w, u14_porta_w},		/* Port A write */
	{u12_portb_w, u14_portb_w, u14_portb_w},		/* Port B write */
	{u12_portc_w, u13_portc_w, u14_portc_w},		/* Port C write */
};


static INTERRUPT_GEN(alvg_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  alvglocals.vblankCount += 1;

  /*-- lamps --*/
  if ((alvglocals.vblankCount % ALVG_LAMPSMOOTH) == 0) {
	memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
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
	//coreGlobals.diagnosticLed = alvglocals.diagnosticLed;
    //alvglocals.diagnosticLed = 0;	//For some reason, LED won't work with this line in
	coreGlobals.diagnosticLed = (alvglocals.diagnosticLeds2<<2) |
								(alvglocals.diagnosticLeds1<<1) |
								alvglocals.diagnosticLed;
	//alvglocals.diagnosticLed = 0;	//For some reason, LED won't work with this line in
	alvglocals.diagnosticLeds1 = 0;
	alvglocals.diagnosticLeds2 = 0;
  }
  core_updateSw(alvglocals.solenoids & 0x80000000);
}

static SWITCH_UPDATE(alvg) {
	static int sndcmd=0;

//  via_irq(1);
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[ALVG_COMINPORT] & 0x0f00)>>8;									//Column 0 Switches
	coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0xe0) | (inports[ALVG_COMINPORT] & 0x1f);		//Column 1 Switches
	coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & 0xfc) | ((inports[ALVG_COMINPORT] & 0x60)>>5);	//Column 2 Switches
  }
  alvglocals.swTest = (core_getSw(ALVG_SWTEST)>0?1:0);
  alvglocals.swEnter = (core_getSw(ALVG_SWENTER)>0?1:0);
  alvglocals.swVolUp = (core_getSw(ALVG_SWVOLUP)>0?1:0);
  alvglocals.swVolDn = (core_getSw(ALVG_SWVOLDN)>0?1:0);
}

WRITE_HANDLER(alvg_sndCmd_w)
{
	printf("SOUND COMMAND: %x\n",data);
	soundlatch_w(0,data);
	cpu_set_irq_line(ALVGS_CPUNO, 0, PULSE_LINE);
	//cpu_set_irq_line(ALVGS_CPUNO, 0, HOLD_LINE);
//	sndbrd_0_data_w(0, data^0xff);
}

static int alvg_sw2m(int no) {
  if (no % 10 > 7) return -1;
  return (no / 10 + 1) * 8 + (no % 10);
}

static int alvg_m2sw(int col, int row) {
  return (col - 1) * 10 + row;
}

/*Alpha Numeric First Generation Init*/
static MACHINE_INIT(alvg) {
  memset(&alvglocals, 0, sizeof(alvglocals));
  
  /* init VIA */
  via_config(0, &via_0_interface);
  via_config(1, &via_1_interface);
  via_reset();

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /* Init the sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard, ALVGS_CPUNO, memory_region(ALVGS_ROMREGION), NULL, NULL);

}

static MACHINE_STOP(alvg) {
  sndbrd_0_exit();
}
//Show Sound Diagnostic LEDS
void alvg_UpdateSoundLEDS(int num,int data)
{
	if(num==0)
		alvglocals.diagnosticLeds1 = data;
	else
		alvglocals.diagnosticLeds2 = data;
}

static WRITE_HANDLER(DMD_LATCH) {} //logerror("DMD_LATCH: data=%x\n",data); }

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(alvg) {
  core_nvram(file, read_or_write, memory_region(ALVG_MEMREG_CPU), 0x2000, 0x00);
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
{0x3800,0x380f,via_1_r},
{0x3c00,0x3c0f,via_0_r},
{0x4000,0xffff,MRA_ROM},
MEMORY_END

static MEMORY_WRITE_START(alvg_writemem)
{0x0000,0x1fff,MWA_RAM},
{0x2000,0x2003,ppi8255_0_w},
{0x2400,0x2403,ppi8255_1_w},
{0x2800,0x2803,ppi8255_2_w},
{0x2c00,0x2fff,DMD_LATCH},
{0x3800,0x380f,via_1_w},
{0x3c00,0x3c0f,via_0_w},
{0x4000,0xffff,MWA_ROM},
MEMORY_END

MACHINE_DRIVER_START(alvg)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(alvg,NULL,alvg)
  MDRV_CPU_ADD(M65C02, 2000000)
  MDRV_CPU_MEMORY(alvg_readmem, alvg_writemem)
  MDRV_CPU_VBLANK_INT(alvg_vblank, ALVG_VBLANKFREQ)
  MDRV_NVRAM_HANDLER(alvg)

  MDRV_SWITCH_UPDATE(alvg)
  MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_CONV(alvg_sw2m,alvg_m2sw)
  MDRV_LAMP_CONV(alvg_sw2m,alvg_m2sw)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(alvgs1)
  MDRV_IMPORT_FROM(alvg)
  MDRV_IMPORT_FROM(alvgs)
  MDRV_SOUND_CMD(alvg_sndCmd_w)
  MDRV_SOUND_CMDHEADING("alvg")
MACHINE_DRIVER_END


