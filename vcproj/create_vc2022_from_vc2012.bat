copy /V /-Y "PinMAME_VC2012.sln" "PinMAME_VC2022.sln"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj" "InstallVPinMAME_VC2022.vcxproj"
copy /V /-Y "PinMAME_VC2012.vcxproj" "PinMAME_VC2022.vcxproj"
copy /V /-Y "PinMAME32_VC2012.vcxproj" "PinMAME32_VC2022.vcxproj"
copy /V /-Y "VPinMAME_VC2012.vcxproj" "VPinMAME_VC2022.vcxproj"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj.filters" "InstallVPinMAME_VC2022.vcxproj.filters"
copy /V /-Y "PinMAME_VC2012.vcxproj.filters" "PinMAME_VC2022.vcxproj.filters"
copy /V /-Y "PinMAME32_VC2012.vcxproj.filters" "PinMAME32_VC2022.vcxproj.filters"
copy /V /-Y "VPinMAME_VC2012.vcxproj.filters" "VPinMAME_VC2022.vcxproj.filters"
copy /V /-Y "LibPinMAME_VC2015.vcxproj" "LibPinMAME_VC2022.vcxproj"
copy /V /-Y "LibPinMAME_VC2015.vcxproj.filters" "LibPinMAME_VC2022.vcxproj.filters"
copy /V /-Y "LibPinMAMETest_VC2015.vcxproj" "LibPinMAMETest_VC2022.vcxproj"
copy /V /-Y "LibPinMAMETest_VC2015.vcxproj.filters" "LibPinMAMETest_VC2022.vcxproj.filters"

@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2022" /in:"PinMAME_VC2022.sln" /out:"PinMAME_VC2022.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2012" /replace:"Visual Studio Version 16" /in:"PinMAME_VC2022.sln" /out:"PinMAME_VC2022.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 12.00" /replace:"Format Version 14.00" /in:"PinMAME_VC2022.sln" /out:"PinMAME_VC2022.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2022" /in:"InstallVPinMAME_VC2022.vcxproj" /out:"InstallVPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v143" /in:"InstallVPinMAME_VC2022.vcxproj" /out:"InstallVPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2022" /in:"PinMAME_VC2022.vcxproj" /out:"PinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v143" /in:"PinMAME_VC2022.vcxproj" /out:"PinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2022" /in:"PinMAME32_VC2022.vcxproj" /out:"PinMAME32_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v143" /in:"PinMAME32_VC2022.vcxproj" /out:"PinMAME32_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2022" /in:"VPinMAME_VC2022.vcxproj" /out:"VPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v143" /in:"VPinMAME_VC2022.vcxproj" /out:"VPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"inline=__inline;" /replace:"" /in:"VPinMAME_VC2022.vcxproj" /out:"VPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2022" /in:"LibPinMAME_VC2022.vcxproj" /out:"LibPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v140_xp" /replace:"v143" /in:"LibPinMAME_VC2022.vcxproj" /out:"LibPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"inline=__inline;" /replace:"" /in:"LibPinMAME_VC2022.vcxproj" /out:"LibPinMAME_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2022" /in:"LibPinMAMETest_VC2022.vcxproj" /out:"LibPinMAMETest_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v140_xp" /replace:"v143" /in:"LibPinMAMETest_VC2022.vcxproj" /out:"LibPinMAMETest_VC2022.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"inline=__inline;" /replace:"" /in:"LibPinMAMETest_VC2022.vcxproj" /out:"LibPinMAMETest_VC2022.vcxproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2012 in the project files with VC2022.

:end
@echo Convert the project files with VC2022 and compile.
@pause
