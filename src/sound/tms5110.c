/**********************************************************************************************

     TMS5110 simulator (modified from TMS5220 by Jarek Burczynski)

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

***********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5110.h"


/* Pull in the ROM tables */
#include "tms5110r.c"


/* these contain data that describes the 64 bits FIFO */
#define FIFO_SIZE 64
static UINT8 fifo[FIFO_SIZE];
static UINT8 fifo_head;
static UINT8 fifo_tail;
static UINT8 fifo_count;

/* these contain global status bits */
static UINT8 PDC;
static UINT8 CTL_pins;
static UINT8 speaking_now;
static UINT8 speak_delay_frames;
static UINT8 talk_status;


static int (*M0_callback)(void);

/* these contain data describing the current and previous voice frames */
static UINT16 old_energy;
static UINT16 old_pitch;
static int old_k[10];

static UINT16 new_energy;
static UINT16 new_pitch;
static int new_k[10];


/* these are all used to contain the current state of the sound generation */
static UINT16 current_energy;
static UINT16 current_pitch;
static int current_k[10];

static UINT16 target_energy;
static UINT16 target_pitch;
static int target_k[10];

static UINT8 interp_count;       /* number of interp periods (0-7) */
static UINT8 sample_count;       /* sample number within interp (0-24) */
static int pitch_count;

static int u[11];
static int x[11];

static INT32 RNG;	/* the random noise generator configuration is: 1 + x + x^3 + x^4 + x^13 */


/* Static function prototypes */
static void parse_frame(void);


#define DEBUG_5110	0


/**********************************************************************************************

     tms5110_reset -- resets the TMS5110

***********************************************************************************************/

void tms5110_reset(void)
{
    /* initialize the FIFO */
    memset(fifo, 0, sizeof(fifo));
    fifo_head = fifo_tail = fifo_count = 0;

    /* initialize the chip state */
    speaking_now = speak_delay_frames = talk_status = 0;
    CTL_pins = 0;

    /* initialize the energy/pitch/k states */
    old_energy = new_energy = current_energy = target_energy = 0;
    old_pitch = new_pitch = current_pitch = target_pitch = 0;
    memset(old_k, 0, sizeof(old_k));
    memset(new_k, 0, sizeof(new_k));
    memset(current_k, 0, sizeof(current_k));
    memset(target_k, 0, sizeof(target_k));

    /* initialize the sample generators */
    interp_count = sample_count = pitch_count = 0;
    RNG = 0x1fff;
    PDC = 0;
    
    memset(u, 0, sizeof(u));
    memset(x, 0, sizeof(x));
}



/******************************************************************************************

     tms5110_set_M0_callback -- set M0 callback for the TMS5110

******************************************************************************************/

void tms5110_set_M0_callback(int (*func)(void))
{
    M0_callback = func;
}


/******************************************************************************************

     FIFO_data_write -- handle bit data write to the TMS5110 (as a result of toggling M0 pin)

******************************************************************************************/
static void FIFO_data_write(int data)
{
	/* add this byte to the FIFO */
	if (fifo_count < FIFO_SIZE)
	{
		fifo[fifo_tail] = (data&1); /* set bit to 1 or 0 */

		fifo_tail = (fifo_tail + 1) % FIFO_SIZE;
		fifo_count++;

		if (DEBUG_5110) logerror("Added bit to FIFO (size=%2d)\n", fifo_count);
	}
	else
	{
		if (DEBUG_5110) logerror("Ran out of room in the FIFO!\n");
	}
}

/******************************************************************************************

     extract_bits -- extract a specific number of bits from the FIFO

******************************************************************************************/

static int extract_bits(int count)
{
    int val = 0;

    while (count--)
    {
        val = (val << 1) | (fifo[fifo_head] & 1);
        fifo_count--;
        fifo_head = (fifo_head + 1) % FIFO_SIZE;
    }
    return val;
}

static void request_bits(int no)
{
int i;
	for (i=0; i<no; i++)
	{
		if (M0_callback)
		{
			int data = (*M0_callback)();
			FIFO_data_write(data);
		}
		else
			if (DEBUG_5110) logerror("-->ERROR: TMS5110 missing M0 callback function\n");
	}
}

/*static void perform_dummy_read(void)
{
	if (M0_callback)
	{
		int data = (*M0_callback)();
		if (DEBUG_5110) logerror("TMS5110 performing dummy read; value read = %1i\n", data&1);
	}
	else
		if (DEBUG_5110) logerror("-->ERROR: TMS5110 missing M0 callback function\n");
}*/

/**********************************************************************************************

     tms5110_status_read -- read status from the TMS5110

        bit 0 = TS - Talk Status is active (high) when the VSP is processing speech data.
                Talk Status goes active at the initiation of a SPEAK command.
                It goes inactive (low) when the stop code (Energy=1111) is processed, or
                immediately(?????? not TMS5110) by a RESET command.
		TMS5110 datasheets mention this is only available as a result of executing
                TEST TALK command.


***********************************************************************************************/

int tms5110_status_read(void)
{

    if (DEBUG_5110) logerror("Status read: TS=%d\n", talk_status);

    return (talk_status << 0); /*CTL1 = still talking ? */
}



/**********************************************************************************************

     tms5110_ready_read -- returns the ready state of the TMS5110

***********************************************************************************************/

int tms5110_ready_read(void)
{
    return (fifo_count < FIFO_SIZE-1);
}



/**********************************************************************************************

     tms5110_process -- fill the buffer with a specific number of samples

***********************************************************************************************/

void tms5110_process(INT16 *buffer, unsigned int size)
{
  int buf_count=0;
  int i, interp_period, bitout;
  INT16 Y11, cliptemp;

/* tryagain: */

  /* if we're not speaking, fill with nothingness */
  if (!speaking_now)
    goto empty;

  /* if we're to speak, but haven't started */
  if (!talk_status)
  {

	/* a "dummy read" is mentioned in the tms5200 datasheet */
	/* The Bagman speech roms data are organized in such a way that
    ** the bit at address 0 is NOT a speech data. The bit at address 1
    ** is the speech data. It seems that the tms5110 performs a dummy read
    ** just before it executes a SPEAK command.
    ** This has been moved to command logic ...
    **  perform_dummy_read(tms);
    */

		/* clear out the new frame parameters (it will become old frame just before the first call to parse_frame() ) */
		new_energy = 0;
		new_pitch = 0;
		for (i = 0; i < 10; i++)
			new_k[i] = 0;

		talk_status = 1;
  }

  /* loop until the buffer is full or we've stopped speaking */
  while ((size > 0) && speaking_now)
  {
    int current_val;

		/* if we're ready for a new frame */
		if ((interp_count == 0) && (sample_count == 0))
		{

			/* remember previous frame */
			old_energy = new_energy;
			old_pitch = new_pitch;
			for (i = 0; i < 10; i++)
				old_k[i] = new_k[i];


			/* if the old frame was a stop frame, exit and do not process any more frames */
			if (old_energy == COEFF_ENERGY_SENTINEL)
			{
				if (DEBUG_5110) logerror("processing frame: stop frame\n");
				target_energy = current_energy = 0;
				speaking_now = talk_status = 0;
				interp_count = sample_count = pitch_count = 0;
				goto empty;
			}


			/* Parse a new frame into the new_energy, new_pitch and new_k[] */
			parse_frame();


			/* Set old target as new start of frame */
			current_energy = old_energy;
			current_pitch = old_pitch;

			for (i = 0; i < 10; i++)
				current_k[i] = old_k[i];


			/* is this the stop (ramp down) frame? */
			if (new_energy == COEFF_ENERGY_SENTINEL)
			{
				/*logerror("processing frame: ramp down\n");*/
				target_energy = 0;
				target_pitch = old_pitch;
				for (i = 0; i < 10; i++)
					target_k[i] = old_k[i];
			}
			else if ((old_energy == 0) && (new_energy != 0)) /* was the old frame a zero-energy frame? */
			{
				/* if so, and if the new frame is non-zero energy frame then the new parameters
                   should become our current and target parameters immediately,
                   i.e. we should NOT interpolate them slowly in.
                */

				/*logerror("processing non-zero energy frame after zero-energy frame\n");*/
				target_energy = new_energy;
				target_pitch = current_pitch = new_pitch;
				for (i = 0; i < 10; i++)
					target_k[i] = current_k[i] = new_k[i];
			}
			else if ((old_pitch == 0) && (new_pitch != 0))	/* is this a change from unvoiced to voiced frame ? */
			{
				/* if so, then the new parameters should become our current and target parameters immediately,
                   i.e. we should NOT interpolate them slowly in.
                */
				/*if (DEBUG_5110) logerror("processing frame: UNVOICED->VOICED frame change\n");*/
				target_energy = new_energy;
				target_pitch = current_pitch = new_pitch;
				for (i = 0; i < 10; i++)
					target_k[i] = current_k[i] = new_k[i];
			}
			else if ((old_pitch != 0) && (new_pitch == 0))	/* is this a change from voiced to unvoiced frame ? */
			{
				/* if so, then the new parameters should become our current and target parameters immediately,
                   i.e. we should NOT interpolate them slowly in.
                */
				/*if (DEBUG_5110) logerror("processing frame: VOICED->UNVOICED frame change\n");*/
				target_energy = new_energy;
				target_pitch = current_pitch = new_pitch;
				for (i = 0; i < 10; i++)
					target_k[i] = current_k[i] = new_k[i];
			}
			else
			{
				/*logerror("processing frame: Normal\n");*/
				/*logerror("*** Energy = %d\n",current_energy);*/
				/*logerror("proc: %d %d\n",last_fbuf_head,fbuf_head);*/

				target_energy = new_energy;
				target_pitch = new_pitch;
				for (i = 0; i < 10; i++)
					target_k[i] = new_k[i];
			}
		}
    else
    {
			interp_period = sample_count / 25;
			switch(interp_count)
			{
				/*         PC=X  X cycle, rendering change (change for next cycle which chip is actually doing) */
				case 0: /* PC=0, A cycle, nothing happens (calc energy) */
				break;
				case 1: /* PC=0, B cycle, nothing happens (update energy) */
				break;
				case 2: /* PC=1, A cycle, update energy (calc pitch) */
				current_energy += ((target_energy - current_energy) >> interp_coeff[interp_period]);
				break;
				case 3: /* PC=1, B cycle, nothing happens (update pitch) */
				break;
				case 4: /* PC=2, A cycle, update pitch (calc K1) */
				current_pitch += ((target_pitch - current_pitch) >> interp_coeff[interp_period]);
				break;
				case 5: /* PC=2, B cycle, nothing happens (update K1) */
				break;
				case 6: /* PC=3, A cycle, update K1 (calc K2) */
				current_k[0] += ((target_k[0] - current_k[0]) >> interp_coeff[interp_period]);
				break;
				case 7: /* PC=3, B cycle, nothing happens (update K2) */
				break;
				case 8: /* PC=4, A cycle, update K2 (calc K3) */
				current_k[1] += ((target_k[1] - current_k[1]) >> interp_coeff[interp_period]);
				break;
				case 9: /* PC=4, B cycle, nothing happens (update K3) */
				break;
				case 10: /* PC=5, A cycle, update K3 (calc K4) */
				current_k[2] += ((target_k[2] - current_k[2]) >> interp_coeff[interp_period]);
				break;
				case 11: /* PC=5, B cycle, nothing happens (update K4) */
				break;
				case 12: /* PC=6, A cycle, update K4 (calc K5) */
				current_k[3] += ((target_k[3] - current_k[3]) >> interp_coeff[interp_period]);
				break;
				case 13: /* PC=6, B cycle, nothing happens (update K5) */
				break;
				case 14: /* PC=7, A cycle, update K5 (calc K6) */
				current_k[4] += ((target_k[4] - current_k[4]) >> interp_coeff[interp_period]);
				break;
				case 15: /* PC=7, B cycle, nothing happens (update K6) */
				break;
				case 16: /* PC=8, A cycle, update K6 (calc K7) */
				current_k[5] += ((target_k[5] - current_k[5]) >> interp_coeff[interp_period]);
				break;
				case 17: /* PC=8, B cycle, nothing happens (update K7) */
				break;
				case 18: /* PC=9, A cycle, update K7 (calc K8) */
				current_k[6] += ((target_k[6] - current_k[6]) >> interp_coeff[interp_period]);
				break;
				case 19: /* PC=9, B cycle, nothing happens (update K8) */
				break;
				case 20: /* PC=10, A cycle, update K8 (calc K9) */
				current_k[7] += ((target_k[7] - current_k[7]) >> interp_coeff[interp_period]);
				break;
				case 21: /* PC=10, B cycle, nothing happens (update K9) */
				break;
				case 22: /* PC=11, A cycle, update K9 (calc K10) */
				current_k[8] += ((target_k[8] - current_k[8]) >> interp_coeff[interp_period]);
				break;
				case 23: /* PC=11, B cycle, nothing happens (update K10) */
				break;
				case 24: /* PC=12, A cycle, update K10 (do nothing) */
				current_k[9] += ((target_k[9] - current_k[9]) >> interp_coeff[interp_period]);
				break;
			}
    }

		/* calculate the output */

		if (current_energy == 0)
		{
			/* generate silent samples here */
			current_val = 0x00;
		}
		else if (old_pitch == 0)
		{
			/* generate unvoiced samples here */
			if (RNG&1)
				current_val = -64; /* according to the patent it is (either + or -) half of the maximum value in the chirp table */
			else
				current_val = 64;

		}
		else
		{
                        // generate voiced samples here
			/* US patent 4331836 Figure 14B shows, and logic would hold, that a pitch based chirp
			 * function has a chirp/peak and then a long chain of zeroes.
			 * The last entry of the chirp rom is at address 0b110011 (51d), the 52nd sample,
			 * and if the address reaches that point the ADDRESS incrementer is
			 * disabled, forcing all samples beyond 51d to be == 51d
			 */

   		if (pitch_count >= 51)
  			current_val = (INT8)chirptable[51];
  		else
  			current_val = (INT8)chirptable[pitch_count];
		}

		/* Update LFSR *20* times every sample, like patent shows */
		for (i=0; i<20; i++)
		{
			bitout = ((RNG>>12)&1) ^
				 ((RNG>>10)&1) ^
				 ((RNG>> 9)&1) ^
				 ((RNG>> 0)&1);
			RNG >>= 1;
			RNG |= (bitout<<12);
		}

		/* Lattice filter here */

		Y11 = (current_val * 64 * current_energy) / 512;

		for (i = 9; i >= 0; i--)
		{
			Y11 = Y11 - ((current_k[i] * x[i]) / 512);
			x[i+1] = x[i] + ((current_k[i] * Y11) / 512);
		}

		x[0] = Y11;


		/* clipping & wrapping, just like the patent */

		/* YL10 - YL4 ==> DA6 - DA0 */
		cliptemp = Y11 / 16;

		if (cliptemp > 511) cliptemp = -512 + (cliptemp-511);
		else if (cliptemp < -512) cliptemp = 511 - (cliptemp+512);

		if (cliptemp > 127)
			buffer[buf_count] = 127*256;
		else if (cliptemp < -128)
			buffer[buf_count] = -128*256;
		else
			buffer[buf_count] = cliptemp *256;

		/* Update all counts */

		sample_count = (sample_count + 1) % 200;

		if (current_pitch != 0)
		{
			pitch_count++;
			if (pitch_count >= current_pitch)
				pitch_count = 0;
		}
		else
			pitch_count = 0;

		interp_count = (interp_count + 1) % 25;

		buf_count++;
		size--;
	}

empty:

	while (size > 0)
	{
		sample_count = (sample_count + 1) % 200;
		interp_count = (interp_count + 1) % 25;

		buffer[buf_count] = 0x00;
		buf_count++;
		size--;
	}
}




/******************************************************************************************

     CTL_set -- set CTL pins named CTL1, CTL2, CTL4 and CTL8

******************************************************************************************/

void tms5110_CTL_set(int data)
{
	CTL_pins = data & 0xf;
}


/******************************************************************************************

     PDC_set -- set Processor Data Clock. Execute CTL_pins command on hi-lo transition.

******************************************************************************************/

void tms5110_PDC_set(int data)
{
	if (PDC != (data & 0x1) )
	{
		PDC = data & 0x1;
		if (PDC == 0) /* toggling 1->0 processes command on CTL_pins */
		{
			/* only real commands we handle now are SPEAK and RESET */

			switch (CTL_pins & 0xe) /*CTL1 - don't care*/
			{
			case TMS5110_CMD_SPEAK:
				speaking_now = 1;
				/*speak_delay_frames = 10;*/

				//should FIFO be cleared now ?????

				break;

        		case TMS5110_CMD_RESET:
				speaking_now = 0;
				talk_status = 0;
				break;

			default:
				break;
			}
		}
	}
}



/******************************************************************************************

     parse_frame -- parse a new frame's worth of data; returns 0 if not enough bits in buffer

******************************************************************************************/

static void parse_frame(void)
{
	int bits, indx, i, rep_flag;
#if (DEBUG_5110)
	int ene;
#endif

	/* count the total number of bits available */
	bits = fifo_count;


	/* attempt to extract the energy index */
	bits -= 4;
	if (bits < 0)
	{
		request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
	}
	indx = extract_bits(4);
	new_energy = energytable[indx];
#if (DEBUG_5110)
	ene = indx;
#endif

	/* if the energy index is 0 or 15, we're done */

	if ((indx == 0) || (indx == 15))
	{
		if (DEBUG_5110) logerror("  (4-bit energy=%d frame)\n",new_energy);

	/* clear the k's */
		if (indx == 0)
		{
			for (i = 0; i < 10; i++)
				new_k[i] = 0;
		}

		/* clear fifo if stop frame encountered */
		if (indx == 15)
		{
			if (DEBUG_5110) logerror("  (4-bit energy=%d STOP frame)\n",new_energy);
			fifo_head = fifo_tail = fifo_count = 0;
		}
		return;
	}


	/* attempt to extract the repeat flag */
	bits -= 1;
	if (bits < 0)
	{
		request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
	}
	rep_flag = extract_bits(1);

	/* attempt to extract the pitch */
	bits -= 5;
	if (bits < 0)
	{
		request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
	}
	indx = extract_bits(5);
	new_pitch = pitchtable[indx];

	/* if this is a repeat frame, just copy the k's */
	if (rep_flag)
	{
	//actually, we do nothing because the k's were already loaded (on parsing the previous frame)

		if (DEBUG_5110) logerror("  (10-bit energy=%d pitch=%d rep=%d frame)\n", new_energy, new_pitch, rep_flag);
		return;
	}


	/* if the pitch index was zero, we need 4 k's */
	if (indx == 0)
	{
		/* attempt to extract 4 K's */
		bits -= 18;
		if (bits < 0)
		{
		request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
		}
		for (i = 0; i < 4; i++)
			new_k[i] = ktable[i][extract_bits(kbits[i])];

	/* and clear the rest of the new_k[] */
		for (i = 4; i < 10; i++)
			new_k[i] = 0;

		if (DEBUG_5110) logerror("  (28-bit energy=%d pitch=%d rep=%d 4K frame)\n", new_energy, new_pitch, rep_flag);
		return;
	}

	/* else we need 10 K's */
	bits -= 39;
	if (bits < 0)
	{
			request_bits( -bits ); /* toggle M0 to receive needed bits */
		bits = 0;
	}
#if (DEBUG_5110)
	printf("FrameDump %02d ", ene);
	for (i = 0; i < 10; i++)
	{
		int xx;
		xx = extract_bits(kbits[i]);
		new_k[i] = ktable[i][xx];
		printf("%02d ", xx);
	}
	printf("\n");
#else
	for (i = 0; i < 10; i++)
	{
		int xx;
		xx = extract_bits(kbits[i]);
		new_k[i] = ktable[i][xx];
	}
#endif
	if (DEBUG_5110) logerror("  (49-bit energy=%d pitch=%d rep=%d 10K frame)\n", new_energy, new_pitch, rep_flag);

}



#if 0
/*This is an example word TEN taken from the TMS5110A datasheet*/
static unsigned int example_word_TEN[619]={
/* 1*/1,0,0,0,	0,	0,0,0,0,0,	1,1,0,0,0,	0,0,0,1,0,	0,1,1,1,	0,1,0,1,
/* 2*/1,0,0,0,	0,	0,0,0,0,0,	1,0,0,1,0,	0,0,1,1,0,	0,0,1,1,	0,1,0,1,
/* 3*/1,1,0,0,	0,	1,0,0,0,0,	1,0,1,0,0,	0,1,0,1,0,	0,1,0,0,	1,0,1,0,	1,0,0,0,	1,0,0,1,	0,1,0,1,	0,0,1,	0,1,0,	0,1,1,
/* 4*/1,1,1,0,	0,	0,1,1,1,1,	1,0,1,0,1,	0,1,1,1,0,	0,1,0,1,	0,1,1,1,	0,1,1,1,	1,0,1,1,	1,0,1,0,	0,1,1,	0,1,0,	0,1,1,
/* 5*/1,1,1,0,	0,	1,0,0,0,0,	1,0,1,0,0,	0,1,1,1,0,	0,1,0,1,	1,0,1,0,	1,0,0,0,	1,1,0,0,	1,0,1,1,	1,0,0,	0,1,0,	0,1,1,
/* 6*/1,1,1,0,	0,	1,0,0,0,1,	1,0,1,0,1,	0,1,1,0,1,	0,1,1,0,	0,1,1,1,	0,1,1,1,	1,0,1,0,	1,0,1,0,	1,1,0,	0,0,1,	1,0,0,
/* 7*/1,1,1,0,	0,	1,0,0,1,0,	1,0,1,1,1,	0,1,1,1,0,	0,1,1,1,	0,1,1,1,	0,1,0,1,	0,1,1,0,	1,0,0,1,	1,1,0,	0,1,0,	0,1,1,
/* 8*/1,1,1,0,	1,	1,0,1,0,1,
/* 9*/1,1,1,0,	0,	1,1,0,0,1,	1,0,1,1,1,	0,1,0,1,1,	1,0,1,1,	0,1,1,1,	0,1,0,0,	1,0,0,0,	1,0,0,0,	1,1,0,	0,1,1,	0,1,1,
/*10*/1,1,0,1,	0,	1,1,0,1,0,	1,0,1,0,1,	0,1,1,0,1,	1,0,1,1,	0,1,0,1,	0,1,0,0,	1,0,0,0,	1,0,1,0,	1,1,0,	0,1,0,	1,0,0,
/*11*/1,0,1,1,	0,	1,1,0,1,1,	1,0,0,1,1,	1,0,0,1,0,	0,1,1,0,	0,0,1,1,	0,1,0,1,	1,0,0,1,	1,0,1,0,	1,0,0,	0,1,1,	0,1,1,
/*12*/1,0,0,0,	0,	1,1,1,0,0,	1,0,0,1,1,	0,0,1,1,0,	0,1,0,0,	0,1,1,0,	1,1,0,0,	0,1,0,1,	1,0,0,0,	1,0,0,	0,1,0,	1,0,1,
/*13*/0,1,1,1,	1,	1,1,1,0,1,
/*14*/0,1,1,1,	0,	1,1,1,1,0,	1,0,0,1,1,	0,0,1,1,1,	0,1,0,1,	0,1,0,1,	1,1,0,0,	0,1,1,1,	1,0,0,0,	1,0,0,	0,1,0,	1,0,1,
/*15*/0,1,1,0,	0,	1,1,1,1,0,	1,0,1,0,1,	0,0,1,1,0,	0,1,0,0,	0,0,1,1,	1,1,0,0,	1,0,0,1,	0,1,1,1,	1,0,1,	0,1,0,	1,0,1,
/*16*/1,1,1,1
};
#endif

