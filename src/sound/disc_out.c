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
/* DSO_OUTPUT - Sinewave generator source code                          */
/*                                                                      */
/************************************************************************/

/* DISCRETE_OUTPUT(_STEREO)_AC (input[3] set) adds an output coupling capacitor for unipolar graphs, similar to real boards
   (wave node bias, 555s swinging 0..Vcc), for machines that don't model it yet: a one-pole high-pass at about 1.6Hz (10uF into a ~10k amp input).
   No clamp either way: levels beyond 16bit full scale reach the mixer, which only clamps the final mix */
#define DSO_DC_CUTOFF_HZ 1.6

struct dso_output_context
{
	float  left;       /* float stream sample, 1.0 = 32768 */
	float  right;
	double dcInteg[2]; /* DC blocker state, left/right */
	double dcPrev[2];
	double dcCoeff;
	int    dcPrimed;   /* 0: settle on the first input, so the initial bias does not thump */
};

/************************************************************************/
/*                                                                      */
/* Usage of node_description values for step function                   */
/*                                                                      */
/* input[0]    - Left channel output value                              */
/* input[1]    - Right channel output value                             */
/* input[2]    - Volume setting (static)                                */
/* input[3]    - AC coupled (static, nonzero: remove DC)                */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dso_output_step(struct node_description *node)
{
	/* We ALWAYS work in stereo here, let the stream update decide if its mono/stereo output */
	struct dso_output_context *context;
	int i;
	context=(struct dso_output_context*)node->context;
	/* Node outputs are in 16bit units, scale them to the float stream (removing their DC if AC coupled) */
	if (node->input[3] == 0.)
	{
		context->left =(float)(node->input[0]*(1./32768.));
		context->right=(float)(node->input[1]*(1./32768.));
		return 0;
	}
	for (i = 0; i < 2; i++)
	{
		const double x = node->input[i];
		if (!context->dcPrimed)
			context->dcPrev[i] = x;
		context->dcInteg[i] = context->dcInteg[i]*context->dcCoeff + (x - context->dcPrev[i]);
		context->dcPrev[i] = x;
	}
	context->dcPrimed = 1;
	context->left =(float)(context->dcInteg[0]*(1./32768.));
	context->right=(float)(context->dcInteg[1]*(1./32768.));
	return 0;
}

int dso_output_reset(struct node_description *node)
{
	struct dso_output_context *context=(struct dso_output_context*)node->context;
	const double coeff=context->dcCoeff;
	memset(context,0,sizeof(struct dso_output_context));
	context->dcCoeff=coeff;
	return 0;
}

int dso_output_init(struct node_description *node)
{
	discrete_log("dso_output_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dso_output_context)))==NULL)
	{
		discrete_log("dso_output_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		const double w = (2.*M_PI*DSO_DC_CUTOFF_HZ)/DISCRETE_SAMPLE_RATE;
		memset(node->context,0,sizeof(struct dso_output_context));
		((struct dso_output_context*)node->context)->dcCoeff = (1. - sin(w))/cos(w); /* exact one-pole corner */
	}

	return 0;
}
