copy /V /-Y "PinMAME_VC2015.sln" "PinMAME_VC2017.sln"
copy /V /-Y "PinMAME_VC2015.vcxproj" "PinMAME_VC2017.vcxproj"
copy /V /-Y "PinMAMEdll_VC2015.vcxproj" "PinMAMEdll_VC2017.vcxproj"
copy /V /-Y "PinMAMEdllTest_VC2015.vcxproj" "PinMAMEdllTest_VC2017.vcxproj"
copy /V /-Y "VPinMAME_VC2015.vcxproj" "VPinMAME_VC2017.vcxproj"
copy /V /-Y "PinMAME_VC2015.vcxproj.filters" "PinMAME_VC2017.vcxproj.filters"
copy /V /-Y "PinMAMEdll_VC2015.vcxproj.filters" "PinMAMEdll_VC2017.vcxproj.filters"
copy /V /-Y "PinMAMEdllTest_VC2015.vcxproj.filters" "PinMAMEdllTest_VC2017.vcxproj.filters"
copy /V /-Y "VPinMAME_VC2015.vcxproj.filters" "VPinMAME_VC2017.vcxproj.filters"

@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Visual Studio 2015" /replace:"Visual Studio 2017" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"Format Version 14.00" /replace:"Format Version 15.00" /in:"PinMAME_VC2017.sln" /out:"PinMAME_VC2017.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAME_VC2017.vcxproj" /out:"PinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAMEdll_VC2017.vcxproj" /out:"PinMAMEdll_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"PinMAMEdllTest_VC2017.vcxproj" /out:"PinMAMEdllTest_VC2017.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2015" /replace:"VC2017" /in:"VPinMAME_VC2017.vcxproj" /out:"VPinMAME_VC2017.vcxproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2015 in the project files with VC2017.

:end
@echo Convert the project files with VC2017 and compile.
@pause
