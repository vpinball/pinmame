/*
 *  Support for the XIL imaging library.
 *
 *  Elias Mårtenson (elias-m@algonet.se)
 */

/* moved above the #ifdef to avoid warning about empty c-files */
#include <stdio.h>

#ifdef USE_XIL

#include <thread.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xil/xil.h>
#include "xmame.h"
#include "x11.h"

static void scale_screen( XilImage );
static void *redraw_thread( void * );

static XilSystemState state;
static XilImage window_image, draw_image;
static int draw_image_xsize, draw_image_ysize;
static int window_width, window_height;

static pthread_mutex_t img_mutex;
static pthread_cond_t img_cond;
static int paintflag;
static XilImage back_image;

void init_xil( void )
{
  if( (state = xil_open()) == NULL ) {
    fprintf( stderr, "Failed to open XIL library, disabling\n" );
    use_xil = 0;
  }
  else {
    printf( "Using XIL library\n" );
  }
}

void setup_xil_images( int xsize, int ysize )
{
  XilMemoryStorage storage_info;
  pthread_t thread_id;

  window_image = xil_create_from_window( state, display, window );

  draw_image_xsize = xsize;
  draw_image_ysize = ysize;

  window_width = xsize;
  window_height = ysize;

  draw_image = xil_create( state, xsize, ysize, 1, XIL_SHORT );
  xil_export( draw_image );
  xil_get_memory_storage( draw_image, &storage_info );
  scaled_buffer_ptr = (char *)storage_info.byte.data;

  if( use_mt_xil ) {
    printf( "initializing scaling thread\n" );
    back_image = xil_create( state, xsize, ysize, 1, XIL_BYTE );
    pthread_mutex_init( &img_mutex, NULL );
    paintflag = 0;
    pthread_create( &thread_id, NULL, redraw_thread, NULL );
  }
}

void refresh_xil_screen( void )
{
  XilMemoryStorage storage_info;

  xil_import( draw_image, TRUE );
  if( use_mt_xil ) {
    pthread_mutex_lock( &img_mutex );
    while( paintflag ) {
      pthread_cond_wait( &img_cond, &img_mutex );
    }
    xil_copy( draw_image, back_image );
    paintflag = 1;
    pthread_mutex_unlock( &img_mutex );
    pthread_cond_signal( &img_cond );
  }
  else {
    scale_screen( draw_image );
  }

  xil_export( draw_image );
  xil_get_memory_storage( draw_image, &storage_info );
  scaled_buffer_ptr = (char *)storage_info.byte.data;
}

static void scale_screen( XilImage source )
{
  xil_scale( source, window_image, "nearest",
	     window_width / (float)draw_image_xsize,
	     window_height / (float)draw_image_ysize );
}

void update_xil_window_size( int width, int height )
{
  window_width = width;
  window_height = height;

  if( window_image != NULL ) {
    xil_destroy( window_image );
  }
  window_image = xil_create_from_window( state, display, window );
}  

static void *redraw_thread( void *arg )
{
  for( ;; ) {
    pthread_mutex_lock( &img_mutex );
    while( !paintflag ) {
      pthread_cond_wait( &img_cond, &img_mutex );
    }
    scale_screen( back_image );
    paintflag = 0;
    pthread_mutex_unlock( &img_mutex );
    pthread_cond_signal( &img_cond );
  }

  return NULL;
}

#endif
