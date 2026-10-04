// license:BSD-3-Clause

#include "zipsrc.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static uint32_t le16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t le32(const uint8_t *p) { return le16(p) | le16(p + 2) << 16; }

static int read_at(FILE *f, uint32_t off, void *buf, uint32_t len)
{
	if (fseek(f, (long)off, SEEK_SET)) return -1;
	return fread(buf, 1, len, f) == len ? 0 : -1;
}

static const char *zs_name(void *ctx, int i) { return ((zipsrc *)ctx)->e[i].name; }
static uint32_t zs_size(void *ctx, int i) { return ((zipsrc *)ctx)->e[i].usize; }

static int data_offset(zipsrc *z, zipsrc_entry *e)
{
	uint8_t h[30];
	if (e->data_off) return 0;
	if (read_at(z->f, e->hdr_off, h, 30) || le32(h) != 0x04034B50u) return -1;
	e->data_off = e->hdr_off + 30 + le16(h + 26) + le16(h + 28);
	return 0;
}

static void evict(zipsrc *z, uint32_t need, int keep)
{
	while (z->cache_used && z->cache_used + need > z->cache_bytes) {
		int k, old = -1;
		for (k = 0; k < z->count; k++)
			if (k != keep && z->e[k].data && (old < 0 || z->e[k].stamp < z->e[old].stamp)) old = k;
		if (old < 0) return;
		free(z->e[old].data);
		z->e[old].data = NULL;
		z->cache_used -= z->e[old].usize;
	}
}

static int inflate_entry(zipsrc *z, int i)
{
	zipsrc_entry *e = &z->e[i];
	z_stream s;
	uint8_t in[16384];
	uint32_t left = e->csize;
	int r = Z_OK;

	evict(z, e->usize, i);
	e->data = (uint8_t *)malloc(e->usize ? e->usize : 1);
	if (!e->data) return -1;
	memset(&s, 0, sizeof(s));
	if (inflateInit2(&s, -MAX_WBITS) != Z_OK) { free(e->data); e->data = NULL; return -1; }
	s.next_out = e->data;
	s.avail_out = e->usize;
	if (fseek(z->f, (long)e->data_off, SEEK_SET)) r = Z_ERRNO;
	while (r == Z_OK && left) {
		uint32_t n = left < sizeof(in) ? left : (uint32_t)sizeof(in);
		if (fread(in, 1, n, z->f) != n) { r = Z_ERRNO; break; }
		left -= n;
		s.next_in = in;
		s.avail_in = n;
		r = inflate(&s, Z_NO_FLUSH);
	}
	inflateEnd(&s);
	if (r != Z_STREAM_END || s.total_out != e->usize) { free(e->data); e->data = NULL; return -1; }
	z->cache_used += e->usize;
	return 0;
}

static int zs_read(void *ctx, int i, uint32_t off, uint8_t *buf, uint32_t len)
{
	zipsrc *z = (zipsrc *)ctx;
	zipsrc_entry *e;

	if (i < 0 || i >= z->count) return -1;
	e = &z->e[i];
	if (off > e->usize || len > e->usize - off) return -1;
	if (data_offset(z, e)) return -1;
	if (e->method == 0) return read_at(z->f, e->data_off + off, buf, len);
	if (e->method != 8) return -1;
	if (!e->data && inflate_entry(z, i)) return -1;
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

int zipsrc_open(zipsrc *z, const char *zip_path, uint32_t cache_bytes)
{
	uint8_t tail[65557], *p, *cd = NULL;
	long size;
	uint32_t n, cd_size, cd_off, i, pos;
	int k;

	memset(z, 0, sizeof(*z));
	z->cache_bytes = cache_bytes;
	z->f = fopen(zip_path, "rb");
	if (!z->f) return -1;
	if (fseek(z->f, 0, SEEK_END) || (size = ftell(z->f)) < 22) goto fail;
	n = size < (long)sizeof(tail) ? (uint32_t)size : (uint32_t)sizeof(tail);
	if (read_at(z->f, (uint32_t)size - n, tail, n)) goto fail;
	for (p = tail + n - 22; p >= tail && le32(p) != 0x06054B50u; p--) ;
	if (p < tail) goto fail;
	z->count = (int)le16(p + 10);
	cd_size = le32(p + 12);
	cd_off = le32(p + 16);
	if (cd_off == 0xFFFFFFFFu || le16(p + 10) == 0xFFFF) goto fail;
	cd = (uint8_t *)malloc(cd_size ? cd_size : 1);
	z->e = (zipsrc_entry *)calloc((size_t)z->count + 1, sizeof(zipsrc_entry));
	if (!cd || !z->e || read_at(z->f, cd_off, cd, cd_size)) goto fail;
	for (i = 0, pos = 0, k = 0; i < (uint32_t)z->count; i++) {
		uint8_t *h = cd + pos;
		uint32_t nl, el, cl;
		char *name;
		if (pos + 46 > cd_size || le32(h) != 0x02014B50u) goto fail;
		nl = le16(h + 28); el = le16(h + 30); cl = le16(h + 32);
		if (pos + 46 + nl > cd_size) goto fail;
		name = (char *)malloc(nl + 1);
		if (!name) goto fail;
		memcpy(name, h + 46, nl);
		name[nl] = 0;
		pos += 46 + nl + el + cl;
		if (!nl) { free(name); continue; }
		{
			char *c;
			for (c = name; *c; c++) if (*c == '\\') *c = '/';
		}
		z->e[k].name = name;
		z->e[k].method = (uint16_t)le16(h + 10);
		z->e[k].csize = le32(h + 20);
		z->e[k].usize = le32(h + 24);
		z->e[k].hdr_off = le32(h + 42);
		k++;
	}
	z->count = k;
	strip_common_folder(z);
	free(cd);
	z->src.ctx = z;
	z->src.count = z->count;
	z->src.name = zs_name;
	z->src.size = zs_size;
	z->src.read = zs_read;
	return 0;
fail:
	free(cd);
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
	if (z->f) fclose(z->f);
	memset(z, 0, sizeof(*z));
}

const vfat_source *zipsrc_source(zipsrc *z) { return &z->src; }
