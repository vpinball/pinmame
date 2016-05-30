/*****************************************************************

  Glide vector routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#if defined xfx || defined svgafx

#include <glide.h>
#include "xmame.h"
#include "driver.h"
#include "artwork.h"

extern int fxwidth,fxheight;
extern GuTexPalette texpal;

unsigned char *vectorram;
int vectorram_size;

int antialias;                            /* flag for anti-aliasing */
int beam;                                 /* size of vector beam    */
int translucency;

int pointnum;
GrVertex vec_vert[10000];

static int vecshift;
static float vecwidth,vecheight;

static int vector_orientation;

static float flicker_correction = 0.0;

/*
 * Initializes vector game video emulation
 */

int vector_vh_start (void)
{
  vecwidth=(float)(Machine->drv->default_visible_area.max_x-
	Machine->drv->default_visible_area.min_x);

  vecheight=(float)(Machine->drv->default_visible_area.max_y-
	Machine->drv->default_visible_area.min_y);

  pointnum=0;

  return 0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */

void vector_vh_stop (void)
{
}

/*
 * Setup scaling. Currently the Sega games are stuck at VECSHIFT 15
 * and the the AVG games at VECSHIFT 16
 */

void vector_set_shift (int shift)
{
  vecshift=shift;
}

/* Convert an xy point to xyz in the 3D scene */

void PointConvert(int x,int y,float *sx,float *sy)
{
  float dx,dy,tmp;

  dx=(float)(x>>vecshift)/vecwidth;
  dy=(float)(y>>vecshift)/vecheight;

  if(Machine->orientation&ORIENTATION_SWAP_XY) {
    tmp=dx;
    dx=dy;
    dy=tmp;
  }

  if(Machine->orientation&ORIENTATION_FLIP_X)
    dx=1.0-dx;

  if(Machine->orientation&ORIENTATION_FLIP_Y)
    dy=1.0-dy;

  *sx=dx*(float)fxwidth;
  *sy=(float)fxheight-dy*(float)fxheight;
}

/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */

void vector_add_point(int x, int y, int color, int intensity)
{
  GrVertex *vert;
  FxU32 pen;

  if(pointnum==10000)
	printf("Vector buffer overflow\n");
  else {
	vert=vec_vert+pointnum;

	vert->oow=1.0;

	pen=texpal.data[Machine->pens[color]];

	vert->r=(float)((pen>>16)&0x000000ff);
	vert->g=(float)((pen>>8)&0x000000ff);
	vert->b=(float)(pen&0x000000ff);

	vert->a=(float)intensity; /* /1.7; */

	PointConvert(x,y,&(vert->x),&(vert->y));
  }

  pointnum++;
}

/*
 * Add new clipping info to the list
 */

void vector_add_clip (int x1, int yy1, int x2, int y2)
{
}

/*
 * The vector CPU creates a new display list.
 */

void vector_clear_list(void)
{
  pointnum=0;
}

/* Called when the frame is complete */

void vector_vh_update(struct mame_bitmap *bitmap,int full_refresh)
{
}

void vector_set_flip_x (int flip)
{
	if (flip)
		vector_orientation |= ORIENTATION_FLIP_X;
	else
		vector_orientation &= ~ORIENTATION_FLIP_X;
}

void vector_set_flip_y (int flip)
{
	if (flip)
		vector_orientation |= ORIENTATION_FLIP_Y;
	else
		vector_orientation &= ~ORIENTATION_FLIP_Y;
}

void vector_set_swap_xy (int swap)
{
	if (swap)
		vector_orientation |= ORIENTATION_SWAP_XY;
	else
		vector_orientation &= ~ORIENTATION_SWAP_XY;
}

void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	/* flicker = (int)(flicker_correction * 2.55); */
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

#endif /* if defined xfx || defined svgafx */
