#ifndef DAC_H
#define DAC_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PINMAME
#define MAX_DAC 8
#else
#define MAX_DAC 4
#endif

struct DACinterface
{
	int num;	/* total number of DACs */
	int mixing_level[MAX_DAC];
};

int DAC_sh_start(const struct MachineSound *msound);

#ifdef PINMAME
void DAC_set_reverb_filter(int num, float delay, float force);
void DAC_set_mixing_level(int num, int pctvol);
#endif

void DAC_data_w(int num,int data);
void DAC_signed_data_w(int num,int data);
void DAC_data_16_w(int num,int data);
void DAC_DC_offset_correction_data_16_w(int num, int data);
void DAC_signed_data_16_w(int num,int data);

WRITE_HANDLER( DAC_0_data_w );
WRITE_HANDLER( DAC_1_data_w );
WRITE_HANDLER( DAC_0_signed_data_w );
WRITE_HANDLER( DAC_1_signed_data_w );

#ifdef __cplusplus
}
#endif

#endif
