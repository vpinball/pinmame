/* for Mach64 and later ATI cards with internal clocks */

/* Tested on: */
/* 3D Rage Pro II+, 3D Rage Charger and Xpert@Play */

/* CONFIG_CHIP_ID register constants */
#define CFG_CHIP_TYPE		0x0000FFFF
#define CFG_CHIP_CLASS		0x00FF0000
#define CFG_CHIP_REV		0xFF000000
#define CFG_CHIP_VERSION	0x07000000
#define CFG_CHIP_FOUNDRY	0x38000000
#define CFG_CHIP_REVISION	0xC0000000

/* Chip IDs read from CONFIG_CHIP_ID */
#define MACH64_GX_ID	0xD7
#define MACH64_CX_ID	0x57
#define MACH64_CT_ID	0x4354
#define MACH64_ET_ID	0x4554
#define MACH64_VT_ID	0x5654
#define MACH64_VU_ID	0x5655
#define MACH64_GT_ID	0x4754
#define MACH64_GU_ID	0x4755
#define MACH64_GP_ID	0x4750
#define MACH64_XP_ID	0x4742
#define MACH64_XP98_ID	0x4c42

/* Mach64 chip types */
#define MACH64_UNKNOWN	0
#define MACH64_GX		1
#define MACH64_CX		2
#define MACH64_CT		3
#define MACH64_ET		4
#define MACH64_VT		5
#define MACH64_GT		6

/* ATI Bios */
#define	BIOS_DATA_BASE	0xC0000
#define	BIOS_DATA_SIZE	0x8000

#define BUS_FIFO_ERR_ACK	0x00200000
#define BUS_HOST_ERR_ACK	0x00800000

/* Position of flags in the General Control register */
#define	CRTC_Enable_Doubling			1
#define	CRTC_Enable_Interlace			(1 << 1)
#define	CRTC_FLAG_lock_regs				(1 << 22)
#define	CRTC_FLAG_enable_crtc			(1 << 25)
#define	CRTC_FLAG_extended_display_mode	(1 << 24)

/* Sync flags */
#define CRTC_H_SYNC_NEG	0x00200000
#define CRTC_V_SYNC_NEG	0x00200000

/* Flags for the General Test register */
#define GEN_OVR_OUTPUT_EN	0x20
#define HWCURSOR_ENABLE		0x80
#define GUI_ENGINE_ENABLE	0x100
#define BLOCK_WRITE_ENABLE	0x200

/* PLL registers */
#define PLL_WR_EN		0x02
#define PLL_MACRO_CNTL	0x01
#define PLL_REF_DIV		0x02
#define PLL_GEN_CNTL	0x03
#define MCLK_FB_DIV		0x04
#define PLL_VCLK_CNTL	0x05
#define VCLK_POST_DIV	0x06
#define VCLK0_FB_DIV	0x07
#define VCLK1_FB_DIV	0x08
#define VCLK2_FB_DIV	0x09
#define VCLK3_FB_DIV	0x0A
#define PLL_XCLK_CNTL	0x0B
#define PLL_TEST_CTRL	0x0E
#define PLL_TEST_COUNT	0x0F

/* MEM_CNTL register constants */
#define MEM_SIZE_ALIAS		0x00000007
#define MEM_SIZE_512K		0x00000000
#define MEM_SIZE_1M			0x00000001
#define MEM_SIZE_2M			0x00000002
#define MEM_SIZE_4M			0x00000003
#define MEM_SIZE_6M			0x00000004
#define MEM_SIZE_8M			0x00000005
#define MEM_SIZE_ALIAS_GTB	0x0000000F
#define MEM_SIZE_2M_GTB		0x00000003
#define MEM_SIZE_4M_GTB		0x00000007
#define MEM_SIZE_6M_GTB		0x00000009
#define MEM_SIZE_8M_GTB		0x0000000B
#define MEM_BNDRY			0x00030000
#define MEM_BNDRY_0K		0x00000000
#define MEM_BNDRY_256K		0x00010000
#define MEM_BNDRY_512K		0x00020000
#define MEM_BNDRY_1M		0x00030000
#define MEM_BNDRY_EN		0x00040000

/* DSP_CONFIG register constants */
#define DSP_XCLKS_PER_QW	0x00003fff
#define DSP_LOOP_LATENCY	0x000f0000
#define DSP_PRECISION		0x00700000

/* DSP_ON_OFF register constants */
#define DSP_OFF	0x000007ff
#define DSP_ON	0x07ff0000

/* CONFIG_STAT0 register constants (CT, ET, VT) */
#define CFG_MEM_TYPE_xT	0x00000007

/* Memory types for CT, ET, VT, GT */
#define DRAM		1
#define EDO_DRAM	2
#define PSEUDO_EDO	3
#define SDRAM		4
#define SGRAM		5

/* type of clock */
#define CLK_ATI18818_0	0
#define CLK_ATI18818_1	1
#define CLK_STG1703		2
#define CLK_CH8398		3
#define CLK_INTERNAL	4
#define CLK_ATT20C408	5
#define CLK_IBMRGB514	6

/* bit in clock register for strobing */
#define CLOCK_STROBE	0x40

/* our 4 driver functions */
int detectati(void);
int widthati15KHz(int width);
int setati15KHz(int vdouble,int width,int height);
void resetati15KHz(void);

/* internal functions */
int calc_mach64_scanline(int *nHzTotal,int *nHzDispTotal,int *nHzSyncOffset,int *N,int *P,int *externaldiv,int *nActualMHz);
int calc_mach64_height(int vdouble,int *nOffSet,int *interlace,int *dispdouble,int *vTotal,int *nVSyncOffset,int *nlastline);
int calc_mach64_clock(int nTargetMHz,int *N,int *P,int *externaldiv);
int get_mach64_port(int io_type, int io_base, int io_sel, int mm_sel);
/* DSP */
int setmach64DSP(int nAdd);
void resetmach64DSP(void);

