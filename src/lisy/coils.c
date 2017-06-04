/*
  coils.c
  part of lisy80
  bontango 01.2016
*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <wiringPi.h>
#include "fileio.h"
#include "hw_lib.h"
#include "utils.h"
#include "coils.h"
#include "externals.h"

//global var
union three {
    unsigned char byte;
    struct {
    unsigned COIL:6, ACTION:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;        
    struct {
    unsigned COMMAND:3, SOUNDS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;                
    } mydata_coil;

/*
  send command to coil PIC, no answer expected
*/

void coil_cmd2pic_noansw(unsigned char command)
{

        /* build control byte */
        mydata_coil.bitv2.COMMAND = command;
        mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );

        return;
}



/*
  send command to coil PIC and get answer
*/

int coil_cmd2pic(unsigned char command)
{

        /* build control byte */
        mydata_coil.bitv2.COMMAND = command;
        mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );

	//wait a bit, PIC migth be slow
        delay(300); //wait 100ms secs

        //read answer and send back
        return( lisy80_read_byte_coil_pic());
}


int coil_get_sw_version(void)
{
 int ver_main, ver_sub;

 ver_main = coil_cmd2pic(LS80COILCMD_GET_SW_VERSION_MAIN);
 ver_sub = coil_cmd2pic(LS80COILCMD_GET_SW_VERSION_SUB);

 return( ver_main * 100 + ver_sub);

}

int coil_get_k3(void)
{
 return(coil_cmd2pic(LS80COILCMD_GET_K3));
}


//sound set via PIC I2C com LISY80
void lisy80_coil_sound_set( int sound)
{

        /* build control byte */
        mydata_coil.bitv2.COMMAND = LS80COILCMD_SETSOUND;
        mydata_coil.bitv2.SOUNDS = sound;
        mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//sound set via PIC I2C com LISY1
void lisy1_coil_sound_set( int sound)
{
   //with SYstem 1 we have three coils for sound, meaning 7 possibilities for sound (3 bit)
   //we may do it the same way as for system80 later
        if ( sound & 0x01 ) lisy1_coil_set( Q_TENS, 1); else  lisy1_coil_set( Q_TENS, 0);
        if ( sound & 0x02 ) lisy1_coil_set( Q_HUND, 1); else  lisy1_coil_set( Q_HUND, 0);
        if ( sound & 0x04 ) lisy1_coil_set( Q_TOUS, 1); else  lisy1_coil_set( Q_TOUS, 0);

if ( ls80dbg.bitv.sound )
  {
      if ( sound & 0x01 ) lisy80_debug("sound Q_TENS to 1"); else lisy80_debug("sound Q_TENS to 0");
      if ( sound & 0x02 ) lisy80_debug("sound Q_HUND to 1"); else lisy80_debug("sound Q_HUND to 0");
      if ( sound & 0x04 ) lisy80_debug("sound Q_TOUS to 1"); else lisy80_debug("sound Q_TOUS to 0");
  }


}

//coil set via PIC I2C com
void coil_coil_set( int coil, int action)
{
	--coil;	//we have only 6 bit, so we start at zero for coil 1

        // build control byte 
        mydata_coil.bitv.COIL = coil;
        mydata_coil.bitv.ACTION = action;
        mydata_coil.bitv.IS_CMD = 0;        //this is a coil setting

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

/*
 set all Lamps and coils OFF LISY80
*/
void lisy80_coil_init ( void )
{
int i;

//put all lamps off
for (i=1; i<=64; i++)  
 {

  if ( i == 49 ) continue; //same as 45 but reversed
  if ( i == 50 ) continue; //same as 46 but reversed
  if ( i == 51 ) continue; //same as 47 but reversed
  if ( i == 52 ) continue; //same as 48 but reversed

  if ( i == 57 ) continue; // driver not used
  if ( i == 61 ) continue; // driver not used
  if ( i == 63 ) continue; // driver not used

  lisy80_coil_set ( i, 0 );
}


}

/*
 set all Lamps and coils OFF LISY1
*/
void lisy1_coil_init ( void )
{
int i;

//put all lamps & solenoids  off
for (i=1; i<=44; i++)  
  lisy80_coil_set ( i, 0 );

}


void coil_test ( void )
{
int i;
for (i=1; i<=64; i++)
	{
	 if ( i == 49 ) continue;
	 if ( i == 50 ) continue;
	 if ( i == 51 ) continue;
	 if ( i == 52 ) continue;
	 if ( i == 57 ) continue;
	 if ( i == 61 ) continue;
	 if ( i == 63 ) continue;
	 fprintf(stderr," coil#: %d set to 1\n",i);
         lisy80_coil_set ( i, 1 );
	 delay (200); // 200 milliseconds delay from wiringpi library
	 fprintf(stderr," coil#: %d set to 0\n",i);
         lisy80_coil_set ( i, 0 );
	 delay (200); // 200 milliseconds delay from wiringpi library
	}
}



/*
coil_set, for test routine only
	coil -> nr of coil
	action -> 0-off, 1-on, 2-pulse

void coil_set ( int coil, int action)
{
   switch(action)
	{
		case '\x0': lisy80_coil_set(coil,action);
			  break;
		case '\x1': lisy80_coil_set(coil,action);
			  break;
		case '\x2': lisy80_coil_set(coil,1);
			  usleep(COIL_PULSE_TIME);
			  lisy80_coil_set(coil,0);
			  break;
		default:
			  break;
	}


}
*/


void coil_set_str ( char *str, int action)
{
	int coil;

	 strtok( str, ",");
	 coil = atoi( strtok( NULL, ","));
	 lisy80_coil_set(coil,action);

}

//control the LED  set via PIC I2C com
void coil_led_set( int action)
{

        /* build control byte */
	if (action)
           mydata_coil.bitv2.COMMAND = LS80COILCMD_LED_ON;
	else
           mydata_coil.bitv2.COMMAND = LS80COILCMD_LED_OFF;
        mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}
