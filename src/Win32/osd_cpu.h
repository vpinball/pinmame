/*******************************************************************************
*                                                                              *
*   Define size independent data types and operations.                         *
*                                                                              *
*   The following types must be supported by all platforms:                    *
*                                                                              *
*   UINT8  - Unsigned 8-bit Integer     INT8  - Signed 8-bit integer           *
*   UINT16 - Unsigned 16-bit Integer    INT16 - Signed 16-bit integer          *
*   UINT32 - Unsigned 32-bit Integer    INT32 - Signed 32-bit integer          *
*   UINT64 - Unsigned 64-bit Integer    INT64 - Signed 64-bit integer          *
*                                                                              *
*                                                                              *
*   The macro names for the artithmatic operations are composed as follows:    *
*                                                                              *
*   XXX_R_A_B, where XXX - 3 letter operation code (ADD, SUB, etc.)            *
*                    R   - The type of the result                              *
*                    A   - The type of operand 1                               *
*                    B   - The type of operand 2 (if binary operation)         *
*                                                                              *
*                    Each type is one of: U8,8,U16,16,U32,32,U64,64            *
*                                                                              *
*******************************************************************************/

#ifndef OSD_CPU_H
#define OSD_CPU_H

#include <stddef.h>

#if defined(__GNUC__)
#include <basetsd.h> /* from w32api */
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef signed char         INT8;
typedef signed short        INT16;
#else
typedef unsigned char       UINT8;
typedef unsigned short      UINT16;
typedef unsigned int        UINT32;
typedef unsigned __int64    UINT64;
typedef signed char         INT8;
typedef signed short        INT16;
typedef signed int          INT32;
typedef signed __int64      INT64;
#endif

/* Combine to 32-bit integers into a 64-bit integer */
#define COMBINE_64_32_32(A,B)     ((((UINT64)(A))<<32) | (UINT32)(B))
#define COMBINE_U64_U32_U32(A,B)  COMBINE_64_32_32(A,B)

/* Return upper 32 bits of a 64-bit integer */
#define HI32_32_64(A)         (((UINT64)(A)) >> 32)
#define HI32_U32_U64(A)       HI32_32_64(A)

/* Return lower 32 bits of a 64-bit integer */
#define LO32_32_64(A)         ((A) & 0xffffffff)
#define LO32_U32_U64(A)       LO32_32_64(A)

#define DIV_64_64_32(A,B)     ((A)/(B))
#define DIV_U64_U64_U32(A,B)  ((A)/(UINT32)(B))

#define MOD_32_64_32(A,B)     ((A)%(B))
#define MOD_U32_U64_U32(A,B)  ((A)%(UINT32)(B))

#define MUL_64_32_32(A,B)     ((A)*(INT64)(B))
#define MUL_U64_U32_U32(A,B)  ((A)*(UINT64)(UINT32)(B))

/***************************** Common types ***********************************/

/******************************************************************************
 * Union of UINT8, UINT16 and UINT32 in native endianess of the target
 * This is used to access bytes and words in a machine independent manner.
 * The upper bytes h2 and h3 normally contain zero (16 bit CPU cores)
 * thus PAIR.d can be used to pass arguments to the memory system
 * which expects 'int' really.
 ******************************************************************************/
typedef union {
#ifdef LSB_FIRST
    struct { UINT8 l,h,h2,h3; } b;
    struct { UINT16 l,h; } w;
#else
    struct { UINT8 h3,h2,h,l; } b;
    struct { UINT16 h,l; } w;
#endif
    UINT32 d;
}   PAIR;

/* Turn off type mismatch warnings */
#pragma warning (disable:4244)
#pragma warning (disable:4761)
#pragma warning (disable:4018)
#pragma warning (disable:4146)
#pragma warning (disable:4068)
#pragma warning (disable:4005)
#pragma warning (disable:4305)

/* Unused variables */
/* #pragma warning (disable:4101) */


#ifndef HAS_CPUS
#define HAS_CPUS

#ifdef  NEOMAME

#define HAS_Z80         1
#define HAS_M68000      1

#else

/* CPUs available */
#ifdef MESS
#define HAS_Z80         1
#define HAS_Z80GB       1
#define HAS_CDP1802     1
#define HAS_8080        0
#define HAS_8085A       0
#define HAS_M6502       1
#define HAS_M65C02      1
#define HAS_M65S02      1
#define HAS_M65CE02     1
#define HAS_M6509       1
#define HAS_M6510       1
#define HAS_M6510T      1
#define HAS_M7501       1
#define HAS_M8502       1
#define HAS_M4510       1
#define HAS_N2A03       1
#define HAS_H6280       1
#define HAS_I86         1
#define HAS_I88         1
#define HAS_I186        1
#define HAS_I188        1
#define HAS_I286        1
#define HAS_I288        1
#define HAS_V20         0
#define HAS_V30         0
#define HAS_V33         0
#define HAS_I8035       0
#define HAS_I8039       1
#define HAS_I8048       1
#define HAS_N7751       0
#define HAS_M6800       1
#define HAS_M6801       0
#define HAS_M6802       0
#define HAS_M6803       1
#define HAS_M6805       0
#define HAS_M6808       1
#define HAS_HD63701     1
#define HAS_NSC8105     1
#define HAS_M68705      0
#define HAS_HD63705     0
#define HAS_HD6309      1
#define HAS_M6309       0
#define HAS_M6809       1
#define HAS_KONAMI      0
#define HAS_M68000      1
#define HAS_M68010      0
#define HAS_M68EC020    0
#define HAS_M68020      0
#define HAS_T11         0
#define HAS_S2650       0
#define HAS_F8          1
#define HAS_CP1600      1
#define HAS_TMS34010    0
#define HAS_TMS9900     1
#define HAS_TMS9940     0
#define HAS_TMS9980     0
#define HAS_TMS9985     0
#define HAS_TMS9989     0
#define HAS_TMS9995     1
#define HAS_TMS99105A   0
#define HAS_TMS99110A   0
#define HAS_Z8000       0
#define HAS_TMS320C10   0
#define HAS_CCPU        0
#define HAS_ADSP2100    0
#define HAS_ADSP2105    0
#define HAS_PDP1        1
#define HAS_PSXCPU      0
#define HAS_SC61860     1
#define HAS_ARM         1
#define HAS_G65816      1
#define HAS_SH2         1
#define HAS_I8X41       0
#define HAS_LH5801      0
#define HAS_SATURN      0
#define HAS_APEXC       0
#define HAS_UPD7810     0
#else
#define HAS_Z80         1
#define HAS_SH2         0
#define HAS_Z80GB       0
#define HAS_CDP1802     0
#define HAS_8080        1
#define HAS_8085A       1
#define HAS_M6502       1
#define HAS_M65C02      1
#define HAS_M65SC02     0
#define HAS_M65CE02     0
#define HAS_M6509       0
#define HAS_M6510       1
#define HAS_M6510T      0
#define HAS_M7501       0
#define HAS_M8502       0
#define HAS_N2A03       1
#define HAS_M4510       0
#define HAS_H6280       1
#define HAS_I86         1
#define HAS_I88         0
#define HAS_I186        1
#define HAS_I188        0
#define HAS_I286        0
#define HAS_V20         1
#define HAS_V30         1
#define HAS_V33         1
#define HAS_I8035       1
#define HAS_I8039       1
#define HAS_I8048       1
#define HAS_N7751       1
#define HAS_M6800       1
#define HAS_M6801       1
#define HAS_M6802       1
#define HAS_M6803       1
#define HAS_M6808       1
#define HAS_HD63701     1
#define HAS_NSC8105     1
#define HAS_M6805       1
#define HAS_M68705      1
#define HAS_HD63705     1
#define HAS_HD6309      1
#define HAS_M6809       1
#define HAS_KONAMI      1
#define HAS_M68000      1
#define HAS_M68010      1
#define HAS_M68EC020    1
#define HAS_M68020      1
#define HAS_T11         1
#define HAS_S2650       1
#define HAS_F8          0
#define HAS_CP1600      0
#define HAS_TMS34010    1
#define HAS_TMS34020    0
#define HAS_TMS9900     0
#define HAS_TMS9940     0
#define HAS_TMS9980     1
#define HAS_TMS9985     0
#define HAS_TMS9989     0
#define HAS_TMS9995     1
#define HAS_TMS99105A   0
#define HAS_TMS99110A   0
#define HAS_Z8000       1
#define HAS_TMS320C10   1
#define HAS_CCPU        1
#define HAS_PDP1        0
#define HAS_ADSP2100    1
#define HAS_ADSP2105    1
#define HAS_PSXCPU      1
#define HAS_SC61860     0
#define HAS_ARM         0
#define HAS_G65816      0
#define HAS_SPC700      0
#define HAS_ASAP        1
#define HAS_SH2         0
#define HAS_I8X41       1
#define HAS_LH5801      0
#define HAS_SATURN      0
#define HAS_APEXC       0
#define HAS_UPD7810     1
#endif

#endif  /* !NEOMAME */
#endif  /* !HAS_CPUS */

#ifndef HAS_SOUND
#define HAS_SOUND

#ifdef  NEOMAME

#define HAS_YM2610      1

#else

/* Sound systems */
#ifdef MESS
#define HAS_CUSTOM      1
#define HAS_SAMPLES     1
#define HAS_DAC         1
#define HAS_AY8910      1
#define HAS_YM2203      0
#define HAS_YM2151      0
#define HAS_YM2151_ALT  1
#define HAS_YM2608      1
#define HAS_YM2610      1
#define HAS_YM2610B     0
#define HAS_YM2612      1
#define HAS_YM3438      0
#define HAS_YM2413      1
#define HAS_YM3812      1
#define HAS_YM3526      0
#define HAS_Y8950       0
#define HAS_SN76477     0
#define HAS_SN76496     1
#define HAS_POKEY       1
#define HAS_TIA         1
#define HAS_NES         1
#define HAS_ASTROCADE   1
#define HAS_NAMCO       0
#define HAS_TMS36XX     0
#define HAS_TMS5110     0
#define HAS_TMS5220     1
#define HAS_VLM5030     0
#define HAS_ADPCM       0
#define HAS_OKIM6295    1
#define HAS_MSM5205     0
#define HAS_UPD7759     0
#define HAS_HC55516     0
#define HAS_K005289     0
#define HAS_K007232     0
#define HAS_K051649     0
#define HAS_K053260     0
#define HAS_SEGAPCM     0
#define HAS_RF5C68      0
#define HAS_CEM3394     0
#define HAS_C140        0
#define HAS_QSOUND      1
#define HAS_SAA1099     1
#define HAS_SPEAKER     1
#define HAS_WAVE        1
#define HAS_BEEP        1
#else
#define HAS_CUSTOM      1
#define HAS_SAMPLES     1
#define HAS_DAC         1
#define HAS_DISCRETE    1
#define HAS_AY8910      1
#define HAS_YM2203      1
#define HAS_YM2151      0
#define HAS_YM2151_ALT  1
#define HAS_YM2608      1
#define HAS_YM2610      1
#define HAS_YM2610B     1
#define HAS_YM2612      1
#define HAS_YM3438      1
#define HAS_YM2413      1
#define HAS_YM3812      1
#define HAS_YM3526      1
#define HAS_YMZ280B     1
#define HAS_Y8950       1
#define HAS_SN76477     1
#define HAS_SN76496     1
#define HAS_POKEY       1
#define HAS_TIA         0
#define HAS_NES         1
#define HAS_ASTROCADE   1
#define HAS_NAMCO       1
#define HAS_TMS36XX     1
#define HAS_TMS5110     1
#define HAS_TMS5220     1
#define HAS_VLM5030     1
#define HAS_ADPCM       1
#define HAS_OKIM6295    1
#define HAS_MSM5205     1
#define HAS_UPD7759     1
#define HAS_HC55516     1
#define HAS_K005289     1
#define HAS_K007232     1
#define HAS_K051649     1
#define HAS_K053260     1
#define HAS_K054539     1
#define HAS_SEGAPCM     1
#define HAS_RF5C68      1
#define HAS_CEM3394     1
#define HAS_C140        1
#define HAS_QSOUND      1
#define HAS_SAA1099     1
#define HAS_IREMGA20    1
#define HAS_ES5505      1
#define HAS_ES5506      1
#define HAS_SPEAKER     0
#define HAS_WAVE        0
#define HAS_BEEP        0
#endif

#endif  /* !NEOMAME */
#endif  /* !HAS_SOUND */

#endif  /* defined OSD_CPU_H */
