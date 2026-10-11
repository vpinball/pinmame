// license:BSD-3-Clause

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A raw 128 x 32 dot matrix scanned by a Propeller cog: per row 128 dots shifted on P16 (data) and P17 (clock,
   rising), P18 latches them (rising), P19 clocks the row driver (rising) with P20 high for the first row. A
   subframe is 32 rows, 1 bit per dot, 16 bytes per row, the first dot shifted in the top bit of byte 0. */
#define DMD_W      128
#define DMD_H      32
#define DMD_ROW    (DMD_W / 8)
#define DMD_SUB    (DMD_ROW * DMD_H)
#define DMD_LEVELS 16
#define DMD_P16 (1u << 16)
#define DMD_P17 (1u << 17)
#define DMD_P18 (1u << 18)
#define DMD_P19 (1u << 19)
#define DMD_P20 (1u << 20)
#define DMD_ROW_PINS (DMD_P18 | DMD_P19 | DMD_P20)
#define DMD_ALL_PINS (DMD_P16 | DMD_P17 | DMD_ROW_PINS)

typedef void (*dmd_sub_fn)(void *ctx, const uint8_t *sub, int level, uint64_t t); /* level 0-15, -1 unknown */
typedef void (*dmd_frame_fn)(void *ctx, const uint8_t *shades, uint64_t t);       /* DMD_W * DMD_H dots, 0-15 */

typedef struct pinheck_dmd {
	void *ctx;
	dmd_sub_fn on_sub;
	dmd_frame_fn on_frame;
	const uint8_t *hub;  /* row model: hub RAM */
	uint8_t (*hub_at)(void *ctx, uint32_t a, uint64_t t); /* row model: hub RAM as at t, if set (else hub) */
	uint32_t buf;        /* row model: hub address of the 4 bpp frame (two dots a byte, the left one high) */
	uint32_t level;      /* pins as last seen */
	int full;            /* 1: dots from the shifted bits (P16/P17), 0: from hub RAM at the latch */
	int on;              /* the scan pins are outputs */
	int rows;            /* latches since the first row, -1 before one */
	int row;             /* the row driver's position, -1 before its first row */
	int count;           /* subframes started since the pins became outputs */
	int sublevel;        /* level of the subframe being drawn, -1 unknown */
	uint32_t nbits;      /* bits shifted */
	uint8_t shift[DMD_ROW], latched[DMD_ROW];
	uint8_t sub[DMD_SUB];
	uint8_t sum[DMD_W * DMD_H];
	int nsum;            /* subframes in sum, from level 0 on; -1 until one starts */
} pinheck_dmd;

/* full: 1 decodes P16/P17 (mask DMD_ALL_PINS), 0 takes the dots from hub (mask DMD_ROW_PINS) */
void pinheck_dmd_init(pinheck_dmd * const d, int full, const uint8_t *hub, uint32_t buf, void *ctx, dmd_sub_fn on_sub, dmd_frame_fn on_frame);
void pinheck_dmd_pins(pinheck_dmd * const d, uint64_t t, uint32_t out, uint32_t dir);

#ifdef __cplusplus
}
#endif
