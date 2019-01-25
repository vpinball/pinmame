/**************************************************************************************
 **************************************************************************************
 **                                                                                  **
 **                      PacDrive.c - (Ultra IO Support)                             **
 **                                                                                  **
 **                      Added to PinMAME by Brad Oldham                             **
 **                                                                                  **
 **                          Last Updated: 01/2019                                   **
 **                                                                                  **
 **************************************************************************************
 **************************************************************************************/

// links: https://www.youtube.com/watch?v=Qjmg_wqtb7Q
//        https://www.youtube.com/watch?v=KJlfxvYAb1U
//        https://www.youtube.com/watch?v=UIq9VfOSVUI
//        https://www.youtube.com/watch?v=wCUi-ZB4Qt0
//        https://www.youtube.com/watch?v=56H_lmWtcfo

// Ultimarc Ultimate I/O and/or PacDrive support, currently hardcoded to drive a real Baby Pac-Man machine (see core.c (update_PacDrive()) and pacdrive.c to configure for other setups!)
// - use new command line option 'ultimateio' to enable it
// - also note that if you try this with a PacDrive, make sure you get the "Special" version of the PacDrive that defaults to all outputs OFF when power is turned on, and that you will need to connect a ground wire to the USB connector housing to get it to work since there is no ground connector on the PacDrive

// There are 12 groups of 8 bits for the Ultimate IO board (outputs 1 - 96)
// So I start mapping the solenoids at Outputs pointer array 97 through 106 (group 13 and 14)

#ifdef _WIN32 //!! also enable linux compilation!

#include "driver.h"
#include "pacdrive.h"

#define PRINT_INFO 1

static const int hw_id_ultimateio = 1; //!!
static const int hw_id_pacdrive = 0; //!!

#define NUM_OUTPUTS 112
#define NUM_GROUPS 14     // overall groups
#define NUM_LED_GROUPS 12 // first X groups are LEDs, rest is solenoids

/*#define b00000001 0x01
#define b00000010 0x02
#define b00000100 0x04
#define b00001000 0x08
#define b00010000 0x10
#define b00100000 0x20
#define b01000000 0x40
#define b10000000 0x80*/

PACINITIALIZE PacInitialize;
PACSHUTDOWN PacShutdown;
PAC64SETLEDSTATES Pac64SetLEDStates;
PAC64SETLEDSTATE Pac64SetLEDState;
PACSETLEDSTATES PacSetLEDStates;

static HMODULE hio;

static unsigned char *Outputs = NULL;

static unsigned char *OutputsPrevious = NULL;      // to avoid having same outputs sent again
static unsigned short OutputsSolenoidPrevious = 0; // dto.

//

static int GetNumOutputs()
{
	return NUM_OUTPUTS;
}

static int GetNumGroups()
{
	return NUM_GROUPS;
}

static int GetNumLEDGroups()
{
	return NUM_LED_GROUPS;
}

static void PacDriveInitOutputs();

void UnloadPacDrive(void)
{
	if (PacInitialize() > 0)
		PacDriveInitOutputs();
	
#if PRINT_INFO
	printf("\nPlayfield control halted.\n");
#endif

	free(Outputs);
	free(OutputsPrevious);

	FreeLibrary(hio);
}

int LoadPacDrive()
{
#if PRINT_INFO
	printf("\nPlayfield control started.\n");
#endif

	hio = LoadLibrary("PacDrive32");
	if (hio == NULL) return 1;
	
	PacInitialize = (PACINITIALIZE)GetProcAddress(hio, "PacInitialize");
	PacShutdown = (PACSHUTDOWN)GetProcAddress(hio, "PacShutdown");
	Pac64SetLEDStates = (PAC64SETLEDSTATES)GetProcAddress(hio, "Pac64SetLEDStates");
	Pac64SetLEDState = (PAC64SETLEDSTATE)GetProcAddress(hio, "Pac64SetLEDState");
	PacSetLEDStates = (PACSETLEDSTATES)GetProcAddress(hio, "PacSetLEDStates");

	if (!PacInitialize || !PacShutdown || !Pac64SetLEDStates || !Pac64SetLEDState || !PacSetLEDStates) return 1;

	atexit(UnloadPacDrive);

	Outputs = malloc(GetNumOutputs());
	OutputsPrevious = malloc(GetNumGroups());

	if (PacInitialize() == 0)
		return 2;

	PacDriveInitOutputs();
	
	return 0;
}

static void PacDriveInitOutputs() 
{
	int i,x;

	for (i = 0; i < GetNumOutputs(); i++) 
		Outputs[i] = 0;

	for (x = 0; x < GetNumGroups(); x++)
		OutputsPrevious[x] = 255;   // force all leds to be updated

	OutputsSolenoidPrevious = 65535;

	PacDriveUpdateOutputs();
}

/*void PacDriveSetByte(const int ByteNumber, const unsigned char Value)
{
	Outputs[ByteNumber*8+0] = ((Value & b00000001) ? 1 : 0);
	Outputs[ByteNumber*8+1] = ((Value & b00000010) ? 1 : 0);
	Outputs[ByteNumber*8+2] = ((Value & b00000100) ? 1 : 0);
	Outputs[ByteNumber*8+3] = ((Value & b00001000) ? 1 : 0);
	Outputs[ByteNumber*8+4] = ((Value & b00010000) ? 1 : 0);
	Outputs[ByteNumber*8+5] = ((Value & b00100000) ? 1 : 0);
	Outputs[ByteNumber*8+6] = ((Value & b01000000) ? 1 : 0);
	Outputs[ByteNumber*8+7] = ((Value & b10000000) ? 1 : 0);
}*/

void PacDriveSetOutput(const int outputNum, const unsigned char Value)
{
	Outputs[outputNum - 1] = Value;
}

void PacDriveUpdateOutputs()
{
	int group;

	//start with the LEDs
	int ledNum = 1;

	for (group = 1; group <= GetNumLEDGroups(); group++)
	{
		const int outputNum = ledNum - 1;

		unsigned char binary_byte = 0;
		binary_byte |= Outputs[outputNum + 0] << 0;
		binary_byte |= Outputs[outputNum + 1] << 1;
		binary_byte |= Outputs[outputNum + 2] << 2;
		binary_byte |= Outputs[outputNum + 3] << 3;
		binary_byte |= Outputs[outputNum + 4] << 4;
		binary_byte |= Outputs[outputNum + 5] << 5;
		binary_byte |= Outputs[outputNum + 6] << 6;
		binary_byte |= Outputs[outputNum + 7] << 7;

		if (OutputsPrevious[group - 1] != binary_byte)
		{
			OutputsPrevious[group - 1] = binary_byte;

			Pac64SetLEDStates(hw_id_ultimateio, group, binary_byte);
#if PRINT_INFO
			printf("\nSetting LED HwID: %d Group: %d to value %d", hw_id_ultimateio, group, binary_byte);
#endif
		}

		ledNum += 8;
	}

	//now continue with the Solenoids
	const int outputNum = ledNum - 1;

	unsigned short binary_short = 0;
	binary_short |= (unsigned short)Outputs[outputNum + 0] << 0;
	binary_short |= (unsigned short)Outputs[outputNum + 1] << 1;
	binary_short |= (unsigned short)Outputs[outputNum + 2] << 2;
	binary_short |= (unsigned short)Outputs[outputNum + 3] << 3;
	binary_short |= (unsigned short)Outputs[outputNum + 4] << 4;
	binary_short |= (unsigned short)Outputs[outputNum + 5] << 5;
	binary_short |= (unsigned short)Outputs[outputNum + 6] << 6;
	binary_short |= (unsigned short)Outputs[outputNum + 7] << 7;
	binary_short |= (unsigned short)Outputs[outputNum + 8] << 8;
	binary_short |= (unsigned short)Outputs[outputNum + 9] << 9;
	binary_short |= (unsigned short)Outputs[outputNum + 10] << 10;
	binary_short |= (unsigned short)Outputs[outputNum + 11] << 11;
	binary_short |= (unsigned short)Outputs[outputNum + 12] << 12;
	binary_short |= (unsigned short)Outputs[outputNum + 13] << 13;
	binary_short |= (unsigned short)Outputs[outputNum + 14] << 14;
	binary_short |= (unsigned short)Outputs[outputNum + 15] << 15;

	if (OutputsSolenoidPrevious != binary_short)
	{
		OutputsSolenoidPrevious = binary_short;
		PacSetLEDStates(hw_id_pacdrive, binary_short);
#if PRINT_INFO
		printf("\nSetting Solenoid HwID: %d to value %d", hw_id_pacdrive, binary_short);
#endif
	}

	/*
	int ledNum = 1;
	
	for (int group = 1; group <= 12; group++)
	{
		for (int i = 0; i < 8; i++)
		{
			int outputNum = ledNum - 1;
			
			if (OutputsPrevious[outputNum] != Outputs[outputNum])
			{
				if (ledNum == 31) {
					Pac64SetLEDState(hw_id_ultimateio, group, i, Outputs[outputNum]);
					OutputsPrevious[outputNum] = Outputs[outputNum];
#if PRINT_INFO
					printf("\nSetting LED Group: %d LEDNum: %d State: %d", group, ledNum, Outputs[outputNum]);
#endif
				}
			}
			
			ledNum++;
		}
	}
	*/
}

#endif
