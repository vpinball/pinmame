/* i2c_check
   based on i2cdetect
   bontango 11.2016
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


static void help(void)
{
	fprintf(stderr,
		"Usage: i2ccheck I2CBUS ADDRESS\n"
		"  I2CBUS is an an I2C bus name (e.g. /dev/i2c-1)\n"
		"  ADDRESS will be probed\n"
		"  rtuens 1 on success and 0 if no device is detected at ADDRESS\n");
}



int main(int argc, char *argv[])
{
 int address, file, res, ret;
 char *end;

 if (argc != 3) 
 {
  help();
  exit(0);
 } 
 
 file = open(argv[1], O_RDWR);
 if ( file < 0 )
  { 
   printf("could not open bus at %s\n\r",argv[1]);
   return -1;
 }

 address = strtol(argv[2], &end, 0);

 /* Set slave address */
 if (ioctl(file, I2C_SLAVE, address) < 0) {
	fprintf(stderr, "Error: Could not set "
			"address to 0x%02x: %s\n", address,
				strerror(errno));
		return -1;
 }

 printf("check i2c bus %s at address 0x%02x : ",argv[1],address);
 res = i2c_smbus_read_byte(file);
 close(file);

 if (res <0 ) 
  { 
    printf("not OK\n");
    exit(1) ;
  }
 else
  {
    printf("OK\n");
    exit(0);
  }

}
