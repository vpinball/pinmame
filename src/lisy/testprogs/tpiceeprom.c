 /*
  RTH 02.2016
  lisy80NG eeprom testes
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <wiringPi.h>
#include "../hw_lib.h"

//the debug options
//in main prg set in  lisy80.c
ls80dbg_t ls80dbg;
int lisy80_is80B;
//local switch Matrix, we need 9 elements
//as pinmame internal starts with 1
//there is one value per return
unsigned char swMatrix[9] = { 0,0,0,0,0,0,0,0 };
int lisy80_time_to_quit_flag; //not used here

//global var
union three {
    unsigned char byte;
    struct {
    unsigned COIL:6, ACTION:1, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;
    struct {
    unsigned COMMAND:3, SOUNDS:4, IS_CMD:1;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv2;
    } mydata_coil;

union eepromaddr_t {
    unsigned char byte;
    struct {
    unsigned FIRSTHALF:4, SECONDHALF:4;
    //signed b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
        } bitv;                
    } eepromaddr;

#define LS80COILCMD_WRITE_EEPROM 5
#define LS80COILCMD_PREP_READ_EEPROM 6
#define LS80COILCMD_READ_EEPROM 7

int main (int argc, char *argv[])
{

  unsigned char data,address;
  int answer;


	// INIT the Hardware
        lisy80_hwlib_init();


   if (strncmp(argv[1],"write",5) == 0 )
   {
    address = atoi(argv[2]);
    data = atoi(argv[3]);
    printf("write Data%d to address %d\n\r",data,address);

    /* build control byte */
    mydata_coil.bitv2.COMMAND = LS80COILCMD_WRITE_EEPROM;
    mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here
    //write to PIC
    lisy80_write_byte_coil_pic( mydata_coil.byte );
    printf("write command done \n\r");

    //wait a bit, PIC migth be slow
    delay(300); //wait 100ms secs

    //first byte is data
    //write to PIC
    lisy80_write_byte_coil_pic( data );
    printf("write data done \n\r");

    //wait a bit, PIC migth be slow
    delay(300); //wait 100ms secs

    //second byte is address
    //write to PIC
    lisy80_write_byte_coil_pic( address );
    printf("write address done \n\r");

   }
   else if (strncmp(argv[1],"read",4) == 0 )
   {
    address = atoi(argv[2]);
    printf("read Data from address %d\n\r",address);

    /* build control byte one */
    mydata_coil.bitv2.COMMAND = LS80COILCMD_PREP_READ_EEPROM;
    mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv2.SOUNDS = eepromaddr.bitv.FIRSTHALF;
    //write to PIC
    lisy80_write_byte_coil_pic( mydata_coil.byte );

    /* build control byte two */
    mydata_coil.bitv2.COMMAND = LS80COILCMD_READ_EEPROM;
    mydata_coil.bitv2.IS_CMD = 1;        //we are sending a command here
    mydata_coil.bitv2.SOUNDS = eepromaddr.bitv.SECONDHALF;
    //write to PIC
    lisy80_write_byte_coil_pic( mydata_coil.byte );

    //now read the result
    answer = lisy80_read_byte_coil_pic();

    printf("answer: %d\n\r",answer);

   }
   else
   {
    printf("Use: <prg> write|read adress [data]\n\r");
    return 1;
   }






  return 0;

}//main
