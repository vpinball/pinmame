/******************************************************************************
 ***** MAMEOPL.H --- Auxiliary Windows NT driver for OPL access in MAME32 *****
 ***** ------------------------------------------------------------------ *****
 *****             Windows NT kernel-mode driver definitions              *****
 *****                    written by Robert Schlabbach                    *****
 *****          e-mail: robert@powerstation.isdn.cs.TU-Berlin.DE          *****
 ******************************************************************************/

// NOTE: Make sure you include either NTDDK.H (for kernel-mode driver) or
// ===== WINIOCTL.H (for user-mode application) BEFORE including this file!!!

// define device name in user space
#define	MAMEOPL_DEVICE_NAME	"\\\\.\\MAMEOPL"

// define I/O control codes
#define	IOCTL_MAMEOPL_WRITE_OPL_INDEX_REGISTER \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	IOCTL_MAMEOPL_WRITE_OPL_DATA_REGISTER \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define	IOCTL_MAMEOPL_SILENCE_OPL \
		CTL_CODE(FILE_DEVICE_UNKNOWN, 3, METHOD_BUFFERED, FILE_ANY_ACCESS)
