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
/* DST_FILTER1           - Generic 1st order filter                     */
/* DST_FILTER2           - Generic 2nd order filter                     */
/* DST_RCFILTER          - Simple RC filter & also lowpass filter       */
/* DST_RCDISC            - Simple discharing RC                         */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* DST_FILTER1 - Generic 1st order filter                               */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Frequency value (initialisation only)                  */
/* input[3]    - Filter type (initialisation only)                      */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/

struct dss_filter1_context
{
	double x1;		/* x[k-1], previous input value */
	double y1;		/* y[k-1], previous output value */
	double a1;		/* digital filter coefficients, denominator */
	double b0, b1;	/* digital filter coefficients, numerator */
};

static void calculate_filter1_coefficients(double fc, double type,
                                           double *a1, double *b0, double *b1)
{
	double den, w, two_over_T;

	/* calculate digital filter coefficents */
    /*w = 2.0*M_PI*fc; no pre-warping */
    w = Machine->sample_rate*2.0*tan(M_PI*fc/Machine->sample_rate); /* pre-warping */
	two_over_T = 2.0*Machine->sample_rate;

    den = w + two_over_T;
    *a1 = (w - two_over_T)/den;
    if (type == DISC_FILTER_LOWPASS)
	{
    	*b0 = *b1 = w/den;
	}
	else if (type == DISC_FILTER_HIGHPASS)
    {
		*b0 = two_over_T/den;
    	*b1 = *b0;
	}
	else
	{
		discrete_log("calculate_filter1_coefficients() - Invalid filter type for 1st order filter.");
	}
}

int dst_filter1_step(struct node_description *node)
{
	struct dss_filter1_context *context;
	double gain = 1.0;

	context=(struct dss_filter1_context*)node->context;
	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 + context->b0*gain*node->input[1] + context->b1*context->x1;

	context->x1 = gain*node->input[1];
	context->y1 = node->output;

	return 0;
}

int dst_filter1_reset(struct node_description *node)
{
	node->output=0;
	return 0;
}

int dst_filter1_init(struct node_description *node)
{
	struct dss_filter1_context *context;

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_filter1_context)))==NULL)
	{
		discrete_log("dss_filter1_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_filter1_context));
	}
	context=(struct dss_filter1_context*)node->context;

	calculate_filter1_coefficients(node->input[2], node->input[3],
								   &context->a1, &context->b0, &context->b1);

	/* Initialise the object */
	dst_filter1_reset(node);
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_FILTER2 - Generic 2nd order filter                               */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Frequency value (initialisation only)                  */
/* input[3]    - Damping value (initialisation only)                    */
/* input[4]    - Filter type (initialisation only) 						*/
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/

struct dss_filter2_context
{
	double x1, x2;		/* x[k-1], x[k-2], previous 2 input values */
	double y1, y2;		/* y[k-1], y[k-2], previous 2 output values */
	double a1, a2;		/* digital filter coefficients, denominator */
	double b0, b1, b2;	/* digital filter coefficients, numerator */
};

static void calculate_filter2_coefficients(double fc, double d, double type,
                                           double *a1, double *a2,
                                           double *b0, double *b1, double *b2)
{
	double w;	/* cutoff freq, in radians/sec */
	double w_squared;
	double den;	/* temp variable */
	double two_over_T = 2*Machine->sample_rate;
	double two_over_T_squared = two_over_T * two_over_T;

	/* calculate digital filter coefficents */
    /*w = 2.0*M_PI*fc; no pre-warping */
    w = Machine->sample_rate*2.0*tan(M_PI*fc/Machine->sample_rate); /* pre-warping */
	w_squared = w*w;

    den = two_over_T_squared + d*w*two_over_T + w_squared;

    *a1 = 2.0*(-two_over_T_squared + w_squared)/den;
    *a2 = (two_over_T_squared - d*w*two_over_T + w_squared)/den;

    if (type == DISC_FILTER_LOWPASS)
	{
    	*b0 = *b2 = w_squared/den;
    	*b1 = 2.0*(*b0);
	}
	else if (type == DISC_FILTER_BANDPASS)
    {
		*b0 = w*two_over_T/den;
    	*b1 = 0.0;
    	*b2 = -(*b0);
	}
	else if (type == DISC_FILTER_HIGHPASS)
    {
		*b0 = *b2 = two_over_T_squared/den;
    	*b1 = -2.0*(*b0);
	}
	else
	{
		discrete_log("calculate_filter2_coefficients() - Invalid filter type for 2nd order filter.");
	}
}

int dst_filter2_step(struct node_description *node)
{
	struct dss_filter2_context *context;
	double gain = 1.0;

	context=(struct dss_filter2_context*)node->context;
	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	node->output = -context->a1*context->y1 - context->a2*context->y2 +
	                context->b0*gain*node->input[1] + context->b1*context->x1 + context->b2*context->x2;

	context->x2 = context->x1;
	context->x1 = gain*node->input[1];
	context->y2 = context->y1;
	context->y1 = node->output;

	return 0;
}

int dst_filter2_reset(struct node_description *node)
{
	node->output=0;
	return 0;
}

int dst_filter2_init(struct node_description *node)
{
	struct dss_filter2_context *context;

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_filter2_context)))==NULL)
	{
		discrete_log("dss_filter2_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_filter2_context));
	}
	context=(struct dss_filter2_context*)node->context;

	calculate_filter2_coefficients(node->input[2], node->input[3], node->input[4],
								   &context->a1, &context->a2,
								   &context->b0, &context->b1, &context->b2);

	/* Initialise the object */
	dst_filter2_reset(node);
	return 0;
}


struct dss_rcdisc_context
{
        int state;
        double t;           // time
        double step;
	double exponent0;
	double exponent1;
};

/************************************************************************/
/*                                                                      */
/* DST_RCFILTER - Usage of node_description values for RC filter        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialisation only)                   */
/* input[3]    - Capacitor Value (initialisation only)                  */
/* input[4]    - NOT USED                                               */
/* input[5]    - Pre-calculated value for exponent                      */
/*                                                                      */
/************************************************************************/
int dst_rcfilter_step(struct node_description *node)
{
	/************************************************************************/
	/* Next Value = PREV + (INPUT_VALUE - PREV)*(1-(EXP(-TIMEDELTA/RC)))    */
	/************************************************************************/

	if(node->input[0])
	{
		node->output=node->output+((node->input[1]-node->output)*node->input[5]);
	}
	else
	{
		node->output=0;
	}
	return 0;
}

int dst_rcfilter_reset(struct node_description *node)
{
	node->output=0;
	return 0;
}

int dst_rcfilter_init(struct node_description *node)
{
	node->input[5]=-1.0/(node->input[2]*node->input[3]*Machine->sample_rate);
	node->input[5]=1-exp(node->input[5]);
	/* Initialise the object */
	dst_rcfilter_reset(node);
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_RCDISC -   Usage of node_description values for RC discharge     */
/*                (inverse slope of DST_RCFILTER)                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialisation only)                   */
/* input[3]    - Capacitor Value (initialisation only)                  */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT_USED                                               */
/*                                                                      */
/************************************************************************/

int dst_rcdisc_step(struct node_description *node)
{
	struct dss_rcdisc_context *context;
	context=(struct dss_rcdisc_context*)node->context;

	switch (context->state) {
		case 0:     /* waiting for trigger  */
			if(node->input[0]) {
				context->state = 1;
				context->t = 0;
			}
			node->output=0;
			break;

		case 1:
			if (node->input[0]) {
				node->output=node->input[1] * exp(context->t / context->exponent0);
				context->t += context->step;
                } else {
					context->state = 0;
			}
		}

	return 0;
}

int dst_rcdisc_reset(struct node_description *node)
{
	struct dss_rcdisc_context *context;
	context=(struct dss_rcdisc_context*)node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->exponent0=-1.0 * node->input[2]*node->input[3];

	return 0;
}

int dst_rcdisc_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_rcdisc_context)))==NULL)
	{
		discrete_log("dss_rcdisc_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_rcdisc_context));
	}

	/* Initialise the object */
	dst_rcdisc_reset(node);
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC2 -  Usage of node_description values for RC discharge     */
/*                Has switchable charge resistor/input                  */
/*                                                                      */
/* input[0]    - Switch input value                                     */
/* input[1]    - input[0] value                                         */
/* input[2]    - Resistor0 value (initialisation only)                  */
/* input[3]    - input[1] value                                         */
/* input[4]    - Resistor1 value (initialisation only)                  */
/* input[5]    - Capacitor Value (initialisation only)                  */
/*                                                                      */
/************************************************************************/


int dst_rcdisc2_step(struct node_description *node)
{
	double diff;
	struct dss_rcdisc_context *context;
	context=(struct dss_rcdisc_context*)node->context;

	/* Works differently to other as we always on, no enable */
	/* exponential based in differnce between input/output   */

	diff = ((node->input[0]==0)?node->input[1]:node->input[3])-node->output;
	diff = diff -(diff * exp(context->step / ((node->input[0]==0)?context->exponent0:context->exponent1)));
	node->output+=diff;
	return 0;
}

int dst_rcdisc2_reset(struct node_description *node)
{
	struct dss_rcdisc_context *context;
	context=(struct dss_rcdisc_context*)node->context;

	node->output=0;

	context->state = 0;
	context->t = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->exponent0=-1.0 * node->input[2]*node->input[5];
	context->exponent1=-1.0 * node->input[4]*node->input[5];

	return 0;
}

int dst_rcdisc2_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_rcdisc_context)))==NULL)
	{
		discrete_log("dss_rcdisc2_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_rcdisc_context));
	}

	/* Initialise the object */
	dst_rcdisc2_reset(node);
	return 0;
}

/* !!!!!!!!!!! NEW FILTERS for testing !!!!!!!!!!!!!!!!!!!!! */


/************************************************************************/
/*                                                                      */
/* DST_RCFILTER - Usage of node_description values for RC filter        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialisation only)                   */
/* input[3]    - Capacitor Value (initialisation only)                  */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/

int dst_rcfilterN_init(struct node_description *node)
{
	double f=1.0/(2*M_PI*node->input[2]*node->input[3]);

	node->input[2] = f;
	node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	return dst_filter1_init(node);
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC -   Usage of node_description values for RC discharge     */
/*                (inverse slope of DST_RCFILTER)                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - input value                                            */
/* input[2]    - Resistor value (initialisation only)                   */
/* input[3]    - Capacitor Value (initialisation only)                  */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT_USED                                               */
/*                                                                      */
/************************************************************************/

int dst_rcdiscN_init(struct node_description *node)
{
	double f=1.0/(2*M_PI*node->input[2]*node->input[3]);

	node->input[2] = f;
	node->input[3] = DISC_FILTER_LOWPASS;

	/* Use first order filter */
	return dst_filter1_init(node);
}

int dst_rcdiscN_step(struct node_description *node)
{
	struct dss_filter1_context *context;
	double gain = 1.0;
	context=(struct dss_filter1_context*)node->context;

	if (node->input[0] == 0.0)
	{
		gain = 0.0;
	}

	/* A rise in the input signal results in an instant charge, */
	/* else discharge through the RC to zero */
	if (gain*node->input[1] > context->x1)
		node->output = gain*node->input[1];
	else
		node->output = -context->a1*context->y1;

	context->x1 = gain*node->input[1];
	context->y1 = node->output;

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_RCDISC2 -  Usage of node_description values for RC discharge     */
/*                Has switchable charge resistor/input                  */
/*                                                                      */
/* input[0]    - Switch input value                                     */
/* input[1]    - input[0] value                                         */
/* input[2]    - Resistor0 value (initialisation only)                  */
/* input[3]    - input[1] value                                         */
/* input[4]    - Resistor1 value (initialisation only)                  */
/* input[5]    - Capacitor Value (initialisation only)                  */
/*                                                                      */
/************************************************************************/

struct dss_rcdisc2_context
{
	double x1;		/* x[k-1], last input value */
	double y1;		/* y[k-1], last output value */
	double a1_0, b0_0, b1_0;	/* digital filter coefficients, filter #1 */
	double a1_1, b0_1, b1_1;	/* digital filter coefficients, filter #2 */
};

int dst_rcdisc2N_step(struct node_description *node)
{
	struct dss_rcdisc2_context *context;
	double input = ((node->input[0]==0)?node->input[1]:node->input[3]);
	context=(struct dss_rcdisc2_context*)node->context;

	if (node->input[0] == 0)
		node->output = -context->a1_0*context->y1 + context->b0_0*input + context->b1_0*context->x1;
	else
		node->output = -context->a1_1*context->y1 + context->b0_1*input + context->b1_1*context->x1;

	context->x1 = input;
	context->y1 = node->output;

	return 0;
}

int dst_rcdisc2N_reset(struct node_description *node)
{
	node->output=0;
	return 0;
}

int dst_rcdisc2N_init(struct node_description *node)
{
	struct dss_rcdisc2_context *context;
	double f1,f2;

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_rcdisc2_context)))==NULL)
	{
		discrete_log("dst_rcdisc2_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_rcdisc2_context));
	}
	context=(struct dss_rcdisc2_context*)node->context;

	f1=1.0/(2*M_PI*node->input[2]*node->input[5]);
	f2=1.0/(2*M_PI*node->input[4]*node->input[5]);

	calculate_filter1_coefficients(f1, DISC_FILTER_LOWPASS, &context->a1_0, &context->b0_0, &context->b1_0);
	calculate_filter1_coefficients(f2, DISC_FILTER_LOWPASS, &context->a1_1, &context->b0_1, &context->b1_1);

	/* Initialise the object */
	dst_rcdisc2_reset(node);

	return 0;
}

