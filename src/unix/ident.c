#include "xmame.h"
#include "audit.h"
#include "unzip.h"
#include "driver.h"
#include <dirent.h>

#ifdef BSD43 /* old style directory handling */
#include <sys/types.h>
#include <sys/dir.h>
#define dirent direct
#endif

unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);
void romident(const char* name, int enter_dirs);

enum { KNOWN_START, KNOWN_ALL, KNOWN_NONE, KNOWN_SOME };

static int silentident = 0;
static int knownstatus = KNOWN_START;
static int ident = 0;

enum { IDENT_IDENT = 1, IDENT_ISKNOWN };

struct rc_option frontend_ident_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Rom Identification Related", NULL,	rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "ident",		"id",			rc_set_int,	&ident,
     NULL,		IDENT_IDENT,		0,		NULL,
     "Identify unknown romdump, or unknown romdumps in dir/zip" },
   { "isknown",		"ik",			rc_set_int,	&ident,
     NULL,		IDENT_ISKNOWN,		0,		NULL,
     "Check if romdump or romdumps in dir/zip are known"} ,
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};


/* Identifies a rom from from this checksum */
static void match_roms(const struct GameDriver *driver,const char* hash,int *found)
{
	const struct RomModule *region, *rom;

	for (region = rom_first_region(driver); region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			if (hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0))
			{
				char baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

				if (!silentident)
				{
					if (*found != 0)
						fprintf(stdout_file, "             ");
					fprintf(stdout_file, "= %s%-12s  %s\n",baddump ? "(BAD) " : "",ROM_GETNAME(rom),driver->description);
				}
				(*found)++;
			}
		}
	}
}

void identify_rom(const char* name, const char* hash, int length)
{
	int found = 0;

	/* remove directory name */
	int i;
	for (i = strlen(name)-1;i >= 0;i--)
	{
		if (name[i] == '/' || name[i] == '\\')
		{
			i++;
			break;
		}
	}
	if (!silentident)
		fprintf(stdout_file, "%s ",&name[0]);

	for (i = 0; drivers[i]; i++)
		match_roms(drivers[i],hash,&found);

	for (i = 0; test_drivers[i]; i++)
		match_roms(test_drivers[i],hash,&found);

	if (found == 0)
	{
		unsigned size = length;
		while (size && (size & 1) == 0) size >>= 1;
		if (size & ~1)
		{
			if (!silentident)
				fprintf(stdout_file, "NOT A ROM\n");
		}
		else
		{
			if (!silentident)
				fprintf(stdout_file, "NO MATCH\n");
			if (knownstatus == KNOWN_START)
				knownstatus = KNOWN_NONE;
			else if (knownstatus == KNOWN_ALL)
				knownstatus = KNOWN_SOME;
		}
	}
	else
	{
		if (knownstatus == KNOWN_START)
			knownstatus = KNOWN_ALL;
		else if (knownstatus == KNOWN_NONE)
			knownstatus = KNOWN_SOME;
	}
}

/* Identifies a file from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	unsigned char* data;
	char hash[HASH_BUF_SIZE];

	f = fopen(name,"rb");
	if (!f) {
		return;
	}

	/* determine length of file */
	if (fseek (f, 0L, SEEK_END)!=0)	{
		fclose(f);
		return;
	}

	length = ftell(f);
	if (length == -1L) {
		fclose(f);
		return;
	}

	/* empty file */
	if (!length) {
		fclose(f);
		return;
	}

	/* allocate space for entire file */
	data = (unsigned char*)malloc(length);
	if (!data) {
		fclose(f);
		return;
	}

	if (fseek (f, 0L, SEEK_SET)!=0) {
		free(data);
		fclose(f);
		return;
	}

	if (fread(data, 1, length, f) != length) {
		free(data);
		fclose(f);
		return;
	}

	fclose(f);

	/* Compute checksum of all the available functions. Since MAME for
	   now carries inforamtions only for CRC and SHA1, we compute only
	   these */
	if (options.crc_only)
		hash_compute(hash, data, length, HASH_CRC);
	else
		hash_compute(hash, data, length, HASH_CRC|HASH_SHA1);
	
	/* Try to identify the ROM */
	identify_rom(name, hash, length);

	free(data);
}

void identify_zip(const char* zipname)
{
	struct zipent* ent;

	ZIP* zip = openzip( FILETYPE_RAW, 0, zipname );
	if (!zip)
		return;

	while ((ent = readzip(zip))) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			char hash[HASH_BUF_SIZE];
			UINT8 crcs[4];

/*			sprintf(buf,"%s/%s",zipname,ent->name); */
			sprintf(buf,"%-12s",ent->name);

			/* Decompress the ROM from the ZIP, and compute all the needed 
			   checksums. Since MAME for now carries informations only for CRC and
			   SHA1, we compute only these (actually, CRC is extracted from the
			   ZIP header) */
			hash_data_clear(hash);

			if (!options.crc_only)
			{
				UINT8* data =  (UINT8*)malloc(ent->uncompressed_size);
				readuncompresszip(zip, ent, (char *)data);
				hash_compute(hash, data, ent->uncompressed_size, HASH_SHA1);
				free(data);
			}
			
			crcs[0] = (UINT8)(ent->crc32 >> 24);
			crcs[1] = (UINT8)(ent->crc32 >> 16);
			crcs[2] = (UINT8)(ent->crc32 >> 8);
			crcs[3] = (UINT8)(ent->crc32 >> 0);
			hash_data_insert_binary_checksum(hash, HASH_CRC, crcs);

			/* Try to identify the ROM */
			identify_rom(buf, hash, ent->uncompressed_size);

			free(buf);
		}
	}

	closezip(zip);
}

void identify_dir(const char* dirname)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir(dirname);
	if (!dir) {
		return;
	}

	ent = readdir(dir);
	while (ent) {
		/* Skip special files */
		if (ent->d_name[0]!='.') {
			char* buf = (char*)malloc(strlen(dirname)+1+strlen(ent->d_name)+1);
			sprintf(buf,"%s/%s",dirname,ent->d_name);
			romident(buf,0);
			free(buf);
		}

		ent = readdir(dir);
	}
	closedir(dir);
}

void romident(const char* name,int enter_dirs)
{
	struct stat s;

	if (stat(name,&s) != 0)	{
		fprintf(stdout_file, "%s: %s\n",name,strerror(errno));
		return;
	}

	if (S_ISDIR(s.st_mode)) {
		if (enter_dirs)
			identify_dir(name);
	} else {
		unsigned l = strlen(name);
		if (l>=4 && stricmp(name+l-4,".zip")==0)
			identify_zip(name);
		else
			identify_file(name);
		return;
	}
}

int frontend_ident(char *gamename)
{
	if (!ident)
		return 1234;

	if (!gamename)
	{
		fprintf(stderr_file, "-ident / -isknown requires a game- or filename as second argument\n");
		return OSD_NOT_OK;
	}

	if (ident == IDENT_ISKNOWN)
		silentident = 1;

	romident(gamename, 1);

	if (ident == IDENT_ISKNOWN)
	{
		switch (knownstatus)
		{
			case KNOWN_START: fprintf(stdout_file, "ERROR     %s\n",gamename); break;
			case KNOWN_ALL:   fprintf(stdout_file, "KNOWN     %s\n",gamename); break;
			case KNOWN_NONE:  fprintf(stdout_file, "UNKNOWN   %s\n",gamename); break;
			case KNOWN_SOME:  fprintf(stdout_file, "PARTKNOWN %s\n",gamename); break;
		}
	}
	return OSD_OK;
}
