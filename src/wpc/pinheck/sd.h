// license:BSD-3-Clause

#ifndef PINHECK_SD_H
#define PINHECK_SD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sd_blockdev {
	void *ctx;
	uint32_t sectors;
	int (*read)(void *ctx, uint32_t lba, uint8_t *buf512);
} sd_blockdev;

typedef struct sd_card {
	sd_blockdev dev;
	int cs, sclk, dout;
	int idle, v2, hc, acmd, multi, wstate;
	uint32_t next_lba, wcount, writes, reads;
	uint8_t cmd[6];
	int ncmd;
	uint8_t in, inbits, risen;
	uint8_t out[640];
	int olen, opos, obit;
} sd_card;

void sd_init(sd_card *s, const sd_blockdev *dev);
int sd_update(sd_card *s, int cs, int sclk, int mosi);

#ifdef __cplusplus
}
#endif

#endif
