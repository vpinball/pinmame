/***************************************************************************

  M.A.M.E.32  -  Multiple Arcade Machine Emulator for Win32
  Win32 Portions Copyright (C) 1997-2001 Michael Soderstrom and Chris Kirmse

  This file is part of MAME32, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

  NTFMSynth.c written by Robert Schlabbach

 ***************************************************************************/

#include <conio.h>

#include "mame.h"
#include "mame32.h"
#include "m32util.h"
#include "win32ui.h"
#include "fmsynth.h"

#include <windows.h>
#include <winioctl.h>
#include "MAMEOPL.H"
#include "NTFMSynth.h"

/***************************************************************************
    function prototypes
 ***************************************************************************/

static int      NTFMSynth_init(options_type *options);
static void     NTFMSynth_exit(void);
static void     NTFMSynth_opl_control(int chip, int reg);
static void     NTFMSynth_opl_write(int chip, int data);

static void     SilenceOpl(void);
static BOOL     LoadDriver(void);
static void     UnLoadDriver(void);

/***************************************************************************
    External variables
 ***************************************************************************/

struct OSDFMSynth NTFMSynth =
{
    NTFMSynth_init,             /* init          */
    NTFMSynth_exit,             /* exit          */
    NTFMSynth_opl_control,      /* opl_control   */
    NTFMSynth_opl_write,        /* opl_write     */
};

/***************************************************************************
    Internal variables
 ***************************************************************************/

static BOOL enabled = FALSE;
static BOOL bDriverLoaded = FALSE;
static int num_used_opl;


static HANDLE hMAMEOPL;
static DWORD  BytesReturned;
static TCHAR  ServiceName[] = TEXT("MAMEOPL");
static TCHAR  DisplayName[] = TEXT("Auxiliary MAME32 OPL Access Driver");
static TCHAR  DevFileName[] = TEXT(MAMEOPL_DEVICE_NAME);

/***************************************************************************
    External OSD functions  
 ***************************************************************************/

static int NTFMSynth_init(options_type *options)
{
    /* initialize enabled flag */
    enabled = FALSE;

    num_used_opl = 0;

    bDriverLoaded = FALSE;

    if (options->fm_ym3812 && (Machine->sample_rate != 0))
    {
        /* get a handle to the installed driver */
        hMAMEOPL = CreateFile
        (
            DevFileName,
            0, 0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hMAMEOPL == INVALID_HANDLE_VALUE)
        {
            /* Dynamically load driver, if possible, and try again. */
            bDriverLoaded = LoadDriver();
            if (bDriverLoaded == TRUE)
            {
                /* get a handle to the loaded driver */
                hMAMEOPL = CreateFile
                (
                    DevFileName,
                    0, 0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );

                if (hMAMEOPL == INVALID_HANDLE_VALUE)
                {
                    UnLoadDriver();
                    bDriverLoaded = FALSE;
                }
            }
        }

        /* if this succeeded, we made it!!! */
        if (hMAMEOPL != INVALID_HANDLE_VALUE)
        {
            enabled = TRUE;
            SilenceOpl();
        }
    };

    return 0;
}

static void NTFMSynth_exit(void)
{
    if (enabled)
    {
        /* silence the OPL */
        SilenceOpl();

        /* close the handle to the driver */
        CloseHandle(hMAMEOPL);

        if (bDriverLoaded == TRUE)
        {
            UnLoadDriver();
            bDriverLoaded = FALSE;
        }

        /* clear enabled flag */
        enabled = FALSE;
    };
}

static void NTFMSynth_opl_control(int chip, int reg)
{
    if (enabled)
    {
        UCHAR   cBuffer[2];

        if (chip >= MAX_OPLCHIP)
            return;

        cBuffer[0] = reg;
        cBuffer[1] = chip;

        DeviceIoControl
        (
            hMAMEOPL,
            IOCTL_MAMEOPL_WRITE_OPL_INDEX_REGISTER,
            cBuffer, 2,
            NULL, 0,
            &BytesReturned,
            NULL
        );
    }
}

static void NTFMSynth_opl_write(int chip, int data)
{
    if (enabled)
    {
        UCHAR   cBuffer[2];

        if (chip >= MAX_OPLCHIP)
            return;

        cBuffer[0] = data;
        cBuffer[1] = chip;

        DeviceIoControl
        (
            hMAMEOPL,
            IOCTL_MAMEOPL_WRITE_OPL_DATA_REGISTER,
            cBuffer, 2,
            NULL, 0,
            &BytesReturned,
            NULL
        );

        if (chip >= num_used_opl)
            num_used_opl = chip + 1;
    }
}

static void SilenceOpl(void)
{
    int chip, n;
    for (chip = 0; chip < num_used_opl; chip++)
    {
        for (n = 0x40; n <= 0x55; n++)
        {
            NTFMSynth_opl_control(chip, n);
            NTFMSynth_opl_write(chip, 0x3f);
        }
        for (n = 0x60; n <= 0x95; n++)
        {
            NTFMSynth_opl_control(chip, n);
            NTFMSynth_opl_write(chip, 0xff);
        }
        for (n = 0xa0; n <= 0xb0; n++)
        {
            NTFMSynth_opl_control(chip, n);
            NTFMSynth_opl_write(chip, 0);
        }
    }
}

static BOOL LoadDriver(void)
{
    /* declare local variables */
    SC_HANDLE   SCManager;
    SC_HANDLE   SCService;
    TCHAR       BinaryPathName[MAX_PATH];
    int         StringPosition;
    BOOL        bLoaded = FALSE;

    /* try to open service control manager on local machine */
    SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (SCManager)
    {
        /* get the full path and filename of this EXE */
        BinaryPathName[0] = 0;
        GetModuleFileName(NULL, BinaryPathName, MAX_PATH);

        /* chop the EXE filename off the string */
        StringPosition = lstrlen(BinaryPathName);
        while ((--StringPosition) && (BinaryPathName[StringPosition] != TEXT('\\')));
        BinaryPathName[StringPosition + 1] = 0;

        /* append driver filename to string */
        lstrcat(BinaryPathName, TEXT("MAMEOPL.SYS"));

        /* try to create the MAME OPL driver service */
        SCService = CreateService
        (
            SCManager,                  /* service control manager handle */
            ServiceName,                /* name of service to install */
            DisplayName,                /* service display name */
            SERVICE_ALL_ACCESS,         /* desired access */
            SERVICE_KERNEL_DRIVER,      /* service type */
            SERVICE_DEMAND_START,       /* start type (manual) */
            SERVICE_ERROR_NORMAL,       /* error control type */
            BinaryPathName,             /* service binary filename */
            NULL,                       /* no load ordering group */
            NULL,                       /* no tag identifier */
            NULL,                       /* no dependencies */
            NULL,                       /* LocalSystem account */
            NULL                        /* no password */
        );

        /* check if creation succeeded or service already exists */
        if ((SCService) || (GetLastError() == ERROR_SERVICE_EXISTS))
        {
            /* if we have a handle, close it for now */
            if (SCService) CloseServiceHandle(SCService);

            /* open the driver service */
            SCService = OpenService(SCManager, ServiceName, SERVICE_ALL_ACCESS);
            if (SCService)
            {
                /* try to load the driver */
                if (StartService(SCService, 0, NULL) || (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING))
                {
                    bLoaded = TRUE;
                }
                else
                {
                    /* driver load failed, remove the service */
                    DeleteService(SCService);
                    bLoaded = FALSE;
                };

                /* close the driver service handle */
                CloseServiceHandle(SCService);
            };
        };

        /* close the service control manager */
        CloseServiceHandle(SCManager);
    };

    return bLoaded;
}

static void UnLoadDriver(void)
{
    /* declare local variables */
    SC_HANDLE       SCManager;
    SC_HANDLE       SCService;
    SERVICE_STATUS  SrvStatus;

    /* try to open service control manager on local machine */
    SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (SCManager)
    {
        /* open the driver service */
        SCService = OpenService(SCManager, ServiceName, SERVICE_ALL_ACCESS);
        if (SCService)
        {
            /* unload the driver and delete the service */
            if (ControlService(SCService, SERVICE_CONTROL_STOP, &SrvStatus))
                DeleteService(SCService);
            CloseServiceHandle(SCService);
        };

        /* close the service control manager */
        CloseServiceHandle(SCManager);
    };
}
