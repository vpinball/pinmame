copy /V /-Y "PinMAME_VC2012.sln" "PinMAME_VC2013.sln"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj" "InstallVPinMAME_VC2013.vcxproj"
copy /V /-Y "PinMAME_VC2012.vcxproj" "PinMAME_VC2013.vcxproj"
copy /V /-Y "PinMAME32_VC2012.vcxproj" "PinMAME32_VC2013.vcxproj"
copy /V /-Y "VPinMAME_VC2012.vcxproj" "VPinMAME_VC2013.vcxproj"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj.filters" "InstallVPinMAME_VC2013.vcxproj.filters"
copy /V /-Y "PinMAME_VC2012.vcxproj.filters" "PinMAME_VC2013.vcxproj.filters"
copy /V /-Y "PinMAME32_VC2012.vcxproj.filters" "PinMAME32_VC2013.vcxproj.filters"
copy /V /-Y "VPinMAME_VC2012.vcxproj.filters" "VPinMAME_VC2013.vcxproj.filters"

@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2013" /in:"PinMAME_VC2013.sln" /out:"PinMAME_VC2013.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2012" /replace:"Visual Studio 2013" /in:"PinMAME_VC2013.sln" /out:"PinMAME_VC2013.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 12.00" /replace:"Format Version 13.00" /in:"PinMAME_VC2013.sln" /out:"PinMAME_VC2013.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2013" /in:"InstallVPinMAME_VC2013.vcxproj" /out:"InstallVPinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v120_xp" /in:"InstallVPinMAME_VC2013.vcxproj" /out:"InstallVPinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2013" /in:"PinMAME_VC2013.vcxproj" /out:"PinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v120_xp" /in:"PinMAME_VC2013.vcxproj" /out:"PinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2013" /in:"PinMAME32_VC2013.vcxproj" /out:"PinMAME32_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v120_xp" /in:"PinMAME32_VC2013.vcxproj" /out:"PinMAME32_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2013" /in:"VPinMAME_VC2013.vcxproj" /out:"VPinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v120_xp" /in:"VPinMAME_VC2013.vcxproj" /out:"VPinMAME_VC2013.vcxproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2012 in the project files with VC2013.

:end
@echo Convert the project files with VC2013 and compile.
@pause
