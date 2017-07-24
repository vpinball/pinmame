
#ifdef AVICAPTURE

#include <math.h>
#include <X11/Xlib.h>
#include "xmame.h"
#include "x11.h"
#include "input.h"
#include "keyboard.h"
 
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#include <ffmpeg/common.h>
#include <ffmpeg/avcodec.h>

const int BUFFSIZE=1000000;
FILE * video_outf;
AVCodec * avc;
AVCodecContext * avctx;
AVPicture inpic, outpic;
char * output_buffer;
AVFrame * pic;

int frame_halver=2; // save each frame=1,  save every other frame=2

void init_dumper( int width, int height )
{  
  double fps = Machine->drv->frames_per_second / (double)frame_halver;

  avcodec_init();
  avcodec_register_all();
#ifdef AVICAPTURE_DEBUG
  av_log_set_level (99);
#endif

  avc = avcodec_find_encoder( CODEC_ID_MPEG1VIDEO );
  if (avc == NULL)
  {
  	  printf ("cannot find MPEG encoder\n");
     exit (1);
  }

  avctx = avcodec_alloc_context();
    
  /* sample parameters */
  avctx->me_method = ME_LOG;
  avctx->pix_fmt = PIX_FMT_YUV420P;
  avctx->bit_rate = 2500000;
  avctx->width = width;
  avctx->height = height;
  avctx->time_base.num = 1;
  avctx->time_base.den = fps;
  avctx->gop_size=10;
  avctx->max_b_frames=1;
  avctx->draw_horiz_band = NULL;
  avctx->idct_algo = FF_IDCT_AUTO;

  int ret = avcodec_open( avctx, avc );
  if (ret)
    {
      printf("FAILED TO OPEN ENCODER, ret=%d, errno=%d\n", ret, errno);
      exit( 1 );
    }
  
  int size=height*width;
  
  pic = avcodec_alloc_frame();
  
  output_buffer=(char *)malloc(BUFFSIZE); /* Find where this value comes from */
  
  outpic.data[0]=(unsigned char *)malloc(size*3/2); /* YUV 420 Planar */
  outpic.data[1]=outpic.data[0]+size;
  outpic.data[2]=outpic.data[1]+size/4;
  outpic.data[3]=NULL;
  outpic.linesize[0]=width;
  outpic.linesize[1]=outpic.linesize[2]=width/2;
  outpic.linesize[3]=0;
  
  pic->data[0]=outpic.data[0];  /* Points to data portion of outpic     */
  pic->data[1]=outpic.data[1];  /* Since encode_video takes an AVFrame, */
  pic->data[2]=outpic.data[2];  /* and img_convert takes an AVPicture   */
  pic->data[3]=outpic.data[3];
  
  pic->linesize[0]=outpic.linesize[0]; /* This doesn't change */
  pic->linesize[1]=outpic.linesize[1];
  pic->linesize[2]=outpic.linesize[2];
  pic->linesize[3]=outpic.linesize[3];
  
  inpic.data[0]=(unsigned char *)malloc(size*3); /* RGB24 packed in 1 plane */
  inpic.data[1]=inpic.data[2]=inpic.data[3]=NULL;
  inpic.linesize[0]=width*3;
  inpic.linesize[1]=inpic.linesize[2]=inpic.linesize[3]=0;

  video_outf = fopen("video.outf","wb");
  if (video_outf == NULL)
  {
    printf ("failed to open output video file\n");
    exit (1);
  }
}

void frame_dump ( struct mame_bitmap * bitmap )
{
  static unsigned int *dumpbig = NULL;
  unsigned char *dumpd;
  int y;
  int xoff, yoff, xsize, ysize;
  int outsz;
  static int framecnt=0;
  static unsigned char * myoutframe;

  framecnt++;
  if ((framecnt % frame_halver) != 0)
    return; // skip this frame

#if 0
  xoff = Machine->visible_area.min_x;
  yoff = Machine->visible_area.min_y;
  xsize= Machine->visible_area.max_x-xoff+1;
  ysize = Machine->visible_area.max_y-yoff+1;
#endif

  xsize = visual_width;
  ysize = visual_height;
  xoff=0;
  yoff=0; 

	if (!dumpbig)
	{
		int dstsize = bitmap->width *bitmap->height * sizeof (unsigned int);
		dumpbig = malloc ( dstsize );
		myoutframe = malloc( dstsize );
  }

  dumpd = (unsigned char*)dumpbig;

	/* Blit into dumpbig */
#define INDIRECT current_palette->lookup
#define DEST dumpbig
#define DEST_WIDTH (bitmap->width)
#define SRC_PIXEL unsigned short
#define DEST_PIXEL unsigned int
#define PACK_BITS
#include "blit.h"
#undef PACK_BITS
#undef SRC_PIXEL
#undef DEST_PIXEL
#undef INDIRECT

	/* Now make some corrections. */
	for (y=0; y < ysize; y++)
   {
   	int offs = bitmap->width*(y+yoff)*4;
      int x;

      for(x=0; x < xsize; x++)
	   {
	   	unsigned char c;
	  		c = dumpd[offs+x*3+2];
	  		dumpd[offs+x*3+2] = dumpd[offs+x*3];
	      dumpd[offs+x*3] = c;
	   }

		memcpy( &myoutframe[xsize*y*3], &dumpd[offs+3*xoff], xsize*3 );
	}

	/* dumpd now contains a nice RGB (or somethiing) frame.. */
  inpic.data[0] = myoutframe;

  img_convert(&outpic, PIX_FMT_YUV420P, &inpic, PIX_FMT_RGB24, xsize, ysize); 
  
  outsz = avcodec_encode_video (avctx, 
  		(unsigned char*)output_buffer, BUFFSIZE, pic);
  fwrite(output_buffer, 1, outsz, video_outf);
}


void done_dumper()
{
  printf("killing dumper...\n");
  avcodec_close(avctx);
  // there might have been something with buffers and b frames that 
  // i should have taken care of at this point.. oh well

  if (video_outf)
  {
     fwrite("\0x00\0x00\0x01\0xb7", 1, 4, video_outf); // mpeg end sequence..
     fclose(video_outf);
	  video_outf = NULL;
  }

  av_free(pic);
  av_free(avctx);
}


#endif /* AVICAPTURE */

