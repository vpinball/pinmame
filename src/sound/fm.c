#define YM2610B_WARNING

/* YM2608 rhythm data is PCM ,not an ADPCM */
#define YM2608_RHYTHM_PCM

/*
**
** File: fm.c -- software implementation of Yamaha FM sound generator
**
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmulator development
**
** Version 0.37f
**
*/

/*
** History:
**
** 18-12-2001 Jarek Burczynski:
**  - added SSG-EG support (verified on real chip)
**
** 12-08-2001 Jarek Burczynski:
**  - corrected sin_tab and tl_tab data (verified on real chip)
**  - corrected feedback calculations (verified on real chip)
**  - corrected phase generator calculations (verified on real chip)
**  - corrected envelope generator calculations (verified on real chip)
**  - corrected FM volume level (YM2610 and YM2610B).
**  - changed YMxxxUpdateOne() functions (YM2203, YM2608, YM2610, YM2610B, YM2612) :
**    this was needed to calculate YM2610 FM channels output correctly.
**    (Each FM channel is calculated as in other chips, but the output of the channel
**    gets shifted right by one *before* sending to accumulator. That was impossible to do
**    with previous implementation).
**
** 23-07-2001 Jarek Burczynski, Nicola Salmoria:
**  - corrected YM2610 ADPCM type A algorithm and tables (verified on real chip)
**
** 11-06-2001 Jarek Burczynski:
**  - corrected end of sample bug in OPNB_ADPCM_CALC_CHA.
**    Real YM2610 checks for equality between current and end addresses (only 20 LSB bits).
**
** 08-12-98 hiro-shi:
** rename ADPCMA -> ADPCMB, ADPCMB -> ADPCMA
** move ROM limit check.(CALC_CH? -> 2610Write1/2)
** test program (ADPCMB_TEST)
** move ADPCM A/B end check.
** ADPCMB repeat flag(no check)
** change ADPCM volume rate (8->16) (32->48).
**
** 09-12-98 hiro-shi:
** change ADPCM volume. (8->16, 48->64)
** replace ym2610 ch0/3 (YM-2610B)
** init cur_chip (restart bug fix)
** change ADPCM_SHIFT (10->8) missing bank change 0x4000-0xffff.
** add ADPCM_SHIFT_MASK
** change ADPCMA_DECODE_MIN/MAX.
*/



/*
	TO DO:
!!!!!!!	CORRECT FIRST MISSING CREDIT SOUND IN GIGANDES (DELTA-T module, when DELTAN register = 0) !!!!!!
		- use real sample rate and let mixer.c do the sample rate convertion

	no check:
		YM2608 rhythm sound
		YM2151 CSM speech mode

	no support:
		YM2608 status mask (register :0x110)
		YM2608 RYTHM sound
		YM2608 PCM memory data access , DELTA-T-ADPCM with PCM port
		YM2151 CSM speech mode with internal timer

	preliminary :
		key scale level rate (?)
		YM2151 noise mode (CH7.OP4)
		LFO contoller (YM2612/YM2610/YM2608/YM2151)

	note:
                        OPN                           OPM
		fnum          fM * 2^20 / (fM/(12*n))
		TimerOverA    ( 12*n)*(1024-NA)/fM        64*(1024-Na)/fM
		TimerOverB    (192*n)*(256-NB)/fM       1024*(256-Nb)/fM
		output bits   10bit<<3bit               16bit * 2ch (YM3012=10bit<<3bit)
		sampling rate fFM / (12*prescaler)      fM / 64
*/

/************************************************************************/
/*    comment of hiro-shi(Hiromitsu Shioya)                             */
/*    YM2610(B) = OPN-B                                                 */
/*    YM2610  : PSG:3ch FM:4ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/*    YM2610B : PSG:3ch FM:6ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifndef __RAINE__
#include "driver.h"		/* use M.A.M.E. */
#include "state.h"
#else
#include "deftypes.h"		/* use RAINE */
#include "support.h"		/* use RAINE */
#endif

#include "ay8910.h"
#include "fm.h"


#ifndef PI
#define PI 3.14159265358979323846
#endif


/***** shared function building option ****/
#define BUILD_OPN (BUILD_YM2203||BUILD_YM2608||BUILD_YM2610||BUILD_YM2610B||BUILD_YM2612)
#define BUILD_OPNB (BUILD_YM2610||BUILD_YM2610B)
#define BUILD_OPN_PRESCALER (BUILD_YM2203||BUILD_YM2608)
#define BUILD_ADPCMA (BUILD_YM2608||BUILD_YM2610||BUILD_YM2610B)
#define BUILD_ADPCMB (BUILD_YM2608||BUILD_YM2610||BUILD_YM2610B)


#if BUILD_ADPCMB
/* include external DELTA-T ADPCM unit */
  #include "ymdeltat.h"		/* DELTA-T ADPCM UNIT */
#endif

/* -------------------- sound quality define selection --------------------- */
#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define ENV_SH			16  /* 16.16 fixed point (envelope calculations)  */
#define LFO_SH			23  /*  9.23 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)
#define ENV_MASK		((1<<ENV_SH)-1)

/* envelope output entries */
#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)
#define ENV_QUIET		((int)(0x68/(ENV_STEP)))

#define MAX_ATT_INDEX	((ENV_LEN<<ENV_SH)-1) /* 1023.ffff */
#define MIN_ATT_INDEX	(      (1<<ENV_SH)-1) /*    0.ffff */

/* sinwave entries */
#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256)	/* 8 bits addressing (real chip) */


/* LFO table entries */
#define LFO_ENT 512
#define LFO_RATE 0x10000
#define PMS_RATE 0x400
/* LFO runtime work */
static UINT32 lfo_amd;
static INT32 lfo_pmd;
#if BUILD_YM2610B
static UINT32 LFOCnt,LFOIncr;	/* LFO Phase Generator */
#endif
/* OPN LFO waveform table */
//static INT32 OPN_LFO_wave[LFO_ENT];

/* -------------------- tables --------------------- */

/* sustain level table (3db per step) */
/* bit0, bit1, bit2, bit3, bit4, bit5, bit6 */
/* 1,    2,    4,    8,    16,   32,   64   (value)*/
/* 0.75, 1.5,  3,    6,    12,   24,   48   (dB)*/

/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (UINT32) ( db * (4.0/ENV_STEP) * (1<<ENV_SH) )
static const UINT32 SL_TABLE[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC

/*	TL_TAB_LEN is calculated as:
*	13 - sinus amplitude bits     (Y axis)
*	2  - sinus sign bit           (Y axis)
*	TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
static signed int tl_tab[TL_TAB_LEN];

/* sin waveform table in 'decibel' scale */
static unsigned int sin_tab[SIN_LEN];



#define OPM_DTTABLE OPN_DTTABLE
#if 0
static UINT8 OPN_DTTABLE[4 * 32]={
/* this is YM2151 and YM2612 phase increment data (in 10.10 fixed point format)*/
/* FD=0 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
	2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
	1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
	5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
	2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
	8 , 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

#endif

/* output final shift */
#if (FM_SAMPLE_BITS==16)
	#define FINAL_SH	(0)
	#define MAXOUT		(+32767)
	#define MINOUT		(-32768)
#else
	#define FINAL_SH	(8)
	#define MAXOUT		(+127)
	#define MINOUT		(-128)
#endif

/* -------------------- local defines , macros --------------------- */
/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)
#define OPM_CHAN(N) (N&7)
#define OPM_SLOT(N) ((N>>3)&3)

/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* bit0 = Right enable , bit1 = Left enable */
#define OUTD_RIGHT  1
#define OUTD_LEFT   2
#define OUTD_CENTER 3



/* ---------- debug section ------------------- */
/* save output as raw 16-bit sample */
/* #define SAVE_SAMPLE */

#ifdef SAVE_SAMPLE
static FILE *sample[1];
	#if 1	/*save to MONO file */
		#define SAVE_ALL_CHANNELS \
		{	signed int pom = lt; \
			fputc((unsigned short)pom&0xff,sample[0]); \
			fputc(((unsigned short)pom>>8)&0xff,sample[0]); \
		}
	#else	/*save to STEREO file */
		#define SAVE_ALL_CHANNELS \
		{	signed int pom = lt; \
			fputc((unsigned short)pom&0xff,sample[0]); \
			fputc(((unsigned short)pom>>8)&0xff,sample[0]); \
			pom = rt; \
			fputc((unsigned short)pom&0xff,sample[0]); \
			fputc(((unsigned short)pom>>8)&0xff,sample[0]); \
		}
	#endif
#endif


/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_slot {
	INT32		 *DT;	/* detune          :DT_TABLE[DT]		*/
	int			 DT2;	/* multiple,Detune2:(DT2<<4)|ML for OPM	*/
	UINT8		 KSR;	/* key scale rate  :3-KSR				*/
	UINT8		ARval;	/* current AR							*/
	const UINT32 *AR;	/* attack rate     :&AR_TABLE[AR<<1]	*/
	const UINT32 *DR;	/* decay rate      :&DR_TABLE[DR<<1]	*/
	const UINT32 *SR;	/* sustain rate    :&DR_TABLE[SR<<1]	*/
	const UINT32 *RR;	/* release rate    :&DR_TABLE[RR<<2+2]	*/
	UINT8		 ksr;	/* key scale rate  :kcode>>(3-KSR)		*/
	UINT32		 mul;	/* multiple        :ML_TABLE[ML]		*/

	/* Phase Generator */
	UINT32 Cnt;			/* phase counter						*/
	UINT32 Incr;		/* phase step							*/

	/* Envelope Generator */
	UINT8	state;		/* phase type							*/
	UINT32	TL;			/* total level     :TL << 3				*/
	INT32	volume;		/* envelope counter						*/
	UINT32	sl;			/* sustain level   :SL_TABLE[SL]		*/
	UINT32	delta_ar;	/* envelope step for Attack				*/
	UINT32	delta_dr;	/* envelope step for Decay				*/
	UINT32	delta_sr;	/* envelope step for Sustain			*/
	UINT32	delta_rr;	/* envelope step for Release			*/
	UINT8	SEG;		/* SSG-EG waveform						*/
    UINT8	SEGn;		/* SSG-EG negated output				*/

	UINT32	key;		/* 0=last key was KEY OFF, 1=KEY ON		*/

	/* LFO */
	UINT32	amon;		/* AMS enable flag						*/
	UINT32	ams;		/* AMS depth level of this SLOT			*/
}FM_SLOT;

typedef struct fm_chan {
	FM_SLOT	SLOT[4];
	UINT8 ALGO;			/* algorithm						*/
	UINT8 FB;			/* feedback shift					*/
	INT32 op1_out[2];	/* op1 output for feedback			*/
	/* algorithm (connection) */
	INT32 *connect1;	/* pointer of SLOT1 output			*/
	INT32 *connect2;	/* pointer of SLOT2 output			*/
	INT32 *connect3;	/* pointer of SLOT3 output			*/
	INT32 *connect4;	/* pointer of SLOT4 output			*/
	/* LFO */
	INT32 pms;			/* PMS depth channel level			*/
	UINT32 ams;			/* AMS depth channel level			*/
	/* Phase Generator */
	UINT32 fc;			/* fnum,blk:adjusted to sample rate	*/
	UINT8 kcode;		/* key code:						*/
} FM_CH;

/* OPN/OPM common state */
typedef struct fm_state {
	UINT8 index;		/* chip index (number of chip)	*/
	int clock;			/* master clock  (Hz)	*/
	int rate;			/* sampling rate (Hz)	*/
	double freqbase;	/* frequency base		*/
	double TimerBase;	/* Timer base time		*/
#if FM_BUSY_FLAG_SUPPORT
	double BusyExpire;	/* ExpireTime of Busy clear */
#endif
	UINT8 address;		/* address register		*/
	UINT8 irq;			/* interrupt level		*/
	UINT8 irqmask;		/* irq mask				*/
	UINT8 status;		/* status flag			*/
	UINT32 mode;		/* mode  CSM / 3SLOT	*/
	UINT8 prescaler_sel;/* prescaler selector	*/
	UINT8 fn_h;			/* freq latch			*/
	int TA;				/* timer a				*/
	int TAC;			/* timer a counter		*/
	UINT8 TB;			/* timer b				*/
	int TBC;			/* timer b counter		*/
	/* local time tables */
	INT32 DT_TABLE[8][32];		/* DeTune table		*/
	UINT32 eg_tab [32+64+32];	/* Envelope Generator rates (32 + 64 rates + 32 RKS) */
	/* Extention Timer and IRQ handler */
	FM_TIMERHANDLER	Timer_Handler;
	FM_IRQHANDLER	IRQ_Handler;
}FM_ST;


/* -------------------- state --------------------- */

/* some globals */
#define TYPE_SSG    0x01    /* SSG support          */
//#define	xxxxxx		0x02	/* not used */
#define TYPE_LFOPAN 0x04    /* OPN type LFO and PAN */
#define TYPE_6CH    0x08    /* FM 6CH / 3CH         */
#define TYPE_DAC    0x10    /* YM2612's DAC device  */
#define TYPE_ADPCM  0x20    /* two ADPCM units      */


#define TYPE_YM2203 (TYPE_SSG)
#define TYPE_YM2608 (TYPE_SSG |TYPE_LFOPAN |TYPE_6CH |TYPE_ADPCM)
#define TYPE_YM2610 (TYPE_SSG |TYPE_LFOPAN |TYPE_6CH |TYPE_ADPCM)
#define TYPE_YM2612 (TYPE_DAC |TYPE_LFOPAN |TYPE_6CH)

/* current chip state */
//static void *cur_chip = 0;		/* pointer of current chip struct */
//static FM_ST  *State;			/* basic status */
//static FM_CH  *cch[8];			/* pointer of FM channels */


/* runtime work */
static INT32 out_fm[8];			/* outputs of working channels */
#if BUILD_ADPCMA
static INT32 out_adpcm[4];		/* channel output NONE,LEFT,RIGHT or CENTER for YM2610 ADPCM */
static INT32 out_delta[4];		/* channel output NONE,LEFT,RIGHT or CENTER for YM2610 DELTAT*/
#endif
static INT32 pg_in2,pg_in3,pg_in4;	/* PG input of SLOTs */


/* -------------------- log output  -------------------- */
/* log output level */
#define LOG_ERR  3      /* ERROR       */
#define LOG_WAR  2      /* WARNING     */
#define LOG_INF  1      /* INFORMATION */
#define LOG_LEVEL LOG_INF

#ifndef __RAINE__
#define LOG(n,x) if( (n)>=LOG_LEVEL ) logerror x
#endif

/* ----- limitter ----- */
#define Limit(val, max,min) { \
	if ( val > max )      val = max; \
	else if ( val < min ) val = min; \
}

/* ----- buffering one of data(STEREO chip) ----- */
#if FM_STEREO_MIX
/* stereo mixing */
#define FM_BUFFERING_STEREO \
{														\
	/* get left & right output with clipping */			\
	out_ch[OUTD_LEFT]  += out_ch[OUTD_CENTER];				\
	Limit( out_ch[OUTD_LEFT] , MAXOUT, MINOUT );	\
	out_ch[OUTD_RIGHT] += out_ch[OUTD_CENTER];				\
	Limit( out_ch[OUTD_RIGHT], MAXOUT, MINOUT );	\
	/* buffering */										\
	*bufL++ = out_ch[OUTD_LEFT] >>FINAL_SH;				\
	*bufL++ = out_ch[OUTD_RIGHT]>>FINAL_SH;				\
}
#else
/* stereo separate */
#define FM_BUFFERING_STEREO \
{														\
	/* get left & right output with clipping */			\
	out_ch[OUTD_LEFT]  += out_ch[OUTD_CENTER];				\
	Limit( out_ch[OUTD_LEFT] , MAXOUT, MINOUT );	\
	out_ch[OUTD_RIGHT] += out_ch[OUTD_CENTER];				\
	Limit( out_ch[OUTD_RIGHT], MAXOUT, MINOUT );	\
	/* buffering */										\
	bufL[i] = out_ch[OUTD_LEFT] >>FINAL_SH;				\
	bufR[i] = out_ch[OUTD_RIGHT]>>FINAL_SH;				\
}
#endif

#if FM_INTERNAL_TIMER
/* ----- internal timer mode , update timer */

/* ---------- calculate timer A ---------- */
	#define INTERNAL_TIMER_A(ST,CSM_CH)					\
	{													\
		if( ST->TAC &&  (ST->Timer_Handler==0) )		\
			if( (ST->TAC -= (int)(ST->freqbase*4096)) <= 0 )	\
			{											\
				TimerAOver( ST );						\
				/* CSM mode total level latch and auto key on */	\
				if( ST->mode & 0x80 )					\
					CSMKeyControll( CSM_CH );			\
			}											\
	}
/* ---------- calculate timer B ---------- */
	#define INTERNAL_TIMER_B(ST,step)						\
	{														\
		if( ST->TBC && (ST->Timer_Handler==0) )				\
			if( (ST->TBC -= (int)(ST->freqbase*4096*step)) <= 0 )	\
				TimerBOver( ST );							\
	}
#else /* FM_INTERNAL_TIMER */
/* external timer mode */
#define INTERNAL_TIMER_A(ST,CSM_CH)
#define INTERNAL_TIMER_B(ST,step)
#endif /* FM_INTERNAL_TIMER */

/* --------------------- subroutines  --------------------- */
/* status set and IRQ handling */
INLINE void FM_STATUS_SET(FM_ST *ST,int flag)
{
	/* set status flag */
	ST->status |= flag;
	if ( !(ST->irq) && (ST->status & ST->irqmask) )
	{
		ST->irq = 1;
		/* callback user interrupt handler (IRQ is OFF to ON) */
		if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->index,1);
	}
}

/* status reset and IRQ handling */
INLINE void FM_STATUS_RESET(FM_ST *ST,int flag)
{
	/* reset status flag */
	ST->status &=~flag;
	if ( (ST->irq) && !(ST->status & ST->irqmask) )
	{
		ST->irq = 0;
		/* callback user interrupt handler (IRQ is ON to OFF) */
		if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->index,0);
	}
}

/* IRQ mask set */
INLINE void FM_IRQMASK_SET(FM_ST *ST,int flag)
{
	ST->irqmask = flag;
	/* IRQ handling check */
	FM_STATUS_SET(ST,0);
	FM_STATUS_RESET(ST,0);
}

#if FM_BUSY_FLAG_SUPPORT
INLINE UINT8 FM_STATUS_FLAG(FM_ST *ST)
{
	if( ST->BusyExpire )
	{
		if( (ST->BusyExpire - FM_GET_TIME_NOW()) > 0)
			return ST->status | 0x80; /* with busy */
		/* expire */
		ST->BusyExpire = 0;
	}
	return ST->status;
}
INLINE void FM_BUSY_SET(FM_ST *ST,int busyclock )
{
	ST->BusyExpire = FM_GET_TIME_NOW() + (ST->TimerBase * busyclock);
}
#define FM_BUSY_CLEAR(ST) ((ST)->BusyExpire = 0)
#else
#define FM_STATUS_FLAG(ST) ((ST)->status)
#define FM_BUSY_SET(ST,bclock) {}
#define FM_BUSY_CLEAR(ST) {}
#endif


/* Envelope Generator phases */

//#define EG_DEC_SSG_EG	6
//#define EG_SUS_SSG_EG	5
//#define EG_HLD_SSG_EG	4
#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0



INLINE void FM_KEYON(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( !SLOT->key )
	{
		SLOT->key = 1;
		/* restart Phase Generator */
		SLOT->Cnt = 0;
		/* phase -> Attack */
		SLOT->state = EG_ATT;
	}
}

INLINE void FM_KEYOFF(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( SLOT->key )
	{
		SLOT->key = 0;
		/* phase -> Release */
		if (SLOT->state>EG_REL)
			SLOT->state = EG_REL;
	}
}

/* set algorithm connection */
static void setup_connection( FM_CH *CH, int ch )
{
	INT32 *carrier = &out_fm[ch];

	switch( CH->ALGO ){
	case 0:
		/*  PG---S1---S2---S3---S4---OUT */
		CH->connect1 = &pg_in2;
		CH->connect2 = &pg_in3;
		CH->connect3 = &pg_in4;
		break;
	case 1:
		/*  PG---S1-+-S3---S4---OUT */
		/*  PG---S2-+               */
		CH->connect1 = &pg_in3;
		CH->connect2 = &pg_in3;
		CH->connect3 = &pg_in4;
		break;
	case 2:
		/* PG---S1------+-S4---OUT */
		/* PG---S2---S3-+          */
		CH->connect1 = &pg_in4;
		CH->connect2 = &pg_in3;
		CH->connect3 = &pg_in4;
		break;
	case 3:
		/* PG---S1---S2-+-S4---OUT */
		/* PG---S3------+          */
		CH->connect1 = &pg_in2;
		CH->connect2 = &pg_in4;
		CH->connect3 = &pg_in4;
		break;
	case 4:
		/* PG---S1---S2-+--OUT */
		/* PG---S3---S4-+      */
		CH->connect1 = &pg_in2;
		CH->connect2 = carrier;
		CH->connect3 = &pg_in4;
		break;
	case 5:
		/*         +-S2-+     */
		/* PG---S1-+-S3-+-OUT */
		/*         +-S4-+     */
		CH->connect1 = 0;	/* special case */
		CH->connect2 = carrier;
		CH->connect3 = carrier;
		break;
	case 6:
		/* PG---S1---S2-+     */
		/* PG--------S3-+-OUT */
		/* PG--------S4-+     */
		CH->connect1 = &pg_in2;
		CH->connect2 = carrier;
		CH->connect3 = carrier;
		break;
	case 7:
		/* PG---S1-+     */
		/* PG---S2-+-OUT */
		/* PG---S3-+     */
		/* PG---S4-+     */
		CH->connect1 = carrier;
		CH->connect2 = carrier;
		CH->connect3 = carrier;
	}
	CH->connect4 = carrier;
}

/* set detune & multiple */
INLINE void set_det_mul(FM_ST *ST,FM_CH *CH,FM_SLOT *SLOT,int v)
{
	SLOT->mul = (v&0x0f)? (v&0x0f)*2 : 1;
	SLOT->DT  = ST->DT_TABLE[(v>>4)&7];
	CH->SLOT[SLOT1].Incr=-1;
}

/* set total level */
INLINE void set_tl(FM_CH *CH,FM_SLOT *SLOT , int v,int csmflag)
{
	SLOT->TL = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
}

/* set attack rate & key scale  */
INLINE void set_ar_ksr(FM_CH *CH,FM_SLOT *SLOT,int v,UINT32 *eg_tab)
{
	SLOT->KSR   = 3-(v>>6);
	SLOT->ARval = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
	SLOT->AR    = &eg_tab[ SLOT->ARval ];

	if ((SLOT->ARval + SLOT->ksr) < 32+62)
		SLOT->delta_ar = SLOT->AR[SLOT->ksr];
	else
		SLOT->delta_ar = MAX_ATT_INDEX+1;

	CH->SLOT[SLOT1].Incr=-1;	/* Optimize: only set this, if new SLOT->KSR is different */
}

/* set decay rate */
INLINE void set_dr(FM_SLOT *SLOT,int v,UINT32 *eg_tab)
{
	SLOT->DR = (v&0x1f) ? &eg_tab[32 + ((v&0x1f)<<1)] : &eg_tab[0];
	SLOT->delta_dr = SLOT->DR[SLOT->ksr];
}

/* set sustain rate */
INLINE void set_sr(FM_SLOT *SLOT,int v,UINT32 *eg_tab)
{
	SLOT->SR = (v&0x1f) ? &eg_tab[32 + ((v&0x1f)<<1)] : &eg_tab[0];
	SLOT->delta_sr = SLOT->SR[SLOT->ksr];
}

/* set release rate */
INLINE void set_sl_rr(FM_SLOT *SLOT,int v,UINT32 *eg_tab)
{
	SLOT->sl = SL_TABLE[ v>>4 ];
	SLOT->RR  = &eg_tab[34 + ((v&0x0f)<<2)];
	SLOT->delta_rr = SLOT->RR[SLOT->ksr];
}



INLINE signed int op_calc(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;

	p = (env<<3) + sin_tab[ ( ((signed int)((phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
}

INLINE signed int op_calc1(UINT32 phase, unsigned int env, signed int pm)
{
	UINT32 p;
	INT32  i;

	i = (phase & ~FREQ_MASK) + pm;

/*logerror("i=%08x (i>>16)&511=%8i phase=%i [pm=%08x] ",i, (i>>16)&511, phase>>FREQ_SH, pm);*/

	p = (env<<3) + sin_tab[ (i>>FREQ_SH) & SIN_MASK];

/*logerror("(p&255=%i p>>8=%i) out= %i\n", p&255,p>>8, tl_tab[p&255]>>(p>>8) );*/

	if (p >= TL_TAB_LEN)
		return 0;
	return tl_tab[p];
}





INLINE unsigned int calc_eg(FM_SLOT *SLOT)
{
	unsigned int out;
	unsigned int swap_flag = 0;


	switch(SLOT->state)
	{
	case EG_ATT:		/* attack phase */
	{
		INT32 step = SLOT->volume;

		SLOT->volume -= SLOT->delta_ar;
		step = (step>>ENV_SH) - (((UINT32)SLOT->volume)>>ENV_SH);	/* number of levels passed since last time */
		if (step > 0)
		{
			INT32 tmp_volume = SLOT->volume + (step<<ENV_SH);	/* adjust by number of levels */
			do
			{
				tmp_volume = tmp_volume - (1<<ENV_SH) - ((tmp_volume>>4) & ~ENV_MASK);
				if (tmp_volume <= MIN_ATT_INDEX)
					break;
				step--;
			}while(step);
			SLOT->volume = tmp_volume;
		}

		if (SLOT->volume <= MIN_ATT_INDEX)
		{
			if (SLOT->volume < 0)
				SLOT->volume = 0;	/* this is not quite correct (checked) */

			SLOT->state = EG_DEC;
		}
	}
	break;


	case EG_DEC:	/* decay phase */

		if (SLOT->SEG&0x08)	/* SSG EG type envelope selected */
		{
				INT32 step = SLOT->volume;
				SLOT->volume += SLOT->delta_dr;
				step = (((UINT32)SLOT->volume)>>ENV_SH) - (step>>ENV_SH);	/* number of levels passed since last time */

				if ( (SLOT->volume += ((step*3)<<ENV_SH)) >= SLOT->sl )
				{
					SLOT->volume = SLOT->sl;	/* this is not quite correct (checked) */
					SLOT->state = EG_SUS;
				}
		}
		else
		{
			if ( (SLOT->volume += SLOT->delta_dr) >= SLOT->sl )
			{
				SLOT->volume = SLOT->sl;	/* this is not quite correct (checked) */
				SLOT->state = EG_SUS;
			}
		}
	break;

	case EG_SUS:	/* sustain phase */

		if (SLOT->SEG&0x08)	/* SSG EG type envelope selected */
		{
			INT32 step = SLOT->volume;
			SLOT->volume += SLOT->delta_sr;
			step = (((UINT32)SLOT->volume)>>ENV_SH) - (step>>ENV_SH);	/* number of levels passed since last time */

			if ( (SLOT->volume += ((step*3)<<ENV_SH)) > MAX_ATT_INDEX )
			{
				SLOT->volume = MAX_ATT_INDEX;

				if (SLOT->SEG&0x01)	/* bit 0 = hold */
				{
					if (SLOT->SEGn&1)	/* have we swapped once ??? */
					{
						/* yes, so do nothing, just hold current level */
					}
					else
						swap_flag = (SLOT->SEG&0x02) | 1 ; /* bit 1 = alternate */

				}
				else
				{
					/* same as KEY-ON operation */

					/* restart of the Phase Generator should be here,
                    	only if AR is not maximum ??? */
					/*SLOT->Cnt = 0;*/

					/* phase -> Attack */
					SLOT->state = EG_ATT;

					swap_flag = (SLOT->SEG&0x02); /* bit 1 = alternate */
				}
			}

		}
		else
		{
			if ( (SLOT->volume += SLOT->delta_sr) > MAX_ATT_INDEX )
			{
				SLOT->volume = MAX_ATT_INDEX;
				/* do not change the EG phase (verified on real chip) */
			}
		};
	break;

	case EG_REL:	/* release phase */
		if ( (SLOT->volume += SLOT->delta_rr) > MAX_ATT_INDEX )
		{
			SLOT->volume = MAX_ATT_INDEX;
			SLOT->state = EG_OFF;
		}
	break;

	}

	out = SLOT->TL + (((unsigned int)SLOT->volume)>>ENV_SH);

	if (SLOT->SEGn&2)	/* negate output (changes come from alternate bit, init comes from attack bit) */
		out ^= 1023;

	SLOT->SEGn ^= swap_flag;


	if(SLOT->ams)
		out += (SLOT->ams*lfo_amd/LFO_RATE);

	return out;
}


/* ---------- calculate one channel ---------- */
INLINE void FM_CALC_CH( FM_CH *CH )
{
	unsigned int eg_out1,eg_out2,eg_out3,eg_out4;  /* envelope output */

	/* Phase Generator */
	pg_in2 = pg_in3 = pg_in4 = 0;

	/* Envelope Generator */
	eg_out1 = calc_eg(&CH->SLOT[SLOT1]);
	eg_out2 = calc_eg(&CH->SLOT[SLOT2]);
	eg_out3 = calc_eg(&CH->SLOT[SLOT3]);
	eg_out4 = calc_eg(&CH->SLOT[SLOT4]);

	/* Connection */
	{
		INT32 out = CH->op1_out[0] + CH->op1_out[1];
		CH->op1_out[0] = CH->op1_out[1];

		if( !CH->connect1 ){
			/* algorithm 5  */
			pg_in2 = pg_in3	= pg_in4 = CH->op1_out[0];
		}else{
			/* other algorithms */
			*CH->connect1 += CH->op1_out[0];
		}

		CH->op1_out[1] = 0;
		if( eg_out1 < ENV_QUIET )	/* SLOT 1 */
			CH->op1_out[1] = op_calc1(CH->SLOT[SLOT1].Cnt, eg_out1, (out<<CH->FB) );
	}

	if( eg_out2 < ENV_QUIET )		/* SLOT 2 */
		*CH->connect2 += op_calc(CH->SLOT[SLOT2].Cnt, eg_out2, pg_in2);

	if( eg_out3 < ENV_QUIET )		/* SLOT 3 */
		*CH->connect3 += op_calc(CH->SLOT[SLOT3].Cnt, eg_out3, pg_in3);

	if( eg_out4 < ENV_QUIET )		/* SLOT 4 */
		*CH->connect4 += op_calc(CH->SLOT[SLOT4].Cnt, eg_out4, pg_in4);


	/* update phase counters AFTER output calculations */
	{
		INT32 pms = lfo_pmd * CH->pms / LFO_RATE;
		if(pms)
		{
			CH->SLOT[SLOT1].Cnt += CH->SLOT[SLOT1].Incr + (INT32)(pms * CH->SLOT[SLOT1].Incr) / PMS_RATE;
			CH->SLOT[SLOT2].Cnt += CH->SLOT[SLOT2].Incr + (INT32)(pms * CH->SLOT[SLOT2].Incr) / PMS_RATE;
			CH->SLOT[SLOT3].Cnt += CH->SLOT[SLOT3].Incr + (INT32)(pms * CH->SLOT[SLOT3].Incr) / PMS_RATE;
			CH->SLOT[SLOT4].Cnt += CH->SLOT[SLOT4].Incr + (INT32)(pms * CH->SLOT[SLOT4].Incr) / PMS_RATE;
		}
		else
		{
			CH->SLOT[SLOT1].Cnt += CH->SLOT[SLOT1].Incr;
			CH->SLOT[SLOT2].Cnt += CH->SLOT[SLOT2].Incr;
			CH->SLOT[SLOT3].Cnt += CH->SLOT[SLOT3].Incr;
			CH->SLOT[SLOT4].Cnt += CH->SLOT[SLOT4].Incr;
		}
	}
}

/* ---------- update phase increment counter of operator ---------- */
INLINE void CALC_FCSLOT(FM_SLOT *SLOT , int fc , int kc )
{
	int ksr;

	/* (frequency) phase increment counter */
	SLOT->Incr = ((fc+SLOT->DT[kc])*SLOT->mul) >> 1;

	ksr = kc >> SLOT->KSR;
	if( SLOT->ksr != ksr )
	{
		SLOT->ksr = ksr;
		/* calculate envelope generator rates */
		if ((SLOT->ARval + ksr) < 32+62)
			SLOT->delta_ar = SLOT->AR[ksr];
		else
			SLOT->delta_ar = MAX_ATT_INDEX+1;
		SLOT->delta_dr = SLOT->DR[ksr];
		SLOT->delta_sr = SLOT->SR[ksr];
		SLOT->delta_rr = SLOT->RR[ksr];
	}
}

/* ---------- update phase increments counters  ---------- */
INLINE void OPN_CALC_FCOUNT(FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		CALC_FCSLOT(&CH->SLOT[SLOT1] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT2] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT3] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT4] , fc , kc );
	}
}

/* ----------- initialize time tables ----------- */
#if 0
static void init_timetables( FM_ST *ST , UINT8 *DTTABLE )
{
	int i,d;
	double rate;

#if 0
	logerror("FM.C: samplerate=%8i chip clock=%8i  freqbase=%f  \n",
			 ST->rate, ST->clock, ST->freqbase );
#endif

	/* DeTune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = ((double)DTTABLE[d*32 + i]) * SIN_LEN  * ST->freqbase  * (1<<FREQ_SH) / ((double)(1<<20));
			ST->DT_TABLE[d][i]   = (INT32) rate;
			ST->DT_TABLE[d+4][i] = (INT32)-rate;
#if 0
			logerror("FM.C: DT [%2i %2i] = %8x  \n", d, i, ST->DT_TABLE[d][i] );
#endif
		}
	}

	/* calculate Envelope Generator rate table */
	for (i=0; i<34; i++)
		ST->eg_tab[i] = 0;						/* infinity */

	for (i=2; i<64; i++)
	{
		rate = ST->freqbase;					/* frequency rate */
		if( i < 60 ) rate *= 1.0+(i&3)*0.25;	/* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
		rate *= 1<< (i>>2);						/* b2-5 : shift bit */
		rate /= 12.0 * 1024.0;
		rate *= (double)(1<<ENV_SH);
		ST->eg_tab[32+i] = rate;
#if 0
		logerror("FM.C: Rate %2i %1i  Decay [real %11.4f ms][emul %11.4f ms][d=%08x]\n",i>>2, i&3,
			( ((double)(ENV_LEN<<ENV_SH)) / rate )                     * (1000.0 / (double)ST->rate),
			( ((double)(ENV_LEN<<ENV_SH)) / (double)ST->eg_tab[32+i] ) * (1000.0 / (double)ST->rate), ST->eg_tab[32+i] );
#endif
	}

	for (i=0; i<32; i++)
	{
		ST->eg_tab[ 32+64+i ] = ST->eg_tab[32+63];
	}
}

/* ---------- reset one channel  ---------- */
static void reset_channel( FM_ST *ST , FM_CH *CH , int chan )
{
	int c,s;

	ST->mode   = 0;	/* normal mode */
	FM_STATUS_RESET(ST,0xff);
	ST->TA     = 0;
	ST->TAC    = 0;
	ST->TB     = 0;
	ST->TBC    = 0;

	for( c = 0 ; c < chan ; c++ )
	{
		CH[c].fc = 0;
		for(s = 0 ; s < 4 ; s++ )
		{
			CH[c].SLOT[s].SEG = 0;
			CH[c].SLOT[s].SEGn = 0;
			CH[c].SLOT[s].state= EG_OFF;
			CH[c].SLOT[s].volume = MAX_ATT_INDEX;
		}
	}
}

/* ---------- initialize generic tables ---------- */

static void init_tables(void)
{
	signed int i,x;
	signed int n;
	double o,m;

	for (x=0; x<TL_RES_LEN; x++)
	{
		m = (1<<16) / pow(2, (x+1) * (ENV_STEP/4.0) / 8.0);
		m = floor(m);

		/* we never reach (1<<16) here due to the (x+1) */
		/* result fits within 16 bits at maximum */

		n = (int)m;		/* 16 bits here */
		n >>= 4;		/* 12 bits here */
		if (n&1)		/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;
						/* 11 bits here (rounded) */
		n <<= 2;		/* 13 bits here (as in real chip) */
		tl_tab[ x*2 + 0 ] = n;
		tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

		for (i=1; i<13; i++)
		{
			tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
			tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
		}
	#if 0
			logerror("tl %04i", x);
			for (i=0; i<13; i++)
				logerror(", [%02i] %4x", i*2, tl_tab[ x*2 /*+1*/ + i*2*TL_RES_LEN ]);
			logerror("\n");
		}
	#endif
	}
	/*logerror("FM.C: TL_TAB_LEN = %i elements (%i bytes)\n",TL_TAB_LEN, (int)sizeof(tl_tab));*/


	for (i=0; i<SIN_LEN; i++)
	{
		/* non-standard sinus */
		m = sin( ((i*2)+1) * PI / SIN_LEN ); /* checked against the real chip */

		/* we never reach zero here due to ((i*2)+1) */

		if (m>0.0)
			o = 8*log(1.0/m)/log(2);	/* convert to 'decibels' */
		else
			o = 8*log(-1.0/m)/log(2);	/* convert to 'decibels' */

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)						/* round to nearest */
			n = (n>>1)+1;
		else
			n = n>>1;

		sin_tab[ i ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("FM.C: sin [%4i]= %4i (tl_tab value=%5i)\n", i, sin_tab[i],tl_tab[sin_tab[i]]);*/
	}

	/*logerror("FM.C: ENV_QUIET= %08x\n",ENV_QUIET );*/

#ifdef SAVE_SAMPLE
	sample[0]=fopen("sampsum.pcm","wb");
#endif
}
#endif
static int FMInitTable( void )
{
	return 1;
}


static void FMCloseTable( void )
{
#if 0
	if( tl_tab ) free( tl_tab );
	tl_tab = 0;
#endif
#ifdef SAVE_SAMPLE
	fclose(sample[0]);
#endif
	return;
}

/* OPN/OPM Mode  Register Write */
INLINE void FMSetMode( FM_ST *ST, int n, int v )
{
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = timer enable b */
	/* b2 = timer enable a */
	/* b1 = load b */
	/* b0 = load a */
	ST->mode = v;

	/* reset Timer b flag */
	if( v & 0x20 )
		FM_STATUS_RESET(ST,0x02);
	/* reset Timer a flag */
	if( v & 0x10 )
		FM_STATUS_RESET(ST,0x01);
	/* load b */
	if( v & 0x02 )
	{
		if( ST->TBC == 0 )
		{
			ST->TBC = ( 256-ST->TB)<<4;
			/* External timer handler */
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,ST->TBC,ST->TimerBase);
		}
	}
	else
	{	/* stop timer b */
		if( ST->TBC != 0 )
		{
			ST->TBC = 0;
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,0,ST->TimerBase);
		}
	}
	/* load a */
	if( v & 0x01 )
	{
		if( ST->TAC == 0 )
		{
			ST->TAC = (1024-ST->TA);
			/* External timer handler */
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,ST->TAC,ST->TimerBase);
		}
	}
	else
	{	/* stop timer a */
		if( ST->TAC != 0 )
		{
			ST->TAC = 0;
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,0,ST->TimerBase);
		}
	}
}

/* Timer A Overflow */
INLINE void TimerAOver(FM_ST *ST)
{
	/* set status (if enabled) */
	if(ST->mode & 0x04) FM_STATUS_SET(ST,0x01);
	/* clear or reload the counter */
	ST->TAC = (1024-ST->TA);
	if (ST->Timer_Handler) (ST->Timer_Handler)(ST->index,0,ST->TAC,ST->TimerBase);
}
/* Timer B Overflow */
INLINE void TimerBOver(FM_ST *ST)
{
	/* set status (if enabled) */
	if(ST->mode & 0x08) FM_STATUS_SET(ST,0x02);
	/* clear or reload the counter */
	ST->TBC = ( 256-ST->TB)<<4;
	if (ST->Timer_Handler) (ST->Timer_Handler)(ST->index,1,ST->TBC,ST->TimerBase);
}
/* CSM Key Controll */
INLINE void CSMKeyControll(FM_CH *CH)
{
	/* all key off */
	/* FM_KEYOFF(CH,SLOT1); */
	/* FM_KEYOFF(CH,SLOT2); */
	/* FM_KEYOFF(CH,SLOT3); */
	/* FM_KEYOFF(CH,SLOT4); */
	/* all key on */
	FM_KEYON(CH,SLOT1);
	FM_KEYON(CH,SLOT2);
	FM_KEYON(CH,SLOT3);
	FM_KEYON(CH,SLOT4);
}

#ifdef _STATE_H
#if 0
/* FM channel save , internal state only */
static void FMsave_state_channel(const char *name,int num,FM_CH *CH,int num_ch)
{
	int slot , ch;
	char state_name[20];
	const char slot_array[4] = { 1 , 3 , 2 , 4 };

	for(ch=0;ch<num_ch;ch++,CH++)
	{
		/* channel */
		sprintf(state_name,"%s.CH%d",name,ch);
		state_save_register_INT32(state_name, num, "feedback" , CH->op1_out , 2);
		state_save_register_UINT32(state_name, num, "phasestep"   , &CH->fc , 1);
		/* slots */
		for(slot=0;slot<4;slot++)
		{
			FM_SLOT *SLOT = &CH->SLOT[slot];

			sprintf(state_name,"%s.CH%d.SLOT%d",name,ch,slot_array[slot]);
			state_save_register_UINT32(state_name, num, "phasecount" , &SLOT->Cnt, 1);
			state_save_register_UINT8 (state_name, num, "state"      , &SLOT->state, 1);
			state_save_register_INT32 (state_name, num, "volume"     , &SLOT->volume, 1);
		}
	}
}
static void FMsave_state_st(const char *state_name,int num,FM_ST *ST)
{
#if FM_BUSY_FLAG_SUPPORT
	state_save_register_double(state_name, num, "BusyExpire", &ST->BusyExpire , 1);
#endif
	state_save_register_UINT8 (state_name, num, "address"   , &ST->address , 1);
	state_save_register_UINT8 (state_name, num, "IRQ"       , &ST->irq     , 1);
	state_save_register_UINT8 (state_name, num, "IRQ MASK"  , &ST->irqmask , 1);
	state_save_register_UINT8 (state_name, num, "status"    , &ST->status  , 1);
	state_save_register_UINT32(state_name, num, "mode"      , &ST->mode    , 1);
	state_save_register_UINT8 (state_name, num, "prescaler" , &ST->prescaler_sel , 1);
	state_save_register_UINT8 (state_name, num, "freq latch", &ST->fn_h , 1);
	state_save_register_int   (state_name, num, "TIMER A"   , &ST->TA   );
	state_save_register_int   (state_name, num, "TIMER Acnt", &ST->TAC  );
	state_save_register_UINT8 (state_name, num, "TIMER B"   , &ST->TB   , 1);
	state_save_register_int   (state_name, num, "TIMER Bcnt", &ST->TBC  );
}
#endif /* _STATE_H */
#endif
#if BUILD_OPN
/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct opn_3slot {
	UINT32  fc[3];			/* fnum3,blk3  : calculated		*/
	UINT8 fn_h;				/* freq3 latch					*/
	UINT8 kcode[3];			/* key code						*/
}FM_3SLOT;

/* OPN/A/B common state */
typedef struct opn_f {
	UINT8 type;				/* chip type					*/
	FM_ST ST;				/* general state				*/
	FM_3SLOT SL3;			/* 3 slot mode state			*/
	FM_CH *P_CH;			/* pointer of CH				*/
	unsigned int PAN[6*2];	/* fm channels output masks (0xffffffff = enable) */

	UINT32 FN_TABLE[2048];	/* fnumber->increment counter	*/
	/* LFO */
	UINT32 LFOCnt;
	UINT32 LFOIncr;
	UINT32 LFO_FREQ[8];		/* LFO FREQ table				*/
} FM_OPN;

/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static const UINT8 OPN_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

//#define LFO_ENT 512
//#define LFO_SH (32-9)
//#define LFO_RATE 0x10000
//#define PMS_RATE 0x400

static int OPNInitTable(void)
{
	int i;

	/* LFO wave table */
	for(i=0; i<LFO_ENT; i++)
	{
		OPN_LFO_wave[i]= i<LFO_ENT/2 ?    i*LFO_RATE/(LFO_ENT/2) :
 	  							(LFO_ENT-i)*LFO_RATE/(LFO_ENT/2);

		/*logerror("FM.C: OPN_LFO_wave[%4i]= %8x\n",i,OPN_LFO_wave[i]);*/
		/* 0, 0x0100, 0x0200, 0x0300 ... 0xff00, 0x10000, 0xff00..0x0100 */
	}

	init_tables();

	return FMInitTable();
}

/* ---------- prescaler set(and make time tables) ---------- */
static void OPNSetPres(FM_OPN *OPN , int pres , int TimerPres, int SSGpres)
{
	int i;

	/* frequency base */
#if 1
	OPN->ST.freqbase = (OPN->ST.rate) ? ((double)OPN->ST.clock / OPN->ST.rate) / pres : 0;
#else
	OPN->ST.rate = (double)OPN->ST.clock / pres;
	OPN->ST.freqbase = 1.0;
#endif

	/* Timer base time */
	OPN->ST.TimerBase = 1.0/((double)OPN->ST.clock / (double)TimerPres);
	/* SSG part  prescaler set */
	if( SSGpres ) SSGClk( OPN->ST.index, OPN->ST.clock * 2 / SSGpres );
	/* make time tables */
	init_timetables( &OPN->ST , OPN_DTTABLE );
	/* calculate fnumber -> increment counter table */
	for( i=0 ; i < 2048 ; i++ )
	{
		/* freq table for octave 7 */
		/* opn phase increment counter = 20bit */
		OPN->FN_TABLE[i] = (UINT32)( (double)i * 64 * OPN->ST.freqbase * (1<<(FREQ_SH-10)) ); /* -10 because chip works with 10.10 fixed point, while we use 16.16 */
#if 0
		logerror("FM.C: FN_TABLE[%4i] = %08x (dec=%8i)\n",
				 i, OPN->FN_TABLE[i]>>6,OPN->FN_TABLE[i]>>6 );
#endif
	}

	/* LFO freq. table */
	{
		/* 3.98Hz,5.56Hz,6.02Hz,6.37Hz,6.88Hz,9.63Hz,48.1Hz,72.2Hz @ 8MHz */
#define FM_LF(Hz) ((double)LFO_ENT*(1<<LFO_SH)*(Hz)/(8000000.0/144))
		static const double freq_table[8] = { FM_LF(3.98),FM_LF(5.56),FM_LF(6.02),FM_LF(6.37),FM_LF(6.88),FM_LF(9.63),FM_LF(48.1),FM_LF(72.2) };
#undef FM_LF
		for(i=0;i<8;i++)
		{
			OPN->LFO_FREQ[i] = (UINT32)(freq_table[i] * OPN->ST.freqbase);
		}
	}

/*	LOG(LOG_INF,("OPN %d set prescaler %d\n",OPN->ST.index,pres));*/
}

/* ---------- write a OPN mode register 0x20-0x2f ---------- */
static void OPNWriteMode(FM_OPN *OPN, int r, int v)
{
	UINT8 c;
	FM_CH *CH;

	switch(r){
	case 0x21:	/* Test */
		break;
	case 0x22:	/* LFO FREQ (YM2608/YM2612) */
		if( OPN->type & TYPE_LFOPAN )
		{
			OPN->LFOIncr = (v&0x08) ? OPN->LFO_FREQ[v&7] : 0;
			cur_chip = NULL;
		}
		break;
	case 0x24:	/* timer A High 8*/
		OPN->ST.TA = (OPN->ST.TA & 0x03)|(((int)v)<<2);
		break;
	case 0x25:	/* timer A Low 2*/
		OPN->ST.TA = (OPN->ST.TA & 0x3fc)|(v&3);
		break;
	case 0x26:	/* timer B */
		OPN->ST.TB = v;
		break;
	case 0x27:	/* mode , timer controll */
		FMSetMode( &(OPN->ST),OPN->ST.index,v );
		break;
	case 0x28:	/* key on / off */
		c = v&0x03;
		if( c == 3 ) break;
		if( (v&0x04) && (OPN->type & TYPE_6CH) ) c+=3;
		CH = OPN->P_CH;
		CH = &CH[c];
		/* csm mode */
		/* if( c == 2 && (OPN->ST.mode & 0x80) ) break; */
		if(v&0x10) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
		if(v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
		if(v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
		if(v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
		break;
	}
}

/* ---------- write a OPN register (0x30-0xff) ---------- */
static void OPNWriteReg(FM_OPN *OPN, int r, int v)
{
	UINT8 c;
	FM_CH *CH;
	FM_SLOT *SLOT;

	/* 0x30 - 0xff */
	if( (c = OPN_CHAN(r)) == 3 ) return; /* 0xX3,0xX7,0xXB,0xXF */

	if( (r >= 0x100) /* && (OPN->type & TYPE_6CH) */ ) c+=3;
		CH = OPN->P_CH;
		CH = &CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);

	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		set_det_mul(&OPN->ST,CH,SLOT,v);
		break;

	case 0x40:	/* TL */
		set_tl(CH,SLOT,v,(c == 2) && (OPN->ST.mode & 0x80) );
		break;

	case 0x50:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v,OPN->ST.eg_tab);
		break;

	case 0x60:	/*     DR */
		/* bit7 = AMS_ON ENABLE(YM2612) */
		set_dr(SLOT,v,OPN->ST.eg_tab);
		if( OPN->type & TYPE_LFOPAN)
		{
			SLOT->amon = (v&0x80) ? ~0: 0;
			SLOT->ams = CH->ams & SLOT->amon;
		}
		break;

	case 0x70:	/*     SR */
		set_sr(SLOT,v,OPN->ST.eg_tab);
		break;

	case 0x80:	/* SL, RR */
		set_sl_rr(SLOT,v,OPN->ST.eg_tab);
		break;

	case 0x90:	/* SSG-EG */

		SLOT->SEG  =  v&0x0f;
		SLOT->SEGn = (v&0x04)>>1; /* bit 1 in SEGn = attack */

		/* SSG-EG envelope shapes :

		E AtAlH
		1 0 0 0  \\\\

		1 0 0 1  \___

		1 0 1 0  \/\/
		          ___
		1 0 1 1  \

		1 1 0 0  ////
		          ___
		1 1 0 1  /

		1 1 1 0  /\/\

		1 1 1 1  /___


		E = SSG-EG enable


		The shapes are generated using Attack, Decay and Sustain phases.

		Each single character in the diagrams above represents this whole
		sequence:

		- when KEY-ON = 1, normal Attack phase is generated (*without* any
		  difference when compared to normal mode),

		- later, when envelope level reaches minimum level (max volume),
		  the EG switches to Decay phase (which works with bigger steps
		  when compared to normal mode - see below),

		- later when envelope level passes the SL level,
		  the EG swithes to Sustain phase (which works with bigger steps
		  when compared to normal mode - see below),

		- finally when envelope level reaches maximum level (min volume),
		  the EG switches to Attack phase again (depends on actual waveform).

		Important is that when switch to Attack phase occurs, the phase counter
		of that operator will be zeroed-out (as in normal KEY-ON) but not always.
		(I havent found the rule for that - perhaps only when the output level is low)

		The difference (when compared to normal Envelope Generator mode) is
		that the resolution in Decay and Sustain phases is 4 times lower;
		this results in only 256 steps instead of normal 1024.
		In other words:
		when SSG-EG is disabled, the step inside of the EG is one,
		when SSG-EG is enabled, the step is four (in Decay and Sustain phases).

		Times between the level changes are the same in both modes.


		Important:
		Decay 1 Level (so called SL) is compared to actual SSG-EG output, so
		it is the same in both SSG and no-SSG modes, with this exception:

		when the SSG-EG is enabled and is generating raising levels
		(when the EG output is inverted) the SL will be found at wrong level !!!
		For example, when SL=02:
			0 -6 = -6dB in non-inverted EG output
			96-6 = -90dB in inverted EG output
		Which means that EG compares its level to SL as usual, and that the
		output is simply inverted afterall.


		The Yamaha's manuals say that AR should be set to 0x1f (max speed).
		That is not necessary, but then EG will be generating Attack phase.

		*/


		break;

	case 0xa0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				UINT32 fn  = (((UINT32)( (OPN->ST.fn_h)&7))<<8) + v;
				UINT8 blk = OPN->ST.fn_h>>3;
				/* keyscale code */
				CH->kcode = (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* phase increment counter */
				CH->fc = OPN->FN_TABLE[fn]>>(7-blk);
				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			OPN->ST.fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			if( r < 0x100)
			{
				UINT32 fn  = (((UINT32)(OPN->SL3.fn_h&7))<<8) + v;
				UINT8 blk = OPN->SL3.fn_h>>3;
				/* keyscale code */
				OPN->SL3.kcode[c]= (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* phase increment counter */
				OPN->SL3.fc[c] = OPN->FN_TABLE[fn]>>(7-blk);
				(OPN->P_CH)[2].SLOT[SLOT1].Incr=-1;
			}
			break;
		case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
			if( r < 0x100)
				OPN->SL3.fn_h = v&0x3f;
			break;
		}
		break;

	case 0xb0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xb0-0xb2 : FB,ALGO */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB   = feedback ? feedback+6 : 0;
				setup_connection( CH, c );
			}
			break;
		case 1:		/* 0xb4-0xb6 : L , R , AMS , PMS (YM2612/YM2610B/YM2610/YM2608) */
			if( OPN->type & TYPE_LFOPAN)
			{

				/* b0-2 PMS */
				/* 0,3.4,6.7,10,14,20,40,80(cent) */
				static const double pmd_table[8]={0,3.4,6.7,10,14,20,40,80};

				/* b4-5 AMS */
				/* 0, 1.4,     5.9,     11.8           (dB) */
				/* 0, 1.40625, 5.90625, 11.90625 (or 11.8125) */
				/* 0, 15,    , 63	  , 127      (or 126)   in internal representation */

				/* bit0,    bit1,   bit2,  bit3, bit4, bit5, bit6, bit7, bit8, bit9 */
				/* 1,       2,      4,     8,    16,   32,   64,   128,  256,  512  (internal representation value)*/
				/* 0.09375, 0.1875, 0.375, 0.75, 1.5,  3,    6,    12,   24,   48   (dB)*/
				static const int amd_table[4]={	(int)( ((0.0    *4)/3)/ENV_STEP),
												(int)( ((1.40625*4)/3)/ENV_STEP),
												(int)( ((5.90625*4)/3)/ENV_STEP),
												(int)(((11.90625*4)/3)/ENV_STEP) };
				/* amd_table simply becomes = { 0, 15, 63, 127 } */

				CH->pms = (INT32)( (1.5/1200.0)*pmd_table[v & 7] * PMS_RATE);

				CH->ams = amd_table[(v>>4) & 0x03];
				CH->SLOT[SLOT1].ams = CH->ams & CH->SLOT[SLOT1].amon;
				CH->SLOT[SLOT2].ams = CH->ams & CH->SLOT[SLOT2].amon;
				CH->SLOT[SLOT3].ams = CH->ams & CH->SLOT[SLOT3].amon;
				CH->SLOT[SLOT4].ams = CH->ams & CH->SLOT[SLOT4].amon;

				/* PAN :  b7 = L, b6 = R */
				OPN->PAN[ c*2   ] = (v & 0x80) ? ~0 : 0;
				OPN->PAN[ c*2+1 ] = (v & 0x40) ? ~0 : 0;

				/* LOG(LOG_INF,("OPN %d,%d : PAN %x %x\n",n,c,OPN->PAN[c*2],OPN->PAN[c*2+1]));*/
			}
			break;
		}
		break;
	}
}

#endif /* BUILD_OPN */

#if BUILD_OPN_PRESCALER
/*
  prescaler circuit (best guess to verified chip behaviour)

               +--------------+  +-sel2-+
               |              +--|in20  |
         +---+ |  +-sel1-+       |      |
M-CLK -+-|1/2|-+--|in10  | +---+ |   out|--INT_CLOCK
       | +---+    |   out|-|1/3|-|in21  |
       +----------|in11  | +---+ +------+
                  +------+

reg.2d : sel2 = in21 (select sel2)
reg.2e : sel1 = in11 (select sel1)
reg.2f : sel1 = in10 , sel2 = in20 (clear selector)
reset  : sel1 = in11 , sel2 = in21 (clear both)

*/
void OPNPrescaler_w(FM_OPN *OPN , int addr, int pre_divider)
{
	static const int opn_pres[4] = { 2*12 , 2*12 , 6*12 , 3*12 };
	static const int ssg_pres[4] = { 1    ,    1 ,    4 ,    2 };
	int sel;

	switch(addr)
	{
	case 0:		/* when reset */
		OPN->ST.prescaler_sel = 2;
		break;
	case 1:		/* when postload */
		break;
	case 0x2d:	/* divider sel : select 1/1 for 1/3line    */
		OPN->ST.prescaler_sel |= 0x02;
		break;
	case 0x2e:	/* divider sel , select 1/3line for output */
		OPN->ST.prescaler_sel |= 0x01;
		break;
	case 0x2f:	/* divider sel , clear both selector to 1/2,1/2 */
		OPN->ST.prescaler_sel = 0;
		break;
	}
	sel = OPN->ST.prescaler_sel & 3;
	/* update prescaler */
	OPNSetPres( OPN,	opn_pres[sel]*pre_divider,
						opn_pres[sel]*pre_divider,
						ssg_pres[sel]*pre_divider );
}
#endif /* BUILD_OPN_PRESCALER */

#if BUILD_YM2203
/*****************************************************************************/
/*		YM2203 local section                                                 */
/*****************************************************************************/

/* here's the virtual YM2203(OPN) */
typedef struct ym2203_f {
#ifdef _STATE_H
	UINT8 REGS[256];		/* registers         */
#endif
	FM_OPN OPN;				/* OPN state         */
	FM_CH CH[3];			/* channel state     */
} YM2203;

static YM2203 *FM2203=NULL;	/* array of YM2203's */
static int YM2203NumChips;	/* number of chips */

/* ---------- update one of chip ----------- */
void YM2203UpdateOne(int num, INT16 *buffer, int length)
{
	YM2203 *F2203 = &(FM2203[num]);
	FM_OPN *OPN =   &(FM2203[num].OPN);
	int i;
	FMSAMPLE *buf = buffer;

	cur_chip = (void *)F2203;
	State = &F2203->OPN.ST;
	cch[0]   = &F2203->CH[0];
	cch[1]   = &F2203->CH[1];
	cch[2]   = &F2203->CH[2];

	/* LFO */
	lfo_amd = lfo_pmd = 0;

	/* frequency counter channel A */
	OPN_CALC_FCOUNT( cch[0] );
	/* frequency counter channel B */
	OPN_CALC_FCOUNT( cch[1] );
	/* frequency counter channel C */
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else OPN_CALC_FCOUNT( cch[2] );

	for (i=0; i < length ; i++)
	{
		/* clear outputs */
		out_fm[0] = 0;
		out_fm[1] = 0;
		out_fm[2] = 0;

		/* calculate FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );

		/* buffering */
		{
			int lt;

			lt = out_fm[0] + out_fm[1] + out_fm[2];

			lt >>= FINAL_SH;

			Limit( lt , MAXOUT, MINOUT );

			#ifdef SAVE_SAMPLE
				SAVE_ALL_CHANNELS
			#endif

			/* buffering */
			buf[i] = lt;
		}

		/* timer A control */
		INTERNAL_TIMER_A( State , cch[2] )
	}
	INTERNAL_TIMER_B(State,length)
}

/* ---------- reset one of chip ---------- */
void YM2203ResetChip(int num)
{
	int i;
	FM_OPN *OPN = &(FM2203[num].OPN);

	/* Reset Prescaler */
	OPNPrescaler_w(OPN, 0 , 1 );
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	FM_BUSY_CLEAR(&OPN->ST);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */
	reset_channel( &OPN->ST , FM2203[num].CH , 3 );
	/* reset OPerator paramater */
	for(i = 0xb2 ; i >= 0x30 ; i-- ) OPNWriteReg(OPN,i,0);
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
}

#ifdef _STATE_H
static void YM2203_postload(void)
{
	int num , r;

	for(num=0;num<YM2203NumChips;num++)
	{
		/* prescaler */
		OPNPrescaler_w(&FM2203[num].OPN,1,1);

		/* SSG registers */
		for(r=0;r<16;r++)
		{
			SSGWrite(num,0,r);
			SSGWrite(num,1,FM2203[num].REGS[r]);
		}

		/* OPN registers */
		/* DT / MULTI , TL , KS / AR , AMON / DR , SR , SL / RR , SSG-EG */
		for(r=0x30;r<0x9e;r++)
			if((r&3) != 3)
				OPNWriteReg(&FM2203[num].OPN,r,FM2203[num].REGS[r]);
		/* FB / CONNECT , L / R / AMS / PMS */
		for(r=0xb0;r<0xb6;r++)
			if((r&3) != 3)
				OPNWriteReg(&FM2203[num].OPN,r,FM2203[num].REGS[r]);

		/* channels */
		/*FM_channel_postload(FM2203[num].CH,3);*/
	}
	cur_chip = NULL;
}

static void YM2203_save_state(void)
{
	int num;
	const char statename[] = "YM2203";

	for(num=0;num<YM2203NumChips;num++)
	{
		state_save_register_UINT8 (statename, num, "regs"   , FM2203[num].REGS   , 256);
		FMsave_state_st(statename,num,&FM2203[num].OPN.ST);
		FMsave_state_channel(statename,num,FM2203[num].CH,3);
		/* 3slots */
		state_save_register_UINT32 (statename, num, "slot3fc" , FM2203[num].OPN.SL3.fc , 3);
		state_save_register_UINT8  (statename, num, "slot3fh" , &FM2203[num].OPN.SL3.fn_h , 1);
		state_save_register_UINT8  (statename, num, "slot3kc" , FM2203[num].OPN.SL3.kcode , 3);
	}
	state_save_register_func_postload(YM2203_postload);
}
#endif /* _STATE_H */

/* ----------  Initialize YM2203 emulator(s) ----------    */
/* 'num' is the number of virtual YM2203's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int YM2203Init(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i;

	if (FM2203) return (-1);	/* duplicate init. */
	cur_chip = NULL;	/* hiro-shi!! */

	YM2203NumChips = num;

	/* allocate ym2203 state space */
	if( (FM2203 = (YM2203 *)malloc(sizeof(YM2203) * YM2203NumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2203,0,sizeof(YM2203) * YM2203NumChips);
	/* allocate total level table (128kb space) */
	if( !OPNInitTable() )
	{
		free( FM2203 );
		return (-1);
	}
	for ( i = 0 ; i < YM2203NumChips; i++ ) {
		FM2203[i].OPN.ST.index = i;
		FM2203[i].OPN.type = TYPE_YM2203;
		FM2203[i].OPN.P_CH = FM2203[i].CH;
		FM2203[i].OPN.ST.clock = clock;
		FM2203[i].OPN.ST.rate = rate;
		/* FM2203[i].OPN.ST.irq = 0; */
		/* FM2203[i].OPN.ST.satus = 0; */
		/* Extend handler */
		FM2203[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2203[i].OPN.ST.IRQ_Handler   = IRQHandler;
		YM2203ResetChip(i);
	}
#ifdef _STATE_H
	YM2203_save_state();
#endif
	return(0);
}

/* ---------- shut down emulator ----------- */
void YM2203Shutdown(void)
{
	if (!FM2203) return;

	FMCloseTable();
	free(FM2203);
	FM2203 = NULL;
}

/* ---------- YM2203 I/O interface ---------- */
int YM2203Write(int n,int a,UINT8 v)
{
	FM_OPN *OPN = &(FM2203[n].OPN);

	if( !(a&1) )
	{	/* address port */
		OPN->ST.address = (v &= 0xff);
		/* Write register to SSG emulator */
		if( v < 16 ) SSGWrite(n,0,v);
		/* prescaler select : 2d,2e,2f  */
		if( v >= 0x2d && v <= 0x2f )
			OPNPrescaler_w(OPN , v , 1);
	}
	else
	{	/* data port */
		int addr = OPN->ST.address;
#ifdef _STATE_H
		FM2203[n].REGS[addr] = v;
#endif
		switch( addr & 0xf0 )
		{
		case 0x00:	/* 0x00-0x0f : SSG section */
			/* Write data to SSG emulator */
			SSGWrite(n,a,v);
			break;
		case 0x20:	/* 0x20-0x2f : Mode section */
			YM2203UpdateReq(n);
			/* write register */
			OPNWriteMode(OPN,addr,v);
			break;
		default:	/* 0x30-0xff : OPN section */
			YM2203UpdateReq(n);
			/* write register */
			OPNWriteReg(OPN,addr,v);
		}
		FM_BUSY_SET(&OPN->ST,1);
	}
	return OPN->ST.irq;
}

UINT8 YM2203Read(int n,int a)
{
	YM2203 *F2203 = &(FM2203[n]);
	int addr = F2203->OPN.ST.address;
	int ret = 0;

	if( !(a&1) )
	{	/* status port */
		ret = FM_STATUS_FLAG(&F2203->OPN.ST);
	}
	else
	{	/* data port (only SSG) */
		if( addr < 16 ) ret = SSGRead(n);
	}
	return ret;
}

int YM2203TimerOver(int n,int c)
{
	YM2203 *F2203 = &(FM2203[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2203->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2203UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2203->OPN.ST) );
		/* CSM mode key,TL control */
		if( F2203->OPN.ST.mode & 0x80 )
		{	/* CSM mode auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->OPN.ST.irq;
}
#endif /* BUILD_YM2203 */



#if (BUILD_YM2608||BUILD_YM2610||BUILD_YM2610B)
/* adpcm type A struct */
typedef struct adpcm_state {
	UINT8		flag;			/* port state				*/
	UINT8		flagMask;		/* arrived flag mask		*/
	UINT8		now_data;		/* current ROM data			*/
	UINT32		now_addr;		/* current ROM address		*/
	UINT32		now_step;
	UINT32		step;
	UINT32		start;			/* sample data start address*/
	UINT32		end;			/* sample data end address	*/
	UINT8		IL;				/* Instrument Level			*/
	INT32		adpcm_acc;		/* accumulator				*/
	INT32		adpcm_step;		/* step						*/
	INT32		adpcm_out;		/* (speedup) hiro-shi!!		*/
	INT8		vol_mul;		/* volume in "0.75dB" steps	*/
	UINT8		vol_shift;		/* volume in "-6dB" steps	*/
	INT32		*pan;			/* &out_adpcm[OPN_xxxx] 	*/
}ADPCM_CH;

/* here's the virtual YM2610 */
typedef struct ym2610_f {
#ifdef _STATE_H
	UINT8		REGS[512];			/* registers			*/
#endif
	FM_OPN		OPN;				/* OPN state			*/
	FM_CH		CH[6];				/* channel state		*/
	int			address1;			/* address register1	*/
/* ADPCM-A unit */
	UINT8		*pcmbuf;			/* pcm rom buffer		*/
	UINT32		pcm_size;			/* size of pcm rom		*/
	UINT8		adpcmTL;			/* adpcmA total level	*/
	ADPCM_CH 	adpcm[6];			/* adpcm channels		*/
	UINT32		adpcmreg[0x30];		/* registers			*/
	UINT8		adpcm_arrivedEndAddress;
	YM_DELTAT 	deltaT;				/* Delta-T ADPCM unit	*/
} YM2610;



/* here is the virtual YM2608 */
typedef YM2610 YM2608;


#endif /* (BUILD_YM2608||BUILD_YM2610||BUILD_YM2610B) */


#if BUILD_ADPCMA

/**** YM2610 ADPCM defines ****/
#define ADPCM_SHIFT    (16)      /* frequency step rate   */
#define ADPCMA_ADDRESS_SHIFT 8   /* adpcm A address shift */

static UINT8 *pcmbufA;
static UINT32 pcmsizeA;


/* Algorithm and tables verified on real YM2610 */

/* usual ADPCM table (16 * 1.1^N) */
static int steps[49] =
{
	 16,  17,   19,   21,   23,   25,   28,
	 31,  34,   37,   41,   45,   50,   55,
	 60,  66,   73,   80,   88,   97,  107,
	118, 130,  143,  157,  173,  190,  209,
	230, 253,  279,  307,  337,  371,  408,
	449, 494,  544,  598,  658,  724,  796,
	876, 963, 1060, 1166, 1282, 1411, 1552
};

/* different from the usual ADPCM table */
static int step_inc[8] = { -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16 };

/* speedup purposes only */
static int jedi_table[ 49*16 ];


static void InitOPNB_ADPCMATable(void){

	int step, nib;

	for (step = 0; step < 49; step++)
	{
		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			int value = (2*(nib & 0x07) + 1) * steps[step] / 8;
			jedi_table[step*16 + nib] = (nib&0x08) ? -value : value;
		}
	}
}

/**** ADPCM A (Non control type) ****/
INLINE void OPNB_ADPCM_CALC_CHA( YM2610 *F2610, ADPCM_CH *ch )
{
	UINT32 step;
	UINT8  data;

	ch->now_step += ch->step;
	if ( ch->now_step >= (1<<ADPCM_SHIFT) )
	{
		step = ch->now_step >> ADPCM_SHIFT;
		ch->now_step &= (1<<ADPCM_SHIFT)-1;
		do{
			/* end check */
			/* 11-06-2001 JB: corrected comparison. Was > instead of == */
			/* YM2610 checks lower 20 bits only, the 4 MSB bits are sample bank */
			/* Here we use 1<<21 to compensate for nibble calculations */

			if (   (ch->now_addr & ((1<<21)-1)) == ((ch->end<<1) & ((1<<21)-1))	   )
			{
				ch->flag = 0;
				F2610->adpcm_arrivedEndAddress |= ch->flagMask;
				return;
			}
#if 0
			if ( ch->now_addr > (pcmsizeA<<1) ) {
				LOG(LOG_WAR,("YM2610: Attempting to play past adpcm rom size!\n" ));
				return;
			}
#endif
			if( ch->now_addr&1 ) data = ch->now_data & 0x0f;
			else
			{
				ch->now_data = *(pcmbufA+(ch->now_addr>>1));
				data = (ch->now_data >> 4)&0x0f;
			}

			ch->now_addr++;

			ch->adpcm_acc += jedi_table[ch->adpcm_step + data];

			/* extend 12-bit signed int */
			if (ch->adpcm_acc & 0x800)
				ch->adpcm_acc |= ~0xfff;
			else
				ch->adpcm_acc &= 0xfff;

			ch->adpcm_step += step_inc[data & 7];
			Limit( ch->adpcm_step, 48*16, 0*16 );

		}while(--step);

		/**** calc pcm * volume data ****/
		ch->adpcm_out = ((ch->adpcm_acc * ch->vol_mul) >> ch->vol_shift) & ~3;	/* multiply, shift and mask out 2 LSB bits */
	}

	/* output for work of output channels (out_adpcm[OPNxxxx])*/
	*(ch->pan) += ch->adpcm_out;
}

/* ADPCM type A */
static void FM_ADPCMAWrite(YM2610 *F2610,int r,int v)
{
	ADPCM_CH *adpcm = F2610->adpcm;
	UINT8 c = r&0x07;

	F2610->adpcmreg[r] = v&0xff; /* stock data */
	switch( r ){
	case 0x00: /* DM,--,C5,C4,C3,C2,C1,C0 */
		/* F2610->port1state = v&0xff; */
		if( !(v&0x80) ){
			/* KEY ON */
			for( c = 0; c < 6; c++ ){
				if( (1<<c)&v ){
					/**** start adpcm ****/
					adpcm[c].step      = (UINT32)((float)(1<<ADPCM_SHIFT)*((float)F2610->OPN.ST.freqbase)/3.0);
					adpcm[c].now_addr  = adpcm[c].start<<1;
					adpcm[c].now_step  = 0;
					adpcm[c].adpcm_acc = 0;
					adpcm[c].adpcm_step= 0;
					adpcm[c].adpcm_out = 0;
					adpcm[c].flag      = 1;
					if(F2610->pcmbuf==NULL){					/* Check ROM Mapped */
						LOG(LOG_WAR,("YM2610: ADPCM-A rom not mapped\n"));
						adpcm[c].flag = 0;
					} else{
						if(adpcm[c].end >= F2610->pcm_size){	/* Check End in Range */
							LOG(LOG_WAR,("YM2610: ADPCM-A end out of range: $%08x\n",adpcm[c].end));
							/*adpcm[c].end = F2610->pcm_size-1;*/ /* JB: DO NOT uncomment this, otherwise you will break the comparison in the ADPCM_CALC_CHA() */
						}
						if(adpcm[c].start >= F2610->pcm_size)	/* Check Start in Range */
						{
							LOG(LOG_WAR,("YM2610: ADPCM-A start out of range: $%08x\n",adpcm[c].start));
							adpcm[c].flag = 0;
						}
					}
				}	/*** (1<<c)&v ***/
			}	/**** for loop ****/
		} else{
			/* KEY OFF */
			for( c = 0; c < 6; c++ ){
				if( (1<<c)&v )  adpcm[c].flag = 0;
			}
		}
		break;
	case 0x01:	/* B0-5 = TL */
		F2610->adpcmTL = (v & 0x3f) ^ 0x3f;
		for( c = 0; c < 6; c++ )
		{
			int volume = F2610->adpcmTL + adpcm[c].IL;

			if ( volume >= 63 )	/* This is correct, 63 = quiet */
			{
				adpcm[c].vol_mul   = 0;
				adpcm[c].vol_shift = 0;
			}
			else
			{
				adpcm[c].vol_mul   = 15 - (volume & 7);		/* so called 0.75 dB */
				adpcm[c].vol_shift =  1 + (volume >> 3);	/* Yamaha engineers used the approximation: each -6 dB is close to divide by two (shift right) */
			}

			/**** calc pcm * volume data ****/
			adpcm[c].adpcm_out = ((adpcm[c].adpcm_acc * adpcm[c].vol_mul) >> adpcm[c].vol_shift) & ~3;	/* multiply, shift and mask out low 2 bits */
		}
		break;
	default:
		c = r&0x07;
		if( c >= 0x06 ) return;
		switch( r&0x38 ){
		case 0x08:	/* B7=L,B6=R, B4-0=IL */
		{
			int volume;

			adpcm[c].IL = (v & 0x1f) ^ 0x1f;

			volume = F2610->adpcmTL + adpcm[c].IL;

			if ( volume >= 63 )	/* This is correct, 63 = quiet */
			{
				adpcm[c].vol_mul   = 0;
				adpcm[c].vol_shift = 0;
			}
			else
			{
				adpcm[c].vol_mul   = 15 - (volume & 7);		/* so called 0.75 dB */
				adpcm[c].vol_shift =  1 + (volume >> 3);	/* Yamaha engineers used the approximation: each -6 dB is close to divide by two (shift right) */
			}

			adpcm[c].pan    = &out_adpcm[(v>>6)&0x03];

			/**** calc pcm * volume data ****/
			adpcm[c].adpcm_out = ((adpcm[c].adpcm_acc * adpcm[c].vol_mul) >> adpcm[c].vol_shift) & ~3;	/* multiply, shift and mask out low 2 bits */
		}
			break;
		case 0x10:
		case 0x18:
			adpcm[c].start  = ( (F2610->adpcmreg[0x18 + c]*0x0100 | F2610->adpcmreg[0x10 + c]) << ADPCMA_ADDRESS_SHIFT);
			break;
		case 0x20:
		case 0x28:
			adpcm[c].end    = ( (F2610->adpcmreg[0x28 + c]*0x0100 | F2610->adpcmreg[0x20 + c]) << ADPCMA_ADDRESS_SHIFT);
			adpcm[c].end   += (1<<ADPCMA_ADDRESS_SHIFT) - 1;
			break;
		}
	}
}

#ifdef _STATE_H
/* FM channel save , internal state only */
static void FMsave_state_adpcma(const char *name,int num,ADPCM_CH *adpcm)
{
	int ch;
	char state_name[20];

	for(ch=0;ch<6;ch++,adpcm++)
	{
		sprintf(state_name,"%s.CH%d",name,ch);

		state_save_register_UINT8 (state_name, num, "flag"    , &adpcm->flag      , 1);
		state_save_register_UINT8 (state_name, num, "data"    , &adpcm->now_data  , 1);
		state_save_register_UINT32(state_name, num, "addr"    , &adpcm->now_addr  , 1);
		state_save_register_UINT32(state_name, num, "step"    , &adpcm->now_step  , 1);
		state_save_register_INT32 (state_name, num, "a_acc"   , &adpcm->adpcm_acc , 1);
		state_save_register_INT32 (state_name, num, "a_step"  , &adpcm->adpcm_step, 1);
		state_save_register_INT32 (state_name, num, "a_out"   , &adpcm->adpcm_out , 1);
	}
}
#endif /* _STATE_H */

#endif /* BUILD_ADPCMA */


#if BUILD_YM2608
/*****************************************************************************/
/*		YM2608 local section                                                 */
/*****************************************************************************/
static YM2608 *FM2608=NULL;	/* array of YM2608's */
static int YM2608NumChips;	/* total chip */

/* YM2608 Rhythm Number */
#define RY_BD  0
#define RY_SD  1
#define RY_TOP 2
#define RY_HH  3
#define RY_TOM 4
#define RY_RIM 5

#if 0
/* Get next pcm data */
INLINE int YM2608ReadADPCM(int n)
{
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* buffer memory */
		/* F2203->OPN.ST.status |= 0x04; */
		return 0;
	}
	else
	{	/* from PCM data register */
		FM_STATUS_SET(F2608->OPN.ST,0x08); /* BRDY = 1 */
		return F2608->ADData;
	}
}

/* Put decoded data */
INLINE void YM2608WriteADPCM(int n,int v)
{
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* for buffer */
		return;
	}
	else
	{	/* for PCM data port */
		F2608->ADData = v;
		FM_STATUS_SET(F2608->OPN.ST,0x08) /* BRDY = 1 */
	}
}
#endif

/* ---------- IRQ flag Controll Write 0x110 ---------- */
INLINE void YM2608IRQFlagWrite(FM_ST *ST,int n,int v)
{
	if( v & 0x80 )
	{	/* Reset IRQ flag */
		FM_STATUS_RESET(ST,0xff);
	}
	else
	{	/* Set IRQ mask */
		/* !!!!!!!!!! pending !!!!!!!!!! */
	}
}

/* ---------- compatible mode & IRQ flag Controll Write 0x29 ---------- */
void YM2608IRQMaskWrite(FM_OPN *OPN,int v)
{
	/* SCH,xx,xxx,EN_ZERO,EN_BRDY,EN_EOS,EN_TB,EN_TA */
	/* extend 3ch. enable/disable */
	if(v&0x80) OPN->type |= TYPE_6CH;
	else       OPN->type &= ~TYPE_6CH;
	/* IRQ MASK */
	FM_IRQMASK_SET(&OPN->ST,v&0x1f);
}

#ifdef YM2608_RHYTHM_PCM
/**** RYTHM (PCM) ****/
INLINE void YM2608_RYTHM( YM2608 *F2608, ADPCM_CH *ch )

{
	UINT32 step;

	ch->now_step += ch->step;
	if ( ch->now_step >= (1<<ADPCM_SHIFT) )
	{
		step = ch->now_step >> ADPCM_SHIFT;
		ch->now_step &= (1<<ADPCM_SHIFT)-1;
		/* end check */
		if ( (ch->now_addr+step) > (ch->end<<1) ) {	/*most likely this comparison is wrong */
			ch->flag = 0;
			F2608->adpcm_arrivedEndAddress |= ch->flagMask;
			return;
		}
		do{
			/* get a next pcm data */
			ch->adpcm_acc = ((short *)pcmbufA)[ch->now_addr];
			ch->now_addr++;
		}while(--step);
		/**** calc pcm * volume data ****/
		ch->adpcm_out = (ch->adpcm_acc * ch->vol_mul ) >> ch->vol_shift;
	}
	/* output for work of output channels (out_adpcm[OPNxxxx])*/
	*(ch->pan) += ch->adpcm_out;
}
#endif /* YM2608_RHYTHM_PCM */

/* ---------- update one of chip ----------- */
void YM2608UpdateOne(int num, INT16 **buffer, int length)
{
	YM2608 *F2608 = &(FM2608[num]);
	FM_OPN *OPN   = &(FM2608[num].OPN);
	YM_DELTAT *DELTAT = &(F2608[num].deltaT);
	int i,j;
	FMSAMPLE  *bufL,*bufR;

	/* setup DELTA-T unit */
	YM_DELTAT_DECODE_PRESET(DELTAT);

	/* set bufer */
	bufL = buffer[0];
	bufR = buffer[1];

	if( (void *)F2608 != cur_chip ){
		cur_chip = (void *)F2608;

		State = &OPN->ST;
		cch[0]   = &F2608->CH[0];
		cch[1]   = &F2608->CH[1];
		cch[2]   = &F2608->CH[2];
		cch[3]   = &F2608->CH[3];
		cch[4]   = &F2608->CH[4];
		cch[5]   = &F2608->CH[5];
		/* setup adpcm rom address */
		pcmbufA  = F2608->pcmbuf;
		pcmsizeA = F2608->pcm_size;

		LFOCnt  = OPN->LFOCnt;
		LFOIncr = OPN->LFOIncr;
		if( !LFOIncr ) lfo_amd = lfo_pmd = 0;
	}
	/* update frequency counter */
	OPN_CALC_FCOUNT( cch[0] );
	OPN_CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else OPN_CALC_FCOUNT( cch[2] );
	OPN_CALC_FCOUNT( cch[3] );
	OPN_CALC_FCOUNT( cch[4] );
	OPN_CALC_FCOUNT( cch[5] );
	/* buffering */
	for(i=0; i < length ; i++)
	{
		/* LFO */
		if( LFOIncr )
		{
			lfo_amd = OPN_LFO_wave[(LFOCnt+=LFOIncr)>>LFO_SH];
			lfo_pmd = lfo_amd-(LFO_RATE/2);
		}

		/* clear output acc. */
		out_adpcm[OUTD_LEFT] = out_adpcm[OUTD_RIGHT]= out_adpcm[OUTD_CENTER] = 0;
		out_delta[OUTD_LEFT] = out_delta[OUTD_RIGHT]= out_delta[OUTD_CENTER] = 0;
		/* clear outputs */
		out_fm[0] = 0;
		out_fm[1] = 0;
		out_fm[2] = 0;
		out_fm[3] = 0;
		out_fm[4] = 0;
		out_fm[5] = 0;

		/* calculate FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		FM_CALC_CH( cch[5] );

		/**** deltaT ADPCM ****/
		if( DELTAT->portstate )
			YM_DELTAT_ADPCM_CALC(DELTAT);

		for( j = 0; j < 6; j++ )
		{
			/**** ADPCM ****/
			if( F2608->adpcm[j].flag )
#ifdef YM2608_RHYTHM_PCM
				YM2608_RYTHM(F2608, &F2608->adpcm[j]);
#else
				OPNB_ADPCM_CALC_CHA( F2608, &F2608->adpcm[j]);
#endif
		}

		/* buffering */
		{
			int lt,rt;

			lt =  out_adpcm[OUTD_LEFT]  + out_adpcm[OUTD_CENTER];
			rt =  out_adpcm[OUTD_RIGHT] + out_adpcm[OUTD_CENTER];
			lt += (out_delta[OUTD_LEFT]  + out_delta[OUTD_CENTER])>>8;
			rt += (out_delta[OUTD_RIGHT] + out_delta[OUTD_CENTER])>>8;

			lt += ((out_fm[0]>>0) & OPN->PAN[0]);	/* we need to find real level on real chip */
			rt += ((out_fm[0]>>0) & OPN->PAN[1]);
			lt += ((out_fm[1]>>0) & OPN->PAN[2]);
			rt += ((out_fm[1]>>0) & OPN->PAN[3]);
			lt += ((out_fm[2]>>0) & OPN->PAN[4]);
			rt += ((out_fm[2]>>0) & OPN->PAN[5]);
			lt += ((out_fm[3]>>0) & OPN->PAN[6]);
			rt += ((out_fm[3]>>0) & OPN->PAN[7]);
			lt += ((out_fm[4]>>0) & OPN->PAN[8]);
			rt += ((out_fm[4]>>0) & OPN->PAN[9]);
			lt += ((out_fm[5]>>0) & OPN->PAN[10]);
			rt += ((out_fm[5]>>0) & OPN->PAN[11]);

			lt >>= FINAL_SH;
			rt >>= FINAL_SH;

			Limit( lt, MAXOUT, MINOUT );
			Limit( rt, MAXOUT, MINOUT );
			/* buffering */
			bufL[i] = lt;
			bufR[i] = rt;
		}

		/* timer A controll */
		INTERNAL_TIMER_A( State , cch[2] )
	}
	INTERNAL_TIMER_B(State,length)
	/* check IRQ for DELTA-T arrived flag */
 	FM_STATUS_SET(State, 0);

	OPN->LFOCnt = LFOCnt;

}
#ifdef _STATE_H
static void YM2608_postload(void)
{
	int num , r;

	for(num=0;num<YM2608NumChips;num++)
	{
		YM2608 *F2608 = &(FM2608[num]);
		/* prescaler */
		OPNPrescaler_w(&F2608->OPN,1,2);
		F2608->deltaT.freqbase = F2608->OPN.ST.freqbase;
		/* IRQ mask / mode */
		YM2608IRQMaskWrite(&F2608->OPN,F2608->REGS[0x29]);
		/* SSG registers */
		for(r=0;r<16;r++)
		{
			SSGWrite(num,0,r);
			SSGWrite(num,1,F2608->REGS[r]);
		}

		/* OPN registers */
		/* DT / MULTI , TL , KS / AR , AMON / DR , SR , SL / RR , SSG-EG */
		for(r=0x30;r<0x9e;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&F2608->OPN,r,F2608->REGS[r]);
				OPNWriteReg(&F2608->OPN,r|0x100,F2608->REGS[r|0x100]);
			}
		/* FB / CONNECT , L / R / AMS / PMS */
		for(r=0xb0;r<0xb6;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&F2608->OPN,r,F2608->REGS[r]);
				OPNWriteReg(&F2608->OPN,r|0x100,F2608->REGS[r|0x100]);
			}
		/* FM channels */
		/*FM_channel_postload(F2608->CH,6);*/
		/* rhythm(ADPCMA) */
		FM_ADPCMAWrite(F2608,1,F2608->REGS[0x111]);
		for( r=0x08 ; r<0x0c ; r++)
			FM_ADPCMAWrite(F2608,r,F2608->REGS[r+0x110]);
		/* Delta-T ADPCM unit */
		YM_DELTAT_postload(&F2608->deltaT , &F2608->REGS[0x100] );
	}
	cur_chip = NULL;
}

static void YM2608_save_state(void)
{
	int num;
	const char statename[] = "YM2608";

	for(num=0;num<YM2608NumChips;num++)
	{
		YM2608 *F2608 = &(FM2608[num]);

		state_save_register_UINT8 (statename, num, "regs"   , F2608->REGS   , 512);
		FMsave_state_st(statename,num,&FM2608[num].OPN.ST);
		FMsave_state_channel(statename,num,FM2608[num].CH,6);
		/* 3slots */
		state_save_register_UINT32(statename, num, "slot3fc" , F2608->OPN.SL3.fc , 3);
		state_save_register_UINT8 (statename, num, "slot3fh" , &F2608->OPN.SL3.fn_h , 1);
		state_save_register_UINT8 (statename, num, "slot3kc" , F2608->OPN.SL3.kcode , 3);
		/* address register1 */
		state_save_register_int (statename, num, "address1" , &F2608->address1);
		/* rythm(ADPCMA) */
		FMsave_state_adpcma(statename,num,F2608->adpcm);
		/* Delta-T ADPCM unit */
		YM_DELTAT_savestate(statename,num,&FM2608[num].deltaT);
	}
	state_save_register_func_postload(YM2608_postload);
}
#endif /* _STATE_H */

/* -------------------------- YM2608(OPNA) ---------------------------------- */
int YM2608Init(int num, int clock, int rate,
               void **pcmrom,int *pcmsize,short *rhythmrom,int *rhythmpos,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i,j;

	if (FM2608) return (-1);	/* duplicate init. */
	cur_chip = NULL;	/* hiro-shi!! */

	YM2608NumChips = num;

	/* allocate extend state space */
	if( (FM2608 = (YM2608 *)malloc(sizeof(YM2608) * YM2608NumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2608,0,sizeof(YM2608) * YM2608NumChips);
	/* allocate total level table (128kb space) */
	if( !OPNInitTable() )
	{
		free( FM2608 );
		return (-1);
	}

	for ( i = 0 ; i < YM2608NumChips; i++ ) {
		FM2608[i].OPN.ST.index = i;
		FM2608[i].OPN.type = TYPE_YM2608;
		FM2608[i].OPN.P_CH = FM2608[i].CH;
		FM2608[i].OPN.ST.clock = clock;
		FM2608[i].OPN.ST.rate = rate;
		/* FM2608[i].OPN.ST.irq = 0; */
		/* FM2608[i].OPN.ST.status = 0; */
		/* Extend handler */
		FM2608[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2608[i].OPN.ST.IRQ_Handler   = IRQHandler;
		/* DELTA-T */
		FM2608[i].deltaT.memory = (UINT8 *)(pcmrom[i]);
		FM2608[i].deltaT.memory_size = pcmsize[i];
		FM2608[i].deltaT.arrivedFlagPtr = &FM2608[i].OPN.ST.status;
		FM2608[i].deltaT.flagMask = 0x04; /* status flag.bit3 */
		/* ADPCM(Rythm) */
		FM2608[i].pcmbuf   = (UINT8 *)rhythmrom;
#ifdef YM2608_RHYTHM_PCM
		/* rhythm sound setup (PCM) */
		for(j=0;j<6;j++)
		{
			/* rhythm sound */
			FM2608[i].adpcm[j].start = rhythmpos[j];
			FM2608[i].adpcm[j].end   = rhythmpos[j+1]-1;
		}
		FM2608[i].pcm_size = rhythmpos[6];
#else
		/* rhythm sound setup (ADPCM) */
		FM2608[i].pcm_size = rhythmsize;
#endif
		YM2608ResetChip(i);
	}
	InitOPNB_ADPCMATable();
#ifdef _STATE_H
	YM2608_save_state();
#endif
	return 0;
}

/* ---------- shut down emulator ----------- */
void YM2608Shutdown()
{
	if (!FM2608) return;

	FMCloseTable();
	free(FM2608);
	FM2608 = NULL;
}

/* ---------- reset one of chips ---------- */
void YM2608ResetChip(int num)
{
	int i;
	YM2608 *F2608 = &(FM2608[num]);
	FM_OPN *OPN   = &(FM2608[num].OPN);
	YM_DELTAT *DELTAT = &(F2608[num].deltaT);

	/* Reset Prescaler */
	OPNPrescaler_w(OPN , 0 , 2);
	F2608->deltaT.freqbase = OPN->ST.freqbase;
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x1f);
	FM_BUSY_CLEAR(&OPN->ST);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	/* extend 3ch. disable */
	/*OPN->type &= (~TYPE_6CH);*/

	reset_channel( &OPN->ST , F2608->CH , 6 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/* reset ADPCM unit */
	/**** ADPCM work initial ****/
	for( i = 0; i < 6; i++ ){		//this was i < 6+1 which must be a bug ???
		F2608->adpcm[i].now_addr  = 0;
		F2608->adpcm[i].now_step  = 0;
		F2608->adpcm[i].step      = 0;
		F2608->adpcm[i].start     = 0;
		F2608->adpcm[i].end       = 0;
		/* F2608->adpcm[i].delta     = 21866; */
		F2608->adpcm[i].vol_mul   = 0;
		F2608->adpcm[i].pan       = &out_adpcm[OUTD_CENTER]; /* default center */
		F2608->adpcm[i].flagMask  = 0; //(i == 6) ? 0x20 : 0;
		F2608->adpcm[i].flag      = 0;
		F2608->adpcm[i].adpcm_acc = 0;
		F2608->adpcm[i].adpcm_step= 0;
		F2608->adpcm[i].adpcm_out = 0;
	}
	F2608->adpcmTL = 0x3f;
	/* F2608->port1state = -1; */
	F2608->adpcm_arrivedEndAddress = 0; /* don't used */

	/* DELTA-T unit */
	DELTAT->freqbase = OPN->ST.freqbase;
	DELTAT->output_pointer = out_delta;
	DELTAT->portshift = 5;		/* allways 5bits shift */ /* ASG */
	DELTAT->output_range = 1<<23;
	YM_DELTAT_ADPCM_Reset(DELTAT,OUTD_CENTER);
}

/* YM2608 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2608Write(int n, int a,UINT8 v)
{
	YM2608 *F2608 = &(FM2608[n]);
	FM_OPN *OPN   = &(FM2608[n].OPN);
	int addr;

	switch(a&3){
	case 0:	/* address port 0 */
		OPN->ST.address = (v &= 0xff);
		/* Write register to SSG emulator */
		if( v < 16 ) SSGWrite(n,0,v);
		/* prescaler selecter : 2d,2e,2f  */
		if( v >= 0x2d && v <= 0x2f )
		{
			OPNPrescaler_w(OPN , v , 2);
			F2608->deltaT.freqbase = OPN->ST.freqbase;
		}
		break;
	case 1:	/* data port 0    */
		addr = OPN->ST.address;
#ifdef _STATE_H
		F2608->REGS[addr] = v;
#endif
		switch(addr & 0xf0)
		{
		case 0x00:	/* SSG section */
			/* Write data to SSG emulator */
			SSGWrite(n,a,v);
			break;
		case 0x10:	/* 0x10-0x1f : Rhythm section */
			YM2608UpdateReq(n);
			FM_ADPCMAWrite(F2608,addr-0x10,v);
			break;
		case 0x20:	/* Mode Register */
			switch(addr)
			{
			case 0x29: /* SCH,xirq mask */
				YM2608IRQMaskWrite(OPN,v);
				break;
			default:
				YM2608UpdateReq(n);
				OPNWriteMode(OPN,addr,v);
			}
			break;
		default:	/* OPN section */
			YM2608UpdateReq(n);
			OPNWriteReg(OPN,addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2608->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2608->address1;
#ifdef _STATE_H
		F2608->REGS[addr+0x100] = v;
#endif
		YM2608UpdateReq(n);
		switch( addr & 0xf0 )
		{
		case 0x00:	/* DELTAT PORT */
			switch( addr )
			{
			case 0x0c:	/* Limit address L */
				/*F2608->ADLimit = (F2608->ADLimit & 0xff00) | v; */
				/*break;*/
			case 0x0d:	/* Limit address H */
				/*F2608->ADLimit = (F2608->ADLimit & 0x00ff) | (v<<8);*/
				/*break;*/
			case 0x0e:	/* DAC data */
				/*break;*/
			case 0x0f:	/* PCM data port */
				/*F2608->ADData = v;*/
				/*FM_STATUS_RESET(F2608->OPN.ST,0x08);*/
				break;
			default:
				/* 0x00-0x0b */
				YM_DELTAT_ADPCM_Write(&F2608->deltaT,addr,v);
			}
			break;
		case 0x10:	/* IRQ Flag controll */
			if( addr == 0x10 )
				YM2608IRQFlagWrite(&(OPN->ST),n,v);
			break;
		default:
			OPNWriteReg(OPN,addr+0x100,v);
		}
	}
	return OPN->ST.irq;
}
UINT8 YM2608Read(int n,int a)
{
	YM2608 *F2608 = &(FM2608[n]);
	int addr = F2608->OPN.ST.address;
	int ret = 0;

	switch( a&3 ){
	case 0:	/* status 0 : YM2203 compatible */
		/* BUSY:x:x:x:x:x:FLAGB:FLAGA */
		if(addr==0xff) ret = 0x00; /* ID code */
		else ret = FM_STATUS_FLAG(&F2608->OPN.ST)&0x83;
		break;
	case 1:	/* status 0 */
		if( addr < 16 ) ret = SSGRead(n);
		break;
	case 2:	/* status 1 : + ADPCM status */
		/* BUSY:x:PCMBUSY:ZERO:BRDY:EOS:FLAGB:FLAGA */
		if(addr==0xff) ret = 0x00; /* ID code */
		else ret = FM_STATUS_FLAG(&F2608->OPN.ST) | (F2608->adpcm[6].flag ? 0x20 : 0);
		break;
	case 3:
		ret = 0;
		break;
	}
	return ret;
}

int YM2608TimerOver(int n,int c)
{
	YM2608 *F2608 = &(FM2608[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2608->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2608UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2608->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2608->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2608->CH[2]) );
		}
	}
	return FM2608->OPN.ST.irq;
}

#endif /* BUILD_YM2608 */


#if BUILD_OPNB
/* -------------------------- YM2610(OPNB) ---------------------------------- */
static YM2610 *FM2610=NULL;	/* array of YM2610's */
static int YM2610NumChips;	/* total chip */

/* ---------- update one of chip (YM2610 FM4: ADPCM-A6: ADPCM-B1) ----------- */
void YM2610UpdateOne(int num, INT16 **buffer, int length)
{
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);
	YM_DELTAT *DELTAT = &(F2610[num].deltaT);
	int i,j;
	FMSAMPLE  *bufL,*bufR;

	/* setup DELTA-T unit */
	YM_DELTAT_DECODE_PRESET(DELTAT);

	/* buffer setup */
	bufL = buffer[0];
	bufR = buffer[1];

	if( (void *)F2610 != cur_chip ){
		cur_chip = (void *)F2610;
		State = &OPN->ST;
		cch[0] = &F2610->CH[1];
		cch[1] = &F2610->CH[2];
		cch[2] = &F2610->CH[4];
		cch[3] = &F2610->CH[5];
		/* setup adpcm rom address */
		pcmbufA  = F2610->pcmbuf;
		pcmsizeA = F2610->pcm_size;

//		LFOCnt  = OPN->LFOCnt;
//		LFOIncr = OPN->LFOIncr;
//		if( !LFOIncr ) lfo_amd = lfo_pmd = 0;
	}
#ifdef YM2610B_WARNING
#define FM_KEY_IS(SLOT) ((SLOT)->key)
#define FM_MSG_YM2610B "YM2610-%d.CH%d is playing,Check whether the type of the chip is YM2610B\n"
	/* Check YM2610B warning message */
	if( FM_KEY_IS(&F2610->CH[0].SLOT[3]) )
		LOG(LOG_WAR,(FM_MSG_YM2610B,num,0));
	if( FM_KEY_IS(&F2610->CH[3].SLOT[3]) )
		LOG(LOG_WAR,(FM_MSG_YM2610B,num,3));
#endif
	/* update frequency counter */
	OPN_CALC_FCOUNT( cch[0] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[1]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[1]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[1]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[1]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[1]->SLOT[SLOT4] , cch[1]->fc , cch[1]->kcode );
		}
	}else OPN_CALC_FCOUNT( cch[1] );
	OPN_CALC_FCOUNT( cch[2] );
	OPN_CALC_FCOUNT( cch[3] );

	/* buffering */
	for(i=0; i < length ; i++)
	{
//		/* LFO */
//		if( LFOIncr )
//		{
//			lfo_amd = OPN_LFO_wave[(LFOCnt+=LFOIncr)>>LFO_SH];
//			lfo_pmd = lfo_amd-(LFO_RATE/2);
//		}

		/* clear output acc. */
		out_adpcm[OUTD_LEFT] = out_adpcm[OUTD_RIGHT]= out_adpcm[OUTD_CENTER] = 0;
		out_delta[OUTD_LEFT] = out_delta[OUTD_RIGHT]= out_delta[OUTD_CENTER] = 0;
		/* clear outputs */
		out_fm[1] = 0;
		out_fm[2] = 0;
		out_fm[4] = 0;
		out_fm[5] = 0;

		/* calculate FM */
		FM_CALC_CH( cch[0] );	/*remapped to 1*/
		FM_CALC_CH( cch[1] );	/*remapped to 2*/
		FM_CALC_CH( cch[2] );	/*remapped to 4*/
		FM_CALC_CH( cch[3] );	/*remapped to 5*/

		/**** deltaT ADPCM ****/
		if( DELTAT->portstate )
			YM_DELTAT_ADPCM_CALC(DELTAT);

		for( j = 0; j < 6; j++ )
		{
			/* ADPCM */
			if( F2610->adpcm[j].flag )
				OPNB_ADPCM_CALC_CHA( F2610, &F2610->adpcm[j]);
		}

		/* buffering */
		{
			int lt,rt;

			lt =  out_adpcm[OUTD_LEFT]  + out_adpcm[OUTD_CENTER];
			rt =  out_adpcm[OUTD_RIGHT] + out_adpcm[OUTD_CENTER];
			lt += (out_delta[OUTD_LEFT]  + out_delta[OUTD_CENTER])>>9;
			rt += (out_delta[OUTD_RIGHT] + out_delta[OUTD_CENTER])>>9;


			lt += ((out_fm[1]>>1) & OPN->PAN[2]);	/* the shift right was verified on real chip */
			rt += ((out_fm[1]>>1) & OPN->PAN[3]);
			lt += ((out_fm[2]>>1) & OPN->PAN[4]);
			rt += ((out_fm[2]>>1) & OPN->PAN[5]);

			lt += ((out_fm[4]>>1) & OPN->PAN[8]);
			rt += ((out_fm[4]>>1) & OPN->PAN[9]);
			lt += ((out_fm[5]>>1) & OPN->PAN[10]);
			rt += ((out_fm[5]>>1) & OPN->PAN[11]);


			lt >>= FINAL_SH;
			rt >>= FINAL_SH;

			Limit( lt, MAXOUT, MINOUT );
			Limit( rt, MAXOUT, MINOUT );

			#ifdef SAVE_SAMPLE
				SAVE_ALL_CHANNELS
			#endif

			/* buffering */
			bufL[i] = lt;
			bufR[i] = rt;
		}

		/* timer A control */
		INTERNAL_TIMER_A( State , cch[1] )
	}
	INTERNAL_TIMER_B(State,length)

//	OPN->LFOCnt = LFOCnt;
}
#endif /* BUILD_OPNB */

#if BUILD_YM2610B
/* ---------- update one of chip (YM2610B FM6: ADPCM-A6: ADPCM-B1) ----------- */
void YM2610BUpdateOne(int num, INT16 **buffer, int length)
{
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);
	YM_DELTAT *DELTAT = &(FM2610[num].deltaT);
	int i,j;
	FMSAMPLE  *bufL,*bufR;

	/* setup DELTA-T unit */
	YM_DELTAT_DECODE_PRESET(DELTAT);
	/* buffer setup */
	bufL = buffer[0];
	bufR = buffer[1];

	if( (void *)F2610 != cur_chip ){
		cur_chip = (void *)F2610;
		State = &OPN->ST;
		cch[0] = &F2610->CH[0];
		cch[1] = &F2610->CH[1];
		cch[2] = &F2610->CH[2];
		cch[3] = &F2610->CH[3];
		cch[4] = &F2610->CH[4];
		cch[5] = &F2610->CH[5];
		/* setup adpcm rom address */
		pcmbufA  = F2610->pcmbuf;
		pcmsizeA = F2610->pcm_size;

		LFOCnt  = OPN->LFOCnt;
		LFOIncr = OPN->LFOIncr;
		if( !LFOIncr ) lfo_amd = lfo_pmd = 0;
	}

	/* update frequency counter */
	OPN_CALC_FCOUNT( cch[0] );
	OPN_CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else OPN_CALC_FCOUNT( cch[2] );
	OPN_CALC_FCOUNT( cch[3] );
	OPN_CALC_FCOUNT( cch[4] );
	OPN_CALC_FCOUNT( cch[5] );

	/* buffering */
    for(i=0; i < length ; i++)
	{
		/* LFO */
		if( LFOIncr )
		{
			lfo_amd = OPN_LFO_wave[(LFOCnt+=LFOIncr)>>LFO_SH];
			lfo_pmd = lfo_amd-(LFO_RATE/2);
		}

		/* clear output acc. */
		out_adpcm[OUTD_LEFT] = out_adpcm[OUTD_RIGHT]= out_adpcm[OUTD_CENTER] = 0;
		out_delta[OUTD_LEFT] = out_delta[OUTD_RIGHT]= out_delta[OUTD_CENTER] = 0;
		/* clear outputs */
		out_fm[0] = 0;
		out_fm[1] = 0;
		out_fm[2] = 0;
		out_fm[3] = 0;
		out_fm[4] = 0;
		out_fm[5] = 0;

		/* calculate FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		FM_CALC_CH( cch[5] );

		/**** deltaT ADPCM ****/
		if( DELTAT->portstate )
			YM_DELTAT_ADPCM_CALC(DELTAT);

		for( j = 0; j < 6; j++ )
		{
			/**** ADPCM ****/
			if( F2610->adpcm[j].flag )
				OPNB_ADPCM_CALC_CHA( F2610, &F2610->adpcm[j]);
		}

		/* buffering */
		{
			int lt,rt;

			lt =  out_adpcm[OUTD_LEFT]  + out_adpcm[OUTD_CENTER];
			rt =  out_adpcm[OUTD_RIGHT] + out_adpcm[OUTD_CENTER];
			lt += (out_delta[OUTD_LEFT]  + out_delta[OUTD_CENTER])>>9;
			rt += (out_delta[OUTD_RIGHT] + out_delta[OUTD_CENTER])>>9;

			lt += ((out_fm[0]>>1) & OPN->PAN[0]);	/* the shift right is verified on YM2610 */
			rt += ((out_fm[0]>>1) & OPN->PAN[1]);
			lt += ((out_fm[1]>>1) & OPN->PAN[2]);
			rt += ((out_fm[1]>>1) & OPN->PAN[3]);
			lt += ((out_fm[2]>>1) & OPN->PAN[4]);
			rt += ((out_fm[2]>>1) & OPN->PAN[5]);
			lt += ((out_fm[3]>>1) & OPN->PAN[6]);
			rt += ((out_fm[3]>>1) & OPN->PAN[7]);
			lt += ((out_fm[4]>>1) & OPN->PAN[8]);
			rt += ((out_fm[4]>>1) & OPN->PAN[9]);
			lt += ((out_fm[5]>>1) & OPN->PAN[10]);
			rt += ((out_fm[5]>>1) & OPN->PAN[11]);


			lt >>= FINAL_SH;
			rt >>= FINAL_SH;

			Limit( lt, MAXOUT, MINOUT );
			Limit( rt, MAXOUT, MINOUT );

			#ifdef SAVE_SAMPLE
				SAVE_ALL_CHANNELS
			#endif

			/* buffering */
			bufL[i] = lt;
			bufR[i] = rt;
		}

		/* timer A control */
		INTERNAL_TIMER_A( State , cch[2] )
	}
	INTERNAL_TIMER_B(State,length)

	OPN->LFOCnt = LFOCnt;
}
#endif /* BUILD_YM2610B */

#if BUILD_OPNB

#ifdef _STATE_H
static void YM2610_postload(void)
{
	int num , r;

	for(num=0;num<YM2610NumChips;num++)
	{
		YM2610 *F2610 = &(FM2610[num]);
		/* SSG registers */
		for(r=0;r<16;r++)
		{
			SSGWrite(num,0,r);
			SSGWrite(num,1,F2610->REGS[r]);
		}

		/* OPN registers */
		/* DT / MULTI , TL , KS / AR , AMON / DR , SR , SL / RR , SSG-EG */
		for(r=0x30;r<0x9e;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&F2610->OPN,r,F2610->REGS[r]);
				OPNWriteReg(&F2610->OPN,r|0x100,F2610->REGS[r|0x100]);
			}
		/* FB / CONNECT , L / R / AMS / PMS */
		for(r=0xb0;r<0xb6;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&F2610->OPN,r,F2610->REGS[r]);
				OPNWriteReg(&F2610->OPN,r|0x100,F2610->REGS[r|0x100]);
			}
		/* FM channels */
		/*FM_channel_postload(F2610->CH,6);*/
		/* rhythm(ADPCMA) */
		FM_ADPCMAWrite(F2610,1,F2610->REGS[0x111]);
		for( r=0x08 ; r<0x0c ; r++)
			FM_ADPCMAWrite(F2610,r,F2610->REGS[r+0x110]);
		/* Delta-T ADPCM unit */
		YM_DELTAT_postload(&F2610->deltaT , &F2610->REGS[0x100] );
	}
	cur_chip = NULL;
}

static void YM2610_save_state(void)
{
	int num;
	const char statename[] = "YM2610";

	for(num=0;num<YM2610NumChips;num++)
	{
		YM2610 *F2610 = &(FM2610[num]);

		state_save_register_UINT8 (statename, num, "regs"   , F2610->REGS   , 512);
		FMsave_state_st(statename,num,&FM2610[num].OPN.ST);
		FMsave_state_channel(statename,num,FM2610[num].CH,6);
		/* 3slots */
		state_save_register_UINT32(statename, num, "slot3fc" , F2610->OPN.SL3.fc , 3);
		state_save_register_UINT8 (statename, num, "slot3fh" , &F2610->OPN.SL3.fn_h , 1);
		state_save_register_UINT8 (statename, num, "slot3kc" , F2610->OPN.SL3.kcode , 3);
		/* address register1 */
		state_save_register_int (statename, num, "address1" , &F2610->address1);
		state_save_register_UINT8 (statename, num, "arrivedFlag", &F2610->adpcm_arrivedEndAddress , 1);
		/* rythm(ADPCMA) */
		FMsave_state_adpcma(statename,num,F2610->adpcm);
		/* Delta-T ADPCM unit */
		YM_DELTAT_savestate(statename,num,&FM2610[num].deltaT);
	}
	state_save_register_func_postload(YM2610_postload);
}
#endif /* _STATE_H */

int YM2610Init(int num, int clock, int rate,
               void **pcmroma,int *pcmsizea,void **pcmromb,int *pcmsizeb,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)

{
	int i;

	if (FM2610) return (-1);	/* duplicate init. */
	cur_chip = NULL;	/* hiro-shi!! */

	YM2610NumChips = num;

	/* allocate extend state space */
	if( (FM2610 = (YM2610 *)malloc(sizeof(YM2610) * YM2610NumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2610,0,sizeof(YM2610) * YM2610NumChips);
	/* allocate total level table (128kb space) */
	if( !OPNInitTable() )
	{
		free( FM2610 );
		return (-1);
	}

	for ( i = 0 ; i < YM2610NumChips; i++ ) {
		YM2610 *F2610 = &(FM2610[i]);
		/* FM */
		F2610->OPN.ST.index = i;
		F2610->OPN.type = TYPE_YM2610;
		F2610->OPN.P_CH = FM2610[i].CH;
		F2610->OPN.ST.clock = clock;
		F2610->OPN.ST.rate = rate;
		/* FM2610[i].OPN.ST.irq = 0; */
		/* FM2610[i].OPN.ST.status = 0; */
		/* Extend handler */
		F2610->OPN.ST.Timer_Handler = TimerHandler;
		F2610->OPN.ST.IRQ_Handler   = IRQHandler;
		/* ADPCM */
		F2610->pcmbuf   = (UINT8 *)(pcmroma[i]);
		F2610->pcm_size = pcmsizea[i];
		/* DELTA-T */
		F2610->deltaT.memory = (UINT8 *)(pcmromb[i]);
		F2610->deltaT.memory_size = pcmsizeb[i];
		F2610->deltaT.arrivedFlagPtr = &F2610->adpcm_arrivedEndAddress;
		/* */
		YM2610ResetChip(i);
	}
	InitOPNB_ADPCMATable();
#ifdef _STATE_H
	YM2610_save_state();
#endif
	return 0;
}

/* ---------- shut down emulator ----------- */
void YM2610Shutdown()
{
	if (!FM2610) return;

	FMCloseTable();
	free(FM2610);
	FM2610 = NULL;
}

/* ---------- reset one of chip ---------- */
void YM2610ResetChip(int num)
{
	int i;
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);
	YM_DELTAT *DELTAT = &(FM2610[num].deltaT);

	/* Reset Prescaler */
	OPNSetPres( OPN, 6*24, 6*24, 4*2); /* OPN 1/6 , SSG 1/4 */
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	FM_BUSY_CLEAR(&OPN->ST);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	reset_channel( &OPN->ST , F2610->CH , 6 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/**** ADPCM work initial ****/
	for( i = 0; i < 6 ; i++ ){			// this was "i < 6+1" which is ... a bug ?
		F2610->adpcm[i].now_addr  = 0;
		F2610->adpcm[i].now_step  = 0;
		F2610->adpcm[i].step      = 0;
		F2610->adpcm[i].start     = 0;
		F2610->adpcm[i].end       = 0;
		/* F2610->adpcm[i].delta     = 21866; */
		F2610->adpcm[i].vol_mul   = 0;
		F2610->adpcm[i].pan       = &out_adpcm[OUTD_CENTER]; /* default center */
		F2610->adpcm[i].flagMask  = 1<<i; //(i == 6) ? 0x80 : (1<<i);
		F2610->adpcm[i].flag      = 0;
		F2610->adpcm[i].adpcm_acc = 0;
		F2610->adpcm[i].adpcm_step= 0;
		F2610->adpcm[i].adpcm_out = 0;
	}
	F2610->adpcmTL = 0x3f;
	/* F2610->port1state = -1; */
	F2610->adpcm_arrivedEndAddress = 0;

	/* DELTA-T unit */
	DELTAT->freqbase = OPN->ST.freqbase;
	DELTAT->output_pointer = out_delta;
	DELTAT->portshift = 8;		/* allways 8bits shift */
	DELTAT->output_range = 1<<23;
	YM_DELTAT_ADPCM_Reset(DELTAT,OUTD_CENTER);
}

/* YM2610 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2610Write(int n, int a,UINT8 v)
{
	YM2610 *F2610 = &(FM2610[n]);
	FM_OPN *OPN   = &(FM2610[n].OPN);
	int addr;
	int ch;


	switch( a&3 ){
	case 0:	/* address port 0 */
		OPN->ST.address = v & 0xff;
		/* Write register to SSG emulator */
		if( v < 16 ) SSGWrite(n,0,v);
		break;
	case 1:	/* data port 0    */
		addr = OPN->ST.address;
#ifdef _STATE_H
		F2610->REGS[addr] = v;
#endif
		switch(addr & 0xf0)
		{
		case 0x00:	/* SSG section */
			/* Write data to SSG emulator */
			SSGWrite(n,a,v);
			break;
		case 0x10: /* DeltaT ADPCM */
			YM2610UpdateReq(n);
			switch(addr)
			{
			case 0x1c: /*  FLAG CONTROL : Extend Status Clear/Mask */
			{
				UINT8 statusmask = ~v;
				/* set arrived flag mask */
				for(ch=0;ch<6;ch++)
					F2610->adpcm[ch].flagMask = statusmask&(1<<ch);
				F2610->deltaT.flagMask      = statusmask&0x80;
				/* clear arrived flag */
				F2610->adpcm_arrivedEndAddress &= statusmask;
			}
				break;
			default:
				/* 0x10-0x1b */
				YM_DELTAT_ADPCM_Write(&F2610->deltaT,addr-0x10,v);
			}
			break;
		case 0x20:	/* Mode Register */
			YM2610UpdateReq(n);
			OPNWriteMode(OPN,addr,v);
			break;
		default:	/* OPN section */
			YM2610UpdateReq(n);
			/* write register */
			OPNWriteReg(OPN,addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2610->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		YM2610UpdateReq(n);
		addr = F2610->address1;
#ifdef _STATE_H
		F2610->REGS[addr|0x100] = v;
#endif
		if( addr < 0x30 )
			/* 100-12f : ADPCM A section */
			FM_ADPCMAWrite(F2610,addr,v);
		else
			OPNWriteReg(OPN,addr|0x100,v);
	}
	return OPN->ST.irq;
}
UINT8 YM2610Read(int n,int a)
{
	YM2610 *F2610 = &(FM2610[n]);
	int addr = F2610->OPN.ST.address;
	UINT8 ret = 0;

	switch( a&3){
	case 0:	/* status 0 : YM2203 compatible */
		ret = FM_STATUS_FLAG(&F2610->OPN.ST) & 0x83;
		break;
	case 1:	/* data 0 */
		if( addr < 16 ) ret = SSGRead(n);
		if( addr == 0xff ) ret = 0x01;
		break;
	case 2:	/* status 1 : ADPCM status */
		/* ADPCM STATUS (arrived End Address) */
		/* B,--,A5,A4,A3,A2,A1,A0 */
		/* B     = ADPCM-B(DELTA-T) arrived end address */
		/* A0-A5 = ADPCM-A          arrived end address */
		ret = F2610->adpcm_arrivedEndAddress;
		break;
	case 3:
		ret = 0;
		break;
	}
	return ret;
}

int YM2610TimerOver(int n,int c)
{
	YM2610 *F2610 = &(FM2610[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2610->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2610UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2610->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2610->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2610->CH[2]) );
		}
	}
	return F2610->OPN.ST.irq;
}

#endif /* BUILD_OPNB */


#if BUILD_YM2612
/*******************************************************************************/
/*		YM2612 local section                                                   */
/*******************************************************************************/
/* here's the virtual YM2612 */
typedef struct ym2612_f {
#ifdef _STATE_H
	UINT8 REGS[512];	/* registers         */
#endif
	FM_OPN OPN;			/* OPN state       */
	FM_CH CH[6];		/* channel state */
	int address1;		/* address register1 */
	/* dac output (YM2612) */
	int dacen;
	INT32 dacout;
} YM2612;

static int YM2612NumChips;	/* total chip */
static YM2612 *FM2612=NULL;	/* array of YM2612's */

static int dacen;

/* ---------- update one of chip ----------- */
void YM2612UpdateOne(int num, INT16 **buffer, int length)
{
	YM2612 *F2612 = &(FM2612[num]);
	FM_OPN *OPN   = &(FM2612[num].OPN);
	int i;
	FMSAMPLE  *bufL,*bufR;
	INT32 dacout  = F2612->dacout;

	/* set bufer */
	bufL = buffer[0];
	bufR = buffer[1];

	if( (void *)F2612 != cur_chip ){
		cur_chip = (void *)F2612;

		State = &OPN->ST;
		cch[0]   = &F2612->CH[0];
		cch[1]   = &F2612->CH[1];
		cch[2]   = &F2612->CH[2];
		cch[3]   = &F2612->CH[3];
		cch[4]   = &F2612->CH[4];
		cch[5]   = &F2612->CH[5];
		/* DAC mode */
		dacen = F2612->dacen;

		LFOCnt  = OPN->LFOCnt;
		LFOIncr = OPN->LFOIncr;
		if( !LFOIncr ) lfo_amd = lfo_pmd = 0;
	}
	/* update frequency counter */
	OPN_CALC_FCOUNT( cch[0] );
	OPN_CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else OPN_CALC_FCOUNT( cch[2] );
	OPN_CALC_FCOUNT( cch[3] );
	OPN_CALC_FCOUNT( cch[4] );
	OPN_CALC_FCOUNT( cch[5] );

	/* buffering */
	for(i=0; i < length ; i++)
	{
		/* LFO */
		if( LFOIncr )
		{
			lfo_amd = OPN_LFO_wave[(LFOCnt+=LFOIncr)>>LFO_SH];
			lfo_pmd = lfo_amd-(LFO_RATE/2);
		}

		/* clear outputs */
		out_fm[0] = 0;
		out_fm[1] = 0;
		out_fm[2] = 0;
		out_fm[3] = 0;
		out_fm[4] = 0;
		out_fm[5] = 0;

		/* calculate FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		if( dacen )
			*cch[5]->connect4 += dacout;
		else
			FM_CALC_CH( cch[5] );


		/* buffering */
		{
			int lt,rt;

			lt  = ((out_fm[0]>>0) & OPN->PAN[0]);
			rt  = ((out_fm[0]>>0) & OPN->PAN[1]);
			lt += ((out_fm[1]>>0) & OPN->PAN[2]);
			rt += ((out_fm[1]>>0) & OPN->PAN[3]);
			lt += ((out_fm[2]>>0) & OPN->PAN[4]);
			rt += ((out_fm[2]>>0) & OPN->PAN[5]);
			lt += ((out_fm[3]>>0) & OPN->PAN[6]);
			rt += ((out_fm[3]>>0) & OPN->PAN[7]);
			lt += ((out_fm[4]>>0) & OPN->PAN[8]);
			rt += ((out_fm[4]>>0) & OPN->PAN[9]);
			lt += ((out_fm[5]>>0) & OPN->PAN[10]);
			rt += ((out_fm[5]>>0) & OPN->PAN[11]);


			lt >>= FINAL_SH;
			rt >>= FINAL_SH;

			Limit( lt, MAXOUT, MINOUT );
			Limit( rt, MAXOUT, MINOUT );

			#ifdef SAVE_SAMPLE
				SAVE_ALL_CHANNELS
			#endif

			/* buffering */
			bufL[i] = lt;
			bufR[i] = rt;
		}

		/* timer A control */
		INTERNAL_TIMER_A( State , cch[2] )
	}
	INTERNAL_TIMER_B(State,length)

	OPN->LFOCnt = LFOCnt;
}

#ifdef _STATE_H
static void YM2612_postload(void)
{
	int num , r;

	for(num=0;num<YM2612NumChips;num++)
	{
		/* DAC data & port */
		FM2612[num].dacout = ((int)FM2612[num].REGS[0x2a] - 0x80) << 0;	/* level unknown */
		FM2612[num].dacen  = FM2612[num].REGS[0x2d] & 0x80;
		/* OPN registers */
		/* DT / MULTI , TL , KS / AR , AMON / DR , SR , SL / RR , SSG-EG */
		for(r=0x30;r<0x9e;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&FM2612[num].OPN,r,FM2612[num].REGS[r]);
				OPNWriteReg(&FM2612[num].OPN,r|0x100,FM2612[num].REGS[r|0x100]);
			}
		/* FB / CONNECT , L / R / AMS / PMS */
		for(r=0xb0;r<0xb6;r++)
			if((r&3) != 3)
			{
				OPNWriteReg(&FM2612[num].OPN,r,FM2612[num].REGS[r]);
				OPNWriteReg(&FM2612[num].OPN,r|0x100,FM2612[num].REGS[r|0x100]);
			}
		/* channels */
		/*FM_channel_postload(FM2612[num].CH,6);*/
	}
	cur_chip = NULL;
}

static void YM2612_save_state(void)
{
	int num;
	const char statename[] = "YM2612";

	for(num=0;num<YM2612NumChips;num++)
	{
		state_save_register_UINT8 (statename, num, "regs"   , FM2612[num].REGS   , 512);
		FMsave_state_st(statename,num,&FM2612[num].OPN.ST);
		FMsave_state_channel(statename,num,FM2612[num].CH,6);
		/* 3slots */
		state_save_register_UINT32 (statename, num, "slot3fc" , FM2612[num].OPN.SL3.fc , 3);
		state_save_register_UINT8  (statename, num, "slot3fh" , &FM2612[num].OPN.SL3.fn_h , 1);
		state_save_register_UINT8  (statename, num, "slot3kc" , FM2612[num].OPN.SL3.kcode , 3);
		/* address register1 */
		state_save_register_int (statename, num, "address1" , &FM2612[num].address1);
	}
	state_save_register_func_postload(YM2612_postload);
}
#endif /* _STATE_H */

/* -------------------------- YM2612 ---------------------------------- */
int YM2612Init(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i;

	if (FM2612) return (-1);	/* duplicate init. */
	cur_chip = NULL;	/* hiro-shi!! */

	YM2612NumChips = num;

	/* allocate extend state space */
	if( (FM2612 = (YM2612 *)malloc(sizeof(YM2612) * YM2612NumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2612,0,sizeof(YM2612) * YM2612NumChips);
	/* allocate total level table (128kb space) */
	if( !OPNInitTable() )
	{
		free( FM2612 );
		return (-1);
	}

	for ( i = 0 ; i < YM2612NumChips; i++ ) {
		FM2612[i].OPN.ST.index = i;
		FM2612[i].OPN.type = TYPE_YM2612;
		FM2612[i].OPN.P_CH = FM2612[i].CH;
		FM2612[i].OPN.ST.clock = clock;
		FM2612[i].OPN.ST.rate = rate;
		/* FM2612[i].OPN.ST.irq = 0; */
		/* FM2612[i].OPN.ST.status = 0; */
		/* Extend handler */
		FM2612[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2612[i].OPN.ST.IRQ_Handler   = IRQHandler;
		YM2612ResetChip(i);
	}
#ifdef _STATE_H
	YM2612_save_state();
#endif
	return 0;
}

/* ---------- shut down emulator ----------- */
void YM2612Shutdown()
{
	if (!FM2612) return;

	FMCloseTable();
	free(FM2612);
	FM2612 = NULL;
}

/* ---------- reset one of chip ---------- */
void YM2612ResetChip(int num)
{
	int i;
	YM2612 *F2612 = &(FM2612[num]);
	FM_OPN *OPN   = &(FM2612[num].OPN);

	OPNSetPres( OPN, 6*24, 6*24, 0);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	FM_BUSY_CLEAR(&OPN->ST);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	reset_channel( &OPN->ST , &F2612->CH[0] , 6 );
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/* DAC mode clear */
	F2612->dacen = 0;
	//printf("reset: DAC disabled\n");
}

/* YM2612 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2612Write(int n, int a,UINT8 v)
{
	YM2612 *F2612 = &(FM2612[n]);
	int addr;

	switch( a&3){
	case 0:	/* address port 0 */
		F2612->OPN.ST.address = v & 0xff;
		break;
	case 1:	/* data port 0    */
		addr = F2612->OPN.ST.address;
#ifdef _STATE_H
		F2612->REGS[addr] = v;
#endif
		switch( addr & 0xf0 )
		{
		case 0x20:	/* 0x20-0x2f Mode */
			switch( addr )
			{
			case 0x2a:	/* DAC data (YM2612) */
				YM2612UpdateReq(n);
				F2612->dacout = ((int)v - 0x80) << 8;	/* level unknown */
				//printf("write dacout %i",F2612->dacout);
				break;
			case 0x2b:	/* DAC Sel  (YM2612) */
				/* b7 = dac enable */
				F2612->dacen = v & 0x80;
				//printf("write dacen %2x",v);
				cur_chip = NULL;
				break;
			default:	/* OPN section */
				YM2612UpdateReq(n);
				/* write register */
				OPNWriteMode(&(F2612->OPN),addr,v);
			}
			break;
		default:	/* 0x30-0xff OPN section */
			YM2612UpdateReq(n);
			/* write register */
			OPNWriteReg(&(F2612->OPN),addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2612->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2612->address1 |0x100;
#ifdef _STATE_H
		F2612->REGS[addr] = v;
#endif
		YM2612UpdateReq(n);
		OPNWriteReg(&(F2612->OPN),addr,v);
		break;
	}
	return F2612->OPN.ST.irq;
}
UINT8 YM2612Read(int n,int a)
{
	YM2612 *F2612 = &(FM2612[n]);

	switch( a&3){
	case 0:	/* status 0 */
		return FM_STATUS_FLAG(&F2612->OPN.ST);
	case 1:
	case 2:
	case 3:
		LOG(LOG_WAR,("YM2612 #%d:A=%d read unmapped area\n"));
		return FM_STATUS_FLAG(&F2612->OPN.ST);
	}
	return 0;
}

int YM2612TimerOver(int n,int c)
{
	YM2612 *F2612 = &(FM2612[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2612->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2612UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2612->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2612->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2612->CH[2]) );
		}
	}
	return F2612->OPN.ST.irq;
}

#endif /* BUILD_YM2612 */


#if BUILD_YM2151
/*******************************************************************************/
/*		YM2151 local section                                                   */
/*******************************************************************************/
/* -------------------------- OPM ---------------------------------- */

#define FREQ_BITS 24		/* frequency turn          */

/* operator output calcrator */
#define OP_OUTN(PG,EG)  NOISE_TABLE[(PG/(0x1000000/SIN_LEN))&(SIN_LEN-1)][EG]


/* here's the virtual YM2151(OPM)  */
typedef struct ym2151_f {
#ifdef _STATE_H
	UINT8 REGS[256];
#endif
	FM_ST ST;					/* general state     */
	FM_CH CH[8];				/* channel state     */
	UINT8 ct;					/* CT0,1             */
	UINT32 NoiseCnt;			/* noise generator   */
	UINT32 NoiseIncr;			/* noise mode enable & step */

	/* LFO */
	UINT32 LFOCnt;
	UINT32 LFOIncr;
	UINT8 pmd;					/* LFO pmd level     */
	UINT8 amd;					/* LFO amd level     */
	INT32 *wavetype;			/* LFO waveform      */
	UINT8 testreg;				/* test register (LFO reset) */
	UINT32 KC_TABLE[8*12*64+950];/* keycode,keyfunction -> count */
	mem_write_handler PortWrite;/*  callback when write CT0/CT1 */
} YM2151;

static YM2151 *FMOPM=NULL;	/* array of YM2151's */
static int YM2151NumChips;	/* total chip */

static INT32 OPM_LFO_waves[LFO_ENT*4];	/* LFO wave tabel    */
static INT32 *OPM_LFO_wave;

/* current chip state */
static UINT32 NoiseCnt , NoiseIncr;

static INT32 *NOISE_TABLE[SIN_LEN];

static const int DT2_TABLE[4]={ /* 4 DT2 values */
/*
 *   DT2 defines offset in cents from base note
 *
 *   The table below defines offset in deltas table...
 *   User's Manual page 22
 *   Values below were calculated using formula:  value = orig.val * 1.5625
 *
 * DT2=0 DT2=1 DT2=2 DT2=3
 * 0     600   781   950
 */
	0,    384,  500,  608
};

static const int KC_TO_SEMITONE[16]={
	/*translate note code KC into more usable number of semitone*/
	0*64, 1*64, 2*64, 3*64,
	3*64, 4*64, 5*64, 6*64,
	6*64, 7*64, 8*64, 9*64,
	9*64,10*64,11*64,12*64
};

/* ---------- frequency counter  ---------- */
INLINE void OPM_CALC_FCOUNT(YM2151 *OPM , FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1)
	{
		int fc = CH->fc;
		int kc = CH->kcode;

		CALC_FCSLOT(&CH->SLOT[SLOT1] , OPM->KC_TABLE[fc + CH->SLOT[SLOT1].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT2] , OPM->KC_TABLE[fc + CH->SLOT[SLOT2].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT3] , OPM->KC_TABLE[fc + CH->SLOT[SLOT3].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT4] , OPM->KC_TABLE[fc + CH->SLOT[SLOT4].DT2] , kc );
	}
}

/* ---------- calculate one of channel7 ---------- */
INLINE void OPM_CALC_CH7( FM_CH *CH )
{
	UINT32 eg_out1,eg_out2,eg_out3,eg_out4;  /*envelope output*/

	/* Phase Generator */
	INT32 pms = lfo_pmd * CH->pms / LFO_RATE;
	if(pms)
	{
		pg_in1 = (CH->SLOT[SLOT1].Cnt += CH->SLOT[SLOT1].Incr + (INT32)(pms * CH->SLOT[SLOT1].Incr) / PMS_RATE);
		pg_in2 = (CH->SLOT[SLOT2].Cnt += CH->SLOT[SLOT2].Incr + (INT32)(pms * CH->SLOT[SLOT2].Incr) / PMS_RATE);
		pg_in3 = (CH->SLOT[SLOT3].Cnt += CH->SLOT[SLOT3].Incr + (INT32)(pms * CH->SLOT[SLOT3].Incr) / PMS_RATE);
		pg_in4 = (CH->SLOT[SLOT4].Cnt += CH->SLOT[SLOT4].Incr + (INT32)(pms * CH->SLOT[SLOT4].Incr) / PMS_RATE);
	}
	else
	{
		pg_in1 = (CH->SLOT[SLOT1].Cnt += CH->SLOT[SLOT1].Incr);
		pg_in2 = (CH->SLOT[SLOT2].Cnt += CH->SLOT[SLOT2].Incr);
		pg_in3 = (CH->SLOT[SLOT3].Cnt += CH->SLOT[SLOT3].Incr);
		pg_in4 = (CH->SLOT[SLOT4].Cnt += CH->SLOT[SLOT4].Incr);
	}
	/* Envelope Generator */
	FM_CALC_EG(eg_out1,CH->SLOT[SLOT1]);
	FM_CALC_EG(eg_out2,CH->SLOT[SLOT2]);
	FM_CALC_EG(eg_out3,CH->SLOT[SLOT3]);
	FM_CALC_EG(eg_out4,CH->SLOT[SLOT4]);

	/* connection */
	if( eg_out1 < ENV_QUIET )	/* SLOT 1 */
	{
		if( CH->FB ){
			/* with self feed back */
			pg_in1 += (CH->op1_out[0]+CH->op1_out[1])>>CH->FB;
			CH->op1_out[1] = CH->op1_out[0];
		}
		CH->op1_out[0] = OP_OUT(pg_in1,eg_out1);
		/* output slot1 */
		if( !CH->connect1 )
		{
			/* algorithm 5  */
			pg_in2 += CH->op1_out[0];
			pg_in3 += CH->op1_out[0];
			pg_in4 += CH->op1_out[0];
		}else{
			/* other algorithm */
			*CH->connect1 += CH->op1_out[0];
		}
	}
	if( eg_out2 < ENV_QUIET )	/* SLOT 2 */
		*CH->connect2 += OP_OUT(pg_in2,eg_out2);
	if( eg_out3 < ENV_QUIET )	/* SLOT 3 */
		*CH->connect3 += OP_OUT(pg_in3,eg_out3);
	/* SLOT 4 */
	if(NoiseIncr)
	{
		NoiseCnt += NoiseIncr;
		if( eg_out4 < ENV_QUIET )
			*CH->connect4 += OP_OUTN(NoiseCnt,eg_out4);
	}
	else
	{
		if( eg_out4 < ENV_QUIET )
			*CH->connect4 += OP_OUT(pg_in4,eg_out4);
	}
}

static int OPMInitTable(void)
{
	int i;

	/* NOISE wave table */

	for(i=0;i<SIN_LEN;i++)
	{
		int sign = rand()&1;
		int lev = rand()&0x1fe;
		/*pom = lev ? 20*log10(0x200/lev) : 0;*/   /* decibel */
		/*NOISE_TABLE[i] = &tl_tab[sign + (int)(pom / ENV_STEP)];*/ /* TL_TAB steps */
		NOISE_TABLE[i] = &tl_tab[sign + lev * ENV_LEN/0x200]; /* TL_TAB steps */
	}

	/* LFO wave tables , 4 pattern */
	for(i=0;i<LFO_ENT;i++)
	{
		OPM_LFO_waves[          i]= LFO_RATE * i / LFO_ENT /127;
		OPM_LFO_waves[LFO_ENT  +i]= ( i<LFO_ENT/2 ? 0 : LFO_RATE )/127;
		OPM_LFO_waves[LFO_ENT*2+i]= LFO_RATE* (i<LFO_ENT/2 ? i : LFO_ENT-i) /(LFO_ENT/2) /127;
		OPM_LFO_waves[LFO_ENT*3+i]= LFO_RATE * (rand()&0xff) /256 /127;
	}
	return FMInitTable();
}

/* ---------- prescaler set(and make time tables) ---------- */
static void OPMResetTable( int num )
{
    YM2151 *OPM = &(FMOPM[num]);
	int i;
	double pom;
	double rate;

	if (FMOPM[num].ST.rate)
		rate = (double)(1<<FREQ_BITS) / (3579545.0 / FMOPM[num].ST.clock * FMOPM[num].ST.rate);
	else rate = 1;

	for (i=0; i<8*12*64+950; i++)
	{
		/* This calculation type was used from the Jarek's YM2151 emulator */
		pom = 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so D#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		OPM->KC_TABLE[i] = (UINT32)(pom * rate);
		/*LOG(LOG_WAR,("OPM KC %d = %x\n",i,OPM->KC_TABLE[i]));*/
	}

	/* make time tables */
	init_timetables( &OPM->ST , OPM_DTTABLE );

}

/* ---------- write a register on YM2151 chip number 'n' ---------- */
static void OPMWriteReg(int n, int r, int v)
{
	UINT8 c;
	FM_CH *CH;
	FM_SLOT *SLOT;

    YM2151 *OPM = &(FMOPM[n]);

	c   = OPM_CHAN(r);
	CH  = &OPM->CH[c];
	SLOT= &CH->SLOT[OPM_SLOT(r)];

	switch( r & 0xe0 ){
	case 0x00: /* 0x00-0x1f */
		switch( r ){
		case 0x01:	/* test */
			if( (OPM->testreg&(OPM->testreg^v))&0x02 ) /* fall eggge */
			{	/* reset LFO counter */
				OPM->LFOCnt = 0;
				cur_chip = NULL;
			}
			OPM->testreg = v;
			break;
		case 0x08:	/* key on / off */
			c = v&7;
			/* CSM mode */
			if( OPM->ST.mode & 0x80 ) break;
			CH = &OPM->CH[c];
			if(v&0x08) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
			if(v&0x10) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
			if(v&0x20) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
			if(v&0x40) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
			break;
		case 0x0f:	/* Noise freq (ch7.op4) */
			/* b7 = Noise enable */
			/* b0-4 noise freq  */
			OPM->NoiseIncr = !(v&0x80) ? 0 :
				/* !!!!! unknown noise freqency rate !!!!! */
				(UINT32)((1<<FREQ_BITS) / 65536 * (v&0x1f) * OPM->ST.freqbase);
			cur_chip = NULL;
#if 1
			if( v & 0x80 ){
				LOG(LOG_WAR,("OPM Noise mode selelted\n"));
			}
#endif
			break;
		case 0x10:	/* timer A High 8*/
			OPM->ST.TA = (OPM->ST.TA & 0x03)|(((int)v)<<2);
			break;
		case 0x11:	/* timer A Low 2*/
			OPM->ST.TA = (OPM->ST.TA & 0x3fc)|(v&3);
			break;
		case 0x12:	/* timer B */
			OPM->ST.TB = v;
			break;
		case 0x14:	/* mode , timer controll */
			FMSetMode( &(OPM->ST),n,v );
			break;

		case 0x18:	/* lfreq   */
			/* f = fm * 2^(LFRQ/16) / (4295*10^6) */
			{
				static double drate[16]={
					1.0        ,1.044273782,1.090507733,1.138788635, /*0-3*/
					1.189207115,1.241857812,1.296839555,1.354255547, /*4-7*/
					1.414213562,1.476826146,1.542210825,1.610490332, /*8-11*/
					1.681792831,1.75625216 ,1.834008086,1.915206561};
				double rate = pow(2.0,v/16)*drate[v&0x0f] / 4295000000.0;
				OPM->LFOIncr = (UINT32)((double)LFO_ENT*(1<<LFO_SH) * (OPM->ST.freqbase*64) * rate);
				cur_chip = NULL;
			}
			break;
		case 0x19:	/* PMD/AMD */
			if( v & 0x80 ) OPM->pmd = v & 0x7f;
			else           OPM->amd = v & 0x7f;
			break;

		case 0x1b:	/* CT , W  */
			/* b7 = CT1 */
			/* b6 = CT0 */
			/* b0-2 = wave form(LFO) 0=nokogiri,1=houkei,2=sankaku,3=noise */
			/*if(OPM->ct != v)*/
			{
				OPM->ct = v>>6;
				if( OPM->PortWrite != 0)
					OPM->PortWrite(0, OPM->ct ); /* bit0 = CT0,bit1 = CT1 */
			}

			if( OPM->wavetype != &OPM_LFO_waves[(v&3)*LFO_ENT])
			{
				OPM->wavetype = &OPM_LFO_waves[(v&3)*LFO_ENT];
				cur_chip = NULL;
			}
			break;
		}
		break;
	case 0x20:	/* 20-3f */
		switch( OPM_SLOT(r) ){
		case 0: /* 0x20-0x27 : RL,FB,CON */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB  = feedback ? 8+1 - feedback : 0;
				/* RL order -> LR order */
				CH->PAN = ((v>>7)&1) | ((v>>5)&2);
				setup_connection( CH );
			}
			break;
		case 1: /* 0x28-0x2f : Keycode */
			{
				int blk = (v>>4)&7;
				/* make keyscale code */
				CH->kcode = (v>>2)&0x1f;
				/* make basic increment counter 22bit = 1 cycle */
				CH->fc = (blk * (12*64)) + KC_TO_SEMITONE[v&0x0f] + (CH->fc&0x3f);
				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 2: /* 0x30-0x37 : Keyfunction */
			CH->fc = (CH->fc&~0x3f) + (v>>2);
			CH->SLOT[SLOT1].Incr=-1;
			break;
		case 3: /* 0x38-0x3f : PMS / AMS */
			/* b0-1 AMS */
			/* AMS * 23.90625db @ AMD=127 */
			/*CH->ams = (v & 0x03) * (23.90625/ENV_STEP);*/
			CH->ams = (UINT32)( (23.90625/ENV_STEP) / (1<<(3-(v&3))) );
			CH->SLOT[SLOT1].ams = CH->ams & CH->SLOT[SLOT1].amon;
			CH->SLOT[SLOT2].ams = CH->ams & CH->SLOT[SLOT2].amon;
			CH->SLOT[SLOT3].ams = CH->ams & CH->SLOT[SLOT3].amon;
			CH->SLOT[SLOT4].ams = CH->ams & CH->SLOT[SLOT4].amon;
			/* b4-6 PMS */
			/* 0,5,10,20,50,100,400,700 (cent) @ PMD=127 */
			{
				/* 1 octabe = 1200cent = +100%/-50% */
				/* 100cent  = 1seminote = 6% ?? */
				static const int pmd_table[8] = {0,5,10,20,50,100,400,700};
				CH->pms = (INT32)( (1.5/1200.0)*pmd_table[(v>>4) & 0x07] * PMS_RATE );
			}
			break;
		}
		break;
	case 0x40:	/* DT1,MUL */
		set_det_mul(&OPM->ST,CH,SLOT,v);
		break;
	case 0x60:	/* TL */
		set_tl(CH,SLOT,v,(OPM->ST.mode & 0x80) );
		break;
	case 0x80:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v,OPM->ST.AR_TABLE);
		break;
	case 0xa0:	/* AMS EN,D1R */
		set_dr(SLOT,v,OPM->ST.DR_TABLE);
		/* bit7 = AMS ENABLE */
		SLOT->amon = (v&0x80) ? ~0: 0;
		SLOT->ams = CH->ams & SLOT->amon;
		break;
	case 0xc0:	/* DT2 ,D2R */
		SLOT->DT2  = DT2_TABLE[v>>6];
		CH->SLOT[SLOT1].Incr=-1;
		set_sr(SLOT,v,OPM->ST.DR_TABLE);
		break;
	case 0xe0:	/* D1L, RR */
		set_sl_rr(SLOT,v,OPM->ST.DR_TABLE);
		break;
    }
}

int YM2151Write(int n,int a,UINT8 v)
{
	YM2151 *F2151 = &(FMOPM[n]);

	if( !(a&1) )
	{	/* address port */
		F2151->ST.address = v & 0xff;
	}
	else
	{	/* data port */
		int addr = F2151->ST.address;
#ifdef _STATE_H
		F2151->REGS[addr] = v;
#endif
		YM2151UpdateReq(n);
		/* write register */
		OPMWriteReg(n,addr,v);
		FM_BUSY_SET(&F2151->ST,1);
	}
	return F2151->ST.irq;
}

/* ---------- reset one of chip ---------- */
void OPMResetChip(int num)
{
	int i;
    YM2151 *OPM = &(FMOPM[num]);

	OPMResetTable( num );
	reset_channel( &OPM->ST , &OPM->CH[0] , 8 );
	/* status clear */
	FM_IRQMASK_SET(&OPM->ST,0x03);
	FM_BUSY_CLEAR(&OPM->ST);
	OPMWriteReg(num,0x1b,0x00);
	/* reset OPerator paramater */
	for(i = 0xff ; i >= 0x20 ; i-- ) OPMWriteReg(num,i,0);
}

#ifdef _STATE_H
static void YM2151_postload(void)
{
	int num , r;

	for(num=0;num<YM2151NumChips;num++)
	{
	    YM2151 *F2151 = &(FMOPM[num]);

		OPMWriteReg(num,0x0f,F2151->REGS[0x0f]);	/* noise sel */
		OPMWriteReg(num,0x18,F2151->REGS[0x18]);	/* lfreq     */
		OPMWriteReg(num,0x1b,F2151->REGS[0x1b]);	/* CT , W    */

		for(r=0xff;r>=0x20;r--)
			OPMWriteReg(num,r,F2151->REGS[r]);
		/* channels */
		/*FM_channel_postload(F2151->CH,8);*/
	}
	cur_chip = NULL;
}

static void YM2151_save_state(void)
{
	int num;
	const char statename[] = "YM2151";

	for(num=0;num<YM2151NumChips;num++)
	{
		YM2151 *F2151 = &(FMOPM[num]);

		state_save_register_UINT8 (statename, num, "regs"   , F2151->REGS   , 256);
		FMsave_state_st(statename,num,&F2151->ST);
		FMsave_state_channel(statename,num,F2151->CH,8);

		state_save_register_UINT32 (statename, num, "NoiseCount" , &F2151->NoiseCnt , 1);
		state_save_register_UINT32 (statename, num, "NoiseStep"  , &F2151->NoiseIncr , 1);
		state_save_register_UINT32 (statename, num, "LFOCount" , &F2151->LFOCnt , 1);
		state_save_register_UINT32 (statename, num, "LFOStep"  , &F2151->LFOIncr , 1);
		state_save_register_UINT8  (statename, num, "LFOPMD"   , &F2151->pmd , 1);
		state_save_register_UINT8  (statename, num, "LFOAMD"   , &F2151->amd , 1);
		state_save_register_UINT8  (statename, num, "test"     , &F2151->testreg , 1);
	}
	state_save_register_func_postload(YM2151_postload);
}
#endif /* _STATE_H */

/* ----------  Initialize YM2151 emulator(s) ----------    */
/* 'num' is the number of virtual YM2151's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPMInit(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
    int i;

    if (FMOPM) return (-1);	/* duplicate init. */
    cur_chip = NULL;	/* hiro-shi!! */

	YM2151NumChips = num;

	/* allocate ym2151 state space */
	if( (FMOPM = (YM2151 *)malloc(sizeof(YM2151) * YM2151NumChips))==NULL)
		return (-1);

	/* clear */
	memset(FMOPM,0,sizeof(YM2151) * YM2151NumChips);

	/* allocate total level table (128kb space) */
	if( !OPMInitTable() )
	{
		free( FMOPM );
		return (-1);
	}
	for ( i = 0 ; i < YM2151NumChips; i++ ) {
		FMOPM[i].ST.index = i;
		FMOPM[i].ST.clock = clock;
		FMOPM[i].ST.rate = rate;
		/* FMOPM[i].ST.irq  = 0; */
		/* FMOPM[i].ST.status = 0; */
		FMOPM[i].ST.freqbase  = rate ? ((double)clock / rate) / 64 : 0;
		FMOPM[i].ST.TimerBase = 1.0/((double)clock / 64.0);
		/* Extend handler */
		FMOPM[i].ST.Timer_Handler = TimerHandler;
		FMOPM[i].ST.IRQ_Handler   = IRQHandler;
		/* Reset callback handler of CT0/1 */
		FMOPM[i].PortWrite = 0;
		OPMResetChip(i);
	}
#ifdef _STATE_H
	YM2151_save_state();
#endif /* _STATE_H */
	return(0);
}

/* ---------- shut down emulator ----------- */
void OPMShutdown()
{
    if (!FMOPM) return;

	FMCloseTable();
	free(FMOPM);
	FMOPM = NULL;
}

UINT8 YM2151Read(int n,int a)
{
	if( !(a&1) ) return 0;
	else         return FM_STATUS_FLAG(&FMOPM[n].ST);
}

/* ---------- make digital sound data ---------- */
void OPMUpdateOne(int num, INT16 **buffer, int length)
{
	YM2151 *OPM = &(FMOPM[num]);
	int i;
	int amd,pmd;
	FM_CH *ch;
	FMSAMPLE  *bufL,*bufR;

	/* set bufer */
	bufL = buffer[0];
	bufR = buffer[1];

	if( (void *)OPM != cur_chip ){
		cur_chip = (void *)OPM;

		State = &OPM->ST;
		/* channel pointer */
		cch[0] = &OPM->CH[0];
		cch[1] = &OPM->CH[1];
		cch[2] = &OPM->CH[2];
		cch[3] = &OPM->CH[3];
		cch[4] = &OPM->CH[4];
		cch[5] = &OPM->CH[5];
		cch[6] = &OPM->CH[6];
		cch[7] = &OPM->CH[7];
		/* ch7.op4 noise mode / step */
		NoiseIncr = OPM->NoiseIncr;
		NoiseCnt  = OPM->NoiseCnt;
		/* LFO */
		LFOCnt  = OPM->LFOCnt;
		/*LFOIncr = OPM->LFOIncr;*/
		if( !LFOIncr ) lfo_amd = lfo_pmd = 0;
		OPM_LFO_wave = OPM->wavetype;
	}
	amd = OPM->amd;
	pmd = OPM->pmd;
	if(amd==0 && pmd==0)
		LFOIncr = 0;
	else
		LFOIncr = OPM->LFOIncr;

	OPM_CALC_FCOUNT( OPM , cch[0] );
	OPM_CALC_FCOUNT( OPM , cch[1] );
	OPM_CALC_FCOUNT( OPM , cch[2] );
	OPM_CALC_FCOUNT( OPM , cch[3] );
	OPM_CALC_FCOUNT( OPM , cch[4] );
	OPM_CALC_FCOUNT( OPM , cch[5] );
	OPM_CALC_FCOUNT( OPM , cch[6] );
	OPM_CALC_FCOUNT( OPM , cch[7] );

	for( i=0; i < length ; i++ )
	{
		/* LFO */
		if( LFOIncr )
		{
			INT32 depth = OPM_LFO_wave[(LFOCnt+=LFOIncr)>>LFO_SH];
			lfo_amd = depth * amd;
			lfo_pmd = (depth-(LFO_RATE/127/2)) * pmd;
		}
		/* clear output acc. */
		out_ch[OUTD_LEFT] = out_ch[OUTD_RIGHT]= out_ch[OUTD_CENTER] = 0;
		/* calculate channel output */
		for(ch = cch[0] ; ch <= cch[6] ; ch++)
			FM_CALC_CH( ch );
		OPM_CALC_CH7( cch[7] );
		/* buffering */
		FM_BUFFERING_STEREO;
		/* timer A controll */
		INTERNAL_TIMER_A( State , cch[7] )
    }
	INTERNAL_TIMER_B(State,length)
	OPM->NoiseCnt = NoiseCnt;
	OPM->LFOCnt = LFOCnt;
}

void OPMSetPortHander(int n,mem_write_handler PortWrite)
{
	FMOPM[n].PortWrite = PortWrite;
}

int YM2151TimerOver(int n,int c)
{
	YM2151 *F2151 = &(FMOPM[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2151->ST) );
	}
	else
	{	/* Timer A */
		YM2151UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2151->ST) );
		/* CSM mode key,TL controll */
		if( F2151->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2151->CH[0]) );
			CSMKeyControll( &(F2151->CH[1]) );
			CSMKeyControll( &(F2151->CH[2]) );
			CSMKeyControll( &(F2151->CH[3]) );
			CSMKeyControll( &(F2151->CH[4]) );
			CSMKeyControll( &(F2151->CH[5]) );
			CSMKeyControll( &(F2151->CH[6]) );
			CSMKeyControll( &(F2151->CH[7]) );
		}
	}
	return F2151->ST.irq;
}

#endif /* BUILD_YM2151 */
