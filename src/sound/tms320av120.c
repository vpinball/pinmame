/**********************************************************************************************
 *
 *   Texas Instruments TMS320AV120 MPG Decoder
 *   by Steve Ellenoff
 *   10/14/2003
 *
 *   MPEG Decoding based on the book:
*    "The Programmer's Guide to Sound" by Tim Kientzle (see copyright below)
 **********************************************************************************************/ 
/**********************************************************************************************
 *   NOTE: Supports ONLY MPEG1 Layer 2 Mono data @ 32KHz/32kbps at this time!
 *         Not configurable to anything else, for speed purposes and coding simplicity at this time!
 *
 *   TODO:
 *		   1) Not sure where to set BOF line properly
 *		   2) Not sure of best pcm buffer size and prebuffer size values (nor when best to put SREQ low again)
 **********************************************************************************************/


//MPEG DECODING:    "The Programmer's Guide to Sound" by Tim Kientzle
/***********************************************************************************************
   Copyright 1997 Tim Kientzle.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:
      This product includes software developed by Tim Kientzle
      and published in ``The Programmer's Guide to Sound.''
4. Neither the names of Tim Kientzle nor Addison-Wesley
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL TIM KIENTZLE OR ADDISON-WESLEY BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "driver.h"
#include "tms320av120.h"


#define VERBOSE

#ifdef VERBOSE
#define LOG(x)	logerror x
//define LOG(x)	printf x
#else
#define LOG(x)
#endif

/**********************************************************************************************
     DECLARATIONS
***********************************************************************************************/
//MPG Stuff
INLINE long Layer2Requant(long sample, long levels, int scaleIndex);
static void Matrix(long *V, long *subbandSamples, int numSamples);
static void Layer12Synthesis(  int num, long *V[16], long *subbandSamples,	int numSubbandSamples);
static long GetBits(int num, int numBits);
static void DecodeLayer2(int num);

//TMS320AV120 Stuff
static void tms320av120_update(int num, INT16 *buffer, int length);
static void set_bof_line(int chipnum, int state);
static void set_sreq_line(int chipnum, int state);
static int valid_header(int chipnum);

/**********************************************************************************************
     CONSTANTS
***********************************************************************************************/
#define MPG_HEADERSIZE				4			//Mpeg1 - Layer 2 Header Size
#define MPG_FRAMESIZE				140			//Mpeg1 - Layer 2 Framesize @ 32KHz/32kbps (excluding header)
#define CAP_PCMBUFFER_SIZE			1152*100	//# of Samples to hold (1 Frame = 1152 Samples)
#define CAP_PREBUFFER_SIZE			1152*3		//# of decoded samples needed before we begin to output pcm

#define	LOG_DATA_IN					0			//Set to 1 to log data input to an mp3 file

/**********************************************************************************************
     INTERNAL DATA STRUCTURES
***********************************************************************************************/
struct TMS320AV120Chip
{
 UINT8  framebuff[MPG_FRAMESIZE];		//Holds raw mpg data for 1 frame (also used as header input buffer)
 UINT8	fb_pos;							//Current frame buffer position
 INT16  pcmbuffer[CAP_PCMBUFFER_SIZE];	//Decoded PCM data buffer
 UINT16 pcm_pos;						//Position of next data to be input into the pcm buffer
 UINT16 sOut;							//Position of next sample to read out of the pcm buffer
 int    stream;							//Holds stream channel assignment
 int    bitsRemaining;					//Keep track of # of bits we've read from frame buffer
 long *_V[16];							//Synthesis window for single channel
 int    mute;							//Mute status ( 0 = off, 1 = Mute )
 int	reset;							//Reset status( 0 = off, 1 = Reset)
 int	bof_line;						//BOF Line status
 int	sreq_line;						//SREQ Line status
 int	found_header;					//Did we find a valid header yet?
 int	prebuff_full;					//If 1, prebuffer full so ok to output pcm
};

//Layer 2 Quantization
typedef struct {
   long _levels;  // Number of levels
   char _bits;    // bits to read
   int _grouping;// Yes->decompose into three samples
} Layer2QuantClass;

//Different Quantization types
static Layer2QuantClass l2qc3 = {3,5,1};
static Layer2QuantClass l2qc5 = {5,7,1};
//static Layer2QuantClass l2qc7 = {7,3,0};		Not used for our specific sampling frequencies
static Layer2QuantClass l2qc9 = {9,10,1};
static Layer2QuantClass l2qc15 = {15,4,0};
static Layer2QuantClass l2qc31 = {31,5,0};
static Layer2QuantClass l2qc63 = {63,6,0};
static Layer2QuantClass l2qc127 = {127,7,0};
static Layer2QuantClass l2qc255 = {255,8,0};
static Layer2QuantClass l2qc511 = {511,9,0};
static Layer2QuantClass l2qc1023 = {1023,10,0};
static Layer2QuantClass l2qc2047 = {2047,11,0};
static Layer2QuantClass l2qc4095 = {4095,12,0};
static Layer2QuantClass l2qc8191 = {8191,13,0};
static Layer2QuantClass l2qc16383 = {16383,14,0};
static Layer2QuantClass l2qc32767 = {32767,15,0};
//static Layer2QuantClass l2qc65535 = {65535,16,0};		Not used for our specific sampling frequencies

//Allocation Table
Layer2QuantClass *l2allocationE[] = {
   0,&l2qc3,&l2qc5,&l2qc9,
   &l2qc15,&l2qc31,&l2qc63,&l2qc127,&l2qc255,&l2qc511,&l2qc1023,&l2qc2047,
   &l2qc4095,&l2qc8191,&l2qc16383,&l2qc32767
};

//Allocation Entry
typedef struct{
   char _numberBits;
   Layer2QuantClass **_quantClasses;
}Layer2BitAllocationTableEntry ;

// 11 active subbands
// Mono requires 38 bits for allocation table
// Used at lowest bit rates 
Layer2BitAllocationTableEntry Layer2AllocationB2d[32] = {
   { 4, l2allocationE },   { 4, l2allocationE },   { 3, l2allocationE },
   { 3, l2allocationE },   { 3, l2allocationE },   { 3, l2allocationE },
   { 3, l2allocationE },   { 3, l2allocationE },   { 3, l2allocationE },
   { 3, l2allocationE },   { 3, l2allocationE },   { 3, l2allocationE },
   {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
   {0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},
};

static long SynthesisWindowCoefficients[] = // 2.16 fixed-point values
{0, -1, -1, -1, -1, -1, -1, -2, -2, -2, -2, -3, -3, -4, -4, -5, -5, -6,
-7, -7, -8, -9, -10, -11, -13, -14, -16, -17, -19, -21, -24, -26,

-29, -31, -35, -38, -41, -45, -49, -53, -58, -63, -68, -73, -79,
-85, -91, -97, -104, -111, -117, -125, -132, -139, -147, -154, -161,
-169, -176, -183, -190, -196, -202, -208,

213, 218, 222, 225, 227, 228, 228, 227, 224, 221, 215, 208, 200, 189,
177, 163, 146, 127, 106, 83, 57, 29, -2, -36, -72, -111, -153, -197,
-244, -294, -347, -401,

-459, -519, -581, -645, -711, -779, -848, -919, -991, -1064, -1137,
-1210, -1283, -1356, -1428, -1498, -1567, -1634, -1698, -1759, -1817,
-1870, -1919, -1962, -2001, -2032, -2057, -2075, -2085, -2087, -2080,
-2063,

2037, 2000, 1952, 1893, 1822, 1739, 1644, 1535, 1414, 1280, 1131, 970,
794, 605, 402, 185, -45, -288, -545, -814, -1095, -1388, -1692, -2006,
-2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,

-5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597, -7910, -8209,
-8491, -8755, -8998, -9219, -9416, -9585, -9727, -9838, -9916, -9959,
-9966, -9935, -9863, -9750, -9592, -9389, -9139, -8840, -8492, -8092,
-7640, -7134,

6574, 5959, 5288, 4561, 3776, 2935, 2037, 1082, 70, -998, -2122,
-3300, -4533, -5818, -7154, -8540, -9975, -11455, -12980, -14548,
-16155, -17799, -19478, -21189, -22929, -24694, -26482, -28289,
-30112, -31947, -33791, -35640,

-37489, -39336, -41176, -43006, -44821, -46617, -48390, -50137,
-51853, -53534, -55178, -56778, -58333, -59838, -61289, -62684,
-64019, -65290, -66494, -67629, -68692, -69679, -70590, -71420,
-72169, -72835, -73415, -73908, -74313, -74630, -74856, -74992,

75038, 74992, 74856, 74630, 74313, 73908, 73415, 72835, 72169, 71420,
70590, 69679, 68692, 67629, 66494, 65290, 64019, 62684, 61289, 59838,
58333, 56778, 55178, 53534, 51853, 50137, 48390, 46617, 44821, 43006,
41176, 39336,

37489, 35640, 33791, 31947, 30112, 28289, 26482, 24694, 22929, 21189,
19478, 17799, 16155, 14548, 12980, 11455, 9975, 8540, 7154, 5818,
4533, 3300, 2122, 998, -70, -1082, -2037, -2935, -3776, -4561, -5288,
-5959,

6574, 7134, 7640, 8092, 8492, 8840, 9139, 9389, 9592, 9750, 9863, 9935,
9966, 9959, 9916, 9838, 9727, 9585, 9416, 9219, 8998, 8755, 8491,
8209, 7910, 7597, 7271, 6935, 6589, 6237, 5879, 5517,

5153, 4788, 4425, 4063, 3705, 3351, 3004, 2663, 2330, 2006, 1692,
1388, 1095, 814, 545, 288, 45, -185, -402, -605, -794, -970, -1131,
-1280, -1414, -1535, -1644, -1739, -1822, -1893, -1952, -2000,

2037, 2063, 2080, 2087, 2085, 2075, 2057, 2032, 2001, 1962, 1919, 1870,
1817, 1759, 1698, 1634, 1567, 1498, 1428, 1356, 1283, 1210, 1137,
1064, 991, 919, 848, 779, 711, 645, 581, 519,

459, 401, 347, 294, 244, 197, 153, 111, 72, 36, 2, -29, -57, -83,
-106, -127, -146, -163, -177, -189, -200, -208, -215, -221, -224,
-227, -228, -228, -227, -225, -222, -218,

213, 208, 202, 196, 190, 183, 176, 169, 161, 154, 147, 139, 132, 125,
117, 111, 104, 97, 91, 85, 79, 73, 68, 63, 58, 53, 49, 45, 41, 38, 35,
31,

29, 26, 24, 21, 19, 17, 16, 14, 13, 11, 10, 9, 8, 7, 7, 6, 5, 5, 4, 4,
3, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1};

/**********************************************************************************************
     GLOBALS
***********************************************************************************************/

static struct TMS320AV120Chip tms320av120[MAX_TMS320AV120];		// Each Chip
static const struct TMS320AV120interface *intf;					// Pointer to the interface
static long layer1ScaleFactors[64];								// MPG Layer 1 Scale Factors
static int bitmasks[] = {0,1,3,7,0xF,0x1F,0x3F,0x7F,0xFF};		// Bit reading masks
//Matrix stuff
static const double MYPI=3.14159265358979323846;
static const char order[] = {0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30,
                             1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31};
static long phaseShiftsR[32], phaseShiftsI[32]; // 1.14
static long vShiftR[64], vShiftI[64]; // 1.13
static long D[512];

#if LOG_DATA_IN	
static FILE *fp;	//For logging
#endif

/**********************************************************************************************
     MPEG DATA HANDLING
***********************************************************************************************/

// return 2.16 requantized and scaled value
INLINE long Layer2Requant(long sample, long levels, int scaleIndex) {
   return (layer1ScaleFactors[scaleIndex] * (((sample+sample+1 - levels)<<15)/levels)) >> 14;
}

// subbandSamples input are 2.16
static void Matrix(long *V, long *subbandSamples, int numSamples) {
   int i,n,size,fftStep;
   long *workR=V; // Re-use V as work storage
   long workI[64]; // Imaginary part
   long *pWorkR;
   long *pWorkI;
   const char *next = order;
   int phaseShiftIndex, phaseShiftStep = 8;
   long tR,tI;
   long *pcmR;
   long *pcmI;

   for(i=numSamples;i<32;i++)
      subbandSamples[i]=0;

   // The initialization step here precalculates the
   // two-point transforms (each two inputs generate four outputs)
   // I'm taking advantage of the fact that my inputs are all real
   pWorkR = workR; // 2.16
   pWorkI = workI; // 2.16

   for(n=0;n<16;n++) {
      long a = subbandSamples[(int)*next++];
      long b = subbandSamples[(int)*next++];
      *pWorkR++ = a+b;  *pWorkI++ = 0; 
      *pWorkR++ = a;    *pWorkI++ = b; 
      *pWorkR++ = a-b;  *pWorkI++ = 0; 
      *pWorkR++ = a;    *pWorkI++ = -b;
   }

   // This is a fast version of the transform in the ISO standard.
   // It's derived using the same principles as the FFT,
   // but it's NOT a Fourier Transform. 

   // In each iteration, I throw out one bit of accuracy
   // This gives me an extra bit of headroom to avoid overflow
   for(size=4; size<64; size <<= 1) {
      // Since the first phase shfit value is always 1,
      // I can save a few multiplies by duplicating the loop
      for(n=0; n < 64; n += 2*size) {
         tR = workR[n+size];
         workR[n+size] = (workR[n] - tR)>>1;
         workR[n] = (workR[n] + tR)>>1;
         tI = workI[n+size];
         workI[n+size] = (workI[n] - tI)>>1;
         workI[n] = (workI[n] + tI)>>1;
      }
      phaseShiftIndex = phaseShiftStep;
      for(fftStep = 1; fftStep < size; fftStep++) {
         long phaseShiftR = phaseShiftsR[phaseShiftIndex];
         long phaseShiftI = phaseShiftsI[phaseShiftIndex];
         phaseShiftIndex += phaseShiftStep;
         for(n=fftStep; n < 64; n += 2*size) {
            tR = (phaseShiftR*workR[n+size] 
                     - phaseShiftI*workI[n+size])>>14;
            tI = (phaseShiftR*workI[n+size] 
                     + phaseShiftI*workR[n+size])>>14;
            workR[n+size] = (workR[n] - tR)>>1;
            workI[n+size] = (workI[n] - tI)>>1;
            workR[n] = (workR[n] + tR)>>1;
            workI[n] = (workI[n] + tI)>>1;
         }
      }
      phaseShiftStep /= 2;
   }

   // Build final V values by massaging transform output
   {
      // Now build V values from the complex transform output
      pcmR = workR+33; // 6.12
      pcmI = workI+33; // 6.12
      V[16] = 0;    // V[16] is always 0
      for(n=1;n<32;n++) {   // V is the real part, V is 6.9
         V[n+16] = (vShiftR[n] * *pcmR++ - vShiftI[n] * *pcmI++)>>15;
      }
      V[48] = (-workR[0])>>1;   // vShift[32] is always -1
      // Exploit symmetries in the result
      for(n=0;n<16;n++) V[n] = -V[32-n];
      for(n=1;n<16;n++) V[48+n] = V[48-n];
   }
}

//Convert Layer 1 or Layer 2 subband samples into pcm samples and put into pcm buffer
static void Layer12Synthesis(  int num,
					    long *V[16],
                        long *subbandSamples,
						int numSubbandSamples)
{
   INT16 *pcmSamples = &(tms320av120[num].pcmbuffer[tms320av120[num].pcm_pos]);
   int i,j;
   long *t = V[15];
   long *nextD;

   for(i=15;i>0;i--) // Shift V buffers over
      V[i] = V[i-1];
   V[0] = t;

   // Convert subband samples into PCM samples in V[0]
   Matrix(V[0],subbandSamples,numSubbandSamples);

   // D is 3.12, V is 6.9, want 16 bit output
   nextD = D;
   for(j=0;j<32;j++) {
      long sample = 0; // 8.16
      for(i=0;i<16;i+=2) {
         sample += (*nextD++ * V[i][j]) >> 8;
         sample += (*nextD++ * V[i+1][j+32]) >> 8;
      }
      *pcmSamples++ = sample >> 1; // Output samples are 16 bit
   }
}

//Return bits from the frame buffer
static long GetBits(int num, int numBits) {
   long result;
   if(tms320av120[num].bitsRemaining == 0) { // If no bits in this byte get from next byte in frame buffer!
      tms320av120[num].fb_pos++;				 // ...
	  //Make sure we're not out of data!
	  if(tms320av120[num].fb_pos > MPG_FRAMESIZE) {
		  LOG(("END OF FRAME BUFFER DATA IN GETBITS!\n"));
		  return 0;
	  }
      tms320av120[num].bitsRemaining = 8;
   }
   if(tms320av120[num].bitsRemaining >= numBits) { // Can I fill it from this byte?
      tms320av120[num].bitsRemaining -= numBits;
      return (tms320av120[num].framebuff[tms320av120[num].fb_pos]>>tms320av120[num].bitsRemaining) & bitmasks[numBits];
   }
   // Use up rest of this byte, then recurse to get more bits
   result = (tms320av120[num].framebuff[tms320av120[num].fb_pos] & bitmasks[tms320av120[num].bitsRemaining]) << (numBits - tms320av120[num].bitsRemaining);
   numBits -= tms320av120[num].bitsRemaining; // I don't need as many bits now
   tms320av120[num].bitsRemaining = 8;  // Move to next bit
   tms320av120[num].fb_pos++;
   return result | GetBits(num,numBits);
}

//Decode an MPEG1 - Layer 2 Frame
static void DecodeLayer2(int num)
{
int allocation[32];				// One set of allocation data for each subband
int scaleFactorSelection[32];	// One set of scale factor selectors for each subband
long sbSamples[3][32];			// Three sets/groups of subband samples for each of the 32 Subbands
int scaleFactor[3][32];			// Scale factors for each of the 3 groups of 32 Subbands
int sblimit=0;					// One past highest subband with non-empty allocation
int sb, sf, gp;
int scale_factor = 0;			//Current scale factor index
long levels = 0;				//Quantization level
Layer2BitAllocationTableEntry *allocationMap;

//Reset bits flag
tms320av120[num].bitsRemaining = 8;
   
// Select which allocation map to use
allocationMap = Layer2AllocationB2d;

// Retrieve allocation for each of the 32 Subbands
for(sb=0;sb<32;sb++) {
   if(allocationMap[sb]._numberBits) {
        allocation[sb] = GetBits(num,allocationMap[sb]._numberBits);
        if(allocation[sb] && (sb >= sblimit))
           sblimit=sb+1;
   } else
        allocation[sb] = 0;
}

// Retrieve scale factor selection information for each of the 32 subbands (skip non-used subbands)
for(sb=0; sb<sblimit; sb++)
	if(allocation[sb] != 0)
		scaleFactorSelection[sb] = GetBits(num,2);

// Read scale factors, using scaleFactorSelection to determine which scale factors apply to more than one group
for(sb=0; sb<sblimit; sb++)
{
    if(allocation[sb] != 0) {
       switch(scaleFactorSelection[sb]) {
       case 0: // Three scale factors
            scaleFactor[0][sb] = GetBits(num,6);
            scaleFactor[1][sb] = GetBits(num,6);
            scaleFactor[2][sb] = GetBits(num,6);
            break;
       case 1: // One for first two-thirds, one for last third
            scaleFactor[0][sb] = GetBits(num,6);
            scaleFactor[1][sb] = scaleFactor[0][sb];
            scaleFactor[2][sb] = GetBits(num,6);
            break;
       case 2: // One for all three
            scaleFactor[0][sb] = GetBits(num,6);
            scaleFactor[1][sb] = scaleFactor[0][sb];
            scaleFactor[2][sb] = scaleFactor[0][sb];
            break;
       case 3: // One for first third, one for last two-thirds
            scaleFactor[0][sb] = GetBits(num,6);
            scaleFactor[1][sb] = GetBits(num,6);
            scaleFactor[2][sb] = scaleFactor[1][sb];
            break;
	   }
	}
}
   
for(sf=0;sf<3;sf++) { // Diff't scale factors for each 1/3
	for(gp=0;gp<4;gp++) { // 4 groups of samples in each 1/3
         
		for(sb=0;sb<sblimit;sb++) { // Read 3 sets of 32 subband samples (skip non-used subbands)
			Layer2QuantClass *quantClass = allocationMap[sb]._quantClasses ? allocationMap[sb]._quantClasses[ allocation[sb] ] : 0 ;
			if(!allocation[sb]) { // No bits, store zero for each set
			    sbSamples[0][sb] = 0;
				sbSamples[1][sb] = 0;
				sbSamples[2][sb] = 0;
			}
			else {
				scale_factor = scaleFactor[sf][sb];	//Grab current scale factor for sf group and subband
				levels = quantClass->_levels;
				if (quantClass->_grouping) { // Grouped samples
					long s = GetBits(num,quantClass->_bits); // Get group
					// Separate out by computing successive remainders
					sbSamples[0][sb] = Layer2Requant(s % levels,levels,scale_factor);
					s /= levels;
					sbSamples[1][sb] = Layer2Requant(s % levels,levels,scale_factor);
					s /= levels;
					sbSamples[2][sb] = Layer2Requant(s % levels,levels,scale_factor);
				}
				else { // Ungrouped samples
					int width = quantClass->_bits;
					long s = GetBits(num,width); // Get 1st sample
					sbSamples[0][sb] = Layer2Requant(s,levels,scale_factor);
					s = GetBits(num,width); // Get 2nd sample
					sbSamples[1][sb] = Layer2Requant(s,levels,scale_factor);
					s = GetBits(num,width); // Get 3rd sample
					sbSamples[2][sb] = Layer2Requant(s,levels,scale_factor);
				}
			}
		}
		// Now, feed three sets of subband samples into synthesis engine
		Layer12Synthesis(num,tms320av120[num]._V,sbSamples[0],sblimit);
		tms320av120[num].pcm_pos += 32;
		Layer12Synthesis(num,tms320av120[num]._V,sbSamples[1],sblimit);
		tms320av120[num].pcm_pos += 32;
		Layer12Synthesis(num,tms320av120[num]._V,sbSamples[2],sblimit);
		tms320av120[num].pcm_pos += 32;
	}	//# of groups
}		//# of scale factors
}

/**********************************************************************************************
     tms320av120_update -- output samples to the stream buffer
***********************************************************************************************/
static void tms320av120_update(int num, INT16 *buffer, int length)
{
 int ii=0;

 //Pre-buffer must be full to process pcm data
 if(tms320av120[num].prebuff_full)	
 {
	/* fill in with samples until we hit the end or run out */
	for (ii = 0; ii < length; ii++) {
		//Ready to receive more data?
		if (tms320av120[num].sreq_line && tms320av120[num].sOut + CAP_PREBUFFER_SIZE*2 > CAP_PCMBUFFER_SIZE) {
			//Flag SREQ LO to request more data to come in!
			set_sreq_line(num,0);
		}
		//No more data ready for output, so abort
		if (tms320av120[num].sOut == tms320av120[num].pcm_pos){
			if(tms320av120[num].found_header)
				LOG(("TMS320AV120 #%d: No more pcm samples ready for output, skipping %d \n",num,length-ii));
			break;
		}
		//Send next pcm sample to output buffer (mute if it is set)
		buffer[ii] = tms320av120[num].pcmbuffer[tms320av120[num].sOut++] * (!tms320av120[num].mute);
		//Loop to beginning if we reach end of pcm buffer
		if( tms320av120[num].sOut == CAP_PCMBUFFER_SIZE)	
			tms320av120[num].sOut = 0;
		}
 }

 /* fill the rest with the silence */
 for ( ; ii < length; ii++)
	buffer[ii] = 0;
}

/**********************************************************************************************
     TMS320AV120_sh_start -- start emulation of the TMS320AV120
***********************************************************************************************/
int TMS320AV120_sh_start(const struct MachineSound *msound)
{
	int i, j, vi, failed=0;
	char stream_name[40];
	long *nextD = D;

	//Get reference to the interface
	intf = msound->sound_interface;

	/* initialize the chips */
	memset(&tms320av120, 0, sizeof(tms320av120));
	for (i = 0; i < intf->num; i++)
	{
		/*-- allocate a DAC stream at 32KHz --*/
		sprintf(stream_name, "%s #%d", sound_name(msound), i);
		tms320av120[i].stream = stream_init(stream_name, intf->mixing_level[i], 32000, i, tms320av120_update);

		//Fail if stream not created
		failed = (tms320av120[i].stream < 0);

		if(failed) break;

		//Create Synthesis Window vars
		for(vi=0;vi<16;vi++) {
		  //For each of the 16 values, we point to 64 PCM Samples (data type: long)
		  tms320av120[i]._V[vi] = (long*)malloc(64*sizeof(long));
		  //Ensure it was created..
		  if(!tms320av120[i]._V[vi]) {
			failed = 1;
			break;
		  }
		  //Initialize to 0
		  for(j=0;j<64;j++)
			tms320av120[i]._V[vi][j] = 0;
		}

		//Open for logging data
		#if LOG_DATA_IN	
		fp = fopen("c:\\tms320av120.mp3","wb");
		#endif
	}

	if(!failed) {
		//Create Scale Factor values
		for(i=0;i<63;i++)
			layer1ScaleFactors[i] = (long)(32767.0 * pow(2.0, 1.0 - i/3.0));
		// For speed, precompute all of the phase shift values
	    for(i=0;i<32;i++) { // 1.14
         phaseShiftsR[i] = (long)(16384.0*cos(i*(MYPI/32.0)));
         phaseShiftsI[i] = (long)(16384.0*sin(i*(MYPI/32.0)));
         vShiftR[i] = (long)(16384.0*cos((32+i)*(MYPI/64.0)));
         vShiftI[i] = (long)(16384.0*sin((32+i)*(MYPI/64.0)));
		}
		// Rearrange synthesis window coefficients into a more
		// useful order, and scale them to 3.12
		for(j=0;j<32;j++)
         for(i=0;i<16;i+=2) {
            *nextD++ = SynthesisWindowCoefficients[j+32*i]>>4;
            *nextD++ = SynthesisWindowCoefficients[j+32*i+32]>>4;
		 }
	}
	return failed;
}


/**********************************************************************************************
     TMS320AV120_sh_stop -- stop emulation of the TMS320AV120
***********************************************************************************************/

void TMS320AV120_sh_stop(void)
{
  int i,vi;

  //Close logging file
  #if LOG_DATA_IN	
  if(fp)	fclose(fp);
  #endif

  /*-- Delete memory we allocated dynamically --*/
  for (i = 0; i < intf->num; i++)
  {
	/*-- Delete Synth windows data --*/
	for(vi=0;vi<16;vi++)
	{
		if(tms320av120[i]._V[vi]) {
			free(tms320av120[i]._V[vi]);
			tms320av120[i]._V[vi] = NULL;
		}
	}
  }
}

/**********************************************************************************************
     TMS320AV120_sh_reset -- reset emulation of the TMS320AV120
***********************************************************************************************/

void TMS320AV120_sh_reset(void)
{
	int i;
	//Reset all chips - force a transition from active to reset, then reset to active
	for (i = 0; i < intf->num; i++) {
		tms320av120[i].reset = 0;
		TMS320AV120_set_reset(i,1);
		tms320av120[i].reset = 1;
		TMS320AV120_set_reset(i,0);
	}
}

/**********************************************************************************************
     TMS320AV120_sh_update -- called after every video frame
***********************************************************************************************/
void TMS320AV120_sh_update(void)
{
}

/**********************************************************************************************
     set_bof_line -- set's the state of the /BOF line
***********************************************************************************************/
static void set_bof_line(int chipnum, int state)
{
	//Keep track of what we set..
	tms320av120[chipnum].bof_line = state;
	//Call our callback if there is one
	if(intf->bof_line) intf->bof_line(chipnum,state);
}
/**********************************************************************************************
     set_sreq_line -- set's the state of the /SREQ line
***********************************************************************************************/
static void set_sreq_line(int chipnum, int state)
{
	//Keep track of what we set..
	tms320av120[chipnum].sreq_line = state;
	//Call our callback if there is one
	if(intf->sreq_line) intf->sreq_line(chipnum,state);
}

/**********************************************************************************************
     TMS320AV120_set_mute -- set/clear mute pin
***********************************************************************************************/
void TMS320AV120_set_mute(int chipnum, int state)
{
	//Act only on change of state
	if(state == tms320av120[chipnum].mute) return;
	//Update state
	tms320av120[chipnum].mute = state;
	//LOG(("TMS320AV120 #%d mute line set to %d!\n",chipnum,state));
}

/**********************************************************************************************
	TMS320AV120_set_reset -- set/clear reset pin
***********************************************************************************************/
void TMS320AV120_set_reset(int chipnum, int state)
{
	//Act only on change of state
	if(state == tms320av120[chipnum].reset) return;

	//LOG(("TMS320AV120 #%d reset line set to %d!\n",chipnum,state));

	//Transition from active to reset?
	if(!tms320av120[chipnum].reset && state)
	{
		//BOF & SREQ Line set to 1
		set_bof_line(chipnum,1);
		set_sreq_line(chipnum,1);
		//Mute is set
		tms320av120[chipnum].mute = 1;
		//Clear buffers
		tms320av120[chipnum].found_header = 0;
		tms320av120[chipnum].fb_pos = 0;
		tms320av120[chipnum].pcm_pos = 0;
		tms320av120[chipnum].prebuff_full = 0;
		tms320av120[chipnum].sOut = 0;
	}

	//Transition from reset to active?
	if(tms320av120[chipnum].reset && !state)
	{
		//SREQ Line set to 0
		set_sreq_line(chipnum,0);
		//Mute turned off
		tms320av120[chipnum].mute = 0;
	}

	//Update state
	tms320av120[chipnum].reset = state;
}

/**********************************************************************************************
     valid header? -- 4 bytes need to be: ff,fd,18 (or 19), c0
	 //NOTE: These are hardcoded to our specific sampling rate/bit rate, and other factors
***********************************************************************************************/
static int valid_header(int chipnum)
{
	if(tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos-4] == 0xff)
		if(tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos-3] == 0xfd)
			if(tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos-2] == 0x18 ||
			   tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos-2] == 0x19)
					if(tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos-1] == 0xc0)
						return 1;
	return 0;
}

/**********************************************************************************************
     TMS320AV120_data_w -- writes data to the chip and processes data
***********************************************************************************************/
WRITE_HANDLER( TMS320AV120_data_w )
{
	int chipnum = offset;

	//Safety check
	if( (intf->num-1) < offset) {
		//LOG(("TMS320AV120_DATA_W: Error trying to send data to undefined chip #%d\n",offset));
		return;
	}

	#if LOG_DATA_IN	
	//Log it for channel 0
	if(offset==0)
		if(fp) fputc(data,fp);
	#endif

	//ABORT IF SREQ HIGH!
	if(tms320av120[chipnum].sreq_line)	
	{
		LOG(("TMS320AV120 #%d: SREQ HI - Data Ignored!\n",chipnum));
		return;
	}

	//Write to input buffer
	tms320av120[chipnum].framebuff[tms320av120[chipnum].fb_pos++] = data;
	
	//If we don't have a header start looking once we have read enough bytes for one
	if(!tms320av120[chipnum].found_header && tms320av120[chipnum].fb_pos > MPG_HEADERSIZE-1)
	{
		//Reset BOF line
		set_bof_line(chipnum,1);
		tms320av120[chipnum].found_header = valid_header(chipnum);
		tms320av120[chipnum].fb_pos=0;	//Overwrite the header since we don't need it
	}

	//Once we have a valid header, see if we've got an entire frame
	if(tms320av120[chipnum].found_header && (tms320av120[chipnum].fb_pos == MPG_FRAMESIZE))
	{
		/*-- WE HAVE 1 FRAME --*/

		//Reset Frame Buffer position for reading
		tms320av120[chipnum].fb_pos = 0;

		//Handle PCM wraparound.. (Leave room for this frame)
		if( (tms320av120[chipnum].pcm_pos+1152) == CAP_PCMBUFFER_SIZE) {
			//Flag SREQ HI to stop data coming in!
			set_sreq_line(chipnum,1);
		}

		//Decode the frame (generates 1152 pcm samples)
		DecodeLayer2(chipnum);		 //note: tms320av120[chipnum].pcm_pos will be adjusted +1152 inside the decode function

		//Do we now have enough pre-buffer to begin?
		if(tms320av120[chipnum].pcm_pos == CAP_PREBUFFER_SIZE)
			tms320av120[chipnum].prebuff_full = 1;

		//Start over for next round if we're at the end of the buffer now!
		if(tms320av120[chipnum].pcm_pos == CAP_PCMBUFFER_SIZE)
			tms320av120[chipnum].pcm_pos=0;

		//Reset flag to search for next header
		tms320av120[chipnum].found_header = 0;

		//Reset frame buffer for next header
		tms320av120[chipnum].fb_pos = 0;

		//Set BOF Line low
		set_bof_line(chipnum,0);

		//Force stream to update
		//stream_update(tms320av120[chipnum].stream, 0);
	}
}

