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

#include "driver.h"

int		VotraxBaseFrequency;		/* Some games (Qbert) change this */
int 	VotraxBaseVolume;
int 	VotraxChannel;

struct  GameSamples *VotraxSamples;

/****************************************************************************
 * 64 Phonemes - currently 1 sample per phoneme, will be combined sometime!
 ****************************************************************************/

static const char *VotraxTable[65] =
{
 "EH3.WAV","EH2.WAV","EH1.WAV","PA0.WAV","DT.WAV" ,"A1.WAV" ,"A2.WAV" ,"ZH.WAV",
 "AH2.WAV","I3.WAV" ,"I2.WAV" ,"I1.WAV" ,"M.WAV"  ,"N.WAV"  ,"B.WAV"  ,"V.WAV",
 "CH.WAV" ,"SH.WAV" ,"Z.WAV"  ,"AW1.WAV","NG.WAV" ,"AH1.WAV","OO1.WAV","OO.WAV",
 "L.WAV"  ,"K.WAV"  ,"J.WAV"  ,"H.WAV"  ,"G.WAV"  ,"F.WAV"  ,"D.WAV"  ,"S.WAV",
 "A.WAV"  ,"AY.WAV" ,"Y1.WAV" ,"UH3.WAV","AH.WAV" ,"P.WAV"  ,"O.WAV"  ,"I.WAV",
 "U.WAV"  ,"Y.WAV"  ,"T.WAV"  ,"R.WAV"  ,"E.WAV"  ,"W.WAV"  ,"AE.WAV" ,"AE1.WAV",
 "AW2.WAV","UH2.WAV","UH1.WAV","UH.WAV" ,"O2.WAV" ,"O1.WAV" ,"IU.WAV" ,"U1.WAV",
 "THV.WAV","TH.WAV" ,"ER.WAV" ,"EH.WAV" ,"E1.WAV" ,"AW.WAV" ,"PA1.WAV","STOP.WAV",
 0
};

void votrax_w(int data);

void sh_votrax_start(int Channel)
{
	VotraxSamples = readsamples(VotraxTable,"votrax");
    VotraxBaseFrequency = 8000;
    VotraxBaseVolume = 230;
    VotraxChannel = Channel;
}

void sh_votrax_stop(void)
{
	freesamples(VotraxSamples);
}

void repeat_votrax_w(int dummy)
{
	votrax_w(-1);
}

void votrax_w(int data)
{
	static int buffer[128];
	static int pos;
	int i,Phoneme,Intonation;

	if (data >= 0) {
		if (pos < 128) buffer[pos++] = data;
	}
	if (pos > 0 && votrax_status_r() < 1) {
		data = buffer[0];
		pos--;
		for (i=0; i < pos; i++)
			buffer[i] = buffer[i+1];
		Phoneme = data & 0x3F;
		Intonation = data >> 6;
		logerror("Speech : %s at intonation %d\n",VotraxTable[Phoneme],Intonation);
/*
		if(Phoneme==63)
			mixer_stop_sample(VotraxChannel);
*/
		if(VotraxSamples->sample[Phoneme])
		{
			mixer_set_volume(VotraxChannel,VotraxBaseVolume+(8*Intonation)*100/255);
			mixer_play_sample(VotraxChannel,VotraxSamples->sample[Phoneme]->data,
					  VotraxSamples->sample[Phoneme]->length,
					  14000 /*VotraxBaseFrequency+(256*Intonation)*/,
					  0);
		}
	} else
		timer_set(TIME_IN_USEC(24000),0,repeat_votrax_w);
}

int votrax_status_r(void)
{
    return mixer_is_sample_playing(VotraxChannel);
}
