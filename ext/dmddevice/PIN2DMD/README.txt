Please copy the dmddevice.dll file into your PinMAME installation folder.
If you run your PIN2DMD device in compatibility mode with libusb-win32
drivers installed instead of WinUSB, you also need to copy libusbk.dll 
to your PinMAME folder. You can switch drivers using Zadig utility.

This driver automatically loads color information from pin2dmd.pal files in the 
subfolder named altcolor\"gamename" of PinMAME. 
(e.g. c:\pinmame\altcolor\tz_92\pin2dmd.pal for TwilightZone 9.2 ROM). 

For latest PIN2DMD dmddevice.dll drivers visit
https://github.com/lucky01/PIN2DMD

For more information visit the project's web site at:
http://www.pin2dmd.com

