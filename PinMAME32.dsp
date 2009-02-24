# Microsoft Developer Studio Project File - Name="PinMAME32" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PinMAME32 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "PinMAME32.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PinMAME32.mak" CFG="PinMAME32 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PinMAME32 - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PinMAME32 - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "PinMAME32 - Win32 Release with MAME Debugger" (based on "Win32 (x86) Application")
!MESSAGE "PinMAME32 - Win32 Debug with MAME Debugger" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PinMAME32 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\obj\VC60\PinMAME32\Win32\Release"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\obj\VC60\PinMAME32\Win32\Release"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "src" /I "src\wpc" /I "src\windows" /I "src\vc" /I "src\zlib" /I "src\htmlhelp\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "LSB_FIRST" /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "ZLIB_DLL" /D MAMEVER=7300 /D "PINMAME" /D MAME32NAME=\"PinMAME32\" /D WINUI=1 /D SUFFIX=32 /D _WIN32_IE=0x0500 /D _WIN32_WINNT=0x0400 /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d _WIN32_IE=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib zlibstatmt.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:".\obj\VC60\PinMAME32\Win32\Release\PinMAME32_VC60.exe" /libpath:"src/htmlhelp/lib" /libpath:"zlib"
# Begin Custom Build - Copying to root and generating gamelist.txt...
ProjDir=.
TargetPath=.\obj\VC60\PinMAME32\Win32\Release\PinMAME32_VC60.exe
TargetName=PinMAME32_VC60
InputPath=.\obj\VC60\PinMAME32\Win32\Release\PinMAME32_VC60.exe
SOURCE="$(InputPath)"

BuildCmds= \
	copy "$(TargetPath)" "$(ProjDir)" \
	"$(ProjDir)\$(TargetName).exe" -gamelist -noclones -sortname >"$(ProjDir)\$(TargetName)_gamelist.txt" \
	

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\$(TargetName)_gamelist.txt" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\obj\VC60\PinMAME32\Win32\Debug"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\obj\VC60\PinMAME32\Win32\Debug"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "src" /I "src\wpc" /I "src\windows" /I "src\vc" /I "src\zlib" /I "src\htmlhelp\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "LSB_FIRST" /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "ZLIB_DLL" /D MAMEVER=7300 /D "PINMAME" /D MAME32NAME=\"PinMAME32\" /D WINUI=1 /D SUFFIX=32 /D _WIN32_IE=0x0500 /D _WIN32_WINNT=0x0400 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d _WIN32_IE=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib zlibstatmtd.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\obj\VC60\PinMAME32\Win32\Debug\PinMAME32_VC60vcd.exe" /pdbtype:sept /libpath:"src/htmlhelp/lib" /libpath:"zlib"
# Begin Custom Build - Copying to root...
ProjDir=.
TargetDir=.\obj\VC60\PinMAME32\Win32\Debug
TargetPath=.\obj\VC60\PinMAME32\Win32\Debug\PinMAME32_VC60vcd.exe
TargetName=PinMAME32_VC60vcd
InputPath=.\obj\VC60\PinMAME32\Win32\Debug\PinMAME32_VC60vcd.exe
SOURCE="$(InputPath)"

BuildCmds= \
	copy "$(TargetPath)" "$(ProjDir)" \
	copy "$(TargetDir)\$(TargetName).pdb" "$(ProjDir)" \
	

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\$(TargetName).pdb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Release with MAME Debugger"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\obj\VC60\PinMAME32\Win32\ReleaseMD"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\obj\VC60\PinMAME32\Win32\ReleaseMD"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /O2 /I "src" /I "src\wpc" /I "src\windows" /I "src\vc" /I "src\zlib" /I "src\htmlhelp\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "LSB_FIRST" /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "ZLIB_DLL" /D MAMEVER=7300 /D "PINMAME" /D MAME32NAME=\"PinMAME32\" /D WINUI=1 /D SUFFIX=32 /D _WIN32_IE=0x0500 /D _WIN32_WINNT=0x0400 /D "MAME_DEBUG" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d _WIN32_IE=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib zlibstatmt.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /machine:I386 /out:".\obj\VC60\PinMAME32\Win32\ReleaseMD\PinMAME32_VC60md.exe" /libpath:"src/htmlhelp/lib" /libpath:"zlib"
# Begin Custom Build - Copying to root...
ProjDir=.
TargetPath=.\obj\VC60\PinMAME32\Win32\ReleaseMD\PinMAME32_VC60md.exe
TargetName=PinMAME32_VC60md
InputPath=.\obj\VC60\PinMAME32\Win32\ReleaseMD\PinMAME32_VC60md.exe
SOURCE="$(InputPath)"

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(TargetPath)" "$(ProjDir)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug with MAME Debugger"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\obj\VC60\PinMAME32\Win32\DebugMD"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\obj\VC60\PinMAME32\Win32\DebugMD"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "src" /I "src\wpc" /I "src\windows" /I "src\vc" /I "src\zlib" /I "src\htmlhelp\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "LSB_FIRST" /D CLIB_DECL=__cdecl /D DECL_SPEC=__cdecl /D inline=__inline /D __inline__=__inline /D INLINE=__inline /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /D "NONAMELESSUNION" /D "ZLIB_DLL" /D MAMEVER=7300 /D "PINMAME" /D MAME32NAME=\"PinMAME32\" /D WINUI=1 /D SUFFIX=32 /D _WIN32_IE=0x0500 /D _WIN32_WINNT=0x0400 /D "MAME_DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d _WIN32_IE=0x0400
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib comdlg32.lib advapi32.lib shell32.lib zlibstatmtd.lib winmm.lib dxguid.lib ddraw.lib dinput.lib dsound.lib comctl32.lib htmlhelp.lib /nologo /subsystem:windows /debug /machine:I386 /out:".\obj\VC60\PinMAME32\Win32\DebugMD\PinMAME32_VC60vcmd.exe" /pdbtype:sept /libpath:"src/htmlhelp/lib" /libpath:"zlib"
# Begin Custom Build - Copying to root...
ProjDir=.
TargetDir=.\obj\VC60\PinMAME32\Win32\DebugMD
TargetPath=.\obj\VC60\PinMAME32\Win32\DebugMD\PinMAME32_VC60vcmd.exe
TargetName=PinMAME32_VC60vcmd
InputPath=.\obj\VC60\PinMAME32\Win32\DebugMD\PinMAME32_VC60vcmd.exe
SOURCE="$(InputPath)"

BuildCmds= \
	copy "$(TargetPath)" "$(ProjDir)" \
	copy "$(TargetDir)\$(TargetName).pdb" "$(ProjDir)" \
	

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(ProjDir)\$(TargetName).pdb" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "PinMAME32 - Win32 Release"
# Name "PinMAME32 - Win32 Debug"
# Name "PinMAME32 - Win32 Release with MAME Debugger"
# Name "PinMAME32 - Win32 Debug with MAME Debugger"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Group "MAME"

# PROP Default_Filter ""
# Begin Group "CPU"

# PROP Default_Filter ""
# Begin Group "ADSP2100"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100ops.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.h
# End Source File
# End Group
# Begin Group "M6809"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6809\6809dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809ops.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809tbl.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.h
# End Source File
# End Group
# Begin Group "M6800"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6800\6800dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\6800ops.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\6800tbl.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.h
# End Source File
# End Group
# Begin Group "Z80"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\z80\z80.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80daa.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80dasm.h
# End Source File
# End Group
# Begin Group "M6502"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6502\6502dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\ill02.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6502.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6502.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m65ce02.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\ops02.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\opsc02.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\opsn2a03.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\t6502.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\t6510.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\t65c02.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\t65sc02.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "M68000"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m68000\cpudefs.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68000.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68k.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kconf.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kcpu.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kcpu.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kdasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kmake.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kmame.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kmame.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kopac.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kopdm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kopnz.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kops.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kops.h
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
# Begin Group "I8085"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i8085\8085dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085cpu.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8085\i8085daa.h
# End Source File
# End Group
# Begin Group "I8039"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i8039\8039dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8039\i8039.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8039\i8039.h
# End Source File
# End Group
# Begin Group "I86"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i86\ea.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\host.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i186intf.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i188intf.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86intf.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\i86time.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\instr186.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\instr186.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\instr86.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\instr86.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\modrm.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\table186.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\table86.h
# End Source File
# End Group
# Begin Group "I4004"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i4004\4004dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i4004\i4004.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i4004\i4004.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i4004\i4004cpu.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i4004\i4004daa.h
# End Source File
# End Group
# Begin Group "PPS4"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\pps4\pps4.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\pps4\pps4.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\pps4\pps4cpu.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\pps4\pps4dasm.c
# End Source File
# End Group
# Begin Group "SCAMP"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\scamp\scamp.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\scamp\scamp.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\scamp\scampdsm.c
# End Source File
# End Group
# Begin Group "I8051"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i8051\8051dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8051\i8051.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8051\i8051.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i8051\i8051ops.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "TMS7000"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\tms7000\7000dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms7000\tms7000.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms7000\tms7000.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms7000\tms70op.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms7000\tms70tb.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "AT91"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\at91\at91.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\at91\at91.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\at91\at91dasm.c
# End Source File
# End Group
# Begin Group "ARM7"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7core.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7core.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\arm7\arm7exec.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Group "CDP1802"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\cdp1802\1802dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\cdp1802\cdp1802.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\cdp1802\cdp1802.h
# End Source File
# End Group
# Begin Group "TMS9900"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\tms9900\9900dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\9900stat.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\99xxcore.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\99xxstat.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9900.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9900.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9980a.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\tms9900\tms9995.c
# End Source File
# End Group
# End Group
# Begin Group "Machine"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\machine\4094.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\4094.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6522via.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6522via.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6530riot.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6530riot.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6532riot.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6532riot.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6821pia.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6821pia.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\mathbox.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\mathbox.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\pic8259.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\pic8259.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\ticket.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\ticket.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.h
# End Source File
# End Group
# Begin Group "Sound"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sound\2151intf.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\2151intf.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\3812intf.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\3812intf.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\5220intf.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\5220intf.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\adpcm.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\adpcm.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\ay8910.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\ay8910.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\bsmt2000.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\bsmt2000.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_dev.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_flt.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_inp.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_mth.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_out.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\disc_wav.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\discrete.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\discrete.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\fmopl.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\fmopl.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\m114s.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\m114s.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\s14001a.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\s14001a.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76477.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76477.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76496.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\sn76496.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\sp0250.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\sp0250.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms320av120.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms320av120.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220r.c
# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\sound\votrax.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\votrax.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\vtxsmpls.inc
# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2151.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2151.h
# End Source File
# End Group
# Begin Group "VidHrdw"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vidhrdw\avgdvg.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\avgdvg.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\crtc6845.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\crtc6845.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\tms9928a.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\tms9928a.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\artwork.c
# End Source File
# Begin Source File

SOURCE=.\src\artwork.h
# End Source File
# Begin Source File

SOURCE=.\src\audit.c
# End Source File
# Begin Source File

SOURCE=.\src\audit.h
# End Source File
# Begin Source File

SOURCE=.\src\cheat.c
# End Source File
# Begin Source File

SOURCE=.\src\cheat.h
# End Source File
# Begin Source File

SOURCE=.\src\common.c
# End Source File
# Begin Source File

SOURCE=.\src\common.h
# End Source File
# Begin Source File

SOURCE=.\src\config.c
# End Source File
# Begin Source File

SOURCE=.\src\config.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuexec.c
# End Source File
# Begin Source File

SOURCE=.\src\cpuexec.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuint.c
# End Source File
# Begin Source File

SOURCE=.\src\cpuint.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\datafile.c
# End Source File
# Begin Source File

SOURCE=.\src\datafile.h
# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.c
# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.h
# End Source File
# Begin Source File

SOURCE=.\src\driver.h
# End Source File
# Begin Source File

SOURCE=.\src\fileio.c
# End Source File
# Begin Source File

SOURCE=.\src\fileio.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\filter.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\filter.h
# End Source File
# Begin Source File

SOURCE=.\src\gfxobj.h
# End Source File
# Begin Source File

SOURCE=.\src\harddisk.c
# End Source File
# Begin Source File

SOURCE=.\src\harddisk.h
# End Source File
# Begin Source File

SOURCE=.\src\hash.c
# End Source File
# Begin Source File

SOURCE=.\src\hash.h
# End Source File
# Begin Source File

SOURCE=.\src\hiscore.c
# End Source File
# Begin Source File

SOURCE=.\src\hiscore.h
# End Source File
# Begin Source File

SOURCE=.\src\info.c
# End Source File
# Begin Source File

SOURCE=.\src\info.h
# End Source File
# Begin Source File

SOURCE=.\src\inptport.c
# End Source File
# Begin Source File

SOURCE=.\src\inptport.h
# End Source File
# Begin Source File

SOURCE=.\src\input.c
# End Source File
# Begin Source File

SOURCE=.\src\input.h
# End Source File
# Begin Source File

SOURCE=.\src\legacy.h
# End Source File
# Begin Source File

SOURCE=.\src\mame.c
# End Source File
# Begin Source File

SOURCE=.\src\mame.h
# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.c
# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.h
# End Source File
# Begin Source File

SOURCE=.\src\md5.c
# End Source File
# Begin Source File

SOURCE=.\src\md5.h
# End Source File
# Begin Source File

SOURCE=.\src\memory.c
# End Source File
# Begin Source File

SOURCE=.\src\memory.h
# End Source File
# Begin Source File

SOURCE=.\src\multidef.h
# End Source File
# Begin Source File

SOURCE=.\src\osdepend.h
# End Source File
# Begin Source File

SOURCE=.\src\palette.c
# End Source File
# Begin Source File

SOURCE=.\src\palette.h
# End Source File
# Begin Source File

SOURCE=.\src\png.c
# End Source File
# Begin Source File

SOURCE=.\src\png.h
# End Source File
# Begin Source File

SOURCE=.\src\profiler.c
# End Source File
# Begin Source File

SOURCE=.\src\profiler.h
# End Source File
# Begin Source File

SOURCE=.\src\sha1.c
# End Source File
# Begin Source File

SOURCE=.\src\sha1.h
# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\sprite.h
# End Source File
# Begin Source File

SOURCE=.\src\state.c
# End Source File
# Begin Source File

SOURCE=.\src\state.h
# End Source File
# Begin Source File

SOURCE=.\src\tilemap.c
# End Source File
# Begin Source File

SOURCE=.\src\tilemap.h
# End Source File
# Begin Source File

SOURCE=.\src\timer.c
# End Source File
# Begin Source File

SOURCE=.\src\timer.h
# End Source File
# Begin Source File

SOURCE=.\src\ui_text.c
# End Source File
# Begin Source File

SOURCE=.\src\ui_text.h
# End Source File
# Begin Source File

SOURCE=.\src\unzip.c
# End Source File
# Begin Source File

SOURCE=.\src\unzip.h
# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\version.c
# End Source File
# Begin Source File

SOURCE=.\src\window.c
# End Source File
# Begin Source File

SOURCE=.\src\window.h
# End Source File
# End Group
# Begin Group "Windows"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\windows\asmblit.asm

!IF  "$(CFG)" == "PinMAME32 - Win32 Release"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\Release\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\Release\Intermediate\Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\Debug\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\Debug\Intermediate\Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Release with MAME Debugger"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\ReleaseMD\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\ReleaseMD\Intermediate\Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug with MAME Debugger"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\DebugMD\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\DebugMD\Intermediate\Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\asmtile.asm

!IF  "$(CFG)" == "PinMAME32 - Win32 Release"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\Release\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\Release\Intermediate\Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\Debug\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\Debug\Intermediate\Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Release with MAME Debugger"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\ReleaseMD\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\ReleaseMD\Intermediate\Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ELSEIF  "$(CFG)" == "PinMAME32 - Win32 Debug with MAME Debugger"

# PROP Intermediate_Dir ".\obj\VC60\PinMAME32\Win32\DebugMD\Intermediate\Windows"
# PROP Ignore_Default_Tool 1
# Begin Custom Build - Assembling $(InputPath)...
IntDir=.\obj\VC60\PinMAME32\Win32\DebugMD\Intermediate\Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o "$(IntDir)\$(InputName).obj" "$(InputPath)"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\blit.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\blit.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\config.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\d3d_extra.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\dirty.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\fileio.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\fronthlp.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\input.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\misc.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\misc.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\osd_cpu.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\osinline.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\pattern.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\rc.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\rc.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\snprintf.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP BASE Exclude_From_Build 1
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\windows\sound.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\ticker.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\video.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\video.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\wind3d.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\wind3d.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\wind3dfx.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\wind3dfx.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\winddraw.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\winddraw.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\window.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\window.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\winmain.c
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# Begin Source File

SOURCE=.\src\windows\winprefix.h
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate\Windows"
# End Source File
# End Group
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zlib.h
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

SOURCE=.\src\wpc\sims\s11\full\dd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\s11\full\milln.c
# End Source File
# End Group
# Begin Group "prelim_s11"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\sims\s11\prelim\eatpm.c
# End Source File
# End Group
# End Group
# Begin Group "wpc"

# PROP Default_Filter ""
# Begin Group "full_wpc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\afm.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\bop.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\br.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\cftbl.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\dd_wpc.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\drac.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\fh.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\ft.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\gi.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\gw.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\hd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\hurr.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\ij.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\jd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\mm.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\ngg.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\pz.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\rs.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\ss.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\sttng.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\t2.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\taf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\tom.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\tz.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\wcs.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\full\ww.c
# End Source File
# End Group
# Begin Group "prelim_wpc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\cc.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\congo.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\corv.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\cp.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\cv.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\dh.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\dm.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\dw.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\fs.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\i500.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\jb.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\jm.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\jy.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\mb.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\nbaf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\nf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\pop.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\sc.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\totan.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\ts.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\wpc\prelim\wd.c
# End Source File
# End Group
# End Group
# Begin Group "s7"

# PROP Default_Filter ""
# Begin Group "full_s7"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\sims\s7\full\bk.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\s7\full\tmfnt.c
# End Source File
# End Group
# Begin Group "prelim_s7"

# PROP Default_Filter ""
# End Group
# End Group
# Begin Group "se"

# PROP Default_Filter ""
# Begin Group "prelim_se"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\sims\se\prelim\elvis.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sims\se\prelim\monopoly.c
# End Source File
# End Group
# End Group
# End Group
# Begin Source File

SOURCE=.\src\wpc\allied.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvg.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvg.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvgdmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvgdmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvggames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvgs.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\alvgs.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atari.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atari.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atarigames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atarisnd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\bingo.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\bowarrow.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\bowlgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by35.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by35.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by35games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by35snd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by35snd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by6803.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by6803.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by6803games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\by68701.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidpin.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidpin.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcom.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcom.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcoms.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcoms.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\core.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\core.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\dedmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\dedmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\degames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\desound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\desound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\driver.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\flicker.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gen.h
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

SOURCE=.\src\wpc\gpsnd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gpsnd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts1.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts1.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts1games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3dmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3dmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3games.c
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

SOURCE=.\src\wpc\hnkgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\hnks.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\hnks.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\inder.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\inder.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\indergames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\jp.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\jp.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\jpgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\jvh.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\kissproto.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\ltd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\ltd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\ltdgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mech.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mech.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mrgame.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mrgame.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mrgamegames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\nsm.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\nuova.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\peyper.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\peyper.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\peypergames.c
# End Source File
# Begin Source File

SOURCE=.\src\pinmame.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\play.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\play.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\playgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\rotation.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\rowamet.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s3games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s4.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s4.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s4games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\se.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\se.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\segames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sim.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sim.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sleic.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sleic.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sleicgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\snd_cmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\snd_cmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sndbrd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sndbrd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\spinb.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\spinb.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\spinbgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\stgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\stsnd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\stsnd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\taito.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\taito.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\taitogames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\taitos.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\taitos.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\techno.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vpintf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vpintf.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wico.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wmssnd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wmssnd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpc.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpc.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsam.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsam.h
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
# Begin Source File

SOURCE=.\src\wpc\zacproto.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zacsnd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zacsnd.h
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
# Begin Group "ui"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ui\audit32.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\audit32.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\bitmask.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\bitmask.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\columnedit.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\columnedit.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\datamap.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\datamap.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\dialogs.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\dijoystick.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\dijoystick.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\directdraw.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\directdraw.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\directinput.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\directinput.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\directories.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\directories.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\dxdecode.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\dxdecode.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\file.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\help.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\help.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\history.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\history.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\layout.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\m32main.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\m32util.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\m32util.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\mame32.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\options.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\options.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\pinmame32.rc
# End Source File
# Begin Source File

SOURCE=.\src\ui\properties.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\properties.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\screenshot.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\screenshot.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\splitters.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\splitters.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\treeview.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\treeview.h
# End Source File
# Begin Source File

SOURCE=.\src\ui\win32ui.c
# End Source File
# Begin Source File

SOURCE=.\src\ui\win32ui.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\ui\res\about.bmp
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\checkmark.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\cpu.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\display.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldavail.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\folder.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldmanu.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldopen.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldsrc.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldunav.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\foldyear.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\harddisk.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\header_down.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\header_up.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\joystick.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\keyboard.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\manufact.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\nonwork.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\pinmame32.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\pinmame32.manifest
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\samples.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\source.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\splith.cur
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\win_clone.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\win_noro.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\win_redx.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\win_roms.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\win_unkn.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\working.ico
# End Source File
# Begin Source File

SOURCE=.\src\ui\res\year.ico
# End Source File
# End Group
# End Target
# End Project
