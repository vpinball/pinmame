/**********************************************************************************************

     TMS5220 interface

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles
     Speech ROM support and a few bug fixes by R Nabet

***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "driver.h"
#include "tms5220.h"


/* the state of the streamed output */
static int stream;
static double baseclock;

/* static function prototypes */
static void tms5220_update(int ch, INT16 *buffer, int samples);



/**********************************************************************************************

     tms5220_sh_start -- allocate buffers and reset the 5220

***********************************************************************************************/

int tms5220_sh_start(const struct MachineSound *msound)
{
    const struct TMS5220interface *intf = msound->sound_interface;

    /* reset the 5220 */
    tms5220_reset();
    tms5220_set_irq(intf->irq);
    tms5220_set_ready(intf->ready);
    /* init the speech ROM handlers */
    tms5220_set_read(intf->read);
    tms5220_set_load_address(intf->load_address);
    tms5220_set_read_and_branch(intf->read_and_branch);

    baseclock = intf->baseclock;

	/* initialize a stream */
	stream = stream_init("TMS5220", intf->mixing_level, intf->baseclock/80, 0, tms5220_update);
	if (stream == -1)
		return 1;

    /* request a sound channel */
    return 0;
}



/**********************************************************************************************

     tms5220_sh_stop -- free buffers

***********************************************************************************************/

void tms5220_sh_stop(void)
{
}



/**********************************************************************************************

     tms5220_sh_update -- update the sound chip

***********************************************************************************************/

void tms5220_sh_update(void)
{
}



/**********************************************************************************************

     tms5220_data_w -- write data to the sound chip

***********************************************************************************************/

WRITE_HANDLER( tms5220_data_w )
{
    /* bring up to date first */
    stream_update(stream, 0);
    tms5220_data_write(data);
}



/**********************************************************************************************

     tms5220_status_r -- read status or data from the sound chip

***********************************************************************************************/

READ_HANDLER( tms5220_status_r )
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5220_status_read();
}



/**********************************************************************************************

     tms5220_ready_r -- return the not ready status from the sound chip

***********************************************************************************************/

int tms5220_ready_r(void)
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5220_ready_read();
}



/**********************************************************************************************

     tms5220_ready_r -- return the time in seconds until the ready line is asserted

***********************************************************************************************/

double tms5220_time_to_ready(void)
{
	double cycles;

	/* bring up to date first */
	stream_update(stream, 0);
	cycles = tms5220_cycles_to_ready();
	return cycles * 80.0 / baseclock;
}



/**********************************************************************************************

     tms5220_int_r -- return the int status from the sound chip

***********************************************************************************************/

int tms5220_int_r(void)
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5220_int_read();
}



/**********************************************************************************************

     tms5220_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void tms5220_update(int ch, INT16 *buffer, int samples)
{
	tms5220_process(buffer, samples);
}

/**********************************************************************************************

     tms5220_set_frequency -- adjusts the playback frequency

***********************************************************************************************/

void tms5220_set_frequency(double frequency)
{
	baseclock = frequency;

	if (stream != -1)
	{
		//stream_update(stream, 0); //!! not necessary as clock change only done once on startup, also leads to garbled sound for whatever reason
		stream_set_sample_rate(stream, (int)(frequency / 80 + 0.5));
	}
}

#ifdef PINMAME
void tms5220_set_reverb_filter(float delay, float force)
{
	//stream_update(stream, 0); //!!?
	mixer_set_reverb_filter(stream, delay, force);
}
#endif
