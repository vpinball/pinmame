/*******************************************************************************
 Pinball Simulator Test Matrixes (PSTM)

 by Marton Larrosa (marton@mail.com)
 Nov. 19, 2000

 Description:
	Shows all the Solenoid/Switch numbers OnScreen reporting if On/Off.

 Usage:
	Add the Game Data from Games.c where required and replace all the xxx
	strings by the driver's name.
        You will find comments on where to put the data in.

 Warning: Solenoids 37-44 are not shown because they are part of the
          WPC_EXTBOARD. You have to simulate these solenoids manually.
          TZ & ST:TNG drivers are nice examples for seeing how they work.

 ******************************************************************************/

#include "driver.h"
#include "wpc.h"
#include "wpcsim.h"

/* Remove the "#include "wpcsound.h"" and uncomment the next line if the game
   has the DCS sound system */

#include "wpcsound.h"
//#include "dcs.h"

/*------------------
/  Local functions
/-------------------*/
static int pstm_getSol(int solNo);
static void pstm_drawMech(unsigned char **line);

/* Replace xxx by the driver's name! */
static void init_xxx(void);

/* Switch Definitions */

#define sw01		1
#define sw02		2
#define sw03		3
#define sw04		4
#define sw05		5
#define sw06		6
#define sw07		7
#define sw08		8
#define sw09		9
#define sw10		10
#define sw11		11
#define sw12		12
#define swStart		13
#define sw13		13
#define swTilt		14
#define sw14		14
#define sw15		15
#define sw16		16
#define sw17		17
#define sw18		18
#define sw19		19
#define sw20		20
#define swSlamTilt	21
#define sw21		21
#define swCoinDoor	22
#define sw22		22
#define swTicket	23
#define sw23		23
#define sw24		24
#define sw25		25
#define sw26		26
#define sw27		27
#define sw28		28
#define sw29		29
#define sw30		30
#define sw31		31
#define sw32		32
#define sw33		33
#define sw34		34
#define sw35		35
#define sw36		36
#define sw37		37
#define sw38		38
#define sw39		39
#define sw40		40
#define sw41		41
#define sw42		42
#define sw43		43
#define sw44		44
#define sw45		45
#define sw46		46
#define sw47		47
#define sw48		48
#define sw49		49
#define sw50		50
#define sw51		51
#define sw52		52
#define sw53		53
#define sw54		54
#define sw55		55
#define sw56		56
#define sw57		57
#define sw58		58
#define sw59		59
#define sw60		60
#define sw61		61
#define sw62		62
#define sw63		63
#define sw64		64
#define sw65		65
#define sw66		66
#define sw67		67
#define sw68		68
#define sw69		69
#define sw70		70
#define sw71		71
#define sw72		72
#define sw73		73
#define sw74		74
#define sw75		75
#define sw76		76
#define sw77		77
#define sw78		78
#define sw79		79
#define sw80		80
#define sw81		81
#define sw82		82
#define sw83		83
#define sw84		84
#define sw85		85
#define sw86		86
#define sw87		87
#define sw88		88
#define sw89		89
#define sw90		90
#define sw91		91
#define sw92		92
#define sw93		93
#define sw94		94
#define sw95		95
#define sw96		96
#define sw97		97
#define sw98		98
#define sw99		99


/* Solenoid Definitions */

#define sSol01		1
#define sSol02		2
#define sSol03		3
#define sSol04		4
#define sSol05		5
#define sSol06		6
#define sSol07		7
#define	sSol08		8
#define sSol09		9
#define sSol10		10
#define sSol11		11
#define sSol12		12
#define sSol13		13
#define sSol14		14
#define sSol15		15
#define sSol16		16
#define sSol17		17
#define sSol18		18
#define sSol19		19
#define sSol20		20
#define sSol21		21
#define sSol22		22
#define sSol23		23
#define	sSol24		24
#define sSol25		25
#define	sSol26		26
#define sSol27		27
#define sSol28		28
#define sSol29		29
#define sSol30		30
#define sSol31		31
#define sSol32		32
#define sSol33		33
#define	sSol34		34
#define sSol35		35
#define	sSol36		36
#define sSol37		37
#define sSol38		38
#define sSol39		39
#define sSol40		40
#define sSol41		41
#define sSol42		42
#define sSol43		43
#define	sSol44		44
#define sSol45		45
#define	sSol46		46
#define sSol47		47
#define sSol48		48
#define sSol49		49


/*--------------------
  Drawing information
  --------------------*/
  static void pstm_drawMech(unsigned char **line) {

  wpc_textOutf(30, 10,BLACK," 0123456789");

  wpc_textOutf(30, 20,BLACK,"0-");
  wpc_textOutf(42, 20,BLACK,"%-6s", wpc_getSol(sSol01) ? "*":".");
  wpc_textOutf(48, 20,BLACK,"%-6s", wpc_getSol(sSol02) ? "*":".");
  wpc_textOutf(54, 20,BLACK,"%-6s", wpc_getSol(sSol03) ? "*":".");
  wpc_textOutf(60, 20,BLACK,"%-6s", wpc_getSol(sSol04) ? "*":".");
  wpc_textOutf(66, 20,BLACK,"%-6s", wpc_getSol(sSol05) ? "*":".");
  wpc_textOutf(72, 20,BLACK,"%-6s", wpc_getSol(sSol06) ? "*":".");
  wpc_textOutf(78, 20,BLACK,"%-6s", wpc_getSol(sSol07) ? "*":".");
  wpc_textOutf(84, 20,BLACK,"%-6s", wpc_getSol(sSol08) ? "*":".");
  wpc_textOutf(90, 20,BLACK,"%-6s", wpc_getSol(sSol09) ? "*":".");
  wpc_textOutf(96, 20,BLACK,"0");
  wpc_textOutf(102,20,BLACK,"  <- Solenoid Matrix");

  wpc_textOutf(30, 30,BLACK,"1%-6s", wpc_getSol(sSol10) ? "*":".");
  wpc_textOutf(42, 30,BLACK,"%-6s",  wpc_getSol(sSol11) ? "*":".");
  wpc_textOutf(48, 30,BLACK,"%-6s",  wpc_getSol(sSol12) ? "*":".");
  wpc_textOutf(54, 30,BLACK,"%-6s",  wpc_getSol(sSol13) ? "*":".");
  wpc_textOutf(60, 30,BLACK,"%-6s",  wpc_getSol(sSol14) ? "*":".");
  wpc_textOutf(66, 30,BLACK,"%-6s",  wpc_getSol(sSol15) ? "*":".");
  wpc_textOutf(72, 30,BLACK,"%-6s",  wpc_getSol(sSol16) ? "*":".");
  wpc_textOutf(78, 30,BLACK,"%-6s",  wpc_getSol(sSol17) ? "*":".");
  wpc_textOutf(84, 30,BLACK,"%-6s",  wpc_getSol(sSol18) ? "*":".");
  wpc_textOutf(90, 30,BLACK,"%-6s",  wpc_getSol(sSol19) ? "*":".");
  wpc_textOutf(96, 30,BLACK,"1");

  wpc_textOutf(30, 40,BLACK,"2%-6s", wpc_getSol(sSol20) ? "*":".");
  wpc_textOutf(42, 40,BLACK,"%-6s",  wpc_getSol(sSol21) ? "*":".");
  wpc_textOutf(48, 40,BLACK,"%-6s",  wpc_getSol(sSol22) ? "*":".");
  wpc_textOutf(54, 40,BLACK,"%-6s",  wpc_getSol(sSol23) ? "*":".");
  wpc_textOutf(60, 40,BLACK,"%-6s",  wpc_getSol(sSol24) ? "*":".");
  wpc_textOutf(66, 40,BLACK,"%-6s",  wpc_getSol(sSol25) ? "*":".");
  wpc_textOutf(72, 40,BLACK,"%-6s",  wpc_getSol(sSol26) ? "*":".");
  wpc_textOutf(78, 40,BLACK,"%-6s",  wpc_getSol(sSol27) ? "*":".");
  wpc_textOutf(84, 40,BLACK,"%-6s",  wpc_getSol(sSol28) ? "*":".");
  wpc_textOutf(90, 40,BLACK,"%-6s",  wpc_getSol(sSol29) ? "*":".");
  wpc_textOutf(96, 40,BLACK,"2");

  wpc_textOutf(30, 50,BLACK,"3%-6s", wpc_getSol(sSol30) ? "*":".");
  wpc_textOutf(42, 50,BLACK,"%-6s",  wpc_getSol(sSol31) ? "*":".");
  wpc_textOutf(48, 50,BLACK,"%-6s",  wpc_getSol(sSol32) ? "*":".");
  wpc_textOutf(54, 50,BLACK,"%-6s",  wpc_getSol(sSol33) ? "*":".");
  wpc_textOutf(60, 50,BLACK,"%-6s",  wpc_getSol(sSol34) ? "*":".");
  wpc_textOutf(66, 50,BLACK,"%-6s",  wpc_getSol(sSol35) ? "*":".");
  wpc_textOutf(72, 50,BLACK,"%-6s",  wpc_getSol(sSol36) ? "*":".");
  wpc_textOutf(78, 50,BLACK,"%-6s",  wpc_getSol(sSol37) ? "*":".");
  wpc_textOutf(84, 50,BLACK,"%-6s",  wpc_getSol(sSol38) ? "*":".");
  wpc_textOutf(90, 50,BLACK,"%-6s",  wpc_getSol(sSol39) ? "*":".");
  wpc_textOutf(96, 50,BLACK,"3");

  wpc_textOutf(30, 60,BLACK,"4%-6s", wpc_getSol(sSol40) ? "*":".");
  wpc_textOutf(42, 60,BLACK,"%-6s",  wpc_getSol(sSol41) ? "*":".");
  wpc_textOutf(48, 60,BLACK,"%-6s",  wpc_getSol(sSol42) ? "*":".");
  wpc_textOutf(54, 60,BLACK,"%-6s",  wpc_getSol(sSol43) ? "*":".");
  wpc_textOutf(60, 60,BLACK,"%-6s",  wpc_getSol(sSol44) ? "*":".");
  wpc_textOutf(66, 60,BLACK,"%-6s",  wpc_getSol(sSol45) ? "*":".");
  wpc_textOutf(72, 60,BLACK,"%-6s",  wpc_getSol(sSol46) ? "*":".");
  wpc_textOutf(78, 60,BLACK,"%-6s",  wpc_getSol(sSol47) ? "*":".");
  wpc_textOutf(84, 60,BLACK,"%-6s",  wpc_getSol(sSol48) ? "*":".");
  wpc_textOutf(90, 60,BLACK,"%-6s",  wpc_getSol(sSol49) ? "*":".");
  wpc_textOutf(96, 60,BLACK,"4");

  wpc_textOutf(30, 70,BLACK," 0123456789");

  wpc_textOutf(140, 50,BLACK," 0123456789");

  wpc_textOutf(140, 60,BLACK,"0-");
  wpc_textOutf(152, 60,BLACK,"%-6s", wpc_getSw(sw01) ? "*":".");
  wpc_textOutf(158, 60,BLACK,"%-6s", wpc_getSw(sw02) ? "*":".");
  wpc_textOutf(164, 60,BLACK,"%-6s", wpc_getSw(sw03) ? "*":".");
  wpc_textOutf(170, 60,BLACK,"%-6s", wpc_getSw(sw04) ? "*":".");
  wpc_textOutf(176, 60,BLACK,"%-6s", wpc_getSw(sw05) ? "*":".");
  wpc_textOutf(182, 60,BLACK,"%-6s", wpc_getSw(sw06) ? "*":".");
  wpc_textOutf(188, 60,BLACK,"%-6s", wpc_getSw(sw07) ? "*":".");
  wpc_textOutf(194, 60,BLACK,"%-6s", wpc_getSw(sw08) ? "*":".");
  wpc_textOutf(200, 60,BLACK,"%-6s", wpc_getSw(sw09) ? "*":".");
  wpc_textOutf(206, 60,BLACK,"0");

  wpc_textOutf(140, 70,BLACK,"1%-6s",wpc_getSw(sw10) ? "*":".");
  wpc_textOutf(152, 70,BLACK,"%-6s", wpc_getSw(sw11) ? "*":".");
  wpc_textOutf(158, 70,BLACK,"%-6s", wpc_getSw(sw12) ? "*":".");
  wpc_textOutf(164, 70,BLACK,"%-6s", wpc_getSw(sw13) ? "*":".");
  wpc_textOutf(170, 70,BLACK,"%-6s", wpc_getSw(sw14) ? "*":".");
  wpc_textOutf(176, 70,BLACK,"%-6s", wpc_getSw(sw15) ? "*":".");
  wpc_textOutf(182, 70,BLACK,"%-6s", wpc_getSw(sw16) ? "*":".");
  wpc_textOutf(188, 70,BLACK,"%-6s", wpc_getSw(sw17) ? "*":".");
  wpc_textOutf(194, 70,BLACK,"%-6s", wpc_getSw(sw18) ? "*":".");
  wpc_textOutf(200, 70,BLACK,"%-6s", wpc_getSw(sw19) ? "*":".");
  wpc_textOutf(206, 70,BLACK,"1");

  wpc_textOutf(140, 80,BLACK,"2%-6s",wpc_getSw(sw20) ? "*":".");
  wpc_textOutf(152, 80,BLACK,"%-6s", wpc_getSw(sw21) ? "*":".");
  wpc_textOutf(158, 80,BLACK,"%-6s", wpc_getSw(sw22) ? "*":".");
  wpc_textOutf(164, 80,BLACK,"%-6s", wpc_getSw(sw23) ? "*":".");
  wpc_textOutf(170, 80,BLACK,"%-6s", wpc_getSw(sw24) ? "*":".");
  wpc_textOutf(176, 80,BLACK,"%-6s", wpc_getSw(sw25) ? "*":".");
  wpc_textOutf(182, 80,BLACK,"%-6s", wpc_getSw(sw26) ? "*":".");
  wpc_textOutf(188, 80,BLACK,"%-6s", wpc_getSw(sw27) ? "*":".");
  wpc_textOutf(194, 80,BLACK,"%-6s", wpc_getSw(sw28) ? "*":".");
  wpc_textOutf(200, 80,BLACK,"%-6s", wpc_getSw(sw29) ? "*":".");
  wpc_textOutf(206, 80,BLACK,"2");

  wpc_textOutf(140, 90,BLACK,"3%-6s",wpc_getSw(sw30) ? "*":".");
  wpc_textOutf(152, 90,BLACK,"%-6s", wpc_getSw(sw31) ? "*":".");
  wpc_textOutf(158, 90,BLACK,"%-6s", wpc_getSw(sw32) ? "*":".");
  wpc_textOutf(164, 90,BLACK,"%-6s", wpc_getSw(sw33) ? "*":".");
  wpc_textOutf(170, 90,BLACK,"%-6s", wpc_getSw(sw34) ? "*":".");
  wpc_textOutf(176, 90,BLACK,"%-6s", wpc_getSw(sw35) ? "*":".");
  wpc_textOutf(182, 90,BLACK,"%-6s", wpc_getSw(sw36) ? "*":".");
  wpc_textOutf(188, 90,BLACK,"%-6s", wpc_getSw(sw37) ? "*":".");
  wpc_textOutf(194, 90,BLACK,"%-6s", wpc_getSw(sw38) ? "*":".");
  wpc_textOutf(200, 90,BLACK,"%-6s", wpc_getSw(sw39) ? "*":".");
  wpc_textOutf(206, 90,BLACK,"3");

  wpc_textOutf(140,100,BLACK,"4%-6s",wpc_getSw(sw40) ? "*":".");
  wpc_textOutf(152,100,BLACK,"%-6s", wpc_getSw(sw41) ? "*":".");
  wpc_textOutf(158,100,BLACK,"%-6s", wpc_getSw(sw42) ? "*":".");
  wpc_textOutf(164,100,BLACK,"%-6s", wpc_getSw(sw43) ? "*":".");
  wpc_textOutf(170,100,BLACK,"%-6s", wpc_getSw(sw44) ? "*":".");
  wpc_textOutf(176,100,BLACK,"%-6s", wpc_getSw(sw45) ? "*":".");
  wpc_textOutf(182,100,BLACK,"%-6s", wpc_getSw(sw46) ? "*":".");
  wpc_textOutf(188,100,BLACK,"%-6s", wpc_getSw(sw47) ? "*":".");
  wpc_textOutf(194,100,BLACK,"%-6s", wpc_getSw(sw48) ? "*":".");
  wpc_textOutf(200,100,BLACK,"%-6s", wpc_getSw(sw49) ? "*":".");
  wpc_textOutf(206,100,BLACK,"4");

  wpc_textOutf(140,110,BLACK,"5%-6s",wpc_getSw(sw50) ? "*":".");
  wpc_textOutf(152,110,BLACK,"%-6s", wpc_getSw(sw51) ? "*":".");
  wpc_textOutf(158,110,BLACK,"%-6s", wpc_getSw(sw52) ? "*":".");
  wpc_textOutf(164,110,BLACK,"%-6s", wpc_getSw(sw53) ? "*":".");
  wpc_textOutf(170,110,BLACK,"%-6s", wpc_getSw(sw54) ? "*":".");
  wpc_textOutf(176,110,BLACK,"%-6s", wpc_getSw(sw55) ? "*":".");
  wpc_textOutf(182,110,BLACK,"%-6s", wpc_getSw(sw56) ? "*":".");
  wpc_textOutf(188,110,BLACK,"%-6s", wpc_getSw(sw57) ? "*":".");
  wpc_textOutf(194,110,BLACK,"%-6s", wpc_getSw(sw58) ? "*":".");
  wpc_textOutf(200,110,BLACK,"%-6s", wpc_getSw(sw59) ? "*":".");
  wpc_textOutf(206,110,BLACK,"5");
  wpc_textOutf(30, 110,BLACK,"Switch Matrix ->");

  wpc_textOutf(140,120,BLACK,"6%-6s",wpc_getSw(sw60) ? "*":".");
  wpc_textOutf(152,120,BLACK,"%-6s", wpc_getSw(sw61) ? "*":".");
  wpc_textOutf(158,120,BLACK,"%-6s", wpc_getSw(sw62) ? "*":".");
  wpc_textOutf(164,120,BLACK,"%-6s", wpc_getSw(sw63) ? "*":".");
  wpc_textOutf(170,120,BLACK,"%-6s", wpc_getSw(sw64) ? "*":".");
  wpc_textOutf(176,120,BLACK,"%-6s", wpc_getSw(sw65) ? "*":".");
  wpc_textOutf(182,120,BLACK,"%-6s", wpc_getSw(sw66) ? "*":".");
  wpc_textOutf(188,120,BLACK,"%-6s", wpc_getSw(sw67) ? "*":".");
  wpc_textOutf(194,120,BLACK,"%-6s", wpc_getSw(sw68) ? "*":".");
  wpc_textOutf(200,120,BLACK,"%-6s", wpc_getSw(sw69) ? "*":".");
  wpc_textOutf(206,120,BLACK,"6");

  wpc_textOutf(140,130,BLACK,"7%-6s",wpc_getSw(sw70) ? "*":".");
  wpc_textOutf(152,130,BLACK,"%-6s", wpc_getSw(sw71) ? "*":".");
  wpc_textOutf(158,130,BLACK,"%-6s", wpc_getSw(sw72) ? "*":".");
  wpc_textOutf(164,130,BLACK,"%-6s", wpc_getSw(sw73) ? "*":".");
  wpc_textOutf(170,130,BLACK,"%-6s", wpc_getSw(sw74) ? "*":".");
  wpc_textOutf(176,130,BLACK,"%-6s", wpc_getSw(sw75) ? "*":".");
  wpc_textOutf(182,130,BLACK,"%-6s", wpc_getSw(sw76) ? "*":".");
  wpc_textOutf(188,130,BLACK,"%-6s", wpc_getSw(sw77) ? "*":".");
  wpc_textOutf(194,130,BLACK,"%-6s", wpc_getSw(sw78) ? "*":".");
  wpc_textOutf(200,130,BLACK,"%-6s", wpc_getSw(sw79) ? "*":".");
  wpc_textOutf(206,130,BLACK,"7");

  wpc_textOutf(140,140,BLACK,"8%-6s",wpc_getSw(sw80) ? "*":".");
  wpc_textOutf(152,140,BLACK,"%-6s", wpc_getSw(sw81) ? "*":".");
  wpc_textOutf(158,140,BLACK,"%-6s", wpc_getSw(sw82) ? "*":".");
  wpc_textOutf(164,140,BLACK,"%-6s", wpc_getSw(sw83) ? "*":".");
  wpc_textOutf(170,140,BLACK,"%-6s", wpc_getSw(sw84) ? "*":".");
  wpc_textOutf(176,140,BLACK,"%-6s", wpc_getSw(sw85) ? "*":".");
  wpc_textOutf(182,140,BLACK,"%-6s", wpc_getSw(sw86) ? "*":".");
  wpc_textOutf(188,140,BLACK,"%-6s", wpc_getSw(sw87) ? "*":".");
  wpc_textOutf(194,140,BLACK,"%-6s", wpc_getSw(sw88) ? "*":".");
  wpc_textOutf(200,140,BLACK,"%-6s", wpc_getSw(sw89) ? "*":".");
  wpc_textOutf(206,140,BLACK,"8");

  wpc_textOutf(140,150,BLACK,"9%-6s",wpc_getSw(sw90) ? "*":".");
  wpc_textOutf(152,150,BLACK,"%-6s", wpc_getSw(sw91) ? "*":".");
  wpc_textOutf(158,150,BLACK,"%-6s", wpc_getSw(sw92) ? "*":".");
  wpc_textOutf(164,150,BLACK,"%-6s", wpc_getSw(sw93) ? "*":".");
  wpc_textOutf(170,150,BLACK,"%-6s", wpc_getSw(sw94) ? "*":".");
  wpc_textOutf(176,150,BLACK,"%-6s", wpc_getSw(sw95) ? "*":".");
  wpc_textOutf(182,150,BLACK,"%-6s", wpc_getSw(sw96) ? "*":".");
  wpc_textOutf(188,150,BLACK,"%-6s", wpc_getSw(sw97) ? "*":".");
  wpc_textOutf(194,150,BLACK,"%-6s", wpc_getSw(sw98) ? "*":".");
  wpc_textOutf(200,150,BLACK,"%-6s", wpc_getSw(sw99) ? "*":".");
  wpc_textOutf(206,150,BLACK,"9");

  wpc_textOutf(140,160,BLACK," 0123456789");

}

/*--------------------------
/ Game specific input ports
/---------------------------*/
/* Replace xxx by the driver's name! */
WPC_INPUT_PORTS_START(xxx,1)
WPC_INPUT_PORTS_END

/*-----------------------
/ Simulation Definitions
/-----------------------*/
static wpcsim_tSimData pstmSimData = {
  1,    				/* 0 game specific input ports */
  NULL,					/* Definition of all states */
  NULL,					/* Keyboard Entries */
  {0},
  NULL, 				/* no init */
  NULL,					/*Function to handle ball state changes*/
  NULL,					/*Function to handle mechanical state changes*/
  NULL, 				/* Simulate manual shooter? */
  NULL  				/* Custom key conditions? */
};


/* Driver Info from Games.c follows */

/*-----------------
/  ROM definitions
/------------------*/
WPC_ROMSTART()
WPC_ROMEND

/*--------------
/  Game drivers
/---------------*/
WPC_GAMEDEF()

/*----------------------
/ Game Data Information
/----------------------*/


/* ... */
  ,pstm_getSol,
  NULL, NULL, NULL,
  pstm_drawMech,		/* Function to Draw the Status of Mechanical Objects on Screen */
  &pstmSimData			/* Definition of Simulator Data */
};

/* Driver Info from Games.c ends */

/*---------------
/  Game handling
/----------------*/
/* Replace xxx by the driver's name! */
static void init_xxx(void) {
  wpc_gameData = &xxxGameData;
}

/*-----------------------
/  Handle solenoids 37-44
/------------------------*/
static int pstm_getSol(int solNo) {
  static int bits[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

  if ( (solNo<37) || (solNo>44) )
    return 0;

  return (wpc_data[WPC_EXTBOARD1] & bits[solNo-37]) > 0;
}
