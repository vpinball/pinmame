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
} CRTC6845;

static CRTC6845 crtc6845[MAX_6845];

void crtc6845_init(int chipnum)
{
	memset(&crtc6845[chipnum],0,sizeof(CRTC6845));
}

READ_HANDLER( crtc6845_register_r )
{
	int retval=0;

	switch(crtc6845[offset].address_latch)
	{
		case 0:
			retval=crtc6845[offset].horiz_total;
			break;
		case 1:
			retval=crtc6845[offset].horiz_disp;
			break;
		case 2:
			retval=crtc6845[offset].horiz_sync_pos;
			break;
		case 3:
			retval=crtc6845[offset].sync_width;
			break;
		case 4:
			retval=crtc6845[offset].vert_total;
			break;
		case 5:
			retval=crtc6845[offset].vert_total_adj;
			break;
		case 6:
			retval=crtc6845[offset].vert_disp;
			break;
		case 7:
			retval=crtc6845[offset].vert_sync_pos;
			break;
		case 8:
			retval=crtc6845[offset].intl_skew;
			break;
		case 9:
			retval=crtc6845[offset].max_ras_addr;
			break;
		case 10:
			retval=crtc6845[offset].cursor_start_ras;
			break;
		case 11:
			retval=crtc6845[offset].cursor_end_ras;
			break;
		case 12:
			retval=(crtc6845[offset].start_addr&0x3f)>>8;
			break;
		case 13:
			retval=crtc6845[offset].start_addr&0xff;
			break;
		case 14:
			retval=(crtc6845[offset].cursor&0x3f)>>8;
			break;
		case 15:
			retval=crtc6845[offset].cursor&0xff;
			break;
		case 16:
			retval=(crtc6845[offset].light_pen&0x3f)>>8;
			break;
		case 17:
			retval=crtc6845[offset].light_pen&0xff;
			break;
		default:
			break;
	}
        return retval;
}


WRITE_HANDLER( crtc6845_address_w )
{
	crtc6845[offset].address_latch=data&0x1f;
}


WRITE_HANDLER( crtc6845_register_w )
{

LOG(("CRT #0 PC %04x: WRITE reg 0x%02x data 0x%02x\n",activecpu_get_pc(),crtc6845[offset].address_latch,data));

	switch(crtc6845[offset].address_latch)
	{
		case 0:
			crtc6845[offset].horiz_total=data;
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
			break;
		case 5:
			crtc6845[offset].vert_total_adj=data&0x1f;
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
			break;
		case 10:
			crtc6845[offset].cursor_start_ras=data&0x7f;
			break;
		case 11:
			crtc6845[offset].cursor_end_ras=data&0x1f;
			break;
		case 12:
			crtc6845[offset].start_addr&=0x00ff;
			crtc6845[offset].start_addr|=(data&0x3f)<<8;
			crtc6845[offset].page_flip=data&0x40;
			break;
		case 13:
			crtc6845[offset].start_addr&=0xff00;
			crtc6845[offset].start_addr|=data;
			break;
		case 14:
			crtc6845[offset].cursor&=0x00ff;
			crtc6845[offset].cursor|=(data&0x3f)<<8;
			break;
		case 15:
			crtc6845[offset].cursor&=0xff00;
			crtc6845[offset].cursor|=data;
			break;
		case 16:
			crtc6845[offset].light_pen&=0x00ff;
			crtc6845[offset].light_pen|=(data&0x3f)<<8;
			break;
		case 17:
			crtc6845[offset].light_pen&=0xff00;
			crtc6845[offset].light_pen|=data;
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
