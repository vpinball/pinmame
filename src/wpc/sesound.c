#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "sound/bsmt2000.h"
#include "se.h"
#include "sesound.h"

#define SE_SCPUNO 2

// Missing things
// When a sound command is written from the Main CPU it generates a BUF-FULL signal
// and latches the data into U5
// The BUF-FULL signal goes into the PAL. No idea what happens there
// When the CPU reads the sound commands (BIN) a FIRQ is generated and BUF-FULL
// is cleared.
// A FIRQ is also generated from the 24MHz signal via two dividers U3 & U2
// (I think it is 24MHz/4/4096=1536 Hz)
//
// The really strange thing is the PAL. It can generate the following output
// BUSY  - Not used on BSMT board. Sent back to CPU board to STATUS latch?
// OSTAT - If D7 from the CPU is high this sends a reset to the BSMT
//         D0 is sent to CPU board as SST0
// BROM  - ROM for CPU
// BRAM  - RAM for CPU
// DSP0  - Latches A0-A7 and D0-D7 (low) into BSMT
// DSP1  - Latches D0-D7 (high) into BSMT. Ack's IRQ
// BIN   - Gets sound command from LATCH, Clears BUF-FUL and Generates FIRQ
// Input to the PAL is
// A15,A14,A13,A2,A1
// BUF-FUL
// FIRQ
// IRQ
// (E, R/W)

/*-- local data --*/
static struct {
  int bufFull;
  int soundCmd;
  int bsmtData;
} locals;

static WRITE_HANDLER(ses_bsmt0_w) {
//  DBGLOG(("bsmtdata = %x\n",data));
  locals.bsmtData = data;
}
static WRITE_HANDLER(ses_bsmt1_w) {
	 int reg = offset^ 0xff;
	 int sdata = ((locals.bsmtData<<8)|data);
	 BSMT2000_data_0_w(reg, sdata, 0);
     logerror("BSMT write to %02X (V%X R%X) = %02X%02X\n",
				reg, reg % 11, reg / 11, locals.bsmtData, data);
  
  //BSMT2000_data_0_w((~offset & 0xff), ((locals.bsmtData<<8)|data), 0);
  // BSMT is ready directly
  cpu_set_irq_line(SE_SCPUNO, M6809_IRQ_LINE, HOLD_LINE);
}
static READ_HANDLER(ses_2000_r) { DBGLOG(("2000r\n")); return 0; }
// Sound command read ? (BIN) Also generates FIRQ?
static READ_HANDLER(ses_2002_r) {
//  DBGLOG(("2002r\n"));
  locals.bufFull = FALSE;
  return locals.soundCmd;
}
static READ_HANDLER(ses_2004_r) { DBGLOG(("2004r\n")); return 0; }
// BSMT is always ready
static READ_HANDLER(ses_2006_r) {
  return 0x80;
}
// Writing 0x80 here resets BSMT ?
static WRITE_HANDLER(ses_2000_w) { /*DBGLOG(("2000w=%2x\n",data));*/ }
static WRITE_HANDLER(ses_2002_w) { DBGLOG(("2002w=%2x\n",data)); }
static WRITE_HANDLER(ses_2004_w) { DBGLOG(("2004w=%2x\n",data)); }
static WRITE_HANDLER(ses_2006_w) { DBGLOG(("2006w=%2x\n",data)); }
/*--------------
/  Memory maps
/---------------*/
MEMORY_READ_START(ses_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
//  { 0x2000, 0x2001, ses_2000_r },
  { 0x2002, 0x2003, ses_2002_r },
//  { 0x2004, 0x2005, ses_2004_r },
  { 0x2006, 0x2007, ses_2006_r },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(ses_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2001, ses_2000_w }, 
//  { 0x2002, 0x2003, ses_2002_w }, 
//  { 0x2004, 0x2005, ses_2004_w }, 
//  { 0x2006, 0x2007, ses_2006_w }, 
  { 0x2008, 0x5fff, MWA_ROM },
  { 0x6000, 0x6000, ses_bsmt0_w },
  { 0x6001, 0x9fff, MWA_ROM },
  { 0xa000, 0xffff, ses_bsmt1_w },
MEMORY_END

/*----------------
/ Sound interface
/-----------------*/

//Older 11 Voice Style BSMT Chip
struct BSMT2000interface ses_bsmt2000Int1 = {
        1, {24000000}, {11}, {SE_MEMREG_SROM1}, {100}, {2000}
};
//Newer 12 Voice Style BSMT Chip
struct BSMT2000interface ses_bsmt2000Int2 = {
        1, {24000000}, {12}, {SE_MEMREG_SROM1}, {100}, {2000}
};
//Older 11 Voice Style BSMT Chip but needs large volume adjustment
struct BSMT2000interface ses_bsmt2000Int3 = {
        1, {24000000}, {11}, {SE_MEMREG_SROM1}, {100}, {4000}
};

int ses_irq(void) {
  cpu_set_irq_line(SE_SCPUNO, M6809_FIRQ_LINE, HOLD_LINE);
  return 0;
}
void ses_init(void) {
}

WRITE_HANDLER(ses_soundCmd_w) {
  DBGLOG(("soundCmd:%2x\n",data));
  locals.soundCmd = data;
  locals.bufFull = TRUE;
}
