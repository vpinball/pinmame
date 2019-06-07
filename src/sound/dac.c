#include <math.h>
#include "driver.h"

#define DAC_SAMPLE_RATE (4*48000)
//#define DAC_ENABLE_INTERPOLATION // if used, machines like Centaur sound way too muffled and crackle, most likely due to the really low sample rate these machines have and this simple upsample mechanism

static int n_chips;
static int channel[MAX_DAC];
static int output[MAX_DAC];

#ifdef DAC_ENABLE_INTERPOLATION
 static int curr_output[MAX_DAC];
#endif

// DC offset correction:
static int prev_data[MAX_DAC];
static double integrator[MAX_DAC];

static int UnsignedVolTable[256];
static int SignedVolTable[256];

static void DAC_update(int num,INT16 *buffer,int length)
{
	/* zero-length? bail */
	if (length == 0)
		return;
	else
	{
		int i;
#ifdef DAC_ENABLE_INTERPOLATION
		INT32 data = curr_output[num];
		INT32 slope = ((output[num] - data) << 15) / length;
		data <<= 15;

		for (i = 0; i < length; i++, data += slope)
			*buffer++ = data >> 15;

		curr_output[num] = output[num];
#else
		for (i = 0; i < length; i++)
			*buffer++ = output[num];
#endif
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

void DAC_DC_offset_correction_data_16_w(int num, int data)
{
	// convert from 0..65535 to -32768..32767, including DC offset correction (e.g. average = 0)
	int out;

	integrator[num] = integrator[num]*0.995 + (data - prev_data[num]); // 0.7 .. 0.995

	out = (int)integrator[num];
	if (out < -32768)
		out = -32768;
	else if (out > 32767)
		out = 32767;

	prev_data[num] = data;

	//if (output[num] != out)
	{
		/* update the output buffer before changing the registers */
		stream_update(channel[num], 0);
		output[num] = out;
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
