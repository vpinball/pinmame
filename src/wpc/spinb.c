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
  Audio: MSM6586 (Upgraded 5205) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin
  
  Page 2 - Music Section
  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz or 6Mhz/2 = 3Mhz
  Interrupt: Not used!
  I/O: PIA 8255
  Roms: IC30 (8192 Bytes), IC25, IC26, IC27 (524288 Bytes)
  Ram: 6116 - 2048 Bytes Hooked to the PIA
  Audio: MSM6586 (Upgraded 5205) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin

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
  #4) Digital Volume Adjustment not implemented - see schematics for DS1666 chip for more details
  #5) After entering sound commander, game does strange things..
  
	  
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
		(P5)    - S1 Pin on MSM6858 (Sample Rate Select 1)
		(P6)    - Reset on MSM6858
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
		(P5)    - S1 Pin on MSM6858 (Sample Rate Select 1)
		(P6)    - Reset on MSM6858
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

#define SPINB_Z80CPU_FREQ   2500000
#define SPINB_8051CPU_FREQ 16000000

//IRQ 500, NMI = 1500 wasn't bad, but still can't find the right timing

#define SPINB_VBLANKFREQ      60 /* VBLANK frequency*/
#define SPINB_INTFREQ        250 /* Z80 Interrupt frequency*/
#define SPINB_NMIFREQ        2270 /* Z80 NMI frequency (shouldn't be used according to schematics!?) */

#define MATRIX_STROBE 0
#define DIPCOL_STROBE 1

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
READ_HANDLER(SPINB_S1_MSM5025_READROM);
READ_HANDLER(SPINB_S2_MSM5025_READROM);
WRITE_HANDLER(SPINB_S1_MSM5025_w);
WRITE_HANDLER(SPINB_S2_MSM5025_w);
static void spinb_z80int(int data);
static void spinb_z80nmi(int data);

//Convenience macros - we should probably put these in the core.h
#define GET_BIT0 (data & 0x01) >> 0
#define GET_BIT1 (data & 0x02) >> 1
#define GET_BIT2 (data & 0x04) >> 2
#define GET_BIT3 (data & 0x08) >> 3
#define GET_BIT4 (data & 0x10) >> 4
#define GET_BIT5 (data & 0x20) >> 5
#define GET_BIT6 (data & 0x40) >> 6
#define GET_BIT7 (data & 0x80) >> 7

#if DMD_FROM_RAM
	static UINT8  *dmd32RAM;
#else
	static UINT8 dmd32RAM[64][16];
	static UINT8 dmd32TMP[64][16];
#endif

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT16 lampColumn, swColumn;
  int    DipCol;
  int    lampRow, swCol;
  int    diagnosticLed;
  int    ssEn;
  int    mainIrq;
  int    DMDBusy;
  int    DMDStat0;
  int    DMDStat1;
  int    DMDData;
  int    DMDPort2;
  int    DMDRamEnabled;
  int    DMDRom1Enabled;
  int    DMDA16Enabled;
  int    DMDA17Enabled;
  int    DMDPage;
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
  int    LastStrobeType;
  mame_timer *irqtimer;
  mame_timer *nmitimer;
  int irqfreq;
  int nmifreq;
} SPINBlocals;

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
/* MSM6585 ADPCM CHIP INTERFACE */
static struct MSM5205interface SPINB_msm6585Int = {
	2,										//# of chips
	640000,									//640Khz Clock Frequency
	{SPINB_S1_msmIrq, SPINB_S2_msmIrq},		//VCLK Int. Callback
	{MSM5205_S48_4B, MSM5205_S48_4B},		//Sample Mode
	{80,50}									//Volume
};
/* Sound board */
const struct sndbrdIntf spinbIntf = {
   "SPINB", NULL, NULL, NULL, spinb_sndCmd_w, NULL, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

//Convert Bit Column Data to corresponding #, ie, if Bit 3=1, return 3 - Zero Based (Bit1=1 returns 0)
//Probably should be moved to core.c since it could be used in many drivers.
static int BitColToNum(int tmp)
{
	int i, data;
	i = data = 0;
	while(tmp)
	{
		if(tmp&1) data+=i;
		tmp = tmp>>1;
		i++;
	}
	return data;
}

/* -- Manual IRQ & NMI Timing Adjustments for debugging -- */
#ifdef MAME_DEBUG
static void adjust_timer(int which, int offset) {
  static char s[8];
  if (which) { // 0 = NMI, others = IRQ
    SPINBlocals.irqfreq += offset;
    if (SPINBlocals.irqfreq < 1) SPINBlocals.irqfreq = 1;
    sprintf(s, "IRQ:%4d", SPINBlocals.irqfreq);
    core_textOut(s, 8, 40, 0, 5);
    timer_adjust(SPINBlocals.irqtimer, 1.0/(double)SPINBlocals.irqfreq, 0, 1.0/(double)SPINBlocals.irqfreq);
  } else {
    SPINBlocals.nmifreq += offset;
    if (SPINBlocals.nmifreq < 1) SPINBlocals.nmifreq = 1;
    sprintf(s, "NMI:%4d", SPINBlocals.nmifreq);
    core_textOut(s, 8, 40, 10, 5);
    timer_adjust(SPINBlocals.nmitimer, 1.0/(double)SPINBlocals.nmifreq, 0, 1.0/(double)SPINBlocals.nmifreq);
  }
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
	//if(data > 0)
	//printf("dmdrow=%02x,r4,5:%02x%02x  - Serial = %x\n",SPINBlocals.DMDRow,r5,r4,data);
	//dmd32RAM[SPINBlocals.DMDRow][SPINBlocals.DMDCol]=data;
	dmd32TMP[SPINBlocals.DMDRow][SPINBlocals.DMDCol]=data;
	SPINBlocals.DMDCol = (SPINBlocals.DMDCol+1) % 16;
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
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
			break;
		case 1:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
            break;
		case 2:
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
            break;
		case 3: // solenoid #25 activates flippers, rest are "fixed" lamps.
			coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFEFFFFFF) | ((data & 0x80)<<17);
			coreGlobals.tmpLampMatrix[8] = data & 0x7f;
			break;
		case 4:
			coreGlobals.tmpLampMatrix[10] = data;
			break;
		case 5:
			coreGlobals.tmpLampMatrix[9] = data;
			break;
		default:
			LOG(("Solenoid_W Logic Error\n"));
	}
}

READ_HANDLER(ci20_porta_r) { LOG(("UNDOCUMENTED: ci20_porta_r\n")); return 0; }
READ_HANDLER(ci20_portb_r) { LOG(("UNDOCUMENTED: ci20_portb_r\n")); return 0; }

//Switch Returns
READ_HANDLER(ci20_portc_r) {
	//Switch Column Strobe
	if(SPINBlocals.LastStrobeType == MATRIX_STROBE)
		return coreGlobals.swMatrix[SPINBlocals.swCol+1];	//+1 so we begin by reading column 1 of input matrix instead of 0 which is used for special switches in many drivers
	else {
	//Dip Column Strobe
 		int dip = core_getDip(SPINBlocals.DipCol);
		return (dip&0xf0) | (core_revnyb(dip&0x0f) & 0x0f);	//Lower nibble is reversed
	}
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
	data |= !(SPINBlocals.DMDBusy)<<2;
	data |= !(SPINBlocals.TestContactos)<<3;
	data |= (SPINBlocals.SoundReady)<<4;
//	LOG(("ci23 = %04x\n",data));
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
	if(data & 0x07) {
		SPINBlocals.LastStrobeType = DIPCOL_STROBE;
		SPINBlocals.DipCol = BitColToNum(data & 0x07);
	}
	else {
		SPINBlocals.LastStrobeType = MATRIX_STROBE;
		SPINBlocals.swColumn = (SPINBlocals.swColumn&0xff00) | (data ^ 0x07) >> 3;
		SPINBlocals.swCol = BitColToNum(SPINBlocals.swColumn);
	}
}
/*
CI-20 8255 PPI
--------------
  Port B:
  (out) P0-P3: J2 - Pins 1 - 4 (Marked Nivel C08-C11) - Switch Strobe (4 lines)
  (xxx) P4-P7: Not Used?
*/
WRITE_HANDLER(ci20_portb_w) {
	SPINBlocals.LastStrobeType = MATRIX_STROBE;
	SPINBlocals.swColumn = (SPINBlocals.swColumn&0x00ff) | (data<<5);
	SPINBlocals.swCol = BitColToNum(SPINBlocals.swColumn);
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
WRITE_HANDLER(ci23_porta_w) { coreGlobals.tmpLampMatrix[SPINBlocals.lampColumn] = data; }

/*
CI-23 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J3 - Pins 11 - 19 (no 18) (Nivel Luces 0-7) - Lamp Column Strobe 0 - 7
*/
WRITE_HANDLER(ci23_portb_w) { SPINBlocals.lampColumn = BitColToNum(data); }

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

/*
CI-22 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J5 - Pins 7 - 14 (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_porta_w) { solenoid_w(3,data); }
/*
CI-22 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J4 - Pins 5 - 12 (Marked Various) - Flasher Control 5-12
*/
WRITE_HANDLER(ci22_portb_w) { solenoid_w(5,data); }
/*
CI-22 8255 PPI
--------------
  Port C:
  (out) P0-P3 : J4 - Pins 1 - 4 (Marked Various) - Flasher Control 1-4
  (out) P4-P7 : J5 - Pins 15-19 (no 16) (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_portc_w) { solenoid_w(4,data); }

/*
CI-21 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J6 - Pins 1 - 8 (Marked Various) - Solenoids 1-8
*/
WRITE_HANDLER(ci21_porta_w) { solenoid_w(0,~data); }

/*
CI-21 8255 PPI
--------------
  Port B:
  (out) P0-P5 : J5 - Pins 1 - 6 (Marked Various) - Solenoids 19-24?
  (out) P6-P7 : J6 - Pins 18-19 (Marked Various) - Solenoids 17-18?
*/
WRITE_HANDLER(ci21_portb_w) { solenoid_w(2,data); }

/*
CI-21 8255 PPI
--------------
  Port C:
  (out) P0-P7 : J6 - Pins 9 - 17 (no 15) (Marked Various) - Solenoids 9-16
*/
WRITE_HANDLER(ci21_portc_w) { solenoid_w(1,~data); }

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
		(P5)    - S1 Pin on MSM6858 (Sample Rate Select 1)
		(P6)    - Reset on MSM6858
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
		int data = SPINB_S1_MSM5025_READROM(0);
		SPINB_S1_MSM5025_w(0,data);
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
		(P5)    - S1 Pin on MSM6858 (Sample Rate Select 1)
		(P6)    - Reset on MSM6858
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
		int data = SPINB_S2_MSM5025_READROM(0);
		SPINB_S2_MSM5025_w(0,data);
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
	SPINBlocals.DMDBusy = 0;
	//Trigger an interrupt on INT0
	cpu_set_irq_line(1, I8051_INT0_LINE, ASSERT_LINE);
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
//	memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  /*-- solenoids --*/
  coreGlobals.solenoids = SPINBlocals.solenoids;
  if ((SPINBlocals.vblankCount % SPINB_SOLSMOOTH) == 0) {
	if (SPINBlocals.ssEn) {
	  int ii;
	  coreGlobals.solenoids |= CORE_SOLBIT(CORE_SSFLIPENSOL);
	  /*-- special solenoids updated based on switches --*/
	  for (ii = 0; ii < 6; ii++)
		if (core_gameData->sxx.ssSw[ii] && core_getSw(core_gameData->sxx.ssSw[ii]))
		  coreGlobals.solenoids |= CORE_SOLBIT(CORE_FIRSTSSSOL+ii);
	}
	SPINBlocals.solenoids = coreGlobals.pulsedSolState;
  }
  core_updateSw(core_getSol(25));
}

static SWITCH_UPDATE(spinb) {
#ifdef MAME_DEBUG
  int which = 0;
  if(!debugger_focus) {
    if (keyboard_pressed(KEYCODE_SPACE)) which = 1; // toggle NMI/IRQ
    if      (keyboard_pressed_memory_repeat(KEYCODE_O, 60))
      adjust_timer(which, -10);
    else if (keyboard_pressed_memory_repeat(KEYCODE_L, 60))
      adjust_timer(which, -1);
    else if (keyboard_pressed_memory_repeat(KEYCODE_COLON, 60))
      adjust_timer(which, 1);
    else if (keyboard_pressed_memory_repeat(KEYCODE_P, 60))
      adjust_timer(which, 10);
  }
#endif /* MAME_DEBUG */
  if (inports) {
	  coreGlobals.swMatrix[0] = (inports[SPINB_COMINPORT] & 0x0100)>>8;  //Column 0 Switches
	  coreGlobals.swMatrix[1] = (inports[SPINB_COMINPORT] & 0x00ff);     //Column 1 Switches
  }
  SPINBlocals.TestContactos = (core_getSw(-7)>0?1:0);
}

//Send a sound command to the sound board
WRITE_HANDLER(spinb_sndCmd_w) {
	soundbd_w(0,data);
}

static int spinb_sw2m(int no) {
	return no + 7;
}

static int spinb_m2sw(int col, int row) {
	return col*8 + row - 9;
}

static void spinb_z80int(int data) {  cpu_set_irq_line(0, 0, PULSE_LINE); }

static void spinb_z80nmi(int data) {  cpu_set_irq_line(0, 127, PULSE_LINE); }

/*Machine Init*/
static MACHINE_INIT(spinb) {
  memset(&SPINBlocals, 0, sizeof(SPINBlocals));

  SPINBlocals.irqtimer = timer_alloc(spinb_z80int);
  SPINBlocals.irqfreq = SPINB_INTFREQ;
  timer_adjust(SPINBlocals.irqtimer, 1.0/(double)SPINBlocals.irqfreq, 0, 1.0/(double)SPINBlocals.irqfreq);
  SPINBlocals.nmitimer = timer_alloc(spinb_z80nmi);
  SPINBlocals.nmifreq = SPINB_NMIFREQ;
  timer_adjust(SPINBlocals.nmitimer, 1.0/(double)SPINBlocals.nmifreq, 0, 1.0/(double)SPINBlocals.nmifreq);

  /* init PPI */
  ppi8255_init(&ppi8255_intf);

  /* Setup DMD External Address Callback*/
  i8051_set_eram_iaddr_callback(dmd_eram_address);
  /* Setup DMD Serial Port Callback */
  i8051_set_serial_tx_callback(dmd_serial_callback);

  /* Init the dmd & sound board */
  sndbrd_0_init(core_gameData->hw.soundBoard,   2, memory_region(SPINB_MEMREG_SND1),NULL,NULL);
}

static MACHINE_RESET(spinb) {
  //Is always off by 1 if we don't correct it here!
  SPINBlocals.DMDRow = 64-33;
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

static WRITE_HANDLER(i8031_port_write)
{
	switch(offset) {
		//Used for external addressing...
		case 0:
		//Port 2 Used for external addressing, but we need to capture it here for access to external ram
		case 2:
			SPINBlocals.DMDPort2 = data;
			break;
		/*PORT 1:
			P1.0    (O) = Active Low Chip Enable for RAM (must be 0 to access ram)
			P1.1    (O) = DESP (Data Enable?) - Goes to 0 while not sending serial data, 1 otherwise
			P1.2    (O) = RDATA  (Set to 1 when beginning @ row 0)
			P1.3    (O) = ROWCK  (Clocked after 16 bytes sent (ie, 1 row) : 0->1 transition)
			P1.4    (O) = COLATCH (Clocked after 1 row sent also) 1->0 transition
			P1.5    (O) = Active Low - Clears INT0? & Enables data in from main cpu
			P1.6    (O) = STAT0
			P1.7    (O) = STAT1 */
		case 1:
			SPINBlocals.DMDRamEnabled = !(data & 1);
			if(!(GET_BIT5)) {
				//Clear it
				cpu_set_irq_line(1, I8051_INT0_LINE, CLEAR_LINE);
				SPINBlocals.DMDBusy = 1;	//Busy = 1 when no INT
				//LOG(("Clearing INT and Reading Data\n"));
			}
			SPINBlocals.DMDStat0 = GET_BIT6;
			SPINBlocals.DMDStat1 = GET_BIT7;

			{
			static int de=0;
			static int rd=0;
			static int rc=0;
			static int cl=0;
			//Data Enable
			if(de != GET_BIT1) {
				de = GET_BIT1;
				//printf("DE = %x\n",de);
			}
			//Row Data
			if(rd != GET_BIT2) {
				rd = GET_BIT2;
				//printf("RD = %x\n",rd);
			}
			//Row Clock
			if(rc != GET_BIT3) {
				rc = GET_BIT3;
				//printf("RC = %x\n",rc);
				if(GET_BIT3) 
				{
#if DMD_FROM_RAM
#else
					SPINBlocals.DMDRow = (SPINBlocals.DMDRow+1) % 64;
					if(SPINBlocals.DMDRow == 0)
						memcpy(dmd32RAM,dmd32TMP,sizeof(dmd32TMP));
#endif					
				}
			}
			//Column Latch
			if(cl != GET_BIT4) {
				cl = GET_BIT4;
				//printf("CL = %x\n",cl);
			}
			}

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
WRITE_HANDLER(digvol_w) { LOGSND(("digvol_w = %x\n",data)); }

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

static READ_HANDLER(SPINB_S1_MSM5025_READROM)
{
	int addr, data;
	addr = (SPINBlocals.S1_CS2<<20) | (SPINBlocals.S1_CS1<<19) | 
		   (SPINBlocals.S1_A18<<18) | (SPINBlocals.S1_A17<<17) | 
		   (SPINBlocals.S1_A16<<16) | (SPINBlocals.S1_AHI<<8) | 
		   (SPINBlocals.S1_ALO);
	data = (UINT8)*(memory_region(REGION_USER1) + addr);
	return data;
}

static READ_HANDLER(SPINB_S2_MSM5025_READROM)
{
	int addr, data;
	addr = (SPINBlocals.S2_CS2<<20) | (SPINBlocals.S2_CS1<<19) | 
		   (SPINBlocals.S2_A18<<18) | (SPINBlocals.S2_A17<<17) | 
		   (SPINBlocals.S2_A16<<16) | (SPINBlocals.S2_AHI<<8)  | 
		   (SPINBlocals.S2_ALO);
	data = (UINT8)*(memory_region(REGION_USER2) + addr);
	return data;
}


static WRITE_HANDLER(SPINB_S1_MSM5025_w) {
  SPINBlocals.S1_MSMDATA = data;
}
static WRITE_HANDLER(SPINB_S2_MSM5025_w) {
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
{0x4000,0x5fff,MWA_RAM, &generic_nvram, &generic_nvram_size},
{0x6000,0x6003,ppi8255_0_w},
{0x6400,0x6403,ppi8255_1_w},
{0x6800,0x6803,ppi8255_2_w},
{0x6c00,0x6c03,ppi8255_3_w},
{0x6c20,0x6c20,soundbd_w},
{0x6c60,0x6c60,dmdbd_w},
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
MACHINE_DRIVER_START(spinbs)
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
  MDRV_CPU_ADD(Z80, SPINB_Z80CPU_FREQ)
  MDRV_CPU_MEMORY(spinb_readmem, spinb_writemem)
  MDRV_CPU_VBLANK_INT(spinb_vblank, SPINB_VBLANKFREQ)

  MDRV_SWITCH_UPDATE(spinb)
  MDRV_SWITCH_CONV(spinb_sw2m,spinb_m2sw)
MACHINE_DRIVER_END

//Main CPU, DMD, Sound hardware Driver
MACHINE_DRIVER_START(spinbs1)
  MDRV_IMPORT_FROM(spinb)
  MDRV_IMPORT_FROM(spinbdmd)
  MDRV_IMPORT_FROM(spinbs)
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

PINMAME_VIDEO_UPDATE(SPINBdmd_update) {
  UINT8 *RAM  = ((UINT8 *)dmd32RAM);
  UINT8 *RAM2;
  tDMDDot dotCol;
  int ii,jj;
  RAM2 = RAM;
  RAM2 = RAM+0x200;
  
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
#endif