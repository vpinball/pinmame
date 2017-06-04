#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wiringPi.h>

// pot_step: original from
// http://www.allaboutcircuits.com/projects/building-raspberry-pi-controllers-part-5-reading-analog-data-with-an-rpi/

//GPIO 4 (7)
#define A_PIN     7
//GPIO 22 (3)
#define B_PIN     3



//discharge function for reading capacitor data
void discharge(void)
{
  pinMode (A_PIN, INPUT) ;
  pinMode (B_PIN, OUTPUT) ;
  digitalWrite (B_PIN, LOW) ;
  //delay(5);  //5 ms
  delay(10);  //10 ms
}

// time function for capturing analog count value
int  charge_time(void)
{
  int count = 0;

  pinMode (B_PIN, INPUT) ;
  pinMode (A_PIN, OUTPUT) ;

  digitalWrite (A_PIN, HIGH) ; 
  while ( digitalRead(B_PIN) == LOW) count++;

  return count;

}


int main (void)
{

  long position;
  long diff;
  long oldpos,i;

  printf ("Raspberry Pi pot_step test\n") ;

  wiringPiSetup () ;

   //first time discharge
   discharge();
   oldpos = charge_time();

  for (;;)
  {

   position = 0;
   for (i=1; i<=100; i++)
   {
   discharge();
     position = position + charge_time();
   }

     position = position  / 100;


   //range is from appr. 600 to 6300
   //so result will be in between 0 and 23
//   volume = position / 250 -2;

   diff = labs( oldpos - position) ;
   printf("diff:%ld\n",diff);

   if ( diff > 250 ) printf("oldpos:%ld - postition:%ld\n",oldpos,position);
   sleep(1);
  }
  return 0 ;
}

