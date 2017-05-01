//============================================================
//
//	video.h - Partial Unix implementation of MAME video routines
//
//============================================================

#ifndef __WIN_VIDEO__
#define __WIN_VIDEO__
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif



//============================================================
//	GLOBAL VARIABLES
//============================================================

// current frameskip/autoframeskip settings
extern int                      frameskip;
extern int			g_low_latency_throttle;

//============================================================
//	PROTOTYPES
//============================================================

void throttle_speed_part(int part, int totalparts);
void SetThrottleAdj(int Adj);


#endif
