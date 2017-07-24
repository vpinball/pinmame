 /*
  RTH 02.2016
  lisy80NG Switch testes
*/

#include <stdio.h>
#include <unistd.h>
#include "../utils.h"
#include "../hw_lib.h"
#include "../coils.h"
#include "../displays.h"
#include "../switches.h"

//the debug options
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;


int main (int argc, char *argv[])
{


  int ret,action;

  // INIT the Hardware
  lisy80_hwlib_init();


  printf("start receive loop, wait for buffer not: empty signal (pi7==1) from slave\n ");
  printf("will loop until test switch (no: 7) is pressed\n");

        do
        {
         ret = lisy80_switch_reader( &action );

         if (ret <80) printf("switch reader returns action:%d for switch:%d\n\r",action,ret);
         if (ret >80) printf("switch reader returns error:%d\n\r",ret);

         if ( ( action == 1) && ( ret == 7) ) break;


        }while(1);

}//main
