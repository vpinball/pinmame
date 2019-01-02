/**************************************************************************************
 **************************************************************************************
 **                                                                                  **
 **                      PacDrive.c - (Ultra IO Support)                             **
 **                                                                                  **
 **                     Added to PinMAME by Brad Oldham                              **
 **                                                                                  **
 **                          Last Updated:  7/13/15                                  **
 **                                                                                  **
 **************************************************************************************
 **************************************************************************************/

#include "driver.h"
#include "pacdrive.h"

#define PRINT_INFO 1

static const int hw_id = 0; //!!

#define NUM_OUTPUTS 96
#define NUM_GROUPS 12
#define b00000001 0x01
#define b00000010 0x02
#define b00000100 0x04
#define b00001000 0x08
#define b00010000 0x10
#define b00100000 0x20
#define b01000000 0x40
#define b10000000 0x80

static const unsigned char b00000000 = 0x00;
static const unsigned char b11111111 = 0xff;

PACINITIALIZE PacInitialize;
PACSHUTDOWN PacShutdown;
PAC64SETLEDSTATES Pac64SetLEDStates;
PAC64SETLEDSTATE Pac64SetLEDState;

static HMODULE hio;

static unsigned char *Outputs = NULL;
static unsigned *OutputsPrevious = NULL;

//

int GetNumOutputs()
{
	return NUM_OUTPUTS;
}

int GetNumGroups()
{
	return NUM_GROUPS;
}

void UnloadPacDrive()
{
	if (PacInitialize() > 0)
		InitOutputs();
	
#if PRINT_INFO
	printf("\nPlayfield control halted.\n");
#endif

	free(Outputs);
	FreeLibrary(hio);
}

int LoadPacDrive()
{
#if PRINT_INFO
	printf("\nPlayfield control started.\n");
#endif
	
	Outputs = malloc(GetNumOutputs());
	OutputsPrevious = malloc(GetNumGroups());
	
	hio = LoadLibrary("pacdrive32");
	if (hio == NULL) return 1;
	
	PacInitialize = (PACINITIALIZE)GetProcAddress(hio, "PacInitialize");
	PacShutdown = (PACSHUTDOWN)GetProcAddress(hio, "PacShutdown");
	Pac64SetLEDStates = (PAC64SETLEDSTATES)GetProcAddress(hio, "Pac64SetLEDStates");
	Pac64SetLEDState = (PAC64SETLEDSTATE)GetProcAddress(hio, "Pac64SetLEDState");
	
	atexit(UnloadPacDrive);
	
	if (PacInitialize() == 0)
		return 2;

	InitOutputs();
	
	return 0;
}

void InitOutputs() 
{
	int i,x;
	
	for (i = 0; i < GetNumOutputs(); i++) 
		Outputs[i] = 0;
	
	for (x = 0; x < GetNumGroups(); x++)
		OutputsPrevious[x] = 255;   // force all leds to be updated
	
	UpdateOutputs();
}

void PacDriveSetByte(int ByteNumber, unsigned char Value)
{   
	Outputs[ByteNumber*8+0] = ((Value & b00000001) ? 1 : 0);
	Outputs[ByteNumber*8+1] = ((Value & b00000010) ? 1 : 0);
	Outputs[ByteNumber*8+2] = ((Value & b00000100) ? 1 : 0);
	Outputs[ByteNumber*8+3] = ((Value & b00001000) ? 1 : 0);    	
	Outputs[ByteNumber*8+4] = ((Value & b00010000) ? 1 : 0);
	Outputs[ByteNumber*8+5] = ((Value & b00100000) ? 1 : 0);    	
	Outputs[ByteNumber*8+6] = ((Value & b01000000) ? 1 : 0);	
	Outputs[ByteNumber*8+7] = ((Value & b10000000) ? 1 : 0);
}

void PacDriveSetOutput(int outputNum, unsigned char Value)
{ 
    Outputs[outputNum - 1] = Value;
}

void UpdateOutputs()
{
	int group;
    int ledNum = 1;
	
	for (group = 1; group <= GetNumGroups(); group++)
	{		
		unsigned char binary_byte = 0;
		int outputNum = ledNum - 1;
		int i;
		
		for (i = 0; i < 8; i++)
		{
			binary_byte |= Outputs[outputNum + 0] << 0;
			binary_byte |= Outputs[outputNum + 1] << 1;
			binary_byte |= Outputs[outputNum + 2] << 2;
			binary_byte |= Outputs[outputNum + 3] << 3;
			binary_byte |= Outputs[outputNum + 4] << 4;
			binary_byte |= Outputs[outputNum + 5] << 5;
			binary_byte |= Outputs[outputNum + 6] << 6;
			binary_byte |= Outputs[outputNum + 7] << 7;
		}
	
		if (OutputsPrevious[group - 1] != binary_byte)
		{
			OutputsPrevious[group - 1] = binary_byte;
			Pac64SetLEDStates(hw_id, group, binary_byte);
#if PRINT_INFO			
			printf("\nSetting LED Group: %d to value %d", group, binary_byte);
#endif
		}

		ledNum += 8;
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
					Pac64SetLEDState(hw_id, group, i, Outputs[outputNum]);
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
