/************************************************************************************************
  Capcom
  -----------------
  by Martin Adrian, Steve Ellenoff, Gerrit Volkenborn (05/07/2002 - 09/25/2003)

  Hardware from 1995-1996

  CPU BOARD:
	CPU: 68306 @ 16.67 Mhz
	I/O: 68306 has 2 8-bit ports and a dual uart
	DMD: Custom programmed U16 chip handles DMD Control Data and some timing 
	Size: 128x32 (all games except Flipper Football - 256x64!)

  SOUND BOARD:
	CPU: 87c52 @ 12 Mhz
	I/O: 87c52 has a uart
	SND: 2 x TMS320AV120 MPG DECODER ( Only 1 on Breakshot )

  Capcom Standard Pins:
  Lamp Matrix     = 2 x (8x8 Matrixs) = 128 Lamps - NOTE: No GI - every single lamp is cpu controlled
  Switch Matrix   = 64 Switches on switch board + 16 Cabinet Switches
  Solenoids	= 32 Solenoids/Flashers
  Sound: 2 Channel Mono Audio

  Capcom "Classic" Pins: (Breakshot)
  Lamp Matrix     = 1 x (8x8 Matrixs) = 64 Lamps - 64 GI Lamps (not directly cpu controlled?)
  Switch Matrix   = 1 x (8x7 Matrix)  = 56 Switches + 9 Cabinet Switches
  Solenoids	= 32 Solenoids/Flashers
  Sound: 1 Channel Mono Audio

  Milestones:
  09/19-09/21/03 - U16 Test successfully bypassed, dmd and lamps working quite well
  09/21-09/24/03 - DMD working 100% incuding 256x64 size, switches, solenoids working on most games except kp, and ff
  09/25          - First time game booted without any errors.. (Even though in reality, the U16 would still fail if not hacked around)
  Hacks & Issues that need to be looked into:
  #1) U16 Needs to be better understood and emulated
  #2) Once U16 better understood, remove hacks that bypass U16 startup check
  #3) IRQ4 appears to do nothing, this seems odd
  #4) 50V Line is being read as 0.0V for some reason, even though Signal Test passes (which involves the 50V Line check)
  #6) Sound communication occurs via the 68306 UARTS (currently not really emulated in the 68306 core)
  #7) Handle opto switches internally? Is this needed?
  #8) Handle EOS switches internally? Is this needed?
  #9) More complete M68306 emulation (although it's fairly good already)
  #10) Lamps will eventually come on in Kingpin/Flipper Football, why does it take so long? And they flicker too fast!

**************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "capcoms.h"

//Comment out to remove U16 Test bypass..
#define USE_U16_TEST_BYPASS

//Comment out to use the correct actual frequency values, but the animations are much too slow..
#define USE_ADJUSTED_FREQ		

#ifdef USE_ADJUSTED_FREQ
	#define CC_ZCFREQ       124			/* Zero cross frequency - Reports ~60Hz on the Solenoid/Line Voltage Test */
	#define CPU_CLOCK		24000000	/* Animation speed is more accurate at this speed, strange.. */
#else
	#define CC_ZCFREQ       87			/* Zero cross frequency - Reports ~60Hz on the Solenoid/Line Voltage Test */
	#define CPU_CLOCK		16670000	/* Animation speed is more accurate at this speed, strange.. */
#endif

#define CC_VBLANKFREQ    60 /* VBLANK frequency */
#define CC_SOLSMOOTH       3 /* Smooth the Solenoids over this numer of VBLANKS */
#define CC_LAMPSMOOTH      4 /* Smooth the lamps over this number of VBLANKS */

static struct {
  UINT32 solenoids;
  UINT16 u16a[4],u16b[4];
  UINT16 driverBoard;
  int vblankCount;
  int u16irqcount;
  int diagnosticLed;
  UINT8 visible_page;
  int zero_cross;
  int blanking;
  int swCol;
  int read_u16;
  UINT8 lastb;
  int vset;
  int line_v;
  int greset;
  int pulse;
} locals;

static UINT32 fixaddr = 0;

static NVRAM_HANDLER(cc);
static void U16_Tests_ByPass(void);

static INTERRUPT_GEN(cc_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % CC_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % CC_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
    coreGlobals.pulsedSolState = 0;
  }

  /*update leds*/
  coreGlobals.diagnosticLed = locals.diagnosticLed;
  locals.diagnosticLed = 0;

  core_updateSw(TRUE);
}

static SWITCH_UPDATE(cc) {
  if (inports) {
    CORE_SETKEYSW(inports[CORE_COREINPORT],0xcf,9);
    if (core_gameData->hw.lampCol)
      CORE_SETKEYSW(inports[CORE_COREINPORT]>>8,0xff,0);
    else
      CORE_SETKEYSW(((inports[CORE_COREINPORT]>>8)<<4),0xf0,2);
  }
}

/********************/
/* IRQ & ZERO CROSS */
/********************/
static void cc_zeroCross(int data) {
 //locals.line_v = !locals.line_v;
 locals.zero_cross = !locals.zero_cross;
 cpu_set_irq_line(0,MC68306_IRQ_2,ASSERT_LINE);
}
static void cc_u16irq1(int data) {
  locals.u16irqcount += 1;
  if (locals.u16irqcount == (0x08>>((locals.u16b[0] & 0xc0)>>6))) {
    cpu_set_irq_line(0,MC68306_IRQ_1,PULSE_LINE);
    locals.u16irqcount = 0;
  }
}
static void cc_u16irq4(int data) {
	cpu_set_irq_line(0,MC68306_IRQ_4,PULSE_LINE);
}

/***************/
/* PORT A READ */
/***************/
//PA0   - J3 - Pin 7 - Token/Coin Meter/Printer Interface Board(Unknown purpose)
//PA1   - J3 - Pin 8 - Token/Coin Meter/Printer Interface Board(Unknown purpose) 
//PA2   - J3 - Pin 9 - Token/Coin Meter/Printer Interface Board(Unknown purpose)
//PA3   - LED(output only)
//PA4   - LINE_5 (Measures +5V Low power D/C Line)
//PA5   - LINE_V (Measures 50V High Power A/C Line for Solenoids)
//PA6   - VSET(output only)
//PA7   - GRESET(output only)
static READ16_HANDLER(cc_porta_r) {
	int data = 0;
	locals.line_v = !locals.line_v;
	data = (1<<4) | (locals.line_v<<5);
	if(!locals.pulse)	data ^= 0x10;
	DBGLOG(("Port A read\n")); 
	//printf("[%08x] Port A read = %x\n",activecpu_get_previouspc(),data);
	return data; 
}
/***************/
/* PORT B READ */
/***************/
//PB0 OR IACK2 - ZERO X ACK(Output Only)
//PB1 OR IACK3 - PULSE (To J2) (Output Only)
//PB2 OR IACK5 - J3 - Pin 13 - Token/Coin Meter/Printer Interface Board - SW3(Unknown Purpose)(Output Only)
//PB3 OR IACK6 - J3 - Pin 12 - Token/Coin Meter/Printer Interface Board - SW4(Unknown Purpose)(Output Only)
//PB4 OR IRQ2  - ZERO CROSS IRQ
//PB5 OR IRQ3  - NOT USED?
//PB6 OR IRQ5  - J3 - Pin 11 - Token/Coin Meter/Printer Interface Board - SW6(Unknown Purpose)(Output Only)
//PB7 OR IRQ6  - NOT USED?
static READ16_HANDLER(cc_portb_r) { 
	int data = 0;
	data |= locals.zero_cross<<4;
	DBGLOG(("Port B read = %x\n",data));
	return data; 
}

/****************/
/* PORT A WRITE */
/****************/
//PA0   - J3 - Pin 7 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA1   - J3 - Pin 8 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA2   - J3 - Pin 9 - Token/Coin Meter/Printer Interface Board(Unknown purpose)(Input Only)
//PA3   - LED
//PA4   - LINE_5 (Input only)
//PA5   - LINE_V (Input only)
//PA6   - VSET   
//PA7   - GRESET (to soundboard - Inverted?)
static WRITE16_HANDLER(cc_porta_w) {
  if(data !=0x0048 && data !=0x0040 && data !=0x0008)
	DBGLOG(("Port A write %04x\n",data));

  locals.diagnosticLed = ((~data)&0x08>>3);
  locals.vset = (data>>6)&1;
  locals.greset = (data>>7)&1;

  if (!core_gameData->hw.lampCol)
	locals.driverBoard = 0x0700; // this value is expected by Breakshot after port writes
}
/****************/
/* PORT B WRITE */
/****************/
//PB0 OR IACK2 - ZERO CROSS ACK (Clears IRQ2)
//PB1 OR IACK3 - PULSE (To J2)
//PB2 OR IACK5 - J3 - Pin 13 - Token/Coin Meter/Printer Interface Board - SW3(Unknown Purpose)(Output Only)
//PB3 OR IACK6 - J3 - Pin 12 - Token/Coin Meter/Printer Interface Board - SW4(Unknown Purpose)(Output Only)
//PB4 OR IRQ2  - ZERO CROSS IRQ (Input Only)
//PB5 OR IRQ3  - NOT USED?
//PB6 OR IRQ5  - J3 - Pin 11 - Token/Coin Meter/Printer Interface Board - SW6(Unknown Purpose)
//PB7 OR IRQ6  - NOT USED?
static WRITE16_HANDLER(cc_portb_w) {
  locals.pulse = (data>>1)&1;
  DBGLOG(("Port B write %04x\n",data));
  if (data & ~locals.lastb & 0x01)
	  cpu_set_irq_line(0,MC68306_IRQ_2,CLEAR_LINE);
  locals.lastb = data;
  //if(locals.pulse) printf("pulse=1\n"); else printf("pulse=0\n");
}
/************/
/* U16 READ */
/************/
static READ16_HANDLER(u16_r) {
  offset &= 0x203;
  //DBGLOG(("U16r [%03x] (%04x)\n",offset,mem_mask));
  //printf("U16r [%03x] (%04x)\n",offset,mem_mask);

  switch (offset) {
    case 0x000: case 0x001: 
		return locals.u16a[offset];	

	//Should probably never occur?
	case 0x002: case 0x003:
		DBGLOG(("reading U16-%x\n",offset));
		return locals.u16a[offset];

    case 0x200: case 0x201: case 0x202: case 0x203:
      return locals.u16b[offset&3];
  }
  return 0;
}

/*************/
/* U16 WRITE */
/*************/
static WRITE16_HANDLER(u16_w) {
  offset &= 0x203;
   // DBGLOG(("U16w [%03x]=%04x (%04x)\n",offset,data,mem_mask));
  //printf("U16w [%03x]=%04x (%04x)\n",offset,data,mem_mask);

  //DMD Visible Block offset
  if (offset==2)
	locals.visible_page = (locals.visible_page & 0x0f) | (data << 4);
  //DMD Visible Page offset
  if (offset==3)
	locals.visible_page = (locals.visible_page & 0xf0) | (data >> 12);

  switch (offset) {
    case 0x000: case 0x001: case 0x002: case 0x003:
      locals.u16a[offset] = (locals.u16a[offset] & mem_mask) | data; break;
    case 0x200: case 0x201: case 0x202: case 0x203:
      locals.u16b[offset&3] = (locals.u16b[offset&3] & mem_mask) | data; break;
  }
}

/*************/
/* I/O READ  */
/*************/
static READ16_HANDLER(io_r) {
  UINT16 data = 0;
  static int swcol = 0;

  switch (offset) {
    //Read from driver board
    case 0x000008:
      DBGLOG(("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask));
      data = locals.driverBoard;
      break;
    //Read from other boards
    case 0x000006:
    case 0x00000a:
	  data=0xffff;
      DBGLOG(("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask));
	  //printf("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask);
      break;
    //Lamp A & B Matrix Row Status? Used to determine non-functioning bulbs?
    case 0x00000c:
    case 0x00000d:
      data = locals.blanking ? 0xffff : 0;
      break;
    //Playfield Switches
    case 0x200008:
    case 0x200009:
    case 0x20000a:
    case 0x20000b:
      swcol = offset-0x200007;
      data = (coreGlobals.swMatrix[swcol+4] << 8 | coreGlobals.swMatrix[swcol]) ^ 0xffff; //Switches are inverted
      break;

    //Solenoid A & B Status???
    case 0x20000c:
    case 0x20000d:
		data = 0;
		//printf("PC%08x - io_r: [%08x] (%04x) = %04x\n",activecpu_get_pc(),offset,mem_mask,data);
		break;

	//Cabinet/Coin Door Switches OR (ALL SWITCH READS FOR GAMES USING ONLY LAMP B MATRIX)
    case 0x400000:
      if (!core_gameData->hw.lampCol) {
        //Cabinet/Coin Door Switches are read as the lower byte on all switch reads
        data = coreGlobals.swMatrix[9];
        switch(locals.swCol) {
          case 0x80: data |= coreGlobals.swMatrix[1]<<8; break;
          case 0x40: data |= coreGlobals.swMatrix[2]<<8; break;
          case 0x20: data |= coreGlobals.swMatrix[3]<<8; break;
          case 0x10: data |= coreGlobals.swMatrix[4]<<8; break;
          case 0x08: data |= coreGlobals.swMatrix[5]<<8; break;
          case 0x04: data |= coreGlobals.swMatrix[6]<<8; break;
          case 0x02: data |= coreGlobals.swMatrix[7]<<8; break;
          case 0x01: data |= coreGlobals.swMatrix[8]<<8; break;
        }
      } else
        data = coreGlobals.swMatrix[0] << 8 | coreGlobals.swMatrix[9];
      data ^= 0xffff; //Switches are inverted
      break;

    default:
      DBGLOG(("PC%08x - io_r: [%08x] (%04x)\n",activecpu_get_pc(),offset,mem_mask));
  }
  return data;
}

/*************/
/* I/O WRITE */
/*************/
static WRITE16_HANDLER(io_w) {
  UINT16 soldata;
  //DBGLOG(("io_w: [%08x] (%04x) = %x\n",offset,mem_mask,data));

  switch (offset) {
    //Write to other boards
    case 0x00000006:
    case 0x00000007:
    case 0x0000000a:
    case 0x0000000b:
      DBGLOG(("PC%08x - io_w: [%08x] (%04x) = %x\n",activecpu_get_pc(),offset,mem_mask,data));
      break;
    //Blanking (for lamps & solenoids?)
    case 0x00000008:
      locals.blanking = (data & 0x0c)?1:0;
      break;
    //Lamp A Matrix OR Switch Strobe Column (Games with only Lamp B Matrix)
    case 0x0000000c:
      if (!core_gameData->hw.lampCol)
        locals.swCol = data;
      else if (!locals.blanking) {
        if (data & 0x0100) coreGlobals.tmpLampMatrix[0] |= (~data & 0xff);
        if (data & 0x0200) coreGlobals.tmpLampMatrix[1] |= (~data & 0xff);
        if (data & 0x0400) coreGlobals.tmpLampMatrix[2] |= (~data & 0xff);
        if (data & 0x0800) coreGlobals.tmpLampMatrix[3] |= (~data & 0xff);
        if (data & 0x1000) coreGlobals.tmpLampMatrix[4] |= (~data & 0xff);
        if (data & 0x2000) coreGlobals.tmpLampMatrix[5] |= (~data & 0xff);
        if (data & 0x4000) coreGlobals.tmpLampMatrix[6] |= (~data & 0xff);
        if (data & 0x8000) coreGlobals.tmpLampMatrix[7] |= (~data & 0xff);
      }
      break;
    //Lamp B Matrix (for games that only have lamp b, shift to lower half of our lamp matrix for easier numbering)
    case 0x0000000d:
      if (!locals.blanking) {
        if (data & 0x0100) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol] |= (~data & 0xff);
        if (data & 0x0200) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+1] |= (~data & 0xff);
        if (data & 0x0400) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+2] |= (~data & 0xff);
        if (data & 0x0800) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+3] |= (~data & 0xff);
        if (data & 0x1000) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+4] |= (~data & 0xff);
        if (data & 0x2000) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+5] |= (~data & 0xff);
        if (data & 0x4000) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+6] |= (~data & 0xff);
        if (data & 0x8000) coreGlobals.tmpLampMatrix[core_gameData->hw.lampCol+7] |= (~data & 0xff);
      }
      break;

    //Write to driver board
    case 0x00200008:
      DBGLOG(("PC%08x - io_w: [%08x] (%04x) = %x\n",activecpu_get_pc(),offset,mem_mask,data));
      locals.driverBoard = data << 8;
      break;

    //Sols: 1-8 (hi byte) & 17-24 (lo byte)
    case 0x0020000c:
      soldata = core_revword(data^0xffff);
      coreGlobals.pulsedSolState |= (soldata & 0x00ff)<<16;
      coreGlobals.pulsedSolState |= (soldata & 0xff00)>>8;
      locals.solenoids = coreGlobals.pulsedSolState;
      break;
    //Sols: 9-16 (hi byte) & 24-32 (lo byte)
    case 0x0020000d:
      soldata = core_revword(data^0xffff);
      coreGlobals.pulsedSolState |= (soldata & 0x00ff)<<24;
      coreGlobals.pulsedSolState |= (soldata & 0xff00)>>0;
      locals.solenoids = coreGlobals.pulsedSolState;
      break;

    default:
      DBGLOG(("PC%08x - io_w: [%08x] (%04x) = %x\n",activecpu_get_pc(),offset,mem_mask,data));
  }
}

static MACHINE_INIT(cc) {
  memset(&locals, 0, sizeof(locals));
  locals.u16a[0] = 0x00bc;
  locals.vblankCount = 1;
  
  //IRQ1 Maximum Frequency 
  timer_pulse(TIME_IN_CYCLES(2811,0),0,cc_u16irq1);		//Only value that passes IRQ1 test (DO NOT CHANGE UNTIL CURRENT HACK IS REPLACED)
  
  //IRQ4 doesn't seem to do anything?!! Also, no idea of the frequency
  timer_pulse(TIME_IN_HZ(60),0,cc_u16irq4);

  //Force U16 Tests on startup to succeed
  #ifdef USE_U16_TEST_BYPASS
	U16_Tests_ByPass();
  #endif
}

static void U16_Tests_ByPass(void){
	UINT16 jmp1 = 0;
	UINT16 jmp2 = 0;
	switch (core_gameData->hw.gameSpecific1) {
		case 0:
			break;
		case 1:
		case 2:
			fixaddr = 0x00092192; //PM
			jmp1 = 0x1009;
			jmp2 = 0x5738;
			break;
		case 3:
			fixaddr = 0x00089486; //AB
			jmp1 = 0x1008;
			jmp2 = 0x984c;
			break;
		case 4:
			fixaddr = 0x00084a12; //ABR
			jmp1 = 0x1008;
			jmp2 = 0x8000;
			break;
		case 5:
		case 6:
			fixaddr = 0x0008b324; //BS
			jmp1 = 0x1008;
			jmp2 = 0xe95c;
			break;
		case 7:
			fixaddr = 0x00086ce2; //BS102R
			jmp1 = 0x1008;
			jmp2 = 0x6f1c;
			break;
		case 8:
			fixaddr = 0x0007cda8; //BSB
			jmp1 = 0x1008;
			jmp2 = 0x03e0;
			break;
		case 9:
			fixaddr = 0x0000048e; //FF
			jmp1 = 0x1000;
			jmp2 = 0x064e;
			break;
		case 10:
			fixaddr = 0x0004dea8; //BBB
			jmp1 = 0x1005;
			jmp2 = 0x1500;
			break;
		case 11:
			fixaddr = 0x000004a2; //KP
			jmp1 = 0x1000;
			jmp2 = 0x0662;
			break;
		default:
			break;
  }
  //Implements a JMP to address at the 1st point of U16-IRQ4 failure.. The JMP Address goes to the beginning of Driver Board Test (ie, we skip the entire ROM checks)
  *((UINT16 *)(memory_region(REGION_CPU1) + fixaddr))   = 0x4ef9;
  *((UINT16 *)(memory_region(REGION_CPU1) + fixaddr+2)) = jmp1;
  *((UINT16 *)(memory_region(REGION_CPU1) + fixaddr+4)) = jmp2;
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*

//See notes near PORTA & B for details on Port Memory

CS0 ROM
CS1 DRAM
CS2 I/O
CS3 NVRAM (D0-D7, A0-A14)
CS4 A20
CS5 A21
CS6 A22
CS7 A23
!I/O & A23

AT BOOT TIME:

CS0
0x00000000-0x000fffff : ROM0 (U1)
0x00400000-0x004fffff : ROM1 (U2)
0x00800000-0x008fffff : ROM2 (U3)
0x00c00000-0x00cfffff : ROM3 (U4)

AFTER CS0 is configured by software:

0x10000000-0x100fffff : ROM0 (U1)
0x10400000-0x104fffff : ROM1 (U2)
0x10800000-0x108fffff : ROM2 (U3)
0x10c00000-0x10cfffff : ROM3 (U4)

Our Emulation Mappings:

0x00000000-0x000fffff : ROM0 (U1)
0x00400000-0x004fffff : ROM1 (U2)
0x00800000-0x008fffff : ROM2 (U3)
0x00c00000-0x00cfffff : ROM3 (U4)

CS2
AUX
EXT
SWITCH0
CS

CS0 defined by writes to internal registers @ ffc0,ffc2
...
CS7 defined by writes to internal registers @ ffdc,ffde

From the very beginning of program execution, we see writes to the following registers, each bit specifies how CS0-CS7 will act:
Note: Invert the mask to see the end address range

CHIP  DATA        MASK     ADDRESS     RANGE             R/W
--------------------------------------------------------------
CS0 = 1000e680 => ff000000,10000000 => 10000000-10ffffff (R)
CS1 = 0001a2df => fff80000,00000000 => 00000000-0007ffff (RW) * Note: CSFC6,2 = 0, CSFC5,1 = 1
CS2 = 4001a280 => ff000000,40000000 => 40000000-40ffffff (RW)
CS3 = 3001a2f0 => fffe0000,30000000 => 30000000-3001ffff (RW)

On flipper football & kingpin:
CS1 = 0001e6df => fff80000,00000000 => 00000000-0007ffff (RW) * Note: CSFC6,5,2,1 = 1

There are a # of issues with the memory map that need to be discussed:
First - ROM accesses is of course @ address 0, but after the CS0 is redefined, it should be at 0x10000000
DRAM access is moved to 0-7fffff.

However, Martin said he couldn't map stuff that way, so he mapped it as 24 bit addresses and moved stuff around..
Most obvious is that CS2 is @ 0x02000000 instead of 0x40000000, which means all I/O is in that range.
All other addresses are mapped >> 4 bits, ie, 0x30000000 now becomes 0x03000000
*/

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static UINT16 *CMOS;

static NVRAM_HANDLER(cc) {
  core_nvram(file, read_or_write, CMOS, 0x10000, 0x00);
}

static int cc_sw2m(int no) {
  if (no < 9)
    return no + 71;
  else if (no < 81)
    return no - 9;
  return no + 7;
}

static int cc_m2sw(int col, int row) {
  if (col == 9)
    return row + 1;
  return col*8 + row + 9;
}

static data16_t *ramptr;
static MEMORY_READ16_START(cc_readmem)
  { 0x00000000, 0x00ffffff, MRA16_ROM },
  { 0x01000000, 0x0107ffff, MRA16_RAM },			/* DRAM */
  { 0x02000000, 0x02bfffff, io_r },					/* I/O */
  { 0x02C00000, 0x02C007ff, u16_r },				/* U16 (A10,A2,A1)*/
  { 0x03000000, 0x0300ffff, MRA16_RAM },			/* NVRAM */
MEMORY_END

static MEMORY_WRITE16_START(cc_writemem)
  { 0x00000000, 0x00ffffff, MWA16_ROM },
  { 0x01000000, 0x0107ffff, MWA16_RAM, &ramptr },	/* DRAM */
  { 0x02000000, 0x02bfffff, io_w },					/* I/O */
  { 0x02C00000, 0x02C007ff, u16_w },				/* U16 (A10,A2,A1)*/
  { 0x03000000, 0x0300ffff, MWA16_RAM, &CMOS },		/* NVRAM */
MEMORY_END

static PORT_READ16_START(cc_readport)
  { M68306_PORTA_START, M68306_PORTA_END, cc_porta_r },
  { M68306_PORTB_START, M68306_PORTB_END, cc_portb_r },
PORT_END
static PORT_WRITE16_START(cc_writeport)
  { M68306_PORTA_START, M68306_PORTA_END, cc_porta_w },
  { M68306_PORTB_START, M68306_PORTB_END, cc_portb_w },
PORT_END

MACHINE_DRIVER_START(cc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(cc, NULL, NULL)
  MDRV_CPU_ADD(M68306, CPU_CLOCK)
  MDRV_CPU_MEMORY(cc_readmem, cc_writemem)
  MDRV_CPU_PORTS(cc_readport, cc_writeport)
  MDRV_CPU_VBLANK_INT(cc_vblank, 1)
  MDRV_NVRAM_HANDLER(cc)
  MDRV_SWITCH_UPDATE(cc)
  MDRV_SWITCH_CONV(cc_sw2m,cc_m2sw)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_TIMER_ADD(cc_zeroCross, CC_ZCFREQ)
  MDRV_IMPORT_FROM(capcoms)
MACHINE_DRIVER_END

/********************************/
/*** 128 X 32 NORMAL SIZE DMD ***/
/********************************/
PINMAME_VIDEO_UPDATE(cc_dmd128x32) {
  tDMDDot dotCol;
  int ii, jj, kk;
  UINT16 *RAM;

  UINT32 offset = 0x800*locals.visible_page-0x10;
  RAM = ramptr+offset;
  for (ii = 0; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
      for (kk = 0; kk < 16; kk++) {
		UINT16 intens1 = RAM[0];
		for(jj=0;jj<8;jj++) {
			*line++ = (intens1&0xc000)>>14;
			intens1 = intens1<<2;
		}
		RAM+=1;
	  }
    *line++ = 0;
	RAM+=16;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

/*******************************/
/*** 256 X 64 SUPER HUGE DMD ***/
/*******************************/
PINMAME_VIDEO_UPDATE(cc_dmd256x64) {
  tDMDDot dotCol;
  int ii, jj, kk;
  UINT16 *RAM;

  UINT32 offset = 0x800*locals.visible_page-0x20;
  RAM = ramptr+offset;
  for (ii = 0; ii <= 64; ii++) {
    UINT8 *linel = &dotCol[ii][0];
	UINT8 *liner = &dotCol[ii][128];
      for (kk = 0; kk < 16; kk++) {
		UINT16 intensl = RAM[0];
		UINT16 intensr = RAM[0x10];
		for(jj=0;jj<8;jj++) {
			*linel++ = (intensl&0xc000)>>14;
			intensl = intensl<<2;
			*liner++ = (intensr&0xc000)>>14;
			intensr = intensr<<2;
		}
		RAM+=1;
	  }
	RAM+=16;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
