/*
** File: tms9928a.h -- software implementation of the TMS9928A VDP.
**
** By Sean Young 1999 (sean@msxnet.org).
**
** MODIFIED: by Steve Ellenoff for multi-chip support (01/24/2002)
*/

#define MAX_VDP 2

#define TMS9928A_PALETTE_SIZE           32
#define TMS9928A_COLORTABLE_SIZE        32

/*
** The different models
*/

#define TMS99x8A	(1)
#define TMS99x8		(2)

/*
** The init, reset and shutdown functions
*/
int TMS9928A_start(int which, int model, unsigned int vram);
void TMS9928A_reset(int which);
void TMS9928A_stop(int num_chips);
extern PALETTE_INIT(TMS9928A);

/*
** The I/O functions
*/
int TMS9928A_vram_r(int which,int offset);
void TMS9928A_vram_w(int which,int offset,int data);
int TMS9928A_register_r(int which,int offset);
void TMS9928A_register_w(int which,int offset,int data);

READ_HANDLER (TMS9928A_vram_0_r);
WRITE_HANDLER (TMS9928A_vram_0_w);
READ_HANDLER (TMS9928A_register_0_r);
WRITE_HANDLER (TMS9928A_register_0_w);
READ_HANDLER (TMS9928A_vram_1_r);
WRITE_HANDLER (TMS9928A_vram_1_w);
READ_HANDLER (TMS9928A_register_1_r);
WRITE_HANDLER (TMS9928A_register_1_w);

/*
** Call this function to render the screen.
*/
void TMS9928A_refresh(int num_chips, struct mame_bitmap *, int full_refresh);
void TMS9928A_refresh_test(int num_chips, struct mame_bitmap *, int full_refresh);

/*
** This next function must be called 50 or 60 times per second,
** to generate the necessary interrupts
*/
int TMS9928A_interrupt (int which);

/*
** The parameter is a function pointer. This function is called whenever
** the state of the INT output of the TMS9918A changes.
*/
void TMS9928A_int_callback (int which, void (*callback)(int));

/*
** Set display of illegal sprites on or off
*/
void TMS9928A_set_spriteslimit (int which, int);

/*
** After loading a state, call this function
*/
void TMS9928A_post_load (int which);


