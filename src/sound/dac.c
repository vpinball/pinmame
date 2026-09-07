#include <math.h>
#include "driver.h"

#define DAC_SAMPLE_RATE (4*48000)  // also change dc_coeff when adapting!
//#define DAC_ENABLE_INTERPOLATION // if used, machines like Centaur sound way too muffled and crackle, most likely due to the really low sample rate these machines have and this simple upsample mechanism

#if (!defined _MSC_VER)
static double min(double x, double y) { return x < y ? x : y; }
static double max(double x, double y) { return x > y ? x : y; }
#endif

static int n_chips;
static int channel[MAX_DAC];
static int output[MAX_DAC];

#ifdef DAC_ENABLE_INTERPOLATION
 static int curr_output[MAX_DAC];
#endif

/* DC offset correction, a one-pole high-pass y[n] = R*y[n-1] + (x[n] - x[n-1]) to convert unipolar signals (=0..MAX range instead of centered around 0).
   Opt-in per channel (if written once through DAC_DC_offset_correction_data_16_w()).
   Currently: Gottlieb System 80B/Techno's, Taito's sintetizador, and Mr. Game's stereo pair (Bingo is left alone deliberately - there the main CPU pokes the DAC port directly and it is not clear the value is a waveform rather than a level).
   NOTE: A channel stays enabled once set; do not mix the plain DAC_*_w entry points with this one on the same channel!

   Filtering per output sample pins it to DAC_SAMPLE_RATE, so the cutoff below is what you actually get.
   10Hz costs -0.04dB at 100Hz and -0.46dB at 30Hz, and settles a DC step in about 16ms */
#define DAC_DC_CUTOFF_HZ 10.0 // also change dc_coeff when adapting!

static int dc_enabled[MAX_DAC];
//static double dc_coeff; /* R for DAC_DC_CUTOFF_HZ at DAC_SAMPLE_RATE */
#define dc_coeff 0.9996728042996023421606962823456233716955572049383392927675559351 // see DAC_sh_start()

static int prev_data[MAX_DAC];
static double integrator[MAX_DAC];

static int UnsignedVolTable[256];
static int SignedVolTable[256];

static void DAC_update(int num,INT16 *const buffer,int length)
{
	/* zero-length? bail */
	if (length == 0)
		return;
	else
	{
		if (dc_enabled[num])
		{
			/* output[] holds the raw unsigned level for these channels; the filter both removes the offset and lands it in signed range */
			int i;
			for (i = 0; i < length; i++)
			{
				const int input = output[num];
				const double out = integrator[num]*dc_coeff + (double)(input - prev_data[num]);
				integrator[num] = out;
				prev_data[num] = input;
				buffer[i] = (INT16)min(max(out, -32768.), 32767.);
			}
			return;
		}
		{
#ifdef DAC_ENABLE_INTERPOLATION
		INT32 data = curr_output[num];
		const INT32 slope = ((output[num] - data) << 15) / length;
		int i;
		data <<= 15;

		for (i = 0; i < length; i++, data += slope)
			buffer[i] = data >> 15;

		curr_output[num] = output[num];
#else
		int i;
		for (i = 0; i < length; i++)
			buffer[i] = output[num];
#endif
		}
	}
}


void DAC_data_w(int num,int data)
{
	int out = UnsignedVolTable[data];

	//if (output[num] != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num],0);
		output[num] = out;
	}
}


void DAC_signed_data_w(int num,int data)
{
	int out = SignedVolTable[data];

	//if (output[num] != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num],0);
		output[num] = out;
	}
}


void DAC_data_16_w(int num,int data)
{
	int out = data >> 1;		/* range      0..32767 */

	//if (output[num] != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num],0);
		output[num] = out;
	}
}

/* Takes 0..65535; DAC_update turns that into -32768..32767 with the offset removed (i.e. average = 0) */
void DAC_DC_offset_correction_data_16_w(int num, int data)
{
	dc_enabled[num] = 1;

	//if (output[num] != data)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num], 0);
		output[num] = data;
	}
}

void DAC_signed_data_16_w(int num,int data)
{
	int out = data - 0x8000;	/* range -32768..32767 */

	//if (output[num] != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num],0);
		output[num] = out;
	}
}


static void DAC_build_voltable(void)
{
	int i;

	/* build volume table (linear) */
	for (i = 0;i < 256;i++)
	{
		UnsignedVolTable[i] = i * 0x101 / 2;	/* range      0..32767 */
		SignedVolTable[i] = i * 0x101 - 0x8000;	/* range -32768..32767 */
	}
}


int DAC_sh_start(const struct MachineSound *msound)
{
	int i;
	const struct DACinterface *intf = msound->sound_interface;

	DAC_build_voltable();

	/* exact one-pole corner: R = (1-sin w)/cos w, w = 2*pi*fc/fs. The usual
	   1 - 2*pi*fc/fs approximation agrees to five decimals at these rates */
	//{
		//const double w = (2.*M_PI*DAC_DC_CUTOFF_HZ)/(double)DAC_SAMPLE_RATE;
		//dc_coeff = (1. - sin(w))/cos(w);
	//}

	n_chips = intf->num;
	for (i = 0; i < n_chips; i++)
	{
		char name[40];

		sprintf(name,"DAC #%d",i);
		channel[i] = stream_init(name,intf->mixing_level[i],DAC_SAMPLE_RATE,i,DAC_update);

		if (channel[i] == -1)
			return 1;

		output[i] = 0;
#ifdef DAC_ENABLE_INTERPOLATION
		curr_output[i] = 0;
#endif
		integrator[i] = 0.;
		prev_data[i] = 0;
		dc_enabled[i] = 0;
	}

	return 0;
}

#ifdef PINMAME
void DAC_set_reverb_filter(int num, float delay, float force)
{
	//stream_update(channel[num], 0); //!!?
	mixer_set_reverb_filter(channel[num], delay, force);
}

void DAC_set_mixing_level(int num, int pctvol)
{
	mixer_set_mixing_level(channel[num], pctvol);
}
#endif


WRITE_HANDLER( DAC_0_data_w )
{
	DAC_data_w(0,data);
}

WRITE_HANDLER( DAC_1_data_w )
{
	DAC_data_w(1,data);
}

WRITE_HANDLER( DAC_0_signed_data_w )
{
	DAC_signed_data_w(0,data);
}

WRITE_HANDLER( DAC_1_signed_data_w )
{
	DAC_signed_data_w(1,data);
}
