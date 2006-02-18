#include "StdAfx.h"
#include ".\displayinfo.h"

#include <Windows.h>

// Helper methods
void CopyString(char** dest, char* src)
{
	if (src == NULL)
	{
		*dest = NULL;
	}
	else
	{
		*dest = new char[strlen(src) + 1];
		strcpy(*dest, src);
	}
}

/************************************************
 * Public Constructors
 ***********************************************/
CDisplayInfo::CDisplayInfo()
{
	InitData();
}

CDisplayInfo::CDisplayInfo(LPGUID lpGuid, LPSTR lpDriverDescription, LPSTR lpDriverName)
{
	InitData();

	// Copy the GUID if found
	if (lpGuid)
	{
		memcpy(&mID, lpGuid, sizeof(GUID));
		mIsDefault = FALSE;
	}
	else
	{
		mID = GUID(GUID_NULL);
		mIsDefault = TRUE;
	}

	// Store description
	CopyString(&mDriverDescription, lpDriverDescription);

	// Store name
	CopyString(&mDriverName, lpDriverName);

	// Get friendly name
	if (mIsDefault)
	{
		CopyString(&mFriendlyName, DEFAULT_DISPLAY_NAME);
	}
	else if ((mDriverName != NULL) && (mDriverDescription != NULL))
	{
		// Get lengths of name and description
		size_t nameLen = strlen(mDriverName);
		size_t onLen = strlen(DISPLAY_ON_TEXT);
		size_t descLen = strlen(mDriverDescription);
		size_t friendlyLen = nameLen + onLen + descLen + 1;

		// Pointer to the start of the name
		char* nameStart = mDriverName;

		// Is it the standard device notation of \\.\?
		if (strncmp("\\\\.\\", mDriverName, 4) == 0)
		{
			// Update to skip the notation
			nameLen -= 4;
			friendlyLen -= 4;
			nameStart += 4;
		}

		// Allocate
		mFriendlyName = new char[friendlyLen];
		
		// Pointer for text insertion
		char* insertion = mFriendlyName;

		// Build friendly name
		strcpy(insertion, nameStart);
		strcpy((insertion += nameLen), DISPLAY_ON_TEXT);
		strcpy((insertion += onLen), mDriverDescription);
	}
}

// Copy Constructor
CDisplayInfo::CDisplayInfo(const CDisplayInfo &ob)
{
	CopyData(ob);
}

/************************************************
 * Destructors
 ***********************************************/
CDisplayInfo::~CDisplayInfo(void)
{
	if (mDriverDescription != NULL) delete mDriverDescription;
	if (mDriverName != NULL) delete mDriverName;
	if (mFriendlyName != NULL) delete mFriendlyName;
}

/************************************************
 * Private Methods
 ***********************************************/
void CDisplayInfo::CopyData(const CDisplayInfo &ob)
{
	CopyString(&mDriverDescription, ob.mDriverDescription);
	CopyString(&mDriverName, ob.mDriverName);
	CopyString(&mFriendlyName, ob.mFriendlyName);
	mID = ob.mID;
	mIsDefault = ob.mIsDefault;
}

void CDisplayInfo::InitData()
{
	mDriverDescription = NULL;
	mDriverName = NULL;
	mFriendlyName = NULL;
	mID = GUID(GUID_NULL);
	BOOL mIsDefault = TRUE;
}

/************************************************
 * Public Properties
 ***********************************************/
// Gets the description for the driver
LPCTSTR CDisplayInfo::GetDriverDescription(void)
{
	return (LPCTSTR)mDriverDescription;
}

// Gets the name of the driver
LPCTSTR CDisplayInfo::GetDriverName(void)
{
	return (LPCTSTR)mDriverName;
}

// Gets the friendly name for the display
LPCTSTR CDisplayInfo::GetFriendlyName(void)
{
	return (LPCTSTR)mFriendlyName;
}

// Gets a value that indicates if the display is the primary windows display
BOOL CDisplayInfo::GetIsDefault(void)
{
	return mIsDefault;
}

/************************************************
 * Public Operators
 ***********************************************/
CDisplayInfo CDisplayInfo::operator=(const CDisplayInfo &ob)
{
	CopyData(ob);

    return *this;
}

