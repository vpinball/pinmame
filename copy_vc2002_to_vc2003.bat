copy /V /-Y "InstallVPinMAME_VC2002.vcproj" "InstallVPinMAME_VC2003.vcproj"
copy /V /-Y "PinMAME_VC2002.vcproj" "PinMAME_VC2003.vcproj"
copy /V /-Y "PinMAME32_VC2002.vcproj" "PinMAME32_VC2003.vcproj"
copy /V /-Y "VPinMAME_VC2002.vcproj" "VPinMAME_VC2003.vcproj"
@echo.
@echo Now replace all occurrences of VC2002 in them with VC2003
@echo then convert them with VC2003 and compile.
@pause
