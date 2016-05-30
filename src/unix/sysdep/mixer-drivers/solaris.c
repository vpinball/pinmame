/* Sysdep Open Sound System sound mixer driver

   Copyright 2000 Hans de Goede, Mathis Rosenhauer
   
   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
/* Changelog
Version 0.1, February 2000
-initial release based on oss.c by Hans de Goede (Mathis Rosenhauer)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/audioio.h>
#include "sysdep/sysdep_mixer.h"
#include "sysdep/sysdep_mixer_priv.h"
#include "sysdep/plugin_manager.h"

/* our per instance private data struct */
struct sol_mixer_priv_data {
	int fd;
};

/* public methods prototypes (static but exported through the sysdep_mixer or
   plugin struct) */
static void *sol_mixer_create(const void *flags);
static void sol_mixer_destroy(struct sysdep_mixer_struct *mixer);
static int sol_mixer_set(struct sysdep_mixer_struct *mixer,
						 int channel, int left, int right);
static int sol_mixer_get(struct sysdep_mixer_struct *mixer,
						 int channel, int *left, int *right);
   
/* public variables */
const struct plugin_struct sysdep_mixer_solaris = {
	"solaris",
	"sysdep_mixer",
	"Solaris mixer plugin",
	NULL, /* no options */
	NULL, /* no init */
	NULL, /* no exit */
	sol_mixer_create,
	3	  /* high priority */
};

/* public methods (static but exported through the sysdep_mixer or plugin
   struct) */
static void *sol_mixer_create(const void *flags)
{
	struct sol_mixer_priv_data *priv = NULL;
	struct sysdep_mixer_struct *mixer = NULL;
	const struct sysdep_mixer_create_params *params = flags;
	const char *device = params->device;
   
	/* allocate the mixer struct */
	if (!(mixer = calloc(1, sizeof(struct sysdep_mixer_struct))))
	{
		perror("error malloc failed for struct sysdep_mixer_struct\n");
		return NULL;
	}
   
	/* alloc private data */
	if(!(priv = calloc(1, sizeof(struct sol_mixer_priv_data))))
	{
		perror("error malloc failed for struct sol_mixer_priv_data\n");
		sol_mixer_destroy(mixer);
		return NULL;
	}
   
	/* fill in the functions and some data */
	priv->fd = -1;
	mixer->_priv = priv;
	mixer->set = sol_mixer_set;
	mixer->get = sol_mixer_get;
	mixer->destroy = sol_mixer_destroy;
   
	/* open the mixer device */
	if (!device)
		device = getenv("AUDIODEV");
	if (!device)
		device = "/dev/audioctl";
   
	if((priv->fd = open(device, O_RDWR)) < 0) {
		fprintf(stderr, "error: opening %s\n", device);
		sol_mixer_destroy(mixer);
		return NULL;
	}
   
	/* are there more channels on other hardware as dbri? */
	mixer->channel_available[SYSDEP_MIXER_PCM1] = 1;
	return mixer;
}

static void sol_mixer_destroy(struct sysdep_mixer_struct *mixer)
{
	struct sol_mixer_priv_data *priv = mixer->_priv;
   
	if(priv)
	{
		if(priv->fd >= 0)
			close(priv->fd);
	  
		free(priv);
	}
	free(mixer);
}
   
static int sol_mixer_set(struct sysdep_mixer_struct *mixer, int channel, int left, int right)
{
	audio_info_t info;
	struct sol_mixer_priv_data *priv = mixer->_priv;
	int maxvol = (left > right)? left: right;

	AUDIO_INITINFO(&info);

	info.play.gain = (maxvol * (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN)) / 100 + AUDIO_MIN_GAIN;
	if (maxvol)
	{
		info.play.balance = (right - left) * (AUDIO_RIGHT_BALANCE - AUDIO_LEFT_BALANCE);
		info.play.balance /= 2 * maxvol;
		info.play.balance += AUDIO_MID_BALANCE;
	}

	if (ioctl(priv->fd, AUDIO_SETINFO, &info) < 0)
	{
		perror("error: AUDIO_SETINFO\n");
		return -1;
	}
	return 0;
}

static int sol_mixer_get(struct sysdep_mixer_struct *mixer, int channel, int *left, int *right)
{
	audio_info_t info;
	struct sol_mixer_priv_data *priv = mixer->_priv;

	if (ioctl(priv->fd, AUDIO_GETINFO, &info) < 0)
	{
		perror("error: AUDIO_GETINFO\n");
		return -1;
	}
	
	if (info.play.balance > AUDIO_MID_BALANCE)
	{
		*right = ((info.play.gain - AUDIO_MIN_GAIN) * 100) / (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN);
		*left = *right * ((AUDIO_RIGHT_BALANCE - (float)info.play.balance) / AUDIO_MID_BALANCE);
	}
	else
	{
		*left = ((info.play.gain - AUDIO_MIN_GAIN) * 100) / (AUDIO_MAX_GAIN - AUDIO_MIN_GAIN);
		*right = *left * ((float)info.play.balance / AUDIO_MID_BALANCE);
	}
	return 0;
}

