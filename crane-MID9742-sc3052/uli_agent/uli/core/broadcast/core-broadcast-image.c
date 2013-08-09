/* core-broadcast-image.c: UpdateTV Internet Core
                           Image Access Interface for broadcast images.

 This module implements a simple memory based file system for accessing update images.
 
 In order to ensure UpdateTV network compatibility, customers must not
 change UpdateTV Core code including this file.

 Copyright (c) 2009 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      03/18/2009  Created.
*/

#include "utv.h"

#define UTV_BROADCAST_IAI_MAX_IMAGES    4

typedef struct 
{
    UTV_BYTE    *pBuffer;
    UTV_UINT32    uiCurrentOffset;
    UTV_BOOL    bOpen;
} UtvBroadcastImage;

static UtvBroadcastImage UtvBroadcastImageArray[ UTV_BROADCAST_IAI_MAX_IMAGES ] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};
    
UTV_RESULT UtvBroadcastImageAccessInterfaceGet( UtvImageAccessInterface *pIAI )
{
    pIAI->pfOpen  = UtvBroadcastImageAccessInterfaceOpen;
    pIAI->pfRead  = UtvBroadcastImageAccessInterfaceRead;
    pIAI->pfClose = UtvBroadcastImageAccessInterfaceClose;
    
    return UTV_OK;
}

UTV_RESULT UtvBroadcastImageAccessInterfaceOpen( UTV_INT8 *pszHost, UTV_INT8 *pszFileName, UTV_UINT32 *phImage )
{
    UTV_UINT32    i;
    
    /* store the image acces info in an available storage entry if there is one. */
    for ( i = 0; i < UTV_BROADCAST_IAI_MAX_IMAGES; i++ )
        if ( !UtvBroadcastImageArray[ i ].bOpen )
            break;
    
    if ( UTV_BROADCAST_IAI_MAX_IMAGES <= i )
        return UTV_NO_IMAGES_ENTRIES;
    
    /* got one, fill it */
    UtvBroadcastImageArray[ i ].pBuffer = (UTV_BYTE *)pszFileName;
    UtvBroadcastImageArray[ i ].uiCurrentOffset = 0;
    UtvBroadcastImageArray[ i ].bOpen = UTV_TRUE;

    *phImage = i;
    
    return UTV_OK;
}

UTV_RESULT UtvBroadcastImageAccessInterfaceRead( UTV_UINT32 hImage, UTV_UINT32 uiOffset, UTV_UINT32 uiLength, UTV_BYTE *pubBuffer )
{
    /* check that handle is OK */
    if ( UTV_BROADCAST_IAI_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvBroadcastImageArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;
    
    /* just copy the text that the caller is interested in */
    UtvByteCopy( pubBuffer, &UtvBroadcastImageArray[ hImage ].pBuffer[ uiOffset ], uiLength );

    return UTV_OK;
}

UTV_RESULT UtvBroadcastImageAccessInterfaceClose( UTV_UINT32 hImage )
{
    /* check that handle is OK */
    if ( UTV_BROADCAST_IAI_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvBroadcastImageArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    UtvBroadcastImageArray[ hImage ].bOpen = UTV_FALSE;

    return UTV_OK;
}
