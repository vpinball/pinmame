#pragma once

/* an interface for the MSM5205 and similar chips */

#define MAX_MSM5205 4

/* prescaler selector defines   */
/* default master clock is 384kHz */
#define MSM5205_S96_3B 0     /* prescaler 1/96(4KHz) , data 3bit */
#define MSM5205_S48_3B 1     /* prescaler 1/48(8KHz) , data 3bit */
#define MSM5205_S64_3B 2     /* prescaler 1/64(6KHz) , data 3bit */
#define MSM5205_SEX_3B 3     /* VCLK slave mode      , data 3bit */
#define MSM5205_S96_4B 4     /* prescaler 1/96(4KHz) , data 4bit */
#define MSM5205_S48_4B 5     /* prescaler 1/48(8KHz) , data 4bit */
#define MSM5205_S64_4B 6     /* prescaler 1/64(6KHz) , data 4bit */
#define MSM5205_SEX_4B 7     /* VCLK slave mode      , data 4bit */

#define MSM6585_S160_4B (4|8)/* prescaler 1/160(4KHz), data 4bit */
#define MSM6585_S80_4B (5|8) /* prescaler 1/80(8KHz) , data 4bit */
#define MSM6585_S40_4B (6|8) /* prescaler 1/40(16KHz), data 4bit */
#define MSM6585_S20_4B (7|8) /* prescaler 1/20(32KHz), data 4bit */

// | 8 = msm6585, but only 4 bit modes and master clock 640kHz

/* Correct against the datasheet's "Functional Description, 1. Sampling Frequency" table, at a 640kHz master clock:
     S1 S2   fSAM    fCUT    div        S1 S2   fSAM    fCUT    div
     L  L     4kHz  1.6kHz  /160        L  H    16kHz  6.4kHz   /40
     H  L     8kHz  3.2kHz   /80        H  H    32kHz 12.8kHz   /20
   With S1 as the code's low bit that is 0..3 -> 160/80/40/20, which prescaler_table in
   msm5205.c now holds; MAME computes the same as (S1 ? 20 : 40) * (S2 ? 1 : 4). (matches MAME)

   The 5205 table (fosc=384kHz) is L/L 4kHz (/96), L/H 6kHz (/64), H/L 8kHz (/48), H/H prohibited - undocumented slave mode (MAME).

   The likely resolution for spinball is the oscillator, not the pins: the 5205
   datasheet notes that "a 384kHz oscillator can be used to select 4kHz, 6kHz or 8kHz.  A
   768kHz oscillator can be used to select 8kHz, 12kHz or 16kHz", so double-clocking this
   family is normal.  At 1.28MHz, S2=L with S1 toggling gives 8 and 16KHz - which fits the
   schematic as drawn, our 16KHz as the S1=H leg (and MAME's current 4KHz being a full 4x slow),
   and spinb.c's "toggles between 4Khz, 8Khz" header as those same pin states computed at 640kHz.
   Settling it needs the crystal data from a real board */

struct MSM5205interface
{
	int num;                       /* total number of chips                 */
	double baseclock;              /* master clock (default = 384KHz)       */ //640 for msm6585
	void (*vclk_callback[MAX_MSM5205])(int);   /* VCLK callback             */
	int select[MAX_MSM5205];       /* prescaler / bit width selector        */
	int variant[MAX_MSM5205];      /* 0=msm5205, 1=msm6585                  */
	int mixing_level[MAX_MSM5205]; /* master volume                         */
};

int MSM5205_sh_start (const struct MachineSound *msound);
void MSM5205_sh_stop (void);   /* empty this function */
void MSM5205_sh_update (void); /* empty this function */
void MSM5205_sh_reset (void);

/* reset signal should keep for 2cycle of VCLK      */
void MSM5205_reset_w (int num, int reset);
/* adpcmata is latched after vclk_interrupt callback */
void MSM5205_data_w (int num, int data);
/* VCLK slave mode option                                        */
/* if VCLK and reset or data is changed at the same time,        */
/* Call MSM5205_vclk_w after MSM5205_data_w and MSM5205_reset_w. */
void MSM5205_vclk_w (int num, int reset);
/* option , selected pin seletor */
void MSM5205_playmode_w(int num,int _select);

void MSM5205_set_volume(int num,int volume);
