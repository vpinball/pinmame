/* Zaccaria Pinball*/
/* CPU: Signetics 2650 (32K Addressable CPU Space)
   Switch strobing & reading performed via the 2650 "fake" output ports (ie D/~C line)
   Lamps, Solenoids, and Display Data is accessed via 256x4 RAM, using
   DMA (Direct Memory Access) with timers to generate the enable bits for each hardware.
*/
#include "driver.h"
#include "cpu/m6800/m6800.h"
#include "cpu/s2650/s2650.h"
#include "core.h"
#include "zac.h"

#define ZAC_VBLANKFREQ    60 /* VBLANK frequency */
#define ZAC_IRQFREQ      366 /* IRQ frequency (can someone confirm this?)*/

static WRITE_HANDLER(ZAC_soundCmd) { }
static void ZAC_soundInit(void) {}
static void ZAC_soundExit(void) {}

static struct {
  int p0_a, p1_a, p1_b, p0_ca2, p1_ca2, p0_cb2, p1_cb2;
  int swCol;
  int bcd[5];	//There are 5 Displays
  int lampadr1;
  UINT32 solenoids;
  core_tSeg segments,pseg;
  int    diagnosticLed;
  int vblankCount;
  int initDone;
  int refresh;
} locals;

static void ZAC_exit(void);
static void ZAC_nvram(void *file, int write);

static void ZAC_dispStrobe(int mask) {
  int digit = locals.p1_a & 0xfc; //PA2-7 Selects Digits 1-6
  int ii,jj;
  logerror("digit = %x (%x,%x,%x,%x,%x)\n",digit,locals.bcd[0],locals.bcd[1],locals.bcd[2],locals.bcd[3],locals.bcd[4]);
  for (ii = 0; digit; ii++, digit>>=1)
    if (digit & 0x01) {
      UINT8 dispMask = mask;
      for (jj = 0; dispMask; jj++, dispMask>>=1)
		if (dispMask & 0x01) {
          ((int *)locals.segments)[jj*8+ii] |= ((int *)locals.pseg)[jj*8+ii] = core_bcd2seg9[locals.bcd[jj]];
		}
    }
}

static void ZAC_lampStrobe(int board, int lampadr) {
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

#if 0
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
    ZAC_dispStrobe(data&0x0f);
  }
  locals.p0_a = data;
  logerror("bcd data=%x\n",data);
  ZAC_lampStrobe(0,locals.lampadr1);
}
/* PIA1:A-W  0   Display Latch #5
			 1   N/A
			 2-7 Display Digit Select 1-6
*/
static WRITE_HANDLER(pia1a_w) {
  int tmp = locals.p1_a;
  logerror("digit_w: %x\n",data);
  locals.p1_a = data;
  logerror("setting digit to %x\n",locals.p1_a);
  return;
  //if (!locals.p0_ca2) {		  // If Display Blanking cleared
    if (tmp & ~data & 0x01) { // Positive Edge
	  logerror("Latch #5: ca2 = %x, p0_a = %x\n",locals.p0_ca2,locals.p0_a);
      locals.bcd[4] = locals.p0_a>>4;
      ZAC_dispStrobe(0x10);
    }
//  }
}

/* PIA0:B-R  Get Data depending on PIA0:A */
static READ_HANDLER(pia0b_r) {
  if (locals.p0_a & 0x20) return core_getDip(0); // DIP#1 1-8
  if (locals.p0_a & 0x40) return core_getDip(1); // DIP#2 9-16
  if (locals.p0_a & 0x80) return core_getDip(2); // DIP#3 17-24
  return core_getSwCol((locals.p0_a & 0x1f) | ((locals.p1_b & 0x80)>>2));
}

/* PIA0:CB2-W Lamp Strobe #1 */
static WRITE_HANDLER(pia0cb2_w) {
  if (locals.p0_cb2 & ~data) locals.lampadr1 = locals.p0_a & 0x0f;
  locals.p0_cb2 = data;
}
/* PIA1:CA2-W Diagnostic LED & Audio Strobe */
static WRITE_HANDLER(pia1ca2_w) {
  locals.diagnosticLed = data;
  ZAC_soundCmd(0, locals.p1_b & 0x0f);
}

/* PIA0:CA2-W Display Blanking */
static WRITE_HANDLER(pia0ca2_w) {
  //DBGLOG(("PIA0:CA2=%d\n",data));
  locals.p0_ca2 = data;
  if (!data) ZAC_dispStrobe(0x1f);
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
#endif

static int ZAC_vblank(void) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  locals.vblankCount += 1;

  /*-- lamps --*/
  if ((locals.vblankCount % ZAC_LAMPSMOOTH) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  /*-- solenoids --*/
  if ((locals.vblankCount % ZAC_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = locals.solenoids;
    locals.solenoids = coreGlobals.pulsedSolState;
  }
  /*-- display --*/
  if ((locals.vblankCount % ZAC_DISPLAYSMOOTH) == 0) {
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

static void ZAC_updSw(int *inports) {
  if (inports) {
    coreGlobals.swMatrix[0] = (inports[ZAC_COMINPORT]>>9) & 0x03;
    coreGlobals.swMatrix[1] = (coreGlobals.swMatrix[1] & (~0x7f)) |
                              ((inports[ZAC_COMINPORT]) & 0x7f);
    coreGlobals.swMatrix[2] = (coreGlobals.swMatrix[2] & (~0x01)) |
                                ((inports[ZAC_COMINPORT]>>7) & 0x01);
  }
  /*-- Diagnostic buttons on CPU board --*/
  //if (core_getSw(ZAC_SWSOUNDDIAG)) cpu_set_nmi_line(ZAC_SCPU1NO, PULSE_LINE);
}

//Generate the IRQ
static int ZAC_irq(void) { 
#if 1
	logerror("IRQ\n");
	printf("IRQ\n");
	return S2650_INT_IRQ;
#else
	return 0;
#endif
}

static core_tData ZACData = {
  4, /* 4 Dips */
  ZAC_updSw, 1, ZAC_soundCmd, "ZAC",
  core_swSeq2m, core_swSeq2m, core_m2swSeq, core_m2swSeq
};

/* Handle yucky rom address scheme */
void setup_rom(int romnum, int banknum, int startaddr)
{
	memcpy(memory_region(ZAC_MEMREG_CPU)+startaddr,
           memory_region(ZAC_MEMREG_CROM1) + 
				((romnum-1)*0x2000)+(0x800*(banknum-1)), 0x800);
}

static void ZAC_init(void) {
  if (locals.initDone) CORE_DOEXIT(ZAC_exit);

  if (core_init(&ZACData)) return;
  memset(&locals, 0, sizeof(locals));

  /*Setup the Roms - What a mess */
  if(memory_region(ZAC_MEMREG_CPU))
  {
	  setup_rom(1,1,0x0000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x0800);	  //U2 - 1st 2K
	  setup_rom(2,3,0x1000);	  //U2 - 3rd 2K
	  setup_rom(1,2,0x2000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x2800);	  //U2 - 2nd 2K
	  setup_rom(2,4,0x3000);	  //U2 - 4th 2K
	  setup_rom(1,3,0x4000);	  //U1 - 3rd 2K
	  setup_rom(1,4,0x6000);	  //U1 - 4th 2K
	  /*setup mirrors*/
	  setup_rom(2,1,0x4800);	  //U2 - 1st 2K
	  setup_rom(2,3,0x5000);	  //U2 - 3rd 2K
	  setup_rom(2,2,0x6800);	  //U2 - 2nd 2K
	  setup_rom(2,4,0x7000);	  //U2 - 4th 2K
  }
  if (coreGlobals.soundEn) ZAC_soundInit();
  locals.vblankCount = 1;
  locals.initDone = TRUE;
}

static void ZAC_init2(void) {
  if (locals.initDone) CORE_DOEXIT(ZAC_exit);

  if (core_init(&ZACData)) return;
  memset(&locals, 0, sizeof(locals));

  /*Setup the Roms - What a mess */
  if(memory_region(ZAC_MEMREG_CPU))
  {
	  setup_rom(1,1,0x0000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x0800);	  //U2 - 1st 2K
	  setup_rom(3,1,0x1000);	  //U3 - 1st 2K
	  setup_rom(1,2,0x2000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x2800);	  //U2 - 2nd 2K
	  setup_rom(3,2,0x3000);	  //U3 - 2nd 2K
	  /*setup mirrors*/
	  setup_rom(1,1,0x4000);	  //U1 - 1st 2K
	  setup_rom(2,1,0x4800);	  //U2 - 1st 2K
	  setup_rom(3,1,0x5000);	  //U3 - 1st 2K
	  setup_rom(1,2,0x6000);	  //U1 - 2nd 2K
	  setup_rom(2,2,0x6800);	  //U2 - 2nd 2K
	  setup_rom(3,2,0x7000);	  //U3 - 2nd 2K
  }

  if (coreGlobals.soundEn) ZAC_soundInit();
  locals.vblankCount = 1;
  locals.initDone = TRUE;
}

static void ZAC_exit(void) {
  if (coreGlobals.soundEn) ZAC_soundExit();
  core_exit();
}
static UINT8 *ZAC_CMOS;
//4 top bits
static WRITE_HANDLER(ZAC_CMOS_w) {
  data |= 0x0f;
  ZAC_CMOS[offset] = data;
}


/*   CTRL PORT : READ = D0-D7 = Switch Returns 0-7 */
static READ_HANDLER(ctrl_port_r)
{
	//logerror("%x: Ctrl Port Read\n",cpu_getpreviouspc());
	logerror("%x: Switch Returns Read: Col = %x\n",cpu_getpreviouspc(),locals.swCol);
	return core_getSwCol(locals.swCol);
}

/*   DATA PORT : READ = D0-D3 = Dip Switch Read D0-D3 & Program Switch 1 on D3
						D4-D7 = ActSnd & ActSpk? Pin 20,19,18,17 of CN?
*/
static READ_HANDLER(data_port_r)
{
	logerror("%x: Dip & ActSnd/Spk Read - Dips=%x\n",cpu_getpreviouspc(),core_getDip(0)&0x0f);
	//logerror("%x: Data Port Read\n",cpu_getpreviouspc());
	return core_getDip(0)&0x0f; // 4Bit Dip Switch;
}

/*
   SENSE PORT: READ = Read Serial Input (What is this hooked to?)
*/
static READ_HANDLER(sense_port_r)
{
	logerror("%x: Sense Port Read\n",cpu_getpreviouspc());
	return 0;
}

/*
   ------------------
   CTRL PORT : WRITE =	D0-D2 = Switch Strobes (3-8 Demultiplexed)
						D3=REFRESH Strobe??
						D4=Pin 5 of CN? & /RUNEN = !D4
						D5=LED
						D6=Pin 15 of CN?
						D7=Pin 11 of CN?
*/
static WRITE_HANDLER(ctrl_port_w)
{
	//logerror("%x: Ctrl Port Write=%x\n",cpu_getpreviouspc(),data);
	//logerror("%x: Switch Strobe & LED Write=%x\n",cpu_getpreviouspc(),data);
	locals.refresh = (data>>3)&1;
	locals.diagnosticLed = (data>>5)&1;
	locals.swCol = data & 0x07;
#if 0
	int tmp;
	locals.swCol = 0;
	//3-8 Demultiplexed
	for(tmp = 0; tmp < 3; tmp++) {
		if((data>>tmp)&1) {
			locals.swCol |= 1<<tmp;
		}
	}
	locals.swCol+=1;
#endif
	logerror("strobe data = %x, swcol = %x\n",data&0x07,locals.swCol);
	logerror("refresh=%x\n",locals.refresh);
}

/*   DATA PORT : WRITE = Sound Data 0-7 */
static WRITE_HANDLER(data_port_w)
{
	//logerror("%x: Data Port Write=%x\n",cpu_getpreviouspc(),data);
	logerror("%x: Sound Data Write=%x\n",cpu_getpreviouspc(),data);
}

/*   SENSE PORT: WRITE = ??*/
static WRITE_HANDLER(sense_port_w)
{
	logerror("%x: Sense Port Write=%x\n",cpu_getpreviouspc(),data);
}


static READ_HANDLER(ram_r) { 
	logerror("ram_r:  offset = %4x\n",offset);
	return 0;
}
static WRITE_HANDLER(ram_w) { 
	logerror("ram_w: offset = %4x data= %4x\n",offset,data);
}

/*-----------------------------------
/  Memory map for CPU board
/------------------------------------*/
/*
Chip Select Decoding:
---------------------
A13 A12 A11 VMA
  L   L   L   L   L  H  H  H  H  H  H  H  = CS0  = (<0x800) & (0x4000 mirrored)
  L   L   H   L   H  L  H  H  H  H  H  H  = CS1  = (0x800)  & (0x4800 mirrored)
  L   H   L   L   H  H  L  H  H  H  H  H  = CS2  = (0x1000) & (0x5000 mirrored)
  L   H   H   L   H  H  H  L  H  H  H  H  = CS3  = (0x1800) & (0x5800 mirrored)
  H   L   L   L   H  H  H  H  L  H  H  H  = CS0* = (0x2000) & (0x6000 mirrored)
  H   L   H   L   H  H  H  H  H  L  H  H  = CS1* = (0x2800) & (0x6800 mirrored)
  H   H   L   L   H  H  H  H  H  H  L  H  = CS2* = (0x3000) & (0x7000 mirrored)
  H   H   H   L   H  H  H  H  H  H  H  L  = CS3* = (0x3800) & (0x7800 mirrored)
  
  *= Mirrored by hardware design

  CS0 = EPROM1
  CS1 = EPROM2
  CS2 = EPROM3 (OR EPROM2 IF 2764 EPROMS USED)
  CS3 = RAM

  2 Types of RAM:
  a) DMA WRITE ONLY RAM:
	 2104 (256x4)
     Read:		N/A			(because Data Outputs are not on Data Bus)
	 Write:		1800-18ff	(and mirrored addresses)
  b) BATTERY BACKED GAME RAM: 
	 2114 & 6514 (1024x4) - Effective 8 bit RAM.
	 Read:		1800 - 1bff						(and mirrored addresses)
	 Write: 1800 - 18b0 WHEN DIP.SW4 NOT SET!	(and mirrored addresses)
	 Write: 1800 - 1bff WHEN DIP.SW4 SET!		(and mirrored addresses)
*/

/* 3 x 2532 EPROM Configuration */
static MEMORY_READ_START(ZAC_readmem)
{ 0x0000, 0x07ff, MRA_ROM },	/* U1 - 1st 2K */
{ 0x0800, 0x0fff, MRA_ROM },	/* U2 - 1st 2K */
{ 0x1000, 0x17ff, MRA_ROM },	/* U3 - 1st 2K */
{ 0x1800, 0x1bff, ram_r },		/* RAM */
{ 0x2000, 0x27ff, MRA_ROM },	/* U1 - 2nd 2K */
{ 0x2800, 0x2fff, MRA_ROM },	/* U2 - 2nd 2K */
{ 0x3000, 0x37ff, MRA_ROM },	/* U3 - 2nd 2K */
{ 0x3800, 0x3bff, ram_r },		/* RAM Mirror  */
{ 0x4000, 0x47ff, MRA_ROM },	/* U1 - 1st 2K - MIRROR */
{ 0x4800, 0x4fff, MRA_ROM },	/* U2 - 1st 2K - MIRROR */
{ 0x5000, 0x57ff, MRA_ROM },	/* U3 - 1st 2K - MIRROR */
{ 0x5800, 0x5bff, ram_r },		/* RAM Mirror  */
{ 0x6000, 0x67ff, MRA_ROM },	/* U1 - 2nd 2K - MIRROR */
{ 0x6800, 0x6fff, MRA_ROM },	/* U2 - 2nd 2K - MIRROR */
{ 0x7000, 0x77ff, MRA_ROM },	/* U3 - 2nd 2K - MIRROR */
{ 0x7800, 0x7bff, ram_r },		/* RAM Mirror */
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem)
{ 0x0000, 0x07ff, MWA_ROM },	/* U1 - 1st 2K */
{ 0x0800, 0x0fff, MWA_ROM },	/* U2 - 1st 2K */
{ 0x1000, 0x17ff, MWA_ROM },	/* U3 - 1st 2K */
{ 0x1800, 0x1bff, ram_w },		/* RAM */
{ 0x2000, 0x27ff, MWA_ROM },	/* U1 - 2nd 2K */
{ 0x2800, 0x2fff, MWA_ROM },	/* U2 - 2nd 2K */
{ 0x3000, 0x37ff, MWA_ROM },	/* U3 - 2nd 2K */
{ 0x3800, 0x3bff, ram_w },		/* RAM Mirror  */
{ 0x4000, 0x47ff, MWA_ROM },	/* U1 - 1st 2K - MIRROR */
{ 0x4800, 0x4fff, MWA_ROM },	/* U2 - 1st 2K - MIRROR */
{ 0x5000, 0x57ff, MWA_ROM },	/* U3 - 1st 2K - MIRROR */
{ 0x5800, 0x5bff, ram_w },		/* RAM Mirror  */
{ 0x6000, 0x67ff, MWA_ROM },	/* U1 - 2nd 2K - MIRROR */
{ 0x6800, 0x6fff, MWA_ROM },	/* U2 - 2nd 2K - MIRROR */
{ 0x7000, 0x77ff, MWA_ROM },	/* U3 - 2nd 2K - MIRROR */
{ 0x7800, 0x7bff, ram_w },		/* RAM Mirror */
MEMORY_END

/* 2 x 2764 EPROM Configuration */
static MEMORY_READ_START(ZAC_readmem2)
{ 0x0000, 0x07ff, MRA_ROM },	/* U1 - 1st 2K */
{ 0x0800, 0x0fff, MRA_ROM },	/* U2 - 1st 2K */
{ 0x1000, 0x17ff, MRA_ROM },	/* U2 - 3rd 2K */
{ 0x1800, 0x1bff, ram_r },		/* RAM */
{ 0x2000, 0x27ff, MRA_ROM },	/* U1 - 2nd 2K */
{ 0x2800, 0x2fff, MRA_ROM },	/* U2 - 2nd 2K */
{ 0x3000, 0x37ff, MRA_ROM },	/* U2 - 4th 2K */
{ 0x3800, 0x3bff, ram_r },		/* RAM Mirror */
{ 0x4000, 0x47ff, MRA_ROM },	/* U1 - 3rd 2K */
{ 0x4800, 0x4fff, MRA_ROM },	/* U2 - 1st 2K - MIRROR */
{ 0x5000, 0x57ff, MRA_ROM },	/* U2 - 3rd 2K - MIRROR */
{ 0x5800, 0x5bff, ram_r },		/* RAM Mirror */
{ 0x6000, 0x67ff, MRA_ROM },	/* U1 - 4th 2K */
{ 0x6800, 0x6fff, MRA_ROM },	/* U2 - 2nd 2K - MIRROR */
{ 0x7000, 0x77ff, MRA_ROM },	/* U2 - 4th 2K - MIRROR */
{ 0x7800, 0x7bff, ram_r },		/* RAM Mirror */
MEMORY_END

static MEMORY_WRITE_START(ZAC_writemem2)
{ 0x0000, 0x07ff, MWA_ROM },	/* U1 - 1st 2K */
{ 0x0800, 0x0fff, MWA_ROM },	/* U2 - 1st 2K */
{ 0x1000, 0x17ff, MWA_ROM },	/* U2 - 3rd 2K */
{ 0x1800, 0x1bff, ram_w },		/* RAM */
{ 0x2000, 0x27ff, MWA_ROM },	/* U1 - 2nd 2K */
{ 0x2800, 0x2fff, MWA_ROM },	/* U2 - 2nd 2K */
{ 0x3000, 0x37ff, MWA_ROM },	/* U2 - 4th 2K */
{ 0x3800, 0x3bff, ram_w },		/* RAM Mirror */
{ 0x4000, 0x47ff, MWA_ROM },	/* U1 - 3rd 2K */
{ 0x4800, 0x4fff, MWA_ROM },	/* U2 - 1st 2K - MIRROR */
{ 0x5000, 0x57ff, MWA_ROM },	/* U2 - 3rd 2K - MIRROR */
{ 0x5800, 0x5bff, ram_w },		/* RAM Mirror */
{ 0x6000, 0x67ff, MWA_ROM },	/* U1 - 4th 2K */
{ 0x6800, 0x6fff, MWA_ROM },	/* U2 - 2nd 2K - MIRROR */
{ 0x7000, 0x77ff, MWA_ROM },	/* U2 - 4th 2K - MIRROR */
{ 0x7800, 0x7bff, ram_w },		/* RAM Mirror */
MEMORY_END

/* PORT MAPPING 
   ------------
   CTRL PORT : READ = D0-D7 = Switch Returns 0-7
   DATA PORT : READ = D0-D3 = Dip Switch Read D0-D3 & Program Switch 1 on D3
					  D4-D7 = ActSnd & ActSpk? Pin 20,19,18,17 of CN?
   SENSE PORT: READ = Read Serial Input (What is this hooked to?)
   ------------------
   CTRL PORT : WRITE =	D0-D3 = Switch Strobes (4-8 Demultiplexed)
						D4=Pin 5 of CN? & /RUNEN = !D4
						D5=LED
						D6=Pin 15 of CN?
						D7=Pin 11 of CN?
   DATA PORT : WRITE = Sound Data 0-7
   SENSE PORT: WRITE = ??
*/
static PORT_WRITE_START( ZAC_writeport )
	{ S2650_CTRL_PORT,  S2650_CTRL_PORT,  ctrl_port_w },
	{ S2650_DATA_PORT,  S2650_DATA_PORT,  data_port_w },
	{ S2650_SENSE_PORT, S2650_SENSE_PORT, sense_port_w },
PORT_END

static PORT_READ_START( ZAC_readport )
	{ S2650_CTRL_PORT, S2650_CTRL_PORT, ctrl_port_r },
	{ S2650_DATA_PORT, S2650_DATA_PORT, data_port_r },
	{ S2650_SENSE_PORT, S2650_SENSE_PORT, sense_port_r },
PORT_END

/* 3 x 2532 EPROM Configuration */
struct MachineDriver machine_driver_ZAC1 = {
  {{  CPU_S2650, 6000000/4, /* 6.00/4 = 1.5Mhz */
      ZAC_readmem, ZAC_writemem, ZAC_readport, ZAC_writeport,
      ZAC_vblank, 1, ZAC_irq, ZAC_IRQFREQ
  }},
  ZAC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, ZAC_init, CORE_EXITFUNC(ZAC_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  ZAC_nvram
};

/* 2 x 2764 EPROM Configuration */
struct MachineDriver machine_driver_ZAC2 = {
  {{  CPU_S2650, 6000000/4, /* 6.00/4 = 1.5Mhz */
      ZAC_readmem2, ZAC_writemem2, ZAC_readport, ZAC_writeport,
      ZAC_vblank, 1, ZAC_irq, ZAC_IRQFREQ
  }},
  ZAC_VBLANKFREQ, DEFAULT_60HZ_VBLANK_DURATION,
  50, ZAC_init2, CORE_EXITFUNC(ZAC_exit)
  CORE_SCREENX, CORE_SCREENY, { 0, CORE_SCREENX-1, 0, CORE_SCREENY-1 },
  0, sizeof(core_palette)/sizeof(core_palette[0][0])/3, 0, core_initpalette,
  VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY, 0,
  NULL, NULL, gen_refresh,
  0,0,0,0, {{0}},
  ZAC_nvram
};

/*-----------------------------------------------
/ Load/Save static ram
/-------------------------------------------------*/
static void ZAC_nvram(void *file, int write) {
  //core_nvram(file, write, ZAC_CMOS, 0x100,0xff);
}
