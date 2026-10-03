// license:BSD-3-Clause

#ifndef PINHECK_BOARD_H
#define PINHECK_BOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOARD_SOLS     24
#define BOARD_SERVOS   5
#define BOARD_RGB_MAX  16
#define BOARD_PORTS    7

enum { BOARD_RGB_ONBOARD, BOARD_RGB_EXTERNAL, BOARD_RGB_CHAINS };

typedef struct pinheck_board_io {
	void *ctx;
	uint8_t (*sw_col)(void *ctx, int col);     /* closed rows of switch column 0..7 */
	uint16_t (*cabinet)(void *ctx);             /* closed cabinet inputs: bits 0-7 U12 D0-D7, bits 8-15 U11 D0-D7 */
	void (*lamps)(void *ctx, uint64_t t, uint8_t cols, uint8_t rows);
	void (*sols)(void *ctx, uint64_t t, uint32_t sols);
	void (*gi)(void *ctx, uint64_t t, uint16_t gi);
	void (*start_lamp)(void *ctx, uint64_t t, int on);
	void (*rgb)(void *ctx, uint64_t t, int chain, int led, uint8_t r, uint8_t g, uint8_t b);
	void (*servo)(void *ctx, uint64_t t, int servo, uint32_t pulse); /* pulse 0: no pulse for 60 ms */
} pinheck_board_io;

typedef struct pinheck_board_ws {
	int bits;
	uint64_t last;
	uint8_t buf[BOARD_RGB_MAX * 3];
} pinheck_board_ws;

typedef struct pinheck_board {
	pinheck_board_io io;
	uint32_t hz;
	uint32_t lat[BOARD_PORTS], tris[BOARD_PORTS];
	uint32_t sol_pins, sols;
	uint64_t wd_until;
	int wd_on;
	uint8_t lamp_cols, lamp_rows;
	int start;
	uint16_t sr595, gi, sr165;
	int cab_lat;
	pinheck_board_ws ws[BOARD_RGB_CHAINS];
	int ws_leds[BOARD_RGB_CHAINS];
	uint64_t servo_rise[BOARD_SERVOS];
	int servo_on[BOARD_SERVOS];
} pinheck_board;

void pinheck_board_init(pinheck_board *b, const pinheck_board_io *io, uint32_t hz);
void pinheck_board_port(pinheck_board *b, int port, uint32_t lat, uint32_t tris, uint64_t t);
uint32_t pinheck_board_read(pinheck_board *b, int port, uint64_t t);
void pinheck_board_tick(pinheck_board *b, uint64_t t);

#ifdef __cplusplus
}
#endif

#endif
