#include "driver.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
//#include "de.h"
#include "sndbrd.h"
#include "dedmd.h"

//extern int crtc6845_start_addr;
static UINT16 *dmd64RAM;
static UINT8  *dmd32RAM;
static UINT8  *dmd16RAM;
static UINT32 dmd16frame[4][32][2];
/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
  // dmd16 stuff
  UINT32 hv5408, hv5408s, hv5308, hv5308s, hv5222;
  int blnk, rowdata, rowclk, frame;
} dmdlocals;
/*-----------------------------*/
/*Data East 192x64 DMD Handling*/
/*-----------------------------*/
void de_dmd192x64_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
//  UINT8 *RAM  = memory_region(DE_MEMREG_DCPU1) + 0x800000 + (crtc6845_start_addr&0x400)*4;
  UINT8 *RAM  = (UINT8 *)(dmd64RAM) + ((crtc6845_start_addr & 0x400)<<2);
  UINT8 *RAM2 = RAM + 0x800;
  tDMDDot dotCol;
  int ii,jj,kk;

  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);
  for (kk = 0, ii = 1; ii <= 64; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (192/16); jj++) {
      UINT8 intens1 = 2*(RAM[kk+1] & 0x55) + (RAM2[kk+1] & 0x55);
      UINT8 intens2 =   (RAM[kk+1] & 0xaa) + (RAM2[kk+1] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      intens1 = 2*(RAM[kk] & 0x55) + (RAM2[kk] & 0x55);
      intens2 =   (RAM[kk] & 0xaa) + (RAM2[kk] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      kk += 2;
    }
    *line = 0;
  }
  dmd_draw(bitmap, dotCol, core_gameData->lcdLayout);
  drawStatus(bitmap,fullRefresh);
}

/*------------------------------------------*/
/*Data East, Sega, Stern 128x32 DMD Handling*/
/*------------------------------------------*/
static core_tLCDLayout de_128x32DMD[] = {
  {0,0,32,128,CORE_DMD}, {0}
};

void de_dmd128x32_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
//  UINT8 *RAM  = memory_region(DE_MEMREG_DCPU1) + 0x2000 + (crtc6845_start_addr&0x0100)*4;
  UINT8 *RAM  = ((UINT8 *)dmd32RAM) + ((crtc6845_start_addr & 0x0100)<<2);
  UINT8 *RAM2 = RAM + 0x200;
  tDMDDot dotCol;
  int ii,jj,kk;

  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (kk = 0, ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (128/8); jj++) {
      UINT8 intens1 = 2*(RAM[kk] & 0x55) + (RAM2[kk] & 0x55);
      UINT8 intens2 =   (RAM[kk] & 0xaa) + (RAM2[kk] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      kk += 1;
    }
    *line = 0;
  }
  dmd_draw(bitmap, dotCol, de_128x32DMD);
  drawStatus(bitmap,fullRefresh);
}
/*DIFFERENT ATTEMPT AT DE 128x16 HANDLING*/
/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
static core_tLCDLayout de_128x16DMD[] = {
  {0,0,16,128,CORE_DMD}, {0}
};
void de2_dmd128x16_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  tDMDDot dotCol;
  int ii,jj,kk,ll;
  DBGLOG(("refresh\n"));
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);
  memset(dotCol,0,sizeof(dotCol));
  for (ll = 2; ll < 4; ll++) {
    for (ii = 0; ii < 16; ii++) {
      UINT8 *line = &dotCol[ii+1][0];
      for (jj = 0; jj < 2; jj++) {
        UINT32 tmp1 = dmd16frame[(dmdlocals.frame+ll)&3][ii*2+jj][0];
        UINT32 tmp2 = dmd16frame[(dmdlocals.frame+ll)&3][ii*2+jj][1];
        for (kk = 0; kk < 32; kk++) {
          *line++ = (tmp2 & 0x80000000) ? 3 : 0; tmp2 <<= 1;
          *line++ = (tmp1 & 0x80000000) ? 3 : 0; tmp1 <<= 1;
        }
      }
    }
  }
  dmd_draw(bitmap, dotCol, de_128x16DMD);
  drawStatus(bitmap,fullRefresh);
}      
      
static WRITE_HANDLER(dmd_data_w)  { dmdlocals.ncmd = data; DBGLOG(("dmdcmd=%x\n",data));}
static READ_HANDLER(dmd_status_r) { return dmdlocals.status; }
static READ_HANDLER(dmd_busy_r)   { return dmdlocals.busy; }

/*------------ DMD 128x32 -------------*/
#define DMD32_BANK0    2
#define DMD32_FIRQFREQ 125

static WRITE_HANDLER(dmd32_ctrl_w);
static void dmd32_init(struct sndbrdData *brdData);

const struct sndbrdIntf dedmd32Intf = {
  dmd32_init, NULL, NULL,
  dmd_data_w, dmd_busy_r, dmd32_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE_HANDLER(dmd32_bank_w);
static WRITE_HANDLER(dmd32_status_w);
static READ_HANDLER(dmd32_latch_r);

MEMORY_READ_START(de_dmd32readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2000, 0x2fff, MRA_RAM }, /* DMD RAM PAGE 0-7 512 bytes each */
  { 0x3000, 0x3000, crtc6845_register_r },
  { 0x3003, 0x3003, dmd32_latch_r },
  { 0x4000, 0x7fff, MRA_BANKNO(DMD32_BANK0) }, /* Banked ROM */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(de_dmd32writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2fff, MWA_RAM, &dmd32RAM }, /* DMD RAM PAGE 0-7 512 bytes each*/
  { 0x3000, 0x3000, crtc6845_address_w },
  { 0x3001, 0x3001, crtc6845_register_w },
  { 0x3002, 0x3002, dmd32_bank_w }, /* DMD Bank Switching*/
  { 0x4000, 0x4000, dmd32_status_w },   /* DMD Status*/
  { 0x8000, 0xffff, MWA_ROM },
MEMORY_END

static void dmd32firq(int data);

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion);
  /* copy last 16K of ROM into last 16K of CPU region*/
  memcpy(memory_region(DE_DMD32CPUREGION) + 0x8000,
         memory_region(DE_DMD32ROMREGION) + memory_region_length(DE_DMD32ROMREGION)-0x8000,0x8000);
  timer_pulse(TIME_IN_HZ(DMD32_FIRQFREQ), 0, dmd32firq);
}

static WRITE_HANDLER(dmd32_ctrl_w) {
  if ((data | dmdlocals.ctrl) & 0x01) {
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, (data & 0x01) ? ASSERT_LINE : CLEAR_LINE);
    if (data & 0x01) {
      sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 1);
      dmdlocals.cmd = dmdlocals.ncmd;
    }
  }
  else if (~data & dmdlocals.ctrl & 0x02) {
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
    cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion);
  }
  dmdlocals.ctrl = data;
}

static WRITE_HANDLER(dmd32_status_w) {
  sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data & 0x0f);
}

static WRITE_HANDLER(dmd32_bank_w) {
  cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion + (data & 0x1f)*0x4000);
}
static READ_HANDLER(dmd32_latch_r) {
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 0); // Clear Busy
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
  return dmdlocals.cmd;
}

static void dmd32firq(int data) {
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}

/*------- DMD 192x64 -----------*/
#define DMD64_FIRQFREQ 150

static WRITE_HANDLER(dmd64_ctrl_w);
static void dmd64_init(struct sndbrdData *brdData);

const struct sndbrdIntf dedmd64Intf = {
  dmd64_init, NULL, NULL,
  dmd_data_w, dmd_busy_r, dmd64_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE16_HANDLER(crtc6845_msb_address_w);
static WRITE16_HANDLER(crtc6845_msb_register_w);
static READ16_HANDLER(crtc6845_msb_register_r);
static READ16_HANDLER(dmd64_latch_r);
static WRITE16_HANDLER(dmd64_status_w);

MEMORY_READ16_START(de_dmd64readmem)
  { 0x00000000, 0x000fffff, MRA16_ROM }, /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, MRA16_RAM }, /* RAM - 0x800000 Page 0, 0x801000 Page 1*/
  { 0x00c00010, 0x00c00011, crtc6845_msb_register_r },
  { 0x00c00020, 0x00c00021, dmd64_latch_r }, /* Read the Latch from CPU*/
MEMORY_END

MEMORY_WRITE16_START(de_dmd64writemem)
  { 0x00000000, 0x000fffff, MWA16_ROM},	 /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, MWA16_RAM, &dmd64RAM},	 /* RAM - 0x800000 Page 0, 0x801000 Page 1*/
  { 0x00c00010, 0x00c00011, crtc6845_msb_address_w},
  { 0x00c00012, 0x00c00013, crtc6845_msb_register_w},
  { 0x00c00020, 0x00c00021, dmd64_status_w},/* Set the Status Line*/
MEMORY_END

static void dmd64irq2(int data);

static void dmd64_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  timer_pulse(TIME_IN_HZ(DMD64_FIRQFREQ), 0, dmd64irq2);
}

static WRITE_HANDLER(dmd64_ctrl_w) {
  if (data & ~dmdlocals.ctrl & 0x01) {
    cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_1, ASSERT_LINE);
    sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 1);
    dmdlocals.cmd = dmdlocals.ncmd;
  }
  if (data & ~dmdlocals.ctrl & 0x02)
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
  dmdlocals.ctrl = data;
}

static WRITE16_HANDLER(dmd64_status_w) {
  if (ACCESSING_LSB) sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data & 0x0f);
}

static READ16_HANDLER(dmd64_latch_r) {
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy = 0);
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_1, CLEAR_LINE);
  return dmdlocals.cmd;
}
static void dmd64irq2(int data) {
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, HOLD_LINE);
}
static WRITE16_HANDLER(crtc6845_msb_address_w)  { if (ACCESSING_MSB) crtc6845_address_w(offset,data>>8);  }
static WRITE16_HANDLER(crtc6845_msb_register_w) { if (ACCESSING_MSB) crtc6845_register_w(offset,data>>8); }
static READ16_HANDLER(crtc6845_msb_register_r)  { return crtc6845_register_r(offset)<<8; }

/*------- DMD 128x16 -----------*/
#define DMD16_BANK0 2
#define DMD16_FIRQFREQ 1000
#define BUSY_CLR    0x01
#define BUSY_SET    0x02
#define BUSY_CLK    0x04

static void dmd16_init(struct sndbrdData *brdData);
static WRITE_HANDLER(dmd16_ctrl_w);

const struct sndbrdIntf dedmd16Intf = {
  dmd16_init, NULL, NULL,
  dmd_data_w, dmd_busy_r, dmd16_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static READ_HANDLER(dmd16_port_r);
static WRITE_HANDLER(dmd16_port_w);

MEMORY_READ_START(de_dmd16readmem)
  { 0x0000, 0x3fff, MRA_ROM },	               /* Z80 ROM CODE*/
  { 0x4000, 0x7fff, MRA_BANKNO(DMD16_BANK0) }, /* ROM BANK*/
  { 0x8000, 0x9fff, MRA_RAM },
MEMORY_END

MEMORY_WRITE_START(de_dmd16writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x7fff, MWA_ROM },
  { 0x8000, 0x9fff, MWA_RAM, &dmd16RAM },
MEMORY_END

PORT_READ_START(de_dmd16readport)
  { 0x00, 0xff, dmd16_port_r },
PORT_END

PORT_WRITE_START(de_dmd16writeport)
  { 0x00, 0xff, dmd16_port_w },
PORT_END

static void dmd16_setbusy(int bit, int value);
static void dmd16_setbank(int bit, int value);
static void de_dmd16nmi(int data);

static void dmd16_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  memcpy(memory_region(DE_DMD16CPUREGION),
         memory_region(DE_DMD16ROMREGION) + memory_region_length(DE_DMD16ROMREGION)-0x4000,0x4000);
  dmd16_setbank(0x07, 0x07);
  dmd16_setbusy(BUSY_SET|BUSY_CLR,0);
  timer_pulse(TIME_IN_HZ(DMD16_FIRQFREQ), 0, de_dmd16nmi);
}
/*--- Port decoding ----
  76543210
  10-001-- Bank0
  10-011-- Bank1
  10-101-- Bank2
  11-001-- Status
  11-111-- Test
  1----0-- IDAT
  ------
  10-111-- Blanking
  11-011-- Row Data
  11-101-- Row Clock
  0----1-- CLATCH
  0----0-- COCLK

--------------------*/
static READ_HANDLER(dmd16_port_r) {
  if ((offset & 0x84) == 0x80) {
    dmd16_setbusy(BUSY_CLR, 0); dmd16_setbusy(BUSY_CLR,1);
    return dmdlocals.cmd;
  }
  dmd16_port_w(offset,0xff);
  return 0xff;
}

static WRITE_HANDLER(dmd16_port_w) {
  switch (offset & 0x84) {
    case 0x00: // COCLK
      dmdlocals.hv5408s = (dmdlocals.hv5408s<<1) | ((data>>((offset & 0x03)*2))   & 0x01);
      dmdlocals.hv5308s = (dmdlocals.hv5308s<<1) | ((data>>((offset & 0x03)*2+1)) & 0x01);
      break;
    case 0x04: // CLATCH
      dmdlocals.hv5408 = dmdlocals.hv5408s; dmdlocals.hv5308 = dmdlocals.hv5308s; break;
    case 0x80: break; // IDAT (ignored on write)
    case 0x84:
      data &= 0x01;
      switch (offset & 0xdc) {
        case 0x84: // Bank0
          dmd16_setbank(0x01, !data); break;
        case 0x8c: // Bank1
          dmd16_setbank(0x02, !data); break;
        case 0x94: // Bank2
          dmd16_setbank(0x04, !data); break;
        case 0xc4: // status
          sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data); break;
        case 0xdc: // Test
          dmd16_setbusy(BUSY_SET, data); break; // Test
        case 0x9c: // blanking
          if (~data & dmdlocals.blnk) {
            UINT32 row = dmdlocals.hv5222;
            int ii;
            for (ii = 0; row && (ii < 32); ii++, row >>= 1)
              if (row & 0x01) { // Row is active copy column data
                dmd16frame[dmdlocals.frame][ii][0] = dmdlocals.hv5408; // even dots
                dmd16frame[dmdlocals.frame][ii][1] = dmdlocals.hv5308; // odd dots
              }
            DBGLOG(("Blanking row=%2d frame=%d data=(%08x,%08x)\n",ii,dmdlocals.frame,dmdlocals.hv5408,dmdlocals.hv5308));
            if (ii == 32) dmdlocals.frame = (dmdlocals.frame + 1) & 3;
          }
          dmdlocals.blnk = data;
          break;
        case 0xcc: // row data
          dmdlocals.rowdata = data; break; // row data
        case 0xd4: // row clock
          if (~data & dmdlocals.rowclk) // negative edge;
            dmdlocals.hv5222 = (dmdlocals.hv5222<<1) | (dmdlocals.rowdata);
            dmdlocals.rowclk = data;
          break;
      } // Switch
      break;
  } // switch
}

static WRITE_HANDLER(dmd16_ctrl_w) {
  if ((data | dmdlocals.ctrl) & 0x01) {
    dmdlocals.cmd = dmdlocals.ncmd;
    dmd16_setbusy(BUSY_CLK, data & 0x01);
  }
  if (~data & dmdlocals.ctrl & 0x02) {
    sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = 0);
    dmd16_setbank(0x07, 0x07);
    dmd16_setbusy(BUSY_CLR|BUSY_SET, 0); 
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
  }
  dmdlocals.ctrl = data;
}

static void dmd16_setbusy(int bit, int value) {
  static int laststat = 0;
  int newstat = (laststat & ~bit) | (value ? bit : 0);
#if 1
  /* In the data-sheet for the HC74 flip-flop is says that SET & CLR are _not_
     edge triggered. For some strange reason doesn't the DMD work unless we
     treat the HC74 as edge-triggered.
  */
  if      (~newstat & laststat & BUSY_CLR) dmdlocals.busy = 0;
  else if (~newstat & laststat & BUSY_SET) dmdlocals.busy = 1;
  else if ((newstat & (BUSY_CLR|BUSY_SET)) == (BUSY_CLR|BUSY_SET)) {
    if (newstat & ~laststat & BUSY_CLK) dmdlocals.busy = 1;
  }
#else
  switch (newstat & 0x03) {
    case 0x00: // CLR=0 SET=0 => CLR
    case 0x02: // CLR=0 SET=1 => CLR
      dmdlocals.busy = 0; break;
    case 0x03: // CLR=1 SET=1 => check clock
      if (!(newstat & ~laststat & BUSY_CLK)) break;
    case 0x01: // CLR=1 SET=0 => SET
      dmdlocals.busy = 1; break;
  }
#endif
  laststat = newstat;
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, Z80_INT_REQ, dmdlocals.busy ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy);
}

static void dmd16_setbank(int bit, int value) {
  dmdlocals.bank = (dmdlocals.bank & ~bit) | (value ? bit : 0);
  cpu_setbank(DMD16_BANK0, dmdlocals.brdData.romRegion + (dmdlocals.bank & 0x07)*0x4000);
}

static void de_dmd16nmi(int data) { DBGLOG(("DMD NMI\n")); cpu_set_nmi_line(dmdlocals.brdData.cpuNo, PULSE_LINE); }

