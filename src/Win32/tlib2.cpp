/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2000 Michael Soderstrom and Chris Kirmse
    
  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

***************************************************************************/
 
/***************************************************************************

  tlib.cpp

  Wrapper for Borland's tlib.exe.

  tlib doesn't handle object files of the same name in different subdirs.
  This renames the input files to unique names, calls tlib with a modified
  command line to create the library, then moves/names the files back.

***************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <process.h> // For spawnvp.

using namespace std;

typedef vector<pair<string, string> > string_pairs;

int main(int argc, char* argv[])
{
    // Move/rename object files to unique files.

    string_pairs filenames;
    for (int i = 2; i < argc; i++)
    {
        // Skip options.
        if (argv[i][0] == '/')
            continue;

        string_pairs::value_type strpair;
 
        strpair.first = argv[i];
        replace(argv[i], (argv[i] + strlen(argv[i])), '\\', '_');
        strpair.second = argv[i];

        rename(strpair.first.c_str(), strpair.second.c_str());

        // Save filenames for restoration.
        filenames.push_back(strpair);
    }

    // Call target exe with modified arglist.

    argv[0] = "tlib.exe";
    int nResult = spawnvp(_P_WAIT, argv[0], argv);

    // Put the object files back.

    string_pairs::iterator sp;
    for (sp = filenames.begin(); sp != filenames.end(); ++sp)
        rename((*sp).second.c_str(), (*sp).first.c_str());

    return nResult;
}