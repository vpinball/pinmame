/*****************************************************************************

  7474 positive-edge-triggered D-type flip-flop with preset, clear and
       complementary outputs.  There are 2 flip-flops per chips


  7474 pin layout:

	[1] /1CLR	  VCC [14]
	[2]  1D		/2CLR [13]
	[3]  1CLK	   2D [12]
	[4] /1PR	 2CLK [11]
	[5]  1Q		 /2PR [10]
	[6] /1Q		   2Q [9]
	[7]  GND	  /2Q [8]


  Truth table (logical states):

	PR	CLR	CLK D | Q  /Q
	--------------|-------
	H	L	X   X | H	L
	L   H   X   X | L   H
	H   H   X   X | H   H  (Note 1)
	L   L  _-   X | D  /D
	L   L   L   X | Q0 /Q0
	----------------------
	L	= lo (0)
	H	= hi (1)
	X	= any state
	_-	= raising edge
	Q0  = previous state

	Note 1: Non-stable configuration

*****************************************************************************/

#ifndef TTL7474_H
#define TTL7474_H

#define MAX_TTL7474 4

/* The interface structure */
struct TTL7474_interface
{
	void (*output_changed_cb)(void);
};


void TTL7474_config(int which, const struct TTL7474_interface *intf);

void TTL7474_set_inputs(int which, int clear_comp, int preset_comp, int clock, int d);
int  TTL7474_output_r(int which);
int  TTL7474_output_comp_r(int which);	/* NOT strictly the same as !TTL7474_output_r() */

#endif
