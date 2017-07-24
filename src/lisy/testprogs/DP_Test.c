 /*
  RTH 11.2015
  lisy80NG first tests
  switches 0.01
*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define PIC_ADDR 0x10   // Address of PIC, shifted right one bit ( org is 0x33 )

int main (int argc, char *argv[])
{
	int fd;

 union both3 {
    unsigned char byte;
    struct {
    unsigned DISPLAY:3, DIGIT:3, FREE:1, COMMAND:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    } mydata;

/* build control byte */
        mydata.bitv.DISPLAY = 4;	//comand send version
        mydata.bitv.DIGIT = 0;
        mydata.bitv.FREE = 0;           //reserved for future use
        mydata.bitv.COMMAND = 1;        //we are sending commands here



        wiringPiSetup () ;

        fd=wiringPiI2CSetup (PIC_ADDR) ; 
        if(fd==-1)
        {
                printf("Can't setup the I2C device\n");
                return -1;
        }
                printf("successfull setup of the I2C device at address %x\n",PIC_ADDR);


	printf("ask for version \n ");
	wiringPiI2CWrite (fd, mydata.byte) ;
	//read answer
//	sleep(1); //wait for data available
	 mydata.byte=wiringPiI2CRead(fd);
                        if(mydata.byte==-1) printf("No data\n"); 
	 printf("return:%d \n",mydata.byte);

return 1;
}//main
