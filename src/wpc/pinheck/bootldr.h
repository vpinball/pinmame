// license:BSD-3-Clause

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_BIT     688u
#define BOOT_WINDOW  240000000ull
#define BOOT_POLL    80000u
#define BOOT_LATENCY 8000u
#define BOOT_PROGRAM 160000u
#define BOOT_EDGES   1024
#define BOOT_FRAME   600

enum { BOOT_WAIT, BOOT_HOST, BOOT_APP };

typedef void (*boot_tx_fn)(void *ctx, uint64_t pic_cycle, int level);
typedef void (*boot_log_fn)(void *ctx, const char *msg);

typedef struct pic32_boot {
	uint8_t *flash;
	uint32_t flash_size;
	int state;
	uint64_t window, window_end, app_at, tx_free; /* window: the hold after a reset, BOOT_WINDOW unless set */
	int line;
	uint64_t edge_t[BOOT_EDGES];
	uint8_t edge_l[BOOT_EDGES];
	int nedge;
	uint64_t scan_from;
	uint8_t frame[BOOT_FRAME];
	int flen;
	uint32_t addr;
	uint8_t erased[128];
	boot_tx_fn tx;
	void *tx_ctx;
	int tx_level;
	boot_log_fn log;
	void *log_ctx;
	uint32_t unknown_logged[8];
	unsigned int frames, programmed, read, errors;
} pic32_boot;

void boot_init(pic32_boot *b, uint8_t *flash, uint32_t flash_size, boot_tx_fn tx, void *tx_ctx);
void boot_set_log(pic32_boot *b, boot_log_fn fn, void *ctx);
void boot_set_window(pic32_boot *b, uint64_t pic_cycles); /* from the next reset on */
void boot_reset(pic32_boot *b, uint64_t pic_cycle);
void boot_rx(pic32_boot *b, uint64_t pic_cycle, int level);
void boot_advance(pic32_boot *b, uint64_t pic_cycle);
void boot_stop(pic32_boot *b);
uint64_t boot_hold(const pic32_boot *b, uint64_t pic_cycle);

#ifdef __cplusplus
}
#endif
