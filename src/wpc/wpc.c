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

#define PRINT_GI_DATA	   0 /* printf the GI Data for debugging purposes   */
#define DEBUG_GI		   0 /* debug GI code - more printf stuff basically */

#define WPC_VBLANKDIV      4 /* How often to check the DMD FIRQ interrupt */
/*-- no of DMD frames to add together to create shades --*/
/*-- (hardcoded, do not change)                        --*/
#define DMD_FRAMES         3

/*-- Smoothing values --*/
#define WPC_SOLSMOOTH      4 /* Smooth the Solenoids over this numer of VBLANKS */
#define WPC_LAMPSMOOTH     2 /* Smooth the lamps over this number of VBLANKS */
#define WPC_DISPLAYSMOOTH  2 /* Smooth the display over this number of VBLANKS */

/*-- IRQ frequence, most WPC functions are performed at 1/16 of this frequency --*/
#define WPC_IRQFREQ      976 /* IRQ Frequency-Timed by JD*/

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
static INTERRUPT_GEN(wpc_vblank);
static INTERRUPT_GEN(wpc_irq);

/*-- PIC security chip emulation --*/
static int  wpc_pic_r(void);
static void wpc_pic_w(int data);
static void wpc_serialCnv(const char no[21], UINT8 schip[16], UINT8 code[3]);
/*-- DMD --*/
static VIDEO_START(wpc_dmd);
PINMAME_VIDEO_UPDATE(wpcdmd_update);
/*-- protected memory --*/
static WRITE_HANDLER(wpc_ram_w);

/*-- misc. --*/
static MACHINE_INIT(wpc);
static MACHINE_STOP(wpc);
static NVRAM_HANDLER(wpc);
static SWITCH_UPDATE(wpc);

/*------------------------
/  DMD display registers
/-------------------------*/
#define DMD_PAGE3000    (0x3fb9 - WPC_BASE)
#define DMD_PAGE3200    (0x3fb8 - WPC_BASE)
#define DMD_PAGE3400    (0x3fbb - WPC_BASE)
#define DMD_PAGE3600    (0x3fba - WPC_BASE)
#define DMD_PAGE3A00    (0x3fbc - WPC_BASE)
#define DMD_PAGE3800    (0x3fbe - WPC_BASE)
#define DMD_VISIBLEPAGE (0x3fbf - WPC_BASE)
#define DMD_FIRQLINE    (0x3fbd - WPC_BASE)

/*---------------------
/  Global variables
/---------------------*/
UINT8 *wpc_data;     /* WPC registers */
const struct core_dispLayout wpc_dispAlpha[] = {
  DISP_SEG_16(0,CORE_SEG16R),DISP_SEG_16(1,CORE_SEG16R),{0}
};
const struct core_dispLayout wpc_dispDMD[] = {
  {0,0,32,128,CORE_DMD,(void *)wpcdmd_update}, {0}
};

/*------------------
/  Local variables
/------------------*/
static struct {
  UINT16  memProtMask;
  UINT32  solData;        /* current value of solenoids 1-28 */
  UINT8   solFlip, solFlipPulse;  /* current value of flipper solenoids */
  UINT8   nonFlipBits;    /* flipper solenoids not used for flipper (smoothed) */
  int  vblankCount;                   /* vblank interrupt counter */
  core_tSeg alphaSeg;
  struct {
    UINT8 sData[16];
    UINT8 codeNo[3];
    UINT8 lastW;             /* last written command */
    UINT8 sNoS;              /* serial number scrambler */
    UINT8 count;
    int   codeW;
  } pic;
  int pageMask;            /* page handling */
  int firqSrc;             /* source of last firq */
  int diagnostic;
  int zc;						/* zero cross flag */
  int gi_irqcnt;                /* Count IRQ occurrences for GI Dimming */
  int gi_active[CORE_MAXGI];    /* Used to check if GI string is accessed at all */
} wpclocals;

static struct {
  UINT8 *DMDFrames[DMD_FRAMES];
  int    nextDMDFrame;
} dmdlocals;

/*-- pointers --*/
static mame_file *wpc_printfile = NULL;
static UINT8 *wpc_ram = NULL;

/*---------------------------
/  Memory map for CPU board
/----------------------------*/
static MEMORY_READ_START(wpc_readmem)
  { 0x0000, 0x2fff, MRA_RAM },
  { 0x3000, 0x31ff, MRA_BANK4 },  /* DMD */
  { 0x3200, 0x33ff, MRA_BANK5 },  /* DMD */
  { 0x3400, 0x35ff, MRA_BANK6 },  /* DMD */
  { 0x3600, 0x37ff, MRA_BANK7 },  /* DMD */
  { 0x3800, 0x39ff, MRA_BANK2 },  /* DMD */
  { 0x3A00, 0x3bff, MRA_BANK3 },  /* DMD */
  { 0x3c00, 0x3faf, MRA_RAM },
  { 0x3fb0, 0x3fff, wpc_r },
  { 0x4000, 0x7fff, MRA_BANK1 },
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(wpc_writemem)
  { 0x0000, 0x2fff, wpc_ram_w, &wpc_ram },
  { 0x3000, 0x31ff, MWA_BANK4 },  /* DMD */
  { 0x3200, 0x33ff, MWA_BANK5 },  /* DMD */
  { 0x3400, 0x35ff, MWA_BANK6 },  /* DMD */
  { 0x3600, 0x37ff, MWA_BANK7 },  /* DMD */
  { 0x3800, 0x39ff, MWA_BANK2 },  /* DMD */
  { 0x3A00, 0x3bff, MWA_BANK3 },  /* DMD */
  { 0x3c00, 0x3faf, MWA_RAM },
  { 0x3fb0, 0x3fff, wpc_w, &wpc_data },
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

// convert lamp and switch numbers
// both use column*10+row
// convert to 0-63 (+8)
// i.e. 11=8,12=9,21=16
static int wpc_sw2m(int no) { return (no/10)*8+(no%10-1); }
static int wpc_m2sw(int col, int row) { return col*10+row+1; }

//Set Zero Cross flag (it's reset when read)
static void wpc_zc(int data) {
	wpclocals.zc = 1;
}

/*-----------------
/  Machine drivers
/------------------*/
static MACHINE_DRIVER_START(wpc)
  MDRV_IMPORT_FROM(PinMAME)
  MDRV_CORE_INIT_RESET_STOP(wpc,NULL,wpc)
  MDRV_CPU_ADD(M6809, 2000000)
  MDRV_CPU_MEMORY(wpc_readmem, wpc_writemem)
  MDRV_CPU_VBLANK_INT(wpc_vblank, WPC_VBLANKDIV)
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
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_dmdS)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_IMPORT_FROM(wmssnd_wpcs)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_dcsS)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_IMPORT_FROM(wmssnd_dcs1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(wpc_95S)
  MDRV_IMPORT_FROM(wpc)
  MDRV_VIDEO_START(wpc_dmd)
  MDRV_IMPORT_FROM(wmssnd_dcs2)
MACHINE_DRIVER_END

/*--------------------------------------------------------------
/ This is generated WPC_VBLANKDIV times per frame
/ = every 32/WPC_VBLANKDIV lines.
/ Generate a FIRQ if it matches the DMD line
/ Also do the smoothing of the solenoids and lamps
/--------------------------------------------------------------*/
static INTERRUPT_GEN(wpc_vblank) {
  wpclocals.vblankCount = (wpclocals.vblankCount+1) % 16;

  if (core_gameData->gen & GENWPC_HASDMD) {
    /*-- check if the DMD line matches the requested interrupt line */
    if ((wpclocals.vblankCount % WPC_VBLANKDIV) == (wpc_data[DMD_FIRQLINE]*WPC_VBLANKDIV/32))
      wpc_firq(TRUE, WPC_FIRQ_DMD);
    if ((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) {
      /*-- This is the real VBLANK interrupt --*/
      dmdlocals.DMDFrames[dmdlocals.nextDMDFrame] = memory_region(WPC_DMDREGION)+ (wpc_data[DMD_VISIBLEPAGE] & 0x0f) * 0x200;
      dmdlocals.nextDMDFrame = (dmdlocals.nextDMDFrame + 1) % DMD_FRAMES;
    }
  }

  /*--------------------------------------------------------
  /  Most solonoids don't have a holding coil so the software
  /  simulates it by pulsing the power to the main (and only) coil.
  /  (I assume this is why the Auto Fire diverter in TZ seems to flicker.)
  /  I simulate the coil position by looking over WPC_SOLSMOOTH vblanks
  /  and only turns the solenoid off if it has not been pulsed
  /  during that time.
  /-------------------------------------------------------------*/
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_SOLSMOOTH)) == 0) {
    coreGlobals.solenoids = (wpc_data[WPC_SOLENOID1] << 24) |
                            (wpc_data[WPC_SOLENOID3] << 16) |
                            (wpc_data[WPC_SOLENOID4] <<  8) |
                             wpc_data[WPC_SOLENOID2];

    wpc_data[WPC_SOLENOID1] = wpc_data[WPC_SOLENOID2] = 0;
    wpc_data[WPC_SOLENOID3] = wpc_data[WPC_SOLENOID4] = 0;
    // Move top 3 GI to solenoids -> gameOn = sol31
    coreGlobals.solenoids2 = (wpc_data[WPC_GILAMPS] & 0xe0)<<3;
    if (core_gameData->gen & GENWPC_HASFLIPTRON) {
      coreGlobals.solenoids2 |= wpclocals.solFlip;
      wpclocals.solFlip = wpclocals.solFlipPulse;
    }
  }
  else if (((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) &&
           (core_gameData->gen & GENWPC_HASFLIPTRON)) {
    coreGlobals.solenoids2 = (coreGlobals.solenoids2 & (0xffffff00 | wpclocals.nonFlipBits)) | wpclocals.solFlip;
    wpclocals.solFlip = (wpclocals.solFlip & wpclocals.nonFlipBits) | wpclocals.solFlipPulse;
  }

  /*--------------------------------------------------------
  / The lamp circuit pulses the power to the lamps one column
  / at a time. By only reading the value every WPC_LAMPSMOOTH
  / vblank we get a steady light.
  /
  / Williams changed the software for the lamphandling at some stage
  / earlier code (TZ 9.2)   newer code (TZ 9.4)
  / ------------                    -----------
  / Activate Column             Activate rows
  / Activate rows               Activate column
  / wait                        wait
  / Deactivate rows             repeat for next column
  / repeat for next column
  / ..
  / For the game it doesn't really matter but it confused me when
  / the lamp code here worked for 9.2 but not in 9.4.
  /-------------------------------------------------------------*/
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_LAMPSMOOTH)) == 0) {
    memcpy(coreGlobals.lampMatrix, coreGlobals.tmpLampMatrix, sizeof(coreGlobals.tmpLampMatrix));
    memset(coreGlobals.tmpLampMatrix, 0, sizeof(coreGlobals.tmpLampMatrix));
  }
  if ((wpclocals.vblankCount % (WPC_VBLANKDIV*WPC_DISPLAYSMOOTH)) == 0) {
    if ((core_gameData->gen & GENWPC_HASDMD) == 0) {
      memcpy(coreGlobals.segments, wpclocals.alphaSeg, sizeof(coreGlobals.segments));
      coreGlobals.segments[15].w &= ~0x8080; coreGlobals.segments[35].w &= ~0x8080;
      memset(wpclocals.alphaSeg, 0, sizeof(wpclocals.alphaSeg));
    }
    coreGlobals.diagnosticLed = wpclocals.diagnostic;
    wpclocals.diagnostic = 0;

	//Display status of GI strings
	#if PRINT_GI_DATA
	{
		int i;
		for(i=0;i<CORE_MAXGI;i++)
			printf("GI[%d]=%d ",i,coreGlobals.gi[i]);
		printf("\n");
	}
	#endif
  }

  /*------------------------------
  /  Update switches every vblank
  /-------------------------------*/
  if ((wpclocals.vblankCount % WPC_VBLANKDIV) == 0) /*-- update switches --*/
    core_updateSw((core_gameData->gen & GENWPC_HASFLIPTRON) ? TRUE : (wpc_data[WPC_GILAMPS] & 0x80));
}

/* The FIRQ line is wired between the WPC chip and all external I/Os (sound) */
/* The DMD firq must be generated via the WPC but I don't how. */
void wpc_firq(int set, int src) {
  if (set)
    wpclocals.firqSrc |= src;
  else
    wpclocals.firqSrc &= ~src;
  cpu_set_irq_line(WPC_CPUNO, M6809_FIRQ_LINE, wpclocals.firqSrc ? HOLD_LINE : CLEAR_LINE);
}

/*----------------------
/ Emulate the WPC chip
/-----------------------*/
READ_HANDLER(wpc_r) {
  switch (offset) {
    case WPC_FLIPPERS: /* Flipper switches */
      if ((core_gameData->gen & GENWPC_HASWPC95) == 0)
        return ~coreGlobals.swMatrix[CORE_FLIPPERSWCOL];
      break;
    case WPC_FLIPPERSW95:
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
      break;
    case WPC_SHIFTADRL:
      return (wpc_data[WPC_SHIFTADRL] + (wpc_data[WPC_SHIFTBIT]>>3)) & 0xff;
    case WPC_SHIFTBIT:
    case WPC_SHIFTBIT2:
      return 1<<(wpc_data[offset] & 0x07);
    case WPC_FIRQSRC: /* FIRQ source */
      //DBGLOG(("R:FIRQSRC\n"));
      return (wpclocals.firqSrc & WPC_FIRQ_DMD) ? 0x00 : 0x80;
    case WPC_DIPSWITCH:
      return core_getDip(0);
    case WPC_RTCHOUR: {
      // Don't know how the clock works.
      // This hack by JD puts right values into the memory locations
      UINT8 *timeMem = wpc_ram + 0x1800;
      UINT16 checksum = 0;
      time_t now;
      struct tm *systime;

      time(&now);
      systime = localtime(&now);
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
      time_t now;
      struct tm *systime;
      time(&now);
      systime = localtime(&now);
      return (systime->tm_min);
    }
    case WPC_WATCHDOG:
		//Zero cross detection flag is read from Bit 8.
		wpc_data[offset] = (wpclocals.zc<<7) | (wpc_data[offset] & 0x7f);

		//Track when Zero Cross occurred and reset gi irq counting
		if(wpclocals.zc)
		{
			wpclocals.gi_irqcnt = 0;

			#if DEBUG_GI
			printf("Zero Cross!\n");
			#endif
		}

		//Reset flag now that it's been read.
		wpclocals.zc = 0;
      break;
    case WPC_SOUNDIF:
      return sndbrd_0_data_r(0);
    case WPC_SOUNDBACK:
      if (core_gameData->gen & (GEN_WPCALPHA_2|GEN_WPCDMD|GEN_WPCFLIPTRON))
        wpc_firq(FALSE, WPC_FIRQ_SOUND); // Ack FIRQ on read?
      return sndbrd_0_ctrl_r(0);
    case WPC_PRINTBUSY:
      return 0;
    case DMD_VISIBLEPAGE:
      break;
    case DMD_PAGE3000:
    case DMD_PAGE3200:
    case DMD_PAGE3400:
    case DMD_PAGE3600:
    case DMD_PAGE3800:
    case DMD_PAGE3A00:
      return 0; /* these can't be read */
    case DMD_FIRQLINE:
      return (wpclocals.firqSrc & WPC_FIRQ_DMD) ? 0x80 : 0x00;
    default:
      DBGLOG(("wpc_r %4x\n", offset+WPC_BASE));
      break;
  }
  return wpc_data[offset];
}

WRITE_HANDLER(wpc_w) {
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
    case WPC_FLIPPERS: /* Flipper coils */
      if ((core_gameData->gen & GENWPC_HASWPC95) == 0) {
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = ~data;
      }
      break;
    case WPC_FLIPPERCOIL95:
      if (core_gameData->gen & GENWPC_HASWPC95) {
        wpclocals.solFlip &= wpclocals.nonFlipBits;
        wpclocals.solFlip |= wpclocals.solFlipPulse = data;
      }
      else if ((core_gameData->gen & GENWPC_HASDMD) == 0)
        wpclocals.alphaSeg[20+wpc_data[WPC_ALPHAPOS]].b.lo |= data;
      break;
    case WPC_LAMPROW: /* row and column can be written in any order */
      core_setLamp(coreGlobals.tmpLampMatrix,wpc_data[WPC_LAMPCOLUMN],data);
      break;
    case WPC_LAMPCOLUMN: /* row and column can be written in any order */
      core_setLamp(coreGlobals.tmpLampMatrix,data,wpc_data[WPC_LAMPROW]);
      break;
    case WPC_SWCOLSELECT:
      if (core_gameData->gen & GENWPC_HASPIC)
        wpc_pic_w(data);
      break;
    case WPC_GILAMPS: {
      int ii, tmp, gi_bit;

	  #if DEBUG_GI
	  printf("%8x: GI_W: %x\n",activecpu_get_pc(),data);
	  #endif

	  //WPC95 only controls 3 of the 5 Triacs, the other 2 are ALWAYS ON (power wired directly)
	  //  We simulate this here by forcing the bits on
	  if (core_gameData->gen & GEN_WPC95)
		  data = (data & 0xe7) | 0x18;

     //Loop over each GI Triac Bit
      for (ii = 0,tmp=data; ii < CORE_MAXGI; ii++, tmp >>= 1) {
        //If Bit is set, Triac is turned on.
        gi_bit = tmp & 0x01;
        if (gi_bit) {
          if (wpclocals.gi_active[ii] == 2 + core_gameData->hw.gameSpecific1) { // if the bit was set last time as well, it's driven continuously.
            coreGlobals.gi[ii] = 8; // seemingly no way to discern levels 7 & 8???
          } else {
            coreGlobals.gi[ii] = wpclocals.gi_irqcnt > 7 ? 0 : 7 - wpclocals.gi_irqcnt;
          }
          wpclocals.gi_active[ii] = 2 + core_gameData->hw.gameSpecific1;
        } else {
          if (wpclocals.gi_active[ii]) {
            wpclocals.gi_active[ii]--;
          } else { // wait two cycles before turning off for smoothing
            coreGlobals.gi[ii] = 0;
          }
        }
      }
      break;
    }
    case WPC_EXTBOARD1: /* WPC_ALPHAPOS */
      break; /* just save position */
    case WPC_EXTBOARD2: /* WPC_ALPHA1 */
      if ((core_gameData->gen & GENWPC_HASDMD) == 0)
        wpclocals.alphaSeg[wpc_data[WPC_ALPHAPOS]].b.lo |= data;
      break;
    case WPC_EXTBOARD3:
      if ((core_gameData->gen & GENWPC_HASDMD) == 0)
        wpclocals.alphaSeg[wpc_data[WPC_ALPHAPOS]].b.hi |= data;
      break;
    /* case WPC_EXTBOARD4: */
    case WPC_EXTBOARD5:
      if ((core_gameData->gen & GENWPC_HASDMD) == 0)
        wpclocals.alphaSeg[20+wpc_data[WPC_ALPHAPOS]].b.hi |= data;
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
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0x00FFFFFF) | (data<<24);
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID2:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFFFF00) | data;
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID3:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFF00FFFF) | (data<<16);
      data |= wpc_data[offset];
      break;
    case WPC_SOLENOID4:
      coreGlobals.pulsedSolState = (coreGlobals.pulsedSolState & 0xFFFF00FF) | (data<<8);
      data |= wpc_data[offset];
      break;
    case 0x3fd1-WPC_BASE:
      //DBGLOG(("sdataX:%2x\n",data));
      if (core_gameData->gen & GEN_WPCALPHA_1) {
        sndbrd_0_data_w(0,data); sndbrd_0_ctrl_w(0,0); sndbrd_0_ctrl_w(0,1);
      }
      break;
    case WPC_SOUNDIF:
      //DBGLOG(("sdata:%2x\n",data));
      sndbrd_0_data_w(0,data);
      if (sndbrd_0_type() == SNDBRD_S11CS) sndbrd_0_ctrl_w(0,0);
      break;
    case WPC_SOUNDBACK:
      //DBGLOG(("sctrl:%2x\n",data));
      if (sndbrd_0_type() == SNDBRD_S11CS)
        { sndbrd_0_data_w(0,data); sndbrd_0_ctrl_w(0,1); }
      else sndbrd_0_ctrl_w(0,data);
      break;
    case WPC_WATCHDOG:
	    //Increment irq count - This is the best way to know an IRQ was serviced as this register is written immediately during the IRQ code.
	    //Only do this if bit 8 is set, as WW_L5 sometimes writes 0x06 here during the interrupt code.
		if(data & 0x80)
		{
			wpclocals.gi_irqcnt++;
			//Clear the IRQ now
		  cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, CLEAR_LINE);
		}
      break;
    case WPC_FIRQSRC:
      /* CPU writes here after a non-dmd firq. Don't know what happens */
      break;
    case WPC_IRQACK:
//      cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, CLEAR_LINE);
      DBGLOG(("WPC_IRQACK. PC=%04x d=%02x\n",activecpu_get_pc(), data));
      break;
    case DMD_PAGE3000: /* set the page that is visible at 0x3000 */
      cpu_setbank(4, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3200: /* set the page that is visible at 0x3200 */
      cpu_setbank(5, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3400: /* set the page that is visible at 0x3400 */
      cpu_setbank(6, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3600: /* set the page that is visible at 0x3600 */
      cpu_setbank(7, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3800: /* set the page that is visible at 0x3800 */
      cpu_setbank(2, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_PAGE3A00: /* set the page that is visible at 0x3A00 */
      cpu_setbank(3, memory_region(WPC_DMDREGION) + (data & 0x0f) * 0x200); break;
    case DMD_FIRQLINE: /* set the line to generate FIRQ at. */
      wpc_firq(FALSE, WPC_FIRQ_DMD);
      break;
    case DMD_VISIBLEPAGE: /* set the visible page */
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
      wpclocals.diagnostic |= (data>>7);
      break;
    case WPC_PRINTDATA:
      break;
    case WPC_PRINTDATAX:
      if (data == 0) {
        if (wpc_printfile == NULL) {
          char filename[13];

          sprintf(filename,"%s.prt", Machine->gamedrv->name);
          wpc_printfile = mame_fopen(Machine->gamedrv->name,filename,FILETYPE_PRINTER,1);
          if (wpc_printfile == NULL) break;
        }
        mame_fwrite(wpc_printfile, &wpc_data[WPC_PRINTDATA], 1);
      }
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
  cpu_set_irq_line(WPC_CPUNO, M6809_IRQ_LINE, HOLD_LINE);
}

static SWITCH_UPDATE(wpc) {
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
}

static WRITE_HANDLER(snd_data_cb) { // WPCS sound generates FIRQ on reply
  wpc_firq(TRUE, WPC_FIRQ_SOUND);
}

static MACHINE_INIT(wpc) {
                              /*128K  256K        512K        768K       1024K*/
  static const int romLengthMask[] = {0x07, 0x0f, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x3f};
  int romLength = memory_region_length(WPC_ROMREGION);

  memset(&wpclocals, 0, sizeof(wpclocals));

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
      sndbrd_0_init(SNDBRD_WPCS, 1, memory_region(WPCS_ROMREGION),snd_data_cb,NULL);
      break;
    case GEN_WPCDCS:
    case GEN_WPCSECURITY:
      sndbrd_0_init(SNDBRD_DCS, 1, memory_region(DCS_ROMREGION),NULL,NULL);
      break;
    case GEN_WPC95:
	  //WPC95 only controls 3 of the 5 Triacs, the other 2 are ALWAYS ON (power wired directly)
	  //  We simulate this here by setting the bits to simulate full intensity immediately at power up.
	  coreGlobals.gi[CORE_MAXGI-2] = 8;
	  coreGlobals.gi[CORE_MAXGI-1] = 8;
    case GEN_WPC95DCS:
	  //Sound board initialization
      sndbrd_0_init(core_gameData->gen == GEN_WPC95DCS ? SNDBRD_DCS : SNDBRD_DCS95, 1, memory_region(DCS_ROMREGION),NULL,NULL);
  }

  wpclocals.pageMask = romLengthMask[((romLength>>17)-1)&0x07];
  wpclocals.memProtMask = 0x1000;
  /* the non-paged ROM is at the end of the image. move it into the CPU region */
  memcpy(memory_region(WPC_CPUREGION) + 0x8000,
         memory_region(WPC_ROMREGION) + romLength - 0x8000, 0x8000);

  /*-- sync counter with vblank --*/
  wpclocals.vblankCount = 1;

  coreGlobals.swMatrix[2] |= 0x08; /* Always closed switch */

  /*-- init security chip (if present) --*/
  if ((core_gameData->gen & GENWPC_HASPIC) &&
      (core_gameData->wpc.serialNo))
    wpc_serialCnv(core_gameData->wpc.serialNo, wpclocals.pic.sData,
                  wpclocals.pic.codeNo);
  /* check flippers we have */
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UR)) == 0) /* No upper right flipper */
    wpclocals.nonFlipBits |= CORE_URFLIPSOLBITS;
  if ((core_gameData->hw.flippers & FLIP_SOL(FLIP_UL)) == 0) /* No upper left flipper */
    wpclocals.nonFlipBits |= CORE_ULFLIPSOLBITS;

  if (options.cheat && !(core_gameData->gen & GEN_WPCALPHA_1)) {
    /*-- speed up startup by disable checksum --*/
    *(memory_region(WPC_CPUREGION) + 0xffec) = 0x00;
    *(memory_region(WPC_CPUREGION) + 0xffed) = 0xff;
  }
}

static MACHINE_STOP(wpc) {
  sndbrd_0_exit();
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
  UINT8 *RAM = memory_region(WPC_DMDREGION);
  int ii;

  for (ii = 0; ii < DMD_FRAMES; ii++)
    dmdlocals.DMDFrames[ii] = RAM;
  dmdlocals.nextDMDFrame = 0;
  return 0;
}

//static VIDEO_UPDATE(wpc_dmd) {
PINMAME_VIDEO_UPDATE(wpcdmd_update) {
  tDMDDot dotCol;
  int ii,jj,kk;

  /* Create a temporary buffer with all pixels */
  for (kk = 0, ii = 1; ii < 33; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 16; jj++) {
      /* Intensity depends on how many times the pixel */
      /* been on in the last 3 frames                  */
      unsigned int intens1 = ((dmdlocals.DMDFrames[0][kk] & 0x55) +
                              (dmdlocals.DMDFrames[1][kk] & 0x55) +
                              (dmdlocals.DMDFrames[2][kk] & 0x55));
      unsigned int intens2 = ((dmdlocals.DMDFrames[0][kk] & 0xaa) +
                              (dmdlocals.DMDFrames[1][kk] & 0xaa) +
                              (dmdlocals.DMDFrames[2][kk] & 0xaa));

      *line++ = (intens1)    & 0x03;
      *line++ = (intens2>>1) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2>>3) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>5) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>7) & 0x03;
      kk +=1;
    }
    *line = 0; /* to simplify antialiasing */
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
