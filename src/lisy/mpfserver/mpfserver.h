#ifndef MPFSERVER_H
#define MPFSERVER_H

//the version
#define MPFSERVER_SOFTWARE_MAIN    0
#define MPFSERVER_SOFTWARE_SUB     45

struct stru_lisy_hw
{
  char lisy_hw[10];  // LISY Hardware LISY1 or LISY80
  char lisy_ver[10];  // LISY version
  char api_ver[10];  // MPF Server API  version
  unsigned char no_lamps;  // Number of Lamps, current hardware 
  unsigned char no_sol;  // Number of Solenoids, current hardware 
  unsigned char no_sounds;  // Number of Sounds, current hardware 
  unsigned char no_disp;  // Number of Displays, current hardware 
  unsigned char no_switches;  // Number of Switches, current hardware 
  char game_info[10];  // Game Info, Char for System1 & 3Char(Number) for System80
} ;


#define MPF_MP3_PATH "./hardware_sounds/"	//the path to get our MP3 files

#endif  /* MPFSERVER_H */

