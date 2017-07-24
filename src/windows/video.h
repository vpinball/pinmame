//============================================================
//
//	video.h - Win32 implementation of MAME video routines
//
//============================================================

#ifndef __WIN_VIDEO__
#define __WIN_VIDEO__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


//============================================================
//	PARAMETERS
//============================================================

// maximum video size
#define MAX_VIDEO_WIDTH			1600
#define MAX_VIDEO_HEIGHT		1200



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
extern int			frameskip;
extern int			autoframeskip;
extern int			g_low_latency_throttle;

// speed throttling
extern int			throttle;
extern int			fastfrms;

// palette lookups
extern UINT8		palette_lookups_invalid;
extern UINT32 		palette_16bit_lookup[];
extern UINT32 		palette_32bit_lookup[];

// rotation
extern UINT8		blit_flipx;
extern UINT8		blit_flipy;
extern UINT8		blit_swapxy;



//============================================================
//	PROTOTYPES
//============================================================

void win_orient_rect(struct rectangle *rect);
void win_disorient_rect(struct rectangle *rect);
void throttle_speed_part(int part, int totalparts);
void SetThrottleAdj(int Adj);


#endif
