/*
 BALL_SAVE.c
 Mai 2022
 bontango
*/

#include <stdio.h>
#include "ball_save.h"
#include "utils.h"

typedef enum
{
    LISY1_BS_IDLE,  //wait for game running
    LISY1_BS_WAIT,  //wait for switch activity to switch to ACTIV state
    LISY1_BS_ACTIV,
    LISY1_BS_WANT_SAVE,
    LISY1_BS_IS_SAVED,
    LISY1_BS_TIMEOUT
} lisy1_bs_state_t;

//the Ball Save eventhandler
int lisy_ball_save_event_handler( int id, int arg1, int arg2)
{

    int returnval;
    static int timer_count = 0;
    static lisy1_bs_state_t lisy1_bs_state = LISY1_BS_IDLE;

	//default return
        returnval = 0;
    switch(id)
        {
         case LISY_BALL_SAVE_EVENT_TIMER: 
		//only each second
		if ( timer_count++ < 300) return 0;
		  else timer_count = 0;
		break;
         case LISY_BALL_SAVE_EVENT_GAME_OVER: 
		printf("game over event: action:%d\n",arg1);
		if (arg1 == 0) lisy1_bs_state = LISY1_BS_IDLE;
		if (arg1 == 1) lisy1_bs_state = LISY1_BS_WAIT;
		break;
         case LISY_BALL_SAVE_EVENT_SWITCH: 
		printf("switch event: sw:%d action:%d\n",arg1,arg2);
		//outhole == 66
		switch(lisy1_bs_state)
		{
			case LISY1_BS_WAIT:
		 		if ( arg1 != 66 ) 
		 		{
		  		//start timer
		  		lisy_timer( 0, 1,4); //reset timer 4
		  		lisy_timer( 10000, 0,4); //start timer 4 10 seconds
		  		lisy1_bs_state = LISY1_BS_ACTIV;
		 		}
			break;
			case LISY1_BS_ACTIV: //activ state, we save the ball, switch to ignore by pinmame
		 		if ( arg1 == 66 )
					{ 
					returnval = 1;
					if ( arg2 == 0)	 lisy1_bs_state = LISY1_BS_WANT_SAVE; //outhole closed
					}

			break;
			case LISY1_BS_WANT_SAVE: //we save the ball, wait until outhole is empty, switch to ignore by pinmame
		 		if ( arg1 == 66 )
					{ 
					returnval = 1;
					if ( arg2 == 1)	 lisy1_bs_state = LISY1_BS_IS_SAVED; //outhole open
					}
			break;
			
			default:
			break;
		} //switch state
		break;
        }


	//state maschine
	switch(lisy1_bs_state)
	{
	  case LISY1_BS_IDLE:
		printf("ball Save Idle State\n");
		break;
	  case LISY1_BS_WAIT:
		printf("ball Save wait State\n");
		break;
	  case LISY1_BS_ACTIV:
		printf("ball Save Activ State\n");
		if ( lisy_timer( 0, 0,4) ) lisy1_bs_state = LISY1_BS_TIMEOUT;
		break;
	  case LISY1_BS_TIMEOUT:
		printf("ball Save timeout State\n");
		break;
	  case LISY1_BS_WANT_SAVE:
		printf("ball Save want SAVE State\n");
		break;
	  case LISY1_BS_IS_SAVED:
		printf("ball Save IS SAVE State\n");
		break;
	}

  return(returnval);
}

