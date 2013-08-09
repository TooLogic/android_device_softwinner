/* linux-mutex.cpp: UpdateTV Personality Map - Mutex and Locks for Linux

   This module is responsible for the implementation of locks under Linux.
   Each of these calls map into a Unix/Linux POSIX Pthreads call.
   Unless the host operating system doesn't support the Pthreads environment, customers
   should not have to modify this file.

  Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      01/25/2010  Added time jitter check to UtvWaitTimed().
  Bob Taylor      08/06/2008  Converted from C++ to C.
  Bob Taylor      08/22/2007  Moved UtvSleep() and UtvGetTickCount() to xxxx-time.cpp.
  Bob Taylor      05/01/2007  Linux only for 1.9.
  Bob Taylor      11/02/2006  Switched from events to counting semaphores as part of the fix for issue #194.
  Chris Nevin     03/13/2006  Changed PTHREADS to POSIX
  Chris Nevin     02/06/2006  Added mutexes to protect the list of wait handles and mutexes
  Chris Nevin     01/31/2006  Returned actual error code and check for malloc failures
  Greg Scott      10/03/2005  Make first mutex handle non-zero to prevent zero handles from being valid.
  Greg Scott      09/22/2005  Check abort state before UtvSleep.
  Greg Scott      09/13/2005  Use abortable timed wait instead of sleep code for UtvSleep.
  Greg Scott      07/20/2005  Fix problem with calculation of UtvWaitTimed on Linux.
  Greg Scott      01/18/2005  Fix problem with lost handles.
  RRLee           12/30/2004  Fixed event signalling.
  Steve Hastings  12/15/2004  Removed Linux compile errors/warnings.
  Greg Scott      10/25/2004  Use handles for mutex and wait functions.
  Greg Scott      10/20/2004  Creation.
*/

#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include "utv.h"

#define ONEBILLION  1000000000  /* nsec roll-over count */
#define UTV_MUTEX_MUTEX pthread_mutex_t 

/* Global Mutex Storage
*/
#define UTV_MUTEX_COUNT 6
#define UTV_MUTEX_HANDLE_BASE 100
#define UTV_MUTEX_HANDLE_LAST ( UTV_MUTEX_HANDLE_BASE + UTV_MUTEX_COUNT - 1 )
UTV_MUTEX_MUTEX * apUtvMutex[ UTV_MUTEX_COUNT ] = { NULL };

/* Mutexes to protect the global mutex and wait lists 
*/
static UTV_MUTEX_MUTEX utvMutexListMutex;
static UTV_MUTEX_MUTEX utvWaitListMutex;

/* static protos
*/
static UTV_MUTEX_MUTEX * UtvValidateMutexHandle( UTV_MUTEX_HANDLE hMutexHandle );
static UTV_RESULT UtvAssignMutexHandle( UTV_MUTEX_HANDLE * phMutexHandle, UTV_MUTEX_MUTEX * * ppMutex );
static UTV_RESULT UtvUnassignMutexHandle( UTV_MUTEX_HANDLE hMutexHandle );
static UTV_RESULT UtvMutexListLock( void );
static UTV_RESULT UtvMutexListUnlock( void );
static UTV_RESULT UtvWaitListLock( void );
static UTV_RESULT UtvWaitListUnlock( void );

/* UtvInitMutexList
    Initialize the mutex that protects the list of mutexes
    This mutex is not available for use outside of this module
*/
UTV_RESULT UtvInitMutexList( void )
{
    /* Init the pthreads mutex */
    if ( 0 != pthread_mutex_init( &utvMutexListMutex, NULL ) )
        return UTV_NO_MUTEX;

    return UTV_OK;
}

/* UtvMutexListLock
    Obtain the exclusive lock on the list of mutexes so the list can be updated
*/
static UTV_RESULT UtvMutexListLock( void )
{
    /* Try to lock it, block until we can get it */
    if ( 0 != pthread_mutex_lock( &utvMutexListMutex ) )
        return UTV_NO_LOCK;

    return UTV_OK;
}

/* UtvMutexListUnlock
    Release the exclusive lock on the list of mutexes
*/
static UTV_RESULT UtvMutexListUnlock( void )
{
    /* Blast the mutex to smithereens, letting other threads grab it */
    if ( 0 != pthread_mutex_unlock( &utvMutexListMutex ) )
        return UTV_NO_LOCK;

    return UTV_OK;

}

/* UtvDestroyMutexList
    Release and destroy the exclusive lock on the list of mutexes
*/
void UtvDestroyMutexList( void )
{
    UTV_UINT32 iReturn;

    /* Blast the mutex to smithereens, letting other threads grab it */
    if ( 0 != pthread_mutex_unlock( &utvMutexListMutex ) )
        return;

    iReturn = pthread_mutex_destroy( &utvMutexListMutex ) ;

    /* We are handed locked mutexes on occasion */
    if ( iReturn == EBUSY )
    {
        pthread_mutex_unlock( &utvMutexListMutex ) ;
        pthread_mutex_destroy( &utvMutexListMutex ) ;
    }
}

/* UtvValidateMutexHandle
    Validates a mutex handle
   Returns NULL or the address of the mutex storage block
*/
static UTV_MUTEX_MUTEX * UtvValidateMutexHandle( 
    UTV_MUTEX_HANDLE hMutex )
{
    /* Handle is index into our table, make sure handle is in range */
    if ( hMutex < UTV_MUTEX_HANDLE_BASE || hMutex > UTV_MUTEX_HANDLE_LAST ) 
        return NULL;

    /* Return the operating system specific handle (or NULL if handle not assigned) */
    return apUtvMutex[ hMutex - UTV_MUTEX_HANDLE_BASE ];
}

UTV_BOOL
UtvMutexHandles( void )
{
    UTV_UINT32 iIndex;

    /* see if any handles are in use */
    for ( iIndex = 0; iIndex < UTV_MUTEX_COUNT; iIndex++ )
    {
      if ( NULL != apUtvMutex[ iIndex ] ) /* first test for init'd mutex */
      {
        /* now test if it's in use - try to lock it */
        if ( 0 != pthread_mutex_trylock( apUtvMutex[ iIndex ] ) )
          return UTV_TRUE; /* it is in use */
        else /* clean up */
          (void) pthread_mutex_unlock( apUtvMutex[ iIndex ] );
      }
    }

    return UTV_FALSE;
}

/* UtvAssignMutexHandle
    Assigns a new mutex handle
   Returns OK if handle assigned
*/
static UTV_RESULT UtvAssignMutexHandle( 
    UTV_MUTEX_HANDLE * phMutexHandle, 
    UTV_MUTEX_MUTEX * * ppMutex )
{
    UTV_RESULT rStatus = UTV_NO_MUTEX;
    UTV_UINT32 iIndex;

    /* lock the list first */
    if ( UtvMutexListLock() != UTV_OK )
        return UTV_NO_LOCK;

    /* Look for an empty slot */
    for ( iIndex = 0; iIndex < UTV_MUTEX_COUNT; iIndex++ )
    {
        if ( apUtvMutex[ iIndex ] == NULL )
        {
            /* found a slot, allocate a new section filter status and return that handle */
            apUtvMutex[ iIndex ] = (UTV_MUTEX_MUTEX *)UtvMalloc( sizeof( UTV_MUTEX_MUTEX ) );
            if ( apUtvMutex[ iIndex ] != NULL )
            {
                *phMutexHandle = iIndex + UTV_MUTEX_HANDLE_BASE;
                *ppMutex = apUtvMutex[ iIndex ];
                rStatus = UTV_OK;
                break;
            }
            else
            {
                rStatus = UTV_NO_MEMORY;
                break;
            }
        }
    }

    UtvMutexListUnlock();

    if ( rStatus == UTV_NO_MUTEX )
    {
        /* No slots available */
        *phMutexHandle = (long unsigned int) -1;
        *ppMutex = NULL;
    }

    return rStatus;
}

/* UtvUnassignMutexHandle
    Clears a new Mutex handle
*/
static UTV_RESULT UtvUnassignMutexHandle( 
    UTV_MUTEX_HANDLE hMutex )
{
    UTV_RESULT rStatus = UTV_OK;
    UTV_MUTEX_MUTEX * pMutex;

    /* lock the list first */
    if ( UtvMutexListLock() != UTV_OK )
        return UTV_NO_LOCK;

    /* Validate handle */
    if ( (pMutex = UtvValidateMutexHandle( hMutex )) != NULL )
    {
        /* Free the memory and clear the slot */
        UtvFree( pMutex );
        apUtvMutex[ hMutex - UTV_MUTEX_HANDLE_BASE ] = NULL;
    }
    else
    {
        rStatus = UTV_BAD_MUTEX_HANDLE;
    }

    UtvMutexListUnlock();
    return rStatus;
}

/* UtvMutexInit
    External interface to initialize a mutex for use
*/
UTV_RESULT UtvMutexInit( UTV_MUTEX_HANDLE * phMutex )
{
    UTV_RESULT rStatus;
    UTV_MUTEX_MUTEX * pMutex = NULL;

    *phMutex = 0;

    /* Get a new handle */
    rStatus = UtvAssignMutexHandle( phMutex, & pMutex );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Init the pthreads mutex */
    if ( 0 != pthread_mutex_init( pMutex, NULL ) )
        return UTV_NO_MUTEX;

    /* Created ok */
    return UTV_OK;
}

/* UtvMutexDestroy
*/
UTV_RESULT UtvMutexDestroy( UTV_MUTEX_HANDLE hMutex )
{
    UTV_UINT32 iReturn;

    /* Validate handle */
    UTV_MUTEX_MUTEX * pMutex = UtvValidateMutexHandle( hMutex );
    if ( pMutex == NULL )
        return UTV_BAD_MUTEX_HANDLE;

    /* Blast the Mutex to smithereens */
    iReturn = pthread_mutex_destroy( pMutex ) ;

    /* We are handed locked mutexes on occasion */
    if ( iReturn == EBUSY )
    {
        pthread_mutex_unlock( pMutex ) ;
        iReturn = pthread_mutex_destroy( pMutex ) ;
    }
    if ( 0 != iReturn )
        return UTV_NO_MUTEX;

    /* Release our handle (storage) and return */
    return UtvUnassignMutexHandle( hMutex );
}

/* UtvMutexLock
    Locks one mutex
   Returns OK if locked
*/
UTV_RESULT UtvMutexLock( UTV_MUTEX_HANDLE hMutex )
{
    /* Validate handle */
    UTV_MUTEX_MUTEX * pMutex = UtvValidateMutexHandle( hMutex );
    if ( pMutex == NULL )
        return UTV_BAD_MUTEX_HANDLE;

    /* Try to lock it, block until we can get it */
    if ( 0 != pthread_mutex_lock( pMutex ) )
        return UTV_NO_LOCK;

    /* Return good status */
    return UTV_OK;
}

/* UtvMutexUnLock
    Unlocks one mutex
   Returns OK if unlocked
*/
UTV_RESULT UtvMutexUnlock( UTV_MUTEX_HANDLE hMutex )
{
    /* Validate handle */
    UTV_MUTEX_MUTEX * pMutex = UtvValidateMutexHandle( hMutex );
    if ( pMutex == NULL )
        return UTV_BAD_MUTEX_HANDLE;

    /* Blast the mutex to smithereens, letting other threads grab it */
    if ( 0 != pthread_mutex_unlock( pMutex ) )
        return UTV_NO_LOCK;

    /* Check error code */
    return UTV_OK;
}

/* UtvWaitWait structure
    Maintain information for UtvWait routine handles
*/
typedef struct {
   UTV_UINT32 uiCount;          /* counting semaphore */
   pthread_cond_t cCond;
   pthread_mutex_t mMutex;
}  UtvWaitWait;
#define UTV_WAIT_MAX 4

/* Wait routines storage
*/
UtvWaitWait * apUtvWait[ UTV_WAIT_MAX ] = { NULL };

/* Worker (private) routines forward declaration
*/
UtvWaitWait * UtvValidateWaitHandle( UTV_WAIT_HANDLE hMutexHandle );
UTV_RESULT UtvAssignWaitHandle( UTV_WAIT_HANDLE * phWait, UtvWaitWait * * ppWait );
UTV_RESULT UtvUnassignWaitHandle( UTV_WAIT_HANDLE hWait );

/* UtvInitMutexList
    Initialize the mutex that protects the list of mutexes
    This mutex is not available for use outside of this module
*/
UTV_RESULT UtvInitWaitList( void )
{
    /* Init the pthreads mutex */
    if ( 0 != pthread_mutex_init( &utvWaitListMutex, NULL ) )
        return UTV_NO_MUTEX;

    return UTV_OK;
}

/* UtvWaitListLock
    Obtain the exclusive lock on the list of wait handles so the list can be updated
*/
static UTV_RESULT UtvWaitListLock( void )
{
    /* Try to lock it, block until we can get it */
    if ( 0 != pthread_mutex_lock( &utvWaitListMutex ) )
        return UTV_NO_LOCK;

    return UTV_OK;
}

/* UtvWaitListUnlock
    Release the exclusive lock on the list of wait handles
*/
static UTV_RESULT UtvWaitListUnlock( void )
{
    /* Blast the mutex to smithereens, letting other threads grab it */
    if ( 0 != pthread_mutex_unlock( &utvWaitListMutex ) )
        return UTV_NO_LOCK;

    return UTV_OK;
}

/* UtvDestroyWaitList
    Release and destroy the exclusive lock on the list of wait handles
*/
void UtvDestroyWaitList( )
{
    UTV_UINT32 iReturn;

    /* Blast the mutex to smithereens, letting other threads grab it */
    if ( 0 != pthread_mutex_unlock( &utvWaitListMutex ) )
        return;

    iReturn = pthread_mutex_destroy( &utvWaitListMutex ) ;

    /* We are handed locked mutexes on occasion */
    if ( iReturn == EBUSY )
    {
        pthread_mutex_unlock( &utvWaitListMutex ) ;
        pthread_mutex_destroy( &utvWaitListMutex ) ;
    }
}

/* UtvValidateWaitHandle
    Validates a wait handle
   Returns NULL or the address of the wait storage block
*/
UtvWaitWait * UtvValidateWaitHandle( UTV_WAIT_HANDLE hWait )
{
    /* Handle is index into our table, make sure handle is in range */
    if ( hWait < 0 ) 
        return NULL;
    if ( hWait >= UTV_WAIT_MAX ) 
        return NULL;

    /* Return the pointer to the UtvWaitWait (or NULL if not assigned) */
    return apUtvWait[ hWait ];
}

UTV_BOOL 
UtvWaitHandles( void )
{
    UTV_UINT32 iIndex;

    /* see if any handles are in use */
    for ( iIndex = 0; iIndex < UTV_WAIT_MAX; iIndex++ )
    {
      if ( ( apUtvWait[ iIndex ] != NULL ) && 
           ( &(apUtvWait[ iIndex ]->mMutex) != NULL ) )
        {
          /* now test if it's in use - try to lock it */
          if ( 0 != pthread_mutex_trylock( &(apUtvWait[ iIndex ]->mMutex) ) )
            return UTV_TRUE; /* it is in use */
          else /* clean up */
            (void) pthread_mutex_unlock( &(apUtvWait[ iIndex ]->mMutex) );
        }
    }

    return UTV_FALSE;
}

/* UtvAssignWaitHandle
    Assigns a new wait handle and its storage
   Returns OK if handle assigned
*/
UTV_RESULT UtvAssignWaitHandle( UTV_WAIT_HANDLE * phWait, UtvWaitWait * * ppWait )
{
    UTV_RESULT rStatus = UTV_NO_WAIT;
    UTV_UINT32 iIndex;

    if ( UtvWaitListLock( ) != UTV_OK )
        return UTV_NO_LOCK;

    /* Look for an empty slot */
    for ( iIndex = 0; iIndex < UTV_WAIT_MAX; iIndex++ )
    {
        if ( apUtvWait[ iIndex ] == NULL )
        {
            /* found a slot, allocate a new section filter status and return that handle */
            apUtvWait[ iIndex ] = (UtvWaitWait *)UtvMalloc( sizeof( UtvWaitWait ) );
            if ( apUtvWait[ iIndex ] != NULL )
            {
                *phWait = iIndex;
                *ppWait = apUtvWait[ iIndex ];
                rStatus = UTV_OK;
                break;
            }
            else
            {
                rStatus = UTV_NO_MEMORY;
                break;
            }
        }
    }

    UtvWaitListUnlock( );

    if ( rStatus == UTV_NO_WAIT )
    {
        /* No slots available */
        *phWait = 0;
        *ppWait = NULL;
    }

    return rStatus;
}

/* UtvUnassignWaitHandle
    Frees the wait handle and its storage
   Returns OK always after freeing the handle
*/
UTV_RESULT UtvUnassignWaitHandle( UTV_WAIT_HANDLE hWait )
{
    UTV_RESULT rStatus = UTV_OK;
    UtvWaitWait * pWait;

    if ( UtvWaitListLock( ) != UTV_OK )
        return UTV_NO_LOCK;

    /* Validate handle */
    if ( (pWait = UtvValidateWaitHandle( hWait )) != NULL )
    {
        /* Free the memory and clear the slot */
        UtvFree( pWait );
        apUtvWait[ hWait ] = NULL;
    }
    else
    {
        rStatus = UTV_BAD_WAIT_HANDLE;
    }

    UtvWaitListUnlock( );   
    return rStatus;
}

/* UtvWaitInit
    Initializes new wait object
   Returns OK if handle ok and wait objects initialized
*/
UTV_RESULT UtvWaitInit( UTV_WAIT_HANDLE * phWait )
{
    UTV_RESULT rStatus;
    UtvWaitWait * pWait = NULL;

    *phWait = 0;

    /* Assign handle */
    rStatus = UtvAssignWaitHandle( phWait, &pWait );
    if ( rStatus != UTV_OK )
        return rStatus;

    /* Intialize the mutex and condition for pthreads */
    pthread_cond_init( &pWait->cCond, NULL );
    pthread_mutex_init( &pWait->mMutex, NULL );
    pWait->uiCount = 0;

    /* It's all good */
    return UTV_OK;
}

/* UtvWaitDestroy
    Dumps the wait objects and unassigns the handle
   Returns OK always
*/
UTV_RESULT UtvWaitDestroy( UTV_WAIT_HANDLE hWait )
{
    /* Validate handle */
    UtvWaitWait * pWait = UtvValidateWaitHandle( hWait );
    if ( pWait == NULL )
        return UTV_BAD_WAIT_HANDLE;

    /* Blast the condition to smithereens */
    pthread_cond_destroy( &pWait->cCond );
    pthread_mutex_destroy( &pWait->mMutex );

    /* It is gone, free up the handle */
    return UtvUnassignWaitHandle( hWait );
}

/* UtvWaitTimed
    Waits for event or a timeout, decrementing a timer
   Returns with status indicating if timeout or normal wakeup and timer decremented
*/
UTV_RESULT UtvWaitTimed( UTV_WAIT_HANDLE hWait, UTV_UINT32 * piTimer )
{
    UTV_UINT32 iEndTickCount;

    /* Default the return status to normal */
    UTV_RESULT rStatus = UTV_OK;

    /* Time this wait */
    UTV_UINT32 iStartTickCount = UtvGetTickCount();

    /* Validate handle */
    UtvWaitWait * pWait = UtvValidateWaitHandle( hWait );
    if ( pWait == NULL )
        return UTV_BAD_WAIT_HANDLE;

    /* Must lock the mutex first */
    if ( 0 != pthread_mutex_lock( &pWait->mMutex ) )
        rStatus = UTV_TIMEOUT;
    if ( rStatus == UTV_OK )
    {
        /* check if event is signalled */
        if ( 0 == pWait->uiCount )
        {
            struct timespec tsTimeout;
            struct timeval tvTimeval;

            /* Get the time of day to wake up in nanoseconds */
            /* Note that timer is in milliseconds but wakeup time is in nanoseconds */
            gettimeofday( &tvTimeval, NULL );
            tsTimeout.tv_sec  = tvTimeval.tv_sec + *piTimer / 1000;
            tsTimeout.tv_nsec = ( ( tvTimeval.tv_usec + ( *piTimer % 1000 ) * 1000 ) * 1000 );

            /* While over one second of nanoseconds, add nanoseconds back to whole seconds */
            while( tsTimeout.tv_nsec >= ONEBILLION )
            {
                tsTimeout.tv_nsec -= ONEBILLION;
                tsTimeout.tv_sec++;
            }

        /* Locked ok now do the timed wait */
        /* atomically releases the mutex, causing the calling thread to block on the condition variable */
        /* Upon successful completion, the mutex is locked and owned by the calling thread.  */
        /* When timeouts occur, pthread_cond_timedwait() releases and reacquires the mutex.  */
        if ( 0 != pthread_cond_timedwait( &pWait->cCond, &pWait->mMutex, &tsTimeout ) )
            rStatus = UTV_TIMEOUT;
        }

        /* decrement semaphore and release Mutex */
        pWait->uiCount--;
        pthread_mutex_unlock( &pWait->mMutex );
    }

    /* Ding, stop the timer */
    iEndTickCount = UtvGetTickCount();

    /* check for rollback which can happen due to STT jitter */
    if ( iEndTickCount < iStartTickCount )
    {
        /* leave *pTimer at its current value since we can't figure out how to decrement it */
        return rStatus;
    }

    /* Return the number of milliseconds left on the timer (don't let the timer go -ive) */
    if ( ( iEndTickCount - iStartTickCount ) < *piTimer )
        *piTimer -= ( iEndTickCount - iStartTickCount );
    else
        *piTimer = 0;

    /* Return indicating timeout or normal wake up */
    return rStatus;
}

/* UtvWaitSignal
    Wakes up any thread waiting
   Returns OK unless bad handle
*/
UTV_RESULT UtvWaitSignal( UTV_WAIT_HANDLE hWait )
{
    /* Validate handle */
    UtvWaitWait * pWait = UtvValidateWaitHandle( hWait );
    if ( pWait == NULL )
        return UTV_BAD_WAIT_HANDLE;

    /* protect the changes under the mutex */
    if ( 0 != pthread_mutex_lock( &pWait->mMutex ) )
        return UTV_NO_LOCK;
    /* Increment semaphore */
    pWait->uiCount++;

    /* restart any waiting threads and release the mutex */
    pthread_cond_signal( &pWait->cCond );
    pthread_mutex_unlock( &pWait->mMutex );

    /* Good morning */
    return UTV_OK;
}

/* UtvWakeAllThreads
    Iterates through the list of wait signals
    and signals each to unblock the thread/process
*/
void UtvWakeAllThreads( void )
{
    UTV_WAIT_HANDLE hWait;

    /* wake up any threads that might be blocked */
    for ( hWait = 0; hWait < UTV_WAIT_MAX; hWait++ )
    {
        /* Signal this one */
        UtvWaitSignal( hWait );
    }
}
