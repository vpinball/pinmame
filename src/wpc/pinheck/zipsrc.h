// license:BSD-3-Clause

#pragma once

#include <stdint.h>
#include "unzip.h"
#include "vfat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zipsrc_entry {
	char *name;
	struct zipent ent; /* its name is NULL, the entry owns name */
	INT64 data_off;    /* stored entries: data offset in the zip, 0 until known */
	uint8_t *data;     /* deflated entries: the decompressed data while cached */
	uint32_t stamp;
	int failed;        /* do not retry (and report) a broken entry */
} zipsrc_entry;

typedef struct zipsrc {
	ZIP *zip;
	int count;
	zipsrc_entry *e;
	uint32_t cache_bytes, cache_used, clock;
	vfat_source src;
} zipsrc;

int zipsrc_open(zipsrc *z, int pathtype, int pathindex, const char *zip_name, uint32_t cache_bytes);
void zipsrc_close(zipsrc *z);
const vfat_source *zipsrc_source(zipsrc *z);

#ifdef __cplusplus
}
#endif
