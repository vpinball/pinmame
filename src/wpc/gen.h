#ifndef INC_GEN
#define INC_GEN

#if defined(__GNUC__)
#define U64(x) __extension__ x##LL
#elif defined(_MSC_VER)
#define U64(x) x##ui64
#endif

/*-- All Hardware Generations --*/
#define GEN_WPCALPHA_1  U64(0x00000000001) /* Alpha-numeric display S11 sound, Dr Dude 10/90 */
#define GEN_WPCALPHA_2  U64(0x00000000002) /* Alpha-numeric display,  - The Machine BOP 4/91*/
#define GEN_WPCDMD      U64(0x00000000004) /* Dot Matrix Display, Terminator 2 7/91 - Party Zone 10/91 */
#define GEN_WPCFLIPTRON U64(0x00000000008) /* Fliptronic flippers, Addams Family 2/92 - Twilight Zone 5/93 */
#define GEN_WPCDCS      U64(0x00000000010) /* DCS Sound system, Indiana Jones 10/93 - Popeye 3/94 */
#define GEN_WPCSECURITY U64(0x00000000020) /* Security chip, World Cup Soccer 3/94 - Jackbot 10/95 */
#define GEN_WPC95DCS    U64(0x00000000040) /* Hybrid WPC95 driver + DCS sound, Who Dunnit */
#define GEN_WPC95       U64(0x00000000080) /* Integreted boards, Congo 3/96 - Cactus Canyon 2/99 */
#define GEN_S11         U64(0x00000000100) /* S11  CPU, 2x7A+2x7+1x4 LED */
#define GEN_S11A        U64(0x00000000200) /* S11A CPU, 2x7A+2x7+1x4 LED */
#define GEN_S11B_1      U64(0x00000000400) /* S11B CPU, 2x7A+2x7 LED */
#define GEN_S11B_2      U64(0x00000000800) /* S11B CPU, 2x16A LED, */
#define GEN_S11B_2x     U64(0x00000001000) /* S11B CPU, 2x16A LED, Solenoid board */
#define GEN_S11B_3      U64(0x00000002000) /* S11B CPU, 2x16A LED ?, Jokerz sound */
#define GEN_S11C        U64(0x00000004000) /* S11C CPU, 2x16A LED */
#define GEN_S9          U64(0x00000008000) /* S9 CPU, 4x7+1x4 */
#define GEN_S7          U64(0x00000010000) /* S7 CPU */
#define GEN_S6          U64(0x00000020000) /* S6 CPU */
#define GEN_S4          U64(0x00000040000) /* S4 CPU */
#define GEN_S3C         U64(0x00000080000) /* S3 CPU */
#define GEN_S3          U64(0x00000100000)
#define GEN_BY17        U64(0x00000200000)
#define GEN_BY35        U64(0x00000400000)
#define GEN_BY6803      U64(0x00000800000)
#define GEN_BY6803A     U64(0x00001000000)
#define GEN_STMPU100	U64(0x00080000000) /* Stern MPU - 100*/
#define GEN_STMPU200	U64(0x00100000000) /* Stern MPU - 200*/
#define GEN_GTS80S	U64(0x00200000000) /* GTS80 - Sound Board */
#define GEN_GTS80SS	U64(0x00400000000) /* GTS80 - Sound & Speech Board */
#define GEN_GTS80B	U64(0x00800000000) /* GTS80B, GTS80B Sound Board */
#define GEN_GTS80B2K	U64(0x01000000000) /* GTS80B, 2KROM */
#define GEN_GTS80B4K	U64(0x02000000000) /* GTS80B, 4KROM, slam switch is reversed */
#define GEN_WS          U64(0x04000000000) /* Whitestar */
#define GEN_WS_1        U64(0x08000000000) /* Whitestar with extra RAM */
#define GEN_WS_2        U64(0x10000000000) /* Whitestar with extra DMD */

#define GEN_ALLWPC      U64(0x000000000ff)
#define GEN_ALLS11      U64(0x00000007f00)
#define GEN_ALLBY35     U64(0x00000600000)
#define GEN_ALLS80      U64(0x03e00000000)


#endif /* INC_GEN */

