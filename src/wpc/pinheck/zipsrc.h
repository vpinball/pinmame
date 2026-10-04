// license:BSD-3-Clause

#ifndef PINHECK_ZIPSRC_H
#define PINHECK_ZIPSRC_H

#include <stdio.h>
#include <stdint.h>
#include "vfat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zipsrc_entry {
	char *name;
	uint32_t csize, usize, hdr_off, data_off;
	uint16_t method;
	uint8_t *data;
	uint32_t stamp;
} zipsrc_entry;

typedef struct zipsrc {
	FILE *f;
	int count;
	zipsrc_entry *e;
	uint32_t cache_bytes, cache_used, clock;
	vfat_source src;
} zipsrc;

int zipsrc_open(zipsrc *z, const char *zip_path, uint32_t cache_bytes);
void zipsrc_close(zipsrc *z);
const vfat_source *zipsrc_source(zipsrc *z);

#ifdef __cplusplus
}
#endif

#endif
