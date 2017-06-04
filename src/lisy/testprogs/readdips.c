 /*
  RTH 02.2016
  lisy80NG Switch testes
*/

#include <stdio.h>
#include "../utils.h"

//define it here
int lisy80_gamenr = 0;

int lisy80_time_to_quit_flag; //not used here


int main (int argc, char *argv[])
{


  printf("this is dip switch1:%d\n\r",lisy80_get_mpudips(  0 ));
  printf("this is dip switch2:%d\n\r",lisy80_get_mpudips(  1 ));
  printf("this is dip switch3:%d\n\r",lisy80_get_mpudips(  2 ));
  printf("this is dip switch4:%d\n\r",lisy80_get_mpudips(  3 ));


}//main
