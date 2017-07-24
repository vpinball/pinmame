# Microsoft Developer Studio Project File - Name="InstallVPinMAME" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=InstallVPinMAME - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "InstallVPinMAME.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "InstallVPinMAME.mak" CFG="InstallVPinMAME - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "InstallVPinMAME - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "InstallVPinMAME - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "InstallVPinMAME - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\obj\VC60\InstallVPinMAME\Win32\Release"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\obj\VC60\InstallVPinMAME\Win32\Release"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "$(OUTDIR)\Intermediate\MIDL" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /out "$(OUTDIR)\Intermediate\MIDL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /version:4.0 /subsystem:windows /machine:I386 /out:".\obj\VC60\InstallVPinMAME\Win32\Release\InstallVPinMAME_VC60.exe"
# Begin Custom Build - Copying to root...
ProjDir=.
TargetPath=.\obj\VC60\InstallVPinMAME\Win32\Release\InstallVPinMAME_VC60.exe
TargetName=InstallVPinMAME_VC60
InputPath=.\obj\VC60\InstallVPinMAME\Win32\Release\InstallVPinMAME_VC60.exe
SOURCE="$(InputPath)"

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(TargetPath)" "$(ProjDir)"

# End Custom Build

!ELSEIF  "$(CFG)" == "InstallVPinMAME - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\obj\VC60\InstallVPinMAME\Win32\Debug"
# PROP BASE Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\obj\VC60\InstallVPinMAME\Win32\Debug"
# PROP Intermediate_Dir "$(OUTDIR)\Intermediate"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "$(OUTDIR)\Intermediate\MIDL" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /out "$(OUTDIR)\Intermediate\MIDL" /win32
# SUBTRACT MTL /mktyplib203
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib version.lib /nologo /version:4.0 /subsystem:windows /debug /machine:I386 /out:".\obj\VC60\InstallVPinMAME\Win32\Debug\InstallVPinMAME_VC60d.exe" /pdbtype:sept
# Begin Custom Build - Copying to root...
ProjDir=.
TargetDir=.\obj\VC60\InstallVPinMAME\Win32\Debug
TargetPath=.\obj\VC60\InstallVPinMAME\Win32\Debug\InstallVPinMAME_VC60d.exe
TargetName=InstallVPinMAME_VC60d
InputPath=.\obj\VC60\InstallVPinMAME\Win32\Debug\InstallVPinMAME_VC60d.exe
SOURCE="$(InputPath)"

"$(ProjDir)\$(TargetName).exe" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(TargetPath)" "$(ProjDir)" 
	copy "$(TargetDir)\$(TargetName).pdb" "$(ProjDir)" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "InstallVPinMAME - Win32 Release"
# Name "InstallVPinMAME - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "VPinMAME"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.idl
# ADD BASE MTL /tlb "VPinMAME.tlb" /h "VPinMAME_h.h"
# ADD MTL /tlb "VPinMAME.tlb" /h "VPinMAME_h.h"
# End Source File
# End Group
# Begin Source File

SOURCE=.\src\instvpm\Globals.cpp
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\InstallVPinMAME.cpp
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\InstallVPinMAME.rc
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\TestVPinMAME.cpp
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\Utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\src\instvpm\Globals.h
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\TestVPinMAME.h
# End Source File
# Begin Source File

SOURCE=.\src\instvpm\Utils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\src\instvpm\Res\InstallVPinMAME.ICO
# End Source File
# End Group
# End Target
# End Project
