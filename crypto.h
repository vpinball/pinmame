#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <wincrypt.h>
#include "utils.h"

//Came from the Platform SDK example.. Not sure what changing these would do
#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)
#define KEYLENGTH  0x00800000
#define ENCRYPT_ALGORITHM CALG_RC4 
#define ENCRYPT_BLOCK_SIZE 8 

class CCrypto
{
public:
	CCrypto(void);
	virtual ~CCrypto();
	BOOL EncryptFile(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination);
	BOOL DecryptFile(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination);

private:
	FILE *hSource; 
	FILE *hDestination; 	
	PBYTE pbBuffer; 
	HCRYPTKEY hKey; 	
	HCRYPTHASH hHash; 	
	HCRYPTPROV hCryptProv; 	
	void HandleError(LPCSTR szString);
	void CloseHandles(void);
	BOOL CryptIt(LPCSTR szSource,LPCSTR szKey, LPCSTR szDestination, BOOL bEncrypt);
};

#endif
