
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
  * - get rid of #ifdef MESS's by providing appropriate hooks
 */

#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include "driver.h"
#include "rc.h"
#include "misc.h"
#include "video.h"
#include "fileio.h"

extern struct rc_option frontend_opts[];
extern struct rc_option fileio_opts[];
extern struct rc_option input_opts[];
extern struct rc_option sound_opts[];
extern struct rc_option video_opts[];

#ifdef MESS
#include "configms.h"
#endif
#ifdef PINMAME
struct rc_option core_opts[];
struct rc_option pinmame_opts[] = {
	/* PinMAME options */
	{ "PinMAME options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
#ifdef VPINMAME
	{ "dmd_red",    NULL, rc_int, &pmoptions.dmd_red,   "255", 0, 255, NULL, "DMD color: Red" },
	{ "dmd_green",  NULL, rc_int, &pmoptions.dmd_green, "88", 0, 255, NULL, "DMD color: Green" },
#else
	{ "dmd_red",    NULL, rc_int, &pmoptions.dmd_red,   "225", 0, 255, NULL, "DMD color: Red" },
	{ "dmd_green",  NULL, rc_int, &pmoptions.dmd_green, "224", 0, 255, NULL, "DMD color: Green" },
#endif
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
extern int frontend_help(char *gamename);
static int config_handle_arg(char *arg);

static FILE *logfile;
static int maxlogsize;
static int curlogsize;
static int errorlog;
static int erroroslog;
static int showconfig;
static int showusage;
static int readconfig;
static int createconfig;
extern int verbose;

struct rc_struct *rc;

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
static float f_intensity;

static int enable_sound = 1;

static int use_artwork = 1;
static int use_backdrops = -1;
static int use_overlays = -1;
static int use_bezels = -1;

static int video_norotate = 0;
static int video_flipy = 0;
static int video_flipx = 0;
static int video_ror = 0;
static int video_rol = 0;
static int video_autoror = 0;
static int video_autorol = 0;

static char *win_basename(char *filename);
static char *win_dirname(char *filename);
static char *win_strip_extension(char *filename);


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

static int video_set_intensity(struct rc_option *option, const char *arg, int priority)
{
	options.vector_intensity = f_intensity;
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

static int init_errorlog(struct rc_option *option, const char *arg, int priority)
{
	/* provide errorlog from here on */
	if (errorlog && !logfile)
	{
		logfile = fopen("error.log","wa");
		curlogsize = 0;
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
#ifdef MESS
	{ NULL, NULL, rc_link, mess_opts, NULL, 0,	0, NULL, NULL },
#endif

	/* options supported by the mame core */
	/* video */
	{ "Mame CORE video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "norotate", NULL, rc_bool, &video_norotate, "0", 0, 0, NULL, "do not apply rotation" },
	{ "ror", NULL, rc_bool, &video_ror, "0", 0, 0, NULL, "rotate screen clockwise" },
	{ "rol", NULL, rc_bool, &video_rol, "0", 0, 0, NULL, "rotate screen anti-clockwise" },
	{ "autoror", NULL, rc_bool, &video_autoror, "0", 0, 0, NULL, "automatically rotate screen clockwise for vertical games" },
	{ "autorol", NULL, rc_bool, &video_autorol, "0", 0, 0, NULL, "automatically rotate screen anti-clockwise for vertical games" },
	{ "flipx", NULL, rc_bool, &video_flipx, "0", 0, 0, NULL, "flip screen upside-down" },
	{ "flipy", NULL, rc_bool, &video_flipy, "0", 0, 0, NULL, "flip screen left-right" },
	{ "debug_resolution", "dr", rc_string, &debugres, "auto", 0, 0, video_set_debugres, "set resolution for debugger window" },
	{ "gamma", NULL, rc_float, &options.gamma, "1.0", 0.5, 2.0, NULL, "gamma correction"},
	{ "brightness", "bright", rc_float, &options.brightness, "1.0", 0.5, 2.0, NULL, "brightness correction"},
	{ "pause_brightness", NULL, rc_float, &options.pause_bright, "0.65", 0.5, 2.0, NULL, "additional pause brightness"},

	/* vector */
	{ "Mame CORE vector game options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "antialias", "aa", rc_bool, &options.antialias, "1", 0, 0, NULL, "draw antialiased vectors" },
	{ "translucency", "tl", rc_bool, &options.translucency, "1", 0, 0, NULL, "draw translucent vectors" },
	{ "beam", NULL, rc_float, &f_beam, "1.0", 1.0, 16.0, video_set_beam, "set beam width in vector games" },
	{ "flicker", NULL, rc_float, &f_flicker, "0.0", 0.0, 100.0, video_set_flicker, "set flickering in vector games" },
	{ "intensity", NULL, rc_float, &f_intensity, "1.5", 0.5, 3.0, video_set_intensity, "set intensity in vector games" },

	/* sound */
	{ "Mame CORE sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "samplerate", "sr", rc_int, &options.samplerate, "44100", 5000, 50000, NULL, "set samplerate" },
	{ "samples", NULL, rc_bool, &options.use_samples, "1", 0, 0, NULL, "use samples" },
	{ "resamplefilter", NULL, rc_bool, &options.use_filter, "1", 0, 0, NULL, "resample if samplerate does not match" },
	{ "sound", NULL, rc_bool, &enable_sound, "1", 0, 0, NULL, "enable/disable sound and sound CPUs" },
	{ "volume", "vol", rc_int, &attenuation, "0", -32, 0, NULL, "volume (range [-32,0])" },

	/* misc */
	{ "Mame CORE misc options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "artwork", "art", rc_bool, &use_artwork, "1", 0, 0, NULL, "use additional game artwork (sets default for specific options below)" },
	{ "use_backdrops", "backdrop", rc_bool, &use_backdrops, "1", 0, 0, NULL, "use backdrop artwork" },
	{ "use_overlays", "overlay", rc_bool, &use_overlays, "1", 0, 0, NULL, "use overlay artwork" },
	{ "use_bezels", "bezel", rc_bool, &use_bezels, "1", 0, 0, NULL, "use bezel artwork" },
	{ "artwork_crop", "artcrop", rc_bool, &options.artwork_crop, "0", 0, 0, NULL, "crop artwork to game screen only" },
	{ "artwork_resolution", "artres", rc_int, &options.artwork_res, "0", 0, 0, NULL, "artwork resolution (0 for auto)" },
	{ "cheat", "c", rc_bool, &options.cheat, "0", 0, 0, NULL, "enable/disable cheat subsystem" },
	{ "debug", "d", rc_bool, &options.mame_debug, "0", 0, 0, NULL, "enable/disable debugger (only if available)" },
	{ "playback", "pb", rc_string, &playbackname, NULL, 0, 0, NULL, "playback an input file" },
	{ "record", "rec", rc_string, &recordname, NULL, 0, 0, NULL, "record an input file" },
	{ "log", NULL, rc_bool, &errorlog, "0", 0, 0, init_errorlog, "generate error.log" },
	{ "maxlogsize", NULL, rc_int, &maxlogsize, "10000", 1, 2000000, NULL, "maximum error.log size (in KB)" },
	{ "oslog", NULL, rc_bool, &erroroslog, "0", 0, 0, NULL, "output error log to debugger" },
	{ "skip_disclaimer", NULL, rc_bool, &options.skip_disclaimer, "0", 0, 0, NULL, "skip displaying the disclaimer screen" },
	{ "skip_gameinfo", NULL, rc_bool, &options.skip_gameinfo, "0", 0, 0, NULL, "skip displaying the game info screen" },
	{ "crconly", NULL, rc_bool, &options.crc_only, "0", 0, 0, NULL, "use only CRC for all integrity checks" },
	{ "bios", NULL, rc_string, &options.bios, "default", 0, 14, NULL, "change system bios" },

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
	mame_file *f;
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
		fprintf(stderr, "parsing %s...", buffer);

	f = mame_fopen (buffer, NULL, FILETYPE_INI, 0);
	if (f)
	{
		if(osd_rc_read(rc, f, buffer, 1, 1))
		{
			if (verbose)
				fprintf (stderr, "problem parsing %s\n", buffer);
			retval = 1;
		}
		else
		{
			if (verbose)
				fprintf (stderr, "OK.\n");
		}
	}
	else
	{
		if (verbose)
			fprintf (stderr, "N/A\n");
	}

	if (f)
		mame_fclose (f);

	return retval;
}

struct rc_struct *cli_rc_create(void)
{
	struct rc_struct *result;

	result = rc_create();
	if (!result)
		return NULL;

	if (rc_register(result, opts))
	{
		rc_destroy(result);
		return NULL;
	}

	return result;
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

	/* create the rc object */
	rc = cli_rc_create();
	if (!rc)
	{
		fprintf (stderr, "error on rc creation\n");
		exit(1);
	}

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, config_handle_arg))
	{
		fprintf (stderr, "error while parsing cmdline\n");
		exit(1);
	}

	/* determine global configfile name */
	cmd_name = win_strip_extension(win_basename(argv[0]));
	if (!cmd_name)
	{
		fprintf (stderr, "who am I? cannot determine the name I was called with\n");
		exit(1);
	}

	sprintf (buffer, "%s.ini", cmd_name);

	/* parse mame.ini/mess.ini even if called with another name */
#ifdef MESS
	if (strcmp(cmd_name, "mess") != 0)
	{
		if (parse_config ("mess.ini", NULL))
			exit(1);
	}
#else
	if (strcmp(cmd_name, "mame") != 0)
	{
		if (parse_config ("mame.ini", NULL))
			exit(1);
	}
#endif

	/* parse cmd_name.ini */
	if (parse_config (buffer, NULL))
		exit(1);

#ifdef MAME_DEBUG
	if (parse_config( "debug.ini", NULL))
		exit(1);
#endif

	/* if requested, write out cmd_name.ini (normally "mame.ini") */
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
        options.playback = mame_fopen(playbackname,0,FILETYPE_INPUTLOG,0);
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
		mame_fread(options.playback, &inp_header, sizeof(INP_HEADER));

		if (!isalnum(inp_header.name[0])) /* If first byte is not alpha-numeric */
			mame_fseek(options.playback, 0, SEEK_SET); /* old .inp file - no header */
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

	gamename = win_basename(gamename);
	gamename = win_strip_extension(gamename);

	/* if not given by .inp file yet */
	if (game_index == -1)
	{
		/* do we have a driver for this? */
		for (i = 0; drivers[i]; i++)
			if (_stricmp(gamename,drivers[i]->name) == 0)
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
			/* call rand() once to get away from the seed */
			rand();
			game_index = rand() % i;

			fprintf(stderr, "running %s (%s) [press return]",drivers[game_index]->name,drivers[game_index]->description);
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

	/* nice hack: load source_file.ini (omit if referenced later any) */
	{
		const struct GameDriver *tmp_gd;

		sprintf(buffer, "%s", drivers[game_index]->source_file+12);
		buffer[strlen(buffer) - 2] = 0;

		tmp_gd = drivers[game_index];
		while (tmp_gd != NULL)
		{
			if (strcmp(tmp_gd->name, buffer) == 0) break;
			tmp_gd = tmp_gd->clone_of;
		}

		if (tmp_gd == NULL)
		/* not referenced later, so load it here */
		{
			strcat(buffer, ".ini");
			if (parse_config (buffer, NULL))
				exit(1);
		}
	}

	/* now load gamename.ini */
	/* this possibly checks for clonename.ini recursively! */
	if (parse_config (NULL, drivers[game_index]))
		exit(1);

	/* handle record option */
	if (recordname)
	{
		options.record = mame_fopen(recordname,0,FILETYPE_INPUTLOG,1);
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
		mame_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
	}

	/* need a decent default for debug width/height */
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;
	options.debug_depth = 8;

	/* no sound is indicated by a 0 samplerate */
	if (!enable_sound)
		options.samplerate = 0;

	/* set the artwork options */
	options.use_artwork = ARTWORK_USE_ALL;
	if (use_backdrops == 0)
		options.use_artwork &= ~ARTWORK_USE_BACKDROPS;
	if (use_overlays == 0)
		options.use_artwork &= ~ARTWORK_USE_OVERLAYS;
	if (use_bezels == 0)
		options.use_artwork &= ~ARTWORK_USE_BEZELS;
	if (!use_artwork)
		options.use_artwork = ARTWORK_USE_NONE;

{
	/* first start with the game's built in orientation */
	int orientation = drivers[game_index]->flags & ORIENTATION_MASK;
	options.ui_orientation = orientation;

	if (options.ui_orientation & ORIENTATION_SWAP_XY)
	{
		/* if only one of the components is inverted, switch them */
		if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
				(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
			options.ui_orientation ^= ROT180;
	}

	/* override if no rotation requested */
	if (video_norotate)
		orientation = options.ui_orientation = ROT0;

	/* rotate right */
	if (video_ror)
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* rotate left */
	if (video_rol)
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* auto-rotate right (e.g. for rotating lcds), based on original orientation */
	if (video_autoror && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* auto-rotate left (e.g. for rotating lcds), based on original orientation */
	if (video_autorol && (drivers[game_index]->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* flip X/Y */
	if (video_flipx)
		orientation ^= ORIENTATION_FLIP_X;
	if (video_flipy)
		orientation ^= ORIENTATION_FLIP_Y;

	blit_flipx = ((orientation & ORIENTATION_FLIP_X) != 0);
	blit_flipy = ((orientation & ORIENTATION_FLIP_Y) != 0);
	blit_swapxy = ((orientation & ORIENTATION_SWAP_XY) != 0);

	if( options.vector_width == 0 && options.vector_height == 0 )
	{
		options.vector_width = 640;
		options.vector_height = 480;
	}
	if( blit_swapxy )
	{
		int temp;
		temp = options.vector_width;
		options.vector_width = options.vector_height;
		options.vector_height = temp;
	}
}

	return game_index;
}

void cli_frontend_exit(void)
{
	/* close open files */
	if (logfile) fclose(logfile);

	if (options.playback) mame_fclose(options.playback);
	if (options.record)   mame_fclose(options.record);
	if (options.language_file) mame_fclose(options.language_file);
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

	rompath_extra = win_dirname(arg);

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
#else /* VPINMAME */
#if (!defined(PINMAME) || defined(MAME_DEBUG) || defined(_DEBUG)) // In PinMAME, log only in debug mode.
void CLIB_DECL logerror(const char *text,...) {
	va_list arg;

	/* standard vfprintf stuff here */
	va_start(arg, text);

	if (errorlog && logfile)
	{
		curlogsize += vfprintf(logfile, text, arg);
		if (curlogsize > maxlogsize * 1024)
		{
			fclose(logfile);
			logfile = NULL;
			exit(1);
		}
	}

	if (erroroslog)
	{
		//extern int vsnprintf(char *s, size_t maxlen, const char *fmt, va_list _arg);
		char buffer[2048];
		_vsnprintf(buffer, sizeof(buffer) / sizeof(buffer[0]), text, arg);
		OutputDebugString(buffer);
	}
	va_end(arg);
}
#endif /* PINMAME DEBUG */
#endif /* VPINMAME */

//============================================================
//	win_basename
//============================================================

static char *win_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}



//============================================================
//	win_dirname
//============================================================

static char *win_dirname(char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
	{
		fprintf(stderr, "error: malloc failed in win_dirname\n");
		return NULL;
	}

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}



//============================================================
//	win_strip_extension
//============================================================

static char *win_strip_extension(char *filename)
{
	char *newname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	newname = malloc(strlen(filename) + 1);
	if (!newname)
	{
		fprintf(stderr, "error: malloc failed in win_strip_extension\n");
		return NULL;
	}

	// copy in the name
	strcpy(newname, filename);

	// search backward for a period, failing if we hit a slash or a colon
	for (c = newname + strlen(newname) - 1; c >= newname; c--)
	{
		// if we hit a period, NULL terminate and break
		if (*c == '.')
		{
			*c = 0;
			break;
		}

		// if we hit a slash or colon just stop
		if (*c == '\\' || *c == '/' || *c == ':')
			break;
	}

	return newname;
}
