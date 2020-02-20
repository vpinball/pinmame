copy /V /-Y "PinMAME_VC2012.sln" "PinMAME_VC2017.sln"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj" "InstallVPinMAME_VC2017.vcxproj"
copy /V /-Y "PinMAME_VC2012.vcxproj" "PinMAME_VC2017.vcxproj"
copy /V /-Y "PinMAME32_VC2012.vcxproj" "PinMAME32_VC2017.vcxproj"
copy /V /-Y "VPinMAME_VC2012.vcxproj" "VPinMAME_VC2017.vcxproj"
copy /V /-Y "InstallVPinMAME_VC2012.vcxproj.filters" "InstallVPinMAME_VC2017.vcxproj.filters"
copy /V /-Y "PinMAME_VC2012.vcxproj.filters" "PinMAME_VC2017.vcxproj.filters"
copy /V /-Y "PinMAME32_VC2012.vcxproj.filters" "PinMAME32_VC2017.vcxproj.filters"
copy /V /-Y "VPinMAME_VC2012.vcxproj.filters" "VPinMAME_VC2017.vcxproj.filters"
copy /V /-Y "PinMAMEdll_VC2015.vcxproj" "PinMAMEdll_VC2017.vcxproj"
copy /V /-Y "PinMAMEdll_VC2015.vcxproj.filters" "PinMAMEdll_VC2017.vcxproj.filters"
copy /V /-Y "PinMAMEdllTest_VC2015.vcxproj" "PinMAMEdllTest_VC2017.vcxproj"
copy /V /-Y "PinMAMEdllTest_VC2015.vcxproj.filters" "PinMAMEdllTest_VC2017.vcxproj.filters"

@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2017" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2012" /replace:"Visual Studio Version 15" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 12.00" /replace:"Format Version 14.00" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2017" /in:"InstallVPinMAME_VC2017.vcxproj" /out:"InstallVPinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v141_xp" /in:"InstallVPinMAME_VC2017.vcxproj" /out:"InstallVPinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2017" /in:"PinMAME_VC2017.vcxproj" /out:"PinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v141_xp" /in:"PinMAME_VC2017.vcxproj" /out:"PinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2017" /in:"PinMAME32_VC2017.vcxproj" /out:"PinMAME32_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v141_xp" /in:"PinMAME32_VC2017.vcxproj" /out:"PinMAME32_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2012" /replace:"VC2017" /in:"VPinMAME_VC2017.vcxproj" /out:"VPinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v110" /replace:"v141_xp" /in:"VPinMAME_VC2017.vcxproj" /out:"VPinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAMEdll_VC2017.vcxproj" /out:"PinMAMEdll_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v140_xp" /replace:"v141_xp" /in:"PinMAMEdll_VC2017.vcxproj" /out:"PinMAMEdll_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAMEdllTest_VC2017.vcxproj" /out:"PinMAMEdllTest_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"v140_xp" /replace:"v141_xp" /in:"PinMAMEdllTest_VC2017.vcxproj" /out:"PinMAMEdllTest_VC2017.vcxproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2012 in the project files with VC2017.

:end
@echo Convert the project files with VC2017 and compile.
@pause
