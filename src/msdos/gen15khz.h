#include "TwkUser.h"
/* monitor types */
#include "monitors.h"

/* generic VGA CRTC register indexes */
#define	HZ_DISPLAY_TOTAL	0x00
#define	HZ_DISPLAY_END		0x01
#define	CRTC_OVERFLOW		0x07
#define	VT_DISPLAY_END		0x12
#define	MEM_OFFSET			0x13

/* indices into our register array */
#define	CLOCK_INDEX				0
#define	H_TOTAL_INDEX			1
#define	H_DISPLAY_INDEX			2
#define	H_BLANKING_START_INDEX	3
#define	H_BLANKING_END_INDEX	4
#define	H_RETRACE_START_INDEX	5
#define	H_RETRACE_END_INDEX		6
#define	V_TOTAL_INDEX			7
#define	OVERFLOW_INDEX			8
#define	MAXIMUM_SCANLINE_INDEX	10
#define	V_RETRACE_START_INDEX	11
#define	V_RETRACE_END_INDEX		12
#define	V_END_INDEX				13
#define	MEM_OFFSET_INDEX		14
#define	UNDERLINE_LOC_INDEX		15
#define	V_BLANKING_START_INDEX	16
#define	V_BLANKING_END_INDEX	17
#define MODE_CONTROL_INDEX		18
#define	MEMORY_MODE_INDEX		20


/* generic VGA functions */
int	readCRTC(int nIndex);
int	getVtEndDisplay(void);

/* tweak values */
extern unsigned char tw640x480arc_h;
extern unsigned char tw640x480arc_v;

/* structure for 15.75KHz SVGA drivers */
typedef struct
{
	char name[64]; /* name of chipset/card */
	int (*detectsvgacard)(void);	/* checks for a specific chipset */
	int (*getlogicalwidth)(int width);	/*the logical width of the mode created */
	int (*setSVGA15KHzmode)(int vdouble, int width, int height);  /* set's 15.75KHz SVGA mode */
	void (*resetSVGA15KHzmode)(void);  /* clears up 15.75KHz SVGA mode */
} SVGA15KHZDRIVER;

/* find the driver for the card */
int getSVGA15KHzdriver(SVGA15KHZDRIVER **driver15KHz);

/* our 'generic' SVGA 15.75KHz functions */
int genericsvga(void);
int widthgeneric15KHz(int width);
int setgeneric15KHz(int vdouble, int width, int height);
void resetgeneric15KHz(void);

/* functions for allowing interlaced display to update both odd/even fields */
/* before app continues */
void setinterlaceflag(int interlaced);
void interlace_sync(void);
int sup_15Khz_res(int width,int height);

void vsync(void);
