/************************************************************************************************
  Spinball
  -----------------
  by Steve Ellenoff (07/26/2004)
  Thanks to Gerrit for helping improve the lamps & solenoids.


  Hardware from 1995-1996

  Main CPU Board:

  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz?
  Interrupt: Tied to a Fixed? System Timer?
  I/O: PIA 8255 (x4)
  Roms: ROM0 (8192 Bytes), ROM1 (8192 Bytes)
  Ram: 8192 Bytes

  Display Board:
  CPU: 80C31
  Clock: 16Mhz
  Interrupt: INT0 (P3.2) to CI7 - Triggered by !ZCS & ZR/W lines
  I/O: None
  Roms: CI-4 (65536 Bytes), CI-11 (131,072 Bytes)
  Ram: 6264 (8192 Bytes)

  Sound Board: (2 Sections - Sound Effects & Music)
  Page 1 - Sound Effects Section
  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz or 6Mhz/2 = 3Mhz
  Interrupt: Not used!
  I/O: PIA 8255
  Roms: IC9 (8192 Bytes), IC15, IC16, IC17 (524288 Bytes)
  Ram: 6116 - 2048 Bytes Hooked to the PIA
  Audio: MSM6585 (5205 with higher clocking) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin

  Page 2 - Music Section
  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz or 6Mhz/2 = 3Mhz
  Interrupt: Not used!
  I/O: PIA 8255
  Roms: IC30 (8192 Bytes), IC25, IC26, IC27 (524288 Bytes)
  Ram: 6116 - 2048 Bytes Hooked to the PIA
  Audio: MSM6585 (5205 with higher clocking) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin

  To get past "Introduzca Bola" (Install Ball) - set the 4 ball switches to 1 in column 7, switches 1 - 4 (keys: T+A,T+S,T+D,T+F)
  To begin a game, press start, then open first ball switch (T+A) and put in switch #5 (T+G)
  To drain, reapply in order, ball switches 1,2,3,4.

  Issues:
  #1) Timing all guessed, and still far from good as a result:
      a) switch registering doesn't always seem to work (although it's pretty decent now)
      b) game sometimes is unresponsive - especially when it says Ball is Lost
      c) After a special is awarded - cpu totally freezes.
  #2) DMD Flicker or DMD Paging problems
      If we use serial port for data, flicker is kind of bad, but paging is fine.
      If we use ram for data, there's paging problems
  #3) Would love to hear samples of the real machine to see if the music/effects are playing
      at the proper speed.
  #4) After entering sound commander, game does strange things..

**************************************************************************************

PIA 8255 CHIPS - PIN DESCRIPTIONS

CI-20 8255 PPI
--------------
  Port A:
  (out) P0-P2: Dip Switch Strobe
  (out) P3-P7: J2 - Pins 6 - 10 (Marked Nivel C03-C07) - Switch Strobe (5 lines)

  Port B:
  (out) P0-P3: J2 - Pins 1 - 4 (Marked Nivel C08-C11) - Switch Strobe (4 lines)
  (xxx) P4-P7: Not Used?

  Port C:
  (in) P0-P7 : J2 - Pins 11-18 (Marked Bit 0 - 7) - Switch & Dip Returns

CI-21 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J6 - Pins 1 - 8 (Marked Various) - Solenoids 1-8

  Port B:
  (out) P0-P5 : J5 - Pins 1 - 6 (Marked Various) - Solenoids 19-24?
  (out) P6-P7 : J6 - Pins 18-19 (Marked Various) - Solenoids 17-18?

  Port C:
  (out) P0-P7 : J6 - Pins 9 - 17 (no 15) (Marked Various) - Solenoids 9-16

CI-22 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J5 - Pins 7 - 14 (Marked Various) - Fixed Lamps Head & Playfield (??)

  Port B:
  (out) P0-P7 : J4 - Pins 5 - 12 (Marked Various) - Flasher Control 5-12

  Port C:
  (out) P0-P3 : J4 - Pins 1 - 4 (Marked Various) - Flasher Control 1-4
  (out) P4-P7 : J5 - Pins 15-19 (no 16) (Marked Various) - Fixed Lamps Head & Playfield (??)

CI-23 8255 PPI
-------
  Port A:
  (out) P0-P1 : J3 - Pins 1 - 2 (Bit Luces 0-1) - Lamp Data 0-1
  (out) P2-P7 : J4 - Pins 13-19 (no 17) (Bit Luces 2-7) - Lamp Data 2-7
  Summary: P0-P7 - Lamp Data Bits 0-7

  Port B:
  (out) P0-P7 : J3 - Pins 11 - 19 (no 18) (Nivel Luces 0-7) - Lamp Column Strobe 0 - 7

  Port C:
  (out) P0-P7 : J3 - Pins 3 - 10 (Marked Various)
        (P0-P3)Pins 3 - 5 : NC
		(P4)   Pin  6     : To Sound Board (Ready)
		(P0)   Pin  7     : To DMD Board (Stat0)
		(P1)   Pin  8     : To DMD Board (Stat1)
		(P2)   Pin  9     : To DMD Board (Busy)
		(P3)   Pin 10     : Test De Contactos

SND CPU #1 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
    (IN)(P0)    - Detects nibble feeds to MSM6585
		(P1-P3) - Not Used?
		(P4)    - Ready Status to Main CPU
		(P5)    - S1 Pin on MSM6585 (Sample Rate Select 1)
		(P6)    - Reset on MSM6585
		(P7)    - Not Used?

SND CPU #2 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
		(IN)(P0)- Detects nibble feeds to MSM6585
		(P1-P3) - Not Used?
		(P4)    - Not Used?
		(P5)    - S1 Pin on MSM6585 (Sample Rate Select 1)
		(P6)    - Reset on MSM6585
		(P7)    - Not Used?

***************************************************************************************/

#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8051/i8051.h"
#include "machine/8255ppi.h"
#include "sound/msm5205.h"
#include "core.h"
#include "sndbrd.h"
#include "spinb.h"

//#define VERBOSE 1

#ifdef VERBOSE
#define LOG(x)	logerror x
#define LOGSND(x) printf x
//#define LOG(x)	printf x
#else
#define LOG(x)
#define LOGSND(x)
#endif

//Set to 1 to display DMD from RAM, otherwise, it uses Serial Port Data (more accurate, but flicker problem)
#define DMD_FROM_RAM 0

#define SPINB_Z80CPU_FREQ   5000000 /* should be  2500000 2.5 MHz, tweaked for playability */
#define SPINB_8051CPU_FREQ 24000000 /* should be 16000000  16 MHz, tweaked for playability */

#define SPINB_VBLANKFREQ      60 /* VBLANK frequency*/
#define SPINB_INTFREQ        210 /* (180 ?) Z80 Interrupt frequency (variable! according to schematics!) */
#define INTCYCLES             90 /* keep irq high for this many cycles */
#define SPINB_NMIFREQ       1440 /* Z80 NMI frequency (confirmed by Jolly Park schematics) (should probably be INTFRQ*8=2000) */

/* Declarations */
WRITE_HANDLER(spinb_sndCmd_w);
READ_HANDLER(ci20_porta_r);
READ_HANDLER(ci20_portb_r);
READ_HANDLER(ci20_portc_r);
READ_HANDLER(ci21_porta_r);
READ_HANDLER(ci21_portb_r);
READ_HANDLER(ci21_portc_r);
READ_HANDLER(ci22_porta_r);
READ_HANDLER(ci22_portb_r);
READ_HANDLER(ci22_portc_r);
READ_HANDLER(ci23_porta_r);
READ_HANDLER(ci23_portb_r);
READ_HANDLER(ci23_portc_r);
READ_HANDLER(snd1_porta_r);
READ_HANDLER(snd1_portb_r);
READ_HANDLER(snd1_portc_r);
READ_HANDLER(snd2_porta_r);
READ_HANDLER(snd2_portb_r);
READ_HANDLER(snd2_portc_r);
WRITE_HANDLER(ci20_porta_w);
WRITE_HANDLER(ci20_portb_w);
WRITE_HANDLER(ci20_portc_w);
WRITE_HANDLER(ci21_porta_w);
WRITE_HANDLER(ci21_portb_w);
WRITE_HANDLER(ci21_portc_w);
WRITE_HANDLER(ci22_porta_w);
WRITE_HANDLER(ci22_portb_w);
WRITE_HANDLER(ci22_portc_w);
WRITE_HANDLER(ci23_porta_w);
WRITE_HANDLER(ci23_portb_w);
WRITE_HANDLER(ci23_portc_w);
WRITE_HANDLER(snd1_porta_w);
WRITE_HANDLER(snd1_portb_w);
WRITE_HANDLER(snd1_portc_w);
WRITE_HANDLER(snd2_porta_w);
WRITE_HANDLER(snd2_portb_w);
WRITE_HANDLER(snd2_portc_w);
static void SPINB_S1_msmIrq(int data);
static void SPINB_S2_msmIrq(int data);
static READ_HANDLER(SPINB_S1_MSM5205_READROM);
static READ_HANDLER(SPINB_S2_MSM5205_READROM);
static WRITE_HANDLER(SPINB_S1_MSM5205_w);
static WRITE_HANDLER(SPINB_S2_MSM5205_w);
static void spinb_z80int(int data);

#if DMD_FROM_RAM
	static UINT8  *dmd32RAM;
#else
	// up to 3 frames, 32 rows, 128 columns
	static UINT8 dmd32RAM[3][32][128/8];
#endif

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT16 lampColumn;
  int    lampRow, swCol;
  int    diagnosticLed;
  int    ssEn;
  int    L16isGameOn;
  int    mainIrq;
  int    DMDReady;
  int    DMDStat0,DMDStat0pend;
  int    DMDStat1;
  int    DMDData;
  int    DMDPort2;
  int    DMDRamEnabled;
  int    DMDRom1Enabled;
  int    DMDA16Enabled;
  int    DMDA17Enabled;
  int    DMDPage;
  int    DMDFrame;
  int    DMDRow;
  int    DMDCol;
  int    S1_ALO;
  int    S1_AHI;
  int    S1_CS0;
  int    S1_CS1;
  int    S1_CS2;
  int    S1_A16;
  int    S1_A17;
  int    S1_A18;
  int    S1_PC0;
  int    S1_MSMDATA;
  int    S1_Reset;
  int    S2_ALO;
  int    S2_AHI;
  int    S2_CS0;
  int    S2_CS1;
  int    S2_CS2;
  int    S2_A16;
  int    S2_A17;
  int    S2_A18;
  int    S2_PC0;
  int    S2_MSMDATA;
  int    S2_Reset;
  int    SoundReady;
  int    SoundCmd;
  int    TestContactos;
  UINT8  volume;
  UINT8  solInv0,solInv1,solInv2;
  mame_timer *irqtimer;
  int    irqfreq;
  int    nonmi;
  int    dmdframes;
  UINT8  dmdP1;
} SPINBlocals;

// meaning of the DMD stat0/1 lines and a macro to evaluate them
enum DMDSTATUSCODE { IDLE=0, ERROR, UNKNOWN, BUSY };
#define DMDSTATUS (SPINBlocals.DMDStat1*2 + SPINBlocals.DMDStat0)

/* -------------------*/
/* --- Interfaces --- */
/* -------------------*/

/* I8255 CHIPS */
static ppi8255_interface ppi8255_intf =
{
	6, 																							/* 6 chips */
	{ci20_porta_r, ci23_porta_r, ci22_porta_r, ci21_porta_r, snd1_porta_r, snd2_porta_r},		/* Port A read */
	{ci20_portb_r, ci23_portb_r, ci22_portb_r, ci21_portb_r, snd1_portb_r, snd2_portb_r},		/* Port B read */
	{ci20_portc_r, ci23_portc_r, ci22_portc_r, ci21_portc_r, snd1_portc_r, snd2_portc_r},		/* Port C read */
	{ci20_porta_w, ci23_porta_w, ci22_porta_w, ci21_porta_w, snd1_porta_w, snd2_porta_w},		/* Port A write */
	{ci20_portb_w, ci23_portb_w, ci22_portb_w, ci21_portb_w, snd1_portb_w, snd2_portb_w},		/* Port B write */
	{ci20_portc_w, ci23_portc_w, ci22_portc_w, ci21_portc_w, snd1_portc_w, snd2_portc_w},		/* Port C write */
};
/* MSM5205 ADPCM CHIP INTERFACE */
static struct MSM5205interface SPINB_msm5205Int = {
	2,										//# of chips
	384000,									//384Khz Clock Frequency
	{SPINB_S1_msmIrq, SPINB_S2_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B, MSM5205_S48_4B},		//Sample Mode
	{100,75}								//Volume
};
/* MSM6585 ADPCM CHIP INTERFACE */
static struct MSM5205interface SPINB_msm6585Int = {
	2,										//# of chips
	640000,									//640Khz Clock Frequency
	{SPINB_S1_msmIrq, SPINB_S2_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B, MSM5205_S48_4B},		//Sample Mode
	{100,75}								//Volume
};
/* Sound board */
const struct sndbrdIntf spinbIntf = {
   "SPINB", NULL, NULL, NULL, spinb_sndCmd_w, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

/* -- Manual IRQ Timing Adjustments for debugging -- */
#ifdef MAME_DEBUG
static void adjust_timer(int offset) {
  static char s[8];
  SPINBlocals.irqfreq += offset;
  if (SPINBlocals.irqfreq < 1) SPINBlocals.irqfreq = 1;
  sprintf(s, "IRQ:%4d", SPINBlocals.irqfreq);
  core_textOut(s, 8, 40, 0, 5);
  timer_adjust(SPINBlocals.irqtimer, 1.0/(double)SPINBlocals.irqfreq, 0, 1.0/(double)SPINBlocals.irqfreq);
}
#endif /* MAME_DEBUG */

//Total hack to determine where in RAM the 8031 is reading data to send via the serial port to the DMD Display
void dmd_serial_callback(int data)
{
#if DMD_FROM_RAM
	int r4,r5;
	r4 = i8051_internal_r(0x04);		//Controls the lo byte into RAM.
	r5 = i8051_internal_r(0x05);		//Controls the hi byte into RAM.
	//yucky code to determine the starting dmd page in ram for display
	if(r5 >= 6 && r5 <= 9) SPINBlocals.DMDPage = 0x600;
	if(r5 >= 10 && r5 <= 13) SPINBlocals.DMDPage = 0xa00;
	if(r5 >= 14 && r5 <= 17) SPINBlocals.DMDPage = 0xe00;
	if(r5 >= 18 && r5 <= 21) SPINBlocals.DMDPage = 0x1200;
	if(r5 >= 22 && r5 <= 25) SPINBlocals.DMDPage = 0x1600;
	if(r5 >= 26 && r5 <= 29) SPINBlocals.DMDPage = 0x1a00;
    if(r5 >= 30 && r5 <= 33) SPINBlocals.DMDPage = 0x1e00;
#else
  if (    (SPINBlocals.DMDFrame >= SPINBlocals.dmdframes )
       || (SPINBlocals.DMDRow   >= 32)
       || (SPINBlocals.DMDCol   >= 16)) {
    logerror("bad DMD position, frame=%d, row=%d, col=%d", SPINBlocals.DMDFrame,
                                                           SPINBlocals.DMDRow,
                                                           SPINBlocals.DMDCol);
  }
  else {
    dmd32RAM [SPINBlocals.DMDFrame] [SPINBlocals.DMDRow] [SPINBlocals.DMDCol] = data;
    SPINBlocals.DMDCol++;
  }
#endif
}

/* -------------------------------- */
/* DMD External Hardware Addressing */
/* -------------------------------- */
//Here we must generate the appropriate 32 bit address based on the hardware configuration for
//external ram access..
//Because we had to map everything in linear address space in our memory map, we must translate the
//hardware control pins to generate the proper address for the cpu to pull from memory properly
//
//There are 2 enable bits, RAM & ROM1 respectively.. if BOTH are disabled, data on the bus
//is coming from the Main CPU
//
//To generate an address, the offset passed is either: 8 bit (MOVX @R0/R1) or 16 bit (MOVX @DPTR)
//for 8 Bit: offset = bits 0-7, Port 2 is used for bits 8-15, and we track A16, A17, and add it.
//for 16 Bit: offset = bits 0-15, and we track A16, A17, and add it.
//NOTE: A16/17 ONLY Apply to ROM1 as those address pins are not wired to RAM.
//
//Then we must translate the address to the appropriate space in the map!
//The 8051 CPU will automatically add 0x10000 to the address we return because this is external addressing!
//
//RAM does not need to translate since it begins at 0x10000.
//ROM must translate to begin @ 0x12000 (so we add 0x2000)
//DMD Commands translate to a fixed address of 0x32000 (so we set it to 0x22000)
READ32_HANDLER(dmd_eram_address)
{
	UINT32 addr = offset;

	//DMD Command?
	if(!SPINBlocals.DMDRamEnabled && !SPINBlocals.DMDRom1Enabled)
		addr = 0x22000;		//Fixed location we use
	else
	{
		//If 8 bit offset, add port2 as upper 8 bits.
		if(mem_mask < 0x100)
			addr = offset | (SPINBlocals.DMDPort2<<8);

		//Accessing ROM1? Translate to begin @ 0x12000 and add A16! (Note: A17 not connected)
		if(SPINBlocals.DMDRom1Enabled) {
			addr |= (SPINBlocals.DMDA16Enabled<<16);
			addr += 0x2000;
		}
	}
	return addr;
}

/*Solenoids - Need to verify correct solenoid # here!*/
static WRITE_HANDLER(solenoid_w)
{
	switch(offset){
		case 0:
			SPINBlocals.solenoids |=
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			break;
		case 1:
			SPINBlocals.solenoids |=
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
			break;
		case 2:
			SPINBlocals.solenoids |=
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
			break;
		default:
			LOG(("Solenoid_W Logic Error\n"));
	}
}

READ_HANDLER(ci20_porta_r) { LOG(("UNDOCUMENTED: ci20_porta_r\n")); return 0; }
READ_HANDLER(ci20_portb_r) { LOG(("UNDOCUMENTED: ci20_portb_r\n")); return 0; }

//Switch Returns
READ_HANDLER(ci20_portc_r) {
	UINT8 data = 0;
	if (SPINBlocals.swCol > 2)	//Switch Column Strobe
		data = coreGlobals.swMatrix[SPINBlocals.swCol-2];	//so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
	else	//Dip Column Strobe
 		data = core_getDip(SPINBlocals.swCol);

	return data; //(data & 0xf0) | (core_revnyb(data & 0x0f));	//Lower nibble is reversed according to schematic, but not IRL!
}
READ_HANDLER(ci23_porta_r) { LOG(("UNDOCUMENTED: ci23_porta_r\n")); return 0; }
READ_HANDLER(ci23_portb_r) { LOG(("UNDOCUMENTED: ci23_portb_r\n")); return 0; }

/*
CI-23 8255 PPI
--------------
  Not marked as inputs on the schematics, but i'm guessing they are!
  Port C:
  (in) P0-P7 : J3 - Pins 3 - 10 (Marked Various)
		(P0)   Pin  7     : To DMD Board (Stat0)
		(P1)   Pin  8     : To DMD Board (Stat1)
		(P2)   Pin  9     : To DMD Board (Busy)
		(P3)   Pin 10     : Test De Contactos
		(P4)   Pin  6     : To Sound Board (Ready)
        (P5-P7)Pins 3 - 5 : NC
*/


READ_HANDLER(ci23_portc_r) {
	int data = 0;
	data |= (SPINBlocals.DMDStat0)<<0;
	data |= (SPINBlocals.DMDStat1)<<1;
	data |= !(SPINBlocals.DMDReady)<<2;
	data |= !(SPINBlocals.TestContactos)<<3;
	data |= (SPINBlocals.SoundReady)<<4;
//	LOG(("ci23 = %04x\n",data));

	// let the DMD get some work done
	if ((SPINBlocals.DMDReady==0) || (DMDSTATUS==BUSY)) {
		activecpu_abort_timeslice();
	}
	return data;
}

READ_HANDLER(ci22_porta_r) { LOG(("UNDOCUMENTED: ci22_porta_r\n")); return 0; }
READ_HANDLER(ci22_portb_r) { LOG(("UNDOCUMENTED: ci22_portb_r\n")); return 0; }
READ_HANDLER(ci22_portc_r) { LOG(("UNDOCUMENTED: ci22_portc_r\n")); return 0; }

READ_HANDLER(ci21_porta_r) { LOG(("UNDOCUMENTED: ci21_porta_r\n")); return 0; }
READ_HANDLER(ci21_portb_r) { LOG(("UNDOCUMENTED: ci21_portb_r\n")); return 0; }
READ_HANDLER(ci21_portc_r) { LOG(("UNDOCUMENTED: ci21_portc_r\n")); return 0; }

/*
CI-20 8255 PPI
--------------
  Port A:
(out) P0-P2: Dip Switch Strobe
(out) P3-P7: J2 - Pins 6 - 10 (Marked Nivel C03-C07) - Switch Strobe (5 lines)
*/
WRITE_HANDLER(ci20_porta_w) {
	SPINBlocals.swCol = core_BitColToNum(data);
}
/*
CI-20 8255 PPI
--------------
  Port B:
  (out) P0-P3: J2 - Pins 1 - 4 (Marked Nivel C08-C11) - Switch Strobe (4 lines)
  (xxx) P4-P7: Not Used?
*/
WRITE_HANDLER(ci20_portb_w) {
	if (data) SPINBlocals.swCol = 8 + core_BitColToNum(data & 0x0f);
}

WRITE_HANDLER(ci20_portc_w) { LOG(("UNDOCUMENTED: ci20_portc_w = %x\n",data)); }

/*
CI-23 8255 PPI
--------------
  Port A:
  (out) P0-P1 : J3 - Pins 1 - 2 (Bit Luces 0-1) - Lamp Data 0-1
  (out) P2-P7 : J4 - Pins 13-19 (no 17) (Bit Luces 2-7) - Lamp Data 2-7
  Summary: P0-P7 - Lamp Data Bits 0-7
*/
WRITE_HANDLER(ci23_porta_w) {
	if (SPINBlocals.nonmi)
		coreGlobals.tmpLampMatrix[4] |= data;
	else
		coreGlobals.tmpLampMatrix[SPINBlocals.lampColumn] |= data;
}
/*
CI-23 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J3 - Pins 11 - 19 (no 18) (Nivel Luces 0-7) - Lamp Column Strobe 0 - 7
*/
WRITE_HANDLER(ci23_portb_w) {
	if (SPINBlocals.nonmi)
		coreGlobals.tmpLampMatrix[5] |= data;
	else
		SPINBlocals.lampColumn = core_BitColToNum(data);
}
/*
CI-23 8255 PPI
--------------
  Port C:
  (out) P0-P7 : J3 - Pins 3 - 10 (Marked Various)
        (P5-P7)Pins 3 - 5 : NC
		(P4)   Pin  6     : To Sound Board (Ready)
		(P0)   Pin  7     : To DMD Board (Stat0)
		(P1)   Pin  8     : To DMD Board (Stat1)
		(P2)   Pin  9     : To DMD Board (Busy)
		(P3)   Pin 10     : Test De Contactos
*/
WRITE_HANDLER(ci23_portc_w) {
	//LOG("ci23_portc_w = %x\n",data);
#if 0
	if(data & 0xe0) LOG(("CI23 - Not Connected = %x\n",data));
	if(data & 0x10) LOG(("Sound Board Ready = %x\n",data & 0x10));
	if(data & 0x03) LOG(("DMD Board Stat0/1 = %x\n",data & 0x03));
	if(data & 0x04) LOG(("DMD Board Busy = %x\n",data & 0x04));
	if(data & 0x08) LOG(("Test De Contactos = %x\n",data & 0x08));
#endif
}

static WRITE_HANDLER(lamp_ex_w) {
	coreGlobals.tmpLampMatrix[6+offset] |= data;
}

/*
CI-22 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J5 - Pins 7 - 14 (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_porta_w) {
	if (SPINBlocals.L16isGameOn) {
		SPINBlocals.solenoids |=
		coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFEFFFFFF) | ((data & 0x80)<<17);
		data &= 0x7f;
	}
	if (SPINBlocals.nonmi)
		coreGlobals.tmpLampMatrix[1] |= data;
	else
		coreGlobals.tmpLampMatrix[8] |= data;
}
/*
CI-22 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J4 - Pins 5 - 12 (Marked Various) - Flasher Control 5-12
*/
WRITE_HANDLER(ci22_portb_w) {
	if (SPINBlocals.nonmi)
		coreGlobals.tmpLampMatrix[2] |= data;
	else
		coreGlobals.tmpLampMatrix[9] |= data;
}
/*
CI-22 8255 PPI
--------------
  Port C:
  (out) P0-P3 : J4 - Pins 1 - 4 (Marked Various) - Flasher Control 1-4
  (out) P4-P7 : J5 - Pins 15-19 (no 16) (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_portc_w) {
	if (SPINBlocals.nonmi)
		coreGlobals.tmpLampMatrix[3] |= data;
	else
		coreGlobals.tmpLampMatrix[10] |= data;
}
/*
CI-21 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J6 - Pins 1 - 8 (Marked Various) - Solenoids 1-8
*/

WRITE_HANDLER(ci21_porta_w) {
  if (data) solenoid_w(0, data ^ ~SPINBlocals.solInv0);
}

/*
CI-21 8255 PPI
--------------
  Port B:
  (out) P0-P5 : J5 - Pins 1 - 6 (Marked Various) - Solenoids 19-24?
  (out) P6-P7 : J6 - Pins 18-19 (Marked Various) - Solenoids 17-18?
*/
WRITE_HANDLER(ci21_portb_w) {
  if (SPINBlocals.nonmi)
    coreGlobals.tmpLampMatrix[0] |= data;
  else
    if (data) solenoid_w(2,data ^ ~SPINBlocals.solInv2);
}
/*
CI-21 8255 PPI
--------------
  Port C:
  (out) P0-P7 : J6 - Pins 9 - 17 (no 15) (Marked Various) - Solenoids 9-16
*/
WRITE_HANDLER(ci21_portc_w) {
  if (data) solenoid_w(1,data ^ ~SPINBlocals.solInv1);
}

/*
SND CPU #1 8255 PPI
-------------------
  Port C:
    (IN)(P0)    - Detects nibble feeds to MSM6585
*/
READ_HANDLER(snd1_porta_r) { LOGSND(("SND1_PORTA_R\n")); return 0; }
READ_HANDLER(snd1_portb_r) { LOGSND(("SND1_PORTB_R\n")); return 0; }
READ_HANDLER(snd1_portc_r) {
	int data = SPINBlocals.S1_PC0;
//	LOGSND(("SND1_PORTC_R = %x\n",data));
	return data;
}

/*
SND CPU #1 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
    (IN)(P0)    - Detects nibble feeds to MSM6585
		(P1-P3) - Not Used?
		(P4)    - Ready Status to Main CPU
		(P5)    - S1 Pin on MSM6585 (Sample Rate Select 1)
		(P6)    - Reset on MSM6585
		(P7)    - Not Used?
*/
WRITE_HANDLER(snd1_porta_w) { SPINBlocals.S1_AHI = data; }
WRITE_HANDLER(snd1_portb_w) { SPINBlocals.S1_ALO = data; }
WRITE_HANDLER(snd1_portc_w)
{
//	LOGSND(("SND1_PORTC_W = %02x\n",data));
	SPINBlocals.SoundReady = GET_BIT4;

	//Set Reset Line on the chip
	MSM5205_reset_w(0, GET_BIT6);

	//PC0 = 1 on Reset
	if(GET_BIT6)
		SPINBlocals.S1_PC0 = 1;
	else {
	//Read Data from ROM & Write Data To MSM Chip
		int msmdata = SPINB_S1_MSM5205_READROM(0);
		SPINB_S1_MSM5205_w(0,msmdata);
	}
	//Store reset value
	SPINBlocals.S1_Reset = GET_BIT6;
}


/*
SND CPU #2 8255 PPI
-------------------
  Port A:
  (out) Address 8-15 for DATA ROM		(manual showes this as Port B incorrectly!)

  Port B:
  (out) Address 0-7 for DATA ROM		(manual showes this as Port A incorrectly!)

  Port C:
  (out)
    (IN)(P0)    - Detects nibble feeds to MSM6585
		(P1-P3) - Not Used?
		(P4)    - Not Used?
		(P5)    - S1 Pin on MSM6585 (Sample Rate Select 1)
		(P6)    - Reset on MSM6585
		(P7)    - Not Used?
*/
READ_HANDLER(snd2_porta_r) { LOGSND(("SND2_PORTA_R\n")); return 0; }
READ_HANDLER(snd2_portb_r) { LOGSND(("SND2_PORTB_R\n")); return 0; }
READ_HANDLER(snd2_portc_r)
{
	int data = SPINBlocals.S2_PC0;
	//LOGSND(("SND2_PORTC_R = %x\n",data));
	return data;

}


WRITE_HANDLER(snd2_porta_w) { SPINBlocals.S2_AHI = data; }
WRITE_HANDLER(snd2_portb_w) { SPINBlocals.S2_ALO = data; }
WRITE_HANDLER(snd2_portc_w)
{
	//LOGSND(("SND2_PORTC_W = %02x\n",data));

	//Set Reset Line on the chip
	MSM5205_reset_w(1, GET_BIT6);

	//PC0 = 1 on Reset
	if(GET_BIT6)
		SPINBlocals.S2_PC0 = 1;
	else {
	//Read Data from ROM & Write Data To MSM Chip
		int msmdata = SPINB_S2_MSM5205_READROM(0);
		SPINB_S2_MSM5205_w(0,msmdata);
	}
	//Store reset value
	SPINBlocals.S2_Reset = GET_BIT6;
}

static WRITE_HANDLER(soundbd_w)
{
	LOG(("SOUND WRITE = %x\n",data));
	SPINBlocals.SoundCmd = data;
}

static WRITE_HANDLER(dmdbd_w)
{
	LOG(("DMD WRITE = %x\n",data));
	SPINBlocals.DMDData = data;
	SPINBlocals.DMDReady = 0;
	//Trigger an interrupt on INT0
	cpu_set_irq_line(SPINB_CPU_DMD, I8051_INT0_LINE, ASSERT_LINE);
	activecpu_abort_timeslice();
}

static INTERRUPT_GEN(spinb_vblank) {
//  spinb_z80int(0);

  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  SPINBlocals.vblankCount += 1;

  /*-- lamps --*/
  if ((SPINBlocals.vblankCount % SPINB_LAMPSMOOTH) == 0) {
	memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
	memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  if ((SPINBlocals.vblankCount % SPINB_SOLSMOOTH) == 0) {
	coreGlobals.solenoids = SPINBlocals.solenoids;
	SPINBlocals.ssEn = (coreGlobals.solenoids >> (SPINBlocals.L16isGameOn ? 25-1 : 5-1)) & 1;
	SPINBlocals.solenoids = coreGlobals.pulsedSolState;
  }
  core_updateSw(SPINBlocals.ssEn);
}

static SWITCH_UPDATE(spinb) {
#ifdef MAME_DEBUG
  if(!debugger_focus) {
    if      (keyboard_pressed_memory_repeat(KEYCODE_O, 60))
      adjust_timer(-10);
    else if (keyboard_pressed_memory_repeat(KEYCODE_L, 60))
      adjust_timer(-1);
    else if (keyboard_pressed_memory_repeat(KEYCODE_COLON, 60))
      adjust_timer(1);
    else if (keyboard_pressed_memory_repeat(KEYCODE_P, 60))
      adjust_timer(10);
  }
#endif /* MAME_DEBUG */
  if (inports) {
	  coreGlobals.swMatrix[0] = (inports[SPINB_COMINPORT] & 0x0100)>>8;  //Column 0 Switches
	  coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & 0x06) | (inports[SPINB_COMINPORT] & 0x00f9);     //Column 1 Switches
  }
  SPINBlocals.TestContactos = (core_getSw(SPINB_SWTEST)>0?1:0);
}

//Send a sound command to the sound board
WRITE_HANDLER(spinb_sndCmd_w) {
	soundbd_w(0,data);
}

// Unknown write address. Used on Bushido only.
static WRITE_HANDLER(X6ce0_w) {
  static int suppresslog=0;
  if (!suppresslog) {
    logerror("Unknown write to 0x6ce0\n");
    suppresslog=1;
  }
}

/*Switches are mapped in the manual as:
  30-37, 40-47, 50-57, 60-67, 70-77, 80-87, 90-97
  which must map to pinmame's internal numbering of 1-64.*/
static int spinb_sw2m(int no) {
  if (no < 0) return no + 8;
  else return (no%10) + ((( no / 10) - 3) * 8) + 8;
}

/*Col 1, Row 1 = 30... Col 2, Row 1 = 40, etc..*/
static int spinb_m2sw(int col, int row) {
	return col*10 + row + 20 - 1;
}

static void setirqline(int state) {
  cpu_set_irq_line(SPINB_CPU_GAME, 0, state);
}

static void spinb_z80int(int data) {
  setirqline(ASSERT_LINE);
  timer_set(TIME_IN_CYCLES(INTCYCLES,SPINB_CPU_GAME),CLEAR_LINE,setirqline);
}

static INTERRUPT_GEN(spinb_z80nmi) {  cpu_set_nmi_line(SPINB_CPU_GAME, PULSE_LINE); }

/*Machine Init*/
static MACHINE_INIT(spinb) {
  memset(&SPINBlocals, 0, sizeof(SPINBlocals));

  memset(dmd32RAM,0,sizeof(dmd32RAM));
  SPINBlocals.dmdframes = core_gameData->hw.display;

  SPINBlocals.solInv0     = (core_gameData->hw.gameSpecific1 >> 24) & 0xff;
  SPINBlocals.solInv1     = (core_gameData->hw.gameSpecific1 >> 16) & 0xff;
  SPINBlocals.solInv2     = (core_gameData->hw.gameSpecific1 >>  8) & 0xff;
  SPINBlocals.L16isGameOn =  core_gameData->hw.gameSpecific1        & 0xff;

  SPINBlocals.irqtimer = timer_alloc(spinb_z80int);
  SPINBlocals.irqfreq = SPINB_INTFREQ;
  timer_adjust(SPINBlocals.irqtimer, 1.0/(double)SPINBlocals.irqfreq, 0, 1.0/(double)SPINBlocals.irqfreq);

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /* Setup DMD External Address Callback*/
  i8051_set_eram_iaddr_callback(dmd_eram_address);
  /* Setup DMD Serial Port Callback */
  i8051_set_serial_tx_callback(dmd_serial_callback);

  /* Init the dmd & sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard,   2, memory_region(SPINB_MEMREG_SND1),NULL,NULL);
  SPINBlocals.nonmi=1;
}

static MACHINE_INIT(spinbnmi) {
  machine_init_spinb();
  SPINBlocals.nonmi=0;
}

static MACHINE_RESET(spinb) {
  SPINBlocals.volume = 122;
}

static MACHINE_STOP(spinb) {
  sndbrd_0_exit();
}

//Only the INT0 pin is configured for read access and we handle that in the dmd command handler
static READ_HANDLER(i8031_port_read)
{
	int data = 0;
	LOG(("%4x:port read @ %x data = %x\n",activecpu_get_pc(),offset,data));
	return data;
}

/*PORT 1:
    P1.0    (O) = Active Low Chip Enable for RAM (must be 0 to access ram)
    P1.1    (O) = DESP (Data Enable?) - Goes to 0 while not sending serial data, 1 otherwise
    P1.2    (O) = RDATA  (Set to 1 when beginning @ row 0)
    P1.3    (O) = ROWCK  (Clocked after 16 bytes sent (ie, 1 row) : 0->1 transition)
    P1.4    (O) = COLATCH (Clocked after 1 row sent also) 1->0 transition
    P1.5    (O) = Active Low - Clears INT0? & Enables data in from main cpu
    P1.6    (O) = STAT0
    P1.7    (O) = STAT1 */

enum P1BITS { RAMCE=0, DESP=1, RDATA=2, ROWCK=3, COLATCH=4, READY=5, STAT0=6, STAT1=7 };
#define GETBIT(value,bitno) (((value) >> (bitno)) & 1)

static void P1_update(int data)
{
  int chg = data ^ SPINBlocals.dmdP1;	// bits which have changed
  int whi = chg & data;					// bits which went high
  int wlo = chg & SPINBlocals.dmdP1;	// bits which went low

  if (SPINBlocals.DMDStat0pend) {
    SPINBlocals.DMDStat0 = GETBIT(data,STAT0);
    SPINBlocals.DMDStat0pend = FALSE;
  }

  if (chg)
  {
    // RAM Chip ~Enable
    if (GETBIT(chg,RAMCE)) {
      SPINBlocals.DMDRamEnabled = !GETBIT(data,RAMCE);
    }

    // Data Enable
    if (GETBIT(chg,DESP)) {
    }

    // Row Data
    if (GETBIT(whi,RDATA)) {
      SPINBlocals.DMDRow = 0;
      SPINBlocals.DMDCol = 0;
    }

    // Row Clock
    if (GETBIT(whi,ROWCK)) {
      SPINBlocals.DMDCol = 0;
      SPINBlocals.DMDRow++;
      if (SPINBlocals.DMDRow >= 32) {
        SPINBlocals.DMDRow = 0;
        SPINBlocals.DMDFrame++;
        if (SPINBlocals.DMDFrame >= SPINBlocals.dmdframes) {
          SPINBlocals.DMDFrame = 0;
        }
      }
    }

    // Column Latch
    if (GETBIT(chg,COLATCH)) {
    }

    // DMD Ready (to read next command)
    if (GETBIT(chg,READY)) {
      SPINBlocals.DMDReady = GETBIT(data,READY);
      if (GETBIT(wlo,READY)) {
        cpu_set_irq_line(SPINB_CPU_DMD, I8051_INT0_LINE, CLEAR_LINE);
      }
    }

    // DMD Status 0
    if (GETBIT(chg,STAT0)) {
      SPINBlocals.DMDStat0pend = TRUE;
    }

    // DMD Status 1
    if (GETBIT(chg,STAT1)) {
      SPINBlocals.DMDStat1 = GETBIT(data,STAT1);
    }
  }

#ifdef MAME_DEBUG
{
  static int prvstat;
  int newstat=DMDSTATUS;
  if (newstat!=prvstat && newstat==ERROR) logerror("DMD reports error\n");
  prvstat=newstat;
}
#endif

  SPINBlocals.dmdP1 = data;
}

static WRITE_HANDLER(i8031_port_write)
{
    switch(offset) {
        //Used for external addressing...
        case 0:
        // break ?

        //Port 2 Used for external addressing, but we need to capture it here for access to external ram
        case 2:
            SPINBlocals.DMDPort2 = data;
            break;

        case 1:
            P1_update(data);
            break;

        /*PORT 3:
            P3.0/RXD (O) = SDATA (Output?)
            P3.1/TXD (O) = DOTCK
            P3.2/INT0(I) = Command Ready from Main CPU
            P3.3/INT1(O) = A16 to ROM1
            P3.4/TO  (O) = ROM1 Chip Enable (Active Low)
            P3.5/T1  (O) = A17 to ROM1 (not connected)
            P3.6     (O) = /WR
            P3.7     (O) = /RD*/
        case 3:
            SPINBlocals.DMDA16Enabled = GET_BIT3;
            SPINBlocals.DMDRom1Enabled = !(GET_BIT4);
            SPINBlocals.DMDA17Enabled = GET_BIT5;
            break;

        default:
            LOG(("writing to port %x data = %x\n",offset,data));
    }
}

//Read the command from the main cpu
static READ_HANDLER(dmd_readcmd)
{
	//LOG(("Reading DMD Command: %x\n",SPINBlocals.DMDData));
	return SPINBlocals.DMDData;
}

//Read Command from Main CPU
READ_HANDLER(sndcmd_r) { return SPINBlocals.SoundCmd; }

//Send commands to Digital Volume
WRITE_HANDLER(digvol_w) {
	if (data && SPINBlocals.volume < 142) SPINBlocals.volume++;
	if (!data && SPINBlocals.volume > 0)  SPINBlocals.volume--;
	LOGSND(("digvol_w = %x; volume = %d\n", data, SPINBlocals.volume));
	MSM5205_set_volume(0, SPINBlocals.volume*100/142);
	MSM5205_set_volume(1, SPINBlocals.volume*100/142);
}

//Sound Control - CPU #1 & CPU #2 (identical)
//FROM MANUAL (AND TOTALLY WRONG!)
//Bit 0 = Chip Select Data Rom 1
//Bit 1 = Chip Select Data Rom 2
//Bit 2 = A16 Select  Data Roms
//Bit 3 = Chip Select Data Rom 3
//Bit 4 = A17 Select  Data Roms
//Bit 5 = NC
//Bit 6 = A18 Select  Data Roms
//Bit 7 = NC

//GUESSED FROM WATCHING EMULATION
//Bit 0 = A16 Select  Data Roms
//Bit 1 = A17 Select  Data Roms
//Bit 2 = A18 Select  Data Roms
//Bit 3 = NC
//Bit 4 = NC
//Bit 5 = Chip Select Data Rom 3
//Bit 6 = Chip Select Data Rom 2
//Bit 7 = Chip Select Data Rom 1

WRITE_HANDLER(sndctrl_1_w)
{
	SPINBlocals.S1_A16 = GET_BIT0;
	SPINBlocals.S1_A17 = GET_BIT1;
	SPINBlocals.S1_A18 = GET_BIT2;
	SPINBlocals.S1_CS0 = !(GET_BIT7);	//Active Low
	SPINBlocals.S1_CS1 = !(GET_BIT6);	//Active Low
	SPINBlocals.S1_CS2 = !(GET_BIT5);	//Active Low
}

WRITE_HANDLER(sndctrl_2_w)
{
	SPINBlocals.S2_A16 = GET_BIT0;
	SPINBlocals.S2_A17 = GET_BIT1;
	SPINBlocals.S2_A18 = GET_BIT2;
	SPINBlocals.S2_CS0 = !(GET_BIT7);	//Active Low
	SPINBlocals.S2_CS1 = !(GET_BIT6);	//Active Low
	SPINBlocals.S2_CS2 = !(GET_BIT5);	//Active Low
}

static READ_HANDLER(SPINB_S1_MSM5205_READROM)
{
	int addr, data;
	addr = (SPINBlocals.S1_CS2<<20) | (SPINBlocals.S1_CS1<<19) |
		   (SPINBlocals.S1_A18<<18) | (SPINBlocals.S1_A17<<17) |
		   (SPINBlocals.S1_A16<<16) | (SPINBlocals.S1_AHI<<8) |
		   (SPINBlocals.S1_ALO);
	data = (UINT8)*(memory_region(REGION_USER1) + addr);
	return data;
}

static READ_HANDLER(SPINB_S2_MSM5205_READROM)
{
	int addr, data;
	addr = (SPINBlocals.S2_CS2<<20) | (SPINBlocals.S2_CS1<<19) |
		   (SPINBlocals.S2_A18<<18) | (SPINBlocals.S2_A17<<17) |
		   (SPINBlocals.S2_A16<<16) | (SPINBlocals.S2_AHI<<8)  |
		   (SPINBlocals.S2_ALO);
	data = (UINT8)*(memory_region(REGION_USER2) + addr);
	return data;
}


static WRITE_HANDLER(SPINB_S1_MSM5205_w) {
  SPINBlocals.S1_MSMDATA = data;
}
static WRITE_HANDLER(SPINB_S2_MSM5205_w) {
  SPINBlocals.S2_MSMDATA = data;
}

/* MSM5205 interrupt callback */
static void SPINB_S1_msmIrq(int data) {
  //Write data
  if(!SPINBlocals.S1_Reset) {
	int mdata = SPINBlocals.S1_MSMDATA>>(4*SPINBlocals.S1_PC0);	//PC0 determines if lo or hi nibble is fed
	MSM5205_data_w(0, mdata&0x0f);
  }
  //Flip it..
  SPINBlocals.S1_PC0 = !SPINBlocals.S1_PC0;
}

/* MSM5205 interrupt callback */
static void SPINB_S2_msmIrq(int data) {
  //Write data
  if(!SPINBlocals.S2_Reset) {
	int mdata = SPINBlocals.S2_MSMDATA>>(4*SPINBlocals.S2_PC0);	//PC0 determines if lo or hi nibble is fed
	MSM5205_data_w(1, mdata&0x0f);
  }
  //Flip it..
  SPINBlocals.S2_PC0 = !SPINBlocals.S2_PC0;
}

/* ------------------- */
/* MAIN CPU MEMORY MAP */
/* ------------------- */
static MEMORY_READ_START(spinb_readmem)
{0x0000,0x3fff,MRA_ROM},
{0x4000,0x5fff,MRA_RAM},
{0x6000,0x6003,ppi8255_0_r},
{0x6400,0x6403,ppi8255_1_r},
{0x6800,0x6803,ppi8255_2_r},
{0x6c00,0x6c03,ppi8255_3_r},
MEMORY_END

static MEMORY_WRITE_START(spinb_writemem)
{0x0000,0x3fff,MWA_ROM},
{0x4000,0x43ff,MWA_RAM},
{0x4400,0x45ff,MWA_RAM, &generic_nvram, &generic_nvram_size},
{0x4600,0x5fff,MWA_RAM},
{0x6000,0x6003,ppi8255_0_w},
{0x6400,0x6403,ppi8255_1_w},
{0x6800,0x6803,ppi8255_2_w},
{0x6c00,0x6c03,ppi8255_3_w},
{0x6c20,0x6c20,soundbd_w},
{0x6c40,0x6c45,lamp_ex_w},
{0x6c60,0x6c60,dmdbd_w},
{0x6ce0,0x6ce0,X6ce0_w},
MEMORY_END

/* ------------------- */
/* DMD CPU MEMORY MAP  */
/* ------------------- */

//The MC51 cpu's can all access up to 64K ROM & 64K RAM in the SAME ADDRESS SPACE
//It uses separate commands to distinguish which area it's reading/writing!
//So to handle this, the cpu core automatically adjusts all external memory access to the follwing setup..
//00000 -  FFFF is used for MOVC(/PSEN=0) commands
//10000 - 1FFFF is used for MOVX(/RD=0 or /WR=0) commands
// We'll map as follows to make it all fit into linear address space:
//     10000- 11FFF (RAM 2K)
//     12000- 31FFF (ROM1 - 128K)
//     32000- 32000 (DATA CMD FROM CPU)
static MEMORY_READ_START(spinbdmd_readmem)
	{ 0x000000, 0x00ffff, MRA_ROM },
	{ 0x010000, 0x011fff, MRA_RAM },
	{ 0x012000, 0x031fff, MRA_ROM },
	{ 0x032000, 0x032000, dmd_readcmd },
MEMORY_END

static MEMORY_WRITE_START(spinbdmd_writemem)
	{ 0x000000, 0x00ffff, MWA_ROM },		//This area can never really be accessed by the cpu core but we'll put this here anyway
#if DMD_FROM_RAM
	{ 0x010000, 0x011fff, MWA_RAM, &dmd32RAM },
#else
	{ 0x010000, 0x011fff, MWA_RAM },
#endif
	{ 0x012000, 0x031fff, MWA_ROM },
	{ 0x032000, 0x032000, MWA_NOP },
MEMORY_END

static PORT_READ_START( spinbdmd_readport )
	{ 0x00,0xff, i8031_port_read },
PORT_END

static PORT_WRITE_START( spinbdmd_writeport )
	{ 0x00,0xff, i8031_port_write },
PORT_END


/* --------------------- */
/* SOUND CPU MEMORY MAP  */
/* --------------------- */

//CPU #1 - SOUND EFFECTS SAMPLES
static MEMORY_READ_START(spinbsnd1_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_r},
	{ 0x8000, 0x8000, sndcmd_r},
MEMORY_END
static MEMORY_WRITE_START(spinbsnd1_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x4003, ppi8255_4_w},
	{ 0x6000, 0x6000, sndctrl_1_w},
MEMORY_END

//CPU #2 - MUSIC SAMPLES
static MEMORY_READ_START(spinbsnd2_readmem)
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x2000, 0x3fff, MRA_RAM },
	{ 0x4000, 0x4003, ppi8255_5_r},
	{ 0x8000, 0x8000, sndcmd_r},
MEMORY_END
static MEMORY_WRITE_START(spinbsnd2_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x2000, 0x3fff, MWA_RAM },
	{ 0x4000, 0x4003, ppi8255_5_w},
	{ 0x6000, 0x6000, sndctrl_2_w},
	{ 0xa000, 0xa000, digvol_w},
MEMORY_END

/* DMD DRIVER */
MACHINE_DRIVER_START(spinbdmd)
  MDRV_CPU_ADD(I8051, SPINB_8051CPU_FREQ)	/*16 Mhz*/
  MDRV_CPU_MEMORY(spinbdmd_readmem, spinbdmd_writemem)
  MDRV_CPU_PORTS(spinbdmd_readport, spinbdmd_writeport)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

/* SOUND SECTION DRIVER */
static MACHINE_DRIVER_START(spinbs5205)
  MDRV_CPU_ADD(Z80, 6000000)		//Schem shows 5 or 6/2 = 2.5/3Mhz, but sound is distorted otherwise
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(spinbsnd1_readmem, spinbsnd1_writemem)
  MDRV_CPU_ADD(Z80, 6000000)		//Schem shows 6/2 = 2.5/3Mhz, but sound is distorted otherwise
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(spinbsnd2_readmem, spinbsnd2_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, SPINB_msm5205Int)
MACHINE_DRIVER_END

/* SOUND SECTION DRIVER */
static MACHINE_DRIVER_START(spinbs6585)
  MDRV_CPU_ADD(Z80, 6000000)		//Schem shows 5 or 6/2 = 2.5/3Mhz, but sound is distorted otherwise
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(spinbsnd1_readmem, spinbsnd1_writemem)
  MDRV_CPU_ADD(Z80, 6000000)		//Schem shows 6/2 = 2.5/3Mhz, but sound is distorted otherwise
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(spinbsnd2_readmem, spinbsnd2_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(MSM5205, SPINB_msm6585Int)
MACHINE_DRIVER_END

//Main Machine Driver (Main CPU Only)
MACHINE_DRIVER_START(spinb)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(spinb,spinb,spinb)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_CPU_ADD_TAG("mcpu", Z80, SPINB_Z80CPU_FREQ)
  MDRV_CPU_MEMORY(spinb_readmem, spinb_writemem)
  MDRV_CPU_VBLANK_INT(spinb_vblank, 1)
  MDRV_SWITCH_UPDATE(spinb)
  MDRV_SWITCH_CONV(spinb_sw2m,spinb_m2sw)
MACHINE_DRIVER_END

//Main CPU without NMI, DMD, Sound hardware Driver
MACHINE_DRIVER_START(spinbs1)
  MDRV_IMPORT_FROM(spinb)
  MDRV_IMPORT_FROM(spinbdmd)
  MDRV_IMPORT_FROM(spinbs5205)
  MDRV_SOUND_CMD(spinb_sndCmd_w)
  MDRV_SOUND_CMDHEADING("spinb")
MACHINE_DRIVER_END

//Main CPU with NMI, DMD, Sound hardware Driver
MACHINE_DRIVER_START(spinbs1n)
  MDRV_IMPORT_FROM(spinb)
  MDRV_CPU_MODIFY("mcpu")
  MDRV_CORE_INIT_RESET_STOP(spinbnmi,spinb,spinb)
  MDRV_CPU_PERIODIC_INT(spinb_z80nmi, SPINB_NMIFREQ)
  MDRV_IMPORT_FROM(spinbdmd)
  MDRV_IMPORT_FROM(spinbs6585)
  MDRV_SOUND_CMD(spinb_sndCmd_w)
  MDRV_SOUND_CMDHEADING("spinb")
MACHINE_DRIVER_END

/* -------------------- */
/* DMD DRAWING ROUTINES */
/* -------------------- */

//DRAW DMD FROM RAM OPTION

#if DMD_FROM_RAM
PINMAME_VIDEO_UPDATE(SPINBdmd_update) {
#ifdef MAME_DEBUG
  static int offset = 0;
#endif
  UINT8 *RAM  = ((UINT8 *)dmd32RAM);
  UINT8 *RAM2;
  tDMDDot dotCol;
  int ii,jj;

  RAM = RAM + SPINBlocals.DMDPage;
  RAM2 = RAM + 0x200;

#ifdef MAME_DEBUG
  core_textOutf(50,20,1,"offset=%08x", offset);
  memset(&dotCol,0,sizeof(dotCol));

  if(!debugger_focus) {
  if(keyboard_pressed_memory_repeat(KEYCODE_C,2))
	  offset=0;
  if(keyboard_pressed_memory_repeat(KEYCODE_V,2))
	  offset+=0x400;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
	  offset-=0x400;
  if(keyboard_pressed_memory_repeat(KEYCODE_N,2))
	  offset+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_M,2))
	  offset-=0x100;
  }
  RAM += offset;
  RAM2 += offset;
#endif

  for (ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (128/8); jj++) {
	  UINT8 intens1, intens2, dot1, dot2;
	  dot1 = core_revbyte(RAM[0]);
	  dot2 = core_revbyte(RAM2[0]);
	  intens1 = 2*(dot1 & 0x55) + (dot2 & 0x55);
      intens2 =   (dot1 & 0xaa) + (dot2 & 0xaa)/2;

      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      RAM += 1; RAM2 += 1;
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

#else

//DRAW DMD FROM SERIAL PORT DATA

// translate dot intensities
static int intens[3][4]= {
 {0,3,3,3},
 {0,1,3,3},
 {0,1,2,3}
};

PINMAME_VIDEO_UPDATE(SPINBdmd_update) {
  int     row,col,bit,dot;
  UINT8   *line;
  tDMDDot dotCol={{0}};
  UINT8   d1,d2,d3;

  for (row=0; row < 32; row++)
  {
    line = &dotCol[row+1][0];
    for (col=0; col < 16; col++)
    {
      d1=dmd32RAM[0][row][col];
      d2=dmd32RAM[1][row][col];
      d3=dmd32RAM[2][row][col];
      for(bit=0; bit < 8; bit++)
      {
        dot = (d1&1) + (d2&1) + (d3&1);
        dot = intens[SPINBlocals.dmdframes-1][dot];
        *line++ = dot;
        d1>>=1; d2>>=1; d3>>=1;
      }
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;

}
#endif
