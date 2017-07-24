#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
/* make ansi compilation happy */
#define inline __inline__
#if defined(__ARCH_freebsd)
#include <machine/cpufunc.h>
#else
#include <asm/io.h>
#endif
#undef inline
#include "twkuser.h"

/*
    readyVgaRegs() does the initialization to make the VGA ready to
	accept any combination of configuration register settings.

	This involves enabling writes to index 0 to 7 of the CRT controller
	(port 0x3d4), by clearing the most significant bit (bit 7) of index
	0x11.
*/

void readyVgaRegs(void)
	{
	int v;

	   /*
	   outportb(0x3d4,0x11);
    v = inportb(0x3d5) & 0x7f;
	outportb(0x3d4,0x11);
	outportb(0x3d5,v);
	   */
	   
	   outb(0x11,0x3d4);
	   v = inb(0x3d5) & 0x7f;
	   outb(0x11,0x3d4);
	   outb(v,0x3d5);
	   
	
	}

/*
	outReg sets a single register according to the contents of the
	passed Register structure.
*/

void outReg(Register r)
	{
	switch (r.port)
		{
		/* First handle special cases: */

		case ATTRCON_ADDR:
			inb(STATUS_ADDR);  		/* reset read/write flip-flop */
			outb(r.index | 0x20,ATTRCON_ADDR);
										/* ensure VGA output is enabled */
			outb(r.value,ATTRCON_ADDR);
			break;

		case MISC_ADDR:
		case VGAENABLE_ADDR:
			outb(r.value,r.port);	/*	directly to the port */
			break;

		case SEQ_ADDR:
		case GRACON_ADDR:
		case CRTC_ADDR:
		default:						/* This is the default method: */
			outb(r.index,r.port);	/*	index to port			   */
			outb(r.value,r.port+1);/*	value to port+1 		   */
			break;
		}
	}


/*
	outRegArray sets n registers according to the array pointed to by r.
	First, indexes 0-7 of the CRT controller are enabled for writing.
*/

void outRegArray(Register *r, int n)
	{
    readyVgaRegs();
	while (n--)
		outReg(*r++);
	}


/*
	loadRegArray opens the given file, does some validity checking, then
	reads the entire file into an array of Registers, which is returned
	via the array parameter.

	You will probably want to provide your own error handling code in
	this function, as it never aborts the program, rather than just
	printing an error message and returning NULL.

	The returned value is the number of Registers read.  The &array
	parameter is set to the allocated Register array.

	If you use this function, remember to free() the returned array
	pointer, as it was allocated dynamically using malloc() (unless NULL
	is returned, which designates an error)!
*/

#if 0
int loadRegArray(char *fpath, RegisterPtr *array)
	{
	int handle, regs;
	long fsize;
	*array = NULL;

	if ((handle = open(fpath, O_BINARY | O_RDONLY)) == -1)
		/* error opening file */
		/* include your error handling code here */
		goto fileerror;

    if ((fsize = filelength(handle)) == -1)
		/* error acquiring file size */
		goto fileerror;
	if (fsize % sizeof(Register))
		{
		fprintf(stderr_file, "Illegal TWEAK file size: %s\n", fpath);
		return 0;
		}
	regs = fsize / sizeof(Register);

    if (!(*array = (Register *)malloc(fsize)))
		{
		fprintf(stderr_file, "Out of memory allocating buffer for %s\n", fpath);
		return 0;
		}
	if (read(handle, (void*)*array, fsize) == -1)
		/* error reading file */
		goto fileerror;

    if (close(handle) == -1)
		{
		/* error closing file */
		goto fileerror;
		}

	/* file read ok, return pointer to buffer */
	return regs;

fileerror:
	perror(fpath);
	if (*array) free(*array);
	return 0;
	}
#endif
