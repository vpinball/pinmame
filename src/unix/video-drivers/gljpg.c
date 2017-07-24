/*****************************************************************

  JPEG loading code for GLmame

  This code is basically taken verbatim from the libjpeg distribution,
  which is under to GNU Public License

*****************************************************************/

#ifdef xgl

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeglib.h"
#include <setjmp.h>
#include <GL/glu.h>

extern char *cabname;

struct my_error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  (*cinfo->err->output_message) (cinfo);

  longjmp(myerr->setjmp_buffer, 1);
}

GLubyte *read_JPEG_file (char * fname)
{
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  FILE * infile;		/* source file */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  long cont;
  JSAMPLE *image_buffer;
  char filename[256];

  sprintf(filename,"%s/cab/%s/%s",XMAMEROOT,cabname,fname);

  if ((infile = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return NULL;
  }

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, infile);

  (void) jpeg_read_header(&cinfo, TRUE);
  (void) jpeg_start_decompress(&cinfo);
  row_stride = cinfo.output_width * cinfo.output_components;

  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

 image_buffer=(JSAMPLE *) malloc(cinfo.image_width*cinfo.image_height*3);

  cont=(long)cinfo.output_height-1;
  while (cinfo.output_scanline < cinfo.output_height) {
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    memcpy(image_buffer+cinfo.image_width*3*cont,buffer[0],row_stride);
    cont--;
  }

  (void) jpeg_finish_decompress(&cinfo);

  jpeg_destroy_decompress(&cinfo);

  fclose(infile);

  return image_buffer;
}


#endif /* ifdef xgl */
