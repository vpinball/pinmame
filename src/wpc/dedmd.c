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
#if 0
/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
extern int de_dmd128x16[16][128];

void de_dmd128x16_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  int cols=128;
  int rows=16;
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  //UINT8 dotCol[34][129];
  int ii,jj;//,kk;

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (ii = 0; ii < rows; ii++) {
	  BMTYPE *line = *lines++;
	  for (jj = 0; jj < cols; jj++) {
		  if(de_dmd128x16[ii][jj])
			  *line++ = (de_dmd128x16[ii][jj]>1) ? CORE_COLOR(DMD_DOTON) : CORE_COLOR(DMD_DOT66);
		  else
			*line++ = CORE_COLOR(DMD_DOTOFF);

		line++;
	  }
	  lines++;
  }

  osd_mark_dirty(0,0,cols*coreGlobals_dmd.DMDsize,rows*coreGlobals_dmd.DMDsize);

  drawStatus(bitmap,fullRefresh);
}
#endif
/*DIFFERENT ATTEMPT AT DE 128x16 HANDLING*/
/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
static core_tLCDLayout de_128x16DMD[] = {
  {0,0,16,128,CORE_DMD}, {0}
};
void de2_dmd128x16_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  UINT8 *RAM  = (UINT8 *)dmd16RAM;
  UINT8 *RAM2 = RAM;
  tDMDDot dotCol;
  int ii,jj,kk;

  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  /* See if ANY data has been written to DMD region #2 0x8100-0x8200*/
  for (ii = 0; ii < 16*128/8; ii++)
    if (RAM[ii+0x0100]) { RAM2 = RAM + 0x0100; break; }

  for (kk = 0, ii = 1; ii <= 16; ii++) {
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
  dmd_draw(bitmap, dotCol, de_128x16DMD);
  drawStatus(bitmap,fullRefresh);
}

/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
} dmdlocals;

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
#define DMD16_FIRQFREQ 2000
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
  dmd16_setbusy(BUSY_SET|BUSY_CLR,BUSY_SET|BUSY_CLR);
  timer_pulse(TIME_IN_HZ(DMD16_FIRQFREQ), 0, de_dmd16nmi);
}
/*--- Port decoding ----
  76543210
  10 001   Bank0
  10 011   Bank1
  10 101   Bank2
  11 001   Status
  11 111   Test
  1    0   IDAT
  ------
  10 111   Blanking
  11 011   Row Data
  11 101   Row Clock
  0    1   CLATCH
  0    0   COCLK

--------------------*/
static READ_HANDLER(dmd16_port_r) {
  if ((offset & 0x84) == 0x80) {
    dmd16_setbusy(BUSY_CLR, 0); dmd16_setbusy(BUSY_CLR,1);
    return dmdlocals.cmd;
  }
  return 0xff;
}
static WRITE_HANDLER(dmd16_port_w) {
  data &= 0x01;
  switch (offset & 0xbc) {
    case 0x84: dmd16_setbank(0x01, !data);    break;
    case 0x8c: dmd16_setbank(0x02, !data);    break;
    case 0x94: dmd16_setbank(0x04, !data);    break;
    case 0xa4: sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data); break;
    case 0xbc: dmd16_setbusy(BUSY_SET, data); break;
  }
}
static WRITE_HANDLER(dmd16_ctrl_w) {
  if ((data | dmdlocals.ctrl) & 0x01) {
    dmdlocals.cmd = dmdlocals.ncmd;
    dmd16_setbusy(BUSY_CLK, data & 0x01);
  }
  if (~data & dmdlocals.ctrl & 0x02) {
    sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = 0);
    dmd16_setbank(0x07, 0x07);
    dmd16_setbusy(BUSY_SET, 0); dmd16_setbusy(BUSY_SET, 1);
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
  }
  dmdlocals.ctrl = data;
}

static void dmd16_setbusy(int bit, int value) {
  static int laststat = 0;
  int newstat = (laststat & ~bit) | (value ? bit : 0);
  switch (newstat & 0x03) {
    case 0x00:
    case 0x02: dmdlocals.busy = 0; break;
    case 0x03: if (!(newstat & ~laststat & BUSY_CLK)) break;
    case 0x01: dmdlocals.busy = 1; break;
  }
  laststat = newstat;
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, Z80_INT_REQ, dmdlocals.busy ? ASSERT_LINE : CLEAR_LINE);
  sndbrd_data_cb(dmdlocals.brdData.boardNo, dmdlocals.busy);
}

static void dmd16_setbank(int bit, int value) {
  dmdlocals.bank = (dmdlocals.bank & ~bit) | (value ? bit : 0);
  cpu_setbank(DMD16_BANK0, dmdlocals.brdData.romRegion + (dmdlocals.bank & 0x07)*0x4000);
}

static void de_dmd16nmi(int data) { cpu_set_nmi_line(dmdlocals.brdData.cpuNo, HOLD_LINE); }

