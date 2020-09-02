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
#include "displays.h"
#include "fadecandy.h"
#include "lisy_home.h"
#include "externals.h"
#include "lisy.h"

//global var
union five {
    unsigned char byte;
    struct {
    unsigned COIL:6, ACTION:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;        
    struct {
    unsigned COMMAND:3, SOUNDS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;                
    struct {
    unsigned LAMP:6, ACTION:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv3;                
    struct {
    unsigned COMMAND:3, COILS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv4;                
    struct {
    unsigned COMMAND:3, EXT_CMD:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv5;                
    } mydata_coil;

    //signal back to lisy35 once hw check is finished
    //done from green_led routine
    unsigned char lisy35_bally_hw_check_finished = 0;

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


//let PIC re read mpu dips
void lisy35_coil_read_mpu_dips(void)
{

    mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
    mydata_coil.bitv5.EXT_CMD = LISY35_READ_DIP_SWITCHES;

    //write to PIC
    lisy80_write_byte_coil_pic(  mydata_coil.byte );

    //wait a second to give PIC time
    sleep(1);
}

//get all mpu dips from PIC buffer
int lisy35_coil_get_mpu_dip(int dip_number)
{
 static unsigned char mpu_dip[4];
 int i;

 //if dip number == 0 we re read all mpu dips from PIC
 //as they might have changed
 if (dip_number == 0)
 {
   for ( i=0; i<=3; i++)
   {

    mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
    mydata_coil.bitv5.EXT_CMD = LISY35_GET_DIP_SWITCHES;

    //write to PIC
    lisy80_write_byte_coil_pic(  mydata_coil.byte );

    //wait a bit, PIC migth be slow
    delay(300); //wait 100ms secs

    //read answer
    mpu_dip[i]= lisy80_read_byte_coil_pic();

    if (ls80dbg.bitv.basic)
     {
       sprintf(debugbuf,"read mpu dip number:%d result:%d",i,mpu_dip[i]);
       lisy80_debug(debugbuf);
     }
    } //for i
 }//if dip_number == 0

 if(dip_number<4) return(mpu_dip[dip_number]);
  else return(-1);

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

//coil set via PIC I2C com
//we do this or lisy1 & lisy80
//for lisy_home we call lisy_home routine
//because there may be a mapping
void coil_coil_set( int coil, int action)
{

   //eventhandler for LISY_HOME?
   if ( lisy_hardware_ID == LISY_HW_ID_HOME)
         {
		lisy_home_event_handler( LISY_HOME_EVENT_COIL, coil, action, NULL);
	 }
   else
     {
        //now do the setting
	--coil;	//we have only 6 bit, so we start at zero for coil 1

        // build control byte 
        mydata_coil.bitv.COIL = coil;
        mydata_coil.bitv.ACTION = action;
        mydata_coil.bitv.IS_CMD = 0;        //this is a coil setting

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );
     }

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

/*
 set all coils OFF LISY35
*/
void lisy35_coil_init ( void )
{
 lisy35_mom_coil_set(15); //15 selects rest postion, all momentary coils off
 lisy35_cont_coil_set(15); //15 selects  all 1, as we ahe active low this puts off all solenoids
}

/*
 set all Lamps OFF LISY35
*/
void lisy35_lamp_init ( void )
{

 int i,nu;

   // board 0
   for ( i=0; i<=59; i++)
        {
         lisy35_lamp_set(0, i, 0 );
        }
   //check board 1
   if ( lisy35_game.aux_lamp_variant != NO_AUX_BOARD )
   {
    switch ( lisy35_game.aux_lamp_variant )
      {
        case AS_2518_43_12_LAMPS: nu = 11; break;
        case AS_2518_52_28_LAMPS: nu = 27; break;
        case AS_2518_23_60_LAMPS: nu = 59; break;
        default: nu=0; break;
      }
       for ( i=0; i<=nu; i++)
        {
         lisy35_lamp_set(1, i, 0 );
        }
     }
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
lisy35_coil_set, FOR TEST ROUTINE ONLY
	coil -> nr of coil
		1..15 momentary solenoids
		16 ..19 continous solenoids
	action -> 0-off, 1-on, 2-pulse
		pulse to ignore on continous solenoids ( >=16 )
*/

void lisy35_coil_set ( int coil, int action)
{
   //remeber state of cont solenoids
   static unsigned char cont_coil = 15;
   unsigned char position;
   

   switch(coil)
	{
		case 1 ... 15:
			      //make sure solenoids are selected
				lisy35_coil_set_sound_select(LISY35_SOLENOIDS_SELECTED);
			      if ( action == 2) //pulse
				{
				  lisy35_mom_coil_set(coil-1);
			          delay(COIL_PULSE_TIME);
				  lisy35_mom_coil_set(15); //15 selects rest postion, all coils off
				   //debug?
    			     if (  ls80dbg.bitv.coils )
    				{
     				 sprintf(debugbuf,"pulsed momentary solenoids: %d\n",coil-1);
     				 lisy80_debug(debugbuf);
     				}

				}	
			       else
				{
				  if ( action == 1) lisy35_mom_coil_set(coil-1);
				   else lisy35_mom_coil_set(15);
    			     	if (  ls80dbg.bitv.coils )
    				{
     				 sprintf(debugbuf,"momentary solenoids: %d %s\n",coil-1, action ? "ON" : "OFF");
     				 lisy80_debug(debugbuf);
     				}
				}	
			break;
		       case 16:
				position = 2;  // cont1 is PB6
				//active low
				if (action == 0) SET_BIT( cont_coil, position);
				  else CLEAR_BIT( cont_coil, position);
				//do the setting
				lisy35_cont_coil_set(cont_coil);
			        break;
		       case 17:
				position = 0;  // cont2 is PB4 'internal via PIC not used here'
				//active low
				if (action == 0) SET_BIT( cont_coil, position);
				  else CLEAR_BIT( cont_coil, position);
				//do the setting
				lisy35_cont_coil_set(cont_coil);
			        break;
		       case 18:
				position = 3;  // cont3 is PB7 'internal via PIC not used here'
				//active low
				if (action == 0) SET_BIT( cont_coil, position);
				  else CLEAR_BIT( cont_coil, position);
				//do the setting
				lisy35_cont_coil_set(cont_coil);
			        break;
		       case 19:
				position = 1;  // cont4 is PB5
				//active low
				if (action == 0) SET_BIT( cont_coil, position);
				  else CLEAR_BIT( cont_coil, position);
				//do the setting
				lisy35_cont_coil_set(cont_coil);
			        break;
	}


}


void coil_set_str ( char *str, int action)
{
	int coil;

	 strtok( str, ",");
	 coil = atoi( strtok( NULL, ","));
	 lisy80_coil_set(coil,action);

}

/*
//control the LED  set via PIC I2C com
//not needed anymore
void coil_led_set( int action)
{

        // build control byte 
	if (action)
           mydata_coil.bitv2.COMMAND = LS80COILCMD_LED_ON;
	else
           mydata_coil.bitv2.COMMAND = LS80COILCMD_LED_OFF;
        mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}
*/

//control the green (Bally) LED set via PIC I2C com
//only 8 times at boot time, as same line is strobe#2 for lamps afterwards
void coil_bally_led_set( int action)
{

 static unsigned char counter = 0;

   if (counter < 7 )
    {

        // build control byte
        mydata_coil.bitv3.LAMP = LISY35_COIL_GREEN_LED;  //special lamp
        mydata_coil.bitv3.IS_CMD = 0;        //this is a lamp setting
        mydata_coil.bitv3.ACTION = action;

        if (!action) //we count offs
          {
           counter++;
    		//debug?
    		if (  ls80dbg.bitv.basic )
    		{
     		sprintf(debugbuf,"Bally Green LED, flash No:%d",counter);
     		lisy80_debug(debugbuf);
    		}//debug
	  }

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

    }
   else lisy35_bally_hw_check_finished = 1;

}


//set continous  solenoid on LISY35
void lisy35_cont_coil_set( unsigned char value )
{
        /* build control byte */
        mydata_coil.bitv4.COMMAND = LS35COIL_CONT_SOL;
        mydata_coil.bitv4.COILS = value;
        mydata_coil.bitv4.IS_CMD = 1;   //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );

    //debug?
    if (  ls80dbg.bitv.coils )
    {
     char str[34];
     my_itoa(value, str, 2);
     sprintf(debugbuf,"set cont sol (PB4..7): %s",str);
     lisy80_debug(debugbuf);
    }//debug
}

//set momentary solenoid on LISY35
void lisy35_mom_coil_set( unsigned char value )
{
        /* build control byte */
        mydata_coil.bitv4.COMMAND = LS35COIL_MOM_SOL;
        mydata_coil.bitv4.COILS = value;
        mydata_coil.bitv4.IS_CMD = 1;   //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );
}


/*
	lisy80_coil_set
        coil -> nr of coil
        action -> 0-off, 1-on, 2-pulse
	lamps 1..48
	solenoid 53 .. 64
*/
void lisy80_coil_set( int coil, int action)
{
 unsigned int now;
 int mycoil, pulsetime;
 //remember the status of the coil, as we MAY need a minimum 'ON' time here
 static unsigned char coilstatus[10] = { 0,0,0,0,0,0,0,0,0,0 };
 static unsigned int coil_last_on[10];

  //at first check if we need to respect minimum pulse time
  //this version BLOCKS until min pulse time is reached
  //RTH: we may need to do the off in 'tickle' routine in later versions
 if (( lisy80_coil_min_pulse_mod ) && ( coil >48 ) )
 {

  //determine which coil we use
  switch( coil )
   {
    case Q_SOL1: mycoil = 1;
                break;
    case Q_SOL2: mycoil = 2;
                break;
    case Q_SOL3: mycoil = 3;
                break;
    case Q_SOL4: mycoil = 4;
                break;
    case Q_SOL5: mycoil = 5;
                break;
    case Q_SOL6: mycoil = 6;
                break;
    case Q_SOL7: mycoil = 7;
                break;
    case Q_SOL8: mycoil = 8;
                break;
    case Q_SOL9: mycoil = 9;
                break;
    default: mycoil = 0;
                break;
   }

    if ( action == 1)
        {
         coilstatus[ mycoil ] = 1;
         coil_last_on[ mycoil ] = millis();
        }
    else //action is 0 -> OFF, wait in case minimum PULSE time is not reached
        {
         if  ( ( coilstatus[ mycoil ] == 1 ) && ( lisy80_coil_min_pulse_time[mycoil] > 0 )) //only if it is not already off and min_pulse > 0
         {
           //see what time is now
           now = millis();
           //beware of 'wrap' which happens each 49 days
           if ( now < coil_last_on[mycoil] ) now = coil_last_on[mycoil]; //we had a wrap
           //calculate difference and sleep a bit
           pulsetime = ( lisy80_coil_min_pulse_time[mycoil] ) - ( now - coil_last_on[mycoil]);
           if (pulsetime > 0) delay( pulsetime );
           //debug?
           if (  ls80dbg.bitv.coils )
    		{
     		  sprintf(debugbuf,"info: pulsetimemod for solenoid %d: %d msec waited %d msec \n",mycoil,lisy80_coil_min_pulse_time[mycoil],pulsetime);
     		  lisy80_debug(debugbuf);
    		}//debug
         }
         //remember new status
         coilstatus[ mycoil ] = 0;
        }
 }//if lisy80_coil_min_pulse_mod


 if (coil <= 48)  //it is a lamp
  {
    if (ls80dbg.bitv.lamps)
     {
        sprintf(debugbuf,"setting lamp:%d to:%d",coil,action);
        lisy80_debug(debugbuf);
     }

   //do set fadecandy LED if active and lamp is mapped
   if ( lisy_has_fadecandy && lisy_lamp_to_led_map[coil-1].is_mapped )
    {
     if ( lisy_fadecandy_set_led(coil-1, action) <= 0)
      {
        lisy80_debug("ERROR in fadecandy set_led, will be deactivated");
	lisy_has_fadecandy = 0;
      }
     //option exclusive, if this is NOT set do ALSO set lamp 
     if ( !lisy_lamp_to_led_map[coil-1].exclusive ) coil_coil_set( coil, action);
    }
   else //no mapping, do normal lamp set
     coil_coil_set( coil, action);

  //special action in case we have lamp 45-48 as the system80 does map this fix (reverse) to lamps 49-52
  if ( ( coil >= 45 ) && ( coil <= 48 ) )
   {
   if ( lisy_has_fadecandy && lisy_lamp_to_led_map[coil-1+4].is_mapped )
    {
     if ( lisy_fadecandy_set_led(coil-1+4, action) <= 0)
      {
        lisy80_debug("ERROR in fadecandy set_led, will be deactivated");
	lisy_has_fadecandy = 0;
      }
     //option exclusive, if this is NOT set do ALSO set lamp (not needed here)
    }
   }//special handling lamps 44-47

  } //if coil <= 48
 else //it is a coil
  {
    if (ls80dbg.bitv.coils)
     {
        sprintf(debugbuf,"setting coil:%d to:%d (SOLENOID)",coil,action);
        lisy80_debug(debugbuf);
     }
     //do the coil setting
     coil_coil_set( coil, action);
  }
}

/*
	lisy1_coil_set
        coil -> nr of coil
        action -> 0-off, 1-on, 2-pulse
	lamps 1..36
	solenoid 37 .. 44

 we respect minimum PULSE_TIME_IN_MSEC for under playfield coils here!
 pulsetime for real coils are red by config file
 add 09.2017: lamp17&18 need also pulsetime for under playfield transistors
 for Joker Poker, Countdown & Buck Rogers
 we use a flag in game structur which is set in fileio.c
*/
#define	PULSE_TIME_IN_MSEC 150
void lisy1_coil_set( int coil, int action)
{

 unsigned int now;
 int mycoil, pulsetime;
 //remember the status of the coil, as we need a minimum 'ON' time here
 static unsigned char coilstatus[8] = { 0,0,0,0,0,0,0,0 };
 static unsigned int coil_last_on[8];
 static unsigned char lampstatus[37] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
 static unsigned int lamp_last_on[37];

 if (coil <= 36)  //it is a lamp
  {
    if (ls80dbg.bitv.lamps)
     {
        sprintf(debugbuf,"setting lamp:%d to:%d",coil,action);
        lisy80_debug(debugbuf);
     }

    //if this is about to set lamp to on, remember when that happened
    //in case we have one of our under playfiled mods
    if ( (( lisy1_game.lamp17_is_Coil == 1) && ( coil == 17)) || (( lisy1_game.lamp18_is_Coil == 1) && ( coil == 18)) )
    {
    if ( action == 1)
	{
	 lampstatus[ coil ] = 1;
	 lamp_last_on[ coil ] = millis();
	}
    else //action is 0 -> OFF, wait in case minimum PULSE time is not reached
	{
	 if  ( lampstatus[ coil ] == 1 ) //only if it is not already off
         {
	   //see what time is now
	   now = millis();
	   //beware of 'wrap' which happens each 49 days
	   if ( now < lamp_last_on[coil] ) now = lamp_last_on[coil]; //we had a wrap
	   //calculate difference and sleep a bit
	   pulsetime = ( PULSE_TIME_IN_MSEC ) - ( now - lamp_last_on[coil]);
	   if (pulsetime > 0) delay( pulsetime );
	 }
	 //remember new status
	 lampstatus[ coil ] = 0;
	}
      }//if under playfiled mod

   //do set fadecandy LED if active and lamp is mapped
   if ( lisy_has_fadecandy && lisy_lamp_to_led_map[coil-1].is_mapped )
    {
     if ( lisy_fadecandy_set_led(coil-1, action) <= 0)
      {
        lisy80_debug("ERROR in fadecandy set_led, will be deactivated");
        lisy_has_fadecandy = 0;
      }
     //option exclusive, if this is NOT set do ALSO set lamp 
     if ( !lisy_lamp_to_led_map[coil-1].exclusive ) coil_coil_set( coil, action);
    }
   else //no mapping, do normal lamp set
     coil_coil_set( coil, action);

  } //if coil <= 36
 else //it is a coil
  {
    //if this is about to set coil to on, remember when that happened
   mycoil = coil - 37; //mycoil index is from 0..7
   if (( mycoil ) <= 7) //range check
   {
    if ( action == 1)
	{
	 coilstatus[ mycoil ] = 1;
	 coil_last_on[ mycoil ] = millis();
	}
    else //action is 0 -> OFF, wait in case minimum PULSE time is not reached
	{
	 if  ( coilstatus[ mycoil ] == 1 ) //only if it is not already off
         {
	   //see what time is now
	   now = millis();
	   //beware of 'wrap' which happens each 49 days
	   if ( now < coil_last_on[mycoil] ) now = coil_last_on[mycoil]; //we had a wrap
	   //calculate difference and sleep according to set value
	   pulsetime = ( lisy1_coil_min_pulse_time[mycoil] ) - ( now - coil_last_on[mycoil]);
	   if (pulsetime > 0) delay( pulsetime );
	 }
	 //remember new status
	 coilstatus[ mycoil ] = 0;
	}
    }

    //debug
    if (ls80dbg.bitv.coils)
     {
	switch(coil)
    	{
        	case Q_KNOCK: 
        		if ( action ) lisy80_debug("Q_KNOCK on"); else lisy80_debug("Q_KNOCK OFF");
               	       break;
        	case Q_OUTH: 
        		if ( action ) lisy80_debug("Q_OUTH on"); else lisy80_debug("Q_OUTH OFF");
               	       break;
        	case Q_SYS1_SOL6: 
        		if ( action ) lisy80_debug("Q_SYS1_SOL6 on"); else lisy80_debug("Q_SYS1_SOL6 OFF");
               	       break;
        	case Q_SYS1_SOL7: 
        		if ( action ) lisy80_debug("Q_SYS1_SOL7 on"); else lisy80_debug("Q_SYS1_SOL7 OFF");
               	       break;
        	case Q_SYS1_SOL8: 
        		if ( action ) lisy80_debug("Q_SYS1_SOL8 on"); else lisy80_debug("Q_SYS1_SOL8 OFF");
               	       break;
        }
     }

    //add debug for sound, in case action == ON
    if ( (ls80dbg.bitv.sound) )
     {
	switch(coil)
    	{
        	case Q_TENS: 
        		if ( action ) lisy80_debug("play sound for Q_TENS"); else lisy80_debug("sound Q_TENS OFF");
               	       break;
        	case Q_HUND: 
        	        if ( action ) lisy80_debug("play sound for Q_HUND"); else lisy80_debug("sound Q_HUND OFF");
               	       break;
        	case Q_TOUS: 
        		if ( action ) lisy80_debug("play sound for Q_TOUS"); else lisy80_debug("sound Q_TOUS OFF");
               	       break;
         }
     }

     //do the setting
     coil_coil_set( coil, action);
  }
}

//for which lampboard is the next lamp number
//needed because we only have 6bits for the lamp command
void lisy35_activate_lampboard( int number)
{

 if (ls80dbg.bitv.lamps)
     {
       sprintf(debugbuf,"switch to lampboard:%d",number);
       lisy80_debug(debugbuf);
     }

        // build control byte
        mydata_coil.bitv3.LAMP = LISY35_COIL_LAMPBOARD;  //special lamp
        mydata_coil.bitv3.IS_CMD = 0;        //this is a lamp setting
        mydata_coil.bitv3.ACTION = number;

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

/*
lisy35_lamp_set
	board 0 or 1
        lamp -> nr of lamp:  0..59
        action -> 0-off, 1-on
*/

void lisy35_lamp_set ( int board, int lamp, int action)
{

 static unsigned char active_lampboard = 0;

 //check for which board the lamp setting is
 if(board == 0) //board0 main board
 {
  //what is the active lampboard, do we need to switch?
  if(active_lampboard == 1) { lisy35_activate_lampboard(0), active_lampboard = 0; }
 }
 else //board 1
 {
  //what is the active lampboard, do we need to switch?
  if(active_lampboard == 0) { lisy35_activate_lampboard(1), active_lampboard = 1; }
 }
 
 if (ls80dbg.bitv.lamps)
     {
       sprintf(debugbuf,"board:%d set lamp %d to %d",active_lampboard,lamp,action);
       lisy80_debug(debugbuf);
     }

        // build control byte
        mydata_coil.bitv3.LAMP = lamp;
        mydata_coil.bitv3.ACTION = action;
        mydata_coil.bitv3.IS_CMD = 0;        //this is a lamp setting


        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );
}


//control the sound/solenoid select line
void lisy35_coil_set_sound_select( unsigned char value)
{

        // build control byte
        mydata_coil.bitv3.LAMP = LISY35_COIL_SOUNDSELECT;  //special lamp
        mydata_coil.bitv3.IS_CMD = 0;        //this is a lamp setting
        mydata_coil.bitv3.ACTION = value;

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//control the standard sound raw/cooked
void lisy35_coil_set_sound_raw( unsigned char value)
{

        // build control byte
        mydata_coil.bitv3.LAMP = LISY35_COIL_SOUNDRAW;  //special lamp
        mydata_coil.bitv3.IS_CMD = 0;        //this is a lamp setting
        mydata_coil.bitv3.ACTION = value;

        //write to PIC
        lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//set the type of teh extended SB installed
// 0 sets to 2581-51 (default)
// 1 sets to S&T 
void lisy35_coil_set_extended_SB_type( unsigned char type)
{

 if (ls80dbg.bitv.basic)
     {
       if (type)  lisy80_debug("Soundboard is a S&T");
        else lisy80_debug("Soundboard is a 2581-51");
     }

  mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
  mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
  if(type)
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_SB_IS_SAT;
  else
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_SB_IS_51;

  //write to PIC
  lisy80_write_byte_coil_pic(  mydata_coil.byte );

}





//set the direction (input/output) of J4PIN5 of coil pic
// 1 sets to input (default)
// 0 sets to output 'use with caution'
void lisy35_coil_set_direction_J4PIN5( unsigned char direction)
{

 if (ls80dbg.bitv.basic)
     {
       sprintf(debugbuf,"J4PIN5 set to %s",direction ? "input" : "output");
       lisy80_debug(debugbuf);
     }

  mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
  mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
  if(direction)
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J4PIN5_INPUT;
  else
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J4PIN5_OUTPUT;

  //write to PIC
  lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//set the direction (input/output) of J4PIN8 of coil pic
// 1 sets to input (default)
// 0 sets to output 'use with caution'
void lisy35_coil_set_direction_J4PIN8( unsigned char direction)
{

 if (ls80dbg.bitv.basic)
     {
       sprintf(debugbuf,"J4PIN8 set to %s",direction ? "input" : "output");
       lisy80_debug(debugbuf);
     }

  mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
  mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
  if(direction)
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J4PIN8_INPUT;
  else
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J4PIN8_OUTPUT;

  //write to PIC
  lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//set the direction (input/output) of J1PIN8 of coil pic
// 1 sets to input (default)
// 0 sets to output 'use with caution'
void lisy35_coil_set_direction_J1PIN8( unsigned char direction)
{

 if (ls80dbg.bitv.basic)
     {
       sprintf(debugbuf,"J1PIN8 set to %s",direction ? "input" : "output");
       lisy80_debug(debugbuf);
     }

  mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
  mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
  if(direction)
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J1PIN8_INPUT;
  else
     mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_J1PIN8_OUTPUT;

  //write to PIC
  lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//set the AUX Lampdriverboard variant
// NO_AUX_BOARD 0  // no aux board
// AS_2518_43_12_LAMPS 1  // AS-2518-43 12 lamps
// AS_2518_52_28_LAMPS 2  // AS-2518-52 28 lamps
// AS_2518_23_60_LAMPS 3  // AS-2518-23 60 lamps
// AS_2518_147_LAMP_COMBO 4 // Combination Solenoid / Lamp Driver Board
void lisy35_coil_set_lampdriver_variant( unsigned char variant)
{

 switch(variant)
  {
   case NO_AUX_BOARD:
       sprintf(debugbuf,"game has no AUX Lampdriverboard");
       mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_AUX_BOARD_0;
       break;
   case AS_2518_43_12_LAMPS:
       sprintf(debugbuf,"game has a AS-2518-43 12 lamps AUX Lampdriverboard");
       mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_AUX_BOARD_1;
       break;
   case AS_2518_52_28_LAMPS:
       sprintf(debugbuf,"game has a AS-2518-52 28 lamps AUX Lampdriverboard");
       mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_AUX_BOARD_2;
       break;
   case AS_2518_23_60_LAMPS:
       sprintf(debugbuf,"game has a AS-2518-23 60 lamps AUX Lampdriverboard");
       mydata_coil.bitv5.EXT_CMD = LISY35_EXT_CMD_AUX_BOARD_3;
       break;
   case AS_2518_147_LAMP_COMBO:
       sprintf(debugbuf,"game has a AS-2518-147 30 lamps Lampdriverboard, settings to board 1 ignored");
       break;
  }

 //print debugbuf only in case of debug ;-)
 if (ls80dbg.bitv.basic)
       lisy80_debug(debugbuf);

  mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
  mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;

  //write to PIC
  lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//send sound data for standard soundboard
void lisy35_sound_std_sb_set( unsigned char value )
{
        /* build control byte */
        mydata_coil.bitv4.COMMAND = LISY35_STANDARD_SOUND;
        mydata_coil.bitv4.COILS = value;
        mydata_coil.bitv4.IS_CMD = 1;   //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );
}


//send sound data for extended soundboard
//need to be send twice each time
void lisy35_sound_ext_sb_set( unsigned char value )
{
        /* build control byte */
        mydata_coil.bitv4.COMMAND = LISY35_EXTENDED_SOUND;
        mydata_coil.bitv4.COILS = value;
        mydata_coil.bitv4.IS_CMD = 1;   //we are sending a command here

        //write to PIC
        lisy80_write_byte_coil_pic( mydata_coil.byte );
}

//lisy home, select solenoidboard
void lisyh_coil_select_solenoid_driver(void)
{

    mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
    mydata_coil.bitv5.EXT_CMD = LISYH_EXT_CMD_FIRST_SOLBOARD;

    //write to PIC
    lisy80_write_byte_coil_pic(  mydata_coil.byte );

}

//lisy home, select lampdriver
void lisyh_coil_select_lamp_driver(void)
{

    mydata_coil.bitv5.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv5.COMMAND = LS80COILCMD_EXT_CMD_ID;
    mydata_coil.bitv5.EXT_CMD = LISYH_EXT_CMD_LED_ROW_1;

    //write to PIC
    lisy80_write_byte_coil_pic(  mydata_coil.byte );

}
