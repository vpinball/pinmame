
/*
 * Configuration routines.
 *
 * 19971219 support for mame.cfg by Valerio Verrando
 * 19980402 moved out of msdos.c (N.S.), generalized routines (BW)
 * 19980917 added a "-cheatfile" option (misc) in MAME.CFG      JCK
 */

#include "mamalleg.h"
#include "driver.h"
#include <ctype.h>
/* types of monitors supported */
#include "monitors.h"
/* from main() */
extern int ignorecfg;

/* from video.c */
extern int frameskip,autoframeskip;
extern int scanlines, use_tweaked, video_sync, wait_vsync, use_triplebuf;
extern int stretch, use_mmx, use_dirty;
extern int vgafreq, always_synced, skiplines, skipcolumns;
extern float osd_gamma_correction;
extern int gfx_mode, gfx_width, gfx_height;

extern int monitor_type;

extern unsigned char tw224x288_h, tw224x288_v;
extern unsigned char tw240x256_h, tw240x256_v;
extern unsigned char tw256x240_h, tw256x240_v;
extern unsigned char tw256x256_h, tw256x256_v;
extern unsigned char tw256x256_hor_h, tw256x256_hor_v;
extern unsigned char tw288x224_h, tw288x224_v;
extern unsigned char tw240x320_h, tw240x320_v;
extern unsigned char tw320x240_h, tw320x240_v;
extern unsigned char tw336x240_h, tw336x240_v;
extern unsigned char tw384x224_h, tw384x224_v;
extern unsigned char tw384x240_h, tw384x240_v;
extern unsigned char tw384x256_h, tw384x256_v;
extern unsigned char tw400x256_h, tw400x256_v;


/* Tweak values for 15.75KHz arcade/ntsc/pal modes */
/* from video.c */
extern unsigned char tw224x288arc_h, tw224x288arc_v, tw288x224arc_h, tw288x224arc_v;
extern unsigned char tw256x256arc_h, tw256x256arc_v, tw256x240arc_h, tw256x240arc_v;
extern unsigned char tw320x240arc_h, tw320x240arc_v, tw320x256arc_h, tw320x256arc_v;
extern unsigned char tw352x240arc_h, tw352x240arc_v, tw352x256arc_h, tw352x256arc_v;
extern unsigned char tw368x224arc_h, tw368x224arc_v;
extern unsigned char tw368x240arc_h, tw368x240arc_v, tw368x256arc_h, tw368x256arc_v;
extern unsigned char tw512x224arc_h, tw512x224arc_v, tw512x256arc_h, tw512x256arc_v;
extern unsigned char tw512x448arc_h, tw512x448arc_v, tw512x512arc_h, tw512x512arc_v;
extern unsigned char tw640x480arc_h, tw640x480arc_v;


/* from sound.c */
extern int soundcard, usestereo, attenuation, sampleratedetect;

/* from input.c */
extern int use_mouse, joystick, use_hotrod, steadykey;

/* from cheat.c */
extern char *cheatfile;

/* from datafile.c */
extern const char *history_filename,*mameinfo_filename;

/* from fileio.c */
void decompose_rom_sample_path (const char *rompath, const char *samplepath);

#ifdef MESS
void decompose_software_path (const char *softwarepath);
#endif

extern const char *nvdir, *hidir, *cfgdir, *inpdir, *stadir, *memcarddir;
extern const char *artworkdir, *screenshotdir;
extern char *alternate_name;

extern const char *cheatdir;

#ifdef MESS
/* path to the CRC database files */
const char *crcdir;
const char *softwarepath;  /* for use in fileio.c */
#define CHEAT_NAME	"CHEAT.CDB"
#define HISTRY_NAME "SYSINFO.DAT"
#else
#define CHEAT_NAME	"CHEAT.DAT"
#define HISTRY_NAME "HISTORY.DAT"
#endif

/* from video.c, for centering tweaked modes */
extern int center_x;
extern int center_y;

/*from video.c flag for 15.75KHz modes (req. for 15.75KHz Arcade Monitor Modes)*/
extern int arcade_mode;

/*from svga15kh.c flag to allow delay for odd/even fields for interlaced displays (req. for 15.75KHz Arcade Monitor Modes)*/
extern int wait_interlace;

static int mame_argc;
static char **mame_argv;
static int game;
static const char *rompath, *samplepath;

struct { char *name; int id; } joy_table[] =
{
	{ "none",           JOY_TYPE_NONE },
	{ "auto",           JOY_TYPE_AUTODETECT },
	{ "standard",       JOY_TYPE_STANDARD },
	{ "dual",           JOY_TYPE_2PADS },
	{ "4button",        JOY_TYPE_4BUTTON },
	{ "6button",        JOY_TYPE_6BUTTON },
	{ "8button",        JOY_TYPE_8BUTTON },
	{ "fspro",          JOY_TYPE_FSPRO },
	{ "wingex",         JOY_TYPE_WINGEX },
	{ "sidewinder",     JOY_TYPE_SIDEWINDER_AG },
	{ "gamepadpro",     JOY_TYPE_GAMEPAD_PRO },
	{ "grip",           JOY_TYPE_GRIP },
	{ "grip4",          JOY_TYPE_GRIP4 },
	{ "sneslpt1",       JOY_TYPE_SNESPAD_LPT1 },
	{ "sneslpt2",       JOY_TYPE_SNESPAD_LPT2 },
	{ "sneslpt3",       JOY_TYPE_SNESPAD_LPT3 },
	{ "psxlpt1",        JOY_TYPE_PSXPAD_LPT1 },
	{ "psxlpt2",        JOY_TYPE_PSXPAD_LPT2 },
	{ "psxlpt3",        JOY_TYPE_PSXPAD_LPT3 },
	{ "n64lpt1",        JOY_TYPE_N64PAD_LPT1 },
	{ "n64lpt2",        JOY_TYPE_N64PAD_LPT2 },
	{ "n64lpt3",        JOY_TYPE_N64PAD_LPT3 },
	{ "wingwarrior",    JOY_TYPE_WINGWARRIOR },
	{ "segaisa",        JOY_TYPE_IFSEGA_ISA },
	{ "segapci",        JOY_TYPE_IFSEGA_PCI },
	{ 0, 0 }
} ;



/* monitor type */
struct { char *name; int id; } monitor_table[] =
{
	{ "standard",       MONITOR_TYPE_STANDARD},
	{ "ntsc",           MONITOR_TYPE_NTSC},
	{ "pal",            MONITOR_TYPE_PAL},
	{ "arcade",         MONITOR_TYPE_ARCADE},
	{ NULL, NULL }
} ;


/*
 * gets some boolean config value.
 * 0 = false, >0 = true, <0 = auto
 * the shortcut can only be used on the commandline
 */
static int get_bool (char *section, char *option, char *shortcut, int def)
{
	const char *yesnoauto;
	int res, i;

	res = def;

	if (ignorecfg) goto cmdline;

	/* look into mame.cfg, [section] */
	if (def == 0)
		yesnoauto = get_config_string(section, option, "no");
	else if (def > 0)
		yesnoauto = get_config_string(section, option, "yes");
	else /* def < 0 */
		yesnoauto = get_config_string(section, option, "auto");

	/* if the option doesn't exist in the cfgfile, create it */
	if (get_config_string(section, option, "#") == "#")
		set_config_string(section, option, yesnoauto);

	/* look into mame.cfg, [gamename] */
	yesnoauto = get_config_string((char *)drivers[game]->name, option, yesnoauto);

	/* also take numerical values instead of "yes", "no" and "auto" */
	if		(stricmp(yesnoauto, "no"  ) == 0) res = 0;
	else if (stricmp(yesnoauto, "yes" ) == 0) res = 1;
	else if (stricmp(yesnoauto, "auto") == 0) res = -1;
	else	res = atoi (yesnoauto);

cmdline:
	/* check the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-') continue;
		/* look for "-option" */
		if (stricmp(&mame_argv[i][1], option) == 0)
			res = 1;
		/* look for "-shortcut" */
		if (shortcut && (stricmp(&mame_argv[i][1], shortcut) == 0))
			res = 1;
		/* look for "-nooption" */
		if (strnicmp(&mame_argv[i][1], "no", 2) == 0)
		{
			if (stricmp(&mame_argv[i][3], option) == 0)
				res = 0;
			if (shortcut && (stricmp(&mame_argv[i][3], shortcut) == 0))
				res = 0;
		}
		/* look for "-autooption" */
		if (strnicmp(&mame_argv[i][1], "auto", 4) == 0)
		{
			if (stricmp(&mame_argv[i][5], option) == 0)
				res = -1;
			if (shortcut && (stricmp(&mame_argv[i][5], shortcut) == 0))
				res = -1;
		}
	}
	return res;
}

static int get_int (char *section, char *option, char *shortcut, int def)
{
	int res,i;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_int (section, option, -777) == -777)
			set_config_int (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_int (section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_int ((char *)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;
		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut ) == 0)))
		{
			i++;
			if (i < mame_argc) res = atoi (mame_argv[i]);
		}
	}
	return res;
}

static float get_float (char *section, char *option, char *shortcut, float def)
{
	int i;
	float res;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_float (section, option, 9999.0) == 9999.0)
			set_config_float (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_float (section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_float ((char *)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;
		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut ) == 0)))
		{
			i++;
			if (i < mame_argc) res = atof (mame_argv[i]);
		}
	}
	return res;
}

static const char *get_string (char *section, char *option, char *shortcut, char *def)
{
	const char *res;
	int i;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_string (section, option, "#") == "#" )
			set_config_string (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_string(section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_string((char*)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;

		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut)  == 0)))
		{
			i++;
			if (i < mame_argc) res = mame_argv[i];
		}
	}
	return res;
}

void get_rom_sample_path (int argc, char **argv, int game_index, char *override_default_rompath)
{
	int i;

	alternate_name = 0;
	mame_argc = argc;
	mame_argv = argv;
	game = game_index;

	rompath = override_default_rompath;
	if (rompath == NULL || rompath[0] == 0)
	{
#ifndef MESS
		rompath    = get_string ("directory", "rompath",    NULL, ".;ROMS");
#else
		rompath    = get_string ("directory", "biospath",   NULL, ".;BIOS");
#endif
	}

#ifndef MESS
	samplepath = get_string ("directory", "samplepath", NULL, ".;SAMPLES");
#else
	softwarepath= get_string ("directory", "softwarepath", NULL, ".;SOFTWARE");
#endif

	/* handle '-romdir' hack. We should get rid of this BW */
	alternate_name = 0;
	for (i = 1; i < argc; i++)
	{
		if (stricmp (argv[i], "-romdir") == 0)
		{
			i++;
			if (i < argc) alternate_name = argv[i];
		}
	}

	/* decompose paths into components (handled by fileio.c) */
	decompose_rom_sample_path (rompath, samplepath);
}


/* for playback of .inp files */
void init_inpdir(void)
{
	inpdir = get_string ("directory", "inp",     NULL, "INP");
}

void parse_cmdline (int argc, char **argv, int game_index, char *override_default_rompath)
{
	static float f_beam;
	const char *resolution, *vectorres;
	const char *vesamode;
	const char *joyname;
	char tmpres[10];
	int i;
	const char *tmpstr;
	const char *monitorname;

	mame_argc = argc;
	mame_argv = argv;
	game = game_index;


	/* force third mouse button emulation to "no" otherwise Allegro will default to "yes" */
	set_config_string(0,"emulate_three","no");

	/* read graphic configuration */
	scanlines	= get_bool	 ("config", "scanlines",    NULL,  1);
	stretch 	= get_bool	 ("config", "stretch",      NULL,  1);
	options.use_artwork = get_bool	 ("config", "artwork",  NULL,  1);
	options.use_samples = get_bool	 ("config", "samples",  NULL,  1);
	video_sync	= get_bool	 ("config", "vsync",        NULL,  0);
	wait_vsync	= get_bool	 ("config", "waitvsync",    NULL,  0);
	use_triplebuf = get_bool ("config", "triplebuffer", NULL,  0);
	use_tweaked = get_bool	 ("config", "tweak",        NULL,  0);
	vesamode	= get_string ("config", "vesamode", NULL,   "vesa3");
	use_mmx 	= get_bool	 ("config", "mmx",      NULL,   -1);
	use_dirty	= get_bool	 ("config", "dirty",    NULL,   -1);
	options.antialias	= get_bool	 ("config", "antialias",    NULL,  1);
	options.translucency = get_bool    ("config", "translucency", NULL, 1);

	vgafreq 	= get_int	 ("config", "vgafreq",      NULL,  -1);
	always_synced = get_bool ("config", "alwayssynced", NULL, 0);

	tmpstr			   = get_string ("config", "depth", NULL, "auto");
	options.color_depth = atoi(tmpstr);
	if (options.color_depth != 8 && options.color_depth != 15 &&
			options.color_depth != 16 && options.color_depth != 32)
		options.color_depth = 0; /* auto */

	skiplines	= get_int	 ("config", "skiplines",    NULL, 0);
	skipcolumns = get_int	 ("config", "skipcolumns",  NULL, 0);
	f_beam		= get_float  ("config", "beam",         NULL, 1.0);
	if (f_beam < 1.0) f_beam = 1.0;
	if (f_beam > 16.0) f_beam = 16.0;
	options.vector_flicker	= get_float  ("config", "flicker",      NULL, 0.0);
	if (options.vector_flicker < 0.0) options.vector_flicker = 0.0;
	if (options.vector_flicker > 100.0) options.vector_flicker = 100.0;
	osd_gamma_correction = get_float ("config", "gamma",   NULL, 1.0);
	if (osd_gamma_correction < 0.5) osd_gamma_correction = 0.5;
	if (osd_gamma_correction > 2.0) osd_gamma_correction = 2.0;

	tmpstr			   = get_string ("config", "frameskip", "fs", "auto");
	if (!stricmp(tmpstr,"auto"))
	{
		frameskip = 0;
		autoframeskip = 1;
	}
	else
	{
		frameskip = atoi(tmpstr);
		autoframeskip = 0;
	}
	options.norotate  = get_bool ("config", "norotate",  NULL, 0);
	options.ror 	  = get_bool ("config", "ror",       NULL, 0);
	options.rol 	  = get_bool ("config", "rol",       NULL, 0);
	options.flipx	  = get_bool ("config", "flipx",     NULL, 0);
	options.flipy	  = get_bool ("config", "flipy",     NULL, 0);

	/* read sound configuration */

	soundcard			= get_int  ("config", "soundcard",  NULL, -1);
	options.use_emulated_ym3812 = !get_bool ("config", "ym3812opl",  NULL,  0);
	options.samplerate = get_int  ("config", "samplerate", "sr", 44100);
	if (options.samplerate < 5000) options.samplerate = 5000;
	if (options.samplerate > 50000) options.samplerate = 50000;
	usestereo			= get_bool ("config", "stereo",  NULL,  1);
	attenuation 		= get_int  ("config", "volume",  NULL,  0);
	if (attenuation < -32) attenuation = -32;
	if (attenuation > 0) attenuation = 0;
	sampleratedetect    = get_bool ("config", "sampleratedetect", NULL, 1);
	options.use_filter = get_bool ("config", "resamplefilter", NULL, 1);

	/* read input configuration */
	use_mouse = get_bool   ("config", "mouse",   NULL,  1);
	joyname   = get_string ("config", "joystick", "joy", "none");
    steadykey = get_bool   ("config", "steadykey", NULL, 0);

	use_hotrod = 0;
	if (get_bool  ("config", "hotrod",   NULL,  0)) use_hotrod = 1;
	if (get_bool  ("config", "hotrodse",   NULL,  0)) use_hotrod = 2;

	/* misc configuration */
	options.cheat	   = get_bool ("config", "cheat", NULL, 0);
#ifdef MAME_DEBUG
	options.mame_debug = get_bool ("config", "debug", NULL, 0);
#endif

	tmpstr = get_string ("config", "cheatfile", "cf", CHEAT_NAME );
	cheatfile = malloc(strlen(tmpstr) + 1);
	strcpy(cheatfile,tmpstr);

	history_filename  = get_string ("config", "historyfile", NULL, HISTRY_NAME);
	mameinfo_filename  = get_string ("config", "mameinfofile", NULL, "MAMEINFO.DAT");    /* JCK 980917 */

	/* get resolution */
	resolution	= get_string ("config", "resolution", NULL, "auto");
	vectorres	= get_string ("config", "vectorres", NULL, "auto");

	/* set default subdirectories */
#ifndef MESS
	hidir		= get_string ("directory", "hi",      NULL, "HI");
	stadir		= get_string ("directory", "sta",     NULL, "STA");
	cheatdir	= get_string ("directory",  "cheat",   NULL, ".");
#else
	crcdir		= get_string ("directory",  "crc",     NULL, "CRC");
	cheatdir	= get_string ("directory",  "cheat",   NULL, "CHEAT");
#endif
	memcarddir	= get_string ("directory", "memcard", NULL, "MEMCARD");
	nvdir		= get_string ("directory", "nvram",   NULL, "NVRAM");
	artworkdir	= get_string ("directory", "artwork", NULL, "ARTWORK");
	cfgdir		= get_string ("directory", "cfg",     NULL, "CFG");
	screenshotdir = get_string ("directory", "snap", NULL, "SNAP");
#ifdef PINMAME_EXT
        { extern char *wavedir;
	wavedir 	= get_string ("directory", "wavefile", NULL, "WAVEFILE");

	/* read palette info */
        pmoptions.dmd_red   = get_int("config", "dmd_red",   NULL, 225);
        pmoptions.dmd_green = get_int("config", "dmd_green", NULL, 224);
        pmoptions.dmd_blue  = get_int("config", "dmd_blue",  NULL, 32);
        pmoptions.dmd_perc0 = get_int("config", "dmd_perc0", NULL, 20);
        pmoptions.dmd_perc33 = get_int("config", "dmd_perc33", NULL, 34);
        pmoptions.dmd_perc66 = get_int("config", "dmd_perc66", NULL, 67);
        if(pmoptions.dmd_red   < 0)   pmoptions.dmd_red   = 0;
        if(pmoptions.dmd_red   > 255) pmoptions.dmd_red   = 255;
        if(pmoptions.dmd_green < 0)   pmoptions.dmd_green = 0;
        if(pmoptions.dmd_green > 255) pmoptions.dmd_green = 255;
        if(pmoptions.dmd_blue  < 0)   pmoptions.dmd_blue  = 0;
        if(pmoptions.dmd_blue  > 255) pmoptions.dmd_blue  = 255;
        if(pmoptions.dmd_perc0 < 0)   pmoptions.dmd_perc0 = 0;
        if(pmoptions.dmd_perc0 > 100) pmoptions.dmd_perc0 = 100;
        if(pmoptions.dmd_perc33 < 0)   pmoptions.dmd_perc33 = 0;
        if(pmoptions.dmd_perc33 > 100) pmoptions.dmd_perc33 = 100;
        if(pmoptions.dmd_perc66 < 0)   pmoptions.dmd_perc66 = 0;
        if(pmoptions.dmd_perc66 > 100) pmoptions.dmd_perc66 = 100;
#endif /* PINMAME_EXT *

	logerror("cheatfile = %s - cheatdir = %s\n",cheatfile,cheatdir);

	tmpstr = get_string ("config", "language", NULL, "english");
	options.language_file = osd_fopen(0,tmpstr,OSD_FILETYPE_LANGUAGE,0);

	/* get tweaked modes info */
	tw224x288_h 	= get_int ("tweaked", "224x288_h",      NULL, 0x5f);
	tw224x288_v 	= get_int ("tweaked", "224x288_v",      NULL, 0x54);
	tw240x256_h 	= get_int ("tweaked", "240x256_h",      NULL, 0x67);
	tw240x256_v 	= get_int ("tweaked", "240x256_v",      NULL, 0x23);
	tw256x240_h 	= get_int ("tweaked", "256x240_h",      NULL, 0x55);
	tw256x240_v 	= get_int ("tweaked", "256x240_v",      NULL, 0x43);
	tw256x256_h 	= get_int ("tweaked", "256x256_h",      NULL, 0x6c);
	tw256x256_v 	= get_int ("tweaked", "256x256_v",      NULL, 0x23);
	tw256x256_hor_h = get_int ("tweaked", "256x256_hor_h",  NULL, 0x55);
	tw256x256_hor_v = get_int ("tweaked", "256x256_hor_v",  NULL, 0x60);
	tw288x224_h 	= get_int ("tweaked", "288x224_h",      NULL, 0x5f);
	tw288x224_v 	= get_int ("tweaked", "288x224_v",      NULL, 0x0c);
	tw240x320_h 	= get_int ("tweaked", "240x320_h",      NULL, 0x5a);
	tw240x320_v 	= get_int ("tweaked", "240x320_v",      NULL, 0x8c);
	tw320x240_h 	= get_int ("tweaked", "320x240_h",      NULL, 0x5f);
	tw320x240_v 	= get_int ("tweaked", "320x240_v",      NULL, 0x0c);
	tw336x240_h 	= get_int ("tweaked", "336x240_h",      NULL, 0x5f);
	tw336x240_v 	= get_int ("tweaked", "336x240_v",      NULL, 0x0c);
	tw384x224_h 	= get_int ("tweaked", "384x224_h",      NULL, 0x6c);
	tw384x224_v 	= get_int ("tweaked", "384x224_v",      NULL, 0x0c);
	tw384x240_h 	= get_int ("tweaked", "384x240_h",      NULL, 0x6c);
	tw384x240_v 	= get_int ("tweaked", "384x240_v",      NULL, 0x0c);
	tw384x256_h 	= get_int ("tweaked", "384x256_h",      NULL, 0x6c);
	tw384x256_v 	= get_int ("tweaked", "384x256_v",      NULL, 0x23);

	/* Get 15.75KHz tweak values */
	tw224x288arc_h	= get_int ("tweaked", "224x288arc_h",   NULL, 0x5d);
	tw224x288arc_v	= get_int ("tweaked", "224x288arc_v",   NULL, 0x38);
	tw288x224arc_h	= get_int ("tweaked", "288x224arc_h",   NULL, 0x5d);
	tw288x224arc_v	= get_int ("tweaked", "288x224arc_v",   NULL, 0x09);
	tw256x240arc_h	= get_int ("tweaked", "256x240arc_h",   NULL, 0x5d);
	tw256x240arc_v	= get_int ("tweaked", "256x240arc_v",   NULL, 0x09);
	tw256x256arc_h	= get_int ("tweaked", "256x256arc_h",   NULL, 0x5d);
	tw256x256arc_v	= get_int ("tweaked", "256x256arc_v",   NULL, 0x17);
	tw320x240arc_h	= get_int ("tweaked", "320x240arc_h",   NULL, 0x69);
	tw320x240arc_v	= get_int ("tweaked", "320x240arc_v",   NULL, 0x09);
	tw320x256arc_h	= get_int ("tweaked", "320x256arc_h",   NULL, 0x69);
	tw320x256arc_v	= get_int ("tweaked", "320x256arc_v",   NULL, 0x17);
	tw352x240arc_h	= get_int ("tweaked", "352x240arc_h",   NULL, 0x6a);
	tw352x240arc_v	= get_int ("tweaked", "352x240arc_v",   NULL, 0x09);
	tw352x256arc_h	= get_int ("tweaked", "352x256arc_h",   NULL, 0x6a);
	tw352x256arc_v	= get_int ("tweaked", "352x256arc_v",   NULL, 0x17);
	tw368x224arc_h	= get_int ("tweaked", "368x224arc_h",   NULL, 0x6a);
	tw368x224arc_v	= get_int ("tweaked", "368x224arc_v",   NULL, 0x09);
	tw368x240arc_h	= get_int ("tweaked", "368x240arc_h",   NULL, 0x6a);
	tw368x240arc_v	= get_int ("tweaked", "368x240arc_v",   NULL, 0x09);
	tw368x256arc_h	= get_int ("tweaked", "368x256arc_h",   NULL, 0x6a);
	tw368x256arc_v	= get_int ("tweaked", "368x256arc_v",   NULL, 0x17);
	tw512x224arc_h	= get_int ("tweaked", "512x224arc_h",   NULL, 0xbf);
	tw512x224arc_v	= get_int ("tweaked", "512x224arc_v",   NULL, 0x09);
	tw512x256arc_h	= get_int ("tweaked", "512x256arc_h",   NULL, 0xbf);
	tw512x256arc_v	= get_int ("tweaked", "512x256arc_v",   NULL, 0x17);
	tw512x448arc_h	= get_int ("tweaked", "512x448arc_h",   NULL, 0xbf);
	tw512x448arc_v	= get_int ("tweaked", "512x448arc_v",   NULL, 0x09);
	tw512x512arc_h	= get_int ("tweaked", "512x512arc_h",   NULL, 0xbf);
	tw512x512arc_v	= get_int ("tweaked", "512x512arc_v",   NULL, 0x17);
	tw640x480arc_h	= get_int ("tweaked", "640x480arc_h",   NULL, 0xc1);
	tw640x480arc_v	= get_int ("tweaked", "640x480arc_v",   NULL, 0x09);

	/* this is handled externally cause the audit stuff needs it, too */
	get_rom_sample_path (argc, argv, game_index, override_default_rompath);

	/* get the monitor type */
	monitorname = get_string ("config", "monitor", NULL, "standard");
	/* get -centerx */
	center_x = get_int ("config", "centerx", NULL,  0);
	/* get -centery */
	center_y = get_int ("config", "centery", NULL,  0);
	/* get -waitinterlace */
	wait_interlace = get_bool ("config", "waitinterlace", NULL,  0);

	/* process some parameters */
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	if (options.beam > 0x00100000)
		options.beam = 0x00100000;

	if (stricmp (vesamode, "vesa1") == 0)
		gfx_mode = GFX_VESA1;
	else if (stricmp (vesamode, "vesa2b") == 0)
		gfx_mode = GFX_VESA2B;
	else if (stricmp (vesamode, "vesa2l") == 0)
		gfx_mode = GFX_VESA2L;
	else if (stricmp (vesamode, "vesa3") == 0)
		gfx_mode = GFX_VESA3;
	else
	{
		logerror("%s is not a valid entry for vesamode\n", vesamode);
		gfx_mode = GFX_VESA3; /* default to VESA2L */
	}

	/* any option that starts with a digit is taken as a resolution option */
	/* this is to handle the old "-wxh" commandline option. */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && isdigit(argv[i][1]) &&
				(strstr(argv[i],"x") || strstr(argv[i],"X")))
			vectorres = resolution = &argv[i][1];
	}

	/* break up resolution into gfx_width and gfx_height */
	gfx_height = gfx_width = 0;
	if (stricmp (resolution, "auto") != 0)
	{
		char *tmp;
		strncpy (tmpres, resolution, 10);
		tmp = strtok (tmpres, "xX");
		gfx_width = atoi (tmp);
		tmp = strtok (0, "xX");
		if (tmp)
			gfx_height = atoi (tmp);

		options.vector_width = gfx_width;
		options.vector_height = gfx_height;
	}

	/* break up vector resolution into gfx_width and gfx_height */
	if (drivers[game]->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (stricmp (vectorres, "auto") != 0)
		{
			char *tmp;
			strncpy (tmpres, vectorres, 10);
			tmp = strtok (tmpres, "xX");
			gfx_width = atoi (tmp);
			tmp = strtok (0, "xX");
			if (tmp)
				gfx_height = atoi (tmp);

			options.vector_width = gfx_width;
			options.vector_height = gfx_height;
		}

	/* convert joystick name into an Allegro-compliant joystick signature */
	joystick = -2; /* default to invalid value */

	for (i = 0; joy_table[i].name != NULL; i++)
	{
		if (stricmp (joy_table[i].name, joyname) == 0)
		{
			joystick = joy_table[i].id;
			logerror("using joystick %s = %08x\n", joyname,joy_table[i].id);
			break;
		}
	}

	if (joystick == -2)
	{
		logerror("%s is not a valid entry for a joystick\n", joyname);
		joystick = JOY_TYPE_NONE;
	}

	/* get monitor type from supplied name */
	monitor_type = MONITOR_TYPE_STANDARD; /* default to PC monitor */

	for (i = 0; monitor_table[i].name != NULL; i++)
	{
		if ((stricmp (monitor_table[i].name, monitorname) == 0))
		{
			monitor_type = monitor_table[i].id;
			break;
		}
	}

	if (options.mame_debug)
	{
		options.debug_width = gfx_width = 640;
		options.debug_height = gfx_height = 480;
		options.vector_width = gfx_width;
		options.vector_height = gfx_height;
		use_dirty = 0;
	}

#ifdef MESS
	/* Is there an override for softwarepath= for this driver? */
	tmpstr = get_string ((char*)drivers[game]->name, "softwarepath", NULL, NULL);
	if (tmpstr)
		softwarepath = tmpstr;
	logerror("Using Software Path %s for %s\n", softwarepath, drivers[game]->name);
	decompose_software_path(softwarepath);
#endif

}

