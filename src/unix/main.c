/*
 * X-mame main-routine
 */

#define __MAIN_C_
#include "xmame.h"

#ifdef __QNXNTO__
#include <sys/mman.h>
#endif

#if defined(LISY_SUPPORT)
 #include "lisy/lisy80.h"
 #include "lisy/lisy1.h"
 #include "lisy/lisyversion.h"
#endif

/* From video.c. */
void osd_video_initpre();

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	/* now invoice system-dependent initialization */
#ifdef XMAME_NET
	if (osd_net_init()      !=OSD_OK) return OSD_NOT_OK;
#endif	
	if (osd_input_initpre() !=OSD_OK) return OSD_NOT_OK;

	return OSD_OK;
}

/*
 * Cleanup routines to be executed when the program is terminated.
 */
void osd_exit(void)
{
#ifdef XMAME_NET
	osd_net_close();
#endif
	osd_input_close();
}


int main(int argc, char **argv)
{
	int res, res2;

#ifdef __QNXNTO__
	printf("info: Trying to enable swapfile.... ");
	munlockall();
	printf("Success!\n");
#endif

#if defined(LISY_SUPPORT)
        //we may want to give back software version only and exit afterwartds
        if (strcmp(argv[1],"-lisyversion") == 0)
           {
            //show up on calling terminal
            fprintf(stdout,"Version %02d.%03d\n",LISY_SOFTWARE_MAIN,LISY_SOFTWARE_SUB);

            //give back
            return(LISY_SOFTWARE_SUB);
           }

        char lisy_gamename[20];

        //LISY80
        //get the gamename from DIP Switch on LISY80 in case gamename (last arg) is 'lisy80'
        if ( strcmp(argv[argc-1],"lisy80") == 0)
        {
            //do init of LISY80 hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy80_hw_init();
                if ( (res=lisy80_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(argv[argc-1],lisy_gamename);
                   fprintf(stderr,"LISY80: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
                   fprintf(stderr,"LISY80: no matching game or other error\n\r");
        }
	
	//LISY1
        //get the gamename from DIP Switch on LISY1 in case gamename (last arg) is 'lisy1'
        if ( strcmp(argv[argc-1],"lisy1") == 0)
        {
            //do init of LISY80 hardware first, as we need to read dip switch from the board to identify game to emulate
            lisy80_hw_init();
                if ( (res=lisy1_get_gamename(lisy_gamename)) >= 0)
                  {
                   strcpy(argv[argc-1],lisy_gamename);
                   fprintf(stderr,"LISY1: we are emulating Game No:%d %s\n\r",res,lisy_gamename);
                  }
                else
                   fprintf(stderr,"LISY1: no matching game or other error\n\r");
        }
#endif


	/* some display methods need to do some stuff with root rights */
	res2 = sysdep_init();

	/* to be absolutely safe force giving up root rights here in case
	   a display method doesn't */
	if (setuid(getuid()))
	{
		perror("setuid");
		sysdep_close();
		return OSD_NOT_OK;
	}

	/* Set the title, now auto build from defines from the makefile */
	sprintf(title,"%s (%s) version %s", NAME, DISPLAY_METHOD,
			build_version);

	/* parse configuration file and environment */
	if ((res = config_init(argc, argv)) != 1234 || res2 == OSD_NOT_OK)
		goto leave;

	/* Check the colordepth we're requesting */
	if (!options.color_depth && !sysdep_display_16bpp_capable())
		options.color_depth = 8;

	/* 
	 * Initialize whatever is needed before the display is actually 
	 * opened, e.g., artwork setup.
	 */
	osd_video_initpre();

	/* go for it */
	res = run_game (game_index);

leave:
	sysdep_close();
	/* should be done last since this also closes stdout and stderr */
	config_exit();

	return res;
}
