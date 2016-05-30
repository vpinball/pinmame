/*****************************************************************

  Generic glide routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#if defined xfx || defined svgafx
#include <stdio.h>
#include <math.h>
#include <glide.h>
#include "xmame.h"
#include "driver.h"

#define _32TO16(p) (UINT16)((((p) & 0x00F80000) >> 9) | \
                            (((p) & 0x0000F800) >> 6) | \
                            (((p) & 0x000000F8) >> 3))

#define PIXELCOLOR(x) fxdepth == 15 ? \
						(UINT16)(x) : \
                        (fxdepth == 16 ? \
                        	color_values[(UINT16)(x)] : \
                            (fxdepth == 24 || fxdepth == 32 ? \
                            _32TO16((UINT32)(x)) : 0))

#define PAUSEDCOLOR(p) (UINT16) ((((p) >> 11) << 10) | ((((p) &0x03E0) >> 6)<< 5) | (((p) & 0x001F) >> 1))

void CalcPoint(GrVertex *vert,int x,int y);
void InitTextures(void);
int  InitVScreen(void);
void CloseVScreen(void);
void UpdateTexture(struct mame_bitmap *bitmap);
void DrawFlatBitmap(void);
void UpdateFlatDisplay(void);
void UpdateFXDisplay(struct mame_bitmap *bitmap);
static int SetResolution(struct rc_option *option, const char *arg,
   int priority);

extern int pointnum;
extern UINT32 direct_rgb_components[3];
extern UINT16 *color_values;
extern int emulation_paused;

int fxwidth = 640;
int fxheight = 480;
static int fxdepth;
static int black;
static int white;

GuTexPalette texpal;

static int Gr_format = GR_TEXFMT_ARGB_1555;
static GrScreenResolution_t Gr_resolution = GR_RESOLUTION_640x480;
static GrHwConfiguration hwconfig;
static char version[80];
static GrTexInfo texinfo;
static int bilinear=1; /* Do binlinear filtering? */
static const int texsize=256;

/* The squares that are tiled to make up the game screen polygon */

struct TexSquare
{
  UINT16 *texture;
  unsigned int texobj;
  long texadd;
  float x1,y1,z1,x2,y2,z2,x3,y3,z3,x4,y4,z4;
  GrVertex vtxA, vtxB, vtxC, vtxD;
  float xcov,ycov;
};

static struct TexSquare *texgrid=NULL;
static int texnumx;
static int texnumy;
static float texpercx;
static float texpercy;
static float vscrntlx;
static float vscrntly;
static float vscrnwidth;
static float vscrnheight;
static float xinc,yinc;

float cscrx1,cscry1,cscrz1,cscrx2,cscry2,cscrz2,
  cscrx3,cscry3,cscrz3,cscrx4,cscry4,cscrz4;
float cscrwdx,cscrwdy,cscrwdz;
float cscrhdx,cscrhdy,cscrhdz;

struct rc_option fx_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "FX (Glide) Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "resolution",	"res",			rc_use_function, NULL,
     "640x480",		0,			0,		SetResolution,
     "Specify the resolution/ windowsize to use in the form of XRESxYRES" },
   { "keepaspect",	NULL,			rc_bool,	&normal_use_aspect_ratio,
     "1",		0,			0,		NULL,
     "Try / don't try to keep the aspect ratio of a game" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

int sysdep_display_16bpp_capable(void)
{
   return 1;
}

/* Allocate a palette */
int sysdep_display_alloc_palette(int writable_colors)
{
  InitTextures();
  return 0;
}

/* Change the color of a pen */
int sysdep_display_set_pen(int pen, unsigned char red,unsigned char green,
					unsigned char blue) 
{
  return 0;
}

void CalcPoint(GrVertex *vert,int x,int y)
{
  vert->x=vscrntlx+(float)x*texpercx*vscrnwidth;
  if(vert->x>vscrntlx+vscrnwidth) vert->x=vscrntlx+vscrnwidth;

  vert->y=vscrntly+vscrnheight-(float)y*texpercy*vscrnheight;
  if(vert->y<vscrntly) vert->y=vscrntly;
}

int InitGlide(void)
{
  int fd = open("/dev/3dfx", O_RDWR);
  if ((fd < 0) && geteuid())
  {
     fprintf(stderr, "Glide error: couldn't open /dev/3dfx and not running as root\n");
     return OSD_NOT_OK;
  }
  if (fd >= 0)
     close(fd);
  putenv("FX_GLIDE_NO_SPLASH=");
  grGlideInit();
  if (!grSstQueryHardware(&hwconfig))
  {
     grGlideShutdown();
     fprintf(stderr, "Glide error: no boards found\n");
     return OSD_NOT_OK;
  }
  return OSD_OK;
}

void InitTextures(void)
{
  int x,y;
  struct TexSquare *tsq;
  long texmem,memaddr;

  texinfo.smallLod=texinfo.largeLod=GR_LOD_256;
  texinfo.aspectRatio=GR_ASPECT_1x1;
  texinfo.format=Gr_format;

  texmem=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&texinfo);

  grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,
				   GR_COMBINE_FACTOR_ONE,
				   GR_COMBINE_LOCAL_NONE,
				   GR_COMBINE_OTHER_TEXTURE,
				   FXFALSE);

  grTexCombine(GR_TMU0,
			   GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
			   GR_COMBINE_FUNCTION_NONE,GR_COMBINE_FACTOR_NONE,
			   FXFALSE, FXFALSE);

  grTexMipMapMode(GR_TMU0,
				  GR_MIPMAP_DISABLE,
				  FXFALSE);

  grTexClampMode(GR_TMU0,
				 GR_TEXTURECLAMP_CLAMP,
				 GR_TEXTURECLAMP_CLAMP);

  if(bilinear)
	grTexFilterMode(GR_TMU0,
					GR_TEXTUREFILTER_BILINEAR,
					GR_TEXTUREFILTER_BILINEAR);
  else
	grTexFilterMode(GR_TMU0,
					GR_TEXTUREFILTER_POINT_SAMPLED,
					GR_TEXTUREFILTER_POINT_SAMPLED);

  /* Allocate the texture memory */
  
  texnumx=visual_width/texsize;
  if(texnumx*texsize!=visual_width) texnumx++;
  texnumy=visual_height/texsize;
  if(texnumy*texsize!=visual_height) texnumy++;
  
  xinc=vscrnwidth*((float)texsize/(float)visual_width);
  yinc=vscrnheight*((float)texsize/(float)visual_height);
  
  texpercx=(float)texsize/(float)visual_width;
  if(texpercx>1.0) texpercx=1.0;
  
  texpercy=(float)texsize/(float)visual_height;
  if(texpercy>1.0) texpercy=1.0;
  
  texgrid=(struct TexSquare *)
	malloc(texnumx*texnumy*sizeof(struct TexSquare));
  memaddr=grTexMinAddress(GR_TMU0);
  
  for(y=0;y<texnumy;y++) {
	for(x=0;x<texnumx;x++) {
	  tsq=texgrid+y*texnumx+x;

	  tsq->texadd=memaddr;
	  memaddr+=texmem;

	  if(x==(texnumx-1) && visual_width%texsize)
		tsq->xcov=(float)((visual_width)%texsize)/(float)texsize;
	  else tsq->xcov=1.0;
	  
	  if(y==(texnumy-1) && visual_height%texsize)
		tsq->ycov=(float)((visual_height)%texsize)/(float)texsize;
	  else tsq->ycov=1.0;

	  tsq->vtxA.oow=1.0;
	  tsq->vtxB=tsq->vtxC=tsq->vtxD=tsq->vtxA;
	
	  CalcPoint(&(tsq->vtxA),x,y);
	  CalcPoint(&(tsq->vtxB),x+1,y);
	  CalcPoint(&(tsq->vtxC),x+1,y+1);
	  CalcPoint(&(tsq->vtxD),x,y+1);
 	
	  tsq->vtxA.tmuvtx[0].sow=0.0;
	  tsq->vtxA.tmuvtx[0].tow=0.0;

	  tsq->vtxB.tmuvtx[0].sow=256.0*tsq->xcov;
	  tsq->vtxB.tmuvtx[0].tow=0.0;

	  tsq->vtxC.tmuvtx[0].sow=256.0*tsq->xcov;
	  tsq->vtxC.tmuvtx[0].tow=256.0*tsq->ycov;

	  tsq->vtxD.tmuvtx[0].sow=0.0;
	  tsq->vtxD.tmuvtx[0].tow=256.0*tsq->ycov;
	  

	  /* Initialize the texture memory */
	  tsq->texture=calloc(texsize*texsize, sizeof *(tsq->texture));
	}
  }
}

typedef struct {
    int         res;
    int       width;
    int       height;
} ResToRes;
		
static ResToRes resTable[] = {
    { GR_RESOLUTION_320x200,   320,  200 },  /* 0x0 */
    { GR_RESOLUTION_320x240,   320,  240 },  /* 0x1 */
    { GR_RESOLUTION_400x256,   400,  256 },  /* 0x2 */
    { GR_RESOLUTION_512x384,   512,  384 },  /* 0x3 */
    { GR_RESOLUTION_640x200,   640,  200 },  /* 0x4 */
    { GR_RESOLUTION_640x350,   640,  350 },  /* 0x5 */
    { GR_RESOLUTION_640x400,   640,  400 },  /* 0x6 */
    { GR_RESOLUTION_640x480,   640,  480 },  /* 0x7 */
    { GR_RESOLUTION_800x600,   800,  600 },  /* 0x8 */
    { GR_RESOLUTION_960x720,   960,  720 },  /* 0x9 */
    { GR_RESOLUTION_856x480,   856,  480 },  /* 0xA */
    { GR_RESOLUTION_512x256,   512,  256 },  /* 0xB */
    { GR_RESOLUTION_1024x768,  1024, 768 },  /* 0xC */
    { GR_RESOLUTION_1280x1024, 1280, 1024 }, /* 0xD */
    { GR_RESOLUTION_1600x1200, 1600, 1200 }, /* 0xE */
    { GR_RESOLUTION_400x300,   400,  300 }   /* 0xF */
};
			
static long resTableSize = sizeof( resTable ) / sizeof( ResToRes );

/* Get screen resolution */
static int SetResolution(struct rc_option *option, const char *arg,
   int priority)
{
  int width, height, match;
  if (sscanf(arg, "%dx%d", &width, &height) == 2)
  {
     for( match = 0; match < resTableSize; match++ )
        if( width==resTable[match].width && height==resTable[match].height)
        {
           Gr_resolution = resTable[match].res;
           fxwidth = width;
           fxheight = height;
           option->priority = priority;
           return 0;
        }
  }
  fprintf(stderr,
      "error: invalid resolution or unknown resolution: \"%s\".\n"
      "   Valid resolutions are:\n", arg);
  for( match = 0; match < resTableSize; match++ )
  {
     fprintf(stderr_file, "   \"%dx%d\"", resTable[match].width,
        resTable[match].height);
     if (match && (match % 5) == 0)
        fprintf(stderr_file, "\n");
  }
  return -1;
}


/* Set up the virtual screen */

int InitVScreen(void)
{
  float scrnaspect,vscrnaspect;

  grGlideGetVersion(version);

  fxdepth = Machine->color_depth;

  fprintf(stderr_file, "info: using Glide version %s\n", version);
  
  grSstSelect(0);

  if(!grSstWinOpen(0,Gr_resolution,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,
     GR_ORIGIN_LOWER_LEFT,2,1))
  {
     fprintf(stderr_file, "error opening Glide window, do you have enough memory on your 3dfx for the selected mode?\n");
     return OSD_NOT_OK;
  }
  fprintf(stderr_file,
     "info: screen resolution set to %dx%d\n", fxwidth, fxheight);

  /* clear the buffer */

  grBufferClear(0,0,GR_ZDEPTHVALUE_FARTHEST);


  if (use_aspect_ratio)
  {
     scrnaspect=(float)visual_width/(float)visual_height;
     vscrnaspect=(float)fxwidth/(float)fxheight;

     if(scrnaspect<vscrnaspect) {
   	vscrnheight=(float)fxheight;
   	vscrnwidth=vscrnheight*scrnaspect;
   	vscrntlx=((float)fxwidth-vscrnwidth)/2.0;
   	vscrntly=0.0;
     }
     else {
   	vscrnwidth=(float)fxwidth;
   	vscrnheight=vscrnwidth/scrnaspect;
   	vscrntlx=0.0;
   	vscrntly=((float)fxheight-vscrnheight)/2.0;
     }
  }
  else
  {
     vscrnwidth=(float)fxwidth;
     vscrnheight=(float)fxheight;
     vscrntlx=0.0;
     vscrntly=0.0;
  }
  
  /* fill the display_palette_info struct */
  memset(&display_palette_info, 0, sizeof(struct sysdep_palette_info));
  switch(fxdepth) {
    case 15:
    case 16:
      display_palette_info.depth = 16;
      display_palette_info.red_mask   = 0x7C00;
      display_palette_info.green_mask = 0x03E0;
      display_palette_info.blue_mask  = 0x001F;
      break;
    case 24:
    case 32:
      display_palette_info.depth = 32;
      display_palette_info.red_mask   = 0xFF0000;
      display_palette_info.green_mask = 0x00FF00;
      display_palette_info.blue_mask  = 0x0000FF;
      break;
  }
   
  black = video_colors_used -2;
  white = video_colors_used -1;

  return OSD_OK;
}

/* Close down the virtual screen */

void CloseVScreen(void)
{
  int x,y;

  /* Free Texture stuff */

  if(texgrid) {
	for(y=0;y<texnumy;y++) {
	  for(x=0;x<texnumx;x++) {
		free(texgrid[y*texnumx+x].texture);
	  }
	}
	
	free(texgrid);
	texgrid = NULL;
  }

  grGlideShutdown();
}

/* Not needed under GL */

void sysdep_clear_screen(void)
{
}


/* Update the texture with the contents of the game screen */

void UpdateTexture(struct mame_bitmap *bitmap)
{
	int y,rline,texline,xsquare,ysquare,ofs,width, i;
	struct TexSquare *square;

	if(visual_width<=texsize) width=visual_width;
	else width=texsize;

	switch (fxdepth) {


	case 15:
		for(y=visual.min_y;y<=visual.max_y;y++) {
			rline=y-visual.min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(visual_width%texsize))
					width=texsize;
				else width=visual_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;

				if (emulation_paused) {	
					for(i = 0;i < width;i++) {	
						square->texture[texline*texsize+i] = ((UINT16*)(bitmap->line[y]))[visual.min_x+ofs+i];
					}
				} else {
                                       	for(i = 0;i < width;i++) {
                                               	square->texture[texline*texsize+i] = PAUSEDCOLOR((((UINT16*)(bitmap->line[y]))[visual.min_x+ofs+i]));
                                       	}
				}
			}
		} 
		break;

	case 16:
		
		for(y=visual.min_y;y<=visual.max_y;y++) {
			rline=y-visual.min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(visual_width%texsize))
					width=texsize;
				else width=visual_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;
				if (emulation_paused) {
					for(i = 0;i < width;i++) {
						square->texture[texline*texsize+i] = 
							color_values[(((UINT16*)(bitmap->line[y]))[visual.min_x+ofs+i])];
					}
				} else {
					for(i = 0;i < width;i++) {
						square->texture[texline*texsize+i] =
							PAUSEDCOLOR(color_values[(((UINT16*)(bitmap->line[y]))[visual.min_x+ofs+i])]);
					}
				}
			}
		}
		break;

	case 24:
	case 32:
		for(y=visual.min_y;y<=visual.max_y;y++) {
			rline=y-visual.min_y;
			ysquare=rline/texsize;
			texline=rline%texsize;
			
			for(xsquare=0;xsquare<texnumx;xsquare++) {
				ofs=xsquare*texsize;
				
				if(xsquare<(texnumx-1) || !(visual_width%texsize))
					width=texsize;
				else width=visual_width%texsize;
				
				square=texgrid+(ysquare*texnumx)+xsquare;
					
				if (emulation_paused) {
					for(i = 0;i < width;i++) {
						square->texture[texline*texsize+i] = 
							PIXELCOLOR((((UINT32*)(bitmap->line[y]))[visual.min_x+ofs+i]));
					}
				} else {
					for(i = 0;i < width;i++) {
						square->texture[texline*texsize+i] =
							PAUSEDCOLOR(PIXELCOLOR((((UINT32*)(bitmap->line[y]))[visual.min_x+ofs+i])));
					}
				}
			}
		}
		break;
	}
}

void DrawFlatBitmap(void)
{
  struct TexSquare *square;
  int x,y;

  for(y=0;y<texnumy;y++) {
	for(x=0;x<texnumx;x++) {
	  square=texgrid+y*texnumx+x;

	  texinfo.data=(void *)square->texture;

	  grTexDownloadMipMapLevel(GR_TMU0,square->texadd,
							   GR_LOD_256,GR_LOD_256,GR_ASPECT_1x1,
							   Gr_format,
							   GR_MIPMAPLEVELMASK_BOTH,texinfo.data);

	  grTexSource(GR_TMU0,square->texadd,
				  GR_MIPMAPLEVELMASK_BOTH,&texinfo);

	  grDrawTriangle(&(square->vtxA),&(square->vtxD),&(square->vtxC));
	  grDrawTriangle(&(square->vtxA),&(square->vtxB),&(square->vtxC));
	}
  }
}

void UpdateFlatDisplay(void)
{
  grBufferClear(0,0,GR_ZDEPTHVALUE_FARTHEST);
  DrawFlatBitmap();
  grBufferSwap(1);
}

void UpdateFXDisplay(struct mame_bitmap *bitmap)
{
  if(bitmap) 
    UpdateTexture(bitmap);

  UpdateFlatDisplay();
}

/* used when expose events received */

void osd_refresh_screen(void)
{
  /* Just re-draw the whole screen */

  UpdateFXDisplay(NULL);
}


/* invoked by main tree code to update bitmap into screen */

void sysdep_update_display(struct mame_bitmap *bitmap)
{
  if(keyboard_pressed(KEYCODE_RCONTROL)) {
	if(keyboard_pressed_memory(KEYCODE_B)) {
	  bilinear=1-bilinear;

	  if(bilinear)
		grTexFilterMode(GR_TMU0,
						GR_TEXTUREFILTER_BILINEAR,
						GR_TEXTUREFILTER_BILINEAR);
	  else
		grTexFilterMode(GR_TMU0,
						GR_TEXTUREFILTER_POINT_SAMPLED,
						GR_TEXTUREFILTER_POINT_SAMPLED);

	}
  }

  UpdateFXDisplay(bitmap);
}

#endif /* if defined xfx || defined svgafx */
