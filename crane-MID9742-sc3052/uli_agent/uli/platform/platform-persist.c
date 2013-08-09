/* platform-persist.c : UpdateTV Personality Map - Common persistent memory functions

   CEM specific functions that are common to all module organizations.

   Copyright (c) 2007-2010 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPeristRead( void )
   UTV_RESULT UtvPeristWrite( void )
   ------------------------------------------------------------------
 
   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      11/17/2011  Resume support.
   Jim Muchow      04/27/2011  Add versioning to persistent storage structure 
   Bob Taylor      12/16/2010  Removed unsed Provisioner field access.
   Bob Taylor      10/01/2010  UQR caching added.
   Bob Taylor      06/10/2010  Removed 2.0.7 conversion support.
   Bob Taylor      02/09/2010  Replaced UTV_PLATFORM_PERSISTENT_STORAGE_FNAME with 
                               UtvPlatformGetPersistName().
   Bob Taylor      11/12/2009  Added missing file closes to error exits.
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Bob Taylor      06/26/2008  Removed signal strength references.
   Bob Taylor      06/17/2008  Added version setting support.
   Bob Taylor      04/08/2008  NAB Datacast demo support (signal strength fields in diag stats).
   Bob Taylor      11/01/2007  If UTV_PLATFORM_PERSISTENT_STORAGE_FNAME is not defined
                               persist read and write fails.
   Bob Taylor      10/23/2007  Example file implementation persistent memory dependent on
                               UTV_PERS_PERSIST_FILE definition.  No persistent memory if not
                               defined.
                               UTV_PLATFORM_PERSISTENT_STORAGE_FNAME.  Optionality removed.
   Bob Taylor      07/24/2007  UTV_TEST_PERSISTENT_STORAGE_FILE_NAME renamed to 
                               UTV_PLATFORM_PERSISTENT_STORAGE_FNAME.  Optionality removed.
   Bob Taylor      06/22/2007  Creation.
*/

#include <stdio.h>
#include <string.h>
#include "utv.h"

#define PERSIST_DEBUG

/* Global State and Diagnostic Structures
*/

/* Global state structure pointer
*/
UtvAgentState         *g_pUtvState = NULL;

/* Global diagnostic stats structure pointer
*/
UtvAgentDiagStats    *g_pUtvDiagStats = NULL;

/* Global best candidate structure pointer
*/
UtvDownloadInfo        *g_pUtvBestCandidate = NULL;

/* Version 1 or default structure containing all fixed-length persistent global data */
UtvAgentPersistentStorage g_utvAgentPersistentStorage;

#ifdef UTV_LIVE_SUPPORT
/* Version 2 addition to default structure */
UtvLiveSupportInfo_t g_utvLiveSupportInfo;
#endif

/* Variable length peristent global data
*/
UtvDownloadTracker      g_utvDownloadTracker;

/* Forward references
 */
static UTV_RESULT UtvReadPersistentMemory( void );

/* UtvPlatformPersistRead
    Initialize global storage from peristent memory.
*/
UTV_RESULT UtvPlatformPersistRead( void )
{
    UTV_RESULT  rStatus;

    /* Clear the structure that will hold the persistent storage */
    UtvMemset( &g_utvAgentPersistentStorage, 0, sizeof( UtvAgentPersistentStorage ));

#ifdef UTV_LIVE_SUPPORT
    /* set up the default values for the SessionInfo struct - this for the first time
       through when a V1 database is found */
    UtvMemset( &g_utvLiveSupportInfo, 0x00, sizeof( g_utvLiveSupportInfo ) );
    g_utvLiveSupportInfo.header.usSigBytes = UTV_LIVE_SUPPORT_INFO_SIG;
    g_utvLiveSupportInfo.header.usStructSize = sizeof ( g_utvLiveSupportInfo );
#endif

    /* Point g_pUtvState at the persistent state structure */
    /* This pointer is used extensively by UtvState.cpp */
    g_pUtvState = &g_utvAgentPersistentStorage.utvState;
    
    /* Point g_pUtvDiagStats at the persistent stats structure */
    /* This structure is used throughout the core to update diagnostic stat info */
    g_pUtvDiagStats = &g_utvAgentPersistentStorage.utvDiagStats;

    /* Point g_pUtvBestCandidate at the persistent best candidate structure */
    /* This structure is used extensively by the scan and download logic */
    g_pUtvBestCandidate =  &g_utvAgentPersistentStorage.utvBestCandidate;

    /* Now load persistent storage depending on what structures you have room to store */
    /* At the very least you must keep utvState, utvBestCandidate, and utvDownloadStatus[UTV_PERS_NUM_GROUPS] */
    /* persistently.  utvDiagStats are useful to keep for trouble shooting. */
    /* For test and development purposes persistent data may be stored as a file OR *not stored at all*. */
    /* In the actual production device if you do not store at least the three structures mentioned above */
    /* the device may not be able to accumulate modules to complete an update. */
    if ( UTV_OK != (rStatus = UtvReadPersistentMemory() ) )
    {
        /* Persistent memory is not available so initialize the global storage structure from scratch. */
        UtvMessage( UTV_LEVEL_WARN, "Persistent memory failure.  Initializing machine state." );


#ifdef UTV_LIVE_SUPPORT
        /* Initialize the signature, struct size, and num groups fields */
        g_utvAgentPersistentStorage.utvHeader.usSigBytes = UTV_PERSISTENT_MEMORY_STRUCT_SIG_V2;
        g_utvAgentPersistentStorage.utvHeader.usStructSize = sizeof( UtvAgentPersistentStorage_V2 );
#else
        /* Initialize the signature, struct size, and num groups fields */
        g_utvAgentPersistentStorage.utvHeader.usSigBytes = UTV_PERSISTENT_MEMORY_STRUCT_SIG;
        g_utvAgentPersistentStorage.utvHeader.usStructSize = sizeof( UtvAgentPersistentStorage );
#endif

        /* Initialize the state */
        g_utvAgentPersistentStorage.utvState.utvState = UTV_PUBLIC_STATE_SCAN;

        /* Initialize the query cache */
        g_utvAgentPersistentStorage.utvQueryCache.b_ValidQueryResponseCached = UTV_FALSE;
        g_utvAgentPersistentStorage.utvQueryCache.l_QueryPacingTimeToWait = 0;
        g_utvAgentPersistentStorage.utvQueryCache.l_ValidQueryRspTimestamp = 0;
        g_utvAgentPersistentStorage.utvQueryCache.UtvUQR.uiNumEntries = 0;

        /* Initialize the version */
        g_utvAgentPersistentStorage.utvVersion.ubMajor = UTV_AGENT_VERSION_MAJOR;
        g_utvAgentPersistentStorage.utvVersion.ubMinor = UTV_AGENT_VERSION_MINOR;
        g_utvAgentPersistentStorage.utvVersion.ubRevision = UTV_AGENT_VERSION_REVISION;

        /* zero the entire diag stats struct */
        UtvMemset( &g_utvAgentPersistentStorage.utvDiagStats, 0, sizeof( UtvAgentDiagStats ) );

        /* error events */
        g_utvAgentPersistentStorage.utvDiagStats.utvEvents[ 0 ].uiClassID = UTV_LEVEL_ERROR;

        /* warning events */
        g_utvAgentPersistentStorage.utvDiagStats.utvEvents[ 1 ].uiClassID = UTV_LEVEL_WARN;

        /* compatiblity events */
        g_utvAgentPersistentStorage.utvDiagStats.utvEvents[ 2 ].uiClassID = UTV_LEVEL_COMPAT;

        /* carriage events */
        g_utvAgentPersistentStorage.utvDiagStats.utvEvents[ 3 ].uiClassID = UTV_LEVEL_NETWORK;

        /* set the variable length header struct */
        g_utvAgentPersistentStorage.utvVarLenHeader.uiTrackerSig = UTV_PERSISTENT_MEMORY_TRACKER_SIG;
        g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerCount = 0;
        g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerModVer = 0;
        g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerStructSize = sizeof( UtvDownloadTracker );
        g_utvDownloadTracker.usCurDownloadCount = 0;
        g_utvDownloadTracker.usModuleVersion = 0;
        g_utvAgentPersistentStorage.utvDiagStats.bRegStoreSent = UTV_FALSE;

        /* set the unreported MV to a value that indicates we should send the current MV when the network connects. */
        g_utvAgentPersistentStorage.utvDiagStats.usLastDownloadEndedUnreportedMV = (UTV_UINT16) UTV_CORE_IMAGE_INVALID;

        /* commit the initialized structure immediately */
        UtvPlatformPersistWrite();

    } else
    {
        /* set the current state to the state of the system when it was last aborted */
        g_utvAgentPersistentStorage.utvState.utvState = g_utvAgentPersistentStorage.utvState.utvStateWhenAborted;
    }
    
    return rStatus;
}

/* UtvPlatformPersistWrite
    Write global storage to peristent memory.
*/
UTV_RESULT UtvPlatformPersistWrite( )
{
    FILE                       *f;
    UTV_UINT32                  i;
#ifdef PERSIST_DEBUG
    int bytesWritten;
#endif
    if ( NULL == UtvPlatformGetPersistName() )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformGetPersistName() returns NULL." );        
        return UTV_NULL_PTR;
    }
#ifdef PERSIST_DEBUG
    UtvMessage( UTV_LEVEL_TEST, "Writing persistent data.");
    bytesWritten = 0;
#endif
    /* Write out the persistent data to a file for use during the next invocation. */
    if ( NULL == (f = fopen( UtvPlatformGetPersistName(), "wb") ))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_OPEN_FAILS, __LINE__ );
        return UTV_PERSIST_FAILS;
    }

#ifdef UTV_LIVE_SUPPORT
    /* set up to write the newest version of the UtvAgentPersistentStorage */
    g_utvAgentPersistentStorage.utvHeader.usSigBytes = UTV_PERSISTENT_MEMORY_STRUCT_SIG_V2;
    g_utvAgentPersistentStorage.utvHeader.usStructSize = sizeof( UtvAgentPersistentStorage_V2 );
#endif

    /* set the variable length header fields */
    g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerCount = g_utvDownloadTracker.usCurDownloadCount;
    g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerModVer = g_utvDownloadTracker.usModuleVersion;

    /* write the fixed portion of persistent memory */
    if ( 1 != fwrite( &g_utvAgentPersistentStorage, sizeof( BaseUtvAgentPersistentStorage ), 1, f ))
    {
        fclose( f );
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_WRITE_FAILS, __LINE__ );
        return UTV_WRITE_FAILS;
    }
#ifdef PERSIST_DEBUG
    bytesWritten += sizeof( BaseUtvAgentPersistentStorage );
#endif

#ifdef UTV_LIVE_SUPPORT
    /* write the V2 fixed portion of persistent memory if need be */
    if ( 1 != fwrite( &g_utvLiveSupportInfo, sizeof( g_utvLiveSupportInfo ), 1, f ) )
    {
        fclose( f );
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_WRITE_FAILS, __LINE__ );
        return UTV_WRITE_FAILS;
    }

#ifdef PERSIST_DEBUG
    bytesWritten += sizeof( g_utvLiveSupportInfo );
#endif
#endif


    if ( 1 != fwrite( &(g_utvAgentPersistentStorage.utvVarLenHeader), sizeof( UtvVarLenHeader ), 1, f ) )
    {
        fclose( f );
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_WRITE_FAILS, __LINE__ );
        return UTV_WRITE_FAILS;
    }
#ifdef PERSIST_DEBUG
    bytesWritten += sizeof( UtvVarLenHeader );
#endif
    /* write the tracker objects if there are any */
    for ( i = 0; i < g_utvDownloadTracker.usCurDownloadCount; i++ )
    {
        if ( 1 != fwrite( &g_utvDownloadTracker.ausModIdArray[ i ], sizeof( UtvModRecord ), 1, f ) )
        {
            fclose( f );
            UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_WRITE_FAILS, __LINE__, i );
            return UTV_WRITE_FAILS;
        }
#ifdef PERSIST_DEBUG
        bytesWritten += sizeof( UTV_UINT16 );
#endif
    }

    fclose( f );
#ifdef PERSIST_DEBUG
    UtvMessage( UTV_LEVEL_TEST, "Saved %d bytes of persistent data.", bytesWritten );
#endif
    return UTV_OK;
}

/* Static Functions local to support persistent memory read/write.
*/

/* UtvReadPersistentMemory
    Reads global storage from a file.  In a real system this might be a read from a fixed
    location in NVRAM.
*/
static UTV_RESULT UtvReadPersistentMemory( void )
{
    FILE                       *f;
    UTV_UINT32                  i;
    UtvModRecord                sModRecord;
#ifdef PERSIST_DEBUG
    int bytesRead;
#endif
    if ( NULL == UtvPlatformGetPersistName() )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformGetPersistName() returns NULL." );        
        return UTV_NULL_PTR;
    }
#ifdef PERSIST_DEBUG
    UtvMessage( UTV_LEVEL_TEST, "Reading persistent data.");
    bytesRead = 0;
#endif
    /* Check to see if persistent data is available to initialize the global status  */
    /* structure */
    if ( NULL == (f = fopen( UtvPlatformGetPersistName(), "rb") ))
    {
        UtvMessage( UTV_LEVEL_WARN, "Unable to open %s for read of persistent data", UtvPlatformGetPersistName() );        
        return UTV_PERSIST_FAILS; /* no file available */
    }

    /* Read in the fixed portion of the base peristent storage structure.
       If for any reason there isn't that much data then punt. */

    if ( 1 != fread( &g_utvAgentPersistentStorage, sizeof( BaseUtvAgentPersistentStorage ), 1, f ) )
    {
        fclose( f );
        UtvMessage( UTV_LEVEL_WARN, "%s is too small", UtvPlatformGetPersistName() );        
        return UTV_PERSIST_FAILS; /* file is too small. */
    }
#ifdef PERSIST_DEBUG
    bytesRead += sizeof( BaseUtvAgentPersistentStorage );
#endif
    /* now figure out and verfiy what is thought to exist out on disk */

#ifdef UTV_LIVE_SUPPORT
    /* check the signature bytes */
    if ( ( g_utvAgentPersistentStorage.utvHeader.usSigBytes == UTV_PERSISTENT_MEMORY_STRUCT_SIG_V2 ) &&
         ( g_utvAgentPersistentStorage.utvHeader.usStructSize == sizeof( UtvAgentPersistentStorage_V2 ) ) )
    {
        /* extract V2 info */
        if ( 1 != fread( &g_utvLiveSupportInfo, sizeof( g_utvLiveSupportInfo ), 1, f ) )
        {
            fclose( f );
            UtvMessage( UTV_LEVEL_WARN, "%s is too small", UtvPlatformGetPersistName() );        
            return UTV_PERSIST_FAILS; /* file is too small. */
        }

        if ( ( g_utvLiveSupportInfo.header.usSigBytes != UTV_LIVE_SUPPORT_INFO_SIG ) ||
             ( g_utvLiveSupportInfo.header.usStructSize != sizeof( g_utvLiveSupportInfo ) ) )
        {
            fclose( f );
            UtvMessage( UTV_LEVEL_WARN, "%s is too small", UtvPlatformGetPersistName() );        
            return UTV_PERSIST_FAILS; /* file is too small. */
        }
#ifdef PERSIST_DEBUG
        bytesRead += sizeof( g_utvLiveSupportInfo );
#endif
    } 
    else 
#endif
	if ( g_utvAgentPersistentStorage.utvHeader.usSigBytes != UTV_PERSISTENT_MEMORY_STRUCT_SIG )
    {
        fclose( f );
        UtvLogEventOneHexNum( UTV_LEVEL_ERROR, UTV_CAT_PERSIST, UTV_PERSIST_FAILS, __LINE__, g_utvAgentPersistentStorage.utvHeader.usSigBytes );
        return UTV_PERSIST_FAILS;
    } 
    /* check the struct size */
    else if ( g_utvAgentPersistentStorage.utvHeader.usStructSize != sizeof( UtvAgentPersistentStorage ) )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Persistent memory struct is bad size and can't be recovered." );
        fclose( f );
        return UTV_PERSIST_FAILS;
    }

    /* at this point, either the extra Vn (n >= 2) structures have been read in successully
       or the structure is the default (aka V1) structure, no matter, continue on regardless */

    /* check and read in the variable length portion of the file */

    if ( 1 != fread( &(g_utvAgentPersistentStorage.utvVarLenHeader), sizeof( UtvVarLenHeader ), 1, f ) )
    {
        fclose( f );
        UtvMessage( UTV_LEVEL_WARN, "%s is too small", UtvPlatformGetPersistName() );        
        return UTV_PERSIST_FAILS; /* file is too small. */
    }
#ifdef PERSIST_DEBUG
    bytesRead += sizeof( UtvVarLenHeader );
#endif
    /* is the tracker signature OK? */
    if ( UTV_PERSISTENT_MEMORY_TRACKER_SIG != g_utvAgentPersistentStorage.utvVarLenHeader.uiTrackerSig )
    {
        /* something is wrong, punt */
        fclose( f );
        UtvMessage( UTV_LEVEL_ERROR, "%s tracker signature is bad", UtvPlatformGetPersistName() );        
        return UTV_PERSIST_FAILS;
    }

    /* if the tracker stucture size changed then punt */
    if ( sizeof( UtvDownloadTracker ) != g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerStructSize )
    {
        /* something is wrong, punt */
        fclose( f );
        UtvMessage( UTV_LEVEL_ERROR, "%s tracker stuct size is bad", UtvPlatformGetPersistName() );        
        return UTV_PERSIST_FAILS;
    }

    /* read the tracked modules into their global struct */
    if ( UTV_PLATFORM_MAX_MODULES < g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerCount )
        g_utvDownloadTracker.usCurDownloadCount = UTV_PLATFORM_MAX_MODULES;
    else
        g_utvDownloadTracker.usCurDownloadCount = g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerCount;
    g_utvDownloadTracker.usModuleVersion = g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerModVer;
                    
    /* read in module IDs */
    for ( i = 0; i < g_utvDownloadTracker.usCurDownloadCount; i++ )
    {
        if ( 1 != fread( &g_utvDownloadTracker.ausModIdArray[ i ], sizeof( UtvModRecord ), 1, f ) )
        {
            fclose( f );
            UtvMessage( UTV_LEVEL_ERROR, "%s has invalid number of module IDs in tracker structure", UtvPlatformGetPersistName() );        
            return UTV_PERSIST_FAILS; /* file is too small. */
        }
#ifdef PERSIST_DEBUG
        bytesRead += sizeof( UTV_UINT16 );
#endif
    }

    /* throw out any additional module IDs (if there are any) */
    for ( ; i < g_utvAgentPersistentStorage.utvVarLenHeader.usTrackerCount; i++ )
    {
        if ( 1 != fread( &sModRecord, sizeof( UtvModRecord ), 1, f ) )
        {
            fclose( f );
            UtvMessage( UTV_LEVEL_ERROR, "%s has invalid number of module IDs in tracker structure", UtvPlatformGetPersistName() );        
            return UTV_PERSIST_FAILS; /* file is too small. */
        }
#ifdef PERSIST_DEBUG
        bytesRead += sizeof( UTV_UINT16 );
#endif
    }

    fclose( f );
#ifdef PERSIST_DEBUG
    UtvMessage( UTV_LEVEL_TEST, "Read %d bytes of persistent data.", bytesRead );
#endif
    return UTV_OK;
}
