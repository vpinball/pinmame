/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@dysfunction.demon.co.uk)             */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DSS_NE555             - NE555 Simulation code                        */
/*                                                                      */
/************************************************************************/

struct dsd_555_astbl_context
{
	int flip_flop;
        double cWaveform;
        double step;
};

struct dsd_squarew555_context
{
	double	phase;
	double	trigger;
	double	k[2];
	int	was_reset;
};

struct dsd_566_context
{
	double phase;
	double trigger;
};




/************************************************************************/
/*                                                                      */
/* DSD_555_ASTBL - Usage of node_description values for 555 function    */
/*                                                                      */
/* input[0]    - Reset value                                            */
/* input[1]    - Amplitude input value                                  */
/* input[2]    - R1 value                                               */
/* input[3]    - R2 value                                               */
/* input[4]    - C value                                                */
/* input[5]    - Control Voltage value                                  */
/*                                                                      */
/************************************************************************/
/* We have to remember to think in the future when doing RC waves.      */
/* What that means is when we calculate the next value for the Cap (VC) */
/* voltage, it is the value at the next sample period.  After a RESET,  */
/* VC=0.  It does not change until the next time period.                */
/* The neat thing is that we know where it will be in the future, so    */
/* if the VC exceeds CV or Trig; we can clamp it to those values.       */
/* Then figure out how much time we overshot, and add that to the start */
/* of the next change.  While this will slightly skew the wave peaks,   */
/* it will not be audible.  And will force the waveform to the exact    */
/* frequency.  We will always round up the in the present and shorten   */
/* the future's time period.  This is because in the present the        */
/* voltage change is smaller then after the change in direction.        */
/************************************************************************/
int dsd_555_astbl_step(struct node_description *node)
{
	struct dsd_555_astbl_context *context=(struct dsd_555_astbl_context*)node->context;
	double cv, cWaveNext, trigger, t, vC;
	int *astblOutTypePTR;

	/* RESET? */
	if(node->input[0])
	{

		/* Fetch the output type descriptor in a local for quick ref */
		astblOutTypePTR = (int*)(node->custom);

		/* Check: if the control voltage node is not connected, set it to a default of 2/3Vcc */
		cv = node->input[5] == NODE_NC ? (node->input[1] * 2.0) / 3.0 : node->input[5];

		/* Calculate future capacitor voltage.
		 * ref@ http://www.nalanda.nitc.ac.in/resources/ee/ebooks/eckts/DC/DC_16.html
		 * ref@ http://www.physics.rutgers.edu/ugrad/205/capacitance.html
		 * The formulas from the ref pages have been modified to reflect that we are steping the change.
		 * t = time of sample (1/sample frequency)
		 * VC = Voltage across capacitor
		 * VC' = Future voltage across capacitor
		 * Vc = Voltage change
		 * Vr = is the voltage across the resistor.  For charging it is Vcc - VC.  Discharging it is VC - 0.
		 * R = R1+R2 (for charging)  R = R2 for discharging.
		 * Vc = Vr*(1-exp(-t/(R*C)))
		 * VC' = VC + Vc (for charging) VC' = VC - Vc for discharging.
		 *
		 * We will also need to calculate the the amount of time we overshoot the CV and trig.
		 * t = amount of time we overshot
		 * Vc = voltage change overshoot
		 * t = R*C(log(1/(1-(Vc/Vr))))
		 */

		t = context->step;
		vC = context->cWaveform;

		/* Keep looping until all toggling in time sample is used up. */
		do
		{
			if (context->flip_flop)
			{
				/* Charging */
				cWaveNext = vC + ((node->input[1] - vC) * (1 - exp(-(t / ((node->input[2] + node->input[3]) * node->input[4])))));
				t = 0;
	
				/* has it charged past upper limit? */
				if (cWaveNext >= cv)
				{
					if (cWaveNext > cv)
					{
						/* calculate the overshoot time */
						t = (node->input[2] + node->input[3]) * node->input[4] * log(1 / (1 - ((cWaveNext - cv) / (node->input[1] - vC))));
					}
					vC = cv;	/* clamp to make up for sampling rate */
					context->flip_flop = 0;
				}
			}
			else
			{
				/* Discharging */
				cWaveNext = vC - (vC * (1 - exp(-(t / (node->input[3] * node->input[4])))));
				t = 0;
	
				/* has it discharged past lower limit? */
				trigger = cv / 2;
				if (cWaveNext <= trigger)
				{
					if (cWaveNext < trigger)
					{
						/* calculate the overshoot time */
						t = node->input[3] * node->input[4] * log(1 / (1 - ((trigger - cWaveNext) / vC)));
					}
					vC = trigger;	/* clamp to make up for sampling rate */
					context->flip_flop = 1;
				}
			}
		} while(t);

		context->cWaveform = vC;

		/* Select output type */
		node->output = *astblOutTypePTR & DISC_555_ASTBL_CAP ? context->cWaveform : context->flip_flop * node->input[1];
		/* AC or DC */
		if (*astblOutTypePTR & DISC_555_ASTBL_AC)
			node->output -= *astblOutTypePTR & DISC_555_ASTBL_CAP ? cv * 3.0 /4.0 : node->input[1] / 2.0;

		/* Save current waveform */
		context->cWaveform = cWaveNext;
	}
	else
	{
		/* We are in RESET */
		node->output = 0;
		context->flip_flop = 1;
		context->cWaveform = 0;
	}

	return 0;
}

int dsd_555_astbl_reset(struct node_description *node)
{
	struct dsd_555_astbl_context *context;
	context=(struct dsd_555_astbl_context*)node->context;
	context->flip_flop=1;
	context->cWaveform = 0;
	context->step = 1.0 / Machine->sample_rate;

	/* Step to set the output */
	dsd_555_astbl_step(node);

	return 0;
}

int dsd_555_astbl_init(struct node_description *node)
{
	discrete_log("dsd_555_astbl_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dsd_555_astbl_context)))==NULL)
	{
		discrete_log("dsd_555_astbl_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dsd_555_astbl_context));
	}

	/* Initialise the object */
	dsd_555_astbl_reset(node);
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSD_SQUAREW555 - Usage of node_description values                    */
/*                                                                      */
/* input[0]    - Reset input value                                      */
/* input[1]    - Amplitude input value                                  */
/* input[2]    - R1 value                                               */
/* input[3]    - R2 value                                               */
/* input[4]    - C value                                                */
/* input[5]    - DC Bias level                                          */
/*                                                                      */
/************************************************************************/
int dsd_squarew555_step(struct node_description *node)
{
	struct dsd_squarew555_context *context=(struct dsd_squarew555_context*)node->context;
	double newphase;
	double tOn, tOff;

	/* 555 is different then other square waves.  It always starts high. */
	/* if we are in the first high after the reset, it uses a different time constant. */
	tOn = context->k[context->was_reset] * (node->input[2] + node->input[3]) * node->input[4];
	tOff = context->k[0] * node->input[3] * node->input[4];

	/* Establish trigger phase from time periods */
	context->trigger=(tOn / (tOn + tOff)) * (2.0 * PI);

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = 2Pi/(output period*sample freq)          */
	newphase = context->phase + ((2.0 * PI) / ((tOn + tOff) * Machine->sample_rate));
	/* Keep the new phasor in the 2Pi range.*/
	context->phase = fmod(newphase, 2.0 * PI);

	if(node->input[0])
	{
		if(context->phase>context->trigger)
		{
			node->output = -(node->input[1] / 2.0);
			context->was_reset = 0;
		}
		else
			node->output = node->input[1] / 2.0;

		/* Add DC Bias component */
		node->output = node->output + node->input[5];
	}
	else
	{
		context->was_reset = 1;

		/* Just output DC Bias */
		node->output = node->input[5];
	}
	return 0;
}

int dsd_squarew555_reset(struct node_description *node)
{
	struct dsd_squarew555_context *context=(struct dsd_squarew555_context*)node->context;

	/* Establish starting phase and reset values */
	context->phase = fmod(0, 2.0 * PI);
	context->was_reset = 1;
	context->k[0] = log(2);	/* standard 555 charge/discharge constant */
	context->k[1] = log(3);	/* after reset 555 charge/discharge constant, used for first pulse */

	/* Step the output */
	dsd_squarew555_step(node);

	return 0;
}

int dsd_squarew555_init(struct node_description *node)
{
	discrete_log("dsd_squarew555_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dsd_squarew555_context)))==NULL)
	{
		discrete_log("dsd_squarew555_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dsd_squarew555_context));
	}

	/* Initialise the object */
	dsd_squarew555_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSD_SQUAREW566 - Usage of node_description values                    */
/*                                                                      */
/* input[0]    - Reset input value                                      */
/* input[1]    - Amplitude input value                                  */
/* input[2]    - R1 value                                               */
/* input[3]    - R2 value                                               */
/* input[4]    - C value                                                */
/* input[5]    - DC Bias level                                          */
/*                                                                      */
/************************************************************************/
int dsd_squarew566_step(struct node_description *node)
{
	struct dsd_566_context *context=(struct dsd_566_context*)node->context;
	double newphase;

	/* Establish trigger phase from duty */
	context->trigger=((100-node->input[3])/100)*(2.0*PI);

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	newphase = context->phase+((2.0*PI*node->input[1])/Machine->sample_rate);
	/* Keep the new phasor in the 2Pi range.*/
	context->phase=fmod(newphase,2.0*PI);

	if(node->input[0])
	{
		if(context->phase>context->trigger)
			node->output=(node->input[2]/2.0);
		else
			node->output=-(node->input[2]/2.0);

		/* Add DC Bias component */
		node->output=node->output+node->input[4];
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[4];
	}
	return 0;
}

int dsd_squarew566_reset(struct node_description *node)
{
	struct dsd_566_context *context=(struct dsd_566_context*)node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[5]/360.0)*(2.0*PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*PI);

	/* Step the output */
	dsd_squarew566_step(node);

	return 0;
}

int dsd_squarew566_init(struct node_description *node)
{
	discrete_log("dsd_squarew566_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dsd_566_context)))==NULL)
	{
		discrete_log("dsd_squarew566_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dsd_566_context));
	}

	/* Initialise the object */
	dsd_squarew566_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSD_TRIANGLEW566 - Usage of node_description values for step function*/
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Frequency input value                                  */
/* input[2]    - Amplitde input value                                   */
/* input[3]    - DC Bias value                                          */
/* input[4]    - Initial Phase                                          */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dsd_trianglew566_step(struct node_description *node)
{
	struct dsd_566_context *context=(struct dsd_566_context*)node->context;
	double newphase;

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	newphase=context->phase+((2.0*PI*node->input[1])/Machine->sample_rate);
	/* Keep the new phasor in the 2Pi range.*/
	newphase=fmod(newphase,2.0*PI);
	context->phase=newphase;

	if(node->input[0])
	{
		node->output=newphase < PI ? (node->input[2] * (newphase / (PI/2.0) - 1.0))/2.0 :
									(node->input[2] * (3.0 - newphase / (PI/2.0)))/2.0 ;

		/* Add DC Bias component */
		node->output=node->output+node->input[3];
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[3];
	}
	return 0;
}

int dsd_trianglew566_reset(struct node_description *node)
{
	struct dsd_566_context *context=(struct dsd_566_context*)node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[4]/360.0)*(2.0*PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*PI);

	/* Step to set the output */
	dsd_trianglew566_step(node);
	return 0;
}

int dsd_trianglew566_init(struct node_description *node)
{
	discrete_log("dsd_trianglew566_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dsd_566_context)))==NULL)
	{
		discrete_log("dsd_trianglew566_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dsd_566_context));
	}

	/* Initialise the object */
	dsd_trianglew566_reset(node);
	return 0;
}

