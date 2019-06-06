/*
 mpfserver for lisy
 bontango 6.2016
 for version information look at mpfserver.h
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <fcntl.h> 
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <wiringPi.h>
#include <termios.h>
#include <errno.h>
#include <dirent.h>
#include "../fileio.h"
#include "../hw_lib.h"
#include "../coils.h"
#include "../displays.h"
#include "../switches.h"
#include "../eeprom.h"
#include "../utils.h"
#include "../sound.h"
#include "../fadecandy.h"
#include "../externals.h"
#include "../lisyversion.h"
#include "../linked_list.h"
#include "lisy_api.h"
#include "mpfserver.h"

char s_mpf_software_version[16];

//dummy shutdowns
void lisy1_shutdown( void ) { }
void lisy80_shutdown( void ) { }
void lisy35_shutdown( void ) { }

//the debug options
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;
int lisy80_is80B;

//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0 };
//internal switch Matrix for system1, we need 7 elements
//as pinmame internal starts with 1
//swMatrix 6 for SLAM and other special switches
unsigned char swMatrixLISY1[7] = { 0,0,0,0,0,0,0 };

//global var for coil min pulse time option, always activated for lisy1
int lisy1_coil_min_pulse_time[8] = { 0,0,0,0,0,0,0,0};

int lisy80_time_to_quit_flag; //not used here
//global var for additional options
//typedef is defined in hw_lib.h
ls80opt_t ls80opt;

//global var for internal game_name structure, set by  lisy80_file_get_gamename in main
t_stru_lisy80_games_csv lisy80_game;
t_stru_lisy1_games_csv lisy1_game;
t_stru_lisy35_games_csv lisy35_game;

//lisy80 pulse mod extension
int lisy80_coil_min_pulse_time[10] = { 0,0,0,0,0,0,0,0,0,0};
int lisy80_coil_min_pulse_mod = 0; //deaktivated by default

//global avr for sound optuions
t_stru_lisy80_sounds_csv lisy80_sound_stru[32];
int lisy_volume = 80; //SDL range from 0..128
//global var for all switches
unsigned char lisy_switches[80];
//global var for all lamps
unsigned char lisy_lamps[52];
//global var for all sounds
unsigned char lisy_sounds[32];
//global vars for all coils
unsigned char lisy_coils[10];

//global var for mpfserver
unsigned char isSYS1;
unsigned char lisy_display_chars[7] = { 0,0,0,0,0,0 };

/* Start with the empty list */
struct Node_sound_element* mpf_sound_list = NULL;
//flag for sound ( phat soundcard on LISY Board)
static unsigned char has_sound = 0;


void error(const char *msg)
{
    perror(msg);
}

//inits (more or less copy of inits in lisy1.c and lisy80.c
void lisy1_init( void )
{
   //init coils
   lisy1_coil_init( );
   //get the configured game
   lisy1_file_get_gamename( &lisy1_game);
   //show the 'boot' message
   display_show_boot_message_lisy1(s_mpf_software_version,lisy1_game.rom_id,lisy1_game.gamename);

}


void lisy80_init( void )
{
   //init coils
   lisy80_coil_init( );
   //get the configured game
   lisy80_file_get_gamename( &lisy80_game);
   //show the 'boot' message
   display_show_boot_message(s_mpf_software_version,lisy80_game.gtb_no,lisy80_game.gamename);
}

void lisy35_init( void )
{
}


//send back string
void send_back_string(int sockfd,unsigned char code,char *str)
{
  write(sockfd,str,strlen(str)+1);

 if (ls80dbg.bitv.basic)
 {
  if ( ( code < 100) && ( code != 41))
   {  sprintf(debugbuf,"code %d; send back string:%s\n",code,str);
      lisy80_debug(debugbuf);
   }
 }

}

//send back byte
void send_back_byte(int sockfd,unsigned char code,unsigned char answer)
{
  write(sockfd,&answer,1);

 if (ls80dbg.bitv.basic)
 {
if ( ( code < 100) && ( code != 41))  //not for watchdog & status switch
   {   sprintf(debugbuf,"code %d, send back byte:%d\n",code,answer);
       lisy80_debug(debugbuf);
   }
 }


}

//read next byte and give back
unsigned char read_next_byte(int sockfd, unsigned char code)
{
  unsigned char nextbyte;

  read(sockfd,&nextbyte,1);

/*
 if (ls80dbg.bitv.basic)
 {
  sprintf(debugbuf,"read parameter:%d\n",nextbyte);
  lisy80_debug(debugbuf);
 }
*/

  return nextbyte;
}

//read \0 terminated string and store into string content
unsigned char read_next_string(int sockfd, char *content)
{

  unsigned char nextbyte;
  int i=0;

 do {
  read(sockfd,&nextbyte,1);
  content[i] = nextbyte;
  i++;
  } while ( nextbyte != '\0');


}

//read next string and set display 
void set_display(int number, char *value)
{
 char display_str[25];

if (ls80dbg.bitv.displays)
 {
  sprintf(debugbuf,"received set command for display %d: \"%s\" \n",number,value);
  lisy80_debug(debugbuf);
 }

 //we just take the string and cut it to the rigth length, no further checking
 if (isSYS1)
 {
   if ( number == 0 )
   {
     sprintf(display_str,"%4s",value);
     display_show_str( 0, display_str);
   }
   else
   {
     sprintf(display_str,"%6s",value);
     display_show_str( number, display_str);
   }
 }//system1
 else
 {
   if (lisy80_game.is80B)
   {
     //only display number1 and 2 here, 20chars per display (which is in fact one display)
     sprintf(display_str,"%20s",value);
     if ( number == 1 ) display_send_row_torow( display_str, 0, 1 );
     if ( number == 2 ) display_send_row_torow( display_str, 1, 0 );
   }//80B
   else
   {
 	if ( number == 0 )
 	{
   	sprintf(display_str,"%4s",value);
   	display_show_str( 0, display_str);
 	}
 	else
 	{
   	if (( lisy80_game.type_from_csv[5] == 'A') && ( number < 5))  sprintf(display_str,"%7s",value); //80A has 7digit for 1-4
   	   else sprintf(display_str,"%6s",value); //RTH: with v21 wehad a 7s here ??
   	display_show_str( number, display_str);
 	}
  }//80 and 80A
 }//system80
}

//play a sound via the Gottlieb hardware
void play_sound(unsigned char code, unsigned char sound_no)
{

if (ls80dbg.bitv.sound)
 {
  sprintf(debugbuf,"MPF received play sound command:%d to play sound %d\n",code,sound_no);
  lisy80_debug(debugbuf);
 }

  if (isSYS1) lisy1_sound_set(sound_no);
  else lisy80_sound_set(sound_no);
}


//play the file via SDL2, file is in ./hardware_sounds
play_file(unsigned char option,char *filename)
{

     Mix_Music *newmusic;
     char filename_with_path[254];

  if (ls80dbg.bitv.sound)
   {
    sprintf(debugbuf,"MPF received play file command to play file %s, with option %d\n",filename,option);
    lisy80_debug(debugbuf);
   }
 if (has_sound)
 {
  // Checks whether the filename is present in linked list
  sprintf(filename_with_path,"%s%s",MPF_MP3_PATH,filename);
  newmusic = search_sound_element( mpf_sound_list, filename_with_path);
  if ( newmusic != NULL)
    mpf_play_mp3(newmusic);
  else
  {
   printf("problem mp3 playing %s\n",filename_with_path);
  }
 }//if has_sound
}

//say the text
say_text(unsigned char option,char *text)
{
if (ls80dbg.bitv.basic)
 {
  sprintf(debugbuf,"MPF received text to speech command to say %s, with option %d\n",text,option);
  lisy80_debug(debugbuf);
 }

 if (has_sound)
 {
  sprintf(debugbuf,"/bin/echo \"%s\" | /usr/bin/festival --tts &",text);
  system(debugbuf);
 }

}

//set volume, parameter is in percent
set_volume(unsigned char volume_in_percent)
{
if (ls80dbg.bitv.basic)
 {
  sprintf(debugbuf,"MPF received command to set volume to %d percent\n",volume_in_percent);
  lisy80_debug(debugbuf);
 }

 if (has_sound)
 {
     // set volume via amixer as it should work for all types of sundcards
     sprintf(debugbuf,"/usr/bin/amixer sset Digital %d%%",volume_in_percent);
     system(debugbuf);
 }

}

//read next  byte and set pulse time accordently
void set_coil_pulsetime(int sockfd, int number )
{

 unsigned char value;

if (ls80dbg.bitv.basic) lisy80_debug("LISY_S_PULSE_TIME\n");


 read(sockfd,&value,1);

if (ls80dbg.bitv.coils)
 {
  sprintf(debugbuf,"received pulsetime command for coil %d: %d \n",number,value);
  lisy80_debug(debugbuf);
 }


}

//check for updated switches
// get changed switches - return byte "bit7 is status; 0..6 is number"
// 127 means no change since last call"
unsigned char check_switch(void)
{

 unsigned char action;

 union both {
    unsigned char byte;
    struct {
    unsigned switchno:7, status:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } my_switch;

//set to default
my_switch.byte = 0;

 if (isSYS1)
 {
    my_switch.bitv.switchno = lisy1_switch_reader( &action );
    //SLAM handling is reverse in lisy1, meaning 0 is CLOSED
    //we suppress processing of SLAM Switch actions with option slam active
    if ( (my_switch.bitv.switchno == 76) && (ls80opt.bitv.slam == 1)) return 127; //no change

    //NOTE: system has has 8*5==40 switches in maximum, counting 00..04;10...14; ...
    //we use 'internal strobe 6' to handle special switches in the same way ( SLAM=06,OUTHOLE=16,RESET=26 )

  } //system1
  else 
  {
    my_switch.bitv.switchno = lisy80_switch_reader( &action );
  }//system80

  //valid for all systems
     if (my_switch.bitv.switchno < 80) {
        if ( action ) lisy_switches[my_switch.bitv.switchno] = my_switch.bitv.status = 0;
		 else lisy_switches[my_switch.bitv.switchno] = my_switch.bitv.status =  1;
	if (ls80dbg.bitv.switches)
	 {
	  sprintf(debugbuf,"MPF switch changed: %d: status:%d\n",my_switch.bitv.switchno,my_switch.bitv.status);
	  lisy80_debug(debugbuf);
	 }
	return my_switch.byte;
        }
 return 127; //no change
}

//set lamp to on or off
//in lisy we do lamp_set with coil_set
void set_lamp( int no, int action)
{

if (ls80dbg.bitv.lamps)
 {
  sprintf(debugbuf,"MPF set lamp %d: action:%d\n",no,action);
  lisy80_debug(debugbuf);
 }

 if (isSYS1)
 {
  if (action == 2)  //pulse
    {
     //now pulse the lamp
     lisy1_coil_set(no,1);
     usleep(COIL_PULSE_TIME);
     lisy1_coil_set(no,0);
     }
   else //on or off
     {
       lisy_lamps[no] = action;
       lisy1_coil_set(no,action);
     }
 }//system1
 else
 {
   //we have to add 1 to the lamp as lisy80 starts with lamp 1
   //and mpf starts (correctly) with 0
   no++;

  if (action == 2)  //pulse
    {
     //now pulse the lamp
     lisy80_coil_set(no,1);
     usleep(COIL_PULSE_TIME);
     lisy80_coil_set(no,0);
     }
   else //on or off
     {
       lisy_lamps[no] = action;
       lisy80_coil_set(no,action);
     }
 }//system80

}//set_lamp



//set coil to on, off or do pulse (action == 2)
int set_coil( int no, int action)
{
 int coil;


 //if the coil number (no) is 100 or more this is a lamp mapped as a coil in mpf
 //so we substract 100 and use set_lamp
 if ( no >= 100 ) 
  {
     if (ls80dbg.bitv.coils)
       {
         sprintf(debugbuf,"MPF coil %d remapped to %d\n",no,no-100);
         lisy80_debug(debugbuf);
        }
    set_lamp( no-100, action);
    return 0;
  }

if (ls80dbg.bitv.coils)
 {
  sprintf(debugbuf,"MPF set coil %d: action:%d\n",no,action);
  lisy80_debug(debugbuf);
 }

if (isSYS1)
 {

 //calculate rigth transistor
 switch(no)
   {
        case 1: coil=Q_OUTH;
                break;
        case 2: coil=Q_KNOCK;
                break;
        case 3: coil=Q_TENS;
                break;
        case 4: coil=Q_HUND;
                break;
        case 5: coil=Q_TOUS;
                break;
        case 6: coil=Q_SYS1_SOL6;
                break;
        case 7: coil=Q_SYS1_SOL7;
                break;
        case 8: coil=Q_SYS1_SOL8;
                break;
        default: return 2;
                 break;
   }
  if (action == 0)  //off
    {
      lisy_coils[no] = action;
      lisy1_coil_set(coil,0);
    }
  else if (action == 1)  //on
    {
      lisy_coils[no] = action;
      lisy1_coil_set(coil,1);
    }
  else if (action == 2)  //pulse
    {
     //now pulse the coil
     lisy1_coil_set(coil,1);
     usleep(COIL_PULSE_TIME);
     lisy1_coil_set(coil,0);
     }
 }//system1
else
 {
 //calculate rigth transistor
 switch(no)
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
        default: return 2;
                 break;
   }

  if (action == 0)  //off
    {
      lisy_coils[no] = action;
      lisy80_coil_set(coil,0);
    }
  else if (action == 1)  //on
    {
      lisy_coils[no] = action;
      lisy80_coil_set(coil,1);
    }
  else if (action == 2)  //pulse
    {
     //now pulse the coil
     lisy80_coil_set(coil,1);
     usleep(COIL_PULSE_TIME);
     lisy80_coil_set(coil,0);
     }
 }//system80
 return 0;
}

//subroutine for serial com
int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    //tty.c_cc[VTIME] = 1;
    tty.c_cc[VTIME] = 0; //RTH: no timimng here, input is not a keyboard

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


//main prg
int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, serfd;
     char *portname = "/dev/ttyGS0";
     socklen_t clilen;
     char buffer[256];
     char cur_version[8]; //current version for display
     unsigned char code;
     unsigned char parameter;
     struct sockaddr_in serv_addr, cli_addr;
     int i;
     //unsigned char action;
     struct stru_lisy_hw lisy_hw;
     unsigned char socket_mode;
     //music vars
     Mix_Music *newmusic;
     //dirent vars
     DIR *d;
     struct dirent *dir;
     int len;
     char file_with_path[512];


   

     //it is serial or lan(socket)  mode
     if ( (argc != 2) || ( (strncmp(argv[1],"socket",6) != 0 ) && (strncmp(argv[1],"serial",6) != 0 )))
        {
         printf("use: %s socket|serial\n",argv[0]);
	 exit (1);
	}


     if (strncmp(argv[1],"socket",6) == 0 )
 	 socket_mode=1;
     else
 	 socket_mode=0;

    //show up on calling terminal
    sprintf(s_mpf_software_version,"%02d.%03d ",MPFSERVER_SOFTWARE_MAIN,MPFSERVER_SOFTWARE_SUB);
    printf("This is MPF Server for LISY by bontango, Version %s (%s)\n",s_mpf_software_version,socket_mode ? "socket mode" : "serial mode");

     //do init the hardware
     lisy_hw_init(0);

    //set signal handler
    lisy80_set_sighandler();
 

    //set values to be returned to mpf
    sprintf(lisy_hw.lisy_ver,"%d.%02d ",LISY_SOFTWARE_MAIN,LISY_SOFTWARE_SUB);
    sprintf(lisy_hw.api_ver,"%d.%02d ",MPFSERVER_SOFTWARE_MAIN,MPFSERVER_SOFTWARE_SUB);

    //set HW
    if ( lisy_hardware_revision == 100 ) isSYS1 = 1; else isSYS1 = 0;

    //determine HW and set HW specific values
    if ( isSYS1 )
	{
	 strcpy( lisy_hw.lisy_hw,"LISY1");
	 lisy_hw.no_lamps = 36;
         lisy_hw.no_sol = 8;
	 lisy_hw.no_sounds = 7;
	 lisy_hw.no_disp = 5;
	 lisy_hw.no_switches = 30;  //5*6 Matrix
	 strcpy( lisy_hw.game_info,lisy1_game.rom_id);
	 lisy_display_chars[0] = 4;
	 lisy_display_chars[1] = 6;
	 lisy_display_chars[2] = 6;
	 lisy_display_chars[3] = 6;
	 lisy_display_chars[4] = 6;
	}
    else 
	{
	 strcpy( lisy_hw.lisy_hw,"LISY80");
	 lisy_hw.no_lamps = 52;
	 lisy_hw.no_sol = 9;
	 lisy_hw.no_sounds = 32;
	 lisy_hw.no_disp = 7; //RTH: for displays we need to do diff later on
	 lisy_hw.no_switches = 64; // 8*8 Matrix
	 sprintf( lisy_hw.game_info,"%3d",lisy80_game.gtb_no);

	 //number of chars per display
	 //RTH we migth insert system80 special cases (BH) here
	 if (lisy80_game.is80B)
         {
	  lisy_display_chars[1] = 20;
	  lisy_display_chars[2] = 20;
	 }
	 else if ( lisy80_game.type_from_csv[5] == 'A')
         {
	  lisy_display_chars[0] = 4;
	  lisy_display_chars[1] = 7;
	  lisy_display_chars[2] = 7;
	  lisy_display_chars[3] = 7;
	  lisy_display_chars[4] = 7;
	  lisy_display_chars[5] = 6;
	  lisy_display_chars[6] = 6;
	 }
	 else
         {
	  lisy_display_chars[0] = 4;
	  lisy_display_chars[1] = 6;
	  lisy_display_chars[2] = 6;
	  lisy_display_chars[3] = 6;
	  lisy_display_chars[4] = 6;
	  lisy_display_chars[5] = 6;
	  lisy_display_chars[6] = 6;
	 }
	}

   //init global vars; all switches supposed to be open, which means value 0 for MPF
   for (i=0; i<80; i++) lisy_switches[i] = 0;
   //init internal lamp vars as well
   for(i=0; i<=47; i++) lisy_lamps[i] = 0;
   for(i=48; i<=51; i++) lisy_lamps[i] = 1; //reversed lamps 44,45,46,47 lisy80 only
   //init internal coil vars as well
   for(i=0; i<=9; i++) lisy_coils[i] = 0;

    //init lisy1 or lisy80
    if ( lisy_hardware_revision == 100 )  //lisy1
	lisy1_init();
    else if ( ( lisy_hardware_revision == 311 ) || ( lisy_hardware_revision == 320 ))  //lisy80
	lisy80_init();
    else
	{
    		fprintf(stderr,"Fatal ERROR: Unknown Hardware\n");
		exit(1);
	}

    //show green ligth for now, lisy80 is running
    lisy80_set_red_led(0);
    lisy80_set_yellow_led(0);
    lisy80_set_green_led(1);

  //check for coil min_pulse parameter
  //option is active if value is either 0 or 1
  fprintf(stderr,"Info: checking for Coil min pulse time extension config\n");
  //first try to read coil opts, for current game
  if ( lisy1_file_get_coilopts() < 0 )
   {
     fprintf(stderr,"Info: no coil opts file; or error occured, using defaults\n");
   }
  else
   {
     fprintf(stderr,"Info: coil opt file read OK, min pulse time set\n");

     if ( ls80dbg.bitv.coils) {
     int i;
     for(i=0; i<=7; i++)
       fprintf(stderr,"coil No [%d]: %d msec minimum pulse time\n",i,lisy1_coil_min_pulse_time[i]);
    }
   }



   //switches, initial state
   //now via check_switch for all systems
   //in order to update internal buffer
   //and give a valid first feedback about status

   while ( check_switch() != 127 );

    //init sound
    if  ( mpf_sound_stream_init(ls80dbg.bitv.sound) == 0)
     {
	has_sound = 1;
	//set volume according to poti
  	lisy_adjust_volume();
  	sprintf(debugbuf,"/bin/echo \"Welcome to MPF Server Version %s\" | /usr/bin/festival --tts",s_mpf_software_version);
  	system(debugbuf);
     }

  //in case we have sound
  //preload sounds and put pointers to linked list
  //open directory with mp3 files and preload them
 if (has_sound)
 {
  d = opendir(MP3_SOUND_PATH);
  if (d) {
    while ((dir = readdir(d)) != NULL) {
  if (dir->d_type == DT_REG) //we only want regular files
  {
   len = strlen(dir->d_name);
   if ( ( len >= 5) && ( strcmp(&(dir->d_name[len-4]),".mp3") == 0) ) //we found something with extension mp3
     {
       sprintf(file_with_path,"%s/%s",MP3_SOUND_PATH,dir->d_name);
       //try to preload that file 
       newmusic = Mix_LoadMUS(file_with_path);

       if(!newmusic) {
          printf("Mix_LoadMUS(%s): %s\n", file_with_path, Mix_GetError());
          // this might be a critical error...
         }
      else
        {
          //create a new node
	  push_sound_element( &mpf_sound_list, file_with_path, newmusic);
          //debug?
          if (ls80dbg.bitv.basic)
	  {
           sprintf(debugbuf,"preload file:%s",file_with_path);
           lisy80_debug(debugbuf);
          }
        }
     }//we have an extenion mp3
    }
  }
  closedir(d);
  }
      else
        {
          sprintf(debugbuf,"cannot open dir %s",MP3_SOUND_PATH);
          lisy80_debug(debugbuf);
        }
 }//if has_sound


    //send something to the displays
    sprintf(cur_version,"v%d.%d",MPFSERVER_SOFTWARE_MAIN,MPFSERVER_SOFTWARE_SUB);
    if ( isSYS1 )
	{
   	 //system1
     	 display_show_str( 1, "LISY1 "); 
    	 display_show_str( 2, "MPFser"); 
    	 display_show_str( 3, cur_version); 
    	 display_show_str( 4, "WAIT  "); 
        }
     else
	{
   	 //no system80B at the moment
     	 display_show_str( 1, "LISY80A"); 
    	 display_show_str( 2, "MPFserv"); 
    	 display_show_str( 3, cur_version); 
    	 display_show_str( 4, "WAIT   "); 
        }
       

   //are we in 'socket mode'?
   if (socket_mode)
   {
   
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     //we want to to send the packets right away, no delay
     //if ( setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (int[]){1}, sizeof(int)) != 0)
     if ( setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (int[]){1}, sizeof(int)) != 0)
        error("ERROR setsockopt");
         
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
    }
   else //no we are in serial mode
    {
    serfd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (serfd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }
    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(serfd, B115200);


    }

 while (1) { //outer loop
    if(socket_mode)
    {
     //welcome message
     printf("mpf socket server for LISY v%d.%d  we listen at port 5963\n",MPFSERVER_SOFTWARE_MAIN,MPFSERVER_SOFTWARE_SUB);
     //wait and listen
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     } //we accept subsequent opens in socket mode
     else
     {
      //welcome message
      printf("mpf serial server for LISY v%d.%d  we listen at /dev/ttyGS0\n",MPFSERVER_SOFTWARE_MAIN,MPFSERVER_SOFTWARE_SUB);
      newsockfd = serfd;  //otherwise our new socket is the serial fd
     }

  //read it byte by byte
  while ( read(newsockfd,&code,1) == 1) {

     switch(code)
     {
	//info, parameter none
	case  LISY_G_HW               :       //get connected LISY hardware - return "LISY1" or "LISY80"
		send_back_string(newsockfd,code,lisy_hw.lisy_hw);
		break;
	case  LISY_G_LISY_VER         :       //get LISY Version - return String
		send_back_string(newsockfd,code,lisy_hw.lisy_ver);
		break;
	case  LISY_G_API_VER          :       //get API Version - return String
		send_back_string(newsockfd,code,lisy_hw.api_ver);
		break;
	case  LISY_G_NO_LAMPS         :       //get number of lamps - return byte
		send_back_byte(newsockfd,code,lisy_hw.no_lamps);
		break;
	case  LISY_G_NO_SOL           :       //get number of soleneoids - return byte
		send_back_byte(newsockfd,code,lisy_hw.no_sol);
		break;
	case  LISY_G_NO_SOUNDS        :       //get number of sounds - return byte
		send_back_byte(newsockfd,code,lisy_hw.no_sounds);
		break;
	case  LISY_G_NO_DISP          :       //get number of displays - return byte
		send_back_byte(newsockfd,code,lisy_hw.no_disp);
		break;
	case  LISY_G_DISP_DETAIL      :       //get display details - return string RTH: TBD
		send_back_string(newsockfd,code,"TBD");
		break;
	case  LISY_G_GAME_INFO        :       //get game info - return string 'Gottlieb internal number/char'
		send_back_string(newsockfd,code,lisy_hw.game_info);
		break;
	case  LISY_G_NO_SW            :       //get number of switches - return byte
		send_back_byte(newsockfd,code,lisy_hw.no_switches);
		break;

	//lamps, parameter byte
	case  LISY_G_STAT_LAMP        :      //get status of lamp # - return byte "0=OFF; 1=ON; 2=Error"
		parameter = read_next_byte(newsockfd,code);
		send_back_byte(newsockfd,code,lisy_lamps[parameter]);
		break;
	case  LISY_S_LAMP_ON          :      //set lamp # to ON - return none
		parameter = read_next_byte(newsockfd,code);
		set_lamp( parameter, 1);
		break;
	case  LISY_S_LAMP_OFF         :      //set lamp # to OFF - return none
		parameter = read_next_byte(newsockfd,code);
		set_lamp( parameter, 0);
		break;

	//solenoids, parameter byte
	case  LISY_G_STAT_SOL         :      //get status of solenoid # - return byte "0=OFF; 1=ON; 2=Error"
		parameter = read_next_byte(newsockfd,code);
		send_back_byte(newsockfd,code,lisy_coils[parameter]);
		break;
	case  LISY_S_SOL_ON           :      //set solenoid # to ON - return none
		parameter = read_next_byte(newsockfd,code);
		set_coil( parameter, 1);
		break;
	case  LISY_S_SOL_OFF          :      //set solenoid # to OFF - return none
		parameter = read_next_byte(newsockfd,code);
		set_coil( parameter, 0);
		break;
	case  LISY_S_PULSE_SOL        :      //pulse solenoid# - return none
		parameter = read_next_byte(newsockfd,code);
		set_coil( parameter, 2);
		break;
	case  LISY_S_PULSE_TIME       :       //set pulse time for solenoid# 1-byte ( 0 - 255 )
		parameter = read_next_byte(newsockfd,code);
		set_coil_pulsetime( newsockfd,parameter );
		break;

	//displays, parameter string
	case  LISY_S_DISP_0           :      //set display 0 (status) to string - return none
		read_next_string(newsockfd,buffer);
		set_display(0,buffer);
		break;
	case  LISY_S_DISP_1           :      //set display 1 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(1,buffer);
		break;
	case  LISY_S_DISP_2           :      //set display 2 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(2,buffer);
		break;
	case  LISY_S_DISP_3           :      //set display 3 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(3,buffer);
		break;
	case  LISY_S_DISP_4           :      //set display 4 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(4,buffer);
		break;
	case  LISY_S_DISP_5           :      //set display 5 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(5,buffer);
		break;
	case  LISY_S_DISP_6           :      //set display 6 to string - return none
		read_next_string(newsockfd,buffer);
		set_display(6,buffer);
		break;
	case  LISY_G_DISP_CH          :      //get possible number of characters of display#
		parameter = read_next_byte(newsockfd,code);
		send_back_byte(newsockfd,code,lisy_display_chars[parameter]);
		break;

	//switches, parameter byte/none
	case  LISY_G_STAT_SW          :      //get status of switch# - return byte "0=OFF; 1=ON; 2=Error"
		parameter = read_next_byte(newsockfd,code);
		send_back_byte(newsockfd,code,lisy_switches[parameter]);
		break;
	case  LISY_G_CHANGED_SW       :      //get changed switches - return byte "bit7 is status; 0..6 is number"
					     // "127 means no change since last call"
		send_back_byte(newsockfd,code,check_switch());
		break;
	//Sound (Hardware), parameter byte/none
	case  LISY_S_PLAY_SOUND       :      //play sound#
		parameter = read_next_byte(newsockfd,code);
		play_sound(code,parameter);
		break;
	case  LISY_S_STOP_SOUND       :      //stop all sounds ( same as play sound 0 )
		play_sound(code,0);
		break;
	case  LISY_S_PLAY_FILE       :      //play a file (in ./hardware_sounds) - option(1byte) + string 'filename'
		parameter = read_next_byte(newsockfd,code);
		read_next_string(newsockfd,buffer);
		play_file(parameter,buffer);
		break;
	case  LISY_S_TEXT_TO_SPEECH       :      //say the text - option(1byte) + string 'text'
		parameter = read_next_byte(newsockfd,code);
		read_next_string(newsockfd,buffer);
		say_text(parameter,buffer);
		break;
	case  LISY_S_SET_VOLUME       :      //sound volume in percent
		parameter = read_next_byte(newsockfd,code);
		set_volume(parameter);
		break;

	//general, parameter none
	case  LISY_INIT               :     //init/reset LISY - return byte 0=OK, >0 Errornumber Errornumbers TBD
		send_back_byte(newsockfd,code,0);
		break;
	case  LISY_WATCHDOG           :     //watchdog - return  byte 0=OK, >0 Errornumber Errornumbers TBD
		send_back_byte(newsockfd,code,0);
		break;

	default:
     		    //as default we print out what we got
		    fprintf(stderr,"unknown code: %d\n",code);

	}
   } // while read == 1
     if (socket_mode) close(newsockfd); //in socket more we will get a new newsocket
  } //while (1);

     //never reached
     close(newsockfd);
     if(socket_mode) close(sockfd);
     return 0; 
}

