/*
  fileio.c
  part of lisy80
  bontango 09.2016
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include<errno.h>
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "externals.h"





//handle dip definition file for current 'pin'
//mode
//mode == 0 -> set /boot to rw; open file and prepare header line
//mode == 1 -> append line to file
//mode == 2 -> appemnd the line, close file; set /boot to ro
int lisy80_file_write_dipfile( int mode, char *line )
{
 static FILE *fstream;
 char dip_file_name[80];


// open / init mode
if ( mode == 0 )
{
  //set mode to read - write
  system("/bin/mount -o remount,rw /boot");
  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY80_DIPS_PATH,lisy80_game.gamenr,LISY80_DIPS_FILE);
  //try to open the file with game nr for writing
  if ( ( fstream = fopen(dip_file_name,"w+")) == NULL ) return -1;
  //send header line
  fprintf(fstream,"Switch;ON_or_OFF;comment (%s) mame-ROM:%s.zip\n",lisy80_game.long_name, lisy80_game.gamename);
  //send first line
  fprintf(fstream,"%s",line);
  return 0;
}

//append mode
if ( mode == 1 )
{ 
  fprintf(fstream,"%s",line);
  return 0;
}

//close mode
if ( mode == 2 )
{
  fprintf(fstream,"%s",line);
  fclose(fstream);
  system("/bin/mount -o remount,ro /boot");
  return 0;
}

 return -2;
}

//handle dip definition file for current 'pin'
//mode
//mode == 0 -> set /boot to rw; open file and prepare header line
//mode == 1 -> append line to file
//mode == 2 -> appemnd the line, close file; set /boot to ro
int lisy1_file_write_dipfile( int mode, char *line )
{
 static FILE *fstream;
 char dip_file_name[80];

// open / init mode
if ( mode == 0 )
{
  //set mode to read - write
  system("/bin/mount -o remount,rw /boot");
  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY1_DIPS_PATH,lisy1_game.gamenr,LISY1_DIPS_FILE);
  //try to open the file with game nr for writing
  if ( ( fstream = fopen(dip_file_name,"w+")) == NULL ) return -1;
  //send header line
  fprintf(fstream,"Switch;ON_or_OFF;comment (%s) mame-ROM:%s.zip\n",lisy1_game.long_name, lisy1_game.gamename);
  //send first line
  fprintf(fstream,"%s",line);
  return 0;
}

//append mode
if ( mode == 1 )
{ 
  fprintf(fstream,"%s",line);
  return 0;
}

//close mode
if ( mode == 2 )
{
  fprintf(fstream,"%s",line);
  fclose(fstream);
  system("/bin/mount -o remount,ro /boot");
  return 0;
}

 return -2;
}


//read one dipswitch value with comment from definition file
//dip_nr is 1..32
// value is 0=off 1=on
// comment is comment from file
// *dip_setting_filename is filename with successfull read
// if re_init is >0 the settings from the file are read again
unsigned char lisy80_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filename, int re_init )
{

 int no;
 static int first_time = 1;
 static unsigned char value[32];
 static char comment[32][256];;
 static char dip_file_name[80];

 FILE *fstream;
 char buffer[1024];
 char *on_or_off;
 char *line;
 int first_line = 1;


 //do we need to read the file again?
 if ( re_init > 0) first_time = 1;

//read dip switch settings only once
 if (first_time)
 {
  //reset flag
  first_time = 0;

  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY80_DIPS_PATH,lisy80_game.gamenr,LISY80_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");

  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY80_DIPS_PATH,LISY80_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  if(fstream == NULL)
      {
        sprintf(dip_file_name,"PINMAME default as no file specified");
 	//copy filename where we was successfull to give back to calling routine
 	strcpy( dip_setting_filename, dip_file_name);
        return -1;
        }

   do
   {
     line=fgets(buffer,sizeof(buffer),fstream);
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     //first field is the number of the dip
     no = atoi(strtok(line, ";"));
     //second field is value ON or OFF
     on_or_off = strdup(strtok(NULL, ";"));
     if ( strcmp( on_or_off, "ON") == 0) value[no-1] = 1; else value[no-1] = 0;
     //third field is comment
     strcpy( comment[no-1],strtok(NULL, ";"));
   }
   while((line!=NULL) && (no!=32)); //make some basic reading checks

   //any error?
   if((line==NULL) || (no!=32)) return -1;

   fclose(fstream);

 } //done only first_time

 //give back stored values (from first time)
 strcpy(dip_comment,comment[dip_nr-1]);
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 return value[dip_nr-1];
}

//read one dipswitch value with comment from definition file
//dip_nr is 1..24
// value is 0=off 1=on
// comment is comment from file
// *dip_setting_filename is filename with successfull read
// if re_init is >0 the settings from the file are read again
unsigned char lisy1_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filename, int re_init )
{

 int no;
 static int first_time = 1;
 static unsigned char value[24];
 static char comment[24][256];;
 static char dip_file_name[80];

 FILE *fstream;
 char buffer[1024];
 char *on_or_off;
 char *line;
 int first_line = 1;


 //do we need to read the file again?
 if ( re_init > 0) first_time = 1;

//read dip switch settings only once
 if (first_time)
 {
  //reset flag
  first_time = 0;

  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY1_DIPS_PATH,lisy1_game.gamenr,LISY1_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");

  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY1_DIPS_PATH,LISY1_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  if(fstream == NULL)
      {
        sprintf(dip_file_name,"PINMAME default as no file specified");
 	//copy filename where we was successfull to give back to calling routine
 	strcpy( dip_setting_filename, dip_file_name);
        return -1;
        }

   do
   {
     line=fgets(buffer,sizeof(buffer),fstream);
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     //first field is the number of the dip
     no = atoi(strtok(line, ";"));
     //second field is value ON or OFF
     on_or_off = strdup(strtok(NULL, ";"));
     if ( strcmp( on_or_off, "ON") == 0) value[no-1] = 1; else value[no-1] = 0;
     //third field is comment
     strcpy( comment[no-1],strtok(NULL, ";"));
   }
   while((line!=NULL) && (no!=24)); //make some basic reading checks

   //any error?
   if((line==NULL) || (no!=24)) return -1;

   fclose(fstream);

 } //done only first_time

 //give back stored values (from first time)
 strcpy(dip_comment,comment[dip_nr-1]);
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 return value[dip_nr-1];
}

//read dipswitchsettings for specific game/mpu
//and give back settings or -1 in case of error
//switch_nr is 0..3
int lisy80_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename )
{

 typedef union dips {
    unsigned char byte;
    struct {
    unsigned DIP1:1, DIP2:1, DIP3:1, DIP4:1, DIP5:1, DIP6:1, DIP7:1, DIP8:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }t_dips;


 static t_dips lisy80_dip[4];
 static int first_time = 1;
 static char dip_file_name[80];

 int i;
 FILE *fstream;
 unsigned char dip[32];
 unsigned char dipvalue;
 char buffer[1024];
 int dip_nr;
 char *on_or_off;
 char *line;
 int first_line = 1;

//read dip switch settings only once
 if (first_time)
 {
  //reset flag
  first_time = 0;
  //set the defaults
  for (i=0; i<=3; i++) lisy80_dip[i].byte = -1;

  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY80_DIPS_PATH,lisy80_game.gamenr,LISY80_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");


  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY80_DIPS_PATH,LISY80_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  if(fstream == NULL)
      {
        sprintf(dip_file_name,"PINMAME default as no file specified");
 	//copy filename where we was successfull to give back to calling routine
 	strcpy( dip_setting_filename, dip_file_name);
        return -1;
        }

   do
   {
     line=fgets(buffer,sizeof(buffer),fstream);
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     dip_nr = atoi(strtok(line, ";"));
     on_or_off = strdup(strtok(NULL, ";"));
     if ( strcmp( on_or_off, "ON") == 0) dipvalue = 1; else dipvalue = 0;
     //assign dip value to temp var
     dip[dip_nr-1] = dipvalue;
   }
   while((line!=NULL) && (dip_nr!=32)); //make some basic reading checks

   //any error?
   if((line==NULL) || (dip_nr!=32)) return -1;

   //assign the dip settings
   for (i=0; i<=3; i++)
    {
        lisy80_dip[i].bitv.DIP1 =  dip[i*8 ];
        lisy80_dip[i].bitv.DIP2 =  dip[i*8 + 1];
        lisy80_dip[i].bitv.DIP3 =  dip[i*8 + 2];
        lisy80_dip[i].bitv.DIP4 =  dip[i*8 + 3];
        lisy80_dip[i].bitv.DIP5 =  dip[i*8 + 4];
        lisy80_dip[i].bitv.DIP6 =  dip[i*8 + 5];
        lisy80_dip[i].bitv.DIP7 =  dip[i*8 + 6];
        lisy80_dip[i].bitv.DIP8 =  dip[i*8 + 7];
    }

   fclose(fstream);

 } //done only first_time

 //debug?
 if(debug) 
  {
    fprintf(stderr,"mpudips return: ");
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP1);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP2);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP3);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP4);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP5);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP6);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP7);
    fprintf(stderr,"%d", lisy80_dip[switch_nr].bitv.DIP8);
    fprintf(stderr," for switch:%d\n\r",switch_nr);
   }
 
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 //return setting, this will be -1 if 'first_time' failed
 return lisy80_dip[switch_nr].byte;

}

//read dipswitchsettings for specific game/mpu
//and give back settings or -1 in case of error
//switch_nr is 0..2
int lisy1_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename )
{

 typedef union dips {
    unsigned char byte;
    struct {
    unsigned DIP1:1, DIP2:1, DIP3:1, DIP4:1, DIP5:1, DIP6:1, DIP7:1, DIP8:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }t_dips;


 static t_dips lisy1_dip[3];
 static int first_time = 1;
 static char dip_file_name[80];

 int i;
 FILE *fstream;
 unsigned char dip[32];
 unsigned char dipvalue;
 char buffer[1024];
 int dip_nr;
 char *on_or_off;
 char *line;
 int first_line = 1;

//read dip switch settings only once
 if (first_time)
 {
  //reset flag
  first_time = 0;
  //set the defaults
  for (i=0; i<=2; i++) lisy1_dip[i].byte = -1;

  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY1_DIPS_PATH,lisy1_game.gamenr,LISY1_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");


  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY1_DIPS_PATH,LISY1_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  if(fstream == NULL)
      {
        sprintf(dip_file_name,"PINMAME default as no file specified");
 	//copy filename where we was successfull to give back to calling routine
 	strcpy( dip_setting_filename, dip_file_name);
        return -1;
        }

   do
   {
     line=fgets(buffer,sizeof(buffer),fstream);
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     //interpret the line
     dip_nr = atoi(strtok(line, ";"));
     on_or_off = strdup(strtok(NULL, ";"));
     if ( strcmp( on_or_off, "ON") == 0) dipvalue = 1; else dipvalue = 0;
     //assign dip value to temp var
     dip[dip_nr-1] = dipvalue;
   }
   while((line!=NULL) && (dip_nr!=24)); //make some basic reading checks

   //any error?
   if((line==NULL) || (dip_nr!=24)) return -1;

   //assign the dip settings
   // we do some bit shifting 'by hand' as system1 is a bit special here
   // Switch 1 is assigned fifferent AND remember that we have a 4-bit system
   //group1 
   lisy1_dip[0].bitv.DIP4 =  dip[0];
   lisy1_dip[0].bitv.DIP3 =  dip[1];
   lisy1_dip[0].bitv.DIP2 =  dip[2];
   lisy1_dip[0].bitv.DIP1 =  dip[3];
   //group 2
   lisy1_dip[0].bitv.DIP8 =  dip[4];
   lisy1_dip[0].bitv.DIP7 =  dip[5];
   lisy1_dip[0].bitv.DIP6 =  dip[6];
   lisy1_dip[0].bitv.DIP5 =  dip[7];


   for (i=1; i<=2; i++)
    {
	//group1
        lisy1_dip[i].bitv.DIP1 =  dip[i*8 ];
        lisy1_dip[i].bitv.DIP2 =  dip[i*8 + 1];
        lisy1_dip[i].bitv.DIP3 =  dip[i*8 + 2];
        lisy1_dip[i].bitv.DIP4 =  dip[i*8 + 3];

        lisy1_dip[i].bitv.DIP5 =  dip[i*8 + 4];
        lisy1_dip[i].bitv.DIP6 =  dip[i*8 + 5];
        lisy1_dip[i].bitv.DIP7 =  dip[i*8 + 6];
        lisy1_dip[i].bitv.DIP8 =  dip[i*8 + 7];
    }

   fclose(fstream);

 } //done only first_time

 //debug?
 if(debug) 
  {
    fprintf(stderr,"mpudips return: ");
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP1);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP2);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP3);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP4);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP5);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP6);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP7);
    fprintf(stderr,"%d", lisy1_dip[switch_nr].bitv.DIP8);
    fprintf(stderr," for switch:%d\n\r",switch_nr);
   }
 
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 //return setting, this will be -1 if 'first_time' failed
 return lisy1_dip[switch_nr].byte;

}


//read the csv file on /boot partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisy80_games_csv
int  lisy80_file_get_gamename(t_stru_lisy80_games_csv *lisy80_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;

 //get value of dipswitch
 dip_switch_val = display_get_dipsw_value();
 //we are looking for the first six dips only for the game number
 dip_switch_val &= 0x3F;
 //see if we are 80/80A or 80B, mainly needed for display routines
 //all values >= LISY80_FIRST_80B are supposed to be 80B (ususally 40)
 if ( dip_switch_val  >= LISY80_FIRST_80B ) lisy80_game->is80B=1; else lisy80_game->is80B = 0;
 //set also global gamenr var to dip switch value, needed for dip switch settings
 lisy80_game->gamenr = dip_switch_val;

 FILE *fstream = fopen(LISY80_GAMES_CSV,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",LISY80_GAMES_CSV);
      return -1 ;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     line_no = atoi(strtok(line, ";")); 	//line number
     if ( dip_switch_val == line_no)
        { 
     	  strcpy(lisy80_game->gamename,strtok(NULL, ";"));	//game name short version
     	  strcpy(lisy80_game->type_from_csv,strtok(NULL, ";")); //type SYS80 / A /B
     	  strcpy(lisy80_game->long_name,strtok(NULL, ";"));	//game long name
     	  lisy80_game->gtb_no = atoi(strtok(NULL, ";"));	//Gottlieb number
     	  lisy80_game->throttle = atoi(strtok(NULL, ";"));	//throttle value per Gottlieb game
     	  strcpy(lisy80_game->comment,strtok(NULL, ";"));	//comment if available
          break;
	}
   } //while
   fclose(fstream);

  //give back the name and the number of the game
  return(line_no);
}

//read the csv file on /boot partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisy1_games_csv
int  lisy1_file_get_gamename(t_stru_lisy1_games_csv *lisy1_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;

 //get value of dipswitch
 dip_switch_val = display_get_dipsw_value();
 //we are looking for the first four dips only for the game number
 dip_switch_val &= 0x0F;
 //set also global gamenr var to dip switch value, needed for dip switch settings
 lisy1_game->gamenr = dip_switch_val;

 FILE *fstream = fopen(LISY1_GAMES_CSV,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY1: opening %s failed ",LISY1_GAMES_CSV);
      return -1 ;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     line_no = atoi(strtok(line, ";")); 	//line number
     if ( dip_switch_val == line_no)
        { 
     	  strcpy(lisy1_game->gamename,strtok(NULL, ";"));	//game name short version
     	  strcpy(lisy1_game->long_name,strtok(NULL, ";"));	//game long name
     	  strcpy(lisy1_game->rom_id,strtok(NULL, ";"));		//system 1 rom ID (1 char)
     	  lisy1_game->throttle = atoi(strtok(NULL, ";"));	//throttle value per Gottlieb game
     	  strcpy(lisy1_game->comment,strtok(NULL, ";"));	//comment if available
          break;
	}
   } //while
   fclose(fstream);

  //give back the name and the number of the game
  return(line_no);
}

//read the csv file for sound opts on /boot partition
//give -1 in case we had an error
//fill structure stru_lisy80_sound_csv
int  lisy80_file_get_soundopts(void)
{

 char buffer[1024];
 char *line;
 char sound_file_name[80];
 int sound_no;
 int first_line = 1;
 FILE *fstream;

 //construct the filename; using global var lisy80_gamenr
 sprintf(sound_file_name,"%s%03d%s",LISY80_SOUND_PATH,lisy80_game.gamenr,LISY80_SOUND_FILE);

 fstream = fopen(sound_file_name,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",sound_file_name);
      return -1;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     sound_no = atoi(strtok(line, ";")); 	//sound number
     lisy80_sound_stru[sound_no].can_be_interrupted = atoi(strtok(NULL, ";"));	
     lisy80_sound_stru[sound_no].loop = atoi(strtok(NULL, ";"));	
     lisy80_sound_stru[sound_no].st_a_catchup = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);

  return 0;
}
