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
/* DST_TRANSFORM         - Multiple math functions                      */
/* DST_ADDDER            - Multichannel adder                           */
/* DST_GAIN              - Gain Factor                                  */
/* DST_SWITCH            - Switch implementation                        */
/* DST_RAMP              - Ramp up/down                                 */
/* DST_ONESHOT           - One shot pulse generator                     */
/* DST_DIVIDER           - Division function                            */
/* DST_LADDER            - Resistor ladder implementation               */
/* DST_SAMPHOLD          - Sample & Hold Implementation                 */
/* DST_LOGIC_INV         - Logic level invertor                         */
/* DST_LOGIC_AND         - Logic AND gate 4 input                       */
/* DST_LOGIC_NAND        - Logic NAND gate 4 input                      */
/* DST_LOGIC_OR          - Logic OR gate 4 input                        */
/* DST_LOGIC_NOR         - Logic NOR gate 4 input                       */
/* DST_LOGIC_XOR         - Logic XOR gate 2 input                       */
/* DST_LOGIC_NXOR        - Logic NXOR gate 2 input                      */
/*                                                                      */
/************************************************************************/

#include <float.h>

struct dss_ramp_context
{
        double step;
        int dir;	/* 1 if End is higher then Start */
	int last_en;	/* Keep track of the last enable value */
};

struct dst_oneshot_context
{
	double countdown;
	double stepsize;
	int state;
};

struct dst_ladder_context
{
        int state;
        double t;           // time
        double step;
		double exponent;
		double total_resistance;
};

struct dst_samphold_context
{
		double lastinput;
		int clocktype;
};

/************************************************************************/
/*                                                                      */
/* DST_ADDER - This is a 4 channel input adder with enable function     */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Channel1 input value                                   */
/* input[3]    - Channel2 input value                                   */
/* input[4]    - Channel3 input value                                   */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_adder_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=node->input[1] + node->input[2] + node->input[3] + node->input[4];
	}
	else
	{
		node->output=0;
	}
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_TRANSFORM - Programmable math module with enable function        */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Channel1 input value                                   */
/* input[3]    - Channel2 input value                                   */
/* input[4]    - Channel3 input value                                   */
/* input[5]    - Channel4 input value                                   */
/*                                                                      */
/************************************************************************/

#define MAX_TRANS_STACK	10

double dst_transform_pop(double *stack,int *pointer)
{
	double value;
	//decrement THEN read
	if(*pointer>0) (*pointer)--;
	value=stack[*pointer];
	return value;
}

double dst_transform_push(double *stack,int *pointer,double value)
{
	//Strore THEN increment
	if(*pointer<MAX_TRANS_STACK) stack[(*pointer)++]=value;
	return value;
}

int dst_transform_step(struct node_description *node)
{

	if(node->input[0])
	{
		double trans_stack[MAX_TRANS_STACK];
		double result,number1,number2;
		int	trans_stack_ptr=0;

		char *fPTR;
		node->output=0;

		fPTR = (char*)(node->custom);

		while(*fPTR!=0)
		{
			switch (*fPTR++)
			{
				case '*':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1*number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '/':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1/number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '+':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1+number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '-':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					number2=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=number1-number2;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '!':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=!number1;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case 'i':
					number1=dst_transform_pop(trans_stack,&trans_stack_ptr);
					result=-number1;
					dst_transform_push(trans_stack,&trans_stack_ptr,result);
					break;
				case '0':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[1]);
					break;
				case '1':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[2]);
					break;
				case '2':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[3]);
					break;
				case '3':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[4]);
					break;
				case '4':
					dst_transform_push(trans_stack,&trans_stack_ptr,node->input[5]);
					break;
				default:
					discrete_log("dst_transform_step - Invalid function type/variable passed");
					node->output = 0;
					break;
			}
		}
		node->output=dst_transform_pop(trans_stack,&trans_stack_ptr);
	}
	else
	{
		node->output=0;
	}
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_GAIN - This is a programmable gain module with enable function   */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Gain value                                             */
/* input[3]    - Final addition offset                                  */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_gain_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=node->input[1] * node->input[2];
		node->output+=node->input[3];
	}
	else
	{
		node->output=0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_DIVIDE  - Programmable divider with enable                       */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Channel0 input value                                   */
/* input[2]    - Divisor                                                */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_divide_step(struct node_description *node)
{
	if(node->input[0])
	{
		if(node->input[2]==0)
		{
			node->output=DBL_MAX;	/* Max out but dont break */
			discrete_log("dst_divider_step() - Divide by Zero attempted.");
		}
		else
		{
			node->output=node->input[1] / node->input[2];
		}
	}
	else
	{
		node->output=0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DSS_SWITCH - Programmable 2 pole switchmodule with enable function   */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - switch position                                        */
/* input[2]    - input[0]                                               */
/* input[3]    - input[1]                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_switch_step(struct node_description *node)
{
	if(node->input[0])
	{
		/* Input 1 switches between input[0]/input[2] */
		node->output=(node->input[1])?node->input[3]:node->input[2];
	}
	else
	{
		node->output=0;
	}
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_RAMP - Ramp up/down model usage                                  */
/*                                                                      */
/* input[0]    - Enable ramp                                            */
/* input[1]    - Ramp Reverse/Forward switch                            */
/* input[2]    - Gradient, change/sec                                   */
/* input[3]    - Start value                                            */
/* input[4]    - End value                                              */
/* input[5]    - Clamp value when disabled                              */
/*                                                                      */
/************************************************************************/

int dst_ramp_step(struct node_description *node)
{
	struct dss_ramp_context *context;
	context=(struct dss_ramp_context*)node->context;

	if(node->input[0])
	{
		if (!context->last_en)
		{
			context->last_en = 1;
			node->output = node->input[3];
		}
		if(context->dir ? node->input[1] : !node->input[1]) node->output+=context->step;
		else node->output-=context->step;
		/* Clamp to min/max */
		if(context->dir ? (node->output < node->input[3])
				: (node->output > node->input[3])) node->output=node->input[3];
		if(context->dir ? (node->output > node->input[4])
				: (node->output < node->input[4])) node->output=node->input[4];
	}
	else
	{
		context->last_en = 0;
		// Disabled so clamp to output
		node->output=node->input[5];
	}
	return 0;
}

int dst_ramp_reset(struct node_description *node)
{
	struct dss_ramp_context *context;
	context=(struct dss_ramp_context*)node->context;

	node->output=node->input[5];
	context->step = node->input[2] / Machine->sample_rate;
	context->dir = ((node->input[4] - node->input[3]) == abs(node->input[4] - node->input[3]));
	context->last_en = 0;
	return 0;
}

int dst_ramp_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_ramp_context)))==NULL)
	{
		discrete_log("dss_ramp_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_ramp_context));
	}

	/* Initialise the object */
	dst_ramp_reset(node);
	return 0;
}



/************************************************************************/
/*                                                                      */
/* dst_oneshot - Usage of node_description values for one shot pulse    */
/*                                                                      */
/* input[0]    - Enable input value                                     */
/* input[1]    - Trigger value                                          */
/* input[2]    - Reset value                                            */
/* input[3]    - Amplitude value                                        */
/* input[4]    - Width of oneshot pulse                                 */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_oneshot_step(struct node_description *node)
{
	struct dst_oneshot_context *context;
	context=(struct dst_oneshot_context*)node->context;

	/* Check state */
	switch(context->state)
	{
		case 0:		/* Waiting for trigger */
			if(node->input[1])
			{
				context->state=1;
				context->countdown=node->input[4];
				node->output=node->input[3];
			}
		 	node->output=0;
			break;

		case 1:		/* Triggered */
			node->output=node->input[3];
			if(node->input[1] && node->input[2])
			{
				// Dont start the countdown if we're still triggering
				// and we've got a reset signal as well
			}
			else
			{
				context->countdown-=context->stepsize;
				if(context->countdown<0.0)
				{
					context->countdown=0;
					node->output=0;
					context->state=2;
				}
			}
			break;

		case 2:		/* Waiting for reset */
		default:
			if(node->input[2]) context->state=0;
		 	node->output=0;
			break;
	}
	return 0;
}


int dst_oneshot_reset(struct node_description *node)
{
	struct dst_oneshot_context *context=(struct dst_oneshot_context*)node->context;
	context->countdown=0;
	context->stepsize=1.0/Machine->sample_rate;
	context->state=0;
 	node->output=0;
 	return 0;
}

int dst_oneshot_init(struct node_description *node)
{
	discrete_log("dst_oneshot_init() - Creating node %d.",node->node-NODE_00);

	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dst_oneshot_context)))==NULL)
	{
		discrete_log("dst_oneshot_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dst_oneshot_context));
	}

	/* Initialise the object */
	dst_oneshot_reset(node);

	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_CLAMP - Simple signal clamping circuit                           */
/*                                                                      */
/* input[0]    - Enable ramp                                            */
/* input[1]    - Input value                                            */
/* input[2]    - Minimum value                                          */
/* input[3]    - Maximum value                                          */
/* input[4]    - Clamp when disabled                                    */
/*                                                                      */
/************************************************************************/
int dst_clamp_step(struct node_description *node)
{
	struct dss_ramp_context *context;
	context=(struct dss_ramp_context*)node->context;

	if(node->input[0])
	{
		if(node->input[1] < node->input[2]) node->output=node->input[2];
		else if(node->input[1] > node->input[3]) node->output=node->input[3];
		else node->output=node->input[1];
	}
	else
	{
		node->output=node->input[4];
	}
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DST_LADDER - Resistor ladder emulation complete with capacitor       */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - voltage high rail                                      */
/* input[2]    - binary bit selector                                    */
/* input[3]    - gain                                                   */
/* input[4]    - offset                                                 */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_ladder_step(struct node_description *node)
{
	struct dst_ladder_context *context;
	int select,loop;
	double onres,demand,diff;
	context=(struct dst_ladder_context*)node->context;

	/* Work out which resistors are "ON" and then use this sum as a ratio to total resistance */
	/* and the output is that proportion of "voltage high rail" */
	select=(int)node->input[2];
	
	/* Clamp at max poss value */
	if(select>((1<<DISC_LADDER_MAXRES)-1)) select=(1<<DISC_LADDER_MAXRES)-1;
	if(select<0) select=0;

	/* Sum the overall resistance for later use */
	onres=0;
	for(loop=0;loop<DISC_LADDER_MAXRES;loop++)
	{
		if(select&0x01) onres+=((struct discrete_ladder*)node->custom)->resistors[loop];
		select=select>>1;
	}

	/* Work out demanded value */
	demand=node->input[1]*(onres/context->total_resistance);
	/* Add gain & offset */
	demand*=node->input[3];
	demand+=node->input[4];

	/* Now discrete RC filter it if required */
	if(((struct discrete_ladder*)node->custom)->smoothing_res != 0.0)
	{
		diff = demand-node->output;
		diff = diff -(diff * exp(context->step/context->exponent));
		node->output+=diff;
	}
	else
	{
		node->output=demand;
	}

	return 0;
}

int dst_ladder_reset(struct node_description *node)
{
	struct dst_ladder_context *context;
	int loop;
	context=(struct dst_ladder_context*)node->context;
	/* Sum the overall resistance for later use */
	for(loop=0;loop<DISC_LADDER_MAXRES;loop++)
	{
		context->total_resistance+=((struct discrete_ladder*)node->custom)->resistors[loop];
	}
	node->output=node->input[4];

	/* Setup filter constants */
	context->state = 0;
	context->t = 0;
	context->step = 1.0 / Machine->sample_rate;
	context->exponent=-1.0 * ((struct discrete_ladder*)node->custom)->smoothing_cap * ((struct discrete_ladder*)node->custom)->smoothing_res;

	return 0;
}

int dst_ladder_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dst_ladder_context)))==NULL)
	{
		discrete_log("dst_ladder_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dst_ladder_context));
	}

	/* Initialise the object */
	dst_ladder_reset(node);
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_SAMPHOLD - Sample & Hold Implementation                          */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - clock node                                             */
/* input[3]    - clock type                                             */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_samphold_step(struct node_description *node)
{
	struct dst_samphold_context *context;
	context=(struct dst_samphold_context*)node->context;

	if(node->input[0])
	{
		switch(context->clocktype)
		{
			case DISC_SAMPHOLD_REDGE:
				/* Clock the whole time the input is rising */
				if(node->input[2] > context->lastinput) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_FEDGE:
				/* Clock the whole time the input is falling */
				if(node->input[2] < context->lastinput) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_HLATCH:
				/* Output follows input if clock != 0 */
				if(node->input[2]) node->output=node->input[1];
				break;
			case DISC_SAMPHOLD_LLATCH:
				/* Output follows input if clock == 0 */
				if(node->input[2]==0) node->output=node->input[1];
				break;
			default:
				discrete_log("dst_samphold_step - Invalid clocktype passed");
				break;
		}
	}
	else
	{
		node->output=0;
	}
	/* Save the last value */
	context->lastinput=node->input[2];
	return 0;
}

int dst_samphold_reset(struct node_description *node)
{
	struct dst_samphold_context *context;
	context=(struct dst_samphold_context*)node->context;

	node->output=0;
	context->lastinput=-1;
	/* Only stored in here to speed up and save casting in the step function */
	context->clocktype=(int)node->input[3];
	dst_samphold_step(node);
	return 0;
}

int dst_samphold_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dst_samphold_context)))==NULL)
	{
		discrete_log("dss_rcdisc2_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dst_samphold_context));
	}

	/* Initialise the object */
	dst_samphold_reset(node);
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_INV - Logic invertor gate implementation                   */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - NOT USED                                               */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_inv_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_AND - Logic AND gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_and_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] && node->input[2] && node->input[3] && node->input[4])?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NAND - Logic NAND gate implementation                      */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_nand_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] && node->input[2] && node->input[3] && node->input[4])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_OR  - Logic OR  gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_or_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] || node->input[2] || node->input[3] || node->input[4])?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NOR - Logic NOR gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - input[2] value                                         */
/* input[4]    - input[3] value                                         */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_nor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=(node->input[1] || node->input[2] || node->input[3] || node->input[4])?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_XOR - Logic XOR gate implementation                        */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_xor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=((node->input[1] && !node->input[2]) || (!node->input[1] && node->input[2]))?1.0:0.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}

/************************************************************************/
/*                                                                      */
/* DST_LOGIC_NXOR - Logic NXOR gate implementation                      */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - input[0] value                                         */
/* input[2]    - input[1] value                                         */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dst_logic_nxor_step(struct node_description *node)
{
	if(node->input[0])
	{
		node->output=((node->input[1] && !node->input[2]) || (!node->input[1] && node->input[2]))?0.0:1.0;
	}
	else
	{
		node->output=0.0;
	}
	return 0;
}
