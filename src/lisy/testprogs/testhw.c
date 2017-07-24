 /*
  RTH 11.2016
  lisy80NG 
  test the hardware
  version 0.01
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <wiringPi.h>
#include "../hw_lib.h"
#include "../coils.h"
#include "../displays.h"
#include "../switches.h"

#define VERSION "0.01"

//the debug options
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;
int lisy80_is80B;
//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0 };
int lisy80_time_to_quit_flag; //not used here

/* 
 ****** MAIN *****
*/

int main( int argc, char **argv )
{



//do we wanr to do debug mode?
ls80dbg.byte = 0;


	printf("\n LISY80 Test the Hardware Version %s\n",VERSION);


	// INIT the Hardware
	printf("init the hardware\n");
	lisy80_hwlib_init();
	//init coils (just to be on the safe side)
	coil_init( );
	coil_led_set( 0 );

	// pi controled leds
	 lisy80_set_red_led(0);
	 lisy80_set_yellow_led(0);
	 lisy80_set_green_led(0);
	 lisy80_set_dp_led(0);

	//now all leds are off, except switch pic

	//init switch pic so that led blinks
	lisy80_switch_pic_init();

	//traffic lights	
	lisy80_set_red_led(1); sleep(1); lisy80_set_red_led(0); sleep(1);
	lisy80_set_yellow_led(1); sleep(1); lisy80_set_yellow_led(0); sleep(1);
	lisy80_set_green_led(1); sleep(1); lisy80_set_green_led(0); sleep(1);


	 lisy80_set_red_led(0);
	 lisy80_set_yellow_led(0);
	 lisy80_set_green_led(0);
	 sleep(1);
	 lisy80_set_red_led(1);
	 lisy80_set_yellow_led(1);
	 lisy80_set_green_led(1);
	 sleep(1);
	 lisy80_set_red_led(0);
	 lisy80_set_yellow_led(0);
	 lisy80_set_green_led(0);

	//display PIC
	lisy80_set_dp_led(1); sleep(1); lisy80_set_dp_led(0); sleep(1); lisy80_set_dp_led(1); sleep(1);

	//coil pic
	coil_led_set(1); sleep(1); coil_led_set(0); sleep(1); coil_led_set(1); sleep(1);

	//all green
	lisy80_set_green_led(1);


	exit(1);
}
