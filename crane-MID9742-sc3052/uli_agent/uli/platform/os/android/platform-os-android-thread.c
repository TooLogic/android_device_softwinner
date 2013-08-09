/* linux-thread.cpp: UpdateTV Personality Map - Thread Support for Linux

   This module is responsible for the implementation of threads under Unix/Linux POSIX Pthreads.
   Unless the host operating system doesn't support the Pthreads environment, customers
   should not have to modify this file.

   Copyright (c) 2004 - 2007 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      12/02/2011  Added UtvThreadSetCancelState().
   Jim Muchow      09/29/2010  Add in slight logic to prevent race conditions.
   Jim Muchow      08/03/2010  Add in function to test for active (sub)threads
   Jim Muchow      07/16/2010  Add in threads accounting infrastructure
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Bob Taylor      05/01/2007  Linux only for 1.9.
   Chris Nevin     03/13/2006  Changed PTHREADS to POSIX.
   Greg Scott      01/18/2005  Add destroy thread code.
   Steve Hastings  01/13/2005  More changes for Linux.
   Greg Scott      12/30/2004  Changes for Linux.
   Greg Scott      10/20/2004  Creation.
*/

#include <stdio.h>

/* Must use POSIX on non-Windows platforms
*/
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "utv.h"

/* Threads accounting: maintain a list of threads created and removed them
   when they are exited or cancelled. The purpose is to ensure that when a
   UtvProjectOnShutdown( ) occurs, there are no threads "waiting around"
   that might become active.

   Under normal circumstances there is seldom more than a single thread in 
   operation; there can, however, be two. Define for three.

   The primary (and previously only) quality to check is the thread ID.
   This is the value returned when a new thread is created and otherwise
   used for everything else. The threads library will happily create more
   than one using an identical function (Routine). This is undesireable
   so a second quanlity is address of the function. New threads may not
   be created using the same function.
*/
/*#define THREADS_DEBUG 1*/

typedef struct {
    UTV_THREAD_HANDLE           hThread;
    PTHREAD_START_ROUTINE       pRoutine;
} threadInfo_t;

#define THREAD_COUNT    3
static threadInfo_t threadExists[THREAD_COUNT] = { { 0, 0 } };
static int threadBeingCreated = 0;

/*extern void jniDetachThread();*/

UTV_BOOL UtvThreadsActive( void )
{
    int index;

    for ( index = 0; index < THREAD_COUNT; index++ )
    {
        if ( threadExists[index].hThread )
            return UTV_TRUE;
    }

    return UTV_FALSE;
}

void UtvThreadKillAllThreads( void )
{
    int index;
    int cancelledCount;
    int result;
    
    /*TODO: Android hates this function. It looks like it kills the main thread*/
    return;
#if 0
    for ( index = 0, cancelledCount = 0; index < THREAD_COUNT; index++ )
    {
        if ( 0 == threadExists[index].hThread )
            continue;

#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: index %d pthread_cancel", index );
#endif
        /*result = pthread_cancel( (pthread_t)(threadExists[index].hThread) );*/
        result = pthread_kill( (pthread_t)(threadExists[index].hThread), SIGKILL  );
#ifdef THREADS_DEBUG
        if ( 0 != result )
            UtvMessage( UTV_LEVEL_INFO, "THREADS: failed %d", result );
#endif
        cancelledCount++;
    }

    if ( 0 == cancelledCount )
        return;

    for ( index = 0; index < THREAD_COUNT; index++ )
    {
        if (0 == threadExists[index].hThread )
            continue;

        result = pthread_join( (pthread_t)(threadExists[index].hThread), NULL );
#ifdef THREADS_DEBUG
        if ( 0 != result )
            UtvMessage( UTV_LEVEL_INFO, "THREADS: join on %d failed %d", index, result );
        else
            UtvMessage( UTV_LEVEL_INFO, "THREADS: join on %d completed", index );
#endif
        threadExists[index].hThread = 0;
        threadExists[index].pRoutine = 0;
    }
#endif    
}

/* UtvThreadSelf
   Returns thread ID of caller's thread.
`*/
UTV_THREAD_HANDLE UtvThreadSelf( void )
{
    return (UTV_THREAD_HANDLE) pthread_self();
}

void *UtvThreadExit( void )
{
    int index, wait;
    UTV_THREAD_HANDLE hThread;
#if 1
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
	    if ( !threadBeingCreated )
		    break;
	    sleep(1);
    }

    wait = index;
#endif
    hThread = UtvThreadSelf();
#ifdef THREADS_DEBUG 
    UtvMessage( UTV_LEVEL_INFO, "THREADS: UtvThreadExit(), I am 0x%08x", hThread );
#endif
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
#ifdef THREADS_DEBUG
	    UtvMessage( UTV_LEVEL_INFO, "THREADS: test existing thread %d 0x%08x", 
			index, threadExists[index].hThread );
#endif
        if ( threadExists[index].hThread == hThread )
            break;
    }

    if ( index == THREAD_COUNT )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: unable to find UtvThreadSelf() 0x%08x",
		    hThread);
#endif
        return NULL;
    }

    /*jniDetachThread();*/
    /* set the thread as detachable so storage can be recovered without a join */
    /* This has to run regardless of whether we've found the thread or not */
    (void)pthread_detach((pthread_t)hThread);
#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: index %d wait %d pthread_exit",
                index, wait );
#endif
    threadExists[index].hThread = 0;
    threadExists[index].pRoutine = 0;
    pthread_exit(NULL);

    /* probably won't return here, but the compiler cares */
    return NULL;
}

void UtvThreadRemove( UTV_THREAD_HANDLE hThread )
{
    int index, wait;
#if 1
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
	    if ( !threadBeingCreated )
		    break;
	    sleep(1);
    }

    wait = index;
#endif
#ifdef THREADS_DEBUG 
    UtvMessage( UTV_LEVEL_INFO, "THREADS: UtvThreadRemove(), removing 0x%08x", hThread );
#endif
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
#ifdef THREADS_DEBUG
	    UtvMessage( UTV_LEVEL_INFO, "THREADS: test existing thread %d 0x%08x", 
			index, threadExists[index].hThread );
#endif
        if ( threadExists[index].hThread == hThread )
            break;
    }

    if ( index == THREAD_COUNT )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: unable to find thread 0x%08x",
		    hThread);
#endif
        return;
    }

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: index %d wait %d pthread_exit",
                index, wait );
#endif
    threadExists[index].hThread = 0;
    threadExists[index].pRoutine = 0;

    return;
}

/* UtvThreadCreate
    Creates a thread
    This code should NOT need to be changed when porting to new operating systems
    This code is optionally used in the sample STUB and FILE personality layers
  Returns OK if thread created
*/

UTV_RESULT UtvThreadCreate( UTV_THREAD_HANDLE *phThread, UTV_THREAD_START_ROUTINE pRoutine, void * pArg )
{
    int index, wait;
    int result;
    pthread_attr_t attr; 
#if 1
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
	    if ( !threadBeingCreated )
		    break;
	    sleep(1);
    }
    
    wait = index;
#endif
    threadBeingCreated = 1;
    (void)pthread_attr_init(&attr); 

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: UtvThreadCreate(), wait %d", wait );
#endif
    /* Make sure no existing threads, if any, use this Routine */
    for ( index = 0; index < THREAD_COUNT; index++ )
    {
        if ( ( threadExists[index].hThread == 0 ) ||
             ( threadExists[index].pRoutine != pRoutine) )
            continue;

#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: index %d routine already threaded",
                    index );
#endif
        return UTV_NO_THREAD;
    }

    /* find the first open spot */
    for (index = 0; index < THREAD_COUNT; index++)
    {
        if (threadExists[index].hThread == 0)
            break;
    }

    if (index == THREAD_COUNT)
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: thread limit (%d) reached",
                    THREAD_COUNT );
#endif
        return UTV_NO_THREAD;
    }

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: index %d pthread_create",
                index );
#endif
    /* Threads, all threads, are created with the non-standard detached attribute.
       They do not need to be joined to ensure that threads data is cleaned up.
    */
    (void)pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    result =  pthread_create( (pthread_t *)phThread, &attr, pRoutine, pArg );
    (void)pthread_attr_destroy(&attr); /* does nothing, could, looks more symmetric */
    if ( 0 != result )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "THREADS: failed %d",
                    result );
#endif
        return UTV_NO_THREAD;
    }

    threadExists[index].hThread = *phThread;
    threadExists[index].pRoutine = pRoutine;
#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: new thread %d 0x%08x",
		index, *phThread);
#endif
    threadBeingCreated = 0;

    return UTV_OK;
}

/* UtvThreadDestroy
    Deletes a thread
    This code should NOT need to be changed when porting to new operating systems
    This code is optionally used in the sample STUB and FILE personality layers
   Returns OK if thread destroyed
*/
UTV_RESULT UtvThreadDestroy( UTV_THREAD_HANDLE * phThread )
{
#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREADS: UtvThreadDestroy 0x%08x",  *phThread);
#endif
  if ( * phThread != 0 ) 
  {
      /* Let threads die programatically.   Enable cancel only if necessary.  */
      /* pthread_cancel( *phThread ); */
      if ( 0 != pthread_join( (pthread_t)*phThread, NULL ))
          return UTV_BAD_THREAD;
      *phThread = 0;
  }
  
  return UTV_OK;
}

/* UtvThreadSetCancelState
    A shim for pthread_setcancelstate().
    Android doesn't support this so it's a NOP.
   Returns OK if new thread state set.
*/
UTV_RESULT UtvThreadSetCancelState( UTV_INT32 iNewState, UTV_INT32 *piOldState )
{
	return UTV_OK;
}
