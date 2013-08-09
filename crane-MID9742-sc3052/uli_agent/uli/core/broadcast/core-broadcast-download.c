/* core-broadcast-download.c: UpdateTV Common Core - UpdateTV Download Module

  This module implements the UpdateTV Broadcast Download Module functions.

  In order to ensure UpdateTV network compatibility, customers must not
  change UpdateTV Common Core code.

  Copyright (c) 2004 - 2009 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Jim Muchow      09/21/2010  BE support.
  Bob Taylor      01/11/2010  Fixed extra copy of incompatible component bug and small dbg print fix.
  Bob Taylor      11/03/2009  Fixed incompatible component bug.
  Bob Taylor      01/26/2009  Created from version 1.9.21
*/

#include "utv.h"

/* Local worker routines */
UTV_RESULT UtvScanDdb( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pModuleBuffer );
UTV_RESULT UtvParseDdb( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pSectionData,
                        UTV_BYTE *pModuleBuffer, UTV_BYTE * pBlockMap, UTV_UINT32 iMaximumBlockNumber,
                        UTV_UINT32 * piNeedBlockCount, UTV_UINT32 * piBlockSize );
UTV_RESULT UtvMiniScanDdb( UTV_UINT32 uiPID, UTV_UINT32 *puiBlockSize, UTV_UINT32 *puiModuleSize );
UTV_RESULT UtvMiniParseDdb( UTV_BYTE * pSectionData, UTV_UINT32 *puiBlockSize, UTV_UINT32 *puiModuleSize );

/* UtvBroadcastDownloadModule
    Called with tuner set, do a carousel scan to make sure then do a module download
   Returns UTV_OK if module successfully downloaded, decrypted, and stored
*/
UTV_RESULT UtvBroadcastDownloadModule( UtvDownloadInfo * pUtvDownloadInfo  )
{
    UTV_RESULT rStatus;
    UTV_BYTE * pModuleBuffer = NULL;

    /* Set desired frequency */
    rStatus = UtvPlatformDataBroadcastSetTunerState( pUtvDownloadInfo->iFrequency );
    if ( rStatus != UTV_OK )
        return rStatus;

    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  tuned to %d MHz", pUtvDownloadInfo->iFrequency / 1000000 );

    /* Create section filter */
    rStatus = UtvCreateSectionFilter();
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Reset the carousel scanner parameters, scanning this frequency only */
    pUtvDownloadInfo->iMillisecondsToStart = 0;

    /* Do a carousel scan, returning best candidate (if any) on the selected frequency and carousel pid */
    rStatus = UtvCarouselScan( pUtvDownloadInfo->iCarouselPid, pUtvDownloadInfo );
    if ( rStatus != UTV_OK )
    {
        UtvDestroySectionFilter();
        return rStatus;
    }

    /* OK now we have a candidate, if it is too far in the future get out */
    if ( pUtvDownloadInfo->iMillisecondsToStart > UTV_WAIT_SLEEP_SHORT )
    {
        /* Too far in the future for a download call */
        UtvDestroySectionFilter();
        rStatus = UTV_FUTURE;
        return rStatus;
    }

    /* Now we have identified the most likely download candidate, see if we can get storage for it */
    pModuleBuffer = (UTV_BYTE *)UtvPlatformInstallAllocateModuleBuffer( pUtvDownloadInfo->iModuleSize );
    if ( pModuleBuffer == NULL )
    {
        UtvDestroySectionFilter();
        return UTV_NO_MEMORY;
    }

    /* Get the data then dispose of it properly */
    rStatus = UtvBroadcastDownloadCarousel( pUtvDownloadInfo, pModuleBuffer );

    /* Free up the module buffer we allocated  */
    UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );

    /* Destroy the section filter */
    UtvDestroySectionFilter();

    /* Done here */
    return rStatus;
}

/* UtvDownloadComponentDirectory
    Sets tuner, does carousel scan to check on availablity of component directory,
    downloads it, calls image write
    Returns UTV_OK if component directory successfully downloaded, decrypted, and stored
*/
UTV_RESULT UtvBroadcastDownloadComponentDirectory( UtvDownloadInfo * pUtvDownloadInfo )
{
    UTV_RESULT                              rStatus;
    UtvImageAccessInterface                 utvIAI;
    UTV_BYTE                               *pModuleBuffer = NULL;
	UTV_UINT32                              hStore;

    /* Set desired frequency */
    rStatus = UtvPlatformDataBroadcastSetTunerState( pUtvDownloadInfo->iFrequency );
    if ( rStatus != UTV_OK )
        return rStatus;

    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  tuned to %d MHz", pUtvDownloadInfo->iFrequency / 1000000 );

    /* Create section filter */
    rStatus = UtvCreateSectionFilter();
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Reset the carousel scanner parameters, scanning this frequency only */
    pUtvDownloadInfo->iMillisecondsToStart = 0;

    /* Do a carousel scan, returning best candidate (if any) on the selected frequency and carousel pid */
    rStatus = UtvCarouselScan( pUtvDownloadInfo->iCarouselPid, pUtvDownloadInfo );
    if ( rStatus != UTV_OK )
    {
        UtvDestroySectionFilter();
        return rStatus;
    }

    /* OK now we have a candidate, if it is too far in the future get out */
    if ( pUtvDownloadInfo->iMillisecondsToStart > UTV_WAIT_SLEEP_SHORT )
    {
        /* Too far in the future for a download call */
        UtvDestroySectionFilter();
        rStatus = UTV_FUTURE;
        return rStatus;
    }

    /* Now we have identified the most likely download candidate, see if we can get storage for it */
    pModuleBuffer = (UTV_BYTE *)UtvPlatformInstallAllocateModuleBuffer( pUtvDownloadInfo->iModuleSize );
    if ( pModuleBuffer == NULL )
    {
        UtvDestroySectionFilter();
        return UTV_NO_MEMORY;
    }

    /* Grab all of the blocks for this module from the DDBs  */
    if ( UTV_OK != ( rStatus = UtvScanDdb( pUtvDownloadInfo, pModuleBuffer )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "  UtvScanDdb error: %s", UtvGetMessageString( rStatus ) );
        UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );
        UtvDestroySectionFilter();
        return rStatus;
    }

    /* get the broadcast version of the image access interface which allows provides access to the memory buffer just downloaded */
    if ( UTV_OK != ( rStatus = UtvBroadcastImageAccessInterfaceGet( &utvIAI )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvBroadcastImageAccessInterfaceGet() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );
        UtvDestroySectionFilter();
        return rStatus;
    }

    /* attempt to open this image which will download and decrypt its component directory AND write it to the store */
    if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *) pModuleBuffer, &utvIAI, &pUtvDownloadInfo->pUpdate->hImage, pUtvDownloadInfo->pUpdate->hStore )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageOpen() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );
        UtvDestroySectionFilter();
        return rStatus;
    }

	if ( UTV_OK != ( rStatus = UtvBroadcastDownloadGenerateSchedule( pUtvDownloadInfo )))
	{
		/* complain, kill, and close up shop */
        UtvMessage( UTV_LEVEL_ERROR, "UtvBroadcastDownloadGenerateSchedule() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        UtvPlatformStoreOpen(pUtvDownloadInfo->pUpdate->hStore, UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE | UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE, 0, &hStore, NULL, NULL );
        UtvPlatformStoreClose( pUtvDownloadInfo->pUpdate->hStore );
        UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );
        UtvDestroySectionFilter();
        return rStatus;
	}

    /* if there were any components found then we're good to go */
    if ( 0 != pUtvDownloadInfo->pUpdate->uiNumComponents )
    {
        /* TODO: insert broadcast duration here? */

		/* let the compatibility machinery know which image we're working from */
		UtvCoreCommonCompatibilityDownloadStart( pUtvDownloadInfo->pUpdate->hImage, (UTV_UINT16) pUtvDownloadInfo->iModuleVersion );
		rStatus = UTV_OK;

    } else
    {
        /* blow away the downloaded comp dir for good measure */
        UtvPlatformStoreOpen(pUtvDownloadInfo->pUpdate->hStore, UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE | UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE, 0, &hStore, NULL, NULL );
        UtvPlatformStoreClose( pUtvDownloadInfo->pUpdate->hStore );
        rStatus = UTV_NO_DOWNLOAD;
    }

    /* Free up the module buffer we allocated  */
    UtvPlatformInstallFreeModuleBuffer( pUtvDownloadInfo->iModuleSize, pModuleBuffer );

    /* Destroy the section filter */
    UtvDestroySectionFilter();

    /* Done here */
    return rStatus;
}

UTV_RESULT UtvBroadcastDownloadGenerateSchedule( UtvDownloadInfo * pUtvDownloadInfo )
{
    UTV_RESULT                              rStatus;
    UTV_BOOL                                bNew;
    UTV_INT8                               *pCompName;
    UTV_UINT32                              n, c;
    UtvCompDirHdr                          *pCompDirHdr;
    UtvCompDirCompDesc                     *pCompDesc;
    UtvRange                               *pHWMRanges, *pSWVRanges;
    UtvDependency                          *pSWDeps;
    UtvTextDef                             *pTextDefs;

    /* check the component dir compatibility information */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( pUtvDownloadInfo->pUpdate->hImage, &pCompDirHdr, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

	/* The module version of the component directory should match the signaled module version.
	   Module version needs to be clipped to 8 bits until broadcast network version 4 takes care of 
	   signaling properly 
	*/
	if ( (Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ) & 0xFF) != 
         (pUtvDownloadInfo->pUpdate->usModuleVersion & 0xFF) )
	{
		UtvMessage( UTV_LEVEL_ERROR, "Comp dir (0x%02X) and signaled (0x%02X) module versions disagree", 
					Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ) & 0xFF, 
                    pUtvDownloadInfo->pUpdate->usModuleVersion & 0xFF );
        return UTV_SIGNALING_DISAGREES;
    }

    /* Check the comp dir thoroughly */
    if ( UTV_OK != ( rStatus = UtvCoreCommonCompatibilityUpdate( pUtvDownloadInfo->pUpdate->hImage, 
                                                                 Utv_32letoh( pCompDirHdr->uiPlatformOUI ), 
                                                                 Utv_16letoh( pCompDirHdr->usPlatformModelGroup ),
                                                                 Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ), 
                                                                 Utv_32letoh( pCompDirHdr->uiUpdateAttributes ),
                                                                 Utv_16letoh( pCompDirHdr->usNumUpdateHardwareModelRanges ), pHWMRanges,
                                                                 Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareVersionRanges ), pSWVRanges,
                                                                 Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareDependencies ), pSWDeps )))
    {
        UtvMessage( UTV_LEVEL_WARN, "UtvCoreCommonCompatibilityUpdate() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "Component Directory Header Compatibility matches signaled Compatibility, checking components." );

    /* Perform component-level Compatibility checking on this update.
       Once qualified insert components into download schedule OVERWRITING the existing dummy schedule. */
    for ( pUtvDownloadInfo->pUpdate->uiNumComponents = c = 0; c < Utv_16letoh( pCompDirHdr->usNumComponents ); c++ )
    {
		UTV_BOOL bCompatible;

        /* get compat info */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( pUtvDownloadInfo->pUpdate->hImage, c, &pCompDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetComponent() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

		/* for debug only */
		if ( UTV_OK != ( rStatus = UtvPublicImageGetText( pUtvDownloadInfo->pUpdate->hImage, 
                                                          Utv_16letoh( pCompDesc->toName ), 
                                                          &pCompName )))
		{
			UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails: \"%s\"", UtvGetMessageString( rStatus ) );
			continue; /* don't let the failure of one component lookup stop the others */
		}

		/* check it */
		switch ( rStatus = UtvCoreCommonCompatibilityComponent( pUtvDownloadInfo->pUpdate->hImage, pCompDesc ) )
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

		n = pUtvDownloadInfo->pUpdate->uiNumComponents;
		pUtvDownloadInfo->pUpdate->tComponents[ n ].pCompDesc = pCompDesc;
		pUtvDownloadInfo->pUpdate->tComponents[ n ].bNew = bNew;
		pUtvDownloadInfo->pUpdate->tComponents[ n ].bStored = UTV_FALSE;
		pUtvDownloadInfo->pUpdate->tComponents[ n ].bCompatible = bCompatible;
		pUtvDownloadInfo->pUpdate->tComponents[ n ].bCopied = UTV_FALSE;
		pUtvDownloadInfo->pUpdate->uiNumComponents++;

    } /* for each component */

	return UTV_OK;
}


/* UtvBroadcastDownloadCarousel
    Worker routine for UtvBroadcastDownloadModule
   Returns OK if module was downloaded and stored properly
*/
UTV_RESULT UtvBroadcastDownloadCarousel( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pModuleBuffer )
{
    UTV_RESULT               rStatus, lStatus;
    UtvModSigHdr            *pModSigHdr;
    UTV_UINT32               uiBytesWritten, n;
    UTV_INT8                *pCompName;
    UtvCompDirCompDesc      *pCompDesc;

    /* Grab all of the blocks for this module from the DDBs  */
    if ( UTV_OK != ( rStatus = UtvScanDdb( pUtvDownloadInfo, pModuleBuffer )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "  UtvScanDdb error: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

	/* do not yet know actual component until decrypt, borrow storage approval for first component */
    if ( pUtvDownloadInfo->pUpdate->tComponents[ 0 ].bApprovedForStorage )
    {
		UtvMessage( UTV_LEVEL_INFO, "  Storing module \"%s\" [%d]", pUtvDownloadInfo->szModuleName, pUtvDownloadInfo->iModuleId );

        if ( UTV_OK != ( rStatus = UtvPlatformStoreWrite( pUtvDownloadInfo->pUpdate->hStore, pModuleBuffer, pUtvDownloadInfo->iModuleSize, &uiBytesWritten )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreWrite( %d, %d ) fails: %s", pUtvDownloadInfo->iModuleSize,
                        uiBytesWritten, UtvGetMessageString( rStatus ) );
            return rStatus;
        }
    }

    UtvMessage( UTV_LEVEL_PERF, "  ****** Start Time - Module Received module ID %u, %s", pUtvDownloadInfo->iModuleId, pUtvDownloadInfo->szModuleName );

    /* We have downloaded all the data, now decrypt it */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  decryption start: module name %s module ID %u", pUtvDownloadInfo->szModuleName, pUtvDownloadInfo->iModuleId );

    /* set diagnostic sub-state to decrypting */
    UtvSetSubState( UTV_PUBLIC_SSTATE_DECRYPTING, 0 );

    /* decrypt module setting the key header in and key header out args to NULL so that the integral key header is used */
    rStatus = UtvDecryptModule( pModuleBuffer, pUtvDownloadInfo->iModuleSize, &pUtvDownloadInfo->iDecryptedModuleSize,
                                pUtvDownloadInfo->iOui, pUtvDownloadInfo->iModelGroup, NULL, NULL );
    switch ( rStatus )
    {
        case UTV_OK:
            /* Decrypted the module OK */
            UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  decryption success: module name %s module ID %u", pUtvDownloadInfo->szModuleName, pUtvDownloadInfo->iModuleId );
            break;

        case UTV_NO_ENCRYPTION:
            /* Module was not encrypted in the first place (testing only) */
            UtvMessage( UTV_LEVEL_INFO  | UTV_LEVEL_TEST, "  module not encrypted: module name %s module ID %u", pUtvDownloadInfo->szModuleName, pUtvDownloadInfo->iModuleId );
            break;

        default:
            /* Module did not decrypt */
            UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_WARN | UTV_LEVEL_ERROR  | UTV_LEVEL_TEST, "  decryption failed: %s - module name %s module ID %u",
                        UtvGetMessageString( rStatus ), pUtvDownloadInfo->szModuleName, pUtvDownloadInfo->iModuleId );
            return rStatus;
    }

    /* check this module to make sure its signaling header matches the signaled data */
    pModSigHdr = (UtvModSigHdr *) pModuleBuffer;

    /* Check signature */
    if ( UtvMemcmp( pModSigHdr->ubModSigHdrSignature, (void *) UTV_MOD_SIG_HDR_SIGNATURE, UTV_SIGNATURE_SIZE ) )
    {
        rStatus = UTV_BAD_MOD_SIG;
        UtvMessage( UTV_LEVEL_ERROR, " module header check fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* Check structure revision */
    if ( UTV_MOD_SIG_HDR_VERSION != pModSigHdr->ubStructureVersion )
    {
        rStatus = UTV_BAD_MOD_VER;
        UtvMessage( UTV_LEVEL_ERROR, " module header check fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* check that decrypted size matches */
    if ( (Utv_32letoh( pModSigHdr->uiCEMModuleSize ) + Utv_16letoh( pModSigHdr->usHeaderSize ) ) != 
         pUtvDownloadInfo->iDecryptedModuleSize )
    {
        rStatus = UTV_MOD_SIZES_DISAGREE;
        UtvMessage( UTV_LEVEL_ERROR, " module header check fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* display mod hdr info */
	pCompName = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pModSigHdr, Utv_16letoh( pModSigHdr->toComponentName ));
    UtvMessage( UTV_LEVEL_INFO, "            Module Header Info:  Component name: %s, MV: 0x%X, Index: %d, Count: %d, Header size: %d, CEM module size: %d, CEM module offset: %d",
                pCompName, 
                Utv_16letoh( pModSigHdr->usModuleVersion ), 
                Utv_16letoh( pModSigHdr->usModuleIndex ),
                Utv_16letoh( pModSigHdr->usModuleCount ), 
                Utv_16letoh( pModSigHdr->usHeaderSize ), 
                Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                Utv_32letoh( pModSigHdr->uiCEMModuleOffset ) );

    /* set diagnostic sub-state to indicate we're storing a module */
    UtvSetSubState( UTV_PUBLIC_SSTATE_STORING_MODULE, 0 );

    /* We successfully got the module downloaded, tell the platform adaptation layer to store the CEM data beyond the mod header.
       We literally don't know anything about which component this module belongs to until now. Look it up.
    */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDescByName( pUtvDownloadInfo->pUpdate->hImage, pCompName, &pCompDesc )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageGetCompDescByName( %s ): %s", pCompName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

	/* look up the component in the schedule */
	for ( n = 0; n < pUtvDownloadInfo->pUpdate->uiNumComponents; n++ )
	{
		if ( pCompDesc == pUtvDownloadInfo->pUpdate->tComponents[ n ].pCompDesc )
			break;
	}

	/* component should be in the schedule */
	if ( n >= pUtvDownloadInfo->pUpdate->uiNumComponents )
	{
        UtvMessage( UTV_LEVEL_ERROR, " Can't find \"%s\" in the schedule", pCompName );
        return UTV_UNKNOWN_COMPONENT;
	}
	
    /* if component not compatible then don't install */
    if ( !pUtvDownloadInfo->pUpdate->tComponents[ n ].bCompatible )
    {
		/* if incompatible component has not yet been copied then copy now */
		if ( !pUtvDownloadInfo->pUpdate->tComponents[ n ].bCopied )
		{
			UtvMessage( UTV_LEVEL_INFO, "  Component \"%s\" not compatible, COPYING", pCompName );
			if ( UTV_OK != ( rStatus = UtvPlatformInstallCopyComponent( pUtvDownloadInfo->pUpdate->hImage, pCompDesc )))
			{
				UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallCopyComponent(): %s", UtvGetMessageString( rStatus ) );
				return rStatus;
			} else
			{
				pUtvDownloadInfo->pUpdate->tComponents[ n ].bCopied = UTV_TRUE;
			}
		}
		
        /* let the compatibility machinery know we've got this one so we don't try to keep downloading it  */
        if ( UTV_OK != ( rStatus = UtvCoreCommonCompatibilityRecordModuleId( pUtvDownloadInfo->pUpdate->hImage, 
																			 pCompDesc, 
																			 (UTV_UINT16) pUtvDownloadInfo->iModuleId )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvCoreCommonCompatibilityRecordModuleId(): %s", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        return UTV_OK;
    }

    /* if not approved for install then punt here */
    if ( !pUtvDownloadInfo->pUpdate->tComponents[ n ].bApprovedForInstall )
    {
		UtvMessage( UTV_LEVEL_INFO, "  Component \"%s\" not approved for install, SKIPPING", pCompName );

        /* let the compatibility machinery know we've got this one */
        if ( UTV_OK != ( rStatus = UtvCoreCommonCompatibilityRecordModuleId( pUtvDownloadInfo->pUpdate->hImage, 
																			 pCompDesc, 
																			 (UTV_UINT16) pUtvDownloadInfo->iModuleId )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvCoreCommonCompatibilityRecordModuleId(): %s", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        return UTV_OK;
    }

    rStatus = UtvPlatformInstallModuleData( pUtvDownloadInfo->pUpdate->hImage, pCompDesc, pModSigHdr, 
                                            pModuleBuffer + Utv_16letoh( pModSigHdr->usHeaderSize ) );
    if ( UTV_OK == rStatus || UTV_COMP_INSTALL_COMPLETE == rStatus )
    {
        /* let the compatibility machinery know we've got this one */
        if ( UTV_OK != ( lStatus = UtvCoreCommonCompatibilityRecordModuleId( pUtvDownloadInfo->pUpdate->hImage, pCompDesc, (UTV_UINT16) pUtvDownloadInfo->iModuleId )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvCoreCommonCompatibilityRecordModuleId(): %s", UtvGetMessageString( lStatus ) );
            return lStatus;
        }

        /* If we installed all of the modules for a given component then inform the platform-specific code. */
        if ( UTV_COMP_INSTALL_COMPLETE == rStatus )
        {
            if ( UTV_COMP_INSTALL_COMPLETE != ( rStatus = UtvPlatformInstallComponentComplete( g_pUtvBestCandidate->pUpdate->hImage, pCompDesc )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallComponentComplete() fails: %s", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            UtvMessage( UTV_LEVEL_INFO, " Component \"%s\", install success", pCompName );
         }

        UtvMessage( UTV_LEVEL_PERF, "  ****** End Time - Module Received module ID %u, %s", pUtvDownloadInfo->iModuleId, pUtvDownloadInfo->szModuleName );
    }
    else
        UtvMessage( UTV_LEVEL_TEST, "  Module storage failed: %s, on module %s", UtvGetMessageString( rStatus ), pUtvDownloadInfo->szModuleName );

    return rStatus;
}

/* UtvScanDdb
    Scan the DDB blocks and put them into the right place in the module storage buffer
   Returns OK if the data was all placed into the module buffer
*/
UTV_RESULT UtvScanDdb( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pModuleBuffer )
{
    UTV_RESULT rStatus;
    UTV_BYTE * pSectionData = NULL;
    UTV_BOOL   bJustOnce = UTV_FALSE;
    UTV_UINT32 uiMillisecondsToEvent, iBlockSize;
    UTV_UINT32 iWaitDdb;
    UTV_BYTE * pBlockMap;
    UTV_UINT32 iNeedBlockCount;
    UTV_UINT32 iMaximumBlockNumber;

    uiMillisecondsToEvent = UtvGetMillisecondsToEvent( pUtvDownloadInfo->dtDownload );

    /* Set up the download timeout, time of broadcast plus time till broadcast start */
    iWaitDdb = pUtvDownloadInfo->iBroadcastSeconds * 1000 +  UtvGetMillisecondsToEvent( pUtvDownloadInfo->dtDownload );

    /* Set the diagnostic sub-state to scanning for DDBs */
    UtvSetSubState( UTV_PUBLIC_SSTATE_SCANNING_DDB, iWaitDdb );

    /* Create a map of the blocks we are to download and the number downloaded */
    if ( NULL == ( pBlockMap = (UTV_BYTE *)UtvMalloc( ( pUtvDownloadInfo->iModuleSize / pUtvDownloadInfo->iModuleBlockSize ) + 1 )))
        return UTV_NO_MEMORY;

    iNeedBlockCount = ( pUtvDownloadInfo->iModuleSize / pUtvDownloadInfo->iModuleBlockSize );
    if ( iNeedBlockCount * pUtvDownloadInfo->iModuleBlockSize != pUtvDownloadInfo->iModuleSize )
        iNeedBlockCount++;
    iMaximumBlockNumber = iNeedBlockCount - 1;

    /* let the stats structure know how many blocks we're looking for */
    /* and that we haven't received any yet. */
    g_pUtvDiagStats->iNeedBlockCount = iNeedBlockCount;
    g_pUtvDiagStats->iReceivedBlockCount = 0;
    g_pUtvDiagStats->iAlreadyHaveCount = 0;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO, " last known broadcast time is %s", UtvConvertGpsTime( UtvGetBroadcastTime() ) );
    UtvMessage( UTV_LEVEL_INFO, " module ID %u starts in %d seconds for %d secs ",
                pUtvDownloadInfo->iModuleId, pUtvDownloadInfo->dtDownload - UtvGetBroadcastTime(), pUtvDownloadInfo->iBroadcastSeconds );
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " waiting for DDBs - %d seconds for module ID %u, need %d blocks", iWaitDdb / 1000, pUtvDownloadInfo->iModuleId, iNeedBlockCount );

#ifdef UTV_TRACE_ALL_DDB  /* LAB ONLY */
    /* Set up the section filter depending on the trace level */
    if ( UtvIsMessageLevelSet( UTV_LEVEL_TRACE ) )
    {
        /* Get all the DDBs on this carousel */
        rStatus = UtvOpenSectionFilter( pUtvDownloadInfo->iCarouselPid, UTV_TID_DDM, 0, UTV_FALSE );
    }
    else
    {
        /* Get only the DDBs that match our download from the carousel pid */
        rStatus = UtvOpenSectionFilter( pUtvDownloadInfo->iCarouselPid, UTV_TID_DDM, pUtvDownloadInfo->iModuleId, UTV_TRUE );
    }
#else /* UTV_TRACE_ALL_DDB  */

    /* Get only the DDBs that match our download from the carousel pid and module ID */
    rStatus = UtvOpenSectionFilter( pUtvDownloadInfo->iCarouselPid, UTV_TID_DDM, pUtvDownloadInfo->iModuleId, UTV_TRUE );

#endif /* UTV_TRACE_ALL_DDB */

    if ( rStatus != UTV_OK )
    {
        UtvFree( pBlockMap );
        return rStatus;
    }

    /* Store blocks as they appear in the transport stream */
    while ( iWaitDdb > 0 && iNeedBlockCount > 0 )
    {
        /* Get the data for the DDBs */
        rStatus = UtvGetSectionData( &iWaitDdb, &pSectionData );
        if ( rStatus == UTV_OK )
        {
            /* Set the diagnostic sub-state to scanning for DDBs */
            if ( !bJustOnce )
            {
                UtvSetSubState( UTV_PUBLIC_SSTATE_DOWNLOADING, pUtvDownloadInfo->iBroadcastSeconds * 1000 );
                bJustOnce = UTV_TRUE;
            }

            /* We got section data, try parsing it now */
            rStatus = UtvParseDdb( pUtvDownloadInfo, pSectionData, pModuleBuffer, pBlockMap, iMaximumBlockNumber, &iNeedBlockCount, &iBlockSize );
        }

        if ( rStatus != UTV_OK )
        {
            break;
        }
    }

    /* We can be here if a time or if all the blocks have been seen */
    if ( ( rStatus == UTV_OK && iNeedBlockCount > 0 ) || rStatus == UTV_TIMEOUT )
    {
        /* A timeout has occured, we still need blocks */
        UtvMessage( UTV_LEVEL_ERROR | UTV_LEVEL_TEST, " download timeout, %d blocks missing", iNeedBlockCount );
        rStatus = UTV_DDB_TIMEOUT;
    }

    /* If an error during the download report that too */
    if ( rStatus != UTV_OK && rStatus != UTV_TIMEOUT )
    {
        /* Report the download error */
        UtvMessage( UTV_LEVEL_ERROR, " download error: %s, %d blocks missing", UtvGetMessageString( rStatus ), iNeedBlockCount );
    }

    /* set the need blocks count if it is non-zero */
    if ( 0 != iNeedBlockCount )
        g_pUtvDiagStats->iUnreceivedBlockCount = iNeedBlockCount;

    /* Close the section filter and release the block map before returning status */
    UtvCloseSectionFilter();
    UtvFree( pBlockMap );
    return rStatus;
}

/* UtvParseDdb
    Parses a DDB section
    Fills in the block in the pModuleBuffer and marks recieved blocks in pBlockMap, counts down blocks in *piNeedBlockCount.
   Returns UTV_OK if DDB parsed, error if download should be aborted
*/
UTV_RESULT UtvParseDdb( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pSectionData, UTV_BYTE *pModuleBuffer,
                        UTV_BYTE * pBlockMap, UTV_UINT32 iMaximumBlockNumber, UTV_UINT32 * piNeedBlockCount,
                        UTV_UINT32 * piBlockSize )
{
    /* Set up to parse the section data, must be DSM CC user network message section */
    UTV_BYTE * pData = pSectionData;
    UTV_UINT32 iSize = 0;
    UTV_UINT32 iModuleId;
    UTV_UINT32 iBlockNumber;
    UTV_UINT32 iDataBytes;
    UTV_BYTE * pStore;
    UTV_UINT32 iMessageLength;

    /* Initialize block size to zero */
    *piBlockSize = 0;

    if ( UTV_TID_DDM != UtvStartSectionPointer( &pData, &iSize ) )
        return UTV_BAD_DDB_TID;

    /* Skip table id ext, version number, section number, last section number */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1, &pData, &iSize );

    /* Check protocol discriminator, if not then something terribly wrong */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_PROTOCOL_DISCRIMINATOR )
        return UTV_BAD_DDB_DPD;

    /* Make sure it is a UNM, if not then something terribly wrong */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_UNM )
        return UTV_BAD_DDB_UNM;

    /* Check for DSM-CC user-network download message that was a DDB */
    /* Only DDBs should be seen on this table id */
    if ( UtvGetUint16Pointer( &pData, &iSize ) != UTV_DSMCC_DDB )
        return UTV_BAD_DDB_MID;

    /* Skip download id and reserved byte */
    UtvSkipBytesPointer( 4 + 1, &pData, &iSize );

    /* Make sure adaptationLength is zero, if not then something terribly wrong */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_DDB_ADP;

    /* Now get the DSM-CC length */
    iMessageLength = UtvGetUint16Pointer( &pData, &iSize );
    if ( iMessageLength == 0 )
        return UTV_BAD_DDB_DLN;

    /* Get the module id  */
    iModuleId = UtvGetUint16Pointer( &pData, &iSize );

    /* Skip module version and reserved byte */
    UtvSkipBytesPointer( 2, &pData, &iSize );

    /* Get this block's number */
   iBlockNumber = UtvGetUint16Pointer( &pData, &iSize );

    /* See if it is the module we wanted today, if not then no big deal */
    if ( iModuleId != pUtvDownloadInfo->iModuleId )
    {
        UtvMessage( UTV_LEVEL_TRACE, "  found module ID %u, block %2d when looking for module ID %u",
                    iModuleId, iBlockNumber, pUtvDownloadInfo->iModuleId );
        return UTV_OK;
    }

    /* Check the block number, fail if our module and not in range */
    if ( iBlockNumber > iMaximumBlockNumber )
    {
        UtvMessage( UTV_LEVEL_INFO, "  BLOCK NUM: %d, MAX BLOCK: %d", iBlockNumber, iMaximumBlockNumber );
        return UTV_BAD_DDB_BTL;
    }


    /* See if we need this particular block number, if not then no big deal */
    if ( pBlockMap[ iBlockNumber ] != 0 )
    {
#ifdef UTV_DDB_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "  already have module ID %u, block %d", iModuleId, iBlockNumber );
#endif
        g_pUtvDiagStats->iAlreadyHaveCount++;

        return UTV_OK;
    }

    /* Count block as grabbed */
    pBlockMap[iBlockNumber]++;
    (*piNeedBlockCount)--;
    g_pUtvDiagStats->iReceivedBlockCount++;
    g_pUtvDiagStats->uiTotalReceivedByteCount += ( iMessageLength - 6 );

#ifdef UTV_DDB_DEBUG
    /* Log this */
    UtvMessage( UTV_LEVEL_INFO, "  got module ID %u, block %2d need %2d more" , pUtvDownloadInfo->iModuleId, iBlockNumber, (*piNeedBlockCount) );
#endif

    /* Compute position in file */
    iDataBytes = iMessageLength - 6;
    pStore = pModuleBuffer + ( iBlockNumber * pUtvDownloadInfo->iModuleBlockSize );
    UtvByteCopy( pStore, pData, iDataBytes );

    /* report block size */
    *piBlockSize = iDataBytes;

    /* Return goodly */
    return UTV_OK;
}
