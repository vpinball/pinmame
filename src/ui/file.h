/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef FILE_H
#define FILE_H

extern int   File_Init(void);
extern void  File_Exit(void);
extern BOOL  File_ExistZip(const char* gamename, int filetype);
extern BOOL  File_Status(const char* gamename, const char* filename, int filetype);
extern void  File_UpdatePaths(void);

extern void* osd_fopen2(const char* gamename, const char* filename, int filetype, int openforwrite);

#define OSD_FILETYPE_FLYER 1001

#endif
