/*
  hw_lib
  part of lisy80NG
  bontango 12.2016
  Version with own I2C routines
*/


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>
//this is for I2C over open&fcntl
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//this is for I2C over wiringPI lib
//#include <wiringPiI2C.h>
#include "utils.h"
#include "displays.h"
#include "switches.h"
#include "coils.h"
#include "fileio.h"
#include "eeprom.h"
#include "hw_lib.h"
#include "externals.h"


//global vars for I2C
int fd_disp_pic;	//file descriptor for display pic
int fd_coil_pic;	//file descriptor for coil pic

//local vars for handling values from switch pic
int k1_from_SWPic, dip_from_SWPic;

//*****
//* functions
//*****


void lisy80_hwlib_wiringPI_init(void)
{
   // wiringPi library
   if ( wiringPiSetup() < 0)
	lisy80_error(1);
}

//get back lisy Hardware revision
// -1 in case of error
int lisy_get_hardware_revision(void)
{

 int file, res;

 //try to open eeprom via i2c in order to detect hardware revision
 // Hardware Version 3.11 has eeprom at I2C address 0x50 & 0x51
 // Hardware Version 3.20 has eeprom at I2C address 0x54 & 0x55
 // LISY1, based on Hardware Version 3.20 has eeprom at I2C address 0x52 & 0x53
 if (( file = open(I2C_DEVICE, O_RDWR)) < 0 )
     lisy80_error(2);

 // probe for slave address at hw revision 320
 ioctl(file, I2C_SLAVE, 0x54);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    fprintf(stderr,"Info: LISY80 Hardware Revision 320\n");
    close(file); //close bus
    return(320);
   }

 // probe for slave address at hw revision 100 (LISY1)
 ioctl(file, I2C_SLAVE, 0x52);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    fprintf(stderr,"Info: LISY1, based on  LISY80 Hardware Revision 320\n");
    close(file); //close bus
    return(100);
   }

 // probe for slave address at hw revision 311
 ioctl(file, I2C_SLAVE, 0x50);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    fprintf(stderr,"Info: LISY80 Hardware Revision 311\n");
    close(file); //close bus
    return(311);
   }

 //no dte3ct so far, so give back error (-1)
 close(file); //close bus
 return(-1);

}



//Hardware INIT LISY80
//also valid for LISY1 as this is based on HW 320
void lisy80_hwlib_init(void)
{

 int dum;

 //look for hardware we ar running on
 lisy_hardware_revision = lisy_get_hardware_revision();
 if ( lisy_hardware_revision < 0)  lisy80_error(11);

 //as the eeprom should be readable at this time
 //print some stats while in debug mode
 if (ls80dbg.bitv.basic == 1 ) lisy80_eeprom_printstats();

 //set GPIOs for the traffic ligth
 if ( lisy_hardware_revision == 311) dum = LISY80_HW311_LED_RED; else  dum = LISY80_HW320_LED_RED;
 pinMode ( dum, OUTPUT); 
 pinMode ( LISY80_LED_YELLOW, OUTPUT); 
 pinMode ( LISY80_LED_GREEN, OUTPUT);
 //set all the leds controlled by the PI
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(1);
 lisy80_set_green_led(0);
 lisy80_set_dp_led(1);

 // open I2C bus and com to switch PIC, assign file descriptors
 // and get SW version of each PIC
 if (ls80dbg.bitv.basic) lisy80_debug("LISY80 Hardware init start");

 //******************
 // PIC 1 - Displays
 //******************
 if ((fd_disp_pic = open( I2C_DEVICE,O_RDWR)) < 0)
        lisy80_error(2);
 // Set the port options and set the address of the device we wish to speak to
 if (ioctl(fd_disp_pic, I2C_SLAVE, DISP_PIC_ADDR) < 0)
        lisy80_error(3);
 //set gpio for LED which shows display PIC activity (no output left on device :-( )
 //HW 311 only
 if ( lisy_hardware_revision == 311)
 {
  pinMode ( LISY80_HW311_LED_DP, OUTPUT);
  digitalWrite (LISY80_HW311_LED_DP, 1);
 }
 //get Software version, TODO: to be checked
 dum = display_get_sw_version();
 if (ls80dbg.bitv.basic)
 {
   sprintf(debugbuf,"Software Display PIC has version %d.%d",dum/100, dum%100);
   lisy80_debug(debugbuf);
 }

 //******************
 // PIC 3 - Coils
 //******************
 if ((fd_coil_pic = open( I2C_DEVICE,O_RDWR)) < 0)
        lisy80_error(4);
 // Set the port options and set the address of the device we wish to speak to
 if (ioctl(fd_coil_pic, I2C_SLAVE, COIL_PIC_ADDR) < 0)
        lisy80_error(5);
 //get Software version, TODO: to be checked
   dum = coil_get_sw_version();
 if (ls80dbg.bitv.basic)
 {
   sprintf(debugbuf,"Software Coil PIC has version %d.%d",dum/100, dum%100);
   lisy80_debug(debugbuf);
 }
 //get value of k3 dip; hardware 320 only 
 //not used yet so debug only
 if ( lisy_hardware_revision != 311)
 {
  if (ls80dbg.bitv.basic)
  {
   dum = coil_get_k3();
   sprintf(debugbuf,"K3 Coil PIC returns %d",dum);
   lisy80_debug(debugbuf);
  }
 }

 //******************
 // PIC 2 - Switches
 //******************
 // - no I2C but 4bit paralell mode 
 //set direction and initial value of Pi GPIO ports
 //
 //communication with Switch PIC
 pinMode ( LISY80_INIT, OUTPUT); digitalWrite (LISY80_INIT, 0);
 pinMode ( LISY80_ACK_FROM_PI, OUTPUT); digitalWrite (LISY80_ACK_FROM_PI, 0);
 pinMode ( LISY80_BUF_READY, INPUT); //the PIC will put that signal on high when switch changed occured
 if ( lisy_hardware_revision == 311)
 {
  pinMode ( LISY80_HW311_DATA_D0, INPUT);
  pinMode ( LISY80_HW311_DATA_D1, INPUT);
  pinMode ( LISY80_HW311_DATA_D2, INPUT);
  pinMode ( LISY80_HW311_DATA_D3, INPUT);
 }
 else
 {
  pinMode ( LISY80_HW320_DATA_D0, INPUT);
  pinMode ( LISY80_HW320_DATA_D1, INPUT);
  pinMode ( LISY80_HW320_DATA_D2, INPUT);
  pinMode ( LISY80_HW320_DATA_D3, INPUT);
 }

 //set com and mode for MCLR
 pinMode ( LISY80_SWPIC_RESET, OUTPUT); digitalWrite (LISY80_SWPIC_RESET, 1);

 //now init switch pic and get Software version, debug options and 'some' S1 DIPs
 dum = lisy80_switch_pic_init();
 if (ls80dbg.bitv.basic)
 {
   sprintf(debugbuf,"Software Switch PIC has version %d.%d\n\r",dum/100, dum%100);
   lisy80_debug(debugbuf);
 }


 //init internal FIFO
 LISY80_BufferInit();
}

//shutdown lisy80 Hardware
void lisy80_hwlib_shutdown(void)
{
 close(fd_disp_pic);
 close(fd_coil_pic);
}
 

//------ PIC write routine ------
// write np bytes to PIC
// int fd : file descriptor of open PIC
// char *buf : pointer to data to write
// returns number of written bytes on success
int lisy80_write_multibyte_pic( int fd, char *buf, int no)
{
 int bytes_written;

 if (( bytes_written = write(fd,buf,no)) != no)
    {
	 if ( fd == fd_disp_pic) lisy80_error(6);
	 if ( fd == fd_coil_pic) lisy80_error(7);
    }
 return(bytes_written);

}


//------ PIC write routine ------
// write one byte to PIC
// int fd : file descriptor of open PIC
// unsigned char buf : byte to write
// returns number of written bytes (1) on success
int lisy80_write_byte_pic( int fd, unsigned char buf)
{
int no;
int repeats = 0;  
char device[20];

 //we try it five times
 while( ++repeats <= 5 )
  {
   if (( no = write(fd,&buf,1)) == 1) //success
     return(no);
   else
     {
	if ( fd == fd_disp_pic) strcpy(device,"Display Pic"); else  strcpy(device,"Coil Pic");
        sprintf(debugbuf,"ERROR: lisy80_write_byte_pic: repeat:%d for byte:%x",repeats,buf);
        lisy80_debug(debugbuf);
     }
  }

  //we are here in case all repeats failed	
  if ( fd == fd_disp_pic) lisy80_error(6);
  if ( fd == fd_coil_pic) lisy80_error(7);
  return(no);
}

//------ DISPLAYS ------
int lisy80_write_byte_disp_pic( unsigned char buf)
{
  return(lisy80_write_byte_pic( fd_disp_pic, buf));
}

int lisy80_write_multibyte_disp_pic( char *buf, int no)
{
  return(lisy80_write_multibyte_pic( fd_disp_pic, buf, no));
}

//------ COILS ------
int lisy80_write_byte_coil_pic( unsigned char buf)
{
  return(lisy80_write_byte_pic( fd_coil_pic, buf));
}


//------ PIC read routine ------
// read one byte from PIC
// int fd : file descriptor of open PIC
//returns byte or <0 in case of error
int lisy80_read_byte_pic( int fd)
{
 unsigned char data;

 if (read(fd, &data, 1) != 1)
  {
    if ( fd == fd_disp_pic) lisy80_error(8);
    if ( fd == fd_coil_pic) lisy80_error(9);
    return(-1);
  }
 else return (data);
}

//read one byte from display PIC
unsigned char lisy80_read_byte_disp_pic(void)
{
 return(lisy80_read_byte_pic(fd_disp_pic));
}
//read one byte from coil PIC
unsigned char lisy80_read_byte_coil_pic(void)
{
 return(lisy80_read_byte_pic(fd_coil_pic));
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

 if (coil <= 48)  //it is a lamp
  {
    if (ls80dbg.bitv.lamps)
     {
        sprintf(debugbuf,"setting lamp:%d to:%d",coil,action);
        lisy80_debug(debugbuf);
     }

   coil_coil_set( coil, action);
  } //if coil <= 48
 else //it is a coil
  {
    if (ls80dbg.bitv.lamps)
     {
        sprintf(debugbuf,"setting coil:%d to:%d (SOLENOID)",coil,action);
        lisy80_debug(debugbuf);
     }
     coil_coil_set( coil, action);
  }
}

/*
	lisy1_coil_set
        coil -> nr of coil
        action -> 0-off, 1-on, 2-pulse
	lamps 1..36
	solenoid 37 .. 44

 we respect minimum PULSE_TIME_IN_MSEC for coils here!
*/
#define	PULSE_TIME_IN_MSEC 150
void lisy1_coil_set( int coil, int action)
{

 unsigned int now;
 int mycoil, pulsetime;
 //remember the status of the coil, as we need a minimum 'ON' time here
 static unsigned char coilstatus[8] = { 0,0,0,0,0,0,0,0 };
 static unsigned int coil_last_on[8];

 if (coil <= 36)  //it is a lamp
  {
    if (ls80dbg.bitv.lamps)
     {
        sprintf(debugbuf,"setting lamp:%d to:%d",coil,action);
        lisy80_debug(debugbuf);
     }

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
	   //calculate difference and sleep a bit
	   pulsetime = ( PULSE_TIME_IN_MSEC ) - ( now - coil_last_on[mycoil]);
	   if (pulsetime > 0) delay( pulsetime );
	 }
	 //remember new status
	 coilstatus[ mycoil ] = 0;
	}
    }

    if (ls80dbg.bitv.coils)
     {
        sprintf(debugbuf,"setting coil:%d to:%d (SOLENOID)",coil,action);
        lisy80_debug(debugbuf);
     }

     //do the setting
     coil_coil_set( coil, action);
  }
}


//set the sound LISY80
void lisy80_sound_set(int sound)
{

if ( ls80dbg.bitv.sound )
  {
        sprintf(debugbuf,"setting sound to:%d" ,sound );
        lisy80_debug(debugbuf);
  }

lisy80_coil_sound_set(sound);


}

//set the sound LISY1
void lisy1_sound_set(int sound)
{

if ( ls80dbg.bitv.sound )
  {
        sprintf(debugbuf,"setting sound to:%d" ,sound );
        lisy80_debug(debugbuf);
  }

lisy1_coil_sound_set(sound);


}

// identiyf if buffer on switch PIC is ready
//resturn 1 if yes, 0 otherwise
//which is the status of the GPIO connected to Switch PIC
int lisy80_switch_readycheck( void )
{
  return( digitalRead(LISY80_BUF_READY) );
}

//init the pic for the switches
//as the pic will send two bytes with
//softwareversion, we give back the version in the int
int lisy80_switch_pic_init(void)
{
 unsigned char version, subversion;

 //do a reset vi MCLR on init
 digitalWrite (LISY80_SWPIC_RESET, 0);
 //wait a bit
 delay(100); //100ms delay from wiringpi
 //set MCLR back to low
 digitalWrite (LISY80_SWPIC_RESET, 1);
 //wait a bit
 delay(100); //100ms delay from wiringpi

 //we do not want to receive data
 digitalWrite (LISY80_ACK_FROM_PI, 0);

 //set init/reset to zero, independent of current state
 digitalWrite (LISY80_INIT, 0);
 //wait a bit
 delay(100); //100ms delay from wiringpi
 //and put to high
 digitalWrite (LISY80_INIT, 1);
 //wait a bit
 delay(100); //100ms delay from wiringpi
 //now the first two "switch values" should be version
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 version = lisy80_read_byte_sw_pic();
 delay(100); //100ms delay from wiringpi
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 subversion = lisy80_read_byte_sw_pic();
 // if subversion is >2 third byte is value of pin connector of switch pic
 //which we use to set debug options
 if (subversion > 2)
  {
    delay(100); //100ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    k1_from_SWPic = lisy80_read_byte_sw_pic();
  }
 else k1_from_SWPic = 0; //no debug for subversion <3!
 // if subversion is >6 fourth byte is part of dipsswitch S1
 if (subversion > 6)
  {
    delay(100); //100ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    dip_from_SWPic = lisy80_read_byte_sw_pic();
  }

 return ( 100*version + subversion);
}

//read the byte from the pic
//we do 'double check' for LISY80_BUF_READY
//even assuming taht the calling routine did
//use lisy80_switch_readycheck()
unsigned char lisy80_read_byte_sw_pic(void)
{

unsigned char paritybit;

union both {
    unsigned char byte;
    struct {
    unsigned D0:1, D1:1, D2:1, D3:1, D4:1, D5:1, D6:1, D7:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } myswitch;

 //now wait for signal from PIC that data is ready
 while ( digitalRead(LISY80_BUF_READY) != 1);

 //once signal is there store first 4 bits in myswitch
 if ( lisy_hardware_revision == 311)
 {
  myswitch.bitv.D0 = digitalRead (LISY80_HW311_DATA_D0);
  myswitch.bitv.D1 = digitalRead (LISY80_HW311_DATA_D1);
  myswitch.bitv.D2 = digitalRead (LISY80_HW311_DATA_D2);
  myswitch.bitv.D3 = digitalRead (LISY80_HW311_DATA_D3);
 }
 else
 {
  myswitch.bitv.D0 = digitalRead (LISY80_HW320_DATA_D0);
  myswitch.bitv.D1 = digitalRead (LISY80_HW320_DATA_D1);
  myswitch.bitv.D2 = digitalRead (LISY80_HW320_DATA_D2);
  myswitch.bitv.D3 = digitalRead (LISY80_HW320_DATA_D3);
 }

 //signal PIC that we want had red first part of databyte
 digitalWrite (LISY80_ACK_FROM_PI, 1);

 //and wait for signal from PIC that second part of data is ready
 while ( digitalRead(LISY80_BUF_READY) != 0);

 //once signal is there store second 4 bits in myswitch
 if ( lisy_hardware_revision == 311)
 {
  myswitch.bitv.D4 = digitalRead (LISY80_HW311_DATA_D0);
  myswitch.bitv.D5 = digitalRead (LISY80_HW311_DATA_D1);
  myswitch.bitv.D6 = digitalRead (LISY80_HW311_DATA_D2);
  myswitch.bitv.D7 = digitalRead (LISY80_HW311_DATA_D3);
 }
 else
 {
  myswitch.bitv.D4 = digitalRead (LISY80_HW320_DATA_D0);
  myswitch.bitv.D5 = digitalRead (LISY80_HW320_DATA_D1);
  myswitch.bitv.D6 = digitalRead (LISY80_HW320_DATA_D2);
  myswitch.bitv.D7 = digitalRead (LISY80_HW320_DATA_D3);
 }

 //and signal  back that we had red the second part
 digitalWrite (LISY80_ACK_FROM_PI, 0);

//check parity and remove parity bit
//b7 is paritybit
paritybit = myswitch.bitv.D7;
myswitch.bitv.D7 = 0;
if ( paritybit != parity(myswitch.byte) )
   fprintf(stderr,"LISY80 lisy80_read_byte_sw_pic PARITY ERROR! \n\r");
 
//do the debugging
//switch debug in switches.c
//for 2.13 we want to have basic debug all time
 lisy80_debug_switches(myswitch.byte);

 //give back the byte
 return myswitch.byte;
}




// identiyf if buffer on switch PIC is ready
// if yes, read switch change from buffer and give back
// if not, returnvalue is 80
int lisy80_switch_reader( unsigned char *action )
{

 int switch_number;
 int switch_return,switch_strobe;

 switchdata_t mydata;

//if the pic is not ready return value 80
if (lisy80_switch_readycheck() == 0) return (80);

//else, read the byte from the pic
mydata.byte=lisy80_read_byte_sw_pic();

switch_return =  mydata.bitv.RETURN;
switch_strobe =  mydata.bitv.STROBE;
switch_number =  switch_return + switch_strobe *10;
*action = mydata.bitv.ONOFF;

return switch_number;

}

// identiyf if buffer on switch PIC is ready
// if yes, read switch change from buffer and give back
// if not, returnvalue is 80
// LISY1 Version for System1
int lisy1_switch_reader( unsigned char *action )
{

 int switch_number;
 int switch_return,switch_strobe;

 switchdata_t mydata;

//if the pic is not ready return value 80
if (lisy80_switch_readycheck() == 0) return (80);

//else, read the byte from the pic
mydata.byte=lisy80_read_byte_sw_pic();

switch_return =  mydata.bitv.RETURN;
switch_strobe =  mydata.bitv.STROBE;
//switch number is different from system80 here
switch_number =  switch_return * 10 + switch_strobe;
*action = mydata.bitv.ONOFF;

return switch_number;

}




//return the value for DIP_SWitch 1 debug option
//remeber that we have inverted logic ON==0
int lisy80_dip1_debug_option(void)
{

 int setting;

 //we support different hardware revisions
 if ( lisy_hardware_revision == 311 )
 {
  pinMode ( LISY80_HW311_DIP1_S7, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S7, PUD_UP);
  setting = digitalRead(LISY80_HW311_DIP1_S7);
 }
 else
 {
  pinMode ( LISY80_HW320_DIP1_S7, INPUT); pullUpDnControl (LISY80_HW320_DIP1_S7, PUD_UP);
  setting = digitalRead(LISY80_HW320_DIP1_S7);
 }

if (setting) return 0; else return 1;

}


//return the value for DIP_SWitch 1 and from stiftleiste
//remember that we have inverted logic ON==0 for dips via GPIO
//we use global vars ls80dbg & ls80opt here
void lisy80_get_dips(void)
{

//we support different hardware revisions
if ( lisy_hardware_revision == 311 )
{
  //set the mode for DIP Switch 1, Input with pull up enable, 'ON' means pulled to GND
  pinMode ( LISY80_HW311_DIP1_S1, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S1, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S2, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S2, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S3, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S3, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S4, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S4, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S5, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S5, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S6, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S6, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S7, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S7, PUD_UP);
  pinMode ( LISY80_HW311_DIP1_S8, INPUT); pullUpDnControl (LISY80_HW311_DIP1_S8, PUD_UP);

  ls80opt.bitv.freeplay = ~digitalRead(LISY80_HW311_DIP1_S1);
  ls80opt.bitv.JustBoom_sound = ~digitalRead(LISY80_HW311_DIP1_S2);
  ls80opt.bitv.watchdog = ~digitalRead(LISY80_HW311_DIP1_S3);
  ls80opt.bitv.coil_protection = ~digitalRead(LISY80_HW311_DIP1_S4);
  ls80opt.bitv.slam = ~digitalRead(LISY80_HW311_DIP1_S5);
  ls80opt.bitv.test = ~digitalRead(LISY80_HW311_DIP1_S6);
  ls80opt.bitv.debug = ~digitalRead(LISY80_HW311_DIP1_S7);
  ls80opt.bitv.autostart = ~digitalRead(LISY80_HW311_DIP1_S8);
}
else
{
  //set the mode for DIP Switch 1, Input with pull up enable, 'ON' means pulled to GND
  //we have only 4 from GPIO as the others are coming from switch PIC
  pinMode ( LISY80_HW320_DIP1_S2, INPUT); pullUpDnControl (LISY80_HW320_DIP1_S2, PUD_UP);
  pinMode ( LISY80_HW320_DIP1_S6, INPUT); pullUpDnControl (LISY80_HW320_DIP1_S6, PUD_UP);
  pinMode ( LISY80_HW320_DIP1_S7, INPUT); pullUpDnControl (LISY80_HW320_DIP1_S7, PUD_UP);
  pinMode ( LISY80_HW320_DIP1_S8, INPUT); pullUpDnControl (LISY80_HW320_DIP1_S8, PUD_UP);

  ls80opt.bitv.JustBoom_sound = ~digitalRead(LISY80_HW320_DIP1_S2);
  ls80opt.bitv.test = ~digitalRead(LISY80_HW320_DIP1_S6);
  ls80opt.bitv.debug = ~digitalRead(LISY80_HW320_DIP1_S7);
  ls80opt.bitv.autostart = ~digitalRead(LISY80_HW320_DIP1_S8);

  ls80opt.byte |= dip_from_SWPic;

}

//in debug mode be verbose
if (ls80dbg.bitv.basic == 1 )
 {
  if (ls80opt.bitv.freeplay ) lisy80_debug("LISY80 option: Freeplay");
  if (ls80opt.bitv.JustBoom_sound ) lisy80_debug("LISY80 option: JustBoom_sound");
  if (ls80opt.bitv.watchdog ) lisy80_debug("LISY80 option: watchdog");
  if (ls80opt.bitv.coil_protection ) lisy80_debug("LISY80 option: coil_protection");
  if (ls80opt.bitv.slam ) lisy80_debug("LISY80 option: slam");
  if (ls80opt.bitv.test ) lisy80_debug("LISY80 option: test");
  if (ls80opt.bitv.debug ) lisy80_debug("LISY80 option: debug");
  if (ls80opt.bitv.autostart ) lisy80_debug("LISY80 option: autostart");
 }

//debug options are coming from switch PIC
ls80dbg.byte |= k1_from_SWPic;

//in debug mode be verbose
if (ls80dbg.bitv.basic == 1 )
{
 if ( ls80dbg.bitv.displays ) lisy80_debug("LISY80 DEBUG activ for displays");
 if ( ls80dbg.bitv.switches ) lisy80_debug("LISY80 DEBUG activ for switches");
 if ( ls80dbg.bitv.lamps ) lisy80_debug("LISY80 DEBUG activ for lamps");
 if ( ls80dbg.bitv.coils ) lisy80_debug("LISY80 DEBUG activ for coils");
 if ( ls80dbg.bitv.sound ) lisy80_debug("LISY80 DEBUG activ for sound");
}
else ls80dbg.byte = 0;  //override settings from stiftleiste, in case S2 is set to OFF, no debug


  return;

}

//set the LED for showing display PIC activity
void lisy80_set_dp_led ( int value )
{
  if ( lisy_hardware_revision == 311)
     digitalWrite (LISY80_HW311_LED_DP, value);
}

//toggle the LED for showing display PIC activity
void toggle_dp_led (void)
{
  static unsigned char status = 1;

  if (status == 1) status =0; else status = 1;

  if ( lisy_hardware_revision == 311)
      digitalWrite (LISY80_HW311_LED_DP, status);
}

//set the LEDs of the 'traffic ligth'
void lisy80_set_red_led( int value )
{
 int dum;
 if ( lisy_hardware_revision == 311) dum = LISY80_HW311_LED_RED; else  dum = LISY80_HW320_LED_RED;
 digitalWrite (dum, value);
}

void lisy80_set_yellow_led( int value )
{
 digitalWrite (LISY80_LED_YELLOW, value);
}

void lisy80_set_green_led( int value )
{
 digitalWrite (LISY80_LED_GREEN, value);
}

//discharge function for reading capacitor data
void lisy80_discharge(void)
{
  pinMode (LISY80_A_PIN, INPUT) ;
  pinMode (LISY80_B_PIN, OUTPUT) ;
  digitalWrite (LISY80_B_PIN, LOW) ;
  delay(10);  //10 ms
}

// time function for capturing analog count value
int  lisy80_charge_time(void)
{
  int count = 0;

  pinMode (LISY80_B_PIN, INPUT) ;
  pinMode (LISY80_A_PIN, OUTPUT) ;

  digitalWrite (LISY80_A_PIN, HIGH) ;
  while ( digitalRead(LISY80_B_PIN) == LOW) count++;

  return count;

}

//read position of poti
int lisy80_get_poti_val(void)
{

 //discharge first
 lisy80_discharge();
 //return position
 return( lisy80_charge_time() );

}
