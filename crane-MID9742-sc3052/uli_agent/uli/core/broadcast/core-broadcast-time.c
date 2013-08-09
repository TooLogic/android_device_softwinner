/* coretime.c: UpdateTV Common Core - UpdateTV Time Functions

   This module implements the UpdateTV Common Core time support functions. 

   All times are "GPS TIME" which is a few seconds different than clock time.
   The GPS epoch is 0000 UT (midnight) on January 6, 1980. GPS time is not 
   adjusted and therefore is offset from UTC by an integer number of seconds, 
   due to the insertion of leap seconds. The number remains constant until the 
   next leap second occurs. The current number of leap seconds is about 15, 
   and is contained in the STT message.

   Customers must not change UpdateTV Common Core code in order to preserve
   UpdateTV network compatibility. 

  Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      08/06/2008  Converted from C++ to C.
  Bob Taylor      08/21/2007  Made UtvConvertGpsTime() dependent on UTV_TEST_MESSAGES.
  Chris Nevin     06/28/2006  Changed calls to localtime and asctime to UtvLocaltime and UtvAsctime
  Greg Scott      10/20/2004  Creation.
*/

#include "utv.h"
#include <time.h>

/* Globals */
UTV_TIME gdtBroadcastTime = 0;
UTV_UINT32 giGpsUtcOffset = 0;

/* UtvSetBroadcastTime
    This routine sets the global time for use later
*/
UTV_RESULT UtvSetBroadcastTime( UTV_TIME dtBroadcastTime )
{
    gdtBroadcastTime = dtBroadcastTime;
    return UTV_OK;
}

/* UtvSetGpsUtcOffset
    This routine sets the leap seconds for future use
*/
UTV_RESULT UtvSetGpsUtcOffset( UTV_UINT32 iGpsUtcOffset )
{
    giGpsUtcOffset = iGpsUtcOffset;
    return UTV_OK;
}

/* UtvGetBroadcastTime
    This routine gets the local time
*/
UTV_TIME UtvGetBroadcastTime( )
{
    return gdtBroadcastTime;
}

/* UtvGetBroadcastTime
    This routine gets the local time
*/
UTV_UINT32 UtvGetGpsUtcOffset( )
{
    return giGpsUtcOffset;
}

#ifndef UTV_TEST_MESSAGES
const char * UtvConvertGpsTime( UTV_TIME dtGps )
{
    static char szTime[ 26 ];

    szTime[0] = '\0';
    return szTime;
}
#else
/* Offset from GPS time to real time
    Unix time was zero on midnight January 1, 1970
    GPS time was zero at midnight January 6, 1980
    Conversion adds 10 years, 2 days for leap years (1972, 1976) and 5 days from the 1st to the 6th
    GPS time is not adjusted by leap seconds, so GPS is now ahead of UTC by 14 seconds (giGpsUtcOffset)
*/
#define UTV_GPS_CONVERSION ( ( 60 * 60 * 24 * 365 * 10 ) + ( 60 * 60 * 24 * 7 ) )

/* UtvConvertGpsTime
    This routine is used to translate the GPS Time into a human readable time 
   Returns static pointer to string
*/
const char * UtvConvertGpsTime( UTV_TIME dtGps )
{
    static char szTime[ 26 ];
    char * pszTime;

    /* Convert time from GPS to UTC using the time constant plus the GPS UTS offset */
    time_t dtConvertedTime = (time_t)dtGps + UTV_GPS_CONVERSION + UtvGetGpsUtcOffset();

    /* Convert time to tm struct. */
    struct tm * ptmTime = UtvGmtime( &dtConvertedTime );    
    if ( ptmTime == NULL )
        return "time conversion error";
        
    pszTime = UtvAsctime( ptmTime );        

    /* Format and return the data */
    UtvStrnCopy( ( UTV_BYTE * )szTime, sizeof( szTime ), ( UTV_BYTE * )pszTime, 24 );    
    return szTime;
}
#endif

/* UtvGetSecondsToEvent
    This routine is used to calculate the number of seconds to future event
    The time is furnished in "GPS time" 
   Returns 0 if event is in the past or too far in the future
*/
UTV_UINT32 UtvGetMillisecondsToEvent( UTV_TIME dtEvent )
{
    /* Get the local time, return if not set yet */
    UTV_UINT32 iEvent = (UTV_UINT32)dtEvent;
    UTV_UINT32 iNow = (UTV_UINT32)UtvGetBroadcastTime();
    UTV_UINT32 iSeconds;

    if ( iNow == 0 ) 
        return 0;

    /* Reject if it is in the past */
    if ( iNow >= iEvent  )
        return 0;

    /* Get the number of MS in the future by subtracting the times (in seconds) */
    iSeconds = ( iEvent - iNow );

    /* Return the number of milliseconds in the future */
    return iSeconds * 1000;
}
