/************************************************************
 * Simple File Encryption/Decryption class using Win32api   *
 * (uses Microsoft's CryptoApi)								*
 *															*
 * by Steve Ellenoff (sellenoff@hotmail.com)	            *
 * 05/29/2002									            *
 ************************************************************/

#include "Crypto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CCrypto::CCrypto(void)
{
	this->hSource = NULL;
	this->hDestination = NULL; 	
	this->pbBuffer = NULL;
	this->hKey = 0; 	
	this->hHash = 0; 
	this->hCryptProv = 0;
}

CCrypto::~CCrypto()
{
}
//////////////////////////////////////////////////////////////////////
// Handle Errors
//////////////////////////////////////////////////////////////////////
void CCrypto::HandleError(LPCSTR szString)
{
	ShowError();
	MessageBox(GetActiveWindow(),szString,"Error!",MB_ICONERROR);
}

//////////////////////////////////////////////////////////////////////
// Encryption/Decryption Methods (Public)
//////////////////////////////////////////////////////////////////////
BOOL CCrypto::DecryptFile(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination)
{
	return CCrypto::CryptIt(szSource,szKey, szDestination, FALSE);
}
BOOL CCrypto::EncryptFile(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination)
{
	return CCrypto::CryptIt(szSource,szKey, szDestination, TRUE);
}

//////////////////////////////////////////////////////////////////////
// CryptIT:
//
//The heart of the work.. copied from the Platform SDK example code (modified slightly)
//////////////////////////////////////////////////////////////////////
BOOL CCrypto::CryptIt(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination, BOOL bEncrypt)
{
DWORD dwBlockLen; 
DWORD dwBufferLen; 
DWORD dwCount; 
char szMessage[MAX_PATH];
 
//--------------------------------------------------------------------
// Open source file. 
if((CCrypto::hSource = fopen(szSource,"rb"))==NULL)
{ 
   wsprintf(szMessage,"Error opening source file: %s!",szSource);
   CCrypto::HandleError(szMessage);
   CCrypto::CloseHandles();
   return FALSE;
} 
//--------------------------------------------------------------------
// Open destination file. 
if((CCrypto::hDestination = fopen(szDestination,"wb"))==NULL)
{
    wsprintf(szMessage,"Error opening destination file: %s!",szDestination);
    CCrypto::HandleError(szMessage);
	CCrypto::CloseHandles();
	return FALSE;
}
// Get handle to the default provider. 
if(!CryptAcquireContext(
	&this->hCryptProv,
	NULL, 
	MS_ENHANCED_PROV, 
	PROV_RSA_FULL, 
	0))	{

    // Attempt to acquire a handle to the default key container.
    if(!CryptAcquireContext(&this->hCryptProv, NULL, NULL, PROV_RSA_FULL, 0)) {
        
		LPSTR pszContainerName = NULL;
		DWORD cbContainerName = 0;
 
		if(GetLastError() != NTE_BAD_KEYSET) {
            // Some sort of error occured.
			CCrypto::HandleError("Error opening default key container!");
			CCrypto::CloseHandles();
			return FALSE;
        }
        
        // Create default key container.
        if(!CryptAcquireContext(&this->hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET)) {
            
			CCrypto::HandleError("Error creating default key container!");
			CCrypto::CloseHandles();
			return FALSE;
        }

        // Get size of the name of the default key container name.
        if(CryptGetProvParam(this->hCryptProv, PP_CONTAINER, NULL, &cbContainerName, 0)) {
            
            // Allocate buffer to receive default key container name.
            pszContainerName = (char *)malloc(cbContainerName);

            if(pszContainerName) {
                // Get name of default key container name.
                if(!CryptGetProvParam(this->hCryptProv, PP_CONTAINER, (unsigned char *)pszContainerName, &cbContainerName, 0)) {
                    // Error getting default key container name.
                    pszContainerName[0] = 0;
                }
            }
        }

        wsprintf(szMessage,"Create key container '%s'", pszContainerName ? pszContainerName : "");
		MessageBox(GetActiveWindow(),szMessage,"Info",0);

        // Free container name buffer (if created)
        if(pszContainerName) {
            free(pszContainerName);
        }
	}
}
if(!CryptAcquireContext(
	&this->hCryptProv,
	NULL, 
	MS_ENHANCED_PROV, 
	PROV_RSA_FULL, 
	0))
	{
		CCrypto::HandleError("Error during CryptAcquireContext!");
		CCrypto::CloseHandles();
		return FALSE;
	}
//--------------------------------------------------------------------
// Create a hash object. 
if(!CryptCreateHash(
	CCrypto::hCryptProv, 
	CALG_MD5, 
	0, 
	0, 
	&this->hHash))
	{ 
		CCrypto::HandleError("Error during CryptCreateHash!");
		CCrypto::CloseHandles();
		return FALSE;
	}  
//--------------------------------------------------------------------
// Hash the password. 
if(!CryptHashData(
       CCrypto::hHash, 
       (BYTE *)szKey, 
       strlen(szKey), 
       0))
 {
    CCrypto::HandleError("Error during CryptHashData!"); 
	CCrypto::CloseHandles();
	return FALSE;
 }
//--------------------------------------------------------------------
// Derive a session key from the hash object. 
if(!CryptDeriveKey(
       CCrypto::hCryptProv, 
       ENCRYPT_ALGORITHM, 
       CCrypto::hHash, 
       KEYLENGTH, 
       &this->hKey))
 {
   CCrypto::HandleError("Error during CryptDeriveKey!"); 
   CCrypto::CloseHandles();
   return FALSE;
 }
//--------------------------------------------------------------------
// Destroy the hash object. 
CryptDestroyHash(CCrypto::hHash); 
CCrypto::hHash = 0; 
 
//--------------------------------------------------------------------
// Determine number of bytes to encrypt at a time. 
// This must be a multiple of ENCRYPT_BLOCK_SIZE.
// ENCRYPT_BLOCK_SIZE is set by a #define statement.

dwBlockLen = 1000 - 1000 % ENCRYPT_BLOCK_SIZE; 

//--------------------------------------------------------------------
// Determine the block size. If a block cipher is used, 
// it must have room for an extra block. 

if(ENCRYPT_BLOCK_SIZE > 1 && bEncrypt)
    dwBufferLen = dwBlockLen + ENCRYPT_BLOCK_SIZE; 
else 
    dwBufferLen = dwBlockLen; 
    
//--------------------------------------------------------------------
// Allocate memory. 
if(CCrypto::pbBuffer = (BYTE *)malloc(dwBufferLen))
{
    ;
}
else
{ 
	CCrypto::HandleError("Error allocating memory!"); 
	CCrypto::CloseHandles();
	return FALSE;
}
//--------------------------------------------------------------------
// In a do loop, encrypt the source file and write to the source file. 

do { 
	//--------------------------------------------------------------------
	// Read up to dwBlockLen bytes from the source file. 
	dwCount = fread(CCrypto::pbBuffer, 1, dwBlockLen, CCrypto::hSource); 
	if(ferror(CCrypto::hSource))
	{ 
		wsprintf(szMessage,"Error reading from source file: %s!",szSource);
		CCrypto::HandleError(szMessage);
		CCrypto::CloseHandles();
		return FALSE;
	}

	if(bEncrypt) {
		//--------------------------------------------------------------------
		// Encrypt data. 
		if(!CryptEncrypt(
			 CCrypto::hKey, 
			 0, 
			 feof(CCrypto::hSource), 
			 0, 
			 CCrypto::pbBuffer, 
			 &dwCount, 
			 dwBufferLen))
		{ 
			CCrypto::HandleError("Error during CryptEncrypt!"); 
			CCrypto::CloseHandles();
			return FALSE;
		} 
	}
	else {
		// Decrypt data. 
		if(!CryptDecrypt(
			  CCrypto::hKey, 
			  0, 
			  feof(CCrypto::hSource), 
			  0, 
			  CCrypto::pbBuffer, 
			  &dwCount))
		{
			CCrypto::HandleError("Error during CryptDecrypt!"); 
			CCrypto::CloseHandles();
			return FALSE;
		} 
	}

	//--------------------------------------------------------------------
	// Write data to the destination file. 
	fwrite(CCrypto::pbBuffer, 1, dwCount, CCrypto::hDestination); 
	if(ferror(CCrypto::hDestination))
	{ 
		wsprintf(szMessage,"Error writing destination file: %s!",szSource);
	    CCrypto::HandleError(szMessage);
		CCrypto::CloseHandles();
		return FALSE;
	}

} while(!feof(CCrypto::hSource)); 
//--------------------------------------------------------------------
//  End the do loop when the last block of the source file has been
//  read, encrypted, and written to the destination file.
//--------------------------------------------------------------------

	CCrypto::CloseHandles();
	return TRUE;
}


//////////////////////////////////////////////////////////////////////
// Cleanup of handles
//////////////////////////////////////////////////////////////////////
void CCrypto::CloseHandles() {
	if(this->hSource)		fclose(this->hSource); 
	if(this->hDestination)	fclose(this->hDestination); 
	if(this->pbBuffer)		free(this->pbBuffer); 
	if(this->hKey)			CryptDestroyKey(this->hKey); 
	if(this->hHash)			CryptDestroyHash(this->hHash); 
	if(this->hCryptProv)	CryptReleaseContext(this->hCryptProv, 0);
}
