/***************************************************************************

	fileio.h

	Core file I/O interface functions and definitions.

***************************************************************************/

#ifndef FILEIO_H
#define FILEIO_H
#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)	// GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <stdarg.h>
#include "osdepend.h"
#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif


/* file types */
enum
{
	FILETYPE_RAW = 0,
	FILETYPE_ROM,
	FILETYPE_IMAGE,
	FILETYPE_IMAGE_DIFF,
#ifdef PINMAME
	FILETYPE_WAVE,
#ifdef PROC_SUPPORT
	FILETYPE_PROC,	/* for path */
	FILETYPE_PROC_YAML,
#endif /* PROC_SUPPORT */
#endif /* PINMAME */
	FILETYPE_SAMPLE,
	FILETYPE_ARTWORK,
	FILETYPE_NVRAM,
	FILETYPE_HIGHSCORE,
	FILETYPE_HIGHSCORE_DB,
	FILETYPE_CONFIG,
	FILETYPE_INPUTLOG,
	FILETYPE_STATE,
	FILETYPE_MEMCARD,
	FILETYPE_SCREENSHOT,
	FILETYPE_HISTORY,
	FILETYPE_CHEAT,
	FILETYPE_LANGUAGE,
	FILETYPE_CTRLR,
	FILETYPE_INI,
#ifdef MESS
	FILETYPE_CRC,
#endif
	FILETYPE_end /* dummy last entry */
};


/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

struct _mame_file
{
#ifdef MAME_DEBUG
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

#define PLAIN_FILE				0
#define RAM_FILE				1
#define ZIPPED_FILE				2
#define UNLOADED_ZIPPED_FILE	3

typedef struct _mame_file mame_file;

int mame_faccess(const char *filename, int filetype);
mame_file *mame_fopen(const char *gamename, const char *filename, int filetype, int openforwrite);
mame_file *mame_fopen_rom(const char *gamename, const char *filename, const char* exphash);
UINT32 mame_fread(mame_file *file, void *buffer, size_t length);
UINT32 mame_fwrite(mame_file *file, const void *buffer, size_t length);
UINT32 mame_fread_swap(mame_file *file, void *buffer, size_t length);
UINT32 mame_fwrite_swap(mame_file *file, const void *buffer, size_t length);
#ifdef LSB_FIRST
#define mame_fread_msbfirst mame_fread_swap
#define mame_fwrite_msbfirst mame_fwrite_swap
#define mame_fread_lsbfirst mame_fread
#define mame_fwrite_lsbfirst mame_fwrite
#else
#define mame_fread_msbfirst mame_fread
#define mame_fwrite_msbfirst mame_fwrite
#define mame_fread_lsbfirst mame_fread_swap
#define mame_fwrite_lsbfirst mame_fwrite_swap
#endif
int mame_fseek(mame_file *file, INT64 offset, int whence);
void mame_fclose(mame_file *file);
int mame_fchecksum(const char *gamename, const char *filename, unsigned int *length, char* hash);
UINT64 mame_fsize(mame_file *file);
const char *mame_fhash(mame_file *file);
int mame_fgetc(mame_file *file);
int mame_ungetc(int c, mame_file *file);
char *mame_fgets(char *s, int n, mame_file *file);
int mame_feof(mame_file *file);
UINT64 mame_ftell(mame_file *file);

int mame_fputs(mame_file *f, const char *s);
int mame_vfprintf(mame_file *f, const char *fmt, va_list va);

#ifdef __GNUC__
int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...)
      __attribute__ ((format (printf, 2, 3)));
#else
int CLIB_DECL mame_fprintf(mame_file *f, const char *fmt, ...);
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif
