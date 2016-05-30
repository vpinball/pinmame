/* Sysdep Wave File Output sound dsp driver

   Written by Donald King.
   This file is placed in the Public Domain.
*/

/* Changelog
   Version 0.1, September 2002
   - Initial release
*/

#ifdef SYSDEP_DSP_WAVEOUT

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sysdep/sysdep_dsp.h"
#include "sysdep/sysdep_dsp_priv.h"
#include "sysdep/plugin_manager.h"

struct waveout_dsp_priv_data {
   const char *name;
   FILE *file;
};

static void *waveout_dsp_create(const void *flags);
static void waveout_dsp_destroy(struct sysdep_dsp_struct *dsp);
static int waveout_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data, int count);

const struct plugin_struct sysdep_dsp_waveout = {
   "waveout",
   "sysdep_dsp",
   "Wave File Output DSP plugin",
   NULL,				/* no options */
   NULL,				/* no init */
   NULL,				/* no exit */
   waveout_dsp_create,
   0					/* very low priority */
};

static int waveout_dsp_bytes_per_sample[4] = SYSDEP_DSP_BYTES_PER_SAMPLE;

static const unsigned char wave_template[44] = {
  'R', 'I', 'F', 'F',	/*  0: 'RIFF' magic */
    0,   0,   0,   0,	/*  4: 'RIFF' length */
  'W', 'A', 'V', 'E',	/*  8: 'RIFF' type */
  'f', 'm', 't', ' ',	/* 12: 'fmt ' chunk-type */
   16,   0,   0,   0,	/* 16: 'fmt ' chunk-length */
    1,   0,		/* 20: WAVE_FORMAT_PCM */
    1,   0,		/* 22: Channels */
    0,   0,   0,   0,	/* 24: Samples per second */
    0,   0,   0,   0,	/* 28: Bytes per second */
    1,   0,		/* 32: Aligned bytes per sample group */
    8,   0,		/* 34: Bits per sample */
  'd', 'a', 't', 'a',	/* 36: 'data' chunk-type */
    0,   0,   0,   0,	/* 40: 'data' chunk-length */
};

static void *waveout_dsp_create(const void *flags)
{
  const struct sysdep_dsp_create_params *params;
  struct sysdep_dsp_struct *dsp;
  struct waveout_dsp_priv_data *priv;
  unsigned char *header;
  unsigned long tmp;

  params = flags;

  /* Allocate DSP structure */
  if (!(dsp = calloc(1, sizeof(struct sysdep_dsp_struct))))
  {
    fprintf(stderr, "error: failed to malloc struct sysdep_dsp_struct\n");
    return NULL;
  }

  /* Allocate private storage */
  if (!(priv = calloc(1, sizeof(struct waveout_dsp_priv_data))))
  {
    fprintf(stderr, "error: failed to malloc struct waveout_dsp_priv_data\n");
    free(dsp);
    return NULL;
  }

  /* Allocate temporary storage for RIFF WAVE header */
  if (!(header = malloc(44)))
  {
    fprintf(stderr, "error: failed to malloc WAVE header storage\n");
    free(priv);
    free(dsp);
    return NULL;
  }

  /* Fill in DSP structure */
  dsp->hw_info.type = params->type;
  dsp->hw_info.samplerate = params->samplerate;
  dsp->write = waveout_dsp_write;
  dsp->destroy = waveout_dsp_destroy;
  dsp->_priv = priv;

  /* Fill in private storage */
  priv->name = params->device;
  if (!priv->name)
    priv->name = "xmameout.wav";

  /* Compute RIFF WAVE header */
  memcpy(header, wave_template, 44);
  tmp = dsp->hw_info.samplerate;
  if (dsp->hw_info.type & SYSDEP_DSP_STEREO)
    header[22] = 2;
  header[24] = (unsigned char)tmp;
  header[25] = (unsigned char)(tmp >>  8);
  header[26] = (unsigned char)(tmp >> 16);
  header[27] = (unsigned char)(tmp >> 24);
  tmp *= waveout_dsp_bytes_per_sample[dsp->hw_info.type];
  header[28] = (unsigned char)tmp;
  header[29] = (unsigned char)(tmp >>  8);
  header[30] = (unsigned char)(tmp >> 16);
  header[31] = (unsigned char)(tmp >> 24);
  header[32] = waveout_dsp_bytes_per_sample[dsp->hw_info.type];
  if (dsp->hw_info.type & SYSDEP_DSP_16BIT)
    header[34] = 16;

  /* Open output file */
  if (!(priv->file = fopen(priv->name, "wb")))
  {
    fprintf(stderr, "error: failed to open \"%s\" for writing: %s\n", priv->name, strerror(errno));
    free(header);
    free(priv);
    free(dsp);
    return NULL;
  }

  /* Write computed header to disk */
  if (fwrite(header, 1, 44, priv->file) != 44)
  {
    fprintf(stderr, "error: failed to write WAVE header to \"%s\": %s\n", priv->name, strerror(errno));
    fclose(priv->file);
    free(header);
    free(priv);
    free(dsp);
  }

  /* Free the header */
  free(header);

  fprintf(stderr, "info: Writing sound output to file \"%s\" in %dHz/%d/%c PCM format\n",
  	priv->name,
  	dsp->hw_info.samplerate,
  	(dsp->hw_info.type & SYSDEP_DSP_16BIT) ? 16 : 8,
  	(dsp->hw_info.type & SYSDEP_DSP_STEREO) ? 'S' : 'M'
  );

  return dsp;
}

static void waveout_dsp_destroy(struct sysdep_dsp_struct *dsp)
{
  struct waveout_dsp_priv_data *priv = dsp->_priv;

  if (priv)
  {
    if (priv->file)
    {
      unsigned char tmp[4];
      long pos;

      /* Find out where we are in the file */
      pos = ftell(priv->file);

      /* Update the RIFF file length */
      pos -= 8L;
      tmp[0] = (unsigned char)pos;
      tmp[1] = (unsigned char)(pos >>  8);
      tmp[2] = (unsigned char)(pos >> 16);
      tmp[3] = (unsigned char)(pos >> 24);
      fseek(priv->file, 4L, SEEK_SET);
      fwrite(tmp, 4, 1, priv->file);

      /* Update the data chunk length */
      pos -= 36L;
      tmp[0] = (unsigned char)pos;
      tmp[1] = (unsigned char)(pos >>  8);
      tmp[2] = (unsigned char)(pos >> 16);
      tmp[3] = (unsigned char)(pos >> 24);
      fseek(priv->file, 40L, SEEK_SET);
      fwrite(tmp, 4, 1, priv->file);

      /* All done, so close the file */
      fclose(priv->file);
    }
    free(priv);
  }
  free(dsp);
}

static int waveout_dsp_write(struct sysdep_dsp_struct *dsp, unsigned char *data, int count)
{
  if(count > 0)
  {
    struct waveout_dsp_priv_data *priv;
    size_t result;

    priv = dsp->_priv;

    /* FIXME: This produces invalid PCM WAVE files on big-endian systems */
    result = fwrite(data, waveout_dsp_bytes_per_sample[dsp->hw_info.type], count, priv->file);

    if (result != count)
      fprintf(stderr, "error: failed to write %d samples to \"%s\": %s\n", count, priv->name, strerror(errno));
    return result;
  }
  return -1;
}

#endif /* SYSDEP_DSP_WAVEOUT */
