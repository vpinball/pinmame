#ifdef PINMAME
/**********************************************************************

        Motorola 6845 CRT Controller interface and emulation

        This function emulates the functionality of a single
        crtc6845.

	** REWRITTEN by Steve Ellenoff for multi-chip support (11/16/2003)

**********************************************************************/

#define MAX_6845 2

void crtc6845_init(int chipnum);
void crtc6845_set_vsync(int chipnum, double clockFreq, void (*handler)(int));

/*Direct handlers*/
READ_HANDLER( crtc6845_register_r );
WRITE_HANDLER( crtc6845_address_w );
WRITE_HANDLER( crtc6845_register_w );

//Return current video start address
int crtc6845_start_address_r(int offset);
int crtc6845_cursor_address_r(int offset);

//Return rasterization size
int crtc6845_rasterized_height_r(int offset);
int crtc6845_rasterized_width_r(int offset);

/*Convenience handlers*/
READ_HANDLER( crtc6845_register_0_r );
WRITE_HANDLER( crtc6845_address_0_w );
WRITE_HANDLER( crtc6845_register_0_w );
READ_HANDLER( crtc6845_register_1_r );
WRITE_HANDLER( crtc6845_address_1_w );
WRITE_HANDLER( crtc6845_register_1_w );







#else

//ORIGINAL MAME CODE HERE-

/**********************************************************************

        Motorola 6845 CRT Controller interface and emulation

        This function emulates the functionality of a single
        crtc6845.

**********************************************************************/


#ifdef CRTC6845_C
	int crtc6845_address_latch=0;
	int crtc6845_horiz_total=0;
	int crtc6845_horiz_disp=0;
	int crtc6845_horiz_sync_pos=0;
	int crtc6845_sync_width=0;
	int crtc6845_vert_total=0;
	int crtc6845_vert_total_adj=0;
	int crtc6845_vert_disp=0;
	int crtc6845_vert_sync_pos=0;
	int crtc6845_intl_skew=0;
	int crtc6845_max_ras_addr=0;
	int crtc6845_cursor_start_ras=0;
	int crtc6845_cursor_end_ras=0;
	int crtc6845_start_addr=0;
	int crtc6845_cursor=0;
	int crtc6845_light_pen=0;
	int crtc6845_page_flip=0;		/* This seems to be present in the HD46505 */

#else
	extern int crtc6845_address_latch;
	extern int crtc6845_horiz_total;
	extern int crtc6845_horiz_disp;
	extern int crtc6845_horiz_sync_pos;
	extern int crtc6845_sync_width;
	extern int crtc6845_vert_total;
	extern int crtc6845_vert_total_adj;
	extern int crtc6845_vert_disp;
	extern int crtc6845_vert_sync_pos;
	extern int crtc6845_intl_skew;
	extern int crtc6845_max_ras_addr;
	extern int crtc6845_cursor_start_ras;
	extern int crtc6845_cursor_end_ras;
	extern int crtc6845_start_addr;
	extern int crtc6845_cursor;
	extern int crtc6845_light_pen;
	extern int crtc6845_page_flip;
#endif

READ_HANDLER( crtc6845_register_r );
WRITE_HANDLER( crtc6845_address_w );
WRITE_HANDLER( crtc6845_register_w );

#endif
