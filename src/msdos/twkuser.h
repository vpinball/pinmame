#ifndef _TwkUser_h
#define _TwkUser_h

/*
	xxxxADDR defines the base port number used to access VGA component xxxx,
	and is defined for xxxx =
		ATTRCON		-	Attribute Controller
		MISC		-	Miscellaneous Register
		VGAENABLE	-	VGA Enable Register
		SEQ			-	Sequencer
		GRACON		-	Graphics Controller
		CRTC		-	Cathode Ray Tube Controller
		STATUS		-	Status Register
*/

#define ATTRCON_ADDR	0x3c0
#define MISC_ADDR		0x3c2
#define VGAENABLE_ADDR	0x3c3
#define SEQ_ADDR		0x3c4
#define GRACON_ADDR		0x3ce
#define CRTC_ADDR		0x3d4
#define STATUS_ADDR		0x3da


/*
	Note that the following C definition of Register is not compatible
	with the C++ definition used in the source code of TWEAK itself!
*/

typedef struct
	{
	unsigned port;
	unsigned char index;
	unsigned char value;
	} Register;

typedef Register *RegisterPtr;

void readyVgaRegs(void);
void outRegArray(Register *r, int n);
void outReg(Register r);
int loadRegArray(char *fpath, RegisterPtr *array);

#endif

