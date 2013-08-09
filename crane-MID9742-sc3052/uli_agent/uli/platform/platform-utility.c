/* platform-utility.c : UpdateTV Personality Map - Common utility functions

   CEM specific functions that are common to all module organizations.

   Copyright (c) 2007-2011 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   void UtvApplication( void )
   void UtvUserAbort( void )
   void UtvUserExit( void )
   void UtvYieldCPU( void )
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      07/12/2011  Added fail safe if diag stats isn't initialized.
   Bob Taylor      07/05/2011  Improved UtvDumpEvents().  Added UtvLogEventStringOneHexNum().
   Bob Taylor      06/09/2011  New uiLinesToSkip arg to UtvPlatformUtilityAssignFileToStore().
   Bob Taylor      06/05/2011  Removed setting of error due to log cats here.
   Bob Taylor      02/08/2010  Removed some platform-specific functions to the project file.
                               Added UtvPlatformGetUpdateRedirectName().
   Bob Taylor      11/13/2009  UtvPublicAgentInit() arg fixup.  Added last error logging.
   Bob Taylor      10/20/2008  UtvSetChannelScanTime() support added to UtvPublicGetDownloadSchedule().
   Bob Taylor      08/15/2008  Interactive Mode support.
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Nathan Ford     06/26/2008  Removed NAB_DEMO support
   Bob Taylor      04/08/2008  NAB Datacast Demo support.
   Bob Taylor      10/26/2007  Added UtvCheckBundleIntegrity().  Added UtvUserExit().
                               Moved UtvCrc32() to here from coreparse.cpp.
   Bob Taylor      10/16/2007  DirectDDB mode support in frequency table.
                               Moved UtvAllocateModuleBuffer() and UtvFreeModuleBuffer() to
                               mod-org-xxxx.cpp
   Bob Taylor      10/01/2007  Single threaded sleep support in UtvApplication().
   Bob Taylor      07/25/2007  Error recovery in case Execute doesn't work.  Only return restart once.
                               iSystemError.
   Bob Taylor      07/17/2007  New error count info arg.  Added UtvConcatModules().
   Bob Taylor      07/11/2007  Error history to FIFO.  New incompatibility status recording functions.
   Bob Taylor      06/22/2007  New persistent memory architecture.  New error reporting.
   Bob Taylor      05/14/2007  UTV_ABORT_TEST_THREAD changed to UTV_TEST_ABORT.
   Bob Taylor      05/08/2007  Removed static designation of UtvGetSoftwareVersion() and
                               UtvGetHardwareModel() so that test-download could use them as well.
   Bob Taylor      04/30/2007  Original
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utv.h"
#include "time.h"

/* Static Data Declarations
*/
static UTV_BOOL s_bRestartRequest = UTV_FALSE;
static char     s_szImageFileName[ 80 ];

/* Static Function Declarations
*/
static UTV_UINT32 UtvGetEventCount( UTV_UINT32 e, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 *puiEventIndex );
static void UtvInsertEventString( UTV_INT8 *pszStr, UTV_UINT32 *puiTextOffset );

/* time-related storage and formating vars */
struct tm gtmTime;

#ifndef UTV_OS_SINGLE_THREAD
/* UtvApplication
    UpdateTV application main entry point for multi-threaded implementations
    Called when the UpdateTV Agent will run on its own asynchronous thread.
    Called by the CEM's application main() function.
    Allocates the persistent storage for the application.
    Calls the module organization specific init code.
    Provides the primary loop for the UpdateTV Agent.
   Returns nothing when UtvUserExit() is called otherwise does not return.
*/
void UtvApplication( UTV_UINT32 uiNumDeliveryModes, UTV_UINT32 *pauiDeliveryModes )
{
    UTV_RESULT  rStatus;
    UTV_UINT32  iSystemError;

#ifdef UTV_TEST_ITERATION_LIMIT
    UTV_UINT32  uiIterationCounter = 0;
#endif

    do
    {
        /* Initialize all data structures and sub-systems */
        if ( UTV_OK != ( rStatus = UtvPublicAgentInit()))
            return;

		/* Establish the delivery modes that will be used during this run of the Agent. */
		if ( UTV_OK != ( rStatus = UtvConfigureDeliveryModes(  uiNumDeliveryModes, pauiDeliveryModes )))
		{
			UtvMessage( UTV_LEVEL_ERROR, "UtvConfigureDeliveryModes fails: %s", UtvGetMessageString( rStatus ));
			return;
		}

        /* Continue running the thread until the exit state is signaled */
        while ( UtvGetState() != UTV_PUBLIC_STATE_EXIT )
        {
            /* Run one task */
            UtvAgentTask( );

            /* always check for abort before sleeping */
            if ( UTV_PUBLIC_STATE_EXIT == UtvGetState() )
                break;

#ifdef UTV_TEST_ITERATION_LIMIT
            uiIterationCounter++;
            /* check to see if we've exceeded the interation limit */
            if ( uiIterationCounter >= UTV_TEST_ITERATION_LIMIT )
            {
                UtvMessage( UTV_LEVEL_INFO, "Iteration count of %d reached, exiting.\n", UTV_TEST_ITERATION_LIMIT );
                break;
            }
#endif
            /* set the diagnostic sub-state to sleeping */
            UtvSetSubState( UTV_PUBLIC_SSTATE_SLEEPING, UtvGetSleepTime() );

            /* Sleep for the specified number of seconds */
            UtvSleep( UtvGetSleepTime() );
        }

        /* set the diagnostic sub-state to shutdown */
        UtvSetSubState( UTV_PUBLIC_SSTATE_SHUTDOWN, 0 );

        UtvPublicAgentShutdown();

        /* if there isn't an execute request then just quit */
        rStatus = UTV_OK;
        if ( UtvGetRestartRequest() )
        {
            if ( UTV_OK != ( rStatus = UtvExecute( s_szImageFileName, &iSystemError ) ) )
            {
                /* if the execute failed then record it and make sure we start from */
                /* the scan state when start back up. */
                UtvSetErrorCount( rStatus, iSystemError );
                /* mutexes are dead because the system is shutdown */
                /* UtvSetState() won't work, write the state fields directly */
                g_utvAgentPersistentStorage.utvState.utvState = UTV_PUBLIC_STATE_SCAN;
                g_utvAgentPersistentStorage.utvState.utvStateWhenAborted = UTV_PUBLIC_STATE_SCAN;
                UtvPlatformPersistWrite();
                UtvMessage( UTV_LEVEL_INFO , "||==================================================||" );
                UtvMessage( UTV_LEVEL_INFO , "||= Restarting System After New Image Execute Fail =||" );
                UtvMessage( UTV_LEVEL_INFO , "||==================================================||" );
            }
        }
        /* we loop here in the case that the execute request failed */

    } while ( rStatus != UTV_OK );
}

#else

/* UtvApplication
    UpdateTV application main entry point for SINGLE-THREADED implementations.
    Called by the CEM's application main control loop.
    Maintains a state machine that executes the agent unless it's in a sleep state.
   Returns everytime after accomplishing the tasks related to one state.
*/
void UtvApplication( UTV_UINT32 uiNumDeliveryModes, UTV_UINT32 *pauiDeliveryModes )
{
    static UTV_BOOL        bSystemInitialized = UTV_FALSE;
    static UTV_BOOL        bSleeping = UTV_FALSE;
    static time_t        tStartedSleeping, tLastReport;

    UTV_RESULT          rStatus;
    UTV_UINT32          iSystemError;

#ifdef UTV_TEST_ITERATION_LIMIT
    static UTV_UINT32   uiIterationCounter = 0;
#endif

    /* Initialize all data structures and sub-systems if this hasn't been done before */
    if ( !bSystemInitialized )
    {
        bSystemInitialized = UTV_TRUE;
        UtvPublicAgentInit();

		/* Establish the delivery modes that will be used during this run of the Agent. */
		if ( UTV_OK != ( rStatus = UtvConfigureDeliveryModes(  uiNumDeliveryModes, pauiDeliveryModes )))
		{
			UtvMessage( UTV_LEVEL_ERROR, "UtvConfigureDeliveryModes fails: %s", UtvGetMessageString( rStatus ));
			return;
		}

    }

    /* if we were asked to abort then don't so anything until we're reset */
    if ( UTV_PUBLIC_STATE_EXIT == UtvGetState() )
    {
        return;
    }

    /* If we're supposed to be sleeping then quit. */
    if ( UtvSleeping() )
    {
        if ( !bSleeping )
        {
            bSleeping = UTV_TRUE;
            UtvMessage( UTV_LEVEL_INFO, "Beginning to sleep" );
            time( &tStartedSleeping );
            time( &tLastReport );
        } else
        {
            time_t tNow;
            time( &tNow );

            if ( (int) difftime( tNow, tLastReport ) > 10 )
            {
                time( &tLastReport );
                UtvMessage( UTV_LEVEL_INFO, "Have been asleep for %d seconds", (int) difftime( tNow, tStartedSleeping ) );
            }
        }
        return;
    } else
    {
        bSleeping = UTV_FALSE;
    }

    /* Run one task */
    UtvAgentTask( );

#ifdef UTV_TEST_ITERATION_LIMIT
    uiIterationCounter++;
    /* check to see if we've exceeded the interation limit */
    if ( uiIterationCounter >= UTV_TEST_ITERATION_LIMIT )
    {
        UtvMessage( UTV_LEVEL_INFO, "Iteration count of %d reached, exiting.\n", UTV_TEST_ITERATION_LIMIT );
        UtvUserAbort();
    }
#endif

    /* if we were asked to exit then don't set sleep time */
    /* and check for a restart request. */
    if ( UTV_PUBLIC_STATE_EXIT == UtvGetState() )
    {
        /* set the diagnostic sub-state to shutdown */
        UtvSetSubState( UTV_PUBLIC_SSTATE_SHUTDOWN, 0 );

        UtvPublicAgentShutdown();

        bSystemInitialized = UTV_FALSE;

        /* if there isn't an execute request then just quite */
        rStatus = UTV_OK;
        if ( UtvGetRestartRequest() )
        {
            if ( UTV_OK != ( rStatus = UtvExecute( s_szImageFileName, &iSystemError ) ) )
            {
                /* if the execute failed then record it and make sure we start from */
                /* the scan state when start back up. */
                UtvSetErrorCount( rStatus, iSystemError );
                /* mutexes are dead because the system is shutdown */
                /* UtvSetState() won't work, write the state fields directly */
                g_utvAgentPersistentStorage.utvState.utvState = UTV_PUBLIC_STATE_SCAN;
                g_utvAgentPersistentStorage.utvState.utvStateWhenAborted = UTV_PUBLIC_STATE_SCAN;
                UtvPlatformPersistWrite();
                UtvMessage( UTV_LEVEL_INFO , "||==================================================||" );
                UtvMessage( UTV_LEVEL_INFO , "||= Restarting System After New Image Execute Fail =||" );
                UtvMessage( UTV_LEVEL_INFO , "||==================================================||" );
            }
        }
        return;
    } else
    {
        /* Agent exited normally.  Set sleep time. */

        /* set the diagnostic sub-state to sleeping */
        UtvSetSubState( UTV_PUBLIC_SSTATE_SLEEPING, UtvGetSleepTime() );

        /* Set the sleep timer for the specified number of seconds */
        UtvSleep( UtvGetSleepTime() );
    }

}
#endif

/* UtvGetPid
    This routine returns a PID constant if the PID decision has been
    made by a mechanism other than a PSI scan.
*/
UTV_RESULT UtvGetPid (
    UTV_UINT32 * piPid )
{
    /* assume failure */
    *piPid = 0;

    return UTV_OK;
}

#ifdef  UTV_OS_SINGLE_THREAD
/* UtvYieldCPU
    Called during lengthy operations to allow a single threaded OS time to perform
    housekeeping issues.  This is only included for platforms that have
    UTV_OS_SINGLE_THREAD enabled.  This will not be called during time critial functions.
*/
void UtvYieldCPU( void )
{

}
#endif

/* UtvUserAbort entry point
    Called when the UpdateTV Agent must cancel any scan or download in progress and revert to
    to a sleep until
    This occurs when the user turns on the TV or cancels an update in such support exists.
    NOTE: Be sure any loops or lengthy operations in the CEM's pers adaptation layer checks for the
    abort state periodically so the abort happens as quickly as possible
*/
void UtvUserAbort( void )
{
    /* Returned status */
    UTV_RESULT rStatus = UTV_OK;

    /* The CEM may need to perform some type of platform specific clean up at this point */

    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "User Abort requested" );

    /* Make the abort request */
    if (UTV_OK != ( rStatus = UtvAbort()))
    {
        UtvSetErrorCount( rStatus, 0 );
        UtvMessage( UTV_LEVEL_ERROR, "UtvAbort - Error returned - %s\n\n", UtvGetMessageString( rStatus ) );
    }
}

/* UtvUserExit entry point
    Called when the the UpdateTV Agent needs to be forced to quit.
    Just calls UtvExit() but allows the user to perform
    platform-specific cleanup.
*/
void UtvUserExit( void )
{
    /* Returned status */
    UTV_RESULT rStatus = UTV_OK;

    /* The CEM may need to perform some type of platform specific clean up at this point */

    UtvMessage( UTV_LEVEL_TRACE | UTV_LEVEL_TEST, "User Exit requested" );

    /* Make the abort request */
    if (UTV_OK != ( rStatus = UtvExit()))
    {
        UtvSetErrorCount( rStatus, 0 );
        UtvMessage( UTV_LEVEL_ERROR, "UtvExit - Error returned - %s\n\n", UtvGetMessageString( rStatus ) );
    }
}

/* This is the CRC32 table generates with the "forward" polynomial (0x04c11db7)
*/
static const UTV_UINT32 UtvCrc32Table[ 256 ] =
{
    0x00000000UL, 0x04c11db7UL, 0x09823b6eUL, 0x0d4326d9UL,
    0x130476dcUL, 0x17c56b6bUL, 0x1a864db2UL, 0x1e475005UL,
    0x2608edb8UL, 0x22c9f00fUL, 0x2f8ad6d6UL, 0x2b4bcb61UL,
    0x350c9b64UL, 0x31cd86d3UL, 0x3c8ea00aUL, 0x384fbdbdUL,
    0x4c11db70UL, 0x48d0c6c7UL, 0x4593e01eUL, 0x4152fda9UL,
    0x5f15adacUL, 0x5bd4b01bUL, 0x569796c2UL, 0x52568b75UL,
    0x6a1936c8UL, 0x6ed82b7fUL, 0x639b0da6UL, 0x675a1011UL,
    0x791d4014UL, 0x7ddc5da3UL, 0x709f7b7aUL, 0x745e66cdUL,
    0x9823b6e0UL, 0x9ce2ab57UL, 0x91a18d8eUL, 0x95609039UL,
    0x8b27c03cUL, 0x8fe6dd8bUL, 0x82a5fb52UL, 0x8664e6e5UL,
    0xbe2b5b58UL, 0xbaea46efUL, 0xb7a96036UL, 0xb3687d81UL,
    0xad2f2d84UL, 0xa9ee3033UL, 0xa4ad16eaUL, 0xa06c0b5dUL,
    0xd4326d90UL, 0xd0f37027UL, 0xddb056feUL, 0xd9714b49UL,
    0xc7361b4cUL, 0xc3f706fbUL, 0xceb42022UL, 0xca753d95UL,
    0xf23a8028UL, 0xf6fb9d9fUL, 0xfbb8bb46UL, 0xff79a6f1UL,
    0xe13ef6f4UL, 0xe5ffeb43UL, 0xe8bccd9aUL, 0xec7dd02dUL,
    0x34867077UL, 0x30476dc0UL, 0x3d044b19UL, 0x39c556aeUL,
    0x278206abUL, 0x23431b1cUL, 0x2e003dc5UL, 0x2ac12072UL,
    0x128e9dcfUL, 0x164f8078UL, 0x1b0ca6a1UL, 0x1fcdbb16UL,
    0x018aeb13UL, 0x054bf6a4UL, 0x0808d07dUL, 0x0cc9cdcaUL,
    0x7897ab07UL, 0x7c56b6b0UL, 0x71159069UL, 0x75d48ddeUL,
    0x6b93dddbUL, 0x6f52c06cUL, 0x6211e6b5UL, 0x66d0fb02UL,
    0x5e9f46bfUL, 0x5a5e5b08UL, 0x571d7dd1UL, 0x53dc6066UL,
    0x4d9b3063UL, 0x495a2dd4UL, 0x44190b0dUL, 0x40d816baUL,
    0xaca5c697UL, 0xa864db20UL, 0xa527fdf9UL, 0xa1e6e04eUL,
    0xbfa1b04bUL, 0xbb60adfcUL, 0xb6238b25UL, 0xb2e29692UL,
    0x8aad2b2fUL, 0x8e6c3698UL, 0x832f1041UL, 0x87ee0df6UL,
    0x99a95df3UL, 0x9d684044UL, 0x902b669dUL, 0x94ea7b2aUL,
    0xe0b41de7UL, 0xe4750050UL, 0xe9362689UL, 0xedf73b3eUL,
    0xf3b06b3bUL, 0xf771768cUL, 0xfa325055UL, 0xfef34de2UL,
    0xc6bcf05fUL, 0xc27dede8UL, 0xcf3ecb31UL, 0xcbffd686UL,
    0xd5b88683UL, 0xd1799b34UL, 0xdc3abdedUL, 0xd8fba05aUL,
    0x690ce0eeUL, 0x6dcdfd59UL, 0x608edb80UL, 0x644fc637UL,
    0x7a089632UL, 0x7ec98b85UL, 0x738aad5cUL, 0x774bb0ebUL,
    0x4f040d56UL, 0x4bc510e1UL, 0x46863638UL, 0x42472b8fUL,
    0x5c007b8aUL, 0x58c1663dUL, 0x558240e4UL, 0x51435d53UL,
    0x251d3b9eUL, 0x21dc2629UL, 0x2c9f00f0UL, 0x285e1d47UL,
    0x36194d42UL, 0x32d850f5UL, 0x3f9b762cUL, 0x3b5a6b9bUL,
    0x0315d626UL, 0x07d4cb91UL, 0x0a97ed48UL, 0x0e56f0ffUL,
    0x1011a0faUL, 0x14d0bd4dUL, 0x19939b94UL, 0x1d528623UL,
    0xf12f560eUL, 0xf5ee4bb9UL, 0xf8ad6d60UL, 0xfc6c70d7UL,
    0xe22b20d2UL, 0xe6ea3d65UL, 0xeba91bbcUL, 0xef68060bUL,
    0xd727bbb6UL, 0xd3e6a601UL, 0xdea580d8UL, 0xda649d6fUL,
    0xc423cd6aUL, 0xc0e2d0ddUL, 0xcda1f604UL, 0xc960ebb3UL,
    0xbd3e8d7eUL, 0xb9ff90c9UL, 0xb4bcb610UL, 0xb07daba7UL,
    0xae3afba2UL, 0xaafbe615UL, 0xa7b8c0ccUL, 0xa379dd7bUL,
    0x9b3660c6UL, 0x9ff77d71UL, 0x92b45ba8UL, 0x9675461fUL,
    0x8832161aUL, 0x8cf30badUL, 0x81b02d74UL, 0x857130c3UL,
    0x5d8a9099UL, 0x594b8d2eUL, 0x5408abf7UL, 0x50c9b640UL,
    0x4e8ee645UL, 0x4a4ffbf2UL, 0x470cdd2bUL, 0x43cdc09cUL,
    0x7b827d21UL, 0x7f436096UL, 0x7200464fUL, 0x76c15bf8UL,
    0x68860bfdUL, 0x6c47164aUL, 0x61043093UL, 0x65c52d24UL,
    0x119b4be9UL, 0x155a565eUL, 0x18197087UL, 0x1cd86d30UL,
    0x029f3d35UL, 0x065e2082UL, 0x0b1d065bUL, 0x0fdc1becUL,
    0x3793a651UL, 0x3352bbe6UL, 0x3e119d3fUL, 0x3ad08088UL,
    0x2497d08dUL, 0x2056cd3aUL, 0x2d15ebe3UL, 0x29d4f654UL,
    0xc5a92679UL, 0xc1683bceUL, 0xcc2b1d17UL, 0xc8ea00a0UL,
    0xd6ad50a5UL, 0xd26c4d12UL, 0xdf2f6bcbUL, 0xdbee767cUL,
    0xe3a1cbc1UL, 0xe760d676UL, 0xea23f0afUL, 0xeee2ed18UL,
    0xf0a5bd1dUL, 0xf464a0aaUL, 0xf9278673UL, 0xfde69bc4UL,
    0x89b8fd09UL, 0x8d79e0beUL, 0x803ac667UL, 0x84fbdbd0UL,
    0x9abc8bd5UL, 0x9e7d9662UL, 0x933eb0bbUL, 0x97ffad0cUL,
    0xafb010b1UL, 0xab710d06UL, 0xa6322bdfUL, 0xa2f33668UL,
    0xbcb4666dUL, 0xb8757bdaUL, 0xb5365d03UL, 0xb1f740b4UL
};

/* UtvCrc32
    Forward CRC-32 polynomial use generator 0x04C11DB7
    x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0
*/
UTV_UINT32 UtvCrc32( const UTV_BYTE * pBuffer, UTV_UINT32 uiSize )
{
    UTV_UINT32 uiCrc = 0xffffffff;
    const UTV_BYTE * p;

    for ( p = pBuffer; uiSize > 0; ++p, --uiSize )
        uiCrc = (uiCrc << 8) ^ UtvCrc32Table[(uiCrc >> 24) ^ *p];
    return uiCrc;
}

/* UtvCrc32Continue - to handle disjointed buffers; pass in CRC from previous buffer
   to continue on; it is otherwise identical to the above if uiCRC is set to 0xFFFFFFFF
*/
UTV_UINT32 UtvCrc32Continue( const UTV_BYTE * pBuffer, UTV_UINT32 uiSize, UTV_UINT32 uiCrc )
{
    const UTV_BYTE * p;

    for ( p = pBuffer; uiSize > 0; ++p, --uiSize )
        uiCrc = (uiCrc << 8) ^ UtvCrc32Table[(uiCrc >> 24) ^ *p];
    return uiCrc;
}

static void UtvLogEventWorker( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_EARG aArg[] );

/* UtvSetErrorCount
    Tracks the errors that occur by UTV_RESULT type to allow summary to be displayed
    This routine will eventually be replaced by UtvLogEvent.  It's not being replaced now
    to minimize code disturbance.  Almost all of its use is in the broadcast related code.
*/
void UtvSetErrorCount( UTV_RESULT utvError, UTV_UINT32 iInfo )
{
    UtvLogEventNoLineOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, utvError, iInfo );
    return;
}

#ifdef UTV_DEBUG
void UtvDumpEvents( void )
{
    UTV_UINT32 c, e, a;
	UTV_INT8   acBuffer[ 128 ];

	if ( NULL == g_pUtvDiagStats )
		return;

    for ( c = 0; c < UTV_PLATFORM_EVENT_CLASS_COUNT; c++ )
	{
		for ( e = 0; e < g_pUtvDiagStats->utvEvents[ c ].uiCount; e++ )
		{
			UtvCTime( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].tTime,
					  acBuffer, sizeof(acBuffer ));
			acBuffer[ UtvStrlen( (UTV_BYTE *)acBuffer ) - 1 ] = 0;

            UtvMessage( UTV_LEVEL_INFO, "LOG: 0x%X, Cat: %s, Msg: %s, Cnt: %lu, %s",
						g_pUtvDiagStats->utvEvents[ c ].uiClassID,
						UtvGetCategoryString( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].rCat ),
						UtvGetMessageString( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].rMsg ),
						g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].uiCount,
						acBuffer);
			for ( a = 0; a < UTV_PLATFORM_ARGS_PER_EVENT; a++ )
			{
				switch ( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].eArgType )
				{
				case UTV_EARG_LINE:
					UtvMessage( UTV_LEVEL_INFO, "    Line: %d", 
								g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].uiArg );
					break;
				case UTV_EARG_STRING_OFFSET:
					UtvMessage( UTV_LEVEL_INFO, "    String: %s", 
								UtvLogConvertTextOffset( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].uiArg ));
					break;
				case UTV_EARG_TIME:
					UtvCTime( g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].uiArg,
							  acBuffer, sizeof(acBuffer ));
					UtvMessage( UTV_LEVEL_INFO, "    Time: %s", 
								acBuffer );
					break;
				case UTV_EARG_HEXNUM:
					UtvMessage( UTV_LEVEL_INFO, "    Value: 0x%08X", 
								g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].uiArg );
					break;
				case UTV_EARG_DECNUM:
					UtvMessage( UTV_LEVEL_INFO, "    Value: %lu", 
								g_pUtvDiagStats->utvEvents[ c ].aEvent[ e ].eArg[ a ].uiArg );
					break;
				default: /* everything else like not used is not displayed */
					break;
				}
			}	
		}
	}
}
#endif

/* UtvGetEventCount
    Get the event count for a particular type of event
*/
static UTV_UINT32 UtvGetEventCount( UTV_UINT32 e, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 *puiEventIndex )
{
    UTV_UINT32 i;

	if ( NULL == g_pUtvDiagStats )
		return 0;

    /* search through the queue for count number of events of the given type looking for a match.
       return the count if found, zero otherwise */
    for ( i = 0; i < g_pUtvDiagStats->utvEvents[ e ].uiCount; i++ )
        if ( g_pUtvDiagStats->utvEvents[ e ].aEvent[ i ].rCat == rCat && g_pUtvDiagStats->utvEvents[ e ].aEvent[ i ].rMsg == rMsg )
            break;

    if ( i < g_pUtvDiagStats->utvEvents[ e ].uiCount )
    {
        *puiEventIndex = i;
        return g_pUtvDiagStats->utvEvents[ e ].aEvent[ i ].uiCount;
    }

    return 0;
}

/* UtvInsertEventString
    inserts event strings into persistent memory wrapping and blowing away old strings as necessary.
*/
static void UtvInsertEventString( UTV_INT8 *pszStr, UTV_UINT32 *puiTextOffset )
{
    UTV_UINT32 uiStrLen = UtvStrlen( (UTV_BYTE *) pszStr );
    UTV_UINT32 uiStoreLeft;

	if ( NULL == g_pUtvDiagStats )
		return;

    uiStoreLeft = (UTV_PLATFORM_EVENT_TEXT_STORE_SIZE - g_pUtvDiagStats->uiTextOffset);

    /* if string is larger than max string size then crop it */
    if ( uiStrLen > UTV_PLATFORM_EVENT_TEXT_MAX_STR_SIZE )
        uiStrLen = UTV_PLATFORM_EVENT_TEXT_MAX_STR_SIZE;

    /* if there enough room in the text block to insert this string? */
    if ( (uiStrLen + 1) <= uiStoreLeft )
    {
        /* then just insert it */
        UtvStrnCopy( (UTV_BYTE *) &g_pUtvDiagStats->cTextStore[g_pUtvDiagStats->uiTextOffset], uiStoreLeft, (UTV_BYTE *) pszStr, uiStrLen );
    } else
    {
        /* text store is full, wrap and zap */
        /* string will get trimmed by UtvStrnCopy if need be */
        g_pUtvDiagStats->uiTextOffset = 0;
        UtvStrnCopy( (UTV_BYTE *) &g_pUtvDiagStats->cTextStore[g_pUtvDiagStats->uiTextOffset], UTV_PLATFORM_EVENT_TEXT_STORE_SIZE, (UTV_BYTE *) pszStr, uiStrLen );
    }

    *puiTextOffset = g_pUtvDiagStats->uiTextOffset;
    g_pUtvDiagStats->uiTextOffset += uiStrLen + 1;

    /* handle end of buffer case */
    if ( g_pUtvDiagStats->uiTextOffset >= UTV_PLATFORM_EVENT_TEXT_STORE_SIZE )
        g_pUtvDiagStats->uiTextOffset = 0;
}

void UtvLogEvent( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;
    aArg[1].eArgType = UTV_EARG_NOT_USED;
    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventNoLine( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    aArg[0].eArgType = UTV_EARG_NOT_USED;
    aArg[1].eArgType = UTV_EARG_NOT_USED;
    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventNoLineOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiV1 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_DECNUM;
    aArg[0].uiArg = uiV1;

    aArg[1].eArgType = UTV_EARG_NOT_USED;
    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_DECNUM;
    aArg[1].uiArg = uiV1;

    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventTwoDecNums( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1, UTV_UINT32 uiV2 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_DECNUM;
    aArg[1].uiArg = uiV1;

    aArg[2].eArgType = UTV_EARG_DECNUM;
    aArg[2].uiArg = uiV2;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventString( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_STRING_PTR;
    aArg[1].uiArg = (UTV_UINT32) pszString;

    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventStringOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString, UTV_UINT32 uiV1 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_STRING_PTR;
    aArg[1].uiArg = (UTV_UINT32) pszString;

    aArg[2].eArgType = UTV_EARG_DECNUM;
    aArg[2].uiArg = uiV1;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventStringOneHexNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString, UTV_UINT32 uiV1 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_STRING_PTR;
    aArg[1].uiArg = (UTV_UINT32) pszString;

    aArg[2].eArgType = UTV_EARG_HEXNUM;
    aArg[2].uiArg = uiV1;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventOneHexNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_HEXNUM;
    aArg[1].uiArg = uiV1;

    aArg[2].eArgType = UTV_EARG_NOT_USED;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventTwoHexNums( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1, UTV_UINT32 uiV2 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_HEXNUM;
    aArg[1].uiArg = uiV1;

    aArg[2].eArgType = UTV_EARG_HEXNUM;
    aArg[2].uiArg = uiV2;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

void UtvLogEventTimes( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_TIME tT1, UTV_TIME tT2 )
{
    UTV_EARG    aArg[UTV_PLATFORM_ARGS_PER_EVENT];

    /* convert the numeric args to multi-type args */
    aArg[0].eArgType = UTV_EARG_LINE;
    aArg[0].uiArg = uiLine;

    aArg[1].eArgType = UTV_EARG_TIME;
    aArg[1].uiArg = (UTV_UINT32) tT1;

    aArg[2].eArgType = UTV_EARG_TIME;
    aArg[2].uiArg = (UTV_UINT32) tT2;

    UtvLogEventWorker( uiLevel, rCat, rMsg, aArg );
}

/* UtvLogEventWorker
    Local function that does all the event posting/logging.
*/
static void UtvLogEventWorker( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_EARG aArg[] )
{
    UTV_UINT32    e, n, a;
#ifdef UTV_DEBUG
    UTV_INT8 cFmtString[ 128 ];
    UTV_INT8 cTime[UTV_PLATFORM_ARGS_PER_EVENT][32];
#endif

/* if in debug mode display this to console */
#ifdef UTV_DEBUG
    UtvStrnCopy( (UTV_BYTE *) cFmtString, sizeof(cFmtString), (UTV_BYTE *) "Cat: \"%s\", Msg: \"%s\", ", 27 );

    for ( a = 0; a < UTV_PLATFORM_ARGS_PER_EVENT; a++ )
    {
        switch ( aArg[ a ].eArgType )
        {
        case UTV_EARG_LINE:
            UtvStrcat( (UTV_BYTE *) cFmtString, (UTV_BYTE *) "Line: %d " );
            break;
        case UTV_EARG_STRING_PTR:
        case UTV_EARG_TIME:
            UtvStrcat( (UTV_BYTE *) cFmtString, (UTV_BYTE *) "\"%s\" " );
            break;
        case UTV_EARG_HEXNUM:
            UtvStrcat( (UTV_BYTE *) cFmtString, (UTV_BYTE *) "0x%08X " );
            break;
        case UTV_EARG_DECNUM:
            UtvStrcat( (UTV_BYTE *) cFmtString, (UTV_BYTE *) "%lu " );
            break;
        default: /* everything else like not used is not displayed */
            break;
        }
    }

    UtvMessage( uiLevel, cFmtString, UtvGetCategoryString( rCat ), UtvGetMessageString( rMsg ),
        aArg[ 0 ].eArgType == UTV_EARG_TIME ? UtvCTime( aArg[ 0 ].uiArg, cTime[ 0 ], sizeof( cTime[ 0 ] ) ) : (UTV_INT8 *) aArg[ 0 ].uiArg,
        aArg[ 1 ].eArgType == UTV_EARG_TIME ? UtvCTime( aArg[ 1 ].uiArg, cTime[ 1 ], sizeof( cTime[ 1 ] ) ) : (UTV_INT8 *) aArg[ 1 ].uiArg,
        aArg[ 2 ].eArgType == UTV_EARG_TIME ? UtvCTime( aArg[ 2 ].uiArg, cTime[ 2 ], sizeof( cTime[ 2 ] ) ) : (UTV_INT8 *) aArg[ 2 ].uiArg );
#endif

    /* if configured for internet delivery then send this message to the NOC */
#ifdef UTV_DELIVERY_INTERNET
    UtvInternetLogEventFiltered( uiLevel, rCat, rMsg, aArg );
#endif

    /* msgs to ignore are filtered out here */
    if ( UTV_OK == rMsg )
        return;

    /* look to see if we're logging events of this type */
    if ( 0 == (uiLevel & UTV_PLATFORM_EVENT_CLASS_MASK))
        return;

	/* if diag stats isn't intialized then punt */
	if ( NULL == g_pUtvDiagStats )
		return;

    /* look up the logging entry for this event type */
    for ( e = 0; e < UTV_PLATFORM_EVENT_CLASS_COUNT; e++ )
        if ( g_pUtvDiagStats->utvEvents[ e ].uiClassID == uiLevel )
            break;

    /* if not found then there's really no one to complain to, this routine cannot create errors */
    if ( e >= UTV_PLATFORM_EVENT_CLASS_COUNT )
        return;
	
    /* If this type of event has been recorded before.  Just bump the count and set the values, string, and time fields */
    if ( 0 != UtvGetEventCount( e, rCat, rMsg, &n ) )
    {
        g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].uiCount++;

        /* count in queue doesn't change and next doesn't change */
    } else
    {
        /* else, it hasn't been recorded before.  Add it, set its count to one */
        n = g_pUtvDiagStats->utvEvents[ e ].uiNext;
        g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].uiCount = 1;

        /* if more space in queue then let the bump count otherwise leave at full */
        if ( g_pUtvDiagStats->utvEvents[ e ].uiCount < UTV_PLATFORM_EVENTS_PER_CLASS )
            g_pUtvDiagStats->utvEvents[ e ].uiCount++;

        g_pUtvDiagStats->utvEvents[ e ].uiNext++;

        if ( UTV_PLATFORM_EVENTS_PER_CLASS <= g_pUtvDiagStats->utvEvents[ e ].uiNext )
            g_pUtvDiagStats->utvEvents[ e ].uiNext = 0;
    }

    g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].rCat = rCat;
    g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].rMsg = rMsg;
    g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].tTime = UtvTime( NULL );

    /* store args, convert string args to offsets */
    for ( a = 0; a < UTV_PLATFORM_ARGS_PER_EVENT; a++ )
    {
        if ( UTV_EARG_STRING_PTR == aArg[a].eArgType )
        {
            g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].eArg[a].eArgType = UTV_EARG_STRING_OFFSET;
            UtvInsertEventString( (UTV_INT8 *)aArg[a].uiArg, &g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].eArg[a].uiArg );
        } else
        {
            g_pUtvDiagStats->utvEvents[ e ].aEvent[ n ].eArg[a] = aArg[a];
        }
    }
}

void UtvLogClearEventLog( UTV_UINT32 uiLevel )
{
    UTV_UINT32    e;

	/* if diag stats isn't intialized then punt */
	if ( NULL == g_pUtvDiagStats )
		return;

    /* look to see if we're logging events of this type */
    if ( 0 == (uiLevel & UTV_PLATFORM_EVENT_CLASS_MASK))
        return;

    /* look up the logging entry for this event type */
    for ( e = 0; e < UTV_PLATFORM_EVENT_CLASS_COUNT; e++ )
        if ( g_pUtvDiagStats->utvEvents[ e ].uiClassID == uiLevel )
            break;

    /* if not found then there's really no one to complain to, this routine cannot create errors */
    if ( e >= UTV_PLATFORM_EVENT_CLASS_COUNT )
        return;

    /* zap event count and we're done */
    g_pUtvDiagStats->utvEvents[ e ].uiCount = 0;
    g_pUtvDiagStats->utvEvents[ e ].uiNext = 0;

}

UTV_UINT32 UtvLogGetEventClassCount( UTV_UINT32 uiLevel, UTV_UINT32 *puiLast )
{
    UTV_UINT32    e;

	/* if diag stats isn't intialized then punt */
	if ( NULL == g_pUtvDiagStats )
		return 0;

    /* look to see if we're logging events of this type */
    if ( 0 == (uiLevel & UTV_PLATFORM_EVENT_CLASS_MASK))
        return 0;

    /* look up the logging entry for this event type */
    for ( e = 0; e < UTV_PLATFORM_EVENT_CLASS_COUNT; e++ )
        if ( g_pUtvDiagStats->utvEvents[ e ].uiClassID == uiLevel )
            break;

    /* if not found then there's really no one to complain to, this routine cannot create errors */
    if ( e >= UTV_PLATFORM_EVENT_CLASS_COUNT )
        return 0;

    if ( 0 == g_pUtvDiagStats->utvEvents[ e ].uiNext )
        *puiLast = UTV_PLATFORM_EVENT_CLASS_COUNT-1;
    else
        *puiLast = g_pUtvDiagStats->utvEvents[ e ].uiNext-1;

    return g_pUtvDiagStats->utvEvents[ e ].uiCount;
}

UTV_UINT32 UtvLogGetStoredEventClassMask( void )
{
    UTV_UINT32    uiLevel, i, uiLast, uiMask = 0;

    /* Create a mask for all stored event types */
    for ( uiLevel = 1, i = 0; i < UTV_LEVEL_COUNT; i++, uiLevel <<= 1 )
    {
        if ( UtvLogGetEventClassCount( uiLevel, &uiLast ) )
            uiMask |= uiLevel;
    }

    return uiMask;
}

UTV_INT8 *UtvLogConvertTextOffset( UTV_UINT32 uiOffset )
{
    if ( UTV_TEXT_OFFSET_INVALID == uiOffset )
        return NULL;

    if ( uiOffset >= UTV_PLATFORM_EVENT_TEXT_STORE_SIZE )
        return NULL;

	/* if diag stats isn't intialized then punt */
	if ( NULL == g_pUtvDiagStats )
		return NULL;

    return &g_pUtvDiagStats->cTextStore[uiOffset];
}

UTV_EVENT_ENTRY *UtvLogGetEvent( UTV_UINT32 uiLevel, UTV_UINT32 uiIndex )
{
    UTV_UINT32    e;

    /* look to see if we're logging events of this type */
    if ( 0 == (uiLevel & UTV_PLATFORM_EVENT_CLASS_MASK))
        return NULL;

	/* if diag stats isn't intialized then punt */
	if ( NULL == g_pUtvDiagStats )
		return NULL;

    /* look up the logging entry for this event type */
    for ( e = 0; e < UTV_PLATFORM_EVENT_CLASS_COUNT; e++ )
        if ( g_pUtvDiagStats->utvEvents[ e ].uiClassID == uiLevel )
            break;

    /* if not found then there's really no one to complain to, this routine cannot create errors */
    if ( e >= UTV_PLATFORM_EVENT_CLASS_COUNT )
        return NULL;

    /* check index */
    if ( uiIndex >= g_pUtvDiagStats->utvEvents[ e ].uiCount )
        return NULL;

    return &g_pUtvDiagStats->utvEvents[ e ].aEvent[ uiIndex ];

}

/* UtvSetRestartRequest
    Sets a flag that is interpreted by the system as a restart request and issues an abort.
*/
void UtvSetRestartRequest( )
{
    s_bRestartRequest = UTV_TRUE;
    UtvExit();
}

/* UtvGetRestartRequest
    Returns the value of s_bRestartRequest
*/
UTV_BOOL UtvGetRestartRequest( )
{
    UTV_BOOL    bRequest = s_bRestartRequest;

    /* only return restart once */
    s_bRestartRequest = UTV_FALSE;
    return bRequest;
}

/* UtvStoreModuleAsFile
    This routine stores a module as a file for test purposes.  In a real system you would be
    accumulating modules in flash memory and then aggregating them to form the updated software
    or data image.
*/
UTV_RESULT UtvStoreModuleAsFile( UTV_BYTE *pModuleData, UTV_UINT32 iModuleSize, char *pszModuleName, UTV_UINT32 iModuleId )
{
    char         szModuleFileName[ 80 ];
    UTV_RESULT   rResult;
    FILE       * f;

    /* Determine which filename to use */
    if ( pszModuleName != NULL )
        sprintf( szModuleFileName, "%s", pszModuleName );
    else
        sprintf( szModuleFileName, "module%lu.module", iModuleId );

#ifdef UTV_TEST_ABORT
    UtvAbortEnterSysCall();
#endif  /* UTV_TEST_ABORT */

    /* Open file, write data, close file */
    if ( NULL == (f = fopen( szModuleFileName, "wb")) )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Unable to open file \"%s\" for module write", szModuleFileName );
        rResult = UTV_MODULE_STORE_FAIL;
    } else
    {
        fwrite( pModuleData, iModuleSize, 1, f );
        fclose( f );
        UtvMessage( UTV_LEVEL_INFO, "Stored %d bytes module ID %u to file \"%s\"", iModuleSize, iModuleId, pszModuleName );
        rResult = UTV_OK;
    }
#ifdef UTV_TEST_ABORT
    UtvAbortExitSysCall();
#endif  /* UTV_TEST_ABORT */

    return rResult;
}

/* UtvConcatModules
    Concatenate the modules previously written by UtvStoreModuleAsFile().
    Give the resulting image file the module base name with the downloaded module version as an extension.
*/
UTV_RESULT UtvConcatModules( char *pszModuleBaseName, UTV_UINT32 iSignalledCount, UTV_UINT32 iModVer )
{
    char         szModuleFileName[ 80 ];
    UTV_BYTE     b;
    UTV_UINT32     i;
    FILE      * fOut, * fIn;

#ifdef UTV_TEST_ABORT
    UtvAbortEnterSysCall();
#endif  /* UTV_TEST_ABORT */

    sprintf( s_szImageFileName, "%s.%lu", pszModuleBaseName, iModVer );

    /* Open image for write */
    if ( NULL == ( fOut = fopen( s_szImageFileName, "wb" )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "Unable to open file \"%s\" for image write", s_szImageFileName );
        return UTV_MODULE_STORE_FAIL;
    }

    if ( 0 == iSignalledCount )
    {
        /* When iSignalledCount equals zero only one image, no XXofYY suffix */
        sprintf( szModuleFileName, "%s", pszModuleBaseName );
        if ( NULL == ( fIn = fopen( szModuleFileName, "rb" )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "Unable to open file \"%s\" for module read", szModuleFileName );
            return UTV_MODULE_STORE_FAIL;
        }

        while ( 1 == fread( &b, 1, 1, fIn ) )
            fwrite( &b, 1, 1, fOut );

        fclose( fIn );
    } else
    {
        /* Iterate through the stored module files assuming that they adhere to the
           ModuleBaseName0Xof0Y naming protocol */
        for ( i = 1; i <= iSignalledCount; i++ )
        {
            sprintf( szModuleFileName, "%s%02luof%02lu", pszModuleBaseName, i, iSignalledCount );
            if ( NULL == ( fIn = fopen( szModuleFileName, "rb" )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "Unable to open file \"%s\" for module read", szModuleFileName );
                return UTV_MODULE_STORE_FAIL;
            }

            while ( 1 == fread( &b, 1, 1, fIn ) )
                fwrite( &b, 1, 1, fOut );

            fclose( fIn );
        }
    }

    fclose( fOut );

#ifdef UTV_TEST_ABORT
    UtvAbortExitSysCall();
#endif  /* UTV_TEST_ABORT */

    return UTV_OK;
}

/* UtvPlatformCommonAcquireHardware
    For each delivery method attempt to acquire the hardware associated with it.
*/
UTV_RESULT UtvPlatformCommonAcquireHardware( UTV_UINT32 iEventType )
{
    UTV_RESULT    rStatus = UTV_OK, rFinalStatus = UTV_UNKNOWN;
    UTV_UINT32    i;

    if ( 0 == g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes )
    {
        rFinalStatus = UTV_NO_DELIVERY_MODES;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformCommonAcquireHardware() fails: %s", UtvGetMessageString( rStatus ) );
        return rFinalStatus;
    }

    for ( i = 0; i < g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes; i++ )
    {
        if ( g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess )
        {
            /* if it's already open there's nothing to do. */
            UtvMessage( UTV_LEVEL_WARN, "UtvPlatformCommonAcquireHardware(), delivery method %d is already acquired.",
                        g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType );
            continue;
        }

        switch ( g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType )
        {
#ifdef UTV_DELIVERY_BROADCAST
           case UTV_PUBLIC_DELIVERY_MODE_BROADCAST:
               if ( UTV_OK == ( rStatus = UtvPlatformDataBroadcastAcquireHardware( iEventType )))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_TRUE;
               else
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformDataBroadcastAcquireHardware() fails: %s", UtvGetMessageString( rStatus ) );
               break;
#endif
#ifdef UTV_DELIVERY_INTERNET
           case UTV_PUBLIC_DELIVERY_MODE_INTERNET:
               if ( UTV_OK == ( rStatus = UtvPlatformInternetCommonAcquireHardware( iEventType )))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_TRUE;
               else
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetCommonAcquireHardware() fails: %s", UtvGetMessageString( rStatus ) );
               break;
#endif
#ifdef UTV_DELIVERY_FILE
           case UTV_PUBLIC_DELIVERY_MODE_FILE:
               if ( UTV_OK == ( rStatus = UtvPlatformFileAcquireHardware( iEventType )))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_TRUE;
               else
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileAcquireHardware() fails: %s", UtvGetMessageString( rStatus ) );
               break;
#endif
           default:
               rStatus = UTV_BAD_DELIVERY_MODE;
               UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformCommonAcquireHardware() fails: %s, type == %d",
                           rStatus, g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType );
               break;
        } /* switch to decode each method in delivery configuration */

        if ( UTV_OK != rFinalStatus )
            rFinalStatus = rStatus;
        else
        {
            if ( UTV_OK != rStatus )
                UtvSetErrorCount( rStatus, 0 );
        }

    } /* for each delivery method */

    return rFinalStatus;
}


/* UtvPlatformCommonReleaseHardware
    For each delivery method attempt to release the hardware associated with it.
*/
UTV_RESULT UtvPlatformCommonReleaseHardware( void )
{
    UTV_RESULT    rStatus, rFinalStatus = UTV_UNKNOWN;
    UTV_UINT32    i;

    for ( i = 0; i < g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes; i++ )
    {
        if ( !g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess )
        {
            /* if it's not open there's nothing to do. */
            UtvMessage( UTV_LEVEL_WARN, "UtvPlatformCommonReleaseHardware(), delivery method %d is already released.",
                        g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType );
            continue;
        }

        switch ( g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType )
        {
#ifdef UTV_DELIVERY_BROADCAST
           case UTV_PUBLIC_DELIVERY_MODE_BROADCAST:
               if ( UTV_OK == ( rStatus = UtvPlatformDataBroadcastReleaseHardware()))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_FALSE;
               else
               {
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformDataBroadcastReleaseHardware() fails: %s", UtvGetMessageString( rStatus ) );
               }
               break;
#endif
#ifdef UTV_DELIVERY_INTERNET
           case UTV_PUBLIC_DELIVERY_MODE_INTERNET:
               if ( UTV_OK == ( rStatus = UtvPlatformInternetCommonReleaseHardware()))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_FALSE;
               else
               {
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetCommonReleaseHardware() fails: %s", UtvGetMessageString( rStatus ) );
               }
               break;
#endif
#ifdef UTV_DELIVERY_FILE
           case UTV_PUBLIC_DELIVERY_MODE_FILE:
               if ( UTV_OK == ( rStatus = UtvPlatformFileReleaseHardware()))
                   g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_FALSE;
               else
               {
                   UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReleaseHardware() fails: %s", UtvGetMessageString( rStatus ) );
               }
               break;
#endif
           default:
               rStatus = UTV_BAD_DELIVERY_MODE;
               UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReleaseHardware() fails: %s, type == %d", rStatus,
                           g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType );
               break;
        } /* switch to decode each method in delivery configuration */

        if ( UTV_OK != rFinalStatus )
            rFinalStatus = rStatus;
        else
        {
            if ( UTV_OK != rStatus )
                UtvSetErrorCount( rStatus, 0 );
        }

    } /* for each delivery method */

    return rFinalStatus;
}

#ifdef UTV_DELIVERY_FILE
/* UtvPlatformUtilityAssignFileToStore()
    Called in response to an external event that detects that a UTV file *may* be available.
    This event could be the insertion of a USB stick, it could be
    the result of an internet download, or it could be a test.
*/
UTV_RESULT UtvPlatformUtilityAssignFileToStore( UTV_UINT32 uiLinesToSkip )
{
    UTV_RESULT                  rStatus;
    UTV_INT8                    cFileName[ 128 ];
    UTV_INT8                    cFilePath[ 128 ];
    UTV_INT8                    cFile[ 256 ];
    UTV_UINT32                  hStore, uiFileHandle, i;

    /* ask the platform layer for a path where this file should be found */
    if ( UTV_OK != ( rStatus = UtvCEMGetUpdateFilePath( cFilePath, sizeof(cFilePath) )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformGetUpdateFilePath() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* create a complete path to the re-direct file */
    UtvMemFormat( (UTV_BYTE *)cFile, sizeof( cFile ), "%s/%s", cFilePath, UtvPlatformGetUpdateRedirectName() );

    /* open that file and get the name of the actual update file name */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( cFile, "r", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen( %s ) fails: \"%s\"", cFile, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

	/* skip n lines */
	for ( i = 0; UTV_TRUE; )
	{
		if ( UTV_OK != ( rStatus = UtvPlatformFileReadText( uiFileHandle, (UTV_BYTE *)cFileName, sizeof( cFileName ) )))
		{
			UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadText( %s ) fails: \"%s\"", cFilePath, UtvGetMessageString( rStatus ) );
			UtvPlatformFileClose( uiFileHandle );
			return rStatus;
		}
		
		/* comments don't count */
		if ( '#' == cFileName[ 0 ] )
		{
			continue;
		}

		if ( i < uiLinesToSkip )
		{
			UtvMessage( UTV_LEVEL_INFO, " SKIPPING \"%s\"", cFileName );
			i++;
			continue;
		}
		
		UtvPlatformFileClose( uiFileHandle );
		break;
	} 
	
    /* remove any CR or LF */
    for ( i = 0; 0 != cFileName[ i ] && '\r' != cFileName[ i ] && '\n' != cFileName[ i ]; i++ );
    cFileName[ i ] = 0;

    /* create a complete path to the actual update file */
    UtvMemFormat( (UTV_BYTE *)cFile, sizeof( cFile ), "%s/%s", cFilePath, cFileName );

    UtvMessage( UTV_LEVEL_INFO, " Assigning file \"%s\" to remote store", cFile );

    if ( UTV_OK != ( rStatus = UtvPlatformUpdateAssignRemote( cFile, &hStore )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformUpdateAssignRemote() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    return rStatus;
}
#endif

/* UtvPlatformUtilityFileOnly()
    Called to determine whether the current mode of the system is file (USB) only.
*/
UTV_BOOL UtvPlatformUtilityFileOnly( void )
{
#ifdef UTV_DELIVERY_FILE
    if ( 1 == g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes &&
         UTV_PUBLIC_DELIVERY_MODE_FILE == g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ 0 ].uiType )
        return UTV_TRUE;
    else
        return UTV_FALSE;
#else
    return UTV_FALSE;
#endif
}

/* UtvCTime
    Implements a reasonable version of ctime.
*/
UTV_INT8 *UtvCTime( UTV_TIME utvTime, UTV_INT8 *pBuffer, UTV_UINT32 uiBufferLen )
{
    UTV_UINT32  uiTimeLen = UtvStrlen( (UTV_BYTE *) ctime( &utvTime ) );
    UtvStrnCopy( (UTV_BYTE *) pBuffer, uiBufferLen, (UTV_BYTE *) ctime( &utvTime ), uiTimeLen );
    return pBuffer;
}

/* UtvGmtime
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns pointer to the tm structure that contains the converted time
*/
struct tm * UtvGmtime( time_t * pTime_t )
{
    struct tm * ptmTime;

    ptmTime = gmtime( pTime_t );

    if ( ptmTime == NULL )
        return ptmTime;

    /* save a copy and return copy to caller */
    UtvByteCopy( ( UTV_BYTE * )&gtmTime, ( UTV_BYTE * )ptmTime, sizeof( struct tm ) );
    return &gtmTime;
}

/* UtvConvertToIpV4String
    Converts an unsigned 32 bit integer value into the string representation of an
    IP Address.
    This code should NOT need to be changed when porting to a new operating system.
*/

UTV_RESULT UtvConvertToIpV4String(
  UTV_UINT32 uiAddress,
  UTV_INT8* pszBuffer,
  UTV_UINT32 uiBufferLen )
{
  UTV_RESULT result = UTV_INVALID_PARAMETER;

  if ( NULL != pszBuffer && 16 <= uiBufferLen )
  {
    sprintf( pszBuffer,
             "%u.%u.%u.%u",
             (UTV_UINT8)((uiAddress)       & 0xFF),
             (UTV_UINT8)((uiAddress >> 8)  & 0xFF),
             (UTV_UINT8)((uiAddress >> 16) & 0xFF),
             (UTV_UINT8)((uiAddress >> 24) & 0xFF));

    result = UTV_OK;
  }

  return ( result );
}

void UtvConvertHexToBin( void *bin, UTV_UINT32 size, UTV_INT8 *hex )
{
	UTV_BYTE *p = (UTV_BYTE *)bin;
    *p = 0x0;

      if (( hex[0] == '0' ) && (( hex[1] == 'x' ) || ( hex[1] == 'X' ))) hex += 2;

      UTV_BOOL shift = !( strlen( hex ) & 0x01 );
      while ( *hex && size )
      {
            if ( shift ) *p = 0x0;
            char c = *hex - '0';
            if ( c > 9 ) c = ( c & 0x17 ) - 7;
            *p |= c;
            if ( shift ) *p <<= 4;
            else { p++; size--; }
            shift = !shift;
            hex ++;
      }
}

/* UtvGetMessageString
    Moved here from platform-os-android-debug.c which isn't included
    in production code.
    Gets the error message string associated with an UTV_RESULT code
    This code should NOT need to be changed when porting to new operating systems
*/
char * UtvGetMessageString( UTV_RESULT iReturn )
{
    static char szMessage[ 32 ];

    /* Use the macro to create a switch table, returning any match */
    switch ( iReturn )
    {
#   define UTV_ERR( code, value, text ) case value: return text;
    UTV_ERRORLIST
#   undef UTV_ERR
    }

    /* Return unknown error */
    sprintf( szMessage, "(error code 0x%lx)", iReturn );
    return szMessage;
}

