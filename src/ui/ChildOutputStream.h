/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

#ifndef CHILDOUTPUTSTREAM_H
#define CHILDOUTPUTSTREAM_H

struct t_ChildOutputStream
{
	HANDLE m_hReadThread;
	HANDLE m_hRead;
	HANDLE m_hWrite;
	char*  m_strBuffer;
	DWORD  m_nBufferLen;
	DWORD  m_nMaxBufferLen;
};

typedef	struct t_ChildOutputStream CChildOutputStream;

extern void ChildOutputStream_Init(struct t_ChildOutputStream*);
extern void ChildOutputStream_Exit(struct t_ChildOutputStream*);
extern BOOL ChildOutputStream_Create(struct t_ChildOutputStream*, DWORD);
extern void ChildOutputStream_WaitForThreadExit(struct t_ChildOutputStream*);
extern void ChildOutputStream_Dump(struct t_ChildOutputStream*, FILE*);

#endif
