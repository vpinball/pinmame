/***************************************************************************

    M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
    Win32 Portions Copyright (C) 1997-98 Michael Soderstrom and Chris Kirmse
    
    This file is part of MAME32, and may only be used, modified and
    distributed under the terms of the MAME license, in "readme.txt".
    By continuing to use, modify or distribute this file you indicate
    that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef __FILE_H__
#define __FILE_H__

struct OSDFile
{
    int          (*init)(void);
    void         (*exit)(void);
    int          (*faccess)(const char *filename, int filetype);
    void*        (*fopen)(const char *gamename, const char *filename, int filetype, int write);
    int          (*fread)(void *file, void *buffer, int length);
    int          (*fwrite)(void *file, const void *buffer, int length);
    int          (*fseek)(void *file, int offset, int whence);
    void         (*fclose)(void *file);
    int          (*fchecksum) (const char *gamename, const char *filename, unsigned int *length, unsigned int *sum);
    int          (*fsize)(void *file);
    unsigned int (*fcrc)(void *file);
	int			 (*fread_scatter)(void *file, void *buffer, int length, int increment);
    int          (*fgetc)(void* file);
    int          (*ungetc)(int c, void* file);
    char*        (*fgets)(char* s, int n, void* file);
    int          (*eof)(void* file);
    int          (*ftell)(void* file);
};

extern struct OSDFile File;

extern BOOL     File_ExistZip(const char *gamename, int filetype);
extern void     File_UpdateRomPath(const char *path);
extern void     File_UpdateSamplePath(const char *path);
extern BOOL     File_Status(const char *gamename,const char *filename,int filetype);

#define OSD_FILETYPE_FLYER      1001

#endif
