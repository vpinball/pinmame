// license:BSD-3-Clause

#include "hexload.h"
#include <stdio.h>

static int nibble(uint8_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;
}

static int byte_at(const uint8_t *p)
{
	int hi = nibble(p[0]), lo = nibble(p[1]);
	return hi < 0 || lo < 0 ? -1 : hi << 4 | lo;
}

long pinheck_hex_flash(const uint8_t *hex, size_t n, uint8_t *flash, uint32_t size, uint32_t base, char *err)
{
	uint8_t rec[5 + 255];
	uint32_t upper = 0;
	size_t i = 0;
	long line = 0, written = 0;
	int k;

	while (i < n) {
		int len, sum = 0, type;
		uint32_t a;
		if (hex[i] == '\r' || hex[i] == '\n' || hex[i] == ' ' || hex[i] == '\t') { i++; continue; }
		line++;
		if (hex[i] != ':') { sprintf(err, "line %ld: no record mark", line); return -1; }
		if (i + 11 > n || (len = byte_at(hex + i + 1)) < 0) { sprintf(err, "line %ld: short record", line); return -1; }
		if (i + 11 + 2 * (size_t)len > n) { sprintf(err, "line %ld: short record", line); return -1; }
		for (k = 0; k < len + 5; k++) {
			int b = byte_at(hex + i + 1 + 2 * k);
			if (b < 0) { sprintf(err, "line %ld: not a hex digit", line); return -1; }
			rec[k] = (uint8_t)b;
			sum += b;
		}
		if (sum & 0xFF) { sprintf(err, "line %ld: checksum", line); return -1; }
		i += 11 + 2 * (size_t)len;
		type = rec[3];
		a = upper + ((uint32_t)rec[1] << 8 | rec[2]);
		if (type == 0) {
			if (a < base || a - base > size || (uint32_t)len > size - (a - base)) { sprintf(err, "line %ld: address %08lx outside flash", line, (unsigned long)a); return -1; }
			for (k = 0; k < len; k++) flash[a - base + k] = rec[4 + k];
			written += len;
		} else if (type == 1) {
			while (i < n && (hex[i] == '\r' || hex[i] == '\n' || hex[i] == ' ' || hex[i] == '\t' || hex[i] == 0x1A)) i++;
			if (i < n) { sprintf(err, "line %ld: data after the end record", line + 1); return -1; }
			return written;
		} else if (type == 4 && len == 2) {
			upper = ((uint32_t)rec[4] << 8 | rec[5]) << 16;
		} else if (type != 5 || len != 4) {
			sprintf(err, "line %ld: record type %02x not supported", line, (unsigned)type);
			return -1;
		}
	}
	sprintf(err, "no end record");
	return -1;
}
