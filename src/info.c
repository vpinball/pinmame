#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "datafile.h"

/* Output format indentation */

/* Indentation */
#define INDENT "\t"

/* Output format configuration
	L1 first level
	L2 second level
	B begin a list of items
	E end a list of items
	P begin an item
	N end an item
*/

/* Output unformatted */
/*
#define L1B "("
#define L1P " "
#define L1N ""
#define L1E ")"
#define L2B "("
#define L2P " "
#define L2N ""
#define L2E ")"
*/

/* Output on one level */
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " ("
#define L2P " "
#define L2N ""
#define L2E " )"

/* Output on two levels */
/*
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"
#define L2B " (\n"
#define L2P INDENT INDENT
#define L2N "\n"
#define L2E INDENT ")"
*/

/* Print a string in C format */
static void print_c_string(FILE* out, const char* s)
{
	fprintf(out, "\"");
	if (s)
	{
		while (*s)
		{
			switch (*s)
			{
				case '\a' : fprintf(out, "\\a"); break;
				case '\b' : fprintf(out, "\\b"); break;
				case '\f' : fprintf(out, "\\f"); break;
				case '\n' : fprintf(out, "\\n"); break;
				case '\r' : fprintf(out, "\\r"); break;
				case '\t' : fprintf(out, "\\t"); break;
				case '\v' : fprintf(out, "\\v"); break;
				case '\\' : fprintf(out, "\\\\"); break;
				case '\"' : fprintf(out, "\\\""); break;
				default:
					if (*s>=' ' && *s<='~')
						fprintf(out, "%c", *s);
					else
						fprintf(out, "\\x%02x", (unsigned)(unsigned char)*s);
			}
			++s;
		}
	}
	fprintf(out, "\"");
}

/* Print a string in statement format (remove space, parentesis, ") */
static void print_statement_string(FILE* out, const char* s)
{
	if (s)
	{
		while (*s)
		{
			if (isspace(*s))
			{
				fprintf(out, "_");
			}
			else
			{
				switch (*s)
				{
					case '(' :
					case ')' :
					case '"' :
						fprintf(out, "_");
						break;
					default:
						fprintf(out, "%c", *s);
				}
			}
			++s;
		}
	}
	else
	{
		fprintf(out, "null");
	}
}

static void print_game_switch(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		if ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_NAME)
		{
			int def = input->default_value;
			const char* def_name = 0;

			fprintf(out, L1P "dipswitch" L2B);

			fprintf(out, L2P "name " );
			print_c_string(out,input->name);
			fprintf(out, "%s", L2N);
			++input;

			while ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_SETTING)
			{
				if (def == input->default_value)
					def_name = input->name;
				fprintf(out, L2P "entry " );
				print_c_string(out,input->name);
				fprintf(out, "%s", L2N);
				++input;
			}

			if (def_name)
			{
				fprintf(out, L2P "default ");
				print_c_string(out,def_name);
				fprintf(out, "%s", L2N);
			}

			fprintf(out, L2E L1N);
		}
		else
			++input;
	}
}

static void print_game_input(FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;
	int nplayer = 0;
	const char* control = 0;
	int nbutton = 0;
	int ncoin = 0;
	const char* service = 0;
	const char* tilt = 0;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		switch (input->type & IPF_PLAYERMASK)
		{
			case IPF_PLAYER1:
				if (nplayer<1) nplayer = 1;
				break;
			case IPF_PLAYER2:
				if (nplayer<2) nplayer = 2;
				break;
			case IPF_PLAYER3:
				if (nplayer<3) nplayer = 3;
				break;
			case IPF_PLAYER4:
				if (nplayer<4) nplayer = 4;
				break;
		}
		switch (input->type & ~IPF_MASK)
		{
			case IPT_JOYSTICK_UP:
			case IPT_JOYSTICK_DOWN:
			case IPT_JOYSTICK_LEFT:
			case IPT_JOYSTICK_RIGHT:
				if (input->type & IPF_2WAY)
					control = "joy2way";
				else if (input->type & IPF_4WAY)
					control = "joy4way";
				else
					control = "joy8way";
				break;
			case IPT_JOYSTICKRIGHT_UP:
			case IPT_JOYSTICKRIGHT_DOWN:
			case IPT_JOYSTICKRIGHT_LEFT:
			case IPT_JOYSTICKRIGHT_RIGHT:
			case IPT_JOYSTICKLEFT_UP:
			case IPT_JOYSTICKLEFT_DOWN:
			case IPT_JOYSTICKLEFT_LEFT:
			case IPT_JOYSTICKLEFT_RIGHT:
				if (input->type & IPF_2WAY)
					control = "doublejoy2way";
				else if (input->type & IPF_4WAY)
					control = "doublejoy4way";
				else
					control = "doublejoy8way";
				break;
			case IPT_BUTTON1:
				if (nbutton<1) nbutton = 1;
				break;
			case IPT_BUTTON2:
				if (nbutton<2) nbutton = 2;
				break;
			case IPT_BUTTON3:
				if (nbutton<3) nbutton = 3;
				break;
			case IPT_BUTTON4:
				if (nbutton<4) nbutton = 4;
				break;
			case IPT_BUTTON5:
				if (nbutton<5) nbutton = 5;
				break;
			case IPT_BUTTON6:
				if (nbutton<6) nbutton = 6;
				break;
			case IPT_BUTTON7:
				if (nbutton<7) nbutton = 7;
				break;
			case IPT_BUTTON8:
				if (nbutton<8) nbutton = 8;
				break;
			case IPT_PADDLE:
				control = "paddle";
				break;
			case IPT_DIAL:
				control = "dial";
				break;
			case IPT_TRACKBALL_X:
			case IPT_TRACKBALL_Y:
				control = "trackball";
				break;
			case IPT_AD_STICK_X:
			case IPT_AD_STICK_Y:
				control = "stick";
				break;
			case IPT_COIN1:
				if (ncoin < 1) ncoin = 1;
				break;
			case IPT_COIN2:
				if (ncoin < 2) ncoin = 2;
				break;
			case IPT_COIN3:
				if (ncoin < 3) ncoin = 3;
				break;
			case IPT_COIN4:
				if (ncoin < 4) ncoin = 4;
				break;
			case IPT_SERVICE :
				service = "yes";
				break;
			case IPT_TILT :
				tilt = "yes";
				break;
		}
		++input;
	}

	fprintf(out, L1P "input" L2B);
	fprintf(out, L2P "players %d" L2N, nplayer );
	if (control)
		fprintf(out, L2P "control %s" L2N, control );
	if (nbutton)
		fprintf(out, L2P "buttons %d" L2N, nbutton );
	if (ncoin)
		fprintf(out, L2P "coins %d" L2N, ncoin );
	if (service)
		fprintf(out, L2P "service %s" L2N, service );
	if (tilt)
		fprintf(out, L2P "tilt %s" L2N, tilt );
	fprintf(out, L2E L1N);
}

static void print_game_rom(FILE* out, const struct GameDriver* game)
{
	const struct RomModule *region, *rom, *chunk;
	const struct RomModule *pregion, *prom, *fprom=NULL;
	extern struct GameDriver driver_0;

	if (!game->rom)
		return;

	if (game->clone_of && game->clone_of != &driver_0)
		fprintf(out, L1P "romof %s" L1N, game->clone_of->name);

	for (region = rom_first_region(game); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			char name[100];
			int offset, length, crc, in_parent;

			sprintf(name,ROM_GETNAME(rom));
			offset = ROM_GETOFFSET(rom);
			crc = ROM_GETCRC(rom);

			in_parent = 0;
			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			if (crc && game->clone_of)
			{
				fprom=NULL;
				for (pregion = rom_first_region(game->clone_of); pregion; pregion = rom_next_region(pregion))
					for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
						if (ROM_GETCRC(prom) == crc)
						{
							if (!fprom || !strcmp(ROM_GETNAME(prom), name))
								fprom=prom;
							in_parent = 1;
						}
			}

			fprintf(out, L1P "rom" L2B);
			if (*name)
				fprintf(out, L2P "name %s" L2N, name);
			if(in_parent)
				fprintf(out, L2P "merge %s" L2N, ROM_GETNAME(fprom));
			fprintf(out, L2P "size %d" L2N, length);
			fprintf(out, L2P "crc %08x" L2N, crc);
			switch (ROMREGION_GETTYPE(region))
			{
				case REGION_CPU1: fprintf(out, L2P "region cpu1" L2N); break;
				case REGION_CPU2: fprintf(out, L2P "region cpu2" L2N); break;
				case REGION_CPU3: fprintf(out, L2P "region cpu3" L2N); break;
				case REGION_CPU4: fprintf(out, L2P "region cpu4" L2N); break;
				case REGION_CPU5: fprintf(out, L2P "region cpu5" L2N); break;
				case REGION_CPU6: fprintf(out, L2P "region cpu6" L2N); break;
				case REGION_CPU7: fprintf(out, L2P "region cpu7" L2N); break;
				case REGION_CPU8: fprintf(out, L2P "region cpu8" L2N); break;
				case REGION_GFX1: fprintf(out, L2P "region gfx1" L2N); break;
				case REGION_GFX2: fprintf(out, L2P "region gfx2" L2N); break;
				case REGION_GFX3: fprintf(out, L2P "region gfx3" L2N); break;
				case REGION_GFX4: fprintf(out, L2P "region gfx4" L2N); break;
				case REGION_GFX5: fprintf(out, L2P "region gfx5" L2N); break;
				case REGION_GFX6: fprintf(out, L2P "region gfx6" L2N); break;
				case REGION_GFX7: fprintf(out, L2P "region gfx7" L2N); break;
				case REGION_GFX8: fprintf(out, L2P "region gfx8" L2N); break;
				case REGION_PROMS: fprintf(out, L2P "region proms" L2N); break;
				case REGION_SOUND1: fprintf(out, L2P "region sound1" L2N); break;
				case REGION_SOUND2: fprintf(out, L2P "region sound2" L2N); break;
				case REGION_SOUND3: fprintf(out, L2P "region sound3" L2N); break;
				case REGION_SOUND4: fprintf(out, L2P "region sound4" L2N); break;
				case REGION_SOUND5: fprintf(out, L2P "region sound5" L2N); break;
				case REGION_SOUND6: fprintf(out, L2P "region sound6" L2N); break;
				case REGION_SOUND7: fprintf(out, L2P "region sound7" L2N); break;
				case REGION_SOUND8: fprintf(out, L2P "region sound8" L2N); break;
				case REGION_USER1: fprintf(out, L2P "region user1" L2N); break;
				case REGION_USER2: fprintf(out, L2P "region user2" L2N); break;
				case REGION_USER3: fprintf(out, L2P "region user3" L2N); break;
				case REGION_USER4: fprintf(out, L2P "region user4" L2N); break;
				case REGION_USER5: fprintf(out, L2P "region user5" L2N); break;
				case REGION_USER6: fprintf(out, L2P "region user6" L2N); break;
				case REGION_USER7: fprintf(out, L2P "region user7" L2N); break;
				case REGION_USER8: fprintf(out, L2P "region user8" L2N); break;
				default: fprintf(out, L2P "region 0x%x" L2N, ROMREGION_GETTYPE(region));
		}
		switch (ROMREGION_GETFLAGS(region))
		{
			case 0:
				break;
			case ROMREGION_SOUNDONLY:
				fprintf(out, L2P "flags soundonly" L2N);
				break;
			case ROMREGION_DISPOSE:
				fprintf(out, L2P "flags dispose" L2N);
				break;
			default:
				fprintf(out, L2P "flags 0x%x" L2N, ROMREGION_GETFLAGS(region));
		}
		fprintf(out, L2P "offs %x", offset);
		fprintf(out, L2E L1N);
	}
}

static void print_game_sample(FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES || HAS_VLM5030)
	struct InternalMachineDriver drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
#if (HAS_SAMPLES)
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
#endif
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				/* output sampleof only if different from game name */
				if (strcmp(samplenames[k] + 1, game->name)!=0)
					fprintf(out, L1P "sampleof %s" L1N, samplenames[k] + 1);
				++k;
			}
			while (samplenames[k] != 0) {
				/* Check if is not empty */
				if (*samplenames[k]) {
					/* Check if sample is duplicate */
					int l = 0;
					while (l<k && strcmp(samplenames[k],samplenames[l])!=0)
						++l;
					if (l==k)
						fprintf(out, L1P "sample %s" L1N, samplenames[k]);
				}
				++k;
			}
		}
	}
#endif
}

static void print_game_micro(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;
	int j;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	for(j=0;j<MAX_CPU;++j)
	{
		if (cpu[j].cpu_type!=0)
		{
			fprintf(out, L1P "chip" L2B);
			if (cpu[j].cpu_type & CPU_AUDIO_CPU)
				fprintf(out, L2P "type cpu flags audio" L2N);
			else
				fprintf(out, L2P "type cpu" L2N);

			fprintf(out, L2P "name ");
			print_statement_string(out, cputype_name(cpu[j].cpu_type));
			fprintf(out, "%s", L2N);

			fprintf(out, L2P "clock %d" L2N, cpu[j].cpu_clock);
			fprintf(out, L2E L1N);
		}
	}

	for(j=0;j<MAX_SOUND;++j) if (sound[j].sound_type)
	{
		if (sound[j].sound_type)
		{
			int num = sound_num(&sound[j]);
			int l;

			if (num == 0) num = 1;

			for(l=0;l<num;++l)
			{
				fprintf(out, L1P "chip" L2B);
				fprintf(out, L2P "type audio" L2N);
				fprintf(out, L2P "name ");
				print_statement_string(out, sound_name(&sound[j]));
				fprintf(out, "%s", L2N);
				if (sound_clock(&sound[j]))
					fprintf(out, L2P "clock %d" L2N, sound_clock(&sound[j]));
				fprintf(out, L2E L1N);
			}
		}
	}
}

static void print_game_video(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	int dx;
	int dy;
	int ax;
	int ay;
	int showxy;
	int orientation;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, L1P "video" L2B);
	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
	{
		fprintf(out, L2P "screen vector" L2N);
		showxy = 0;
	}
	else
	{
		fprintf(out, L2P "screen raster" L2N);
		showxy = 1;
	}

	if (game->flags & ORIENTATION_SWAP_XY)
	{
		ax = VIDEO_ASPECT_RATIO_DEN(driver.video_attributes);
		ay = VIDEO_ASPECT_RATIO_NUM(driver.video_attributes);
		if (ax == 0 && ay == 0) {
			ax = 3;
			ay = 4;
		}
		dx = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		dy = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		orientation = 1;
	}
	else
	{
		ax = VIDEO_ASPECT_RATIO_NUM(driver.video_attributes);
		ay = VIDEO_ASPECT_RATIO_DEN(driver.video_attributes);
		if (ax == 0 && ay == 0) {
			ax = 4;
			ay = 3;
		}
		dx = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		dy = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		orientation = 0;
	}

	fprintf(out, L2P "orientation %s" L2N, orientation ? "vertical" : "horizontal" );
	if (showxy)
	{
		fprintf(out, L2P "x %d" L2N, dx);
		fprintf(out, L2P "y %d" L2N, dy);
	}

	fprintf(out, L2P "aspectx %d" L2N, ax);
	fprintf(out, L2P "aspecty %d" L2N, ay);

	fprintf(out, L2P "freq %f" L2N, driver.frames_per_second);
	fprintf(out, L2E L1N);
}

static void print_game_sound(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;
	const struct MachineCPU* cpu;
	const struct MachineSound* sound;

	/* check if the game have sound emulation */
	int has_sound = 0;
	int i;

	expand_machine_driver(game->drv, &driver);
	cpu = driver.cpu;
	sound = driver.sound;

	i = 0;
	while (i < MAX_SOUND && !has_sound)
	{
		if (sound[i].sound_type)
			has_sound = 1;
		++i;
	}
	i = 0;
	while (i < MAX_CPU && !has_sound)
	{
		if  ((cpu[i].cpu_type & CPU_AUDIO_CPU)!=0)
			has_sound = 1;
		++i;
	}

	fprintf(out, L1P "sound" L2B);

	/* sound channel */
	if (has_sound)
	{
		if (driver.sound_attributes & SOUND_SUPPORTS_STEREO)
			fprintf(out, L2P "channels 2" L2N);
		else
			fprintf(out, L2P "channels 1" L2N);
	}
	else
		fprintf(out, L2P "channels 0" L2N);

	fprintf(out, L2E L1N);
}

#define HISTORY_BUFFER_MAX 16384

static void print_game_history(FILE* out, const struct GameDriver* game)
{
	char buffer[HISTORY_BUFFER_MAX];

	if (load_driver_history(game,buffer,HISTORY_BUFFER_MAX)==0)
	{
		fprintf(out, L1P "history ");
		print_c_string(out, buffer);
		fprintf(out, "%s", L1N);
	}
}

static void print_game_driver(FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, L1P "driver" L2B);
	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, L2P "status preliminary" L2N);
	else
		fprintf(out, L2P "status good" L2N);

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, L2P "color preliminary" L2N);
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, L2P "color imperfect" L2N);
	else
		fprintf(out, L2P "color good" L2N);

	if (game->flags & GAME_NO_SOUND)
		fprintf(out, L2P "sound preliminary" L2N);
	else if (game->flags & GAME_IMPERFECT_SOUND)
		fprintf(out, L2P "sound imperfect" L2N);
	else
		fprintf(out, L2P "sound good" L2N);

	fprintf(out, L2P "palettesize %d" L2N, driver.total_colors);

	if (driver.video_attributes & VIDEO_SUPPORTS_DIRTY)
		fprintf(out, L2P "blit dirty" L2N);
	else
		fprintf(out, L2P "blit plain" L2N);

	fprintf(out, L2E L1N);
}

/* Print the MAME info record for a game */
static void print_game_info(FILE* out, const struct GameDriver* game)
{

#ifndef MESS
	fprintf(out, "game" L1B );
#else
	fprintf(out, "machine" L1B );
#endif

	fprintf(out, L1P "name %s" L1N, game->name );

	if (game->description)
	{
		fprintf(out, L1P "description ");
		print_c_string(out, game->description );
		fprintf(out, "%s", L1N);
	}

	/* print the year only if is a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, L1P "year %s" L1N, game->year );

	if (game->manufacturer)
	{
		fprintf(out, L1P "manufacturer ");
		print_c_string(out, game->manufacturer );
		fprintf(out, "%s", L1N);
	}

	print_game_history(out,game);

	if (game->clone_of && !(game->clone_of->flags & NOT_A_DRIVER))
		fprintf(out, L1P "cloneof %s" L1N, game->clone_of->name);

	print_game_rom(out,game);
	print_game_sample(out,game);
	print_game_micro(out,game);
	print_game_video(out,game);
	print_game_sound(out,game);
	print_game_input(out,game);
	print_game_switch(out,game);
	print_game_driver(out,game);

	fprintf(out, L1E);
}

#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME)
/* Print the resource info */
static void print_resource_info(FILE* out, const struct GameDriver* game)
{
	fprintf(out, "resource" L1B );

	fprintf(out, L1P "name %s" L1N, game->name );

	if (game->description)
	{
		fprintf(out, L1P "description ");
		print_c_string(out, game->description );
		fprintf(out, "%s", L1N);
	}

	/* print the year only if it's a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, L1P "year %s" L1N, game->year );

	if (game->manufacturer)
	{
		fprintf(out, L1P "manufacturer ");
		print_c_string(out, game->manufacturer );
		fprintf(out, "%s", L1N);
	}

	print_game_rom(out,game);
	print_game_sample(out,game);

	fprintf(out, L1E);
}

/* Import the driver object and print it as a resource */
#define PRINT_RESOURCE(s) \
	{ \
		extern struct GameDriver driver_##s; \
		print_resource_info( out, &driver_##s ); \
	}

#endif

/* Print all the MAME info database */
void print_mame_info(FILE* out, const struct GameDriver* games[])
{
	int j;

	/* print games */
	for(j=0;games[j];++j)
		print_game_info( out, games[j] );

	/* print the resources (only if linked) */
#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME)
	PRINT_RESOURCE(neogeo);
#if !defined(NEOMAME)
	PRINT_RESOURCE(cvs);
	PRINT_RESOURCE(decocass);
	PRINT_RESOURCE(playch10);
#endif
#endif
}
