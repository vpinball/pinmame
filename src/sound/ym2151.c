/*******************************************************************************
*
*   Yamaha YM2151 driver
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "driver.h"
#include "state.h"
#include "ym2151.h"


/* undef this to not use MAME timer system */
#define USE_MAME_TIMERS

/* #define FM_EMU */
#ifdef FM_EMU
	#define INLINE static __inline__
	#ifdef USE_MAME_TIMERS
		#undef USE_MAME_TIMERS
	#endif
#endif
#ifdef USE_MAME_TIMERS
	/* #define LOG_CYM_FILE */
	#ifdef LOG_CYM_FILE
		FILE * cymfile = NULL;
		void * cymfiletimer = 0;
	#endif
#endif


/* struct describing a single operator */
typedef struct{
	UINT32		phase;					/* accumulated operator phase */
	UINT32		freq;					/* operator frequency count */
	INT32		dt1;					/* current DT1 (detune 1 phase inc/decrement) value */
	UINT32		mul;					/* frequency count multiply */
	UINT32		dt1_i;					/* DT1 index * 32 */
	UINT32		dt2;					/* current DT2 (detune 2) value */

	signed int *connect;				/* operator output 'direction' */

	/* channel specific data; note: each operator number 0 contains channel specific data */
	UINT32		fb_shift;				/* feedback shift value for operators 0 in each channel */
	INT32		fb_out_curr;			/* operator feedback value (used only by operators 0) */
	INT32		fb_out_prev;			/* previous feedback value (used only by operators 0) */
	UINT32		kc;						/* channel KC (copied to all operators) */
	UINT32		kc_i;					/* just for speedup */
	UINT32		pms;					/* channel PMS */
	UINT32		ams;					/* channel AMS */
	/* end of channel specific data */

	UINT32		AMmask;					/* LFO Amplitude Modulation enable mask */
	UINT32		state;					/* Envelope state: 4-attack(AR) 3-decay(D1R) 2-sustain(D2R) 1-release(RR) 0-off */
	UINT32		delta_ar;				/* volume delta (attack state) */
	UINT32		tl;						/* Total attenuation Level */
	INT32		volume;					/* current envelope attenuation level */
	UINT32		delta_d1r;				/* volume delta (decay state) */
	UINT32		d1l;					/* envelope switches to sustain state after reaching this level */
	UINT32		delta_d2r;				/* volume delta (sustain state) */
	UINT32		delta_rr;				/* volume delta (release state) */

	UINT32		key;					/* 0=last key was KEY OFF, 1=last key was KEY ON */

	UINT32		ks;						/* key scale    */
	UINT32		ar;						/* attack rate  */
	UINT32		d1r;					/* decay rate   */
	UINT32		d2r;					/* sustain rate */
	UINT32		rr;						/* release rate */

	UINT32		reserved0;				/**/
	UINT32		reserved1;				/**/
	UINT32		reserved2;				/**/

} YM2151Operator;


typedef struct
{
	YM2151Operator	oper[32];			/* the 32 operators */

	UINT32		pan[16];				/* channels output masks (0xffffffff = enable) */

	UINT32		lfo_phase;				/* accumulated LFO phase          */
	UINT32		lfo_freq;				/* LFO frequency count            */
	UINT32		lfo_wave;				/* LFO waveform (0-saw, 1-square, 2-triangle, 3-random noise) */
	UINT32		pmd;					/* LFO Phase Modulation Depth     */
	UINT32		amd;					/* LFO Amplitude Modulation Depth */
	UINT32		lfa;					/* LFO current AM output          */
	INT32		lfp;					/* LFO current PM output          */

	UINT32		test;					/* TEST register */
	UINT32		ct;						/* output control pins (bit7-CT2, bit6-CT1) */

	UINT32		noise;					/* noise enable/period register (bit 7 - noise enable, bits 4-0 - noise period */
	UINT32		noise_rng;				/* 17 bit noise shift register */
	UINT32		noise_p;				/* current noise 'phase'*/
	UINT32		noise_f;				/* current noise period */

	UINT32		csm_req;				/* CSM  KEY ON / KEY OFF sequence request */

	UINT32		irq_enable;				/* IRQ enable for timer B (bit 3) and timer A (bit 2); bit 7 - CSM mode (keyon to all slots, everytime timer A overflows) */
	UINT32		status;					/* chip status (BUSY, IRQ Flags) */
	UINT32		connect[8];				/* channels connections */

#ifdef USE_MAME_TIMERS
/* ASG 980324 -- added for tracking timers */
	void		*timer_A;
	void		*timer_B;
	double		timer_A_time[1024];		/* timer A times for MAME */
	double		timer_B_time[256];		/* timer B times for MAME */
#else
	UINT8		tim_A;					/* timer A enable (0-disabled) */
	UINT8		tim_B;					/* timer B enable (0-disabled) */
	INT32		tim_A_val;				/* current value of timer A */
	INT32		tim_B_val;				/* current value of timer B */
	UINT32		tim_A_tab[1024];		/* timer A deltas */
	UINT32		tim_B_tab[256];			/* timer B deltas */
#endif
	UINT32		timer_A_index;			/* timer A index */
	UINT32		timer_B_index;			/* timer B index */
	UINT32		timer_A_index_old;		/* timer A previous index */
	UINT32		timer_B_index_old;		/* timer B previous index */

	/*	Frequency-deltas to get the closest frequency possible.
	*	There are 11 octaves because of DT2 (max 950 cents over base frequency)
	*	and LFO phase modulation (max 800 cents below AND over base frequency)
	*	Summary:   octave  explanation
	*              0       note code - LFO PM
	*              1       note code
	*              2       note code
	*              3       note code
	*              4       note code
	*              5       note code
	*              6       note code
	*              7       note code
	*              8       note code
	*              9       note code + DT2 + LFO PM
	*              10      note code + DT2 + LFO PM
	*/
	UINT32		freq[11*768];			/* 11 octaves, 768 'cents' per octave */

	/*	Frequency deltas for DT1. These deltas alter operator frequency
	*	after it has been taken from frequency-deltas table.
	*/
	INT32		dt1_freq[8*32];			/* 8 DT1 levels, 32 KC values */

	UINT32		eg_tab   [32+64+32];	/* Envelope Generator deltas (32 + 64 rates + 32 RKS) */
	UINT32		lfo_tab  [256];			/* LFO frequency deltas */
	UINT32		noise_tab[32];			/* 17bit Noise Generator periods */

	void (*irqhandler)(int irq);		/* IRQ function handler */
	mem_write_handler porthandler;		/* port write function handler */

	unsigned int clock;					/* chip clock in Hz (passed from 2151intf.c) */
	unsigned int sampfreq;				/* sampling frequency in Hz (passed from 2151intf.c) */

} YM2151;


/*
*	Shifts below are subject to change when sampling frequency changes...
*/
#define FREQ_SH			16  /* 16.16 fixed point (frequency calculations) */
#define ENV_SH			16  /* 16.16 fixed point (envelope calculations)  */
#define LFO_SH			23  /*  9.23 fixed point (LFO calculations)       */
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

#define FREQ_MASK		((1<<FREQ_SH)-1)
#define ENV_MASK		((1<<ENV_SH)-1)

#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(128.0/ENV_LEN)
#define ENV_QUIET		((int)(0x68/(ENV_STEP)))

#define MAX_ATT_INDEX	((ENV_LEN<<ENV_SH)-1) /* 1023.ffff */
#define MIN_ATT_INDEX	(      (1<<ENV_SH)-1) /*    0.ffff */

#define EG_ATT			4
#define EG_DEC			3
#define EG_SUS			2
#define EG_REL			1
#define EG_OFF			0

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define TL_RES_LEN		(256) /* 8 bits addressing (real chip) */

#define LFO_BITS		9
#define LFO_LEN			(1<<LFO_BITS)
#define LFO_MASK		(LFO_LEN-1)


#if (SAMPLE_BITS==16)
	#define FINAL_SH	(0)
	#define MAXOUT		(+32767)
	#define MINOUT		(-32768)
#else
	#define FINAL_SH	(8)
	#define MAXOUT		(+127)
	#define MINOUT		(-128)
#endif


/*	TL_TAB_LEN is calculated as:
*	13 - sinus amplitude bits     (Y axis)
*	2  - sinus sign bit           (Y axis)
*	TL_RES_LEN - sinus resolution (X axis)
*/
#define TL_TAB_LEN (13*2*TL_RES_LEN)
static signed int tl_tab[TL_TAB_LEN];

/* sin waveform table in 'decibel' scale */
static unsigned int sin_tab[SIN_LEN];

/* four AM/PM LFO waveforms (8 in total) */
static unsigned int lfo_tab[LFO_LEN*4*2];

/* LFO amplitude modulation depth table (128 levels) */
static unsigned int lfo_md_tab[128];

/* translate from D1L to volume index (16 D1L levels) */
static unsigned int d1l_tab[16];

/*	DT2 defines offset in cents from base note
*
*	This table defines offset in frequency-deltas table.
*	User's Manual page 22
*
*	Values below were calculated using formula: value =  orig.val / 1.5625
*
*	DT2=0 DT2=1 DT2=2 DT2=3
*	0     600   781   950
*/
static unsigned int dt2_tab[4] = { 0, 384, 500, 608 };

/*	DT1 defines offset in Hertz from base note
*	This table is converted while initialization...
*	Detune table in YM2151 User's Manual is wrong (checked against the real chip)
*/

static unsigned char dt1_tab[4*32] = { /* 4*32 DT1 values */
/* DT1=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

/* DT1=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,

/* DT1=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,

/* DT1=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

static UINT16 phaseinc_rom[768]={
1299,1300,1301,1302,1303,1304,1305,1306,1308,1309,1310,1311,1313,1314,1315,1316,
1318,1319,1320,1321,1322,1323,1324,1325,1327,1328,1329,1330,1332,1333,1334,1335,
1337,1338,1339,1340,1341,1342,1343,1344,1346,1347,1348,1349,1351,1352,1353,1354,
1356,1357,1358,1359,1361,1362,1363,1364,1366,1367,1368,1369,1371,1372,1373,1374,
1376,1377,1378,1379,1381,1382,1383,1384,1386,1387,1388,1389,1391,1392,1393,1394,
1396,1397,1398,1399,1401,1402,1403,1404,1406,1407,1408,1409,1411,1412,1413,1414,
1416,1417,1418,1419,1421,1422,1423,1424,1426,1427,1429,1430,1431,1432,1434,1435,
1437,1438,1439,1440,1442,1443,1444,1445,1447,1448,1449,1450,1452,1453,1454,1455,
1458,1459,1460,1461,1463,1464,1465,1466,1468,1469,1471,1472,1473,1474,1476,1477,
1479,1480,1481,1482,1484,1485,1486,1487,1489,1490,1492,1493,1494,1495,1497,1498,
1501,1502,1503,1504,1506,1507,1509,1510,1512,1513,1514,1515,1517,1518,1520,1521,
1523,1524,1525,1526,1528,1529,1531,1532,1534,1535,1536,1537,1539,1540,1542,1543,
1545,1546,1547,1548,1550,1551,1553,1554,1556,1557,1558,1559,1561,1562,1564,1565,
1567,1568,1569,1570,1572,1573,1575,1576,1578,1579,1580,1581,1583,1584,1586,1587,
1590,1591,1592,1593,1595,1596,1598,1599,1601,1602,1604,1605,1607,1608,1609,1610,
1613,1614,1615,1616,1618,1619,1621,1622,1624,1625,1627,1628,1630,1631,1632,1633,
1637,1638,1639,1640,1642,1643,1645,1646,1648,1649,1651,1652,1654,1655,1656,1657,
1660,1661,1663,1664,1666,1667,1669,1670,1672,1673,1675,1676,1678,1679,1681,1682,
1685,1686,1688,1689,1691,1692,1694,1695,1697,1698,1700,1701,1703,1704,1706,1707,
1709,1710,1712,1713,1715,1716,1718,1719,1721,1722,1724,1725,1727,1728,1730,1731,
1734,1735,1737,1738,1740,1741,1743,1744,1746,1748,1749,1751,1752,1754,1755,1757,
1759,1760,1762,1763,1765,1766,1768,1769,1771,1773,1774,1776,1777,1779,1780,1782,
1785,1786,1788,1789,1791,1793,1794,1796,1798,1799,1801,1802,1804,1806,1807,1809,
1811,1812,1814,1815,1817,1819,1820,1822,1824,1825,1827,1828,1830,1832,1833,1835,
1837,1838,1840,1841,1843,1845,1846,1848,1850,1851,1853,1854,1856,1858,1859,1861,
1864,1865,1867,1868,1870,1872,1873,1875,1877,1879,1880,1882,1884,1885,1887,1888,
1891,1892,1894,1895,1897,1899,1900,1902,1904,1906,1907,1909,1911,1912,1914,1915,
1918,1919,1921,1923,1925,1926,1928,1930,1932,1933,1935,1937,1939,1940,1942,1944,
1946,1947,1949,1951,1953,1954,1956,1958,1960,1961,1963,1965,1967,1968,1970,1972,
1975,1976,1978,1980,1982,1983,1985,1987,1989,1990,1992,1994,1996,1997,1999,2001,
2003,2004,2006,2008,2010,2011,2013,2015,2017,2019,2021,2022,2024,2026,2028,2029,
2032,2033,2035,2037,2039,2041,2043,2044,2047,2048,2050,2052,2054,2056,2058,2059,
2062,2063,2065,2067,2069,2071,2073,2074,2077,2078,2080,2082,2084,2086,2088,2089,
2092,2093,2095,2097,2099,2101,2103,2104,2107,2108,2110,2112,2114,2116,2118,2119,
2122,2123,2125,2127,2129,2131,2133,2134,2137,2139,2141,2142,2145,2146,2148,2150,
2153,2154,2156,2158,2160,2162,2164,2165,2168,2170,2172,2173,2176,2177,2179,2181,
2185,2186,2188,2190,2192,2194,2196,2197,2200,2202,2204,2205,2208,2209,2211,2213,
2216,2218,2220,2222,2223,2226,2227,2230,2232,2234,2236,2238,2239,2242,2243,2246,
2249,2251,2253,2255,2256,2259,2260,2263,2265,2267,2269,2271,2272,2275,2276,2279,
2281,2283,2285,2287,2288,2291,2292,2295,2297,2299,2301,2303,2304,2307,2308,2311,
2315,2317,2319,2321,2322,2325,2326,2329,2331,2333,2335,2337,2338,2341,2342,2345,
2348,2350,2352,2354,2355,2358,2359,2362,2364,2366,2368,2370,2371,2374,2375,2378,
2382,2384,2386,2388,2389,2392,2393,2396,2398,2400,2402,2404,2407,2410,2411,2414,
2417,2419,2421,2423,2424,2427,2428,2431,2433,2435,2437,2439,2442,2445,2446,2449,
2452,2454,2456,2458,2459,2462,2463,2466,2468,2470,2472,2474,2477,2480,2481,2484,
2488,2490,2492,2494,2495,2498,2499,2502,2504,2506,2508,2510,2513,2516,2517,2520,
2524,2526,2528,2530,2531,2534,2535,2538,2540,2542,2544,2546,2549,2552,2553,2556,
2561,2563,2565,2567,2568,2571,2572,2575,2577,2579,2581,2583,2586,2589,2590,2593
};




static YM2151 * YMPSG = NULL;	/* array of YM2151's */
static unsigned int YMNumChips;	/* total # of YM2151's emulated */


/* these variables stay here because of speedup purposes only */
static YM2151 * PSG;
static signed int chanout[8];
static signed int c1,m2,c2; /* Phase Modulation input for operators 2,3,4 */



/* save output as raw 16-bit sample */
/* #define SAVE_SAMPLE */
/* #define SAVE_SEPARATE_CHANNELS */

#if defined SAVE_SAMPLE || defined SAVE_SEPARATE_CHANNELS
static FILE *sample[9];
#endif



/* own PI definition */
#ifdef PI
	#undef PI
#endif
#define PI 3.14159265358979323846




static void init_tables(void)
{
	signed int i,x;
	signed int n;
	double o,m;

	for (x=0; x<TL_RES_LEN; x++)
	{
		m = (1<<16) / pow(2, (x+1) * (ENV_STEP/4.0) / 8.0);
		m = floor(m);

		/* we never reach (1<<16) here due to the (x+1) */
		/* result fits within 16 bits at maximum */

		n = (int)m;		/* 16 bits here */
		n >>= 4;		/* 12 bits here */
		if (n&1)		/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;
						/* 11 bits here (rounded) */
		n <<= 2;		/* 13 bits here (as in real chip) */
		tl_tab[ x*2 + 0 ] = n;
		tl_tab[ x*2 + 1 ] = -tl_tab[ x*2 + 0 ];

		for (i=1; i<13; i++)
		{
			tl_tab[ x*2+0 + i*2*TL_RES_LEN ] =  tl_tab[ x*2+0 ]>>i;
			tl_tab[ x*2+1 + i*2*TL_RES_LEN ] = -tl_tab[ x*2+0 + i*2*TL_RES_LEN ];
		}
	#if 0
			logerror("tl %04i", x);
			for (i=0; i<13; i++)
				logerror(", [%02i] %4x", i*2, tl_tab[ x*2 /*+1*/ + i*2*TL_RES_LEN ]);
			logerror("\n");
		}
	#endif
	}
	/*logerror("TL_TAB_LEN = %i (%i bytes)\n",TL_TAB_LEN, (int)sizeof(tl_tab));*/


	for (i=0; i<SIN_LEN; i++)
	{
		/* non-standard sinus */
		m = sin( ((i*2)+1) * PI / SIN_LEN ); /* checked against the real chip */

		/* we never reach zero here due to ((i*2)+1) */

		if (m>0.0)
			o = 8*log(1.0/m)/log(2);	/* convert to 'decibels' */
		else
			o = 8*log(-1.0/m)/log(2);	/* convert to 'decibels' */

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)						/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;

		sin_tab[ i ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("sin [%4i]= %4i (tl_tab value=%5i)\n", i, sin_tab[i],tl_tab[sin_tab[i]]);*/
	}

	/*logerror("ENV_QUIET= %08x\n",ENV_QUIET );*/


	/* calculate LFO AM waveforms */
	for (x=0; x<4; x++)
	{
	    for (i=0; i<LFO_LEN; i++)
	    {
		switch (x)
		{
		case 0:	/* saw (255 down to 0) */
			m = 255 - (i/2);
			break;
		case 1: /* square (255,0) */
			if (i<256)
				m = 255;
			else
				m = 0;
			break;
		case 2: /* triangle (255 down to 0, up to 255) */
			if (i<256)
				m = 255 - i;
			else
				m = i - 256;
			break;
		case 3: /* random (range 0 to 255) */
			m = ((int)rand()) & 255;
			break;
		}
		/* we reach m = zero here !!!*/

		if (m>0.0)
			o = 8*log(255.0/m)/log(2);			/* convert to 'decibels' */
		else
		{
			if (m<0.0)
				o = 8*log(-255.0/m)/log(2);		/* convert to 'decibels' */
			else
				o = 8*log(255.0/0.01)/log(2);	/* small number */
		}

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)		/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;

		lfo_tab[ x*LFO_LEN*2 + i*2 ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("lfo am waveofs[%i] %04i = %i\n", x, i*2, lfo_tab[ x*LFO_LEN*2 + i*2 ] );*/
	    }
	}
	for (i=0; i<128; i++)
	{
		m = i*2;	/* m=0,2,4,6,8,10,..,252,254 */

		/* we reach m = zero here !!!*/

		if (m>0.0)
			o = 8*log(8192.0/m)/log(2);		/* convert to 'decibels' */
		else
			o = 8*log(8192.0/0.01)/log(2);	/* small number (m=0)*/

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)							/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;

		lfo_md_tab[ i ] = n*2;
		/*logerror("lfo_md_tab[%i](%i) = ofs %i shr by %i\n", i, i*2, (lfo_md_tab[i]>>1)&255, lfo_md_tab[i]>>9 );*/
	}

	/* calculate LFO PM waveforms*/
	for (x=0; x<4; x++)
	{
	    for (i=0; i<LFO_LEN; i++)
	    {
		switch (x)
		{
		case 0:	/* saw (0 to 127, -128 to -1) */
			if (i<256)
				m = (i/2);
			else
				m = (i/2)-256;
			break;
		case 1: /* square (127,-128) */
			if (i<256)
				m = 127;
			else
				m = -128;
			break;
		case 2: /* triangle (0 to 127,127 to -128,-127 to 0) */
			if (i<128)
				m = i; /*0 to 127*/
			else
			{
				if (i<384)
					m = 255 - i; /* 127 down to -128 */
				else
					m = i - 511; /* -127 to 0 */
			}
			break;
		case 3: /* random (range -128 to 127) */
			m = ((int)rand()) & 255;
			m -=128;
			break;
		}
		/* we reach m = zero here !!!*/

		if (m>0.0)
			o = 8*log(127.0/m)/log(2);			/* convert to 'decibels' */
		else
		{
			if (m<0.0)
				o = 8*log(-128.0/m)/log(2);		/* convert to 'decibels' */
			else
				o = 8*log(127.0/0.01)/log(2);	/* small number */
		}

		o = o / (ENV_STEP/4);

		n = (int)(2.0*o);
		if (n&1)								/* round to closest */
			n = (n>>1)+1;
		else
			n = n>>1;

		lfo_tab[ x*LFO_LEN*2 + i*2 + 1 ] = n*2 + (m>=0.0? 0: 1 );
		/*logerror("lfo pm waveofs[%i] %04i = %i\n", x, i*2+1, lfo_tab[ x*LFO_LEN*2 + i*2 + 1 ] );*/
	    }
	}

	/* calculate d1l_tab table */
	for (i=0; i<16; i++)
	{
		m = (i!=15 ? i : i+16) * (4.0/ENV_STEP);   /* every 3 'dB' except for all bits = 1 = 45dB+48dB */
		d1l_tab[i] = m * (1<<ENV_SH);
		/*logerror("d1l_tab[%02x]=%08x\n",i,d1l_tab[i] );*/
	}

#ifdef SAVE_SAMPLE
	sample[8]=fopen("sampsum.raw","ab");
#endif
#ifdef SAVE_SEPARATE_CHANNELS
	sample[0]=fopen("samp0.raw","wb");
	sample[1]=fopen("samp1.raw","wb");
	sample[2]=fopen("samp2.raw","wb");
	sample[3]=fopen("samp3.raw","wb");
	sample[4]=fopen("samp4.raw","wb");
	sample[5]=fopen("samp5.raw","wb");
	sample[6]=fopen("samp6.raw","wb");
	sample[7]=fopen("samp7.raw","wb");
#endif
}


static void init_chip_tables(YM2151 *chip)
{
	int i,j;
	double mult,pom,pom2,clk,phaseinc,Hz;

	double scaler;	/* formula below is true for chip clock=3579545 */
	/* so we need to scale its output accordingly to the chip clock */

	/*scaler = (double)chip->clock / 3579545.0;*/
	scaler = ( (double)chip->clock / 64.0 ) / ( (double)chip->sampfreq );
#if 0
	logerror("scaler    = %20.15f\n", scaler);
	logerror("scalerold = %20.15f\n", (double)chip->clock / 3579545.0 );
#endif

	/* this loop calculates Hertz values for notes from c-0 to b-7 */
	/* including 64 'cents' (100/64 that is 1.5625 of real cent) per note */
	/* i*100/64/1200 is equal to i/768 */

	/* real chip works with 10 bits fixed point values (10.10) */
	mult = (1<<(FREQ_SH-10)); /* -10 because phaseinc_rom table values are already in 10.10 format */

	for (i=0; i<768; i++)
	{
		/* 3.4375 Hz is note A; C# is 4 semitones higher */
		Hz = 1000;
#if 0
/* Hz is close, but not perfect */
		//Hz = scaler * 3.4375 * pow (2, (i + 4 * 64 ) / 768.0 );
		/* calculate phase increment */
		phaseinc = (Hz*SIN_LEN) / (double)chip->sampfreq;
#endif

		phaseinc = phaseinc_rom[i];	/* real chip phase increment */
		phaseinc *= scaler;			/* adjust */


		/* octave 2 - reference octave */
		chip->freq[ 768+2*768+i ] = ((int)(phaseinc*mult)) & 0xffffffc0; /* adjust to X.10 fixed point */
		/* octave 0 and octave 1 */
		for (j=0; j<2; j++)
		{
			chip->freq[768 + j*768 + i] = (chip->freq[ 768+2*768+i ] >> (2-j) ) & 0xffffffc0; /* adjust to X.10 fixed point */
		}
		/* octave 3 to 7 */
		for (j=3; j<8; j++)
		{
			chip->freq[768 + j*768 + i] = chip->freq[ 768+2*768+i ] << (j-2);
		}

	#if 0
			pom = (double)chip->freq[ 768+2*768+i ] / ((double)(1<<FREQ_SH));
			pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
			logerror("1freq[%4i][%08x]= real %20.15f Hz  emul %20.15f Hz\n", i, chip->freq[ 768+2*768+i ], Hz, pom);
	#endif
	}

	/* octave -1 (all equal to: oct 0, _KC_00_, _KF_00_) */
	for (i=0; i<768; i++)
	{
		chip->freq[ 0*768 + i ] = chip->freq[1*768+0];
	}

	/* octave 8 and 9 (all equal to: oct 7, _KC_14_, _KF_63_) */
	for (j=8; j<10; j++)
	{
		for (i=0; i<768; i++)
		{
			chip->freq[768+ j*768 + i ] = chip->freq[768 + 8*768 -1];
		}
	}

#if 0
		for (i=0; i<11*768; i++)
		{
			pom = (double)chip->freq[i] / ((double)(1<<FREQ_SH));
			pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
			logerror("freq[%4i][%08x]= emul %20.15f Hz\n", i, chip->freq[i], pom);
		}
#endif

	mult = (1<<FREQ_SH);
	for (j=0; j<4; j++)
	{
		for (i=0; i<32; i++)
		{
			Hz = ( (double)dt1_tab[j*32+i] * ((double)chip->clock/64.0) ) / (double)(1<<20);

			/*calculate phase increment*/
			phaseinc = (Hz*SIN_LEN) / (double)chip->sampfreq;

			/*positive and negative values*/
			chip->dt1_freq[ (j+0)*32 + i ] = phaseinc * mult;
			chip->dt1_freq[ (j+4)*32 + i ] = -chip->dt1_freq[ (j+0)*32 + i ];

#if 0
			{
				int x = j*32 + i;
				pom = (double)chip->dt1_freq[x] / mult;
				pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
				logerror("DT1(%03i)[%02i %02i][%08x]= real %19.15f Hz  emul %19.15f Hz\n",
						 x, j, i, chip->dt1_freq[x], Hz, pom);
			}
#endif
		}
	}

	mult = (1<<LFO_SH);
	clk  = (double)chip->clock;
	for (i=0; i<256; i++)
	{
		j = i & 0x0f;
		pom = fabs(  (clk/65536/(1<<(i/16)) ) - (clk/65536/32/(1<<(i/16)) * (j+1)) );

		/* calculate phase increment */
		chip->lfo_tab[0xff-i] = ( (pom*LFO_LEN) / (double)chip->sampfreq ) * mult; /* fixed point */
		/*logerror("LFO[%02x] (%08x)= real %20.15f Hz  emul %20.15f Hz\n",0xff-i, chip->lfo_tab[0xff-i], pom,
			(((double)chip->lfo_tab[0xff-i] / mult) * (double)chip->sampfreq ) / (double)LFO_LEN );*/
	}

	for (i=0; i<34; i++)
		chip->eg_tab[i] = 0;		/* infinity */

	for (i=2; i<64; i++)
	{
		pom2 = (double)chip->clock / (double)chip->sampfreq;
		if (i<60) pom2 *= ( 1 + (i&3)*0.25 );
		pom2 *= 1<<((i>>2));
		pom2 /= 768.0 * 1024.0;
		pom2 *= (double)(1<<ENV_SH);
		chip->eg_tab[32+i] = pom2;
#if 0
		logerror("Rate %2i %1i  Decay [real %11.4f ms][emul %11.4f ms][d=%08x]\n",i>>2, i&3,
			( ((double)(ENV_LEN<<ENV_SH)) / pom2 )                       * (1000.0 / (double)chip->sampfreq),
			( ((double)(ENV_LEN<<ENV_SH)) / (double)chip->eg_tab[32+i] ) * (1000.0 / (double)chip->sampfreq), chip->eg_tab[32+i] );
#endif
	}

	for (i=0; i<32; i++)
	{
		chip->eg_tab[ 32+64+i ] = chip->eg_tab[32+63];
	}

	/* calculate timers' deltas */
	/* User's Manual pages 15,16  */
	mult = (1<<TIMER_SH);
	for (i=0; i<1024; i++)
	{
		/* ASG 980324: changed to compute both tim_A_tab and timer_A_time */
		pom= ( 64.0  *  (1024.0-i) / (double)chip->clock );
		#ifdef USE_MAME_TIMERS
			chip->timer_A_time[i] = pom;
		#else
			chip->tim_A_tab[i] = pom * (double)chip->sampfreq * mult;  /* number of samples that timer period takes (fixed point) */
		#endif
	}
	for (i=0; i<256; i++)
	{
		/* ASG 980324: changed to compute both tim_B_tab and timer_B_time */
		pom= ( 1024.0 * (256.0-i)  / (double)chip->clock );
		#ifdef USE_MAME_TIMERS
			chip->timer_B_time[i] = pom;
		#else
			chip->tim_B_tab[i] = pom * (double)chip->sampfreq * mult;  /* number of samples that timer period takes (fixed point) */
		#endif
	}

	/* calculate noise periods table */
	scaler = ( (double)chip->clock / 64.0 ) / ( (double)chip->sampfreq );
	for (i=0; i<32; i++)
	{
		j = (i!=31 ? i : 30);				/* period 30 and 31 are the same */
		j = 32-j;
		j = (65536.0 / (double)(j*32.0));	/* number of samples per one shift of the shift register */
		/*chip->noise_tab[i] = j * 64;*/	/* number of chip clock cycles per one shift */
		chip->noise_tab[i] = j * 64 * scaler;
		/*logerror("noise_tab[%02x]=%08x\n", i, chip->noise_tab[i]);*/
	}

}





/*#define RESET_FEEDBACK_ON_KEYON*/

INLINE void envelope_KONKOFF(YM2151Operator * op, int v)
{
	if (v&0x08)
	{
		if (!op->key)
		{
			op->key   = 1;			/* KEY ON */
			op->phase = 0;			/* clear phase */
			op->state = EG_ATT;		/* KEY ON = attack */
		}
	}
	else
	{
		if (op->key)
		{
			op->key   = 0;			/* KEY OFF */
			if (op->state>EG_REL)
				op->state = EG_REL;	/* KEY OFF = release */
		}
	}

	op+=8;

	if (v&0x20)
	{
		if (!op->key)
		{
			op->key   = 1;
			op->phase = 0;
			op->state = EG_ATT;
		}
	}
	else
	{
		if (op->key)
		{
			op->key   = 0;
			if (op->state>EG_REL)
				op->state = EG_REL;
		}
	}

	op+=8;

	if (v&0x10)
	{
		if (!op->key)
		{
			op->key   = 1;
			op->phase = 0;
			op->state = EG_ATT;
		}
	}
	else
	{
		if (op->key)
		{
			op->key   = 0;
			if (op->state>EG_REL)
				op->state = EG_REL;
		}
	}

	op+=8;

	if (v&0x40)
	{
		if (!op->key)
		{
			op->key   = 1;
			op->phase = 0;
			op->state = EG_ATT;
		}
	}
	else
	{
		if (op->key)
		{
			op->key   = 0;
			if (op->state>EG_REL)
				op->state = EG_REL;
		}
	}
}


#ifdef USE_MAME_TIMERS
static void timer_callback_a (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->timer_A = timer_set (chip->timer_A_time[ chip->timer_A_index ], n, timer_callback_a);
	chip->timer_A_index_old = chip->timer_A_index;
	if (chip->irq_enable & 0x04)
	{
		int oldstate = chip->status & 3;
		chip->status |= 1;
		if ((!oldstate) && (chip->irqhandler)) (*chip->irqhandler)(1);
	}
	if (chip->irq_enable & 0x80)
		chip->csm_req = 2;		/* request KEY ON / KEY OFF sequence */
}
static void timer_callback_b (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->timer_B = timer_set (chip->timer_B_time[ chip->timer_B_index ], n, timer_callback_b);
	chip->timer_B_index_old = chip->timer_B_index;
	if (chip->irq_enable & 0x08)
	{
		int oldstate = chip->status & 3;
		chip->status |= 2;
		if ((!oldstate) && (chip->irqhandler)) (*chip->irqhandler)(1);
	}
}
#if 0
static void timer_callback_chip_busy (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->status &= 0x7f;	/* reset busy flag */
}
#endif
#endif


INLINE void set_connect( YM2151Operator *om1, int v, int cha)
{
	YM2151Operator *om2 = om1+8;
	YM2151Operator *oc1 = om1+16;
	/*YM2151Operator *oc2 = om1+24;*/
	/*oc2->connect = &chanout[cha];*/

	/* set connect algorithm */

	switch( v & 7 )
	{
	case 0:
		/* M1---C1---M2---C2---OUT */
		om1->connect = &c1;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 1:
		/* M1-+-M2---C2---OUT */
		/* C1-+               */
		om1->connect = &m2;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 2:
		/* M1------+-C2---OUT */
		/* C1---M2-+          */
		om1->connect = &c2;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 3:
		/* M1---C1-+-C2---OUT */
		/* M2------+          */
		om1->connect = &c1;
		oc1->connect = &c2;
		om2->connect = &c2;
		break;
	case 4:
		/* M1---C1-+--OUT */
		/* M2---C2-+      */
		om1->connect = &c1;
		oc1->connect = &chanout[cha];
		om2->connect = &c2;
		break;
	case 5:
		/*    +-C1-+     */
		/* M1-+-M2-+-OUT */
		/*    +-C2-+     */
		om1->connect = 0;	/* special mark */
		oc1->connect = &chanout[cha];
		om2->connect = &chanout[cha];
		break;
	case 6:
		/* M1---C1-+     */
		/*      M2-+-OUT */
		/*      C2-+     */
		om1->connect = &c1;
		oc1->connect = &chanout[cha];
		om2->connect = &chanout[cha];
		break;
	case 7:
		/* M1-+     */
		/* C1-+-OUT */
		/* M2-+     */
		/* C2-+     */
		om1->connect = &chanout[cha];
		oc1->connect = &chanout[cha];
		om2->connect = &chanout[cha];
		break;
	}
}


INLINE void refresh_EG( YM2151 *chip, YM2151Operator * op)
{
	UINT32 kc;
	UINT32 v;

	kc = op->kc;

	/* v = 32 + 2*RATE + RKS = max 126 */

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
		op->delta_ar  = chip->eg_tab[ op->ar + v];
	else
		op->delta_ar  = MAX_ATT_INDEX+1;
	op->delta_d1r = chip->eg_tab[op->d1r + v];
	op->delta_d2r = chip->eg_tab[op->d2r + v];
	op->delta_rr  = chip->eg_tab[op->rr  + v];

	op+=8;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
		op->delta_ar  = chip->eg_tab[ op->ar + v];
	else
		op->delta_ar  = MAX_ATT_INDEX+1;
	op->delta_d1r = chip->eg_tab[op->d1r + v];
	op->delta_d2r = chip->eg_tab[op->d2r + v];
	op->delta_rr  = chip->eg_tab[op->rr  + v];

	op+=8;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
		op->delta_ar  = chip->eg_tab[ op->ar + v];
	else
		op->delta_ar  = MAX_ATT_INDEX+1;
	op->delta_d1r = chip->eg_tab[op->d1r + v];
	op->delta_d2r = chip->eg_tab[op->d2r + v];
	op->delta_rr  = chip->eg_tab[op->rr  + v];

	op+=8;

	v = kc >> op->ks;
	if ((op->ar+v) < 32+62)
		op->delta_ar  = chip->eg_tab[ op->ar + v];
	else
		op->delta_ar  = MAX_ATT_INDEX+1;
	op->delta_d1r = chip->eg_tab[op->d1r + v];
	op->delta_d2r = chip->eg_tab[op->d2r + v];
	op->delta_rr  = chip->eg_tab[op->rr  + v];

}


/* write a register on YM2151 chip number 'n' */
void YM2151WriteReg(int n, int r, int v)
{
	YM2151 *chip = &(YMPSG[n]);
	YM2151Operator *op = &chip->oper[ r&0x1f ];

	/* adjust bus to 8 bits */
	r &= 0xff;
	v &= 0xff;

#if 0
	/* There is no info on what YM2151 really does when busy flag is set */
	if ( chip->status & 0x80 ) return;
	timer_set ( 66.0 / (double)chip->clock, n, timer_callback_chip_busy);
	chip->status |= 0x80;	/* set busy flag for 66 chip clock cycles */
#endif

#ifdef LOG_CYM_FILE
	if ((cymfile) && (r!=0) )
	{
		fputc( (unsigned char)r, cymfile );
		fputc( (unsigned char)v, cymfile );
	}
#endif


	switch(r & 0xe0){
	case 0x00:
		switch(r){
		case 0x01:	/* LFO reset(bit 1), Test Register (other bits) */
			chip->test = v;
			if (v&2) chip->lfo_phase = 0;
			break;

		case 0x08:
			envelope_KONKOFF(&chip->oper[ v&7 ], v );
			break;

		case 0x0f:	/* noise mode enable, noise period */
			chip->noise = v;
			chip->noise_f = chip->noise_tab[ v & 0x1f ];
			#if 0
			if (v){
				usrintf_showmessage("YM2151 noise mode = %02x",v);
				logerror("YM2151 noise (%02x)\n",v);
			}
			#endif
			break;

		case 0x10:	/* timer A hi */
			chip->timer_A_index = (chip->timer_A_index & 0x003) | (v<<2);
			break;

		case 0x11:	/* timer A low */
			chip->timer_A_index = (chip->timer_A_index & 0x3fc) | (v & 3);
			break;

		case 0x12:	/* timer B */
			chip->timer_B_index = v;
			break;

		case 0x14:	/* CSM, irq flag reset, irq enable, timer start/stop */

			chip->irq_enable = v;	/* bit 3-timer B, bit 2-timer A, bit 7 - CSM */

			if (v&0x20)	/* reset timer B irq flag */
			{
				UINT32 oldstate = chip->status & 3;
				chip->status &= 0xfd;
				if ((oldstate==2) && (chip->irqhandler)) (*chip->irqhandler)(0);
			}

			if (v&0x10)	/* reset timer A irq flag */
			{
				UINT32 oldstate = chip->status & 3;
				chip->status &= 0xfe;
				if ((oldstate==1) && (chip->irqhandler)) (*chip->irqhandler)(0);

			}

			if (v&0x02){	/* load and start timer B */
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
				/* start timer _only_ if it wasn't already started (it will reload time value next round) */
					if (!chip->timer_B)
					{
						chip->timer_B = timer_set (chip->timer_B_time[ chip->timer_B_index ], n, timer_callback_b);
						chip->timer_B_index_old = chip->timer_B_index;
					}
					#if 0
					else
					{
						/*check if timer value has changed since last start and update if necessary*/
						if (chip->timer_B_index!=chip->timer_B_index_old)
						{
							double timepassed = timer_timeelapsed(chip->timer_B);
							timer_remove (chip->timer_B);
							chip->timer_B = timer_set (chip->timer_B_time[ chip->timer_B_index ] - timepassed, n, timer_callback_b);
							chip->timer_B_index_old = chip->timer_B_index;
						}
					}
					#endif
				#else
					if (!chip->tim_B)
					{
						chip->tim_B = 1;
						chip->tim_B_val = chip->tim_B_tab[ chip->timer_B_index ];
					}
				#endif
			}else{		/* stop timer B */
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->timer_B) timer_remove (chip->timer_B);
					chip->timer_B = 0;
				#else
					chip->tim_B = 0;
				#endif
			}

			if (v&0x01){	/* load and start timer A */
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
				/* start timer _only_ if it wasn't already started (it will reload time value next round) */
					if (!chip->timer_A)
					{
						chip->timer_A = timer_set (chip->timer_A_time[ chip->timer_A_index ], n, timer_callback_a);
						chip->timer_A_index_old = chip->timer_A_index;
					}
					#if 0
					else
					{
						/*check if timer value has changed since last start and update if necessary*/
						if (chip->timer_A_index!=chip->timer_A_index_old)
						{
							double timepassed = timer_timeelapsed(chip->timer_A);
							timer_remove (chip->timer_A);
							chip->timer_A = timer_set (chip->timer_A_time[ chip->timer_A_index ] - timepassed, n, timer_callback_a);
							chip->timer_A_index_old = chip->timer_A_index;
						}
					}
					#endif
				#else
					if (!chip->tim_A)
					{
						chip->tim_A = 1;
						chip->tim_A_val = chip->tim_A_tab[ chip->timer_A_index ];
					}
				#endif
			}else{		/* stop timer A */
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->timer_A) timer_remove (chip->timer_A);
					chip->timer_A = 0;
				#else
					chip->tim_A = 0;
				#endif
			}
			break;

		case 0x18:	/* LFO frequency */
			chip->lfo_freq = chip->lfo_tab[v];
			break;

		case 0x19:	/* PMD (bit 7==1) or AMD (bit 7==0) */
			if (v&0x80)
				chip->pmd = lfo_md_tab[v&0x7f] + 512;
			else
				chip->amd = lfo_md_tab[v&0x7f];
			break;

		case 0x1b:	/* CT2, CT1, LFO waveform */
			chip->ct = v;
			chip->lfo_wave = (v & 3) * LFO_LEN*2;
			if (chip->porthandler) (*chip->porthandler)(0 , (chip->ct) >> 6 );
			break;

		default:
			logerror("YM2151 Write %02x to undocumented register #%02x\n",v,r);
			break;
		}
		break;

	case 0x20:
		op = &chip->oper[r & 7];
		switch(r & 0x18){
		case 0x00:	/* RL enable, Feedback, Connection */
			op->fb_shift = ((v>>3)&7) ? ((v>>3)&7)+6:0;
			chip->pan[ (r&7)*2    ] = (v & 0x40) ? 0xffffffff : 0x0;
			chip->pan[ (r&7)*2 +1 ] = (v & 0x80) ? 0xffffffff : 0x0;
			chip->connect[r&7] = v&7;
			set_connect(op, v&7, r&7);
			break;

		case 0x08:	/* Key Code */
			v &= 0x7f;
			if (v != op->kc)
			{
				UINT32 kc, kc_channel;

				kc_channel = (v - (v>>2))*64;
				kc_channel += 768;
				kc_channel |= (op->kc_i & 63);

				(op+ 0)->kc = v;
				(op+ 0)->kc_i = kc_channel;
				(op+ 8)->kc = v;
				(op+ 8)->kc_i = kc_channel;
				(op+16)->kc = v;
				(op+16)->kc_i = kc_channel;
				(op+24)->kc = v;
				(op+24)->kc_i = kc_channel;

				kc = v>>2;

				(op+ 0)->dt1 = chip->dt1_freq[ (op+ 0)->dt1_i + kc ];
				(op+ 0)->freq = ( (chip->freq[ kc_channel + (op+ 0)->dt2 ] + (op+ 0)->dt1) * (op+ 0)->mul ) >> 1;

				(op+ 8)->dt1 = chip->dt1_freq[ (op+ 8)->dt1_i + kc ];
				(op+ 8)->freq = ( (chip->freq[ kc_channel + (op+ 8)->dt2 ] + (op+ 8)->dt1) * (op+ 8)->mul ) >> 1;

				(op+16)->dt1 = chip->dt1_freq[ (op+16)->dt1_i + kc ];
				(op+16)->freq = ( (chip->freq[ kc_channel + (op+16)->dt2 ] + (op+16)->dt1) * (op+16)->mul ) >> 1;

				(op+24)->dt1 = chip->dt1_freq[ (op+24)->dt1_i + kc ];
				(op+24)->freq = ( (chip->freq[ kc_channel + (op+24)->dt2 ] + (op+24)->dt1) * (op+24)->mul ) >> 1;

				refresh_EG( chip, op );
			}
			break;

		case 0x10:	/* Key Fraction */
			v >>= 2;
			if (v !=  (op->kc_i & 63))
			{
				UINT32 kc_channel;

				kc_channel = v;
				kc_channel |= (op->kc_i & ~63);

				(op+ 0)->kc_i = kc_channel;
				(op+ 8)->kc_i = kc_channel;
				(op+16)->kc_i = kc_channel;
				(op+24)->kc_i = kc_channel;

				(op+ 0)->freq = ( (chip->freq[ kc_channel + (op+ 0)->dt2 ] + (op+ 0)->dt1) * (op+ 0)->mul ) >> 1;
				(op+ 8)->freq = ( (chip->freq[ kc_channel + (op+ 8)->dt2 ] + (op+ 8)->dt1) * (op+ 8)->mul ) >> 1;
				(op+16)->freq = ( (chip->freq[ kc_channel + (op+16)->dt2 ] + (op+16)->dt1) * (op+16)->mul ) >> 1;
				(op+24)->freq = ( (chip->freq[ kc_channel + (op+24)->dt2 ] + (op+24)->dt1) * (op+24)->mul ) >> 1;
			}
			break;

		case 0x18:	/* PMS, AMS */
			op->pms = (v>>4) & 7;
			op->ams = v & 3;
			break;
		}
		break;

	case 0x40:		/* DT1, MUL */
		{
			UINT32 olddt1_i = op->dt1_i;
			UINT32 oldmul = op->mul;
			op->dt1_i = (v&0x70)<<1;
			op->mul   = (v&0x0f) ? (v&0x0f)<<1: 1;
			if (olddt1_i != op->dt1_i)
			{
				op->dt1 = chip->dt1_freq[ op->dt1_i + (op->kc>>2) ];
			}
			if ( (olddt1_i != op->dt1_i) || (oldmul != op->mul) )
			{
				op->freq = ( (chip->freq[ op->kc_i + op->dt2 ] + op->dt1) * op->mul ) >> 1;
			}
		}
		break;

	case 0x60:		/* TL */
		op->tl = (v&0x7f)<<(ENV_BITS-7); /* 7bit TL */
		break;

	case 0x80:		/* KS, AR */
		{
			UINT32 oldks = op->ks;
			UINT32 oldar = op->ar;

			op->ks = 5-(v>>6);
			op->ar = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;

			if ( (op->ar != oldar) || (op->ks != oldks) )
			{
				if ((op->ar + (op->kc>>op->ks)) < 32+62)
					op->delta_ar  = chip->eg_tab[op->ar  + (op->kc>>op->ks) ];
				else
					op->delta_ar  = MAX_ATT_INDEX+1;
			}

			if (op->ks != oldks)
			{
				op->delta_d1r = chip->eg_tab[op->d1r + (op->kc>>op->ks) ];
				op->delta_d2r = chip->eg_tab[op->d2r + (op->kc>>op->ks) ];
				op->delta_rr  = chip->eg_tab[op->rr  + (op->kc>>op->ks) ];
			}
		}
		break;

	case 0xa0:		/* LFO AM enable, D1R */
		op->AMmask = (v&0x80) ? 0xffffffff : 0;
		op->d1r    = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
		op->delta_d1r = chip->eg_tab[op->d1r + (op->kc>>op->ks) ];
		break;

	case 0xc0:		/* DT2, D2R */
		{
			UINT32 olddt2 = op->dt2;
			op->dt2 = dt2_tab[ v>>6 ];
			if (op->dt2 != olddt2)
			{
				op->freq = ( (chip->freq[ op->kc_i + op->dt2 ] + op->dt1) * op->mul ) >> 1;
			}
		}
		op->d2r = (v&0x1f) ? 32 + ((v&0x1f)<<1) : 0;
		op->delta_d2r = chip->eg_tab[op->d2r + (op->kc>>op->ks) ];
		break;

	case 0xe0:		/* D1L, RR */
		op->d1l = d1l_tab[ v>>4 ];
		op->rr  = 34 + ((v&0x0f)<<2);
		op->delta_rr  = chip->eg_tab[op->rr  + (op->kc>>op->ks) ];
		break;
	}
}


#ifdef LOG_CYM_FILE
static void cymfile_callback (int n)
{
	if (cymfile)
	{
		fputc( (unsigned char)0, cymfile );
	}
}
#endif


int YM2151ReadStatus( int n )
{
	return YMPSG[n].status;
}



/*
*	state save support for MAME
*/

static void ym2151_postload_refresh(void)
{
	int i,j;

	for (i=0; i<YMNumChips; i++)
	{
		for (j=0; j<8; j++)
		{
			set_connect(&YMPSG[i].oper[j], YMPSG[i].connect[j], j);
		}
	}
}

static void ym2151_state_save_register( int numchips )
{
	int i,j;
	char buf1[20];


	for (i=0; i<numchips; i++)
	{
		/* save all 32 operators of chip #i */
		for (j=0; j<32; j++)
		{
			YM2151Operator *op;

			sprintf(buf1,"YM2151.op%02i",j);
			op = &YMPSG[i].oper[j];

			state_save_register_UINT32 (buf1, i, "phase", &op->phase, 1);
			state_save_register_UINT32 (buf1, i, "freq" , &op->freq,  1);
			state_save_register_INT32  (buf1, i, "dt1"  , &op->dt1,   1);
			state_save_register_UINT32 (buf1, i, "mul"  , &op->mul,   1);
			state_save_register_UINT32 (buf1, i, "dt1_i", &op->dt1_i, 1);
			state_save_register_UINT32 (buf1, i, "dt2"  , &op->dt2,   1);
			/* operators connection is saved in chip data block */
			state_save_register_UINT32 (buf1, i, "fb_sh", &op->fb_shift,   1);
			state_save_register_INT32  (buf1, i, "fb_c" , &op->fb_out_curr,1);
			state_save_register_INT32  (buf1, i, "fb_p" , &op->fb_out_prev,1);
			state_save_register_UINT32 (buf1, i, "kc"   , &op->kc,    1);
			state_save_register_UINT32 (buf1, i, "kc_i" , &op->kc_i,  1);
			state_save_register_UINT32 (buf1, i, "pms"  , &op->pms,   1);
			state_save_register_UINT32 (buf1, i, "ams"  , &op->ams,   1);
			state_save_register_UINT32 (buf1, i, "AMmask",&op->AMmask,1);

			state_save_register_UINT32 (buf1, i, "state" ,&op->state,    1);
			state_save_register_UINT32 (buf1, i, "dltaAR",&op->delta_ar, 1);
			state_save_register_UINT32 (buf1, i, "tl"    ,&op->tl,       1);
			state_save_register_INT32  (buf1, i, "volume",&op->volume,   1);
			state_save_register_UINT32 (buf1, i, "dltaD1",&op->delta_d1r,1);
			state_save_register_UINT32 (buf1, i, "d1l"   ,&op->d1l,      1);
			state_save_register_UINT32 (buf1, i, "dltaD2",&op->delta_d2r,1);
			state_save_register_UINT32 (buf1, i, "dltaRR",&op->delta_rr, 1);

			state_save_register_UINT32 (buf1, i, "key"   ,&op->key, 1);
			state_save_register_UINT32 (buf1, i, "ks"    ,&op->ks,  1);
			state_save_register_UINT32 (buf1, i, "ar"    ,&op->ar,  1);
			state_save_register_UINT32 (buf1, i, "d1r"   ,&op->d1r, 1);
			state_save_register_UINT32 (buf1, i, "d2r"   ,&op->d2r, 1);
			state_save_register_UINT32 (buf1, i, "rr"    ,&op->rr,  1);

			state_save_register_UINT32 (buf1, i, "rsrvd0",&op->reserved0, 1);
			state_save_register_UINT32 (buf1, i, "rsrvd1",&op->reserved1, 1);
			state_save_register_UINT32 (buf1, i, "rsvrd2",&op->reserved2, 1);
		}

		sprintf(buf1,"YM2151.registers");

		state_save_register_UINT32  (buf1, i, "pan"     , &YMPSG[i].pan[0], 16);

		state_save_register_UINT32  (buf1, i, "lfo_phas", &YMPSG[i].lfo_phase,1);
		state_save_register_UINT32  (buf1, i, "lfo_freq", &YMPSG[i].lfo_freq, 1);
		state_save_register_UINT32  (buf1, i, "lfo_wave", &YMPSG[i].lfo_wave, 1);
		state_save_register_UINT32  (buf1, i, "pmd"     , &YMPSG[i].pmd, 1);
		state_save_register_UINT32  (buf1, i, "amd"     , &YMPSG[i].amd, 1);
		state_save_register_UINT32  (buf1, i, "lfa"     , &YMPSG[i].lfa, 1);
		state_save_register_INT32   (buf1, i, "lfp"     , &YMPSG[i].lfp, 1);

		state_save_register_UINT32  (buf1, i, "test"    , &YMPSG[i].test,1);
		state_save_register_UINT32  (buf1, i, "ct"      , &YMPSG[i].ct,  1);

		state_save_register_UINT32  (buf1, i, "noise"   , &YMPSG[i].noise,     1);
		state_save_register_UINT32  (buf1, i, "noiseRNG", &YMPSG[i].noise_rng, 1);
		state_save_register_UINT32  (buf1, i, "noise_p" , &YMPSG[i].noise_p,   1);
		state_save_register_UINT32  (buf1, i, "noise_f" , &YMPSG[i].noise_f,   1);

		state_save_register_UINT32  (buf1, i, "csm_req" , &YMPSG[i].csm_req,   1);
		state_save_register_UINT32  (buf1, i, "irq_ena" , &YMPSG[i].irq_enable,1);
		state_save_register_UINT32  (buf1, i, "status"  , &YMPSG[i].status,    1);

		state_save_register_UINT32  (buf1, i, "TimAind" , &YMPSG[i].timer_A_index, 1);
		state_save_register_UINT32  (buf1, i, "TimBind" , &YMPSG[i].timer_B_index, 1);
		state_save_register_UINT32  (buf1, i, "TimAold" , &YMPSG[i].timer_A_index_old, 1);
		state_save_register_UINT32  (buf1, i, "TimBold" , &YMPSG[i].timer_B_index_old, 1);

		state_save_register_UINT32  (buf1, i, "connect" , &YMPSG[i].connect[0], 8);
	}

	state_save_register_func_postload(ym2151_postload_refresh);
}



/*
*	Initialize YM2151 emulator(s).
*
*	'num' is the number of virtual YM2151's to allocate
*	'clock' is the chip clock in Hz
*	'rate' is sampling rate
*/

int YM2151Init(int num, int clock, int rate)
{
	int i;

	if (YMPSG)
		return -1;	/* duplicate init. */

	YMNumChips = num;

	YMPSG = (YM2151 *)malloc(sizeof(YM2151) * YMNumChips);
	if (YMPSG == NULL)
		return 1;

	memset(YMPSG, 0, sizeof(YM2151) * YMNumChips);

#if 0
	tl_tab = (signed int *)malloc(sizeof(signed int) * TL_TAB_LEN );
	if (tl_tab == NULL)
	{
		free (YMPSG);
		YMPSG = NULL;
		return 1;
	}
#endif

	ym2151_state_save_register( YMNumChips );

	init_tables();
	for (i=0 ; i<YMNumChips; i++)
	{
		YMPSG[i].clock = clock;
		/*rate = clock/64;*/
		YMPSG[i].sampfreq = rate ? rate : 44100;	/* avoid division by 0 in init_chip_tables() */
		YMPSG[i].irqhandler = NULL;					/* interrupt handler  */
		YMPSG[i].porthandler = NULL;				/* port write handler */
		init_chip_tables( &YMPSG[i] );
#ifdef USE_MAME_TIMERS
/* this must be done _before_ a call to YM2151ResetChip() */
		YMPSG[i].timer_A = 0;
		YMPSG[i].timer_B = 0;
#else
		YMPSG[i].tim_A      = 0;
		YMPSG[i].tim_B      = 0;
#endif
		YM2151ResetChip(i);
		/*logerror("YM2151[init] clock=%i sampfreq=%i\n", YMPSG[i].clock, YMPSG[i].sampfreq);*/
	}

#ifdef LOG_CYM_FILE
	cymfile = fopen("2151_.cym","wb");
	if (cymfile)
		cymfiletimer = timer_pulse ( TIME_IN_HZ(110), 0, cymfile_callback); /*110 Hz pulse timer*/
	else
		logerror("Could not create file 2151_.cym\n");
#endif

	return 0;
}



void YM2151Shutdown()
{
	if (!YMPSG)
		return;

	free (YMPSG);
	YMPSG = NULL;
#if 0
	if (tl_tab)
	{
		free (tl_tab);
		tl_tab = NULL;
	}
#endif

#ifdef LOG_CYM_FILE
	fclose (cymfile);
	cymfile = NULL;
	if (cymfiletimer)
		timer_remove (cymfiletimer);
	cymfiletimer = 0;
#endif

#ifdef SAVE_SAMPLE
	fclose(sample[8]);
#endif
#ifdef SAVE_SEPARATE_CHANNELS
	fclose(sample[0]);
	fclose(sample[1]);
	fclose(sample[2]);
	fclose(sample[3]);
	fclose(sample[4]);
	fclose(sample[5]);
	fclose(sample[6]);
	fclose(sample[7]);
#endif
}



/*
*	reset all chip registers.
*/

void YM2151ResetChip(int num)
{
	int i;
	YM2151 *chip = &YMPSG[num];


	/* initialize hardware registers */
	for (i=0; i<32; i++)
	{
		memset(&chip->oper[i],'\0',sizeof(YM2151Operator));
		chip->oper[i].volume = MAX_ATT_INDEX;
	}

	chip->lfo_phase = 0;
	chip->lfo_freq  = 0;
	chip->lfo_wave  = 0;
	chip->pmd = lfo_md_tab[ 0 ] + 512;
	chip->amd = lfo_md_tab[ 0 ];
	chip->lfa = 0;
	chip->lfp = 0;

	chip->test= 0;

	chip->irq_enable = 0;
#ifdef USE_MAME_TIMERS
	/* ASG 980324 -- reset the timers before writing to the registers */
	if (chip->timer_A) timer_remove (chip->timer_A);
	chip->timer_A = 0;
	if (chip->timer_B) timer_remove (chip->timer_B);
	chip->timer_B = 0;
#else
	chip->tim_A      = 0;
	chip->tim_B      = 0;
	chip->tim_A_val  = 0;
	chip->tim_B_val  = 0;
#endif
	chip->timer_A_index = 0;
	chip->timer_B_index = 0;
	chip->timer_A_index_old = 0;
	chip->timer_B_index_old = 0;

	chip->noise     = 0;
	chip->noise_rng = 0;
	chip->noise_p   = 0;
	chip->noise_f   = chip->noise_tab[0];

	chip->csm_req	= 0;
	chip->status    = 0;

	YM2151WriteReg(num, 0x1b, 0);	/* only because of CT1, CT2 output pins */
	for (i=0x20; i<0x100; i++)		/* just to set the operators */
	{
		YM2151WriteReg(num, i, 0);
	}
}



INLINE void lfo_calc(void)
{
	UINT32 phase;
	UINT32 lfx;


	if (PSG->test&2)
	{
		PSG->lfo_phase = 0;
		phase = PSG->lfo_wave;
	}
	else
	{
		phase = (PSG->lfo_phase>>LFO_SH) & LFO_MASK;
		phase = phase*2 + PSG->lfo_wave;
	}

	lfx = lfo_tab[phase] + PSG->amd;
	PSG->lfa = 0;
	if (lfx < TL_TAB_LEN)
		PSG->lfa = tl_tab[ lfx ];

	lfx = lfo_tab[phase+1] + PSG->pmd;
	PSG->lfp = 0;
	if (lfx < TL_TAB_LEN)
		PSG->lfp = tl_tab[ lfx ];
}


INLINE void calc_lfo_pm(YM2151Operator *op)
{
	INT32 mod_ind;
	INT32 pom;


	mod_ind = PSG->lfp;		/* -128..+127 (8bits signed) */
	if (op->pms < 6)
		mod_ind >>= (6 - op->pms);
	else
		mod_ind <<= (op->pms - 5);

	if (mod_ind)
	{
		UINT32 kc_channel;

		kc_channel = op->kc_i + mod_ind;

		pom = ( (PSG->freq[ kc_channel + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		op->phase += (pom - op->freq);

		op+=8;
		pom = ( (PSG->freq[ kc_channel + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		op->phase += (pom - op->freq);

		op+=8;
		pom = ( (PSG->freq[ kc_channel + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		op->phase += (pom - op->freq);

		op+=8;
		pom = ( (PSG->freq[ kc_channel + op->dt2 ] + op->dt1) * op->mul ) >> 1;
		op->phase += (pom - op->freq);
	}
}


INLINE signed int op_calc(YM2151Operator * OP, unsigned int env, signed int pm)
{
	UINT32 p;


	p = (env<<3) + sin_tab[ ( ((signed int)((OP->phase & ~FREQ_MASK) + (pm<<15))) >> FREQ_SH ) & SIN_MASK ];

	if (p >= TL_TAB_LEN)
		return 0;

	return tl_tab[p];
}

INLINE signed int op_calc1(YM2151Operator * OP, unsigned int env, signed int pm)
{
	UINT32 p;
	INT32  i;


	i = (OP->phase & ~FREQ_MASK) + pm;

/*logerror("i=%08x (i>>16)&511=%8i phase=%i [pm=%08x] ",i, (i>>16)&511, OP->phase>>FREQ_SH, pm);*/

	p = (env<<3) + sin_tab[ (i>>FREQ_SH) & SIN_MASK];

/*logerror("(p&255=%i p>>8=%i) out= %i\n", p&255,p>>8, tl_tab[p&255]>>(p>>8) );*/

	if (p >= TL_TAB_LEN)
		return 0;

	return tl_tab[p];
}



#define volume_calc(OP) (OP->tl + (((unsigned int)OP->volume)>>ENV_SH) + (AM & OP->AMmask))

INLINE void chan_calc(unsigned int chan)
{
	YM2151Operator *op;
	unsigned int env;
	UINT32 AM;


	chanout[chan]= c1 = m2 = c2 = 0;
	AM = 0;

	op = &PSG->oper[chan];	/* M1 */

	if (op->ams)
		AM = PSG->lfa << (op->ams-1);

	if (op->pms)
		calc_lfo_pm(op);

	env = volume_calc(op);
	{
		INT32 out;

		out = op->fb_out_prev + op->fb_out_curr;
		op->fb_out_prev = op->fb_out_curr;

		if (!op->connect)
			/* algorithm 5 */
			c1 = m2 = c2 = op->fb_out_prev;
		else
			/* other algorithms */
			*op->connect = op->fb_out_prev;

		op->fb_out_curr = 0;

		if (env < ENV_QUIET)
			op->fb_out_curr = op_calc1(op, env, (out<<op->fb_shift) );
	}

	op += 16;	/* C1 */
	env = volume_calc(op);
	if (env < ENV_QUIET)
		*op->connect += op_calc(op, env, c1);

	op -= 8;	/* M2 */
	env = volume_calc(op);
	if (env < ENV_QUIET)
		*op->connect += op_calc(op, env, m2);

	op += 16;	/* C2 */
	env = volume_calc(op);
	if (env < ENV_QUIET)
		chanout[chan] += op_calc(op, env, c2);

}
INLINE void chan7_calc(void)
{
	YM2151Operator *op;
	unsigned int env;
	UINT32 AM;


	chanout[7]= c1 = m2 = c2 = 0;
	AM = 0;

	op = &PSG->oper[7];	/* M1 */

	if (op->ams)
		AM = PSG->lfa << (op->ams-1);

	if (op->pms)
		calc_lfo_pm(op);

	env = volume_calc(op);
	{
		INT32 out;

		out = op->fb_out_prev + op->fb_out_curr;
		op->fb_out_prev = op->fb_out_curr;

		if (!op->connect)
			/* algorithm 5 */
			c1 = m2 = c2 = op->fb_out_prev;
		else
			/* other algorithms */
			*op->connect = op->fb_out_prev;

		op->fb_out_curr = 0;

		if (env < ENV_QUIET)
			op->fb_out_curr = op_calc1(op, env, (out<<op->fb_shift) );
	}

	op += 16;	/* C1 */
	env = volume_calc(op);
	if (env < ENV_QUIET)
		*op->connect += op_calc(op, env, c1);

	op -= 8;	/* M2 */
	env = volume_calc(op);
	if (env < ENV_QUIET)
		*op->connect += op_calc(op, env, m2);

	op += 16;	/* C2 */
	env = volume_calc(op);

	if (PSG->noise & 0x80)
	{
		UINT32 noiseout;

		noiseout = 0;
		if (env < 0x3ff)
			noiseout = (env ^ 0x3ff) * 2;	/* range of the YM2151 noise output is -2044 to 2040 */
		chanout[7] += ((PSG->noise_rng&0x10000) ? noiseout: -noiseout); /* bit 16 -> output */
	}
	else
	{
		if (env < ENV_QUIET)
			chanout[7] += op_calc(op, env, c2);
	}

}

INLINE void advance(void)
{
	YM2151Operator *op;
	int i;


	if (!(PSG->test&2))
		PSG->lfo_phase += PSG->lfo_freq;

	/*	The Noise Generator of the YM2151 is 17-bit shift register.
	*	Input to the bit16 is negated (bit0 XOR bit3) (EXNOR).
	*	Output of the register is negated (bit0 XOR bit3).
	*	Simply use bit16 as the noise output.
	*/
	PSG->noise_p += PSG->noise_f;
	i = (PSG->noise_p>>16);		/* number of events (shifts of the shift register) */
	PSG->noise_p &= 0xffff;
	while (i)
	{
		UINT32 j;
		j = ( (PSG->noise_rng ^ (PSG->noise_rng>>3) ) & 1) ^ 1;
		PSG->noise_rng = (j<<16) | (PSG->noise_rng>>1);
		i--;
	}


	/*	In real it seems that CSM keyon line is ORed with the KO line inside of the chip.
	*	This causes it to only work when KO is off, ie. 0
	*	Below is my implementation only.
	*/
	if (PSG->csm_req)	/* CSM KEYON/KEYOFF seqeunce request */
	{
		if (PSG->csm_req==2)			/* KEY ON */
		{
			op = &PSG->oper[0];	/* CH 0 M1 */
			i = 32;
			do
			{
				if (op->key==0)		/* _ONLY_ when KEY is OFF (checked) */
				{
					op->phase = 0;
					op->state = EG_ATT;
				}
				op++;
				i--;
			}while (i);
			PSG->csm_req = 1;
		}
		else						/* KEY OFF */
		{
			op = &PSG->oper[0];	/* CH 0 M1 */
			i = 32;
			do
			{
				if (op->key==0)		/* _ONLY_ when KEY is OFF (checked) */
				{
					if (op->state>EG_REL)
						op->state = EG_REL;
				}
				op++;
				i--;
			}while (i);
			PSG->csm_req = 0;
		}
	}

	op = &PSG->oper[0];	/* CH 0 M1 */
	i = 32;
	do
	{
		op->phase += op->freq;

		switch(op->state)
		{
		case EG_ATT:		/* attack phase */
		{
			INT32 step = op->volume;

			op->volume -= op->delta_ar;
			step = (step>>ENV_SH) - (((UINT32)op->volume)>>ENV_SH);	/* number of levels passed since last time */
			if (step > 0)
			{
				INT32 tmp_volume = op->volume + (step<<ENV_SH);	/* adjust by number of levels */
				do
				{
					tmp_volume = tmp_volume - (1<<ENV_SH) - ((tmp_volume>>4) & ~ENV_MASK);
					if (tmp_volume <= MIN_ATT_INDEX)
						break;
					step--;
				}while(step);
				op->volume = tmp_volume;
			}

			if (op->volume <= MIN_ATT_INDEX)
			{
				if (op->volume < 0)
					op->volume = 0;	/* this is not quite correct (checked) */
				op->state = EG_DEC;
			}
		}
		break;

		case EG_DEC:	/* decay phase */
			if ( (op->volume += op->delta_d1r) >= op->d1l )
			{
				op->volume = op->d1l;	/* this is not quite correct (checked) */
				op->state = EG_SUS;
			}
		break;

		case EG_SUS:	/* sustain phase */
			if ( (op->volume += op->delta_d2r) > MAX_ATT_INDEX )
			{
				op->volume = MAX_ATT_INDEX;
				op->state = EG_OFF;
			}
		break;

		case EG_REL:	/* release phase */
			if ( (op->volume += op->delta_rr) > MAX_ATT_INDEX )
			{
				op->volume = MAX_ATT_INDEX;
				op->state = EG_OFF;
			}
		break;
		}
		op++;
		i--;
	}while (i);
}

#if 0
INLINE signed int acc_calc(signed int value)
{
	if (value>=0)
	{
		if (value < 0x0200)
			return (value);
		if (value < 0x0400)
			return (value & 0xfffffffe);
		if (value < 0x0800)
			return (value & 0xfffffffc);
		if (value < 0x1000)
			return (value & 0xfffffff8);
		if (value < 0x2000)
			return (value & 0xfffffff0);
		if (value < 0x4000)
			return (value & 0xffffffe0);
		return (value & 0xffffffc0);
	}
	/*else value < 0*/
	if (value > -0x0200)
		return (value);
	if (value > -0x0400)
		return (value & 0xfffffffe);
	if (value > -0x0800)
		return (value & 0xfffffffc);
	if (value > -0x1000)
		return (value & 0xfffffff8);
	if (value > -0x2000)
		return (value & 0xfffffff0);
	if (value > -0x4000)
		return (value & 0xffffffe0);
	return (value & 0xffffffc0);

}
#endif

/* first macro saves left and right channels to mono file */
/* second macro saves left and right channels to stereo file */
#if 0	/*MONO*/
	#ifdef SAVE_SEPARATE_CHANNELS
	  #define SAVE_SINGLE_CHANNEL(j) \
	  {	signed int pom= -(chanout[j] & PSG->pan[j*2]); \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		fputc((unsigned short)pom&0xff,sample[j]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[j]);  }
	#else
	  #define SAVE_SINGLE_CHANNEL(j)
	#endif
#else	/*STEREO*/
	#ifdef SAVE_SEPARATE_CHANNELS
	  #define SAVE_SINGLE_CHANNEL(j) \
	  {	signed int pom = -(chanout[j] & PSG->pan[j*2]); \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		fputc((unsigned short)pom&0xff,sample[j]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[j]); \
		pom = -(chanout[j] & PSG->pan[j*2+1]); \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		fputc((unsigned short)pom&0xff,sample[j]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[j]); \
	  }
	#else
	  #define SAVE_SINGLE_CHANNEL(j)
	#endif
#endif

/* first macro saves left and right channels to mono file */
/* second macro saves left and right channels to stereo file */
#if 1	/*MONO*/
	#ifdef SAVE_SAMPLE
	  #define SAVE_ALL_CHANNELS \
	  {	signed int pom = outr; \
		pom >>= FINAL_SH; \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		/*pom = acc_calc(pom);*/ \
		/*fprintf(sample[8]," %i\n",pom);*/ \
		fputc((unsigned short)pom&0xff,sample[8]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[8]); \
	  }
	#else
	  #define SAVE_ALL_CHANNELS
	#endif
#else	/*STEREO*/
	#ifdef SAVE_SAMPLE
	  #define SAVE_ALL_CHANNELS \
	  {	signed int pom = outl; \
		pom >>= FINAL_SH; \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		fputc((unsigned short)pom&0xff,sample[8]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[8]); \
		pom = outr; \
		pom >>= FINAL_SH; \
		if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
		fputc((unsigned short)pom&0xff,sample[8]); \
		fputc(((unsigned short)pom>>8)&0xff,sample[8]); \
	  }
	#else
	  #define SAVE_ALL_CHANNELS
	#endif
#endif


/*	Generate samples for one of the YM2151's
*
*	'num' is the number of virtual YM2151
*	'**buffers' is table of pointers to the buffers: left and right
*	'length' is the number of samples that should be generated
*/
void YM2151UpdateOne(int num, INT16 **buffers, int length)
{
	int i;
	signed int outl,outr;
	SAMP *bufL, *bufR;

	bufL = buffers[0];
	bufR = buffers[1];

	PSG = &YMPSG[num];

#ifdef USE_MAME_TIMERS
		/* ASG 980324 - handled by real timers now */
#else
	if (PSG->tim_B)
	{
		PSG->tim_B_val -= ( length << TIMER_SH );
		if (PSG->tim_B_val<=0)
		{
			PSG->tim_B_val += PSG->tim_B_tab[ PSG->timer_B_index ];
			if ( PSG->irq_enable & 0x08 )
			{
				int oldstate = PSG->status & 3;
				PSG->status |= 2;
				if ((!oldstate) && (PSG->irqhandler)) (*PSG->irqhandler)(1);
			}
		}
	}
#endif

	for (i=0; i<length; i++)
	{

		chan_calc(0);
		SAVE_SINGLE_CHANNEL(0)
		chan_calc(1);
		SAVE_SINGLE_CHANNEL(1)
		chan_calc(2);
		SAVE_SINGLE_CHANNEL(2)
		chan_calc(3);
		SAVE_SINGLE_CHANNEL(3)
		chan_calc(4);
		SAVE_SINGLE_CHANNEL(4)
		chan_calc(5);
		SAVE_SINGLE_CHANNEL(5)
		chan_calc(6);
		SAVE_SINGLE_CHANNEL(6)
		chan7_calc();
		SAVE_SINGLE_CHANNEL(7)

		SAVE_ALL_CHANNELS

		outl = chanout[0] & PSG->pan[0];
		outr = chanout[0] & PSG->pan[1];
		outl += (chanout[1] & PSG->pan[2]);
		outr += (chanout[1] & PSG->pan[3]);
		outl += (chanout[2] & PSG->pan[4]);
		outr += (chanout[2] & PSG->pan[5]);
		outl += (chanout[3] & PSG->pan[6]);
		outr += (chanout[3] & PSG->pan[7]);
		outl += (chanout[4] & PSG->pan[8]);
		outr += (chanout[4] & PSG->pan[9]);
		outl += (chanout[5] & PSG->pan[10]);
		outr += (chanout[5] & PSG->pan[11]);
		outl += (chanout[6] & PSG->pan[12]);
		outr += (chanout[6] & PSG->pan[13]);
		outl += (chanout[7] & PSG->pan[14]);
		outr += (chanout[7] & PSG->pan[15]);

		outl >>= FINAL_SH;
		outr >>= FINAL_SH;
		if (outl > MAXOUT) outl = MAXOUT;
			else if (outl < MINOUT) outl = MINOUT;
		if (outr > MAXOUT) outr = MAXOUT;
			else if (outr < MINOUT) outr = MINOUT;
		((SAMP*)bufL)[i] = (SAMP)outl;
		((SAMP*)bufR)[i] = (SAMP)outr;

#ifdef USE_MAME_TIMERS
		/* ASG 980324 - handled by real timers now */
#else
		/* calculate timer A */
		if (PSG->tim_A)
		{
			PSG->tim_A_val -= ( 1 << TIMER_SH );
			if (PSG->tim_A_val <= 0)
			{
				PSG->tim_A_val += PSG->tim_A_tab[ PSG->timer_A_index ];
				if (PSG->irq_enable & 0x04)
				{
					int oldstate = PSG->status & 3;
					PSG->status |= 1;
					if ((!oldstate) && (PSG->irqhandler)) (*PSG->irqhandler)(1);
				}
				if (PSG->irq_enable & 0x80)
					PSG->csm_req = 2;	/* request KEY ON / KEY OFF sequence */
			}
		}
#endif

		lfo_calc();
		advance();
	}
}

void YM2151SetIrqHandler(int n, void(*handler)(int irq))
{
	YMPSG[n].irqhandler = handler;
}

void YM2151SetPortWriteHandler(int n, mem_write_handler handler)
{
	YMPSG[n].porthandler = handler;
}

