/* platform-store.c : UpdateTV "File" Implemetation of the Update Store Functions

   Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformStoreOpen( UTV_UINT32 uiStoreIndex, UTV_UINT32 uiAccessModeFlags, UTV_UINT32 uiUpdateSize, UTV_UINT32 *puiStoreIndex )
   UTV_RESULT UtvPlatformStoreWrite( UTV_UINT32 uiStoreIndex, UTV_BYTE *pDataBuffer, UTV_UINT32 uiBytesToWrite, UTV_UINT32 *puiBytesWritten )
   UTV_RESULT UtvPlatformStoreClose( UTV_UINT32 uiStoreIndex )
   UTV_RESULT UtvPlatformUpdateAssignRemote( UTV_INT8 *pszFileName, UTV_UINT32 *puiStoreIndex )
   UTV_RESULT UtvPlatformUpdateUnassignRemote( void );
   UTV_RESULT UtvFileImageAccessInterfaceGet( UtvImageAccessInterface *pIAI )
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      06/18/2011  Fixed UtvPlatformStoreInit() return value.
   Bob Taylor      06/27/2010  Changed manifest access from persistent ("active") to read-only ("backup").
   Bob Taylor      06/23/2010  Added UtvPlatformUpdateUnassignRemote().  Changed close logic to leave assigned image names intact.
   Seb Emonet      03/23/2010  Set pszFileName pointer to NULL after UtvFree() to avoid a double deletion crash 
   Bob Taylor      02/08/2010  Adapted from platform-store-file.c.  Added UtvPlatformStoreInit().
*/

#include "utv.h"

/* needed for file writes. */
#include <stdio.h>

/* needed for atoi(). */
#include <stdlib.h>

typedef struct
{
    UTV_BOOL    bOpen;
    UTV_BOOL    bPrimed;
    UTV_BOOL    bAssignable;
    UTV_BOOL    bManifest; /* reserved for component manifest only */
    FILE       *fp;
    UTV_INT8   *pszFileName;
    UTV_UINT32  uiBytesStored;
    UTV_UINT32  uiMaxBytes;
    UTV_INT8    szHostName[UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS];
    UTV_INT8    szFilePath[UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS];
    UTV_BOOL    abComponentAvailable[ UTV_PLATFORM_MAX_COMPONENTS ];
} UtvUpdateStoreEntry;

static UtvUpdateStoreEntry    s_aStoreEntries[ UTV_PLATFORM_UPDATE_STORE_MAX_STORES ] = {

    { UTV_FALSE, UTV_FALSE, UTV_FALSE, UTV_FALSE, NULL, NULL, 0, UTV_PLATFORM_UPDATE_STORE_0_SIZE }, /* store 0 */
    { UTV_FALSE, UTV_FALSE, UTV_FALSE, UTV_FALSE, NULL, NULL, 0, UTV_PLATFORM_UPDATE_STORE_1_SIZE }, /* store 1 */
    { UTV_FALSE, UTV_FALSE, UTV_TRUE,  UTV_FALSE, NULL, NULL, 0, 0 }, /* an entry that can be assigned to an existing file */
    { UTV_FALSE, UTV_FALSE, UTV_FALSE, UTV_TRUE,  NULL, NULL, 0, UTV_MAX_COMP_DIR_SIZE } /* component manifest */
};

/* UtvPlatformStoreInit
    Called by UtvPublicAgentInit() to intialize the store struct.
*/
UTV_RESULT UtvPlatformStoreInit( void )
{
    /* assign store 0 name */
    s_aStoreEntries[ 0 ].pszFileName = UtvPlatformGetStore0Name();
    
    /* assign store 1 name */
    s_aStoreEntries[ 1 ].pszFileName = UtvPlatformGetStore1Name();

    /* store 2 doesn't have a pre-assigned name */

    /* assign store 3 name */
    s_aStoreEntries[ 3 ].pszFileName = UtvPlatformGetBackupCompManifestName();

    return UTV_OK;
}

/* UtvPlatformStoreOpen
    This routine is called to store an unencrypted download before it is installed.
    It accepts a uiStoreIndex argument which allows multiple update stores to be accessed.
    If there is only one store then

   Returns UTV_OK and a store handle if the storage system is ready to receive calls to
   UtvUpdateStoreWrite() which will provide the data to be stored.

   Returns UTV_NO_OVERWRITE if an update already exists and permission was not given to overwrite it.

   Returns UTV_TOO_BIG if the provided size exceeds the storage capacity of the system.
*/
UTV_RESULT UtvPlatformStoreOpen( UTV_UINT32 uiStoreIndex, UTV_UINT32 uiAccessFlags, UTV_UINT32 uiUpdateSize, UTV_UINT32 *puiStoreIndex,
                                 UTV_INT8 *pszHostName, UTV_INT8 *pszFilePath )
{
    UTV_RESULT   rStatus;
    FILE        *fp;
    UTV_INT8    *pszMode;
    UTV_UINT32   i;

    do
    {
        /* sanity check store index */
        if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <=  uiStoreIndex )
        {
            rStatus = UTV_BAD_INDEX;
            break;
        }

        /* if this store is already open then fail this, but return the index so it can continue to be used*/
        if ( s_aStoreEntries[ uiStoreIndex ].bOpen )
        {
            rStatus = UTV_ALREADY_OPEN;
            *puiStoreIndex = uiStoreIndex;
            break;
        }

        /* sanity check access flags and set mode */
        if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_READ )
            pszMode = "rb";
        else if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE )
        {
            if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_APPEND )
                pszMode = "ab";
            else
                pszMode = "wb";
        }
        else
        {
            rStatus = UTV_BAD_ACCESS;
            break;
        }

        /* if this store hasn't been primed yet size it, unless this hasn't been assigned yet. */
        if ( !s_aStoreEntries[ uiStoreIndex ].bPrimed )
        {
            /* assume empty */
            s_aStoreEntries[ uiStoreIndex ].uiBytesStored = 0;

            if ( NULL != s_aStoreEntries[ uiStoreIndex ].pszFileName )
            {
                if ( NULL != (fp = fopen( s_aStoreEntries[ uiStoreIndex ].pszFileName, "rb" )))
                {
                    fseek( fp, 0, SEEK_END );
                    s_aStoreEntries[ uiStoreIndex ].uiBytesStored = ftell( fp );
                    fclose( fp );
                }
            }

            s_aStoreEntries[ uiStoreIndex ].bPrimed = UTV_TRUE;
        }

        /* if this is a write we need to check some things */
        if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE )
        {
            /* check if anything is in it, if so look for overwrite or append */
            if ( 0 != s_aStoreEntries[ uiStoreIndex ].uiBytesStored )
            {
                /* if we're appending then we need to see that we're appending to the same image */
                if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_APPEND )
                {
                    /* compare images names */
                    if ( NULL == pszFilePath ||
                         0 != UtvMemcmp( s_aStoreEntries[ uiStoreIndex ].szHostName, pszHostName, UtvStrlen( (UTV_BYTE *) pszHostName ) ||
                         0 != UtvMemcmp( s_aStoreEntries[ uiStoreIndex ].szFilePath, pszFilePath, UtvStrlen( (UTV_BYTE *) pszFilePath ))))
                    {
                        /* a different image is stored here, overwrite if allowed to otherwise fail */
                        if (uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE )
                        {
                            /* open it for write to clear it */
                            if ( NULL == (fp = fopen( s_aStoreEntries[ uiStoreIndex ].pszFileName, "wb" )))
                            {
                                rStatus = UTV_FILE_OPEN_FAILS;
                                break;
                            }
                            fclose( fp );
                            s_aStoreEntries[ uiStoreIndex ].uiBytesStored = 0;

                            /* clear the components tracker */
                            for ( i = 0; i < UTV_PLATFORM_MAX_COMPONENTS; i++ )
                                s_aStoreEntries[ uiStoreIndex ].abComponentAvailable[ i ] = UTV_FALSE;

                        } else
                        {
                            rStatus = UTV_NO_OVERWRITE;
                            break;
                        }
                    }
                } else
                {
                    /* no append, make sure we have overwrite permission */
                    if ( !(uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE ))
                    {
                        rStatus = UTV_NO_OVERWRITE;
                        break;
                    } else
                    {
                        /* clear the components tracker */
                        for ( i = 0; i < UTV_PLATFORM_MAX_COMPONENTS; i++ )
                            s_aStoreEntries[ uiStoreIndex ].abComponentAvailable[ i ] = UTV_FALSE;

                        s_aStoreEntries[ uiStoreIndex ].uiBytesStored = 0;

                    }
                }
            } /* if there is anything already in the store */

            /* sanity check size of write */
            if ( s_aStoreEntries[ uiStoreIndex ].uiMaxBytes < uiUpdateSize )
            {
                rStatus = UTV_TOO_BIG;
                break;
            }

            if ( NULL != pszFilePath )
            {
                UtvStrnCopy( (UTV_BYTE *) s_aStoreEntries[ uiStoreIndex ].szHostName, UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS,
                             (UTV_BYTE *) pszHostName, UtvStrlen( (UTV_BYTE *) pszHostName ));
                UtvStrnCopy( (UTV_BYTE *) s_aStoreEntries[ uiStoreIndex ].szFilePath, UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS,
                             (UTV_BYTE *) pszFilePath, UtvStrlen( (UTV_BYTE *) pszFilePath ));
            }
        }

        /* if this is a read than we need to check if anything is here */
        if ( uiAccessFlags & UTV_PLATFORM_UPDATE_STORE_FLAG_READ )
        {
            if ( 0 == s_aStoreEntries[ uiStoreIndex ].uiBytesStored )
            {
                rStatus = UTV_STORE_EMPTY;
                break;
            }
        }

        /* open it */
        if ( NULL == (fp = fopen( s_aStoreEntries[ uiStoreIndex ].pszFileName, pszMode )))
        {
            rStatus = UTV_FILE_OPEN_FAILS;
            break;
        }

        /* success */
        s_aStoreEntries[ uiStoreIndex ].bOpen = UTV_TRUE;
        s_aStoreEntries[ uiStoreIndex ].fp = fp;

        /* Return the index as a handle.
           This is done because future expansion might decouple store indices from handles
         */
        *puiStoreIndex = uiStoreIndex;
        rStatus = UTV_OK;

    } while ( UTV_FALSE );

    return rStatus;
}

UTV_RESULT UtvPlatformStoreWrite( UTV_UINT32 uiStoreIndex, UTV_BYTE *pDataBuffer, UTV_UINT32 uiBytesToWrite, UTV_UINT32 *puiBytesWritten )
{
    UTV_RESULT         rStatus;

    do
    {
        /* sanity check store handle which we treat as an index */
        if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= uiStoreIndex )
        {
            rStatus = UTV_BAD_INDEX;
            break;
        }

        /* if this store isn't already open then fail this */
        if ( !s_aStoreEntries[ uiStoreIndex ].bOpen )
        {
            rStatus = UTV_NOT_OPEN;
            break;
        }

        /* write data */
        *puiBytesWritten = fwrite( pDataBuffer, 1, uiBytesToWrite, s_aStoreEntries[ uiStoreIndex ].fp );
        s_aStoreEntries[ uiStoreIndex ].uiBytesStored += *puiBytesWritten;
        if ( *puiBytesWritten != uiBytesToWrite )
        {
            rStatus = UTV_WRITE_FAILS;
            break;
        }

        rStatus = UTV_OK;

    } while ( UTV_FALSE );

    if ( UTV_OK != rStatus )
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreWrite(): %s", UtvGetMessageString( rStatus ) );

    return rStatus;
}

UTV_RESULT UtvPlatformStoreClose( UTV_UINT32 uiStoreIndex )
{
    UTV_RESULT         rStatus;

    do
    {
        /* sanity check store handle which we treat as an index */
        if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= uiStoreIndex )
        {
            rStatus = UTV_BAD_INDEX;
            break;
        }

        /* if this store isn't already open then fail this */
        if ( !s_aStoreEntries[ uiStoreIndex ].bOpen )
        {
            rStatus = UTV_NOT_OPEN;
            break;
        }

        /* close it */
        fclose( s_aStoreEntries[ uiStoreIndex ].fp );
        s_aStoreEntries[ uiStoreIndex ].bOpen = UTV_FALSE;
        rStatus = UTV_OK;

    } while ( UTV_FALSE );

    return rStatus;

}

UTV_RESULT UtvPlatformStoreDelete( UTV_UINT32 uiStoreIndex )
{
    UTV_RESULT     rStatus;
    FILE        *fp;

    do
    {
        /* sanity check store handle which we treat as an index */
        if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= uiStoreIndex )
        {
            rStatus = UTV_BAD_INDEX;
            break;
        }

        /* if this store is already open then fail this */
        if ( s_aStoreEntries[ uiStoreIndex ].bOpen )
        {
            rStatus = UTV_ALREADY_OPEN;
            break;
        }

        /* delete it */
        s_aStoreEntries[ uiStoreIndex ].uiBytesStored = 0;

        /* not allowed to delete assignable (external) stores */
        if ( !s_aStoreEntries[ uiStoreIndex ].bAssignable && NULL != s_aStoreEntries[ uiStoreIndex ].pszFileName )
        {
            if ( NULL != (fp = fopen( s_aStoreEntries[ uiStoreIndex ].pszFileName, "wb" )))
                fclose( fp );
        }

        s_aStoreEntries[ uiStoreIndex ].bPrimed = UTV_FALSE;
        rStatus = UTV_OK;

    } while ( UTV_FALSE );

    if ( UTV_OK != rStatus )
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreDelete(): %s", UtvGetMessageString( rStatus ) );

    return rStatus;

}

UTV_RESULT UtvPlatformStoreAttributes( UTV_UINT32 uiStoreIndex, UTV_BOOL *pbOpen, UTV_BOOL *pbPrimed, UTV_BOOL *pbAssignable,
                                       UTV_BOOL *pbManifest, UTV_UINT32 *puiBytesWritten )
{
    *puiBytesWritten = 0;

    /* sanity check store index */
    if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <=  uiStoreIndex )
        return UTV_BAD_INDEX;

    *pbOpen = s_aStoreEntries[ uiStoreIndex ].bOpen;
    *pbPrimed = s_aStoreEntries[ uiStoreIndex ].bPrimed;
    *pbAssignable = s_aStoreEntries[ uiStoreIndex ].bAssignable;
    *pbManifest = s_aStoreEntries[ uiStoreIndex ].bManifest;
    *puiBytesWritten = s_aStoreEntries[ uiStoreIndex ].uiBytesStored;

    return UTV_OK;
}

UTV_RESULT UtvPlatformStoreSetCompStatus( UTV_UINT32 uiStoreIndex, UTV_UINT32 uiCompIndex, UTV_BOOL bStatus )
{
    /* sanity check store index */
    if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <=  uiStoreIndex )
        return UTV_BAD_INDEX;

    /* sanity check comp index */
    if ( UTV_PLATFORM_MAX_COMPONENTS <= uiCompIndex )
        return UTV_BAD_COMPONENT_INDEX;

    s_aStoreEntries[ uiStoreIndex ].abComponentAvailable[ uiCompIndex ] = (UTV_BYTE) bStatus;

    return UTV_OK;
}

/* UtvPlatformStoreAssignRemote
    This routine is called to assign a remotely stored file into a slot in the store array where
    it can be treated like any other store.

   Returns UTV_OK if an open assignable store was found as well as a handle to that store.
*/
UTV_RESULT UtvPlatformUpdateAssignRemote( UTV_INT8 *pszFileName, UTV_UINT32 *puiStoreIndex )
{
    UTV_UINT32    i;

    /* locate a spot in the store array that is marked for remote access */
    for ( i = 0; i < UTV_PLATFORM_UPDATE_STORE_MAX_STORES; i++ )
    {
        if ( s_aStoreEntries[ i ].bAssignable )
            break;
    }

    if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= i )
        return UTV_NO_OPEN_STORES;

    /* close and free memory associated with any outstanding assigments */
    UtvPlatformUpdateUnassignRemote();

    /* found one, assign it and return its index */

    /* free up any existing, allocate space for new, copy it in. */
    if ( NULL != s_aStoreEntries[ i ].pszFileName )
    {
        UtvFree( s_aStoreEntries[ i ].pszFileName );
        s_aStoreEntries[ i ].pszFileName = NULL;
    }

    if ( NULL == ( s_aStoreEntries[ i ].pszFileName = UtvMalloc( UtvStrlen( (UTV_BYTE *) pszFileName ) + 1 )))
        return UTV_NO_MEMORY;

    UtvStrnCopy( (UTV_BYTE *) s_aStoreEntries[ i ].pszFileName, UtvStrlen( (UTV_BYTE *) pszFileName ) + 1,
                 (UTV_BYTE *) pszFileName, UtvStrlen( (UTV_BYTE *) pszFileName ));
    s_aStoreEntries[ i ].pszFileName[ UtvStrlen( (UTV_BYTE *) pszFileName ) ] = 0;

    *puiStoreIndex = i;

    return UTV_OK;
}

/* UtvPlatformStoreUnassignRemote
    This routine is called when the Agent is shutdown to free up any assignment memory.

   Returns UTV_OK if an open assignable store was found as well as a handle to that store.
*/
UTV_RESULT UtvPlatformUpdateUnassignRemote( void )
{
    UTV_UINT32    i;

    /* locate a spot in the store array that is marked for remote access */
    for ( i = 0; i < UTV_PLATFORM_UPDATE_STORE_MAX_STORES; i++ )
    {
        if ( s_aStoreEntries[ i ].bAssignable )
            break;
    }

    if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= i )
        return UTV_NO_OPEN_STORES;

    UtvPlatformStoreClose( i );

    /* free up any existing, allocate space for new, copy it in. */
    if ( NULL != s_aStoreEntries[ i ].pszFileName )
    {
        UtvFree( s_aStoreEntries[ i ].pszFileName );
        s_aStoreEntries[ i ].pszFileName = NULL;
    }

    return UTV_OK;
}

static UTV_RESULT UtvFileImageAccessInterfaceOpen( UTV_INT8 *pszHost, UTV_INT8 *pszFileName, UTV_UINT32 *phImage )
{
    UTV_UINT32    uiStoreIndex;

    /* ignore the host name and treat the file name as a store index */
    uiStoreIndex = (UTV_UINT32) pszFileName;

    /* sanity check store index */
    if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <=  uiStoreIndex )
        return UTV_BAD_INDEX;

    /* attempt to open it for a read */
    return UtvPlatformStoreOpen( uiStoreIndex, UTV_PLATFORM_UPDATE_STORE_FLAG_READ, 0, phImage, NULL, NULL );
}

static UTV_RESULT UtvFileImageAccessInterfaceRead( UTV_UINT32 hImage, UTV_UINT32 uiOffset, UTV_UINT32 uiLength, UTV_BYTE *pubBuffer )
{
    UTV_RESULT    rStatus;
    UTV_UINT32  uiBytesRead;

    do
    {
        /* sanity check store handle which we treat as an index */
        if ( UTV_PLATFORM_UPDATE_STORE_MAX_STORES <= hImage )
        {
            rStatus = UTV_BAD_INDEX;
            break;
        }

        /* if this store isn't already open then fail this */
        if ( !s_aStoreEntries[ hImage ].bOpen )
        {
            rStatus = UTV_NOT_OPEN;
            break;
        }

        /* seek to offset */
        if ( fseek( s_aStoreEntries[ hImage ].fp, uiOffset, SEEK_SET ))
        {
            rStatus = UTV_SEEK_FAILS;
            break;
        }

        /* read data */
        uiBytesRead = fread( pubBuffer, 1, uiLength, s_aStoreEntries[ hImage ].fp );
        if ( uiBytesRead != uiLength )
        {
            rStatus = UTV_READ_FAILS;
            break;
        }

        rStatus = UTV_OK;

    } while ( UTV_FALSE );

    if ( UTV_OK != rStatus )
        UtvMessage( UTV_LEVEL_ERROR, " UtvFileImageAccessInterfaceRead(): %s", UtvGetMessageString( rStatus ) );

    return rStatus;
}

UTV_RESULT UtvFileImageAccessInterfaceGet( UtvImageAccessInterface *pIAI )
{
    pIAI->pfOpen  = UtvFileImageAccessInterfaceOpen;
    pIAI->pfRead  = UtvFileImageAccessInterfaceRead;
    pIAI->pfClose = UtvPlatformStoreClose;

    return UTV_OK;
}
