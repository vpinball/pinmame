/*
This program is hereby placed into the public domain.
Of course the program is provided without warranty of any kind.

Originally Downloaded from http://www.lm-sensors.org/browser/i2c-tools/trunk/eepromer/eeprom.c
Downloaded for lisy80 from http://www.gallot.be/?p=180

Modified for lisy80 bontango April 2016
added pic eeprom routines bontango January 2018

*/
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <wiringPi.h>
#include "utils.h"
#include "eeprom.h"
#include "fileio.h"
#include "hw_lib.h"
#include "displays.h"
#include "coils.h"
#include "fadecandy.h"
#include "lisy_home.h"
#include "externals.h"

#define MAX_EEPROM_WRITE_RETRIES 30

//global blockbuffer for eeprom content, used to optimize read/write
// read is done always from buffer (except first time)
// write is done to buffer AND to eeprom in case content changed
// used only for eeprom of pic, as 24c04 does block write anyway
unsigned char lisy_eeprom_buf_block0[256];
unsigned char lisy_eeprom_buf_block1[256];


/* write len bytes (stored in buf) to eeprom at address addr, page-offset offset */
/* if len=0 (buf may be NULL in this case) you can reposition the eeprom's read-pointer */
/* return 0 on success, -1 on failure */
int eeprom_write(int fd,
		 unsigned int addr,
		 unsigned int offset,
		 char *buf,
		 unsigned char len
){
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg             i2cmsg;
	int i;
	char _buf[MAX_BYTES + 1];

	if(len>MAX_BYTES){
	    fprintf(stderr,"I can only write MAX_BYTES bytes at a time!\n");
	    return -1;
	}

	if(len+offset >256){
	    fprintf(stderr,"Sorry, len(%d)+offset(%d) > 256 (page boundary)\n",
			len,offset);
	    return -1;
	}

	_buf[0]=offset;    /* _buf[0] is the offset into the eeprom page! */
	for(i=0;i<len;i++) /* copy buf[0..n] -> _buf[1..n+1] */
	    _buf[1+i]=buf[i];

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr  = addr;
	i2cmsg.flags = 0;
	i2cmsg.len   = 1+len;
	i2cmsg.buf   = _buf;

	if((i=ioctl(fd,I2C_RDWR,&msg_rdwr))<0){
	    if ( len > 0 ) {
	      perror("ioctl()");
	      fprintf(stderr,"ioctl write returned %d\n",i);
            }
	    return -1;
	}

	return 0;
}

/* read len bytes stored in eeprom at address addr, offset offset in array buf */
/* return -1 on error, 0 on success */
int eeprom_read(int fd,
		 unsigned int addr,
		 unsigned int offset,
		 char *buf,
		 unsigned char len
){
	struct i2c_rdwr_ioctl_data msg_rdwr;
	struct i2c_msg             i2cmsg;
	int i;

	if(len>MAX_BYTES){
	    fprintf(stderr,"I can only write MAX_BYTES bytes at a time!\n");
	    return -1;
	}

	if(eeprom_write(fd,addr,offset,NULL,0)<0)
	    return -1;

	msg_rdwr.msgs = &i2cmsg;
	msg_rdwr.nmsgs = 1;

	i2cmsg.addr  = addr;
	i2cmsg.flags = I2C_M_RD;
	i2cmsg.len   = len;
	i2cmsg.buf   = buf;

	if((i=ioctl(fd,I2C_RDWR,&msg_rdwr))<0){
	    perror("ioctl()");
	    fprintf(stderr,"ioctl read returned %d\n",i);
	    return -1;
	}

	return 0;
}



//write buffer of 256 bytes to eeprom to block 0 or 1
//for 24c04 only block 0 or 1 is possible
//0 on success, <0 on error
int lisy_eeprom_256byte_write_24c04( char *wbuf, int block)
{

 int j;
 /* filedescriptor and name of device */
 int d;
 unsigned int addr;
 char *dn=LISY80_EEPROM_I2C_BUS;
 int wait = 0;
 int acked = 0;
 int adr1,adr2;

 if ( lisy_hardware_revision == 311 )
    { adr1 = LISY80_HW311_EEPROM_ADDR1; adr2 = LISY80_HW311_EEPROM_ADDR2; }
 else if ( lisy_hardware_revision == 100 )
    { adr1 = LISY1_EEPROM_ADDR1; adr2 = LISY1_EEPROM_ADDR2; }
 else
    { adr1 = LISY80_HW320_EEPROM_ADDR1; adr2 = LISY80_HW320_EEPROM_ADDR2; }

 if (block == 1) addr = adr2;
  else addr = adr1;
 
    if((d=open(dn,O_RDWR))<0){
	fprintf(stderr,"Could not open i2c at %s\n",dn);
	return(-1);
    }

            for(j=0;j<(BYTES_PER_PAGE/MAX_BYTES);j++) {
		if(eeprom_write(d,addr,j*MAX_BYTES,wbuf+(j*MAX_BYTES),MAX_BYTES)<0)
		    return(-1);
                //fprintf(stderr,".");
                for ( wait = 0; wait < MAX_EEPROM_WRITE_RETRIES; wait ++ ) {
                  acked = eeprom_write(d,addr,j*MAX_BYTES,NULL,0);
                  if ( acked == 0 )
                    break;
                  //fprintf(stderr,".");
                  usleep( 100 );
                }
                if ( acked != 0 ) {
                  fprintf(stderr,"Ack of write operation timedout");
                  return ( -1 );
                }
                //fprintf(stderr," acked \n");
            }
 close(d);
 return(0);
} //256byte_write


//write buffer of 256 bytes to eeprom at pic
//0 on success, <0 on error
int lisy_eeprom_256byte_write_pic( char *wbuf, int block)
{
  int i,bytes_written;

  unsigned char *old_eeprom_content;


  //assign the rigth block
  if ( block == 0) old_eeprom_content = lisy_eeprom_buf_block0;
   else old_eeprom_content = lisy_eeprom_buf_block1;

  //check if the byte has changed, and if so write to eeprom
  bytes_written = 0;
  for ( i=0; i<=255; i++)
  {
   //has it changed?
   if ( *wbuf != *old_eeprom_content )
     {
	 //yes, change it
         //write number of writes in debug
         if (ls80dbg.bitv.basic == 1 )
         {
          sprintf(debugbuf,"eeprom: write to PIC: Position:%d changed from %d to %d",i,*old_eeprom_content,*wbuf);
          lisy80_debug(debugbuf);
         }
	 lisy_eeprom_1byte_write( i, *wbuf, block);
         *old_eeprom_content = *wbuf;
	 bytes_written++;
     }
	 //next byte to compare
         old_eeprom_content++;
         wbuf++;
  }


 return 0;
}


int lisy_eeprom_256byte_write( char *wbuf, int block)
{

 if (lisy_has24c04)
   return ( lisy_eeprom_256byte_write_24c04( wbuf, block) );
 else
   return ( lisy_eeprom_256byte_write_pic( wbuf, block) );

}



//read buffer of 256 bytes from eeprom block 0 or 1 into buffer
//for 24c04 only block 0 or 1 is possible
//0 on success, <0 on error
int lisy_eeprom_256byte_read_24c04( char *rbuf, int block)
{

 int j;
/* filedescriptor and name of device */
    int d;
    char *dn=LISY80_EEPROM_I2C_BUS;
    unsigned int addr;
 int adr1,adr2;

 if ( lisy_hardware_revision == 311 )
    { adr1 = LISY80_HW311_EEPROM_ADDR1; adr2 = LISY80_HW311_EEPROM_ADDR2; }
 else if ( lisy_hardware_revision == 100 )
    { adr1 = LISY1_EEPROM_ADDR1; adr2 = LISY1_EEPROM_ADDR2; }
 else
    { adr1 = LISY80_HW320_EEPROM_ADDR1; adr2 = LISY80_HW320_EEPROM_ADDR2; }


 if (block == 1) addr = adr2;
  else addr = adr1;

    if((d=open(dn,O_RDWR))<0){
	fprintf(stderr,"Could not open i2c at %s\n",dn);
	perror(dn);
	exit(1);
    }

            for(j=0;j<(BYTES_PER_PAGE/MAX_BYTES);j++)
		if(eeprom_read(d,addr,j*MAX_BYTES,rbuf+(j*MAX_BYTES),MAX_BYTES)<0)
		    return(-1);

 close(d);
 return(0);
} //256 byte read


//read buffer of 256 bytes from eeprom of pic into buffer
//0 on success, <0 on error
int lisy_eeprom_256byte_read_pic( char *rbuf, int block)
{
 int i;
 static unsigned char first_time = 1;

 //need to read content into bufer?
 if ( first_time )
  {
   first_time = 0;

   //read both blocks into buffer
   for ( i=0; i<=255; i++)
    {
      lisy_eeprom_buf_block0[i] = lisy_eeprom_1byte_read ( i, 0);
      lisy_eeprom_buf_block1[i] = lisy_eeprom_1byte_read ( i, 1);
    }



  } //only first time

  //fill rbuf from buffer
  if ( block == 0)
  {
   for ( i=0; i<=255; i++) { *rbuf = lisy_eeprom_buf_block0[i]; rbuf++; };
  }
  else
  {
   for ( i=0; i<=255; i++) { *rbuf = lisy_eeprom_buf_block1[i]; rbuf++; };
  }

 return 0;

}


int lisy_eeprom_256byte_read( char *rbuf, int block)
{

 if (lisy_has24c04)
  return ( lisy_eeprom_256byte_read_24c04( rbuf, block) );
 else
  return ( lisy_eeprom_256byte_read_pic( rbuf, block) );

}

//initialize the content the second blockof the eeprom
//returns 0 on success
int lisy_eeprom_init(void)
{

 int i;
 eeprom_block_t myblock;

 strcpy(myblock.content.signature,"LISY80 by bontango - www.lisy80.com");
 myblock.content.gamenr = 80;  //non existing game nr at init
 myblock.content.starts = 0;
 myblock.content.debugs = 0;
 for(i=0; i<=63; i++) myblock.content.counts[i] = 0;
 myblock.content.Software_Main = 0;
 myblock.content.Software_Sub = 0;

 return(  lisy_eeprom_256byte_write ( myblock.byte, 1));

}


//check signature of second block of eeprom
// returns 1 if signature is OK
//-1 in case of errror
int lisy_eeprom_checksignature(void)
{

 eeprom_block_t myblock;

 //read second block
 if ( lisy_eeprom_256byte_read( myblock.byte, 1) != 0)
   return -1;

 if (strncmp(myblock.content.signature,"LISY80 by bontango - www.lisy80.com",35) == 0)
   return 1;
 else return 0;


}

//print out the content of the second blockof the eeprom
eeprom_block_t lisy_eeprom_getstats(void)
{
 eeprom_block_t myblock;

 //read second block
 lisy_eeprom_256byte_read ( myblock.byte, 1);

 return myblock;

}

//print out the content of the second block of the eeprom
//in case signature is valid
int lisy_eeprom_printstats(void)
{

 int i;
 eeprom_block_t myblock;

 //is the signature valid?
 if ( !lisy_eeprom_checksignature())
 {
   fprintf(stderr, "no valid signature in content of second block of eeprom\n\r");
 }
 else
 {

 //read second block
 if ( lisy_eeprom_256byte_read ( myblock.byte, 1) != 0)
   return -1;


 fprintf(stderr, "Content of second block of eeprom\n\r");
 fprintf(stderr, "Game number: %d\n\r",myblock.content.gamenr);
 fprintf(stderr, "Number of starts: %d\n\r",myblock.content.starts);
 fprintf(stderr, "Number of starts with debug: %d\n\r",myblock.content.debugs);
 fprintf(stderr, "Number of starts, game specific if >0\n\r");
 for(i=0; i<=63; i++)
   {
     if(myblock.content.counts[i] > 0)
       fprintf(stderr, "Number of starts Game No(%d): %d\n\r",i,myblock.content.counts[i]);
   }
 fprintf(stderr, "Software version last used: %d.%03d\n\r",myblock.content.Software_Main,myblock.content.Software_Sub);
 }//valid signature

return 0;
}

//read one byte from PIC eeprom
//block 0 means displaypic, block one coilpic
int lisy_eeprom_1byte_read( unsigned char address, int block)
{

  char buf[2];

  union eeprom_two {
    unsigned char byte;
    struct {
    unsigned COMMAND_BYTE:7, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    struct {
    unsigned COMMAND:3, SOUNDS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;
    } eeprom_cmd;

 if ( block == 0) //display pic
 {
  eeprom_cmd.bitv.IS_CMD = 1;        //we are sending a command here
  eeprom_cmd.bitv.COMMAND_BYTE = LS80DPCMD_READ_EEPROM;

  buf[0] = eeprom_cmd.byte;
  buf[1] = address;

  //write data to PIC
  lisy80_write_multibyte_disp_pic( buf, 2 );

  //wait a bit, PIC migth be slow
  delay(10); //wait 10ms secs

  //read answer and send back
  return( lisy80_read_byte_disp_pic());
 }
 else //coil pic
 {
  eeprom_cmd.bitv2.IS_CMD = 1;        //we are sending a command here
  eeprom_cmd.bitv2.COMMAND = LS80COILCMD_EXT_CMD_ID;
  eeprom_cmd.bitv2.SOUNDS = LISY_EXT_CMD_EEPROM_READ;

  buf[0] = eeprom_cmd.byte;
  buf[1] = address;

  //write data to PIC
  lisy80_write_multibyte_coil_pic( buf, 2 );

  //wait a bit, PIC migth be slow
  delay(10); //wait 10ms secs

  //read answer and send back
  return( lisy80_read_byte_coil_pic());
 }

}

//write one byte to PIC eeprom
//block 0 means displaypic, block one coilpic
int lisy_eeprom_1byte_write( unsigned char address, unsigned char data,  int block)
{

  char buf[3];

  union eeprom_two {
    unsigned char byte;
    struct {
    unsigned COMMAND_BYTE:7, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    struct {
    unsigned COMMAND:3, SOUNDS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;
    } eeprom_cmd;

 if ( block == 0) //display pic
 {
  eeprom_cmd.bitv.IS_CMD = 1;        //we are sending a command here
  eeprom_cmd.bitv.COMMAND_BYTE = LS80DPCMD_WRITE_EEPROM; 


  buf[0] = eeprom_cmd.byte;
  buf[1] = address;
  buf[2] = data;

  //write data to PIC
  return ( lisy80_write_multibyte_disp_pic( buf, 3 ));
 }
 else //coil pic
 {
  eeprom_cmd.bitv2.IS_CMD = 1;        //we are sending a command here
  eeprom_cmd.bitv2.COMMAND = LS80COILCMD_EXT_CMD_ID; 
  eeprom_cmd.bitv2.SOUNDS = LISY_EXT_CMD_EEPROM_WRITE;

  buf[0] = eeprom_cmd.byte;
  buf[1] = address;
  buf[2] = data;

  //write data to PIC
  return ( lisy80_write_multibyte_coil_pic( buf, 3 ));
 }
}

