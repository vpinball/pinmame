# Microsoft Developer Studio Project File - Name="PinMAME" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=PinMAME - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PinMAME.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PinMAME.mak" CFG="PinMAME - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PinMAME - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "PinMAME - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "obj/PinMAMEVC"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "src" /I "src\wpc" /I "src\zlib" /I "src\vc" /I "src\windows" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D LSB_FIRST=1 /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "_WINDOWS" /D "ZLIB_DLL" /D MAMEVER=3716 /D "PINMAME" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib zlibstatmt.lib /nologo /subsystem:console /machine:I386 /libpath:"zlib"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "obj/PinMAMEVCd"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "src" /I "src\wpc" /I "src\zlib" /I "src\vc" /I "src\windows" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D LSB_FIRST=1 /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "_WINDOWS" /D "ZLIB_DLL" /D "MAME_DEBUG" /D MAMEVER=3716 /D "PINMAME" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib zlibstatmtd.lib /nologo /subsystem:console /debug /machine:I386 /out:"PinMAMEVCd.exe" /pdbtype:sept /libpath:"zlib"

!ENDIF 

# Begin Target

# Name "PinMAME - Win32 Release"
# Name "PinMAME - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "MAME"

# PROP Default_Filter ""
# Begin Group "CPU"

# PROP Default_Filter ""
# Begin Group "ADSP2100"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\adsp2100\2100dasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\adsp2100\adsp2100.c
# End Source File
# Begin Source File

SOURCE=src\cpu\adsp2100\adsp2100.h
# End Source File
# End Group
# Begin Group "M6809"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\m6809\6809dasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6809\m6809.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6809\m6809.h
# End Source File
# End Group
# Begin Group "M6800"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\m6800\6800dasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6800\m6800.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6800\m6800.h
# End Source File
# End Group
# Begin Group "Z80"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\z80\z80.c
# End Source File
# Begin Source File

SOURCE=src\cpu\z80\z80.h
# End Source File
# Begin Source File

SOURCE=src\cpu\z80\z80daa.h
# End Source File
# Begin Source File

SOURCE=src\cpu\z80\z80dasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\z80\z80dasm.h
# End Source File
# End Group
# Begin Group "M6502"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\m6502\6502dasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6502\ill02.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m6502\m6502.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m6502\m6502.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m6502\ops02.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m6502\opsc02.h
# End Source File
# End Group
# Begin Group "M68000"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\cpu\m68000\cpudefs.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68000.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68k.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kconf.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kcpu.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kcpu.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kdasm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kmake.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kmame.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kmame.h
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kopac.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kopdm.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kopnz.c
# End Source File
# Begin Source File

SOURCE=src\cpu\m68000\m68kops.c
# End Source File
# End Group
# Begin Group "S2650"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\s2650\2650dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\s2650\s2650cpu.h
# End Source File
# End Group
# End Group
# Begin Group "Machine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\machine\6522via.c
# End Source File
# Begin Source File

SOURCE=src\machine\6522via.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6530riot.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6530riot.h
# End Source File
# Begin Source File

SOURCE=src\machine\6532riot.c
# End Source File
# Begin Source File

SOURCE=src\machine\6532riot.h
# End Source File
# Begin Source File

SOURCE=src\machine\6821pia.c
# End Source File
# Begin Source File

SOURCE=src\machine\6821pia.h
# End Source File
# Begin Source File

SOURCE=src\machine\8255ppi.c
# End Source File
# Begin Source File

SOURCE=src\machine\8255ppi.h
# End Source File
# Begin Source File

SOURCE=src\machine\eeprom.c
# End Source File
# Begin Source File

SOURCE=src\machine\eeprom.h
# End Source File
# Begin Source File

SOURCE=src\machine\mathbox.c
# End Source File
# Begin Source File

SOURCE=src\machine\mathbox.h
# End Source File
# Begin Source File

SOURCE=src\machine\ticket.c
# End Source File
# Begin Source File

SOURCE=src\machine\ticket.h
# End Source File
# Begin Source File

SOURCE=src\machine\z80fmly.c
# End Source File
# Begin Source File

SOURCE=src\machine\z80fmly.h
# End Source File
# End Group
# Begin Group "Sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\sound\2151intf.c
# End Source File
# Begin Source File

SOURCE=src\sound\2151intf.h
# End Source File
# Begin Source File

SOURCE=src\sound\5220intf.c
# End Source File
# Begin Source File

SOURCE=src\sound\5220intf.h
# End Source File
# Begin Source File

SOURCE=src\sound\adpcm.c
# End Source File
# Begin Source File

SOURCE=src\sound\adpcm.h
# End Source File
# Begin Source File

SOURCE=src\sound\ay8910.c
# End Source File
# Begin Source File

SOURCE=src\sound\ay8910.h
# End Source File
# Begin Source File

SOURCE=src\sound\bsmt2000.c
# End Source File
# Begin Source File

SOURCE=src\sound\bsmt2000.h
# End Source File
# Begin Source File

SOURCE=src\sound\dac.c
# End Source File
# Begin Source File

SOURCE=src\sound\dac.h
# End Source File
# Begin Source File

SOURCE=src\sound\fm.c
# End Source File
# Begin Source File

SOURCE=src\sound\fm.h
# End Source File
# Begin Source File

SOURCE=src\sound\hc55516.c
# End Source File
# Begin Source File

SOURCE=src\sound\hc55516.h
# End Source File
# Begin Source File

SOURCE=src\sound\mixer.c
# End Source File
# Begin Source File

SOURCE=src\sound\mixer.h
# End Source File
# Begin Source File

SOURCE=src\sound\msm5205.c
# End Source File
# Begin Source File

SOURCE=src\sound\msm5205.h
# End Source File
# Begin Source File

SOURCE=src\sound\samples.c
# End Source File
# Begin Source File

SOURCE=src\sound\samples.h
# End Source File
# Begin Source File

SOURCE=src\sound\streams.c
# End Source File
# Begin Source File

SOURCE=src\sound\streams.h
# End Source File
# Begin Source File

SOURCE=src\sound\tms5220.c
# End Source File
# Begin Source File

SOURCE=src\sound\tms5220.h
# End Source File
# Begin Source File

SOURCE=src\sound\votrax.c
# End Source File
# Begin Source File

SOURCE=src\sound\ym2151.c
# End Source File
# Begin Source File

SOURCE=src\sound\ym2151.h
# End Source File
# End Group
# Begin Group "VidHrdw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\vidhrdw\avgdvg.c
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\avgdvg.h
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\crtc6845.c
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\crtc6845.h
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\generic.c
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\generic.h
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\tms9928a.c
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\tms9928a.h
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\vector.c
# End Source File
# Begin Source File

SOURCE=src\vidhrdw\vector.h
# End Source File
# End Group
# Begin Source File

SOURCE=src\artwork.c
# End Source File
# Begin Source File

SOURCE=src\artwork.h
# End Source File
# Begin Source File

SOURCE=src\audit.c
# End Source File
# Begin Source File

SOURCE=src\audit.h
# End Source File
# Begin Source File

SOURCE=src\cheat.c
# End Source File
# Begin Source File

SOURCE=src\cheat.h
# End Source File
# Begin Source File

SOURCE=src\common.c
# End Source File
# Begin Source File

SOURCE=src\common.h
# End Source File
# Begin Source File

SOURCE=src\cpuintrf.c
# End Source File
# Begin Source File

SOURCE=src\cpuintrf.h
# End Source File
# Begin Source File

SOURCE=src\datafile.c
# End Source File
# Begin Source File

SOURCE=src\datafile.h
# End Source File
# Begin Source File

SOURCE=src\drawgfx.c
# End Source File
# Begin Source File

SOURCE=src\drawgfx.h
# End Source File
# Begin Source File

SOURCE=src\driver.h
# End Source File
# Begin Source File

SOURCE=src\sound\filter.c
# End Source File
# Begin Source File

SOURCE=src\sound\filter.h
# End Source File
# Begin Source File

SOURCE=src\gfxobj.h
# End Source File
# Begin Source File

SOURCE=src\hiscore.c
# End Source File
# Begin Source File

SOURCE=src\hiscore.h
# End Source File
# Begin Source File

SOURCE=src\info.c
# End Source File
# Begin Source File

SOURCE=src\info.h
# End Source File
# Begin Source File

SOURCE=src\inptport.c
# End Source File
# Begin Source File

SOURCE=src\inptport.h
# End Source File
# Begin Source File

SOURCE=src\input.c
# End Source File
# Begin Source File

SOURCE=src\input.h
# End Source File
# Begin Source File

SOURCE=src\legacy.h
# End Source File
# Begin Source File

SOURCE=src\mame.c
# End Source File
# Begin Source File

SOURCE=src\mame.h
# End Source File
# Begin Source File

SOURCE=src\mamedbg.c
# End Source File
# Begin Source File

SOURCE=src\mamedbg.h
# End Source File
# Begin Source File

SOURCE=src\memory.c
# End Source File
# Begin Source File

SOURCE=src\memory.h
# End Source File
# Begin Source File

SOURCE=src\network.c
# End Source File
# Begin Source File

SOURCE=src\network.h
# End Source File
# Begin Source File

SOURCE=src\osdepend.h
# End Source File
# Begin Source File

SOURCE=src\palette.c
# End Source File
# Begin Source File

SOURCE=src\palette.h
# End Source File
# Begin Source File

SOURCE=src\png.c
# End Source File
# Begin Source File

SOURCE=src\png.h
# End Source File
# Begin Source File

SOURCE=src\profiler.c
# End Source File
# Begin Source File

SOURCE=src\profiler.h
# End Source File
# Begin Source File

SOURCE=src\sndintrf.c
# End Source File
# Begin Source File

SOURCE=src\sndintrf.h
# End Source File
# Begin Source File

SOURCE=src\sprite.h
# End Source File
# Begin Source File

SOURCE=src\state.c
# End Source File
# Begin Source File

SOURCE=src\state.h
# End Source File
# Begin Source File

SOURCE=src\tilemap.c
# End Source File
# Begin Source File

SOURCE=src\tilemap.h
# End Source File
# Begin Source File

SOURCE=src\timer.c
# End Source File
# Begin Source File

SOURCE=src\timer.h
# End Source File
# Begin Source File

SOURCE=src\ui_text.c
# End Source File
# Begin Source File

SOURCE=src\ui_text.h
# End Source File
# Begin Source File

SOURCE=src\unzip.c
# End Source File
# Begin Source File

SOURCE=src\unzip.h
# End Source File
# Begin Source File

SOURCE=src\usrintrf.c
# End Source File
# Begin Source File

SOURCE=src\usrintrf.h
# End Source File
# Begin Source File

SOURCE=src\version.c
# End Source File
# Begin Source File

SOURCE=src\window.c
# End Source File
# Begin Source File

SOURCE=src\window.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\windows\asmblit.asm

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\obj/PinMAMEVC/windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build
IntDir=.\obj/PinMAMEVCd/Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\blit.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\blit.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\config.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\dirty.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\fileio.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\fronthlp.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\input.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\misc.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\misc.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\osd_cpu.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\osinline.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\rc.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\rc.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\snprintf.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\sound.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\ticker.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\ticker.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\video.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\video.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\window.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\window.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\winmain.c

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=src\windows\winprefix.h

!IF  "$(CFG)" == "PinMAME - Win32 Release"

# PROP Intermediate_Dir "obj/PinMAMEVC/windows"

!ELSEIF  "$(CFG)" == "PinMAME - Win32 Debug"

# PROP Intermediate_Dir "obj/PinMAMEVCd/Windows"

!ENDIF 

# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=src\zlib\zlib.h
# End Source File
# End Group
# Begin Group "PinMAME"

# PROP Default_Filter ""
# Begin Group "sims"

# PROP Default_Filter ""
# Begin Group "s11"

# PROP Default_Filter ""
# Begin Group "full_s11"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\wpc\sims\s11\full\dd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\s11\full\milln.c
# End Source File
# End Group
# Begin Group "prelim_s11"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\wpc\sims\s11\prelim\eatpm.c
# End Source File
# End Group
# End Group
# Begin Group "wpc"

# PROP Default_Filter ""
# Begin Group "full_wpc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\afm.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\bop.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\br.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\cftbl.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\dd_wpc.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\drac.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\fh.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\ft.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\gi.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\gw.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\hd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\hurr.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\ij.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\jd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\mm.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\ngg.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\pz.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\rs.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\ss.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\sttng.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\t2.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\taf.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\tom.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\tz.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\wcs.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\full\ww.c
# End Source File
# End Group
# Begin Group "prelim_wpc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\cc.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\congo.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\corv.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\cp.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\cv.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\dh.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\dm.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\dw.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\fs.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\i500.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\jb.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\jm.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\jy.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\mb.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\nbaf.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\nf.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\pop.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\sc.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\totan.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\ts.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sims\wpc\prelim\wd.c
# End Source File
# End Group
# End Group
# Begin Group "s7"

# PROP Default_Filter ""
# Begin Group "full_s7"

# PROP Default_Filter ""
# Begin Source File

SOURCE=src\wpc\sims\s7\full\tmfnt.c
# End Source File
# End Group
# Begin Group "prelim_s7"

# PROP Default_Filter ""
# End Group
# End Group
# End Group
# Begin Source File

SOURCE=src\wpc\by35.c
# End Source File
# Begin Source File

SOURCE=src\wpc\by35.h
# End Source File
# Begin Source File

SOURCE=src\wpc\by35games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\by35snd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\by35snd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by6803.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by6803.h
# End Source File
# Begin Source File

SOURCE=src\wpc\byvidpin.c
# End Source File
# Begin Source File

SOURCE=src\wpc\byvidpin.h
# End Source File
# Begin Source File

SOURCE=src\wpc\core.c
# End Source File
# Begin Source File

SOURCE=src\wpc\core.h
# End Source File
# Begin Source File

SOURCE=src\wpc\dcs.c
# End Source File
# Begin Source File

SOURCE=src\wpc\dcs.h
# End Source File
# Begin Source File

SOURCE=src\wpc\dedmd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\dedmd.h
# End Source File
# Begin Source File

SOURCE=src\wpc\degames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\desound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\desound.h
# End Source File
# Begin Source File

SOURCE=src\wpc\driver.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gen.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gp.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gp.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gpgames.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3.h
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3dmd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3dmd.h
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3sound.c
# End Source File
# Begin Source File

SOURCE=src\wpc\gts3sound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts80.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts80.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts80games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts80s.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts80s.h
# End Source File
# Begin Source File

SOURCE=src\wpc\hnk.c
# End Source File
# Begin Source File

SOURCE=src\wpc\hnk.h
# End Source File
# Begin Source File

SOURCE=src\wpc\hnkgames.c
# End Source File
# Begin Source File

SOURCE=src\wpc\mech.c
# End Source File
# Begin Source File

SOURCE=src\wpc\mech.h
# End Source File
# Begin Source File

SOURCE=.\src\pinmame.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s11.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s11.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s11csoun.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s11csoun.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s11games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s3games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s4.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s4.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s4games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s6.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s6.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s67s.c
# End Source File
# Begin Source File

SOURCE=src\wpc\S67s.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s6games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s7.c
# End Source File
# Begin Source File

SOURCE=src\wpc\s7.h
# End Source File
# Begin Source File

SOURCE=src\wpc\s7games.c
# End Source File
# Begin Source File

SOURCE=src\wpc\se.c
# End Source File
# Begin Source File

SOURCE=src\wpc\se.h
# End Source File
# Begin Source File

SOURCE=src\wpc\segames.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sesound.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sesound.h
# End Source File
# Begin Source File

SOURCE=src\wpc\sim.c
# End Source File
# Begin Source File

SOURCE=src\wpc\sim.h
# End Source File
# Begin Source File

SOURCE=src\wpc\snd_cmd.c
# End Source File
# Begin Source File

SOURCE=src\wpc\snd_cmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sndbrd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sndbrd.h
# End Source File
# Begin Source File

SOURCE=src\wpc\vpintf.c
# End Source File
# Begin Source File

SOURCE=src\wpc\vpintf.h
# End Source File
# Begin Source File

SOURCE=src\wpc\wpc.c
# End Source File
# Begin Source File

SOURCE=src\wpc\wpc.h
# End Source File
# Begin Source File

SOURCE=src\wpc\wpcgames.c
# End Source File
# Begin Source File

SOURCE=src\wpc\wpcsam.c
# End Source File
# Begin Source File

SOURCE=src\wpc\wpcsam.h
# End Source File
# Begin Source File

SOURCE=src\wpc\wpcsound.c
# End Source File
# Begin Source File

SOURCE=src\wpc\wpcsound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zac.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zac.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zacgames.c
# End Source File
# End Group
# Begin Group "VC"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vc\dirent.c
# End Source File
# Begin Source File

SOURCE=.\src\vc\dirent.h
# End Source File
# Begin Source File

SOURCE=.\src\vc\unistd.h
# End Source File
# End Group
# End Group
# End Target
# End Project
