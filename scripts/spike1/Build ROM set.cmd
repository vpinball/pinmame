@echo off
rem Opens the Spike 1 ROM set builder. Drop Stern's .iso.zip, a card dump or a .spk on this file to fill it in.
powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Build-Spike1RomSet.ps1" -Gui %*
