#include "pindmd.h"

#ifndef PINDMD3

#include <windows.h>
//#include <process.h>

#ifdef PINDMD2
#include "..\..\ext\libusb\include\lusb0_usb.h"
#include "..\..\ext\libusb\lib\dynamic\libusb_dyn.c"

//define vendor id and product id
#define VID 0x0314
#define PID 0xe457
//configure and interface number
//TODO figure out what these mean
#define MY_CONFIG 1
#define MY_INTF 0
//endpoints for communication
#define EP_IN 0x81
#define EP_OUT 0x01
//maximum packet size, 64 for full-speed, 512 for high-speed
#define BUF_SIZE 2052
//gets device handle
usb_dev_handle* open_dev( void );

char writeBuffer[BUF_SIZE];
//context handle for transfer and receive
//void* asyncReadContext = NULL;
void* asyncWriteContext = NULL;

usb_dev_handle *device = NULL;
#else
#include "..\..\ext\ftdi\ftd2xx.h"
#pragma comment(lib, "ext\\ftdi\\i386\\ftd2xx.lib")

FT_STATUS ftStatus;
FT_HANDLE ftHandle;
#endif

UINT8			enabled = 0;
UINT8			do16;
#ifndef PINDMD2
UINT8			doOther;
UINT8			slowUSB = 0;
UINT8			sendFrameCount;
#endif
//UINT8			pinDMD_Version = 1;
//FILE			*f;

void sendFrame(void);
                  
//*****************************************************
//* Name:			pindmdInit
//* Purpose:	initialize ftdi driver
//* In:
//* Out:
//*****************************************************
void pinddrvInit(void)
{
#ifdef PINDMD2
	int ret = 0;
	enabled = 1;
	do16 = 1;
	//f = fopen("debug.txt","w");

	//init usb library
	usb_init();
	//find busses
	usb_find_busses();
	//find devices
	usb_find_devices();

	//try to open our device
	if( !( device = open_dev() ) ) {
		//if not successfull, print error message
		MessageBox(NULL, "PinDMD not found", "Error", MB_ICONERROR);
		enabled = 0;
		return;
	}
	//set configuration
	if( usb_set_configuration( device, MY_CONFIG ) < 0 ) {
		MessageBox(NULL, "PinDMD cannot configure", "Error", MB_ICONERROR);
		enabled = 0;
		usb_close( device );
		return;
	}
	//try to claim interface for use
	if( usb_claim_interface( device, MY_INTF ) < 0 ) {
		MessageBox(NULL, "PinDMD cannot claim interface", "Error", MB_ICONERROR);
		enabled = 0;
		usb_close( device );
		return;
	}
	// SETUP async
	ret = usb_bulk_setup_async( device, &asyncWriteContext, EP_OUT );
	if( ret < 0 ) {
		MessageBox(NULL, "PinDMD cannot setup async", "Error", MB_ICONERROR);
		enabled = 0;
		//relese interface
		usb_release_interface( device, MY_INTF );
		//close device and exit
		usb_close( device );
		return;
	}
	//if(f)
	//	fprintf(f,"all ok:%d \n",enabled);
#else
	FT_DEVICE_LIST_INFO_NODE *devInfo;
	DWORD numDevs;
	UINT8 i;
	UINT8 BitMode;
	UINT8 deviceId = 0;
	sendFrameCount = 0;

	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	enabled = 0;

	// ftdi devices found
	if (numDevs > 0) {
		// allocate storage for list based on numDevs
		devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
		// info request successful
		if (ftStatus == FT_OK) {
			for (i = 0; i < numDevs; i++) {
				// search for pindmd board serial number
				if((strcmp(devInfo[i].SerialNumber,"DMD1000")==0) || (strcmp(devInfo[i].SerialNumber,"DMD1001")==0) || (strcmp(devInfo[i].SerialNumber,"DMD2000A")==0)){
					// assign divice id (incase other ftdi devices are connected)
					deviceId= i;
					enabled = 1;
				}
				// pinDMD 2
				//if(strcmp(devInfo[i].SerialNumber,"DMD2000A")==0)
				//	pinDMD_Version = 2;
				// slow usb
				if(strcmp(devInfo[i].SerialNumber,"DMD1001")==0)
					slowUSB = 1;
			}
		}
	} else {
		enabled=0;
		return;
	}

	// get handle on device
	ftStatus = FT_Open(deviceId, &ftHandle);
	if(ftStatus != FT_OK){
		// FT_Open failed return;
		enabled=0;
		return;
	}
	
	// check pinDMD firmware to see if its 16 colour (bit4=true)
	FT_SetBitMode(ftHandle, 0x0, 0x20);
	ftStatus = FT_GetBitMode(ftHandle, &BitMode);
	if (ftStatus == FT_OK) {
		// BitMode contains current value
		do16	= ((BitMode&0x08)==0x08);
		doOther = ((BitMode&0x04)==0x04);
	}

	// original pinDMD
	//if(pinDMD_Version==1){
		// set Asynchronous Bit Bang Mode
		FT_SetBitMode(ftHandle, 0xff, 0x1);
		// set Baud
		FT_SetBaudRate(ftHandle, slowUSB?11000:12000);  // Actually 10400 * 16
	// new pinDMD 2 FPGA
	/*} else {
			do16 = 1;
      FT_SetBitMode(ftHandle, 0xff, 0);
      Sleep(10);
      FT_SetBitMode(ftHandle, 0xff, 0x40);
      FT_SetLatencyTimer(ftHandle, 16);
      FT_SetUSBParameters(ftHandle,0x10000, 0x10000);
      FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0x0, 0x0);
      FT_Purge(ftHandle, FT_PURGE_RX);
      Sleep(10);
	}*/
#endif
}

//*****************************************************
//* Name:			pinddrvDeInit
//* Purpose:
//* In:
//* Out:
//*****************************************************
void pinddrvDeInit(void)
{
	if(enabled)
	{
#ifdef PINDMD2
		usb_release_interface( device, MY_INTF );
		usb_close( device );
#else
		// have to reset bitbangmode or the ftdi chip will flood the serial with '[00]'
		FT_SetBitMode(ftHandle, 0x00, 0x0);
		FT_Close(ftHandle);
#endif
	}
}
	
//*****************************************************
//* Name:			pinddrvSendFrame
//* Purpose:
//* In:
//* Out:
//*****************************************************
void pinddrvSendFrame(void)
{
	if(enabled)
	{
#ifdef PINDMD2
		sendFrame();
#else
		if(sendFrameCount < 3)
			_beginthread(sendFrame,0,NULL);
#endif
	}
}

void sendFrame(void)
{
	if(enabled)
	{
#ifdef PINDMD2
		usb_submit_async( asyncWriteContext, frame_buf, 2052 );
		usb_reap_async( asyncWriteContext, 5000 );
#else
		DWORD bytes;
		sendFrameCount++;
		// send dmd frame buffer to pindmd board
		ftStatus = FT_Write(ftHandle, &frame_buf, (do16==1)?(DWORD)2052:(DWORD)1028, &bytes);
		sendFrameCount--;
#endif
	}
}

#ifdef PINDMD2
//gets device handle
//@param none
//@return device handle
usb_dev_handle* open_dev( void )
{
	//contains usb busses present on computer
	struct usb_bus *bus;
	//contains devices present on computer
	struct usb_device *dev;
	//used to skip first device
	//bool first = true;
	//loop through busses and devices
	for (bus = usb_get_busses(); bus; bus = bus->next){
		for (dev = bus->devices; dev; dev = dev->next){
			//if device vendor id and product id are match
			if (dev->descriptor.idVendor == VID && dev->descriptor.idProduct == PID)
				return usb_open(dev);
		}
	}
	//return null if no devices were found
	return NULL;
}
#endif
#endif
