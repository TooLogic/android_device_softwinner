/* platform-data-broadcast-static.c: UpdateTV Static Data Source Platform Adaptation Layer
                                     Emulates a tuner by generating static packets.

   Copyright (c) 2007 - 2008 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Platform Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformDataBroadcastAcquireHardware( UTV_UINT32 iEventType )
   UTV_RESULT UtvPlatformDataBroadcastReleaseHardware( void )
   UTV_RESULT UtvPlatformDataBroadcastSetTunerState( UTV_UINT32 iFrequency )
   UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( void )
   UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pFunc )
   UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( UTV_UINT32 iPid, UTV_UINT32 iTableId, 
                                        UTV_UINT32 iTableIdExt, UTV_BOOL bEnableTableIdExt )
   UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( void )
   UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( void )
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Seb Emonet      05/12/2010  Changed the UTV_STATUS_ABORT define to UTV_ABORT
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Bob Taylor      04/11/2007  Original.
*/

#include "utv.h"

/* Proto to access UtvGetFakeSectionData which lives in static-section.c
*/
UTV_BYTE * UtvGetFakeSectionData( UTV_UINT32 iPid, UTV_UINT32 iTableId );

/* Local storage
*/
typedef struct
{
    /* This data is private to this module, put in a struct to keep it all together */
    UTV_UINT32 iPid;                        /* MPEG-2 virtual program id  */
    UTV_UINT32 iTableId;                    /* MPEG-2 private section table id */
    UTV_UINT32 iTableIdExt;                 /* MPEG-2 private section table id extension */
    UTV_UINT32 iPacketsPerSecond;           /* Simulated packets per second */
    UTV_BOOL bEnableTableIdExt;             /* true if filtering on table id extension */
    UTV_THREAD_HANDLE hThread;              /* Handle for thread */
    UTV_BOOL bThreadRunning;                /* true when thread is running */
    UTV_BOOL bSectionFilterOpen;            /* true when thread is supposed to be running */
} UtvPersSectionFilterStatus;

UtvPersSectionFilterStatus gPersSectionFilterStatus = { 0 };

/* Public Tuner Interface Functions Required By the UpdateTV Core follow...
*/

/* UtvGetTunerFrequencyList
   For TS_FILE only a aingle fake test frequency is returned.
*/
static UTV_UINT32  s_uiFreqList[] = { 0 };
UTV_RESULT UtvGetTunerFrequencyList (
    UTV_UINT32 * piFrequencyListSize,
    UTV_UINT32 * * ppFrequencyList )
{
    *piFrequencyListSize = sizeof( s_uiFreqList ) / sizeof ( UTV_UINT32 );
    *ppFrequencyList = s_uiFreqList;

    return UTV_OK;
}

/* UtvPlatformDataBroadcastAcquireHardware
    This call checks to see if the TV hardware is idle (consumer is not watching TV) and
    then reserves the hardware for UpdateTV use. The hardware resources needed include
     - Tuner
     - Hardware section filters
     - Module download memory
   Returns true if TV is idle, false if TV is busy
*/
UTV_RESULT UtvPlatformDataBroadcastAcquireHardware( UTV_UINT32 iEventType )
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastReleaseHardware
    This call releases the hardware reserved by the UtvAcquireHardware function
   Returns true if resources released, false if resources were not acquired
*/
UTV_RESULT UtvPlatformDataBroadcastReleaseHardware()
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastSetTunerState
    This call sets the tuner state and frequency.
    Cycles through modulation types when lock fails attempting what worked
    last time first and then the other two:  VSB, QAM_64, QAM_256.
   Returns OK if tuner set
*/
UTV_RESULT UtvPlatformDataBroadcastSetTunerState( 
    UTV_UINT32 iFrequency )
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastGetTunerFrequency
    This call sets the tuner frequency, state is ignored.
   Returns frequency or 0 if unknown
*/
UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( )
{
    return 0;
}

#ifdef UTV_OS_SINGLE_THREAD

/* SINGLE-THREADED PUMP EXAMPLE */

/* UtvStaticPump
    This function is called directly by the core in UtvGetSectionData when UTV_OS_SINGLE_THREAD
    is defined.
    Thread-free delivery of section data from static data.
    Note timer value is never updated because there is ALWAYS a
    section available and delivered each time this function is called.
*/
UTV_RESULT UtvStaticPump( UTV_UINT32 * piTimer )
{
    UTV_RESULT rStatus;
    UTV_BYTE * pSectionData;

    /* If section filter not running just return right now thank you very much */
    if ( ! gPersSectionFilterStatus.bSectionFilterOpen )
        return UTV_BAD_SECTION_FILTER_NOT_RUNNING;

    /* If we got some fake data, deliver it now */
    if ( NULL != ( pSectionData = UtvGetFakeSectionData( gPersSectionFilterStatus.iPid, gPersSectionFilterStatus.iTableId )))
    {
        /* Get the size of the fake data */
        UTV_BYTE * pData = pSectionData;
        UTV_UINT32 iSize = 0;
        UtvStartSectionPointer( &pData, &iSize );
        
        /* Deliver it to the other thread */
        /* Data will be thrown away if the other thread isn't ready for it yet */
        rStatus = UtvDeliverSectionFilterData( iSize + 3, pSectionData );

        /* Dump the memory we allocated */
        UtvFree( pSectionData );
    }
    else
    {
        /* if we couldn't get a fake packet - we're out of memory */
        rStatus = UTV_NO_MEMORY;
    }

    /* Otherwise return OK we got a section */
    return rStatus;
}

#else /* UTV_OS_SINGLE_THREAD */

/* MULTI-THREADED PUMP EXAMPLE
*/

/* UtvStaticPumpThread
    Demonstration thread that delivers made-up data for testing 
    UtvStaticPumpThread is created from UtvPersOpenSectionFilter() and destroyed by 
    UtvPersCloseSectionFilter() when UTV_OS_SINGLE_THREAD is NOT defined (default).
*/
UTV_THREAD_ROUTINE UtvStaticPumpThread( void * pArg )
{
    /* Remember thread is running */
    gPersSectionFilterStatus.bThreadRunning = UTV_TRUE;

    /* Data pump to other thread */
    while ( gPersSectionFilterStatus.bSectionFilterOpen )
    {
        /* Get a pointer to some private data */
        UTV_BYTE * pSectionData = UtvGetFakeSectionData( gPersSectionFilterStatus.iPid, gPersSectionFilterStatus.iTableId );
        /* If we got some fake data, deliver it now */
        if ( pSectionData != NULL )
        {
            /* Get the size of the fake data */
            UTV_BYTE * pData = pSectionData;
            UTV_UINT32 iSize = 0;
            UtvStartSectionPointer( &pData, &iSize );
            
            /* Deliver it to the other thread */
            /* Data will be thrown away if the other thread isn't ready for it yet */
            UtvDeliverSectionFilterData( iSize + 3, pSectionData );
            
            /* Dump the memory we allocated */
            UtvFree( pSectionData );
        }

        /* just sleep a little in between sending sections. */
        /* this keeps the section queue from overflowing */
        UtvSleep( 100 );
        
        /* Was there an abort? */
        if ( UtvGetState() == UTV_ABORT )
            break;

    }

    /* When we get here we need to terminate this thread now, just return to exit */
    gPersSectionFilterStatus.bThreadRunning = UTV_FALSE;

    /* perform os-specific exit of thread. */
    UTV_THREAD_EXIT;
}

#endif /* UTV_OS_STATIC_THREAD */

/* UtvPlatformDataBroadcastCreateSectionFilter
    Called after Mutex and Wait handles established with state set to STOPPED
    First step in using a section filter
    Sets up the section filter context
    Next step is to open the section filter
   Returns OK if the section filter has been created ok
*/
UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pPumpFunc )
{
    /*  This code should make the section filter ready but not start it delivering data */

    /* If this is the single threaded version of this data source then */
    /* return a pointer to the pump function.  Else return NULL. */
#ifdef UTV_OS_SINGLE_THREAD
    *pPumpFunc = UtvStaticPump;
#else
    *pPumpFunc = NULL;    
#endif

    /* Return status, always good for stub */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastOpenSectionFilter
    Called here from UtvPersOpenSectionFilter to start the section filter with the state set to READY
    Next step is to get section data
   Returns UTV_OK if section filter opened and ready
*/
UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( 
    IN UTV_UINT32 iPid, 
    IN UTV_UINT32 iTableId, 
    IN UTV_UINT32 iTableIdExt, 
    IN UTV_BOOL bEnableTableIdExt )
{
    /* Start data delivery even though main thread may not be ready for data yet */

    /* For demonstration we show that we need to copy the data for local use */
    if ( gPersSectionFilterStatus.bSectionFilterOpen == UTV_TRUE )
        return UTV_NO_SECTION_FILTER;
    gPersSectionFilterStatus.iPid = iPid;
    gPersSectionFilterStatus.iTableId = iTableId;
    gPersSectionFilterStatus.iTableIdExt = iTableIdExt;
    gPersSectionFilterStatus.bEnableTableIdExt = bEnableTableIdExt;
    gPersSectionFilterStatus.bSectionFilterOpen = UTV_TRUE;

#ifndef UTV_OS_SINGLE_THREAD
    /* Start up the pump thread that delivers fake section data */
    UtvThreadCreate( &gPersSectionFilterStatus.hThread, (UTV_THREAD_START_ROUTINE)&UtvStaticPumpThread, (void *)&gPersSectionFilterStatus );
#endif /* UTV_OS_SINGLE_THREAD */

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCloseSectionFilter
    Called to stop delivery of section filter data with mutex locked and start changed to STOPPED
    Next step is to destroy the section filter or be opened on another PID/TID
   Returns OK if everything closed normally
*/
UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( )
{
    /*  This code should leave resources available for the next section filter open operation */
    UTV_RESULT rStatus = UTV_OK;
#ifndef UTV_OS_SINGLE_THREAD
    UTV_UINT32 iCount;
#endif

    /* Closed now */
    gPersSectionFilterStatus.bSectionFilterOpen = UTV_FALSE;

#ifndef UTV_OS_SINGLE_THREAD

    /* Wait for the thread to die off */
    for ( iCount = 0; iCount < 30; iCount++ )
    {
        /* Sleep for a bit to give the thread time to exit */
        UtvSleep( 100 );

        /* something interrupted the wait - check for abort */
        if ( UtvGetState() == UTV_ABORT )
        {
            rStatus = UTV_ABORT;
            break;
        }

        /* If the other thread is gone then go ahead, otherwise wait a bit longer */
        if ( gPersSectionFilterStatus.bThreadRunning == UTV_FALSE ) 
            break;
    }

    /* If abort signaled, get out now */
    if ( rStatus == UTV_ABORT )
        return rStatus;
    
    if ( gPersSectionFilterStatus.bThreadRunning == UTV_TRUE )
        rStatus = UTV_BAD_THREAD;

    /* Ask for its destruction just to make sure, nulls the handle */
    UtvThreadDestroy( &gPersSectionFilterStatus.hThread );

#endif /* UTV_OS_SINGLE_THREAD */

    /* Return status */
    return rStatus;
}

/* UtvPlatformDataBroadcastDestroySectionFilter
    Destroy the context created by the UtvOpenSectionFilter call
    Called with mutex locked and state changed to UNKNOWN
    Any hardware context is released
    All memory allocated by the given section filter is freed
   Returns OK if everything done normally
*/
UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( )
{
    /* Release all resources associated with the section filter */

    /* Return status */
    return UTV_OK;
}

UTV_RESULT UtvPlatformDataBroadcastGetTuneInfo( UTV_UINT32 *puiPhysicalFrequency, UTV_UINT32 *puiModulationType )
{
    *puiPhysicalFrequency = 0;
    *puiModulationType = 0;
    return UTV_OK;
}
