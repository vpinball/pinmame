/*
  hw_lib
  part of lisy80NG
  bontango 09.2017
  LISY1 & LISY36 included
*/


#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <wiringPi.h>
//this is for I2C over open&fcntl
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <i2c/smbus.h>
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
#include "fadecandy.h"
#include "usbserial.h"
#include "lisy_api_com.h"
#include "lisy_home.h"
#include "externals.h"
#include "lisy.h"


#define SW_PIC_INIT_DEL_IN_MS 50

//global vars for I2C
int fd_disp_pic;	//file descriptor for display pic
int fd_coil_pic;	//file descriptor for coil pic
//fd_apc via externals.h from lisy_w.c

//local vars for handling values for K1(debug) and part of S1, usually from switch pic
unsigned char K1_debug_values = 0;
unsigned char dip_missing_values = 0;

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
//and hardware_ID
// -1 in case of error
int lisy_get_hardware_revision(int disp_sw_ver, int *hw_ID)
{

 int file, res;
/*
 * new: eeprom of display & coil pic used, no separate eeprom anymore
 * hardware revision is stored in display pic now
 * 1 .. 40 LISY80
 *    21 LISY_HOME
 *    31 LISY80_LED
 * 41 .. 80 LISY1
 * 81 .. 120 LISY35
 * with that, siftware version of PIC has to be is >= 4
 *
 *  otherwise try to open eeprom via i2c in order to detect hardware revision
 *  Hardware Version 3.11 has eeprom at I2C address 0x50 & 0x51
 *  Hardware Version 3.20 has eeprom at I2C address 0x54 & 0x55
 *  LISY1, based on Hardware Version 3.20 has eeprom at I2C address 0x52 & 0x53
 *  LISY35, based on Hardware Version 3.20 has eeprom at I2C address 0x56 & 0x57
*/

 //we want main version only
 disp_sw_ver =  disp_sw_ver/100;

 //init hw_ID
 *hw_ID = LISY_HW_ID_NONE;
 
 if (disp_sw_ver >= 4)
 {

   //set global var, we do not have an separate eeprom anymore (24c04)
   lisy_has24c04 = 0;

   //get hw rev stored in PIC
   res = display_get_hw_revision( );

   //send back hw ID also
   *hw_ID = res;

   //send back hw revision based on ID
   switch(res)
    {
	case 1 ... 40: return(320);
		       break;
	case 41 ... 80: return(100);
		       break;
	case 81 ... 120: return(350);
		       break;
	default: return(-1);
		       break;
    }
 }
 else
 {
 //old version with eeprom here
 if (( file = open(I2C_DEVICE, O_RDWR)) < 0 )
     lisy80_error(2);

 // probe for slave address at hw revision 320
 ioctl(file, I2C_SLAVE, 0x54);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    //fprintf(stderr,"Info: LISY80 Hardware Revision 320\n");
    close(file); //close bus
    return(320);
   }

 // probe for slave address at hw revision 100 (LISY1)
 ioctl(file, I2C_SLAVE, 0x52);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    //fprintf(stderr,"Info: LISY1, based on  LISY80 Hardware Revision 320\n");
    close(file); //close bus
    return(100);
   }

 // probe for slave address at hw revision 311
 ioctl(file, I2C_SLAVE, 0x50);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    //fprintf(stderr,"Info: LISY80 Hardware Revision 311\n");
    close(file); //close bus
    return(311);
   }

 // probe for slave address at hw revision 350 (LISY35)
 ioctl(file, I2C_SLAVE, 0x56);
 res = i2c_smbus_read_byte(file);
 if ( res >= 0)
   {
    fprintf(stderr,"Info: LISY Hardware Revision 350 (LISY35)\n");
    close(file); //close bus
    return(350);
   }

 //no detect so far, so give back error (-1)
 close(file); //close bus
 return(-1);
 
 }//eeprom version
}


//Hardware INIT LISY mini (e.g Williams)
void lisymini_hwlib_init( void )
{

 //set GPIOs for the traffic ligth
 pinMode ( LISY_MINI_LED_RED, OUTPUT);
 pinMode ( LISY80_LED_YELLOW, OUTPUT);
 pinMode ( LISY_MINI_LED_GREEN, OUTPUT);

 //lets do debug
 if (ls80dbg.bitv.basic) lisy80_debug("LISY_Mini Hardware init start");

 //fix for LISYmini at the moment
 lisy_hardware_revision = LISY_HW_LISY_W;

 //get value of k3 dip;
 //3 == no options active
 lisy_K3_value = lisymini_get_dip("K3");

 //now get debug options
 K1_debug_values = lisymini_get_dip("K1");

 //init usb serial
 fd_api = lisy_usb_init();
 if ( fd_api >= 0) 
  fprintf(stderr,"Info: usb serial successfull initiated\n");
 else
 {
  fprintf(stderr,"ERROR: cannot open usb serial communication\n");
 lisy80_set_red_led(1);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(0);
  exit(1);
 }

 //do some debug output if requested
 //number of displays
 if (ls80dbg.bitv.basic) lisy_api_print_hw_info();


 //set all the leds controlled by the PI
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(1);

 //init internal FIFO
 LISY80_BufferInit();
}


//Hardware INIT LISY on APC via serial (e.g Williams)
void lisyapc_hwlib_init( void )
{

 int ret;

//set GPIOs for the traffic ligth
 pinMode ( LISY_MINI_LED_RED, OUTPUT);
 pinMode ( LISY80_LED_YELLOW, OUTPUT);
 pinMode ( LISY_MINI_LED_GREEN, OUTPUT);

 //lets do debug
 if (ls80dbg.bitv.basic) lisy80_debug("LISY APC Hardware init start");

 //fix for LISYmini at the moment
 lisy_hardware_revision = LISY_HW_LISY_W;

 //get value of k3 dip;
 //3 == no options active
 //lisy_K3_value = lisymini_get_dip("K3");
 lisy_K3_value = 3;


 //init usb serial
 fd_api = lisy_serial_init();
 if ( fd_api >= 0)
  fprintf(stderr,"Info: serial com successfull initiated\n");
 else
 {
  fprintf(stderr,"ERROR: cannot open serial communication\n");
 lisy80_set_red_led(1);
 lisy80_set_yellow_led(0);
 lisy80_set_green_led(0);
  exit(1);
 }

 //make sure  connected hardware ID is APC
 ret = lisy_api_check_con_hw( "APC" );
 fprintf(stderr,"Info: check ID for 'APC' returns %d\n",ret);

 //now get debug options
 if (ls80dbg.bitv.basic)
 { 
   K1_debug_values = lisyapc_get_dip("K1");

   sprintf(debugbuf,"LISY APC basic DEBUG activ, APC optionbyte is: %d",K1_debug_values);
   lisy80_debug(debugbuf);

   //debug options are coming from switch APC, adaption needed
   ls80dbg.bitv.displays = CHECK_BIT( K1_debug_values, 0);
   ls80dbg.bitv.switches = CHECK_BIT( K1_debug_values, 1);
   ls80dbg.bitv.lamps = CHECK_BIT( K1_debug_values, 2);
   ls80dbg.bitv.coils = CHECK_BIT( K1_debug_values, 3);
   ls80dbg.bitv.sound = CHECK_BIT( K1_debug_values, 4);


   if ( ls80dbg.bitv.displays ) lisy80_debug("LISY80 DEBUG activ for displays");
   if ( ls80dbg.bitv.switches ) lisy80_debug("LISY80 DEBUG activ for switches");
   if ( ls80dbg.bitv.lamps ) lisy80_debug("LISY80 DEBUG activ for lamps");
   if ( ls80dbg.bitv.coils ) lisy80_debug("LISY80 DEBUG activ for coils");
   if ( ls80dbg.bitv.sound ) lisy80_debug("LISY80 DEBUG activ for sound");
   }
   else ls80dbg.byte = 0; 

 //do some debug output if requested
 //number of displays
 if (ls80dbg.bitv.basic) lisy_api_print_hw_info();

 //init internal FIFO
 LISY80_BufferInit();
}



//Hardware INIT LISY 1/35/80
void lisy_hwlib_init( void )
{

 int disp_sw_ver,coil_sw_ver,switch_sw_ver;


 //open the I2C bus
 //as filedescriptors are global vars
 //
 // open I2C bus and com to switch PIC, assign file descriptors
 // and get SW version of each PIC
 if (ls80dbg.bitv.basic) lisy80_debug("LISY Hardware init start");


 //******************
 // PIC 1 - Displays
 //******************
 if ((fd_disp_pic = open( I2C_DEVICE,O_RDWR)) < 0)
        lisy80_error(2);
 // Set the port options and set the address of the device we wish to speak to
 if (ioctl(fd_disp_pic, I2C_SLAVE, DISP_PIC_ADDR) < 0)
        lisy80_error(3);

 //get Software version, TODO: to be checked
 disp_sw_ver = display_get_sw_version();
 //store it global
 lisy_env.disp_sw_ver = disp_sw_ver;
 if (ls80dbg.bitv.basic)
 {
   sprintf(debugbuf,"Software Display PIC has version %d.%d",disp_sw_ver/100, disp_sw_ver%100);
   lisy80_debug(debugbuf);
 }

 //look for hardware we ar running on
 //info is either via i/O port eeprom or stored in display PIC
 lisy_hardware_revision = lisy_get_hardware_revision(disp_sw_ver, &lisy_hardware_ID);
 if ( lisy_hardware_revision < 0)  lisy80_error(11);
 //print info about hw_revision
 if (ls80dbg.bitv.basic == 1 ) fprintf(stderr,"Info: LISY80 Hardware:  Revision %d ; HW ID is %d \n",lisy_hardware_revision,lisy_hardware_ID);

 //show what version this pic is using
 display_show_pic_sw_version(1,disp_sw_ver);

 //set gpio for LED which shows display PIC activity (no output left on device :-( )
 //HW 311 only
 if ( lisy_hardware_revision == 311)
 {
  pinMode ( LISY80_HW311_LED_DP, OUTPUT);
  digitalWrite (LISY80_HW311_LED_DP, 1);
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
 coil_sw_ver = coil_get_sw_version();
 //store it global
 lisy_env.coil_sw_ver = coil_sw_ver;
 if (ls80dbg.bitv.basic)
 {
   sprintf(debugbuf,"Software Coil PIC has version %d.%d",coil_sw_ver/100, coil_sw_ver%100);
   lisy80_debug(debugbuf);
 }

 //show what version this pic is using
 display_show_pic_sw_version(2,coil_sw_ver);

 //get value of k3 dip; hardware 320 only 
 if ( lisy_hardware_revision != 311)
 {
  lisy_K3_value = coil_get_k3();
  //debug
  if (ls80dbg.bitv.basic)
  {
   sprintf(debugbuf,"K3 Coil PIC returns %d",lisy_K3_value);
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
 //for LISY35 we have a separate routine ( lisy35_switch_pic_init() )
 //which is called from main.c via 'lisy35_set_variant' in addition
 switch_sw_ver = lisy80_switch_pic_init();
 //store it global
 lisy_env.switch_sw_ver = switch_sw_ver;
   if (ls80dbg.bitv.basic)
   {
     sprintf(debugbuf,"Software Switch PIC has version %d.%d",switch_sw_ver/100, switch_sw_ver%100);
     lisy80_debug(debugbuf);
   }

 //show what version this pic is using
 display_show_pic_sw_version(3,switch_sw_ver);


 //******************
 // general stuff
 //******************

 //as the eeprom should be readable at this time
 //print some stats while in debug mode
 //if (ls80dbg.bitv.basic == 1 ) lisy_eeprom_printstats();

 //set GPIOs for the traffic ligth
 if ( lisy_hardware_revision == 311)
    pinMode ( LISY80_HW311_LED_RED, OUTPUT);
 else  pinMode ( LISY80_HW320_LED_RED, OUTPUT);

 pinMode ( LISY80_LED_YELLOW, OUTPUT); 
 pinMode ( LISY80_LED_GREEN, OUTPUT);
 //set all the leds controlled by the PI
 lisy80_set_red_led(0);
 lisy80_set_yellow_led(1);
 lisy80_set_green_led(0);
 lisy80_set_dp_led(1);


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
        sprintf(debugbuf,"ERROR: lisy80_write_byte_pic(%s): repeat:%d for byte:%x",device,repeats,buf);
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

int lisy80_write_multibyte_coil_pic( char *buf, int no)
{
  return(lisy80_write_multibyte_pic( fd_coil_pic, buf, no));
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

//set the sound LISY1
void lisy1_sound_set(int sound)
{

  int mysound;

  //sound is solenoids 3..7, so need to shift here
  if ( sound & 0x01 ) lisy1_coil_set( Q_TENS, 1); else lisy1_coil_set( Q_TENS, 0);
  if ( sound & 0x02 ) lisy1_coil_set( Q_HUND, 1); else lisy1_coil_set( Q_HUND, 0);
  if ( sound & 0x04 ) lisy1_coil_set( Q_TOUS, 1); else lisy1_coil_set( Q_TOUS, 0);
}

//set the sound LISY80
void lisy80_sound_set(int sound)
{
  lisy80_coil_sound_set(sound);
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
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 //set MCLR back to low
 digitalWrite (LISY80_SWPIC_RESET, 1);
 //wait a bit
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi

 //we do not want to receive data
 digitalWrite (LISY80_ACK_FROM_PI, 0);

 //set init/reset to zero, independent of current state
 digitalWrite (LISY80_INIT, 0);
 //wait a bit
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 //and put to high
 digitalWrite (LISY80_INIT, 1);
 //wait a bit
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 //now the first two "switch values" should be version
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 version = lisy80_read_byte_sw_pic();
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 subversion = lisy80_read_byte_sw_pic();

 // if subversion is >2 or version >3 
 // third byte is value of pin connector of switch pic
 // which we use to set debug options
 if ( (subversion > 2) || ( version > 3 ) )
  {
    delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    K1_debug_values = lisy80_read_byte_sw_pic();
  }
 else K1_debug_values = 0; //no debug for subversion <3!

 // if subversion is >6 or version > 3 fourth byte is part of dipsswitch S1
 if ( (subversion > 6)  || ( version > 3 ) )
  {
    delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    dip_missing_values = lisy80_read_byte_sw_pic();
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
// LISY35 Version for Bally
int lisy35_switch_reader( unsigned char *action )
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
//switch number is different from system80/system1 here
//as bally decided to number the switches serially and start with '1' (not '0')
//40 switches in general (48 with extension for some games)
switch_number =  switch_return  + switch_strobe * 8;
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
  ls80opt.bitv.sevendigit = ~digitalRead(LISY80_HW311_DIP1_S4);
  ls80opt.bitv.slam = ~digitalRead(LISY80_HW311_DIP1_S5);
  ls80opt.bitv.test = ~digitalRead(LISY80_HW311_DIP1_S6);
  ls80opt.bitv.debug = ~digitalRead(LISY80_HW311_DIP1_S7);
  ls80opt.bitv.autostart = ~digitalRead(LISY80_HW311_DIP1_S8);
}
else if ( lisy_hardware_revision == 121 )  //lisy mini
{
 ls80opt.byte = lisymini_get_dip("S1");
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

  ls80opt.byte |= dip_missing_values;

  //RTH only sevendigit if autostart is on (==0), meaning no MPF support at the moment
  if (ls80opt.bitv.autostart == 1 ) ls80opt.bitv.sevendigit = 0;

}

//in debug mode be verbose
if (ls80dbg.bitv.basic == 1 )
 {
  if (ls80opt.bitv.freeplay ) lisy80_debug("LISY80 option: Freeplay");
  if (ls80opt.bitv.JustBoom_sound ) lisy80_debug("LISY80 option: JustBoom_sound");
  if (ls80opt.bitv.watchdog ) lisy80_debug("LISY80 option: watchdog");
  if (ls80opt.bitv.sevendigit ) lisy80_debug("LISY80 option: seven digit");
  if (ls80opt.bitv.slam ) lisy80_debug("LISY80 option: slam");
  if (ls80opt.bitv.test ) lisy80_debug("LISY80 option: test");
  if (ls80opt.bitv.debug ) lisy80_debug("LISY80 option: debug");
  if (ls80opt.bitv.autostart ) lisy80_debug("LISY80 option: autostart");
 }



//debug options are coming from switch PIC
ls80dbg.byte |= K1_debug_values;

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
//red LED is used for error signaling
//only set if status changed
void lisy80_set_red_led( int value )
{
 int dum;
 static unsigned char first = 1;
 static unsigned int status;

 if (first)
 {
   first = 0;
   // make status != value
   status = value+1;
 } 

 //only set if status changed
 if ( status != value)
 {
  if ( lisy_hardware_revision == 311) dum = LISY80_HW311_LED_RED; 
  else if ( lisy_hardware_revision == LISY_HW_LISY_W) dum = LISY_MINI_LED_RED; 
  else  dum = LISY80_HW320_LED_RED;

  digitalWrite (dum, value);
  status = value;
 }
}

void lisy80_set_yellow_led( int value )
{
 digitalWrite (LISY80_LED_YELLOW, value);
}

void lisy80_set_green_led( int value )
{
int dum;
 if ( lisy_hardware_revision == LISY_HW_LISY_W) dum = LISY_MINI_LED_GREEN; 
 else  dum = LISY80_LED_GREEN;

 digitalWrite (dum, value);
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
#define LISY_CHARGE_TIME_MAXCOUNT 10000
int  lisy80_charge_time(void)
{
  int count = 0;

  pinMode (LISY80_B_PIN, INPUT) ;
  pinMode (LISY80_A_PIN, OUTPUT) ;

  digitalWrite (LISY80_A_PIN, HIGH) ;
  while ( digitalRead(LISY80_B_PIN) == LOW)
     {
        count++;
        if (count > LISY_CHARGE_TIME_MAXCOUNT)
          {
          sprintf(debugbuf,"Warning: LISY LISY_CHARGE_TIME_MAXCOUNT exceeded, volume setting may not be correct\n");
          lisy80_debug(debugbuf);
          break;
          }
	}

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


//init the pic for the switches
//as the pic will send two bytes with
//softwareversion, we give back the version in the int
//routine for LISY35 called from main via lisy35_set_variant
void lisy35_switch_pic_init(unsigned char variant)
{
 unsigned char version, subversion, dum;

 //signal switch pic which variant we have
 // variant - INIT - ACK
 //   0        0      0
 //   'RUN'    1      0
 //   1        0      1
 //   2        1      1
  if (ls80dbg.bitv.basic)
  {
    sprintf(debugbuf,"Info: LISY35 will use switch variant:%d ( next 4 switch_reader actions to be ignored)",variant);
    lisy80_debug(debugbuf);
  }
 if (variant == 1) {  digitalWrite (LISY80_INIT, 0); digitalWrite (LISY80_ACK_FROM_PI, 1); }
 else if (variant == 2) {  digitalWrite (LISY80_INIT, 1); digitalWrite (LISY80_ACK_FROM_PI, 1); }
 else {  digitalWrite (LISY80_INIT, 0); digitalWrite (LISY80_ACK_FROM_PI, 0); }


 //do a reset vi MCLR on init
 digitalWrite (LISY80_SWPIC_RESET, 0);
 //wait a bit
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 //set MCLR back to low
 digitalWrite (LISY80_SWPIC_RESET, 1);


 //now the switch pic will read INIT & ACK and set switch_variant accordantly
 //we need to give him 'some time' to do so 
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi

 //now we set ACK=0 & INIT=1 which means 'running' for switch PIC
 digitalWrite (LISY80_INIT, 1); digitalWrite (LISY80_ACK_FROM_PI, 0);
 //wait a bit
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi

 //now the first two "switch values" should be version
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 version = lisy80_read_byte_sw_pic();
 delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
 while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
 subversion = lisy80_read_byte_sw_pic();
 // if subversion is >2 or version > 3
 //third byte is value of pin connector of switch pic
 //which we do not need here as already red by lisy80_switch_init
 if ( (subversion > 2) || ( version > 3 ) )
  {
    delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    dum = lisy80_read_byte_sw_pic();
  }
 // if subversion is >6 or version >3  fourth byte is part of dipsswitch S1
 //which we do not need here as already red by lisy80_switch_init
  if ( (subversion > 6)  || ( version > 3 ) )
  {
    delay(SW_PIC_INIT_DEL_IN_MS); //ms delay from wiringpi
    while (lisy80_switch_readycheck() == 0); //wait for switch pic to be ready
    dum = lisy80_read_byte_sw_pic();
  }

}

//read dips connected to lisy_mini
//which are:
// part of S1: options
// S2: game slection (DIP8)
// K1: debug options (5)
// K3: option fadecandy/hotspot (2)
// K2: not used (1)
unsigned char lisymini_get_dip( char* wantdip)
{

#define LISYMINI_STROBE_1 25
#define LISYMINI_STROBE_2 13
#define LISYMINI_STROBE_3 2
#define LISYMINI_STROBE_4 0

#define LISYMINI_RET_1 10
#define LISYMINI_RET_2 11
#define LISYMINI_RET_3 26
#define LISYMINI_RET_4 27
#define LISYMINI_RET_5 28

#define LISYMINI_WAITTIME 10

typedef union {
    unsigned char byte;
    struct {
    unsigned one:1, two:1, three:1, four:1, five:1, six:1, seven:1, eight:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } bitfield_t;

static bitfield_t S1,S2,K1,K2,K3;
static unsigned char first = 1;

//only one time to read
 if(first)
 {
  S1.byte = S2.byte = K1.byte = K2.byte = K3.byte = 0;

  pinMode ( LISYMINI_STROBE_1, OUTPUT);
pinMode ( LISYMINI_STROBE_2, OUTPUT);
pinMode ( LISYMINI_STROBE_3, OUTPUT);
pinMode ( LISYMINI_STROBE_4, OUTPUT);

pinMode ( LISYMINI_RET_1, INPUT);
pinMode ( LISYMINI_RET_2, INPUT);
pinMode ( LISYMINI_RET_3, INPUT);
pinMode ( LISYMINI_RET_4, INPUT);
pinMode ( LISYMINI_RET_5, INPUT);

pullUpDnControl (LISYMINI_RET_1, PUD_DOWN);
pullUpDnControl (LISYMINI_RET_2, PUD_DOWN);
pullUpDnControl (LISYMINI_RET_3, PUD_DOWN);
pullUpDnControl (LISYMINI_RET_4, PUD_DOWN);
pullUpDnControl (LISYMINI_RET_5, PUD_DOWN);

//strobe 4
digitalWrite (LISYMINI_STROBE_1,0);
digitalWrite (LISYMINI_STROBE_2,0);
digitalWrite (LISYMINI_STROBE_3,0);
digitalWrite (LISYMINI_STROBE_4,1);
//wait a bit
delay(LISYMINI_WAITTIME); //100ms delay from wiringpi
S2.bitv.eight = digitalRead (LISYMINI_RET_1);
S2.bitv.four = digitalRead (LISYMINI_RET_2);
K1.bitv.five = digitalRead (LISYMINI_RET_3);
K1.bitv.one = digitalRead (LISYMINI_RET_4);
S1.bitv.five = digitalRead (LISYMINI_RET_5);

//strobe 3
digitalWrite (LISYMINI_STROBE_1,0);
digitalWrite (LISYMINI_STROBE_2,0);
digitalWrite (LISYMINI_STROBE_3,1);
digitalWrite (LISYMINI_STROBE_4,0);
//wait a bit
delay(LISYMINI_WAITTIME); //100ms delay from wiringpi
S2.bitv.seven = digitalRead (LISYMINI_RET_1);
S2.bitv.three = digitalRead (LISYMINI_RET_2);
K1.bitv.four = digitalRead (LISYMINI_RET_3);
K3.bitv.two = ~digitalRead (LISYMINI_RET_4);  //k3 is invers
S1.bitv.four = digitalRead (LISYMINI_RET_5);

//strobe 2
digitalWrite (LISYMINI_STROBE_1,0);
digitalWrite (LISYMINI_STROBE_2,1);
digitalWrite (LISYMINI_STROBE_3,0);
digitalWrite (LISYMINI_STROBE_4,0);
//wait a bit
delay(LISYMINI_WAITTIME); //100ms delay from wiringpi
S2.bitv.six = digitalRead (LISYMINI_RET_1);
S2.bitv.two = digitalRead (LISYMINI_RET_2);
K1.bitv.three = digitalRead (LISYMINI_RET_3);
K3.bitv.one = ~digitalRead (LISYMINI_RET_4);  //K3 is invers
S1.bitv.three = digitalRead (LISYMINI_RET_5);

//strobe 1
digitalWrite (LISYMINI_STROBE_1,1);
digitalWrite (LISYMINI_STROBE_2,0);
digitalWrite (LISYMINI_STROBE_3,0);
digitalWrite (LISYMINI_STROBE_4,0);
//wait a bit
delay(LISYMINI_WAITTIME); //100ms delay from wiringpi
S2.bitv.five = digitalRead (LISYMINI_RET_1);
S2.bitv.one = digitalRead (LISYMINI_RET_2);
K1.bitv.two = digitalRead (LISYMINI_RET_3);
K2.bitv.one = digitalRead (LISYMINI_RET_4);
S1.bitv.one = digitalRead (LISYMINI_RET_5);

  //only one time
  first = 0;
 }

 //give back value wanted
 if(strcmp(wantdip,"S1") == 0) return S1.byte;
 else if(strcmp(wantdip,"S2") == 0) return S2.byte;
 else if(strcmp(wantdip,"K1") == 0) return K1.byte;
 else if(strcmp(wantdip,"K2") == 0) return K2.byte;
 else if(strcmp(wantdip,"K3") == 0) return K3.byte;
 else return 0;

} 

//read dips connected to lisy_mini
//which are:
// part of S1: options
// S2: game slection (DIP8)
// K1: debug options (5)
// K3: option fadecandy/hotspot (2)
// K2: not used (1)
unsigned char lisyapc_get_dip( char* wantdip)
{

typedef union {
    unsigned char byte;
    struct {
    unsigned one:1, two:1, three:1, four:1, five:1, six:1, seven:1, eight:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } bitfield_t;

static bitfield_t S1,S2,K1,K2,K3;
static unsigned char first = 1;

//- Setting 2 -> Alle Options DIP Schalter als Wert von 0 - 255
//- Setting 3 -> Alle Game DIP Schalter (S2) als Wert von 0 - 255 (Nummer des PinMame Spiels)
//- Settings 4 -> Alle Debug Jumper K1, K2 und K3 als Wert von 0 - 255 (K1 stellt dann die Bits 0-4, K2 Bit 5 / 6 und K3 ist Bit 7)

 //give back value wanted
 if(strcmp(wantdip,"S1") == 0) return lisy_api_get_dip_switch(2);
 else if(strcmp(wantdip,"S2") == 0) return lisy_api_get_dip_switch(3); //S2 is gamenumber
 else if(strcmp(wantdip,"K1") == 0) return ( 0x1F & lisy_api_get_dip_switch(4)); //debug options
 else if(strcmp(wantdip,"K2") == 0) return ( 0x60 & lisy_api_get_dip_switch(4));
 else if(strcmp(wantdip,"K3") == 0) return ( 0x80 & lisy_api_get_dip_switch(4));
 else return 0;


}
