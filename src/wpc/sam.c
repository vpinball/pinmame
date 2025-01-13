/************************************************************************************************
  Stern Pinball - S.A.M. Hardware System
  initial version by Steve Ellenoff and Gerrit Volkenborn
  (09/25/2006 - 10/18/2006)
  with various improvements from the open source community after that (notably Arngrim, CarnyPriest and especially DJRobX)

  Hardware from 01/2006 (World Poker Tour) - 09/2014 (The Walking Dead) (and Spider-Man Vault Edition 02/2016)

  CPU Board: S.A.M. System Board
             SPI Part Nº: 520-5246-00

  Issues: (outdated?)
  -USB Interface not hooked up or emulated
  -FlashROM not emulated (treated as a regular read only rom)
  -Real Time Clock not hooked up or emulated
  -We can't hook up a sound commander easily since there's no external sound command sent, it's all
   internalized, so we'd need to look for each game's spot in RAM where they write a command,
   and send our commands there.
  -Still a # of unmapped / unhandled address writes (maybe reads as well)
  -sam_LED_hack() triggers specialized hacks to get LED updates going immediately (otherwise takes a while)
  -IJ/CSI still have timing issues that are worked around for now, see at91_block_timers

  FIRQ frequency of 4008Hz was measured on real machine.

  Fixes history:
  05/25/2008 - Finally fixed the bug (reported by Destruk 11/15/07) that was causing crashes since
               Spiderman 1.90 and recently 1.92, as well as Indiana Jones 1.13 - (Steve Ellenoff)
  10/24/2008 - Added on-table DMD panels and extra lamp columns for World Poker Tour, Family Guy,
               Wheel of Fortune, Shrek, Batman, and CSI (Gerrit Volkenborn)
  03/09/2012 - Added SAM2 generation for extended memory, and possible stereo support one day
************************************************************************************************/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "vpintf.h"
#include "video.h"
#include "cpu/at91/at91.h"
#include "sndbrd.h"
#include "mech.h"

#if defined(VPINMAME)
extern void dmddeviceFwdConsoleData(UINT8 data);
#elif defined(LIBPINMAME)
extern void libpinmame_forward_console_data(void* data, int size);
#endif

// Defines

#define SAM_USE_JIT
#define SAM_DISPLAYSMOOTH 4

#define SAM_SOL_FLIPSTART 13
#define SAM_SOL_FLIPEND 16
#define SAM_FASTFLIPSOL 33

#define SAM_CPUFREQ 40000000
#define SAM_IRQFREQ 4008

#define SAM_SOUNDFREQ 24000
#define SAM_ZC_FREQ 120 // was 145
// 100ms sound buffer.
#define SNDBUFSIZE (SAM_SOUNDFREQ * 100 / 1000)

#define SAM_ROMBANK0 1


#define SAM_NO_AUX                 0x0000 // all games without special stuff
#define SAM_GAME_TRON              0x0001 // Board 511-6927-01: Tron RGB ramps strobed on C and D outputs
#define SAM_GAME_WPT               0x0002 // Board 520-5250-14: World Poker Tour 14 LED matrix blocks
#define SAM_GAME_FG                0x0004 // Board 520-5264-00: Family Guy & Shrek mini playfield LEDs
#define SAM_GAME_WOF               0x0008 // Board 520-5274-00: Wheel of Fortune Playfield Mini-Dot Display (5X7) & Board 520-5283-00: Wheel of Fortune Opto, LEDs and flasher
#define SAM_GAME_3LEDS_BSTB        0x0010 // Board 520-5290-00: 5 Optos and 3 LEDs #86, #87, #88 wired to BSTB for Batman The Dark Knight
#define SAM_GAME_3LEDS_CSTB        0x0020 // Board 520-5290-00: 5 Optos and 3 LEDs #86, #87, #88 wired to CSTB fot CSI
#define SAM_GAME_AUXSOL8_CSTB      0x0040 // Board 520-5325-00: Driver Board 8 Transistors wired to CSTB for Avengers, X-Men LE
#define SAM_GAME_AUXSOL8_DSTB      0x0080 // Board 520-5325-00: Driver Board 8 Transistors wired to DSTB for AC/DC Premium/LE
#define SAM_GAME_AUXSOL6           0x0100 // Board 520-5326-01: Driver Board 6 Transistors for Metallica
#define SAM_GAME_AUXSOL12          0x0200 // Board 520-5326-02: Driver Board 12 Transistors
#define SAM_GAME_IJ4               0x0400 // Flipper bats timing hack & Board 520-5289-00: directly wired LED flashers used by Indiana Jones
#define SAM_GAME_CSI               0x0800 // Flipper bats timing hack
#define SAM_GAME_ACDC_FLAMES       0x1000 // Board 520-5332-00: AC/DC LE special aux board for flame lights (8 LEDs)
#define SAM_GAME_METALLICA_MAGNET  0x2000 // Board 520-6801-00: Magnet processor for Metallica LE (coffin magnet)

#define SAM_2COL   2
#define SAM_3COL   3
#define SAM_5COL   5
#define SAM_8COL   8
#define SAM_9COL   9
#define SAM_12COL  12
#define SAM_20COL  20
#define SAM_33COL  33

//Logging Options
#define LOGALL                      0   // set to 0 to log NOTHING
#define LOG_RAW_SOUND_DATA			0   // Set to 1 to log all raw sample data to a file
#define LOG_TO_SCREEN				0   // Set to 1 to print log data to screen instead of logfile
#define LOG_SWITCH_READ_HANDLER		0   // Set to 1 to log the main Switch Read Handler
#define LOG_SWITCH_MATRIX			0   // Set to 1 to log the Switch Matrix reads
#define LOG_UNKNOWN1				0   // Set to 1 to log Unknown location read
#define LOG_IO_STAT					0   // Set to 1 to log IO Status reads
#define LOG_CS_W					0   // Set to 1 to log CS Writes
#define LOG_IOPORT_W				0   // Set to 1 to log IO Port Writes
#define LOG_LED1					0   // Set to 1 to log Led 1 Writes
#define LOG_LED2					0   // Set to 1 to log Led 2? Writes
#define LOG_ROM_BANK				0   // Set to 1 to log ROM BANK Writes
#define LOG_PORT_READ				0   // Set to 1 to log Port Reads

//Static Variables
static data32_t *sam_reset_ram;
static data32_t *sam_page0_ram;
static data32_t *nvram;

#define SAM_LEDS_MAX_STRINGS 4
#define SAM_LED_MAX_STRING_LENGTH 65
#define SAM_LEDS_MAX (SAM_LED_MAX_STRING_LENGTH * SAM_LEDS_MAX_STRINGS)

extern int at91_block_timers;
int at91_receive_serial(int usartno, data8_t *buf, int size);


/*----------------
/ Local variables
/-----------------*/
struct {
	int vblankCount;
	UINT8 diagnosticLed;
	int sw_stb;
	UINT8 zc; // bool
	int video_page[2];
	INT16 samplebuf[2][SNDBUFSIZE];
	INT16 lastsamp[2];
	int sampout;
	int sampnum;
	UINT8 volume[2], mute[2], DAC_mute[2]; //0..127, 0/1
	int pass;
	UINT8 coindoor; // bool
	UINT8 samVersion; // 1 or 2
	UINT16 value;
	INT16 bank;

	// IO Board:
	UINT16 lampcol;
	UINT8 lamprow;
	UINT8 auxstrb;
	UINT8 auxdata;

	// IJ4 & CSI Flipper Solenoid hack
	int flipSolHackCountL;
	data32_t flipSolHackStateL;
	int flipSolHackCountR;
	data32_t flipSolHackStateR;
	int flipSolHackCountUL;
	data32_t flipSolHackStateUL;

	// WPT
	UINT16 latchA, latchB, latchC, latchD, latchE, latchF, latchH;
	UINT16 col;

	// Shrek,FG
	UINT8 latch[4];

	// WOF
	UINT8 ledLatch[6];
	UINT8 dmdLatch[6];
	UINT8 dmdOutputDisabled; // bool

	UINT32 fastflipaddr;
} samlocals;


#if LOG_RAW_SOUND_DATA
FILE *fpSND = NULL;
#endif

#if LOG_TO_SCREEN
#if LOGALL
#define LOG(x) printf x
#else
#define LOG(x)
#endif
#else
#if LOGALL
#define LOG(x) logerror x
#else
#define LOG(x)
#endif
#endif


/************************************/
/*  Helper functions & definitions  */
/************************************/
#define MAKE16BIT(x,y) (((x)<<8)|(y))

//adjust address offset based on mask - this allows us to work with separate addresses, rather than seeing
//read/write to various pieces of the 32 bit address.
static int adj_offset(int mask)
{
	int i;
	int offset = 0;
	for(i=0;i<4;i++)
	{
		if( 0xFFu<<(i*8) & mask )
		{
			offset=i;
			break;
		}
	}
	return offset;
}

/*****************/
/*  I/O Section  */
/*****************/

/*
 Dedicated Switches 0-16
 ------------------------
 D0  - DED #1  - Left Coin
 D1  - DED #2  - Center Coin
 D2  - DED #3  - Right Coin
 D3  - DED #4  - 4th Coin
 D4  - DED #5  - 5th Coin
 D5  - DED #6  - NA
 D6  - DED #7  - Left Post Save (UK Only)
 D7  - DED #8  - Right Post Save (UK Only)
 D8  - DED #9  - Left Flipper
 D9  - DED #10 - Left Flipper EOS
 D10 - DED #11 - Right Flipper
 D11 - DED #12 - Right Flipper EOS
 D12 - DED #13 - Upper Left Flipper
 D13 - DED #14 - Upper Left Flipper EOS
 D14 - DED #15 - Upper Right Flipper
 D15 - DED #16 - Upper Right Flipper EOS
*/
static int dedswitch_lower_r(void)
{
  /* CORE Defines flippers in order as: RFlipEOS, RFlip, LFlipEOS, LFlip*/
  /* We need to adjust to: LFlip, LFlipEOS, RFlip, RFlipEOS*/
  /* Swap the 4 lowest bits*/
  const UINT8 cds = coreGlobals.swMatrix[9];
  UINT8 fls = coreGlobals.swMatrix[CORE_FLIPPERSWCOL];
  fls = core_revnyb(fls & 0x0f) | (core_revnyb(fls >> 4) << 4);
  return MAKE16BIT(fls,cds);
}

/*
 Dedicated Switches 17-32
 ------------------------
 D16 - DED #17 - Tilt
 D17 - DED #18 - Slam Tilt
 D18 - DED #19 - Ticket Notch
 D19 - DED #20 - NA
 D20 - DED #21 - Back (Coin Door)
 D21 - DED #22 - Plus (Coin Door)
 D22 - DED #23 - Minus (Coin Door)
 D23 - DED #24 - Select (Coin Door)
 D24 - DED #25 - Dip #1
 D25 - DED #26 - Dip #2
 D26 - DED #27 - Dip #3
 D27 - DED #28 - Dip #4
 D28 - DED #29 - Dip #5
 D29 - DED #30 - Dip #6
 D30 - DED #31 - Dip #7
 D31 - DED #32 - Dip #8
*/
static int dedswitch_upper_r(void)
{
	return MAKE16BIT(core_getDip(0),coreGlobals.swMatrix[0]);
}

/*-------------------------
/ Machine driver constants
/--------------------------*/
/*-- Common Inports for SAM Games --*/
#define SAM_COMPORTS \
  PORT_START /* 0 */ \
    /*Switch Col. 0*/ \
    COREPORT_BITDEF(  0x0010, IPT_TILT,           KEYCODE_INSERT) \
    COREPORT_BIT   (  0x0020, "Slam Tilt",        KEYCODE_HOME) \
    COREPORT_BIT   (  0x0040, "Ticket Notch",     KEYCODE_K) \
    COREPORT_BIT   (  0x0080, "Dedicated Sw#20",  KEYCODE_L) \
    COREPORT_BIT   (  0x0100, "Back",             KEYCODE_7) \
    COREPORT_BIT   (  0x0200, "Minus",            KEYCODE_8) \
    COREPORT_BIT   (  0x0400, "Plus",             KEYCODE_9) \
    COREPORT_BIT   (  0x0800, "Select",           KEYCODE_0) \
    /*Switch Col. 2*/ \
    COREPORT_BIT   (  0x8000, "Start Button",     KEYCODE_1) \
    COREPORT_BIT   (  0x4000, "Tournament Start", KEYCODE_2) \
    /*Switch Col. 9*/ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,          KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,          KEYCODE_4) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,          KEYCODE_5) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,          KEYCODE_6) \
    /*None*/ \
    COREPORT_BITTOG(  0x1000, "Coin Door",        KEYCODE_END) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x001f, 0x0000, "Country") \
      COREPORT_DIPSET(0x0000, "USA" ) \
      COREPORT_DIPSET(0x000d, "Australia" ) \
      COREPORT_DIPSET(0x0001, "Austria" ) \
      COREPORT_DIPSET(0x0002, "Belgium" ) \
      COREPORT_DIPSET(0x0003, "Canada 1" ) \
      COREPORT_DIPSET(0x001a, "Canada 2" ) \
      COREPORT_DIPSET(0x001c, "China" ) \
      COREPORT_DIPSET(0x0013, "Chuck E. Cheese" ) \
      COREPORT_DIPSET(0x0016, "Croatia" ) \
      COREPORT_DIPSET(0x0009, "Denmark" ) \
      COREPORT_DIPSET(0x0005, "Finland" ) \
      COREPORT_DIPSET(0x0006, "France" ) \
      COREPORT_DIPSET(0x0007, "Germany" ) \
      COREPORT_DIPSET(0x000f, "Greece" ) \
      COREPORT_DIPSET(0x0008, "Italy" ) \
      COREPORT_DIPSET(0x001b, "Lithuania" ) \
      COREPORT_DIPSET(0x0015, "Japan" ) \
      COREPORT_DIPSET(0x0017, "Middle East" ) \
      COREPORT_DIPSET(0x0004, "Netherlands" ) \
      COREPORT_DIPSET(0x0010, "New Zealand" ) \
      COREPORT_DIPSET(0x000a, "Norway" ) \
      COREPORT_DIPSET(0x0011, "Portugal" ) \
      COREPORT_DIPSET(0x0019, "Russia" ) \
      COREPORT_DIPSET(0x0014, "South Africa" ) \
      COREPORT_DIPSET(0x0012, "Spain" ) \
      COREPORT_DIPSET(0x000b, "Sweden" ) \
      COREPORT_DIPSET(0x000c, "Switzerland" ) \
      COREPORT_DIPSET(0x0018, "Taiwan" ) \
      COREPORT_DIPSET(0x000e, "U.K." ) \
      COREPORT_DIPSET(0x001d, "Unknown (00011101)" ) \
      COREPORT_DIPSET(0x001e, "Unknown (00011110)" ) \
      COREPORT_DIPSET(0x001f, "Unknown (00011111)" ) \
      COREPORT_DIPNAME( 0x0020, 0x0000, "Dip #6") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0020, "1" ) \
      COREPORT_DIPNAME( 0x0040, 0x0000, "Dip #7") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0040, "1" ) \
      COREPORT_DIPNAME( 0x0080, 0x0080, "Dip #8") \
      COREPORT_DIPSET(0x0000, "1" ) \
      COREPORT_DIPSET(0x0080, "0" )

/*-- Standard input ports --*/
#define SAM_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SAM_COMPORTS \
  INPUT_PORTS_END

#define SAM_COMINPORT CORE_COREINPORT

#define SAM_NB_ACDC_METALLICA           0
#define SAM_NB_STARTREK                 1
#define SAM_NB_MUSTANG                  2
#define SAM_NB_TWD                      3
static void sam_init_nodeboard(int bridge);
static void sam_nodebus_transmit(int usartno, data8_t* data, int size);

static int sam_getSol(int solNo)
{
	return coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + solNo - 1].value > 0.5f;
}

static WRITE_HANDLER(scmd_w) { }
static WRITE_HANDLER(man3_w) { }
static void samsnd_init(struct sndbrdData *brdData) { }

//Sound Interface
const struct sndbrdIntf samIntf = {
	"SAM", samsnd_init, NULL, NULL, man3_w, scmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};


static void sam_sh_update(int num, INT16 *buffer[2], int length)
{
	if (length > 0)
	{
		int ii, channel;
		for (ii = 0; ii < length && samlocals.sampout != samlocals.sampnum; ii++)
		{
			for (channel = 0; channel < 2; channel++)
			{
				samlocals.lastsamp[channel] = buffer[channel][ii] = samlocals.samplebuf[channel][samlocals.sampout];
			}
			if (++samlocals.sampout == SNDBUFSIZE)
				samlocals.sampout = 0;
		}
		core_sound_throttle_adj(samlocals.sampnum, &samlocals.sampout, SNDBUFSIZE, SAM_SOUNDFREQ);
		
		for (; ii < length; ++ii)
			for (channel = 0; channel < 2; channel++)
				buffer[channel][ii] = samlocals.lastsamp[channel];
	}
}

static int sam_sh_start(const struct MachineSound *msound)
{
	const char *stream_name[] = { "SAM Left", "SAM Right" };
	const int volume[2] = { MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) };
	/*-- allocate a DAC stream at fixed frequency --*/
	return stream_init_multi
	  (2,
	  stream_name,
	  volume,
	  SAM_SOUNDFREQ,
	  0,
	  sam_sh_update) < 0;
}

static void sam_sh_stop(void) { }

static struct CustomSound_interface samCustInt =
{
	sam_sh_start,
	sam_sh_stop,
	0,
};

//Memory Reads
//Complete - Needs cleanup
static READ32_HANDLER(samswitch_r)
{
	data32_t data = 0;
	const int base = 0x01100000;
	const int mask = ~mem_mask;
	const int adj = adj_offset(mask);
	const int realoff = (offset*4);
	const int addr = base + realoff + adj;
	int logit = LOG_SWITCH_READ_HANDLER;
	offset = addr;

	//Switch Reads - Switch Matrix (16 columns), Dedicated Switch Matrix (24 columns), DipSwitches (8 columns)
	if(offset >= 0x1100000 && offset <= 0x1100006)
	{
		logit = LOG_SWITCH_MATRIX;
		switch(offset)
		{
			//0x1100000 = Switch Matrix (0-16)
			case 0x1100000:
			{
				//Must convert from our 8x8 matrix to a 4x16 matrix - and return 16 bits.
				data = MAKE16BIT(coreGlobals.swMatrix[2+samlocals.sw_stb*2], coreGlobals.swMatrix[1+samlocals.sw_stb*2]);
				data ^= 0xffff;
				break;
			}
			//0x1100002 = Dedicated Switch Matrix (0-16)
			case 0x1100002:
			{
				data = dedswitch_lower_r();
				// Copy flipper states (D8, D10, D12, D14) to EOS (Dx+1).   SAM is not 
				// using standard VPM flipper coils, so the EOS simulation does not
				// take place, and the ROM reports technician errors.
				data |= (data & ((1 << 8) | (1 << 10) | (1 << 12) | (1 << 14))) << 1;
				data ^= 0xffff;
				break;
			}
			//0x1100004 = Dedicated Switch Matrix (17-24) & Dips
			case 0x1100004:
			{
				//Same as 0x1100005
				if(mask == 0xff00)
				{
					data = core_getDip(0);
					data ^= 0xff;
					data <<= 8;
				}
				else
				{
					data = dedswitch_upper_r();
					data ^= 0xffff;
				}
				break;
			}
			//Dips only
			case 0x1100005:
				data = core_getDip(0);
				data ^= 0xff;
				break;
			default:
				LOG(("error"));
				break;
		}
	}

	//5-25-08
	//Related somehow to the bank switch write @ 258000, but not sure how. The data read from this
	//address takes the last bank switch read and this value and keeps the 1st four bits (0xf).
	//To see this, load up sman_192 and set break point to 0x24e50 and watch as it goes into the
	//bank routine @ 0x102b0. Keep your eyes on R0. This address is read on line 0x102bc, and the key line
	//is 0x102cc AND R0,R1,#$f.

	//For now we simply return the last value written to the bank switch which seems to work. It fixed
	//the bugs with Spiderman 1.90,1.92 and Indiana Jones 113 crashing, since the wrong rom bank would
	//get set because this value was previously returning zero. It was not caught sooner because most of the
	//earlier games always used bank 0 anyway, thus returning 0 worked. Spiderman 1.92 and beyond start their
	//game code in bank 1, thus the crashes & problems "appeared". I hope there's nothing more to this function,
	//although I can't understand what possible purpose it really serves. (SJE)

	if(offset == 0x1180000)
	{
		logit = LOG_UNKNOWN1;
		data = samlocals.bank | 0x10;  // 0x10  (bits 4-7, mask 0x70) is hardware rev.
	}

	if(logit) LOG(("%08x: reading from: %08x = %08x (mask=%x)\n",activecpu_get_pc(),offset,data,mask));

	return (data<<(adj*8));
}

/*
  U19 & U20 of IOBOARD - Address decoding (see sambank_w)
*/
static READ32_HANDLER(samio_r)
{
	const int base = 0x02400000;

	const int mask = ~mem_mask;
	const int adj = adj_offset(mask);
	const int realoff = (offset * 4);
	const int addr = base + realoff + adj;
	int logit = LOG_IO_STAT;
	offset = addr;

	data32_t data = 0;
	switch (offset)
	{
	case 0x02400025: // STATUS
		//	D0 = Interlock 20V
		// D1 = Interlock 50V
		// D2 = Zero Cross Circuit
		// D3 = Lamp 1 Stat tied to Lamp Column Drivers 1,3,5,7
		// D4 = Lamp 2 Stat tied to Lamp Column Drivers 2,4,6,8
		// D5 = NA - GND
		// D6 = NA - GND
		// D7 = NA - GND
		data = (7u << 3) | (samlocals.zc<<2) | (samlocals.coindoor != 0 ? 3u:0u);
		break;
	case 0x02400027: // AUX_IN data from J3 connector
		// TODO implement for aux boards using it
		logit = 1;
		break;
	default:
		logit = 1;
		break;
	}
	if(logit) LOG(("%08x: reading from: %08x = %08x (mask=%x)\n",activecpu_get_pc(),0x02400024+offset,data,mask));
	const int newdata = (data << (adj * 8));
	return newdata;
}

/*static READ32_HANDLER(nvram_r)
{
	if (offset == 2104)
		return 0;
	if (offset == 2105)
		return 4294967295;

#if 0
	if ( offset < 0x4000 )
		return nvram[offset];

	if ( offset != 0x4240 
		&& offset != 0x4414
		&& offset != 0x441a
		&& offset != 0x4420
		&& offset != 0x4426)
		return nvram[offset];
#endif

	return nvram[offset];

#if 0
	switch( offset )
	{
		case 0x4414:
		case 0x441a:
		case 0x4420:
		case 0x4426:
			//nvram[offset] = 0x00000001;
			return nvram[offset];
		
		case 0x4021:
			//nvram[offset] = 0x00000000;
			return nvram[offset];

		case 0x4022:
			//nvram[offset] = 0xFFFF0000;
			return nvram[offset];

		case 0x4035:
			//nvram[offset] = 0xFFFF0022;
			return nvram[offset];
		//Language
		case 0x4036:
			//nvram[offset] = 0xFFFE0100;
			return nvram[offset];
		//Time?
		case 0x4037:
			//nvram[offset] = 0x34352345;
			return nvram[offset];

		//Key pressed
		case 0x4041:
		case 0x408d:
			LOG(("Test"));
			break;

		//Should be 01
		case 0x4240:
			break;

		case 0x4402:
			//nvram[offset] = 0x00000001;
			break;

		default:
			//LOG(("Test"));
			break;
	}
	return nvram[offset];
#endif
}*/

/*****************************/
/*  Memory map for SAM CPU  */
/*****************************/
static MEMORY_READ32_START(sam_readmem)
	{ 0x00000000, 0x000FFFFF, MRA32_RAM },					//Boot RAM
	{ 0x00300000, 0x003FFFFF, MRA32_RAM },					//Swapped RAM
	{ 0x01000000, 0x0107FFFF, MRA32_RAM },					//U13 RAM - General Usage (Code is executed here sometimes)
	{ 0x01080000, 0x0109EFFF, MRA32_RAM },					//U13 RAM - DMD Data for output
	{ 0x0109F000, 0x010FFFFF, MRA32_RAM },					//U13 RAM - Sound Data for output
	{ 0x01100000, 0x011FFFFF, samswitch_r },				//Various Input Signals
	{ 0x02000000, 0x020FFFFF, MRA32_RAM },					//U9 Boot Flash Eprom
	{ 0x02100000, 0x0211FFFF, MRA32_RAM }, //nvram_r },		//U11 NVRAM (128K)
	{ 0x02400000, 0x024000FF, samio_r },				    //I/O Related
	{ 0x03000000, 0x030000FF, MRA32_RAM },					//USB Related
	{ 0x04000000, 0x047FFFFF, MRA32_RAM },					//1st 8MB of Flash ROM U44 Mapped here
	{ 0x04800000, 0x04FFFFFF, MRA32_BANK1 },				//Banked Access to Flash ROM U44 (including 1st 8MB ALSO!)
	{ 0x05000000, 0x057FFFFF, MRA32_RAM },
	{ 0x05800000, 0x05FFFFFF, MRA32_BANK1 },
	{ 0x06000000, 0x067FFFFF, MRA32_RAM },
	{ 0x06800000, 0x06FFFFFF, MRA32_BANK1 },
	{ 0x07000000, 0x077FFFFF, MRA32_RAM },
	{ 0x07800000, 0x07FFFFFF, MRA32_BANK1 },
MEMORY_END

//Memory Writes
static WRITE32_HANDLER(samxilinx_w)
{
	if(~mem_mask & 0xFFFF0000) // Left channel??
	{
		data >>= 16;
		samlocals.samplebuf[0][samlocals.sampnum] = data;
		if (++samlocals.sampnum == SNDBUFSIZE)
			samlocals.sampnum = 0;
	}
	else
	{
		samlocals.samplebuf[1][samlocals.sampnum] = data;
	}

	#if LOG_RAW_SOUND_DATA
	if(fpSND)
		fwrite(&data,2,1,fpSND);
	#endif
}

static WRITE32_HANDLER(samdmdram_w)
{
  const int mask = ~mem_mask;

  switch(offset)
  {
	// Switch Strobe
	case 0x2:
	  samlocals.sw_stb = core_BitColToNum(data);
	  break;
	// DMD Page Register
	case 0x8:
	  if (mask & 0xffff0000)
	    samlocals.video_page[1] = data >> 16;
	  else
	    samlocals.video_page[0] = data;
	  break;

	// ?? - The code reads the dips, and if the value read is 0x76, this address is never written to, otherwise
	//      a hardcoded value of 1 is written here (not during any interrupt code, but regular code).
	case 0x2aaa0:
	  break;

	default:
	  LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),0x1100000+offset,data));
  }
}

static WRITE32_HANDLER(sam_io2_w)
{
  // DJRobX: The new LED stuff is all done by serial IO, so I'm not sure what that would be. Would be interested if it's related to the delay getting TWD and Mustang to start sending LED data though.
  LOG(("%08x: IO2 output to %05x=%02x\n", activecpu_get_pc(), offset, data)); //!! find out what is what for LE support (but even normal TWD uses it (and more??))
}

static WRITE32_HANDLER(sambank_w)
{
	const int base = 0x02400000;

	const int mask = ~mem_mask;
	const int adj = adj_offset(mask);
	const int realoff = (offset*4);
	const int addr = base + realoff + adj;
	const int newdata = (data>>(adj*8));
	int logit = LOG_CS_W;
	offset = addr;
	data = newdata;

	if (offset > 0x024000FF) // CPU/Sound board
	{
		switch (offset)
		{
			//1st LED?
			case 0x02500000:
				logit = LOG_LED1;
				samlocals.diagnosticLed = (data & 1) | (samlocals.diagnosticLed & 2);
				break;
			//Rom Bank Select:
			//D0 = FF1(U42) -> A22
			//D1 = FF2(U42) -> A23
			//D2 = FF3(U42) -> A24	(Not Connected when used with a 256MBIT Flash Memory)
			//D3 = FF4(U42) -> A25	(Not Connected when used with a 256MBIT Flash Memory)
			case 0x02580000:
				logit = LOG_ROM_BANK;

				//log access to A24/A25 depending on the SAM version.
				if(data > (samlocals.samVersion > 1 ? (data32_t)0x0f : (data32_t)0x03))
					LOG(("ERROR IN BANK SELECT DATA = %d\n",data));

				//enforce validity to try and prevent crashes into illegal romspaces.
				data &= (samlocals.samVersion > 1 ? 0x0f : 0x03);

				//Swap bank memory
				cpu_setbank(SAM_ROMBANK0, memory_region(REGION_USER1) + (data << 23));

				//save value for read @ 1180000
				samlocals.bank = data;
				break;
			//2nd LED?
			case 0x02F00000:
				logit = LOG_LED2;
				//LED is ON when zero (pulled to gnd)
				if (!data)
					samlocals.diagnosticLed = (samlocals.diagnosticLed & 1) | 2;
				break;
			default:
				logit = 1;
				LOG(("error"));
				break;
		}
	}
	else if( offset >= 0x2400000 && offset <= 0x24000ff ) // IO Board
	{
		/*
		U19 & U20 of IOBOARD - Address decoding
		A3 A2 A1 A0 (When IOSTB = 0)
		---------------------------------------------------------------------------------------------
		 0  0  0  0 - 0: SOL A Driver
		 0  0  0  1 - 1: SOL B Driver
		 0  0  1  0 - 2: SOL C Driver
		 0  0  1  1 - 3: Flash Lamp Driver
		 0  1  0  1 - 5: STATUS (read)
		 0  1  1  0 - 6: AUX Driver ( Activates 8 Bits Output to J2 Connector )
		 0  1  1  1 - 7: AUX In     ( Activates 8 Bits Input from J3 Connector )
		 1  0  0  0 - 8: Lamp Strobe *(See below)
		 1  0  0  1 - 9: Aux Lamp   ( Activates 2 Bits (D0-D1) to drive Lamp Row Signals 8 & 9 )
		 1  0  1  0 - a: Lamp Driver *(See below)
		 1  0  1  1 - b: Additional Strobe Driver (see below)

		 Lamp Columns are driven by Lamp DRV Signal & 8 Data Bits
		 Lamp Rows are driven by the Lamp Strobe signal & Aux Lamp Signal
		 There are 10 Lamp Rows Max
		 Row 0 - From Lamp Strobe, Data Bit 0
		 Row 1 - From Lamp Strobe, Data Bit 1
		 Row 2 - From Lamp Strobe, Data Bit 2
		 Row 3 - From Lamp Strobe, Data Bit 3
		 Row 4 - From Lamp Strobe, Data Bit 4
		 Row 5 - From Lamp Strobe, Data Bit 5
		 Row 6 - From Lamp Strobe, Data Bit 6
		 Row 7 - From Lamp Strobe, Data Bit 7
		 Row 8 - From Aux Lamp Strobe Line, Data Bit 0
		 Row 9 - From Aux Lamp Strobe Line, Data Bit 1
		---------------------------------------------------------------------------------------------*/
		logit = LOG_IOPORT_W;
		switch (offset)
		{
			case 0x02400020: // SOL_A
				// Solenoids are written every 250us. Flippers are fully CPU controlled with a 40ms power pulse then hold with 1ms pulse every 12ms (so a PWM cycle of 48 writes during hold)
				if (core_gameData->hw.gameSpecific1 & (SAM_GAME_IJ4 | SAM_GAME_CSI)) {
					// FIXME IJ4 & CSI have some emulation timing issues that will make the emulation regularly miss and delay the solenoid writes by 40 to 80ms (masked IRQ ?) 
					// leading to flickering flippers. This is hidden for the flippers by counting the number of writes and only consider the state after the 48 writes (which 
					// means that flippers suffer from eratic latencies. Previous implementations did filter out writes and react based on write count too).
					samlocals.flipSolHackStateL |= data;
					samlocals.flipSolHackStateR |= data;
					data32_t hackedData;
					if (core_gameData->hw.gameSpecific1 & SAM_GAME_CSI) { // CSI also have an upper flipper on solenoid #14
						samlocals.flipSolHackStateUL |= data;
						hackedData = (data & 0x1F) | (samlocals.flipSolHackStateUL & 0x20) | (samlocals.flipSolHackStateL & 0x40) | (samlocals.flipSolHackStateR & 0x80);
						if (data & 0x20) { // align to pulse on
							samlocals.flipSolHackCountUL = 0;
						}
						else if (samlocals.flipSolHackCountUL >= 48) {
							samlocals.flipSolHackCountUL = 0;
							samlocals.flipSolHackStateUL = 0;
						}
					}
					else
					{
						hackedData = (data & 0x3F) | (samlocals.flipSolHackStateL & 0x40) | (samlocals.flipSolHackStateR & 0x80);
					}
					coreGlobals.pulsedSolState &= ~(0xFFu << 8);
					coreGlobals.pulsedSolState |= hackedData << 8;
					core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8, hackedData);
					samlocals.flipSolHackCountL++;
					if (data & 0x40) { // align to pulse on
						samlocals.flipSolHackCountL = 0;
					}
					else if (samlocals.flipSolHackCountL >= 48) {
						samlocals.flipSolHackCountL = 0;
						samlocals.flipSolHackStateL = 0;
					}
					samlocals.flipSolHackCountR++;
					if (data & 0x80) { // align to pulse on
						samlocals.flipSolHackCountR = 0;
					}
					else if (samlocals.flipSolHackCountR >= 48) {
						samlocals.flipSolHackCountR = 0;
						samlocals.flipSolHackStateR = 0;
					}
				}
				else {
					coreGlobals.pulsedSolState &= ~(0xFFu << 8);
					coreGlobals.pulsedSolState |= data << 8;
					core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 8, data);
				}
				break;
			case 0x02400021: // SOL_B
				if (core_gameData->hw.gameSpecific1 & SAM_GAME_WOF)
				{
					// Special case for Wheel of Fortune.   It has a 4-way stepper motor that goes 4,6,5,7 ... but 
					// VPM mech only supports a dual stepper motor.   OR 5 and 7 to 4 and 6 so we never miss these pulses.
					// Need to put state into pulsedSolState for mech handling to see it.
					coreGlobals.pulsedSolState = data | ((data & ((1u << 4) | (1u << 6))) >> 1);
				}
				else
				{
					coreGlobals.pulsedSolState &= ~(0xFFu);
					coreGlobals.pulsedSolState |= data;
				}
				core_write_pwm_output_8b(CORE_MODOUT_SOL0, data);
				break;
			case 0x02400022: // SOL_C
				coreGlobals.pulsedSolState &= ~(0xFFu << 16);
				coreGlobals.pulsedSolState |= data << 16;
				core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 16, data);
				break;
			case 0x02400023: // FLSH_LMP
				coreGlobals.pulsedSolState &= ~(0xFFu << 24);
				coreGlobals.pulsedSolState |= data << 24;
				core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 24, data);
				break;
			// case 0x02400024: // Unconnected
			// case 0x02400025: // STATUS read only
			case 0x02400026: // AUX_DRV (latched data to J2 AUX connector)
				if (core_gameData->hw.gameSpecific1 & SAM_GAME_IJ4) {
					// Indiana Jones uses 4 small LED flasher boards directly connected to AUX DATA pins (no strobe, see schematics on page 137 of manual)
					core_write_pwm_output(CORE_MODOUT_SOL0 + 54 - 1, 1, GET_BIT3); // ark leds (2 boards)
					core_write_pwm_output(CORE_MODOUT_SOL0 + 55 - 1, 1, GET_BIT4); // swordman
					core_write_pwm_output(CORE_MODOUT_SOL0 + 56 - 1, 1, GET_BIT5); // skull
				}
				samlocals.auxdata = data;
				break;
			// case 0x02400027: // AUX_IN read only from J3 connector
			case 0x02400028: // LMP_STB & AUX_LMP (word writes)
				samlocals.lampcol = (samlocals.lampcol & mem_mask) | data;
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0,       samlocals.lampcol       & 0x00FF, samlocals.lamprow, 8);
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (samlocals.lampcol >> 8) & 0x0003, samlocals.lamprow, 2);
				break;
			case 0x02400029: // AUX_LMP
				samlocals.lampcol = (samlocals.lampcol & 0x00FF) | (data << 8);
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0,       samlocals.lampcol       & 0x00FF, samlocals.lamprow, 8);
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (samlocals.lampcol >> 8) & 0x0003, samlocals.lamprow, 2);
				break;
			case 0x0240002A: // LMP_DRV
				samlocals.lamprow = core_revbyte(data);
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0,       samlocals.lampcol       & 0x00FF, samlocals.lamprow, 8);
				core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (samlocals.lampcol >> 8) & 0x0003, samlocals.lamprow, 2);
				break;
			case 0x0240002B: // Latched data for GI relay and AUX strobe (ASTB to ESTB on J2 and J3 connectors)

				// Note that unlike stated in previous version of the code, beta-brite connector is wired to the serial output TXD1/RXD1 of CPU/Sound board 520-5246-00, 520-5246-02, not on the IO power driver board 520-5249-00. The following lines are just kept for reference bit will likely be removed.
				//if ((~data & 0x40) && (samlocals.auxdata & 0x03))
				//	LOG(("%08x: writing to betabrite: %x\n", activecpu_get_pc(), auxdata & 0x03));

				// Bit 0 drives GI relay
				coreGlobals.gi[0] = (~data & 0x01) ? 9 : 0;
				core_write_pwm_output_8b(CORE_MODOUT_GI0, ~data & 0x01);
				
				// Bit 1 & 2 are unconnected on all boards

				// Bit 3..7 are BSTB..ESTB then ASTB which are used to strobe auxiliary boards
				// normal sequence is write data to AUX_DRV latch, then strobe it here by writing a pulse on the right strobe bit

				// Board 511-6927-01: Tron TriColor Assembly strobed on C and D outputs
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_TRON) && (samlocals.auxstrb & ~data & 0x10)) // CSTB
					core_write_pwm_output(CORE_MODOUT_LAMP0 + 100, 3, samlocals.auxdata >> 3); // Right RGB ramp
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_TRON) && (samlocals.auxstrb & ~data & 0x20)) // DSTB
					core_write_pwm_output(CORE_MODOUT_LAMP0 + 103, 3, samlocals.auxdata >> 3); // Left RGB ramp

				// Board 520-5250-14: 14 Block LED (World Poker Tour)
				if (core_gameData->hw.gameSpecific1 & SAM_GAME_WPT)
				{
					if (samlocals.auxstrb & ~data & 0x80) // ASTB
					{
						// 2 Latches for active columns
						samlocals.col =
							  ((samlocals.latchH & 0x10) >> 4)  // H5 -> Col 1
							| ((samlocals.latchH & 0x02))       // H2 -> Col 2
							| ((samlocals.latchH & 0x04))       // H3 -> Col 3
							| ((samlocals.latchH & 0x08))       // H4 -> Col 4
							| ((samlocals.latchH & 0x01) << 4)  // H1 -> Col 5
							| ((samlocals.latchH & 0x80) >> 2)  // H8 -> Col 6
							| ((samlocals.latchH & 0x20) << 1)  // H6 -> Col 7
							| ((samlocals.latchH & 0x40) << 1)  // H7 -> Col 8
							| ((samlocals.latchF & 0x80) << 1)  // F7 -> Col 9
							| ((samlocals.latchE & 0x80) << 2); // E7 -> Col 10
						samlocals.col = ((core_revword(samlocals.col) >> 6) & 0x03E0) | ((samlocals.col >> 5) & 0x001F);
						// 7 Latches for active rows
						const UINT16 latchI = (samlocals.latchH & 0x07); // Does not really exist, made using D7/Q7 of latches E/F/H from Q4/Q5/Q6 of latch H
						samlocals.latchH = (samlocals.latchF & 0x7F) | ((latchI & 0x04) << 5);
						samlocals.latchF = (samlocals.latchE & 0x7F) | ((latchI & 0x01) << 7);
						samlocals.latchE = (samlocals.latchD & 0x7F) | ((latchI & 0x02) << 6);
						samlocals.latchD = (samlocals.latchC & 0x7F);
						samlocals.latchC = (samlocals.latchB & 0x7F);
						samlocals.latchB = (samlocals.latchA & 0x7F);
						samlocals.latchA = samlocals.auxdata;
					}
					for (int row = 0; row < 10; row++) { // 2 rows of 7 LED matrix, each matrix being 7 cols x 5 rows (so 10 rows, 49 cols)
						int r = ((samlocals.latchA & 0x80) || (~data & 0x80)) ? 0 : (samlocals.col & (1 << row));
						int c = CORE_MODOUT_LAMP0 + 10 * 8 + 49 * row;
						core_write_pwm_output(c     , 7, r ? (core_revbyte((UINT8)samlocals.latchH) >> 1) : 0);
						core_write_pwm_output(c +  7, 7, r ? (core_revbyte((UINT8)samlocals.latchF) >> 1) : 0);
						core_write_pwm_output(c + 14, 7, r ? (core_revbyte((UINT8)samlocals.latchE) >> 1) : 0);
						core_write_pwm_output(c + 21, 7, r ? (core_revbyte((UINT8)samlocals.latchD) >> 1) : 0);
						core_write_pwm_output(c + 28, 7, r ? (core_revbyte((UINT8)samlocals.latchC) >> 1) : 0);
						core_write_pwm_output(c + 35, 7, r ? (core_revbyte((UINT8)samlocals.latchB) >> 1) : 0);
						core_write_pwm_output(c + 42, 7, r ? (core_revbyte((UINT8)samlocals.latchA) >> 1) : 0);
					}
					//printf("%8.5f s=%02x d=%02x    %02x %02x %02x %02x %02x %02x %02x   Col=%03x Blank=%d\n", timer_get_time(), data, samlocals.auxdata, samlocals.latchA, samlocals.latchB, samlocals.latchC, samlocals.latchD, samlocals.latchE, samlocals.latchF, samlocals.latchH, col, (samlocals.latchA & 0x80) ? 1 : 0);
				}

				// Board 520-5264-00: Family Guy & Shrek mini playfield LEDs
				if (core_gameData->hw.gameSpecific1 & SAM_GAME_FG)
				{
					// The board contains 4 latches which are shifted to latch row (13 bits), col (2 bits), and output enable (1 bit)
					if (samlocals.auxstrb & ~data & 0x80) // ASTB
					{
						samlocals.latch[3] = samlocals.latch[2];
						samlocals.latch[2] = samlocals.latch[1];
						samlocals.latch[1] = samlocals.latch[0];
						samlocals.latch[0] = samlocals.auxdata;
					}
					int col = ((samlocals.latch[0] & 0x80) || (~data & 0x80)) ? 0 : samlocals.latch[3]; // Output disable if Bit 7 on latch 0 high or ASTB strobe high
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  80, (col & 1) ? samlocals.latch[2] & 0x0F : 0);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  88, (col & 1) ? samlocals.latch[1] & 0x0F : 0);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  96, (col & 1) ? samlocals.latch[0] & 0x1F : 0);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 104, (col & 2) ? samlocals.latch[2] & 0x0F : 0);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 112, (col & 2) ? samlocals.latch[1] & 0x0F : 0);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 120, (col & 2) ? samlocals.latch[0] & 0x1F : 0);
					//printf("%8.5f  write %02x %02x: %02x %02x %02x %02x  out=%d\n", timer_get_time(), data, samlocals.auxdata, samlocals.latch[0] & 0x1F, samlocals.latch[1] &0x0F, samlocals.latch[2] & 0x0F, samlocals.latch[3], col & 3);
				}

				if (core_gameData->hw.gameSpecific1 & SAM_GAME_WOF)
				{
					// Board 520-5283-00: Wheel of Fortune Opto, LEDs and flasher
					if (~samlocals.auxstrb & data & 0x08) // inversed BSTB
					{
						samlocals.ledLatch[2] = samlocals.ledLatch[1];
						samlocals.ledLatch[1] = samlocals.ledLatch[0];
						samlocals.ledLatch[0] = samlocals.auxdata;
					}
					if (samlocals.auxstrb & ~data & 0x08) // BSTB
					{
						samlocals.ledLatch[5] = samlocals.ledLatch[4];
						samlocals.ledLatch[4] = samlocals.ledLatch[3];
						samlocals.ledLatch[3] = samlocals.auxdata;
					}
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  80, samlocals.ledLatch[0] & 0x80 ? 0 : samlocals.ledLatch[0] & 0x7f);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  96, samlocals.ledLatch[0] & 0x80 ? 0 : samlocals.ledLatch[1] & 0x7f);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 112, samlocals.ledLatch[0] & 0x80 ? 0 : samlocals.ledLatch[2] & 0x7f);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 +  88, samlocals.ledLatch[3] & 0x80 ? 0 : samlocals.ledLatch[3] & 0x7f);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 104, samlocals.ledLatch[3] & 0x80 ? 0 : samlocals.ledLatch[4] & 0x7f);
					core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 120, samlocals.ledLatch[3] & 0x80 ? 0 : samlocals.ledLatch[5] & 0x7f);
					//printf("%8.5f  write %02x %02x:  %02x %02x %02x  %02x %02x %02x\n", timer_get_time(), data, samlocals.auxdata, samlocals.ledLatch[0], samlocals.ledLatch[1], samlocals.ledLatch[2], samlocals.ledLatch[3], samlocals.ledLatch[4], samlocals.ledLatch[5]);

					// Board 520-5274-00: Playfield Mini-Dot Display (5X7) (Wheel of Fortune)
					if (samlocals.auxstrb & ~data & 0x10) // CSTB
					{
						UINT8 wasOutputDisabled = samlocals.dmdOutputDisabled;
						samlocals.dmdOutputDisabled = samlocals.auxdata & 0x80;
						if (wasOutputDisabled && samlocals.dmdOutputDisabled)
						{
							samlocals.dmdLatch[5] = samlocals.dmdLatch[4];
							samlocals.dmdLatch[4] = samlocals.dmdLatch[3];
							samlocals.dmdLatch[3] = samlocals.dmdLatch[2];
							samlocals.dmdLatch[2] = samlocals.dmdLatch[1];
							samlocals.dmdLatch[1] = samlocals.dmdLatch[0];
							samlocals.dmdLatch[0] = samlocals.auxdata;
						}
					}
					int col = (samlocals.dmdOutputDisabled || (~data & 0x80)) ? 0 : samlocals.dmdLatch[5];
					for (int row = 0; row < 5; row++) {
						int r = col & (1 << row);
						int c = CORE_MODOUT_LAMP0 + 140 + 35 * row;
						core_write_pwm_output(c,      7, r ? (core_revbyte(samlocals.dmdLatch[4]) >> 1) : 0);
						core_write_pwm_output(c +  7, 7, r ? (core_revbyte(samlocals.dmdLatch[3]) >> 1) : 0);
						core_write_pwm_output(c + 14, 7, r ? (core_revbyte(samlocals.dmdLatch[2]) >> 1) : 0);
						core_write_pwm_output(c + 21, 7, r ? (core_revbyte(samlocals.dmdLatch[1]) >> 1) : 0);
						core_write_pwm_output(c + 28, 7, r ? (core_revbyte(samlocals.dmdLatch[0]) >> 1) : 0);
					}
					//printf("%8.5f  write %02x %02x: %02x %02x %02x %02x %02x  col=%02x Blank=%d\n", timer_get_time(), data, samlocals.auxdata, samlocals.dmdLatch[0], samlocals.dmdLatch[1], samlocals.dmdLatch[2], samlocals.dmdLatch[3], samlocals.dmdLatch[4], col & 0x1F, samlocals.dmdOutputDisabled ? 0 : 1);
				}

				// Board 520-5290-00: Opto and auxiliary LED PCB (Batman The Dark Knight & CSI): 3 LEDs #86, #87, #88
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_3LEDS_BSTB) && (samlocals.auxstrb & ~data & 0x08)) // BSTB
					core_write_masked_pwm_output_8b(CORE_MODOUT_LAMP0 + 80, core_revbyte(samlocals.auxdata), 0xE0);
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_3LEDS_CSTB) && (samlocals.auxstrb & ~data & 0x10)) // CSTB
					core_write_masked_pwm_output_8b(CORE_MODOUT_LAMP0 + 80, core_revbyte(samlocals.auxdata), 0xE0);

				// Board 520-5325-00: 8 transistor driver board (Avengers, AC/DC Premium/LE, X-Men LE)
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_AUXSOL8_CSTB) && (samlocals.auxstrb & ~data & 0x10)) // CSTB
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 8, samlocals.auxdata);
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_AUXSOL8_DSTB) && (samlocals.auxstrb & ~data & 0x20)) // DSTB
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 8, samlocals.auxdata);

				// Board 520-5326-01: 6 transistor driver board (Metallica)
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_AUXSOL6) && (samlocals.auxstrb & ~data & 0x20)) // DSTB
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 6, samlocals.auxdata);

				// Board 520-5326-02: 12 transistor driver board (lots of games)
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_AUXSOL12) && (samlocals.auxstrb & ~data & 0x10)) // CSTB
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL + 8 - 1, 6, samlocals.auxdata);
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_AUXSOL12) && (samlocals.auxstrb & ~data & 0x20)) // DSTB
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 6, samlocals.auxdata);

				// Board 520-5332-00: AC/DC LE special aux board for flame lights (8 LEDs)
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_ACDC_FLAMES) && (samlocals.auxstrb & ~data & 0x08)) // BSTB
					core_write_pwm_output(CORE_MODOUT_LAMP0 + 152 - 1, 8, samlocals.auxdata); // 151..158

				// Board 520-6801-00: Magnet processor (Metallica)
				if ((core_gameData->hw.gameSpecific1 & SAM_GAME_METALLICA_MAGNET) && (~data & 0x08)) // BSTB (not sure if it is trigerred by edge or state since it uses a microcontroller)
				{
					// Operation mode is strobed here from the 2 highest bits D6/D7 of Aux Data
					// 00 => Off
					// 10 => Grab: activate magnet for 1 second then switch it off. It grabs the ball from centimeters away
					// 01 => Detect: detect ball and report it to switch 63
					// 11 => Detect & Hold: detect ball, if detected report to switch 63 and also grab it
					// See https://missionpinball.org/mechs/magnets/stern_magnet_pcb/ for a detailled description:
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL + 6 - 1, 1, samlocals.auxdata >> 7);
					core_write_pwm_output(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL + 7 - 1, 1, samlocals.auxdata >> 6);
				}

				samlocals.auxstrb = data;
				break;
			default:
				LOG(("error"));
				break;
		}
	}
	else logit = 1;

	if(logit) LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
}

/*static WRITE32_HANDLER(nvram_w)
{
	switch( offset )
	{
		//clock??
		case 0x4021:
		case 0x4023:
		case 0x4025:
		case 0x4027:
		case 0x4029:
		case 0x402b:
		case 0x402d:
		case 0x402f:
		case 0x4031:
		case 0x4033:
			//nvram[offset] = 0x00000000;
			//LOG(("Test"));
			return;
		//clock??
		case 0x4022:
		case 0x4024:
		case 0x4026:
		case 0x4028:
		case 0x402a:
		case 0x402c:
		case 0x402e:
		case 0x4030:
		case 0x4032:
		case 0x4034:
			//nvram[offset] = 0xFFFF0000;
			//LOG(("Test"));
			return;

		case 0x4037:
			LOG(("Test"));
			return;

		case 0x4041:
		case 0x408d:
			LOG(("Test"));
			break;

		case 0x4672:
		case 0x4674:
			LOG(("Test"));
			break;

		case 0x4634:
		case 0x4636:
			LOG(("Test"));
			break;

		default:
			LOG(("Test"));
			break;
	}
	nvram[offset] = (nvram[offset] & mem_mask) | (data & ~mem_mask);
}*/

static MEMORY_WRITE32_START(sam_writemem)
	{ 0x00000000, 0x000FFFFF, MWA32_RAM, &sam_page0_ram},  //Boot RAM
	{ 0x00300000, 0x003FFFFF, MWA32_RAM, &sam_reset_ram},  //Swapped RAM
	{ 0x01000000, 0x0107FFFF, MWA32_RAM },				   //U13 RAM - General Usage (Code is executed here sometimes)
	{ 0x01080000, 0x0109EFFF, MWA32_RAM },				   //U13 RAM - DMD Data for output
	{ 0x0109F000, 0x010FFFFF, samxilinx_w },			   //U13 RAM - Sound Data for output
	{ 0x01100000, 0x01FFFFFF, samdmdram_w },			   //Various Output Signals
	{ 0x02100000, 0x0211FFFF, MWA32_RAM, &nvram },		   //U11 NVRAM (128K) 0x02100000,0x0211ffff
	{ 0x02200000, 0x022fffff, sam_io2_w },				   //LE versions: more I/O stuff (mostly LED lamps)
	{ 0x02400000, 0x02FFFFFF, sambank_w },				   //I/O Related
	{ 0x03000000, 0x030000FF, MWA32_RAM },				   //USB Related
	{ 0x04000000, 0x047FFFFF, MWA32_RAM },				   //1st 8MB of Flash ROM U44 Mapped here
	{ 0x04800000, 0x04FFFFFF, MWA32_RAM },				   //Banked Access to Flash ROM U44 (including 1st 8MB ALSO!)
	{ 0x05000000, 0x057FFFFF, MWA32_RAM },
	{ 0x05800000, 0x05FFFFFF, MWA32_RAM },
	{ 0x06000000, 0x067FFFFF, MWA32_RAM },
	{ 0x06800000, 0x06FFFFFF, MWA32_RAM },
	{ 0x07000000, 0x077FFFFF, MWA32_RAM },
	{ 0x07800000, 0x07FFFFFF, MWA32_RAM },
MEMORY_END

/*********************/
/* Port I/O Section  */
/*********************/
static READ32_HANDLER(sam_port_r)
{
	const int logit = LOG_PORT_READ;
	//Set P10,P11 to +1 as shown in schematic since they are tied to voltage.
	const data32_t data = 0x00000C00;
	//Eventually need to add in P16,17,18 for feedback from Real Time Clock
	//Possibly also P23 (USB Related) & P24 (Dip #8)

	if(logit) LOG(("%08x: reading from %08x = %08x\n",activecpu_get_pc(),offset,data));

	return data;
}

//P16,17,18 are connected to Real Time Clock for writing data
//P3,P4,P5 are connected to PCM1755 DAC for controlling sound output & effects (volume and other features).
static WRITE32_HANDLER(sam_port_w)
{
  // Bits 4 to 6 are used to issue a command to the PCM1755 chip as a serial 16-bit value.
  if ((data & 0x10) && samlocals.pass >= 0) samlocals.value |= ((data & 0x20) >> 5) << samlocals.pass--;
  if (data & 0x08) { // end of command data
    int l_vol = 0;
    int r_vol = 0;

    LOG(("Writing to PCM1755 register #$%02x = %02x\n", samlocals.value >> 8, samlocals.value & 0xff));

    switch (samlocals.value >> 8) { // register number is the upper command byte
      case 0x10: // left channel attenuation
        samlocals.volume[0] = samlocals.value & 0xff;
        break;
      case 0x11: // right channel attenuation
        samlocals.volume[1] = samlocals.value & 0xff;
        break;
      case 0x12: // soft mute //!! according to datasheet this should actually fade in(0->samlocals.volume)/out(samlocals.volume->0) 1 step every 8/fS timestep and not kick in immediately to avoid pops
        samlocals.mute[0] = (samlocals.value & 1);
        samlocals.mute[1] = (samlocals.value & 2) >> 1;
        break;
      case 0x13: // DAC off
        samlocals.DAC_mute[0] = (samlocals.value & 1);
        samlocals.DAC_mute[1] = (samlocals.value & 2) >> 1;
        break;
      default: // Filters and the like, nothing serious
        LOG(("Unhandled PCM1755 command"));
        break;
    }
    // Mixer set volume has volume range of 0..100, while the SAM hardware uses 0..127 value range for volume. "Real" volume values start @ 0x80: For ATx[7:0]DEC = 0 through 128, attenuation is set to infinite attenuation (=mute)
    if (samlocals.mute[0] == 0 && samlocals.DAC_mute[0] == 0 && samlocals.volume[0] > 0x80)
      l_vol = (int)(samlocals.volume[0] & 0x7f) * 100 / 0x7f;
    if (samlocals.mute[1] == 0 && samlocals.DAC_mute[1] == 0 && samlocals.volume[1] > 0x80)
      r_vol = (int)(samlocals.volume[1] & 0x7f) * 100 / 0x7f;
    mixer_set_stereo_volume(0, l_vol, 0);
    mixer_set_stereo_volume(1, 0, r_vol);

    samlocals.pass = 16;
    samlocals.value = 0;
  }
  if (data & 0x70000) LOG(("SET RTC: %x%x%x\n", (data >> 18) & 1, (data >> 17) & 1, (data >> 16) & 1));
}

/*****************************/
/*  Port map for SAM CPU    */
/*****************************/
//AT91 has only 1 port address it writes to - all 32 ports are sent via each bit of a 32 bit double word.
//However, if I didn't use 0-0xFF as a range it crashed for some reason.
static PORT_READ32_START(sam_readport)
	{ 0x00,0xFF, sam_port_r },
PORT_END

static PORT_WRITE32_START(sam_writeport)
	{ 0x00,0xFF, sam_port_w },
PORT_END

static MACHINE_INIT(sam) {
	at91_set_ram_pointers(sam_reset_ram, sam_page0_ram);
	at91_set_transmit_serial(sam_nodebus_transmit);
#ifdef SAM_USE_JIT
	if (options.at91jit)
	{
		at91_init_jit(0,(options.at91jit > 1) ? options.at91jit : 0x1080000);
	}
#endif
	#if LOG_RAW_SOUND_DATA
	fpSND = fopen("sam_snd.raw","wb");
	if(!fpSND)
		LOG(("Unable to create sam_snd.raw file\n"));
	#endif

	// Initialize outputs
	// Force physical output emulation since we use them to compute solenoids
	options.usemodsol |= CORE_MODOUT_FORCE_ON;
	coreGlobals.nGI = 1;
	coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
	coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
	core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_BULB_44_18V_DC_SE);
	// For auxiliary LEDs board (LED fading level is directly sent through serial link, no integrator needed, just use it)
	core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, CORE_MODOUT_LAMP_MAX - CORE_MODOUT_LAMP0 - 80, CORE_MODOUT_NONE);
	core_set_pwm_output_type(CORE_MODOUT_GI0, coreGlobals.nGI, CORE_MODOUT_BULB_44_5_7V_AC);
	core_set_pwm_output_type(CORE_MODOUT_SOL0, 32, CORE_MODOUT_SOL_2_STATE); // Base 32 solenoid outputs
	core_set_pwm_output_type(CORE_MODOUT_SOL0 + SAM_FASTFLIPSOL - 1, 1, CORE_MODOUT_NONE); // GameOn fake solenoid
	core_set_pwm_output_type(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL - 1, 16, CORE_MODOUT_SOL_2_STATE); // Aux board
	// Flipper use a 12ms PWM duty cycle so we try to limit the latency added by the fact we do not support PWM solenoids for the time being (it impacts flipper tricks)
	// FIXME: we set this for solenoids 13/14 which are upper flippers but not for all tables (this is not declared in the hardware)
	for (int ii = SAM_SOL_FLIPSTART; ii <= SAM_SOL_FLIPEND; ii++)
		coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + ii - 1].state.sol.switchDownLatency = 0.014f;
	// Game specific hardware
	const struct GameDriver* rootDrv = Machine->gamedrv;
	while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
		rootDrv = rootDrv->clone_of;
	const char* const gn = rootDrv->name;
	// Missing definitions:
	// - Simpsons Kooky Carnival
	if (strncasecmp(gn, "acd_", 4) == 0) { // AC DC
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // LED flasher
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
		sam_init_nodeboard(SAM_NB_ACDC_METALLICA);
	}
	else if (strncasecmp(gn, "avs_", 4) == 0) { // Avengers
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 27 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper 3 LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "avr_", 4) == 0) { // Avatar
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 27 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper 3 LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "bbh_", 4) == 0) { // Big Buck Hunter
		// Did not find a complete manual anywhere (even Stern's downloads do not have the schematics/solenoids) so this is from the VPX table, completed with the backglass flasher map
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "bdk_", 4) == 0) { // Batman The Dark Knight
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 27 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 29 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 32 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 29, 1, CORE_MODOUT_NONE);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 45, 1, CORE_MODOUT_NONE);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 59, 1, CORE_MODOUT_LED_STROBE_1_10MS);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60, 1, CORE_MODOUT_LED_STROBE_1_10MS);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, 8, CORE_MODOUT_LED); // LEDs on opto board
	}
	else if (strncasecmp(gn, "csi_", 4) == 0) { // CSI
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC); // Note that #22 is 2 #89 under playfield and 1 #161 above playfield #161 is 12V
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, 8, CORE_MODOUT_LED); // LEDs on opto board
	}
	else if (strncasecmp(gn, "fg_", 3) == 0) { // Family Guy [TODO crash in AT91 jit]
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, 3 * 2 * 8, CORE_MODOUT_LED_STROBE_8_16MS); // Mini playfield LEDs
	}
	else if (strncasecmp(gn, "ij4_", 4) == 0) { // Indiana Jones
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 27 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper 3 LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 54 - 1, 2, CORE_MODOUT_LED); // Ark leds (2 boards)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 55 - 1, 2, CORE_MODOUT_LED); // Swordman
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 56 - 1, 2, CORE_MODOUT_LED); // Skull
	}
	else if (strncasecmp(gn, "im_", 3) == 0) { // Iron Man
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 4, CORE_MODOUT_LED); // Led Flasher (Iron Man Vault Edition)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_LED); // Led Flasher (Iron Man Vault Edition)
	}
	else if (strncasecmp(gn, "mtl_", 4) == 0) { // Metallica LE
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL + 6 - 1, 1, CORE_MODOUT_PULSE); // Coffin board mode bit #0
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + CORE_FIRSTCUSTSOL + 7 - 1, 1, CORE_MODOUT_PULSE); // Coffin board mode bit #1
		sam_init_nodeboard(SAM_NB_ACDC_METALLICA);
	}
	else if (strncasecmp(gn, "mt_", 3) == 0) { // Mustang LE
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		sam_init_nodeboard(SAM_NB_MUSTANG);
	}
	else if (strncasecmp(gn, "nba_", 4) == 0) { // NBA
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 13 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "potc_", 5) == 0) { // Pirates of the Caribbean
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 27 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper 3 LEDs
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 24 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: 'H' LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 32 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: 'E' LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 40 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: 'A' LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 48 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: 'R' LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 56 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: 'T' LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 64 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: Heart Chest double LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 72 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Board 520-5258-00: Heart Chest double LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "rsn_", 4) == 0) { // Rolling Stones
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60 - 1, 3, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "scarn", 5) == 0) { // Simpsons Kooky Carnival
		// TODO no manual found
	}
	else if (strncasecmp(gn, "shr_", 4) == 0) { // Shrek
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 61 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper 2 LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, 3*2*8, CORE_MODOUT_LED_STROBE_8_16MS); // Mini playfield LEDs
	}
	else if (strncasecmp(gn, "sman_", 5) == 0) { // Spider-Man
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60 - 1, 3, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "smanve_", 7) == 0) { // Spider-Man Vault Edition
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "st_", 3) == 0) { // Star Trek LE
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		sam_init_nodeboard(SAM_NB_STARTREK);
	}
	else if (strncasecmp(gn, "tf_", 3) == 0) { // Transformers
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 58 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Megatron
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60 - 1, 3, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "trn_", 4) == 0) { // Tron Legacy
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 100, 6, CORE_MODOUT_LED); // Ramp RGB LEDs
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
		if (Machine->gamedrv->name[strlen(Machine->gamedrv->name)-1] == 'h')
			core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // Limited Edition: All LED
		else
			core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60 - 1, 3, CORE_MODOUT_LED_STROBE_1_10MS); // Pro: #555 except for bumper LEDs
	}
	else if (strncasecmp(gn, "twd_", 4) == 0) { // The Walking Dead LE
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		sam_init_nodeboard(SAM_NB_TWD);
	}
	else if (strncasecmp(gn, "twenty4_", 8) == 0) { // 24
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	else if (strncasecmp(gn, "wof_", 4) == 0) { // Wheel of Fortune
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 60 - 1, 3, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LEDs
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80, 48, CORE_MODOUT_LED); // Wheel LEDs
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 140, 175, CORE_MODOUT_LED_STROBE_1_5MS); // Mini DMD (175 LEDs)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_LED); // 4 LED flasher at the back of the wheel
	}
	else if (strncasecmp(gn, "wpt_", 4) == 0) { // World Poker Tour
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 62 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 70 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LED
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 78 - 1, 1, CORE_MODOUT_LED_STROBE_1_10MS); // Bumper LED
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 80 - 1, 49 * 10, CORE_MODOUT_LED_STROBE_1_10MS); // 14 Block LED (actually 2ms strobe every 20ms, but are they really faded ?)
	}
	else if (strncasecmp(gn, "xmn_", 4) == 0) { // X-Men
		core_set_pwm_output_type(CORE_MODOUT_LAMP0, 80, CORE_MODOUT_LED_STROBE_1_10MS); // All LED (Looks nicer with bulbs, but it really has LEDs)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
	}
	// Defaults to 2 state legacy integrator for better backward compatibility
	if ((options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL)) == 0)
		for (int i = 0; i < coreGlobals.nSolenoids; i++)
			if (coreGlobals.physicOutputState[i].type != CORE_MODOUT_SOL_2_STATE)
				core_set_pwm_output_type(CORE_MODOUT_SOL0, 1, CORE_MODOUT_LEGACY_SOL_2_STATE);
}

void sam_init(void)
{
	const char * const gn = Machine->gamedrv->name;

	memset(&samlocals, 0, sizeof(samlocals));
	samlocals.pass = 16;
	samlocals.coindoor = 1;

	//!! timing hacks for CSI and IJ
	if (strncasecmp(gn, "csi_", 4) == 0 || strncasecmp(gn, "ij4_", 4) == 0)
		at91_block_timers = 1;
	else
		at91_block_timers = 0;

	// Fast flips support.   My process for finding these is to load them in pinmame32 in VC debugger.
	// Load the balls in trough (E+SDFG(maybe also HJ)), start the game.
	// Use CheatEngine to find byte memory locations that are 1.
	// Enter service menu.  Use cheatengine to find memory locations that are 0.
	// Further go down the menu until you end up within the first switch test.  Search for 1.
	// Back one level.  Search for 0.
	// Exit service menu, *quickly* search for items that changed back to 1.
	// Enter service menu once again.  Use cheat engine to poke "1" into 0 values until you find the one
	// that lets solenoid 15 (left flipper) fire when you hit the left shift key (usually its near to the last entry).
	// Now you know the dynamic memory address - break PinMame32 in VC debugger.
	// Add a memory/data breakpoint (1 byte) on that address (needs another active breakpoint to allow for that).  Resume debugging.  Exit/enter the service menu,
	// breakpoint will hit.  Walk back up the stack one step to find the arm7core write.  This will have the
	// arm7 memory addr as the parameter.  This is the value you need here.

	if (strncasecmp(gn, "trn_174h", 8) == 0)
		samlocals.fastflipaddr = 0x0107adc2; 
	else if (strncasecmp(gn, "trn_174", 7) == 0) // and/or 17402?
		samlocals.fastflipaddr = 0x0107614a;
	else if (strncasecmp(gn, "acd_168h", 8) == 0)
		samlocals.fastflipaddr = 0x0107cd82;
	else if (strncasecmp(gn, "acd_170h", 8) == 0)
		samlocals.fastflipaddr = 0x0107af62;
	else if (strncasecmp(gn, "acd_170", 7) == 0)
		samlocals.fastflipaddr = 0x0106e1b6;
	else if (strncasecmp(gn, "mtl_170h", 8) == 0)
		samlocals.fastflipaddr = 0x0107f646;
	else if (strncasecmp(gn, "mtl_180h", 8) == 0)
		samlocals.fastflipaddr = 0x0107f676;
	else if (strncasecmp(gn, "mtl_180", 7) == 0)
		samlocals.fastflipaddr = 0x01078e52;
	else if (strncasecmp(gn, "twd_160h", 8) == 0)
		samlocals.fastflipaddr = 0x0107f7d2;
	else if (strncasecmp(gn, "twd_160", 7) == 0)
		samlocals.fastflipaddr = 0x01074d4a;
	else if (strncasecmp(gn, "wof_602h", 8) == 0)
		samlocals.fastflipaddr = 0x0107050a;
	else if (strncasecmp(gn, "wof_500", 7) == 0)
		samlocals.fastflipaddr = 0x0106e7e6;
	else if (strncasecmp(gn, "st_161h", 7) == 0)
		samlocals.fastflipaddr = 0x0107d7a2;
	else if (strncasecmp(gn, "st_162h", 7) == 0)
		samlocals.fastflipaddr = 0x01079aaa;
	else if (strncasecmp(gn, "st_162", 6) == 0)
		samlocals.fastflipaddr = 0x01075d6a;
	else if (strncasecmp(gn, "xmn_151h", 8) == 0)
		samlocals.fastflipaddr = 0x0107b222;
	else if (strncasecmp(gn, "xmn_151", 7) == 0)
		samlocals.fastflipaddr = 0x0107680a;
	else if (strncasecmp(gn, "fg_1200", 7) == 0)
		samlocals.fastflipaddr = 0x010681d6;
	else if (strncasecmp(gn, "potc_600", 8) == 0)
		samlocals.fastflipaddr = 0x0105a7fe;
	else if (strncasecmp(gn, "im_183ve", 8) == 0)
		samlocals.fastflipaddr = 0x01055bf6;
	else if (strncasecmp(gn, "im_185ve", 8) == 0)
		samlocals.fastflipaddr = 0x01053ed6;
	else if (strncasecmp(gn, "im_186ve", 8) == 0)
		samlocals.fastflipaddr = 0x01053fee;
	else if (strncasecmp(gn, "im_185", 6) == 0)
		samlocals.fastflipaddr = 0x01068bde;
	else if (strncasecmp(gn, "im_186", 6) == 0)
		samlocals.fastflipaddr = 0x01068cf6;
	else if (strncasecmp(gn, "avr_120h", 8) == 0)
		samlocals.fastflipaddr = 0x01069772;
	else if (strncasecmp(gn, "avr_200", 7) == 0)
		samlocals.fastflipaddr = 0x01056afa;
	else if (strncasecmp(gn, "avs_170h", 8) == 0)
		samlocals.fastflipaddr = 0x0106dada;
	else if (strncasecmp(gn, "avs_170", 7) == 0)
		samlocals.fastflipaddr = 0x0106db1e;
	else if (strncasecmp(gn, "wpt_140a", 8) == 0)
		samlocals.fastflipaddr = 0x01075712;
	else if (strncasecmp(gn, "tf_180h", 7) == 0)
		samlocals.fastflipaddr = 0x0107472e;
	else if (strncasecmp(gn, "tf_180", 6) == 0)
		samlocals.fastflipaddr = 0x0106ea72;
	else if (strncasecmp(gn, "bdk_294", 7) == 0)
		samlocals.fastflipaddr = 0x010791be;
	else if (strncasecmp(gn, "shr_141", 7) == 0)
		samlocals.fastflipaddr = 0x01068fde;
	else if (strncasecmp(gn, "bbh_170", 7) == 0)
		samlocals.fastflipaddr = 0x0106acae;
	else if (strncasecmp(gn, "sman_261", 8) == 0)
		samlocals.fastflipaddr = 0x0107d89e;
	else if (strncasecmp(gn, "smanve_101", 10) == 0)
		samlocals.fastflipaddr = 0x0106d61e;
	else if (strncasecmp(gn, "nba_802", 7) == 0)
		samlocals.fastflipaddr = 0x010609be;
	else if (strncasecmp(gn, "rsn_110h", 8) == 0)
		samlocals.fastflipaddr = 0x01070716;
	else if (strncasecmp(gn, "rsn_110", 7) == 0)
		samlocals.fastflipaddr = 0x0106fb3a;
	else if (strncasecmp(gn, "csi_240", 7) == 0)
		samlocals.fastflipaddr = 0x0106475e;
	else if (strncasecmp(gn, "ij4_210", 7) == 0)
		samlocals.fastflipaddr = 0x01072dea;
	else if (strncasecmp(gn, "twenty4_150", 11) == 0)
		samlocals.fastflipaddr = 0x0106ec1e;
	else if (strncasecmp(gn, "mt_145hb", 8) == 0)
		samlocals.fastflipaddr = 0x01077a92;
	else if (strncasecmp(gn, "mt_145h", 7) == 0)
		samlocals.fastflipaddr = 0x01077b82;
	else if (strncasecmp(gn, "mt_145", 6) == 0)
		samlocals.fastflipaddr = 0x01070912;
	//else if (strncasecmp(gn, "scarn200", 8) == 0)
	//	samlocals.fastflipaddr = ;
}


static MACHINE_RESET(sam1) {
	sam_init();
	samlocals.samVersion = 1;
}

static MACHINE_RESET(sam2) {
	sam_init();
	samlocals.samVersion = 2;
}

static MACHINE_STOP(sam) {
	#if LOG_RAW_SOUND_DATA
	if(fpSND) fclose(fpSND);
	fpSND = NULL;
	#endif
}

static SWITCH_UPDATE(sam) {
	if (inports) {
		// 1111      .... checking 0x000X and putting into column 9
		/*Switch Col 9 = Coin Slots*/
		CORE_SETKEYSW(inports[SAM_COMINPORT], 0x0f, 9);

		// 1111 1111 .... checking 0x0XX0 and putting those into column 0
		/*Switch Col 0 = Tilt, Slam, Coin Door Buttons*/
		CORE_SETKEYSW(inports[SAM_COMINPORT] >> 4, 0xff, 0);

		// 1100 0000 .... checking 0x8000 and 0x4000 and setting switch column 2
		/*Switch Col 2 = Start */
		CORE_SETKEYSW(inports[SAM_COMINPORT] >> 8, 0xc0, 2);

		// 0x1000(coin door) >> 12
		/*Coin Door Switch - Not mapped to the switch matrix*/
		samlocals.coindoor = (inports[SAM_COMINPORT] & 0x1000) ? 0 : 1;
	}
}

// -- High level emulation of nodeboards --
// 
//   This is built from reading main gamecode, as well as nodeboard firmware and schematics
//
//   The 5 first games using these boards were more or less beta version from ACDC & Metallica which have a very basic 
//   protocol, to TWD and Mustang which implement most of it and are very similar to Spike nodeboards.
// 
// - ACDC LE (2012) tested with acd_170h
// - Metallica Premium Monsters (2013) tested with mtl_180h
//   Board 520-5331-00: Grinder Multi-Color LED driver
//     Single board with the bridge and the single child node into the same little controller (ATtiny4313-SU), driving 4 GI outputs and
//     some LED drivers. Does not implement the full communication protocol (see below)
// 
// - Star Trek LE (2013) tested with st_162h
//   Board 520-6812-00: top board, bridge and child board 5
//   Board 520-6811-00: bill Premium / LE Left, child board 6
//   Board 520-6808-00: bill Premium / LE Center, child board 7
//   Board 520-5322-00: 32 LEDs, 32 switch, child board 8
//   Implements the communication protocol but Star Trek gamecode just sends LED state of the 4 nodeboards every 32ms
// 
// - Mustang LE (2014) tested with mt_145h
//   Board 520-6822-00 Board 5: Bridge (ATtiny2313A-SU) and child (LPC1112FHN33/xxx) on the same board
//   Gamecode runs a little startup sequence to validate bridge communication and setup nodeboards then simply send Led states
// 
// - The Walking Dead LE (2014) tested with twd_160h
//   Board 520-6937-00: bridge & child Board 3 (10 onboard LEDs, 14 driven outputs)
//   Board 520-5322-00: 32 LEDs, 32 switch, child board 8
//   TWD gamecode runs a startup sequence with bridge setup (03/05 broadcasts), then nodeboards setup (F1/FE/FF/FB messages), 
//   then sends LED data at a 16ms pace (no fading, 2 messages per board)
// 
//   The architecture is designed around the Main CPU, a very simple 'Bridge node' and several 'Child nodes'.
//   The bridge node is often located on a board that also has a child node. Bridge is a very basic function
//   built with an ATtiny2313A controller which makes the bridge between CPU (simple point to point communication)
//   and child nodes (RS485 multi drop communication that support addressing messages).
// 
//   Child Nodeboard firmwares are partially available in main CPU ROM likely used to update the nodeboards.
//   But when updating the nodeboard firmware, the CPU only writes to the flash ROM after the first 0x1000 bytes.
//   These 4Kb of memory contains static information which is likely flashed in factory and never changed.
//   Therefore values for these are guessed by reverse engineering what the CPU expects when reading. These
//   could also be obtained from real boards, or exploiting the 0xF5 GetChecksum command but noone has done
//   that yet.
//
//   The child nodeboards use LPC11xx/LPC13xx controllers, for which there has been an attempt to reverse engineer
//   the boot loader here: https://github.com/domenpk/lpc13xx_boot_analysis/blob/master/boot_disassembly.txt
//   This boot loader is 16kb mapped at 0x1fff0000
//
// Communication protocol between CPU and bridge is the following:
//   CPU->Bridge command:
//    . Bridge command (Bit7=0, Bit6/5=unknown, Bit0..4=command)
//    . Payload Size (number of bytes after this one)
//    . Payload Data
//   CPU->Bridge to broadcast to node:
//    . Node address (Bit7=1), Bit4..6=unknown, Bit0..3=board address)
//    . Payload Size (number of bytes after this one)
//    . Payload Data
//    . Payload Data Checksum (TWD only)
//    . Expected Response Size (TWD only)
//   Bridge->CPU: Data then Checksum
//    . CPU and Bridge know the length of the response so the protocol is just the corresponding data with a (negative) checksum
// 
// Communication between bridge and child nodeboards is not emulated here since the high level emulation is done at the bridhe node.

#define SAM_NB_TYPE_520_5331_00           0 // Metallica Premium Monsters, Grinder Multi-Color LED driver
#define SAM_NB_TYPE_520_6937_10           1 // TWD with a LPC1112HN33/101 (schematics in the TWD LE manual, using firmware included in the main CPU gamecode)
#define SAM_NB_TYPE_520_5322_10           2 // TWD, 32 LEDs, 32 switch, child board 8
#define SAM_NB_TYPE_520_5322_20           3 // Star Trek, 32 LEDs, 32 switch, child board 8
#define SAM_NB_TYPE_520_6808_00           4 // Star Trek, bill Premium / LE Center, child board 7
#define SAM_NB_TYPE_520_6811_00           5 // Star Trek, bill Premium / LE Left, child board 6
#define SAM_NB_TYPE_520_6812_00           6 // Star Trek, top board, bridge and child board 5
#define SAM_NB_TYPE_520_6822_00           7 // Mustang with a LPC1112HN33/101 (schematics in the Mustang LE manual, using firmware included in the main CPU gamecode)

#define SAM_NB_STATUS_RESET_POR           0x0001 // Last reset cause: reset from POR (power on reset)
#define SAM_NB_STATUS_RESET_WDG           0x0004 // Last reset cause: watchdog (or F1 command)
#define SAM_NB_STATUS_RESET_BOD           0x0008 // Last reset cause: reset from BOD (brown-out detect: supply voltage went below minimum causing a reset)
#define SAM_NB_STATUS_UART_OVERRUN        0x0010 // UART overrun error
#define SAM_NB_STATUS_UART_FRAMING_ERR    0x0020 // UART framing eroor
#define SAM_NB_STATUS_UART_BREAK_INT      0x0040 // UART break interrupt
#define SAM_NB_STATUS_FAULT               0x0080 // Fault on board output (/FAULT signal)
#define SAM_NB_STATUS_MSG_LENGTH_OVERFLOW 0x0100 // Message length overflow (message is too long)
#define SAM_NB_STATUS_MSG_COUNT_OVERFLOW  0x0400 // Message count overflow (there are too many messages pending)
#define SAM_NB_STATUS_ISENSE              0x1000 // ISense signal (ADC comparator)
#define SAM_NB_STATUS_MSG_CHECKSUM_ERROR  0x2000 // Message received with invalid checksum
#define SAM_NB_STATUS_UNK1                0x4000 // 
#define SAM_NB_STATUS_UNK2                0x8000 // 

#define LOG_NODEBOARD 0

typedef struct sam_nodeboard {
	UINT8        address;
	int          type;
	UINT32       lpcPartId;     // LPC11xx part ID see LPC11 manual section 26.5.11 Part Identification number for a comprehensive list
	UINT32       status;
	UINT32       nMsgReceived;
	int          ledCount;
	int          ledMap[64];
} sam_nodeboard_t;

struct {
	// Nodebus setup
	int             bridge;
	int             bridgeVersionId;
	sam_nodeboard_t nodeboards[16];
	// Buffer for message sent by the main CPU board
	int             rcvMsgPos;
	UINT8           rcvChecksum;
	UINT8           rcvMsg[256];
	// Buffer for messages sent from nodeboards
	int             sendMsgPos;
	UINT8           sendChecksum;
	UINT8           sendMsg[1024];
} nblocals;

static void sam_set_nodeboard(UINT8 address, int type);

static void sam_init_nodeboard(int bridge) {
	memset(&nblocals, 0, sizeof(nblocals));
	nblocals.bridge = bridge;
	switch (bridge)
	{
	case SAM_NB_ACDC_METALLICA:
		// Not really a nodeboard since it uses a dedicated, non addressed (single board) communication protocol
		sam_set_nodeboard(0, SAM_NB_TYPE_520_5331_00);
		// The 48 Leds used to be mapped to 81..128, and the 4 GI strings to 130, 132, 134, 136
		for (int i = 0; i < 48; i++)
			nblocals.nodeboards[0].ledMap[i] = CORE_MODOUT_LAMP0 + 80 + i;
		break;
	case SAM_NB_STARTREK:
		sam_set_nodeboard(5, SAM_NB_TYPE_520_6812_00);
		sam_set_nodeboard(6, SAM_NB_TYPE_520_6811_00);
		sam_set_nodeboard(7, SAM_NB_TYPE_520_6808_00);
		sam_set_nodeboard(8, SAM_NB_TYPE_520_5322_20);
		// Led mapping for backward compatibility
		for (int i = 0; i < 64; i++) {
			nblocals.nodeboards[5].ledMap[i] = CORE_MODOUT_LAMP0 + 80       + i;
			nblocals.nodeboards[6].ledMap[i] = CORE_MODOUT_LAMP0 + 80 +  65 + i;
			nblocals.nodeboards[7].ledMap[i] = CORE_MODOUT_LAMP0 + 80 + 130 + i;
			nblocals.nodeboards[8].ledMap[i] = CORE_MODOUT_LAMP0 + 80 + 195 + i;
		}
		break;
	case SAM_NB_MUSTANG:
		nblocals.bridgeVersionId = 0x000805;
		sam_set_nodeboard(5, SAM_NB_TYPE_520_6822_00);
		for (int i = 0; i < 64; i++)
			nblocals.nodeboards[5].ledMap[i] = CORE_MODOUT_LAMP0 + 80 + i;
		break;
	case SAM_NB_TWD:
		nblocals.bridgeVersionId = 0x000d05;
		// Board #3 drives directly 18 onboard Leds as well as 14 outputs, and additional Leds through chained serial boards 3A, 3B, 3C
		// Board #8 drives directly 32 outputs, and additional Leds through chained serial boards 8A, 8B, 8C, 8D
		// Chained serial Led boards are:
		// - 520-6827-00A (6 onboard Leds, 2 outputs) for board 3A, 3C, 8A, 8B, 8C
		// - 520-6829-00A (6 onboard Leds, 2 outputs) for board 3B
		// - 520-6830-00A (7 onboard Leds, 1 outputs) for board 8D
		sam_set_nodeboard(3, SAM_NB_TYPE_520_6937_10);
		sam_set_nodeboard(8, SAM_NB_TYPE_520_5322_10);
		// Backward compatible light number mapping:
		for (int i = 0; i < 32; i++) {
			nblocals.nodeboards[3].ledMap[i     ] = CORE_MODOUT_LAMP0 +  81 + i;
			nblocals.nodeboards[8].ledMap[i     ] = CORE_MODOUT_LAMP0 + 116 + i;
			nblocals.nodeboards[3].ledMap[i + 32] = CORE_MODOUT_LAMP0 + 151 + i;
			nblocals.nodeboards[8].ledMap[i + 32] = CORE_MODOUT_LAMP0 + 186 + i;
		}
		break;
	}
}

static void sam_set_nodeboard(UINT8 address, int type) {
	nblocals.nodeboards[address].address = address;
	nblocals.nodeboards[address].type = type;
	nblocals.nodeboards[address].status = SAM_NB_STATUS_RESET_POR;
	nblocals.nodeboards[address].lpcPartId = 0x042D502B; // LPC1112FHN33/101, part id can be either 0x042D502B or 0x2524D02B
	//nblocals.nodeboards[address].lpcPartId = 0x2524D02B; // LPC1112FHN33/101, part id can be either 0x042D502B or 0x2524D02B
	if (type == SAM_NB_TYPE_520_5331_00)
		nblocals.nodeboards[address].ledCount = 48; // Metallica
	else
		nblocals.nodeboards[address].ledCount = 64; // All other boards drive 64 outputs (including serial childs)
	for (int i = 0; i < 64; i++)
		nblocals.nodeboards[address].ledMap[i] = i;
}

static void sam_nodebus_send_byte(UINT8 data) {
	nblocals.sendMsg[nblocals.sendMsgPos] = data;
	nblocals.sendMsgPos++;
	nblocals.sendChecksum = nblocals.sendChecksum - data;
}

static void sam_nodebus_send_word(UINT16 data) {
	sam_nodebus_send_byte(data & 0xFF);
	sam_nodebus_send_byte((data >> 8) & 0xFF);
}

static void sam_nodebus_send_dword(UINT32 data) {
	sam_nodebus_send_byte( data        & 0xFF);
	sam_nodebus_send_byte((data >>  8) & 0xFF);
	sam_nodebus_send_byte((data >> 16) & 0xFF);
	sam_nodebus_send_byte((data >> 24) & 0xFF);
}

static void sam_nodebus_send_flush(sam_nodeboard_t* nodeboard, int reqResponseSize) {
	#if LOG_NODEBOARD
		printf("%6.3f   ", timer_get_time());
		for (int i = 0; i < nblocals.sendMsgPos; i++)
			printf("%02x ", nblocals.sendMsg[i]);
	#endif

	// Response checksum
	// TWD only ? also for other nodeboards ?
	if ((nblocals.bridge == SAM_NB_TWD) && nodeboard) {
		nblocals.sendMsg[nblocals.sendMsgPos] = nblocals.sendChecksum;
		nblocals.sendMsgPos++;
		#if LOG_NODEBOARD
			printf("[Chk = %02x] ", nblocals.sendMsg[nblocals.sendMsgPos - 1]);
		#endif
	}

	// Status of response (added by bridge)
	// ....XX.. => error flags (set by bridge, unknown reasons yet: invalid response length? com error with child ?...)
	// 1....... => flag that the byte contains a valid bridge status (to be reused instead of sending a GetBridgeStatus request)
	// .???..?? => unknown status flags
	if (nodeboard)
	{
		nblocals.sendMsg[nblocals.sendMsgPos] = 0x80;
		nblocals.sendMsgPos++;
		#if LOG_NODEBOARD
			printf("[Status = %02x] ", nblocals.sendMsg[nblocals.sendMsgPos-1]);
		#endif
	}

	// TWD bridge only forward valid response, or set an error flags in the status byte
	assert((reqResponseSize == -1) || (reqResponseSize = nblocals.sendMsgPos));

	#if LOG_NODEBOARD
		printf("\n");
	#endif

	int remaining = at91_receive_serial(0, nblocals.sendMsg, nblocals.sendMsgPos);
	#if LOG_NODEBOARD
		if (remaining)
			printf("Transmit failed: %d remaining bytes\n", remaining);
	#endif
	nblocals.sendMsgPos = 0;
	nblocals.sendChecksum = 0;
}

static void sam_nodebus_msg_received()
{
	int msgLength;
	if (nblocals.bridge == SAM_NB_ACDC_METALLICA)
		// Metallica has a very simple protocol: either one of the GI output address+state, or the 48 LED string + command + values
		msgLength = (nblocals.rcvMsg[0] == 0x80) ? (2 + 48) : 2;
	else
		// Size = Cmd + Payload Size + Payload. Broadcasted messages have an additional expected response length byte which is not included in the payload length
		msgLength = ((nblocals.rcvMsg[0] & 0x80) != 0) ? (2 + nblocals.rcvMsg[1] + 1) : (2 + nblocals.rcvMsg[1]);
	
	// Bridge command
	if ((nblocals.rcvMsg[0] & 0x80) == 0) {
		#if LOG_NODEBOARD
			printf("%6.3f > ", timer_get_time());
			for (int i = 0; i < msgLength; i++)
				printf("%02x ", nblocals.rcvMsg[i]);
			printf("[Bridge Cmd]\n");
		#endif
		switch (nblocals.rcvMsg[0])
		{
		case 0x03: // GetBridgeVersion
			sam_nodebus_send_byte((nblocals.bridgeVersionId >> 16) & 0xFF);
			sam_nodebus_send_byte((nblocals.bridgeVersionId >>  8) & 0xFF);
			sam_nodebus_send_byte( nblocals.bridgeVersionId        & 0xFF);
			sam_nodebus_send_flush(NULL, -1);
			break;
		case 0x05: // GetBridgeStatus
			sam_nodebus_send_byte(0x80); //
			sam_nodebus_send_flush(NULL, -1);
			break;
		}
	}
	// Nodeboard addressed command
	// These commands are still processed by the bridge that will broadcast them using RS485 multi drop
	// addressing mode, and will buffer the response until the requested size is reached.
	else
	{
		#if LOG_NODEBOARD
			printf("%6.3f > ", timer_get_time());
			for (int i = 0; i < msgLength - 1; i++)
				if ((nblocals.bridge == SAM_NB_TWD) && (i == msgLength - 2))
					printf("[Chk = %02x] ", nblocals.rcvMsg[i]);
				else
					printf("%02x ", nblocals.rcvMsg[i]);
			if (nblocals.bridge == SAM_NB_ACDC_METALLICA)
				printf("%02x ", nblocals.rcvMsg[msgLength - 1]);
			else
				printf("[Response Length = %d]", nblocals.rcvMsg[msgLength - 1]);
			printf("\n");
		#endif

		if (nblocals.bridge == SAM_NB_ACDC_METALLICA) {
			UINT8 cmd = nblocals.rcvMsg[0];
			switch (cmd & 0xF0)
			{
			case 0x80:
				// 48 outputs driving 16 RGB Leds. Intensity is expressed as 0..100 (so 101 levels)
				// nblocals.rcvMsg[1] is likely a fade command but I have only witnessed 0x00 there
				for (int i = 0; i < 48; i++)
					coreGlobals.physicOutputState[nblocals.nodeboards[0].ledMap[i]].value = nblocals.rcvMsg[2 + i] / 100.f;
				break;
			case 0x90:
				// GI Outputs, for backward compatibility we map them to lamps 130/132/134/136, intensity goes from 0..50 (51 levels)
				coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + 129 + ((cmd & 0x0F) * 2)].value = nblocals.rcvMsg[1] / 50.f;
				break;
			}
		}
		else {
			UINT8 address = nblocals.rcvMsg[0] & 0x7F;
			if (address > 15) {
				assert(FALSE); // Invalid address (should be 4 bits only)
				return;
			}

			sam_nodeboard_t* nodeboard = &nblocals.nodeboards[address];
			if (nodeboard->address != address) {
				assert(FALSE); // missing nodeboard
				return;
			}
			nodeboard->nMsgReceived++;

			int reqResponseSize = nblocals.rcvMsg[msgLength - 1];

			UINT8 payload = nblocals.rcvMsg[1];
			if (payload == 0) {
				assert(FALSE); // No payload => invalid as we do not have any information beside the nodeboard address
				return;
			}

			if ((nblocals.bridge == SAM_NB_TWD) && (nblocals.rcvChecksum != nblocals.rcvMsg[msgLength - 1])) { // Requested response length is not part of the checksum
				assert(FALSE); // Invalid checksum
				return;
			}

			UINT8 cmd = nblocals.rcvMsg[2];
			if ((cmd & 0xC0) == 0x80) {
				// 0x80/0x90/0xA0/0xB0: Set/Fade LED command
				int startLedIndex = cmd & 0x3F;
				if ((nblocals.bridge == SAM_NB_STARTREK) || (nblocals.bridge == SAM_NB_MUSTANG) ) {
					int ledCount = payload - 1;
					for (int i = 0; i < ledCount; i++)
						coreGlobals.physicOutputState[nodeboard->ledMap[startLedIndex + i]].value = nblocals.rcvMsg[3 + i] / 255.f;
				}
				else {
					int ledCount = payload - 3;
					// TODO implement fadeMode to get smoother light shading (320Hz fading & PWM, instead of update at communication rate which seems to vary from 30 to 60Hz)
					UINT8 fadeMode = nblocals.rcvMsg[3];
					if (fadeMode == 0xFF) { // Per Led fade mode
						ledCount /= 2;
						assert((ledCount * 2) == (payload - 3));
						for (int i = 0; i < ledCount; i++)
							// nblocals.rcvMsg[4 + i * 2 + 0] is the fade mode per Led (not implemented)
							coreGlobals.physicOutputState[nodeboard->ledMap[startLedIndex + i]].value = nblocals.rcvMsg[4 + i * 2 + 1] / 255.f;
					}
					else { // Global fade mode
						for (int i = 0; i < ledCount; i++)
							coreGlobals.physicOutputState[nodeboard->ledMap[startLedIndex + i]].value = nblocals.rcvMsg[4 + i] / 255.f;
					}
				}
			}
			else {
				switch (cmd) {
				case 0x20: // SSPWriteAndTriggerRead: write provided output to external ouput serial port and trigger a serial port read
					break;
				case 0x21: // SSPRead: return the last data read after a SSPWriteAndTriggerRead
					break;
				case 0xF0: // ??
					break;
				case 0xF1: // Reset nodeboard
					nodeboard->nMsgReceived = 0;
					nodeboard->status = SAM_NB_STATUS_RESET_WDG;
					for (int i = 0; i < nodeboard->ledCount; i++)
						coreGlobals.physicOutputState[nodeboard->ledMap[i]].value = 0.f;
					break;
				case 0xF2: // ??
					sam_nodebus_send_byte(4);
					sam_nodebus_send_flush(nodeboard, reqResponseSize);
					break;
				case 0xF3: // ??
					break;
				case 0xF4: // Unimplemented in firmware
					break;
				case 0xF5: // GetChecksum
					// Unimplemented, performs a checksum of a requested area of the flash RAM
					break;
				case 0xF6: // Unimplemented in firmware
				case 0xF7:
				case 0xF9:
					break;
				case 0xF8: // GetBootStatus (seems unimplemented in TWD firmware but part of the boot sequence, handled by bridge ? not the right firmware ?)
					sam_nodebus_send_dword(0x00000001);
					sam_nodebus_send_flush(nodeboard, reqResponseSize);
					break;
				case 0xFA:
					break;
				case 0xFB: // Disable OC
					break;
				case 0xFC: // GetBoardID
					break;
				case 0xFD: // GetVersion
					break;
				case 0xFE: // GetBoardInfo
					//sam_nodebus_send_byte(nodeboard->address | 0x80);
					sam_nodebus_send_byte(nodeboard->address);
					sam_nodebus_send_byte((nblocals.bridgeVersionId >> 16) & 0xFF);
					sam_nodebus_send_byte((nblocals.bridgeVersionId >>  8) & 0xFF);
					sam_nodebus_send_byte( nblocals.bridgeVersionId        & 0xFF);
					sam_nodebus_send_dword(nodeboard->lpcPartId);
					sam_nodebus_send_byte(0x00); // Boot code version major (here from TWD 160h)
					sam_nodebus_send_byte(0x05); // Boot code version minor (here from TWD 160h)
					sam_nodebus_send_flush(nodeboard, reqResponseSize);
					break;
				case 0xFF: // GetAndResetStatus
					if (nodeboard->type == SAM_NB_TYPE_520_5322_10) {
						sam_nodebus_send_dword(nodeboard->nMsgReceived); // Increased before response
						sam_nodebus_send_dword(nodeboard->status);
					}
					else if (nodeboard->type == SAM_NB_TYPE_520_6822_00) {
						sam_nodebus_send_dword(nodeboard->nMsgReceived-1); // Increased after response
						sam_nodebus_send_word(nodeboard->status);
					}
					sam_nodebus_send_flush(nodeboard, reqResponseSize);
					nodeboard->status = 0;
					break;
				}
			}
		}
	}
}

static void sam_nodebus_transmit(int usartno, data8_t *data, int size)
{
	// Nodebus messages
	if (usartno == 0) {
		for (; size > 0; size--, data++) {
			UINT8 rcvByte = *data;
			nblocals.rcvChecksum += rcvByte;
			nblocals.rcvMsg[nblocals.rcvMsgPos] = rcvByte;
			int msgLength;
			if (nblocals.bridge == SAM_NB_ACDC_METALLICA)
				// Metallica has a very simple protocol: either one of the GI outputs (address + state), or the 48 LED string (address + command + states)
				msgLength = (nblocals.rcvMsg[0] == 0x80) ? (2 + 48) : 2;
			else
				// Broadcasted messages have an additional expected response length byte which is not included in the payload length
				msgLength = ((nblocals.rcvMsg[0] & 0x80) != 0) ? (2 + nblocals.rcvMsg[1] + 1) : (2 + nblocals.rcvMsg[1]);
			if (nblocals.rcvMsgPos == msgLength - 1) {
				sam_nodebus_msg_received();
				nblocals.rcvMsgPos = 0;
				nblocals.rcvChecksum = 0;
			}
			else {
				nblocals.rcvMsgPos++;
				if (nblocals.rcvMsgPos >= 256) {
					// Bug: we overflowed the reception buffer. We are likely out of sync and did not detect the start/end of a message.
					nblocals.rcvMsgPos = 0;
					nblocals.rcvChecksum = 0;
				}
			}
		}
	}
	// Console messages
	else if (usartno == 1) {
#if defined(VPINMAME)
		while(size--)
			dmddeviceFwdConsoleData(*(data++));
#elif defined(LIBPINMAME)
		libpinmame_forward_console_data(data, size);
#endif
	}
}


/******************************/
/*  Interface update Section  */
/******************************/
static INTERRUPT_GEN(sam_interface_update) {
	/*-------------------------------
	/  copy local data to interface
	/--------------------------------*/
	samlocals.vblankCount++;

	/*-- display --*/
	if ((samlocals.vblankCount % SAM_DISPLAYSMOOTH) == 0) {
		coreGlobals.diagnosticLed = samlocals.diagnosticLed;
		samlocals.diagnosticLed = 0;
	}

	/*-- GameOn for Fast Flips --*/
	if (samlocals.fastflipaddr > 0 && cpu_readmem32ledw(samlocals.fastflipaddr) == 0x01)
	{
		coreGlobals.solenoids2 = 0x10; // Not sure why we do that, but kept as it used to be what is done before 3.6
		coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + SAM_FASTFLIPSOL - 1].value = 1.0f;
	}
	else
	{
		coreGlobals.solenoids2 = 0x00; // Not sure why we do that, but kept as it used to be what is done before 3.6
		coreGlobals.physicOutputState[CORE_MODOUT_SOL0 + SAM_FASTFLIPSOL - 1].value = 0.0f;
	}
	core_updateSw(1);
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(sam) {
	core_nvram(file, read_or_write, nvram, 0x20000, 0xff);		//128K NVRAM
}

//Toggle Zero Cross bit
static void sam_timer(int data)
{
	samlocals.zc = !samlocals.zc;
	core_zero_cross();
}

static INTERRUPT_GEN(sam_irq)
{
	at91_fire_irq(AT91_FIQ_IRQ);
}

/*********************************************/
/* S.A.M. Generation #1 - Machine Definition */
/*********************************************/
static MACHINE_DRIVER_START(sam1)
    MDRV_IMPORT_FROM(PinMAME)
    MDRV_SWITCH_UPDATE(sam)
    MDRV_CPU_ADD(AT91, SAM_CPUFREQ) // AT91R40008
    MDRV_CPU_MEMORY(sam_readmem, sam_writemem)
    MDRV_CPU_PORTS(sam_readport, sam_writeport)
    MDRV_CPU_VBLANK_INT(sam_interface_update, 1)
    MDRV_CPU_PERIODIC_INT(sam_irq, SAM_IRQFREQ)
    MDRV_CORE_INIT_RESET_STOP(sam, sam1, sam)
    MDRV_DIPS(8)
    MDRV_NVRAM_HANDLER(sam)
    MDRV_TIMER_ADD(sam_timer, SAM_ZC_FREQ)
    MDRV_SOUND_ADD(CUSTOM, samCustInt)
    MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
    MDRV_DIAGNOSTIC_LEDH(2)
MACHINE_DRIVER_END

/*********************************************/
/* S.A.M. Generation #2 - Machine Definition */
/*********************************************/
static MACHINE_DRIVER_START(sam2)
  MDRV_IMPORT_FROM(sam1)
  MDRV_CORE_INIT_RESET_STOP(sam, sam2, sam)
MACHINE_DRIVER_END


#define INITGAME(name, gen, disp, lampcol, hw) \
	static core_tGameData name##GameData = { \
		gen, disp, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, lampcol, 16, 0, 0, hw,0, sam_getSol}}; \
	static void init_##name(void) { core_gameData = &name##GameData; }

/*****************/
/*  DMD Section  */
/*****************/

/*-- 
  SAM DMD display uses 32 x 128 pixels by accessing 0x1000 bytes per page.
  That's 8 bits for each pixel, but they are distributed into 4 brightness
  bits (16 colors), and 4 translucency bits that perform the masking of the
  secondary or "background" page that will "shine through" if the mask bits
  of the foreground are set.

  SAM rasterizer renders each line 4 times with different display lengths 
  to create shades. Each line is therefore made up of a pattern of 12 x 41.55us, 
  with the 4 planes corresponding to combination of 1 / 2 / 4 / 5 of these 12 
  time slots. The resulting frame rate is 1e6/(32x12x41.55) = 62.67Hz which 
  has been validated with real hardware measure. The flicker/fusion threshold 
  is supposed to be somewhere around 25-30Hz based on the fact that other 
  hardwares like GTS3 and WPC have PWM pattern around these frequencies.
  Therefore, to get the final luminance, we need to perform integration of at
  least the last 24 frames. For the time being we only apply a LUT corresponding
  to the 1 / 2 / 4 / 5 pattern.
--*/
static PINMAME_VIDEO_UPDATE(samdmd_update) {
	// This LUT suppose that each bitplane correspond to one of the frame, since the display length is 1 / 2 / 4 / 5,
	// RAM never contains 8/9/10/11 which creates a monotonic LUT, but with a discontinuity as the hardware has 13 shades while the code uses 12.
	static const UINT8 lumLUT[16] = { 0, 21, 43, 64, 85, 106, 128, 149, 106 /*unused*/, 128 /*unused*/, 149 /*unused*/, 170 /*unused*/, 191, 213, 234, 255};
	int ii;
	for( ii = 0; ii < 32; ii++ )
	{
		UINT8 *dotRaw = &coreGlobals.dmdDotRaw[ii * layout->length];
		UINT8 *dotLum = &coreGlobals.dmdDotLum[ii * layout->length];
		const UINT8* const offs1 = memory_region(REGION_CPU1) + 0x1080000 + (samlocals.video_page[0] << 12) + ii * 128;
		const UINT8* const offs2 = memory_region(REGION_CPU1) + 0x1080000 + (samlocals.video_page[1] << 12) + ii * 128;
		int jj;
		for( jj = 0; jj < 128; jj++ )
		{
			const UINT8 RAM1 = offs1[jj];
			const UINT8 RAM2 = offs2[jj];
			const UINT8 mix = RAM1 >> 4;
			const UINT8 temp = (RAM2 & mix) | (RAM1 & (mix^0xF)); //!! is this correct or is mix rather a multiplier/ratio/alphavalue??
			if ((mix != 0xF) && (mix != 0x0)) //!! happens e.g. in POTC in extra ball explosion animation: RAM1 values triggering this: 223, 190, 175, 31, 25, 19, 17 with RAM2 being always 0. But is this just wrong game data (as its a converted animation)?!
				LOG(("Special DMD Bitmask %01X RAM1=%02x RAM2=%02x pix@(%3dx%2d)", mix, RAM1, RAM2, jj, ii));
			*dotRaw++ = temp;
			*dotLum++ = lumLUT[temp];
		}
	}
	core_dmd_video_update(bitmap, cliprect, layout, NULL);
	return 0;
}

#define SAT_NYB(v) (UINT8)(15.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v))
#define SAT_BYTE(v) (UINT8)(255.0f * (v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v))

// Little 5x7 led matrix used in World Poker Tour (2 rows of 7 each)
static PINMAME_VIDEO_UPDATE(samminidmd_update) {
    const int dmd_x = (layout->left-10)/7;
    const int dmd_y = (layout->top-34)/9;
    assert(layout->length == 5);
    assert(layout->start == 7);
    assert(0 <= dmd_x && dmd_x < 7);
    assert(0 <= dmd_y && dmd_y < 2);
    for (int y = 0; y < 7; y++)
		for (int x = 0; x < 5; x++) {
			const int target = 10 * 8 + (dmd_y * 5 + x) * 49 + (dmd_x * 7 + y);
			const float v = coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + target].value;
			coreGlobals.dmdDotRaw[y * 5 + x] = SAT_NYB(v); // TODO raw value should not be tied to the PWM integration
			coreGlobals.dmdDotLum[y * 5 + x] = SAT_BYTE(v);
		}
    // Use the video update to output mini DMD as LED segments (somewhat hacky)
    for (int x = 0; x < 5; x++) {
        int bits = 0;
        for (int y = 0; y < 7; y++)
            bits = (bits << 1) | (coreGlobals.dmdDotRaw[y * 5 + x] ? 1 : 0);
        coreGlobals.drawSeg[35 * dmd_y + 5 * dmd_x + x] = bits;
    }
    if (!pmoptions.dmd_only)
        core_dmd_video_update(bitmap, cliprect, layout, NULL);
    return 0;
}

// Wheel of Fortune Led matrix display
static PINMAME_VIDEO_UPDATE(samminidmd2_update) {
    int ii,jj,kk;
    for (jj = 0; jj < 35; jj++)
		for (kk = 0; kk < 5; kk++) {
			const int target = 140 + jj + (kk * 35);
			const float v = coreGlobals.physicOutputState[CORE_MODOUT_LAMP0 + target].value;
			coreGlobals.dmdDotRaw[kk * layout->length + jj] = SAT_NYB(v); // TODO raw value should not be tied to the PWM integration
			coreGlobals.dmdDotLum[kk * layout->length + jj] = SAT_BYTE(v);
		}
    // Use the video update to output mini DMD as LED segments (somewhat hacky)
    for (ii = 0; ii < 35; ii++) {
      int bits = 0;
      for (kk = 0; kk < 5; kk++)
        bits = (bits<<1) | (coreGlobals.dmdDotRaw[kk * layout->length + ii] ? 1 : 0);
      coreGlobals.drawSeg[ii] = bits;
    }
    if (!pmoptions.dmd_only)
      core_dmd_video_update(bitmap, cliprect, layout, NULL);
    return 0;
}

static struct core_dispLayout sam_dmd128x32[] = {
	{0, 0, 32, 128, CORE_DMD/*| CORE_DMDNOAA*/, (genf*)samdmd_update},
	{0}
};

static struct core_dispLayout sammini1_dmd128x32[] = {
	DISP_SEG_IMPORT(sam_dmd128x32),
	{34, 10, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 17, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 24, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 31, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 38, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 45, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{34, 52, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 10, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 17, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 24, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 31, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 38, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 45, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{43, 52, 7, 5, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd_update},
	{0}
};

static struct core_dispLayout sammini2_dmd128x32[] = {
	DISP_SEG_IMPORT(sam_dmd128x32),
	{34, 10, 5, 35, CORE_DMD|CORE_DMDNOAA | CORE_NODISP, (genf *)samminidmd2_update},
	{0}
};

/**********************/
/* ROM LOADING MACROS */
/**********************/

/* BOOT FLASH ROM */
#define SAM1_BOOTFLASH(game,rom,chk,len) \
ROM_START(game) \
	ROM_REGION32_LE(0x2000000, REGION_USER1,0) \
		ROM_LOAD(rom, 0, len, chk) \
	ROM_REGION32_LE(0x5000000, REGION_CPU1, ROMREGION_ERASEMASK) \
	ROM_COPY(REGION_USER1,0,0,0x100000) \
	ROM_COPY(REGION_USER1,0,0x2000000,len) \
ROM_END

/* 32MB ROM */
#define SAM1_ROM32MB(game,rom,chk,len) \
ROM_START(game) \
	ROM_REGION32_LE(0x2000000, REGION_USER1,0) \
		ROM_LOAD(rom, 0, len, chk) \
	ROM_REGION32_LE(0x5000000, REGION_CPU1, ROMREGION_ERASEMASK) \
	ROM_COPY(REGION_USER1,0,0,0x100000) \
	ROM_COPY(REGION_USER1,0,0x4000000,0x800000) \
	ROM_COPY(REGION_USER1,0,0x4800000,0x800000) \
ROM_END

/* 128MB ROM */
#define SAM1_ROM128MB(game,rom,chk,len) \
ROM_START(game) \
	ROM_REGION32_LE(0x8000000, REGION_USER1,0) \
		ROM_LOAD(rom, 0, len, chk) \
	ROM_REGION32_LE(0x5000000, REGION_CPU1, ROMREGION_ERASEMASK) \
	ROM_COPY(REGION_USER1,0,0,0x100000) \
	ROM_COPY(REGION_USER1,0,0x4000000,0x800000) \
	ROM_COPY(REGION_USER1,0,0x4800000,0x800000) \
ROM_END

/********************/
/* GAME DEFINITIONS */
/********************/

/*-------------------------------------------------------------------
/ S.A.M. Boot Flash
/-------------------------------------------------------------------*/
INITGAME(sam1_flashb, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_BOOTFLASH(sam1_flashb_0102, "boot_102.bin", CRC(92c93cba) SHA1(aed7ba2f988df8c95e2ad08f70409152d5caa49a), 0x00100000)
SAM1_BOOTFLASH(sam1_flashb_0106, "boot_106.bin", CRC(fe7bcece) SHA1(775590bbd52c24950db86cc231566ba3780030d8), 0x000e8ac8)
SAM1_BOOTFLASH(sam1_flashb_0210, "boot_210.bin", CRC(0f3fd4a4) SHA1(115d0b73c40fcdb2d202a0a9065472d216ca89e0), 0x000f0304)
SAM1_BOOTFLASH(sam1_flashb_0230, "boot_230.bin", CRC(a4258c49) SHA1(d865edf7d1c6d2c922980dd192222dc24bc092a0), 0x000f0624)
SAM1_BOOTFLASH(sam1_flashb_0310, "boot_310.bin", CRC(de017f82) SHA1(e4a9a818fa3f1754374cd00b52b8a087d6c442a9), 0x00100000)

SAM_INPUT_PORTS_START(sam1_flashb, 1)

CORE_GAMEDEF(sam1_flashb, 0310, "S.A.M. Boot Flash Update (V3.1)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sam1_flashb, 0102, 0310, "S.A.M. System Flash Boot (V1.02)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(sam1_flashb, 0106, 0310, "S.A.M. System Flash Boot (V1.06)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(sam1_flashb, 0210, 0310, "S.A.M. System Flash Boot (V2.10)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sam1_flashb, 0230, 0310, "S.A.M. System Flash Boot (V2.3)", 2007, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ World Poker Tour
/-------------------------------------------------------------------*/
INITGAME(wpt, GEN_SAM, sammini1_dmd128x32, SAM_2COL + 60, SAM_GAME_WPT) // 62 additinal columns for Mini DMD (5*7 displays * 14)

SAM1_ROM32MB(wpt_103a, "wpt_103a.bin", CRC(cd5f80bc) SHA1(4aaab2bf6b744e1a3c3509dc9dd2416ff3320cdb), 0x019bb1dc)

SAM1_ROM32MB(wpt_105a, "wpt_105a.bin", CRC(51608819) SHA1(a14aa47bdbce1dc958504d866ac963b06cd93bef), 0x019bb198)

SAM1_ROM32MB(wpt_106a, "wpt_106a.bin", CRC(72fd2e58) SHA1(3e910b964d0dc67fd538c027b474b3587b216ce5), 0x019bb198)
SAM1_ROM32MB(wpt_106f, "wpt_106f.bin", CRC(efa3eeb9) SHA1(a5260511b6325917a9076bac6c92f1a8472142b8), 0x01aa3fdc)
SAM1_ROM32MB(wpt_106g, "wpt_106g.bin", CRC(9b486bc4) SHA1(c2c3c426201db99303131c5efb4275291ab721d7), 0x01a33de8)
SAM1_ROM32MB(wpt_106i, "wpt_106i.bin", CRC(177146f0) SHA1(279380fcc3924a8bb8e3002a66c317473d3fc773), 0x01b2c6ec)
SAM1_ROM32MB(wpt_106l, "wpt_106l.bin", CRC(e38034a1) SHA1(c391887a90f9387f86dc94e22bb7fca57c8e91be), 0x01c706d8)

SAM1_ROM32MB(wpt_108a, "wpt_108a.bin", CRC(bca1f1f7) SHA1(cba81c9645f91f4b0b62ec1eed514069248c19b7), 0x019bb198)
SAM1_ROM32MB(wpt_108f, "wpt_108f.bin", CRC(b1a8f235) SHA1(ea7b553f2340eb82c34f7e95f4dee6fdd3026f14), 0x01aa3fdc)
SAM1_ROM32MB(wpt_108g, "wpt_108g.bin", CRC(b77ccfae) SHA1(730de2c5e9fa85e25ce799577748c9cf7b83c5e0), 0x01a33de8)
SAM1_ROM32MB(wpt_108i, "wpt_108i.bin", CRC(748362f2) SHA1(174733a2d0f45c46dca8bc6d6bc35d39e36e465d), 0x01b2c6ec)
SAM1_ROM32MB(wpt_108l, "wpt_108l.bin", CRC(6440224a) SHA1(e1748f0204464d134c5f5083b5c12723186c0422), 0x01c706d8)

SAM1_ROM32MB(wpt_109a, "wpt_109a.bin", CRC(6702e90c) SHA1(5d208894ef293c8a7157ab27eac9a8bca012dc43), 0x019bb198)
SAM1_ROM32MB(wpt_109f, "wpt_109f.bin", CRC(44f64903) SHA1(f3bcb8acbc8a6cad6f8573f78de53ce8336e7879), 0x01aa3fdc)
SAM1_ROM32MB(wpt_109f2,"wpt_109f2.bin",CRC(656f3957) SHA1(8c68b00fe528f6467a9c34663bbaa9bc308fc971), 0x01aa3fdc)
SAM1_ROM32MB(wpt_109g, "wpt_109g.bin", CRC(0699b279) SHA1(e645361f02865aa5560a4bbae45e085df0c4ae22), 0x01a33de8)
SAM1_ROM32MB(wpt_109i, "wpt_109i.bin", CRC(87e5f39f) SHA1(9c79bb0f9ebb5f4f4b9ef959f56812a3fe2fda11), 0x01b2c6ec)
SAM1_ROM32MB(wpt_109l, "wpt_109l.bin", CRC(a724e6c4) SHA1(161c9e6319a305953ac169cdeceeca304ab582e6), 0x01c706d8)

SAM1_ROM32MB(wpt_111a, "wpt_111a.bin", CRC(423138a9) SHA1(8df7b9358cacb9399c7886b9905441dc727693a6), 0x019bb19c)
SAM1_ROM32MB(wpt_111af,"wpt_111af.bin",CRC(e3a53741) SHA1(395ffe5e25248504d61bb1c96b914e712e7360c3), 0x01a46cf0)
SAM1_ROM32MB(wpt_111ai,"wpt_111ai.bin",CRC(a1e819c5) SHA1(f4e2dc6473f31e7019495d0f37b9b60f2c252f70), 0x01a8c8b8)
SAM1_ROM32MB(wpt_111al,"wpt_111al.bin",CRC(fbe2e2cf) SHA1(ed837d6ecc1f312c84a2fd235ade86227c2df843), 0x01b2ebb0)
SAM1_ROM32MB(wpt_111f, "wpt_111f.bin", CRC(25573be5) SHA1(20a33f387fbf150adda835d2f91ec456077e4c41), 0x01aa3fe0)
SAM1_ROM32MB(wpt_111g, "wpt_111g.bin", CRC(96782b8e) SHA1(4b89f0d44894f0157397a65a93346e637d71c4f2), 0x01a33dec)
SAM1_ROM32MB(wpt_111gf,"wpt_111gf.bin",CRC(c1488680) SHA1(fc652273e55d32b0c6e8e12c8ece666edac42962), 0x01a74b80)
SAM1_ROM32MB(wpt_111i, "wpt_111i.bin", CRC(4d718e63) SHA1(3ae6cefd6f96a31634f1399d1ce5d2c60955a93c), 0x01b2c6f0)
SAM1_ROM32MB(wpt_111l, "wpt_111l.bin", CRC(61f4e257) SHA1(10b11e1340593c9555ff88b0ac971433583fbf13), 0x01c706dc)

SAM1_ROM32MB(wpt_112a, "wpt_112a.bin", CRC(b98b4bf9) SHA1(75257a2759978d5fc699f78e809543d1cc8c456b), 0x019bb1b0)
SAM1_ROM32MB(wpt_112af,"wpt_112af.bin",CRC(8fe1e3c8) SHA1(837bfc2cf7f4601f99d110428f5de5dd69d2186f), 0x01A46D04)
SAM1_ROM32MB(wpt_112ai,"wpt_112ai.bin",CRC(ac878dfb) SHA1(13db57c77f5d75e87b21d3cfd7aba5dcbcbef59b), 0x01A8C8CC)
SAM1_ROM32MB(wpt_112al,"wpt_112al.bin",CRC(2c0dc704) SHA1(d5735977463ee92d87aba3a41d368b92a76b2908), 0x01B2EBC4)
SAM1_ROM32MB(wpt_112f, "wpt_112f.bin", CRC(1f7e081c) SHA1(512d44353f619f974d98294c55378f5a1ab2d04b), 0x01AA3FF4)
SAM1_ROM32MB(wpt_112g, "wpt_112g.bin", CRC(2fbac57d) SHA1(fb19e7a4a5384fc8c91e166617dad29a47b2d8b0), 0x01A33E00)
SAM1_ROM32MB(wpt_112gf,"wpt_112gf.bin",CRC(a6b933b3) SHA1(72a36a427527c3c5cb455a74afbbb43f2bee6480), 0x01A74B94)
SAM1_ROM32MB(wpt_112i, "wpt_112i.bin", CRC(0ba02986) SHA1(db1cbe0611d40c334205d0a8b9f5c6147b259549), 0x01B2C704)
SAM1_ROM32MB(wpt_112l, "wpt_112l.bin", CRC(203c3a05) SHA1(6173f6a6110e2a226beb566371b2821b0a5b8609), 0x01C706F0)
SAM1_ROM32MB(wpt_1129af,"wpt_1129af.bin",CRC(e5660763) SHA1(72b8b878aa8272f9cf54fde2c9ddc7635757e59c),0x01A46CF0)

SAM1_ROM32MB(wpt_140a, "wpt_1400a.bin", CRC(4b287770) SHA1(e19b60a08de9067a2b4c4dd71783fc812b3c7648), 0x019BB1EC)
SAM1_ROM32MB(wpt_140af,"wpt_1400af.bin",CRC(bed3e3f1) SHA1(43b9cd6deccc8e516e2f5e99295b751ccadbac29), 0x01A46D40)
SAM1_ROM32MB(wpt_140ai,"wpt_1400ai.bin",CRC(12a62641) SHA1(680283a7493921904f7fe9fae10d965db839f986), 0x01A8C908)
SAM1_ROM32MB(wpt_140al,"wpt_1400al.bin",CRC(2f03204b) SHA1(c7a0b645258dc1aca6a297641bc5cc10c255d5a7), 0x01B2EC00)
SAM1_ROM32MB(wpt_140f, "wpt_1400f.bin", CRC(3c9ce123) SHA1(5e9f6c9e5d4cdba36b7eacc24b602ea4dde92514), 0x01AA4030)
SAM1_ROM32MB(wpt_140g, "wpt_1400g.bin", CRC(5f8216da) SHA1(79b79acf7c457e6d70af458712bf946094d08d2a), 0x01A33E3C)
SAM1_ROM32MB(wpt_140gf,"wpt_1400gf.bin",CRC(7be526fa) SHA1(a42e5c2c1fde9ab97d7dcfe64b8c0055372729f3), 0x01A74BD0)
SAM1_ROM32MB(wpt_140i, "wpt_1400i.bin", CRC(9f19ed03) SHA1(4ef570be084b1e5196a19b7f516f621025c174bc), 0x01B2C740)
SAM1_ROM32MB(wpt_140l, "wpt_1400l.bin", CRC(00eff09c) SHA1(847203d4d2ce8d11a5403374f2d5b6dda8458bc9), 0x01C7072C)

SAM_INPUT_PORTS_START(wpt, 1)

CORE_GAMEDEF(wpt, 140a, "World Poker Tour (V14.0)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 103a, 140a, "World Poker Tour (V1.03)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 105a, 140a, "World Poker Tour (V1.05)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 106a, 140a, "World Poker Tour (V1.06)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 106f, 140a, "World Poker Tour (V1.06 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 106g, 140a, "World Poker Tour (V1.06 German)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 106i, 140a, "World Poker Tour (V1.06 Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 106l, 140a, "World Poker Tour (V1.06 Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 108a, 140a, "World Poker Tour (V1.08)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 108f, 140a, "World Poker Tour (V1.08 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 108g, 140a, "World Poker Tour (V1.08 German)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 108i, 140a, "World Poker Tour (V1.08 Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 108l, 140a, "World Poker Tour (V1.08 Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 109a, 140a, "World Poker Tour (V1.09)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 109f, 140a, "World Poker Tour (V1.09 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 109f2,140a, "World Poker Tour (V1.09-2 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 109g, 140a, "World Poker Tour (V1.09 German)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 109i, 140a, "World Poker Tour (V1.09 Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 109l, 140a, "World Poker Tour (V1.09 Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 111a, 140a, "World Poker Tour (V1.11)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111af,140a, "World Poker Tour (V1.11 English, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111ai,140a, "World Poker Tour (V1.11 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111al,140a, "World Poker Tour (V1.11 English, Spanish)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111f, 140a, "World Poker Tour (V1.11 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111g, 140a, "World Poker Tour (V1.11 German)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111gf,140a, "World Poker Tour (V1.11 German, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111i, 140a, "World Poker Tour (V1.11 Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 111l, 140a, "World Poker Tour (V1.11 Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 112a, 140a, "World Poker Tour (V1.12)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112af,140a, "World Poker Tour (V1.12 English, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112ai,140a, "World Poker Tour (V1.12 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112al,140a, "World Poker Tour (V1.12 English, Spanish)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112f, 140a, "World Poker Tour (V1.12 French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112g, 140a, "World Poker Tour (V1.12 German)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112gf,140a, "World Poker Tour (V1.12 German, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112i, 140a, "World Poker Tour (V1.12 Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 112l, 140a, "World Poker Tour (V1.12 Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 1129af,140a,"World Poker Tour (V1.129 English, French)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(wpt, 140af,140a, "World Poker Tour (V14.0 English, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140ai,140a, "World Poker Tour (V14.0 English, Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140al,140a, "World Poker Tour (V14.0 English, Spanish)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140f, 140a, "World Poker Tour (V14.0 French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140g, 140a, "World Poker Tour (V14.0 German)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140gf,140a, "World Poker Tour (V14.0 German, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140i, 140a, "World Poker Tour (V14.0 Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(wpt, 140l, 140a, "World Poker Tour (V14.0 Spanish)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ The Simpsons Kooky Carnival Redemption
/-------------------------------------------------------------------*/
INITGAME(scarn9nj, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)
INITGAME(scarn103, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)
INITGAME(scarn105, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)
INITGAME(scarn200, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(scarn9nj, "scarn09nj.bin",CRC(3a9142e0) SHA1(57d75763fb52c891d1bb16e85ae170c38e6dd818), 0x0053B7CC)
SAM1_ROM32MB(scarn103, "scarn103.bin", CRC(69f5bb8a) SHA1(436db9872d5809c7ed5fe607c4167cdc0e1b5294), 0x0053A860)
SAM1_ROM32MB(scarn105, "scarn105.bin", CRC(a09ffa33) SHA1(fab75f338a5d6c82632cd0804ddac1ab78466636), 0x0053DD14)
SAM1_ROM32MB(scarn200, "scarn200.bin", CRC(f08a2cf0) SHA1(ae32da8b35006061d397832563b71976899625bb), 0x005479F8)

SAM_INPUT_PORTS_START(scarn9nj, 1)
SAM_INPUT_PORTS_START(scarn103, 1)
SAM_INPUT_PORTS_START(scarn105, 1)
SAM_INPUT_PORTS_START(scarn200, 1)

CORE_GAMEDEFNV(scarn200, "Simpsons Kooky Carnival, The (Redemption) (V2.0)", 2008, "Stern", sam1, 0)
CORE_CLONEDEFNV(scarn9nj, scarn200, "Simpsons Kooky Carnival, The (Redemption) (V0.90 New Jersey)", 2006, "Stern", sam1, 0)
CORE_CLONEDEFNV(scarn103, scarn200, "Simpsons Kooky Carnival, The (Redemption) (V1.03)", 2006, "Stern", sam1, 0)
CORE_CLONEDEFNV(scarn105, scarn200, "Simpsons Kooky Carnival, The (Redemption) (V1.05)", 2006, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Family Guy
/-------------------------------------------------------------------*/
INITGAME(fg, GEN_SAM, sam_dmd128x32, SAM_8COL, SAM_GAME_FG)

SAM1_ROM32MB(fg_200a,   "fg_200a.bin",   CRC(c72e89df) SHA1(6cac3812d733c9d030542badb9c65934ecbf8399), 0x0185ea9c)
SAM1_ROM32MB(fg_300ai,  "fg_300ai.bin",  CRC(e2cffa79) SHA1(59dff445118ed8a3a76b6e93950802d1fec87619), 0x01FC0290)
SAM1_ROM32MB(fg_400a,   "fg_400a.bin",   CRC(af6c2dd4) SHA1(e3164e982c90a5300144e63e4a74dd225fe1b272), 0x013E789C)
SAM1_ROM32MB(fg_400ag,  "fg_400ag.bin",  CRC(3b4ae199) SHA1(4ef674badce2c90334fa7a8b6b90c32dcabc2334), 0x01971684)
SAM1_ROM32MB(fg_700af,  "fg_700af.bin",  CRC(bbeda480) SHA1(792c396dee1b5abe113484e1fd4c4b449d8e7d95), 0x01A4D3D4)
SAM1_ROM32MB(fg_700al,  "fg_700al.bin",  CRC(25288f43) SHA1(5a2ed2e0b264895938466ca1104ba4ed9be86b3a), 0x01BCE8F8)
SAM1_ROM32MB(fg_800al,  "fg_800al.bin",  CRC(b74dc3bc) SHA1(b24bab06b9f451cf9f068c555d3f70ffdbf40da7), 0x01BC6CB4)
SAM1_ROM32MB(fg_1000af, "fg_1000af.bin", CRC(27cabf5d) SHA1(dde359c1fed728c8f91901f5ce351b5adef399f3), 0x01CE5514)
SAM1_ROM32MB(fg_1000ag, "fg_1000ag.bin", CRC(130e0bd6) SHA1(ced815270d419704d94d5acdc5335460a64484ae), 0x01C53678)
SAM1_ROM32MB(fg_1000ai, "fg_1000ai.bin", CRC(2137e62a) SHA1(ac892d2536c5dde97194ffb69c74d0517000357a), 0x01D9F8B8)
SAM1_ROM32MB(fg_1000al, "fg_1000al.bin", CRC(0f570f24) SHA1(8861bf3e6add7a5372d81199c135808d09b5e600), 0x01E5F448)
SAM1_ROM32MB(fg_1100af, "fg_1100af.bin", CRC(31304627) SHA1(f36d6924f1f291f675f162ff056b6ea2f03f4351), 0x01CE5514)
SAM1_ROM32MB(fg_1100ag, "fg_1100ag.bin", CRC(d2735578) SHA1(a38b8f690ffcdb96875d3c8293e6602d7142be11), 0x01C53678)
SAM1_ROM32MB(fg_1100ai, "fg_1100ai.bin", CRC(4fa2c59e) SHA1(7fce5c1fd306eccc567ae7d155c782649c022074), 0x01D9F8B8)
SAM1_ROM32MB(fg_1100al, "fg_1100al.bin", CRC(d9b724a8) SHA1(33ac12fd4bbed11e38ade68426547ed97612cbd3), 0x01E5F448)
SAM1_ROM32MB(fg_1200ai, "fg_1200ai.bin", CRC(078b0c9a) SHA1(f1472d2c4a06d674bf652dd481cce5d6ca125e0c), 0x01D9F8B8)
SAM1_ROM32MB(fg_1200al, "fg_1200al.bin", CRC(d10cff88) SHA1(e312a3b24f1b69db9f88a5313db168d9f2a71450), 0x01E5F448)
SAM1_ROM32MB(fg_1200af, "fg_1200af.bin", CRC(ba6a3a2e) SHA1(78eb2e26abe00d7ce5fa998b6ec1381ac0f1db31), 0x01CE5514)
SAM1_ROM32MB(fg_1200ag, "fg_1200ag.bin", CRC(d9734f94) SHA1(d56ddf5961e5ac4c3565f9d92d6fb7e0e0af4bcb), 0x01C53678)

SAM_INPUT_PORTS_START(fg, 1)

CORE_GAMEDEF(fg, 1200ag, "Family Guy (V12.0 English, German)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 200a,   1200ag, "Family Guy (V2.00 English)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 300ai,  1200ag, "Family Guy (V3.00 English, Italian)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 400a,   1200ag, "Family Guy (V4.00 English)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 400ag,  1200ag, "Family Guy (V4.00 English, German)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 700af,  1200ag, "Family Guy (V7.00 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 700al,  1200ag, "Family Guy (V7.00 English, Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 800al,  1200ag, "Family Guy (V8.00 English, Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 1000af, 1200ag, "Family Guy (V10.00 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1000ag, 1200ag, "Family Guy (V10.00 English, German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1000ai, 1200ag, "Family Guy (V10.00 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1000al, 1200ag, "Family Guy (V10.00 English, Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 1100af, 1200ag, "Family Guy (V11.0 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1100ag, 1200ag, "Family Guy (V11.0 English, German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1100ai, 1200ag, "Family Guy (V11.0 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1100al, 1200ag, "Family Guy (V11.0 English, Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(fg, 1200af, 1200ag, "Family Guy (V12.0 English, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1200ai, 1200ag, "Family Guy (V12.0 English, Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(fg, 1200al, 1200ag, "Family Guy (V12.0 English, Spanish)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Disney's Pirates of the Caribbean
/-------------------------------------------------------------------*/
INITGAME(potc, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(potc_108as, "potc_108as.bin", CRC(6c3a3f7f) SHA1(52e97a4f479f8f3f55a72c9c104fb1335a253f1a), 0x01C61F6C)

SAM1_ROM32MB(potc_109ai, "potc_109ai.bin", CRC(a8baad2d) SHA1(f2f2a4a16f646a57cc191f8ae2e483e036edb1e7), 0x01B178E8)
SAM1_ROM32MB(potc_109as, "potc_109as.bin", CRC(eb68b86b) SHA1(416c4bf9b4dc035b8dfed3610a4ac5ae31209ca5), 0x01C829B4)
SAM1_ROM32MB(potc_109gf, "potc_109gf.bin", CRC(9866b803) SHA1(c7bef6220cc865614974d02406f109e851a86714), 0x01B60464)

SAM1_ROM32MB(potc_110af, "potc_110af.bin", CRC(9d87bb49) SHA1(9db04259a0b2733d6f5966a2f3e0fc1c7002cef1), 0x01AC6550)
SAM1_ROM32MB(potc_110ai, "potc_110ai.bin", CRC(027916d9) SHA1(0ddc0fa86da55ea0494f2095c838b41b53f568de), 0x01B178E8)
SAM1_ROM32MB(potc_110gf, "potc_110gf.bin", CRC(ce29b69c) SHA1(ecc9ad8f77ab30538536631d513d25654f5a2f3c), 0x01B60464)

SAM1_ROM32MB(potc_111as, "potc_111as.bin", CRC(09903169) SHA1(e284b1dc2642337633867bac9739fdda692acb2f), 0x01C829B4)

SAM1_ROM32MB(potc_113af, "potc_113af.bin", CRC(1c52b3f5) SHA1(2079f06f1f1514614fa7cb240559b4e72925c70c), 0x01AC6550)
SAM1_ROM32MB(potc_113ai, "potc_113ai.bin", CRC(e8b487d1) SHA1(037435b40347a8e1197876fbf7a79e03befa11f4), 0x01B178E8)
SAM1_ROM32MB(potc_113as, "potc_113as.bin", CRC(2c819a02) SHA1(98a79b50e6c80bd58b2571fefc2f5f61030bc25d), 0x01C829B4)
SAM1_ROM32MB(potc_113gf, "potc_113gf.bin", CRC(a508a2f8) SHA1(45e46af267c7caec86e4c92526c4cda85a1bb168), 0x01B60464)

SAM1_ROM32MB(potc_115af, "potc_115af.bin", CRC(008e93b2) SHA1(5a272670cb3e5e59071500124a0086ef86e2b528), 0x01AC6564)
SAM1_ROM32MB(potc_115ai, "potc_115ai.bin", CRC(88b66285) SHA1(1d65e4f7a31e51167b91f82d96c3951442b16264), 0x01B178FC)
SAM1_ROM32MB(potc_115as, "potc_115as.bin", CRC(9c107d0e) SHA1(5213246ee78c6cc082b9f895b1d1abfa52016ede), 0x01C829C8)
SAM1_ROM32MB(potc_115gf, "potc_115gf.bin", CRC(09a8454c) SHA1(1af420b314d339231d3b7772ffa44175a01ebd30), 0x01B60478)

SAM1_ROM32MB(potc_300af, "potc_300af.bin", CRC(b6fc0c4b) SHA1(5c0d6b46dd6c4f14e03298500558f376ee342de0), 0x01AD2B40)
SAM1_ROM32MB(potc_300ai, "potc_300ai.bin", CRC(2d3eb95e) SHA1(fea9409ffea3554ff0ec1c9ef6642465ec4120e7), 0x01B213A8)
SAM1_ROM32MB(potc_300al, "potc_300al.bin", CRC(e5e7049d) SHA1(570125f9eb6d7a04ba97890095c15769f0e0dbd6), 0x01C88124)
SAM1_ROM32MB(potc_300gf, "potc_300gf.bin", CRC(52772953) SHA1(e820ca5f347ab637bee07a9d7426058b9fd6557c), 0x01B67104)

SAM1_ROM32MB(potc_400af, "potc_400af.bin", CRC(03cfed21) SHA1(947fff6bf3ed69cb346ae9f159e378902901033f), 0x01AD2B40)
SAM1_ROM32MB(potc_400ai, "potc_400ai.bin", CRC(5382440b) SHA1(01d8258b98e256fc54565afd9915fd5079201973), 0x01B213A8)
SAM1_ROM32MB(potc_400al, "potc_400al.bin", CRC(f739474d) SHA1(43bf3fbd23498e2cbeac3d87f5da727e7c05eb86), 0x01C88124)
SAM1_ROM32MB(potc_400gf, "potc_400gf.bin", CRC(778d02e7) SHA1(6524e56ebf6c5c0effc4cb0521e3a463540ceac4), 0x01B67104)

SAM1_ROM32MB(potc_600af, "potc_600af.bin", CRC(39a51873) SHA1(9597d356a3283c5a4e488a399196a51bf5ed16ca), 0x01AD2B40)
SAM1_ROM32MB(potc_600ai, "potc_600ai.bin", CRC(2d7aebae) SHA1(9e383507d225859b4df276b21525f500ba98d600), 0x01B24CC8)
SAM1_ROM32MB(potc_600as, "potc_600as.bin", CRC(5d5e1aaa) SHA1(9c7a416ae6587a86c8d2c6350621f09580226971), 0x01C92990)
SAM1_ROM32MB(potc_600gf, "potc_600gf.bin", CRC(44eb2610) SHA1(ec1e1f7f2cd135942531e0e3f540afadb5d2f527), 0x01B67104)

SAM_INPUT_PORTS_START(potc, 1)

CORE_GAMEDEF(potc, 600af, "Pirates of the Caribbean (V6.0 English, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 108as, 600af, "Pirates of the Caribbean (V1.08 English, Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 109ai, 600af, "Pirates of the Caribbean (V1.09 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 109as, 600af, "Pirates of the Caribbean (V1.09 English, Spanish)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 109gf, 600af, "Pirates of the Caribbean (V1.09 German, French)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 110af, 600af, "Pirates of the Caribbean (V1.10 English, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 110ai, 600af, "Pirates of the Caribbean (V1.10 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 110gf, 600af, "Pirates of the Caribbean (V1.10 German, French)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 111as, 600af, "Pirates of the Caribbean (V1.11 English, Spanish)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 113af, 600af, "Pirates of the Caribbean (V1.13 English, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 113ai, 600af, "Pirates of the Caribbean (V1.13 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 113as, 600af, "Pirates of the Caribbean (V1.13 English, Spanish)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 113gf, 600af, "Pirates of the Caribbean (V1.13 German, French)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 115af, 600af, "Pirates of the Caribbean (V1.15 English, French)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 115ai, 600af, "Pirates of the Caribbean (V1.15 English, Italian)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 115as, 600af, "Pirates of the Caribbean (V1.15 English, Spanish)", 2006, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 115gf, 600af, "Pirates of the Caribbean (V1.15 German, French)", 2006, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 300af, 600af, "Pirates of the Caribbean (V3.00 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 300ai, 600af, "Pirates of the Caribbean (V3.00 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 300al, 600af, "Pirates of the Caribbean (V3.00 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 300gf, 600af, "Pirates of the Caribbean (V3.00 German, French)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 400af, 600af, "Pirates of the Caribbean (V4.00 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 400ai, 600af, "Pirates of the Caribbean (V4.00 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 400al, 600af, "Pirates of the Caribbean (V4.00 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 400gf, 600af, "Pirates of the Caribbean (V4.00 German, French)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(potc, 600ai, 600af, "Pirates of the Caribbean (V6.0 English, Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 600as, 600af, "Pirates of the Caribbean (V6.0 English, Spanish)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(potc, 600gf, 600af, "Pirates of the Caribbean (V6.0 German, French)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Spider-Man (Stern)
/-------------------------------------------------------------------*/
INITGAME(sman, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(sman_102af, "sman_102af.bin", CRC(1e77651c) SHA1(fbce7dbe4ce70cd8bd1c01279a774f410f5aaeff), 0x00fbb834)
SAM1_ROM32MB(sman_130af, "sman_130af.bin", CRC(6aa6a03a) SHA1(f56442e84b8789f49127bf4ba97dd05c77ea7c36), 0x017916C8)
SAM1_ROM32MB(sman_130ai, "sman_130ai.bin", CRC(92aab158) SHA1(51662102da54e7e7c0f63689fffbf70653ee8f11), 0x017B7960)
SAM1_ROM32MB(sman_130al, "sman_130al.bin", CRC(33004e72) SHA1(3bc30200945d896aefbff51c7b427595885a23c4), 0x0180AAA0)
SAM1_ROM32MB(sman_130gf, "sman_130gf.bin", CRC(2838d2f3) SHA1(2192f1fbc393c5e0dcd59198d098bb2531d8b6de), 0x017AEC84)
SAM1_ROM32MB(sman_132,   "sman_132a.bin",  CRC(c8cd8f0a) SHA1(c2e1b54de54e8bd480300054c98a4f09d723edb7), 0x01588E0C)
SAM1_ROM32MB(sman_140,   "sman_140a.bin",  CRC(48c2565d) SHA1(78f5d3242cfaa85fa0fd3937b6042f067dff535b), 0x016CE3C0)
SAM1_ROM32MB(sman_140af, "sman_140af.bin", CRC(d181fa71) SHA1(66af219d9266b6b24e6857ad1a6b4fe539058052), 0x01A50398)
SAM1_ROM32MB(sman_140ai, "sman_140ai.bin", CRC(0de6937e) SHA1(f2e60b545ef278e1b7981bf0a3dc2c622205e8e1), 0x01A70F78)
SAM1_ROM32MB(sman_140al, "sman_140al.bin", CRC(fd372e14) SHA1(70f3e4d210a4da4b6122089c477b5b3f51d3593f), 0x01ADC768)
SAM1_ROM32MB(sman_140gf, "sman_140gf.bin", CRC(f1124c86) SHA1(755f15dd566f86695c7143512d81e16af71c8853), 0x01A70F78)
SAM1_ROM32MB(sman_142,   "sman_142a.bin",  CRC(307b0163) SHA1(015c8c86763c645b43bd71a3cdb8975fcd36a99f), 0x016E8D60)
SAM1_ROM32MB(sman_160,   "sman_160a.bin",  CRC(05425962) SHA1(a37f61239a7116e5c14a345c288f781fa6248cf8), 0x01725778)
SAM1_ROM32MB(sman_160af, "sman_160af.bin", CRC(d0b552e9) SHA1(2550baba3c4be5308779d502a2d2d01e1c2539ef), 0x01B0121C)
SAM1_ROM32MB(sman_160ai, "sman_160ai.bin", CRC(b776f59b) SHA1(62600474b8a5e1e2d40319817505c8b5fd3df2fa), 0x01B26D28)
SAM1_ROM32MB(sman_160al, "sman_160al.bin", CRC(776937d9) SHA1(631cadd665f895feac90c3cbc14eb8e321d19b4e), 0x01BB15BC)
SAM1_ROM32MB(sman_160gf, "sman_160gf.bin", CRC(1498f877) SHA1(e625a7e683035665a0a1a97e5de0947628c3f7ea), 0x01B24430)
SAM1_ROM32MB(sman_170,   "sman_170a.bin",  CRC(45c9e5f5) SHA1(8af3215ecc247186c83e235c60c3a2990364baad), 0x01877484)
SAM1_ROM32MB(sman_170af, "sman_170af.bin", CRC(b38f3948) SHA1(8daae4bc8b1eaca2bd43198365474f5da09b4788), 0x01C6F32C)
SAM1_ROM32MB(sman_170ai, "sman_170ai.bin", CRC(ba176624) SHA1(56c847995b5a3e2286e231c1d69f82cf5492cd5d), 0x01C90F74)
SAM1_ROM32MB(sman_170al, "sman_170al.bin", CRC(0455f3a9) SHA1(134ff31605798989b396220f8580d1c079678084), 0x01D24E70)
SAM1_ROM32MB(sman_170gf, "sman_170gf.bin", CRC(152aa803) SHA1(e18f9dcc5380126262cf1e32e99b6cc2c4aa23cb), 0x01C99C74)
SAM1_ROM32MB(sman_190,   "sman_190a.bin",  CRC(7822a6d1) SHA1(6a21dfc44e8fa5e138fe6474c467ef6d6544d78c), 0x01652310)
SAM1_ROM32MB(sman_190af, "sman_190af.bin", CRC(dac27fde) SHA1(93a236afc4be6514a8fc57e45eb5698bd999eef6), 0x018B5C34)
SAM1_ROM32MB(sman_190ai, "sman_190ai.bin", CRC(95c769ac) SHA1(e713677fea9e28b2438a30bf5d81448d3ca140e4), 0x018CD02C)
SAM1_ROM32MB(sman_190al, "sman_190al.bin", CRC(4df8168c) SHA1(8ebfda5378037c231075017713515a3681a0e38c), 0x01925DD0)
SAM1_ROM32MB(sman_190gf, "sman_190gf.bin", CRC(a4a874a4) SHA1(1e46720462f1279c417d955c500e829e878ce31f), 0x018CD02C)
SAM1_ROM32MB(sman_192,   "sman_192a.bin",  CRC(a44054fa) SHA1(a0910693d13cc61dba7a2bbe9185a24b33ef20ec), 0x01920870)
SAM1_ROM32MB(sman_192af, "sman_192af.bin", CRC(c9f8a7dd) SHA1(e63e98965d08b8a645c92fb34ce7fc6e1ad05ddc), 0x01B81624)
SAM1_ROM32MB(sman_192ai, "sman_192ai.bin", CRC(f02acad4) SHA1(da103d5ddbcbdcc19cca6c17b557dcc71942970a), 0x01B99F88)
SAM1_ROM32MB(sman_192al, "sman_192al.bin", CRC(501f9986) SHA1(d93f973f9eddfd85903544f0ce49c1bf17b36eb9), 0x01BF19A0)
SAM1_ROM32MB(sman_192gf, "sman_192gf.bin", CRC(32597e1d) SHA1(47a28cdba11b32661dbae95e3be1a41fc475fa5e), 0x01B9A1B4)
SAM1_ROM32MB(sman_200,   "sman_200a.bin",  CRC(3b13348c) SHA1(4b5c6445d7805c0a39054bd51522751030b73162), 0x0168E8A8)
SAM1_ROM32MB(sman_210,   "sman_210a.bin",  CRC(f983df18) SHA1(a0d46e1a58f016102773861a4f1b026755f776c8), 0x0168e8a8)
SAM1_ROM32MB(sman_210af, "sman_210af.bin", CRC(2e86ac24) SHA1(aa223db6289a876e77080e16f29cbfc62183fa67), 0x0192b160)
SAM1_ROM32MB(sman_210ai, "sman_210ai.bin", CRC(aadd1ea7) SHA1(a41b0067f7490c6df5d85e80b208c9993f806366), 0x0193cc7c)
SAM1_ROM32MB(sman_210al, "sman_210al.bin", CRC(8c441caa) SHA1(e40ac748284f65de5c444ac89d3b02dd987facd0), 0x019a5d5c)
SAM1_ROM32MB(sman_210gf, "sman_210gf.bin", CRC(2995cb97) SHA1(0093d3f20aebbf6129854757cc10aff63fc18a4a), 0x01941c04)
SAM1_ROM32MB(sman_220,   "sman_220a.bin",  CRC(44f31e8e) SHA1(4c07d01c95c5fab1955b11e4f7c65f369a91dfd7), 0x018775B8)
SAM1_ROM32MB(sman_230,   "sman_230a.bin",  CRC(a86f1768) SHA1(72662dcf05717d3b2b335077ceddabe562738468), 0x018775B8)
SAM1_ROM32MB(sman_240,   "sman_240a.bin",  CRC(dc5ee57e) SHA1(7453db81b161cdbf7be690da15ea8a78e4a4e57d), 0x018775B8)
SAM1_ROM32MB(sman_250,   "sman_250a.bin",  CRC(78d61e14) SHA1(3241d62e12d716ed661fbd0949cf4a39feb64437), 0x018775B8)
SAM1_ROM32MB(sman_260,   "sman_260a.bin",  CRC(acfc813e) SHA1(bcbb0ec2bbfc55b1256c83b0300c0c38d15a3db1), 0x018775E0)
SAM1_ROM32MB(sman_261,   "sman_261a.bin",  CRC(9900cd4c) SHA1(1b95f957f8d709bba9fb3b7dcd4bca99176a010c), 0x01718F64)
SAM1_ROM32MB(sman_262,   "sman_262a.bin",  CRC(9ad85331) SHA1(64a4b4087aee06b79d959e2bc9490ba269cd47a6), 0x018775E0)

SAM_INPUT_PORTS_START(sman, 1)

CORE_GAMEDEF(sman, 261, "Spider-Man (V2.61)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 102af, 261, "Spider-Man (V1.02 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 130af, 261, "Spider-Man (V1.30 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 130ai, 261, "Spider-Man (V1.30 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 130al, 261, "Spider-Man (V1.30 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 130gf, 261, "Spider-Man (V1.30 German, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 132,   261, "Spider-Man (V1.32)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 140,   261, "Spider-Man (V1.4)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 140af, 261, "Spider-Man (V1.4 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 140ai, 261, "Spider-Man (V1.4 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 140al, 261, "Spider-Man (V1.4 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 140gf, 261, "Spider-Man (V1.4 German, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 142,   261, "Spider-Man (V1.42 BETA)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 160,   261, "Spider-Man (V1.6)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 160af, 261, "Spider-Man (V1.6 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 160ai, 261, "Spider-Man (V1.6 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 160al, 261, "Spider-Man (V1.6 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 160gf, 261, "Spider-Man (V1.6 German, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 170,   261, "Spider-Man (V1.7)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 170af, 261, "Spider-Man (V1.7 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 170ai, 261, "Spider-Man (V1.7 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 170al, 261, "Spider-Man (V1.7 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 170gf, 261, "Spider-Man (V1.7 German, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 190,   261, "Spider-Man (V1.9)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 190af, 261, "Spider-Man (V1.9 English, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 190ai, 261, "Spider-Man (V1.9 English, Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 190al, 261, "Spider-Man (V1.9 English, Spanish)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 190gf, 261, "Spider-Man (V1.9 German, French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 192,   261, "Spider-Man (V1.92)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 192af, 261, "Spider-Man (V1.92 English, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 192ai, 261, "Spider-Man (V1.92 English, Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 192al, 261, "Spider-Man (V1.92 English, Spanish)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 192gf, 261, "Spider-Man (V1.92 German, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 200,   261, "Spider-Man (V2.0)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 210,   261, "Spider-Man (V2.1)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 210af, 261, "Spider-Man (V2.1 English, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 210ai, 261, "Spider-Man (V2.1 English, Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 210al, 261, "Spider-Man (V2.1 English, Spanish)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 210gf, 261, "Spider-Man (V2.1 German, French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 220,   261, "Spider-Man (V2.2)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 230,   261, "Spider-Man (V2.3)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 240,   261, "Spider-Man (V2.4)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 250,   261, "Spider-Man (V2.5)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 260,   261, "Spider-Man (V2.6)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(sman, 262,   261, "Spider-Man (V2.62) (MOD with replaced music)", 2014, "Stern/Destruk", sam1, 0)

/*-------------------------------------------------------------------
/ Wheel of Fortune
/-------------------------------------------------------------------*/

#define WOF_WHEEL_OPTO_SW 47
// WOF wheel actually goes 4,6,5,7 ... we will do a special mapping
// in solenoid code to map 5 to 4 and 7 to 6 so we get all pulses, and avoid
// needing to re-do mech code. 
#define WOF_WHEEL_SOL1 4
#define WOF_WHEEL_SOL2 6

static mech_tInitData mechwofWheel = {
	WOF_WHEEL_SOL1, WOF_WHEEL_SOL2, MECH_CIRCLE | MECH_LINEAR | MECH_TWOSTEPSOL | MECH_FAST, 398, 360,
	{ { WOF_WHEEL_OPTO_SW, 358, 360 }, 0 }
};

static void wof_handleMech(int mech) {
}

static int wof_getMech(int mechNo) {
	return mech_getPos(mechNo);
}
static void wof_drawMech(BMTYPE **line) {
	core_textOutf(35, 10, BLACK, "Wheel Pos   : %3d", wof_getMech(0));
}

static core_tGameData wofGameData = { // 62 additional LED columns for MiniDMD (35x14 DMD = 490 LEDs)
	GEN_SAM, sammini2_dmd128x32, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, SAM_8COL + 62, 16, 0, 0, SAM_GAME_WOF ,0, sam_getSol, wof_handleMech, wof_getMech, wof_drawMech}
};

static void init_wof(void) { 
	core_gameData = &wofGameData; 
	mech_add(0, &mechwofWheel);
}


SAM1_ROM32MB(wof_100,  "wof_100a.bin", CRC(f3b80429) SHA1(ab1c9752ea74b5950b51aabc6dbca4f405705240), 0x01C7DF60)
SAM1_ROM32MB(wof_200,  "wof_200a.bin", CRC(2e56b65f) SHA1(908662261548f4b80433d58359e9ff1013bf315b), 0x01C7DFD0)
SAM1_ROM32MB(wof_200f, "wof_200f.bin", CRC(d48d4885) SHA1(25cabea55f30d86b8d6398f94e1d180377c34de6), 0x01E76BA4)
SAM1_ROM32MB(wof_200g, "wof_200g.bin", CRC(81f61e6c) SHA1(395be7e0ccb9a806738fc6338b8e6dbea561986d), 0x01CDEC2C)
SAM1_ROM32MB(wof_200i, "wof_200i.bin", CRC(3e48eef7) SHA1(806a0313852405cd9913406201dd9e434b9b160a), 0x01D45EE8)
SAM1_ROM32MB(wof_300,  "wof_300a.bin", CRC(7a8483b8) SHA1(e361eea5a01d6ba22782d34538edd05f3b068472), 0x01C7DFD0)
SAM1_ROM32MB(wof_300f, "wof_300f.bin", CRC(fd5c2bec) SHA1(77f6e4177df8a17f43198843f8a0a3cf5caf1704), 0x01E76BA4)
SAM1_ROM32MB(wof_300g, "wof_300g.bin", CRC(54b50069) SHA1(909b98a7f5fdfa0164c7dc52e9c830eecada2a64), 0x01CDEC2C)
SAM1_ROM32MB(wof_300i, "wof_300i.bin", CRC(7528800b) SHA1(d55024935861aa8895f9604e92f0d74cb2f3827d), 0x01D45EE8)
SAM1_ROM32MB(wof_300l, "wof_300l.bin", CRC(12e1b3a5) SHA1(6b62e40e7b124477dc8508e39722c3444d4b39a4), 0x01B080B0)
SAM1_ROM32MB(wof_400,  "wof_400a.bin", CRC(974e6dd0) SHA1(ce4d7537e8f42ab6c3e84eac19688e2155115345), 0x01C7DFD0)
SAM1_ROM32MB(wof_400f, "wof_400f.bin", CRC(91a793c0) SHA1(6c390ab435dc20889bccfdd11bbfc411efd1e4f9), 0x01E76BA4)
SAM1_ROM32MB(wof_400g, "wof_400g.bin", CRC(ee97a6f3) SHA1(17a3093f7e5d052c23b669ee8717a21a80b61813), 0x01CDEC2C)
SAM1_ROM32MB(wof_400i, "wof_400i.bin", CRC(35053d2e) SHA1(3b8d176c7b34e7eaf20f9dcf27649841c5122609), 0x01D45EE8)
SAM1_ROM32MB(wof_401l, "wof_401l.bin", CRC(4db936f4) SHA1(4af1d4642529164cb5bc0b9adbc229b131098007), 0x01B080B0)
SAM1_ROM32MB(wof_500,  "wof_500a.bin", CRC(6613e864) SHA1(b6e6dcfa782720e7d0ce36f8ea33a0d05763d6bd), 0x01C7DFD0)
SAM1_ROM32MB(wof_500f, "wof_500f.bin", CRC(3aef1035) SHA1(4fa0a40fea403beef0b3ce695ff52dec3d90f7bf), 0x01E76BA4)
SAM1_ROM32MB(wof_500g, "wof_500g.bin", CRC(658f8622) SHA1(31926717b5914f91b70eeba182eb219a4fd51299), 0x01CDEC2C)
SAM1_ROM32MB(wof_500i, "wof_500i.bin", CRC(27fb48bc) SHA1(9a9846c84a1fc543ec2236a28991d0cd70e86b52), 0x01D45EE8)
SAM1_ROM32MB(wof_500l, "wof_500l.bin", CRC(b8e09fcd) SHA1(522983ce75b24733a0827a2eeea3d44419c7998e), 0x01B080B0)
SAM1_ROM32MB(wof_602h, "wof_602h.bin", CRC(89ebffc5) SHA1(49d2bf1abf40ddc15a1f66c9fa27605e60148473), 0x01B07E10)

SAM_INPUT_PORTS_START(wof, 1)

CORE_GAMEDEF(wof, 500, "Wheel of Fortune (V5.0)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 100,  500, "Wheel of Fortune (V1.0)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 200,  500, "Wheel of Fortune (V2.0)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 200f, 500, "Wheel of Fortune (V2.0 French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 200g, 500, "Wheel of Fortune (V2.0 German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 200i, 500, "Wheel of Fortune (V2.0 Italian)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(wof, 300,  500, "Wheel of Fortune (V3.0)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 300f, 500, "Wheel of Fortune (V3.0 French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 300g, 500, "Wheel of Fortune (V3.0 German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 300i, 500, "Wheel of Fortune (V3.0 Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 300l, 500, "Wheel of Fortune (V3.0 Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(wof, 400,  500, "Wheel of Fortune (V4.0)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 400f, 500, "Wheel of Fortune (V4.0 French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 400g, 500, "Wheel of Fortune (V4.0 German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 400i, 500, "Wheel of Fortune (V4.0 Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 401l, 500, "Wheel of Fortune (V4.01 Spanish)", 2007, "Stern", sam1, 0)

CORE_CLONEDEF(wof, 500f, 500, "Wheel of Fortune (V5.0 French)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 500g, 500, "Wheel of Fortune (V5.0 German)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 500i, 500, "Wheel of Fortune (V5.0 Italian)", 2007, "Stern", sam1, 0)
CORE_CLONEDEF(wof, 500l, 500, "Wheel of Fortune (V5.0 Spanish)", 2007, "Stern", sam1, 0)

// V6.00I missing, compiled by Keith for tournaments/IFPA:
/*
I released a version of software, “6.00I” (for IFPA, I don’t remember what tournament it was for) or some such,
that basically added some competition mode stuff (derandomized wild card and big spin).
Those are the ONLY changes from 5.00 which is the last public release I did while at Stern.
6.00I was circulated a fair amount amongst tournament types, mostly those running tournaments.
*/

CORE_CLONEDEF(wof, 602h, 500, "Wheel of Fortune (V6.02 Home Rom)", 2009, "Stern", sam1, 0) // unofficial version, but apparently compiled by Stern
/*
As someone stated, the main gameplay change that is noticeable is that there are “mode goals.”
The goal is simply to score x points before time runs out. If you get the goal, you won, great.
The next mode, the goal would be higher. If you didn’t win, oh well, the next mode, the goal would be lower.
Also, you could replay the mode you failed, at 2x points.
If you failed the same mode twice, you could play it a 3rd time for 3x points.
If you failed 3 times, the game gave up. Oh, also for each mode you won, you got a “winnings x” for that ball’s bonus.
IIRC there’s no logic for completing the wheel yet, but the reward was going to be something like 10M for each mode won on try #1,
5M for each on try #2, 2.5M for each on try #3, and 1M for failed modes.
If by some unfathomable stroke of luck you completed every mode on the first try, you’d get a bonus to round up the total to 100M.
*/
/*
The mode stuff is basically there’s a common goal of points (starts at 5M).
If you finish a mode, the goal goes up, if you lose a mode, the goal goes down.
If you don’t finish a mode, it stays in the rotation, and if you play it again it scores 2x, and if you fail and play again it’s 3x.
If you don’t hit the goal after 3x, it stops trying. Winning a mode also gives you a winnings x for that ball.
This was all meant to lead up to a payout for hitting the goal in every mode:
10M/5M/2.5M/1M for finishing in 1 try / 2 tries / 3 tries / DNF.
If you finished every goal on the first try (somehow), the bonus would get augmented to 100M.
*/
/*
Lamp test was extended so the wheel LEDs are now tested in lamps test,
in six additional columns!

Included in the lamp test are also two formerly undocumented LEDs:
GI upper left and right.
*/

/*-------------------------------------------------------------------
/ Shrek
/-------------------------------------------------------------------*/
INITGAME(shr, GEN_SAM, sam_dmd128x32, SAM_8COL, SAM_GAME_FG)

SAM1_ROM32MB(shr_130, "shr_130.bin", CRC(0c4efde5) SHA1(58e156a43fef983d48f6676e8d65fb30d45f8ec3), 0x01BB0824)
SAM1_ROM32MB(shr_141, "shr_141.bin", CRC(f4f847ce) SHA1(d28f9186bb04036e9ff56d540e70a50f0816051b), 0x01C55290)

SAM_INPUT_PORTS_START(shr, 1)

CORE_GAMEDEF(shr, 141, "Shrek (V1.41)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(shr, 130, 141, "Shrek (V1.3)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Indiana Jones (Stern)
/-------------------------------------------------------------------*/
INITGAME(ij4, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_IJ4)

SAM1_ROM32MB(ij4_113,  "ij4_113.bin",  CRC(aa2bdf3e) SHA1(71fd1c970fe589cec5124237684facaae92cbf09), 0x01C6D98C)
SAM1_ROM32MB(ij4_113f, "ij4_113f.bin", CRC(cb7b7c31) SHA1(3a2f718a9a533941c5476f8348dacf7e3523ddd0), 0x01C6D98C)
SAM1_ROM32MB(ij4_113g, "ij4_113g.bin", CRC(30a33bfd) SHA1(c37b6035c313cce85d325ab87039f5a872d28f5a), 0x01BFF3F4)
SAM1_ROM32MB(ij4_113i, "ij4_113i.bin", CRC(fcb37e0f) SHA1(7b23a56baa9985e2322aee954befa13dc2d55119), 0x01C81FA4)
SAM1_ROM32MB(ij4_113l, "ij4_113l.bin", CRC(e4ff8120) SHA1(f5537cf920633a621b4c7a740bfc07cefe3a99d0), 0x01D02988)

SAM1_ROM32MB(ij4_114,  "ij4_114.bin",  CRC(00e5b850) SHA1(3ad57120d11aff4ca8917dea28c2c26ae254e2b5), 0x01C6D9E4)
SAM1_ROM32MB(ij4_114f, "ij4_114f.bin", CRC(a7c2a5e4) SHA1(c0463b055096a3112a31680dc509f421c1a5c1cf), 0x01C6D9E4)
SAM1_ROM32MB(ij4_114g, "ij4_114g.bin", CRC(7176b0be) SHA1(505132887bca0fa9d6ca8597101357f26501a0ad), 0x01C34974)
SAM1_ROM32MB(ij4_114i, "ij4_114i.bin", CRC(dac0563e) SHA1(30dbaed1b1a180f7ca68a4caef469c2997bf0355), 0x01C875F8)
SAM1_ROM32MB(ij4_114l, "ij4_114l.bin", CRC(e9b3a81a) SHA1(574377e7a398083f3498d91640ad7dc5250acbd7), 0x01D0B290)

SAM1_ROM32MB(ij4_116,  "ij4_116.bin",  CRC(80293485) SHA1(043c857a8dfa79cb7ae876c55a10227bdff8e873), 0x01C6D9E4)
SAM1_ROM32MB(ij4_116f, "ij4_116f.bin", CRC(56821942) SHA1(484f4359b6d1ecb45c29bef7532a8136028504f4), 0x01C6D9E4)
SAM1_ROM32MB(ij4_116g, "ij4_116g.bin", CRC(2b7b81be) SHA1(a70ed07daec7f13165a0256bc011a72136e25210), 0x01C34974)
SAM1_ROM32MB(ij4_116i, "ij4_116i.bin", CRC(7b07c207) SHA1(67969e85cf96949f8b85d88acfb69be55f32ea52), 0x01C96B38)
SAM1_ROM32MB(ij4_116l, "ij4_116l.bin", CRC(833ae2fa) SHA1(cb931e473164ddfa2559f3a58f2fcac5d456dc96), 0x01D14FD8)

SAM1_ROM32MB(ij4_210,  "ij4_210.bin",  CRC(b96e6fd2) SHA1(f59cbdefc5ab6b21662981b3eb681fd8bd7ade54), 0x01C6D9E4)
SAM1_ROM32MB(ij4_210f, "ij4_210f.bin", CRC(d1d37248) SHA1(fd6819e0e86b83d658790ff30871596542f98c8e), 0x01C6D9E4)

SAM_INPUT_PORTS_START(ij4, 1)

CORE_GAMEDEF(ij4, 210, "Indiana Jones (V2.1)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 113,  210, "Indiana Jones (V1.13)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 113f, 210, "Indiana Jones (V1.13 French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 113g, 210, "Indiana Jones (V1.13 German)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 113i, 210, "Indiana Jones (V1.13 Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 113l, 210, "Indiana Jones (V1.13 Spanish)", 2008, "Stern", sam1, 0)

CORE_CLONEDEF(ij4, 114,  210, "Indiana Jones (V1.14)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 114f, 210, "Indiana Jones (V1.14 French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 114g, 210, "Indiana Jones (V1.14 German)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 114i, 210, "Indiana Jones (V1.14 Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 114l, 210, "Indiana Jones (V1.14 Spanish)", 2008, "Stern", sam1, 0)

CORE_CLONEDEF(ij4, 116,  210, "Indiana Jones (V1.16)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 116f, 210, "Indiana Jones (V1.16 French)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 116g, 210, "Indiana Jones (V1.16 German)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 116i, 210, "Indiana Jones (V1.16 Italian)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(ij4, 116l, 210, "Indiana Jones (V1.16 Spanish)", 2008, "Stern", sam1, 0)

CORE_CLONEDEF(ij4, 210f, 210, "Indiana Jones (V2.1 French)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Batman: The Dark Knight
/-------------------------------------------------------------------*/
INITGAME(bdk, GEN_SAM, sam_dmd128x32, SAM_3COL, SAM_GAME_3LEDS_BSTB)

SAM1_ROM32MB(bdk_130, "bdk_130.bin", CRC(83a32958) SHA1(0326891bc142c8b92bd4f6d29bd4301bacbed0e7), 0x01BA1E94)
SAM1_ROM32MB(bdk_150, "bdk_150.bin", CRC(ed11b88c) SHA1(534224de597cbd3632b902397d945ab725e24912), 0x018EE5E8)
SAM1_ROM32MB(bdk_160, "bdk_160.bin", CRC(5554ea47) SHA1(0ece4779ad9a3d6c8428306774e2bf36a20d680d), 0x01B02F70)
SAM1_ROM32MB(bdk_200, "bdk_200.bin", CRC(07b716a9) SHA1(4cde06308bb967435c7c1bf078a2cda36088e3ec), 0x01B04378)
SAM1_ROM32MB(bdk_210, "bdk_210.bin", CRC(ac84fef1) SHA1(bde3250f3d95a12a5f3b74ac9d11ba0bd331e9cd), 0x01B96D94)
SAM1_ROM32MB(bdk_220, "bdk_220.bin", CRC(6e415ce7) SHA1(30a3938817da20ccb87c7e878cdd8a13ada097ab), 0x01b96d94)
SAM1_ROM32MB(bdk_240, "bdk_240.bin", CRC(6cf8c983) SHA1(fd1396e1075fd938f8a95c27c96a0164137b62dc), 0x01b96d94)
SAM1_ROM32MB(bdk_290, "bdk_290.bin", CRC(09ce777e) SHA1(79b6d3f91aa4d42318c698a44444bf875ad573f2), 0x01d3d2d4)
SAM1_ROM32MB(bdk_294, "bdk_294.bin", CRC(e087ec82) SHA1(aad2c43e6de9a520954eb50b6c824a138cd6f47f), 0x01C00844)
SAM1_ROM32MB(bdk_300, "bdk_300.bin", CRC(8325bc80) SHA1(04f20d78ad33956618e576bba108ab145e26f9aa), 0x01C6AD84)
#ifdef SAM_INCLUDE_COLORED
//SAM1_ROM128MB(bdk_294c, "bdk_294c.bin", CRC(5a4246a1) SHA1(725eb666ffaef894d2bd694d412658395c7fa7f9), 0x077FFFF0)
#endif 

SAM_INPUT_PORTS_START(bdk, 1)

CORE_GAMEDEF(bdk, 294, "Batman: The Dark Knight (V2.94)", 2010, "Stern", sam1, 0) // Pro model in Stern terms
CORE_CLONEDEF(bdk, 130, 294, "Batman: The Dark Knight (V1.3)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 150, 294, "Batman: The Dark Knight (V1.5)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 160, 294, "Batman: The Dark Knight (V1.6)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 200, 294, "Batman: The Dark Knight (V2.0)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 210, 294, "Batman: The Dark Knight (V2.1)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 220, 294, "Batman: The Dark Knight (V2.2)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 240, 294, "Batman: The Dark Knight (V2.4)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 290, 294, "Batman: The Dark Knight (V2.9)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(bdk, 300, 294, "Batman: The Dark Knight Home Edition/Costco (V3.00)", 2010, "Stern", sam1, 0) // Standard model in Stern terms
#ifdef SAM_INCLUDE_COLORED
//CORE_CLONEDEF(bdk, 294c, 294, "Batman: The Dark Knight (V2.9) (Colored MOD)", 2010, "Stern", sam1, 0) // seems to be lost by now :/
#endif

/*-------------------------------------------------------------------
/ CSI: Crime Scene Investigation
/-------------------------------------------------------------------*/
INITGAME(csi, GEN_SAM, sam_dmd128x32, SAM_3COL, SAM_GAME_3LEDS_CSTB | SAM_GAME_CSI)

SAM1_ROM32MB(csi_102, "csi_102a.bin", CRC(770f4ab6) SHA1(7670022926fcf5bb8f8848374cf1a6237803100a), 0x01e21fc0)
SAM1_ROM32MB(csi_103, "csi_103a.bin", CRC(371bc874) SHA1(547588b85b4d6e79123178db3f3e51354e8d2229), 0x01E61C88)
SAM1_ROM32MB(csi_104, "csi_104a.bin", CRC(15694586) SHA1(3a6b70d43f9922d7a459e1dc4c235bcf03e7858e), 0x01e21fc0)
SAM1_ROM32MB(csi_200, "csi_200a.bin", CRC(ecb25112) SHA1(385bede7955e06c1e1b7cd06e988a64b0e6ea54f), 0x01e21fc0)
SAM1_ROM32MB(csi_210, "csi_210a.bin", CRC(afebb31f) SHA1(9b8179baa2f6e61852b57aaad9a28def0c014861), 0x01e21fc0)
SAM1_ROM32MB(csi_230, "csi_230a.bin", CRC(c25ccc67) SHA1(51a21fca06db4b05bda2c7d5a09d655c97ba19c6), 0x01e21fc0)
SAM1_ROM32MB(csi_240, "csi_240a.bin", CRC(2be97fa3) SHA1(5aa231bde81f7787cc06567c8b3d28c750588071), 0x01e21fc0)

SAM_INPUT_PORTS_START(csi, 1)

CORE_GAMEDEF(csi, 240, "CSI: Crime Scene Investigation (V2.4)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 102, 240, "CSI: Crime Scene Investigation (V1.02)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 103, 240, "CSI: Crime Scene Investigation (V1.03)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 104, 240, "CSI: Crime Scene Investigation (V1.04)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 200, 240, "CSI: Crime Scene Investigation (V2.0)", 2008, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 210, 240, "CSI: Crime Scene Investigation (V2.1)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(csi, 230, 240, "CSI: Crime Scene Investigation (V2.3)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ 24
/-------------------------------------------------------------------*/
INITGAME(twenty4, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(twenty4_130, "24_130a.bin", CRC(955a5c12) SHA1(66e33fb438c831679aeb3ba68af7b4a3c59966ef), 0x01C08280)
SAM1_ROM32MB(twenty4_140, "24_140a.bin", CRC(bab92fb1) SHA1(07c8d9c28730411dd0f23d5960a223beb4c587b2), 0x01C08280)
SAM1_ROM32MB(twenty4_144, "24_144a.bin", CRC(29c47da0) SHA1(8d38e35a0df843a71cac6cd4dd6aa460347a208c), 0x01CA8E50)
SAM1_ROM32MB(twenty4_150, "24_150a.bin", CRC(9d7d87cc) SHA1(df6b2f60b87226fdda33bdbbe03ea87d690fc563), 0x01CA8E50)

SAM_INPUT_PORTS_START(twenty4, 1)

CORE_GAMEDEF(twenty4, 150, "24 (V1.5)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(twenty4, 130, 150, "24 (V1.3)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(twenty4, 140, 150, "24 (V1.4)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(twenty4, 144, 150, "24 (V1.44)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ NBA
/-------------------------------------------------------------------*/
INITGAME(nba, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(nba_500, "nba_500.bin", CRC(01b0c27a) SHA1(d7f4f6b24630b55559a48cde4475422905811106), 0x019112d0)
SAM1_ROM32MB(nba_600, "nba_600.bin", CRC(af2fbcf4) SHA1(47df1992a1eb6c4cd5ec246912eab9f5636499a7), 0x019112d0)
SAM1_ROM32MB(nba_700, "nba_700.bin", CRC(15ece43b) SHA1(90cc8b4c52a61da9701fcaba0a21144fe576eaf4), 0x019112d0)
SAM1_ROM32MB(nba_801, "nba_801.bin", CRC(0f8b146e) SHA1(090d73a9bff0a0b0c17ced1557d5e63e5c986e95), 0x019112d0)
SAM1_ROM32MB(nba_802, "nba_802.bin", CRC(ba681dac) SHA1(184f3315a54b1a5295b19222c718ac38fa60d340), 0x019112d0)

SAM_INPUT_PORTS_START(nba, 1)

CORE_GAMEDEF(nba, 802, "NBA (V8.02)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(nba, 500, 802, "NBA (V5.0)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(nba, 600, 802, "NBA (V6.0)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(nba, 700, 802, "NBA (V7.0)", 2009, "Stern", sam1, 0)
CORE_CLONEDEF(nba, 801, 802, "NBA (V8.01)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Big Buck Hunter Pro
/-------------------------------------------------------------------*/
INITGAME(bbh, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(bbh_140, "bbh_140.bin", CRC(302e29f0) SHA1(0c500c0a5588f8476a71599be70b515ba3e19cab), 0x01bb8fa4)
SAM1_ROM32MB(bbh_150, "bbh_150.bin", CRC(18bad072) SHA1(16e499046107baceda6f6c934d70ba2108915973), 0x01bb8fa4)
SAM1_ROM32MB(bbh_160, "bbh_160.bin", CRC(75077f85) SHA1(c58a2ae5c1332390f0d1191ee8ff920ceec23352), 0x01BB8FA4)
SAM1_ROM32MB(bbh_170, "bbh_170.bin", CRC(0c2d3e64) SHA1(9a71959c57b9a75028e21bce9ee03871f8914138), 0x01BB8FD0)

SAM_INPUT_PORTS_START(bbh, 1)

CORE_GAMEDEF(bbh, 170, "Big Buck Hunter Pro (V1.7)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(bbh, 140, 170, "Big Buck Hunter Pro (V1.4)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(bbh, 150, 170, "Big Buck Hunter Pro (V1.5)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(bbh, 160, 170, "Big Buck Hunter Pro (V1.6)", 2010, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Iron Man
/-------------------------------------------------------------------*/
INITGAME(im, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL12)

SAM1_ROM32MB(im_100,  "im_100.bin",  CRC(b27d12bf) SHA1(dfb497f2edaf4321823b243cced9d9e2b7bac628), 0x01b8fe44)
SAM1_ROM32MB(im_110,  "im_110.bin",  CRC(3140cb7c) SHA1(20b0e84b61069e09f189d79e6b4d5abf0369a893), 0x01b8fe44)
SAM1_ROM32MB(im_120,  "im_120.bin",  CRC(71df27ad) SHA1(9e1745522d28af6bdcada56f2cf0b489656fc885), 0x01b8fe44)
SAM1_ROM32MB(im_140,  "im_140.bin",  CRC(9cbfd6ef) SHA1(904c058a00c268593a62988127f8a18d974eda5e), 0x01CB8870)
SAM1_ROM32MB(im_160,  "im_160.bin",  CRC(ed0dd2bb) SHA1(789b9dc5f5d97a86eb406f864f2785f371db6ca5), 0x01C1FD64)
SAM1_ROM32MB(im_181,  "im_181.bin",  CRC(915d972b) SHA1(0d29929ae304bc4bbdbab7813a279f3200cac6ef), 0x01C52C5C)
SAM1_ROM32MB(im_182,  "im_182.bin",  CRC(c65aff0b) SHA1(ce4d26ffdfd8539e8f7fca78dfa55f80247f9334), 0x01C52C5C)
SAM1_ROM32MB(im_183,  "im_183.bin",  CRC(cf2791a6) SHA1(eb616e3bf33024374f4e998a579bc88f63282ba6), 0x01C52C5C)
SAM1_ROM32MB(im_183ve,"im_183ve.bin",CRC(e477183c) SHA1(6314b44b58c79889f95af1792395203dbbb36b0b), 0x01C52C5C)
SAM1_ROM32MB(im_185,  "im_185.bin",  CRC(fc93c3e0) SHA1(41a89a77b60d831231f5b4c66e6ddfa542225013), 0x01C52CDC)
SAM1_ROM32MB(im_185ve,"im_185ve.bin",CRC(3f1529f1) SHA1(a3654efa93a70f40fe718b31d8c0d40ccc99df6b), 0x01C52CDC)
SAM1_ROM32MB(im_186,  "im_186.bin",  CRC(4ff711fb) SHA1(34baeb72214484a7d026e0760c090650db2cebeb), 0x01C52CDC)
SAM1_ROM32MB(im_186ve,"im_186ve.bin",CRC(afb723b6) SHA1(11787b8c73883e417b92791fa2ab45416ee718ef), 0x01C52CDC)
SAM_INPUT_PORTS_START(im, 1)

CORE_GAMEDEF(im, 186ve, "Iron Man Vault Edition (V1.86)", 2020, "Stern", sam1, 0)
CORE_CLONEDEF(im, 100, 186ve, "Iron Man (V1.0)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(im, 110, 186ve, "Iron Man (V1.1)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(im, 120, 186ve, "Iron Man (V1.2)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(im, 140, 186ve, "Iron Man (V1.4)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(im, 160, 186ve, "Iron Man (V1.6)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(im, 181, 186ve, "Iron Man (V1.81)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(im, 182, 186ve, "Iron Man (V1.82)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(im, 183, 186ve, "Iron Man (V1.83)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(im, 183ve, 186ve, "Iron Man Vault Edition (V1.83)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(im, 185, 186ve, "Iron Man (V1.85)", 2020, "Stern", sam1, 0)
CORE_CLONEDEF(im, 185ve, 186ve, "Iron Man Vault Edition (V1.85)", 2020, "Stern", sam1, 0)
CORE_CLONEDEF(im, 186, 186ve, "Iron Man (V1.86)", 2020, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ TRON: Legacy
/-------------------------------------------------------------------*/
INITGAME(trn, GEN_SAM, sam_dmd128x32, SAM_5COL + 1, SAM_GAME_TRON) // Add custom column for ramp leds

SAM1_ROM32MB(trn_100h, "trn_100h.bin", CRC(4c2abebd) SHA1(8e22454932680351d58f863cf9644a9f3db24800), 0x01F19368)
SAM1_ROM32MB(trn_110,  "trn_110.bin",  CRC(bdaf1803) SHA1(f32d5bfb87be85483b0486bbb6f2858efca6efe5), 0x01AB97D8)
SAM1_ROM32MB(trn_120,  "trn_120.bin",  CRC(80dcb8a2) SHA1(9f0810969058222c0104f2b35d17e14bf3f5f8e8), 0x01C7C978)
SAM1_ROM32MB(trn_140,  "trn_140.bin",  CRC(7c9ce1bd) SHA1(c75a6a4c7f0d72460061e5532e1d604f6ea829e3), 0x01E464E8)
SAM1_ROM32MB(trn_150,  "trn_150.bin",  CRC(5ac4021b) SHA1(740187e7a60a32b1d21708a4194e0524211b53a7), 0x01F0420C)
SAM1_ROM32MB(trn_160,  "trn_160.bin",  CRC(38eb16b1) SHA1(4f18080d76e07a3308497116cf3e39a7fab4cd25), 0x01f0f314)
SAM1_ROM32MB(trn_170,  "trn_170.bin",  CRC(1f3b314d) SHA1(59df759539c02600d2579b4e59a184ac3db64020), 0x01F13C9C)
SAM1_ROM32MB(trn_110h, "trn_110h.bin", CRC(43a7e45a) SHA1(b03798f00fe481f662ed07fbf7a14766bccbb92e), 0x01F19368)
SAM1_ROM32MB(trn_130h, "trn_130h.bin", CRC(adf02601) SHA1(6e3c2706e39a1c01a002ceaea839b934cdac28bc), 0x01F1ED40)
SAM1_ROM32MB(trn_140h, "trn_140h.bin", CRC(7de92a4b) SHA1(87eb46e1564b8a913d6cc17a86b50828dd1273de), 0x01f286d8)
SAM1_ROM32MB(trn_174,  "trn_174.bin",  CRC(20e44481) SHA1(88e6e75efb640a7978f4003f0df5ee1e41087f72), 0x01F79E70)
SAM1_ROM32MB(trn_17402,"trn_17402.bin",CRC(94a5946c) SHA1(5026e33a8bb00c83caf06891727b8439d1274fbb), 0x01F79E70)
SAM1_ROM32MB(trn_174h, "trn_174h.bin", CRC(a45224bf) SHA1(40e36764af332175f653e8ddc2a8bb77891c1230), 0x01F93B84)

SAM_INPUT_PORTS_START(trn, 1)

CORE_GAMEDEF(trn, 174h, "TRON: Legacy Limited Edition (V1.74)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 100h, 174h, "TRON: Legacy Limited Edition (V1.0)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 110,  174h, "TRON: Legacy (V1.1)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 110h, 174h, "TRON: Legacy Limited Edition (V1.1)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 120,  174h, "TRON: Legacy (V1.2)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 130h, 174h, "TRON: Legacy Limited Edition (V1.3)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 140,  174h, "TRON: Legacy (V1.4)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 140h, 174h, "TRON: Legacy Limited Edition (V1.4)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 150,  174h, "TRON: Legacy (V1.5)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 160,  174h, "TRON: Legacy (V1.6)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 170,  174h, "TRON: Legacy (V1.7)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 174,  174h, "TRON: Legacy (V1.74)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(trn, 17402,174h, "TRON: Legacy (V1.7402)", 2013, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Transformers
/-------------------------------------------------------------------*/
INITGAME(tf, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL12)

SAM1_ROM32MB(tf_088h, "tf_088h.bin", CRC(a79ca893) SHA1(8f1228727422f5f99a20d60968eeca6c64f6c253), 0x01EB4CE8)
SAM1_ROM32MB(tf_100h, "tf_100h.bin", CRC(3be6ffc2) SHA1(c57d00af7ea189ea37ceed28bf85cff1054a1b8c), 0x01F4CCF0)
SAM1_ROM32MB(tf_120,  "tf_120.bin",  CRC(a6dbb32d) SHA1(ac1bef87278ff1ebc98d66cc062c3a7e49580a82), 0x01E306CC)
SAM1_ROM32MB(tf_140,  "tf_140.bin",  CRC(b41c8d33) SHA1(e96462df7481759d5c29a192766f03334b2b4562), 0x01D0C800)
SAM1_ROM32MB(tf_150,  "tf_150.bin",  CRC(130cbed2) SHA1(b299b1b25a6007cbec0ea15d2a156a197215e288), 0x01D0C800)
SAM1_ROM32MB(tf_160,  "tf_160.bin",  CRC(9dd1b01b) SHA1(10dd73ade7b662bf17b95c2413d23fa942e54660), 0x01D0C800)
SAM1_ROM32MB(tf_170,  "tf_170.bin",  CRC(cd8707e6) SHA1(847c37988bbc12e8200a6762c2851b610a0b849f), 0x01D24F34)
SAM1_ROM32MB(tf_180,  "tf_180.bin",  CRC(0b6e3a4f) SHA1(62e1328e8462680694157aca266055d57347e904), 0x01D24F34)
SAM1_ROM32MB(tf_120h, "tf_120h.bin", CRC(0f750246) SHA1(7ab3c9278f443511e5e7fcf062ffc9e8d1456396), 0x01EB1C4C)
SAM1_ROM32MB(tf_130h, "tf_130h.bin", CRC(8a1b676f) SHA1(d74f6060091293e6a781e129d19a408baabcf716), 0x01EB1C4C)
SAM1_ROM32MB(tf_140h, "tf_140h.bin", CRC(7e7920d6) SHA1(9db41874081d5f28adb5ab23903038f5c959eb1d), 0x01EB1C4C)
SAM1_ROM32MB(tf_150h, "tf_150h.bin", CRC(5cec6bfc) SHA1(30899241c2c0a9d42aa19fa3eb4180452bdaec91), 0x01EB1E5C)
SAM1_ROM32MB(tf_180h, "tf_180h.bin", CRC(467aeeb3) SHA1(feec42b083d81e632ef8ae402eb9f20f3104db08), 0x01EB1E04)

SAM_INPUT_PORTS_START(tf, 1)

CORE_GAMEDEF(tf, 180h, "Transformers Limited Edition (V1.8)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 088h, 180h, "Transformers Limited Edition (V0.88)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 100h, 180h, "Transformers Limited Edition (V1.0)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 120,  180h, "Transformers (V1.2)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 120h, 180h, "Transformers Limited Edition (V1.2)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 130h, 180h, "Transformers Limited Edition (V1.3)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 140,  180h, "Transformers (V1.4)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 140h, 180h, "Transformers Limited Edition (V1.4)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 150,  180h, "Transformers (V1.5)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 150h, 180h, "Transformers Limited Edition (V1.5)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 160,  180h, "Transformers (V1.6)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 170,  180h, "Transformers (V1.7)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(tf, 180,  180h, "Transformers (V1.8)", 2013, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ James Cameron's Avatar
/-------------------------------------------------------------------*/
INITGAME(avr, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL12)

SAM1_ROM32MB(avr_101h, "avr_101h.bin", CRC(dbdcc7e5) SHA1(bf9a79209ecdae93efb2930091d2658259a3bd03), 0x01EE1CB8)
SAM1_ROM32MB(avr_106,  "avr_106.bin",  CRC(695799e5) SHA1(3e216fd4273adb7417294b3e648befd69350ab25), 0x01ED31B4)
SAM1_ROM32MB(avr_110,  "avr_110.bin",  CRC(e28df0a8) SHA1(7bc42d329efcb59d71af1736d8881c14ce3f7e5e), 0x01D53CA4)
SAM1_ROM32MB(avr_120h, "avr_120h.bin", CRC(85a55e02) SHA1(204d796c2cbc776c1305dabade6306527122a13e), 0x01D53CA4)
SAM1_ROM32MB(avr_200,  "avr_200.bin",  CRC(dc225785) SHA1(ecaba25a470bf03e6e43ab8779d14898e1b8e67f), 0x01D53CA4)

SAM_INPUT_PORTS_START(avr, 1)

CORE_GAMEDEF(avr, 120h, "Avatar Limited Edition (V1.2)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(avr, 101h, 120h, "Avatar Limited Edition (V1.01)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(avr, 106,  120h, "Avatar (V1.06)", 2010, "Stern", sam1, 0)
CORE_CLONEDEF(avr, 110,  120h, "Avatar (V1.1)", 2011, "Stern", sam1, 0)
//Avatar Pro FTDI-USB CPU Board Part #520-5246-02 ONLY
CORE_CLONEDEF(avr, 200,  120h, "Avatar (V2.0)", 2013, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ The Rolling Stones
/-------------------------------------------------------------------*/
INITGAME(rsn, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_NO_AUX)

SAM1_ROM32MB(rsn_100h, "rsn_100h.bin", CRC(7cdb082a) SHA1(2f35057b80ffeec05cdbc62bc86da8a32f859425), 0x01EB50C8)
SAM1_ROM32MB(rsn_103,  "rsn_103.bin",  CRC(2039ac97) SHA1(4cbcc758fc74dd32f5804b9548645fba3431bdce), 0x01D38788)
SAM1_ROM32MB(rsn_105,  "rsn_105.bin",  CRC(58025883) SHA1(7f63bbe98f1151e0276ede1412ed5960ce9b3395), 0x01EB4FC0)
SAM1_ROM32MB(rsn_110,  "rsn_110.bin",  CRC(f4aad67f) SHA1(f5dc335a2b9cc92b3da9a33e24cd0b155c6385aa), 0x01EB4FC4)
SAM1_ROM32MB(rsn_110h, "rsn_110h.bin", CRC(f5122852) SHA1(b92461983d7a3b55ac8be4df4def1b4ca12327af), 0x01EB50CC)

SAM_INPUT_PORTS_START(rsn, 1)

CORE_GAMEDEF(rsn, 110h, "Rolling Stones, The Limited Edition (V1.1)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(rsn, 103,  110h, "Rolling Stones, The (V1.03)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(rsn, 105,  110h, "Rolling Stones, The (V1.05)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(rsn, 100h, 110h, "Rolling Stones, The Limited Edition (V1.0)", 2011, "Stern", sam1, 0)
CORE_CLONEDEF(rsn, 110,  110h, "Rolling Stones, The (V1.1)", 2011, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ AC/DC
/-------------------------------------------------------------------*/
INITGAME(acd, GEN_SAM, sam_dmd128x32, SAM_12COL, SAM_GAME_AUXSOL8_DSTB | SAM_GAME_ACDC_FLAMES)
 
SAM1_ROM128MB(acd_121,   "acd_121.bin",  CRC(4f5f43e9) SHA1(19045e9cdb2522770013c24c6fed265009278dea), 0x03D8F40C)
SAM1_ROM128MB(acd_125,   "acd_125.bin",  CRC(0307663f) SHA1(d40e3aaf94d1d314835fa59a177ce0c386399f4c), 0x03E53DEC)
SAM1_ROM128MB(acd_130,   "acd_130.bin",  CRC(da97014e) SHA1(f0a2684076008b0234c089fea8f95e4f3d8816dd), 0x040C4038)
SAM1_ROM128MB(acd_140,   "acd_140.bin",  CRC(43bbbf54) SHA1(33e3795ab850dfab1fd8b1b4f6364a696cc62aa9), 0x040C4038)
SAM1_ROM128MB(acd_150,   "acd_150.bin",  CRC(8b4c0fae) SHA1(d973be4306a7fc1ff9f898145197081cbe823584), 0x041308CC)
SAM1_ROM128MB(acd_150h,  "acd_150h.bin", CRC(1b8b823d) SHA1(9cda6a3f609e94d93126e105ac2945151006325b), 0x040F555C)
SAM1_ROM128MB(acd_152,   "acd_152.bin",  CRC(78cef38c) SHA1(656acabed2241587f512cdd53a095228d9642d1b), 0x04185458)
SAM1_ROM128MB(acd_152h,  "acd_152h.bin", CRC(bbf6b303) SHA1(8f29a5e8b5503df59ec8a6039a36e78cf7d871a9), 0x0414A0E8)
SAM1_ROM128MB(acd_160,   "acd_160.bin",  CRC(6b98a14c) SHA1(a34841b1e136c9647c89f83e2bf59ecdccb2a0fb), 0x04E942C8)
SAM1_ROM128MB(acd_160h,  "acd_160h.bin", CRC(733f15a4) SHA1(61e96ceac327387e84b8e24467aee2f5c0a8ce97), 0x04E942E0)
SAM1_ROM128MB(acd_161,   "acd_161.bin",  CRC(a0c27c59) SHA1(83d19fe6b344eb95866f7d5179b65ed26938b9da), 0x04E58B6C)
SAM1_ROM128MB(acd_161h,  "acd_161h.bin", CRC(1c66055b) SHA1(f33e5bd5753acc90202565639b6a8d22d6380054), 0x04E58B6C)
SAM1_ROM128MB(acd_163,   "acd_163.bin",  CRC(0bf53436) SHA1(0758d4a881ce87c9af90132741bf1e5c89fc575b), 0x04E5EFE8)
SAM1_ROM128MB(acd_163h,  "acd_163h.bin", CRC(12c404e8) SHA1(e3a5937abaa9e5b4b18b214b4f5c74f1c110247f), 0x04E5EFE8)
SAM1_ROM128MB(acd_165,   "acd_165.bin",  CRC(819b2b35) SHA1(f29814ba985a5887f5cd382666e7f14f8d6e3702), 0x07223FA0)
SAM1_ROM128MB(acd_165h,  "acd_165h.bin", CRC(9f9c41e9) SHA1(b4a61944218ab57af128e91b032a82342c8c4ccc), 0x07223FA0)
SAM1_ROM128MB(acd_168,   "acd_168.bin",  CRC(9fdcb32e) SHA1(f36b289e1868a051f4302b2551750b750fa52e30), 0x07223FA0)
SAM1_ROM128MB(acd_168h,  "acd_168h.bin", CRC(5a4246a1) SHA1(725eb666ffaef894d2bd694d412658395c7fa7f9), 0x07223FA0)
SAM1_ROM128MB(acd_170,   "acd_170.bin",  CRC(f44e175c) SHA1(8d355511bc22a489f7820057d3b32e1548864612), 0x07228E48)
SAM1_ROM128MB(acd_170h,  "acd_170h.bin", CRC(c2979275) SHA1(581db92af267095d2fc92bdc8aa9ff6595219ffe), 0x07228E48)
SAM1_ROM128MB(acd_170_ac,"acd_170_ac.bin",CRC(94984c1e) SHA1(72c9cdd309bd1c1986849b549c4e4d6b46cb2198), 0x07228E48)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(acd_168c,  "acd_168c.bin", NO_DUMP, 0x077FFFF0)
SAM1_ROM128MB(acd_168hc, "acd_168hc.bin",NO_DUMP, 0x077FFFF0)
SAM1_ROM128MB(acd_170c,  "acd_170c.bin", NO_DUMP, 0x077FFFF0)
SAM1_ROM128MB(acd_170hc, "acd_170hc.bin",NO_DUMP, 0x077FFFF0)
#endif

SAM_INPUT_PORTS_START(acd, 1)

CORE_GAMEDEF(acd, 170h, "AC/DC Limited Edition (V1.70.0)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 121,  170h, "AC/DC (V1.21)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 125,  170h, "AC/DC (V1.25)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 130,  170h, "AC/DC (V1.3)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 140,  170h, "AC/DC (V1.4)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 150,  170h, "AC/DC (V1.5)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 150h, 170h, "AC/DC Limited Edition (V1.5)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 152,  170h, "AC/DC (V1.52)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 152h, 170h, "AC/DC Limited Edition (V1.52)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 160,  170h, "AC/DC (V1.6)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 160h, 170h, "AC/DC Limited Edition (V1.6)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 161,  170h, "AC/DC (V1.61)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 161h, 170h, "AC/DC Limited Edition (V1.61)", 2012, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 163,  170h, "AC/DC (V1.63)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 163h, 170h, "AC/DC Limited Edition (V1.63)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 165,  170h, "AC/DC (V1.65)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 165h, 170h, "AC/DC Limited Edition (V1.65)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 168,  170h, "AC/DC (V1.68)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 168h, 170h, "AC/DC Limited Edition (V1.68)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 170,  170h, "AC/DC (V1.70.0)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 170_ac,170h,"American Country (AC/DC MOD)", 2024, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(acd, 168c, 170h, "AC/DC (V1.68) (Colored MOD)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 168hc,170h, "AC/DC Limited Edition (V1.68) (Colored MOD)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 170c, 170h, "AC/DC (V1.70.0) (Colored MOD)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(acd, 170hc,170h, "AC/DC Limited Edition (V1.70.0) (Colored MOD)", 2018, "Stern", sam2, 0)
#endif

/*-------------------------------------------------------------------
/ X-Men // uses sam1 and SAM1_ROM32MB instead of sam2 and SAM1_ROM128MB, should be okay
/-------------------------------------------------------------------*/
INITGAME(xmn, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL8_CSTB)

SAM1_ROM32MB(xmn_100,   "xmn_100.bin",  CRC(997b2973) SHA1(68bb379860a0fe5be6a8a8f28b6fd8fe640e172a), 0x01FB7DEC)
SAM1_ROM32MB(xmn_102,   "xmn_102.bin",  CRC(5df923e4) SHA1(28f86abc792008aa816d93e91dcd9b62fd2d01ee), 0x01FB7DEC)
SAM1_ROM32MB(xmn_120h,  "xmn_120h.bin", CRC(93da2d0b) SHA1(92c4c2e7fe6392e4ff8824d5b217dcbda8ce3a96), 0x01FB7DEC)
SAM1_ROM32MB(xmn_121h,  "xmn_121h.bin", CRC(7029ce71) SHA1(c7559ed963e18eecb8115214a3e154874c214f89), 0x01FB7DEC)
SAM1_ROM32MB(xmn_104,   "xmn_104.bin",  CRC(59f2e106) SHA1(10e9fb0ec72462654c0e4fb53c5cc9f2cbb3dbcb), 0x01FB7DEC)
SAM1_ROM32MB(xmn_122h,  "xmn_122h.bin", CRC(3609e1be) SHA1(86d368297ec6ca3b132c6e8dab17cd1c1c18bde2), 0x01FB7DEC)
SAM1_ROM32MB(xmn_123h,  "xmn_123h.bin", CRC(66c74598) SHA1(c0c0cd2e8e37eba6668aaadab76325afca103b32), 0x01FB7DEC)
SAM1_ROM32MB(xmn_105,   "xmn_105.bin",  CRC(e585d64b) SHA1(6126b29c9355398bac427e1b214e58e8e407bec4), 0x01FB850C)
SAM1_ROM32MB(xmn_124h,  "xmn_124h.bin", CRC(662591e9) SHA1(1abb26c589fbb1b5a4ec5577a4a842e8a84484a3), 0x01FB850C)
SAM1_ROM32MB(xmn_130,   "xmn_130.bin",  CRC(1fff4f39) SHA1(e8c02ab980499fbb81569ce1f191d0d2e5c13234), 0x01FB887C)
SAM1_ROM32MB(xmn_130h,  "xmn_130h.bin", CRC(b2a7f125) SHA1(a42834c3e562c239f56c27c0cb65885fdffd261c), 0x01FB887C)
SAM1_ROM32MB(xmn_150,   "xmn_150.bin",  CRC(fc579436) SHA1(2aa71da4a5f61165e41e7a63f3534202880c3b90), 0x01FA5268)
SAM1_ROM32MB(xmn_150h,  "xmn_150h.bin", CRC(8e2c3870) SHA1(ddfb4370bb4f32d440538f1432d1be09df9b5557), 0x01FA5268)
SAM1_ROM32MB(xmn_151,   "xmn_151.bin",  CRC(84c744a4) SHA1(db4339be7e9d47c46a13f95520dfe58da8450a19), 0x01FA5268)
SAM1_ROM32MB(xmn_151h,  "xmn_151h.bin", CRC(21d1088f) SHA1(9a0278c0324fbf549b5b7bcc93bc327f3eb65e19), 0x01FA5268)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM32MB(xmn_151c,  "xmn_151c.bin", NO_DUMP, 0x01FFFFF0)
SAM1_ROM32MB(xmn_151hc, "xmn_151hc.bin",NO_DUMP, 0x01FFFFF0)
#endif

SAM_INPUT_PORTS_START(xmn, 1)

CORE_GAMEDEF(xmn, 151h, "X-Men Limited Edition (V1.51)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 100,  151h, "X-Men (V1.0)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 102,  151h, "X-Men Limited Edition (V1.02)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 104,  151h, "X-Men (V1.04)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 105,  151h, "X-Men (V1.05)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 120h, 151h, "X-Men Limited Edition (V1.2)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 121h, 151h, "X-Men Limited Edition (V1.21)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 122h, 151h, "X-Men Limited Edition (V1.22)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 123h, 151h, "X-Men Limited Edition (V1.23)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 124h, 151h, "X-Men Limited Edition (V1.24)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 130,  151h, "X-Men (V1.3)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 130h, 151h, "X-Men Limited Edition (V1.3)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 150,  151h, "X-Men (V1.5)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 150h, 151h, "X-Men Limited Edition (V1.5)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 151,  151h, "X-Men (V1.51)", 2014, "Stern", sam1, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(xmn, 151c, 151h, "X-Men (V1.51) (Colored MOD)", 2014, "Stern", sam1, 0)
CORE_CLONEDEF(xmn, 151hc,151h, "X-Men Limited Edition (V1.51) (Colored MOD)", 2014, "Stern", sam1, 0)
#endif

/*-------------------------------------------------------------------
/ The Avengers // uses sam1 and SAM1_ROM32MB instead of sam2 and SAM1_ROM128MB, should be okay
/-------------------------------------------------------------------*/
INITGAME(avs, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL8_CSTB)

SAM1_ROM32MB(avs_110,  "avs_110.bin",  CRC(2cc01e3c) SHA1(0ae7c9ced7e1d48b0bf4afadb6db508e558a7ebb), 0x01D032AC)
SAM1_ROM32MB(avs_120h, "avs_120h.bin", CRC(a74b28c4) SHA1(35f65691312c547ec6c6bf52d0c5e330b5d464ca), 0x01E270D0)
SAM1_ROM32MB(avs_140,  "avs_140.bin",  CRC(92642508) SHA1(1d55cd178104b43377f079fd0209d74d1b10bea8), 0x01F2EDA0)
SAM1_ROM32MB(avs_140h, "avs_140h.bin", CRC(9b7e13f8) SHA1(eb97e92013a8d1d706a119b50d36d69eb26cb273), 0x01F2EDA0)
SAM1_ROM32MB(avs_170,  "avs_170.bin",  CRC(AA4A7203) SHA1(f2f4a9851097a07291f3469f94362f4cb1f7a127), 0x01F5C990)
SAM1_ROM32MB(avs_170h, "avs_170h.bin", CRC(07FEB01C) SHA1(25cca6c2f8fc2e3a38a72263cb25cefaf7f3b832), 0x01F5C990)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM32MB(avs_170c, "avs_170c.bin", NO_DUMP, 0x01FFFFF0)
SAM1_ROM32MB(avs_170hc,"avs_170hc.bin",NO_DUMP, 0x01FFFFF0)
#endif

SAM_INPUT_PORTS_START(avs, 1)

CORE_GAMEDEF(avs, 170h, "Avengers, The Limited Edition (V1.7)", 2016, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 110,  170h, "Avengers, The (V1.1)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 120h, 170h, "Avengers, The Limited Edition (V1.2)", 2012, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 140,  170h, "Avengers, The (V1.4)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 140h, 170h, "Avengers, The Limited Edition (V1.4)", 2013, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 170,  170h, "Avengers, The (V1.7)", 2016, "Stern", sam1, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(avs, 170c, 170h, "Avengers, The (V1.7) (Colored MOD)", 2016, "Stern", sam1, 0)
CORE_CLONEDEF(avs, 170hc,170h, "Avengers, The Limited Edition (V1.7) (Colored MOD)", 2016, "Stern", sam1, 0)
#endif

/*-------------------------------------------------------------------
/ Metallica
/-------------------------------------------------------------------*/
INITGAME(mtl, GEN_SAM, sam_dmd128x32, SAM_9COL, SAM_GAME_AUXSOL6 | SAM_GAME_METALLICA_MAGNET)

SAM1_ROM128MB(mtl_052,   "mtl_052.bin",  CRC(6150be49) SHA1(0095722d2a96c58c66a78180146c358e3b859817), 80135496)
SAM1_ROM128MB(mtl_103,   "mtl_103.bin",  CRC(9b073858) SHA1(129872e38d21d9d6d20f81388825113f13645bab), 0x04D24D04)
SAM1_ROM128MB(mtl_105,   "mtl_105.bin",  CRC(4699e2cf) SHA1(b56e85583362056b33f7b8eb6255d34d234ea5ea), 0x04DEDE5C)
SAM1_ROM128MB(mtl_106,   "mtl_106.bin",  CRC(5ac6c70a) SHA1(aaa68eebd1b894383416d2a491ac074a73be8d91), 0x04E8A214)
SAM1_ROM128MB(mtl_112,   "mtl_112.bin",  CRC(093ba7ef) SHA1(e49810ca3500be503343296105ea9dd85e2c00f0), 0x04FDCD28)
SAM1_ROM128MB(mtl_113,   "mtl_113.bin",  CRC(be73c2e7) SHA1(c91bbb554aaa21520360773e1215fe80557d6c2f), 0x04FDCD28)
SAM1_ROM128MB(mtl_113h,  "mtl_113h.bin", CRC(3392e27c) SHA1(3cfa5c5fdc51bd6886ffe6865739cf71de145ef1), 0x04FDCD28)
SAM1_ROM128MB(mtl_116,   "mtl_116.bin",  CRC(85793613) SHA1(454813cb405bb6bda1d26288b10606c3a4ec72fc), 0x050FFF04)
SAM1_ROM128MB(mtl_116h,  "mtl_116h.bin", CRC(3a96d383) SHA1(671fae4565c0739d98c9c40b05b9b41ae7917671), 0x050FFF04)
SAM1_ROM128MB(mtl_120,   "mtl120.bin",   CRC(43028b40) SHA1(64ab9306b28f3dc59e645ce49dbf3468e7f590bd), 0x0512C72C)
SAM1_ROM128MB(mtl_120h,  "mtl120h.bin",  CRC(1c5b4643) SHA1(8075e7ecf1031fc89e75cc5a5487340bc3fae507), 0x0512C72C)
SAM1_ROM128MB(mtl_122,   "mtl122.bin",   CRC(5201e6a6) SHA1(46e76f3e518448627419edf1aa08cc42259b39d2), 0x0512C72C)
SAM1_ROM128MB(mtl_122h,  "mtl122h.bin",  CRC(0d439a2b) SHA1(91eb84184cb93cfa83e0129a448760ac6586e85d), 0x0512C72C)
SAM1_ROM128MB(mtl_150,   "mtl150.bin",   CRC(647cbad4) SHA1(d49906906a075a656d7768bffc47bd88a6306699), 0x0542CA94)
SAM1_ROM128MB(mtl_150h,  "mtl150h.bin",  CRC(9f314ca1) SHA1(b620c4742a3ce137cbb099857d96fb6af67b7fec), 0x0542CA94)
SAM1_ROM128MB(mtl_151,   "mtl151.bin",   CRC(dac2819e) SHA1(c57fe4252a7a84cd543458bc54038b6ae9d79816), 0x05D44000)
SAM1_ROM128MB(mtl_151h,  "mtl151h.bin",  CRC(18e5a613) SHA1(ebd697bdc8f67188e160e2f8e76f908206127d26), 0x05D44000)
SAM1_ROM128MB(mtl_160,   "mtl160.bin",   CRC(c440d2f5) SHA1(4584542430579f9ff0174f4dd2817afbc778bc40), 0x05DD011C)
SAM1_ROM128MB(mtl_160h,  "mtl160h.bin",  CRC(cb69b0fe) SHA1(aa7275f33db95742a8b1ae8a5f6973a0b27953fa), 0x05DD011C)
SAM1_ROM128MB(mtl_163,   "mtl163.bin",   CRC(94d38355) SHA1(0f51c3d99e1227dcde132738ef539d0d452ca003), 0x05DD0138)
SAM1_ROM128MB(mtl_163d,  "mtl163d.bin",  CRC(de390393) SHA1(23a9f02514bc592e0799c91cd42786809bfc8c1d), 0x05DD0138)
SAM1_ROM128MB(mtl_163h,  "mtl163h.bin",  CRC(12c1a5bb) SHA1(701eda4251ebdfcce2bea3ec9c84ac9c35832e2f), 0x05DD0138)
SAM1_ROM128MB(mtl_164,   "mtl164.bin",   CRC(668b8cfa) SHA1(65c6bb31ace6b4ce70e99c42c040f734de235bd0), 0x05DD0138)
SAM1_ROM128MB(mtl_164h,  "mtl164h.bin",  CRC(ebbf5845) SHA1(4411279b3e4ea9621638bb81e47dc8753bfc0a05), 0x05DD0138)
SAM1_ROM128MB(mtl_170,   "mtl170.bin",   CRC(2bdc4668) SHA1(1c28f4d1e3a2c36a045cadb00a0aa8494b1d9243), 0x05DDD0BC)
SAM1_ROM128MB(mtl_170h,  "mtl170h.bin",  CRC(99d42a4e) SHA1(1983a6d1cd5664cf03599b035f520e0c6aa33632), 0x05DDD0BC)
SAM1_ROM128MB(mtl_180,   "mtl180.bin",   CRC(2d2e8cdc) SHA1(a426e71f5673dab5e2d36260d0e8edbbbcd4d34a), 0x05DFB40C)
SAM1_ROM128MB(mtl_180h,  "mtl180h.bin",  CRC(e37bc6e2) SHA1(1f2d74a4c22a369717cbbcfc4e9702fc03a52c7e), 0x05DFB40C)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(mtl_164c,  "mtl164c.bin",  NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(mtl_164hc, "mtl164hc.bin", NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(mtl_170c,  "mtl170c.bin",  NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(mtl_170hc, "mtl170hc.bin", NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(mtl_180c,  "mtl180c.bin",  NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(mtl_180hc, "mtl180hc.bin", NO_DUMP, 0x05FFFFF0)
#endif

SAM_INPUT_PORTS_START(mtl, 1)

CORE_GAMEDEF(mtl, 180h, "Metallica Limited Edition (V1.80.0)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 052,  180h, "Metallica (V0.52)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 103,  180h, "Metallica (V1.03)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 105,  180h, "Metallica (V1.05)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 106,  180h, "Metallica (V1.06)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 112,  180h, "Metallica (V1.12)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 113,  180h, "Metallica (V1.13)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 113h, 180h, "Metallica Limited Edition (V1.13)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 116,  180h, "Metallica (V1.16)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 116h, 180h, "Metallica Limited Edition (V1.16)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 120,  180h, "Metallica (V1.2)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 120h, 180h, "Metallica Limited Edition (V1.2)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 122,  180h, "Metallica (V1.22)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 122h, 180h, "Metallica Limited Edition (V1.22)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 150,  180h, "Metallica (V1.5)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 150h, 180h, "Metallica Limited Edition (V1.5)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 151,  180h, "Metallica (V1.51)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 151h, 180h, "Metallica Limited Edition (V1.51)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 160,  180h, "Metallica (V1.6)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 160h, 180h, "Metallica Limited Edition (V1.6)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 163,  180h, "Metallica (V1.63)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 163d, 180h, "Metallica (V1.63 LED)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 163h, 180h, "Metallica Limited Edition (V1.63)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 164,  180h, "Metallica (V1.64)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 164h, 180h, "Metallica Limited Edition (V1.64)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 170,  180h, "Metallica (V1.7)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 170h, 180h, "Metallica Limited Edition (V1.7)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 180,  180h, "Metallica (V1.80.0)", 2018, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(mtl, 164c, 180h, "Metallica (V1.64) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 164hc,180h, "Metallica Limited Edition (V1.64) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 170c, 180h, "Metallica (V1.7) (Colored MOD)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 170hc,180h, "Metallica Limited Edition (V1.7) (Colored MOD)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 180c, 180h, "Metallica (V1.80.0) (Colored MOD)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(mtl, 180hc,180h, "Metallica Limited Edition (V1.80.0) (Colored MOD)", 2018, "Stern", sam2, 0)
#endif

/*-------------------------------------------------------------------
/ Star Trek (Stern)
/-------------------------------------------------------------------*/
INITGAME(st, GEN_SAM, sam_dmd128x32, SAM_33COL, SAM_GAME_AUXSOL12)

SAM1_ROM128MB(st_120,  "st_120.bin",  CRC(dde9db23) SHA1(09e67564bce0ff7c67f1d16c4f9d8595f8130372), 0x02B14CFC)													
SAM1_ROM128MB(st_130,  "st_130.bin",  CRC(f501eb87) SHA1(6fa2f4e30cdd397d5443dfc690463495d22d9229), 0x02B71A98)													
SAM1_ROM128MB(st_140,  "st_140.bin",  CRC(c4f97ce3) SHA1(ef2d7cef153b5a6e9ab90c1ea31fdf5667eb327f), 0x02F7C398)													
SAM1_ROM128MB(st_140h, "st_140h.bin", CRC(6f84cec4) SHA1(d92391005eed3c4dcb66ac0bccd19a50c4120792), 0x02F7C398)
SAM1_ROM128MB(st_141h, "st_141h.bin", CRC(ae20d360) SHA1(0a840767b4e9fee26d7a4c2a9545fa7fd818d74e), 0x02F7C398)													
SAM1_ROM128MB(st_142h, "st_142h.bin", CRC(01acc115) SHA1(9881ea34852890a3fc960b78db96b70f17a28e56), 0x02F7C398)													
SAM1_ROM128MB(st_150,  "st_150.bin",  CRC(979c9644) SHA1(20a89ad337690a9ab652a599a77e30ccf2018e14), 0x035B2B94)													
SAM1_ROM128MB(st_150h, "st_150h.bin", CRC(a187581c) SHA1(b68ca52140bafd6b309b120d38df5b3bcf633a13), 0x035B2B94)													
SAM1_ROM128MB(st_160,  "st_160.bin",  CRC(cf0e0b60) SHA1(d91cfcd3ea28f174d9e7a9cff85a5ba6bccb0f34), 0x037ab5d4)													
SAM1_ROM128MB(st_160h, "st_160h.bin", CRC(80419698) SHA1(2c4514d1712d9503828c2f57bfbec465026ac012), 0x037ab5d4)													
SAM1_ROM128MB(st_161,  "st_161.bin",  CRC(e7a923ce) SHA1(d7f676a13bfa93b540af8469adb2bd20dda681a8), 0x037ab5d4)
SAM1_ROM128MB(st_161h, "st_161h.bin", CRC(74ad8a31) SHA1(18c940d021441ba87854f5eb6edb84aeffabdaae), 0x037ab5d4)
SAM1_ROM128MB(st_162,  "st_162.bin",  CRC(49cd0105) SHA1(48e040eb05e7cb781edacab6d04dd444ef5459a0), 0x037ab474)
SAM1_ROM128MB(st_162h, "st_162h.bin", CRC(2b42c5bb) SHA1(6190583b889f258afbdc0f99597823b18912d480), 0x037ab474)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(st_161c, "st_161c.bin", NO_DUMP, 0x037FFFF0)
SAM1_ROM128MB(st_161hc,"st_161hc.bin",NO_DUMP, 0x037FFFF0)
SAM1_ROM128MB(st_162c, "st_162c.bin", NO_DUMP, 0x037FFFF0)
SAM1_ROM128MB(st_162hc,"st_162hc.bin",NO_DUMP, 0x037FFFF0)
#endif

SAM_INPUT_PORTS_START(st, 1)

CORE_GAMEDEF(st, 162h, "Star Trek (Stern) Limited Edition (V1.62)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(st, 120,  162h, "Star Trek (Stern) (V1.2)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(st, 130,  162h, "Star Trek (Stern) (V1.3)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(st, 140,  162h, "Star Trek (Stern) (V1.4)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(st, 140h, 162h, "Star Trek (Stern) Limited Edition (V1.4)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(st, 141h, 162h, "Star Trek (Stern) Limited Edition (V1.41)", 2013, "Stern", sam2, 0)
CORE_CLONEDEF(st, 142h, 162h, "Star Trek (Stern) Limited Edition (V1.42)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(st, 150,  162h, "Star Trek (Stern) (V1.5)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(st, 150h, 162h, "Star Trek (Stern) Limited Edition (V1.5)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(st, 160,  162h, "Star Trek (Stern) (V1.6)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 160h, 162h, "Star Trek (Stern) Limited Edition (V1.6)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 161,  162h, "Star Trek (Stern) (V1.61)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 161h, 162h, "Star Trek (Stern) Limited Edition (V1.61)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 162,  162h, "Star Trek (Stern) (V1.62)", 2018, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(st, 161c, 162h, "Star Trek (Stern) (V1.61) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 161hc,162h, "Star Trek (Stern) Limited Edition (V1.61) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(st, 162c, 162h, "Star Trek (Stern) (V1.62) (Colored MOD)", 2018, "Stern", sam2, 0)
CORE_CLONEDEF(st, 162hc,162h, "Star Trek (Stern) Limited Edition (V1.62) (Colored MOD)", 2018, "Stern", sam2, 0)
#endif

/*-------------------------------------------------------------------
/ Mustang
/-------------------------------------------------------------------*/
INITGAME(mt, GEN_SAM, sam_dmd128x32, SAM_12COL,SAM_GAME_AUXSOL12)

SAM1_ROM128MB(mt_120,  "mt_120.bin",  CRC(be7437ac) SHA1(5db10d7f48091093c33d522a663f13f262c08c3e), 0x037DA5EC)													
SAM1_ROM128MB(mt_130,  "mt_130.bin",  CRC(b6086db1) SHA1(0a50864b0de1b4eb9a764f36474b6fddea767c0d), 0x0323573C)													
SAM1_ROM128MB(mt_130h, "mt_130h.bin", CRC(dcb5c923) SHA1(cf9e6042ae33080368ecffac233379135bf680ae), 0x03BF07F0)													
SAM1_ROM128MB(mt_140,  "mt_140.bin",  CRC(48010b61) SHA1(1bc615a86c4718ff407116a4e637e38e8386ded0), 0x03267340)													
SAM1_ROM128MB(mt_140h, "mt_140h.bin", CRC(FCB69947) SHA1(be64b13b3c6865f4fed1a8f03e9eaf84799fa2ab), 0x03C2CCC4)													
SAM1_ROM128MB(mt_140hb,"mt_140hb.bin",CRC(64B660E9) SHA1(01d0f0e61e99bc53ccde853f1604cec5ab0c59cf), 0x03C2CCC4)													
SAM1_ROM128MB(mt_145,  "mt_145.bin",  CRC(67a38387) SHA1(31626b54a5b2dd7fbc98c4b97ed84ce1a6705955), 0x03267340)													
SAM1_ROM128MB(mt_145h, "mt_145h.bin", CRC(20ec78b3) SHA1(95443dd1d545de409a692793ad609ed651cb61d8), 0x03C2CCC4)													
SAM1_ROM128MB(mt_145hb,"mt_145hb.bin",CRC(91fd5615) SHA1(0dbd7f3fc68218bcb10c893069d35447a445bc11), 0x03C2CCC4)													
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(mt_145c, "mt_145c.bin", NO_DUMP, 0x037FFFF0)
SAM1_ROM128MB(mt_145hc,"mt_145hc.bin",NO_DUMP, 0x03FFFFF0)
#endif

SAM_INPUT_PORTS_START(mt, 1)

CORE_GAMEDEF(mt, 145h, "Mustang Limited Edition (V1.45)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 120,  145h, "Mustang (V1.2)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 130,  145h, "Mustang (V1.3)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 130h, 145h, "Mustang Limited Edition (V1.3)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 140,  145h, "Mustang (V1.4)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 140h, 145h, "Mustang Limited Edition (V1.4)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 140hb,145h, "Mustang Boss (V1.4)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 145,  145h, "Mustang (V1.45)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 145hb,145h, "Mustang Boss (V1.45)", 2016, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(mt, 145c, 145h, "Mustang (V1.45) (Colored MOD)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(mt, 145hc,145h, "Mustang Limited Edition (V1.45) (Colored MOD)", 2016, "Stern", sam2, 0)
#endif

/*-------------------------------------------------------------------
/ The Walking Dead
/-------------------------------------------------------------------*/
INITGAME(twd, GEN_SAM, sam_dmd128x32, SAM_20COL, SAM_GAME_AUXSOL12)

SAM1_ROM128MB(twd_105,  "twd_105.ebi",  CRC(59b4e4d6) SHA1(642e827d58c9877a9f3c29b75784660894f045ad), 0x04F4FBF8)													
SAM1_ROM128MB(twd_111,  "twd_111.bin",  CRC(6b2faad0) SHA1(1f3dd34e5f7cd7ae539b39c0f3c87b966d2c2f45), 0x0512CF54)													
SAM1_ROM128MB(twd_111h, "twd_111h.bin", CRC(873feba1) SHA1(3b3a76c09d39550554b89b0a300f72a42722470e), 0x0512CF54)													
SAM1_ROM128MB(twd_119,  "twd_119.bin",  CRC(5fb5529e) SHA1(cdc3def52fd00219894327520122b905fd75ad1f), 0x0579167C)													
SAM1_ROM128MB(twd_119h, "twd_119h.bin", CRC(529089e0) SHA1(bcc5b3f6f549212dfdc36eece220af6913a22f78), 0x0579167C)													
SAM1_ROM128MB(twd_124,  "twd_124.bin",  CRC(9f30b0a9) SHA1(60f689717f9060260ef4ae32b11c3ca6e66004dc), 0x059DD02C)
SAM1_ROM128MB(twd_124h, "twd_124h.bin", CRC(85e24f0e) SHA1(c211676a1d202fa839f71f10e4168ffdc87a6159), 0x059DD02C)
SAM1_ROM128MB(twd_125,  "twd_125.bin",  CRC(2eaa2387) SHA1(15f597a839e6e9e95de34b9a4bce7efa30474f02), 0x059DD02C)
SAM1_ROM128MB(twd_125h, "twd_125h.bin", CRC(9a3b3ee6) SHA1(46609f708fcc7c6550bae024d7afd516e7fe46ad), 0x059DD02C)
SAM1_ROM128MB(twd_128,  "twd_128.bin",  CRC(42f2b016) SHA1(a2a59150286ab9cb14ae62b2fad9e7d8f53078fe), 0x059DD02C)
SAM1_ROM128MB(twd_128h, "twd_128h.bin", CRC(bc2fb73b) SHA1(52c97d92c040886bd3b7abea61754ec3795aba94), 0x059DD02C)
SAM1_ROM128MB(twd_141,  "twd_141.bin",  CRC(f160ecaf) SHA1(ecf09c58bfa68c4f65935f490bf5b3292cfe039d), 0x05BD11C0)
SAM1_ROM128MB(twd_141h, "twd_141h.bin", CRC(630e9c5e) SHA1(f3170ff138c30032b693b2b73ee704da879e3a0f), 0x05BD11C0)
SAM1_ROM128MB(twd_153,  "twd_153.bin",  CRC(59ca44ef) SHA1(1c89f9c5f6e15bcf2a245eb6c2ca5af6a19244ab), 0x05C80448)
SAM1_ROM128MB(twd_153h, "twd_153h.bin", CRC(941024A6) SHA1(5ed00791253d95efec4c07438053ddf0cc934238), 0x05C80448)
SAM1_ROM128MB(twd_156,  "twd_156.bin",  CRC(4bd62b0f) SHA1(b1d5e7d96f45fb3e076bc993b09a7eae9c73f610), 0x05C80448)
SAM1_ROM128MB(twd_156h, "twd_156h.bin", CRC(4594a287) SHA1(1e1a3b94bacf54a0c20cfa978db1284008c0e0a1), 0x05C80448)
SAM1_ROM128MB(twd_160,  "twd_160.bin",  CRC(44409cd9) SHA1(cace8725771e9fc09720a7a79f95abac44325232), 0x05C80448)
SAM1_ROM128MB(twd_160h, "twd_160h.bin", CRC(1ed7b80a) SHA1(1fbaa077ec834ff9d289008ef1169e0e7fd68271), 0x05C80448)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(twd_156c, "twd_156c.bin", NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(twd_156hc,"twd_156hc.bin",NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(twd_160c, "twd_160c.bin", NO_DUMP, 0x05FFFFF0)
SAM1_ROM128MB(twd_160hc,"twd_160hc.bin",NO_DUMP, 0x05FFFFF0)
#endif

SAM_INPUT_PORTS_START(twd, 1)

CORE_GAMEDEF(twd, 160h, "Walking Dead, The Limited Edition (V1.60.0)", 2017, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 105,  160h, "Walking Dead, The (V1.05)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 111,  160h, "Walking Dead, The (V1.11)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 111h, 160h, "Walking Dead, The Limited Edition (V1.11)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 119,  160h, "Walking Dead, The (V1.19)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 119h, 160h, "Walking Dead, The Limited Edition (V1.19)", 2014, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 124,  160h, "Walking Dead, The (V1.24)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 124h, 160h, "Walking Dead, The Limited Edition (V1.24)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 125,  160h, "Walking Dead, The (V1.25)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 125h, 160h, "Walking Dead, The Limited Edition (V1.25)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 128,  160h, "Walking Dead, The (V1.28)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 128h, 160h, "Walking Dead, The Limited Edition (V1.28)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 141,  160h, "Walking Dead, The (V1.41)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 141h, 160h, "Walking Dead, The Limited Edition (V1.41)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 153,  160h, "Walking Dead, The (V1.53)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 153h, 160h, "Walking Dead, The Limited Edition (V1.53)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 156,  160h, "Walking Dead, The (V1.56)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 156h, 160h, "Walking Dead, The Limited Edition (V1.56)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 160,  160h, "Walking Dead, The (V1.60.0)", 2017, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(twd, 156c, 160h, "Walking Dead, The (V1.56) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 156hc,160h, "Walking Dead, The Limited Edition (V1.56) (Colored MOD)", 2015, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 160c, 160h, "Walking Dead, The (V1.60.0) (Colored MOD)", 2017, "Stern", sam2, 0)
CORE_CLONEDEF(twd, 160hc,160h, "Walking Dead, The Limited Edition (V1.60.0) (Colored MOD)", 2017, "Stern", sam2, 0)
#endif

/*-------------------------------------------------------------------
/ Spider-Man Vault Edition (Stern)
/-------------------------------------------------------------------*/
INITGAME(smanve, GEN_SAM, sam_dmd128x32, SAM_2COL, SAM_GAME_AUXSOL12)

SAM1_ROM128MB(smanve_100, "smanve_100.bin",  CRC(f761fa19) SHA1(259bd6d42e742eaad1b7b50f9b5e4830c81084b0), 0x03F2CA8C)
SAM1_ROM128MB(smanve_101, "smanve_101.bin",  CRC(b7a525e8) SHA1(43fd9520225b11ba8ba5f9e8055689a652237983), 0x03F2CA8C)
#ifdef SAM_INCLUDE_COLORED
SAM1_ROM128MB(smanve_100c,"smanve_100c.bin", NO_DUMP, 0x03FFFFF0)
SAM1_ROM128MB(smanve_101c,"smanve_101c.bin", NO_DUMP, 0x03FFFFF0)
#endif

SAM_INPUT_PORTS_START(smanve, 1)

CORE_GAMEDEF(smanve, 101, "Spider-Man Vault Edition (V1.01)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(smanve, 100,  101, "Spider-Man Vault Edition (V1.0)", 2016, "Stern", sam2, 0)
#ifdef SAM_INCLUDE_COLORED
CORE_CLONEDEF(smanve, 100c, 101, "Spider-Man Vault Edition (V1.0) (Colored MOD)", 2016, "Stern", sam2, 0)
CORE_CLONEDEF(smanve, 101c, 101, "Spider-Man Vault Edition (V1.01) (Colored MOD)", 2016, "Stern", sam2, 0)
#endif
