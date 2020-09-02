/*
  displays.c
  part of lisy80NG
  bontango 01.2016
  Version 2.01
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <wiringPi.h>
#include "fileio.h"
#include "displays.h"
#include "hw_lib.h"
#include "utils.h"
#include "fadecandy.h"
#include "lisy_home.h"
#include "externals.h"

//globale var, used in most routines
union three {
    unsigned char byte;
    struct {
    unsigned COMMAND_BYTE:7, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;    
    struct {
    unsigned DISPLAY:3, DIGIT:3, FREE:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;   //for 80A 
    struct {
    unsigned ROW:2, POSITION:4, COOKED:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv3;   //for 80B 
    } mydata_display;



/*
 System1
 set one digit on display, dat is int in this case
 display 0 is status display
 others are 1..4 for player 1..4
 with debugging and 7digit support
*/
void display1_show_int( int display, int digit, int dat)
{
int i;
char datchar;
static unsigned char first_time = 1;
//we store what is on the displays
static char player[4][8] = { "       ", "       ","       ","       "};
static char status[5] = "    ";

   //convert dat(int) to datchar for PIC
   switch (dat) {
      case 0: datchar = '0'; break;
      case 1: datchar = '1'; break;
      case 2: datchar = '2'; break;
      case 3: datchar = '3'; break;
      case 4: datchar = '4'; break;
      case 5: datchar = '5'; break;
      case 6: datchar = '6'; break;
      case 7: datchar = '7'; break;
      case 8: datchar = '8'; break;
      case 9: datchar = '9'; break;
      default : datchar = ' '; break;
	}

  //with 6digit support just store value for possible debugging
  if (!ls80opt.bitv.sevendigit )
  {
    if ( display == 0) 
     {
	if(digit<=4) status[digit] = datchar;
     }
   else
     {
       if (digit <= 7) player[display-1][digit] = datchar;
     }
  }


  //with 7digit support we need to modify things
  if (ls80opt.bitv.sevendigit )
  {
  //first time sys1 sets something to the displays we need to blank
  //all digit five ( first digit at diplay) as this is not
  //under control of sys1 program anymore
  if(first_time)
   {
	/* build control byte */
	mydata_display.bitv2.DIGIT = 5;  //#digit is five ( outer left)
        mydata_display.bitv2.IS_CMD = 0;
      //do it for all 4 displays
      for( i=1; i<=4; i++)
       {
	mydata_display.bitv2.DISPLAY = i;
	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( ' ' );
       }
    //only once
    first_time = 0;
   }

    //digit moves one to the right, which dec by 1 for system1
    //original digit order is from left to right: 5,4,3,2,1,0
    //new digit order is from left to right:    5,4,3,2,1,0,6
    //so digit 5 is under LISY1 control

    //do also storing value in string for easy compare
    //OLD:          5,4,3,2,1,0,6
    //order here is 0,1,2,3,4,5,6
    //so player[display-1][0] is the one under lISY1 control
    if ( display != 0) //not for status display
     {
      switch (digit) {
        case 0:
		 digit = 6;
	         player[display-1][6] = datchar;
		 break;
        case 1:
		 --digit;
	         player[display-1][5] = datchar;
		 break;
        case 2:
		 --digit;
	         player[display-1][4] = datchar;
		 break;
        case 3:
		 --digit;
	         player[display-1][3] = datchar;
		 break;
        case 4:
		 --digit;
	         player[display-1][2] = datchar;
		 break;
        case 5:
		 --digit;
	         player[display-1][1] = datchar;
		 break;
       }
     }


    //and add a zero or a blank if needed to outmost left digit player[x][0]
    //do we need a blank?
    if ( ( strcmp( &player[display-1][1],"      ") == 0) & ( player[display-1][0] != ' '))
      {

        if ( ls80dbg.bitv.displays ) lisy80_debug("7digit: add a space outmost left");

	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = 5; //outmost left is index 5
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( ' ' );

	//store new value 
	player[display-1][0] = ' ';
      }
    //do we need a zero?
    if ( ( strcmp( &player[display-1][1],"000000") == 0) & ( player[display-1][0] != '0'))
      {
        if ( ls80dbg.bitv.displays ) lisy80_debug("7digit: add a ZERO outmost left");

	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = 5; //outmost left is index 5
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( '0' );

	//store new value 
	player[display-1][0] = '0';
      }

    //we do want a 'double zero' blinking for first ball
    //do we need a blank?
    if ( ( player[display-1][6] == ' ') & ( player[display-1][5] != ' '))
      {
        if ( ls80dbg.bitv.displays ) lisy80_debug("7digit: add a space secind digit");

	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = 0; //second digit is index 0
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( ' ' );

	//store new value 
	player[display-1][5] = ' ';
      }
    //do we need a zero?
    if ( ( player[display-1][6] == '0') & ( player[display-1][5] != '0'))
      {
        if ( ls80dbg.bitv.displays ) lisy80_debug("7digit: add a ZERO secind digit");

	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = 0; //second digit is index 0
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( '0' );

	//store new value 
	player[display-1][5] = '0';
      }
  }//for sevendigit option


        /* send it to the display */
	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = digit;
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( datchar );


   //debug?
   if ( ls80dbg.bitv.displays )
    {
     printf(" digit  :0123456\n");
     printf("Player 1:%s\n",player[0]);
     printf("Player 2:%s\n",player[1]);
     printf("Player 3:%s\n",player[2]);
     printf("Player 4:%s\n",player[3]);
     printf("credits:%c%c\n",status[3],status[2]);
     printf("Ball in play:%c%c\n",status[1],status[0]);
     }
}


/*
 80&80A only, need to be extended to 80B
 set one digit on display, dat is int in this case
 display 0 is status display
 others are 1..6, with 1..4 for player 1..4
 player5 as bonus display
 player6 as timer display
*/
void display_show_int( int display, int digit, int dat)
{
char datchar;
static char player[5][9] = { "        ", "        ","        ","        ","        "};


switch (dat) {
      case 0: datchar = '0'; break;
      case 1: datchar = '1'; break;
      case 2: datchar = '2'; break;
      case 3: datchar = '3'; break;
      case 4: datchar = '4'; break;
      case 5: datchar = '5'; break;
      case 6: datchar = '6'; break;
      case 7: datchar = '7'; break;
      case 8: datchar = '8'; break;
      case 9: datchar = '9'; break;
      default : datchar = ' '; break;
	}
 if ( lisy_hardware_revision == 311) toggle_dp_led(); //show activity
 display_show_char( display, digit, datchar);

  //with just store value for possible debugging
  if (!ls80opt.bitv.sevendigit )
  {
    //if ( display != 0) //not for status display at the moment //RTH
      player[display][digit] = datchar;

   //debug?
   if ( ls80dbg.bitv.displays )
    {
     printf(" digit  :0123456\n");
     printf("Player 1:%s\n",player[1]);
     printf("Player 2:%s\n",player[2]);
     printf("Player 3:%s\n",player[3]);
     printf("Player 4:%s\n",player[4]);
     printf("status:%s\n",player[0]);
    }
  }

}



/*
 set one digit on display
 display 0 is status display
 others are 1..6, with 1..4 for player 1..4
 player5 as bonus display
 player6 as timer display
 .
 For 80B we place it on 80B display, display 5 & 6 to be ignored
*/
void display_show_char( int display, int digit, char data)
{

 //int position;
 //int row;
 //int ignore=0;

if (lisy80_game.is80B)
{
 /* ignore 80B for now */
}
else
{
	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = digit;
        mydata_display.bitv2.IS_CMD = 0;

	//write to PIC
	lisy80_write_byte_disp_pic( mydata_display.byte );
	lisy80_write_byte_disp_pic( data );

}
}


/*
 For Bally/LISY35 only 0..9 and space BCD decoded
 will be filtered by display PIC
 set one digit on display, dat is int in this case
 display 0 is status display
 others are 1..6, with 1..4 for player 1..4
*/
void display35_show_int( int display, int digit, unsigned char dat)
{

 char buf[2];

	/* build control byte */
	mydata_display.bitv2.DISPLAY = display;
	mydata_display.bitv2.DIGIT = digit;
        mydata_display.bitv2.IS_CMD = 0;

  buf[0] = mydata_display.byte;
  buf[1] = dat;

  //write data to PIC
  lisy80_write_multibyte_disp_pic( buf, 2 );

}


/*
  display string on display
  0==status display
  1..4 == player 1..4
  player5 as bonus display
  player6 as timer display
*/
void display_show_str( int display, char *data)
{

	int i;
 if ( display == 0) // status display?
 {
        for(i=3; i >= 0; i--)
                {
                 display_show_char( display, i, data[3-i]);
                }

 }
 else
 {
	//TODO: we need to look for real length in the future
        for(i=5; i >= 0; i--)
                {
                 display_show_char( display, i, data[5-i]);
                }
	    //in SYS8A mode digit 6 is for data[6]
            display_show_char( display, 6, data[6]);
  }
}

/*
  display string on display Bally/LISY35 version
  0==status display
  1..4 == player 1..4
  player5, player 6 special displays
  uses display35_show_int, so no characters possible
  
  we decide with the length of the string if we have a 6 or a 7digit display
*/
void display35_show_str( int display, char *data)
{

	int i,len;

	len = strlen(data);

        for(i=0; i<len; i++)
                {
		 // -48 to get an int out of the ascii code ( 0..9)
		 //for 6-digit input we start at index 2
		 //for 7digit at index 1
		 if (len <= 6) display35_show_int( display, i+2, data[i] - 48);
		  else display35_show_int( display, i+1, data[i] - 48);
                }
}


/*
  send command to display PIC, no answer expected
  80A mode
*/

void display_cmd2pic_noansw(unsigned char command)
{

        /* build control byte */
        mydata_display.bitv.COMMAND_BYTE = command;
        mydata_display.bitv.IS_CMD = 1;        //we are sending a command here

        //write to PIC
        lisy80_write_byte_disp_pic( mydata_display.byte );

	return;
}

/*
  send command to display PIC and get answer
*/

int display_cmd2pic(unsigned char command)
{

        /* build control byte */
        mydata_display.bitv.COMMAND_BYTE = command;
        mydata_display.bitv.IS_CMD = 1;        //we are sending a command here


        //write to PIC
        lisy80_write_byte_disp_pic( mydata_display.byte );

	//wait a bit, PIC migth be slow
	delay(300); //wait 100ms secs

	//read answer and send back
	return( lisy80_read_byte_disp_pic());
}


/*
  send a command  plus byte to display PIC, no answer expected
  80B mode
*/

void display_cmd2pic_80b_noansw(unsigned char rows, unsigned char value)
{

        /* build control byte */
        mydata_display.bitv3.ROW = rows;
        mydata_display.bitv3.COOKED = 0;
        mydata_display.bitv3.IS_CMD = 0;

        //write to PIC
        lisy80_write_byte_disp_pic( mydata_display.byte );
        lisy80_write_byte_disp_pic( value );

	return;
}

/*
  send a command  plus multiple byte to display PIC
  80B mode
*/

void display_multibyte_to_pic_80b_noansw(unsigned char rows, char *buf, int no)
{
        /* build control byte */
        mydata_display.bitv.IS_CMD = 1;        //we are sending a command here

	switch(rows)
	{
      	  case 0:  //both rows
		mydata_display.bitv.COMMAND_BYTE = LS80DPCMD_NEXT20_ROW1; 
        	//write command to PIC
        	lisy80_write_byte_disp_pic( mydata_display.byte );
        	//write data to PIC
        	lisy80_write_multibyte_disp_pic( buf, no );
		//both rows
		mydata_display.bitv.COMMAND_BYTE = LS80DPCMD_NEXT20_ROW2; 
        	//write command to PIC
        	lisy80_write_byte_disp_pic( mydata_display.byte );
        	//write data to PIC
        	lisy80_write_multibyte_disp_pic( buf, no );
		break;
      	  case 1: 
		mydata_display.bitv.COMMAND_BYTE = LS80DPCMD_NEXT20_ROW1; 
        	//write command to PIC
        	lisy80_write_byte_disp_pic( mydata_display.byte );
        	//write data to PIC
        	lisy80_write_multibyte_disp_pic( buf, no );
		break;
      	  case 2: 
		mydata_display.bitv.COMMAND_BYTE = LS80DPCMD_NEXT20_ROW2; 
        	//write command to PIC
        	lisy80_write_byte_disp_pic( mydata_display.byte );
        	//write data to PIC
        	lisy80_write_multibyte_disp_pic( buf, no );
		break;
	}
  return;
}


//get the sw version of the pic
int display_get_sw_version(void)
{
 int ver_main, ver_sub;

 ver_main = display_cmd2pic(LS80DPCMD_GET_SW_VERSION_MAIN);
 ver_sub = display_cmd2pic(LS80DPCMD_GET_SW_VERSION_SUB);

 return( ver_main * 100 + ver_sub);
}


//get the hw revision stored in the PIC ( PIC SW >= 4.x needed)
int display_get_hw_revision(void)
{
 int hw_rev;

 hw_rev = display_cmd2pic(LS80DPCMD_GET_HW_REV);

 return( hw_rev );
}

//get the DIP Switch value read by the PIC
int display_get_dipsw_value()
{
return( display_cmd2pic(LS80DPCMD_GET_DIPSW_VALUE));
}

//let display pic do a reset; needed if we want to switch between 80 & 80B
void display_init()
{
display_cmd2pic_noansw(LS80DPCMD_INIT);
return;
}




//let display pic do a reset of the display chip
void display_reset()
{
display_cmd2pic_noansw(LS80DPCMD_RESET);
return;
}


//send one byte to display, to rigth row
void display_send_byte_torow( unsigned char data, int LD1_was_set, int LD2_was_set )
{

 if ( LD1_was_set & LD2_was_set )
    {
	//send byte to row1 & row2 of Display
	display_cmd2pic_80b_noansw(LS80D_B_TOROW12, data);
    }
   else
    {
     if ( LD1_was_set )
      {
	//send byte to row1 of Display
	display_cmd2pic_80b_noansw(LS80D_B_TOROW1, data);
      }
     if ( LD2_was_set )
      {
	//send byte to row2 of Display
	display_cmd2pic_80b_noansw(LS80D_B_TOROW2, data);
      }
    }
}

//send one row (20 chars) to display, to rigth row
void display_send_row_torow( char *buf, int LD1_was_set, int LD2_was_set )
{

 if ( LD1_was_set & LD2_was_set )
    {
	//send byte to row1 & row2 of Display
	display_multibyte_to_pic_80b_noansw(LS80D_B_TOROW12, buf, 20);
	//for (i=0; i<20; i++) display_cmd2pic_80b_noansw(LS80D_B_TOROW12, *(buf + i) );
    }
   else
    {
     if ( LD1_was_set )
      {
	//send byte to row1 of Display
	display_multibyte_to_pic_80b_noansw(LS80D_B_TOROW1, buf, 20);
	//for (i=0; i<20; i++) display_cmd2pic_80b_noansw(LS80D_B_TOROW1, *(buf + i) );
      }
     if ( LD2_was_set )
      {
	//send byte to row2 of Display
	display_multibyte_to_pic_80b_noansw(LS80D_B_TOROW2, buf, 20);
	//for (i=0; i<20; i++) display_cmd2pic_80b_noansw(LS80D_B_TOROW2, *(buf + i) );
      }
    }
}


//send to display, buffered version, called from lisy80.c
//we send the row only when something has changed
//control bytes will be send immidiatly
void display_sendtorow( unsigned char data, int LD1_was_set, int LD2_was_set, int debug )
{

//int i;
static int last_data_was_ctrl=0;
static char row1[21];
static char row2[21];
static char oldrow1[21] = "";
static char oldrow2[21] = "";
static int row1_counter=0;
static int row2_counter=0;

 //show activity
 if ( lisy_hardware_revision == 311) toggle_dp_led(); //show activity

//is this a control byte? (=1)
//if yes, send it and set flag
if ( data == 1 ) 
  { 
    last_data_was_ctrl = 1;
    display_send_byte_torow( data, LD1_was_set, LD2_was_set );
    return;
  }

//indicate if it was for LD1,LD2 or both


if ( last_data_was_ctrl == 1)
 {
  //reset ctrl flag
  last_data_was_ctrl = 0;

  switch (data)
  {
	case 5: if (debug) lisy80_debug("Set digit time to 16 cycles per grid\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 6: if (debug) lisy80_debug("Set digit time to 32 cycles per grid\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 7: if (debug) lisy80_debug("Set digit time to 64 cycles per grid\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 8: if (debug) lisy80_debug("Enable Normal Display Mode (MSB in data words is cursor control only\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 9: if (debug) lisy80_debug("Enable Blank Mode (data words with MSB = 1 will be blanked and cursor will be on\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 0x0a: if (debug) lisy80_debug("Enable inverse Mode (data words with MSB = 1 will be inversed and cursor will be on\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 0x0e:  if (debug) lisy80_debug("start display refresh cycle\n\r");
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 0x40 ... 0x7f: if (debug)  { sprintf(debugbuf,"load duty cycle:%d\n\r",data&0x3f); lisy80_debug(debugbuf); } //lower six bits
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 0x80 ... 0x9f: if (debug) { sprintf(debugbuf,"load digit counter:%d\n\r",data-0x80); lisy80_debug(debugbuf); }
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		break;
	case 0xc0 ... 0xdf: //if (debug) fprintf(stderr,"load buffer pointer reg with:%d\n\r",data&0x1f); //lower five bits
		//dont forget to send byte for load buffer pointer as well
    		display_send_byte_torow( data, LD1_was_set, LD2_was_set );
		if (LD1_was_set)
			{
			   //if (debug) fprintf(stderr,"row1_counter:%d ",row1_counter);
			   row1_counter = data&0x1f;
			   //if (debug) fprintf(stderr,"now:%d\n\r",row1_counter);
			}
		if (LD2_was_set)
			{
			   //if (debug) fprintf(stderr,"row2_counter:%d ",row2_counter);
			   row2_counter = data&0x1f;
			   //if (debug) fprintf(stderr,"now:%d\n\r",row2_counter);
			}
		break;
	default: if (debug) { sprintf(debugbuf,"unknown command after ctrl:%d \n\r",data); lisy80_debug(debugbuf); }
  } //switch
 }
else
 {
  if (LD1_was_set)
	{
	 //print out when row is full -> rowcounter = 20
	 row1[row1_counter++] = data;
	 //only send data to the display in case we have to tell something new
	 if ( row1_counter == 20 )
		{
		  if ( memcmp( row1, oldrow1, 20*sizeof(char) ) != 0)
			{
			 display_send_row_torow( row1, 1, 0); //10 = first row
			 memcpy(oldrow1,row1,20*sizeof(char));
		  	 //if (debug) fprintf(stderr,"[1][01234567890123456789]\n\r");
			 if (debug) {  sprintf(debugbuf,"[row 1 changed][%s]\n\r",row1);
					 lisy80_debug(debugbuf);
				        //int ii;
					//for( ii=0; ii<=19; ii++) printf("%c ( %d )\n",row1[ii],row1[ii]);
					 }
			}
		}
	 //error?
	 if ( row1_counter >20 ) { fprintf(stderr,"ERROR row1 counter overflow\n\r"); row1_counter = 0; }
	}
  if (LD2_was_set)
	{
	 row2[row2_counter++] = data;
	 //only send data to the display in case we have to tell something new
	 if ( row2_counter == 20 )
		{
		  if ( memcmp( row2, oldrow2, 20*sizeof(char) ) != 0)
			{
			 display_send_row_torow( row2, 0, 1); //01=second row
			 memcpy(oldrow2,row2,20*sizeof(char));
			 //if (debug) fprintf(stderr,"[2][01234567890123456789]\n\r");
	 		 if (debug) { 
					sprintf(debugbuf,"[row 2 changed][%s]\n\r",row2);
					lisy80_debug(debugbuf); }
				        //int ii;
					//for( ii=0; ii<=19; ii++) printf("%c ( %d )\n",row2[ii],row2[ii]);
			}
		}
	 //error?
	 if ( row2_counter >20 ) { fprintf(stderr,"ERROR row2 counter overflow\n\r"); row2_counter = 0; }
	}
 }

}

//this is the error message
void display_show_error_message(int errornumber, char *msg80, char *msg80B )
{

	char buf[22];

if (lisy80_game.is80B)
 {
        //line 1 
        sprintf(buf,"%02d %-17s",errornumber,msg80B);
        display_send_row_torow( buf, 1, 0 );
 }
 else
 {
        //show error message on on display one
        display_show_str( 1, msg80);
        //show error number on display two
        sprintf(buf,"ERR %02d",errornumber);
        display_show_str( 2, buf);
 }



}

//show sw version of pic on status display (LISY35 only at the moment)
void display_show_pic_sw_version(int pic_no, int sw_version)
{

 char str_sw_version[10];

 if ( lisy_hardware_revision == 350 )
 {
  //not on digit 1 & 4, because they are covered by backglas
  sprintf(str_sw_version," %d%d %02d",pic_no,sw_version/100,sw_version%100);
  display35_show_str( 0, str_sw_version);
 }
}


//this is the boot message for lis35y
void display_show_boot_message_lisy35(char *s_lisy35_software_version)
{

  //show 'gamename' on display one
  //sprintf(buf,"%-6s",lisy1_gamename);
  //display_show_str( 1, buf);
  //show Gottlieb internal ROM ID  on display two
  //sprintf(buf,"GTB %-2s",rom_id);
  //display_show_str( 2, buf);

  //show 'boot' (6001) on display three
  display35_show_str( 3, "6001  ");

  //show Version number on Display 4
  display35_show_str( 4, s_lisy35_software_version);

}

//this is the boot message for lisy1
void display_show_boot_message_lisy1(char *s_lisy1_software_version,char *rom_id,char *lisy1_gamename)
{
	//int i;
	char buf[22];

  //show 'gamename' on display one
  sprintf(buf,"%-6s",lisy1_gamename);
  display_show_str( 1, buf);
  //show Gottlieb internal ROM ID  on display two
  sprintf(buf,"GTB %-2s",rom_id);
  display_show_str( 2, buf);
  //show 'boot' on display three
  display_show_str( 3, "BOOT   ");
  //show Version number on Display 4
  display_show_str( 4, s_lisy1_software_version);
  //status display countdown
  /* Countdown not used as System1 is slow booting anyway
  for (i=5; i>=0; i--)
   {
     display_show_int(0,3,i);
     sleep(1);
    }
  */
  //blank status display
  display_show_str( 0, "    ");
}

//this is the boot message for lisy80
void display_show_boot_message(char *s_lisy80_software_version,int lisy80_gtb_no,char *lisy80_gamename)
{
	int i;
	char buf[22];

 if (lisy80_game.is80B)
 {
	//line 2
	sprintf(buf,"BOOT LISY80 V %6s",s_lisy80_software_version);
	display_send_row_torow( buf, 0, 1 );
	//line 1 make sure it is all uppercase
	for (i=0; i<8; i++) lisy80_gamename[i] = toupper(lisy80_gamename[i]);
	//line 1 countdown
	for (i=5; i>=0; i--)
	 { 
		sprintf(buf,"%-8s GTB%03d   %02d",lisy80_gamename,lisy80_gtb_no,i);
		display_send_row_torow( buf, 1, 0 );
		sleep(1);
	  }
 }
 else
 {
	//show 'gamename' on display one
	sprintf(buf,"%-6s",lisy80_gamename);
	display_show_str( 1, buf);
	//show Gottlieb internal Gamenr on displaytwo 
	sprintf(buf,"GTB%3d ",lisy80_gtb_no);
	display_show_str( 2, buf);
	//show 'boot' on display three
	display_show_str( 3, "BOOT   ");
	//show Version number on Display 4
	display_show_str( 4, s_lisy80_software_version);
	//status display countdown
	for (i=5; i>=0; i--)
	 {
	   display_show_int(0,3,i);
	   sleep(1);
	  }
	//blank status display
	display_show_str( 0, "    ");
 }

}

//this is the shutdown message
void display_show_shut_message(void)
{

if (lisy80_game.is80B)
 {
	char buf[22];
	sprintf(buf,"SHUTDOWN INITIATED ");
	//shutdown message on row 2
	display_send_row_torow( buf, 0, 1 );
 }
else
 {
  display_show_str( 3, "SHUT   ");
 }
}

//control the soundE line which is
//used also for display 7. digit in some cases
void lisy35_display_set_soundE( unsigned char value)
{
 if (value)  display_cmd2pic_noansw(LS35_SOUNDE_1);
  else display_cmd2pic_noansw(LS35_SOUNDE_0);
}

//set the display variant
/* different variants
 * 0 default: 4 player display plus 1 status (6-digit)
 * 1: 4 player display plus 1 status (7-digit)
 * 2 status display via J4-PIN5 ( PIA1-PB4) Medusa
 *  extra output: RD2 -> J4PIN5
 * 3 status display via J4-PIN8 ( PIA1-PB7) Elektra,Vector,Pac Man
 * *  extra output: RD3 -> J4PIN8
 * 4 two extra displays J4-PIN8 ( PIA1-PB7) & J1-PIN8 (Lamp strobe 2) Six million dollar man
 * *  extra outputs: RD3 -> J4PIN8 & RD6 -> J1PIN8
 */

void lisy35_display_set_variant( unsigned char variant)
{

 if ( ls80dbg.bitv.basic)
     {
       sprintf(debugbuf,"display variant set to:%d",variant);
       lisy80_debug(debugbuf);
     }
 switch (variant) {
        case 0: display_cmd2pic_noansw(LS35DPCMD_VARIANT_0); break;
        case 1: display_cmd2pic_noansw(LS35DPCMD_VARIANT_1); break;
        case 2: display_cmd2pic_noansw(LS35DPCMD_VARIANT_2); break;
        case 3: display_cmd2pic_noansw(LS35DPCMD_VARIANT_3); break;
        case 4: display_cmd2pic_noansw(LS35DPCMD_VARIANT_4); break;
	}
}

