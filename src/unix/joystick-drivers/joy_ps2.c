/*
 * XMame PlayStation2 controller support.  Supports standard PlayStation
 * controllers on a PlayStation2-Linux machine with /dev/ps2padxx drivers
 *
 * Written by Joan Touzet <joant@ieee.org> May 2002.
 * Release 1.0 -- "Cling Peaches"
 * 
 * TODO: * Consider allowing user to change controller mode during runtime
 *         (problematic for games expecting analog input)
 *       * Select optimal controller mode on a per-game basis
 *         (wait for 0.60 per-game mapping module to be in place)
 *       * Store the controller's pre-xmame state and restore on exit
 *       * Integrate Analog+ patches for better analog support.
 *       * Test with pure digital, analog controllers (if avail.)
 */

/* Format of data read from the PS2 Pad:
 * 32 character-sized bytes
 * Byte #	Data
 * ========================================
 *  0		0x0 = valid data, other = invalid data
 *  		  (in fact, ps2pad_read will return -EIO if data[0] != 0)
 *  		  
 *  1		Packed status data
 *                Upper byte: pad type, use PS2PAD_TYPE() to shift
 *  		  Known types:
 *  		    NEJICON   -- 0x2
 *  		    DIGITAL   -- 0x4
 *  		    ANALOG    -- 0x5
 *  		    DUALSHOCK -- 0x7
 *  		  (Lower byte * 2 + 2) = upper bound of data[] index
 *  		    For DIGITAL controllers (and DUALSHOCK controllers in
 *  		      digital mode) = 0 (only data[2] and data[3] filled)
 *		    For DUALSHOCK controllers in analog mode = 3
 *		      (data[4] through data[7] filled)
 *		      
 *  2		First half of digital button bits:
 *  		  76543210
 *  		  --------
 *  		  |||||||\__ SELECT
 *  		  ||||||\___ L3
 *  		  |||||\____ R3
 *  		  ||||\_____ START
 *  		  |||\______ UP
 *  		  ||\_______ DOWN
 *  		  |\________ LEFT
 *  		  \_________ RIGHT
 *
 *  3		Second half of digital button bits:
 *  		  76543210
 *  		  --------
 *  		  |||||||\__ L2
 *  		  ||||||\___ R2
 *  		  |||||\____ L1
 *  		  ||||\_____ R1
 *  		  |||\______ TRIANGLE
 *  		  ||\_______ CIRCLE
 *  		  |\________ CROSS
 *  		  \_________ SQUARE
 *
 * 4		(DualShock Analog Mode) Analog Right X Axis
 * 		  8 unsigned bits ( -127 for signed data)
 *
 * 5		(DualShock Analog Mode) Analog Right Y Axis
 * 		  8 unsigned bits ( -127 for signed data)
 *
 * 6		(DualShock Analog Mode) Analog Left X Axis
 * 		  8 unsigned bits ( -127 for signed data)
 *
 * 7		(DualShock Analog Mode) Analog Left Y Axis
 * 		  8 unsigned bits ( -127 for signed data)
 */

#ifdef PS2_JOYSTICK

#include "xmame.h"
#include "devices.h"
#include <sys/ioctl.h>
#include <linux/ps2/pad.h>

/* define the following for PS2 driver debugging */
#undef JDEBUG
#define BUFSIZE 128
/* name of ps2pad device prefix */
#define PS2PADDEV "/dev/ps2pad"
/* name of ps2pad status device */
#define PS2PADSTAT "/dev/ps2padstat"

/* Global definitions to make code more readable.  Taken from the actual
 * PS2 pad kernel driver source code.
 */
char *pad_type_names[16] = {
	"type 0",
	"type 1",
	"NEJICON",	/* PS2PAD_TYPE_NEJICON	*/
	"type 3",
	"DIGITAL",	/* PS2PAD_TYPE_DIGITAL	*/
	"ANALOG",	/* PS2PAD_TYPE_ANALOG	*/
	"type 6",
	"DUALSHOCK",	/* PS2PAD_TYPE_DUALSHOCK*/
	"type 8",
	"type 9",
	"type A",
	"type B",
	"type C",
	"type D",
	"type E",
	"type F",
};

char *pad_stat_names[4] = {
	"Not connected",
	"Ready",
	"Busy",
	"Error",
};

char *pad_rstat_names[4] = {
	"Complete",
	"Failed",
	"Busy",
	"UNKNOWN",
};

struct rc_option joy_ps2_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static u_char pad_data[PS2PAD_DATASIZE];
static int numpads = 0;

/* Forward declarations */
void joy_ps2_poll(void);


/* Lock/unlock the PS2 pad's mode. */
static void ps2pad_lockset (
	int fd,				/* fd pointing to pad device */
	char padtype,			/* Controller mode/type */
	int lock)			/* 0 = unlock, 1 = lock */
{
    struct ps2pad_mode padmode;
    struct ps2pad_modeinfo modeinfo;
    int rc, i, max;

    /* First, search the controller's table of possible modes for
     * a match with the requested mode.  If no match is found,
     * use the current mode.
     */
    modeinfo.term = PS2PAD_MODETABLE;
    modeinfo.offs = -1;
    rc = ioctl (fd, PS2PAD_IOCMODEINFO, &modeinfo);
    if (rc != 0)
    {
	fprintf(stderr_file, "Unexpected rc=%d from ps2 IOCMODEINFO ioctl!\n", rc);
	return;
    }
    max = modeinfo.result;
    for (i=0; i<max; i++)
    {
	modeinfo.offs = i;
	rc = ioctl (fd, PS2PAD_IOCMODEINFO, &modeinfo);
	if (rc != 0)
	{
	    fprintf(stderr_file, "Unexpected rc=%d from ps2 IOCMODEINFO ioctl!\n", rc);
	    return;
	}
	if (modeinfo.result == padtype) 
	    break;
    }

    if (i == max) {
	/* Couldn't find our mode.  Use current mode. */
	modeinfo.term = PS2PAD_MODECUROFFS;
	rc = ioctl (fd, PS2PAD_IOCMODEINFO, &modeinfo);
	i = modeinfo.result;
	if (rc != 0)
	{
	    fprintf(stderr_file, "Unexpected rc=%d from ps2 IOCMODEINFO ioctl!\n", rc);
	    return;
	}
    }
	
    /* Now lock or unlock our controller. */
    padmode.offs = i;
    /* Magic numbers used:  0, 1: maintain present lock status
     *                         2: unlock switch
     *                         3: lock switch
     */
    padmode.lock = ((lock == 1) ? 3 : 2);
    rc = ioctl (fd, PS2PAD_IOCSETMODE, &padmode);
    if (rc != 0) 
    {
	fprintf (stderr_file, "PS2 Pad could not be %slocked! rc=%d \n",
			(lock == 0) ? "un" : "", rc);
	exit(-1);
    }
    return;
}


/* Initialization routine. */
void joy_ps2_init (void)
{
    int i, j, res, tempfd;
    char devname[BUFSIZE];

    fprintf (stderr_file, "PlayStation2 pad interface initialization...\n");


    /* Check how many possible pads can be connected. */
    /* The following code has been commented out because it seems to
     * cause the PS2 to crash intermittently.  Until the bug is
     * worked out, we'll assume a maximum of 2 devices (since no
     * more are supported by the kernel anyway.
     */
    numpads = 2;
#if 0
    sprintf (devname, "%s", PS2PADSTAT);
    tempfd = open(devname, O_RDONLY|O_NONBLOCK);
    if (tempfd < 0) {
	fprintf (stderr_file, "PlayStation2 padstat failed to open!\n");
	fflush(stderr);
	return;
    }
    res = ioctl(tempfd, PS2PAD_IOCGETNPADS, &numpads);
    if (res < 0) {
	fprintf (stderr_file, "PlayStation2 padstat ioctl failed!\n");
	fflush(stderr);
	return;
    }
    close(tempfd);
#endif

    /* Now loop through each controller. */
    for (i = 0; i < numpads; i++)
    {
	sprintf (devname, "%s%1d0", PS2PADDEV, i);

	if ((joy_data[i].fd = open (devname, O_RDONLY)) >= 0)
	{
	    int ires = PS2PAD_STAT_BUSY;

	    /* We might have just woken up the pad.  Wait for READY */
	    while (ires == PS2PAD_STAT_READY)
	    {
		res = ioctl(joy_data[i].fd, PS2PAD_IOCGETSTAT, &ires);
		if (res != 0)
		{
		    fprintf(stderr_file,
			"Unexpected rc=%d from ps2 IOCGETSTAT ioctl!\n", res);
		}
	    }
	    if ((ires == PS2PAD_STAT_NOTCON) ||
			(ires == PS2PAD_STAT_ERROR))
	    {
		if (ires == PS2PAD_STAT_ERROR)
		    fprintf(stderr_file,
			"PS2 Pad #%d had PS2PAD_STAT_ERROR!\n", i);
		fprintf(stderr_file,
			"Pad data corrupt, closing fd %d\n", joy_data[i].fd);
		close(joy_data[i].fd);
		joy_data[i].fd = -1;
		continue;
	    }

	    /* Lock down the controller's mode. */
	    ps2pad_lockset(joy_data[i].fd, PS2PAD_TYPE_DUALSHOCK, 1);

	    /* Read pad data and fill in default values */
	    res = read(joy_data[i].fd, &pad_data, PS2PAD_DATASIZE);
	    if ((res == 0) || (pad_data[0] != 0))
 	    {
	 	fprintf(stderr_file,
			"Pad data corrupt, closing fd %d\n", joy_data[i].fd);
		close(joy_data[i].fd);
		joy_data[i].fd = -1;
		continue;
	    }
	    
	    switch(PS2PAD_TYPE(pad_data[1]))
	    {
		case PS2PAD_TYPE_DUALSHOCK:
			/* 4 axes, 16 buttons. */
			joy_data[i].num_axis = 4;
			joy_data[i].num_buttons = 16;
			break;
		case PS2PAD_TYPE_DIGITAL:
		default:
			/* 0 axes, 16 buttons. */
			joy_data[i].num_axis = 0;
			joy_data[i].num_buttons = 16;
	    }
	    /* Sanity check. */
	    if (joy_data[i].num_buttons > JOY_BUTTONS)
		joy_data[i].num_buttons = JOY_BUTTONS;
	    if (joy_data[i].num_axis > JOY_AXIS)
		joy_data[i].num_axis = JOY_AXIS;

	    if (PS2PAD_TYPE(pad_data[1]) == PS2PAD_TYPE_DUALSHOCK)
	    {
		joy_data[i].axis[0].center = (int)pad_data[6];
		joy_data[i].axis[1].center = (int)pad_data[7];
		joy_data[i].axis[2].center = (int)pad_data[4];
		joy_data[i].axis[3].center = (int)pad_data[5];

		/* Set min/max values to +1/-1 and let autocalibrate
		 * take care of the rest.
                 */
		for (j=0; j<joy_data[i].num_axis; j++)
		{
		    if (joy_data[i].axis[j].center == 0)
			joy_data[i].axis[j].center = 0x7f;
		    joy_data[i].axis[j].min = joy_data[i].axis[j].center - 1;
		    joy_data[i].axis[j].max = joy_data[i].axis[j].center + 1;
		}
	    }

	    fprintf (stderr_file, "PS2 pad %s is %s\n",
			    devname,
			    pad_type_names[PS2PAD_TYPE(pad_data[1])]);
	    joy_poll_func = joy_ps2_poll;
	}

    } /* for (numpads) */
}


void joy_ps2_poll (void)
{
    int i, res, ires;
    int buttons;

    for (i=0; i<numpads; i++)
    {
	/* Gracefully fail */
	if (joy_data[i].fd < 0)
	    continue;

	/* We might have just woken up the pad.  Wait for READY */
	res = ioctl(joy_data[i].fd, PS2PAD_IOCGETSTAT, &ires);
	if (res != 0)
	{
	    fprintf(stderr_file,
		"Unexpected rc=%d from ps2 IOCGETSTAT ioctl!\n", res);
	    fflush(stderr);
	    return;
	}
	if ((ires == PS2PAD_STAT_NOTCON) ||
		(ires == PS2PAD_STAT_ERROR))
	{
	    if (ires == PS2PAD_STAT_ERROR)
		fprintf(stderr_file,
			"PS2 Pad #%d had PS2PAD_STAT_ERROR!\n", i);
	    fprintf(stderr_file,
		"Pad data corrupt, closing fd %d\n", joy_data[i].fd);
	    close(joy_data[i].fd);
	    joy_data[i].fd = -1;
	    continue;
	}

	if (ires != PS2PAD_STAT_READY)
	    continue;

	res = read(joy_data[i].fd, &pad_data, PS2PAD_DATASIZE);
	if ((res == 0) || (pad_data[0] != 0))
	{
	    fprintf(stderr_file,
		"Pad data corrupt, closing fd %d\n", joy_data[i].fd);
	    close(joy_data[i].fd);
	    joy_data[i].fd = -1;
	    continue;
	}
     
	buttons = ((int)pad_data[2] << 8) | pad_data[3];

#ifdef JDEBUG
	fprintf(stderr_file, "pad: %x %02x %02x %02x %02x\n",
			buttons,
			pad_data[4], pad_data[5], pad_data[6],
			pad_data[7]);
#endif

	/* get button values */
	joy_data[i].buttons[0]  = (buttons & PS2PAD_BUTTON_SQUARE  ) ? 0 : 1;
	joy_data[i].buttons[1]  = (buttons & PS2PAD_BUTTON_TRIANGLE) ? 0 : 1;
	joy_data[i].buttons[2]  = (buttons & PS2PAD_BUTTON_CROSS   ) ? 0 : 1;
	joy_data[i].buttons[3]  = (buttons & PS2PAD_BUTTON_CIRCLE  ) ? 0 : 1;
	joy_data[i].buttons[4]  = (buttons & PS2PAD_BUTTON_L1      ) ? 0 : 1;
	joy_data[i].buttons[5]  = (buttons & PS2PAD_BUTTON_R1      ) ? 0 : 1;
	joy_data[i].buttons[6]  = (buttons & PS2PAD_BUTTON_SELECT  ) ? 0 : 1;
	joy_data[i].buttons[7]  = (buttons & PS2PAD_BUTTON_START   ) ? 0 : 1;
	joy_data[i].buttons[8]  = (buttons & PS2PAD_BUTTON_L2      ) ? 0 : 1;
	joy_data[i].buttons[9]  = (buttons & PS2PAD_BUTTON_R2      ) ? 0 : 1;
	joy_data[i].buttons[10]  = (buttons & PS2PAD_BUTTON_L3      ) ? 0 : 1;
	joy_data[i].buttons[11]  = (buttons & PS2PAD_BUTTON_R3      ) ? 0 : 1;
	joy_data[i].buttons[12]  = (buttons & PS2PAD_BUTTON_LEFT    ) ? 0 : 1;
	joy_data[i].buttons[13]  = (buttons & PS2PAD_BUTTON_RIGHT   ) ? 0 : 1;
	joy_data[i].buttons[14]  = (buttons & PS2PAD_BUTTON_UP      ) ? 0 : 1;
	joy_data[i].buttons[15]  = (buttons & PS2PAD_BUTTON_DOWN    ) ? 0 : 1;

	/* Only read analog data if the pad is in DualShock mode */
	if (PS2PAD_TYPE(pad_data[1]) == PS2PAD_TYPE_DUALSHOCK)
	{
	    joy_data[i].axis[0].val = (int) pad_data[6];
	    joy_data[i].axis[1].val = (int) pad_data[7];
	    joy_data[i].axis[2].val = (int) pad_data[4];
	    joy_data[i].axis[3].val = (int) pad_data[5];
	}

    }

    /* evaluate joystick movements */
   joy_evaluate_moves ();
}


void joy_ps2_exit()
{

   int i;
   for (i=0; i<numpads; i++)
   {
      /* A -1 as the joytype will leave the pad in its current state,
       * since it will never match a real joypad type
       */
      ps2pad_lockset(joy_data[i].fd, -1, 0);
   }
}

#endif	/* PS2_JOYSTICK */
