/*
 USB and serial routines for LISY
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
#include "lisy_home.h"
#include "externals.h"
#include "usbserial.h"


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


#define ARDUINO_NO_TRIES 9
int lisy_usb_init()
{

   int i,j,n,ret,tries;
   int lisy_usb_serfd;
   char *portname = "/dev/ttyACM0";
   unsigned char data,cmd;
   char answer[80];

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

    fprintf(stderr,"LISY_Mini: HW Client is: %s \n",answer);

  return lisy_usb_serfd;
}

int lisy_serial_init()
{

   int i,j,n,ret,tries;
   int lisy_serfd;
   char *portname = "/dev/serial0";
   unsigned char data,cmd;
   char answer[80];

    //open interface
    lisy_serfd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (lisy_serfd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    /*baudrate 115200, 8 bits, no parity, 1 stop bit */
    set_interface_attribs(lisy_serfd, B115200);


    //start with asking for Hardware string
    //for init we reapeat that up to ten times in case
    //the other side ( Arduino) needs time to resset
    //set the command first
    cmd = LISY_G_HW;
    //and flush buffer
    sleep(1); //need to wait a bit beofre doing that
    tcflush(lisy_serfd,TCIOFLUSH);
    //now send and try to read answer (one byte)
    ret = tries = 0;
    do
    {
	//send cmd
     if ( write( lisy_serfd,&cmd,1) != 1)
      {
        printf("Error writing to serial %s\n",strerror(errno));
        return -1;
      }

      //read answer
	if ( ( ret = read(lisy_serfd,&data,1)) != 1)
         {
	   tries++; //count tries
	   sleep(1); //wait a second
	   tcflush(lisy_serfd,TCIOFLUSH); //flush buffers
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
    if ( ( ret = read(lisy_serfd,&data,1)) != 1)
    {
        printf("Error reading from serial, return:%d %s\n",ret,strerror(errno));
        return -1;
    }
    answer[n] = data;
    n++;
    } while (( data != '\0') & ( n < 10));

    fprintf(stderr,"LISY_Mini: HW Client is: %s \n",answer);

  return lisy_serfd;
}

