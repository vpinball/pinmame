/*

 TSI S14001A emulator v1.11
 By Jonathan Gevaryahu ("Lord Nightmare") with help from Kevin Horton ("kevtris")
 MAME conversion and integration by R. Belmont

 Copyright (c) 2007 Jonathan Gevaryahu.

 Version history:
 0.8 initial version - LN
 0.9 MAME conversion, glue code added - R. Belmont
 1.0 partly fixed stream update - LN (0.111u4)
 1.01 fixed clipping problem - LN (0.111u5)
 1.1 add VSU-1000 features, fully fixed stream update by fixing word latching - LN (0.111u6)
 1.11 fix signedness of output, pre-multiply, fixes clicking on VSU-1000 volume change - LN (0.111u7)

 TODO:
 * increase accuracy of internal S14001A 'filter' for both driven and undriven cycles (its not terribly inaccurate for undriven cycles, but the dc sliding of driven cycles is not emulated)
 * add option for and attach Frank P.'s emulation of the Analog external filter from the vsu-1000 using the discrete core.
*/

/* state map:

 * state machine 1: odd/even clock state
 * on even clocks, audio output is floating, /romen is low so rom data bus is driven, input is latched?
 * on odd clocks, audio output is driven, /romen is high, state machine 2 is clocked
 * *****
 * state machine 2: decoder state
 * NOTE: holding the start line high forces the state machine 2 state to go to or remain in state 1!
 * state 0(Idle): Idle (no sample rom bus activity, output at 0), next state is 0(Idle)

 * state 1(GetHiWord):
 *   grab byte at (wordinput<<1) -> register_WH
 *   reset output DAC accumulator to 0x8 <- ???
 *   reset OldValHi to 1
 *   reset OldValLo to 0
 *   next state is 2(GetLoWord) UNLESS the PLAY line is still high, in which case the state remains at 1

 * state 2(GetLoWord):
 *   grab byte at (wordinput<<1)+1 -> register_WL
 *   next state is 3(GetHiPhon)

 * state 3(GetHiPhon):
 *   grab byte at ((register_WH<<8) + (register_WL))>>4 -> phoneaddress
 *   next state is 4(GetLoPhon)

 * state 4(GetLoPhon):
 *   grab byte at (((register_WH<<8) + (register_WL))>>4)+1 -> playparams
 *   set phonepos register to 0
 *   set oddphone register to 0
 *   next state is 5(PlayForward1)
 *   playparams:
 *   7 6 5 4 3 2 1 0
 *   G                G = LastPhone
 *     B              B = PlayMode
 *       Y            Y = Silenceflag
 *         S S S      S = Length count load value
 *               R R  R = Repeat count reload value (upon carry/overflow of 3 bits)
 *   load the repeat counter with the bits 'R R 0'
 *   load the length counter with the bits 'S S S 0'
 *   NOTE: though only three bits of the length counter load value are controllable, there is a fourth lower bit which is assumed 0 on start and controls the direction of playback, i.e. forwards or backwards within a phone.
 *   NOTE: though only two bits of the repeat counter reload value are controllable, there is a third bit which is loaded to 0 on phoneme start, and this hidden low-order bit of the counter itself is what controls whether the output is forced to silence in mirrored mode. the 'carry' from the highest bit of the 3 bit counter is what increments the address pointer for pointing to the next phoneme in mirrored mode


 *   shift register diagram:
 *   F E D C B A 9 8 7 6 5 4 3 2 1 0
 *   <new byte here>
 *               C C                 C = Current delta sample read point
 *                   O O             O = Old delta sample read point
 * I *OPTIMIZED OUT* the shift register by making use of the fact that the device reads each rom byte 4 times

 * state 5(PlayForward1):
 *   grab byte at (((phoneaddress<<8)+(oddphone*8))+(phonepos>>2)) -> PlayRegister high end, bits F to 8
 *   if Playmode is mirrored, set OldValHi and OldValLo to 1 and 0 respectively, otherwise leave them with whatever was in them before.
 *   Put OldValHi in bit 7 of PlayRegister
 *   Put OldValLo in bit 6 of PlayRegister
 *   Get new OldValHi from bit 9
 *   Get new OldValLo from bit 8
 *   feed current delta (bits 9 and 8) and olddelta (bits 7 and 6) to delta demodulator table, delta demodulator table applies a delta to the accumulator, accumulator goes to enable/disable latch which Silenceflag enables or disables (forces output to 0x8 on disable), then to DAC to output.
 *   next state: state 6(PlayForward2)

 * state 6(PlayForward2):
 *   grab byte at (((phoneaddress<<8)+oddphone)+(phonepos>>2)) -> PlayRegister bits D to 6.
 *   Put OldValHi in bit 7 of PlayRegister\____already done by above operation
 *   Put OldValLo in bit 6 of PlayRegister/
 *   Get new OldValHi from bit 9
 *   Get new OldValLo from bit 8
 *   feed current delta (bits 9 and 8) and olddelta (bits 7 and 6) to delta demodulator table, delta demodulator table applies a delta to the accumulator, accumulator goes to enable/disable latch which Silenceflag enables or disables (forces output to 0x8 on disable), then to DAC to output.
 *   next state: state 7(PlayForward3)

 * state 7(PlayForward3):
 *   grab byte at (((phoneaddress<<8)+oddphone)+(phonepos>>2)) -> PlayRegister bits B to 4.
 *   Put OldValHi in bit 7 of PlayRegister\____already done by above operation
 *   Put OldValLo in bit 6 of PlayRegister/
 *   Get new OldValHi from bit 9
 *   Get new OldValLo from bit 8
 *   feed current delta (bits 9 and 8) and olddelta (bits 7 and 6) to delta demodulator table, delta demodulator table applies a delta to the accumulator, accumulator goes to enable/disable latch which Silenceflag enables or disables (forces output to 0x8 on disable), then to DAC to output.
 *   next state: state 8(PlayForward4)

 * state 8(PlayForward4):
 *   grab byte at (((phoneaddress<<8)+oddphone)+(phonepos>>2)) -> PlayRegister bits 9 to 2.
 *   Put OldValHi in bit 7 of PlayRegister\____already done by above operation
 *   Put OldValLo in bit 6 of PlayRegister/
 *   Get new OldValHi from bit 9
 *   Get new OldValLo from bit 8
 *   feed current delta (bits 9 and 8) and olddelta (bits 7 and 6) to delta demodulator table, delta demodulator table applies a delta to the accumulator, accumulator goes to enable/disable latch which Silenceflag enables or disables (forces output to 0x8 on disable), then to DAC to output.
 *   Call function: increment address

 *   next state: depends on playparams:
 *     if we're in mirrored mode, next will be LoadAndPlayBackward1

 * state 9(LoadAndPlayBackward1)
 * state 10(PlayBackward2)
 * state 11(PlayBackward3)
 * state 12(PlayBackward4)
*/

/* increment address function:
 *   increment repeat counter
        if repeat counter produces a carry, do two things:
           1. if mirrored mode is ON, increment oddphone. if oddphone carries out (i.e. if it was 1), increment phoneaddress and zero oddphone
       2. increment lengthcounter. if lengthcounter carries out, we're done this phone.
 *   increment output counter
 *      if mirrored mode is on, output direction is
 *   if mirrored mode is OFF, increment oddphone. if not, don't touch it here. if oddphone was 1 before the increment, increment phoneaddress and set oddphone to 0
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "driver.h"
#include "sndintrf.h"
#include "s14001a.h"
#include "streams.h"

int stream;

UINT8 WordInput; // value on word input bus
UINT8 LatchedWord; // value latched from input bus
UINT16 SyllableAddress; // address read from word table
UINT16 PhoneAddress; // starting/current phone address from syllable table
UINT8 PlayParams; // playback parameters from syllable table
UINT8 PhoneOffset; // offset within phone
UINT8 LengthCounter; // 4-bit counter which holds the inverted length of the word in phones, leftshifted by 1
UINT8 RepeatCounter; // 3-bit counter which holds the inverted number of repeats per phone, leftshifted by 1
UINT8 OutputCounter; // 2-bit counter to determine forward/backward and output/silence state.
UINT8 machineState; // chip state machine state
UINT8 nextstate; // chip state machine's new state
UINT8 laststate; // chip state machine's previous state, needed for mirror increment masking
UINT8 resetState; // reset line state
UINT8 oddeven; // odd versus even cycle toggle
UINT8 GlobalSilenceState; // same as above but for silent syllables instead of silent portions of mirrored syllables
UINT8 OldDelta; // 2-bit old delta value
UINT8 DACOutput; // 4-bit DAC Accumulator/output
UINT8 audioout; // filtered audio output
UINT8 *SpeechRom; // array to hold rom contents, mame will not need this, will use a pointer
UINT8 filtervals[8];
UINT8 VSU1000_amp; // amplitude setting on VSU-1000 board
UINT16 VSU1000_freq; // frequency setting on VSU-1000 board
UINT16 VSU1000_counter; // counter for freq divider

//#define DEBUGSTATE

#define SILENCE 0x77 // value output when silent

#define LASTSYLLABLE ((PlayParams & 0x80)>>7)
#define MIRRORMODE ((PlayParams & 0x40)>>6)
#define SILENCEFLAG ((PlayParams & 0x20)>>5)
#define LENGTHCOUNT ((PlayParams & 0x1C)>>1) // remember: its 4 bits and the bottom bit is always zero!
#define REPEATCOUNT ((PlayParams<<1)&0x6) // remember: its 3 bits and the bottom bit is always zero!
#define LOCALSILENCESTATE ((OutputCounter & 0x2) && (MIRRORMODE)) // 1 when silent output, 0 when DAC output.

static INT8 DeltaTable[4][4] =
{
	{ (INT8)0xCD, (INT8)0xCD, (INT8)0xEF, (INT8)0xEF, },
	{ (INT8)0xEF, (INT8)0xEF, 0x00, 0x00, },
	{ 0x00, 0x00, 0x11, 0x11, },
	{ 0x11, 0x11, 0x33, 0x33  },
};

static UINT8 audiofilter(void) /* rewrite me to better match the real filter! */
{
	UINT16 temp1, temp2 = 0;
	/* crappy averaging filter! */
	for (temp1 = 0; temp1 < 8; temp1++) { temp2 += filtervals[temp1]; }
	temp2 >>= 3;
	return temp2;
}

static void shiftIntoFilter(UINT8 inputvalue)
{
	UINT8 temp1;
	for (temp1 = 7; temp1 > 0; temp1--)
	{
		filtervals[temp1] = filtervals[(temp1 - 1)];
	}
	filtervals[0] = inputvalue;

}

static void PostPhoneme(void) /* figure out what the heck to do after playing a phoneme */
{
#ifdef DEBUGSTATE
	fprintf(stderr,"0: entered PostPhoneme\n");
#endif
	RepeatCounter++; // increment the repeat counter
	OutputCounter++; // increment the output counter
	if (MIRRORMODE) // if mirroring is enabled
	{
#ifdef DEBUGSTATE
		fprintf(stderr,"1: MIRRORMODE was on\n");
#endif
		if (RepeatCounter == 0x8) // exceeded 3 bits?
		{
#ifdef DEBUGSTATE
			fprintf(stderr,"2: RepeatCounter was == 8\n");
#endif
			// reset repeat counter, increment length counter
			// but first check if lowest bit is set
			RepeatCounter = REPEATCOUNT; // reload repeat counter with reload value
			if (LengthCounter & 0x1) // if low bit is 1 (will carry after increment)
			{
#ifdef DEBUGSTATE
				fprintf(stderr,"3: LengthCounter's low bit was 1\n");
#endif
				PhoneAddress+=8; // go to next phone in this syllable
			}
			LengthCounter++;
			if (LengthCounter == 0x10) // if Length counter carried out of 4 bits
			{
#ifdef DEBUGSTATE
				fprintf(stderr,"3: LengthCounter overflowed\n");
#endif
				SyllableAddress += 2; // go to next syllable
				nextstate = LASTSYLLABLE ? 13 : 3; // if we're on the last syllable, go to end state, otherwise go and load the next syllable.
			}
			else
			{
#ifdef DEBUGSTATE
				fprintf(stderr,"3: LengthCounter's low bit wasn't 1 and it didn't overflow\n");
#endif
				PhoneOffset = (OutputCounter&1) ? 7 : 0;
				nextstate = (OutputCounter&1) ? 9 : 5;
			}
		}
		else // repeatcounter did NOT carry out of 3 bits so leave length counter alone
		{
#ifdef DEBUGSTATE
			fprintf(stderr,"2: RepeatCounter is less than 8 (its actually %d)\n", RepeatCounter);
#endif
			PhoneOffset = (OutputCounter&1) ? 7 : 0;
			nextstate = (OutputCounter&1) ? 9 : 5;
		}
	}
	else // if mirroring is NOT enabled
	{
#ifdef DEBUGSTATE
		fprintf(stderr,"1: MIRRORMODE was off\n");
#endif
		if (RepeatCounter == 0x8) // exceeded 3 bits?
		{
#ifdef DEBUGSTATE
			fprintf(stderr,"2: RepeatCounter was == 8\n");
#endif
			// reset repeat counter, increment length counter
			RepeatCounter = REPEATCOUNT; // reload repeat counter with reload value
			LengthCounter++;
			if (LengthCounter == 0x10) // if Length counter carried out of 4 bits
			{
#ifdef DEBUGSTATE
				fprintf(stderr,"3: LengthCounter overflowed\n");
#endif
				SyllableAddress += 2; // go to next syllable
				nextstate = LASTSYLLABLE ? 13 : 3; // if we're on the last syllable, go to end state, otherwise go and load the next syllable.
#ifdef DEBUGSTATE
				fprintf(stderr,"nextstate is now %d\n", nextstate); // see line below, same reason.
#endif
				return; // need a return here so we don't hit the 'nextstate = 5' line below
			}
		}
		PhoneAddress += 8; // regardless of counters, the phone address always increments in non-mirrored mode
		PhoneOffset = 0;
		nextstate = 5;
	}
#ifdef DEBUGSTATE
	fprintf(stderr,"nextstate is now %d\n", nextstate);
#endif
}

void s14001a_clock(void) /* called once per clock */
{
	UINT8 CurDelta; // Current delta

	/* on even clocks, audio output is floating, /romen is low so rom data bus is driven, input is latched?
	 * on odd clocks, audio output is driven, /romen is high, state machine 2 is clocked */
	oddeven = !(oddeven); // invert the clock
	if (oddeven == 0) // even clock
        {
		audioout = audiofilter(); // function to handle output filtering by internal capacitance based on clock speed and such
#ifdef PINMAME
		if (!machineState) audioout = SILENCE;
#endif
		shiftIntoFilter(audioout); // shift over all the filter outputs and stick in audioout
	}
	else // odd clock
	{
		// fix dac output between samples. theoretically this might be unnecessary but it would require some messy logic in state 5 on the first sample load.
		if (GlobalSilenceState || LOCALSILENCESTATE)
		{
			DACOutput = SILENCE;
			OldDelta = 2;
		}
		audioout = (GlobalSilenceState || LOCALSILENCESTATE) ? SILENCE : DACOutput; // when either silence state is 1, output silence.
#ifdef PINMAME
		if (!machineState) audioout = SILENCE;
#endif
		shiftIntoFilter(audioout); // shift over all the filter outputs and stick in audioout
		switch(machineState) // HUUUUUGE switch statement
		{
		case 0: // idle state
			nextstate = 0;
			break;
		case 1: // read starting syllable high byte from word table
			SyllableAddress = 0; // clear syllable address
			SyllableAddress |= SpeechRom[(LatchedWord<<1)]<<4;
			nextstate = resetState ? 1 : 2;
			break;
		case 2: // read starting syllable low byte from word table
			SyllableAddress |= SpeechRom[(LatchedWord<<1)+1]>>4;
			nextstate = 3;
			break;
		case 3: // read starting phone address
			PhoneAddress = SpeechRom[SyllableAddress]<<4;
			nextstate = 4;
			break;
		case 4: // read playback parameters and prepare for play
			PlayParams = SpeechRom[SyllableAddress+1];
			GlobalSilenceState = SILENCEFLAG; // load phone silence flag
			LengthCounter = LENGTHCOUNT; // load length counter
			RepeatCounter = REPEATCOUNT; // load repeat counter
			OutputCounter = 0; // clear output counter and disable mirrored phoneme silence indirectly via LOCALSILENCESTATE
			PhoneOffset = 0; // set offset within phone to zero
			OldDelta = 0x2; // set old delta to 2 <- is this right?
			DACOutput = 0x88; // set DAC output to center/silence position (0x88)
			nextstate = 5;
			break;
		case 5: // Play phone forward, shift = 0 (also load)
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0xc0)>>6; // grab current delta from high 2 bits of high nybble
			DACOutput += DeltaTable[CurDelta][OldDelta]; // send data to forward delta table and add result to accumulator
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 6;
			break;
		case 6: // Play phone forward, shift = 2
	   		CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0x30)>>4; // grab current delta from low 2 bits of high nybble
			DACOutput += DeltaTable[CurDelta][OldDelta]; // send data to forward delta table and add result to accumulator
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 7;
			break;
		case 7: // Play phone forward, shift = 4
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0xc)>>2; // grab current delta from high 2 bits of low nybble
			DACOutput += DeltaTable[CurDelta][OldDelta]; // send data to forward delta table and add result to accumulator
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 8;
			break;
		case 8: // Play phone forward, shift = 6 (increment address if needed)
			CurDelta = SpeechRom[(PhoneAddress)+PhoneOffset]&0x3; // grab current delta from low 2 bits of low nybble
			DACOutput += DeltaTable[CurDelta][OldDelta]; // send data to forward delta table and add result to accumulator
			OldDelta = CurDelta; // Move current delta to old
			PhoneOffset++; // increment phone offset
			if (PhoneOffset == 0x8) // if we're now done this phone
			{
				/* call the PostPhoneme Function */
				PostPhoneme();
			}
			else
			{
				nextstate = 5;
			}
			break;
		case 9: // Play phone backward, shift = 6 (also load)
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0x3); // grab current delta from low 2 bits of low nybble
			if (laststate != 8) // ignore first (bogus) dac change in mirrored backwards mode. observations and the patent show this.
			{
				DACOutput -= DeltaTable[OldDelta][CurDelta]; // send data to forward delta table and subtract result from accumulator
			}
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 10;
			break;
		case 10: // Play phone backward, shift = 4
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0xc)>>2; // grab current delta from high 2 bits of low nybble
			DACOutput -= DeltaTable[OldDelta][CurDelta]; // send data to forward delta table and subtract result from accumulator
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 11;
			break;
		case 11: // Play phone backward, shift = 2
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0x30)>>4; // grab current delta from low 2 bits of high nybble
			DACOutput -= DeltaTable[OldDelta][CurDelta]; // send data to forward delta table and subtract result from accumulator
			OldDelta = CurDelta; // Move current delta to old
			nextstate = 12;
			break;
		case 12: // Play phone backward, shift = 0 (increment address if needed)
			CurDelta = (SpeechRom[(PhoneAddress)+PhoneOffset]&0xc0)>>6; // grab current delta from high 2 bits of high nybble
			DACOutput -= DeltaTable[OldDelta][CurDelta]; // send data to forward delta table and subtract result from accumulator
			OldDelta = CurDelta; // Move current delta to old
			PhoneOffset--; // decrement phone offset
			if (PhoneOffset == 0xFF) // if we're now done this phone
			{
				/* call the PostPhoneme() function */
				PostPhoneme();
			}
			else
			{
				nextstate = 9;
			}
			break;
		case 13: // For those pedantic among us, consume an extra two clocks like the real chip does.
			nextstate = 0;
			break;
		}
#ifdef DEBUGSTATE
		fprintf(stderr, "Machine state is now %d, was %d, PhoneOffset is %d\n", nextstate, machineState, PhoneOffset);
#endif
		laststate = machineState;
		machineState = nextstate;
	}
}

/**************************************************************************
   MAME glue code
 **************************************************************************/

static void s14001a_update(int ch, INT16 *buffer, int length)
{
	int i;

	for (i = 0; i < length; i++)
	{
		if (--VSU1000_counter <= 0) {
		  s14001a_clock();
		  VSU1000_counter = VSU1000_freq;
		}
#ifdef PINMAME
		buffer[i] = ((((INT16)audioout)-128)*36)*((21 + 2 * VSU1000_amp) / 5);
#else
		buffer[i] = ((((INT16)audioout)-128)*36)*VSU1000_amp;
#endif
	}
}

int s14001a_sh_start(const struct MachineSound *msound)
{
	const struct S14001A_interface *intf = msound->sound_interface;
	int i;

	GlobalSilenceState = 1;
	OldDelta = 0x02;
	DACOutput = SILENCE;
	VSU1000_amp = 0; /* reset by /reset line */
	VSU1000_freq = 1; /* base-1; reset by /reset line */
	VSU1000_counter = 1; /* base-1; not reset by /reset line but this is the best place to reset it */

	for (i = 0; i < 8; i++)
	{
		filtervals[i] = SILENCE;
	}

	SpeechRom = memory_region(intf->region);

#ifdef PINMAME
	stream = stream_init("S14001A", 100, 19000, 0, s14001a_update);
#else
	stream = stream_init("S14001A", 100, 44100, 0, s14001a_update);
#endif
	if (stream == -1)
		return 1;

	return 0;
}

void s14001a_sh_stop(void)
{
}

int S14001A_bsy_0_r(void)
{
	if (stream != -1)
		stream_update(stream, 0);
#ifdef DEBUGSTATE
	fprintf(stderr,"busy state checked: %d\n",(machineState != 0) );
#endif
	return machineState;
}

void S14001A_reg_0_w(int data)
{
	if (stream != -1)
		stream_update(stream, 0);
	WordInput = data;
}

void S14001A_rst_0_w(int data)
{
	if (stream != -1)
		stream_update(stream, 0);
    LatchedWord = WordInput;
	resetState = (data==1);
	machineState = resetState ? 1 : machineState;
}

void S14001A_set_rate(int newrate)
{
#ifdef PINMAME
	static int rates[8] = { 19000, 20500, 22000, 24500, 27000, 29500, 31000, 33500 };
#endif
	if (stream != -1)
		stream_update(stream, 0);
#ifdef PINMAME
	if (newrate < 0) newrate = 0;
	else if (newrate > 7) newrate = 7;
	stream_set_sample_rate(stream, rates[newrate]);
#else
	VSU1000_freq = newrate;
#endif
}

void S14001A_set_volume(int volume)
{
	if (stream != -1)
		stream_update(stream, 0);
#ifdef PINMAME
	if (volume < 0) volume = 0;
	else if (volume > 7) volume = 7;
#endif
    VSU1000_amp = volume;
}
