/*============================================================ */
/* */
/*	fileio.c - Unix file access functions */
/* */
/*============================================================ */

#include <stdarg.h>
#include "xmame.h"
#include "osdutils.h"
#include "unzip.h"
#ifdef MESS
#include "image.h"
#endif


#define VERBOSE				0

#define MAX_OPEN_FILES		16
#define FILE_BUFFER_SIZE	256


/*============================================================ */
/*	EXTERNALS */
/*============================================================ */

extern char *rompath_extra;

/* from datafile.c */
extern const char *db_filename;
extern const char *history_filename;
extern const char *mameinfo_filename;

/* from cheat.c */
extern char *cheatfile;



/*============================================================ */
/*	TYPE DEFINITIONS */
/*============================================================ */

struct pathdata
{
	const char *rawpath;
	const char **path;
	int pathcount;
};

struct _osd_file
{
	FILE		*fileptr;
	long		filepos;
	long		end;
	long		offset;
	long		bufferbase;
	long		bufferbytes;
	unsigned char	buffer[FILE_BUFFER_SIZE];
};

static struct pathdata pathlist[FILETYPE_end];
static osd_file openfile[MAX_OPEN_FILES];



/*============================================================ */
/*	GLOBAL VARIABLES */
/*============================================================ */

FILE *errorlog = NULL;

#ifdef MESS
static char crcfilename[256] = "";
const char *crcfile = crcfilename;
static char pcrcfilename[256] = "";
const char *pcrcfile = pcrcfilename;
char crcdir[256];
#endif

char *playbackname;
char *recordname;

FILE *stdout_file;
FILE *stderr_file;


/*============================================================ */
/*	FILE PATH OPTIONS */
/*============================================================ */

struct rc_option fileio_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "File I/O-related", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
#ifndef MESS
	{ "rompath", "rp", rc_string, &pathlist[FILETYPE_ROM].rawpath, XMAMEROOT"/roms", 0, 0, NULL, "Search path for rom files" },
#else
	{ "biospath", "bp", rc_string, &pathlist[FILETYPE_ROM].rawpath, XMAMEROOT"/bios", 0, 0, NULL, "Search path for BIOS sets" },
	{ "softwarepath", "swp", rc_string, &pathlist[FILETYPE_IMAGE].rawpath, XMAMEROOT"/software", 0, 0, NULL,  "Search path for software" },
	{ "CRC_directory", "crc", rc_string, &pathlist[FILETYPE_CRC].rawpath, XMAMEROOT"/crc", 0, 0, NULL, "Directory containing CRC files" },
#endif
	{ "samplepath", "sp", rc_string, &pathlist[FILETYPE_SAMPLE].rawpath, XMAMEROOT"/samples", 0, 0, NULL, "Search path for sample files" },
	{ "inipath", NULL, rc_string, &pathlist[FILETYPE_INI].rawpath, XMAMEROOT"/ini", 0, 0, NULL, "Search path for ini files" },
	{ "cfg_directory", NULL, rc_string, &pathlist[FILETYPE_CONFIG].rawpath, "$HOME/."NAME"/cfg", 0, 0, NULL, "Directory to save configurations" },
	{ "nvram_directory", NULL, rc_string, &pathlist[FILETYPE_NVRAM].rawpath, "$HOME/."NAME"/nvram", 0, 0, NULL, "Directory to save nvram contents" },
	{ "memcard_directory", NULL, rc_string, &pathlist[FILETYPE_MEMCARD].rawpath, "$HOME/."NAME"/memcard", 0, 0, NULL, "Directory to save memory card contents" },
	{ "input_directory", NULL, rc_string, &pathlist[FILETYPE_INPUTLOG].rawpath, "$HOME/."NAME"/inp", 0, 0, NULL, "Directory to save input device logs" },
	{ "hiscore_directory", NULL, rc_string, &pathlist[FILETYPE_HIGHSCORE].rawpath, "$HOME/."NAME"/hi", 0, 0, NULL, "Directory to save hiscores" },
	{ "state_directory", NULL, rc_string, &pathlist[FILETYPE_STATE].rawpath, "$HOME/."NAME"/sta", 0, 0, NULL, "Directory to save states" },
	{ "artwork_directory", NULL, rc_string, &pathlist[FILETYPE_ARTWORK].rawpath, XMAMEROOT"/artwork", 0, 0, NULL, "Directory for Artwork (Overlays etc.)" },
	{ "snapshot_directory", NULL, rc_string, &pathlist[FILETYPE_SCREENSHOT].rawpath, XMAMEROOT"/snap", 0, 0, NULL, "Directory for screenshots (.png format)" },
	{ "diff_directory", NULL, rc_string, &pathlist[FILETYPE_IMAGE_DIFF].rawpath, "$HOME/."NAME"/diff", 0, 0, NULL, "Directory for hard drive image difference files" },
	{ "ctrlr_directory", NULL, rc_string, &pathlist[FILETYPE_CTRLR].rawpath, XMAMEROOT"/ctrlr", 0, 0, NULL, "Directory to save controller definitions" },
	{ "cheat_file", NULL, rc_string, &cheatfile, XMAMEROOT"/cheat.dat", 0, 0, NULL, "Cheat filename" },
	{ "hiscore_file", NULL, rc_string, &db_filename, XMAMEROOT"/hiscore.dat", 0, 0, NULL, NULL },
#ifdef MESS
	{ "sysinfo_file", NULL, rc_string, &history_filename, XMAMEROOT"/sysinfo.dat", 0, 0, NULL, NULL },
	{ "messinfo_file", NULL, rc_string, &mameinfo_filename, XMAMEROOT"/messinfo.dat", 0, 0, NULL, NULL },
#else
	{ "history_file", NULL, rc_string, &history_filename, XMAMEROOT"/history.dat", 0, 0, NULL, NULL },
	{ "mameinfo_file", NULL, rc_string, &mameinfo_filename, XMAMEROOT"/mameinfo.dat", 0, 0, NULL, NULL },
#endif
	{ "record", "rec", rc_string, &recordname, NULL, 0, 0, NULL, "Set a file to record keypresses into" },
	{ "playback", "pb", rc_string, &playbackname, NULL, 0, 0, NULL, "Set a file to playback keypresses from" },
	{ "stdout-file", "out", rc_file, &stdout_file, NULL, 1,	0, NULL, "Set a file to redirect stdout to" },
	{ "stderr-file", "err",	rc_file, &stderr_file, NULL, 1, 0, NULL, "Set a file to redirect stderr to" },
	{ "log", "L", rc_file, &errorlog, NULL, 1, 0, NULL, "Set a file to log debug info to" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};



/*============================================================ */
/*	is_pathsep */
/*============================================================ */

INLINE int is_pathsep(char c)
{
	return (c == '/' || c == '\\');
}



/*============================================================ */
/*	find_reverse_path_sep */
/*============================================================ */

static char *find_reverse_path_sep(char *name)
{
	char *p = name + strlen(name) - 1;
	while (p >= name && !is_pathsep(*p))
		p--;
	return (p >= name) ? p : NULL;
}



/*============================================================ */
/*	create_path */
/*============================================================ */

static void create_path(char *path, int has_filename)
{
	char *sep = find_reverse_path_sep(path);

	/* if there's still a separator, and it's not the root, nuke it and recurse */
	if (sep && sep > path && !is_pathsep(sep[-1]))
	{
		*sep = 0;
		create_path(path, 0);
		*sep = '/';
	}

	/* if we have a filename, we're done */
	if (has_filename)
		return;

	/* create the path */
	rc_check_and_create_dir(path);
}



/*============================================================ */
/*	is_variablechar */
/*============================================================ */

INLINE int is_variablechar(char c)
{
	return (isalnum(c) || c == '_' || c == '-');
}



/*============================================================ */
/*	parse_variable */
/*============================================================ */

static const char *parse_variable(const char **start, const char *end)
{
	const char *src = *start, *var;
	char variable[1024];
	char *dest = variable;

	/* copy until we hit the end or until we hit a non-variable character */
	for (src = *start; src < end && is_variablechar(*src); src++)
		*dest++ = *src;

	/* an empty variable means "$" and should not be expanded */
	if (src == *start)
		return "$";

	/* NULL terminate and return a pointer to the end */
	*dest = 0;
	*start = src;

	/* return the actual variable value */
	var = getenv(variable);
	return (var) ? var : "";
}



/*============================================================ */
/*	copy_and_expand_variables */
/*============================================================ */

static char *copy_and_expand_variables(const char *path, int len)
{
	char *dst, *result;
	const char *src;
	int length = 0;

	/* first determine the length of the expanded string */
	for (src = path; src < path + len; )
		if (*src++ == '$')
			length += strlen(parse_variable(&src, path + len));
		else
			length++;

	/* allocate a string of the appropriate length */
	result = malloc(length + 1);
	if (!result)
		goto out_of_memory;

	/* now actually generate the string */
	for (src = path, dst = result; src < path + len; )
	{
		char c = *src++;
		if (c == '$')
			dst += sprintf(dst, "%s", parse_variable(&src, path + len));
		else
			*dst++ = c;
	}

	/* NULL terminate and return */
	*dst = 0;
	return result;

out_of_memory:
	fprintf(stderr, "Out of memory in variable expansion!\n");
	exit(1);
}



/*============================================================ */
/*	expand_pathlist */
/*============================================================ */

static void expand_pathlist(struct pathdata *list)
{
	const char *rawpath = (list->rawpath) ? list->rawpath : "";
	const char *token;

#if VERBOSE
	printf("Expanding: %s\n", rawpath);
#endif

	/* free any existing paths */
	if (list->pathcount != 0)
	{
		int pathindex;

		for (pathindex = 0; pathindex < list->pathcount; pathindex++)
			free((void *)list->path[pathindex]);
		free(list->path);
	}

	/* by default, start with an empty list */
	list->path = NULL;
	list->pathcount = 0;

	/* look for separators */
	token = strchr(rawpath, ':');
	if (!token)
		token = rawpath + strlen(rawpath);

	/* loop until done */
	while (1)
	{
		/* allocate space for the new pointer */
		list->path = realloc(list->path, (list->pathcount + 1) * sizeof(char *));
		if (!list->path)
			goto out_of_memory;

		/* copy the path in */
		list->path[list->pathcount++] = copy_and_expand_variables(rawpath, token - rawpath);
#if VERBOSE
		printf("  %s\n", list->path[list->pathcount - 1]);
#endif

		/* if this was the end, break */
		if (*token == 0)
			break;
		rawpath = token + 1;

		/* find the next separator */
		token = strchr(rawpath, ':');
		if (!token)
			token = rawpath + strlen(rawpath);
	}

	/* when finished, reset the path info, so that future INI parsing will */
	/* cause us to get called again */
	list->rawpath = NULL;
	return;

out_of_memory:
	fprintf(stderr, "Out of memory!\n");
	exit(1);
}



/*============================================================ */
/*	get_path_for_filetype */
/*============================================================ */

static const char *get_path_for_filetype(int filetype, int pathindex, int *count)
{
	struct pathdata *list;

	/* handle aliasing of some paths */
	switch (filetype)
	{
#ifndef MESS
		case FILETYPE_IMAGE:
			list = &pathlist[FILETYPE_ROM];
			break;
#endif

		default:
			list = &pathlist[filetype];
			break;
	}

	/* if we don't have expanded paths, expand them now */
	if (list->pathcount == 0 || list->rawpath)
	{
		/* special hack for ROMs */
		if (list == &pathlist[FILETYPE_ROM] && rompath_extra)
		{
			/* this may leak a little memory, but it's a hack anyway! :-P */
			const char *rawpath = (list->rawpath) ? list->rawpath : "";
			char *newpath = malloc(strlen(rompath_extra) + strlen(rawpath) + 2);
			sprintf(newpath, "%s:%s", rompath_extra, rawpath);
			list->rawpath = newpath;
		}

		/* decompose the path */
		expand_pathlist(list);
	}

	/* set the count */
	if (count)
		*count = list->pathcount;

	/* return a valid path always */
	return (pathindex < list->pathcount) ? list->path[pathindex] : "";
}



/*============================================================ */
/*	compose_path */
/*============================================================ */

static void compose_path(char *output, int pathtype, int pathindex, const char *filename)
{
	const char *basepath = get_path_for_filetype(pathtype, pathindex, NULL);
	char *p;

#ifdef MESS
	if (osd_is_absolute_path(filename))
		basepath = NULL;
#endif

	/* compose the full path */
	*output = 0;
	if (basepath)
		strcat(output, basepath);
	if (*output && !is_pathsep(output[strlen(output) - 1]))
		strcat(output, "/");
	strcat(output, filename);

	/* convert backslashes to forward slashes */
	for (p = output; *p; p++)
		if (*p == '\\')
			*p = '/';
}



/*============================================================ */
/*	osd_get_path_count */
/*============================================================ */

int osd_get_path_count(int pathtype)
{
	int count;

	/* get the count and return it */
	get_path_for_filetype(pathtype, 0, &count);
	return count;
}



/*============================================================ */
/*	osd_get_path_info */
/*============================================================ */

int osd_get_path_info(int pathtype, int pathindex, const char *filename)
{
	struct stat buf;
	char fullpath[1024];

	/* compose the full path */
	compose_path(fullpath, pathtype, pathindex, filename);

	/* get the file attributes */
	if (stat(fullpath, &buf))
		return PATH_NOT_FOUND;
	else if (S_ISDIR(buf.st_mode))
		return PATH_IS_DIRECTORY;
	else
		return PATH_IS_FILE;
}



/*============================================================ */
/*	osd_fopen */
/*============================================================ */

osd_file *osd_fopen(int pathtype, int pathindex, const char *filename, const char *mode)
{
	char fullpath[1024];
	osd_file *file;
	int i;
	int offs;

	/* find an empty file pointer */
	for (i = 0; i < MAX_OPEN_FILES; i++)
		if (openfile[i].fileptr == NULL)
			break;
	if (i == MAX_OPEN_FILES)
		return NULL;

	/* zap the file record */
	file = &openfile[i];
	memset(file, 0, sizeof(*file));

	/* compose the full path */
	compose_path(fullpath, pathtype, pathindex, filename);

	/* attempt to open the file */
	file->fileptr = fopen(fullpath, mode);
	if (file->fileptr == NULL)
	{
		/* if it's read-only, or if the path exists, then that's final */
		if (!(strchr(mode, 'w')) || errno != EACCES)
			return NULL;

		/* create the path and try again */
		create_path(fullpath, 1);
		file->fileptr = fopen(fullpath, mode);

		/* if that doesn't work, we give up */
		if (file->fileptr == NULL)
			return NULL;
	}

	/* get the file size */
	offs = fseek(file->fileptr, 0, SEEK_END);
	file->end = ftell(file->fileptr);
	fseek(file->fileptr, offs, SEEK_SET);
	return file;
}



/*============================================================ */
/*	osd_fseek */
/*============================================================ */

int osd_fseek(osd_file *file, INT64 offset, int whence)
{
	/* convert the whence into method */
	switch (whence)
	{
		default:
		case SEEK_SET:	file->offset = offset;				break;
		case SEEK_CUR:	file->offset += offset;				break;
		case SEEK_END:	file->offset = file->end + offset;	break;
	}
	return 0;
}



/*============================================================ */
/*	osd_ftell */
/*============================================================ */

UINT64 osd_ftell(osd_file *file)
{
	return file->offset;
}



/*============================================================ */
/*	osd_feof */
/*============================================================ */

int osd_feof(osd_file *file)
{
	return (file->offset >= file->end);
}



/*============================================================ */
/*	osd_fread */
/*============================================================ */

UINT32 osd_fread(osd_file *file, void *buffer, UINT32 length)
{
	UINT32 bytes_left = length;
	int bytes_to_copy;
	int result;

	/* handle data from within the buffer */
	if (file->offset >= file->bufferbase && file->offset < file->bufferbase + file->bufferbytes)
	{
		/* copy as much as we can */
		bytes_to_copy = file->bufferbase + file->bufferbytes - file->offset;
		if (bytes_to_copy > length)
			bytes_to_copy = length;
		memcpy(buffer, &file->buffer[file->offset - file->bufferbase], bytes_to_copy);

		/* account for it */
		bytes_left -= bytes_to_copy;
		file->offset += bytes_to_copy;
		buffer = (unsigned char *)buffer + bytes_to_copy;

		/* if that's it, we're done */
		if (bytes_left == 0)
			return length;
	}

	/* attempt to seek to the current location if we're not there already */
	if (file->offset != file->filepos)
	{
		result = fseek(file->fileptr, file->offset, SEEK_SET);
		if (result && errno)
		{
			file->filepos = ~0;
			return length - bytes_left;
		}
		file->filepos = file->offset;
	}

	/* if we have a small read remaining, do it to the buffer and copy out the results */
	if (length < FILE_BUFFER_SIZE/2)
	{
		/* read as much of the buffer as we can */
		file->bufferbase = file->offset;
		file->bufferbytes = 0;
		file->bufferbytes = fread(file->buffer, sizeof(unsigned char), FILE_BUFFER_SIZE, file->fileptr);
		file->filepos += file->bufferbytes;

		/* copy it out */
		bytes_to_copy = bytes_left;
		if (bytes_to_copy > file->bufferbytes)
			bytes_to_copy = file->bufferbytes;
		memcpy(buffer, file->buffer, bytes_to_copy);

		/* adjust pointers and return */
		file->offset += bytes_to_copy;
		bytes_left -= bytes_to_copy;
		return length - bytes_left;
	}

	/* otherwise, just read directly to the buffer */
	else
	{
		/* do the read */
		result = fread(buffer, sizeof(unsigned char), bytes_left, file->fileptr);
		file->filepos += result;

		/* adjust the pointers and return */
		file->offset += result;
		bytes_left -= result;
		return length - bytes_left;
	}
}



/*============================================================ */
/*	osd_fwrite */
/*============================================================ */

UINT32 osd_fwrite(osd_file *file, const void *buffer, UINT32 length)
{
	int result;

	/* invalidate any buffered data */
	file->bufferbytes = 0;

	/* attempt to seek to the current location */
	result = fseek(file->fileptr, file->offset, SEEK_SET);
	if (result && errno)
		return 0;

	/* do the write */
	result = fwrite(buffer, sizeof(unsigned char), length, file->fileptr);
	file->filepos += result;

	/* adjust the pointers */
	file->offset += result;
	if (file->offset > file->end)
		file->end = file->offset;
	return result;
}



/*============================================================ */
/*	osd_fclose */
/*============================================================ */

void osd_fclose(osd_file *file)
{
	/* close the handle and clear it out */
	if (file->fileptr)
		fclose(file->fileptr);
	file->fileptr = NULL;
}



#ifdef MESS
/*============================================================ */
/*	osd_create_directory */
/*============================================================ */

int osd_create_directory(int pathtype, int pathindex, const char *dirname)
{
	char fullpath[1024];

	/* compose the full path */
	compose_path(fullpath, pathtype, pathindex, dirname);

	return rc_check_and_create_dir(fullpath) ? 0 : 1;
}
#endif

/*============================================================ */
/*	osd_display_loading_rom_message */
/*============================================================ */

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,
		struct rom_load_data *romdata)
{
	static int count = 0;
	
	if (name)
		fprintf(stderr_file,"loading rom %d: %-12s\n", count, name);
	else
		fprintf(stderr_file,"done\n");
	
	fflush(stderr_file);
	count++;

	return 0;
}



#ifdef MESS
/*============================================================ */
/*	build_crc_database_filename */
/*============================================================ */

void build_crc_database_filename(int game_index)
{
	/* Build the CRC database filename */
	sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
	if (drivers[game_index]->clone_of->name)
		sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
	else
		pcrcfilename[0] = 0;
}

int osd_select_file(mess_image *img, char *filename)
{
	return 0;
}

void osd_begin_final_unloading(void)
{
}

void osd_image_load_status_changed(mess_image *img, int is_final_unload)
{
}
#endif
