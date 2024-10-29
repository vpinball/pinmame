copy /V /-Y "PinMAME_VC2008.sln" "PinMAME_VC2002.sln"
copy /V /-Y "InstallVPinMAME_VC2008.vcproj" "InstallVPinMAME_VC2002.vcproj"
copy /V /-Y "PinMAME_VC2008.vcproj" "PinMAME_VC2002.vcproj"
copy /V /-Y "PinMAME32_VC2008.vcproj" "PinMAME32_VC2002.vcproj"
copy /V /-Y "VPinMAME_VC2008.vcproj" "VPinMAME_VC2002.vcproj"

@REM *** correct VS version and pathes
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2002" /in:"PinMAME_VC2002.sln" /out:"PinMAME_VC2002.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2002" /in:"InstallVPinMAME_VC2002.vcproj" /out:"InstallVPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2002" /in:"PinMAME_VC2002.vcproj" /out:"PinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2002" /in:"PinMAME32_VC2002.vcproj" /out:"PinMAME32_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2002" /in:"VPinMAME_VC2002.vcproj" /out:"VPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual

@REM *** correct solution file
@cscript "simplereplace.wsf" //nologo /search:"Format Version 10.00" /replace:"Format Version 7.00" /in:"PinMAME_VC2002.sln" /out:"PinMAME_VC2002.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"GlobalSection(SolutionConfigurationPlatforms)" /replace:"GlobalSection(SolutionConfiguration)" /in:"PinMAME_VC2002.sln" /out:"PinMAME_VC2002.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"GlobalSection(ProjectConfigurationPlatforms)" /replace:"GlobalSection(ProjectConfiguration)" /in:"PinMAME_VC2002.sln" /out:"PinMAME_VC2002.sln"
@if errorlevel 1 goto manual

@REM *** correct project files
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'7.00^'" /quotes /in:"InstallVPinMAME_VC2002.vcproj" /out:"InstallVPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'7.00^'" /quotes /in:"PinMAME_VC2002.vcproj" /out:"PinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'7.00^'" /quotes /in:"PinMAME32_VC2002.vcproj" /out:"PinMAME32_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'7.00^'" /quotes /in:"VPinMAME_VC2002.vcproj" /out:"VPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@REM *** second possible format
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'7.00^'" /quotes /in:"InstallVPinMAME_VC2002.vcproj" /out:"InstallVPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'7.00^'" /quotes /in:"PinMAME_VC2002.vcproj" /out:"PinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'7.00^'" /quotes /in:"PinMAME32_VC2002.vcproj" /out:"PinMAME32_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'7.00^'" /quotes /in:"VPinMAME_VC2002.vcproj" /out:"VPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@REM
@cscript "simplereplace.wsf" //nologo /search:";PROC_SUPPORT" /replace:"" /quotes /in:"InstallVPinMAME_VC2002.vcproj" /out:"InstallVPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:";PROC_SUPPORT" /replace:"" /quotes /in:"PinMAME_VC2002.vcproj" /out:"PinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:";PROC_SUPPORT" /replace:"" /quotes /in:"PinMAME32_VC2002.vcproj" /out:"PinMAME32_VC2002.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:";PROC_SUPPORT" /replace:"" /quotes /in:"VPinMAME_VC2002.vcproj" /out:"VPinMAME_VC2002.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2008 in the solution and project files with VC2002.
@echo Replace all occurrences of Format Version 10.00 in the solution file with 7.00.
@echo Replace all occurrences of Version="9,00" in the project files with "7.00".
@echo.

:end
@echo Load the solution with VC2002, then in each project change a setting and hit enter, revert the change and click ok. Close the solution and save changes.
@echo.
@pause
