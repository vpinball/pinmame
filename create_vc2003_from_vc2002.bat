copy /V /-Y "PinMAME_VC2002.sln" "PinMAME_VC2003.sln"
copy /V /-Y "InstallVPinMAME_VC2002.vcproj" "InstallVPinMAME_VC2003.vcproj"
copy /V /-Y "PinMAME_VC2002.vcproj" "PinMAME_VC2003.vcproj"
copy /V /-Y "PinMAME32_VC2002.vcproj" "PinMAME32_VC2003.vcproj"
copy /V /-Y "VPinMAME_VC2002.vcproj" "VPinMAME_VC2003.vcproj"

@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2003" /in:"PinMAME_VC2003.sln" /out:"PinMAME_VC2003.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2003" /in:"InstallVPinMAME_VC2003.vcproj" /out:"InstallVPinMAME_VC2003.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2003" /in:"PinMAME_VC2003.vcproj" /out:"PinMAME_VC2003.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2003" /in:"PinMAME32_VC2003.vcproj" /out:"PinMAME32_VC2003.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2003" /in:"VPinMAME_VC2003.vcproj" /out:"VPinMAME_VC2003.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2002 in the project files with VC2003.

:end
@echo Convert the project files with VC2003 and compile.
@pause
