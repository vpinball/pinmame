#include "driver.h"
#include "core.h"
#include "gts3.h"
#include "gts3dmd.h"

//#define DEBUGSWAP

extern UINT8 DMDFrames[GTS3DMD_FRAMES][0x200];
extern int crtc6845_start_addr;

#if 1
void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  //tDMDDot dotCol;
  int ii,jj,kk,ll;
  BMTYPE **lines = (BMTYPE **)bitmap->line;
#ifdef DEBUGSWAP
  char temp[250];
  sprintf(temp,"location=%04x   %04x",0x1000+(crtc6845_start_addr>>2), crtc6845_start_addr);
  core_textOutf(50,50,1,temp);
#endif

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  for (ii = 0; ii < 32; ii++) {
      BMTYPE *line = *lines++;
	  for (jj = 0; jj < 128/8; jj++) {
		  for(kk=7;kk>=0;kk--) {
			  int data = 0;
			  for(ll=0;ll<GTS3DMD_FRAMES;ll++) {
				data += ((DMDFrames[ll][(ii*128/8)+jj])>>kk)&1;
			  }
			  if(data>(GTS3DMD_FRAMES-(FRAMEDIV*1))
				*line++ = CORE_COLOR(DMD_DOTON);
			  else 
			  if(data>(GTS3DMD_FRAMES-(FRAMEDIV*2))
				*line++ = CORE_COLOR(DMD_DOT66);
			  else
			  if(data>1)
				*line++ = CORE_COLOR(DMD_DOT33);
			  else
				*line++ = CORE_COLOR(DMD_DOTOFF);
		  line++;
		  }
	  }
	  lines++;
  }
  drawStatus(bitmap,fullRefresh);
}

#else

/*--------------------------------------*/
/*Gottlieb System 3 128x32 DMD Handling*/
/*-------------------------------------*/
static core_tLCDLayout gts3_128x32DMD[] = {
	{0,0,32,128,CORE_DMD}, {0}
};

#if 0
void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  tDMDDot dotCol;
  int ii,jj,kk,ll;
  int offset;
  char temp[250];
  //logerror("crtc start address = %x\n",crtc6845_start_addr);

  offset = (crtc6845_start_addr>>2);
  sprintf(temp,"location=%04x   %04x",0x1000+offset, crtc6845_start_addr);
  core_textOutf(50,50,1,temp);

  /* Drawing is not optimised so just clear everything */
  if (fullRefresh) fillbitmap(bitmap,Machine->pens[0],NULL);

  /* Create a temporary buffer with all pixels */
  for (kk = 0, ii = 1; ii < (11*3); ii++) {
    UINT8 *line = &dotCol[ii][0];
    for (jj = 0; jj < 16; jj++) {
      /* Intensity depends on how many times the pixel */
      /* been on in the last 3 frames*/
		unsigned int intens1,intens2;
		intens1=intens2=0;
		for(ll = 0; ll < 3; ll++){
			intens1+=DMDFrames[ll][kk] & 0x55;
			intens2+=DMDFrames[ll][kk] & 0xaa;
		}
      *line++ = (intens2>>7) & 3;
      *line++ = (intens1>>6) & 3;
      *line++ = (intens2>>5) & 3;
      *line++ = (intens1>>4) & 3;
      *line++ = (intens2>>3) & 3;
      *line++ = (intens1>>2) & 3;
      *line++ = (intens2>>1) & 3;
	  *line++ = (intens1)    & 3;
      kk +=1;
    }
    *line = 0; /* to simplify antialiasing */
  }
  dmd_draw(bitmap, dotCol, core_gameData->lcdLayout ? core_gameData->lcdLayout : &gts3_128x32DMD[0]);

  drawStatus(bitmap,fullRefresh);
}

#else
static int offset=0;

//Start Address = 0000 = 0x1000
//Start Address = 0800 = 0x1200
//Start Address = 1000 = 0x1400
//Start Address = 1800 = 0x1600
//Start Address = 2000 = 0x1800
//Start Address = 2800 = 0x1a00
//Start Address = 3000 = 0x1c00
//Start Address = 3800 = 0x1e00

void gts3_dmd128x32_refresh(struct mame_bitmap *bitmap, int fullRefresh) {
  UINT8 *RAM,*RAM2;
  tDMDDot dotCol;
  int ii,jj,kk;
  char temp[250];
  //logerror("crtc start address = %x\n",crtc6845_start_addr);

  sprintf(temp,"location=%04x   %04x",0x1000+offset, crtc6845_start_addr);
  core_textOutf(50,50,1,temp);

#if 0
  if(keyboard_pressed_memory_repeat(KEYCODE_A,2))
    offset+=0x200;
  if(keyboard_pressed_memory_repeat(KEYCODE_B,2))
    offset-=0x200;
#endif

  offset=(crtc6845_start_addr>>2);

  RAM = memory_region(GTS3_MEMREG_DCPU1) + 0x1000+offset;
  RAM2= memory_region(GTS3_MEMREG_DCPU1) + 0x1000+offset;

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
  dmd_draw(bitmap, dotCol, gts3_128x32DMD);
  drawStatus(bitmap,fullRefresh);
}
#endif
#endif
