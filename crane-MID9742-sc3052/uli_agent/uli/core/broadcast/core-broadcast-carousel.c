/* core-broadcast-carousel.c: UpdateTV Common Core - UpdateTV Carousel Scan

 This module implements the UpdateTV Carousel Scan. The Carousel Scan is called
 to find any download in the next 24 hours on a certain frequency and PID.

 In order to ensure UpdateTV network compatibility, customers must not
 change UpdateTV Common Core code.

 Copyright (c) 2004 - 2009 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      06/30/2009  Removed code that filtered DII results on comp dir to allow multiple comp dirs.
                              Moved schedule creation from UtvParseDII() to UtvFindBestDownloadCandidate()
                              and simplified to one time slot per update.
  Bob Taylor      02/02/2009  Created from 1.9.21 for version 2.0.0.
*/

#include "utv.h"

/* Static data 
*/
static char s_ubServerVersion[UTV_SERVER_VER_MAX_LEN] = {0};
static char s_ubServerID[UTV_SERVER_ID_MAX_LEN] = {0};

static UTV_UINT32 s_uiChannelScanTime = UTV_MAX_CHANNEL_SCAN;

static UTV_UINT32 s_uiAttributeBitMask = 0;
static UTV_UINT32 s_uiAttributeSenseMask = 0;

/* UtvGroupCandidate storage management 
*/
UtvGroupCandidate * apUtvGroupCandidate[ UTV_GROUP_CANDIDATE_MAX ] = { NULL };
UtvGroupCandidate * UtvNewGroupCandidate( void );

/* UtvDownloadInfo storage management
*/
UtvDownloadInfo * apUtvDownloadCandidate[ UTV_DOWNLOAD_CANDIDATE_MAX ] = { NULL };
UTV_UINT32 iUtvDownloadCandidate = 0;
UtvDownloadInfo * UtvNewDownloadCandidate( void );

/* Carousel Scan worker routines
*/
UTV_RESULT UtvFindBestDownloadCandidate( UtvDownloadInfo * pUtvBestCandidate );
void UtvCopyDownloadInfo( UtvDownloadInfo * pUtvCurrentBestCandidate, UtvDownloadInfo * pUtvNewBestCandidate );
UTV_RESULT UtvScanDsi( UTV_UINT32 iCarouselPid );
UTV_RESULT UtvScanDii( UTV_UINT32 iCarouselPid );
UTV_RESULT UtvParseDsi( UTV_UINT32 iCarouselPid, UTV_BYTE * pSectionData );
UTV_RESULT UtvParseDii( UTV_BYTE * pSectionData );


/* UtvCalcTimeToDownload
    Called with the best candidate module to download to recalculate 
    the time to download by getting the DSI again to adjust for time
    spent completing the channel scan 
*/
UTV_RESULT UtvCalcTimeToDownload( UtvDownloadInfo * pUtvBestCandidate )
{
    UTV_RESULT rStatus;

    /* Set desired frequency */
    rStatus = UtvPlatformDataBroadcastSetTunerState( pUtvBestCandidate->pUpdate->uiFrequency );
    if ( rStatus != UTV_OK )
        return rStatus;

    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  tuned to %d MHz", pUtvBestCandidate->pUpdate->uiFrequency / 1000000 );

    /* Create section filter */
    rStatus = UtvCreateSectionFilter();
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Scan for the DSI to update the broadcast time */
    rStatus = UtvScanDsi( pUtvBestCandidate->iCarouselPid );
    if ( rStatus != UTV_OK )
    {
        UtvDestroySectionFilter();
        return rStatus;
    }

    pUtvBestCandidate->iMillisecondsToStart = UtvGetMillisecondsToEvent( pUtvBestCandidate->dtDownload );

    /* Destroy the section filter */
    UtvDestroySectionFilter();

    /* Done here */
    return rStatus;
}

/* UtvCarouselScan
    Called with the elementary PID of the carousel to scan the carousel on that PID.
   Returns OK if at least one download candidate coming in next 24 hours
*/
UTV_RESULT UtvCarouselScan( UTV_UINT32 iCarouselPid, UtvDownloadInfo * pUtvBestCandidate )
{
    UTV_RESULT rStatus;

    /* Clear the results of any previous scan */
    UtvClearGroupCandidate();
    UtvClearDownloadCandidate();

    /* Get the data from the DSI */
    if ( UTV_OK != ( rStatus = UtvScanDsi( iCarouselPid )))
        return rStatus;

    /* DSI found, data was there we liked, now get the data from the DIIs */
    if ( UTV_OK != ( rStatus = UtvScanDii( iCarouselPid )))
        return rStatus;

#ifdef UTV_INTERACTIVE_MODE
    /* in factory mode there isn't a second DSI scan */
    if ( !UtvGetFactoryMode() )
    {
#endif
        /* Grab the DSI again so we can get the broadcast time */
        if ( UTV_OK != ( rStatus = UtvScanDsi( iCarouselPid )))
            return rStatus;

#ifdef UTV_INTERACTIVE_MODE
    }
#endif
    
    /* Get the best candidate and return his info */
    return UtvFindBestDownloadCandidate( pUtvBestCandidate );
}

/* UtvFindBestDownloadCandidate
    Finds the soonest download by looking at the download candidate table
    Always updates return values
   Returns OK if a best candidate was found
*/
UTV_RESULT UtvFindBestDownloadCandidate( UtvDownloadInfo * pUtvBestCandidate )
{
    /* Find and return the information on the most recent good download candidate */
    UTV_RESULT                 rStatus = UTV_NO_DOWNLOAD;
    UTV_UINT32                 iMillisecondsToStart = pUtvBestCandidate->iMillisecondsToStart;
    UTV_TIME                   dtDownload = pUtvBestCandidate->dtDownload;
    UTV_UINT32                 iModulePriority = pUtvBestCandidate->iModulePriority, uiMatchingPriority;
    UTV_UINT32                 iNumberOfModules = 1;
    UtvDownloadInfo         *  pUtvNewBestCandidate = NULL;
    UTV_UINT32                 iDownloadCandidate;
    UTV_BOOL                   bNewFavorite, bPriorityMatch = UTV_FALSE;

    /* Examine each interesting download to see which one download we should target */
    for ( iDownloadCandidate = 0; iDownloadCandidate < iUtvDownloadCandidate; iDownloadCandidate++ )
    {
        UtvDownloadInfo * pUtvDownloadCandidate = apUtvDownloadCandidate[ iDownloadCandidate ];
        if ( pUtvDownloadCandidate != NULL )
        {
            /* Choose a new favorite:
                The primary qualification is the module's attributes (which are stored in the priority field).
                The secondary qualification is the modules's start time (sooner time better).
                Obviously if we don't have a favorite yet ( iMillisecondsToStart == 0 ) then this candidate 
                becomes are new favorite.
            */
            if ( iMillisecondsToStart == 0 )
                bNewFavorite = UTV_TRUE;
            else
            {
                /* only have to look for a new favorite if we've already seen another candidate */
                bNewFavorite = UTV_FALSE;

                /* Update milliseconds to start based on current time */
                pUtvDownloadCandidate->iMillisecondsToStart = UtvGetMillisecondsToEvent( pUtvDownloadCandidate->dtDownload );

                /* Apply attributes conditions.    
                        Attribute conditions are set via the s_uiAttributeBitMask and s_uiAttributeSenseMask variables.
                        the bit mask identifies the bits that we're interested in.  The sense mask identifies the
                        the sense of those bits.
                        If the bit mask is zero then attributes don't play a role in the module choice.
                        If the bit mask is 0x00000001 and the sense mask is 0x00000001 then we're looking for bit one to be ON
                        and will *only* accept modules that meet this requirement.
                        If the bit mask is 0x00000001 and the sense mask is 0x00000000 then we're looking for bit one to be OFF
                        and will *only* accept modules that meet this requirement.
                        Multiple bits can be sepcified in the bit mask and sense mask at the same time.
                */
                if ( 0 != s_uiAttributeBitMask )
                {
                    /* Attribute conditions exist, does this candidate satisfy them?
                       Create a mask that should match this candidate's attribute byte. */
                    uiMatchingPriority = s_uiAttributeBitMask & pUtvDownloadCandidate->iModulePriority;
                    uiMatchingPriority &= s_uiAttributeSenseMask;

                    /* tell this algorithm that the current favorite has a priority match */
                    if ( uiMatchingPriority == pUtvDownloadCandidate->iModulePriority )
                    {    
                        /* if we didn't previously have a priority match then this candidate
                           automatically becomes the new favorite */
                        if ( !bPriorityMatch )
                            bNewFavorite = UTV_TRUE;
                        {
                            /* we have an existing priority match, compare times */
                            if ( dtDownload > pUtvDownloadCandidate->dtDownload )
                                bNewFavorite = UTV_TRUE;
                        }

                        bPriorityMatch = UTV_TRUE;
                    }
                } else
                {
                    /* no attribute conditions exist, use broadcast start time only */
                    if ( dtDownload > pUtvDownloadCandidate->dtDownload )
                        bNewFavorite = UTV_TRUE;
                }
            }

            /* If we have a new favorite, copy his data over */
            if ( bNewFavorite )
            {
                iMillisecondsToStart = pUtvDownloadCandidate->iMillisecondsToStart;
                dtDownload = pUtvDownloadCandidate->dtDownload;
                iModulePriority = pUtvDownloadCandidate->iModulePriority;
                pUtvNewBestCandidate = pUtvDownloadCandidate;
                iNumberOfModules = pUtvDownloadCandidate->iNumberOfModules;
                rStatus = UTV_OK;
            }
        }
    }

    if ( rStatus != UTV_OK )
        return rStatus;

    /* Found a new best candidate, update current best */
    UtvCopyDownloadInfo( pUtvBestCandidate, pUtvNewBestCandidate );
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " best candidate is module %s ID %u priority 0x%X on carsousel pid 0x%X in %d seconds",
                pUtvBestCandidate->szModuleName, pUtvBestCandidate->iModuleId, pUtvBestCandidate->iModulePriority, pUtvBestCandidate->iCarouselPid,
                pUtvBestCandidate->iMillisecondsToStart / 1000 );

    /* Record last broadcast time, freq, and PID of compatible scan. */
    UtvSaveLastCompatibleScanTime();
    g_pUtvDiagStats->iLastCompatibleFreq = pUtvNewBestCandidate->iFrequency;
    g_pUtvDiagStats->iLastCompatiblePid = (UTV_UINT16) pUtvNewBestCandidate->iCarouselPid;

#ifdef UTV_INTERACTIVE_MODE
    if ( g_bAccumulateDownloadInfo )
    {
        UTV_UINT32  c;

        /* init the update structure and set the module start time. */
        if ( g_sSchedule.uiNumUpdates < UTV_PLATFORM_MAX_UPDATES )
        {
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_BROADCAST;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hStore = UTV_PLATFORM_UPDATE_STORE_INVALID;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].hImage = UTV_CORE_IMAGE_INVALID;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiFrequency = pUtvNewBestCandidate->iFrequency;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiPid = pUtvNewBestCandidate->iCarouselPid;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].usModuleVersion = (UTV_UINT16) pUtvNewBestCandidate->iModuleVersion;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiBlockSize = pUtvNewBestCandidate->iModuleBlockSize;
            /* only track one slot */
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumTimeSlots  = 1;
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiSecondsToStart[ 0 ] = pUtvBestCandidate->iMillisecondsToStart / 1000;
                                
            /* Nothing is known about the components at this point with broadcast delivery.
               Pretend that max components are being downloaded, but null out the pCompDesc ptr
               to indicate that we don't know anything about them. */
            g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].uiNumComponents = UTV_PLATFORM_MAX_COMPONENTS;
            for ( c = 0; c < UTV_PLATFORM_MAX_COMPONENTS; c++ )
            {
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ c ].pCompDesc = NULL;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ c ].bStored = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ c ].bNew = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ c ].bApprovedForStorage = UTV_FALSE;
                g_sSchedule.tUpdates[ g_sSchedule.uiNumUpdates ].tComponents[ c ].bApprovedForInstall = UTV_FALSE;
            }
            g_sSchedule.uiNumUpdates++;
        }
    }
#endif
                        
    /* Return the success back */
    return UTV_OK;
}

/* UtvCopyDownloadInfo
    Copy the data from download candidate to download info
*/
void UtvCopyDownloadInfo( UtvDownloadInfo * pUtvBestCandidate, UtvDownloadInfo * pUtvNewBestCandidate )
{
    pUtvBestCandidate->iFrequency = pUtvNewBestCandidate->iFrequency;
    pUtvBestCandidate->iCarouselPid = pUtvNewBestCandidate->iCarouselPid;
    pUtvBestCandidate->iTransactionId = pUtvNewBestCandidate->iTransactionId;
    pUtvBestCandidate->iOui = pUtvNewBestCandidate->iOui;
    pUtvBestCandidate->iModelGroup = pUtvNewBestCandidate->iModelGroup;
    pUtvBestCandidate->iHardwareModelBegin = pUtvNewBestCandidate->iHardwareModelBegin;
    pUtvBestCandidate->iHardwareModelEnd = pUtvNewBestCandidate->iHardwareModelEnd;
    pUtvBestCandidate->iSoftwareVersionBegin = pUtvNewBestCandidate->iSoftwareVersionBegin;
    pUtvBestCandidate->iSoftwareVersionEnd = pUtvNewBestCandidate->iSoftwareVersionEnd;
    pUtvBestCandidate->iModuleId = pUtvNewBestCandidate->iModuleId;
    pUtvBestCandidate->iModulePriority = pUtvNewBestCandidate->iModulePriority;
    pUtvBestCandidate->iModuleSize = pUtvNewBestCandidate->iModuleSize;
    pUtvBestCandidate->iModuleVersion = pUtvNewBestCandidate->iModuleVersion;
    pUtvBestCandidate->iModuleBlockSize = pUtvNewBestCandidate->iModuleBlockSize;
    pUtvBestCandidate->iModuleNameLength = pUtvNewBestCandidate->iModuleNameLength;
    UtvByteCopy( pUtvBestCandidate->szModuleName, pUtvNewBestCandidate->szModuleName, pUtvNewBestCandidate->iModuleNameLength + 1 );
    pUtvBestCandidate->iBroadcastSeconds = pUtvNewBestCandidate->iBroadcastSeconds;
    pUtvBestCandidate->iMillisecondsToStart = pUtvNewBestCandidate->iMillisecondsToStart;
    pUtvBestCandidate->dtDownload = pUtvNewBestCandidate->dtDownload;
}

/* UtvScanDsi
    There is only one DSI per Carousel PID.
    This routine examines the DSI and adds one UtvGroupCandidate for each download that is compatible.
   Returns OK if one (or more) candidates seen.
*/
UTV_RESULT UtvScanDsi( UTV_UINT32 iCarouselPid )
{
    UTV_RESULT  rStatus = UTV_OK, rParseError = UTV_OK;
    UTV_BYTE  * pSectionData = NULL;
    UTV_UINT32  iWaitDsi = UTV_WAIT_FOR_DSI;

    /* Set the diagnostic sub-state to scanning for a DSI */
    UtvSetSubState( UTV_PUBLIC_SSTATE_SCANNING_DSI, iWaitDsi );

    /* Scanning DSI */
    UtvMessage( UTV_LEVEL_INFO, " scanning for DSI on PID 0x%X for %d ms", iCarouselPid, iWaitDsi );

    /* Set up the section filter for the Carousel PID and the table for DSM-CC userNetworkMessage */
    rStatus = UtvOpenSectionFilter( iCarouselPid, UTV_TID_UNM, 0, UTV_TRUE );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Since the DSI and DII and DC share the same Carousel PID and table id, we have to filter on just DSI */
    while ( iWaitDsi > 0 )
    {
        /* Get the data for the DSI, there's only one per Carousel PID */
        rStatus = UtvGetSectionData( &iWaitDsi, &pSectionData );
        if ( rStatus == UTV_ABORT )
            break;

        if ( rStatus == UTV_OK )
        {
            /* We got section data, try parsing it now */
            rParseError = UtvParseDsi( iCarouselPid, pSectionData );
            if ( rParseError == UTV_OK )
            {
                /* We found a compatible DSI */
                UtvMessage( UTV_LEVEL_INFO, "  found compatible DSI on PID 0x%X", iCarouselPid );
                break;
            }
            if ( rParseError == UTV_NO_DOWNLOAD )
            {
                /* We found a DSI but no compatible group candidates */
                UtvMessage( UTV_LEVEL_INFO, "  found DSI (no compatible groups) on PID 0x%X", iCarouselPid );
                break;
            }
            if ( rParseError == UTV_BAD_DSI_MID )
            {
                /* This was a DII, continue to look for a DSI and change the parse error to OK because */
                /* we don't want to percolate this error back up. */
                rParseError = UTV_OK;
            }
            if ( rParseError == UTV_BAD_DSI_MRD )
            {
                /* We found a DSI that has an incorrect MRD.  This is a network configuration problem */
                break;
            }
            /* note that all other errors cause the scan to continue to look for 
               a compatible DSI on this PID until the timeout period elapses. */
        }
    }

    /* If timeout and parse is happy then no DSI found, report the timeout */
    if ( UTV_TIMEOUT == rStatus && UTV_OK == rParseError )
    {
        UtvMessage( UTV_LEVEL_TEST, " DSI scan timeout - no carousel on PID 0x%X", iCarouselPid );
        rStatus = UTV_DSI_TIMEOUT;
    } else
    {
        /* If section data retrieval worked, pass on whatever the parse found. */
        /* otherwise UtvGetSectionData's problem trumps the parse. */
        if ( UTV_OK == rStatus || UTV_TIMEOUT == rStatus )
            rStatus = rParseError;
    }

    UtvCloseSectionFilter();
    return rStatus;
}

/* UtvParseDsi
    Parses the DSI data. 
   Returns UTV_NO_DOWNLOAD if parsed ok but no download indicated.
   Returns OK if UtvIsDownloadCompatible() found a candidate(s). 
   If this is a DII then we ignore it and return UTV_BAD_DSI_MID.
   First checks network version and ID.  If bad returns UTV_BAD_DSI_BDC.
   Returns UTV error codes on parsing/formatting errors.
*/
UTV_RESULT UtvParseDsi( UTV_UINT32 iCarouselPid, UTV_BYTE * pSectionData )
{
    /* Set up to parse the section data, must be DSM CC user network message section */
    UTV_BYTE * pData = pSectionData;
    UTV_UINT32 iSize = 0;
    UTV_UINT32 iSectionTableId = UtvStartSectionPointer( &pData, &iSize );
    UTV_UINT32 iGroupCount, iNumberOfGroups;
    UTV_BYTE * pDescriptorData;
    UTV_UINT32 iDescriptorLength;
    UTV_UINT16 iGroupsInfoPrivateDataLength;
    UTV_UINT32 dtSystemTime;
    UTV_UINT32 iGpsUtcOffset;
    UTV_UINT32 iTimeProtocolVersion;
    UTV_UINT16 iNetworkId;
    UTV_UINT16 iNetworkVersion;
    UTV_INT32  i;
    UTV_BOOL   bFirstDescriptor;
    UTV_BOOL   bOurMRDFound;
    UTV_BOOL   bOurVersionFound;
    UTV_BYTE   szRegistration[ UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_LENGTH + 1 ] = { 0 };
    UTV_UINT16 iGroupsInfoDescriptor;
    UTV_UINT16 iGroupsInfoSize;
    UTV_RESULT rStatus = UTV_NO_DOWNLOAD;
    UTV_UINT32 iGroupId;
    UTV_UINT16 iGroupCompatibilityDescriptorLength;
    UTV_UINT32 iOui;
    UTV_UINT32 iModelGroup;
    UTV_UINT16 iGroupCompatibilityDescriptorCount;
    UTV_UINT8  iCdDescriptorTag;
    UTV_UINT8  iCdDescriptorLength;
    UTV_UINT8  iCdSpecifierType;
    UTV_UINT32 iCdSpecifierData;
    UTV_UINT16 iCdModel;
    UTV_RESULT uTVResult;

    if ( UTV_TID_UNM != iSectionTableId  )
        return UTV_BAD_DSI_TID;

    /* Skip table id ext, version, section number, last section number */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1, &pData, &iSize );

    /* Check protocol */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_PROTOCOL_DISCRIMINATOR )
        return UTV_BAD_DSI_DPD;

    /* Make sure it is a DSI */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_UNM )
        return UTV_BAD_DSI_UNM;

    /* Check for DSM-CC user-network download message that was a DSI message */
    if ( UtvGetUint16Pointer( &pData, &iSize ) != UTV_DSMCC_DSI )
        return UTV_BAD_DSI_MID;

    /* Skip the DSI transactionId, reserved byte */
    UtvSkipBytesPointer( 4 + 1, &pData, &iSize );

    /* Make sure adaptationLength is zero */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_DSI_ADP;

    /* Now the DSM-CC length */
    if ( UtvGetUint16Pointer( &pData, &iSize ) == 0 )
        return UTV_BAD_DSI_DLN;

    /* The 20 bytes of serverId isn't used */
    UtvSkipBytesPointer( 20, &pData, &iSize );

    /* Length compatibilityDescriptorLength must be zero */
    if ( UtvGetUint16Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_DSI_CDL;

    /* ATSC A/90 specifies the mandatory use of the Group Information Indication (GII) field */
    /* which occupies the privateDataBytes section of the DSI message */
    if ( UtvGetUint16Pointer( &pData, &iSize ) == 0 )
        return UTV_BAD_DSI_PDL;

    /* Now we have to verify that it is our network */
    /* Skip ahead past the groups/compatibility data to the private descriptors */

    /* Next is the numberOfGroups (GIIs), may be zero indicating no download.  */
    if ( ( iNumberOfGroups = UtvGetUint16Pointer( &pData, &iSize ) ) == 0 )
        return UTV_NO_DOWNLOAD;

    /*--- SWITCH TO USING pDescriptorData HERE UNTIL WE PROCESS THE PRIVATE DATA */
    /*--- (save pData so we can return to here after processing the private data.) */

    pDescriptorData = pData;
    iDescriptorLength = iSize;

    /* Skip past the compatibility data to the private descriptors. */
    for ( iGroupCount = 0; iGroupCount < iNumberOfGroups; iGroupCount++ )
    {
        /* GroupId and Size */
        UtvSkipBytesPointer( 8, &pDescriptorData, &iDescriptorLength );

        /* groupCompatibilityDescriptor */
        UtvSkipBytesPointer( UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength ), &pDescriptorData, &iDescriptorLength );

        /* GII groupInfoLength, should be zero, not used by UpdateTV */
        UtvSkipBytesPointer( UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength ), &pDescriptorData, &iDescriptorLength );
    }

    /* End of group loop so now we should be pointing at the UpdateTV privateData */
    /* Setup variables to receive privatedata descriptors. */
    iGroupsInfoPrivateDataLength = UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength );
    dtSystemTime = 0;
    iGpsUtcOffset = 0;
    iTimeProtocolVersion = 0;
    iNetworkId = 0;
    iNetworkVersion = 0;
    bFirstDescriptor = 1;
    bOurMRDFound = 0;
    bOurVersionFound = 0;

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
    /* factory mode is always assumed to be false and can be proven true only by 
       the presence of a factory mode descriptor. */
    g_bDSIContainsFactoryDescriptor = UTV_FALSE;
#endif

    while ( iGroupsInfoPrivateDataLength > 0 )
    {
        iGroupsInfoDescriptor = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
        iGroupsInfoSize = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
        iGroupsInfoPrivateDataLength -= ( 2 + iGroupsInfoSize );
        switch ( iGroupsInfoDescriptor )
        {
            /* Version 3 networks start with an MRD */
            case UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_TAG:
                /* MUST BE first descriptor */
                if ( !bFirstDescriptor )
                {
                    return UTV_BAD_DSI_MRD;
                }

                /* Size depends on the number of descriptors and some are optional */
                /* At the very least it must be big enough to contain a 4 byte MRD, */
                /* a 6 byte network ID descriptor, and a 10 byte time descriptor */
                if ( 20 > iGroupsInfoSize )
                {
                    return UTV_BAD_DSI_BDC;
                }

                /* Get format ID */
                for ( i = 0; i < UTV_ULI_PMT_REG_DESC_TOT_LEN; i++ )
                    szRegistration[ i ] = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                szRegistration[ i ] = 0;    /* zero terminate */

                /* check for base comparison then range */
                if ( UtvMemcmp( szRegistration, (void *) UTV_ULI_PMT_REG_DESC_BASE, UTV_ULI_PMT_REG_DESC_BASE_LEN ) != 0 ||
                     szRegistration[ UTV_ULI_PMT_REG_DESC_BASE_LEN ] < UTV_ULI_PMT_REG_DESC_ORG ||
                     szRegistration[ UTV_ULI_PMT_REG_DESC_BASE_LEN ] > UTV_ULI_PMT_REG_DESC_END )
                {
                    /* MRD descriptor is not ours, punt */
                    UtvMessage( UTV_LEVEL_ERROR, "  bad MRD \"%s\" found for DSI on PID 0x%X", szRegistration, iCarouselPid );
                    return UTV_BAD_DSI_MRD;
                }

                /* Descriptor is ours, this means that the rest of the parse for DSI descriptors is going */
                /* to take place in the "additional identification info" just beyond the MRD therefore */
                /* redirect the byte slurp to where we are now. */
                iGroupsInfoPrivateDataLength = iGroupsInfoSize - UTV_ULI_PMT_REG_DESC_TOT_LEN;
                bOurMRDFound = 1;
                break;

            /* Network id */
            case UTV_DSI_NETWORK_DESCRIPTOR:
                /* Check size */
                if ( 4 != iGroupsInfoSize )
                    return UTV_BAD_DSI_BDC;
                /* Get network */
                iNetworkId = UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength );
                iNetworkVersion = UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength );

                /* If this ID is bad then punt immediately. */
                if ( UTV_NETWORK_ID != iNetworkId )
                {
                    return UTV_BAD_DSI_BDC;
                }

                /* if we don't yet have a version match then look to see if this version */
                /* matches our target network ID */
                if ( !bOurVersionFound && UTV_NETWORK_VERSION == iNetworkVersion )
                    bOurVersionFound = 1;
                break;

            /* Copy of STT */
            case UTV_DSI_TIME_DESCRIPTOR:
                /* Check size */
                if ( 8 != iGroupsInfoSize )
                    return UTV_BAD_DSI_STT;
                /* Make sure time protocol version is right */
                iTimeProtocolVersion = UtvGetUint8Pointer( &pDescriptorData,  &iDescriptorLength);
                if ( 0 != iTimeProtocolVersion )
                    return UTV_BAD_DSI_STV;
                /* Now get the 32 bit system time aka epoch time (GPS seconds since January 6, 1980 0:00:00 UTC). */
                dtSystemTime = UtvGetUint32Pointer( &pDescriptorData, &iDescriptorLength );
                iGpsUtcOffset = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                /* Skip the DST bytes */
                UtvSkipBytesPointer( 2, &pDescriptorData, &iDescriptorLength );
                /* Set the system broadcast time */
                UtvSetBroadcastTime( dtSystemTime );
                UtvSetGpsUtcOffset( iGpsUtcOffset );
                UtvMessage( UTV_LEVEL_INFO, "    DSI broadcast time is %s", UtvConvertGpsTime( dtSystemTime ) );
                break;

            case UTV_DSI_SRVR_VERSION_DESCRIPTOR:
                /* it's the server's version descriptor, Copy it into a throw away buffer for now and display it. */
                for ( i = 0; i < iGroupsInfoSize && i < (UTV_UINT16)sizeof(s_ubServerVersion)-1; i++ )
                    s_ubServerVersion[ i ] = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                s_ubServerVersion[ i ] = 0;
                UtvMessage( UTV_LEVEL_INFO, "    Server Version: %s", (char *) s_ubServerVersion );
                break;

            case UTV_DSI_SRVR_ID_DESCRIPTOR:
                /* it's the server's ID.  Copy it into a throw away buffer for now and display it. */
                for ( i = 0; i < iGroupsInfoSize && i < (UTV_UINT16)sizeof(s_ubServerID)-1; i++ )
                    s_ubServerID[ i ] = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                s_ubServerID[ i ] = 0;
                UtvMessage( UTV_LEVEL_INFO, "    Server ID: %s", (char *) s_ubServerID );
                {
                    /* record server found event */
                    UTV_UINT32 uiPhysicalFrequency, uiModulationType;
                    if ( UTV_OK == UtvPlatformDataBroadcastGetTuneInfo( &uiPhysicalFrequency, &uiModulationType ))
                    {
                        UtvLogEventStringOneDecNum( UTV_LEVEL_NETWORK, UTV_CAT_INTERNET, UTV_TUNE_INFO, uiPhysicalFrequency, s_ubServerID, uiModulationType );
                    }
                }
                break;

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
            case UTV_DSI_FACTORY_MODE_DESCRIPTOR:
                /* Factory mode descriptor presence enables unencrypted payload. */
                g_bDSIContainsFactoryDescriptor = UTV_TRUE;
                UtvMessage( UTV_LEVEL_INFO, "    Factory mode descriptor detected" );
                break;
#endif

            /* All other descriptors result in an error */
            default:
                UtvMessage( UTV_LEVEL_WARN, "    DSI unknown private descriptor type %d size %d", iGroupsInfoDescriptor, iGroupsInfoSize );
                UtvSkipBytesPointer( iGroupsInfoSize, &pDescriptorData, &iDescriptorLength );
                break;
        }

        /* First descriptor encountered. */
        bFirstDescriptor = 0;

    } /* ( iGroupsInfoPrivateDataLength > 0 ) */

    /* Check to see that our network version was found in the descriptors. */
    if ( !bOurVersionFound )
    {
        UtvMessage( UTV_LEVEL_ERROR, "  DSI - invalid network version.  Version 0x%X not found", UTV_NETWORK_VERSION );
        return UTV_BAD_DSI_VER;
    }

    /*--- SWITCH BACK TO USING pData and iSize */

    /* Examine each GII, looking for OUI/Model Group combinations that interest us */
    for ( iGroupCount = 0; iGroupCount < iNumberOfGroups; iGroupCount++ )
    {
        /* The GroupId matches the transactionId of the DII */
        iGroupId = UtvGetUint32Pointer( &pData, &iSize );

        /* Skip by the groupSize (not used) */
        UtvSkipBytesPointer( 4, &pData, &iSize );

        /* Examine the GII's groupCompatibility structure */
        /* The UpdateTV implementation of A/97 reduces the role of this descriptor, enhancing fields in the DII instead */
        iGroupCompatibilityDescriptorLength = UtvGetUint16Pointer( &pData, &iSize );
        iOui = 0xFFFFFFFF;
        iModelGroup = 0;
        if ( iGroupCompatibilityDescriptorLength > 0 )
        {
            /* Parse the groupCompatibilityDescriptor using its own length */
            pDescriptorData = pData;
            iDescriptorLength = iGroupCompatibilityDescriptorLength;

            /* Nonzero iGroupCompatibilityDescriptorLength, get the compatibilityDescriptorCount */
            iGroupCompatibilityDescriptorCount = UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength  );

            /* The count must be at least one */
            if ( iGroupCompatibilityDescriptorCount > 0 )
            {
                /* A/97 says the first descriptor must be a certain format */
                iCdDescriptorTag = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                iCdDescriptorLength = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                iCdSpecifierType = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorLength );
                iCdSpecifierData = UtvGetUint24Pointer( &pDescriptorData, &iDescriptorLength );
                iCdModel = UtvGetUint16Pointer( &pDescriptorData, &iDescriptorLength );
                /* We don't care about the rest of the information in this descriptor */

                /* UpdateTV policy is to only look at the first descriptor, which must be the hardware descriptor */
                /* If the specifier type is system hardware with IEEE OUI and we are set */
                if ( iCdDescriptorLength != 0 && iCdDescriptorTag == 0x01 && iCdSpecifierType == 0x01 )
                {
                    /* UpdateTV policy is to only use the OUI and model (which is treated as a model group) */
                    iOui = iCdSpecifierData;
                    iModelGroup = iCdModel;
                }
            }
        }

        /* Skip over the entire groupCompatibilityDescriptor, as we may not look at all of it */
        UtvSkipBytesPointer( iGroupCompatibilityDescriptorLength, &pData, &iSize );

        /* Next in the GII groupInfoLength, should be zero, not used by UpdateTV */
        UtvSkipBytesPointer( UtvGetUint16Pointer( &pData, &iSize ), &pData, &iSize );

        /* Did we get a good OUI and MG? */
        if ( iOui != 0xFFFFFFFF )
        {
            /* Yes, see if we have an OUI and Model Group that is worth checking out */
            if ( ( uTVResult = UtvCoreCommonCompatibilityDownload( iOui, iModelGroup ) ) == UTV_OK )
            {
                /* We need to look at some DIIs so add this group to the list */
                UtvGroupCandidate * pUtvGroupCandidate = UtvNewGroupCandidate();
                if ( pUtvGroupCandidate == ( UtvGroupCandidate * ) -1 )
                    return UTV_NO_MEMORY;

                if ( pUtvGroupCandidate != NULL )
                {
                    /* Add this group id as a transaction id we are interested in */
                    pUtvGroupCandidate->iTransactionId = iGroupId;
                    pUtvGroupCandidate->iOui = iOui;
                    pUtvGroupCandidate->iModelGroup = iModelGroup;
                    pUtvGroupCandidate->iCarouselPid = iCarouselPid;
                    rStatus = UTV_OK;
                }

                UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "    compatible download: pid 0x%X, oui 0x%X, model group 0x%X", iCarouselPid, iOui, iModelGroup );
            }
            else
            {
                UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "    incompatible download: %s - pid 0x%X, oui 0x%X, model group 0x%X",
                    UtvGetMessageString( uTVResult ), iCarouselPid, iOui, iModelGroup );

                /* keep track of the last DSI related incompatibility status */
                UtvSetDSIIncompatStatus( uTVResult );
            }
        }
    }

    /* Save Server ID for the last scan whether successful or not */
    UtvSaveServerIDScan();

    /* Record last broadcast time that a module was received. */
    UtvSaveLastScanTime();

    /* Return our findings */
    return rStatus;
}

/* UtvScanDii
    Examine the DII data and look for something interesting to download

   Returns:

     UTV_OK - we found DIIs for all of the groups (OUI/MGs)
     that we're compatible with and at least one has a module has
     an acceptable download slot and UtvIsModuleCompatible() has OK'd

     UTV_NO_DOWNLOAD - we found DII's for all of the groups (OUI/MGs)
     that we're compatible with and NO modules have acceptable download
     times or are unacceptible to UtvIsModuleCompatible().

     UTV_DII_TIMEOUT - all other conditions whether due to a section filter
     error or a badly formed DII are returned as a timeout.
*/
UTV_RESULT UtvScanDii( UTV_UINT32 iCarouselPid )
{
    /* Return "no download available" status if no DII data found */
    UTV_RESULT rStatus = UTV_OK, rParseError = UTV_OK, rFinal = UTV_NO_DOWNLOAD;
    UTV_BYTE * pSectionData = NULL;
    UTV_UINT32 iWaitDii = UTV_WAIT_FOR_DII;
    UTV_UINT32 iNeedGroupInformation = 0;
    UTV_UINT32 iGroupCandidate;

    /* Set the diagnostic sub-state to scanning for a DII */
    UtvSetSubState( UTV_PUBLIC_SSTATE_SCANNING_DII, iWaitDii );

    /* Scanning DII */
    UtvMessage( UTV_LEVEL_INFO, " scanning for DII on PID 0x%X for %d ms", iCarouselPid, iWaitDii );

    /* Set up the section filter for the Carousel PID and the table for DSM-CC userNetworkMessage */
    if ( UTV_OK != ( rStatus = UtvOpenSectionFilter( iCarouselPid, UTV_TID_UNM, 0, UTV_FALSE ) ) )
        return rStatus;

    /* Loop around and get all of the DII data until we timeout or until we have found all the interesting groups */
    while ( iWaitDii > 0 )
    {
        /* Get the count of groups we still need information for */
        UtvGroupCandidate * pUtvGroupCandidate = NULL;
        iNeedGroupInformation = 0;
        for ( iGroupCandidate = 0; iGroupCandidate < UTV_GROUP_CANDIDATE_MAX; iGroupCandidate++ )
        {
            pUtvGroupCandidate = apUtvGroupCandidate[ iGroupCandidate ];
            if ( pUtvGroupCandidate != NULL )
            {
                if ( pUtvGroupCandidate->iSeenCount == 0 )
                {
                    /* We need information about this group */
                    iNeedGroupInformation++;
                }
            }
        }

        /* If we have the information on all the groups, we can stop waiting for data */
        if ( iNeedGroupInformation == 0 )
            break;

        /* Get the data for the DII, set return status if any interesting data seen */
        rStatus = UtvGetSectionData( &iWaitDii, &pSectionData );
        if ( rStatus == UTV_ABORT )
        {
            rFinal = UTV_ABORT;
            break;
        }

        if ( rStatus == UTV_OK )
        {
            /* We got section data, try parsing it now */
            rParseError = UtvParseDii( pSectionData );

            /* Whenever we get a compatible module then the final return state */
            /* has the chance of going to OK.  We may still not find DIIs for all of the */
            /* groups that we happen to be compatible with which will result in a timeout */
            if ( UTV_OK == rParseError )
                rFinal = UTV_OK;

            /* There are three conditions that don't result in an error being recorded: */
            /*    1.  Modules found that are compatible. */
            /*    2.  No modules found that are compatible. */
            /*    3.  The section we thought was a DII turned out to be a DSI. */
            /* All of these are normal. */
            if ( rParseError != UTV_OK &&
                 rParseError != UTV_NO_DOWNLOAD &&
                 rParseError != UTV_BAD_DII_MID )
            {
                /* otherwise record the error */
                UtvSetErrorCount( rParseError, iCarouselPid );
            }
        } else
        {
            /* record the UtvGetSectionData error */
            UtvSetErrorCount( rStatus, iCarouselPid );
        }

    }

    /* Timeout means we didn't find what we wanted for many possible reasons. */
    /* Just let timeout represent all of those reasons. */
    /* The set error counts calls in the loop above will record exception conditions */
    if ( UTV_TIMEOUT == rStatus )
    {
        UtvMessage( UTV_LEVEL_TEST, " DII scan timeout - waiting for a maximum of 0x%X groups on PID 0x%X", iNeedGroupInformation, iCarouselPid );
        rFinal = UTV_DII_TIMEOUT;
    }

    /* convert the final return value to the last parse error if it's still no download */
    if ( UTV_NO_DOWNLOAD == rFinal )
        rFinal = rParseError;

    UtvCloseSectionFilter();
    return rFinal;
}

/* UtvParseDii
    Parses one section worth of DII data
   Returns OK if this section parsed and was interesting to us
*/
UTV_RESULT UtvParseDii( UTV_BYTE * pSectionData )
{
    /* Set up to parse the section data, must be DSM CC user network message section */
    UTV_BYTE * pData = pSectionData;
    UTV_UINT32 iSize = 0;
    UTV_UINT32 iGroupCandidate;
    UTV_UINT32 iModule;
    UTV_UINT32 iScheduleDescriptorCount;
    UTV_UINT32 iUtvIndex;
    UTV_UINT32 iWakeUpEarly = UtvGetChannelScanTime();
    UTV_UINT32 iTransactionId;
    UTV_RESULT rStatus = UTV_NO_DOWNLOAD;
    UTV_UINT32 iModuleBlockSize;
    UTV_UINT32 iNumberOfModules;
    UTV_UINT32 iMiLength;
    UTV_BYTE * pMiData;
    UTV_UINT8  iMiDescriptorTag;
    UTV_UINT8  iMiDescriptorLength;
    UTV_UINT32 dtPossibleDownload;
    UTV_UINT32 iPossibleBroadcastSeconds;
    UTV_UINT32 iPossibleMillisecondsToStart;
    UTV_UINT32 iModuleNameBytes;
    UTV_UINT32 iSignatureLength;
    UTV_UINT32 iPrivateModuleLength;
    UTV_UINT32 iPmLength;
    UTV_BYTE * pPmData;
    UTV_UINT8  iUtvDescriptorLength;
    UTV_RESULT rIsCompatible = UTV_NO_DOWNLOAD;
    UtvGroupCandidate * pUtvGroupCandidate;
    UTV_BOOL   bNewModuleVersionDetected;

    if ( UTV_TID_UNM != UtvStartSectionPointer( &pData, &iSize ) )
        return UTV_BAD_DII_TID;

    /* if we're in a download event, change the wait time to something close to 30 seconds */
    if ( UtvGetState() == UTV_PUBLIC_STATE_DOWNLOAD )
        iWakeUpEarly = UTV_DOWNLOAD_TOO_SOON;

    /* Skip table id ext, version, section number, last section number */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1, &pData, &iSize );

    /* Check protocol */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_PROTOCOL_DISCRIMINATOR )
        return UTV_BAD_DII_DPD;

    /* Make sure it is a DII */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != UTV_DSMCC_UNM )
        return UTV_BAD_DII_UNM;

    /* Check for DSM-CC user-network download message that was a DSI */
    if ( UtvGetUint16Pointer( &pData, &iSize ) != UTV_DSMCC_DII )
        return UTV_BAD_DII_MID;

    /* Grab the transactionId */
    iTransactionId = UtvGetUint32Pointer( &pData, &iSize );

    /* Examine each group candidate to see if it matches this DII */
    for ( iGroupCandidate = 0; iGroupCandidate < UTV_GROUP_CANDIDATE_MAX; iGroupCandidate++ )
    {
        pUtvGroupCandidate = apUtvGroupCandidate[ iGroupCandidate ];
        if ( pUtvGroupCandidate != NULL )
        {
            /* Does this download candidate's transaction id from the DSI match this DII's transaction id? */
            if ( ( pUtvGroupCandidate->iTransactionId  & UTV_TRANSACTION_ID_MASK ) == ( iTransactionId & UTV_TRANSACTION_ID_MASK ) )
            {
                /* Yes, have we seen this transaction? */
                if ( pUtvGroupCandidate->iSeenCount == 0 )
                {
                    /* Indicate that we have a match pointed to by pUtvGroupCandidate */
                    rStatus = UTV_OK;

                    /* Remember that this transaction id has been looked at (so we don't rescan this DII again) */
                    pUtvGroupCandidate->iSeenCount++;
                    break;
                }
            }
        }
    }

    /* Return if we aren't interested in this DII */
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Make sure reserved byte set to fox fox */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0xff )
        return UTV_BAD_DII_RSV;

    /* Make sure adaptationLength is zero */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_DII_ADP;

    /* Now the DSM-CC length, completing the DSM-CC header */
    if ( UtvGetUint16Pointer( &pData, &iSize ) == 0 )
        return UTV_BAD_DII_DLN;

    /* Skip download id and get block size of every DDB block */
    UtvSkipBytesPointer( 4, &pData, &iSize );
    iModuleBlockSize = UtvGetUint16Pointer( &pData, &iSize );

    /* Skip the window size, ack period, download window, download scenario */
    UtvSkipBytesPointer( 1 + 1 + 4 + 4, &pData, &iSize );

    /* The compatibility descriptor length must be zero */
    if ( UtvGetUint16Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_DII_CDL;

    /* Get the count of number of modules to follow */
    iNumberOfModules = UtvGetUint16Pointer( &pData, &iSize );

    /* Check out each A/90 module */
    rStatus = UTV_NO_DOWNLOAD;
    for ( iModule = 0; iModule < iNumberOfModules; iModule++ )
    {
        /* Get module id, module size, skip module version */
        UTV_UINT16 iModuleId = UtvGetUint16Pointer( &pData, &iSize );
        UTV_UINT32 iModuleSize = UtvGetUint32Pointer( &pData, &iSize );
        UTV_UINT8  iModuleVersion = UtvGetUint8Pointer( &pData, &iSize );

        /* Get the moduleInfoLength field */
        UTV_UINT32 iModuleInfoLength = UtvGetUint8Pointer( &pData, &iSize );

        /* Init data pulled from the descriptors */
        UTV_BYTE   szModuleName[ UTV_MAX_NAME_BYTES + 1 ] = { 0 };
        UTV_UINT32 iModuleNameLength = 0;
        UTV_UINT32 dtDownload = 0;
        UTV_UINT32 iMillisecondsToStart = 0;
        UTV_UINT32 iBroadcastSeconds = 0;
        UTV_UINT32 iModulePriority = 0;
        UTV_UINT32 iUtvCompatLength = 0;
        UTV_BYTE * pUtvCompatData = NULL;
        UTV_UINT32 iUtvDescriptorTag = 0;
        UTV_UINT32 iScheduleDescriptors = 0;
        UTV_UINT32 iUtvModelHardwareSoftwareCount = 0;

        /* The moduleInfoBytes should be there */
        if ( iModuleInfoLength > 0 )
        {
            /* Parse the descriptors in the moduleInfoBytes */
            iMiLength = iModuleInfoLength;
            pMiData = pData;

            /* Examine descriptor data until there is no more to look at */
            while ( iMiLength > 0 )
            {
                /* Get the tag and length of the descriptor */
                iMiDescriptorTag = UtvGetUint8Pointer( &pMiData, &iMiLength );
                iMiDescriptorLength = UtvGetUint8Pointer( &pMiData, &iMiLength );
                switch ( iMiDescriptorTag )
                {
                    /* ATSC A/97 scheduleDescriptor decode */
                    case UTV_DII_SCHEDULE_DESCRIPTOR_TAG:
                        {
                            iScheduleDescriptors = iMiDescriptorLength / 8;
                            iMillisecondsToStart = 0;
                            for ( iScheduleDescriptorCount = 0; iScheduleDescriptorCount < iScheduleDescriptors; iScheduleDescriptorCount++ )
                            {
                                /* Get the download time and broadcast seconds for this descriptor */
                                dtPossibleDownload = UtvGetUint32Pointer( &pMiData, &iMiLength );  /* 32 bits of download time */
                                iPossibleBroadcastSeconds = UtvGetUint32Pointer( &pMiData, &iMiLength ) & 0xFFFFF;   /* (reserved 12 bits) 20 bits of seconds */
                                iPossibleMillisecondsToStart = UtvGetMillisecondsToEvent( dtPossibleDownload );

                                /* If ( we don't have a candidate yet OR this candidate is soonest ) */
                                /*  and the time to start is far enough in the future */
                                /*  and the time to start isn't too far in the future */
                                /*   then pick this scheduled time as the candidate */
                                if ( ( iMillisecondsToStart == 0 || dtPossibleDownload < dtDownload )
                                    && ( iPossibleMillisecondsToStart >= iWakeUpEarly )
                                    && ( iPossibleMillisecondsToStart < UTV_DOWNLOAD_TOO_FAR ) )
                                {
                                    /* Yes we have a possible candidate, not too far away */
                                    dtDownload = dtPossibleDownload;
                                    iBroadcastSeconds = iPossibleBroadcastSeconds;
                                    iMillisecondsToStart = iPossibleMillisecondsToStart;

                                    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                                        "    slot accepted: module %s ID %u, priority 0x%X, in %d seconds",
                                        szModuleName, iModuleId, iModulePriority, iPossibleMillisecondsToStart / 1000 );
                                }
                                else
                                {
                                    /* Log rejected time */
                                    if ( 0 == dtPossibleDownload || iPossibleMillisecondsToStart > UTV_DOWNLOAD_TOO_FAR )
                                    {
                                        /* too late */
                                        UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                                                    "    slot too late: module ID %u, scheduled in %d seconds",
                                                    iModuleId, iPossibleMillisecondsToStart / 1000 );
                                        UtvSetDIIIncompatStatus( UTV_BAD_DII_TTL );

                                    } else if ( iPossibleMillisecondsToStart < iWakeUpEarly )
                                    {
                                        /* too early */
                                        UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                                                    "    slot too early: module ID %u, scheduled in %d seconds",
                                                    iModuleId, iPossibleMillisecondsToStart / 1000 );
                                        UtvSetDIIIncompatStatus( UTV_BAD_DII_TTE );

                                    } else
                                    {
                                        /* time is later than another accepted download */
                                        UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                                                    "    slot later: module ID %u, scheduled in %d seconds",
                                                    iModuleId, iPossibleMillisecondsToStart / 1000 );
                                        UtvSetDIIIncompatStatus( UTV_BAD_DII_LAT );
                                    }
                                }
                            }
                            break;
                        }


                    /* ATSC A/97 moduleInfoDescriptor decode with UpdateTV extension */
                    case UTV_DII_MODULEINFO_DESCRIPTOR_TAG:
                        {
                            /* Skip the A/97 encoding */
                            UtvSkipBytesPointer( 1, &pMiData, &iMiLength );

                            /* Save the module name as null terminated ascii */
                            iModuleNameLength = UtvGetUint8Pointer( &pMiData, &iMiLength );
                            iModuleNameBytes = iModuleNameLength;
                            if ( iModuleNameBytes > UTV_MAX_NAME_BYTES )
                                iModuleNameBytes = UTV_MAX_NAME_BYTES;
                            UtvByteCopy( szModuleName, pMiData, iModuleNameBytes );
                            szModuleName[ iModuleNameBytes ] = 0;
                            UtvSkipBytesPointer( iModuleNameLength, &pMiData, &iMiLength );

                            /* Skip the A/97 signature type and the signature */
                            UtvSkipBytesPointer( 1, &pMiData, &iMiLength );
                            iSignatureLength = UtvGetUint8Pointer( &pMiData, &iMiLength );
                            UtvSkipBytesPointer( iSignatureLength, &pMiData, &iMiLength );

                            /* Get A/97 privateModuleLength, only continue with privateModuleByte section if length non-zero */
                            iPrivateModuleLength = UtvGetUint8Pointer( &pMiData, &iMiLength );
                            if ( iPrivateModuleLength != 0 )
                            {
                                /* Parse the descriptors in the moduleInfoBytes */
                                iPmLength = iPrivateModuleLength;
                                pPmData = pMiData;

                                /* UpdateTV descriptor starts with priority byte */
                                iModulePriority = UtvGetUint8Pointer( &pPmData, &iPmLength );

                                /* Verify UpdateTV descriptor tag */
                                iUtvDescriptorTag = UtvGetUint8Pointer( &pPmData, &iPmLength );
                                if ( iUtvDescriptorTag != UTV_DII_COMPATABILITY_DESCRIPTOR_TAG )
                                    break;

                                /* Save these pointers for later examination */
                                iUtvCompatLength = iPmLength;
                                pUtvCompatData = pPmData;

                                /* Skip by the parsed moduleInfoBytes */
                                UtvSkipBytesPointer( iPrivateModuleLength, &pMiData, &iMiLength );
                                break;
                            }
                        }
                    default:
                        {
                            UtvSkipBytesPointer( iMiDescriptorLength, &pMiData, &iMiLength );
                            break;
                        }
                }
            }
        }


        if ( iMillisecondsToStart == 0 )
            UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "    no acceptable slots for module %s, ID %u", szModuleName, iModuleId );

        /* The number UpdateTV of model/software/hardware descriptors determined by dividing the size */
        /* Old is 6 bytes per, new is 4 bytes per */
        iUtvDescriptorLength = UtvGetUint8Pointer( &pUtvCompatData, &iUtvCompatLength );
        iUtvModelHardwareSoftwareCount = iUtvDescriptorLength / 4;
        for ( iUtvIndex = 0; iUtvIndex < iUtvModelHardwareSoftwareCount; iUtvIndex++ )
        {
            UTV_UINT32 iHardwareModelBegin  = UtvGetUint8Pointer( &pUtvCompatData, &iUtvCompatLength );
            UTV_UINT32 iHardwareModelEnd = UtvGetUint8Pointer( &pUtvCompatData, &iUtvCompatLength );
            UTV_UINT32 iSoftwareVersionBegin  = UtvGetUint8Pointer( &pUtvCompatData, &iUtvCompatLength );
            UTV_UINT32 iSoftwareVersionEnd = UtvGetUint8Pointer( &pUtvCompatData, &iUtvCompatLength );

            /* Call the Compatibility layer to check out this module */
            rIsCompatible = UtvCoreCommonCompatibilityModule( pUtvGroupCandidate->iOui, pUtvGroupCandidate->iModelGroup,
                                                       iModulePriority, iModuleNameLength, szModuleName, iModuleVersion,
                                                       iHardwareModelBegin, iHardwareModelEnd,
                                                       iSoftwareVersionBegin, iSoftwareVersionEnd, iModuleSize, iModuleId, 
                                                       &bNewModuleVersionDetected );
            switch ( rIsCompatible )
            {
                case UTV_OK:

                    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                        "    compatible: module %s, ID %u, model group 0x%X, hardware 0x%X to 0x%X, software 0x%X to 0x%X, module version 0x%X",
                        szModuleName, iModuleId, pUtvGroupCandidate->iModelGroup, iHardwareModelBegin, iHardwareModelEnd, iSoftwareVersionBegin,
                        iSoftwareVersionEnd, iModuleVersion );
                    break;

                default:
                    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "    incompatible: %s", UtvGetMessageString( rIsCompatible ) );
                    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST,
                        "    (module %s hardware model 0x%X to 0x%X software version 0x%X to 0x%X module version 0x%X)",
                        szModuleName, iHardwareModelBegin, iHardwareModelEnd, iSoftwareVersionBegin, iSoftwareVersionEnd, iModuleVersion);

                    /* keep track of the last DII related incompatibility status. */
                    UtvSetDIIIncompatStatus( rIsCompatible );
                    break;
            }

            /* If we like this module add it to the download candidate list */
            if ( rIsCompatible == UTV_OK )
            {
                /* If a new module version is detected then flush all accumulated candidates */
                if ( bNewModuleVersionDetected )
                    UtvClearDownloadCandidate();

                /* If we have a start time, add this module as a new download candidate on this carousel */
                if ( iMillisecondsToStart != 0 )
                {
                    UtvDownloadInfo * pUtvDownloadCandidate = UtvNewDownloadCandidate();
                    if ( pUtvDownloadCandidate == ( UtvDownloadInfo * ) -1 )
                        return UTV_NO_MEMORY;

                    if ( pUtvDownloadCandidate != NULL )
                    {

                        /* We have a potential candidate */
                        UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "  adding candidate module ID %u, %s scheduled in %d seconds for %d seconds", 
                                    iModuleId, szModuleName, iMillisecondsToStart /1000, iBroadcastSeconds );
                        /* Copy the DSI/group candidate information to the download candidate */
                        pUtvDownloadCandidate->iFrequency = pUtvGroupCandidate->iFrequency;
                        pUtvDownloadCandidate->iTransactionId = pUtvGroupCandidate->iTransactionId;
                        pUtvDownloadCandidate->iModelGroup = pUtvGroupCandidate->iModelGroup;
                        pUtvDownloadCandidate->iOui = pUtvGroupCandidate->iOui;
                        pUtvDownloadCandidate->iCarouselPid = pUtvGroupCandidate->iCarouselPid;

                        /* Copy the DII/GII data into this download candidate */
                        pUtvDownloadCandidate->iNumberOfModules = iNumberOfModules;
                        pUtvDownloadCandidate->iModuleNameLength = iModuleNameLength;
                        UtvByteCopy( pUtvDownloadCandidate->szModuleName, szModuleName, iModuleNameLength + 1 );
                        pUtvDownloadCandidate->iBroadcastSeconds = iBroadcastSeconds;
                        pUtvDownloadCandidate->dtDownload = dtDownload;
                        pUtvDownloadCandidate->iMillisecondsToStart = iMillisecondsToStart;
                        pUtvDownloadCandidate->iModulePriority = iModulePriority;
                        pUtvDownloadCandidate->iModuleSize = iModuleSize;
                        pUtvDownloadCandidate->iModuleVersion = iModuleVersion;
                        pUtvDownloadCandidate->iModuleBlockSize = iModuleBlockSize;
                        pUtvDownloadCandidate->iHardwareModelBegin = iHardwareModelBegin;
                        pUtvDownloadCandidate->iHardwareModelEnd = iHardwareModelEnd;
                        pUtvDownloadCandidate->iSoftwareVersionBegin = iSoftwareVersionBegin;
                        pUtvDownloadCandidate->iSoftwareVersionEnd = iSoftwareVersionEnd;
                        pUtvDownloadCandidate->iModuleId = iModuleId;

                        /* Indicate we found at least one candidate */
                        rStatus = UTV_OK;

                        /* We know this module is acceptable, no need to keep checking sw hw compat entries */
                        break;
                    }
                }
            }
        }

        if ( iUtvModelHardwareSoftwareCount == 0 )
        {
            UtvMessage( UTV_LEVEL_TEST, "   compatibility descriptors missing - invalid module %s, ID %u", szModuleName, iModuleId );
            rStatus = UTV_BAD_DII_NCD;
            break;
        }

        if ( iScheduleDescriptors == 0 )
        {
            UtvMessage( UTV_LEVEL_TEST, "   schedule descriptors missing - invalid module %s, ID %u", szModuleName, iModuleId );
            rStatus = UTV_BAD_DII_NSD;
            break;
        }

        /* Skip by the parsed moduleInfoBytes */
        UtvSkipBytesPointer( iModuleInfoLength, &pData, &iSize );

    } /* for each module */

    /* Now get the length of the private data bytes and skip them */
    UtvSkipBytesPointer( UtvGetUint16Pointer( &pData, &iSize ), &pData, &iSize );

    /* Return the status of our search through this DII */
    if ( rStatus == UTV_OK )
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  found DII 0x%X with module candidates", iTransactionId );
    else
    {
        /* if we didn't find a candidate and there weren't any unusual erros then return the Compatibility error */
        if ( rStatus == UTV_NO_DOWNLOAD )
            rStatus = rIsCompatible;
        UtvMessage( UTV_LEVEL_INFO, "  found DII 0x%X but no module candidates were found", iTransactionId );
    }
    return rStatus;
}

/* UtvClearGroupCandidate
    Clears all information in table.
*/
void UtvClearGroupCandidate( )
{
    UTV_UINT32 iIndex;
    
    /* Go through each possible UtvGroupCandidate */
    for ( iIndex = 0; iIndex < UTV_GROUP_CANDIDATE_MAX; iIndex++ )
    {
        /* If this slot is in use, free it */
        if ( apUtvGroupCandidate[ iIndex ] != NULL )
        {
            UtvFree( apUtvGroupCandidate[ iIndex ] );
            apUtvGroupCandidate[ iIndex ] = NULL;
        }
    }
}

/* UtvNewGroupCandidate
    Gets memory for a new UtvGroupCandidate.
   Returns NULL or pointer to new candidate.
*/
UtvGroupCandidate * UtvNewGroupCandidate( )
{
    UTV_UINT32 iIndex;

    /* Look for an empty slot */
    for ( iIndex = 0; iIndex < UTV_GROUP_CANDIDATE_MAX; iIndex++ )
    {
        /* Is this slot free? */
        if ( apUtvGroupCandidate[ iIndex ] == NULL )
        {
            /* Found a slot, allocate a new one, fill with zero */
            apUtvGroupCandidate[ iIndex ] = (UtvGroupCandidate *) UtvMalloc( sizeof( UtvGroupCandidate ) );

            /* If we got the memory, return the pointer to the new guy */
            if ( apUtvGroupCandidate[ iIndex ] != NULL )
            {
                /* Fill in the frequency and return the pointer to the new guy */
                apUtvGroupCandidate[ iIndex ]->iFrequency = UtvPlatformDataBroadcastGetTunerFrequency();
                return apUtvGroupCandidate[ iIndex ];
            }
            else
            {
                /* out of memory - no reason to keep going */
                return ( UtvGroupCandidate * ) -1;
            }
        }
    }

    /* No slots available */
    return NULL;
}

/* UtvClearDownloadCandidate
    Clears all information in UtvDownloadCandidate table.
*/
void UtvClearDownloadCandidate( )
{
    UTV_UINT32 iIndex;

    /* Clear counter */
    iUtvDownloadCandidate = 0;

    /* Clear each possible UtvDownloadCandidate */
    for ( iIndex = 0; iIndex < UTV_DOWNLOAD_CANDIDATE_MAX; iIndex++ )
    {
        /* If this slot is in use, free it */
        if ( apUtvDownloadCandidate[ iIndex ] != NULL )
        {
            /* Free the download candidate storage */
            UtvFree( apUtvDownloadCandidate[ iIndex ] );
            apUtvDownloadCandidate[ iIndex ] = NULL;
        }
    }
}

/* UtvNewDownloadCandidate
    Assigns a new UtvDownloadInfo
   Returns NULL or pointer to new storage
*/
UtvDownloadInfo * UtvNewDownloadCandidate( )
{
    UTV_UINT32 iIndex;

    /* Look for an empty slot */
    for ( iIndex = 0; iIndex < UTV_DOWNLOAD_CANDIDATE_MAX; iIndex++ )
    {
        /* Is this slot free? */
        if ( apUtvDownloadCandidate[ iIndex ] == NULL )
        {
            /* Found a slot, allocate a new one, fill with zero */
            apUtvDownloadCandidate[ iIndex ] = (UtvDownloadInfo *) UtvMalloc( sizeof( UtvDownloadInfo ) );

            /* If we got the memory, return the pointer to the new guy */
            if ( apUtvDownloadCandidate[ iIndex ] != NULL )
            {
                iUtvDownloadCandidate++;
                return apUtvDownloadCandidate[ iIndex ];
            }
            else
            {
                return ( UtvDownloadInfo * ) -1;
            }
        }
    }

    /* No slots available */
    return NULL;
}

/* UtvGetServerVersion
   Returns value of server version.  Only valid if called from within UtvIsDownloadCompatible()
   otherwise returns NULL.
*/
char *UtvGetServerVersion( )
{
    if ( 0 == s_ubServerVersion[ 0 ] )
        return "(unknown)";
    return s_ubServerVersion;
}

/* UtvGetServerID
   Returns value of server ID (call sign).  Only valid if called from within UtvIsDownloadCompatible()
   otherwise returns NULL.
*/
char *UtvGetServerID( )
{
    if ( 0 == s_ubServerID[ 0 ] )
        return "(unknown)";
    return s_ubServerID;
}

/* UtvSetChannelScanTime
    Is used in Interactive Mode to set the threshold used to reject download candidates that are too soon.
*/
void UtvSetChannelScanTime( UTV_UINT32 uiChannelScanTime )
{
    s_uiChannelScanTime = uiChannelScanTime;
}

/* UtvGetChannelScanTime
    Called to get the threshold used to reject download candidates that are too soon.
   Returns time of channel scan in milliseconds.  Returns UTV_MAX_CHANNEL_SCAN if UtvSetChannelScanTime() is not called
   to override that default value.
*/
UTV_UINT32 UtvGetChannelScanTime( void )
{
    return s_uiChannelScanTime;
}

void UtvSetAttributeMatchCriteria( UTV_UINT32 uiAttributeBitMask, UTV_UINT32 uiAttributeSenseMask )
{
    s_uiAttributeBitMask = uiAttributeBitMask;
    s_uiAttributeSenseMask = uiAttributeSenseMask;
}
