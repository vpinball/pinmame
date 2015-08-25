#include "xmame.h"
#include "driver.h"

static int mode_disable(struct rc_option *option, const char *s, int priority);

struct rc_option mode_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Mode Selection Related", NULL,	rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "keepaspect",	"ka",			rc_bool,	&normal_use_aspect_ratio,
     "1",		0,			0,		NULL,
     "Try / don't try to keep the aspect ratio of a game when selecting the best videomode" },
   { "displayaspectratio", "dar",		rc_float,	&display_aspect_ratio,
     "1.33",		0.75,			1.33,		NULL,
     "Set the display aspect ratio of your monitor. This is used for -keepaspect The default = 1.33 (4/3). Use 0.75 (3/4) for a portrait monitor" },
   { "disablemode",	"dm",			rc_use_function, NULL,
     NULL,		0,			0,		mode_disable,
     "Don't use mode XRESxYRESxDEPTH this can be used to disable specific video modes which don't work on your system. The xDEPTH part of the string is optional. This option may be used more then once" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

#define MODE_DISABLED_MAX 32

static int disabled_modes_count = 0;

static struct 
{
   int width;
   int height;
   int depth;
} disabled_modes[MODE_DISABLED_MAX];

static int mode_disable(struct rc_option *option, const char *s, int priority)
{
   if (disabled_modes_count == MODE_DISABLED_MAX)
   {
      /* stderr_file doesn't have a valid value yet when we're called ! */
      fprintf(stderr, "OSD: Warning: You can't disable more then %d modes. Mode %s not disabled\n",
          MODE_DISABLED_MAX, s);
      return OSD_OK;
   }
   if (sscanf(s, "%dx%dx%d",
       &disabled_modes[disabled_modes_count].width,
       &disabled_modes[disabled_modes_count].height,
       &disabled_modes[disabled_modes_count].depth) < 2)
      return OSD_NOT_OK;
   switch (disabled_modes[disabled_modes_count].depth)
   {
      case 0:
      case 256:
      case 65536:
         break;
      default:
         /* stderr_file doesn't have a valid value yet when we're called ! */
         fprintf(stderr, "Svgalib: Warning: No such depth: %d. Mode not disabled\n",
            disabled_modes[disabled_modes_count].depth);
         return OSD_NOT_OK;
   }
   disabled_modes_count++;
   return OSD_OK;
}

int mode_disabled(int width, int height, int depth)
{
   int i;
   for(i=0; i<disabled_modes_count; i++)
   {
      if (disabled_modes[i].width  == width &&
          disabled_modes[i].height == height)
      {
         switch (disabled_modes[disabled_modes_count].depth)
         {
            case 0:
               return TRUE;
            case 256:
               if(depth == 8)
                  return TRUE;
               break;
            case 65536:
               if(depth == 16)
                  return TRUE;
               break;
         }
      }
   }
   return FALSE;
}

void mode_perfect(int *width, int *height)
{
   double pixel_aspect_ratio;
   static int first_time = TRUE;
   
   if (use_aspect_ratio)
   {
      /* first of all calculate the pixel aspect_ratio the game has */
      if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
      {
         pixel_aspect_ratio = 1.0;
      }
      else
      {
         pixel_aspect_ratio = (visual_width * widthscale) / 
	   (yarbsize ? yarbsize :
	    (visual_height * heightscale * aspect_ratio));
      }
      
      /* should we maximize the used height, or the used width? */
      if (display_aspect_ratio >= aspect_ratio)
      {
	*height = yarbsize ? yarbsize : (visual_height * heightscale);
	*width  = *height * pixel_aspect_ratio * display_aspect_ratio;
      }
      else
      {
         *width  = visual_width  * widthscale;
         *height = *width / (pixel_aspect_ratio * display_aspect_ratio);
      }
      if (first_time)
      {
         fprintf(stderr_file, "OSD: Info: Ideal mode for this game = %dx%d\n",
            *width, *height);
         first_time = FALSE;
      }
   }
   else
   {
      *width  = visual_width;
      *height = visual_height;
   }
}

/* match a given mode to the needed width, height and aspect ratio to
   perfectly display a game.
   This function returns 0 for a not usable mode and 100 for the perfect mode.
*/

int mode_match(int width, int height)
{
   int wanted_width, wanted_height;
   mode_perfect(&wanted_width, &wanted_height);
       
   /* does the game fit at all ? */
   if(width  < (visual_width  * widthscale) ||
      height < (yarbsize ? yarbsize : (visual_height * heightscale)))
      return 0;
   
   return ( 100 *
      ((float)wanted_width  / (abs(width -wanted_width )+wanted_width )) *
      ((float)wanted_height / (abs(height-wanted_height)+wanted_height)));
}
