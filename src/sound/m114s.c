/**********************************************************************************************
 *
 *   SGS-Thomson Microelectronics M114S/M114A/M114AF Digital Sound Generator
 *   by Steve Ellenoff
 *   09/02/2004
 *
 *   Thanks to R.Belmont for Support & Agreeing to read through that nasty data sheet with me..
 *   Big thanks to Destruk for help in tracking down the data sheet.. Could have never done it
 *   without it!!
 *
 *   Some ideas were taken from the source from BSMT2000 & AY8910
 **********************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"

#define LOG_DATA_IN 0

#if LOG_DATA_IN	
static FILE *fp;	//For logging
#endif

#if 0
#define LOG(x) printf x
#else
#define LOG(x) logerror x
#endif

/**********************************************************************************************

     CONSTANTS

***********************************************************************************************/

#define M114S_CHANNELS			16				// Chip has 16 internal channels for sound output

#define FRAC_BITS				16
#define FRAC_ONE				(1 << FRAC_BITS)
#define FRAC_MASK				(FRAC_ONE - 1)

/**********************************************************************************************

     INTERNAL DATA STRUCTURES

***********************************************************************************************/

/* struct describing a single playing voice/channel */
struct M114SChannel
{
	/* registers */
        UINT8			atten;							/* Attenuation Register */
		UINT8			outputs;						/* Output Pin Register */
		UINT8			table1_addr;					/* Table 1 MSB Starting Address Register */
		UINT8			table2_addr;					/* Table 2 MSB Starting Address Register */
		UINT8			table_len;						/* Table Length Register */
		UINT8			read_meth;						/* Read Meathod Register */
		UINT8			interp;							/* Interpolation Register */
		UINT8			env_enable;						/* Envelope Enable Register */
		UINT8			oct_divisor;					/* Octave Divisor Register */
		UINT8			frequency;						/* Frequency Register */
	/* internal state */
		UINT8			active;							/* is the channel active */
		UINT8			reread[2];						/* # of times to re-read wave for tables 1 & 2 */
        UINT32          position[2];                    /* current position for tables 1 & 2 */
        UINT32          loop_start_position[2];         /* loop start position for tables 1 & 2 */
        UINT32          loop_stop_position[2];          /* loop stop position for tables 1 & 2 */
        UINT32          adjusted_rate;				    /* adjusted rate */
};

struct M114SChip
{
	int			stream;								/* which stream are we using */
	INT8 *		region_base;						/* pointer to the base of the region */

	struct M114SChannel channels[M114S_CHANNELS];	/* All the chip's internal channels */
	struct M114SChannel tempchannel;				/* temporary channel for gathering the data programming */
	int			eatbytes;							/* # of bytes to eat at start up before processing data */
	int			bytes_read;							/* # of bytes read */
	int			channel;							/* Which channel is being programmed via the data bus */
};



/**********************************************************************************************

     GLOBALS

***********************************************************************************************/
static struct M114SChip m114schip[MAX_M114S];		//Each M114S chip

/* Table 1 & Table 2 Repetition Values based on Mode */
static const int mode_to_rep[8][2] = {
  {2,2},	//Mode 0
  {1,1},	//Mode 1
  {4,4},	//Mode 2
  {1,1},	//Mode 3
  {1,2},	//Mode 4
  {1,1},	//Mode 5
  {1,4},	//Mode 6
  {1,1}	    //Mode 7
};

/* Table 1 Length Values based on Mode */
static const int mode_to_len_t1[8][8] = {
{16,32,64,128,256,512,1024,2048 },	//Mode 0
{16,32,64,128,256,512,1024,2048 },	//Mode 1
{16,32,64,128,256,512,1024,1024 },	//Mode 2
{16,32,64,128,256,512,1024,2048 },	//Mode 3
{16,32,64,128,256,512,1024,2048 },	//Mode 4
{16,32,64,128,256,512,1024,2048 },	//Mode 5
{16,32,64,128,256,512,1024,2048 },	//Mode 6
{16,32,64,128,256,512,1024,2048 },	//Mode 7
};

/* Table 2 Length Values based on Mode */
static const int mode_to_len_t2[8][8] = {
{16,32,64,128,256,512,1024,2048 },	//Mode 0
{16,32,64,128,256,512,1024,2048 },	//Mode 1
{16,32,64,128,256,512,1024,1024 },	//Mode 2
{16,32,64,128,256,512,1024,2048 },	//Mode 3
{ 8,16,32, 64,128,256, 512,1024 },	//Mode 4
{16,16,16, 32, 64,128, 256, 512 },	//Mode 5
{ 4, 8,16, 32, 64,128, 256, 512 },	//Mode 6
{16,16,16, 16, 32, 64, 128, 256 },	//Mode 7
};

/* Frequency Table for a 4Mhz Clocked Chip */
static const double freqtable4Mhz[0xff] = {
 1016.78,1021.45,1026.69,1031.46,1036.27,1041.67,1044.39,1045.48,
 1046.57,1047.67,1048.77,1051.52,1056.52,1061.57,1066.67,1071.81,	//0x00 - 0x0F
 1077.01,1082.25,1087.55,1092.90,1098.30,1103.14,1106.81,1107.42,
 1108.65,1109.88,1111.11,1114.21,1119.19,1124.86,1130.58,1135.72,	//0x10 - 0x1F
 1140.90,1146.79,1152.07,1158.08,1163.47,1168.91,1172.33,1173.71,
 1174.40,1175.78,1177.16,1180.64,1186.24,1191.90,1197.60,1203.37,	//0x20 - 0x2F
 1209.19,1215.07,1221.00,1226.99,1232.29,1238.39,1242.24,1243.78,
 1244.56,1245.33,1246.88,1250.78,1256.28,1262.63,1269.04,1274.70,	//0x30 - 0x3F
 1281.23,1287.00,1293.66,1299.55,1305.48,1312.34,1315.79,1317.52,
 1318.39,1319.26,1321.00,1324.50,1331.56,1337.79,1344.09,1350.44,	//0x40 - 0x4F
 1356.85,1363.33,1369.86,1376.46,1383.13,1389.85,1393.73,1395.67,
 1396.65,1397.62,1398.60,1403.51,1410.44,1417.43,1424.50,1430.62,	//0x50 - 0x5F
 1437.81,1445.09,1451.38,1458.79,1466.28,1472.75,1478.20,1479.29,
 1480.38,1481.48,1482.58,1486.99,1494.77,1501.50,1508.30,1516.30,	//0x60 - 0x6F
 1523.23,1530.22,1538.46,1545.60,1552.80,1560.06,1564.95,1566.17,
 1567.40,1568.63,1569.86,1576.04,1583.53,1591.09,1598.72,1606.43,	//0x70 - 0x7F
 1614.21,1622.06,1629.99,1638.00,1644.74,1652.89,1658.37,1659.75,
 1661.13,1662.51,1663.89,1669.45,1677.85,1684.92,1693.48,1702.13,	//0x80 - 0x8F
 1709.40,1718.21,1727.12,1734.61,1743.68,1751.31,1757.47,1759.01,	//(2nd entry in manual shows 1781.21 but that's cleary wrong)
 1760.56,1762.11,1763.89,1768.35,1777.78,1785.71,1793.72,1803.43,	//0x90 - 0x9F
 1811.59,1819.84,1829.83,1838.24,1846.72,1855.29,1860.47,1862.20,
 1863.93,1865.67,1867.41,1874.41,1883.24,1892.15,1901.14,1910.22,	//0xA0 - 0xAF
 1919.39,1928.64,1937.98,1947.42,1956.95,1966.57,1972.39,1974.33,
 1976.28,1978.24,1980.20,1984.13,1994.02,2004.01,2014.10,2024.29,	//0xB0 - 0xBF
 2032.52,2042.90,2053.39,2063.98,2072.54,2083.33,2087.68,2089.86,
 2092.05,2094.24,2096.44,2103.05,2114.16,2123.14,2134.47,2143.62,	//0xC0 - 0xCF
 2155.17,2164.50,2176.28,2185.79,2195.39,2207.51,2212.39,2214.84,
 2217.29,2219.76,2222.22,2227.17,2239.64,2249.72,2259.89,2272.73,	//0xD0 - 0xDF
 2283.11,2293.58,2304.15,2314.81,2325.58,2339.18,2344.67,2347.42,
 2350.18,2352.94,2355.71,2361.28,2372.48,2383.79,2395.21,2406.74,	//0xE0 - 0xEF
 0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,														//0xF0 - 0xFF (Special codes)
};

static INT8 tb1[4096];
static INT8 tb2[4096];
static INT16 out[4096];

static void record_it(int len)
{
	#if LOG_DATA_IN	
		static int recorded=0;
		if(fp && !recorded)
		{
			int i;
			INT8 data;
			recorded = 0;
			for(i=0;i<len;i++)
			{
			data = out[i];
			fputc(data,fp);
			}
		}
	#endif
}

static INT16 read_sample(struct M114SChannel *channel)
{
	static UINT32 indx=0;
	int lent1 = (channel->loop_stop_position[0] - channel->loop_start_position[0]+1);
	int rep1 = channel->reread[0];
	UINT32 incr = (channel->adjusted_rate<<FRAC_BITS)/Machine->sample_rate;
	INT16 sample = 0;
	if(indx < (lent1*rep1) << FRAC_BITS)
	{
//		printf("index = %d, pos = %d\n",index,index>>FRAC_BITS);
		sample = out[indx>>FRAC_BITS];
		indx+= incr;
	}
	else
		indx = 0;
	return sample;
}

static void read_table(struct M114SChip *chip, struct M114SChannel *channel)
{
	int t1start, t2start, lent1,lent2, rep1, rep2, i,j,intp;
	INT16 d;
	INT8 *rom = &chip->region_base[0];
	t1start = channel->loop_start_position[0];
	t2start = channel->loop_start_position[1];
	lent1 = channel->loop_stop_position[0] - channel->loop_start_position[0]+1;
	lent2 = channel->loop_stop_position[1] - channel->loop_start_position[1]+1;
	rep1 = channel->reread[0];
	rep2 = channel->reread[1];
	intp = channel->interp;
	memset(&tb1,0,sizeof(tb1));
	memset(&tb2,0,sizeof(tb2));
	memset(&out,0,sizeof(out));
	
	//Table1 is always larger, so use that as the size
	//Scan Table 1
	for(i=0; i<lent1; i++)
		for(j=0; j<rep1; j++)
			tb1[j+(i*rep1)] = rom[i+t1start];
	//Scan Table 2
	for(i=0; i<lent2; i++)
		for(j=0; j<rep2; j++)
			tb2[j+(i*rep2)] = rom[i+t2start+0x2000];		//A13 is toggled high on Table 2 reading (Implementation specific - ie, Mr. Game)
	//Make up difference with zero?
	for(i=0;i<(lent1-lent2);i++)
		tb2[lent2+i] = 0;

	//Now Mix based on Interpolation Bits
	for(i=0; i<lent1*rep1; i++)	{
		d = (INT16)(tb1[i] * (intp + 1) / 16) + (tb2[i] * (15-intp) / 16);
		out[i] = d;
	}
	//freq out is the value from the frequency table.. only divided by octave bit.
	record_it(lent1*rep1);
}

/**********************************************************************************************

     m114s_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void m114s_update(int num, INT16 **buffer, int length)
{
	struct M114SChip *chip = &m114schip[num];
	struct M114SChannel *channel;
	INT16 *ldest = buffer[0];
	INT16 *rdest = buffer[1];
	INT16 sample;
	int c;
	int remaining = length;

	while (remaining > 0)
	{

		/* loop over channels */
		//for (c = 0; c < M114S_CHANNELS; c++)
		for (c = 2; c < 3; c++)
		{
			channel = &chip->channels[c];
			if(channel->active)
//			if(channel->atten !=0x3f)
			{
				sample = read_sample(channel);
				*ldest++ = sample * 0x7fff;
				*rdest++ = sample * 0x7fff;
			}
			else
			{
				*ldest++ = 0;
				*rdest++ = 0;
			}
		}
		remaining--;
	}
}



/**********************************************************************************************

     M114S_sh_start -- start emulation of the M114S

***********************************************************************************************/

INLINE void init_channel(struct M114SChannel *channel)
 {
	//set all internal registers to 0!
	channel->active = 0;
	channel->reread[0] = 0;
	channel->reread[1] = 0;
 	channel->position[0] = 0;
	channel->position[1] = 0;
	channel->loop_start_position[0] = 0;
	channel->loop_start_position[1] = 0;
	channel->loop_stop_position[0] = 0;
	channel->loop_stop_position[1] = 0;
 	channel->adjusted_rate = 0;
 }
 
 
INLINE void init_all_channels(struct M114SChip *chip)
 {
 	int i;
 
 	/* init the channels */
 	for (i = 0; i < M114S_CHANNELS; i++)
 		init_channel(&chip->channels[i]);

	//Chip init stuff
	memset(&chip->tempchannel,0,sizeof(&chip->tempchannel));
	chip->channel = 0;
	chip->bytes_read = 0;
 }
 
int M114S_sh_start(const struct MachineSound *msound)
{
	const struct M114Sinterface *intf = msound->sound_interface;
	char stream_name[2][40];
	const char *stream_name_ptrs[2];
	int vol[2];
	int i;

//	Machine->sample_rate = 6092;

	memset(&tb1,0,sizeof(tb1));
	memset(&tb2,0,sizeof(tb2));
	memset(&out,0,sizeof(out));

	
	/* initialize the chips */
	memset(&m114schip, 0, sizeof(m114schip));
	for (i = 0; i < intf->num; i++)
	{
		/* generate the name and create the stream */
		sprintf(stream_name[0], "%s #%d Ch1", sound_name(msound), i);
		sprintf(stream_name[1], "%s #%d Ch2", sound_name(msound), i);
		stream_name_ptrs[0] = stream_name[0];
		stream_name_ptrs[1] = stream_name[1];

		/* set the volumes */
		vol[0] = MIXER(intf->mixing_level[i], MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->mixing_level[i], MIXER_PAN_RIGHT);

		/* create the stream */
		m114schip[i].stream = stream_init_multi(2, stream_name_ptrs, vol, Machine->sample_rate, i, m114s_update);
		if (m114schip[i].stream == -1)
			return 1;

		/* initialize the regions */
		m114schip[i].region_base = (INT8 *)memory_region(intf->region[i]);

		/* set up # of eat bytes */
		m114schip[i].eatbytes = intf->eatbytes[i];

		/* init the channels */
		init_all_channels(&m114schip[i]);
	}

	//Open for logging data
	#if LOG_DATA_IN	
	fp = fopen("c:\\m114s.raw","wb");
	#endif

	/* success */
	return 0;
}



/**********************************************************************************************

     M114S_sh_stop -- stop emulation of the M114S

***********************************************************************************************/

void M114S_sh_stop(void)
{
  //Close logging file
  #if LOG_DATA_IN	
  if(fp)	fclose(fp);
  #endif
}



/**********************************************************************************************

     M114S_sh_reset -- reset emulation of the M114S

***********************************************************************************************/

void M114S_sh_reset(void)
{
	int i;
	for (i = 0; i < MAX_M114S; i++)
		init_all_channels(&m114schip[i]);
}


/**********************************************************************************************

     process_freq_codes -- There are up to 16 special values for frequency that signify a code

***********************************************************************************************/
static void process_freq_codes(struct M114SChip *chip)
{
	//Grab pointer to channel being programmed
	struct M114SChannel *channel = &chip->channels[chip->channel];
	switch(channel->frequency)
	{
		//ROMID - ROM Identification  (Are you kidding me?)
		case 0xf8:
			LOG(("* * Channel: %02d: Frequency Code: %02x - ROMID * * \n",chip->channel,channel->frequency));
			break;
		//SSG - Set Syncro Global
		case 0xf9:
			LOG(("* * Channel: %02d: Frequency Code: %02x - SSG * * \n",chip->channel,channel->frequency));
			break;
		//RSS - Reverse Syncro Status
		case 0xfa:
			LOG(("* * Channel: %02d: Frequency Code: %02x - RSS * * \n",chip->channel,channel->frequency));
			break;
		//RSG - Reset Syncro Global
		case 0xfb:
			LOG(("* * Channel: %02d: Frequency Code: %02x - RSG * * \n",chip->channel,channel->frequency));
			break;
		//PSF - Previously Selected Frequency
		case 0xfc:
			LOG(("* * Channel: %02d: Frequency Code: %02x - PSF * * \n",chip->channel,channel->frequency));
			break;
		//FFT - Forced Table Termination
		case 0xff:
			LOG(("* * Channel: %02d: Frequency Code: %02x - FFT * * \n",chip->channel,channel->frequency));
			break;
		default:
			LOG(("* * Channel: %02d: Frequency Code: %02x - UNKNOWN * * \n",chip->channel,channel->frequency));
	}
}

/**********************************************************************************************

     process_channel_data -- complete programming for a channel now exists, process it!

***********************************************************************************************/

static void process_channel_data(struct M114SChip *chip)
{
	//Grab pointer to channel being programmed
	struct M114SChannel *channel = &chip->channels[chip->channel];

	/* Force stream to update */
	stream_update(chip->stream, 0);

	//Reset # of bytes for next group
	chip->bytes_read = 0;

	//Copy data to the appropriate channel from our temp channel
	memcpy(channel,&chip->tempchannel,sizeof(chip->tempchannel));

	//Look for the 16 special frequency codes starting from 0xff
	if(channel->frequency > (0xff-16)) {
			process_freq_codes(chip);
			if(channel->frequency != 0xff)
				return;
	}

	//If Attenuation set to 0x3F - The channel becomes inactive - if it was previously active, force stream update
	if(channel->atten == 0x3f) {
		int update=channel->active;
		channel->active = 0;
		channel->position[0] = 0;
		if(update)	
			stream_update(chip->stream, 0);
		return;
	}

	//Process this channel
	if(0 || chip->channel == 2)
	{
		//Calculate # of repetitions for Table 1 & Table 2
		int rep1 = mode_to_rep[channel->read_meth][0];
		int rep2 = mode_to_rep[channel->read_meth][1];
		//Calculate Table Length for Table 1 & Table 2
		int lent1 = mode_to_len_t1[channel->read_meth][channel->table_len];
		int lent2 = mode_to_len_t2[channel->read_meth][channel->table_len];
		//Calculate Table 1 Start & End Address in ROM
		int t1start = (channel->table1_addr<<5) & (~(lent1-1)&0x1fff);		//T1 Addr is upper 8 bits, but masked by length
		int t1end = t1start + (lent1-1);
		//Calculate Table 2 Start & End Address in ROM
		int t2start = (channel->table2_addr<<5) & (~(lent2-1)&0x1fff);		//T2 Addr is upper 8 bits, but masked by length
		int t2end = t2start + (lent2-1);
		double freq1, freq2;
		freq1 = freq2 = freqtable4Mhz[channel->frequency];

#if 1
		//Freq Table is based on 16 byte length & 1 pass read - adjust for length..
		if(lent1>16) freq1 /= (double)(lent1/16);
		if(lent2>16) freq2 /= (double)(lent2/16);
#endif
		
		//Special case for table length of 16 - Bit 5 always 1 in this case
		if(lent1 == 16) {
			t1start |= 0x10;
			t1end |= 0x10;
		}
		if(lent2 == 16) {
			t2start |= 0x10;
			t2end |= 0x10;
		}

		//Channel is now active!
		channel->active = 1;

		//Adjust frequency if octave divisor set
		if(channel->oct_divisor) channel->frequency = channel->frequency / 2;

#if 0

		//Divide the frequency based on # of times to re-read the chip (1x, 2x, 4x)
		freq1 /= (double)rep1;
		freq2 /= (double)rep2;
#endif

//		wavegen((int)(freq1*128),t1start,lent1,t2start,lent2,chip->channel);

		//Assign Frequency
//		channel->adjusted_rate = (UINT32)((freq1 * (double)FRAC_ONE) / (double)Machine->sample_rate);
		channel->adjusted_rate = freq1 * lent1;

		//Assign loop start & stop
		channel->loop_start_position[0] = t1start;
		channel->loop_stop_position[0] = t1end;
		channel->loop_start_position[1] = t2start;
		channel->loop_stop_position[1] = t2end;

		//Assign # of times to re-read tables 1 & 2
		channel->reread[0] = rep1;
		channel->reread[1] = rep2;

		//Start reading at the start..
		channel->position[0] = t1start;

//		printf("freq = %d\n",channel->adjusted_rate);

		if(out[0] == 0)
			read_table(chip,channel);

		//		wavegen((int)(freq1*128),t1start,lent1,t2start,lent2,chip->channel);

#if 0
		LOG(("V:%02d FQ:%03x TS1:%02x TS2:%02x T1L:%04d T1R:%01d T2L:%04d T2R:%01d OD=%01d I:%02d E:%01d\n",
		channel->atten,
		channel->frequency,
		t1start,t2start,
		lent1,rep1,
		lent2,rep2,
		channel->oct_divisor,
		channel->interp,
		channel->env_enable
		));
#endif

	}
}

/**********************************************************************************************

     m114s_data_write -- handle a write to the data bus of the M114S

     The chip has a data bus width of 6 bits, and must be fed 8 consecutive bytes
	 - thus 48 bits of programming! All data must be fed in order, so we make a few assumptions.

***********************************************************************************************/

static void m114s_data_write(struct M114SChip *chip, data8_t data)
{
	data &= 0x3f;						//Strip off bits 7-8 (only 6 bits for the data bus to the chip)
	chip->bytes_read++;
	switch(chip->bytes_read)
	{
	/*  BYTE #1 - 
	    Bits 0-5: Attenuation Value (0-63) - 0 = No Attenuation, 3E = Max, 3F = Silence active channel */
		case 1:
			chip->tempchannel.atten = data;
			//LOG(("%02x: M114S = %02x, ATTEN = %02x(%02d)\n",locals.chip->bytes_read,data,chip->tempchannel.atten,chip->tempchannel.atten));
			break;
		
	/*  BYTE #2 - 
		Bits 0-1: Table 2 Address (Bits 6-7)
		Bits 2-3: Table 1 Address (Bits 6-7)
		Bits 3-5: Output Pin Selection (0-3)  */
		case 2:
			chip->tempchannel.table2_addr = (data & 0x03)<<6;
			chip->tempchannel.table1_addr = (data & 0x0c)<<4;
			chip->tempchannel.outputs = (data & 0x30)>>4;
			//LOG(("%02x: M114S = %02x, OUTPUTS = %02x, T1ADDR_MSB = %02x, T2ADDR_MSB = %02x\n",locals.chip->bytes_read,data,chip->tempchannel.outputs,chip->tempchannel.table1_addr,chip->tempchannel.table2_addr));
			break;

	/*  BYTE #3 -
		Bits 0-5: Table 2 Address (Bits 0-5) */
		case 3:
			chip->tempchannel.table2_addr |= data;
			//LOG(("%02x: M114S = %02x, T2ADDR_LSB = %02x - T2ADDR=%02x\n",locals.chip->bytes_read,data,data,chip->tempchannel.table2_addr));
			break;

	/*	BYTE #4 -
		Bits 0-5: Table 1 Address (Bits 0-5) */
		case 4:
			chip->tempchannel.table1_addr |= data;
			//LOG(("%02x: M114S = %02x, T1ADDR_LSB = %02x - T1ADDR=%02x\n",locals.chip->bytes_read,data,data,chip->tempchannel.table1_addr));
			break;

	/*  BYTE #5 -
		Bits 0-2: Reading Method
		Bits 3-5: Table Length */
		case 5:
			chip->tempchannel.read_meth = data & 0x07;
			chip->tempchannel.table_len = (data & 0x38)>>3;
			//LOG(("%02x: M114S = %02x, TABLE LEN = %02x, READ METH = %02x\n",locals.chip->bytes_read,data,chip->tempchannel.table_len,chip->tempchannel.read_meth));
			break;

	/*	BYTE #6 -
		Bits 0  : Octave Divisor
		Bits 1  : Envelope Enable/Disable
		Bits 2-5: Interpolation Value (0-4)	*/
		case 6:
			chip->tempchannel.oct_divisor = data & 1;
			chip->tempchannel.env_enable = (data & 2)>>1;
			chip->tempchannel.interp = (data & 0x3c)>>2; 
			//LOG(("%02x: M114S = %02x, INTERP = %02x, ENV ENA = %02x, OCT DIV = %02x\n",locals.chip->bytes_read,data,chip->tempchannel.interp,chip->tempchannel.env_enable,chip->tempchannel.oct_divisor));
			break;

	/*	BYTE #7 -
		Bits 0-1: Frequency (Bits 0-1)
		Bits 2-5: Channel */
		case 7:
			chip->tempchannel.frequency = (data&0x03);
			chip->channel = (data & 0x3c)>>2; 
			//LOG(("%02x: M114S = %02x, CHANNEL = %02d, FREQ D0-D1= %02x\n",locals.chip->bytes_read,data,chip->tempchannel.channel,chip->tempchannel.frequency));
			break;

	/*	BYTE #8 -
		Bits 0-5: Frequency (Bits 2-7) */
		case 8:
			chip->tempchannel.frequency |= (data<<2);
			//LOG(("%02x: M114S = %02x, FREQ D2-D7 = %02x - FREQ=%02x\n",locals.chip->bytes_read,data,data,chip->tempchannel.frequency));

			/* Process the channel data */
			process_channel_data(chip);

			break;

		default:
			LOG(("M114S.C - logic error - too many bytes processed: %x\n",chip->bytes_read));
	}
}



/**********************************************************************************************

     M114S_data_0_w -- handle a write to the current register

***********************************************************************************************/

WRITE_HANDLER( M114S_data_w )
{
	if(m114schip[offset].eatbytes)
		m114schip[offset].eatbytes--;
	else
		m114s_data_write(&m114schip[offset], data);
}
