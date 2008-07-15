/***************************************************************************

	fileio.c - file access functions

***************************************************************************/

#include <zlib.h>

#include <assert.h>
#include "driver.h"
#include "unzip.h"

#ifdef MESS
#include "image.h"
#endif


/***************************************************************************
	DEBUGGING
***************************************************************************/

/* Verbose outputs to error.log ? */
#define VERBOSE 					0

/* enable lots of logging */
#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif



/***************************************************************************
	CONSTANTS
***************************************************************************/

#define PLAIN_FILE				0
#define RAM_FILE				1
#define ZIPPED_FILE				2
#define UNLOADED_ZIPPED_FILE	3

#define FILEFLAG_OPENREAD		0x01
#define FILEFLAG_OPENWRITE		0x02
#define FILEFLAG_HASH			0x04
#define FILEFLAG_REVERSE_SEARCH	0x08
#define FILEFLAG_VERIFY_ONLY	0x10
#define FILEFLAG_NOZIP			0x20


#ifdef MAME_DEBUG
#define DEBUG_COOKIE			0xbaadf00d
#endif

/***************************************************************************
	TYPE DEFINITIONS
***************************************************************************/

struct _mame_file
{
#ifdef DEBUG_COOKIE
	UINT32 debug_cookie;
#endif
	osd_file *file;
	UINT8 *data;
	UINT64 offset;
	UINT64 length;
	UINT8 eof;
	UINT8 type;
	char hash[HASH_BUF_SIZE];
};



/***************************************************************************
	PROTOTYPES
***************************************************************************/

static mame_file *generic_fopen(int pathtype, const char *gamename, const char *filename, const char* hash, UINT32 flags);
static const char *get_extension_for_filetype(int filetype);
static int checksum_file(int pathtype, int pathindex, const char *file, UINT8 **p, UINT64 *size, char* hash);


/***************************************************************************
	mame_fopen
***************************************************************************/

mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite)
{
	/* first verify that we aren't trying to open read-only types as writeables */
	switch (filetype)
	{
		/* read-only cases */
		case FILETYPE_ROM:
#ifndef MESS
		case FILETYPE_IMAGE:
#endif
		case FILETYPE_SAMPLE:
		case FILETYPE_HIGHSCORE_DB:
		case FILETYPE_ARTWORK:
		case FILETYPE_HISTORY:
		case FILETYPE_LANGUAGE:
		case FILETYPE_INI:
			if (openforwrite)
			{
				logerror("mame_fopen: type %02x write not supported\n", filetype);
				return NULL;
			}
			break;

		/* write-only cases */
		case FILETYPE_SCREENSHOT:
#ifdef PINMAME
                case FILETYPE_WAVE:
#endif /* PINMAME */

			if (!openforwrite)
			{
				logerror("mame_fopen: type %02x read not supported\n", filetype);
				return NULL;
			}
			break;
	}

	/* now open the file appropriately */
	switch (filetype)
	{
		/* ROM files */
		case FILETYPE_ROM:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_HASH);

		/* read-only disk images */
		case FILETYPE_IMAGE:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_NOZIP);

		/* differencing disk images */
		case FILETYPE_IMAGE_DIFF:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD | FILEFLAG_OPENWRITE);

		/* samples */
		case FILETYPE_SAMPLE:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD);

		/* artwork files */
		case FILETYPE_ARTWORK:
			return generic_fopen(filetype, gamename, filename, 0, FILEFLAG_OPENREAD);

		/* NVRAM files */
		case FILETYPE_NVRAM:
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* high score files */
		case FILETYPE_HIGHSCORE:
			if (!mame_highscore_enabled())
				return NULL;
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* highscore database */
		case FILETYPE_HIGHSCORE_DB:
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD);

		/* config files */
		case FILETYPE_CONFIG:
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* input logs */
		case FILETYPE_INPUTLOG:
			return generic_fopen(filetype, NULL, gamename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* save state files */
		case FILETYPE_STATE:
			return generic_fopen(filetype, NULL, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* memory card files */
		case FILETYPE_MEMCARD:
			return generic_fopen(filetype, NULL, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* screenshot files */
		case FILETYPE_SCREENSHOT:
#ifdef PINMAME
                case FILETYPE_WAVE:
#endif /* PINMAME */

			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENWRITE);

		/* history files */
		case FILETYPE_HISTORY:
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD);

		/* cheat file */
		case FILETYPE_CHEAT:
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD | (openforwrite ? FILEFLAG_OPENWRITE : 0));

		/* language file */
		case FILETYPE_LANGUAGE:
			return generic_fopen(filetype, NULL, filename, 0, FILEFLAG_OPENREAD);

		/* ctrlr files */
		case FILETYPE_CTRLR:
			return generic_fopen(filetype, gamename, filename, 0, openforwrite ? FILEFLAG_OPENWRITE : FILEFLAG_OPENREAD);

		/* game specific ini files */
		case FILETYPE_INI:
			return generic_fopen(filetype, NULL, gamename, 0, FILEFLAG_OPENREAD);

		/* anything else */
		default:
			logerror("mame_fopen(): unknown filetype %02x\n", filetype);
			return NULL;
	}
	return NULL;
}



/***************************************************************************
	mame_fopen_rom
***************************************************************************/

/* Similar to mame_fopen(,,FILETYPE_ROM), but lets you specify an expected checksum 
   (better encapsulation of the load by CRC used for ZIP files) */
mame_file *mame_fopen_rom(const char *gamename, const char *filename, const char* exphash)
{
	return generic_fopen(FILETYPE_ROM, gamename, filename, exphash, FILEFLAG_OPENREAD | FILEFLAG_HASH);
}


/***************************************************************************
	mame_fclose
***************************************************************************/

void mame_fclose(mame_file *file)
{
#ifdef DEBUG_COOKIE
	assert(file->debug_cookie == DEBUG_COOKIE);
	file->debug_cookie = 0;
#endif

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			osd_fclose(file->file);
			break;

		case ZIPPED_FILE:
		case RAM_FILE:
			if (file->data)
				free(file->data);
			break;
	}

	/* free the file data */
	free(file);
}



/***************************************************************************
	mame_faccess
***************************************************************************/

int mame_faccess(const char *filename, int filetype)
{
	const char *extension = get_extension_for_filetype(filetype);
	int pathcount = osd_get_path_count(filetype);
	char modified_filename[256];
	int pathindex;

	/* copy the filename and add an extension */
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

	/* loop over all paths */
	for (pathindex = 0; pathindex < pathcount; pathindex++)
	{
		char name[256];

		/* first check the raw filename, in case we're looking for a directory */
		sprintf(name, "%s", filename);
		LOG(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;

		/* try again with a .zip extension */
		sprintf(name, "%s.zip", filename);
		LOG(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;

		/* does such a directory (or file) exist? */
		sprintf(name, "%s", modified_filename);
		LOG(("mame_faccess: trying %s\n", name));
		if (osd_get_path_info(filetype, pathindex, name) != PATH_NOT_FOUND)
			return 1;
	}

	/* no match */
	return 0;
}



/***************************************************************************
	mame_fread
***************************************************************************/

UINT32 mame_fread(mame_file *file, void *buffer, UINT32 length)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fread(file->file, buffer, length);

		case ZIPPED_FILE:
		case RAM_FILE:
			if (file->data)
			{
				if (file->offset + length > file->length)
				{
					length = file->length - file->offset;
					file->eof = 1;
				}
				memcpy(buffer, file->data + file->offset, length);
				file->offset += length;
				return length;
			}
			break;
	}

	return 0;
}



/***************************************************************************
	mame_fwrite
***************************************************************************/

UINT32 mame_fwrite(mame_file *file, const void *buffer, UINT32 length)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fwrite(file->file, buffer, length);
	}

	return 0;
}



/***************************************************************************
	mame_fseek
***************************************************************************/

int mame_fseek(mame_file *file, INT64 offset, int whence)
{
	int err = 0;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_fseek(file->file, offset, whence);

		case ZIPPED_FILE:
		case RAM_FILE:
			switch (whence)
			{
				case SEEK_SET:
					file->offset = offset;
					break;
				case SEEK_CUR:
					file->offset += offset;
					break;
				case SEEK_END:
					file->offset = file->length + offset;
					break;
			}
			file->eof = 0;
			break;
	}

	return err;
}



/***************************************************************************
	mame_fchecksum
***************************************************************************/

int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, char* hash)
{
	mame_file *file;

	/* first open the file; we pass the source hash because it contains
	   the expected checksum for the file (used to load by checksum) */
	file = generic_fopen(FILETYPE_ROM, gamename, filename, hash, FILEFLAG_OPENREAD | FILEFLAG_HASH | FILEFLAG_VERIFY_ONLY);

	/* if we didn't succeed return -1 */
	if (!file)
		return -1;

	/* close the file and save the length & checksum */
	hash_data_copy(hash, file->hash);
	*length = file->length;
	mame_fclose(file);
	return 0;
}



/***************************************************************************
	mame_fsize
***************************************************************************/

UINT64 mame_fsize(mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
		{
			int size, offs;
			offs = osd_ftell(file->file);
			osd_fseek(file->file, 0, SEEK_END);
			size = osd_ftell(file->file);
			osd_fseek(file->file, offs, SEEK_SET);
			return size;
		}

		case RAM_FILE:
		case ZIPPED_FILE:
			return file->length;
	}

	return 0;
}



/***************************************************************************
	mame_fhash
***************************************************************************/

const char* mame_fhash(mame_file *file)
{
	return file->hash;
}



/***************************************************************************
	mame_fgetc
***************************************************************************/

int mame_fgetc(mame_file *file)
{
	unsigned char buffer;

	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			if (osd_fread(file->file, &buffer, 1) == 1)
				return buffer;
			return EOF;

		case RAM_FILE:
		case ZIPPED_FILE:
			if (file->offset < file->length)
				return file->data[file->offset++];
			else
				file->eof = 1;
			return EOF;
	}
	return EOF;
}



/***************************************************************************
	mame_ungetc
***************************************************************************/

int mame_ungetc(int c, mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			if (osd_feof(file->file))
			{
				if (osd_fseek(file->file, 0, SEEK_CUR))
					return c;
			}
			else
			{
			if (osd_fseek(file->file, -1, SEEK_CUR))
				return c;
			}
			return EOF;

		case RAM_FILE:
		case ZIPPED_FILE:
			if (file->eof)
				file->eof = 0;
			else if (file->offset > 0)
			{
				file->offset--;
				return c;
			}
			return EOF;
	}
	return EOF;
}



/***************************************************************************
	mame_fgets
***************************************************************************/

char *mame_fgets(char *s, int n, mame_file *file)
{
	char *cur = s;

	/* loop while we have characters */
	while (n > 0)
	{
		int c = mame_fgetc(file);
		if (c == EOF)
			break;

		/* if there's a CR, look for an LF afterwards */
		if (c == 0x0d)
		{
			int c2 = mame_fgetc(file);
			if (c2 != 0x0a)
				mame_ungetc(c2, file);
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* if there's an LF, reinterp as a CR for consistency */
		else if (c == 0x0a)
		{
			*cur++ = 0x0d;
			n--;
			break;
		}

		/* otherwise, pop the character in and continue */
		*cur++ = c;
		n--;
	}

	/* if we put nothing in, return NULL */
	if (cur == s)
		return NULL;

	/* otherwise, terminate */
	if (n > 0)
		*cur++ = 0;
	return s;
}



/***************************************************************************
	mame_feof
***************************************************************************/

int mame_feof(mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_feof(file->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return (file->eof);
	}

	return 1;
}



/***************************************************************************
	mame_ftell
***************************************************************************/

UINT64 mame_ftell(mame_file *file)
{
	/* switch off the file type */
	switch (file->type)
	{
		case PLAIN_FILE:
			return osd_ftell(file->file);

		case RAM_FILE:
		case ZIPPED_FILE:
			return file->offset;
	}

	return -1L;
}



/***************************************************************************
	mame_fread_swap
***************************************************************************/

UINT32 mame_fread_swap(mame_file *file, void *buffer, UINT32 length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	/* standard read first */
	res = mame_fread(file, buffer, length);

	/* swap the result */
	buf = buffer;
	for (i = 0; i < res; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}



/***************************************************************************
	mame_fwrite_swap
***************************************************************************/

UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, UINT32 length)
{
	UINT8 *buf;
	UINT8 temp;
	int res, i;

	/* swap the data first */
	buf = (UINT8 *)buffer;
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	/* do the write */
	res = mame_fwrite(file, buffer, length);

	/* swap the data back */
	for (i = 0; i < length; i += 2)
	{
		temp = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = temp;
	}

	return res;
}



/***************************************************************************
	compose_path
***************************************************************************/

INLINE void compose_path(char *output, const char *gamename, const char *filename, const char *extension)
{
	char *filename_base = output;
	*output = 0;

	/* if there's a gamename, add that; only add a '/' if there is a filename as well */
	if (gamename)
	{
		strcat(output, gamename);
		if (filename)
		{
			strcat(output, "/");
			filename_base = &output[strlen(output)];
		}
	}

	/* if there's a filename, add that */
	if (filename)
		strcat(output, filename);

	/* if there's no extension in the filename, add the extension */
	if (extension && !strchr(filename_base, '.'))
	{
		strcat(output, ".");
		strcat(output, extension);
	}
}



/***************************************************************************
	get_extension_for_filetype
***************************************************************************/

static const char *get_extension_for_filetype(int filetype)
{
	const char *extension;

	/* now open the file appropriately */
	switch (filetype)
	{
		case FILETYPE_RAW:			/* raw data files */
		case FILETYPE_ROM:			/* ROM files */
		case FILETYPE_HIGHSCORE_DB:	/* highscore database/history files */
		case FILETYPE_HISTORY:		/* game history files */
		case FILETYPE_CHEAT:		/* cheat file */
		default:					/* anything else */
			extension = NULL;
			break;

#ifndef MESS
		case FILETYPE_IMAGE:		/* disk image files */
			extension = "chd";
			break;
#endif

		case FILETYPE_IMAGE_DIFF:	/* differencing drive images */
			extension = "dif";
			break;

		case FILETYPE_SAMPLE:		/* samples */
			extension = "wav";
			break;

		case FILETYPE_ARTWORK:		/* artwork files */
		case FILETYPE_SCREENSHOT:	/* screenshot files */
			extension = "png";
			break;

		case FILETYPE_NVRAM:		/* NVRAM files */
			extension = "nv";
			break;

		case FILETYPE_HIGHSCORE:	/* high score files */
			extension = "hi";
			break;

		case FILETYPE_LANGUAGE:		/* language files */
			extension = "lng";
			break;

		case FILETYPE_CONFIG:		/* config files */
			extension = "cfg";
			break;

		case FILETYPE_INPUTLOG:		/* input logs */
			extension = "inp";
			break;

		case FILETYPE_STATE:		/* save state files */
			extension = "sta";
			break;

		case FILETYPE_MEMCARD:		/* memory card files */
			extension = "mem";
			break;

		case FILETYPE_CTRLR:		/* config files */
		case FILETYPE_INI:			/* game specific ini files */
			extension = "ini";
			break;
#ifdef PINMAME
		// wavefiles
		case FILETYPE_WAVE:
			extension = "wav";
			break;
#endif /* PINMAME */
	}
	return extension;
}



/***************************************************************************
	generic_fopen
***************************************************************************/

static mame_file *generic_fopen(int pathtype, const char *gamename, const char *filename, const char* hash, UINT32 flags)
{
	static const char *access_modes[] = { "rb", "rb", "wb", "r+b" };
	const char *extension = get_extension_for_filetype(pathtype);
	int pathcount = osd_get_path_count(pathtype);
	int pathindex, pathstart, pathstop, pathinc;
	mame_file file, *newfile;
	char tempname[256];

	LOG(("generic_fopen(%d, %s, %s, %s, %X)\n", pathc, gamename, filename, extension, flags));

	/* reset the file handle */
	memset(&file, 0, sizeof(file));

	/* check for incompatible flags */
	if ((flags & FILEFLAG_OPENWRITE) && (flags & FILEFLAG_HASH))
		fprintf(stderr, "Can't use HASH option with WRITE option in generic_fopen!\n");

	/* determine start/stop based on reverse search flag */
	if (!(flags & FILEFLAG_REVERSE_SEARCH))
	{
		pathstart = 0;
		pathstop = pathcount;
		pathinc = 1;
	}
	else
	{
		pathstart = pathcount - 1;
		pathstop = -1;
		pathinc = -1;
	}

	/* loop over paths */
	for (pathindex = pathstart; pathindex != pathstop; pathindex += pathinc)
	{
		char name[1024];

		/* ----------------- STEP 1: OPEN THE FILE RAW -------------------- */

		/* first look for path/gamename as a directory */
		compose_path(name, gamename, NULL, NULL);
		LOG(("Trying %s\n", name));

		/* if the directory exists, proceed */
		if (*name == 0 || osd_get_path_info(pathtype, pathindex, name) == PATH_IS_DIRECTORY)
		{
			/* now look for path/gamename/filename.ext */
			compose_path(name, gamename, filename, extension);

			/* if we need checksums, load it into RAM and compute it along the way */
			if (flags & FILEFLAG_HASH)
			{
				if (checksum_file(pathtype, pathindex, name, &file.data, &file.length, file.hash) == 0)
				{
					file.type = RAM_FILE;
					break;
				}
			}

			/* otherwise, just open it straight */
			else
			{
				file.type = PLAIN_FILE;
				file.file = osd_fopen(pathtype, pathindex, name, access_modes[flags & 3]);
				if (file.file == NULL && (flags & 3) == 3)
					file.file = osd_fopen(pathtype, pathindex, name, "w+b");
				if (file.file != NULL)
					break;
			}
		}

		/* ----------------- STEP 2: OPEN THE FILE IN A ZIP -------------------- */

		/* now look for it within a ZIP file */
		if (!(flags & (FILEFLAG_OPENWRITE | FILEFLAG_NOZIP)))
		{
			/* first look for path/gamename.zip */
			compose_path(name, gamename, NULL, "zip");
			LOG(("Trying %s file\n", name));

			/* if the ZIP file exists, proceed */
			if (osd_get_path_info(pathtype, pathindex, name) == PATH_IS_FILE)
			{
				UINT32 ziplength;

				/* if the file was able to be extracted from the ZIP, continue */
				compose_path(tempname, NULL, filename, extension);

				/* verify-only case */
				if (flags & FILEFLAG_VERIFY_ONLY)
				{
					UINT8 crcs[4];
					UINT32 crc = 0;

					/* Since this is a .ZIP file, we extract the CRC from the expected hash
					   (if any), so that we can load by CRC if needed. We must check that
					   the hash really contains a CRC, because it could be a NO_DUMP rom
					   for which we do not know the CRC yet. */
					if (hash && hash_data_extract_binary_checksum(hash, HASH_CRC, crcs) != 0)
					{
						/* Store the CRC in a single DWORD */
						crc = ((unsigned long)crcs[0] << 24) |
							  ((unsigned long)crcs[1] << 16) |
							  ((unsigned long)crcs[2] <<  8) |
							  ((unsigned long)crcs[3] <<  0);
					}

					hash_data_clear(file.hash);
						
					if (checksum_zipped_file(pathtype, pathindex, name, tempname, &ziplength, &crc) == 0)
					{
						file.length = ziplength;
						file.type = UNLOADED_ZIPPED_FILE;
						crcs[0] = (UINT8)(crc >> 24);
						crcs[1] = (UINT8)(crc >> 16);
						crcs[2] = (UINT8)(crc >> 8);
						crcs[3] = (UINT8)(crc >> 0);
						hash_data_insert_binary_checksum(file.hash, HASH_CRC, crcs);
						break;
					}
				}

				/* full load case */
				else
				{
					int err;

					/* Try loading the file */
					err = load_zipped_file(pathtype, pathindex, name, tempname, &file.data, &ziplength);

					/* If it failed, since this is a ZIP file, we can try to load by CRC 
					   if an expected hash has been provided. unzip.c uses this ugly hack 
					   of specifying the CRC as filename. */
					if (err && hash)
					{
						char crcn[9];

						hash_data_extract_printable_checksum(hash, HASH_CRC, crcn);

						err = load_zipped_file(pathtype, pathindex, name, crcn, &file.data, &ziplength);
					}

					if (err == 0)
					{
						unsigned functions;

						LOG(("Using (mame_fopen) zip file for %s\n", filename));
						file.length = ziplength;
						file.type = ZIPPED_FILE;
						/* Since we already loaded the file, we can easily calculate the
						   checksum of all the functions. In practice, we use only the
						   functions for which we have an expected checksum to compare with. */
						functions = hash_data_used_functions(hash);

						/* If user asked for CRC only, and there is an expected checksum
						   for CRC in the driver, compute only CRC. */
						if (options.crc_only && (functions & HASH_CRC))
							functions = HASH_CRC;

						hash_compute(file.hash, file.data, file.length, functions);
						break;
					}
				}
			}
		}
	}

	/* if we didn't succeed, just return NULL */
	if (pathindex == pathstop)
		return NULL;

	/* otherwise, duplicate the file */
	newfile = malloc(sizeof(file));
	if (newfile)
	{
		*newfile = file;
#ifdef DEBUG_COOKIE
		newfile->debug_cookie = DEBUG_COOKIE;
#endif
	}

	return newfile;
}



/***************************************************************************
	checksum_file
***************************************************************************/

static int checksum_file(int pathtype, int pathindex, const char *file, UINT8 **p, UINT64 *size, char* hash)
{
	UINT64 length;
	UINT8 *data;
	osd_file *f;
	unsigned int functions;

	/* open the file */
	f = osd_fopen(pathtype, pathindex, file, "rb");
	if (!f)
		return -1;

	/* determine length of file */
	if (osd_fseek(f, 0L, SEEK_END) != 0)
	{
		osd_fclose(f);
		return -1;
	}

	length = osd_ftell(f);
	if (length == -1L)
	{
		osd_fclose(f);
		return -1;
	}

	/* allocate space for entire file */
	data = malloc(length);
	if (!data)
	{
		osd_fclose(f);
		return -1;
	}

	/* read entire file into memory */
	if (osd_fseek(f, 0L, SEEK_SET) != 0)
	{
		free(data);
		osd_fclose(f);
		return -1;
	}

	if (osd_fread(f, data, length) != length)
	{
		free(data);
		osd_fclose(f);
		return -1;
	}

	*size = length;
	/* compute the checksums (only the functions for which we have an expected
	   checksum). Take also care of crconly: if the user asked, we will calculate
	   only the CRC, but only if there is an expected CRC for this file. */
	functions = hash_data_used_functions(hash);
	if (options.crc_only && (functions & HASH_CRC))
		functions = HASH_CRC;
	hash_compute(hash, data, length, functions);

	/* if the caller wants the data, give it away, otherwise free it */
	if (p)
		*p = data;
	else
		free(data);

	/* close the file */
	osd_fclose(f);
	return 0;
}
/***************************************************************************
	mame_fputs
***************************************************************************/

int mame_fputs(mame_file *f, const char *s)
{
	return mame_fwrite(f, s, strlen(s));
}



/***************************************************************************
	mame_vfprintf
***************************************************************************/

int mame_vfprintf(mame_file *f, const char *fmt, va_list va)
{
	char buf[512];
	_vsnprintf(buf, sizeof(buf), fmt, va);
	return mame_fputs(f, buf);
}



/***************************************************************************
	mame_fprintf
***************************************************************************/

int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...)
{
	int rc;
	va_list va;
	va_start(va, fmt);
	rc = mame_vfprintf(f, fmt, va);
	va_end(va);
	return rc;
}

