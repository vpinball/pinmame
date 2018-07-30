/*******************************************************************************
 Harley Davidson (Sega/Stern) Pinball Simulator

 by Steve Ellenoff and Brian Smith
 Jul 14, 2011

 NOTES:

 Thanks to Destruk for sorting out the mess with the clones, since I couldn't
 manage it... brain too rusty! Haven't written a sim for PinMAME since, what
 the year, 2000 or so?

 Code was a rush job, so don't judge harshly... 
 
 ******************************************************************************/

/*------------------------------------------------------------------------------
  Keys for The Harley Davidson Simulator:
  ------------------------------------
    +I  L/R Inlane
    +O  L/R Outlane
    +-  L/R Slingshot
	+L	L/R Loop Shots
     Q  SDTM (Drain Ball)
	 M  Motorcycle
	 R  Ramp
	 T  Traffic Light
	 N  Next City Hole / Scoop
	 B  Jet Bumper Area
	 W  Moto Stand Up #1
	 E  Moto Stand Up #2
	 Y  Ramp Stand Up #1
	 U  Ramp Stand Up #2
	 A  L Drop Target
	 S  I Drop Target
	 D  V Drop Target
	 F  E Drop Target
	 G  R Drop Target
	 H  I Drop Target
	 J  D Drop Target
	 K  E Drop Target
	 X  Spinner
    

------------------------------------------------------------------------------*/

#include "driver.h"
#include "core.h"
#include "sim.h"
#include "vpintf.h"
#include "se.h"
#include "dedmd.h"
#include "desound.h"
#include <time.h>

/*------------------
/  Local functions
/-------------------*/
static int  harley_handleBallState(sim_tBallStatus *ball, int *inports);
static void harley_drawStatic(BMTYPE **line);
static void init_harl(void);
static void harley_drawMech(BMTYPE **line);
static void harley_handleMech(int mech);


//motor cycle tracking
#define MOTO_UP			50
#define MOTO_DOWN		0
enum{
	MOTO_DIR_NONE,
	MOTO_DIR_UP,
	MOTO_DIR_DOWN
};

/*-----------------------
  local static variables
 ------------------------*/
static struct {
  int motopos;
  int motodir;
} locals;

/*--------------------------
/ Game specific input ports
/---------------------------*/
SE_INPUT_PORTS_START(harl,4)
  PORT_START /* 0 */
    COREPORT_BIT(0x0001,"Left Qualifier",	KEYCODE_LCONTROL)
    COREPORT_BIT(0x0002,"Right Qualifier",	KEYCODE_RCONTROL)
    COREPORT_BIT(0x0004,"Launch Button",	KEYCODE_ENTER)
    COREPORT_BIT(0x0008,"L/R Outlane",		KEYCODE_O)
    COREPORT_BIT(0x0010,"L/R Slingshot",	KEYCODE_MINUS)
    COREPORT_BIT(0x0020,"L/R Inlane",		KEYCODE_I)
    COREPORT_BIT(0x0040,"L/R Loop",			KEYCODE_L)
    COREPORT_BIT(0x0080,"Traffic Lite",		KEYCODE_T)
    COREPORT_BIT(0x0100,"Ramp",				KEYCODE_R)
	COREPORT_BIT(0x0200,"Moto Target 1",	KEYCODE_W)    
    COREPORT_BIT(0x0400,"Moto Target 2",	KEYCODE_E)
    COREPORT_BIT(0x0800,"Ramp Target 1",	KEYCODE_Y)
    COREPORT_BIT(0x1000,"Ramp Target 2",	KEYCODE_U)
    COREPORT_BIT(0x2000,"Jets",				KEYCODE_B)
    COREPORT_BIT(0x4000,"Drain",			KEYCODE_Q)
    COREPORT_BIT(0x8000,"Next City Scoop",	KEYCODE_N)

  PORT_START /* 1 */
    COREPORT_BIT(0x0001,"L Drop Target",			KEYCODE_A)
    COREPORT_BIT(0x0002,"I Drop Target",			KEYCODE_S)
    COREPORT_BIT(0x0004,"V Drop Target",			KEYCODE_D)
    COREPORT_BIT(0x0008,"E Drop Target",			KEYCODE_F)
    COREPORT_BIT(0x0010,"R Drop Target",			KEYCODE_G)
    COREPORT_BIT(0x0020,"I Drop Target",			KEYCODE_H)
    COREPORT_BIT(0x0040,"D Drop Target",			KEYCODE_J)
    COREPORT_BIT(0x0080,"E Drop Target",			KEYCODE_K)
    COREPORT_BIT(0x0100,"Motorcycle",				KEYCODE_M)
    COREPORT_BIT(0x0200,"Spinner",					KEYCODE_X)
    COREPORT_BIT(0x0400,"",							KEYCODE_C)
    COREPORT_BIT(0x0800,"",							KEYCODE_V)
    COREPORT_BIT(0x1000,"",							KEYCODE_SLASH)
    COREPORT_BIT(0x2000,"",							KEYCODE_Z)
    COREPORT_BIT(0x4000,"",							KEYCODE_BACKSPACE)
    COREPORT_BIT(0x8000,"",							KEYCODE_COMMA)
SE_INPUT_PORTS_END

//SPECIFIC SWITCH #
#define HARLEY_PF_SW_LT_BUTTON			01
#define HARLEY_PF_SW_RT_BUTTON			08
//#09 - Not Used
//#10 - Not Used

//common
#define HARLEY_PF_SW_TROUGH1		11
#define HARLEY_PF_SW_TROUGH2		12
#define HARLEY_PF_SW_TROUGH3		13
#define HARLEY_PF_SW_TROUGH4		14	// Called VUK Opto
#define HARLEY_PF_SW_TROUGH_STACK	15	// Called 4-Ball Stacking Opto

#define HARLEY_PF_SW_SHOOTER_LANE	16

//Left Side "LIVE" drop targets
#define HARLEY_PF_SW_LEFT_DT_L_IVE	17
#define HARLEY_PF_SW_LEFT_DT_LI_VE	18
#define HARLEY_PF_SW_LEFT_DT_LIV_E	19
#define HARLEY_PF_SW_LEFT_DT_LIVE_	20

//Right Side "RIDE" drop targets
#define HARLEY_PF_SW_RIGHT_DT_R_IDE  21
#define HARLEY_PF_SW_RIGHT_DT_RI_DE  22
#define HARLEY_PF_SW_RIGHT_DT_RID_E  23
#define HARLEY_PF_SW_RIGHT_DT_RIDE_  24

//Ramp
#define HARLEY_PF_SW_RT_RAMP_ENTER	 25
#define HARLEY_PF_SW_RT_RAMP_EXIT	 26
#define HARLEY_PF_SW_RT_RAMP_MID	 27

#define HARLEY_PF_SW_SPINNER		 28

//Stand Up Targets
#define HARLEY_PF_SW_ST_LT_MCYCLE	 29	// Called Left Stand Up Target (M-CYCLE)
#define HARLEY_PF_SW_ST_RT_MCYCLE	 30	// Called Right Stand Up Target (M-CYCLE)
#define HARLEY_PF_SW_ST_LT_RTRAMP	 31	// Called Left Stand Up Target (RT RAMP)
#define HARLEY_PF_SW_ST_RT_RTRAMP	 32	// Called Right Stand Up Target (RT RAMP)

//Orbits
#define HARLEY_PF_SW_LT_ORBIT		 33
#define HARLEY_PF_SW_RT_ORBIT		 34

//Motor
#define HARLEY_PF_SW_MOTOR_UP		 35
#define HARLEY_PF_SW_MOTOR_DOWN		 36
#define HARLEY_PF_SW_OPTO			 37 // Motorcycle Hit
//#38 - Not Used
//#39 - Not Used
//#40 - Not Used
#define HARLEY_PF_SW_TROUGH4_MCYCLE	 41
#define HARLEY_PF_SW_TROUGH3_MCYCLE	 42
#define HARLEY_PF_SW_TROUGH2_MCYCLE	 43
#define HARLEY_PF_SW_TROUGH1_MCYCLE	 44

#define HARLEY_PF_SW_SUPER_VUK		 45
#define HARLEY_PF_SW_SCOOP_EJECT	 46
//#47 - Not Used
#define HARLEY_PF_SW_BEHIND_VUK		 48

//Bumpers
#define HARLEY_PF_SW_LEFT_BUMPER	 49
#define HARLEY_PF_SW_RIGHT_BUMPER	 50
#define HARLEY_PF_SW_BOTT_BUMPER	 51
#define HARLEY_PF_SW_TOP_BUMPER		 52

#define HARLEY_PF_SW_LAUNCH_BUTTON	 53

//Common 
#define HARLEY_PF_SW_START_BUTTON	 54
#define HARLEY_PF_SW_SLAM_TILT		 55
#define HARLEY_PF_SW_TILT			 56

#define HARLEY_PF_SW_LEFT_OUTLANE	 57
#define HARLEY_PF_SW_LEFT_RETURN	 58
#define HARLEY_PF_SW_LEFT_SLING		 59
#define HARLEY_PF_SW_RIGHT_OUTLANE	 60
#define HARLEY_PF_SW_RIGHT_RETURN	 61
#define HARLEY_PF_SW_RIGHT_SLING	 62
//#63 - Not Used
//#64 - Not Used

//SPECIFIC COIL #
#define HARLEY_COIL_TROUGH_UP		1
#define HARLEY_COIL_AUTO_LAUNCH		2
#define HARLEY_COIL_SUPER_VUK		3
#define HARLEY_COIL_SCOOP			4
#define HARLEY_COIL_MOTO_LAUNCH		5
#define HARLEY_COIL_LEFT_DT			6
#define HARLEY_COIL_RIGHT_DT		7
#define HARLEY_COIL_EURO_TOKEN		8
#define HARLEY_COIL_L_BUMPER		9
#define HARLEY_COIL_R_BUMPER		10
#define HARLEY_COIL_B_BUMPER		11
#define HARLEY_COIL_T_BUMPER		12
#define HARLEY_COIL_SHAKER_MOTOR	13
#define HARLEY_COIL_MAGNET			14
#define HARLEY_COIL_L_FLIPPER		15
#define HARLEY_COIL_R_FLIPPER		16
#define HARLEY_COIL_L_SLING			17
#define HARLEY_COIL_R_SLING			18
#define HARLEY_COIL_MOTOR_RELAY		19
#define HARLEY_FLASH_SCOOP			20		// Flasher
#define HARLEY_COIL_LT_OUTLANE		21
#define HARLEY_COIL_RT_OUTLANE		22
#define HARLEY_COIL_UP_DOWN_POST	23
#define HARLEY_COIL_COIN_METER		24
#define HARLEY_FLASH_F1				25		// Flasher
#define HARLEY_FLASH_F2				26		// Flasher
#define HARLEY_FLASH_F3				27		// Flasher
#define HARLEY_FLASH_F4 			28		// Flasher
#define HARLEY_FLASH_F5				29		// Flasher
#define HARLEY_FLASH_F6				30		// Flasher
#define HARLEY_FLASH_F7				31		// Flasher
#define HARLEY_FLASH_F8				32		// Flasher

/*---------------------
/  Ball state handling
/----------------------*/
enum {stTrough4=SIM_FIRSTSTATE, stTrough3, stTrough2, stTrough1, stTrough, stDrain,
      stShooter, stBallLane, stRightOutlane, stLeftOutlane, stRightInlane, stLeftInlane, stLeftSling, stRightSling,
	  stMotoEntrance, stMotoDown, stMotoUp, stMTrough4, stMTrough3, stMTrough2, stMTrough1,
	  stRampEnter, stRampMiddle, stRampExit, stTrafficLite, stScoop, stBehindLR, stBehindRL, stBehindTrafficLiteVUK,
	  stLoopLR, stLoopRL, stLoopL, stLoopR, stJetArea, stJet1, stJet2, stJet3, stJet4,
	  stSpinner, stSpinner2, stSpinner3
	  };

static sim_tState harley_stateDef[] = {
  {"Not Installed",		0,0,		 0,		stDrain,	0,	0,	0,	SIM_STNOTEXCL},
  {"Moving"},
  {"Playfield",			0,0,		 0,		0,		0,	0,	0,	SIM_STNOTEXCL},

  /*Line 1*/
  {"Trough 1",			1,HARLEY_PF_SW_TROUGH1,	0,		stTrough3,	1},
  {"Trough 2",			1,HARLEY_PF_SW_TROUGH2,	0,		stTrough2,	1},
  {"Trough 3",			1,HARLEY_PF_SW_TROUGH3,	0,		stTrough1,	1},
  {"Trough 4",			1,HARLEY_PF_SW_TROUGH4,	HARLEY_COIL_TROUGH_UP,	stTrough,	1},
  {"Trough Jam",		1,HARLEY_PF_SW_TROUGH_STACK,  0,		stShooter,	1},
  {"Drain",				1,0,		0,		stTrough4,	0,	0,	0,	SIM_STNOTEXCL},

  /*Line 2*/
  {"Shooter",			1,HARLEY_PF_SW_SHOOTER_LANE,	 HARLEY_COIL_AUTO_LAUNCH,	stBallLane,	0,	0,	0,	SIM_STNOTEXCL|SIM_STSHOOT},
  {"Ball Lane",			1,0,		 0,		stLoopRL,		7,	0,	0,	SIM_STNOTEXCL},
  {"Right Outlane",		1,HARLEY_PF_SW_RIGHT_OUTLANE,0,		stDrain,	15},
  {"Left Outlane",		1,HARLEY_PF_SW_LEFT_OUTLANE, 0,		stDrain,	15},
  {"Right Inlane",		1,HARLEY_PF_SW_RIGHT_RETURN, 0,		stFree,		5},
  {"Left Inlane",		1,HARLEY_PF_SW_LEFT_RETURN,	 0,		stFree,		5},
  {"Left Slingshot",	1,HARLEY_PF_SW_LEFT_SLING,	 0,		stFree,		1},
  {"Rt Slingshot",		1,HARLEY_PF_SW_RIGHT_SLING,	 0,		stFree,		1},

  /*Line 3*/
  {"Moto-Entrance",		1,0,    0,       0, 0},	// harley_handleBallState handler
  {"Moto-Down",			1,HARLEY_PF_SW_OPTO,			0,		stFree, 5},
  {"Moto-Up",		    1,HARLEY_PF_SW_OPTO,			0,		stMTrough4, 5},
  {"Moto-Trough 4",		1,HARLEY_PF_SW_TROUGH4_MCYCLE,	0,		stMTrough3,	1},
  {"Moto-Trough 3",		1,HARLEY_PF_SW_TROUGH3_MCYCLE,	0,		stMTrough2,	1},
  {"Moto-Trough 2",		1,HARLEY_PF_SW_TROUGH2_MCYCLE,	0,		stMTrough1,	1},
  {"Moto-Trough 1",		1,HARLEY_PF_SW_TROUGH1_MCYCLE,	HARLEY_COIL_MOTO_LAUNCH,	stMotoDown,	1},

  /*Line 4*/
  {"Ramp Enter",		1,HARLEY_PF_SW_RT_RAMP_ENTER,	0,		stRampMiddle, 5},
  {"Ramp Middle",		1,HARLEY_PF_SW_RT_RAMP_MID,		0,		stRampExit,	  2},
  {"Ramp Exit",		    1,HARLEY_PF_SW_RT_RAMP_EXIT,	0,		stLeftInlane, 10},

  {"Traffic Lite",			1,HARLEY_PF_SW_SUPER_VUK,	HARLEY_COIL_SUPER_VUK,	stRightInlane,	10},
  {"Next City Scoop",		1,HARLEY_PF_SW_SCOOP_EJECT,	HARLEY_COIL_SCOOP,	stFree,	1},
  {"Behind Traffic",		1,0,	0,	0,	0},
  {"Behind Traffic",		1,0,	0,	0,	0},
  {"Behind VUK",			1,HARLEY_PF_SW_BEHIND_VUK,	0,	stTrafficLite,	10},

  /*Line 5*/
  {"Left Loop",			1,HARLEY_PF_SW_LT_ORBIT,	0,		stSpinner2, 10},
  {"Right Loop",		1,HARLEY_PF_SW_RT_ORBIT,	0,		stBehindRL, 10},
  {"Left Loop",			1,HARLEY_PF_SW_LT_ORBIT,	0,		stSpinner, 10},
  {"Right Loop",		1,HARLEY_PF_SW_RT_ORBIT,	0,		stFree, 10},
  {"Bumper Jets",		1,0,	0,		0, 0},
  {"L.Bumper",			1,HARLEY_PF_SW_LEFT_BUMPER,	0,		stJetArea, 1},
  {"R.Bumper",			1,HARLEY_PF_SW_RIGHT_BUMPER,0,		stJetArea, 2},
  {"B.Bumper",			1,HARLEY_PF_SW_BOTT_BUMPER,	0,		stJetArea, 1},
  {"T.Bumper",			1,HARLEY_PF_SW_TOP_BUMPER,	0,		stJetArea, 2},

  /*Line 6*/
  {"Spinner",			1,HARLEY_PF_SW_SPINNER, 0, stJetArea, 3, 0,0,SIM_STSPINNER},
  {"Spinner",			1,HARLEY_PF_SW_SPINNER, 0, stBehindLR, 3, 0,0,SIM_STSPINNER},
  {"Spinner",			1,HARLEY_PF_SW_SPINNER, 0, stFree, 3, 0,0,SIM_STSPINNER}
};

static int harley_handleBallState(sim_tBallStatus *ball, int *inports)
{
  static int bumps = 0;
  static int wait = 0;
  
  switch (ball->state)
  {
	/* Behind Traffic Lite */
	case stBehindLR:
	case stBehindRL:
		// give time for magnet to activate..
		if(wait++ > 5)
		{
			// Magnet can stop the ball and send to the VUK
			int coil = core_getSol(HARLEY_COIL_MAGNET);
			wait = 0;

			// ball is coming from left -> right
			if(ball->state == stBehindLR)
			{
				// magnet sends to traffic light VUK
				if(coil)
					return setState(stBehindTrafficLiteVUK,1);
				// continue to right loop switch
				else
					return setState(stLoopR,5);
			}
			// ball is coming from right -> left
			else
			{
				// magnet sends to traffic light VUK
				if(coil)
					return setState(stBehindTrafficLiteVUK,1);
				// continue to left loop switch
				else
					return setState(stLoopL,5);
			}
		}
		break;

	  /* Moto Entrance */
      case stMotoEntrance:
		// Moto Up? ( Half way at least )
		if (locals.motopos >= MOTO_UP / 2)
			return setState(stMotoUp,10);		/*Yes, ball goes into trough*/
		else
			return setState(stMotoDown,10);    /*No ball goes back to playfield*/
		break;

	 /* Jet Area */
	  case stJetArea:
		//first time to Jet Area?
		if(bumps == 0)
		{
			// 75% random chance we'll hit a bumper
			if((rand() % 4 + 1) > 1)
			{
				// random # up to 6 different times as to how many hits
				bumps = rand() % 6 + 1;
			}
			// - No bumper this time..
			else
			{
				return setState(stFree,10);
			}
		}
		if(bumps > 0)
		{
			// Random bumper will be activated
			int bumpr = rand() % 4;
			bumps--;
			//was this the last one?
			if(bumps == 0)
				return setState(stFree,10);
			else
				return setState(stJet1+bumpr,2);
		}
		break;
  }
  return 0;

}

/*---------------------------
/  Keyboard conversion table
/----------------------------*/

static sim_tInportData harley_inportData[] = {

/* Port 0 */
//  {0, 0x0005, st},
//  {0, 0x0006, st},
  {0, 0x0004, HARLEY_PF_SW_LAUNCH_BUTTON, SIM_STANY},
  {0, 0x0009, stLeftOutlane},
  {0, 0x000a, stRightOutlane},
  {0, 0x0011, stLeftSling},
  {0, 0x0012, stRightSling},
  {0, 0x0021, stLeftInlane},
  {0, 0x0022, stRightInlane},
  {0, 0x0041, stLoopLR},
  {0, 0x0042, stLoopRL},
  {0, 0x0080, stTrafficLite},
  {0, 0x0100, stRampEnter},
  {0, 0x0200, HARLEY_PF_SW_ST_LT_MCYCLE, SIM_STANY},
  {0, 0x0400, HARLEY_PF_SW_ST_RT_MCYCLE, SIM_STANY},
  {0, 0x0800, HARLEY_PF_SW_ST_LT_RTRAMP, SIM_STANY},
  {0, 0x1000, HARLEY_PF_SW_ST_RT_RTRAMP, SIM_STANY},
  {0, 0x2000, stJetArea},
  {0, 0x4000, stDrain},
  {0, 0x8000, stScoop},

/* Port 1 */
  {1, 0x0001, HARLEY_PF_SW_LEFT_DT_L_IVE, SIM_STANY},
  {1, 0x0002, HARLEY_PF_SW_LEFT_DT_LI_VE, SIM_STANY},
  {1, 0x0004, HARLEY_PF_SW_LEFT_DT_LIV_E, SIM_STANY},
  {1, 0x0008, HARLEY_PF_SW_LEFT_DT_LIVE_, SIM_STANY},
  {1, 0x0010, HARLEY_PF_SW_RIGHT_DT_R_IDE, SIM_STANY},
  {1, 0x0020, HARLEY_PF_SW_RIGHT_DT_RI_DE, SIM_STANY},
  {1, 0x0040, HARLEY_PF_SW_RIGHT_DT_RID_E, SIM_STANY},
  {1, 0x0080, HARLEY_PF_SW_RIGHT_DT_RIDE_, SIM_STANY},
  {1, 0x0100, stMotoEntrance},
  {1, 0x0200, stSpinner3},
//  {1, 0x0400, st},
//  {1, 0x0800, st},
//  {1, 0x1000, st},
//  {1, 0x2000, st},
//  {1, 0x4000, st},
//  {1, 0x8000, st},
  {0}
};

/*--------------------
  Drawing information
  --------------------*/
static void harley_drawStatic(BMTYPE **line) {

/* Help */

  core_textOutf(30, 60,BLACK,"Help on this Simulator:");
  core_textOutf(30, 70,BLACK,"L/R Ctrl+- = L/R Slingshot");
  core_textOutf(30, 80,BLACK,"L/R Ctrl+I/O = L/R Inlane/Outlane");
  core_textOutf(30, 90,BLACK,"L/R Ctrl+L = L/R Loops");
  core_textOutf(30,100,BLACK,"Q = Drain Ball, M = Motorcylce");
  core_textOutf(30,110,BLACK,"R = Ramp, T = Traffic Lite");
  core_textOutf(30,120,BLACK,"N = Next City Hole, B = Bumpers");
  core_textOutf(30,130,BLACK,"W/E = Motorcycle Standup Targets");
  core_textOutf(30,140,BLACK,"Y/U = Ramp Standup Targets");
  core_textOutf(30,150,BLACK,"A/S/D/F = LIVE Drop Targets");
  core_textOutf(30,160,BLACK,"G/H/J/K = RIDE Drop Targets");
  core_textOutf(30,170,BLACK,"X = Spinner");
}

/*-----------------
/  ROM definitions
/------------------*/
#define HARLEY_SOUND \
DE2S_SOUNDROM18884("hdsnd.u7",CRC(b9accb75) SHA1(9575f1c372ec5603322255778fc003047acc8b01), \
                  "hdvc1.u17",CRC(0265fe72) SHA1(7bd7b321bfa2a5092cdf273dfaf4ccdb043c06e8), \
                  "hdvc2.u21",CRC(89230898) SHA1(42d225e33ac1d679415e9dbf659591b7c4109740), \
                  "hdvc3.u36",CRC(41239811) SHA1(94fceff4dbefd3467ecb8b19e4b8baf24ddd68a3), \
                  "hdvc4.u37",CRC(a1bc39f6) SHA1(25af40cb3d8f774e1e37cbef9166e41753440460))

SE128_ROMSTART(harl_a13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispa.104",CRC(fc7c2924) SHA1(172fceb4d3221608f48a4abe4c4c5f3043834957)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_u13,"harcpuk.103",CRC(e39130a7) SHA1(7854c885a82f42f35e266e3cb96a68969d49fbad))
DE_DMD32ROM8x("hddispa.104",CRC(fc7c2924) SHA1(172fceb4d3221608f48a4abe4c4c5f3043834957)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_a10,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispa.100",CRC(bdeac0fd) SHA1(5aa1392a13f3c632b660ea6cb3dee23327404d80)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_f13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispf.104",CRC(5f80436e) SHA1(e89e561807670118c3d9e623d4aec2321c774576)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_g13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispg.104",CRC(c7f197a0) SHA1(3b7f0699c08d387c67ff6cd185360e60fcd21b9e)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_i13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddispi.104",CRC(387a5aad) SHA1(a0eb99b240f6044db05668c4504e908aee205220)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_l13,"harcpu.103",CRC(2a812c75) SHA1(46e1f18e1c9992ca1823f7818b6d51c001f5a934))
DE_DMD32ROM8x("hddisps.104",CRC(2d26514a) SHA1(f15b22cad6329f29cd5cccfb91a2ba7ca2cd6d59)) HARLEY_SOUND
SE_ROMEND

CORE_GAMEDEF(harl,a13,"Harley-Davidson (Sega, 1.03)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,u13,a13,"Harley-Davidson (Sega, 1.03 English)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,a10,a13,"Harley-Davidson (Sega, 1.03, Display 1.00)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,f13,a13,"Harley-Davidson (Sega, 1.03 French)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,g13,a13,"Harley-Davidson (Sega, 1.03 German)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,i13,a13,"Harley-Davidson (Sega, 1.03 Italian)",1999,"Sega",de_mSES1,0)
CORE_CLONEDEF(harl,l13,a13,"Harley-Davidson (Sega, 1.03 Spanish)",1999,"Sega",de_mSES1,0)

/********************* STERN GAMES  **********************/
/*-------------------------------------------------------------------
/ Harley ( 4.00 )
/-------------------------------------------------------------------*/
SE128_ROMSTART(harl_a40,"harcpu.400",CRC(752ed258) SHA1(aea0ab3c45649178a3b0e17a2eacc516600a2b63))
DE_DMD32ROM8x("hddispa.400",CRC(e2c98397) SHA1(212ac1a509f608c490dc4dfdc5cc04187ed2fe10)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_f40,"harcpu.400",CRC(752ed258) SHA1(aea0ab3c45649178a3b0e17a2eacc516600a2b63))
DE_DMD32ROM8x("hddispf.400",CRC(d061c238) SHA1(cb29e58970d43c2845c96e149e8fdd0c16e501c9)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_g40,"harcpu.400",CRC(752ed258) SHA1(aea0ab3c45649178a3b0e17a2eacc516600a2b63))
DE_DMD32ROM8x("hddispg.400",CRC(4bc89a23) SHA1(fb7dcc61194560845e150bc1c032c098ffd026e8)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_i40,"harcpu.400",CRC(752ed258) SHA1(aea0ab3c45649178a3b0e17a2eacc516600a2b63))
DE_DMD32ROM8x("hddispi.400",CRC(c4fc4990) SHA1(79d501c3123b604becbb87c12aca9848675811ec)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_l40,"harcpu.400",CRC(752ed258) SHA1(aea0ab3c45649178a3b0e17a2eacc516600a2b63))
DE_DMD32ROM8x("hddispl.400",CRC(96096e73) SHA1(b22f03ab3f08ff192a55e92ebe85bafa893c6234)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_a30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispa.300",CRC(61b274f8) SHA1(954e4b3527cefcb24376de9f6f7e5f9192ab3304)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_f30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispf.300",CRC(106f7f1f) SHA1(92a8ab7d834439a2211208e0812cdb1199acb21d)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_g30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispg.300",CRC(8f7da748) SHA1(fee1534b76769517d4e6dbed373583e573fb95b6)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_i30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispi.300",CRC(686d3cf6) SHA1(fb27e2e4b39abb56deb1e66f012d151126971474)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_l30,"harcpu.300",CRC(c4ba9df5) SHA1(6bdcd555db946e396b630953db1ba50677177137))
DE_DMD32ROM8x("hddispl.300",CRC(4cc7251b) SHA1(7660fca37ac9fb442a059ddbafc2fa13f94dfae1)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_a18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispa.105",CRC(401a7b9f) SHA1(37e99a42738c1147c073585391772ecc55c9a759)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_f18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispf.105",CRC(31c77078) SHA1(8a0e2dbb698da77dffa1ab01df0f360fecf6c4c7)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_g18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispg.105",CRC(aed5a82f) SHA1(4c44b052a9b1fa702ff49c9b2fb7cf48173459d2)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_i18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddispi.105",CRC(49c53391) SHA1(98f88eb8a49bbc59f78996d713c72ec495ba806f)) HARLEY_SOUND
SE_ROMEND

SE128_ROMSTART(harl_l18,"harcpu.108",CRC(a8e24328) SHA1(cc32d97521f706e3d8ddcf4117ec0c0e7424a378))
DE_DMD32ROM8x("hddisps.105",CRC(6d6f2a7c) SHA1(1609c69a1584398c3504bb5a0c46f878e8dd547c)) HARLEY_SOUND
SE_ROMEND

CORE_GAMEDEF(harl,a40,"Harley-Davidson (Stern, 4.00)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,f40,a40,"Harley-Davidson (Stern, 4.00 French)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,g40,a40,"Harley-Davidson (Stern, 4.00 German)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,i40,a40,"Harley-Davidson (Stern, 4.00 Italian)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,l40,a40,"Harley-Davidson (Stern, 4.00 Spanish)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,a30,a40,"Harley-Davidson (Stern, 3.00)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,f30,a40,"Harley-Davidson (Stern, 3.00 French)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,g30,a40,"Harley-Davidson (Stern, 3.00 German)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,i30,a40,"Harley-Davidson (Stern, 3.00 Italian)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,l30,a40,"Harley-Davidson (Stern, 3.00 Spanish)",2004,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,a18,a40,"Harley-Davidson (Stern, 1.08)",2003,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,f18,a40,"Harley-Davidson (Stern, 1.08 French)",2003,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,g18,a40,"Harley-Davidson (Stern, 1.08 German)",2003,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,i18,a40,"Harley-Davidson (Stern, 1.08 Italian)",2003,"Stern",de_mSES1,0)
CORE_CLONEDEF(harl,l18,a40,"Harley-Davidson (Stern, 1.08 Spanish)",2003,"Stern",de_mSES1,0)



static struct core_dispLayout se_dmd128x32[] = {
  {0,0, 32,128, CORE_DMD, (genf *)dedmd32_update}, {0}
};

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static sim_tSimData harleySimData = {
  2,    				/* 2 game specific input ports */
  harley_stateDef,	/* Definition of all states */
  harley_inportData,	/* Keyboard Entries */
  { stTrough1, stTrough2, stTrough3, stTrough4, stDrain, stDrain, stDrain },	/*Position where balls start.. Max 7 Balls Allowed */
  NULL, 				/* no init */
  harley_handleBallState,	/*Function to handle ball state changes */
  harley_drawStatic,	/* Function to handle mechanical state changes */
  FALSE, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};

/*----------------------
/ Game Data Information
/----------------------*/

/*---------------
/  Game handling
/----------------*/
static core_tGameData harlGameData = { 
	GEN_WS, se_dmd128x32,
  {
    FLIP_SW(FLIP_L) | FLIP_SOL(FLIP_L),
    0, 2, 0, 0, 0, 0, 0,
    0, harley_handleMech, NULL, harley_drawMech
  },
  &harleySimData,
  {
    "",
    /*Coin    1     2     3     4     5     6     7     8     9    10   Cab.  Cust */
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* inverted switches */
    /*Start    Tilt    SlamTilt */
    { HARLEY_PF_SW_START_BUTTON, HARLEY_PF_SW_TILT, HARLEY_PF_SW_SLAM_TILT },
  }
};

static void init_harl(void) {
  core_gameData = &harlGameData;
  /* initialize random seed: */
  srand ( (unsigned int)time(NULL) );
}


static void harley_drawMech(BMTYPE **line) 
{
  char temp[255];
  if(locals.motopos >= MOTO_UP)
	  sprintf(temp,"Up");
  else
  if(locals.motopos <= MOTO_DOWN)
	  sprintf(temp,"Down");
  else
  {
	  if(locals.motodir == MOTO_DIR_UP)
		  sprintf(temp,"Mov.Up");
	  else
	  if(locals.motodir == MOTO_DIR_DOWN)
		  sprintf(temp,"Mov.Down");
	  else
		sprintf(temp,"Stopped");
  }

  core_textOutf(30, 0,BLACK,"Moto: %-8s",temp);
}

static void harley_handleMech(int mech)
{
  if (mech & 0x01)
  {
	//Handle Moto Up & Down Switch
	core_setSw(HARLEY_PF_SW_MOTOR_UP,locals.motopos >= MOTO_UP);
	core_setSw(HARLEY_PF_SW_MOTOR_DOWN,locals.motopos <= MOTO_DOWN);

	//Moving Moto?
	if (core_getSol(HARLEY_COIL_MOTOR_RELAY))
	{
		//What direction?
		switch(locals.motodir)
		{
			case MOTO_DIR_NONE:
				//Go Up if down, or Down if Up
				if(locals.motopos <= MOTO_DOWN)
					locals.motodir = MOTO_DIR_UP;
				else
					locals.motodir = MOTO_DIR_DOWN;
				break;
			case MOTO_DIR_UP:
				locals.motopos++;
				//stop at top
				if(locals.motopos == MOTO_UP + 10)
					locals.motodir = MOTO_DIR_NONE;
				break;
			case MOTO_DIR_DOWN:
				locals.motopos--;
				//stop at bottom
				if(locals.motopos == MOTO_DOWN - 10)
					locals.motodir = MOTO_DIR_NONE;
				break;
		}
	}
  }
}
