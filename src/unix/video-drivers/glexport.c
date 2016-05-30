#include "driver.h"
#include "../../png.h"
#include "glmame.h"
#include <stdarg.h>

extern int snapno;

void gl_save_screen_snapshot()
{
        void *fp;
        char name[20];
 
 
        /* avoid overwriting existing files */
        /* first of all try with "gamename.png" */
        sprintf(name,"%.8s", Machine->gamedrv->name);
        if (mame_faccess(name,FILETYPE_SCREENSHOT))
        {
                do
                {
                        /* otherwise use "nameNNNN.png" */
                        sprintf(name,"%.4s%04d",Machine->gamedrv->name,snapno++);
                } while (mame_faccess(name, FILETYPE_SCREENSHOT));
        }
 
        if ((fp = mame_fopen(Machine->gamedrv->name, name, FILETYPE_SCREENSHOT, 1)) != NULL)
        {
                gl_png_write_bitmap(fp);
                mame_fclose(fp);
        }
}

int gl_png_write_bitmap(void *fp)
{
	UINT8 *ip;
	struct png_info p;

	memset (&p, 0, sizeof (struct png_info));
	p.xscale = p.yscale = p.source_gamma = 0.0;
	p.palette = p.trans = p.image = p.zimage = p.fimage = NULL;
	p.width = winwidth;
	p.height = winheight;

	p.color_type = 2;

	p.rowbytes = p.width * 3;
	p.bit_depth = 8;
	if((p.image = (UINT8 *)malloc (p.height * p.rowbytes))==NULL)
	{
		logerror("Out of memory\n");
		return 0;
	}

	ip = p.image;

        disp__glPixelStorei(GL_PACK_ALIGNMENT, 1);
        disp__glPixelStorei(GL_PACK_ROW_LENGTH, p.width);
        disp__glPixelStorei(GL_PACK_LSB_FIRST, GL_TRUE);

        disp__glReadPixels(0,0, p.width, p.height,
		     GL_RGB, GL_UNSIGNED_BYTE,
		     ip);

        disp__glPixelStorei(GL_PACK_ALIGNMENT,4);
        disp__glPixelStorei(GL_PACK_ROW_LENGTH, 0);

	if(png_filter (&p)==0)
		return 0;

	if (png_deflate_image(&p)==0)
		return 0;

	if(png_write_sig(fp) == 0)
		return 0;

	if (png_write_datastream(fp, &p)==0)
		return 0;

	if (p.palette) free (p.palette);
	if (p.image) free (p.image);
	if (p.zimage) free (p.zimage);
	if (p.fimage) free (p.fimage);
	return 1;
}


void ppm_save_snapshot (void *fp)
{
  FILE						*file = fp;
  int						pixelsize, m, i, pixelnum;
  static unsigned char		*pixels;

  int width  = winwidth;
  int height = winheight;

  if(gl_is_initialized==0) return;

  m         = width * 3 /*RGB*/ ;
  pixelsize = width * height  * 3 /*RGB*/ ;

  pixels=(unsigned char *)calloc(pixelsize, 1);

  disp__glPixelStorei(GL_PACK_ALIGNMENT, 1);
  disp__glPixelStorei(GL_PACK_ROW_LENGTH, width);
  disp__glPixelStorei(GL_PACK_LSB_FIRST, GL_TRUE);

  disp__glReadPixels(0,0, width, height,
			   GL_RGB, GL_UNSIGNED_BYTE,
			   pixels);

  disp__glPixelStorei(GL_PACK_ALIGNMENT,4);
  disp__glPixelStorei(GL_PACK_ROW_LENGTH, 0);

  fprintf(file,"P6\n#GLmame screenshot\n%d %d\n255\n",
		  (int)width, (int)height);
  
  
  for(pixelnum=0, i=height-1;i>=0;i--) 
  {
	fwrite(&pixels[i*m], sizeof(unsigned char), m, file);
  }

  free(pixels); pixels=0;

  return;
}


