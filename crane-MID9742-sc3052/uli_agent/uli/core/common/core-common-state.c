/* corestate.c: UpdateTV Common Core - UpdateTV State

   This module implements the UpdateTV Common Core state routines.

   In order to ensure UpdateTV network compatibility, customers must not 
   change UpdateTV Common Core code.

  Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Jim Muchow      02/14/2011  New and improved code used to monitor socket select() calls to determine when UtvAbort() can complete
  Jim Muchow      08/23/2010  Add code used to monitor socket select() calls to determine when UtvAbort() can complete
  Bob Taylor      01/26/2009  Created from 1.9.21
*/

#include <unistd.h>

#include "utv.h"
#include <sys/socket.h>

UTV_MUTEX_HANDLE ghUtvStateMutex = 0;

selectMonitorControlBlock_t g_selectMonitorControlBlock;

/* Local statics */
static int bExitRequest = UTV_FALSE;

/* UtvSetStateTime
    Set the next state and time to sleep in one call
*/
UTV_RESULT UtvSetStateTime( UTV_PUBLIC_STATE iNewState, UTV_UINT32 iMilliseconds )
{
    /* Set the sleep time */
    UTV_RESULT rStatus = UtvSetSleepTime( iMilliseconds );

    /* Set the state and return the status */
    if ( UTV_OK == rStatus )
        rStatus = UtvSetState( iNewState );
    return rStatus;
}

/* UtvGetState
   Return the global state
*/
UTV_PUBLIC_STATE UtvGetState( void )
{
    return g_pUtvState->utvState;
}

/* UtvClearState
    Called to clear the global state, even if it is abort
*/
UTV_RESULT UtvClearState( void )
{
    /* Clear the state */
    g_pUtvState->utvState = UTV_PUBLIC_STATE_SCAN;

    /* Return success */
    return UTV_OK;
}

/* UtvSetState
    Set the global state
   Returns UTV_ABORT if abort state is set, otherwise returns UTV_OK if it worked
*/
UTV_RESULT UtvSetState( UTV_PUBLIC_STATE iNewState )
{
    /* Returned status */
    UTV_RESULT rStatus;

    /* Lock the state */
    if ( UTV_OK != ( rStatus = UtvMutexLock( ghUtvStateMutex ) ))
        return rStatus;

    /* Make sure that the state is within range */
    if ( iNewState >= UTV_PUBLIC_STATE_COUNT  ) 
    {
        /* New status is not in the range of legal status values */
        rStatus = UTV_BAD_STATE;
    }

    /* See if the current state is already set to abort */
    if ( UTV_OK == rStatus && UTV_PUBLIC_STATE_ABORT == g_pUtvState->utvState )
    {
        /* If abort set already then return the abort status */
        rStatus = UTV_ABORT;
    }

    /* Set the new state */
    if ( UTV_OK == rStatus )
    {
        /* if the new state is abort then record the current state */
        if ( UTV_PUBLIC_STATE_ABORT == iNewState )
            g_pUtvState->utvStateWhenAborted = g_pUtvState->utvState;

        /* Time to set the status */
        g_pUtvState->utvState = iNewState;
    }

    /* Unlock the state */
    UtvMutexUnlock( ghUtvStateMutex );

    /* Return the status derived under lock */
    return rStatus;
}

/* UtvSetSubState
    Set the diagnostic sub-state
*/
void UtvSetSubState( UTV_PUBLIC_SSTATE utvSubState, UTV_UINT32 iWait )
{
    g_pUtvDiagStats->utvSubState = utvSubState;
    g_pUtvDiagStats->iSubStateWait = iWait;
    
    /* update the persistent data structure with this change. */
    UtvPlatformPersistWrite();
}

/* UtvGetSleepTime
    Get the global sleep time in milliseconds
*/
UTV_UINT32 UtvGetSleepTime( void )
{
    /* Return the delay in milliseconds */
    return g_pUtvState->iDelayMilliseconds;
}

/* UtvSetSleepTime
    Set the global sleep time in milliseconds
*/
UTV_RESULT UtvSetSleepTime( UTV_UINT32 iMilliseconds )
{
    /* Copy it to the global */
    g_pUtvState->iDelayMilliseconds = iMilliseconds;

#ifdef UTV_TEST_DISCARD_LONG_WAITS  /* For testing/debugging ONLY */
    /* Check if this is a long wait or a short wait and adjust accordingly */
    if ( g_pUtvState->iDelayMilliseconds > 2000 )
    {
        UtvMessage( UTV_LEVEL_WARN, 
                    "Application changing sleep time from %d seconds to 2 seconds because UTV_TEST_DISCARD_LONG_WAITS is defined.", 
                    g_pUtvState->iDelayMilliseconds / 1000 );
        g_pUtvState->iDelayMilliseconds = 2000;
    }
#endif /* UTV_CORE_DISCARD_LONG_WAITS */

    /* It worked */
    return UTV_OK;
}

/* UtvSetExitRequest
    Sets the internal flag which tells UtvAgentTask() to respond to an abort by exiting
*/
void UtvSetExitRequest( void )
{
    bExitRequest = UTV_TRUE;
}

/* UtvGetExitRequest
    Returns the state of the internal exit request flag
*/
int UtvGetExitRequest( void )
{
    return bExitRequest;
}

/* Define UtvAbort() debug flag to monitor "actual" time it takes for the
   function to exit. There are other debug tests that could've been retained
   but they can all be easily restored. */
#define CHATTY
#undef CHATTY
#ifdef CHATTY
static struct timeval  startOfAbort;
static struct timeval  endOfAbort;
#endif

/* UtvAbort
    Must be called by UtvUserAbort to initiate an abort.
    Takes care of internal abort request logistics
*/
UTV_RESULT UtvAbort( void )
{
    int loopTimeInMsecs, waitTime;
    /* Returned status */
    UTV_RESULT rStatus = UTV_OK;
#ifdef CHATTY
    gettimeofday (&startOfAbort, NULL);
#endif
    /* Set the abort state */
    if ( UTV_OK != (rStatus = UtvSetState( UTV_PUBLIC_STATE_ABORT )))
        return rStatus;

    /* check if there is nothing really going on for an early exit */
    if ( ( UTV_FALSE == UtvThreadsActive() ) &&
         ( UTV_FALSE == UtvFilesOpen() ) &&
         ( UTV_FALSE == UtvMutexHandles() ) &&
         ( UTV_FALSE == UtvWaitHandles() ) &&
         ( 0 == g_selectMonitorControlBlock.active ) )
    {
#ifdef CHATTY
    gettimeofday (&endOfAbort, NULL);
    timersub(&endOfAbort, &startOfAbort, &startOfAbort);
    printf("******************** UtvAbort(): early exit %02d.%06d\n", 
           (int)startOfAbort.tv_sec, (int)startOfAbort.tv_usec);
#endif
      return UTV_OK;
    }

    /* Something is apparently active, start waking things up */

    /* Wake up any sleeping threads so they will free up any resources and terminate */

    /* Poke a getHostByNameThread() thread */
    UtvWakeAllThreads( );

    /* Check for and poke a waiting connect or read select() */
    UtvAbortSelectMonitor();

    /* leave everything else alone - the point of this function is to get things
       moving not close down and clean up (that's what shutdown is for */

    /* It is necessary to give any of those sleeping threads a chance to clean up.
       The loop below will wait as long as a full select() would take to time out,
       but the state of the select() will be checked every waitTime msecs */
    loopTimeInMsecs = UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC;
    loopTimeInMsecs *= 1000; /* convert to msecs */
    waitTime = 100; /* how often to check */

    do
    {
        UtvSleepNoAbort( waitTime );

        /* test if a select or other blocking condition is now released */
        if ( 0 == g_selectMonitorControlBlock.active )
        {
          break;
        }

        loopTimeInMsecs -= waitTime;

    } while ( 0 < loopTimeInMsecs );
#ifdef CHATTY
    gettimeofday (&endOfAbort, NULL);
    timersub(&endOfAbort, &startOfAbort, &startOfAbort);
    printf("******************** UtvAbort(): delayed exit %02d.%06d\n", 
           (int)startOfAbort.tv_sec, (int)startOfAbort.tv_usec);
#endif
    return UTV_OK;
}

UTV_RESULT UtvActivateSelectMonitor( void *timeval, int socketFd )
{
    if (g_selectMonitorControlBlock.active)
    {
        UtvMessage( UTV_LEVEL_ERROR, "%s:%d - g_selectMonitorControlBlock.active",
                    __FILE__, __LINE__);
    }

    gettimeofday (&g_selectMonitorControlBlock.startTime, NULL);
    g_selectMonitorControlBlock.waitLength = *( (struct timeval *)timeval );
    g_selectMonitorControlBlock.socketFd = socketFd;
    g_selectMonitorControlBlock.active = 1;
    return UTV_OK;
}

UTV_RESULT UtvAbortSelectMonitor( void )
{
    if (g_selectMonitorControlBlock.active)
      shutdown(g_selectMonitorControlBlock.socketFd, SHUT_RDWR);

    return UTV_OK;
}

UTV_RESULT UtvDeactivateSelectMonitor( void )
{
    g_selectMonitorControlBlock.active = 0;

    return UTV_OK;
}

/* UtvExit
    Must be called by UtvUserExit to initiate an exit request
   Takes care of internal exit request logistics
*/
UTV_RESULT UtvExit( void )
{
    /* indicate to the system that an exit request is being made */
    UtvSetExitRequest();

    /* initiate an abort which will then pick up the exit request */
    return UtvAbort( );
}

/* Diag stat access functions follow
*/


#ifdef UTV_DELIVERY_BROADCAST
/* UtvSaveServerIDScan
    Saves the current server ID to the last scanned stat
*/
void UtvSaveServerIDScan( )
{
    UtvStrnCopy( g_pUtvDiagStats->aubLastScanID,  UTV_SERVER_ID_MAX_LEN, (UTV_BYTE *) UtvGetServerID(), UTV_SERVER_ID_MAX_LEN );
}

/* UtvSaveServerIDModule
    Saves the current server ID to the module downloaded stat
*/
void UtvSaveServerIDModule( )
{
    UtvStrnCopy( g_pUtvDiagStats->aubLastDownloadID,  UTV_SERVER_ID_MAX_LEN, (UTV_BYTE *) UtvGetServerID(), UTV_SERVER_ID_MAX_LEN );
}

/* UtvSaveLastScanTime
    Saves the last DSI broadcast time to the last scanned stat
*/
void UtvSaveLastScanTime( )
{
    g_pUtvDiagStats->tLastScanTime = UtvGetBroadcastTime();
}

/* UtvSaveLastCompatibleScanTime
    Saves the last DSI broadcast time to the last compatible scan stat
*/
void UtvSaveLastCompatibleScanTime( )
{
    g_pUtvDiagStats->tLastCompatibleScanTime = UtvGetBroadcastTime();
}

/* UtvSaveLastModuleTime
    Saves the last DSI broadcast time to the last module downloaded stat
*/
void UtvSaveLastModuleTime( )
{
    g_pUtvDiagStats->tLastModuleTime = UtvGetBroadcastTime();
}

/* UtvSaveLastModuleTime
    Saves the last DSI broadcast time to the last complete download stat
*/
void UtvSaveLastDownloadTime( )
{
    g_pUtvDiagStats->tLastDownloadTime = UtvGetBroadcastTime();
}

#endif

/* UtvIncModuleCount
    Bumps the modules downloaded count stat
*/
void UtvIncModuleCount( )
{
    g_pUtvDiagStats->iModCount++;
}
