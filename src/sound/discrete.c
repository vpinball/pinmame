#include "driver.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/************************************************************************/
/*                                                                      */
/*  MAME - Discrete sound system emulation library                      */
/*                                                                      */
/*  Written by Keith Wilkins (mame@esplexo.co.uk)                       */
/*                                                                      */
/*  (c) K.Wilkins 2000                                                  */
/*                                                                      */
/*  Coding started in November 2000                                     */
/*  KW - Added Sawtooth waveforms  Feb2003                              */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* SEE DISCRETE.H for documentation on usage                            */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* Each sound primative DSS_xxxx or DST_xxxx has its own implementation */
/* file. All discrete sound primatives MUST implement the following     */
/* API:                                                                 */
/*                                                                      */
/* dsX_NAME_init() returns pointer to context or -1 for failure         */
/* dsX_NAME_step(inputs, context,float timestep)  - Perform time step   */
/*                                                  return output value */
/* dsX_NAME_kill(context) - Free context memory and return              */
/* dsX_NAME_reset(context) - Reset to initial state                     */
/*                                                                      */
/* Core software takes care of traversing the netlist in the correct    */
/* order                                                                */
/*                                                                      */
/* discrete_sh_start()       - Read Node list, initialise & reset       */
/* discrete_sh_stop()        - Shutdown discrete sound system           */
/* discrete_sh_reset()       - Put sound system back to time 0          */
/* discrete_sh_update()      - Update streams to current time           */
/* discrete_stream_update()  - This does the real update to the sim     */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/* Context -  Memory pointer (void)                                     */
/*            Last output value                                         */
/*            Next object in list (Doubly linked?? or Array of struct)  */
/*                                                                      */
/* Initialisation -                                                     */
/*     Parse table into linked object list                              */
/*     Based on blocking/input tree sort list, perhaps the best         */
/*     solution is to traverse the netlist backwards to work out the    */
/*     dependancies, how will this react to feedback loops.             */
/*     Call init in order                                               */
/*                                                                      */
/* Runtime - Use streams interface and callback to update channels      */
/*     Calc number of steps and deltaT                                  */
/*       Traverse list with step for each object                        */
/*       Store output/s in streaming buffer                             */
/*     Repeat until all steps done                                      */
/*                                                                      */
/* Shutdown -                                                           */
/*     Tranverse list                                                   */
/*       Call _kill for each item                                       */
/*     Free core memory array list.                                     */
/*                                                                      */
/************************************************************************/

static int init_ok=0;
static struct node_description **running_order=NULL;
static int node_count=0;
static struct node_description *node_list=NULL;
static struct node_description *output_node=NULL;
static int discrete_stream=0;
static int discrete_stereo=0;

/* Uncomment this line to log discrete sound output to a file */
//#define DISCRETE_WAVELOG
/* Uncomment this line to log discrete sound debug log information to a file */
//#define DISCRETE_DEBUGLOG

#ifdef DISCRETE_WAVELOG
#include "wavwrite.h"
static void *wav_file;
#endif

#ifdef DISCRETE_DEBUGLOG

static FILE *disclogfile=NULL;

void CLIB_DECL discrete_log(const char *text, ...)
{
    va_list arg;
    va_start(arg,text);

    if(disclogfile)
	{
		vfprintf(disclogfile, text, arg);
		fprintf(disclogfile,"\n");
	}

    va_end(arg);
}
#else
void CLIB_DECL discrete_log(const char *text, ...)
{
}
#endif

/* Include simulation objects */
#include "disc_wav.c"		/* Wave sources   - SINE/SQUARE/NOISE/etc */
#include "disc_mth.c"		/* Math Devices   - ADD/GAIN/etc */
#include "disc_inp.c"		/* Input Devices  - INPUT/CONST/etc */
#include "disc_flt.c"		/* Filter Devices - RCF/HPF/LPF */
#include "disc_dev.c"		/* Popular Devices - NE555/etc */
#include "disc_out.c"		/* Output devices */

int dss_default_kill(struct node_description *node)
{
	if(node->context) free(node->context);
	node->context=NULL;
	return 0;
}

/************************************************************************/
/*                                                                      */
/*        Define the call tables for running the simulation,            */
/*        add your new node types into here to allow them to be         */
/*        called within the simulation environment.                     */
/*                                                                      */
/************************************************************************/
struct discrete_module module_list[]=
{
	{ DSS_INPUT       ,"DSS_INPUT"       ,dss_input_init       ,dss_input_kill       ,dss_input_reset       ,dss_input_step       },
	{ DSS_INPUT_PULSE ,"DSS_INPUT_PULSE" ,dss_input_init       ,dss_input_kill       ,dss_input_reset       ,dss_input_pulse_step },
	{ DSS_CONSTANT    ,"DSS_CONSTANT"    ,NULL                 ,NULL                 ,NULL                  ,dss_constant_step    },
	{ DSS_ADJUSTMENT  ,"DSS_ADJUSTMENT"  ,dss_adjustment_init  ,dss_default_kill     ,dss_adjustment_reset  ,dss_adjustment_step  },
	{ DSS_SQUAREWAVE  ,"DSS_SQUAREWAVE"  ,dss_squarewave_init  ,dss_default_kill     ,dss_squarewave_reset  ,dss_squarewave_step  },
	{ DSS_SQUAREWFIX  ,"DSS_SQUAREWFIX"  ,dss_squarewfix_init  ,dss_default_kill     ,dss_squarewfix_reset  ,dss_squarewfix_step  },
	{ DSS_SQUAREWAVE2 ,"DSS_SQUAREWAVE2" ,dss_squarewave2_init ,dss_default_kill     ,dss_squarewave2_reset ,dss_squarewave2_step  },
	{ DSS_SINEWAVE    ,"DSS_SINEWAVE"    ,dss_sinewave_init    ,dss_default_kill     ,dss_sinewave_reset    ,dss_sinewave_step    },
	{ DSS_NOISE       ,"DSS_NOISE"       ,dss_noise_init       ,dss_default_kill     ,dss_noise_reset       ,dss_noise_step       },
	{ DSS_LFSR_NOISE  ,"DSS_LFSR_NOISE"  ,dss_lfsr_init        ,dss_default_kill     ,dss_lfsr_reset        ,dss_lfsr_step        },
	{ DSS_TRIANGLEWAVE,"DSS_TRIANGLEWAVE",dss_trianglewave_init,dss_default_kill     ,dss_trianglewave_reset,dss_trianglewave_step},
	{ DSS_SAWTOOTHWAVE,"DSS_SAWTOOTHWAVE",dss_sawtoothwave_init,dss_default_kill     ,dss_sawtoothwave_reset,dss_sawtoothwave_step},
	{ DSS_ADSR        ,"DSS_ADSR"        ,dss_adsrenv_init     ,dss_default_kill     ,dss_adsrenv_reset     ,dss_adsrenv_step     },

	{ DST_TRANSFORM   ,"DST_TRANSFORM"   ,NULL                 ,NULL                 ,NULL                  ,dst_transform_step   },
	{ DST_GAIN        ,"DST_GAIN"        ,NULL                 ,NULL                 ,NULL                  ,dst_gain_step        },
	{ DST_DIVIDE      ,"DST_DIVIDE"      ,NULL                 ,NULL                 ,NULL                  ,dst_divide_step      },
	{ DST_ADDER       ,"DST_ADDER"       ,NULL                 ,NULL                 ,NULL                  ,dst_adder_step       },
	{ DST_SWITCH      ,"DST_SWITCH"      ,NULL                 ,NULL                 ,NULL                  ,dst_switch_step      },
	{ DST_RCFILTER    ,"DST_RCFILTER"    ,dst_rcfilter_init    ,NULL                 ,dst_rcfilter_reset    ,dst_rcfilter_step    },
	{ DST_RCDISC      ,"DST_RCDISC"      ,dst_rcdisc_init      ,dss_default_kill     ,dst_rcdisc_reset      ,dst_rcdisc_step      },
	{ DST_RCDISC2     ,"DST_RCDISC2"     ,dst_rcdisc2_init     ,dss_default_kill     ,dst_rcdisc2_reset     ,dst_rcdisc2_step     },
	{ DST_RCFILTERN   ,"DST_RCFILTERN"   ,dst_rcfilterN_init   ,dss_default_kill     ,dst_filter1_reset     ,dst_filter1_step     },
	{ DST_RCDISCN     ,"DST_RCDISCN"     ,dst_rcdiscN_init     ,dss_default_kill     ,dst_filter1_reset     ,dst_rcdiscN_step      },
	{ DST_RCDISC2N    ,"DST_RCDISC2N"    ,dst_rcdisc2N_init    ,dss_default_kill     ,dst_rcdisc2N_reset    ,dst_rcdisc2N_step     },
	{ DST_FILTER1     ,"DST_FILTER1"     ,dst_filter1_init     ,dss_default_kill     ,dst_filter1_reset     ,dst_filter1_step     },
	{ DST_FILTER2     ,"DST_FILTER2"     ,dst_filter2_init     ,dss_default_kill     ,dst_filter2_reset     ,dst_filter2_step     },

	{ DST_RAMP        ,"DST_RAMP"        ,dst_ramp_init        ,dss_default_kill     ,dst_ramp_reset        ,dst_ramp_step        },
	{ DST_CLAMP       ,"DST_CLAMP"       ,NULL                 ,NULL                 ,NULL                  ,dst_clamp_step       },
	{ DST_LADDER      ,"DST_LADDER"      ,dst_ladder_init      ,dss_default_kill     ,dst_ladder_reset      ,dst_ladder_step      },
	{ DST_ONESHOT     ,"DST_ONESHOT"     ,dst_oneshot_init     ,dss_default_kill     ,dst_oneshot_reset     ,dst_oneshot_step     },
	{ DST_SAMPHOLD    ,"DST_SAMPHOLD"    ,dst_samphold_init    ,dss_default_kill     ,dst_samphold_reset    ,dst_samphold_step    },

	{ DST_LOGIC_INV   ,"DST_LOGIC_INV"   ,NULL                 ,NULL                 ,NULL                  ,dst_logic_inv_step   },
	{ DST_LOGIC_AND   ,"DST_LOGIC_AND"   ,NULL                 ,NULL                 ,NULL                  ,dst_logic_and_step   },
	{ DST_LOGIC_NAND  ,"DST_LOGIC_NAND"  ,NULL                 ,NULL                 ,NULL                  ,dst_logic_nand_step  },
	{ DST_LOGIC_OR    ,"DST_LOGIC_OR"    ,NULL                 ,NULL                 ,NULL                  ,dst_logic_or_step    },
	{ DST_LOGIC_NOR   ,"DST_LOGIC_NOR"   ,NULL                 ,NULL                 ,NULL                  ,dst_logic_nor_step   },
	{ DST_LOGIC_XOR   ,"DST_LOGIC_XOR"   ,NULL                 ,NULL                 ,NULL                  ,dst_logic_xor_step   },
	{ DST_LOGIC_NXOR  ,"DST_LOGIC_NXOR"  ,NULL                 ,NULL                 ,NULL                  ,dst_logic_nxor_step  },

	{ DSD_555_ASTBL   ,"DSD_555_ASTBL"   ,dsd_555_astbl_init   ,dss_default_kill     ,dsd_555_astbl_reset   ,dsd_555_astbl_step   },
	{ DSD_SQUAREW555  ,"DSD_SQUAREW555"  ,dsd_squarew555_init  ,dss_default_kill     ,dsd_squarew555_reset  ,dsd_squarew555_step  },
	{ DSD_SQUAREW566  ,"DSD_SQUAREW566"  ,dsd_squarew566_init  ,dss_default_kill     ,dsd_squarew566_reset  ,dsd_squarew566_step  },
	{ DSD_TRIANGLEW566,"DSD_TRIANGLEW566",dsd_trianglew566_init,dss_default_kill     ,dsd_trianglew566_reset,dsd_trianglew566_step},

	{ DSO_OUTPUT      ,"DSO_OUTPUT"      ,dso_output_init      ,NULL                 ,NULL                  ,dso_output_step      },
	{ DSS_NULL        ,"DSS_NULL"        ,NULL                 ,NULL                 ,NULL                  ,NULL                 }
};


static struct node_description* find_node(int node)
{
	int loop;
	for(loop=0;loop<node_count;loop++)
	{
		if(node_list[loop].node==node) return &node_list[loop];
	}
	return NULL;
}

static void discrete_stream_update_stereo(int ch, INT16 **buffer, int length)
{
	/* Now we must do length iterations of the node list, one output for each step */
	int loop,loop2,loop3;
	struct node_description *node;

	for(loop=0;loop<length;loop++)
	{
		for(loop2=0;loop2<node_count;loop2++)
		{
			/* Pick the first node to process */
			node=running_order[loop2];

			/* Work out what nodes/inputs are required, dont process NO CONNECT nodes */
			/* these are ones that are connected to NODE_LIST[0]                      */
			for(loop3=0;loop3<node->active_inputs;loop3++)
			{
				if(node->input_node[loop3] && (node->input_node[loop3])->node!=NODE_NC) node->input[loop3]=(node->input_node[loop3])->output;
			}
			/* Now step the node */
			if(module_list[node->module].step) (*module_list[node->module].step)(node);
		}

		/* Now put the output into the buffers */
		buffer[0][loop]=((struct dso_output_context*)(output_node->context))->left;
		buffer[1][loop]=((struct dso_output_context*)(output_node->context))->right;
	}
#ifdef DISCRETE_WAVELOG
	wav_add_data_16lr(wav_file, buffer[0],buffer[1], length);
#endif
}

static void discrete_stream_update_mono(int ch,INT16 *buffer, int length)
{
	/* Now we must do length iterations of the node list, one output for each step */
	int loop,loop2,loop3;
	struct node_description *node;

	for(loop=0;loop<length;loop++)
	{
		for(loop2=0;loop2<node_count;loop2++)
		{
			/* Pick the first node to process */
			node=running_order[loop2];

			/* Work out what nodes/inputs are required, dont process NO CONNECT nodes */
			/* these are ones that are connected to NODE_LIST[0]                      */
			for(loop3=0;loop3<node->active_inputs;loop3++)
			{
				if(node->input_node[loop3] && (node->input_node[loop3])->node!=NODE_NC) node->input[loop3]=(node->input_node[loop3])->output;
			}

			/* Now step the node */
			if(module_list[node->module].step) (*module_list[node->module].step)(node);
		}

		/* Now put the output into the buffer */
		buffer[loop]=(((struct dso_output_context*)(output_node->context))->left+((struct dso_output_context*)(output_node->context))->right)/2;
	}
#ifdef DISCRETE_WAVELOG
	wav_add_data_16(wav_file, buffer, length);
#endif
}

void discrete_sh_reset(void)
{
	struct node_description *node;

	/* Reset all of the objects */
	int loop=0,loop2=0;

	if(!init_ok) return;

	for(loop=0;loop<node_count;loop++)
	{
		/* Pick the first node to process */
		node=running_order[loop];

		/* Work out what nodes/inputs are required, dont process NO CONNECT nodes */
		/* these are ones that are connected to NODE_LIST[0]                      */
		for(loop2=0;loop2<node->active_inputs;loop2++)
		{
			if(node->input_node[loop2] && (node->input_node[loop2])->node!=NODE_NC) node->input[loop2]=(node->input_node[loop2])->output;
		}

		/* Now that the inputs have been setup then we should call the reset function */
		/* if a node is stateless then it may have no reset function in which case we */
		/* will call its _step function to setup the output.                          */
		if(module_list[node_list[loop].module].reset)
		{
			discrete_log("discrete_sh_reset() - Calling reset for %s node %d.",module_list[node_list[loop].module].name,node_list[loop].node-NODE_00);
			(*module_list[node_list[loop].module].reset)(&node_list[loop]);
		}
		else if (module_list[node_list[loop].module].step)
		{
			discrete_log("discrete_sh_reset() - Node has no reset, calling step for %s node %d.",module_list[node_list[loop].module].name,node_list[loop].node-NODE_00);
			(*module_list[node_list[loop].module].step)(&node_list[loop]);
		}	
	}
}

int discrete_sh_start (const struct MachineSound *msound)
{
	struct discrete_sound_block *intf;
	int loop=0,loop2=0,search=0,failed=0;

#ifdef DISCRETE_WAVELOG
	wav_file = wav_open("discrete.wav", Machine->sample_rate, ((Machine->drv->sound_attributes&SOUND_SUPPORTS_STEREO) == SOUND_SUPPORTS_STEREO) ? 2: 1);
#endif
#ifdef DISCRETE_DEBUGLOG
	if(!disclogfile) disclogfile=fopen("discrete.log", "w");
#endif

	/* Initialise */
	intf=msound->sound_interface;
	node_count=0;

	/* Sanity check and node count */
	discrete_log("discrete_sh_start() - Doing node list sanity check");
	while(1)
	{
		/* Check the node parameter is a valid node */
		if(intf[node_count].node<NODE_START || intf[node_count].node>NODE_END)
		{
			logerror("discrete_sh_start() - Invalid node number on node %02d descriptor\n",node_count);
			return 1;
		}
		if(intf[node_count].type>DSO_OUTPUT)
		{
			logerror("discrete_sh_start() - Invalid function type on node %02d descriptor\n",node_count);
			return 1;
		}

		/* Node count must include the NULL node as well */
		if(intf[node_count].type==DSS_NULL)
		{
			node_count++;
			break;
		}

		node_count++;

		/* Sanity check */
		if(node_count>DISCRETE_MAX_NODES)
		{
			logerror("discrete_sh_start() - Upper limit of %d nodes exceeded, have you terminated the interface block.",DISCRETE_MAX_NODES);
			return 1;
		}
	}
	discrete_log("discrete_sh_start() - Sanity check counted %d nodes", node_count);

	/* Allocate memory for the context array and the node execution order array */
	if((running_order=malloc(node_count*sizeof(struct node_description*)))==NULL)
	{
		logerror("discrete_sh_start() - Failed to allocate running order array.\n");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(running_order,0,node_count*sizeof(struct node_description*));
	}

	if((node_list=malloc(node_count*sizeof(struct node_description)))==NULL)
	{
		logerror("discrete_sh_start() - Failed to allocate context list array.\n");
		return 1;
	}
	else
	{
		/* Initialise memory */
		memset(node_list,0,node_count*sizeof(struct node_description));

		/* Initialise structs */
		for(loop=0;loop<node_count;loop++)
		{
			for(loop2=0;loop2<DISCRETE_MAX_INPUTS;loop2++)
			{
				node_list[loop].input[loop2]=0.0;
				node_list[loop].input_node[loop2]=NULL;
			}
		}


	}
	discrete_log("discrete_sh_start() - Malloc completed", node_count);

	/* Work out the execution order */
	/* FAKE IT FOR THE MOMENT, EXECUTE IN ORDER */
	for(loop=0;loop<node_count;loop++)
	{
		running_order[loop]=&node_list[loop];
	}
	discrete_log("discrete_sh_start() - Running order sort completed", node_count);

	/* Configure the input node pointers, the find_node function wont work without the node ID setup beforehand */
	for(loop=0;loop<node_count;loop++) node_list[loop].node=intf[loop].node;
	failed=0;

	/* Duplicate node number test */
	for(loop=0;loop<node_count;loop++)
	{
		for(loop2=0;loop2<node_count;loop2++)
		{
			if(node_list[loop].node==node_list[loop2].node && loop!=loop2)
			{
				logerror("discrete_sh_start - Node NODE_%02d defined more than once\n",node_list[loop].node-NODE_00);
				failed=1;
			}
		}
	}

	/* Initialise and start all of the objects */
	for(loop=0;loop<node_count;loop++)
	{
		/* Configure the input node pointers */
		node_list[loop].node=intf[loop].node;
		node_list[loop].output=0;
		node_list[loop].active_inputs=intf[loop].active_inputs;
		for(loop2=0;loop2<intf[loop].active_inputs;loop2++)
		{
			node_list[loop].input[loop2]=intf[loop].initial[loop2];
			node_list[loop].input_node[loop2]=find_node(intf[loop].input_node[loop2]);
		}
		node_list[loop].name=intf[loop].name;
		node_list[loop].custom=intf[loop].custom;

		/* Check that all referenced nodes have actually been found */
		for(loop2=0;loop2<intf[loop].active_inputs;loop2++)
		{
			if(node_list[loop].input_node[loop2]==NULL && intf[loop].input_node[loop2]>=NODE_START && intf[loop].input_node[loop2]<=NODE_END)
			{
				logerror("discrete_sh_start - Node NODE_%02d referenced a non existant node NODE_%02d\n",node_list[loop].node-NODE_00,intf[loop].input_node[loop2]-NODE_00);
				failed=1;
			}
		}

		/* Try to find the simulation module in the module list table */
		search=0;
		while(1)
		{
			if(module_list[search].type==intf[loop].type)
			{
				node_list[loop].module=search;
				discrete_log("discrete_sh_start() - Calling init for %s",module_list[search].name);
				if(module_list[search].init)
					if(((*module_list[search].init)(&node_list[loop]))==1) failed=1;
				break;
			}
			else if(module_list[search].type==DSS_NULL)
			{
				if(intf[loop].type==DSS_NULL) break;
				else
				{
					logerror("discrete_sh_start() - Invalid DSS/DST/DSO module type specified in interface, item %02d\n",loop+1);
					failed=1;
					break;
				}
			}
			search++;
		}
	}
	/* Setup the output node */
	if((output_node=find_node(NODE_OP))==NULL)
	{
		logerror("discrete_sh_start() - Couldn't find an output node");
		failed=1;
	}

	discrete_log("discrete_sh_start() - Nodes initialised", node_count);

	/* Different setup for Mono/Stereo systems */
	if ((Machine->drv->sound_attributes&SOUND_SUPPORTS_STEREO) == SOUND_SUPPORTS_STEREO)
	{
		int vol[2];
		const char *stereo_names[2] = { "Discrete Left", "Discrete Right" };
		vol[0] = MIXER((int)output_node->input[2],MIXER_PAN_LEFT);
		vol[1] = MIXER((int)output_node->input[2],MIXER_PAN_RIGHT);
		/* Initialise a stereo, stream, we always use stereo even if only a mono system */
		discrete_stream=stream_init_multi(2,stereo_names,vol,Machine->sample_rate,0,discrete_stream_update_stereo);
		discrete_log("discrete_sh_start() - Stereo Audio Stream Initialised", node_count);
		discrete_stereo=1;
	}
	else
	{
		int vol;
		vol = output_node->input[2];
		/* Initialise a stereo, stream, we always use stereo even if only a mono system */
		discrete_stream=stream_init("Discrete Sound",vol,Machine->sample_rate,0,discrete_stream_update_mono);
		discrete_log("discrete_sh_start() - Mono Audio Stream Initialised", node_count);
	}

	if(discrete_stream==-1)
	{
		logerror("discrete_sh_start - Stream init returned an error\n");
		failed=1;
	}

	/* Report success or fail */
	if(!failed) init_ok=1;

	/* Now reset the system to a sensible state */
	discrete_sh_reset();
	discrete_log("discrete_sh_start() - Nodes reset", node_count);

	return failed;
}

void discrete_sh_stop (void)
{
	int loop=0;
	if(!init_ok) return;

#ifdef DISCRETE_WAVELOG
	wav_close(wav_file);
#endif

	for(loop=0;loop<node_count;loop++)
	{
	/* Destruct all of the objects */
		discrete_log("discrete_sh_stop() - Calling stop for %s",module_list[node_list[loop].module].name);
		if(module_list[node_list[loop].module].kill) (*module_list[node_list[loop].module].kill)(&node_list[loop]);
	}
	if(node_list) free(node_list);
	if(running_order) free(running_order);
	node_count=0;
	node_list=NULL;
	running_order=NULL;

#ifdef DISCRETE_DEBUGLOG
    if(disclogfile) fclose(disclogfile);
	disclogfile=NULL;
#endif
}

void discrete_sh_update(void)
{
	if(!init_ok) return;

	/* Bring stream upto the present time */
	stream_update(discrete_stream, 0);
}
