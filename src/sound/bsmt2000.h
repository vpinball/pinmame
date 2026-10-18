/**********************************************************************************************
 *
 *   Data East BSMT2000 driver
 *   by Aaron Giles, Steve Ellenoff, Martin Adrian & Carsten Waechter
 *
 **********************************************************************************************/

#pragma once

#define MAX_BSMT2000 1

struct BSMT2000interface
{
        int num;                                /* total number of chips */
        double baseclock[MAX_BSMT2000];         /* input clock */
        int voices[MAX_BSMT2000];               /* number of voices (11 or 12) */
        int region[MAX_BSMT2000];               /* memory region where the sample ROM lives */
        int mixing_level[MAX_BSMT2000];         /* master volume */
#ifdef PINMAME
        int use_de_rom_banking;                 /* special flag to use Data East rom bank handling */
        int shift_data;                         /* not wired up anymore/unused!!  shift integer to apply to samples for changing volume - this is most likely done external to the bsmt chip in the real hardware */
        int reverse_stereo;                     /* special flag to determine if left and right channels should be reversed, seems like this is not needed anymore, as Mystery Castle, Pistol Poker and Al's Garage Band do not need it anymore (see sound test: left/right speaker test) */
#endif
};

int BSMT2000_sh_start(const struct MachineSound *msound);
void BSMT2000_sh_stop(void);
void BSMT2000_sh_reset(void);

WRITE16_HANDLER( BSMT2000_data_0_w );

/* Low level emulation (the chip's own program, see bsmt2000.c); all of these also work with the HLE */
int  BSMT2000_lle_active(void);                          /* nonzero when the real chip program runs */
int  BSMT2000_status_0_r(void);                          /* 1: ready for the next write, 0: the last one is still pending (HLE: always 1) */
void BSMT2000_set_ready_callback(int num, void (*cb)(void)); /* called when the chip takes a data word (LLE only) */
