#ifdef PINMAME
/**********************************************************************

        Motorola 6845 CRT Controller interface and emulation

        This function emulates the functionality of a single
        crtc6845.

        This is just a storage shell for crtc6845 variables due
        to the fact that the hardware implementation makes a big
        difference and that is done by the specific vidhrdw code.

	** REWRITTEN by Steve Ellenoff for multi-chip support (11/16/2003)


**********************************************************************/
#include "driver.h"
#include "crtc6845.h"

#ifdef VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

typedef struct {
	int address_latch;
	int horiz_total;
	int horiz_disp;
	int horiz_sync_pos;
	int sync_width;
	int vert_total;
	int vert_total_adj;
	int vert_disp;
	int vert_sync_pos;
	int intl_skew;
	int max_ras_addr;
	int cursor_start_ras;
	int cursor_end_ras;
	int start_addr;
	int cursor;
	int light_pen;
	int page_flip;		/* This seems to be present in the HD46505 */
	mame_timer* vsync_timer;
	double clock_freq;
} CRTC6845;

static CRTC6845 crtc6845[MAX_6845];

void crtc6845_init(int chipnum)
{
	memset(&crtc6845[chipnum],0,sizeof(CRTC6845));
}

void update_vsync_timer(int chipnum)
{
	if (crtc6845[chipnum].vsync_timer)
	{
		// Each scanline is (horiz_total+1) clock cycles
		// VSync happens after (vert_total+1) rows of characters + vert_total_adj scanlines, each character is made of (max_ras_addr+1) scanlines
		int nCycles = (crtc6845[chipnum].horiz_total + 1) * ((crtc6845[chipnum].vert_total + 1) * (crtc6845[chipnum].max_ras_addr + 1) + crtc6845[chipnum].vert_total_adj);
		double period = nCycles / crtc6845[chipnum].clock_freq;
		if (period < 1e-3) // Hack: disable timer if values are too low (less than 1ms here)
			timer_enable(crtc6845[chipnum].vsync_timer, 0);
		else
			timer_adjust(crtc6845[chipnum].vsync_timer, period, chipnum, period);
		//if (period > 1e-3) printf("CRT #%d PC %04x: VSync freq:%5.1fHz\n", chipnum, activecpu_get_pc(), 1.0 / period);
	}
}

void crtc6845_set_vsync(int chipnum, double clockFreq, void (*handler)(int))
{
	if (crtc6845[chipnum].vsync_timer != NULL)
		timer_remove(crtc6845[chipnum].vsync_timer);
	crtc6845[chipnum].vsync_timer = NULL;
	crtc6845[chipnum].clock_freq = clockFreq;
	if (handler && clockFreq > 0.)
	{
		crtc6845[chipnum].vsync_timer = timer_alloc(handler);
		update_vsync_timer(chipnum);
	}
}

READ_HANDLER( crtc6845_register_r )
{
	int retval=0;

	// Most 6845 registers are write-only, except R14/R15 (cursor) and R16/R17 (lightpen).

	switch(crtc6845[offset].address_latch)
	{
		case 0:
			// retval=crtc6845[offset].horiz_total;                  // write-only register
			break;
		case 1:
			// retval=crtc6845[offset].horiz_disp;                   // write-only register
			break;
		case 2:
			// retval=crtc6845[offset].horiz_sync_pos;               // write-only register
			break;
		case 3:
			// retval=crtc6845[offset].sync_width;                   // write-only register
			break;
		case 4:
			// retval=crtc6845[offset].vert_total;                   // write-only register
			break;
		case 5:
			// retval=crtc6845[offset].vert_total_adj;               // write-only register
			break;
		case 6:
			// retval=crtc6845[offset].vert_disp;                    // write-only register
			break;
		case 7:
			// retval=crtc6845[offset].vert_sync_pos;                // write-only register
			break;
		case 8:
			// retval=crtc6845[offset].intl_skew;                    // write-only register
			break;
		case 9:
			// retval=crtc6845[offset].max_ras_addr;                 // write-only register
			break;
		case 10:
			// retval=crtc6845[offset].cursor_start_ras;             // write-only register
			break;
		case 11:
			// retval=crtc6845[offset].cursor_end_ras;               // write-only register
			break;
		case 12:
			// retval=(crtc6845[offset].start_addr >> 8) & 0x003f;   // write-only register
			break;
		case 13:
			// retval=crtc6845[offset].start_addr&0xff;              // write-only register
			break;
		case 14:
			retval=(crtc6845[offset].cursor >> 8) & 0x003f;
			break;
		case 15:
			retval=crtc6845[offset].cursor&0xff;
			break;
		case 16:
			retval=(crtc6845[offset].light_pen >> 8) & 0x003f;
			break;
		case 17:
			retval=crtc6845[offset].light_pen&0xff;
			break;
		default:
			break;
	}
	LOG(("%8.5f CRT #0 PC %04x: READ reg 0x%02x data 0x%02x\n", timer_get_time(), activecpu_get_pc(), crtc6845[offset].address_latch, retval));
	return retval;
}


WRITE_HANDLER( crtc6845_address_w )
{
	crtc6845[offset].address_latch=data&0x1f;
}


WRITE_HANDLER( crtc6845_register_w )
{
	LOG(("%8.5f CRT #0 PC %04x: WRITE reg 0x%02x data 0x%02x\n",timer_get_time(),activecpu_get_pc(),crtc6845[offset].address_latch,data));

	// Most 6845 registers can be written, except R16/R17 (lightpen) that are read-only.

	switch(crtc6845[offset].address_latch)
	{
		case 0:
			crtc6845[offset].horiz_total=data;
			update_vsync_timer(offset);
			break;
		case 1:
			crtc6845[offset].horiz_disp=data;
			break;
		case 2:
			crtc6845[offset].horiz_sync_pos=data;
			break;
		case 3:
			crtc6845[offset].sync_width=data;
			break;
		case 4:
			crtc6845[offset].vert_total=data&0x7f;
			update_vsync_timer(offset);
			break;
		case 5:
			crtc6845[offset].vert_total_adj=data&0x1f;
			update_vsync_timer(offset);
			break;
		case 6:
			crtc6845[offset].vert_disp=data&0x7f;
			break;
		case 7:
			crtc6845[offset].vert_sync_pos=data&0x7f;
			break;
		case 8:
			crtc6845[offset].intl_skew=data;
			break;
		case 9:
			crtc6845[offset].max_ras_addr=data&0x1f;
			update_vsync_timer(offset);
			break;
		case 10:
			crtc6845[offset].cursor_start_ras=data&0x7f;
			break;
		case 11:
			crtc6845[offset].cursor_end_ras=data&0x1f;
			break;
		case 12:
			crtc6845[offset].start_addr = ((data & 0x003f) << 8) | (crtc6845[offset].start_addr & 0x00ff);
			crtc6845[offset].page_flip = data&0x40;
			break;
		case 13:
			crtc6845[offset].start_addr = (crtc6845[offset].start_addr & 0xff00) | data;
			break;
		case 14:
			crtc6845[offset].cursor = ((data & 0x003f) << 8) | (crtc6845[offset].cursor & 0x00ff);
			break;
		case 15:
			crtc6845[offset].cursor = (crtc6845[offset].cursor & 0xff00) | data;
			break;
		case 16:
			// crtc6845[offset].light_pen = ((data & 0x003f) << 8) | (crtc6845[offset].light_pen & 0x00ff);  // read-only register
			break;
		case 17:
			// crtc6845[offset].light_pen = (crtc6845[offset].light_pen & 0xff00) | data;                    // read-only register
			break;
		default:
			break;
	}
}

READ_HANDLER( crtc6845_register_0_r )
{
	return crtc6845_register_r(0);
}
WRITE_HANDLER( crtc6845_address_0_w )
{
	crtc6845_address_w(0,data);
}
WRITE_HANDLER( crtc6845_register_0_w )
{
	crtc6845_register_w(0,data);
}
READ_HANDLER( crtc6845_register_1_r )
{
	return crtc6845_register_r(1);
}
WRITE_HANDLER( crtc6845_address_1_w )
{
	crtc6845_address_w(1,data);
}
WRITE_HANDLER( crtc6845_register_1_w )
{
	crtc6845_register_w(1,data);
}

//Return current video start address
int crtc6845_start_address_r(int offset)
{
	return crtc6845[offset].start_addr;
}
int crtc6845_cursor_address_r(int offset)
{
	return crtc6845[offset].cursor;
}

//Return rasterization size
int crtc6845_rasterized_height_r(int offset)
{
	// height in scanlines is the number of displayed character line * number of scanlines per character
	return crtc6845[offset].vert_disp * (crtc6845[offset].max_ras_addr + 1);
}

int crtc6845_rasterized_width_r(int offset)
{
	return crtc6845[offset].horiz_disp;
}





#else

//ORIGINAL MAME CODE HERE-

/**********************************************************************

        Motorola 6845 CRT Controller interface and emulation

        This function emulates the functionality of a single
        crtc6845.

        This is just a storage shell for crtc6845 variables due
        to the fact that the hardware implementation makes a big
        difference and that is done by the specific vidhrdw code.

**********************************************************************/

#define CRTC6845_C

#include "driver.h"
#include "crtc6845.h"


READ_HANDLER( crtc6845_register_r )
{
	int retval=0;

	switch(crtc6845_address_latch)
	{
		case 0:
			retval=crtc6845_horiz_total;
			break;
		case 1:
			retval=crtc6845_horiz_disp;
			break;
		case 2:
			retval=crtc6845_horiz_sync_pos;
			break;
		case 3:
			retval=crtc6845_sync_width;
			break;
		case 4:
			retval=crtc6845_vert_total;
			break;
		case 5:
			retval=crtc6845_vert_total_adj;
			break;
		case 6:
			retval=crtc6845_vert_disp;
			break;
		case 7:
			retval=crtc6845_vert_sync_pos;
			break;
		case 8:
			retval=crtc6845_intl_skew;
			break;
		case 9:
			retval=crtc6845_max_ras_addr;
			break;
		case 10:
			retval=crtc6845_cursor_start_ras;
			break;
		case 11:
			retval=crtc6845_cursor_end_ras;
			break;
		case 12:
			retval=(crtc6845_start_addr&0x3f)>>8;
			break;
		case 13:
			retval=crtc6845_start_addr&0xff;
			break;
		case 14:
			retval=(crtc6845_cursor&0x3f)>>8;
			break;
		case 15:
			retval=crtc6845_cursor&0xff;
			break;
		case 16:
			retval=(crtc6845_light_pen&0x3f)>>8;
			break;
		case 17:
			retval=crtc6845_light_pen&0xff;
			break;
		default:
			break;
	}
        return retval;
}


WRITE_HANDLER( crtc6845_address_w )
{
	crtc6845_address_latch=data&0x1f;
}


WRITE_HANDLER( crtc6845_register_w )
{

LOG(("CRT #0 PC %04x: WRITE reg 0x%02x data 0x%02x\n",activecpu_get_pc(),crtc6845_address_latch,data));

	switch(crtc6845_address_latch)
	{
		case 0:
			crtc6845_horiz_total=data;
			break;
		case 1:
			crtc6845_horiz_disp=data;
			break;
		case 2:
			crtc6845_horiz_sync_pos=data;
			break;
		case 3:
			crtc6845_sync_width=data;
			break;
		case 4:
			crtc6845_vert_total=data&0x7f;
			break;
		case 5:
			crtc6845_vert_total_adj=data&0x1f;
			break;
		case 6:
			crtc6845_vert_disp=data&0x7f;
			break;
		case 7:
			crtc6845_vert_sync_pos=data&0x7f;
			break;
		case 8:
			crtc6845_intl_skew=data;
			break;
		case 9:
			crtc6845_max_ras_addr=data&0x1f;
			break;
		case 10:
			crtc6845_cursor_start_ras=data&0x7f;
			break;
		case 11:
			crtc6845_cursor_end_ras=data&0x1f;
			break;
		case 12:
			crtc6845_start_addr&=0x00ff;
			crtc6845_start_addr|=(data&0x3f)<<8;
			crtc6845_page_flip=data&0x40;
			break;
		case 13:
			crtc6845_start_addr&=0xff00;
			crtc6845_start_addr|=data;
			break;
		case 14:
			crtc6845_cursor&=0x00ff;
			crtc6845_cursor|=(data&0x3f)<<8;
			break;
		case 15:
			crtc6845_cursor&=0xff00;
			crtc6845_cursor|=data;
			break;
		case 16:
			crtc6845_light_pen&=0x00ff;
			crtc6845_light_pen|=(data&0x3f)<<8;
			break;
		case 17:
			crtc6845_light_pen&=0xff00;
			crtc6845_light_pen|=data;
			break;
		default:
			break;
	}
}

#endif
