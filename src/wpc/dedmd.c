#include "driver.h"
#include "vidhrdw/crtc6845.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "core.h"
#include "sndbrd.h"
#include "dedmd.h"

/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
  UINT32 *framedata;
  // dmd16 stuff
  UINT32 hv5408, hv5408s, hv5308, hv5308s, hv5222, lasthv5222;
  int blnk, rowdata, rowclk, frame;
} dmdlocals;

static UINT16 *dmd64RAM;
static UINT8  *dmd32RAM;

static WRITE_HANDLER(dmd_data_w)  { dmdlocals.ncmd = data; }
static READ_HANDLER(dmd_status_r) { return dmdlocals.status; }
static READ_HANDLER(dmd_busy_r)   { return dmdlocals.busy; }

/*------------------------------------------*/
/*Data East, Sega, Stern 128x32 DMD Handling*/
/*------------------------------------------*/
#define DMD32_BANK0    2
#define DMD32_FIRQFREQ 125

static WRITE_HANDLER(dmd32_ctrl_w);
static void dmd32_init(struct sndbrdData *brdData);

const struct sndbrdIntf dedmd32Intf = {
  NULL, dmd32_init, NULL, NULL,NULL, 
  dmd_data_w, dmd_busy_r, dmd32_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE_HANDLER(dmd32_bank_w);
static WRITE_HANDLER(dmd32_status_w);
static READ_HANDLER(dmd32_latch_r);
static INTERRUPT_GEN(dmd32_firq);

static MEMORY_READ_START(dmd32_readmem)
  { 0x0000, 0x1fff, MRA_RAM },
  { 0x2000, 0x2fff, MRA_RAM }, /* DMD RAM PAGE 0-7 512 bytes each */
  { 0x3000, 0x3000, crtc6845_register_0_r },
  { 0x3001, 0x3002, MRA_NOP },
  { 0x3003, 0x3003, dmd32_latch_r },
  { 0x3004, 0x3fff, MRA_NOP },
  { 0x4000, 0x7fff, MRA_BANKNO(DMD32_BANK0) }, /* Banked ROM */
  { 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(dmd32_writemem)
  { 0x0000, 0x1fff, MWA_RAM },
  { 0x2000, 0x2fff, MWA_RAM, &dmd32RAM }, /* DMD RAM PAGE 0-7 512 bytes each*/
  { 0x3000, 0x3000, crtc6845_address_0_w },
  { 0x3001, 0x3001, crtc6845_register_0_w },
  { 0x3002, 0x3002, dmd32_bank_w }, /* DMD Bank Switching*/
  { 0x3003, 0x3fff, MWA_NOP },
  { 0x4000, 0x4000, dmd32_status_w },   /* DMD Status*/
  { 0x4001, 0xffff, MWA_NOP },
MEMORY_END

MACHINE_DRIVER_START(de_dmd32)
  MDRV_CPU_ADD(M6809, 4000000)
  MDRV_CPU_MEMORY(dmd32_readmem, dmd32_writemem)
  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion);
  /* copy last 16K of ROM into last 16K of CPU region*/
  memcpy(memory_region(DE_DMD32CPUREGION) + 0x8000,
         memory_region(DE_DMD32ROMREGION) + memory_region_length(DE_DMD32ROMREGION)-0x8000,0x8000);
  //Init 6845
  crtc6845_init(0);
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

static INTERRUPT_GEN(dmd32_firq) {
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}

PINMAME_VIDEO_UPDATE(dedmd32_update) {
  UINT8 *RAM  = ((UINT8 *)dmd32RAM) + ((crtc6845_start_address_r(0) & 0x0100)<<2);
  UINT8 *RAM2 = RAM + 0x200;
  tDMDDot dotCol;
  int ii,jj;

  for (ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (128/8); jj++) {
      UINT8 intens1 = 2*(RAM[0] & 0x55) + (RAM2[0] & 0x55);
      UINT8 intens2 =   (RAM[0] & 0xaa) + (RAM2[0] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      RAM += 1; RAM2 += 1;
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

/*-----------------------------*/
/*Data East 192x64 DMD Handling*/
/*-----------------------------*/
#define DMD64_IRQ2FREQ 150

static WRITE_HANDLER(dmd64_ctrl_w);
static void dmd64_init(struct sndbrdData *brdData);

const struct sndbrdIntf dedmd64Intf = {
  NULL, dmd64_init, NULL, NULL,NULL, 
  dmd_data_w, dmd_busy_r, dmd64_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE16_HANDLER(crtc6845_msb_address_w);
static WRITE16_HANDLER(crtc6845_msb_register_w);
static READ16_HANDLER(crtc6845_msb_register_r);
static READ16_HANDLER(dmd64_latch_r);
static WRITE16_HANDLER(dmd64_status_w);
static INTERRUPT_GEN(dmd64_irq2);

static MEMORY_READ16_START(dmd64_readmem)
  { 0x00000000, 0x000fffff, MRA16_ROM }, /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, MRA16_RAM }, /* RAM - 0x800000 Page 0, 0x801000 Page 1*/
  { 0x00c00010, 0x00c00011, crtc6845_msb_register_r },
  { 0x00c00020, 0x00c00021, dmd64_latch_r }, /* Read the Latch from CPU*/
MEMORY_END

static MEMORY_WRITE16_START(dmd64_writemem)
  { 0x00000000, 0x000fffff, MWA16_ROM},	 /* ROM (2 X 512K)*/
  { 0x00800000, 0x0080ffff, MWA16_RAM, &dmd64RAM},	 /* RAM - 0x800000 Page 0, 0x801000 Page 1*/
  { 0x00c00010, 0x00c00011, crtc6845_msb_address_w},
  { 0x00c00012, 0x00c00013, crtc6845_msb_register_w},
  { 0x00c00020, 0x00c00021, dmd64_status_w},/* Set the Status Line*/
MEMORY_END

MACHINE_DRIVER_START(de_dmd64)
  MDRV_CPU_ADD(M68000, 6000000)
  MDRV_CPU_MEMORY(dmd64_readmem, dmd64_writemem)
  MDRV_CPU_PERIODIC_INT(dmd64_irq2, DMD64_IRQ2FREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

static void dmd64_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
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
static INTERRUPT_GEN(dmd64_irq2) {
  cpu_set_irq_line(dmdlocals.brdData.cpuNo, MC68000_IRQ_2, HOLD_LINE);
}
static WRITE16_HANDLER(crtc6845_msb_address_w)  { if (ACCESSING_MSB) crtc6845_address_0_w(offset,data>>8);  }
static WRITE16_HANDLER(crtc6845_msb_register_w) { if (ACCESSING_MSB) crtc6845_register_0_w(offset,data>>8); }
static READ16_HANDLER(crtc6845_msb_register_r)  { return crtc6845_register_0_r(offset)<<8; }

/*-- update display --*/
PINMAME_VIDEO_UPDATE(dedmd64_update) {
  UINT8 *RAM  = (UINT8 *)(dmd64RAM) + ((crtc6845_start_address_r(0) & 0x400)<<2);
  UINT8 *RAM2 = RAM + 0x800;
  tDMDDot dotCol;
  int ii,jj;

  for (ii = 1; ii <= 64; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (192/16); jj++) {
      UINT8 intens1 = 2*(RAM[1] & 0x55) + (RAM2[1] & 0x55);
      UINT8 intens2 =   (RAM[1] & 0xaa) + (RAM2[1] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      intens1 = 2*(RAM[0] & 0x55) + (RAM2[0] & 0x55);
      intens2 =   (RAM[0] & 0xaa) + (RAM2[0] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      RAM += 2; RAM2 += 2;
    }
    *line = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
#define DMD16_BANK0 2
// Steve said he measured this to 2000 on his game
// but that sometimes causes a new NMI to be triggered before the
// previous one is finished and it leads to stack overflow
#define DMD16_NMIFREQ 2000
/* HC74 bits */
#define BUSY_CLR      0x01
#define BUSY_SET      0x02
#define BUSY_CLK      0x04

static void dmd16_init(struct sndbrdData *brdData);
static WRITE_HANDLER(dmd16_ctrl_w);
static INTERRUPT_GEN(dmd16_nmi);

const struct sndbrdIntf dedmd16Intf = {
  NULL, dmd16_init, NULL, NULL,NULL, 
  dmd_data_w, dmd_busy_r, dmd16_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static READ_HANDLER(dmd16_port_r);
static WRITE_HANDLER(dmd16_port_w);

static MEMORY_READ_START(dmd16_readmem)
  { 0x0000, 0x3fff, MRA_ROM },	               /* Z80 ROM CODE*/
  { 0x4000, 0x7fff, MRA_BANKNO(DMD16_BANK0) }, /* ROM BANK*/
  { 0x8000, 0x9fff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START(dmd16_writemem)
  { 0x0000, 0x3fff, MWA_ROM },
  { 0x4000, 0x7fff, MWA_ROM },
  { 0x8000, 0x9fff, MWA_RAM },
MEMORY_END

static PORT_READ_START(dmd16_readport)
  { 0x00, 0xff, dmd16_port_r },
PORT_END

static PORT_WRITE_START(dmd16_writeport)
  { 0x00, 0xff, dmd16_port_w },
PORT_END

MACHINE_DRIVER_START(de_dmd16)
  MDRV_CPU_ADD(Z80, 4000000)
  MDRV_CPU_MEMORY(dmd16_readmem, dmd16_writemem)
  MDRV_CPU_PORTS(dmd16_readport, dmd16_writeport)
  MDRV_CPU_PERIODIC_INT(dmd16_nmi, DMD16_NMIFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

static void dmd16_setbusy(int bit, int value);
static void dmd16_setbank(int bit, int value);

static void dmd16_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
  memcpy(memory_region(DE_DMD16CPUREGION),
         memory_region(DE_DMD16ROMREGION) + memory_region_length(DE_DMD16ROMREGION)-0x4000,0x4000);
  dmdlocals.framedata = (UINT32 *)memory_region(DE_DMD16DMDREGION);
  dmd16_setbank(0x07, 0x07);
  dmd16_setbusy(BUSY_SET|BUSY_CLR,0);
}
/*--- Port decoding ----
  76543210
  10-001-- Bank0 (stat)
  10-011-- Bank1 (stat)
  10-101-- Bank2 (stat)
  11-001-- Status (stat)
  11-111-- Test (stat)
  1----0-- IDAT (mom)
  ------
  10-111-- Blanking (stat)
  11-011-- Row Data (stat)
  11-101-- Row Clock (stat)
  0----1-- CLATCH (mom)
  0----0-- COCLK (mom)

--------------------*/
static READ_HANDLER(dmd16_port_r) {
  if ((offset & 0x84) == 0x80) {
    dmd16_setbusy(BUSY_CLR, 0); dmd16_setbusy(BUSY_CLR,1);
    return dmdlocals.cmd;
  }
  dmd16_port_w(offset,0xff);
  return 0xff;
}

static void dmd16_dmdout(void) {
  UINT32 row = dmdlocals.hv5222;
  const int same = (dmdlocals.lasthv5222 == dmdlocals.hv5222);
  UINT32 *frame;

  /* Swap frame when no row is selected */
  if (dmdlocals.hv5222 == 0) dmdlocals.frame = !dmdlocals.frame;
  frame = &dmdlocals.framedata[dmdlocals.frame*0x80];

  while (row) {
    if (row & 0x01) {
      frame[0] = dmdlocals.hv5408; frame[1] = dmdlocals.hv5308;
      // low intesity is created by unlighting the dots immediatly after lighting them
      if (!same) { frame[2] = frame[0]; frame[3] = frame[1]; }
    }
    frame += 4; row >>= 1;
  }
  dmdlocals.lasthv5222 = dmdlocals.hv5222;
}

static WRITE_HANDLER(dmd16_port_w) {
  switch (offset & 0x84) {
    case 0x00: // COCLK
      data = data>>((offset & 0x03)*2);
      // switch in bits backwards to easy decoding
      dmdlocals.hv5408s = (dmdlocals.hv5408s>>1) | ((data & 0x02)?0x80000000:0);
      dmdlocals.hv5308s = (dmdlocals.hv5308s>>1) | ((data & 0x01)?0x80000000:0);
      break;
    case 0x04: // CLATCH
      dmdlocals.hv5408 = dmdlocals.hv5408s; dmdlocals.hv5308 = dmdlocals.hv5308s;
      if (dmdlocals.blnk) dmd16_dmdout();
      break;
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
        case 0xc4: // Status
          sndbrd_ctrl_cb(dmdlocals.brdData.boardNo, dmdlocals.status = data); break;
        case 0xdc: // Test
          dmd16_setbusy(BUSY_SET, data); break;
        case 0x9c: // Blanking
          dmdlocals.blnk = data; if (data) dmd16_dmdout();
          break;
        case 0xcc: // Row data
          dmdlocals.rowdata = data; break;
        case 0xd4: // Row clock
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
    dmd16_setbank(0x07, 0x07); dmd16_setbusy(BUSY_SET, 0);
    dmdlocals.rowdata = dmdlocals.rowdata = dmdlocals.blnk = 0;
    cpu_set_reset_line(dmdlocals.brdData.cpuNo, PULSE_LINE);
    dmdlocals.hv5408 = dmdlocals.hv5408s = dmdlocals.hv5308 = dmdlocals.hv5308s =
    dmdlocals.hv5222 = dmdlocals.lasthv5222 = dmdlocals.rowclk = dmdlocals.frame = 0;
  }
  dmdlocals.ctrl = data;
}

static void dmd16_setbusy(int bit, int value) {
  static int laststat = 0;
  int newstat = (laststat & ~bit) | (value ? bit : 0);
#if 1
  /* In the data-sheet for the HC74 flip-flop is says that SET & CLR are _not_
     edge triggered. For some strange reason, the DMD doesn't work unless we
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
static INTERRUPT_GEN(dmd16_nmi) { cpu_set_nmi_line(dmdlocals.brdData.cpuNo, PULSE_LINE); }

/*-- update display --*/
PINMAME_VIDEO_UPDATE(dedmd16_update) {
  UINT32 *frame = &dmdlocals.framedata[(!dmdlocals.frame)*0x80];
  tDMDDot dotCol;
  int ii,jj,kk;

  for (ii = 1; ii <= 16; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 2; jj++) {
      UINT32 tmp0 = frame[0];
      UINT32 tmp1 = frame[1];
      UINT32 tmp2 = frame[2];
      UINT32 tmp3 = frame[3];
      for (kk = 0; kk < 32; kk++) {
		//If both dots are lit, we use color 3, but if only 1, we use 1.
        *line++ = (tmp2 & 0x01) + (tmp0 & 0x01) + 1*(tmp2 & tmp0 & 0x01);
        *line++ = (tmp3 & 0x01) + (tmp1 & 0x01) + 1*(tmp3 & tmp1 & 0x01);
        tmp0 >>= 1; tmp1 >>= 1; tmp2 >>= 1; tmp3 >>= 1;
      }
      frame += 4;
    }
    *line++ = 0;
  }
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}

