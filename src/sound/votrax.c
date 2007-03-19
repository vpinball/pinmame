/**************************************************************************

	Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk
	Tom.Haukap@t-online.de

**************************************************************************

VOTRAXSC01_sh_start  - Start emulation, load samples from Votrax subdirectory
VOTRAXSC01_sh_stop   - End emulation, free memory used for samples
votraxsc01_w         - Write data to votrax port
votraxsc01_status_r  - Return busy status (1 = busy)

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable VotraxBaseFrequency, this is defaulted to 8000

// #define REAL_DEVICE
Uncomment the above line if you like to use a real SC-01 chip connected to
the printer port 1. Connect the data lines P0-P5 to D1-D6, I0-I1 to D7-D6,
ACK to ACK and STROBE to STROBE.

**************************************************************************/



#define VERBOSE 1

#if VERBOSE
#define LOG(x) logerror x
#else
#define LOG(x)
#endif

#include "driver.h"
#include "votrax.h"
#include "windows.h"
#ifdef REAL_DEVICE
#include "dlportio.h"
#endif
#include "math.h"

static struct {
	int	baseFrequency;
	int busy;
	int channels[4];

	int actPhoneme;
	int actIntonation;

	struct VOTRAXSC01interface *intf;

	INT16* pActPos;
	int	iRemainingSamples;

	void* timer;

	INT16 *lpBuffer;
	INT16* pBufferPos;

	int   iSamplesInBuffer;
	int	  iDelay;

	unsigned char strobe;
} votraxsc01_locals;

#include "vtxsmpls.inc"

const int _dataOutAdr	= 0x378;
const int _statusInAdr	= 0x379;
const int _ctrlOutAdr  = 0x37a;

const byte _bitmaskAck = 0x40;

const byte _pitchMask = 0x40;

#if VERBOSE
static const char *PhonemeNames[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A2" ,"A1" ,"ZH",
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

static const int PhonemeLengths[65] =
{
	 59,  71, 121,  47,  47,  71, 103,  90,
	 71,  55,  80, 121, 103,  80,  71,  71,
	 71, 121,  71, 146, 121, 146, 103, 185,
	103,  80,  47,  71,  71, 103,  55,  90,
	185,  65,  80,  47, 250, 103, 185, 185,
	185, 103,  71,  90, 185,  80, 185, 103,
	 90,  71, 103, 185,  80, 121,  59,  90,
	 80,  71, 146, 185, 121, 250, 185,  47,
	0
};

#define PT_NS 0
#define PT_V  1
#define PT_VF 2
#define PT_F  3
#define PT_N  4
#define PT_VS 5
#define PT_FS 6


// int sample_rate[4] = {22050, 22550, 23050, 23550};
long sample_rate[4] = {22050, 22050, 22050, 22050};

INLINE int time_to_samples(int ms)
{
	return sample_rate[votraxsc01_locals.actIntonation]*ms/1000;
}

void PrepareVoiceData(int nextPhoneme, int nextIntonation)
{
	int iNextRemainingSamples;
	INT16 *pNextPos, *lpHelp;

	int iFadeOutSamples;
	int iFadeOutPos;

	int iFadeInSamples;
	int iFadeInPos;

	int doMix;
	int AdditionalSamples;
	int dwCount, i;

	INT16 data;

	if ( votraxsc01_locals.lpBuffer )
		free(votraxsc01_locals.lpBuffer);
	votraxsc01_locals.lpBuffer = NULL;

	AdditionalSamples = 0;
	if ( PhonemeData[votraxsc01_locals.actPhoneme].iType>=PT_VS && votraxsc01_locals.actPhoneme!=nextPhoneme ) {
		AdditionalSamples = PhonemeData[votraxsc01_locals.actPhoneme].iSecondStart;
	}

	if ( PhonemeData[nextPhoneme].iType>=PT_VS ) {
		// 'stop phonemes' will stop playing until the next phoneme is send
		votraxsc01_locals.iRemainingSamples = 0;
		return;
	}

	// length of samples to produce
	dwCount = time_to_samples(PhonemeData[nextPhoneme].iLengthms);

	votraxsc01_locals.iSamplesInBuffer = dwCount+AdditionalSamples;
	votraxsc01_locals.lpBuffer = (INT16*) malloc(votraxsc01_locals.iSamplesInBuffer*sizeof(INT16));

	if ( AdditionalSamples )
		memcpy(votraxsc01_locals.lpBuffer, PhonemeData[votraxsc01_locals.actPhoneme].lpStart[votraxsc01_locals.actIntonation], AdditionalSamples*sizeof(INT16));

	lpHelp = votraxsc01_locals.lpBuffer + AdditionalSamples;

	iNextRemainingSamples = 0;
	pNextPos = NULL;

	iFadeOutSamples = 0;
	iFadeOutPos     = 0;

	iFadeInSamples   = 0;
	iFadeInPos       = 0;

	doMix = 0;

	// set up processing
	if ( PhonemeData[votraxsc01_locals.actPhoneme].sameAs!=PhonemeData[nextPhoneme].sameAs  ) {
		// do something, if they are the same all FadeIn/Out values are 0,
		// the buffer is simply filled with the samples of the new phoneme

		switch ( PhonemeData[votraxsc01_locals.actPhoneme].iType ) {
			case PT_NS:
				// "fade" out NS:
				iFadeOutSamples = time_to_samples(30);
				iFadeOutPos = 0;

				// fade in new phoneme
				iFadeInPos = -time_to_samples(30);
				iFadeInSamples = time_to_samples(30);
				break;

			case PT_V:
			case PT_VF:
				switch ( PhonemeData[nextPhoneme].iType ){
					case PT_F:
					case PT_VF:
						// V-->F, V-->VF: fade out 30 ms fade in from 30 ms to 60 ms without mixing
						iFadeOutPos = 0;
						iFadeOutSamples = time_to_samples(30);

						iFadeInPos = -time_to_samples(30);
						iFadeInSamples = time_to_samples(30);
						break;

					case PT_N:
						// V-->N: fade out 40 ms fade from 0 ms to 40 ms without mixing
						iFadeOutPos = 0;
						iFadeOutSamples = time_to_samples(40);

						iFadeInPos = -time_to_samples(10);
						iFadeInSamples = time_to_samples(10);
						break;

					default:
						// fade out 20 ms, no fade in from 10 ms to 30 ms
						iFadeOutPos = 0;
						iFadeOutSamples = time_to_samples(20);

						iFadeInPos = -time_to_samples(0);
						iFadeInSamples = time_to_samples(20);
						break;
				}
				break;

			case PT_N:
				switch ( PhonemeData[nextPhoneme].iType ){
					case PT_V:
					case PT_VF:
						// N-->V, N-->VF: fade out 30 ms fade in from 10 ms to 50 ms without mixing
						iFadeOutPos = 0;
						iFadeOutSamples = time_to_samples(30);

						iFadeInPos = -time_to_samples(10);
						iFadeInSamples = time_to_samples(40);
						break;

					default:
						break;
				}

			case PT_VS:
			case PT_FS:
				iFadeOutPos = 0;
				iFadeOutSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength[votraxsc01_locals.actIntonation] - PhonemeData[votraxsc01_locals.actPhoneme].iSecondStart;
				votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart[votraxsc01_locals.actIntonation] + PhonemeData[votraxsc01_locals.actPhoneme].iSecondStart;
				votraxsc01_locals.iRemainingSamples = iFadeOutSamples;
				doMix = 1;

				iFadeInPos = -time_to_samples(0);
				iFadeInSamples = time_to_samples(0);

				break;

			default:
				// fade out 30 ms, no fade in
				iFadeOutPos = 0;
				iFadeOutSamples = time_to_samples(20);

				iFadeInPos = -time_to_samples(20);
				break;
		}

		if ( !votraxsc01_locals.iDelay ) {
			// this is true if after a stop and a phoneme was send a second phoneme is send
			// during the delay time of the chip. Ignore the first phoneme data
			iFadeOutPos = 0;
			iFadeOutSamples = 0;
		}

	}
	else {
		// the next one is of the same type as the previous one; continue to use the samples of the last phoneme
		iNextRemainingSamples = votraxsc01_locals.iRemainingSamples;
		pNextPos = votraxsc01_locals.pActPos;
	}

	for (i=0; i<dwCount; i++)
	{
		data = 0x00;

		// fade out
		if ( iFadeOutPos<iFadeOutSamples )
		{
			double dFadeOut = 1.0;

			if ( !doMix )
				dFadeOut = 1.0-sin((1.0*iFadeOutPos/iFadeOutSamples)*3.1415/2);

			if ( !votraxsc01_locals.iRemainingSamples ) {
				votraxsc01_locals.iRemainingSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength[votraxsc01_locals.actIntonation];
				votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart[votraxsc01_locals.actIntonation];
			}

			data = (INT16) (*votraxsc01_locals.pActPos++ * dFadeOut);

			votraxsc01_locals.iRemainingSamples--;
			iFadeOutPos++;
		}

		// fade in or copy
		if ( iFadeInPos>=0 )
		{
			double dFadeIn = 1.0;

			if ( iFadeInPos<iFadeInSamples ) {
				dFadeIn = sin((1.0*iFadeInPos/iFadeInSamples)*3.1415/2);
				iFadeInPos++;
			}

			if ( !iNextRemainingSamples ) {
				iNextRemainingSamples = PhonemeData[nextPhoneme].iLength[nextIntonation];
				pNextPos = PhonemeData[nextPhoneme].lpStart[nextIntonation];
			}

			data += (INT16) (*pNextPos++ * dFadeIn);

			iNextRemainingSamples--;
		}
		iFadeInPos++;

		*lpHelp++ = data;
	}

	votraxsc01_locals.pBufferPos = votraxsc01_locals.lpBuffer;

	votraxsc01_locals.pActPos = pNextPos;
	votraxsc01_locals.iRemainingSamples = iNextRemainingSamples;
}

WRITE_HANDLER(votraxsc01_w)
{
	int Phoneme, Intonation;

	Phoneme = data & 0x3F;
	Intonation = (data >> 6)&0x03;

	LOG(("Votrax SC-01: %s at intonation %d\n", PhonemeNames[Phoneme], Intonation));

#ifndef REAL_DEVICE

//	votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart;
//	votraxsc01_locals.iRemainingSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength;
//	timer_reset(votraxsc01_locals.timer, PhonemeLengths[votraxsc01_locals.actPhoneme]/1000.0);
//	timer_enable(votraxsc01_locals.timer,1);

	PrepareVoiceData(Phoneme, Intonation);

	if ( votraxsc01_locals.actPhoneme==0x3f )
		votraxsc01_locals.iDelay = time_to_samples(20);

	if ( !votraxsc01_locals.busy )
	{
		votraxsc01_locals.busy = -1;
		if ( votraxsc01_locals.intf->BusyCallback[0] )
			(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);
	}
#else
	DlPortWritePortUshort(_dataOutAdr, data);

	votraxsc01_locals.strobe = 0x00;
	DlPortWritePortUshort(_ctrlOutAdr, votraxsc01_locals.strobe);
#endif

	votraxsc01_locals.actPhoneme = Phoneme;
	votraxsc01_locals.actIntonation = Intonation;
}

#ifdef REAL_DEVICE
int get_busy()
{
	if ( !((DlPortReadPortUchar(_statusInAdr)&_bitmaskAck)>0) )
		return -1;
	else
		return 0;

}
#endif

READ_HANDLER(votraxsc01_status_r)
{
#ifdef REAL_DEVICE
	return get_busy();
#else
	return votraxsc01_locals.busy;
#endif
}

void votraxsc01_set_base_frequency(int baseFrequency)
{
	int i;
	if ( baseFrequency>=0 )
		votraxsc01_locals.baseFrequency = baseFrequency;
	for (i=0; i < 4; i++) {
		stream_set_sample_rate(votraxsc01_locals.channels[i], baseFrequency);
	}
}

static void Votrax_Update(int num, INT16 *buffer, int length)
{
#ifndef REAL_DEVICE
	int samplesToCopy;

	if ( num!=votraxsc01_locals.actIntonation ) {
		memset(buffer, 0x00, length*sizeof(INT16));
		return;
	}

	while ( length ) {
//		if ( votraxsc01_locals.iRemainingSamples==0 ) {
//			votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart;
//			votraxsc01_locals.iRemainingSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength;
//		}

//		samplesToCopy = (length<=votraxsc01_locals.iRemainingSamples)?length:votraxsc01_locals.iRemainingSamples;
//
//		memcpy(buffer, votraxsc01_locals.pActPos, samplesToCopy*sizeof(INT16));
//		buffer += samplesToCopy;

//		votraxsc01_locals.pActPos += samplesToCopy;
//		votraxsc01_locals.iRemainingSamples -= samplesToCopy;

//		length -= samplesToCopy;

		if ( votraxsc01_locals.iDelay ) {
			samplesToCopy = (length<=votraxsc01_locals.iDelay)?length:votraxsc01_locals.iDelay;

			memset(buffer, 0x00, samplesToCopy*sizeof(INT16));
			buffer += samplesToCopy;

			votraxsc01_locals.iDelay -= samplesToCopy;
		}
		else if ( votraxsc01_locals.iSamplesInBuffer==0 ) {
			if ( votraxsc01_locals.busy ) {
				votraxsc01_locals.busy = 0;
				if ( votraxsc01_locals.intf->BusyCallback[0] )
					(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);
			}

			if ( votraxsc01_locals.iRemainingSamples==0 ) {
				if ( PhonemeData[votraxsc01_locals.actPhoneme].iType>=PT_VS ) {
					votraxsc01_locals.pActPos = PhonemeData[0x3f].lpStart[0];
					votraxsc01_locals.iRemainingSamples = PhonemeData[0x3f].iLength[0];
				}
				else {
					votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart[votraxsc01_locals.actIntonation];
					votraxsc01_locals.iRemainingSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength[votraxsc01_locals.actIntonation];
				}

			}

			samplesToCopy = (length<=votraxsc01_locals.iRemainingSamples)?length:votraxsc01_locals.iRemainingSamples;

			memcpy(buffer, votraxsc01_locals.pActPos, samplesToCopy*sizeof(INT16));
			buffer += samplesToCopy;

			votraxsc01_locals.pActPos += samplesToCopy;
			votraxsc01_locals.iRemainingSamples -= samplesToCopy;

			length -= samplesToCopy;
		}
		else {
			samplesToCopy = (length<=votraxsc01_locals.iSamplesInBuffer)?length:votraxsc01_locals.iSamplesInBuffer;

			memcpy(buffer, votraxsc01_locals.pBufferPos, samplesToCopy*sizeof(INT16));
			buffer += samplesToCopy;

			votraxsc01_locals.pBufferPos += samplesToCopy;
			votraxsc01_locals.iSamplesInBuffer -= samplesToCopy;

			length -= samplesToCopy;
		}
	}
#endif
}

static void VOTRAXSC01_sh_start_timeout(int which)
{
#ifdef REAL_DEVICE
	int help;

	if ( !votraxsc01_locals.strobe ) {
		votraxsc01_locals.strobe = 0x01;
		DlPortWritePortUshort(_ctrlOutAdr, votraxsc01_locals.strobe);
	}

	help = get_busy();

	if ( votraxsc01_locals.busy!=help ) {
		votraxsc01_locals.busy = help;

		if ( votraxsc01_locals.intf->BusyCallback[0] )
			(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);
	}
#endif
}

int VOTRAXSC01_sh_start(const struct MachineSound *msound)
{
	static char s[32];
#ifndef REAL_DEVICE
	int i;
#endif

	memset(&votraxsc01_locals, 0x00, sizeof votraxsc01_locals);

	votraxsc01_locals.intf = msound->sound_interface;

    votraxsc01_locals.baseFrequency = votraxsc01_locals.intf->baseFrequency[0];
	if ( votraxsc01_locals.baseFrequency<=0 )
		votraxsc01_locals.baseFrequency = 8000;

//	votraxsc01_locals.busy = -1;
//	votraxsc01_locals.baseFrequency = 8000;

	votraxsc01_locals.actPhoneme = 0x3f;
#ifndef REAL_DEVICE
	// votraxsc01_locals.pActPos = PhonemeData[votraxsc01_locals.actPhoneme].lpStart;
	// votraxsc01_locals.iRemainingSamples = PhonemeData[votraxsc01_locals.actPhoneme].iLength;
	// votraxsc01_locals.timer = timer_alloc(VOTRAXSC01_sh_start_timeout);
	// timer_adjust(votraxsc01_locals.timer, PhonemeLengths[votraxsc01_locals.actPhoneme]/1000.0, 0, 0);
	// timer_enable(votraxsc01_locals.timer,0);

	PrepareVoiceData(votraxsc01_locals.actPhoneme, votraxsc01_locals.actIntonation);

	for (i=0; i<=3; i++) {
		sprintf(s, "Votrax-SC01 #%d Int %d", 0, i);
		votraxsc01_locals.channels[i] = stream_init(s, votraxsc01_locals.intf->mixing_level[0], sample_rate[i], i, Votrax_Update);
//		set_RC_filter(votraxsc01_locals.channels[i], 270000, 15000, 0, 10000);
	}

//	if ( votraxsc01_locals.intf->BusyCallback[0] )
//		(*votraxsc01_locals.intf->BusyCallback[0])(votraxsc01_locals.busy);

#else
	DlPortWritePortUshort(_dataOutAdr, 0x3f);
	votraxsc01_locals.strobe = 0x00;
	DlPortWritePortUshort(_ctrlOutAdr, votraxsc01_locals.strobe);

	votraxsc01_locals.timer = timer_alloc(VOTRAXSC01_sh_start_timeout);
	timer_adjust(votraxsc01_locals.timer, 0, 0, 0.00001);
	timer_enable(votraxsc01_locals.timer,1);
#endif
	return 0;
}

void VOTRAXSC01_sh_stop(void)
{
	if ( votraxsc01_locals.timer )
		timer_remove(votraxsc01_locals.timer);
	votraxsc01_locals.timer = 0;

	if ( votraxsc01_locals.lpBuffer ) {
		free(votraxsc01_locals.lpBuffer);
		votraxsc01_locals.lpBuffer = NULL;
	}

#ifdef REAL_DEVICE
	DlPortWritePortUshort(_ctrlOutAdr, 0x01);
#endif
}
