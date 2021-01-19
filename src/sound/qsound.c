// license:BSD-3-Clause
// copyright-holders:superctr, Valley Bell

/***************************************************************************

  Capcom QSound DL-1425 (HLE)
  ===========================

  Driver by superctr with thanks to Valley Bell.

  Based on disassembled DSP code.

  Links:
  https://siliconpr0n.org/map/capcom/dl-1425

***************************************************************************/
#if (!defined _MSC_VER)
#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))
#endif

#include "driver.h"
#include "../ext/vgm/vgmwrite.h"

#include "qsound_dl-1425.h"

/*
Debug defines
*/
#define LOG_WAVE	0
#define LOG_QSOUND  0

#define QSOUND_CLOCKDIV (2*1248)  // DSP program uses 1248 machine cycles per iteration

#define uint8_t unsigned char
#define int8_t signed char
#define uint16_t unsigned short
#define int16_t signed short
#define uint32_t unsigned int
#define int32_t signed int

static const uint8_t *qsound_sample_rom; // QSound sample ROM
static unsigned int qsound_sample_rom_length;
static unsigned int qsound_sample_rom_mask;

static uint16_t m_vgm_idx;

//

// DSP ROM sample map
enum {
	DATA_PAN_TAB = 0x110,
	DATA_ADPCM_TAB = 0x9dc,
	DATA_FILTER_TAB = 0xd53,    // dual filter mode, 5 tables * 95 taps each
	DATA_FILTER_TAB2 = 0xf2e,   // overlapping data (95+15+95)

	STATE_BOOT = 0x000,
	STATE_INIT1 = 0x288,
	STATE_INIT2 = 0x61a,
	STATE_REFRESH1 = 0x039,
	STATE_REFRESH2 = 0x04f,
	STATE_NORMAL1 = 0x314,
	STATE_NORMAL2 = 0x6b2
};

static const uint16_t PAN_TABLE_DRY = 0;
static const uint16_t PAN_TABLE_WET = 98;
static const uint16_t PAN_TABLE_CH_OFFSET = 196;
static const uint16_t FILTER_ENTRY_SIZE = 95;
static const uint16_t DELAY_BASE_OFFSET = 0x554;
static const uint16_t DELAY_BASE_OFFSET2 = 0x53c;

typedef struct qsound_voice_t {
	uint16_t m_bank;
	int16_t m_addr; // top word is the sample address
	uint16_t m_phase;
	uint16_t m_rate;
	int16_t m_loop_len;
	int16_t m_end_addr;
	int16_t m_volume;
	int16_t m_echo;
} qsound_voice;

typedef struct qsound_adpcm_t {
	uint16_t m_start_addr;
	uint16_t m_end_addr;
	uint16_t m_bank;
	int16_t m_volume;
	uint16_t m_flag;
	int16_t m_cur_vol;
	int16_t m_step_size;
	uint16_t m_cur_addr;
} qsound_adpcm;

// Q1 Filter
typedef struct qsound_fir_t {
	int m_tap_count;    // usually 95
	int m_delay_pos;
	uint16_t m_table_pos;
	int16_t m_taps[95];
	int16_t m_delay_line[95];
} qsound_fir;

// Delay line
typedef struct qsound_delay_t {
	int16_t m_delay;
	int16_t m_volume;
	int16_t m_write_pos;
	int16_t m_read_pos;
	int16_t m_delay_line[51];
} qsound_delay;

typedef struct qsound_echo_t {
	uint16_t m_end_pos;

	int16_t m_feedback;
	int16_t m_length;
	int16_t m_last_sample;
	int16_t m_delay_line[1024];
	int16_t m_delay_pos;
} qsound_echo;

static int m_stream;

static uint16_t m_data_latch;
static int16_t m_out[2];

static qsound_voice m_voice[16];
static qsound_adpcm m_adpcm[3];

static uint16_t m_voice_pan[16 + 3];
static int16_t m_voice_output[16 + 3];

static qsound_echo m_echo;

static qsound_fir m_filter[2];
static qsound_fir m_alt_filter[2];

static qsound_delay m_wet[2];
static qsound_delay m_dry[2];

static uint16_t m_state;
static uint16_t m_next_state;

static uint16_t m_delay_update;

static int m_state_counter;
static int m_ready_flag;

static uint16_t *m_register_map[256];

INLINE uint16_t read_dsp_rom(uint16_t addr) { return ((uint16_t*)dl_1425)[addr & 0xfff]; }

//

#if LOG_WAVE
static FILE *fpRawDataL;
static FILE *fpRawDataR;
#endif

/* Function prototypes */
void qsound_update( int num, INT16 **buffer, int length );
INLINE void qsound_write_data(uint8_t address, uint16_t data);
static void init_register_map();
static void state_init();
static void state_refresh_filter_1();
static void state_refresh_filter_2();
static void state_normal_update();
INLINE void qsound_delay_update(qsound_delay *qd);
static int32_t qsound_fir_apply(qsound_fir *qf, int16_t input);
INLINE int32_t qsound_delay_apply(qsound_delay *qd, const int32_t input);

INLINE UINT32 pow2_mask(UINT32 v)
{
	if (v == 0)
		return 0;
	v --;
	v |= (v >>  1);
	v |= (v >>  2);
	v |= (v >>  4);
	v |= (v >>  8);
	v |= (v >> 16);
	return v;
}

int qsound_sh_start(const struct MachineSound *msound)
{
	const struct QSound_interface *intf = msound->sound_interface;

	qsound_sample_rom = (uint8_t *)memory_region(intf->region);
	qsound_sample_rom_length = (unsigned int)memory_region_length(intf->region);
	qsound_sample_rom_mask = pow2_mask(qsound_sample_rom_length);

	m_vgm_idx = vgm_open(VGMC_QSOUND, intf->clock);
	vgm_dump_sample_rom(m_vgm_idx, 0x01, intf->region);

	//!! init all stuff incl. structs with 0? should not be necessary as STATE_BOOT will init all

	m_data_latch = 0;

	m_ready_flag = 0;
	m_out[0] = m_out[1] = 0;
	m_state = STATE_BOOT;
	m_state_counter = 0;

	init_register_map();

	{
		/* Allocate stream */
#define CHANNELS 2
		char buf[CHANNELS][40];
		const char *name[CHANNELS];
		int  vol[2];
		name[0] = buf[0];
		name[1] = buf[1];
		sprintf( buf[0], "%s L", sound_name(msound) );
		sprintf( buf[1], "%s R", sound_name(msound) );
		vol[0] = MIXER(intf->mixing_level[0], MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->mixing_level[1], MIXER_PAN_RIGHT);
		m_stream = stream_init_multi(
			CHANNELS,
			name,
			vol,
			intf->clock / QSOUND_CLOCKDIV,
			0,
			qsound_update );
	}

#if LOG_WAVE
	fpRawDataR=fopen("qsoundr.raw", "w+b");
	fpRawDataL=fopen("qsoundl.raw", "w+b");
	if (!fpRawDataR || !fpRawDataL)
	{
		return 1;
	}
#endif

	return 0x00;
}

void qsound_sh_stop (void)
{
#if LOG_WAVE
	if (fpRawDataR)
	{
		fclose(fpRawDataR);
	}
	if (fpRawDataL)
	{
		fclose(fpRawDataL);
	}
#endif
}

void qsound_sh_reset(void)
{
	m_ready_flag = 0;
	m_out[0] = m_out[1] = 0;
	m_state = STATE_BOOT;
	m_state_counter = 0;
}

WRITE_HANDLER( qsound_data_h_w )
{
	m_data_latch = (m_data_latch & 0x00ff) | (data << 8);
}

WRITE_HANDLER( qsound_data_l_w )
{
	m_data_latch = (m_data_latch & 0xff00) | data;
}

WRITE_HANDLER( qsound_cmd_w )
{
	vgm_write(m_vgm_idx, 0x00, m_data_latch, data);

	stream_update(m_stream, 0);
	qsound_write_data(data, m_data_latch);
}

READ_HANDLER( qsound_status_r )
{
	// ready bit (0x00 = busy, 0x80 == ready)
	stream_update(m_stream, 0);
	return m_ready_flag;
}

INLINE void qsound_write_data(uint8_t address, uint16_t data)
{
	uint16_t *destination = m_register_map[address];
	if (destination)
		*destination = data;
	m_ready_flag = 0;
}

static void init_register_map()
{
	int i;
	// unused registers
	for (i = 0; i < 256; ++i)
	    m_register_map[i] = NULL;

	// PCM registers
	for (i = 0; i < 16; i++) // PCM voices
	{
		m_register_map[(i << 3) + 0] = (uint16_t*)&m_voice[(i + 1) % 16].m_bank; // Bank applies to the next channel
		m_register_map[(i << 3) + 1] = (uint16_t*)&m_voice[i].m_addr; // Current sample position and start position.
		m_register_map[(i << 3) + 2] = (uint16_t*)&m_voice[i].m_rate; // 4.12 fixed point decimal.
		m_register_map[(i << 3) + 3] = (uint16_t*)&m_voice[i].m_phase;
		m_register_map[(i << 3) + 4] = (uint16_t*)&m_voice[i].m_loop_len;
		m_register_map[(i << 3) + 5] = (uint16_t*)&m_voice[i].m_end_addr;
		m_register_map[(i << 3) + 6] = (uint16_t*)&m_voice[i].m_volume;
		m_register_map[(i << 3) + 7] = NULL; // unused
		m_register_map[i + 0x80] = (uint16_t*)&m_voice_pan[i];
		m_register_map[i + 0xba] = (uint16_t*)&m_voice[i].m_echo;
	}

	// ADPCM registers
	for (i = 0; i < 3; i++) // ADPCM voices
	{
		// ADPCM sample rate is fixed to 8khz. (one channel is updated every third sample)
		m_register_map[(i << 2) + 0xca] = (uint16_t*)&m_adpcm[i].m_start_addr;
		m_register_map[(i << 2) + 0xcb] = (uint16_t*)&m_adpcm[i].m_end_addr;
		m_register_map[(i << 2) + 0xcc] = (uint16_t*)&m_adpcm[i].m_bank;
		m_register_map[(i << 2) + 0xcd] = (uint16_t*)&m_adpcm[i].m_volume;
		m_register_map[i + 0xd6] = (uint16_t*)&m_adpcm[i].m_flag; // non-zero to start ADPCM playback
		m_register_map[i + 0x90] = (uint16_t*)&m_voice_pan[16 + i];
	}

	// QSound registers
	m_register_map[0x93] = (uint16_t*)&m_echo.m_feedback;
	m_register_map[0xd9] = (uint16_t*)&m_echo.m_end_pos;
	m_register_map[0xe2] = (uint16_t*)&m_delay_update; // non-zero to update delays
	m_register_map[0xe3] = (uint16_t*)&m_next_state;
	for (i = 0; i < 2; i++)  // left, right
	{
		// Wet
		m_register_map[(i << 1) + 0xda] = (uint16_t*)&m_filter[i].m_table_pos;
		m_register_map[(i << 1) + 0xde] = (uint16_t*)&m_wet[i].m_delay;
		m_register_map[(i << 1) + 0xe4] = (uint16_t*)&m_wet[i].m_volume;
		// Dry
		m_register_map[(i << 1) + 0xdb] = (uint16_t*)&m_alt_filter[i].m_table_pos;
		m_register_map[(i << 1) + 0xdf] = (uint16_t*)&m_dry[i].m_delay;
		m_register_map[(i << 1) + 0xe5] = (uint16_t*)&m_dry[i].m_volume;
	}
}

INLINE int16_t read_sample(uint16_t bank, uint16_t address)
{
	const uint32_t rom_addr = ((bank & 0x7FFF) << 16) | (address << 0);
	const uint8_t sample_data = qsound_sample_rom[rom_addr & qsound_sample_rom_mask];
	return (int16_t)(sample_data << 8); // bit0-7 is tied to ground
}

// updates one DSP sample
INLINE void update_sample()
{
	switch (m_state)
	{
		default:
		case STATE_INIT1:
		case STATE_INIT2:
			state_init();
			return;
		case STATE_REFRESH1:
			state_refresh_filter_1();
			return;
		case STATE_REFRESH2:
			state_refresh_filter_2();
			return;
		case STATE_NORMAL1:
		case STATE_NORMAL2:
			state_normal_update();
			return;
	}
}

// Initialization routine
static void state_init()
{
	int i;
	int mode = (m_state == STATE_INIT2) ? 1 : 0;

	// we're busy for 4 samples, including the filter refresh.
	if (m_state_counter >= 2)
	{
		m_state_counter = 0;
		m_state = m_next_state;
		return;
	}
	else if (m_state_counter == 1)
	{
		m_state_counter++;
		return;
	}

	memset(m_voice, 0, sizeof(m_voice));
	memset(m_adpcm, 0, sizeof(m_adpcm));
	memset(m_filter, 0, sizeof(m_filter));
	memset(m_alt_filter, 0, sizeof(m_alt_filter));
	memset(m_wet, 0, sizeof(m_wet));
	memset(m_dry, 0, sizeof(m_dry));
	memset(&m_echo, 0, sizeof(m_echo));

	for (i = 0; i < 19; i++)
	{
		m_voice_pan[i] = DATA_PAN_TAB + 0x10;
		m_voice_output[i] = 0;
	}

	for (i = 0; i < 16; i++)
		m_voice[i].m_bank = 0x8000;
	for (i = 0; i < 3; i++)
		m_adpcm[i].m_bank = 0x8000;

	if (mode == 0)
	{
		// mode 1
		m_wet[0].m_delay = 0;
		m_dry[0].m_delay = 46;
		m_wet[1].m_delay = 0;
		m_dry[1].m_delay = 48;
		m_filter[0].m_table_pos = DATA_FILTER_TAB + (FILTER_ENTRY_SIZE*1);
		m_filter[1].m_table_pos = DATA_FILTER_TAB + (FILTER_ENTRY_SIZE*2);
		m_echo.m_end_pos = DELAY_BASE_OFFSET + 6;
		m_next_state = STATE_REFRESH1;
	}
	else
	{
		// mode 2
		m_wet[0].m_delay = 1;
		m_dry[0].m_delay = 0;
		m_wet[1].m_delay = 0;
		m_dry[1].m_delay = 0;
		m_filter[0].m_table_pos = 0xf73;
		m_filter[1].m_table_pos = 0xfa4;
		m_alt_filter[0].m_table_pos = 0xf73;
		m_alt_filter[1].m_table_pos = 0xfa4;
		m_echo.m_end_pos = DELAY_BASE_OFFSET2 + 6;
		m_next_state = STATE_REFRESH2;
	}

	m_wet[0].m_volume = 0x3fff;
	m_dry[0].m_volume = 0x3fff;
	m_wet[1].m_volume = 0x3fff;
	m_dry[1].m_volume = 0x3fff;

	m_delay_update = 1;
	m_ready_flag = 0;
	m_state_counter = 1;
}

// Updates filter parameters for mode 1
static void state_refresh_filter_1()
{
	int ch;
	for (ch = 0; ch < 2; ch++)
	{
		int i;

		m_filter[ch].m_delay_pos = 0;
		m_filter[ch].m_tap_count = 95;

		for (i = 0; i < 95; i++)
			m_filter[ch].m_taps[i] = read_dsp_rom(m_filter[ch].m_table_pos + i);
	}

	m_state = m_next_state = STATE_NORMAL1;
}

// Updates filter parameters for mode 2
static void state_refresh_filter_2()
{
	int ch;
	for (ch = 0; ch < 2; ch++)
	{
		int i;

		m_filter[ch].m_delay_pos = 0;
		m_filter[ch].m_tap_count = 45;

		for (i = 0; i < 45; i++)
			m_filter[ch].m_taps[i] = (int16_t)read_dsp_rom(m_filter[ch].m_table_pos + i);

		m_alt_filter[ch].m_delay_pos = 0;
		m_alt_filter[ch].m_tap_count = 44;

		for (i = 0; i < 44; i++)
			m_alt_filter[ch].m_taps[i] = (int16_t)read_dsp_rom(m_alt_filter[ch].m_table_pos + i);
	}

	m_state = m_next_state = STATE_NORMAL2;
}

// Updates a PCM voice. There are 16 voices, each are updated every sample
// with full rate and volume control.
INLINE int16_t qsound_voice_update(qsound_voice* qv, int32_t *echo_out)
{
	int32_t new_phase;
	// Read sample from rom and apply volume
	const int16_t output = (qv->m_volume * read_sample(qv->m_bank, qv->m_addr)) >> 14;

	*echo_out += (output * qv->m_echo) << 2;

	// Add delta to the phase and loop back if required
	new_phase = qv->m_rate + ((qv->m_addr << 12) | (qv->m_phase >> 4));

	if ((new_phase >> 12) >= qv->m_end_addr)
		new_phase -= (qv->m_loop_len << 12);

	new_phase = min(max(new_phase, (int32_t)-0x8000000), (int32_t)0x7FFFFFF);
	qv->m_addr = new_phase >> 12;
	qv->m_phase = (new_phase << 4) & 0xffff;

	return output;
}

// Updates an ADPCM voice. There are 3 voices, one is updated every sample
// (effectively making the ADPCM rate 1/3 of the master sample rate), and
// volume is set when starting samples only.
// The ADPCM algorithm is supposedly similar to Yamaha ADPCM. It also seems
// like Capcom never used it, so this was not emulated in the earlier QSound
// emulators.
INLINE int16_t qsound_adpcm_update(qsound_adpcm *qa, int16_t curr_sample, int nibble)
{
	int8_t step;
	int32_t delta;
	if (!nibble)
	{
		// Mute voice when it reaches the end address.
		if (qa->m_cur_addr == qa->m_end_addr)
			qa->m_cur_vol = 0;

		// Playback start flag
		if (qa->m_flag)
		{
			curr_sample = 0;
			qa->m_flag = 0;
			qa->m_step_size = 10;
			qa->m_cur_vol = qa->m_volume;
			qa->m_cur_addr = qa->m_start_addr;
		}

		// get top nibble
		step = read_sample(qa->m_bank, qa->m_cur_addr) >> 8;
	}
	else
	{
		// get bottom nibble
		step = read_sample(qa->m_bank, qa->m_cur_addr++) >> 4;
	}

	// shift with sign extend
	step >>= 4;

	// delta = (0.5 + abs(step)) * m_step_size
	delta = ((1 + abs(step << 1)) * qa->m_step_size) >> 1;
	if (step <= 0)
		delta = -delta;
	delta += curr_sample;
	delta = min(max(delta, (int32_t)-32768), (int32_t)32767);

	qa->m_step_size = (read_dsp_rom(DATA_ADPCM_TAB + 8 + step) * qa->m_step_size) >> 6;
	qa->m_step_size = min(max(qa->m_step_size, (int16_t)1), (int16_t)2000);

	return (delta * qa->m_cur_vol) >> 16;
}

// The echo effect is pretty simple. A moving average filter is used on
// the output from the delay line to smooth samples over time.
INLINE int16_t qsound_echo_apply(qsound_echo *qe, int32_t input)
{
	int32_t new_sample;
	// get average of last 2 samples from the delay line
	int32_t old_sample = qe->m_delay_line[qe->m_delay_pos];
	const int32_t last_sample = qe->m_last_sample;
	qe->m_last_sample = old_sample;
	old_sample = (old_sample + last_sample) >> 1;

	// add current sample to the delay line
	new_sample = input + ((old_sample * qe->m_feedback) << 2);
	qe->m_delay_line[qe->m_delay_pos++] = new_sample >> 16;

	if (qe->m_delay_pos >= qe->m_length)
		qe->m_delay_pos = 0;

	return old_sample;
}

// Process a sample update
static void state_normal_update()
{
	int i,ch;
	int32_t echo_input;
	int16_t echo_output;
	int adpcm_voice;

	m_ready_flag = 0x80;

	// recalculate echo length
	if (m_state == STATE_NORMAL2)
		m_echo.m_length = m_echo.m_end_pos - DELAY_BASE_OFFSET2;
	else
		m_echo.m_length = m_echo.m_end_pos - DELAY_BASE_OFFSET;

	m_echo.m_length = min(max(m_echo.m_length, (int16_t)0), (int16_t)1024);

	// update PCM voices
	echo_input = 0;
	for (i = 0; i < 16; i++)
		m_voice_output[i] = qsound_voice_update(m_voice+i, &echo_input);

	// update ADPCM voices (one every third sample)
	adpcm_voice = m_state_counter % 3;
	m_voice_output[16 + adpcm_voice] = qsound_adpcm_update(m_adpcm+adpcm_voice, m_voice_output[16 + adpcm_voice], m_state_counter / 3);

	echo_output = qsound_echo_apply(&m_echo,echo_input);

	// now, we do the magic stuff
	for (ch = 0; ch < 2; ch++)
	{
		int32_t output;
		// Echo is output on the unfiltered component of the left channel and
		// the filtered component of the right channel.
		int32_t wet = (ch == 1) ? echo_output << 14 : 0;
		int32_t dry = (ch == 0) ? echo_output << 14 : 0;

		for (i = 0; i < 19; i++)
		{
			uint16_t pan_index = m_voice_pan[i] + (ch * PAN_TABLE_CH_OFFSET);

			// Apply different volume tables on the dry and wet inputs.
			dry -= (m_voice_output[i] * (int16_t)read_dsp_rom(pan_index + PAN_TABLE_DRY));
			wet -= (m_voice_output[i] * (int16_t)read_dsp_rom(pan_index + PAN_TABLE_WET));
		}
		// Saturate accumulated voices
		dry = (min(max(dry, (int32_t)-0x1fffffff), (int32_t)0x1fffffff)) << 2;
		wet = (min(max(wet, (int32_t)-0x1fffffff), (int32_t)0x1fffffff)) << 2;

		// Apply FIR filter on 'wet' input
		wet = qsound_fir_apply(m_filter+ch, wet >> 16);

		// in mode 2, we do this on the 'dry' input too
		if (m_state == STATE_NORMAL2)
			dry = qsound_fir_apply(m_alt_filter+ch,dry >> 16);

		// output goes through a delay line and attenuation
		output = (qsound_delay_apply(m_wet+ch,wet) + qsound_delay_apply(m_dry+ch,dry));

		// DSP round function
		output = (output + 0x2000) & ~0x3fff;
		m_out[ch] = (min(max(output >> 14, (int32_t)-0x7fff), (int32_t)0x7fff));

		if (m_delay_update)
		{
			qsound_delay_update(m_wet+ch);
			qsound_delay_update(m_dry+ch);
		}
	}

	m_delay_update = 0;

	// after 6 samples, the next state is executed.
	m_state_counter++;
	if (m_state_counter > 5)
	{
		m_state_counter = 0;
		m_state = m_next_state;
	}
}

// Apply the FIR filter used as the Q1 transfer function
static int32_t qsound_fir_apply(qsound_fir *qf, int16_t input)
{
	int32_t output = 0, tap = 0;
	for (; tap < (qf->m_tap_count - 1); tap++)
	{
		output -= (qf->m_taps[tap] * qf->m_delay_line[qf->m_delay_pos++]) << 2;

		if (qf->m_delay_pos >= qf->m_tap_count - 1)
			qf->m_delay_pos = 0;
	}

	output -= (qf->m_taps[tap] * input) << 2;

	qf->m_delay_line[qf->m_delay_pos++] = input;
	if (qf->m_delay_pos >= qf->m_tap_count - 1)
		qf->m_delay_pos = 0;

	return output;
}

// Apply delay line and component volume
INLINE int32_t qsound_delay_apply(qsound_delay *qd, const int32_t input)
{
	int32_t output;

	qd->m_delay_line[qd->m_write_pos++] = input >> 16;
	if (qd->m_write_pos >= 51)
		qd->m_write_pos = 0;

	output = qd->m_delay_line[qd->m_read_pos++] * qd->m_volume;
	if (qd->m_read_pos >= 51)
		qd->m_read_pos = 0;

	return output;
}

// Update the delay read position to match new delay length
INLINE void qsound_delay_update(qsound_delay *qd)
{
	const int16_t new_read_pos = (qd->m_write_pos - qd->m_delay) % 51;
	if (new_read_pos < 0)
		qd->m_read_pos = new_read_pos + 51;
	else
		qd->m_read_pos = new_read_pos;
}

void qsound_update( int num, INT16 **buffer, int length )
{
	int i;
	for (i = 0; i < length; i++)
	{
		update_sample();
		buffer[0][i] = m_out[0];
		buffer[1][i] = m_out[1];
	}

#if LOG_WAVE
	fwrite(buffer[0], length*sizeof(INT16), 1, fpRawDataL);
	fwrite(buffer[1], length*sizeof(INT16), 1, fpRawDataR);
#endif
}
