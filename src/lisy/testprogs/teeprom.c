 /*
  RTH 04.2016
  lisy80NG eeprom tests
*/

#include <stdio.h>
#include <string.h>
#include "../eeprom.h"

//define it here
int lisy80_gamenr = 0;

int lisy80_time_to_quit_flag; //not used here


int main (int argc, char *argv[])
{
 int i; 
 char mybuf[256];

printf("possible options: init|destroy\n\r\n\r");

if (argc >= 2)
 {
 if (strncmp(argv[1],"init",4) == 0 )
  {
   printf("we initialize the eeprom\n\r");

   if ( lisy80_eeprom_init() == 0)
     printf(" init done\n\r");
   else
     printf(" init failed\n\r");
  }
 if (strncmp(argv[1],"destroy",6) == 0 )
  {
     //init buffer
    for(i=0; i<=255; i++) mybuf[i] = '#';
    lisy80_eeprom_256byte_write( mybuf, 1);
    printf(" eeprom filled with '#'\n\r");
    return 0;
  }
 }

  if (lisy80_eeprom_checksignature())
    lisy80_eeprom_printstats();   
  else
     printf("no valid signature \n\r");

return 0;
}//main
