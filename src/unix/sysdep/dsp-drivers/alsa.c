/*
 * ALSA Sound Driver for xMAME
 *
 *  Copyright 2000 Luc Saillard <luc.saillard@alcove.fr>
 *  Copyright 2001, 2002, 2003 Shyouzou Sugitani <shy@debian.or.jp>
 *  
 *  This file and the acompanying files in this directory are free software;
 *  you can redistribute them and/or modify them under the terms of the GNU
 *  Library General Public License as published by the Free Software Foundation;
 *  either version 2 of the License, or (at your option) any later version.
 *
 *  These files are distributed in the hope that they will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with these files; see the file COPYING.LIB.  If not,
 *  write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 * Changelog:
 *   v 0.1 Thu, 10 Aug 2000 08:29:00 +0200
 *     - initial release
 *     - TODO: find the best sound card to play sound.
 *   v 0.2 Wed, 13 Sep 2000    Shyouzou Sugitani <shy@debian.or.jp>
 *     - change from block to stream mode.
 *   v 0.3 Sat, 16 Sep 2000    Shyouzou Sugitani <shy@debian.or.jp>
 *     - one important bug fix, performance improvements and code cleanup.
 *   v 0.4 Sun, 15 Apr 2001    Shyouzou Sugitani <shy@debian.or.jp>
 *     - minor cosmetic changes.
 *     - suppression of bogus warnings about underruns.
 *     - TODO: add support for ALSA 0.9 API.
 *   v 0.5 Thu, 17 May 2001    Shyouzou Sugitani <shy@debian.or.jp>
 *     - added preliminary support for ALSA 0.9 API.
 *     - split of the 0.5 and 0.9 API stuff into separate files.
 *   v 0.6 Sat, 19 May 2001    Shyouzou Sugitani <shy@debian.or.jp>
 *     - update of the 0.9 API stuff.
 *       added -list-alsa-pcm option.
 *       improved write error handling.
 *   v 0.7 Sat, 08 Sep 2001    Shyouzou Sugitani <shy@debian.or.jp>
 *     - update of the 0.9 API stuff.
 *       added -alsa-buffer option.
 *       use SND_PCM_FORMAT_S16 instead of SND_PCM_FORMAT_S16_{LE,BE}.
 *   v 0.8 Thu, 13 Sep 2001    Shyouzou Sugitani <shy@debian.or.jp>
 *     - update of the 0.9 API stuff.
 *       changed the -alsapcm(-pcm) to -alsa-pcm(-apcm).
 *       changed the default value of the -alsa-pcm.
 *   V 0.8a                    Stephen Anthony
 *     - Fixed a problem in the ALSA 0.9 driver with setting the sample rate
 *       on SB128 soundcards.
 *   V 0.9 Mon, 13 Jan 2003    Shyouzou Sugitani <shy@debian.or.jp>
 *     - removed the 0.5 API support.
 *   V 1.0 Wed, 21 Jan 2004    Shyouzou Sugitani <shy@debian.or.jp>
 *     - Update to the ALSA 1.0 API.
 *   V 1.1 (for LISY) June 2017    bontango <bontango@lisy80.com>
 *     - Updated to latest version replacing depreciated functions
 *
 */

#include "xmame.h"           /* xMAME common header */
#include "devices.h"         /* xMAME device header */

#ifdef SYSDEP_DSP_ALSA

#define ALSA_PCM_NEW_HW_PARAMS_API 1

#include <sys/ioctl.h>       /* System and I/O control */
#include <alsa/asoundlib.h>  /* ALSA sound library header */
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct alsa_dsp_priv_data
{
	snd_pcm_t *pcm_handle;
	size_t bits_per_frame;
};

/* public methods prototypes (static but exported through the sysdep_dsp or
   plugin struct) */
static int alsa_dsp_init(void);
static void *alsa_dsp_create(const void *flags);
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp);
static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
			  int count);
static int alsa_device_list(struct rc_option *option, const char *arg,
			    int priority);
static int alsa_pcm_list(struct rc_option *option, const char *arg,
			 int priority);

/* private variables */
static unsigned int buffer_time;
static int alsa_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;
static int resample = 1;                                /* enable alsa-lib resampling */

/* public variables */
struct rc_option alsa_dsp_opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Alsa Sound System", NULL,     rc_seperator, NULL,
	  NULL,    0,      0,    NULL,
	  NULL },
	{ "list-alsa-cards", NULL,	rc_use_function_no_arg, NULL,
	  NULL,    0,      0,    alsa_device_list,
	  "List available sound cards" },
	{ "list-alsa-pcm", NULL,	rc_use_function_no_arg, NULL,
	  NULL,    0,      0,    alsa_pcm_list,
	  "List available pcm devices" },
	{ "alsa-buffer", "abuf",  rc_int,       &buffer_time,
	  "250000",      0,    0,    NULL,
	  "Set the buffer size [micro sec] (default: 250000)" },
	{ NULL,    NULL,     rc_end,   NULL,
	  NULL,    0,      0,    NULL,
	  NULL }
};

const struct plugin_struct sysdep_dsp_alsa = {
	"alsa",
	"sysdep_dsp",
	"Alsa Sound System DSP plugin",
	alsa_dsp_opts,
	alsa_dsp_init,
	NULL, /* no exit */
	alsa_dsp_create,
	4     /* high priority */
};


/* public methods (static but exported through the sysdep_dsp or plugin
   struct) */

/*
 * Function name : alsa_dsp_init
 *
 * Description : Detect if a card is present on the machine
 * Output :
 *   a boolean
 */
static int alsa_dsp_init(void)
{
	int card = -1;
	
	if (snd_card_next(&card) < 0 || card < 0) {
		fprintf(stderr, "No cards detected.\n"
			"ALSA sound disabled.\n");
		return 1;
	}
	return 0;
}

/*
 * Function name : alsa_dsp_create
 *
 * Description : Create an instance of dsp plugins
 * Input :
 *   flags: a ptr to struct sysdep_dsp_create_params
 * Output :
 *   a ptr to a struct sysdep_dsp_struct
 */
static void *alsa_dsp_create(const void *flags)
{
	int err, dir;
	struct alsa_dsp_priv_data *priv;
	struct sysdep_dsp_struct *dsp;
	snd_pcm_info_t *info;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t buffer_size;
	snd_pcm_uframes_t period_size;
	snd_pcm_uframes_t chunk_size;
      
static snd_output_t *output = NULL;
 snd_pcm_uframes_t size;

        unsigned int btime                            = buffer_time;
	const struct sysdep_dsp_create_params *params = flags;
        const char *device                            = params->device;
//        snd_pcm_uframes_t period_frames               = 1;
static unsigned int period_time = 100000;               /* period time in us */
	/* rate >= 2000 && rate <= 128000 */
	unsigned int rate       = params->samplerate;
	unsigned int channels   = (params->type & SYSDEP_DSP_STEREO) ? 2 : 1;
	snd_pcm_format_t format = (params->type & SYSDEP_DSP_16BIT) ?
		SND_PCM_FORMAT_S16 /* Signed 16 bit CPU endian */ :
		SND_PCM_FORMAT_U8;
	unsigned int rrate;

	/* allocate the dsp struct */
	dsp = calloc(1, sizeof(struct sysdep_dsp_struct));
	if (!dsp) {
		fprintf(stderr,
			"error malloc failed for struct sysdep_dsp_struct\n");
		return NULL;
	}

	/* alloc private data */
	priv = calloc(1, sizeof(struct alsa_dsp_priv_data));
	if(!priv) {
		fprintf(stderr,
			"error malloc failed for struct alsa_dsp_priv_data\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	}
	
	if (!device)
		device = "default";

	err = snd_pcm_open(&priv->pcm_handle, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (err < 0) {
		fprintf(stderr_file, "Alsa error: audio open error: %s\n",
			snd_strerror(err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	snd_pcm_info_alloca(&info);
	err = snd_pcm_info(priv->pcm_handle, info);
	if (err < 0) {
		fprintf(stderr_file, "Alsa error: info error: %s\n",
			snd_strerror(err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	/* set non-blocking mode if selected */
	if (params->flags & SYSDEP_DSP_O_NONBLOCK) {
		err = snd_pcm_nonblock(priv->pcm_handle, 1);
		if (err < 0) {
			fprintf(stderr_file,
				"Alsa error: nonblock setting error: %s\n",
				snd_strerror(err));
			alsa_dsp_destroy(dsp);
			return NULL;
		}
	}

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);
	/* choose all parameters */
	err = snd_pcm_hw_params_any(priv->pcm_handle, hw_params);
	if (err < 0) {
		fprintf(stderr_file,
			"Alsa error: no configurations available\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	} 
        /* set hardware resampling */
        err = snd_pcm_hw_params_set_rate_resample(priv->pcm_handle, hw_params, resample);
        if (err < 0) {
                printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
                return NULL;
        }

	/* set the interleaved read/write format */
	if (snd_pcm_hw_params_set_access(priv->pcm_handle, hw_params,
					 SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		fprintf(stderr_file,
			"Alsa error: interleaved access mode not available\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	
}
	/* set the sample format */
	err = snd_pcm_hw_params_set_format(priv->pcm_handle, hw_params, format);
	if (err < 0) {
		fprintf(stderr_file,
			"Alsa error: requested format %s isn't supported with hardware\n",
			snd_pcm_format_name(format));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	/* set the count of channels */
	err = snd_pcm_hw_params_set_channels(priv->pcm_handle, hw_params, channels);
	if (err < 0) {
		fprintf(stderr_file,
			"Alsa error: channels count non available\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	}

/*
	if (snd_pcm_hw_params_set_rate_near(priv->pcm_handle, hw_params, &rate, 0) < 0) {
		unsigned int min, max;

		snd_pcm_hw_params_get_rate_min(hw_params, &min, 0);
		snd_pcm_hw_params_get_rate_max(hw_params, &max, 0);
		fprintf(stderr_file,
			"Alsa error: unsupported rate %uHz (valid range is %uHz-%iHz)\n",
			rate, min, max);
		alsa_dsp_destroy(dsp);
		return NULL;
	}
*/
        /* set the stream rate */
        rrate = rate;
        err = snd_pcm_hw_params_set_rate_near(priv->pcm_handle, hw_params, &rrate, 0);
        if (err < 0) {
                printf("Rate %iHz not available for playback: %s\n", rate, snd_strerror(err));
                return NULL;
        }
        if (rrate != rate) {
                printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
                return NULL;
        }

	/* set the buffer time */
	err = snd_pcm_hw_params_set_buffer_time_near(priv->pcm_handle, hw_params, &btime, 0);
        if (err < 0) {
                printf("Unable to set buffer time %i for playback: %s\n", buffer_time, snd_strerror(err));
                return NULL;
        }
        err = snd_pcm_hw_params_get_buffer_size(hw_params, &size);
        if (err < 0) {
                printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
                return NULL;
        }
        buffer_size = size;
	/* set the period time */
	err = snd_pcm_hw_params_set_period_time_near(priv->pcm_handle, hw_params, &period_time, &dir);
        if (err < 0) {
                printf("Unable to set period time %i for playback: %s\n", period_time, snd_strerror(err));
                return NULL;
        }
        err = snd_pcm_hw_params_get_period_size(hw_params, &size, &dir);
        if (err < 0) {
                printf("Unable to get period size for playback: %s\n", snd_strerror(err));
                return NULL;
        }
        period_size = size;

        /* write the parameters to device */
	err = snd_pcm_hw_params(priv->pcm_handle, hw_params);
	if (err < 0) {
		fprintf(stderr_file,
			"Alsa error: Unable to install hw params\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	}


//do we need that stuff below?
/*
	snd_pcm_hw_params_get_period_size(hw_params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	if (chunk_size == buffer_size) {
		fprintf(stderr_file,
			"Alsa error: cannot use period equal to buffer size (%lu == %lu)\n",
			chunk_size, buffer_size);
		alsa_dsp_destroy(dsp);
		return NULL;
	}
*/

// set_swparams from the example on alsa-project.org

        /* get the current swparams */
	err = snd_pcm_sw_params_current(priv->pcm_handle, sw_params);
        if (err < 0) {
                printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
                return NULL;
        }

        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(priv->pcm_handle, sw_params, (buffer_size / period_size) * period_size);
        if (err < 0) {
                printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return NULL;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        //err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_event ? buffer_size : period_size);
        err = snd_pcm_sw_params_set_avail_min(priv->pcm_handle, sw_params, period_size);
        if (err < 0) {
                printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
                return NULL;
        }

	// depreciated snd_pcm_sw_params_set_sleep_min(priv->pcm_handle, sw_params, 0);
	// depreciated snd_pcm_sw_params_set_xfer_align(priv->pcm_handle, sw_params, 1);
	//snd_pcm_sw_params_set_avail_min(priv->pcm_handle, sw_params, 1);
	//snd_pcm_sw_params_set_start_threshold(priv->pcm_handle, sw_params, 1);
	//snd_pcm_sw_params_set_stop_threshold(priv->pcm_handle, sw_params, buffer_size);

	/* write the parameters to the playback device */
	if (snd_pcm_sw_params(priv->pcm_handle, sw_params) < 0) {
		fprintf(stderr_file, "Alsa error: unable to install sw params\n");
		alsa_dsp_destroy(dsp);
		return NULL;
	}

#if 0 /* DEBUG */
	{
		snd_output_t *log;
		snd_output_stdio_attach(&log, stderr_file, 0);
		snd_pcm_dump(priv->pcm_handle, log);
		snd_output_close(log);
	}
#endif

	err = snd_pcm_prepare(priv->pcm_handle);
	if (err < 0) {
		fprintf(stderr_file,
			"Alsa error: unable to prepare audio: %s\n",
			snd_strerror(err));
		alsa_dsp_destroy(dsp);
		return NULL;
	}

	/* fill in the functions and some data */
	priv->bits_per_frame = snd_pcm_format_physical_width(format) * channels;
	dsp->_priv = priv;
	dsp->get_freespace = alsa_dsp_get_freespace;
	dsp->write = alsa_dsp_write;
	dsp->destroy = alsa_dsp_destroy;
	dsp->hw_info.type = params->type;
	dsp->hw_info.samplerate = rate;
	dsp->hw_info.bufsize = 0;

	fprintf(stderr_file, "info: set to %dbit linear %s %dHz\n",
		(dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8,
		(dsp->hw_info.type & SYSDEP_DSP_STEREO) ? "stereo" : "mono",
		dsp->hw_info.samplerate);

/*
        err = snd_output_stdio_attach(&output, stdout, 0);
        if (err < 0) {
                printf("Output failed: %s\n", snd_strerror(err));
                return 0;
        }

 snd_pcm_dump(priv->pcm_handle, output);
*/

	return dsp;
}

/*
 * Function name : alsa_dsp_destroy
 *
 * Description :
 * Input :
 * Output :
 */
static void alsa_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
	struct alsa_dsp_priv_data *priv = dsp->_priv;

	if (priv) {
		if (priv->pcm_handle) {
			snd_pcm_close(priv->pcm_handle);
		}
		free(priv);
	}
	free(dsp);
}

/*
 * Function name : alsa_dsp_get_freespace
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_dsp_get_freespace(struct sysdep_dsp_struct *dsp)
{
	int i;
	struct alsa_dsp_priv_data *priv = dsp->_priv;
	snd_pcm_status_t *status;
	snd_pcm_uframes_t frames;

	snd_pcm_status_alloca(&status);
	i = snd_pcm_status(priv->pcm_handle, status);
	if (i < 0) {
		fprintf(stderr_file, "Alsa error: status error: %s\n",
			snd_strerror(i));
		return -1;
	}
	frames = snd_pcm_status_get_avail(status);
	if (frames < 0)
		return -1;
		
	i = frames * priv->bits_per_frame / 8
		/ alsa_dsp_bytes_per_sample[dsp->hw_info.type];
	
	return i;
}

/*
 * Function name : alsa_dsp_write
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data,
			  int count)
{
	int data_size, result;
	struct alsa_dsp_priv_data *priv = dsp->_priv;
	
	data_size = count * alsa_dsp_bytes_per_sample[dsp->hw_info.type]
		* 8 / priv->bits_per_frame;

	result = snd_pcm_writei(priv->pcm_handle, data, data_size);
	if (result == -EAGAIN) {
		return 0;
	} else if (result == -EPIPE) {
		int err;
		snd_pcm_status_t *status;

		snd_pcm_status_alloca(&status);
		err = snd_pcm_status(priv->pcm_handle, status);
		if (err < 0) {
			fprintf(stderr_file,
				"Alsa error: status error: %s\n",
				snd_strerror(err));
			return -1;
		}
		if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
			err = snd_pcm_prepare(priv->pcm_handle);
			if (err < 0) {
				fprintf(stderr_file,
					"Alsa error: prepare error: %s\n",
					snd_strerror(err));
				return -1;
			}
			/* ok, data should be accepted again */
			return 0;
		}
		fprintf(stderr_file, "Alsa error: write error: %s\n",
			snd_strerror(result));
		return -1;
	} else if (result < 0) {
		fprintf(stderr_file, "Alsa error: write error: %s\n",
			snd_strerror(result));
		return -1;
	}

	return result * priv->bits_per_frame / 8
		/ alsa_dsp_bytes_per_sample[dsp->hw_info.type];
}

/*
 * Function name : alsa_device_list
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_device_list(struct rc_option *option, const char *arg,
			    int priority)
{
	snd_ctl_t *handle;
	int card, err, dev;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		printf("Alsa: no soundcards found...\n");
		return -1;
	}
	fprintf(stdout, "Alsa cards:\n");
	while (card >= 0) {
		char name[32];
		sprintf(name, "hw:%d", card);
		err = snd_ctl_open(&handle, name, 0);
		if (err < 0) {
			fprintf(stderr, "Alsa error: control open (%i): %s\n",
				card, snd_strerror(err));
			continue;
		}
		err = snd_ctl_card_info(handle, info);
		if (err < 0) {
			fprintf(stderr,
				"Alsa error: control hardware info (%i): %s\n",
				card, snd_strerror(err));
			snd_ctl_close(handle);
			continue;
		}
		dev = -1;
		while (1) {
			int idx;
			unsigned int count;

			if (snd_ctl_pcm_next_device(handle, &dev) < 0)
				;
			if (dev < 0)
				break;
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
			err = snd_ctl_pcm_info(handle, pcminfo);
			if (err < 0) {
				if (err != -ENOENT)
					fprintf(stderr,
						"Alsa error: control digital audio info (%i): %s\n",
						card, snd_strerror(err));
				continue;
			}
			fprintf(stderr,
				"card %i: %s [%s], device %i: %s [%s]\n",
				card,
				snd_ctl_card_info_get_id(info),
				snd_ctl_card_info_get_name(info),
				dev,
				snd_pcm_info_get_id(pcminfo),
				snd_pcm_info_get_name(pcminfo));
			count = snd_pcm_info_get_subdevices_count(pcminfo);
			fprintf(stderr, "  Subdevices: %i/%i\n",
				snd_pcm_info_get_subdevices_avail(pcminfo),
				count);
			for (idx = 0; idx < count; idx++) {
				snd_pcm_info_set_subdevice(pcminfo, idx);
				err = snd_ctl_pcm_info(handle, pcminfo);
				if (err < 0) {
					fprintf(stderr,
						"Alsa error: control digital audio playback info (%i): %s",
						card, snd_strerror(err));
				} else {
					fprintf(stderr,
						"  Subdevice #%i: %s\n",
						idx, snd_pcm_info_get_subdevice_name(pcminfo));
				}
			}
		}
		snd_ctl_close(handle);
		if (snd_card_next(&card) < 0) {
			break;
		}
	}
	return -1;
}

/*
 * Function name : alsa_pcm_list
 *
 * Description :
 * Input :
 * Output :
 */
static int alsa_pcm_list(struct rc_option *option, const char *arg,
			 int priority)
{
        snd_config_t *conf;
        snd_output_t *out;

        int err;

	err = snd_config_update();
        if (err < 0) {
		fprintf(stderr, "Alsa error: snd_config_update: %s\n",
			snd_strerror(err));
                return -1;
        }
        snd_output_stdio_attach(&out, stderr, 0);
        err = snd_config_search(snd_config, "pcm", &conf);
        if (err < 0)
                return -1;
        fprintf(stderr, "ALSA PCM devices:\n");
        snd_config_save(conf, out);
        snd_output_close(out);

	return -1;
}

#endif /* SYSDEP_DSP_ALSA */
