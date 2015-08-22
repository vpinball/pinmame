/* Chip types based on die marks from decap:
    chip type
    |||||| rom number
    |||||| |||||
    VVVVVV VVVVV
    T0280A 0281  = 1978 speak & spell, unknown difference to below, assumed same? uses old chirp
    T0280B 0281A = 1979 speak & spell, also == TMS5100, uses old chirp
    ?????? ????? (no decap; likely 'T0280D 0281D') = 1980 speak & spell, 1981 speak & spell compact, changed energy table, otherwise same as above, uses old chirp
    T0280F 2801A = 1980 speak & math, 1980 speak and read, uses old chirp
    ?????? ????? (no decap; likely 'T0280F 2802') = touch and tell, language translator; uses a unique chirp rom.
    ?????? ????? = TMS5110
    T0280F 5110A = TMS5110AN2L
*/

#define COEFF_ENERGY_SENTINEL	(511)

const static unsigned short kbits[10] = {
5, 5, 4, 4, 4, 4, 4, 3, 3, 3
};

#if 1

//this is the same as
//later_0280
//later_5110
//later_5110_5220
//later_0280
//=TMS5110A

/* This is the energy lookup table */ // == later_0280
const static unsigned short energytable[0x10]= {
 0,   1,   2,   3,   4,   6,   8,  11,
16,  23,  33,  47,  63,  85, 114, COEFF_ENERGY_SENTINEL
};

/* This is the pitch lookup table */ // == later_5110
const static unsigned short pitchtable[0x20]= {
 0,  15,  16,  17,  19,  21,  22,  25,
26,  29,  32,  36,  40,  42,  46,  50,
55,  60,  64,  68,  72,  76,  80,  84,
86,  93, 101, 110, 120, 132, 144, 159
};

/* These are the reflection coefficient lookup tables */

/* K1 is (5-bits -> 9 bits+sign, 2's comp. fractional (-1 < x < 1) */

const static short ktable[10][0x20]= { // == later_5110_5220
{
-501, -498, -497, -495, -493, -491, -488, -482,
-478, -474, -469, -464, -459, -452, -445, -437,
-412, -380, -339, -288, -227, -158,  -81,   -1,
  80,  157,  226,  287,  337,  379,  411,  436
},
{
-328, -303, -274, -244, -211, -175, -138,  -99,
 -59,  -18,   24,   64,  105,  143,  180,  215,
 248,  278,  306,  331,  354,  374,  392,  408,
 422,  435,  445,  455,  463,  470,  476,  506
},
{
-441, -387, -333, -279, -225, -171, -117,  -63,
  -9,   45,   98,  152,  206,  260,  314,  368
},
{
-328, -273, -217, -161, -106,  -50,    5,   61,
 116,  172,  228,  283,  339,  394,  450,  506
},
{
-328, -282, -235, -189, -142,  -96,  -50,   -3,
  43,   90,  136,  182,  229,  275,  322,  368
},
{
-256, -212, -168, -123,  -79,  -35,   10,   54,
  98,  143,  187,  232,  276,  320,  365,  409
},
{
-308, -260, -212, -164, -117,  -69,  -21,   27,
  75,  122,  170,  218,  266,  314,  361,  409
},
{
-256, -161,  -66,   29,  124,  219,  314,  409
},
{
-256, -176,  -96,  -15,   65,  146,  226,  307
},
{
-205, -132,  -59,   14,   87,  160,  234,  307
}
};

/* chirp table */

const static char chirptable[52]= { // == later_0280
	0x00, 0x03, 0x0f, 0x28, 0x4c, 0x6c, 0x71, 0x50,
	0x25, 0x26, 0x4c, 0x44, 0x1a, 0x32, 0x3b, 0x13,
	0x37, 0x1a, 0x25, 0x1f, 0x1d, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};

/* interpolation coefficients */

const static char interp_coeff[8] = {
3, 3, 3, 2, 2, 2, 1, 0
};

#else

/* common, shared coefficients */

/* energy */
const static unsigned short patent_0280_energytable[0x10]= {
 0,  0,  1,  1,  2,  3,  5,  7,
10, 15, 21, 30, 43, 61, 86, COEFF_ENERGY_SENTINEL // last rom value is actually really 0, but the tms5110.c code still requires the sentinel value to function correctly, until it is properly updated or merged with tms5220.c
};

const static unsigned short later_0280_energytable[0x10]= {
 0,  1,  2,  3,  4,  6,  8, 11,
16, 23, 33, 47, 63, 85,114, COEFF_ENERGY_SENTINEL // last rom value is actually really 0, but the tms5110.c code still requires the sentinel value to function correctly, until it is properly updated or merged with tms5220.c
};

//technically this is the same as above, but tms5220.c expects the 0 rather than needing a sentinel value
//TODO: when tms5110.c no longer needs sentinel, get rid of this and merge with above.
const static unsigned short later_0285_energytable[0x10]= {
 0,  1,  2,  3,  4,  6,  8, 11,
16, 23, 33, 47, 63, 85,114, 0
};

/* pitch */
const static unsigned short patent_0280_2801_pitchtable[0x20]= {
 0,   41,  43,  45,  47,  49,  51,  53,
 55,  58,  60,  63,  66,  70,  73,  76,
 79,  83,  87,  90,  94,  99, 103, 107,
112, 118, 123, 129, 134, 140, 147, 153
};

const static unsigned short later_2802_pitchtable[0x20]= {
  0,  16,  18,  19,  21,  24,  26,  28,
 31,  35,  37,  42,  44,  47,  50,  53,
 56,  59,  63,  67,  71,  75,  79,  84,
 89,  94, 100, 106, 112, 126, 141, 150
};

const static unsigned short later_5110_pitchtable[0x20]= {
  0,  15,  16,  17,  19,  21,  22,  25,
 26,  29,  32,  36,  40,  42,  46,  50,
 55,  60,  64,  68,  72,  76,  80,  84,
 86,  93, 101, 110, 120, 132, 144, 159
};

const static unsigned short later_2501E_pitchtable[0x40]= {
  0,  14,  15,  16,  17,  18,  19,  20,
 21,  22,  23,  24,  25,  26,  27,  28,
 29,  30,  31,  32,  34,  36,  38,  40,
 41,  43,  45,  48,  49,  51,  54,  55,
 57,  60,  62,  64,  68,  72,  74,  76,
 81,  85,  87,  90,  96,  99, 103, 107,
112, 117, 122, 127, 133, 139, 145, 151,
157, 164, 171, 178, 186, 194, 202, 211
};

const static unsigned short later_5220_pitchtable[0x40]= {
  0,  15,  16,  17,  18,  19,  20,  21,
 22,  23,  24,  25,  26,  27,  28,  29,
 30,  31,  32,  33,  34,  35,  36,  37,
 38,  39,  40,  41,  42,  44,  46,  48,
 50,  52,  53,  56,  58,  60,  62,  65,
 68,  70,  72,  76,  78,  80,  84,  86,
 91,  94,  98, 101, 105, 109, 114, 118,
122, 127, 132, 137, 142, 148, 153, 159
};

/* LPC */
/* These are the reflection coefficient lookup tables */
/* K1 is (5-bits -> 9 bits+sign, 2's comp. fractional (-1 < x < 1) */
const static short patent_ktable[10][0x20]= { // K1..K10
{
-501, -497, -493, -488, -480, -471, -460, -446,
-427, -405, -378, -344, -305, -259, -206, -148,
-86,  -21,   45,  110,  171,  227,  277,  320,
357,  388,  413,  434,  451,  464,  474,  498
},
{
-349, -328, -305, -280, -252, -223, -192, -158,
-124,  -88,  -51,  -14,  23,    60,   97,  133,
 167,  199,  230,  259,  286,  310,  333,  354,
 372,  389,  404,  417,  429,  439,  449,  506
},
{
-397, -365, -327, -282, -229, -170, -104, -36,
  35,  104,  169,  228,  281,  326,  364, 396
},
{
-369, -334, -293, -245, -191, -131, -67,  -1,
  64,  128,  188,  243,  291,  332, 367, 397
},
{
-319, -286, -250, -211, -168, -122, -74, -25,
  24,   73,  121,  167,  210,  249, 285, 318
},
{
-290, -252, -209, -163, -114,  -62,  -9,  44,
  97,  147,  194,  238,  278,  313, 344, 371
},
{
-291, -256, -216, -174, -128, -80, -31,  19,
  69,  117,  163,  206,  246, 283, 316, 345
},
{
-218, -133,  -38,  59,  152,  235, 305, 361
},
{
-226, -157,  -82,  -3,   76,  151, 220, 280
},
{
-179, -122,  -61,    1,   62,  123, 179, 231
}
};

const static short later_2801_2501E_ktable[10][0x20]= { // K1..K10
{ -501, -498, -495, -490, -485, -478, -469, -459,
  -446, -431, -412, -389, -362, -331, -295, -253,
  -207, -156, -102,  -45,   13,   70,  126,  179,
   228,  272,  311,  345,  374,  399,  420,  437 },
{ -376, -357, -335, -312, -286, -258, -227, -195,
  -161, -124,  -87,  -49,  -10,   29,   68,  106,
   143,  178,  212,  243,  272,  299,  324,  346,
   366,  384,  400,  414,  427,  438,  448,  506 },
{ -407, -381, -349, -311, -268, -218, -162, -102,
   -39,   25,   89,  149,  206,  257,  302,  341 },
{ -290, -252, -209, -163, -114,  -62,   -9,   44,
    97,  147,  194,  238,  278,  313,  344,  371 },
{ -318, -283, -245, -202, -156, -107,  -56,   -3,
    49,  101,  150,  196,  239,  278,  313,  344 },
{ -193, -152, -109,  -65,  -20,   26,   71,  115,
   158,  198,  235,  270,  301,  330,  355,  377 },
{ -254, -218, -180, -140,  -97,  -53,   -8,   36,
    81,  124,  165,  204,  240,  274,  304,  332 },
{ -205, -112,  -10,   92,  187,  269,  336,  387 },
{ -249, -183, -110,  -32,   48,  126,  198,  261 }, /* on patents 4,403,965 and 4,946,391 the 4th entry is 0x3ED (-19) which is a typo of the correct value of 0x3E0 (-32)*/\
{ -190, -133,  -73,  -10,   53,  115,  173,  227 }
};

// below is the same as 2801/2501E above EXCEPT for K4 which is completely different.
const static short later_2802_ktable[10][0x20]= { // K1..K10
{ -501, -498, -495, -490, -485, -478, -469, -459,
  -446, -431, -412, -389, -362, -331, -295, -253,
  -207, -156, -102,  -45,   13,   70,  126,  179,
   228,  272,  311,  345,  374,  399,  420,  437},
{ -376, -357, -335, -312, -286, -258, -227, -195,
  -161, -124,  -87,  -49,  -10,   29,   68,  106,
   143,  178,  212,  243,  272,  299,  324,  346,
   366,  384,  400,  414,  427,  438,  448,  506},
{ -407, -381, -349, -311, -268, -218, -162, -102,
   -39,   25,   89,  149,  206,  257,  302,  341},
{ -289, -248, -202, -152,  -98,  -43,   14,   71,
   125,  177,  225,  269,  307,  341,  371,  506},
{ -318, -283, -245, -202, -156, -107,  -56,   -3,
    49,  101,  150,  196,  239,  278,  313,  344},
{ -193, -152, -109,  -65,  -20,   26,   71,  115,
   158,  198,  235,  270,  301,  330,  355,  377},
{ -254, -218, -180, -140,  -97,  -53,   -8,   36,
    81,  124,  165,  204,  240,  274,  304,  332},
{ -205, -112,  -10,   92,  187,  269,  336,  387},
{ -249, -183, -110,  -32,   48,  126,  198,  261},
{ -190, -133,  -73,  -10,   53,  115,  173,  227}
};

const static short later_5110_5220_ktable[10][0x20]= { // K1..K10
{ -501, -498, -497, -495, -493, -491, -488, -482,
  -478, -474, -469, -464, -459, -452, -445, -437,
  -412, -380, -339, -288, -227, -158,  -81,   -1,
    80,  157,  226,  287,  337,  379,  411,  436 },
{ -328, -303, -274, -244, -211, -175, -138,  -99,
   -59,  -18,   24,   64,  105,  143,  180,  215,
   248,  278,  306,  331,  354,  374,  392,  408,
   422,  435,  445,  455,  463,  470,  476,  506 },
{ -441, -387, -333, -279, -225, -171, -117,  -63,
    -9,   45,   98,  152,  206,  260,  314,  368  },
{ -328, -273, -217, -161, -106,  -50,    5,   61,
   116,  172,  228,  283,  339,  394,  450,  506  },
{ -328, -282, -235, -189, -142,  -96,  -50,   -3,
    43,   90,  136,  182,  229,  275,  322,  368  },
{ -256, -212, -168, -123,  -79,  -35,   10,   54,
    98,  143,  187,  232,  276,  320,  365,  409  },
{ -308, -260, -212, -164, -117,  -69,  -21,   27,
    75,  122,  170,  218,  266,  314,  361,  409  },
{ -256, -161,  -66,   29,  124,  219,  314,  409  },
{ -256, -176,  -96,  -15,   65,  146,  226,  307  },
{ -205, -132,  -59,   14,   87,  160,  234,  307  }
};

/* chirp table */
const static char patent_0280_chirptable[52]= {
  0x00, 0x2a, 0xd4, 0x32, 0xb2, 0x12, 0x25, 0x14,
  0x02, 0xe1, 0xc5, 0x02, 0x5f, 0x5a, 0x05, 0x0f,
  0x26, 0xfc, 0xa5, 0xa5, 0xd6, 0xdd, 0xdc, 0xfc,
  0x25, 0x2b, 0x22, 0x21, 0x0f, 0xff, 0xf8, 0xee,
  0xed, 0xef, 0xf7, 0xf6, 0xfa, 0x00, 0x03, 0x02,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// almost, but not exactly the same as the patent chirp above (25 bits differ)
const static char patent_0281_chirptable[52]= {
  0x00, 0x2b, 0xd4, 0x33, 0xb3, 0x12, 0x25, 0x14,
  0x02, 0xe2, 0xc6, 0x03, 0x60, 0x5b, 0x05, 0x0f,
  0x26, 0xfc, 0xa6, 0xa5, 0xd6, 0xdd, 0xdd, 0xfd,
  0x25, 0x2b, 0x23, 0x22, 0x0f, 0xff, 0xf8, 0xef,
  0xed, 0xef, 0xf7, 0xf7, 0xfa, 0x01, 0x04, 0x03,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

const static char patent_0282_chirptable[52]= {
  0x00, 0xa5, 0xbd, 0xee, 0x34, 0x73, 0x7e, 0x3d,\
  0xe8, 0xea, 0x34, 0x24, 0xd1, 0x01, 0x13, 0xc3,\
  0x0c, 0xd2, 0xe7, 0xdd, 0xd9, 0x00, 0x00, 0x00,\
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\
  0x00, 0x00, 0x00, 0x00
};

const static char later_chirptable[52]= {
  0x00, 0x03, 0x0f, 0x28, 0x4c, 0x6c, 0x71, 0x50,
  0x25, 0x26, 0x4c, 0x44, 0x1a, 0x32, 0x3b, 0x13,
  0x37, 0x1a, 0x25, 0x1f, 0x1d, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

/* interpolation coefficients */

const static char interp_coeff[8] = {
3, 3, 3, 2, 2, 1, 1, 0
};

//
//
//

/* TMS5100/TMC0281:
   (Die revs A, B; 1977?-1981?)
   The TMS5100NL was decapped and imaged by digshadow in April, 2013.
    The LPC table is verified to match the decap.
    It also matches the intended contents of US Patent 4,209,836 and several others.
    The chirp table is verified to match the decap, and also matches the patents.
   In April, 2013, digshadow decapped a TMS5100 chip from 1980: http://siliconpr0n.org/map/ti/tms5100nl/
    The TMS5100 had the die markings: "0281 B  281A"
   In December 2014, Sean riddle decapped a TMC0281 chip from 1978 from an early speak
   and spell.
    The TMC0281 die had the die markings "0281 A  281"
    The chirp table matches what digshadow had decapped earlier.
    The LPC table hasn't been fully typed yet.
   Digitally dumped via PROMOUT by PlgDavid in 2014 for verification.
*/
//TI_0280_PATENT_ENERGY
//TI_0280_2801_PATENT_PITCH
//TI_0280_PATENT_LPC
//TI_0280_PATENT_CHIRP


/* TMS5110A/TMC0281D:
   This chip is used on the later speak & spell, and speak & spell compact;
   The energy table differs from the original tmc0281/tms5100, as does the interpolation behavior,
   which is the 'alternate' behavior.
   The chips have datecodes in the 1983-1984 range, probably 1982 also.
   Digitally dumped via PROMOUT by PlgDavid in 2014
*/
//TI_0280_LATER_ENERGY
//TI_0280_2801_PATENT_PITCH
//TI_0280_PATENT_LPC
//TI_0280_PATENT_CHIRP


/* TMC0280/CD2801:
   Used in the Speak & Math, Speak & Read, and Language Translator
   Decapped by Digshadow in 2014 http://siliconpr0n.org/map/ti/tmc0280fnl/
   Digitally dumped via PROMOUT by PlgDavid in 2014
   The coefficients are exactly the same as the TMS5200.
   The coefficients also come from US Patents 4,403,965 and 4,946,391 (with
   one typo in the patent).
   The chirp table is very slightly different from the 4,209,836 patent one,
   but matches the table in the 4,403,965 and 4,946,391 patents.
   The Mitsubishi M58817 also seems to work best with these coefficients, so
   it is possible the engineers of that chip copied them from the TI patents.
   ***TODO: there are 2 versions of this chip, and the interpolation
      behavior between the two differs slightly:
   * TMC0280NLP // CD2801 with datecodes around 1980 has the same
     interpolation inhibit behavior as 5100/TMC0281 on unvoiced->silent
     transition.
   * CD2801A-NL with datecodes around 1982 have the 'alternate behavior'
*/
//TI_0280_LATER_ENERGY
//TI_0280_2801_PATENT_PITCH
//TI_2801_2501E_LPC
//TI_2801_PATENT_CHIRP


/* Mitsubishi M58817
The Mitsubishi M58817 seems to have (partly?) copied the coefficients from the
TMC0280/CD2801 above, but has some slight differences to it within the chip:
the main accumulator seems to have 1 extra bit and the digital values are
tapped 1 bit higher than on the TI chips. This is emulated within tms5110.c
*/
//TI_0280_LATER_ENERGY
//TI_0280_2801_PATENT_PITCH
//TI_2801_2501E_LPC
//TI_2801_PATENT_CHIRP


/* CD2802:
   (1984 era?)
   Used in Touch and Tell only (and Vocaid?), this chip has a unique pitch, LPC and chirp table.
   Has the 'alternate' interpolation behavior.
*/
//TI_0280_LATER_ENERGY
//TI_2802_PITCH
//TI_2802_LPC
//TI_2802_CHIRP


/* TMS5110A:
   (1984-90 era? early chips may be called TMS5110C; later chips past 1988 or so may be called TSP5110A)
   The TMS5110A LPC coefficients were originally read from an actual TMS5110A
   chip by Jarek Burczynski using the PROMOUT pin, later verified/redumped
   by PlgDavid.
   NullMoogleCable decapped a TMS5110AN2L in 2015: http://wtfmoogle.com/wp-content/uploads/2015/03/0317_1.jpg
   which was used to verify the chirp table.
   The slightly older but otherwise identical TMS5111NLL was decapped and imaged by digshadow in April, 2013.
   The die is marked "TMS5110AJ"
   The LPC table is verified from decap to match the values from Jarek and PlgDavid's PROMOUT dumps of the TMS5110.
   The LPC table matches that of the TMS5220.
   It uses the 'newer' 5200-style chirp table.
   It has the 'alternate' interpolation behavor (tested on 5110a; 5111 behavior is unknown)
*/
//TI_0280_LATER_ENERGY
//TI_5110_PITCH
//TI_5110_5220_LPC
//TI_LATER_CHIRP


/* TMS5200/CD2501E:
   (1979-1983 era)
The TMS5200NL was decapped and imaged by digshadow in March, 2013.
It is equivalent to the CD2501E (internally: "TMC0285") chip used
 on the TI 99/4(A) speech module.
The LPC table is verified to match the decap.
 (It was previously dumped with PROMOUT which matches as well)
The chirp table is verified to match the decap. (sum = 0x3da)
Note that the K coefficients are VERY different from the coefficients given
 in the US 4,335,277 patent, which may have been for some sort of prototype or
 otherwise intentionally scrambled. The energy and pitch tables, however, are
 identical to that patent.
Also note, that the K coefficients are identical to the coefficients from the
 CD2801 (which itself is almost identical to the CD2802).
NOTE FROM DECAP: immediately to the left of each of the K1,2,3,4,5,and 6
 coefficients in the LPC rom are extra columns containing the constants
 -510, -502, 313, 318, or in hex 0x202, 0x20A, 0x139, 0x13E.
 Those EXACT constants DO appear (rather nonsensically) on the lpc table in US
 patent 4,335,277. They are likely related to the multiplicative interpolator
 described in us patent 4,419,540; whether the 5200/2501E and the 5220 or 5220C
 actually implement this interpolator or not is unclear. This interpolator
 seems intended for chips with variable frame rate, so it may only exist
 on the TMS/TSP5220C and CD2501ECD.
*/
//TI_0285_LATER_ENERGY
//TI_2501E_PITCH
//TI_2801_2501E_LPC
//TI_LATER_CHIRP


/* TMS5220/5220C:
   (1983 era for 5220, 1986-1992 era for 5220C; 5220C may also be called TSP5220C)
The TMS5220NL was decapped and imaged by digshadow in April, 2013.
The LPC table table is verified to match the decap.
The chirp table is verified to match the decap. (sum = 0x3da)
Note that all the LPC K* values match the TMS5110a table (as read via PROMOUT)
exactly.
The TMS5220CNL was decapped and imaged by digshadow in April, 2013.
The LPC table table is verified to match the decap and exactly matches TMS5220NL.
The chirp table is verified to match the decap. (sum = 0x3da)
*/
//TI_0285_LATER_ENERGY
//TI_5220_PITCH
//TI_5110_5220_LPC
//TI_LATER_CHIRP


/* The following Sanyo VLM5030 coefficients are derived from decaps of the chip
done by ogoun, plus image stitching done by John McMaster. The organization of
coefficients beyond k2 is derived from work by Tatsuyuki Satoh.
The actual coefficient rom on the chip die has 5 groups of bits only:
Address |   K1A   |   K1B   |   K2   | Energy | Pitch |
Decoder |   K1A   |   K1B   |   K2   | Energy | Pitch |
K1A, K1B and K2 are 10 bits wide, 32 bits long each.
Energy and pitch are both 7 bits wide, 32 bits long each.
K1A holds odd values of K1, K1B holds even values.
K2 holds values for K2 only
K3 and K4 are actually the table index values <<6
K5 thru K10 are actually the table index values <<7
The concept of only having non-binary weighted reflection coefficients for the
first two k stages is mentioned in Markel & Gray "Linear Prediction of Speech"
and in Thomas Parsons' "Voice and Speech Processing"
*/

#endif
