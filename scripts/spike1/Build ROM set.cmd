@echo off
rem Opens the Spike 1 ROM set builder. Drop Stern's .iso.zip or .iso, a card dump (.bin, .img, or a .7z,
rem which needs 7-Zip) or a .spk on this file to fill it in.
powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Build-Spike1RomSet.ps1" -Gui %*
