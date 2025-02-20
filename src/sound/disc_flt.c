// license:BSD-3-Clause
// copyright-holders:K.Wilkins
/************************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@esplexo.co.uk)
 *
 *  (c) K.Wilkins 2000
 *
 ***********************************************************************
 *
 * DST_CRFILTER          - Simple CR filter & also highpass filter
 * DST_FILTER1           - Generic 1st order filter
 * DST_FILTER2           - Generic 2nd order filter
 * DST_RCFILTER          - Simple RC filter & also lowpass filter
 * DST_RCDISC            - Simple discharging RC
 * DST_RCDISC2           - Simple charge R1/C, discharge R0/C
 * DST_RCDISC5           - Diode in series with R//C
 *
 ************************************************************************/


struct dst_op_amp_filt_context
{
	int		type;		// What kind of filter
	int		is_norton;	// 1 = Norton op-amps
	double	vRef;
	double	vP;
	double	vN;
	double	rTotal;		// All input resistance in parallel.
	double	iFixed;		// Current supplied by r3 & r4 if used.
	double	exponentC1;
	double	exponentC2;
	double	exponentC3;
	double	rRatio;		// divide ratio of resistance network
	double	vC1;		// Charge on C1
	double	vC1b;		// Charge on C1, part of C1 charge if needed
	double	vC2;		// Charge on C2
	double	vC3;		// Charge on C2
	double	gain;		// Gain of the filter
	double  x1, x2;		/* x[k-1], x[k-2], previous 2 input values */
	double  y1, y2;		/* y[k-1], y[k-2], previous 2 output values */
	double  a1,a2;		/* digital filter coefficients, denominator */
	double  b0,b1,b2;	/* digital filter coefficients, numerator */
};

struct dst_rcdisc_context
{
	int state;
	double t;           // time
	double f;			// RCINTEGRATE
	double R1;			// RCINTEGRATE
	double R2;			// RCINTEGRATE
	double R3;			// RCINTEGRATE
	double C;			// RCINTEGRATE
	double vCap;		// RCDISC_MOD
	double vCE;			// RCINTEGRATE
	double exponent0;
	double exponent1;
};

struct dst_rcfilter_context
{
	double  rc;
	double	exponent;
	double	vCap;
};
/************************************************************************
 *
 * DST_CRFILTER - Usage of node_description values for CR filter
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 * input[4]    - Voltage reference. Usually 0V.
 *
 ************************************************************************/
#define DST_CRFILTER__IN        node->input[0]
#define DST_CRFILTER__R         node->input[1]
#define DST_CRFILTER__C         node->input[2]
#define DST_CRFILTER__VREF      node->input[3]

int dst_crfilter_step(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	double rc = DST_CRFILTER__R * DST_CRFILTER__C;
	if (rc != context->rc)
	{
		context->rc = rc;
		context->exponent = RC_CHARGE_EXP(rc);
	}

	double v_out = DST_CRFILTER__IN - context->vCap;
	double v_diff = v_out - DST_CRFILTER__VREF;
	node->output = v_out;
	context->vCap += v_diff * context->exponent;

	return 0;
}

int dst_crfilter_reset(struct node_description *node)
{
	struct dst_rcfilter_context *context = node->context;

	context->rc = DST_CRFILTER__R * DST_CRFILTER__C;
	context->exponent = RC_CHARGE_EXP(context->rc);
	context->vCap = 0;
	node->output = DST_CRFILTER__IN;

	return 0;
}

int dst_crfilter_init(struct node_description *node)
{
	struct dst_rcfilter_context *context = (struct dst_rcfilter_context*)node->context;

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dst_rcfilter_context)))==NULL)
	{
		discrete_log("dst_crfilter_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dst_rcfilter_context));
	}

	/* Initialise the object */
	dst_crfilter_reset(node);
	return 0;
}

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
/* This creates an IIR Digital Filter from an Analog Filter,
 * Using the Bilinear Transformation, with pre-warping
 *
 * wc = 2*pi*fc (cutoff frequency)
 * (wc modified by pre-warping formula wc = (2/T)*tan(wc*T/2)
 *
 * Prototype filters come from "Active-Filter Cookbook" by Don Lancaster, Ch. 3
 *
 * Low Pass:        High Pass:
 *
 *         K*wc             K*s
 * H(s) = ------    H(s) = ------
 *        s + wc           s + wc
 *
 * Apply bilinear transform: s = (2/T) * (1-z^-1) / (1+z^1)
 *
 *        G*(b0 + b1*z^-1)
 * H(z) = ----------------
 *           1  + a1*z^-1
 *
 * Which gives:
 *
 * y(k) = -a1*y(k-1) + b0*K*x(k) + b1*K*x(k-1)
 *
 ************************************************************************/

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
		*b1 = -*b0;
	}
	else
	{
		discrete_log("calculate_filter1_coefficients() - Invalid filter type for 1st order filter.");
	}
}

int dst_filter1_step(struct node_description *node)
{
	struct dss_filter1_context *context = (struct dss_filter1_context*)node->context;
	double gain = 1.0;

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
	struct dss_filter1_context *context = (struct dss_filter1_context*)node->context;

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
/*
 * This creates an IIR Digital Filter from an Analog Filter,
 * Using the Bilinear Transformation, with pre-warping
 *
 * wc = 2*pi*fc (cutoff frequency)
 * (wc modified by pre-warping formula wc = (2/T)*tan(wc*T/2)
 *
 * Prototype filters come from "Active-Filter Cookbook" by Don Lancaster, Ch. 3
 * (d is the Damping Factor, which is also 1/Q)
 *
 * Low Pass:
 *
 *              K*wc^2
 * H(s) = -------------------
 *        s^2 + d*wc*s + wc^2
 *
 * Band Pass:
 *
 *              K*wc*s
 * H(s) = -------------------
 *        s^2 + d*wc*s + wc^2
 *
 *
 * High Pass:
 *
 *               K*s^2
 * H(s) = -------------------
 *        s^2 + d*wc*s + wc^2
 *
 * Apply bilinear transform: s = (2/T) * (1-z^-1) / (1+z^-1)
 *
 *        K*(b0 + b1*z^-1 + b2*z^-2)
 * H(z) = --------------------------
 *           1  + a1*z^-1 + a2*z^-2
 *
 * Which gives:
 *
 * y(k) = -a1*y(k-1) - a2*y(k-2) + b0*K*x(k) + b1*K*x(k-1) + b2*K*x(k-2)
 *
 ************************************************************************/

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
	double two_over_T = 2.0*Machine->sample_rate;
	double two_over_T_squared = two_over_T * two_over_T;

	/* calculate digital filter coefficents */
	/*w = 2.0*M_PI*fc; no pre-warping */
	w = two_over_T*tan(M_PI*fc/Machine->sample_rate); /* pre-warping */
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
		*b0 = d*w*two_over_T/den;
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
	struct dss_filter2_context *context = (struct dss_filter2_context*)node->context;
	double gain = 1.0;

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
	struct dss_filter2_context *context = (struct dss_filter2_context*)node->context;

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

	calculate_filter2_coefficients(node->input[2], node->input[3], node->input[4],
								   &context->a1, &context->a2,
								   &context->b0, &context->b1, &context->b2);

	/* Initialise the object */
	dst_filter2_reset(node);
	return 0;
}


/************************************************************************
 *
 * DST_OP_AMP_FILT - Op Amp filter circuit RC filter
 *
 * input[0]    - Enable input value
 * input[1]    - IN0 node
 * input[2]    - IN1 node
 * input[3]    - Filter Type
 *
 * also passed discrete_op_amp_filt_info structure
 *
 * Mar 2004, D Renaud.
 ************************************************************************/
#define DST_OP_AMP_FILT__ENABLE node->input[0]
#define DST_OP_AMP_FILT__INP1   node->input[1]
#define DST_OP_AMP_FILT__INP2   node->input[2]
#define DST_OP_AMP_FILT__TYPE   node->input[3]

int dst_op_amp_filt_step(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	double i, v = 0;

	if (DST_OP_AMP_FILT__ENABLE)
	{
		if (context->is_norton)
		{
			v = DST_OP_AMP_FILT__INP1 - OP_AMP_NORTON_VBE;
			if (v < 0) v = 0;
		}
		else
		{
			/* Millman the input voltages. */
			i = context->iFixed;
			switch (context->type)
			{
				case DISC_OP_AMP_FILTER_IS_LOW_PASS_1_A:
					i += (DST_OP_AMP_FILT__INP1 - DST_OP_AMP_FILT__INP2) / info->r1;
					if (info->r2 != 0)
						i += (context->vP - DST_OP_AMP_FILT__INP2) / info->r2;
					if (info->r3 != 0)
						i += (context->vN - DST_OP_AMP_FILT__INP2) / info->r3;
					break;
				default:
					i += (DST_OP_AMP_FILT__INP1 - context->vRef) / info->r1;
					if (info->r2 != 0)
						i += (DST_OP_AMP_FILT__INP2 - context->vRef) / info->r2;
					break;
			}
			v = i * context->rTotal;
		}

		switch (context->type)
		{
			case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_LOW_PASS_1_A:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + DST_OP_AMP_FILT__INP2;
				break;

			case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
				node->output = (v - context->vC1) * context->gain + info->vRef;
				context->vC1 += (v - context->vC1) * context->exponentC1;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
				node->output = (v - context->vC2);
				context->vC2 += (v - context->vC2) * context->exponentC2;
				context->vC1 += (node->output - context->vC1) * context->exponentC1;
				node->output = context->vC1 * context->gain + info->vRef;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON:
				context->vC1 += (v - context->vC1) * context->exponentC1;
				context->vC2 += (context->vC1 - context->vC2) * context->exponentC2;
				v = context->vC2;
				node->output = v - context->vC3;
				context->vC3 += (v - context->vC3) * context->exponentC3;
				i = node->output / context->rTotal;
				node->output = (context->iFixed - i) * info->rF;
				break;

			case DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | DISC_OP_AMP_IS_NORTON:
				node->output = v - context->vC1;
				context->vC1 += (v - context->vC1) * context->exponentC1;
				i = node->output / context->rTotal;
				node->output = (context->iFixed - i) * info->rF;
				break;

			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
			case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | DISC_OP_AMP_IS_NORTON:
				node->output = -context->a1*context->y1 - context->a2*context->y2 +
								context->b0*v + context->b1*context->x1 + context->b2*context->x2 +
								context->vRef;
				context->x2 = context->x1;
				context->x1 = v;
				context->y2 = context->y1;
				break;
		}

		/* Clip the output to the voltage rails.
		 * This way we get the original distortion in all it's glory.
		 */
		if (node->output > context->vP) node->output = context->vP;
		if (node->output < context->vN) node->output = context->vN;
		context->y1 = node->output - context->vRef;
	}
	else
		node->output = 0;

	return 0;
}

int dst_op_amp_filt_reset(struct node_description *node)
{
	const struct discrete_op_amp_filt_info *info = node->custom;
	struct dst_op_amp_filt_context *context = node->context;

	/* Convert the passed filter type into an int for easy use. */
	context->type = (int)DST_OP_AMP_FILT__TYPE & DISC_OP_AMP_FILTER_TYPE_MASK;
	context->is_norton = (int)DST_OP_AMP_FILT__TYPE & DISC_OP_AMP_IS_NORTON;

	if (context->is_norton)
	{
		context->vRef = 0;
		context->rTotal = info->r1;
		if (context->type == (DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON))
			context->rTotal += info->r2 +  info->r3;

		/* Setup the current to the + input. */
		context->iFixed = (info->vP - OP_AMP_NORTON_VBE) / info->r4;

		/* Set the output max. */
		context->vP =  info->vP - OP_AMP_NORTON_VBE;
		context->vN =  info->vN;
	}
	else
	{
		context->vRef = info->vRef;
		/* Set the output max. */
		context->vP =  info->vP - OP_AMP_VP_RAIL_OFFSET;
		context->vN =  info->vN;

		/* Work out the input resistance.  It is all input and bias resistors in parallel. */
		context->rTotal  = 1.0 / info->r1;			// There has to be an R1.  Otherwise the table is wrong.
		if (info->r2 != 0) context->rTotal += 1.0 / info->r2;
		if (info->r3 != 0) context->rTotal += 1.0 / info->r3;
		context->rTotal = 1.0 / context->rTotal;

		context->iFixed = 0;

		context->rRatio = info->rF / (context->rTotal + info->rF);
		context->gain = -info->rF / context->rTotal;
	}

	switch (context->type)
	{
		case DISC_OP_AMP_FILTER_IS_LOW_PASS_1:
		case DISC_OP_AMP_FILTER_IS_LOW_PASS_1_A:
			context->exponentC1 = RC_CHARGE_EXP(info->rF * info->c1);
			context->exponentC2 =  0;
			break;
		case DISC_OP_AMP_FILTER_IS_HIGH_PASS_1:
			context->exponentC1 = RC_CHARGE_EXP(context->rTotal * info->c1);
			context->exponentC2 =  0;
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1:
			context->exponentC1 = RC_CHARGE_EXP(info->rF * info->c1);
			context->exponentC2 = RC_CHARGE_EXP(context->rTotal * info->c2);
			break;
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M | DISC_OP_AMP_IS_NORTON:
			if (info->r2 == 0)
				context->rTotal = info->r1;
			else
				context->rTotal = RES_2_PARALLEL(info->r1, info->r2);
			// fallthrough
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_1M:
		{
			double fc = 1.0 / (2 * M_PI * sqrt(context->rTotal * info->rF * info->c1 * info->c2));
			double d  = (info->c1 + info->c2) / sqrt(info->rF / context->rTotal * info->c1 * info->c2);
			double gain = -info->rF / context->rTotal * info->c2 / (info->c1 + info->c2);

			calculate_filter2_coefficients(fc, d, DISC_FILTER_BANDPASS,
										   &context->a1, &context->a2,
										   &context->b0, &context->b1, &context->b2);
			context->b0 *= gain;
			context->b1 *= gain;
			context->b2 *= gain;

			if (context->is_norton)
				context->vRef = (info->vP - OP_AMP_NORTON_VBE) / info->r3 * info->rF;
			else
				context->vRef = info->vRef;

			break;
		}
		case DISC_OP_AMP_FILTER_IS_BAND_PASS_0 | DISC_OP_AMP_IS_NORTON:
			context->exponentC1 = RC_CHARGE_EXP(RES_2_PARALLEL(info->r1, info->r2 + info->r3 + info->r4) * info->c1);
			context->exponentC2 = RC_CHARGE_EXP(RES_2_PARALLEL(info->r1 + info->r2, info->r3 + info->r4) * info->c2);
			context->exponentC3 = RC_CHARGE_EXP((info->r1 + info->r2 + info->r3 + info->r4) * info->c3);
			break;
		case DISC_OP_AMP_FILTER_IS_HIGH_PASS_0 | DISC_OP_AMP_IS_NORTON:
			context->exponentC1 = RC_CHARGE_EXP(info->r1 * info->c1);
			break;
	}

	/* At startup there is no charge on the caps and output is 0V in relation to vRef. */
	context->vC1 = 0;
	context->vC1b = 0;
	context->vC2 = 0;
	context->vC3 = 0;

	node->output = info->vRef;

	return 0;
}

int dst_op_amp_filt_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dst_op_amp_filt_context)))==NULL)
	{
		discrete_log("dst_op_amp_filt_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dst_op_amp_filt_context));
	}

	/* Initialise the object */
	dst_op_amp_filt_reset(node);
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

/************************************************************************
 *
 * DST_RCDISC5 -  Diode in series with R//C
 *
 * input[0]    - Enable input value
 * input[1]    - input value
 * input[2]    - Resistor value (initialization only)
 * input[3]    - Capacitor Value (initialization only)
 *
 ************************************************************************/
#define DST_RCDISC5__ENABLE node->input[0]
#define DST_RCDISC5__IN     node->input[1]
#define DST_RCDISC5__R      node->input[2]
#define DST_RCDISC5__C      node->input[3]

int dst_rcdisc5_step(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	double diff,u;

	/* Exponential based in difference between input/output   */

	u = DST_RCDISC5__IN - 0.7; /* Diode drop */
	if( u < 0)
		u = 0;

	diff = u - context->vCap;

	if(DST_RCDISC5__ENABLE)
	{
		if(diff < 0)
			diff = diff * context->exponent0;

		context->vCap += diff;
		node->output = context->vCap;
	}
	else
	{
		if(diff > 0)
			context->vCap = u;

		node->output = 0;
	}

	return 0;
}

int dst_rcdisc5_reset(struct node_description *node)
{
	struct dst_rcdisc_context *context = node->context;

	node->output = 0;

	context->state = 0;
	context->t = 0;
	context->vCap = 0;
	context->exponent0 = RC_CHARGE_EXP(DST_RCDISC5__R * DST_RCDISC5__C);

	return 0;
}

int dst_rcdisc5_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_rcdisc_context)))==NULL)
	{
		discrete_log("dss_rcdisc5_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_rcdisc_context));
	}

	/* Initialise the object */
	dst_rcdisc5_reset(node);
	return 0;
}


/* !!!!!!!!!!! NEW FILTERS for testing !!!!!!!!!!!!!!!!!!!!! */


/************************************************************************/
/*                                                                      */
/* DST_RCFILTERN - Usage of node_description values for RC filter        */
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
	double f=1.0/((2.*M_PI)*node->input[2]*node->input[3]);

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
	double f=1.0/((2.*M_PI)*node->input[2]*node->input[3]);

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

	f1=1.0/((2.*M_PI)*node->input[2]*node->input[5]);
	f2=1.0/((2.*M_PI)*node->input[4]*node->input[5]);

	calculate_filter1_coefficients(f1, DISC_FILTER_LOWPASS, &context->a1_0, &context->b0_0, &context->b1_0);
	calculate_filter1_coefficients(f2, DISC_FILTER_LOWPASS, &context->a1_1, &context->b0_1, &context->b1_1);

	/* Initialise the object */
	dst_rcdisc2_reset(node);

	return 0;
}
