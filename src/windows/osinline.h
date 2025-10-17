//============================================================
//
//	osinline.h - Win32 inline functions
//
//============================================================
#pragma once

#include "osd_cpu.h"

//============================================================
//	MACROS
//============================================================

#if defined(_WIN64) || defined(LIBPINMAME) //!!
#define pdo16		NULL
#define pdt16		NULL
#define pdt16np		NULL
#else
#define pdo16		osd_pdo16
#define pdt16		osd_pdt16
#define pdt16np		osd_pdt16np
#endif


//============================================================
//	PROTOTYPES
//============================================================

void osd_pend(void);
void osd_pdo16( UINT16 *dest, const UINT16 *source, int count, UINT8 *pri, UINT32 pcode );
void osd_pdt16( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );
void osd_pdt16np( UINT16 *dest, const UINT16 *source, const UINT8 *pMask, int mask, int value, int count, UINT8 *pri, UINT32 pcode );
