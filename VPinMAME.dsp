# Microsoft Developer Studio Project File - Name="Visual PinMame" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Visual PinMame - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "VPinMAME.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "VPinMAME.mak" CFG="Visual PinMame - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Visual PinMame - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Visual PinMame - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "obj/VPinMAME/Debug"
# PROP Intermediate_Dir "obj/VPinMAME/Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /G5 /MTd /W3 /Gm /ZI /Od /I "src" /I "src\zlib" /I "src\wpc" /I "src\vc" /I "src\win32c" /I "src\win32com" /I "src\win32com\autogen" /I "src\windows" /D "_DEBUG" /D "MAME_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D PROCESSOR_ARCHITECTURE=x86 /D "LSB_FIRST" /D "ZLIB_DLL" /D "VPINMAME" /D "PINMAME" /D MAMEVER=5900 /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /YX /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD MTL /out ".\src\win32com\autogen"
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i ".\src\win32com\autogen" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib shell32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib zlibstatmtd.lib winmm.lib dsound.lib version.lib dxguid.lib ddraw.lib dinput.lib /nologo /version:4.0 /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"zlib"
# Begin Custom Build - Performing registration
IntDir=.\obj/VPinMAME/Debug
TargetPath=.\obj\VPinMAME\Debug\VPinMAME.dll
InputPath=.\obj\VPinMAME\Debug\VPinMAME.dll
SOURCE="$(InputPath)"

"$(IntDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(IntDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/VPinMAME/Release"
# PROP Intermediate_Dir "obj/VPinMAME/Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G5 /MT /w /W0 /O2 /I "src" /I "src\zlib" /I "src\wpc" /I "src\vc" /I "src\win32c" /I "src\win32com" /I "src\win32com\autogen" /I "src\windows" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D PROCESSOR_ARCHITECTURE=x86 /D "LSB_FIRST" /D "ZLIB_DLL" /D "VPINMAME" /D "PINMAME" /D MAMEVER=5900 /D DIRECTINPUT_VERSION=0x0500 /D DIRECTDRAW_VERSION=0x0300 /YX /FD /c
# ADD MTL /out ".\src\win32com\autogen"
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".\src\win32com\autogen" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib shell32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib zlibstatmt.lib winmm.lib dsound.lib version.lib dxguid.lib ddraw.lib dinput.lib /nologo /version:20.0 /subsystem:windows /dll /machine:I386 /libpath:"zlib"
# Begin Custom Build - Copying and performing registration
IntDir=.\obj/VPinMAME/Release
ProjDir=.
TargetPath=.\obj\VPinMAME\Release\VPinMAME.dll
TargetName=VPinMAME
InputPath=.\obj\VPinMAME\Release\VPinMAME.dll
SOURCE="$(InputPath)"

BuildCmds= \
	copy "$(TargetPath)" "$(ProjDir)\$(TargetName).dll" \
	regsvr32 /s /c "$(ProjDir)\$(TargetName).dll" \
	echo regsvr32 exec.time > "$(IntDir)\regsvr32.trg" \
	

"$(ProjDir)\$(TargetName).dll" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"$(IntDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# Begin Target

# Name "Visual PinMame - Win32 Debug"
# Name "Visual PinMame - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zlib.h
# End Source File
# End Group
# Begin Group "Win32COM"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32com\Controller.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\Controller.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerDisclaimerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerDisclaimerDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGame.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGame.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGames.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGames.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGameSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGameSettings.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRegkeys.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRom.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRom.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRoms.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRoms.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRun.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerSettings.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerSettings.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerSplashWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerSplashWnd.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.def
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.idl
# ADD MTL /tlb "VPinMAME.tlb" /h "VPinMAME.h" /iid "VPinMAME_i.c"
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.rc
# ADD BASE RSC /l 0x407 /i "src\win32com"
# SUBTRACT BASE RSC /i ".\src\win32com\autogen"
# ADD RSC /l 0x409 /i "src\win32com" /i "src\win32com\autogen"
# SUBTRACT RSC /i ".\src\win32com\autogen"
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEAboutDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEAboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEConfig.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEConfig.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMECP.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlgCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlgCtrl.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlgCtrls.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlgCtrls.h
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

SOURCE=.\src\wpc\sims\s7\full\tmfnt.c
# End Source File
# End Group
# Begin Group "prelim_s7"

# PROP Default_Filter ""
# End Group
# End Group
# End Group
# Begin Source File

SOURCE=.\src\wpc\atari.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atari.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\wpc\atarigames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\atarisnd.c
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

SOURCE=.\src\wpc\byvidgames.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidpin.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidpin.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcom.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\capcom.h
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

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3dmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3dmd.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Exclude_From_Build 1

!ENDIF 

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

SOURCE=.\src\wpc\hnk.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\hnk.h
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

SOURCE=.\src\wpc\mech.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mech.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\midgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\midway.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\midway.h
# End Source File
# Begin Source File

SOURCE=.\src\pinmame.h
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

SOURCE=.\src\wpc\stgames.c
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

SOURCE=.\src\wpc\vpintf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vpintf.h
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
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\wpc\zac.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\wpc\zacgames.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
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

SOURCE=.\src\cpu\m6502\ops02.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\opsc02.h
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
# Begin Group "I86"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\i86\ea.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\i86\host.h
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

SOURCE=.\src\cpu\i86\modrm.h
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
# End Group
# Begin Group "Machine"

# PROP Default_Filter ""
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

SOURCE=.\src\sound\fm.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.h
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

SOURCE=.\src\sound\samples.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\tms5220.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\votrax.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\votrax.h
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

SOURCE=.\src\network.c
# End Source File
# Begin Source File

SOURCE=.\src\network.h
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

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"
# Begin Custom Build
IntDir=.\obj/VPinMAME/Debug/Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"
# Begin Custom Build
IntDir=.\obj/VPinMAME/Release/Windows
InputPath=.\src\windows\asmblit.asm
InputName=asmblit

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\asmtile.asm

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"
# Begin Custom Build
IntDir=.\obj/VPinMAME/Debug/Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"
# Begin Custom Build
IntDir=.\obj/VPinMAME/Release/Windows
InputPath=.\src\windows\asmtile.asm
InputName=asmtile

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -f coff -o $(IntDir)\$(InputName).obj $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\blit.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\blit.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\dirty.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\fileio.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\fronthlp.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\input.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\misc.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\osinline.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\rc.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\snprintf.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\sound.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\ticker.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\ticker.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\video.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\winddraw.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\winddraw.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\window.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\windows\winprefix.h

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# PROP Intermediate_Dir "obj/VPinMAME/Debug/Windows"

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

# PROP Intermediate_Dir "obj/VPinMAME/Release/Windows"

!ENDIF 

# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\src\win32com\Controller.rgs
# End Source File
# Begin Source File

SOURCE=.\src\win32com\Res\VPinMAMELogo.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\Res\VPinMAMESplash.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\Res\VPinMAMESplash4.bmp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\WSHDlg.rgs
# End Source File
# End Group
# Begin Group "Included Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100ops.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\6800ops.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\6800tbl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809ops.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\6809tbl.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\t6502.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
