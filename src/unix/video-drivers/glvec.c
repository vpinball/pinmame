/*****************************************************************

  OpenGL vector routines

  Copyright 1998 by Mike Oliphant - oliphant@ling.ed.ac.uk

    http://www.ling.ed.ac.uk/~oliphant/glmame

  Improved by Sven Goethel, http://www.jausoft.com, sgoethel@jausoft.com

  This code may be used and distributed under the terms of the
  Mame license

*****************************************************************/

#ifdef xgl

#include "xmame.h"
#include "glmame.h"
#include "driver.h"
#include "artwork.h"
#include "vidhrdw/vector.h"

#include "osinline.h"

void CalcCabPointbyViewpoint( 
		   GLdouble vx_gscr_view, GLdouble vy_gscr_view, 
                   GLdouble *vx_p, GLdouble *vy_p, GLdouble *vz_p
		 );

extern int winwidth,winheight;
extern GLdouble  s__cscr_w_view, s__cscr_h_view;


unsigned char *vectorram;
size_t vectorram_size;

GLuint veclist=0;

int listcreated=0;
int inlist=0;
int inbegin=0;

static int vec_min_x;
static int vec_min_y;
static int vec_max_x;
static int vec_max_y;
static int vec_cent_x;
static int vec_cent_y;
static int vecwidth;
static int vecheight;
static int vecshift;

static GLdouble vecoldx,vecoldy;

float gl_beam=1.0;
static int flicker;                              /* beam flicker value     */

static int vector_orientation;

/*
static double gamma_correction = 1.2;
*/
static double flicker_correction = 0.0;
static double intensity_correction = 1.5;

vector_pixel_t *vector_dirty_list;

double osd_get_gamma(void);

/*
 * multiply and divide routines for drawing lines
 * can be be replaced by an assembly routine in osinline.h
 */
#ifndef vec_mult
INLINE int vec_mult(int parm1, int parm2)
{
	int temp,result;

	temp     = abs(parm1);
	result   = (temp&0x0000ffff) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp&0x0000ffff) * (parm2>>16       );
	result  += (temp>>16       ) * (parm2&0x0000ffff);
	result >>= 16;
	result  += (temp>>16       ) * (parm2>>16       );

	if( parm1 < 0 )
		return(-result);
	else
		return( result);
}
#endif

/* can be be replaced by an assembly routine in osinline.h */
#ifndef vec_div
INLINE int vec_div(int parm1, int parm2)
{
	if( (parm2>>12) )
	{
		parm1 = (parm1<<4) / (parm2>>12);
		if( parm1 > 0x00010000 )
			return( 0x00010000 );
		if( parm1 < -0x00010000 )
			return( -0x00010000 );
		return( parm1 );
	}
	return( 0x00010000 );
}
#endif

void set_gl_beam(float new_value)
{
	gl_beam = new_value;
	disp__glLineWidth(gl_beam);
	disp__glPointSize(gl_beam);
	printf("GLINFO (vec): beamer size %f\n", gl_beam);
}

float get_gl_beam()
{ return gl_beam; }

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

/*
void vector_set_gamma(double _gamma)
{
	gamma_correction = _gamma;
}

double vector_get_gamma(void)
{
	return gamma_correction;
}
*/

void vector_set_flicker(float _flicker)
{
	flicker_correction = _flicker;
	flicker = (int)(flicker_correction * 2.55);
}

float vector_get_flicker(void)
{
	return flicker_correction;
}

void vector_set_intensity(float _intensity)
{
	intensity_correction = _intensity;
}

float vector_get_intensity(void)
{
	return intensity_correction;
}


/*
 * Initializes vector game video emulation
 */

VIDEO_START( vector )
{
        vec_min_x =Machine->visible_area.min_x;
        vec_min_y =Machine->visible_area.min_y;
        vec_max_x =Machine->visible_area.max_x;
        vec_max_y =Machine->visible_area.max_y;
        vecwidth  =vec_max_x-vec_min_x;
        vecheight =vec_max_y-vec_min_y;
        vec_cent_x=(vec_max_x+vec_min_x)/2;
        vec_cent_y=(vec_max_y+vec_min_y)/2;

	printf("GLINFO (vec): min %d/%d, max %d/%d, cent %d/%d,\n\t vecsize %dx%d\n", 
		vec_min_x, vec_min_y, vec_max_x, vec_max_y,
		vec_cent_x, vec_cent_y, vecwidth, vecheight);

        veclist=disp__glGenLists(1);
	listcreated=1;

	set_gl_beam(gl_beam);
	vector_set_flicker(options.vector_flicker);

  	return 0;
}

/*
 * Stop the vector video hardware emulation. Free memory.
 */

void vector_vh_stop (void)
{
	if(inlist && inbegin)
	{
		GL_END();
		inbegin=0;
	}

	CHECK_GL_BEGINEND();
	CHECK_GL_ERROR ();

	if(inlist) {
		disp__glEndList();
		inlist=0;
	}

	if(listcreated)
	{
		disp__glDeleteLists(veclist, 1);
		listcreated=0;
	}
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

int PointConvert(int x,int y,GLdouble *sx,GLdouble *sy,GLdouble *sz)
{
  int x1, y1;
  GLdouble dx,dy,tmp;

  x1=x>>vecshift;
  y1=y>>vecshift;

  dx=(GLdouble)x1/(GLdouble)vecwidth;
  dy=(GLdouble)y1/(GLdouble)vecheight;

  if(!cabview) {
	  if(Machine->orientation&ORIENTATION_SWAP_XY) {
	    tmp=dx;
	    dx=dy;
	    dy=tmp;
	  }

	  if(Machine->orientation&ORIENTATION_FLIP_X)
	    dx=1.0-dx;

	  if(Machine->orientation&ORIENTATION_FLIP_Y)
	    dy=1.0-dy;

	  *sx=dx;
	  *sy=dy;
  } else {
	CalcCabPointbyViewpoint ( dx*s__cscr_w_view,
	                          dy*s__cscr_h_view,
				  sx, sy, sz );
  }

  if( 0<=*sx && *sx<=1.0 &&
      0<=*sy && *sy<=1.0 
    )
    return 1;

  /*
  printf("GLINFO (vec): x/y %d/%d,\n\tx1/y1 %d/%d, dx/dy %f/%f, size %dx%d\n",
  	x, y, x>>vecshift, y>>vecshift, dx, dy, vecwidth, vecheight);
  */

  return 0;
}

static void vector_begin_list(void)
{
  disp__glNewList(veclist,GL_COMPILE);
  inlist=1;

  CHECK_GL_ERROR ();

  disp__glColor4d(1.0,1.0,1.0,1.0);

  GL_BEGIN(GL_LINE_STRIP);
  inbegin=1;
}


/*
 * Adds a line end point to the vertices list. The vector processor emulation
 * needs to call this.
 */
void vector_add_point (int x, int y, rgb_t color, int intensity)
{
  unsigned char r1,g1,b1;
  double red=0.0, green=0.0, blue=0.0;
  GLdouble sx,sy,sz;
  int ptHack=0;
  int ok;
  double gamma_correction = palette_get_global_gamma();

  if(!inlist)
	vector_begin_list();

  ok = PointConvert(x,y,&sx,&sy,&sz);

  b1 =   color        & 0xff ;
  g1 =  (color >>  8) & 0xff ;
  r1 =  (color >> 16) & 0xff ;

  if(intensity<0)
	intensity=0;

  if(sx==vecoldx&&sy==vecoldy&&inbegin&&intensity>0) 
  {
	  /**
	   * Hack to draw points -- zero-length lines don't show up
	   *
	   * But games, e.g. tacscan have zero lines within the LINE_STRIP,
	   * so we do try to continue the line strip :-)
	   *
	   * Part 1
	   */
	  GL_END();
	  inbegin=0;
  	  ptHack=1;
  } else {
  	  
	  /**
	   * the usual "normal" way ..
	   */
	  if(intensity==0&&inbegin)
	  {
		GL_END();
		inbegin=0;
	  }

	  if(!inbegin)
	  {
		GL_BEGIN(GL_LINE_STRIP);
		inbegin=1;
	  }
  	  ptHack=0;
  }

  intensity *= intensity_correction;
  if (intensity > 0xff)
	intensity = 0xff;

  if (flicker && (intensity > 0))
  {
	intensity += (intensity * (0x80-(rand()&0xff)) * flicker)>>16;
	if (intensity < 0)
		intensity = 0;
	if (intensity > 0xff)
		intensity = 0xff;
  }

  if(use_mod_ctable)
  {
	  red   = (double)intensity/255.0 * pow (r1 / 255.0, 1 / gamma_correction);
	  green = (double)intensity/255.0 * pow (g1 / 255.0, 1 / gamma_correction);
	  blue  = (double)intensity/255.0 * pow (b1 / 255.0, 1 / gamma_correction);
	  disp__glColor3d(red, green, blue);
  } else {
	  disp__glColor3ub(r1, g1, b1);
  }

  if(ptHack)
  {
	/**
	 * Hack to draw points -- zero-length lines don't show up
	 *
	 * But games, e.g. tacscan have zero lines within the LINE_STRIP,
	 * so we do try to continue the line strip :-)
	 *
	 * Part 2
	 */
  	if(inbegin) {
	        /* see above Part 1 */
		fprintf(stderr,"GLERROR: never ever at %s, %d\n",
			__FILE__, __LINE__);
	}
	GL_BEGIN(GL_POINTS);
	inbegin=1;

	if(cabview)
	  disp__glVertex3d(sx,sy,sz);
	else disp__glVertex2d(sx,sy);

	GL_END();
	inbegin=0;

	GL_BEGIN(GL_LINE_STRIP);
	inbegin=1;
  }

  if(cabview)
    disp__glVertex3d(sx,sy,sz);
  else disp__glVertex2d(sx,sy);

  vecoldx=sx; vecoldy=sy;
}

void vector_add_point_callback (int x, int y, rgb_t (*color_callback)(void), int intensity)
{
  vector_add_point(x, y, (*color_callback)(), intensity);
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
  if(inbegin) {
	GL_END();
	inbegin=0;
  }

  CHECK_GL_BEGINEND();
  CHECK_GL_ERROR ();

  if(!listcreated)
  {
	  CHECK_GL_BEGINEND();
	  veclist=disp__glGenLists(1);
	  listcreated=1;
  }

  CHECK_GL_ERROR ();

  if(inlist) {
	disp__glEndList();
	inlist=0;
  }

  CHECK_GL_ERROR ();
}

/* Called when the frame is complete */

VIDEO_UPDATE( vector )
{
  if (!cliprect && bitmap!=NULL)
  {
	fillbitmap(bitmap,Machine->uifont->colortable[0],NULL);
  }

  if(inlist && inbegin)
  {
	GL_END();
	inbegin=0;
  }

  if(inlist) {
	disp__glEndList();
	inlist=0;
  }
}

#endif /* ifdef xgl */
