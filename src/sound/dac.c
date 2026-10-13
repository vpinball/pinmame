#include <math.h>
#include "driver.h"

#define DAC_SAMPLE_RATE (4*48000)  // also change dc_coeff when adapting!
//#define DAC_ENABLE_INTERPOLATION // if used, machines like Centaur sound way too muffled and crackle, most likely due to the really low sample rate these machines have and this simple upsample mechanism

static int n_chips;
static int channel[MAX_DAC];
static int output[MAX_DAC];

#ifdef DAC_ENABLE_INTERPOLATION
 static int curr_output[MAX_DAC];
#endif

/* DC offset correction, a one-pole high-pass y[n] = R*y[n-1] + (x[n] - x[n-1]) to convert unipolar signals (=0..MAX range instead of centered around 0).
   Opt-in per chip via DACinterface.dc_offset_correction: only write through DAC_DC_offset_correction_data_16_w(), and get
   a float stream; the filtered output spans up to twice the input range (+-2.0 for 0..65535, e.g. Gottlieb's and Tecnoplay's
   volume*data), which an int16 stream would need to clip.

   All other chips keep the plain int16 path.

   Currently: Gottlieb System 80B/3 and Tecnoplay's (Scramble), Taito's sintetizador, Mr. Game's stereo pair, and the Williams System 3-7, 9/11
   and WPC89 sound boards (Bingo is left alone deliberately - there the main CPU pokes the DAC port directly and it is not clear the value is a waveform rather than a level).

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

/* for chips with the DC offset correction enabled: output[] holds the raw unsigned level, the filter both removes the offset and centers it around 0 */
static void DAC_update_dc(int num,INT16 *const buffer,int length)
{
	float * const out = (float *)buffer;
	int i;
	for (i = 0; i < length; i++)
	{
		const int input = output[num];
		const double y = integrator[num]*dc_coeff + (double)(input - prev_data[num]);
		integrator[num] = y;
		prev_data[num] = input;
		out[i] = (float)(y * (1. / 32768.));
	}
}

static void DAC_update(int num,INT16 *const buffer,int length)
{
	/* zero-length? bail */
	if (length == 0)
		return;
	else
	{
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

/* Takes 0..65535; DAC_update_dc turns that into a signal centered around 0 (the offset removed, i.e. average = 0).
   Only for chips declared in DACinterface.dc_offset_correction */
void DAC_DC_offset_correction_data_16_w(int num, int data)
{
	if (!dc_enabled[num])
	{
		/* not declared in the interface (int16 stream): play it uncorrected rather than overflowing it */
		static int logged = 0;
		if (!logged++) logerror("DAC #%d: DAC_DC_offset_correction_data_16_w() without dc_offset_correction in its DACinterface\n", num);
		DAC_data_16_w(num, data);
		return;
	}

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
		dc_enabled[i] = intf->dc_offset_correction[i] != 0;
		channel[i] = dc_enabled[i] ? stream_init_float(name,intf->mixing_level[i],DAC_SAMPLE_RATE,i,DAC_update_dc,1)
		                           : stream_init(name,intf->mixing_level[i],DAC_SAMPLE_RATE,i,DAC_update);

		if (channel[i] == -1)
			return 1;

		output[i] = 0;
#ifdef DAC_ENABLE_INTERPOLATION
		curr_output[i] = 0;
#endif
		integrator[i] = 0.;
		prev_data[i] = 0;
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
