/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  YM3812.c

 ***************************************************************************/

#include <conio.h>

/* Borland conio.h dos not include outp and inp, so use ours. */
#if defined(__BORLANDC__)
#include "portio.h"
#endif

#include "mame.h"
#include "mame32.h"
#include "m32util.h"
#include "win32ui.h"
#include "fmsynth.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int      FMSynth_init(options_type *options);
static void     FMSynth_exit(void);
static void     FMSynth_opl_control(int chip, int reg);
static void     FMSynth_opl_write(int chip, int data);

static void     SilenceOpl(void);
static void     WriteOPLRegister(BYTE opl_register,BYTE value);
static void     tenmicrosec(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDFMSynth FMSynth =
{
    FMSynth_init,           /* init           */
    FMSynth_exit,           /* exit           */
    FMSynth_opl_control,    /* opl_control    */
    FMSynth_opl_write,      /* opl_write      */
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static BOOL enabled;
static int num_used_opl;

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static int FMSynth_init(options_type *options)
{
    enabled = options->fm_ym3812 && (Machine->sample_rate != 0);
    num_used_opl = 0;
    return 0;
}

static void FMSynth_exit(void)
{
   if (enabled)
      SilenceOpl();
}

static void FMSynth_opl_control(int chip, int reg)
{
#if defined(_M_IX86)
    if (enabled)
    {
        if (chip >= MAX_OPLCHIP)
            return;

        tenmicrosec();
        _outp((unsigned short)(0x388 + chip * 2), reg);
    }
#endif
}

static void FMSynth_opl_write(int chip, int data)
{
#if defined(_M_IX86)
    if (enabled)
    {
        if (chip >= MAX_OPLCHIP)
            return;

        tenmicrosec();
        _outp((unsigned short)(0x389 + chip * 2), data);

        if (chip >= num_used_opl)
            num_used_opl = chip + 1;
    }
#endif
}

/***************************************************************************
    Internal functions  
 ***************************************************************************/

static void SilenceOpl(void)
{
    int chip, n;
    for (chip = 0; chip < num_used_opl; chip++)
    {
        for (n = 0x40; n <= 0x55; n++)
        {
            FMSynth_opl_control(chip, n);
            FMSynth_opl_write(chip, 0x3f);
        }
        for (n = 0x60; n <= 0x95; n++)
        {
            FMSynth_opl_control(chip, n);
            FMSynth_opl_write(chip, 0xff);
        }
        for (n = 0xa0; n <= 0xb0; n++)
        {
            FMSynth_opl_control(chip, n);
            FMSynth_opl_write(chip, 0);
        }
    }
}

static void WriteOPLRegister(BYTE opl_register, BYTE value)
{
#if defined(_M_IX86)
   int i;

   _outp(0x388, opl_register);
   for (i = 0; i < 6; i++)
      _inp(0x388);

   _outp(0x389, value);

   for (i = 0; i < 35; i++)
      _inp(0x388);
#endif
}

/* linux sound driver opl3.c does a so called tenmicrosec() delay */
static void tenmicrosec(void)
{
#if defined(_M_IX86)
    int i;
    for (i = 0; i < 16; i++)
        _inp(0x80);
#endif
}
