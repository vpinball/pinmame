/*Capcom Sound Hardware
  ------------------------------------
*/
#include "driver.h"
#include "core.h"
#include "cpu/i8051/i8051.h"
#include "capcom.h"
#include "capcoms.h"
#include "sndbrd.h"

/*Declarations*/

/*Interfaces*/
static struct TMS320AV120interface capcoms_TMS320AV120Int1 = {
  1,		//# of chips
  {100}		//Volume levels
};
static struct TMS320AV120interface capcoms_TMS320AV120Int2 = {
  2,		//# of chips
  {50,50}	//Volume levels
};

#if 0
/* Sound board */
const struct sndbrdIntf alvgsIntf = {
   "BSMT", alvgs_init, NULL, NULL, alvg_sndCmd_w, alvgs_data_w, NULL, alvgs_ctrl_w, alvgs_ctrl_r, SNDBRD_NODATASYNC
};
#endif

/*-- local data --*/
static struct {
  struct sndbrdData brdData;
} locals;

static MEMORY_READ_START(capcoms_readmem)
MEMORY_END

static MEMORY_WRITE_START(capcoms_writemem)
MEMORY_END

MACHINE_DRIVER_START(capcom1s)
//  MDRV_CPU_ADD(I8051, 12000000)
//  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//  MDRV_CPU_MEMORY(capcoms_readmem, capcoms_writemem)
//  MDRV_INTERLEAVE(50)
MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int1)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(capcom2s)
//  MDRV_CPU_ADD(I8051, 12000000)
//  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//  MDRV_CPU_MEMORY(capcoms_readmem, capcoms_writemem)
//  MDRV_INTERLEAVE(50)
MDRV_SOUND_ADD_TAG("tms320av120", TMS320AV120, capcoms_TMS320AV120Int2)
MACHINE_DRIVER_END

static void capcoms_init(struct sndbrdData *brdData) {
  memset(&locals, 0, sizeof(locals));
  locals.brdData = *brdData;
}
