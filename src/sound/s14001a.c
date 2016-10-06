// license:BSD-3-Clause
// copyright-holders:Ed Bernard, Jonathan Gevaryahu, hap
// thanks-to:Kevin Horton
/*
    SSi TSI S14001A speech IC emulator
    aka CRC: Custom ROM Controller, designed in 1975, first usage in 1976 on TSI Speech+ calculator
    Originally written for MAME by Jonathan Gevaryahu(Lord Nightmare) 2006-2013,
    replaced with near-complete rewrite by Ed Bernard in 2016

    TODO:
    - nothing at the moment?

    Further reading:
    - http://www.vintagecalculators.com/html/speech-.html
    - http://www.vintagecalculators.com/html/development_of_the_tsi_speech-.html
    - http://www.vintagecalculators.com/html/speech-_state_machine.html
    - https://archive.org/stream/pdfy-QPCSwTWiFz1u9WU_/david_djvu.txt
*/

/* Chip Pinout:
The original datasheet (which is lost as far as I know) clearly called the
s14001a chip the 'CRC chip', or 'Custom Rom Controller', as it appears with
this name on the Stern and Canon schematics, as well as on some TSI speech
print advertisements.
Labels are not based on the labels used by the Atari wolf pack and Stern
schematics, as these are inconsistent. Atari calls the word select/speech address
input pins SAx while Stern calls them Cx. Also Atari and Canon both have the bit
ordering for the word select/speech address bus backwards, which may indicate it
was so on the original datasheet. Stern has it correct, and I've used their Cx
labeling.

                      ______    ______
                    _|o     \__/      |_
            +5V -- |_|1             40|_| -> /BUSY*
                    _|                |_
          ?TEST ?? |_|2             39|_| <- ROM D7
                    _|                |_
 XTAL CLOCK/CKC -> |_|3             38|_| -> ROM A11
                    _|                |_
  ROM CLOCK/CKR <- |_|4             37|_| <- ROM D6
                    _|                |_
  DIGITAL OUT 0 <- |_|5             36|_| -> ROM A10
                    _|                |_
  DIGITAL OUT 1 <- |_|6             35|_| -> ROM A9
                    _|                |_
  DIGITAL OUT 2 <- |_|7             34|_| <- ROM D5
                    _|                |_
  DIGITAL OUT 3 <- |_|8             33|_| -> ROM A8
                    _|                |_
        ROM /EN <- |_|9             32|_| <- ROM D4
                    _|       S        |_
          START -> |_|10 7   1   T  31|_| -> ROM A7
                    _|   7   4   S    |_
      AUDIO OUT <- |_|11 3   0   I  30|_| <- ROM D3
                    _|   7   0        |_
         ROM A0 <- |_|12     1      29|_| -> ROM A6
                    _|       A        |_
SPCH ADR BUS C0 -> |_|13            28|_| <- SPCH ADR BUS C5
                    _|                |_
         ROM A1 <- |_|14            27|_| <- ROM D2
                    _|                |_
SPCH ADR BUS C1 -> |_|15            26|_| <- SPCH ADR BUS C4
                    _|                |_
         ROM A2 <- |_|16            25|_| <- ROM D1
                    _|                |_
SPCH ADR BUS C2 -> |_|17            24|_| <- SPCH ADR BUS C3
                    _|                |_
         ROM A3 <- |_|18            23|_| <- ROM D0
                    _|                |_
         ROM A4 <- |_|19            22|_| -> ROM A5
                    _|                |_
            GND -- |_|20            21|_| -- -10V
                     |________________|

*Note from Kevin Horton when testing the hookup of the S14001A: the /BUSY line
is not a standard voltage line: when it is in its HIGH state (i.e. not busy) it
puts out a voltage of -10 volts, so it needs to be dropped back to a sane
voltage level before it can be passed to any sort of modern IC. The address
lines for the speech rom (A0-A11) do not have this problem, they output at a
TTL/CMOS compatible voltage. The AUDIO OUT pin also outputs a voltage below GND,
and the TEST pins may do so too.

START is pulled high when a word is to be said and the word number is on the
word select/speech address input lines. The Canon 'Canola' uses a separate 'rom
strobe' signal independent of the chip to either enable or clock the speech rom.
It's likely that they did this to be able to force the speech chip to stop talking,
which is normally impossible. The later 'version 3' TSI speech board as featured in
an advertisement in the John Cater book probably also has this feature, in addition
to external speech rom banking.

The Digital out pins supply a copy of the 4-bit waveform which also goes to the
internal DAC. They are only valid every other clock cycle. It is possible that
on 'invalid' cycles they act as a 4 bit input to drive the dac.

Because it requires -10V to operate, the chip manufacturing process must be PMOS.

* Operation:
Put the 6-bit address of the word to be said onto the C0-C5 word select/speech
address bus lines. Next, clock the START line low-high-low. As long as the START
line is held high, the first address byte of the first word will be read repeatedly
every clock, with the rom enable line enabled constantly (i.e. it doesn't toggle on
and off as it normally does during speech). Once START has gone low-high-low, the
/BUSY line will go low until 3 clocks after the chip is done speaking.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "driver.h"
#include "sndintrf.h"
#include "s14001a.h"
#include "streams.h"

#define BOOL int
#define FALSE 0
#define TRUE 1

UINT8 *m_SpeechRom;
int stream;
int VSU1000_amp;

//devcb_write_line m_bsy_handler;
//devcb_read8 m_ext_read_handler;

// internal state
BOOL m_bPhase1; // 1 bit internal clock

enum states
{
	IDLE = 0,
	WORDWAIT,
	CWARMSB,    // read 8 CWAR MSBs
	CWARLSB,    // read 4 CWAR LSBs from rom d7-d4
	DARMSB,     // read 8 DAR  MSBs
	CTRLBITS,   // read Stop, Voiced, Silence, Length, XRepeat
	PLAY,
	DELAY
};

// registers
int m_uStateP1;          // 3 bits, enum 'states'
int m_uStateP2;

UINT16 m_uDAR13To05P1;      // 9 MSBs of delta address register
UINT16 m_uDAR13To05P2;      // incrementing uDAR05To13 advances ROM address by 8 bytes

UINT16 m_uDAR04To00P1;      // 5 LSBs of delta address register
UINT16 m_uDAR04To00P2;      // 3 address ROM, 2 mux 8 bits of data into 2 bit delta
// carry indicates end of quarter pitch period (32 cycles)

UINT16 m_uCWARP1;           // 12 bits Control Word Address Register (syllable)
UINT16 m_uCWARP2;

BOOL m_bStopP1;
BOOL m_bStopP2;
BOOL m_bVoicedP1;
BOOL m_bVoicedP2;
BOOL m_bSilenceP1;
BOOL m_bSilenceP2;
UINT8 m_uLengthP1;          // 7 bits, upper three loaded from ROM length
UINT8 m_uLengthP2;          // middle two loaded from ROM repeat and/or uXRepeat
// bit 0 indicates mirror in voiced mode
// bit 1 indicates internal silence in voiced mode
// incremented each pitch period quarter

UINT8 m_uXRepeatP1;         // 2 bits, loaded from ROM repeat
UINT8 m_uXRepeatP2;
UINT8 m_uDeltaOldP1;        // 2 bit old delta
UINT8 m_uDeltaOldP2;
UINT8 m_uOutputP1;          // 4 bits audio output, calculated during phase 1

// derived signals
BOOL m_bDAR04To00CarryP2;
BOOL m_bPPQCarryP2;
BOOL m_bRepeatCarryP2;
BOOL m_bLengthCarryP2;
UINT16 m_RomAddrP1;         // rom address

// output pins
UINT8 m_uOutputP2;          // output changes on phase2
UINT16 m_uRomAddrP2;        // address pins change on phase 2
BOOL m_bBusyP1;             // busy changes on phase 1

// input pins
BOOL m_bStart;
UINT8 m_uWord;              // 6 bit word noumber to be spoken

// emulator variables
// statistics
UINT32 m_uNPitchPeriods;
UINT32 m_uNVoiced;
UINT32 m_uNControlWords;

// diagnostic output
UINT32 m_uPrintLevel;

UINT8 readmem(UINT16 offset, BOOL phase);
BOOL Clock(void); // called once to toggle external clock twice

// emulator helper functions
UINT8 Mux8To2(BOOL bVoicedP2, UINT8 uPPQtrP2, UINT8 uDeltaAdrP2, UINT8 uRomDataP2);
void CalculateIncrement(BOOL bVoicedP2, UINT8 uPPQtrP2, BOOL bPPQStartP2, UINT8 uDeltaP2, UINT8 uDeltaOldP2, UINT8 *uDeltaOldP1, UINT8 *uIncrementP2, BOOL *bAddP2);
UINT8 CalculateOutput(BOOL bVoicedP2, BOOL bXSilenceP2, UINT8 uPPQtrP2, BOOL bPPQStartP2, UINT8 uLOutputP2, UINT8 uIncrementP2, BOOL bAddP2);
void GetStatistics(UINT32 *uNPitchPeriods, UINT32 *uNVoiced, UINT32 *uNControlWords);

void s14001a_update(int ch, INT16 *buffer, int length);

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

int s14001a_sh_start(const struct MachineSound *msound)
{
	const struct S14001A_interface *intf = msound->sound_interface;

	m_SpeechRom = memory_region(intf->region);

	//!! m_stream = machine().sound().stream_alloc(*this, 0, 1, clock() ? clock() : machine().sample_rate());
#ifdef PINMAME
	stream = stream_init("S14001A", 100, 19000, 0, s14001a_update); //!! 19.5kHz to 34.7kHz?
#else
	stream = stream_init("S14001A", 100, 44100, 0, s14001a_update);
#endif
	if (stream == -1)
		return 1;

	VSU1000_amp = 15;

	// resolve callbacks
	//m_ext_read_handler.resolve();
	//m_bsy_handler.resolve();

	m_bPhase1 = 0;
	m_uStateP1 = 0;
	m_uStateP2 = 0;

	m_uDAR13To05P1 = 0;
	m_uDAR13To05P2 = 0;
	m_uDAR04To00P1 = 0;
	m_uDAR04To00P2 = 0;
	m_uCWARP1 = 0;
	m_uCWARP2 = 0;

	m_bStopP1 = 0;
	m_bStopP2 = 0;
	m_bVoicedP1 = 0;
	m_bVoicedP2 = 0;
	m_bSilenceP1 = 0;
	m_bSilenceP2 = 0;
	m_uLengthP1 = 0;
	m_uLengthP2 = 0;
	m_uXRepeatP1 = 0;
	m_uXRepeatP2 = 0;
	m_uDeltaOldP1 = 0;
	m_uDeltaOldP2 = 0;
	m_bDAR04To00CarryP2 = 0;
	m_bPPQCarryP2 = 0;
	m_bRepeatCarryP2 = 0;
	m_bLengthCarryP2 = 0;
	m_RomAddrP1 = 0;
	m_uRomAddrP2 = 0;
	m_bBusyP1 = 0;
	m_bStart = 0;
	m_uWord = 0;
	m_uNPitchPeriods = 0;
	m_uNVoiced = 0;
	m_uNControlWords = 0;
	m_uPrintLevel = 0;

	m_uOutputP1 = m_uOutputP2 = 7;

	// register for savestates
	//!!
	/*save_item(NAME(m_bPhase1));
	save_item(NAME(m_uStateP1));
	save_item(NAME(m_uStateP2));
	save_item(NAME(m_uDAR13To05P1));
	save_item(NAME(m_uDAR13To05P2));
	save_item(NAME(m_uDAR04To00P1));
	save_item(NAME(m_uDAR04To00P2));
	save_item(NAME(m_uCWARP1));
	save_item(NAME(m_uCWARP2));

	save_item(NAME(m_bStopP1));
	save_item(NAME(m_bStopP2));
	save_item(NAME(m_bVoicedP1));
	save_item(NAME(m_bVoicedP2));
	save_item(NAME(m_bSilenceP1));
	save_item(NAME(m_bSilenceP2));
	save_item(NAME(m_uLengthP1));
	save_item(NAME(m_uLengthP2));
	save_item(NAME(m_uXRepeatP1));
	save_item(NAME(m_uXRepeatP2));
	save_item(NAME(m_uDeltaOldP1));
	save_item(NAME(m_uDeltaOldP2));
	save_item(NAME(m_uOutputP1));

	save_item(NAME(m_bDAR04To00CarryP2));
	save_item(NAME(m_bPPQCarryP2));
	save_item(NAME(m_bRepeatCarryP2));
	save_item(NAME(m_bLengthCarryP2));
	save_item(NAME(m_RomAddrP1));

	save_item(NAME(m_uOutputP2));
	save_item(NAME(m_uRomAddrP2));
	save_item(NAME(m_bBusyP1));
	save_item(NAME(m_bStart));
	save_item(NAME(m_uWord));

	save_item(NAME(m_uNPitchPeriods));
	save_item(NAME(m_uNVoiced));
	save_item(NAME(m_uNControlWords));
	save_item(NAME(m_uPrintLevel));*/

	return 0;
}

void s14001a_sh_stop(void)
{
}

//-------------------------------------------------
//  sound_stream_update - handle a stream update
//-------------------------------------------------

void s14001a_update(int ch, INT16 *buffer, int length)
{
	int i;
	int sample;
	for (i = 0; i < length; i++)
	{
		Clock();
		sample = m_uOutputP2 - 7; // range -7..8
		buffer[i] = (INT16)(sample * 0xf00 * VSU1000_amp / 15);
	}
}


/**************************************************************************
    External interface
**************************************************************************/

int S14001A_bsy_0_r(void)
{
	if (stream != -1)
		stream_update(stream, 0);
#ifdef DEBUGSTATE
	fprintf(stderr,"busy state checked: %d\n",(machineState != 0) );
#endif
	return (m_bBusyP1) ? 1 : 0;
}

#if 0 //!!
READ_LINE_MEMBER(s14001a_device::romen_r)
{
	m_stream->update();
	return (m_bPhase1) ? 1 : 0;
}
#endif

void S14001A_reg_0_w(int data)
{
	if (stream != -1)
		stream_update(stream, 0);
	m_uWord = data & 0x3f; // C0-C5
}

void S14001A_rst_0_w(int data)
{
	if (stream != -1)
		stream_update(stream, 0);
	m_bStart = (data != 0);
	if (m_bStart) m_uStateP1 = WORDWAIT;
}

void S14001A_set_rate(int newrate)
{
#ifdef PINMAME
	//static int rates[8] = { 19000, 20500, 22000, 24500, 27000, 29500, 31000, 33500 };
#endif
	if (stream != -1)
		stream_update(stream, 0);
#ifdef PINMAME
	//if (newrate < 0) newrate = 0;
	//else if (newrate > 7) newrate = 7;
	stream_set_sample_rate(stream, newrate/*rates[newrate]*/);
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
	else if (volume > 15) volume = 15;
#endif
	VSU1000_amp = volume;
}

/**************************************************************************
    Device emulation
**************************************************************************/

UINT8 readmem(UINT16 offset, BOOL phase)
{
	offset &= 0xfff; // 11-bit internal
	return /*((m_ext_read_handler.isnull()) ? */m_SpeechRom[offset /*& (m_SpeechRom.bytes() - 1)*/] /*: m_ext_read_handler(offset))*/;
}

BOOL Clock(void)
{
	// effectively toggles external clock twice, one cycle
	// internal clock toggles on external clock transition from 0 to 1 so internal clock will always transition here
	// return false if some emulator problem detected

	// On the actual chip, all register phase 1 values needed to be refreshed from phase 2 values
	// or else risk losing their state due to charge loss.
	// But on a computer the values are static.
	// So to reduce code clutter, phase 1 values are only modified if they are different
	// from the preceeding phase 2 values.

	if (m_bPhase1)
	{
		// transition to phase2
		m_bPhase1 = FALSE;

		// transfer phase1 variables to phase2
		m_uStateP2     = m_uStateP1;
		m_uDAR13To05P2 = m_uDAR13To05P1;
		m_uDAR04To00P2 = m_uDAR04To00P1;
		m_uCWARP2      = m_uCWARP1;
		m_bStopP2      = m_bStopP1;
		m_bVoicedP2    = m_bVoicedP1;
		m_bSilenceP2   = m_bSilenceP1;
		m_uLengthP2    = m_uLengthP1;
		m_uXRepeatP2   = m_uXRepeatP1;
		m_uDeltaOldP2  = m_uDeltaOldP1;

		m_uOutputP2    = m_uOutputP1;
		m_uRomAddrP2   = m_RomAddrP1;

		// setup carries from phase 2 values
		m_bDAR04To00CarryP2  = m_uDAR04To00P2 == 0x1F;
		m_bPPQCarryP2        = m_bDAR04To00CarryP2 && ((m_uLengthP2&0x03) == 0x03); // pitch period quarter
		m_bRepeatCarryP2     = m_bPPQCarryP2       && ((m_uLengthP2&0x0C) == 0x0C);
		m_bLengthCarryP2     = m_bRepeatCarryP2    && ( m_uLengthP2       == 0x7F);

		return TRUE;
	}
	m_bPhase1 = TRUE;

	// logic done during phase 1
	switch (m_uStateP1)
	{
	case IDLE:
		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;

		//if (m_bBusyP1 && !m_bsy_handler.isnull())
		//	m_bsy_handler(0);
		m_bBusyP1 = FALSE;
		break;

	case WORDWAIT:
		// the delta address register latches the word number into bits 03 to 08
		// all other bits forced to 0.  04 to 08 makes a multiply by two.
		m_uDAR13To05P1 = (m_uWord&0x3C)>>2;
		m_uDAR04To00P1 = (m_uWord&0x03)<<3;
		m_RomAddrP1 = (m_uDAR13To05P1<<3)|(m_uDAR04To00P1>>2); // remove lower two bits
		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = CWARMSB;

		//if (!m_bBusyP1 && !m_bsy_handler.isnull())
		//	m_bsy_handler(1);
		m_bBusyP1 = TRUE;
		break;

	case CWARMSB:
		if (m_uPrintLevel >= 1)
			printf("\n speaking word %02x",m_uWord);

		// use uDAR to load uCWAR 8 msb
		m_uCWARP1 = readmem(m_uRomAddrP2,m_bPhase1)<<4; // note use of rom address setup in previous state
		// increment DAR by 4, 2 lsb's count deltas within a byte
		m_uDAR04To00P1 += 4;
		if (m_uDAR04To00P1 >= 32) m_uDAR04To00P1 = 0; // emulate 5 bit counter
		m_RomAddrP1 = (m_uDAR13To05P1<<3)|(m_uDAR04To00P1>>2); // remove lower two bits

		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = CWARLSB;
		break;

	case CWARLSB:
		m_uCWARP1   = m_uCWARP2|(readmem(m_uRomAddrP2,m_bPhase1)>>4); // setup in previous state
		m_RomAddrP1 = m_uCWARP1;

		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = DARMSB;
		break;

	case DARMSB:
		m_uDAR13To05P1 = readmem(m_uRomAddrP2,m_bPhase1)<<1; // 9 bit counter, 8 MSBs from ROM, lsb zeroed
		m_uDAR04To00P1 = 0;
		m_uCWARP1++;
		m_RomAddrP1 = m_uCWARP1;
		m_uNControlWords++; // statistics

		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = CTRLBITS;
		break;

	case CTRLBITS:
		m_bStopP1 = readmem(m_uRomAddrP2, m_bPhase1) & 0x80 ? TRUE : FALSE;
		m_bVoicedP1 = readmem(m_uRomAddrP2, m_bPhase1) & 0x40 ? TRUE : FALSE;
		m_bSilenceP1 = readmem(m_uRomAddrP2, m_bPhase1) & 0x20 ? TRUE : FALSE;
		m_uXRepeatP1 = readmem(m_uRomAddrP2,m_bPhase1)&0x03;
		m_uLengthP1  =(readmem(m_uRomAddrP2,m_bPhase1)&0x1F)<<2; // includes external length and repeat
		m_uDAR04To00P1 = 0;
		m_uCWARP1++; // gets ready for next DARMSB
		m_RomAddrP1  = (m_uDAR13To05P1<<3)|(m_uDAR04To00P1>>2); // remove lower two bits

		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = PLAY;

		if (m_uPrintLevel >= 2)
			printf("\n cw %d %d %d %d %d",m_bStopP1,m_bVoicedP1,m_bSilenceP1,m_uLengthP1>>4,m_uXRepeatP1);

		break;

	case PLAY:
	{
		UINT8 uDeltaP2;     // signal line
		UINT8 uIncrementP2; // signal lines
		BOOL bAddP2;        // signal line

		// statistics
		if (m_bPPQCarryP2)
		{
			// pitch period end
			if (m_uPrintLevel >= 3)
				printf("\n ppe: RomAddr %03x",m_uRomAddrP2);

			m_uNPitchPeriods++;
			if (m_bVoicedP2) m_uNVoiced++;
		}
		// end statistics

		// modify output
		uDeltaP2 = Mux8To2(m_bVoicedP2,
					m_uLengthP2 & 0x03,     // pitch period quater counter
					m_uDAR04To00P2 & 0x03,  // two bit delta address within byte
					readmem(m_uRomAddrP2,m_bPhase1)
		);
		CalculateIncrement(m_bVoicedP2,
					m_uLengthP2 & 0x03,     // pitch period quater counter
					m_uDAR04To00P2 == 0,    // pitch period quarter start
					uDeltaP2,
					m_uDeltaOldP2,          // input
					&m_uDeltaOldP1,          // output
					&uIncrementP2,           // output 0, 1, or 3
					&bAddP2                  // output
		);
		m_uOutputP1 = CalculateOutput(m_bVoicedP2,
					m_bSilenceP2,
					m_uLengthP2 & 0x03,     // pitch period quater counter
					m_uDAR04To00P2 == 0,    // pitch period quarter start
					m_uOutputP2,            // last output
					uIncrementP2,
					bAddP2
		);

		// advance counters
		m_uDAR04To00P1++;
		if (m_bDAR04To00CarryP2) // pitch period quarter end
		{
			m_uDAR04To00P1 = 0; // emulate 5 bit counter

			m_uLengthP1++; // lower two bits of length count quarter pitch periods
			if (m_uLengthP1 >= 0x80)
			{
				m_uLengthP1 = 0; // emulate 7 bit counter
			}
		}

		if (m_bVoicedP2 && m_bRepeatCarryP2) // repeat complete
		{
			m_uLengthP1 &= 0x70; // keep current "length"
			m_uLengthP1 |= (m_uXRepeatP1<<2); // load repeat from external repeat
			m_uDAR13To05P1++; // advances ROM address 8 bytes
			if (m_uDAR13To05P1 >= 0x200) m_uDAR13To05P1 = 0; // emulate 9 bit counter
		}
		if (!m_bVoicedP2 && m_bDAR04To00CarryP2)
		{
			// unvoiced advances each quarter pitch period
			// note repeat counter not reloaded for non voiced speech
			m_uDAR13To05P1++; // advances ROM address 8 bytes
			if (m_uDAR13To05P1 >= 0x200) m_uDAR13To05P1 = 0; // emulate 9 bit counter
		}

		// construct m_RomAddrP1
		m_RomAddrP1 = m_uDAR04To00P1;
		if (m_bVoicedP2 && m_uLengthP1&0x1) // mirroring
		{
			m_RomAddrP1 ^= 0x1f; // count backwards
		}
		m_RomAddrP1 = (m_uDAR13To05P1<<3) | m_RomAddrP1>>2;

		// next state
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else if (m_bStopP2 && m_bLengthCarryP2) m_uStateP1 = DELAY;
		else if (m_bLengthCarryP2)
		{
			m_uStateP1  = DARMSB;
			m_RomAddrP1 = m_uCWARP1; // output correct address
		}
		else m_uStateP1 = PLAY;
		break;
	}

	case DELAY:
		m_uOutputP1 = 7;
		if (m_bStart) m_uStateP1 = WORDWAIT;
		else          m_uStateP1 = IDLE;
		break;
	}

	return TRUE;
}

UINT8 Mux8To2(BOOL bVoicedP2, UINT8 uPPQtrP2, UINT8 uDeltaAdrP2, UINT8 uRomDataP2)
{
	// pick two bits of rom data as delta

	if (bVoicedP2 && uPPQtrP2&0x01) // mirroring
	{
		uDeltaAdrP2 ^= 0x03; // count backwards
	}
	// emulate 8 to 2 mux to obtain delta from byte (bigendian)
	switch (uDeltaAdrP2)
	{
	case 0x00:
		return (uRomDataP2&0xC0)>>6;
	case 0x01:
		return (uRomDataP2&0x30)>>4;
	case 0x02:
		return (uRomDataP2&0x0C)>>2;
	case 0x03:
		return (uRomDataP2&0x03)>>0;
	}
	return 0xFF;
}

void CalculateIncrement(BOOL bVoicedP2, UINT8 uPPQtrP2, BOOL bPPQStartP2, UINT8 uDelta, UINT8 uDeltaOldP2, UINT8 *uDeltaOldP1, UINT8 *uIncrementP2, BOOL *bAddP2)
{
	static const UINT8 uIncrements[4][4] =
	{
	//    00  01  10  11
		{ 3,  3,  1,  1,}, // 00
		{ 1,  1,  0,  0,}, // 01
		{ 0,  0,  1,  1,}, // 10
		{ 1,  1,  3,  3 }, // 11
	};

        // uPPQtr, pitch period quarter counter; 2 lsb of uLength
	// bPPStart, start of a pitch period
	// implemented to mimic silicon (a bit)

	// beginning of a pitch period
	if (uPPQtrP2 == 0x00 && bPPQStartP2) // note this is done for voiced and unvoiced
	{
		uDeltaOldP2 = 0x02;
	}

#define MIRROR  (uPPQtrP2&0x01)

	// calculate increment from delta, always done even if silent to update uDeltaOld
	// in silicon a PLA determined 0,1,3 and add/subtract and passed uDelta to uDeltaOld
	if (!bVoicedP2 || !MIRROR)
	{
		*uIncrementP2 = uIncrements[uDelta][uDeltaOldP2];
		*bAddP2       = uDelta >= 0x02;
	}
	else
	{
		*uIncrementP2 = uIncrements[uDeltaOldP2][uDelta];
		*bAddP2       = uDeltaOldP2 < 0x02;
	}
	*uDeltaOldP1 = uDelta;
	if (bVoicedP2 && bPPQStartP2 && MIRROR) uIncrementP2 = 0; // no change when first starting mirroring
}

UINT8 CalculateOutput(BOOL bVoiced, BOOL bXSilence, UINT8 uPPQtr, BOOL bPPQStart, UINT8 uLOutput, UINT8 uIncrementP2, BOOL bAddP2)
{
	// implemented to mimic silicon (a bit)
	// limits output to 0x00 and 0x0f
	UINT8 uTmp; // used for subtraction

#define SILENCE (uPPQtr&0x02)

	// determine output
	if (bXSilence || (bVoiced && SILENCE)) return 7;

	// beginning of a pitch period
	if (uPPQtr == 0x00 && bPPQStart) // note this is done for voiced and nonvoiced
	{
		uLOutput = 7;
	}

	// adder
	uTmp = uLOutput;
	if (!bAddP2) uTmp ^= 0x0F; // turns subtraction into addition

	// add 0, 1, 3; limit at 15
	uTmp += uIncrementP2;
	if (uTmp > 15) uTmp = 15;

	if (!bAddP2) uTmp ^= 0x0F; // turns addition back to subtraction
	return uTmp;
}

void GetStatistics(UINT32 *uNPitchPeriods, UINT32 *uNVoiced, UINT32 *uNControlWords)
{
	*uNPitchPeriods = m_uNPitchPeriods;
	*uNVoiced = m_uNVoiced;
	*uNControlWords = m_uNControlWords;
}
