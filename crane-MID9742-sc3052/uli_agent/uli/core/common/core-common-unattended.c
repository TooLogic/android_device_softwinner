/* core-common-unnattended.c : UpdateTV Common Core -  top-level non-public entry points into the core.

   Main control logic for unattended mode of the UpdateTV Agent.

   In order to ensure UpdateTV network compatibility, customers must not
   change UpdateTV Common Core code.

   Copyright (c) 2004 - 2011 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      02/08/2011  Changed arguments to UtvPlatformInstallCleanup().
   Jim Muchow      09/21/2010  BE support.
   Bob Taylor      04/23/2010  Fixed uninit variable warning.
   Bob Taylor      03/15/2010  OOB milestone support.
   Bob Taylor      02/03/2010  Check for abort prior to download tracking clear.  Broadcast only.
                               Added calls to UtvPlatformInstallCleanup() to close partitions/files on exit. 
   Bob Taylor      11/05/2009  UtvBroadcastDownloadGenerateSchedule() added to check stored cm.
                               Changed check of UtvUpdateInstallComplete() to UTV_DOWNLOAD_COMPLETE.
   Bob Taylor      06/30/2010  New UtvImageGetTotalModuleCount() arg to get correct module count value.
   Bob Taylor      01/26/2009  Created from 1.9.21
*/

#include "utv.h"
#include <stdio.h>
#include <string.h>

/* Forward routines */
UTV_RESULT UtvScanEvent( void );
UTV_RESULT UtvDownloadEvent( void );

/* extern access */
extern UTV_MUTEX_HANDLE ghUtvStateMutex;

/* UtvAgentTask
    Called by UtvApplication to run the UtvAgent for one event
    Upon return, the next event and the delay time until that event is set.
    The UtvAgent state information is kept in the structure whose pointer is included in the call
*/
void UtvAgentTask( )
{
    UTV_BYTE * pState;
    UTV_RESULT rStatus;
    UTV_PUBLIC_STATE utvCurrState = UtvGetState();

    /* if the state is somehow illegal then reset it to scan here. */
    if ( (UTV_UINT32) utvCurrState > (UTV_UINT32) UTV_PUBLIC_STATE_EXIT )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Illegal system state.  Resetting to Scan" );
        utvCurrState = UTV_PUBLIC_STATE_SCAN;
        UtvSetState( UTV_PUBLIC_STATE_SCAN );
    }

    /* Initalize the return status */
    rStatus = UTV_OK;

    /* The state determines the next action */
    switch ( utvCurrState )
    {
        /* Scan Event */
        case UTV_PUBLIC_STATE_SCAN:
        {
            /* Assume next event will be a scan after a long wait */
            /* UtvScanEvent may change the next state or sleep time */
            rStatus = UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_LONG );
            if ( rStatus != UTV_OK )
                break;


            /* Do the scan event */
            rStatus = UtvScanEvent();
            switch ( rStatus )
            {
            case UTV_OK:
                /* Count good scan, next state and time are set elsewhere */
                g_pUtvDiagStats->iScanGood++;
                break;

            case UTV_ABORT:
                /* Keep track of aborted scans */
                g_pUtvDiagStats->iScanAborted++;
                break;

            default:
                /* Everything else is "bad" (incompatible, already have, etc.) */
                g_pUtvDiagStats->iScanBad++;
                break;
            }
            break;
        }

        /* Download Event */
        case UTV_PUBLIC_STATE_DOWNLOAD:
        {
            /* Try the download event */
            rStatus = UtvDownloadEvent();
            switch ( rStatus )
            {
            case UTV_OK:
                /* Count partial downloads (got some modules and then no more found, too far in future, etc) */
                g_pUtvDiagStats->iDownloadPartial++;
                break;

            case UTV_ABORT:
                /* Keep track of aborted downloads */
                g_pUtvDiagStats->iDownloadAborted++;
                break;

            case UTV_DOWNLOAD_COMPLETE:
#ifdef UTV_DELIVERY_BROADCAST
                /* record stats related to the most recent download. */
                UtvSaveLastDownloadTime();
#endif

                /* Count complete download(s) (all modules delivered) */
                g_pUtvDiagStats->iDownloadComplete++;
                break;
            default:
                /* Everything else is "bad" (none found, too far in future, etc) */
                g_pUtvDiagStats->iDownloadBad++;
                break;
            }

            /* Next event after a download is usually a scan event after a short sleep */
            /*  but if an update was completely downloaded, the next event will be a */
            /*  download complete event after a minimal sleep */
            if ( UtvGetState() != UTV_PUBLIC_STATE_ABORT && UtvGetState() != UTV_PUBLIC_STATE_EXIT )
            {
                if ( rStatus == UTV_DOWNLOAD_COMPLETE )
                {
                    /* new state in 5 seconds. */
                    UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD_DONE, UTV_WAIT_DOWNLOAD_DONE_DELAY );
                }
                else
                {
                    /* if there is a module to schedule in the future, we may need to schedule the download event now */
                    if ( rStatus == UTV_FUTURE )
                    {
                        /* if the module is not far enough in the future to be scheduled by the next scan event */
                        /*  schedule another download event so we don't miss this module */
                        if( g_pUtvBestCandidate->iMillisecondsToStart < ( UTV_WAIT_SLEEP_SHORT + UTV_MAX_CHANNEL_SCAN ) )
                        {
                            UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD, g_pUtvBestCandidate->iMillisecondsToStart - UTV_WAIT_UP_EARLY );
                            break;
                        }
                    }
                    /* this catches all cases, error or not where downloading does not */
                    /* complete in a single call to UtvDownloadEvent().  Revert to scan mode. */
                    UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_SHORT );
                }
            }
            break;
        }
        /* Download Complete Event */
        case UTV_PUBLIC_STATE_DOWNLOAD_DONE:
        {
            UtvMessage( UTV_LEVEL_INFO, "Begin UpdateTV Download Complete Event - installing new update, receiver may re-boot" );

            /* set diagnostic sub-state to indicate we're storing an image */
            UtvSetSubState( UTV_PUBLIC_SSTATE_STORING_IMAGE, 0 );

#if _download_complete_handling
            /* The download just completed, call pers in case a reboot is needed */
            rStatus = UtvDownloadComplete( );
#endif

            UtvMessage( UTV_LEVEL_INFO, "End UpdateTV Download Complete Event\n\n" );

            /* Next event after a download complete is a scan event after a short sleep */
            if ( UtvGetState() != UTV_PUBLIC_STATE_ABORT && UtvGetState() != UTV_PUBLIC_STATE_EXIT )
            {
                UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_SHORT );
            }
            break;
        }
        /* Abort state just falls through */
        default:
        {
            break;
        }
    }

    /* Check for abort (could be set during one of the other events) */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
    {
#ifdef UTV_TEST_ABORT
        UtvAbortComplete();
#endif  /* UTV_TEST_ABORT */

        /* Hardware and sofware resources have already been released by now */
        UtvMessage( UTV_LEVEL_WARN, "UpdateTV Abort requested, resetting state to scan and time to %d seconds\n\n", UTV_WAIT_SLEEP_SHORT / 1000 );

        /* Since the state is ABORT, we have to clear it in order to set another state */
        UtvClearState();

        /* if exit request then handle it here */
        if (UtvGetExitRequest())
            UtvSetState( UTV_PUBLIC_STATE_EXIT );
        else
        {
            /* Set the next event for 30 minutes in the future - the default is a scan event, but if a download complete */
            /*  was in progress but didn't complete, schedule the event again so the image can be installed */
            if ( ( utvCurrState == UTV_PUBLIC_STATE_DOWNLOAD_DONE ) && ( rStatus == UTV_ABORT ) )
                rStatus = UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD_DONE, UTV_WAIT_SLEEP_SHORT );
            else
                rStatus = UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_SHORT );
        }
    }
    

    /* the following return codes are informational and should not tigger the recording of an error. */
    if ( rStatus != UTV_OK && rStatus != UTV_DOWNLOAD_COMPLETE && rStatus != UTV_NO_DOWNLOAD && rStatus != UTV_FUTURE && rStatus != UTV_ABORT )
    {
        UtvSetErrorCount( rStatus, 0 );
        UtvMessage( UTV_LEVEL_ERROR, "UtvAgentTask - Error returned - %s\n\n", UtvGetMessageString( rStatus ) );
    }

    /* report high water if asked to */
#ifdef UTV_TEST_REPORT_HIGH_WATER
    UtvMessage( UTV_LEVEL_INFO, "UtvAgentTask - Section Queue High Water Mark: %u\n\n", UtvGetSectionsHighWater() );
#endif

#ifdef UTV_DELIVERY_BROADCAST
    /* detect and report overflows */
    {
        UTV_UINT32 uiOverflows;
        if ( 0 != (uiOverflows = UtvGetSectionsOverflowed()) )
        {
            UtvSetErrorCount( UTV_SECTION_QUEUE_OVERFLOW, 0 );
            UtvMessage( UTV_LEVEL_ERROR, "UtvAgentTask - Error returned - %d section queue overflows reported\n\n", uiOverflows );
            /* reset high water mark back to zero after detecting overflow. */
            UtvClearSectionsHighWater();
        }
    }
#endif

#ifdef _implemented
    /* Output summary before returning */
    UtvStatsSummary();
#endif

    if ( UtvGetState() != UTV_PUBLIC_STATE_EXIT )
    {
        switch ( UtvGetState() )
        {
            case UTV_PUBLIC_STATE_DOWNLOAD:
                pState = ( UTV_BYTE * )"Download";
                break;

            case UTV_PUBLIC_STATE_DOWNLOAD_DONE:
                pState = ( UTV_BYTE * )"Download Complete";
                break;

            case UTV_PUBLIC_STATE_SCAN:
            default:
                pState = ( UTV_BYTE * )"Scan";
                break;
        }
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "Next agent task is %s Event in %d Seconds\n", pState, UtvGetSleepTime() / 1000 );
    }

       /* Update persistent memory with the status of the Agent. */
    UtvPlatformPersistWrite();
}


static UTV_RESULT DownloadSynchronous( void )
{
    UTV_RESULT              rStatus;
    UTV_PUBLIC_SCHEDULE    *pSchedule;

    /* STEP ONE:  Retrieve the components that are available for download() */

    if ( UTV_OK != ( rStatus = UtvPublicGetDownloadSchedule( UTV_FALSE, UTV_FALSE, NULL, 0, &pSchedule, 
                                                             UTV_TRUE ) /* set to true to maintain previous behavior */ ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPublicGetDownloadSchedule( UTV_FALSE, NULL, 0, &pSchedule ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );        
        return rStatus;
    } else
    {
        UtvMessage( UTV_LEVEL_INFO, "UtvPublicGetDownloadSchedule( UTV_FALSE, NULL, 0, &pSchedule ) success" );
        if ( NULL != pSchedule )
            UtvDisplaySchedule(pSchedule);
        else
        {
            UtvMessage( UTV_LEVEL_ERROR, "Retured schedule pointer is NULL" );
            return rStatus;
        }
    }
    
    /* STEP TWO:  Choose the first update [0] from among those returned by UtvPublicGetUpdateschedule().
                  If it's a file delivery (USB) skip the rest of this step.  It's already downloaded.
                  If it's optional ask the user.
                  If it's optional and ok'd or mandatory then approve it for download.
                  Provide a store index to be used.
                  Sleep until the download is available. 
    */
    if ( UTV_PUBLIC_DELIVERY_MODE_FILE != pSchedule->tUpdates[ 0 ].uiDeliveryMode )
    {
        if ( UTV_OK != ( rStatus = UtvPublicApproveForDownload( &pSchedule->tUpdates[ 0 ], UTV_PLATFORM_UPDATE_STORE_0_INDEX )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicApproveUpdate( &pSchedule->tUpdates[ 0 ], UTV_PLATFORM_UPDATE_STORE_0_INDEX ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );        
            return rStatus;
        }

        UtvMessage( UTV_LEVEL_INFO, "Sleeping %d seconds until the selected download is available", pSchedule->tUpdates[ 0 ].uiSecondsToStart[ 0 ] );
        UtvSleep( pSchedule->tUpdates[ 0 ].uiSecondsToStart[ 0 ] * 1000 );
        
        rStatus = UtvPublicDownloadComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 );
        if ( UTV_DOWNLOAD_COMPLETE != rStatus && UTV_UPD_INSTALL_COMPLETE != rStatus )
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicDownloadComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );  
            return rStatus;
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "UtvPublicDownloadComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) success" );
        }
    }

    /* STEP THREE:  Choose to install components that have been stored in the previous step. 
                     Thius isn't necessary if it was performed during download. 
    */
    if ( !UtvGetInstallWithoutStoreStatus() )
    {
        if ( UTV_OK != ( rStatus = UtvPublicApproveForInstall( &pSchedule->tUpdates[ 0 ] )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicApproveForInstall( &pSchedule->tUpdates[ 0 ] ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );        
            return rStatus;
        }

        if ( UTV_UPD_INSTALL_COMPLETE != ( rStatus = UtvPublicInstallComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) ))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicInstallComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );        
            return rStatus;
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "UtvPublicInstallComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) success" );
        }
    }
  
    return rStatus;
}

/* UtvScanEvent
    Called to process a scan event from the process control
    Changes the sleep time to short if the tv hardware is busy
    Called with next event set to scan and sleep time set to long
   Returns OK if scan found an upcoming download
*/


UTV_RESULT UtvScanEvent( )
{
    UTV_RESULT              rStatus;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "Begin UpdateTV Scan Event" );

    /* Obtain TV hardware, return in 30 mins if we can't use the hardware */
    rStatus = UtvPlatformCommonAcquireHardware( UTV_PUBLIC_STATE_SCAN );
    if ( UTV_OK != rStatus )
    {
        UtvMessage( UTV_LEVEL_ERROR | UTV_LEVEL_TEST, " Failed to acquire hardware: %s\n\n", UtvGetMessageString( rStatus ) );
        /* Sleep for a short time then try scan */
        if ( UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_SHORT ) != UTV_OK )
            UtvMessage( UTV_LEVEL_ERROR, " UtvScanEvent - error unable to set state\n\n" );
        return rStatus;
    }

#ifdef UTV_TEST_ABORT
    UtvAbortAcquireHardware();
#endif  /* UTV_TEST_ABORT */

    /* Check if anything is in the stores.  This is always synchronous because it happens quickly. */
    if ( UTV_OK != ( rStatus = DownloadSynchronous() ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "DownloadSynchronous() fails with error \"%s\"", UtvGetMessageString( rStatus ) );        
    } else
    {
        UtvMessage( UTV_LEVEL_INFO, "DownloadSynchronous() success" );
    }

    if ( rStatus == UTV_OK )
    {
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " UtvScanEvent - updating schedule time for download\n" );

#ifdef UTV_DELIVERY_BROADCAST
        /* Recalculate the time left to download the module since the channel scan can be quite lengthy */
        rStatus = UtvCalcTimeToDownload( g_pUtvBestCandidate );
#endif
        if ( rStatus == UTV_OK )
        {
            /* Renewal on carousel!  We have a upcoming download! */
            /* Wake up early to allow for the carousel scan to complete before the module is broadcast */
            if ( g_pUtvBestCandidate->iMillisecondsToStart > UTV_WAIT_UP_EARLY )
                g_pUtvBestCandidate->iMillisecondsToStart -= UTV_WAIT_UP_EARLY;
            else
                g_pUtvBestCandidate->iMillisecondsToStart = 0;

            /* Set the next event as download, set elapsed time until upcoming event */
            rStatus = UtvSetStateTime( UTV_PUBLIC_STATE_DOWNLOAD, g_pUtvBestCandidate->iMillisecondsToStart );
            UtvMessage( UTV_LEVEL_INFO, " scheduling Download Event in %d seconds", g_pUtvBestCandidate->iMillisecondsToStart / 1000 );
        }
    }
    if ( rStatus != UTV_OK && rStatus != UTV_NO_DOWNLOAD )
    {
        /* Log scan error */
        UtvMessage( UTV_LEVEL_ERROR, "   channel scan failure: %s", UtvGetMessageString( rStatus ) );

        /* There was some time of error, so scan again in short order */
        UtvSetStateTime( UTV_PUBLIC_STATE_SCAN, UTV_WAIT_SLEEP_SHORT );

    }

    /* Release ownership of hardware */
    UtvPlatformCommonReleaseHardware();

#ifdef UTV_TEST_ABORT
    UtvAbortReleaseHardware();
#endif  /* UTV_TEST_ABORT */

    /* End of scan event */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "End UpdateTV Scan Event\n\n" );
    return rStatus;
}

#ifdef UTV_DELIVERY_BROADCAST
static UTV_RESULT getDownloadWorkerBroadcast()
{
    UTV_RESULT rStatus = UTV_OK, lStatus;
    UTV_UINT32 iDownloadsCompleted, uiTargetDownloads;
    UtvImageAccessInterface        utvIAI;
    UTV_BOOL                       bOpen, bPrimed, bAssignable, bManifest, bAlreadyHave, bAnyApprovedForInstall = UTV_FALSE;
    UTV_UINT32                     uiBytesWritten, c, d;
    UtvCompDirHdr                 *pCompDirHdr;
    UtvRange                      *pHWMRanges, *pSWVRanges;
    UtvDependency                 *pSWDeps;
    UtvTextDef                    *pTextDefs;
    UTV_UINT32                     lPercentComplete = 0;
    UTV_UINT32                     lPercentCompleteTMP = 0;

    /* if we don't have a component directory yet, see if we've already stored one. */
    if ( g_pUtvBestCandidate->pUpdate->hImage == UTV_CORE_IMAGE_INVALID )
    {
        bAlreadyHave = UTV_FALSE;
        do 
        {
            /* get the file version of the image access interface */
            if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
            {
                UtvMessage( UTV_LEVEL_WARN, "UtvFileImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                break;
            }

            /* try to open it */
            if ( UTV_OK != ( rStatus = UtvPlatformStoreAttributes( g_pUtvBestCandidate->pUpdate->hStore, &bOpen, &bPrimed, &bAssignable, &bManifest, &uiBytesWritten )))
            {
                UtvMessage( UTV_LEVEL_WARN, "UtvPlatformStoreAttributes( %d ) fails with error \"%s\"", g_pUtvBestCandidate->pUpdate->hStore, UtvGetMessageString( rStatus ) );
                break;
            }

            /* attempt to open this image in the store */
            if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)g_pUtvBestCandidate->pUpdate->hStore, &utvIAI, &g_pUtvBestCandidate->pUpdate->hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
            {
                UtvMessage( UTV_LEVEL_WARN, "UtvImageOpen( %d ) fails with error \"%s\"", g_pUtvBestCandidate->pUpdate->hStore, UtvGetMessageString( rStatus ) );
                break;
            }

            /* get the component directory */
            if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( g_pUtvBestCandidate->pUpdate->hImage, &pCompDirHdr, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs )))
            {
                UtvMessage( UTV_LEVEL_WARN, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                break;
            }

            /* check the module version */
            if ( Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ) != g_pUtvBestCandidate->pUpdate->usModuleVersion )
            {
                UtvMessage( UTV_LEVEL_WARN, "Existing COMPONENT DIRECTORY MV 0x%04X disagrees with update MV 0x%04X", 
                            Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ), 
                            g_pUtvBestCandidate->pUpdate->usModuleVersion );
                break;
            }

            bAlreadyHave = UTV_TRUE;

   
        } while ( UTV_FALSE );

        /* check for abort prior to download clear */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        {
            return UTV_ABORT;
        }

        /* if we don't have it then get it */
        if ( !bAlreadyHave )
        {
            UtvMessage( UTV_LEVEL_INFO, "Don't yet have signaled COMPONENT DIRECTORY with MV 0x%04X, downloading.", g_pUtvBestCandidate->pUpdate->usModuleVersion );  
            
            /* kill the tracking info, no such thing as valid tracking info without an associated component directory stored to update-0.store  */
            UtvCoreCommonCompatibilityDownloadClear();

            if ( UTV_OK != ( rStatus = UtvBroadcastDownloadComponentDirectory( g_pUtvBestCandidate )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvBroadcastDownloadComponentDirectory(): %s", UtvGetMessageString( rStatus ) );
                return rStatus;
            }
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "Already have COMPONENT DIRECTORY with MV 0x%04X.", g_pUtvBestCandidate->pUpdate->usModuleVersion );  
            if ( UTV_OK != ( rStatus = UtvBroadcastDownloadGenerateSchedule( g_pUtvBestCandidate )))
            {
                UtvMessage( UTV_LEVEL_WARN, "Schedule generation from stored component directory failed, downloading a new one." );  
                UtvCoreCommonCompatibilityDownloadClear();
            } else
            {
                UtvCoreCommonCompatibilityDownloadStart( g_pUtvBestCandidate->pUpdate->hImage, g_pUtvBestCandidate->pUpdate->usModuleVersion );
            }
        }
    }
    
    /* we should have the component directory by now, how many modules are we supposed to get from all components? */
    if ( UTV_OK != ( rStatus = UtvImageGetTotalModuleCount( g_pUtvBestCandidate->pUpdate->hImage, g_pUtvBestCandidate->pUpdate, &uiTargetDownloads )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageGetTotalModuleCount(): %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* recover from the case where we have an illegal number fo target downloads */ 
    if ( 0 == uiTargetDownloads )
    {
        UtvMessage( UTV_LEVEL_ERROR, "   Recovering from zero target download error" );
        UtvCoreCommonCompatibilityDownloadClear();
        UtvImageGetTotalModuleCount( g_pUtvBestCandidate->pUpdate->hImage, g_pUtvBestCandidate->pUpdate, &uiTargetDownloads );
        /* if still zero then punt */
        if ( 0 == uiTargetDownloads )
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvImageGetTotalModuleCount(): still zero target downloads" );
            return UTV_COUNTS_DISAGREE;
        }
    }

    UtvMessage( UTV_LEVEL_INFO, "   Looking for %d module downloads", uiTargetDownloads );

    /* scan components for size (used for progress) and install status */
    for ( c = 0, g_pUtvBestCandidate->pUpdate->uiDownloadSize = 0; c < g_pUtvBestCandidate->pUpdate->uiNumComponents; c++ )
    {
        if ( g_pUtvBestCandidate->pUpdate->tComponents[ c ].bApprovedForInstall )
            bAnyApprovedForInstall = UTV_TRUE;
        
        if ( !g_pUtvBestCandidate->pUpdate->tComponents[ c ].bApprovedForStorage && !g_pUtvBestCandidate->pUpdate->tComponents[ c ].bApprovedForInstall )
            continue;
        g_pUtvBestCandidate->pUpdate->uiDownloadSize += Utv_32letoh( g_pUtvBestCandidate->pUpdate->tComponents[ c ].pCompDesc->uiImageSize );
    }

    /* Get multiple modules in a row, up to the maximum */
    iDownloadsCompleted = 0;
    while ( UTV_OK == rStatus || ( UTV_COMP_INSTALL_COMPLETE == rStatus && iDownloadsCompleted < uiTargetDownloads ) )
    {
        /* See if we need a message */
        if ( iDownloadsCompleted > 0 )
            UtvMessage( UTV_LEVEL_INFO, "  looking for up to %d more module(s)", uiTargetDownloads - iDownloadsCompleted );

        /* Try (another) download event */
        rStatus = UtvBroadcastDownloadModule( g_pUtvBestCandidate );
        if ( UTV_OK == rStatus || UTV_COMP_INSTALL_COMPLETE == rStatus )
        {
            /* Reset the dtDownload time to indicate to external probes that we are done with */
            /* this download. */
            g_pUtvBestCandidate->dtDownload = 0;

            /* Count one as done as we only do so many in a row */
            iDownloadsCompleted++;

            /* Record where the last successful module download came from. */
            UtvSaveServerIDModule();

            /* Record the number of modules downloaded so far */
            UtvIncModuleCount();

            /* Record the frequency where the last successful module download came from. */
            g_pUtvDiagStats->iLastModuleFreq = g_pUtvBestCandidate->pUpdate->uiFrequency;

            /* Record the PID where the last successful module download came from. */
            g_pUtvDiagStats->iLastModulePid = (UTV_UINT16) g_pUtvBestCandidate->iCarouselPid;

            /* Record last broadcast time that a module was received. */
            UtvSaveLastModuleTime();

            /* Update persistent memory with the status of the Agent. */
            UtvPlatformPersistWrite();
        }

        /* if we're done then let the platform code know */
        if ( iDownloadsCompleted == uiTargetDownloads )
        {    
            rStatus = UTV_DOWNLOAD_COMPLETE;

            /* prevent a failure of the commit from leaving us thinking we have all of the modules */
            UtvCoreCommonCompatibilityDownloadClear();

            /* clean up any open partitions/files before calling UtvInstallUpdateComplete()
               This is to catch any discrepancy between module counting in this while loop and
               the module counting within UtvPlatformInstallModuleData(). */
            UtvPlatformInstallCleanup();

            /* store needs to be closed prior to calling UtvInstallUpdateComplete() */
            if ( UTV_OK != ( lStatus = UtvPlatformStoreClose( g_pUtvBestCandidate->pUpdate->hStore )))
            {
                /* tolerate already closed errors */
                if ( UTV_NOT_OPEN != lStatus )
                    UtvMessage( UTV_LEVEL_WARN, " UtvPlatformStoreClose() fails \"%s\"", UtvGetMessageString( lStatus ) );
            }

            /* if this was an install then call update complete */
            if ( bAnyApprovedForInstall )
            {
                if ( UTV_DOWNLOAD_COMPLETE != ( lStatus = UtvInstallUpdateComplete( g_pUtvBestCandidate->pUpdate->hImage, g_pUtvBestCandidate->pUpdate->usModuleVersion )))
                {
                    UtvMessage( UTV_LEVEL_ERROR, " UtvInstallUpdateComplete() fails: %s", UtvGetMessageString( lStatus ) );        
                    return lStatus;
                }
            }

            /* close the image */
            UtvImageClose( g_pUtvBestCandidate->pUpdate->hImage );
        }
    }
    
    /* log per 10 percent block for OOB testing */
    if ( 0 != ( g_pUtvBestCandidate->pUpdate->uiDownloadSize / 100 ) )
        lPercentCompleteTMP = g_pUtvDiagStats->uiTotalReceivedByteCount / ( g_pUtvBestCandidate->pUpdate->uiDownloadSize / 100 );
    else
        lPercentCompleteTMP = 100;
    if( lPercentCompleteTMP > 100 )
        lPercentCompleteTMP = 100;
    for ( d = lPercentComplete+10; d <= lPercentCompleteTMP; d+=10)
    {
        UtvMessage( UTV_LEVEL_OOB_TEST, "OAD: Progress %d %%",d);
    }
    lPercentComplete = d-10;

    if ( 0 != iDownloadsCompleted )
    {
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Number of modules downloaded is %d", iDownloadsCompleted );

        /* if the error indicates no download, it just means there were no more to find at this time, but there was a successful */
        /*  download so indicate that upon return */
        if ( rStatus == UTV_NO_DOWNLOAD )
            rStatus = UTV_OK;
    }

    /* Always close any open partitions/files on the way out of this function so that nothing is left open when the device
       powers down.  This may be a harmless duplicate call in the case of having received all modules */  
    UtvPlatformInstallCleanup();

    return rStatus;
}
#endif


/* UtvDownloadEvent
    Called to process a download event from the process control.
   Returns OK if we got a download and saved it
*/
UTV_RESULT UtvDownloadEvent( )
{
    /* Local storage */
    UTV_RESULT rStatus = UTV_OK;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "Begin UpdateTV Download Event" );

    /* TODO - we should only be allocating the hw we need for the download specified, not all modes */

    /* Obtain delivery specific hardware */
    if ( UTV_OK != (rStatus = UtvPlatformCommonAcquireHardware( UTV_PUBLIC_STATE_DOWNLOAD )))
    {
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Failed to acquire hardware: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

#ifdef UTV_TEST_ABORT
    UtvAbortAcquireHardware();
#endif  /* UTV_TEST_ABORT */

    /* Download is unlike scan wrt multilpe configuration modes.  In download only one configuration mode is 
       active and that's indicated by the g_pUtvBestCandidate machinery.  Determine who which delivery mode
       brought us here and use that mode to intiate the download. */

    switch ( g_pUtvBestCandidate->pUpdate->uiDeliveryMode )
    {
#ifdef UTV_DELIVERY_BROADCAST
    case UTV_PUBLIC_DELIVERY_MODE_BROADCAST:
        rStatus = getDownloadWorkerBroadcast();
        break;
#endif
    default:
        rStatus = UTV_BAD_DELIVERY_MODE;
        break;
    }

    /* Give up TV hardware */
    UtvPlatformCommonReleaseHardware();

#ifdef UTV_TEST_ABORT
    UtvAbortReleaseHardware();
#endif  /* UTV_TEST_ABORT */

    /* End of download event */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "End UpdateTV Download Event\n\n" );
    return rStatus;
}

