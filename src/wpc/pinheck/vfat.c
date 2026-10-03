// license:BSD-3-Clause

#include "vfat.h"
#include <stdlib.h>
#include <string.h>

#define RESERVED 32u
#define CLUSTER_BYTES (VFAT_SPC * 512u)
#define MIN_CLUSTERS 65536u
#define FTIME 0x6000u
#define FDATE 0x48E1u

typedef struct rec { char key[64]; char part[8][11]; int nparts, src, is_dir; } rec;

static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }

static int valid83(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c && strchr("$%'-_@~`!(){}^#&", c));
}

static int to83(const char *s, size_t n, char *out)
{
	size_t i, dot = n, b = 0, e = 0;
	memset(out, ' ', 11);
	for (i = 0; i < n; i++)
		if (s[i] == '.') { if (dot != n) return -1; dot = i; }
	if (dot == 0 || dot > 8 || (dot < n && (n - dot - 1 < 1 || n - dot - 1 > 3))) return -1;
	for (i = 0; i < n; i++) {
		char c = s[i];
		if (i == dot) continue;
		if (c >= 'a' && c <= 'z') c = (char)(c - 32);
		if (!valid83(c)) return -1;
		if (i < dot) out[b++] = c; else out[8 + e++] = c;
	}
	return 0;
}

static int split(const char *path, rec *r)
{
	const char *p = path, *q;
	int k = 0;
	size_t kl = 0;
	if (strchr(path, '/') && !((path[0] == 'D' || path[0] == 'd') && (path[1] == 'M' || path[1] == 'm') && (path[2] == 'D' || path[2] == 'd') && path[3] == '/') &&
	    !((path[0] == 'S' || path[0] == 's') && (path[1] == 'F' || path[1] == 'f') && (path[2] == 'X' || path[2] == 'x') && path[3] == '/'))
		return 1;
	r->is_dir = 0;
	for (;;) {
		q = strchr(p, '/');
		if (!q) q = p + strlen(p);
		if (*q == '/' && !q[1]) r->is_dir = 1;
		if (k == 8 || to83(p, (size_t)(q - p), r->part[k])) return -1;
		if (kl + 12 >= sizeof(r->key)) return -1;
		memcpy(r->key + kl, r->part[k], 11);
		r->key[kl + 11] = '/';
		kl += 12;
		k++;
		if (!*q || !q[1]) break;
		p = q + 1;
	}
	r->key[kl] = 0;
	r->nparts = k;
	return 0;
}

static int keycmp(const void *a, const void *b) { return strcmp(((const rec *)a)->key, ((const rec *)b)->key); }

static int find_child(const vfat *v, int parent, const char *name, int is_dir)
{
	int k;
	for (k = 1; k < v->nnodes; k++)
		if (v->node[k].parent == parent && v->node[k].is_dir == is_dir && !memcmp(v->node[k].name, name, 11)) return k;
	return -1;
}

static int add_node(vfat *v, int parent, const char *name, int is_dir, int src, uint32_t size)
{
	vfat_node *n = &v->node[v->nnodes];
	memset(n, 0, sizeof(*n));
	memcpy(n->name, name, 11);
	n->parent = parent;
	n->is_dir = is_dir;
	n->src = src;
	n->size = size;
	return v->nnodes++;
}

static void dirent(uint8_t *d, const char *name, uint8_t attr, uint32_t cl, uint32_t size)
{
	memcpy(d, name, 11);
	d[11] = attr;
	put16(d + 14, FTIME); put16(d + 16, FDATE); put16(d + 18, FDATE);
	put16(d + 20, cl >> 16); put16(d + 22, FTIME); put16(d + 24, FDATE);
	put16(d + 26, cl & 0xFFFF); put32(d + 28, size);
}

int vfat_init(vfat *v, const vfat_source *src)
{
	rec *r = NULL;
	int i, k, nr = 0, ndirs;
	uint32_t cl;

	memset(v, 0, sizeof(*v));
	v->src = src;
	v->part_start = VFAT_PART_START;
	r = (rec *)calloc((size_t)src->count + 1, sizeof(rec));
	v->node = (vfat_node *)calloc((size_t)src->count * 8 + 1, sizeof(vfat_node));
	v->alloc = (int *)calloc((size_t)src->count * 8 + 1, sizeof(int));
	if (!r || !v->node || !v->alloc) goto fail;
	for (i = 0; i < src->count; i++) {
		int s = split(src->name(src->ctx, i), &r[nr]);
		if (s < 0) v->skipped++;
		if (s) continue;
		r[nr++].src = i;
	}
	qsort(r, (size_t)nr, sizeof(rec), keycmp);
	add_node(v, -1, "DOMINOS    ", 1, -1, 0);
	for (i = 0; i < nr; i++) {
		int parent = 0, depth = r[i].nparts - (r[i].is_dir ? 0 : 1);
		for (k = 0; k < depth; k++) {
			int c = find_child(v, parent, r[i].part[k], 1);
			parent = c >= 0 ? c : add_node(v, parent, r[i].part[k], 1, -1, 0);
		}
	}
	ndirs = v->nnodes;
	for (i = 0; i < nr; i++) {
		int parent = 0;
		if (r[i].is_dir) continue;
		for (k = 0; k < r[i].nparts - 1; k++) parent = find_child(v, parent, r[i].part[k], 1);
		if (find_child(v, parent, r[i].part[r[i].nparts - 1], 0) >= 0 || find_child(v, parent, r[i].part[r[i].nparts - 1], 1) >= 0) { v->skipped++; continue; }
		add_node(v, parent, r[i].part[r[i].nparts - 1], 0, r[i].src, src->size(src->ctx, r[i].src));
	}
	free(r);
	r = NULL;
	for (k = 0; k < ndirs; k++) {
		uint32_t n = k ? 2 : 1;
		for (i = 1; i < v->nnodes; i++) if (v->node[i].parent == k) n++;
		v->node[k].size = n * 32;
	}
	for (k = 0, cl = 2; k < v->nnodes; k++) {
		vfat_node *n = &v->node[k];
		n->nclus = (n->size + CLUSTER_BYTES - 1) / CLUSTER_BYTES;
		if (!n->nclus) continue;
		n->first = cl;
		cl += n->nclus;
		v->alloc[v->nalloc++] = k;
	}
	v->used = cl - 2;
	v->clusters = v->used < MIN_CLUSTERS ? MIN_CLUSTERS : v->used;
	v->fat_sectors = ((v->clusters + 2) * 4 + 511) / 512;
	v->data_start = RESERVED + 2 * v->fat_sectors;
	v->vol_sectors = v->data_start + v->clusters * VFAT_SPC;
	for (k = 0; k < ndirs; k++) {
		vfat_node *n = &v->node[k];
		uint8_t *d;
		n->dir = (uint8_t *)calloc(n->nclus, CLUSTER_BYTES);
		if (!n->dir) goto fail;
		d = n->dir;
		if (k) {
			dirent(d, ".          ", 0x10, n->first, 0);
			dirent(d + 32, "..         ", 0x10, n->parent ? v->node[n->parent].first : 0, 0);
			d += 64;
		} else {
			memcpy(d, n->name, 11);
			d[11] = 0x08;
			put16(d + 22, FTIME); put16(d + 24, FDATE);
			d += 32;
		}
		for (i = 1; i < v->nnodes; i++) {
			vfat_node *c = &v->node[i];
			if (c->parent != k) continue;
			dirent(d, c->name, c->is_dir ? 0x10 : 0x20, c->first, c->is_dir ? 0 : c->size);
			d += 32;
		}
	}
	return 0;
fail:
	free(r);
	vfat_free(v);
	return -1;
}

void vfat_free(vfat *v)
{
	int k;
	if (v->node) for (k = 0; k < v->nnodes; k++) free(v->node[k].dir);
	free(v->node);
	free(v->alloc);
	memset(v, 0, sizeof(*v));
}

uint32_t vfat_sectors(const vfat *v) { return v->part_start + v->vol_sectors; }

static int owner(const vfat *v, uint32_t cl)
{
	int lo = 0, hi = v->nalloc - 1;
	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		const vfat_node *n = &v->node[v->alloc[mid]];
		if (cl < n->first) hi = mid - 1;
		else if (cl >= n->first + n->nclus) lo = mid + 1;
		else return v->alloc[mid];
	}
	return -1;
}

static void boot_sector(const vfat *v, uint8_t *b)
{
	b[0] = 0xEB; b[1] = 0x58; b[2] = 0x90;
	memcpy(b + 3, "MSWIN4.1", 8);
	put16(b + 11, 512); b[13] = (uint8_t)VFAT_SPC; put16(b + 14, RESERVED); b[16] = 2;
	b[21] = 0xF8; put16(b + 24, 63); put16(b + 26, 255);
	put32(b + 28, v->part_start); put32(b + 32, v->vol_sectors); put32(b + 36, v->fat_sectors);
	put32(b + 44, 2); put16(b + 48, 1); put16(b + 50, 6);
	b[64] = 0x80; b[66] = 0x29; put32(b + 67, 0x20160701u);
	memcpy(b + 71, "DOMINOS    ", 11); memcpy(b + 82, "FAT32   ", 8);
	b[510] = 0x55; b[511] = 0xAA;
}

static void fsinfo(const vfat *v, uint8_t *b)
{
	put32(b, 0x41615252u); put32(b + 484, 0x61417272u);
	put32(b + 488, v->clusters - v->used); put32(b + 492, 2 + v->used);
	put32(b + 508, 0xAA550000u);
}

static void fat_sector(const vfat *v, uint32_t s, uint8_t *b)
{
	uint32_t j;
	for (j = 0; j < 128; j++) {
		uint32_t cl = s * 128 + j, e = 0;
		if (cl == 0) e = 0x0FFFFFF8u;
		else if (cl == 1) e = 0x0FFFFFFFu;
		else if (cl < 2 + v->used) {
			int o = owner(v, cl);
			if (o >= 0) e = cl == v->node[o].first + v->node[o].nclus - 1 ? 0x0FFFFFFFu : cl + 1;
		}
		put32(b + 4 * j, e);
	}
}

int vfat_read(vfat *v, uint32_t lba, uint8_t *buf512)
{
	uint32_t r;
	memset(buf512, 0, 512);
	if (lba >= vfat_sectors(v)) return -1;
	if (lba < v->part_start) {
		if (lba == 0) {
			buf512[446 + 1] = 0xFE; buf512[446 + 2] = 0xFF; buf512[446 + 3] = 0xFF;
			buf512[446 + 4] = 0x0C;
			buf512[446 + 5] = 0xFE; buf512[446 + 6] = 0xFF; buf512[446 + 7] = 0xFF;
			put32(buf512 + 446 + 8, v->part_start);
			put32(buf512 + 446 + 12, v->vol_sectors);
			put32(buf512 + 440, 0x20160701u);
			buf512[510] = 0x55; buf512[511] = 0xAA;
		}
		return 0;
	}
	r = lba - v->part_start;
	if (r == 0 || r == 6) { boot_sector(v, buf512); return 0; }
	if (r == 1 || r == 7) { fsinfo(v, buf512); return 0; }
	if (r < RESERVED) return 0;
	if (r < v->data_start) { fat_sector(v, (r - RESERVED) % v->fat_sectors, buf512); return 0; }
	r -= v->data_start;
	{
		uint32_t cl = 2 + r / VFAT_SPC, off;
		int o;
		vfat_node *n;
		if (cl >= 2 + v->used || (o = owner(v, cl)) < 0) return 0;
		n = &v->node[o];
		off = ((cl - n->first) * VFAT_SPC + r % VFAT_SPC) * 512;
		if (n->is_dir) { memcpy(buf512, n->dir + off, 512); return 0; }
		if (off >= n->size) return 0;
		return v->src->read(v->src->ctx, n->src, off, buf512, n->size - off < 512 ? n->size - off : 512);
	}
}
