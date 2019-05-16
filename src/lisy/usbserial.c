/*
 USB serial routines for LISY
 April 2019
 bontango
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "lisy_api.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "fadecandy.h"
#include "opc.h"
#include "utils.h"
#include "externals.h"
#include "usbserial.h"


//local vars
static int lisy_usb_serfd;

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
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1; //timeout is 0.1secs
    //tty.c_cc[VTIME] = 0; //RTH: no timimng here, input is not a keyboard

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


//send command cmd
//read \0 terminated string and store into string content
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive string
int read_string(unsigned char cmd, char *content)
{

  char nextbyte;
  int i,n,ret;

 //send command
 if ( write( lisy_usb_serfd,&cmd,1) != 1)
    {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
    }
 
 //receive answer
 i=0;
 do {
  if ( ( ret = read(lisy_usb_serfd,&nextbyte,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }
  content[i] = nextbyte;
  i++;
  } while ( nextbyte != '\0');
 
 return(i);

}

//read one byte, and return data into *data 
//return -2 in case we had problems to send  cmd
//return -1 in case we had problems to receive byte
//return 0 otherwise
unsigned char read_byte(unsigned char cmd, unsigned char *data)
{

 //send command
 if ( write( lisy_usb_serfd,&cmd,1) != 1) return (-2);

 //receive answer
 if ( read(lisy_usb_serfd,data,1) != 1) return (-1);

 return(0);

}


#define ARDUINO_NO_TRIES 9
int lisy_usb_init()
{

   int i,j,n,ret,tries;
   char *portname = "/dev/ttyACM0";
   unsigned char data,cmd;
   char answer[40];

    //open interface
    lisy_usb_serfd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (lisy_usb_serfd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(lisy_usb_serfd, B115200);


    //start with asking for Hardware string
    //for init we reapeat that up to ten times in case
    //the other side ( Arduino) needs time to resset
    //set the command first
    cmd = LISY_G_HW;
    //and flush buffer
    sleep(1); //need to wait a bit beofre doing that
    tcflush(lisy_usb_serfd,TCIOFLUSH);
    //now send and try to read answer (one byte)
    ret = tries = 0;
    do
    {
	//send cmd
     if ( write( lisy_usb_serfd,&cmd,1) != 1)
      {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
      }

      //read answer
	if ( ( ret = read(lisy_usb_serfd,&data,1)) != 1)
         {
	   tries++; //count tries
	   sleep(1); //wait a second
	   tcflush(lisy_usb_serfd,TCIOFLUSH); //flush buffers
	   fprintf(stderr,"send cmd to %s, %d times\n",portname,tries);
	 }
    }
    while( (ret == 0) & ( tries < ARDUINO_NO_TRIES));


    //look if we exceeded tries
    if (tries >= ARDUINO_NO_TRIES) 
    {
      fprintf(stderr,"USBSerial: Init failed\n");
      return (-1);
    }
    else { n=0; answer[n] = data; n++; }

    
    //now read the rest
    do {
    if ( ( ret = read(lisy_usb_serfd,&data,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }
    answer[n] = data;
    n++;
    } while (( data != '\0') & ( n < 10));

    fprintf(stderr,"LISY_Mini: HW Client is: %s (nis%d)\n",answer,n);


  if ( ls80dbg.bitv.basic ) 
  {
/*
    sprintf(debugbuf,"LISY_Mini: Client uses API Version: %s",answer);
    lisy80_debug(debugbuf);

    if ( read_string(LISY_G_HW, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: HW Client is: %s",answer);
    lisy80_debug(debugbuf);

    if ( read_string(LISY_G_LISY_VER, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: SW Client is: %s",answer);
    lisy80_debug(debugbuf);
*/
    if ( read_byte(LISY_G_NO_LAMPS, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d lamps",data);
    lisy80_debug(debugbuf);

    if ( read_byte(LISY_G_NO_SOL, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d solenoids",data);
    lisy80_debug(debugbuf);
/*  
    if ( read_byte(LISY_G_NO_SOUNDS, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d sounds",data);
    lisy80_debug(debugbuf);

    if ( read_byte(LISY_G_NO_DISP, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d displays",data);
    lisy80_debug(debugbuf);
*/
    if ( read_byte(LISY_G_NO_SW, &data) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Client supports %d switches",data);
    lisy80_debug(debugbuf);

    //print status of switches
    printf("Switch Status:\n");
    for(i=0; i<=7; i++)
     {
       for(j=1; j<=10; j++)
	{
	 n = j + i*10;
	 printf("%d ",lisy_usb_get_switch_status(n));
	}
      printf("\n");
     }//i

/*  
    if ( read_string(LISY_G_GAME_INFO, answer) < 0) return (-1);
    sprintf(debugbuf,"LISY_Mini: Game Info: %s",answer);
    lisy80_debug(debugbuf);
*/
   }

  return 0;

}


//send a string to a display
int lisy_usb_send_str_to_disp(unsigned char num, char *str)
{

 unsigned char cmd, len;

 cmd = 255;
 switch(num)
 {
	case 0: cmd = LISY_S_DISP_0; break;
	case 1: cmd = LISY_S_DISP_1; break;
	case 2: cmd = LISY_S_DISP_2; break;
	case 3: cmd = LISY_S_DISP_3; break;
	case 4: cmd = LISY_S_DISP_4; break;
	case 5: cmd = LISY_S_DISP_5; break;
	case 6: cmd = LISY_S_DISP_6; break;
 }

 if ( ls80dbg.bitv.displays )
   {
    sprintf(debugbuf,"send cmd %d to Display %d: str: %s",cmd,num,str);
    lisy80_debug(debugbuf);
   }


 if( cmd != 255 )
 {
 //send command
 if ( write( lisy_usb_serfd,&cmd,1) != 1) { fprintf(stderr,"ERROR write cmd\n"); return (-1); }
 //send string
 len = strlen(str)+1; //include trailing \0
 if ( write( lisy_usb_serfd,str,len) != len) { fprintf(stderr,"ERROR write display\n"); return (-2); }
 }

 return (len+1);
}

//ask for changed switches
unsigned char lisy_usb_ask_for_changed_switch(void)
{

 unsigned char my_switch;
 int ret;

 //ask APC via serial
 ret = read_byte(LISY_G_CHANGED_SW, &my_switch);
 if (ret < 0) 
  {
    fprintf(stderr,"ERROR: problem with switchreading: %d\n",ret);
    return 80;
  }

 return my_switch;

}

//get status of specific switch
unsigned char lisy_usb_get_switch_status( unsigned char number)
{

 int ret;
 unsigned char cmd,status;

      //send cmd
     cmd = LISY_G_STAT_SW;
     if ( write( lisy_usb_serfd,&cmd,1) != 1)
      {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
      }

 //ask APC via serial
 ret = read_byte(number, &status);
 if (ret < 0) 
  {
    fprintf(stderr,"ERROR: problem with switch status reading: %d\n",ret);
  }

 return status;

}

//lamp control
void lisy_usb_lamp_ctrl(int lamp_no,unsigned char action)
{
 uint8_t cmd;

  if (action) cmd=LISY_S_LAMP_ON; else cmd=LISY_S_LAMP_OFF;

      //send cmd
     if ( write( lisy_usb_serfd,&cmd,1) != 1)
        fprintf(stderr,"Lamps Error writing to serial %s\n",strerror(errno));
     //send lamp number
      //send cmd
     if ( write( lisy_usb_serfd,&lamp_no,1) != 1)
        fprintf(stderr,"Lamps Error writing to serial %s\n",strerror(errno));
}

//solenoid control
void lisy_usb_sol_ctrl(int sol_no,unsigned char action)
{
 uint8_t cmd;

  if (action) cmd=LISY_S_SOL_ON; else cmd=LISY_S_SOL_OFF;

      //send cmd
     if ( write( lisy_usb_serfd,&cmd,1) != 1)
        fprintf(stderr,"Solenoids Error writing to serial %s\n",strerror(errno));
     //send lamp number
      //send cmd
     if ( write( lisy_usb_serfd,&sol_no,1) != 1)
        fprintf(stderr,"Solenoids Error writing to serial %s\n",strerror(errno));


}

