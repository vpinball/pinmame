/*
 LISY_mame.c
 Mai 2019
 bontango
*/

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include "xmame.h"

//nvram handler for file writing
//callled sometinmes by lisy prg
//to avoid lost of data when game
//is just switched off
void lisy_nvram_write_to_file( void )
{

  /* save the NVRAM */
  /* RTH: from mame.c */
  if (Machine->drv->nvram_handler)
  {
      mame_file *nvram_file = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1);
      if (nvram_file != NULL)
      {
      (*Machine->drv->nvram_handler)(nvram_file, 1);
         mame_fclose(nvram_file);
            }
   }
}

