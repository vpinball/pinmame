#include "mamalleg.h"
#include "driver.h"
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include <math.h>
#include "TwkUser.h"
#include "gen15khz.h"
#include "ati15khz.h"



extern int center_x;
extern int center_y;
extern int use_triplebuf;

/* Save values */
static int DSPSet;
static int SaveDSPOnOff;
static int SaveDSPConfig;

/* Currently -
	only ATI cards based on the Mach64 chipset which have an internal clock/DAC
	are supported (see note below)*/

/* Tested on a 3D Rage Pro II+ , 3D Rage Charger and Xpert@Play */

/* NOTE: There are 2 versions of this driver, */
/* one reprograms the clock to generate the mode (original driver) */
/* the other doubles the scan line to generate the mode */

/* The first version's display is smaller (and therefore faster), but only works on */
/* cards which have internal clocks */
/* The second should work on all Mach64 based cards */

/* Comment out the following line to use the no-clock programming driver */
#define ATI_PROGRAM_CLOCK


/* Type of clock */
char *mach64ClockTypeTable[] =
{
	"ATI18818-0",
	"ATI18818-1/ICS2595",
	"STG1703",
	"CH8398",
	"Internal",
	"AT&T20C408",
	"IBM-RGB514",
};

/* the clock info we need for integrated DACs */
static int RefFreq;     /* Reference frequency for clock */
static int RefDivider;  /* Reference divider for clock */
static int MinFreq;     /* Minimum frequency of clock */
static int MaxFreq;     /* Maximum frequency of clock */
static int VRAMMemClk;  /* Speed of video RAM clock */
static int CXClk;       /* ID of clock we're going to program */
static int MemSize;     /* Memory on the card */
static int MemType;     /* Type of memory on the card */
static int ChipID;      /* ID of chip on card */
static int ChipRev;     /* revision of chip on card */
static int ChipType;    /* what we decide in our wisdom the chip is */



/* the MACH64 registers we're interested in */
static int _mach64_clock_reg;
static int _mach64_gen_cntrl;
static int _mach64_off_pitch;
static int _mach64_dac_cntl;
static int _mach64_config_stat0;
static int _mach64_crtc_h_total;
static int _mach64_crtc_h_sync;
static int _mach64_crtc_v_total;
static int _mach64_crtc_v_sync;
static int _mach64_crtc_v_line;
static int _mach64_over_left_right;
static int _mach64_over_top_bott;
static int _mach64_mem_cntl;
static int _mach64_chip_id;
static int _mach64_dsp_config;
static int _mach64_dsp_on_off;


/*see if we've really got an ATI card with a Mach64 chipset  */
int detectati(void)
{
	__dpmi_regs r;
	int scratch_reg;
	unsigned long old;
	char bios_data[BIOS_DATA_SIZE];
	unsigned short *sbios_data = (unsigned short *)bios_data;
	int ROM_Table_Offset;
	int Freq_Table_Ptr;
	int Clock_Type;



	/* query mach64 BIOS for the I/O base address */
	r.x.ax = 0xA012;
	r.x.cx = 0;
	__dpmi_int (0x10, &r);

	if (r.h.ah)
		return 0;

	/* test scratch register to confirm we have a mach64 */
	scratch_reg = get_mach64_port (r.x.cx, r.x.dx, 0x11, 0x21);

	old = inportl(scratch_reg);

	outportl (scratch_reg, 0x55555555);
	if (inportl (scratch_reg) != 0x55555555)
	{
		outportl (scratch_reg, old);
		logerror("15.75KHz: Not Mach64 Chipset\n");
		return 0;
	}

	outportl (scratch_reg, 0xAAAAAAAA);
	if (inportl (scratch_reg) != 0xAAAAAAAA)
	{
		outportl (scratch_reg, old);
		logerror("15.75KHz: Not Mach64 Chipset\n");
		return 0;
	}

	outportl (scratch_reg, old);

	/* get info from the ATI BIOS */
	dosmemget (BIOS_DATA_BASE, BIOS_DATA_SIZE, bios_data);
	ROM_Table_Offset = sbios_data[0x48 >> 1];
	Freq_Table_Ptr = sbios_data[(ROM_Table_Offset >> 1) + 8];
	Clock_Type = bios_data[Freq_Table_Ptr];
	CXClk = bios_data[Freq_Table_Ptr + 6];
	RefFreq = sbios_data[(Freq_Table_Ptr >> 1) + 4];
	RefDivider = sbios_data[(Freq_Table_Ptr >> 1) + 5];
	MinFreq = sbios_data[(Freq_Table_Ptr >> 1) + 1];
	MaxFreq = sbios_data[(Freq_Table_Ptr >> 1) + 2];
	VRAMMemClk = sbios_data[(Freq_Table_Ptr >> 1) + 9];
	logerror("15.75KHz: type of MACH64 clk: %s (%d)\n", mach64ClockTypeTable[Clock_Type], Clock_Type);
	logerror("15.75KHz: MACH64 ref Freq:%d\n", RefFreq);
	logerror("15.75KHz: MACH64 ref Div:%d\n", RefDivider);
	logerror("15.75KHz: MACH64 min Freq:%d\n", MinFreq);
	logerror("15.75KHz: MACH64 max Freq:%d\n", MaxFreq);
	logerror("15.75KHz: MACH64 Mem Clk %d\n", VRAMMemClk);


/*Get some useful registers while we're here  */

/*
M+000h/02ECh D(R/W):  Crtc_H_Total_Disp
bit   0-7  Crtc_H_Total. Horizontal Total in character clocks (8 pixel units)
    16-23  Crtc_H_Disp. Horizontal Display End in character clocks.
*/

	_mach64_crtc_h_total = get_mach64_port (r.x.cx, r.x.dx, 0x00, 0x00);
/*
M+004h/06ECh D(R/W):  Crtc_H_Sync_Strt_Wid
bit   0-7  Crtc_H_Sync_Strt. Horizontal Sync Start in character clocks (8
           pixel units)
     8-10  Crtc_H_Sync_Dly. Horizontal Sync Start delay in pixels
    16-20  Crtc_H_Sync_Wid. Horizontal Sync Width in character clocks
       21  Crtc_H_Sync_Pol. Horizontal Sync Polarity
*/
	_mach64_crtc_h_sync = get_mach64_port (r.x.cx, r.x.dx, 0x01, 0x01);
/*
M+008h/0AECh D(R/W):  Crtc_V_Total_Disp
bit  0-10  Crtc_V_Total. Vertical Total
    16-26  Crtc_V_Disp. Vertical Displayed End
*/
	_mach64_crtc_v_total = get_mach64_port (r.x.cx, r.x.dx, 0x02, 0x02);
/*
M+00Ch/0EECh D(R/W):  Crtc_V_Sync_Strt_Wid
bit  0-10  Crtc_V_Sync_Strt. Vertical Sync Start
    16-20  Crtc_V_Sync_Wid. Vertical Sync Width
       21  Crtc_V_Sync_Pol. Vertical Sync Polarity
*/
	_mach64_crtc_v_sync = get_mach64_port (r.x.cx, r.x.dx, 0x03, 0x03);

/*
M+010h/12ECh D(R/W):  Crtc_Vline_Crnt_Vline
bit  0-10  The line at which Vertical Line interrupt is triggered
    16-26  (R) Crtc_Crnt_Vline. The line currently being displayed
*/
	_mach64_crtc_v_line = get_mach64_port (r.x.cx, r.x.dx, 0x04, 0x04);

/*
M+014h/16ECh D(R/W):  Crtc_Off_Pitch
bit  0-19  Crtc_Offset. Display Start Address in units of 8 bytes.
    22-31  Crtc_Pitch. Display pitch in units of 8 pixels
*/
	_mach64_off_pitch = get_mach64_port (r.x.cx, r.x.dx, 0x05, 0x05);

/*
M+01Ch/1EECh D(R/W):  Crtc_Gen_Cntl
bit     0  Crtc_Dbl_Scan_En. Enables double scan
        1  Crtc_Interlace_En. Enables interlace.
        2  Crtc_Hsync_Dis. Disables Horizontal Sync output
        3  Crtc_Vsync_Dis. Disables Vertical Sync output
        4  Crtc_Csync_En. Enable composite sync on Horizontal Sync output
        5  Crtc_Pic_By_2_En. CRTC advances 2 pixels per pixel clock
     8-10  Crtc_Pix_Width. Displayed bits/pixel: 1: 4bpp, 2: 8bpp, 3: 15bpp
            (5:5:5), 4: 16bpp (5:6:5), 5: 24bpp(undoc), 6: 32bpp
       11  Crtc_Byte_Pix_Order. Pixel order within each byte (4bpp).
            0: High nibble displayed first, 1: low nibble displayed first
    16-19  Crtc_Fifo_Lwm. Low Water Mark of the 16entry deep display FIFO.
           Only used in DRAM configurations. The minimum number of entries
           remaining in the FIFO before the CRTC starts refilling. Ideally
           should be set to the lowest number that gives a stable display.
       24  Crtc_Ext_Disp_En. 1:Extended display mode , 0:VGA display mode
       25  Crtc_En. Enables CRTC if set, resets if clear

*/

	_mach64_gen_cntrl = get_mach64_port (r.x.cx, r.x.dx, 0x07, 0x07);

/* DSP registers  */
/* No reliable information available about these....  */
	_mach64_dsp_config = get_mach64_port (r.x.cx, r.x.dx, 0x08, 0x08);
	_mach64_dsp_on_off = get_mach64_port (r.x.cx, r.x.dx, 0x09, 0x09);


/*
M+090h/4AECh D(R/W):  Clock_Cntl
bit   0-3  Clock_Sel. Clock select bit 0-3. Output to the clock chip
      4-5  Clock_Div. Internal clock divider. 0: no div, 1: /2, 2: /4
        6  (W) Clock_Strobe. Connected to the strobe or clk input on
            programmable clock chips
        7  Clock_Serial_Data. Data I/O for programmable clock chips
*/
	_mach64_clock_reg = get_mach64_port (r.x.cx, r.x.dx, 0x12, 0x24);

/*
M+0C4h/62ECh D(R/W):  Dac_Cntl
bit   0-1  Dac_Ext_Sel. Connected to the RS2 and RS3 inputs on the DAC.
        8  Dac_8bit_En. Enables 8bit DAC mode (256colors of 16M) if set
     9-10  Dac_Pix_Dly. Setup and hold time on pixel data. 0: None,
            1: 2ns - 4ns delay, 2: 4ns - 8ns delay
    11-12  Dac_Blank_Adj. Blank Delay in number of pixel clock periods.
            0: None, 1: 1 pixel clock, 2: 2 pixel clocks
       13  Dac_VGA_Adr_En. When bit 24 of Crtc_Gen_Cntl (M+01Ch/1EECh) is set,
           this bit enables access to the VGA DAC I/O addresses (3C6h-3C9h).
    16-18  Dac_Type. The DAC type - initialised from configuration straps on
           power-up. See Config_Stat0 (M+0E4h/72ECh) bits 9-11 for details
*/
	_mach64_dac_cntl = get_mach64_port (r.x.cx, r.x.dx, 0x18, 0x31);
/*
M+0E4h/72ECh D(R):  Config_Stat0
bit   0-2  Cfg_Bus_Type. Host Bus type. 0: ISA, 1: EISA, 6: VLB, 7: PCI
      3-5  Cfg_Mem_Type. Memory Type. 0: DRAM (256Kx4), 1: VRAM (256Kx4, x8,
            x16), 2: VRAM (256Kx16 short shift reg), 3: DRAM (256Kx16),
            4: Graphics DRAM (256Kx16), 5: Enh VRAM (256Kx4, x8, x16), 6: Enh
            VRAM (256Kx16 short shift reg)
        6  Cfg_Dual_CAS_En. Dual CAS support enabled if set
      7-8  Cfg_Local_Bus_Option. Local Bus Option.
             1: Local option 1, 2: Local option 2, 3: Local option 3
     9-11  Cfg_Init_DAC_Type. DAC type. 2: ATI68875/TI 34075, 3: Bt476/Bt478,
             4: Bt481, 5: ATI68860/ATI68880, 6: STG1700, 7: SC15021
    12-14  Cfg_Init_Card_ID. Card ID. 0-6: Card ID 0-6, 7: Disable Card ID
       15  Cfg_Tri_Buf_Dis. Tri-stating of output buffers during reset
           disabled if set
    16-21  Cfg_Ext_ROM_Addr. Extended Mode ROM Base Address. Bits 12-17 of the
           ROM base address, 0: C0000h, 1: C1000h ... 3Fh: FE000h
       22  Cfg_ROM_Dis. Disables ROM if set
       23  Cfg_VGA_Enm. Enables VGA Controller
       24  Cfg_Local_Bus_Cfg. 0: Local Bus configuration 2, 1: configuration 1
       25  Cfg_Chip_En. Enables chip if set
       26  Cfg_Local_Read_Dly_Dis. If clear delays read cycle termination by 1
           bus clock, no delay if set
       27  Cfg_ROM_Option. ROM Address. 0: E0000h, 1: C0000h
       28  Cfg_Bus_option. EISA bus: Enables POS registers if set, disables
           POS registers and enables chip if clear.
           VESA Local Bus: Enables decode of I/O address 102h if clear,
           disables if set
       29  Cfg_Local_DAC_Wr_En. Enables local bus DAC writes if set
       30  Cfg_VLB_Rdy_Dis. Disables VESA local bus compliant RDY if set
       31  Cfg_Ap_4Gbyte_Dis. Disables 4GB Aperture Addressing if set
*/
	_mach64_config_stat0 = get_mach64_port (r.x.cx, r.x.dx, 0x1c, 0x39);
/*
M+044h/26ECh D(R/W):  Ovr_Wid_Left_Right
bit   0-3  Ovr_Wid_Left. Left overscan width in character clocks
    16-19  Ovr_Wid_Right. Right overscan width in character clocks
*/
	_mach64_over_left_right = get_mach64_port (r.x.cx, r.x.dx, 0x09, 0x11);
/*
M+048h/2AECh D(R/W):  Ovr_Wid_Top_Bottom
bit   0-7  Ovr_Wid_Top. Top overscan width in lines
    16-23  Ovr_Wid_Bottom. Bottom overscan width in lines
*/
	_mach64_over_top_bott = get_mach64_port (r.x.cx, r.x.dx, 0x0a, 0x12);
/*
M+0B0h/52ECh D(R/W):  Mem_Cntl
bit   0-2  Mem_Size. Video Memory Size. 0: 512K, 1: 1MB, 2: 2MB, 3: 4MB,
            4: 6MB, 5: 8MB
        4  Mem_Rd_Latch_En. Enables latching on RAM port data
        5  Mem_Rd_Latch_Dly. Delays latching of RAM port data by 1/2 memory
           clock period
        6  Mem_Sd_Latch_En. Enables latching of data on serial port data
        7  Mem_Sd_Latch_Dly. Delays latching of serial port data by 1/2 memory
           clock period
        8  Mem_Fill_Pls. One memory clock period set for width of data latch
           pulse
     9-10  Mem_Cyc_Lnth. memory cycle length for non-paged access:
             0: 5 mem clock periods, 1: 6 mem clks, 2: 7 mem clks
    16-17  Mem_Bndry. VGA/Mach Memory boundary. If the memory boundary is
           enabled (bit 18 is set) defines the amount of memory reserved for
           the VGA.  0: 0K, 1: 256K, 2: 512K, 3: 1M
       18  Mem_Bndry_En. If set the video memory is divided between the VGA
           engine and the Mach engine, with the low part reserved for the VGA
           engine, if clear they share the video memory
*/
	_mach64_mem_cntl = get_mach64_port (r.x.cx, r.x.dx, 0x14, 0x2c);
/*
M+0E0h/6EECh D(R):  Config_Chip_ID.
bit  0-15  Cfg_Chip_Type. Product Type Code. 0D7h for the 88800GX,
             57h for the 88800CX (guess)
    16-23  Cfg_Chip_Class. Class code
    24-31  Cfg_Chip_Rev. Revision code
*/
	_mach64_chip_id = get_mach64_port (r.x.cx, r.x.dx, 0x1b, 0x38);
/* get the chip ID  */
	old = inportl(_mach64_chip_id);
	ChipID = (int)(old & CFG_CHIP_TYPE);
	ChipRev = (int)((old & CFG_CHIP_REV) >> 24);

	logerror("15.75KHz: Chip ID :%d\n", ChipID);
	logerror("15.75KHz: Chip Rev :%d\n", ChipRev);

	switch (ChipID)
	{
		case MACH64_GX_ID:
			ChipType = MACH64_GX;
			break;
		case MACH64_CX_ID:
			ChipType = MACH64_CX;
			break;
		case MACH64_CT_ID:
			ChipType = MACH64_CT;
			break;
		case MACH64_ET_ID:
			ChipType = MACH64_ET;
			break;
		case MACH64_VT_ID:
		case MACH64_VU_ID:
			ChipType = MACH64_VT;
			break;
		case MACH64_GT_ID:
		case MACH64_GU_ID:
		case MACH64_GP_ID:
		case MACH64_XP_ID:
		case MACH64_XP98_ID:
			ChipType = MACH64_GT;
			break;
		default:
			ChipType=MACH64_UNKNOWN;
	}

	logerror("15.75KHz: Chip Type :%d\n", ChipType);
/* and the memory on the card  */
	old = inportl (_mach64_mem_cntl);
	switch (old & MEM_SIZE_ALIAS_GTB)
	{
		case MEM_SIZE_512K:
		case MEM_SIZE_1M:
			MemSize = (int)old & MEM_SIZE_ALIAS_GTB;
			break;
		case MEM_SIZE_2M_GTB:
			MemSize= MEM_SIZE_2M;
			break;
		case MEM_SIZE_4M_GTB:
			MemSize= MEM_SIZE_4M;
			break;
		case MEM_SIZE_6M_GTB:
			MemSize= MEM_SIZE_6M;
			break;
		case MEM_SIZE_8M_GTB:
			MemSize= MEM_SIZE_8M;
			break;
		default:
			MemSize=MEM_SIZE_1M;
	}
	logerror("15.75KHz: Video Memory %d\n", MemSize);
/* and the type of memory  */
	old = inportl(_mach64_config_stat0);
	MemType = old & CFG_MEM_TYPE_xT;
	logerror("15.75KHz: Video Memory  Type %d\n",MemType);

#ifdef ATI_PROGRAM_CLOCK
/* only bail out here if the clock's wrong  */
/* so we can collect as much info as possible about the card  */
/* - just in case I ever feel like adding RAMDAC support  */
	if (Clock_Type != CLK_INTERNAL)
	{
		logerror("15.75KHz: Clock type not supported, only internal clocks implemented\n");
		return 0;
	}
#endif

	DSPSet=0;

	logerror("15.75KHz: Found Mach64 based card with internal clock\n");
	return 1;
}

int widthati15KHz(int width)
{
#ifdef ATI_PROGRAM_CLOCK
/* standard width */
	return width;
#else
/* double width scan line */
/* turn off triple buffering */
	use_triplebuf = 0;
/* and return the actual width of the mode */
	return width << 1;
#endif
}

int setati15KHz(int vdouble,int width, int height)
{
	int n,extdiv,P;
	int nActualMHz;
	int nHzTotal;
	int nHzDisplay;
	int nHzSyncOffset;
	int nVSyncOffset;
	int nOffSet;
	int interlace;
	int dispdouble;
	long int temp;
	long int gen;
#ifdef ATI_PROGRAM_CLOCK
  	int temp1,temp2,temp3;
#endif
	int	lastvisline;
	int	vTotal;

	nHzDisplay = inportb (_mach64_crtc_h_total+2);


	if (!sup_15Khz_res (width, height))
		return 0;
/* last visible line */
	lastvisline = height >> 1;

/* get clock and horizontal/vertical totals  */
	if (!calc_mach64_height (vdouble, &nOffSet, &interlace, &dispdouble, &vTotal, &nVSyncOffset, &lastvisline))
		return 0;

	if (!calc_mach64_scanline (&nHzTotal, &nHzDisplay, &nHzSyncOffset, &n, &P, &extdiv, &nActualMHz))
		return 0;
	logerror("15.75KHz: Offset:%d ,Interlace:%d ,Dis. Double:%d\n", nOffSet, interlace, dispdouble);

	gen = inportl(_mach64_gen_cntrl);


/* unlock the regs and disable the CRTC */
	outportw (_mach64_gen_cntrl, gen & ~(CRTC_FLAG_lock_regs|CRTC_FLAG_enable_crtc));

#ifdef ATI_PROGRAM_CLOCK
/* we need to program the DSP - otherwise we'll get nasty artifacts all over the screen */
/* when we change the clock speed */
	if (((ChipType == MACH64_VT || ChipType == MACH64_GT)&&(ChipRev & 0x07)))
	{
		logerror("15.75KHz: Programming the DSP\n");
		if (!setmach64DSP(0))
		{
			outportl (_mach64_gen_cntrl, gen);
			return 0;
		}
	}
	else
	{
		logerror("15.75KHz: Decided NOT to program the DSP\n");
	}

/* now we can program the clock */
	outportb (_mach64_clock_reg + 1, PLL_VCLK_CNTL << 2);
	temp1 = inportb (_mach64_clock_reg + 2);
	outportb (_mach64_clock_reg + 1, (PLL_VCLK_CNTL  << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, temp1 | 4);


	outportb (_mach64_clock_reg + 1, (VCLK_POST_DIV << 2));
	temp2 = inportb (_mach64_clock_reg + 2);


	outportb (_mach64_clock_reg + 1, ((VCLK0_FB_DIV + CXClk) << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg +2,n);

	outportb (_mach64_clock_reg + 1, (VCLK_POST_DIV << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, (temp2 & ~(0x03 << (2 * CXClk))) | (P << (2 * CXClk)));

	outportb (_mach64_clock_reg + 1, PLL_XCLK_CNTL << 2);
	temp3 = inportb (_mach64_clock_reg + 2);
	outportb (_mach64_clock_reg + 1, (PLL_XCLK_CNTL << 2) | PLL_WR_EN);

	if (extdiv)
		outportb (_mach64_clock_reg + 2, temp3 | (1 << (CXClk + 4)));
	else
		outportb (_mach64_clock_reg + 2, temp3 & ~(1 << (CXClk + 4)));



	outportb (_mach64_clock_reg + 1, (PLL_VCLK_CNTL << 2) | PLL_WR_EN);
	outportb (_mach64_clock_reg + 2, temp1&~0x04);

/* reset the DAC */
	inportb (_mach64_dac_cntl);
#endif

	logerror("15.75KHz: H total %d, H display %d, H sync offset %d\n",nHzTotal,nHzDisplay,nHzSyncOffset);
	logerror("15.75KHz: V total %d, V display %d, V sync offset %d\n",vTotal,lastvisline,nVSyncOffset);

/* now setup the CRTC timings */

	outportb (_mach64_crtc_h_total, nHzTotal);  /* h total */
	outportb (_mach64_crtc_h_total + 2, nHzDisplay); /* h display width */
	outportb (_mach64_crtc_h_sync, nHzDisplay+nHzSyncOffset + center_x);   /* h sync start */
	outportb (_mach64_crtc_h_sync + 1, 0);  /* h sync delay */
#ifdef ATI_PROGRAM_CLOCK
	outportb (_mach64_crtc_h_sync + 2, 12); /* h sync width */
#else
	outportb (_mach64_crtc_h_sync + 2, 20); /* h sync width */
#endif
	outportw (_mach64_crtc_v_total, (vTotal<<interlace));   /* v total */
	outportw (_mach64_crtc_v_total + 2, (lastvisline<<interlace)-1);  /* v display height */
	outportw (_mach64_crtc_v_sync, ((lastvisline+nVSyncOffset)<<interlace) + center_y);    /* v sync start */
	outportb (_mach64_crtc_v_sync + 2, 3);          /* v sync width */

/* make sure sync is negative */
	temp = inportl(_mach64_crtc_h_sync);
	temp |= CRTC_H_SYNC_NEG;
	outportl (_mach64_crtc_h_sync,temp);

	temp = inportl(_mach64_crtc_v_sync);
	temp |= CRTC_V_SYNC_NEG;
  	outportl (_mach64_crtc_v_sync,temp);

/* clear any overscan */
	outportb (_mach64_over_left_right,0);
	outportb (_mach64_over_left_right+2,0);
	outportb (_mach64_over_top_bott,0);
	outportb (_mach64_over_top_bott+2,0);

/* set memory for each line */
	temp = inportl(_mach64_off_pitch);
	temp &= 0xfffff;
	outportl (_mach64_off_pitch,temp|(nOffSet<<22));

/* max out the FIFO */
	gen |= (15<<16);

/* turn on/off interlacing */
  	if (interlace)
		gen |= CRTC_Enable_Interlace;
  	else
		gen &=~ CRTC_Enable_Interlace;
/* set the interlace flag */
	setinterlaceflag (interlace);
/* turn off scanline doubling if we need to */
	if (dispdouble)
		gen &=~ CRTC_Enable_Doubling;

/* set the display going again */
	outportl (_mach64_gen_cntrl,gen);
/* finally select and strobe the clock */
	outportb (_mach64_clock_reg, CXClk | CLOCK_STROBE);
	return 1;
}

void resetati15KHz()
{
/* reset the DSP */
	resetmach64DSP();
/* it could be a bit risky resetting the clock + general registers if we're running on an arcade monitor */
/* so, I'll leave 'em for the moment */
}

int calc_mach64_height(int vdouble,int *nOffSet,int *interlace,int *dispdouble,int *vTotal,int *nVSyncOffset,int *nlastline)
{
	int nVtDispTotal;
	long int lport;
	long int temp;

/* get the current visible display total */
	nVtDispTotal = getVtEndDisplay();
/* assume it's hires and we're :-  */
/* going to interlace, not alter scanline doubling, and not change the offset */
	*interlace = 1;
	*dispdouble = 0;
	temp = inportl (_mach64_off_pitch);
	temp >>= 22;
	*nOffSet = temp;
	*vTotal = 0;


	switch (nVtDispTotal)
	{
		case 479:	/* 640x480 */
			*vTotal = 256 + tw640x480arc_v;
			*nVSyncOffset = 2;
			if (vdouble)
			{
				logerror("15.75KHz: Mode is using software doubling, disabling interlace and halfing y res\n");
				*interlace = 0;
/* only draw every other line */
				*nOffSet <<= 1;
			}
			else
			{
/* check if the mode was set up with internal scan line doubling */
				lport = inportl(_mach64_gen_cntrl);
				if (lport & CRTC_Enable_Doubling)
				{
					logerror("15.75KHz: Mode is using hardware doubling, disabling interlace, disabling hardware doubling\n");
					*interlace = 0;
					*dispdouble = 1;
				}
			}
			break;
		default:    /* unhandled resolution */
			logerror("15.75KHz: Unsupported SVGA 15.75KHz height (%d)\n", nVtDispTotal+1);
			return 0;

	}
	return 1;
}

int calc_mach64_scanline(int *nHzTotal,int *nHzDispTotal,int *nHzSyncOffset,int *N,int *P,int *externaldiv,int *nActualMHz)
{
/* set clock and horizontal Total based on Horizontal display width */
/* NOTE: Only 640x480 so far, */
/* 320x240 has a very low clock width and even with the slowest clock speed has a 'bad' aspect ratio */
/* doubled 640x480 looks a lot better and runs about as fast */
/* higher resolutions just result in more and more overscan */

	int nTargetHzTotal, nActualHzTotal;
	int nTargetMHz=0;
/* get the horizontal width */
	*nHzDispTotal = inportb (_mach64_crtc_h_total + 2);
	switch (*nHzDispTotal)
	{
		case 79: /* 640x480 */
			logerror("15.75KHz: 640x480 mode Attempting to use 14MHz Clock\n");
			nTargetMHz = 1400;
			nTargetHzTotal = tw640x480arc_h - 83;
			*nHzSyncOffset = 6;
			break;
		default: /* unhandled res, return error */
			logerror("15.75KHz: Unsupported SVGA 15.75KHz mode (%d chars)\n", *nHzDispTotal);

			return 0;
	}
#ifdef ATI_PROGRAM_CLOCK
/* calculate the clock */
	*nActualMHz = calc_mach64_clock (nTargetMHz, N, P, externaldiv);
/* adjust the horizontal total */
	nActualHzTotal = (int)(((float)nTargetHzTotal / (float)nTargetMHz) * (float)*nActualMHz);
	logerror("15.75KHz: tgt MHz:%d, act MHz:%d, tgt HzTot:%d, act HzTot:%d\n",
				nTargetMHz, *nActualMHz, nTargetHzTotal, nActualHzTotal);
#else
/* not programming the clock, so setup a double width scan line */
	*nActualMHz = 0;
	nActualHzTotal = tw640x480arc_h + 5;
	*nHzDispTotal = (*nHzDispTotal * 2) + 1;
	*nHzSyncOffset = 0;
	logerror("15.75KHz: HzTot:%d\n", nActualHzTotal);

#endif

	*nHzTotal = nActualHzTotal;
	return 1;
}

int calc_mach64_clock(int nTargetMHz,int *N,int *P,int *externaldiv)
{

	int postDiv;
	int nActualMHz;
	float Q;

/* assume the best */
	nActualMHz = nTargetMHz;
/* check clock is in range */
	if (nActualMHz < MinFreq)
		nActualMHz = MinFreq;

	if (nActualMHz > MaxFreq)
		nActualMHz = MaxFreq;
/* formula for clock is as follows */
/* Clock = ((2 * mach64RefFreq * N)/(mach64RefDivider * postDiv)) */


	Q = (nActualMHz * RefDivider)/(2.0 * RefFreq);
	logerror("15.75KHz: Q:%d\n", (int)Q);
	*externaldiv = 0;

	if (Q > 255)
	{
		logerror("15.75KHz: Q too big\n");
		Q = 255;
		*P = 0;
		postDiv = 1;
	}
	else if (Q > 127.5)
	{
		*P = 0;
		postDiv = 1;
	}
	else if (Q > 85)
	{
		*P = 1;
		postDiv = 2;
	}
	else if (Q > 63.75)
	{
		*P = 0;
		postDiv = 3;
		*externaldiv = 1;
	}
	else if (Q > 42.5)
	{
		*P = 2;
		postDiv = 4;
	}
	else if (Q > 31.875)
	{
		*P = 2;
		postDiv = 6;
		*externaldiv = 1;
	}
	else if (Q > 21.25)
	{
		*P = 3;
		postDiv = 8;
	}
	else if (Q >= 10.6666666667)
	{
		*P = 3;
		postDiv = 12;
		*externaldiv = 1;
	}
	else
	{
		*P = 3;
		postDiv = 12;
		*externaldiv = 1;
	}
	*N = (int)(Q * postDiv + 0.5);

	nActualMHz = ((2 * RefFreq * (*N)) / (RefDivider * postDiv));


	logerror("15.75KHz: MACH64 N val:%d\n", *N);
	logerror("15.75KHz: MACH64 Post Div val:%d\n", *P);
	logerror("15.75KHz: MACH64 external div:%d\n", *externaldiv);

	return nActualMHz;
}


int get_mach64_port(int io_type, int io_base, int io_sel, int mm_sel)
{
	if (io_type)
	{
		return (mm_sel << 2) + io_base;
	}
	else
	{
		if (!io_base)
			io_base = 0x2EC;
		return (io_sel << 10) + io_base;
	}
}



int setmach64DSP(int nAdd)
{
	unsigned short dsp_on, dsp_off, dsp_xclks_per_qw, dsp_prec, loop_latency;
	long portval;
	static int offset=0;

	offset+=nAdd;
/* okay, I've been unable to get any reliable information about the DSP */
/* so these are just values that work for this :- */
/* dot clock speed, colour depth and video clock speed */
/* found after a bit of hacking around */
/*- The memory stuff is probably real though */

/* get the colour depth */
	portval = inportb (_mach64_gen_cntrl+1) & 7;

	switch (portval)
	{
		case  2: /* 8 bit colour */
			switch (VRAMMemClk)
			{
				/* 100MHz -  for newer/faster cards */
				case 10000:
                    dsp_xclks_per_qw = 2239+offset;
					break;
				/* Standard video memory speed (usually 60MHz)*/
				default:
                    dsp_xclks_per_qw = 2189+offset;
			}
            logerror("DSP value %d (8bit)\n",dsp_xclks_per_qw);

			break;
		case  3: /* 16 bit colour */
		case  4: /* either 555 or 565 */
			switch (VRAMMemClk)
			{
				/* 100MHz -  for newer/faster cards */
				case 10000:
                    dsp_xclks_per_qw = 3655+offset;
					break;
				/* Standard video memory speed (usually 60MHz)*/
				default:
					dsp_xclks_per_qw = 3679+offset;
			}
            logerror("DSP value %d (16bit)\n",dsp_xclks_per_qw);

			break;
		default: /* any other colour depth */
			logerror("15.75KHz: Unsupported colour depth for ATI driver (%d)\n",(int)inportb (_mach64_gen_cntrl+1));
			return 0;
	}


	if (MemSize > MEM_SIZE_1M)
	{
		if (MemType >= SDRAM)
			loop_latency = 8;
		else
			loop_latency = 6;
	}
	else
	{
		if (MemType >= SDRAM)
			loop_latency = 9;
		else
			loop_latency = 8;
	}

/* our DSP values */
	dsp_on = 106;
	dsp_off = 206;
	dsp_prec = 5;

	logerror("15.75KHz: DSP on:%d DSP off:%d DSP clks :%d  DSP prec:%d  latency :%d\n", (int)dsp_on, (int)dsp_off,
				(int)dsp_xclks_per_qw, (int)dsp_prec, (int)loop_latency);
/* save whats there */
	SaveDSPOnOff = inportw (_mach64_dsp_on_off);
	SaveDSPConfig = inportw (_mach64_dsp_on_off);

/* write everything out */
	outportw (_mach64_dsp_on_off, ((dsp_on << 16) & DSP_ON) | (dsp_off & DSP_OFF));
	outportw (_mach64_dsp_config, ((dsp_prec << 20) & DSP_PRECISION) | ((loop_latency << 16) & DSP_LOOP_LATENCY)
				| (dsp_xclks_per_qw & DSP_XCLKS_PER_QW));
	DSPSet = 1;

	return dsp_xclks_per_qw;
}


void resetmach64DSP()
{
	if(DSPSet)
	{
		outportw (_mach64_dsp_on_off, SaveDSPOnOff);
		outportw (_mach64_dsp_config, SaveDSPConfig);
	}
}


