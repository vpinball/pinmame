/*
 * X-mame config-file and commandline parsing
 * We don't use stderr_file resp stdout_file in here since we don't know if 
 * it's valid yet.
 */

#define __CONFIG_C_
#include <time.h>
#include "xmame.h"
#include "fileio.h"
#include "driver.h"
#include "audit.h"
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_mixer.h"
#include "sysdep/misc.h"
#include "effect.h"

/* be sure that device names are nullified */
extern void XInput_trackballs_reset();

/* from ... */
extern char *cheatfile;
extern char *db_filename;
extern char *history_filename;
extern char *mameinfo_filename;

extern char *playbackname;
extern char *recordname;

/* some local vars */
static int showconfig = 0;
static int showmanusage = 0;
static int showversion = 0;
static int showusage  = 0;
static int use_fuzzycmp = 1;
static int loadconfig = 1;
static char *language = NULL;
static char *gamename = NULL;
char *rompath_extra = NULL;
#ifndef MESS
static char *defaultgamename;
#else
static const char *mess_opts;
void build_crc_database_filename(int game_index);

static int specify_ram(struct rc_option *option, const char *arg, int priority)
{
	UINT32 specified_ram;

	specified_ram = ram_parse_string(arg);
	if (specified_ram == 0)
	{
		fprintf(stderr, "Cannot recognize the RAM option %s; aborting\n", arg);
		return -1;
	}
	options.ram = specified_ram;
	return 0;
}
#endif

static int config_handle_arg(char *arg);
#ifdef MAME_DEBUG
static int config_handle_debug_size(struct rc_option *option, const char *arg,
		int priority);
#endif
void show_usage(void);

#ifdef MESS
static int add_device(struct rc_option *option, const char *arg, int priority);
#endif

/* struct definitions */
#ifdef PINMAME
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
	{ "dmd_compact",NULL, rc_bool,&pmoptions.dmd_compact, "0",  0, 0,   NULL, "Show compact display" },
	{ "dmd_antialias",NULL, rc_int,&pmoptions.dmd_antialias,  "50", 0, 100, NULL, "DMD antialias intensity [%]" },
#ifdef PROC_SUPPORT
	{ "alpha_on_dmd",NULL, rc_bool,&pmoptions.alpha_on_dmd, "0",  0, 0, NULL, "Emulate alphanumeric display on DMD" },
	{ "p-roc",NULL, rc_string,&pmoptions.p_roc, "None",  0, 0, NULL, "YAML Machine description file" },
	{ "virtual_dmd", NULL, rc_bool,&pmoptions.virtual_dmd,  "1",  0, 0, NULL, "Enable DMD emulation" },
#endif
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};
#endif /* PINMAME */
static struct rc_option opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, video_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, sound_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, input_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, network_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
#ifdef PINMAME
	{ NULL, NULL, rc_link, pinmame_opts, NULL, 0,	0, NULL, NULL },
#endif /* PINMAME */
#ifdef MESS
	/* FIXME - these option->names should NOT be hardcoded! */
	{ "MESS specific options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "cartridge", "cart", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to cartridge device" },
	{ "floppydisk","flop", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to floppy disk device" },
	{ "harddisk",  "hard", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to hard disk device" },
	{ "cylinder",  "cyln", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to cylinder device" },
	{ "cassette",  "cass", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to cassette device" },
	{ "punchcard", "pcrd", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to punch card device" },
	{ "punchtape", "ptap", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to punch tape device" },
	{ "printer",   "prin", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to printer device" },
	{ "serial",    "serl", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to serial device" },
	{ "parallel",  "parl", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to parallel device" },
	{ "snapshot",  "dump", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to snapshot device" },
	{ "quickload", "quik", rc_string, &mess_opts, NULL, 0, 0, add_device, "Attach software to quickload device" },
	{ "ramsize", "ram", rc_string, &mess_opts, NULL, 0, 0, specify_ram, "Specifies size of RAM (if supported by driver)" },
#else
	{ "MAME Related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "defaultgame", "def", rc_string, &defaultgamename, "pacman", 0, 0, NULL, "Set the default game started when no game is given on the commandline, only useful for in the configfiles" },
#endif
	{ "language", "lang", rc_string, &language, "english", 0, 0, NULL, "Select the language for the menus and osd" },
	{ "fuzzycmp", "fc", rc_bool, &use_fuzzycmp, "1", 0, 0, NULL, "Enable/disable use of fuzzy gamename matching when there is no exact match" },
	{ "cheat", "c", rc_bool, &options.cheat, "0", 0, 0, NULL, "Enable/disable cheat subsystem" },
	{ "skip_disclaimer", NULL, rc_bool, &options.skip_disclaimer, "0", 0, 0, NULL, "Skip displaying the disclaimer screen" },
	{ "skip_gameinfo", NULL, rc_bool, &options.skip_gameinfo, "0", 0, 0, NULL, "Skip displaying the game info screen" },
	{ "crconly", NULL, rc_bool, &options.crc_only, "0", 0, 0, NULL, "Use only CRC for all integrity checks" },
	{ "bios", NULL, rc_string, &options.bios, "default", 0, 14, NULL, "change system bios" },
#ifdef MAME_DEBUG
	{ "debug", "d", rc_bool, &options.mame_debug, NULL, 0, 0, NULL, "Enable/disable debugger" },
	{ "debug-size", "ds", rc_use_function, NULL, "640x480", 0, 0, config_handle_debug_size, "Specify the resolution/window size to use for the debugger (window) in the form of XRESxYRES (minimum size = 640x480)" },
#endif
	{ NULL, NULL, rc_link, frontend_list_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, frontend_ident_opts, NULL, 0, 0, NULL, NULL },
	{ "General Options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "loadconfig", "lcf", rc_bool, &loadconfig, "1", 0, 0, NULL, "Load (don't load) configfiles" },
	{ "showconfig", "sc", rc_set_int, &showconfig, NULL, 1, 0, NULL, "Display running parameters in rc style" },
	{ "manhelp", "mh", rc_set_int, &showmanusage, NULL, 1, 0, NULL, "Print commandline help in man format, useful for manpage creation" },
	{ "version", "V", rc_set_int, &showversion, NULL, 1, 0, NULL, "Display version" },
	{ "help", "?", rc_set_int, &showusage, NULL, 1, 0, NULL, "Show this help" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};

/* fuzzy string compare, compare short string against long string        */
/* e.g. astdel == "Asteroids Deluxe". The return code is the fuzz index, */
/* we simply count the gaps between maching chars.                       */
static int fuzzycmp (const char *s, const char *l)
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

#ifndef MESS
/* for verify roms which is used for the random game selection */
static int config_printf(const char *fmt, ...)
{
	return 0;
}
#endif

static int config_handle_arg(char *arg)
{
	static int got_gamename = 0;

	if (!got_gamename) /* notice: for MESS game means system */
	{
		gamename     = arg;
		got_gamename = 1;
	}
	else
#ifdef MESS
	{
		if( options.image_count >= MAX_IMAGES )
		{
			fprintf(stderr, "error: too many image names specified!\n");
			return -1;
		}
		/* options.image_files[options.image_count].type = iodevice_type; */
		/* options.image_files[options.image_count].name = arg; */
		/* options.image_count++; */
	}
#else
	{
		fprintf(stderr,"error: duplicate gamename: %s\n", arg);
		return -1;
	}
#endif

	return 0;
}

#ifdef MAME_DEBUG
static int config_handle_debug_size(struct rc_option *option, const char *arg,
		int priority)
{
	int width, height;

	if (sscanf(arg, "%dx%d", &width, &height) == 2)
	{
		if((width >= 640) && (height >= 480))
		{
			options.debug_width  = width;
			options.debug_height = height;
			return 0;
		}
	}
	fprintf(stderr,
			"error: invalid debugger size or too small (minimum size = 640x480): \"%s\".\n",
			arg);
	return -1;
}
#endif /* MAME_DEBUG */

/*
 * get configuration from configfile and env.
 */
int config_init (int argc, char *argv[])
{
	char buffer[BUF_SIZE];
	unsigned char lsb_test[2]={0,1};
	int i;

	memset(&options,0,sizeof(options));

	/* reset trackball devices */
#ifdef USE_XINPUT_DEVICES
	XInput_trackballs_reset();
#endif

	/* Let's see if the endianess of this arch is correct; otherwise,
	   YELL about it and bail out. */
#ifdef LSB_FIRST
	if(*((unsigned short*)lsb_test) != 0x0100)
#else	
	if(*((unsigned short*)lsb_test) != 0x0001)
#endif
	{
		fprintf(stderr, "error: compiled byte ordering doesn't match machine byte ordering\n"
				"are you sure you choose the right arch?\n"
#ifdef LSB_FIRST
				"compiled for lsb-first, are you sure you choose the right cpu in makefile.unix\n");
#else
				"compiled for msb-first, are you sure you choose the right cpu in makefile.unix\n");
#endif
		return OSD_NOT_OK;
	}

	/* some settings which are static for xmame and thus aren't controled
	   by options */
	options.gui_host = 1;
	cheatfile = NULL;
	db_filename = NULL;
	history_filename = NULL;
	mameinfo_filename = NULL;

	/* create the rc object */
	if (!(rc = rc_create()))
		return OSD_NOT_OK;

	if(sysdep_dsp_init(rc, NULL))
		return OSD_NOT_OK;

	if(sysdep_mixer_init(rc, NULL))
		return OSD_NOT_OK;

	if(rc_register(rc, opts))
		return OSD_NOT_OK;

	/* get the homedir */
	if(!(home_dir = rc_get_home_dir()))
		return OSD_NOT_OK;

	/* check that the required dirs exist, and create them if necessary */
	snprintf(buffer, BUF_SIZE, "%s/.%s", home_dir, NAME);
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "cfg");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "mem");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "sta");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "nvram");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "diff");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "rc");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "hi");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	snprintf(buffer, BUF_SIZE, "%s/.%s/%s", home_dir, NAME, "inp");
	if (rc_check_and_create_dir(buffer))
		return OSD_NOT_OK;

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, config_handle_arg))
		return OSD_NOT_OK;

	/* parse the various configfiles, starting with the one with the
	   lowest priority */
	if(loadconfig)
	{
		snprintf(buffer, BUF_SIZE, "%s/%src", XMAMEROOT, NAME);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
		snprintf(buffer, BUF_SIZE, "%s/.%s/%src", home_dir, NAME, NAME);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
		snprintf(buffer, BUF_SIZE, "%s/%s-%src", XMAMEROOT, NAME, DISPLAY_METHOD);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
		snprintf(buffer, BUF_SIZE, "%s/.%s/%s-%src", home_dir, NAME, NAME,
				DISPLAY_METHOD);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
	}

	/* setup stderr_file and stdout_file */
	if (!stderr_file) stderr_file = stderr;
	if (!stdout_file) stdout_file = stdout;

	effect_init1();

	if (showconfig)
	{
		rc_write(rc, stdout_file, NAME" running parameters");
		return OSD_OK;
	}

	if (showmanusage)
	{
		rc_print_man_options(rc, stdout_file);
		return OSD_OK;
	}

	if (showversion)
	{
		fprintf(stdout_file, "%s\n", title);
		return OSD_OK;
	}

	if (showusage)
	{
		show_usage();
		return OSD_OK;
	}

	/* handle frontend options */
	if ( (i=frontend_list(gamename)) != 1234)
		return i;

	if ( (i=frontend_ident(gamename)) != 1234)
		return i;

	if (playbackname)
	{
		options.playback = mame_fopen(playbackname, 0, FILETYPE_INPUTLOG, 0);
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

	/* handle the game selection */
	game_index = -1;

	if (!gamename)
#ifdef MESS
	{
		show_usage();
		return OSD_NOT_OK;
	}
#else
	gamename = defaultgamename;

	/* random game? */
	if (strcasecmp(gamename, "random") == 0)
	{
		for (i=0; drivers[i]; i++) ; /* count available drivers */

		srand(time(NULL));

		for(;;)
		{
			game_index = (float)rand()*i/RAND_MAX;

			fprintf(stdout_file, "Random game selected: %s (%s)\n  verifying roms... ",drivers[game_index]->name,drivers[game_index]->description);
			if(VerifyRomSet (game_index, (verify_printf_proc)config_printf) == CORRECT)
			{
				fprintf(stdout_file, "OK\n");
				break;
			}
			else
				fprintf(stdout_file, "FAILED\n");
		}
	}
	else
#endif
		/* do we have a driver for this? */
#ifdef MESS
		for (i = 0; drivers[i]; i++)
		{
			if (strcasecmp(gamename,drivers[i]->name) == 0)
			{
				game_index = i;
				break;
			}
		}
#else
	{
		char *begin = strrchr(gamename, '/'), *end;
		int len;

		if (begin == 0)
			begin = gamename;
		else
			begin++;

		end = strchr(begin, '.');
		if (end == 0)
			len = strlen(begin);
		else
			len = end - begin;            

		for (i = 0; drivers[i]; i++)
		{
			if (strncasecmp(begin, drivers[i]->name, len) == 0 
					&& len == strlen(drivers[i]->name))
			{
				begin = strrchr(gamename,'/');
				if (begin)
				{
					*begin='\0'; /* dynamic allocation and copying will be better */
					rompath_extra = malloc(strlen(gamename) + 1);
					strcpy(rompath_extra, gamename);
				}
				game_index = i;
				break;
			}
		}
	}
#endif                                

	/* educated guess on what the user wants to play */
	if ( (game_index == -1) && use_fuzzycmp)
	{
		int fuzz = 9999; /*best fuzz factor so far*/

		for (i = 0; (drivers[i] != 0); i++)
		{
			int tmp;
			tmp = fuzzycmp(gamename, drivers[i]->description);
			/* continue if the fuzz index is worse */
			if (tmp > fuzz)
				continue;
			/* on equal fuzz index, we prefear working, original games */
			if (tmp == fuzz)
			{
				/* game is a clone */
				if (drivers[i]->clone_of != 0 && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
				{
					if ((!drivers[game_index]->flags & GAME_NOT_WORKING) || (drivers[i]->flags & GAME_NOT_WORKING))
						continue;
				}
				else continue;
			}


			/* we found a better match */
			game_index = i;
			fuzz = tmp;
		}

		if (game_index != -1)
			fprintf(stdout_file,
					"fuzzy name compare, running %s\n", drivers[game_index]->name);
	}

	if (game_index == -1)
	{
		fprintf(stderr_file, "\"%s\" not supported\n", gamename);
		return OSD_NOT_OK;
	}

	/* now that we've got the gamename parse the game specific configfile */
	if (loadconfig)
	{
		snprintf(buffer, BUF_SIZE, "%s/rc/%src", XMAMEROOT,
				drivers[game_index]->name);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
		snprintf(buffer, BUF_SIZE, "%s/.%s/rc/%src", home_dir, NAME,
				drivers[game_index]->name);
		if(rc_load(rc, buffer, 1, 1))
			return OSD_NOT_OK;
	}

#ifdef MESS
	build_crc_database_filename(game_index);

	/* set the image type if nescesarry */
	for(i=0; i<options.image_count; i++)
	{
		if(options.image_files[i].type)
		{
			logerror("User specified %s for %s\n",
					device_typename(options.image_files[i].type),
					options.image_files[i].name);
		}
		else
		{
			char *ext;
			char name[BUF_SIZE];
			const struct IODevice *dev;

			/* make a copy of the name */
			strncpy(name, options.image_files[i].name, BUF_SIZE);
			/* strncpy is buggy */
			name[BUF_SIZE-1]=0;

			/* get ext, skip .gz */
			ext = strrchr(name, '.');
			if (ext && !strcmp(ext, ".gz"))
			{
				*ext = 0;
				ext = strrchr(name, '.');
			}

			/* Look up the filename extension in the drivers device list */
			if (ext && (dev = device_first(drivers[game_index])))
			{
				ext++; /* skip the "." */

				while (dev)
				{
					const char *dst = dev->file_extensions;
					/* scan supported extensions for this device */
					while (dst && *dst)
					{
						if (strcasecmp(dst,ext) == 0)
						{
							logerror("Extension match %s [%s] for %s\n",
									device_typename(dev->type), dst,
									options.image_files[i].name);

							options.image_files[i].type = dev->type;
						}
						/* skip '\0' once in the list of extensions */
						dst += strlen(dst) + 1;
					}
					dev = device_next(drivers[game_index], dev); 
				}
			}
			if(!options.image_files[i].type)
				options.image_files[i].type = IO_CARTSLOT;
		}
	}
#endif

	if (recordname)
	{
		options.record = mame_fopen(recordname, 0, FILETYPE_INPUTLOG, 1);
		if (!options.record)
		{
			fprintf(stderr_file, "failed to open %s for recording\n", recordname);
			exit(1);
		}
	}

	if (options.record)
	{
		INP_HEADER inp_header;

		memset(&inp_header, '\0', sizeof(INP_HEADER));
		strcpy(inp_header.name, drivers[game_index]->name);
		mame_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
	}

	if(language)
		options.language_file = mame_fopen(0,language,FILETYPE_LANGUAGE,0);

	return 1234;
}

void config_exit(void)
{
	if(rc)
	{
		sysdep_mixer_exit();
		sysdep_dsp_exit();
		rc_destroy(rc);
	}

	if(home_dir)
		free(home_dir);

	/* close open files */
	if (options.playback)
		mame_fclose(options.playback);
	if (options.record)
		mame_fclose(options.record);
	if (options.language_file)
		mame_fclose(options.language_file);
}

/* 
 * show help and exit
 */
void show_usage(void) 
{
	/* header */
	fprintf(stdout_file, 
#ifdef MESS
			"Usage: xmess <system> [game] [options]\n"
#else
			"Usage: xmame [game] [options]\n"
#endif 
			"Options:\n");

	/* actual help message */
	rc_print_help(rc, stdout_file);

	/* footer */
	fprintf(stdout_file, "\nFiles:\n\n");
	fprintf(stdout_file, "Config Files are parsed in the following order:\n");
	fprint_columns(stdout_file, XMAMEROOT"/"NAME"rc",
			"Global configuration config file");
	fprint_columns(stdout_file, "${HOME}/."NAME"/"NAME"rc",
			"User configuration config file");
	fprint_columns(stdout_file, XMAMEROOT"/"NAME"-"DISPLAY_METHOD"rc",
			"Global per display method config file");
	fprint_columns(stdout_file, "${HOME}/."NAME"/"NAME"-"DISPLAY_METHOD"rc",
			"User per display method config file");
	fprint_columns(stdout_file, XMAMEROOT"/rc/<game>rc",
			"Global per game config file");
	fprint_columns(stdout_file, "${HOME}/."NAME"/rc/<game>rc",
			"User per game config file");
	/*  fprintf(stdout_file, "\nEnvironment variables:\n\n");
	    fprint_columns(stdout_file, "ROMPATH", "Rom search path"); */
	fprintf(stdout_file, "\n"
#ifdef MESS
			"M.E.S.S. - Multi-Emulator Super System\n"
			"Copyright (C) 1998-2003 by the MESS team\n"
#else
			"M.A.M.E. - Multiple Arcade Machine Emulator\n"
			"Copyright (C) 1997-2003 by Nicola Salmoria and the MAME Team\n"
#endif
			"%s port maintained by Lawrence Gold\n", NAME);
}

#ifdef MESS
/*	add_device() is called when the MESS CLI option has been identified
 *	This searches throught the devices{} struct array to grab the ID of the
 *	option, which then registers the device using register_device()
 */
static int add_device(struct rc_option *option, const char *arg, int priority)
{
	int id;
	id = device_typeid(option->name);
	if (id < 0)
	{
		/* If we get to here, log the error - This is mostly due to a mismatch in the array */
		logerror("Command Line Option [-%s] not a valid device - ignoring\n", option->name);
		return -1;
	}

	/* A match!  we now know the ID of the device */
	option->priority = priority;
	return register_device(id, arg);
}
#endif

#ifdef MAME_DEBUG
/*============================================================ */
/*	logerror */
/*============================================================ */

extern FILE *errorlog;

void logerror(const char *text, ...)
{
	va_list arg;

	if (errorlog)
	{
		va_start(arg, text);
		vfprintf(errorlog, text, arg);
		va_end(arg);
		fflush(errorlog);
	}
}
#endif
