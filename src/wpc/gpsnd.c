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

// SSU-1

static struct SN76477interface  gpSS1_sn76477Int = { 1, { 50 }, /* mixing level */
/*						   pin description		 */
	{	0 /* N/C */	},	/*	4  noise_res		 */
	{	0 /* N/C */	},	/*	5  filter_res		 */
	{	0 /* N/C */	},	/*	6  filter_cap		 */
	{	0 /* N/C */	},	/*	7  decay_res		 */
	{	0 /* N/C */	},	/*	8  attack_decay_cap  */
	{	0 /* N/C */	},	/* 10  attack_res		 */
	{	RES_K(220) 	},	/* 11  amplitude_res	 */
	{	RES_K(56.2)	},	/* 12  feedback_res 	 */
	{	1.5 /* ? */	},	/* 16  vco_voltage		 */
	{	CAP_U(0.1)	},	/* 17  vco_cap			 */
	{	RES_K(56)	},	/* 18  vco_res			 */
	{	5.0			},	/* 19  pitch_voltage	 */
	{	RES_K(220)	},	/* 20  slf_res			 */
	{	CAP_U(0.1)	},	/* 21  slf_cap			 */
	{	0 /* N/C */	},	/* 23  oneshot_cap		 */
	{	0 /* N/C */	}	/* 24  oneshot_res		 */
};

static WRITE_HANDLER(gpss1_data_w)
{ // tone frequencies       C    E             A    A low
 static double voltage[] = {2.8, 3.3, 0, 0, 0, 4.5, 2.2};
  if (data != 0x0f) {
    SN76477_set_vco_voltage(0, voltage[data]);
    SN76477_enable_w(0, 0);
  } else {
    SN76477_set_vco_voltage(0, 1.5);
    SN76477_enable_w(0, 1);
  }
}

static void gpss1_init(struct sndbrdData *brdData)
{
  /* MIXER = 0 */
  SN76477_mixer_w(0, 0);
  /* ENVELOPE is constant: pin1 = lo, pin 28 = hi */
  SN76477_envelope_w(0, 2);
}

// SSU-2/3

static struct SN76477interface  gpSS2_sn76477Int = { 3, { 50, 50, 50 }, /* mixing levels */
/*			#0			#1			#2			   pin description		*/
	{	RES_K(47),	RES_K(100),	0			},	/*	4  noise_res		*/
	{	RES_K(330),	RES_K(470),	0			},	/*	5  filter_res		*/
	{	CAP_P(680),	CAP_P(680),	0			},	/*	6  filter_cap		*/
	{	RES_M(2.2),	RES_M(2.2),	0			},	/*	7  decay_res		*/
	{	CAP_U(1),	CAP_U(0.1),	0			},	/*	8  attack_decay_cap */
	{	RES_K(4.7),	RES_K(4.7),	0			},	/* 10  attack_res		*/
	{	RES_K(68),	RES_K(68),	RES_K(68)	},	/* 11  amplitude_res	*/
	{	RES_K(57.4),RES_K(57.4),RES_K(57.4)	},	/* 12  feedback_res 	*/
	{ 	0,			1.5,/* ? */	0.8 /* ? */	},	/* 16  vco_voltage		*/
	{ 	0,			CAP_U(0.1),	CAP_U(0.1)	},	/* 17  vco_cap			*/
	{	0,			RES_K(56),	RES_K(100)	},	/* 18  vco_res			*/
	{	0,			5.0,		5.0			},	/* 19  pitch_voltage	*/
	{	0,			RES_M(2.2),	0			},	/* 20  slf_res			*/
	{	0,			CAP_U(1),	0			},	/* 21  slf_cap			*/
	{	CAP_U(10),	CAP_U(1),	0			},	/* 23  oneshot_cap		*/
	{	RES_K(100),	RES_K(330),	0			}	/* 24  oneshot_res		*/
};

static WRITE_HANDLER(gpss2_data_w)
{ // tone frequencies        D'   C'   B    A    H    G    F          E    D
  static double voltage[] = {5.7, 5.4, 4.8, 4.5, 5.1, 4.0, 3.6, 0, 0, 3.3, 3.0};
  static int howl_or_whoop = 0;
  int sb = core_gameData->hw.soundBoard & 0x01; // 1 if SSU3
  switch (data) {
    case 0x07: // gunshot
      SN76477_enable_w(1, 1);
      SN76477_mixer_w(1, 2);
      SN76477_envelope_w(1, 1);
      SN76477_vco_w(1, 1);
      SN76477_set_noise_res(1, RES_K(50)); /* 4 */
      SN76477_set_filter_res(1, RES_K(120)); /* 5 */
      SN76477_set_decay_res(1, RES_M(2.2)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.1)); /* 8 */
      SN76477_set_vco_cap(1, CAP_U(0.1)); /* 17 */
      SN76477_set_vco_res(1, RES_K(56)); /* 18 */
      SN76477_set_slf_res(1, RES_M(2.2)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(1)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(110)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x08: // rattlesnake / warble
      SN76477_enable_w(1, 1);
      SN76477_mixer_w(1, sb ? 0 : 4);
      SN76477_envelope_w(1, sb ? 1/*0*/: 1); // schematics suggest 0 for SSU-3, which won't work!
      SN76477_vco_w(1, 1);
      SN76477_set_noise_res(1, RES_K(100)); /* 4 */
      SN76477_set_filter_res(1, RES_K(120)); /* 5 */
      SN76477_set_decay_res(1, RES_M(2.2)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.1)); /* 8 */
      SN76477_set_vco_cap(1, CAP_U(0.1)); /* 17 */
      SN76477_set_vco_res(1, RES_K(56)); /* 18 */
      SN76477_set_slf_res(1, sb ? RES_K(221) : RES_K(47)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(1)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0b: // horse / pony
      SN76477_enable_w(1, 1);
      SN76477_mixer_w(1, 3);
      SN76477_envelope_w(1, 1);
      SN76477_vco_w(1, 0);
      SN76477_set_noise_res(1, RES_K(100)); /* 4 */
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_K(4.7)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.1)); /* 8 */
      SN76477_set_vco_cap(1, CAP_U(1.1)); /* 17 */
      SN76477_set_vco_res(1, RES_K(56)); /* 18 */
      SN76477_set_slf_res(1, RES_M(2.2)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(1)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0c: // howl or whoop
      SN76477_enable_w(1, 1);
      SN76477_mixer_w(1, 0);
      SN76477_envelope_w(1, 1);
      SN76477_vco_w(1, 1);
      SN76477_set_noise_res(1, RES_K(100)); /* 4 */
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_K(47)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(10.1)); /* 8 */
      SN76477_set_vco_cap(1, CAP_U(0.1)); /* 17 */
      SN76477_set_vco_res(1, RES_K(40)); /* 18 */
      SN76477_set_slf_res(1, RES_M(2.2)); /* 20 */
      SN76477_set_slf_cap(1, howl_or_whoop ? CAP_U(1.1) : CAP_U(23)); /* 21 */
      SN76477_set_oneshot_cap(1, howl_or_whoop ? CAP_U(11) : CAP_U(23)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      if (sb) howl_or_whoop = !howl_or_whoop; // alternate sound on SSU-3
      break;
    case 0x0d: // ricochet
      SN76477_enable_w(1, 1);
      SN76477_mixer_w(1, 0);
      SN76477_envelope_w(1, 1);
      SN76477_vco_w(1, 1);
      SN76477_set_noise_res(1, RES_K(100)); /* 4 */
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_M(2.2)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.2)); /* 8 */
      SN76477_set_vco_cap(1, CAP_U(0.1)); /* 17 */
      SN76477_set_vco_res(1, RES_K(28)); /* 18 */
      SN76477_set_slf_res(1, RES_M(1.1)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(2)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(110)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0e: // explosion
      SN76477_enable_w(0, 1);
      SN76477_enable_w(0, 0);
      break;
    case 0x0f: // sounds off
      SN76477_enable_w(2, 1);
      break;
    default:   // chime sounds
      SN76477_set_vco_voltage(2, voltage[data]);
      SN76477_enable_w(2, 0);
  }
}

static void gpss2_init(struct sndbrdData *brdData)
{
  /* MIXER B = 1 */
  SN76477_mixer_w(0, 2);
  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(0, 1);

  /* MIXER = 0 */
  SN76477_mixer_w(2, 0);
  /* ENVELOPE is constant: pin1 = lo, pin 28 = lo */
  SN76477_envelope_w(2, 0);
}

// MSU-1

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

static WRITE_HANDLER(gpsm_ctrl_w)
{
	logerror("snd_ctrl_w: %i\n", data);
}

static WRITE_HANDLER(gpsm_data_w)
{
    logerror("snd_data_w: %i\n", data);
}

static void gpsm_init(struct sndbrdData *brdData)
{
	memset(&gps_locals, 0x00, sizeof(gps_locals));
	gps_locals.brdData = *brdData;

//	init_m6840();

	pia_config(GPS_PIA0, PIA_STANDARD_ORDERING, &gps_pia[0]);
	pia_config(GPS_PIA1, PIA_STANDARD_ORDERING, &gps_pia[1]);
}

static WRITE_HANDLER( m6840_w ) {
    logerror("M6840: offset %d = %02x\n", offset, data);
}

static WRITE_HANDLER( mem_3000_w ) {
    logerror("Unknown output write = %02x\n", data);
}

MEMORY_READ_START(gps_readmem)
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x0810, 0x0813, pia_r(GPS_PIA0) },
  { 0x3000, 0x3fff, MRA_ROM },
  { 0x7800, 0x7fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(gps_writemem)
  { 0x0000, 0x00ff, MWA_RAM },
  { 0x0810, 0x0813, pia_w(GPS_PIA0) },
  { 0x0820, 0x0823, pia_w(GPS_PIA1) },
  { 0x0840, 0x0847, m6840_w },
  { 0x3000, 0x3000, mem_3000_w }, // for Andromeda
  { 0x3001, 0x3fff, MWA_ROM },
  { 0x7800, 0x7fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

/*-------------------
/ exported interfaces
/--------------------*/
const struct sndbrdIntf gpSSU1Intf = {
  "GPS1", gpss1_init, NULL, NULL, gpss1_data_w, gpss1_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpSSU2Intf = {
  "GPS2", gpss2_init, NULL, NULL, gpss2_data_w, gpss2_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpMSU1Intf = {
  "GPSM", gpsm_init, NULL, NULL, gpsm_data_w, gpsm_data_w, NULL, gpsm_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

MACHINE_DRIVER_START(gpSSU1)
  MDRV_SOUND_ADD(SN76477, gpSS1_sn76477Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gpSSU2)
  MDRV_SOUND_ADD(SN76477, gpSS2_sn76477Int)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gpMSU1)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579500) // NTSC quartz ???
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(gps_readmem, gps_writemem)
  MDRV_INTERLEAVE(50)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END
