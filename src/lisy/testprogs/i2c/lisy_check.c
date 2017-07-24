/* lisy_check
   based on i2cdetect
   bontango 03.2017
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>


/*
  Hardware Version 3.11 has eeprom at I2C address 0x50 & 0x51
  Hardware Version 3.21 has eeprom at I2C address 0x54 & 0x55
  Hardware Version for SYstem1 has eeprom at I2C address 0x52 & 0x53
  we probe all three, starting with 0x54 for 3.21
  return is:
  0 -> no lisy80 detected
  1 -> lisy80 HW Version 3.11
  2 -> lisy80 HW Version 3.21
  3 -> lisy1 baed on Lisy80 HW Version 3.21
 -1 -> on error

  use option -v (or anything else as second argument) for being verbose testing
*/


int main(int argc, char *argv[])
{
 int address, file, res, ret;
 int verbose = 0;
 char *end;

 if (argc >= 2) 
 {
  verbose = 1;
  printf("we are in verbose mode\n");
 } 
 
 file = open("/dev/i2c-1", O_RDWR);
 if ( file < 0 )
  { 
   printf("could not open bus at /dev/i2c-1\n");
   return -1;
 }

 //we start with probing for HW 3.21 -> 0x54
 address = 0x54;
 /* Set slave address */
 if (ioctl(file, I2C_SLAVE, address) < 0) {
	fprintf(stderr, "Error: Could not set "
			"address to 0x%02x: %s\n", address,
				strerror(errno));
		return -1;
 }

 if (verbose) printf("check i2c bus /dev/i2c-1 at address 0x54\n");
 res = i2c_smbus_read_byte(file);
 if (res >= 0 ) 
  { 
    if (verbose) printf("HW 3.21 detected\n");
    close(file);
    exit(2) ;
  }

 //now probe for HW 3.11 -> 0x50
 address = 0x50;
 /* Set slave address */
 if (ioctl(file, I2C_SLAVE, address) < 0) {
	fprintf(stderr, "Error: Could not set "
			"address to 0x%02x: %s\n", address,
				strerror(errno));
		return -1;
 }

 if (verbose) printf("check i2c bus /dev/i2c-1 at address 0x50\n");
 res = i2c_smbus_read_byte(file);
 if (res >= 0 ) 
  { 
    if (verbose) printf("HW 3.11 detected\n");
    close(file);
    exit(1) ;
  }

 //now probe for System1 -> 0x52
 address = 0x52;
 /* Set slave address */
 if (ioctl(file, I2C_SLAVE, address) < 0) {
	fprintf(stderr, "Error: Could not set "
			"address to 0x%02x: %s\n", address,
				strerror(errno));
		return -1;
 }

 if (verbose) printf("check i2c bus /dev/i2c-1 at address 0x52\n");
 res = i2c_smbus_read_byte(file);
 if (res >= 0 ) 
  { 
    if (verbose) printf("LISY1 detected\n");
    close(file);
    exit(3) ;
  }

 //OK, looks like lisy80 Hardware is not connected
 fprintf(stderr,"Could not find LISY80/LISY1 Hardware, please contact bontango at www.lisy80.com to buy it ;-)\n");
 close(file);
 exit(0) ;

}
