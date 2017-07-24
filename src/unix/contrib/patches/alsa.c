/*
 * ALSA Sound Driver for xMAME
 */

/* Written by an anonymous programmer at wwtk@mail.com
   and based on the OSS sound driver for xMAME written
   by Mike Oliphant (oliphant@ling.ed.ac.uk) */

/* CURRENT PROGRESS

   10/22/1999 - v.0.1.1 for xMAME released.
                The soundcard scanner and initialization has been improved
                considerably with the addition of soundcard prioritization
                and automatic scaling.  It works like this.  When a PCM
                channel is found, it compares its abilities with the ones you
                want.  The lower the priority number, the closer a fit you
                have (0 means perfect fit).  Naturally, the list is now ordered
                by priority first and then card/channel order.

                When the initialization begins, it runs down the list in
                order of priority, first trying the perfect fits, then the ones
                with different bits, then the ones with different rates, then the
                mono ones if stereo is desired, finishing the job if it can.

                Hans de Goede has accepted my work and is adding it to the
                contrib section of the xMAME source.  Whether it will make it to
                the actual source tree will depend on the progress of xMAME itself.
                In the meantime, you can still use this as a replacement oss.c.
                Just remember to add -lasound to the linker when you do that!

                Pending word on how override options can be added, I'm seeing if
                there are advantages in ALSA's non-block mode.  If there are, I
                may try to include them in here and provide something better than
                the basic code I started out with.

   10/19/1999 - v.0.0.1 for xMAME released.  INITIAL RELEASE
                After examining the relatively simple OSS code,
                I decided to see if I could convert the routines
                to use ALSA instead.  This is the result.

                Functionality is pretty much similar.  Nothing is lost
                by using the ALSA driver in Linux instead of OSS.
                Whether this ALSA driver gives a performance boost or
                not remains to be seen.

                Though I have no way of proving it, any sound card
                supported by ALSA and capable of PCM output should run
                this driver without any trouble.  My own system, a K6-2/300
                with an SB AWE64, runs fine.

                Because of the ALSA implementation, I have included an
                autoscanner that scans the system for ALSA PCM channels.
                Once it finds an open one, it will open the channel and set
                it up for output.  Once that's done, it acts just like the OSS
                driver with the exception of using ALSA calls.

                Currently, automatic sample and bitrate scaledown are not
                incorporated.  These will be added once I can figure out a
                way to take them into account in the autoscan while prioritizing
                better cards.  An autoscan override also needs to be included.
                However, this would require additional changes beyond this code
                piece.

                Some users may experience unusual sound quality.  This is because
                my approach to finding the right frag_size and num_frags is very
                experimental.  The formulae I'm using work for my machine, but
                your performance may vary.
*/

#include "xmame.h"           /* xMAME common header */
#include "sound.h"           /* xMAME sound header */
#include "devices.h"         /* xMAME device header */
#include <sys/ioctl.h>       /* System and I/O control */
#include <sys/asoundlib.h>   /* ALSA sound library header */

/* SCANNING CONSTANTS */
#define MAX_CARDS 8          /* Maximum number of cards to detect */
#define MAX_PCMS  8          /* Maximum number of PCM channels per card to detect */
#define MAX_CHOICES 16       /* Maximum number of choices to queue */


/* ERROR CHECKING MACROS */
#define ALSA_SCAN_CHECK(f) alsa_stat=f;if (alsa_stat) {ALSA_error(alsa_stat);return 0;}
#define ALSA_INIT_CHECK(f) alsa_stat=f;if (alsa_stat) return ALSA_error(alsa_stat);

/* PRIORITIZING MACROS */
#define RANGE(x,y,z) ((x<y) ? (x-y) : (x>z) ? (x-z) : 0)

/* TYPE DEFINITIONS */
typedef struct
{
    int card,pcm,priority,rate,bits,channels;
} choice;

/* GLOBAL VARIABLES */
int alsa_stat;                     /* ALSA error status */
static snd_pcm_t * handle_pcm;     /* ALSA sound PCM handler */

/* ALSA ERROR HANDLER */
int ALSA_error(int a)
{
    fprintf(stderr_file,"\nALSA system error: %s\nALSA sound disabled.\n",snd_strerror(a));
    play_sound=FALSE;
    return 0;
}

/* PRIORITIZATION FUNCTIONS */

int prioritize_level3(int *bits,const int b_flags)
{
    /* Check if correct bits are specified */
    if (!(*bits & b_flags))
    {
        *bits=b_flags;                     /* Change bits to correct */
        return 1;                          /* Bits had to be corrected */
    }
    return 0;                              /* Bits are already correct */
}

int prioritize_level2(int *rate,int *bits, const int min,const int max,const int b_flags)
{
    /* Given rate is in range */
    if (!RANGE(*rate,min,max))
        return prioritize_level3(bits,b_flags);

    /* Given rate is too high.  Must reduce. */
    if (RANGE(*rate,min,max)>0)
    {
        *rate = max;
        return 2+prioritize_level3(bits,b_flags);
    }

    /* Given rate is too small.  Must increase. */
    *rate = min;
    return 4+prioritize_level3(bits,b_flags);
}

int prioritize(int *rate,int *bits,int *channels,
               const int min,const int max,const int b_flags,const int max_chan)
{
    /* Not enough channels supported (ie mono device, stereo option). */
    /* Must adjust. */
    if (max_chan<(*channels))
    {
        *channels=max_chan;
        return 6+prioritize_level2(rate,bits,min,max,b_flags);
    }

    /* Otherwise, device is capable of handling channels */
    return prioritize_level2(rate,bits,min,max,b_flags);
}

int insert(choice list[],int *list_max,int card,int pcm,int rate,int bits,int channels,
           const int min,const int max,const int b_flags,const int max_chan)
{
    choice temp;
    /* Obtain priority and correct if necessary */
    int priority=prioritize(&rate,&bits,&channels,min,max,b_flags,max_chan);
    int i=0,j,lmax=*list_max;
    
    while ((i<lmax) && (list[i++].priority<=priority))
	;

    /* If i is at MAX_CHOICES, the list is full of channels with higher or equal priority. */
    /* Ignore addition and return. */
    if (i==MAX_CHOICES)
        return lmax;

    /* At this point, an addition WILL be made, so fill in the necessary information for the */
    /* new entry. */
    
    temp.card=card;
    temp.pcm=pcm;
    temp.priority=priority;
    temp.rate=rate;
    temp.bits=bits;
    temp.channels=channels;

    /* If i is between lmax and MAX_CHOICES, there is room at the end of the list */
    /* for the new entry.  Add the entry and return adjusted values. */
    if (i==lmax)
    {
        list[i]=temp;
	(*list_max)++;
        return lmax+1;
    }
    
    /* If i is less than lmax, then the entry supercedes others in the list. */
    /* They'll have to be bumped down so the new entry can be inserted. */
    j=(lmax==MAX_CHOICES) ? lmax : lmax+1;
    while (j>i)
    {
        list[j]=list[j-1];j--;
    }
    list[j]=temp;
    if (lmax==MAX_CHOICES)
        return lmax;
    (*list_max)++;
    return lmax+1;
}

/* ALSA sound channel scanner */
int scan_cards(choice list[])
{

    /* ALSA Control Handle */
    static snd_ctl_t * handle_ctl;       /* ALSA sound control handler */

    /* ALSA information variables */
    snd_ctl_hw_info_t info;              /* Holds current card settings */ 
    snd_pcm_info_t id;                   /* Holds current PCM ID */ 
    snd_pcm_playback_info_t temp;        /* Holds current PCM settings */ 

    /* Detection counters */
    int max_cards;                       /* Maximum ALSA cards detected */
    int cur_card;                        /* Current ALSA card in use */
    int cur_pcm;                         /* Current ALSA PCM channel in use */
    int num_choices=0;                   /* Number of valid choices found */
    int bit_mask;                        /* Mask for available bitwidth */

    /* Modified option specifiers */
    int voices = (sound_stereo) ? 2 : 1; /* Is stereo or mono desired? */
    int bits = (sound_8bit)
     ? SND_PCM_PINFO_8BITONLY
     : SND_PCM_PINFO_16BITONLY;          /* Are 8 or 16 bits desired? */

    /* STEP 1: DETERMINE # OF CARDS AVAILABLE */

    fprintf(stderr_file,"Detecting ALSA sound cards... ");
    alsa_stat=snd_cards();
    /* If no sound is found, disable sound */
    if (!alsa_stat)
    {
        fprintf(stderr_file,"No cards detected.\nALSA sound disabled.\n");
        play_sound = FALSE;
        return OSD_OK;
    }
    if (alsa_stat<0)
    {
        ALSA_error(alsa_stat);return 0;
    }
    fprintf(stderr_file,"%d detected OK.\n",alsa_stat);

    /* Cards above 8 ignored */
    max_cards=(alsa_stat>8) ? 8 : alsa_stat;

    /* STEP 2: GATHER PCM INFORMATION ABOUT DETECTED CARDS */
    for (cur_card=0;cur_card<max_cards;cur_card++)
    {
        /* Open control handle to soundcard for information gathering */
        ALSA_SCAN_CHECK(snd_ctl_open(&handle_ctl,cur_card));

        /* Obtain card info (particularly number of PCM channels) */
        ALSA_SCAN_CHECK(snd_ctl_hw_info(handle_ctl,&info));

        /* Scan PCM channels for capabilities */
        for (cur_pcm=0;cur_pcm<info.pcmdevs;cur_pcm++)
        {
            ALSA_SCAN_CHECK(snd_ctl_pcm_info(handle_ctl,cur_pcm,&id));
            ALSA_SCAN_CHECK(snd_ctl_pcm_playback_info(handle_ctl,cur_pcm,0,&temp));

            /* Is the channel capable of playback? */
            if (id.flags & SND_PCM_INFO_PLAYBACK)
            {
                /* Adjust flags */
                bit_mask = temp.flags & (SND_PCM_PINFO_8BITONLY | SND_PCM_PINFO_16BITONLY);
                if (!bit_mask)
                    bit_mask = (SND_PCM_PINFO_8BITONLY | SND_PCM_PINFO_16BITONLY);
                
                /* Insert item into list via prioritizing insertion function. */
                insert(list,&num_choices,cur_card,cur_pcm,options.samplerate,
                       bits,voices,temp.min_rate,temp.max_rate,bit_mask,temp.max_channels);

                /* Relay information to console */
                fprintf(stderr_file,"%s detected, %d-%dHz, ",id.name,temp.min_rate,temp.max_rate);
                switch (temp.flags)
                {
                    case SND_PCM_PINFO_8BITONLY:  fprintf(stderr_file,"8 bit ");break;
                    case SND_PCM_PINFO_16BITONLY: fprintf(stderr_file,"16 bit ");break;
                    default:                      fprintf(stderr_file,"8/16 bit ");
                 };
                fprintf(stderr_file,"%s.\n",(temp.max_channels==1) ? "mono" : "stereo");
            }
        } /* End PCM for */
        
        /* Close handle to prepare for next iteration */
        ALSA_SCAN_CHECK(snd_ctl_close(handle_ctl));

    } /* End card for */

    /* Check to see if a usable channel WAS found */
    if (!num_choices)
    {
        fprintf(stderr_file,"No usable PCM channels found for your settings.");
        fprintf(stderr_file,"\nALSA sound disabled.\n");
        return 0;
    }

    return num_choices;
}

/* Initialize audio system */
int sysdep_audio_init(void)
{
    /* Initialization variables */
    choice list[MAX_CHOICES];            /* Holds list of channels to try */
    int num_choices;                     /* Amount of elements in list */
    snd_pcm_playback_info_t temp;        /* Holds current PCM settings */ 
    snd_pcm_playback_params_t par;       /* Holds current PCM parameters */ 
    snd_pcm_format_t fmt;                /* Sound format structure */
    int i;                               /* Index counter */

    /* Variables held from OSS driver */
    int corrected_frag_size;

    /* Check if sound is enabled */
    if (!play_sound) return OSD_OK;

    /* Prior to initializing, obtain corrected numbers */
    corrected_frag_size = (options.samplerate * frag_size) / 22050;
    if (!sound_8bit) corrected_frag_size += corrected_frag_size;
    if (sound_stereo) corrected_frag_size += corrected_frag_size;

    /* Inform user of ALSA initialization */
    fprintf(stderr_file,"ALSA Sound initialization...\n");

    /* Transfer control to card scanner */
    num_choices=scan_cards(list);
    if (!num_choices)
        return OSD_OK;


    /* Begin sound initialization */
    fprintf(stderr_file,"Opening PCM channel... ");
    i=-1;num_choices--;
    do	
    {
        i++;
        /* Open handle to PCM channel */
        alsa_stat=snd_pcm_open(&handle_pcm,list[i].card,list[i].pcm,SND_PCM_OPEN_PLAYBACK);
    } while (alsa_stat && i<num_choices);

    /* Failed to open ANY channel? */
    if (alsa_stat)
    {
        fprintf(stderr_file,"FAILED.");
        return ALSA_error(alsa_stat);
    }

    /* Initialize sound format structure */
    fmt.format=(list[i].bits==SND_PCM_PINFO_8BITONLY)
               ? SND_PCM_SFMT_U8 : SND_PCM_SFMT_S16_LE;
    fmt.rate=list[i].rate;
    fmt.channels=list[i].channels;

    fprintf(stderr_file,"OK.\nObtaining parameters... ");

    /* Prepare sound parameters */
    /* NOTE: Setting frag_size and num_frags NOT REALLY WORKING YET. */
    /* Attemtping to discover better algorithm given ALSA stats. */
    memset(&par,0,sizeof(par));
    ALSA_INIT_CHECK(snd_pcm_playback_info(handle_pcm,&temp));
    frag_size = (temp.max_fragment_size<corrected_frag_size)
                ? temp.max_fragment_size : corrected_frag_size;
/*    frag_size = 2048; */

    /* EXPERIMENTAL.  Basically dividing buffer capacity by 8. */
    num_frags = (temp.buffer_size/frag_size)>>3;
/*    num_frags = 4;  */

    /* Set parameters and pass onto PCM channel */
    par.fragment_size=frag_size;
    par.fragments_max=num_frags;
    par.fragments_room=1;
    ALSA_INIT_CHECK(snd_pcm_playback_params(handle_pcm,&par));
    fprintf(stderr_file,"%d fragments of size %d... ",
            num_frags,frag_size);

    fprintf(stderr_file,"OK.\nSetting sound format... %dHz, %s %s... ",
            fmt.rate,(sound_8bit) ? "8-bit" : "16-bit",
            (fmt.channels>1) ? "stereo" : "mono");
    ALSA_INIT_CHECK(snd_pcm_playback_format(handle_pcm,&fmt));
    fprintf(stderr_file,"OK.\nALSA initialization complete.\n");
    return OSD_OK;
    
} /* End sysdep_audio_init() */

void sysdep_audio_close(void)
{
    if (play_sound)
    {
        snd_pcm_drain_playback(handle_pcm);
        snd_pcm_close(handle_pcm);
    }
}

long sysdep_audio_get_freespace()
{	
    int test;
    snd_pcm_playback_status_t stats;

    test=snd_pcm_playback_status(handle_pcm,&stats);

    if (test)
        return -1;
    else
        return stats.count;
}

int sysdep_audio_play(unsigned char *buf,int bufsize)
{
    return (int) snd_pcm_write(handle_pcm,(const void *)buf,(size_t)bufsize);
}
