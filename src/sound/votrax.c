/**************************************************************************

	Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk
	Tom.Haukap@t-online.de

**************************************************************************

VOTRAXSC01_sh_start  - Start emulation, load samples from Votrax subdirectory
sh_votrax_stop   - End emulation, free memory used for samples
votrax_w		 - Write data to votrax port
votrax_status    - Return busy status (-1 = busy)

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable VotraxBaseFrequency, this is defaulted to 8000

**************************************************************************/

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#include "driver.h"
#include "votrax.h"

static struct {
	int	baseFrequency;
	int busy;
	int channel;

	struct VOTRAXSC01interface *intf;

	unsigned char* pActPos;
	int	iRemainingSamples;

	unsigned char* pActPos1;
	int	iRemainingSamples1;

	int iLastValue;
} votraxsc01_locals;

#include "vtxsmpls.inc"

#if VERBOSE
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
#endif

WRITE_HANDLER(votraxsc01_w)
{
	int Phoneme,Intonation;

	Phoneme = data & 0x3F;
	Intonation = data >> 6;
	LOG(("Votrax SC-01: %s at intonation %d\n",PhonemeNames[Phoneme],Intonation));

	if ( Phoneme==0x3f ) {
		votraxsc01_locals.iRemainingSamples = 0;
		votraxsc01_locals.iRemainingSamples1 = 0;
		return;
	}

	if ( votraxsc01_locals.iRemainingSamples ) {
		votraxsc01_locals.pActPos1           = PhonemeData[Phoneme].pStart;
		votraxsc01_locals.iRemainingSamples1 = PhonemeData[Phoneme].iLength;
	}
	else {
		votraxsc01_locals.pActPos           = PhonemeData[Phoneme].pStart;
		votraxsc01_locals.iRemainingSamples = PhonemeData[Phoneme].iLength;
	}

	if ( !votraxsc01_locals.busy ) {
		votraxsc01_locals.busy = 1;
		if ( votraxsc01_locals.intf->BusyCallback[0] )
			(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);
	}
}

READ_HANDLER(votraxsc01_status_r)
{
    return votraxsc01_locals.busy;
}

void votraxsc01_set_base_freqency(int baseFrequency)
{
	if ( baseFrequency>=0 )
		votraxsc01_locals.baseFrequency = baseFrequency;
}

static void Votrax_Update(int num, INT16 *buffer, int length)
{
	if ( votraxsc01_locals.iRemainingSamples<length && votraxsc01_locals.pActPos )
		votraxsc01_locals.iLastValue = (0x80-*(votraxsc01_locals.pActPos+votraxsc01_locals.iRemainingSamples-1))*0x00f0;

	while ( length && votraxsc01_locals.iRemainingSamples ) {
		*buffer++ = (0x80-*votraxsc01_locals.pActPos++)*0x0f0;
		length--;
		votraxsc01_locals.iRemainingSamples--;
	}

	if ( !votraxsc01_locals.iRemainingSamples && votraxsc01_locals.iRemainingSamples1 ) {
		votraxsc01_locals.iRemainingSamples = votraxsc01_locals.iRemainingSamples1;
		votraxsc01_locals.pActPos = votraxsc01_locals.pActPos1;

		votraxsc01_locals.iRemainingSamples1 = 0;
	}

	while ( length && votraxsc01_locals.iRemainingSamples ) {
		*buffer++ = (0x80-*votraxsc01_locals.pActPos++)*0x0f0;
		length--;
		votraxsc01_locals.iRemainingSamples--;
	}

	while ( length ) {
		*buffer++ = votraxsc01_locals.iLastValue;
		length--;
	}

	if ( (votraxsc01_locals.iRemainingSamples<=200) && (!votraxsc01_locals.iRemainingSamples1) ) {
		if ( votraxsc01_locals.busy ) {
			votraxsc01_locals.busy = 0;
			if ( votraxsc01_locals.intf->BusyCallback[0] )
				(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);
		}
	}
}

int VOTRAXSC01_sh_start(const struct MachineSound *msound)
{
	memset(&votraxsc01_locals, 0x00, sizeof votraxsc01_locals);

	votraxsc01_locals.intf = msound->sound_interface;

    votraxsc01_locals.baseFrequency = votraxsc01_locals.intf->baseFrequency[0];
	if ( votraxsc01_locals.baseFrequency<=0 )
		votraxsc01_locals.baseFrequency = 8000;

	votraxsc01_locals.busy = 0;

	votraxsc01_locals.iRemainingSamples  = 0;
	votraxsc01_locals.iRemainingSamples1 = 0;
	votraxsc01_locals.iLastValue = 0x0000;

	votraxsc01_locals.channel = stream_init("SND VOTRAX-SC01", votraxsc01_locals.intf->mixing_level[0], votraxsc01_locals.baseFrequency, 0, Votrax_Update);
    set_RC_filter(votraxsc01_locals.channel, 270000, 15000, 0, 10000);

		if ( votraxsc01_locals.intf->BusyCallback[0] )
			(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);

	return 0;
}

void VOTRAXSC01_sh_stop(void)
{
}
