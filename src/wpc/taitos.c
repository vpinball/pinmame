#include "driver.h"
#include "machine/6821pia.h"
#include "cpu/m6800/m6800.h"
#include "core.h"
#include "taito.h"
#include "snd_cmd.h"
#include "sndbrd.h"
#include "taitos.h"

/*----------------------------------------
/ Taito Sound System
/ Taito uses four different sound boards:
/ - sintetizador
/ - sintevox (sintetizador with an additonal SC-01)
/ - sintetizador with a daugher board installed
/ - sintevox with a daugher board installed
/
/ cpu: 6802
/ 0x0000 - 0x007F: RAM
/ 0x0400 - 0x0403: PIA
/ 0x1000 - 0x17FF: ROM 2
/ 0x1800 - 0x1fff: ROM 1
/ (some games like Shark or Sure Shot differ)
/ sintevox:
/ SC-01 connected to output of PIA Port B
/
/-----------------------------------------*/

#define SINTETIZADOR	0
#define SINTEVOX		1
#define SINTETIZADOR_PP	2
#define SINTEVOX_PP		3

static struct {
  struct sndbrdData brdData;
  UINT8 votrax_data;
} taitos_locals;

#define SP_PIA0  0

MEMORY_READ_START(taitos_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0080, 0x03ff, MRA_NOP },
  { 0x0400, 0x0403, pia_r(SP_PIA0) },
  { 0x0800, 0x1fff, MRA_ROM }, // 0x800 - 0xfff for sureshot
  { 0x2000, 0x3fff, MRA_ROM }, // for sharkt & lunelle
  { 0x7800, 0x7fff, MRA_ROM }, // for shock
  { 0x8400, 0x8403, pia_r(SP_PIA0) }, // for shock
  { 0xf800, 0xffff, MRA_ROM }, // reset vector
MEMORY_END

MEMORY_WRITE_START(taitos_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0400, 0x0403, pia_w(SP_PIA0) },
  { 0x8400, 0x8403, pia_w(SP_PIA0) }, // for shock
MEMORY_END

static void taitos_irq(int state) {
//	logerror("sound irq\n");
	cpu_set_irq_line(taitos_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static void taitos_nmi(int state) {
//	logerror("sound nmi\n");
	cpu_set_nmi_line(taitos_locals.brdData.cpuNo, PULSE_LINE);
}

static WRITE_HANDLER(pia0a_w)
{
//	logerror("pia0a_w: %02x\n", data);
	DAC_data_w(0, data);
}

static WRITE_HANDLER(pia0b_w)
{
//	logerror("pia0b_w: %02x\n", data);
	taitos_locals.votrax_data = data;
}

/* enable diagnostic led */
static WRITE_HANDLER(pia0ca2_w)
{
//	logerror("pia0ca2_w: %02x\n", data);
	coreGlobals.diagnosticLed = data;
}

static WRITE_HANDLER(pia0cb2_w)
{
//	logerror("pia0cb2_w: %02x\n", data);
	if (data && (taitos_locals.brdData.subType & 0x01) == SINTEVOX)
		votraxsc01_w(0, taitos_locals.votrax_data);
}

static void votrax_busy(int state)
{
//	logerror("votrax busy: %i\n", state);
	pia_set_input_ca1(SP_PIA0, state?0:1);
}

static const struct pia6821_interface sp_pia = {
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, PIA_UNUSED_VAL(0), PIA_UNUSED_VAL(0), 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ taitos_nmi, taitos_irq
};

/* sound input */
static WRITE_HANDLER(taitos_data_w)
{
	logerror("taitos_data_w: %02x\n", data);
	pia_set_input_b(SP_PIA0, data^0xff);
	pia_set_input_cb1(SP_PIA0, data?0x01:0x00);
}

static void taitos_init(struct sndbrdData *brdData)
{
	memset(&taitos_locals, 0x00, sizeof(taitos_locals));
	taitos_locals.brdData = *brdData;

	pia_config(SP_PIA0, PIA_STANDARD_ORDERING, &sp_pia);
}

struct DACinterface TAITO_dacInt =
{
  1,			/* 1 Chip */
  {25}		    /* Volume */
};

struct VOTRAXSC01interface TAITO_votrax_sc01_interface = {
	1,						/* 1 chip */
	{ 50 },					/* master volume */
	{ 8000 },				/* dynamically changing this is currently not supported */
	{ &votrax_busy }		/* set NMI when busy signal get's low */
};

/*-------------------
/ exported interface
/--------------------*/
const struct sndbrdIntf taitoIntf = {
  "TAITO", taitos_init, NULL, NULL, taitos_data_w, taitos_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(taitos_sintetizador)
  MDRV_CPU_ADD_TAG("scpu", M6802, 600000) // 0.6 MHz ??? */
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(taitos_readmem, taitos_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, TAITO_dacInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taitos_sintevox)
  MDRV_IMPORT_FROM(taitos_sintetizador)

  MDRV_SOUND_ADD(VOTRAXSC01, TAITO_votrax_sc01_interface)
MACHINE_DRIVER_END

// piggy pack board

static WRITE_HANDLER(ay8910_0_ctrl_port)
{
	// logerror("ay8910_0_ctrl_port: %0x\n", data);
	// ctrl port
	AY8910Write(0,0,data);
}

static WRITE_HANDLER(ay8910_0_data_port)
{
	// logerror("ay8910_0_data_port: %0x\n", data);
	// data port
	AY8910Write(0,1,data);
}

static WRITE_HANDLER(ay8910_1_ctrl_port)
{
	// logerror("ay8910_1_ctrl_port: %0x\n", data);
	// ctrl port
	AY8910Write(1,0,data);
}

static WRITE_HANDLER(ay8910_1_data_port)
{
	// logerror("ay8910_1_data_port: %0x\n", data);
	// data port
	AY8910Write(1,1,data);
}

static WRITE_HANDLER(unknown2000)
{
	logerror("unknown2000 write:%02x\n", data);
}

static READ_HANDLER(unknown1007)
{
	logerror("unknown1007 read\n");
	return AY8910Read(0);
}

static READ_HANDLER(unknown100d)
{
	logerror("unknown100d read\n");
	return AY8910Read(1);
}

struct AY8910interface TAITO_ay8910Int = {
	2,			/* 2 chips */
	2000000,	/* 2 MHz */
	{ 30, 30 }, /* Volume */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

MEMORY_READ_START(taitospp_readmem)
  { 0x0000, 0x007f, MRA_RAM },
//  { 0x0080, 0x03ff, MRA_NOP },
  { 0x0400, 0x0403, pia_r(SP_PIA0) },
//  { 0x0404, 0x1006, MRA_NOP },
  { 0x1007, 0x1007, unknown1007 },
//  { 0x1008, 0x100a, MRA_NOP },
  { 0x100d, 0x100d, unknown100d },
//  { 0x100e, 0x1fff, MRA_NOP },
  { 0x2000, 0x3fff, MRA_ROM },
//  { 0x4000, 0x4fff, MRA_NOP },
  { 0x5000, 0x7fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM }, /* reset vector */
MEMORY_END

MEMORY_WRITE_START(taitospp_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0400, 0x0403, pia_w(SP_PIA0) },
  { 0x1000, 0x1000, ay8910_0_ctrl_port },
  { 0x1003, 0x1003, ay8910_0_ctrl_port },
  { 0x100a, 0x100a, ay8910_0_data_port },
  { 0x100b, 0x100b, ay8910_0_data_port },
  { 0x100c, 0x100c, ay8910_1_ctrl_port },
  { 0x100e, 0x100e, ay8910_1_data_port },
  { 0x2000, 0x2000, unknown2000 },
MEMORY_END

MACHINE_DRIVER_START(taitos_sintetizadorpp)
  MDRV_CPU_ADD_TAG("scpu", M6802, 600000) // 0.6 MHz ??? */
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(taitospp_readmem, taitospp_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(DAC, TAITO_dacInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_SOUND_ADD(AY8910, TAITO_ay8910Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(taitos_sintevoxpp)
  MDRV_IMPORT_FROM(taitos_sintetizadorpp)

  MDRV_SOUND_ADD(VOTRAXSC01, TAITO_votrax_sc01_interface)
MACHINE_DRIVER_END
