// license:BSD-3-Clause

#include "zipsrc.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* The SD card's files, read from the romset zip with unzip.c: stored entries are read in place, deflated ones
   are decompressed whole on first use and kept in a cache of cache_bytes, the least recently used evicted */

static const char *zs_name(void *ctx, int i) { return ((zipsrc *)ctx)->e[i].name; }
static uint32_t zs_size(void *ctx, int i) { return ((zipsrc *)ctx)->e[i].ent.uncompressed_size; }

static void evict(zipsrc *z, uint32_t need, int keep)
{
	while (z->cache_used && z->cache_used + need > z->cache_bytes) {
		int k, old = -1;
		for (k = 0; k < z->count; k++)
			if (k != keep && z->e[k].data && (old < 0 || z->e[k].stamp < z->e[old].stamp)) old = k;
		if (old < 0) return;
		free(z->e[old].data);
		z->e[old].data = NULL;
		z->cache_used -= z->e[old].ent.uncompressed_size;
	}
}

static int inflate_entry(zipsrc *z, int i)
{
	zipsrc_entry *e = &z->e[i];
	const uint32_t size = e->ent.uncompressed_size;

	evict(z, size, i);
	e->data = (uint8_t *)malloc(size ? size : 1);
	if (!e->data) return -1;
	if (readuncompresszip(z->zip, &e->ent, (char *)e->data)) {
		free(e->data);
		e->data = NULL;
		return -1;
	}
	z->cache_used += size;
	return 0;
}

static int zs_read(void *ctx, int i, uint32_t off, uint8_t *buf, uint32_t len)
{
	zipsrc *z = (zipsrc *)ctx;
	zipsrc_entry *e;

	if (i < 0 || i >= z->count) return -1;
	e = &z->e[i];
	if (e->failed || off > e->ent.uncompressed_size || len > e->ent.uncompressed_size - off) return -1;
	if (e->ent.compression_method == 0) {
		if (!e->data_off && (e->data_off = offsetcompresszip(z->zip, &e->ent)) <= 0) { e->data_off = 0; e->failed = 1; return -1; }
		if (readzipat(z->zip, e->data_off + off, (char *)buf, len)) { e->failed = 1; return -1; }
		return 0;
	}
	if (e->ent.compression_method != 8 || (!e->data && inflate_entry(z, i))) { e->failed = 1; return -1; }
	e->stamp = ++z->clock;
	memcpy(buf, e->data + off, len);
	return 0;
}

static int is_media_dir(const char *n, size_t len)
{
	return len == 3 && ((toupper((unsigned char)n[0]) == 'D' && toupper((unsigned char)n[1]) == 'M' && toupper((unsigned char)n[2]) == 'D') ||
	                    (toupper((unsigned char)n[0]) == 'S' && toupper((unsigned char)n[1]) == 'F' && toupper((unsigned char)n[2]) == 'X'));
}

/* a zip made from a folder keeps everything under one top-level folder; present its contents at the root */
static void strip_common_folder(zipsrc *z)
{
	const char *slash;
	size_t len;
	int i, k;
	if (z->count < 1 || (slash = strchr(z->e[0].name, '/')) == NULL) return;
	len = (size_t)(slash - z->e[0].name) + 1;
	if (is_media_dir(z->e[0].name, len - 1)) return;
	for (i = 0; i < z->count; i++)
		if (strncmp(z->e[i].name, z->e[0].name, len)) return;
	for (i = 0, k = 0; i < z->count; i++) {
		memmove(z->e[i].name, z->e[i].name + len, strlen(z->e[i].name + len) + 1);
		if (!z->e[i].name[0]) { free(z->e[i].name); continue; }
		z->e[k++] = z->e[i];
	}
	z->count = k;
}

int zipsrc_open(zipsrc *z, int pathtype, int pathindex, const char *zip_name, uint32_t cache_bytes)
{
	struct zipent *ent;
	int max;

	memset(z, 0, sizeof(*z));
	z->cache_bytes = cache_bytes;
	z->zip = openzip(pathtype, pathindex, zip_name);
	if (!z->zip) return -1;
	max = z->zip->total_entries_cent_dir;
	z->e = (zipsrc_entry *)calloc((size_t)max + 1, sizeof(zipsrc_entry));
	if (!z->e) goto fail;
	while (z->count < max && (ent = readzip(z->zip)) != NULL) {
		zipsrc_entry *e = &z->e[z->count];
		char *c;
		if (!ent->filename_length) continue;
		e->name = (char *)malloc((size_t)ent->filename_length + 1);
		if (!e->name) goto fail;
		memcpy(e->name, ent->name, (size_t)ent->filename_length + 1);
		for (c = e->name; *c; c++) if (*c == '\\') *c = '/';
		e->ent = *ent;
		e->ent.name = NULL;
		z->count++;
	}
	/* readzip stops early on a broken directory */
	if (z->zip->cd_pos < z->zip->size_of_cent_dir && z->count < max) goto fail;
	strip_common_folder(z);
	z->src.ctx = z;
	z->src.count = z->count;
	z->src.name = zs_name;
	z->src.size = zs_size;
	z->src.read = zs_read;
	return 0;
fail:
	zipsrc_close(z);
	return -1;
}

void zipsrc_close(zipsrc *z)
{
	int k;
	if (z->e) {
		for (k = 0; k < z->count; k++) { free(z->e[k].name); free(z->e[k].data); }
		free(z->e);
	}
	if (z->zip) closezip(z->zip);
	memset(z, 0, sizeof(*z));
}

const vfat_source *zipsrc_source(zipsrc *z) { return &z->src; }
