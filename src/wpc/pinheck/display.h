// license:BSD-3-Clause

#ifndef PINHECK_DISPLAY_H
#define PINHECK_DISPLAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_W          128
#define DISPLAY_H          32
#define DISPLAY_FRAME      (DISPLAY_W * DISPLAY_H)
#define DISPLAY_FRAME_MAX  (DISPLAY_W * 2 * DISPLAY_H) /* the 128 x 64 module */
#define DISPLAY_SIZE_OK(w, h) ((w) == DISPLAY_W && ((h) == DISPLAY_H || (h) == 2 * DISPLAY_H))
#define DISPLAY_CFG_MAX    64
#define DISPLAY_LOG_FRAMES 16 /* discarded frames logged one by one */

#define DISPLAY_P17 (1u << 17)
#define DISPLAY_P20 (1u << 20)
#define DISPLAY_P21 (1u << 21)
#define DISPLAY_P22 (1u << 22)

/* the look: the module's settings from its config packet (14 bytes from the 128x32 module's service menu, 12 for the 128x64 module) */
#define DISPLAY_LOOK_W   (2 * DISPLAY_W)
#define DISPLAY_LOOK_H   (2 * DISPLAY_H)
#define DISPLAY_LOOK_MAX (DISPLAY_LOOK_W * 2 * DISPLAY_LOOK_H * 3) /* bytes of the 128x64 module's look */
#define DISPLAY_ROUND    0
#define DISPLAY_SQUARE   1
#define DISPLAY_HIGHREZ  2
#define DISPLAY_ALIGNED  340 /* the POSITION drawn unshifted */

typedef struct display_look {
	int shape;      /* PIXEL SHAPE: DISPLAY_ROUND, DISPLAY_SQUARE or DISPLAY_HIGHREZ */
	int brightness; /* BRIGHTNESS: 175-255 in the menu, 255 = the colours as sent */
	int position;   /* POSITION: 300-500 in the menu, DISPLAY_ALIGNED = aligned */
	int bar;        /* BAR BRIGHT: 0-62 in the menu, the light between round dots */
} display_look;

typedef void (*display_frame_fn)(void *ctx, const uint8_t *frame, uint64_t t); /* frame: d->frame bytes */
typedef void (*display_config_fn)(void *ctx, const uint8_t *bytes, int n, uint64_t t);
typedef void (*display_log_fn)(void *ctx, const char *msg);

typedef struct display {
	void *ctx;
	display_frame_fn on_frame;
	display_config_fn on_config;
	display_log_fn log;
	uint32_t level;
	uint8_t buf[DISPLAY_FRAME_MAX];
	long frame;     /* bytes in a frame: DISPLAY_FRAME, or as pinheck_display_size set it */
	long nbits;
	int mode;
	int logged_bits, logged_mixed, logged_cfg;
	int logged_frame; /* discarded frames logged, up to DISPLAY_LOG_FRAMES + 1 */
} display;

void pinheck_display_init(display *d, void *ctx, display_frame_fn on_frame, display_config_fn on_config, display_log_fn log);
int pinheck_display_size(display *d, int w, int h);
void pinheck_display_pins(display *d, uint64_t t, uint32_t out, uint32_t dir);
int pinheck_display_look(display_look *look, const uint8_t *cfg, int n);
void pinheck_display_render(const display_look *look, const uint8_t *frame, int h, uint8_t *rgb);

#ifdef __cplusplus
}
#endif

#endif
