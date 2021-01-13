/*
 communication routines for LISY API
 May 2020
 bontango
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "lisy_api.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "fadecandy.h"
#include "opc.h"
#include "utils.h"
#include "lisy_home.h"
#include "externals.h"
#include "lisy_api_com.h"


//local vars
int fd_api;
static long lisy_api_counter = 0;

//lisy routine for writing to usb serial device
//we do it here in order to beable to log all bytes send to APC
//exceptions are 'init' and switch poll routine 'lisy_api_ask_for_changed_switch'
int lisy_api_write( unsigned char *data, int count, int debug  )
{

    int i;
    char helpstr[10];

    //do some statistics
    lisy_api_counter += count;


   if ( debug == 63 ) //only with full debug
   {
     sprintf(debugbuf,"API_write(%d bytes):",count);
     for(i=0; i<count; i++)
     {
       sprintf(helpstr," 0x%02x",data[i]);
       strcat(debugbuf,helpstr);
     }
    lisy80_debug(debugbuf);

    //print statistics each 1000 bytes
    if ( lisy_api_counter > 1000)
    {
     sprintf(debugbuf,"STATISTICS: API_write %ld bytes since last log",lisy_api_counter);
     lisy80_debug(debugbuf);
     lisy_api_counter = 0;
    }
   }

   return( write( fd_api,data,count));
}


//send command cmd
//read \0 terminated string and store into string content
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive string
int lisy_api_read_string(unsigned char cmd, char *content)
{

  char nextbyte;
  int i,n,ret;

 //send command
 if ( lisy_api_write( &cmd,1,ls80dbg.bitv.basic) != 1)
    {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
    }
 
 //receive answer
 i=0;
 do {
  if ( ( ret = read(fd_api,&nextbyte,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }
  content[i] = nextbyte;
  if ( ls80dbg.byte >= 63 ) //only with full debug
  {
    sprintf(debugbuf,"API_read_string: Byte no %d is (0x%02x)\"%c\"",i,nextbyte,nextbyte);
    lisy80_debug(debugbuf);
  }
  i++;
  } while ( nextbyte != '\0');

  //USB debug?
  if ( ls80dbg.byte >= 63 ) //only with full debug
  {
    sprintf(debugbuf,"API_read_string: %s",content);
    lisy80_debug(debugbuf);
  }

 return(i);

}

//read one byte, and return data into *data 
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive byte
//return 0 otherwise
unsigned char lisy_api_read_byte(unsigned char cmd, unsigned char *data)
{

 //send command
 if ( lisy_api_write( &cmd,1,ls80dbg.bitv.basic) != 1) return (-2);

 //receive answer
 if ( read(fd_api,data,1) != 1) return (-1);

  //USB debug?
  if ( ls80dbg.byte >= 63 ) //only with full debug
  {
    sprintf(debugbuf,"API_read_byte: 0x%02x",*data);
    lisy80_debug(debugbuf);
  }

 return(0);

}

//read one byte, and return data into *data
//blocking version, we wait 5 seconds before send error back
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive byte
//return 0 otherwise
unsigned char lisy_api_read_byte_wblock(unsigned char cmd, unsigned char *data)
{
 uint8_t tries = 0;
 int ret;

 //send command
 if ( lisy_api_write( &cmd,1,ls80dbg.bitv.basic) != 1) return (-2);

 //receive answer ; 50 tries with 100msec driver timeout
 while ( tries < 50)
 {
  ret = read(fd_api,data,1);
  if ( ret == 0) tries++;
  else if ( ret == 1)
   {
     //USB debug?
     if ( ls80dbg.byte >= 63 ) //only with full debug
     {
       sprintf(debugbuf,"API_read_byte: 0x%02x",*data);
       lisy80_debug(debugbuf);
     }
     return(0);
   }
  else
   {
     //USB debug?
     if ( ls80dbg.byte >= 63 ) //only with full debug
     {
       sprintf(debugbuf,"API_read_byte_wblock returned %d",ret);
       lisy80_debug(debugbuf);
     }
     return(-1);
   }
  } //while tries < 50

   //USB debug?
     if ( ls80dbg.byte >= 63 ) //only with full debug
     {
       sprintf(debugbuf,"API_read_byte_wblock timeout occured after %d tries",tries);
       lisy80_debug(debugbuf);
     }
     return(-1);
 }

//this command has an option
//read answer of two byte, and return data into *data1 and *data2
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive byte
//return 0 otherwise
unsigned char lisy_api_read_2bytes(unsigned char cmd, unsigned char option, unsigned char *data1, unsigned char *data2)
{

 unsigned char cmd_data[2];
 //send command
 cmd_data[0] = cmd;
 cmd_data[1] = option;
 if ( lisy_api_write( cmd_data,2,ls80dbg.bitv.basic) != 2) return (-2);

 //receive answer
 if ( read(fd_api,data1,1) != 1) return (-1);
 if ( read(fd_api,data2,1) != 1) return (-1);

  //USB debug?
  if ( ls80dbg.byte >= 63 ) //only with full debug
  {
    sprintf(debugbuf,"API_read_2bytes: 0x%02x 0x%02x",*data1,*data2);
    lisy80_debug(debugbuf);
  }

 return(0);

}


//print some usefull parameters
int lisy_api_print_hw_info(void)
{
   unsigned char data,cmd,data1,data2,no_of_displays,no;
   char answer[80];
   int i,j,n,ret;
   unsigned char my_switch_status[80];

    if ( lisy_api_read_string(LISY_G_LISY_VER, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client has SW version: %s",answer);
    lisy80_debug(debugbuf);

    if ( lisy_api_read_string(LISY_G_API_VER, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client uses API Version: %s",answer);
    lisy80_debug(debugbuf);

    if ( lisy_api_read_byte(LISY_G_NO_LAMPS, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d lamps",data);
    lisy80_debug(debugbuf);

    if ( lisy_api_read_byte(LISY_G_NO_SOL, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d solenoids",data);
    lisy80_debug(debugbuf);

    if ( lisy_api_read_byte(LISY_G_NO_DISP, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d displays",data);
    lisy80_debug(debugbuf);
    no_of_displays = data;

    for(no=0; no<no_of_displays; no++)
	{
	  //query each display
          if ( lisy_api_read_2bytes(LISY_G_DISP_DETAIL, no, &data1, &data2) < 0) return (-1);
	  // verbose type
	  switch(data1)
	  {
		case 0: strcpy(answer,"Display index is invalid or does not exist in machine."); break;
		case 1: strcpy(answer,"BCD7, BCD Code for 7 Segment Displays without comma"); break;
		case 2: strcpy(answer,"BCD8, BCD Code for 8 Segment Displays (same as BCD7 but with comma"); break;
		case 3: strcpy(answer,"SEG7, Fully addressable 7 Segment Display (with comma)"); break;
		case 4: strcpy(answer,"SEG14,Fully addressable 14 Segment Display (with comma)"); break;
		case 5: strcpy(answer,"ASCII, ASCII Code"); break;
		case 6: strcpy(answer,"ASCII_DOT, ASCII Code with comma"); break;
	  }
    	  sprintf(debugbuf,"Display no:%d has type:%d (%s) with %d segments",no,data1,answer,data2);
    	  lisy80_debug(debugbuf);
	}

    if ( lisy_api_read_byte(LISY_G_NO_SW, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d switches",data);
    lisy80_debug(debugbuf);

    //get status of switches
    for (i=1; i<=64; i++) my_switch_status[i] = lisy_api_get_switch_status(i);

    //print status of switches
    fprintf(stderr,"Switch Status 1..8;9..16;17..24; ...:\n");
    for(i=0; i<=8; i++)
     {
       for(j=1; j<=8; j++)
	{
	 n = j + i*8;
	 fprintf(stderr,"%d ",my_switch_status[i]);
	}
      fprintf(stderr,"\n");
     }//i
    
     //advance & auto up
      fprintf(stderr,"advance: %d\n",lisy_api_get_switch_status(72));
      fprintf(stderr,"UP/Down: %d\n",lisy_api_get_switch_status(73));

/*  
    if ( read_string(LISY_G_GAME_INFO, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Game Info: %s",answer);
    lisy80_debug(debugbuf);
*/


 return 0;

}


//send SEG7 data to a display
int lisy_api_send_SEG7_to_disp(unsigned char disp, int num, uint16_t *data)
{

 unsigned char hi, lo;
 unsigned char cmd, i;
 int len,pos;
 unsigned char seg7_data[20];
 map_byte1_t apc1, pinmame1;

 cmd = 255;
 switch(disp)
 {
        case 0: cmd = LISY_S_DISP_0; break;
        case 1: cmd = LISY_S_DISP_1; break;
        case 2: cmd = LISY_S_DISP_2; break;
        case 3: cmd = LISY_S_DISP_3; break;
        case 4: cmd = LISY_S_DISP_4; break;
        case 5: cmd = LISY_S_DISP_5; break;
        case 6: cmd = LISY_S_DISP_6; break;
 }

 if( cmd != 255 )
 {
  pos = 0;
  //construct data
  //command
  seg7_data[pos++] = cmd;
  //send length of byte sequence
  len = num; //we send one byte per data
  seg7_data[pos++] = len;

  //send SEG7 data
  //we need a mapping
  for ( i=0; i<num; i++)
  {

   //low byte first
    pinmame1.byte = data[i] & 0xFF;
    //do map
    apc1.bitv_apc.a = pinmame1.bitv_pinmame.a;
    apc1.bitv_apc.b = pinmame1.bitv_pinmame.b;
    apc1.bitv_apc.c = pinmame1.bitv_pinmame.c;
    apc1.bitv_apc.d = pinmame1.bitv_pinmame.d;
    apc1.bitv_apc.e = pinmame1.bitv_pinmame.e;
    apc1.bitv_apc.f = pinmame1.bitv_pinmame.f;
    apc1.bitv_apc.g = pinmame1.bitv_pinmame.g;
    apc1.bitv_apc.comma = pinmame1.bitv_pinmame.FREE;
    //assign
    seg7_data[pos++] = apc1.byte;
  }
 }

//debug?
if ( ls80dbg.bitv.displays )
   {
    sprintf(debugbuf,"send cmd %d to Display %d: %d bytes of SEG7 data",cmd,disp,len);
    lisy80_debug(debugbuf);
   }

 //send it all
 if ( lisy_api_write( seg7_data,len+2,ls80dbg.bitv.displays) != len+2) { fprintf(stderr,"ERROR write display\n"); return (-2); }

 return (len+2);

}

//send SEG14 data to a display
int lisy_api_send_SEG14_to_disp(unsigned char disp, int num, uint16_t *data)
{

 unsigned char hi, lo;
 unsigned char cmd, i;
 int len,pos;
 unsigned char seg14_data[40];
 map_byte1_t apc1, pinmame1;
 map_byte2_t apc2, pinmame2;

 cmd = 255;
 switch(disp)
 {
        case 0: cmd = LISY_S_DISP_0; break;
        case 1: cmd = LISY_S_DISP_1; break;
        case 2: cmd = LISY_S_DISP_2; break;
        case 3: cmd = LISY_S_DISP_3; break;
        case 4: cmd = LISY_S_DISP_4; break;
        case 5: cmd = LISY_S_DISP_5; break;
        case 6: cmd = LISY_S_DISP_6; break;
 }

 if( cmd != 255 )
 {
  pos = 0;
  //construct data
  //command
  seg14_data[pos++] = cmd;
  //send length of byte sequence
  len = num * 2; //we send two byte per data
  seg14_data[pos++] = len;
  //send SEG14 data
  //LISYAPI 0.09
  //Unser Mapping ist d, c, b, a, e, f, g, Komma für's erste Byte 
  //und j, h, m, k, p, r , Punkt, n fuer's zweite. 
  //pinmame has
  //2 bytes (a-g encoded as bit 0 to 6 in first byte. 
  //h to r encoded as bit 0 to 6 in second byte. comma as bit 7 in second byte)
  //so we need to remap the segments
  for ( i=0; i<num; i++)
  {
    
    //low byte first
    pinmame1.byte = data[i] & 0xFF;
    //do map
    apc1.bitv_apc.a = pinmame1.bitv_pinmame.a;
    apc1.bitv_apc.b = pinmame1.bitv_pinmame.b;
    apc1.bitv_apc.c = pinmame1.bitv_pinmame.c;
    apc1.bitv_apc.d = pinmame1.bitv_pinmame.d;
    apc1.bitv_apc.e = pinmame1.bitv_pinmame.e;
    apc1.bitv_apc.f = pinmame1.bitv_pinmame.f;
    apc1.bitv_apc.g = pinmame1.bitv_pinmame.g;
    apc1.bitv_apc.comma = pinmame1.bitv_pinmame.FREE;
    //assign
    seg14_data[pos++] = apc1.byte;

    //high byte in second byte
    pinmame2.byte = data[i] >> 8;
    //do map
    apc2.bitv_apc.h = pinmame2.bitv_pinmame.h;
    apc2.bitv_apc.j = pinmame2.bitv_pinmame.j;
    apc2.bitv_apc.k = pinmame2.bitv_pinmame.k;
    apc2.bitv_apc.m = pinmame2.bitv_pinmame.m;
    apc2.bitv_apc.n = pinmame2.bitv_pinmame.n;
    apc2.bitv_apc.p = pinmame2.bitv_pinmame.p;
    apc2.bitv_apc.r = pinmame2.bitv_pinmame.r;
    apc2.bitv_apc.dot = pinmame2.bitv_pinmame.dot;


    seg14_data[pos++] = apc2.byte;
  }
 }

//debug?
if ( ls80dbg.bitv.displays )
   {
    sprintf(debugbuf,"send cmd %d to Display %d: %d bytes of SEG14 data",cmd,disp,len);
    lisy80_debug(debugbuf);
   }

 //send it all
 if ( lisy_api_write( seg14_data,len+2,ls80dbg.bitv.displays) != len+2) { fprintf(stderr,"ERROR write display\n"); return (-2); }

 return (len+2);

}


//send a string to a display
//new with api 0.09: we send 'len' number of bytes, not a null terminated string
int lisy_api_send_str_to_disp(unsigned char num, char *str)
{

 unsigned char cmd,len,i;
 unsigned char cmd_data[20];

 cmd = 255;
 switch(num)
 {
	case 0: cmd = LISY_S_DISP_0; break;
	case 1: cmd = LISY_S_DISP_1; break;
	case 2: cmd = LISY_S_DISP_2; break;
	case 3: cmd = LISY_S_DISP_3; break;
	case 4: cmd = LISY_S_DISP_4; break;
	case 5: cmd = LISY_S_DISP_5; break;
	case 6: cmd = LISY_S_DISP_6; break;
 }

 if ( ls80dbg.bitv.displays )
   {
    sprintf(debugbuf,"send cmd %d to Display %d: str: %s",cmd,num,str);
    lisy80_debug(debugbuf);
   }


 if( cmd != 255 )
 {
 //send command
 cmd_data[0] = cmd;
 //send length of byte sequence
 len = cmd_data[1] = strlen(str);
 //send bytes of string
 for(i=0; i<len; i++) cmd_data[i+2] = str[i];

 if ( lisy_api_write( cmd_data,len+2,ls80dbg.bitv.displays) != len+2) { fprintf(stderr,"ERROR write display\n"); return (-2); }
 }

 return (len+2);
}

//ask for changed switches
unsigned char lisy_api_ask_for_changed_switch(void)
{

 unsigned char my_switch,cmd;
 int ret;

 //do some statistics
 lisy_api_counter++;

 //send command
 cmd = LISY_G_CHANGED_SW;
 if ( write( fd_api,&cmd,1) != 1) return (-2);
//receive answer
 if ( read(fd_api,&my_switch,1) != 1) return (-1);

 //USB debug? only if reurn is not 0x7f == no switch changed
 if((ls80dbg.bitv.switches) & ( my_switch != 0x7f))
 {
    sprintf(debugbuf,"API_write: 0x%02x",cmd);
    lisy80_debug(debugbuf);
    sprintf(debugbuf,"API_read_byte: 0x%02x",my_switch);
    lisy80_debug(debugbuf);
 }

 return my_switch;

}

//get status of specific switch
unsigned char lisy_api_get_switch_status( unsigned char number)
{

 int ret;
 unsigned char cmd,status;
 unsigned char cmd_data[2];

     cmd = LISY_G_STAT_SW;

  //send cmd
  cmd_data[0] = cmd;
  //send switch number
  cmd_data[1] = number;

      //send cmd
     if ( lisy_api_write( cmd_data,2,ls80dbg.bitv.switches) != 2)
      {
        printf("Error get switch status writing to serial\n");
        return -1;
      }

 //receive answer
  if ( ( ret = read(fd_api,&status,1)) != 1)
    {
        printf("Error reading from serial switch status, return:%d %s\n",ret,strerror(errno));
        return -1;
    }

 return status;

}

//lamp control
void lisy_api_lamp_ctrl(int lamp_no,unsigned char action)
{
 uint8_t cmd;
 unsigned char cmd_data[2];

  if (action) cmd=LISY_S_LAMP_ON; else cmd=LISY_S_LAMP_OFF;

  //send cmd
  cmd_data[0] = cmd;
  //send lamp number
  cmd_data[1] = lamp_no;

  if ( lisy_api_write( cmd_data,2,ls80dbg.bitv.lamps) != 2)
        fprintf(stderr,"Lamps Error writing to serial %s\n",strerror(errno));
}

//SOLENOID CONTROL
//solenoid ON and OFF
void lisy_api_sol_ctrl(int sol_no,unsigned char action)
{
 uint8_t cmd;
 unsigned char cmd_data[2];

  if (action) cmd=LISY_S_SOL_ON; else cmd=LISY_S_SOL_OFF;

  //send cmd
  cmd_data[0] = cmd;
  //send lamp number
  cmd_data[1] = sol_no;

  if ( lisy_api_write( cmd_data,2,ls80dbg.bitv.coils) != 2)
        fprintf(stderr,"Solenoids Error writing to serial %s\n",strerror(errno));


}

//pulse solenoid
void lisy_api_sol_pulse(int sol_no)
{
 uint8_t cmd;
 unsigned char cmd_data[2];

  cmd=LISY_S_PULSE_SOL;

  //send cmd
  cmd_data[0] = cmd;
  //send lamp number
  cmd_data[1] = sol_no;

  if ( lisy_api_write( cmd_data,2,ls80dbg.bitv.coils) != 2)
        fprintf(stderr,"Solenoids Error writing to serial %s\n",strerror(errno));
}

//set HW rule for solenoid
//RTH minimal version for the moment
void lisy_api_sol_set_hwrule(int sol_no, int special_switch)
{
 uint8_t cmd;
 unsigned char cmd_data[11];

  cmd_data[0] = LISY_S_SET_HWRULE;
  cmd_data[1] = sol_no; //Index c of the solenoid to configure
  cmd_data[2] = special_switch; //Switch sw1. Set bit 7 to invert the switch.
  cmd_data[3] = 127; //Switch sw2. Set bit 7 to invert the switch.
  cmd_data[4] = 127;  //Switch sw3. Set bit 7 to invert the switch.
  cmd_data[5] = 80; //Pulse time in ms (0-255)
  cmd_data[6] = 191; ///Pulse PWM power (0-255). 0=0% power. 255=100% power 75% not used by APC
  cmd_data[7] = 64;  //Hold PWM power (0-255). 0=0% power. 255=100% power 25% not used by APC
  cmd_data[8] = 3; //sw1 will enable the rule and disable it when released.
  cmd_data[9] = 0; //do not use sw2
  cmd_data[10] = 0; //do not use sw3

  if ( ls80dbg.bitv.basic ) 
  {
    sprintf(debugbuf,"LISY_Mini: HW Rule set for solenoid:%d and switch:%d",sol_no,special_switch);
    lisy80_debug(debugbuf);
  }

  //11 bytes to follow
  if ( lisy_api_write( cmd_data,11,ls80dbg.bitv.basic) != 11)
        fprintf(stderr,"Setting hW rules, Error writing to serial\n");
}

//display control
//set a protocol for a display
// displayno is 0 to ... max displays -1
// option is
// 1: BCD7, BCD Code for 7 Segment Displays without comma
// 2: BCD8, BCD Code for 8 Segment Displays (same as BCD7 but with comma
// 3: SEG7, Fully addressable 7 Segment Display (with comma)
// 4: SEG14,Fully addressable 14 Segment Display (with comma)
// 5: ASCII, ASCII Code
// 6: ASCII_DOT, ASCII Code with comma
void lisy_api_display_set_prot(uint8_t display_no,uint8_t protocol)
{
	uint8_t cmd;
        unsigned char cmd_data[3];

   cmd = LISY_S_DISP_PROT;

    //send cmd
    cmd_data[0] = cmd;
    //send display number
    cmd_data[1] = display_no;
    //send protocol number
    cmd_data[2] = protocol;

     if ( lisy_api_write( cmd_data,3,ls80dbg.bitv.displays) != 3)
        fprintf(stderr,"display option Error writing to serial\n");
}


//play sound (index) API 0x32
void lisy_api_sound_play_index( unsigned char board, unsigned char index )
{
        uint8_t cmd;
        uint8_t data;
        int ret;
        unsigned char cmd_data[3];

 if ( ls80dbg.bitv.sound )
  {
    sprintf(debugbuf,"play soundindex %d on board %d ",index, board);
    lisy80_debug(debugbuf);
  }

     cmd = LISY_S_PLAY_SOUND;

     //send cmd
     cmd_data[0] = cmd;
     //track defines the board, track 1 -> board 0; track 2 -> board 2
     cmd_data[1] = board + 1;
     //no flags
     cmd_data[2] = index;

     if ( lisy_api_write( cmd_data,3,ls80dbg.bitv.sound) != 3)
        fprintf(stderr,"sound play file error writing to serial\n");

     //check if remote side is ready to receive next command
    ret = lisy_api_read_byte_wblock(LISY_BACK_WHEN_READY, &data);
    if ( ret < 0)
     {
  	if(ls80dbg.bitv.basic)
  	{ 
    	  sprintf(debugbuf,"Error: LISY_BACK_WHEN_READY: returned %d",ret);
    	  lisy80_debug(debugbuf);
  	}
     }
    else
     {
  	if( ls80dbg.bitv.basic & ( data != 0) )
  	{
    	 sprintf(debugbuf,"Error: LISY_BACK_WHEN_READY: returned 0x%02x",data);
    	 lisy80_debug(debugbuf);
  	}

     }

}


//play soundfile API 0x34
void lisy_api_sound_play_file( unsigned char board, char *filename )
{
	uint8_t cmd,data;
	int i,len,ret;
	unsigned char cmd_data[80];

 if ( ls80dbg.bitv.sound )
  {
    sprintf(debugbuf,"play sound %s",filename);
    lisy80_debug(debugbuf);
  }

     cmd = LISY_S_PLAY_FILE;

     //send cmd
     cmd_data[0] = cmd;
     //track defines the board, track 1 -> board 0; track 2 -> board 2
     cmd_data[1] = board + 1;
     //no flags
     cmd_data[2] = 0;
     //len of filename
     len = strlen(filename);
     //filename plus trailing \0
     for(i=0; i<=len; i++) cmd_data[i+3] = filename[i];

     if ( lisy_api_write( cmd_data,len+4,ls80dbg.bitv.sound) != len+4)
        fprintf(stderr,"sound play file error writing to serial\n");


     //check if remote side is ready to receive next command
    ret = lisy_api_read_byte_wblock(LISY_BACK_WHEN_READY, &data);
    if ( ret <= 0)
     {
        if(ls80dbg.bitv.basic)
        {
         if (ret == 0)
         {
          sprintf(debugbuf,"Error: LISY_BACK_WHEN_READY: timeout occured");
          lisy80_debug(debugbuf);
         }
         else
         {
          sprintf(debugbuf,"Error: LISY_BACK_WHEN_READY: returned %d",ret);
          lisy80_debug(debugbuf);
         }
        }
     }
    else
     {
        if( ls80dbg.bitv.basic & ( data != 0) )
        {
         sprintf(debugbuf,"LISY_BACK_WHEN_READY: returned 0x%02x",data);
         lisy80_debug(debugbuf);
        }

     }

}


//this is the boot message for lisy Mini
void lisy_api_show_boot_message(char *software_version,char *system_id, int game_no, char *gamename)
{
        int i;
        char buf[22];


  //show 'system_id' on display one -> "LISY_W"
  sprintf(buf,"%-6s",system_id);
  lisy_api_send_str_to_disp( 1, buf);
  if(ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"Info_boot: System_ID: %s",system_id);
    lisy80_debug(debugbuf);
  }


  //show 'gamename' on display two
  sprintf(buf,"%-6s",gamename);
  lisy_api_send_str_to_disp( 2, buf);
  if(ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"Info_boot: gamename: %s",gamename);
    lisy80_debug(debugbuf);
  }

  //show S2 Setting on display three
  sprintf(buf,"S2 %03d",game_no);
  lisy_api_send_str_to_disp( 3, buf);
  if(ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"Info_boot: S2 Setting: %d",game_no);
    lisy80_debug(debugbuf);
  }

  //show Version number on Display 4
  lisy_api_send_str_to_disp( 4, software_version);
  if(ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"Info_boot: SW Version: %s",software_version);
    lisy80_debug(debugbuf);
  }


  //status display countdown
  for (i=5; i>=0; i--)
   {
     sprintf(buf,"  %02d",i);
     lisy_api_send_str_to_disp( 0, buf);
     sleep(1);
    }

  //blank status display
  lisy_api_send_str_to_disp( 0, "    ");
}

//get Info about connected Hardware
int lisy_api_get_con_hw( char *idstr )
{
 if ( ls80dbg.bitv.basic )
  {
    lisy80_debug("ask for connected hardware");
  }

  return (lisy_api_read_string(LISY_G_HW, idstr));
}

//get status of (external or virtuel) DIP switch
unsigned char lisy_api_get_dip_switch( unsigned char number)
{

  int ret;
  unsigned char cmd,status;
  unsigned char cmd_data[2];

  cmd = LISY_G_SW_SETTING;

  //send cmd
  cmd_data[0] = cmd;
  //number of setting group
  cmd_data[1] = 1;
  //send switch number
  cmd_data[2] = number;

      //send cmd
     if ( lisy_api_write( cmd_data,3,ls80dbg.bitv.switches) != 3)
      {
        printf("Error get switch status writing to serial\n");
        return -1;
      }

 //receive answer
  if ( ( ret = read(fd_api,&status,1)) != 1)
    {
        printf("Error reading from serial dip switch value, return:%d %s\n",ret,strerror(errno));
        return -1;
    }

 return status;

}

//Rmake sure connected hardware is of type 'idstr'
//will also sync in case there are still bytes in the send/recv queue
int lisy_api_check_con_hw( char *idstr )
{
  char nextbyte;
  int i,n,ret;
  unsigned char cmd;
  char content[10];

  cmd = LISY_G_HW;

 if ( ls80dbg.bitv.basic )
  {
    sprintf(debugbuf,"check if connected hardware is\"%s\"",idstr);
    lisy80_debug(debugbuf);
  }

 //send command
 if ( lisy_api_write( &cmd,1,ls80dbg.bitv.basic) != 1)
    {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
    }

 //receive answer
 i=0;
 do {
  if ( ( ret = read(fd_api,&nextbyte,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }
  content[i] = nextbyte;
  if ( ls80dbg.byte >= 63 ) //only with full debug
  {
    sprintf(debugbuf,"API_read_string: Byte no %d is (0x%02x)\"%c\"",i,nextbyte,nextbyte);
    lisy80_debug(debugbuf);
  }

  //check if idstr mactch content
  if ( idstr[i] == content[i]) i++;


  } while ( strncmp( idstr,content,strlen(idstr)) != 0); //read until idstr is content
  //will block when not

  //read trailing \0
    if ( ( ret = read(fd_api,&nextbyte,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }

  //USB debug?
  if(ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"connected HW is: %s",content);
    lisy80_debug(debugbuf);
  }

 return(0);
}
