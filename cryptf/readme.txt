*************************
*************************
*** File Crypter 1.0  ***
*** by Steve Ellenoff ***
*** May 30,2002       ***
*************************
*************************

What does it do?
----------------
Allows you to encrypt or decrypt TEXT files using a Crypter key that 
you specify (can be numbers or letters or both).

**IMPORTANT**
Every file that you wish to encrypt/decrypt MUST have the name of the file contained within the file somewhere.
Example: If the file is named test.c, then somewhere in the file it should contain the text "test.c".

WHY?
The program searches for the filename within the text file to determine if the file has already been decrypted/encrypted.
This is a safety measure to help ensure you don't try ehcrypting/decrypting a file more than once, and ultimately ruining it.

Installation:
-------------
Create a folder anywhere on your computer and unzip all files.

Uninstallation:
---------------
Simply delete the folder you installed it in. THe program does not rely on any DLL files, nor
does it write to the registry, so it is perfectly harmless to remove.

Modes of Operation:
-------------------
The program works in two modes: Command Line Mode and Graphical Interface Mode.
If you simply double click on the cryptf.exe program, it will open in Graphical Interface Mode.
If you pass parameters to the program when opening it, such as "cryptf.exe -d c:\docs\test.cpp etc.."
it will run in command mode.

Command Line Mode:
------------------
In command line mode, you process one file at a time, by specifying the information on the command line.
For more info, launch the program as: cryptf.exe /?
You must give it a valid INI file which will specify the Key you wish to use for encryption/decryption.
All other information, such as type of action(encrypt/decrypt), source, destination, and INI file names and
locations can be specified on the command line.
This mode allows for dos-style batches and processing, as well as compiler batching and processing.

Graphical Interface Mode:
-------------------------
In the graphical interface mode, a windows form appears allowing you to setup the
encryption/decryption process. In this mode, a file called "files.txt" lists the names of all
the files you wish to process, allowing you to easily batch the process into 1 step.

Setup your file list:
---------------------
From the menu, select File->Modify->Files List, and your "files.txt" will open.
Enter the names of all files you wish to include for processing in the file.
Make sure only 1 file name is listed on each line, and DO NOT INCLUDE PATH/DIRECTORY INFORMATION!
All files are assumed to live in 1 directory, which you specify on the windows form.
Wildcards are not supported at this time.

Crypt.ini
---------
This file stores your preferences, and is saved each time you close the application.
You can manually modify it if you wish by selecting from the menu: 
File->Modify->INI File.

Having Problems?
----------------
Send email to sellenoff@hotmail.com for any issues you are having.

Version History:
----------------

05/30/2002 - Version 1.0 - Initial Release

