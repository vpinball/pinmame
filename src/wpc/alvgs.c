/*Alvin G & Co. Sound Hardware
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/m6809/m6809.h"
#include "alvg.h"
#include "alvgs.h"
#include "machine/6821pia.h"
#include "sndbrd.h"

extern WRITE_HANDLER(alvg_sndCmd_w);

static void alvgs_init(struct sndbrdData *brdData);
static INTERRUPT_GEN(alvgs_firq);

//D6 = Watchdog
//D5 = Sound LED
static WRITE_HANDLER(watch_w)
{
	alvg_UpdateSoundLEDS(0,(data&0x40)>>6);
	if(data > 0 && !(data&0x60 || data&0x40))
		logerror("UNKNOWN DATA IN SOUND WATCH_W: data=%x\n",data);
}

const struct sndbrdIntf alvgsIntf = {
   "BSMT", alvgs_init, NULL, NULL, alvg_sndCmd_w, alvg_sndCmd_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
  //"BSMT", alvgs_init, NULL, NULL, soundlatch_w, soundlatch_w, NULL, NULL, NULL, SNDBRD_NODATASYNC
};

/* Newer 12 Voice Style BSMT Chip */
static struct BSMT2000interface alvgs_bsmt2000Int = {
  1, {24000000}, {12}, {ALVGS_ROMREGION}, {100}, {6000}, 0
};

WRITE_HANDLER(data_to_main_cpu) { logerror("SOUND DATA TO MAIN %x\n",data); }

WRITE_HANDLER(bsmt_write)
{
	static int data_hi = 0;
	static int addr = 0;

	int lo = (offset & 0x01);
	//printf("offset=%x, data=%x\n",offset,data);

	if(lo)
		BSMT2000_data_0_w(addr, ((data_hi<<8)|data), 0);
	else {
		addr = (offset>>1) & 0x7f;
		data_hi = data;
	}
}
//RDSTATE line looks to D6&D7 for a busy from BSMT
static READ_HANDLER(bsmtready_r) { return 0xc0; }

static MEMORY_READ_START(alvgs_readmem)
  { 0x0100, 0x0100, bsmtready_r },
  { 0x0800, 0x0800, soundlatch_r},
  { 0x2000, 0x3fff, MRA_RAM },
  { 0x4000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START(alvgs_writemem)
  { 0x0100, 0x0100, watch_w },
  { 0x0800, 0x0800, data_to_main_cpu },
  { 0x1000, 0x10ff, bsmt_write },
  { 0x2000, 0x3fff, MWA_RAM },
  { 0x4000, 0xffff, MWA_ROM },
MEMORY_END

MACHINE_DRIVER_START(alvgs)
  MDRV_CPU_ADD(M6809, 1000000)
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(alvgs_readmem, alvgs_writemem)
  MDRV_CPU_PERIODIC_INT(alvgs_firq, 1000000/2048) /*Not sure what the freq is.. based on a jumper setting */
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD_TAG("bsmt", BSMT2000, alvgs_bsmt2000Int)
  MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
MACHINE_DRIVER_END

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
  int bsmtData;
} alvgslocals;

static void alvgs_init(struct sndbrdData *brdData) {
  memset(&alvgslocals, 0, sizeof(alvgslocals));
  alvgslocals.brdData = *brdData;
}

static INTERRUPT_GEN(alvgs_firq) {
  cpu_set_irq_line(1, M6809_FIRQ_LINE, PULSE_LINE);
}
