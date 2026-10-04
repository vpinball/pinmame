// license:BSD-3-Clause

#ifndef PINHECK_VFAT_H
#define PINHECK_VFAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vfat_source {
	void *ctx;
	int count;
	const char *(*name)(void *ctx, int i);
	uint32_t (*size)(void *ctx, int i);
	int (*read)(void *ctx, int i, uint32_t off, uint8_t *buf, uint32_t len);
} vfat_source;

typedef struct vfat_node {
	char name[11];
	int parent, src, is_dir;
	uint32_t size, first, nclus;
	uint8_t *dir;
} vfat_node;

typedef struct vfat {
	const vfat_source *src;
	vfat_node *node;
	int *alloc;
	int nnodes, nalloc, skipped;
	uint32_t part_start, vol_sectors, fat_sectors, data_start, clusters, used;
} vfat;

#define VFAT_PART_START 8192u
#define VFAT_SPC 64u

int vfat_init(vfat *v, const vfat_source *src);
void vfat_free(vfat *v);
uint32_t vfat_sectors(const vfat *v);
int vfat_read(vfat *v, uint32_t lba, uint8_t *buf512);

#ifdef __cplusplus
}
#endif

#endif
