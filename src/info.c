#include <ctype.h>

#include "driver.h"
#include "sound/samples.h"
#include "info.h"
#include "hash.h"
#include "datafile.h"

/* Format */
#define SELECT(a,b) (OUTPUT_XML ? (b) : (a))

/* MESS/MAME configuration */
#ifdef MESS
#define XML_ROOT "mess"
#define XML_TOP "machine"
#else
#define XML_ROOT "mame"
#define XML_TOP "game"
#endif

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

/* Print a free format string */
static void print_free_string(int OUTPUT_XML, FILE* out, const char* s)
{
	if (!OUTPUT_XML)
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
	else
	{
		if (s)
		{
			while (*s)
			{
				switch (*s)
				{
					case '\"' : fprintf(out, "&quot;"); break;
					case '&'  : fprintf(out, "&amp;"); break;
					case '<'  : fprintf(out, "&lt;"); break;
					case '>'  : fprintf(out, "&gt;"); break;
					default:
						if (*s>=' ' && *s<='~')
							fprintf(out, "%c", *s);
						else
							fprintf(out, "&#%d;", (unsigned)(unsigned char)*s);
				}
				++s;
			}
		}
	}
}

/* Print a statement string */
static void print_statement_string(int OUTPUT_XML, FILE* out, const char* s)
{
	if (OUTPUT_XML)
	{
		print_free_string(OUTPUT_XML, out, s);
		return;
	}
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

static void print_game_switch(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	const struct InputPortTiny* input = game->input_ports;

	while ((input->type & ~IPF_MASK) != IPT_END)
	{
		if ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_NAME)
		{
			int def = input->default_value;

			fprintf(out, SELECT(L1P "dipswitch" L2B, "\t\t<dipswitch"));

			fprintf(out, SELECT(L2P "name ", " name=\""));
			print_free_string(OUTPUT_XML, out, input->name);
			fprintf(out, "%s", SELECT(L2N, "\""));
			++input;

			fprintf(out, "%s", SELECT("", ">\n"));

			while ((input->type & ~IPF_MASK)==IPT_DIPSWITCH_SETTING)
			{
				fprintf(out, SELECT(L2P "entry ", "\t\t\t<dipvalue"));
				fprintf(out, "%s", SELECT("", " name=\""));
				print_free_string(OUTPUT_XML, out, input->name);
				fprintf(out, "%s", SELECT(L2N, "\""));
				if (def == input->default_value)
				{
					if (OUTPUT_XML)
					{
						fprintf(out, " default=\"yes\"");
					}
					else
					{
						fprintf(out, L2P "default ");
						print_free_string(OUTPUT_XML, out, input->name);
						fprintf(out, "%s", L2N);
					}
				}

				fprintf(out, "%s", SELECT("", "/>\n"));

				++input;
			}

			fprintf(out, SELECT(L2E L1N, "\t\t</dipswitch>\n"));
		}
		else
			++input;
	}
}

static void print_game_input(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
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
		/* skip analog extension fields */
		if ((input->type & ~IPF_MASK) != IPT_EXTENSION)
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
				case IPF_PLAYER5:
					if (nplayer<5) nplayer = 5;
					break;
				case IPF_PLAYER6:
					if (nplayer<6) nplayer = 6;
					break;
				case IPF_PLAYER7:
					if (nplayer<7) nplayer = 7;
					break;
				case IPF_PLAYER8:
					if (nplayer<8) nplayer = 8;
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
				case IPT_BUTTON9:
					if (nbutton<9) nbutton = 9;
					break;
				case IPT_BUTTON10:
					if (nbutton<10) nbutton = 10;
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
				case IPT_LIGHTGUN_X:
				case IPT_LIGHTGUN_Y:
					control = "lightgun";
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
				case IPT_COIN5:
					if (ncoin < 5) ncoin = 5;
					break;
				case IPT_COIN6:
					if (ncoin < 6) ncoin = 6;
					break;
				case IPT_COIN7:
					if (ncoin < 7) ncoin = 7;
					break;
				case IPT_COIN8:
					if (ncoin < 8) ncoin = 8;
					break;
				case IPT_SERVICE :
					service = "yes";
					break;
				case IPT_TILT :
					tilt = "yes";
					break;
			}
		}
		++input;
	}

	fprintf(out, SELECT(L1P "input" L2B, "\t\t<input"));
	fprintf(out, SELECT(L2P "players %d" L2N, " players=\"%d\""), nplayer );
	if (control)
		fprintf(out, SELECT(L2P "control %s" L2N, " control=\"%s\""), control );
	if (nbutton)
		fprintf(out, SELECT(L2P "buttons %d" L2N, " buttons=\"%d\""), nbutton );
	if (ncoin)
		fprintf(out, SELECT(L2P "coins %d" L2N, " coins=\"%d\""), ncoin );
	if (service)
		fprintf(out, SELECT(L2P "service %s" L2N, " service=\"%s\""), service );
	if (tilt)
		fprintf(out, SELECT(L2P "tilt %s" L2N, " tilt=\"%s\""), tilt );
	fprintf(out, SELECT(L2E L1N, "/>\n"));
}

static void print_game_bios(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	const struct SystemBios *thisbios;

	if(!game->bios)
		return;

	thisbios = game->bios;

	/* Match against bios short names */
	while(!BIOSENTRY_ISEND(thisbios))
	{
		fprintf(out, SELECT(L1P "biosset" L2B, "\t\t<biosset"));

		if (thisbios->_name)
			fprintf(out, SELECT(L2P "name %s" L2N, " name=\"%s\""), thisbios->_name);
		if (thisbios->_description)
			fprintf(out, SELECT(L2P "description \"%s\"" L2N, " description=\"%s\""), thisbios->_description);
		if (thisbios->value == 0)
			fprintf(out, SELECT(L2P "default yes" L2N, " default=\"yes\""));

		fprintf(out, SELECT(L2E L1N, "/>\n"));

		thisbios++;
	}
}

static void print_game_rom(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	const struct RomModule *region, *rom, *chunk;
	const struct RomModule *pregion, *prom, *fprom=NULL;
//	extern struct GameDriver driver_0;

	if (!game->rom)
		return;

	for (region = rom_first_region(game); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			int offset, length, in_parent, is_disk, is_bios, found_bios, i;
			char name[100], bios_name[100];

			strcpy(name,ROM_GETNAME(rom));
			offset = ROM_GETOFFSET(rom);
			is_disk = ROMREGION_ISDISKDATA(region);
			is_bios = ROM_GETBIOSFLAGS(rom);

			in_parent = 0;
			length = 0;
			for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
				length += ROM_GETLENGTH(chunk);

			if (!ROM_NOGOODDUMP(rom) && game->clone_of)
			{
				fprom=NULL;
				for (pregion = rom_first_region(game->clone_of); pregion; pregion = rom_next_region(pregion))
					for (prom = rom_first_file(pregion); prom; prom = rom_next_file(prom))
						if (hash_data_is_equal(ROM_GETHASHDATA(rom), ROM_GETHASHDATA(prom), 0))
						{
							if (!fprom || !strcmp(ROM_GETNAME(prom), name))
								fprom=prom;
							in_parent = 1;
						}
			}

			found_bios = 0;
			if(!is_disk && is_bios && game->bios)
			{
				const struct SystemBios *thisbios = game->bios;

				/* Match against bios short names */
				while(!found_bios && !BIOSENTRY_ISEND(thisbios) )
				{
					if((is_bios-1) == thisbios->value) /* Note '-1' */
					{
						strcpy(bios_name,thisbios->_name);
						found_bios = 1;
					}

					thisbios++;
				}
			}


			if (!is_disk)
				fprintf(out, SELECT(L1P "rom" L2B, "\t\t<rom"));
			else
				fprintf(out, SELECT(L1P "disk" L2B, "\t\t<disk"));

			if (*name)
				fprintf(out, SELECT(L2P "name %s" L2N, " name=\"%s\""), name);
			if (!is_disk && in_parent)
				fprintf(out, SELECT(L2P "merge %s" L2N, " merge=\"%s\""), ROM_GETNAME(fprom));
			if (!is_disk && found_bios)
				fprintf(out, SELECT(L2P "bios %s" L2N, " bios=\"%s\""), bios_name);
			if (!is_disk)
				fprintf(out, SELECT(L2P "size %d" L2N, " size=\"%d\""), length);

			/* dump checksum information only if there is a known dump */
			if (!hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
			{
				for (i=0;i<HASH_NUM_FUNCTIONS;i++)
				{
					int func = 1<<i;
					const char* func_name = hash_function_name(func);
					char checksum[1000];

					if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), func, checksum))
					{
						fprintf(out, SELECT(L2P "%s %s" L2N, " %s=\"%s\""), func_name, checksum);
					}
				}
			}

			switch (ROMREGION_GETTYPE(region))
			{
				case REGION_CPU1: fprintf(out, SELECT(L2P "region cpu1" L2N, " region=\"cpu1\"")); break;
				case REGION_CPU2: fprintf(out, SELECT(L2P "region cpu2" L2N, " region=\"cpu2\"")); break;
				case REGION_CPU3: fprintf(out, SELECT(L2P "region cpu3" L2N, " region=\"cpu3\"")); break;
				case REGION_CPU4: fprintf(out, SELECT(L2P "region cpu4" L2N, " region=\"cpu4\"")); break;
				case REGION_CPU5: fprintf(out, SELECT(L2P "region cpu5" L2N, " region=\"cpu5\"")); break;
				case REGION_CPU6: fprintf(out, SELECT(L2P "region cpu6" L2N, " region=\"cpu6\"")); break;
				case REGION_CPU7: fprintf(out, SELECT(L2P "region cpu7" L2N, " region=\"cpu7\"")); break;
				case REGION_CPU8: fprintf(out, SELECT(L2P "region cpu8" L2N, " region=\"cpu8\"")); break;
				case REGION_GFX1: fprintf(out, SELECT(L2P "region gfx1" L2N, " region=\"gfx1\"")); break;
				case REGION_GFX2: fprintf(out, SELECT(L2P "region gfx2" L2N, " region=\"gfx2\"")); break;
				case REGION_GFX3: fprintf(out, SELECT(L2P "region gfx3" L2N, " region=\"gfx3\"")); break;
				case REGION_GFX4: fprintf(out, SELECT(L2P "region gfx4" L2N, " region=\"gfx4\"")); break;
				case REGION_GFX5: fprintf(out, SELECT(L2P "region gfx5" L2N, " region=\"gfx5\"")); break;
				case REGION_GFX6: fprintf(out, SELECT(L2P "region gfx6" L2N, " region=\"gfx6\"")); break;
				case REGION_GFX7: fprintf(out, SELECT(L2P "region gfx7" L2N, " region=\"gfx7\"")); break;
				case REGION_GFX8: fprintf(out, SELECT(L2P "region gfx8" L2N, " region=\"gfx8\"")); break;
				case REGION_PROMS: fprintf(out, SELECT(L2P "region proms" L2N, " region=\"proms\"")); break;
				case REGION_SOUND1: fprintf(out, SELECT(L2P "region sound1" L2N, " region=\"sound1\"")); break;
				case REGION_SOUND2: fprintf(out, SELECT(L2P "region sound2" L2N, " region=\"sound2\"")); break;
				case REGION_SOUND3: fprintf(out, SELECT(L2P "region sound3" L2N, " region=\"sound3\"")); break;
				case REGION_SOUND4: fprintf(out, SELECT(L2P "region sound4" L2N, " region=\"sound4\"")); break;
				case REGION_SOUND5: fprintf(out, SELECT(L2P "region sound5" L2N, " region=\"sound5\"")); break;
				case REGION_SOUND6: fprintf(out, SELECT(L2P "region sound6" L2N, " region=\"sound6\"")); break;
				case REGION_SOUND7: fprintf(out, SELECT(L2P "region sound7" L2N, " region=\"sound7\"")); break;
				case REGION_SOUND8: fprintf(out, SELECT(L2P "region sound8" L2N, " region=\"sound8\"")); break;
				case REGION_USER1: fprintf(out, SELECT(L2P "region user1" L2N, " region=\"user1\"")); break;
				case REGION_USER2: fprintf(out, SELECT(L2P "region user2" L2N, " region=\"user2\"")); break;
				case REGION_USER3: fprintf(out, SELECT(L2P "region user3" L2N, " region=\"user3\"")); break;
				case REGION_USER4: fprintf(out, SELECT(L2P "region user4" L2N, " region=\"user4\"")); break;
				case REGION_USER5: fprintf(out, SELECT(L2P "region user5" L2N, " region=\"user5\"")); break;
				case REGION_USER6: fprintf(out, SELECT(L2P "region user6" L2N, " region=\"user6\"")); break;
				case REGION_USER7: fprintf(out, SELECT(L2P "region user7" L2N, " region=\"user7\"")); break;
				case REGION_USER8: fprintf(out, SELECT(L2P "region user8" L2N, " region=\"user8\"")); break;
				case REGION_DISKS: fprintf(out, SELECT(L2P "region disks" L2N, " region=\"disks\"")); break;
				default: fprintf(out, SELECT(L2P "region 0x%x" L2N, " region=\"0x%x\""), ROMREGION_GETTYPE(region));
		}

		if (!is_disk)
		{
			if (OUTPUT_XML)
			{
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
					fprintf(out, " status=\"nodump\"");
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
					fprintf(out, " status=\"baddump\"");
				if (ROMREGION_GETFLAGS(region) & ROMREGION_DISPOSE)
					fprintf(out, " dispose=\"yes\"");
				if (ROMREGION_GETFLAGS(region) & ROMREGION_SOUNDONLY)
					fprintf(out, " soundonly=\"yes\"");
			}
			else
			{
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_NO_DUMP))
					fprintf(out, L2P "flags nodump" L2N);
				if (hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP))
					fprintf(out, L2P "flags baddump" L2N);
				if (ROMREGION_GETFLAGS(region) & ROMREGION_DISPOSE)
					fprintf(out, L2P "flags dispose" L2N);
				if (ROMREGION_GETFLAGS(region) & ROMREGION_SOUNDONLY)
					fprintf(out, L2P "flags soundonly" L2N);
			}

			fprintf(out, SELECT(L2P "offs %x", " offset=\"%x\""), offset);
			fprintf(out, SELECT(L2E L1N, "/>\n"));
		}
		else
		{
			fprintf(out, SELECT(L2P "index %x", " index=\"%x\""), DISK_GETINDEX(rom));
			fprintf(out, SELECT(L2E L1N, "/>\n"));
		}
	}
}

static void print_game_sampleof(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES)
	struct InternalMachineDriver drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				/* output sampleof only if different from game name */
				if (strcmp(samplenames[k] + 1, game->name)!=0)
					fprintf(out, SELECT(L1P "sampleof %s" L1N, " sampleof=\"%s\""), samplenames[k] + 1);
				++k;
			}
		}
	}
#endif
}

static void print_game_sample(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
#if (HAS_SAMPLES)
	struct InternalMachineDriver drv;
	int i;

	expand_machine_driver(game->drv, &drv);

	for( i = 0; drv.sound[i].sound_type && i < MAX_SOUND; i++ )
	{
		const char **samplenames = NULL;
		if( drv.sound[i].sound_type == SOUND_SAMPLES )
			samplenames = ((struct Samplesinterface *)drv.sound[i].sound_interface)->samplenames;
		if (samplenames != 0 && samplenames[0] != 0) {
			int k = 0;
			if (samplenames[k][0]=='*')
			{
				++k;
			}
			while (samplenames[k] != 0) {
				/* check if is not empty */
				if (*samplenames[k]) {
					/* check if sample is duplicate */
					int l = 0;
					while (l<k && strcmp(samplenames[k],samplenames[l])!=0)
						++l;
					if (l==k)
						fprintf(out, SELECT(L1P "sample %s" L1N, "\t\t<sample name=\"%s\"/>\n"), samplenames[k]);
				}
				++k;
			}
		}
	}
#endif
}

static void print_game_micro(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
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
			fprintf(out, SELECT(L1P "chip" L2B, "\t\t<chip"));
			if (cpu[j].cpu_flags & CPU_AUDIO_CPU)
				fprintf(out, SELECT(L2P "type cpu flags audio" L2N, " type=\"cpu\" soundonly=\"yes\""));
			else
				fprintf(out, SELECT(L2P "type cpu" L2N, " type=\"cpu\""));

			fprintf(out, SELECT(L2P "name ", " name=\""));
			print_statement_string(OUTPUT_XML, out, cputype_name(cpu[j].cpu_type));
			fprintf(out, "%s", SELECT(L2N, "\""));

			fprintf(out, SELECT(L2P "clock %d" L2N, " clock=\"%d\""), cpu[j].cpu_clock);
			fprintf(out, SELECT(L2E L1N, "/>\n"));
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
				fprintf(out, SELECT(L1P "chip" L2B, "\t\t<chip"));
				fprintf(out, SELECT(L2P "type audio" L2N, " type=\"audio\""));
				fprintf(out, SELECT(L2P "name ", " name=\""));
				print_statement_string(OUTPUT_XML, out, sound_name(&sound[j]));
				fprintf(out, "%s", SELECT(L2N, "\""));
				if (sound_clock(&sound[j]))
					fprintf(out, SELECT(L2P "clock %d" L2N, " clock=\"%d\""), sound_clock(&sound[j]));
				fprintf(out, SELECT(L2E L1N, "/>\n"));
			}
		}
	}
}

static void print_game_video(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	int dx;
	int dy;
	int ax;
	int ay;
	int showxy;
	int orientation;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, SELECT(L1P "video" L2B, "\t\t<video"));
	if (driver.video_attributes & VIDEO_TYPE_VECTOR)
	{
		fprintf(out, SELECT(L2P "screen vector" L2N, " screen=\"vector\""));
		showxy = 0;
	}
	else
	{
		fprintf(out, SELECT(L2P "screen raster" L2N, " screen=\"raster\""));
		showxy = 1;
	}

	if (game->flags & ORIENTATION_SWAP_XY)
	{
		ax = driver.aspect_y;
		ay = driver.aspect_x;
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
		ax = driver.aspect_x;
		ay = driver.aspect_y;
		if (ax == 0 && ay == 0) {
			ax = 4;
			ay = 3;
		}
		dx = driver.default_visible_area.max_x - driver.default_visible_area.min_x + 1;
		dy = driver.default_visible_area.max_y - driver.default_visible_area.min_y + 1;
		orientation = 0;
	}

	fprintf(out, SELECT(L2P "orientation %s" L2N, " orientation=\"%s\""), orientation ? "vertical" : "horizontal" );
	if (showxy)
	{
		fprintf(out, SELECT(L2P "x %d" L2N, " width=\"%d\""), dx);
		fprintf(out, SELECT(L2P "y %d" L2N, " height=\"%d\""), dy);
	}

	fprintf(out, SELECT(L2P "aspectx %d" L2N, " aspectx=\"%d\""), ax);
	fprintf(out, SELECT(L2P "aspecty %d" L2N, " aspecty=\"%d\""), ay);

	fprintf(out, SELECT(L2P "freq %f" L2N, " refresh=\"%f\""), driver.frames_per_second);
	fprintf(out, SELECT(L2E L1N, "/>\n"));
}

static void print_game_sound(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
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
		if  ((cpu[i].cpu_flags & CPU_AUDIO_CPU)!=0)
			has_sound = 1;
		++i;
	}

	fprintf(out, SELECT(L1P "sound" L2B, "\t\t<sound"));

	/* sound channel */
	if (has_sound)
	{
		if (driver.sound_attributes & SOUND_SUPPORTS_STEREO)
			fprintf(out, SELECT(L2P "channels 2" L2N, " channels=\"2\""));
		else
			fprintf(out, SELECT(L2P "channels 1" L2N, " channels=\"1\""));
	}
	else
		fprintf(out, SELECT(L2P "channels 0" L2N, " channels=\"0\""));

	fprintf(out, SELECT(L2E L1N, "/>\n"));
}

#define HISTORY_BUFFER_MAX 16384

static void print_game_history(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	char buffer[HISTORY_BUFFER_MAX];

	if (load_driver_history(game,buffer,HISTORY_BUFFER_MAX)==0)
	{
		fprintf(out, SELECT(L1P "history ", "\t\t<history>"));
		print_free_string(OUTPUT_XML, out, buffer);
		fprintf(out, SELECT(L1N, "</history>\n"));
	}
}

static void print_game_driver(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	struct InternalMachineDriver driver;

	expand_machine_driver(game->drv, &driver);

	fprintf(out, SELECT(L1P "driver" L2B, "\t\t<driver"));
	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, SELECT(L2P "status preliminary" L2N, " status=\"preliminary\""));
	else
		fprintf(out, SELECT(L2P "status good" L2N, " status=\"good\""));

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, SELECT(L2P "color preliminary" L2N, " color=\"preliminary\""));
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, SELECT(L2P "color imperfect" L2N, " color=\"imperfect\""));
	else
		fprintf(out, SELECT(L2P "color good" L2N, " color=\"good\""));

	if (game->flags & GAME_NO_SOUND)
		fprintf(out, SELECT(L2P "sound preliminary" L2N, " sound=\"preliminary\""));
	else if (game->flags & GAME_IMPERFECT_SOUND)
		fprintf(out, SELECT(L2P "sound imperfect" L2N, " sound=\"imperfect\""));
	else
		fprintf(out, SELECT(L2P "sound good" L2N, " sound=\"good\""));

	fprintf(out, SELECT(L2P "palettesize %d" L2N, " palettesize=\"%d\""), driver.total_colors);

	fprintf(out, SELECT(L2E L1N, "/>\n"));
}

#ifdef MESS
static void print_game_device(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	const struct IODevice* dev = device_first(game);

	while (dev) {
		fprintf(out, SELECT(L1P "device" L2B, "\t\t<device"));
		fprintf(out, SELECT(L2P "name ", " name=\""));
		print_statement_string(OUTPUT_XML, out, device_typename(dev->type));
		fprintf(out, "%s", SELECT(L2N, "\""));
		fprintf(out, "%s", SELECT("", ">\n"));

		if (dev->file_extensions) {
			const char* ext = dev->file_extensions;
			while (*ext) {
				fprintf(out, SELECT(L2P "ext ", "\t\t\t<extension"));
				fprintf(out, "%s", SELECT("", " name=\""));
				print_free_string(OUTPUT_XML, out, ext);
				fprintf(out, "%s", SELECT(L2N, "\""));
				fprintf(out, "%s", SELECT("", "/>\n"));
				ext += strlen(ext) + 1;
			}
		}

		fprintf(out, SELECT(L2E L1N, "\t\t</device>\n"));

		dev = device_next(game, dev);
	}
}
#endif

/* Print the MAME info record for a game */
static void print_game_info(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	extern struct GameDriver driver_0;

	fprintf(out, SELECT(XML_TOP L1B, "\t<" XML_TOP));

	fprintf(out, SELECT(L1P "name %s" L1N, " name=\"%s\""), game->name );

	if (game->clone_of && !(game->clone_of->flags & NOT_A_DRIVER))
		fprintf(out, SELECT(L1P "cloneof %s" L1N, " cloneof=\"%s\""), game->clone_of->name);

	if (game->clone_of && game->clone_of != &driver_0)
		fprintf(out, SELECT(L1P "romof %s" L1N, " romof=\"%s\""), game->clone_of->name);

	print_game_sampleof(OUTPUT_XML, out, game);

	fprintf(out, "%s", SELECT("", ">\n"));

	if (game->description)
	{
		fprintf(out, SELECT(L1P "description ", "\t\t<description>"));
		print_free_string(OUTPUT_XML, out, game->description);
		fprintf(out, SELECT(L1N, "</description>\n"));
	}

	/* print the year only if is a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, SELECT(L1P "year %s" L1N, "\t\t<year>%s</year>\n"), game->year );

	if (game->manufacturer)
	{
		fprintf(out, SELECT(L1P "manufacturer ", "\t\t<manufacturer>"));
		print_free_string(OUTPUT_XML, out, game->manufacturer);
		fprintf(out, SELECT(L1N, "</manufacturer>\n"));
	}

	print_game_history(OUTPUT_XML, out, game);
	print_game_bios(OUTPUT_XML, out, game);
	print_game_rom(OUTPUT_XML, out, game);
	print_game_sample(OUTPUT_XML, out, game);
	print_game_micro(OUTPUT_XML, out, game);
	print_game_video(OUTPUT_XML, out, game);
	print_game_sound(OUTPUT_XML, out, game);
	print_game_input(OUTPUT_XML, out, game);
	print_game_switch(OUTPUT_XML, out, game);
	print_game_driver(OUTPUT_XML, out, game);
#ifdef MESS
	print_game_device(OUTPUT_XML, out, game);
#endif

	fprintf(out, SELECT(L1E, "\t</" XML_TOP ">\n"));
}

#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME) && !defined(MMSND)
/* Print the resource info */
static void print_resource_info(int OUTPUT_XML, FILE* out, const struct GameDriver* game)
{
	fprintf(out, SELECT("resource" L1B, "\t<" XML_TOP " runnable=\"no\"") );

	fprintf(out, SELECT(L1P "name %s" L1N, " name=\"%s\""), game->name );

	fprintf(out, "%s", SELECT("", ">\n"));

	if (game->description)
	{
		fprintf(out, SELECT(L1P "description ", "\t\t<description>"));
		print_free_string(OUTPUT_XML, out, game->description);
		fprintf(out, SELECT(L1N, "</description>\n"));
	}

	/* print the year only if it's a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, SELECT(L1P "year %s" L1N, "\t\t<year>%s</year>\n"), game->year );

	if (game->manufacturer)
	{
		fprintf(out, SELECT(L1P "manufacturer ", "\t\t<manufacturer>"));
		print_free_string(OUTPUT_XML, out, game->manufacturer);
		fprintf(out, SELECT(L1N, "</manufacturer>\n"));
	}

	print_game_bios(OUTPUT_XML, out, game);
	print_game_rom(OUTPUT_XML, out, game);
	print_game_sample(OUTPUT_XML, out, game);

	fprintf(out, SELECT(L1E, "\t</" XML_TOP ">\n"));
}

/* Import the driver object and print it as a resource */
#define PRINT_RESOURCE(format, s) \
	{ \
		extern struct GameDriver driver_##s; \
		print_resource_info(format, out, &driver_##s); \
	}

#endif

static void print_mame_data(int OUTPUT_XML, FILE* out, const struct GameDriver* games[])
{
	int j;

	/* print games */
	for(j=0;games[j];++j)
		print_game_info(OUTPUT_XML, out, games[j]);

	/* print the resources (only if linked) */
#if !defined(MESS) && !defined(TINY_COMPILE) && !defined(CPSMAME) && !defined(MMSND)
	PRINT_RESOURCE(OUTPUT_XML, neogeo);
#if !defined(NEOMAME)
	PRINT_RESOURCE(OUTPUT_XML, cvs);
	PRINT_RESOURCE(OUTPUT_XML, decocass);
	PRINT_RESOURCE(OUTPUT_XML, playch10);
	PRINT_RESOURCE(OUTPUT_XML, pgm);
	PRINT_RESOURCE(OUTPUT_XML, skns);
	PRINT_RESOURCE(OUTPUT_XML, stvbios);
	PRINT_RESOURCE(OUTPUT_XML, konamigx);
	PRINT_RESOURCE(OUTPUT_XML, nss);
	PRINT_RESOURCE(OUTPUT_XML, megatech);
	PRINT_RESOURCE(OUTPUT_XML, megaplay);
	PRINT_RESOURCE(OUTPUT_XML, cpzn1);
	PRINT_RESOURCE(OUTPUT_XML, cpzn2);
	PRINT_RESOURCE(OUTPUT_XML, tps);
	PRINT_RESOURCE(OUTPUT_XML, taitofx1);
#endif
#endif
}

/* Print the MAME database in XML format */
void print_mame_xml(FILE* out, const struct GameDriver* games[])
{
	fprintf(out,
		"<?xml version=\"1.0\"?>\n"
		"<!DOCTYPE " XML_ROOT " [\n"
		"<!ELEMENT " XML_ROOT " (" XML_TOP "+)>\n"
#ifdef MESS
		"\t<!ELEMENT " XML_TOP " (description, year?, manufacturer, history?, biosset*, rom*, disk*, sample*, chip*, video?, sound?, input?, dipswitch*, driver?, device*)>\n"
#else
		"\t<!ELEMENT " XML_TOP " (description, year?, manufacturer, history?, biosset*, rom*, disk*, sample*, chip*, video?, sound?, input?, dipswitch*, driver?)>\n"
#endif
		"\t\t<!ATTLIST " XML_TOP " name CDATA #REQUIRED>\n"
		"\t\t<!ATTLIST " XML_TOP " runnable (yes|no) \"yes\">\n"
		"\t\t<!ATTLIST " XML_TOP " cloneof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " romof CDATA #IMPLIED>\n"
		"\t\t<!ATTLIST " XML_TOP " sampleof CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT description (#PCDATA)>\n"
		"\t\t<!ELEMENT year (#PCDATA)>\n"
		"\t\t<!ELEMENT manufacturer (#PCDATA)>\n"
		"\t\t<!ELEMENT history (#PCDATA)>\n"
		"\t\t<!ELEMENT biosset EMPTY>\n"
		"\t\t\t<!ATTLIST biosset name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset description CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST biosset default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT rom EMPTY>\n"
		"\t\t\t<!ATTLIST rom name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom bios CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom size CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST rom crc CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom merge CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom offset CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST rom status (baddump|nodump|good) \"good\">\n"
		"\t\t\t<!ATTLIST rom dispose (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST rom soundonly (yes|no) \"no\">\n"
		"\t\t<!ELEMENT disk EMPTY>\n"
		"\t\t\t<!ATTLIST disk name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST disk md5 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk sha1 CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk region CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST disk index CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT sample EMPTY>\n"
		"\t\t\t<!ATTLIST sample name CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT chip EMPTY>\n"
		"\t\t\t<!ATTLIST chip name CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip type (cpu|audio) #REQUIRED>\n"
		"\t\t\t<!ATTLIST chip soundonly (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST chip clock CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT video EMPTY>\n"
		"\t\t\t<!ATTLIST video screen (raster|vector) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video orientation (vertical|horizontal) #REQUIRED>\n"
		"\t\t\t<!ATTLIST video width CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video height CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspectx CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video aspecty CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST video refresh CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT sound EMPTY>\n"
		"\t\t\t<!ATTLIST sound channels CDATA #REQUIRED>\n"
		"\t\t<!ELEMENT input EMPTY>\n"
		"\t\t\t<!ATTLIST input service (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input tilt (yes|no) \"no\">\n"
		"\t\t\t<!ATTLIST input players CDATA #REQUIRED>\n"
		"\t\t\t<!ATTLIST input control CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input buttons CDATA #IMPLIED>\n"
		"\t\t\t<!ATTLIST input coins CDATA #IMPLIED>\n"
		"\t\t<!ELEMENT dipswitch (dipvalue*)>\n"
		"\t\t\t<!ATTLIST dipswitch name CDATA #REQUIRED>\n"
		"\t\t\t<!ELEMENT dipvalue EMPTY>\n"
		"\t\t\t\t<!ATTLIST dipvalue name CDATA #REQUIRED>\n"
		"\t\t\t\t<!ATTLIST dipvalue default (yes|no) \"no\">\n"
		"\t\t<!ELEMENT driver EMPTY>\n"
		"\t\t\t<!ATTLIST driver status (good|preliminary|test) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver color (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver sound (good|imperfect|preliminary) #REQUIRED>\n"
		"\t\t\t<!ATTLIST driver palettesize CDATA #REQUIRED>\n"
#ifdef MESS
		"\t\t<!ELEMENT device (extension*)>\n"
		"\t\t\t<!ATTLIST device name CDATA #REQUIRED>\n"
		"\t\t\t<!ELEMENT extension EMPTY>\n"
		"\t\t\t\t<!ATTLIST extension name CDATA #REQUIRED>\n"
#endif
		"]>\n\n"
		"<" XML_ROOT ">\n"
	);

	print_mame_data(1, out, games);

	fprintf(out, "</" XML_ROOT ">\n");
}

/* Print the MAME database in INFO format */
void print_mame_info(FILE* out, const struct GameDriver* games[])
{
	print_mame_data(0, out, games);
}
