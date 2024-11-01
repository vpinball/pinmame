#ifndef INC_PINMAME
#define INC_PINMAME

#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#define WPCDCSSPEEDUP   1 // DCS Speedup added to MAME ADSP emulation
#define DBG_BPR         1 // BPR command added to debugger

#if !(defined(VPINMAME) || defined(LIBPINMAME) || defined(LISY_SUPPORT))
 #define ENABLE_MECHANICAL_SAMPLES // maybe remove this at some point completely? not wired up for all machines anyway!
#endif

#ifndef PI
 #define PI 3.1415926535897932384626433832795
#endif
#define FILETYPE_PRINTER FILETYPE_MEMCARD
#define BMTYPE UINT16
#define M65C02_INT_IRQ M65C02_IRQ_LINE
#define M65C02_INT_NMI INTERRUPT_NMI
#define VIDEO_MODIFIES_PALETTE 0

#ifdef _MSC_VER // These must be in the makefile for WIN32 & DOS
// CPUs
#define HAS_M6809    1
#define HAS_M6808    1
#define HAS_M6800    1
#define HAS_M6803    1
#define HAS_M6802    1
#define HAS_ADSP2101 1 // must be defined for 2105 to work
#define HAS_ADSP2105 1
#define HAS_Z80      1
#define HAS_M6502    1
#define HAS_M65C02   1
#define HAS_M68000   1
#define HAS_M68306   1
#define HAS_S2650    1
#define HAS_8080     1
#define HAS_8085A	 1
#define HAS_I8035    1
#define HAS_I8039    1
#define HAS_I86      1
#define HAS_I88      1
#define HAS_I186     1
#define HAS_I188     1
#define HAS_4004     1
#define HAS_PPS4     1
#define HAS_SCAMP    1
#define HAS_I8051    1
#define HAS_I8752    1
#define HAS_TMS7000  1
#define HAS_AT91     1
#define HAS_ARM7	 1
#define HAS_CDP1802	 1
#define HAS_TMS9980  1
#define HAS_TMS9995	 1
#define HAS_COP420	 1

// Sound
#define HAS_DAC        1
//#define HAS_YM2151_ALT 1
//#define HAS_YM2151_NUKED 1
#define HAS_YM2151_YMFM 1
#define HAS_HC55516    1
#define HAS_MC3417     1
#define HAS_SAMPLES    1
#define HAS_TMS5220    1
#define HAS_AY8910     1
#define HAS_MSM5205    1
#define HAS_CUSTOM     1
#define HAS_BSMT2000   1
#define HAS_OKIM6295   1
#define HAS_ADPCM      1
#define HAS_VOTRAXSC01 1
#define HAS_SN76477    1
#define HAS_SN76496    1
#define HAS_DISCRETE   1
#define HAS_SP0250     1
#define HAS_TMS320AV120 1
#define HAS_M114S      1
#define HAS_YM3812     1
#define HAS_S14001A    1
#define HAS_YM2203     1
#define HAS_YM3526     1
#define HAS_TMS5110    1
#define HAS_SP0256     1
#define HAS_Y8950      1
#define HAS_ASTROCADE  1
#define HAS_YMF262     1
#define HAS_MEA8000    1
#define HAS_SAA1099    1
#define HAS_QSOUND     1
#endif /* _MSC_VER */

#ifdef _MSC_VER // Disable some VC++ warnings
// The IDE project manager can't seem to handle "INLINE=static __inline" as
// part of the project settings without screwing up, so the project declares
// "INLINE=__inline", and we overwrite it here with the proper expansion

#ifdef INLINE
#undef INLINE
#endif

#if defined(_DEBUG)
 #define INLINE static
#else
 #if defined __LP64__ || defined _WIN64 // at least VC2015s 64bit linker gets stuck
  #define INLINE static __inline
 #else
  #define INLINE static __forceinline
 #endif
#endif
//#pragma warning(disable:4018)		// "signed/unsigned mismatch"
//#pragma warning(disable:4146)		// "unary minus operator applied to unsigned type"
//#pragma warning(disable:4244)		// "possible loss of data"
//#pragma warning(disable:4761)		// "integral size mismatch in argument"
//#pragma warning(disable:4550)		// "expression evaluates to a function which is missing an argument list"
#pragma warning(disable:4090)		// "different 'const' qualifiers"
#define M_PI 3.1415926535897932384626433832795

#ifndef DD_OK
#define DD_OK DS_OK
#endif /* DD_OK */
#endif /* _MSC_VER */
#ifndef CAT3
  #define CAT3(x,y,z) _CAT3(x,y,z)
  #define _CAT3(x,y,z) x##y##z
#endif /* CAT3 */
#ifndef CAT2
  #define CAT2(x,y)   _CAT2(x,y)
  #define _CAT2(x,y)   x##y
#endif /* CAT2 */
#define pia_r(no)     CAT3(pia_,no,_r)
#define pia_w(no)     CAT3(pia_,no,_w)
#define pia_msb_w(no) CAT3(pia_,no,_msb_w)
#define pia_msb_r(no) CAT3(pia_,no,_msb_r)
#define pia_lsb_w(no) CAT3(pia_,no,_lsb_w)
#define pia_lsb_r(no) CAT3(pia_,no,_lsb_r)
#define MWA_BANKNO(no) CAT2(MWA_BANK,no)
#define MRA_BANKNO(no) CAT2(MRA_BANK,no)

#ifdef PX_ZEN
#undef INLINE
#define INLINE static
#endif

#endif /* INC_PINMAME */
