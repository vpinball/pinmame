/*
 socketserver for lisy1 webinterface
 bontango 04.2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include "../fileio.h"
#include "../hw_lib.h"
#include "../coils.h"
#include "../displays.h"
#include "../switches.h"
#include "../eeprom.h"
#include "../utils.h"
#include "../sound.h"

//the version
#define LISY1control_SOFTWARE_MAIN    0
#define LISY1control_SOFTWARE_SUB     2

//dummy inits
void lisy1_init( int lisy80_throttle_val) { }
void lisy80_init( int lisy80_throttle_val) { }
//dummy shutdowns
void lisy1_shutdown( void ) { }
void lisy80_shutdown( void ) { }

//the debug options
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;
int lisy80_is80B;
//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrixLISY1[9] = { 0,0,0,0,0,0,0,0,0 };
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0,0 };
int lisy80_time_to_quit_flag; //not used here
//global var for additional options
//typedef is defined in hw_lib.h
ls80opt_t ls80opt;

//global vars
char switch_description[80][80];
char lamp_description_line1[36][80];
char lamp_description_line2[36][80];
//global var for internal game_name structure, set by  lisy1_file_get_gamename in main
t_stru_lisy1_games_csv lisy1_game;
t_stru_lisy80_games_csv lisy80_game;
//global avr for sound optuions
t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
int lisy80_volume = 80; //SDL range from 0..128
//global var for all lamps
unsigned char lamp[36];
//global var for all sounds
unsigned char sound[32];
//global vars for all displays
char display_D0[8]="";
char display_D1[8],display_D2[8],display_D3[8],display_D4[8];

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//short form of write command
void sendit( int sockfd, char *buffer)
{
   int n;

//   printf("%s",buffer);

   n = write(sockfd,buffer,strlen(buffer));
   if (n < 0) error("ERROR writing to socket");
}



//set sound and update internal vars
void do_sound_set( char *buffer)
{

 int sound_no,i;
 char wav_file_name[80];

 //the format here is 'Sxx' 
 //we trust ASCII values
 sound_no = (10 * (buffer[1]-48)) + buffer[2]-48;

 //set again same sound means set to zero
 if ( sound[sound_no] == 1)
  {
   lisy1_sound_set( 0 );
   //set internal var (remember)
   for(i=0; i<=15; i++) sound[i] = 0;
   sound[0] = 1;
   return;
  }


//do seound to zero first (needed?) 
 lisy1_sound_set( 0 );
 //set internal var (remember)
 for(i=0; i<=15; i++) sound[i] = 0;
 sound[sound_no] = 1;

 //JustBoom Sound? we may want to play wav files here
 if ( ls80opt.bitv.JustBoom_sound )
 {
  //construct the filename, according to game_nr
  sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,sound_no);
  sprintf(debugbuf,"/usr/bin/aplay %s",wav_file_name);
  system(debugbuf);
 }

 //now set sound
 lisy1_sound_set( sound_no );

}


//set the display according to buffer message
void do_display_set( char *buffer)
{
 int display_no;
 char display_str[21];

 //we trust ASCII values
 display_no = buffer[1]-48;

 //the format here is 'Dx_'
 if (buffer[2] == '_') //sys1
 {
  switch( display_no )
  {
   case 0: 
		sprintf(display_str,"%-4s",&buffer[3]);
 		display_show_str( 0, display_str);
	        strcpy(display_D0,display_str);
		break;
   case 1: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 1, display_str);
	        strcpy(display_D1,display_str);
		break;
   case 2: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 2, display_str);
	        strcpy(display_D2,display_str);
		break;
   case 3: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 3, display_str);
	        strcpy(display_D3,display_str);
		break;
   case 4: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 4, display_str);
	        strcpy(display_D4,display_str);
		break;
  }
 }
}


//pulse specific solenoid
void do_solenoid_set( char *buffer)
{
 int solenoid,coil;

 //the format here is 'Cxx' or 'Cxx'
 //we trust ASCII values
 solenoid = (10 * (buffer[1]-48)) + buffer[2]-48;

 switch(solenoid)
   {
        case 1: coil=Q_KNOCK;
                break;
        case 2: coil=Q_TENS;
                break;
        case 3: coil=Q_HUND;
                break;
        case 4: coil=Q_TOUS;
                break;
        case 5: coil=Q_OUTH;
                break;
        case 6: coil=Q_SYS1_SOL6;
                break;
        case 7: coil=Q_SYS1_SOL7;
                break;
        case 8: coil=Q_SYS1_SOL8;
                break;
        default: return;
                 break;
   }
     //now pulse the coil
         lisy1_coil_set(coil,1);
         //usleep(COIL_PULSE_TIME); done in lisy1_coil_set already
         lisy1_coil_set(coil,0);

}


//read all the set dip switch settings
//and store to filesystem
void do_dip_set( char *buffer)
{
 int dip_no,i;
 char dip_setting[10];
 static char part1[24][80];
 static char part2[24][257];
 char line[512];


 //remember number of settings received so far
 static int no_settings = 0;

 //the format here is ' V_DIPxx_ON' or ' V_DIPxx_OFF'
 //or ' K_DIPxx_'comment' 
 //we trust ASCII values
 dip_no = (10 * (buffer[5]-48)) + buffer[6]-48;

 //Note: we do NOT trust the order of the settings
 //as sometimes the browser does mix things up
 //so we collect all settings an write them all at the end
 //which is after receiving 64 entries

 //dip setting?
 if ( buffer[0] == 'V')
  {
   //construct first part of the line
   if ( buffer[9] == 'N') strcpy(dip_setting,"ON"); else strcpy(dip_setting,"OFF");
   sprintf(part1[dip_no-1],"%d;%s;",dip_no,dip_setting);
   no_settings++;
  }
 //no, it is a comment
 else
  {
   //construct second part of the line
   strcpy( part2[dip_no-1],&buffer[8]);
   //construct the line
   no_settings++;
  }

 //once all collected, we write them to the file
 if ( no_settings >=48 )
 {
   //dip number 1 ( 0 as we start with 0 ) means we need to open/create the file: mode = 0
   sprintf(line,"%s%s\n",part1[0],part2[0]);
   if ( lisy1_file_write_dipfile( 0, line ) < 0) syserr( "problems dip setting file", 77, 1);

   //now write settings 2..23 with mode 1
   for ( i=1; i<=22; i++)
    {   
      sprintf(line,"%s%s\n",part1[i],part2[i]);
      if ( lisy1_file_write_dipfile( 1, line ) < 0) syserr( "problems dip setting file", 77, 1);
    }

   //dip number 24 (23)  means we need to close the file after writing, mode = 2
   sprintf(line,"%s%s\n",part1[23],part2[23]);
   if ( lisy1_file_write_dipfile( 2, line ) < 0) syserr( "problems dip setting file", 77, 1);

   //reset number of settings
   no_settings = 0;
 }

}//do_dip_set


// LAMPS

//set lamp and update internal vars
void do_lamp_set( char *buffer)
{

 int lamp_no;
 int action;
 //the format here is 'Lxx_on' or 'Lxx_off'
 //we trust ASCII values
 lamp_no = (10 * (buffer[1]-48)) + buffer[2]-48;
 
 //on or off?
 if ( buffer[5] == 'f') action=0; else  action=1;

 lisy80_coil_set( lamp_no + 1, action);
 lamp[lamp_no] = action;

}

//read the lamp descriptions from the file
#define LISY1_LAMPS_PATH "/boot/lisy1/control/lamp_descriptions/"
#define LISY1_LAMPS_FILE "_lisy1_lamps.csv"
void get_lamp_descriptions(void)
{

   FILE *fstream;
   char lamp_file_name[80];
   char buffer[1024];
   char *line;
   int first_line = 1;
   int lamp_no;




 //construct the filename; using global var lisy80_gamenr
 sprintf(lamp_file_name,"%s%03d%s",LISY1_LAMPS_PATH,lisy1_game.gamenr,LISY1_LAMPS_FILE);

 //try to read the file with game nr
 fstream = fopen(lamp_file_name,"r");
   if(fstream != NULL)
   {
      fprintf(stderr,"LISY80 Info: lamp descriptions according to %s\n\r",lamp_file_name);
   }
   else
   {
    //second try: to read the file with default
    //construct the new filename; using 'default'
    sprintf(lamp_file_name,"%sdefault%s",LISY1_LAMPS_PATH,LISY1_LAMPS_FILE);
    fstream = fopen(lamp_file_name,"r");
      if(fstream != NULL)
      {
      fprintf(stderr,"LISY80 Info: lamp descriptions according to %s\n\r",lamp_file_name);
      }
    }//second try

  //check if first or second try where successfull
  if(fstream == NULL)
      {
        fprintf(stderr,"LISY1 Info: lamp descriptions not found \n\r");
        return ;
        }
  //now assign teh descriptions
   while( ( line=fgets(buffer,sizeof(buffer),fstream)) != NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     lamp_no = atoi(strtok(line, ";"));
     if (lamp_no <52) 
        { 
	  strcpy ( lamp_description_line1[lamp_no], strtok(NULL, ";"));
	  strcpy ( lamp_description_line2[lamp_no], strtok(NULL, ";"));
        }
	else fprintf(stderr,"LISY1 Info: Lamp descriptions wrong info \n\r");
   }

}

//read the switch descriptions from the file
#define LISY1_SWITCHES_PATH "/boot/lisy1/control/switch_descriptions/"
#define LISY1_SWITCHES_FILE "_lisy1_switches.csv"
void get_switch_descriptions(void)
{

   FILE *fstream;
   char switch_file_name[80];
   char buffer[1024];
   char *line;
   int first_line = 1;
   int switch_no;




 //construct the filename; using global var lisy80_gamenr
 sprintf(switch_file_name,"%s%03d%s",LISY1_SWITCHES_PATH,lisy1_game.gamenr,LISY1_SWITCHES_FILE);

 //try to read the file with game nr
 fstream = fopen(switch_file_name,"r");
   if(fstream != NULL)
   {
      fprintf(stderr,"LISY1 Info: switch descriptions according to %s\n\r",switch_file_name);
   }
   else
   {
    //second try: to read the file with default
    //construct the new filename; using 'default'
    sprintf(switch_file_name,"%sdefault%s",LISY1_SWITCHES_PATH,LISY1_SWITCHES_FILE);
    fstream = fopen(switch_file_name,"r");
      if(fstream != NULL)
      {
      fprintf(stderr,"LISY1 Info: switch descriptions according to %s\n\r",switch_file_name);
      }
    }//second try

  //check if first or second try where successfull
  if(fstream == NULL)
      {
        fprintf(stderr,"LISY1 Info: DIP switch descriptions not found \n\r");
        return ;
        }
  //now assign teh descriptions
   while( ( line=fgets(buffer,sizeof(buffer),fstream)) != NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     switch_no = atoi(strtok(line, ";"));
     if (switch_no <80) strcpy ( switch_description[switch_no], strtok(NULL, ";"));
	else fprintf(stderr,"LISY1 Info: Switch descriptions wrong info \n\r");
   }

}


//SOLENOIDS

//send all the infos about the solenoids
void send_solenoid_infos( int sockfd )
{
  int i;
  char colorcode[80],buffer[256];

     //basic info, header line
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy1_game.gamename,lisy1_game.gamenr);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to PULSE specific solenoid<br><br>\n");
     sendit( sockfd, buffer);
     //the color and style
     strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");

  //Knocker (Sol1)
  sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'C01\' %s value=\'Knocker\' /> </form>\n",colorcode);
  sendit( sockfd, buffer);
  //Ouhole (Sol5)
  sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'C05\' %s value=\'Outhole\' /> </form>\n",colorcode);
  sendit( sockfd, buffer);


  //Sol 6,7,8
  for(i=6; i<=8; i++)
    {
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'C%02d\' %s value=\'Solenoid\n%02d\' /> </form>\n",i,colorcode,i);
     sendit( sockfd, buffer);
    }
}


//send all the infos about the sound
void send_sound_infos( int sockfd )
{
  int k;
  char colorcode[80],buffer[256];

     //basic info, header line
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy1_game.gamename,lisy1_game.gamenr);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to send specific sound<br><br>\n");

  for(k=0; k<=3; k++)
    {
     if (sound[k]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px; height: 5em;\'");
                 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'S%02d\' %s value=\'Sound\n%02d\' /> </form>\n",k,colorcode,k);
     sendit( sockfd, buffer);
    }
  sprintf(buffer,"<br>\n"); sendit( sockfd, buffer);

  for(k=4; k<=7; k++)
    {
     if (sound[k]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px; height: 5em;\'");
                 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'S%02d\' %s value=\'Sound\n%02d\' /> </form>\n",k,colorcode,k);
     sendit( sockfd, buffer);
    }
   sprintf(buffer,"<br>\n"); sendit( sockfd, buffer);

}


void send_dipswitch_infos( int sockfd )
{

 int i;
 unsigned char dipvalue; 
 char dip_comment[256];
 char filename[80];
 char buffer[256];

 //basic info, header line
 sprintf(buffer,"Selected game is %s, internal number %d<br>\n",lisy1_game.gamename,lisy1_game.gamenr);
 sendit( sockfd, buffer);
 //dummy read to init read csv routine
 dipvalue = lisy1_file_get_onedip( 1, dip_comment, filename, 1 );
 //and the source where it came from
 sprintf(buffer,"DIP switch settings according to %s<br><br>\n",filename);
 sendit( sockfd, buffer);


 //now send the status
 sprintf(buffer,"<form action='' method='post'>");
 sendit( sockfd, buffer);

 for( i=1; i<=24; i++)
  {
    dipvalue = lisy1_file_get_onedip( i, dip_comment, filename, 0 );
    sprintf(buffer,"<tr>\nSwitch No:%02d -\n",i);
    sendit( sockfd, buffer);
    if ( dipvalue )
     {
       sprintf(buffer,"<td><input id=\"on\" type=\"radio\" name=\"dip%02d_v\" value=\"DIP%02d_ON\" checked></td> ON",i,i);
       sendit( sockfd, buffer);
       sprintf(buffer,"<td><input id=\"off\" type=\"radio\" name=\"dip%02d_v\" value=\"DIP%02d_OFF\"></td> OFF",i,i);
       sendit( sockfd, buffer);
     }
    else
     {
       sprintf(buffer,"<td><input id=\"on\" type=\"radio\" name=\"dip%02d_v\" value=\"DIP%02d_ON\"></td> ON",i,i);
       sendit( sockfd, buffer);
       sprintf(buffer,"<td><input id=\"off\" type=\"radio\" name=\"dip%02d_v\" value=\"DIP%02d_OFF\" checked></td> OFF",i,i);
       sendit( sockfd, buffer);
     }

    sprintf(buffer," --- comment --- ");
    sendit( sockfd, buffer);
    sprintf(buffer,"<input type=\"text\" name=\"dip%02d_k\" size=\"200\" maxlength=\"256\" value=\"%s\" ><br>\n</tr>\n",i,dip_comment);
    sendit( sockfd, buffer);
  }


 //basic info, footer line
 sprintf(buffer,"<br><input type='submit' name='setdip' style='BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;' value='set values'><form>");
 sendit( sockfd, buffer);
 sprintf(buffer," Note: In case of default values this will create a specific definition file for your pin<br>");
 sendit( sockfd, buffer);



} 


void send_lamp_infos( int sockfd )
{
  int i,j,lamp_no;
  char colorcode[80],buffer[256],name[10];

     //basic info, header line
     sprintf(buffer,"Selected game is %s<br><br>\n",lisy1_game.gamename);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to switch lamp OFF or ON  Yellow lamps are ON<br><br>\n");
     sendit( sockfd, buffer);

   //send all the lamps together with the status
   //Name for system1 starts with 1 rather then zero
   for(i=0; i<=2; i++)
   {
    for(j=0; j<=9; j++)
    {
     lamp_no=i*10+j;
     if (lamp[lamp_no]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px; height: 5em;\'");
		 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");
     if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'L%02d\n%15s\n%15s\' /> </form>\n",name,colorcode,lamp_no+1,lamp_description_line1[lamp_no],lamp_description_line2[lamp_no]);
     sendit( sockfd, buffer);
    }
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);
   }


    for(lamp_no=30; lamp_no<=35; lamp_no++)
    {
     if (lamp[lamp_no]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px; height: 5em;\'");
		 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");
     if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'L%02d\n%15s\n%15s\' /> </form>\n",name,colorcode,lamp_no+1,lamp_description_line1[lamp_no],lamp_description_line2[lamp_no]);
     sendit( sockfd, buffer);
/*
     if (lamp[lamp_no]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px\'");
		 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px\'");
     if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no+1); else sprintf(name,"L%02d_on",lamp_no+1);
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'L%02d\n%15s\n%15s\' /> </form>\n",name,colorcode,lamp_no+1,lamp_description_line1[lamp_no],lamp_description_line2[lamp_no]);
     sendit( sockfd, buffer);
*/
    }
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);
}


void send_display_infos( int sockfd )
{
  char buffer[256];

  //start with some header
  sprintf(buffer,"Selected game is %s<br><br>\n",lisy1_game.gamename);
  sendit( sockfd, buffer);

 sprintf(buffer,"<p>  Status Display: <input type=\"text\" name=\"D0\" size=\"4\" maxlength=\"4\" value=\"%4s\" /></p>",display_D0);
 //sprintf(buffer,"<p>  Status Display: <input type=\"text\" name=\"D0\" size=\"4\" maxlength=\"4\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 1: <input type=\"text\" name=\"D1\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D1);
 //sprintf(buffer,"<p>Display Player 1: <input type=\"text\" name=\"D1\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 2: <input type=\"text\" name=\"D2\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D2);
 //sprintf(buffer,"<p>Display Player 2: <input type=\"text\" name=\"D2\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 3: <input type=\"text\" name=\"D3\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D3);
 //sprintf(buffer,"<p>Display Player 3: <input type=\"text\" name=\"D3\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 4: <input type=\"text\" name=\"D4\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D4);
 //sprintf(buffer,"<p>Display Player 4: <input type=\"text\" name=\"D4\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);

}



void send_switch_infos( int sockfd )
{
  int ret,i,j,switch_no;
  unsigned char action;
  char colorcode[40],buffer[256];
  unsigned char strobe,returnval;


  //update internal switch matrix with buffer from switch pic
  //swMatrix[0] is pinmame internal (sound?)
  //swMatrix[1..6] is system1, where 6 is not used?!
  //swMatrix[7] is  'special switches' bit7:SLAM; bit6:Outhole; bit5:Reset
  //Note: SLAM is reverse, which means closed with value 0
 do
    {
     ret = lisy1_switch_reader( &action );

     if (ret < 80) //ret is switchnumber: NOTE: system has has 8*5==40 switches in maximum, counting 01..05;10...15; ...
      {

	//calculate strobe & return
        //Note: this is different from system80
        strobe = ret % 10;
        returnval = ret / 10;

        //set the bit in the Matrix var according to action
        // action 0 means set the bit
        // any other means delete the bit
        if (action == 0) //set bit
          {        SET_BIT(swMatrixLISY1[strobe],returnval); printf("set_bit strobe:%d return:%d\n",strobe,returnval); }
        else  //delete bit
          {        CLEAR_BIT(swMatrixLISY1[strobe],returnval); printf("clear_bit strobe:%d return:%d\n",strobe,returnval); }
	
        }
     }while( ret < 80);

     //now send whole matrix back together with some header
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy1_game.gamename,lisy1_game.gamenr);
     sendit( sockfd, buffer);


     for(i=0; i<=7; i++) //this is the return 
     {
      sprintf(buffer,"<tr style=\"background-color:green;\" border=\"1\">\n");
      sendit( sockfd, buffer);
       for(j=0; j<=4; j++) //this is the strobe 
       { 
	switch_no = i*10 + j;
 	//assign color, red is closed, green is open, default fo table is green
	if ( !CHECK_BIT(swMatrixLISY1[j], i)) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
	//if (lisy80_gtb_socket_switches[switch_no]) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
	sprintf(buffer,"%sSwitch %02d<br>%s</td>\n",colorcode,switch_no,switch_description[switch_no]);
        sendit( sockfd, buffer);
	}
      sprintf(buffer,"</tr>\n");
      sendit( sockfd, buffer);
     }
    //now look for special switches
     sprintf(buffer,"<tr style=\"background-color:green;\" border=\"1\">\n");
     sendit( sockfd, buffer);
     if (!CHECK_BIT(swMatrixLISY1[6], 7)) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
     sprintf(buffer,"%sSLAM<br></td>\n",colorcode);
     sendit( sockfd, buffer);
     if (!CHECK_BIT(swMatrixLISY1[6], 6)) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
     sprintf(buffer,"%sOuthole<br></td>\n",colorcode);
     sendit( sockfd, buffer);
     if (!CHECK_BIT(swMatrixLISY1[6], 5)) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
     sprintf(buffer,"%sRESET<br>onboard</td>\n",colorcode);
     sendit( sockfd, buffer);
     if (!CHECK_BIT(swMatrixLISY1[6], 4)) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
     sprintf(buffer,"%sOPTION<br>not used</td>\n",colorcode);
     sendit( sockfd, buffer);
     strcpy(colorcode,"<td>");
     sprintf(buffer,"%s <br>n/a</td>\n",colorcode);
     sendit( sockfd, buffer);
    sprintf(buffer,"</tr>\n");
    sendit( sockfd, buffer);


}



//send infos for the homepage
void send_home_infos( int sockfd )
{
     char buffer[256];

   sprintf(buffer,"<h2>LISY1 Webeditor Home Page</h2> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"This is LISY1control version %d.%d<br>\n",LISY1control_SOFTWARE_MAIN,LISY1control_SOFTWARE_SUB);
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_switches.php\">Switches</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_lamps.php\">Lamps</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_solenoids.php\">Solenoids</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_displays.php\">Displays</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_dipswitches.php\">DIP Switches</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_sound.php\">Sound</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_nvram.php\">NVRAM Information</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_software.php\">Software installed</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy1_exit.php\">Exit and reboot</a><br><br> \n");
   sendit( sockfd, buffer);
}





//send software version(s)
void send_software_infos( int sockfd )
{
     char buffer[256];
     int dum;

   //get installed version of lisy1
   dum = system("/boot/lisy1/bin/lisy1 -lisy1version");
   sprintf(buffer,"LISY1 version installed is 1.0%02d<br>\n",WEXITSTATUS(dum));
   sendit( sockfd, buffer);

   //init switch pic and get Software version
   dum = lisy80_switch_pic_init();
   sprintf(buffer,"Software Switch PIC has version %d.%d<br>\n",dum/100, dum%100);
   sendit( sockfd, buffer);

   //get Software version Display PIC
   dum = display_get_sw_version();
   sprintf(buffer,"Software Display PIC has version %d.%d<br>\n",dum/100, dum%100);
   sendit( sockfd, buffer);

  //get Software version, Coil PIC
   dum = coil_get_sw_version();
   sprintf(buffer,"Software Coil PIC has version %d.%d<br>\n",dum/100, dum%100);
   sendit( sockfd, buffer);

}

//send info from nvram
void send_nvram_infos( int sockfd )
{
     char buffer[256];
     int i;
     eeprom_block_t myblock;

   //give back the stats from the nvram
   myblock = lisy80_eeprom_getstats();

 sprintf(buffer, "Content of second block of eeprom<br>\n\r");
 sendit( sockfd, buffer);
 sprintf(buffer, "Game number content first block: %d<br>\n\r",myblock.content.gamenr);
 sendit( sockfd, buffer);
 sprintf(buffer, "Number of starts: %d<br>\n\r",myblock.content.starts);
 sendit( sockfd, buffer);
 sprintf(buffer, "Number of starts with debug: %d<br>\n\r",myblock.content.debugs);
 sendit( sockfd, buffer);
 sprintf(buffer, "Number of starts, game specific if >0<br>\n\r");
 sendit( sockfd, buffer);
 for(i=0; i<=63; i++)
   {
     if(myblock.content.counts[i] > 0)
      {
       sprintf(buffer, "Number of starts Game No(%d): %d<br>\n\r",i,myblock.content.counts[i]);
       sendit( sockfd, buffer);
      }
   }
 sprintf(buffer, "Software version last used: %d.%03d<br>\n\r",myblock.content.Software_Main,myblock.content.Software_Sub);
 sendit( sockfd, buffer);

}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     char ip_interface[10];
     struct sockaddr_in serv_addr, cli_addr, *myip;
     int i,n;
     int do_exit = 0;
     struct ifreq ifa;
     char *line;



     //init switch description
     for (i=0; i<80; i++) strcpy ( switch_description[i], "NOT SET");


     //start init lisy80 hardware
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
    lisy1_coil_init( );


    //init internal lamp vars as well
    for(i=0; i<=35; i++) lamp[i] = 0;

    //init sound
    lisy1_sound_set(0);  // zero
    //init internal sound vars as well
    sound[0] = 1;
    for(i=1; i<=7; i++) sound[i] = 0;

    //get the configured game
    lisy1_file_get_gamename( &lisy1_game);


    //read the descriptions for the switches
    get_switch_descriptions();
    //read the descriptions for the lamps
    get_lamp_descriptions();


 // try say something about LISY80 if sound is requested
 if ( ls80opt.bitv.JustBoom_sound )
 {
  //set volume according to poti
  sprintf(debugbuf,"/bin/echo \"Welcome to LISY 1 Control Version 0.%d\" | /usr/bin/festival --tts",LISY1control_SOFTWARE_SUB);
  system(debugbuf);
 }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     //try to find out our IP on Wlan0
     strcpy (ifa.ifr_name, "wlan0");
     strcpy (ip_interface, "WLAN0"); //upercase for message
     if((n=ioctl(sockfd, SIOCGIFADDR, &ifa)) != 0) 
      {
	//no IP on WLAN0, we try eth0 now
        strcpy (ifa.ifr_name, "eth0");
        strcpy (ip_interface, "ETH0"); //upercase for message
        if((n=ioctl(sockfd, SIOCGIFADDR, &ifa)) != 0) 
           strcpy (ifa.ifr_name, "noip");
      }

     if(n) //no IP found
     {
       //construct the message
        fprintf(stderr,"SYS80/A NO IP");
        display_show_str( 1, "NO IP  ");
        display_show_str( 2, "CHECK  ");
        display_show_str( 3, "ETH0   ");
        display_show_str( 4, "WLAN0  ");
     }
     else //we found an IP
     {
      myip = (struct sockaddr_in*)&ifa.ifr_addr;
	//get teh pouinter to teh Ip address
        line = inet_ntoa(myip->sin_addr);
	//split the ip to four displays and store value for display routine
        sprintf(buffer,"%-6s",strtok(line, "."));
        display_show_str( 1, buffer); strcpy(display_D1,buffer);
       fprintf(stderr,"%s\n",buffer);
        sprintf(buffer,"%-6s",strtok(NULL, "."));
        display_show_str( 2, buffer); strcpy(display_D2,buffer);
       fprintf(stderr,"%s\n",buffer);
        sprintf(buffer,"%-6s",strtok(NULL, "."));
        display_show_str( 3, buffer); strcpy(display_D3,buffer);
       fprintf(stderr,"%s\n",buffer);
        sprintf(buffer,"%-6s",strtok(NULL, "."));
        display_show_str( 4, buffer); strcpy(display_D4,buffer);
       fprintf(stderr,"%s\n",buffer);
     }


     //now set up the socketserver
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = 5963;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     //wait and listen
    do {
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");


     //the home screen woth all available commands
     if ( strcmp( buffer, "home") == 0) { send_home_infos(newsockfd); close(newsockfd); }
     //software used, send all the infos to teh webserver
     else if ( strcmp( buffer, "software") == 0) { send_software_infos(newsockfd); close(newsockfd); }
     //nvra stats, send all the infos to teh webserver
     else if ( strcmp( buffer, "nvram") == 0) { send_nvram_infos(newsockfd); close(newsockfd); }
     //overview & control switches, send all the infos to teh webserver
     else if ( strcmp( buffer, "switches") == 0) { send_switch_infos(newsockfd); close(newsockfd); }
     //overview & control lamps, send all the infos to teh webserver
     else if ( strcmp( buffer, "lamps") == 0) { send_lamp_infos(newsockfd); close(newsockfd); }
     //overview and control solenoids, send all the infos to teh webserver
     else if ( strcmp( buffer, "solenoids") == 0) { send_solenoid_infos(newsockfd); close(newsockfd); }
     //overview and cobtrol sounds, send all the infos to teh webserver
     else if ( strcmp( buffer, "sounds") == 0) { send_sound_infos(newsockfd); close(newsockfd); }
     //internal dip-switch emulation, send all the infos to teh webserver
     else if ( strcmp( buffer, "dipsswitches") == 0) { send_dipswitch_infos(newsockfd); close(newsockfd); }
     //overview & control displays, send all the infos to teh webserver
     else if ( strcmp( buffer, "displays") == 0) { send_display_infos(newsockfd); close(newsockfd); }
     //overview and control solenoids, send all the infos to teh webserver
     //should we exit?
     else if ( strcmp( buffer, "exit") == 0) do_exit = 1;
     //we interpret all Messages with an uppercase 'V' as dip settings
     else if (buffer[0] == 'V') do_dip_set(buffer);
     //we interpret all Messages with an uppercase 'K' as dip comment settings
     else if (buffer[0] == 'K') do_dip_set(buffer);
     //we interpret all Messages with an uppercase 'L' as lamp settings
     else if (buffer[0] == 'L') do_lamp_set(buffer);
     //we interpret all Messages with an uppercase 'S' as sound settings
     else if (buffer[0] == 'S') do_sound_set(buffer);
     //we interpret all Messages with an uppercase 'C' as coil (solenoid) settings
     else if (buffer[0] == 'C') do_solenoid_set(buffer);
     //we interpret all Messages with an uppercase 'D' as display settings
     else if (buffer[0] == 'D') do_display_set(buffer);
     //as default we print out what we got
     else fprintf(stderr,"Message: %s\n",buffer);


  } while (do_exit == 0);

     close(newsockfd);
     close(sockfd);
     return 0; 
}

