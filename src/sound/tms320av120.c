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
 *         1) Redo buffers
 *         2) Implement /BOF and /SREQ callbacks
 *         3) Implement data handlers
 *         4) Remove current hack to feed data to the chip for playback
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

/**********************************************************************************************
     CONSTANTS
***********************************************************************************************/
#define TMS320AV120_BUFFER			512							//Chip has a 512 byte internal buffer
#define CAP_PCMBUFFER_SIZE			1152						//Hold 1 decoded frame
#define CAP_OUTBUFFER_SIZE			4096						//Output buffer
#define CAP_BUFFER_MASK				(CAP_OUTBUFFER_SIZE - 1)	//Output buffer mask
#define MPG_FRAMESIZE				140							//Mpeg1 - Layer 2 Framesize @ 32KHz/32kbps
#define LOOP_MPG_SAMPLE				0

/**********************************************************************************************
     INTERNAL DATA STRUCTURES
***********************************************************************************************/
struct TMS320AV120Chip
{
 UINT8  inputbuff[TMS320AV120_BUFFER];	//Holds raw mpg data (including header)
 INT16  pcmbuffer[CAP_PCMBUFFER_SIZE];
 INT16  *buffer;					//Output Buffer
 UINT32 sIn;						//Position of next sample to load into buffer
 UINT32 sOut;						//Position of next sample to read out of the buffer
 INT8	*start_region;				//Points to start of rom region
 UINT32 curr;						//Current position in memory region
 int    stream;						//Holds stream channel assignment
 UINT8  framebuff[MPG_FRAMESIZE];	//Holds raw mpg data for 1 frame
 UINT8	fb_pos;						//Current frame buffer position
 UINT16 pcm_pos;					//Position of PCM buffer
 int    mute;						//Mute status (0 = off, 1 = Mute )
 int    bitsRemaining;				// Keep track of # of bits we've read from frame buffer
 long *_V[16];						// Synthesis window for single channel
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
void Layer12Synthesis(  int num,
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
		  logerror("END OF FRAME BUFFER DATA IN GETBITS!\n");
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
void DecodeLayer2(int num)
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


//Get One Byte - Reads a single byte from the rom stream, returns 0 if unable to read the byte
static int get_one_byte(int num, UINT8* byte, UINT32 pos)
{
	//If reached end of memory region, abort
	if(pos>0x300000) return 0;
	*byte = (UINT8)tms320av120[num].start_region[pos];
	return 1;
}

//Read current frame data into our frame buffer (assumes rom positioned at start of frame) - return 0 if unable to read entire frame
static int Read_Frame(int num)
{	
	UINT8 onebyte;
	int i;

	//clear current frame buffer
	memset(tms320av120[num].framebuff,0,sizeof(tms320av120[num].framebuff));

	//Fill entire frame buffer if we can
	for(i=0; i<MPG_FRAMESIZE; i++) {
		if(!get_one_byte(num,&onebyte,tms320av120[num].curr)) 
			return 0;
		tms320av120[num].framebuff[i] = onebyte;
		tms320av120[num].curr++;
	}
	//Reset position
	tms320av120[num].fb_pos = 0;
	return 1;
}

//Find next MPG Header in rom stream, return 0 if not found - current position will be at start of frame if found
static int Find_MPG_Header(int num)
{	
	UINT8 onebyte;
	int quit = 0;
	while(!quit) {
		//Find start of possible syncword (begins with 0xff)
		do {
			//Grab 1 byte
			if(!get_one_byte(num,&onebyte,tms320av120[num].curr)) 
				quit=1;
			else
				tms320av120[num].curr++;
		}while(onebyte!=0xff && !quit);

		//Found 0xff?
		if(onebyte==0xff) {
			//Next byte must be 0xfd (note: already positioned on it from above while() code)
			if(get_one_byte(num, &onebyte,tms320av120[num].curr) && onebyte==0xfd) {
				//Next byte must be 0x18 or 0x19
				if(get_one_byte(num,&onebyte,tms320av120[num].curr+1) && (onebyte==0x18 || onebyte==0x19)) {
					//Next byte must be 0xc0
					if(get_one_byte(num,&onebyte,tms320av120[num].curr+2) && onebyte==0xc0) {
						tms320av120[num].curr+=3;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

//Return Next Decoded Word (16 Bit Signed Data) from ROM Stream
static INT16 cap_GetNextWord(int num)
{
#if LOOP_MPG_SAMPLE
   //If reached end of memory region, start again
   if (tms320av120[num].curr>0x300000)
	   tms320av120[num].curr=0;
#else
   //If reached end of memory region, we're done..
   if (tms320av120[num].curr>0x300000)
	   return 0;
#endif

   //Do we need more data to be decoded?
   if (tms320av120[num].pcm_pos+1 > CAP_PCMBUFFER_SIZE) {

		//Find Next Valid Header - Abort if not found
		if(!Find_MPG_Header(num))
				return 0;
		//Read Frame Data
		if(!Read_Frame(num))
			return 0;

		//Reset position for writing the pcm data
		tms320av120[num].pcm_pos = 0;

		//Decode the frame
		DecodeLayer2(num);

		//Reset position for reading the pcm data
		tms320av120[num].pcm_pos = 0;
   }

   //Return the next word from pcm buffer
   return (INT16)tms320av120[num].pcmbuffer[tms320av120[num].pcm_pos++];
}

//Fill our pcm buffer with decoded data until buffer is full or until we catch up to last sample output from buffer
void tms_FillBuff(int num) {
  int length = CAP_OUTBUFFER_SIZE-10;
  int ii;
  INT16 word;
  UINT32 tmpN;

  //Safety check
  if(num > intf->num - 1) return;

  /* fill in with bytes until we hit the end or run out */
  for (ii = 0; ii < length; ii++) {

    tmpN = (tms320av120[num].sIn + 1) & CAP_BUFFER_MASK;

    //Abort if we're about to pass output samples..
	if (tmpN == tms320av120[num].sOut) break;

	//Grab next word from decoded pcm stream
	word = cap_GetNextWord(num);
 
	//Update our buffer
    tms320av120[num].buffer[tms320av120[num].sIn] = word;

	//Increment next sample in position
	tms320av120[num].sIn = (tms320av120[num].sIn + 1) & CAP_BUFFER_MASK;
  }
}

/**********************************************************************************************
     set_bof_line -- set's the state of the /BOF line
***********************************************************************************************/
static void set_bof_line(int chipnum, int state)
{
	if(intf->bof_line[chipnum]) intf->bof_line[chipnum](state);
}
/**********************************************************************************************
     set_sreq_line -- set's the state of the /SREQ line
***********************************************************************************************/
static void set_sreq_line(int chipnum, int state)
{
	if(intf->sreq_line[chipnum]) intf->sreq_line[chipnum](state);
}

/**********************************************************************************************
     tms320av120_update -- output samples to the stream buffer
***********************************************************************************************/
static void tms320av120_update(int num, INT16 *buffer, int length)
{
 int ii;
 /* fill in with samples until we hit the end or run out */
 for (ii = 0; ii < length; ii++) {
	if (tms320av120[num].sOut == tms320av120[num].sIn)	break;				//No more data in buffer, so abort
    buffer[ii] = tms320av120[num].buffer[tms320av120[num].sOut] * (!tms320av120[num].mute);
    tms320av120[num].sOut = (tms320av120[num].sOut + 1) & CAP_BUFFER_MASK;	//Increment current samples out position
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

		/*-- allocate memory for our buffer --*/
		tms320av120[i].buffer = malloc(CAP_OUTBUFFER_SIZE * sizeof(INT16));

		//Fail if stream or buffer not created
		failed = (tms320av120[i].stream < 0 || (tms320av120[i].buffer==0));

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

		//point to beg of rom data
		tms320av120[i].start_region = (INT8*)memory_region(REGION_SOUND1);
		tms320av120[i].curr  = 0;

		//Must initialize these..
		tms320av120[i].pcm_pos = CAP_PCMBUFFER_SIZE+1;	//Cause PCM buffer to read 1st time
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
  int i;

  /*-- Delete our buffer --*/
  for (i = 0; i < intf->num; i++)
  {
	if (tms320av120[i].buffer) { 
		free(tms320av120[i].buffer);
		tms320av120[i].buffer = NULL; 
	}
	
	/*-- Delete Synth windows data --*/
	for(i=0;i<16;i++)
	{
		if(tms320av120[i]._V[i]) {
			free(tms320av120[i]._V[i]);
			tms320av120[i]._V[i] = NULL;
		}
	}
  }
}

/**********************************************************************************************
     TMS320AV120_sh_reset -- reset emulation of the TMS320AV120
***********************************************************************************************/

void TMS320AV120_sh_reset(void)
{
}

/**********************************************************************************************
     TMS320AV120_sh_update -- called after every video frame
***********************************************************************************************/
void TMS320AV120_sh_update(void)
{
}

/**********************************************************************************************
     tms320av120_data_write -- send data to the chip
***********************************************************************************************/
static void tms320av120_data_write(struct TMS320AV120Chip *chip, data8_t data)
{
}

/**********************************************************************************************
     TMS320AV120_set_mute -- set/clear mute pin
***********************************************************************************************/
void TMS320AV120_set_mute(int chipnum, int state)
{
	tms320av120[chipnum].mute = state;
	//printf("TMS320AV120 #%d mute line set to %d!\n",chipnum,state);
}

/**********************************************************************************************
     //TMS320AV120_reset -- Resets the chip
	TMS320AV120_set_reset -- set/clear reset pin
***********************************************************************************************/
//void TMS320AV120_reset(int chipnum)
void TMS320AV120_set_reset(int chipnum, int state)
{
	//printf("TMS320AV120 #%d reset line set to %d!\n",chipnum,state);
	//Todo: put reset code here..
}

/**********************************************************************************************
     TMS320AV120_data_0_w -- send data to the chip
***********************************************************************************************/
WRITE_HANDLER( TMS320AV120_data_0_w )
{
	tms320av120_data_write(&tms320av120[0], data);
}
WRITE_HANDLER( TMS320AV120_data_1_w )
{
	tms320av120_data_write(&tms320av120[1], data);
}
