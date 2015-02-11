copy /V /-Y "PinMAME_VC2008.sln" "PinMAME_VC2010.sln"
copy /V /-Y "InstallVPinMAME_VC2008.vcproj" "InstallVPinMAME_VC2010.vcproj"
copy /V /-Y "PinMAME_VC2008.vcproj" "PinMAME_VC2010.vcproj"
copy /V /-Y "PinMAME32_VC2008.vcproj" "PinMAME32_VC2010.vcproj"
copy /V /-Y "VPinMAME_VC2008.vcproj" "VPinMAME_VC2010.vcproj"

@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME_VC2010.sln" /out:"PinMAME_VC2010.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2008" /replace:"Visual Studio 2010" /in:"PinMAME_VC2010.sln" /out:"PinMAME_VC2010.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 10.00" /replace:"Format Version 11.00" /in:"PinMAME_VC2010.sln" /out:"PinMAME_VC2010.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"InstallVPinMAME_VC2010.vcproj" /out:"InstallVPinMAME_VC2010.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME_VC2010.vcproj" /out:"PinMAME_VC2010.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME32_VC2010.vcproj" /out:"PinMAME32_VC2010.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"VPinMAME_VC2010.vcproj" /out:"VPinMAME_VC2010.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2008 in the project files with VC2010.

:end
@echo Convert the project files with VC2010 and compile.
@echo After conversion delete the *_VC2010.vcproj files.
@pause
