copy /V /-Y "InstallVPinMAME_VC2002.vcproj" "InstallVPinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME_VC2002.vcproj" "PinMAME_VC2005.vcproj"
copy /V /-Y "PinMAME32_VC2002.vcproj" "PinMAME32_VC2005.vcproj"
copy /V /-Y "VPinMAME_VC2002.vcproj" "VPinMAME_VC2005.vcproj"
@echo.
@echo Now replace all occurrences of VC2002 in them with VC2005
@echo then convert them with VC2005.
@echo Add _CRT_SECURE_NO_WARNINGS to the C/C++ preprocessor definitions in all builds.
@pause
