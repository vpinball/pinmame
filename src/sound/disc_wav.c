/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* DSS_SINEWAVE          - Sinewave generator source code               */
/* DSS_SQUAREWAVE        - Squarewave generator source code             */
/* DSS_TRIANGLEWAVE      - Triangle waveform generator                  */
/* DSS_SAWTOOTHWAVE      - Sawtooth waveform generator                  */
/* DSS_NOISE             - Noise Source - Random source                 */
/* DSS_LFSR_NOISE        - Linear Feedback Shift Register Noise         */
/*                                                                      */
/************************************************************************/

struct dss_sinewave_context
{
	double phase;
};

struct dss_noise_context
{
	double phase;
};

struct dss_adsr_context
{
	double phase;
};

struct dss_lfsr_context
{
	unsigned int	lfsr_reg;
	double	sampleStep;
	double	shiftStep;
	double	t;
};

struct dss_squarewave_context
{
	double phase;
	double trigger;
};

struct dss_squarewfix_context
{
	int	flip_flop;
	double	sampleStep;
	double	tLeft;
	double	tOff;
	double	tOn;
};

struct dss_trianglewave_context
{
	double phase;
};

struct dss_sawtoothwave_context
{
	double phase;
	int type;
};

/************************************************************************/
/*                                                                      */
/* DSS_SINWAVE - Usage of node_description values for step function     */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitude input value                                    */
/* input3    - DC Bias                                                  */
/* input4    - Starting phase                                           */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_sinewave_step(struct node_description *node)
{
	struct dss_sinewave_context *context=(struct dss_sinewave_context*)node->context;

	/* Set the output */
	if(node->input[0])
	{
		node->output=(node->input[2]/2.0) * sin(context->phase);
		/* Add DC Bias component */
		node->output=node->output+node->input[3];
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[3];
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*node->input[1])/Machine->sample_rate)),2.0*M_PI);

	return 0;
}

int dss_sinewave_reset(struct node_description *node)
{
	struct dss_sinewave_context *context;
	double start;
	context=(struct dss_sinewave_context*)node->context;
	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[4]/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);
	/* Step the output to make it correct */
	dss_sinewave_step(node);
	return 0;
}

int dss_sinewave_init(struct node_description *node)
{
	discrete_log("dss_sinewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_sinewave_context)))==NULL)
	{
		discrete_log("dss_sinewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_sinewave_context));
	}

	/* Initialise the object */
	dss_sinewave_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_SQUAREWAVE - Usage of node_description values for step function  */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitude input value                                    */
/* input3    - Duty Cycle                                               */
/* input4    - DC Bias level                                            */
/* input5    - Start Phase                                              */
/*                                                                      */
/************************************************************************/
int dss_squarewave_step(struct node_description *node)
{
	struct dss_squarewave_context *context=(struct dss_squarewave_context*)node->context;

	/* Establish trigger phase from duty */
	context->trigger=((100-node->input[3])/100)*(2.0*M_PI);

	/* Set the output */
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

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*node->input[1])/Machine->sample_rate)),2.0*M_PI);

	return 0;
}

int dss_squarewave_reset(struct node_description *node)
{
	struct dss_squarewave_context *context;
	double start;
	context=(struct dss_squarewave_context*)node->context;

	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[5]/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Step the output */
	dss_squarewave_step(node);

	return 0;
}

int dss_squarewave_init(struct node_description *node)
{
	discrete_log("dss_squarewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_squarewave_context)))==NULL)
	{
		discrete_log("dss_squarewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_squarewave_context));
	}

	/* Initialise the object */
	dss_squarewave_reset(node);

	return 0;
}

/************************************************************************/
/*                                                                      */
/* DSS_SQUAREWFIX - Usage of node_description values for step function  */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitude input value                                    */
/* input3    - Duty Cycle                                               */
/* input4    - DC Bias level                                            */
/* input5    - Start Phase                                              */
/*                                                                      */
/************************************************************************/
int dss_squarewfix_step(struct node_description *node)
{
	struct dss_squarewfix_context *context=(struct dss_squarewfix_context*)node->context;

	context->tLeft -= context->sampleStep;

	/* The enable input only curtails output, phase rotation still occurs */
	while (context->tLeft <= 0)
	{
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}
//discrete_log("Step out - tLeft:%f FF:%d",context->tLeft,context->flip_flop);

	if(node->input[0])
	{
//discrete_log("Step in - F:%f D:%f tOff:%f tOn:%f tSample:%f tLeft:%f FF:%d",node->input1,node->input3,tOff,tOn,context->sampleStep,context->tLeft,context->flip_flop);
//		context->tLeft += context->sampleStep;
//		while (context->tLeft >= (context->flip_flop ? context->tOn : context->tOff))
//		{
//			context->tLeft -= context->flip_flop ? context->tOn : context->tOff;
//			context->flip_flop = context->flip_flop ? 0 : 1;
//		}

		/* Add gain and DC Bias component */

		context->tOff = 1.0 / node->input[1];	/* cycle time */
		context->tOn = context->tOff * (node->input[3] / 100.0);
		context->tOff -= context->tOn;

		node->output = (context->flip_flop ? node->input[2] / 2.0 : -(node->input[2] / 2.0)) + node->input[4];
	}
	else
	{
		/* Just output DC Bias */
		node->output = node->input[4];
	}
	return 0;
}

int dss_squarewfix_reset(struct node_description *node)
{
	struct dss_squarewfix_context *context=(struct dss_squarewfix_context*)node->context;

	context->sampleStep = 1.0 / Machine->sample_rate;
	context->flip_flop = 1;

	/* Do the intial time shift and convert freq to off/on times */
	context->tOff = 1.0 / node->input[1];	/* cycle time */
	context->tLeft = node->input[5] / 360.0;	/* convert start phase to % */
	context->tLeft = context->tLeft - (int)context->tLeft;	/* keep % between 0 & 1 */
	context->tLeft = context->tLeft < 0 ? 1.0 + context->tLeft : context->tLeft;	/* if - then flip to + phase */
	context->tLeft *= context->tOff;
	context->tOn = context->tOff * (node->input[3] / 100.0);
	context->tOff -= context->tOn;

discrete_log("RESET in - F:%f D:%f P:%f == tOff:%f tOn:%f tLeft:%f",node->input[1],node->input[3],node->input[5],context->tOff,context->tOn,context->tLeft);
//	while (context->tLeft >= context->flip_flop ? context->tOn : context->tOff)
//	{
//		context->tLeft -= context->flip_flop ? context->tOn : context->tOff;
//		context->flip_flop = context->flip_flop ? 0 : 1;
//	}


	context->tLeft = -context->tLeft;

	/* toggle output and work out intial time shift */
	while (context->tLeft <= 0)
	{
		context->flip_flop = context->flip_flop ? 0 : 1;
		context->tLeft += context->flip_flop ? context->tOn : context->tOff;
	}
discrete_log("RESET out - tLeft:%f FF:%d",context->tLeft,context->flip_flop);

	/* Step the output */
	dss_squarewfix_step(node);

	return 0;
}

int dss_squarewfix_init(struct node_description *node)
{
	discrete_log("dss_squarewfix_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_squarewfix_context)))==NULL)
	{
		discrete_log("dss_squarewave2_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_squarewfix_context));
	}

	/* Initialise the object */
	dss_squarewfix_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_SQUAREWAVE2 - Usage of node_description values                   */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Amplitude input value                                    */
/* input2    - OFF Time                                                 */
/* input3    - ON Time                                                  */
/* input4    - DC Bias level                                            */
/* input5    - Initial Time Shift                                       */
/*                                                                      */
/************************************************************************/
int dss_squarewave2_step(struct node_description *node)
{
	struct dss_squarewave_context *context=(struct dss_squarewave_context*)node->context;
	double newphase;

	/* Establish trigger phase from time periods */
	context->trigger=(node->input[2] / (node->input[2] + node->input[3])) * (2.0 * M_PI);

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */

	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = 2Pi/(output period*sample freq)          */
	newphase = context->phase + ((2.0 * M_PI) / ((node->input[2] + node->input[3]) * Machine->sample_rate));
	/* Keep the new phasor in the 2Pi range.*/
	context->phase = fmod(newphase, 2.0 * M_PI);

	if(node->input[0])
	{
		if(context->phase>context->trigger)
			node->output=(node->input[1]/2.0);
		else
			node->output=-(node->input[1]/2.0);

		/* Add DC Bias component */
		node->output = node->output + node->input[4];
	}
	else
	{
		/* Just output DC Bias */
		node->output = node->input[4];
	}
	return 0;
}

int dss_squarewave2_reset(struct node_description *node)
{
	struct dss_squarewave_context *context=(struct dss_squarewave_context*)node->context;
	double start;

	/* Establish starting phase, convert from degrees to radians */
	start = (node->input[5] / (node->input[2] + node->input[3])) * (2.0 * M_PI);
	/* Make sure its always mod 2Pi */
	context->phase = fmod(start, 2.0 * M_PI);

	/* Step the output */
	dss_squarewave2_step(node);

	return 0;
}

int dss_squarewave2_init(struct node_description *node)
{
	discrete_log("dss_squarewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_squarewave_context)))==NULL)
	{
		discrete_log("dss_squarewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_squarewave_context));
	}

	/* Initialise the object */
	dss_squarewave_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_TRIANGLEWAVE - Usage of node_description values for step function*/
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitde input value                                     */
/* input3    - DC Bias value                                            */
/* input4    - Initial Phase                                            */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_trianglewave_step(struct node_description *node)
{
	struct dss_trianglewave_context *context=(struct dss_trianglewave_context*)node->context;

	if(node->input[0])
	{
		node->output=context->phase < M_PI ? (node->input[2] * (context->phase / (M_PI/2.0) - 1.0))/2.0 :
									(node->input[2] * (3.0 - context->phase / (M_PI/2.0)))/2.0 ;

		/* Add DC Bias component */
		node->output=node->output+node->input[3];
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[3];
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*node->input[1])/Machine->sample_rate)),2.0*M_PI);

	return 0;
}

int dss_trianglewave_reset(struct node_description *node)
{
	struct dss_trianglewave_context *context;
	double start;

	context=(struct dss_trianglewave_context*)node->context;
	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[4]/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Step to set the output */
	dss_trianglewave_step(node);
	return 0;
}

int dss_trianglewave_init(struct node_description *node)
{
	discrete_log("dss_trianglewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_trianglewave_context)))==NULL)
	{
		discrete_log("dss_trianglewave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_trianglewave_context));
	}

	/* Initialise the object */
	dss_trianglewave_reset(node);
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_SAWTOOTHWAVE - Usage of node_description values for step function*/
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Frequency input value                                    */
/* input2    - Amplitde input value                                     */
/* input3    - DC Bias Value                                            */
/* input4    - Gradient                                                 */
/* input5    - Initial Phase                                            */
/*                                                                      */
/************************************************************************/
int dss_sawtoothwave_step(struct node_description *node)
{
	struct dss_sawtoothwave_context *context=(struct dss_sawtoothwave_context*)node->context;

	if(node->input[0])
	{
		node->output=(context->type==0)?context->phase*(node->input[2]/(2.0*M_PI)):node->input[2]-(context->phase*(node->input[2]/(2.0*M_PI)));
		node->output-=node->input[2]/2.0;
		/* Add DC Bias component */
		node->output=node->output+node->input[3];
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[3];
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*node->input[1])/Machine->sample_rate)),2.0*M_PI);

	return 0;
}

int dss_sawtoothwave_reset(struct node_description *node)
{
	struct dss_sawtoothwave_context *context;
	double start;

	context=(struct dss_sawtoothwave_context*)node->context;
	/* Establish starting phase, convert from degrees to radians */
	start=(node->input[5]/360.0)*(2.0*M_PI);
	/* Make sure its always mod 2Pi */
	context->phase=fmod(start,2.0*M_PI);

	/* Invert gradient depending on sawtooth type /|/|/|/|/| or |\|\|\|\|\ */
	context->type=(node->input[4])?1:0;

	/* Step the node to set the output */
	dss_sawtoothwave_step(node);

	return 0;
}

int dss_sawtoothwave_init(struct node_description *node)
{
	discrete_log("dss_trianglewave_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_sawtoothwave_context)))==NULL)
	{
		discrete_log("dss_sawtoothwave_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_sawtoothwave_context));
	}

	/* Initialise the object */
	dss_sawtoothwave_reset(node);
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_NOISE - Usage of node_description values for white nose generator*/
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Noise sample frequency                                   */
/* input2    - Amplitude input value                                    */
/* input3    - DC Bias value                                            */
/* input4    - NOT USED                                                 */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_noise_step(struct node_description *node)
{
	struct dss_noise_context *context;
	context=(struct dss_noise_context*)node->context;

	if(node->input[0])
	{
		/* Only sample noise on rollover to next cycle */
		if(context->phase>(2.0*M_PI))
		{
			int newval=rand() & 0x7fff;
			node->output=node->input[2]*(1-(newval/16384.0));

			/* Add DC Bias component */
			node->output=node->output+node->input[3];
		}
	}
	else
	{
		/* Just output DC Bias */
		node->output=node->input[3];
	}

	/* Work out the phase step based on phase/freq & sample rate */
	/* The enable input only curtails output, phase rotation     */
	/* still occurs                                              */
	/*     phase step = 2Pi/(output period/sample period)        */
	/*                    boils out to                           */
	/*     phase step = (2Pi*output freq)/sample freq)           */
	/* Also keep the new phasor in the 2Pi range.                */
	context->phase=fmod((context->phase+((2.0*M_PI*node->input[1])/Machine->sample_rate)),2.0*M_PI);

	return 0;
}


int dss_noise_reset(struct node_description *node)
{
	struct dss_noise_context *context=(struct dss_noise_context*)node->context;
	context->phase=0;
	dss_noise_step(node);
	return 0;
}

int dss_noise_init(struct node_description *node)
{
	discrete_log("dss_noise_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_noise_context)))==NULL)
	{
		discrete_log("dss_noise_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_noise_context));
	}

	/* Initialise the object */
	dss_noise_reset(node);

	return 0;
}

/************************************************************************/
/*                                                                      */
/* DSS_LFSR_NOISE - Usage of node_description values for LFSR noise gen */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Register reset                                           */
/* input2    - Noise sample frequency                                   */
/* input3    - Amplitude input value                                    */
/* input4    - Input feed bit                                           */
/* input5    - Bias                                                     */
/*                                                                      */
/************************************************************************/
int	dss_lfsr_function(int myfunc,int in0,int in1,int bitmask)
{
	int retval;

	in0&=bitmask;
	in1&=bitmask;

	switch(myfunc)
	{
		case DISC_LFSR_XOR:
			retval=in0^in1;
			break;
		case DISC_LFSR_OR:
			retval=in0|in1;
			break;
		case DISC_LFSR_AND:
			retval=in0&in1;
			break;
		case DISC_LFSR_XNOR:
			retval=in0^in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_NOR:
			retval=in0|in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_NAND:
			retval=in0&in1;
			retval=retval^bitmask;	/* Invert output */
			break;
		case DISC_LFSR_IN0:
			retval=in0;
			break;
		case DISC_LFSR_IN1:
			retval=in1;
			break;
		case DISC_LFSR_NOT_IN0:
			retval=in0^bitmask;
			break;
		case DISC_LFSR_NOT_IN1:
			retval=in1^bitmask;
			break;
		case DISC_LFSR_REPLACE:
			retval=in0&~in1;
			retval=in0|in1;
			break;
		default:
			discrete_log("dss_lfsr_function - Invalid function type passed");
			retval=0;
			break;
	}
	return retval;
}

/* reset prototype so that it can be used in init function */
int dss_lfsr_reset(struct node_description *node);

int dss_lfsr_step(struct node_description *node)
{
	struct dss_lfsr_context *context;
	double shiftAmount;
	int fb0,fb1,fbresult,i;
	struct discrete_lfsr_desc *lfsr_desc;
	context=(struct dss_lfsr_context*)node->context;

	/* Fetch the LFSR descriptor structure in a local for quick ref */
	lfsr_desc=(struct discrete_lfsr_desc*)(node->custom);

	/* Reset everything if necessary */
	if((node->input[1] ? 1 : 0) == ((lfsr_desc->flags & DISC_LFSR_FLAG_RESET_TYPE_H) ? 1 : 0))
	{
		dss_lfsr_reset(node);
	}

	i=0;
	/* Calculate the number of full shift register cycles since last machine sample. */
	shiftAmount = ((context->sampleStep + context->t) / context->shiftStep);
	context->t = (shiftAmount - (int)shiftAmount) * context->shiftStep;    /* left over amount of time */
	while(i<(int)shiftAmount)
	{
		i++;
		/* Now clock the LFSR by 1 cycle and output */

		/* Fetch the last feedback result */
		fbresult=((context->lfsr_reg)>>(lfsr_desc->bitlength))&0x01;

		/* Stage 2 feedback combine fbresultNew with infeed bit */
		fbresult=dss_lfsr_function(lfsr_desc->feedback_function1,fbresult,((node->input[4])?0x01:0x00),0x01);

		/* Stage 3 first we setup where the bit is going to be shifted into */
		fbresult=fbresult*lfsr_desc->feedback_function2_mask;
		/* Then we left shift the register, */
		context->lfsr_reg=(context->lfsr_reg)<<1;
		/* Now move the fbresult into the shift register and mask it to the bitlength */
		context->lfsr_reg=dss_lfsr_function(lfsr_desc->feedback_function2,fbresult, (context->lfsr_reg), ((1<<(lfsr_desc->bitlength))-1));

		/* Now get and store the new feedback result */
		/* Fetch the feedback bits */
		fb0=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel0))&0x01;
		fb1=((context->lfsr_reg)>>(lfsr_desc->feedback_bitsel1))&0x01;
		/* Now do the combo on them */
		fbresult=dss_lfsr_function(lfsr_desc->feedback_function0,fb0,fb1,0x01);
		context->lfsr_reg=dss_lfsr_function(DISC_LFSR_REPLACE,(context->lfsr_reg), fbresult<<(lfsr_desc->bitlength), ((2<<(lfsr_desc->bitlength))-1));

		/* Now select the output bit */
		node->output=((context->lfsr_reg)>>(lfsr_desc->output_bit))&0x01;

		/* Final inversion if required */
		if(lfsr_desc->flags & DISC_LFSR_FLAG_OUT_INVERT) node->output=(node->output)?0.0:1.0;

		/* Gain stage */
		node->output=(node->output)?(node->input[3])/2:-(node->input[3])/2;
		/* Bias input as required */
		node->output=node->output+node->input[5];
	}

	/* If disabled then clamp the output to DC Bias */
	if(!node->input[0])
	{
		node->output=node->input[5];
	}

	return 0;
}

int dss_lfsr_reset(struct node_description *node)
{
	struct dss_lfsr_context *context;
	struct discrete_lfsr_desc *lfsr_desc;

	context=(struct dss_lfsr_context*)node->context;
	lfsr_desc=(struct discrete_lfsr_desc*)(node->custom);

	context->lfsr_reg=lfsr_desc->reset_value;

	context->lfsr_reg=dss_lfsr_function(DISC_LFSR_REPLACE,0, (dss_lfsr_function(lfsr_desc->feedback_function0,0,0,0x01))<<(lfsr_desc->bitlength),((2<<(lfsr_desc->bitlength))-1));
	discrete_log("Shift register RESET to     %#10X.\n",(context->lfsr_reg));

	/* Now select and setup the output bit */
	node->output=((context->lfsr_reg)>>(lfsr_desc->output_bit))&0x01;

	/* Final inversion if required */
	if(lfsr_desc->flags&DISC_LFSR_FLAG_OUT_INVERT) node->output=(node->output)?0.0:1.0;

	/* Gain stage */
	node->output=(node->output)?(node->input[3])/2:-(node->input[3])/2;
	/* Bias input as required */
	node->output=node->output+node->input[5];

	return 0;
}

int dss_lfsr_init(struct node_description *node)
{
	struct dss_lfsr_context *context;

	discrete_log("dss_lfsr_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_lfsr_context)))==NULL)
	{
		discrete_log("dss_lfsr_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_lfsr_context));
	}

	/* Initialise the object */
	context=(struct dss_lfsr_context*)node->context;
	context->sampleStep = 1.0 / Machine->sample_rate;
	context->shiftStep = 1.0 / node->input[2];
	context->t = 0;

	dss_lfsr_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_ADSR - Attack Decay Sustain Release                              */
/*                                                                      */
/* input0    - Enable input value                                       */
/* input1    - Trigger value                                            */
/* input2    - gain scaling factor                                      */
/* input3    - NOT USED                                                 */
/* input4    - NOT USED                                                 */
/* input5    - NOT USED                                                 */
/*                                                                      */
/************************************************************************/
int dss_adsrenv_step(struct node_description *node)
{
	struct dss_adsr_context *context;
	context=(struct dss_adsr_context*)node->context;

	if(node->input[0])
	{
		node->output=0;
	}
	else
	{
		node->output=0;
	}
	return 0;
}


int dss_adsrenv_reset(struct node_description *node)
{
//	struct dss_adsr_context *context=(struct dss_adsr_context*)node->context;
	dss_adsrenv_step(node);
	return 0;
}

int dss_adsrenv_init(struct node_description *node)
{
	discrete_log("dss_adsrenv_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_adsr_context)))==NULL)
	{
		discrete_log("dss_adsrenv_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_adsr_context));
	}

	/* Initialise the object */
	dss_noise_reset(node);

	return 0;
}
