 /*
  bontango 11.2015
  lisy80NG first tests
  switches 0.01
*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include "fileio.h"
#include "hw_lib.h"
#include "switches.h"
#include "utils.h"
#include "fadecandy.h"
#include "lisy_home.h"
#include "externals.h"
#include "lisy.h"

extern unsigned char swMatrixLISY35[9];

void lisy80_debug_switches (unsigned char data)
{

switchdata_t mydata;

mydata.byte = data;
unsigned char strobe, returnval, action;
//int n;
int switch_no;



//debug
if( ls80dbg.bitv.switches )
 {
  strobe = mydata.bitv.STROBE;
  returnval = mydata.bitv.RETURN;
  action = mydata.bitv.ONOFF;
   if ( lisy_hardware_revision == 100 )  //switch numbering for system1
   {
	switch_no = returnval * 10 + strobe;
        sprintf(debugbuf,"LISY1 Switch_reader: Switch%d, action:%d",switch_no,action);
        lisy80_debug(debugbuf);
   } 
   else if ( ( lisy_hardware_revision == 350 ) || ( lisy_hardware_revision == LISY_HW_LISY_H_SS ))  //switch numbering for Bally
   {
	switch_no = 1 + returnval + strobe * 8;
        sprintf(debugbuf,"LISY35 Switch_reader: Switch%d, action:%d",switch_no,action);
        lisy80_debug(debugbuf);
   }
   else  //switch numbering for System80
   {
	switch_no = returnval + strobe *10;
        sprintf(debugbuf,"LISY80 Switch_reader: Switch%d, action:%d",switch_no,action);
        lisy80_debug(debugbuf);
   } 
	//replay add on
	lisy80_debug_swreplay( switch_no, action);
 }


/*
 //print new setting out
 //fprintf(stderr,"LISY80 DEBUG_SWITCHES (Strobe:%d Return:%d) set to %d ",strobe,returnval,action);

 //check if the routine wants to (un)set a switch which is already (un)set
 //each value of the swMatrix represents a return value, we start with 1 rather then 0 here
 n = swMatrix[returnval + 1];
 //each bit represents a switch, here we start with zero
 n >>= strobe;

 //now compare new value with old value, remember that action is inverse
 if (n & 1) {
//     fprintf(stderr,"want to set to 1; ");
     if ( action == 0)  
          fprintf(stderr,"want to set to 0; old value was 0 ERROR!\n\r");
//     else
//          fprintf(stderr,"old value was 0 OK\n\r");
   }
 else {
//     fprintf(stderr,"want to set to 0; ");
     if ( action == 1)  
          fprintf(stderr,"want to set to 1; old value was 1 ERROR!\n\r");
//     else
//          fprintf(stderr,"old value was 1 OK\n\r");
   }
*/

}


//monitor switches is called only from Tester.c!
void monitor_switches(void)
{

  int ret;
  unsigned char action;
  unsigned char strobe,returnval;
  switchdata_t mydata;

  fprintf(stderr,"start receive loop, wait for buffer not: empty signal (pi7==1) from slave\n ");
  fprintf(stderr,"will loop until test switch on System80 (no: 7) or on System1 (no: 0)  is pressed\n");

	do
	{
	 if ( lisy_hardware_revision == 100 )
           ret = lisy1_switch_reader( &action );
	 else if ( lisy_hardware_revision == 350 )
           ret = lisy35_switch_reader( &action );
	 else
           ret = lisy80_switch_reader( &action );

	 if (ret <80) fprintf(stderr,"switch reader returns action:%d for switch:%d\n\r",action,ret);

	 if (ret < 80)
         {
         strobe = ret / 10;
         returnval = ret % 10;


	//prepare data for debug
	mydata.bitv.RETURN = returnval;
	mydata.bitv.STROBE = strobe;
	action = mydata.bitv.ONOFF = action;
	lisy80_debug_switches(mydata.byte);

 //set the bit in the Matrix var according to action
  // action 0 means set the bit
  // any other means delete the bit
  if (action == 0) //set bit
    swMatrix[returnval+1] |= ( 1 << strobe );
  else  //delete bit
    swMatrix[returnval+1] &= ~( 1 << strobe );

	 }

	 if ( ( ( action == 1) && ( ret == 7) ) || ( ( action == 1) && ( ret == 0) ) ) break;


	}while(1);
}


//read lisy35 switchmatrix
//one try without delay
// use with care! in order not to stress switch PIC
//gives back status read ( 80 == no change)
int lisy35_switchmatrix_read(void )
{
 int ret;
 unsigned char strobe,returnval;
 unsigned char action = 1;

     ret = lisy35_switch_reader( &action );

     if (ret < 80) {
         //ret is switchnumber: NOTE: Bally  8*6==48 switches in maximum, counting 01..48
         //starship is using all 64 switches
        //Switches_LISY35[ret] = action;

        //calculate strobe & return
        //Note: this is different from system80
        strobe = ret / 8;
        returnval = ret % 8;

        //set the bit in the Matrix var according to action
        // action 1 means set the bit
        // any other means delete the bit
        if (action ) //set bit
                   SET_BIT(swMatrixLISY35[strobe+1],returnval);
        else  //delete bit
                   CLEAR_BIT(swMatrixLISY35[strobe+1],returnval);

        }
 return(ret);
}



//update lisy35 switchmatrix
//used for zero detection in wheels for Starship
//has 20ms delay in minimum
void lisy35_switchmatrix_update(int mydelay )
{
 int ret;
 unsigned char strobe,returnval;
 unsigned char action = 1;

 do
 {
     delay(mydelay); // millisecond delay from wiringpi library
                     // for giving PIC some time to send switchcodes
     ret = lisy35_switch_reader( &action );

     if (ret < 80) {
         //ret is switchnumber: NOTE: Bally  8*6==48 switches in maximum, counting 01..48
         //starship is using all 64 switches
        //Switches_LISY35[ret] = action;

        //calculate strobe & return
        //Note: this is different from system80
        strobe = ret / 8;
        returnval = ret % 8;

        //set the bit in the Matrix var according to action
        // action 1 means set the bit
        // any other means delete the bit
        if (action ) //set bit
                   SET_BIT(swMatrixLISY35[strobe+1],returnval);
        else  //delete bit
                   CLEAR_BIT(swMatrixLISY35[strobe+1],returnval);

        }
 } while ( ret < 80);

}

