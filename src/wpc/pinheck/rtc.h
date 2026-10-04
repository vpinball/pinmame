// license:BSD-3-Clause

#ifndef PINHECK_RTC_H
#define PINHECK_RTC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ds1340 {
	uint8_t reg[10];
	int scl, sda;
	int state, bit, out, ack, ptr, sent;
	uint8_t shift;
	int64_t base;
	uint64_t ticks, ticks_per_s;
} ds1340;

void ds1340_init(ds1340 *d, int64_t unix_seconds, uint64_t ticks_per_second);
void ds1340_tick(ds1340 *d, uint64_t ticks);
int ds1340_update(ds1340 *d, int scl, int sda);

#ifdef __cplusplus
}
#endif

#endif
