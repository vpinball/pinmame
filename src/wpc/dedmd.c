#include "driver.h"
#include "core.h"
#include "de.h"
#include "dedmd.h"

extern int crtc6845_start_addr;

/*-----------------------------*/
/*Data East 192x64 DMD Handling*/
/*-----------------------------*/
void de_dmd192x64_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  UINT8 *RAM  = memory_region(DE_MEMREG_DCPU1) + 0x800000 + (crtc6845_start_addr&0x400)*4;
  UINT8 *RAM2 = memory_region(DE_MEMREG_DCPU1) + 0x800800 + (crtc6845_start_addr&0x400)*4;
  tDMDDot dotCol;
  int ii,jj,kk;

  //  logerror("crtc start address = %x\n",crtc6845_start_addr);
  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (kk = 0, ii = 1; ii <= 64; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (192/16); jj++) {
      UINT8 intens1 = 2*(RAM[kk+1] & 0x55) + (RAM2[kk+1] & 0x55);
      UINT8 intens2 =   (RAM[kk+1] & 0xaa) + (RAM2[kk+1] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      intens1 = 2*(RAM[kk] & 0x55) + (RAM2[kk] & 0x55);
      intens2 =   (RAM[kk] & 0xaa) + (RAM2[kk] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      kk += 2;
    }
    *line = 0;
  }
  dmd_draw(bitmap, dotCol, core_gameData->lcdLayout);
  drawStatus(bitmap,fullRefresh);
}


/*------------------------------------------*/
/*Data East, Sega, Stern 128x32 DMD Handling*/
/*------------------------------------------*/
static core_tLCDLayout de_128x32DMD[] = {
	{0,0,32,128,CORE_DMD}, {0}
};

void de_dmd128x32_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  UINT8 *RAM  = memory_region(DE_MEMREG_DCPU1) + 0x2000 + (crtc6845_start_addr&0x0100)*4;
  UINT8 *RAM2 = memory_region(DE_MEMREG_DCPU1) + 0x2200 + (crtc6845_start_addr&0x0100)*4;
  tDMDDot dotCol;
  int ii,jj,kk;
//  logerror("crtc start address = %x\n",crtc6845_start_addr);

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (kk = 0, ii = 1; ii <= 32; ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < (128/8); jj++) {
      UINT8 intens1 = 2*(RAM[kk] & 0x55) + (RAM2[kk] & 0x55);
      UINT8 intens2 =   (RAM[kk] & 0xaa) + (RAM2[kk] & 0xaa)/2;
      *line++ = (intens2>>6) & 0x03;
      *line++ = (intens1>>6) & 0x03;
      *line++ = (intens2>>4) & 0x03;
      *line++ = (intens1>>4) & 0x03;
      *line++ = (intens2>>2) & 0x03;
      *line++ = (intens1>>2) & 0x03;
      *line++ = (intens2)    & 0x03;
      *line++ = (intens1)    & 0x03;
      kk += 1;
    }
    *line = 0;
  }
  dmd_draw(bitmap, dotCol, de_128x32DMD);
  drawStatus(bitmap,fullRefresh);
}

/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
extern int de_dmd128x16[16][128];

void de_dmd128x16_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  int cols=128;
  int rows=16;
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  //UINT8 dotCol[34][129];
  int ii,jj;//,kk;

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (ii = 0; ii < rows; ii++) {
	  BMTYPE *line = *lines++;
	  for (jj = 0; jj < cols; jj++) {
		  if(de_dmd128x16[ii][jj])
			  *line++ = (de_dmd128x16[ii][jj]>1) ? CORE_COLOR(DMD_DOTON) : CORE_COLOR(DMD_DOT66);
		  else
			*line++ = CORE_COLOR(DMD_DOTOFF);

		line++;
	  }
	  lines++;
  }

  osd_mark_dirty(0,0,cols*coreGlobals_dmd.DMDsize,rows*coreGlobals_dmd.DMDsize);

  drawStatus(bitmap,fullRefresh);
}

/*DIFFERENT ATTEMPT AT DE 128x16 HANDLING*/
/*------------------------------*/
/*Data East 128x16 DMD Handling*/
/*------------------------------*/
static int offset=0;
void de2_dmd128x16_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  UINT8 *RAM;
  UINT8 *RAM2;
  BMTYPE **lines = (BMTYPE **)bitmap->line;
  int cols=128;
  int rows=16;
  int ii,jj,kk;
  int anydata = 0;
#if 0
  char temp[50];

  sprintf(temp,"offset=%4x",offset);
  core_textOutf(50,20,1,temp);

  if(keyboard_pressed_memory_repeat(KEYCODE_A,2))
	  offset+=0x100;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
	  offset-=0x100;
#endif
  RAM = memory_region(DE_MEMREG_DCPU1)+0x8000+offset;
  RAM2 = memory_region(DE_MEMREG_DCPU1)+0x8100+offset;

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  /* See if ANY data has been written to DMD region #2 0x8100-0x8200*/
  for (ii = 0; ii < rows; ii++)
	  for (jj = 0; jj < (cols/8); jj++)
		  anydata += RAM2[(ii*(cols/8))+jj];

  for (ii = 0; ii < rows; ii++) {
	  BMTYPE *line = *lines++;
	  for (jj = 0; jj < (cols/8); jj++) {
  		  int data  = RAM[(ii*(cols/8))+jj];
		  int data2 = RAM2[(ii*(cols/8))+jj];
		  for(kk = 7; kk >= 0; kk--) {
		  	//int ndata = 2*((data>>kk)&0x01) +
			//		      ((data2>>kk)&0x01);
			int ndata = ((data>>kk)&0x01) +
						((data2>>kk)&0x01);
			if(ndata>0)
				/* If Data has been written to both DMD segments..*/
				if(anydata>0)
					if(ndata==1)
						*line++ = CORE_COLOR(DMD_DOT33);
					else
						*line++ = CORE_COLOR(DMD_DOTON);
				else
					*line++ = CORE_COLOR(DMD_DOTON);
			else
				*line++ = CORE_COLOR(DMD_DOTOFF);
			line++;
		}
	  }
	  lines++;
  }

osd_mark_dirty(0,0,cols*coreGlobals_dmd.DMDsize,rows*coreGlobals_dmd.DMDsize);
  drawStatus(bitmap,fullRefresh);
}
