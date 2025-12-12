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

TOPS feature: mainly time keeping upgrade chip, see https://drive.google.com/drive/folders/1yAyZNsNdm2fuIsHeWo1hxn0S0IWW-19d
              and https://github.com/mamedev/mame/blob/master/src/devices/machine/timekpr.h

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
#include "bulb.h"
#ifdef PROC_SUPPORT
#include "p-roc/p-roc.h"
#endif

#define SE_FIRQFREQ        976 /* FIRQ Frequency according to Theory of Operation */
#define SE_ROMBANK0        1

#ifdef PROC_SUPPORT
// TODO/PROC: Make variables out of these defines. Values depend on "-proc" switch.
#define VBLANK             1
#define SE_SOLSMOOTH       1 /* Don't smooth values on real hardware */
#define SE_LAMPSMOOTH      1
#define SE_DISPLAYSMOOTH   1
#else
#define VBLANK             2 // (at least) LOTR requires some kind of oversampling of the lamp state, otherwise the flickering lamps during modes are always on!
#define SE_SOLSMOOTH       4 /* Smooth the Solenoids over this number of VBLANKS */
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
static struct {
  int    vblankCount;
  int    initDone;
  UINT32 solenoids;
  UINT8  lampRow;
  UINT16 lampColumn;
  int    diagnosticLed;
  int    swCol;
  int    flipsol, flipsolPulse;
  int    sst0;			//SST0 bit from sound section
  int    plin;			//Plasma In (not connected prior to LOTR Hardware)
  UINT8 *ram8000;
  UINT8  auxdata;   // Data latched on J2
  UINT8  lastgiaux; // Last data latched on J3 (to detect strobe edges)
  // Misc extension boards
  int    titanicBankLatch;
  core_tWord lotrLedLatch;
  // Mini DMD extension boards
  int    miniDMDcol, prevMiniDMDCol;
  UINT8  miniDMDLatches[6];
  UINT8  miniDMD21x5[5][3];   // RCT
  UINT16 miniDMD15x7[7];      // HRC, Monopoly
  UINT8  miniDMD5x7[3][7];    // Ripley's (3 displays of 5x7 dots)
  UINT16 miniDMD14x10[2][10]; // Simpsons (14x10 bicolor displays, 1 red and 1 green)
  /* trace ram related */
#if SUPPORT_TRACERAM
  UINT8 *traceRam;
#endif
  UINT8  curBank;                   /* current bank select */
  #define TRACERAM_SELECTED 0x10    /* this bit set maps trace ram to 0x0000-0x1FFF */
  int fastflipaddr;

  UINT8 lampstate[80];
} selocals;

#ifdef PROC_SUPPORT
static int switches_retrieved=0;
#endif

/*-------------------------
/  Generate IRQ interrupt
/--------------------------*/
static INTERRUPT_GEN(se_irq) {
   irq1_line_pulse();
}

static INTERRUPT_GEN(se_vblank) {
  /*-------------------------------
  /  copy local data to interface
  /--------------------------------*/
  selocals.vblankCount++;

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
	memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
#else
    int i;
    for (i = 0; i < 10; ++i) {
        int i2;
        for (i2 = 0; i2 < 8; ++i2)
            selocals.lampstate[i * 8 + i2] += (coreGlobals.tmpLampMatrix[i] >> i2) & 1;
    }
    if ((selocals.vblankCount % (VBLANK*SE_LAMPSMOOTH)) == 0)
    {
        memcpy((void*)(coreGlobals.lampMatrix+10), (void*)(coreGlobals.tmpLampMatrix+10), sizeof(coreGlobals.tmpLampMatrix)-10);
        memset((void*)coreGlobals.lampMatrix, 0, 10);
        for (i = 0; i < 10; ++i) {
            int i2;
            for (i2 = 0; i2 < 8; ++i2)
                coreGlobals.lampMatrix[i] |= selocals.lampstate[i * 8 + i2] > 1 ? (1 << i2) : 0; //!! > 1 related to the magic of VBLANK and SE_LAMPSMOOTH, so that LOTR lamps blink
        }
        memset(selocals.lampstate, 0, sizeof(selocals.lampstate));
    }
#endif
    memset((void*)coreGlobals.tmpLampMatrix, 0, 10);
  }
  /*-- solenoids --*/
  if ((selocals.vblankCount % VBLANK) == 0) {
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xfff0) | selocals.flipsol;
    selocals.flipsol = selocals.flipsolPulse;
  }
  if ((selocals.vblankCount % (VBLANK*SE_SOLSMOOTH)) == 0) {
    coreGlobals.solenoids = selocals.solenoids;
    // Fast flips.   Use Solenoid 15, this is the left flipper solenoid that is
    // unused because it is remapped to VPM flipper constants.
    if (selocals.fastflipaddr > 0 && memory_region(SE_CPUREGION)[selocals.fastflipaddr - 1] > 0) {
       coreGlobals.solenoids |= 0x4000;
       core_write_pwm_output(CORE_MODOUT_SOL0 + 15 - 1, 1, 1);
    }
    else
    {
       core_write_pwm_output(CORE_MODOUT_SOL0 + 15 - 1, 1, 0);
    }
	 selocals.solenoids = coreGlobals.pulsedSolState;
#ifdef PROC_SUPPORT
		if (coreGlobals.p_rocEn) {
			static UINT64 lastSol = 0;
			UINT64 allSol = core_getAllSol();
			if (coreGlobals.p_rocEn) {
				int ii;
				UINT64 chgSol = (allSol ^ lastSol) & 0xffffffffffffffff; //vp_getSolMask64();
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
			lastSol = allSol;
		}
#endif
  }
#ifdef PROC_SUPPORT
	if (coreGlobals.p_rocEn) {
		procCheckActiveCoils();
	}
#endif
  /*-- display --*/
  if ((selocals.vblankCount % (VBLANK*SE_DISPLAYSMOOTH)) == 0) {
    coreGlobals.diagnosticLed = selocals.diagnosticLed;
    selocals.diagnosticLed = 0;
  }
  if ((selocals.vblankCount % VBLANK) == 0)
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
   if (core_gameData->hw.display & SE_BOARDID_TITANIC) {
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
	memset(&selocals, 0, sizeof(selocals));
	//const char * const gn = Machine->gamedrv->name;
	sndbrd_0_init(SNDBRD_DEDMD32, 2, memory_region(DE_DMD32ROMREGION),NULL,NULL);
	sndbrd_1_init(SNDBRD_DE3S,    1, memory_region(DE2S_ROMREGION), NULL, NULL);

	// Fast flips support.   My process for finding these, is to load them in pinmame32 in VC debugger.  
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

   // Initialize outputs
   coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
   core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_BULB_44_18V_DC_SE);
   coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
   core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
   core_set_pwm_output_type(CORE_MODOUT_SOL0 + 15 - 1, 2, CORE_MODOUT_PULSE); // Fake solenoids for fast flip
   coreGlobals.nGI = 1;
   core_set_pwm_output_type(CORE_MODOUT_GI0, coreGlobals.nGI, CORE_MODOUT_BULB_44_5_7V_AC);
   const struct GameDriver* rootDrv = Machine->gamedrv;
   while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
      rootDrv = rootDrv->clone_of;
   const char* const grn = rootDrv->name;
   if (strncasecmp(grn, "elvis", 5) == 0) { // Elvis
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
   }
   else if (strncasecmp(grn, "gprix", 5) == 0) { // Grand Prix
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
   }
   else if (strncasecmp(grn, "lotr", 4) == 0) { // The Lord of The Rings
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 3, CORE_MODOUT_BULB_906_20V_DC_WPC);
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 29 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
      core_set_pwm_output_led_vfd(CORE_MODOUT_LAMP0 + 80, 3 * 8, CORE_MODOUT_LED, 4.f / 2.f); // 2ms strobing over a 4ms period
   }
   else if ((strncasecmp(grn, "nascar", 6) == 0) || (strncasecmp(grn, "dalejr", 6) == 0)) { // Nascar & Dale Jr. (limited edition of Nascar)
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
   }
   else if (strncasecmp(grn, "ripleys", 7) == 0) { // Ripley's
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 1, CORE_MODOUT_PULSE); // Idol Opto LED
      core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
      core_dmd_pwm_init(&core_gameData->lcdLayout[1], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0);
      core_dmd_pwm_init(&core_gameData->lcdLayout[2], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0);
      core_dmd_pwm_init(&core_gameData->lcdLayout[3], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0);
   }
   else if (strncasecmp(grn, "sopranos", 8) == 0) { // Sopranos
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_LED);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
   }
}

static MACHINE_INIT(se) {
  const char * const gn = Machine->gamedrv->name;
  memset(&selocals, 0, sizeof(selocals));
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
  else if (strncasecmp(gn, "startrp", 7) == 0)
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
#if SUPPORT_TRACERAM
  selocals.traceRam = 0;
  if (core_gameData->gen & GEN_WS_1) {
    selocals.traceRam = malloc(0x2000);
    if (selocals.traceRam) memset(selocals.traceRam,0,0x2000);
  }
#endif

  // Initialize outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_BULB_44_18V_DC_SE);
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol;
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 15 - 1, 2, CORE_MODOUT_PULSE); // Fake solenoids for fast flip
  coreGlobals.nGI = 1;
  core_set_pwm_output_type(CORE_MODOUT_GI0, coreGlobals.nGI, CORE_MODOUT_BULB_44_5_7V_AC);
  // Game specific hardware
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
     rootDrv = rootDrv->clone_of;
  const char* const grn = rootDrv->name;
  // Missing definition:
  // - Golden Cue
  if (strncasecmp(grn, "apollo13", 8) == 0) { // Apollo 13
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "austin", 6) == 0) { // Austin Powers
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 5 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 12 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "godzilla", 8) == 0) { // Godzilla
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "gldneye", 7) == 0) { // GoldenEye
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "id4", 3) == 0) { // Independence Day
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 6 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "harl_", 5) == 0) { // Harley Davidson
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "hirolcas", 8) == 0) { // High Roller Casino
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 8 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 30 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_dmd_pwm_init(&core_gameData->lcdLayout[1], CORE_DMD_PWM_FILTER_WPC, CORE_DMD_PWM_COMBINER_1, 0);
  }
  else if (strncasecmp(grn, "jplstw22", 8) == 0) { // Jurassic Park, Lost World
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "lostspc", 7) == 0) { // Lost in Space
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "monopoly", 8) == 0) { // Monopoly
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 29 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_dmd_pwm_init(&core_gameData->lcdLayout[1], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0);
  }
  else if (strncasecmp(grn, "playboys", 8) == 0) { // Playboy
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "rctycn", 6) == 0) { // RollerCoaster Tycoon
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 27 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 29 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_dmd_pwm_init(&core_gameData->lcdLayout[1], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0);
  }
  else if (strncasecmp(grn, "simpprty", 8) == 0) { // Simpsons Pinball Party
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 31 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 73 - 1, 8, CORE_MODOUT_LED_STROBE_1_10MS); // Green LEDs
     core_dmd_pwm_init(&core_gameData->lcdLayout[1], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0); // Green
     core_dmd_pwm_init(&core_gameData->lcdLayout[2], CORE_DMD_PWM_FILTER_WPC_PH, CORE_DMD_PWM_COMBINER_1, 0); // Red
  }
  else if (strncasecmp(grn, "spacejam", 8) == 0) { // Space Jam
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "shrkysht", 8) == 0) { // Sharkeys Shootout
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 32 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "sprk_", 5) == 0) { // South Park
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 7 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "startrp", 7) == 0) { // Starship Troopers
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 49 - 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 LED segments
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 57 - 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 LED segments
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 65 - 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 LED segments
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 73 - 1, 7, CORE_MODOUT_LED_STROBE_1_10MS); // 7 LED segments
  }
  else if (strncasecmp(grn, "swtril", 6) == 0) { // Star Wars Trilogy
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 27 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if ((strncasecmp(grn, "nfl", 3) == 0) // NFL
        || (strncasecmp(grn, "strikext", 8) == 0)) { // Striker Extreme
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 27 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "term3", 5) == 0) { // Terminator 3
     core_set_pwm_output_bulb(CORE_MODOUT_SOL0 + 4 - 1, 1, BULB_44, (float)(19. - 0.7), TRUE, 0.f, 1.f); // Backbox GI: 19V AC switched #44 Bulbs (Sol 1-16 uses Mosfets with low voltage drop)
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "titanic", 5) == 0) { // Titanic (redemption game)
     core_set_pwm_output_led_vfd(CORE_MODOUT_LAMP0 + 80, 32 * 8, CORE_MODOUT_LED, 1.f); // PWM is not implemented for the time being, so just latched at 100%
  }
  else if (strncasecmp(grn, "twst_", 5) == 0) { // Twister
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "viprsega", 8) == 0) { // Viper Night Driving
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 32 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(grn, "xfiles", 6) == 0) { // X-Files
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
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
  //core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn, selocals.lampRow);
  core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0     ,  selocals.lampColumn       & 0x00FF, selocals.lampRow, 8);
  core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (selocals.lampColumn >> 8) & 0x00FF, selocals.lampRow, 2);
}
static READ_HANDLER(lampdriv_r) { return core_revbyte(selocals.lampRow); } // GoldenEye reads from here (while it is a write location), so wire it up
static WRITE_HANDLER(lampstrb_w) {
  //core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0xff00) | data, selocals.lampRow);
  selocals.lampColumn = (selocals.lampColumn & 0xff00) | data;
  core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0,  selocals.lampColumn & 0x00FF, selocals.lampRow, 8);
}
static READ_HANDLER(lampstrb_r) { return selocals.lampColumn & 0xff; }
static WRITE_HANDLER(auxlamp_w) {
  //core_setLamp(coreGlobals.tmpLampMatrix, selocals.lampColumn = (selocals.lampColumn & 0x00ff) | (data<<8), selocals.lampRow);
  selocals.lampColumn = (selocals.lampColumn & 0x00ff) | (data<<8);
  core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, (selocals.lampColumn >> 8) & 0x00FF, selocals.lampRow, 2);
}
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
WRITE_HANDLER(se_solenoid_w) {
  UINT32 mask = ~(0xff<<solmaskno[offset]);
  UINT32 sols = data<<solmaskno[offset];
  if (offset == 0) { /* move flipper power solenoids (L=15,R=16) to (R=45,L=47) */
    selocals.flipsol |= selocals.flipsolPulse = ((data & 0x80)>>7) | ((data & 0x40)>>4);
    sols &= 0xffff3fff; /* mask off flipper solenoids */
    core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + solmaskno[offset], data, 0x3F);
  }
  else
  {
    core_write_pwm_output_8b(CORE_MODOUT_SOL0 + solmaskno[offset], data);
  }
  coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & mask) | sols;
  selocals.solenoids |= sols;
}
// Some Whitestar II ROMS read from the solenoid ports
static READ_HANDLER(solenoid_r) {
  data8_t data = (coreGlobals.pulsedSolState >> solmaskno[offset]) & 0xff;
  if (offset == 0) {
    data &= 0x3f;
    data |= ((selocals.flipsolPulse & 0x01) << 7); // Bit 16
    data |= ((selocals.flipsolPulse & 0x04) << 4); // Bit 15
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
  data8_t data = (sndbrd_0_data_r(0) ? 0x80 : 0x00) | (sndbrd_0_ctrl_r(0)<<3) | (selocals.sst0<<1);
  return data;
}

/* PLIN - Plasma? In Data Latch
   Not connected prior to LOTR Hardware
   U404 - HC245 of 520-5300-00(ATMEL 91)
   Ironically not used for Plasma info, but Sound Info, but probably called that when designed way back
   in the initial Whitestar system design
*/
static READ_HANDLER(dmdie_r) {
	return selocals.plin;
}

/* U203 - HC245 - Read Data from J3 (Aux In) - Pins 1-7 (D0-D6) - D7 from LST (Lamp Strobe?) Line

   So far, it seems this port is only used when combined with the TOPS system for reading status
   of the DUART found on the TSIB board.
   -------------------------------------
   Bit 1 = Byte received from TSIB DUART
   Bit 6 = DUART Transmit Empty & Ready for Transmission
*/
static READ_HANDLER(auxboard_r) {
	data8_t data = selocals.auxdata;
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

   /*   When Tournament Serial Board Interface connected -
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
   if (GET_BIT4 == 0)
      printf("giaux = %x, (GI=%x A=%x B=%x C=%x D=%x E=%x), aux = %x (%c)\n", data, GET_BIT0, GET_BIT7, GET_BIT3, GET_BIT4, GET_BIT5, GET_BIT6, selocals.auxdata, selocals.auxdata);
#endif

   // GI relay
   coreGlobals.gi[0] = (~data & 0x01) ? 9 : 0;
   core_write_pwm_output_8b(CORE_MODOUT_GI0, ~data & 0x01);

   // Board 520-5068-01: auxiliary driver board with 3 latched solenoid outputs
   if ((core_gameData->hw.display & SE_BOARDID_520_5068_01) && (core_lowToHigh(selocals.lastgiaux, data, 0x40))) { // ESTB: auxiliary solenoid driver board (3 outputs)
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
      core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, selocals.auxdata, 0x07); // Solenoids 33..35
   }

   // Board 520-5192-00: Solenoid Expander board (3 outputs, not latched)
   if (core_gameData->hw.display & SE_BOARDID_520_5192_00) {
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | ((data & 0x38) << 1);
      core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, (data & 0x38) >> 3, 0x07); // Solenoids 33..35
   }

   // Board 520-5152-00 (ID4) or 520-5078-00 (Tommy) servo controller board
   if (core_gameData->hw.display & SE_BOARDID_520_5152_00) {
      // Board 520-5152-00 directly process the 1 bit data received (no latching, no other informations).
      // Board 520-5078-00 uses 3 signals: ASTB to latch the position data bit, and another (unlatched) bit to reset the device.
      // Alien head is controlled by servo board 520-5152-00 (1 bit to toggle between 2 positions) but CPU also 
      // sends data to allow using Tommy's Blinder servo board 520-5078-00 as a spare.
      // Sol 34 can be used to identify position 2 versus position 1.
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | ((selocals.auxdata & 0x03) << 4);
      core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, selocals.auxdata, 0x0F); // Solenoids 33..36
   }

   // Board 520-5242-00: Lord of The Rings 19 LED Board
   if (core_gameData->hw.display & SE_BOARDID_520_5242_00) {
      // The board has 2 x 74ACT574 latches which are driven by opposite edges of ASTB to power a bunch of LEDs
      UINT16 prev = selocals.lotrLedLatch.w;
      if (core_highToLow(selocals.lastgiaux, data, 0x80)) // ASTB negative edge clock first latch (8 LEDs)
         selocals.lotrLedLatch.b.lo = selocals.auxdata;
      else if (core_lowToHigh(selocals.lastgiaux, data, 0x80)) // ASTB positive edge clock second latch (3 LEDs and row enable)
         selocals.lotrLedLatch.b.hi = selocals.auxdata;
      if (prev != selocals.lotrLedLatch.w) {
         if ((selocals.lotrLedLatch.w & 0xC000) == 0x4000) {
            coreGlobals.tmpLampMatrix[10] = selocals.lotrLedLatch.b.lo;
            coreGlobals.tmpLampMatrix[12] = selocals.lotrLedLatch.b.hi & 0x07;
         }
         else if ((selocals.lotrLedLatch.w & 0xC000) == 0x8000) {
            coreGlobals.tmpLampMatrix[11] = selocals.lotrLedLatch.b.lo;
         }
         // Full power is 2ms over 4ms (50% duty cycle)
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 80, (selocals.lotrLedLatch.w & 0x4000) ? selocals.lotrLedLatch.b.lo : 0);
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 88, (selocals.lotrLedLatch.w & 0x8000) ? selocals.lotrLedLatch.b.lo : 0);
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 96, (selocals.lotrLedLatch.w & 0x4000) ? (selocals.lotrLedLatch.b.hi & 0x07) : 0);
      }
   }

   // Board 520-xxxx-xx: Titanic (coin dropper) with a whole lotta extra lamps...
   if (core_gameData->hw.display & SE_BOARDID_TITANIC) {
      if (core_lowToHigh(selocals.lastgiaux, data, 0x08)) { // BSTB select bank
         // Map back to initial implementation bank ordering
         if (selocals.auxdata <= 0x17)
            selocals.titanicBankLatch = selocals.auxdata - 0x08; // 0x08..0x17 -> bank 0..15
         else if (selocals.auxdata <= 0x27)
            selocals.titanicBankLatch = selocals.auxdata - 0x10; // 0x20..0x27 -> bank 16..23
         else
            selocals.titanicBankLatch = selocals.auxdata - 0x28; // 0x40..0x27 -> bank 24..31
      }
      if (core_lowToHigh(selocals.lastgiaux, data, 0x80)) { // ASTB loads bank
         coreGlobals.tmpLampMatrix[10 + selocals.titanicBankLatch] = selocals.auxdata;
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 80 + selocals.titanicBankLatch * 8, selocals.auxdata);
      }
   }

   // Board 520-5130-06: Appollo 13, one 7 segment display and a modulated magnet driver (2 bits Hold/On to define between 3 operation modes)
   if (core_gameData->hw.display & SE_BOARDID_520_5130_06) {
      if (core_lowToHigh(selocals.lastgiaux, data, 0x40)) { // ESTB: latch clear
         coreGlobals.segments[0].w = core_bcd2seg7[0];
         coreGlobals.solenoids2 = coreGlobals.solenoids2 & 0xff0f;
         core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, 0x00, 0x03); // Solenoids 33..34: magnet mode
      }
      if (core_lowToHigh(selocals.lastgiaux, data, 0x20)) { // DSTB: latch load
         coreGlobals.segments[0].w = core_bcd2seg7[selocals.auxdata & 0x0f];
         coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (core_revbyte(selocals.auxdata & 0xf0) << 2); // D4/D5 are magnet mode bits, exposed on solenoids 33 (D5) / 34 (D4)
         core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, core_revbyte(selocals.auxdata & 0xf0) >> 2, 0x03); // Solenoids 33..34: magnet mode
      }
   }

   // Board 520-5143-00: GoldenEye & Twister double magnet processor (drive magnet but also perform ball detection and report it on the switch matrix)
   if (core_gameData->hw.display & SE_BOARDID_520_5143_00) {
      coreGlobals.solenoids2 = (coreGlobals.solenoids2 & 0xff0f) | (selocals.auxdata << 4);
      core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, selocals.auxdata, 0x03); // Solenoids 33..34: magnet 1 & 2 enable states
      core_write_pwm_output(CORE_MODOUT_SOL0 + 35 - 1, 1, data >> 7); // Solenoids 35: ASTB (reset processor)
   }

   // High Roller Casino, RollerCoaster Tycoon, Ripley's & Monopoly Mini DMDs
   if ((core_gameData->hw.display & (SE_BOARDID_520_5197_00 | SE_BOARDID_520_5221_00 | SE_BOARDID_520_5236_00)) && (core_lowToHigh(selocals.lastgiaux, data, 0x80))) { // ASTB clock in data to minidmd
      // clock in data to minidmd (4 cascaded latches used like a large shift register)
      selocals.miniDMDLatches[3] = selocals.miniDMDLatches[2];
      selocals.miniDMDLatches[2] = selocals.miniDMDLatches[1];
      selocals.miniDMDLatches[1] = selocals.miniDMDLatches[0];
      selocals.miniDMDLatches[0] = selocals.auxdata;
      const int blank = (selocals.auxdata & 0x80) || (~data & 0x80);
      if (blank == 0) {
         // HRC: 2ms strobe per column => 10ms per frame of 5 columns, column order is 01/02/04/08/10
         // RCT: 4ms strobe per column => 20ms per frame of 5 columns, column order is 10/08/04/02/01
         // Monopoly: 4ms strobe per column => 20ms per frame of 5 columns, column order is 01/02/04/08/10
         // Ripley's: 4ms strobe per column => 20ms per frame of 5 columns, column order is 01/02/04/08/10
         // Note that during startup, the CPU suspends the rasterization which causes a little glitch as the core implementation expects a continuous frame stream (noticeable on Ripley's)
         selocals.miniDMDcol = core_BitColToNum(selocals.miniDMDLatches[3] & 0x1f);
         if (selocals.miniDMDcol < 5 && selocals.prevMiniDMDCol != selocals.miniDMDcol) {
            selocals.miniDMD21x5[selocals.miniDMDcol][0] = selocals.miniDMDLatches[0];
            selocals.miniDMD21x5[selocals.miniDMDcol][1] = selocals.miniDMDLatches[1];
            selocals.miniDMD21x5[selocals.miniDMDcol][2] = selocals.miniDMDLatches[2];
            // Store frame when we reach the last line (note that depending on the gamecode rasterization is either done 0x01..0x10 or 0x10..0x01)
            if ((selocals.prevMiniDMDCol == 3 && selocals.miniDMDcol == 4) || (selocals.prevMiniDMDCol == 1 && selocals.miniDMDcol == 0)) {
               const core_tLCDLayout* layout = &core_gameData->lcdLayout[1];

               // Board 520-5221-00: RCT is 21x5 so we just have to remove the 8th bits and pack these
               if (core_gameData->hw.display & SE_BOARDID_520_5221_00) {
                  for (int r = 0; r < 5; r++)
                  {
                     selocals.miniDMD21x5[r][0] = (selocals.miniDMD21x5[r][0] << 1) | ((selocals.miniDMD21x5[r][1] >> 6) & 0x01);
                     selocals.miniDMD21x5[r][1] = (selocals.miniDMD21x5[r][1] << 2) | ((selocals.miniDMD21x5[r][2] >> 5) & 0x03);
                     selocals.miniDMD21x5[r][2] = (selocals.miniDMD21x5[r][2] << 3);
                  }
                  core_dmd_submit_frame(layout, &selocals.miniDMD21x5[0][0], 1);
                  // Output mini DMD as LED segments (backward compatibility, but not exactly the same as it used to be a 2 bit value corresponding to the sum of the last 3 frames, independently of the refresh rate)
                  UINT16* seg = coreGlobals.drawSeg;
                  for (int ii = 0; ii < 21; ii++) {
                     UINT16 bits = 0;
                     const int c = (20 - ii) / 8, shift = 7 - ((20 - ii) & 7);
                     for (int jj = 0; jj < 5; jj++)
                        bits = (bits << 2) | (((selocals.miniDMD21x5[jj][c] >> shift) & 0x01) ? 3 : 0);
                     *seg++ = bits;
                  }
               }
               // Board 520-5236-00: Ripley's has 3 5x7 independent mini DMDs, so they need to be rotated and submitted separately
               else if (core_gameData->hw.display & SE_BOARDID_520_5236_00) {
                  for (int l = 0; l < 3; l++) {
                     layout = &core_gameData->lcdLayout[1 + 2 - l];
                     for (int k = 0; k < 7; k++) {
                        for (int j = 0; j < 5; j++)
                           selocals.miniDMD5x7[l][k] = (selocals.miniDMD5x7[l][k] << 1) | ((selocals.miniDMD21x5[j][2 - l] >> (6 - k)) & 0x01);
                        selocals.miniDMD5x7[l][k] <<= 3;
                     }
                     core_dmd_submit_frame(layout, selocals.miniDMD5x7[l], 1);
                     // Output mini DMD as LED segments (backward compatibility, but not exactly the same as it used to be a 2 bit value corresponding to the sum of the last 3 frames, independently of the refresh rate)
                     UINT16* seg = &coreGlobals.drawSeg[5 * (2 - l)];
                     for (int ii = 0; ii < 5; ii++) {
                        UINT16 bits = 0;
                        for (int jj = 0; jj < 7; jj++)
                           bits = (bits << 2) | (((selocals.miniDMD5x7[l][jj] >> (7 - ii)) & 0x01) ? 3 : 0);
                        *seg++ = bits;
                     }
                  }
               }
               // Board 520-5197-00: Monopoly & High Roller Casino. These are 15x7 so they need to be rotated (they also have different orientations)
               // High Roller Casino rasterize at 100Hz in order to create a 2 frame PWM pattern allowing 0/50/100% brightness levels
               // Monopoly rasterizes at 50Hz only (no PWM) for a simple monochrome display
               else if (core_gameData->hw.display & SE_BOARDID_520_5197_00) {
                  for (int k = 0; k < 7; k++) {
                     if (Machine->gamedrv->name[0] == 'm') // Monopoly (somewhat hacky => use a hardware flag)
                        for (int l = 0; l < 3; l++)
                           for (int j = 0; j < 5; j++)
                              selocals.miniDMD15x7[k] = (selocals.miniDMD15x7[k] << 1) | ((selocals.miniDMD21x5[4 - j][2 - l] >> k) & 0x01);
                     else // HRC
                        for (int l = 0; l < 3; l++)
                           for (int j = 0; j < 5; j++)
                              selocals.miniDMD15x7[k] = (selocals.miniDMD15x7[k] << 1) | ((selocals.miniDMD21x5[j][l] >> (6 - k)) & 0x01);
                     selocals.miniDMD15x7[k] <<= 1;
                     selocals.miniDMD15x7[k] = (selocals.miniDMD15x7[k] >> 8) | (selocals.miniDMD15x7[k] << 8);
                  }
                  core_dmd_submit_frame(layout, (UINT8*)selocals.miniDMD15x7, 1);
                  // Output mini DMD as LED segments (backward compatibility, but not exactly the same as it used to be a 2 bit value corresponding to the sum of the last 3 frames, independently of the refresh rate)
                  UINT16* seg = coreGlobals.drawSeg;
                  for (int ii = 0; ii < 15; ii++) {
                     UINT16 bits = 0;
                     for (int jj = 0; jj < 7; jj++) {
                        UINT16 v = (selocals.miniDMD15x7[jj] >> 8) | (selocals.miniDMD15x7[jj] << 8);
                        // FIXME output HRC as 4 shades of brightness
                        bits = (bits << 2) | (((v >> (15 - ii)) & 0x01) ? 3 : 0);
                     }
                     *seg++ = bits;
                  }
               }
               // static double lastFrameTime = 0.0; printf("MiniDMD FPS: %8.5f\n", 1.0 / (timer_get_time() - lastFrameTime)); lastFrameTime = timer_get_time();
               selocals.prevMiniDMDCol = selocals.miniDMDcol;
            }
         }
      }
   }

   // Board 520-5219-00: The Simpsons Pinball Party mini DMD
   if ((core_gameData->hw.display & SE_BOARDID_520_5219_00) && (core_lowToHigh(selocals.lastgiaux, data, 0x80))) { // ASTB: clock in data to minidmd
      // clock in data to minidmd (6 cascaded latches used like a large shift register)
      selocals.miniDMDLatches[5] = selocals.miniDMDLatches[4];
      selocals.miniDMDLatches[4] = selocals.miniDMDLatches[3];
      selocals.miniDMDLatches[3] = selocals.miniDMDLatches[2];
      selocals.miniDMDLatches[2] = selocals.miniDMDLatches[1];
      selocals.miniDMDLatches[1] = selocals.miniDMDLatches[0];
      selocals.miniDMDLatches[0] = selocals.auxdata;
      const int blank = (selocals.auxdata & 0x80) || (~data & 0x80);
      if (blank == 0) { // 2ms strobe per column => 20ms per frame of 10 columns
         const UINT16 colMask = ((selocals.miniDMDLatches[5] & 0x7) << 7) | (selocals.miniDMDLatches[4] & 0x7F);
         selocals.miniDMDcol = core_BitColToNum(colMask);
         if (selocals.miniDMDcol < 10 && selocals.prevMiniDMDCol != selocals.miniDMDcol) {
            const UINT16 greenLEDs = ((selocals.miniDMDLatches[0] & 0x7F) << 9) | ((selocals.miniDMDLatches[2] & 0x7F) << 2);
            selocals.miniDMD14x10[0][selocals.miniDMDcol] = (greenLEDs >> 8) | (greenLEDs << 8);
            const UINT16 redLEDs = ((selocals.miniDMDLatches[1] & 0x7F) << 9) | ((selocals.miniDMDLatches[3] & 0x7F) << 2);
            selocals.miniDMD14x10[1][selocals.miniDMDcol] = (redLEDs >> 8) | (redLEDs << 8);
            if ((selocals.prevMiniDMDCol == 8 && selocals.miniDMDcol == 9) || (selocals.prevMiniDMDCol == 1 && selocals.miniDMDcol == 0)) {
               core_dmd_submit_frame(&core_gameData->lcdLayout[1], (UINT8*)(selocals.miniDMD14x10[0]), 1); // Green Leds
               core_dmd_submit_frame(&core_gameData->lcdLayout[2], (UINT8*)(selocals.miniDMD14x10[1]), 1); // Red Leds
               // Output mini DMD as LED segments (backward compatibility, 2 blocks of 7 bicolor LEDs)
               UINT16* seg = coreGlobals.drawSeg;
               int isGrn, isRed;
               for (int ii = 0; ii < 10; ii++) {
                  UINT16 bits1 = 0, mask1 = 0x8000;
                  UINT16 bits2 = 0, mask2 = 0x0100;
                  UINT16 greenLEDs = selocals.miniDMD14x10[0][ii];
                  UINT16 redLEDs = selocals.miniDMD14x10[1][ii];
                  redLEDs = (redLEDs >> 8) | (redLEDs << 8);
                  greenLEDs = (greenLEDs >> 8) | (greenLEDs << 8);
                  for (int kk = 0; kk < 7; kk++, mask1 >>= 1, mask2 >>= 1) {
                     isGrn = (greenLEDs & mask1) > 0;
                     isRed = (redLEDs & mask1) > 0;
                     bits1 = (bits1 << 2) | (isGrn << 1) | isRed;
                     isGrn = (greenLEDs & mask2) > 0;
                     isRed = (redLEDs & mask2) > 0;
                     bits2 = (bits2 << 2) | (isGrn << 1) | isRed;
                  }
                  *seg++ = bits1;
                  *seg++ = bits2;
               }
            }
            selocals.prevMiniDMDCol = selocals.miniDMDcol;
         }
      }
   }

   selocals.lastgiaux = data;
}

// Custom video renderer for the 3-Color MINI DMD Type 4 (Simpsons) (2 color R/G Led matrix 14x10)
PINMAME_VIDEO_UPDATE(seminidmd_update) {
   if (pmoptions.dmd_only)
      return 0;
   static const UINT16 colors[4] = { 0, 7, 8, 10 }; // off, green, red, yellow
   const UINT16 *seg = coreGlobals.drawSeg;
   const int x = layout->left;
   const int y = layout->top;
   const int displaySize = pmoptions.dmd_compact ? 1 : 2;
   BMTYPE** lines = ((BMTYPE**)bitmap->line) + (y * displaySize);
   unsigned int o = 0;
   for (int ii = 0; ii < 10; ii++) {
      BMTYPE* line1 = (*lines) + (x * displaySize);
      BMTYPE* line2 = (*lines) + ((x + 7) * displaySize);
      UINT16 bits1 = *seg++;
      UINT16 bits2 = *seg++;
      for (int kk = 0; kk < 7; kk++, bits1 <<= 2, bits2 <<= 2) {
         *line1 = colors[(bits1 >> 12) & 3];
         line1 += displaySize;
         *line2 = colors[(bits2 >> 12) & 3];
         line2 += displaySize;
      }
      lines += displaySize;
   }
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
  { 0x200a, 0x200a, lampdriv_r },
  { 0x3000, 0x3000, dedswitch_r },
  { 0x3100, 0x3100, dip_r },
  { 0x3400, 0x3400, switch_r },
  { 0x3406, 0x3407, gilamp_r }, // GI lamps on Simpsons?
  { 0x3500, 0x3500, dmdie_r },
  { 0x3700, 0x3700, dmdstatus_r },
  { 0x200c, 0x3fff, unknown_r }, // unknown ports from $200c to $3fff, excluding known ports mapped above
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(se_writemem)
  { 0x0000, 0x1fff, ram_w },
  { 0x2000, 0x2003, se_solenoid_w },
  { 0x2006, 0x2007, auxboard_w },
  { 0x2008, 0x2008, lampstrb_w },
  { 0x2009, 0x2009, auxlamp_w },
  { 0x200a, 0x200a, lampdriv_w },
  { 0x200b, 0x200b, giaux_w },
//{ 0x201a, 0x201a, x201a_w },    // Golden Cue unknown device
  { 0x3200, 0x3200, mcpu_bank_w },
  { 0x3300, 0x3300, switch_w },
  { 0x3406, 0x3407, gilamp_w }, // GI lamps on Simpsons?
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
  MDRV_CPU_VBLANK_INT(se_vblank, VBLANK)
  MDRV_CPU_PERIODIC_INT(se_irq, SE_FIRQFREQ)
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
  MDRV_CPU_VBLANK_INT(se_vblank, VBLANK)
  MDRV_CPU_PERIODIC_INT(se_irq, SE_FIRQFREQ)
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
