/*********************************************************************

  png.c

  PNG reading functions.

  07/15/1998 Created by Mathis Rosenhauer
  10/02/1998 Code clean up and abstraction by Mike Balfour
             and Mathis Rosenhauer
  10/15/1998 Image filtering. MLR
  11/09/1998 Bit depths 1-8 MLR
  11/10/1998 Some additional PNG chunks recognized MLR
  05/14/1999 Color type 2 and PNG save functions added
  05/15/1999 Handle RGB555 while saving, use mame_fxxx
             functions for writing MSH
  04/27/2001 Simple MNG support MLR

  TODO : Fully comply with the "Recommendations for Decoders"
         of the W3C

*********************************************************************/

#include <math.h>
#include <zlib.h>
#include "driver.h"
#include "png.h"


/* convert_uint is here so we don't have to deal with byte-ordering issues */
static UINT32 convert_from_network_order (UINT8 *v)
{
	UINT32 i;

	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

int png_unfilter(struct png_info *p)
{
	int i, j, bpp, filter;
	INT32 prediction, pA, pB, pC, dA, dB, dC;
	UINT8 *src, *dst;

	if((p->image = (UINT8 *)malloc (p->height*p->rowbytes))==NULL)
	{
		logerror("Out of memory\n");
		free (p->fimage);
		return 0;
	}

	src = p->fimage;
	dst = p->image;
	bpp = p->bpp;

	for (i=0; i<p->height; i++)
	{
		filter = *src++;
		if (!filter)
		{
			memcpy (dst, src, p->rowbytes);
			src += p->rowbytes;
			dst += p->rowbytes;
		}
		else
			for (j=0; j<p->rowbytes; j++)
			{
				pA = (j<bpp) ? 0: *(dst - bpp);
				pB = (i<1) ? 0: *(dst - p->rowbytes);
				pC = ((j<bpp)||(i<1)) ? 0: *(dst - p->rowbytes - bpp);

				switch (filter)
				{
				case PNG_PF_Sub:
					prediction = pA;
					break;
				case PNG_PF_Up:
					prediction = pB;
					break;
				case PNG_PF_Average:
					prediction = ((pA + pB) / 2);
					break;
				case PNG_PF_Paeth:
					prediction = pA + pB - pC;
					dA = abs(prediction - pA);
					dB = abs(prediction - pB);
					dC = abs(prediction - pC);
					if (dA <= dB && dA <= dC) prediction = pA;
					else if (dB <= dC) prediction = pB;
					else prediction = pC;
					break;
				default:
					logerror("Unknown filter type %i\n",filter);
					prediction = 0;
				}
				*dst = 0xff & (*src + prediction);
				dst++; src++;
			}
	}

	free (p->fimage);
	return 1;
}

int png_verify_signature (mame_file *fp)
{
	INT8 signature[8];

	if (mame_fread (fp, signature, 8) != 8)
	{
		logerror("Unable to read PNG signature (EOF)\n");
		return 0;
	}

	if (memcmp(signature, PNG_Signature, 8))
	{
		logerror("PNG signature mismatch found: %s expected: %s\n",signature,PNG_Signature);
		return 0;
	}
	return 1;
}

int png_inflate_image (struct png_info *p)
{
	unsigned long fbuff_size;

	fbuff_size = p->height * (p->rowbytes + 1);

	if((p->fimage = (UINT8 *)malloc (fbuff_size))==NULL)
	{
		logerror("Out of memory\n");
		free (p->zimage);
		return 0;
	}

	if (uncompress(p->fimage, &fbuff_size, p->zimage, p->zlength) != Z_OK)
	{
		logerror("Error while inflating image\n");
		return 0;
	}

	free (p->zimage);
	return 1;
}

int png_read_file(mame_file *fp, struct png_info *p)
{
	/* translates color_type to bytes per pixel */
	const int samples[] = {1, 0, 3, 1, 2, 0, 4};

	UINT32 chunk_length, chunk_type=0, chunk_crc, crc;
	UINT8 *chunk_data, *temp;
	UINT8 str_chunk_type[5], v[4];

	struct idat
	{
		struct idat *next;
		int length;
		UINT8 *data;
	} *ihead, *pidat;

	if ((ihead = malloc (sizeof(struct idat))) == 0)
		return 0;

	pidat = ihead;

	p->zlength = 0;
	p->num_palette = 0;
	p->num_trans = 0;
	p->trans = NULL;
	p->palette = NULL;

	if (png_verify_signature(fp)==0)
		return 0;

	while (chunk_type != PNG_CN_IEND)
	{
		if (mame_fread(fp, v, 4) != 4)
			logerror("Unexpected EOF in PNG\n");
		chunk_length=convert_from_network_order(v);

		if (mame_fread(fp, str_chunk_type, 4) != 4)
			logerror("Unexpected EOF in PNG file\n");

		str_chunk_type[4]=0; /* terminate string */

		crc=crc32(0,str_chunk_type, 4);
		chunk_type = convert_from_network_order(str_chunk_type);

		if (chunk_length)
		{
			if ((chunk_data = (UINT8 *)malloc(chunk_length+1))==NULL)
			{
				logerror("Out of memory\n");
				return 0;
			}
			if (mame_fread (fp, chunk_data, chunk_length) != chunk_length)
			{
				logerror("Unexpected EOF in PNG file\n");
				free(chunk_data);
				return 0;
			}

			crc=crc32(crc,chunk_data, chunk_length);
		}
		else
			chunk_data = NULL;

		if (mame_fread(fp, v, 4) != 4)
			logerror("Unexpected EOF in PNG\n");
		chunk_crc=convert_from_network_order(v);

		if (crc != chunk_crc)
		{
			logerror("CRC check failed while reading PNG chunk %s\n",str_chunk_type);
			logerror("Found: %08X  Expected: %08X\n",crc,chunk_crc);
			return 0;
		}

		logerror("Reading PNG chunk %s\n", str_chunk_type);

		switch (chunk_type)
		{
		case PNG_CN_IHDR:
			p->width = convert_from_network_order(chunk_data);
			p->height = convert_from_network_order(chunk_data+4);
			p->bit_depth = *(chunk_data+8);
			p->color_type = *(chunk_data+9);
			p->compression_method = *(chunk_data+10);
			p->filter_method = *(chunk_data+11);
			p->interlace_method = *(chunk_data+12);
			free (chunk_data);

			logerror("PNG IHDR information:\n");
			logerror("Width: %i, Height: %i\n", p->width, p->height);
			logerror("Bit depth %i, color type: %i\n", p->bit_depth, p->color_type);
			logerror("Compression method: %i, filter: %i, interlace: %i\n",
					p->compression_method, p->filter_method, p->interlace_method);
			break;

		case PNG_CN_PLTE:
			p->num_palette=chunk_length/3;
			p->palette=chunk_data;
			logerror("%i palette entries\n", p->num_palette);
			break;

		case PNG_CN_tRNS:
			p->num_trans=chunk_length;
			p->trans=chunk_data;
			logerror("%i transparent palette entries\n", p->num_trans);
			break;

		case PNG_CN_IDAT:
			pidat->data = chunk_data;
			pidat->length = chunk_length;
			if ((pidat->next = malloc (sizeof(struct idat))) == 0)
				return 0;
			pidat = pidat->next;
			pidat->next = 0;
			p->zlength += chunk_length;
			break;

		case PNG_CN_tEXt:
			{
				char *text = (char *)chunk_data;

				while(*text++) ;
				chunk_data[chunk_length]=0;
 				logerror("Keyword: %s\n", chunk_data);
				logerror("Text: %s\n", text);
			}
			free(chunk_data);
			break;

		case PNG_CN_tIME:
			{
				UINT8 *t=chunk_data;
				logerror("Image last-modification time: %i/%i/%i (%i:%i:%i) GMT\n",
					((short)(*t) << 8)+ (short)(*(t+1)), *(t+2), *(t+3), *(t+4), *(t+5), *(t+6));
			}

			free(chunk_data);
			break;

		case PNG_CN_gAMA:
			p->source_gamma	 = convert_from_network_order(chunk_data)/100000.0;
			logerror( "Source gamma: %f\n",p->source_gamma);

			free(chunk_data);
			break;

		case PNG_CN_pHYs:
			p->xres = convert_from_network_order(chunk_data);
			p->yres = convert_from_network_order(chunk_data+4);
			p->resolution_unit = *(chunk_data+8);
			logerror("Pixel per unit, X axis: %i\n",p->xres);
			logerror("Pixel per unit, Y axis: %i\n",p->yres);
			if (p->resolution_unit)
				logerror("Unit is meter\n");
			else
				logerror("Unit is unknown\n");
			free(chunk_data);
			break;

		case PNG_CN_IEND:
			break;

		default:
			if (chunk_type & 0x20000000)
				logerror("Ignoring ancillary chunk %s\n",str_chunk_type);
			else
				logerror("Ignoring critical chunk %s!\n",str_chunk_type);
			if (chunk_data)
				free(chunk_data);
			break;
		}
	}
	if ((p->zimage = (UINT8 *)malloc(p->zlength))==NULL)
	{
		logerror("Out of memory\n");
		return 0;
	}

	/* combine idat chunks to compressed image data */
	temp = p->zimage;
	while (ihead->next)
	{
		pidat = ihead;
		memcpy (temp, pidat->data, pidat->length);
		free (pidat->data);
		temp += pidat->length;
		ihead = pidat->next;
		free (pidat);
	}
	p->bpp = (samples[p->color_type] * p->bit_depth) / 8;
	p->rowbytes = ceil((p->width * p->bit_depth * samples[p->color_type]) / 8.0);

	if (png_inflate_image(p)==0)
		return 0;

	if(png_unfilter (p)==0)
		return 0;

	return 1;
}

int png_read_info(mame_file *fp, struct png_info *p)
{
	UINT32 chunk_length, chunk_type=0, chunk_crc, crc;
	UINT8 *chunk_data;
	UINT8 str_chunk_type[5], v[4];
	int res = 0;

	if (png_verify_signature(fp)==0)
		return 0;

	while (chunk_type != PNG_CN_IEND)
	{
		if (mame_fread(fp, v, 4) != 4)
			logerror("Unexpected EOF in PNG\n");
		chunk_length=convert_from_network_order(v);

		if (mame_fread(fp, str_chunk_type, 4) != 4)
			logerror("Unexpected EOF in PNG file\n");

		str_chunk_type[4]=0; /* terminate string */

		crc=crc32(0,str_chunk_type, 4);
		chunk_type = convert_from_network_order(str_chunk_type);

		if (chunk_length)
		{
			if ((chunk_data = (UINT8 *)malloc(chunk_length+1))==NULL)
			{
				logerror("Out of memory\n");
				return 0;
			}
			if (mame_fread (fp, chunk_data, chunk_length) != chunk_length)
			{
				logerror("Unexpected EOF in PNG file\n");
				free(chunk_data);
				return 0;
			}

			crc=crc32(crc,chunk_data, chunk_length);
		}
		else
			chunk_data = NULL;

		if (mame_fread(fp, v, 4) != 4)
			logerror("Unexpected EOF in PNG\n");
		chunk_crc=convert_from_network_order(v);

		if (crc != chunk_crc)
		{
			logerror("CRC check failed while reading PNG chunk %s\n",str_chunk_type);
			logerror("Found: %08X  Expected: %08X\n",crc,chunk_crc);
			return 0;
		}

		logerror("Reading PNG chunk %s\n", str_chunk_type);

		switch (chunk_type)
		{
		case PNG_CN_IHDR:
			p->width = convert_from_network_order(chunk_data);
			p->height = convert_from_network_order(chunk_data+4);
			p->bit_depth = *(chunk_data+8);
			p->color_type = *(chunk_data+9);
			p->compression_method = *(chunk_data+10);
			p->filter_method = *(chunk_data+11);
			p->interlace_method = *(chunk_data+12);
			free (chunk_data);

			logerror("PNG IHDR information:\n");
			logerror("Width: %i, Height: %i\n", p->width, p->height);
			logerror("Bit depth %i, color type: %i\n", p->bit_depth, p->color_type);
			logerror("Compression method: %i, filter: %i, interlace: %i\n",
					p->compression_method, p->filter_method, p->interlace_method);
			break;

		case PNG_CN_tEXt:
			{
				char *text = (char *)chunk_data;
				int c;

				while(*text++) ;
				chunk_data[chunk_length]=0;
				if (strcmp ((const char *)chunk_data, "Screen") == 0)
				{
					c = sscanf (text, "%i%i%i%i", &p->screen.min_x, &p->screen.max_x,
								&p->screen.min_y, &p->screen.max_y);
					if (c == 4)
					{
						res = 1;
						logerror("Screen location found at %i, %i, %i, %i\n",
								 p->screen.min_x, p->screen.max_x,
								 p->screen.min_y, p->screen.max_y);
					}
					else
						logerror("Invalid %s value %s\n", chunk_data, text);
				}
			}
			free(chunk_data);
			break;

		default:
			if (chunk_data)
				free(chunk_data);
			break;
		}
	}
	return res;
}

/*	Expands a p->image from p->bit_depth to 8 bit */
int png_expand_buffer_8bit (struct png_info *p)
{
	int i,j, k;
	UINT8 *inp, *outp, *outbuf;

	if (p->bit_depth < 8)
	{
		if ((outbuf = (UINT8 *)malloc(p->width*p->height))==NULL)
		{
			logerror("Out of memory\n");
			return 0;
		}

		inp = p->image;
		outp = outbuf;

		for (i = 0; i < p->height; i++)
		{
			for(j = 0; j < p->width / ( 8 / p->bit_depth); j++)
			{
				for (k = 8 / p->bit_depth-1; k >= 0; k--)
					*outp++ = (*inp >> k * p->bit_depth) & (0xff >> (8 - p->bit_depth));
				inp++;
			}
			if (p->width % (8 / p->bit_depth))
			{
				for (k = p->width % (8 / p->bit_depth)-1; k >= 0; k--)
					*outp++ = (*inp >> k * p->bit_depth) & (0xff >> (8 - p->bit_depth));
				inp++;
			}
		}
		free (p->image);
		p->image = outbuf;
	}
	return 1;
}

void png_delete_unused_colors (struct png_info *p)
{
	int i, tab[256], pen=0, trns=0;
	UINT8 ptemp[3*256], ttemp[256];

	memset (tab, 0, 256*sizeof(int));
	memcpy (ptemp, p->palette, 3*p->num_palette);
	memcpy (ttemp, p->trans, p->num_trans);

	/* check which colors are actually used */
	for (i = 0; i < p->height*p->width; i++)
		tab[p->image[i]]++;

	/* shrink palette and transparency */
	for (i = 0; i < p->num_palette; i++)
		if (tab[i])
		{
			p->palette[3*pen+0]=ptemp[3*i+0];
			p->palette[3*pen+1]=ptemp[3*i+1];
			p->palette[3*pen+2]=ptemp[3*i+2];
			if (i < p->num_trans)
			{
				p->trans[pen] = ttemp[i];
				trns++;
			}
			tab[i] = pen++;
		}

	/* remap colors */
	for (i = 0; i < p->height*p->width; i++)
		p->image[i]=tab[p->image[i]];

	if (p->num_palette!=pen)
		logerror("%i unused pen(s) deleted\n", p->num_palette-pen);

	p->num_palette = pen;
	p->num_trans = trns;
}

/********************************************************************************

  PNG write functions

********************************************************************************/

struct png_text
{
	char *data;
	int length;
	struct png_text *next;
};

static struct png_text *png_text_list = 0;

static void convert_to_network_order (UINT32 i, UINT8 *v)
{
	v[0]=i>>24;
	v[1]=(i>>16)&0xff;
	v[2]=(i>>8)&0xff;
	v[3]=i&0xff;
}

int png_add_text (const char *keyword, const char *text)
{
	struct png_text *pt;

	pt = malloc (sizeof(struct png_text));
	if (pt == 0)
		return 0;

	pt->length = strlen(keyword) + strlen(text) + 1;
	pt->data = malloc (pt->length + 1);
	if (pt->data == 0)
		return 0;

	strcpy (pt->data, keyword);
	strcpy (pt->data + strlen(keyword) + 1, text);
	pt->next = png_text_list;
	png_text_list = pt;

	return 1;
}

static int write_chunk(mame_file *fp, UINT32 chunk_type, UINT8 *chunk_data, UINT32 chunk_length)
{
	UINT32 crc;
	UINT8 v[4];
	int written;

	/* write length */
	convert_to_network_order(chunk_length, v);
	written = mame_fwrite(fp, v, 4);

	/* write type */
	convert_to_network_order(chunk_type, v);
	written += mame_fwrite(fp, v, 4);

	/* calculate crc */
	crc=crc32(0, v, 4);
	if (chunk_length > 0)
	{
		/* write data */
		written += mame_fwrite(fp, chunk_data, chunk_length);
		crc=crc32(crc, chunk_data, chunk_length);
	}
	convert_to_network_order(crc, v);

	/* write crc */
	written += mame_fwrite(fp, v, 4);

	if (written != 3*4+chunk_length)
	{
		logerror("Chunk write failed\n");
		return 0;
	}
	return 1;
}

int png_write_sig(mame_file *fp)
{
	/* PNG Signature */
	if (mame_fwrite(fp, PNG_Signature, 8) != 8)
	{
		logerror("PNG sig write failed\n");
		return 0;
	}
	return 1;
}

int png_write_datastream(mame_file *fp, struct png_info *p)
{
	UINT8 ihdr[13];
	struct png_text *pt;

	/* IHDR */
	convert_to_network_order(p->width, ihdr);
	convert_to_network_order(p->height, ihdr+4);
	*(ihdr+8) = p->bit_depth;
	*(ihdr+9) = p->color_type;
	*(ihdr+10) = p->compression_method;
	*(ihdr+11) = p->filter_method;
	*(ihdr+12) = p->interlace_method;
	logerror("Type(%d) Color Depth(%d)\n", p->color_type,p->bit_depth);
	if (write_chunk(fp, PNG_CN_IHDR, ihdr, 13)==0)
		return 0;

	/* PLTE */
	if (p->num_palette > 0)
		if (write_chunk(fp, PNG_CN_PLTE, p->palette, p->num_palette*3)==0)
			return 0;

	/* IDAT */
	if (write_chunk(fp, PNG_CN_IDAT, p->zimage, p->zlength)==0)
		return 0;

	/* tEXt */
	while (png_text_list)
	{
		pt = png_text_list;
		if (write_chunk(fp, PNG_CN_tEXt, (UINT8 *)pt->data, pt->length)==0)
			return 0;
		free (pt->data);

		png_text_list = pt->next;
		free (pt);
	}

	/* IEND */
	if (write_chunk(fp, PNG_CN_IEND, NULL, 0)==0)
		return 0;

	return 1;
}

int png_filter(struct png_info *p)
{
	int i;
	UINT8 *src, *dst;

	if((p->fimage = (UINT8 *)malloc (p->height*(p->rowbytes+1)))==NULL)
	{
		logerror("Out of memory\n");
		return 0;
	}

	dst = p->fimage;
	src = p->image;

	for (i=0; i<p->height; i++)
	{
		*dst++ = 0; /* No filter */
		memcpy (dst, src, p->rowbytes);
		src += p->rowbytes;
		dst += p->rowbytes;
	}
	return 1;
}

int png_deflate_image(struct png_info *p)
{
	unsigned long zbuff_size;

	zbuff_size = (p->height*(p->rowbytes+1))*1.1+12;

	if((p->zimage = (UINT8 *)malloc (zbuff_size))==NULL)
	{
		logerror("Out of memory\n");
		return 0;
	}

	if (compress(p->zimage, &zbuff_size, p->fimage, p->height*(p->rowbytes+1)) != Z_OK)
	{
		logerror("Error while deflating image\n");
		return 0;
	}
	p->zlength = zbuff_size;

	return 1;
}

static int png_pack_buffer (struct png_info *p)
{
	UINT8 *outp, *inp;
	int i,j,k;

	outp = inp = p->image;

	if (p->bit_depth < 8)
	{
		for (i=0; i<p->height; i++)
		{
			for(j=0; j<p->width/(8/p->bit_depth); j++)
			{
				for (k=8/p->bit_depth-1; k>=0; k--)
					*outp |= *inp++ << k * p->bit_depth;
				outp++;
				*outp = 0;
			}
			if (p->width % (8/p->bit_depth))
			{
				for (k=p->width%(8/p->bit_depth)-1; k>=0; k--)
					*outp |= *inp++ << k * p->bit_depth;
				outp++;
				*outp = 0;
			}
		}
	}
	return 1;
}


/*********************************************************************

  Writes an mame_bitmap in a PNG file. If the depth of the bitmap
  is 8, a color type 3 PNG with palette is written. Otherwise a
  color type 2 true color RGB PNG is written.

 *********************************************************************/

static int png_create_datastream(void *fp, struct mame_bitmap *bitmap)
{
	int i, j;
	int r, g, b;
	UINT32 color;
	UINT8 *ip;
	struct png_info p;

	memset (&p, 0, sizeof (struct png_info));
	p.xscale = p.yscale = p.source_gamma = 0.0;
	p.palette = p.trans = p.image = p.zimage = p.fimage = NULL;
	p.width = bitmap->width;
	p.height = bitmap->height;

	if ((bitmap->depth == 16) && (Machine->drv->total_colors <= 256))
	{
		p.color_type = 3;
		if((p.palette = (UINT8 *)malloc (3*256))==NULL)
		{
			logerror("Out of memory\n");
			return 0;
		}
		memset (p.palette, 0, 3*256);
		/* get palette */
		for (i = 0; i < Machine->drv->total_colors; i++)
			palette_get_color(i,&p.palette[3*i],&p.palette[3*i+1],&p.palette[3*i+2]);

		p.num_palette = 256;
		if((p.image = (UINT8 *)malloc (p.height*p.width))==NULL)
		{
			logerror("Out of memory\n");
			return 0;
		}

		for (i = 0; i < p.height; i++)
			for (j = 0; j < p.width; j++)
				p.image[i * p.width + j] = ((UINT16 *)bitmap->line[i])[j];

		png_delete_unused_colors (&p);
		p.bit_depth = p.num_palette > 16 ? 8 : p.num_palette > 4 ? 4 : p.num_palette > 2 ? 2 : 1;
		p.rowbytes=ceil((p.width*p.bit_depth)/8.0);
		if (png_pack_buffer (&p) == 0)
			return 0;

	}
	else
	{
		p.color_type = 2;
		p.rowbytes = p.width * 3;
		p.bit_depth = 8;
		if((p.image = (UINT8 *)malloc (p.height * p.rowbytes))==NULL)
		{
			logerror("Out of memory\n");
			return 0;
		}

		ip = p.image;

		switch (bitmap->depth)
		{
		case 16: /* 16BIT */
			for (i = 0; i < p.height; i++)
				for (j = 0; j < p.width; j++)
				{
					palette_get_color(((UINT16 *)bitmap->line[i])[j],ip, ip+1, ip+2);
					ip += 3;
				}
			break;
		case 15: /* DIRECT_15BIT */
			for (i = 0; i < p.height; i++)
				for (j = 0; j < p.width; j++)
				{
					color = ((UINT16 *)bitmap->line[i])[j];

					r = (color & direct_rgb_components[0]) / (direct_rgb_components[0] / 0x1f);
					g = (color & direct_rgb_components[1]) / (direct_rgb_components[1] / 0x1f);
					b = (color & direct_rgb_components[2]) / (direct_rgb_components[2] / 0x1f);

					*ip++ = (r << 3) | (r >> 2);
					*ip++ = (g << 3) | (g >> 2);
					*ip++ = (b << 3) | (b >> 2);
				}
			break;
		case 32: /* DIRECT_32BIT */
			for (i = 0; i < p.height; i++)
				for (j = 0; j < p.width; j++)
				{
					color = ((UINT32 *)bitmap->line[i])[j];

					r = (color & direct_rgb_components[0]) / (direct_rgb_components[0] / 0xff);
					g = (color & direct_rgb_components[1]) / (direct_rgb_components[1] / 0xff);
					b = (color & direct_rgb_components[2]) / (direct_rgb_components[2] / 0xff);

					*ip++ = r;
					*ip++ = g;
					*ip++ = b;
				}
			break;
		default:
			logerror("Unknown color depth\n");
			break;
		}
	}

	if(png_filter (&p)==0)
		return 0;

	if (png_deflate_image(&p)==0)
		return 0;

	if (png_write_datastream(fp, &p)==0)
		return 0;

	if (p.palette) free (p.palette);
	if (p.image) free (p.image);
	if (p.zimage) free (p.zimage);
	if (p.fimage) free (p.fimage);
	return 1;
}

int png_write_bitmap(mame_file *fp, struct mame_bitmap *bitmap)
{
	char text[1024];

#ifdef MESS
	sprintf (text, "MESS %s", build_version);
#else
	sprintf (text, "MAME %s", build_version);
#endif
	png_add_text("Software", text);
	sprintf (text, "%s %s", Machine->gamedrv->manufacturer, Machine->gamedrv->description);
	png_add_text("System", text);

	if(png_write_sig(fp) == 0)
		return 0;

	if(png_create_datastream(fp, bitmap) == 0)
		return 0;

	return 1;
}

/********************************************************************************

  MNG write functions

********************************************************************************/

static int mng_status;

int mng_capture_start(mame_file *fp, struct mame_bitmap *bitmap)
{
	UINT8 mhdr[28];
/*	UINT8 term; */

	if (mame_fwrite(fp, MNG_Signature, 8) != 8)
	{
		logerror("MNG sig write failed\n");
		return 0;
	}

	memset (mhdr, 0, 28);
	convert_to_network_order(bitmap->width, mhdr);
	convert_to_network_order(bitmap->height, mhdr+4);
	convert_to_network_order(Machine->drv->frames_per_second, mhdr+8);
	convert_to_network_order(0x0041, mhdr+24); /* Simplicity profile */
	/* frame count and play time unspecified because
	   we don't know at this stage */
	if (write_chunk(fp, MNG_CN_MHDR, mhdr, 28)==0)
		return 0;

/*	term = 0x03;    loop sequence    */
/*	if (write_chunk(fp, MNG_CN_TERM, &term, 1)==0) */
/*		return 0; */

	mng_status = 1;
	return 1;
}

int mng_capture_frame(mame_file *fp, struct mame_bitmap *bitmap)
{
	if (mng_status)
	{
		if(png_create_datastream(fp, bitmap) == 0)
			return 0;
		return 1;
	}
	else
	{
		logerror("MNG recording not running\n");
		return 0;
	}
}

int mng_capture_stop(mame_file *fp)
{
	if (write_chunk(fp, MNG_CN_MEND, NULL, 0)==0)
		return 0;

	mng_status = 0;
	return 1;
}

int mng_capture_status(void)
{
	return mng_status;
}

