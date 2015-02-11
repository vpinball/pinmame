copy /V /-Y "PinMAME_VC2008.sln" "PinMAME_VC2012.sln"
copy /V /-Y "InstallVPinMAME_VC2008.vcproj" "InstallVPinMAME_VC2012.vcproj"
copy /V /-Y "PinMAME_VC2008.vcproj" "PinMAME_VC2012.vcproj"
copy /V /-Y "PinMAME32_VC2008.vcproj" "PinMAME32_VC2012.vcproj"
copy /V /-Y "VPinMAME_VC2008.vcproj" "VPinMAME_VC2012.vcproj"

@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2012" /in:"PinMAME_VC2012.sln" /out:"PinMAME_VC2012.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2008" /replace:"Visual Studio 2012" /in:"PinMAME_VC2012.sln" /out:"PinMAME_VC2012.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 10.00" /replace:"Format Version 12.00" /in:"PinMAME_VC2012.sln" /out:"PinMAME_VC2012.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2012" /in:"InstallVPinMAME_VC2012.vcproj" /out:"InstallVPinMAME_VC2012.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2012" /in:"PinMAME_VC2012.vcproj" /out:"PinMAME_VC2012.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2012" /in:"PinMAME32_VC2012.vcproj" /out:"PinMAME32_VC2012.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2012" /in:"VPinMAME_VC2012.vcproj" /out:"VPinMAME_VC2012.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2008 in the project files with VC2012.

:end
@echo Convert the project files with VC2012 and compile.
@echo After conversion delete the *_VC2012.vcproj files.
@pause
