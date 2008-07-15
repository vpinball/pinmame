copy /V /-Y "InstallVPinMAME_VC2002.vcproj" "InstallVPinMAME_VC2008.vcproj"
copy /V /-Y "PinMAME_VC2002.vcproj" "PinMAME_VC2008.vcproj"
copy /V /-Y "PinMAME32_VC2002.vcproj" "PinMAME32_VC2008.vcproj"
copy /V /-Y "VPinMAME_VC2002.vcproj" "VPinMAME_VC2008.vcproj"
@echo.
@echo Now replace all occurrences of VC2002 in them with VC2008
@echo then convert them with VC2008.
@echo Add _CRT_SECURE_NO_WARNINGS to the C/C++ preprocessor definitions in all builds.
@pause
