#ifndef INC_GEN
#define INC_GEN
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#if defined(__GNUC__)
#define U64(x) __extension__ x##LL
#elif defined(_MSC_VER)
#define U64(x) x##ui64
#endif

/*-- All Hardware Generations --*/
#define GEN_WPCALPHA_1  U64(0x0000000000001) /* Alpha-numeric display S11 sound, Dr Dude 10/90 */
#define GEN_WPCALPHA_2  U64(0x0000000000002) /* Alpha-numeric display,  - The Machine BOP 4/91*/
#define GEN_WPCDMD      U64(0x0000000000004) /* Dot Matrix Display, Terminator 2 7/91 - Party Zone 10/91 */
#define GEN_WPCFLIPTRON U64(0x0000000000008) /* Fliptronic flippers, Addams Family 2/92 - Twilight Zone 5/93 */
#define GEN_WPCDCS      U64(0x0000000000010) /* DCS Sound system, Indiana Jones 10/93 - Popeye 3/94 */
#define GEN_WPCSECURITY U64(0x0000000000020) /* Security chip, World Cup Soccer 3/94 - Jackbot 10/95 */
#define GEN_WPC95DCS    U64(0x0000000000040) /* Hybrid WPC95 driver + DCS sound, Who Dunnit */
#define GEN_WPC95       U64(0x0000000000080) /* Integrated boards, Congo 3/96 - Cactus Canyon 2/99 */
#define GEN_S11         U64(0x0000080000000) /* No external sound board */
#define GEN_S11X        U64(0x0000000000100) /* S11C sound board */
#define GEN_S11A        GEN_S11X
#define GEN_S11B        GEN_S11X
#define GEN_S11B2       U64(0x0000000000200) /* Jokerz! sound board */
#define GEN_S11C        U64(0x0000000000400) /* No CPU board sound */
#define GEN_S9          U64(0x0000000000800) /* S9 CPU, 4x7+1x4 */
#define GEN_DE          U64(0x0000000001000) /* DE AlphaSeg */
#define GEN_DEDMD16     U64(0x0000000002000) /* DE 128x16 */
#define GEN_DEDMD32     U64(0x0000000004000) /* DE 128x32 */
#define GEN_DEDMD64     U64(0x0000000008000) /* DE 192x64 */
#define GEN_S7          U64(0x0000000010000) /* S7 CPU */
#define GEN_S6          U64(0x0000000020000) /* S6 CPU */
#define GEN_S4          U64(0x0000000040000) /* S4 CPU */
#define GEN_S3C         U64(0x0000000080000) /* S3 CPU No Chimes */
#define GEN_S3          U64(0x0000000100000)
#define GEN_BY17        U64(0x0000000200000)
#define GEN_BY35        U64(0x0000000400000)
#define GEN_STMPU100    U64(0x0000000800000) /* Stern MPU - 100*/
#define GEN_STMPU200    U64(0x0000001000000) /* Stern MPU - 200*/
#define GEN_ASTRO       U64(0x0000002000000) /* Black Sheep Squadron, Stern hardware */
#define GEN_HNK	        U64(0x0000004000000) /* Hankin */
#define GEN_BYPROTO     U64(0x0000008000000) /* Bally Bow & Arrow prototype */
#define GEN_BY6803      U64(0x0000010000000)
#define GEN_BY6803A     U64(0x0000020000000)
#define GEN_BOWLING     U64(0x0000040000000) /* Big Ball Bowling, Stern hardware */

#define GEN_GTS1		U64(0x0000100000000) /* GTS1 */
#define GEN_GTS80       U64(0x0000200000000) /* GTS80 */
#define GEN_GTS80A      GEN_GTS80
#define GEN_GTS80B      U64(0x0000400000000) /* GTS80B */
#define GEN_WS          U64(0x0004000000000) /* Whitestar */
#define GEN_WS_1        U64(0x0008000000000) /* Whitestar with extra RAM */
#define GEN_WS_2        U64(0x0010000000000) /* Whitestar with extra DMD */
#define GEN_GTS3        U64(0x0020000000000)

#define GEN_ZAC1        U64(0x0040000000000)
#define GEN_ZAC2        U64(0x0080000000000)

#define GEN_SAM         U64(0x0100000000000) /* Stern SAM */

#define GEN_ALVG        U64(0x0200000000000) /* Alvin G Hardware */
#define GEN_ALVG_DMD2   U64(0x0400000000000) /* Alvin G Hardware, with more shades */
#define GEN_MRGAME		U64(0x0800000000000) /* Mr.Game Hardware */
#define GEN_SLEIC		U64(0x1000000000000) /* Sleic Hardware */
#define GEN_WICO		U64(0x2000000000000) /* Wico Hardware */

#define GEN_ALLWPC      U64(0x00000000000ff)
#define GEN_ALLS11      U64(0x000008000ff00)
#define GEN_ALLBY35     U64(0x0000047e00000)
#define GEN_ALLS80      U64(0x0000600000000)
#define GEN_ALLWS       U64(0x001c000000000)

#endif /* INC_GEN */
