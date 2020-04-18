/************************************************************************************************
  Sega/Stern Pinball
  by Steve Ellenoff

  Additional work by Martin Adrian, Gerrit Volkenborn, Tom Behrens
************************************************************************************************

Hardware from 1994-2006

  CPU Boards: Whitestar System

  Display Boards:
    ??

  Sound Board Revisions:
    Integrated with CPU. Similar cap. as 520-5126-xx Series: (Baywatch to Batman Forever)

    11/2003 -New CPU/Sound board (520-5300-00) with an Atmel AT91 (ARM7DMI Variant) CPU for sound & Xilinx FPGA
             emulating the BSMT2000 as well as added 16 bit sample playback and ADPCM compression.

    07/2006 -New hardware system - S.A.M. (see sam.c for details)

Issues:
DMD Timing is still wrong.. FIRQ rate is variable, and it's not fully understood.

*************************************************************************************************/
#include <stdarg.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "core.h"
#include "sndbrd.h"
#include "dedmd.h"
#include "se.h"
#include "desound.h"
#include "cpu/at91/at91.h"
#include "cpu/arm7/arm7core.h"
#include "sound/wavwrite.h"
#ifdef PROC_SUPPORT
#include "p-roc/p-roc.h"
#endif

#define SE_FIRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define SE_ROMBANK0        1

#ifdef PROC_SUPPORT
// TODO/PROC: Make variables out of these defines. Values depend on "-proc" switch.
#define SE_SOLSMOOTH       1 /* Don't smooth values on real hardware */
#define SE_LAMPSMOOTH      1
#define SE_DISPLAYSMOOTH   1
#else
#define SE_SOLSMOOTH       4 /* Smooth the Solenoids over this numer of VBLANKS */
#define SE_LAMPSMOOTH      2 /* Smooth the lamps over this number of VBLANKS */
#define SE_DISPLAYSMOOTH   2 /* Smooth the display over this number of VBLANKS */
#endif

#define SUPPORT_TRACERAM 0

static NVRAM_HANDLER(se);
static WRITE_HANDLER(mcpu_ram8000_w);
static READ_HANDLER(mcpu_ram8000_r);

/*----------------
/ Local variables
/-----------------*/
struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  int    lampRow, lampColumn;
  int    diagnosticLed;
  int    swCol;
  int	 flipsol, flipsolPulse;
  int    sst0;			//SST0 bit from sound section
  int	 plin;			//Plasma In (not connected prior to LOTR Hardware)
  UINT8 *ram8000;
  int    auxdata;
  /* Mini DMD stuff */
  int    lastgiaux, miniidx, miniframe;
  int    minidata[7], minidmd[4][3][8];
  /* trace ram related */
#if SUPPORT_TRACERAM
  UINT8 *traceRam;
#endif
  UINT8  curBank;                   /* current bank select */
  #define TRACERAM_SELECTED 0x10    /* this bit set maps trace ram to 0x0000-0x1FFF */
  int fastflipaddr;
} selocals;

#ifdef PROC_SUPPORT
static int switches_retrieved=0;
#endif
static INTERRUPT_GEN(se_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  selocals.vblankCount = (selocals.vblankCount+1) % 16;

#ifdef PROC_SUPPORT
	if (coreGlobals.p_rocEn) {
		procTickleWatchdog();
	}
#endif

  /*-- lamps --*/
  if ((selocals.vblankCount % SE_LAMPSMOOTH) == 0) {
#ifdef PROC_SUPPORT
		if (coreGlobals.p_rocEn) {
			if (!switches_retrieved) {
				procSetSwitchStates();
				switches_retrieved = 1;
			}
			if (coreGlobals.p_rocEn) {
				int col, row;
				for(col = 0; col < 10; col++) {
					UINT8 chgLamps = coreGlobals.lampMatrix[col] ^ coreGlobals.tmpLampMatrix[col];
					UINT8 tmpLamps = coreGlobals.tmpLampMatrix[col];
					for (row = 0; row < 8; row++) {
						if (chgLamps & 0x01) {
							procDriveLamp( 80 + 16*(7-row) + col, tmpLamps & 0x01);
						}
						chgLamps >>= 1;
						tmpLamps >>= 1;
					}
				}
			}
		}
#endif
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, 10);
  }
  /*-- solenoids --*/
  coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xfff0) | selocals.flipsol; selocals.flipsol = selocals.flipsolPulse;
  if ((selocals.vblankCount % SE_SOLSMOOTH) == 0) {
    coreGlobals.solenoids = selocals.solenoids;
	// Fast flips.   Use Solenoid 15, this is the left flipper solenoid that is 
	// unused because it is remapped to VPM flipper constants.  
	if (selocals.fastflipaddr > 0 && memory_region(SE_CPUREGION)[selocals.fastflipaddr-1] > 0)  
		coreGlobals.solenoids |= 0x4000;
    selocals.solenoids = coreGlobals.pulsedSolState;
#ifdef PROC_SUPPORT
		if (coreGlobals.p_rocEn) {
			UINT64 allSol = core_getAllSol();
			if (coreGlobals.p_rocEn) {
				int ii;
				UINT64 chgSol = (allSol ^ coreGlobals.lastSol) & 0xffffffffffffffff; //vp_getSolMask64();
				UINT64 tmpSol = allSol;

				/* standard coils */
				for (ii = 0; ii < 28; ii++) {
					// TODO/PROC: disable LOTR flippers.  Hardcoded now.  Will need to do this dynamically
					if (chgSol & 0x01) {
						procDriveCoil(ii + 32, tmpSol & 0x01);
					}
					chgSol >>= 1;
					tmpSol >>= 1;
				}
			}
			procFlush();
			// TODO/PROC: This doesn't seem to be happening in core.c.  Why not?
			coreGlobals.lastSol = allSol;
		}
#endif
  }
#ifdef PROC_SUPPORT
	if (coreGlobals.p_rocEn) {
		procCheckActiveCoils();
	}
#endif
  /*-- display --*/
  if ((selocals.vblankCount % SE_DISPLAYSMOOTH) == 0) {
    coreGlobals.diagnosticLed = selocals.diagnosticLed;
    selocals.diagnosticLed = 0;
  }
  core_updateSw(TRUE); /* flippers are CPU controlled */
}

static SWITCH_UPDATE(se) {
#ifdef PROC_SUPPORT
	if (coreGlobals.p_rocEn) {
		procGetSwitchEvents();
	} else {
		// TODO/PROC: Really not necessary for P-ROC?
#endif
  if (inports) {
   if (core_gameData->hw.display & SE_LED2) {
    /*Switch Col 6 = Dedicated Switches */
    CORE_SETKEYSW(core_revbyte(inports[SE_COMINPORT])<<1, 0xe0, 7);
    /*Switch Col 12 = Dedicated Switches - Coin Door */
    CORE_SETKEYSW(core_revbyte(inports[SE_COMINPORT]>>8)>>4, 0x0e, 11);
    CORE_SETKEYSW(inports[SE_COMINPORT], 0x70, 11);
   } else {
    /*Switch Col 0 = Dedicated Switches - Coin Door Only - Begin at 6th Spot*/
    CORE_SETKEYSW(inports[SE_COMINPORT]<<4, 0xe0, 0);
    /*Switch Col 1 = Coin Switches - (Switches begin at 4th Spot)*/
    CORE_SETKEYSW(inports[SE_COMINPORT]>>5, 0x78, 1);
    /*Copy Start, Tilt, and Slam Tilt to proper position in Matrix: Switches 54,55,56*/
    /*Clear bits 6,7,8 first*/
    CORE_SETKEYSW(inports[SE_COMINPORT]<<1, 0xe0, 7);
   }
  }
#ifdef PROC_SUPPORT
	}
#endif
}

static MACHINE_INIT(se3) {
	//const char * const gn = Machine->gamedrv->name;
	sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
	sndbrd_1_init(SNDBRD_DE3S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);

	// Fast flips support.   My process for finding these is to load them in pinmame32 in VC debugger.  
	// Debug and break on this line:
	//  return memory_region(SE_CPUREGION)[offset];
	//
	// Add "&memory_region(0x81)[0]" to watch.  This will give you the current memory 
	// location of the WhiteStar RAM block where it goes, narrows things down a lot!
	//
	// Use CheatEngine to find memory locations that are 0 starting at the address
	// you got from the last step, through a couple KB more. 
	//
	// Load the balls in trough (W+SDFGHJ), start the game.   Usually have to futz with
	// W+SDF before the table moves onto BALL 1 and the flippers activate.
	// You can see when the flippers are active when four sets of dots change
	// on the bottom two blocks of dots.  When they are not active only 
	// the switches (middle block) flicker
	// Use CheatEngine to find memory locations that is 192 (so far all change to this,
	// except RollerCoaster Tycoon which was 240)
	// Validation:
	// Enter service menu.  Value should change back to 0.
	// Force value to be 192, the flippers should activate in service menu. 
	// Fastflipaddr is "+1" because a few were found at location 0!  

	// It appears all systems of se3 generation are the same... :) 
	selocals.fastflipaddr = 0x04 + 1;

/*	if (strncasecmp(gn, "sopranos", 8) == 0)
		selocals.fastflipaddr = 0x04 + 1;
	else if (strncasecmp(gn, "elvis", 5) == 0)
		selocals.fastflipaddr = 0x04 + 1;
	else if (strncasecmp(gn, "gprix", 5) == 0)
		selocals.fastflipaddr = 0x04 + 1;
	else if (strncasecmp(gn, "nascar", 6) == 0)
		selocals.fastflipaddr = 0x04 + 1;
	else if (strncasecmp(gn, "ripleys", 7) == 0)
		selocals.fastflipaddr = 0x04 + 1;
	else if (strncasecmp(gn, "lotr", 4) == 0)
		selocals.fastflipaddr = 0x04 + 1;*/
}

static MACHINE_INIT(se) {
  const char * const gn = Machine->gamedrv->name;
  sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
  sndbrd_1_init(SNDBRD_DE2S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);

  // for description on Fast flips, see above
  if (strncasecmp(gn, "sprk_103", 8) == 0)
	  selocals.fastflipaddr = 0x0 + 1;
  else if (strncasecmp(gn, "austin", 6) == 0)
	  selocals.fastflipaddr = 0x0 + 1;
  else if (strncasecmp(gn, "monopoly", 8) == 0)
	  selocals.fastflipaddr = 0xf0 + 1;
  else if (strncasecmp(gn, "twst_405", 8) == 0)
	  selocals.fastflipaddr = 0x14d + 1;
  else if (strncasecmp(gn, "shrkysht", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "harl_a30", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "hirolcas", 8) == 0)
	  selocals.fastflipaddr = 0x04 + 1;
  else if (strncasecmp(gn, "simpprty", 8) == 0)
	  selocals.fastflipaddr = 0x04 + 1;
  else if (strncasecmp(gn, "term3", 5) == 0)
	  selocals.fastflipaddr = 0x04 + 1;
  else if (strncasecmp(gn, "playboys", 8) == 0)
	  selocals.fastflipaddr = 0x04 + 1;
  else if (strncasecmp(gn, "rctycn", 6) == 0)
	  selocals.fastflipaddr = 0x04 + 1;
  // For apollo13, fast flips is not really necessary: The flipper "solenoids" are enable/disable flags and act just like the fast flip solenoid!
  // I'll leave it in there for consistency though since it makes the VP table counter part easier to code.
  else if (strncasecmp(gn, "apollo13", 8) == 0)
	  selocals.fastflipaddr = 0x122 + 1;
  else if (strncasecmp(gn, "godzilla", 8) == 0)
	  selocals.fastflipaddr = 0x0 + 1;
  else if (strncasecmp(gn, "id4", 3) == 0)
	  selocals.fastflipaddr = 0x150 + 1;
  else if (strncasecmp(gn, "lostspc", 7) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "jplstw22", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "spacejam", 8) == 0)
	  selocals.fastflipaddr = 0x14d + 1;
  else if (strncasecmp(gn, "swtril43", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "vipr_102", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "xfiles", 6) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "nfl", 3) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "startrp2", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "strikext", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  else if (strncasecmp(gn, "strxt_uk", 8) == 0)
	  selocals.fastflipaddr = 0x00 + 1;
  // Sharkeys got some extra ram
  if (core_gameData->gen & GEN_WS_1) {
    selocals.ram8000 = install_mem_write_handler(0,0x8000,0x81ff,mcpu_ram8000_w);
                       install_mem_read_handler (0,0x8000,0x81ff,mcpu_ram8000_r);
  }
  selocals.miniidx = selocals.lastgiaux = 0;
  if (core_gameData->hw.display & SE_LED2) selocals.miniidx = 1;
#if SUPPORT_TRACERAM
  selocals.traceRam = 0;
  if (core_gameData->gen & GEN_WS_1) {
    selocals.traceRam = malloc(0x2000);
    if (selocals.traceRam) memset(selocals.traceRam,0,0x2000);
  }
#endif
}

static MACHINE_STOP(se) {
  sndbrd_0_exit(); sndbrd_1_exit();
#if SUPPORT_TRACERAM
  if (selocals.traceRam) free(selocals.traceRam);
#endif
}

/*-- Main CPU Bank Switch --*/
// D0-D5 Bank Address
// D6    Unused
// D7    Diagnostic LED (0 = ON; 1 = OFF)
static WRITE_HANDLER(mcpu_bank_w) {
  selocals.curBank = data;   /* keep track of current bank select for trace ram */
  // Should be 0x3f but memreg is only 512K */
  cpu_setbank(SE_ROMBANK0, memory_region(SE_ROMREGION) + (data & 0x1f)* 0x4000);
  selocals.diagnosticLed = (data>>7)?0:1;
}

// Ram 0x0000-0x1FFF Read Handler
static READ_HANDLER(ram_r) {
  if (   (core_gameData->gen & GEN_WS_1)
      && (selocals.curBank & TRACERAM_SELECTED)) {
    DBGLOG(("Read access to traceram[%04X] from pc=%04X",offset,activecpu_get_previouspc()));
#if SUPPORT_TRACERAM
    return selocals.traceRam ? selocals.traceRam[offset] : 0;
#else
    return 0;
#endif
  }
  else
    return memory_region(SE_CPUREGION)[offset];
}

// Ram 0x0000-0x1FFF Write Handler
static WRITE_HANDLER(ram_w) {
  if (   (core_gameData->gen & GEN_WS_1)
      && (selocals.curBank & TRACERAM_SELECTED)) {
#if SUPPORT_TRACERAM
    if (selocals.traceRam) selocals.traceRam[offset] = data;
#else
    return;
#endif
  }
  else
    memory_region(SE_CPUREGION)[offset] = data;
}

/* Sharkey's ShootOut got some ram at 0x8000-0x81ff */
static WRITE_HANDLER(mcpu_ram8000_w) { selocals.ram8000[offset] = data; }
static READ_HANDLER(mcpu_ram8000_r) { return selocals.ram8000[offset]; }

/*-- Lamps --*/
static WRITE_HANDLER(lampdriv_w) {
  selocals.lampRow = core_revbyte(data);
  core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn, selocals.lampRow);
}
static WRITE_HANDLER(lampstrb_w) { core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0xff00) | data, selocals.lampRow);}
static READ_HANDLER(lampstrb_r) { return selocals.lampColumn & 0xff; }
static WRITE_HANDLER(auxlamp_w) { core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0x00ff) | (data<<8), selocals.lampRow);}
static READ_HANDLER(auxlamp_r) { return (selocals.lampColumn >> 8) & 0xff; }
static WRITE_HANDLER(gilamp_w) {
  logerror("GI lamps %d=%02x\n", offset, data);
  coreGlobals.tmpLampMatrix[10 + offset] = data;
}
static READ_HANDLER(gilamp_r) { return coreGlobals.tmpLampMatrix[10 + offset]; }

/*-- Switches --*/
static READ_HANDLER(switch_r)	{ return ~core_getSwCol(selocals.swCol); }
static WRITE_HANDLER(switch_w)	{ selocals.swCol = data; }

/*-- Dedicated Switches --*/
// Note: active low
// D0 - DED #1 - Left Flipper
// D1 - DED #2 - Left Flipper EOS
// D2 - DED #3 - Right Flipper
// D3 - DED #4 - Right Flipper EOS
// D4 - DED #5 - Not Used (Upper Flipper on some games!)
// D5 - DED #6 - Volume (Red Button)
// D6 - DED #7 - Service Credit (Green Button)
// D7 - DED #8 - Begin Test (Black Button)
static READ_HANDLER(dedswitch_r) {
  /* CORE Defines flippers in order as: RFlipEOS, RFlip, LFlipEOS, LFlip*/
  /* We need to adjust to: LFlip, LFlipEOS, RFlip, RFlipEOS*/
  /* Swap the 4 lowest bits*/
  UINT8 cds = (coreGlobals.swMatrix[0] & 0xe0);        /* BGRx xxxx */
  UINT8 fls = coreGlobals.swMatrix[CORE_FLIPPERSWCOL]; /* Uxxx LxRx */
  fls = core_revnyb(fls & 0x0f) | ((fls & 0x80)>>3);   /* xxxU xRxL */
  return ~(cds | fls);
}

/*-- Dip Switch SW300 - Country Settings --*/
static READ_HANDLER(dip_r) { return ~core_getDip(0); }

/*-- Solenoids --*/
static const int solmaskno[] = { 8, 0, 16, 24 };
static WRITE_HANDLER(solenoid_w) {
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];
  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    selocals.flipsol |= selocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  selocals.solenoids |= sols;
}
// Some Whitestar II ROMS read from the solenoid ports
static READ_HANDLER(solenoid_r) {
  int data = (coreGlobals.pulsedSolState >> solmaskno[offset]) & 0xff;
  if (offset == 0) {
	data &= 0x3f;
	data |= ((selocals.flipsolPulse & 0x01) << 7);
	data |= ((selocals.flipsolPulse & 0x04) >> 4);
  }
  return data;
}

// *** Unknown memory mapped ports $200C to $3FFF - read ***
// The CPU #0 ROM code in some Whitestar II games read from a sequence
// of memory locations from $200C to $3FE8 at some game events.  The
// reads don't hit every address in this range and don't follow an
// obvious sequence, but they hit about every 4th-8th byte in a mostly
// ascending order.  The reads occur on specific game events and seem
// to always follow reads on the known solenoid, lamp, and aux lamp
// ports.  My best guess from the grouping with the solenoid/lamp port
// reads is that the unknown locations are related, perhaps reading the
// status of other output devices or the status of individual lamps or
// solenoids.  It's not clear from the schematics that these addresses
// are mapped at all, but the deliberate ROM reads suggest they have
// some purpose.  It doesn't seem to have any ill effect on game play
// to have these locations always read as "0" values.
//
// This explicit handler is mainly for the sake of documentation. 
// These addresses can alternatively be delegated to the default
// "unmapped byte" handler, since that will just return 0 as well, but
// it seemed worth calling these out for future reference, in case
// anyone wants to look into the real purpose of these locations at
// some point.
static READ_HANDLER(unknown_r) {
  return 0;
}

// *** Unknown memory mapped port $3801 - write ***
// Similar to above: The CPU #0 code writes a byte to port $3801.  This
// is rare and seems to happen after end of game.  Port $3800 is the
// sound board command port, so the location suggests this is another
// function on the sound board - maybe a reset or something like that?
// Ignoring it has no obvious bad effect, so this handler is here just
// for the sake of documentation.
static WRITE_HANDLER(sndbrd_unk_data_w) {
}

/*-- DMD communication --*/
static WRITE_HANDLER(dmdlatch_w) {
  sndbrd_0_data_w(0,data);
  sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
}
static WRITE_HANDLER(dmdreset_w) {
  sndbrd_0_ctrl_w(0,data?0x02:0x00);
}

/* Handler called by the Sound System to set the various data needed by the main cpu */
void set_at91_data(int plin, int sst0, int led)
{
	selocals.sst0 = sst0;
	selocals.plin = plin;
	selocals.diagnosticLed &= 2;
	selocals.diagnosticLed |= (led<<1);
}

/*U202 - HC245
  D0 = BUSY   -> SOUND BUSY?
  D1 = SSTO   -> SOUND RELATED
  D2 = MPIN   -> ??
  D3 = CN8-22 -> DMD STAT0
  D4 = CN8-23 -> DMD STAT1
  D5 = CN8-24 -> DMD STAT2
  D6 = CN8-25 -> DMD STAT3
  D7 = CN8-26 -> DMD BUSY
*/
static READ_HANDLER(dmdstatus_r) {
  int data = (sndbrd_0_data_r(0) ? 0x80 : 0x00) | (sndbrd_0_ctrl_r(0)<<3) | (selocals.sst0<<1);
  return data;
}

/* PLIN - Plasma? In Data Latch
   Not connected prior to LOTR Hardare
   U404 - HC245 of 520-5300-00(ATMEL 91)
   Ironically not used for Plasma info, but Sound Info, but probably called that when designed way back
   in the initial Whitestar system design
*/
static READ_HANDLER(dmdie_r) {
	return selocals.plin;
}

/* U203 - HC245 - Read Data from J3 (Aux In) - Pins 1-7 (D0-D6) - D7 from LST (Lamp Strobe?) Line)

   So far, it seems this port is only used when combined with the TOPS system for reading status
   of the DUART found on the TSIB board.
   -------------------------------------
   Bit 1 = Byte received from TSIB DUART
   Bit 6 = DUART Transmit Empty & Ready for Transmission
*/
static READ_HANDLER(auxboard_r) {
	int data = selocals.auxdata;
	// Did D Strobe just go low? If so, TSIB board wants to read DUART status.
	if((selocals.lastgiaux & 0x20) == 0) {
		data = 0x40;						//signal that TOPS Duart is ready to receive
		//printf("%04x: reading aux\n",activecpu_get_previouspc());
	}
	return data;
}

/* U201 - HC273 - Write Data to J2 (Aux Out) - Pins 1-9 (D0-D7) */
static WRITE_HANDLER(auxboard_w) { selocals.auxdata = data; }

/* U206 - HCT273
   Bit 0 -> GI Relay Driver
   Bit 1 -> NC
   Bit 2 -> NC
   Bit 3 -> BSTB -> J3 - 09 (Aux In)
   Bit 4 -> CSTB -> J3 - 10 (Aux In)
   Bit 5 -> DSTB -> J3 - 11 (Aux In)
   Bit 6 -> ESTB -> J3 - 12 (Aux In)
   Bit 7 -> ASTB -> J2 - 10 (Aux Out)
*/
static WRITE_HANDLER(giaux_w) {

  /*     When Tournament Serial Board Interface connected -
		 BSTB is the address data strobe of the TSIB:
		 Aux. Data previously written is as follows:

		 Bit 0 - 2: Address of DUART Registers
		 Bit 4 - 5: Address mapping (see below)
		 Bit 7    : Reset of DUART (active low)

		 Mapping:   Bit
					5 4
					---
		            0 0 = (<0x10) DUART - Channel #1
					0 1 = ( 0x10) DUART - Channel #2
					1 0 = ( 0x20) AUXIN - To J3 Connector of TSBI
					1 1 = ( 0x30) AUXOUT - To J4 Connector of TSBI ( To Mini DMD )
  */

#if 0
  if(GET_BIT4 == 0)
    printf("giaux = %x, (GI=%x A=%x B=%x C=%x D=%x E=%x), aux = %x (%c)\n",data,GET_BIT0,GET_BIT7,GET_BIT3,GET_BIT4,GET_BIT5,GET_BIT6,selocals.auxdata,selocals.auxdata);
#endif
  coreGlobals.gi[0]=(~data & 0x01) ? 9 : 0;
  if (core_gameData->hw.display & (SE_MINIDMD|SE_MINIDMD3)) {
    if (data & ~selocals.lastgiaux & 0x80) { /* clock in data to minidmd */
      selocals.minidata[selocals.miniidx] = selocals.auxdata & 0x7f;
      selocals.miniidx = (selocals.miniidx + 1) % 4;
      if (!(selocals.auxdata & 0x80)) { /* enabled column? */
        int tmp = selocals.minidata[selocals.miniidx] & 0x1f;
        if (tmp) {
          int col = 1;
          while (tmp >>= 1) col += 1;
          selocals.minidmd[selocals.miniframe][0][col-1] = selocals.minidata[(selocals.miniidx + 1) % 4];
          selocals.minidmd[selocals.miniframe][1][col-1] = selocals.minidata[(selocals.miniidx + 2) % 4];
          selocals.minidmd[selocals.miniframe][2][col-1] = selocals.minidata[(selocals.miniidx + 3) % 4];
          // Should find a better way to detect different frames but columns seem
          // to always be updated in order 1-5 so this works
          if (col == 5) selocals.miniframe = (selocals.miniframe + 1) % 3;
        }
      }
	}
    if (core_gameData->hw.display & SE_MINIDMD3) {
      if (data == 0xbe)	coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
	}
	else
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | ((data & 0x38) << 1);
    selocals.lastgiaux = data;
  }
  else if (core_gameData->hw.display & SE_MINIDMD2) {
    if (data & ~selocals.lastgiaux & 0x80) { /* clock in data to minidmd */
      selocals.miniidx = (selocals.miniidx + 1) % 7;
      selocals.minidata[selocals.miniidx] = selocals.auxdata & 0x7f;
      if ((selocals.auxdata & 0x80) == 0) { /* enabled column? */
        int ii, bits;
        for (ii=0, bits = 0x01; ii < 14; ii++, bits <<= 1) {
          if (bits & ((selocals.minidata[3] << 7) | selocals.minidata[4])) {
            selocals.minidmd[0][ii/7][ii%7] = selocals.minidata[0];
            selocals.minidmd[1][ii/7][ii%7] = selocals.minidata[1];
            selocals.minidmd[2][ii/7][ii%7] = selocals.minidata[5];
            selocals.minidmd[3][ii/7][ii%7] = selocals.minidata[6];
            break;
          }
        }
      }
    }
    if (data == 0xbe)
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
    selocals.lastgiaux = data;
  }
  else if (core_gameData->hw.display & SE_LED) { // map LEDs as extra lamp columns
    static int order[] = { 6, 2, 4, 5, 1, 3, 0 };
    if (selocals.auxdata == 0x30 && (selocals.lastgiaux & 0x40)) selocals.miniidx = 0;
    if (data == 0x7e) {
      if (order[selocals.miniidx])
        coreGlobals.tmpLampMatrix[9 + order[selocals.miniidx]] = selocals.auxdata;
      if (selocals.miniidx < 6) selocals.miniidx++;
    } else if (data == 0xbe)
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
    selocals.lastgiaux = selocals.auxdata;
  }
  else if (core_gameData->hw.display & SE_LED2) { // a whole lotta extra lamps...
    if (data & ~selocals.lastgiaux & 0x80) { /* clock in data to minidmd */
      selocals.miniidx = (selocals.miniidx + 1) % 32;
      coreGlobals.tmpLampMatrix[selocals.miniidx+10] = selocals.auxdata;
    }
    selocals.lastgiaux = data;
  } else if (core_gameData->hw.display & SE_DIGIT) {
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (core_revbyte(selocals.auxdata & 0xf0) << 2);
    coreGlobals.segments[0].w = core_bcd2seg7[selocals.auxdata & 0x0f];
  } else
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
}

// MINI DMD Type 1 (HRC) (15x7)
PINMAME_VIDEO_UPDATE(seminidmd1_update) {
  int ii,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &coreGlobals.dotCol[ii+1][0];
    int jj,kk;
    for (jj = 2; jj >= 0; jj--)
      for (kk = 0; kk < 5; kk++) {
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
      }
  }
  for (ii = 0; ii < 15; ii++) {
    int jj;
    bits = 0;
    for (jj = 0; jj < 7; jj++)
      bits = (bits<<2) | coreGlobals.dotCol[jj+1][ii];
    *seg++ = bits;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}
// MINI DMD Type 1 (Ripley's) (3 x 5x7)
PINMAME_VIDEO_UPDATE(seminidmd1s_update) {
  int ii,bits;
  int jj = 2-(layout->left-10)/8;
  UINT16 *seg = &coreGlobals.drawSeg[5*(2-jj)];

  for (ii = 0, bits = 0x40; ii < 7; ii++, bits >>= 1) {
    UINT8 *line = &coreGlobals.dotCol[ii+1][0];
    int kk;
    for (kk = 0; kk < 5; kk++)
      *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                 (selocals.minidmd[2][jj][kk] & bits))/bits;
  }
  for (ii = 0; ii < 5; ii++) {
    int kk;
    bits = 0;
    for (kk = 0; kk < 7; kk++)
      bits = (bits<<2) | coreGlobals.dotCol[kk+1][ii];
    *seg++ = bits;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}
// MINI DMD Type 2 (Monopoly) (15x7)
PINMAME_VIDEO_UPDATE(seminidmd2_update) {
  int ii,bits;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii = 0, bits = 0x01; ii < 7; ii++, bits <<= 1) {
    UINT8 *line = &coreGlobals.dotCol[ii+1][0];
    int jj,kk;
    for (jj = 0; jj < 3; jj++)
      for (kk = 4; kk >= 0; kk--)
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
  }
  for (ii = 0; ii < 15; ii++) {
    int jj;
    bits = 0;
    for (jj = 0; jj < 7; jj++)
      bits = (bits<<2) | coreGlobals.dotCol[jj+1][ii];
    *seg++ = bits;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}
// MINI DMD Type 3 (RCT) (21x5)
PINMAME_VIDEO_UPDATE(seminidmd3_update) {
  int ii,kk;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  memset(coreGlobals.dotCol,0,sizeof(coreGlobals.dotCol));
  for (kk = 0; kk < 5; kk++) {
    UINT8 *line = &coreGlobals.dotCol[kk+1][0];
    int jj,bits;
    for (jj = 0; jj < 3; jj++)
      for (ii = 0, bits = 0x01; ii < 7; ii++, bits <<= 1)
        *line++ = ((selocals.minidmd[0][jj][kk] & bits) + (selocals.minidmd[1][jj][kk] & bits) +
                   (selocals.minidmd[2][jj][kk] & bits))/bits;
  }
  for (ii = 0; ii < 21; ii++) {
    int bits = 0;
    int jj;
    for (jj = 0; jj < 5; jj++)
      bits = (bits<<2) | coreGlobals.dotCol[jj+1][ii];
    *seg++ = bits;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}
// 3-Color MINI DMD Type 4 (Simpsons) (14x10)
PINMAME_VIDEO_UPDATE(seminidmd4_update) {
  static int color[2][2] = {
    { 0, 6 }, { 7, 9 } // off, green, red, yellow
  };
  int ii;
  UINT8 *line;
  UINT16 *seg = &coreGlobals.drawSeg[0];

  for (ii=0; ii < 14; ii++) {
    UINT16 bits1 = 0;
    UINT16 bits2 = 0;
    int kk, bits;
    for (kk=0, bits = 0x40; kk < 7; kk++, bits >>= 1) {
      int isRed = (selocals.minidmd[0][ii/7][ii%7] & bits) > 0;
      int isGrn = (selocals.minidmd[1][ii/7][ii%7] & bits) > 0;
      bits1 = (bits1 << 2) | (isGrn << 1) | isRed;
      line = &coreGlobals.dotCol[ii+1][kk];
      *line = color[isRed][isGrn];
      isRed = (selocals.minidmd[2][ii/7][ii%7] & bits) > 0;
      isGrn = (selocals.minidmd[3][ii/7][ii%7] & bits) > 0;
      bits2 = (bits2 << 2) | (isGrn << 1) | isRed;
      line = &coreGlobals.dotCol[ii+1][7+kk];
      *line = color[isRed][isGrn];
    }
    *seg++ = bits1;
    *seg++ = bits2;
  }
  if (!pmoptions.dmd_only)
    video_update_core_dmd(bitmap, cliprect, layout);
  return 0;
}

/*---------------------------
/  Memory map for main CPU
/----------------------------*/

static MEMORY_READ_START(se_readmem)
  { 0x0000, 0x1fff, ram_r },
  { 0x2000, 0x2003, solenoid_r },
  { 0x2007, 0x2007, auxboard_r },
  { 0x2008, 0x2008, lampstrb_r },
  { 0x2009, 0x2009, auxlamp_r },
  { 0x3000, 0x3000, dedswitch_r },
  { 0x3100, 0x3100, dip_r },
  { 0x3400, 0x3400, switch_r },
  { 0x3406, 0x3407, gilamp_r }, // GI lamps on SPP?
  { 0x3500, 0x3500, dmdie_r },
  { 0x3700, 0x3700, dmdstatus_r },
  { 0x200c, 0x3fff, unknown_r }, // unknown ports from $200c to $3fff, excluding known ports mapped above
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(se_writemem)
  { 0x0000, 0x1fff, ram_w },
  { 0x2000, 0x2003, solenoid_w },
  { 0x2006, 0x2007, auxboard_w },
  { 0x2008, 0x2008, lampstrb_w },
  { 0x2009, 0x2009, auxlamp_w },
  { 0x200a, 0x200a, lampdriv_w },
  { 0x200b, 0x200b, giaux_w },
//{ 0x201a, 0x201a, x201a_w },    // Golden Cue unknown device
  { 0x3200, 0x3200, mcpu_bank_w },
  { 0x3300, 0x3300, switch_w },
  { 0x3406, 0x3407, gilamp_w }, // GI lamps on SPP?
  { 0x3600, 0x3600, dmdlatch_w },
  { 0x3601, 0x3601, dmdreset_w },
  { 0x3800, 0x3800, sndbrd_1_data_w },
  { 0x3801, 0x3801, sndbrd_unk_data_w },
  { 0x4000, 0xffff, MWA_NOP },
MEMORY_END

static MACHINE_DRIVER_START(se)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M6809, 2000000) // MC6809E
  MDRV_CORE_INIT_RESET_STOP(se,NULL,se)
  MDRV_CPU_MEMORY(se_readmem, se_writemem)
  MDRV_CPU_VBLANK_INT(se_vblank, 1)
  MDRV_CPU_PERIODIC_INT(irq1_line_pulse, SE_FIRQFREQ)
  MDRV_NVRAM_HANDLER(se)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(se)
  MDRV_DIAGNOSTIC_LEDH(1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2aS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2as)
  MDRV_IMPORT_FROM(se_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2tS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2ts)
  MDRV_IMPORT_FROM(se_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se2bS)
  MDRV_IMPORT_FROM(se)
  MDRV_IMPORT_FROM(de2bs)
  MDRV_IMPORT_FROM(se_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

MACHINE_DRIVER_START(se3aS)
//from se driver - but I need a different init routine
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CORE_INIT_RESET_STOP(se3,NULL,se)
  MDRV_CPU_MEMORY(se_readmem, se_writemem)
  MDRV_CPU_VBLANK_INT(se_vblank, 1)
  MDRV_CPU_PERIODIC_INT(irq1_line_pulse, SE_FIRQFREQ)
  MDRV_NVRAM_HANDLER(se)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(se)
  MDRV_DIAGNOSTIC_LEDH(2)
//
  MDRV_IMPORT_FROM(de3as)
  MDRV_IMPORT_FROM(se_dmd32)
  MDRV_SOUND_CMD(sndbrd_1_data_w)
  MDRV_SOUND_CMDHEADING("se")
MACHINE_DRIVER_END

/*-----------------------------------------------
/ Load/Save static ram
/ Save RAM & CMOS Information
/-------------------------------------------------*/
static NVRAM_HANDLER(se) {
  core_nvram(file, read_or_write, memory_region(SE_CPUREGION), 0x2000, 0xff);
}

// convert switch numbers
#ifdef PROC_SUPPORT
int se_m2sw(int col, int row) { return col*8+(7-row)+1; }
#endif

//Stern S.A.M. Hardware support
#ifndef SAM_ORIGINAL
#include "sam.c" // vastly improved variant of the leaked/hacked version
#else
#include "sam_original.c" // original internal version (for reference/deprecated)
#endif
