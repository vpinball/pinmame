//============================================================
//
//	video.h - Partial Unix implementation of MAME video routines
//
//============================================================

#pragma once

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
