/**************************************************************************

	Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk

**************************************************************************

sh_votrax_start  - Start emulation, load samples from Votrax subdirectory
sh_votrax_stop   - End emulation, free memory used for samples
votrax_w		 - Write data to votrax port
votrax_status    - Return busy status (-1 = busy)

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable VotraxBaseFrequency, this is defaulted to 8000

**************************************************************************/

#define VERBOSE 1

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#include "driver.h"
#include "votrax.h"

int		VotraxBaseFrequency;
int     VotraxBusy;
int 	VotraxChannel;

void (*busy_callback)(int state);

unsigned char* pActPos;
int	iRemainingSamples;

unsigned char* pActPos1;
int	iRemainingSamples1;

int iLastValue;

#include "vtxsmpls.inc"

static const char *PhonemeNames[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

int votrax_status_r(void)
{
    return VotraxBusy;
}

void votrax_set_busy_func(void (*busy_func)(int state))
{
	busy_callback = busy_func;
}

void votrax_set_base_freqency(int baseFrequency)
{
	VotraxBaseFrequency = baseFrequency;
}

void votrax_repeat_w(int dummy)
{
	votrax_w(-1);
}

void votrax_w(int data)
{
	int Phoneme,Intonation;

	Phoneme = data & 0x3F;
	Intonation = data >> 6;
	LOG(("Speech : %s at intonation %d\n",PhonemeNames[Phoneme],Intonation));

	if ( iRemainingSamples ) {
		pActPos1           = PhonemeData[Phoneme].pStart;
		iRemainingSamples1 = PhonemeData[Phoneme].iLength;
	}
	else {
		pActPos           = PhonemeData[Phoneme].pStart;
		iRemainingSamples = PhonemeData[Phoneme].iLength;
	}

	if ( !VotraxBusy ) {
		VotraxBusy = 1;
		if ( busy_callback )
			(*busy_callback)(VotraxBusy);
	}
}

static void Votrax_Update(int num, INT16 *buffer, int length)
{
	if ( iRemainingSamples<length )
		iLastValue = (0x80-*(pActPos+iRemainingSamples-1))*0x00f0;

	while ( length && iRemainingSamples ) {
		*buffer++ = (0x80-*pActPos++)*0x0f0;
		length--;
		iRemainingSamples--;
	}

	if ( !iRemainingSamples && iRemainingSamples1 ) {
		iRemainingSamples = iRemainingSamples1;
		pActPos = pActPos1;

		iRemainingSamples1 = 0;
	}

	while ( length && iRemainingSamples ) {
		*buffer++ = (0x80-*pActPos++)*0x0f0;
		length--;
		iRemainingSamples--;
	}

	while ( length ) {
		*buffer++ = iLastValue;
		length--;
	}

	if ( (iRemainingSamples<=200) && (!iRemainingSamples1) ) {
		if ( VotraxBusy ) {
			VotraxBusy = 0;
			if ( busy_callback )
				(*busy_callback)(VotraxBusy);
		}
	}
}

static int votrax_sh_start(const struct MachineSound *msound) {
    VotraxBaseFrequency = 7000;
	VotraxBusy = 0;

	iRemainingSamples  = 0;
	iRemainingSamples1 = 0;

	iLastValue = 0x0000;

	VotraxChannel = stream_init("SND DAC", 100, VotraxBaseFrequency, 0, Votrax_Update);
    set_RC_filter(VotraxChannel, 270000, 15000, 0, 10000);
	return 0;
}

static void votrax_sh_stop(void)
{
}

struct CustomSound_interface votrax_custInt = {votrax_sh_start, votrax_sh_stop};
