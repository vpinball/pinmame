#include "driver.h"
#include "cpu/i8051/i8051.h"
#include "core.h"
#include "sndbrd.h"
#include "alvgdmd.h"

/*--------- Common DMD stuff ----------*/
static struct {
  struct sndbrdData brdData;
  int cmd, ncmd, busy, status, ctrl, bank;
  UINT32 *framedata;
  int blnk, rowdata, rowclk, frame;
} dmdlocals;

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

const struct sndbrdIntf alvgdmdIntf = {
  NULL, dmd32_init, NULL, NULL,NULL, 
  dmd_data_w, dmd_busy_r, dmd32_ctrl_w, dmd_status_r, SNDBRD_NOTSOUND
};

static WRITE_HANDLER(dmd32_bank_w);
static WRITE_HANDLER(dmd32_status_w);
static READ_HANDLER(dmd32_latch_r);
static INTERRUPT_GEN(dmd32_firq);

static READ_HANDLER(read_ext_mem)
{
	//printf("reading external address %x\n",offset);
	logerror("reading external address %x\n", offset);
	return 0;
}

static WRITE_HANDLER(write_ext_mem)
{
	//printf("writing to external address %x, data=%x\n", offset, data);
	logerror("writing to external address %x, data=%x\n", offset, data);
}

//This will need to be changed - since our program rom is 64K! (Not 32K as the schem shows)

static MEMORY_READ_START(alvgdmd_readmem)
	{ 0x0000, 0xffff, read_ext_mem },
MEMORY_END

static MEMORY_WRITE_START(alvgdmd_writemem)
	{ 0x0000, 0xffff, write_ext_mem },
MEMORY_END

static PORT_READ_START( alvgdmd_readport )
PORT_END

static PORT_WRITE_START( alvgdmd_writeport )
PORT_END

MACHINE_DRIVER_START(alvgdmd)
  MDRV_CPU_ADD(I8051, 12000000)		/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
//  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
  MDRV_INTERLEAVE(50)
MACHINE_DRIVER_END

//Use only for testing the 8031 core emulation
#ifdef MAME_DEBUG
MACHINE_DRIVER_START(test8031)
  MDRV_CPU_ADD(I8051, 12000000)		/*12 Mhz*/
  MDRV_CPU_MEMORY(alvgdmd_readmem, alvgdmd_writemem)
  MDRV_CPU_PORTS(alvgdmd_readport, alvgdmd_writeport)
//  MDRV_CPU_PERIODIC_INT(dmd32_firq, DMD32_FIRQFREQ)
MACHINE_DRIVER_END
#endif

static void dmd32_init(struct sndbrdData *brdData) {
  memset(&dmdlocals, 0, sizeof(dmdlocals));
  dmdlocals.brdData = *brdData;
#if 0
  cpu_setbank(DMD32_BANK0, dmdlocals.brdData.romRegion);
  /* copy last 16K of ROM into last 16K of CPU region*/
  memcpy(memory_region(DE_DMD32CPUREGION) + 0x8000,
         memory_region(DE_DMD32ROMREGION) + memory_region_length(DE_DMD32ROMREGION)-0x8000,0x8000);
#endif
}

static WRITE_HANDLER(dmd32_ctrl_w) {
  if ((data | dmdlocals.ctrl) & 0x01) {
    //cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, (data & 0x01) ? ASSERT_LINE : CLEAR_LINE);
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
//  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_IRQ_LINE, CLEAR_LINE);
  return dmdlocals.cmd;
}

static INTERRUPT_GEN(dmd32_firq) {
//  cpu_set_irq_line(dmdlocals.brdData.cpuNo, M6809_FIRQ_LINE, HOLD_LINE);
}

PINMAME_VIDEO_UPDATE(alvgdmd_update) {

  //UINT8 *RAM  = ((UINT8 *)dmd32RAM) + ((crtc6845_start_addr & 0x0100)<<2);
  //UINT8 *RAM2 = RAM + 0x200;
  tDMDDot dotCol;
  //For now just keep the dmd blank
  memset(&dotCol,0,sizeof(dotCol));
#if 0
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
#endif
  video_update_core_dmd(bitmap, cliprect, dotCol, layout);
  return 0;
}
