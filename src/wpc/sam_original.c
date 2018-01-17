/************************************************************************************************
  Stern Pinball - S.A.M. Hardware System
  by Steve Ellenoff and Gerrit Volkenborn
  (09/25/2006 - 10/18/2006)

  Hardware from 01/2006 (World Poker Tour) - 09/2014 (The Walking Dead) (and Spider-Man Vault Edition 02/2016)

  CPU Board: S.A.M. System Board
  			 SPI Part Nº: 520-5246-00

  Issues:
  -USB Interface not hooked up or emulated
  -Serial Port Interfaces not hooked up (also requires support in the AT91 core)
  -FlashROM not emulated (treated as a regular read only rom)
  -Real Time Clock not hooked up or emulated
  -We can't hook up a sound commander easily since there's no external sound command sent, it's all
   internalized, so we'd need to look for each game's spot in RAM where they write a command,
   and send our commands there.
  -Not sure how the Hardware revision is read for display on the Service Screen.
  -Still a # of unmapped / unhandled address writes (maybe reads as well)
  -Later LE versions (starting from Star Trek LE (around 1.40??)) DMD animations are running in slow motion
  -GI?

  FIRQ frequency of 4008Hz was measured on real machine.

  Fixes history:
  5/25/2008 - Finally fixed the bug (reported by Destruk 11/15/07) that was causing crashes since
			  Spiderman 1.90 and recently 1.92, as well as Indiana Jones 1.13 - (Steve Ellenoff)
  10/24/2008 - Added on-table DMD panels and extra lamp columns for World Poker Tour, Family Guy,
              Wheel of Fortune, Shrek, Batman, and CSI (Gerrit Volkenborn)
  03/09/2012 - Added SAM2 generation for extended memory, and possible stereo support one day
************************************************************************************************/

#define SAM1_USE_REAL_FREQ	1							// Use real cpu frequency and sample rate //!! at least CSI and IJ4 need this, but FG timers are off/too slow (along with many other games)

#define SAM_USE_JIT

#ifdef SAM_USE_JIT
// We need the MAME scheduler to check for IRQ events more frequently than the FIQ if JIT is in use
// JIT does not relinqish CPU until the next timer event.  This causes input problems. Checking
// IRQs more frequently is a good use of the higher performing code anyhow. 
#define SAM_OVERSAMPLING 8
#else
#define SAM_OVERSAMPLING 1
#endif

//!! IJ/CSI has the old problems again

#if SAM1_USE_REAL_FREQ
	#define SAM1_ARMCPU_FREQ	40000000				// 40 MHZ - Marked on the schematic
	#define SAM1_ZC_FREQ	    120                     // was 111 for 60hz //112(?) for 50hz // This produces a 60Hz zero cross detection for Family Guy (and upwards) RoHs detection
    #define SAM1_FIRQ_FREQ		(4008*SAM_OVERSAMPLING) // measured on real machine, ~(SAM1_SOUNDFREQ/6) //!! with old SAM1_SOUNDFREQ (1600000/33/2) it was an experimental value of 4039 (found by destruk), originally 4040
    #define SAM1_SOUNDFREQ		24000                   // Same rate used by the AT91 hardware and BSMT2000
#else
    #define SAM1_ARMCPU_FREQ    55000000				// 55 MHZ - DMD Animations run closer to accurate
    #define SAM1_ZC_FREQ        146                     //!! has changed since ALU changes in ARM7core //143 for 60hz //144 for 50hz // This produces a 60Hz zero cross detection for Family Guy (and upwards) RoHs detection
    #define SAM1_FIRQ_FREQ		(4040*SAM_OVERSAMPLING) // (SAM1_SOUNDFREQ/6) // =4040, Sample rate / 6 samples per FIQ
    #define SAM1_SOUNDFREQ		1600000/33/2            //!! also change to 24000, etc
#endif

#define SNDBUFSIZE			    0x40000
#define SAM1_SOUND_CATCHUP_HACK                         // Catch up during sound buffer update, if generated sound and consumed sound is too far apart (e.g. somewhere some frequencies or CPU cycles do not match yet :/)

//Smoothing Options
#define SAM_DISPLAYSMOOTH       2                       // Smooth the display over this number of VBLANKS
#define SAM_SOLSMOOTH           24                      // Smooth the solenoids over this number of write cycles

//Game Options
#define SAM_WPT  1
#define SAM_FG   2
#define SAM_WOF  4
#define SAM_BDK  8
#define SAM_CSI 16

//Logging Options
#define LOGALL 0 // set to 0 to log NOTHING
#define LOG_RAW_SOUND_DATA			0	// Set to 1 to log all raw sample data to a file
#define LOG_TO_SCREEN				0	// Set to 1 to print log data to screen instead of logfile
#define LOG_SWITCH_READ_HANDLER		0	// Set to 1 to log the main Switch Read Handler
#define LOG_SWITCH_MATRIX			0   // Set to 1 to log the Switch Matrix reads
#define LOG_UNKNOWN1				0	// Set to 1 to log Unknown location read
#define LOG_IO_STAT					0   // Set to 1 to log IO Status reads
#define LOG_CS_W					0   // Set to 1 to log CS Writes
#define LOG_IOPORT_W				0   // Set to 1 to log IO Port Writes
#define LOG_LED1					0   // Set to 1 to log Led 1 Writes
#define LOG_LED2					0   // Set to 1 to log Led 2? Writes
#define LOG_ROM_BANK				0   // Set to 1 to log ROM BANK Writes
#define LOG_PORT_READ				0   // Set to 1 to log Port Reads

//Static Variables
static data32_t *sam1_reset_ram;
static data32_t *sam1_page0_ram;
static data32_t *nvram;

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  UINT32 solenoids;
  UINT8  solenoids2;
  int    diagnosticLed;
  int sw_stb;
  int zc;
  int video_page[2];
  INT16 samplebuf[2][SNDBUFSIZE];
  INT16 lastsamp[2];
  int sampout[2];
  int sampnum[2];
  int volume[2], mute[2];
  UINT16 value;
  int pass;
  int coindoor;
  int last_bank_data;
  UINT8 miniDMDData[14][16];
  int samVersion;
} samlocals;


#if LOG_RAW_SOUND_DATA
FILE *fpSND = NULL;
#endif

#if LOG_TO_SCREEN
#define LOG(x) printf x
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
		if( 0xFF<<(i*8) & mask )
		{
			offset=i;
			break;
		}
	}
	return offset;
}

/*****************/
/*  DMD Section  */
/*****************/

/*-- SAM DMD display uses 32 x 128 pixels by accessing 0x1000 bytes per page.
     That's 8 bits for each pixel, but they are distributed into 4 brightness
     bits (16 colors), and 4 translucency bits that perform the masking of the
     secondary or "background" page that will "shine through" if the mask bits
     of the foreground are set.
--*/
static PINMAME_VIDEO_UPDATE(sam1_dmd32_update) {
  static int color_table[16] = { 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 14, 14, 15 };
  tDMDDot dotCol;
  int ii,jj,kk,ll,mm;
  UINT32 *pp0, *pp1;

  for (ii = 0; ii < 32; ii++) {
    UINT8 *line = &dotCol[ii+1][0];
    for (jj = 0; jj < 128; jj+=4) {
      pp0  = (void *)(memory_region(REGION_CPU1) + 0x01080000 + (samlocals.video_page[0] << 12) + ii*128 + jj);
      pp1  = (void *)(memory_region(REGION_CPU1) + 0x01080000 + (samlocals.video_page[1] << 12) + ii*128 + jj);
      for (mm = 0; mm < 4; mm++) {
        ll = 15 & (pp0[0] >> (8*mm+4));
        kk = 15 & ((15 == ll ? pp1[0] : pp0[0]) >> (8*mm));
        *line++ = color_table[kk/*core_revnyb(((kk & 0x3) << 2) | ((kk & 0xc) >> 2))*/]; // no revnyb needed, otherwise f.e. has some issues on scoring fade (f.e. BDK) or highscore fade (f.e. FG)
      }
    }
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);

  return 0;
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
  UINT8 cds = coreGlobals.swMatrix[9];
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
static void ioport_w(int offset, int data)
{
  // with the current technique, a maximum of 14 displayed rows is possible (WPT only uses 10),
  // yet the amount of columns is virtually unlimited!
  static int rowMap[14] = { 5, 8, 7, 6, 9, 1, 2, 3, 4, 0, 10, 11, 12, 13 };
  static int smooth[40];
  static int lampcol = 0;
  static UINT8 auxstrb = 0;
  static UINT8 auxdata = 0;
  static int dataBlock = 0;
  static int colWrites;
  static int dataWrites[6];
  static int miniDMDCol = 0;
  static int miniDMDRow = 0;
  int i;
  switch (offset) {
		case 0x02400020:
			if (++dataWrites[0] == 1) {
				samlocals.solenoids = (samlocals.solenoids & 0xffff00ff) | (data << 8);
				for (i = 0; i < 8; i++) {
					if (data & (1 << i)) smooth[8 + i] = SAM_SOLSMOOTH; else smooth[8 + i]--;
					samlocals.solenoids |= (smooth[8 + i] > 0) << (8 + i);
				}
			}
			break;
		case 0x02400021:
			if (++dataWrites[1] == 1) {
				samlocals.solenoids = (samlocals.solenoids & 0xffffff00) | data;
				for (i = 0; i < 8; i++) {
					if (data & (1 << i)) smooth[i] = SAM_SOLSMOOTH; else smooth[i]--;
					samlocals.solenoids |= (smooth[i] > 0) << i;
				}
			}
			break;
		case 0x02400022:
			if (++dataWrites[2] == 1) {
				samlocals.solenoids = (samlocals.solenoids & 0xff00ffff) | (data << 16);
				for (i = 0; i < 8; i++) {
					if (data & (1 << i)) smooth[16 + i] = SAM_SOLSMOOTH; else smooth[16 + i]--;
					samlocals.solenoids |= (smooth[16 + i] > 0) << (16 + i);
				}
			}
			break;
		case 0x02400023:
			if (++dataWrites[3] == 1) {
				samlocals.solenoids = (samlocals.solenoids & 0x00ffffff) | (data << 24);
				for (i = 0; i < 8; i++) {
					if (data & (1 << i)) smooth[24 + i] = SAM_SOLSMOOTH; else smooth[24 + i]--;
					samlocals.solenoids |= (smooth[24 + i] > 0) << (24 + i);
				}
			}
			break;
		case 0x02400026:
			if (++dataWrites[4] == 1 && !(data & 0x80)) {
				samlocals.solenoids2 = (samlocals.solenoids2 & 0xff00) | data;
				for (i = 0; i < 8; i++) {
					if (data & (1 << i)) smooth[32 + i] = SAM_SOLSMOOTH; else smooth[32 + i]--;
					samlocals.solenoids2 |= (smooth[32 + i] > 0) << i;
				}
			}
      if (core_gameData->hw.gameSpecific1 & SAM_WPT) {
        if (miniDMDCol == 1) // the DMD row is decided by the bits set in the first two columns that are written to each column!
          miniDMDRow = rowMap[core_BitColToNum(((auxdata & 0x7f) << 7) | (data & 0x7f))];
        if (miniDMDCol > 1)
          samlocals.miniDMDData[miniDMDRow][miniDMDCol-2] = data & 0x7f;
        if (miniDMDCol < 17) miniDMDCol++; // limit the displayed column count to 16 for now
        if (data & ~auxdata & 0x80) // observe change low -> high to trigger the column reset
          miniDMDCol = 0;
      } else if (core_gameData->hw.gameSpecific1 & SAM_FG) {
        if (data & ~auxdata & 0x80) { // observe change low -> high to trigger the column reset
          miniDMDCol = 0;
          miniDMDRow = core_BitColToNum(data & 0x03); // 2 rows
        } else if (miniDMDCol < 3) {
          i = 10 + miniDMDCol + 3*miniDMDRow;
          coreGlobals.lampMatrix[i] = coreGlobals.tmpLampMatrix[i] = data & 0x7f;
          miniDMDCol++;
        }
      } else if (core_gameData->hw.gameSpecific1 & SAM_WOF) {
        if (data & ~auxdata & 0x80) { // observe change low -> high to trigger the column reset
          dataBlock = !dataBlock;
          miniDMDCol = 0;
        } else
          miniDMDCol++;
        if (dataBlock && miniDMDCol == 1) {
          miniDMDRow = core_BitColToNum(data & 0x1f); // 5 rows
        } else if (dataBlock && miniDMDCol > 1 && miniDMDCol < 7) {
          samlocals.miniDMDData[miniDMDRow][miniDMDCol-2] = data & 0x7f;
        } else if (!dataBlock && miniDMDCol < 6) {
          coreGlobals.lampMatrix[10+miniDMDCol] = coreGlobals.tmpLampMatrix[10+miniDMDCol] = data & 0x7f;
        }
      }
      auxdata = data;
			break;
		case 0x02400028:
      memset(dataWrites, 0, sizeof(dataWrites));
      if (++colWrites == 2)
        lampcol = core_BitColToNum(data);
      break;
		case 0x0240002a:
      colWrites = 0;
      if (++dataWrites[5] == 1)
        coreGlobals.tmpLampMatrix[lampcol] = core_revbyte(data);
      break;
		case 0x0240002b:
      if (core_gameData->hw.gameSpecific1 & SAM_WOF) {
        if (~auxstrb & data & 0x08) // LEDs
          dataBlock = 0;
        else if (~auxstrb & data & 0x10) // DMD
          dataBlock = 1;
      }
      if ((core_gameData->hw.gameSpecific1 & SAM_BDK) && (~data & 0x08)) // extra lamp column
        coreGlobals.lampMatrix[10] = coreGlobals.tmpLampMatrix[10] = core_revbyte(auxdata);
      if ((core_gameData->hw.gameSpecific1 & SAM_CSI) && (~data & 0x10)) // extra lamp column
        coreGlobals.lampMatrix[10] = coreGlobals.tmpLampMatrix[10] = core_revbyte(auxdata);
      if (~data & 0x40) // writes to beta-brite connector
        if (auxdata & 0x03) LOG(("%08x: writing to betabrite: %x\n",activecpu_get_pc(),auxdata & 0x03));
      auxstrb = data;
      break;
		default:
      LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
	}
}

static READ32_HANDLER(sam1_sw_r)
{
	int data = 0;
	int base = 0x01100000;
	int mask = ~mem_mask;
	int adj = adj_offset(mask);
	int realoff = (offset*4);
	int addr = base + realoff + adj;
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
		data = samlocals.last_bank_data;
	}

	if(logit) LOG(("%08x: reading from: %08x = %08x (mask=%x)\n",activecpu_get_pc(),offset,data,mask));

//	return data;
	return (data<<(adj*8));
}

/*
 IO Status
 ---------
 Status Data Read
 D0 = Interlock 20V
 D1 = Interlock 50V
 D2 = Zero Cross Circuit
 D3 = Lamp 1 Stat*
 D4 = Lamp 2 Stat*
 D5 = NA - GND
 D6 = NA - GND
 D7 = NA - GND

 Lamp1 Stat is tied to Lamp Column Drivers 1,3,5,7
 Lamp2 Stat is tied to Lamp Column Drivers 2,4,6,8

*/
static READ32_HANDLER(io_stat_r)
{
	int data = 0;
	int mask = ~mem_mask;
	int logit = LOG_IO_STAT;
	//This is read in the upper bits of the address.
	if(mask == 0xff00)
	{
		data = (0x1C & 0xFB) | (samlocals.zc<<2) | (samlocals.coindoor ? 3:0);
		data = data << 8;
	}
	if(logit) LOG(("%08x: reading from: %08x = %08x (mask=%x)\n",activecpu_get_pc(),0x02400024+offset,data,mask));
	return data;
}

static WRITE32_HANDLER(sam1_sw_w) {
  int mask = ~mem_mask;

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

static WRITE32_HANDLER(sam1_io2_w)
{
  LOG(("%08x: IO2 output to %05x=%02x\n", activecpu_get_pc(), offset, data)); //!! find out what is what for LE support (but even normal TWD uses it (and more??))
}

static WRITE32_HANDLER(sam1_cs_w)
{
	int base = 0x02400000;
	int mask = ~mem_mask;
	int adj = adj_offset(mask);
	int realoff = (offset*4);
	int addr = base + realoff + adj;
	int newdata = (data>>(adj*8));
	int logit = LOG_CS_W;
	offset = addr;
	data = newdata;

	//IO Board
	if( offset >= 0x2400000 && offset <= 0x24000ff )
	{
		logit = LOG_IOPORT_W;
		ioport_w(offset,data);
	}

	//1st LED?
	else if(offset == 0x2500000)
	{
		logit = LOG_LED1;
		samlocals.diagnosticLed &= 2;
		samlocals.diagnosticLed |= (data & 1);
	}

	//Rom Bank Select:
	//D0 = FF1(U42) -> A22
	//D1 = FF2(U42) -> A23
	//D2 = FF3(U42) -> A24	(Not Connected when used with a 256MBIT Flash Memory)
	//D3 = FF4(U42) -> A25	(Not Connected when used with a 256MBIT Flash Memory)
	else if(offset == 0x2580000)
	{
		logit = LOG_ROM_BANK;

		//log access to A24/A25 depending on the SAM version.
		if(data > (samlocals.samVersion > 1 ? 15 : 3))
			LOG(("ERROR IN BANK SELECT DATA = %d\n",data));

		//enforce validity to try and prevent crashes into illegal romspaces.
		data &= (samlocals.samVersion > 1 ? 0x0f : 0x03);

		//Swap bank memory
		cpu_setbank(1,memory_region(REGION_USER1) + data * 0x800000);

		//save value for read @ 1180000
		samlocals.last_bank_data = data;
	}

	//2nd LED?
	else if(offset == 0x2F00000)
	{
		logit = LOG_LED2;
		//LED is ON when zero (pulled to gnd)
		if(data == 0)
		{
			samlocals.diagnosticLed &= 1;
			samlocals.diagnosticLed |= 2;
		}
	}
	else logit = 1;

	if(logit) LOG(("%08x: writing to: %08x = %08x\n",activecpu_get_pc(),offset,data));
}

/********************/
/*  VBLANK Section  */
/********************/
static INTERRUPT_GEN(sam1_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  samlocals.vblankCount += 1;

  /*-- lamps --*/
  memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));

  /*-- display --*/
  if ((samlocals.vblankCount % (SAM_DISPLAYSMOOTH)) == 0) {
    coreGlobals.diagnosticLed = samlocals.diagnosticLed;
    samlocals.diagnosticLed = 0;
  }

  /*-- solenoids --*/
  coreGlobals.solenoids = samlocals.solenoids;
  coreGlobals.solenoids2 = samlocals.solenoids2 << 4;

  core_updateSw(1);
}

static SWITCH_UPDATE(sam1) {
  if (inports) {
	/*Switch Col 9 = Coin Slots*/
	CORE_SETKEYSW(inports[SE_COMINPORT], 0x0f, 9);
    /*Switch Col 0 = Tilt, Slam, Coin Door Buttons*/
    CORE_SETKEYSW(inports[SE_COMINPORT]>>4, 0xff, 0);
	/*Switch Col 2 = Start */
	CORE_SETKEYSW(inports[SE_COMINPORT]>>8, 0xc0, 2);
	/*Coin Door Switch - Not mapped to the switch matrix*/
	samlocals.coindoor = (inports[SE_COMINPORT] & 0x1000)?0:1;
  }
}

/*********************/
/* Port I/O Section  */
/*********************/
static READ32_HANDLER(sam1_port_r)
{
	int logit = LOG_PORT_READ;
	data32_t data;
	//Set P10,P11 to +1 as shown in schematic since they are tied to voltage.
	data = 0x00000C00;
	//Eventually need to add in P16,17,18 for feedback from Real Time Clock
	//Possibly also P23 (USB Related) & P24 (Dip #8)

	if(logit) LOG(("%08x: reading from %08x = %08x\n",activecpu_get_pc(),offset,data));

	return data;
}

//P16,17,18 are connected to Real Time Clock for writing data
//P3,P4,P5 are connected to PCM1755 DAC for controlling sound output & effects (volume and other features).
static WRITE32_HANDLER(sam1_port_w)
{
  // Bits 4 to 6 are used to issue a command to the PCM1755 chip as a serial 16-bit value.
  if ((data & 0x10) && samlocals.pass >= 0) samlocals.value |= ((data & 0x20) >> 5) << samlocals.pass--;
  if (data & 0x08) { // end of command data
    LOG(("Writing to PCM1755 register #$%02x = %02x\n", samlocals.value >> 8, samlocals.value & 0xff));
    switch (samlocals.value >> 8) { // register number is the upper command byte
      case 0x10: // left channel attenuation
        samlocals.volume[0] = samlocals.value & 0xff;
        break;
      case 0x11: // right channel attenuation
        samlocals.volume[1] = samlocals.value & 0xff;
        break;
      case 0x12: // soft mute
        samlocals.mute[0] = samlocals.value & 1;
        samlocals.mute[1] = samlocals.value & 2 >> 1;
        break;
      //!! other commands?
    }
    //Mixer set volume has volume range of 0..100, while the SAM1 hardware uses a 128 value range for volume. Volume values start @ 0x80 for some reason.
    mixer_set_volume(0, samlocals.mute[0] ? 0 : (samlocals.volume[0] & 0x7f) * 50 / 63);
    mixer_set_volume(1, samlocals.mute[1] ? 0 : (samlocals.volume[1] & 0x7f) * 50 / 63);
    samlocals.pass = 16; samlocals.value = 0;
  }
  if (data & 0x70000) LOG(("SET RTC: %x%x%x\n", (data >> 18) & 1, (data >> 17) & 1, (data >> 16) & 1));
}

//Toggle Zero Cross bit
static void sam1_zc(int data)
{
    samlocals.zc = !samlocals.zc;
}

//Trigger an FIQ Interrupt - This drives the sound output
static INTERRUPT_GEN(sam1_firq)
{
#ifdef SAM_USE_JIT
	static int count = 0;

	count++;
	if (count == SAM_OVERSAMPLING)
	{
		cpu_set_irq_line(0, AT91_SW_IRQ, PULSE_LINE);
		count = 0;
	}
#else
	cpu_set_irq_line(0, AT91_SW_IRQ, PULSE_LINE);
#endif
}

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(sam1)
{
  core_nvram(file, read_or_write, nvram, 0x20000, 0xff);		//128K NVRAM
}


/**********************/
/* SAM1 SOUND SECTION */
/**********************/
static void sam1_sh_update(int num, INT16 *buffer[2], int length)
{
  int ii, jj;
  for (jj = 0; jj < 2; jj++) {
    /* fill in with samples until we hit the end or run out */
    for (ii = 0; ii < length; ii++) {
    	if(samlocals.sampout[jj] == samlocals.sampnum[jj]
#if 0
			|| samlocals.sampnum[jj] < 64 //!! check is stupid due to wrap around?!
#endif
			) {
    		#ifdef MAME_DEBUG
    		LOG(("not enough samples to play\n"));
    		#endif
    		break;	//drop out of loop
    	}
    
    	//Send next pcm sample to output buffer and store last output
    	samlocals.lastsamp[jj] = buffer[jj][ii] = samlocals.samplebuf[jj][samlocals.sampout[jj]];
    
    	//Loop to beginning if we reach end of pcm buffer
    	samlocals.sampout[jj] = (samlocals.sampout[jj] + 1) % SNDBUFSIZE;
    }

    /* fill the rest with last sample output */ //!! should only be needed, if at all, initially?
    for ( ; ii < length; ii++)
    	buffer[jj][ii] = samlocals.lastsamp[jj];

#ifdef SAM1_SOUND_CATCHUP_HACK
	/* if sound is more than 1s/10 = 100ms apart, then catch up */
	if (samlocals.sampnum[jj] - samlocals.sampout[jj] > SAM1_SOUNDFREQ / 10)
		samlocals.sampout[jj] = samlocals.sampnum[jj];
#endif
  }
}

static int sam1_sh_start(const struct MachineSound *msound)
{
	const char* stream_name[2] = { "Channel #1", "Channel #2" };
	int volume[2] = { MIXER(100,MIXER_PAN_LEFT), MIXER(100,MIXER_PAN_RIGHT) };
	/*-- allocate a DAC stream at fixed frequency --*/
	return stream_init_multi
	  (2,
	  stream_name,
	  volume,
	  SAM1_SOUNDFREQ,
	  0,
	  sam1_sh_update) < 0;
}

static void sam1_sh_stop(void) { }
static WRITE_HANDLER(scmd_w) { }
static WRITE_HANDLER(man3_w) { }
static void sam1snd_init(struct sndbrdData *brdData) { }

const struct sndbrdIntf sam1Intf = {
	"SAM1", sam1snd_init, NULL, NULL, man3_w, scmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

static struct CustomSound_interface sam1CustIntf =
{
	sam1_sh_start,
	sam1_sh_stop,
	0,
};

static WRITE32_HANDLER(soundram_w)
{
	int mask = ~mem_mask;

	//if upper mask is used, shift the data
	if(mask & 0xffff0000) {
		data = data >> 16;
		//Store 16 bit data to our buffer
		samlocals.samplebuf[0][samlocals.sampnum[0]] = data & 0xffff;
		samlocals.sampnum[0] = (samlocals.sampnum[0] + 1) % SNDBUFSIZE;
	} else {
		//Store 16 bit data to our buffer
		samlocals.samplebuf[1][samlocals.sampnum[1]] = data & 0xffff;
		samlocals.sampnum[1] = (samlocals.sampnum[1] + 1) % SNDBUFSIZE;
	}

	#if LOG_RAW_SOUND_DATA
	if(fpSND)
		fwrite(&data,2,1,fpSND);
	#endif
}


/*****************************/
/*  Memory map for SAM1 CPU  */
/*****************************/
static MEMORY_READ32_START(sam1_readmem)
{0x00000000,0x000FFFFF,MRA32_RAM},					//Boot RAM
{0x00300000,0x003FFFFF,MRA32_RAM},					//Swapped RAM
{0x01000000,0x0107FFFF,MRA32_RAM},					//U13 RAM - General Usage (Code is executed here sometimes)
{0x01080000,0x0109EFFF,MRA32_RAM},					//U13 RAM - DMD Data for output
{0x0109F000,0x010FFFFF,MRA32_RAM},					//U13 RAM - Sound Data for output
{0x01100000,0x011fffff,sam1_sw_r},					//Various Input Signals
{0x02000000,0x020fffff,MRA32_RAM},					//U9 Boot Flash Eprom
{0x02100000,0x0211ffff,MRA32_RAM},					//U11 NVRAM (128K)
{0x02400024,0x02400027,io_stat_r},					//I/O Related
{0x03000000,0x030000ff,MRA32_RAM},					//USB Related
{0x04000000,0x047FFFFF,MRA32_RAM},					//1st 8MB of Flash ROM U44 Mapped here
{0x04800000,0x04FFFFFF,MRA32_BANK1},				//Banked Access to Flash ROM U44 (including 1st 8MB ALSO!)
MEMORY_END

static MEMORY_WRITE32_START(sam1_writemem)
{0x00000000,0x000FFFFF,MWA32_RAM,&sam1_page0_ram},	//Boot RAM
{0x00300000,0x003FFFFF,MWA32_RAM,&sam1_reset_ram},	//Swapped RAM
{0x01000000,0x0107FFFF,MWA32_RAM},					//U13 RAM - General Usage (Code is executed here sometimes)
{0x01080000,0x0109EFFF,MWA32_RAM},					//U13 RAM - DMD Data for output
{0x0109F000,0x010FFFFF,soundram_w},					//U13 RAM - Sound Data for output
{0x01100000,0x01ffffff,sam1_sw_w},					//Various Output Signals
{0x02100000,0x0211ffff,MWA32_RAM,&nvram},			//U11 NVRAM (128K) 0x02100000,0x0211ffff
{0x02200000,0x022fffff,sam1_io2_w},					//LE versions: more I/O stuff (mostly LED lamps) //!!
{0x02400000,0x02ffffff,sam1_cs_w},					//I/O Related
{0x03000000,0x030000ff,MWA32_RAM},					//USB Related
{0x04000000,0x047FFFFF,MWA32_RAM},					//1st 8MB of Flash ROM U44 Mapped here
{0x04800000,0x04FFFFFF,MWA32_BANK1},				//Banked Access to Flash ROM U44 (including 1st 8MB ALSO!)
MEMORY_END

/*****************************/
/*  Port map for SAM1 CPU    */
/*****************************/
//AT91 has only 1 port address it writes to - all 32 ports are sent via each bit of a 32 bit double word.
//However, if I didn't use 0-0xFF as a range it crashed for some reason.
static PORT_READ32_START( sam1_readport )
	{ 0x00,0xFF, sam1_port_r },
PORT_END
static PORT_WRITE32_START( sam1_writeport )
	{ 0x00,0xFF, sam1_port_w },
PORT_END

static MACHINE_INIT(sam1) {
#ifdef SAM_USE_JIT
	//set up the JIT memory map
	at91_init_jit(0, 0x1080000);
#endif
	//because the boot rom code gets written to ram, and then remapped to page 0, we need an interface to handle this.
	at91_set_ram_pointers(sam1_reset_ram,sam1_page0_ram);

	#if LOG_RAW_SOUND_DATA
	fpSND = fopen("sam1snd.raw","wb");
	if(!fpSND)
		LOG(("Unable to create sam1snd.raw file\n"));
	#endif
}

static MACHINE_RESET(sam1) {
	memset(&samlocals, 0, sizeof(samlocals));
	samlocals.samVersion = 1;
	samlocals.pass = 16;
	samlocals.coindoor = 1;
}

static MACHINE_RESET(sam2) {
	memset(&samlocals, 0, sizeof(samlocals));
	samlocals.samVersion = 2;
	samlocals.pass = 16;
	samlocals.coindoor = 1;
}

static MACHINE_STOP(sam1) {
	#if LOG_RAW_SOUND_DATA
	if(fpSND) fclose(fpSND);
	#endif
}

/********************************************/
/* S.A.M. Generation #1 - Machine Definition */
/********************************************/
MACHINE_DRIVER_START(sam1)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_SWITCH_UPDATE(sam1)
  MDRV_CPU_ADD(AT91, SAM1_ARMCPU_FREQ)
  MDRV_CPU_MEMORY(sam1_readmem, sam1_writemem)
  MDRV_CPU_PORTS(sam1_readport, sam1_writeport)
  MDRV_CPU_VBLANK_INT(sam1_vblank, 1)
  MDRV_CPU_PERIODIC_INT(sam1_firq, SAM1_FIRQ_FREQ)
  MDRV_CORE_INIT_RESET_STOP(sam1, sam1, sam1)
  MDRV_DIPS(8)
  MDRV_NVRAM_HANDLER(sam1)
  MDRV_TIMER_ADD(sam1_zc, SAM1_ZC_FREQ)
  MDRV_SOUND_ADD(CUSTOM,  sam1CustIntf)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
  MDRV_DIAGNOSTIC_LEDH(2)
MACHINE_DRIVER_END

/********************************************/
/* S.A.M. Generation #2 - Machine Definition */
/********************************************/
MACHINE_DRIVER_START(sam2)
  MDRV_IMPORT_FROM(sam1)
  MDRV_CORE_INIT_RESET_STOP(sam1, sam2, sam1)
MACHINE_DRIVER_END

/*-------------------------
/ Machine driver constants
/--------------------------*/
/*-- Common Inports for SAM1 Games --*/
#define SAM1_COMPORTS \
  PORT_START /* 0 */ \
	/*Switch Col. 0*/ \
    COREPORT_BITDEF(  0x0010, IPT_TILT,         KEYCODE_INSERT)  \
    COREPORT_BIT   (  0x0020, "Slam Tilt",      KEYCODE_HOME) \
    COREPORT_BIT   (  0x0040, "Ticket Notch",   KEYCODE_O) \
    COREPORT_BIT   (  0x0080, "Reserved",       KEYCODE_L) \
    COREPORT_BIT   (  0x0100, "Back",           KEYCODE_7) \
    COREPORT_BIT   (  0x0200, "Minus",          KEYCODE_8) \
    COREPORT_BIT   (  0x0400, "Plus",           KEYCODE_9) \
    COREPORT_BIT   (  0x0800, "Select",         KEYCODE_0) \
	/*Switch Col. 2*/ \
	COREPORT_BIT   (  0x8000, "Start Button",   KEYCODE_1) \
	COREPORT_BIT   (  0x4000, "Tournament Start", KEYCODE_2) \
    /*Switch Col. 9*/ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,        KEYCODE_3) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,        KEYCODE_4) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,        KEYCODE_5) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,        KEYCODE_6) \
	/*None*/ \
	COREPORT_BITTOG(  0x1000, "Coin Door",      KEYCODE_END) \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x001f, 0x0000, "Country") \
      COREPORT_DIPSET(0x0000, "USA" ) \
      COREPORT_DIPSET(0x000d, "Australia" ) \
      COREPORT_DIPSET(0x0001, "Austria" ) \
      COREPORT_DIPSET(0x0002, "Belgium" ) \
      COREPORT_DIPSET(0x0003, "Canada 1" ) \
      COREPORT_DIPSET(0x001a, "Canada 2" ) \
      COREPORT_DIPSET(0x0013, "Chuck-E-Cheese" ) \
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
      COREPORT_DIPSET(0x001c, "Unknown (00011100)" ) \
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
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0080, "1" )

/*-- Standard input ports --*/
#define SAM1_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    SAM1_COMPORTS

//SAM1 Display Data
static struct core_dispLayout sam1_dmd128x32[] = {
  {0,0, 32,128, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_dmd32_update}, {0}
};

static PINMAME_VIDEO_UPDATE(sam1_minidmd_update) {
  int digit = 7 * (layout->top - 34) / 9 + (layout->left - 10) / 7;
  UINT16 *seg = &coreGlobals.drawSeg[(digit % 7)*5];
  UINT16 bits;
  tDMDDot dotCol;
  int ii, jj;
  for (jj = 0; jj < 5; jj++) {
    for (ii = 0; ii < 7; ii++) {
      UINT8 *line = &dotCol[ii+1][jj];
      *line = (samlocals.miniDMDData[jj + (digit > 6 ? 5 : 0)][digit % 7]) & (1 << (6-ii)) ? 3 : 0;
    }
  }
  for (ii = 0; ii < 5; ii++) {
    bits = *seg;
    bits &= (digit > 6 ? 0xff00 : 0x00ff);
    for (jj = 0; jj < 7; jj++)
      bits |= (dotCol[jj+1][ii] ? 1 : 0) << ((6-jj) + 8*(digit > 6 ? 0 : 1));
    *seg++ = bits;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

static struct core_dispLayout sam1_wptDisp[] = {
  {0,0, 32,128, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_dmd32_update},
  {34,10, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,17, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,24, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,31, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,38, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,45, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {34,52, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,10, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,17, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,24, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,31, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,38, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,45, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {43,52, 7, 5, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd_update},
  {0}
};

static PINMAME_VIDEO_UPDATE(sam1_minidmd2_update) {
  UINT16 *seg = &coreGlobals.drawSeg[0];
  tDMDDot dotCol;
  int ii, jj, kk;
  for (jj = 0; jj < 5; jj++) {
    for (ii = 0; ii < 5; ii++) {
      UINT8 *line = &dotCol[ii+1][jj*7];
      for (kk = 0; kk < 7; kk++) {
        *line++ = samlocals.miniDMDData[ii][jj] & (1 << (6-kk)) ? 3 : 0;
      }
    }
  }
  for (ii = 0; ii < 35; ii++) {
    *seg++ = (dotCol[1][ii] ? 16 : 0) | (dotCol[2][ii] ? 8 : 0)
      | (dotCol[3][ii] ? 4 : 0) | (dotCol[4][ii] ? 2 : 0) | (dotCol[5][ii] ? 1 : 0);
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

static struct core_dispLayout sam1_wofDisp[] = {
  {0,0, 32,128, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_dmd32_update},
  {34,10, 5,35, CORE_DMD|CORE_DMDNOAA, (genf *)sam1_minidmd2_update},
  {0}
};

/**********************/
/* ROM LOADING MACROS */
/**********************/

/* BOOT FLASH ROM */
#define SAM1_BOOTFLASH(game,rom,len,chk) \
ROM_START(game) \
	ROM_REGION32_LE(0x2000000, REGION_USER1,0) \
		ROM_LOAD(rom, 0, len, chk) \
	ROM_REGION32_LE(0x5000000, REGION_CPU1, ROMREGION_ERASEMASK) \
	ROM_COPY(REGION_USER1,0,0,0x100000) \
	ROM_COPY(REGION_USER1,0,0x2000000,len) \
ROM_END

/* 32MB ROM */
#define SAM1_ROM32MB(game,rom,len,chk) \
ROM_START(game) \
	ROM_REGION32_LE(0x2000000, REGION_USER1,0) \
		ROM_LOAD(rom, 0, len, chk) \
	ROM_REGION32_LE(0x5000000, REGION_CPU1, ROMREGION_ERASEMASK) \
	ROM_COPY(REGION_USER1,0,0,0x100000) \
	ROM_COPY(REGION_USER1,0,0x4000000,0x800000) \
	ROM_COPY(REGION_USER1,0,0x4800000,0x800000) \
ROM_END

/* 128MB ROM */
#define SAM1_ROM128MB(game,rom,len,chk) \
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
#define SAM1_INIT(name, disp, lamps, flags) \
SAM1_INPUT_PORTS_START(name, 1) INPUT_PORTS_END \
static core_tGameData name##_GameData = {GEN_SAM, disp, {FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L), 0, lamps, 0, 0, 0, flags }}; \
static void init_##name(void) { \
  core_gameData = &name##_GameData; \
}

/*-------------------------------------------------------------------
/ S.A.M. Boot Flash
/-------------------------------------------------------------------*/
SAM1_INIT(sam1_flashb, sam1_dmd128x32, 2, 0)
SAM1_BOOTFLASH(sam1_flashb_0310,"boot_310.bin",0x100000,CRC(de017f82) SHA1(e4a9a818fa3f1754374cd00b52b8a087d6c442a9))
CORE_GAMEDEF(sam1_flashb, 0310, "S.A.M. Boot Flash Update (3.1)", 2008, "Stern", sam1, 0)

SAM1_BOOTFLASH(sam1_flashb_0230,"boot_230.bin",0xf0624,CRC(a4258c49) SHA1(d865edf7d1c6d2c922980dd192222dc24bc092a0))
CORE_CLONEDEF(sam1_flashb, 0230, 0310, "S.A.M. Boot Flash Update (2.3)", 2007, "Stern", sam1, 0)

SAM1_BOOTFLASH(sam1_flashb_0210,"boot_210.bin",0xf0304,CRC(0f3fd4a4) SHA1(115d0b73c40fcdb2d202a0a9065472d216ca89e0))
CORE_CLONEDEF(sam1_flashb, 0210, 0310, "S.A.M. Boot Flash Update (2.10)", 2007, "Stern", sam1, 0)

SAM1_BOOTFLASH(sam1_flashb_0106,"boot_106.bin",0xE8ac8,CRC(fe7bcece) SHA1(775590bbd52c24950db86cc231566ba3780030d8))
CORE_CLONEDEF(sam1_flashb, 0106, 0310, "S.A.M. Boot Flash Update (1.06)", 2006, "Stern", sam1, 0)

SAM1_BOOTFLASH(sam1_flashb_0102,"boot_102.bin",0x100000,CRC(92c93cba) SHA1(aed7ba2f988df8c95e2ad08f70409152d5caa49a))
CORE_CLONEDEF(sam1_flashb, 0102, 0310, "S.A.M. Boot Flash Update (1.02)", 2006, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ World Poker Tour
/-------------------------------------------------------------------*/
SAM1_INIT(wpt, sam1_wptDisp, 2, SAM_WPT)
SAM1_ROM32MB(wpt_140a,"wpt1400a.bin",0x19BB1EC,CRC(4b287770) SHA1(e19b60a08de9067a2b4c4dd71783fc812b3c7648))
CORE_GAMEDEF(wpt, 140a, "World Poker Tour (14.0)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140al,"wpt140al.bin",0x1B2EC00,CRC(2f03204b) SHA1(c7a0b645258dc1aca6a297641bc5cc10c255d5a7))
CORE_CLONEDEF(wpt, 140al, 140a, "World Poker Tour (14.0 English/Spanish)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140af,"wpt140af.bin",0x1A46D40,CRC(bed3e3f1) SHA1(43b9cd6deccc8e516e2f5e99295b751ccadbac29))
CORE_CLONEDEF(wpt, 140af, 140a, "World Poker Tour (14.0 English/French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140ai,"wpt140ai.bin",0x1A8C908,CRC(12a62641) SHA1(680283a7493921904f7fe9fae10d965db839f986))
CORE_CLONEDEF(wpt, 140ai, 140a, "World Poker Tour (14.0 English/Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140f,"wpt1400f.bin",0x1AA4030,CRC(3c9ce123) SHA1(5e9f6c9e5d4cdba36b7eacc24b602ea4dde92514))
CORE_CLONEDEF(wpt, 140f, 140a, "World Poker Tour (14.0 French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140g,"wpt1400g.bin",0x1A33E3C,CRC(5f8216da) SHA1(79b79acf7c457e6d70af458712bf946094d08d2a))
CORE_CLONEDEF(wpt, 140g, 140a, "World Poker Tour (14.0 German)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140gf,"wpt140gf.bin",0x1A74BD0,CRC(7be526fa) SHA1(a42e5c2c1fde9ab97d7dcfe64b8c0055372729f3))
CORE_CLONEDEF(wpt, 140gf, 140a, "World Poker Tour (14.0 German/French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140i,"wpt1400i.bin",0x1B2C740,CRC(9f19ed03) SHA1(4ef570be084b1e5196a19b7f516f621025c174bc))
CORE_CLONEDEF(wpt, 140i, 140a, "World Poker Tour (14.0 Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_140l,"wpt1400l.bin",0x1C7072C,CRC(00eff09c) SHA1(847203d4d2ce8d11a5403374f2d5b6dda8458bc9))
CORE_CLONEDEF(wpt, 140l, 140a, "World Poker Tour (14.0 Spanish)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_1129af,"wpt1129af.bin",0x1A46CF0,CRC(e5660763) SHA1(72b8b878aa8272f9cf54fde2c9ddc7635757e59c))
CORE_CLONEDEF(wpt, 1129af, 140a, "World Poker Tour (1.129 English/French)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_112a,"wpt112a.bin",0x19BB1B0,CRC(b98b4bf9) SHA1(75257a2759978d5fc699f78e809543d1cc8c456b))
CORE_CLONEDEF(wpt, 112a, 140a, "World Poker Tour (1.12)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112al,"wpt112al.bin",0x1b2ebc4,CRC(2c0dc704) SHA1(d5735977463ee92d87aba3a41d368b92a76b2908))
CORE_CLONEDEF(wpt, 112al, 140a, "World Poker Tour (1.12 English/Spanish)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112af,"wpt112af.bin",0x1A46D04,CRC(8fe1e3c8) SHA1(837bfc2cf7f4601f99d110428f5de5dd69d2186f))
CORE_CLONEDEF(wpt, 112af, 140a, "World Poker Tour (1.12 English/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112ai,"wpt112ai.bin",0x1A8C8CC,CRC(ac878dfb) SHA1(13db57c77f5d75e87b21d3cfd7aba5dcbcbef59b))
CORE_CLONEDEF(wpt, 112ai, 140a, "World Poker Tour (1.12 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112f,"wpt112f.bin",0x1AA3FF4,CRC(1f7e081c) SHA1(512d44353f619f974d98294c55378f5a1ab2d04b))
CORE_CLONEDEF(wpt, 112f, 140a, "World Poker Tour (1.12 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112g,"wpt112g.bin",0x1A33E00,CRC(2fbac57d) SHA1(fb19e7a4a5384fc8c91e166617dad29a47b2d8b0))
CORE_CLONEDEF(wpt, 112g, 140a, "World Poker Tour (1.12 German)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112gf,"wpt112gf.bin",0x1A74B94,CRC(a6b933b3) SHA1(72a36a427527c3c5cb455a74afbbb43f2bee6480))
CORE_CLONEDEF(wpt, 112gf, 140a, "World Poker Tour (1.12 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112i,"wpt112i.bin",0x1B2C704,CRC(0ba02986) SHA1(db1cbe0611d40c334205d0a8b9f5c6147b259549))
CORE_CLONEDEF(wpt, 112i, 140a, "World Poker Tour (1.12 Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_112l,"wpt112l.bin",0x1C706F0,CRC(203c3a05) SHA1(6173f6a6110e2a226beb566371b2821b0a5b8609))
CORE_CLONEDEF(wpt, 112l, 140a, "World Poker Tour (1.12 Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_111a,"wpt111a.bin",0x19BB19C,CRC(423138a9) SHA1(8df7b9358cacb9399c7886b9905441dc727693a6))
CORE_CLONEDEF(wpt, 111a, 140a, "World Poker Tour (1.11)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111al,"wpt111al.bin",0x1b2ebb0,CRC(fbe2e2cf) SHA1(ed837d6ecc1f312c84a2fd235ade86227c2df843))
CORE_CLONEDEF(wpt, 111al, 140a, "World Poker Tour (1.11 English/Spanish)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111af,"wpt111af.bin",0x1A46CF0,CRC(e3a53741) SHA1(395ffe5e25248504d61bb1c96b914e712e7360c3))
CORE_CLONEDEF(wpt, 111af, 140a, "World Poker Tour (1.11 English/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111ai,"wpt111ai.bin",0x1A8C8B8,CRC(a1e819c5) SHA1(f4e2dc6473f31e7019495d0f37b9b60f2c252f70))
CORE_CLONEDEF(wpt, 111ai, 140a, "World Poker Tour (1.11 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111f,"wpt111f.bin",0x1AA3FE0,CRC(25573be5) SHA1(20a33f387fbf150adda835d2f91ec456077e4c41))
CORE_CLONEDEF(wpt, 111f, 140a, "World Poker Tour (1.11 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111g,"wpt111g.bin",0x1A33DEC,CRC(96782b8e) SHA1(4b89f0d44894f0157397a65a93346e637d71c4f2))
CORE_CLONEDEF(wpt, 111g, 140a, "World Poker Tour (1.11 German)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111gf,"wpt111gf.bin",0x1A74B80,CRC(c1488680) SHA1(fc652273e55d32b0c6e8e12c8ece666edac42962))
CORE_CLONEDEF(wpt, 111gf, 140a, "World Poker Tour (1.11 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111i,"wpt111i.bin",0x1B2C6F0,CRC(4d718e63) SHA1(3ae6cefd6f96a31634f1399d1ce5d2c60955a93c))
CORE_CLONEDEF(wpt, 111i, 140a, "World Poker Tour (1.11 Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_111l,"wpt111l.bin",0x1C706DC,CRC(61f4e257) SHA1(10b11e1340593c9555ff88b0ac971433583fbf13))
CORE_CLONEDEF(wpt, 111l, 140a, "World Poker Tour (1.11 Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_109a,"wpt109a.bin",26980760,CRC(6702e90c) SHA1(5d208894ef293c8a7157ab27eac9a8bca012dc43))
CORE_CLONEDEF(wpt, 109a, 140a, "World Poker Tour (1.09)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_109i,"wpt109i.bin",28493548,CRC(87e5f39f) SHA1(9c79bb0f9ebb5f4f4b9ef959f56812a3fe2fda11))
CORE_CLONEDEF(wpt, 109i, 140a, "World Poker Tour (1.09 Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_109g,"wpt109g.bin",27475432,CRC(0699b279) SHA1(e645361f02865aa5560a4bbae45e085df0c4ae22))
CORE_CLONEDEF(wpt, 109g, 140a, "World Poker Tour (1.09 German)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_109f,"wpt109f.bin",27934684,CRC(44f64903) SHA1(f3bcb8acbc8a6cad6f8573f78de53ce8336e7879))
CORE_CLONEDEF(wpt, 109f, 140a, "World Poker Tour (1.09 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_109f2,"wpt109f2.bin",0x1aa3fdc,CRC(656f3957) SHA1(8c68b00fe528f6467a9c34663bbaa9bc308fc971))
CORE_CLONEDEF(wpt, 109f2,140a, "World Poker Tour (1.09-2 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_109l,"wpt109l.bin",29820632,CRC(a724e6c4) SHA1(161c9e6319a305953ac169cdeceeca304ab582e6))
CORE_CLONEDEF(wpt, 109l, 140a, "World Poker Tour (1.09 Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_108a,"wpt108a.bin",26980760,CRC(bca1f1f7) SHA1(cba81c9645f91f4b0b62ec1eed514069248c19b7))
CORE_CLONEDEF(wpt, 108a, 140a, "World Poker Tour (1.08)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_108i,"wpt108i.bin",28493548,CRC(748362f2) SHA1(174733a2d0f45c46dca8bc6d6bc35d39e36e465d))
CORE_CLONEDEF(wpt, 108i, 140a, "World Poker Tour (1.08 Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_108g,"wpt108g.bin",27475432,CRC(b77ccfae) SHA1(730de2c5e9fa85e25ce799577748c9cf7b83c5e0))
CORE_CLONEDEF(wpt, 108g, 140a, "World Poker Tour (1.08 German)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_108f,"wpt108f.bin",27934684,CRC(b1a8f235) SHA1(ea7b553f2340eb82c34f7e95f4dee6fdd3026f14))
CORE_CLONEDEF(wpt, 108f, 140a, "World Poker Tour (1.08 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_108l,"wpt108l.bin",29820632,CRC(6440224a) SHA1(e1748f0204464d134c5f5083b5c12723186c0422))
CORE_CLONEDEF(wpt, 108l, 140a, "World Poker Tour (1.08 Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_106a,"wpt106a.bin",26980760,CRC(72fd2e58) SHA1(3e910b964d0dc67fd538c027b474b3587b216ce5))
CORE_CLONEDEF(wpt, 106a, 140a, "World Poker Tour (1.06)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_106i,"wpt106i.bin",28493548,CRC(177146f0) SHA1(279380fcc3924a8bb8e3002a66c317473d3fc773))
CORE_CLONEDEF(wpt, 106i, 140a, "World Poker Tour (1.06 Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_106g,"wpt106g.bin",27475432,CRC(9b486bc4) SHA1(c2c3c426201db99303131c5efb4275291ab721d7))
CORE_CLONEDEF(wpt, 106g, 140a, "World Poker Tour (1.06 German)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_106f,"wpt106f.bin",27934684,CRC(efa3eeb9) SHA1(a5260511b6325917a9076bac6c92f1a8472142b8))
CORE_CLONEDEF(wpt, 106f, 140a, "World Poker Tour (1.06 French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(wpt_106l,"wpt106l.bin",29820632,CRC(e38034a1) SHA1(c391887a90f9387f86dc94e22bb7fca57c8e91be))
CORE_CLONEDEF(wpt, 106l, 140a, "World Poker Tour (1.06 Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_105a,"wpt105a.bin",26980760,CRC(51608819) SHA1(a14aa47bdbce1dc958504d866ac963b06cd93bef))
CORE_CLONEDEF(wpt, 105a, 140a, "World Poker Tour (1.05)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(wpt_103a,"wpt103a.bin",26980828,CRC(cd5f80bc) SHA1(4aaab2bf6b744e1a3c3509dc9dd2416ff3320cdb))
CORE_CLONEDEF(wpt, 103a, 140a, "World Poker Tour (1.03)", 2006, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ The Simpsons Kooky Carnival Redemption
/-------------------------------------------------------------------*/
SAM1_INIT(scarn200, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(scarn200,"scarn200.bin",5536248,CRC(f08a2cf0) SHA1(ae32da8b35006061d397832563b71976899625bb))
CORE_GAMEDEFNV(scarn200, "Simpson's Kooky Carnival, The (Redemption 2.0)", 2008, "Stern", sam1, 0)

SAM1_INIT(scarn105, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(scarn105,"scarn105.bin",0x53dd14,CRC(a09ffa33) SHA1(fab75f338a5d6c82632cd0804ddac1ab78466636))
CORE_CLONEDEFNV(scarn105, scarn200, "Simpson's Kooky Carnival, The (Redemption 1.05)", 2006, "Stern", sam1, 0)

SAM1_INIT(scarn103, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(scarn103,"scarn103.bin",0x53A860,CRC(69f5bb8a) SHA1(436db9872d5809c7ed5fe607c4167cdc0e1b5294))
CORE_CLONEDEFNV(scarn103, scarn200, "Simpson's Kooky Carnival, The (Redemption 1.03)", 2006, "Stern", sam1, 0)

SAM1_INIT(scarn9nj, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(scarn9nj,"scarn9nj.bin",0x53B7CC,CRC(3a9142e0) SHA1(57d75763fb52c891d1bb16e85ae170c38e6dd818))
CORE_CLONEDEFNV(scarn9nj, scarn200, "Simpson's Kooky Carnival, The (Redemption 0.90 New Jersey)", 2006, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Pirates of the Caribbean
/-------------------------------------------------------------------*/
SAM1_INIT(potc, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(potc_600as,"poc600as.bin",0x1C92990,CRC(5d5e1aaa) SHA1(9c7a416ae6587a86c8d2c6350621f09580226971))
CORE_GAMEDEF(potc, 600as, "Pirates of the Caribbean (6.0 English/Spanish)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(potc_600af_c, "poc600afc.bin", 28126016, CRC(057fbb03) SHA1(5d11f564543ce28f23f77efec0123d44bfdb350a))
CORE_CLONEDEF(potc, 600af_c, 600as, "Pirates of the Caribbean (6.0 English/French Color)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(potc_600af,"poc600af.bin",0x1AD2B40,CRC(39a51873) SHA1(9597d356a3283c5a4e488a399196a51bf5ed16ca))
CORE_CLONEDEF(potc, 600af, 600as, "Pirates of the Caribbean (6.0 English/French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(potc_600ai,"poc600ai.bin",0x1B24CC8,CRC(2d7aebae) SHA1(9e383507d225859b4df276b21525f500ba98d600))
CORE_CLONEDEF(potc, 600ai, 600as, "Pirates of the Caribbean (6.0 English/Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(potc_600gf,"poc600gf.bin",0x1B67104,CRC(44eb2610) SHA1(ec1e1f7f2cd135942531e0e3f540afadb5d2f527))
CORE_CLONEDEF(potc, 600gf, 600as, "Pirates of the Caribbean (6.0 German/French)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(potc_400al,"poc400al.bin",0x1C88124,CRC(f739474d) SHA1(43bf3fbd23498e2cbeac3d87f5da727e7c05eb86))
CORE_CLONEDEF(potc, 400al, 600as, "Pirates of the Caribbean (4.0 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_400af,"poc400af.bin",0x1AD2B40,CRC(03cfed21) SHA1(947fff6bf3ed69cb346ae9f159e378902901033f))
CORE_CLONEDEF(potc, 400af, 600as, "Pirates of the Caribbean (4.0 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_400ai,"poc400ai.bin",0x1B213A8,CRC(5382440b) SHA1(01d8258b98e256fc54565afd9915fd5079201973))
CORE_CLONEDEF(potc, 400ai, 600as, "Pirates of the Caribbean (4.0 English/Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_400gf,"poc400gf.bin",0x1B67104,CRC(778d02e7) SHA1(6524e56ebf6c5c0effc4cb0521e3a463540ceac4))
CORE_CLONEDEF(potc, 400gf, 600as, "Pirates of the Caribbean (4.0 German/French)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(potc_300al,"poc300al.bin",0x1C88124,CRC(e5e7049d) SHA1(570125f9eb6d7a04ba97890095c15769f0e0dbd6))
CORE_CLONEDEF(potc, 300al, 600as, "Pirates of the Caribbean (3.0 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_300af,"poc300af.bin",0x1AD2B40,CRC(b6fc0c4b) SHA1(5c0d6b46dd6c4f14e03298500558f376ee342de0))
CORE_CLONEDEF(potc, 300af, 600as, "Pirates of the Caribbean (3.0 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_300ai,"poc300ai.bin",0x1B213A8,CRC(2d3eb95e) SHA1(fea9409ffea3554ff0ec1c9ef6642465ec4120e7))
CORE_CLONEDEF(potc, 300ai, 600as, "Pirates of the Caribbean (3.0 English/Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(potc_300gf,"poc300gf.bin",0x1B67104,CRC(52772953) SHA1(e820ca5f347ab637bee07a9d7426058b9fd6557c))
CORE_CLONEDEF(potc, 300gf, 600as, "Pirates of the Caribbean (3.0 German/French)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(potc_115as,"poc115as.bin",0x1C829C8,CRC(9c107d0e) SHA1(5213246ee78c6cc082b9f895b1d1abfa52016ede))
CORE_CLONEDEF(potc, 115as, 600as, "Pirates of the Caribbean (1.15 English/Spanish)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_115gf,"poc115gf.bin",0x1B60478,CRC(09a8454c) SHA1(1af420b314d339231d3b7772ffa44175a01ebd30))
CORE_CLONEDEF(potc, 115gf, 600as, "Pirates of the Caribbean (1.15 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_115ai,"poc115ai.bin",0x1B178FC,CRC(88b66285) SHA1(1d65e4f7a31e51167b91f82d96c3951442b16264))
CORE_CLONEDEF(potc, 115ai, 600as, "Pirates of the Caribbean (1.15 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_115af,"poc115af.bin",0x1AC6564,CRC(008e93b2) SHA1(5a272670cb3e5e59071500124a0086ef86e2b528))
CORE_CLONEDEF(potc, 115af, 600as, "Pirates of the Caribbean (1.15 English/French)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(potc_113as,"poc113as.bin",0x1C829B4,CRC(2c819a02) SHA1(98a79b50e6c80bd58b2571fefc2f5f61030bc25d))
CORE_CLONEDEF(potc, 113as, 600as, "Pirates of the Caribbean (1.13 English/Spanish)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_113ai,"poc113ai.bin",0x1B178E8,CRC(e8b487d1) SHA1(037435b40347a8e1197876fbf7a79e03befa11f4))
CORE_CLONEDEF(potc, 113ai, 600as, "Pirates of the Caribbean (1.13 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_113gf,"poc113gf.bin",0x1B60464,CRC(a508a2f8) SHA1(45e46af267c7caec86e4c92526c4cda85a1bb168))
CORE_CLONEDEF(potc, 113gf, 600as, "Pirates of the Caribbean (1.13 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_113af,"poc113af.bin",0x1C829B4,CRC(1c52b3f5) SHA1(2079f06f1f1514614fa7cb240559b4e72925c70c))
CORE_CLONEDEF(potc, 113af, 600as, "Pirates of the Caribbean (1.13 English/French)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(potc_111as,"poc111as.bin",0x1C829B4,CRC(09903169) SHA1(e284b1dc2642337633867bac9739fdda692acb2f))
CORE_CLONEDEF(potc, 111as, 600as, "Pirates of the Caribbean (1.11 English/Spanish)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(potc_110ai,"poc110ai.bin",0x1B178E8,CRC(027916d9) SHA1(0ddc0fa86da55ea0494f2095c838b41b53f568de))
CORE_CLONEDEF(potc, 110ai, 600as, "Pirates of the Caribbean (1.10 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_110gf,"poc110gf.bin",0x1B60464,CRC(ce29b69c) SHA1(ecc9ad8f77ab30538536631d513d25654f5a2f3c))
CORE_CLONEDEF(potc, 110gf, 600as, "Pirates of the Caribbean (1.10 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_110af,"poc110af.bin",0x1AC6550,CRC(9d87bb49) SHA1(9db04259a0b2733d6f5966a2f3e0fc1c7002cef1))
CORE_CLONEDEF(potc, 110af, 600as, "Pirates of the Caribbean (1.10 English/French)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(potc_109as,"poc109as.bin",0x1C829B4,CRC(eb68b86b) SHA1(416c4bf9b4dc035b8dfed3610a4ac5ae31209ca5))
CORE_CLONEDEF(potc, 109as, 600as, "Pirates of the Caribbean (1.09 English/Spanish)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_109ai,"poc109ai.bin",0x1B178E8,CRC(a8baad2d) SHA1(f2f2a4a16f646a57cc191f8ae2e483e036edb1e7))
CORE_CLONEDEF(potc, 109ai, 600as, "Pirates of the Caribbean (1.09 English/Italian)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_109gf,"poc109gf.bin",0x1B60464,CRC(9866b803) SHA1(c7bef6220cc865614974d02406f109e851a86714))
CORE_CLONEDEF(potc, 109gf, 600as, "Pirates of the Caribbean (1.09 German/French)", 2006, "Stern", sam1, 0)

SAM1_ROM32MB(potc_109af,"poc109af.bin",0x1AC6550,CRC(9d87bb49) SHA1(9db04259a0b2733d6f5966a2f3e0fc1c7002cef1))
CORE_CLONEDEF(potc, 109af, 600as, "Pirates of the Caribbean (1.09 English/French)", 2006, "Stern", sam1, 0)


SAM1_ROM32MB(potc_108as,"poc108as.bin",0x1C61F6C,CRC(6c3a3f7f) SHA1(52e97a4f479f8f3f55a72c9c104fb1335a253f1a))
CORE_CLONEDEF(potc, 108as, 600as, "Pirates of the Caribbean (1.08 English/Spanish)", 2006, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Family Guy
/-------------------------------------------------------------------*/
SAM1_INIT(fg, sam1_dmd128x32, 8, SAM_FG)
SAM1_ROM32MB(fg_1200al,"fg1200al.bin",0x1E5F448,CRC(d10cff88) SHA1(e312a3b24f1b69db9f88a5313db168d9f2a71450))
CORE_GAMEDEF(fg, 1200al, "Family Guy (12.0 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1200af,"fg1200af.bin",0x1CE5514,CRC(ba6a3a2e) SHA1(78eb2e26abe00d7ce5fa998b6ec1381ac0f1db31))
CORE_CLONEDEF(fg, 1200af, 1200al, "Family Guy (12.0 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1200ag,"fg1200ag.bin",0x1C53678,CRC(d9734f94) SHA1(d56ddf5961e5ac4c3565f9d92d6fb7e0e0af4bcb))
CORE_CLONEDEF(fg, 1200ag, 1200al, "Family Guy (12.0 English/German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1200ai,"fg1200ai.bin",0x1D9F8B8,CRC(078b0c9a) SHA1(f1472d2c4a06d674bf652dd481cce5d6ca125e0c))
CORE_CLONEDEF(fg, 1200ai, 1200al, "Family Guy (12.0 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_1100al,"fg1100al.bin",0x1E5F448,CRC(d9b724a8) SHA1(33ac12fd4bbed11e38ade68426547ed97612cbd3))
CORE_CLONEDEF(fg, 1100al, 1200al, "Family Guy (11.0 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1100af,"fg1100af.bin",0x1CE5514,CRC(31304627) SHA1(f36d6924f1f291f675f162ff056b6ea2f03f4351))
CORE_CLONEDEF(fg, 1100af, 1200al, "Family Guy (11.0 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1100ag,"fg1100ag.bin",0x1C53678,CRC(d2735578) SHA1(a38b8f690ffcdb96875d3c8293e6602d7142be11))
CORE_CLONEDEF(fg, 1100ag, 1200al, "Family Guy (11.0 English/German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1100ai,"fg1100ai.bin",0x1D9F8B8,CRC(4fa2c59e) SHA1(7fce5c1fd306eccc567ae7d155c782649c022074))
CORE_CLONEDEF(fg, 1100ai, 1200al, "Family Guy (11.0 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_1000ag,"fg1000ag.bin",0x1c53678,CRC(130e0bd6) SHA1(ced815270d419704d94d5acdc5335460a64484ae))
CORE_CLONEDEF(fg, 1000ag, 1200al, "Family Guy (10.00 English/German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1000af,"fg1000af.bin",0x1CE5514,CRC(27cabf5d) SHA1(dde359c1fed728c8f91901f5ce351b5adef399f3))
CORE_CLONEDEF(fg, 1000af, 1200al, "Family Guy (10.00 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1000ai,"fg1000ai.bin",0x1D9F8B8,CRC(2137e62a) SHA1(ac892d2536c5dde97194ffb69c74d0517000357a))
CORE_CLONEDEF(fg, 1000ai, 1200al, "Family Guy (10.00 English/Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_1000al,"fg1000al.bin",0x1E5F448,CRC(0f570f24) SHA1(8861bf3e6add7a5372d81199c135808d09b5e600))
CORE_CLONEDEF(fg, 1000al, 1200al, "Family Guy (10.00 English/Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_800al,"fg800al.bin",0x1bc6cb4,CRC(b74dc3bc) SHA1(b24bab06b9f451cf9f068c555d3f70ffdbf40da7))
CORE_CLONEDEF(fg, 800al, 1200al, "Family Guy (8.00 English/Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_700af,"fg700af.bin",0x1A4D3D4,CRC(bbeda480) SHA1(792c396dee1b5abe113484e1fd4c4b449d8e7d95))
CORE_CLONEDEF(fg, 700af, 1200al, "Family Guy (7.00 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_700al,"fg700al.bin",0x1BCE8F8,CRC(25288f43) SHA1(5a2ed2e0b264895938466ca1104ba4ed9be86b3a))
CORE_CLONEDEF(fg, 700al, 1200al, "Family Guy (7.00 English/Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_400a,"fg400a.bin",0x13E789C,CRC(af6c2dd4) SHA1(e3164e982c90a5300144e63e4a74dd225fe1b272))
CORE_CLONEDEF(fg, 400a, 1200al, "Family Guy (4.00)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(fg_400ag,"fg400ag.bin",0x1971684,CRC(3b4ae199) SHA1(4ef674badce2c90334fa7a8b6b90c32dcabc2334))
CORE_CLONEDEF(fg, 400ag, 1200al, "Family Guy (4.00 English/German)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_300ai,"fg300ai.bin",0x1fc0290,CRC(e2cffa79) SHA1(59dff445118ed8a3a76b6e93950802d1fec87619))
CORE_CLONEDEF(fg, 300ai, 1200al, "Family Guy (3.00 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(fg_200a,"fg200a.bin",0x185ea9c,CRC(c72e89df) SHA1(6cac3812d733c9d030542badb9c65934ecbf8399))
CORE_CLONEDEF(fg, 200a, 1200al, "Family Guy (2.00)", 2007, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Spider-Man (Stern)
/-------------------------------------------------------------------*/
SAM1_INIT(sman, sam1_dmd128x32, 2, 0)

SAM1_ROM32MB(sman_261,"smn261.bin",24219492,CRC(9900cd4c) SHA1(1b95f957f8d709bba9fb3b7dcd4bca99176a010c))
CORE_GAMEDEF(sman, 261, "Spider-Man (2.61)", 2014, "Stern", sam1, 0)

SAM1_ROM128MB(sman_100ve,"smn100ve.bin",66243212,CRC(f761fa19) SHA1(259bd6d42e742eaad1b7b50f9b5e4830c81084b0))
CORE_CLONEDEF(sman, 100ve, 261, "Spider-Man (1.0 Vault Edition)", 2016, "Stern", sam2, 0)

SAM1_ROM128MB(sman_101ve, "smn101ve.bin", 66243212, CRC(b7a525e8) SHA1(43fd9520225b11ba8ba5f9e8055689a652237983))
CORE_CLONEDEF(sman, 101ve, 261, "Spider-Man (1.01 Vault Edition)", 2016, "Stern", sam2, 0)

SAM1_ROM32MB(sman_260,"smn260a.bin",0x18775E0,CRC(acfc813e) SHA1(bcbb0ec2bbfc55b1256c83b0300c0c38d15a3db1))
CORE_CLONEDEF(sman, 260, 261, "Spider-Man (2.6)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_250,"smn250a.bin",0x18775B8,CRC(78d61e14) SHA1(3241d62e12d716ed661fbd0949cf4a39feb64437))
CORE_CLONEDEF(sman, 250, 261, "Spider-Man (2.5)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_240,"smn240a.bin",0x18775B8,CRC(dc5ee57e) SHA1(7453db81b161cdbf7be690da15ea8a78e4a4e57d))
CORE_CLONEDEF(sman, 240, 261, "Spider-Man (2.4)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_230,"smn230a.bin",0x18775B8,CRC(a86f1768) SHA1(72662dcf05717d3b2b335077ceddabe562738468))
CORE_CLONEDEF(sman, 230, 261, "Spider-Man (2.3)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_220,"smn220a.bin",0x18775B8,CRC(44f31e8e) SHA1(4c07d01c95c5fab1955b11e4f7c65f369a91dfd7))
CORE_CLONEDEF(sman, 220, 261, "Spider-Man (2.2)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(sman_210,"smn210a.bin",0x168E8A8,CRC(f983df18) SHA1(a0d46e1a58f016102773861a4f1b026755f776c8))
CORE_CLONEDEF(sman, 210, 261, "Spider-Man (2.1)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_210af,"smn210af.bin",0x192B160,CRC(2e86ac24) SHA1(aa223db6289a876e77080e16f29cbfc62183fa67))
CORE_CLONEDEF(sman, 210af, 261, "Spider-Man (2.1 English/French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_210ai,"smn210ai.bin",0x193CC7C,CRC(aadd1ea7) SHA1(a41b0067f7490c6df5d85e80b208c9993f806366))
CORE_CLONEDEF(sman, 210ai, 261, "Spider-Man (2.1 English/Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_210al,"smn210al.bin",0x19A5D5C,CRC(8c441caa) SHA1(e40ac748284f65de5c444ac89d3b02dd987facd0))
CORE_CLONEDEF(sman, 210al, 261, "Spider-Man (2.1 English/Spanish)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_210gf,"smn210gf.bin",0x1941C04,CRC(2995cb97) SHA1(0093d3f20aebbf6129854757cc10aff63fc18a4a))
CORE_CLONEDEF(sman, 210gf, 261, "Spider-Man (2.1 German/French)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(sman_200,"smn200a.bin",0x168E8A8,CRC(3b13348c) SHA1(4b5c6445d7805c0a39054bd51522751030b73162))
CORE_CLONEDEF(sman, 200, 261, "Spider-Man (2.0)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(sman_192,"smn192a.bin",0x1920870,CRC(a44054fa) SHA1(a0910693d13cc61dba7a2bbe9185a24b33ef20ec))
CORE_CLONEDEF(sman, 192, 261, "Spider-Man (1.92)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_192af,"smn192af.bin",0x1B81624,CRC(c9f8a7dd) SHA1(e63e98965d08b8a645c92fb34ce7fc6e1ad05ddc))
CORE_CLONEDEF(sman, 192af, 261, "Spider-Man (1.92 English/French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_192ai,"smn192ai.bin",0x1B99F88,CRC(f02acad4) SHA1(da103d5ddbcbdcc19cca6c17b557dcc71942970a))
CORE_CLONEDEF(sman, 192ai, 261, "Spider-Man (1.92 English/Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_192al,"smn192al.bin",0x1BF19A0,CRC(501f9986) SHA1(d93f973f9eddfd85903544f0ce49c1bf17b36eb9))
CORE_CLONEDEF(sman, 192al, 261, "Spider-Man (1.92 English/Spanish)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_192gf,"smn192gf.bin",0x1B9A1B4,CRC(32597e1d) SHA1(47a28cdba11b32661dbae95e3be1a41fc475fa5e))
CORE_CLONEDEF(sman, 192gf, 261, "Spider-Man (1.92 German/French)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(sman_190,"smn190a.bin",0x1652310,CRC(7822a6d1) SHA1(6a21dfc44e8fa5e138fe6474c467ef6d6544d78c))
CORE_CLONEDEF(sman, 190, 261, "Spider-Man (1.9)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_190af,"smn190af.bin",0x18B5C34,CRC(dac27fde) SHA1(93a236afc4be6514a8fc57e45eb5698bd999eef6))
CORE_CLONEDEF(sman, 190af, 261, "Spider-Man (1.9 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_190ai,"smn190ai.bin",0x18CD02C,CRC(95c769ac) SHA1(e713677fea9e28b2438a30bf5d81448d3ca140e4))
CORE_CLONEDEF(sman, 190ai, 261, "Spider-Man (1.9 English/Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_190al,"smn190al.bin",0x1925DD0,CRC(4df8168c) SHA1(8ebfda5378037c231075017713515a3681a0e38c))
CORE_CLONEDEF(sman, 190al, 261, "Spider-Man (1.9 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_190gf,"smn190gf.bin",0x18CD02C,CRC(a4a874a4) SHA1(1e46720462f1279c417d955c500e829e878ce31f))
CORE_CLONEDEF(sman, 190gf, 261, "Spider-Man (1.9 German/French)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_170,"smn170a.bin",0x1877484,CRC(45c9e5f5) SHA1(8af3215ecc247186c83e235c60c3a2990364baad))
CORE_CLONEDEF(sman, 170, 261, "Spider-Man (1.7)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_170al,"smn170al.bin",0x1D24E70,CRC(0455f3a9) SHA1(134ff31605798989b396220f8580d1c079678084))
CORE_CLONEDEF(sman, 170al, 261, "Spider-Man (1.70 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_170af,"smn170af.bin",0x1C6F32C,CRC(b38f3948) SHA1(8daae4bc8b1eaca2bd43198365474f5da09b4788))
CORE_CLONEDEF(sman, 170af, 261, "Spider-Man (1.7 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_170gf,"smn170gf.bin",0x1C99C74,CRC(152aa803) SHA1(e18f9dcc5380126262cf1e32e99b6cc2c4aa23cb))
CORE_CLONEDEF(sman, 170gf, 261, "Spider-Man (1.7 German/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_170ai,"smn170ai.bin",0x1C90F74,CRC(ba176624) SHA1(56c847995b5a3e2286e231c1d69f82cf5492cd5d))
CORE_CLONEDEF(sman, 170ai, 261, "Spider-Man (1.7 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_160,"smn160a.bin",0x1725778,CRC(05425962) SHA1(a37f61239a7116e5c14a345c288f781fa6248cf8))
CORE_CLONEDEF(sman, 160, 261, "Spider-Man (1.6)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_160al,"smn160al.bin",0x1BB15BC,CRC(776937d9) SHA1(631cadd665f895feac90c3cbc14eb8e321d19b4e))
CORE_CLONEDEF(sman, 160al, 261, "Spider-Man (1.6 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_160af,"smn160af.bin",0x1B0121C,CRC(d0b552e9) SHA1(2550baba3c4be5308779d502a2d2d01e1c2539ef))
CORE_CLONEDEF(sman, 160af, 261, "Spider-Man (1.6 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_160gf,"smn160gf.bin",0x1B24430,CRC(1498f877) SHA1(e625a7e683035665a0a1a97e5de0947628c3f7ea))
CORE_CLONEDEF(sman, 160gf, 261, "Spider-Man (1.6 German/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_160ai,"smn160ai.bin",0x1B26D28,CRC(b776f59b) SHA1(62600474b8a5e1e2d40319817505c8b5fd3df2fa))
CORE_CLONEDEF(sman, 160ai, 261, "Spider-Man (1.6 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_142,"smn142a.bin",0x16E8D60,CRC(307b0163) SHA1(015c8c86763c645b43bd71a3cdb8975fcd36a99f))
CORE_CLONEDEF(sman, 142, 261, "Spider-Man (1.42)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_140,"smn140a.bin",0x16CE3C0,CRC(48c2565d) SHA1(78f5d3242cfaa85fa0fd3937b6042f067dff535b))
CORE_CLONEDEF(sman, 140, 261, "Spider-Man (1.4)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_140al,"smn140al.bin",0x1ADC768,CRC(fd372e14) SHA1(70f3e4d210a4da4b6122089c477b5b3f51d3593f))
CORE_CLONEDEF(sman, 140al, 261, "Spider-Man (1.4 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_140af,"smn140af.bin",0x1A50398,CRC(d181fa71) SHA1(66af219d9266b6b24e6857ad1a6b4fe539058052))
CORE_CLONEDEF(sman, 140af, 261, "Spider-Man (1.4 English/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_140gf,"smn140gf.bin",0x1A70F78,CRC(f1124c86) SHA1(755f15dd566f86695c7143512d81e16af71c8853))
CORE_CLONEDEF(sman, 140gf, 261, "Spider-Man (1.4 German/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_140ai,"smn140ai.bin",0x1A70F78,CRC(0de6937e) SHA1(f2e60b545ef278e1b7981bf0a3dc2c622205e8e1))
CORE_CLONEDEF(sman, 140ai, 261, "Spider-Man (1.4 English/Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_132,"smn132a.bin",0x1588E0C,CRC(c8cd8f0a) SHA1(c2e1b54de54e8bd480300054c98a4f09d723edb7))
CORE_CLONEDEF(sman, 132, 261, "Spider-Man (1.32)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_130al,"smn130al.bin",0x180aaa0,CRC(33004e72) SHA1(3bc30200945d896aefbff51c7b427595885a23c4))
CORE_CLONEDEF(sman, 130al, 261, "Spider-Man (1.30 English/Spanish)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_130ai,"smn130ai.bin",0x17B7960,CRC(92aab158) SHA1(51662102da54e7e7c0f63689fffbf70653ee8f11))
CORE_CLONEDEF(sman, 130ai, 261, "Spider-Man (1.30 English/Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_130gf,"smn130gf.bin",0x17AEC84,CRC(2838d2f3) SHA1(2192f1fbc393c5e0dcd59198d098bb2531d8b6de))
CORE_CLONEDEF(sman, 130gf, 261, "Spider-Man (1.30 German/French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(sman_130af,"smn130af.bin",0x17916C8,CRC(6aa6a03a) SHA1(f56442e84b8789f49127bf4ba97dd05c77ea7c36))
CORE_CLONEDEF(sman, 130af, 261, "Spider-Man (1.30 English/French)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(sman_261x,"smn261a.bin",0x18775E0,CRC(9ed2687d) SHA1(ceab8dd24b41ddc1e07f938adfd1b868027f7fb5))
CORE_CLONEDEF(sman, 261x, 261, "Spider-Man (2.61 hacked)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(sman_262x,"smn262a.bin",0x18775E0,CRC(9ad85331) SHA1(64a4b4087aee06b79d959e2bc9490ba269cd47a6))
CORE_CLONEDEF(sman, 262x, 261, "Spider-Man (2.62 hacked replaced music)", 2008, "Stern", sam1, 0)

//!! this has same checksum as 100ve, maybe because its WIP??
SAM1_ROM128MB(sman_100ve_c, "smn100ve_c.bin", 0x03FFFFF0, CRC(f761fa19) SHA1(259bd6d42e742eaad1b7b50f9b5e4830c81084b0))
CORE_CLONEDEF(sman, 100ve_c, 261, "Spider-Man (1.0 Vault Edition Color)", 2016, "Stern", sam2, 0)

/*-------------------------------------------------------------------
/ Wheel of Fortune
/-------------------------------------------------------------------*/
SAM1_INIT(wof, sam1_wofDisp, 8, SAM_WOF)
SAM1_ROM32MB(wof_500,"wof500a.bin",0x1C7DFD0,CRC(6613e864) SHA1(b6e6dcfa782720e7d0ce36f8ea33a0d05763d6bd))
CORE_GAMEDEF(wof, 500, "Wheel of Fortune (5.0)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_500f,"wof500f.bin",0x1E76BA4,CRC(3aef1035) SHA1(4fa0a40fea403beef0b3ce695ff52dec3d90f7bf))
CORE_CLONEDEF(wof, 500f, 500, "Wheel of Fortune (5.0 French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_500g,"wof500g.bin",0x1CDEC2C,CRC(658f8622) SHA1(31926717b5914f91b70eeba182eb219a4fd51299))
CORE_CLONEDEF(wof, 500g, 500, "Wheel of Fortune (5.0 German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_500i,"wof500i.bin",0x1D45EE8,CRC(27fb48bc) SHA1(9a9846c84a1fc543ec2236a28991d0cd70e86b52))
CORE_CLONEDEF(wof, 500i, 500, "Wheel of Fortune (5.0 Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_500l,"wof500l.bin",0x1B080B0,CRC(b8e09fcd) SHA1(522983ce75b24733a0827a2eeea3d44419c7998e))
CORE_CLONEDEF(wof, 500l, 500, "Wheel of Fortune (5.0 Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(wof_401l,"wof401l.bin",0x1B080B0,CRC(4db936f4) SHA1(4af1d4642529164cb5bc0b9adbc229b131098007))
CORE_CLONEDEF(wof, 401l, 500, "Wheel of Fortune (4.01 Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(wof_400,"wof400a.bin",0x1C7DFD0,CRC(974e6dd0) SHA1(ce4d7537e8f42ab6c3e84eac19688e2155115345))
CORE_CLONEDEF(wof, 400, 500, "Wheel of Fortune (4.0)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_400f,"wof400f.bin",0x1E76BA4,CRC(91a793c0) SHA1(6c390ab435dc20889bccfdd11bbfc411efd1e4f9))
CORE_CLONEDEF(wof, 400f, 500, "Wheel of Fortune (4.0 French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_400g,"wof400g.bin",0x1CDEC2C,CRC(ee97a6f3) SHA1(17a3093f7e5d052c23b669ee8717a21a80b61813))
CORE_CLONEDEF(wof, 400g, 500, "Wheel of Fortune (4.0 German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_400i,"wof400i.bin",0x1D45EE8,CRC(35053d2e) SHA1(3b8d176c7b34e7eaf20f9dcf27649841c5122609))
CORE_CLONEDEF(wof, 400i, 500, "Wheel of Fortune (4.0 Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(wof_300,"wof300a.bin",0x1C7DFD0,CRC(7a8483b8) SHA1(e361eea5a01d6ba22782d34538edd05f3b068472))
CORE_CLONEDEF(wof, 300, 500, "Wheel of Fortune (3.0)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_300f,"wof300f.bin",0x1E76BA4,CRC(fd5c2bec) SHA1(77f6e4177df8a17f43198843f8a0a3cf5caf1704))
CORE_CLONEDEF(wof, 300f, 500, "Wheel of Fortune (3.0 French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_300g,"wof300g.bin",0x1CDEC2C,CRC(54b50069) SHA1(909b98a7f5fdfa0164c7dc52e9c830eecada2a64))
CORE_CLONEDEF(wof, 300g, 500, "Wheel of Fortune (3.0 German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_300i,"wof300i.bin",0x1D45EE8,CRC(7528800b) SHA1(d55024935861aa8895f9604e92f0d74cb2f3827d))
CORE_CLONEDEF(wof, 300i, 500, "Wheel of Fortune (3.0 Italian)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_300l,"wof300l.bin",0x1B080B0,CRC(12e1b3a5) SHA1(6b62e40e7b124477dc8508e39722c3444d4b39a4))
CORE_CLONEDEF(wof, 300l, 500, "Wheel of Fortune (3.0 Spanish)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(wof_200,"wof200a.bin",0x1C7DFD0,CRC(2e56b65f) SHA1(908662261548f4b80433d58359e9ff1013bf315b))
CORE_CLONEDEF(wof, 200, 500, "Wheel of Fortune (2.0)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_200f,"wof200f.bin",0x1E76BA4,CRC(d48d4885) SHA1(25cabea55f30d86b8d6398f94e1d180377c34de6))
CORE_CLONEDEF(wof, 200f, 500, "Wheel of Fortune (2.0 French)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_200g,"wof200g.bin",0x1CDEC2C,CRC(81f61e6c) SHA1(395be7e0ccb9a806738fc6338b8e6dbea561986d))
CORE_CLONEDEF(wof, 200g, 500, "Wheel of Fortune (2.0 German)", 2007, "Stern", sam1, 0)

SAM1_ROM32MB(wof_200i,"wof200i.bin",0x1D45EE8,CRC(3e48eef7) SHA1(806a0313852405cd9913406201dd9e434b9b160a))
CORE_CLONEDEF(wof, 200i, 500, "Wheel of Fortune (2.0 Italian)", 2007, "Stern", sam1, 0)


SAM1_ROM32MB(wof_100,"wof100a.bin",0x1C7DF60,CRC(f3b80429) SHA1(ab1c9752ea74b5950b51aabc6dbca4f405705240))
CORE_CLONEDEF(wof, 100, 500, "Wheel of Fortune (1.0)", 2007, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Shrek
/-------------------------------------------------------------------*/
SAM1_INIT(shr, sam1_dmd128x32, 8, SAM_FG)
SAM1_ROM32MB(shr_141,"shr_141.bin",0x1C55290,CRC(f4f847ce) SHA1(d28f9186bb04036e9ff56d540e70a50f0816051b))
CORE_GAMEDEF(shr, 141, "Shrek (1.41)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(shr_141_c, "shr_141_c.bin", 29708944, CRC(362f3a3b) SHA1(3de2cc7ddafaf818216a56d33314e08241d61bf0))
CORE_CLONEDEF(shr, 141_c, 141, "Shrek (1.41 Color)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(shr_130,"shr_130.bin",0x1BB0824,CRC(0c4efde5) SHA1(58e156a43fef983d48f6676e8d65fb30d45f8ec3))
CORE_CLONEDEF(shr, 130, 141, "Shrek (1.3)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Indiana Jones (Stern)
/-------------------------------------------------------------------*/
SAM1_INIT(ij4, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(ij4_210,"ij4_210.bin",0x1C6D9E4,CRC(b96e6fd2) SHA1(f59cbdefc5ab6b21662981b3eb681fd8bd7ade54))
CORE_GAMEDEF(ij4, 210, "Indiana Jones (2.10)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_210f,"ij4_210f.bin",0x1C6D9E4,CRC(d1d37248) SHA1(fd6819e0e86b83d658790ff30871596542f98c8e))
CORE_CLONEDEF(ij4, 210f, 210, "Indiana Jones (2.10 French)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(ij4_116,"ij4_116.bin",0x1C6D9E4,CRC(80293485) SHA1(043c857a8dfa79cb7ae876c55a10227bdff8e873))
CORE_CLONEDEF(ij4, 116, 210, "Indiana Jones (1.16)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_116f,"ij4_116f.bin",0x1C6D9E4,CRC(56821942) SHA1(484f4359b6d1ecb45c29bef7532a8136028504f4))
CORE_CLONEDEF(ij4, 116f, 210, "Indiana Jones (1.16 French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_116g,"ij4_116g.bin",0x1C34974,CRC(2b7b81be) SHA1(a70ed07daec7f13165a0256bc011a72136e25210))
CORE_CLONEDEF(ij4, 116g, 210, "Indiana Jones (1.16 German)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_116i,"ij4_116i.bin",0x1C96B38,CRC(7b07c207) SHA1(67969e85cf96949f8b85d88acfb69be55f32ea52))
CORE_CLONEDEF(ij4, 116i, 210, "Indiana Jones (1.16 Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_116l,"ij4_116l.bin",0x1D14FD8,CRC(833ae2fa) SHA1(cb931e473164ddfa2559f3a58f2fcac5d456dc96))
CORE_CLONEDEF(ij4, 116l, 210, "Indiana Jones (1.16 Spanish)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(ij4_114,"ij4_114.bin",0x1C6D9E4,CRC(00e5b850) SHA1(3ad57120d11aff4ca8917dea28c2c26ae254e2b5))
CORE_CLONEDEF(ij4, 114, 210, "Indiana Jones (1.14)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_114f,"ij4_114f.bin",0x1C6D9E4,CRC(a7c2a5e4) SHA1(c0463b055096a3112a31680dc509f421c1a5c1cf))
CORE_CLONEDEF(ij4, 114f, 210, "Indiana Jones (1.14 French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_114g,"ij4_114g.bin",0x1C34974,CRC(7176b0be) SHA1(505132887bca0fa9d6ca8597101357f26501a0ad))
CORE_CLONEDEF(ij4, 114g, 210, "Indiana Jones (1.14 German)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_114i,"ij4_114i.bin",0x1C875F8,CRC(dac0563e) SHA1(30dbaed1b1a180f7ca68a4caef469c2997bf0355))
CORE_CLONEDEF(ij4, 114i, 210, "Indiana Jones (1.14 Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_114l,"ij4_114l.bin",0x1D0B290,CRC(e9b3a81a) SHA1(574377e7a398083f3498d91640ad7dc5250acbd7))
CORE_CLONEDEF(ij4, 114l, 210, "Indiana Jones (1.14 Spanish)", 2008, "Stern", sam1, 0)


SAM1_ROM32MB(ij4_113,"ij4_113.bin",0x1C6D98C,CRC(aa2bdf3e) SHA1(71fd1c970fe589cec5124237684facaae92cbf09))
CORE_CLONEDEF(ij4, 113, 210, "Indiana Jones (1.13)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_113f,"ij4_113f.bin",0x1C6D98C,CRC(cb7b7c31) SHA1(3a2f718a9a533941c5476f8348dacf7e3523ddd0))
CORE_CLONEDEF(ij4, 113f, 210, "Indiana Jones (1.13 French)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_113g,"ij4_113g.bin",0x1BFF3F4,CRC(30a33bfd) SHA1(c37b6035c313cce85d325ab87039f5a872d28f5a))
CORE_CLONEDEF(ij4, 113g, 210, "Indiana Jones (1.13 German)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_113i,"ij4_113i.bin",0x1C81FA4,CRC(fcb37e0f) SHA1(7b23a56baa9985e2322aee954befa13dc2d55119))
CORE_CLONEDEF(ij4, 113i, 210, "Indiana Jones (1.13 Italian)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(ij4_113l,"ij4_113l.bin",0x1D02988,CRC(e4ff8120) SHA1(f5537cf920633a621b4c7a740bfc07cefe3a99d0))
CORE_CLONEDEF(ij4, 113l, 210, "Indiana Jones (1.13 Spanish)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Batman The Dark Knight
/-------------------------------------------------------------------*/
SAM1_INIT(bdk, sam1_dmd128x32, 3, SAM_BDK)
SAM1_ROM32MB(bdk_294,"bdk_294.bin",0x1C00844,CRC(e087ec82) SHA1(aad2c43e6de9a520954eb50b6c824a138cd6f47f))
CORE_GAMEDEF(bdk, 294, "Batman The Dark Knight (2.94)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_300,"bdk_300.bin",0x1C6AD84,CRC(8325bc80) SHA1(04f20d78ad33956618e576bba108ab145e26f9aa))
CORE_CLONEDEF(bdk, 300, 294, "Batman The Dark Knight (3.00 Home Edition/Costco)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_294e, "bdk_294e.bin", 29362244, CRC(1341670c) SHA1(45fb75a128324310a383005905078b0c97167311))
CORE_CLONEDEF(bdk, 294e, 294, "Batman The Dark Knight (2.94e hacked)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_290, "bdk_290.bin", 0x1D3D2D4, CRC(09ce777e) SHA1(79b6d3f91aa4d42318c698a44444bf875ad573f2))
CORE_CLONEDEF(bdk, 290, 294, "Batman The Dark Knight (2.90)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_240,"bdk_240.bin",0x1B96D94,CRC(6cf8c983) SHA1(fd1396e1075fd938f8a95c27c96a0164137b62dc))
CORE_CLONEDEF(bdk, 240, 294, "Batman The Dark Knight (2.40)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_220,"bdk_220.bin",0x1B96D94,CRC(6e415ce7) SHA1(30a3938817da20ccb87c7e878cdd8a13ada097ab))
CORE_CLONEDEF(bdk, 220, 294, "Batman The Dark Knight (2.20)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_210,"bdk_210.bin",0x1B96D94,CRC(ac84fef1) SHA1(bde3250f3d95a12a5f3b74ac9d11ba0bd331e9cd))
CORE_CLONEDEF(bdk, 210, 294, "Batman The Dark Knight (2.10)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_200,"bdk_200.bin",0x1B04378,CRC(07b716a9) SHA1(4cde06308bb967435c7c1bf078a2cda36088e3ec))
CORE_CLONEDEF(bdk, 200, 294, "Batman The Dark Knight (2.00)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_160,"bdk_160.bin",0x1B02F70,CRC(5554ea47) SHA1(0ece4779ad9a3d6c8428306774e2bf36a20d680d))
CORE_CLONEDEF(bdk, 160, 294, "Batman The Dark Knight (1.60)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_150,"bdk_150.bin",0x18EE5E8,CRC(ed11b88c) SHA1(534224de597cbd3632b902397d945ab725e24912))
CORE_CLONEDEF(bdk, 150, 294, "Batman The Dark Knight (1.50)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(bdk_130,"bdk_130.bin",0x1BA1E94,CRC(83a32958) SHA1(0326891bc142c8b92bd4f6d29bd4301bacbed0e7))
CORE_CLONEDEF(bdk, 130, 294, "Batman The Dark Knight (1.30)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ C.S.I. Crime Scene Investigation
/-------------------------------------------------------------------*/
SAM1_INIT(csi, sam1_dmd128x32, 3, SAM_CSI)
SAM1_ROM32MB(csi_240,"csi240a.bin",0x1E21FC0,CRC(2be97fa3) SHA1(5aa231bde81f7787cc06567c8b3d28c750588071))
CORE_GAMEDEF(csi, 240, "C.S.I. (2.4)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(csi_230,"csi230a.bin",0x1E21FC0,CRC(c25ccc67) SHA1(51a21fca06db4b05bda2c7d5a09d655c97ba19c6))
CORE_CLONEDEF(csi, 230, 240, "C.S.I. (2.3)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(csi_210,"csi210a.bin",0x1E21FC0,CRC(afebb31f) SHA1(9b8179baa2f6e61852b57aaad9a28def0c014861))
CORE_CLONEDEF(csi, 210, 240, "C.S.I. (2.1)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(csi_200,"csi200a.bin",0x1E21FC0,CRC(ecb25112) SHA1(385bede7955e06c1e1b7cd06e988a64b0e6ea54f))
CORE_CLONEDEF(csi, 200, 240, "C.S.I. (2.0)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(csi_104,"csi104a.bin",0x1E21FC0,CRC(15694586) SHA1(3a6b70d43f9922d7a459e1dc4c235bcf03e7858e))
CORE_CLONEDEF(csi, 104, 240, "C.S.I. (1.04)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(csi_103,"csi103a.bin",0x1E61C88,CRC(371bc874) SHA1(547588b85b4d6e79123178db3f3e51354e8d2229))
CORE_CLONEDEF(csi, 103, 240, "C.S.I. (1.03)", 2008, "Stern", sam1, 0)

SAM1_ROM32MB(csi_102,"csi102a.bin",0x1E21FC0,CRC(770f4ab6) SHA1(7670022926fcf5bb8f8848374cf1a6237803100a))
CORE_CLONEDEF(csi, 102, 240, "C.S.I. (1.02)", 2008, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ 24 Twenty-Four
/-------------------------------------------------------------------*/
SAM1_INIT(twenty4, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(twenty4_150,"24_150a.bin",0x1CA8E50,CRC(9d7d87cc) SHA1(df6b2f60b87226fdda33bdbbe03ea87d690fc563))
CORE_GAMEDEF(twenty4, 150, "24 (1.50)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(twenty4_144,"24_144a.bin",0x1CA8E50,CRC(29c47da0) SHA1(8d38e35a0df843a71cac6cd4dd6aa460347a208c))
CORE_CLONEDEF(twenty4, 144, 150, "24 (1.44)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(twenty4_140,"24_140a.bin",0x1c08280,CRC(bab92fb1) SHA1(07c8d9c28730411dd0f23d5960a223beb4c587b2))
CORE_CLONEDEF(twenty4, 140, 150, "24 (1.4)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(twenty4_130,"24_130a.bin",0x1c08280,CRC(955a5c12) SHA1(66e33fb438c831679aeb3ba68af7b4a3c59966ef))
CORE_CLONEDEF(twenty4, 130, 150, "24 (1.3)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ NBA
/-------------------------------------------------------------------*/
SAM1_INIT(nba, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(nba_802,"nba802a.bin",0x19112D0,CRC(ba681dac) SHA1(184f3315a54b1a5295b19222c718ac38fa60d340))
CORE_GAMEDEF(nba, 802, "NBA (8.02)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(nba_801,"nba801a.bin",0x19112D0,CRC(0f8b146e) SHA1(090d73a9bff0a0b0c17ced1557d5e63e5c986e95))
CORE_CLONEDEF(nba, 801, 802, "NBA (8.01)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(nba_700,"nba700a.bin",0x19112D0,CRC(15ece43b) SHA1(90cc8b4c52a61da9701fcaba0a21144fe576eaf4))
CORE_CLONEDEF(nba, 700, 802, "NBA (7.0)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(nba_600,"nba600a.bin",0x19112D0,CRC(af2fbcf4) SHA1(47df1992a1eb6c4cd5ec246912eab9f5636499a7))
CORE_CLONEDEF(nba, 600, 802, "NBA (6.0)", 2009, "Stern", sam1, 0)

SAM1_ROM32MB(nba_500,"nba500a.bin",0x19112D0,CRC(01b0c27a) SHA1(d7f4f6b24630b55559a48cde4475422905811106))
CORE_CLONEDEF(nba, 500, 802, "NBA (5.0)", 2009, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Big Buck Hunter
/-------------------------------------------------------------------*/
SAM1_INIT(bbh, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(bbh_170,"bbh170a.bin",0x1BB8FD0,CRC(0c2d3e64) SHA1(9a71959c57b9a75028e21bce9ee03871f8914138))
CORE_GAMEDEF(bbh, 170, "Big Buck Hunter Pro (1.7)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bbh_160,"bbh160a.bin",0x1BB8FA4,CRC(75077f85) SHA1(c58a2ae5c1332390f0d1191ee8ff920ceec23352))
CORE_CLONEDEF(bbh, 160, 170, "Big Buck Hunter Pro (1.6)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bbh_150,"bbh150a.bin",0x1BB8FA4,CRC(18bad072) SHA1(16e499046107baceda6f6c934d70ba2108915973))
CORE_CLONEDEF(bbh, 150, 170, "Big Buck Hunter Pro (1.5)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(bbh_140,"bbh140a.bin",0x1BB8FA4,CRC(302e29f0) SHA1(0c500c0a5588f8476a71599be70b515ba3e19cab))
CORE_CLONEDEF(bbh, 140, 170, "Big Buck Hunter Pro (1.4)", 2010, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Iron Man
/-------------------------------------------------------------------*/
SAM1_INIT(im, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(im_183ve,"im_183v.bin",29699164,CRC(e477183c) SHA1(6314b44b58c79889f95af1792395203dbbb36b0b))
CORE_GAMEDEF(im, 183ve, "Iron Man (1.83 Vault Edition)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(im_183,"im_183.bin",29699164,CRC(cf2791a6) SHA1(eb616e3bf33024374f4e998a579bc88f63282ba6))
CORE_CLONEDEF(im, 183, 183ve, "Iron Man (1.83)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(im_182,"im_182.bin",29699164,CRC(c65aff0b) SHA1(ce4d26ffdfd8539e8f7fca78dfa55f80247f9334))
CORE_CLONEDEF(im, 182, 183ve, "Iron Man (1.82)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(im_181,"im_181.bin",29699164,CRC(915d972b) SHA1(0d29929ae304bc4bbdbab7813a279f3200cac6ef))
CORE_CLONEDEF(im, 181, 183ve, "Iron Man (1.81)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(im_160,"im_160e.bin",0x1C1FD64,CRC(ed0dd2bb) SHA1(789b9dc5f5d97a86eb406f864f2785f371db6ca5))
CORE_CLONEDEF(im, 160, 183ve, "Iron Man (1.6)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(im_140,"im_140e.bin",0x1CB8870,CRC(9cbfd6ef) SHA1(904c058a00c268593a62988127f8a18d974eda5e))
CORE_CLONEDEF(im, 140, 183ve, "Iron Man (1.4)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(im_120,"im_120e.bin",0x1B8FE44,CRC(71df27ad) SHA1(9e1745522d28af6bdcada56f2cf0b489656fc885))
CORE_CLONEDEF(im, 120, 183ve, "Iron Man (1.2)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(im_110,"im_110e.bin",0x1B8FE44,CRC(3140cb7c) SHA1(20b0e84b61069e09f189d79e6b4d5abf0369a893))
CORE_CLONEDEF(im, 110, 183ve, "Iron Man (1.1)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(im_100,"im_100e.bin",0x1B8FE44,CRC(b27d12bf) SHA1(dfb497f2edaf4321823b243cced9d9e2b7bac628))
CORE_CLONEDEF(im, 100, 183ve, "Iron Man (1.0)", 2010, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ James Cameron's Avatar
/-------------------------------------------------------------------*/
SAM1_INIT(avr, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(avr_110,"av_110.bin",0x1D53CA4,CRC(e28df0a8) SHA1(7bc42d329efcb59d71af1736d8881c14ce3f7e5e))
CORE_GAMEDEF(avr, 110, "Avatar, James Cameron's (1.1)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(avr_106,"av_106.bin",0x1ED31B4,CRC(695799e5) SHA1(3e216fd4273adb7417294b3e648befd69350ab25))
CORE_CLONEDEF(avr, 106, 110, "Avatar, James Cameron's (1.06)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(avr_101h,"av_101_e.bin",0x1EE1CB8,CRC(dbdcc7e5) SHA1(bf9a79209ecdae93efb2930091d2658259a3bd03))
CORE_CLONEDEF(avr, 101h, 110, "Avatar, James Cameron's (1.01 Limited Edition)", 2010, "Stern", sam1, 0)

SAM1_ROM32MB(avr_120h,"av_120_e.bin",0x1D53CA4,CRC(85a55e02) SHA1(204d796c2cbc776c1305dabade6306527122a13e))
CORE_CLONEDEF(avr, 120h, 110, "Avatar, James Cameron's (1.2 Limited Edition)", 2011, "Stern", sam1, 0)

//Avatar Pro FTDI-USB CPU Board Part #520-5246-02 ONLY
SAM1_ROM32MB(avr_200,"avt200v.bin",0x1D53CA4,CRC(dc225785) SHA1(ecaba25a470bf03e6e43ab8779d14898e1b8e67f))
CORE_CLONEDEF(avr, 200, 110, "Avatar, James Cameron's (2.0)", 2013, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ The Rolling Stones
/-------------------------------------------------------------------*/
SAM1_INIT(rsn, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(rsn_110,"rsn_110.bin",0x1EB4FC4,CRC(f4aad67f) SHA1(f5dc335a2b9cc92b3da9a33e24cd0b155c6385aa))
CORE_GAMEDEF(rsn, 110, "Rolling Stones, The (1.1)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(rsn_105,"rsn_105.bin",0x1EB4FC0,CRC(58025883) SHA1(7f63bbe98f1151e0276ede1412ed5960ce9b3395))
CORE_CLONEDEF(rsn, 105, 110, "Rolling Stones, The (1.05)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(rsn_103,"rsn_103.bin",0x1D38788,CRC(2039ac97) SHA1(4cbcc758fc74dd32f5804b9548645fba3431bdce))
CORE_CLONEDEF(rsn, 103, 110, "Rolling Stones, The (1.03)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(rsn_100h,"rsn_100h.bin",0x1EB50C8,CRC(7cdb082a) SHA1(2f35057b80ffeec05cdbc62bc86da8a32f859425))
CORE_CLONEDEF(rsn, 100h, 110, "Rolling Stones, The (1.00 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(rsn_110h,"rsn_110h.bin",0x1EB50CC,CRC(f5122852) SHA1(b92461983d7a3b55ac8be4df4def1b4ca12327af))
CORE_CLONEDEF(rsn, 110h, 110, "Rolling Stones, The (1.1 Limited Edition)", 2011, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ TRON: Legacy
/-------------------------------------------------------------------*/
SAM1_INIT(trn, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(trn_174,"trn_174e.bin",0x1F79E70,CRC(20e44481) SHA1(88e6e75efb640a7978f4003f0df5ee1e41087f72))
CORE_GAMEDEF(trn, 174, "TRON: Legacy (1.74)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_17402,"trn_1742.bin",0x1F79E70,CRC(94a5946c) SHA1(5026e33a8bb00c83caf06891727b8439d1274fbb))
CORE_CLONEDEF(trn, 17402, 174, "TRON: Legacy (1.7402)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(trn_170,"trn_170e.bin",0x1F13C9C,CRC(1f3b314d) SHA1(59df759539c02600d2579b4e59a184ac3db64020))
CORE_CLONEDEF(trn, 170, 174, "TRON: Legacy (1.7)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_160,"trn_160e.bin",0x1F0F314,CRC(38eb16b1) SHA1(4f18080d76e07a3308497116cf3e39a7fab4cd25))
CORE_CLONEDEF(trn, 160, 174, "TRON: Legacy (1.6)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_150,"trn_150e.bin",0x1F0420C,CRC(5ac4021b) SHA1(740187e7a60a32b1d21708a4194e0524211b53a7))
CORE_CLONEDEF(trn, 150, 174, "TRON: Legacy (1.50)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_140,"trn_140e.bin",0x1E464E8,CRC(7c9ce1bd) SHA1(c75a6a4c7f0d72460061e5532e1d604f6ea829e3))
CORE_CLONEDEF(trn, 140, 174, "TRON: Legacy (1.4)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_120,"trn_120e.bin",0x1C7C978,CRC(80dcb8a2) SHA1(9f0810969058222c0104f2b35d17e14bf3f5f8e8))
CORE_CLONEDEF(trn, 120, 174, "TRON: Legacy (1.20)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_110,"trn_110e.bin",0x1AB97D8,CRC(bdaf1803) SHA1(f32d5bfb87be85483b0486bbb6f2858efca6efe5))
CORE_CLONEDEF(trn, 110, 174, "TRON: Legacy (1.10)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_100h,"trnle100.bin",0x1F19368,CRC(4c2abebd) SHA1(8e22454932680351d58f863cf9644a9f3db24800))
CORE_CLONEDEF(trn, 100h, 174, "TRON: Legacy (1.00 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_110h,"trnle110.bin",0x1F19368,CRC(43a7e45a) SHA1(b03798f00fe481f662ed07fbf7a14766bccbb92e))
CORE_CLONEDEF(trn, 110h, 174, "TRON: Legacy (1.10 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_130h,"trnle130.bin",0x1F1ED40,CRC(adf02601) SHA1(6e3c2706e39a1c01a002ceaea839b934cdac28bc))
CORE_CLONEDEF(trn, 130h, 174, "TRON: Legacy (1.30 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_140h,"trnle140.bin",0x1F286D8,CRC(7de92a4b) SHA1(87eb46e1564b8a913d6cc17a86b50828dd1273de))
CORE_CLONEDEF(trn, 140h, 174, "TRON: Legacy (1.40 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_174h,"trnle174.bin",0x1F93B84,CRC(a45224bf) SHA1(40e36764af332175f653e8ddc2a8bb77891c1230))
CORE_CLONEDEF(trn, 174h, 174, "TRON: Legacy (1.74 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(trn_1741h,"trn_1741h.bin",33110916,CRC(dbce28f1) SHA1(39cee4095964a2e73b86b1687d895d6cb367d8a5))
CORE_CLONEDEF(trn, 1741h, 174, "TRON: Legacy (1.741 Limited Edition hacked replaced music)", 2015, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Transformers
/-------------------------------------------------------------------*/
SAM1_INIT(tf, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(tf_180,"tf_v180.bin",0x1D24F34,CRC(0b6e3a4f) SHA1(62e1328e8462680694157aca266055d57347e904))
CORE_GAMEDEF(tf, 180, "Transformers (1.8)", 2013, "Stern", sam1, 0)

SAM1_ROM32MB(tf_170,"tf_v170.bin",0x1D24F34,CRC(cd8707e6) SHA1(847c37988bbc12e8200a6762c2851b610a0b849f))
CORE_CLONEDEF(tf, 170, 180, "Transformers (1.7)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(tf_160,"tf_v160.bin",0x1D0C800,CRC(1D0C800) SHA1(10dd73ade7b662bf17b95c2413d23fa942e54660))
CORE_CLONEDEF(tf, 160, 180, "Transformers (1.60)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_150,"tf_v150.bin",0x1D0C800,CRC(130cbed2) SHA1(b299b1b25a6007cbec0ea15d2a156a197215e288))
CORE_CLONEDEF(tf, 150, 180, "Transformers (1.50)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_140,"tf_v140.bin",0x1D0C800,CRC(b41c8d33) SHA1(e96462df7481759d5c29a192766f03334b2b4562))
CORE_CLONEDEF(tf, 140, 180, "Transformers (1.4)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_120,"tf_v120.bin",0x1E306CC,CRC(a6dbb32d) SHA1(ac1bef87278ff1ebc98d66cc062c3a7e49580a82))
CORE_CLONEDEF(tf, 120, 180, "Transformers (1.2)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_088h,"tfle_088.bin",0x1EB4CE8,CRC(a79ca893) SHA1(8f1228727422f5f99a20d60968eeca6c64f6c253))
CORE_CLONEDEF(tf, 088h, 180, "Transformers (0.88 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_100h,"tfle_100.bin",0x1F4CCF0,CRC(3be6ffc2) SHA1(c57d00af7ea189ea37ceed28bf85cff1054a1b8c))
CORE_CLONEDEF(tf, 100h, 180, "Transformers (1.00 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_120h,"tfle_120.bin",0x1EB1C4C,CRC(0f750246) SHA1(7ab3c9278f443511e5e7fcf062ffc9e8d1456396))
CORE_CLONEDEF(tf, 120h, 180, "Transformers (1.2 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_130h,"tfle_130.bin",0x1EB1C4C,CRC(8a1b676f) SHA1(d74f6060091293e6a781e129d19a408baabcf716))
CORE_CLONEDEF(tf, 130h, 180, "Transformers (1.30 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_140h,"tfle_140.bin",0x1EB1C4C,CRC(7e7920d6) SHA1(9db41874081d5f28adb5ab23903038f5c959eb1d))
CORE_CLONEDEF(tf, 140h, 180, "Transformers (1.40 Limited Edition)", 2011, "Stern", sam1, 0)

SAM1_ROM32MB(tf_150h,"tfle_150.bin",0x1EB1E5C,CRC(5cec6bfc) SHA1(30899241c2c0a9d42aa19fa3eb4180452bdaec91))
CORE_CLONEDEF(tf, 150h, 180, "Transformers (1.5 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(tf_180h,"tfle_180.bin",0x1EB1E04,CRC(467aeeb3) SHA1(feec42b083d81e632ef8ae402eb9f20f3104db08))
CORE_CLONEDEF(tf, 180h, 180, "Transformers (1.8 Limited Edition)", 2013, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ AC/DC
/-------------------------------------------------------------------*/
SAM1_INIT(acd, sam1_dmd128x32, 2, 0)
SAM1_ROM128MB(acd_168,"acd_168e.bin",119685024,CRC(9fdcb32e) SHA1(f36b289e1868a051f4302b2551750b750fa52e30))
CORE_GAMEDEF(acd, 168, "AC/DC (1.68)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(acd_165,"acd_165e.bin",0x7223FA0,CRC(819b2b35) SHA1(f29814ba985a5887f5cd382666e7f14f8d6e3702))
CORE_CLONEDEF(acd, 165, 168, "AC/DC (1.65)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_163,"acd_163e.bin",0x4E5EFE8,CRC(0bf53436) SHA1(0758d4a881ce87c9af90132741bf1e5c89fc575b))
CORE_CLONEDEF(acd, 163, 168, "AC/DC (1.63)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_161,"acd_161e.bin",0x4E58B6C,CRC(a0c27c59) SHA1(83d19fe6b344eb95866f7d5179b65ed26938b9da))
CORE_CLONEDEF(acd, 161, 168, "AC/DC (1.61)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_160,"acd_160e.bin",0x4E942C8,CRC(6b98a14c) SHA1(a34841b1e136c9647c89f83e2bf59ecdccb2a0fb))
CORE_CLONEDEF(acd, 160, 168, "AC/DC (1.6)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_152,"acd_152e.bin",0x4185458,CRC(78cef38c) SHA1(656acabed2241587f512cdd53a095228d9642d1b))
CORE_CLONEDEF(acd, 152, 168, "AC/DC (1.52)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_150,"acd_150e.bin",0x41308CC,CRC(8b4c0fae) SHA1(d973be4306a7fc1ff9f898145197081cbe823584))
CORE_CLONEDEF(acd, 150, 168, "AC/DC (1.50)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_140,"acd_140e.bin",0x40C4038,CRC(43bbbf54) SHA1(33e3795ab850dfab1fd8b1b4f6364a696cc62aa9))
CORE_CLONEDEF(acd, 140, 168, "AC/DC (1.4)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_130,"acd_130e.bin",0x40C4038,CRC(da97014e) SHA1(f0a2684076008b0234c089fea8f95e4f3d8816dd))
CORE_CLONEDEF(acd, 130, 168, "AC/DC (1.3)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_125,"acd_125.bin",0x3E53DEC,CRC(0307663f) SHA1(d40e3aaf94d1d314835fa59a177ce0c386399f4c))
CORE_CLONEDEF(acd, 125, 168, "AC/DC (1.25)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_121,"acd_121.bin",0x3D8F40C,CRC(4f5f43e9) SHA1(19045e9cdb2522770013c24c6fed265009278dea))
CORE_CLONEDEF(acd, 121, 168, "AC/DC (1.21)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_150h,"acle_150.bin",0x40F555C,CRC(1b8b823d) SHA1(9cda6a3f609e94d93126e105ac2945151006325b))
CORE_CLONEDEF(acd, 150h, 168, "AC/DC (1.50 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_152h,"acle_152.bin",0x414A0E8,CRC(bbf6b303) SHA1(8f29a5e8b5503df59ec8a6039a36e78cf7d871a9))
CORE_CLONEDEF(acd, 152h, 168, "AC/DC (1.52 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_160h,"acle_160.bin",0x4E942E0,CRC(733f15a4) SHA1(61e96ceac327387e84b8e24467aee2f5c0a8ce97))
CORE_CLONEDEF(acd, 160h, 168, "AC/DC (1.6 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_161h,"acle_161.bin",0x4E58B6C,CRC(1c66055b) SHA1(f33e5bd5753acc90202565639b6a8d22d6380054))
CORE_CLONEDEF(acd, 161h, 168, "AC/DC (1.61 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_163h,"acle_163.bin",0x4E5EFE8,CRC(12c404e8) SHA1(e3a5937abaa9e5b4b18b214b4f5c74f1c110247f))
CORE_CLONEDEF(acd, 163h, 168, "AC/DC (1.63 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_165h,"acle_165.bin",0x7223FA0,CRC(9f9c41e9) SHA1(b4a61944218ab57af128e91b032a82342c8c4ccc))
CORE_CLONEDEF(acd, 165h, 168, "AC/DC (1.65 Limited Edition)", 2012, "Stern", sam2, 0)

SAM1_ROM128MB(acd_168h,"acle_168.bin",0x7223FA0,CRC(5a4246a1) SHA1(725eb666ffaef894d2bd694d412658395c7fa7f9))
CORE_CLONEDEF(acd, 168h, 168, "AC/DC (1.68 Limited Edition)", 2014, "Stern", sam2, 0)

//!! SAM1_ROM128MB(acd_168_c, "acd_168c.bin",0x077FFFF0,CRC(5a4246a1) SHA1(725eb666ffaef894d2bd694d412658395c7fa7f9))
SAM1_ROM128MB(acd_168h_c,"acle_168_c.bin",125829104,CRC(748e55b7) SHA1(febdf5f5cc03e985a25ce47639d87e90bcd97bc6))
CORE_CLONEDEF(acd, 168h_c, 168, "AC/DC (1.68 Limited Edition Color)", 2014, "Stern", sam2, 0)

/*-------------------------------------------------------------------
/ X-Men // uses sam1 and SAM1_ROM32MB instead of sam2 and SAM1_ROM128MB, should be okay
/-------------------------------------------------------------------*/
SAM1_INIT(xmn, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(xmn_151,"xmen_151.bin",33182312,CRC(84c744a4) SHA1(db4339be7e9d47c46a13f95520dfe58da8450a19))
CORE_GAMEDEF(xmn, 151, "X-Men (1.51)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_150,"xmen_150.bin",0x1FA5268,CRC(fc579436) SHA1(2aa71da4a5f61165e41e7a63f3534202880c3b90))
CORE_CLONEDEF(xmn, 150, 151, "X-Men (1.5)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_130,"xmen_130.bin",0x1FB887C,CRC(1fff4f39) SHA1(e8c02ab980499fbb81569ce1f191d0d2e5c13234))
CORE_CLONEDEF(xmn, 130, 151, "X-Men (1.3)", 2013, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_105,"xmen_105.bin",0x1FB850C,CRC(e585d64b) SHA1(6126b29c9355398bac427e1b214e58e8e407bec4))
CORE_CLONEDEF(xmn, 105, 151, "X-Men (1.05)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_104,"xmen_104.bin",0x1FB7DEC,CRC(59f2e106) SHA1(10e9fb0ec72462654c0e4fb53c5cc9f2cbb3dbcb))
CORE_CLONEDEF(xmn, 104, 151, "X-Men (1.04)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_102,"xmen_102.bin",0x1FB7DEC,CRC(5df923e4) SHA1(28f86abc792008aa816d93e91dcd9b62fd2d01ee))
CORE_CLONEDEF(xmn, 102, 151, "X-Men (1.02)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_100,"xmen_100.bin",0x1FB7DEC,CRC(997b2973) SHA1(68bb379860a0fe5be6a8a8f28b6fd8fe640e172a))
CORE_CLONEDEF(xmn, 100, 151, "X-Men (1.0)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_120h,"xmle_120.bin",0x1FB7DEC,CRC(93da2d0b) SHA1(92c4c2e7fe6392e4ff8824d5b217dcbda8ce3a96))
CORE_CLONEDEF(xmn, 120h, 151, "X-Men (1.2 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_121h,"xmle_121.bin",0x1FB7DEC,CRC(7029ce71) SHA1(c7559ed963e18eecb8115214a3e154874c214f89))
CORE_CLONEDEF(xmn, 121h, 151, "X-Men (1.21 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_122h,"xmle_122.bin",0x1FB7DEC,CRC(3609e1be) SHA1(86d368297ec6ca3b132c6e8dab17cd1c1c18bde2))
CORE_CLONEDEF(xmn, 122h, 151, "X-Men (1.22 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_123h,"xmle_123.bin",0x1FB7DEC,CRC(66c74598) SHA1(c0c0cd2e8e37eba6668aaadab76325afca103b32))
CORE_CLONEDEF(xmn, 123h, 151, "X-Men (1.23 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_124h,"xmle_124.bin",0x1FB850C,CRC(662591e9) SHA1(1abb26c589fbb1b5a4ec5577a4a842e8a84484a3))
CORE_CLONEDEF(xmn, 124h, 151, "X-Men (1.24 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_130h,"xmle_130.bin",0x1FB887C,CRC(b2a7f125) SHA1(a42834c3e562c239f56c27c0cb65885fdffd261c))
CORE_CLONEDEF(xmn, 130h, 151, "X-Men (1.3 Limited Edition)", 2013, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_150h,"xmle_150.bin",33182312,CRC(8e2c3870) SHA1(ddfb4370bb4f32d440538f1432d1be09df9b5557))
CORE_CLONEDEF(xmn, 150h, 151, "X-Men (1.5 Limited Edition)", 2014, "Stern", sam1, 0)

SAM1_ROM32MB(xmn_151h,"xmle_151.bin",33182312,CRC(21d1088f) SHA1(9a0278c0324fbf549b5b7bcc93bc327f3eb65e19))
CORE_CLONEDEF(xmn, 151h, 151, "X-Men (1.51 Limited Edition)", 2014, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ The Avengers // uses sam1 and SAM1_ROM32MB instead of sam2 and SAM1_ROM128MB, should be okay
/-------------------------------------------------------------------*/
SAM1_INIT(avs, sam1_dmd128x32, 2, 0)
SAM1_ROM32MB(avs_140,"as_140.bin",0x1F2EDA0,CRC(92642508) SHA1(1d55cd178104b43377f079fd0209d74d1b10bea8))
CORE_GAMEDEF(avs, 140, "Avengers, The (1.40)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(avs_110,"as_110.bin",0x1D032AC,CRC(2cc01e3c) SHA1(0ae7c9ced7e1d48b0bf4afadb6db508e558a7ebb))
CORE_CLONEDEF(avs, 110, 140, "Avengers, The (1.10)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(avs_170, "as_170.bin", 32885136, CRC(aa4a7203) SHA1(f2f4a9851097a07291f3469f94362f4cb1f7a127))
CORE_CLONEDEF(avs, 170, 140, "Avengers, The (1.70)", 2016, "Stern", sam1, 0)

SAM1_ROM32MB(avs_120h,"asle_120.bin",0x1E270D0,CRC(a74b28c4) SHA1(35f65691312c547ec6c6bf52d0c5e330b5d464ca))
CORE_CLONEDEF(avs, 120h, 140, "Avengers, The (1.20 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(avs_140h,"asle_140.bin",0x1F2EDA0,CRC(9b7e13f8) SHA1(eb97e92013a8d1d706a119b50d36d69eb26cb273))
CORE_CLONEDEF(avs, 140h, 140, "Avengers, The (1.40 Limited Edition)", 2012, "Stern", sam1, 0)

SAM1_ROM32MB(avs_170h, "asle_170.bin", 32885136, CRC(07feb01c) SHA1(25cca6c2f8fc2e3a38a72263cb25cefaf7f3b832))
CORE_CLONEDEF(avs, 170h, 140, "Avengers, The (1.70 Limited Edition)", 2016, "Stern", sam1, 0)

SAM1_ROM32MB(avs_170_c, "as_170_c.bin", 0x01FFFFF0, CRC(ff1a39e5) SHA1(44949f8aca36a8a1896fe253278ef7f146764d79))
CORE_CLONEDEF(avs, 170_c, 140, "Avengers, The (1.70 Color)", 2016, "Stern", sam1, 0)

/*-------------------------------------------------------------------
/ Metallica
/-------------------------------------------------------------------*/
SAM1_INIT(mtl, sam1_dmd128x32, 2, 0)
SAM1_ROM128MB(mtl_163,"mtl_163.bin",98369848,CRC(94d38355) SHA1(0f51c3d99e1227dcde132738ef539d0d452ca003))
CORE_GAMEDEF(mtl, 163, "Metallica (1.63)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_163d,"mtl_163d.bin",0x5DD0138,CRC(de390393) SHA1(23a9f02514bc592e0799c91cd42786809bfc8c1d))
CORE_CLONEDEF(mtl, 163d, 163, "Metallica (1.63) LED", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_164, "mtl_164.bin", 0x5DD0138, CRC(668b8cfa) SHA1(65c6bb31ace6b4ce70e99c42c040f734de235bd0))
CORE_CLONEDEF(mtl, 164, 163, "Metallica (1.64)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_170, "mtl_170.bin", 0x5DDD0BC, CRC(2bdc4668) SHA1(1c28f4d1e3a2c36a045cadb00a0aa8494b1d9243))
CORE_CLONEDEF(mtl, 170, 163, "Metallica (1.70)", 2016, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_160,"mtl_160.bin",98369820,CRC(c440d2f5) SHA1(4584542430579f9ff0174f4dd2817afbc778bc40))
CORE_CLONEDEF(mtl, 160, 163, "Metallica (1.6)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_151,"mtl_151.bin",97796096,CRC(dac2819e) SHA1(c57fe4252a7a84cd543458bc54038b6ae9d79816))
CORE_CLONEDEF(mtl, 151, 163, "Metallica (1.51)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_150,"mtl_150.bin",0x542CA94,CRC(647cbad4) SHA1(d49906906a075a656d7768bffc47bd88a6306699))
CORE_CLONEDEF(mtl, 150, 163, "Metallica (1.5)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_122,"mtl_122.bin",0x512C72C,CRC(5201e6a6) SHA1(46e76f3e518448627419edf1aa08cc42259b39d2))
CORE_CLONEDEF(mtl, 122, 163, "Metallica (1.22)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_120,"mtl_120.bin",0x512C72C,CRC(43028b40) SHA1(64ab9306b28f3dc59e645ce49dbf3468e7f590bd))
CORE_CLONEDEF(mtl, 120, 163, "Metallica (1.2)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_116,"mtl_116.bin",0x50FFF04,CRC(85793613) SHA1(454813cb405bb6bda1d26288b10606c3a4ec72fc))
CORE_CLONEDEF(mtl, 116, 163, "Metallica (1.16)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_113,"mtl_113.bin",0x4FDCD28,CRC(be73c2e7) SHA1(c91bbb554aaa21520360773e1215fe80557d6c2f))
CORE_CLONEDEF(mtl, 113, 163, "Metallica (1.13)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_112,"mtl_112.bin",0x4FDCD28,CRC(093ba7ef) SHA1(e49810ca3500be503343296105ea9dd85e2c00f0))
CORE_CLONEDEF(mtl, 112, 163, "Metallica (1.12)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_106,"mtl_106.bin",0x4E8A214,CRC(5ac6c70a) SHA1(aaa68eebd1b894383416d2a491ac074a73be8d91))
CORE_CLONEDEF(mtl, 106, 163, "Metallica (1.06)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_105,"mtl_105.bin",0x4DEDE5C,CRC(4699e2cf) SHA1(b56e85583362056b33f7b8eb6255d34d234ea5ea))
CORE_CLONEDEF(mtl, 105, 163, "Metallica (1.05)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_103,"mtl_103.bin",0x4D24D04,CRC(9b073858) SHA1(129872e38d21d9d6d20f81388825113f13645bab))
CORE_CLONEDEF(mtl, 103, 163, "Metallica (1.03)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_113h,"mtl_113h.bin",0x4FDCD28,CRC(3392e27c) SHA1(3cfa5c5fdc51bd6886ffe6865739cf71de145ef1))
CORE_CLONEDEF(mtl, 113h, 163, "Metallica (1.13 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_116h,"mtl_116h.bin",0x50FFF04,CRC(3a96d383) SHA1(671fae4565c0739d98c9c40b05b9b41ae7917671))
CORE_CLONEDEF(mtl, 116h, 163, "Metallica (1.16 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_120h,"mtl_120h.bin",0x512C72C,CRC(1c5b4643) SHA1(8075e7ecf1031fc89e75cc5a5487340bc3fae507))
CORE_CLONEDEF(mtl, 120h, 163, "Metallica (1.2 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_122h,"mtl_122h.bin",0x512C72C,CRC(0d439a2b) SHA1(91eb84184cb93cfa83e0129a448760ac6586e85d))
CORE_CLONEDEF(mtl, 122h, 163, "Metallica (1.22 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_150h,"mtl_150h.bin",88263316,CRC(9f314ca1) SHA1(b620c4742a3ce137cbb099857d96fb6af67b7fec))
CORE_CLONEDEF(mtl, 150h, 163, "Metallica (1.5 Limited Edition)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_151h,"mtl_151h.bin",97796096,CRC(18e5a613) SHA1(ebd697bdc8f67188e160e2f8e76f908206127d26))
CORE_CLONEDEF(mtl, 151h, 163, "Metallica (1.51 Limited Edition)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_160h,"mtl_160h.bin",0x5DD011C,CRC(cb69b0fe) SHA1(aa7275f33db95742a8b1ae8a5f6973a0b27953fa))
CORE_CLONEDEF(mtl, 160h, 163, "Metallica (1.6 Limited Edition)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_163h,"mtl_163h.bin",0x5DD0138,CRC(12c1a5bb) SHA1(701eda4251ebdfcce2bea3ec9c84ac9c35832e2f))
CORE_CLONEDEF(mtl, 163h, 163, "Metallica (1.63 Limited Edition)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_164h, "mtl_164h.bin", 0x05DD0138, CRC(ebbf5845) SHA1(4411279b3e4ea9621638bb81e47dc8753bfc0a05))
CORE_CLONEDEF(mtl, 164h, 163, "Metallica (1.64 Limited Edition)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_170h, "mtl_170h.bin", 0x5DDD0BC, CRC(99d42a4e) SHA1(1983a6d1cd5664cf03599b035f520e0c6aa33632))
CORE_CLONEDEF(mtl, 170h, 163, "Metallica (1.70 Limited Edition)", 2016, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_164h_c, "mtl_164h_c.bin", 100663280, CRC(5d5a3493) SHA1(64f5d8f826c59d4ceac9fb06e18c4a3c0a50e4f9))
CORE_CLONEDEF(mtl, 164h_c, 163, "Metallica (1.64 Limited Edition Color)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(mtl_164_c, "mtl_164_c.bin", 0x05FFFFF0, CRC(ebbf5845) SHA1(4411279b3e4ea9621638bb81e47dc8753bfc0a05))
CORE_CLONEDEF(mtl, 164_c, 163, "Metallica (1.64 Color)", 2015, "Stern", sam2, 0)

/*-------------------------------------------------------------------
/ Star Trek
/-------------------------------------------------------------------*/
SAM1_INIT(st, sam1_dmd128x32, 2, 0)
SAM1_ROM128MB(st_161,"st_161.bin",58373588,CRC(e7a923ce) SHA1(d7f676a13bfa93b540af8469adb2bd20dda681a8))
CORE_GAMEDEF(st, 161, "Star Trek (1.61)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(st_161_c,"st_161_c.bin",0x037FFFF0,CRC(74ad8a31) SHA1(18c940d021441ba87854f5eb6edb84aeffabdaae))
CORE_CLONEDEF(st, 161_c, 161, "Star Trek (1.61 Color)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(st_160,"st_160.bin",58373588,CRC(cf0e0b60) SHA1(d91cfcd3ea28f174d9e7a9cff85a5ba6bccb0f34))
CORE_CLONEDEF(st, 160, 161, "Star Trek (1.60)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(st_150,"st_150.bin",56306580,CRC(979c9644) SHA1(20a89ad337690a9ab652a599a77e30ccf2018e14))
CORE_CLONEDEF(st, 150, 161, "Star Trek (1.50)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(st_140,"st_140.bin",49791896,CRC(c4f97ce3) SHA1(ef2d7cef153b5a6e9ab90c1ea31fdf5667eb327f))
CORE_CLONEDEF(st, 140, 161, "Star Trek (1.40)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(st_130,"st_130.bin",45554328,CRC(f501eb87) SHA1(6fa2f4e30cdd397d5443dfc690463495d22d9229))
CORE_CLONEDEF(st, 130, 161, "Star Trek (1.30)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(st_120,"st_120.bin",0x2B14CFC,CRC(dde9db23) SHA1(09e67564bce0ff7c67f1d16c4f9d8595f8130372))
CORE_CLONEDEF(st, 120, 161, "Star Trek (1.20)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(st_140h,"st_140h.bin",49791896,CRC(6f84cec4) SHA1(d92391005eed3c4dcb66ac0bccd19a50c4120792))
CORE_CLONEDEF(st, 140h, 161, "Star Trek (1.4 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(st_141h,"st_141h.bin",0x2F7C398,CRC(ae20d360) SHA1(0a840767b4e9fee26d7a4c2a9545fa7fd818d74e))
CORE_CLONEDEF(st, 141h, 161, "Star Trek (1.41 Limited Edition)", 2013, "Stern", sam2, 0)

SAM1_ROM128MB(st_142h,"st_142h.bin",49791896,CRC(01acc115) SHA1(9881ea34852890a3fc960b78db96b70f17a28e56))
CORE_CLONEDEF(st, 142h, 161, "Star Trek (1.42 Limited Edition)", 2014, "Stern", sam2, 0)

SAM1_INIT(st_150h, sam1_dmd128x32, 12, 0)
SAM1_ROM128MB(st_150h,"st_150h.bin",0x35B2B94,CRC(a187581c) SHA1(b68ca52140bafd6b309b120d38df5b3bcf633a13))
CORE_CLONEDEFNV(st_150h, st_161, "Star Trek (1.5 Limited Edition)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(st_150h_c,"st_150h_c.bin",56306580,CRC(7fffed8e) SHA1(6a886fa4332b51456079c64a8ba01a64f2bf1d6c))
CORE_CLONEDEF(st, 150h_c, 161, "Star Trek (1.5 Limited Edition Color)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(st_160h,"st_160h.bin",58373588,CRC(80419698) SHA1(2c4514d1712d9503828c2f57bfbec465026ac012))
CORE_CLONEDEF(st, 160h, 161, "Star Trek (1.6 Limited Edition)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(st_161h,"st_161h.bin",58373588,CRC(74ad8a31) SHA1(18c940d021441ba87854f5eb6edb84aeffabdaae))
CORE_CLONEDEF(st, 161h, 161, "Star Trek (1.61 Limited Edition)", 2015, "Stern", sam2, 0)


SAM1_ROM128MB(st_162,"st_162.bin",58373588,CRC(d11f7501) SHA1(6a8c46a2e975d9cbfb5f52d24f238e433ce19559))
CORE_CLONEDEF(st, 162, 161, "Star Trek (1.62 hacked replaced music)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(st_163,"st_163.bin",558373588,CRC(dceeb908) SHA1(0ff2f64e30bf969d49a0945e40c5e8ed0930192c))
CORE_CLONEDEF(st, 163, 161, "Star Trek (1.63 hacked replaced music)", 2015, "Stern", sam2, 0)

/*-------------------------------------------------------------------
/ Mustang
/-------------------------------------------------------------------*/
SAM1_INIT(mt, sam1_dmd128x32, 12, 0)
SAM1_ROM128MB(mt_140,"mt_140.bin",52851520,CRC(48010b61) SHA1(1bc615a86c4718ff407116a4e637e38e8386ded0))
CORE_GAMEDEF(mt, 140, "Mustang (1.40)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_145, "mt_145.bin", 52851520, CRC(67a38387) SHA1(31626b54a5b2dd7fbc98c4b97ed84ce1a6705955))
CORE_CLONEDEF(mt, 145, 140, "Mustang (1.45)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_120,"mt_120.bin",58566124,CRC(be7437ac) SHA1(5db10d7f48091093c33d522a663f13f262c08c3e))
CORE_CLONEDEF(mt, 120, 140, "Mustang (1.20)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_130,"mt_130.bin",52647740,CRC(b6086db1) SHA1(0a50864b0de1b4eb9a764f36474b6fddea767c0d))
CORE_CLONEDEF(mt, 130, 140, "Mustang (1.30)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_130h,"mt_130h.bin",0x3BF07F0,CRC(dcb5c923) SHA1(cf9e6042ae33080368ecffac233379135bf680ae))
CORE_CLONEDEF(mt, 130h, 140, "Mustang (1.30 Limited Edition)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_140h,"mt_140h.bin",0x3C2CCC4,CRC(fcb69947) SHA1(be64b13b3c6865f4fed1a8f03e9eaf84799fa2ab))
CORE_CLONEDEF(mt, 140h, 140, "Mustang (1.40 Limited Edition)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_140hb, "mt_140hb.bin", 0x03C2CCC4, CRC(64B660E9) SHA1(01d0f0e61e99bc53ccde853f1604cec5ab0c59cf))
CORE_CLONEDEF(mt, 140hb, 140, "Mustang (1.40 Boss)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_145h, "mt_145h.bin", 63098052, CRC(20ec78b3) SHA1(95443dd1d545de409a692793ad609ed651cb61d8))
CORE_CLONEDEF(mt, 145h, 140, "Mustang (1.45 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(mt_145hb, "mt_145hb.bin", 63098052, CRC(91fd5615) SHA1(0dbd7f3fc68218bcb10c893069d35447a445bc11))
CORE_CLONEDEF(mt, 145hb, 140, "Mustang (1.45 Boss)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

/*-------------------------------------------------------------------
/ The Walking Dead
/-------------------------------------------------------------------*/
SAM1_INIT(twd, sam1_dmd128x32, 2, 0)
SAM1_ROM128MB(twd_124,"twd_124.bin",94228524,CRC(9f30b0a9) SHA1(60f689717f9060260ef4ae32b11c3ca6e66004dc))
CORE_GAMEDEF(twd, 124, "Walking Dead, The (1.24)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_105,"twd_105.bin",0x4F4FBF8,CRC(59b4e4d6) SHA1(642e827d58c9877a9f3c29b75784660894f045ad))
CORE_CLONEDEF(twd, 105, 124, "Walking Dead, The (1.05)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(twd_111,"twd_111.bin",0x512CF54,CRC(6b2faad0) SHA1(1f3dd34e5f7cd7ae539b39c0f3c87b966d2c2f45))
CORE_CLONEDEF(twd, 111, 124, "Walking Dead, The (1.11)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(twd_119,"twd_119.bin",0x579167C,CRC(5fb5529e) SHA1(cdc3def52fd00219894327520122b905fd75ad1f))
CORE_CLONEDEF(twd, 119, 124, "Walking Dead, The (1.19)", 2014, "Stern", sam2, 0)

SAM1_ROM128MB(twd_125,"twd_125.bin",94228524,CRC(2eaa2387) SHA1(15f597a839e6e9e95de34b9a4bce7efa30474f02))
CORE_CLONEDEF(twd, 125, 124, "Walking Dead, The (1.25)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_128, "twd_128.bin", 94228524, CRC(42f2b016) SHA1(a2a59150286ab9cb14ae62b2fad9e7d8f53078fe))
CORE_CLONEDEF(twd, 128, 124, "Walking Dead, The (1.28)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_141, "twd_141.bin", 96276928, CRC(f160ecaf) SHA1(ecf09c58bfa68c4f65935f490bf5b3292cfe039d))
CORE_CLONEDEF(twd, 141, 124, "Walking Dead, The (1.41)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_153, "twd_153.bin", 96994376, CRC(59ca44ef) SHA1(1c89f9c5f6e15bcf2a245eb6c2ca5af6a19244ab))
CORE_CLONEDEF(twd, 153, 124, "Walking Dead, The (1.53)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_156, "twd_156.bin", 96994376, CRC(4bd62b0f) SHA1(b1d5e7d96f45fb3e076bc993b09a7eae9c73f610))
CORE_CLONEDEF(twd, 156, 124, "Walking Dead, The (1.56)", 2015, "Stern", sam2, 0)

SAM1_ROM128MB(twd_160, "twd_160.bin", 96994376, CRC(44409cd9) SHA1(cace8725771e9fc09720a7a79f95abac44325232))
CORE_CLONEDEF(twd, 160, 124, "Walking Dead, The (1.60)", 2017, "Stern", sam2, 0)

SAM1_INIT(twd_111h, sam1_dmd128x32, 12, 0)
SAM1_ROM128MB(twd_111h,"twd111le.bin",0x512CF54,CRC(873feba1) SHA1(3b3a76c09d39550554b89b0a300f72a42722470e))
CORE_CLONEDEFNV(twd_111h, twd_124, "Walking Dead, The (1.11 Limited Edition)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_119h,"twd119le.bin",0x579167C,CRC(529089e0) SHA1(bcc5b3f6f549212dfdc36eece220af6913a22f78))
CORE_CLONEDEF(twd, 119h, 124, "Walking Dead, The (1.19 Limited Edition)", 2014, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_124h,"twd124le.bin",94228524,CRC(85e24f0e) SHA1(c211676a1d202fa839f71f10e4168ffdc87a6159))
CORE_CLONEDEF(twd, 124h, 124, "Walking Dead, The (1.24 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_125h,"twd125le.bin",94228524,CRC(9a3b3ee6) SHA1(46609f708fcc7c6550bae024d7afd516e7fe46ad))
CORE_CLONEDEF(twd, 125h, 124, "Walking Dead, The (1.25 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_128h, "twd128le.bin", 0x059DD02C, CRC(bc2fb73b) SHA1(52c97d92c040886bd3b7abea61754ec3795aba94))
CORE_CLONEDEF(twd, 128h, 124, "Walking Dead, The (1.28 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_141h, "twd141le.bin", 0x05BD11C0, CRC(630e9c5e) SHA1(f3170ff138c30032b693b2b73ee704da879e3a0f))
CORE_CLONEDEF(twd, 141h, 124, "Walking Dead, The (1.41 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_153h, "twd153le.bin", 0x05C80448, CRC(941024A6) SHA1(5ed00791253d95efec4c07438053ddf0cc934238))
CORE_CLONEDEF(twd, 153h, 124, "Walking Dead, The (1.53 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_156h, "twd156le.bin", 0x05C80448, CRC(4594a287) SHA1(1e1a3b94bacf54a0c20cfa978db1284008c0e0a1))
CORE_CLONEDEF(twd, 156h, 124, "Walking Dead, The (1.56 Limited Edition)", 2015, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)

SAM1_ROM128MB(twd_160h, "twd160le.bin", 0x05C80448, CRC(1ed7b80a) SHA1(1fbaa077ec834ff9d289008ef1169e0e7fd68271))
CORE_CLONEDEF(twd, 160h, 124, "Walking Dead, The (1.60 Limited Edition)", 2017, "Stern", sam2, GAME_IMPERFECT_GRAPHICS)


SAM1_ROM128MB(twd_1191,"twd_1191.bin",91821692,CRC(f43a3f74) SHA1(b87f31ce1af8bd045d87b2c0d465e50c43138200))
CORE_CLONEDEF(twd, 1191, 124, "Walking Dead, The (1.191 hacked replaced music)", 2014, "Stern", sam2, 0)
