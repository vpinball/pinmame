 /*
  RTH 11.2016
  lisy80NG first tests
  provide menu for testing
  version 0.03
*/

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <wiringPi.h>
#include "../fileio.h"
#include "../hw_lib.h"
#include "../coils.h"
#include "../displays.h"
#include "../switches.h"
#include "../utils.h"
#include "../eeprom.h"

//global var for additional options
//typedef is defined in hw_lib.h
ls80opt_t ls80opt;
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;
int lisy80_is80B;
//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0,0 };
int lisy80_time_to_quit_flag; //not used here
//global var for internal game_name structure, set by  lisy80_file_get_gamename in main
t_stru_lisy80_games_csv lisy80_game;
//global vars for timing & speed
struct timeval lisy80_start_t;
long no_of_tickles = 0;
long no_of_throttles = 0;
int g_lisy80_throttle_val = 1000;
//global var for sound options
t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
int lisy80_volume = 80; //SDL range from 0..128



#define VERSION "1.31"
	int position;
	int gtb_sys80_type = 0;  //0==SYS80, 1=SYS80A, 2==SYS80B


/*
  send a byte to a specific row on 80B display
*/
void send_to_80b( char *str)
{
	int row,data;

       strtok( str, ",");
       row = atoi( strtok( NULL, ","));
       data = atoi( strtok( NULL, ","));

	printf("sending row:%d data:%d",row,data);

	switch(row)
	{
	 case 1:  display_sendtorow( 1, 0, data, 0);
		   break;
	 case 2:  display_sendtorow( 0, 1, data, 0);
		   break;
	 case 3:  display_sendtorow( 1, 1, data, 0);
		   break;

	}

}


/*
  display string on display
*/
void set_display_str( char *str)
{

        int i;
        int display;
        char *data;

       strtok( str, ",");
       display = atoi( strtok( NULL, ","));
       data = strtok( NULL, ",");

if ( display == 0) // status display?
{
        for(i=3; i >= 0; i--)
                {
                 display_show_char( display, i, data[3-i]);
                }
}
else if ( display <= 4)
{
        for(i=5; i>=0; i--) display_show_char( display, i, data[5-i]);
       //for SYS8A mode digit 6 is for data[6]
        //we dont care and set it for SYS80 also
        display_show_char( display, 6, data[6]);
}
else // displays 5 and 6 have 6 digits
{
       //for SYS8A mode digit 6 is for data[6]
        for(i=5; i>=0; i--) display_show_char( display, i, data[5-i]);
}
}




void lisy_help( int position)
{
int pos;

pos = position / 10;

switch (pos)
	{
	 case 0:   printf("HELP, possible commands:\n");
		   printf(" Display\n");
		   printf(" Coil\n");
		   printf(" Switch\n");
		   printf(" sOund\n");
		   printf(" eXit\n");
		   printf(" Help\n");
		   break;
	 //DISPLAY
	 case 1:   printf("HELP, possible commands:\n");
		   printf(" s Show,<#Display>,<nnnnnn>   SYS80/SYS80A mode\n");
		   printf(" e sEnd,<byte>   SYS80B mode\n");
		   printf(" v show version used at display pic\n");
		   printf(" d show dip switch value connected to display pic\n");
		   printf(" r do a reset of the System80B display\n");
		   printf(" b row,value send value to row on System80B display\n");
		   printf(" n get and show next byte from input buffer\n");
		   printf(" Up\n");
		   printf(" Help\n");
		   break;
	 //COILS
	 case 2:   printf("HELP, possible commands:\n");
		   printf(" o On,<#Coil>\n");
		   printf(" f oFf,<#Coil>\n");
		   printf(" p Pulse,<#Coil>\n");
		   printf(" t Test all coils\n");
		   printf(" s pulse Solenoid,<#Solenoid>\n");
		   printf(" v show version used at coils pic\n");
		   printf(" Up\n");
		   printf(" Help\n");
		   break;
	 //SWITCH
	 case 3:   printf("HELP, possible commands:\n");
		   printf(" m Monitor switch stati\n");
		   printf(" v show version used at switch pic\n");
		   printf(" n get and show next byte from input buffer\n");
		   printf(" s show all closed switches\n");
		   printf(" Up\n");
		   printf(" Help\n");
		   break;
	 case 4:   printf("HELP, possible commands:\n");
		   printf(" p Play sound, <#No(1..15)>\n");
		   printf(" Up\n");
		   printf(" Help\n");
		   break;
	}
}


/* 
 ****** DISPLAY *****
*/

void read_next_disppic_byte(void)
{
printf("Display PIC next byte in buffer:%d\n\r",lisy80_read_byte_disp_pic());
}


void show_disppic_version(void)
{
int version;
version = display_get_sw_version( );
   printf("Software Display PIC has version %d.%d\n\r",version/100, version%100);

}

void show_dipsw_value(void)
{
int dipsw;
dipsw = display_get_dipsw_value( );
printf("Display PIC resturns %d for DIP switch setting\n\r",dipsw);
}


void m_display(void)
{
	char input[80];
	char prompt[80];
	int position = 10;

strcpy(prompt, "LISY80/Display>");

while ( 1 )
           {
                printf("%s", prompt);
		fgets(input, sizeof(input), stdin);
                //gets ( input );
                switch ( tolower ( input[0] ))
                        {
                         case 's': set_display_str(input);
				   break;
                         case 'u': return;
				   break;
                         case 'v': show_disppic_version();
				   break;
                         case 'n': read_next_disppic_byte();
				   break;
                         case 'r': display_reset();
				   break;
                         case 'b': send_to_80b(input);
				   break;
                         case 'd': show_dipsw_value();
				   break;
                         case 'h':
			 case '?': lisy_help(position);
                                   break;
                        }
                }
}

/* 
 ****** COIL *****
*/

void show_coilpic_version(void)
{
int version;
version = coil_get_sw_version( );
   printf("Software Coil PIC has version %d.%d\n\r",version/100, version%100);

}

void coil_pulse_solenoid ( char *input)
{
        int coil,solenoid;

         strtok( input, ",");
         solenoid = atoi( strtok( NULL, ","));

  switch(solenoid)
   {
	case 1: coil=Q_SOL1;
		break;
	case 2: coil=Q_SOL2;
		break;
	case 3: coil=Q_SOL3;
		break;
	case 4: coil=Q_SOL4;
		break;
	case 5: coil=Q_SOL5;
		break;
	case 6: coil=Q_SOL6;
		break;
	case 7: coil=Q_SOL7;
		break;
	case 8: coil=Q_SOL8;
		break;
	case 9: coil=Q_SOL9;
		break;
	default: return;
	         break;
   }
  printf("Pulse Solenoid %d which is Coil %d\n\r",solenoid,coil);
         coil_set(coil,1);
	 usleep(COIL_PULSE_TIME);
         coil_set(coil,0);

}


//coil Menue
void m_coil()
{
	char input[80];
	char prompt[80];
	int position = 20;

 strcpy(prompt, "LISY80/Coil>");

while ( 1 )
           {
                printf("%s", prompt);
		fgets(input, sizeof(input), stdin);
                //gets ( input );
                switch ( tolower ( input[0] ))
                        {
                         case 'u': return;
			           break;
                         case 'o': coil_set_str(input,1);
				   break;
                         case 'v': show_coilpic_version();
				   break;
                         case 'f': coil_set_str(input,0);
				   break;
                         case 'p': coil_set_str(input,2);
				   break;
                         case 's': coil_pulse_solenoid(input);
				   break;
                         case 't': coil_test();
				   break;
                         case 'h':
			 case '?': lisy_help(position);
                                   break;
                        }
                }

}

/* 
 ****** SWITCH *****
*/
void read_next_swpic_byte( void )
{
if (lisy80_switch_readycheck() == 0)
	printf("no byte in buffer of Switch PIC\n\r");
else
    printf("Switch PIC next byte in buffer:%d\n\r",lisy80_read_byte_sw_pic());
}



void show_closed_switches(void)
{

 int ret;
 unsigned char action;

 //init the switch PIC
 lisy80_switch_pic_init();
 
 printf("init switch PIC and read all closed switches\n\r");

 //and read all switchactions done afterwards
  while ( (ret=lisy80_switch_reader( &action )) < 80)
    printf("switch:%d (%d)\n\r",ret,action);

 printf("done \n\r");

}


void show_swpic_version(void)
{
int version;
version = lisy80_switch_pic_init();
printf("Switch PIC returns software version:%d.%d\n\r",version/100, version%100);
}



void m_switch()
{
	char input[80];
	char prompt[80];
	int position = 30;

 strcpy(prompt, "LISY80/Switch>");

while ( 1 )
           {
                printf("%s", prompt);
		fgets(input, sizeof(input), stdin);
                //gets ( input );
                switch ( tolower ( input[0] ))
                        {
                         case 'u': return;
                         case 'm': monitor_switches();
				   break;
                         case 'n': read_next_swpic_byte();
				   break;
                         case 's':  show_closed_switches();
				   break;
                         case 'v': show_swpic_version();
				   break;
                         case 'h':
			 case '?': lisy_help(position);
                                   break;
                        }
                }

}

/* 
 ****** SOUND *****
*/


void play_sound_str( char *str)
{

        int sound;

       strtok( str, ",");
       sound = atoi( strtok( NULL, ","));

	lisy80_sound_set(sound);

}


void m_sound()
{
	char input[80];
	char prompt[80];
	int position = 40;

 strcpy(prompt, "LISY80/sOund>");

while ( 1 )
           {
                printf("%s", prompt);
		fgets(input, sizeof(input), stdin);
                //gets ( input );
                switch ( tolower ( input[0] ))
                        {
                         case 'u': return;
			 case 'p': play_sound_str(input);
                                   break;
                         case 'h':
			 case '?': lisy_help(position);
                                   break;
                        }
                }
}


/* 
 ****** MAIN *****
*/

int main( int argc, char **argv )
{

	char input[80];
	char prompt[80];
	int position = 0;


	printf("\n LISY80 Tester Version %s",VERSION);
	printf("\n 'h' or '?' for help\n\n");
	strcpy(prompt, "LISY80>");

//this is 1:1 copy from lisy80.c for init routine
//store start time in global var
gettimeofday(&lisy80_start_t,(struct timezone *)0);

//init th wiringPI library first
lisy80_hwlib_wiringPI_init();

//any options?
//at this stage only look for basic debug option here
//ls80opt.byte = lisy80_get_dip1();
if ( lisy80_dip1_debug_option() )
{
 fprintf(stderr,"LISY80 basic DEBUG activ\n");
 ls80dbg.bitv.basic = 1;
 lisy80_debug("LISY80 DEBUG timer set"); //first message sets print timer to zero
}
else ls80dbg.bitv.basic = 0;

//do init the hardware
//this also sets the debug options by reading jumpers via switch pic
lisy80_hwlib_init();

//now look for the other dips and for extended debug options
lisy80_get_dips();



	//init coils
	coil_init( );


	while ( 1 )
	   {
		printf("%s", prompt);
		fgets(input, sizeof(input), stdin);
		//gets ( input );
		switch ( tolower ( input[0] ))
			{
			 case 'x': break;
			 case 'd': m_display();
				   break;
			 case 'c': m_coil();
				   break;
			 case 's': m_switch();
				   break;
			 case 'o': m_sound();
				   break;
			 case '?': 
			 case 'h': lisy_help(position);
				   break;
			}

		 if ( tolower(input[0]) == 'x')
		 {
		    exit(0);
 		 }

	   }

	exit(1); //never reached
}
