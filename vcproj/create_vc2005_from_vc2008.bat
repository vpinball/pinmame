copy /V /-Y "PinMAME_VC2008.sln" "PinMAME_VC2005.sln"
copy /V /-Y "InstallVPinMAME_VC2008.vcproj" "InstallVPinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME_VC2008.vcproj" "PinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME32_VC2008.vcproj" "PinMAME32_VC2005.vcproj"
copy /V /-Y "VPinMAME_VC2008.vcproj" "VPinMAME_VC2005.vcproj"

@REM *** correct VS version and pathes
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2005" /in:"PinMAME_VC2005.sln" /out:"PinMAME_VC2005.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2005" /in:"InstallVPinMAME_VC2005.vcproj" /out:"InstallVPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2005" /in:"PinMAME_VC2005.vcproj" /out:"PinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2005" /in:"PinMAME32_VC2005.vcproj" /out:"PinMAME32_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2005" /in:"VPinMAME_VC2005.vcproj" /out:"VPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual

@REM *** correct solution file
@cscript "simplereplace.wsf" //nologo /search:"Format Version 10.00" /replace:"Format Version 9.00" /in:"PinMAME_VC2005.sln" /out:"PinMAME_VC2005.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"# Visual Studio 2008" /replace:"# Visual Studio 2005" /in:"PinMAME_VC2005.sln" /out:"PinMAME_VC2005.sln"
@if errorlevel 1 goto manual

@REM *** correct project files
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'8.00^'" /quotes /in:"InstallVPinMAME_VC2005.vcproj" /out:"InstallVPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'8.00^'" /quotes /in:"PinMAME_VC2005.vcproj" /out:"PinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'8.00^'" /quotes /in:"PinMAME32_VC2005.vcproj" /out:"PinMAME32_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9,00^'" /replace:"Version=^'8.00^'" /quotes /in:"VPinMAME_VC2005.vcproj" /out:"VPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@REM *** second possible format
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'8.00^'" /quotes /in:"InstallVPinMAME_VC2005.vcproj" /out:"InstallVPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'8.00^'" /quotes /in:"PinMAME_VC2005.vcproj" /out:"PinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'8.00^'" /quotes /in:"PinMAME32_VC2005.vcproj" /out:"PinMAME32_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Version=^'9.00^'" /replace:"Version=^'8.00^'" /quotes /in:"VPinMAME_VC2005.vcproj" /out:"VPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2008 in the solution and project files with VC2005.
@echo Replace all occurrences of Format Version 10.00 in the solution file with 9.00.
@echo Replace all occurrences of Visual Studio 2008 in the solution file with 2005.
@echo Replace all occurrences of Version="9,00" in the project files with "8,00".
@echo.

:end
@echo Load the solution with VC2005, then in each project change a setting and hit enter, revert the change and click ok. Close the solution and save changes.
@echo.
@pause
