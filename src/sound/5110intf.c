/******************************************************************************

     TMS5110 interface

     slightly modified from 5220intf by Jarek Burczynski

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "driver.h"
#include "tms5110.h"


/* the state of the streamed output */
static int stream;

/* static function prototypes */
static void tms5110_update(int ch, INT16 *buffer, int samples);



/******************************************************************************

     tms5110_sh_start -- allocate buffers and reset the 5110

******************************************************************************/

int tms5110_sh_start(const struct MachineSound *msound)
{
    const struct TMS5110interface *intf = msound->sound_interface;

    if (intf->M0_callback==NULL)
    {
        logerror("\n file: 5110intf.c, tms5110_sh_start(), line 53:\n  Missing _mandatory_ 'M0_callback' function pointer in the TMS5110 interface\n  This function is used by TMS5110 to call for a single bits\n  needed to generate the speech\n  Aborting startup...\n");
        return 1;
    }
    tms5110_set_M0_callback( intf->M0_callback );

    /* reset the 5110 */
    tms5110_reset();

	/* initialize a stream */
	stream = stream_init("TMS5110", intf->mixing_level, intf->baseclock/80., 0, tms5110_update);
	if (stream == -1)
		return 1;

    /* request a sound channel */
    return 0;
}



/******************************************************************************

     tms5110_sh_stop -- free buffers

******************************************************************************/

void tms5110_sh_stop(void)
{
}



/******************************************************************************

     tms5110_sh_update -- update the sound chip

******************************************************************************/

void tms5110_sh_update(void)
{
}



/******************************************************************************

     tms5110_CTL_w -- write Control Command to the sound chip
commands like Speech, Reset, etc., are loaded into the chip via the CTL pins

******************************************************************************/

WRITE_HANDLER( tms5110_CTL_w )
{
    /* bring up to date first */
    stream_update(stream, 0);
    tms5110_CTL_set(data);
}

/******************************************************************************

     tms5110_PDC_w -- write to PDC pin on the sound chip

******************************************************************************/

WRITE_HANDLER( tms5110_PDC_w )
{
    /* bring up to date first */
    stream_update(stream, 0);
    tms5110_PDC_set(data);
}



/******************************************************************************

     tms5110_status_r -- read status from the sound chip

******************************************************************************/

READ_HANDLER( tms5110_status_r )
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5110_status_read();
}



/******************************************************************************

     tms5110_ready_r -- return the not ready status from the sound chip

******************************************************************************/

int tms5110_ready_r(void)
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5110_ready_read();
}



/******************************************************************************

     tms5110_update -- update the sound chip so that it is in sync with CPU execution

******************************************************************************/

static void tms5110_update(int ch, INT16 *buffer, int samples)
{
	tms5110_process(buffer, samples);
}



/******************************************************************************

     tms5110_set_frequency -- adjusts the playback frequency

******************************************************************************/

void tms5110_set_frequency(double frequency)
{
	if (stream != -1)
	{
		stream_update(stream, 0);
		stream_set_sample_rate(stream, frequency/80.);
	}
}
