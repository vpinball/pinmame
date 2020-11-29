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
#include "coils.h"
#include "fadecandy.h"
#include "opc.h"
#include "utils.h"
#include "lisy_home.h"
#include "externals.h"


//handle dip definition file for current 'pin'
//mode
//mode == 0 -> set /lisy to rw; open file and prepare header line
//mode == 1 -> append line to file
//mode == 2 -> appemnd the line, close file; set /lisy to ro
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
//mode == 0 -> set /lisy to rw; open file and prepare header line
//mode == 1 -> append line to file
//mode == 2 -> appemnd the line, close file; set /lisy to ro
int lisy35_file_write_dipfile( int mode, char *line )
{
 static FILE *fstream;
 char dip_file_name[80];


// open / init mode
if ( mode == 0 )
{
  //set mode to read - write
  system("/bin/mount -o remount,rw /boot");
  //construct the filename; using global var lisy35_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY35_DIPS_PATH,lisy35_game.gamenr,LISY35_DIPS_FILE);
  //try to open the file with game nr for writing
  if ( ( fstream = fopen(dip_file_name,"w+")) == NULL ) return -1;
  //send header line
  fprintf(fstream,"Switch;ON_or_OFF;comment (%s) mame-ROM:%s.zip\n",lisy35_game.long_name, lisy35_game.gamename);
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
//mode == 0 -> set /lisy to rw; open file and prepare header line
//mode == 1 -> append line to file
//mode == 2 -> appemnd the line, close file; set /lisy to ro
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
unsigned char lisy35_file_get_onedip( int dip_nr, char *dip_comment, char *dip_setting_filename, int re_init )
{

 int no,i,j,myswitch;
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

  //construct the filename; using global var lisy35_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY35_DIPS_PATH,lisy35_game.gamenr,LISY35_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");

  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY35_DIPS_PATH,LISY35_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  //we read dips on LISY35 board
  if(fstream == NULL)
      {
	for( i=0; i<=3; i++)
	{
	 //read the whole switch
	 myswitch = lisy35_file_get_mpudips( i, 0, dip_file_name );
	   // the get dip on switch
	   for( j=0; j<=7; j++)
	   {
	     value[i*8 + j] = CHECK_BIT( myswitch, j);
             strcpy( comment[i*8 + j]," ---- comment ---- ");
	   }
 	    //copy filename where we was successfull to give back to calling routine
 	    strcpy( dip_setting_filename, dip_file_name);
        }
      }
  else
  {
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
  } //if fstream!=NULL
 } //done only first_time

 //give back stored values (from first time)
 strcpy(dip_comment,comment[dip_nr-1]);
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 return value[dip_nr-1];
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
int lisy35_file_get_mpudips( int switch_nr, int debug, char *dip_setting_filename )
{

 typedef union dips {
    unsigned char byte;
    struct {
    unsigned DIP1:1, DIP2:1, DIP3:1, DIP4:1, DIP5:1, DIP6:1, DIP7:1, DIP8:1;
    //unsigned b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    }t_dips;


 static t_dips lisy35_dip[4];
 static int first_time = 1;
 static char dip_file_name[80];
 static char first_time_debug[4] = { 1,1,1,1 };

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
  for (i=0; i<=3; i++) lisy35_dip[i].byte = -1;

  //construct the filename; using global var lisy80_gamenr
  sprintf(dip_file_name,"%s%03d%s",LISY35_DIPS_PATH,lisy35_game.gamenr,LISY35_DIPS_FILE);
  //copy filename where we was successfull to give back to calling routine
  strcpy( dip_setting_filename, dip_file_name);

  //try to read the file with game nr
  fstream = fopen(dip_file_name,"r");


  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(dip_file_name,"%sdefault%s",LISY35_DIPS_PATH,LISY35_DIPS_FILE);
    //copy filename where we was successfull to give back to calling routine
    strcpy( dip_setting_filename, dip_file_name);
    fstream = fopen(dip_file_name,"r");
   }//second try

  //check if first or second try where successfull
  //if it is still NULL both tries where not successfull
  //so we give back settings from dip switches on LISY board
  if(fstream == NULL)
      {
        //we re read the dips as they could have changed without powering of the pi
        lisy35_coil_read_mpu_dips();
        //now assign dips to internal vars
        //return setting, (only once for first time call)
        for(i=0; i<=3; i++) lisy35_dip[i].byte = lisy35_coil_get_mpu_dip(i);
 	//copy filename where we was successfull to give back to calling routine
        sprintf(dip_file_name,"dip setting from LISY board used: %d %d %d %d",lisy35_dip[0].byte,lisy35_dip[1].byte,lisy35_dip[2].byte,lisy35_dip[3].byte);
 	strcpy( dip_setting_filename, dip_file_name);
        return lisy35_dip[switch_nr].byte;
        }
   //at this point we have a valid file descriptor
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
        lisy35_dip[i].bitv.DIP1 =  dip[i*8 ];
        lisy35_dip[i].bitv.DIP2 =  dip[i*8 + 1];
        lisy35_dip[i].bitv.DIP3 =  dip[i*8 + 2];
        lisy35_dip[i].bitv.DIP4 =  dip[i*8 + 3];
        lisy35_dip[i].bitv.DIP5 =  dip[i*8 + 4];
        lisy35_dip[i].bitv.DIP6 =  dip[i*8 + 5];
        lisy35_dip[i].bitv.DIP7 =  dip[i*8 + 6];
        lisy35_dip[i].bitv.DIP8 =  dip[i*8 + 7];
    }

   fclose(fstream);

 } //done only first_time

 //debug?
 if(debug) 
  {
   if( first_time_debug[switch_nr] ) //only one time debug as bally tend to do that too often
    {
    fprintf(stderr,"mpudips return: ");
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP1);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP2);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP3);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP4);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP5);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP6);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP7);
    fprintf(stderr,"%d", lisy35_dip[switch_nr].bitv.DIP8);
    fprintf(stderr," for switch:%d\n\r",switch_nr);
    }
   first_time_debug[switch_nr] = 0;
   }
 
 //copy filename where we was successfull to give back to calling routine
 strcpy( dip_setting_filename, dip_file_name);

 //return setting, this will be -1 if 'first_time' failed
 return lisy35_dip[switch_nr].byte;

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


//read the csv file on /lisy partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisy80_games_csv
int  lisy80_file_get_gamename(t_stru_lisy80_games_csv *lisy80_game)
{

 char buffer[1024];
 char games_csv[80];
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

//do we want to read the special 7digit definition file?
if ( ls80opt.bitv.sevendigit) strcpy(games_csv,LISY80_GAMES_7DIGIT_CSV);
  else strcpy(games_csv,LISY80_GAMES_CSV);


 FILE *fstream = fopen(games_csv,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",games_csv);
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

//read the csv file on /lisy partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisy1_games_csv
int  lisy1_file_get_gamename(t_stru_lisy1_games_csv *lisy1_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;
 float clockscale_int;  //integer clockscale from cfg file, will be devided by 1000;

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
     	  clockscale_int = atoi(strtok(NULL, ";"));	        //clockscale value per Gottlieb game
     	  lisy1_game->clockscale = clockscale_int / 1000; 	//clockscale is a float
     	  strcpy(lisy1_game->comment,strtok(NULL, ";"));	//comment if available
          break;
	}
   } //while
   fclose(fstream);

   //for LISY1 we need a special marker for games which are using lamps as coils
   lisy1_game->lamp17_is_Coil = lisy1_game->lamp18_is_Coil = 0;

   if ( lisy1_game->rom_id[0] == 'C' ) //joker Poker
    {
      fprintf(stderr,"Info: Joker Poker special: Lamp17 is coil\n"); lisy1_game->lamp17_is_Coil = 1;
    }
   if ( lisy1_game->rom_id[0] == 'F' ) //Countdown
    {
      fprintf(stderr,"Info: Countdown special: Lamp17 is coil\n"); lisy1_game->lamp17_is_Coil = 1;
      fprintf(stderr,"Info: Countdown special: Lamp18 is coil\n"); lisy1_game->lamp18_is_Coil = 1;
    }
   if ( lisy1_game->rom_id[0] == 'I' ) //Pinball Pool
    {
      fprintf(stderr,"Info: Pinball Pool special: Lamp17 is coil\n"); lisy1_game->lamp17_is_Coil = 1;
    }
   if ( lisy1_game->rom_id[0] == 'N' ) //Buck Rogers
    {
      fprintf(stderr,"Info: Buck Rogers special: Lamp17 is coil\n"); lisy1_game->lamp17_is_Coil = 1;
    }

  //give back the name and the number of the game
  return(line_no);
}

//read the csv file on /lisy partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisy35_games_csv
int  lisy35_file_get_gamename(t_stru_lisy35_games_csv *lisy35_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;

 //get value of dipswitch
 dip_switch_val = display_get_dipsw_value();
 //set also global gamenr var to dip switch value, needed for dip switch settings
 lisy35_game->gamenr = dip_switch_val;

 FILE *fstream = fopen(LISY35_GAMES_CSV,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY35: opening %s failed ",LISY35_GAMES_CSV);
      return -1 ;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     line_no = atoi(strtok(line, ";")); 	//line number
     if ( dip_switch_val == line_no)
        { 
     	  strcpy(lisy35_game->gamename,strtok(NULL, ";"));	//game name short version
     	  strcpy(lisy35_game->long_name,strtok(NULL, ";"));	//game long name
     	  lisy35_game->throttle = atoi(strtok(NULL, ";"));	//throttle value per Bally game
     	  lisy35_game->special_cfg = atoi(strtok(NULL, ";"));	 // is there a special config? (some games )
     	  lisy35_game->aux_lamp_variant = atoi(strtok(NULL, ";"));//  what AUX lampdriverboard variant the game has
     	  //lisy35_game->soundboard_variant = atoi(strtok(NULL, ";"));// NEW: get it from core pinmame (what soundboard variant the game has)
     	  strcpy(lisy35_game->comment,strtok(NULL, ";"));	//comment if available
          break;
	}
   } //while
   fclose(fstream);

  //give back the name and the number of the game
  return(line_no);
}

//read the csv file for sound opts on /lisy partition
//give -1 in case we had an error
//fill structure stru_lisy80_sound_csv
int  lisy80_file_get_soundopts(void)
{

 int i;
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

   //init values of sound structure
   for(i=0; i<=63; i++)
    {
	lisy80_sound_stru[i].volume = 0;
	lisy80_sound_stru[i].loop = 0;
	lisy80_sound_stru[i].not_int_loops = 0;
     }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     sound_no = atoi(strtok(line, ";")); 	//sound number
     //sanity check for soundnumber ( max = 63)
     if (sound_no < 64)
	{
     	 lisy80_sound_stru[sound_no].volume = atoi(strtok(NULL, ";"));	
     	 lisy80_sound_stru[sound_no].loop = atoi(strtok(NULL, ";"));	
     	 lisy80_sound_stru[sound_no].not_int_loops = atoi(strtok(NULL, ";"));	
	}
     else fprintf(stderr,"soundnumber is too big:%d in lisy80_sound_stru\n",sound_no);
   } //while
   fclose(fstream);

  return 0;
}

//read the csv file for sound opts on /lisy partition
//give -1 in case we had an error
//fill structure stru_lisy35_sound_csv
int  lisy35_file_get_soundopts(void)
{

 char buffer[1024];
 char *line;
 char *str;
 char sound_file_name[80];
 int sound_no,i;
 int first_line = 1;
 FILE *fstream;

 //construct the filename; using global var lisy35_gamenr
 sprintf(sound_file_name,"%s%03d%s",LISY35_SOUND_PATH,lisy35_game.gamenr,LISY35_SOUND_FILE);

 fstream = fopen(sound_file_name,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY35: opening %s failed ",sound_file_name);
      return -1;
   }

 //init soundnumber to 0
 for ( i=0; i<=255; i++) lisy35_sound_stru[i].soundnumber = 0;
   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     str = strdup(strtok(line, ";"));   //sound number in hex
     sound_no = strtol(str, NULL, 16); // to be converted
     //sound_no = atoi(strtok(line, ";")); 	//sound number
     lisy35_sound_stru[sound_no].soundnumber = sound_no;   // != 0 if mapped
     lisy35_sound_stru[sound_no].path = strdup(strtok(NULL, ";"));	//path to soundfile
     lisy35_sound_stru[sound_no].name = strdup(strtok(NULL, ";"));	//name of soundfile
     lisy35_sound_stru[sound_no].option = atoi(strtok(NULL, ";"));	//option
     lisy35_sound_stru[sound_no].comment = strdup(strtok(NULL, ";"));	//comment
   } //while
   fclose(fstream);

  return 0;
}

//read the csv file for coil opts on /lisy partition for lisy1
//give -1 in case we had an error
//fill structure stru_lisy1_coil_csv
int  lisy1_file_get_coilopts(void)
{

 char buffer[1024];
 char *line;
 char coil_file_name[80];
 int coil_no;
 int first_line = 1;
 FILE *fstream;
 int gtb_pulse[9];
 int i;

 //set lisy1 defaults which is 150 msec for coils
 for ( i=0; i<=7; i++) lisy1_coil_min_pulse_time[i] = 150;

 //construct the filename; using global var lisy1_gamenr
 sprintf(coil_file_name,"%s%03d%s",LISY1_COIL_PATH,lisy1_game.gamenr,LISY1_COIL_FILE);

 //try to read the file with game nr
 fstream = fopen(coil_file_name,"r");

  //second try: to read the file with default
  if(fstream == NULL)
  {
    //construct the new filename; using 'default'
    sprintf(coil_file_name,"%sdefault%s",LISY1_COIL_PATH,LISY1_COIL_FILE);
    fstream = fopen(coil_file_name,"r");
   }//second try

   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY1: opening %s failed ",coil_file_name);
      return -1;
   }

   //config file will hav coil numbering 1..8
   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     coil_no = atoi(strtok(line, ";")); 	//coil number
     //range check
     if ( coil_no <= 8)
        gtb_pulse[coil_no] = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);

  //change position of OUTHOLE as OUTHOLE is SOL1 internal
  //so that numbering of coila correspondent with Gottlieb numbering
  //in order not to need change PIC SW, historical reason
  lisy1_coil_min_pulse_time[0] = gtb_pulse[2]; //knock
  lisy1_coil_min_pulse_time[1] = gtb_pulse[3]; //tens
  lisy1_coil_min_pulse_time[2] = gtb_pulse[4]; //hund
  lisy1_coil_min_pulse_time[3] = gtb_pulse[5]; //tous
  lisy1_coil_min_pulse_time[4] = gtb_pulse[1]; //OUTHOLE
  lisy1_coil_min_pulse_time[5] = gtb_pulse[6]; //SOL6
  lisy1_coil_min_pulse_time[6] = gtb_pulse[7]; //SOL7
  lisy1_coil_min_pulse_time[7] = gtb_pulse[8]; //SOL8


  return 0;
}

//read the csv file for coil opts on /lisy partition for lisy80
//give -1 in case we had an error
//fill structure stru_lisy80_coil_csv
int  lisy80_file_get_coilopts(void)
{

 char buffer[1024];
 char *line;
 char coil_file_name[80];
 int coil_no;
 int first_line = 1;
 FILE *fstream;

 //construct the filename; using global var lisy80_gamenr
 sprintf(coil_file_name,"%s%03d%s",LISY80_COIL_PATH,lisy80_game.gamenr,LISY80_COIL_FILE);

 fstream = fopen(coil_file_name,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",coil_file_name);
      return -1;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     coil_no = atoi(strtok(line, ";")); 	//coil number
     //range check
     if ( coil_no <= 9)
        lisy80_coil_min_pulse_time[coil_no] = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);

  return 0;
}

//read the csv file for fadecandy led mapping /lisy partition
// system is either 1/35/80 according to system1 35 80
//give -1 in case we had an error
//fill structure and set internal pixel for GI ( RTH: will be extended )
int  lisy_file_get_led_mappings(unsigned char system)
{
 char buffer[1024];
 char *line;
 char fadecandy_file_name[80];
 int lamp_no;
 int led_no;
 int lamp_start, lamp_end;
 int first_line = 1;
 FILE *fstream;
 int i,dum;

 extern pixel lisy_fc_leds[512]; //from fadecandy.c

//construct the filename; using global var lisy_gamenr
//we start with GI basic mapping
//LED;Mode;follower;Red;Green;Blue;
if ( system == 1) sprintf(fadecandy_file_name,"%s%03d%s",LISY1_FADECANDY_PATH,lisy1_game.gamenr,LISY1_FADECANDY_GI_FILE);
else if ( system == 80) sprintf(fadecandy_file_name,"%s%03d%s",LISY80_FADECANDY_PATH,lisy80_game.gamenr,LISY80_FADECANDY_GI_FILE);
else if ( system == 35) sprintf(fadecandy_file_name,"%s%03d%s",LISY35_FADECANDY_PATH,lisy35_game.gamenr,LISY35_FADECANDY_GI_FILE);
else sprintf(fadecandy_file_name,"not there");

 fstream = fopen(fadecandy_file_name,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",fadecandy_file_name);
      return -1;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     led_no = atoi(strtok(line, ";")); 	//LED number
     if ( led_no > 511 ) continue; //skip line if LED number is out of range
     dum = atoi(strtok(NULL, ";")); 	//RTH: reserved for future use Mode e.g. blinking
     dum = atoi(strtok(NULL, ";")); 	//RTH: reserved for future use follower e.g Tilt
     lisy_fc_leds[led_no].r = atoi(strtok(NULL, ";"));	//direct set in fc_led array
     lisy_fc_leds[led_no].g = atoi(strtok(NULL, ";"));	
     lisy_fc_leds[led_no].b = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);

//now map the lamps to the LEDs
//0 means not mapped, we start counting lamps with 0 here
//init with no mapping and default values exclusive & white
//if ( system == 1)  { lamp_start=1; lamp_end=36; }
//if ( system == 80)  { lamp_start=0; lamp_end=47; }
//we init all lamps, independent of the system
//for system80 lamps 48-51 are fixed mapped reverse to lamps 44-47
lamp_start=0; lamp_end=51; 

for(i=lamp_start; i<=lamp_end;i++) 
  { 
     lisy_lamp_to_led_map[i].is_mapped = 0;
     lisy_lamp_to_led_map[i].mapled = i;
     lisy_lamp_to_led_map[i].exclusive = 1; //means we do NOT acivate the lamp driver in paralell
     lisy_lamp_to_led_map[i].r = 255;
     lisy_lamp_to_led_map[i].g = 255;
     lisy_lamp_to_led_map[i].b = 255;
  }


 //construct the filename; using global var lisy_gamenr
if ( system == 1) sprintf(fadecandy_file_name,"%s%03d%s",LISY1_FADECANDY_PATH,lisy1_game.gamenr,LISY1_FADECANDY_LAMP_FILE);
else if ( system == 80) sprintf(fadecandy_file_name,"%s%03d%s",LISY80_FADECANDY_PATH,lisy80_game.gamenr,LISY80_FADECANDY_LAMP_FILE);
else if ( system == 35) sprintf(fadecandy_file_name,"%s%03d%s",LISY35_FADECANDY_PATH,lisy35_game.gamenr,LISY35_FADECANDY_LAMP_FILE);
else sprintf(fadecandy_file_name,"not there");

 fstream = fopen(fadecandy_file_name,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISY80: opening %s failed ",fadecandy_file_name);
      return -1;
   }

   first_line = 1;
   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     lamp_no = atoi(strtok(line, ";")); 	//lamp number
     if ( lamp_no > 51 ) continue; //skip line if lamp number is out of range
     lisy_lamp_to_led_map[lamp_no].exclusive = atoi(strtok(NULL, ";"));	
     lisy_lamp_to_led_map[lamp_no].mapled = atoi(strtok(NULL, ";"));	
     lisy_lamp_to_led_map[lamp_no].r = atoi(strtok(NULL, ";"));	
     lisy_lamp_to_led_map[lamp_no].g = atoi(strtok(NULL, ";"));	
     lisy_lamp_to_led_map[lamp_no].b = atoi(strtok(NULL, ";"));	
     lisy_lamp_to_led_map[lamp_no].is_mapped = 1;
   } //while
   fclose(fstream);

   if ( ls80dbg.bitv.lamps )
     {
	fprintf(stderr,"active Fadecandy lamp - LED mapping: \n");
	for(i=lamp_start; i<=lamp_end;i++) 
  	{ 
          if ( lisy_lamp_to_led_map[i].is_mapped )
	   {
		fprintf(stderr,"Lamp %d mapped ",i);
		fprintf(stderr,"to LED %d ",lisy_lamp_to_led_map[i].mapled);
		fprintf(stderr,"with RGB %d ",lisy_lamp_to_led_map[i].r);
		fprintf(stderr," %d ",lisy_lamp_to_led_map[i].g);
		fprintf(stderr," %d \n",lisy_lamp_to_led_map[i].b);
	   }
	}
      }

 return 0;
}

//read the csv file on /lisy partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisymini_games_csv
int  lisymini_file_get_gamename(t_stru_lisymini_games_csv *lisymini_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;
 unsigned char found = 0;

 //get value of dipswitch
 dip_switch_val = lisymini_get_dip("S2");
 //set also global gamenr var to dip switch value, needed for dip switch settings
 lisymini_game->gamenr = dip_switch_val;

 FILE *fstream = fopen(LISYMINI_GAMES_CSV,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISYMINI: opening %s failed ",LISYMINI_GAMES_CSV);
      return -1 ;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     line_no = atoi(strtok(line, ";")); 	//line number
     if ( dip_switch_val == line_no)
        { 
     	  strcpy(lisymini_game->gamename,strtok(NULL, ";"));	//game name short version
     	  strcpy(lisymini_game->long_name,strtok(NULL, ";"));	//game long name
     	  strcpy(lisymini_game->type,strtok(NULL, ";"));	//game type
     	  lisymini_game->throttle = atoi(strtok(NULL, ";"));	//throttle value per Bally game
     	  strcpy(lisymini_game->comment,strtok(NULL, ";"));	//comment if available
	  found = 1; //found it
          break;
	}
   } //while
   fclose(fstream);

  //give back the name and the number of the game
  if (found) return(line_no); else return(-1);
}

//read the csv file for lisy Home lamp & coil mapping /lisy partition
//give -1 in case we had an error
//fill structure 
int  lisy_file_get_home_mappings(void)
{
 char buffer[1024];
 char *line;
 char file_name[80];
 int no;
 int is_coil;
 int first_line = 1;
 FILE *fstream;
 int i,dum;


//map to default 1:1
for(i=0; i<=48;i++) 
  { 
     lisy_home_lamp_map[i].mapped_to_no = i;
     lisy_home_lamp_map[i].mapped_is_coil = 0;
     lisy_home_lamp_map[i].r = 255;
     lisy_home_lamp_map[i].g = 255;
     lisy_home_lamp_map[i].b = 255;
   }
for(i=0; i<=9;i++) 
  { 
     lisy_home_coil_map[i].mapped_to_no = i;
     lisy_home_coil_map[i].mapped_is_coil = 1;
  }

//LAMPS construct the filename
//Lamp ;LED=0|COIL=1;Number;Red;Green;Blue;Comment
sprintf(file_name,"%s%s",LISYH_MAPPING_PATH,LISYH_LAMP_MAPPING_FILE);

 fstream = fopen(file_name,"r");
  if(fstream == NULL)
  {
      fprintf(stderr,"LISY_Home: opening %s failed, using defaults for lamps\n",file_name);
  }
  else
  {
   first_line = 1;
   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     no = atoi(strtok(line, ";")); 	//lamp number
     if ( no > 48 ) continue; //skip line if lamp number is out of range
     lisy_home_lamp_map[no].mapped_is_coil = atoi(strtok(NULL, ";")); 	//lamp or coil
     lisy_home_lamp_map[no].mapped_to_no = atoi(strtok(NULL, ";"));	
     lisy_home_lamp_map[no].r = atoi(strtok(NULL, ";"));	
     lisy_home_lamp_map[no].g = atoi(strtok(NULL, ";"));	
     lisy_home_lamp_map[no].b = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);
  }

//COILS construct the filename
//Coil;LED=0|COIL=1;Number;Comment
sprintf(file_name,"%s%s",LISYH_MAPPING_PATH,LISYH_COIL_MAPPING_FILE);

 fstream = fopen(file_name,"r");
  if(fstream == NULL)
  {
      fprintf(stderr,"LISY_Home: opening %s failed, using defaults for coils\n",file_name);
  }
  else
  {
   first_line = 1;
   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     no = atoi(strtok(line, ";")); 	//lamp number
     if ( no > 9 ) continue; //skip line if lamp number is out of range
     lisy_home_coil_map[no].mapped_is_coil = atoi(strtok(NULL, ";")); 	//lamp or coil
     lisy_home_coil_map[no].mapped_to_no = atoi(strtok(NULL, ";"));	
   } //while
   fclose(fstream);
  }

/*
fprintf(stderr,"active LISY HOME mapping: \n");
for(i=0; i<=47;i++)
  {
 printf("  map lamp number:%d TO %s number:%d\n",i,lisy_home_lamp_map[i].mapped_is_coil ? "coil" : "lamp", lisy_home_lamp_map[i].mapped_to_no);
   }
for(i=1; i<=9;i++)
  {
 printf("  map coil number:%d TO %s number:%d\n",i,lisy_home_coil_map[i].mapped_is_coil ? "coil" : "lamp", lisy_home_coil_map[i].mapped_to_no);
  }
*/

 return 0;
}

//read the text file on /lisy partition
//give back string for welcome message
//fill structure stru_lisymini_games_csv
int  lisy_file_get_welcome_msg(char *message)
{

 FILE *fstream = fopen(LISY_WELCOME_MSG_FILE,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISYI: opening %s failed ",LISY_WELCOME_MSG_FILE);
      return -1 ;
   }

   if ( fgets(message,200,fstream)!=NULL)
	return 0;
   else
	return -2;
   
fclose(fstream);

}

//read the csv file on /lisy partition and the DIP switch setting
//give back line number; -1 in case we had an error
//fill structure stru_lisymini_games_csv
int  lisyapc_file_get_gamename(t_stru_lisymini_games_csv *lisymini_game)
{

 char buffer[1024];
 char *line;
 unsigned char dip_switch_val;
 int line_no;
 int first_line = 1;
 unsigned char found = 0;

 //get value of dipswitch
 dip_switch_val = lisyapc_get_dip("S2");
 //set also global gamenr var to dip switch value, needed for dip switch settings
 lisymini_game->gamenr = dip_switch_val;

 FILE *fstream = fopen(LISYMINI_GAMES_CSV,"r");
   if(fstream == NULL)
   {
      fprintf(stderr,"\n LISYMINI: opening %s failed ",LISYMINI_GAMES_CSV);
      return -1 ;
   }

   while( (line=fgets(buffer,sizeof(buffer),fstream))!=NULL)
   {
     if (first_line) { first_line=0; continue; } //skip first line (Header)
     line_no = atoi(strtok(line, ";")); 	//line number
     if ( dip_switch_val == line_no)
        {
     	  strcpy(lisymini_game->gamename,strtok(NULL, ";"));	//game name short version
     	  strcpy(lisymini_game->long_name,strtok(NULL, ";"));	//game long name
     	  strcpy(lisymini_game->type,strtok(NULL, ";"));	//game type
     	  lisymini_game->throttle = atoi(strtok(NULL, ";"));	//throttle value per Bally game
     	  strcpy(lisymini_game->comment,strtok(NULL, ";"));	//comment if available
     	  found = 1; //found it
          break;
        }
   } //while
   fclose(fstream);

  //give back the name and the number of the game
  if (found) return(line_no); else return(-1);
}
