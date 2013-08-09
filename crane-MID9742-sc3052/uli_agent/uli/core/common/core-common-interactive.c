/* core-common-interactive.c: UpdateTV Common Core - Interactive Mode Support

 This module implements the routines that Interactive Mode.  Interactive mode
 is designed to let the CEM control when the Agent is active.

 In order to ensure UpdateTV network compatibility, customers must not
 change UpdateTV Common Core code.

 Copyright (c) 2004-2011 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      12/08/2011  Added UtvPlatformInstallCleanup() calls to downloadComponentWorkerInternet() to handle resume
                              properly when a comunnication error occurs.
  Bob Taylor      11/18/2011  Removed broadcast support.
  Jim Muchow      11/02/2011  Download Resume support
  Bob Taylor      06/05/2011  RELEASE mode support.  Fixed bad logic when UtvMessage() is disabled.
  Jim Muchow      09/21/2010  BE support.
  Nathan Ford     05/20/2010  Added UtvImageClose() calls where appropriate in checkStoredImages for proper cleanup.
                              Removed UtvImageOpen() and UtvImageClose() from UtvInstallComponentsWorker() as it is using
                              the already open pUpdate->hImage
  Bob Taylor      04/23/2010  Fixed uninit variable warning.
  Bob Taylor      04/15/2010  Fixed additional UQ after SD logic in get internet schedule function.
                              Added module download retry if decrypt fails.
  Seb Emonet      03/25/2010  Added progress for USB updates in UtvInstallComponentsWorker()
  Seb Emonet      03/15/2010  OOB Message support.
  Bob Taylor      02/09/2010  Added fix for last component optional in USB download case.
  Bob Taylor      02/03/2010  Check for abort prior to download tracking clear.  Broadcast only.
  Bob Taylor      11/03/2009  Don't display expected store is empty warning.  Rationalize DOWNLOAD_ENDED return values.
                              Include download ended for file delivery.  Support for new bCompatible flag.  Addition of
                              copy only test mode.  Fixed UtvInstallComponentsWorker() hImage references.  Changed
                              ErrorStats to LastError in UtvPublicGetStatus().
  Bob Taylor      06/29/2009  Moved socket session open/close to just around UtvInternetDownloadRange() in
                              downloadComponentWorkerInternet() to fix long decrypt time socket session timeout.
                              Fixed build error when UTV_DELIVERY_FILE was not defined.
  Bob Taylor      05/27/2009  Fixed component download fails in the middle of a multi-component download error.
                              Modified for new schedule format.
  Bob Taylor      02/02/2009  Derived from core-broadcast-carousel.c
*/

#include "utv.h"

/* static protos */
static void UtvDownloadComponentsWorker( void );
static void *UtvDownloadComponentsWorkerAsThread( void * );
static void UtvInstallComponentsWorker( void );
static void *UtvInstallComponentsWorkerAsThread( void * );
static UTV_RESULT checkStoredImages( void );
#ifdef UTV_DELIVERY_INTERNET
static UTV_RESULT downloadComponentWorkerInternet( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
static UTV_RESULT downloadComponentWorkerInternetWrapper( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
#endif

/* static vars */
static UTV_BOOL        s_bInstallWithoutStore = UTV_FALSE;
static UTV_BOOL        s_bConvertOptional = UTV_FALSE;

/* global async result object that is used to return results
   of operations that take place in their own thread and are
   remotely monitored.
*/
UTV_PUBLIC_RESULT_OBJECT    g_sUtvAsyncResultObject;

/* callback function storage variable. */
UTV_PUBLIC_CALLBACK         g_fCallbackFunction = NULL;

/* Time slot structure stored here. */
/* TODO: fix the initialization of this structure. */
UTV_PUBLIC_SCHEDULE         g_sSchedule = {0};

static UTV_BOOL             s_bFactoryMode = UTV_FALSE;

static UTV_BOOL             s_bStoredOnly = UTV_FALSE;

/* This value allows a DownloadSchedule task to be separate from the Download(Image)
   task. For example, the NOC Monitor code is configured to only perform the former
   in almost all cases. This flag does not prevent the two tasks from being run, it
   only prevents their operations from being coordinated to avoid creating unnecessary 
   data structures and performing redundant communications with the NOC.
*/
static UTV_BOOL             s_bCoordinateDownloadScheduleAndDownloadImage = UTV_TRUE;

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
UTV_BOOL            g_bDSIContainsFactoryDescriptor = UTV_FALSE;
static UTV_BOOL     s_bFactoryDecryptEnabled = UTV_FALSE;

/* UtvEnableFactoryDecrypt
    One step in the two-step process that permits factory-strength encryption.
*/
void UtvEnableFactoryDecrypt( UTV_BOOL bFactoryDecryptEnable )
{
    s_bFactoryDecryptEnabled = bFactoryDecryptEnable;
}

/* UtvFactoryDecryptPermission
   Returns the logical and of s_bDSIContainsFactoryDescriptor and s_bFactoryDecryptEnabled.
   Used by UtvDecryptModule() to determine whether factory strength encryption is permitted.
*/
UTV_BOOL UtvFactoryDecryptPermission( )
{
    return g_bDSIContainsFactoryDescriptor && s_bFactoryDecryptEnabled;
}
#endif

/* UtvSetFactoryMode
    Allows other routines to check whether factory mode has been enabled.
*/
void UtvSetFactoryMode( UTV_BOOL bFactoryMode )
{
    s_bFactoryMode = bFactoryMode;
}

/* UtvGetFactoryMode
    Allows other routines to check whether factory mode has been enabled.
*/
UTV_BOOL UtvGetFactoryMode( void )
{
    return s_bFactoryMode;
}

/* UtvClearDownloadTimes
    Clears the download times list for a new scan
*/
UTV_RESULT UtvClearDownloadTimes( )
{
    UTV_UINT32 u, i;

    /* initialize the schedule structure before parsing any modules. */
    g_sSchedule.uiNumUpdates = 0;
    for ( u = 0; u < UTV_PLATFORM_MAX_UPDATES; u++ )
    {
        g_sSchedule.tUpdates[ u ].uiNumComponents = 0;
        for ( i = 0; i < UTV_PLATFORM_MAX_MODULES; i++ )
            g_sSchedule.tUpdates[ u ].aubModuleEncounteredFlag[ i ] = UTV_FALSE;

        g_sSchedule.tUpdates[ u ].uiNumTimeSlots = 0;
        for ( i = 0; i < UTV_PLATFORM_MAX_SCHEDULE_SLOTS; i++ )
            g_sSchedule.tUpdates[ u ].uiSecondsToStart[ i ] = 0;
    }

    return UTV_OK;
}

/* UtvSortSchedule
    Sorts the schedule from earliest to latest.
*/
UTV_RESULT UtvSortSchedule( void )
{
    UTV_UINT32      i, j, u, uiTemp;

    if ( !g_sSchedule.uiNumUpdates )
        return UTV_NO_DOWNLOAD;

    /* for each update */
    for ( u = 0; u < g_sSchedule.uiNumUpdates; u++ )
    {
        /* sort the list */
        for ( i = 0; i < g_sSchedule.tUpdates[ u ].uiNumTimeSlots; i++ )
            for ( j = 0; j < g_sSchedule.tUpdates[ u ].uiNumTimeSlots - 1; j++ )
                if ( g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j ] > g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j + 1 ] )
                {
                    uiTemp = g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j + 1 ];
                    g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j + 1 ] = g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j ];
                    g_sSchedule.tUpdates[ u ].uiSecondsToStart[ j ] = uiTemp;
                }

    }

    /* perform the wake-up-early fixup */
    for ( u = 0; u < g_sSchedule.uiNumUpdates; u++ )
    {
        for ( i = 0; i < g_sSchedule.tUpdates[ u ].uiNumTimeSlots; i++ )
        {
            if ( g_sSchedule.tUpdates[ u ].uiSecondsToStart[ i ] > (UTV_WAIT_UP_EARLY/1000) )
                 g_sSchedule.tUpdates[ u ].uiSecondsToStart[ i ] -= (UTV_WAIT_UP_EARLY/1000);
            else
                g_sSchedule.tUpdates[ u ].uiSecondsToStart[ i ] = 0;
        }
    }

    return UTV_OK;
}

#ifdef UTV_DELIVERY_INTERNET
static UTV_RESULT getDownloadScheduleWorkerInternet( UTV_UINT32 hPlatformStoreImage )
{
    UTV_RESULT                            rStatus;
    UTV_BOOL                              bNew, bDownloadScheduled = UTV_FALSE;
    UTV_INT8                             *pCompName;
    UTV_UINT32                            u, c, hImage;
    UtvInternetUpdateQueryResponse       *putvUQR;
    UtvInternetScheduleDownloadResponse  *pUtvSDR;
    UtvImageAccessInterface               utvIAI;
    UtvCompDirHdr                        *pCompDirHdr;
    UtvCompDirCompDesc                   *pCompDesc;
    UtvRange                             *pHWMRanges, *pSWVRanges;
    UtvDependency                        *pSWDeps;
    UtvTextDef                           *pTextDefs;

    /* assumre no updates */
    g_sSchedule.uiNumUpdates = 0;

    /* ask the NOC whether it has any updates for us */
    rStatus = UtvInternetCheckForUpdates( &putvUQR );

    /* if there is no download available then return that and
       make sure the schedule structure is updated with the
       next time we're supposed to query.
    */
    if ( UTV_NO_DOWNLOAD == rStatus )
    {
        g_sSchedule.uiNumUpdates = 0;
        UtvMessage( UTV_LEVEL_INFO, "UpdateQuery indicates NOC says there aren't any updates for this platform" );
        return rStatus;
    }

    /* if there's an error return it */
    if ( UTV_OK != rStatus )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvInternetCheckForUpdates(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "UpdateQuery indicates NOC says there are %d updates for this platform", putvUQR->uiNumEntries );
    for ( u = 0; u < putvUQR->uiNumEntries; u++ )
    {
        UtvMessage( UTV_LEVEL_INFO, "    UQE[ %d ] OUI: 0x%06X, Model Group: 0x%04X, Module Version: 0x%04X, Attributes: 0x%08X",
                    u, putvUQR->aUtvUQE[ u ].uiOUI, putvUQR->aUtvUQE[ u ].usModelGroup, putvUQR->aUtvUQE[ u ].usModuleVersion, putvUQR->aUtvUQE[ u ].uiAttributes );

        for ( c = 0; c < putvUQR->aUtvUQE[ u ].uiNumCompatEntries; c++ )
            UtvMessage( UTV_LEVEL_INFO, "             Compat - HM Start: 0x%02X, HM End: 0x%02X, SV Start: 0x%02X, SV End: 0x%02X",
                        putvUQR->aUtvUQE[ u ].utvHWMRange[c].usStart, putvUQR->aUtvUQE[ u ].utvHWMRange[c].usEnd,
                        putvUQR->aUtvUQE[ u ].utvSWVRange[c].usStart, putvUQR->aUtvUQE[ u ].utvSWVRange[c].usEnd );
    }

    /* The NOC has a package of updates for us that contain update steps (module versions) greater than our
       current update step. Check the rest of the UQR for other compatibility info.  */
    for ( u = 0; u < putvUQR->uiNumEntries; u++ )
    {
        /* sending an hImage of zero when there is no SW Deps doesn't have any side effect */
        if ( UTV_OK == ( rStatus = UtvCoreCommonCompatibilityUpdateConvert( 0, putvUQR->aUtvUQE[ u ].uiOUI, putvUQR->aUtvUQE[ u ].usModelGroup,
                                                                   (UTV_UINT32) putvUQR->aUtvUQE[ u ].usModuleVersion,
                                                                   (UTV_UINT32) putvUQR->aUtvUQE[ u ].uiAttributes,
                                                                    putvUQR->aUtvUQE[ u ].uiNumCompatEntries,
                                                                   &putvUQR->aUtvUQE[ u ].utvHWMRange[ 0 ],
                                                                    putvUQR->aUtvUQE[ u ].uiNumCompatEntries,
                                                                   &putvUQR->aUtvUQE[ u ].utvSWVRange[ 0 ],
                                                                    0, NULL )))
        {
            /* This update us OUI/MG/HWM/SWV/FT compatible.  Download its component directory to provide component-level
               resolution of the update. */
            UtvMessage( UTV_LEVEL_INFO, "Update-level Compatibility OK, checking component directory header." );

            /* issue an additional UQ if we've already issued a schedule download otherwise the schedule download will fail. */
            if ( bDownloadScheduled )
            {
                UtvInternetCheckForUpdates( &putvUQR );
            }

            /* acquire the download schedule */
            if ( UTV_OK != ( rStatus = UtvInternetScheduleDownload( &putvUQR->aUtvUQE[ u ], UTV_TRUE, &pUtvSDR )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvInternetScheduleDownload() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one update stop the others */
            }

            /* as soon as a download is scheduled set a flag to let future calls to schedule a download know that they should
               issue an update query first. */
            bDownloadScheduled = UTV_TRUE;

             /* download the component dir */

            /* get the internet version of the image access interface */
            if ( UTV_OK != ( rStatus = UtvInternetImageAccessInterfaceGet( &utvIAI )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one update stop the others */
            }

            /* attempt to open this image which will download and decrypt its component directory */
            if ( UTV_OK != ( rStatus = UtvImageOpen( pUtvSDR->szHostName, pUtvSDR->szFilePath, &utvIAI, &hImage, hPlatformStoreImage ) ) )
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvImageOpen() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one update stop the others */
            }

            /* got component directory, double check it */
            if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one update stop the others */
            }

            /* These parameters must agree with the signaled parameters */
            if ( putvUQR->aUtvUQE[ u ].uiOUI != Utv_32letoh( pCompDirHdr->uiPlatformOUI ) ||
                 putvUQR->aUtvUQE[ u ].usModelGroup != Utv_16letoh( pCompDirHdr->usPlatformModelGroup ) ||
                 putvUQR->aUtvUQE[ u ].usModuleVersion != Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ) ||
                 putvUQR->aUtvUQE[ u ].uiAttributes != Utv_32letoh( pCompDirHdr->uiUpdateAttributes ) )
            {
                rStatus = UTV_SIGNALING_DISAGREES;
                UtvMessage( UTV_LEVEL_ERROR, "signaled Compatibility and stamped Compatibility disagree.  Error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one update stop the others */
            }

            UtvMessage( UTV_LEVEL_INFO, "Component Directory Header Compatibility matches signaled Compatibility, checking components." );

            /* Perform component-level Compatibility checking on this update.
               Once qualified insert components into download schedule.
               No components qualified to start with.
            */
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents = 0;
            for ( c = 0; c < Utv_16letoh( pCompDirHdr->usNumComponents ); c++ )
            {
                UTV_BOOL bCompatible;

                /* get compat info */
                if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, c, &pCompDesc )))
                {
                    UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetComponent() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                    continue; /* don't let the failure of one component lookup stop the others */
                }

                /* for debug only */
                if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), 
                                                                  &pCompName )))
                {
                    UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails: \"%s\"", UtvGetMessageString( rStatus ) );
                    continue; /* don't let the failure of one component lookup stop the others */
                }

                /* check it */
                switch ( rStatus = UtvCoreCommonCompatibilityComponent( hImage, pCompDesc ) )
                {
                case UTV_OK:
                    bCompatible = UTV_TRUE;
                    bNew = UTV_FALSE;
                    UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is compatible.", pCompName );
                    break;

                case UTV_UNKNOWN_COMPONENT:
                    /* are we allowed to accept new components? */
                    if ( !UtvManifestAllowAdditions( ) )
                    {
                        UtvMessage( UTV_LEVEL_ERROR, "Component \"%s\" is new and NO additions are allowed.", pCompName );
                        continue;
                    } else
                    {
                        UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is new, accepted.", pCompName );
                        rStatus = UTV_OK;
                        bNew = UTV_TRUE ;
                        bCompatible = UTV_TRUE;
                    }
                    break;

                case UTV_INCOMPATIBLE_HWMODEL:
                case UTV_INCOMPATIBLE_SWVER:
                case UTV_DEPENDENCY_MISSING:
                case UTV_INCOMPATIBLE_DEPEND:
                     /* Compatiblity errors at the component level mean don't accept the update, copy the old data instead. */
                    bCompatible = UTV_FALSE;
                    bNew = UTV_FALSE;
                    UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is incompatible.  Instructing platform to copy old data.", pCompName );
                    break;

                default:
                    UtvMessage( UTV_LEVEL_INFO, "UtvCoreCommonCompatibilityComponent fails: \"%s\".", UtvGetMessageString( rStatus ) );
                    continue;
                    break;
                }

                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].pCompDesc = pCompDesc;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bNew = bNew;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bStored = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bApprovedForStorage = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bApprovedForInstall = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bCompatible = bCompatible;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bCopied = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents++;

            } /* for each potential component candidate */

            /* if there were any candidates then bump the update count and snag the global update info */
            if ( 0 != g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents )
            {
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_INTERNET;
                /* distribute the update global next query seconds */
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNextQuerySeconds = putvUQR->uiNextQuerySeconds;
                /* the store where this download will live (if it is chosen) hasn't been configured yet */
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hStore = hPlatformStoreImage;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hImage = hImage;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumTimeSlots = 0;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiSecondsToStart[ 0 ] = pUtvSDR->uiDownloadInSeconds;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].usModuleVersion = pUtvSDR->usModuleVersion;
                /* add host name and file path */
                UtvStrnCopy( (UTV_BYTE *) g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].szHostName, UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS,
                             (UTV_BYTE *) pUtvSDR->szHostName, UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS );
                UtvStrnCopy( (UTV_BYTE *) g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].szFilePath, UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS,
                             (UTV_BYTE *) pUtvSDR->szFilePath, UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS );
                g_sSchedule.uiNumUpdates++;

                /* As soon as we find a good download then punt.  The Agent should not be put into a position where it is choosing between
                   multiple available updates.  It should just take the next compatible one. */
                break;
            }

        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "Update-level Compatibility check fails due to \"%s\"", UtvGetMessageString( rStatus ) );
        }

    } /* for each UQR entry */

    /* this is a one shot, close the image opened above */
    if ( UTV_PLATFORM_UPDATE_STORE_INVALID == hPlatformStoreImage )
        UtvImageClose( hImage );

    /* if any download have been accepted for consideration then return OK */
    if ( 0 != g_sSchedule.uiNumUpdates )
        rStatus = UTV_OK;
    return rStatus;
}
#endif

static UTV_RESULT checkStoredImages( void )
{
    UTV_RESULT                     rStatus, rCompatError = UTV_NO_DOWNLOAD;
    UTV_BOOL                       bNew;
    UTV_INT8                      *pCompName;
    UTV_UINT32                     hStore, c, hImage;
    UtvImageAccessInterface        utvIAI;
    UtvCompDirHdr                 *pCompDirHdr;
    UtvCompDirCompDesc            *pCompDesc;
    UtvRange                      *pHWMRanges, *pSWVRanges;
    UtvDependency                 *pSWDeps;
    UtvTextDef                    *pTextDefs;
    UTV_BOOL                       bOpen, bPrimed, bAssignable, bManifest;
    UTV_UINT32                     uiBytesWritten;
#ifdef UTV_DELIVERY_FILE
    UTV_BOOL                       bAssignableOnly = UTV_FALSE;
#endif

    /* assume none */
    g_sSchedule.uiNumUpdates = 0;

    /* get the file version of the image access interface */
    if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvFileImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* If this query is for FILE-enabled updates only then restrict the images looked at to those that are "assignable"
       meaning the stores exist on external devices that have been temporarily linked into the system. */
#ifdef UTV_DELIVERY_FILE
    /* if the Agent is only configured to look for file delivery then
       it will look like the query is taking place via stored
       updates only.
     */
    if ( UtvPlatformUtilityFileOnly() )
        bAssignableOnly = UTV_TRUE;
#endif

    for ( hStore = 0; hStore < UTV_PLATFORM_UPDATE_STORE_MAX_STORES; hStore++ )
    {
        /* check the attributes of this image. */
        if ( UTV_OK != ( rStatus = UtvPlatformStoreAttributes( hStore, &bOpen, &bPrimed, &bAssignable, &bManifest, &uiBytesWritten )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformStoreAttributes( %d ) fails with error \"%s\"", hStore, UtvGetMessageString( rStatus ) );
            continue;
        }

        /* skip the manifest */
        if ( bManifest )
            continue;

#ifdef UTV_DELIVERY_FILE
        if ( bAssignableOnly && !bAssignable )
            continue;
#endif

        /* attempt to open this image which will download and decrypt its component directory */
        if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)hStore, &utvIAI, &hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
        {
#ifdef UTV_DEBUG
            /* ignore empty */
            if ( UTV_STORE_EMPTY != rStatus )
                UtvMessage( UTV_LEVEL_ERROR, "UtvImageOpen( %d ) fails with error \"%s\"", hStore, UtvGetMessageString( rStatus ) );
#endif
            continue; /* don't let the failure of one update stop the others */
        }

        /* get the component directory */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            UtvImageClose( hImage );
            continue; /* don't let the failure of one update stop the others */
        }

        /* stores can include USB files that may not be compatible with us. */
        if ( UTV_OK != ( rCompatError = UtvCoreCommonCompatibilityUpdate( hImage, 
                                                                          Utv_32letoh( pCompDirHdr->uiPlatformOUI ), 
                                                                          Utv_16letoh( pCompDirHdr->usPlatformModelGroup ),
                                                                          Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ), 
                                                                          Utv_32letoh( pCompDirHdr->uiUpdateAttributes ),
                                                                          Utv_16letoh( pCompDirHdr->usNumUpdateHardwareModelRanges ), 
                                                                          pHWMRanges,
                                                                          Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareVersionRanges ), 
                                                                          pSWVRanges,
                                                                          Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareDependencies ), 
                                                                          pSWDeps )))
        {
            UtvMessage( UTV_LEVEL_WARN, "UtvCoreCommonCompatibilityUpdate(): \"%s\"", UtvGetMessageString( rCompatError ) );
            UtvImageClose( hImage );
            continue; /* don't let the failure of one update stop the others */
        }

        UtvMessage( UTV_LEVEL_INFO, "Component Directory Header Compatibility matches signaled Compatibility, checking components." );

        /* Perform component-level Compatibility checking on this update.
           Once qualified insert components into download schedule. */
        g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents = 0;
        for ( c = 0; c < Utv_16letoh( pCompDirHdr->usNumComponents ); c++ )
        {
            UTV_BOOL bCompatible;

            /* get compat info */
            if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, c, &pCompDesc )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetComponent() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one component lookup stop the others */
            }

            /* for debug only */
            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pCompName )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails: \"%s\"", UtvGetMessageString( rStatus ) );
                continue; /* don't let the failure of one component lookup stop the others */
            }

            /* check it */
            switch ( rStatus = UtvCoreCommonCompatibilityComponent( hImage, pCompDesc ) )
            {
            case UTV_OK:
                bCompatible = UTV_TRUE;
                bNew = UTV_FALSE;
                UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is compatible.", pCompName );
                break;

            case UTV_UNKNOWN_COMPONENT:
                /* are we allowed to accept new components? */
                if ( !UtvManifestAllowAdditions( ) )
                {
                    UtvMessage( UTV_LEVEL_ERROR, "Component \"%s\" is new and NO additions are allowed.", pCompName );
                    continue;
                } else
                {
                    UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is new, accepted.", pCompName );
                    bNew = UTV_TRUE ;
                    bCompatible = UTV_TRUE;
                }
                break;

            case UTV_INCOMPATIBLE_HWMODEL:
            case UTV_INCOMPATIBLE_SWVER:
            case UTV_DEPENDENCY_MISSING:
            case UTV_INCOMPATIBLE_DEPEND:
                /* Compatiblity errors at the component level mean don't accept the update, copy the old data instead. */
                bCompatible = UTV_FALSE;
                bNew = UTV_FALSE;
                UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" is incompatible.  Instructing platform to copy old data.", pCompName );
                break;

            default:
                UtvMessage( UTV_LEVEL_INFO, "UtvCoreCommonCompatibilityComponent fails: \"%s\".", UtvGetMessageString( rStatus ) );
                continue;
                break;
            }

            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].pCompDesc = pCompDesc;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bNew = bNew;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bStored = UTV_TRUE;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bApprovedForStorage = UTV_FALSE;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bApprovedForInstall = UTV_FALSE;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bCompatible = bCompatible;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents ].bCopied = UTV_FALSE;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents++;

        } /* for each component candidate */

        if ( 0 != g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents )
        {
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_FILE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hStore = hStore;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hImage = hImage;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumTimeSlots = 0;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiSecondsToStart[ 0 ] = 0;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].usModuleVersion = Utv_16letoh( pCompDirHdr->usUpdateModuleVersion );
                g_sSchedule.uiNumUpdates++;
        }

    } /* for each store */

    /* if any download have been accepted for consideration then return OK */
    if ( 0 != g_sSchedule.uiNumUpdates )
        rStatus = UTV_OK;
    else
        rStatus = rCompatError;

    return rStatus;
}

/* UtvGetDownloadScheduleWorker
     Called to retrieve download start times from the network for updates that we're compatible with.
    Returns UTV_OK if data present, UTV_NO_DOWNLOAD otherwise
*/
void UtvGetDownloadScheduleWorker( void )
{
    UTV_RESULT rStatus = UTV_OK;
    UTV_PUBLIC_RESULT_OBJECT  *pResult;
    UTV_PUBLIC_CALLBACK        pCallback;
    UTV_UINT32            i;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO, "UtvGetDownloadScheduleWorker()" );

    g_sSchedule.uiNumUpdates = 0;

#ifdef UTV_DELIVERY_FILE
    /* if the Agent is only configured to look for file delivery then
       it will look like the query is taking place via stored
       updates only.
     */
    if ( UtvPlatformUtilityFileOnly() )
        s_bStoredOnly = UTV_TRUE;
#endif

    /* if we're supposed to look for stored items then do that now */
    if ( s_bStoredOnly )
    {
        /* store the result where it can be picked up and retrieve any registered callback functions. */
        pResult = UtvGetAsyncResultObject();
        pResult->rOperationResult = checkStoredImages();

        /* no other data associated with this result. */
        pResult->pOperationResultData = &g_sSchedule;

        /* if a callback function has been provided then call it. */
        if ( NULL != ( pCallback = UtvGetCallbackFunction() ))
            (*pCallback)( pResult );

        return;
    }

    do
    {
        /* Obtain All hardware required to execute this command */
        if ( UTV_OK != ( rStatus = UtvPlatformCommonAcquireHardware( UTV_PUBLIC_STATE_SCAN )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformCommonAcquireHardware(): %s", UtvGetMessageString( rStatus ));
            break;
        }

        /* look for a download via each delivery mode in the order presented
           as soon as one returns a positive result break, otherwise
           continue looking.
        */
        for ( i = 0, rStatus = UTV_UNKNOWN;
              rStatus != UTV_OK && i < g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes;
              i++ )
        {
            switch ( g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType )
            {
#ifdef UTV_DELIVERY_INTERNET
            case UTV_PUBLIC_DELIVERY_MODE_INTERNET:

                if ( s_bCoordinateDownloadScheduleAndDownloadImage )
                {
                    /* there is precedence for using (hard-coding) UTV_PLATFORM_UPDATE_STORE_0_INDEX
                       as the store location, but perhaps this is a bit inflexible? */
                    rStatus = getDownloadScheduleWorkerInternet( UTV_PLATFORM_UPDATE_STORE_0_INDEX );
                }
                else
                {
                    /* implies "don't store" */
                    rStatus = getDownloadScheduleWorkerInternet( UTV_PLATFORM_UPDATE_STORE_INVALID );
                }

                /* if the NOC was contacted then it has the final word */
                /* check for update available, no update available, or already have */
                if ( UTV_OK == rStatus || UTV_NO_DOWNLOAD == rStatus || UTV_BAD_MODVER_CURRENT == rStatus )
                {
                    /* force end of loop */
                    i = g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes;
                }
                break;
#endif
            default:
                rStatus = UTV_BAD_DELIVERY_MODE;
                break;
            }
        } /* for each delivery mode */

    } while( UTV_FALSE ); /* error bail */

    /* Release ownership of hardware */
    UtvPlatformCommonReleaseHardware();

    /* store the result where it can be picked up and retrieve any registered callback functions. */
    pResult = UtvGetAsyncResultObject();
    pResult->rOperationResult = rStatus;

    /* no other data associated with this result. */
    pResult->pOperationResultData = &g_sSchedule;

    /* if a callback function has been provided then call it. */
    if ( NULL != ( pCallback = UtvGetCallbackFunction() ))
        (*pCallback)( pResult );

    return;
}

void *UtvGetDownloadScheduleWorkerAsThread( void *placeholder )
{
    UtvGetDownloadScheduleWorker();
    UTV_THREAD_EXIT;
}

/* UtvGetAsyncResultObject
   Returns a pointer to a result object which is used to store
   results of operations that need to be remotely monitored
*/
UTV_PUBLIC_RESULT_OBJECT * UtvGetAsyncResultObject( void )
{
    return &g_sUtvAsyncResultObject;
}

/* UtvGetCallbackFunction
   Returns a pointer to a callback function storage area.
*/
void UtvSetCallbackFunction( UTV_PUBLIC_CALLBACK pCallback )
{
    g_fCallbackFunction = pCallback;
}


/* UtvGetCallbackFunction
   Returns a pointer to a callback function storage area.
*/
UTV_PUBLIC_CALLBACK UtvGetCallbackFunction( void )
{
    return g_fCallbackFunction;
}

#ifdef UTV_DELIVERY_INTERNET
static UTV_BOOL
skipThisModule( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModRecord *pModRecord)
{
    UTV_RESULT rStatus;
    UTV_BYTE *pBuffer;
    UTV_UINT32 ulComputedCRC;

    if ( ( NULL == pCompDesc ) || ( NULL == pModRecord ) )
        return UTV_FALSE;

    UtvMessage( UTV_LEVEL_INFO, "skipThisModule?");

    pBuffer = UtvPlatformInstallAllocateModuleBuffer( pModRecord->ulSize );
    if (NULL == pBuffer)
        return UTV_FALSE;

    if ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION )
    {
        UTV_UINT32 hPartHandle;
        
        rStatus = UtvPlatformInstallPartitionOpenRead( hImage, pCompDesc, 0, &hPartHandle );
        if ( UTV_OK != rStatus )  
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionOpenRead(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }

        rStatus = UtvPlatformInstallPartitionRead( hPartHandle, pBuffer,
                                                   pModRecord->ulSize, pModRecord->ulOffset);
        if ( UTV_OK != rStatus )  
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionRead(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }

        if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionClose( hPartHandle )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionClose(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }
    } 
    else
    {
        UTV_UINT32 hFileHandle;

        rStatus = UtvPlatformInstallFileOpenRead( hImage, pCompDesc, 0, &hFileHandle );
        if ( UTV_OK != rStatus )  
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileOpenRead(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }

        rStatus = UtvPlatformInstallFileSeekRead( hFileHandle, pBuffer,  
                                                  pModRecord->ulSize, pModRecord->ulOffset);
        if ( UTV_OK != rStatus )  
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileRead(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }

        if ( UTV_OK != ( rStatus = UtvPlatformInstallFileClose( hFileHandle )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileClose(): \"%s\"", UtvGetMessageString( rStatus ) );
            return UTV_FALSE;
        }
    }

    ulComputedCRC = UtvCrc32( pBuffer, pModRecord->ulSize);

    UtvPlatformInstallFreeModuleBuffer( pModRecord->ulSize, pBuffer );

    if (ulComputedCRC != pModRecord->ulCRC)
    {
        UtvMessage( UTV_LEVEL_ERROR, "CRCs do not match - redownload module");
        return UTV_FALSE;
    }

    UtvMessage( UTV_LEVEL_INFO, "CRCs match - module can be skipped");
    return UTV_TRUE;
}

static UTV_BOOL
findExistingModuleRecord(UTV_UINT32 componentIndex, UTV_UINT32 moduleIndex, UtvModRecord **ppModRecord)
{
    int index;

    for ( index = 0; index < g_utvDownloadTracker.usCurDownloadCount; index++ )
        if ( ( g_utvDownloadTracker.ausModIdArray[ index ].usCompIndex == componentIndex ) &&
             ( g_utvDownloadTracker.ausModIdArray[ index ].usModId == moduleIndex ) )
            break;

    /* a record is always found, the issue is whether it is the very first */
    /* one (in the case of a new download), an entry for something already */
    /* downloaded, or the entry just after the one last downloaded         */
    *ppModRecord = &g_utvDownloadTracker.ausModIdArray[ index ];

    /* cases 1 and 3 from above */
    if ( index >= g_utvDownloadTracker.usCurDownloadCount )
        return UTV_FALSE;

    /* case 2 */
    return UTV_TRUE;
}

static UTV_RESULT downloadComponentWorkerInternet( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_RESULT                             rStatus, lStatus;
    UTV_UINT32                             c, m, d, hStore, uiBytesWritten, hImage, hSession = 0;
    UTV_BYTE                              *pBuffer = NULL;
    UtvRange                              *pRange;
    UtvDependency                         *pSWDeps;
    UtvTextDef                            *pTextDefs;
    UtvCompDirModDesc                     *pModDesc;
    UtvImageAccessInterface                utvIAI;
    UTV_BOOL                               bAnyApprovedForInstall = UTV_FALSE, bComponentsInstalled = UTV_FALSE;
    UTV_INT8                              *pszHostName;
    UTV_INT8                              *pszFileName;
    UTV_INT8                              *pszCompName;
    UTV_UINT32                             lPercentComplete = 0, uiModuleRetry;
    UTV_UINT32                             lPercentCompleteTMP = 0;
    UTV_BOOL                               downloadRemainingModules;
    UtvModRecord                          *pModRecord;

    /* under optimal conditions, images are downloaded in one shot, but the  */
    /* reality is that networking environments can require multiple attempts */
    downloadRemainingModules = UTV_FALSE;
    pModRecord = NULL;

    /* see if we can use the component from the download schedule */
    rStatus = UTV_UNKNOWN; /* just need something non-UTV_OK */
    pszHostName = NULL;
    pszFileName = NULL;
    if ( ( UTV_OK == ( rStatus = UtvImageGetStore( pUpdate->hImage, &hImage ) ) ) &&
         ( UTV_OK == ( rStatus = UtvInternetImageAccessInterfaceInfo( &pszHostName, &pszFileName, hImage ) ) ) )
    {
        if ( ( 0 == UtvMemcmp( (UTV_BYTE *)pUpdate->szHostName, (UTV_BYTE *) pszHostName, 
                               UtvStrlen( (UTV_BYTE *)pszHostName ))) &&
             ( 0 == UtvMemcmp( (UTV_BYTE *)pUpdate->szFilePath, (UTV_BYTE *) pszFileName, 
                               UtvStrlen( (UTV_BYTE *)pszFileName ))))
        {
            hImage = pUpdate->hImage;
            hStore = pUpdate->hStore;
            rStatus = UTV_OK;
        }
    }

    if ( rStatus != UTV_OK )
    {
        /* close and then open the store that we have been told to download the update into initialize it */
        UtvPlatformStoreClose( pUpdate->hStore );

        if ( UTV_OK != ( rStatus = UtvPlatformStoreOpen( pUpdate->hStore,
                                                         UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE | \
                                                         UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE,
                                                         pUpdate->uiDownloadSize, &hStore,
                                                         pUpdate->szHostName, pUpdate->szFilePath )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreOpen() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        /* get the internet version of the image access interface */
        if ( UTV_OK != ( rStatus = UtvInternetImageAccessInterfaceGet( &utvIAI )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvInternetImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            UtvPlatformStoreClose( hStore );
            return rStatus;
        }

        /* Open the image which will write the component directory into the specified store.  This is needed even in
           the case where no modules are being stored so the manifest commit works. */
        if ( UTV_OK != ( rStatus = UtvImageOpen( pUpdate->szHostName, pUpdate->szFilePath, &utvIAI, &hImage, hStore )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageOpen() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            UtvPlatformStoreClose( hStore );
            return rStatus;
        }
    }

    /* scan components for size (used for progress) and install status */
    pUpdate->uiDownloadSize = 0;
    for ( c = 0; c < pUpdate->uiNumComponents; c++ )
    {
        if ( pUpdate->tComponents[ c ].bApprovedForInstall )
            bAnyApprovedForInstall = UTV_TRUE;

        if ( !pUpdate->tComponents[ c ].bApprovedForStorage && !pUpdate->tComponents[ c ].bApprovedForInstall )
            continue;
        pUpdate->uiDownloadSize += Utv_32letoh( pUpdate->tComponents[ c ].pCompDesc->uiImageSize );
    }

    /* resume update support */
    UtvCoreCommonCompatibilityDownloadStart( hImage, pUpdate->usModuleVersion );

    for ( c = 0; c < pUpdate->uiNumComponents; c++ )
    {
        /* get this component's name */
        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( pUpdate->hImage, 
                                                          Utv_16letoh( pUpdate->tComponents[ c ].pCompDesc->toName ),
                                                          &pszCompName )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            continue;
        }

        /* get a pointer to the module descriptor pointer */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescInfo( pUpdate->tComponents[ c ].pCompDesc, &pRange, &pRange, &pSWDeps, &pTextDefs, &pModDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvImageGetCompDescInfo() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            continue;
        }

        /* if there will be installation and an updated component is not compatible then we need to copy it. */
        if ( bAnyApprovedForInstall && !pUpdate->tComponents[ c ].bCompatible )
        {
            UtvMessage( UTV_LEVEL_INFO, " Component \"%s\" is not compatible.  COPYING", pszCompName );
            if ( UTV_OK != ( rStatus = UtvPlatformInstallCopyComponent( hImage, pUpdate->tComponents[ c ].pCompDesc )))
            {
                UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__, pszCompName );
            } else
            {
                bComponentsInstalled = UTV_TRUE;
                pUpdate->tComponents[ c ].bCopied = UTV_TRUE;
            }
            continue;
        }

        if ( !pUpdate->tComponents[ c ].bApprovedForStorage && !pUpdate->tComponents[ c ].bApprovedForInstall )
        {
            UtvMessage( UTV_LEVEL_INFO, " Component \"%s\" not approved for storage or install.  SKIPPING", pszCompName );
            continue;
        }

        UtvMessage( UTV_LEVEL_INFO, " Component \"%s\" compatible and approved, DOWNLOADING", pszCompName );

        /* write all the modules in this component */
        for ( m = 0, uiModuleRetry = UTV_PLATFORM_INTERNET_MODULE_RETRY; 
              m < Utv_16letoh( pUpdate->tComponents[ c ].pCompDesc->usModuleCount ); 
              m++, pModDesc++ )
        {
            /* ignore this resume stuff unless approved for install */
            if ( pUpdate->tComponents[ c ].bApprovedForInstall )
            {
                UTV_BOOL existingdModRecord;
                UTV_BOOL skipResult;

                UtvMessage( UTV_LEVEL_INFO, "Component %d Module #%d, %d bytes", 
                            c, m, 
                            Utv_32letoh( pModDesc->uiEncryptedModuleSize ) );

                existingdModRecord = findExistingModuleRecord(c, m, &pModRecord);
                if ( UTV_FALSE == existingdModRecord )
                    downloadRemainingModules = UTV_TRUE;
                    
                if ( UTV_FALSE == downloadRemainingModules )
                {
                    skipResult = skipThisModule(hImage, pUpdate->tComponents[ c ].pCompDesc, pModRecord);

                    if (skipResult == UTV_TRUE)
                    {
                        g_pUtvDiagStats->uiTotalReceivedByteCount += Utv_32letoh( pModDesc->uiEncryptedModuleSize );
                        UtvMessage( UTV_LEVEL_INFO, "\t\tno need to download this module");
                        continue;
                    }
                    else 
                    {
                        /* note this is a one shot - once set it stays TRUE until the fuinction exist */
                        downloadRemainingModules = UTV_TRUE;
                        if ( 0 == m)
                            UtvCoreCommonCompatibilityDownloadClear();
                    }
                }

                UtvMessage( UTV_LEVEL_INFO, "\t\tto be downloaded");
            }

            /* allocate memory for the entire encrypted module */
            if ( NULL == ( pBuffer = UtvPlatformInstallAllocateModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ) )))
            {
                rStatus = UTV_NO_MEMORY;
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallAllocateModuleBuffer( %d ) fails: %s", 
                            Utv_32letoh( pModDesc->uiEncryptedModuleSize ), UtvGetMessageString( rStatus ) );
                UtvImageClose( hImage );
                UtvPlatformStoreClose( hStore );
                UtvPlatformInstallCleanup();
                return rStatus;
            }

            /* Begin the download session */
            if ( UTV_OK != ( rStatus = UtvInternetDownloadStart( &hSession, pUpdate->szHostName )))
            {
                UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                UtvImageClose( hImage );
                UtvPlatformStoreClose( hStore );
                UtvPlatformInstallCleanup();
                UtvMessage( UTV_LEVEL_ERROR, " UtvInternetDownloadStart() failes with error \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            UtvMessage( UTV_LEVEL_INFO, " DOWNLOAD START %d bytes", Utv_32letoh( pModDesc->uiEncryptedModuleSize ) );

            /* download this module */
            if ( UTV_OK != ( rStatus = UtvInternetDownloadRange( &hSession, 
                                                                 pUpdate->szHostName, pUpdate->szFilePath,
                                                                 Utv_32letoh( pModDesc->uiModuleOffset ),
                                                                 ( Utv_32letoh( pModDesc->uiModuleOffset ) + 
                                                                   Utv_32letoh( pModDesc->uiEncryptedModuleSize ) - 1 ),
                                                                 pBuffer, 
                                                                 Utv_32letoh( pModDesc->uiEncryptedModuleSize  ))))
            {
                UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                UtvInternetDownloadStop( hSession );
                UtvImageClose( hImage );
                UtvPlatformStoreClose( hStore );
                UtvPlatformInstallCleanup();
                UtvMessage( UTV_LEVEL_ERROR, " UtvInternetDownloadRange() fails: %s", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            UtvMessage( UTV_LEVEL_INFO, " DOWNLOAD DONE" );

            /* log per 10 percent block for OOB testing */
            if ( 0 != (pUpdate->uiDownloadSize / 100) )
                lPercentCompleteTMP = g_pUtvDiagStats->uiTotalReceivedByteCount / ( pUpdate->uiDownloadSize / 100 );
            else
                lPercentCompleteTMP = 100;
            if( lPercentCompleteTMP > 100 )
                lPercentCompleteTMP = 100;
            for ( d = lPercentComplete+10; d <= lPercentCompleteTMP; d+=10)
            {
                UtvMessage( UTV_LEVEL_OOB_TEST, "IP Download: Progress %d %%",d);
            }
            lPercentComplete = d-10;

            /* Clean up download session */
            UtvInternetDownloadStop( hSession );

            /* write to the module to store if we were asked to store */
            if (pUpdate->tComponents[ c ].bApprovedForStorage )
            {
                if ( UTV_OK != ( rStatus = UtvPlatformStoreWrite( hStore, pBuffer, 
                                                                  Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                                  &uiBytesWritten )))
                {
                    UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                    UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreWrite( %d, %d ) fails: %s", 
                                Utv_32letoh( pModDesc->uiEncryptedModuleSize ),
                                uiBytesWritten, UtvGetMessageString( rStatus ) );
                    UtvImageClose( hImage );
                    UtvPlatformStoreClose( hStore );
                    UtvPlatformInstallCleanup();
                    return rStatus;
                }

                UtvMessage( UTV_LEVEL_INFO, " Module #%d, %d bytes, store success", m, 
                            Utv_32letoh( pModDesc->uiEncryptedModuleSize ) );

            }

            /* install the module if we were asked to */
            if ( pUpdate->tComponents[ c ].bApprovedForInstall )
            {
                rStatus = UtvImageInstallModule( hImage, pUpdate->tComponents[ c ].pCompDesc, 
                                                 pModDesc, pBuffer, pModRecord );
                if ( UTV_COMP_INSTALL_COMPLETE != rStatus && UTV_OK != rStatus )
                {
                    /* detect decryption problems and retry */
                    if ( UTV_BAD_HASH_VALUE == rStatus )
                    {
                        if ( 0 != --uiModuleRetry)
                        {
                            m--;
                            pModDesc--;
                            continue;
                        }
                    }

                    UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                    UtvMessage( UTV_LEVEL_ERROR, " UtvImageInstallModule() fails: %s", UtvGetMessageString( rStatus ) );
                    UtvImageClose( hImage );
                    UtvPlatformStoreClose( hStore );
                    UtvPlatformInstallCleanup();
                    return rStatus;
                }

                /* successful install reset module retry count */
                uiModuleRetry = UTV_PLATFORM_INTERNET_MODULE_RETRY;

                UtvMessage( UTV_LEVEL_INFO, " Module #%d, %d bytes, install success", 
                            m, Utv_32letoh( pModDesc->uiEncryptedModuleSize ) );

                /* let the compatibility machinery know we've got this one so we don't try to keep downloading it  */
                if ( UTV_OK != ( lStatus = UtvCoreCommonCompatibilityRecordModuleId( hImage, 
                                                                                     pUpdate->tComponents[ c ].pCompDesc, 
                                                                                     (UTV_UINT16) m)))
                {
                    UtvMessage( UTV_LEVEL_ERROR, " UtvCoreCommonCompatibilityRecordModuleId(): %s", UtvGetMessageString( lStatus ) );
                    UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                    UtvImageClose( hImage );
                    UtvPlatformStoreClose( hStore );
                    UtvPlatformInstallCleanup();
                    return lStatus;
                }

                if ( UTV_COMP_INSTALL_COMPLETE == rStatus )
                {
                    /* notify the platform specific code that we've installed a complete component */
                    if ( UTV_COMP_INSTALL_COMPLETE != ( rStatus = UtvPlatformInstallComponentComplete( hImage, pUpdate->tComponents[ c ].pCompDesc )))
                    {
                        UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__, pszCompName );
                        UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                        UtvImageClose( hImage );
                        UtvPlatformStoreClose( hStore );
                        UtvPlatformInstallCleanup();
                        return rStatus;
                    }
                    UtvMessage( UTV_LEVEL_INFO, " Component \"%s\", install success", pszCompName );
                    bComponentsInstalled = UTV_TRUE;
                }
            } /* if install */

            /* free the module buffer */
            UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );

        } /* for each module */

        /* mark this component in the store as having been written */
        if ( UTV_OK != ( lStatus = UtvPlatformStoreSetCompStatus( hStore, c, UTV_TRUE )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreSetCompStatus() fails with error \"%s\"", UtvGetMessageString( lStatus ) );
        }

        UtvMessage( UTV_LEVEL_INFO, " Component \"%s\", download success", pszCompName );

    } /* for each component */

    UtvCoreCommonCompatibilityDownloadClear();

    /* store needs to be closed prior to calling UtvInstallUpdateComplete() */
    if ( UTV_OK != ( lStatus = UtvPlatformStoreClose( hStore )))
    {

#ifdef UTV_DEBUG
        /* tolerate not open */
        if ( UTV_NOT_OPEN != lStatus )
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreClose() fails with error \"%s\"", UtvGetMessageString( lStatus ) );
#endif
    }

    /* if components were installed call the platform-specific code that handles this event */
    if ( bComponentsInstalled )
    {
        if ( UTV_DOWNLOAD_COMPLETE != ( rStatus = UtvInstallUpdateComplete( pUpdate->hImage, pUpdate->usModuleVersion )))
        {
            UtvLogEventOneHexNum( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__, pUpdate->usModuleVersion );
        }
    } else
    {
        /* if no components were installed, but everything is OK then convert to download complete */
        if ( UTV_OK == rStatus )
            rStatus = UTV_DOWNLOAD_COMPLETE;
    }

    /* close the image */
    UtvImageClose( hImage );

    return rStatus;
}

/* Because every call to download must result in a response to the NOC about download success
   wrap the download call and report the result here.
*/
static UTV_RESULT downloadComponentWorkerInternetWrapper( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UtvInternetScheduleDownloadResponse    utvSDR;
    UTV_RESULT                             rDownloadStatus, rStatus;

    UtvMessage( UTV_LEVEL_OOB_TEST, "IP Download: Start");

    rStatus = downloadComponentWorkerInternet( pUpdate );

    if ( UTV_DOWNLOAD_COMPLETE == rStatus)
    {
        UtvMessage( UTV_LEVEL_OOB_TEST, "IP Download: Complete");
    }
    else
    {
        UtvMessage( UTV_LEVEL_OOB_TEST, "IP Download: Failed");
    }


    utvSDR.uiOUI = UtvCEMGetPlatformOUI();
    utvSDR.usModelGroup = UtvCEMGetPlatformModelGroup();
    utvSDR.usModuleVersion = pUpdate->usModuleVersion;

    /* Translate the status of the download for the NOC.  If we send UTV_OK (0) we won't
       see this download again.  For testing it's useful to continue to receive the same
       update over and over.  Do the right thing depending on whether UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL
       is defined or not.
    */


    rDownloadStatus = ( rStatus == UTV_DOWNLOAD_COMPLETE ? UTV_OK : rStatus );
#ifndef UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL
    if ( UTV_OK != ( lStatus = UtvInternetDownloadEnded( &utvSDR, UTV_IP_DOWNLOAD_TYPE_INTERNET, rDownloadStatus )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvInternetDownloadEnded() fails with error \"%s\"", UtvGetMessageString( lStatus ) );
    }
#else
    g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedMV = utvSDR.usModuleVersion;
    g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedType = UTV_IP_DOWNLOAD_TYPE_INTERNET;
    g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedStatus = rDownloadStatus;
    g_utvAgentPersistentStorage.utvDiagStats.tLastDownloadEndedEvent = UtvTime( NULL );

    UtvPlatformPersistWrite();
#endif
    return rStatus;
}
#endif

UTV_RESULT UtvInstallUpdateComplete( UTV_UINT32 hImage, UTV_UINT16 usModuleVersion )
{
    UTV_RESULT    rStatus;

    /* call the platform-specific code to handle update install success */
    if ( UTV_UPD_INSTALL_COMPLETE != ( rStatus = UtvPlatformInstallUpdateComplete( hImage )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvInstallUpdateComplete() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
    } else
    {
        /* convert return status to download complete for backwards compatiblity */
        rStatus = UTV_DOWNLOAD_COMPLETE;
    }

    return rStatus;
}

/* UtvDownloadComponentsWorker
     Called to download modules from from the network.
    Returns UTV_DOWNLOAD_COMPLETE if successful.  UTV_NO_DOWNLOAD or other errors if not.
*/
static void UtvDownloadComponentsWorker( void )
{
    UTV_RESULT                           rStatus = UTV_NO_DOWNLOAD;
    UTV_PUBLIC_RESULT_OBJECT            *pResult;
    UTV_PUBLIC_CALLBACK                  pCallback;
    UTV_PUBLIC_UPDATE_SUMMARY           *pUpdate;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO, "UtvDownloadComponentsWorker()" );

    /* Traverse the update for entries that have been enabled for storage.
     */
    pResult = UtvGetAsyncResultObject();
    pUpdate = (UTV_PUBLIC_UPDATE_SUMMARY *) pResult->pOperationResultData;

    switch ( pUpdate->uiDeliveryMode )
    {
#ifdef UTV_DELIVERY_FILE
        case UTV_PUBLIC_DELIVERY_MODE_FILE:
            /* file delivered updates can't be downloaded, only installed because they're already local */
            rStatus = UTV_DOWNLOAD_NOT_SUPPORTED;
            break;
#endif
#ifdef UTV_DELIVERY_INTERNET
        case UTV_PUBLIC_DELIVERY_MODE_INTERNET:
            /* internet delivered updates are downloaded component by component */
            rStatus = downloadComponentWorkerInternetWrapper( pUpdate );
            break;
#endif
        default:
            rStatus = UTV_BAD_DELIVERY_MODE;
            break;
    }

    /* store the result where it can be picked up and retrieve any registered callback functions. */
    pResult = UtvGetAsyncResultObject();
    pResult->rOperationResult = rStatus;

    /* no other data associated with this result. */
    pResult->pOperationResultData = NULL;

    /* if a callback function has been provided then call it. */
    if ( NULL != ( pCallback = UtvGetCallbackFunction() ))
        (pCallback)( pResult );

    return;
}

static void *UtvDownloadComponentsWorkerAsThread( void *placeholder )
{
    UtvDownloadComponentsWorker();
    UTV_THREAD_EXIT;
}

/* UtvInstallComponentsWorker
     Called to install modules that are already stored.
    Returns UTV_INSTALL_COMPLETE if successful.  UTV_NO_INSTALL or other errors if not.
*/
static void UtvInstallComponentsWorker( void )
{
    UTV_RESULT                    rStatus = UTV_NO_INSTALL;
    UTV_PUBLIC_RESULT_OBJECT     *pResult;
    UTV_PUBLIC_CALLBACK           pCallback;
    UTV_PUBLIC_UPDATE_SUMMARY    *pUpdate;
    UTV_UINT32                    i, c;
    UtvImageAccessInterface       utvIAI;
    UTV_INT8                     *pszCompName;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO,  "UtvInstallComponentsWorker()" );

    /* Traverse the provided schedule for entries that have been enabled for storage.
     */
    pResult = UtvGetAsyncResultObject();
    pUpdate = (UTV_PUBLIC_UPDATE_SUMMARY *) pResult->pOperationResultData;

    /* reset the byte counter for the update progress info */
    g_pUtvDiagStats->uiTotalReceivedByteCount = 0;
    pUpdate->uiDownloadSize = 0;

    do
    {
        /* get the file version of the image access interface */
        if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvFileImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            break;
        }

/*        if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)pUpdate->hStore, &utvIAI, &pUpdate->hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
        {
            if ( UTV_ALREADY_OPEN != rStatus )
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvImageOpen() fails: %s", UtvGetMessageString( rStatus ) );
                break;
            }
        }
*/
        /* scan components for size (used for progress)  */
        for ( c = 0; c < pUpdate->uiNumComponents; c++ )
        {
            pUpdate->uiDownloadSize += Utv_32letoh( pUpdate->tComponents[ c ].pCompDesc->uiImageSize );
        }

        /* for each component in the update */
        for ( i = 0; i < pUpdate->uiNumComponents; i++ )
        {
            /* get this component's name */
            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( pUpdate->hImage, 
                                                              Utv_16letoh( pUpdate->tComponents[ i ].pCompDesc->toName ), 
                                                              &pszCompName )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                continue;
            }

            /* if this one isn't compatible then just copy it */
            if ( !pUpdate->tComponents[ i ].bCompatible )
            {
                UtvMessage( UTV_LEVEL_INFO, " Component \"%s\" is not compatible.  COPYING", pszCompName );
                if ( UTV_OK != ( rStatus = UtvPlatformInstallCopyComponent( pUpdate->hImage, pUpdate->tComponents[ i ].pCompDesc )))
                {
                    UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__, pszCompName );
                } else
                {
                    pUpdate->tComponents[ i ].bCopied = UTV_TRUE;

                    /* fix up rStatus so that check for install complete handles this correctly */
                    rStatus = UTV_COMP_INSTALL_COMPLETE;
                }
                continue;
            }

            /* check the delivery mode of the download and use the associated acquisition code. */
            if ( pUpdate->tComponents[ i ].bApprovedForInstall )
            {
                UtvMessage( UTV_LEVEL_INFO, " Component \"%s\" is compatible.  INSTALLING", pszCompName );
                if ( UTV_COMP_INSTALL_COMPLETE != ( rStatus = UtvImageInstallComponent( pUpdate->hImage, pUpdate->tComponents[ i ].pCompDesc )))
                {
                    UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__, pszCompName );
                }
            }
        }

        /* If we made it all the way through then convert the UTV_COMP_INSTALL_COMPLETE to a UTV_UPD_INSTALL_COMPLETE */
        if ( UTV_COMP_INSTALL_COMPLETE == rStatus )
        {
            if ( UTV_DOWNLOAD_COMPLETE != ( rStatus = UtvInstallUpdateComplete( pUpdate->hImage, pUpdate->usModuleVersion )))
            {
                UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_INSTALL, rStatus, __LINE__ );
            }
        }

    } while ( UTV_FALSE );

    /* close the image */
/*    UtvImageClose( pUpdate->hImage ); */

#ifdef UTV_DELIVERY_FILE
    /* This is where we signal for download ended for file delivery since there isn't an actual download.
     */
    if ( UtvPlatformUtilityFileOnly() )
    {

        rStatus = ( rStatus == UTV_DOWNLOAD_COMPLETE ? UTV_OK : rStatus );

        {
        #ifndef UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL
            UTV_RESULT lStatus;
        #endif
            UtvInternetScheduleDownloadResponse    utvSDR;
            utvSDR.uiOUI = UtvCEMGetPlatformOUI();
            utvSDR.usModelGroup = UtvCEMGetPlatformModelGroup();
            utvSDR.usModuleVersion = pUpdate->usModuleVersion;
        #ifndef UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL
            if ( UTV_OK != ( lStatus = UtvInternetDownloadEnded( &utvSDR, UTV_IP_DOWNLOAD_TYPE_FILE, rStatus )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvInternetDownloadEnded() fails with error \"%s\"", UtvGetMessageString( lStatus ) );
            }
        #else
                g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedMV = utvSDR.usModuleVersion;
                g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedType = UTV_IP_DOWNLOAD_TYPE_FILE;
                g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedStatus = rStatus;
                g_utvAgentPersistentStorage.utvDiagStats.tLastDownloadEndedEvent = UtvTime( NULL );
                UtvPlatformPersistWrite();    
        #endif
        }

    }

#endif

    /* store the result where it can be picked up and retrieve any registered callback functions. */
    pResult = UtvGetAsyncResultObject();
    pResult->rOperationResult = rStatus;

    /* no other data associated with this result. */
    pResult->pOperationResultData = NULL;

    /* if a callback function has been provided then call it. */
    if ( NULL != ( pCallback = UtvGetCallbackFunction() ))
        (pCallback)( pResult );

    return;
}

static void *UtvInstallComponentsWorkerAsThread( void *placeholder )
{
    UtvInstallComponentsWorker( );
    UTV_THREAD_EXIT;
}

#ifdef UTV_INTERACTIVE_MODE

/* UtvPublicGetDownloadSchedule
     Called to get a list of start times for compatible 01ofYY modules on the network.
     Accepts an optional callback function which when specified spawns scan as a seperate thread
     and returns result via a UTV_PUBLIC_RESULT_OBJECT pointer.
    If called synchronously returns UTV_OK and fills out UTV_PUBLIC_SCHEDULE pointer, otherwise returns error.
*/
UTV_RESULT UtvPublicGetDownloadSchedule( UTV_BOOL bStoredOnly, UTV_BOOL bFactoryMode, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken, 
                                         UTV_PUBLIC_SCHEDULE **pSchedule, UTV_BOOL bCoordinateDownloadScheduleAndDownloadImage )
{
    UTV_PUBLIC_RESULT_OBJECT  *pResultObject = UtvGetAsyncResultObject();
    UTV_THREAD_HANDLE    hThread;

    /* store the callback function where it can be picked up. */
    UtvSetCallbackFunction( pCallback );

    /* store the token in case the callback function has been set. */
    pResultObject->iToken  = iToken;

    /* store a bad result as default */
    pResultObject->rOperationResult  = UTV_UNKNOWN;
    pResultObject->pOperationResultData  = NULL;

    /* set static stored only to get picked up by worker */
    s_bStoredOnly = bStoredOnly;
    s_bCoordinateDownloadScheduleAndDownloadImage = bCoordinateDownloadScheduleAndDownloadImage;

    /* set factory mode (or not).
    */
    UtvSetFactoryMode( bFactoryMode );

    /* if the caller has requested an asynch schedule retrieval then spawn this
       otherwise just call it directly. */
    if ( NULL != pCallback )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: UtvGetDownloadScheduleWorker from\n%s:%d", __FILE__, __LINE__);
#endif
        return UtvThreadCreate( &hThread, UtvGetDownloadScheduleWorkerAsThread, NULL );
    }

    /* call function directly, results will be placed in result object */
    UtvGetDownloadScheduleWorker();

    /* return a pointer to the schedule */
    *pSchedule = (UTV_PUBLIC_SCHEDULE *) pResultObject->pOperationResultData ;

    return pResultObject->rOperationResult;
}

/* UtvPublicDownloadComponents
     Called to initiate a download and store/install of specified components.
     Accepts an optional callback function which when specified spawns scan as a separate thread
     and returns result via a UTV_PUBLIC_RESULT_OBJECT pointer.
    If called synchronously returns download attempt result.
*/
UTV_RESULT UtvPublicDownloadComponents(  UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_BOOL bFactoryMode, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken )
{
    UTV_PUBLIC_RESULT_OBJECT  *pResultObject = UtvGetAsyncResultObject();
    UTV_THREAD_HANDLE    hThread;

    /* store the callback function where it can be picked up. */
    UtvSetCallbackFunction( pCallback );

    /* store the token in case the callback function has been set. */
    pResultObject->iToken  = iToken;

    /* store a bad result as default */
    pResultObject->rOperationResult  = UTV_UNKNOWN;

    /* store slot pointer in operation data so it can be picked up by the worker. */
    pResultObject->pOperationResultData  = pUpdate;

    /* enable factory mode and decrypt (or not).
       Note that although factory mode may be enabled here it is not permitted until
       the presence of a factory mode descriptor is detected in the DSI.
    */
    UtvSetFactoryMode( bFactoryMode );

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
    UtvEnableFactoryDecrypt( bFactoryMode );
#endif

    /* Set the state to download starting immediately. */
    UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD, 0 );

    /* Clear the total blocks received count */
    g_pUtvDiagStats->uiTotalReceivedByteCount = 0;

    /* if the caller has requested an asynch schedule retrieval then spawn this
       otherwise just call it directly. */
    if ( NULL != pCallback )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: UtvDownloadComponentsWorker from\n%s:%d", __FILE__, __LINE__);
#endif
        return UtvThreadCreate( &hThread, UtvDownloadComponentsWorkerAsThread, NULL );
    }

    /* call function directly, results will be placed in result object */
    UtvDownloadComponentsWorker();

    return pResultObject->rOperationResult;
}

/* UtvPublicInstallComponents
     Called to initiate an installation of specified components.
     Accepts an optional callback function which when specified spawns scan as a separate thread
     and returns result via a UTV_PUBLIC_RESULT_OBJECT pointer.
    If called synchronously returns download attempt result.
*/
UTV_RESULT UtvPublicInstallComponents( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_BOOL bFactoryMode, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken )
{
    UTV_PUBLIC_RESULT_OBJECT  *pResultObject = UtvGetAsyncResultObject();
    UTV_THREAD_HANDLE    hThread;

    /* store the callback function where it can be picked up. */
    UtvSetCallbackFunction( pCallback );

    /* store the token in case the callback function has been set. */
    pResultObject->iToken  = iToken;

    /* store a bad result as default */
    pResultObject->rOperationResult  = UTV_UNKNOWN;

    /* store slot pointer in operation data so it can be picked up by the worker. */
    pResultObject->pOperationResultData  = pUpdate;

    /* enable factory mode and decrypt (or not).
       Note that although factory mode may be enabled here it is not permitted until
       the presence of a factory mode descriptor is detected in the DSI.
    */
    UtvSetFactoryMode( bFactoryMode );

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
    UtvEnableFactoryDecrypt( bFactoryMode );
#endif

    /* Set the state to download starting immediately. */
    UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD, 0 );

    /* Clear the total blocks received count */
    g_pUtvDiagStats->uiTotalReceivedByteCount = 0;

    /* if the caller has requested an asynch schedule retrieval then spawn this
       otherwise just call it directly. */
    if ( NULL != pCallback )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: UtvInstallComponentsWorker from\n%s:%d", __FILE__, __LINE__);
#endif
        return UtvThreadCreate( &hThread, UtvInstallComponentsWorkerAsThread, NULL );
    }

    /* call function directly, results will be placed in result object */
    UtvInstallComponentsWorker();

    return pResultObject->rOperationResult;
}

/* UtvPublicGetStatus */
UTV_RESULT UtvPublicGetStatus( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_PUBLIC_STATE *eState, UTV_PUBLIC_SSTATE *eSubState, UTV_PUBLIC_ERR_STAT **ptError, UTV_UINT32 *puiPercentComplete )
{
    *eState = g_utvAgentPersistentStorage.utvState.utvState;
    *eSubState = g_utvAgentPersistentStorage.utvDiagStats.utvSubState;
    *ptError = &g_utvAgentPersistentStorage.utvDiagStats.utvLastError;

    /* if the state is downloading then report the percent complete
       else we're done. */
    if ( UTV_PUBLIC_STATE_DOWNLOAD != *eState )
    {
        *puiPercentComplete = 0;
        return UTV_OK;
    }

    if ( 0 != ( pUpdate->uiDownloadSize / 100 ) )
    {
        *puiPercentComplete = g_pUtvDiagStats->uiTotalReceivedByteCount / ( pUpdate->uiDownloadSize / 100 );
        if(*puiPercentComplete > 100)
            *puiPercentComplete = 100;
    }
    else
        *puiPercentComplete = 0;

    return UTV_OK;
}

UTV_RESULT UtvPublicGetUpdateTotalSize( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_UINT32 *puiUpdateSize )
{
    UTV_UINT32 c;
    *puiUpdateSize = 0;
    for ( c = 0; c < pUpdate->uiNumComponents; c++ )
    {
        /*if ( !pUpdate->tComponents[ c ].bApprovedForStorage && !pUpdate->tComponents[ c ].bApprovedForInstall )
            continue;*/
        *puiUpdateSize += Utv_32letoh( pUpdate->tComponents[ c ].pCompDesc->uiImageSize );
    }
    return UTV_OK;
}

UTV_RESULT UtvPublicGetPercentComplete( UTV_UINT32 totalSize, UTV_UINT32 *puiPercentComplete )
{

    /* if the state is downloading then report the percent complete
       else we're done. */
    if ( UTV_PUBLIC_STATE_DOWNLOAD != g_utvAgentPersistentStorage.utvState.utvState )
    {
        *puiPercentComplete = 0;
        return UTV_OK;
    }

    if ( 0 != ( totalSize / 100 ) )
    {
        *puiPercentComplete = g_pUtvDiagStats->uiTotalReceivedByteCount / ( totalSize / 100 );
        if(*puiPercentComplete > 100)
            *puiPercentComplete = 100;
    }
    else
        *puiPercentComplete = 0;

    return UTV_OK;
}

#endif

UTV_RESULT UtvPublicSetUpdatePolicy( UTV_BOOL bInstallWithoutStore, UTV_BOOL bConvertOptional )
{
    s_bInstallWithoutStore = bInstallWithoutStore;
    s_bConvertOptional = bConvertOptional;

    return UTV_OK;
}

UTV_RESULT UtvPublicApproveForDownload( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_UINT32 uiStore )
{
    UTV_UINT32  i;

    /* you can't approve an update for download if it's already downloaded (file) */
    if ( UTV_PUBLIC_DELIVERY_MODE_FILE == pUpdate->uiDeliveryMode )
        return UTV_BAD_DELIVERY_MODE;

    pUpdate->hStore = UTV_PLATFORM_UPDATE_STORE_0_INDEX;
    for ( i = 0; i < pUpdate->uiNumComponents; i++ )
    {
        if ( s_bInstallWithoutStore )
        {
            pUpdate->tComponents[ i ].bApprovedForInstall = UTV_TRUE;
            pUpdate->tComponents[ i ].bApprovedForStorage = UTV_FALSE;
        } else
        {
            pUpdate->tComponents[ i ].bApprovedForInstall = UTV_FALSE;
            pUpdate->tComponents[ i ].bApprovedForStorage = UTV_TRUE;
        }
    }

    return UTV_OK;
}

UTV_RESULT UtvPublicApproveForInstall( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_UINT32  i;

    for ( i = 0; i < pUpdate->uiNumComponents; i++ )
        pUpdate->tComponents[ i ].bApprovedForInstall = UTV_TRUE;

    return UTV_OK;
}

UTV_BOOL UtvGetInstallWithoutStoreStatus( void )
{ return s_bInstallWithoutStore; }
