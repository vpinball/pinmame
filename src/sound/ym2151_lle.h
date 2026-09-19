#pragma once

/**********************************************
	YM2151-LLE glue - internal interface, used by 2151intf.c only.

	The public YM2151 API (struct YM2151interface and the port handlers) is in
	2151intf.h and is the same whichever core is compiled in.

	*** The core this wraps, fmopm.c/.h, is GPLv2. Enabling HAS_YM2151_LLE makes
	*** the resulting binary GPLv2. See src/sound/fmopm-LICENSE.txt
**********************************************/

void  ym2151lle_init(int num, int is_ym2164);
void  ym2151lle_reset(int num);
void  ym2151lle_set_handlers(int num, void (*irqhandler)(int irq), mem_write_handler porthandler);
void  ym2151lle_write(int num, UINT8 a0, UINT8 data);
UINT8 ym2151lle_read_status(int num);
void  ym2151lle_generate(int num, INT16 **buffers, int length);
