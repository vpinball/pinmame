/**********************************************************************************************
 *
 *   SGS-Thomson Microelectronics M114S/M114A/M114AF Digital Sound Generator
 *   by Steve Ellenoff
 *   09/02/2004
 *
 *   Thanks to R.Belmont for Support & Agreeing to read through that nasty data sheet with me..
 *   Big thanks to Destruk for help in tracking down the data sheet.. Could have never done it
 *   without it!!
 *
 *
 *   Code based largely on Aaron Gile's BSMT2000 driver.
 **********************************************************************************************/

#ifndef M114S_H
#define M114S_H

//If set to 1, we mix down the 16 channels into the 4 appropriate output channels
//If set to 0, we simply output each 16 channel independantly (which is not how the real chip works)
#define USE_REAL_OUTPUTS 1
//Test the volume envelope
#define USE_VOL_ENVELOPE 1

#if USE_REAL_OUTPUTS
#define M114S_OUTPUT_CHANNELS 4
#else
#define M114S_OUTPUT_CHANNELS 16
#endif

#define MAX_M114S 1

struct M114Sinterface
{
        int num;                             /* total number of chips */
        int baseclock[MAX_M114S];            /* input clock - Allowed values are 4Mhz & 6Mhz only! */
        int region[MAX_M114S];               /* memory region where the sample ROM lives */
        int mixing_level[MAX_M114S];         /* master volume */
		int cpunum[MAX_M114S];				 /* # of the cpu controlling the M114S */
};

int M114S_sh_start(const struct MachineSound *msound);
void M114S_sh_stop(void);
void M114S_sh_reset(void);

WRITE_HANDLER( M114S_data_w );

#endif
