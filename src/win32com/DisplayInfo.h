#pragma once

#include <ddraw.h>

#define DEFAULT_DISPLAY_NAME "(Default)"
#define DISPLAY_ON_TEXT " on "

class CDisplayInfo
{
private:
/************************************************
 * Member Variables
 ***********************************************/
	char* mDriverDescription;
	char* mDriverName;
	char* mFriendlyName;
	GUID mID;
	BOOL mIsDefault;

public:
/************************************************
 * Public Constructors
 ***********************************************/
	CDisplayInfo();
	CDisplayInfo(LPGUID lpGuid, LPSTR lpDriverDescription, LPSTR lpDriverName);
	CDisplayInfo(const CDisplayInfo &ob);

/************************************************
 * Destructors
 ***********************************************/
	~CDisplayInfo(void);

private:
/************************************************
 * Private Methods
 ***********************************************/
	void CopyData(const CDisplayInfo &ob);
	void InitData();

public:
/************************************************
 * Public Properties
 ***********************************************/
	LPCTSTR GetDriverDescription(void);
	LPCTSTR GetDriverName(void);
	LPCTSTR GetFriendlyName(void);
	BOOL GetIsDefault(void);

/************************************************
 * Public Operators
 ***********************************************/
	CDisplayInfo operator=(const CDisplayInfo &orig);
};
