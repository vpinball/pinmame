#ifndef LISY_API_H
#define LISY_API_H

#define LISY_API_VERSION_STR	"0.08"

//info, parameter none
#define	LISY_G_HW		0	//get connected LISY hardware -	return "LISY1" or "LISY80"
#define	LISY_G_LISY_VER		1  	//get LISY Version - return String
#define	LISY_G_API_VER		2 	//get API Version - return String
#define	LISY_G_NO_LAMPS		3	//get number of lamps - return byte
#define	LISY_G_NO_SOL		4 	//get number of soleneoids - return byte
#define	LISY_G_NO_SOUNDS	5	//get number of sounds - return byte
#define	LISY_G_NO_DISP		6	//get number of displays - return byte
#define	LISY_G_DISP_DETAIL	7	//get display details - return string TBD
#define	LISY_G_GAME_INFO	8	//get game info - return string 'Gottlieb internal number/char'
#define	LISY_G_NO_SW		9	//get number of switches - return byte

//lamps, parameter byte
#define	LISY_G_STAT_LAMP	10	//get status of lamp # - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_S_LAMP_ON		11	//set lamp # to ON - return none
#define	LISY_S_LAMP_OFF		12	//set lamp # to OFF - return none

//solenoids, parameter byte
#define	LISY_G_STAT_SOL		20	//get status of solenoid # - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_S_SOL_ON		21	//set solenoid # to ON - return none
#define	LISY_S_SOL_OFF		22	//set solenoid # to OFF - return none
#define	LISY_S_PULSE_SOL	23	//pulse solenoid# - return none
#define	LISY_S_PULSE_TIME	24	//set pulse time for solenoid# 1-byte integer ( 0 - 255 )

//displays, parameter string
#define	LISY_S_DISP_0		30	//set display 0 (status) to string - return none
#define	LISY_S_DISP_1		31	//set display 1 to string - return none
#define	LISY_S_DISP_2		32	//set display 2 to string - return none
#define	LISY_S_DISP_3		33	//set display 3 to string - return none
#define	LISY_S_DISP_4		34	//set display 4 to string - return none
#define	LISY_S_DISP_5		35	//set display 5 to string - return none
#define	LISY_S_DISP_6		36	//set display 6 to string - return none
#define	LISY_G_DISP_CH		37	//get possible number of characters of display#

//switches, parameter byte/none
#define	LISY_G_STAT_SW		40	//get status of switch# - return byte "0=OFF; 1=ON; 2=Error"
#define	LISY_G_CHANGED_SW	41	//get changed switches - return byte "bit7 is status; 0..6 is number" "127 means no change since last call"

//Sound (Hardware), parameter byte/none
#define	LISY_S_PLAY_SOUND	50	//play sound#  
#define	LISY_S_STOP_SOUND	51	//stop all sounds ( same as play sound 0 )
#define	LISY_S_PLAY_FILE 	52	//play a file (in ./hardware_sounds) - option(1byte) + string 'filename'
#define	LISY_S_TEXT_TO_SPEECH	53	//say the text - option(1byte) + string 'text'
#define	LISY_S_SET_VOLUME	54	//sound volume in percent

//general, parameter none
#define	LISY_INIT		100	//init/reset LISY - return byte 0=OK, >0 Errornumber Errornumbers TBD
#define	LISY_WATCHDOG		101	//watchdog - return byte 0=OK, >0 Errornumber Errornumbers TBD

#endif  /* LISY_API_H */
