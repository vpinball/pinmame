/**********************************************************************************************
 *
 *   Texas Instruments TMS320AV120 MPG Decoder
 *   by Steve Ellenoff
 *   10/14/2003
 *
 **********************************************************************************************/


#ifndef TMS320AV120_H
#define TMS320AV120_H

#define MAX_TMS320AV120 2

struct TMS320AV120interface
{
        int num;											/* total number of chips */
        int mixing_level[MAX_TMS320AV120];					/* master volume */
		void (*bof_line)(int chipnum,int state);			/* BOF Line Callback */
		void (*sreq_line)(int chipnum,int state);			/* SREQ Line Callback */
};

/*Called from MAME core*/
int TMS320AV120_sh_start(const struct MachineSound *msound);
void TMS320AV120_sh_stop(void);
void TMS320AV120_sh_reset(void);
void TMS320AV120_sh_update(void);

/*Externally called functions from drivers*/
WRITE_HANDLER( TMS320AV120_data_w );
void TMS320AV120_set_mute(int chipnum, int state);
void TMS320AV120_set_reset(int chipnum, int state);

#endif

