// license:BSD-3-Clause

#pragma once

#include "../../cpu/p8x32a/p8x32a.h"
#include "eeprom.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PROP_EDGES 4096
#define PROP_SEGS  32
#define PROP_PIC_PINS ((1u << 24) | (1u << 25) | (1u << 26))
#define PROP_SAMPS 1024
/* gap that keeps the two threads' fields on separate cache lines: Intel's adjacent-line prefetcher fetches 64 byte lines in pairs, Apple's M cores have 128 byte lines */
#define PROP_PAD 128

typedef int (*prop_spi_fn)(void *ctx, int cs, int sclk, int mosi);
typedef void (*prop_log_fn)(void *ctx, const char *msg);
typedef void (*prop_tx_fn)(void *ctx, uint64_t pic_cycle, int level);
typedef void (*prop_ctr_fn)(void *ctx, uint64_t t, int cog, int ctr, uint32_t ctr_reg, uint32_t frq);
typedef void (*prop_pins_fn)(void *ctx, uint64_t prop_cycle, uint32_t out, uint32_t dir);
typedef uint64_t (*prop_clock_fn)(void *ctx);

typedef struct prop_edge { uint64_t pic, prop; uint32_t pins; } prop_edge; /* prop = pic in Propeller cycles */
typedef struct prop_seg { uint64_t pic0, prop0; uint32_t num, den; } prop_seg;

typedef struct pinheck_prop {
	p8x32a chip;
	cat24m01 eeprom;
	uint8_t *eemem;
	prop_edge edge[PROP_EDGES];
	int head, count;
	uint32_t base_pins, last_pins;
	prop_seg seg[PROP_SEGS];
	int nseg;
	uint64_t pic_last;
	uint32_t ee_bits, sd_do;
	uint32_t po_out, po_dir; /* the pins the devices last saw; po_ok 0: all see the next change */
	int po_ok;
	int reset_pending;
	prop_spi_fn sd;
	void *sd_ctx;
	prop_log_fn log;
	void *log_ctx;
	prop_tx_fn tx;
	void *tx_ctx;
	int tx_level;
	prop_ctr_fn snd_ctr;
	prop_pins_fn snd_pins;
	void *snd_ctx;
	prop_pins_fn pins;
	void *pins_ctx;
	uint32_t pins_mask; /* pins calls fn only on a change of these (default all) */
	prop_pins_fn pins_lazy; /* a lazy cog's pins (p8x32a.h), if they are all in lazy_mask */
	void *pins_lazy_ctx;
	uint32_t lazy_mask;
	uint32_t lz_mask, lz_out, lz_dir; /* the lazy cog's pins and their last state */
	/* With the worker thread, the fields above are the worker's. What the emulation thread uses for each post is
	   kept off the worker's lines, so that a post does not fetch a line back from the worker's core */
	char pad_poster[PROP_PAD];
	prop_clock_fn clock; /* the PIC32 cycle now */
	void *clock_ctx;
	void *worker; /* worker thread, NULL = calls run inline */
	uint64_t owner; /* the thread that started the worker: the only one whose prop_sync waits for it */
	uint32_t samp_post, samp_cmd[PROP_SAMPS]; /* P24 samples (prop_sample): posted, and the queue position of each */
	char pad_worker[PROP_PAD];
	/* written by the worker */
	uint64_t stamp; /* the PIC32 cycle at which the running call was made */
	volatile uint32_t samp_done;
	uint8_t samp_val[PROP_SAMPS];
} pinheck_prop;

void prop_init(pinheck_prop * const p, const uint8_t *rom32k, uint8_t *eemem);
void prop_attach_sd(pinheck_prop * const p, prop_spi_fn fn, void *ctx);
void prop_set_log(pinheck_prop * const p, prop_log_fn fn, void *ctx);
void prop_set_tx(pinheck_prop * const p, prop_tx_fn fn, void *ctx);
void prop_set_sound(pinheck_prop * const p, prop_ctr_fn ctr, prop_pins_fn pins, void *ctx);
void prop_set_pins(pinheck_prop * const p, prop_pins_fn fn, void *ctx);
void prop_set_pins_mask(pinheck_prop * const p, uint32_t mask);
void prop_set_pins_lazy(pinheck_prop * const p, prop_pins_fn fn, void *ctx, uint32_t mask);
void prop_reset(pinheck_prop * const p, uint64_t pic_cycle);
void prop_pic_pins(pinheck_prop * const p, uint64_t pic_cycle, uint32_t pins);
void prop_catch_up(pinheck_prop * const p, uint64_t pic_cycle);
int prop_p24(pinheck_prop * const p, uint64_t pic_cycle);
uint32_t prop_sample(pinheck_prop * const p, uint64_t pic_cycle);      /* with the worker: prop_p24 left to it; a token */
int prop_sample_get(pinheck_prop * const p, uint32_t token, int wait); /* its P24 (0, 1); -1 not yet (wait 0) */
void prop_set_clock(pinheck_prop * const p, prop_clock_fn fn, void *ctx);
uint64_t prop_stamp(const pinheck_prop * const p);
int prop_start_thread(pinheck_prop * const p);
void prop_stop_thread(pinheck_prop * const p);
void prop_sync(pinheck_prop * const p);
uint64_t prop_time(pinheck_prop * const p, uint64_t pic_cycle);

/* prop_governor, called each vblank with host and emulated time in seconds, keeps the worker only while faster. */
#define PROP_GOV_RING 5
typedef struct prop_gov {
	int state, flip, slow, n, k, warm;
	double w0, e0, wl, el, rate, next, pause, inl, thr[PROP_GOV_RING];
} prop_gov;
int prop_gov_start(pinheck_prop * const p, prop_gov * const g, double host_s, int flip);
void prop_governor(pinheck_prop * const p, prop_gov * const g, double host_s, double emu_s);

#ifdef __cplusplus
}
#endif
