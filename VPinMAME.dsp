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
# ADD CPP /nologo /G5 /MTd /W3 /Gm /ZI /Od /I ".\src" /I ".\src\wpc" /I ".\src\win32c" /I ".\src\zlib" /I ".\src\win32com" /I ".\src\win32com\autogen" /D "_DEBUG" /D "WPC_DCSSOUND" /D "WPC_WPCSOUND" /D BETA_VERSION=8 /D "WPCEXTENSION" /D mame_bitmap=osd_bitmap /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D VERSION=37 /D "MAME_MMX" /D MAMEVER=3716 /D PROCESSOR_ARCHITECTURE=x86 /D "LSB_FIRST" /D PI=3.1415926535 /D "PNG_SAVE_SUPPORT" /D "ZLIB_DLL" /D "WPCDCSSPEEDUP" /D "NOMIDAS" /D "TINY_COMPILE" /D "NEOFREE" /D "WPCMAME" /D "VPINMAME" /D "PINMAME" /D "PINMAME_EXT" /D BMTYPE=UINT8 /D "PINMAME_EXIT" /FR /YX /FD /GZ /c
# ADD MTL /out ".\src\win32com\autogen"
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i ".\src\win32com\autogen" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib shell32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib zlibstatmtd.lib winmm.lib version.lib /nologo /version:4.0 /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:".\zlib"
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
# ADD CPP /nologo /G5 /MT /W3 /O2 /I ".\src" /I ".\src\wpc" /I ".\src\win32c" /I ".\src\zlib" /I ".\src\win32com" /I ".\src\win32com\autogen" /D "NDEBUG" /D "_ATL_STATIC_REGISTRY" /D WPC_DCSSOUND=1 /D WPC_WPCSOUND=1 /D "WPCMMAME" /D BETA_VERSION=9 /D "WPCEXTENSION" /D mame_bitmap=osd_bitmap /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "_USRDLL" /D INLINE="static __inline" /D inline=__inline /D __inline__=__inline /D VERSION=37 /D "MAME_MMX" /D MAMEVER=3716 /D PROCESSOR_ARCHITECTURE=x86 /D "LSB_FIRST" /D PI=3.1415926535 /D "PNG_SAVE_SUPPORT" /D "ZLIB_DLL" /D "WPCDCSSPEEDUP" /D "NOMIDAS" /D "TINY_COMPILE" /D "NEOFREE" /D "WPCMAME" /D "VPINMAME" /D "PINMAME" /D "PINMAME_EXT" /D BMTYPE=UINT8 /D "PINMAME_EXIT" /YX /FD /c
# ADD MTL /out ".\src\win32com\autogen"
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i ".\src\win32com\autogen" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib shell32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib zlibstatmt.lib winmm.lib version.lib /nologo /version:4.0 /subsystem:windows /dll /machine:I386 /libpath:".\zlib"
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
# Begin Group "mame_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\artwork.c
# End Source File
# Begin Source File

SOURCE=.\src\audit.c
# End Source File
# Begin Source File

SOURCE=.\src\cheat.c
# End Source File
# Begin Source File

SOURCE=.\src\common.c
# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\datafile.c
# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\filter.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\filter.h
# End Source File
# Begin Source File

SOURCE=.\src\hiscore.c
# End Source File
# Begin Source File

SOURCE=.\src\info.c
# End Source File
# Begin Source File

SOURCE=.\src\inptport.c
# End Source File
# Begin Source File

SOURCE=.\src\input.c
# End Source File
# Begin Source File

SOURCE=.\src\mame.c
# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.c
# End Source File
# Begin Source File

SOURCE=.\src\memory.c
# End Source File
# Begin Source File

SOURCE=.\src\network.c
# End Source File
# Begin Source File

SOURCE=.\src\palette.c
# End Source File
# Begin Source File

SOURCE=.\src\png.c
# End Source File
# Begin Source File

SOURCE=.\src\profiler.c
# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\state.c
# End Source File
# Begin Source File

SOURCE=.\src\tilemap.c
# End Source File
# Begin Source File

SOURCE=.\src\timer.c
# End Source File
# Begin Source File

SOURCE=.\src\ui_text.c
# End Source File
# Begin Source File

SOURCE=.\src\unzip.c
# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.c
# End Source File
# Begin Source File

SOURCE=.\src\version.c
# End Source File
# Begin Source File

SOURCE=.\src\window.c
# End Source File
# End Group
# Begin Group "cpu_c"

# PROP Default_Filter ""
# Begin Group "adsp2100_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\adsp2100\2100dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.c
# End Source File
# End Group
# Begin Group "m6809_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6809\6809dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.c
# End Source File
# End Group
# Begin Group "m6800_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6800\6800dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.c
# End Source File
# End Group
# Begin Group "z80_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\z80\z80.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80dasm.c
# End Source File
# End Group
# Begin Group "m6502_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6502\6502dasm.c
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m6502\m6502.c
# End Source File
# End Group
# Begin Group "m68000_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kcpu.c
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
# End Group
# Begin Group "machine_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\machine\6522via.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\6532riot.c

!IF  "$(CFG)" == "Visual PinMame - Win32 Debug"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "Visual PinMame - Win32 Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\machine\6821pia.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\mathbox.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\ticket.c
# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.c
# End Source File
# End Group
# Begin Group "win32c_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32c\DirectSound.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Dirty.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Display.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\DXDecode.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\File.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Keyboard.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\M32Util.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\OSDepend.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Registry.c
# End Source File
# Begin Source File

SOURCE=.\src\win32c\UClock.c
# End Source File
# End Group
# Begin Group "sound_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sound\2151intf.c
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

SOURCE=.\src\sound\ay8910.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\bsmt2000.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.c
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.c
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

SOURCE=.\src\sound\ym2151.c
# End Source File
# End Group
# Begin Group "vidhrdw_c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vidhrdw\avgdvg.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\crtc6845.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\tms9928a.c
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.c
# End Source File
# End Group
# Begin Group "wpc.c"

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

SOURCE=.\src\wpc\by35.c
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

SOURCE=.\src\wpc\byvidpin.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\core.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\dcs.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de1sound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de2.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de2sound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de3.c
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

SOURCE=.\src\wpc\driver.c
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

SOURCE=.\src\wpc\gts3games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3sound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mech.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11csoun.c
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

SOURCE=.\src\wpc\s4games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s67s.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80games.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80sound0.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80sound1.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80sound2.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\se.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\segames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sesound.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sim.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\snd_cmd.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vpintf.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpc.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcgames.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsam.c
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsound.c
# End Source File
# End Group
# Begin Group "win32com_c"

# PROP Default_Filter "*.cpp;*.def;*idl"
# Begin Source File

SOURCE=.\src\win32com\Controller.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGameInfo.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGlobals.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerOptions.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerOptionsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRun.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\StdAfx.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.def
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.idl
# ADD MTL /tlb "VPinMAME.tlb"
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

SOURCE=.\src\win32com\VPinMAMEDisclaimerDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMESplashWnd.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "mame_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\artwork.h
# End Source File
# Begin Source File

SOURCE=.\src\audit.h
# End Source File
# Begin Source File

SOURCE=.\src\cheat.h
# End Source File
# Begin Source File

SOURCE=.\src\common.h
# End Source File
# Begin Source File

SOURCE=.\src\cpuintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\datafile.h
# End Source File
# Begin Source File

SOURCE=.\src\drawgfx.h
# End Source File
# Begin Source File

SOURCE=.\src\driver.h
# End Source File
# Begin Source File

SOURCE=.\src\gfxobj.h
# End Source File
# Begin Source File

SOURCE=.\src\hiscore.h
# End Source File
# Begin Source File

SOURCE=.\src\info.h
# End Source File
# Begin Source File

SOURCE=.\src\inptport.h
# End Source File
# Begin Source File

SOURCE=.\src\input.h
# End Source File
# Begin Source File

SOURCE=.\src\legacy.h
# End Source File
# Begin Source File

SOURCE=.\src\mame.h
# End Source File
# Begin Source File

SOURCE=.\src\mamedbg.h
# End Source File
# Begin Source File

SOURCE=.\src\memory.h
# End Source File
# Begin Source File

SOURCE=.\src\network.h
# End Source File
# Begin Source File

SOURCE=.\src\osdepend.h
# End Source File
# Begin Source File

SOURCE=.\src\palette.h
# End Source File
# Begin Source File

SOURCE=.\src\png.h
# End Source File
# Begin Source File

SOURCE=.\src\profiler.h
# End Source File
# Begin Source File

SOURCE=.\src\sndintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\sprite.h
# End Source File
# Begin Source File

SOURCE=.\src\state.h
# End Source File
# Begin Source File

SOURCE=.\src\tilemap.h
# End Source File
# Begin Source File

SOURCE=.\src\timer.h
# End Source File
# Begin Source File

SOURCE=.\src\ui_text.h
# End Source File
# Begin Source File

SOURCE=.\src\unzip.h
# End Source File
# Begin Source File

SOURCE=.\src\usrintrf.h
# End Source File
# Begin Source File

SOURCE=.\src\window.h
# End Source File
# End Group
# Begin Group "cpu_h"

# PROP Default_Filter ""
# Begin Group "adsp2100_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\adsp2100\adsp2100.h
# End Source File
# End Group
# Begin Group "m6809_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6809\m6809.h
# End Source File
# End Group
# Begin Group "m6800_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6800\m6800.h
# End Source File
# End Group
# Begin Group "z80_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\z80\z80.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80daa.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\z80\z80dasm.h
# End Source File
# End Group
# Begin Group "m6502_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\cpu\m6502\ill02.h
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
# Begin Group "m68000_h"

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

SOURCE=.\src\cpu\m68000\m68kcpu.h
# End Source File
# Begin Source File

SOURCE=.\src\cpu\m68000\m68kmame.h
# End Source File
# End Group
# End Group
# Begin Group "machine_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\machine\6522via.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6532riot.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\6821pia.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\8255ppi.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\eeprom.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\mathbox.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\ticket.h
# End Source File
# Begin Source File

SOURCE=.\src\machine\z80fmly.h
# End Source File
# End Group
# Begin Group "win32c_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\win32c\DirectSound.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Dirty.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Display.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\DXDecode.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\File.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\FilePrivate.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Keyboard.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\M32Util.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\osd_cpu.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\OSInline.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Registry.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\Strings.h
# End Source File
# Begin Source File

SOURCE=.\src\win32c\UClock.h
# End Source File
# End Group
# Begin Group "sound_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\sound\2151intf.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\adpcm.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\ay8910.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\bsmt2000.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\dac.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\fm.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\hc55516.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\mixer.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\msm5205.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\samples.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\streams.h
# End Source File
# Begin Source File

SOURCE=.\src\sound\ym2151.h
# End Source File
# End Group
# Begin Group "vidhrdw_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\vidhrdw\avgdvg.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\crtc6845.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\generic.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\tms9928a.h
# End Source File
# Begin Source File

SOURCE=.\src\vidhrdw\vector.h
# End Source File
# End Group
# Begin Group "wpc_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\wpc\by35.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\byvidpin.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\core.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\dcs.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de1sound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de2.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\de2sound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gen.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3dmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\gts3sound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\mech.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s11csoun.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s4.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s6.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\S67s.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s7.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\s80sound0.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\S80SOUND1.H
# End Source File
# Begin Source File

SOURCE=.\src\wpc\se.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sesound.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\sim.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\snd_cmd.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\vpintf.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpc.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsam.h
# End Source File
# Begin Source File

SOURCE=.\src\wpc\wpcsound.h
# End Source File
# End Group
# Begin Group "zlib_h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=.\src\zlib\zlib.h
# End Source File
# End Group
# Begin Group "win32com_h"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\src\win32com\Controller.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGameInfo.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerGlobals.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerOptions.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerOptionsDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRegkeys.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\ControllerRun.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\dlldatax.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\resource.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAME.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEAboutDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMECP.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMEDisclaimerDlg.h
# End Source File
# Begin Source File

SOURCE=.\src\win32com\VPinMAMESplashWnd.h
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
