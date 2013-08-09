/* coresectionfilter.c: UpdateTV Common Core - Section Filter

  This module contains the UpdateTV Common Core Section Filter interface routines. 
  These routines call the UpdateTV Personality Map routines for each operation
  supported by the API.

  In order to ensure UpdateTV network compatibility, customers must not 
  change UpdateTV Common Core code.

  Copyright (c) 2004 - 2009 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      02/02/2008  Created from 1.9.21 for version 2.0.0.
*/

#include <string.h> /* memset() */
#include "utv.h"

UTV_RESULT UtvDeliverSectionFilterDataWork( UTV_UINT32 iSectionDataLength, UTV_BYTE * pSectionData );
UTV_RESULT UtvVirtualStopSectionFilter( );

/* UtvSectionFilterStatus
    Maintain state and interface to UpdateTV code.
    This struct is private to this module. 
    Platform implementations may add data to this struct, but it is suggested you keep the BDC section together
    Add interface routines if you need to expose parts of this data to other modules.
*/
typedef struct
{
    UTV_SECTION_FILTER_STATUS eStatus;      /* Filter status */
    UTV_UINT32 iPid;                        /* MPEG-2 virtual program id  */
    UTV_UINT32 iTableId;                    /* MPEG-2 private section table id */
    UTV_UINT32 iTableIdExt;                 /* MPEG-2 private section table id extension */
    UTV_BOOL bEnableTableIdExt;             /* true if filtering on table id extension */
    UTV_BYTE * pSectionQueue;               /* Pointer to current incoming raw section data */
    UTV_UINT32 uiSectionQueueInput;         /* Section queue input index */
    UTV_UINT32 uiSectionQueueOutput;        /* Section queue output index */
    UTV_UINT32 uiSectionQueueCount;         /* Section queue contents count */
    UTV_UINT32 uiSectionQueueOverflows;     /* Section queue overflow count */
    UTV_UINT32 uiSectionQueueHighWater;     /* Section queue high water mark */
    UTV_WAIT_HANDLE hWait;                  /* Handle for event wait mutex */
    UTV_SINGLE_THREAD_PUMP_FUNC pfSingleThreadPump;    /* entry point for single thread pump call. */
}  UtvSectionFilterStatus;

static UtvSectionFilterStatus gUtvSectionFilterStatus = { UTV_SECTION_FILTER_UNKNOWN, 0, 0, 0, UTV_FALSE, NULL, 0, 0, 0, 0, 0, 0, NULL };

/* UtvSectionQueueInit
    Needs to be done once on initialization by UtvAgenInit().
    Allocates memory and initializes section queue mamangement structure
    REQUIRES: gUtvSectionFilterStatus.pSectionQueue to be NULL on startup.
   Returns OK if allocation worked else UTV_NO_MEMORY
*/
UTV_RESULT UtvSectionQueueInit( )
{
    /* allocate the section queue if it isn't already allocated */
    if ( NULL == gUtvSectionFilterStatus.pSectionQueue )
    {
        if (NULL == (gUtvSectionFilterStatus.pSectionQueue = (UTV_BYTE *) UtvMalloc( UTV_MPEG_MAX_SECTION_SIZE * UTV_SECTION_QUEUE_DEPTH )))
          return UTV_NO_MEMORY;
    }

    /* initialize the section queue machinery */
    gUtvSectionFilterStatus.uiSectionQueueInput = 0;
    gUtvSectionFilterStatus.uiSectionQueueOutput = 0;
    gUtvSectionFilterStatus.uiSectionQueueCount = 0;
    gUtvSectionFilterStatus.uiSectionQueueOverflows = 0;
    gUtvSectionFilterStatus.uiSectionQueueHighWater = 0;

    return UTV_OK;
}

/* UtvClearSectionQueue
    Needs to be done once on system shutdown by UtvPublicAgentShutdown().
    Deallocates section queue memory if it has been allocated.
   Returns OK if everything was freed up OK.
*/
UTV_RESULT UtvClearSectionQueue( )
{
    /* de-allocate the section queue if it's already been allocated. */
    if ( NULL != gUtvSectionFilterStatus.pSectionQueue )
    {
        UtvFree( gUtvSectionFilterStatus.pSectionQueue );
        gUtvSectionFilterStatus.pSectionQueue = NULL;
    }

    return UTV_OK;
}

/* UtvCreateSectionFilter
    First step in using a section filter
    Sets up the section filter context
   Returns OK if the section filter has been created ok
*/
UTV_RESULT UtvCreateSectionFilter( )
{
    UTV_SINGLE_THREAD_PUMP_FUNC    pPumpFunc;
    UTV_RESULT rStatus;

    /* Set up the state */
    gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_STOPPED;
    
    /* Call personality map to finish the creation */
    /* Get a pointer to the single thread pump function if there is one. */
    rStatus = UtvPlatformDataBroadcastCreateSectionFilter( &pPumpFunc );
    if ( rStatus != UTV_OK )
    {
        UtvDestroySectionFilter();
        return rStatus;
    }

    /* all sections sitting in the queue are thrown out  */
    /* whenever we create a new section filter */
    gUtvSectionFilterStatus.uiSectionQueueInput = 0;
    gUtvSectionFilterStatus.uiSectionQueueOutput = 0;
    gUtvSectionFilterStatus.uiSectionQueueCount = 0;
    gUtvSectionFilterStatus.pfSingleThreadPump = pPumpFunc;

    /* Return goodness */
    return UTV_OK;
}

/* UtvOpenSectionFilter
    Starts the section filter running
*/
UTV_RESULT UtvOpenSectionFilter( 
    IN UTV_UINT32 iPid, 
    IN UTV_UINT32 iTableId, 
    IN UTV_UINT32 iTableIdExt, 
    IN UTV_BOOL bEnableTableIdExt )
{
    UTV_RESULT rStatus = UTV_OK;

    /* Make sure we are in the right state */
    if ( gUtvSectionFilterStatus.eStatus != UTV_SECTION_FILTER_STOPPED ) 
    {
        return UTV_BAD_SECTION_FILTER_NOT_STOPPED;
    }

    /* any leftover sections sitting in the queue are thrown out  */
    /* whenever we open a new section filter */
    gUtvSectionFilterStatus.uiSectionQueueInput = 0;
    gUtvSectionFilterStatus.uiSectionQueueOutput = 0;
    gUtvSectionFilterStatus.uiSectionQueueCount = 0;

    /* Set the status to RUNNING, store configuration data that may be used later */
    gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_RUNNING;
    gUtvSectionFilterStatus.iPid = iPid;
    gUtvSectionFilterStatus.iTableId = iTableId;
    gUtvSectionFilterStatus.iTableIdExt = iTableIdExt;
    gUtvSectionFilterStatus.bEnableTableIdExt = bEnableTableIdExt;

    /* Set up the blocking handle. */
    /* Needs to be done with the initialization of each  */
    /* new section to eliminate signalled sections that haven't */
    /* yet been picked up from the last section. */
    rStatus = UtvWaitInit( &gUtvSectionFilterStatus.hWait );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Call the personality module to do the heavy lifting to open the section filter */
    rStatus = UtvPlatformDataBroadcastOpenSectionFilter( iPid, iTableId, iTableIdExt, bEnableTableIdExt );

    /* if the open doesn't succeed, return the filter to the STOPPED state   */
    if ( UTV_OK != rStatus ) 
    {
        gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_STOPPED;
        /* Dump the wait handle */
        UtvWaitDestroy( gUtvSectionFilterStatus.hWait );
        gUtvSectionFilterStatus.hWait = 0;
    }
    
    /* Return to caller */
    return rStatus;
}

/* UtvDeliverSectionFilterDataWork
    Worker routine for UtvDeliverSectionFilterData
    Called with section filter data LOCKED
   Returns OK if data was delivered or error code if not delivered
*/
UTV_RESULT UtvDeliverSectionFilterDataWork( 
    UTV_UINT32 iSectionDataLength, 
    UTV_BYTE * pSectionData )
{
#ifdef UTV_CORE_PACKET_FILTER
    UTV_RESULT rCrc;
#endif

    /* Called back with data to deliver, make sure we're expecting it */
    if ( gUtvSectionFilterStatus.eStatus != UTV_SECTION_FILTER_RUNNING )
    {
        /* We are not ready for this data */
        /* This is expected to happen if the main thread is not inside a UtvGetSectionData */
        return UTV_BAD_SECTION_FILTER_NOT_RUNNING;
    }

    /* Find some space in the section queue for this data. */
    if ( gUtvSectionFilterStatus.uiSectionQueueCount >= UTV_SECTION_QUEUE_DEPTH )
    {
        /* no room in the queue, record the overflow error. */
        gUtvSectionFilterStatus.uiSectionQueueOverflows++;
        return UTV_SECTION_QUEUE_OVERFLOW;
    }

    /* Check the incoming pointer */
    if ( pSectionData == NULL )
    {
        /* Called with null data pointer */
        return UTV_BAD_SECTION_FILTER_NULL_DATA;
    }

    /* Check the incoming data length */
    if ( iSectionDataLength == 0 )
    {
        /* Called with zero length section */
        return UTV_BAD_SECTION_FILTER_ZERO_DATA;
    }

    /* Make sure delivered packet's table id matches the table id we are looking for */
    if ( 0xFFFFFFFF != gUtvSectionFilterStatus.iTableId && pSectionData[ 0 ] != gUtvSectionFilterStatus.iTableId )
    {
        /* Table id wasn't the right one */
        UtvMessage( UTV_LEVEL_TRACE, "    dumping section with incorrect TID 0x%X should have been TID 0x%X", pSectionData[ 0 ], gUtvSectionFilterStatus.iTableId );
        return UTV_BAD_SECTION_FILTER_DELIVER_TID_MATCH;
    }

#ifdef UTV_CORE_PACKET_FILTER
    /* Do CRC32 check on the data in the software */
    rCrc = UtvVerifySectionData( pSectionData, iSectionDataLength );
    if ( UTV_OK != rCrc )
    {
        /* Checksum error dump the data now */
        UtvMessage( UTV_LEVEL_TRACE, "    dumping section with incorrect CRC32" );
        return rCrc;
    }
#endif /* UTV_CORE_PACKET_FILTER */

    /* N.B.  This is the only place where data is written into the section queue so the input index doesn't */
    /* need to be mutex protected. */

    /* The incoming pointer to the data is memory owned by the data delivery driver (our caller) */
    /* We need to store it for processing in the section queue where it can be picked up  */
    /* by the core. */
    memcpy( &gUtvSectionFilterStatus.pSectionQueue[gUtvSectionFilterStatus.uiSectionQueueInput*UTV_MPEG_MAX_SECTION_SIZE], 
            pSectionData, iSectionDataLength );

    /* update input index */
    if ( ++gUtvSectionFilterStatus.uiSectionQueueInput >= UTV_SECTION_QUEUE_DEPTH )
        gUtvSectionFilterStatus.uiSectionQueueInput = 0;

    /* Bump section queue contents count.  This NEEDS to be the last thing done so that the reader is only  */
    /* woken up when all the data is available. */
    if ( ++gUtvSectionFilterStatus.uiSectionQueueCount > gUtvSectionFilterStatus.uiSectionQueueHighWater )
        gUtvSectionFilterStatus.uiSectionQueueHighWater = gUtvSectionFilterStatus.uiSectionQueueCount;

    /* Notify the core thread that a section is available to process. */
    UtvWaitSignal( gUtvSectionFilterStatus.hWait );

    /* Indicate we delivered one section */
    return UTV_OK;
}

/* UtvDeliverSectionFilterData
    This routine glues the section filter callback to the UpdateTV system
    Makes sure that the other thread is wanting data before sending the data
    Called from the driver thread with a section's worth of data to deliver to the other thread
    The caller may free the pSectionData after this routine returns
    CAUTION: YOU SHOULD NOT NEED TO CHANGE THIS CODE FOR PORTING
   Returns OK if data was delivered
*/
UTV_RESULT UtvDeliverSectionFilterData( 
    UTV_UINT32 iSectionDataLength, 
    UTV_BYTE * pSectionData )
{
    /* Do the work */
    UTV_RESULT rStatus = UtvDeliverSectionFilterDataWork( iSectionDataLength, pSectionData );

    /* Count as delivered or discarded */
    if ( UTV_OK == rStatus )
        g_pUtvDiagStats->iSectionsDelivered++;
    else
        g_pUtvDiagStats->iSectionsDiscarded++;
    
    /* Return the status */
    return rStatus;
}

/* UtvGetSectionData
    Called by the UpdateTV core routines to get one section's worth of data
    CAUTION: YOU SHOULD NOT NEED TO CHANGE THIS CODE FOR PORTING
   Returns OK if data found, pointer returned 
   Returns error code if data not found
*/
UTV_RESULT UtvGetSectionData( UTV_UINT32 * piTimer, UTV_BYTE * * ppSectionData )
{
    /* Keep the rolling section data status */
    UTV_RESULT rStatus = UTV_NO_DOWNLOAD;

    /* No returned data yet */
    *ppSectionData = NULL;

    /* Check abort state, get out now if aborting */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    /* Check state, make sure it is ready  */
    if ( gUtvSectionFilterStatus.eStatus != UTV_SECTION_FILTER_RUNNING ) 
    {
        /* Not ready, did you forget the open? */
        *piTimer = 0;
        return UTV_BAD_SECTION_FILTER_NOT_RUNNING;
    }

#ifdef UTV_OS_SINGLE_THREAD
    /* SINGLE-THREADED version */
    /* */
    /* If no pump function provided then we're in trouble */
    if ( NULL == gUtvSectionFilterStatus.pfSingleThreadPump )
    {
        *piTimer = 0;
        return UTV_NO_SECTION_FILTER;
    }
    
    while ( UTV_OK != rStatus && 0 != *piTimer )
    {
        /* call section acquisition function */
        rStatus = gUtvSectionFilterStatus.pfSingleThreadPump( piTimer );

        /* Check abort state again, get out now if aborting */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            return UTV_ABORT;
    }

    if ( 0 == *piTimer )
        rStatus = UTV_TIMEOUT;
#else
    /* MULTI-THREADED version */
    /* */
    /* Wait for the data to arrive via asynchronous thread */
    if ( rStatus == UTV_NO_DOWNLOAD )
        rStatus = UtvWaitTimed( gUtvSectionFilterStatus.hWait, piTimer );
#endif

    /* -- end data gathering --- */

    /* Check abort state again, get out now if aborting */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    /* If the return was OK but there is no data then set an error */
    if ( UTV_OK == rStatus && 0 == gUtvSectionFilterStatus.uiSectionQueueCount )
    {
        UtvMessage( UTV_LEVEL_WARN, "   woke up but no section data delivered" );
        rStatus = UTV_BAD_SECTION_FILTER_ZERO_RETURN;
        *ppSectionData = NULL;
    } else
    {
        /* if the return was not OK then don't return any data. */
        if ( UTV_OK != rStatus )
        {
            *ppSectionData = NULL;
        } else
        {
            /* return was OK and there is data, return the section data pointer back to the caller */
            /* N.B.  This is the only place where data is read out of the section queue so the output index doesn't */
            /* need to be mutex protected. */
            *ppSectionData = &gUtvSectionFilterStatus.pSectionQueue[gUtvSectionFilterStatus.uiSectionQueueOutput*UTV_MPEG_MAX_SECTION_SIZE];

            /* update output index */
            if ( ++gUtvSectionFilterStatus.uiSectionQueueOutput >= UTV_SECTION_QUEUE_DEPTH )
                gUtvSectionFilterStatus.uiSectionQueueOutput = 0;

            /* decrement queue count */
            gUtvSectionFilterStatus.uiSectionQueueCount--;
        }
    }

    /* Return status to caller (OK or error code) */
    return rStatus;
}

/* UtvCloseSectionFilter
    Closes the section filter so that it no longer tries to deliver data
   Returns OK if section filter closed without errors
*/
UTV_RESULT UtvCloseSectionFilter( )
{
    /* Check state, make sure it is running, change it to stopped */
    if ( gUtvSectionFilterStatus.eStatus != UTV_SECTION_FILTER_RUNNING ) 
    {
        return UTV_BAD_SECTION_FILTER_NOT_RUNNING;
    }

    gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_STOPPED; 

    /* Call into the personality map to do the work to shut down data delivery */
    UtvPlatformDataBroadcastCloseSectionFilter();

    /* Dump the wait handle */
    UtvWaitDestroy( gUtvSectionFilterStatus.hWait );
    gUtvSectionFilterStatus.hWait = 0;

    /* Return success to the caller */
    return UTV_OK;
}

/* UtvDestroySectionFilter
    Destroy the context created by the UtvOpenSectionFilter call
    Any hardware context is released
    All memory allocated by the given section filter is freed
   Returns OK if we destroyed it
*/
UTV_RESULT UtvDestroySectionFilter( )
{
    /* Check state, make sure it is stopped, set it to destroyed */
    if ( gUtvSectionFilterStatus.eStatus != UTV_SECTION_FILTER_STOPPED ) 
    {
        return UTV_BAD_SECTION_FILTER_NOT_STOPPED;
    }

    gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_UNKNOWN;
    gUtvSectionFilterStatus.pfSingleThreadPump = NULL;

    /* Call into the personality map to do the work */
    UtvPlatformDataBroadcastDestroySectionFilter();

    /* Return status */
    return UTV_OK;
}

/* UtvOverrideSectionFilterStatus
    Just sets the state of the filter to whatever the caller wants without calling the
    associated pers functions so that section data can be manually pushed into the section queue.
   Returns OK if section filter closed without errors
*/
void UtvOverrideSectionFilterStatus( UTV_SECTION_FILTER_STATUS eStatus )
{
    /* Change section filter status. */
    gUtvSectionFilterStatus.eStatus = eStatus;
}

/* UtvVirtualOpenSectionFilter
    Just sets the state of the filter to running without calling the pers
    associated pers functions to that section data can be manually pushed into the section queue.
   Returns OK if section filter closed without errors
*/
UTV_RESULT UtvVirtualStopSectionFilter( )
{
    /* Check state, make sure it is ready or running, change it to stopped */
    gUtvSectionFilterStatus.eStatus = UTV_SECTION_FILTER_RUNNING;

    /* Return success to the caller */
    return UTV_OK;
}

/* UtvGetSectionsQueueOverflowed
*/
UTV_UINT32 UtvGetSectionsOverflowed() 
{
    UTV_UINT32 uiSQOverflows = gUtvSectionFilterStatus.uiSectionQueueOverflows;
    gUtvSectionFilterStatus.uiSectionQueueOverflows = 0;
    return uiSQOverflows;
}

/* UtvGetSectionsQueueHighWater
*/
UTV_UINT32 UtvGetSectionsHighWater() 
{
    return gUtvSectionFilterStatus.uiSectionQueueHighWater;
}

/* UtvClearSectionsQueueHighWater
*/
void UtvClearSectionsHighWater() 
{
    gUtvSectionFilterStatus.uiSectionQueueHighWater = 0;
}
