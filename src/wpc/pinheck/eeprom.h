// license:BSD-3-Clause

#ifndef PINHECK_EEPROM_H
#define PINHECK_EEPROM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cat24m01 {
	uint8_t *mem;
	int addr_pins;
	int scl, sda;
	int state, phase, bit, out, ack;
	uint8_t shift;
	uint32_t addr;
	uint8_t page[256];
	uint32_t page_base;
	int page_count;
	uint16_t page_mask[16];
} cat24m01;

void cat24m01_init(cat24m01 *e, uint8_t *mem, int addr_pins);
int cat24m01_update(cat24m01 *e, int scl, int sda);

#ifdef __cplusplus
}
#endif

#endif
