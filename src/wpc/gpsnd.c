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
/ MSU-3:   6802 CPU, 1 PIA, DAC
/-----------------------------------------*/

// SSU-1
// support added for MSU-1 sound board by Oliver Kaegi (23/01/2006)
//
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
{ // tone frequencies          C    E             A    A low
  static double voltage[16] = {2.8, 3.3, 0, 0, 0, 4.5, 2.2};
  data &= 0x0f;
  if (voltage[data]) {
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
{ // tone frequencies          D'   C'   B    A    H    G    F          E    D
  static double voltage[16] = {5.7, 5.4, 4.8, 4.5, 5.1, 4.0, 3.6, 0, 0, 3.3, 3.0};
  static int howl_or_whoop = 0;
  int sb = core_gameData->hw.soundBoard & 0x01; // 1 if SSU3
  data &= 0x0f;
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
  }
  if (voltage[data]) {
    SN76477_set_vco_voltage(2, voltage[data]);
    SN76477_enable_w(2, 0);
  } else {
    SN76477_set_vco_voltage(2, 0.8);
    SN76477_enable_w(2, 1);
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

// SSU-4

static struct SN76477interface  gpSS4_sn76477Int = { 4, { 50, 50, 50, 50 }, /* mixing levels */
/*			#0			#1			#2			#3			  pin  description		*/
	{	RES_K(47),	RES_K(100),	0,			0			},	/*	4  noise_res		*/
	{	RES_K(330),	RES_K(470),	0,			0			},	/*	5  filter_res		*/
	{	CAP_P(680),	CAP_P(680),	0,			0			},	/*	6  filter_cap		*/
	{	RES_M(2.2),	RES_M(2.2),	0,			0			},	/*	7  decay_res		*/
	{	CAP_U(1),	CAP_U(0.1),	0,			0			},	/*	8  attack_decay_cap */
	{	RES_K(4.7),	RES_K(4.7),	0,			0			},	/* 10  attack_res		*/
	{	RES_K(68),	RES_K(68),	RES_K(68),	RES_K(68),	},	/* 11  amplitude_res	*/
	{	RES_K(57.4),RES_K(57.4),RES_K(57.4),RES_K(57.4)	},	/* 12  feedback_res 	*/
	{ 	0,			0,			0.8,/* ? */	0			},	/* 16  vco_voltage		*/
	{ 	0,			CAP_U(0.1),	CAP_U(0.1),	CAP_U(0.1),	},	/* 17  vco_cap			*/
	{	0,			RES_K(56),	RES_K(100),	RES_K(100),	},	/* 18  vco_res			*/
	{	0,			5.0,		5.0,		0			},	/* 19  pitch_voltage	*/
	{	0,			RES_M(2.2),	0,			RES_M(4.7)	},	/* 20  slf_res			*/
	{	0,			CAP_U(0.1),	0,			CAP_U(47)	},	/* 21  slf_cap			*/
	{	CAP_U(10),	CAP_U(1),	0,			0			},	/* 23  oneshot_cap		*/
	{	RES_K(100),	RES_K(330),	0,			0			}	/* 24  oneshot_res		*/
};


static mame_timer *capTimer;
static void capTimer_timer(int n) {
  static int offset = 1;
  static int pin18res = 320;
  pin18res += offset;
  if (pin18res > 419)
    offset = -1;
  else if (pin18res < 321)
    offset = 1;
  SN76477_set_vco_res(3, RES_K(pin18res)); /* 18 */
}

static WRITE_HANDLER(gpss4_data_w)
{ // tone frequencies             C'   B    A
  static double voltage[16] = {0, 5.4, 4.8, 4.5};
  data &= 0x0f;
  if (data < 0x0f) SN76477_enable_w(1, 1);
  switch (data) {
    case 0x00: // stop wave
      SN76477_enable_w(3, 1);
      timer_adjust(capTimer, TIME_NEVER, 0, TIME_NEVER);
      break;
    case 0x06: // start wave
      SN76477_enable_w(3, 0);
      timer_adjust(capTimer, 0.005, 0, 0.005);
      break;
    case 0x07: // twang
      SN76477_mixer_w(1, 0);
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_M(2.2)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.2)); /* 8 */
      SN76477_set_vco_res(1, RES_K(28)); /* 18 */
      SN76477_set_slf_res(1, RES_K(99.5)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(0.1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(2)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(110)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x08: // spark
      SN76477_mixer_w(1, 4);
      SN76477_set_filter_res(1, RES_K(120)); /* 5 */
      SN76477_set_decay_res(1, RES_K(4.7)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.1)); /* 8 */
      SN76477_set_vco_res(1, RES_K(56)); /* 18 */
      SN76477_set_slf_res(1, RES_K(99.5)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(0.1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(1)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0b: // siren
      SN76477_mixer_w(1, 0);
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_K(47)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(10.1)); /* 8 */
      SN76477_set_vco_res(1, RES_K(8.5)); /* 18 */
      SN76477_set_slf_res(1, RES_M(2.2)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(1)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0c: // howl
      SN76477_mixer_w(1, 0);
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_K(47)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(10.1)); /* 8 */
      SN76477_set_vco_res(1, RES_K(40)); /* 18 */
      SN76477_set_slf_res(1, RES_M(2.2)); /* 20 */
      SN76477_set_slf_cap(1, CAP_U(22.1)); /* 21 */
      SN76477_set_oneshot_cap(1, CAP_U(11)); /* 23 */
      SN76477_set_oneshot_res(1, RES_K(330)); /* 24 */
      SN76477_enable_w(1, 0);
      break;
    case 0x0d: // warble
      SN76477_mixer_w(1, 0);
      SN76477_set_filter_res(1, RES_K(470)); /* 5 */
      SN76477_set_decay_res(1, RES_M(2.2)); /* 7 */
      SN76477_set_attack_decay_cap(1, CAP_U(0.2)); /* 8 */
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
  }
  if (voltage[data]) {
    SN76477_set_vco_voltage(2, voltage[data]);
    SN76477_enable_w(2, 0);
  } else {
    SN76477_set_vco_voltage(2, 0.8);
    SN76477_enable_w(2, 1);
  }
}

static void gpss4_init(struct sndbrdData *brdData)
{
  capTimer = timer_alloc(capTimer_timer);
  timer_adjust(capTimer, TIME_NEVER, 0, TIME_NEVER);

  /* MIXER (pins 27,25,26) = 010 */
  SN76477_mixer_w(0, 2);
  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(0, 1);

  /* ENVELOPE is constant: pin1 = hi, pin 28 = lo */
  SN76477_envelope_w(1, 1);
  SN76477_vco_w(1, 1);

  /* MIXER = 0 */
  SN76477_mixer_w(2, 0);
  /* ENVELOPE is constant: pin1 = lo, pin 28 = lo */
  SN76477_envelope_w(2, 0);

  /* MIXER = 0 */
  SN76477_mixer_w(3, 0);
  /* ENVELOPE is constant: pin1 = lo, pin 28 = hi */
  SN76477_envelope_w(3, 2);
  SN76477_vco_w(3, 1);
}

// MSU-1 and MSU-3

#define MSU1_INTCLOCK    894875  // clock speed in hz of msu_1 board ! 

static  INT16  volumemsu1[] = {
	00,  00,  35, 40, 45, 50, 55, 60, 65, 70,  75,  80,  85,  90,95,100,100       
};
//	30,  30,  40, 40, 50, 50, 60, 60, 70, 70,  80,  80,  90,  90,100,100,100       
//              100,  100,  100, 100, 100, 100, 100, 100, 100, 100,  100,  100,  100,  100,100,100,100

static  INT16  sineWaveext[32000]; // Noise wave

/*-- m6840 interface --*/
static struct {
  int c0;
  int ax[9];
  int axb[9];	
  UINT16 timer1,timer2,timer3;
} m6840d;



static struct {
  struct sndbrdData brdData;
  UINT8 sndCmd;
  int stateca1;
  UINT16 timlat1,timlat2,timlat3;
  UINT16 timlats1,timlats2,timlats3;  
  int    cr1,cr2,cr3, channel,timp1,timp2,timp3, tfre1,tfre2,tfre3 ;
  int    reset;
} gps_locals;

#define GPS_PIA0  0
#define GPS_PIA1  1

static void gps_irq(int state) {
//	logerror("sound irq\n");
	cpu_set_irq_line(gps_locals.brdData.cpuNo, M6802_IRQ_LINE, state ? ASSERT_LINE : CLEAR_LINE);
}

static READ_HANDLER(pia0b_r)
{
	return gps_locals.sndCmd;
}

static WRITE_HANDLER(pia0a_w)
{
// pia0 a channels full emulated
	int indexq1,indexq2;
	logerror("pia0a_w: %02x\n", data);
	indexq1 = data  & 0x0f;
	indexq2 = (data  & 0xf0) >> 4;
  	mixer_set_volume(gps_locals.channel,volumemsu1[indexq1]); 	// Q1
  	mixer_set_volume(gps_locals.channel+1,volumemsu1[indexq2]);  	// q2
}

static WRITE_HANDLER(pia0b_w)
{
	logerror("pia0b_w: %02x\n", data);
}

static WRITE_HANDLER(pia0ca2_w)
{
	logerror("%04x:pia0ca2_w: %02x\n",activecpu_get_previouspc(), data);
	coreGlobals.diagnosticLed = (coreGlobals.diagnosticLed & 0x01) | (data << 1);
}

static READ_HANDLER(pia0ca1_r)
{
	logerror("%04x:pia0ca1_r: %02x\n",activecpu_get_previouspc(),gps_locals.stateca1);
	return gps_locals.stateca1;
}


static WRITE_HANDLER(pia0cb2_w)
{
	logerror("pia0cb2_w: %02x\n", data);
}

static WRITE_HANDLER(pia1a_w)
{
// pia1 a channels 0 - 3 emulated
	int indexq3,indexn;
	logerror("pia1a_w: %02x\n", data);
	indexq3 = data  & 0x0f;
	indexn = (data  & 0xf0) >> 4;
  	mixer_set_volume(gps_locals.channel+2,volumemsu1[indexq3]);       // q3
// pia a channels 4 - 7 seems to be volume of the noise generator
  	mixer_set_volume(gps_locals.channel+3,volumemsu1[indexn]);       // noise
}

static void playnoise(int param){
   int f;

   f = (625000 / 23) / param;
   mixer_set_sample_frequency(gps_locals.channel+3, f );
   logerror("*** playsam noise frequenz %08d data  ***\n",f);
   logerror("*** playsam noise param %08d data  ***\n",param);
}


static WRITE_HANDLER(pia1b_w)
{
	int startnoise;
	logerror("pia1b_w: %02x\n", data);
// pia1 b channels 0 - 2 don't know, you can have a wav file from the autor (okaegi) with the
// orignal wav sound (sharpshooter 2, soundcmd 05)
// pia1 b channels 3 - 7 seems to be start value from noise generator
	startnoise = (data  & 0xf8) >> 3;
	if (startnoise) playnoise(startnoise);
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
//  /*i: A/B,CA/B1,CA/B2 */ 0, pia0b_r, PIA_UNUSED_VAL(1), 0, 0, 0,
  /*i: A/B,CA/B1,CA/B2 */ 0, pia0b_r, pia0ca1_r, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia0a_w, pia0b_w, pia0ca2_w, pia0cb2_w,
  /*irq: A/B           */ 0, gps_irq
},
{
  /*i: A/B,CA/B1,CA/B2 */ 0, 0, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ pia1a_w, pia1b_w, pia1ca2_w, pia1cb2_w,
  /*irq: A/B           */ 0, 0
},
{
  /*i: A/B,CA/B1,CA/B2 */ 0, pia0b_r, 0, 0, 0, 0,
  /*o: A/B,CA/B2       */ 0, 0, pia0ca2_w, 0,
  /*irq: A/B           */ 0, gps_irq
}};

static WRITE_HANDLER(gpsm_ctrl_w)
{
	logerror("snd_ctrl_w: %02x\n", data);
}

static WRITE_HANDLER(gpsm_data_w)
{
// only four bits from mpu, upper ones are high
    logerror("snd_data_w: %02x\n", data);
    gps_locals.sndCmd = 0xf0 | (data & 0x0f);
    pia_set_input_b(GPS_PIA0, gps_locals.sndCmd);
}

static  INT16  sineWaveinp[] = {
0, 8192,  16384,  24576, 32767,  24576,  16384,  8192,	0,-8192, -16384, -24576,-32767,	-24576,	-16384,	-8192
}; // 6840 wave



static void oneshoot (int param) {

   logerror("oneshoot time ca1 started \n");
   gps_locals.stateca1 = 1;
}

static void playsam1(int param){
// timer 1 (q1) is easy wave 
   if ((gps_locals.cr1 & 0x80)  && (gps_locals.timlat1 > 0) && (gps_locals.reset == 0))   { // output is enabled...
  	mixer_play_sample_16(gps_locals.channel,sineWaveinp, sizeof(sineWaveinp), gps_locals.tfre1*sizeof(sineWaveinp) / 2 / 1.137, 1);
 	if (mixer_is_sample_playing(gps_locals.channel))	{	// is already playing
 		 mixer_set_sample_frequency(gps_locals.channel,(gps_locals.tfre1*sizeof(sineWaveinp) / 2) );
 	} else	{
	  	mixer_play_sample_16(gps_locals.channel,sineWaveinp, sizeof(sineWaveinp), gps_locals.tfre1*sizeof(sineWaveinp) / 2, 1);
		logerror("*** playsam Q1 start %04d ***\n",gps_locals.tfre1);
	}
	}
}

static void playsam2(int param){
// timer 2 (q2) is easy wave 
   if ((gps_locals.cr2 & 0x80)  && (gps_locals.timlat2 > 0) && (gps_locals.reset == 0))   { // output is enabled...
	if (mixer_is_sample_playing(gps_locals.channel+1))	{	// is already playing
		 mixer_set_sample_frequency(gps_locals.channel+1,(gps_locals.tfre2*sizeof(sineWaveinp) / 2) );
	} else	{
 		mixer_play_sample_16(gps_locals.channel+1,sineWaveinp, sizeof(sineWaveinp), gps_locals.tfre2*sizeof(sineWaveinp) / 2 , 1);
		logerror("*** playsam Q2 start %04d ***\n",gps_locals.tfre2);
	}
	}	
}

static void playsam3(int param){
// timer 3 (q3) is easy wave 
   if ((gps_locals.cr3 & 0x80)  && (gps_locals.timlat3 > 0) && (gps_locals.reset == 0))   { // output is enabled...
 	if (mixer_is_sample_playing(gps_locals.channel+2))	{	// is already playing
 		 mixer_set_sample_frequency(gps_locals.channel+2,(gps_locals.tfre3*sizeof(sineWaveinp) / 2) );
 	} else	{
	 	mixer_play_sample_16(gps_locals.channel+2,sineWaveinp, sizeof(sineWaveinp), gps_locals.tfre3*sizeof(sineWaveinp) / 2 , 1);
		logerror("*** playsam Q3 start %04d ***\n",gps_locals.tfre3);
	}
	}
}



static void m6840_pulse (int param) {
// param = 0x02 -> internal 6840 clock
// decrase timers and update interface
  if (((gps_locals.cr1 & 0x02) == param) && (gps_locals.cr1 & 0x80) && (gps_locals.reset ==0)) {
  	if (m6840d.timer1 > 0) {
        	m6840d.timer1--;
  	}
  	if ((m6840d.timer1 == 0) && (gps_locals.timlat1 != 0)) {
    		m6840d.timer1 = gps_locals.timlat1;
    		if (gps_locals.timlat1 != gps_locals.timlats1) {
    			playsam1(0);
    		}
    		gps_locals.timlats1 = gps_locals.timlat1;
    		gps_locals.timp1 =  (gps_locals.timp1 ? 0 : 1);
    	}
  }
  if (((gps_locals.cr2 & 0x02) == param) && (gps_locals.cr2 & 0x80) && (gps_locals.reset ==0)) {
  	if (m6840d.timer2 > 0) {
        	m6840d.timer2--;
  	}
  	if ((m6840d.timer2 == 0) && (gps_locals.timlat2 != 0)) {
    		m6840d.timer2 = gps_locals.timlat2;
    		if (gps_locals.timlat2 != gps_locals.timlats2) {
    			playsam2(0);
    		}
    		gps_locals.timlats2 = gps_locals.timlat2;
    		gps_locals.timp2 =  (gps_locals.timp2 ? 0 : 1);

    	}
  }

  if (((gps_locals.cr3 & 0x02) == param) && (gps_locals.cr3 & 0x80) && (gps_locals.reset ==0)) {
  	if (m6840d.timer3 > 0) {
        	m6840d.timer3--;
  	}
  	if ((m6840d.timer3 == 0) && (gps_locals.timlat3 != 0)) {
    		m6840d.timer3 = gps_locals.timlat3;

    		if (gps_locals.timlat3 != gps_locals.timlats3) {
    			playsam3(0);
    		}
    		gps_locals.timlats3 = gps_locals.timlat3;


    		gps_locals.timp3 =  (gps_locals.timp3 ? 0 : 1);
    	}
  }

}



static void gpsm_init(struct sndbrdData *brdData)
{
  	int mixing_levels[4] = {25,25,25,25};
	int i;
  	int s = 0;
  	memset(&gps_locals, 0x00, sizeof(gps_locals));
	gps_locals.brdData = *brdData;
  	for (i = 0;i < 32000;i++) {
    		s =  (s ? 0 : 1);
    		if (s) {
      			sineWaveext[i] = rand();
    		} else
      			sineWaveext[i] = 0-rand();
  	}
	pia_config(GPS_PIA0, PIA_STANDARD_ORDERING, &gps_pia[0]);
	pia_config(GPS_PIA1, PIA_STANDARD_ORDERING, &gps_pia[1]);
	gps_locals.channel = mixer_allocate_channels(4, mixing_levels);
  	mixer_set_name  (gps_locals.channel, "MC6840 #Q1");   // 6840 Output timer 1 (q1) is easy wave
  	mixer_set_volume(gps_locals.channel,0); 
  	mixer_set_name  (gps_locals.channel+1,"MC6840 #Q2");  // 6840 Output timer 2 (q2) is easy wave
  	mixer_set_volume(gps_locals.channel+1,0);  
  	mixer_set_name  (gps_locals.channel+2,"MC6840 #Q3");  // 6840 Output timer 3 (q3) is easy wave
  	mixer_set_volume(gps_locals.channel+2,0);  
  	mixer_set_name  (gps_locals.channel+3,"Noise");  // Noise generator
  	mixer_set_volume(gps_locals.channel+3,0);  
   	mixer_play_sample_16(gps_locals.channel+3,sineWaveext, sizeof(sineWaveext), 625000 , 1);
        gps_locals.stateca1 = 0;
        timer_set(TIME_IN_NSEC(814000000),0,oneshoot); // fire ca1 only once
//
// this time should run to emulate the 6840 correctly, but it is not needed for gampelan games i think
// because the sound rum never reads back the decreased values from the m6840
//
//        timer_pulse(TIME_IN_HZ(MSU1_INTCLOCK),0x02,m6840_pulse); // start internal clock 6840
//
}





static void softreset (int param) {
  gps_locals.reset = param;
  if (gps_locals.reset) { // reset
	m6840d.timer1 = gps_locals.timlat1;
	m6840d.timer2 = gps_locals.timlat2;
	m6840d.timer3 = gps_locals.timlat3;
	gps_locals.timp1 = 0;
	gps_locals.timp2 = 0;
	gps_locals.timp3 = 0;
	mixer_stop_sample(gps_locals.channel);
	logerror ("Playsam Q1 off ");	
	mixer_stop_sample(gps_locals.channel+1);
	logerror ("Playsam Q2 off ");	
	mixer_stop_sample(gps_locals.channel+2);
	logerror ("Playsam Q3 off ");	
	gps_locals.timlats1 = 0;
	gps_locals.timlats2 = 0;
	gps_locals.timlats3 = 0;
  } else {
  }
}





static WRITE_HANDLER(m6840_w ) {
  int w;
  long int w1;
//  logerror("M6840: offset %d = %02x\n", offset, data);
  m6840d.ax[offset]=data;
  if (offset == 3) {
	gps_locals.timlat1 = m6840d.ax[offset] + m6840d.ax[(offset-1)] * 256;
	m6840d.timer1 = gps_locals.timlat1;
	w1 = MSU1_INTCLOCK / (2 * (m6840d.timer1 + 1));
	gps_locals.tfre1 = w1;
    	logerror("%04x: m6840_w  timlat1 loaded %04x freq %04d  \n", activecpu_get_previouspc(), gps_locals.timlat1,gps_locals.tfre1);
  	if (gps_locals.timlat1 == 0) {
		gps_locals.timlats1 = 0;
  	 	mixer_stop_sample(gps_locals.channel);
		logerror ("Playsam Q1 off\n");
	}
  }
  if (offset == 5) {
	gps_locals.timlat2 = m6840d.ax[offset] + m6840d.ax[(offset-1)] * 256;
	m6840d.timer2 = gps_locals.timlat2;
	gps_locals.tfre2 = MSU1_INTCLOCK / (2 * (m6840d.timer2 + 1));
    	logerror("%04x: m6840_w  timlat2 loaded %04x freq %04d  \n", activecpu_get_previouspc(), gps_locals.timlat2,gps_locals.tfre2);
  	if (gps_locals.timlat2 == 0) {
		gps_locals.timlats2 = 0;
  	 	mixer_stop_sample(gps_locals.channel+1);
		logerror ("Playsam Q2 off\n");
	}

  }
  if (offset == 7) {
	gps_locals.timlat3 = m6840d.ax[offset] + m6840d.ax[(offset-1)] * 256;
	m6840d.timer3 = gps_locals.timlat3;
	gps_locals.tfre3 = (MSU1_INTCLOCK / (2 * (m6840d.timer3 + 1)));
  	logerror("%04x: m6840_w  timlat3 loaded %04x freq %04d  \n", activecpu_get_previouspc(), gps_locals.timlat3,gps_locals.tfre3);
  	if (gps_locals.timlat3 == 0) {
  	 	mixer_stop_sample(gps_locals.channel+2);
		gps_locals.timlats3 = 0;
		logerror ("Playsam Q3 off\n");
	}


  }
  if (offset == 1)  {
	gps_locals.cr2= m6840d.ax[offset];
 	logerror("%04x: m6840_w  CR2 %02x       ", activecpu_get_previouspc(), gps_locals.cr2);
	if ((gps_locals.cr2 & 0x80) == 0) {
  	}
	if (gps_locals.cr2 & 0x80)  {
		logerror ("Output enabl ");
    		if (gps_locals.timlat2 != gps_locals.timlats2) {
    			playsam2(0);
    			gps_locals.timlats2 = gps_locals.timlat2;
    		}
	} else {
//		logerror ("Output OFF   ");
		logerror ("PlaysamQ2off ");	
		gps_locals.timlats2 = 0;
		mixer_stop_sample(gps_locals.channel+1);
	}
	if (gps_locals.cr2 & 0x40)  {
		logerror ("Inter  ENABLE ");
	} else {
		logerror ("Inter  off    ");
	}
	w = (gps_locals.cr2 & 0x38) >> 3;
 	logerror ("Mode (N 2)   %01x ",w);
	if (gps_locals.cr2 & 0x04)  {
		logerror ("count d 8 ");
	} else {
		logerror ("count 16  ");
	}
	if (gps_locals.cr2 & 0x02)  {
		logerror ("int clock ");
	} else {
		logerror ("ext clock ");
	}
	logerror ("\n");
        }

	if (offset == 0) {
	        if ((gps_locals.cr2 & 0x01) == 0x01) {
			gps_locals.cr1 = m6840d.ax[offset];
			logerror("%04x: m6840_w  CR1 %02x ", activecpu_get_previouspc(), gps_locals.cr1);
// check reset very early !!!
			if (gps_locals.cr1 & 0x01)  {
				logerror ("reset ");
				softreset(1);
			}
			else {
				softreset(0);
				logerror ("norm  ");
			}
			if (gps_locals.cr1 & 0x80)  {
				logerror ("Output enabl ");
		    		if (gps_locals.timlat1 != gps_locals.timlats1) {
    					playsam1(0);
    					gps_locals.timlats1 = gps_locals.timlat1;
    				}
			}
			else {
//				logerror ("Output OFF   ");
				logerror ("PlaysamQ1off ");
				gps_locals.timlats1 = 0;
				mixer_stop_sample(gps_locals.channel);
			}
			if (gps_locals.cr1 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (gps_locals.cr1 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (gps_locals.cr1 & 0x04)  {
				logerror ("count d 8 ");
			}
			else {
				logerror ("count 16  ");
			}
			if (gps_locals.cr1 & 0x02)  {
				logerror ("int clock ");
			}
			else {
				logerror ("ext clock ");
			}
			logerror ("\n");
        	} else {
			gps_locals.cr3 = m6840d.ax[offset];
			logerror("%04x: m6840_w  CR3 %02x       ", activecpu_get_previouspc(), gps_locals.cr3);
			if (gps_locals.cr3 & 0x80)  {
				logerror ("Output enabl ");
		    		if (gps_locals.timlat3 != gps_locals.timlats3) {
    					playsam3(0);
    					gps_locals.timlats3 = gps_locals.timlat3;
    				}
			} else {
				logerror ("PlaysamQ3off ");
				gps_locals.timlats3 = 0;
				mixer_stop_sample(gps_locals.channel+2);				
			}
			if (gps_locals.cr3 & 0x40)  {
				logerror ("Inter  ENABLE ");
			}
			else {
				logerror ("Inter  off    ");
			}
			w = (gps_locals.cr3 & 0x38) >> 3;
 			logerror ("Mode (N 2)   %01x ",w);
			if (gps_locals.cr3 & 0x04)  {
				logerror ("count d 8 ");
			} else {
				logerror ("count 16  ");
			}
			if (gps_locals.cr3 & 0x02)  {
				logerror ("int clock ");
			} else {
				logerror ("ext clock ");
			}
			if (gps_locals.cr3 & 0x01)  {
				logerror ("clock / 8 ");
			} else {
				logerror ("clock / 1 ");
			}
			logerror ("\n");
       		}
       }
  }






static void pia_cb1_w(int data) {
	static int last;
    pia_set_input_cb1(GPS_PIA0, (last = !last));
}



MEMORY_READ_START(gps_readmem)
  { 0x0000, 0x00ff, MRA_RAM },
  { 0x0810, 0x0813, pia_r(GPS_PIA0) },
  { 0x0820, 0x0823, pia_r(GPS_PIA1) },  // is important for ssh 2 , makes an init write and read with all pias ! ok
  { 0x3000, 0x3fff, MRA_ROM },
  { 0x7800, 0x7fff, MRA_ROM },
  { 0xf000, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(gps_writemem)
  { 0x0000, 0x00ff, MWA_RAM },
  { 0x0810, 0x0813, pia_w(GPS_PIA0) },
  { 0x0820, 0x0823, pia_w(GPS_PIA1) },
  { 0x0840, 0x0847, m6840_w },
  { 0x3001, 0x3fff, MWA_ROM },
  { 0x7800, 0x7fff, MWA_ROM },
  { 0xf000, 0xffff, MWA_ROM },
MEMORY_END

static void gpsm3_init(struct sndbrdData *brdData) {
	memset(&gps_locals, 0, sizeof(gps_locals));
	gps_locals.brdData = *brdData;

	pia_config(GPS_PIA0, PIA_STANDARD_ORDERING, &gps_pia[2]);
}

static WRITE_HANDLER(gpsm3_data_w) {
    gps_locals.sndCmd = 0xf0 | (data & 0x0f);
    pia_set_input_b(GPS_PIA0, gps_locals.sndCmd);
}

static struct DACinterface msu3_dacInt = { 1, { 30 }};

MEMORY_READ_START(gps3_readmem)
  { 0x0000, 0x007f, MRA_RAM },
  { 0x0810, 0x0813, pia_r(GPS_PIA0) },
  { 0x3800, 0x3fff, MRA_ROM },
  { 0x7800, 0x7fff, MRA_ROM },
  { 0xf800, 0xffff, MRA_ROM },
MEMORY_END

MEMORY_WRITE_START(gps3_writemem)
  { 0x0000, 0x007f, MWA_RAM },
  { 0x0810, 0x0813, pia_w(GPS_PIA0) },
  { 0x3000, 0x3000, DAC_0_data_w },
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

const struct sndbrdIntf gpSSU4Intf = {
  "GPS4", gpss4_init, NULL, NULL, gpss4_data_w, gpss4_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpMSU1Intf = {
  "GPSM", gpsm_init, NULL, NULL, gpsm_data_w, gpsm_data_w, NULL, gpsm_ctrl_w, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};

const struct sndbrdIntf gpMSU3Intf = {
  "GPSM3", gpsm3_init, NULL, NULL, gpsm3_data_w, gpsm3_data_w, NULL, NULL, NULL, SNDBRD_NODATASYNC|SNDBRD_NOCTRLSYNC
};


static int msu1_sh_start(const struct MachineSound *msound)  {


  return 0;
}




static void msu1_sh_stop(void) {
	mixer_stop_sample(gps_locals.channel);
	mixer_stop_sample(gps_locals.channel+1);
	mixer_stop_sample(gps_locals.channel+2);	
	mixer_stop_sample(gps_locals.channel+3);

}

static struct CustomSound_interface msu1_custInt = {msu1_sh_start, msu1_sh_stop};


MACHINE_DRIVER_START(gpSSU1)
  MDRV_SOUND_ADD(SN76477, gpSS1_sn76477Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gpSSU2)
  MDRV_SOUND_ADD(SN76477, gpSS2_sn76477Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gpSSU4)
  MDRV_SOUND_ADD(SN76477, gpSS4_sn76477Int)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
MACHINE_DRIVER_END

MACHINE_DRIVER_START(gpMSU1)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579500/4) // NTSC quartz ???
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(gps_readmem, gps_writemem)
  MDRV_SOUND_ADD(CUSTOM, msu1_custInt) // uses an MC6840, to be implemented yet!
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_TIMER_ADD(pia_cb1_w, 413.793 * 2)
MACHINE_DRIVER_END

//  MDRV_SOUND_ADD(DISCRETE, gpsm_discInt) // uses an MC6840, to be implemented yet!





MACHINE_DRIVER_START(gpMSU3)
  MDRV_CPU_ADD_TAG("scpu", M6802, 3579500/4) // NTSC quartz ???
  MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
  MDRV_CPU_MEMORY(gps3_readmem, gps3_writemem)
  MDRV_SOUND_ADD(DAC, msu3_dacInt)
  MDRV_SOUND_ADD(SAMPLES, samples_interface)
  MDRV_TIMER_ADD(pia_cb1_w, 413.793 * 2)
MACHINE_DRIVER_END
