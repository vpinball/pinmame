/*
 * Microsoft [Visual] C/C++ handling
 */
#ifndef MSC_H
#define MSC_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	/* GCC supports "pragma once" correctly since 3.4 */
#pragma once
#endif

#ifdef _MSC_VER

#if _MSC_VER > 1200
#define HAS_DUMMYUNIONNAME
#endif

/*
 * Handling ISO C99:TC3 conformity for Microsoft C compiler
 *
 * MSC only supports ANSI-C (=ISO C89/C90) and some hand-picked ISO C99 features.
 * Therefore features of ISO C99 not in ISO C90 are named ISO conformant with an underscore.
 *
 * For ISO C99 see http://en.wikipedia.org/wiki/C99 and http://www.iso-9899.info/wiki/The_Standard
 *
 * Note that case-insensitive string comparisions are not in ISO C99:2003 (e.g. stricmp, strnicmp)
 */
#if _MSC_VER < 1400	// missing in MSVC until VS2005
#define vsnprintf _vsnprintf
#endif
#define snprintf _snprintf	// last check: VS2008 = 1500

#endif	/* _MSC_VER */

#endif	/* MSC_H */
