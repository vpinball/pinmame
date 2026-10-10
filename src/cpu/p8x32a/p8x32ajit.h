// license:BSD-3-Clause

#pragma once

#include "p8x32a.h"

#ifdef __cplusplus
extern "C" {
#endif

/* x86-64 translation of local instruction runs; p8x32a_jit_new returns NULL where it is not available */
void *p8x32a_jit_new(void);
void p8x32a_jit_free(void *jit);
p8x32a_jblk *p8x32a_jit_build(void *jit, p8x32a_jblk *old, unsigned a, uint32_t ix, const uint32_t *ram, const uint32_t *var, int outa);

#ifdef __cplusplus
}
#endif
