/* platform-debug.c: UpdateTV Platform Layer - Common, non-OS-specific
                    Debug and Logging support

   Customers should not need to modify this file when porting.

   -*- SHOULD NOT BE INCLUDED IN PRODUCTION BUILDS -*-

   Copyright (c) 2004-2010 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      12/16/2010  Removed definition of UtvDisplayProvisionedObjects().
   Jim Muchow      09/21/2010  BE support
   Bob Taylor      06/29/2010  Added private library, serial number, and query host output to announce configuration.
   Jim Muchow      03/30/2010  Fixed test log.
   Jim Muchow      03/19/2010  Modified usage of new UTV_ERRORLIST w/ explicit enumerations
   Bob Taylor      02/08/2010  ifdef'd UtvProvisionerTestBenchDump() with UTV_PROVISIONER_DEBUG
                               Improved thread name tracker.
   Bob Taylor      01/29/2009  Created from 1.9.21 for 2.0.0
*/

#include <stdio.h>
#include "utv.h"

/* All debug message related functions are only compiled when
   UTV_TEST_MESSAGES is defined.  Undefine UTV_TEST_MESSAGES to
   remove all message output
*/

#ifdef UTV_TEST_MESSAGES

extern UTV_UINT32 g_uiMessageLevel;

/* time-related storage and formating vars */
UTV_INT8 gszTime[28];

#ifdef _implemented
/* Local static function protos */
static const char * LocalConvertGpsTime( UTV_TIME dtGps );
#endif

#define _MAX_THREADS 10

typedef struct 
{
	UTV_THREAD_HANDLE	hThread;
	UTV_INT8      *pName;
} _ThreadTracker;

static _ThreadTracker s_ThreadTracker[ _MAX_THREADS ];
static UTV_BOOL       s_bThreadTracker = UTV_FALSE;

/* UtvDebugSetThreadName
    Sets a string asssociated with the caller's thread to track who is calling the
    Agent.
*/
void UtvDebugSetThreadName( UTV_INT8 *pName )
{
	UTV_UINT32	i;
	
	/* init if necessary */
	if ( !s_bThreadTracker )
	{
		for ( i = 0; i < _MAX_THREADS; i++ )
			s_ThreadTracker[ i ].hThread = 0;
		s_bThreadTracker = UTV_TRUE;
	}
	
	/* look for an existing entry for this thread or name and overwrite it
       else find an open one and add it */
	for ( i = 0; i < _MAX_THREADS; i++ )
	{
		if ( 0 == s_ThreadTracker[ i ].hThread || NULL == s_ThreadTracker[ i ].pName )
			continue;
		
		/* re-use same name */
		if ( (UtvStrlen( (UTV_BYTE *) pName ) == UtvStrlen( (UTV_BYTE *) s_ThreadTracker[ i ].pName )) &&
			 (0 == UtvMemcmp( (void *) pName, (void *) s_ThreadTracker[ i ].pName, UtvStrlen( (UTV_BYTE *) pName ) )))
		{
			s_ThreadTracker[ i ].hThread = UtvThreadSelf();
			return;
		}
		
		/* re-use same thread ID */
		if ( s_ThreadTracker[ i ].hThread == UtvThreadSelf() )
		{
			s_ThreadTracker[ i ].pName = pName;
			return;
		}
	}

	/* not found, look for an open slot */
	for ( i = 0; i < _MAX_THREADS; i++ )
	{
		if ( 0 == s_ThreadTracker[ i ].hThread )
		{
			s_ThreadTracker[ i ].hThread = UtvThreadSelf();
			s_ThreadTracker[ i ].pName = pName;
			return;
		}
	}

	UtvMessage( UTV_LEVEL_ERROR, "UtvDebugRegisterThreadName() NO MORE TRACKING SPACE for the thread named \"%s\"", pName );
}

/* UtvDebugGetThreadName
    Gets the name of the caller's thread.
*/
UTV_INT8 *UtvDebugGetThreadName( void )
{
	UTV_UINT32	i;
	
	/* init if necessary */
	if ( !s_bThreadTracker )
	{
		for ( i = 0; i < _MAX_THREADS; i++ )
			s_ThreadTracker[ i ].hThread = 0;
		s_bThreadTracker = UTV_TRUE;
	}

	/* look for a thread with this handle */
	for ( i = 0; i < _MAX_THREADS; i++ )
	{
		if ( s_ThreadTracker[ i ].hThread == UtvThreadSelf() )
			return s_ThreadTracker[ i ].pName;
	}

	return "unknown";
}



/* UtvGetCategoryString
    Gets the category message string associated with an UTV_CATEGORY code
    This code should NOT need to be changed when porting to new operating systems
*/
char * UtvGetCategoryString( UTV_CATEGORY rCat )
{
    static char szCategory[ 32 ];

    /* Use the macro to create a switch table, returning any match */
    switch ( rCat )
    {
#   define UTV_CAT( code, text ) case code: return text;
    UTV_CATLIST
#   undef UTV_CAT
    case UTV_CATEGORY_COUNT: return "unknown";
    }

    /* Return unknown category */
    sprintf( szCategory, "(category code 0x%x)", rCat );
    return szCategory;
}

/* UtvDisplayComponentDirectory()
    Dump the component directory of a given image.
*/
UTV_RESULT UtvDisplayComponentDirectory( UTV_UINT32 hImage )
{
    UTV_RESULT                rStatus;
    UTV_UINT32                i, n;
    UtvCompDirHdr          *pCompDirHdr;
    UtvCompDirCompDesc        *pCompDesc;
    UtvRange               *pHWMRanges;
    UtvRange               *pSWVRanges;
    UtvDependency           *pSWDeps;
    UtvTextDef               *pTextDefs;
    UtvCompDirModDesc       *pModDesc;
    UTV_INT8               *pszString1, *pszString2;

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDirHdr->toCreateUserName ), &pszString1 )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "" );
    UtvMessage( UTV_LEVEL_INFO, "Component Directory Dump" );
    UtvMessage( UTV_LEVEL_INFO, "" );
    UtvMessage( UTV_LEVEL_INFO, "    Update Summary:" );
    UtvMessage( UTV_LEVEL_INFO, "        Created by: AppID: %u, MajorVer: %u, MinorVer: %u, Revision: %u, User: %s",
                pCompDirHdr->ubCreateAppID, pCompDirHdr->ubCreateVerMajor, pCompDirHdr->ubCreateVerMinor,
                pCompDirHdr->ubCreateVerRevision, pszString1 );

    UtvMessage( UTV_LEVEL_INFO, "        OUI: 0x%06X, Model Group: %04X", 
                Utv_32letoh( pCompDirHdr->uiPlatformOUI ), 
                Utv_16letoh( pCompDirHdr->usPlatformModelGroup ) );
    UtvMessage( UTV_LEVEL_INFO, "        Update Attributes: 0x%08X, Module Version: 0x%X, Software Version: %d",
                Utv_32letoh( pCompDirHdr->uiUpdateAttributes ), 
                Utv_16letoh( pCompDirHdr->usUpdateModuleVersion ), 
                Utv_16letoh( pCompDirHdr->usUpdateSoftwareVersion ) );

    UtvMessage( UTV_LEVEL_INFO, "        Update-Level Hardware Model Ranges: %u", 
                Utv_16letoh( pCompDirHdr->usNumUpdateHardwareModelRanges ) );
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateHardwareModelRanges ); i++, pHWMRanges++ )
        UtvMessage( UTV_LEVEL_INFO, "            %u to %u", 
                    Utv_16letoh( pHWMRanges->usStart ), 
                    Utv_16letoh( pHWMRanges->usEnd ) );


    UtvMessage( UTV_LEVEL_INFO, "        Update-Level Software Version Ranges: %u", 
                Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareVersionRanges ) );
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareVersionRanges ); i++, pSWVRanges++ )
        UtvMessage( UTV_LEVEL_INFO, "            %u to %u", 
                    Utv_16letoh( pSWVRanges->usStart ), 
                    Utv_16letoh( pSWVRanges->usEnd ) );

    UtvMessage( UTV_LEVEL_INFO, "        Update-Level Software Dependencies: %u", 
                Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareDependencies ) );
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareDependencies ); i++, pSWDeps++ )
    {
        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                          Utv_16letoh( pSWDeps->toComponentName ), 
                                                          &pszString1 )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        UtvMessage( UTV_LEVEL_INFO, "            \"%s\" %u to %u", pszString1, 
                    Utv_16letoh( pSWDeps->utvRange.usStart ), 
                    Utv_16letoh( pSWDeps->utvRange.usEnd ) );
    }

    UtvMessage( UTV_LEVEL_INFO, "        Update-Level Text Definitions: %u", 
                Utv_16letoh( pCompDirHdr->usNumUpdateTextDefs ) );
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateTextDefs ); i++, pTextDefs )
    {
        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                          Utv_16letoh( pTextDefs->toIdentifier ), 
                                                          &pszString1 )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                          Utv_16letoh( pTextDefs->toText ), 
                                                          &pszString2 )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        UtvMessage( UTV_LEVEL_INFO, "            \"%s\" ==> \"%s\"", pszString1, pszString2 );
    }
    UtvMessage( UTV_LEVEL_INFO, "" );

    UtvMessage( UTV_LEVEL_INFO, "    Component Summary:" );

    /* spin through each component displaying info about it */
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumComponents ); i++ )
    {
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, i, &pCompDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescByIndex() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        if ( UTV_OK != ( rStatus = UtvImageGetCompDescInfo( pCompDesc, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs, &pModDesc  )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescInfo() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                          Utv_16letoh( pCompDesc->toName ), 
                                                          &pszString1 )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        UtvMessage( UTV_LEVEL_INFO, "        Name: \"%s\", Attributes: %08X, Module Count: %d",
                    pszString1, Utv_32letoh( pCompDesc->uiComponentAttributes ), 
                    Utv_16letoh( pCompDesc->usModuleCount ) );

        UtvMessage( UTV_LEVEL_INFO, "        Image size: %d, HM Ranges: %d, SW Ranges: %d, SW Depends: %d",
                    Utv_32letoh( pCompDesc->uiImageSize ), 
                    Utv_16letoh( pCompDesc->usNumHardwareModelRanges ), 
                    Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges ), 
                    Utv_16letoh( pCompDesc->usNumSoftwareDependencies ) );

        UtvMessage( UTV_LEVEL_INFO, "        Component-Level Hardware Model Ranges: %u", 
                    Utv_16letoh( pCompDesc->usNumHardwareModelRanges ) );
        for ( n = 0; n < Utv_16letoh( pCompDesc->usNumHardwareModelRanges ); n++, pHWMRanges++ )
            UtvMessage( UTV_LEVEL_INFO, "            %u to %u", 
                        Utv_16letoh( pHWMRanges->usStart ), 
                        Utv_16letoh( pHWMRanges->usEnd ) );

        UtvMessage( UTV_LEVEL_INFO, "        Component-Level Software Version Ranges: %u", 
                    Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges ) );
        for ( n = 0; n < Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges ); n++, pSWVRanges++ )
            UtvMessage( UTV_LEVEL_INFO, "            %u to %u", 
                        Utv_16letoh( pSWVRanges->usStart ), Utv_16letoh( pSWVRanges->usEnd ) );

        UtvMessage( UTV_LEVEL_INFO, "        Component-Level Software Dependencies: %u", 
                    Utv_16letoh( pCompDesc->usNumSoftwareDependencies ) );
        for ( n = 0; n < Utv_16letoh( pCompDesc->usNumSoftwareDependencies ); n++, pSWDeps++ )
        {
            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                              Utv_16letoh( pSWDeps->toComponentName ), 
                                                              &pszString1 )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            UtvMessage( UTV_LEVEL_INFO, "            \"%s\" %u to %u", pszString1, 
                        Utv_16letoh( pSWDeps->utvRange.usStart ), 
                        Utv_16letoh( pSWDeps->utvRange.usEnd ) );
        }

        UtvMessage( UTV_LEVEL_INFO, "        Component-Level Text Definitions: %u", 
                    Utv_16letoh( pCompDesc->usNumTextDefs ) );
                    for ( n = 0; n < Utv_16letoh( pCompDesc->usNumTextDefs ); n++, pTextDefs++ )
        {
            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, pTextDefs->toIdentifier, &pszString1 )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, pTextDefs->toText, &pszString2 )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetText() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            UtvMessage( UTV_LEVEL_INFO, "            \"%s\" ==> \"%s\"", pszString1, pszString2 );
        }

        UtvMessage( UTV_LEVEL_INFO, "" );
        UtvMessage( UTV_LEVEL_INFO, "        Module Summary for component \"%s\":", UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, 
                                                                                                                    Utv_16letoh( pCompDesc->toName ) ) );
        for ( n = 0; n < Utv_16letoh( pCompDesc->usModuleCount ); n++, pModDesc++ )
        {

            UtvMessage( UTV_LEVEL_INFO, "            Module %d of %d, size: %lu, encrypted size: %lu, offset: %lu",
                        n + 1, Utv_16letoh( pCompDesc->usModuleCount ),
                        Utv_32letoh( pModDesc->uiModuleSize ), 
                        Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                        Utv_32letoh( pModDesc->uiModuleOffset ) );
        }

        UtvMessage( UTV_LEVEL_INFO, "" );

    }

    return UTV_OK;
}

#ifdef _implemented

/* UtvStatsSummary
    Function that outputs the stats summary stored in persistent memory.
*/
void UtvStatsSummary( )
{
    UtvStatsDisplay( &g_utvAgentPersistentStorage );
}

/* UtvStatsDisplay
    Formats and displays the stats passed in via the argument pointer.
*/
void UtvStatsDisplay( UtvAgentPersistentStorage *pStats )
{
    UTV_UINT32 i;
    UTV_PUBLIC_STATE utvState;

    UtvMessage( UTV_LEVEL_INFO, UTV_AGENT_VERSION " - Summary" );
    UtvMessage( UTV_LEVEL_INFO, "UTV Private Library Version \"%s\"", UtvPrivateLibraryVersion() );

    /* check signature bytes and version */
    if ( pStats->utvHeader.usSigBytes != UTV_PERSISTENT_MEMORY_STRUCT_SIG )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Persistent memory struct signature bytes are incorrect: 0x%X", pStats->utvHeader.usSigBytes );
        return;
    }

    /* check signature bytes and version */
    if ( pStats->utvHeader.usStructSize != sizeof( UtvAgentPersistentStorage ) )
    {
        UtvMessage( UTV_LEVEL_WARN, "Persistent memory struct size mismatch.  Struct size: %d, expected: %d",
                    pStats->utvHeader.usStructSize, sizeof( UtvAgentPersistentStorage ) );
    }

    /* Display the version information. */
    UtvMessage( UTV_LEVEL_INFO, "Persistent memory structure created with version %d.%d.%d of the Agent",
                g_utvAgentPersistentStorage.utvVersion.ubMajor,
                g_utvAgentPersistentStorage.utvVersion.ubMinor,
                g_utvAgentPersistentStorage.utvVersion.ubRevision );

    /* If the Agent isn't running then display the state it was in when it was last aborted. */
    /* Else display the state it's in now. */
    if ( UTV_PUBLIC_STATE_EXIT != pStats->utvState.utvState )
    {
        utvState = pStats->utvState.utvState;
        UtvMessage( UTV_LEVEL_INFO, "Current system state (%d seconds until next event):", pStats->utvState.iDelayMilliseconds/1000 );
    } else
    {
        utvState = pStats->utvState.utvStateWhenAborted;
        UtvMessage( UTV_LEVEL_INFO, "System state at time of abort:" );
    }

    switch ( utvState )
    {
    case UTV_PUBLIC_STATE_SCAN:
        UtvMessage( UTV_LEVEL_INFO, "   Scan" );
        break;

    case UTV_PUBLIC_STATE_DOWNLOAD:
        UtvMessage( UTV_LEVEL_INFO, "   Download" );
        break;

    case UTV_PUBLIC_STATE_DOWNLOAD_DONE:
        UtvMessage( UTV_LEVEL_INFO, "   Download Done" );
        break;

    case UTV_PUBLIC_STATE_ABORT:
        UtvMessage( UTV_LEVEL_INFO, "   Abort" );
        break;

    case UTV_PUBLIC_STATE_EXIT:
        UtvMessage( UTV_LEVEL_INFO, "   Exit" );
        break;

    default:
        UtvMessage( UTV_LEVEL_INFO, "   Unknown: %d", utvState );
        break;
    }

    UtvMessage( UTV_LEVEL_INFO, "Diagnostic sub-state:" );

    switch ( pStats->utvDiagStats.utvSubState )
    {
    case UTV_PUBLIC_SSTATE_SLEEPING:
        UtvMessage( UTV_LEVEL_INFO, "   Sleeping for %d seconds", pStats->utvDiagStats.iSubStateWait / 1000 );
        break;

    case UTV_PUBLIC_SSTATE_SCANNING_DSI:
        UtvMessage( UTV_LEVEL_INFO, "   Scanning for DSIs for %d seconds", pStats->utvDiagStats.iSubStateWait / 1000 );
        break;

    case UTV_PUBLIC_SSTATE_SCANNING_DII:
        UtvMessage( UTV_LEVEL_INFO, "   Scanning for DIIs for %d seconds", pStats->utvDiagStats.iSubStateWait / 1000 );
        break;

    case UTV_PUBLIC_SSTATE_SCANNING_DDB:
        UtvMessage( UTV_LEVEL_INFO, "   Scanning for DDBs for %d seconds", pStats->utvDiagStats.iSubStateWait / 1000 );
        break;

    case UTV_PUBLIC_SSTATE_DOWNLOADING:
        UtvMessage( UTV_LEVEL_INFO, "   Downloading DDBs for %d seconds", pStats->utvDiagStats.iSubStateWait / 1000  );
        break;

    case UTV_PUBLIC_SSTATE_DECRYPTING:
        UtvMessage( UTV_LEVEL_INFO, "   Decrypting" );
        break;

    case UTV_PUBLIC_SSTATE_STORING_MODULE:
        UtvMessage( UTV_LEVEL_INFO, "   Storing Module" );
        break;

    case UTV_PUBLIC_SSTATE_STORING_IMAGE:
        UtvMessage( UTV_LEVEL_INFO, "   Storing Image" );
        break;

    case UTV_PUBLIC_SSTATE_SHUTDOWN:
        UtvMessage( UTV_LEVEL_INFO, "   Shutdown" );
        break;

    default:
        UtvMessage( UTV_LEVEL_INFO, "   Unknown: %d", pStats->utvDiagStats.utvSubState );
        break;
    }

    UtvMessage( UTV_LEVEL_INFO, "Diagnostic stats:" );

    UtvMessage( UTV_LEVEL_INFO, "   Compatible scans: %d", pStats->utvDiagStats.iScanGood );
    UtvMessage( UTV_LEVEL_INFO, "   Scans without result: %d", pStats->utvDiagStats.iScanBad );
    UtvMessage( UTV_LEVEL_INFO, "   Interrupted scans: %d", pStats->utvDiagStats.iScanAborted );
    UtvMessage( UTV_LEVEL_INFO, "   Complete downloads: %d", pStats->utvDiagStats.iDownloadComplete );
    UtvMessage( UTV_LEVEL_INFO, "   Partial downloads: %d", pStats->utvDiagStats.iDownloadPartial );
    UtvMessage( UTV_LEVEL_INFO, "   Unsuccessful downloads: %d", pStats->utvDiagStats.iDownloadBad );
    UtvMessage( UTV_LEVEL_INFO, "   Interrupted downloads: %d", pStats->utvDiagStats.iDownloadAborted );
    UtvMessage( UTV_LEVEL_INFO, "   Total modules downloaded: %d", pStats->utvDiagStats.iModCount );
    UtvMessage( UTV_LEVEL_INFO, "   Total channels in frequency list: %d", pStats->utvDiagStats.iFreqListSize );
    UtvMessage( UTV_LEVEL_INFO, "   Current channel index: %d", pStats->utvDiagStats.iChannelIndex );
    UtvMessage( UTV_LEVEL_INFO, "   Last frequency tuned: %d", pStats->utvDiagStats.iLastFreqTuned );
    UtvMessage( UTV_LEVEL_INFO, "   Channels in frequency list with tune problems: %d", pStats->utvDiagStats.iBadChannelCount );
    UtvMessage( UTV_LEVEL_INFO, "   Blocks missing during download: %d", pStats->utvDiagStats.iNeedBlockCount );
    UtvMessage( UTV_LEVEL_INFO, "   Already have count: %d", pStats->utvDiagStats.iAlreadyHaveCount );
    UtvMessage( UTV_LEVEL_INFO, "   Server ID of last scan: %s", pStats->utvDiagStats.aubLastScanID );
    UtvMessage( UTV_LEVEL_INFO, "   Server ID of last download: %s", pStats->utvDiagStats.aubLastDownloadID );
    UtvMessage( UTV_LEVEL_INFO, "   Time of last scan: %s", LocalConvertGpsTime( pStats->utvDiagStats.tLastScanTime ) );
    UtvMessage( UTV_LEVEL_INFO, "   Last compatible scan time: %s", LocalConvertGpsTime( pStats->utvDiagStats.tLastCompatibleScanTime ) );
    UtvMessage( UTV_LEVEL_INFO, "   Last compatible scan frequency: %u", pStats->utvDiagStats.iLastCompatibleFreq );
    UtvMessage( UTV_LEVEL_INFO, "   Last compatible scan PID: 0x%X", pStats->utvDiagStats.iLastCompatiblePid );
    UtvMessage( UTV_LEVEL_INFO, "   Last module download time: %s", LocalConvertGpsTime( pStats->utvDiagStats.tLastModuleTime ) );
    UtvMessage( UTV_LEVEL_INFO, "   Last module download frequency: %u", pStats->utvDiagStats.iLastModuleFreq );
    UtvMessage( UTV_LEVEL_INFO, "   Last module download PID: 0x%X", pStats->utvDiagStats.iLastModulePid );
    UtvMessage( UTV_LEVEL_INFO, "   Time of last complete download: %s", LocalConvertGpsTime( pStats->utvDiagStats.tLastDownloadTime ) );
    UtvMessage( UTV_LEVEL_INFO, "   Number of sections delivered: %d", pStats->utvDiagStats.iSectionsDelivered );
    UtvMessage( UTV_LEVEL_INFO, "   Number of sections discarded: %d", pStats->utvDiagStats.iSectionsDiscarded );
#ifdef UTV_TEST_ABORT
    UtvMessage( UTV_LEVEL_INFO, "   Abort count: %lu, highest: %lu, average: %lu",
                pStats->utvDiagStats.iAbortCount, pStats->utvDiagStats.iHighestAbortTime, pStats->utvDiagStats.iAvgAbortTime );
#endif

    UtvMessage( UTV_LEVEL_INFO, "Scan status:" );

    if ( 0 == pStats->utvDiagStats.utvLastDSIIncompatState.usError )
    {
        UtvMessage( UTV_LEVEL_INFO, "   No Group Rejections" );
    } else
    {
        UtvMessage( UTV_LEVEL_INFO, "   Last Group Rejection: %s (0x%X) on %s",
                    UtvGetMessageString( pStats->utvDiagStats.utvLastDSIIncompatState.usError ),
                    pStats->utvDiagStats.utvLastDSIIncompatState.iInfo,
                    LocalConvertGpsTime( pStats->utvDiagStats.utvLastDSIIncompatState.tTime ) );
    }

    if ( 0 == pStats->utvDiagStats.utvLastDIIIncompatState.usError )
    {
        UtvMessage( UTV_LEVEL_INFO, "   No Module Rejections" );
    } else
    {
        UtvMessage( UTV_LEVEL_INFO, "   Last Module Rejection: %s (0x%X) on %s",
                    UtvGetMessageString( pStats->utvDiagStats.utvLastDIIIncompatState.usError ),
                    pStats->utvDiagStats.utvLastDIIIncompatState.iInfo,
                    LocalConvertGpsTime( pStats->utvDiagStats.utvLastDIIIncompatState.tTime ) );
    }

    UtvMessage( UTV_LEVEL_INFO, "Error summary:" );

    if ( 0 == pStats->utvDiagStats.utvErrorStats[0].usError )
    {
        UtvMessage( UTV_LEVEL_INFO, "   No errors to report" );
    } else
    {
        for ( i = 0; i < UTV_ERROR_COUNT; i++ )
            if ( pStats->utvDiagStats.utvErrorStats[ i ].usCount )
            {
                UtvMessage( UTV_LEVEL_INFO, "   %d error(s) (%d) - %s on %s",
                            pStats->utvDiagStats.utvErrorStats[ i ].usCount,
                            pStats->utvDiagStats.utvErrorStats[ i ].iInfo,
                            UtvGetMessageString( pStats->utvDiagStats.utvErrorStats[ i ].usError ),
                            LocalConvertGpsTime( pStats->utvDiagStats.utvErrorStats[ i ].tTime ) );
            }
    }

    /* Dump current download candidate if there is one.  A non-zero download time indicates this. */
    if ( 0 != pStats->utvBestCandidate.dtDownload )
    {
        UtvMessage( UTV_LEVEL_INFO, "Download candidate:" );
#ifdef UTV_DELIVERY_BROADCAST
        UtvMessage( UTV_LEVEL_INFO, "   name: \"%s\", date/time: %s",
                    pStats->utvBestCandidate.szModuleName, UtvConvertGpsTime( pStats->utvBestCandidate.dtDownload ) );
#else
        UtvMessage( UTV_LEVEL_INFO, "   name: \"%s\"",
                    pStats->utvBestCandidate.szModuleName );
#endif
        UtvMessage( UTV_LEVEL_INFO, "   PID: 0x%X, ID: %d, priority: 0x%X, module version: %d",
                    pStats->utvBestCandidate.iCarouselPid, pStats->utvBestCandidate.iModuleId,
                    pStats->utvBestCandidate.iModulePriority, pStats->utvBestCandidate.iModuleVersion );
        UtvMessage( UTV_LEVEL_INFO, "   hw model range: %d to %d, sw version range: %d to %d",
                    pStats->utvBestCandidate.iHardwareModelBegin, pStats->utvBestCandidate.iHardwareModelEnd,
                    pStats->utvBestCandidate.iSoftwareVersionBegin, pStats->utvBestCandidate.iSoftwareVersionEnd  );
    }
}

#endif

/* UtvDisplayComponentManifest
    Function that displays the component manifest of the system
*/
void UtvDisplayComponentManifest()
{
    UTV_UINT32                i;
    UtvCompDirCompDesc       *pCompDesc;
    UTV_INT8                 *pszCompName;

    UtvMessage( UTV_LEVEL_INFO, "-------------------" );
    UtvMessage( UTV_LEVEL_INFO, "COMPONENT MANIFEST:" );

    UtvMessage( UTV_LEVEL_INFO, "-------------------" );

    UtvMessage( UTV_LEVEL_INFO, "   Platform - OUI: 0x%06X, MG: 0x%04X, HW Model: %d, SW Ver: 0x%X, Mod Ver: 0x%X, Max Mods: %d,  AA: %d, Field: %d, Factory: %d",
                UtvCEMGetPlatformOUI(), UtvCEMGetPlatformModelGroup(), UtvCEMTranslateHardwareModel(), UtvManifestGetSoftwareVersion(),
                UtvManifestGetModuleVersion(), UTV_PLATFORM_MAX_MODULES, UtvManifestAllowAdditions(), UtvManifestGetFieldTestStatus(), UtvManifestGetFactoryTestStatus() );

    UtvMessage( UTV_LEVEL_INFO, "   Components:" );

    for ( i = 0; UTV_OK == UtvImageGetCompDescByIndex( g_hManifestImage, i, &pCompDesc ); i++ )
    {
        UtvPublicImageGetText( g_hManifestImage, Utv_16letoh( pCompDesc->toName ), &pszCompName );

                UtvMessage( UTV_LEVEL_INFO, "       Name: \"%s\", Current SW Ver: 0x%X, Attributes: 0x%08X, OUI: 0x%X, Model Group 0x%04X",
                    pszCompName,
                            Utv_16letoh( pCompDesc->usSoftwareVersion ),
                            Utv_32letoh( pCompDesc->uiComponentAttributes ),
                            Utv_32letoh( pCompDesc->uiOUI ),
                            Utv_16letoh( pCompDesc->usModelGroup ) );
    }

    UtvMessage( UTV_LEVEL_INFO, "-------------------" );
}

/* UtvTranslateDeliveryMode
    Translate a delivery mode enum into a debug string.
*/
UTV_INT8 *UtvTranslateDeliveryMode( UTV_UINT32 uiDeliveryMode )
{
    switch ( uiDeliveryMode )
    {
        case UTV_PUBLIC_DELIVERY_MODE_BROADCAST:
            return UTV_DELIVERY_MODE_BROADCAST_STR;
        case UTV_PUBLIC_DELIVERY_MODE_INTERNET:
            return UTV_DELIVERY_MODE_INTERNET_STR;
        case UTV_PUBLIC_DELIVERY_MODE_FILE:
            return UTV_DELIVERY_MODE_FILE_STR;
        default:
            break;
   }

   return "unknown";
}

/* UtvAnnounceConfiguration
    Function that dumps the configuration information to the
    output device and the log file if enabled
*/
void UtvAnnouceConfiguration( void )
{
    UTV_UINT8     utvPersModInstall = UTV_PLATFORM_INSTALL;
    char       *pszModInstall, *pszThreading, *apszDataSources[UTV_DELIVERY_MAX_MODES+1], *apszDeliveryModes[UTV_DELIVERY_MAX_MODES];
    UTV_UINT32    uiDataSourceCount, uiDeliveryModeCount;

#ifdef UTV_DELIVERY_BROADCAST
#if !defined(UTV_PLATFORM_DATA_BROADCAST_STATIC) && !defined(UTV_PLATFORM_DATA_BROADCAST_TS_FILE)
    UTV_UINT32     uiList;
    UTV_UINT32 uiFreqListLen, *puiFreqList;
#endif
#endif

    /* Announce message level */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " configured for logging%s%s%s%s",
        ( g_uiMessageLevel & UTV_LEVEL_INFO ) ? " INFO" : "",
        ( g_uiMessageLevel & UTV_LEVEL_WARN ) ? " WARN" : "",
        ( g_uiMessageLevel & UTV_LEVEL_ERROR ) ? " ERROR" : "",
        ( g_uiMessageLevel & UTV_LEVEL_PERF ) ? " PERF" : "",
        ( g_uiMessageLevel & UTV_LEVEL_TEST ) ? " TEST" : "",
        ( g_uiMessageLevel & UTV_LEVEL_TRACE ) ? " TRACE" : "" );

#ifdef UTV_DELIVERY_BROADCAST
#if !defined(UTV_PLATFORM_DATA_BROADCAST_STATIC) && !defined(UTV_PLATFORM_DATA_BROADCAST_TS_FILE)
    /* Announce frequency list */
    {
        UtvGetTunerFrequencyList ( &uiFreqListLen, &puiFreqList );
        for ( uiList = 0; uiList < uiFreqListLen; uiList++ )
            UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " configured for frequency %d on %d MHz", uiList, puiFreqList[ uiList ] / 1000000 );
    }
#endif
#endif

#ifdef UTV_TEST_LOG_FILE_NAME
    /* Announce log file if any */
    if ( UtvStrlen( (UTV_BYTE *)UTV_TEST_LOG_FILE_NAME ) > 0 )
        UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " configured for log file \"%s\"", UTV_TEST_LOG_FILE_NAME );
#endif

    /* convert configuration defines into strings for announcement. */
    switch (utvPersModInstall)
    {
    case  UTV_PLATFORM_INSTALL_FILE:  pszModInstall = UTV_PLATFORM_INSTALL_FILE_STR; break;
    case  UTV_PLATFORM_INSTALL_CUST:  pszModInstall = UTV_PLATFORM_INSTALL_CUST_STR; break;
    default:                          pszModInstall = "Unknown"; break;
    }

    /* Create a list of the available data sources.  These are dependent on
       compile-time defines which configure what data sources are linked in. */

    uiDataSourceCount = 0;
#ifdef UTV_PLATFORM_DATA_BROADCAST_LINUX_DVBAPI
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_BROADCAST_LINUX_DVBAPI_STR;
#endif
#ifdef UTV_PLATFORM_DATA_BROADCAST_TS_FILE
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_BROADCAST_TS_FILE_STR;
#endif
#ifdef UTV_PLATFORM_DATA_BROADCAST_STATIC
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_BROADCAST_STATIC_STR;
#endif
#ifdef UTV_PLATFORM_DATA_BROADCAST_CUST
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_BROADCAST_CUST_STR;
#endif

#ifdef UTV_PLATFORM_DATA_INTERNET_CLEAR_SOCKET
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_CLEAR_SOCKET_STR;
#endif
#ifdef UTV_PLATFORM_DATA_INTERNET_CLEAR_WINSOCK
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_CLEAR_WINSOCK_STR;
#endif
#ifdef UTV_PLATFORM_DATA_INTERNET_CLEAR_CUST
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_CLEAR_CUST_STR;
#endif

#ifdef UTV_PLATFORM_DATA_INTERNET_SSL_SOCKET
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_SSL_SOCKET_STR;
#endif
#ifdef UTV_PLATFORM_DATA_INTERNET_SSL_WINSOCK
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_SSL_WINSOCK_STR;
#endif
#ifdef UTV_PLATFORM_DATA_INTERNET_SSL_OPENSSL
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_SSL_OPENSSL_STR;
#endif
#ifdef UTV_PLATFORM_DATA_INTERNET_SSL_CUST
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_INTERNET_SSL_CUST_STR;
#endif

#ifdef UTV_PLATFORM_DATA_FILE_STDIO
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_FILE_STDIO_STR;
#endif
#ifdef UTV_PLATFORM_DATA_FILE_CUST
    apszDataSources[ uiDataSourceCount++ ] = UTV_PLATFORM_DATA_FILE_CUST_STR;
#endif
    /* fill out the empty ones with blanks */
    while ( uiDataSourceCount < UTV_DELIVERY_MAX_MODES )
        apszDataSources[ uiDataSourceCount++ ] = "";

    /* Create a list of the configured delivery modes.  These are dependent on
       run-time configuration */

    for ( uiDeliveryModeCount = 0;
          uiDeliveryModeCount < g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes;
          uiDeliveryModeCount++ )
        apszDeliveryModes[ uiDeliveryModeCount ] = \
        UtvTranslateDeliveryMode( g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ uiDeliveryModeCount ].uiType );

    /* fill out the empty ones with blanks */
    while ( uiDeliveryModeCount < UTV_DELIVERY_MAX_MODES )
        apszDeliveryModes[ uiDeliveryModeCount++ ] = "";

#ifdef UTV_OS_SINGLE_THREAD
    pszThreading = "single";
#else
    pszThreading = "multi";
#endif

    /* Generate module organization independent announcement */
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Using \"%s\" module installation", pszModInstall );
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Available data sources: \"%s\" \"%s\" \"%s\"", apszDataSources[0], apszDataSources[1], apszDataSources[2] );
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Configured delivery modes: \"%s\" \"%s\" \"%s\"", apszDeliveryModes[0], apszDeliveryModes[1], apszDeliveryModes[2] );
    UtvMessage( UTV_LEVEL_INFO | UTV_LEVEL_TEST, " Using %s-threaded configuration", pszThreading );

    UtvMessage( UTV_LEVEL_INFO, "UTV Private Library Version \"%s\"", UtvPrivateLibraryVersion() );
	{
		UTV_INT8 szSN[64];
		if ( UTV_OK == UtvCEMGetSerialNumber( szSN, sizeof(szSN) ))
			UtvMessage( UTV_LEVEL_INFO, "Serial Number: \"%s\"", szSN );
	}
	UtvMessage( UTV_LEVEL_INFO, "Query Host: \"%s\"", UtvManifestGetQueryHost() );

    UtvDisplayComponentManifest();

#ifndef UTV_FILE_BASED_PROVISIONING
    UtvDisplayProvisionedObjects();
#endif
}

void UtvDisplaySchedule(UTV_PUBLIC_SCHEDULE *pSchedule)
{
    UTV_RESULT     rStatus;
    UTV_UINT32     u, n, i, s;
    UTV_INT8      *pszCompName, *pszTextID, *pszTextDef;

    UtvMessage( UTV_LEVEL_INFO, "Schedule returned with %d updates", pSchedule->uiNumUpdates );
    for ( u = 0; u < pSchedule->uiNumUpdates; u++ )
    {
        UtvMessage( UTV_LEVEL_INFO, "    Update #%d, module version 0x%02X:", u, pSchedule->tUpdates[ u ].usModuleVersion );

        UtvMessage( UTV_LEVEL_INFO, "    Delivery mode: %s, number of components: %d",
                    UtvTranslateDeliveryMode( pSchedule->tUpdates[ u ].uiDeliveryMode ), pSchedule->tUpdates[ u ].uiNumComponents );

        if ( UTV_PUBLIC_DELIVERY_MODE_BROADCAST == pSchedule->tUpdates[ u ].uiDeliveryMode )
        {
            UtvMessage( UTV_LEVEL_INFO, "    Broadcast download duration: %d, Size: %d, Block size: %d, Frequency: %d, PID: 0x%X",
                        pSchedule->tUpdates[ u ].uiBroadcastDuration,
                        pSchedule->tUpdates[ u ].uiDownloadSize, pSchedule->tUpdates[ u ].uiBlockSize,
                        pSchedule->tUpdates[ u ].uiFrequency, pSchedule->tUpdates[ u ].uiPid );

            for ( s = 0; s < pSchedule->tUpdates[ u ].uiNumTimeSlots; s++ )
                UtvMessage( UTV_LEVEL_INFO, "    Start time #%d: %d seconds from now", s, pSchedule->tUpdates[ u ].uiSecondsToStart[ s ] );
        } else {

            /* broadcast doesn't have component-level info yet */
            for ( i = 0; i < pSchedule->tUpdates[ u ].uiNumComponents; i++ )
            {
                UtvPublicImageGetText( pSchedule->tUpdates[ u ].hImage, 
                                       Utv_16letoh( pSchedule->tUpdates[ u ].tComponents[ i ].pCompDesc->toName ), 
                                       &pszCompName );
                UtvMessage( UTV_LEVEL_INFO, "        Component Name: %s, Attributes: 0x%08X", pszCompName, 
                            Utv_32letoh( pSchedule->tUpdates[ u ].tComponents[ i ].pCompDesc->uiComponentAttributes ) );
                for ( n = 0; UTV_OK == UtvPublicImageGetCompTextId( pSchedule->tUpdates[ u ].hImage, pSchedule->tUpdates[ u ].tComponents[ i ].pCompDesc, n, &pszTextID); n++ )
                {
                    if ( UTV_OK != ( rStatus = UtvPublicImageGetCompTextDef( pSchedule->tUpdates[ u ].hImage, pSchedule->tUpdates[ u ].tComponents[ i ].pCompDesc, pszTextID, &pszTextDef )))
                    {
                        UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetCompTextDef() fails: \"%s\"", UtvGetMessageString( rStatus ) );
                        return;
                    }
                    UtvMessage( UTV_LEVEL_INFO, "            Text Def %d: %s => %s", n, pszTextID, pszTextDef );
                }
            }
        }
    }
}

/* UtvAsctime
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns pointer to a formated string that contains the time
*/
UTV_INT8 * UtvAsctime( struct tm * ptmTime )
{
    UTV_INT8     * pszTime;
    UTV_UINT32      iLen;

    UtvMemset( gszTime, 0x0, sizeof( gszTime ) );

    pszTime = asctime( ptmTime );

    if ( pszTime == NULL )
        return pszTime;

    iLen = UtvStrlen( (UTV_BYTE *)pszTime );

    /* knock out CR */
    iLen -= 1;

    /* save a copy and return copy to caller */
    UtvStrnCopy( ( UTV_BYTE * )gszTime, sizeof( gszTime ), ( UTV_BYTE * )pszTime, iLen );

    return gszTime;
}

/* UtvMktime
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns a time_t structure that contains the new time
*/
time_t UtvMktime( struct tm * ptmTime )
{

    /* no local copy needed */
    return mktime( ptmTime );
}

/* Static Functions local to this module follow...
*/

#ifdef _implemented
/* LocalConvertGpsTime
    Just like UtvConvertGpsTime() except it handles the case in which time is zero.
*/
static const char * LocalConvertGpsTime( UTV_TIME dtGps )
{
    static char szTimeUnknown[] = "(Time Unknown)";

    if ( 0 == dtGps )
        return szTimeUnknown;

#ifdef UTV_DELIVERY_BROADCAST
    return UtvConvertGpsTime( dtGps );
#else
    return NULL;
#endif

}
#endif

#endif /* UTV_TEST_MESSAGES */
