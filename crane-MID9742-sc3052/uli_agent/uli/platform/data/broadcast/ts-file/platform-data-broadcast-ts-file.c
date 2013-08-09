/* platform-data-broadcast-ts-file.c: Platform broadcast data adaptation layer.

   Tuner Interface Example based on a TS File emulation.

   Copyright (c) 2007-2010 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
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
   Bob Taylor      02/09/2010  Changed hard-coded define to UtvPlatformGetTSName().
   Bob Taylor      02/02/2009  Created from 1.9.21 for version 2.0.0.
*/

#include "utv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>

/* Private data source management structure
*/
#define UTV_PERS_FILENAME_MAX 255
typedef struct
{
    UTV_UINT32 iPid;                        /* MPEG-2 virtual program id  */
    UTV_UINT32 iTableId;                    /* MPEG-2 private section table id */
    UTV_UINT32 iTableIdExt;                 /* MPEG-2 private section table id extension */
    UTV_BOOL bEnableTableIdExt;             /* true if filtering on table id extension */
    UTV_THREAD_HANDLE hThread;              /* Handle for thread */
    UTV_BOOL bThreadRunning;                /* true when thread is running */
    volatile UTV_BOOL bSectionFilterOpen;   /* true when thread is supposed to be running */
    FILE * pFile;                           /* CRTL file handle */
    UTV_BYTE * pPacketBuffer;               /* buffer for file operations */
    UTV_UINT32 iPacketsRead;                /* counter of packets read */
    UTV_UINT32 iPacketsPosition;            /* position of colors in the file */
    UTV_UINT32 iSimulatedTicks;             /* simulated clock time from start of section filter */
    UTV_UINT32 iPacketsPerTick;             /* packets to deliver per second */
    UTV_UINT32 iMillisecondsPerTick;        /* milliseconds per tick */
    UTV_UINT32 iFilesRead;                  /* counter of number of times files read */
    UTV_UINT32 iOpenTime;                   /* Time when UtvOpenSection() was called. */
    UTV_UINT32 iOpenTotalTime;              /* Total open time. */
    UTV_WAIT_HANDLE hKillWait;              /* used to time thread kill wait */
}  UtvPlatformSectionFilterStatus;

UtvPlatformSectionFilterStatus gPersSectionFilterStatus = { 0 };

UTV_UINT32 g_uiTSFilePacketOffset = 0;

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
    This call releases the hardware reserved by the UtvPlatformDataBroadcastAcquireHardware function
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

UTV_RESULT UtvPlatformDataBroadcastGetTuneInfo( UTV_UINT32 *puiPhysicalFrequency, UTV_UINT32 *puiModulationType )
{
    *puiPhysicalFrequency = 0;
    *puiModulationType = 0;
    return UTV_OK;
}

/* UtvPlatformDataBroadcastGetFileSectionPacketDisk
    Gets one packet from the file
   Returns NULL or pointer to 188 byte packet
*/
UTV_BOOL UtvPlatformGetTransportStreamPacketDisk()
{
    UTV_UINT32 iReadCount;

    /* See if we need to get out */
    if ( gPersSectionFilterStatus.bSectionFilterOpen == UTV_FALSE )
        return UTV_FALSE;

    /* Read one packet */
    iReadCount = 0;
    if ( gPersSectionFilterStatus.pFile != NULL )
        iReadCount = (UTV_UINT32)fread( gPersSectionFilterStatus.pPacketBuffer, UTV_MPEG_PACKET_SIZE, 1, gPersSectionFilterStatus.pFile );

    /* Did we get a packet that time? */
    if ( iReadCount == 1 )
        return UTV_TRUE;
    
    /* Didn't read a packet, make sure handle still good */
    if ( gPersSectionFilterStatus.pFile != NULL ) 
    {
        /* The handle is still lit, are we at the end of the file? */
        if ( feof( gPersSectionFilterStatus.pFile ) )
        {
            /* Count this as a file that we read completely */
            gPersSectionFilterStatus.iFilesRead++;
            UtvMessage( UTV_LEVEL_INFO, " file %s EOF detected after %d packets",  	UtvPlatformGetTSName(), gPersSectionFilterStatus.iPacketsPosition );

#ifndef UTV_TEST_TS_FILE_LOOP
            UtvUserExit();
            return UTV_FALSE;
#endif

            UtvMessage( UTV_LEVEL_INFO, " file %s has been read %d time%s, rewinding",  
                        	UtvPlatformGetTSName(), gPersSectionFilterStatus.iPacketsPosition, gPersSectionFilterStatus.iFilesRead == 1 ? "" : "s" );

            /* Seek back to the beginning of the file */
            gPersSectionFilterStatus.iPacketsPosition = 0;
            if ( 0 != fseek( gPersSectionFilterStatus.pFile, 0, SEEK_SET ) )
                return UTV_FALSE;
        }
    }

    /* Now try to read the file one more time */
    if ( gPersSectionFilterStatus.pFile != NULL )
        iReadCount = (UTV_UINT32)fread( gPersSectionFilterStatus.pPacketBuffer, UTV_MPEG_PACKET_SIZE, 1, gPersSectionFilterStatus.pFile );
    
    /* Did we get a packet that time? */
    if ( iReadCount == 1 )
        return UTV_TRUE;

    /* No data available, punt */
    return UTV_FALSE;
}

/* UtvGetFileSectionPacket
    Gets one packet from the file
    Optionally checks for sync loss and resyncs file-based transport streams
   Returns NULL or pointer to 188 byte packet
*/
UTV_BYTE * UtvPlatformGetTransportStreamPacket()
{
#ifdef UTV_PERS_DISK_RESYNC  /* Recover from sync loss */

    UTV_BOOL bGotPacket = UTV_FALSE;
    UTV_BOOL bLostSyncMessage = UTV_FALSE;
    fpos_t fpPosition;

    /* Loop to get a wholesome packet from the disk file, resyncing if needed */
    while ( ! bGotPacket )
    {
        /* Try for a packet */
        bGotPacket = UtvPlatformGetTransportStreamPacketDisk();

        /* Was there an abort? */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            return NULL;
    
        /* Make sure the sync byte  */
        if ( bGotPacket && gPersSectionFilterStatus.pPacketBuffer[ 0 ] != 0x47 )
        {
            /* We got a packet without a sync byte, skip back 187 bytes */
            if ( 0 == fgetpos( gPersSectionFilterStatus.pFile, & fpPosition ) )
            {
                /* Could get the position, back it up */
                bGotPacket = UTV_FALSE;
                if ( ! bLostSyncMessage )
                {
                    bLostSyncMessage = UTV_TRUE;
                    UtvMessage( UTV_LEVEL_INFO, " file %s lost transport stream sync at %d of %s", UtvGetStreamFilename(), UTV_INT32)fpPosition  );
                }
                fpPosition -= 1;
                fsetpos( gPersSectionFilterStatus.pFile, & fpPosition );
            }
        }
    }

    /* Hey we got it back */
    if ( bLostSyncMessage )
        UtvMessage( UTV_LEVEL_INFO, " file %s acquired transport stream sync at %d", UtvGetStreamFilename(), UTV_INT32)fpPosition );
#else /* UTV_PERS_DISK_RESYNC */

    /* Get a packet */
    if ( !UtvPlatformGetTransportStreamPacketDisk() )
        return NULL;

#endif /* UTV_PERS_DISK_RESYNC */

    /* Count a packet and return the pointer to the buffer */
    gPersSectionFilterStatus.iPacketsRead++;
    gPersSectionFilterStatus.iPacketsPosition++;
    return gPersSectionFilterStatus.pPacketBuffer;
}

#ifdef UTV_OS_SINGLE_THREAD

/* SINGLE-THREADED PUMP EXAMPLE
*/

/* UtvFilePump
    Thread-free delivery of packets from a file to the section filter.
    Deliver packets to the packet assembler until we notice that a section has been delivered.
    UtvFilePump is called directly by the core in UtvGetSectionData when UTV_OS_SINGLE_THREAD
    is defined. 
*/
UTV_RESULT UtvPlatformFilePump( UTV_UINT32 * piTimer )
{
    UTV_UINT32 uiTimeElapsed = 0;
    UTV_BYTE * pData;
    UTV_UINT32 uiStartTime;
    UTV_UINT32 iTotalSections;

    /* If section filter not running just return right now thank you very much */
    if ( ! gPersSectionFilterStatus.bSectionFilterOpen )
        return UTV_BAD_SECTION_FILTER_NOT_RUNNING;

    /* Make note of the time for timeout purposes */
    uiStartTime = UtvGetTickCount();

    /* Get the number of sections delivered up to now */
    iTotalSections = g_pUtvDiagStats->iSectionsDelivered;

    /* Stay here until we process enough packets to deliver a section  */
    while ( iTotalSections == g_pUtvDiagStats->iSectionsDelivered )
    {
        /* If we get a packet, deliver it to the assembler */
        if ( NULL != ( pData = UtvPlatformGetTransportStreamPacket() ))
            UtvDeliverTransportStreamPacket( gPersSectionFilterStatus.iPid, gPersSectionFilterStatus.iTableId, gPersSectionFilterStatus.iTableIdExt, pData );

        /* keep track of elapsed time and check for timeout. */
        uiTimeElapsed = UtvGetTickCount() - uiStartTime;
        
        if ( *piTimer <= uiTimeElapsed )
            break;

        /* Was there an abort? */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            break;
    }

    /* update returned timer value */
    if ( *piTimer < uiTimeElapsed )
        *piTimer = 0;
    else
        *piTimer -= uiTimeElapsed;

    /* Was an abort signaled? */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    /* See if we timed out */
    if ( *piTimer == 0 )
        return UTV_TIMEOUT;

    /* Otherwise return OK we got a section */
    return UTV_OK;
}

#else /* UTV_OS_SINGLE_THREAD */

/* MULTI-THREADED PUMP EXAMPLE
*/

/* UtvFilePumpThread
    Thread that delivers file data at a controlled rate to the main thread
    Each section from the file is delivered with a short delay between each section, simulating callback of a hardware section fitler
    UtvFilePumpThread is created from UtvPlatformOpenSectionFilter() and destroyed by
    UtvPlatformCloseSectionFilter() when UTV_OS_SINGLE_THREAD is NOT defined (default).
*/
UTV_THREAD_ROUTINE UtvFilePumpThread( )
{
    /* Remember thread is running */
    gPersSectionFilterStatus.bThreadRunning = UTV_TRUE;

    /* Initialize throttling machinery */
    if ( gPersSectionFilterStatus.iMillisecondsPerTick == 0 )
    {
        gPersSectionFilterStatus.iMillisecondsPerTick = 100;
        gPersSectionFilterStatus.iPacketsPerTick = ( ( UTV_TEST_TS_FILE_BITRATE / 8 ) / 188 ) / 10;
        if ( 0 == gPersSectionFilterStatus.iPacketsPerTick )
            gPersSectionFilterStatus.iPacketsPerTick = 1;
    }

    /* Data pump to other thread */
    while ( gPersSectionFilterStatus.bSectionFilterOpen )
    {
        /* Get a packet from the file */
        UTV_BYTE * pData = UtvPlatformGetTransportStreamPacket();

        /* If we got a packet, deliver it to the assembler */
        if ( pData != NULL )
            UtvDeliverTransportStreamPacket( gPersSectionFilterStatus.iPid, gPersSectionFilterStatus.iTableId, gPersSectionFilterStatus.iTableIdExt, pData );
        /* See if we need to sleep to simulate stream speed */
        if ( gPersSectionFilterStatus.iSimulatedTicks < gPersSectionFilterStatus.iPacketsRead / gPersSectionFilterStatus.iPacketsPerTick )
        {
            /* Sleep a while and count it on our clock */
            UtvSleep( gPersSectionFilterStatus.iMillisecondsPerTick );

            /* Count the simulated time that has gone by */
            gPersSectionFilterStatus.iSimulatedTicks++;
        }

        /* Was there an abort? */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            break;
    }

    /* When we get here we need to terminate this thread now, just return to exit */
    gPersSectionFilterStatus.bThreadRunning = UTV_FALSE;

    /* perform os-specific exit of thread. */
    UTV_THREAD_EXIT;
}

#endif /* UTV_OS_SINGLE_THREAD */

/* UtvPlatformDataBroadcastCreateSectionFilter
    Called after Mutex and Wait handles established with state set to STOPPED
    First step in using a section filter
    Sets up the section filter context
    Next step is to open the section filter
   Returns OK if the section filter has been created ok
*/
UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pPumpFunc )
{
    /* Open the file (only if it isn't already open) */
    if ( gPersSectionFilterStatus.pFile == NULL )
        gPersSectionFilterStatus.pFile = fopen( UtvPlatformGetTSName(), "rb" );
    if ( gPersSectionFilterStatus.pFile == NULL )
    {
        UtvMessage( UTV_LEVEL_WARN, " file %s could not be read", UtvPlatformGetTSName() );
        return UTV_NO_SECTION_FILTER;
    }

    if ( 0 != g_uiTSFilePacketOffset )
        fseek( gPersSectionFilterStatus.pFile, UTV_MPEG_PACKET_SIZE * g_uiTSFilePacketOffset, SEEK_SET );

    /* Get the buffer */
    gPersSectionFilterStatus.pPacketBuffer = (UTV_BYTE *)UtvMalloc( UTV_MPEG_PACKET_SIZE );
    if ( gPersSectionFilterStatus.pPacketBuffer == NULL )
        return UTV_NO_MEMORY;

    /* If this is the single threaded version of this data source then */
    /* return a pointer to the pump function.  Else return NULL. */
#ifdef UTV_OS_SINGLE_THREAD
    *pPumpFunc = UtvPlatformFilePump;
#else
    *pPumpFunc = NULL;    
#endif

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastOpenSectionFilter
    Called here from UtvPlatformDataBroadcastPersOpenSectionFilter to start the section filter with the state set to READY
    Should start data delivery even though main thread may not be ready for data yet
    Next step is to get section data 
   Returns UTV_OK if section filter opened and ready
*/
UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( 
    IN UTV_UINT32 iPid, 
    IN UTV_UINT32 iTableId, 
    IN UTV_UINT32 iTableIdExt, 
    IN UTV_BOOL bEnableTableIdExt )
{
    /* We need to copy the data for local use */
    if ( gPersSectionFilterStatus.bSectionFilterOpen == UTV_TRUE )
        return UTV_NO_SECTION_FILTER;
    gPersSectionFilterStatus.iPid = iPid;
    gPersSectionFilterStatus.iTableId = iTableId;
    gPersSectionFilterStatus.iTableIdExt = iTableIdExt;
    gPersSectionFilterStatus.bEnableTableIdExt = bEnableTableIdExt;
    gPersSectionFilterStatus.bSectionFilterOpen = UTV_TRUE;

    /* keep track of time for bit rate display */
    gPersSectionFilterStatus.iOpenTime = UtvGetTickCount();

#ifndef UTV_OS_SINGLE_THREAD
    /* Init the kill wait handle */
    UtvWaitInit( &gPersSectionFilterStatus.hKillWait );

    /* Start the data pump thread */
#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: UtvFilePumpThread from\n%s:%d", __FILE__, __LINE__);
#endif
    if ( UTV_OK != UtvThreadCreate( &gPersSectionFilterStatus.hThread, &UtvFilePumpThread, (void *)&gPersSectionFilterStatus ) ) 
    {
        return UTV_NO_SECTION_FILTER;
    }
    
#endif 

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCloseSectionFilter
    Called to stop delivery of section filter data with mutex locked and start changed to STOPPED
    Next step is to destroy the section filter or be opened on another PID/TID
    This code should leave resources available for the next section filter open operation
   Returns OK if everything closed normally
*/
UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( )
{
    UTV_RESULT rStatus = UTV_OK;
#ifndef UTV_OS_SINGLE_THREAD
    UTV_UINT32 iCount;
#endif

    /* Thread shutdown, first set the flag */
    gPersSectionFilterStatus.bSectionFilterOpen = UTV_FALSE;

#ifndef UTV_OS_SINGLE_THREAD

    /* Wait for the death of the thread */
    for ( iCount = 0; iCount < 30; iCount++ )
    {
        /* Sleep for a bit to give the thread time to exit */
        UTV_UINT32 iTimer = 100;
        rStatus = UtvWaitTimed( gPersSectionFilterStatus.hKillWait, &iTimer );

        if ( iTimer != 0 )
        {
            /* something interrupted the wait - check for abort */
            if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            {
                rStatus = UTV_ABORT;
                break;
            }
        }
        /* If the other thread is gone then go ahead, otherwise wait a bit longer */
        if ( gPersSectionFilterStatus.bThreadRunning == UTV_FALSE ) 
            break;
    }

    /* If abort signaled, get out now */
    if ( rStatus == UTV_ABORT )
        return rStatus;

    /* Is the thread still running? */
    if ( gPersSectionFilterStatus.bThreadRunning == UTV_TRUE ) 
    {
        UtvMessage( UTV_LEVEL_ERROR, " unable to stop thread  "); 
        rStatus = UTV_BAD_THREAD;
    }

    /* Clean house */
    rStatus = UtvThreadDestroy( &gPersSectionFilterStatus.hThread );
    if ( UTV_OK != rStatus )
    {
        UtvMessage( UTV_LEVEL_ERROR, " unable to destroy thread, error %s (%s) ", UtvGetMessageString(rStatus), strerror( errno ) ); 
    }

    /* free the kill wait handle */
    UtvWaitDestroy( gPersSectionFilterStatus.hKillWait );
#endif

    gPersSectionFilterStatus.iOpenTotalTime += (UtvGetTickCount() - gPersSectionFilterStatus.iOpenTime);

    /* avoid divide by zero */
    if ( 0 == gPersSectionFilterStatus.iOpenTotalTime )
        gPersSectionFilterStatus.iOpenTotalTime = 1;

    UtvMessage( UTV_LEVEL_INFO, " TS_FILE Bitrate: %lu", 
                ((gPersSectionFilterStatus.iPacketsRead * 188 * 8)/gPersSectionFilterStatus.iOpenTotalTime)*1000 );

    /* Return status */
    return rStatus;
}

/* UtvPlatformDataBroadcastDestroySectionFilter
    Destroy the context created by the UtvPlatformDataBroadcastOpenSectionFilter call
    Called with mutex locked and state changed to UNKNOWN
    Any hardware context is released
    All memory allocated by the given section filter is freed
   Returns OK if everything done normally
*/
UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( )
{
    /* Dump the buffer */
    if ( gPersSectionFilterStatus.pPacketBuffer != NULL )
        UtvFree( gPersSectionFilterStatus.pPacketBuffer );
    gPersSectionFilterStatus.pPacketBuffer = NULL;

    /* Return status */
    return UTV_OK;
}
