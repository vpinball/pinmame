/**********************************************************************************************
 *
 *   Texas Instruments TMS320AV120 MPG Decoder
 *   by Steve Ellenoff
 *
 **********************************************************************************************/


#ifndef TMS320AV120_H
#define TMS320AV120_H

#define MAX_TMS320AV120 2

struct TMS320AV120interface
{
        int num;                                /* total number of chips */
        int mixing_level[MAX_TMS320AV120];      /* master volume */
};

int TMS320AV120_sh_start(const struct MachineSound *msound);
void TMS320AV120_sh_stop(void);
void TMS320AV120_sh_reset(void);
void TMS320AV120_sh_update(void);

WRITE_HANDLER( TMS320AV120_data_0_w );
WRITE_HANDLER( TMS320AV120_data_1_w );

#endif
