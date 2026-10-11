// license:BSD-3-Clause

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Intel HEX (records 00, 01, 04, 05) into a flash image of size bytes at physical address base, with checksums
   and bounds checked. Returns the data bytes written, or -1 with the reason in err (at least 80 bytes). */
int pinheck_hex_flash(const uint8_t *hex, size_t n, uint8_t *flash, uint32_t size, uint32_t base, char *err);

#ifdef __cplusplus
}
#endif
