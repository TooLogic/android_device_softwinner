/* core-internet-image.c: UpdateTV Internet Core
                          Image Access Interface for internet images.

 This module implements the high level NOC conversation logic needed to
 access images remotely.
 
 In order to ensure UpdateTV network compatibility, customers must not
 change UpdateTV Core code including this file.

 Copyright (c) 2009 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Jim Muchow      04/14/2010  Add retry logic to UtvInternetImageAccessInterfaceRead()
  Jim Muchow      04/07/2010  Use the UtvInternetDownloadStart() to connect instead of individual opens
  Bob Taylor      03/18/2009  Created.
*/

#include "utv.h"

#define UTV_INTERNET_IAI_MAX_HOST_NAME_SIZE    128
#define UTV_INTERNET_IAI_MAX_FILE_NAME_SIZE    256
#define UTV_INTERNET_IAI_MAX_IMAGES            4

typedef struct 
{
    UTV_INT8    cbHostName[ UTV_INTERNET_IAI_MAX_HOST_NAME_SIZE ];
    UTV_INT8    cbFileName[ UTV_INTERNET_IAI_MAX_FILE_NAME_SIZE ];
    UTV_UINT32  uiCurrentOffset;
    UTV_BOOL    bOpen;
} UtvInternetImage;

static UtvInternetImage UtvInternetImageArray[ UTV_INTERNET_IAI_MAX_IMAGES ] = {
    { {0}, {0}, 0, 0 },
    { {0}, {0}, 0, 0 },
    { {0}, {0}, 0, 0 },
    { {0}, {0}, 0, 0 }
};

void UtvInternetImageAccessInterfaceReinit( void )
{
    int index;

    for ( index = 0; index < UTV_INTERNET_IAI_MAX_IMAGES; index++ )
    {
        UtvInternetImageArray[index].cbHostName[ 0 ] = '0';
        UtvInternetImageArray[index].cbFileName[  0 ] = '0';
        UtvInternetImageArray[index].uiCurrentOffset = 0;
        UtvInternetImageArray[index].bOpen = UTV_FALSE;
    }
}
    
UTV_RESULT UtvInternetImageAccessInterfaceGet( UtvImageAccessInterface *pIAI )
{
    pIAI->pfOpen  = UtvInternetImageAccessInterfaceOpen;
    pIAI->pfRead  = UtvInternetImageAccessInterfaceRead;
    pIAI->pfClose = UtvInternetImageAccessInterfaceClose;
    return UTV_OK;
}

UTV_RESULT UtvInternetImageAccessInterfaceInfo( UTV_INT8 **pszHostName, UTV_INT8 **pszFileName, UTV_UINT32 hImage )
{
    /* check that handle is OK */
    if ( UTV_INTERNET_IAI_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvInternetImageArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    *pszHostName = UtvInternetImageArray[ hImage ].cbHostName;
    *pszFileName = UtvInternetImageArray[ hImage ].cbFileName;
    
    return UTV_OK;
}

UTV_RESULT UtvInternetImageAccessInterfaceOpen( UTV_INT8 *pszHost, UTV_INT8 *pszFileName, UTV_UINT32 *phImage )
{
    UTV_UINT32    i;
    
    /* store the image acces info in an available storage entry if there is one. */
    for ( i = 0; i < UTV_INTERNET_IAI_MAX_IMAGES; i++ )
        if ( !UtvInternetImageArray[ i ].bOpen )
            break;
    
    if ( UTV_INTERNET_IAI_MAX_IMAGES <= i )
        return UTV_NO_IMAGES_ENTRIES;
    
    /* got one, fill it */
    UtvStrnCopy( (UTV_BYTE *)UtvInternetImageArray[ i ].cbHostName, UTV_INTERNET_IAI_MAX_HOST_NAME_SIZE, (UTV_BYTE *)pszHost, UtvStrlen( (UTV_BYTE *)pszHost ) );
    UtvStrnCopy( (UTV_BYTE *)UtvInternetImageArray[ i ].cbFileName, UTV_INTERNET_IAI_MAX_FILE_NAME_SIZE, (UTV_BYTE *)pszFileName, UtvStrlen( (UTV_BYTE *)pszFileName ) );
    UtvInternetImageArray[ i ].uiCurrentOffset = 0;
    UtvInternetImageArray[ i ].bOpen = UTV_TRUE;

    *phImage = i;
    
    return UTV_OK;
}

UTV_RESULT UtvInternetImageAccessInterfaceRead( UTV_UINT32 hImage, UTV_UINT32 uiOffset, UTV_UINT32 uiLength, UTV_BYTE *pubBuffer )
{
    UTV_RESULT    rStatus;
    UTV_UINT32 uiRetryCount;
	UTV_UINT32    hSession;

    /* check that handle is OK */
    if ( UTV_INTERNET_IAI_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvInternetImageArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;
    
    /* Open the internet connection and session */
    if ( UTV_OK != ( rStatus = UtvInternetDownloadStart( &hSession, UtvInternetImageArray[ hImage ].cbHostName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceRead: UtvInternetDownloadStart() failed with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    for ( uiRetryCount = 0; uiRetryCount < UTV_PLATFORM_INTERNET_DOWNLOAD_RETRY; uiRetryCount++ )
    {
        /* perform the download */
        if ( UTV_OK == ( rStatus = UtvInternetMsgDownloadRange( hSession, UtvInternetImageArray[ hImage ].cbFileName, UtvInternetImageArray[ hImage ].cbHostName,
                                                                uiOffset, uiOffset + uiLength - 1, pubBuffer, uiLength )))
        {
            break;
        }

        UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceRead: UtvInternetMsgDownloadRange() fails with error \"%s\"", UtvGetMessageString( rStatus ) );

        /* catch return values from UtvInternetMsgDownloadRange() that shouldn't be retried */
        if ( UTV_RANGE_REQUEST_FAILURE == rStatus || UTV_ABORT == rStatus )
        {
            break;
        }
        
        if ( ( rStatus == UTV_INTERNET_CONN_CLOSED ) ||
             ( rStatus == UTV_INTERNET_PERS_FAILURE) ||
             ( rStatus == UTV_INTERNET_READ_TIMEOUT) ||
             ( rStatus == UTV_INTERNET_SRVR_FAILURE ) )
        {
            UTV_RESULT local_rStatus = rStatus;

            UtvPlatformInternetClearSessionClose( hSession );
            if ( UTV_OK == ( rStatus = UtvPlatformInternetClearSessionOpen( &hSession )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceRead() re-connected - retrying download" );
                rStatus = local_rStatus; /* restore original error status */
                continue;
            }

            UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceRead() re-connect error: \"%s\"", UtvGetMessageString( rStatus ) );
            break;
        }

        UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceRead() - retry" );
    }
    
    /* all done, close the session and internet connection */
    UtvPlatformInternetClearSessionClose( hSession );
    UtvPlatformInternetClearClose();

	return rStatus;
}

UTV_RESULT UtvInternetImageAccessInterfaceClose( UTV_UINT32 hImage )
{
    /* check that handle is OK */
    if ( UTV_INTERNET_IAI_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvInternetImageArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    UtvInternetImageArray[ hImage ].bOpen = UTV_FALSE;

    return UTV_OK;
}
