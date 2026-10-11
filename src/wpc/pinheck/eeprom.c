// license:BSD-3-Clause

#include "eeprom.h"
#include <string.h>

enum { IDLE, CTRL, ADDR_HI, ADDR_LO, WRITE, READ, IGNORE };
enum { DATA, ACK_NEXT, ACK_OUT, MACK };

#define SIZE 0x20000u

void cat24m01_init(cat24m01 *e, uint8_t *mem, int addr_pins)
{
	memset(e, 0, sizeof(*e));
	e->mem = mem;
	e->addr_pins = addr_pins & 3;
	e->scl = e->sda = e->out = 1;
}

static void commit(cat24m01 *e)
{
	int i;
	for (i = 0; i < 256; i++)
		if (e->page_mask[i >> 4] & (1u << (i & 15)))
			e->mem[e->page_base | (uint32_t)i] = e->page[i];
	memset(e->page_mask, 0, sizeof(e->page_mask));
	e->page_count = 0;
}

static void put_bit(cat24m01 *e)
{
	e->out = (e->mem[e->addr] >> (7 - e->bit)) & 1;
	e->bit++;
}

static void byte_in(cat24m01 *e)
{
	uint8_t b = e->shift;
	e->ack = 1;
	switch (e->state) {
	case CTRL:
		if ((b >> 4) != 0xA || ((b >> 2) & 3) != e->addr_pins) { e->ack = 0; e->state = IGNORE; break; }
		e->addr = (e->addr & 0xFFFFu) | ((uint32_t)(b >> 1) & 1u) << 16;
		e->state = (b & 1) ? READ : ADDR_HI;
		break;
	case ADDR_HI:
		e->addr = (e->addr & 0x100FFu) | (uint32_t)b << 8;
		e->state = ADDR_LO;
		break;
	case ADDR_LO:
		e->addr = (e->addr & 0x1FF00u) | b;
		e->page_base = e->addr & 0x1FF00u;
		e->state = WRITE;
		break;
	case WRITE:
		e->page[e->addr & 0xFF] = b;
		e->page_mask[(e->addr & 0xFF) >> 4] |= (uint16_t)(1u << (e->addr & 15));
		e->page_count++;
		e->addr = e->page_base | ((e->addr + 1) & 0xFF);
		break;
	}
}

int cat24m01_update(cat24m01 *e, int scl, int sda)
{
	scl = scl != 0;
	sda = sda != 0;
	if (scl && e->scl && sda != e->sda) {
		if (!sda) {
			memset(e->page_mask, 0, sizeof(e->page_mask));
			e->page_count = 0;
			e->state = CTRL;
		} else {
			if (e->state == WRITE && e->page_count) commit(e);
			e->state = IDLE;
		}
		e->phase = DATA;
		e->bit = 0;
		e->shift = 0;
		e->out = 1;
	} else if (scl && !e->scl) {
		if (e->state == READ) {
			if (e->phase == MACK) {
				e->addr = (e->addr + 1) & (SIZE - 1);
				if (sda) e->state = IGNORE;
			}
		} else if (e->state != IDLE && e->state != IGNORE && e->phase == DATA) {
			e->shift = (uint8_t)((e->shift << 1) | sda);
			if (++e->bit == 8) {
				byte_in(e);
				e->phase = ACK_NEXT;
			}
		}
	} else if (!scl && e->scl) {
		switch (e->phase) {
		case ACK_NEXT:
			e->phase = ACK_OUT;
			e->out = e->ack ? 0 : 1;
			break;
		case ACK_OUT:
			e->phase = DATA;
			e->bit = 0;
			e->shift = 0;
			e->out = 1;
			if (e->state == READ) put_bit(e);
			break;
		case DATA:
			if (e->state != READ) break;
			if (e->bit < 8) put_bit(e);
			else { e->out = 1; e->phase = MACK; }
			break;
		case MACK:
			if (e->state == READ) { e->phase = DATA; e->bit = 0; put_bit(e); }
			else e->out = 1;
			break;
		}
	}
	e->scl = scl;
	e->sda = sda;
	return e->out;
}
