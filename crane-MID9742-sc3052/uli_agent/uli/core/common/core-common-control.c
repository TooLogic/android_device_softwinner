/* core-common-control.c : UpdateTV Common Core -  top-level non-public entry points into the core.

 Main control logic for the UpdateTV Agent.

  In order to ensure UpdateTV network compatibility, customers must not
   change UpdateTV Common Core code.

 Copyright (c) 2004 - 2010 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      07/18/2011  UtvPublicAgentEarlyInit() added.
  Bob Taylor      07/12/2011  Moved UtvCommonDecryptULIDRead() to here from project layer to consolidate 
                              boot error handling.
  Bob Taylor      07/07/2011  Added persistent storage write if there is a secure open or crypt init error.
  Jim Muchow      06/29/2011  Add calls the explicit SSL Init/Deinit functions
  Chris Nigbur    12/13/2010  Added Network Assistance initialization and shutdown.
  Chris Nigbur    10/21/2010  Added Live Support initialization and shutdown.
  Jim Muchow      08/23/2010  Added call UtvUserAbort() in void UtvPublicAgentShutdown()
  Jim Muchow      08/20/2010  Added call to UtvCloseAllFiles()
  Jim Muchow      08/20/2010  Added calls to UtvCloseLogFile() and UtvInternetReset_UpdateQueryDisabled()
  Bob Taylor      06/23/2010  Added call to UtvPlatformUpdateUnassignRemote() in shutdown.
  Bob Taylor      05/10/2010  Added call to UtvPlatformInstallInit().
  Bob Taylor      03/15/2010  Added call to UtvCommonDecryptInit().
  Bob Taylor      02/09/2010  UtvPlatformStoreInit() call added to UtvPublicAgentInit().
  Chris Nigbur    02/02/2010  UtvImageInit() call added to UtvPublicAgentInit().
  Bob Taylor      11/10/2009  New UtvPublicAgentInit() and UtvCEMConfigure() protos.
  Bob Taylor      01/26/2009  Created from 1.9.21
*/

#include "utv.h"
#include <stdio.h>
#include <string.h>

static UTV_BOOL   s_bEarlyInit = UTV_FALSE;
static UTV_UINT32 s_uiEarlyInitLine = 0;
static UTV_RESULT s_uiEarlyInitResult = 0;

#ifdef _CHKPNT
#define _CHKPNT_HERE  UtvChkPnt( __FILE__, __LINE__ )
#else
#define _CHKPNT_HERE

#endif

/* extern access */
extern UTV_MUTEX_HANDLE ghUtvStateMutex;

/* UtvPublicAgentEarlyInit
    Must be called before UtvPublicAgentInit() is called to intiialize low-level 
    sub-systems in the Agent.  Does not exercise the file system or device-specific interfaces.
    OS mutexes must be available.

	All function calls here must not use error logging!
*/
void UtvPublicAgentEarlyInit( void )
{
	UTV_RESULT lStatus;
	
    /* NO ERROR LOGGING ALLOWED WITH THESE CALLS! */

	_CHKPNT_HERE;

    /* Initialize install tracking */
    UtvPlatformInstallInit();
	
	_CHKPNT_HERE;

    /* Initialize platform store structure */
    lStatus = UtvPlatformStoreInit();
	if ( UTV_OK != lStatus )
	{
		s_uiEarlyInitLine = __LINE__;
		s_uiEarlyInitResult = lStatus;
	}

	_CHKPNT_HERE;

    /* Initialize the string table */
    lStatus = UtvStringsInit( );
	if ( UTV_OK != lStatus )
	{
		s_uiEarlyInitLine = __LINE__;
		s_uiEarlyInitResult = lStatus;
	}

	_CHKPNT_HERE;

    /* Prepare for wait use. */
    lStatus = UtvInitWaitList( );
	if ( UTV_OK != lStatus )
	{
		s_uiEarlyInitLine = __LINE__;
		s_uiEarlyInitResult = lStatus;
	}

	_CHKPNT_HERE;

    /* Prepare for mutex use */
    lStatus = UtvInitMutexList( );
	if ( UTV_OK != lStatus )
	{
		s_uiEarlyInitLine = __LINE__;
		s_uiEarlyInitResult = lStatus;
	}

	_CHKPNT_HERE;

#ifdef UTV_DEVICE_WEB_SERVICE
    /* init Events Facility */
    lStatus = UtvEventsInit( );
    if ( UTV_OK != lStatus )
    {
        s_uiEarlyInitLine = __LINE__;
        s_uiEarlyInitResult = lStatus;
    }
#endif

    /* Initialize the image access array */
    lStatus = UtvImageInit();
	if ( UTV_OK != lStatus )
	{
		s_uiEarlyInitLine = __LINE__;
		s_uiEarlyInitResult = lStatus;
	}

#ifdef UTV_INTERNAL_SSL_INIT
    /* initialize OpenSSL base structures */
    (void)UtvPlatformInternetSSL_Init( );
#endif

	s_bEarlyInit = UTV_TRUE;

	return;
}

/* UtvPublicAgentInit
    Called by any of the project layer Agent calls to intialize the Agent.
	UtvPublicAgentInit() must be called before this funtion!
*/
UTV_RESULT UtvPublicAgentInit( void )
{
    UTV_RESULT rSecureStatus, rCryptStatus, rULIDStatus;

	_CHKPNT_HERE;

    /* Call UtvPlatformPersistRead() which will initialize global storage from persistent memory
       which will in turn enable persistent logging. */
    UtvPlatformPersistRead();

    /* ERROR LOGGING ALLOWED AFTER UtvPlatformPersistRead() */

	_CHKPNT_HERE;

	/* if early init hasn't been called punt */
	if ( !s_bEarlyInit )
	{
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, UTV_NOT_INITIALIZED, __LINE__ );
		return UTV_NOT_INITIALIZED;
	}

	/* if early init had a problem report it but continue */
	if ( UTV_OK != s_uiEarlyInitResult )
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, s_uiEarlyInitResult, s_uiEarlyInitLine );

	_CHKPNT_HERE;

    /* Open the platform specific secure facility needs to happen ahead of Provisioner open */
    rSecureStatus = UtvPlatformSecureOpen();

	_CHKPNT_HERE;

    /* Initialize the decryption sub-system.  No corresponding close/deinit needed */
    rCryptStatus = UtvCommonDecryptInit();

	_CHKPNT_HERE;

    /* attempt to read ULID from secure storage */
    if ( UTV_OK != ( rULIDStatus = UtvCommonDecryptULIDRead() ))
    {
#ifdef UTV_DEBUG        
        UTV_UINT32 uiLevel;
        
	_CHKPNT_HERE;

        /* not open is just a warning because on first boot ULID isn't expected to be present */
        if ( UTV_OPEN_FAILS != rULIDStatus )
            uiLevel = UTV_LEVEL_ERROR;
        else
            uiLevel = UTV_LEVEL_WARN;

        UtvMessage( uiLevel, "UtvCommonDecryptULIDRead() \"%s\"", UtvGetMessageString( rULIDStatus ) );
#endif

    }

	_CHKPNT_HERE;

    /* Peristent storage is now operational.  If there has been an error make sure log is written out. */
    if ( (UTV_OK != rSecureStatus) || (UTV_OK != rCryptStatus) || (UTV_OK != rULIDStatus) )
        UtvPlatformPersistWrite();

	_CHKPNT_HERE;

	/* perform any CEM configuration here */
	UtvCEMConfigure();

	_CHKPNT_HERE;

     /* Initialize the mutex for state access once - This should probably be moved to persistent storage */
    UtvMutexInit( &ghUtvStateMutex );

	_CHKPNT_HERE;

#   ifdef UTV_DELIVERY_BROADCAST
    /* Allocate memory for the section queue */
    UtvSectionQueueInit();
#   endif

	_CHKPNT_HERE;

#ifdef _ANNOUNCE_DBG
    /* output the configuration of the receiver */
    UtvAnnouceConfiguration();
#endif

	_CHKPNT_HERE;

#ifdef UTV_DEVICE_WEB_SERVICE
    UtvInternetDeviceWebServiceInit();
#endif

	_CHKPNT_HERE;

#ifdef UTV_WEBSERVER
    /* Initialize the base web server, this will start the web server worker thread */
    UtvWebServerInit();

    /* Initialize the ULI endpoint for the web server */
    UtvWebServerUliEndPointInit();

    /* Initialize the ULI event handlers for the web server */
    UtvWebServerUliEventsInit();
#endif

	_CHKPNT_HERE;

#ifdef UTV_LIVE_SUPPORT
    /* Initialize the Live Support sub-system. */
    UtvLiveSupportAgentInit();
#endif

	_CHKPNT_HERE;

#ifdef UTV_LOCAL_SUPPORT
    /* Call the CEM specific initialization routines for Local Support. */
    UtvCEMLocalSupportInitialize();
#endif

    _CHKPNT_HERE;

#ifdef UTV_LAN_REMOTE_CONTROL
    /* Call the CEM specific initialization routines for LAN Remote Control. */
    UtvCEMLanRemoteControlInitialize();
#endif

    _CHKPNT_HERE;

    return UTV_OK;
}

/* UtvPublicAgentShutdown
    Call to release any resources and free up memory allocation to the UpdateTV application
*/
void UtvPublicAgentShutdown( void )
{
    /* Make sure all other threads are told to shutdown */
    UtvUserAbort();

#ifdef UTV_WEBSERVER
    /* Shutdown the base web server */
    UtvWebServerShutdown();

    /* Shutdown the ULI event handlers for the web server */
    UtvWebServerUliEventsShutdown();

    /* Shutdown the ULI endpoint for the web server */
    UtvWebServerUliEndPointShutdown();
#endif

#ifdef UTV_LIVE_SUPPORT
    /* Shutdown the Live Support sub-system. */
    UtvLiveSupportAgentShutdown();
#endif

#ifdef UTV_DEVICE_WEB_SERVICE
    UtvInternetDeviceWebServiceShutdown();
#endif

    /* Release all open images */
    UtvManifestClose();
    UtvImageCloseAll();
    UtvPlatformUpdateUnassignRemote();

#   ifdef UTV_PLATFORM_PROVISIONER
    /* Release provisioner mutex, always done on exit whether we used Provisioner or not */
    UtvInternetProvisionerClose();
#   endif

#ifdef UTV_INTERNAL_SSL_INIT
    /* release OpenSSL base structures */
    (void)UtvPlatformInternetSSL_Deinit( );
#endif
    /* Close the platform specific secure facility.  Needs to happen after Provisioner closes */
    UtvPlatformSecureClose();

#   ifdef UTV_DELIVERY_BROADCAST
    /* Make calls to release allocated memory here. */
    UtvClearCarouselCandidate();
    UtvClearGroupCandidate();
    UtvClearDownloadCandidate();
    UtvClearSectionQueue();
#   endif

    /* Write out the global state structure to persistent memory. */
    UtvPlatformPersistWrite();

    /* Cleanup strings table */
    UtvStringsRelease( );

#ifdef UTV_DEVICE_WEB_SERVICE
    /* init Events Facility */
    UtvEventsShutdown( );
#endif

    /* Must do all this last because of mutex related code */
    UtvMutexDestroy( ghUtvStateMutex );
    UtvDestroyMutexList( );
    UtvDestroyWaitList( );

    UtvInternetImageAccessInterfaceReinit();

    /* check and cancel any existing threads */
    UtvThreadKillAllThreads();
    UtvCloseAllFiles();
#ifdef UTV_TEST_LOG_FNAME    
    UtvCloseLogFile( );
#endif
#ifdef UTV_TEST_RSPTIME_LOG_FNAME
    UtvCloseRspTimeLogFile( );
#endif
    UtvInternetReset_UpdateQueryDisabled();
}

/* UtvConfigureDeliveryModes
    Store the specified delivery methods into peristent storage.
    They will be will be referenced by the Agent to determine how to
    get updates.
*/
UTV_RESULT UtvConfigureDeliveryModes( UTV_UINT32 uiNumModes, UTV_UINT32 *pModes )
{
    UTV_UINT32    i, j, uiLocalCount = 0, uiLocalList[ UTV_DELIVERY_MAX_MODES ];

    /* sanity check count */
    if ( uiNumModes > UTV_DELIVERY_MAX_MODES )
        return UTV_BAD_DELIVERY_COUNT;

    /* sanity check and then install each method */
    for ( i = 0; i < uiNumModes; i++ )
    {
        /* sanity check it */
        switch ( pModes[ i ] )
        {
        case UTV_PUBLIC_DELIVERY_MODE_BROADCAST:
#           ifndef UTV_DELIVERY_BROADCAST
            return UTV_BAD_DELIVERY_MODE;
#           else
            break;
#           endif
        case UTV_PUBLIC_DELIVERY_MODE_INTERNET:
#           ifndef UTV_DELIVERY_INTERNET
            return UTV_BAD_DELIVERY_MODE;            
#           else
            break;
#           endif
        case UTV_PUBLIC_DELIVERY_MODE_FILE:
#           ifndef UTV_DELIVERY_FILE
            return UTV_BAD_DELIVERY_MODE;
#           else
            break;
#           endif
        default:
            return UTV_BAD_DELIVERY_MODE;
        }

        /* make sure it isn't a duplicate */
        for ( j = 0; j < uiLocalCount; j++ )
            if ( uiLocalList[ j ] == pModes[ i ] )
                return UTV_DELIVERY_DUPLICATE;

        /* place it in the local list */
        uiLocalList[ uiLocalCount++ ] = pModes[ i ];
    }
    
    /* got through them all, update persistent memory struct */
    g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes = uiLocalCount;
    for ( i = 0; i < uiLocalCount; i++ )
    {
        g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType = uiLocalList[ i ];
        g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].bAccess = UTV_FALSE;
    }
    
    return UTV_OK;
}

UTV_BOOL UtvCheckDeliveryMode( UTV_UINT32 uiDeliveryMode )
{
    UTV_UINT32    i;
    
    for ( i = 0; i < g_utvAgentPersistentStorage.utvDeliveryConfiguration.uiNumModes; i++ )
    {
        if ( uiDeliveryMode == g_utvAgentPersistentStorage.utvDeliveryConfiguration.Mode[ i ].uiType )
            return UTV_TRUE;
    }
    
    return UTV_FALSE;
}

