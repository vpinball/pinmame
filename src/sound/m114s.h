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

#define MAX_M114S 1

struct M114Sinterface
{
        int num;                             /* total number of chips */
        int baseclock[MAX_M114S];            /* input clock */
        int region[MAX_M114S];               /* memory region where the sample ROM lives */
        int mixing_level[MAX_M114S];         /* master volume */
		int eatbytes[MAX_M114S];			 /* # of bytes to eat (ignore) at start up before storing data to registers*/
};

int M114S_sh_start(const struct MachineSound *msound);
void M114S_sh_stop(void);
void M114S_sh_reset(void);

WRITE_HANDLER( M114S_data_w );

#endif
