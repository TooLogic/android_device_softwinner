/* linux-time.cpp: UpdateTV Personality Map - Linux Time Functions

   Porting this module to another operating system should be easy if it supports
   the standard C runtime library calls. Therefore, this file will probably
   not be modified by customers.

   Copyright (c) 2007 - 2011 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      02/23/2011  Added UtvSleepNoAbort().
   Chris Nigbur    11/02/2010  Added UtvUpTime().
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Bob Taylor      10/01/2007  g_pUtvState->iDelayMilliseconds updated with
                               amount of millseconds left to sleep after abort so that
                               a supervisor program can check the master control structure
                               to determine the amount of time to sleep.
                               Addition of UtvSleeping() for single thread support.
   Bob Taylor      08/22/2007  Removed UtvLocaltime().  Moved UtvGetTickCount() and
                               UtvSleep() to here from xxxx-mutex.cpp.
   Bob Taylor      07/12/2007  Knocked out CR in UtvAsctime().
   Bob Taylor      05/01/2007  Linux only for 1.9.
   Chris Nevin     06/28/2006  Creation.
*/

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "utv.h"

/* UtvGetTickCount
   Returns milliseconds of system uptime, used for time calculations
*/
UTV_UINT32 UtvGetTickCount( void )
{
    UTV_UINT32 iTickCount;

    /* Convert the time to the tick count */
    struct timeval tvTimeval;
    gettimeofday( &tvTimeval, NULL );
    iTickCount = tvTimeval.tv_usec / 1000 + tvTimeval.tv_sec * 1000;

    /* Return the count */
    return iTickCount;
}

/* UtvSleep
    MULTI-THREADED version.  Actually invokes OS sleep as opposed to single
    threaded version.
    Sleep for the given number of milliseconds
    Uses a timed wait so that an abort can wake it up
*/
#define UTV_SLEEP_MIN_DELAY 5
void UtvSleep( UTV_UINT32 iMilliseconds )
{
    UTV_UINT32 iTimer = iMilliseconds;
    UTV_WAIT_HANDLE hWait = 0;

    /* Call to nanosleep for small values (2 msec on linux 2.4.18) will busywait. */
    /* Make sure sleep is long enough to do a context switch */
    if ( iMilliseconds < UTV_SLEEP_MIN_DELAY )
        iMilliseconds = UTV_SLEEP_MIN_DELAY;

    /* Set up storage to handle the waiting */
    UtvWaitInit( &hWait );

    /* The abort signals all the outstanding waits but our handle may not exist when the abort is signaled. */
    /* If not abort then just wait the specified time, and we don't care why we woke up */
    if ( UtvGetState() != UTV_PUBLIC_STATE_ABORT )
    {
        UtvWaitTimed( hWait, &iTimer );
    }

    /* update the amount of millseconds left to sleep so that when the system is aborted */
    /* a supervisor program can check the master control structure and find this value. */
    g_pUtvState->iDelayMilliseconds = iTimer;

    /* Delete the wait and return */
    UtvWaitDestroy( hWait );
}

/* UtvSleepNoAbort
    MULTI-THREADED version.  Actually invokes OS sleep as opposed to single
    threaded version.
    Sleep for the given number of milliseconds
    No abort interruption allowed.
*/
void UtvSleepNoAbort( UTV_UINT32 iMilliseconds )
{
    UTV_UINT32 iTimer = iMilliseconds;
    UTV_WAIT_HANDLE hWait = 0;

    /* Call to nanosleep for small values (2 msec on linux 2.4.18) will busywait. */
    /* Make sure sleep is long enough to do a context switch */
    if ( iMilliseconds < UTV_SLEEP_MIN_DELAY )
        iMilliseconds = UTV_SLEEP_MIN_DELAY;

    /* Set up storage to handle the waiting */
    UtvWaitInit( &hWait );

    /* wait the specified time */
	UtvWaitTimed( hWait, &iTimer );

    /* Delete the wait and return */
    UtvWaitDestroy( hWait );
}

#ifdef UTV_OS_SPECIAL_SINGLE_THREAD

/* UtvSleep
    SINGLE-THREADED version.  Just sets a sleep timer which UtvSleeping()
    will check to tell us when we should do something.
*/
void UtvSleep( UTV_UINT32 iMilliseconds )
{
    time ( &s_timeToWakeUp );
    s_timeToWakeUp += (iMilliseconds / 1000);
}

/* UtvSleepNoAbort
    Same as above.
*/
void UtvSleepNoAbort( UTV_UINT32 iMilliseconds )
{
    time ( &s_timeToWakeUp );
    s_timeToWakeUp += (iMilliseconds / 1000);
}

#endif

#ifdef UTV_OS_SINGLE_THREAD
/* UtvSleeping
    defined for both single and multi threaded builds for the time being to simplify version 2 testing.
    Tells caller whether previous sleep time set with UtvSleep has elapsed.
*/
static time_t s_timeToWakeUp = 0;
UTV_BOOL UtvSleeping( void )
{
    time_t tNow;

    time ( &tNow );

    return tNow <  s_timeToWakeUp;
}
#endif

/* UtvTime
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns a time_t structure that contains the new time
*/
time_t UtvTime( time_t * pTime_t )
{
    /* no local copy needed */
    return time( pTime_t );
}

/* UtvUpTime
    Calculates the seconds of device uptime using /proc/uptime.
   Returns a UTV_TIME structure containing the seconds of device uptime.
*/
UTV_TIME UtvUpTime( void )
{
  FILE* fd;
  char pszUpTime[64];
  char* pszBuf;
  int iRes;
  double dUpTimeSeconds;
  UTV_TIME tUpTime = 0;

  fd = fopen ("/proc/uptime", "r");
  if (fd != NULL)
  {
    pszBuf = fgets( pszUpTime, 64, fd );
    if ( pszBuf == pszUpTime )
    {
      /* The following sscanf must use the C locale.  */
      setlocale( LC_NUMERIC, "C" );
      iRes = sscanf( pszUpTime, "%lf", &dUpTimeSeconds );
      setlocale( LC_NUMERIC, "" );
      if ( iRes == 1 )
      {
        tUpTime = (UTV_TIME) dUpTimeSeconds;
      }
    }
  }

  fclose( fd );

  return ( tUpTime );
}
