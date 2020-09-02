/*
 fadecandy.c
 May 2018
 bontango
 based on 'openpixelcontrol'
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "opc.h"
#include "fileio.h"
#include "utils.h"
#include "hw_lib.h"
#include "fadecandy.h"
#include "lisy_home.h"
#include "externals.h"

//we reserve 512 pixels which is the max value for one fadecandy
pixel lisy_fc_leds[512];
// the sink for our opc device (fadecandy)
opc_sink lisy_opc_sink;

//init the fadecandy vars
//and connection to fadecandyserver
int lisy_fadecandy_init(unsigned char system)
{
int i,res;


//get opc sink, no connection here 
lisy_opc_sink = opc_new_sink("127.0.0.1:7890");


//init set all pixels to zero ( -> OFF )
for ( i=0; i<=511; i++)
  lisy_fc_leds[i].r = lisy_fc_leds[i].g = lisy_fc_leds[i].b = 0;
//set this to all connected LEDs
opc_put_pixels( lisy_opc_sink,1, 512, lisy_fc_leds);

//check for fadecandy config -> do the mapping
res = lisy_file_get_led_mappings(system);
 if ( res < 0 )
  {
    fprintf(stderr,"Error:no LED mapping or LED mapping error\n");
    return -1;
  }
//set this to all connected LEDs (for activating GI )
opc_put_pixels( lisy_opc_sink,1, 512, lisy_fc_leds);

//start the fadecandy server: will be done in /usr/local/run_lisy

//check for hw ???

return 0;

}

//set the LED via fadecandy
int lisy_fadecandy_set_led(int lamp, unsigned char value)
{

  int led;

  //get the mapped led
  led = lisy_lamp_to_led_map[lamp].mapled;
  

  //assign the new colorcode to the pixel var
  if(value)
   {
    lisy_fc_leds[led].r = lisy_lamp_to_led_map[lamp].r;
    lisy_fc_leds[led].g = lisy_lamp_to_led_map[lamp].g;
    lisy_fc_leds[led].b = lisy_lamp_to_led_map[lamp].b;
   }
  else
   {
    lisy_fc_leds[led].r = 0;
    lisy_fc_leds[led].g = 0;
    lisy_fc_leds[led].b = 0;
   }

 //set the LED, we need the first n LED value to set LED n
 //we use channel No 1
 if ( ls80dbg.bitv.lamps )
     {
 	sprintf(debugbuf,"Fadecandy: we set led %d with %d : %d %d %d \n",led,value,lisy_fc_leds[led].r,lisy_fc_leds[led].g,lisy_fc_leds[led].b);
	lisy80_debug(debugbuf);
     }
 return ( opc_put_pixels( lisy_opc_sink,1, led+1, lisy_fc_leds));
 
}
