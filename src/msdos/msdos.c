#include "mamalleg.h"
#include "driver.h"
#include <dos.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include "ticker.h"

#ifdef MESS
#include "msdos.h"
/* from msdos/config.c */
extern char *crcdir;
static char crcfilename[256] = "";
const char *crcfile = crcfilename;
extern char *pcrcdir;
static char pcrcfilename[256] = "";
const char *pcrcfile = pcrcfilename;
#endif


int  msdos_init_seal (void);
int  msdos_init_sound(void);
void msdos_init_input(void);
void msdos_shutdown_sound(void);
void msdos_shutdown_input(void);
int  frontend_help (int argc, char **argv);
void parse_cmdline (int argc, char **argv, int game, char *override_default_rompath);
void init_inpdir(void);


int  ignorecfg;

static FILE *errorlog;


/* avoid wild card expansion on the command line (DJGPP feature) */
char **__crt0_glob_function(void)
{
	return 0;
}

static void signal_handler(int num)
{
	if (errorlog) fflush(errorlog);

	osd_exit();
	allegro_exit();
	ScreenClear();
	ScreenSetCursor( 0, 0 );
	if( num == SIGINT )
		cpu_dump_states();

	signal(num, SIG_DFL);
	raise(num);
}

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	if (msdos_init_sound())
		return 1;
	msdos_init_input();
	return 0;
}


/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	msdos_shutdown_sound();
	msdos_shutdown_input();
}

/* fuzzy string compare, compare short string against long string        */
/* e.g. astdel == "Asteroids Deluxe". The return code is the fuzz index, */
/* we simply count the gaps between maching chars.                       */
int fuzzycmp (const char *s, const char *l)
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

int main (int argc, char **argv)
{
	int res, i, j = 0, game_index;
    char *playbackname = NULL;
	char override_path[256];


	override_path[0] = 0;

	memset(&options,0,sizeof(options));

	/* these two are not available in mame.cfg */
	ignorecfg = 0;
	errorlog = 0;

	game_index = -1;

	for (i = 1;i < argc;i++) /* V.V_121997 */
	{
		if (stricmp(argv[i],"-ignorecfg") == 0) ignorecfg = 1;
		if (stricmp(argv[i],"-log") == 0)
			errorlog = fopen("error.log","wa");
        if (stricmp(argv[i],"-playback") == 0)
		{
			i++;
			if (i < argc)  /* point to inp file name */
				playbackname = argv[i];
        }
	}

    allegro_init();

	/* Allegro changed the signal handlers... change them again to ours, to */
	/* avoid the "Shutting down Allegro" message which confuses users into */
	/* thinking crashes are caused by Allegro. */
	signal(SIGABRT, signal_handler);
	signal(SIGFPE,  signal_handler);
	signal(SIGILL,  signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGKILL, signal_handler);
	signal(SIGQUIT, signal_handler);

	#ifdef MESS
    set_config_file ("mess.cfg");
	#else
    set_config_file ("mame.cfg");
	#endif

	/* check for frontend options */
	res = frontend_help (argc, argv);

	/* if frontend options were used, return to DOS with the error code */
	if (res != 1234)
		exit (res);

	/* Initialize the audio library */
	if (msdos_init_seal())
	{
		printf ("Unable to initialize SEAL\n");
		return (1);
	}

	init_ticker();	/* after Allegro init because we use cpu_cpuid */

    /* handle playback which is not available in mame.cfg */
	init_inpdir(); /* Init input directory for opening .inp for playback */

    if (playbackname != NULL)
        options.playback = osd_fopen(playbackname,0,OSD_FILETYPE_INPUTLOG,0);

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
                    printf("Playing back previously recorded game %s (%s) [press return]\n",
                        drivers[game_index]->name,drivers[game_index]->description);
                    getchar();
                    break;
                }
            }
        }
    }

	/* If not playing back a new .inp file */
    if (game_index == -1)
    {
        /* take the first commandline argument without "-" as the game name */


        for (j = 1; j < argc; j++)
        {
            if (argv[j][0] != '-') break;
        }
		/* do we have a driver for this? */
#ifdef MAME_DEBUG
        /* pick a random game */
        if (stricmp(argv[j],"random") == 0)
        {
            struct timeval t;

            i = 0;
            while (drivers[i]) i++;	/* count available drivers */

            gettimeofday(&t,0);
            srand(t.tv_sec);
            game_index = rand() % i;

            printf("Running %s (%s) [press return]\n",drivers[game_index]->name,drivers[game_index]->description);
            getchar();
        }
        else
#endif
        {
			char gamename[256];
			char *n,*c;


			/* separate leading path */
			strcpy(override_path,argv[j]);
			n = override_path;
			do
			{
				c = strchr(n,'\\');
				if (c) n = c+1;
			} while (c);
			strcpy(gamename,n);
			if (n == override_path)
				*n = 0;
			else
				*(n-1) = 0;

			/* strip out trailing extension */
			c = strchr(gamename,'.');
			if (c) *c = 0;

			for (i = 0; drivers[i] && (game_index == -1); i++)
			{
				if (stricmp(gamename,drivers[i]->name) == 0)
				{
					game_index = i;
					break;
				}
			}

			/* educated guess on what the user wants to play */
			if (game_index == -1)
			{
				int fuzz = 9999; /* best fuzz factor so far */

				for (i = 0; (drivers[i] != 0); i++)
				{
					int tmp;
					tmp = fuzzycmp(gamename, drivers[i]->description);
					/* continue if the fuzz index is worse */
					if (tmp > fuzz)
						continue;

					/* on equal fuzz index, we prefer working, original games */
					if (tmp == fuzz)
					{
						/* game is a clone */
						if (drivers[i]->clone_of != 0
								&& !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
						{
							/* if the game we already found works, why bother. */
							/* and broken clones aren't very helpful either */
							if ((!drivers[game_index]->flags & GAME_NOT_WORKING) ||
								(drivers[i]->flags & GAME_NOT_WORKING))
								continue;
						}
						else continue;
					}

					/* we found a better match */
					game_index = i;
					fuzz = tmp;
				}

				if (game_index != -1)
					printf("fuzzy name compare, running %s\n",drivers[game_index]->name);
			}
		}

		if (game_index == -1)
		{
			printf("Game \"%s\" not supported\n", argv[j]);
			return 1;
		}
	}

	#ifdef MESS
	/* This function has been added to MESS.C as load_image() */
	load_image(argc, argv, j, game_index);
	#endif

	/* parse generic (os-independent) options */
	parse_cmdline (argc, argv, game_index, override_path);

{	/* Mish:  I need sample rate initialised _before_ rom loading for optional rom regions */
	extern int soundcard;

	if (soundcard == 0) {    /* silence, this would be -1 if unknown in which case all roms are loaded */
		Machine->sample_rate = 0; /* update the Machine structure to show that sound is disabled */
		options.samplerate=0;
	}
}

	/* handle record which is not available in mame.cfg */
	for (i = 1; i < argc; i++)
	{
		if (stricmp(argv[i],"-record") == 0)
		{
			i++;
			if (i < argc)
				options.record = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,1);
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

	#ifdef MESS
	/* Build the CRC database filename */
	sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
	if (drivers[game_index]->clone_of->name)
		sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
	else
		pcrcfilename[0] = 0;
    #endif

    /* go for it */
	res = run_game (game_index);

	/* close open files */
	if (errorlog) fclose (errorlog);
	if (options.playback) osd_fclose (options.playback);
	if (options.record)   osd_fclose (options.record);
	if (options.language_file) osd_fclose (options.language_file);

	exit (res);
}



void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	if (errorlog)
		vfprintf(errorlog,text,arg);
	va_end(arg);
}
