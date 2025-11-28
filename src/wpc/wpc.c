// license:BSD-3-Clause

#include <stdarg.h>
#include <time.h>
#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sndbrd.h"
#include "snd_cmd.h"
#include "wmssnd.h"
#include "sim.h"
#include "core.h"
#include "wpc.h"
#include "bulb.h"
#ifdef PINMAME_HOST_UART
#include "uart_16c450.h"
#include "uart_8251.h"
#include "uart_host.h"
#endif
#ifdef PROC_SUPPORT
#include "p-roc/p-roc.h"
#endif

#define PRINT_GI_DATA      0 /* printf the GI Data for debugging purposes   */
#define DEBUG_GI           0 /* debug GI PWM code - more printf stuff basically */
#define DEBUG_GI_W         0 /* debug GI write - even more printf stuff */
#define WPC_FAST_FLIP      1
#define WPC_N_GI           5 // Number of GI lines (5 lines, but 2 are always on WPC95)
#ifdef PROC_SUPPORT
#define WPC_INTERFACE_UPD_PER_FRAME      32 // Not sure how PROC handles its input/output so keep the value before interrupt refactoring
#else
#define WPC_INTERFACE_UPD_PER_FRAME       1 // For regular lib/V/PinMame, one interface update at 60Hz should be enough (it's what most other drivers do)
#endif

/*-- Smoothing values --*/
#if defined(PROC_SUPPORT) || defined(PPUC_SUPPORT)
// TODO/PROC: Make variables out of these defines. Values depend on "-proc" switch.
#define WPC_SOLSMOOTH      1 /* Don't smooth values on real hardware */
#define WPC_LAMPSMOOTH     1
#define WPC_DISPLAYSMOOTH  1
#else
#define WPC_SOLSMOOTH      4 /* Smooth the Solenoids over this number of VBLANKS (=60Hz/X) */
#define WPC_LAMPSMOOTH     2 /* Smooth the lamps over this number of VBLANKS (=60Hz/X) */
#define WPC_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS (=60Hz/X) */
#endif

/*-- IRQ frequency, most WPC functions are performed at 1/16 of this frequency --*/
#define WPC_IRQFREQ        (8000000./8192.) /* IRQ Frequency-Timed by JD (976) */

#define DMD_ROWFREQ        (2000000./(16.*16.*2.)) // DMD row frequency deduced from schematics 16.9148.1 (half of length is serial dot shift then column latch)

#define GENWPC_HASDMD      (GEN_ALLWPC & ~(GEN_WPCALPHA_1|GEN_WPCALPHA_2))
#define GENWPC_HASFLIPTRON (GEN_ALLWPC & ~(GEN_WPCALPHA_1|GEN_WPCALPHA_2|GEN_WPCDMD))
#define GENWPC_HASWPC95    (GEN_WPC95 | GEN_WPC95DCS)
#define GENWPC_HASPIC      (GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS)
/*-- protected memory unlock value --*/
#define WPC_PROTMEMCODE 0xb4

/*---------------------
/  local WPC functions
/----------------------*/
/*-- interrupt handling --*/
static INTERRUPT_GEN(wpc_irq);
static void wpc_highres_timer(int param);

/*-- PIC security chip emulation --*/
static int  wpc_pic_r(void);
static void wpc_pic_w(int data);
static void wpc_serialCnv(const char no[21], UINT8 pic[16], UINT8 code[3]);

/*-- DMD --*/
static VIDEO_START(wpc_dmd);
static void wpc_dmd_hsync(int);
PINMAME_VIDEO_UPDATE(wpcdmd_update32);
PINMAME_VIDEO_UPDATE(wpcdmd_update64);

/*-- protected memory --*/
static WRITE_HANDLER(wpc_ram_w);

/*-- misc. --*/
static MACHINE_INIT(wpc);
static MACHINE_STOP(wpc);
static NVRAM_HANDLER(wpc);
static SWITCH_UPDATE(wpc);

/*-- external interfaces (input, display, PROC,...) --*/
static INTERRUPT_GEN(wpc_interface_update);


/*---------------------
/  Global variables
/---------------------*/
UINT8 *wpc_data;     /* WPC registers */

const struct core_dispLayout wpc_dispAlpha[] = {
  {0,0, 0,13,CORE_SEG16R},{0,26,13,2,CORE_SEG16D},{0,30,15,1,CORE_SEG16N},
  {4,0,20,13,CORE_SEG16R},{4,26,33,2,CORE_SEG16D},{4,30,35,1,CORE_SEG16N},
  {0}
};
const struct core_dispLayout wpc_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(genf *)wpcdmd_update32,NULL}, {0}
};
const struct core_dispLayout wpc_dispDMD64[] = {
  {0,0, 0,5,CORE_SEG7},
  {11,0,64,128,CORE_DMD,(genf *)wpcdmd_update64,NULL}, {0}
};

/*------------------
/  Local variables
/------------------*/
static struct {
  UINT16  memProtMask;
  UINT32  solData;        /* current value of solenoids 1-28 */
  UINT8   solFlip, solFlipPulse;  /* current value of flipper solenoids */
  UINT8   nonFlipBits;    /* flipper solenoids not used for flipper (smoothed) */
  int     interfaceUpdateCount;    /* interface update per frame counter */
  core_tSeg alphaSeg;
  struct {
    UINT8 sData[16];
    UINT8 codeNo[3];
    UINT8 lastW;          /* last written command */
    UINT8 sNoS;           /* serial number scrambler */
    UINT8 count;
    int   codeW;
  } pic;
  int pageMask;           /* page handling */
  UINT8 diagnosticLed;
  int zc;                 /* zero cross flag */
  int phase;
  double lastACZeroCrossTimeStamp; /* Last time AC did cross 0 (120Hz) */
  double gi_on_time[WPC_N_GI]; /* Global time when GI Triac was turned on */
  volatile UINT8 conductingGITriacs; /* Current conducting triacs of WPC GI strings (triacs conduct if pulsed, then continue to conduct until current is near 0, that it to say at zero cross) */
  volatile UINT8 conductingChaseLightTriacs; /* Current conducting triacs of CFTBL Chase light GI strings (triacs conduct if pulsed, then continue to conduct until current is near 0, that it to say at zero cross) */
  UINT32 solenoidbits[64];
  int modsol_count;
  int modsol_sample;
  mame_timer* highres_timer;
  int wpcFIRQ;            // State of the WPC chip high res timer FIRQ output
  int sndFIRQ;            // State of the FIRQ from pre DCS sound board (part number A-12738)
} wpclocals;

// Have to put this here, instead of wpclocals, since wpclocals is cleared/initialized AFTER game specific init.   Grrr.
static int wpc_modsol_aux_board = 0;
static int wpc_fastflip_addr = 0;

static struct {
  int    row;                    // Rasterizer row position
  int    firq;                   // State of FIRQ output line to CPU
  core_tDMDPWMState pwm_state;
} dmdlocals;

/*-- pointers --*/
static mame_file *wpc_printfile = NULL;
UINT8 *wpc_ram = NULL;

/*---------------------------
/  Memory map for CPU board
/----------------------------*/
static MEMORY_READ_START(wpc_readmem)
  { 0x0000, 0x2fff, MRA_RAM },
  { 0x3000, 0x31ff, MRA_BANK4 },  /* DMD WPC-95 only */
  { 0x3200, 0x33ff, MRA_BANK5 },  /* DMD WPC-95 only */
  { 0x3400, 0x35ff, MRA_BANK6 },  /* DMD WPC-95 only */
  { 0x3600, 0x37ff, MRA_BANK7 },  /* DMD WPC-95 only */
  { 0x3800, 0x39ff, MRA_BANK2 },  /* DMD */
  { 0x3A00, 0x3bff, MRA_BANK3 },  /* DMD */
  { 0x3c00, 0x3faf, MRA_RAM },
  { 0x3fb0, 0x3fff, wpc_r },      /* WPC chip and IO/DMD/aux boards */
  { 0x4000, 0x7fff, MRA_BANK1 },  /* bank switched ROM */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(wpc_writemem)
  { 0x0000, 0x2fff, wpc_ram_w, &wpc_ram },
  { 0x3000, 0x31ff, MWA_BANK4 },        /* DMD WPC-95 only */
  { 0x3200, 0x33ff, MWA_BANK5 },        /* DMD WPC-95 only */
  { 0x3400, 0x35ff, MWA_BANK6 },        /* DMD WPC-95 only */
  { 0x3600, 0x37ff, MWA_BANK7 },        /* DMD WPC-95 only */
  { 0x3800, 0x39ff, MWA_BANK2 },        /* DMD */
  { 0x3A00, 0x3bff, MWA_BANK3 },        /* DMD */
  { 0x3c00, 0x3faf, MWA_RAM },
  { 0x3fb0, 0x3fff, wpc_w, &wpc_data }, /* WPC chip and IO/DMD/aux boards */
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

// convert lamp and switch numbers
// both use column*10+row
// convert to 0-63 (+8)
// i.e. 11=8,12=9,21=16
static int wpc_sw2m(int no) { return (no/10)*8+(no%10-1); }
int wpc_m2sw(int col, int row) { return col*10+row+1; }

// Zero Cross: a voltage comparator triggers when +5V AC reaches +5V or -5V, so at 120Hz in US (would be 100Hz in Europe), leading to around ~8.3ms period
static void wpc_zc(int data) {
   wpclocals.phase = 1 - wpclocals.phase;

   // Set Zero Cross flag (it's reset when read)
   wpclocals.zc = 1;

   // GI outputs are driven by Triac which continue to conduct when turned on until current decreases under the holding current.
   // Current drops below this threshold at zero cross, so we update conducting triacs which are the only one still being triggered by the latched value.
   wpclocals.conductingGITriacs = wpc_data[WPC_GILAMPS];

   // GI Dimming seems to be processed by the CPU during IRQ handling with IRQ being every ~1ms
   // Since zero cross and IRQ are not perfectly aligned, there can be either 8 or 9 irq per zero cross period in 60Hz countries (~1ms / ~8.3ms, this would be 9 or 10 in 50Hz countries ~1ms / ~10ms).
   // The resulting GI level is the ratio of on/off pulse of the PWM cycle computed using MAME global time
   double zc_time = timer_get_time();
   if (zc_time > wpclocals.lastACZeroCrossTimeStamp)
   {
      for (int ii = 0, tmp= wpclocals.conductingGITriacs; ii < 5; ii++, tmp >>= 1) {
         if (wpclocals.gi_on_time[ii] >= zc_time)
            // Not turned on since last zero cross
            coreGlobals.gi[ii] = 0;
         else
            // The initial implementation would return values between 0 and 8, so we keep this scaling for backward compatibility
            coreGlobals.gi[ii] = (int)(0.5 + 8.0 * (1.0 - (wpclocals.gi_on_time[ii] - wpclocals.lastACZeroCrossTimeStamp) / (zc_time - wpclocals.lastACZeroCrossTimeStamp)));
         // If bit is still set, ASIC GI output Txx is continuously high and Triac continuously conduct (for the complete AC period), otherwise we set it's turn on time far after the next zero cross.
         wpclocals.gi_on_time[ii] = tmp & 0x01 ? zc_time : zc_time + 100.0;
      }
      #if DEBUG_GI
      printf("[%8f] Zero Cross: ", timer_get_time());
      for (int i = 0; i < WPC_N_GI; i++)
         printf("GI[%d]=%d ", i, coreGlobals.gi[i]);
      printf("\n");
      #endif
   }

   // Synchronize core PWM integration AC signal (keeping the phase right to avoid breaking AC integration)
   if (wpclocals.phase)
      core_zero_cross();
   wpclocals.lastACZeroCrossTimeStamp = timer_get_time();

   // More precise implementation with better physic emulation
   if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_GI | CORE_MODOUT_FORCE_ON))
      core_write_pwm_output_8b(CORE_MODOUT_GI0, wpclocals.conductingGITriacs & 0x1F);
   if ((core_gameData->hw.gameSpecific1 & WPC_CFTBL) && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_LAMPS | CORE_MODOUT_FORCE_ON))) // CFTBL chase lights
   {
      int chase_2b = ((coreGlobals.pulsedSolState >> 22) & 2) | ((coreGlobals.pulsedSolState >> 19) & 1); // 2 bit decoder => select one of the 4 chase light strings
      int chase_gi = ((wpclocals.conductingGITriacs & 1) ? 0x0F : 0x00) | ((wpclocals.conductingGITriacs & 8) ? 0xF0 : 0x00); // GI outputs
      wpclocals.conductingChaseLightTriacs = chase_gi & (0x11 << chase_2b);
      core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 64, wpclocals.conductingChaseLightTriacs);
   }
}

void wpc_set_modsol_aux_board(int board)
{
  wpc_modsol_aux_board = board;
}

void wpc_set_fastflip_addr(int addr)
{
  wpc_fastflip_addr = addr;
}


#ifdef PROC_SUPPORT
  /*
    Configure a function pointer with a default handler for processing
    solenoid changes.  This allows specific machines to override the handler
    in their init routine with one that can intercept certain solenoid
    changes and then have the default handler cover all other cases.

    `solNum` is 0 to 63 and represents a bit number in the value returned by
    core_getAllSol().

    `enabled` indicates whether the solenoid has changed to enabled (1) or
    disabled (0).

    `smoothed` indicated whether this came from the period solenoid change
    processing with smoothed readings (1) or came from an immediate solenoid
    change in the emulator (0).
  */
  wpc_proc_solenoid_handler_t wpc_proc_solenoid_handler = default_wpc_proc_solenoid_handler;
  void default_wpc_proc_solenoid_handler(int solNum, int enabled, int smoothed) {
    // by default, we only processed smoothed solenoid readings.  Custom
    // handlers in the various sims may process non-smoothed readings (for
    // reduced latency) and ignore the smoothed readings for some solenoids.
    if (!smoothed)
      return;

    // Standard Coils
    if (solNum < 32) {
      if (solNum > 27 && (core_gameData->gen & GEN_ALLWPC)) { // 29-32 GameOn
        switch (solNum) {
          case 28:
            //fprintf(stderr, "SOL28: %s\n", enabled ? "game over" : "start game");
            // If game supports this "GameOver" solenoid, it's safe to disable the
            // flippers here (something that happens when the game starts up) and
            // rely on solenoid 30 telling us when to enable them.
            // ...on WPC-95 games, SOL28 doesn't seem to fire
            if (enabled) {
              procConfigureFlipperSwitchRules(0);
            } else if (core_gameData->gen & GEN_WPC95) {
              procConfigureFlipperSwitchRules(1);
            }
            break;
          case 30:
            //fprintf(stderr, "SOL30: %s flippers\n", enabled ? "enable" : "disable");
            procConfigureFlipperSwitchRules(enabled);
            // enable flippers on pre-fliptronic games (with no PRFlippers)
            if (core_gameData->gen & (GEN_WPCALPHA_1 | GEN_WPCALPHA_2 | GEN_WPCDMD)) {
              procDriveLamp(79, enabled);
            }
            break;
          default:
            //fprintf(stderr, "SOL%d (%s) does not map\n", solNum, enabled ? "on" : "off");
            return;
        }
      } else {
        // C01 to C28 (WPC) or C32 (all others)
        procDriveCoil(solNum+40, enabled);
      }
    } else if (solNum < 36) {
      // upper flipper coils, C33 to C36
      procDriveCoil(solNum+4, enabled);
    } else if (solNum < 44) {
      if (core_gameData->gen & GENWPC_HASWPC95) {
        // solNum 40 to 43 are duplicates of 36 to 39
        if (solNum < 40)
          procDriveCoil(solNum+32, enabled);
      } else {
        procDriveCoil(solNum+108, enabled);
      }
    } else if (solNum < 48) {
      // In pre-fliptronic WPC games, coil 44 (odd?) seems to work as the flipper enable/disable
      if (core_gameData->gen & (GEN_WPCDMD | GEN_WPCALPHA_1 | GEN_WPCALPHA_2)) {
          if (solNum == 44) procFlipperRelay(enabled);
      } else {
        // lower flipper coils, C29 to C32
        procDriveCoil(solNum-12, enabled);
      }
    } else if (solNum >= 50 && solNum < 58) {
      // 8-driver board (DM, IJ, RS, STTNG, TZ), C37 to C44
      // note that this maps to same P-ROC coils as SOL 36-43 on non-WPC95
      procDriveCoil(solNum+94, enabled);
    } else {
      //fprintf(stderr, "SOL%d (%s) does not map\n", solNum, enabled ? "on" : "off");
      return;
    }
    // TODO:PROC: Upper flipper circuits in WPC-95. (Is this still the case?)
    // Some games (AFM) seem to use sim files to activate these coils.  Others (MM) don't ever seem to activate them (Trolls).
  }

  // Called from wpc_w() to process immediate changes to solenoid values.
  void proc_immediate_solenoid_change(int offset, UINT8 new_data) {
    static UINT32 current_values = 0;
    UINT32 mask = (0xFF << offset);
    UINT8 changed_data = new_data ^ ((current_values & mask) >> offset);

    if (changed_data) {
      int i;
      current_values = (current_values & ~mask) | (new_data << offset);
      for (i = offset; changed_data; ++i, changed_data >>= 1, new_data >>= 1) {
        // bits 28-31 map to C37 to C40 (index 36 to 39 of wpc_proc_solenoid_handler)
        if (i == 28) i += 8;
        if (changed_data & 1)
          wpc_proc_solenoid_handler(i, new_data & 1, FALSE);
      }
    }
  }
#endif

/*-----------------
/  Machine drivers
/------------------*/
static MACHINE_DRIVER_START(wpc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(wpc,NULL,wpc)
  MDRV_CPU_ADD(M6809, 2000000) // MC6809E, 68B09E XTAL8/4
  MDRV_CPU_MEMORY(wpc_readmem, wpc_writemem)
  MDRV_CPU_VBLANK_INT(wpc_interface_update, WPC_INTERFACE_UPD_PER_FRAME)
  MDRV_CPU_PERIODIC_INT(wpc_irq, WPC_IRQFREQ)
  MDRV_NVRAM_HANDLER(wpc)
  MDRV_DIPS(8)
  MDRV_SWITCH_UPDATE(wpc)
  MDRV_DIAGNOSTIC_LEDH(1)
  MDRV_SWITCH_CONV(wpc_sw2m,wpc_m2sw)
  MDRV_LAMP_CONV(wpc_sw2m,wpc_m2sw)
  MDRV_TIMER_ADD(wpc_zc,120)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_alpha)
  MDRV_IMPORT_FROM(wpc)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_alpha1S)
  MDRV_IMPORT_FROM(wpc)
  MDRV_IMPORT_FROM(wmssnd_s11cs)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_alpha2S)
  MDRV_IMPORT_FROM(wpc)
  MDRV_IMPORT_FROM(wmssnd_wpcs)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_dmd)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_TIMER_ADD(wpc_dmd_hsync,DMD_ROWFREQ)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_dmdS)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_TIMER_ADD(wpc_dmd_hsync,DMD_ROWFREQ)
  MDRV_IMPORT_FROM(wmssnd_wpcs)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_dcsS)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_TIMER_ADD(wpc_dmd_hsync,DMD_ROWFREQ)
  MDRV_IMPORT_FROM(wmssnd_dcs1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_95S)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_TIMER_ADD(wpc_dmd_hsync,DMD_ROWFREQ)
  MDRV_IMPORT_FROM(wmssnd_dcs2)
MACHINE_DRIVER_END

// The FIRQ line is wired to 2 or 3 sources:
// - the WPC FIRQ output, which is triggered by its internal high res timer
// - the programmable DMD FIRQ output (raised when a choosen row is reached, cleared when the FIRQ row is defined)
// - the FIRQ from pre DCS sound board (sound board part A-12738)
static void update_firq(void) {
  cpu_set_irq_line(WPC_CPUNO, M6809_FIRQ_LINE, (wpclocals.wpcFIRQ | wpclocals.sndFIRQ | dmdlocals.firq) ? HOLD_LINE : CLEAR_LINE);
}

// High resolution timer provided by the WPC chip. Used by alpha gen games to dim segments
// For example, BOP uses it to dim/blink entries in the service menu
static void wpc_highres_timer(int param) {
  // printf("%8.5f High res timer FIRQ\n", timer_get_time());
  wpclocals.wpcFIRQ = 1;
  update_firq();
}

/*--------------------------------------------------------------
/ This is generated WPC_INTERFACE_UPD_PER_FRAME times per frame (=60*WPC_INTERFACE_UPD_PER_FRAME Hz)
/ to handle overall bookkeeping (switch update, PROC management, smoothing of the solenoids and lamps, ...)
/--------------------------------------------------------------*/
static INTERRUPT_GEN(wpc_interface_update) {
  #ifdef PROC_SUPPORT
	 static int gi_last[WPC_N_GI];
	 int changed_gi[WPC_N_GI];
	  if (coreGlobals.p_rocEn) {
		 procTickleWatchdog();
	  }
  #endif

  wpclocals.interfaceUpdateCount++;

  /*--------------------------------------------------------
  /  Most solenoids don't have a holding coil so the software
  /  simulates it by pulsing the power to the main (and only) coil.
  /  (I assume this is why the Auto Fire diverter in TZ seems to flicker.)
  /  I simulate the coil position by looking over WPC_SOLSMOOTH vblanks
  /  and only turn the solenoid off if it has not been pulsed
  /  during that time.
  /-------------------------------------------------------------*/
  if ((wpclocals.interfaceUpdateCount % (WPC_INTERFACE_UPD_PER_FRAME*WPC_SOLSMOOTH)) == 0) {
    coreGlobals.solenoids = (wpc_data[WPC_SOLENOID1] << 24) |
                            (wpc_data[WPC_SOLENOID3] << 16) |
                            (wpc_data[WPC_SOLENOID4] <<  8) |
                             wpc_data[WPC_SOLENOID2];
    #ifdef PROC_SUPPORT
    //TODO/PROC: Check implementation
    if (coreGlobals.p_rocEn) {
      int ii;
      static UINT64 lastSol = 0;
      UINT64 allSol = core_getAllSol();
      UINT64 chgSol = (allSol ^ lastSol);
      lastSol = allSol;

      if (chgSol) {
        for (ii=0; ii<64; ii++) {
          if (chgSol & 0x1) {
            if (mame_debug) {
              //fprintf( stderr,"Drive SOL%02d %s\n", ii, (allSol & 0x1) ? "on" : "off");
            }
            wpc_proc_solenoid_handler(ii, allSol & 0x1, TRUE);
          }
          chgSol >>= 1;
          allSol >>= 1;
        }
      }

      // GI
      for (ii = 0; ii < WPC_N_GI; ii++) {
        changed_gi[ii] = gi_last[ii] != coreGlobals.gi[ii];
        gi_last[ii] = coreGlobals.gi[ii];

        if (changed_gi[ii]) {
          procDriveLamp(ii+72, coreGlobals.gi[ii] > 2);
        }
      }
    }
    #endif

    wpc_data[WPC_SOLENOID1] = wpc_data[WPC_SOLENOID2] = 0;
    wpc_data[WPC_SOLENOID3] = wpc_data[WPC_SOLENOID4] = 0;

    // GameOn Solenoid (solenoid #31) and other state bits (solenoids #29 and #30):
    // - We always mirror the state of 2 out of the 3 GPIO on J111 to solenoids #29 and #30
    // - Before fliptronic: real game on solenoid, controlled by CPU through the high bit of WPC_GILAMPS
    //   CPU board: U4 - J121 / Power driver board: J113 - Relay - J110 - Cabinet switch - J109 / Flippers
    // - Starting with fliptronic, no GameOn solenoid as flippers are ROM controlled:
    //   We use state of FastFlip address as a fake GameOn (letting the client decide to use it or not)
    //   If Fastflip is missing, we default to mirroring the high bit of WPC_GILAMPS
    // Note that these states are stored in solenoids2 (sol 33..35) but are remapped to solenoid 29..31 in core.c
    if (wpc_fastflip_addr == 0)
    {
      coreGlobals.solenoids2 = (wpc_data[WPC_GILAMPS] & 0xe0) << 3;
      core_write_pwm_output(CORE_MODOUT_SOL0 + 29 - 1, 3, wpc_data[WPC_GILAMPS] >> 5);
    }
    else
    {
      assert(core_gameData->gen & GENWPC_HASFLIPTRON);
      coreGlobals.solenoids2 = (wpc_data[WPC_GILAMPS] & 0x60) << 3;
      core_write_pwm_output(CORE_MODOUT_SOL0 + 29 - 1, 2, wpc_data[WPC_GILAMPS] >> 5);
      if (wpc_ram[wpc_fastflip_addr] > 0)
        coreGlobals.solenoids2 |= 0x400;
      core_write_pwm_output(CORE_MODOUT_SOL0 + 31 - 1, 1, wpc_ram[wpc_fastflip_addr] > 0 ? 1 : 0);
    }

    // ROM controlled flipper coils when using FlipTronics
    if (core_gameData->gen & GENWPC_HASFLIPTRON) {
      coreGlobals.solenoids2 |= wpclocals.solFlip;
      wpclocals.solFlip = wpclocals.solFlipPulse;
    }
  }
  else if (((wpclocals.interfaceUpdateCount % WPC_INTERFACE_UPD_PER_FRAME) == 0) && (core_gameData->gen & GENWPC_HASFLIPTRON)) {
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & (0xffffff00 | wpclocals.nonFlipBits)) | wpclocals.solFlip;
    wpclocals.solFlip = (wpclocals.solFlip & wpclocals.nonFlipBits) | wpclocals.solFlipPulse;
  }

  /*--------------------------------------------------------
  / The lamp circuit pulses the power to the lamps one column
  / at a time. By only reading the value every WPC_LAMPSMOOTH
  / vblank we get a steady light.
  /
  / Williams changed the software for the lamphandling at some stage
  / earlier code (TZ 9.2)       newer code (TZ 9.4)
  / ------------                -----------
  / Activate Column             Activate rows
  / Activate rows               Activate column
  / wait                        wait
  / Deactivate rows             repeat for next column
  / repeat for next column
  / ..
  / For the game it doesn't really matter but it confused me when
  / the lamp code here worked for 9.2 but not in 9.4.
  /-------------------------------------------------------------*/
  if ((wpclocals.interfaceUpdateCount % (WPC_INTERFACE_UPD_PER_FRAME*WPC_LAMPSMOOTH)) == 0) {
    #ifdef PROC_SUPPORT
		if (coreGlobals.p_rocEn) {
			int col, row, procLamp;
			for(col = 0; col < CORE_STDLAMPCOLS; col++) {
				UINT8 chgLamps = coreGlobals.lampMatrix[col] ^ coreGlobals.tmpLampMatrix[col];
				UINT8 tmpLamps = coreGlobals.tmpLampMatrix[col];
				for (row = 0; row < 8; row++) {
					procLamp = 80 + (8 * col) + row;
					if (chgLamps & 0x01) {
						procDriveLamp(procLamp, tmpLamps & 0x01);
					}
					if (coreGlobals.isKickbackLamp[procLamp]) {
						procKickbackCheck(procLamp);
					}
					chgLamps >>= 1;
					tmpLamps >>= 1;
				}
			}
			procFlush();
		}
    #endif
    memcpy((void*)coreGlobals.lampMatrix, (void*)coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset((void*)coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }

  // Update segments & diagnostic leds accumulated over a few 'frames'
  if ((wpclocals.interfaceUpdateCount % (WPC_INTERFACE_UPD_PER_FRAME*WPC_DISPLAYSMOOTH)) == 0) {
    if ((core_gameData->gen & GENWPC_HASDMD) == 0) {
      memcpy(coreGlobals.segments, wpclocals.alphaSeg, sizeof(coreGlobals.segments));
      coreGlobals.segments[15].w &= ~0x8080; coreGlobals.segments[35].w &= ~0x8080;
      memset(wpclocals.alphaSeg, 0, sizeof(wpclocals.alphaSeg));
    }
    coreGlobals.diagnosticLed = wpclocals.diagnosticLed;
    wpclocals.diagnosticLed = 0;
  }

  #ifdef PROC_SUPPORT
  // Check for any coils that need to be disabled due to inactivity.
  if (coreGlobals.p_rocEn) {
    procCheckActiveCoils();
    procFullTroughDisablesFlippers();
  }
  #endif

  /*------------------------------
  /  Update switches every vblank or on every interface update when using P-ROC
  /-------------------------------*/
  #ifdef PROC_SUPPORT
  if (coreGlobals.p_rocEn || (wpclocals.interfaceUpdateCount % WPC_INTERFACE_UPD_PER_FRAME) == 0)
  #endif
  core_updateSw((core_gameData->gen & GENWPC_HASFLIPTRON) ? TRUE : (wpc_data[WPC_GILAMPS] & 0x80));
}

/*----------------------
/ Emulate the WPC chip
/-----------------------*/
READ_HANDLER(wpc_r) {
#ifdef PINMAME_HOST_UART
  if (offset < 8
      && (core_gameData->gen & GENWPC_HASWPC95)
      && pmoptions.serial_device != NULL)
  {
    /* Emulated UART(16C450) on WPC95 AV board */
    return uart_16c450_read(offset);
  }
  else 
#endif
  switch (offset) {
    case WPC_FLIPPERS: /* Flipper switches */
      if ((core_gameData->gen & GENWPC_HASWPC95) == 0)
        return ~coreGlobals.swMatrix[CORE_FLIPPERSWCOL];
      break;
    case WPC_FLIPPERSW95:
      if (core_gameData->hw.gameSpecific1 & WPC_PH) { // PH: reel switches
        return ~coreGlobals.swMatrix[3];
      }
      if (core_gameData->gen & GENWPC_HASWPC95)
        return ~coreGlobals.swMatrix[CORE_FLIPPERSWCOL];
      break;
    case WPC_SWCOINDOOR: /* cabinet switches */
      return coreGlobals.swMatrix[CORE_COINDOORSWCOL];
    case WPC_SWROWREAD: /* read row */
      if (core_gameData->gen & GENWPC_HASPIC)
        return wpc_pic_r();
      return core_getSwCol(wpc_data[WPC_SWCOLSELECT]);
    case WPC_SHIFTADRH:
      return wpc_data[WPC_SHIFTADRH] +
             ((wpc_data[WPC_SHIFTADRL] + (wpc_data[WPC_SHIFTBIT]>>3))>>8);
    case WPC_SHIFTADRL:
      return (wpc_data[WPC_SHIFTADRL] + (wpc_data[WPC_SHIFTBIT]>>3)) & 0xff;
    case WPC_SHIFTBIT:
    case WPC_SHIFTBIT2:
      return 1<<(wpc_data[offset] & 0x07);
    case WPC_HIGHRESTIMER:
      //DBGLOG(("R:WPC_HIGHRESTIMER\n"));
      return (wpclocals.wpcFIRQ != 0) ? 0x80 : 0x00;
    case WPC_DIPSWITCH:
      return core_getDip(0);
    case WPC_RTCHOUR: {
      // Don't know how the clock works.
      // This hack by JD puts right values into the memory locations
      UINT8 *timeMem = wpc_ram + 0x1800;
      UINT16 checksum = 0;
      static time_t oldTime = 0;
      time_t now;
      static struct tm *systime;

      time(&now);
      if (now != oldTime)
      {
          systime = localtime(&now);
          oldTime = now;
      }
      checksum += *timeMem++ = (systime->tm_year + 1900)>>8;
      checksum += *timeMem++ = (systime->tm_year + 1900)&0xff;
      checksum += *timeMem++ = systime->tm_mon + 1;
      checksum += *timeMem++ = systime->tm_mday;
      checksum += *timeMem++ = systime->tm_wday + 1;
      checksum += *timeMem++ = 0;
      checksum += *timeMem++ = 1;
      checksum = 0xffff - checksum;
      *timeMem++ = checksum>>8;
      *timeMem   = checksum & 0xff;
      return systime->tm_hour;
    }
    case WPC_RTCMIN: {
      static time_t oldTime = 0;
      time_t now;
      static struct tm *systime;

      time(&now);
      if (now != oldTime)
      {
          systime = localtime(&now);
          oldTime = now;
      }
      return systime->tm_min;
    }
    case WPC_ZC_IRQ_ACK:
      // Bit7: zero cross state (latched value reseted on read)
      // Bit2: periodic IRQ enabled
      wpc_data[offset] = (wpclocals.zc<<7) | (wpc_data[offset] & 0x7f);
      // FIXME it seems that the zero cross should just be the live zc state, not a latched value reseted on read
      wpclocals.zc = 0;
      break;
    case WPC_SND_S11_DATA0:
      if (sndbrd_0_type() == SNDBRD_S11CS) {
        DBGLOG(("DD_DATA_R: %04x %02x\n", activecpu_get_pc(), sndbrd_0_data_r(0)));
        return sndbrd_0_data_r(0);
      }
      break;
    case WPC_SND_S11_CTRL:
      if (sndbrd_0_type() == SNDBRD_S11CS) {
        DBGLOG(("DD_CTRL_R: %04x %02x\n", activecpu_get_pc(), sndbrd_0_ctrl_r(0)));
        return sndbrd_0_ctrl_r(0);
      }
      break;
    case WPC_SND_S11_UNK:
      if (sndbrd_0_type() == SNDBRD_S11CS) {
        DBGLOG(("DD_UNKNOWN_R: %04x\n", activecpu_get_pc()));
        UINT8 dd_unknown = sndbrd_0_ctrl_r(0) & 0x10 ? 0x81 : 0;
        return dd_unknown;
      }
      break;
    case WPC_SND_DATA:
      if (wpclocals.sndFIRQ != 0)
      {
        wpclocals.sndFIRQ = 0;
        update_firq();
      }
      return sndbrd_0_data_r(0);
    case WPC_SND_CTRL:
      return sndbrd_0_ctrl_r(0);
    case WPC_PRINTBUSY:
      return 0;
    case WPC_SERIAL_DATA:
    case WPC_SERIAL_CTRL:
    case WPC_SERIAL_BAUD:
#ifdef PINMAME_HOST_UART
        if (pmoptions.serial_device != NULL) {
          return uart_8251_read(offset);
        }
#endif
        break;
    case WPC_DMD_SHOWPAGE:
      break;
    case WPC_DMD_PAGE3000:
    case WPC_DMD_PAGE3200:
    case WPC_DMD_PAGE3400:
    case WPC_DMD_PAGE3600:
    case WPC_DMD_PAGE3800:
    case WPC_DMD_PAGE3A00:
      return 0; /* these can't be read */
    case WPC_DMD_FIRQLINE: // Read state of DMD controller board FIRQ
      return dmdlocals.firq ? 0x80 : 0x00;
    default:
      DBGLOG(("wpc_r %4x\n", offset+WPC_BASE));
      break;
  }
  return wpc_data[offset];
}

WRITE_HANDLER(wpc_w) {
#ifdef PROC_SUPPORT
  if (coreGlobals.p_rocEn) {
    // process immediate coil changes for C01 to C28 and C37 to C40
    switch (offset) {
      case WPC_SOLENOID1: proc_immediate_solenoid_change(24, data); break;
      case WPC_SOLENOID2: proc_immediate_solenoid_change( 0, data); break;
      case WPC_SOLENOID3: proc_immediate_solenoid_change(16, data); break;
      case WPC_SOLENOID4: proc_immediate_solenoid_change( 8, data); break;
    }
  }
#endif

#ifdef PINMAME_HOST_UART
  if (offset < 8
      && (core_gameData->gen & GENWPC_HASWPC95)
      && pmoptions.serial_device != NULL)
  {
    /* Emulated UART(16C450) on WPC95 AV board */
    uart_16c450_write(offset, data);
  }
  else
#endif
  switch (offset) {
    case WPC_ROMBANK: { /* change rom bank */
      int bank = data & wpclocals.pageMask;
      cpu_setbank(1, memory_region(WPC_ROMREGION)+ bank * 0x4000);
      #ifdef PINMAME
        /* Bank support for CODELIST */
        cpu_bankid[1] = bank + ( 0x3F ^ wpclocals.pageMask );
      #endif /* PINMAME */
      break;
    }
    case WPC_FLIPPERS: /* Flipper coils of Fliptronics 2 Board */
      if ((core_gameData->gen & GENWPC_HASWPC95) == 0) {
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = ~data;
        #ifdef WPC_FAST_FLIP
          coreGlobals.solenoids2 |= wpclocals.solFlip;
        #endif
        core_write_pwm_output(CORE_MODOUT_SOL0 + 45 - 1, 4, ~data);
        core_write_pwm_output(CORE_MODOUT_SOL0 + 33 - 1, 4, (~data) >> 4);
      }
      break;
    case WPC_FLIPPERCOIL95: /* WPC_EXTBOARD4 */
      if (core_gameData->hw.gameSpecific1 & WPC_PH) { // PH: LED digits
        if (data != 0xff) {
          coreGlobals.segments[core_BitColToNum(0xff ^ data)].w = wpclocals.alphaSeg[core_BitColToNum(0xff ^ data)].w = core_bcd2seg7[wpc_data[WPC_EXTBOARD1]];
        }
      } else if (core_gameData->gen & GENWPC_HASWPC95) { // WPC_FLIPPERCOIL95
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = data;
        #ifdef WPC_FAST_FLIP
          coreGlobals.solenoids2 |= wpclocals.solFlip;
        #endif
        core_write_pwm_output(CORE_MODOUT_SOL0 + 45 - 1, 4, data);
        core_write_pwm_output(CORE_MODOUT_SOL0 + 33 - 1, 4, data >> 4);
      }
      else if ((core_gameData->gen & GENWPC_HASDMD) == 0) // WPC_ALPHA2LO
      {
        //static double prev; printf("WPC_ALPHA2LO %8.5fms %02x %02x\n", timer_get_time() - prev, wpc_data[WPC_ALPHAPOS], data); prev = timer_get_time();
        wpclocals.alphaSeg[20+wpc_data[WPC_ALPHAPOS]].b.lo |= data;
        if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS | CORE_MODOUT_FORCE_ON))
          core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (20 + wpc_data[WPC_ALPHAPOS]) * 2 * 8, data);
      }
      break;
    case WPC_ALPHA2HI:
      if ((core_gameData->gen & GENWPC_HASDMD) == 0)
      {
        wpclocals.alphaSeg[20+wpc_data[WPC_ALPHAPOS]].b.hi |= data;
        if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS | CORE_MODOUT_FORCE_ON))
          core_write_pwm_output_8b(CORE_MODOUT_SEG0 + ((20 + wpc_data[WPC_ALPHAPOS]) * 2 + 1) * 8, data);
      }
      break;
    case WPC_LAMPROW: /* row and column can be written in any order */
      core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0, wpc_data[WPC_LAMPCOLUMN], data, 8);
      break;
    case WPC_LAMPCOLUMN: /* row and column can be written in any order */
      core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0, data, wpc_data[WPC_LAMPROW], 8);
      if (core_gameData->hw.gameSpecific1 & WPC_PH) { // PH uses 192 lamps
        core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 +  64, data, wpc_data[WPC_EXTBOARD2], 8);
        core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 128, data, wpc_data[WPC_EXTBOARD3], 8);
      }
      break;
    case WPC_SWCOLSELECT:
      if (core_gameData->gen & GENWPC_HASPIC)
        wpc_pic_w(data);
      break;
    case WPC_GILAMPS: {
      int ii, tmp;

      // Note that WPC_GILAMPS also controls the 3 GPIO outputs on J111 (3 highest bits) and the GameOn relay of pre-Fliptronics (highest bit)

      //WPC95 only controls 3 of the 5 Triacs, the other 2 are ALWAYS ON (power wired directly)
      //  We simulate this here by forcing the bits on
      if (core_gameData->gen & GEN_WPC95)
        data = data | 0x18;

      // Loop over each GI Triac Bit and turn on according Triacs
      wpclocals.conductingGITriacs |= data; // Triac that were turned on before will continue to conduct until next zero cross, therefore we 'or' them with previous pulsed state
      if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_GI | CORE_MODOUT_FORCE_ON))
         core_write_pwm_output_8b(CORE_MODOUT_GI0, wpclocals.conductingGITriacs & 0x1F);
      if ((core_gameData->hw.gameSpecific1 & WPC_CFTBL) && (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_LAMPS | CORE_MODOUT_FORCE_ON))) // CFTBL chase lights
      {
         int chase_2b = ((coreGlobals.pulsedSolState >> 22) & 2) | ((coreGlobals.pulsedSolState >> 19) & 1); // 2 bit decoder => select one of the 4 chase light strings
         int chase_gi = ((wpclocals.conductingGITriacs & 1) ? 0x0F : 0x00) | ((wpclocals.conductingGITriacs & 8) ? 0xF0 : 0x00); // GI outputs
         wpclocals.conductingChaseLightTriacs |= chase_gi & (0x11 << chase_2b);
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 64, wpclocals.conductingChaseLightTriacs);
      }
      double write_time = timer_get_time();
      for (ii = 0, tmp = data; ii < WPC_N_GI; ii++, tmp >>= 1) {
         // If Bit is set, Triac is turned on (if it was not already on).
         // We save turn on time to compute dimming ratio on next zero cross.
         if ((tmp & 0x01) && write_time < wpclocals.gi_on_time[ii])
            wpclocals.gi_on_time[ii] = write_time;
      }

      #if DEBUG_GI_W
      printf("[%8f] GI_W PC=%4x, data=%2x > GI State/On time ", write_time, activecpu_get_pc(), data);
      for (ii = 0, tmp = data; ii < WPC_N_GI; ii++, tmp >>= 1) {
        printf(" || GI[%d]=%d => %8f", ii, tmp & 0x01, wpclocals.gi_on_time[ii]);
      }
      printf("\n");
      #endif

      break;
    }
    case WPC_EXTBOARD1: /* WPC_ALPHAPOS */
      if (wpc_modsol_aux_board == 1)
      {
        assert(CORE_FIRSTCUSTSOL == 51);
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 48, data << 2, 0xFC); // Write 50..55
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 56, data >> 6, 0x03); // Write 56..57
      }
      else if ((core_gameData->gen & GENWPC_HASDMD) == 0) // WPC_ALPHAPOS
      {
        // Alphanumeric segment strobing change => turn off previous segment and on the ones of the new selected position
        // Operation is set all segs to 0 (blanking), then strove to next column, then set segments.
        // The delay between setting segments then blanking them is used to a rough PWM dimming
        // Overall timing is 1ms maximum per digit over a 16ms period
        //static double prev; printf("WPC_ALPHAPOS %8.5fms %02x\n", timer_get_time() - prev, data); prev = timer_get_time();
        if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS | CORE_MODOUT_FORCE_ON))
        {
          int prevIndex = CORE_MODOUT_SEG0 + wpc_data[WPC_ALPHAPOS] * 2 * 8;
          int newIndex  = CORE_MODOUT_SEG0 +     data               * 2 * 8;
          if (prevIndex != newIndex)
            for (int i = 0; i < 4; i++)
            {
              int offst = i == 0 ? 0 : i == 1 ? 8 : i == 2 ? 320 : 328;
              core_write_pwm_output_8b(newIndex  + offst, coreGlobals.binaryOutputState[(prevIndex + offst) >> 3]);
              core_write_pwm_output_8b(prevIndex + offst, 0);
            }
        }
      }
      break; /* just save position */
    case WPC_EXTBOARD2: /* WPC_ALPHA1LO */
      if (core_gameData->hw.gameSpecific1 & WPC_PH) { // PH: lamps 65 .. 128 (data is row of 2nd matrix)
        core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 64, wpc_data[WPC_LAMPCOLUMN], data, 8);
      }
      else if (wpc_modsol_aux_board == 2)
      {
        assert(CORE_FIRSTCUSTSOL == 51);
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 48, data << 2, 0xFC); // Write 50..55
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 56, data >> 6, 0x03); // Write 56..57
      }
      else if ((core_gameData->gen & GENWPC_HASDMD) == 0) // WPC_ALPHA1LO
      {
        wpclocals.alphaSeg[wpc_data[WPC_ALPHAPOS]].b.lo |= data;
        if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_ALPHASEGS | CORE_MODOUT_FORCE_ON))
          core_write_pwm_output_8b(CORE_MODOUT_SEG0 + wpc_data[WPC_ALPHAPOS] * 2 * 8, data);
      }
      break;
    case WPC_EXTBOARD3: /* WPC_ALPHA1HI */
      if (core_gameData->hw.gameSpecific1 & WPC_PH) { // PH: lamps 129 .. 192 (data is row of 3rd matrix)
        core_write_pwm_output_lamp_matrix(CORE_MODOUT_LAMP0 + 128, wpc_data[WPC_LAMPCOLUMN], data, 8);
      }
      if ((core_gameData->gen & GENWPC_HASDMD) == 0) // WPC_ALPHA1HI
      {
        wpclocals.alphaSeg[wpc_data[WPC_ALPHAPOS]].b.hi |= data;
        core_write_pwm_output_8b(CORE_MODOUT_SEG0 + (wpc_data[WPC_ALPHAPOS] * 2 + 1) * 8, data);
      }
      break;
    case WPC_SHIFTADRH:
    case WPC_SHIFTADRL:
    case WPC_SHIFTBIT:
    case WPC_SHIFTBIT2:
      break; /* just save value */
    case WPC_DIPSWITCH:
      //DBGLOG(("W:DIPSWITCH %x\n",data));
      break; /* just save value */
    case WPC_SOLENOID1:
      if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON))
      {
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 25 - 1, data, 0x0F); // 25..28 are standard solenoids
        // 4 additional GPIO (J122 / J123 / J124) which appear as LPDC outputs 37..40 in manuals (29..32 are used for J111 GPIO and GameOn)
        core_write_masked_pwm_output_8b(CORE_MODOUT_SOL0 + 33 - 1, data, 0xF0); // 37..40 are GPIO added in later generations
      }
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID2:
      if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON))
        core_write_pwm_output_8b(CORE_MODOUT_SOL0, data); // 1..8
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID3:
      if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON))
        core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 17 - 1, data); // 17..24
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
      data |= wpc_data[offset];
      if (core_gameData->hw.gameSpecific1 & WPC_CFTBL) // CFTBL chase lights
      {
         int chase_2b = ((coreGlobals.pulsedSolState >> 22) & 2) | ((coreGlobals.pulsedSolState >> 19) & 1); // 2 bit decoder => select one of the 4 chase light strings
         int chase_gi = ((wpclocals.conductingGITriacs & 1) ? 0x0F : 0x00) | ((wpclocals.conductingGITriacs & 8) ? 0xF0 : 0x00); // GI outputs
         wpclocals.conductingChaseLightTriacs |= chase_gi & (0x11 << chase_2b);
         core_write_pwm_output_8b(CORE_MODOUT_LAMP0 + 64, wpclocals.conductingChaseLightTriacs);
         coreGlobals.lampMatrix[8] = coreGlobals.tmpLampMatrix[8] = 0x11 << chase_2b;
      }
      break;
    case WPC_SOLENOID4:
      if (options.usemodsol & (CORE_MODOUT_ENABLE_PHYSOUT_SOLENOIDS | CORE_MODOUT_ENABLE_MODSOL | CORE_MODOUT_FORCE_ON))
        core_write_pwm_output_8b(CORE_MODOUT_SOL0 + 9 - 1, data); // 9..16
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
      data |= wpc_data[offset];
      break;
    case WPC_SND_S11_DATA1:
      //DBGLOG(("sdataX:%2x\n",data));
      if (core_gameData->gen & GEN_WPCALPHA_1) {
        sndbrd_0_data_w(0,data); sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
      }
      break;
    case WPC_SND_DATA:
      //DBGLOG(("sdata:%2x\n",data));
      if (sndbrd_0_type() != SNDBRD_S11CS)
        sndbrd_0_data_w(0,data);
      break;
    case WPC_SND_CTRL:
      //DBGLOG(("sctrl:%2x\n",data));
      if (sndbrd_0_type() == SNDBRD_S11CS)
        { sndbrd_0_data_w(0,data); sndbrd_0_ctrl_w(0,1); }
      else sndbrd_0_ctrl_w(0,data);
      break;
    case WPC_ZC_IRQ_ACK:
      // Bit 7: IRQ ACK
      if(data & 0x80) // IRQ acknowledge
        cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, CLEAR_LINE);
      // Bit 2: Periodic timer on IRQ enable
      //  Noop as it is checked when interrupt is raised
      // Bit 1(?): Watchdog reset (write only)
      //  Noop as we do not have a watchdog here :)
      break;
    case WPC_HIGHRESTIMER:
      // Ack previous high res timer FIRQ if any
      if (wpclocals.wpcFIRQ != 0) {
        wpclocals.wpcFIRQ = 0;
        update_firq();
      }
      // Eventually starts a new timer from the given value. The clock frequency is a wild guess as schematic has a 32.768 kHz oscillator 
      // and this makes the blinking in BOP service menu looks okish compared to this video: https://www.youtube.com/watch?v=wXY63U1mSZk.
      if (data > 0)
        timer_adjust(wpclocals.highres_timer, data / 32768., 0, 0.);
      else if (timer_enabled(wpclocals.highres_timer))
        timer_enable(wpclocals.highres_timer, 0);
      // if (wpc_data[offset] != 0 || data != 0) printf("%8.5f High res timer: %02x\n", timer_get_time(), data);
      break;
    case WPC_IRQACK:
      // cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, CLEAR_LINE);
      DBGLOG(("WPC_IRQACK. PC=%04x d=%02x\n",activecpu_get_pc(), data));
      break;
    case WPC_DMD_PAGE3000: /* set the page that is accessed by CPU at 0x3000 (WPC-95 only) */
      if (core_gameData->gen & (GEN_WPC95DCS | GEN_WPC95)) cpu_setbank(4, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_PAGE3200: /* set the page that is accessed by CPU at 0x3200 (WPC-95 only) */
      if (core_gameData->gen & (GEN_WPC95DCS | GEN_WPC95)) cpu_setbank(5, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_PAGE3400: /* set the page that is accessed by CPU at 0x3400 (WPC-95 only) */
      if (core_gameData->gen & (GEN_WPC95DCS | GEN_WPC95)) cpu_setbank(6, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_PAGE3600: /* set the page that is accessed by CPU at 0x3600 (WPC-95 only) */
      if (core_gameData->gen & (GEN_WPC95DCS | GEN_WPC95)) cpu_setbank(7, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_PAGE3800: /* set the page that is accessed by CPU at 0x3800 */
      cpu_setbank(2, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_PAGE3A00: /* set the page that is accessed by CPU at 0x3A00 */
      cpu_setbank(3, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200);
      break;
    case WPC_DMD_FIRQLINE: /* acknowledge raised DMD FIRQ if any, and set the line to generate the next FIRQ (0xFF to ack and disable) */
      //printf("%8.5f FIRQ ROW: %02x PC: %04x\n", timer_get_time(), data, activecpu_get_pc());
      if (dmdlocals.firq != 0) {
        dmdlocals.firq = 0;
        update_firq();
      }
      break;
    case WPC_DMD_SHOWPAGE: /* set the page that will be rasterized after next DMD vblank */
      //{ static double prev = 0.; printf("%8.5f Set page: %02x PC: %04x elapsed:%8.5f\n", timer_get_time(), data, activecpu_get_pc(), timer_get_time()-prev); prev = timer_get_time(); }
      break;
    case WPC_RTCHOUR:
    case WPC_RTCMIN:
      break;
    case WPC_PROTMEMCTRL:
      if (wpc_data[WPC_PROTMEM] == WPC_PROTMEMCODE)
        // not sure if this formula is correct
        wpclocals.memProtMask = ((UINT16)(core_revnyb(data & 0xf) + (data & 0xf0) + 0x10))<<8;
      break;
    case WPC_PROTMEM:
      break; // just save value
    case WPC_LED:
      wpclocals.diagnosticLed |= (data>>7);
      break;
    case WPC_PRINTDATA:
      break;
    case WPC_PRINTDATAX:
      if (data == 0) {
        if (wpc_printfile == NULL) {
          char filename[64];

          sprintf(filename,"%s.prt", Machine->gamedrv->name);
          wpc_printfile = mame_fopen(Machine->gamedrv->name,filename,FILETYPE_PRINTER,1);
          if (wpc_printfile == NULL) break;
        }
        mame_fwrite(wpc_printfile, &wpc_data[WPC_PRINTDATA], 1);
      }
      break;
    case WPC_SERIAL_DATA:
    case WPC_SERIAL_CTRL:
    case WPC_SERIAL_BAUD:
#ifdef PINMAME_HOST_UART
      if (pmoptions.serial_device != NULL) {
        uart_8251_write(offset, data);
      }
#endif
      break;
    default:
      DBGLOG(("wpc_w %4x %2x\n", offset+WPC_BASE, data));
      break;
  }
  wpc_data[offset] = data;
}
/*--------------------------
/ Protected memory
/---------------------------*/
static WRITE_HANDLER(wpc_ram_w) {
  if ((wpc_data[WPC_PROTMEM] == WPC_PROTMEMCODE) ||
      ((offset & wpclocals.memProtMask) != wpclocals.memProtMask))
    wpc_ram[offset] = data;
  else DBGLOG(("mem prot violation. PC=%04x a=%04x d=%02x\n",activecpu_get_pc(), offset, data));
}

/*--------------------------
/ Security chip simulation
/---------------------------*/
static int wpc_pic_r(void) {
  int ret = 0;
  if (wpclocals.pic.lastW == 0x0d)
    ret = wpclocals.pic.count;
  else if ((wpclocals.pic.lastW >= 0x16) && (wpclocals.pic.lastW <= 0x1f))
    ret = coreGlobals.swMatrix[wpclocals.pic.lastW-0x15];
  else if ((wpclocals.pic.lastW & 0xf0) == 0x70) {
    ret = wpclocals.pic.sData[wpclocals.pic.lastW & 0x0f];
    /* update serial number scrambler */
    wpclocals.pic.sNoS = ((wpclocals.pic.sNoS>>4) | (wpclocals.pic.lastW <<4)) & 0xff;
    wpclocals.pic.sData[5]  = (wpclocals.pic.sData[5]  ^ wpclocals.pic.sNoS) + wpclocals.pic.sData[13];
    wpclocals.pic.sData[13] = (wpclocals.pic.sData[13] + wpclocals.pic.sNoS) ^ wpclocals.pic.sData[5];
  }
  return ret;
}

static void wpc_pic_w(int data) {
  if (wpclocals.pic.codeW > 0) {
    if (wpclocals.pic.codeNo[3 - wpclocals.pic.codeW] != data)
      DBGLOG(("Wrong code %2x (expected %2x) sent to pic.",
               data, wpclocals.pic.codeNo[3 - wpclocals.pic.codeW]));
    wpclocals.pic.codeW -= 1;
  }
  else if (data == 0) {
    wpclocals.pic.sNoS = 0xa5;
    wpclocals.pic.sData[5]  = wpclocals.pic.sData[0] ^ wpclocals.pic.sData[15];
    wpclocals.pic.sData[13] = wpclocals.pic.sData[2] ^ wpclocals.pic.sData[12];
    wpclocals.pic.count = 0x20;
  }
  else if (data == 0x20) {
    wpclocals.pic.codeW = 3;
  }
  else if (data == 0x0d) {
    wpclocals.pic.count = (wpclocals.pic.count - 1) & 0x1f;
  }

  wpclocals.pic.lastW = data;
}

/*-------------------------
/  Generate IRQ interrupt
/--------------------------*/
static INTERRUPT_GEN(wpc_irq) {
  // Only raise line if periodic IRQ is enabled on WPC chip
  if (wpc_data[WPC_ZC_IRQ_ACK] & 0x04)
    cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, HOLD_LINE);
}

static SWITCH_UPDATE(wpc) {
#ifdef PROC_SUPPORT
  if (coreGlobals.p_rocEn) {
    procGetSwitchEvents();
  } else {
#endif
  if (inports) {
    coreGlobals.swMatrix[CORE_COINDOORSWCOL] = inports[WPC_COMINPORT] & 0xff;
    /*-- check standard keys --*/
    if (core_gameData->wpc.comSw.start)
      core_setSw(core_gameData->wpc.comSw.start,    inports[WPC_COMINPORT] & WPC_COMSTARTKEY);
    if (core_gameData->wpc.comSw.tilt)
      core_setSw(core_gameData->wpc.comSw.tilt,     inports[WPC_COMINPORT] & WPC_COMTILTKEY);
    if (core_gameData->wpc.comSw.sTilt)
      core_setSw(core_gameData->wpc.comSw.sTilt,    inports[WPC_COMINPORT] & WPC_COMSTILTKEY);
    if (core_gameData->wpc.comSw.coinDoor)
      core_setSw(core_gameData->wpc.comSw.coinDoor, inports[WPC_COMINPORT] & WPC_COMCOINDOORKEY);
    if (core_gameData->wpc.comSw.shooter)
      core_setSw(core_gameData->wpc.comSw.shooter,  inports[CORE_SIMINPORT] & SIM_SHOOTERKEY);
  }
#ifdef PROC_SUPPORT
  }
#endif
}

static WRITE_HANDLER(snd_data_cb) { // Pre DCS sound board A-12738 may generate FIRQ on data ready if W1 jumper is soldered
  if (wpclocals.sndFIRQ != 1) {
    wpclocals.sndFIRQ = 1;
    update_firq();
  }
}

static MACHINE_INIT(wpc) {
                                    /*128K  256K        512K        768K       1024K*/
  static const int romLengthMask[] = {0x07, 0x0f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f};
  const size_t romLength = memory_region_length(WPC_ROMREGION);

  memset(&wpclocals, 0, sizeof(wpclocals));

  wpclocals.highres_timer = timer_alloc(wpc_highres_timer);

  // map dmd banks to standard ram for games that don't use it
  cpu_setbank(4, memory_region(WPC_CPUREGION) + 0x3000);
  cpu_setbank(5, memory_region(WPC_CPUREGION) + 0x3200);
  cpu_setbank(6, memory_region(WPC_CPUREGION) + 0x3400);
  cpu_setbank(7, memory_region(WPC_CPUREGION) + 0x3600);

  switch (core_gameData->gen) {
    case GEN_WPCALPHA_1:
      sndbrd_0_init(SNDBRD_S11CS, 1, memory_region(S11CS_ROMREGION),NULL,NULL);
      break;
    case GEN_WPCALPHA_2:
    case GEN_WPCDMD:
    case GEN_WPCFLIPTRON:
      // Pre DCS sound board A-12738 may generate a FIRQ on data ready if W1 jumper is soldered, but so far, I didn't find any game with W1 soldered
      // and inspected gamecode only managed FIRQ coming from WPC or DMD. Moreover, if enabling, this would break the DMD timing as the gamecode
      // would mistakenly consider sound a FIRQ as a DMD FIRQ, breaking display during score in lots of games.
      sndbrd_0_init(SNDBRD_WPCS, 1, memory_region(WPCS_ROMREGION), NULL /*snd_data_cb*/, NULL);
      break;
    case GEN_WPCDCS:
    case GEN_WPCSECURITY:
      sndbrd_0_init(SNDBRD_DCS, 1, memory_region(DCS_ROMREGION),NULL,NULL);
      break;
    case GEN_WPC95:
      // WPC95 only controls 3 of the 5 Triacs, the other 2 are ALWAYS ON (power wired directly)
      //  We simulate this here by setting the bits to simulate full intensity immediately at power up.
      wpc_data[WPC_GILAMPS] = 0x18;
      coreGlobals.gi[WPC_N_GI-2] = 8;
      coreGlobals.gi[WPC_N_GI-1] = 8;
    case GEN_WPC95DCS:
      //Sound board initialization
      sndbrd_0_init(core_gameData->gen == GEN_WPC95DCS ? SNDBRD_DCS : SNDBRD_DCS95, 1, memory_region(DCS_ROMREGION),NULL,NULL);
  }

  // Init DMD PWM shading
  if (core_gameData->gen & (GEN_WPCDMD | GEN_WPCFLIPTRON | GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95DCS | GEN_WPC95))
  {
    const int isPH = (core_gameData->hw.gameSpecific1 & WPC_PH);
    core_dmd_pwm_init(&dmdlocals.pwm_state, 128, isPH ? 64 : 32, isPH ? CORE_DMD_PWM_FILTER_WPC_PH : CORE_DMD_PWM_FILTER_WPC, isPH ? CORE_DMD_PWM_COMBINER_SUM_2 : CORE_DMD_PWM_COMBINER_SUM_3);
    dmdlocals.pwm_state.revByte = 1;
  }

#ifdef PINMAME_HOST_UART
  if (pmoptions.serial_device != NULL) {
    uart_16c450_reset();
    uart_8251_reset();

    // use default baud rate; game will set a baud rate at startup
    if (uart_open(pmoptions.serial_device, 9600) < 0) {
      printf("serial_device: failed to open %s.\n", pmoptions.serial_device);
      pmoptions.serial_device = NULL;       // open failed, disable UART
    }
    else {
      printf("serial_device: WPC serial port redirected to %s.\n", pmoptions.serial_device);
    }
  }
#endif

  // Initialize outputs
  coreGlobals.nLamps = 64 + core_gameData->hw.lampCol * 8;
  core_set_pwm_output_type(CORE_MODOUT_LAMP0, coreGlobals.nLamps, CORE_MODOUT_BULB_44_18V_DC_WPC);
  coreGlobals.nSolenoids = CORE_FIRSTCUSTSOL - 1 + core_gameData->hw.custSol; // Auxiliary solenoid board adding 8 outputs are already included in the base solenoid span (see core_gelAllModSol) (WPC Fliptronics: TZ / WPC DCS: DM, IJ, STTNG / WPC Security : RS / WPC 95: NGG)
  core_set_pwm_output_type(CORE_MODOUT_SOL0, coreGlobals.nSolenoids, CORE_MODOUT_SOL_2_STATE);
  core_set_pwm_output_type(CORE_MODOUT_SOL0 + 29 -1, 3, CORE_MODOUT_PULSE); // GameOn/FastFlip and J111 GPIO
  if (core_gameData->gen & GENWPC_HASFLIPTRON)
  {
     coreGlobals.hasModulatedFlippers = TRUE;
     // We always use the 2 state integrator, eventually overriden by a flasher integrator below, as some tables use these outputs for solenoids (Medieval Madness for example) and a full review of all schematics with per table definition was not yet performed
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 45 - 1, 2, CORE_MODOUT_SOL_2_STATE); // LR flipper
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 47 - 1, 2, CORE_MODOUT_SOL_2_STATE); // LL flipper
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 33 - 1, 2, ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)) == 0) ? CORE_MODOUT_NONE : CORE_MODOUT_SOL_2_STATE); // UR flipper
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 35 - 1, 2, ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)) == 0) ? CORE_MODOUT_NONE : CORE_MODOUT_SOL_2_STATE); // UL flipper
  }
  coreGlobals.nGI = 5;
  core_set_pwm_output_type(CORE_MODOUT_GI0, coreGlobals.nGI, CORE_MODOUT_BULB_44_6_3V_AC);
  if (core_gameData->gen & (GEN_WPCALPHA_1 | GEN_WPCALPHA_2)) { // BOP, FH, HD alpahanumeric segments
    coreGlobals.nAlphaSegs = 40 * 16;
    core_set_pwm_output_led_vfd(CORE_MODOUT_SEG0, 16 * 16, 1, 16.f / 1.f);
    core_set_pwm_output_led_vfd(CORE_MODOUT_SEG0 + 20 * 16, 16 * 16, 1, 16.f / 1.f);
  }
  const struct GameDriver* rootDrv = Machine->gamedrv;
  while (rootDrv->clone_of && (rootDrv->clone_of->flags & NOT_A_DRIVER) == 0)
     rootDrv = rootDrv->clone_of;
  const char* const gn = rootDrv->name;
  // Missing definitions:
  // - Ticket Tac Toe
  // - Phantom Haus
  // - Rush
  if (strncasecmp(gn, "afm_", 4) == 0) { // Attack from Mars
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8, 16, CORE_MODOUT_LED); // Auxiliary LEDs driven through solenoids 37/38
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 39 - 1, 1, CORE_MODOUT_LED); // Xenon strobe light (crudely considered as a LED since it is meant to flicker)
  }
  else if (strncasecmp(gn, "bop_", 4) == 0) { // The Machine: Bride of the Pinbot
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
     // FIXME this is likely incorrect: I don't have the wiring for the helmet bulbs so they are defined like underpower #44 but this is very unlikely to be true (these are #555 but the power maybe from GI line 2 
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8, 16, CORE_MODOUT_BULB_44_5_7V_AC); // Helmet lights
  }
  else if (strncasecmp(gn, "br_", 3) == 0) { // Black Rose
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "cc_", 3) == 0) { // Cactus Canyon
     static const int flashers[] = { 18, 19, 20, 24, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "corv_", 5) == 0) { // Corvette
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "cp_", 3) == 0) { // The Champion Pub
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
     // Auxiliary LEDs driven through solenoids 37/38/39/40, only 24 (2x12 on each side) of the 32 outputs are used by the game (board A-21967)
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8, 32, CORE_MODOUT_LED);
  }
  else if (strncasecmp(gn, "cv_", 3) == 0) { // Cirqus Voltaire
     static const int flashers[] = { 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "cftbl_", 6) == 0) { // Creature From The Black Lagoon
     static const int flashers[] = { 2, 8, 9, 10, 11, 16, 17, 18, 19, 22, 25, 28, 33, 34, 35, 36 }; // 28 is hologram lamp, 33-36 are flashers inside backglass using Fliptronic upper flippers (not mentionned in manual)
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		 core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 64, 8, CORE_MODOUT_BULB_86_6_3V_AC); // chase lights (8 strings of #86 bulbs wired between GI and solenoids outputs through triacs and a 2 bit decoder)
  }
  else if (strncasecmp(gn, "congo_", 6) == 0) { // Congo
     static const int flashers[] = { 17, 18, 19, 20, 21, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "dh_", 3) == 0) { // Dirty Harry
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "dm_", 3) == 0) { // Demolition Man
     static const int flashers[] = { 17, 21, 22, 23, 24, 25, 26, 27, 28, 37 + 14, 38 + 14, 39 + 14, 40 + 14, 41 + 14, 42 + 14, 43 + 14, 44 + 14 }; // Aux Driver board
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "drac_", 5) == 0) { // Bram Stoker's Dracula
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 24 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC); // Backbox
  }
  else if (strncasecmp(gn, "dw_", 3) == 0) { // Doctor Who
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 23, 24 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "fh_", 3) == 0) { // Fun House
     static const int flashers[] = { 17, 18, 19, 20, 23, 24 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "fs_", 3) == 0) { // The Flintstones
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 6, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 24 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ft_", 3) == 0) { // Fish Tales
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 7, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "gi_", 3) == 0) { // Gilligan's Island
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "gw_", 3) == 0) { // High Speed II: The Getaway
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
  }
  else if ((strncasecmp(gn, "hd_", 3) == 0) // Harley Davidson
        || (strncasecmp(gn, "che_cho", 7) == 0)) { // Cheech & Chong: Road-Trip'pin
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "hshot_", 6) == 0) { // Hot Shot
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 7, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "hurr_", 5) == 0) { // Hurricane
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 12, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "i500_", 5) == 0) { // Indy 500
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 10, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ij_", 3) == 0) { // Indiana Jones
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 19 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 37 + 14 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC); // Aux Driver board
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 24 - 1, 1, CORE_MODOUT_LED); // Plane Guns LED
  }
  else if (strncasecmp(gn, "jb_", 3) == 0) { // Jack Bot
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 15 - 1, 10, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "jd_", 3) == 0) { // Judge Dredd
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "jm_", 3) == 0) { // Johnny Mnemonic
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "jy_", 3) == 0) { // Junk Yard
     static const int flashers[] = { 17, 18, 19, 20, 22, 23, 24, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "mb_", 3) == 0) { // Monster Bash
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 10, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "mm_", 3) == 0) { // Medieval Madness
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 23, 24, 25 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "nbaf_", 5) == 0) { // NBA Fast Break
     static const int flashers[] = { 17, 19, 20, 22, 23, 24 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "nf_", 3) == 0) { // No Fear
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 24 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ngg_", 4) == 0) { // No Good Goofers
     static const int flashers[] = { 17, 18, 19, 20, 21, 25, 26, 42 + 14, 43 + 14, 44 + 14, 45 + 14, 46 + 14, 47 + 14, 48 + 14, 49 + 14 }; // Aux Driver board
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "pop_", 4) == 0) { // Popeye Save The Earth
     static const int flashers[] = { 18, 19, 20, 21, 22, 23, 24, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "pz_", 3) == 0) { // Party Zone
     static const int flashers[] = { 17, 18, 19, 20, 21, 22, 25, 26, 27, 28 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "rs_", 3) == 0) { // Road Show
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 37 + 14 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC); // Aux Driver board
  }
  else if (strncasecmp(gn, "sc_", 3) == 0) { // Safe Cracker
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 8, CORE_MODOUT_BULB_89_20V_DC_WPC);
     // A-20909: 48 bulbs switched through solenoids 37..40, powered from GI string 2/4/5
     // FIXME not yet fully emulated (only the on/off switch is emulated, needing to be crossed with the GI string at the bulb level)
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8, 48, CORE_MODOUT_BULB_44_6_3V_AC);
  }
  else if (strncasecmp(gn, "sf_", 3) == 0) { // SlugFest
     static const int flashers[] = { 17, 18, 19, 20, 25, 26 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ss_", 3) == 0) { // Scared Stiff
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 12, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 35 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC); // Lower left/lower right flasher (use free upper flipper sol outputs)
     core_set_pwm_output_type(CORE_MODOUT_LAMP0 + 8 * 8, 16, CORE_MODOUT_LED); // 16 auxiliary Skull LEDs driven through solenoids 37/38 (only used in prototype 0.1, removed for later prototype and production builds)
  }
  else if (strncasecmp(gn, "sttng_", 6) == 0) { // Star Trek Next Generation
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 3, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 41 + 14 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC); // Aux Driver board
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 42 + 14 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "totan_", 6) == 0) { // Tales Of The Arabian Nights
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 2, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 7, CORE_MODOUT_BULB_906_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "taf_", 4) == 0) { // The Addams Family
     static const int flashers[] = { 17, 18, 19, 20, 21, 22 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     // Playfield magnets, they are pulsed with 64ms pulses over a 524ms period
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 16 - 1, 1, CORE_MODOUT_MAGNET);
     // core_set_pwm_output_type(CORE_MODOUT_SOL0 + 23 - 1, 2, CORE_MODOUT_MAGNET);
  }
  else if (strncasecmp(gn, "tafg_", 5) == 0) { // The Addams Family Gold Edition
     static const int flashers[] = { 17, 18, 19, 20, 21, 22 };
     for (int i = 0; i < sizeof(flashers) / sizeof(int); i++)
		core_set_pwm_output_type(CORE_MODOUT_SOL0 + flashers[i] - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "tom_", 4) == 0) { // Theatre of Magic
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 20 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 24 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ts_", 3) == 0) { // The Shadow
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 2, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 21 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 26 - 1, 3, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "tz_", 3) == 0) { // Twilight Zone
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 4, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 28 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 37 + 14 - 1, 5, CORE_MODOUT_BULB_906_20V_DC_WPC); // Aux Driver board
  }
  else if (strncasecmp(gn, "t2_", 3) == 0) { // Terminator 2
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 1, CORE_MODOUT_BULB_906_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 18 - 1, 10, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "wcs_", 4) == 0) { // World Cup Soccer
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 22 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 25 - 1, 4, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "wd_", 3) == 0) { // Who Dunnit
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 14 - 1, 1, CORE_MODOUT_BULB_89_20V_DC_WPC);
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 17 - 1, 5, CORE_MODOUT_BULB_89_20V_DC_WPC);
  }
  else if (strncasecmp(gn, "ww_", 3) == 0) { // White Water
     core_set_pwm_output_type(CORE_MODOUT_SOL0 + 15 - 1, 10, CORE_MODOUT_BULB_89_20V_DC_WPC);
     // FIXME The right bulb here would be #194 (12V x 0.27A = 4W, so more or less half of #89) wired directly through a 2N6427 between +12V DC and ground
     core_set_pwm_output_bulb(CORE_MODOUT_LAMP0 + 8 * 8, 16, BULB_89, 13.0, FALSE, 0.0, 1.0);
   }

  // Reset GI dimming timers
  core_zero_cross();
  for (int ii = 0,tmp= wpc_data[WPC_GILAMPS]; ii < 5; ii++, tmp >>= 1) {
     wpclocals.gi_on_time[ii] = wpclocals.lastACZeroCrossTimeStamp + (tmp & 0x01 ? 0. : 100.);
  }

  wpclocals.pageMask = romLengthMask[((romLength>>17)-1)&0x07];
  wpclocals.memProtMask = 0x1000;
  /* the non-paged ROM is at the end of the image. move it into the CPU region */
  memcpy(memory_region(WPC_CPUREGION) + 0x8000,
         memory_region(WPC_ROMREGION) + romLength - 0x8000, 0x8000);

  /*-- sync counter with vblank --*/
  wpclocals.interfaceUpdateCount = 1;

  coreGlobals.swMatrix[2] |= 0x08; /* Always closed switch */

  /*-- init security chip (if present) --*/
  if (core_gameData->gen & GENWPC_HASPIC)
    wpc_serialCnv(core_gameData->wpc.serialNo, wpclocals.pic.sData,
                  wpclocals.pic.codeNo);
  /* check flippers we have */
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)) == 0) /* No upper right flipper */
    wpclocals.nonFlipBits |= CORE_URFLIPSOLBITS;
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)) == 0) /* No upper left flipper */
    wpclocals.nonFlipBits |= CORE_ULFLIPSOLBITS;

  if (options.cheat && !(core_gameData->gen & GEN_WPCALPHA_1)) {
    /*-- speed up startup by disable checksum and also make tom_14h(c) work --*/
    *(memory_region(WPC_CPUREGION) + 0xffec) = 0x00;
    *(memory_region(WPC_CPUREGION) + 0xffed) = 0xff;
  }
}

static MACHINE_STOP(wpc) {
  sndbrd_0_exit();
  if (core_gameData->gen & (GEN_WPCDMD | GEN_WPCFLIPTRON | GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95DCS | GEN_WPC95))
      core_dmd_pwm_exit(&dmdlocals.pwm_state);
  if (wpc_printfile)
    { mame_fclose(wpc_printfile); wpc_printfile = NULL; }
}

/*-----------------------------------------------
/ Load/Save static ram
/ Most of the SRAM is cleared on startup but the
/ non-cleared part differs from game to game.
/ Better save it all
/ The RAM size was changed from 8K to 16K starting with
/ DCS generation
/-------------------------------------------------*/
static NVRAM_HANDLER(wpc) {
  core_nvram(file, read_or_write, wpc_ram,
             (core_gameData->gen & (GEN_WPCDCS | GEN_WPCSECURITY | GEN_WPC95 | GEN_WPC95DCS)) ? 0x3000 : 0x2000,0xff);
}

static void wpc_serialCnv(const char no[21], UINT8 pic[16], UINT8 code[3]) {
  int x;

  pic[10] = 0x12; /* whatever */
  pic[2]  = 0x34; /* whatever */
  x = (no[5] - '0') + 10*(no[8]-'0') + 100*(no[1]-'0');
  x += pic[10]*5;
  x *= 0x001bcd; /*   7117 = 11*647         */
  x += 0x01f3f0; /* 127984 = 2*2*2*2*19*421 */
  pic[1]  = x >> 16;
  pic[11] = x >> 8;
  pic[9]  = x;
  x = (no[7]-'0') + 10*(no[9]-'0') + 100*(no[0]-'0') + 1000*(no[18]-'0') + 10000*(no[2]-'0');
  x += 2*pic[10] + pic[2];
  x *= 0x0000107f; /*    4223 = 41*103     */
  x += 0x0071e259; /* 7463513 = 53*53*2657 */
  pic[7]  = x >> 24;
  pic[12] = x >> 16;
  pic[0]  = x >> 8;
  pic[8]  = x;
  x = (no[17]-'0') + 10*(no[6]-'0') + 100*(no[4]-'0') + 1000*(no[19]-'0');
  x += pic[2];
  x *= 0x000245; /*   581 = 7*83         */
  x += 0x003d74; /* 15732 =2*2*3*3*19*23 */
  pic[3]  = x >> 16;
  pic[14] = x >> 8;
  pic[6]  = x;
  x = ('9'-no[11]) + 10*('9'-no[12]) + 100*('9'-no[13]) + 1000*('9'-no[14]) + 10000*('9'-no[15]);
  pic[15] = x >> 8;
  pic[4]  = x;

  x = 100*(no[0]-'0') + 10*(no[1]-'0') + (no[2]-'0');
  x = (x >> 8) * (0x100*no[17] + no[19]) + (x & 0xff) * (0x100*no[18] + no[17]);
  code[0] = x >> 16;
  code[1] = x >> 8;
  code[2] = x;
}

/*----------------------------------*/
/* Williams WPC 128x32 DMD Handling */
/*----------------------------------*/
static VIDEO_START(wpc_dmd) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  return 0;
}

// The DMD controller constantly rasterizes the content of a page of RAM (while 
// CPU prepare next frame in another page).
// 
// It is driven by the main CPU clock at 2MHz and uses freq dividers to generate
// all timings. In the end, pages are rasterized at 2MHz / (128*32*2*2) = 122.07Hz
// where the first 2 dividers are the dot clock divider and the second is the 
// VBlank divider. All of this was deduced from the pre WPC-95 schematics.
// lucky1 measured on a real machine 122.1Hz which validates it.
//
// WPC games uses either a 2 or 3 frame PWM pattern to create 0/50/100 or 0/33/66/100 shades.
// For Phantom Haus, the DMD is bigger with 64 rows instead of 32, therefore
// the rasterizer timings are 2MHz / (128*64*2*2) = 61.04Hz. The PWM pattern 
// used is therefore limited to 2 frames (30.5Hz) with 0/50/100 shades to avoid 
// flickering.
// 
// CPU may ask the DMD board to raise FIRQ when a given row is reached.
// The FIRQ is then acked (pulled down) by writing again the requested FIRQ row 
// to the corresponding register (game code use 0xFF to disables DMD FIRQ).
static void wpc_dmd_hsync(int param) {
  dmdlocals.row = (dmdlocals.row + 1) % dmdlocals.pwm_state.height; // FIXME Phantom Haus uses the same AV card than other WPC95 but with a 64 row display, therefore the CPU must tell the rasterizer that it is 64 row high somewhere we don't know
  if (dmdlocals.row == 0) { // VSYNC
    // Rasterize next page (latched while rasterizing the previous page)
    const int rasterizedPage = wpc_data[WPC_DMD_SHOWPAGE] & 0x0f;
    //printf("%8.5f Rnd page: %02x\n", timer_get_time(), rasterizedPage);
    core_dmd_submit_frame(&dmdlocals.pwm_state, memory_region(WPC_DMDREGION) + rasterizedPage * dmdlocals.pwm_state.rawFrameSize, 1);
    #ifdef PROC_SUPPORT
      if (coreGlobals.p_rocEn) /* looks like P-ROC uses the last 3 subframes sent rather than the first 3 */
        procFillDMDSubFrame(dmd_state->frame_index % 3, memory_region(WPC_DMDREGION) + rasterizedPage * dmdlocals.pwm_state.rawFrameSize, dmdlocals.pwm_state.rawFrameSize);
      /* Don't explicitly update the DMD from here. The P-ROC code will update after the next DMD event. */
    #endif
  }
  if (dmdlocals.row == wpc_data[WPC_DMD_FIRQLINE] && dmdlocals.firq != 1) {
    //printf("%8.5f FIRQ at row %02x\n", timer_get_time(), dmdlocals.row);
    dmdlocals.firq = 1;
    update_firq();
  }
}

int wpcdmd_update(int height, struct mame_bitmap* bitmap, const struct rectangle* cliprect, const struct core_dispLayout* layout) {
  core_dmd_video_update(bitmap, cliprect, layout, &dmdlocals.pwm_state);
  return 0;
}

PINMAME_VIDEO_UPDATE(wpcdmd_update32) { return wpcdmd_update(32, bitmap, cliprect, layout); }
PINMAME_VIDEO_UPDATE(wpcdmd_update64) { return wpcdmd_update(64, bitmap, cliprect, layout); }
