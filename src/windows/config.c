
/*
 * Configuration routines.
 *
 * 20010424 BW uses Hans de Goede's rc subsystem
 * last changed 20010727 BW
 *
 * TODO:
 * - make errorlog a ringbuffer
 *
 * Suggestions
 * - norotate? funny, leads to option -nonorotate ...
 *   fix when rotation options take turnable LCD's in account
 * - win_switch_res --> switch_resolution, swres
 * - win_switch_bpp --> switch_bpp, swbpp
 * - give up distinction between vector_width and win_gfx_width
 *   eventually introduce options.width, options.height
 * - new core options:
 *   gamma (is already osd_)
 *   sound (enable/disable sound)
 *   volume
 * - rename	options.use_emulated_ym3812 to options_use_real_ym3812;
 * - get rid of #ifdef MESS's by providing appropriate hooks
 */

#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "driver.h"
#include "rc.h"
#include "misc.h"

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif

extern struct rc_option frontend_opts[];
extern struct rc_option fileio_opts[];
extern struct rc_option input_opts[];
extern struct rc_option sound_opts[];
extern struct rc_option video_opts[];

#ifdef MESS
extern struct rc_option mess_opts[];
#endif

extern int frontend_help(char *gamename);
static int config_handle_arg(char *arg);

static FILE *logfile;
static int errorlog;
static int showconfig;
static int showusage;
static int readconfig;
static int createconfig;
extern int verbose;
#ifndef VPINMAME // VPM need to access the options
static
#endif /* VPINMAME */
struct rc_struct *rc;

/* fix me - need to have the core call osd_set_gamma with this value */
/* instead of relying on the name of an osd variable */
extern float gamma_correct;

/* fix me - need to have the core call osd_set_mastervolume with this value */
/* instead of relying on the name of an osd variable */
extern int attenuation;

static char *debugres;
static char *playbackname;
static char *recordname;
static char *gamename;

char *rompath_extra;

static float f_beam;
static float f_flicker;

static int enable_sound = 1;

static int video_set_beam(struct rc_option *option, const char *arg, int priority)
{
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	if (options.beam > 0x00100000)
		options.beam = 0x00100000;
	option->priority = priority;
	return 0;
}

static int video_set_flicker(struct rc_option *option, const char *arg, int priority)
{
	options.vector_flicker = (int)(f_flicker * 2.55);
	if (options.vector_flicker < 0)
		options.vector_flicker = 0;
	if (options.vector_flicker > 255)
		options.vector_flicker = 255;
	option->priority = priority;
	return 0;
}

static int video_set_debugres(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		options.debug_width = options.debug_height = 0;
	}
	else if(sscanf(arg, "%dx%d", &options.debug_width, &options.debug_height) != 2)
	{
		options.debug_width = options.debug_height = 0;
		fprintf(stderr, "error: invalid value for debugres: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg, int priority)
{
	if ((options.color_depth != 0) &&
		(options.color_depth != 8) &&
		(options.color_depth != 15) &&
		(options.color_depth != 16) &&
		(options.color_depth != 32))
	{
		options.color_depth = 0;
		fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

static int init_errorlog(struct rc_option *option, const char *arg, int priority)
{
	/* provide errorlog from here on */
	if (errorlog && !logfile)
	{
		logfile = fopen("error.log","wa");
		if (!logfile)
		{
			perror("unable to open log file\n");
			exit (1);
		}
	}
	option->priority = priority;
	return 0;
}


/* struct definitions */
#ifdef PINMAME
struct rc_option core_opts[];
struct rc_option pinmame_opts[] = {
	/* PinMAME options */
	{ "PinMAME options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "dmd_red",    NULL, rc_int, &pmoptions.dmd_red,   "255", 0, 255, NULL, "DMD color: Red" },
	{ "dmd_green",  NULL, rc_int, &pmoptions.dmd_green, "224", 0, 255, NULL, "DMD color: Green" },
	{ "dmd_blue",   NULL, rc_int, &pmoptions.dmd_blue,   "32", 0, 255, NULL, "DMD color: Blue" },
	{ "dmd_perc0",	NULL, rc_int, &pmoptions.dmd_perc0,  "20", 0, 100, NULL, "DMD off intensity [%]" },
	{ "dmd_perc33",	NULL, rc_int, &pmoptions.dmd_perc33,  "33", 0, 100, NULL, "DMD low intensity [%]" },
	{ "dmd_perc66", NULL, rc_int, &pmoptions.dmd_perc66,  "67", 0, 100, NULL, "DMD medium intensity [%]" },
	{ "dmd_only",	NULL, rc_bool,&pmoptions.dmd_only,    "0",  0, 0,   NULL, "Show only DMD" },
	{ "dmd_compact",NULL, rc_bool,&pmoptions.dmd_compact, "0",  0, 0,   NULL, "Show comact display" },
	{ "dmd_antialias", NULL, rc_int, &pmoptions.dmd_antialias,  "50", 0, 100, NULL, "DMD antialias intensity [%]" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};
#endif /* PINMAME */
static struct rc_option opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, frontend_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, video_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, sound_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, input_opts, NULL, 0,	0, NULL, NULL },
#ifdef PINMAME
	{ NULL, NULL, rc_link, pinmame_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, core_opts, NULL, 0,	0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};
struct rc_option core_opts[] = {
#endif /* PINMAME */

	/* options supported by the mame core */
	/* video */
	{ "Mame CORE video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "bpp", NULL, rc_int, &options.color_depth, "0",	0, 0, video_verify_bpp, "specify the colordepth the core should render in bits per pixel (bpp), one of: auto(0), 8, 16, 32" },
	{ "norotate", NULL, rc_bool , &options.norotate, "0", 0, 0, NULL, "do not apply rotation" },
	{ "ror", NULL, rc_bool, &options.ror, "0", 0, 0, NULL, "rotate screen clockwise" },
	{ "rol", NULL, rc_bool, &options.rol, "0", 0, 0, NULL, "rotate screen anti-clockwise" },
	{ "flipx", NULL, rc_bool, &options.flipx, "0", 0, 0, NULL, "flip screen upside-down" },
	{ "flipy", NULL, rc_bool, &options.flipy, "0", 0, 0, NULL, "flip screen left-right" },
	{ "debug_resolution", "dr", rc_string, &debugres, "auto", 0, 0, video_set_debugres, "set resolution for debugger window" },
	/* make it options.gamma_correction? */
	{ "gamma", NULL, rc_float, &gamma_correct , "1.0", 0.5, 2.0, NULL, "gamma correction"},

	/* vector */
	{ "Mame CORE vector game options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "antialias", "aa", rc_bool, &options.antialias, "1", 0, 0, NULL, "draw antialiased vectors" },
	{ "translucency", "tl", rc_bool, &options.translucency, "1", 0, 0, NULL, "draw translucent vectors" },
	{ "beam", NULL, rc_float, &f_beam, "1.0", 1.0, 16.0, video_set_beam, "set beam width in vector games" },
	{ "flicker", NULL, rc_float, &f_flicker, "0.0", 0.0, 100.0, video_set_flicker, "set flickering in vector games" },

	/* sound */
	{ "Mame CORE sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "samplerate", "sr", rc_int, &options.samplerate, "44100", 5000, 50000, NULL, "set samplerate" },
	{ "samples", NULL, rc_bool, &options.use_samples, "1", 0, 0, NULL, "use samples" },
	{ "resamplefilter", NULL, rc_bool, &options.use_filter, "1", 0, 0, NULL, "resample if samplerate does not match" },
	{ "sound", NULL, rc_bool, &enable_sound, "1", 0, 0, NULL, "enable/disable sound and sound CPUs" },
	{ "volume", "vol", rc_int, &attenuation, "0", -32, 0, NULL, "volume (range [-32,0])" },

	/* misc */
	{ "Mame CORE misc options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "artwork", "art", rc_bool, &options.use_artwork, "1", 0, 0, NULL, "use additional game artwork" },
	{ "cheat", "c", rc_bool, &options.cheat, "0", 0, 0, NULL, "enable/disable cheat subsystem" },
	{ "debug", "d", rc_bool, &options.mame_debug, "0", 0, 0, NULL, "enable/disable debugger (only if available)" },
	{ "playback", "pb", rc_string, &playbackname, NULL, 0, 0, NULL, "playback an input file" },
	{ "record", "rec", rc_string, &recordname, NULL, 0, 0, NULL, "record an input file" },
	{ "log", NULL, rc_bool, &errorlog, "0", 0, 0, init_errorlog, "generate error.log" },

	/* config options */
	{ "Configuration options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "createconfig", "cc", rc_set_int, &createconfig, NULL, 1, 0, NULL, "create the default configuration file" },
	{ "showconfig",	"sc", rc_set_int, &showconfig, NULL, 1, 0, NULL, "display running parameters in rc style" },
	{ "showusage", "su", rc_set_int, &showusage, NULL, 1, 0, NULL, "show this help" },
	{ "readconfig",	"rc", rc_bool, &readconfig, "1", 0, 0, NULL, "enable/disable loading of configfiles" },
	{ "verbose", "v", rc_bool, &verbose, "0", 0, 0, NULL, "display additional diagnostic information" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

/*
 * Penalty string compare, the result _should_ be a measure on
 * how "close" two strings ressemble each other.
 * The implementation is way too simple, but it sort of suits the
 * purpose.
 * This used to be called fuzzy matching, but there's no randomness
 * involved and it is in fact a penalty method.
 */

int penalty_compare (const char *s, const char *l)
{
	int gaps = 0;
	int match = 0;
	int last = 1;

	for (; *s && *l; l++)
	{
		if (*s == *l)
			match = 1;
		else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
			match = 1;
		else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
			match = 1;
		else
			match = 0;

		if (match)
			s++;

		if (match != last)
		{
			last = match;
			if (!match)
				gaps++;
		}
	}

	/* penalty if short string does not completely fit in */
	for (; *s; s++)
		gaps++;

	return gaps;
}

/*
 * We compare the game name given on the CLI against the long and
 * the short game names supported
 */
void show_approx_matches(void)
{
	struct { int penalty; int index; } topten[10];
	int i,j;
	int penalty; /* best fuzz factor so far */

	for (i = 0; i < 10; i++)
	{
		topten[i].penalty = 9999;
		topten[i].index = -1;
	}

	for (i = 0; (drivers[i] != 0); i++)
	{
		int tmp;

		penalty = penalty_compare (gamename, drivers[i]->description);
		tmp = penalty_compare (gamename, drivers[i]->name);
		if (tmp < penalty) penalty = tmp;

		/* eventually insert into table of approximate matches */
		for (j = 0; j < 10; j++)
		{
			if (penalty >= topten[j].penalty) break;
			if (j > 0)
			{
				topten[j-1].penalty = topten[j].penalty;
				topten[j-1].index = topten[j].index;
			}
			topten[j].index = i;
			topten[j].penalty = penalty;
		}
	}

	for (i = 9; i >= 0; i--)
	{
		if (topten[i].index != -1)
			fprintf (stderr, "%-10s%s\n", drivers[topten[i].index]->name, drivers[topten[i].index]->description);
	}
}

/*
 * gamedrv  = NULL --> parse named configfile
 * gamedrv != NULL --> parse gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int parse_config (const char* filename, const struct GameDriver *gamedrv)
{
	FILE *f;
	char buffer[128];
	int retval = 0;

	if (!readconfig) return 0;

	if (gamedrv)
	{
		if (gamedrv->clone_of && strlen(gamedrv->clone_of->name))
		{
			retval = parse_config (NULL, gamedrv->clone_of);
			if (retval)
				return retval;
		}
		sprintf(buffer, "%s.ini", gamedrv->name);
	}
	else
	{
		sprintf(buffer, "%s", filename);
	}

	if (verbose)
		fprintf(stderr, "trying to parse %s\n", buffer);

	f = fopen (buffer, "r");
	if (f)
	{
		if(rc_read(rc, f, buffer, 1, 1))
		{
			if (verbose)
				fprintf (stderr, "problem parsing %s\n", buffer);
			retval = 1;
		}
	}

	if (f)
		fclose (f);

	return retval;
}

int cli_frontend_init (int argc, char **argv)
{
	struct InternalMachineDriver drv;
	char buffer[128];
	char *cmd_name;
	int game_index;
	int i;

	gamename = NULL;
	game_index = -1;

	/* clear all core options */
	memset(&options,0,sizeof(options));

	/* directly define these */
	options.use_emulated_ym3812 = 1;

	/* create the rc object */
	if (!(rc = rc_create()))
	{
		fprintf (stderr, "error on rc creation\n");
		exit(1);
	}

	if (rc_register(rc, opts))
	{
		fprintf (stderr, "error on registering opts\n");
		exit(1);
	}

#ifdef MESS
	/* mess registers its additional options and callbacks here */
	if (rc_register(rc, mess_opts))
	{
		fprintf (stderr, "error on registering mess options\n");
		exit(1);
	}
#endif

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, config_handle_arg))
	{
		fprintf (stderr, "error while parsing cmdline\n");
		exit(1);
	}

	/* determine global configfile name */
	cmd_name = osd_strip_extension(osd_basename(argv[0]));
	if (!cmd_name)
	{
		fprintf (stderr, "who am I? cannot determine the name I was called with\n");
		exit(1);
	}

	sprintf (buffer, "%s.ini", cmd_name);

	/* parse the global configfile */
	if (parse_config (buffer, NULL))
		exit(1);

#ifdef MAME_DEBUG
	if (parse_config( "debug.ini", NULL))
		exit(1);
#endif

	if (createconfig)
	{
		rc_save(rc, buffer, 0);
		exit(0);
	}

	if (showconfig)
	{
		sprintf (buffer, " %s running parameters", cmd_name);
		rc_write(rc, stdout, buffer);
		exit(0);
	}

	if (showusage)
	{
		fprintf(stdout, "Usage: %s [game] [options]\n" "Options:\n", cmd_name);

		/* actual help message */
		rc_print_help(rc, stdout);
		exit(0);
	}

	/* no longer needed */
	free(cmd_name);

	/* handle playback */
    if (playbackname != NULL)
	{
        options.playback = osd_fopen(playbackname,0,OSD_FILETYPE_INPUTLOG,0);
		if (!options.playback)
		{
			fprintf(stderr, "failed to open %s for playback\n", playbackname);
			exit(1);
		}
	}

    /* check for game name embedded in .inp header */
    if (options.playback)
    {
        INP_HEADER inp_header;

        /* read playback header */
        osd_fread(options.playback, &inp_header, sizeof(INP_HEADER));

        if (!isalnum(inp_header.name[0])) /* If first byte is not alpha-numeric */
            osd_fseek(options.playback, 0, SEEK_SET); /* old .inp file - no header */
        else
        {
            for (i = 0; (drivers[i] != 0); i++) /* find game and play it */
			{
                if (strcmp(drivers[i]->name, inp_header.name) == 0)
                {
                    game_index = i;
					gamename = (char *)drivers[i]->name;
                    printf("Playing back previously recorded game %s (%s) [press return]\n",
                        drivers[game_index]->name,drivers[game_index]->description);
                    getchar();
                    break;
                }
            }
        }
    }

	/* check for frontend options, horrible 1234 hack */
	if (frontend_help(gamename) != 1234)
		exit(0);

	gamename = osd_basename(gamename);
	gamename = osd_strip_extension(gamename);

	/* if not given by .inp file yet */
	if (game_index == -1)
	{
		/* do we have a driver for this? */
		for (i = 0; drivers[i]; i++)
			if (strcasecmp(gamename,drivers[i]->name) == 0)
			{
				game_index = i;
				break;
			}
	}

#ifdef MAME_DEBUG
	if (game_index == -1)
	{
		/* pick a random game */
		if (strcmp(gamename,"random") == 0)
		{
			i = 0;
			while (drivers[i]) i++;	/* count available drivers */

			srand(time(0));
			game_index = rand() % i;

			fprintf(stderr, "running %s (%s) [press return]\n",drivers[game_index]->name,drivers[game_index]->description);
			getchar();
		}
	}
#endif

	/* we give up. print a few approximate matches */
	if (game_index == -1)
	{
		fprintf(stderr, "\n\"%s\" approximately matches the following\n"
				"supported games (best match first):\n\n", gamename);
		show_approx_matches();
		exit(1);
	}

	/* ok, got a gamename */

	/* if this is a vector game, parse vector.ini first */
	expand_machine_driver(drivers[game_index]->drv, &drv);
	if (drv.video_attributes & VIDEO_TYPE_VECTOR)
		if (parse_config ("vector.ini", NULL))
			exit(1);

	/* nice hack: load source_file.ini */
	sprintf(buffer, "%s", drivers[game_index]->source_file+12);
	buffer[strlen(buffer) - 2] = 0;
	strcat(buffer, ".ini");
		if (parse_config (buffer, NULL))
			exit(1);

	/* now load gamename.ini */
	/* this possibly checks for clonename.ini recursively! */
		if (parse_config (NULL, drivers[game_index]))
			exit(1);

	/* handle record option */
	if (recordname)
	{
		options.record = osd_fopen(recordname,0,OSD_FILETYPE_INPUTLOG,1);
		if (!options.record)
		{
			fprintf(stderr, "failed to open %s for recording\n", recordname);
			exit(1);
		}
	}

    if (options.record)
    {
        INP_HEADER inp_header;

        memset(&inp_header, '\0', sizeof(INP_HEADER));
        strcpy(inp_header.name, drivers[game_index]->name);
        /* MAME32 stores the MAME version numbers at bytes 9 - 11
         * MAME DOS keeps this information in a string, the
         * Windows code defines them in the Makefile.
         */
        /*
        inp_header.version[0] = 0;
        inp_header.version[1] = VERSION;
        inp_header.version[2] = BETA_VERSION;
        */
        osd_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
    }

	/* need a decent default for debug width/height */
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;

	/* no sound is indicated by a 0 samplerate */
	if (!enable_sound)
		options.samplerate = 0;

	return game_index;
}

void cli_frontend_exit(void)
{
	/* close open files */
	if (logfile) fclose(logfile);

	if (options.playback) osd_fclose(options.playback);
	if (options.record)   osd_fclose(options.record);
	if (options.language_file) osd_fclose(options.language_file);
}

static int config_handle_arg(char *arg)
{
	static int got_gamename = 0;

	/* notice: for MESS game means system */
	if (got_gamename)
	{
		fprintf(stderr,"error: duplicate gamename: %s\n", arg);
		return -1;
	}

	rompath_extra = osd_dirname(arg);

	if (rompath_extra && !strlen(rompath_extra))
	{
		free (rompath_extra);
		rompath_extra = NULL;
	}

	gamename = arg;

	if (!gamename || !strlen(gamename))
	{
		fprintf(stderr,"error: no gamename given in %s\n", arg);
		return -1;
	}

	got_gamename = 1;
	return 0;
}


/*
 * logerror
 */
#ifdef VPINMAME // VPM defines its own log function
FILE *config_get_logfile(void) { return errorlog ? logfile : NULL; }
#else
void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);
	if (errorlog && logfile)
		vfprintf(logfile, text, arg);
	va_end(arg);
}
#endif /* VPINMAME */

