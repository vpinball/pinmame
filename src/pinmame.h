#ifndef INC_PINMAME
#define INC_PINMAME

#define PINMAME_EXT     1 // PinMAME extensions added to MAME source
#define PINMAME_EXIT    1 // Use Machine->exitfunc (normally only in MESS)
#define WPCDCSSPEEDUP   1 // DCS Speedup added to MAME ADSP emulation
#define DBG_BPR         1 // BPR command added to debugger
#define PINMAME_SAMPLES 1 // Sample Support

#define TINY_COMPILE
#define NEOFREE


#if (MAMEVER >= 6800) && (MAMEVER < 7100)
#  ifndef INVALID_FILE_ATTRIBUTES
#    define INVALID_FILE_ATTRIBUTES 0xffffffff
#  endif
#  ifndef INVALID_SET_FILE_POINTER
#    define INVALID_SET_FILE_POINTER 0xffffffff
#  endif
#endif // MAMEVER
#if (MAMEVER >= 6800)
#  ifndef PI
#    define PI 3.14159265354
#  endif
#else // MAMEVER < 6800
#  define CRC(a) 0x##a
#  define SHA1(a)
#  define NO_DUMP 0x00000000
#endif // MAMEVER < 6800
#if MAMEVER >= 6100
#define osd_mark_dirty(a,b,c,d)
#endif /* MAMEVER */
#if MAMEVER >= 6300
#define VIDEO_SUPPORTS_DIRTY 0
#define FILETYPE_PRINTER FILETYPE_MEMCARD
#endif /* MAMEVER */
#if MAMEVER < 6300
#define FILETYPE_WAVE OSD_FILETYPE_WAVEFILE
#define FILETYPE_HIGHSCORE_DB OSD_FILETYPE_HIGHSCORE_DB
#define FILETYPE_PRINTER OSD_FILETYPE_MEMCARD
#define FILETYPE_ROM OSD_FILETYPE_ROM
#define mame_file void
#define mame_fopen osd_fopen
#define mame_fclose osd_fclose
#define mame_fgets osd_fgets
#define mame_faccess osd_faccess
#define mame_fwrite osd_fwrite
#define mame_fwrite_lsbfirst osd_fwrite_lsbfirst
#endif /* MAMEVER */
#if MAMEVER > 3716
#define BMTYPE UINT16
#define M65C02_INT_IRQ M65C02_IRQ_LINE
#define M65C02_INT_NMI INTERRUPT_NMI
#define VIDEO_MODIFIES_PALETTE 0
#else  /* MAMEVER */
#define BMTYPE UINT8
#define mame_bitmap osd_bitmap
#define cpu_triggerint(x) cpu_trigger(-2000+(x))
#define activecpu_get_reg(x) cpu_get_reg(x)
#define activecpu_set_reg(x,y) cpu_set_reg((x),(y))
#define activecpu_get_previouspc cpu_getpreviouspc
#endif /* MAMEVER */

#ifdef _MSC_VER // These must be in the makefile for WIN32 & DOS
// CPUs
#define HAS_M6809    1
#define HAS_M6808    1
#define HAS_M6800    1
#define HAS_M6803    1
#define HAS_M6802    1
#if MAMEVER >= 6300
#define HAS_ADSP2101 1 // must be defined for 2105 to work
#endif // MAMEVER
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

// Sound
#define HAS_DAC        1
#if MAMEVER > 3716
#define HAS_YM2151_ALT 1
#else /* MAMEVER */
#define HAS_YM2151     1
#endif /* MAMEVER */
#define HAS_HC55516    1
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
#endif /* _MSC_VER */

#ifdef _MSC_VER // Disable some VC++ warnings
// The IDE project manager can't seem to handle "INLINE=static __inline" as
// part of the project settings without screwing up, so the project declares
// "INLINE=__inline", and we overwrite it here with the proper expansion

#ifdef INLINE
#undef INLINE
#endif

#define INLINE static __inline
#pragma warning(disable:4018)		// "signed/unsigned mismatch"
#pragma warning(disable:4146)		// "unary minus operator applied to unsigned type"
#pragma warning(disable:4244)		// "possible loss of data"
#pragma warning(disable:4761)		// "integral size mismatch in argument"
#pragma warning(disable:4550)		// "expression evaluates to a function which is missing an argument list"
#pragma warning(disable:4090)		// "different 'const' qualifiers"
#define M_PI 3.14159265358

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
#endif /* INC_PINMAME */
