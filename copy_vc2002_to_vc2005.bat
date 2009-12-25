copy /V /-Y "InstallVPinMAME_VC2002.vcproj" "InstallVPinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME_VC2002.vcproj" "PinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME32_VC2002.vcproj" "PinMAME32_VC2005.vcproj"
copy /V /-Y "VPinMAME_VC2002.vcproj" "VPinMAME_VC2005.vcproj"

@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2005" /in:"InstallVPinMAME_VC2005.vcproj" /out:"InstallVPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2005" /in:"PinMAME_VC2005.vcproj" /out:"PinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2005" /in:"PinMAME32_VC2005.vcproj" /out:"PinMAME32_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2002" /replace:"VC2005" /in:"VPinMAME_VC2005.vcproj" /out:"VPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual

@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"InstallVPinMAME_VC2005.vcproj" /out:"InstallVPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"PinMAME_VC2005.vcproj" /out:"PinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"PinMAME32_VC2005.vcproj" /out:"PinMAME32_VC2005.vcproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"VPinMAME_VC2005.vcproj" /out:"VPinMAME_VC2005.vcproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2002 in the project files with VC2005.
@echo Add _CRT_SECURE_NO_WARNINGS to the C/C++ preprocessor definitions in all builds.

:end
@echo Convert the project files with VC2005 and compile.
@pause
