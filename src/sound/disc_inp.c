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
/* DSS_INPUT             - Memory Mapped input device                   */
/* DSS_CONSTANT          - Node based constand - Do we need this ???    */
/* DSS_ADJUSTER          - UI Mapped adjustable input                   */
/*                                                                      */
/************************************************************************/

#define DSS_INPUT_SPACE	0x1000

static struct node_description **dss_input_map=NULL;

static int *dss_adjustment_map=NULL;

struct dss_adjustment_context
{
	double value;
};



WRITE_HANDLER(discrete_sound_w)
{
	/* Bring the system upto now */
	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		dss_input_map[offset]->input[0]=data;
	}
}

READ_HANDLER(discrete_sound_r)
{
	int data=0;

	discrete_sh_update();
	/* Mask the memory offset to stay in the space allowed */
	offset&=(DSS_INPUT_SPACE-1);
	/* Update the node input value if allowed */
	if(dss_input_map[offset])
	{
		data=dss_input_map[offset]->input[0];
	}
    return data;
}


/************************************************************************/
/*                                                                      */
/* DSS_INPUT    - This is a programmable gain module with enable funct  */
/*                                                                      */
/* input[0]    - Constant value                                         */
/* input[1]    - Address value                                          */
/* input[2]    - Address mask                                           */
/* input[3]    - Gain value                                             */
/* input[4]    - Offset value                                           */
/* input[5]    - Starting Position                                      */
/*                                                                      */
/************************************************************************/
int dss_input_step(struct node_description *node)
{
	node->output=(node->input[0]*node->input[3])+node->input[4];
	return 0;
}

int dss_input_reset(struct node_description *node)
{
	node->input[0]=node->input[5];
	dss_input_step(node);
	return 0;
}

int dss_input_pulse_step(struct node_description *node)
{
	/* Set a valid output */
	node->output=(node->input[0]*node->input[3])+node->input[4];
	/* Reset the input to default for the next cycle */
	/* node order is now important */
	node->input[0]=node->input[5];
	return 0;
}

int dss_input_init(struct node_description *node)
{
	int loop,addr,mask;
	/* We just allocate memory on the first call */
	if(dss_input_map==NULL)
	{
		if((dss_input_map=malloc(DSS_INPUT_SPACE*sizeof(struct node_description*)))==NULL) return 1;
		memset(dss_input_map,0,DSS_INPUT_SPACE*sizeof(struct node_description*));
	}

	/* Initialise the input mapping array for this particular node */
	addr=((int)node->input[1])&(DSS_INPUT_SPACE-1);
	mask=((int)node->input[2])&(DSS_INPUT_SPACE-1);
	for(loop=0;loop<DSS_INPUT_SPACE;loop++)
	{
		if((loop&mask)==addr) dss_input_map[loop]=node;
	}
	dss_input_reset(node);
	return 0;
}

int dss_input_kill(struct node_description *node)
{
	free(dss_input_map);
	dss_input_map=NULL;
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_CONSTANT - This is a programmable gain module with enable funct  */
/*                                                                      */
/* input[0]    - Constant value                                         */
/* input[1]    - NOT USED                                               */
/* input[2]    - NOT USED                                               */
/* input[3]    - NOT USED                                               */
/* input[4]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dss_constant_step(struct node_description *node)
{
	node->output=node->input[0];
	return 0;
}


/************************************************************************/
/*                                                                      */
/* DSS_ADJUSTMENT - UI Adjustable constant node to emulate trimmers     */
/*                                                                      */
/* input[0]    - Enable                                                 */
/* input[1]    - Minimum value                                          */
/* input[2]    - Maximum value                                          */
/* input[3]    - Default value                                          */
/* input[4]    - Log/Linear 0=Linear !0=Log                             */
/* input[5]    - NOT USED                                               */
/*                                                                      */
/************************************************************************/
int dss_adjustment_step(struct node_description *node)
{
	struct dss_adjustment_context *context=(struct dss_adjustment_context*)node->context;
	node->output=context->value;
	return 0;
}

int dss_adjustment_reset(struct node_description *node)
{
	struct dss_adjustment_context *context=(struct dss_adjustment_context*)node->context;
	context->value=node->input[3];
	dss_adjustment_step(node);
	return 0;
}

int dss_adjustment_init(struct node_description *node)
{
	/* Allocate memory for the context array and the node execution order array */
	if((node->context=malloc(sizeof(struct dss_adjustment_context)))==NULL)
	{
		discrete_log("dss_adjustment_init() - Failed to allocate local context memory.");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node->context,0,sizeof(struct dss_adjustment_context));
	}

	dss_adjustment_reset(node);
	return 0;
}


int  discrete_sh_adjuster_count(struct discrete_sound_block *dsintf)
{
	int node_counter=0;
	int count=0;

	if(dss_adjustment_map!=NULL) free(dss_adjustment_map);

	if((dss_adjustment_map=malloc(sizeof(int)*DISCRETE_MAX_ADJUSTERS))==NULL) return -1;
	for(count=0;count<DISCRETE_MAX_ADJUSTERS;count++) dss_adjustment_map[count]=0;

	count=0;
	while(1)
	{
		int sanity_abort=0;
		/* Check the node parameter is a valid node */
		if(dsintf[count].node<NODE_START || dsintf[count].node>NODE_END) return -1;
		if(sanity_abort++>255) return -1;
		/* Only count adjustment nodes */
		if(dsintf[count].type==DSS_ADJUSTMENT) dss_adjustment_map[node_counter++]=count;
		/* Abort on either END node */
		if(dsintf[count].type==DSS_NULL) break;
		count++;
	}
	return node_counter;
}

int  discrete_sh_adjuster_get(int arg,struct discrete_sh_adjuster *adjuster)
{
	int node=0;
	if(adjuster==NULL) return -1;

	/* Reference our node in the running list */
	node=dss_adjustment_map[arg];

	/* Sanity check */
	if(node<0 || node>DISCRETE_MAX_NODES) return -1;

	adjuster->name=node_list[node].name;
	adjuster->initial=node_list[node].input[3];
	adjuster->min=node_list[node].input[1];
	adjuster->max=node_list[node].input[2];
	adjuster->value=((struct dss_adjustment_context*)node_list[node].context)->value;
	adjuster->islogscale=(int)node_list[node].input[4];

	return arg;
}

int discrete_sh_adjuster_set(int arg,struct discrete_sh_adjuster *adjuster)
{
	int node=0;
	if(adjuster==NULL) return -1;

	/* Reference our node in the running list */
	node=dss_adjustment_map[arg];

	/* Sanity check */
	if(node<0 || node>DISCRETE_MAX_NODES) return -1;

	/* Only allow output value to be set */

   /* Only allow value to be set */
/*	node_list[node].name=adjuster->name; */
/*	node_list[node].input[3]=adjuster->initial; */
/*	node_list[node].input[1]=adjuster->min; */
/*	node_list[node].input[2]=adjuster->max; */
/*	node_list[node].input[4]=adjuster->islogscale; */
	((struct dss_adjustment_context*)node_list[node].context)->value=adjuster->value;

	return arg;
}
