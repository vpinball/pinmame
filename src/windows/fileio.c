//============================================================
//
//	fileio.c - Win32 file access functions
//
//============================================================

#include <sys/stat.h>
#include <signal.h>
#include "driver.h"
#include "unzip.h"
#include "zlib/zlib.h"
#include "rc.h"

#ifdef MESS
#include "mess/msdos.h"
#endif


//============================================================
//	EXTERNALS
//============================================================

extern char *rompath_extra;

// from datafile.c
extern const char *history_filename;
extern const char *mameinfo_filename;

// from cheat.c
extern char *cheatfile;



//============================================================
//	DEBUGGING
//============================================================

// Verbose outputs to error.log ?
#define VERBOSE 					0

// Support ZipMagic/ZipFolders?
#define SUPPORT_AUTOZIP_UTILITIES	0

// enable lots of logging
#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



//============================================================
//	CONSTANTS
//============================================================

#define PLAIN_FILE				0
#define RAM_FILE				1
#define ZIPPED_FILE				2
#define UNLOADED_ZIPPED_FILE	3

#define FILEFLAG_OPENREAD		0x01
#define FILEFLAG_OPENWRITE		0x02
#define FILEFLAG_CRC			0x04
#define FILEFLAG_REVERSE_SEARCH	0x08
#define FILEFLAG_VERIFY_ONLY	0x10

#define STATCACHE_SIZE			64



//============================================================
//	TYPE DEFINITIONS
//============================================================

struct fake_file_handle
{
	FILE *file;
	UINT8 *data;
	UINT32 offset;
	UINT32 length;
	UINT8 type;
	UINT32 crc;
};


struct stat_cache_entry
{
	struct stat stat_buffer;
	int result;
	char *file;
};



//============================================================
//	GLOBAL VARIABLES
//============================================================

static const char **rompathv = NULL;
static int rompathc = 0;
static int rompath_needs_decomposition = 1;

static const char **samplepathv = NULL;
static int samplepathc = 0;
static int samplepath_needs_decomposition = 1;

static const char *rompath;
static const char *samplepath;
static const char *cfgdir, *nvdir, *hidir, *inpdir, *stadir, *diffdir;
static const char *memcarddir, *artworkdir, *screenshotdir, *cheatdir;

static struct stat_cache_entry *stat_cache[STATCACHE_SIZE];
static struct stat_cache_entry stat_cache_entry[STATCACHE_SIZE];



//============================================================
//	PROTOTYPES
//============================================================

static int cache_stat(const char *path, struct stat *statbuf);
static void flush_cache(void);

static void *generic_fopen(int pathc, const char **pathv, const char *gamename, const char *filename, const char *extension, UINT32 crc, UINT32 flags);
static int get_pathlist_for_filetype(int filetype, const char ***pathlist, const char **extension);
static int checksum_file(const char *file, UINT8 **p, unsigned int *size, unsigned int *crc);

static int request_decompose_rompath(struct rc_option *option, const char *arg, int priority);
static int request_decompose_samplepath(struct rc_option *option, const char *arg, int priority);



//============================================================
//	FILE PATH OPTIONS
//============================================================

struct rc_option fileio_opts[] =
{
	// name, shortname, type, dest, deflt, min, max, func, help
	{ "Windows path and directory options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "rompath", "rp", rc_string, &rompath, "roms", 0, 0, request_decompose_rompath, "path to romsets" },
	{ "samplepath", "sp", rc_string, &samplepath, "samples", 0, 0, request_decompose_samplepath, "path to samplesets" },
	{ "cfg_directory", NULL, rc_string, &cfgdir, "cfg", 0, 0, NULL, "directory to save configurations" },
	{ "nvram_directory", NULL, rc_string, &nvdir, "nvram", 0, 0, NULL, "directory to save nvram contents" },
	{ "memcard_directory", NULL, rc_string, &memcarddir, "memcard", 0, 0, NULL, "directory to save memory card contents" },
	{ "input_directory", NULL, rc_string, &inpdir, "inp", 0, 0, NULL, "directory to save input device logs" },
	{ "hiscore_directory", NULL, rc_string, &hidir, "hi", 0, 0, NULL, "directory to save hiscores" },
	{ "state_directory", NULL, rc_string, &stadir, "sta", 0, 0, NULL, "directory to save states" },
	{ "artwork_directory", NULL, rc_string, &artworkdir, "artwork", 0, 0, NULL, "directory for Artwork (Overlays etc.)" },
	{ "snapshot_directory", NULL, rc_string, &screenshotdir, "snap", 0, 0, NULL, "directory for screenshots (.png format)" },
	{ "diff_directory", NULL, rc_string, &diffdir, "diff", 0, 0, NULL, "directory for hard drive image difference files" },
	{ "cheat_file", NULL, rc_string, &cheatfile, "cheat.dat", 0, 0, NULL, "cheat filename" },
	{ "history_file", NULL, rc_string, &history_filename, "history.dat", 0, 0, NULL, NULL },
	{ "mameinfo_file", NULL, rc_string, &mameinfo_filename, "mameinfo.dat", 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



//============================================================
//	osd_fopen
//============================================================

void *osd_fopen(const char *gamename, const char *filename, int filetype, int openforwrite)
{
	const char **pathv;
	const char *extension;
	int pathc = get_pathlist_for_filetype(filetype, &pathv, &extension);

	// first verify that we aren't trying to open read-only types as writeables
	switch (filetype)
	{
		// read-only cases
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_ROM_NOCRC:
		case OSD_FILETYPE_IMAGE_R:
		case OSD_FILETYPE_SAMPLE:
		case OSD_FILETYPE_HIGHSCORE_DB:
		case OSD_FILETYPE_ARTWORK:
		case OSD_FILETYPE_HISTORY:
		case OSD_FILETYPE_LANGUAGE:
			if (openforwrite)
			{
				logerror("osd_fopen: type %02x write not supported\n", filetype);
				return NULL;
			}
			break;

		// write-only cases
		case OSD_FILETYPE_SCREENSHOT:
			if (!openforwrite)
			{
				logerror("osd_fopen: type %02x read not supported\n", filetype);
				return NULL;
			}
			break;
	}

	// if we're opening for write, invalidate the cache
	if (openforwrite)
		flush_cache();

	// now open the file appropriately
	switch (filetype)
	{
		// ROM files
		case OSD_FILETYPE_ROM:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD | FILEFLAG_CRC);

		// ROM files with no CRC
		case OSD_FILETYPE_ROM_NOCRC:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD);

		// read-only disk images
		case OSD_FILETYPE_IMAGE_R:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD);

		// read/write disk images (MESS only)
		case OSD_FILETYPE_IMAGE_RW:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE | FILEFLAG_REVERSE_SEARCH);

		// differencing disk images
		case OSD_FILETYPE_IMAGE_DIFF:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE);

		// samples
		case OSD_FILETYPE_SAMPLE:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD);

		// artwork files
		case OSD_FILETYPE_ARTWORK:
			return generic_fopen(pathc, pathv, gamename, filename, extension, 0, FILEFLAG_OPENREAD);

		// NVRAM files
		case OSD_FILETYPE_NVRAM:
			return generic_fopen(pathc, pathv, NULL, gamename, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		// high score files
		case OSD_FILETYPE_HIGHSCORE:
			if (!mame_highscore_enabled())
				return NULL;
			return generic_fopen(pathc, pathv, NULL, gamename, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		// highscore database
		case OSD_FILETYPE_HIGHSCORE_DB:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, FILEFLAG_OPENREAD);

		// config files
		case OSD_FILETYPE_CONFIG:
			return generic_fopen(pathc, pathv, NULL, gamename, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		// input logs
		case OSD_FILETYPE_INPUTLOG:
			return generic_fopen(pathc, pathv, NULL, gamename, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		// save state files
		case OSD_FILETYPE_STATE:
		{
			char temp[256];
			sprintf(temp, "%s-%s", gamename, filename);
			return generic_fopen(pathc, pathv, NULL, temp, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);
		}

		// memory card files
		case OSD_FILETYPE_MEMCARD:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		// screenshot files
		case OSD_FILETYPE_SCREENSHOT:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, FILEFLAG_OPENWRITE);

		// history files
		case OSD_FILETYPE_HISTORY:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, FILEFLAG_OPENREAD);

		// cheat file
		case OSD_FILETYPE_CHEAT:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, FILEFLAG_OPENREAD | (openforwrite ? FILEFLAG_OPENWRITE : 0));

		// language file
		case OSD_FILETYPE_LANGUAGE:
			return generic_fopen(pathc, pathv, NULL, filename, extension, 0, FILEFLAG_OPENREAD);

		// anything else
		default:
			logerror("osd_fopen(): unknown filetype %02x\n", filetype);
			return NULL;
	}
	return NULL;
}



//============================================================
//	osd_fclose
//============================================================

void osd_fclose(void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			fclose (f->file);
			break;

		case ZIPPED_FILE:
		case RAM_FILE:
			if (f->data)
				free(f->data);
			break;
	}

	// free the file data
	free(f);
}



//============================================================
//	osd_faccess
//============================================================

int osd_faccess(const char *filename, int filetype)
{
	const char **pathv;
	const char *extension;
	int pathc = get_pathlist_for_filetype(filetype, &pathv, &extension);
	char modified_filename[256];
	int path_index;

	// copy the filename and add an extension
	strcpy(modified_filename, filename);
	if (extension)
	{
		char *p = strchr(modified_filename, '.');
		if (p)
			strcpy(p, extension);
		else
		{
			strcat(modified_filename, ".");
			strcat(modified_filename, extension);
		}
	}

	// loop over all paths
	for (path_index = 0; path_index < pathc; path_index++)
	{
		const char *dir_name = pathv[path_index];
		if (dir_name)
		{
			struct stat stat_buffer;
			char name[256];

			// does such a directory (or file) exist?
			sprintf(name, "%s/%s", dir_name, modified_filename);
			LOG(("osd_faccess: trying %s\n", name));
			if (cache_stat(name, &stat_buffer) == 0)
				return 1;

			// try again with a .zip extension
			sprintf(name, "%s/%s.zip", dir_name, modified_filename);
			LOG(("osd_faccess: trying %s\n", name));
			if (cache_stat(name, &stat_buffer) == 0)
				return 1;

#if SUPPORT_AUTOZIP_UTILITIES
			// try again with a .zif extension
			sprintf(name, "%s/%s.zif", dir_name, modified_filename);
			LOG(("osd_faccess: trying %s\n", name));
			if (cache_stat(name, &stat_buffer) == 0)
				return 1;
#endif
		}
	}

	// no match
	return 0;
}



//============================================================
//	osd_fread
//============================================================

int osd_fread(void *file, void *buffer, int length)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return fread(buffer, 1, length, f->file);

		case ZIPPED_FILE:
		case RAM_FILE:
			if (f->data)
			{
				if (f->offset + length > f->length)
					length = f->length - f->offset;
				memcpy(buffer, f->data + f->offset, length);
				f->offset += length;
				return length;
			}
			break;
	}

	return 0;
}



//============================================================
//	osd_fwrite
//============================================================

int osd_fwrite(void *file, const void *buffer, int length)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return fwrite(buffer, 1, length, f->file);
	}

	return 0;
}



//============================================================
//	osd_fseek
//============================================================

int osd_fseek(void *file, int offset, int whence)
{
	struct fake_file_handle *f = file;
	int err = 0;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return fseek(f->file, offset, whence);

		case ZIPPED_FILE:
		case RAM_FILE:
			switch (whence)
			{
				case SEEK_SET:
					f->offset = offset;
					break;
				case SEEK_CUR:
					f->offset += offset;
					break;
				case SEEK_END:
					f->offset = f->length + offset;
					break;
			}
			break;
	}

	return err;
}



//============================================================
//	osd_fchecksum
//============================================================

int osd_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum)
{
	struct fake_file_handle *f;
	const char **pathv;
	const char *extension;
	int pathc = get_pathlist_for_filetype(OSD_FILETYPE_ROM, &pathv, &extension);

	// first open the file; we get the CRC for free
	f = generic_fopen(pathc, pathv, gamename, filename, NULL, *sum, FILEFLAG_OPENREAD | FILEFLAG_CRC | FILEFLAG_VERIFY_ONLY);

	// if we didn't succeed return -1
	if (!f)
		return -1;

	// close the file and save the length & sum
	*sum = f->crc;
	*length = f->length;
	osd_fclose(f);
	return 0;
}



//============================================================
//	osd_fsize
//============================================================

int osd_fsize(void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
		{
			int size, offs;
			offs = ftell(f->file);
			fseek(f->file, 0, SEEK_END);
			size = ftell(f->file);
			fseek(f->file, offs, SEEK_SET);
			return size;
		}

		case RAM_FILE:
		case ZIPPED_FILE:
			return f->length;
	}

	return 0;
}



//============================================================
//	osd_fcrc
//============================================================

unsigned int osd_fcrc(void *file)
{
	struct fake_file_handle *f = file;
	return f->crc;
}



//============================================================
//	osd_fgetc
//============================================================

int osd_fgetc(void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return fgetc(f->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			if (f->offset < f->length)
				return f->data[f->offset++];
			return EOF;
	}
	return EOF;
}



//============================================================
//	osd_ungetc
//============================================================

int osd_ungetc(int c, void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return ungetc(c, f->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			if (f->offset > 0)
			{
				f->offset--;
				return c;
			}
			return EOF;
	}
	return EOF;
}



//============================================================
//	osd_fgets
//============================================================

char *osd_fgets(char *s, int n, void *file)
{
	char *cur = s;

	// loop while we have characters
	while (n > 0)
	{
		int c = osd_fgetc(file);
		if (c == EOF)
			break;

		// if there's a CR, look for an LF afterwards
		if (c == 0x0d)
		{
			int c2 = osd_fgetc(file);
			if (c2 != 0x0a)
				osd_ungetc(c2, file);
			*cur++ = 0x0d;
			n--;
			break;
		}

		// if there's an LF, reinterp as a CR for consistency
		else if (c == 0x0a)
		{
			*cur++ = 0x0d;
			n--;
			break;
		}

		// otherwise, pop the character in and continue
		*cur++ = c;
		n--;
	}

	// if we put nothing in, return NULL
	if (cur == s)
		return NULL;

	// otherwise, terminate
	if (n > 0)
		*cur++ = 0;
	return s;
}



//============================================================
//	osd_feof
//============================================================

int osd_feof(void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return feof(f->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return (f->offset >= f->length);
	}

	return 1;
}



//============================================================
//	osd_ftell
//============================================================

int osd_ftell(void *file)
{
	struct fake_file_handle *f = file;

	// switch off the file type
	switch (f->type)
	{
		case PLAIN_FILE:
			return ftell(f->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return f->offset;
	}

	return -1L;
}



//============================================================
//	osd_fread_swap
//============================================================

int osd_fread_swap(void *file, void *buffer, int length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	// standard read first
	res = osd_fread(file, buffer, length);

	// swap the result
	buf = buffer;
	for (i = 0; i < res; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}



//============================================================
//	osd_fwrite_swap
//============================================================

int osd_fwrite_swap(void *file, const void *buffer, int length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	// swap the data first
	buf = (UINT8 *)buffer;
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	// do the write
	res = osd_fwrite(file, buffer, length);

	// swap the data back
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}



//============================================================
//	osd_basename
//============================================================

char *osd_basename(char *filename)
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
//	osd_dirname
//============================================================

char *osd_dirname(char *filename)
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
		fprintf(stderr, "error: malloc failed in osd_dirname\n");
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
//	osd_strip_extension
//============================================================

char *osd_strip_extension(char *filename)
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
		fprintf(stderr, "error: malloc failed in osd_newname\n");
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


#ifndef WINUI
//============================================================
//	osd_display_loading_rom_message
//============================================================

// called while loading ROMs. It is called a last time with name == 0 to signal
// that the ROM loading process is finished.
// return non-zero to abort loading
int osd_display_loading_rom_message(const char *name, int current, int total)
{
	if (name)
		fprintf(stdout, "loading %-12s\r", name);
	else
		fprintf(stdout, "                    \r");
	fflush(stdout);

	return 0;
}
#endif


//============================================================
//	cache_stat
//============================================================

static int cache_stat(const char *path, struct stat *statbuf)
{
	struct stat_cache_entry *entry;
	int i;

	// if not initialized yet, do it
	if (!stat_cache[0])
		for (i = 0; i < STATCACHE_SIZE; i++)
			stat_cache[i] = &stat_cache_entry[i];

	// search in the cache for a matching filename
	for (i = 0; i < STATCACHE_SIZE; i++)
		if (stat_cache[i]->file && strcmp(stat_cache[i]->file, path) == 0)
		{
			// move this entry to the head of the list
			entry = stat_cache[i];
			if (i != 0)
				memmove(&stat_cache[1], &stat_cache[0], i * sizeof(stat_cache[0]));
			stat_cache[0] = entry;

			// return the cached result
			if (entry->result == 0)
				*statbuf = entry->stat_buffer;
			return entry->result;
		}

	// kill off the oldest entry
	entry = stat_cache[STATCACHE_SIZE - 1];
	free(entry->file);

	// move everyone else up
	memmove(&stat_cache[1], &stat_cache[0], (STATCACHE_SIZE - 1) * sizeof(stat_cache[0]));

	// set the first entry
	stat_cache[0] = entry;

	// file
	entry->file = malloc(strlen(path) + 1);
	if (entry->file)
		strcpy(entry->file, path);

	// result and stat
	entry->result = stat(path, &entry->stat_buffer);
	if (entry->result == 0)
		*statbuf = entry->stat_buffer;
	return entry->result;
}



//============================================================
//	flush_cache
//============================================================

static void flush_cache(void)
{
	int i;

	// if not initialized yet, do it
	if (!stat_cache[0])
		for (i = 0; i < STATCACHE_SIZE; i++)
			stat_cache[i] = &stat_cache_entry[i];

	// search in the cache for a matching filename
	for (i = 0; i < STATCACHE_SIZE; i++)
		if (stat_cache[i]->file)
		{
			free(stat_cache[i]->file);
			stat_cache[i]->file = NULL;
		}
}



//============================================================
//	decompose_path
//============================================================

static void decompose_path(const char *pathlist, const char *extrapath, int *pathcount, const char ***patharray)
{
	char **pathv = NULL;
	const char *token;

	// log the basic info
	LOG(("decomposing path\n"));
	if (extrapath)
		LOG(("  extrapath = %s\n", extrapath));
	LOG(("  pathlist = %s\n", pathlist));

	// free any existing paths
	if (*pathcount != 0)
	{
		int path_index;

		for (path_index = 0; path_index < *pathcount; path_index++)
			free((void *)(*patharray)[path_index]);
		free(*patharray);
		*pathcount = 0;
	}

	// by default, allocate one entry of the pathlist to start
	pathv = malloc(sizeof(char *));
	if (!pathv)
		goto out_of_memory;

	// if we have an extra path, add that to the list first
	if (extrapath)
	{
		// allocate space for this path
		pathv[*pathcount] = malloc(strlen(extrapath) + 1);
		if (!pathv[*pathcount])
			goto out_of_memory;

		// copy it in
		strcpy(pathv[*pathcount], extrapath);
		*pathcount += 1;
	}

	// look for separators
	token = strchr(pathlist, ';');
	if (!token) token = pathlist + strlen(pathlist);

	// loop until done
	while (1)
	{
		// allocate space for the new pointer
		pathv = realloc(pathv, (*pathcount + 1) * sizeof(char *));
		if (!pathv)
			goto out_of_memory;

		// allocate space for the path
		pathv[*pathcount] = malloc(token - pathlist + 1);
		if (!pathv[*pathcount])
			goto out_of_memory;

		// copy it in
		memcpy(pathv[*pathcount], pathlist, token - pathlist);
		pathv[*pathcount][token - pathlist] = 0;
		*pathcount += 1;

		// if this was the end, break
		if (*token == 0)
			break;
		pathlist = token + 1;

		// find the next separator
		token = strchr(pathlist, ';');
		if (!token) token = pathlist + strlen(pathlist);
	}

	*patharray = (const char **)pathv;
	return;

out_of_memory:
	fprintf(stderr, "Out of memory!\n");
	exit(1);
}



//============================================================
//	request_decompose_rompath
//============================================================

static int request_decompose_rompath(struct rc_option *option, const char *arg, int priority)
{
	rompath_needs_decomposition = 1;
	option->priority = priority;
	return 0;
}



//============================================================
//	request_decompose_samplepath
//============================================================

static int request_decompose_samplepath(struct rc_option *option, const char *arg, int priority)
{
	samplepath_needs_decomposition = 1;
	option->priority = priority;
	return 0;
}



//============================================================
//	decompose_paths_if_needed
//============================================================

static void decompose_paths_if_needed(void)
{
	if (rompath_needs_decomposition)
	{
		decompose_path(rompath, rompath_extra, &rompathc, &rompathv);
		rompath_needs_decomposition = 0;
	}

	if (samplepath_needs_decomposition)
	{
		decompose_path(samplepath, NULL, &samplepathc, &samplepathv);
		samplepath_needs_decomposition = 0;
	}
}



//============================================================
//	compose_path
//============================================================

INLINE void compose_path(char *output, const char *path, const char *gamename, const char *filename, const char *extension)
{
	char *filename_base = output;
	*output = 0;

	// if there's a path, add that with a trailing '/'
	if (path)
	{
		strcat(output, path);
		if (gamename || filename)
		{
			strcat(output, "/");
			filename_base = &output[strlen(output)];
		}
	}

	// if there's a gamename, add that; only add a '/' if there is a filename as well
	if (gamename)
	{
		strcat(output, gamename);
		if (filename)
		{
			strcat(output, "/");
			filename_base = &output[strlen(output)];
		}
	}

	// if there's a filename, add that
	if (filename)
		strcat(output, filename);

	// if there's no extension in the filename, add the extension
	if (extension && !strchr(filename_base, '.'))
	{
		strcat(output, ".");
		strcat(output, extension);
	}
}



//============================================================
//	get_pathlist_for_filetype
//============================================================

static int get_pathlist_for_filetype(int filetype, const char ***pathlist, const char **extension)
{
	static const char *null_pathv = NULL;

	// update path info
	decompose_paths_if_needed();

	// now open the file appropriately
	switch (filetype)
	{
		// ROM files and drive images
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_ROM_NOCRC:
			*pathlist = (const char **)rompathv;
			*extension = NULL;
			return rompathc;

		case OSD_FILETYPE_IMAGE_R:
		case OSD_FILETYPE_IMAGE_RW:
			*pathlist = (const char **)rompathv;
			*extension = "chd";
			return rompathc;

		// differencing drive images
		case OSD_FILETYPE_IMAGE_DIFF:
			*pathlist = &diffdir;
			*extension = "dif";
			return 1;

		// samples
		case OSD_FILETYPE_SAMPLE:
			*pathlist = (const char **)samplepathv;
			*extension = "wav";
			return samplepathc;

		// artwork files
		case OSD_FILETYPE_ARTWORK:
			*pathlist = &artworkdir;
			*extension = "png";
			return 1;

		// NVRAM files
		case OSD_FILETYPE_NVRAM:
			*pathlist = &nvdir;
			*extension = "nv";
			return 1;

		// high score files
		case OSD_FILETYPE_HIGHSCORE:
			*pathlist = &hidir;
			*extension = "hi";
			return 1;

		// highscore database/history files
		case OSD_FILETYPE_HIGHSCORE_DB:
		case OSD_FILETYPE_HISTORY:
			*pathlist = &null_pathv;
			*extension = NULL;
			return 1;

		// language files
		case OSD_FILETYPE_LANGUAGE:
			*pathlist = &null_pathv;
			*extension = "lng";
			return 1;

		// config files
		case OSD_FILETYPE_CONFIG:
			*pathlist = &cfgdir;
			*extension = "cfg";
			return 1;

		// input logs
		case OSD_FILETYPE_INPUTLOG:
			*pathlist = &inpdir;
			*extension = "inp";
			return 1;

		// save state files
		case OSD_FILETYPE_STATE:
			*pathlist = &stadir;
			*extension = "sta";
			return 1;

		// memory card files
		case OSD_FILETYPE_MEMCARD:
			*pathlist = &memcarddir;
			*extension = "mem";
			return 1;

		// screenshot files
		case OSD_FILETYPE_SCREENSHOT:
			*pathlist = &screenshotdir;
			*extension = "png";
			return 1;

		// cheat file
		case OSD_FILETYPE_CHEAT:
			*pathlist = &cheatdir;
			*extension = NULL;
			return 1;

		// anything else
		default:
			logerror("osd_fopen(): unknown filetype %02x\n", filetype);
			break;
	}

	// default to just looking the MAME directory
	*pathlist = &null_pathv;
	*extension = NULL;
	return 1;
}



//============================================================
//	generic_fopen
//============================================================

static void *generic_fopen(int pathc, const char **pathv, const char *gamename, const char *filename, const char *extension, UINT32 crc, UINT32 flags)
{
	static const char *access_modes[] = { "rb", "rb", "wb", "r+b" };
	int path_index, path_start, path_stop, path_inc;
	struct fake_file_handle file, *newfile;
	struct stat stat_buffer;
	char tempname[256];

	LOG(("generic_fopen(%d, %s, %s, %s, %X)\n", pathc, gamename, filename, extension, flags));

	// reset the file handle
	memset(&file, 0, sizeof(file));

	// check for incompatible flags
	if ((flags & FILEFLAG_OPENWRITE) && (flags & FILEFLAG_CRC))
		fprintf(stderr, "Can't use CRC option with WRITE option in generic_fopen!\n");

	// determine start/stop based on reverse search flag
	if (!(flags & FILEFLAG_REVERSE_SEARCH))
	{
		path_start = 0;
		path_stop = pathc;
		path_inc = 1;
	}
	else
	{
		path_start = pathc - 1;
		path_stop = -1;
		path_inc = -1;
	}

	// loop over paths
	for (path_index = path_start; path_index != path_stop; path_index += path_inc)
	{
		const char *dir_name = pathv[path_index];
		char name[1024];

		// ----------------- STEP 1: OPEN THE FILE RAW --------------------

		// first look for path/gamename as a directory
		compose_path(name, dir_name, gamename, NULL, NULL);
		LOG(("Trying %s\n", name));

		// if the directory exists, proceed
		if (*name == 0 || (cache_stat(name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR)))
		{
			// now look for path/gamename/filename.ext
			compose_path(name, dir_name, gamename, filename, extension);

			// if we need a CRC, load it into RAM and compute it along the way
			if (flags & FILEFLAG_CRC)
			{
				if (checksum_file(name, &file.data, &file.length, &file.crc) == 0)
				{
					file.type = RAM_FILE;
					break;
				}
			}

			// otherwise, just open it straight
			else
			{
				file.type = PLAIN_FILE;
				file.file = fopen(name, access_modes[flags & 3]);
				if (file.file == NULL && (flags & 3) == 3)
					file.file = fopen(name, "w+b");
				if (file.file != NULL)
					break;
			}
		}

		// ----------------- STEP 2: OPEN THE FILE IN A ZIP --------------------

		// now look for it within a ZIP file
		if (!(flags & FILEFLAG_OPENWRITE))
		{
			// first look for path/gamename.zip
			compose_path(name, dir_name, gamename, NULL, "zip");
			LOG(("Trying %s file\n", name));

			// if the ZIP file exists, proceed
			if (cache_stat(name, &stat_buffer) == 0 && !(stat_buffer.st_mode & S_IFDIR))
			{
				// if the file was able to be extracted from the ZIP, continue
				compose_path(tempname, NULL, NULL, filename, extension);

				// verify-only case
				if (flags & FILEFLAG_VERIFY_ONLY)
				{
					file.crc = crc;
					if (checksum_zipped_file(name, tempname, &file.length, &file.crc) == 0)
					{
						file.type = UNLOADED_ZIPPED_FILE;
						break;
					}
				}

				// full load case
				else
				{
					if (load_zipped_file(name, tempname, &file.data, &file.length) == 0)
					{
						LOG(("Using (osd_fopen) zip file for %s\n", filename));
						file.type = ZIPPED_FILE;
						file.crc = crc32(0L, file.data, file.length);
						break;
					}
				}
			}
		}

#if SUPPORT_AUTOZIP_UTILITIES
		// ----------------- STEP 3: OPEN THE FILE USING ZIPMAGIC --------------------

		// first look for path/gamename.zip as a directory
		compose_path(name, dir_name, gamename, NULL, "zip");
		LOG(("Trying %s directory\n", name));

		// if the ZIP directory exists, load the file
		if (cache_stat(name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR))
		{
			// now look for path/gamename.zip/filename
			strcpy(tempname, name);
			compose_path(name, tempname, NULL, filename, extension);

			// if we need a CRC, load it into RAM and compute it along the way
			if (flags & FILEFLAG_CRC)
			{
				if (checksum_file(name, &file.data, &file.length, &file.crc) == 0)
				{
					file.type = RAM_FILE;
					break;
				}
			}

			// otherwise, just open it straight
			else
			{
				file.type = PLAIN_FILE;
				file.file = fopen(name, access_modes[flags & 3]);
				if (file.file == NULL && (flags & 3) == 3)
					file.file = fopen(name, "w+b");
				if (file.file != NULL);
					break;
			}
		}

		// ----------------- STEP 4: OPEN THE FILE USING ZIPFOLDERS --------------------

		// first look for path/gamename.zif as a directory
		compose_path(name, dir_name, gamename, NULL, "zif");
		LOG(("Trying %s directory\n", name));

		// if the ZIP directory exists, load the file
		if (cache_stat(name, &stat_buffer) == 0 && (stat_buffer.st_mode & S_IFDIR))
		{
			// now look for path/gamename.zif/filename
			strcpy(tempname, name);
			compose_path(name, tempname, NULL, filename, extension);

			// if we need a CRC, load it into RAM and compute it along the way
			if (flags & FILEFLAG_CRC)
			{
				if (checksum_file(name, &file.data, &file.length, &file.crc) == 0)
				{
					file.type = RAM_FILE;
					break;
				}
			}

			// otherwise, just open it straight
			else
			{
				file.type = PLAIN_FILE;
				file.file = fopen(name, access_modes[flags & 3]);
				if (file.file == NULL && (flags & 3) == 3)
					file.file = fopen(name, "w+b");
				if (file.file != NULL);
					break;
			}
		}
#endif
	}

	// if we didn't succeed, just return NULL
	if (path_index == path_stop)
		return NULL;

	// otherwise, duplicate the file
	newfile = malloc(sizeof(file));
	if (newfile)
		*newfile = file;

	return newfile;
}



//============================================================
//	checksum_file
//============================================================

static int checksum_file(const char *file, UINT8 **p, unsigned int *size, unsigned int *crc)
{
	UINT8 *data;
	int length;
	FILE *f;

	// open the file
	f = fopen(file, "rb");
	if (!f)
		return -1;

	// determine length of file
	if (fseek(f, 0L, SEEK_END) != 0)
	{
		fclose(f);
		return -1;
	}

	length = ftell(f);
	if (length == -1L)
	{
		fclose(f);
		return -1;
	}

	// allocate space for entire file
	data = malloc(length);
	if (!data)
	{
		fclose(f);
		return -1;
	}

	// read entire file into memory
	if (fseek(f, 0L, SEEK_SET) != 0)
	{
		free(data);
		fclose(f);
		return -1;
	}

	if (fread(data, sizeof (UINT8), length, f) != length)
	{
		free(data);
		fclose(f);
		return -1;
	}

	// compute the CRC
	*size = length;
	*crc = crc32(0L, data, length);

	// if the caller wants the data, give it away, otherwise free it
	if (p)
		*p = data;
	else
		free(data);

	// close the file
	fclose(f);
	return 0;
}
