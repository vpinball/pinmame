/************************************************************************************************
  Spinball
  -----------------
  by Steve Ellenoff (07/26/2004)

  Hardware from 1995-1996

  Main CPU Board:

  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz
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
  Page 1 - ??
  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz?
  Interrupt: Hooked to a Timer 555
  I/O: PIA 8255
  Roms: IC9 (8192 Bytes), IC15, IC16, IC17 (524288 Bytes)
  Ram: 6116 - 2048 Bytes Hooked to the PIA
  Audio: MSM6586 (Upgraded 5205) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin
  (But sounds good at 11025Hz in Goldwave)

  Page 2 - ??
  CPU: Z80
  Clock: 5Mhz/2 = 2.5Mhz?
  Interrupt: Not used?!
  I/O: PIA 8255
  Roms: IC30 (8192 Bytes), IC25, IC26, IC27 (524288 Bytes)
  Ram: 6116 - 2048 Bytes Hooked to the PIA
  Audio: MSM6586 (Upgraded 5205) - Toggles between 4Khz, 8Khz from PIA - PC5 Pin
  (But sounds good at 11025Hz in Goldwave)


  The following combination does something interesting: INT = 500, NMI = 2500, Z80=2Mhz, 8051=12Mhz*2
  The following combination does something interesting: INT = 500, NMI = 2500, Z80=3.6Mhz, 8051=12Mhz*2
  The following combination does something interesting: INT = 750, NMI = 1000, Z80=3.6Mhz, 8051=16Mhz*3 or
  The following combination does something interesting: INT = 750, NMI = 100, Z80=3.6Mhz, 8051=16Mhz*3

  To get past "Introduzca Bola" - set the 4 ball switches to 1 in column 7, switches 1 - 4 (keys: T+A,T+S,T+D,T+F)
  To begin a game, press start, then eventually remove all ball switches 1-4, and then activate
  switches 5,6,7,8, in order, 1 at a time, and you should register some stuff..
  To drain, reapply in order, ball switches 1,2,3,4.

  Issues:
  #1) Timing all guessed, and still far from good as a result:
      a) switch registering doesn't always seem to work (although it's pretty decent now)
	  b) game sometimes freezes up - especially at boot up
  #2) DMD Page flipping not implemented - can't determine how it does it.
  #3) Serial Port should transmit data, but it doesn't - could have used it to help with DMD perhaps
  #4) Sound not implemented
**************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8051/i8051.h"
#include "machine/8255ppi.h"
#include "core.h"
//#include "sndbrd.h"
#include "spinb.h"

//#define VERBOSE 1

#ifdef VERBOSE
//#define LOG(x)	logerror x
#define LOG(x)	printf x
#else
#define LOG(x)
#endif

#define SPINB_VBLANKFREQ      60 /* VBLANK frequency*/
#define SPINB_INTFREQ        600 /* Z80 Interrupt frequency*/
#define SPINB_NMIFREQ       1080 /* Z80 NMI frequency (shouldn't be used according to schematics!?) */

WRITE_HANDLER(spinb_sndCmd_w);

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
  int    sound_strobe;
  int    SoundReady;
  int    TestContactos;
  int    LastStrobeType;
  mame_timer *irqtimer;
  mame_timer *nmitimer;
  int irqfreq;
  int nmifreq;
} SPINBlocals;

#ifdef MAME_DEBUG
static void adjust_timer(int which, int offset) {
  static char s[4];
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
	int r5;
	r5 = i8051_internal_r(0x05);		//Controls the hi byte into RAM.
#if 0
	if(last_r5 != r5) {
		last_r5 = r5;
		printf("r5 = %x\n",r5);
	}
#endif

	//yucky code to determine the starting dmd page in ram for display
	if(r5 >= 6 && r5 <= 9) SPINBlocals.DMDPage = 0x600;
	if(r5 >= 10 && r5 <= 13) SPINBlocals.DMDPage = 0xa00;
	if(r5 >= 14 && r5 <= 17) SPINBlocals.DMDPage = 0xe00;
	if(r5 >= 18 && r5 <= 21) SPINBlocals.DMDPage = 0x1200;
	if(r5 >= 22 && r5 <= 25) SPINBlocals.DMDPage = 0x1600;
	if(r5 >= 26 && r5 <= 29) SPINBlocals.DMDPage = 0x1a00;
    if(r5 >= 30 && r5 <= 33) SPINBlocals.DMDPage = 0x1e00;

	//if(data > 0)
	//printf("r4,5:%02x%02x  - Serial = %x\n",r5,r4,data);
}


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
		case 3:
			coreGlobals.tmpLampMatrix[8] = data;
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

static void UpdateSwCol(void) {
	int i, tmp, data;
	i = data = 0;
	tmp = SPINBlocals.swColumn;
	while(tmp)
	{
		i++;
		if(tmp&1) data+=i;
		tmp = tmp>>1;
	}
	SPINBlocals.swCol = data;
//	printf("COL = %x SwColumn = %d\n",SPINBlocals.swColumn,data);
//	printf("SwColumn = %d\n",data);
}

static void UpdateLampCol(int col) {
	int i, tmp, lmpCol;
	i = lmpCol = 0;
	tmp = col;
	while(tmp)
	{
		if(tmp&1) lmpCol+=i;
		tmp = tmp>>1;
		i++;
	}
	SPINBlocals.lampColumn = lmpCol;
	//printf("COL = %x LampColumn = %d\n",SPINBlocals.lampColumn,col);
	//printf("LampColumn = %d\n",data);
}


READ_HANDLER(ci20_porta_r) { LOG(("UNDOCUMENTED: ci20_porta_r\n")); return 0; }
READ_HANDLER(ci20_portb_r) { LOG(("UNDOCUMENTED: ci20_portb_r\n")); return 0; }

//Switch Returns
READ_HANDLER(ci20_portc_r) {
	int data = 0;
	if(SPINBlocals.LastStrobeType == 0)
		return coreGlobals.swMatrix[SPINBlocals.swCol];
	else
	{
		if(SPINBlocals.DipCol & 1) return core_getDip(0); // Dip Bank #1
		if(SPINBlocals.DipCol & 2) return core_getDip(1); // Dip Bank #2
		if(SPINBlocals.DipCol & 4) return core_getDip(2); // Dip Bank #3
	}
	//LOG(("ci20_portc_r\n"));
	//printf("%4x: reading switches col: %x = %x\n",activecpu_get_pc(),SPINBlocals.swCol,data);
	return data;
}
READ_HANDLER(ci23_porta_r) { LOG(("UNDOCUMENTED: ci23_porta_r\n")); return 0; }
READ_HANDLER(ci23_portb_r) { LOG(("UNDOCUMENTED: ci23_portb_r\n")); return 0; }

/*
CI-23 8255 PPI
--------------
  Not marked as inputs on the schematics, but i'm guessing they are!
  Port C:
  (in) P0-P7 : J3 - Pins 3 - 10 (Marked Various)
        (P5-P7)Pins 3 - 5 : NC
		(P4)   Pin  6     : To Sound Board (Ready)
		(P0)   Pin  7     : To DMD Board (Stat0)
		(P1)   Pin  8     : To DMD Board (Stat1)
		(P2)   Pin  9     : To DMD Board (Busy)
		(P3)   Pin 10     : Test De Contactos
*/
READ_HANDLER(ci23_portc_r) {
	int data = 0;
	LOG(("UNDOCUMENTED: ci23_portc_r\n"));
		data |= (SPINBlocals.DMDStat0)<<0;
		data |= (SPINBlocals.DMDStat1)<<1;
		data |= !(SPINBlocals.DMDBusy)<<2;
		data |= !(SPINBlocals.TestContactos)<<3;
		data |= (SPINBlocals.SoundReady)<<4;
//		printf("ci23 = %04x\n",data);
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
	//LOG("ci20_porta_w = %x\n",data);
	if(data & 0x7) {
		SPINBlocals.DipCol = data & 0x7;
		SPINBlocals.LastStrobeType = 1;
		//printf("* * DIP STROBE * * \n");
		//LOG(("dip strobe = %x\n",data));
	}
	else {
		//LOG(("switch strobe 3-7 = %x\n",data));
		SPINBlocals.LastStrobeType = 0;
		SPINBlocals.swColumn = (SPINBlocals.swColumn&0xff00) | (data ^ 0x07) >> 3;
		UpdateSwCol();
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
	//LOG("ci20_portb_w = %x\n",data);
	//LOG(("switch strobe 8-11 = %x\n",data));
	SPINBlocals.LastStrobeType = 0;
	SPINBlocals.swColumn = (SPINBlocals.swColumn&0x00ff) | (data<<5);
	UpdateSwCol();
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
	//LOG("ci23_porta_w = %x\n",data);
	LOG(("Lamp Data = %x\n",data));
	coreGlobals.tmpLampMatrix[SPINBlocals.lampColumn] = data;
}

/*
CI-23 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J3 - Pins 11 - 19 (no 18) (Nivel Luces 0-7) - Lamp Column Strobe 0 - 7
*/
WRITE_HANDLER(ci23_portb_w) {
	//LOG("ci23_portb_w = %x\n",data);
	LOG(("Lamp Strobe = %x\n",data));
	UpdateLampCol(data);
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
	if(data & 0xe0) LOG(("CI23 - Not Connected = %x\n",data));
	if(data & 0x10) LOG(("Sound Board Ready = %x\n",data & 0x10));
	if(data & 0x03) LOG(("DMD Board Stat0/1 = %x\n",data & 0x03));
	if(data & 0x04) LOG(("DMD Board Busy = %x\n",data & 0x04));
	if(data & 0x08) LOG(("Test De Contactos = %x\n",data & 0x08));
}

/*
CI-22 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J5 - Pins 7 - 14 (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_porta_w) {
	//LOG("ci22_porta_w = %x\n",data);
	//LOG(("Fixed Lamps 7-14 = %x\n",data));
	solenoid_w(3,data);
}
/*
CI-22 8255 PPI
--------------
  Port B:
  (out) P0-P7 : J4 - Pins 5 - 12 (Marked Various) - Flasher Control 5-12
*/
WRITE_HANDLER(ci22_portb_w) {
	//LOG("ci22_portb_w = %x\n",data);
	//LOG(("Flasher Control 5-12 = %x\n",data));
	solenoid_w(5,data);
}
/*
CI-22 8255 PPI
--------------
  Port C:
  (out) P0-P3 : J4 - Pins 1 - 4 (Marked Various) - Flasher Control 1-4
  (out) P4-P7 : J5 - Pins 15-19 (no 16) (Marked Various) - Fixed Lamps Head & Playfield (??)
*/
WRITE_HANDLER(ci22_portc_w) {
	//LOG("ci22_portc_w = %x\n",data);
	solenoid_w(4,data);
#if 0
	if(data & 0x0f)
		LOG(("Flasher Control 1-4 = %x\n",data));
	else
		LOG(("Fixed Lamps = %x\n",data));
#endif
}

/*
CI-21 8255 PPI
--------------
  Port A:
  (out) P0-P7 : J6 - Pins 1 - 8 (Marked Various) - Solenoids 1-8
*/
WRITE_HANDLER(ci21_porta_w) {
	//LOG("ci21_porta_w = %x\n",data);
	//LOG(("solenoids 1-8 = %x\n",data));
	solenoid_w(0,~data);
}
/*
CI-21 8255 PPI
--------------
  Port B:
  (out) P0-P5 : J5 - Pins 1 - 6 (Marked Various) - Solenoids 19-24?
  (out) P6-P7 : J6 - Pins 18-19 (Marked Various) - Solenoids 17-18?
*/
WRITE_HANDLER(ci21_portb_w) {
	//LOG("ci21_portb_w = %x\n",data);
	//LOG(("solenoids 17-24 = %x\n",data));
	solenoid_w(2,data);
}
/*
CI-21 8255 PPI
--------------
  Port C:
  (out) P0-P7 : J6 - Pins 9 - 17 (no 15) (Marked Various) - Solenoids 9-16
*/
WRITE_HANDLER(ci21_portc_w) {
	//LOG("ci21_portc_w = %x\n",data);
	//LOG(("solenoids 9-16 = %x\n",data));
	solenoid_w(1,~data);
}

//Switch Strobe Info: - Switch Matrix marked as Nivel 03-09 (7 x 8 Matrix)

/*
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
*/

static ppi8255_interface ppi8255_intf =
{
	4, 																/* 4 chips */
	{ci20_porta_r, ci23_porta_r, ci22_porta_r, ci21_porta_r},		/* Port A read */
	{ci20_portb_r, ci23_portb_r, ci22_portb_r, ci21_portb_r},		/* Port B read */
	{ci20_portc_r, ci23_portc_r, ci22_portc_r, ci21_portc_r},		/* Port C read */
	{ci20_porta_w, ci23_porta_w, ci22_porta_w, ci21_porta_w},		/* Port A write */
	{ci20_portb_w, ci23_portb_w, ci22_portb_w, ci21_portb_w},		/* Port B write */
	{ci20_portc_w, ci23_portc_w, ci22_portc_w, ci21_portc_w},		/* Port C write */
};

static WRITE_HANDLER(soundbd_w)
{
	//LOG(("SOUND WRITE = %x\n",data));
	//printf("SOUND WRITE = %x\n",data);
}

static WRITE_HANDLER(dmdbd_w)
{
	//LOG(("DMD WRITE = %x\n",data));
	//printf("DMD WRITE = %x\n",data);
	SPINBlocals.DMDData = data;
	SPINBlocals.DMDBusy = 0;
	//Trigger an interrupt on INT0
	cpu_set_irq_line(1, I8051_INT0_LINE, ASSERT_LINE);
}

static INTERRUPT_GEN(spinb_vblank) {
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
  core_updateSw(TRUE);
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

#if 0
//Send a sound command to the sound board
WRITE_HANDLER(spinb_sndCmd_w) {
	sndbrd_1_data_w(0, data);
	sndbrd_1_ctrl_w(0, 0);
}
#endif

static int spinb_sw2m(int no) {
	return no + 7;
}

static int spinb_m2sw(int col, int row) {
	return col*8 + row - 9;
}

static void spinb_z80int(int data) {
  LOG(("z80int\n"));
  cpu_set_irq_line(0, 0, PULSE_LINE);
}

static void spinb_z80nmi(int data) {
  LOG(("z80nmi\n"));
  cpu_set_irq_line(0, 127, PULSE_LINE);
}

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

  i8051_set_serial_tx_callback(dmd_serial_callback);

  /* Init the dmd & sound board */
//  sndbrd_0_init(core_gameData->hw.display,    SPINBDMD_CPUNO, memory_region(SPINBDMD_ROMREGION),data_from_dmd,NULL);
//  sndbrd_1_init(core_gameData->hw.soundBoard, SPINBS_CPUNO,   memory_region(SPINBS_ROMREGION)  ,NULL,NULL);
}

static MACHINE_STOP(spinb) {
//  sndbrd_0_exit();
//  sndbrd_1_exit();
}

#if 0
//Send data to the DMD CPU
static WRITE_HANDLER(DMD_LATCH) {
	sndbrd_0_data_w(0,data);
	sndbrd_0_ctrl_w(0,0);
}
#endif

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
			P1.1    (O) = DESP (Data Enable?)
			P1.2    (O) = RDATA
			P1.3    (O) = ROWCK
			P1.4    (O) = COLATCH
			P1.5    (O) = Active Low - Clears INT0? & Enables data in from main cpu
			P1.6    (O) = STAT0
			P1.7    (O) = STAT1 */
		case 1:
			SPINBlocals.DMDRamEnabled = !(data & 1);
			//if(SPINBlocals.DMDRamEnabled) printf("RAM Enalbed\n"); else printf("RAM Disabled\n");
			if((data & 0x20)==0) {
				//Clear it
				cpu_set_irq_line(1, I8051_INT0_LINE, CLEAR_LINE);
				SPINBlocals.DMDBusy = 1;	//Busy = 1 when no INT
				//printf("Clearing INT and Reading Data\n");
			}
			SPINBlocals.DMDStat0 = (data & 0x40) >> 6;
			SPINBlocals.DMDStat1 = (data & 0x80) >> 7;
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
			SPINBlocals.DMDA16Enabled = (data & 0x08)>>3;
			SPINBlocals.DMDRom1Enabled = (((data & 0x10)>>4)==0);
			SPINBlocals.DMDA17Enabled = (data & 0x20)>>5;
			break;
		default:
			LOG(("writing to port %x data = %x\n",offset,data));
	}
}

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

static PORT_READ_START( spinb_readport )
PORT_END

static PORT_WRITE_START( spinb_writeport )
PORT_END

static UINT8  *dmd32RAM;

//Read the command from the main cpu
static READ_HANDLER(dmd_readcmd)
{
	//printf("Reading DMD Command: %x\n",SPINBlocals.DMDData);
	return SPINBlocals.DMDData;
}

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
	{ 0x010000, 0x011fff, MWA_RAM, &dmd32RAM },
	{ 0x012000, 0x031fff, MWA_ROM },
	{ 0x032000, 0x032000, MWA_NOP },
MEMORY_END

static PORT_READ_START( spinbdmd_readport )
	{ 0x00,0xff, i8031_port_read },
PORT_END

static PORT_WRITE_START( spinbdmd_writeport )
	{ 0x00,0xff, i8031_port_write },
PORT_END

MACHINE_DRIVER_START(spinbdmd)
  MDRV_CPU_ADD(I8051, 16000000*3)	/*16 Mhz*/
  MDRV_CPU_MEMORY(spinbdmd_readmem, spinbdmd_writemem)
  MDRV_CPU_PORTS(spinbdmd_readport, spinbdmd_writeport)
  //MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Main Machine Driver (Main CPU Only)
MACHINE_DRIVER_START(spinb)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(spinb,NULL,spinb)
  MDRV_NVRAM_HANDLER(generic_0fill)
  MDRV_CPU_ADD(Z80, 2500000)
  MDRV_CPU_MEMORY(spinb_readmem, spinb_writemem)
  MDRV_CPU_PORTS(spinb_readport,spinb_writeport)
  MDRV_CPU_VBLANK_INT(spinb_vblank, SPINB_VBLANKFREQ)

  MDRV_SWITCH_UPDATE(spinb)
  //MDRV_DIAGNOSTIC_LEDH(3)
  MDRV_SWITCH_CONV(spinb_sw2m,spinb_m2sw)

  MDRV_TIMER_ADD(spinb_z80int, SPINB_INTFREQ)
  MDRV_TIMER_ADD(spinb_z80nmi, SPINB_NMIFREQ)
MACHINE_DRIVER_END

//Main CPU, DMD, Sound hardware Driver
MACHINE_DRIVER_START(spinbs1)
  MDRV_IMPORT_FROM(spinb)
  MDRV_IMPORT_FROM(spinbdmd)
//  MDRV_IMPORT_FROM(spinbs)
//  MDRV_SOUND_CMD(spinb_sndCmd_w)
//  MDRV_SOUND_CMDHEADING("spinb")
MACHINE_DRIVER_END

PINMAME_VIDEO_UPDATE(SPINBdmd_update) {
#ifdef MAME_DEBUG
  static int offset = 0;
#endif
  UINT8 *RAM  = ((UINT8 *)dmd32RAM);
  UINT8 *RAM2;
  tDMDDot dotCol;
  int ii,jj;

  RAM = RAM + SPINBlocals.DMDPage;
  //RAM  = RAM + 0x600;	//Display may start here?
  RAM2 = RAM + 0x200;
  //RAM2 = RAM;

#ifdef MAME_DEBUG
  core_textOutf(50,20,1,"offset=%08x", offset);
  memset(&dotCol,0,sizeof(dotCol));

  if(!debugger_focus) {
  if(keyboard_pressed_memory_repeat(KEYCODE_Z,2))
	  offset+=0x0001;
  if(keyboard_pressed_memory_repeat(KEYCODE_X,2))
	  offset-=0x0001;
  if(keyboard_pressed_memory_repeat(KEYCODE_C,2))
	  offset=0;
  if(keyboard_pressed_memory_repeat(KEYCODE_V,2))
	  offset+=0x400;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
	  offset-=0x400;
  if(keyboard_pressed_memory_repeat(KEYCODE_N,2))
	  offset+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_M,2))
  {
	  offset-=0x100;
//	  dmd32_data_w(0,offset);
//	  dmd32_ctrl_w(0,0);
  }
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
