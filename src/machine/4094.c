/**********************************************************************************************
 *
 *   HC4094 8-Stage Shift and Store Bus Register
 *   by Steve Ellenoff
 *   08/15/2004
 *
 *   1 Bit Data on the Data Pin is shifted into the 8 bit shift register during 0->1 clock transition
 *   As far as I can tell, this will occur regardless of the strobe & output enable pins..
 *
 *   On the 7th Positve clock strobe, the 7th bit is shifted to the 8th and to QS serial output pin.
 *   On the 7th Negative clock strobe, the value of the new 8th bit is output to the Q'S serial output pin.
 *
 *   When Strobe is 1, each of the 8 bit shift register values is output to the 8 bit store register
 *   When Output Enable is 1, the output pins mirror the 8 bit store register.
 *
 *	 Cascading not handled... The driver using the chips, must capture the QS or QS1 output and
 *   manually cascade the 2 (or more) chips themselves..
 **********************************************************************************************/
#include "driver.h"
#include "4094.h"


#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//define LOG(x)	printf x
#else
#define LOG(x)
#endif

/**********************************************************************************************
     DECLARATIONS
***********************************************************************************************/
/**********************************************************************************************
     CONSTANTS
***********************************************************************************************/
/**********************************************************************************************
     INTERNAL DATA STRUCTURES
***********************************************************************************************/
struct HC4094Chip
{
	UINT8 shift_register;					// Internal Shift Register Value
	UINT8 store_register;					// Store Register 8 bit
	UINT8 clock;							// Status of clock pin
	UINT8 clkcount;							// Count of the clock strobes
	UINT8 strobe;							// Status of strobe pin
	UINT8 oe;								// Status of output enable
	UINT8 data;								// Status of data pin
	UINT8 poutput;							// Parrallel Output pins as 1 Byte
	UINT8 qs;								// QS Serial Output pin
	UINT8 qs1;								// Q'S Serial Output pin
	mem_write_handler parallel_data_out;	/* Parallel Data Out Callback */
	mem_write_handler qspin_data_out;		/* QS Serial Data Out Callback */
	mem_write_handler qs1pin_data_out;		/* QS1 Serial Data Out Callback */
};

static int numchips = 0;

/**********************************************************************************************
     GLOBALS
***********************************************************************************************/
static struct HC4094Chip hc4094[MAX_HC4094];		// Each Chip

/**********************************************************************************************
	Send Data To Parrallel Outputs
***********************************************************************************************/
static void send_data_to_parrallel_output(int chipnum)
{
	hc4094[chipnum].poutput = hc4094[chipnum].store_register;

	//Send the callback function the output data
	if(hc4094[chipnum].parallel_data_out)
		hc4094[chipnum].parallel_data_out(chipnum,hc4094[chipnum].poutput);
}

/**********************************************************************************************
	Send Shift Registers to Storage Registers
***********************************************************************************************/
static void shift_reg_to_storage_reg(int chipnum)
{
	hc4094[chipnum].store_register = hc4094[chipnum].shift_register;

	//If output enabled pin - also send to outputs
	if(hc4094[chipnum].oe)
		send_data_to_parrallel_output(chipnum);
}
/**********************************************************************************************
	Send Data To QS Output
***********************************************************************************************/
static void send_data_to_qs_output(int chipnum)
{
	hc4094[chipnum].qs = (hc4094[chipnum].shift_register>>7);

	//Send the callback function the output data
	if(hc4094[chipnum].qspin_data_out)
		hc4094[chipnum].qspin_data_out(chipnum,hc4094[chipnum].qs);
}

/**********************************************************************************************
	Send Data To QS1 Output
***********************************************************************************************/
static void send_data_to_qs1_output(int chipnum)
{
	hc4094[chipnum].qs1 = (hc4094[chipnum].shift_register>>7);

	//Send the callback function the output data
	if(hc4094[chipnum].qs1pin_data_out)
		hc4094[chipnum].qs1pin_data_out(chipnum,hc4094[chipnum].qs1);
}

/**********************************************************************************************
     Shift the shift register and add the data pin to the bottom bit
***********************************************************************************************/
static void shift_data(int chipnum)
{
	hc4094[chipnum].shift_register = ((hc4094[chipnum].shift_register<<1) & 0xff) | 
									 (hc4094[chipnum].data & 1);
	//Is strobe enabled?
	if(hc4094[chipnum].strobe)
		shift_reg_to_storage_reg(chipnum);
}


/**********************************************************************************************

								**** EXTERNAL FUNCTIONS ****

***********************************************************************************************/






/**********************************************************************************************
     HC4094_data_w -- writes data (1 bit) to the data pin
***********************************************************************************************/
WRITE_HANDLER( HC4094_data_w )
{
	//Safety check
	int chipnum = offset;
	if(numchips < chipnum) {
		LOG(("HC4094_DATA_W: Error trying to send data to undefined chip #%d\n",chipnum));
		return;
	}

	//Store to data pin
	hc4094[chipnum].data = data & 1;
}
/**********************************************************************************************
     HC4094_oe_w -- writes data (1 bit) to the output enable pin
***********************************************************************************************/
WRITE_HANDLER( HC4094_oe_w )
{
	//Safety check
	int chipnum = offset;
	if(numchips < chipnum) {
		LOG(("HC4094_OUTPUT_W: Error trying to send data to undefined chip #%d\n",chipnum));
		return;
	}

	//Store to output enable pin
	hc4094[chipnum].oe = data & 1;

	//If Output Enable is high - move storage register to parrallel output pins
	if(data & 1)
		send_data_to_parrallel_output(chipnum);
}

/**********************************************************************************************
     HC4094_clock_w -- writes data (1 bit) to the Clock pin
***********************************************************************************************/
WRITE_HANDLER( HC4094_clock_w )
{
	//Safety check
	int chipnum = offset;
	int strb = data & 1;
	if(numchips < chipnum) {
		LOG(("HC4094_CLOCK_W: Error trying to send data to undefined chip #%d\n",chipnum));
		return;
	}

	//Positive Transition? (0->1) - Read Data pin and shift into the shift register
	if(!hc4094[chipnum].clock && strb) {
		//Shift the data
		shift_data(chipnum);
		//7th Positive transition?
		if(hc4094[chipnum].clkcount == 7)
			send_data_to_qs_output(chipnum);
	}
	else {
		//Negative Transition?
		if(hc4094[chipnum].clock && !strb) {
			//7th Negative transition?
			if(hc4094[chipnum].clkcount == 7)
				send_data_to_qs1_output(chipnum);
		//Update count (only done after negative transition
		hc4094[chipnum].clkcount = (hc4094[chipnum].clkcount + 1) % 8;
		}
	}
	//Store to clock
	hc4094[chipnum].clock = strb;
}

/**********************************************************************************************
     HC4094_strobe_w -- writes data (1 bit) to the Strobe pin
***********************************************************************************************/
WRITE_HANDLER( HC4094_strobe_w )
{
	//Safety check
	int chipnum = offset;
	if(numchips < chipnum) {
		LOG(("HC4094_STROBE_W: Error trying to send data to undefined chip #%d\n",chipnum));
		return;
	}

	//Store to strobe pin
	hc4094[chipnum].strobe = data & 1;

	//If Strobe is 1 - store shift register to the storage register
	if(data & 1)
		shift_reg_to_storage_reg(chipnum);
}

/**********************************************************************************************
     HC4094_init -- initialize the chips
***********************************************************************************************/
void HC4094_init(HC4094interface *intf)
{
	int i, numchips;

	numchips = intf->num;

	for (i = 0; i < numchips; i++)
	{
		hc4094[i].parallel_data_out = intf->parallel_data_out[i];
		hc4094[i].qspin_data_out = intf->qspin_data_out[i];
		hc4094[i].qs1pin_data_out = intf->qs1pin_data_out[i];
	}
}
