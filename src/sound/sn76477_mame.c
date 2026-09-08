// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari
// thanks-to:Derrick Renaud
/*****************************************************************************

    Texas Instruments SN76477 emulator

    authors: Derrick Renaud - info
             Zsolt Vasvari  - software

    Ported to PinMAME from MAME's src/devices/sound/sn76477.cpp, as literally as
    the two interfaces allow, so that a later diff against MAME still shows real
    differences rather than transcription noise. The measured constants, the
    compute_*() formulas and their comments, the noise LFSR and the whole of the
    per-sample loop are MAME's text unchanged; what differs is only the plumbing:

      - sn76477_device       -> struct SN76477, one per chip, passed as 'sn'
      - m_foo                -> sn->m_foo (member names kept for diffability)
      - m_channel->update()  -> stream_update(sn->channel, 0)
      - sound_stream_update  -> SN76477_update_one(), writing INT16 rather than
                                MAME's normalised float: the -1..1 expression is
                                MAME's, the *32767 is ours
      - min()/max()          -> sn_min()/sn_max(), to stay clear of the macros
                                PinMAME's headers bring in
      - m_our_sample_rate    -> the machine sample rate. MAME prefers its device
                                clock() when one is configured; PinMAME's
                                SN76477interface carries no clock, so there is
                                only the one choice, and the stream runs at it -
                                which also avoids the rate mismatch MAME has when
                                the two differ.
      - dropped: the LOG_WAV wav-file capture (needs MAME's util::wav) and the
                 log_*() tracing (needs MAME's logmacro.h). The CHECK_* asserts
                 are dropped too; PinMAME drivers pass RES_INF-free values.
      - not exposed: the *_cap_voltage_w() external-voltage inputs. MAME lets a
                 driver drive the one-shot/SLF/VCO/noise/AD caps directly; no
                 PinMAME driver does, so the m_*_ext flags exist and are honoured
                 in the loop but stay 0.

    Notes (MAME's):
        * All formulas were derived by taking measurements of a real device,
          then running the data sets through the numerical analysis
          application at http://zunzun.com to come up with the functions.

    Known issues/to-do's:
        * Use RES_INF for unconnected resistor pins and treat 0 as a short
          circuit

        * VCO
            * confirm value of VCO_MAX_EXT_VOLTAGE, VCO_TO_SLF_VOLTAGE_DIFF
              VCO_CAP_VOLTAGE_MIN and VCO_CAP_VOLTAGE_MAX
            * confirm value of VCO_MIN_DUTY_CYCLE
            * get real formulas for VCO cap charging and discharging
            * get real formula for VCO duty cycle
            * what happens if no vco_res
            * what happens if no vco_cap (needed for laserbat/lazarian)

        * Attack/Decay
            * get real formulas for a/d cap charging and discharging

        * Output
            * what happens if output is taken at pin 12 with no feedback_res
              (needed for laserbat/lazarian)

 *****************************************************************************/

#include <math.h>
#include "driver.h"
#include "sn76477.h"

/* Both unused here, kept because they are MAME's vocabulary for the two cases a
   driver may one day need to express: an unconnected resistor, and a cap whose
   voltage this chip is not driving. MAME's log_*() tracing and its VERBOSE knob are not ported. */
#define EXTERNAL_VOLTAGE_DISCONNECT (-1.0)  /* indicates that we don't use our internal voltage */
#define RES_INF                     (-1.0)  /* indicates that a resistor is out of circuit */

struct SN76477
{
	int channel;                        /* returned by stream_init() */

	/* chip's external state, as written by the driver */
	UINT32 m_enable;
	UINT32 m_envelope_mode;
	UINT32 m_vco_mode;
	UINT32 m_mixer_mode;
	double m_one_shot_res;
	double m_one_shot_cap;
	UINT32 m_one_shot_cap_voltage_ext;
	double m_slf_res;
	double m_slf_cap;
	UINT32 m_slf_cap_voltage_ext;
	double m_vco_voltage;
	double m_vco_res;
	double m_vco_cap;
	UINT32 m_vco_cap_voltage_ext;
	double m_noise_clock_res;
	UINT32 m_noise_clock_ext;
	UINT32 m_noise_clock;
	double m_noise_filter_res;
	double m_noise_filter_cap;
	UINT32 m_noise_filter_cap_voltage_ext;
	double m_attack_res;
	double m_decay_res;
	double m_attack_decay_cap;
	UINT32 m_attack_decay_cap_voltage_ext;
	double m_amplitude_res;
	double m_feedback_res;
	double m_pitch_voltage;

	/* internal state */
	double m_one_shot_cap_voltage;      /* voltage on the one-shot cap */
	UINT32 m_one_shot_running_ff;       /* 1 = one-shot running, 0 = stopped */
	double m_slf_cap_voltage;           /* voltage on the SLF cap */
	UINT32 m_slf_out_ff;                /* output of the SLF */
	double m_vco_cap_voltage;           /* voltage on the VCO cap */
	UINT32 m_vco_out_ff;                /* output of the VCO */
	UINT32 m_vco_alt_pos_edge_ff;       /* keeps track of the # of positive edges for VCO Alt envelope */
	double m_noise_filter_cap_voltage;  /* voltage on the noise filter cap */
	UINT32 m_real_noise_bit_ff;         /* the current noise bit before filtering */
	UINT32 m_filtered_noise_bit_ff;     /* the noise bit after filtering */
	UINT32 m_noise_gen_count;           /* noise freq emulation */
	double m_attack_decay_cap_voltage;  /* voltage on the attack/decay cap */
	UINT32 m_rng;                       /* current value of the random number generator */
	UINT32 m_mixer_a;
	UINT32 m_mixer_b;
	UINT32 m_mixer_c;
	UINT32 m_envelope_1;
	UINT32 m_envelope_2;
	int    m_our_sample_rate;
};

static struct SN76477 sn76477[MAX_SN76477];
static const struct SN76477interface *sn76477_intf;
static int sn76477_num;
/*****************************************************************************
 *
 *  Constants
 *
 *****************************************************************************/

#define ONE_SHOT_CAP_VOLTAGE_MIN    (0)         /* the voltage at which the one-shot starts from (measured) */
#define ONE_SHOT_CAP_VOLTAGE_MAX    (2.5)       /* the voltage at which the one-shot finishes (measured) */
#define ONE_SHOT_CAP_VOLTAGE_RANGE  (ONE_SHOT_CAP_VOLTAGE_MAX - ONE_SHOT_CAP_VOLTAGE_MIN)

#define SLF_CAP_VOLTAGE_MIN         (0.33)      /* the voltage at the bottom peak of the SLF triangle wave (measured) */
#define SLF_CAP_VOLTAGE_MAX         (2.37)      /* the voltage at the top peak of the SLF triangle wave (measured) */
#define SLF_CAP_VOLTAGE_RANGE       (SLF_CAP_VOLTAGE_MAX - SLF_CAP_VOLTAGE_MIN)

#define VCO_MAX_EXT_VOLTAGE         (2.35)      /* the external voltage at which the VCO saturates and produces no output,
                                                   also used as the voltage threshold for the SLF */
#define VCO_TO_SLF_VOLTAGE_DIFF     (0.35)
#define VCO_CAP_VOLTAGE_MIN         (SLF_CAP_VOLTAGE_MIN)   /* the voltage at the bottom peak of the VCO triangle wave */
#define VCO_CAP_VOLTAGE_MAX         (SLF_CAP_VOLTAGE_MAX + VCO_TO_SLF_VOLTAGE_DIFF) /* the voltage at the bottom peak of the VCO triangle wave */
#define VCO_CAP_VOLTAGE_RANGE       (VCO_CAP_VOLTAGE_MAX - VCO_CAP_VOLTAGE_MIN)
#define VCO_DUTY_CYCLE_50           (5.0)       /* the high voltage that produces a 50% duty cycle */
#define VCO_MIN_DUTY_CYCLE          (18)        /* the smallest possible duty cycle, in % */

#define NOISE_MIN_CLOCK_RES         RES_K(10)   /* the maximum resistor value that still produces a noise (measured) */
#define NOISE_MAX_CLOCK_RES         RES_M(3.3)  /* the minimum resistor value that still produces a noise (measured) */
#define NOISE_CAP_VOLTAGE_MIN       (0)         /* the minimum voltage that the noise filter cap can hold (measured) */
#define NOISE_CAP_VOLTAGE_MAX       (5.0)       /* the maximum voltage that the noise filter cap can hold (measured) */
#define NOISE_CAP_VOLTAGE_RANGE     (NOISE_CAP_VOLTAGE_MAX - NOISE_CAP_VOLTAGE_MIN)
#define NOISE_CAP_HIGH_THRESHOLD    (3.35)      /* the voltage at which the filtered noise bit goes to 0 (measured) */
#define NOISE_CAP_LOW_THRESHOLD     (0.74)      /* the voltage at which the filtered noise bit goes to 1 (measured) */

#define AD_CAP_VOLTAGE_MIN          (0)         /* the minimum voltage the attack/decay cap can hold (measured) */
#define AD_CAP_VOLTAGE_MAX          (4.44)      /* the minimum voltage the attack/decay cap can hold (measured) */
#define AD_CAP_VOLTAGE_RANGE        (AD_CAP_VOLTAGE_MAX - AD_CAP_VOLTAGE_MIN)

#define OUT_CENTER_LEVEL_VOLTAGE    (2.57)      /* the voltage that gets outputted when the volumne is 0 (measured) */
#define OUT_HIGH_CLIP_THRESHOLD     (3.51)      /* the maximum voltage that can be put out (measured) */
#define OUT_LOW_CLIP_THRESHOLD      (0.715)     /* the minimum voltage that can be put out (measured) */

/* gain factors for OUT voltage in 0.1V increments (measured) */
static const double out_pos_gain[] =
{
	0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.01,  /* 0.0 - 0.9V */
	0.03, 0.11, 0.15, 0.19, 0.21, 0.23, 0.26, 0.29, 0.31, 0.33,  /* 1.0 - 1.9V */
	0.36, 0.38, 0.41, 0.43, 0.46, 0.49, 0.52, 0.54, 0.57, 0.60,  /* 2.0 - 2.9V */
	0.62, 0.65, 0.68, 0.70, 0.73, 0.76, 0.80, 0.82, 0.84, 0.87,  /* 3.0 - 3.9V */
	0.90, 0.93, 0.96, 0.98, 1.00                                 /* 4.0 - 4.4V */
};

static const double out_neg_gain[] =
{
	 0.00,  0.00,  0.00,  0.00,  0.00,  0.00,  0.00,  0.00,  0.00, -0.01,  /* 0.0 - 0.9V */
	-0.02, -0.09, -0.13, -0.15, -0.17, -0.19, -0.22, -0.24, -0.26, -0.28,  /* 1.0 - 1.9V */
	-0.30, -0.32, -0.34, -0.37, -0.39, -0.41, -0.44, -0.46, -0.48, -0.51,  /* 2.0 - 2.9V */
	-0.53, -0.56, -0.58, -0.60, -0.62, -0.65, -0.67, -0.69, -0.72, -0.74,  /* 3.0 - 3.9V */
	-0.76, -0.78, -0.81, -0.84, -0.85                                      /* 4.0 - 4.4V */
};

static double sn_max(double a, double b)
{
	return (a > b) ? a : b;
}


static double sn_min(double a, double b)
{
	return (a < b) ? a : b;
}



/*****************************************************************************
 *
 *  Functions for computing frequencies, voltages and similar values based
 *  on the hardware itself.  Do NOT put anything emulation specific here,
 *  such as calculations based on sample_rate.
 *
 *****************************************************************************/

static double compute_one_shot_cap_charging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	 Res (kohms)  Cap (uF)   Time (millisec)
	     47         0.33         11.84
	     47         1.0          36.2
	     47         1.5          52.1
	     47         2.0          76.4
	    100         0.33         24.4
	    100         1.0          75.2
	    100         1.5         108.5
	    100         2.0         158.4
	*/

	double ret = 0;

	if ((sn->m_one_shot_res > 0) && (sn->m_one_shot_cap > 0))
	{
		ret = ONE_SHOT_CAP_VOLTAGE_RANGE / (0.8024 * sn->m_one_shot_res * sn->m_one_shot_cap + 0.002079);
	}
	else if (sn->m_one_shot_cap > 0)
	{
		/* if no resistor, there is no current to charge the cap,
		   effectively making the one-shot time effectively infinite */
		ret = +1e-30;
	}
	else if (sn->m_one_shot_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively making the one-shot time 0 */
		ret = +1e+30;
	}

	return ret;
}


static double compute_one_shot_cap_discharging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	Cap (uF)   Time (microsec)
	  0.33           300
	  1.0            850
	  1.5           1300
	  2.0           1900
	*/

	double ret = 0;

	if ((sn->m_one_shot_res > 0) && (sn->m_one_shot_cap > 0))
	{
		ret = ONE_SHOT_CAP_VOLTAGE_RANGE / (854.7 * sn->m_one_shot_cap + 0.00001795);
	}
	else if (sn->m_one_shot_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively making the one-shot time 0 */
		ret = +1e+30;
	}

	return ret;
}


static double compute_slf_cap_charging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	Res (kohms)  Cap (uF)   Time (millisec)
	     47        0.47          14.3
	    120        0.47          35.6
	    200        0.47          59.2
	     47        1.00          28.6
	    120        1.00          71.6
	    200        1.00         119.0
	*/
	double ret = 0;

	if ((sn->m_slf_res > 0) && (sn->m_slf_cap > 0))
	{
		ret = SLF_CAP_VOLTAGE_RANGE / (0.5885 * sn->m_slf_res * sn->m_slf_cap + 0.001300);
	}

	return ret;
}


static double compute_slf_cap_discharging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	Res (kohms)  Cap (uF)   Time (millisec)
	     47        0.47          13.32
	    120        0.47          32.92
	    200        0.47          54.4
	     47        1.00          26.68
	    120        1.00          66.2
	    200        1.00         109.6
	*/
	double ret = 0;

	if ((sn->m_slf_res > 0) && (sn->m_slf_cap > 0))
	{
		ret = SLF_CAP_VOLTAGE_RANGE / (0.5413 * sn->m_slf_res * sn->m_slf_cap + 0.001343);
	}

	return ret;
}


static double compute_vco_cap_charging_discharging_rate(struct SN76477 *sn) /* in V/sec */
{
	double ret = 0;

	if ((sn->m_vco_res > 0) && (sn->m_vco_cap > 0))
	{
		ret = 0.64 * 2 * VCO_CAP_VOLTAGE_RANGE / (sn->m_vco_res * sn->m_vco_cap);
	}

	return ret;
}


static double compute_vco_duty_cycle(struct SN76477 *sn) /* no measure, just a number */
{
	double ret = 0.5;   /* 50% */

	if ((sn->m_vco_voltage > 0) && (sn->m_pitch_voltage != VCO_DUTY_CYCLE_50))
	{
		ret = sn_max(0.5 * (sn->m_pitch_voltage / sn->m_vco_voltage), (VCO_MIN_DUTY_CYCLE / 100.0));

		ret = sn_min(ret, 1);
	}

	return ret;
}


static UINT32 compute_noise_gen_freq(struct SN76477 *sn) /* in Hz */
{
	/* this formula was derived using the data points below

	 Res (ohms)   Freq (Hz)
	    10k         97493
	    12k         83333
	    15k         68493
	    22k         49164
	    27k         41166
	    33k         34449
	    36k         31969
	    47k         25126
	    56k         21322
	    68k         17721.5
	    82k         15089.2
	    100k        12712.0
	    150k         8746.4
	    220k         6122.4
	    270k         5101.5
	    330k         4217.2
	    390k         3614.5
	    470k         3081.7
	    680k         2132.7
	    820k         1801.8
	      1M         1459.9
	    2.2M          705.13
	    3.3M          487.59
	*/

	UINT32 ret = 0;

	if ((sn->m_noise_clock_res >= NOISE_MIN_CLOCK_RES) &&
		(sn->m_noise_clock_res <= NOISE_MAX_CLOCK_RES))
	{
		ret = (UINT32)(339100000 * pow(sn->m_noise_clock_res, -0.8849));
	}

	return ret;
}


static double compute_noise_filter_cap_charging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	 R*C        Time (sec)
	.000068     .0000184
	.0001496    .0000378
	.0002244    .0000548
	.0003196    .000077
	.0015       .000248
	.0033       .000540
	.00495      .000792
	.00705      .001096
	*/

	double ret = 0;

	if ((sn->m_noise_filter_res > 0) && (sn->m_noise_filter_cap > 0))
	{
		ret = NOISE_CAP_VOLTAGE_RANGE / (0.1571 * sn->m_noise_filter_res * sn->m_noise_filter_cap + 0.00001430);
	}
	else if (sn->m_noise_filter_cap > 0)
	{
		/* if no resistor, there is no current to charge the cap,
		   effectively making the filter's output constants */
		ret = +1e-30;
	}
	else if (sn->m_noise_filter_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively disabling the filter */
		ret = +1e+30;
	}

	return ret;
}


static double compute_noise_filter_cap_discharging_rate(struct SN76477 *sn) /* in V/sec */
{
	/* this formula was derived using the data points below

	 R*C        Time (sec)
	.000068     .000016
	.0001496    .0000322
	.0002244    .0000472
	.0003196    .0000654
	.0015       .000219
	.0033       .000468
	.00495      .000676
	.00705      .000948
	*/

	double ret = 0;

	if ((sn->m_noise_filter_res > 0) && (sn->m_noise_filter_cap > 0))
	{
		ret = NOISE_CAP_VOLTAGE_RANGE / (0.1331 * sn->m_noise_filter_res * sn->m_noise_filter_cap + 0.00001734);
	}
	else if (sn->m_noise_filter_cap > 0)
	{
		/* if no resistor, there is no current to charge the cap,
		   effectively making the filter's output constants */
		ret = +1e-30;
	}
	else if (sn->m_noise_filter_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively disabling the filter */
		ret = +1e+30;
	}

	return ret;
}


static double compute_attack_decay_cap_charging_rate(struct SN76477 *sn)  /* in V/sec */
{
	double ret = 0;

	if ((sn->m_attack_res > 0) && (sn->m_attack_decay_cap > 0))
	{
		ret = AD_CAP_VOLTAGE_RANGE / (sn->m_attack_res * sn->m_attack_decay_cap);
	}
	else if (sn->m_attack_decay_cap > 0)
	{
		/* if no resistor, there is no current to charge the cap,
		   effectively making the attack time infinite */
		ret = +1e-30;
	}
	else if (sn->m_attack_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively making the attack time 0 */
		ret = +1e+30;
	}

	return ret;
}


static double compute_attack_decay_cap_discharging_rate(struct SN76477 *sn)  /* in V/sec */
{
	double ret = 0;

	if ((sn->m_decay_res > 0) && (sn->m_attack_decay_cap > 0))
	{
		ret = AD_CAP_VOLTAGE_RANGE / (sn->m_decay_res * sn->m_attack_decay_cap);
	}
	else if (sn->m_attack_decay_cap > 0)
	{
		/* if no resistor, there is no current to charge the cap,
		   effectively making the decay time infinite */
		ret = +1e-30;
	}
	else if (sn->m_attack_res > 0)
	{
		/* if no cap, the voltage changes extremely fast,
		   effectively making the decay time 0 */
		ret = +1e+30;
	}

	return ret;
}


static double compute_center_to_peak_voltage_out(struct SN76477 *sn)
{
	/* this formula was derived using the data points below

	 Ra (kohms)  Rf (kohms)   Voltage
	    150         47          1.28
	    200         47          0.96
	     47         22          1.8
	    100         22          0.87
	    150         22          0.6
	    200         22          0.45
	     47         10          0.81
	    100         10          0.4
	    150         10          0.27
	*/

	double ret = 0;

	if (sn->m_amplitude_res > 0)
	{
		ret = 3.818 * (sn->m_feedback_res / sn->m_amplitude_res) + 0.03;
	}

	return ret;
}



/*****************************************************************************
 *
 *  Logging functions
 *
 *****************************************************************************/

static void intialize_noise(struct SN76477 *sn)
{
	sn->m_rng = 0;
}

static UINT32 generate_next_real_noise_bit(struct SN76477 *sn)
{
	UINT32 out = ((sn->m_rng >> 28) & 1) ^ ((sn->m_rng >> 0) & 1);

		/* if bits 0-4 and 28 are all zero then force the output to 1 */
	if ((sn->m_rng & 0x1000001f) == 0)
	{
		out = 1;
	}

	sn->m_rng = (sn->m_rng >> 1) | (out << 30);

	return out;
}

static void SN76477_update_one(struct SN76477 *sn, INT16 *buffer, int length)
{
	double one_shot_cap_charging_step;
	double one_shot_cap_discharging_step;
	double slf_cap_charging_step;
	double slf_cap_discharging_step;
	double vco_duty_cycle_multiplier;
	double vco_cap_charging_step;
	double vco_cap_discharging_step;
	double vco_cap_voltage_max;
	UINT32 noise_gen_freq;
	double noise_filter_cap_charging_step;
	double noise_filter_cap_discharging_step;
	double attack_decay_cap_charging_step;
	double attack_decay_cap_discharging_step;
	int    attack_decay_cap_charging;
	double voltage_out;
	double center_to_peak_voltage_out;

	/* compute charging values, doing it here ensures that we always use the latest values */
	one_shot_cap_charging_step = compute_one_shot_cap_charging_rate(sn) / sn->m_our_sample_rate;
	one_shot_cap_discharging_step = compute_one_shot_cap_discharging_rate(sn) / sn->m_our_sample_rate;

	slf_cap_charging_step = compute_slf_cap_charging_rate(sn) / sn->m_our_sample_rate;
	slf_cap_discharging_step = compute_slf_cap_discharging_rate(sn) / sn->m_our_sample_rate;

	vco_duty_cycle_multiplier = (1 - compute_vco_duty_cycle(sn)) * 2;
	vco_cap_charging_step = compute_vco_cap_charging_discharging_rate(sn) / vco_duty_cycle_multiplier / sn->m_our_sample_rate;
	vco_cap_discharging_step = compute_vco_cap_charging_discharging_rate(sn) * vco_duty_cycle_multiplier / sn->m_our_sample_rate;

	noise_filter_cap_charging_step = compute_noise_filter_cap_charging_rate(sn) / sn->m_our_sample_rate;
	noise_filter_cap_discharging_step = compute_noise_filter_cap_discharging_rate(sn) / sn->m_our_sample_rate;
	noise_gen_freq = compute_noise_gen_freq(sn);

	attack_decay_cap_charging_step = compute_attack_decay_cap_charging_rate(sn) / sn->m_our_sample_rate;
	attack_decay_cap_discharging_step = compute_attack_decay_cap_discharging_rate(sn) / sn->m_our_sample_rate;

	center_to_peak_voltage_out = compute_center_to_peak_voltage_out(sn);


	/* process 'samples' number of samples */
	int sampindex;
	for (sampindex = 0; sampindex < length; sampindex++)
	{
		/* update the one-shot cap voltage */
		if (!sn->m_one_shot_cap_voltage_ext)
		{
			if (sn->m_one_shot_running_ff)
			{
				/* charging */
				sn->m_one_shot_cap_voltage = sn_min(sn->m_one_shot_cap_voltage + one_shot_cap_charging_step, ONE_SHOT_CAP_VOLTAGE_MAX);
			}
			else
			{
				/* discharging */
				sn->m_one_shot_cap_voltage = sn_max(sn->m_one_shot_cap_voltage - one_shot_cap_discharging_step, ONE_SHOT_CAP_VOLTAGE_MIN);
			}
		}

		if (sn->m_one_shot_cap_voltage >= ONE_SHOT_CAP_VOLTAGE_MAX)
		{
			sn->m_one_shot_running_ff = 0;
		}


		/* update the SLF (super low frequency oscillator) */
		if (!sn->m_slf_cap_voltage_ext)
		{
			/* internal */
			if (!sn->m_slf_out_ff)
			{
				/* charging */
				sn->m_slf_cap_voltage = sn_min(sn->m_slf_cap_voltage + slf_cap_charging_step, SLF_CAP_VOLTAGE_MAX);
			}
			else
			{
				/* discharging */
				sn->m_slf_cap_voltage = sn_max(sn->m_slf_cap_voltage - slf_cap_discharging_step, SLF_CAP_VOLTAGE_MIN);
			}
		}

		if (sn->m_slf_cap_voltage >= SLF_CAP_VOLTAGE_MAX)
		{
			sn->m_slf_out_ff = 1;
		}
		else if (sn->m_slf_cap_voltage <= SLF_CAP_VOLTAGE_MIN)
		{
			sn->m_slf_out_ff = 0;
		}


		/* update the VCO (voltage controlled oscillator) */
		if (sn->m_vco_mode)
		{
			/* VCO is controlled by SLF */
			vco_cap_voltage_max =  sn->m_slf_cap_voltage + VCO_TO_SLF_VOLTAGE_DIFF;
		}
		else
		{
			/* VCO is controlled by external voltage */
			vco_cap_voltage_max = sn->m_vco_voltage + VCO_TO_SLF_VOLTAGE_DIFF;
		}

		if (!sn->m_vco_cap_voltage_ext)
		{
			if (!sn->m_vco_out_ff)
			{
				/* charging */
				sn->m_vco_cap_voltage = sn_min(sn->m_vco_cap_voltage + vco_cap_charging_step, vco_cap_voltage_max);
			}
			else
			{
				/* discharging */
				sn->m_vco_cap_voltage = sn_max(sn->m_vco_cap_voltage - vco_cap_discharging_step, VCO_CAP_VOLTAGE_MIN);
			}
		}

		if (sn->m_vco_cap_voltage >= vco_cap_voltage_max)
		{
			if (!sn->m_vco_out_ff)
			{
				/* positive edge */
				sn->m_vco_alt_pos_edge_ff = !sn->m_vco_alt_pos_edge_ff;
			}

			sn->m_vco_out_ff = 1;
		}
		else if (sn->m_vco_cap_voltage <= VCO_CAP_VOLTAGE_MIN)
		{
			sn->m_vco_out_ff = 0;
		}


		/* update the noise generator */
		while (!sn->m_noise_clock_ext && (sn->m_noise_gen_count <= noise_gen_freq))
		{
			sn->m_noise_gen_count = sn->m_noise_gen_count + sn->m_our_sample_rate;

			sn->m_real_noise_bit_ff = generate_next_real_noise_bit(sn);
		}

		sn->m_noise_gen_count = sn->m_noise_gen_count - noise_gen_freq;


		/* update the noise filter */
		if (!sn->m_noise_filter_cap_voltage_ext)
		{
			/* internal */
			if (sn->m_real_noise_bit_ff)
			{
				/* charging */
				sn->m_noise_filter_cap_voltage = sn_min(sn->m_noise_filter_cap_voltage + noise_filter_cap_charging_step, NOISE_CAP_VOLTAGE_MAX);
			}
			else
			{
				/* discharging */
				sn->m_noise_filter_cap_voltage = sn_max(sn->m_noise_filter_cap_voltage - noise_filter_cap_discharging_step, NOISE_CAP_VOLTAGE_MIN);
			}
		}

		/* check the thresholds */
		if (sn->m_noise_filter_cap_voltage >= NOISE_CAP_HIGH_THRESHOLD)
		{
			sn->m_filtered_noise_bit_ff = 0;
		}
		else if (sn->m_noise_filter_cap_voltage <= NOISE_CAP_LOW_THRESHOLD)
		{
			sn->m_filtered_noise_bit_ff = 1;
		}


		/* based on the envelope mode figure out the attack/decay phase we are in */
		switch (sn->m_envelope_mode)
		{
		case 0:     /* VCO */
			attack_decay_cap_charging = sn->m_vco_out_ff;
			break;

		case 1:     /* one-shot */
			attack_decay_cap_charging = sn->m_one_shot_running_ff;
			break;

		case 2:
		default:    /* mixer only */
			attack_decay_cap_charging = 1;  /* never a decay phase */
			break;

		case 3:     /* VCO with alternating polarity */
			attack_decay_cap_charging = sn->m_vco_out_ff && sn->m_vco_alt_pos_edge_ff;
			break;
		}


		/* update a/d cap voltage */
		if (!sn->m_attack_decay_cap_voltage_ext)
		{
			if (attack_decay_cap_charging)
			{
				if (attack_decay_cap_charging_step > 0)
				{
					sn->m_attack_decay_cap_voltage = sn_min(sn->m_attack_decay_cap_voltage + attack_decay_cap_charging_step, AD_CAP_VOLTAGE_MAX);
				}
				else
				{
					/* no attack, voltage to max instantly */
					sn->m_attack_decay_cap_voltage = AD_CAP_VOLTAGE_MAX;
				}
			}
			else
			{
				/* discharging */
				if (attack_decay_cap_discharging_step > 0)
				{
					sn->m_attack_decay_cap_voltage = sn_max(sn->m_attack_decay_cap_voltage - attack_decay_cap_discharging_step, AD_CAP_VOLTAGE_MIN);
				}
				else
				{
					/* no decay, voltage to min instantly */
					sn->m_attack_decay_cap_voltage = AD_CAP_VOLTAGE_MIN;
				}
			}
		}


		/* mix the output, if enabled, or not saturated by the VCO */
		if (!sn->m_enable && (sn->m_vco_cap_voltage <= VCO_CAP_VOLTAGE_MAX))
		{
			UINT32 out;

			/* enabled */
			switch (sn->m_mixer_mode)
			{
			case 0:     /* VCO */
				out = sn->m_vco_out_ff;
				break;

			case 1:     /* SLF */
				out = sn->m_slf_out_ff;
				break;

			case 2:     /* noise */
				out = sn->m_filtered_noise_bit_ff;
				break;

			case 3:     /* VCO and noise */
				out = sn->m_vco_out_ff & sn->m_filtered_noise_bit_ff;
				break;

			case 4:     /* SLF and noise */
				out = sn->m_slf_out_ff & sn->m_filtered_noise_bit_ff;
				break;

			case 5:     /* VCO, SLF and noise */
				out = sn->m_vco_out_ff & sn->m_slf_out_ff & sn->m_filtered_noise_bit_ff;
				break;

			case 6:     /* VCO and SLF */
				out = sn->m_vco_out_ff & sn->m_slf_out_ff;
				break;

			case 7:     /* inhibit */
			default:
				out = 0;
				break;
			}

			/* determine the OUT voltage from the attack/delay cap voltage and clip it */
			if (out)
			{
				voltage_out = OUT_CENTER_LEVEL_VOLTAGE + center_to_peak_voltage_out * out_pos_gain[(int)(sn->m_attack_decay_cap_voltage * 10)],
				voltage_out = sn_min(voltage_out, OUT_HIGH_CLIP_THRESHOLD);
			}
			else
			{
				voltage_out = OUT_CENTER_LEVEL_VOLTAGE + center_to_peak_voltage_out * out_neg_gain[(int)(sn->m_attack_decay_cap_voltage * 10)],
				voltage_out = sn_max(voltage_out, OUT_LOW_CLIP_THRESHOLD);
			}
		}
		else
		{
			/* disabled */
			voltage_out = OUT_CENTER_LEVEL_VOLTAGE;
		}


		/* convert it to a signed 16-bit sample,
		   -32767 = OUT_LOW_CLIP_THRESHOLD
		        0 = OUT_CENTER_LEVEL_VOLTAGE
		    32767 = 2 * OUT_CENTER_LEVEL_VOLTAGE + OUT_LOW_CLIP_THRESHOLD

		              / Vout - Vmin    \
		    sample = |  ----------- - 1 |
		              \ Vcen - Vmin    /
		 */
		/* MAME writes the normalised -1..1 value straight into its float stream;
		   PinMAME wants INT16, so scale by 32767 here and nowhere else. */
		buffer[sampindex] = (INT16)(32767.0 * (((voltage_out - OUT_LOW_CLIP_THRESHOLD) / (OUT_CENTER_LEVEL_VOLTAGE - OUT_LOW_CLIP_THRESHOLD)) - 1));

		/* MAME logs to a wav here (LOG_WAV/add_wav_data); dropped, it needs util::wav */
	}
}
/*****************************************************************************
 *
 *  PinMAME interface. Each entry point maps onto the MAME device method of the
 *  same job; the pattern is MAME's - update the stream, then change the state -
 *  so that a write never takes effect retroactively over samples already made.
 *
 *****************************************************************************/

#define SN_CHIP(c)  struct SN76477 *sn; if ((c) < 0 || (c) >= sn76477_num) return; sn = &sn76477[c]

/* Enable (one input line: 0 enabled, 1 inhibited) - resets one shot.
   MAME: enable_w(). The old PinMAME core also forced slf_level back to 5V on
   disable (056b1723, 2003, "Fixed SLF reset"). That was local, not from MAME,
   and it is deliberately not carried over: it drove a slf_level variable of the
   old timer model that does not exist here, and 5V is outside this model's SLF
   cap range (0.33 .. 2.37V) anyway, so there is no value to translate it to.
   Whether the games it was aimed at still behave is a listening test - untested. */
void SN76477_enable_w(int chip, UINT32 data)
{
	SN_CHIP(chip);

	if (data != sn->m_enable)
	{
		stream_update(sn->channel, 0);

		sn->m_enable = data;

			/* if falling edge */
		if (!sn->m_enable)
		{
			/* start the attack phase */
			sn->m_attack_decay_cap_voltage = AD_CAP_VOLTAGE_MIN;

			/* one-shot runs regardless of envelope mode */
			sn->m_one_shot_running_ff = 1;
		}
	}
}

/* MAME: mixer_a_w()/mixer_b_w()/mixer_c_w(), one bit of the mode each */
void SN76477_mixer_a_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != ((sn->m_mixer_mode >> 0) & 0x01))
	{
		stream_update(sn->channel, 0);
		sn->m_mixer_mode = (sn->m_mixer_mode & ~0x01) | data;
	}
}

void SN76477_mixer_b_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != ((sn->m_mixer_mode >> 1) & 0x01))
	{
		stream_update(sn->channel, 0);
		sn->m_mixer_mode = (sn->m_mixer_mode & ~0x02) | (data << 1);
	}
}

void SN76477_mixer_c_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != ((sn->m_mixer_mode >> 2) & 0x01))
	{
		stream_update(sn->channel, 0);
		sn->m_mixer_mode = (sn->m_mixer_mode & ~0x04) | (data << 2);
	}
}

/* Mixer select (three input lines, data 0 to 7). No MAME equivalent - it only
   has the three line writes - but the old core took the mode directly here too,
   and m_mixer_mode is the same field, so the drivers need no change. */
void SN76477_mixer_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != sn->m_mixer_mode)
	{
		stream_update(sn->channel, 0);
		sn->m_mixer_mode = data;
	}
}

/* MAME: envelope_1_w()/envelope_2_w(), one bit of the mode each */
void SN76477_envelope_1_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != ((sn->m_envelope_mode >> 0) & 0x01))
	{
		stream_update(sn->channel, 0);
		sn->m_envelope_mode = (sn->m_envelope_mode & ~0x01) | data;
	}
}

void SN76477_envelope_2_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != ((sn->m_envelope_mode >> 1) & 0x01))
	{
		stream_update(sn->channel, 0);
		sn->m_envelope_mode = (sn->m_envelope_mode & ~0x02) | (data << 1);
	}
}

/* Select envelope (two input lines, data 0 to 3); see SN76477_mixer_w above */
void SN76477_envelope_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != sn->m_envelope_mode)
	{
		stream_update(sn->channel, 0);
		sn->m_envelope_mode = data;
	}
}

/* VCO select (one input line: 0 external control, 1: SLF control). MAME: vco_w() */
void SN76477_vco_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != sn->m_vco_mode)
	{
		stream_update(sn->channel, 0);
		sn->m_vco_mode = data;
	}
}

/* Noise clock write, useful only if noise_res is zero. MAME: noise_clock_w() */
void SN76477_noise_clock_w(int chip, UINT32 data)
{
	SN_CHIP(chip);
	if (data != sn->m_noise_clock)
	{
		sn->m_noise_clock = data;

		/* on the rising edge shift generate next value,
		   if external control is enabled */
		if (sn->m_noise_clock && sn->m_noise_clock_ext)
		{
			stream_update(sn->channel, 0);
			sn->m_real_noise_bit_ff = generate_next_real_noise_bit(sn);
		}
	}
}

/* MAME: noise_clock_res_w(). The only setter that is not a plain store - a
   resistor value of 0 selects the external noise clock instead. */
void SN76477_set_noise_res(int chip, double data)
{
	SN_CHIP(chip);

	if (((data == 0) && !sn->m_noise_clock_ext) ||
		((data != 0) && (data != sn->m_noise_clock_res)))
	{
		stream_update(sn->channel, 0);

		if (data == 0)
		{
			sn->m_noise_clock_ext = 1;
		}
		else
		{
			sn->m_noise_clock_ext = 0;
			sn->m_noise_clock_res = data;
		}
	}
}

/* The double setters. MAME names these one_shot_res_w() etc; the SN76477_set_*
   spellings are PinMAME's and are what the drivers already call. */
#define SN_SETTER(pmname, member)                       \
	void pmname(int chip, double data)                  \
	{                                                   \
		SN_CHIP(chip);                                  \
		if (data != sn->member)                         \
		{                                               \
			stream_update(sn->channel, 0);              \
			sn->member = data;                          \
		}                                               \
	}

SN_SETTER(SN76477_set_filter_res,        m_noise_filter_res)  /* MAME: noise_filter_res_w */
SN_SETTER(SN76477_set_filter_cap,        m_noise_filter_cap)  /* MAME: noise_filter_cap_w */
SN_SETTER(SN76477_set_decay_res,         m_decay_res)
SN_SETTER(SN76477_set_attack_decay_cap,  m_attack_decay_cap)
SN_SETTER(SN76477_set_attack_res,        m_attack_res)
SN_SETTER(SN76477_set_amplitude_res,     m_amplitude_res)
SN_SETTER(SN76477_set_feedback_res,      m_feedback_res)
SN_SETTER(SN76477_set_slf_res,           m_slf_res)
SN_SETTER(SN76477_set_slf_cap,           m_slf_cap)
SN_SETTER(SN76477_set_oneshot_res,       m_one_shot_res)      /* MAME: one_shot_res_w */
SN_SETTER(SN76477_set_oneshot_cap,       m_one_shot_cap)      /* MAME: one_shot_cap_w */
SN_SETTER(SN76477_set_vco_res,           m_vco_res)
SN_SETTER(SN76477_set_vco_cap,           m_vco_cap)
SN_SETTER(SN76477_set_pitch_voltage,     m_pitch_voltage)
SN_SETTER(SN76477_set_vco_voltage,       m_vco_voltage)

/*****************************************************************************
 *
 *  Stream, start and stop
 *
 *****************************************************************************/

static void SN76477_update(int chip, INT16 *buffer, int length)
{
	SN76477_update_one(&sn76477[chip], buffer, length);
}

int SN76477_sh_start(const struct MachineSound *msound)
{
	int chip;
	char name[40];

	sn76477_intf = msound->sound_interface;
	sn76477_num  = sn76477_intf->num;
	if (sn76477_num > MAX_SN76477)
		sn76477_num = MAX_SN76477;

	for (chip = 0; chip < sn76477_num; chip++)
	{
		struct SN76477 *sn = &sn76477[chip];

		memset(sn, 0, sizeof(*sn));

		/* the interface values, as MAME's device_start() takes them from the config */
		sn->m_noise_clock_res   = sn76477_intf->noise_res[chip];
		sn->m_noise_filter_res  = sn76477_intf->filter_res[chip];
		sn->m_noise_filter_cap  = sn76477_intf->filter_cap[chip];
		sn->m_decay_res         = sn76477_intf->decay_res[chip];
		sn->m_attack_decay_cap  = sn76477_intf->attack_decay_cap[chip];
		sn->m_attack_res        = sn76477_intf->attack_res[chip];
		sn->m_amplitude_res     = sn76477_intf->amplitude_res[chip];
		sn->m_feedback_res      = sn76477_intf->feedback_res[chip];
		sn->m_vco_voltage       = sn76477_intf->vco_voltage[chip];
		sn->m_vco_cap           = sn76477_intf->vco_cap[chip];
		sn->m_vco_res           = sn76477_intf->vco_res[chip];
		sn->m_pitch_voltage     = sn76477_intf->pitch_voltage[chip];
		sn->m_slf_res           = sn76477_intf->slf_res[chip];
		sn->m_slf_cap           = sn76477_intf->slf_cap[chip];
		sn->m_one_shot_cap      = sn76477_intf->oneshot_cap[chip];
		sn->m_one_shot_res      = sn76477_intf->oneshot_res[chip];

		/* MAME's constructor leaves enable/mixer/envelope/vco all at 0; the memset
		   above already did that. Its device_start() then seeds the cap voltages: */
		sn->m_one_shot_cap_voltage      = ONE_SHOT_CAP_VOLTAGE_MIN;
		sn->m_slf_cap_voltage           = SLF_CAP_VOLTAGE_MIN;
		sn->m_vco_cap_voltage           = VCO_CAP_VOLTAGE_MIN;
		sn->m_noise_filter_cap_voltage  = NOISE_CAP_VOLTAGE_MIN;
		sn->m_attack_decay_cap_voltage  = AD_CAP_VOLTAGE_MIN;
		intialize_noise(sn);

		sn->m_our_sample_rate = (int)(Machine->sample_rate ? Machine->sample_rate : 44100.);

		sprintf(name, "SN76477 #%d", chip);
		sn->channel = stream_init(name, sn76477_intf->mixing_level[chip],
		                          sn->m_our_sample_rate, chip, SN76477_update);
		if (sn->channel == -1)
			return 1;
	}

	return 0;
}

void SN76477_sh_stop(void)
{
}

/* PinMAME-local: no MAME counterpart, and every other sound core here leaves this
   empty. Kept because it was deliberately maintained - it passed the chip index
   where a channel was wanted until ffcc63db (2022) fixed it - and a per-frame flush
   costs nothing, every driver write already calling stream_update itself. */
void SN76477_sh_update(void)
{
	int chip;
	for (chip = 0; chip < sn76477_num; chip++)
		stream_update(sn76477[chip].channel, 0);
}
