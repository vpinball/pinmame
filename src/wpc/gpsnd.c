#include "driver.h"
#include "machine/6821pia.h"
#include "sound/sn76477.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "gpsnd.h"

/*----------------------------------------
/ Game Plan Sound System
/ 4 different boards:
/
/ SSU-1:   1 x SN76477
/ SSU-2/3: 3 x SN76477
/ MSU-1:   6802 CPU, 2 PIAs, 6840 timer,
/          but only discrete circuits used!
/-----------------------------------------*/
static struct {
  struct sndbrdData brdData;
} gps_locals;

#define GPS_PIA0  0
#define GPS_PIA1  1

static void gps_irq(int state) {
	logerror("sound irq\n");
	cpu_set_irq_line(gps_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static WRITE_HANDLER(pia0a_w)
{
	logerror("pia0a_w: %02x\n", data);
}

static WRITE_HANDLER(pia0b_w)
{
	logerror("pia0b_w: %02x\n", data);
}

static WRITE_HANDLER(pia0ca2_w)
{
	logerror("pia0ca2_w: %02x\n", data);
}

static WRITE_HANDLER(pia0cb2_w)
{
	logerror("pia0cb2_w: %02x\n", data);
}

static WRITE_HANDLER(pia1a_w)
{
	logerror("pia1a_w: %02x\n", data);
}

static WRITE_HANDLER(pia1b_w)
{
	logerror("pia1b_w: %02x\n", data);
}

static WRITE_HANDLER(pia1ca2_w)
{
	logerror("pia1ca2_w: %02x\n", data);
}

static WRITE_HANDLER(pia1cb2_w)
{
	logerror("pia1cb2_w: %02x\n", data);
}

static const struct pia6821_interface gps_pia[] = {
{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ 0, 0
},
{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
  /*irq: A/B           */ 0, gps_irq
}};

static WRITE_HANDLER(gps_ctrl_w)
{
	logerror("snd_ctrl_w: %i\n", data);
}

static WRITE_HANDLER(gps_data_w)
{
    logerror("snd_data_w: %i\n", data);
}

static void gps_init(struct sndbrdData *brdData)
{
	memset(&gps_locals, 0x00, sizeof(gps_locals));
	gps_locals.brdData = *brdData;

//	init_m6840();

	pia_config(GPS_PIA0, PIA_STANDARD_ORDERING, &gps_pia[0]);
	pia_config(GPS_PIA1, PIA_STANDARD_ORDERING, &gps_pia[1]);
}

static WRITE_HANDLER( m6840_1_w ) {
    logerror("M6840 #1: offset %d = %02x\n", offset, data);
}

static WRITE_HANDLER( m6840_2_w ) {
    logerror("M6840 #2: offset %d = %02x\n", offset, data);
}

static WRITE_HANDLER( m6840_3_w ) {
    logerror("M6840 #3: offset %d = %02x\n", offset, data);
}

static READ_HANDLER( m6840_2_r ) {
    logerror("M6840 #2: offset %d READ\n", offset);
    return 0;
}

static READ_HANDLER( m6840_3_r ) {
    logerror("M6840 #3: offset %d READ\n", offset);
    return 0;
}

MEMORY_READ_START(gps_readmem)
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x0810, 0x0813, pia_r(GPS_PIA0) },
  { 0x3000, 0x3fff, MRA_ROM },
  { 0x7000, 0x7fff, MRA_ROM },
  { 0x8d00, 0x8dff, MRA_RAM },
  { 0xbe30, 0xbe37, m6840_2_r }, // More M6840s???
  { 0xbe38, 0xbe3f, m6840_3_r }, // More M6840s???
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(gps_writemem)
  { 0x0000, 0x00ff, MWA_RAM },
  { 0x0810, 0x0813, pia_w(GPS_PIA0) },
  { 0x0820, 0x0823, pia_w(GPS_PIA1) },
  { 0x0840, 0x0847, m6840_1_w },
  { 0x3000, 0x3fff, MWA_ROM },
  { 0x7000, 0x7fff, MWA_ROM },
  { 0x8d00, 0x8dff, MWA_RAM },
  { 0xbe30, 0xbe37, m6840_2_w }, // More M6840s???
  { 0xbe38, 0xbe3f, m6840_3_w }, // More M6840s???
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf gpSSU1Intf = {
  gps_init, NULL, NULL, gps_data_w, NULL, gps_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpSSU2Intf = {
  gps_init, NULL, NULL, gps_data_w, NULL, gps_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpMSU1Intf = {
  gps_init, NULL, NULL, gps_data_w, NULL, gps_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gpMSU1)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579500) // NTSC quartz ???
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(gps_readmem, gps_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END
