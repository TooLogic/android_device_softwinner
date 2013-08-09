/* corechannel.c: UpdateTV Common Core - UpdateTV Channel Scan

  This module implements the UpdateTV Channel Scan.

  In order to ensure UpdateTV network compatibility, customers must not
  change UpdateTV Common Core code.

 Copyright (c) 2004 - 2009 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      02/02/2009  Created from 1.9.21 for version 2.0.0.
*/

#include <string.h>
#include "utv.h"

/* UtvCarouselCandidate storage management
*/
UtvCarouselCandidate * apUtvCarouselCandidate[ UTV_CAROUSEL_CANDIDATE_MAX ] = { NULL };
UTV_UINT32 giUtvCarouselCandidates = 0;

/* Local and worker routines 
*/
UTV_RESULT UtvScanVct( UTV_TID iTableId );
UTV_RESULT UtvParseVct( UTV_TID iTableId, UTV_BYTE * pSectionData );
UTV_RESULT UtvScanPat( void );
UTV_RESULT UtvParsePat( UTV_BYTE * pSectionData );
UTV_RESULT UtvScanPmt( UtvCarouselCandidate * pUtvCarouselCandidate );
UtvCarouselCandidate * UtvNewCarouselCandidate( void );
UTV_RESULT UtvDeleteCarouselCandidate( UTV_UINT32 iIndex );
UtvCarouselCandidate * UtvGetCarouselCandidateFromPNum( UTV_UINT32 iProgramNumber );
UtvCarouselCandidate * UtvGetCarouselCandidateFromEPid( UTV_UINT32 iElementaryPid );

/* UtvClearCarouselCandidate
    Clears all information in table and PMT Pid list
*/
void UtvClearCarouselCandidate( )
{
    UTV_UINT32 iIndex;
    
    /* Go through each possible UtvCarouselCandidate */
    for ( iIndex = 0; iIndex < UTV_CAROUSEL_CANDIDATE_MAX; iIndex++ )
    {
        /* If this slot is in use, free it */
        if ( apUtvCarouselCandidate[ iIndex ] != NULL )
        {
            UtvFree( apUtvCarouselCandidate[ iIndex ] );
            apUtvCarouselCandidate[ iIndex ] = NULL;
        }
    }

    /* None in use */
    giUtvCarouselCandidates = 0;

}

/* UtvNewCarouselCandidate
    Assigns a new section filter handle
   Returns the new one or NULL
*/
UtvCarouselCandidate * UtvNewCarouselCandidate( )
{
    UTV_UINT32 iIndex;

    /* Look for an empty slot */
    for ( iIndex = 0; iIndex < UTV_CAROUSEL_CANDIDATE_MAX; iIndex++ )
    {
        /* Is this slot free? */
        if ( apUtvCarouselCandidate[ iIndex ] == NULL )
        {
            /* Found a slot, allocate a new one filled with zeros and return that address */
            apUtvCarouselCandidate[ iIndex ] = ( UtvCarouselCandidate * )UtvMalloc( sizeof( UtvCarouselCandidate ) );

            /* If we got the memory, return the pointer to the new guy */
            if ( apUtvCarouselCandidate[ iIndex ] != NULL )
            {
                giUtvCarouselCandidates++;
                return apUtvCarouselCandidate[ iIndex ];
            }
            else
            {
                return NULL;
            }
        }
    }

    /* No slots available */
    return NULL;
}

/* UtvDeleteCarouselCandidate
    Delete an entry and its resources in the candidate table
   Returns UTV_OK if it worked, UTV_NO_DATA if it didn't
*/
UTV_RESULT UtvDeleteCarouselCandidate( UTV_UINT32 iIndex )
{
    UTV_UINT32 n;

    /* sanity check index */
    if ( iIndex >= UTV_CAROUSEL_CANDIDATE_MAX || NULL == apUtvCarouselCandidate[ iIndex ] )
        return UTV_NO_DATA;

    /* Free memory associated with entry */
    UtvFree( apUtvCarouselCandidate[ iIndex ] );

    /* copy subsequent entries up overwriting the deleted entry */
    for ( n = iIndex; n < giUtvCarouselCandidates-1; n++ )
        apUtvCarouselCandidate[ n ] = apUtvCarouselCandidate[ n + 1 ];
    apUtvCarouselCandidate[ n ] = NULL;
    giUtvCarouselCandidates -= 1;

    return UTV_OK;
}

UtvCarouselCandidate * UtvGetCarouselCandidateFromPNum( UTV_UINT32 iProgramNumber )
{
    UTV_UINT32 iIndex;

    for ( iIndex = 0; iIndex < giUtvCarouselCandidates; iIndex++ )
    {
        if ( apUtvCarouselCandidate[iIndex]->iProgramNumber == iProgramNumber )
        {
            return apUtvCarouselCandidate[iIndex];
        }
    }

    return NULL;
}

UtvCarouselCandidate * UtvGetCarouselCandidateFromEPid( UTV_UINT32 iElementaryPid )
{
    UTV_UINT32 iIndex;

    for ( iIndex = 0; iIndex < giUtvCarouselCandidates; iIndex++ )
    {
        if ( apUtvCarouselCandidate[iIndex]->iCarouselPid == iElementaryPid )
        {
            return apUtvCarouselCandidate[iIndex];
        }
    }

    return NULL;
}

/* UtvChannelScan
    Scan the list of frequencies and return the pid and time any download in the next 24 hours
   Returns the soonest compatible download's frequency, carousel pid, download start, and milliseconds to sleep
*/
UTV_RESULT UtvChannelScan( UtvDownloadInfo * pUtvBestCandidate )
{
    /* Local storage */
    UTV_UINT32 iFrequencyListSize = 0;
    UTV_UINT32 * pFrequencyList = NULL;
    UTV_RESULT rStatus = UTV_NO_DOWNLOAD;
    UTV_BOOL bDownloadFound = UTV_FALSE;
    UTV_UINT32 iIndex;

    /* Clear the best candidate download info, reset all scanning parameters */
    /* These three fields are all that needs to be cleared for intialization */
    pUtvBestCandidate->dtDownload = 0;
    pUtvBestCandidate->iMillisecondsToStart = 0;
    pUtvBestCandidate->iCarouselPid = 0;

    /* Ask the Personality Map for the list of frequencies for the Channel Scan */
    if ( UtvGetTunerFrequencyList( &iFrequencyListSize, &pFrequencyList ) != UTV_OK )
        return UTV_NO_FREQUENCY_LIST;

    /* set frequency list size in diag stats */
    g_pUtvDiagStats->iFreqListSize = iFrequencyListSize;
    g_pUtvDiagStats->iChannelIndex = 0;
    g_pUtvDiagStats->iBadChannelCount = 0;

    /* Check all the frequencies */
    for ( iIndex = 0; iIndex < iFrequencyListSize; iIndex++ )
    {
        /* Tune the desired frequency now */
        UTV_UINT32 iFrequency = pFrequencyList[ iIndex ];
        rStatus = UtvPlatformDataBroadcastSetTunerState( iFrequency );

        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            return UTV_ABORT;

        if ( rStatus == UTV_OK )
        {
            /* set last frequency tuned in diag stats and update persistent memory */
            g_pUtvDiagStats->iLastFreqTuned = iFrequency;
            UtvPlatformPersistWrite();

            UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, "  tuned to %d ", iFrequency );
            /* Frequency tuned, now create section filter */
            rStatus = UtvCreateSectionFilter();
            if ( rStatus != UTV_OK )
                return rStatus;

            /* Section filter is all set, now scan this frequency */
            rStatus =  UtvScanOneFrequency( pUtvBestCandidate );
            if ( UTV_OK == rStatus )
                bDownloadFound = UTV_TRUE;
            else
                UtvMessage( UTV_LEVEL_INFO, "  no download on frequency %d: %s", iFrequency, UtvGetMessageString( rStatus ) );

            /* Shut down the section filter */
            UtvDestroySectionFilter();

            if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
                return UTV_ABORT;

        } else
        {
            /* Update persistent stats and write persistent memory so that scan progress can be detected. */
            g_pUtvDiagStats->iBadChannelCount++;
            UtvSetErrorCount( rStatus, iFrequency );
            UtvPlatformPersistWrite();
        }

        g_pUtvDiagStats->iChannelIndex++;
    }

    /* Finished with scanning all frequencies, best candidate set up if ok, otherwise return last error code */
    if ( bDownloadFound == UTV_TRUE )
    {
        return UTV_OK;
    }
    else
    {
        if ( rStatus == UTV_NO_DOWNLOAD )
            UtvMessage( UTV_LEVEL_TEST, "  No compatibile downloads found during scan event" );
        return rStatus;
    }
}

/* UtvScanOneFrequency
    Scan the carousels on one frequency
   Returns OK if a carousel pid and download time returned
   else returns the most severe error encountered.
*/
UTV_RESULT UtvScanOneFrequency( UtvDownloadInfo * pUtvBestCandidate )
{
    UTV_RESULT                 rStatus, lStatus, rError;
    UTV_UINT32                 iIndex, iInfo = 0, iPid;
    UtvCarouselCandidate  * pUtvCarouselCandidate;

    /* Clear the UtvCarouselCandidate table */
    UtvClearCarouselCandidate();

    /* STEP ZERO.  If a PID has been provided then use it. */
    if ( UTV_OK != ( rStatus = UtvGetPid( &iPid )))
        return rStatus;

    /* if the PID is non-zero then create a carousel candidate from it. */
    if ( 0 != iPid )
    {
        if ( NULL == ( pUtvCarouselCandidate = UtvNewCarouselCandidate( )))
            return UTV_NO_MEMORY;

        pUtvCarouselCandidate->iCarouselPid = iPid;

        /* Get frequency from tuner.  Assumes tuner has already been tuned. */
        pUtvCarouselCandidate->iFrequency = UtvPlatformDataBroadcastGetTunerFrequency();
        pUtvCarouselCandidate->iProgramNumber = 0;
        pUtvCarouselCandidate->bDiscoveredInVCT = 0;
        UtvMessage( UTV_LEVEL_INFO, "  carousel on PID 0x%X via programmatic control", iPid );

    } else
    {
#ifdef UTV_INTERACTIVE_MODE
        /* in factory mode there isn't a VCT scan */
        if ( !UtvGetFactoryMode() )
        {
#endif
#ifndef UTV_DISABLE_VCT_SCAN
            /* STEP ONE.  Generate candidates from the T & C VCT if they exist. */

            /* Scan the TVCT looking for any download services */
            if ( UTV_OK != ( rStatus = UtvScanVct( UTV_TID_TVCT )))
            {
                /* If we haven't found any download service try the CVCT */
                rStatus = UtvScanVct( UTV_TID_CVCT );
            }

            /* If we failed to get a valid VCT, we can scan using the PAT and PMT only */
            /* Any other error indicates we're dealing with a hw or sw problem and we're done */
            if ( rStatus != UTV_OK && rStatus != UTV_VCT_TIMEOUT && rStatus != UTV_NO_DOWLOAD_SERVICE )
                return rStatus;
#endif
#ifdef UTV_INTERACTIVE_MODE
        }
#endif

        /* STEP TWO.  Generate PMT PIDs for all existing candidates from the T/C VCT scan */
        /*            AND create new candidates for all programs that are not associated with */
        /*            VCT entries. */

        /* Scan the PAT which will generate a list of candidates that might be a super set of */
        /* the candidates already recommended by the VCT scan. */
        if ( UTV_OK != ( rStatus = UtvScanPat()))
            return rStatus;

        /* If the PAT scan didn't turn up any candidates we might have an empty PAT. */
        /* indicate no BDC data. */
        if ( 0 == giUtvCarouselCandidates )
            return UTV_NO_ULI_DATA;

        /* STEP THREE.  Find an elementary PID for each candidate by looking at the */
        /*              PMT associated with it. */

        for ( iIndex = 0; iIndex < giUtvCarouselCandidates; iIndex++ )
        {
            if ( NULL == ( pUtvCarouselCandidate = apUtvCarouselCandidate[ iIndex ] ))
            {
                /* this can't happen.  The carousel candidate list is not sparse. */
                return UTV_NO_DATA;
            }

            /* Scan the PMT asscoiated with this program for a download service PID. */
            if ( UTV_OK != ( rStatus = UtvScanPmt( pUtvCarouselCandidate )))
            {
                /* check for abort */
                if ( UTV_ABORT == rStatus )
                {
                    /* if this was an abort then kill all candidates and punt. */
                    UtvClearCarouselCandidate();
                    return UTV_ABORT;
                }

                /* something wrong with this candidate, delete it */
                if ( UTV_OK != ( lStatus = UtvDeleteCarouselCandidate( iIndex )))
                    return lStatus;

                /* back up the index to look at the current location unless */
                /* this was the last entry. */
                if ( iIndex != giUtvCarouselCandidates )
                    iIndex--;
            }
        }
    }  /* if PID not programmatically provided */

    /* If there are no candidates left after the PMT scan then indicate no download. */
    if ( 0 == giUtvCarouselCandidates )
        return rStatus;

     /* If we are only scanning to see if BDC data MIGHT be available, and not looking for a download, return here */
    if (!pUtvBestCandidate)
        return UTV_OK;

   /* PID scan candidates established:  look at each for a carousel candidate */

    /* keep track of the worst error */
    rError = UTV_NO_DOWNLOAD;
    for ( iIndex = 0; iIndex < giUtvCarouselCandidates; iIndex++ )
    {
        if ( NULL == ( pUtvCarouselCandidate = apUtvCarouselCandidate[ iIndex ] ))
        {
            /* this can't happen.  The carousel candidate list is not sparse. */
            return UTV_NO_DATA;
        }

        if ( 0 == pUtvCarouselCandidate->iCarouselPid )
        {
            /* If we don't have an elementary pid do then there is a serious error */
            return UTV_BAD_PID;
        }

        /* Scan the carousel looking for downloads on this one PID */
        if ( UTV_OK != ( rStatus = UtvCarouselScan( pUtvCarouselCandidate->iCarouselPid, pUtvBestCandidate )))
        {
            /* check for abort */
            if ( UTV_ABORT == rStatus )
            {
                /* if this was an abort then kill all candidates and punt. */
                UtvClearCarouselCandidate();
                return UTV_ABORT;
            }

            /* keep track of errors other than there isn't any download info available */
            if ( UTV_NO_DOWNLOAD != rStatus )
            {
                rError = rStatus;
                /* remember the PID the error occurred on */
                iInfo = pUtvCarouselCandidate->iCarouselPid;
            }


            /* something wrong with this candidate, delete it */
            if ( UTV_OK != ( rStatus = UtvDeleteCarouselCandidate( iIndex )))
                return rStatus;

            /* back up the index to look at the current location unless */
            /* this was the last entry. */
            if ( iIndex != giUtvCarouselCandidates )
                iIndex--;
        }
    }


    /* If there are no candidates left after the carousel scan then return */
    /* the worst error encountered. */
    if ( 0 == giUtvCarouselCandidates )
        return rError;

    /* There are candidates.  If there are any errors record them now because they won't */
    /* percolate up due to our pre-occupation with going after the candidate. */
    if ( UTV_NO_DOWNLOAD != rError )
        UtvSetErrorCount( rError, iInfo );

    return UTV_OK;
}

/* UtvScanVct
    Scan the VCT looking for virtual channels offering service for us
    Can scan CVCT or TVCT since they are in the same format except for the table id
    Get the information for virutal channels with service_type 0x05 (software download data service)
   Returns OK if data added to the UtvCarouselCandidate
*/
UTV_RESULT UtvScanVct( UTV_TID iTableId )
{
    UTV_RESULT rStatus = UTV_UNKNOWN;
    UTV_BYTE * pSectionData = NULL;
    UTV_UINT32 iWaitVct = UTV_WAIT_FOR_VCT;

    /* Scanning Virtual Channel Table */
    UtvMessage( UTV_LEVEL_INFO, " scanning for VCT - frequency %d, table id 0x%X for %d ms", UtvPlatformDataBroadcastGetTunerFrequency(), iTableId, iWaitVct );

    /* Set up the section filter for the table for CVCT or TVCT */
    rStatus = UtvOpenSectionFilter( UTV_PID_PSIP, iTableId, 0, UTV_FALSE );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Get the data for the VCT, there's only one */
    rStatus = UtvGetSectionData( &iWaitVct, &pSectionData );
    if ( rStatus == UTV_OK )
    {
        UtvMessage( UTV_LEVEL_INFO, "  found VCT on table id 0x%X", iTableId );
        /* We got VCT data, try parsing it now */
        rStatus = UtvParseVct( iTableId, pSectionData );
    }
    else
    {
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " no VCT found: %s on table id 0x%X", UtvGetMessageString( rStatus ), iTableId );
    }

    UtvCloseSectionFilter();

    /* return specific type of timeout */
    if ( rStatus == UTV_TIMEOUT )
        rStatus = UTV_VCT_TIMEOUT;

    /* Return OK if one found, or failure if none found */
    return rStatus;
}

/* UtvParseVct
    Parses VCT section data looking for virtual channels with service type of software download data service
*/
UTV_RESULT UtvParseVct( UTV_TID iTableId, UTV_BYTE * pSectionData )
{
    UTV_RESULT rStatus = UTV_NO_DOWLOAD_SERVICE;
    UTV_BYTE * pData = pSectionData;
    UTV_UINT32 iSize = 0;
    UTV_UINT32 iSectionTableId = UtvStartSectionPointer( &pData, &iSize );
    UTV_UINT32 iIndex;
    UTV_UINT32 iNumChannelsInSection;
    UTV_UINT32 iProgramNumber;
    UTV_UINT32 iServiceType;

    if ( iTableId != iSectionTableId )
        return UTV_BAD_VCT_TID;

    /* Skip transport stream id, version, section number, last section number */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1, &pData, &iSize );

    /* Check the protocol version */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0 )
        return UTV_BAD_VCT_PRV;

    /* Check the number of channels in the section */
    iNumChannelsInSection = UtvGetUint8Pointer( &pData, &iSize );
    if ( iNumChannelsInSection == 0 )
        return UTV_BAD_VCT_CIS;

    /* Loop through the channel data, looking for the right service type */
    for ( iIndex = 0; iIndex < iNumChannelsInSection; iIndex++ )
    {
        /* Skip over channel name, channel number, modulation mode, carrier frequency, TSID */
        UtvSkipBytesPointer( 14 + 3 + 1 + 4 + 2, &pData, &iSize );

        /* Get the program number */
        iProgramNumber = UtvGetUint16Pointer( &pData, &iSize );

        /* Get the service type */
        iServiceType = UtvGetUint16Pointer( &pData, &iSize ) & 0x3f;

        /* If the program number is 0 (inactive) or 0xFFFF (analog) then reject this channel. */
        if ( iProgramNumber != 0 && iProgramNumber != 0xFFFF )
        {
            /* Is this the service type we are looking for? */
            if ( iServiceType == UTV_SERVICE_TYPE_DOWNLOAD || iServiceType == UTV_SERVICE_TYPE_DATA )
            {
                /* Yes, add this one and remember his program number */
                UtvCarouselCandidate * pUtvCarouselCandidate = UtvNewCarouselCandidate();
                if ( pUtvCarouselCandidate == ( UtvCarouselCandidate * ) -1 )
                    return UTV_NO_MEMORY;

                pUtvCarouselCandidate->iFrequency = UtvPlatformDataBroadcastGetTunerFrequency();
                pUtvCarouselCandidate->iProgramNumber = iProgramNumber;
                pUtvCarouselCandidate->bDiscoveredInVCT = UTV_TRUE;
                rStatus = UTV_OK;
            }
        }

        /* Skip by the source id */
        UtvSkipBytesPointer( 2, &pData, &iSize );

        /* Load the length of the descriptors for this virtual channel and skip by them */
        UtvSkipBytesPointer( UtvGetUint16Pointer( &pData, &iSize ) & 0x3ff, &pData, &iSize );
    }

    /* Return the status of the parse back to the caller */
    return rStatus;
}

/* UtvScanPat
    Scan the PAT and find the PMT PID for any download service program_number in CarouselCandidate
   Returns OK if PMT PID found
*/
UTV_RESULT UtvScanPat()
{
    UTV_RESULT rStatus = UTV_UNKNOWN;
    UTV_BYTE * pSectionData = NULL;
    UTV_UINT32 iWaitPat = UTV_WAIT_FOR_PAT;

    /* Scanning DSI */
    UtvMessage( UTV_LEVEL_INFO, " scanning for PAT on PID 0x0 for %d ms", iWaitPat );

    /* Set up the section filter for the Carousel PID and the table for DSM-CC userNetworkMessage */
    rStatus = UtvOpenSectionFilter( UTV_PID_PAT, UTV_TID_PAT, 0, UTV_FALSE );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Get the data for the PAT, there's only one per Carousel PID */
    rStatus = UtvGetSectionData( &iWaitPat, &pSectionData );
    if ( rStatus == UTV_OK )
    {
        /* We got section data, try parsing it now */
        rStatus = UtvParsePat( pSectionData );
        if ( rStatus == UTV_OK )
            UtvMessage( UTV_LEVEL_INFO, "  found PAT on PID 0x0" );
        else
            UtvMessage( UTV_LEVEL_ERROR, " PAT found with errors", UtvGetMessageString( rStatus ) );

    } else
        UtvMessage( UTV_LEVEL_WARN | UTV_LEVEL_TEST, " no PAT found on PID 0x0: %s", UtvGetMessageString( rStatus ) );

    /* Return OK if one found, or failure if none found */
    UtvCloseSectionFilter();

    /* return specific type of timeout */
    if ( rStatus == UTV_TIMEOUT )
        rStatus = UTV_PAT_TIMEOUT;

    /* Return status to caller */
    return rStatus;
}

/* UtvParsePat
    Parses the PAT
   Creates candidate entries for each program in the PAT unless one already
   exists from the VCT scan done earlier. If an enrty already exists its PMT
   PID is recorded.  All valid candidates should now have PMT PIDs.
*/
UTV_RESULT UtvParsePat( UTV_BYTE * pSectionData )
{
    UTV_RESULT              rStatus = UTV_NO_DOWNLOAD;
    UtvCarouselCandidate  * pUtvCarouselCandidate;
    UTV_BYTE              * pData = pSectionData;
    UTV_UINT32              iSize = 0;
    UTV_UINT32                 iIndex;
    UTV_UINT32                 iProgramNumber;
    UTV_UINT32                 iProgramMapTablePid;
    UTV_UINT32                 iChannelCount;

    if ( UTV_TID_PAT != UtvStartSectionPointer( &pData, &iSize ) )
        return UTV_BAD_PAT_TID;

    /* Skip transport stream id, version, section number, last section number */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1, &pData, &iSize );

    /* Determine count of channels, four bytes per channel */
    iChannelCount = ( iSize - 4 ) / 4;

    /* For each channel in the PAT... */
    for ( iIndex = 0; iIndex < iChannelCount; iIndex++ )
    {
        iProgramNumber = UtvGetUint16Pointer( &pData, &iSize );
        iProgramMapTablePid = UtvGetUint16Pointer( &pData, &iSize ) & 0x1fff;

        /* Does a candidate already exist for this program number? */
        if ( NULL != ( pUtvCarouselCandidate = UtvGetCarouselCandidateFromPNum( iProgramNumber )))
        {
            /* yes, then provide this candidate with its PMT PID */
            pUtvCarouselCandidate->iPmtPid = iProgramMapTablePid;
            rStatus = UTV_OK;
        } else
        {
            /* no, we need to create a new candidate then. */
            if ( NULL == ( pUtvCarouselCandidate = UtvNewCarouselCandidate()))
                return UTV_NO_MEMORY;

            pUtvCarouselCandidate->iFrequency = UtvPlatformDataBroadcastGetTunerFrequency();
            pUtvCarouselCandidate->iProgramNumber = iProgramNumber;
            pUtvCarouselCandidate->iPmtPid = iProgramMapTablePid;
            rStatus = UTV_OK;
        }
    }

    /* Return OK if we found at least one PMT PID */
    return rStatus;
}

/* UtvScanPmt
    Scan the PMT for this program
    If the program_number matches, and the stream_type is 0x0B, store the elementary PID
   Returns OK if Carousel PID found
*/
UTV_RESULT UtvScanPmt( UtvCarouselCandidate * pUtvCarouselCandidate )
{
    UTV_RESULT rStatus = UTV_UNKNOWN;
    UTV_BYTE * pSectionData = NULL;
    UTV_UINT32 iWaitPmt = UTV_WAIT_FOR_PMT;

    /* Scanning PMT */
    UtvMessage( UTV_LEVEL_INFO, " scanning for PMT on PID 0x%X for %d ms", pUtvCarouselCandidate->iPmtPid, iWaitPmt );

    /* Set up the section filter for the Carousel PID and the table for DSM-CC userNetworkMessage */
    rStatus = UtvOpenSectionFilter( pUtvCarouselCandidate->iPmtPid, UTV_TID_PMT, 0, UTV_FALSE );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Get the data for the PMT, there's only one copy per PMT PID */
    rStatus = UtvGetSectionData( &iWaitPmt, &pSectionData );
    if ( rStatus == UTV_OK )
    {
        /* We got section data, try parsing it now */
        rStatus = UtvParsePmt( pUtvCarouselCandidate, pSectionData );

        if ( rStatus != UTV_OK )
            UtvMessage( UTV_LEVEL_WARN | UTV_LEVEL_TEST, " PMT does not contain appropriate PSIP data: %s", UtvGetMessageString( rStatus ) );

    } else
    {
        /* no PMT found */
        UtvMessage( UTV_LEVEL_WARN | UTV_LEVEL_TEST, " no PMT found: %s on PID 0x%X", UtvGetMessageString( rStatus ),
                    pUtvCarouselCandidate->iPmtPid );
    }

    /* All done with section filter. */
    UtvCloseSectionFilter();

    /* return specific type of timeout */
    if ( rStatus == UTV_TIMEOUT )
        rStatus = UTV_PMT_TIMEOUT;

    return rStatus;
}

/* UtvParsePmt
    Parse the section data capured for a PMT for a particular candidate
    We have the program number of a download service, we need the elementary PID
   Returns OK if carousel pid found
*/
UTV_RESULT UtvParsePmt( UtvCarouselCandidate * pUtvCarouselCandidate, UTV_BYTE * pSectionData )
{
    UTV_INT32       iCount;
    UTV_RESULT      rStatus = UTV_NO_ULI_DATA;
    UTV_BYTE        szRegistration[ UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_LENGTH + 1 ] = { 0 };
    UTV_BOOL        bBDCStream = UTV_FALSE;
    UTV_BYTE      * pData = pSectionData;
    UTV_UINT32      iSize = 0;
    UTV_UINT8         iStreamType;
    UTV_UINT16         iElementaryPid;
    UTV_UINT32         iEsInfoLength;
    UTV_BYTE      * pDescriptorData;
    UTV_UINT32         iDescriptorBytes;
    UTV_INT32         iDescriptorTag;
    UTV_INT32         iDescriptorLength;

    if ( UTV_TID_PMT != UtvStartSectionPointer( &pData, &iSize ) )
        return UTV_BAD_PMT_TID;

    /* Skip table id ext, version, section number, last section number, PCR PID */
    UtvSkipBytesPointer( 2 + 1 + 1 + 1 + 2, &pData, &iSize );

    /* Skip by the program info */
    UtvSkipBytesPointer( UtvGetUint16Pointer( &pData, &iSize ) & 0xfff, &pData, &iSize );

    /* Loop through the all the program elements looking for the right stream type */
    while ( iSize > 4 )
    {
        /* Get stream type (8 bits) and elementary PID (13 bits) */
        iStreamType = UtvGetUint8Pointer( &pData, &iSize );
        iElementaryPid = UtvGetUint16Pointer( &pData, &iSize ) & 0x1fff;

        /* the esinfo data is needed if we have to parse the descriptors */
        iEsInfoLength = UtvGetUint16Pointer( &pData, &iSize ) & 0xfff;

        /* Only elemetary streams with a type of 0xB (A/90 Async. Data Download Protocol) are interesting */
        if ( UTV_STREAM_TYPE_DOWNLOAD == iStreamType )
        {
            bBDCStream = UTV_FALSE;
            pDescriptorData = pData;
            iDescriptorBytes = iEsInfoLength;

            while ( iDescriptorBytes )
            {
                /* Get the tag and length */
                iDescriptorTag = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorBytes );
                iDescriptorLength = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorBytes );

                /* If it was the registration descriptor and it is UTV_ULI_PMT_REG_DESC_TOT_LEN bytes long save it now */
                if ( UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_TAG == iDescriptorTag && UTV_ULI_PMT_REG_DESC_TOT_LEN == iDescriptorLength)
                {
                    for ( iCount = 0; iCount < iDescriptorLength && (UTV_UINT32)iCount < sizeof( szRegistration ); iCount++ )
                        szRegistration[ iCount ] = UtvGetUint8Pointer( &pDescriptorData, &iDescriptorBytes );

                    szRegistration[ iCount ] = 0;

                    /* check for base comparison then range */
                    if ( UtvMemcmp( szRegistration, (void *) UTV_ULI_PMT_REG_DESC_BASE, UTV_ULI_PMT_REG_DESC_BASE_LEN ) == 0 )
                    {
                        if ( szRegistration[ UTV_ULI_PMT_REG_DESC_BASE_LEN ] >= UTV_ULI_PMT_REG_DESC_ORG &&
                             szRegistration[ UTV_ULI_PMT_REG_DESC_BASE_LEN ] <= UTV_ULI_PMT_REG_DESC_END )
                        {    
                            bBDCStream = UTV_TRUE;
                            break;
                        }
                    }
                }
                else
                {
                    /* skip over any remaining bytes in this descriptor */
                    UtvSkipBytesPointer( iDescriptorLength, &pDescriptorData, &iDescriptorBytes );
                }
            } /* look through all the descriptors */

            /* If this candidate is OK, use the current ES as the carousel PID and stop searching */
            if ( bBDCStream )
            {
                pUtvCarouselCandidate->iCarouselPid = iElementaryPid;
                if ( pUtvCarouselCandidate->bDiscoveredInVCT )
                    UtvMessage( UTV_LEVEL_INFO, "  found PMT indicating carousel on PID 0x%X via VCT", pUtvCarouselCandidate->iCarouselPid );
                else
                    UtvMessage( UTV_LEVEL_INFO, "  found PMT indicating carousel on PID 0x%X", pUtvCarouselCandidate->iCarouselPid );
                return UTV_OK;
            }

        } /* if stream type is 0xB */

        /* Skip over ES info */
        UtvSkipBytesPointer( iEsInfoLength, &pData, &iSize );

    } /* while data in the PMT */

    /* Bubble up status to caller */
    return rStatus;
}

#ifdef UTV_INTERACTIVE_MODE
/* UtvTunedChannelHasDownloadDataWorker
     Called to see if a tuned channel has any ULI data on it.
     Worker function for UtvTunedChannelHasDownloadData() which is declared in 
     common-utility.c.
    Returns UTV_OK if data present, UTV_NO_DOWNLOAD otherwise.  All results returned
    via g_sAsyncResultObject
*/
void UtvTunedChannelHasDownloadDataWorker ( )
{
    UTV_RESULT             rStatus;
    UTV_PUBLIC_RESULT_OBJECT  *pResult;
    UTV_PUBLIC_CALLBACK        pCallback;

    /* Log this */
    UtvMessage( UTV_LEVEL_INFO, "Looking for ULI data on currently tuned channel" );

    /* Create section filter */
    rStatus = UtvCreateSectionFilter();
    if ( UTV_OK == ( rStatus = UtvCreateSectionFilter() ))
    {
        /* Section filter is all set, now scan this frequency */
        if ( UTV_OK == ( rStatus =  UtvScanOneFrequency( NULL )))
            UtvMessage( UTV_LEVEL_INFO, "  ULI data found!");
        else
            UtvMessage( UTV_LEVEL_INFO, "  no ULI data found: %s", UtvGetMessageString( rStatus ) );
    }

    /* Shut down the section filter */
    UtvDestroySectionFilter();

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

void *UtvTunedChannelHasDownloadDataWorkerAsThread( void *placeholder )
{
    UtvTunedChannelHasDownloadDataWorker();    
    UTV_THREAD_EXIT;
}

#endif
