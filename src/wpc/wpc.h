// license:BSD-3-Clause

#ifndef INC_WPC
#define INC_WPC
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

/*-- Common Inports for WPCGames --*/
#define WPC_COMPORTS \
  PORT_START /* 0 */ \
    /* These go into column 0 */ \
    COREPORT_BITDEF(  0x0001, IPT_COIN1,        IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0002, IPT_COIN2,        IP_KEY_DEFAULT) \
    COREPORT_BITDEF(  0x0004, IPT_COIN3,        KEYCODE_3) \
    COREPORT_BITDEF(  0x0008, IPT_COIN4,        KEYCODE_4) \
    COREPORT_BIT(     0x0010, "Enter",          KEYCODE_7) \
    COREPORT_BIT(     0x0020, "Up",             KEYCODE_8) \
    COREPORT_BIT(     0x0040, "Down",           KEYCODE_9) \
    COREPORT_BIT(     0x0080, "Escape",         KEYCODE_0) \
    /* Common switches */ \
    COREPORT_BITTOG(  0x0100, "Coin Door",      KEYCODE_END)  \
    COREPORT_BITDEF(  0x0200, IPT_START1,       IP_KEY_DEFAULT)  \
    COREPORT_BITDEF(  0x0400, IPT_TILT,         KEYCODE_INSERT)  \
    COREPORT_BIT(     0x0800, "Slam Tilt",      KEYCODE_HOME)  \
  PORT_START /* 1 */ \
    COREPORT_DIPNAME( 0x0001, 0x0001, "SW1") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0001, "1" ) \
    COREPORT_DIPNAME( 0x0002, 0x0002, "SW2") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0002, "1" ) \
    COREPORT_DIPNAME( 0x0004, 0x0000, "W20") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0004, "1" ) \
    COREPORT_DIPNAME( 0x0008, 0x0000, "W19") \
      COREPORT_DIPSET(0x0000, "0" ) \
      COREPORT_DIPSET(0x0008, "1" ) \
    COREPORT_DIPNAME( 0x00f0, 0x0000, "Country") \
      COREPORT_DIPSET(0x0000, "USA 1" ) \
      COREPORT_DIPSET(0x0010, "France 1" ) \
      COREPORT_DIPSET(0x0020, "Germany" ) \
      COREPORT_DIPSET(0x0030, "France 2" ) \
      COREPORT_DIPSET(0x0040, "Unknown 1" ) \
      COREPORT_DIPSET(0x0050, "Unknown 2" ) \
      COREPORT_DIPSET(0x0060, "Unknown 3" ) \
      COREPORT_DIPSET(0x0070, "Unknown 4" ) \
      COREPORT_DIPSET(0x0080, "Export 1" ) \
      COREPORT_DIPSET(0x0090, "France 3" ) \
      COREPORT_DIPSET(0x00a0, "Export 2" ) \
      COREPORT_DIPSET(0x00b0, "France 4" ) \
      COREPORT_DIPSET(0x00c0, "UK" ) \
      COREPORT_DIPSET(0x00d0, "European" ) \
      COREPORT_DIPSET(0x00e0, "Spain" ) \
      COREPORT_DIPSET(0x00f0, "USA 2" )

/*-- Common keys inport --*/
#define WPC_COMINPORT      CORE_COREINPORT
#define WPC_COMCOINDOORKEY 0x0100
#define WPC_COMSTARTKEY    0x0200
#define WPC_COMTILTKEY     0x0400
#define WPC_COMSTILTKEY    0x0800

/*-- Standard input ports --*/
#define WPC_INPUT_PORTS_START(name,balls) \
  INPUT_PORTS_START(name) \
    CORE_PORTS \
    SIM_PORTS(balls) \
    WPC_COMPORTS

#define WPC_INPUT_PORTS_END INPUT_PORTS_END

#define WPC_swF1   (CORE_FLIPPERSWCOL*10+1)
#define WPC_swF2   (CORE_FLIPPERSWCOL*10+2)
#define WPC_swF3   (CORE_FLIPPERSWCOL*10+3)
#define WPC_swF4   (CORE_FLIPPERSWCOL*10+4)
#define WPC_swF5   (CORE_FLIPPERSWCOL*10+5)
#define WPC_swF6   (CORE_FLIPPERSWCOL*10+6)
#define WPC_swF7   (CORE_FLIPPERSWCOL*10+7)
#define WPC_swF8   (CORE_FLIPPERSWCOL*10+8)

#define WPC_swLRFlipEOS swF1
#define WPC_swLRFlip    swF2
#define WPC_swLLFlipEOS swF3
#define WPC_swLLFlip    swF4
#define WPC_swURFlipEOS swF5
#define WPC_swURFlip    swF6
#define WPC_swULFlipEOS swF7
#define WPC_swULFlip    swF8

/*-------------------------
/ Machine driver constants
/--------------------------*/
#define WPC_CPUNO   0

/*-- Memory regions --*/
#define WPC_CPUREGION       REGION_CPU1
#define WPC_DMDREGION       REGION_USER1
#define WPC_ROMREGION       REGION_USER2

/*-- standard display layout --*/
extern const core_tLCDLayout wpc_dispAlpha[];
extern const core_tLCDLayout wpc_dispDMD[];

/*-- Main CPU regions and ROM --*/
#define WPC_ROMSTART(name, ver, n1, size, chk1) \
   ROM_START(name##_##ver) \
     NORMALREGION(0x10000, WPC_CPUREGION) \
     NORMALREGION(0x2000,  WPC_DMDREGION) \
     NORMALREGION(size,    WPC_ROMREGION) \
       ROM_LOAD(n1, 0x00000, size, chk1)
#define WPC_ROMEND ROM_END

#define WPC_ROMSTARTNV(name, n1, size, chk1) \
   ROM_START(name) \
     NORMALREGION(0x10000, WPC_CPUREGION) \
     NORMALREGION(0x2000,  WPC_DMDREGION) \
     NORMALREGION(size,    WPC_ROMREGION) \
       ROM_LOAD(n1, 0x00000, size, chk1)

/*----------------------------------
/  Start address for the WPC chip
/-----------------------------------*/
#define WPC_BASE        0x3fb0

/*----------------------------------
/ The WPC registers I know about
/ Some registers were moved in later
/ WPC generations. These end with "95"
/ See https://www.scottkirvan.com/FreeWPC/The-WPC-Hardware.html for a detailed presentation
/-----------------------------------*/
/*----------------------------------
/  WPC_FLIPPERS write (active low)
/   0  LL Stroke    4 LU Stroke
/   1  LL Hold      5 LU Hold
/   2  RL Stroke    6 RU Stroke
/   3  RL Hold      7 RU Hold
/-----------------------------------*/
/*-- WPC bit shifter -------------------------------------------------------
/ This function adds on a bit level
/ SHIFTADRH and SHIFTADRL forms a 16 bit base address
/ SHIFTBIT is the number of bits to add
/ Writing to registers only stores the value
/ When the address is read the value of SHIFTBIT is added to the stored address
/ Reading the SHIFTBIT returns the bit position
/ Write address 0x1000 and SHIFTBIT 0x26. The values read will then be:
/ address=0x1004 and SHIFTBIT=0x10
/ or in c-syntax:
/   address = address + SHIFTBIT/8
/   SHIFTBIT = 1<<(SHIFTBIT & 7)
/
/ SHIFTBIT2 works in the same way but the address is unaffected
/   SHIFTBIT2 = 1<<(SHIFTBIT2 & 7)
/
/ Doctor Who is the only game testing the shifter.
/------------------------------------------------------------------------------
/ A = Alpha, M = ?, F = Fliptronics, D = DCS, S = Security, 9 = WPC95
/------------------------------------------------AMFDS9-------------------------*/
/* 3fb0 .. 3fb7 - RS232 on WPC95 Audio/Video Board (used for NBA fastbreak linking and debugging) */
/* 3fb8 .. 3fbf - DMD controller board */
#define WPC_DMD_PAGE3200  (0x3fb8 - WPC_BASE) /*      x W: CPU access memory bank select (added with WPC-95 as discrete IC was replaced by a custom chip) */
#define WPC_DMD_PAGE3000  (0x3fb9 - WPC_BASE) /*      x W: CPU access memory bank select (added with WPC-95 as discrete IC was replaced by a custom chip) */
#define WPC_DMD_PAGE3600  (0x3fba - WPC_BASE) /*      x W: CPU access memory bank select (added with WPC-95 as discrete IC was replaced by a custom chip) */
#define WPC_DMD_PAGE3400  (0x3fbb - WPC_BASE) /*      x W: CPU access memory bank select (added with WPC-95 as discrete IC was replaced by a custom chip) */
#define WPC_DMD_PAGE3A00  (0x3fbc - WPC_BASE) /*      x W: CPU access memory bank select (HIGHPAGEWR in schematics 16.9148.1) */
#define WPC_DMD_FIRQLINE  (0x3fbd - WPC_BASE) /*   xxxx R: Bit7 FIRQ state W: ack DMD FIRQ and write row number to raise next FIRQ (FIRQRD/ROW_IRQ in schematics 16.9148.1) */
#define WPC_DMD_PAGE3800  (0x3fbe - WPC_BASE) /*   xxxx W: CPU access memory bank select (LOWPAGEWR in schematics 16.9148.1) */
#define WPC_DMD_SHOWPAGE  (0x3fbf - WPC_BASE) /*   xxxx W: page to rasterize on next VBlank (DISPAGEWR in schematics 16.9148.1) */
/* 3fc0 .. 3fdf - External IO boards */
/* Printer board */
#define WPC_PRINTBUSY     (0x3fc0 - WPC_BASE) /* xxxxx  R: Printer ready ??? */
#define WPC_PRINTDATA     (0x3fc1 - WPC_BASE) /* xxxxx  W: send to printer */
#define WPC_PRINTDATAX    (0x3fc2 - WPC_BASE) /* xxxxx  W: 0: Printer data available */
#define WPC_SERIAL_DATA   (0x3fc3 - WPC_BASE) /* xxxxx  8251 UART of WPC Printer Option Kit */
#define WPC_SERIAL_CTRL   (0x3fc4 - WPC_BASE) /* xxxxx  8251 UART of WPC Printer Option Kit */
#define WPC_SERIAL_BAUD   (0x3fc5 - WPC_BASE) /* xxxxx  baud rate divider (0=9600, 1=4800, ..., 5=300) */
/* Ticket dispenser board */
#define WPC_TICKET_DISP   (0x3fc6 - WPC_BASE)
/* S11 Sound board */
#define WPC_SND_S11_DATA0 (0x3fd0 - WPC_BASE) /*         */
#define WPC_SND_S11_DATA1 (0x3fd1 - WPC_BASE) /*         */
#define WPC_SND_S11_CTRL  (0x3fd2 - WPC_BASE) /*         */
#define WPC_SND_S11_UNK   (0x3fd3 - WPC_BASE) /*         */
/* Fliptronics 2 Board */
#define WPC_FLIPPERS      (0x3fd4 - WPC_BASE) /*   xxx  R: switches W: Solenoids */
/* WPC and DCS Sound board 0x3fd8-0x3fdf */
#define WPC_SND_DATA      (0x3fdc - WPC_BASE) /* xxx    RW: Sound board interface */
#define WPC_SND_CTRL      (0x3fdd - WPC_BASE) /* xxx    RW: R: Sound data available, W: Reset soundboard ? */
/* Power driver board and misc. features */
#define WPC_SOLENOID1     (0x3fe0 - WPC_BASE) /* xxxxxx W: Solenoid 25-28 */
#define WPC_SOLENOID2     (0x3fe1 - WPC_BASE) /* xxxxxx W: Solenoid  1- 8 */
#define WPC_SOLENOID3     (0x3fe2 - WPC_BASE) /* xxxxxx W: Solenoid 17-24 */
#define WPC_SOLENOID4     (0x3fe3 - WPC_BASE) /* xxxxxx W: Solenoid  9-16 */
#define WPC_LAMPROW       (0x3fe4 - WPC_BASE) /* xxxxxx W: Lamp row */
#define WPC_LAMPCOLUMN    (0x3fe5 - WPC_BASE) /* xxxxxx W: Lamp column enable */
#define WPC_GILAMPS       (0x3fe6 - WPC_BASE) /*        W: GI Triac driver */
#define WPC_DIPSWITCH     (0x3fe7 - WPC_BASE) /* xxxxxx R: CPU board dip-switches */
#define WPC_SWCOINDOOR    (0x3fe8 - WPC_BASE) /* xxxxxx W: Coin door switches */
#define WPC_SWROWREAD     (0x3fe9 - WPC_BASE) /* xxxx   R: Switch row read */
#define WPC_PICREAD       (0x3fe9 - WPC_BASE) /*     xx R: PIC data (include switch read) */
#define WPC_SWCOLSELECT   (0x3fea - WPC_BASE) /* xxxx   W: Switch column enable */
#define WPC_PICWRITE      (0x3fea - WPC_BASE) /*     xx R: PIC data (include switch strobe) */
/* External board / Alphanum display / WPC95 flippers 0x3feb-0x3fef */
#define WPC_EXTBOARD1     (0x3feb - WPC_BASE) /*   x    W: Extension Driver Board 1 */
#define WPC_ALPHAPOS      (0x3feb - WPC_BASE) /* x      W: Alphanumeric column /DIS_STROBE */
#define WPC_EXTBOARD2     (0x3fec - WPC_BASE) /*   x    W: Extension Driver Board 2 */
#define WPC_ALPHA1LO      (0x3fec - WPC_BASE) /* x      W: Alphanumeric 1st row lo bits /DIS_1 */
#define WPC_EXTBOARD3     (0x3fed - WPC_BASE) /*   x    W: Extension Driver Board 3 */
#define WPC_ALPHA1HI      (0x3fed - WPC_BASE) /* x      W: Alphanumeric 1st row hi bits /DIS_2 */
#define WPC_EXTBOARD4     (0x3fee - WPC_BASE) /*   x    W: Extension Driver Board 4 */
#define WPC_FLIPPERCOIL95 (0x3fee - WPC_BASE) /*      x W: Flipper Solenoids */
#define WPC_ALPHA2LO      (0x3fee - WPC_BASE) /* x      W: Alphanumeric 2nd row lo bits /DIS_3 */
#define WPC_EXTBOARD5     (0x3fef - WPC_BASE) /*   x    W: Extension Driver Board 5 */
#define WPC_FLIPPERSW95   (0x3fef - WPC_BASE) /*      x R: Flipper switches */
#define WPC_ALPHA2HI      (0x3fef - WPC_BASE) /* x      W: Alphanumeric 2nd row hi bits /DIS_4 */
/* CPU board features 0x3ff0 - 0x3fff */
#define WPC_LED           (0x3ff2 - WPC_BASE) /* xxxxxx W: CPU LED (bit 7) */
#define WPC_IRQACK        (0x3ff3 - WPC_BASE) /*        W: IRQ Ack ??? */
#define WPC_SHIFTADRH     (0x3ff4 - WPC_BASE) /* xxxxxx RW: See above */
#define WPC_SHIFTADRL     (0x3ff5 - WPC_BASE) /* xxxxxx RW: See above */
#define WPC_SHIFTBIT      (0x3ff6 - WPC_BASE) /* xxxxxx RW: See above */
#define WPC_SHIFTBIT2     (0x3ff7 - WPC_BASE) /* xxxxxx RW: See above */
#define WPC_HIGHRESTIMER  (0x3ff8 - WPC_BASE) /* xxxxxx guessed: R: bit 7=1 if counter is 0 (and FIRQ is raised) W: Clear FIRQ line and restart counter */
#define WPC_RTCHOUR       (0x3ffa - WPC_BASE) /* xxxxxx RW: Real time clock: hour */
#define WPC_RTCMIN        (0x3ffb - WPC_BASE) /* xxxxxx RW: Real time clock: minute */
#define WPC_ROMBANK       (0x3ffc - WPC_BASE) /* xxxxxx W: Rombank switch */
#define WPC_PROTMEM       (0x3ffd - WPC_BASE) /* xxxxxx W: enabled/disable protected memory */
#define WPC_PROTMEMCTRL   (0x3ffe - WPC_BASE) /* xxxxxx W: Set protected memory area */
#define WPC_ZC_IRQ_ACK    (0x3fff - WPC_BASE) /* xxxxxx R: Bit7 zero cross state, Bit2 IRQ timer enabled W: Bit7 Ack IRQ, Bit2 enable IRQ timer, Bit1 reset watchdog */

/*-- the internal state of the WPC chip. Should only be used in memory handlers --*/
extern UINT8 *wpc_data;
extern UINT8 *wpc_ram;

/*---------------------
/  Exported functions
/----------------------*/

/*--- memory handlers for the Orkin debugger interface --*/
extern READ_HANDLER(orkin_r);
extern WRITE_HANDLER(orkin_w);

/*-- use this if a fallback is required in a custom memory handler --*/
extern READ_HANDLER(wpc_r);
extern WRITE_HANDLER(wpc_w);

extern MACHINE_DRIVER_EXTERN(wpc_alpha);
extern MACHINE_DRIVER_EXTERN(wpc_dmd);
#define wpc_mAlpha       wpc_alpha
#define wpc_mDMD         wpc_dmd
#define wpc_mFliptron    wpc_dmd
#define wpc_m95          wpc_dmd
extern MACHINE_DRIVER_EXTERN(wpc_alpha1S);
extern MACHINE_DRIVER_EXTERN(wpc_alpha2S);
extern MACHINE_DRIVER_EXTERN(wpc_dmdS);
extern MACHINE_DRIVER_EXTERN(wpc_dcsS);
extern MACHINE_DRIVER_EXTERN(wpc_95S);
#define wpc_mAlpha1S     wpc_alpha1S
#define wpc_mAlpha2S     wpc_alpha2S
#define wpc_mFliptronS   wpc_dmdS
#define wpc_mDMDS        wpc_dmdS
#define wpc_mDCSS        wpc_dcsS
#define wpc_mSecurityS   wpc_dcsS
#define wpc_m95DCSS      wpc_dcsS
#define wpc_m95S         wpc_95S

// Game specific options
#define WPC_CFTBL        0x01 // CFTBL specific hardware: chase light 2 bit decoder from solenoid #3 output, wired through triacs to 2 GI outputs, leading to 8 additional PWM controlled GI
#define WPC_PH           0x02 // Phantom Haus specific hardware (not really a pinball)
// next: 0x04

int wpc_m2sw(int col, int row);
void wpc_set_modsol_aux_board(int board);
void wpc_set_fastflip_addr(int addr);

#ifdef PROC_SUPPORT
  typedef void (*wpc_proc_solenoid_handler_t)(int solNum, int enabled, int smoothed);
  void default_wpc_proc_solenoid_handler(int solNum, int enabled, int smoothed);
  extern wpc_proc_solenoid_handler_t wpc_proc_solenoid_handler;
#endif

#endif /* INC_WPC */
