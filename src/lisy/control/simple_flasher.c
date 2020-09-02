 /*
  bontango 07.2019
  lisy simple flasher, all plattforms
*/
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>
#include "../utils.h"

#define VERSION "1.2"

#define TESTSWITCH 22
#define DIP2 12
#define DIP7 23
#define DIP8 21
#define LED1 4 //green LED
#define LED2 5 //yellow LED
#define LED3 14 //red LED
#define GREEN_LED 1
#define YELLOW_LED 2
#define RED_LED 3
//the variants
#define LISY1 1
#define LISY80 2
#define LISY35 3
#define LISY80_LED 4
//environment
#define PIC_DIR "/boot/lisy/picpgm/"
#define ONE_SHOT_DIR "/boot/lisy/picpgm/one_shot/"

//global vars
FILE *f_serial;
#define SERIAL_DEVICE "/dev/serial0"
unsigned char serialexist = 0;
FILE *f_usbserial;


//log the message to the console
void log_message(char *str)
{

 if (serialexist) fprintf(f_serial,"%s\n",str);
   else printf("%s\n",str);

}


//simple debounce and 'long hold' routine
//returns:
//0 - switch not pressed or bounce detected
#define PIN_NOT_PRESSED 0
//1 - switch pressed short
#define PIN_PRESSED_SHORT 1
//2 - switch pressed long period
#define PIN_PRESSED_LONG 2
int get_switch_status( int pin)
{

int counts;

//switch closed?
if (digitalRead(pin)) return PIN_NOT_PRESSED; //we have a pull up resistor, so status!=0 means switch open

//switch is closed, lets do a debounce
delay(10);  //wait 10 milliseconds
if (digitalRead(pin)) return PIN_NOT_PRESSED; //closed to short

//OK, switch is closed, lets wait for open again or until a 'long press' is detected
counts = 0;
do {
 delay(1); //wait a msec
 if ( digitalRead(pin) ) //open again
   return(PIN_PRESSED_SHORT);
} while ( counts++ < 800);

   //we passed the loop
   return(PIN_PRESSED_LONG);
}

//control all leds
//0 -> OFF
//1 -> ON
//2 -> toggle
//will set to 0 with first call
void ctrl_leds(unsigned char cmd)
{
 static unsigned char status = 1;

switch(cmd)
{
  case 0:
   digitalWrite ( LED1, 0);
   digitalWrite ( LED2, 0);
   digitalWrite ( LED3, 0);
  break;
  case 1:
   digitalWrite ( LED1, 1);
   digitalWrite ( LED2, 1);
   digitalWrite ( LED3, 1);
  break;
  case 2:
 if(status)
 {
   digitalWrite ( LED1, 0);
   digitalWrite ( LED2, 0);
   digitalWrite ( LED3, 0);
   status = 0;
 }
 else
 {
   digitalWrite ( LED1, 1);
   digitalWrite ( LED2, 1);
   digitalWrite ( LED3, 1);
   status = 1;
 }
 break;
}//switch
}

//set all leds
//binary code
void set_leds(unsigned char value)
{
 if( CHECK_BIT(value,0)) digitalWrite ( LED1, 1); else digitalWrite ( LED1, 0);
 if( CHECK_BIT(value,1)) digitalWrite ( LED2, 1); else digitalWrite ( LED2, 0);
 if( CHECK_BIT(value,2)) digitalWrite ( LED3, 1); else digitalWrite ( LED3, 0);
}

//set one led
void set_led(unsigned char led)
{
  ctrl_leds(0);

  switch(led)
  {
   case 1: digitalWrite ( LED1, 1); break;
   case 2: digitalWrite ( LED2, 1); break;
   case 3: digitalWrite ( LED3, 1); break;
  }

}

//flash the green led
void flash_green_led(void)
{
  ctrl_leds(0);
  do
  {
  digitalWrite ( LED1, 1);
  delay(150);
  digitalWrite ( LED1, 0);
  delay(150);
  } while((get_switch_status(TESTSWITCH)) == PIN_NOT_PRESSED);
  delay(150);
}

//flash the red led
void flash_red_led(void)
{
  ctrl_leds(0);
  do
  {
  digitalWrite ( LED3, 1);
  delay(150);
  digitalWrite ( LED3, 0);
  delay(150);
  } while((get_switch_status(TESTSWITCH)) == PIN_NOT_PRESSED);
  delay(150);
}

//do the programming with picpgm
int do_picpgm(int lisy_variant, int selected_PIC)
{
    
  DIR *d;
  struct dirent *dir;
  char message[255];
  char system_str[255];
  char file_to_prg[255];
  char str_variant[80];
  char str_PIC[80];
  char str_search[80];
  char str_found[80];
  int found = 0;
  int ret_val = 0;


  //construct the filename
switch (lisy_variant)
{
  case LISY1:
     strcpy(str_variant,"LISY1_");
  break;
  case LISY35:
     strcpy(str_variant,"LISY35_");
  break;
  case LISY80:
     strcpy(str_variant,"LISY80_");
  break;
  case LISY80_LED:
     strcpy(str_variant,"LISY80_LED_");
  break;
}
switch (selected_PIC)
{
  case 1:
     strcpy(str_PIC,"Displays");
  break;
  case 2:
     strcpy(str_PIC,"Sol_Lamps");
  break;
  case 3:
     strcpy(str_PIC,"Switches");
  break;
}

 sprintf(str_search,"%s%s_",str_variant,str_PIC);

  d = opendir(PIC_DIR);

  if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
	    if ( strncmp(dir->d_name,str_search,strlen(str_search)) == 0)
	     {
		strcpy(str_found,  dir->d_name);
		found = 1;
	     }
        }
        closedir(d);
    }
  else
   {
    log_message("problem open directory\n");
    return 1;
   }

 if(found)
  {
     sprintf(message,"hex file name is %s",str_found);
     log_message(message);
  }
 else
  {
     sprintf(message,"hex file %s could not be found",str_search);
     log_message(message);
     return 1;
  }

 sprintf(message,"we now program %s PIC",str_PIC);
 log_message(message);
 log_message("please stand by");

 //construct filename with path for picpgm
 sprintf(file_to_prg,"%s%s",PIC_DIR,str_found);

 sprintf(system_str,"/usr/bin/picpgm -p %s >/tmp/picpgm_output",file_to_prg);
 ret_val = system(system_str);
 sprintf(message,"returnvalue of picpgm was:%d\n",ret_val);
 log_message(message);

 return ret_val;

}

//init switches and leds
void init_gpios(void)
{
pinMode ( TESTSWITCH, INPUT);
pinMode ( DIP2, INPUT);
pinMode ( DIP7, INPUT);
pinMode ( DIP8, INPUT);
pullUpDnControl (TESTSWITCH, PUD_UP);
pullUpDnControl (DIP2, PUD_UP);
pullUpDnControl (DIP7, PUD_UP);
pullUpDnControl (DIP8, PUD_UP);
//init LEDs
pinMode ( LED1, OUTPUT);
pinMode ( LED2, OUTPUT);
pinMode ( LED3, OUTPUT);
set_leds(0);
//set MCLR port of switchpic (0)  to input to not interfere with picpgm port
pinMode ( 0, INPUT);
}

//look if one_shot-file exists
//if no, give back zero
//if yes, give back number of files found  and filename of last file found
int one_shot_file_exists(char *one_shot_file)
{
  DIR *d;
  struct dirent *dir;
  char str[255];
  int i = 0;
  int no_of_files = 0;


  d = opendir(PIC_DIR);

  //read the whole dir
  d = opendir(ONE_SHOT_DIR);

  if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
             {
                //check for .hex extension
                if (strlen(dir->d_name) >= 5)
                  {
                    strcpy(str,dir->d_name);
                    printf("check: %s\n",str);
                    if (strncmp( &str[strlen(str)-4],".hex",4) == 0)
                        {
                         strcpy(one_shot_file,dir->d_name);
                         printf("match: %s\n",one_shot_file);
                         ++i;
                        }//match
                  } //strlen >= 5
             }
        }
        no_of_files = i;
        closedir(d);
     }

 return(no_of_files);
}

int main (int argc, char *argv[])
{

int i, ret, status, counts;
//default variant LISY35
int lisy_variant = LISY35;
int selected_PIC;
char message[255];
char one_shot_file[255];



// wiringPi library
   wiringPiSetup();

init_gpios();

//check if serial devices exist
if ( ( f_serial = fopen(SERIAL_DEVICE,"r+")) != NULL ) serialexist = 1;
  else fprintf(stderr,"problem open serial console\n");

sprintf(message,"simple_flasher Version %s started",VERSION);
log_message(message);
log_message("press switch S3 to continue");


//lets blink all LEDs to show that we are ready
counts = 0;
do {

	delay(1);
	if (counts++ > 1000) //one sec passed
	{
	 ctrl_leds(2);
	 counts = 0;
	}
} while ( ( status = get_switch_status(TESTSWITCH) ) == PIN_NOT_PRESSED );

//switch pressed (long or short)

//look if we have a '.hex' file in ONE_SHOT_DIR
//if yes, start programming right away
if (one_shot_file_exists(one_shot_file))
{
 char file_to_prg[255];
 int ret_val;
 char system_str[255];

 sprintf(message,"hex file name is %s",one_shot_file);
 log_message(message);
 log_message("start programming right away");
 //construct filename with path for picpgm
 sprintf(file_to_prg,"%s%s",ONE_SHOT_DIR,one_shot_file);
do
{
 sprintf(system_str,"/usr/bin/picpgm -p %s >/tmp/picpgm_output",file_to_prg);
 ret_val = system(system_str);
 sprintf(message,"returnvalue of picpgm was:%d\n",ret_val);
 log_message(message);
    if (ret_val)
     {
       init_gpios();
       log_message("programming PIC failed\n");
       flash_red_led();
     }
    else
     {
       init_gpios();
       log_message("programming PIC successful\n");
       flash_green_led();
     }

   //wait for testswitch press for next try
   while((get_switch_status(TESTSWITCH)) == PIN_NOT_PRESSED) ;
  } while(1); //endless loop
}
else log_message("INFO: no one shot file found");


//set LEDs to 0
ctrl_leds(0);

log_message("select the LISY variant with S3");
log_message("push and hold switch S3 for one second to confirm selection");


//variant setting
//default is LISY35
lisy_variant = LISY35;
set_leds(lisy_variant);
log_message("LISY35 selected");
do
{
  if ( ( status = get_switch_status(TESTSWITCH)) == PIN_PRESSED_SHORT  )
  {
    lisy_variant++;
    if( lisy_variant >4) lisy_variant = 1;
    set_leds(lisy_variant);
  switch (lisy_variant)
  {
  case LISY1:
     log_message("LISY1 selected");
  break;
  case LISY35:
     log_message("LISY35 selected");
  break;
  case LISY80:
     log_message("LISY80 selected");
  break;
  case LISY80_LED:
     log_message("LISY80_LED selected");
  break;
  }

  }
}while ( status != PIN_PRESSED_LONG);

//blink fast to indicate setting variant made
for (i=0; i<3; i++)
{
set_leds(0);
delay(150);
set_leds(lisy_variant);
delay(150);
}

switch (lisy_variant)
{
  case LISY1: 
     log_message("LISY1 selection confirmed");
  break;
  case LISY35: 
     log_message("LISY35 selection confirmed");
  break;
  case LISY80: 
     log_message("LISY80 selection confirmed");
  break;
  case LISY80_LED: 
     log_message("LISY80_LED selection confirmed");
  break;
}

log_message("now select the PIC you want to program with S3");
log_message("push and hold switch S3 for one second to confirm selection and start programming");
log_message("do not forget to set the four jumpers accordantly");

//now set PIC ( 1,2,3) and do the programming
selected_PIC = 1;
set_led(selected_PIC);
log_message("display PIC selected");
do
{
  switch ( get_switch_status(TESTSWITCH))
  {
   case PIN_PRESSED_SHORT:
    selected_PIC++;
    if( selected_PIC >3) selected_PIC = 1;
    set_led(selected_PIC);
	switch (selected_PIC)
	{
  	case 1:
	  log_message("display PIC selected");
  	break;
  	case 2:
	  log_message("solenoid and lamp PIC selected");
  	break;
  	case 3:
	  log_message("switch PIC selected");
 	 break;
	}
   break;
   case PIN_PRESSED_LONG:
	//blink fast to indicate setting variant made
	for (i=0; i<3; i++)
	{
	set_leds(0);
	delay(150);
	set_led(selected_PIC);
	delay(150);
	}
	switch (selected_PIC)
	{
  	case 1:
	  log_message("display PIC selection confirmed");
  	break;
  	case 2:
	  log_message("solenoid and lamp PIC selection confirmed");
  	break;
  	case 3:
	  log_message("switch PIC selection confirmed");
 	 break;
	}
    log_message("start programming now");

    ret = do_picpgm(lisy_variant, selected_PIC);
    if (ret) 
     {  
       init_gpios();
       log_message("programming PIC failed\n");
       flash_red_led();
     }
    else
     {
       init_gpios();
       log_message("programming PIC successful\n");
       flash_green_led();
     }
    set_led(selected_PIC);
   break;
  }
 }while(1); //endless loop

}//main
