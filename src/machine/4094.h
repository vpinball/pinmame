/**********************************************************************************************
 *
 *   HC4094 8-Stage Shift and Store Bus Register
 *   by Steve Ellenoff
 *   08/15/2004
 *
 **********************************************************************************************/


#ifndef HC4094_H
#define HC4094_H

#define MAX_HC4094         16
#define MAX_CASCADE_HC4094 4	// 4 chips cascaded = 32 bit number

typedef struct
{
        int num;											/* total number of chips */
		mem_write_handler parallel_data_out[MAX_HC4094];	/* Parallel Data Out Callback */
		mem_write_handler qspin_data_out[MAX_HC4094];		/* QS Serial Data Out Callback */
		mem_write_handler qs1pin_data_out[MAX_HC4094];		/* QS1 Serial Data Out Callback */
} HC4094interface;

/*Externally called functions from drivers*/
void HC4094_init(HC4094interface *intf);
WRITE_HANDLER( HC4094_data_w );
WRITE_HANDLER( HC4094_oe_w );
WRITE_HANDLER( HC4094_clock_w );
WRITE_HANDLER( HC4094_strobe_w );
#endif

