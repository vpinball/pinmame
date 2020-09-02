/*
 socketserver for lisy80 webinterface
 bontango 9.2016
*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
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
#include <pthread.h>
#include "../lisy_w.h"
#include "../fileio.h"
#include "../hw_lib.h"
#include "../displays.h"
#include "../coils.h"
#include "../switches.h"
#include "../utils.h"
#include "../eeprom.h"
#include "../sound.h"
#include "../lisy.h"
#include "../fadecandy.h"
#include "../lisy_api.h"
#include "../usbserial.h"
#include "../utils.h"

//the version
#define LISY80control_SOFTWARE_MAIN    0
#define LISY80control_SOFTWARE_SUB     29


//fake definiton needed in lisy_w
void core_setSw(int myswitch, unsigned char action) {  };


//fake definiton needed in lisy1
void cpunum_set_clockscale(int cpu, float clockscale) {  };

//fake definiton needed in lisy80
typedef struct {
 struct {
    unsigned int  soundBoard;
  } hw;
} core_tGameData;

core_tGameData *core_gameData;

//fake definiton needed for mame functions
typedef struct
{
 unsigned char lampMatrix[2];
} t_coreGlobals;
t_coreGlobals coreGlobals;
void lisy_nvram_write_to_file( void ) {  }
void sound_stream_update(int *dum ) {  };
unsigned char sound_stream = 0;
unsigned char  sound_enabled = 0;




//the debug options
//in main prg set in  lisy80.c
//ls80dbg_t ls80dbg;
//int lisy80_is80B;
//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
extern unsigned char swMatrix[9];
//unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0 };
//int lisy80_time_to_quit_flag; //not used here
//global var for additional options
//typedef is defined in hw_lib.h
//ls80opt_t ls80opt;
unsigned char lisy80_gtb_socket_switches[80];
//global var for coil min pulse time option ( e.g. for spring break )
//int lisy80_coil_min_pulse_time[10] = { 0,0,0,0,0,0,0,0,0,0};
//int lisy80_coil_min_pulse_mod = 0; //deaktivated by default
//global var for coil min pulse time option, always activated for lisy1
//int lisy1_coil_min_pulse_time[8] = { 0,0,0,0,0,0,0,0};

//global vars
char switch_description_line1[80][80];
char switch_description_line2[80][80];
char lamp_description_line1[52][80];
char lamp_description_line2[52][80];
char coil_description_line1[52][80];
char coil_description_line2[52][80];
//global var for internal game_name structure, set by  lisy80_file_get_gamename in main
t_stru_lisy1_games_csv lisy1_game;
t_stru_lisy35_games_csv lisy35_game;
t_stru_lisy80_games_csv lisy80_game;

//global avr for sound optuions
t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
//int lisy_volume = 80; //SDL range from 0..128
//global var for all lamps
unsigned char lamp[52];
//global var for all sounds
unsigned char sound[32];
//global vars for all displays
//we use the same for SYS80 & SYS80A
char display_D0[8]="";
char display_D1[8],display_D2[8],display_D3[8],display_D4[8];
char display_D5[8]="";
char display_D6[8]="";
char display_row1[21],display_row2[21];
//hostname settings
char hostname[10]=" "; //' ' indicates that there is no new hostname
//hostname settings
char update_path[255]=" "; //' ' indicates that there is no update file


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
 //char wav_file_name[80];

 //the format here is 'Sxx' 
 //we trust ASCII values
 sound_no = (10 * (buffer[1]-48)) + buffer[2]-48;

 //set again same sound means set to zero
 if ( sound[sound_no] == 1)
  {
   lisy80_sound_set( 0 );
   //set internal var (remember)
   for(i=0; i<=31; i++) sound[i] = 0;
   sound[0] = 1;
   return;
  }


//do seound to zero first (needed?) 
 lisy80_sound_set( 0 );
 //set internal var (remember)
 for(i=0; i<=31; i++) sound[i] = 0;
 sound[sound_no] = 1;

 //JustBoom Sound? we may want to play wav files here
 if ( ls80opt.bitv.JustBoom_sound )
 {
  lisy80_play_wav(sound_no);
  //construct the filename, according to game_nr
  //sprintf(wav_file_name,"%s%03d/%d.wav",LISY80_SOUND_PATH,lisy80_game.gamenr,sound_no);
  //sprintf(debugbuf,"/usr/bin/aplay %s",wav_file_name);
  //system(debugbuf);
 }

 if ( ls80dbg.bitv.sound )
  {
    if(lisy80_game.is80B)
       sprintf(debugbuf,"80B:send sound %d",sound_no);
    else 
       sprintf(debugbuf,"send sound %d",sound_no);
    lisy80_debug(debugbuf);
  }

 //in case sound is >=16 we need to involve
 // Lamp9/Q10 which is sound 16 on system80 games
 // Lamp4/Q5 which is sound 16 on system80B games

 if(sound_no >= 16)
   { sound_no = sound_no -16;
     if(lisy80_game.is80B) lisy80_coil_set ( 5, 1);
     else lisy80_coil_set ( 10, 1);
   }
    else
   {
     if(lisy80_game.is80B) lisy80_coil_set ( 5, 0);
     else lisy80_coil_set ( 10, 0);
   }

 //now set sound
 lisy80_sound_set( sound_no );

}

//do an update of lisy
void do_updatepath_set( char *buffer)
{

 //we trust ASCII values
 //the format here is 'U_'
 sprintf(update_path,"%s",&buffer[2]);

 printf("update path: %s\n",&buffer[2]);

}

//do an update of lisy, local file
void do_update_local( int sockfd, char *what)
{

 char *real_name,*tmp_name;
 char *line;
 char buffer[255];

 //we trust ASCII values
 //the format here is 'Y'
 line = &what[1];

 real_name = strtok(line, ";");
 tmp_name = strtok(NULL, ";");

 sprintf(buffer,"<a href=\"./index.php\">Back to LISY Homepage</a><br><br>");
 sendit( sockfd, buffer);

 sprintf(buffer,"WE WILL NOW do the System update<br><br>\n");
 sendit( sockfd, buffer);

 //set mode to read - write
 sprintf(buffer,"setting system mode to read/write<br><br>\n");
 sendit( sockfd, buffer);
 system("/bin/mount -o remount,rw /boot");
 system("/bin/mount -o remount,rw /");

 //just unpack the lisy_update.tgz and execute install.sh from within
 sprintf(buffer,"try to get extract the update file<br><br>\n");
 sendit( sockfd, buffer);
 sprintf(buffer,"/bin/tar -xzf %s -C /home/pi/update",tmp_name);
 system(buffer);

 sprintf(buffer,"try to execute install.sh from within update pack<br><br>\n");
 sendit( sockfd, buffer);
 sprintf(buffer,"/bin/bash /home/pi/update/install.sh");
 system(buffer);

 sprintf(buffer,"update done, you may want to reboot now<br><br>\n");
 sendit( sockfd, buffer);

}

//set the hostname of the system and reboot
void do_hostname_set( char *buffer)
{

 //we trust ASCII values
 //the format here is 'H_'
 sprintf(hostname,"%s",&buffer[2]);

}

//set the display according to buffer message
void do_display_set( char *buffer)
{
 int display_no;
 char display_str[21];

 //we trust ASCII values
 display_no = buffer[1]-48;

 //the format here is 'Dx_', 'DxA_' or 'DxB_'
 if (buffer[2] == '_') //sys80
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
   case 5: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 5, display_str);
	        strcpy(display_D5,display_str);
		break;
   case 6: 
		sprintf(display_str,"%-6s",&buffer[3]);
 		display_show_str( 6, display_str);
	        strcpy(display_D6,display_str);
		break;
  }


 }
 else if (buffer[2] == 'A') //system80A
 {
  switch( display_no )
  {
   case 0: 
		sprintf(display_str,"%-4s",&buffer[4]);
 		display_show_str( 0, display_str);
	        strcpy(display_D0,display_str);
		break;
   case 1: 
		sprintf(display_str,"%-7s",&buffer[4]);
 		display_show_str( 1, display_str);
	        strcpy(display_D1,display_str);
		break;
   case 2: 
		sprintf(display_str,"%-7s",&buffer[4]);
 		display_show_str( 2, display_str);
	        strcpy(display_D2,display_str);
		break;
   case 3: 
		sprintf(display_str,"%-7s",&buffer[4]);
 		display_show_str( 3, display_str);
	        strcpy(display_D3,display_str);
		break;
   case 4: 
		sprintf(display_str,"%-7s",&buffer[4]);
 		display_show_str( 4, display_str);
	        strcpy(display_D4,display_str);
		break;
   case 5: 
		sprintf(display_str,"%-6s",&buffer[4]);
 		display_show_str( 5, display_str);
	        strcpy(display_D5,display_str);
		break;
   case 6: 
		sprintf(display_str,"%-6s",&buffer[4]);
 		display_show_str( 6, display_str);
	        strcpy(display_D6,display_str);
		break;
  }


 }
 else //system80B
 {
  switch( display_no )
  {
   case 1: 
		sprintf(display_str,"%-20s",&buffer[4]);
       		display_send_row_torow( display_str, 1, 0);
		break;
   case 2: 
		sprintf(display_str,"%-20s",&buffer[4]);
       		display_send_row_torow( display_str, 0, 1);
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
     //now pulse the coil
         lisy80_coil_set(coil,1);
         delay (150); // 150 milliseconds delay from wiringpi library
         lisy80_coil_set(coil,0);

}


//read all the set dip switch settings
//and store to filesystem
void do_dip_set( char *buffer)
{
 int dip_no,i;
 char dip_setting[10];
 static char part1[32][80];
 static char part2[32][257];
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
 if ( no_settings >=64 )
 {
   //dip number 1 ( 0 as we start with 0 ) means we need to open/create the file: mode = 0
   sprintf(line,"%s%s\n",part1[0],part2[0]);
   if ( lisy80_file_write_dipfile( 0, line ) < 0) syserr( "problems dip setting file", 77, 1);

   //now write settings 2..31 with mode 1
   for ( i=1; i<=30; i++)
    {   
      sprintf(line,"%s%s\n",part1[i],part2[i]);
      if ( lisy80_file_write_dipfile( 1, line ) < 0) syserr( "problems dip setting file", 77, 1);
    }

   //dip number 32 (31)  means we need to close the file after writing, mode = 2
   sprintf(line,"%s%s\n",part1[31],part2[31]);
   if ( lisy80_file_write_dipfile( 2, line ) < 0) syserr( "problems dip setting file", 77, 1);

   //reset number of settings
   no_settings = 0;
 }

}//do_dip_set

//the blinking thread
void *do_lamp_blink(void *myarg)
{
 int i,nu;
 int action = 1;
 int rev_action = 0;

 pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
 pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

 nu = (int *)myarg;

 while(1) //run untilmainfunction send cancel
 {
 if (action == 0) { rev_action=0; action = 1; } else { rev_action = 1; action = 0; }
 for(i=0; i<nu; i=i+2) lisy80_coil_set( i+1, action);
 lisy80_coil_set( 49, rev_action);
 lisy80_coil_set( 51, rev_action);

 if (action == 0) { rev_action=0; action = 1; } else { rev_action = 1; action = 0; }
 for(i=1; i<nu; i=i+2) lisy80_coil_set( i+1, action);
 lisy80_coil_set( 50, rev_action);
 lisy80_coil_set( 52, rev_action);

 sleep(1);
 if (action == 0) { rev_action=0; action = 1; } else { rev_action = 1; action = 0; }
 }
 return NULL;

}


//set lamp and update internal vars
void do_lamp_set( char *buffer)
{

 int lamp_no;
 int action;
 int rev_action;
 int i,nu;

//for the blinking thread
 static pthread_t p;
 int myarg;

 //the format here is 'Lxx_on' or 'Lxx_off'
 //we trust ASCII values
 lamp_no = (10 * (buffer[1]-48)) + buffer[2]-48;
 
 //on or off?
 if ( buffer[5] == 'f') { action=0; rev_action=1; } else  { action=1; rev_action=0; }

 nu = 48; //system80 has 48 lamps
 //special lamp 77 means ALL lamps
 if ( lamp_no == 77)
 {
  for ( i=0; i<nu; i++)
   {
     lisy80_coil_set( i+1 , action);
     lamp[i] = action;
     lamp[77] = action;
     //reverse action for lamps 45..48
     lamp[48] = rev_action;
     lisy80_coil_set( 49 , rev_action);
     lamp[49] = rev_action;
     lisy80_coil_set( 50 , rev_action);
     lamp[50] = rev_action;
     lisy80_coil_set( 51 , rev_action);
     lamp[51] = rev_action;
     lisy80_coil_set( 52 , rev_action);
   }
 }
 //special lamp 78 means blinking lamps with pthread
 else if ( lamp_no == 78)
 {
  myarg = nu;
  //start thread here
  lamp[lamp_no] = action;
  if (action) //start thread
    pthread_create (&p, NULL, do_lamp_blink, (void *)myarg);
  else //cancel thread
   {
    pthread_cancel (p);
    pthread_join (p, NULL);
  //and set all lamps to OFF
  for ( i=0; i<nu; i++)
   {
     lisy80_coil_set( i+1, action);
     lamp[i] = action;
   }
  }
 }
 else
 {
    //special handling for 44=~48; 45=~49;  46=~50;  47=~51
    switch(lamp_no)
    {
	case 44: lisy80_coil_set( 44 + 1, action); lamp[44] = action; lamp[48] = rev_action; break;
	case 45: lisy80_coil_set( 45 + 1, action); lamp[45] = action; lamp[49] = rev_action; break;
	case 46: lisy80_coil_set( 46 + 1, action); lamp[46] = action; lamp[50] = rev_action; break;
	case 47: lisy80_coil_set( 47 + 1, action); lamp[47] = action; lamp[51] = rev_action; break;
	case 48: lamp[48] = action; lamp[44] = rev_action; lisy80_coil_set( 44 + 1, rev_action); break;
	case 49: lamp[49] = action; lamp[45] = rev_action; lisy80_coil_set( 45 + 1, rev_action); break;
	case 50: lamp[50] = action; lamp[46] = rev_action; lisy80_coil_set( 46 + 1, rev_action); break;
	case 51: lamp[51] = action; lamp[47] = rev_action; lisy80_coil_set( 47 + 1, rev_action); break;
        default: lisy80_coil_set( lamp_no + 1, action); lamp[lamp_no] = action;
    }
  }
}

//read the lamp descriptions from the file
#define LISY80_LAMPS_PATH "/boot/lisy/lisy80/control/lamp_descriptions/"
#define LISY80_LAMPS_FILE "_lisy80_lamps.csv"
void get_lamp_descriptions(void)
{

   FILE *fstream;
   char lamp_file_name[80];
   char buffer[1024];
   char *line,*desc;
   int first_line = 1;
   int lamp_no;




 //construct the filename; using global var lisy80_gamenr
 sprintf(lamp_file_name,"%s%03d%s",LISY80_LAMPS_PATH,lisy80_game.gamenr,LISY80_LAMPS_FILE);

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
    sprintf(lamp_file_name,"%sdefault%s",LISY80_LAMPS_PATH,LISY80_LAMPS_FILE);
    fstream = fopen(lamp_file_name,"r");
      if(fstream != NULL)
      {
      fprintf(stderr,"LISY80 Info: lamp descriptions according to %s\n\r",lamp_file_name);
      }
    }//second try

  //check if first or second try where successfull
  if(fstream == NULL)
      {
        fprintf(stderr,"LISY80 Info: lamp descriptions not found \n\r");
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
          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( lamp_description_line1[lamp_no], desc);
                //remove trailing CR/LF
                lamp_description_line1[lamp_no][strcspn(lamp_description_line1[lamp_no], "\r\n")] = 0;
                //if we have an empty string, put one blank
                if( strlen( lamp_description_line1[lamp_no]) == 0) strcpy(  lamp_description_line1[lamp_no]," ");
                }
          else  strcpy ( lamp_description_line1[lamp_no], " ");

          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( lamp_description_line2[lamp_no], desc);
                //remove trailing CR/LF
                lamp_description_line2[lamp_no][strcspn(lamp_description_line2[lamp_no], "\r\n")] = 0;
                //if we have an empty string, put one blank
                if( strlen( lamp_description_line2[lamp_no]) == 0) strcpy(  lamp_description_line2[lamp_no]," ");
                }
          else  strcpy ( lamp_description_line2[lamp_no], " ");
        }

	else fprintf(stderr,"LISY80 Info: Lamp descriptions wrong info \n\r");
   }

}

//read the switch descriptions from the file
#define LISY80_SWITCHES_PATH "/boot/lisy/lisy80/control/switch_descriptions/"
#define LISY80_SWITCHES_FILE "_lisy80_switches.csv"
void get_switch_descriptions(void)
{

   FILE *fstream;
   char switch_file_name[80];
   char buffer[1024];
   char *line,*desc;
   int first_line = 1;
   int switch_no;




 //construct the filename; using global var lisy80_gamenr
 sprintf(switch_file_name,"%s%03d%s",LISY80_SWITCHES_PATH,lisy80_game.gamenr,LISY80_SWITCHES_FILE);

 //try to read the file with game nr
 fstream = fopen(switch_file_name,"r");
   if(fstream != NULL)
   {
      fprintf(stderr,"LISY80 Info: switch descriptions according to %s\n\r",switch_file_name);
   }
   else
   {
    //second try: to read the file with default
    //construct the new filename; using 'default'
    sprintf(switch_file_name,"%sdefault%s",LISY80_SWITCHES_PATH,LISY80_SWITCHES_FILE);
    fstream = fopen(switch_file_name,"r");
      if(fstream != NULL)
      {
      fprintf(stderr,"LISY80 Info: switch descriptions according to %s\n\r",switch_file_name);
      }
    }//second try

  //check if first or second try where successfull
  if(fstream == NULL)
      {
        fprintf(stderr,"LISY80 Info: DIP switch descriptions not found \n\r");
        return ;
        }
  //now assign teh descriptions
   while( ( line=fgets(buffer,sizeof(buffer),fstream)) != NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     switch_no = atoi(strtok(line, ";"));
     if (switch_no <80)
        {
          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( switch_description_line1[switch_no], desc);
                //remove trailing CR/LF
                switch_description_line1[switch_no][strcspn(switch_description_line1[switch_no], "\r\n")] = 0;
                }
          else  strcpy ( switch_description_line1[switch_no], "");

          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( switch_description_line2[switch_no], desc);
                //remove trailing CR/LF
                switch_description_line2[switch_no][strcspn(switch_description_line2[switch_no], "\r\n")] = 0;
                }
          else  strcpy ( switch_description_line2[switch_no], "");
        }
        else fprintf(stderr,"LISY80 Info: Switch descriptions wrong info \n\r");
   }

}

//SOLENOIDS

//read the coil descriptions from the file
#define LISY80_COILS_PATH "/boot/lisy/lisy80/control/coil_descriptions/"
#define LISY80_COILS_FILE "_lisy80_coils.csv"
void get_coil_descriptions(void)
{

   FILE *fstream;
   char coil_file_name[80];
   char buffer[1024];
   char *line,*desc;
   int first_line = 1;
   int coil_no;

 //construct the filename; using global var lisy80_gamenr
 sprintf(coil_file_name,"%s%03d%s",LISY80_COILS_PATH,lisy80_game.gamenr,LISY80_COILS_FILE);

 //try to read the file with game nr
 fstream = fopen(coil_file_name,"r");
   if(fstream != NULL)
   {
      fprintf(stderr,"LISY1 Info: coil descriptions according to %s\n\r",coil_file_name);
   }
   else
   {
    //second try: to read the file with default
    //construct the new filename; using 'default'
    sprintf(coil_file_name,"%sdefault%s",LISY80_COILS_PATH,LISY80_COILS_FILE);
    fstream = fopen(coil_file_name,"r");
      if(fstream != NULL)
      {
      fprintf(stderr,"LISY1 Info: coil descriptions according to %s\n\r",coil_file_name);
      }
    }//second try

  //check if first or second try where successfull
  if(fstream == NULL)
      {
        fprintf(stderr,"LISY1 Info: coil descriptions not found \n\r");
        return ;
        }
  //now assign teh descriptions
   while( ( line=fgets(buffer,sizeof(buffer),fstream)) != NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     coil_no = atoi(strtok(line, ";"));
     if (coil_no <80)
        {
          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( coil_description_line1[coil_no], desc);
                //remove trailing CR/LF
                coil_description_line1[coil_no][strcspn(coil_description_line1[coil_no], "\r\n")] = 0;
                //if we have an empty string, put one blank
                if( strlen( coil_description_line1[coil_no]) == 0) strcpy(  coil_description_line1[coil_no]," ");
                }
          else  strcpy ( switch_description_line1[coil_no], "");

          if (( desc = strtok(NULL, ";") ) != NULL )
              {
                strcpy ( coil_description_line2[coil_no], desc);
                //remove trailing CR/LF
                coil_description_line2[coil_no][strcspn(coil_description_line2[coil_no], "\r\n")] = 0;
                //if we have an empty string, put one blank
                if( strlen( coil_description_line2[coil_no]) == 0) strcpy(  coil_description_line2[coil_no]," ");
                }
          else  strcpy ( coil_description_line2[coil_no], "");
        }
        else fprintf(stderr,"LISY1 Info: coil descriptions wrong info \n\r");
   }

}


//send all the infos about the solenoids
void send_solenoid_infos( int sockfd )
{
  int i;
  char colorcode[80],buffer[512];

     //basic info, header line
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy80_game.long_name,lisy80_game.gamenr);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to PULSE specific solenoid<br><br>\n");
     sendit( sockfd, buffer);
     //the color and style
     strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; margin:auto; height: 5em;\'");

  for(i=1; i<=9; i++)
    {
      sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'C%02d\' %s value=\'%s\n%s\' /> </form>\n",i,colorcode,coil_description_line1[i-1],coil_description_line2[i-1]);
     sendit( sockfd, buffer);
    }
}


//send all the infos about the sound
void send_sound_infos( int sockfd )
{
  int i,j,k;
  char colorcode[80],buffer[512];

     //basic info, header line
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy80_game.long_name,lisy80_game.gamenr);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to send specific sound<br><br>\n");

i = 0;
for(j=0; j<=7; j++)
  {
  for(k=0; k<=3; k++)
    {
     if (sound[i]) strcpy(colorcode,"style=\'BACKGROUND-COLOR:yellow; width: 125px; height: 5em;\'");
                 else  strcpy(colorcode,"style=\'BACKGROUND-COLOR:powderblue; width: 125px; height: 5em;\'");
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'S%02d\' %s value=\'Sound\n%02d\' /> </form>\n",i,colorcode,i);
     sendit( sockfd, buffer);
    //next sound
    i++;
    }
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);
  }
}


void send_dipswitch_infos( int sockfd )
{

 int i;
 unsigned char dipvalue; 
 char dip_comment[256];
 char filename[80];
 char buffer[512];

 //basic info, header line
 sprintf(buffer,"Selected game is %s, internal number %d<br>\n",lisy80_game.long_name,lisy80_game.gamenr);
 sendit( sockfd, buffer);
 //dummy read to init read csv routine
 dipvalue = lisy80_file_get_onedip( 1, dip_comment, filename, 1 );
 //and the source where it came from
 sprintf(buffer,"DIP switch settings according to %s<br><br>\n",filename);
 sendit( sockfd, buffer);


 //now send the status
 sprintf(buffer,"<form action='' method='post'>");
 sendit( sockfd, buffer);

 for( i=1; i<=32; i++)
  {
    dipvalue = lisy80_file_get_onedip( i, dip_comment, filename, 0 );
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
  char colorcode[80],buffer[512],name[10];
 //colorcodes
  char *code_yellow = "style=\'BACKGROUND-COLOR:yellow; width: 125px; margin:auto; height: 5em;\'";
  char *code_blue = "style=\'BACKGROUND-COLOR:powderblue; width: 125px; margin:auto; height: 5em;\'";


     //basic info, header line
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy80_game.long_name,lisy80_game.gamenr);
     sendit( sockfd, buffer);
     sprintf(buffer,"push button to switch lamp OFF or ON  Yellow lamps are ON<br><br>\n");
     sendit( sockfd, buffer);

   //special lamp 77, set all lamps
   lamp_no = 77;
   if (lamp[lamp_no]) strcpy(colorcode,code_yellow); else  strcpy(colorcode,code_blue);
   if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
   sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'\n%s\n%s\' /> </form>\n" \
        ,name,colorcode,"set ALL lamps","");
     sendit( sockfd, buffer);
   //special lamp 78, blinking lamps via thread
   lamp_no = 78;
   if (lamp[lamp_no]) strcpy(colorcode,code_yellow); else  strcpy(colorcode,code_blue);
   if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
   sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'\n%s\n%s\' /> </form>\n" \
        ,name,colorcode,"ALL lamps blink","");
     sendit( sockfd, buffer);
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);

     //send all the cwlampsswitches together with the status
   for(i=0; i<=4; i++)
   {
    for(j=0; j<=9; j++)
    {
     lamp_no=i*10+j;
     if (lamp[lamp_no]) strcpy(colorcode,code_yellow); else  strcpy(colorcode,code_blue);
     if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'L%02d\n%s\n%s\' /> </form>\n",name,colorcode,lamp_no,lamp_description_line1[lamp_no],lamp_description_line2[lamp_no]);
     sendit( sockfd, buffer);
    }
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);
   }
    for(lamp_no=50; lamp_no<=51; lamp_no++)
    {
     if (lamp[lamp_no]) strcpy(colorcode,code_yellow); else  strcpy(colorcode,code_blue);
     if (lamp[lamp_no]) sprintf(name,"L%02d_off",lamp_no); else sprintf(name,"L%02d_on",lamp_no);
     sprintf(buffer,"<form action=\'\' method=\'post\'><input type=\'submit\' name=\'%s\' %s value=\'L%02d\n%s\n%s\' /> </form>\n",name,colorcode,lamp_no,lamp_description_line1[lamp_no],lamp_description_line2[lamp_no]);
     sendit( sockfd, buffer);
    }
   sprintf(buffer,"<br>\n");
   sendit( sockfd, buffer);
}


void send_update_infos( int sockfd )
{
  char buffer[512];
  int ret_val;

  if ( update_path[0] != ' ')  //there was a setting of the update path
  {
    sprintf(buffer,"WE WILL NOW do the update<br><br>\n");
    sendit( sockfd, buffer);

    //set mode to read - write
    sprintf(buffer,"setting system mode to read/write<br><br>\n");
    sendit( sockfd, buffer);
    system("/bin/mount -o remount,rw /boot");
    system("/bin/mount -o remount,rw /");

    //clean up /home/pi/update
    sprintf(buffer,"clean up update dir<br><br>\n");
    sendit( sockfd, buffer);
    sprintf(buffer,"/bin/rm -rf /home/pi/update/*");
    ret_val = system(buffer);

    //try to get the update file with wget
    sprintf(buffer,"try to get the update file with wget<br><br>\n");
    sendit( sockfd, buffer);
    sprintf(buffer,"/usr/bin/wget %s -O /home/pi/update/lisy_update.tgz",update_path);
    ret_val = system(buffer);

    if (ret_val != 0)
    {
     sprintf(buffer,"return value of wget was %d<br><br>",ret_val);
     sendit( sockfd, buffer);
     sprintf(buffer,"update failed<br><br>");
     sendit( sockfd, buffer);
    }
 else  //just unpack the lisy_update.tgz and execute install.sh from within
    {
      sprintf(buffer,"try to get extract the update file<br><br>\n");
      sendit( sockfd, buffer);
      sprintf(buffer,"/bin/tar -xzf /home/pi/update/lisy_update.tgz -C /home/pi/update");
      ret_val = system(buffer);

      sprintf(buffer,"try to execute install.sh from within update pack<br><br>\n");
      sendit( sockfd, buffer);
      sprintf(buffer,"/bin/bash /home/pi/update/install.sh");
      ret_val = system(buffer);

      sprintf(buffer,"update done, you may want to reboot now<br><br>\n");
      sendit( sockfd, buffer);

    }

    //reset update path to prevent endless loop
    update_path[0] = ' ';


   }
  else  //normal header
  {

  //start with some header
  sprintf(buffer,"do an update, with the URL specified<br><br>\n");
  sendit( sockfd, buffer);
  sprintf(buffer,"NOTE: do only use URLs from lisy.dev<br><br>\n");
  sendit( sockfd, buffer);
  sprintf(buffer,"for latest update use: http://www.flipperkeller.de/lisy/lisy_update.tgz <br><br>\n");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p>  Update path: <input type=\"text\" name=\"U\" size=\"100\" maxlength=\"250\" value=\"\" /></p>");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p><input type=\"submit\" /></p> ");
  sendit( sockfd, buffer);
  }

}

void send_hostname_infos( int sockfd )
{
  char buffer[256];

  if ( hostname[0] != ' ')  //there was a setting of the hostname
  {
    sprintf(buffer,"WE WILL NOW SET the hostname of the system<br><br>\n");
    sendit( sockfd, buffer);

    //set mode to read - write
    sprintf(buffer,"setting system mode to read/write<br><br>\n");
    sendit( sockfd, buffer);
    system("/bin/mount -o remount,rw /boot");
    system("/bin/mount -o remount,rw /");

    //create new /etc/hosts
    sprintf(buffer,"creating a new /etc/hosts<br><br>\n");
    sendit( sockfd, buffer);
    system("/usr/bin/printf \"127.0.0.1\tlocalhost\n\" >/etc/hosts");
    sprintf(buffer,"/usr/bin/printf \"127.0.0.1\t%s\" >>/etc/hosts",hostname);
    system(buffer);

    //create new /etc/hostname
    sprintf(buffer,"creating a new /etc/hosts<br><br>\n");
    sendit( sockfd, buffer);
    sprintf(buffer,"/usr/bin/printf \"%s\n\" >/etc/hostname",hostname);
    system(buffer);

    //create new /etc/hostname
    sprintf(buffer,"now sync and rebooting<br><br>\n");
    sendit( sockfd, buffer);
    system("/bin/sync");
    sleep(2);
    system("/sbin/reboot");

    //reset hostname setting to prevent endless loop
    hostname[0] = ' ';
  }
  else  //normal header
  {

  //start with some header
  sprintf(buffer,"set the hostname of the system<br><br>\n");
  sendit( sockfd, buffer);
  sprintf(buffer,"NOTE: system will be rebooted afterwards!!<br><br>\n");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p>  Hostname: <input type=\"text\" name=\"H\" size=\"32\" maxlength=\"32\" value=\"\" /></p>");
  sendit( sockfd, buffer);

  sprintf(buffer,"<p><input type=\"submit\" /></p> ");
  sendit( sockfd, buffer);
  }

}


void send_display_infos( int sockfd )
{
  char buffer[256];

  //start with some header
  sprintf(buffer,"Selected game is %s, which is a Gottlieb %s<br><br>\n",lisy80_game.long_name,lisy80_game.type_from_csv);
  sendit( sockfd, buffer);

 //show display submit forms depending on what type of System80 we have
 if ( strcmp(lisy80_game.type_from_csv,"SYS80") == 0 )
 {
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
 sprintf(buffer,"<p>Display Player 5: <input type=\"text\" name=\"D5\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D5);
 //sprintf(buffer,"<p>Display Player 5: <input type=\"text\" name=\"D5\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 6: <input type=\"text\" name=\"D6\" size=\"6\" maxlength=\"6\" value=\"%6s\" /></p>",display_D6);
 //sprintf(buffer,"<p>Display Player 6: <input type=\"text\" name=\"D6\" size=\"6\" maxlength=\"6\" /></p>");
 sendit( sockfd, buffer);
 }
 else if ( strcmp(lisy80_game.type_from_csv,"SYS80A") == 0 )
 {
 sprintf(buffer,"<p>  Status Display: <input type=\"text\" name=\"D0\" size=\"4\" maxlength=\"4\" value=\"%4s\" /></p>",display_D0);
 //sprintf(buffer,"<p>  Status Display: <input type=\"text\" name=\"D0\" size=\"4\" maxlength=\"4\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 1: <input type=\"text\" name=\"D1A\" size=\"7\" maxlength=\"7\" value=\"%7s\" /></p>",display_D1);
 //sprintf(buffer,"<p>Display Player 1: <input type=\"text\" name=\"D1A\" size=\"7\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 2: <input type=\"text\" name=\"D2A\" size=\"7\" maxlength=\"7\" value=\"%7s\" /></p>",display_D2);
 //sprintf(buffer,"<p>Display Player 2: <input type=\"text\" name=\"D2A\" size=\"7\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 3: <input type=\"text\" name=\"D3A\" size=\"7\" maxlength=\"7\" value=\"%7s\" /></p>",display_D3);
 //sprintf(buffer,"<p>Display Player 3: <input type=\"text\" name=\"D3A\" size=\"7\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 4: <input type=\"text\" name=\"D4A\" size=\"7\" maxlength=\"7\" value=\"%7s\" /></p>",display_D4);
 //sprintf(buffer,"<p>Display Player 4: <input type=\"text\" name=\"D4A\" size=\"7\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 5: <input type=\"text\" name=\"D5\" size=\"6\" maxlength=\"6\" value=\"%7s\" /></p>",display_D5);
 //sprintf(buffer,"<p>Display Player 5: <input type=\"text\" name=\"D5\" size=\"6\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Player 6: <input type=\"text\" name=\"D6\" size=\"6\" maxlength=\"6\" value=\"%7s\" /></p>",display_D6);
 //sprintf(buffer,"<p>Display Player 6: <input type=\"text\" name=\"D6\" size=\"6\" maxlength=\"7\" /></p>");
 sendit( sockfd, buffer);
 }
 else if ( strcmp(lisy80_game.type_from_csv,"SYS80B") == 0 )
 {
 sprintf(buffer,"<p>Display Reihe 1: <input type=\"text\" name=\"D1B\" size=\"20\" maxlength=\"20\" value=\"%20s\" /></p>",display_row1);
 //sprintf(buffer,"<p>Display Reihe 1: <input type=\"text\" name=\"D1B\" size=\"20\" maxlength=\"20\" /></p>");
 sendit( sockfd, buffer);
 sprintf(buffer,"<p>Display Reihe 2: <input type=\"text\" name=\"D2B\" size=\"20\" maxlength=\"20\" value=\"%20s\" /></p>",display_row2);
 //sprintf(buffer,"<p>Display Reihe 2: <input type=\"text\" name=\"D2B\" size=\"20\" maxlength=\"20\" /></p>");
 sendit( sockfd, buffer);
 }
 else
 {
 sprintf(buffer,"<p>wrong Gottlieb type, check CSV file</p>");
 sendit( sockfd, buffer);
 }

}



void send_switch_infos( int sockfd )
{
  int ret,i,j,switch_no;
  unsigned char action;
  char colorcode[80],buffer[512];
  unsigned char strobe,returnval;
  //colorcodes
  char *code_red = "<td align=center style=\"background-color:red;\">";
  char *code_green = "<td align=center>";



  //update internal switch matrix with buffer from switch pic
 do
    {
     ret = lisy80_switch_reader( &action );

     if (ret < 80) //ret is switchnumber: NOTE: there are no switches with x8 & x9
      {
 
        lisy80_gtb_socket_switches[ret] = action;
	
	//we need to do the following  in order not to confues hw_lib which checks for double sets
        strobe = ret / 10;
        returnval = ret % 10;

 	//set the bit in the Matrix var according to action
  	// action 0 means set the bit
  	// any other means delete the bit
  	if (action == 0) //set bit
    	  swMatrix[returnval+1] |= ( 1 << strobe );
  	else  //delete bit
    	  swMatrix[returnval+1] &= ~( 1 << strobe );
        }
     }while( ret < 80);

     //now send whole matrix back together with some header
     sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy80_game.long_name,lisy80_game.gamenr);
     sendit( sockfd, buffer);


     for(i=0; i<=7; i++) 
     {
      sprintf(buffer,"<tr style=\"background-color:lawngreen;\" border=\"1\">\n");
      sendit( sockfd, buffer);
       for(j=0; j<=7; j++) 
       { 
	switch_no = i*10 + j;
 	//assign color, red is closed, green is open, default fo table is green
	//if (lisy80_gtb_socket_switches[switch_no]) strcpy(colorcode,"<td>"); else  strcpy(colorcode,"<td style=\"background-color:red;\">");
	if (lisy80_gtb_socket_switches[switch_no])  strcpy(colorcode,code_green); else  strcpy(colorcode,code_red);
	sprintf(buffer,"%sSwitch %02d<br>%s<br>%s</td>\n",colorcode,switch_no,switch_description_line1[switch_no],switch_description_line2[switch_no]);
        sendit( sockfd, buffer);
	}
      sprintf(buffer,"</tr>\n");
      sendit( sockfd, buffer);
     }


}



//send infos for the homepage
void send_home_infos( int sockfd )
{
     char buffer[256];

   sprintf(buffer,"<h2>LISY80 Webeditor Home Page</h2> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"This is LISY80control version %d.%d<br>\n",LISY80control_SOFTWARE_MAIN,LISY80control_SOFTWARE_SUB);
   sendit( sockfd, buffer);
   sprintf(buffer,"Selected game is %s, internal number %d<br><br>\n",lisy80_game.long_name,lisy80_game.gamenr);
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_switches.php\">Switches</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_lamps.php\">Lamps</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_solenoids.php\">Solenoids</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_displays.php\">Displays</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_dipswitches.php\">DIP Switches</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_sound.php\">Sound</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_nvram.php\">NVRAM Information</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_software.php\">Software installed</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./hostname.php\">Set the hostname of the system</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./update.php\">update System via internet </a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./update_local.html\">update System with local tgz file</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./upload.html\">upload new lamp, coil or switch configuration files</a><br><br> \n");
   sendit( sockfd, buffer);
   sprintf(buffer,"<p>\n<a href=\"./lisy80_exit.php\">Exit and reboot</a><br><br> \n");
   sendit( sockfd, buffer);

}

//send software version(s)
void send_software_infos( int sockfd )
{
     char buffer[512];
     char versionstring[256];
     int dum;
     FILE *fp;

   //get installed version of lisy
   // Open the command for reading.
  fp = popen("/usr/local/bin/lisy -lisyversion", "r");
  if (fp == NULL) {
   sprintf(buffer,"LISY Version: unknown(internal error)\n");
   sendit( sockfd, buffer);
  }
  else
  {
   fgets(versionstring, sizeof(versionstring)-1, fp);
   sprintf(buffer,"LISY Version: %s<br>\n",versionstring);
   sendit( sockfd, buffer);
   pclose(fp);
  }

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
   myblock = lisy_eeprom_getstats();

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
//do upload 'csv' files
void do_upload( int sockfd, char *what)
{

 char *destination, *source;
 char *line;
 char buffer[256];
 int ret_val;
 //we trust ASCII values
 //the format here is 'U_'

 line = &what[1];

 destination = strtok(line, ";");
 source = strtok(NULL, ";");

 sprintf(buffer,"<a href=\"./index.php\">Back to LISY1 Homepage</a><br><br>");
 sendit( sockfd, buffer);

 sprintf(buffer,"WE WILL NOW do the upload<br><br>\n");
 sendit( sockfd, buffer);

 //set mode to read - write
 sprintf(buffer,"setting system mode to read/write<br><br>\n");
 sendit( sockfd, buffer);
 system("/bin/mount -o remount,rw /boot");

 sprintf(buffer,"we copy file to %s<br><br>\n",destination);
 sendit( sockfd, buffer);

 sprintf(buffer,"/bin/cp %s %s",source,destination);
 printf("/bin/cp %s %s\n",source,destination);
 ret_val = system(buffer);


 if (ret_val != 0)
    {
     sprintf(buffer,"return value of cp was %d<br><br>",ret_val);
     sendit( sockfd, buffer);
     sprintf(buffer,"upload failed<br><br>");
     sendit( sockfd, buffer);
    }
  else
    {
     sprintf(buffer,"upload done<br><br>");
     sendit( sockfd, buffer);
     sprintf(buffer,"we re-read the cofig files now<br><br>\n");
     sendit( sockfd, buffer);

    //re-read configurations
    get_switch_descriptions();
    get_lamp_descriptions();
    get_coil_descriptions();
    }

 //set mode to read only again
 sprintf(buffer,"setting system mode back to to read only<br><br>\n");
 sendit( sockfd, buffer);
 system("/bin/mount -o remount,ro /boot");

 sprintf(buffer,"all done<br><br>\n");
 sendit( sockfd, buffer);

}

//****** MAIN  ******

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     char ip_interface[10];
     struct sockaddr_in serv_addr, cli_addr, *myip;
     int i,n,res;
     int do_exit = 0;
     struct ifreq ifa;
     char *line;
     int tries = 0;
     char lisy_gamename[20];
     char lisy_variant[20];

     //init global vars; all switches supposed to be open, whcih means value 1
     for (i=0; i<80; i++) lisy80_gtb_socket_switches[i] = 1;
     //init switch description
     for (i=0; i<80; i++)
      { 
         strcpy ( switch_description_line1[i], "NOT SET");
         strcpy ( switch_description_line2[i], "");
      }


     //check which pinball we are going to control
     //this will also call lisy_hw_init
     strcpy(lisy_variant,"lisy80");
     if ( (res = lisy_set_gamename(lisy_variant, lisy_gamename)) != 0)
           {
             fprintf(stderr,"LISY80: no matching game or other error\n\r");
             return (-1);
           }

    //use the init functions from lisy.c
    lisy_init();


    //init coils
    lisy80_coil_init( );
    //init internal lamp vars as well
    for(i=0; i<=47; i++) lamp[i] = 0;
    for(i=48; i<=51; i++) lamp[i] = 1; //reversed lamps 44,45,46,47

    //init sound
    lisy80_sound_set(0);  // zero
    //init internal sound vars as well
    sound[0] = 1;
    for(i=1; i<=31; i++) sound[i] = 0;

    //read the descriptions for the switches
    get_switch_descriptions();
    //read the descriptions for the lamps
    get_lamp_descriptions();
    //read the descriptions for the coils
    get_coil_descriptions();

 // try say something about LISY80 if sound is requested
 if ( ls80opt.bitv.JustBoom_sound )
 {
  //set volume according to poti
  sprintf(debugbuf,"/bin/echo \"Welcome to LISY 80 Control Version 0.%d\" | /usr/bin/festival --tts",LISY80control_SOFTWARE_SUB);
  system(debugbuf);
 }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     //try to find out our IP on eth0
     strcpy (ifa.ifr_name, "eth0");
     strcpy (ip_interface, "ETH0"); //upercase for message
     if((n=ioctl(sockfd, SIOCGIFADDR, &ifa)) != 0)
      {
        //no IP on eth0, we try wlan0 now, 20 times
        strcpy (ifa.ifr_name, "wlan0");
        strcpy (ip_interface, "WLAN0"); //upercase for message
        do
        {
          sleep(1);
          n=ioctl(sockfd, SIOCGIFADDR, &ifa);
          tries++;
          if ( ls80dbg.bitv.basic )
           {
             sprintf(debugbuf,"get IP of wlan0: try number:%d",tries);
             lisy80_debug(debugbuf);
           }
        } while ( (n!=0) &( tries<20));
      }


     if(n) //no IP found
     {
      strcpy (ifa.ifr_name, "noip");

      if (lisy80_game.is80B)
      {
       //construct the message
       sprintf(buffer,"NO IP ASSIGNED      ");
       display_send_row_torow( buffer, 1, 0);
       fprintf(stderr,"%s\n",buffer);
       sprintf(buffer,"CHECK WLAN0 & ETH0  ");
       display_send_row_torow( buffer, 0, 1);
       fprintf(stderr,"%s\n",buffer);
       }//is 80B
       else
       {
       //construct the message
        fprintf(stderr,"SYS80/A NO IP");
        display_show_str( 1, "NO IP  ");
        display_show_str( 2, "CHECK  ");
        display_show_str( 3, "ETH0   ");
        display_show_str( 4, "WLAN0  ");
       }//80 or 80A
     }
     else //we found an IP
     {
      myip = (struct sockaddr_in*)&ifa.ifr_addr;
      if (lisy80_game.is80B)
      {
       //construct the message
       sprintf(buffer,"IP %17s",inet_ntoa(myip->sin_addr));
       display_send_row_torow( buffer, 1, 0);
       //and store value for display routine
       strcpy(display_row1,buffer);
       fprintf(stderr,"%s\n",buffer);
       sprintf(buffer,"ON INTERFACE %7s",ip_interface);
       //and store value for display routine
       strcpy(display_row2,buffer);
       display_send_row_torow( buffer, 0, 1);
       fprintf(stderr,"%s\n",buffer);
       }//is 80B
       else
       {
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
        }//80 or 80A
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
     //provide simple input field for setting hostname other then the default
     else if ( strcmp( buffer, "hostname") == 0) { send_hostname_infos(newsockfd); close(newsockfd); }
     //provide simple input field for initiating update
     else if ( strcmp( buffer, "update") == 0) { send_update_infos(newsockfd); close(newsockfd); }
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
     //with an uppercase 'H' we do setting a new hostname to the system and reboot
     else if (buffer[0] == 'H') do_hostname_set(buffer);
     //with an uppercase 'U' we do try to initiate an update of lisy
     else if (buffer[0] == 'U') do_updatepath_set(buffer);
     //with an uppercase 'X' we do try to initiate upload of csv files
     else if (buffer[0] == 'X') { do_upload(newsockfd,buffer);close(newsockfd); }
     //with an uppercase 'Y' we do update the system with clientfile
     else if (buffer[0] == 'Y') { do_update_local(newsockfd,buffer);close(newsockfd); }
     //as default we print out what we got
     else fprintf(stderr,"Message: %s\n",buffer);


  } while (do_exit == 0);

     close(newsockfd);
     close(sockfd);
     //menu says: exit and reboot: so do it
     system("/sbin/reboot");
     return 0; 
}

