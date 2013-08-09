/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *               Copyright (C) 2009-2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Primary Device Agent adaptation function file.
 *
 * @file      project-generic.c
 *
 * @author    Bob Taylor
 *
 * @version   $Revision: 5882 $
 *            $Author: chris.nigbur $
 *            $Date: 2011-12-01 16:13:35 -0500 (Thu, 01 Dec 2011) $
 *
 * @mainpage
 *
 * @section Adaption      Platform Adaptation
 *
 * @subsection General      General Adapatation
 *
 * The following functions may require adaptation and/or invocation.
 *
 * - UtvPlatformNetworkUp()
 * - UtvProjectOnNetworkUp()
 * - UtvProjectOnForceNOCContact()
 * - UtvCEMGetSerialNumber()
 * - UtvCEMReadHWMFromRegister()
 * - UtvCEMTranslateHardwareModel()
 * - UtvPlatformGetEtherMAC()
 * - UtvPlatformGetWiFiMAC()
 * - UtvPlatformSystemTimeSet()
 * - UtvProjectOnObjectRequestStatus()
 * - UtvProjectOnObjectRequestSize()
 * - UtvProjectOnObjectRequest()
 * - UtvPlatformInstallAllocateModuleBuffer()
 * - UtvPlatformInstallFreeModuleBuffer()
 * - UtvPlatformInstallPartitionOpenWrite()
 * - UtvPlatformInstallPartitionOpenRead()
 * - UtvPlatformInstallPartitionWrite()
 * - UtvPlatformInstallPartitionRead()
 * - UtvPlatformInstallPartitionClose()
 * - UtvPlatformInstallFileOpenWrite()
 * - UtvPlatformInstallFileOpenRead()
 * - UtvPlatformInstallFileWrite()
 * - UtvPlatformInstallFileRead()
 * - UtvPlatformInstallFileClose()
 * - UtvPlatformInstallComponentComplete()
 * - UtvPlatformInstallUpdateComplete()
 * - UtvPlatformInstallCopyComponent()
 * - UtvPlatformFactoryModeNotice()
 * - UtvPlatformCommandInterface()
 * - UtvPlatformSecureDeliverKeys()
 * - UtvPlatformSecureDecrypt()
 *
 * @subsection LiveSupport      Live Support Video Adaptation
 *
 * The following functions may require adaptation when the platform supports Live Support
 * session functionality.
 * 
 * - UtvCEMLiveSupportInitialize()
 * - UtvCEMGetDeviceInfo()
 * - UtvCEMNetworkDiagnostics()
 * - s_LiveSupportProcessRemoteButton()
 * - s_LiveSupportGetDeviceInfo()
 * - s_LiveSupportCheckForUpdates()
 * - s_LiveSupportUpdateSoftware()
 * - s_LiveSupportCheckProductRegistered()
 * - s_LiveSupportRegisterProduct()
 * - s_LiveSupportProductReturnPreProcess()
 * - s_LiveSupportGetNetworkInfo()
 * - s_LiveSupportNetworkDiagnostics()
 * - s_LiveSupportHardwareDiagQuick()
 * - s_LiveSupportHardwareDiagFull()
 * - s_LiveSupportCheckInputs()
 * - s_LiveSupportGetPartsList()
 * - s_LiveSupportGetSystemStatistics()
 * - s_LiveSupportResetFactoryDefaults()
 * - s_LiveSupportSetAspectRatio()
 * - s_LiveSupportApplyVideoPattern()
 * - s_LiveSupportGetDeviceConfig()
 * - s_LiveSupportSetDeviceConfig()
 * - s_LiveSupportSetDateTime()
 *
 * @subsection LiveSupportVideo      Live Support Video Adaptation
 *
 * The following functions in this file may require adaptation when the platform supports
 * a media output stream as part of Live Support sessions.
 *
 * - UtvPlatformLiveSupportVideoSecureMediaStream()
 * - UtvPlatformLiveSupportVideoGetLibraryPath()
 * - UtvPlatformLiveSupportVideoGetVideoFrameSpecs()
 * - UtvPlatformLiveSupportVideoFillVideoFrameBuffer()
 *
 **********************************************************************************************
 */

/*
   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      01/17/2012  Added object ID to UtvPlatformOnProvisioned().
   Bob Taylor      12/26/2011  Added new uiClearObjectSize arg to UtvPlatformOnProvisioned().
\  Bob Taylor      12/20/2011  Added new arguments to UtvPlatformOnProvisioned().  Added UTV_FILE_BASED_PROVISIONING 
                               ifdef to eliminate object request mechanism when using UtvPlatformOnProvisioned().
                               Added self-contained secure sub-system.
                               Added transit key OTP sub-system.
   Bob Taylor      12/02/2011  Intrinsic ULPK transit key support.
   Bob Taylor      11/30/2011  3.2.2:  Introduced UtvProjectOnEarlyBoot() boot.  Changed UtvProjectOnBoot() to s_Init() which is now called
                               intrinsically by this layer and not called externally.  Added UtvPlatformOnProvisioned().
   Bob Taylor      11/18/2011  Removed broadcast related functions.  Changed "wb+" arg to "wb" in UtvPlatformInstallFileOpenWrite()
                               and UtvPlatformInstallPartitionOpenWrite() because modules arrived out of order in broadcast
                               only.
   Jim Muchow      11/02/2011  Download Resume Support
   Bob Taylor      02/23/2011  Improved UtvProjectOnAbort() logic.  Added UtvPlatformSecureWriteReadTest().
   Nathan Ford     02/21/2011  Saving component manifest as update-0.store is now off by default.
   Jim Muchow      02/04/2011  UtvPlatformSecureGetGUID() as part of ULID Copy Protection scheme implementation
   Bob Taylor      12/16/2010  Added UtvProjectOnObjectRequestStatus().  
                               Fixed bad error message in UtvPlatformSecureDecryptDirect().
   Chris Nigbur    10/21/2010  Added Live Support command handlers.
   Bob Taylor      10/01/2010  UtvPlatformSystemTimeSet() returns UTV_TRUE bey default now for test purposes.
                               Should still be implmented to refelect whether system time is actually set or not.
   Jim Muchow      09/21/2010  BE support.
   Jim Muchow      08/16/2010  Add UtvProjectOnTestAgent(), call UtvThreadKillAllThreads()
   Nathan Ford     07/01/2010  Removed code to decrypt common objects from UtvPlatformSecureRead() as it is handled internally now.
                               Streamlined UtvPlatformSecureRead() and UtvPlatformSecureWrite()
   Bob Taylor      06/30/2010  Added support for UtvProjectOnDebugInfo().
   Bob Taylor      05/03/2010  Added network up logic and more warn/error discrimination.
   Nate Ford       04/28/2010  Removed secure save and secure delete.
   Bob Taylor      04/07/2010  Added security model two support to UtvPlatformSecureRead().
   Bob Taylor      02/08/2010  Added more platform-specific name re-direct functions.
                               Added abort handling added to s_UtvDownloadThread.
                               Moved platform-install, platform-secure and
                               platform-data-broadcast, platform-file primitives to here.
   Bob Taylor      12/13/2009  Broke schedule check and download into two separate calls.
                               Added UtvProjectOnScanForStoredUpdate()
   Bob Taylor      11/10/2009  Pre-release for review.
*/

#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include "utv.h"
#include UTV_TARGET_PROJECT
#include UTV_TARGET_CEM

/*
*****************************************************************************************************
**
** STATIC DEFINES
**
*****************************************************************************************************
*/

/* max number of characters in a path */
#define S_FILE_PATH_MAX    128

/* placeholder provisioned object names */
#define S_OBJECT1_NAME          "object1"
#define S_OBJECT1_SAVED_NAME    "object1_SAVED_NAME"

/* placeholder DVD access define, would normaly be /mnt/cdrom for example */
#define S_DVD_MOUNT_NAME  "./dvd"

/* Text ID for update version */
#define S_TEXTID_VERSION "TEXTID_VERSION"

/* Text ID for update info */
#define S_TEXTID_INFO "TEXTID_INFO"

/* placeholder ethernet MAC until code for UtvPlatformGetEtherMAC() is available */
#define S_FAKE_ETHERNET_MAC    "11:22:33:44:55:66"

/* placeholder WiFi MAC until code for UtvPlatformGetWiFiMAC() is available */
#define S_FAKE_WIFI_MAC    "77:88:99:00:11:22"

/* Max abort wait timeout */
#define S_MAX_ABORT_WAIT_IN_MSECS 5000

/* Max time to delay between checking whether agent threads have gone idle */
#define S_THREAD_CHECK_GRANULARITY_IN_MSECS 10

/*
*****************************************************************************************************
**
** STATIC VARS
**
*****************************************************************************************************
*/
static UTV_BOOL s_bUtvInit = UTV_FALSE;
static UTV_RESULT s_rSecureInit = UTV_NOT_INITIALIZED;
static UTV_MUTEX_HANDLE s_AccessMutex;
static UTV_INT8 s_cUpdateFilePath[ S_FILE_PATH_MAX ] = { 0 };
static UTV_PUBLIC_CALLBACK s_fCallbackFunction = NULL;
static UTV_PROJECT_GET_SCHEDULE_REQUEST    s_scanRequest;
static UTV_PROJECT_DOWNLOAD_REQUEST    s_downloadRequest;
#ifdef DECLARE_NETWORK_UP
static UTV_BOOL s_bNetworkUp = UTV_TRUE;
#else
static UTV_BOOL s_bNetworkUp = UTV_FALSE;
#endif
#ifdef UTV_LIVE_SUPPORT
static UTV_BOOL s_bProductRegistered = UTV_FALSE;
#endif

/* ULPK context flag.  True means use factory path.  False means use read-only path. 
   Factory context is always off by default. */
UTV_BOOL s_bULPKFactoryContext = UTV_FALSE;

/* Android specific helper function static vars */

static UTV_INT8 s_installPath[256];
static UTV_BOOL s_updateReadyToInstall = UTV_FALSE;
static UTV_PUBLIC_SCHEDULE *s_pCurrentUpdateScheduled = NULL;

UTV_INT8 UTV_PLATFORM_PERSISTENT_PATH[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_ULID_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_FACTORY_PATH[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_READ_ONLY_ULPK_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_ULPK_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_SERIAL_NUMBER_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_PROVISIONER_PATH[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_READ_ONLY_PATH[256]={0};
UTV_INT8 UTV_PLATFORM_INTERNET_SSL_CA_CERT_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_UPDATE_STORE_0_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_UPDATE_STORE_1_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_PERSISTENT_STORAGE_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_KEYSTORE_FNAME[256]={0};
UTV_INT8 UTV_PLATFORM_REGSTORE_STORE_AND_FWD_FNAME[256]={0};

/*
*****************************************************************************************************
**
** STATIC FUNCTIONS
**
*****************************************************************************************************
*/
static void *s_UtvGetScheduleThread( void *pArg );
static void *s_UtvDownloadThread( void *pArg );
static void *s_UtvInstallFromMediaThread( void *pArg );
static UTV_RESULT s_UtvSetUpdateFilePath( UTV_INT8 *pszFilePath );
static UTV_RESULT s_GetScheduleBlocking(void *pArg );
static UTV_RESULT s_AESDecrypt( UTV_BYTE *encData, UTV_UINT32 uiencDataSize,UTV_BYTE **clearData,UTV_UINT32 *uiclearDataSize,  UTV_BYTE *aesKey, UTV_BYTE *aesVector );
static void s_SetDefaultULPKContext( void );
static void s_SetFactoryULPKContext( void );

/*
*****************************************************************************************************
**
** Define _OTP_TRANSIT_KEY to enable transit key OTP sub-system
**
*****************************************************************************************************
*/

#define _OTP_TRANSIT_KEY
#ifdef _OTP_TRANSIT_KEY

/* function that will OTP the transit key */
static UTV_RESULT s_SC_OTPTransitKey(UTV_BYTE *pucTransitKey, UTV_UINT32 uiLength);

/* length of the transit key material */
#define OTP_TRANSIT_KEY_LEN 16

/* Define secure core error values here */
#define SC_ERROR_OK                                0
#define SC_ERROR_TransitKeyAlreadyBurned           10001

#endif

/*
** Declare the methods used to handle LAN Remote Access commands.  If certain commands are not
** handled on a given platform they should be removed from this section.
*/
#ifdef UTV_LAN_REMOTE_CONTROL_WEB_SERVER
static UTV_RESULT s_LanRemoteControlRemoteButton( cJSON* inputs, cJSON* outputs );
#endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */

/*
** Declare the methods used to handle Network Assistance commands.  If certain commands are not
** handled on a given platform they should be removed from this section.
*/
#ifdef UTV_LOCAL_SUPPORT_WEB_SERVER
static UTV_RESULT s_LocalSupportGetDeviceInfo( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LocalSupportGetNetworkInfo( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LocalSupportNetworkDiagnostics( cJSON* inputs, cJSON* outputs );
#endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */

/*
** Declare the methods used to handle live support commands.  If certain commands are not handled
** on a given platform they should be removed from this section.
*/
#ifdef UTV_LIVE_SUPPORT
static void s_LiveSupportRegisterCommandHandler_RegisterProduct( void );
static UTV_RESULT s_LiveSupportProcessRemoteButton( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetDeviceInfo( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportCheckForUpdates( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportUpdateSoftware( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportCheckProductRegistered( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportRegisterProduct( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportProductReturnPreProcess( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetNetworkInfo( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportNetworkDiagnostics( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportHardwareDiagQuick( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportHardwareDiagFull( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportCheckInputs( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetPartsList( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetSystemStatistics( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportResetFactoryDefaults( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportSetAspectRatio( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportApplyVideoPattern( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetDeviceConfig( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportSetDeviceConfig( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportSetDateTime( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportCustomToolSample( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportCustomDiagnosticSample( cJSON* inputs, cJSON* outputs );
#endif /* UTV_LIVE_SUPPORT */

/*
*****************************************************************************************************
**
** EVENT DRIVEN FUNCTIONS THAT MUST BE CALLED BY YOUR SYSTEM
** The following functions need to be called by your application when specified events are detected.
** They should not need to be modified.
**
*****************************************************************************************************
*/

/* UtvProjectOnEarlyBoot
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED --ONCE--- ON SYSTEM BOOT ***
    You should not have to modify this code.
*/

void UtvProjectOnEarlyBoot( void )
{
    UTV_RESULT rStatus;

    /* init early boot machinery that has very little system service overhead */
    UtvPublicAgentEarlyInit();
    
    /* init access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexInit( &s_AccessMutex ) ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexInit( &s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
    }
}


/* s_Init
    *** PORTING STEP:  Add system dependent initialization code if necessary where indicated. ***
*/

static void s_Init( void )
{
    UTV_RESULT rStatus;

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return;
    }

    if ( s_bUtvInit )
    {
        /* no problem, already initialized */
        if ( UTV_OK != ( rStatus = UtvMutexUnlock( s_AccessMutex )))
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        }
        return;
    }
    
    UtvPublicAgentInit();

    /* perform any additional initialization that needs to wait for the system to come up here */

    s_bUtvInit = UTV_TRUE;

    UtvMessage( UTV_LEVEL_INFO, "Agent has been initialized." );

    if ( UTV_OK != ( rStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
    }
}

/* UtvProjectOnShutdown
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED ONCE ON SYSTEM SHUTDOWN ***
    You should not have to modify this code.
*/
void UtvProjectOnShutdown( void )
{
    UTV_RESULT rStatus;

    if ( !s_bUtvInit )
        return;

    UtvMessage( UTV_LEVEL_INFO, "Shutting down the Agent." );

    UtvProjectSetUpdateSchedule(NULL);

    /* free access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexDestroy( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexDestroy( s_AccessMute ) \"%s\"", UtvGetMessageString( rStatus ) );
    }

    /* free up agent resources */
    UtvPublicAgentShutdown();

    s_bUtvInit = UTV_FALSE;
}

/* UtvProjectOnTestAgent
    *** PORTING STEP:  CALL THIS FUNCTION WHEN THE SYSTEM NEEDS TO KNOW IF THE AGENT IS ACTIVE ***
    Test agent activity. Return will indicate that the agent is uninitialized, busy, or idle (OK).
    Includes a wait time parameter. If zero, return immediately with current activity status. 
    If non-zero, the wait time is the maximum number of milliseconds the caller is willing to
    wait for the agent to go idle. A timeout is returned as a timeout.

    You should not have to modify this code.
*/
UTV_RESULT UtvProjectOnTestAgentActive( UTV_UINT32 msecsToWait )
{
    UTV_BOOL result;
    UTV_UINT32 waitTime;

    if ( !s_bUtvInit )
        return UTV_NOT_INITIALIZED;

    if ( 0 == msecsToWait )
    {
        result = UtvThreadsActive();
        return ( ( UTV_FALSE == result ) ? UTV_OK : UTV_BUSY );
    }

    do {
        result = UtvThreadsActive();
        if ( UTV_FALSE == result )
            return UTV_OK;

        if ( msecsToWait > S_THREAD_CHECK_GRANULARITY_IN_MSECS )
            waitTime = S_THREAD_CHECK_GRANULARITY_IN_MSECS;
        else
            waitTime = msecsToWait;
        UtvSleepNoAbort( waitTime );
        msecsToWait -= waitTime;

    } while ( msecsToWait > 0 );

    return UTV_TIMEOUT;
}

#ifdef UTV_DELIVERY_FILE
/* UtvProjectOnUSBInsertionBlocking
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED WHEN A USB DEVICE IS INSERTED ***
    You should not have to modify this code except to name the mount path and max index.
    It's assumed that the system auto-mounts the USB when it's inserted.
*/
UTV_RESULT UtvProjectOnUSBInsertionBlocking( UTV_INT8 *pszPath )
{
    UTV_INT8            cUpdatePath[ 256 ];
    UTV_INT8            cUpdateFile[ 256 ];

    if ( !s_bUtvInit )
        s_Init();

    UtvMemFormat( (UTV_BYTE *) cUpdatePath, sizeof(cUpdatePath), "%s", pszPath );
        UtvMemFormat( (UTV_BYTE *) cUpdateFile, sizeof(cUpdateFile), "%s/%s", cUpdatePath, UtvPlatformGetUpdateRedirectName() );
        UtvMessage( UTV_LEVEL_INFO, "Looking for update re-direct file in %s", cUpdateFile );

    if ( !UtvPlatformFilePathExists( cUpdateFile ) ){
        UtvMessage( UTV_LEVEL_INFO, "USB re-direct file NOT found" );
        g_pUtvDiagStats->utvLastError.usError = UTV_NO_DOWNLOAD;
        g_pUtvDiagStats->utvLastError.tTime = UtvTime( NULL );
        g_pUtvDiagStats->utvLastError.usCount++;
        return UTV_NO_DOWNLOAD;
    }

    /* let the system know where the update files can be found */
    s_UtvSetUpdateFilePath( cUpdatePath );

    /* build scan request for usb-based updates */
    s_scanRequest.pszThreadName = "usbscan";
    s_scanRequest.uiEventType = UTV_PROJECT_SYSEVENT_USB_SCAN_RESULT;
    s_scanRequest.bDeliveryFile = UTV_TRUE;
    s_scanRequest.bDeliveryBroadcast = UTV_FALSE;
    s_scanRequest.bDeliveryInternet = UTV_FALSE;
    s_scanRequest.bStored = UTV_TRUE;
    s_scanRequest.bFactoryMode = UTV_FALSE;

    return s_GetScheduleBlocking(&s_scanRequest);
}

/* UtvProjectOnDVDInsertion
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED WHEN A DVD IS INSERTED ***
    You should not have to modify this code except to name the device and max index.
*/
UTV_RESULT UtvProjectOnDVDInsertion( void )
{
    UTV_INT8            cUpdateFile[ 256 ];
    UTV_THREAD_HANDLE    hThread;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* look for the update root file */
    UtvMemFormat( (UTV_BYTE *) cUpdateFile, sizeof(cUpdateFile), "%s/%s", S_DVD_MOUNT_NAME, UtvPlatformGetUpdateRedirectName() );
    UtvMessage( UTV_LEVEL_INFO, "Looking for update re-direct file in %s", cUpdateFile );

    /* if it doesn't exist then punt */
    if ( !UtvPlatformFilePathExists( cUpdateFile ) )
    {
        UtvMessage( UTV_LEVEL_INFO, "DVD re-direct file NOT found" );
        return UTV_NO_DOWNLOAD;
    }

    /* let the system know where the update files can be found */
    s_UtvSetUpdateFilePath( S_DVD_MOUNT_NAME );

    /* build scan request for dvd-based updates */
    s_scanRequest.pszThreadName = "dvdscan";
    s_scanRequest.uiEventType = UTV_PROJECT_SYSEVENT_DVD_SCAN_RESULT;
    s_scanRequest.bDeliveryFile = UTV_TRUE;
    s_scanRequest.bDeliveryBroadcast = UTV_FALSE;
    s_scanRequest.bDeliveryInternet = UTV_FALSE;
    s_scanRequest.bStored = UTV_FALSE;
    s_scanRequest.bFactoryMode = UTV_FALSE;

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: s_UtvGetScheduleThread from\n%s:%d", __FILE__, __LINE__);
#endif
    return UtvThreadCreate( &hThread, s_UtvGetScheduleThread, &s_scanRequest );
}

#endif

/* UtvProjectOnScanForStoredUpdate
    *** PORTING STEP: None ***
*/
UTV_RESULT UtvProjectOnScanForStoredUpdate( UTV_PUBLIC_SCHEDULE *pSchedule )
{
    UTV_THREAD_HANDLE                    hThread;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* build scan request for stored updates */
    s_scanRequest.pszThreadName = "storescan";
    s_scanRequest.uiEventType = UTV_PROJECT_SYSEVENT_STORE_SCAN_RESULT;
    s_scanRequest.bDeliveryFile = UTV_TRUE;
    s_scanRequest.bDeliveryBroadcast = UTV_FALSE;
    s_scanRequest.bDeliveryInternet = UTV_FALSE;
    s_scanRequest.bStored = UTV_TRUE;
    s_scanRequest.bFactoryMode = UTV_FALSE;

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: s_UtvGetScheduleThread from\n%s:%d", __FILE__, __LINE__);
#endif
    return UtvThreadCreate( &hThread, s_UtvGetScheduleThread, &s_scanRequest );
}

/* UtvProjectOnScanForNetUpdate
    *** PORTING STEP: None ***
*/
UTV_RESULT UtvProjectOnScanForNetUpdate( UTV_PUBLIC_SCHEDULE *pSchedule )
{
    UTV_THREAD_HANDLE    hThread;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* build scan request for net updates */
    s_scanRequest.pszThreadName = "netscan";
    s_scanRequest.uiEventType = UTV_PROJECT_SYSEVENT_NETWORK_SCAN_RESULT;
    s_scanRequest.bDeliveryFile = UTV_FALSE;
    s_scanRequest.bDeliveryInternet = UTV_TRUE;
    s_scanRequest.bDeliveryBroadcast = UTV_TRUE;
    s_scanRequest.bStored = UTV_FALSE;
    s_scanRequest.bFactoryMode = UTV_FALSE;

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: s_UtvGetScheduleThread from\n%s:%d", __FILE__, __LINE__);
#endif
    return UtvThreadCreate( &hThread, s_UtvGetScheduleThread, &s_scanRequest );
}

UTV_RESULT UtvProjectOnScanForOTAUpdate()
{

    if ( !s_bUtvInit )
        s_Init();

    /* build scan request for net updates */
    s_scanRequest.pszThreadName = "netscan";
    s_scanRequest.uiEventType = UTV_PROJECT_SYSEVENT_NETWORK_SCAN_RESULT;
    s_scanRequest.bDeliveryFile = UTV_FALSE;
    s_scanRequest.bDeliveryInternet = UTV_TRUE;
    s_scanRequest.bDeliveryBroadcast = UTV_TRUE;
    s_scanRequest.bStored = UTV_FALSE;
    s_scanRequest.bFactoryMode = UTV_FALSE;

    return s_GetScheduleBlocking(&s_scanRequest);  
}

#ifdef UTV_DELIVERY_FILE
/* UtvProjectOnInstallFromMedia
    *** PORTING STEP: None ***
*/
UTV_RESULT UtvProjectOnInstallFromMedia( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_THREAD_HANDLE    hThread;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: s_UtvInstallFromMediaThread from\n%s:%d", __FILE__, __LINE__);
#endif
    return UtvThreadCreate( &hThread, s_UtvInstallFromMediaThread, pUpdate );
}
#endif

/* UtvProjectOnUpdate
    *** PORTING STEP: None ***
*/
UTV_RESULT UtvProjectOnUpdate( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_THREAD_HANDLE                hThread;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    s_downloadRequest.pszThreadName = "netdload";
    s_downloadRequest.pUpdate = pUpdate;
    s_downloadRequest.bFactoryMode = UTV_FALSE;

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: s_UtvDownloadThread from\n%s:%d", __FILE__, __LINE__);
#endif
    return UtvThreadCreate( &hThread, s_UtvDownloadThread, &s_downloadRequest );
}


#ifdef UTV_DELIVERY_INTERNET
/**
 **********************************************************************************************
 *
 * This function is called by the system's supervising program when the network first comes up.
 * It causes the Device Agent to make contact with the NetReady Services if the last time the
 * contact was made was 24 hours or more ago.
 *
 * The actual network communication and local cache management occurs asynchronously in its
 * own thread.  Therefore, this function call will return almost instantly.
 *
 * This function does not return any value because it is assumed to be called by an event
 * handler.
 * 
 * @note
 *
 * Calls to this function typically trigger off of a DHCP complete event, or similar in the
 * case of a static IP address network configuration.
 *
 **********************************************************************************************
 */

void UtvProjectOnNetworkUp( void )
{
    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* Let UtvPlatformNetworkUp() know that the network is up.
       This must take place before the provisioning thread is called */
    s_bNetworkUp = UTV_TRUE;

#ifdef UTV_LIVE_SUPPORT
    /*
     * If Live Support is enabled attempt to resume any stored session.
     */

    UtvLiveSupportSessionResume();
#endif

    /* removed provision thread on network up in Android because Network Up 
       is always accompanied by a check for update which will accomplish exactly the same thing */

    return;
}

/**
 **********************************************************************************************
 *
 * This function is called by the system's supervising program when it would like the next
 * operation to force a NetReady Services Update Query 
 *
 * Normally, only one Update Query can occur in some time period (usually 24 hours) specified by
 * the NetReady Services. This override is only in effect once.  Additional calls to this function
 * must be made to force the device to contact the NetReady Services for subsequent operations.
 * 
 **********************************************************************************************
 */

void UtvProjectOnForceNOCContact( void )
{
    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* invalidate update query cache */
    UtvInternetOverrideValidQueryResponseCached( UTV_FALSE );

    return;
}

#ifndef UTV_FILE_BASED_PROVISIONING
/**
 **********************************************************************************************
 *
 * This function is used to test for an object and if found, return its status. The status of
 * an object will indicate if the object has been changed and also how many times another 
 * entity has requested it using UtvPlatformProvisionerGetObject(). Note that the status info
 * is only valid for the present instanstiation of the agent; a power-off will reset the status.
 * 
 * It is thread-safe if other provisioning operations are taking place at the same time.
 *
 * @param   pszOwner        Name of object's owner (see note below)
 *
 * @param   pszName         Name of object (see note below)
 *
 * @param   puiStatus       Container to return status info about the object if found
 *
 * @param   puiRequestCount Container to return number of times this object has been accessed
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 * @note
 *
 * The name and owner strings are not required to have any particular format, but production 
 * objects will often have name strings that are portions of UUID strings. For example, the names
 * 1bba3829ce55 and 2be866b888c1 are not unusual as the Object Name and Owner strings.
 *
 *****************************************************************************************************
*/

UTV_RESULT UtvProjectOnObjectRequestStatus( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                            UTV_UINT32 *puiStatus, UTV_UINT32 *puiRequestCount  )
{
    UTV_RESULT rStatus, lStatus;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    UtvDebugSetThreadName( "getobjstatus" );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    rStatus = UtvPlatformProvisionerGetObjectStatus( pszOwner, pszName, puiStatus, puiRequestCount );

    /* release access mutex */
    if ( UTV_OK != ( lStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( lStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is used to test for an object and fetch its size. It is intended as a
 * companion function to UtvProjectOnObjectRequest() so an adequately sized buffer can be
 * allocated.
 * 
 * It is thread-safe if other provisioning operations are taking place at the same time.
 *
 * @param   pszOwner        Name of object's owner (see note below)
 *
 * @param   pszName         Name of object (see note below)
 *
 * @param   puiObjectSize   Container to return size of object
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 * @note
 *
 * The name and owner strings are not required to have any particular format, but production 
 * objects will often have name strings that are portions of UUID strings. For example, the names
 * 1bba3829ce55 and 2be866b888c1 are not unusual as the Object Name and Owner strings.
 *
 *****************************************************************************************************
*/

UTV_RESULT UtvProjectOnObjectRequestSize( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                          UTV_UINT32 *puiObjectSize)
{
    UTV_RESULT rStatus, lStatus;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    UtvDebugSetThreadName( "getobjsize" );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    rStatus = UtvPlatformProvisionerGetObjectSize( pszOwner, pszName, puiObjectSize );

    /* release access mutex */
    if ( UTV_OK != ( lStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( lStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is used by CSP client programs to access their provisioned objects. If
 * successful, it will return a copy of the object in addition to the object's size, its 
 * encryption type, and its Object ID in the buffers provided. If encrypted, the code will
 * attempt to decrypt the object if possible.
 *
 * It is recommended that UtvProjectOnObjectRequestSize() be called prior to this to ensure
 * that an appropriately sized buffer is used.
 * 
 * It is thread-safe if other provisioning operations are taking place at the same time.
 *
 * @param   pszOwner        Name of object's owner (see note below)
 *
 * @param   pszName         Name of object (see note below)
 *
 * @param pszObjectIDBuffer Container to return Object ID (if any)
 *
 * @param uiObjectIDBufferSize Size of pszObjectIDBuffer
 *
 * @param pubBuffer         Container to return Object itself
 *
 * @param uiBufferSize      Size of pubBuffer
 *
 * @param puiObjectSize     Actual size of object
 *
 * @param puiEncryptionType Container to return the object's encryption type (see 
 * the UTV_PROVISIONER_ENCRYPTION_ types defined in utv-core.h)
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 * @note
 *
 * The name and owner strings are not required to have any particular format, but production 
 * objects will often have name strings that are portions of UUID strings. For example, the names
 * 1bba3829ce55 and 2be866b888c1 are not unusual as the Object Name and Owner strings.
 *
 *****************************************************************************************************
*/

UTV_RESULT UtvProjectOnObjectRequest( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                      UTV_INT8 *pszObjectIDBuffer, UTV_UINT32 uiObjectIDBufferSize,
                                      UTV_BYTE *pubBuffer, UTV_UINT32 uiBufferSize,
                                      UTV_UINT32 *puiObjectSize, UTV_UINT32 *puiEncryptionType )
{
    UTV_RESULT rStatus, lStatus;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    UtvDebugSetThreadName( "getobj" );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* Get the object from provisioner */
    rStatus = UtvPlatformProvisionerGetObject( pszOwner, pszName,
                                               pszObjectIDBuffer, uiObjectIDBufferSize,
                                               pubBuffer, uiBufferSize,
                                               puiObjectSize, puiEncryptionType );

    /* decrypt externally if asked for */
    if ( UTV_OK == rStatus && UTV_PROVISIONER_ENCRYPTION_3DES_UR <= *puiEncryptionType )
    {
        rStatus = UtvPlatformSecureDecryptDirect( pubBuffer, puiObjectSize );
    }

    /* release access mutex */
    if ( UTV_OK != ( lStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( lStatus ) );
    }

    return rStatus;
}
#else
/**
 **********************************************************************************************
 *
 * This function is called automatically by the provisioning sub-system when delivery info is 
 * added to the object.  Note that the integrator does NOT call this function.  It is called 
 * for them and they adapt it to do what they want with the object that is delivered.
 *
 * @param   pubObject            Pointer to the object which may or may not be encrypted.
 *
 * @param   uiObjectSize         Size of the encrypted object in bytes.
 *
 * @param   uiClearObjectSize    Size of the decrypted object in bytes.
 *
 * @param   pszName              Object name (12-byte GUID)
 *
 * @param   pszOwner             Object owner (12-byte GUID)
 *
 * @param   pszObjectID          Object ID (optional device-unique object identifier)
 *
 * @param   uiEncryptionType     Encryption type.  See enc type definitions in utv-core.h
 *
 * @param   pszDeliveryinfo      Delivery info string set on the NOC when the object was created.
 *
 * @param   uiOperation          UTV_PROVISION_OP_INSTALL or UTV_PROVISION_OP_DELETE
 *
 * @return  void
 *
 *****************************************************************************************************
*/
void UtvPlatformOnProvisioned(UTV_BYTE *pubObject, UTV_UINT32 uiObjectSize, UTV_UINT32 uiClearObjectSize,
                              UTV_INT8 *pszName, UTV_INT8 *pszOwner, UTV_INT8 *pszObjectID, UTV_UINT32 uiEncryptionType, 
                              UTV_INT8 *pszDeliveryInfo, UTV_UINT32 uiOperation )
{
    UTV_RESULT rStatus;
    UTV_UINT32 uiFileHandle, uiBytesWritten;
    UTV_INT8   ubInstallPath[ 256 ];
    
    UtvMessage( UTV_LEVEL_INFO, 
                "UtvProjectOnProvisioned Encrypted Size = %d, Clear Size = %d, Name = %s, Owner = %s, ID = %s, Encrypt Type: 0x%X, DeliveryInfo = %s, Operation: %d",
                uiObjectSize, uiClearObjectSize, pszName, pszOwner, pszObjectID, uiEncryptionType, pszDeliveryInfo, uiOperation );

    if ( UTV_PROVISION_OP_INSTALL != uiOperation && UTV_PROVISION_OP_DELETE != uiOperation )
    {
        UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, 
                           UTV_UNKNOWN, __LINE__, pszDeliveryInfo );
        return;
    }
    
    /* Look at the delivery info an figure out what to do with the object */
    if ( 0 == UtvMemcmp( (UTV_BYTE *) S_OBJECT1_NAME, 
                         (UTV_BYTE *) pszDeliveryInfo, 
                         UtvStrlen( (UTV_BYTE *) S_OBJECT1_NAME )))
    {
        /* decide where to put it */
        UtvMemFormat( (UTV_BYTE *) ubInstallPath, sizeof(ubInstallPath), 
                      "./%s", S_OBJECT1_SAVED_NAME  );

        if ( UTV_PROVISION_OP_DELETE == uiOperation )
        {
            if ( UTV_OK != ( rStatus = UtvPlatformFileDelete( ubInstallPath )))
            {
                UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, 
                                   rStatus, __LINE__, ubInstallPath );
            } else
            {
                UtvMessage( UTV_LEVEL_INFO, "Deleted PR model cert %s", ubInstallPath );
            }

            return;
        }

        UtvMessage( UTV_LEVEL_INFO, "Installing object 1 into %s", ubInstallPath );

        if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( ubInstallPath, "wb", &uiFileHandle )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, 
                               rStatus, __LINE__, ubInstallPath );
            return;
        }

        /* attempt to write the entire object in one shot */
        if ( UTV_OK != ( rStatus = UtvPlatformFileWriteBinary( uiFileHandle, 
                                                               pubObject, uiObjectSize, &uiBytesWritten )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, 
                               rStatus, __LINE__, ubInstallPath );
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "object 1 cert installed into %s", ubInstallPath );
        }
    
        UtvPlatformFileClose( uiFileHandle );

        return;
    }

    return;
}
#endif
#endif

/* UtvProjectOnAbort
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED WHEN THE USER INTERRUPTS A DOWNLOAD OR OTHER
                       UTV OPERATION ***
    You should not have to modify this code.
*/
void UtvProjectOnAbort( void )
{
    UTV_RESULT    rStatus;
    
    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* abort the Agent */
    UtvUserAbort();

    /* wait for the Agent to abort or 1 second whichever is shorter */
    rStatus = UtvProjectOnTestAgentActive( S_MAX_ABORT_WAIT_IN_MSECS );

    /* print out the wait result for logging purposes */
    UtvMessage( UTV_LEVEL_INFO, "UtvProjectOnTestAgentActive() returns: %s",
                UtvGetMessageString( rStatus ) );

    /* clear the abort state */
    UtvClearState();
            
    /* clear the install machinery */
    UtvPlatformInstallCleanup();
}

/* UtvProjectOnStatusRequested
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED WHEN THE SYSTEM WOULD LIKE TO GET THE AGENT'S STATUS ***
    You should not have to modify this code.
*/
UTV_RESULT UtvProjectOnStatusRequested( UTV_PROJECT_STATUS_STRUCT *pStatus )
{
    UTV_RESULT    rStatus;
    UTV_INT8   *pszText;

    /* reject NULL ptr */
    if ( NULL == pStatus )
        return UTV_NULL_PTR;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    pStatus->uiOUI = UtvCEMGetPlatformOUI();
    pStatus->usModelGroup = UtvCEMGetPlatformModelGroup();
    pStatus->usHardwareModel = UtvCEMTranslateHardwareModel();
    pStatus->usSoftwareVersion = UtvManifestGetSoftwareVersion();
    pStatus->usModuleVersion = UtvManifestGetModuleVersion();
    pStatus->usNumObjectsProvisioned = 0;
    UtvInternetProvisionerGetCount( (UTV_UINT32 *)&pStatus->usNumObjectsProvisioned );

    pStatus->usStatusMask = 0;
    if ( UtvInternetRegistered() )
        pStatus->usStatusMask |= UTV_PROJECT_STATUS_MASK_REGISTERED;
    if ( UtvManifestGetFactoryTestStatus() )
        pStatus->usStatusMask |= UTV_PROJECT_STATUS_MASK_FACTORY_TEST_MODE;
    if ( UtvManifestGetFieldTestStatus() )
        pStatus->usStatusMask |= UTV_PROJECT_STATUS_MASK_FIELD_TEST_MODE;
    if ( UtvManifestGetLoopTestStatus() )
        pStatus->usStatusMask |= UTV_PROJECT_STATUS_MASK_LOOP_TEST_MODE;

    if ( UTV_OK != ( rStatus = UtvCommonDecryptGetPKID( &pStatus->uiULPKIndex )))
        pStatus->uiULPKIndex = 0;

    if ( ( NULL != pStatus->pszESN ) && UTV_OK != ( rStatus = UtvCEMGetSerialNumber( pStatus->pszESN, pStatus->uiESNBufferLen )))
        pStatus->pszESN = NULL;

    if ( NULL != UtvCEMGetInternetQueryHost() && NULL != pStatus->pszQueryHost )
    {
        // simplify the query host name to avoid exposing the url
        if ( UtvMemcmp( "extdev", UtvCEMGetInternetQueryHost(), 6 ))
            UtvStrnCopy( (UTV_BYTE *)pStatus->pszQueryHost, pStatus->uiQueryHostBufferLen,
                         (UTV_BYTE *)"production", 10 );
        else
        UtvStrnCopy( (UTV_BYTE *)pStatus->pszQueryHost, pStatus->uiQueryHostBufferLen,
                         (UTV_BYTE *)"extdev", 6 );
    } else
    {
        pStatus->pszQueryHost = NULL;
    }

    if ( UTV_OK == ( rStatus = UtvPublicImageGetUpdateTextDef( g_hManifestImage, S_TEXTID_VERSION, &pszText )) && NULL != pStatus->pszVersion )
    {
        pStatus->pszVersion = pszText;
    } else
    {
        pStatus->pszVersion = NULL;
    }

    if ( UTV_OK == ( rStatus = UtvPublicImageGetUpdateTextDef( g_hManifestImage, S_TEXTID_INFO, &pszText )) && NULL != pStatus->pszInfo )
    {
        UtvStrnCopy( (UTV_BYTE *)pStatus->pszInfo, pStatus->uiInfoBufferLen, (UTV_BYTE *) pszText, UtvStrlen( (UTV_BYTE *) pszText) );
    } else
    {
        pStatus->pszInfo = NULL;
    }

    pStatus->uiErrorCount = g_pUtvDiagStats->utvLastError.usCount;
    pStatus->uiLastErrorCode = g_pUtvDiagStats->utvLastError.usError;
    pStatus->tLastErrorTime = g_pUtvDiagStats->utvLastError.tTime;

#ifdef UTV_TEST_MESSAGES
    if ( NULL != pStatus->pszLastError )
    {
        UtvStrnCopy( (UTV_BYTE *)pStatus->pszLastError, pStatus->uiLastErrorBufferLen, (UTV_BYTE *)
                     UtvGetMessageString( pStatus->uiLastErrorCode ),
                     UtvStrlen( (UTV_BYTE *) UtvGetMessageString( pStatus->uiLastErrorCode )) );
    } else
    {
        pStatus->pszLastError = NULL;
    }
#else
    pStatus->pszLastError = NULL;
#endif
    return UTV_OK;
}

/* UtvProjectOnUpdateInfo
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED WHEN THE SYSTEM WOULD LIKE TO GET
                       INFORMATION ABOUT A PENDING UPDATE ***
    You should not have to modify this code.
*/
UTV_RESULT UtvProjectOnUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_PROJECT_UPDATE_INFO *pInfo )
{
    UTV_RESULT    rStatus;
    UTV_INT8   *pszText;

    /* reject NULL ptr */
    if ( NULL == pUpdate || NULL == pInfo )
        return UTV_NULL_PTR;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    /* use the info in the update schdule to get the update information */
    if ( UTV_OK == ( rStatus = UtvPublicImageGetUpdateTextDef( pUpdate->hImage, S_TEXTID_VERSION, &pszText )) && NULL != pInfo->pszVersion )
    {
        UtvStrnCopy( (UTV_BYTE *)pInfo->pszVersion, pInfo->uiVersionBufferLen, (UTV_BYTE *) pszText, UtvStrlen( (UTV_BYTE *) pszText) );
    } else
    {
        pInfo->pszVersion = NULL;
        return UTV_TEXT_DEF_NOT_FOUND;
    }

    if ( UTV_OK == ( rStatus = UtvPublicImageGetUpdateTextDef( pUpdate->hImage, S_TEXTID_INFO, &pszText )) && NULL != pInfo->pszInfo )
    {
        UtvStrnCopy( (UTV_BYTE *)pInfo->pszInfo, pInfo->uiInfoBufferLen, (UTV_BYTE *) pszText, UtvStrlen( (UTV_BYTE *) pszText) );
    } else
    {
        pInfo->pszInfo = NULL;
        return UTV_TEXT_DEF_NOT_FOUND;
    }

    return UTV_OK;
}

UTV_RESULT UtvProjectGetAvailableUpdateInfo( UTV_INT8 *pszInfo, UTV_UINT32 uiInfo, UTV_INT8 *pszVersion, UTV_UINT32 uiVersion, UTV_INT8 *pszDesc, UTV_UINT32 uiDesc,  UTV_INT8 *pszType, UTV_UINT32 uiType )
{
    UTV_INT8 *pszText = NULL;
    UTV_RESULT rStatus = UTV_OK;

    if ( !s_bUtvInit )
        s_Init();

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        return UTV_NO_MUTEX;
    }

     if ( NULL != s_pCurrentUpdateScheduled ) 
     {
        if (s_pCurrentUpdateScheduled->tUpdates[ 0 ].uiDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_INTERNET){
            pszText = "OTA";
            UtvStrnCopy( (UTV_BYTE *)pszType, uiType, (UTV_BYTE *)pszText, UtvStrlen( (UTV_BYTE *)pszText ));
        }
        if (s_pCurrentUpdateScheduled->tUpdates[ 0 ].uiDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_FILE){
            pszText = "File";
            UtvStrnCopy( (UTV_BYTE *)pszType, uiType, (UTV_BYTE *)pszText, UtvStrlen( (UTV_BYTE *)pszText ));
        }
        if ( NULL != pszInfo )
        {
            pszText = NULL;
            if( UTV_OK == (rStatus = UtvPublicImageGetUpdateTextDef( s_pCurrentUpdateScheduled->tUpdates[ 0 ].hImage, "TEXTID_INFO", &pszText )))
                UtvStrnCopy( (UTV_BYTE *)pszInfo, uiInfo, (UTV_BYTE *)pszText, UtvStrlen( (UTV_BYTE *)pszText ));
            else
                UtvMessage( UTV_LEVEL_WARN, "UtvProjectGetAvailableUpdateInfo TEXTID_INFO not found %s", UtvGetMessageString( rStatus ));
        }
        if ( NULL != pszVersion )
        {
            pszText = NULL;
            if( UTV_OK == (rStatus = UtvPublicImageGetUpdateTextDef( s_pCurrentUpdateScheduled->tUpdates[ 0 ].hImage, "TEXTID_VERSION", &pszText )))
                UtvStrnCopy( (UTV_BYTE *)pszVersion, uiVersion, (UTV_BYTE *)pszText, UtvStrlen( (UTV_BYTE *)pszText ));
            else
                UtvMessage( UTV_LEVEL_WARN, "UtvProjectGetAvailableUpdateInfo TEXTID_VERSION not found %s", UtvGetMessageString( rStatus )); 
        }
        if ( NULL != pszDesc ) 
        {
            pszText = NULL;
            if( UTV_OK == (rStatus = UtvPublicImageGetUpdateTextDef( s_pCurrentUpdateScheduled->tUpdates[ 0 ].hImage, "TEXTID_DESCRIPTION", &pszText )))
                UtvStrnCopy( (UTV_BYTE *)pszDesc, uiDesc, (UTV_BYTE *)pszText, UtvStrlen( (UTV_BYTE *)pszText ));
            else
                UtvMessage( UTV_LEVEL_WARN, "UtvProjectGetAvailableUpdateInfo TEXTID_DESCRIPTION not found %s", UtvGetMessageString( rStatus ));
        }
    } 
    
    /* release access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/* UtvProjectOnDebugInfo
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to return extensive debug info about the system
    during development.
*/
void UtvProjectOnDebugInfo( void )
{
    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    UtvDebugSetThreadName( "dbg info" );

    UtvAnnouceConfiguration();

    return;
}

/* UtvProjectOnRegisterCallback
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED ON SYSTEM INITIALIZATION SO THAT THE
                       SUPERVISING APPLICATION RECEIVES CALLBACK EVENTS ***
    You should not have to modify this code.
*/
void UtvProjectOnRegisterCallback( UTV_PUBLIC_CALLBACK pCallback )
{
    s_fCallbackFunction = pCallback;
}

/* UtvProjectOnResetComponentManifest
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED BY YOUR SYSTEM FOR TEST PURPOSES TO
                       REVERT TO THE BACKUP COMPONENT MANIFEST ***
    You should not have to modify this code.
*/
UTV_RESULT UtvProjectOnResetComponentManifest( void )
{
    UTV_RESULT    rStatus;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    UtvMessage( UTV_LEVEL_INFO, "RESETTING MANIFEST FROM %s TO %s",
                UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME, UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME );

    if ( UTV_OK != ( rStatus = UtvPlatformFileCopy( UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME, UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileCopy() \"%s\"", UtvGetMessageString( rStatus ) );
    }

    /* reinit agent to pickup new component manifest */
    UtvProjectOnShutdown();
    UtvProjectOnEarlyBoot();

    return rStatus;
}

/* UtvProjectOnFactoryMode
    *** PORTING STEP:  MAKE SURE THIS FUNCTION GETS CALLED BY YOUR SYSTEM FOR TEST PURPOSES TO
                       WHEN IT ENTERS FACTORY MODE ***
    You should not have to modify this code.
*/
UTV_RESULT UtvProjectOnFactoryMode( void )
{
    /* not used in Android */
    return UTV_UNKNOWN;
}

/* UtvProjectManageRegStore
   *** PORTING STEP:  NONE. ***
*/
UTV_RESULT UtvProjectManageRegStore( UTV_UINT32 uiNumStrings, UTV_INT8 *apszString[] )
{
    if ( !s_bUtvInit )
        s_Init();

    return UtvPublicInternetManageRegStore( uiNumStrings, apszString );
}

#ifdef UTV_LOCAL_SUPPORT
/* UtvProjectOnLocalSupportGetDeviceInfo
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to retrieve the Local Support
    Device Information.
*/
UTV_RESULT UtvProjectOnLocalSupportGetDeviceInfo( UTV_BYTE** ppszDeviceInfo )
{
  UTV_RESULT result = UTV_OK;
  cJSON* pDeviceInfo = NULL;

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  pDeviceInfo = cJSON_CreateObject();

  if ( NULL == pDeviceInfo )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvCEMGetDeviceInfo( pDeviceInfo );

  if ( UTV_OK == result )
  {
    *ppszDeviceInfo = (UTV_BYTE*)cJSON_PrintUnformatted( pDeviceInfo );
  }

  cJSON_Delete( pDeviceInfo );

  return ( result );
}

/* UtvProjectOnLocalSupportGetNetworkConfiguration
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to retrieve the Local Support
    Network Configuration.
*/
UTV_RESULT UtvProjectOnLocalSupportGetNetworkInfo( UTV_BYTE** ppszNetworkInfo )
{
  UTV_RESULT result = UTV_OK;
  cJSON* pNetworkInfo = NULL;

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  pNetworkInfo = cJSON_CreateObject();

  if ( NULL == pNetworkInfo )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvCEMGetNetworkInfo( pNetworkInfo );

  if ( UTV_OK == result )
  {
    *ppszNetworkInfo = (UTV_BYTE*)cJSON_PrintUnformatted( pNetworkInfo );
  }

  cJSON_Delete( pNetworkInfo );

  return ( result );
}

/* UtvProjectOnLocalSupportNetworkDiagnostics
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to test execute the Local Support
    Network Diagnostics.
*/
UTV_RESULT UtvProjectOnLocalSupportNetworkDiagnostics( UTV_BYTE** ppszNetworkDiagnostics )
{
  UTV_RESULT result = UTV_OK;
  cJSON* pNetworkDiagnostics = NULL;

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  pNetworkDiagnostics = cJSON_CreateObject();

  if ( NULL == pNetworkDiagnostics )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvCEMNetworkDiagnostics( pNetworkDiagnostics );

  if ( UTV_OK == result )
  {
    *ppszNetworkDiagnostics = (UTV_BYTE*)cJSON_PrintUnformatted( pNetworkDiagnostics );
  }

  cJSON_Delete( pNetworkDiagnostics );

  return ( result );
}
#endif /* UTV_LOCAL_SUPPORT */

#ifdef UTV_LIVE_SUPPORT
/**
 **********************************************************************************************
 *
 * This function is used to test connectivity to the NetReady Services.  The connectivity test
 * requires that the network interface(s) have been activated.  This requirement is validated
 * using UtvPlatformNetworkUp().
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 * @sa      UtvPlatformNetworkUp
 *
 **********************************************************************************************
 */

UTV_RESULT UtvProjectOnLiveSupportTestConnection( void )
{
  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  return ( UtvLiveSupportTestConnection() );
}

/**
 **********************************************************************************************
 *
 * This function is used to determine if a SupportTV session is current active.
 *
 * @return  UTV_TRUE if a SupportTV session is active.
 * @return  UTV_FALSE if a SupportTV session IS NOT active.
 *
 * @sa      UtvLiveSupportIsSessionActive
 *
 **********************************************************************************************
 */

UTV_BOOL UtvProjectOnLiveSupportIsSessionActive( void )
{
  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  return ( UtvLiveSupportIsSessionActive() );
}

/**
 **********************************************************************************************
 *
 * This function is used to initiate a SupportTV session with the NetReady Services.
 *
 * @param       pbSessionCode - Pointer to a buffer to hold the session code associated with
 *                              the SupportTV session.  The buffer must be at least
 *                              SESSION_CODE_SIZE bytes to hold the session code.
 *
 * @param       uiSessionCodeSize - Size of the buffer to hold the session code.
 *
 * @return      UTV_TRUE if a SupportTV session is active.
 * @return      UTV_FALSE if a SupportTV session IS NOT active.
 *
 * @sa          UtvProjectOnLiveSupportInitiateSession1
 *
 * @deprecated  Use UtvProjectOnLiveSupportInitiateSession1
 *
 **********************************************************************************************
 */

UTV_RESULT UtvProjectOnLiveSupportInitiateSession(
  UTV_BYTE* pbSessionCode,
  UTV_UINT32 uiSessionCodeSize )
{
  UTV_RESULT result;
  UtvLiveSupportSessionInitiateInfo1 sessionInitiateInfo;
  UtvLiveSupportSessionInfo1 sessionInfo;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == pbSessionCode || SESSION_CODE_SIZE > uiSessionCodeSize )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvMemset( &sessionInitiateInfo, 0x00, sizeof( sessionInitiateInfo ) );

  /*
   * Initialize the 'allowSessionCodeBypass' flag to UTV_FALSE to ensure the correct default
   * value.
   */

  sessionInitiateInfo.allowSessionCodeBypass = UTV_FALSE;

  result = UtvLiveSupportInitiateSession1( &sessionInitiateInfo, &sessionInfo );

  if ( UTV_OK == result )
  {
    UtvStrnCopy(
      pbSessionCode,
      uiSessionCodeSize,
      sessionInfo.code,
      UtvStrlen( sessionInfo.code ) );
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * This function is used to initiate a SupportTV session with the NetReady Services.
 *
 * @param   sessionInitiateInfo - Pointer to a buffer containing the information used to
 *                                initiate a session.
 *
 * @param   sessionInfo - Pointer to a buffer to hold the information for the initiated
 *                        session.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvProjectOnLiveSupportInitiateSession1(
  UtvLiveSupportSessionInitiateInfo1* sessionInitiateInfo,
  UtvLiveSupportSessionInfo1* sessionInfo )
{
  UTV_RESULT result = UTV_OK;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == sessionInitiateInfo || NULL == sessionInfo )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  if ( UtvProjectOnLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_INIT_ALREADY_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvLiveSupportInitiateSession1( sessionInitiateInfo, sessionInfo );

  if ( UTV_OK != result )
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * This function is used to terminate a SupportTV session with the NetReady Services.
 *
 * @note If a session is not active this function will return the following error:
 *    UTV_LIVE_SUPPORT_SESSION_TERMINATE_NOT_ACTIVE
 *
 * @return      UTV_OK if the processing executed successfully
 * @return      UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvProjectOnLiveSupportTerminateSession( void )
{
  UTV_RESULT result;

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  if ( !UtvProjectOnLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_TERMINATE_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvLiveSupportTerminateSession();

  if ( UTV_OK != result )
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * This function is used to retrieve the session code for a SupportTV session.
 *
 * @param   pbSessionCode - Pointer to a buffer to hold the session code associated with the
 *                          SupportTV session.  The buffer must be at least SESSION_CODE_SIZE
 *                          bytes to hold the session code.
 *
 * @param   uiSessionCodeSize - Size of the buffer to hold the session code.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvProjectOnLiveSupportGetSessionCode(
  UTV_BYTE* pbSessionCode,
  UTV_UINT32 uiSessionCodeSize )
{
  UTV_RESULT result;

  /* init if necessary */
  if ( !s_bUtvInit )
      s_Init();

  if ( !UtvProjectOnLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvLiveSupportGetSessionCode( pbSessionCode, uiSessionCodeSize );

  if ( UTV_OK != result )
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
  }

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT */

/*
*****************************************************************************************************
**
** GENERAL SUPPORT FUNCTIONS THAT MUST BE ADAPTED TO YOUR SYSTEM
** The following functions include porting steps for general functions that may require
** custom adaptation on your platform
**
*****************************************************************************************************
*/

/**
 **********************************************************************************************
 *
 * This function is used to retreive the platform's serial number.
 *
 * This is an example implementation that reads the serial number from a text file as given
 * by UTV_PLATFORM_SERIAL_NUMBER_FNAME. 
 * 
 * @intg
 *
 * It should be replaced by code that reads the serial number from the appropriate location.
 *
 * @param   pubBuffer     Buffer in which to place the serial number string.
 *
 * @param   uiBufferSize  The size of pubBuffer (maximum number of bytes to place).
 *
 * @return  UTV_OK if the serial number string was successfully gotten
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvCEMGetSerialNumber( UTV_INT8 *pubBuffer, UTV_UINT32 uiBufferSize )
{
    UTV_RESULT                  rStatus;
    UTV_INT8                    cSNum[ 48 ];
    UTV_UINT32                  uiFileHandle, i;

    /* open that file and get the name of the actual update file name */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( UTV_PLATFORM_SERIAL_NUMBER_FNAME, "r", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen( %s ) fails: \"%s\"",
                    UTV_PLATFORM_SERIAL_NUMBER_FNAME, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* read the file name from the snum file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileReadText( uiFileHandle, (UTV_BYTE *)cSNum, sizeof( cSNum ) )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadText( %s ) fails: \"%s\"",
                    UTV_PLATFORM_SERIAL_NUMBER_FNAME, UtvGetMessageString( rStatus ) );
        UtvPlatformFileClose( uiFileHandle );
        return rStatus;
    }

    UtvPlatformFileClose( uiFileHandle );

    /* remove any CR or LF */
    for ( i = 0; 0 != cSNum[ i ] && '\r' != cSNum[ i ] && '\n' != cSNum[ i ]; i++ );
    cSNum[ i ] = 0;

    /* copy it into the return buffer */
    UtvStrnCopy( (UTV_BYTE *) pubBuffer, uiBufferSize, (UTV_BYTE *) cSNum, UtvStrlen( (UTV_BYTE *) cSNum) );

    /* zero terminate */
    pubBuffer[ UtvStrlen( (UTV_BYTE *) cSNum ) ] = 0;

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is used to retreive the platform's Hardware Model Number index.
 *
 * This is an example implementation that returns a hardcoded hardware model index (1). It
 * should be replaced by code that reads the value from an appropriate location.
 * 
 * @intg
 *
 * It should be replaced by code that reads the value from an appropriate location. If there is
 * no more than one hardware model or there is otherwise no need to translate between hardware
 * model indices and and hardware model numbers, this can be ignored.
 *
 * @return  value indicating hardware model index
 * 
 * @note
 *
 * The hardware model index is value from 1 to n of all of the different hardware configurations
 * in a given model group that share the same update image. Hardware model differentiation is
 * typically applied to panel sizes. The 42" panel could be hardware model index 1. The 48" panel
 * will be hardware model index 2, and so on.
 *
 **********************************************************************************************
 */

static UTV_UINT16 UtvCEMReadHWMFromRegister( void )
{
    return 1;
}

/**
 **********************************************************************************************
 *
 * This function is used to retreive the platform's Hardware Model Number.
 *
 * This is an example implementation that returns a hardware model number based on a
 * translation of the hardware model index returned from UtvCEMReadHWMFromRegister().
 *
 * @intg
 *
 * It should be replaced by code that reads and/or translates the index from an
 * appropriate location. See also Integration Task comment in UtvCEMReadHWMFromRegister().
 *
 * @return  value indicating hardware model number
 * 
 * @note
 *
 * The actual hardware model values are defined and mapped to indices found in the 
 * cem-[OUI]-[MG].h file. The values must have counterparts created on the NOC.
 *
 **********************************************************************************************
 */

UTV_UINT16 UtvCEMTranslateHardwareModel( void )
{
    switch( UtvCEMReadHWMFromRegister() )
    {
    case 1: return UTV_CEM_HW_MODEL_1;
    case 2: return UTV_CEM_HW_MODEL_2;
    case 3: return UTV_CEM_HW_MODEL_3;
    default: break;
    }

    UtvMessage( UTV_LEVEL_ERROR, " UtvCEMTranslateHardwareModel(): \"%s\"", UtvGetMessageString( UTV_UNKNOWN_HW_MODEL ) );

    return -1;
}

/**
 **********************************************************************************************
 *
 * This function is used to get state of network interface
 *
 * @intg
 *
 * Not needed if s_bNetworkUp is set appropriately in UtvProjectOnNetworkUp()
 *
 * @return  UTV_TRUE if network is up (s_bNetworkUp is true)
 *
 * @return  UTV_FALSE otherwise
 *
 **********************************************************************************************
 */

UTV_BOOL UtvPlatformNetworkUp( void )
{
    return s_bNetworkUp;
}

/**
 **********************************************************************************************
 *
 * This function is used to retreive the platform's MAC address
 *
 * This is an example implementation to return the MAC addresss defined in S_FAKE_ETHERNET_MAC.
 *
 * @intg
 *
 * It should be replaced by code that reads the actual Ethernet MAC address of the device.
 *
 * @param   pszMAC     Buffer in which to place the MAC Address string.
 *
 * @param   uiBufferSize  The size of pubBuffer (maximum number of bytes to place).
 *
 * @return  UTV_OK if the Ethernet MAC address string was successfully gotten
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformGetEtherMAC( UTV_INT8 *pszMAC, UTV_UINT32 uiBufferSize )
{
    /* copy it into the return buffer */
    UtvStrnCopy( (UTV_BYTE *) pszMAC, uiBufferSize, (UTV_BYTE *) S_FAKE_ETHERNET_MAC, UtvStrlen( (UTV_BYTE *) S_FAKE_ETHERNET_MAC) );
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is used to retreive the platform's WiFi MAC address
 *
 * This is an example implementation to return the MAC addresss defined in S_FAKE_WIFI_MAC.
 *
 * @intg
 *
 * It should be replaced by code that reads the actual WiFi MAC address of the device.
 *
 * @param   pszMAC     Buffer in which to place the MAC Address string.
 *
 * @param   uiBufferSize  The size of pubBuffer (maximum number of bytes to place).
 *
 * @return  UTV_OK if the WiFi MAC address string was successfully gotten
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 * @note
 *
 * Only needed on devices with Wifi. Leave code in place to return fake WiFi MAC if your device 
 * does not support it.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformGetWiFiMAC( UTV_INT8 *pszMAC, UTV_UINT32 uiBufferSize )
{
    /* copy it into the return buffer */
    UtvStrnCopy( (UTV_BYTE *) pszMAC, uiBufferSize, (UTV_BYTE *) S_FAKE_WIFI_MAC, UtvStrlen( (UTV_BYTE *) S_FAKE_WIFI_MAC) );
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is used to test if the system time is set
 *
 * This is an example implementation to test if the system time has been set via NTP, STT, or
 * the consumer.
 *
 * @intg
 *
 * Replace the body of this function with code to sense whether system time has been set via
 * any of the methods described. If the device has a real-time clock then always return TRUE
 * unless there has been a power outage or other event that could create an invalid time.
 *
 * @return  UTV_TRUE if system is set
 *
 * @return  UTV_FALSE otherwise
 *
 **********************************************************************************************
 */

UTV_BOOL UtvPlatformSystemTimeSet( void )
{
    /* Make a system call to determine whether system time has been set or not.
       Mocked up to always assume system time has not been set for now. */
    if ( UTV_FALSE )
    {
        UtvMessage( UTV_LEVEL_WARN, "PlatformSystemTimeSet:  time not set" );
        return UTV_FALSE;
    }

    UtvMessage( UTV_LEVEL_INFO, "PlatformSystemTimeSet:  time set" );
    return UTV_TRUE;
}

/* UtvPlatformSetSystemTime
    Set the system time with provided value.
*/
UTV_RESULT UtvPlatformSetSystemTime( UTV_UINT32 uiTime )
{
    UTV_TIME    tNow;
    struct timeval tv;

    /* set the system time from the provided time. */
    tv.tv_sec = uiTime;
    tv.tv_usec = 0;

    if ( 0 == settimeofday( &tv, NULL ))
    {
        /* prove that we can read it back */
        /* get the current system time */
        tNow = UtvTime( NULL );
#ifdef _extra_time_debug
        {
            UTV_INT8 cNow[32], cSet[32];
            UtvCTime( tNow, cNow, 32 );
            UtvCTime( uiTime, cSet, 32 );

            UtvMessage( UTV_LEVEL_INFO, "UtvSetSystemTime(), actual: %s, expected: %s", cNow, cSet );
        }
#endif

        if ( (UTV_UINT32) tNow < uiTime )
            UtvMessage( UTV_LEVEL_WARN, "UtvSetSystemTime() has not yet taken effect: %d < %d", tNow, uiTime );
        else
            return UTV_OK;

    }

    UtvMessage( UTV_LEVEL_ERROR, "settimeofday() errno: %s", strerror( errno ) );
    return UTV_NO_NETWORK_TIME;
}

/**
 **********************************************************************************************
 *
 * This function is called by the Agent with the factory mode state.
 *
 * The placeholder code displays a simple string if factory mode is true.
 *
 * @intg
 *
 * Replace the body of this function with code that displays some sort of warning like text
 * in an overlay in the graphics plane so that devices are never shipped in factory mode.
 * Typically this is something like a low z-order overlay that is impossible to miss so that 
 * devices are never shipped in this mode.
 * 
 * @param   bFactoryMode indicates the factory mode state
 *
 * @return  UTV_OK
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformFactoryModeNotice( UTV_BOOL bFactoryMode )
{
    if ( bFactoryMode )
        UtvMessage( UTV_LEVEL_INFO, "FACTORY MODE NOTICE" );

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is used by the Agent to allow secure commands to be executed by the device. 
 * The command set is entirely up to the CEM. These commands are created by special entries in
 * Publisher script files. Please see the UpdateLogic Publisher document for more information.
 *
 * The placeholder code just displays the command that would be executed.
 *
 * @intg
 *
 * Replace the body of this function with code that executes the command provided. This can 
 * be as simple as spawning the command through the Linux "system" command or as complex as 
 * you would like to make it. Typically access to the Linux "system" command and diagnostic 
 * and development functions are supported.
 * 
 * @param   pszCommand the command to be executed
 *
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note
 *
 * The commands executed by this function will be encrypted in a tamper-proof command file 
 * so that this interface can include commands that are potential security risks as long as 
 * the command files built to execute them are not released into the wild.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformCommandInterface( UTV_INT8 *pszCommand )
{
    UtvMessage( UTV_LEVEL_INFO, "EXCUTING COMMAND: %s", pszCommand );

    return UTV_OK;
}

/*
*****************************************************************************************************
**
** FLASH WRITE SUPPORT FUNCTIONS THAT MUST BE ADAPTED TO YOUR SYSTEM
** The following functions include porting steps that will require custom adaptation to your
** platform's flash write sub-system.
**
*****************************************************************************************************
*/

/**
 **********************************************************************************************
 *
 * This function is used to allocate a large buffer for module storage. This memory may simply 
 * be allocated from the heap or may need to use video memory or another large chunk of 
 * available memory depending on system heap size.
 *
 * This is an example implementation using the heap.
 *
 * @intg
 *
 * Replace the body of this function with code to allocate a 2MB chunk of RAM. Many modern devices
 * simply use the placeholder function as is. If your device is particularly RAM-constrained you
 * can do things like allocate video buffer memory or other special purpose RAM. Please make sure
 * to turn off all active DMAs to special purpose RAM so that they don.t interfere with the use of
 * the allocated buffer!
 *
 * @param   iModuleSize  the size of buffer requested
 *
 * @return  a pointer to the buffer is allocated
 *
 * @return  NULL pointer otherwise
 *
*****************************************************************************************************
*/

void * UtvPlatformInstallAllocateModuleBuffer( UTV_UINT32 iModuleSize )
{
    void * pModuleBuffer = UtvMalloc( iModuleSize );

    /* Return the pointer or NULL */
    return pModuleBuffer;
}

/**
 **********************************************************************************************
 *
 * This function is used to free a buffer which was previously allocated using
 * UtvPlatformInstallAllocateModuleBuffer() 
 *
 * @intg
 *
 * This is an example implementation and that returns memory allocated from the heap. Replace the 
 * body of this function only if you resorted to special allocation handling in
 * UtvPlatformInstallAllocateModuleBuffer().
 *
 * @param   iModuleSize  the size of buffer to be freed
 *
 * @param   pModuleBuffer  the buffer to be freed
 *
 * @return  UTV_OK
 *
 * @note
 *
 * Other, non-UTV_OK, returns are possible and may be useful
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFreeModuleBuffer( UTV_UINT32 iModuleSize, void * pModuleBuffer )
{
    /* example source follows for the simple system heap case */
    if ( iModuleSize > 0 )
        UtvFree( pModuleBuffer );

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update when the Agent first encounters a module from a
 * component whose partition attribute is true.
 *
 * This is example code to show how to treat the component name as a partition name and
 * simulates opening that partition.
 *
 * @intg
 *
 * Replace the body of this function with code that actually calls the correct MTD management
 * functions on your platform to open the specified partition for write.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the partitions name
 *
 * @param  pModSigHdr points to a UtvModSigHdr structure (defined in utv-image.h) that contains
 * information about the module itself including its size, etc.
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallPartitionWrite() and UtvPlatformInstallPartitionClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallPartitionOpenWrite( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszPartitionName;
    UTV_UINT32   uiBytesWritten;
    UTV_BOOL     bInitialize = UTV_TRUE;

    if ( UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszPartitionName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* if the file doesn't exist or is of zero length fill it with zeroes so the flash memory simulation will work */
    if ( UTV_OK == (rStatus = UtvPlatformFileGetSize( pszPartitionName, &uiBytesWritten )))
    {
        if ( 0 != uiBytesWritten )
            bInitialize = UTV_FALSE;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( pszPartitionName, "wb", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", pszPartitionName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, " PLACEHOLDER: UtvPlatformInstallPartitionOpenWrite( %s ) OK", pszPartitionName );

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update when the Agent is resuming writing modules to a
 * previously incompletely written component whose partition attribute is true.
 *
 * This is example code to show how to treat the component name as a partition name and
 * simulates opening that partition for continued writing
 *
 * @intg
 *
 * Replace the body of this function with code that actually calls the correct MTD management
 * functions on your platform to open the specified partition for write.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the partitions name
 *
 * @param  pModSigHdr points to a UtvModSigHdr structure (defined in utv-image.h) that contains
 * information about the module itself including its size, etc.
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallPartitionWrite() and UtvPlatformInstallPartitionClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallPartitionOpenAppend( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszPartitionName;
    UTV_UINT32   uiBytesWritten;

    if ( UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszPartitionName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* if the file doesn't exist or is of zero length, exit in error */
    if ( ( UTV_OK != (rStatus = UtvPlatformFileGetSize( pszPartitionName, &uiBytesWritten ))) ||
         ( 0 == uiBytesWritten ) )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileGetSize(): appending to a non-existant or zero lenth partition");
        return UTV_FILE_EOF;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( pszPartitionName, "rb+", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", pszPartitionName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, " PLACEHOLDER: UtvPlatformInstallPartitionOpenAppend( %s )", pszPartitionName );

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during validation of a partition that was previously written with
 * the functions UtvPlatformInstallPartitionOpenWrite(), UtvPlatformInstallPartitionWrite()
 * and UtvPlatformInstallPartitionClose(). 
 *
 * This is example code to perform an open for read on the partition simulation file.
 *
 * @intg
 *
 * Replace the body of this function with code that calls the correct MTD management functions
 * on your platform to open the specified partition for read.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the partitions name
 *
 * @param  bActive reserved
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallPartitionWrite() and UtvPlatformInstallPartitionClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallPartitionOpenRead( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_BOOL bActive, UTV_UINT32 *puiHandle  )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszPartitionName;

    if (UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszPartitionName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( pszPartitionName, "rb", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", pszPartitionName, UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update after UtvPlatformInstallPartitionOpenWrite() to
 * write update modules to the currently open partition. 
 *
 * Placeholder code performs a write on the partition simulation file.
 *
 * @intg
 *
 * Replace the body of the function with code that calls the correct MTD write function on your
 * platform to write the data specified at the offset specified.
 *
 * @param  uiHandle the same handle returned by the UtvPlatformInstallPartitionOpenWrite() call
 *
 * @param  pData argument points to the data to write
 *
 * @param uiDataLen argument is the length of that data
 *
 * @param uiDataOffset the offset into the partition that the data should be written.
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallPartitionWrite( UTV_UINT32 uiHandle, UTV_BYTE *pData, UTV_UINT32 uiDataLen, UTV_UINT32 uiDataOffset )
{
    UTV_RESULT   rStatus;
    UTV_UINT32   uiBytesWritten;

    /* Write into component image at the specified offset. */
    if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiHandle, uiDataOffset, UTV_SEEK_SET )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileSeek(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != ( rStatus = UtvPlatformFileWriteBinary( uiHandle, pData, uiDataLen, &uiBytesWritten )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileWriteBinary( 0 ): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, " PLACEHOLDER: UtvPlatformInstallPartitionWrite( data len == %d, offset == 0x%X ) OK", uiDataLen, uiDataOffset );
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called during validation of a partition that was previously opened with
 * the function UtvPlatformInstallPartitionOpenRead(). 
 *
 * Placeholder code performs a read on the partition simulation file.
 *
 * @intg
 *
 * Replace the body of the function with code that calls the correct MTD function on your 
 * platform to read the data specified at the offset specified.
 *
 * @param  uiHandle the same handle returned by UtvPlatformInstallPartitionOpenRead()
 *
 * @param  pBuff argument points to a buffer into which the data read can be written
 *
 * @param  uiSize size of pBuff
 *
 * @param uiOffset the offset into the partition to read the uiSize bytes from.
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallPartitionRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize, UTV_UINT32 uiOffset )
{
    UTV_RESULT   rStatus;
    UTV_UINT32    uiBytesRead;

    if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiHandle, uiOffset, UTV_SEEK_SET )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileSeek(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileReadBinary( uiHandle, pBuff, uiSize, &uiBytesRead )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileReadBinary(): \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This functions closes partitions previously opened for write or read by
 *  UtvPlatformInstallPartitionOpenWrite() or UtvPlatformInstallPartitionOpenRead(). 
 *
 * Placeholder code closes the partition simulation file.
 *
 * @intg
 *
 * Replace the body of the function with code that calls the correct MTD function on your 
 * platform to read the data specified at the offset specified.
 *
 * @param  uiHandle the same handle returned by UtvPlatformInstallPartitionOpenRead() or
 * UtvPlatformInstallPartitionOpenWrite() or
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_UINT32 UtvPlatformInstallPartitionClose( UTV_UINT32 uiHandle )
{
    UTV_RESULT   rStatus;

    if ( UTV_OK != ( rStatus = UtvPlatformFileClose( uiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileClose(): \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update when the Agent first encounters a module from a
 * component whose partition attribute is false.
 *
 * This is example code to show how to treat the component name as a file name and
 * simulates opening that file.
 *
 * @intg
 *
 * Replace the body of this function with code that actually calls the correct 
 * functions on your platform to open the specified file for write. If your device does not use
 * file-based updates or this implmentation is adequate for your file-based update needs then
 * this function does not need to be changed.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the partitions name
 *
 * @param  pModSigHdr points to a UtvModSigHdr structure (defined in utv-image.h) that contains
 * information about the module itself including its size, etc.
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallFileWrite() and UtvPlatformInstallFileClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileOpenWrite( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszFileName;
    UTV_INT8     szPath[256];

    if (UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszFileName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMemFormat( (UTV_BYTE *) szPath, sizeof(szPath), "%s%s",UtvProjectGetUpdateInstallPath(),pszFileName);

    /* open and zap file if it already exists */
    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( szPath, "wb", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", szPath, UtvGetMessageString( rStatus ) );
        return rStatus;
    } else

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update when the Agent is resuming writing modules to a
 * previously incompletely written component whose partition attribute is false.
 *
 * This is example code to show how to treat the component name as a file name and
 * simulates opening that file for continued writing
 *
 * @intg
 *
 * Replace the body of this function with code that actually calls the correct 
 * functions on your platform to open the specified file for write. If your device does not use
 * file-based updates or this implmentation is adequate for your file-based update needs then
 * this function does not need to be changed.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the partitions name
 *
 * @param  pModSigHdr points to a UtvModSigHdr structure (defined in utv-image.h) that contains
 * information about the module itself including its size, etc.
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallFileWrite() and UtvPlatformInstallFileClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileOpenAppend( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszFileName;
    UTV_UINT32   uiBytesWritten;
    UTV_INT8     szPath[256];

    if (UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszFileName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMemFormat( (UTV_BYTE *) szPath, sizeof(szPath), "%s%s",UtvProjectGetUpdateInstallPath(),pszFileName);

    /* if the file doesn't exist or is of zero length, exit in error */
    if ( ( UTV_OK != (rStatus = UtvPlatformFileGetSize( szPath, &uiBytesWritten ))) ||
         ( 0 == uiBytesWritten ) )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileGetSize(): appending to a non-existant or zero lenth file");
        return UTV_FILE_EOF;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( szPath, "rb+", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", szPath, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during validation of a file that was previously written with
 * the functions UtvPlatformInstallFileOpenWrite() , UtvPlatformInstallFileWrite()
 * and UtvPlatformInstallFileClose(). 
 *
 * This is example code to perform an open on the file simulation file.
 *
 * @intg
 *
 * Replace the body of this function with code that calls the correct MTD management functions
 * on your platform to open the specified file for read. If your device does not use file-based
 * updates or this implmentation is adequate for your file-based update needs then this function
 * does not need to be changed.
 *
 * @param   hImage describes which internal "store" the component directory for the component 
 * is located in. 
 *
 * @param pCompDesc  points to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which may be used as
 * the files name
 *
 * @param  bActive reserved
 *
 * @param puiHandle points to a instance handle returned by this function which is passed to the
 * functions UtvPlatformInstallPartitionWrite() and UtvPlatformInstallPartitionClose().
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileOpenRead( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_BOOL bActive, UTV_UINT32 *puiHandle )
{
    UTV_RESULT   rStatus;
    UTV_INT8    *pszFileName;
    UTV_INT8     szPath[256];

    if (UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszFileName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetTextDef(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMemFormat( (UTV_BYTE *) szPath, sizeof(szPath), "%s%s",UtvProjectGetUpdateInstallPath(),pszFileName);

    if ( UTV_OK != (rStatus = UtvPlatformFileOpen( szPath, "rb", puiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileOpen( %s ): \"%s\"", szPath, UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during an update after UtvPlatformInstallFileOpenWrite() to
 * write update modules to the currently open file. 
 *
 * Placeholder code performs a write on the file simulation file.
 *
 * @intg
 *
 * Replace the body of this function with code that calls the correct MTD management functions
 * on your platform to open the specified file for write. If your device does not use file-based
 * updates or this implmentation is adequate for your file-based update needs then this function
 * does not need to be changed.
 *
 * @param  uiHandle the same handle returned by the UtvPlatformInstallFileOpenWrite() call
 *
 * @param  pData argument points to the data to write
 *
 * @param uiDataLen argument is the length of that data
 *
 * @param uiDataOffset the offset into the file that the data should be written.
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileWrite( UTV_UINT32 uiHandle, UTV_BYTE *pData, UTV_UINT32 uiDataLen, UTV_UINT32 uiDataOffset )
{
    UTV_RESULT   rStatus;
    UTV_UINT32   uiBytesWritten;

    /* Write into component image at the specified offset. */
    if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiHandle, uiDataOffset, UTV_SEEK_SET )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileSeek(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != ( rStatus = UtvPlatformFileWriteBinary( uiHandle, pData, uiDataLen, &uiBytesWritten )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileWriteBinary( 0 ): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called during validation of a file that was previously opened with
 * the function UtvPlatformInstallFileOpenRead(). 
 *
 * Placeholder code performs a read on the file simulation file.
 *
 * @intg
 *
 * Replace the body of this function with code that calls the correct MTD management functions
 * on your platform to read the data. If your device does not use file-based
 * updates or this implmentation is adequate for your file-based update needs then this function
 * does not need to be changed.
 *
 * @param  uiHandle the same handle returned by UtvPlatformInstallFileOpenRead()
 *
 * @param  pBuff argument points to a buffer into which the data read can be written
 *
 * @param  uiSize size of pBuff
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize )
{
    UTV_RESULT   rStatus;
    UTV_UINT32    uiBytesRead;

    if ( UTV_OK != (rStatus = UtvPlatformFileReadBinary( uiHandle, pBuff, uiSize, &uiBytesRead )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileReadBinary(): \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This function is called during validation of a file that was previously opened with
 * the function UtvPlatformInstallFileOpenRead(). 
 *
 * @intg
 *
 * Replace the body of this function with code that calls the correct MTD management functions
 * on your platform to read the data specified at the offset specified. If your device does not
 * use file-based updates or this implmentation is adequate for your file-based update needs 
 * then this function does not need to be changed.
 *
 * @param  uiHandle the same handle returned by UtvPlatformInstallFileOpenRead()
 *
 * @param  pBuff argument points to a buffer into which the data read can be written
 *
 * @param  uiSize size of pBuff
 *
 * @param uiOffset the offset into the faile to read the uiSize bytes from.
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileSeekRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize, UTV_UINT32 uiOffset )
{
    UTV_RESULT   rStatus;
    UTV_UINT32    uiBytesRead;

    if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiHandle, uiOffset, UTV_SEEK_SET )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileSeek(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != (rStatus = UtvPlatformFileReadBinary( uiHandle, pBuff, uiSize, &uiBytesRead )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileReadBinary(): \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This functions closes files previously opened for write or read by
 *  UtvPlatformInstallFileOpenWrite() or UtvPlatformInstallFileOpenRead(). 
 *
 * Placeholder code closes the file simulation file.
 *
 * @intg
 *
 * Replace the body of the function with code that calls the correct MTD function on your 
 * platform to read the data specified at the offset specified.
 *
 * @param  uiHandle the same handle returned by UtvPlatformInstallFileOpenRead() or
 * UtvPlatformInstallFileOpenWrite() or
 *
 * @return  UTV_OK if the requested object is found.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallFileClose( UTV_UINT32 uiHandle )
{
    UTV_RESULT   rStatus;

    if ( UTV_OK != ( rStatus = UtvPlatformFileClose( uiHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformFileClose(): \"%s\"", UtvGetMessageString( rStatus ) );
    }

    return rStatus;
}

/**
 **********************************************************************************************
 *
 * This routine is used to do any platform specific steps once a component has been installed.
 *
 * Typically this call is used to call a platform-specific CRC validation function if there is
 * one or to give an opportunity for the platform-specific machinery to perform installation
 * housekeeping.  It's not necessary to have this call do anything at all.
 *
 * @intg
 *
 * Replace the body of this function with platform-specific code that may be needed after a
 * partition is written.
 *
 * @param  hImage  an index to which internal "store" the component directory for the component
 * is located in.
 *
 * @param  pCompDesc pointer to a UtvCompDirCompDesc structure (defined in utv-image.h) that
 * contains a description of the component, including most importantly its name which is used
 * as the partitions name.
 *
 * @return  UTV_COMP_INSTALL_COMPLETE if successful
 *
 * @return  UTV_PLATFORM_COMP_COMP (or another UTV_RESULT) is there is an error
 *
*****************************************************************************************************
*/

UTV_RESULT UtvPlatformInstallComponentComplete( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc )
{

    /* This function doesn't do anything currently because the component install if already complete
       because the update.zip file that was written is closed by the time we get here.
       Leaving code commented out so it can be used if partition support is needed. */
#if 0
    UTV_RESULT   rStatus;
    UTV_INT8    *pszCompName;

    if (UTV_OK != (rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszCompName )))
        return rStatus;

    if ( UTV_FALSE /* insert platform-specific call here */ )
    {
        UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, UTV_PLATFORM_COMP_COMP, __LINE__, pszCompName );
        return UTV_PLATFORM_COMP_COMP;
    }
#endif
    return UTV_COMP_INSTALL_COMPLETE;
}

/**
 **********************************************************************************************
 *
 * This routine is used to perform any platform specific steps once an entire update has been
 * installed. This typically includes writing NVRAM with platform-specific indications that on
 * the next boot the bootloader should switch to the new update image, etc.
 * 
 * The call to validate partition and file hashes should be left in place unless the system 
 * performs its own validation.
 *
 * @intg
 *
 * Replace the body of this function with platform-specific code that handles telling the 
 * system that an update has been installed and is ready to be activated.
 *
 * @param  hImage  an index to which internal "store" the component directory for the component
 * is located in.
 *
 * @return  UTV_UPD_INSTALL_COMPLETE if successful
 *
 * @return  UTV_PLATFORM_UPDATE_COMP (or another UTV_RESULT) is there is an error
 *
 * @note
 *
 * The call to commit the update manifest MUST be left in place.
 *
*****************************************************************************************************
*/
UTV_RESULT UtvPlatformInstallUpdateComplete( UTV_UINT32 hImage )
{
    UTV_RESULT  rStatus;

    /* Validate component hashes */
    if ( UTV_OK != ( rStatus = UtvImageValidateHashes( hImage )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageValidateHashes(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

#ifndef UTV_TEST_NO_MOD_VERSION_UPDATE
    /* Commit the new component manifest.  Please note that if successful, this operation cannot be undone.
       Only make this call after all new components have been installed.  The component directory of the new
       update becomes the system component manifest after this call is made.
    */
    if ( UTV_OK != ( rStatus = UtvManifestCommit( hImage )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvManifestCommit(): \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }
    
#else
    UtvMessage(UTV_LEVEL_INFO, "Deleting active component manifest" );
    rStatus = UtvPlatformFileDelete( UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME );
    if ( UTV_OK != rStatus )
    {
        UtvMessage(UTV_LEVEL_ERROR, "delete active CM failed: %s",
        UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME );
    }
#endif

    // tell the JNI interface that an update.zip is ready.
    UtvProjectSetUpdateReadyToInstall(UTV_TRUE);

#ifdef UTV_LIVE_SUPPORT
    /*
     * If Live Support is enabled and there is still a session active then pause the
     * session.  The device agent will attempt to resume the session after the device reboots
     * and when the network starts back up.
     */

    if ( UtvLiveSupportIsSessionActive() )
    {
      UtvLiveSupportSessionPause();
    }
#endif /* UTV_LIVE_SUPPORT */

    if ( UTV_FALSE /* insert platform-specific call here */ )
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, UTV_PLATFORM_UPDATE_COMP, __LINE__ );
        return UTV_PLATFORM_UPDATE_COMP;
    }

    return UTV_UPD_INSTALL_COMPLETE;
}

/* UtvPlatformInstallCopyComponent
   Not supported in the android project.

*/
UTV_RESULT UtvPlatformInstallCopyComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc )
{
    return UTV_UNKNOWN;
}

/*
*****************************************************************************************************
**
** SECURITY SUPPORT FUNCTIONS THAT MUST BE ADAPTED TO YOUR SYSTEM
** The following functions include porting steps that will require custom adaptation to your
** platform's crypto sub-system.
**
*****************************************************************************************************
*/

/**
 **********************************************************************************************
 *
 * This function is called from UtvProjectOnBoot via UtvPublicAgentInit() to initialize the
 * SoC-specific security layer (Please see the NetReady Agent Porting Instructions for more 
 * information about UtvProjectOnBoot().)
 *
 * This is a default implementation that exists to allow special case initializations above and
 * beyond this default case. 
 *
 * Please note that it's quite common for this function to be a NOP except for setting the 
 * value of s_rSecureInit to UTV_OK.
 *
 * @intg
 *
 * This code could be replaced by code that acquires and store instance handles for the other 
 * security-related functions that follow.
 * 
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem initializing
 * the SoC's security layer.
 *
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureOpen( void )
{
    /* Tell the rest of the secure functions that the secure sub-system is OK (or not).
       This allows the system to come up and report secure sub-system errors. */
    s_rSecureInit = UTV_OK;
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called from UtvProjectOnShutdown() via UtvPublicAgentShutdown() to return
 * any SoC-specific security layer entities created at initialization (Please see the NetReady 
 * Agent Porting Instructions for more information about UtvProjectOnShutdown().)
 *
 * This is a default implementation that exists to release the special case initialization
 * entities. The default implementation may be all that is needed.
 *
 * @intg
 *
 * This code should be replaced by code that releases and returns the entities acquired in 
 * UtvPlatformSecureOpen()
 * 
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureClose( void )
{
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called at various times to retrieve various objects that the Agent has
 * stored securely. A call to this function requests that an object with a specified name 
 * previously written by UtvPlatformSecureWrite() be read back, decrypted and then put into 
 * the buffer provided. 
 * 
 * The example implementation below uses an example AES-128 decryption scheme based on an 
 * K-SOC Transit Development key.
 *
 * @intg
 *
 * The decryption portion of the code below can be removed and replaced with platform-specific
 * code using a different encryption scheme and/or hardware-assisted encryption
 *
 * The implementation of this function should remove any padding that was added by 
 * UtvPlatformSecureWrite() to maintain data fidelity
 * 
 * @param   pszFName    a string with a unique name for the object to be read. 
 *
 * @param   pData       buffer in which to place the object data
 *
 * @param   puiDataLen  the size of pData (maximum number of bytes to place).
 *
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note
 *
 * The pszFName may need additional platform-specific partition and path information pre-pended
 * to it.
 *
 * If the platform provides an encrypted file system that is able to decrypt on the fly, the 
 * specified data can simply be read back from that partition using fread().
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureRead( UTV_INT8 *pszFName, UTV_BYTE **pData, UTV_UINT32 *puiDataLen )
{
    UTV_RESULT        rStatus;
    UTV_BYTE         *pBuff = NULL;
    UTV_UINT32        uiFileHandle, uiFileSize, uiBufferSize, uiSize;
    UTV_BYTE           aesKey[ 16 ];
    UTV_BYTE           aesVector[ 16 ];
    int               iInLen, iOutLen;
    EVP_CIPHER_CTX    ctx;
    UTV_BYTE          *pOut = NULL, *p = NULL;
    UTV_INT8          *pStr;
    UtvExtClrHdr      sClearHdr;

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    /* determine the size of the file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileGetSize( pszFName, &uiFileSize )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileGetSize(%s) fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }
    
    /* open the file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszFName, "rb", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen(%s) fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* attempt to read in the clear header */
    rStatus = UtvPlatformFileReadBinary( uiFileHandle, (UTV_BYTE *)&sClearHdr, sizeof(UtvExtClrHdr), &uiBufferSize );
    if ( rStatus != UTV_OK )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary(%s) fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }
    
    /* sanity check the clear header */
    if ( sClearHdr.ucMagic != PLATFORM_SPECIFIC_EXT_ENCRYPT_MAGIC ||
         sClearHdr.ucHeaderVersion != PLATFORM_SPECIFIC_EXT_ENCRYPT_VERSION )
    {
        /* This file is not encrypted with a known format.
         */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureRead(): \"%s\" unknown encryption header!", pszFName );
        return UTV_NO_ENCRYPTION;
    }

    /* check OUI */
    if ( Utv_32letoh(sClearHdr.uiOUI) != UTV_CEM_OUI )
    {
        rStatus = UTV_BAD_CRYPT_OUI;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureRead() fails: %s", UtvGetMessageString( rStatus ));
        return rStatus;
    }

    /* check model group */
    if ( Utv_32letoh(sClearHdr.usModelGroup) !=  UTV_CEM_MODEL_GROUP )
    {
        rStatus = UTV_BAD_CRYPT_MODEL;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureRead(): %s", UtvGetMessageString( rStatus ));
        return rStatus;
    }

    /* check encryption type */
    if ( Utv_32letoh(sClearHdr.usEncryptType) ==  UTV_PROVISIONER_ENCRYPTION_NONE )
    {
        /* allow no encryption if this is a dev build */
#ifndef UTV_PRODUCTION_BUILD
        UtvMessage( UTV_LEVEL_INFO, "DEBUG ONLY!!!  Accepting UNENCRYPTED ULPK" );

        /* read the remainder of the ULPK in */
        uiFileSize -= sizeof(UtvExtClrHdr);
        if ( NULL == ( *pData = (UTV_BYTE *)UtvMalloc( uiFileSize )))
        {
            rStatus = UTV_NO_MEMORY;
            UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc() fails: %s", UtvGetMessageString( rStatus ));
            return rStatus;
        }
        
        rStatus = UtvPlatformFileReadBinary( uiFileHandle, *pData, uiFileSize, puiDataLen );
        if ( rStatus != UTV_OK )
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary(%s) fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
            UtvFree(*pData);
            return rStatus;
        }

        UtvPlatformFileClose( uiFileHandle );
        return UTV_OK;
#else
        rStatus = UTV_BAD_CRYPT_PAYLOAD_TYPE;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureRead(): %s", UtvGetMessageString( rStatus ));
        return rStatus;

#endif        
    }
    
    uiFileSize -= sizeof(UtvExtClrHdr);

    /* allocate a buffer for the file */
    if ( NULL == ( pBuff = UtvMalloc( uiFileSize )))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc() fails: %s", UtvGetMessageString( rStatus ));
        return rStatus;
    }


    /* attempt to read in the entire file */
    rStatus = UtvPlatformFileReadBinary( uiFileHandle, pBuff, uiFileSize, &uiFileSize );
    if ( rStatus != UTV_OK )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary() fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
        UtvFree( pBuff );
        return rStatus;
    }

    UtvPlatformFileClose( uiFileHandle );

    if ( NULL == ( pOut = (UTV_BYTE *)UtvMalloc( uiFileSize ) ))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc() fails: %s", UtvGetMessageString( rStatus ));
        UtvFree( pBuff );
        return rStatus;
    }
    p = pOut;

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_SECURE_KEY )))
    {
        /* can't find secure key string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesKey, sizeof(aesKey), pStr );

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_SECURE_VECTOR )))
    {
        /* can't find secure vector string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesVector, sizeof(aesVector), pStr );

    EVP_CIPHER_CTX_init( &ctx );
    EVP_DecryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, aesKey, aesVector );

    iOutLen = iInLen = (int)uiFileSize;
    if ( 0 == EVP_DecryptUpdate( &ctx, p, &iOutLen, pBuff, iInLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptUpdate() failed" );
        UtvFree( pOut );
        UtvFree( pBuff );
        return UTV_BAD_DECRYPTION;
    }
    p += iOutLen; 
    uiSize = iOutLen;

    iOutLen = iInLen - iOutLen;
    if ( 0 == EVP_DecryptFinal_ex( &ctx, p, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptFinal_ex() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    uiSize += iOutLen;

    UtvFree( pBuff );

    EVP_CIPHER_CTX_cleanup( &ctx );

    /* return the object data */
    *puiDataLen = uiSize;
    *pData = pOut;

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function is called at various times for various objects that the Agent needs to store
 * securely such that it will persist between power cycles. It will encrypt and write the data
 * in the buffer to non-volatile memory such that it can later retrieved by UtvPlatformSecureRead()
 * using the same filename.
 *
 * The example code below uses an example AES-128 encryption scheme based on an K-SOC Transit
 * Development key.
 *
 * @intg
 *
 * The encryption portion of the code below can be removed and replaced with platform-specific
 * code using a different encryption scheme and/or hardware-assisted encryption
 * 
 * @param   pszFileName A string with a unique name for the object to be written
 *
 * @param   pubData     Buffer containing the object data
 *
 * @param   uiSize      The size of pubData
 *
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note
 *
 * The pszFileName may need additional platform-specific partition and path information prepended
 * to it.
 *
 * If the platform provides a secure file partition that encrypts on the fly then this file 
 * may simply be written into that partition.
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureWrite( UTV_INT8 *pszFileName, UTV_BYTE *pubData, UTV_UINT32 uiSize )
{
    UTV_BYTE   aesKey[ 16 ];
    UTV_BYTE   aesVector[ 16 ];
    UTV_RESULT rStatus;
    UTV_UINT32 uiFileHandle, writeHeaderSize, uiBytesWritten, uiBufferSize;
    int iInLen = 0;
    int iOutLen = 0;
    EVP_CIPHER_CTX ctx;
    UTV_BYTE *pEnc = NULL, *pOut = NULL;
    UTV_INT8 *pStr;
    UtvExtClrHdr  sClearHdr;

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_SECURE_KEY )))
    {
        /* can't find secure key string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesKey, sizeof(aesKey), pStr );

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_SECURE_VECTOR )))
    {
        /* can't find secure vector string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesVector, sizeof(aesVector), pStr );

    uiBufferSize = ( uiSize & 0xF ? (( uiSize & ~0xF ) + 16 ) : uiSize ) + 16;
    if ( NULL == ( pOut = (UTV_BYTE *)UtvMalloc( uiBufferSize ) ))
        return UTV_NO_MEMORY;
    pEnc = pOut;

    EVP_CIPHER_CTX_init( &ctx );
    EVP_EncryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, aesKey, aesVector );

    iInLen = (int)uiSize;
    iOutLen = (int)uiBufferSize;
    if ( 0 == EVP_EncryptUpdate( &ctx, pEnc, &iOutLen, pubData, uiSize ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_EncryptUpdate() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    pEnc += iOutLen; uiBytesWritten = iOutLen;

    iOutLen = (int)uiBufferSize - iOutLen;
    if ( 0 == EVP_EncryptFinal_ex( &ctx, pEnc, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_EncryptFinal_ex() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    uiBytesWritten += iOutLen;

    EVP_CIPHER_CTX_cleanup( &ctx );

    /* open the file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszFileName, "wb", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen(%s) fails: \"%s\"", pszFileName, UtvGetMessageString( rStatus ) );
        UtvFree( pOut );
        return rStatus;
    }
    
    sClearHdr.ucMagic = PLATFORM_SPECIFIC_EXT_ENCRYPT_MAGIC;
    sClearHdr.ucHeaderVersion = PLATFORM_SPECIFIC_EXT_ENCRYPT_VERSION;
    sClearHdr.usModelGroup = UtvCEMGetPlatformModelGroup();
    sClearHdr.uiOUI = UtvCEMGetPlatformOUI();
    sClearHdr.usEncryptType = UTV_PROVISIONER_ENCRYPTION_AES;
    sClearHdr.usKeySize = 0;
    sClearHdr.ulPayloadSize = uiSize;
    
    /* attempt to write the header */
    rStatus = UtvPlatformFileWriteBinary( uiFileHandle, (UTV_BYTE *) &sClearHdr, sizeof(sClearHdr), &writeHeaderSize );
    if ( rStatus != UTV_OK )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileWriteBinary( %s ) fails: \"%s\"", pszFileName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* attempt to write the entire file */
    rStatus = UtvPlatformFileWriteBinary( uiFileHandle, pOut, uiBytesWritten, &uiBytesWritten );
    if ( rStatus != UTV_OK )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileWriteBinary( %s ) fails: \"%s\"", pszFileName, UtvGetMessageString( rStatus ) );
        UtvFree( pOut );
        return rStatus;
    }

    UtvPlatformFileClose( uiFileHandle );
    UtvFree( pOut );

    return UTV_OK;
}

static unsigned char s_aesKey[ 16 ];
static unsigned char s_aesVector[ 16 ];

/**
 **********************************************************************************************
 *
 * Called by the update payload decryption code to deliver SoC-specific keys that are embedded 
 * in the encrypted payload header of an update module. These keys should be stored in memory
 * (not flash) for later use by the UtvPlatformSecureDecrypt() function. This function will be
 * called by the UTV private library when the update payload encryption type is 
 * UTV_ENCRYPT_VER_PAYLOAD_EXT.
 *
 * The function below is a placeholder and does nothing.
 *
 * @intg
 *
 * Replace the body of this function with code to store the delivered key(s) so that it can be
 * used by UtvPlatformSecureDecrypt(). The number, size, and structure of these keys is platform
 * dependent and should be coordinated with ULI so that the Publisher utility can deliver the 
 * appropriate key format. If your platform doesn.t have platform-specific keys that are 
 * delivered with a payload then this function can be a NOP.
 * 
 * @param   pKeys points to one or more SoC-specific keys that were provided by a EXT plug-in to
 * the ULI Publisher utility.
 *
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note
 *
 * The pszFileName may need additional platform-specific partition and path information pre-pended
 * to it.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureDeliverKeys( void *pKeys )
{
    unsigned char *p = (unsigned char *)pKeys;

    UtvMessage( UTV_LEVEL_INFO, "PLACEHOLDER: UtvPlatformSecureDeliverKeys() called" );

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    memcpy( s_aesKey, p, sizeof( s_aesKey ));
    memcpy( s_aesVector, p+16, sizeof( s_aesVector ));

    return UTV_OK;
}

/* UtvPlatformSecureGetBlockSize
   *** PORTING STEP:  REPLACE THE BODY OF THIS FUNCTION WITH PLATFORM-SPECIFIC CODE TO MANAGE THE KEYS THAT BELONG ARE REQUIRED
   This function will be called by the UTV private library when decrypting a module encrypted via EXT to determine the block
   size of the cipher.
*/
UTV_RESULT UtvPlatformSecureBlockSize( UTV_UINT32 *puiBlockSize )
{
    UtvMessage( UTV_LEVEL_INFO, "PLACEHOLDER: UtvPlatformSecureBlockSize() called" );

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    *puiBlockSize = 16;
    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * Called by the ULI private library when the update payload decryption code detects that it 
 * is supposed to use hardware accelerated decryption of a chunk of payload data. Typically
 * these chunks are about 2MB long. This function will only be called when Publisher uses the
 * -x option to "externally" encrypt the update payloads with a format that the hardware is
 * capable of decrypting. 
 * 
 * This function should use the key(s) provided by UtvPlatformSecureDeliverKeys() to perform
 * hardware-accelerated decryption of the buffer pointed to by pInBuff and return it in
 * memory pointed to by pOutBuff. If your platform doesn.t perform hardware-assisted payload
 * decryption then this function can be a NOP.
 *
 * @intg
 *
 * Replace the body of this function with platform-specific code that decrypts the payloads 
 * of update modules. This function will be called by the UTV private library when the update
 * payload encryption type is UTV_ENCRYPT_VER_PAYLOAD_EXT. The function is expected to decrypt
 * the payload and return it in the provided buffer. The encryption type is pre-shared with a
 * platform-specific EXT plug-in for the ULI Publisher utility. The function 
 * UtvPlatformSecureDeliverKeys() will always be called with the keys required for this decrypt
 * prior to this function being called. 
 * 
 * @param uiOUI contains the OUI of the update that is being decrypted
 * 
 * @param uiModelGroup  contains the model group of the update that is being decrypted. 
 * 
 * @param pInBuff  points to the data to be decrypted
 * 
 * @param pOutBuff points to the buffer to receive the data that has been decrypted
 * 
 * @param uiDataLen contains the length of the data to be decrypted. 
 * 
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note 
 *
 * It is assumed that the length of the encrypted data is the same as the length of the decrypted data.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureDecrypt( UTV_UINT32 uiOUI, UTV_UINT32 uiModelGroup, UTV_BYTE *pInBuff, UTV_BYTE *pOutBuff, UTV_UINT32 uiDataLen )
{
    /* BEGIN OPENSSL REFERENCE IMPLEMENTATION: REPLACE WITH YOUR HARDWARE SPECIFIC DECRYPTION HERE */
    int iOutLen = (int)uiDataLen;
    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init( &ctx );
    EVP_DecryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, s_aesKey, s_aesVector );
    EVP_CIPHER_CTX_set_padding( &ctx, 0 );

    UtvMessage( UTV_LEVEL_INFO, "PLACEHOLDER: UtvPlatformSecureDecrypt( %d ) called", uiDataLen );

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    if ( 0 == EVP_DecryptUpdate( &ctx, pOutBuff, &iOutLen, pInBuff, uiDataLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptUpdate() failed" );
        return UTV_BAD_DECRYPTION;
    }
    pOutBuff += iOutLen;
    iOutLen = (int)uiDataLen - iOutLen;

    if ( 0 == EVP_DecryptFinal_ex( &ctx, pOutBuff, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptFinal_ex() failed" );
        return UTV_BAD_DECRYPTION;
    }

    EVP_CIPHER_CTX_cleanup( &ctx );
    /* END OPENSSL REFERENCE IMPLEMENTATION */

    return UTV_OK;
}

/* UtvPlatformSecureDecryptDirect
   *** PORTING STEP:  REPLACE THE BODY OF THIS FUNCTION WITH PLATFORM-SPECIFIC CODE THAT PERFORMS NATIVE DECRYPTION. ***
   This function will be called by the UtvProjectOnObjectRequest() above when the encryption type is special to the
   platform (UTV_PROVISIONER_ENCRYPTION_3DES_UR >=). The function is expected to decrypt the payload in place.
   This function must return UTV_OK if successful.
   Any other value may be returned if there is an error.
*/
UTV_RESULT UtvPlatformSecureDecryptDirect( UTV_BYTE *pInBuff, UTV_UINT32 *puiDataLen )
{
    UTV_BYTE aesKey[ 16 ] = { 0xc9, 0x6d, 0xd0, 0xe0, 0xe3, 0xb3, 0x9f, 0xc8, 0x48, 0x65, 0xbc, 0xb4, 0xd0, 0x80, 0x17, 0x66 };
    UTV_BYTE aesVector[ 16 ];
    int iInLen = (int)*puiDataLen - sizeof(UtvExtClrHdr);
    int iOutLen = 0;
    EVP_CIPHER_CTX ctx;
    UTV_BYTE *pOut = NULL, *pEnc = NULL;
    UtvExtClrHdr  *pClearHdr;

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

   /* initial vector set */
    memset( aesVector, 0, sizeof( aesVector ));

    /* check the header */
    pClearHdr = (UtvExtClrHdr *)pInBuff;

    /* if not external bail */
    if ( Utv_16letoh( pClearHdr->usEncryptType ) != PLATFORM_SPECIFIC_EXT_ENCRYPT_TYPE ||
         Utv_16letoh( pClearHdr->usKeySize ) != PLATFORM_SPECIFIC_EXT_ENCRYPT_KEY_SIZE )
    {
        /* This file is not encrypted with a known external format.
         */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureDecryptDirect(): unknown external encryption type!" );
        return UTV_BAD_CRYPT_PAYLOAD_TYPE;
    }

    /* BEGIN OPENSSL REFERENCE IMPLEMENTATION: REPLACE WITH YOUR HARDWARE SPECIFIC K-SOC DECRYPTION HERE */
    EVP_CIPHER_CTX_init( &ctx );
    EVP_DecryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, aesKey, aesVector );
    EVP_CIPHER_CTX_set_padding( &ctx, 0 );

     UtvMessage( UTV_LEVEL_INFO, "PLACEHOLDER: UtvPlatformSecureDecryptDirect( %d bytes ) called", *puiDataLen );

    if ( NULL == ( pOut = UtvMalloc( *puiDataLen )))
        return UTV_NO_MEMORY;
    pEnc = pOut;

    iOutLen = iInLen;
    if ( 0 == EVP_DecryptUpdate( &ctx, pEnc, &iOutLen, pInBuff + sizeof(UtvExtClrHdr), iInLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptUpdate() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    pEnc += iOutLen;
    iOutLen = iInLen - iOutLen;

    if ( 0 == EVP_DecryptFinal_ex( &ctx, pEnc, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptFinal_ex() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }

    UtvByteCopy( pInBuff, pOut, *puiDataLen );
    UtvFree( pOut );

    EVP_CIPHER_CTX_cleanup( &ctx );
    /* END OPENSSL REFERENCE IMPLEMENTATION */

    return UTV_OK;
}

/**
 **********************************************************************************************
 *
 * This function gets a copy of the ULPK from wherever it was stored during factory insertion,
 * decrypts it, and returns a copy.
 * 
 * A certain amount of validation will occur but the two purposes of this function are to get
 * the ULPK and decrypt it. 
 *
 * This is a placeholder function that uses an example AES-128 decryption scheme based on an
 * K-SOC Transit Development key.
 *
 * @intg
 *
 * The encryption portion of the code below can be removed and replaced with platform-specific
 * code using a different encryption scheme and/or hardware-assisted encryption
 * 
 * @param   pULPK       Buffer pointer to hold the ULPK copy
 *
 * @param   puiULPKLength Size of pULPK buffer
 *
 * @return  UTV_OK 
 *
 * @return  UTV_RESULT a non-zero platform-specific error if there was a problem
 *
 * @note
 *
 * It is recommended that the ULPK's K-SOC-Transit encryption be retained intact when the ULPK
 * is inserted into the secure file system that is protected by K-SOC-Unique. This would, 
 * therefore, require two decryptions to access the in-the-clear ULPK data that is returned by
 * this function. This function is called once when the Agent is first initialized using the
 * UtvProjectOnBoot() call. (Please see the NetReady Agent Porting Instructions for 
 * more information about UtvProjectOnBoot().)
 *
 * ULPK encryption is platform-specific. ULI will work with the integrator to create an 
 * encrypt/decrypt model that takes advantage of the platform's crypto block features. 
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformSecureGetULPK( UTV_BYTE **pULPK, UTV_UINT32 *puiULPKLength )
{
    UTV_BYTE aesKey[ 16 ];
    UTV_BYTE aesVector[ 16 ];
    UTV_RESULT    rStatus = UTV_OK;
    UTV_BYTE     *pData;
    UTV_UINT32    uiSize, uiCheckSize;
    UTV_UINT32    uiFileHandle;
    int iInLen = 0;
    int iOutLen = 0;
    EVP_CIPHER_CTX ctx;
    UTV_BYTE *pOut = NULL, *pEnc = NULL;
    UtvExtClrHdr  *pClearHdr;
    UTV_INT8 *pStr;

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    /* read it from the secure store.  it's always returned naked */
    if ( UTV_OK == (rStatus = UtvPlatformSecureRead( UtvCEMInternetGetULPKFileName(), pULPK, puiULPKLength )))
    {
        /* read from secure store OK */
        /* caller will free buffer */
        return UTV_OK;
    }

    UtvMessage( UTV_LEVEL_WARN, "ULPK is not secure store protected, attempting transit decrypt" );

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_TRANSIT_KEY )))
    {
        /* can't find transit key string */
        rStatus = UTV_NULL_PTR;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureGetULPK(tk): %s", UtvGetMessageString( s_rSecureInit ) );        
        return rStatus;
    }

    UtvConvertHexToBin( aesKey, sizeof(aesKey), pStr );

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_TRANSIT_VECTOR )))
    {
        /* can't find transit key string */
        rStatus = UTV_NULL_PTR;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureGetULPK(tv): %s", UtvGetMessageString( s_rSecureInit ) );        
        return rStatus;
    }

    UtvConvertHexToBin( aesVector, sizeof(aesVector), pStr );

    /* determine the size of the ULPK file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileGetSize( UtvCEMInternetGetULPKFileName(), &uiSize )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileGetSize(%s): %s", UtvCEMInternetGetULPKFileName(), UtvGetMessageString( rStatus ));
        return rStatus;
    }

    /* allocate a buffer for the file */
    if ( NULL == ( pData = UtvMalloc( uiSize )))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc() fails: %s", UtvGetMessageString( rStatus ));
        return rStatus;
    }

    /* open the file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( UtvCEMInternetGetULPKFileName(), "rb", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen(%s) fails: \"%s\"", UtvCEMInternetGetULPKFileName(), UtvGetMessageString( rStatus ) );
        UtvFree( pData );
        return rStatus;
    }

    /* attempt to read in the entire file */
    rStatus = UtvPlatformFileReadBinary( uiFileHandle, pData, uiSize, &uiCheckSize );
    if ( rStatus != UTV_OK )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary(%s) fails: \"%s\"", UtvCEMInternetGetULPKFileName(), UtvGetMessageString( rStatus ) );
        UtvFree( pData );
        return rStatus;
    }

    UtvPlatformFileClose( uiFileHandle );

    /* sanity check file size */
    if ( uiSize != uiCheckSize  )
    {
        rStatus = UTV_BAD_SIZE;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary(%s) fails: \"%s\"", UtvCEMInternetGetULPKFileName(), UtvGetMessageString( rStatus ) );
        UtvFree( pData );
        return rStatus;
    }
    
    /* sanity check the clear header */
    pClearHdr = (UtvExtClrHdr *)pData;

    if ( pClearHdr->ucMagic != PLATFORM_SPECIFIC_EXT_ENCRYPT_MAGIC ||
         pClearHdr->ucHeaderVersion != PLATFORM_SPECIFIC_EXT_ENCRYPT_VERSION )
    {
        /* This file is not encrypted with a known format.
         */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureGetULPK: unknown encryption header!" );
        UtvFree( pData );
        return UTV_NO_ENCRYPTION;
    }

    /* check OUI */
    if ( Utv_32letoh(pClearHdr->uiOUI) !=  UtvCEMGetPlatformOUI() )
    {
        rStatus = UTV_BAD_CRYPT_OUI;
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureGetULPK(): %s", UtvGetMessageString( rStatus ));
        UtvFree( pData );
        return rStatus;
    }

    /* return payload size */
    *puiULPKLength = Utv_32letoh(pClearHdr->ulPayloadSize);

    EVP_CIPHER_CTX_init( &ctx );
    EVP_DecryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, aesKey, aesVector );
    EVP_CIPHER_CTX_set_padding( &ctx, 0 );

    if ( NULL == ( pOut = UtvMalloc( Utv_32letoh(pClearHdr->ulPayloadSize) )))
    {
        UtvFree( pData );
        return UTV_NO_MEMORY;
    }
    pEnc = pOut;

    iOutLen = iInLen = (int)(Utv_32letoh(pClearHdr->ulPayloadSize));
    if ( 0 == EVP_DecryptUpdate( &ctx, pEnc, &iOutLen, pData + sizeof( UtvExtClrHdr ), iInLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptUpdate() failed" );
        UtvFree( pData );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    pEnc += iOutLen;
    iOutLen = (int)(Utv_32letoh(pClearHdr->ulPayloadSize)) - iOutLen;

    if ( 0 == EVP_DecryptFinal_ex( &ctx, pEnc, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "EVP_DecryptFinal_ex() failed" );
        UtvFree( pData );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }

    UtvFree( pData );
    *pULPK = pOut;

    EVP_CIPHER_CTX_cleanup( &ctx );

    /* caller will free buffer */
    return UTV_OK;
}


UTV_RESULT UtvPlatformSecureInsertULPK( UTV_BYTE *pbULPK )
{ 
    UTV_RESULT     rStatus;
    UTV_BYTE      *pbClearULPK = NULL;
    UTV_BYTE      *pDataCheck;
    UTV_UINT32     i, uiSizeCheck, clearUlpkSize;
    UTV_UINT16     usWarning = 0;
    UTV_BYTE       aesKey[ 16 ];
    UTV_BYTE       aesVector[ 16 ];
    UtvExtClrHdr  *pClearHdr; 
    UTV_INT8      *pStr;
#ifdef _SPLIT_ULPK_SUPPORT
    UTV_BYTE       securityModelTwoUniqueKey[ 16 ];
    UTV_UINT32     securityModelTwoUniqueKeySize = 16;
#endif

    /* implicit call to initialize system.  assumed to be unitialized. */
    UtvProjectOnEarlyBoot();

    if ( !s_bUtvInit )
        s_Init();

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    pClearHdr=(UtvExtClrHdr*)pbULPK;

    /* sanity check the clear header */
    if ( pClearHdr->ucMagic != PLATFORM_SPECIFIC_EXT_ENCRYPT_MAGIC ||
         pClearHdr->ucHeaderVersion != PLATFORM_SPECIFIC_EXT_ENCRYPT_VERSION )
    {
        /* The inserted ULPK is not encrypted with a known format.
         */
        rStatus = UTV_BAD_CRYPT_HEADER;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    /* check OUI */
    if ( Utv_32letoh(pClearHdr->uiOUI) != UTV_CEM_OUI )
    {
        rStatus = UTV_BAD_CRYPT_OUI;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    /* check model group */
    if ( Utv_32letoh(pClearHdr->usModelGroup) != UTV_CEM_MODEL_GROUP )
    {
        rStatus = UTV_BAD_CRYPT_MODEL;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    /* check encryption type */
    if ( Utv_32letoh(pClearHdr->usEncryptType) != UTV_PROVISIONER_ENCRYPTION_AES )
    {
        rStatus = UTV_BAD_KEY_TYPE;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }
    
    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_TRANSIT_KEY )))
    {
        /* can't find transit key string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesKey, sizeof(aesKey), pStr );

    if ( NULL == (pStr = UtvStringsLookup( UTV_ULPK_TRANSIT_VECTOR )))
    {
        /* can't find transit key string */
        rStatus = UTV_NULL_PTR;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }

    UtvConvertHexToBin( aesVector, sizeof(aesVector), pStr );

    if ( UTV_OK != ( rStatus = s_AESDecrypt( &pbULPK[ sizeof( UtvExtClrHdr ) ], 
                                             Utv_32letoh(pClearHdr->ulPayloadSize), 
                                             &pbClearULPK, &clearUlpkSize, aesKey, aesVector )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        return rStatus;
    }
    
#ifdef _OTP_TRANSIT_KEY
    /*** This is where the transitkey gets extracted from the ULPK and provided to 
         s_SecureStoreTransitKeyProvision() so it can be OTP'd */

    {
        UTV_BYTE ubTransitKey[ OTP_TRANSIT_KEY_LEN ];
        UTV_UINT32 ubTransitKeySize = OTP_TRANSIT_KEY_LEN;

        if ( UTV_OK != ( rStatus = UtvCommonDecryptSplitULPK( pbClearULPK, clearUlpkSize, 
                                                              ubTransitKey, 
                                                              &ubTransitKeySize  )))
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
            return rStatus;
        }
 
        /* write the transit key */
        if ( UTV_OK != ( rStatus = s_SC_OTPTransitKey(ubTransitKey, OTP_TRANSIT_KEY_LEN)))
        {
            if ( SC_ERROR_TransitKeyAlreadyBurned == rStatus )
            {
                UtvMessage( UTV_LEVEL_WARN, "s_SecureStoreTransitKeyProvision() TRANSIT KEY HAS BEEN PREVIOUSLY BURNED!" );
                /* return this as a warning, but continue */
                usWarning = SC_ERROR_TransitKeyAlreadyBurned;
                /* make sure the warning event is logged, but allow insertion to continue */
                UtvPlatformPersistWrite();
            } else
            {
                UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
                UtvPlatformPersistWrite();
                return rStatus;
            }
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "s_SecureStoreTransitKeyProvision() OK" );
        }
    }
#endif

    /* Switch to factory context to insert ULPK into factory partition */
    s_SetFactoryULPKContext();

    /* and now the ULPK itself */
    UtvMessage( UTV_LEVEL_INFO, "UtvPlatformSecureWrite( %s, clearUlpkSize = %d )", UtvCEMInternetGetULPKFileName(),clearUlpkSize );
    if ( UTV_OK != ( rStatus = UtvPlatformSecureWrite( UtvCEMInternetGetULPKFileName(), pbClearULPK, (UTV_UINT32)clearUlpkSize )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        /* revert to default ULPK context */
        s_SetDefaultULPKContext();
        return rStatus;
    }

    /* Read the ULPK structure from flash. */
    if ( UTV_OK != ( rStatus = UtvPlatformSecureGetULPK( &pDataCheck, &uiSizeCheck )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        /* revert to default ULPK context */
        s_SetDefaultULPKContext();
        return rStatus;
    }
    
    UtvMessage( UTV_LEVEL_INFO, "UtvPlatformSecureGetULPK( Size = %d )", uiSizeCheck );

    if ( clearUlpkSize != uiSizeCheck )
    {
        UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__, uiSizeCheck );
        UtvFree(pDataCheck);
        /* revert to default ULPK context */
        s_SetDefaultULPKContext();
        return UTV_BAD_SIZE;
    }
    
    for( i=0; i<uiSizeCheck; i++ )
    {
       if(pDataCheck[i] != pbClearULPK[i])
       {
           UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, UTV_DATA_VERIFY_FAILS, __LINE__ );
           UtvFree(pDataCheck);
           /* revert to default ULPK context */
           s_SetDefaultULPKContext();
           return UTV_DATA_VERIFY_FAILS;
       }       
    }

    /* free check data, we're done with it */
    UtvFree(pDataCheck);

    /* Call UtvCommonDecryptInit() which will sanity check the ULPK internals */
    if ( UTV_OK != ( rStatus = UtvCommonDecryptInit() ))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
    } else
    {
        if ( 0 == usWarning )
            UtvMessage( UTV_LEVEL_INFO, "ULPK Insertion OK");
        else
            UtvMessage( UTV_LEVEL_INFO, "ULPK Insertion with WARNING: %08X", (UTV_UINT32) usWarning );
        /* fold warning into high 16 bits */
        rStatus = usWarning;
        rStatus = (rStatus << 16);
    }    

    /* revert to default ULPK context */
    s_SetDefaultULPKContext();
    
    return rStatus;
}  

#ifdef _OTP_TRANSIT_KEY
/* s_SC_OTPTransitKey
   *** PORTING STEP:  Adapt to integrate a call to the secure core to OTP the transit key.
   Returns (0) if OK.
   Other values if not.
*/
static UTV_RESULT s_SC_OTPTransitKey(UTV_BYTE *pucTransitKey, UTV_UINT32 uiLength)
{
    /* OTP transit key here */


    /* return SC_ERROR_OK if success, other error values if not */
    return SC_ERROR_OK;
}
#endif

/* UtvPlatformSecureDecryptDirect
   *** PORTING STEP:  NONE.  Used to validate factory insertion.
   Returns value of inserted ULPK or 0xFFFFFFFF if there is an error.
*/
UTV_UINT32 UtvPlatformSecureGetULPKIndex( void )
{
    UTV_RESULT    rStatus;
    UTV_UINT32    uiPKID;

    /* implicit call to initialize system.  assumed to be unitialized. */
    UtvProjectOnEarlyBoot();

    if ( !s_bUtvInit )
        s_Init();

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    /* always get ULPK index from factory path */
    s_SetFactoryULPKContext();

    /* Init the decryption sub-system if necessary. */
    if ( UTV_OK != ( rStatus = UtvCommonDecryptInit() ))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        /* Revert to the default context */
        s_SetDefaultULPKContext();
        return 0xFFFFFF;
    }

    /* get the ULPK index */
    if ( UTV_OK != ( rStatus = UtvCommonDecryptGetPKID( &uiPKID )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_PLATFORM, rStatus, __LINE__ );
        /* Revert to the default context */
        s_SetDefaultULPKContext();
        return 0xFFFFFFFF;
    }
    
    /* Revert to the default context */
    s_SetDefaultULPKContext();

    return uiPKID;
}  

static UTV_RESULT s_AESDecrypt( UTV_BYTE *encData, UTV_UINT32 uiencDataSize,UTV_BYTE **clearData,UTV_UINT32 *uiclearDataSize,  UTV_BYTE *aesKey, UTV_BYTE *aesVector )
{
    UTV_RESULT        rStatus;
    UTV_UINT32        uiSize;

    int iInLen, iOutLen;
    EVP_CIPHER_CTX ctx;
    UTV_BYTE *pOut = NULL, *p = NULL;

    if ( NULL == ( pOut = (UTV_BYTE *)UtvMalloc( uiencDataSize ) ))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "s_AESDecrypt: UtvMalloc() fails: %s", UtvGetMessageString( rStatus ));
        return rStatus;
    }
    p = pOut;


    EVP_CIPHER_CTX_init( &ctx );
    EVP_DecryptInit_ex( &ctx, EVP_aes_128_cbc(), 0, aesKey, aesVector );
    EVP_CIPHER_CTX_set_padding( &ctx, 0 );

    iOutLen = iInLen = (int)uiencDataSize;
    if ( 0 == EVP_DecryptUpdate( &ctx, p, &iOutLen, encData, iInLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "s_AESDecrypt: EVP_DecryptUpdate() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    p += iOutLen; 
    uiSize = iOutLen;

    iOutLen = iInLen - iOutLen;
    if ( 0 == EVP_DecryptFinal_ex( &ctx, p, &iOutLen ))
    {
        UtvMessage( UTV_LEVEL_ERROR, "s_AESDecrypt: EVP_DecryptFinal_ex() failed" );
        UtvFree( pOut );
        return UTV_BAD_DECRYPTION;
    }
    uiSize += iOutLen;


    EVP_CIPHER_CTX_cleanup( &ctx );

    /* return the object data */
    *uiclearDataSize = uiSize;
    *clearData = pOut;

    return UTV_OK;
}

/* In eng and user builds set a flag so that the ULPK is found in the factory path.
   In a product build don't do anything. */
static void s_SetFactoryULPKContext( void )
{
#ifdef UTV_PRODUCTION_BUILD
    return;
#else
    UTV_RESULT rStatus;

    /* setup factory context */
    s_bULPKFactoryContext = UTV_TRUE;

    /* load ULPK material from the factory path */
    if ( UTV_OK != ( rStatus = UtvCommonDecryptInit() ))
    {
        UtvMessage( UTV_LEVEL_ERROR, 
                    "UtvCommonDecryptInit( to factory path ) fails: %s", 
                    UtvGetMessageString( rStatus ));
    }
#endif
}

/* In eng and user builds revert to the default context so that the 
   ULPK is found in read-only.  In a product build don't do anything. */
static void s_SetDefaultULPKContext( void )
{

#ifdef UTV_PRODUCTION_BUILD
    return;
#else
    UTV_RESULT rStatus;

    /* return to default context */
    s_bULPKFactoryContext = UTV_FALSE;

    /* load ULPK material from the read-only path */
    if ( UTV_OK != ( rStatus = UtvCommonDecryptInit() ))
    {
        UtvMessage( UTV_LEVEL_ERROR, 
                    "UtvCommonDecryptInit( to read-only path ) fails: %s", 
                    UtvGetMessageString( rStatus ));
    }
#endif
}

/* UtvPlatformSecureGetGUID
   *** PORTING STEP: Replace the contents of this function with code that retrieves a GUID.

   A GUID as used here must be a fixed value that will remain the same over power cycles.
   It can be created using any number of ways. The easiest and, for many implmentations,
   the best is to use a MAC address or a serial number.

   Another possibility is to use something like a UUID, string or binary. In this case, 
   however, the code must also provide a way to save this value in non-volatile storage
   after generation and then to provide a means to determine when to fetch that retrieved
   value or generate a new one.
*/
UTV_RESULT UtvPlatformSecureGetGUID( UTV_BYTE *pGUID, UTV_UINT16 *puiGUIDLength )
{
    /* Example code: a MAC address

       NB, puiGUIDLength is not only used to indicate how long the actual GUID array
       is, but also, on entry, the max size of the pGUID buffer. Do not exceed this
       value.
    */

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    (void)UtvPlatformGetWiFiMAC( (UTV_INT8 *)pGUID, *puiGUIDLength );
    *puiGUIDLength = UtvStrlen( (UTV_BYTE *) S_FAKE_WIFI_MAC);

    return UTV_OK;
}

/* UtvPlatformSecureWriteReadTest
   *** PORTING STEP: For test only.  
                     Change _SECURE_READ_WRITE_TEST_FILE_PATH to use path that's appropriate to your system.
*/

#define _SECURE_READ_WRITE_TEST_FILE_PATH    "./testfile.enc"
#define _SECURE_READ_WRITE_TEST_DATA_SIZE     256
#define _SECURE_READ_WRITE_TEST_ITERATIONS    1000

UTV_RESULT UtvPlatformSecureWriteReadTest( void )
{
    UTV_BYTE            cTestData[ _SECURE_READ_WRITE_TEST_DATA_SIZE ], *pbReadData;
    UTV_UINT32          i, n, uiLen;
    UTV_RESULT            rStatus;

    /* init if necessary */
    if ( !s_bUtvInit )
        s_Init();

    if ( UTV_OK != s_rSecureInit )
    {
        /* error will already have been logged */
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureOpen failed: %s", UtvGetMessageString( s_rSecureInit ) );        
        return s_rSecureInit;

    }

    /* look for the update root file which indicates this insertion event is related to an attempted update */
    for ( i = 0; i < _SECURE_READ_WRITE_TEST_ITERATIONS; i++ )
    {
        /* generate some test data that's unique */
        for ( n = 0; n < sizeof( cTestData ); n++ )
            cTestData[ n ] = (UTV_BYTE) (i + n);

        /* write it */
        if ( UTV_OK != (rStatus = UtvPlatformSecureWrite( _SECURE_READ_WRITE_TEST_FILE_PATH, cTestData , sizeof( cTestData ) )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureWrite() fails: %s", UtvGetMessageString( rStatus ));
            return rStatus;
        }
        
        /* read it back */
        if ( UTV_OK != (rStatus = UtvPlatformSecureRead( _SECURE_READ_WRITE_TEST_FILE_PATH, &pbReadData, &uiLen )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureRead() fails: %s", UtvGetMessageString( rStatus ));
            return rStatus;
        }

        /* compare it */
        for ( n = 0; n < sizeof( cTestData ); n++ )
        {
            if ( pbReadData[ n ] != (UTV_BYTE) (i + n) )
            {
                UtvMessage( UTV_LEVEL_ERROR, "Data read back [0x%02X] does not equal date written [0x%02X] at index %d, iteration %d", pbReadData[ n ],  (UTV_BYTE) (i + n), n, i );
                UtvFree( pbReadData );
                return UTV_BAD_DECRYPTION;
            }
        }
        
        /* free data read buffer */
        UtvFree( pbReadData );
    }
    
    return UTV_OK;
}

/*
*****************************************************************************************************
**
** The following functions typically do not have porting steps required of them but are included here
** just in case some customized adapatation is required.
**
*****************************************************************************************************
*/

/* UtvCEMGetPlatformOUI
    *** PORTING STEP:  None ***
    Return the OUI of the platform.
*/
UTV_UINT32 UtvCEMGetPlatformOUI( void )
{
    return UTV_CEM_OUI;
}

/* UtvCEMGetPlatformModelGroup
    *** PORTING STEP:  None ***
    Return the Model Group of the platform.
*/
UTV_UINT16 UtvCEMGetPlatformModelGroup( void )
{
        return UTV_CEM_MODEL_GROUP;
    /* snap on model groups are disabled */
#if 0
    return UtvManifestGetModelGroup();
#endif
}

/* UtvPlatformGetPersistentPath
    *** PORTING STEP:  None ***
    Return a path to the persistent R/W storage directory
*/
UTV_INT8 *UtvPlatformGetPersistentPath( void )
{
    return UTV_PLATFORM_PERSISTENT_PATH;
}

/* UtvCEMGetKeystoreFile
    *** PORTING STEP:  None ***
    Return a pointer to the keystore file name
*/
UTV_INT8 *UtvCEMGetKeystoreFile( void )
{
    return UTV_PLATFORM_KEYSTORE_FNAME;
}

/* UtvPlatformGetActiveCompManifestName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the active component manifest.
*/
UTV_INT8 *UtvPlatformGetActiveCompManifestName( void )
{
    return UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME;
}

/* UtvPlatformGetBackupCompManifestName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the backup component manifest.
*/
UTV_INT8 *UtvPlatformGetBackupCompManifestName( void )
{
    return UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME;
}

/* UtvPlatformGetUpdateRedirectName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the update redirect file.
*/
UTV_INT8 *UtvPlatformGetUpdateRedirectName( void )
{
    return UTV_PLATFORM_UPDATE_REDIRECT_FNAME;
}

/* UtvPlatformGetStore0Name
    *** PORTING STEP:  None ***
    Return a pointer to the name of the store 0 file.
*/
UTV_INT8 *UtvPlatformGetStore0Name( void )
{
    return UTV_PLATFORM_UPDATE_STORE_0_FNAME;
}

/* UtvPlatformGetStore1Name
    *** PORTING STEP:  None ***
    Return a pointer to the name of the store 1 file.
*/
UTV_INT8 *UtvPlatformGetStore1Name( void )
{
    return UTV_PLATFORM_UPDATE_STORE_1_FNAME;
}

/* UtvPlatformGetPersistName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the persistent storage file.
*/
UTV_INT8 *UtvPlatformGetPersistName( void )
{
    return UTV_PLATFORM_PERSISTENT_STORAGE_FNAME;
}

/* UtvPlatformGetRegstoreName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the regstore store and
    forward file.
*/
UTV_INT8 *UtvPlatformGetRegstoreName( void )
{
    return UTV_PLATFORM_REGSTORE_STORE_AND_FWD_FNAME;
}

/* UtvPlatformGetTSName
    *** PORTING STEP:  None ***
    Return a pointer to the name of the broadcast test TS file.
*/
UTV_INT8 *UtvPlatformGetTSName( void )
{
    return UTV_PLATFORM_TEST_TS_FNAME;
}

/* UtvCEMConfigure
    *** PORTING STEP:  None ***
    CEM-specific configuration routine that is called on intialization.
    The stat, mkdir, and copy are Linux specific and will have to be modified
    for non-Linux systems.
*/
UTV_RESULT UtvCEMConfigure( void )
{
    /* announce ULI system version */
    UtvMessage( UTV_LEVEL_INFO , "||===================||" );
    UtvMessage( UTV_LEVEL_INFO , "||= Booting System  =||" );
#if 0
    UtvMessage( UTV_LEVEL_INFO , "||= Booting System With ULI Version %5u =||", UtvManifestGetSoftwareVersion() );
#endif
    UtvMessage( UTV_LEVEL_INFO , "||===================||" );

    return UTV_OK;
}

#ifdef UTV_DELIVERY_FILE

/* UtvPlatformFileAcquireHardware
    Called by the core to give the CEM with an opportunity to establish access to the file access
    hardware and arbitrate conflicts before a series of file open/read/write/close calls.
*/
UTV_RESULT UtvPlatformFileAcquireHardware( UTV_UINT32 uiEventType )
{
    return UTV_OK;
}

/* UtvPlatformInternetReleaseHardware
    Called by the core to indicate that the socket resource is being freed until the next call
    to UtvPlatformInternetAcquireHardware().
*/
UTV_RESULT UtvPlatformFileReleaseHardware()
{
    return UTV_OK;
}

static UTV_RESULT s_UtvSetUpdateFilePath( UTV_INT8 *pszFilePath )
{
    UtvStrnCopy( (UTV_BYTE *)s_cUpdateFilePath, sizeof(s_cUpdateFilePath), ( UTV_BYTE *)pszFilePath, UtvStrlen( ( UTV_BYTE *)pszFilePath ) );
    s_cUpdateFilePath[ UtvStrlen( ( UTV_BYTE *)pszFilePath ) ] = 0;
    return UTV_OK;
}

/* UtvCEMGetUpdateFilePath
    *** PORTING STEP:  None ***
    Returns a path to where update files of interested may be stored.
*/
UTV_RESULT UtvCEMGetUpdateFilePath( UTV_INT8 *pszFilePath, UTV_UINT32 uiBufferLen )
{
    if ( UtvStrlen( ( UTV_BYTE *)s_cUpdateFilePath ) + 1 > uiBufferLen )
        return UTV_NO_MEMORY;

    UtvStrnCopy( (UTV_BYTE *)pszFilePath, uiBufferLen, ( UTV_BYTE *)s_cUpdateFilePath, UtvStrlen( ( UTV_BYTE *)s_cUpdateFilePath ) );

    pszFilePath[ UtvStrlen( ( UTV_BYTE *)s_cUpdateFilePath ) ] = 0;

    return UTV_OK;
}




#endif

#ifdef UTV_DELIVERY_INTERNET

/* UtvCEMInternetQueryHost
    *** PORTING STEP:  None ***
    Return a pointer to the query host.
*/
UTV_INT8 *UtvCEMGetInternetQueryHost( void )
{
    return UtvManifestGetQueryHost();
}

/* UtvCEMSSLGetCertFileName
    *** PORTING STEP:  None ***
    Return a pointer to the ULI cert verification file
*/
UTV_INT8 *UtvCEMSSLGetCertFileName( void )
{
    return UTV_PLATFORM_INTERNET_SSL_CA_CERT_FNAME;
}

/* UtvCEMInternetGetULIDFileName( void )
    *** PORTING STEP:  None ***
    Return a pointer to the ULID file name.
*/
UTV_INT8 *UtvCEMInternetGetULIDFileName( void )
{
    return UTV_PLATFORM_INTERNET_ULID_FNAME;
}

/* UtvCEMInternetGetULPKFileName
    *** PORTING STEP:  None ***
    Return a pointer to the ULPK file name.
*/
UTV_INT8 *UtvCEMInternetGetULPKFileName( void )
{
    /* The path returned here depends on whether UTV_PRODUCTION_BUILD is defined or not.
       If in the "factory context" (using insert ULPK or get ULPK index) then no matter what the build variant is we 
       return the factory path to the ULPK.
       If in a non-factory context then return a path that depends on wether UTV_PRODUCTION_BUILD is defined or not.
       Any device talking to the production NOC uses the factory path.
       Any device talking to the extdev NOC uses the read-only path which should
       retrieve the universal (PKID==0) ULPK. */
#ifdef UTV_PRODUCTION_BUILD
#define __PRODUCT_BUILD UTV_TRUE
#else
#define __PRODUCT_BUILD UTV_FALSE
#endif
    UTV_BOOL bProductVariant = __PRODUCT_BUILD;

    if ( s_bULPKFactoryContext || bProductVariant )
        return UTV_PLATFORM_INTERNET_ULPK_FNAME;
    else
        return UTV_PLATFORM_INTERNET_READ_ONLY_ULPK_FNAME;
}

/* UtvCEMInternetGetProvisionerPath
    *** PORTING STEP:  None ***
    Return a pointer to the Provisioner directory.
*/
UTV_INT8 *UtvCEMInternetGetProvisionerPath( void )
{
    return UTV_PLATFORM_INTERNET_PROVISIONER_PATH;
}
#endif

/*
*****************************************************************************************************
**
** STATIC UTILITY THREADS
** These routine should not need to be adapted.
**
*****************************************************************************************************
*/

/* s_UtvGetSchduleThread
    *** PORTING STEP: None, UNLESS you would like to change the order
                      of the internet/broadcast delivery mode ***
*/
static void *s_UtvGetScheduleThread( void *pArg )
{
    UTV_RESULT                          rStatus = UTV_NO_DOWNLOAD, lStatus;
    UTV_UINT32                          uiDeliveryModes[2];
    UTV_UINT32                          uiDeliveryCount, uiMsgLevel, uiLinesToSkip = 0;
    UTV_PUBLIC_RESULT_OBJECT           *pResult;
    UTV_PUBLIC_SCHEDULE                *pSchedule;
    UTV_PROJECT_GET_SCHEDULE_REQUEST   *pReq = (UTV_PROJECT_GET_SCHEDULE_REQUEST *)pArg;
    UTV_BOOL                            bStoredOnly = pReq->bStored;
    UTV_BOOL                            bFactoryMode = pReq->bFactoryMode;
    UTV_BOOL                            bDeliveryFile = pReq->bDeliveryFile;
    UTV_BOOL                            bDeliveryInternet = pReq->bDeliveryInternet;
    UTV_UINT32                          uiEventType = pReq->uiEventType;

    UtvDebugSetThreadName( pReq->pszThreadName );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        UTV_THREAD_EXIT;
    }

    /* N.B.  The order of these two delivery method setup commands controls the order the
       of internet vs. broadcast update search.
    */
    uiDeliveryCount = 0;
#ifdef UTV_DELIVERY_FILE
    if ( bDeliveryFile )
    {
        /* file delivery can only occur in isolation */
        uiDeliveryModes[uiDeliveryCount++] = UTV_PUBLIC_DELIVERY_MODE_FILE;
    } else
    {
#endif
#ifdef UTV_DELIVERY_INTERNET
        if ( bDeliveryInternet )
            uiDeliveryModes[uiDeliveryCount++] = UTV_PUBLIC_DELIVERY_MODE_INTERNET;
#endif
#ifdef UTV_DELIVERY_FILE
    }
#endif

    /* error exit do while false */
    while ( UTV_TRUE )
    {
        /* setup delivery configuration */
        if ( UTV_OK != ( lStatus = UtvConfigureDeliveryModes( uiDeliveryCount, uiDeliveryModes )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvConfigureDeliveryModes() \"%s\"", UtvGetMessageString( lStatus ) );
            break;
        }

        /* if this is a file delivery assign the path found to a "store" */
        if ( bDeliveryFile )
        {
            if ( UTV_OK != ( lStatus = UtvPlatformUtilityAssignFileToStore( uiLinesToSkip++ )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformUtilityAssignFileToStore(): %s", UtvGetMessageString( lStatus ) );
                break;
            }
        }

        /* Retrieve the components that are available for download */
        rStatus = UtvPublicGetDownloadSchedule( bStoredOnly, bFactoryMode, NULL, 0, &pSchedule, UTV_TRUE );

        /* set message type and report response */
        switch ( rStatus )
        {
        case UTV_OK:
        case UTV_NO_DOWNLOAD:
            uiMsgLevel = UTV_LEVEL_INFO;
            break;
        case UTV_COMMANDS_DETECTED:
            uiMsgLevel = UTV_LEVEL_WARN;
            break;
        default:
            uiMsgLevel = UTV_LEVEL_ERROR;
            break;
        }

        UtvMessage( uiMsgLevel, "UtvPublicGetDownloadSchedule( stored only: %d, factory mode: %d, &pSchedule ) \"%s\"",
                    bStoredOnly, bFactoryMode, UtvGetMessageString( rStatus ) );
        g_pUtvDiagStats->utvLastError.usError = rStatus;
        g_pUtvDiagStats->utvLastError.tTime = UtvTime( NULL );
        g_pUtvDiagStats->utvLastError.usCount++;

        /* if this is a file delivery and a download or command were not found then try the next file in updatelogic.txt */
        if ( bDeliveryFile && (UTV_OK != rStatus) && (UTV_COMMANDS_DETECTED != rStatus) )
            continue;

        /* we found a schedule that has a compatible update */
        break;
    }

    pResult = UtvGetAsyncResultObject();

    /* set the event type. */
    pResult->iToken  = uiEventType;
    /* set the result from above */
    pResult->rOperationResult = rStatus;

/* if a callback function has been provided then call it. */
    /*if ( NULL != s_fCallbackFunction )
        (s_fCallbackFunction)( pResult );
    else
        UtvMessage( UTV_LEVEL_WARN, "NO CALLBACK REGISTERED!" );*/
    if(UTV_COMMANDS_DETECTED == rStatus){
        lStatus = UtvImageProcessUSBCommands();
    }

    /* release access mutex */
    if ( UTV_OK != ( lStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( lStatus ) );
    }

    UTV_THREAD_EXIT;
}

static UTV_RESULT s_GetScheduleBlocking(void *pArg )
{
    UTV_RESULT                          rStatus = UTV_NO_DOWNLOAD, lStatus;
    UTV_UINT32                          uiDeliveryModes[2];
    UTV_UINT32                          uiDeliveryCount, uiMsgLevel, uiLinesToSkip = 0;
    UTV_PUBLIC_SCHEDULE                *pSchedule;
    UTV_PROJECT_GET_SCHEDULE_REQUEST   *pReq = (UTV_PROJECT_GET_SCHEDULE_REQUEST *)pArg;
    UTV_BOOL                            bStoredOnly = pReq->bStored;
    UTV_BOOL                            bFactoryMode = pReq->bFactoryMode;
    UTV_BOOL                            bDeliveryFile = pReq->bDeliveryFile;
    UTV_BOOL                            bDeliveryInternet = pReq->bDeliveryInternet;

    UtvMessage( UTV_LEVEL_INFO, "lock in" );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "lock out" );

    /* N.B.  The order of these two delivery method setup commands controls the order the
       of internet vs. broadcast update search.
    */
    uiDeliveryCount = 0;
#ifdef UTV_DELIVERY_FILE
    if ( bDeliveryFile )
    {
        /* file delivery can only occur in isolation */
        uiDeliveryModes[uiDeliveryCount++] = UTV_PUBLIC_DELIVERY_MODE_FILE;
    } else
    {
#endif
#ifdef UTV_DELIVERY_INTERNET
        if ( bDeliveryInternet )
            uiDeliveryModes[uiDeliveryCount++] = UTV_PUBLIC_DELIVERY_MODE_INTERNET;
#endif

#ifdef UTV_DELIVERY_FILE
    }
#endif

    while ( UTV_TRUE )
    {
        /* setup delivery configuration */
        if ( UTV_OK != ( lStatus = UtvConfigureDeliveryModes( uiDeliveryCount, uiDeliveryModes )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvConfigureDeliveryModes() \"%s\"", UtvGetMessageString( lStatus ) );
            break;
        }

        /* if this is a file delivery assign the path found to a "store" */
        if ( bDeliveryFile )
        {
            if ( UTV_OK != ( lStatus = UtvPlatformUtilityAssignFileToStore( uiLinesToSkip++ )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformUtilityAssignFileToStore(): %s", UtvGetMessageString( lStatus ) );
                break;
            }
        }

        /* Retrieve the components that are available for download */
        rStatus = UtvPublicGetDownloadSchedule( bStoredOnly, bFactoryMode, NULL, 0, &pSchedule, UTV_TRUE );

        /* set message type and report response */
        switch ( rStatus )
        {
        case UTV_OK:
            UtvProjectSetUpdateSchedule(pSchedule);
            uiMsgLevel = UTV_LEVEL_INFO;
            break;
        case UTV_NO_DOWNLOAD:
            UtvProjectSetUpdateSchedule(NULL);
            uiMsgLevel = UTV_LEVEL_INFO;
            break;
        case UTV_COMMANDS_DETECTED:
            uiMsgLevel = UTV_LEVEL_WARN;
            break;
        default:
            uiMsgLevel = UTV_LEVEL_ERROR;
            break;
        }

        UtvMessage( uiMsgLevel, "UtvPublicGetDownloadSchedule( stored only: %d, factory mode: %d, &pSchedule ) \"%s\"",
                    bStoredOnly, bFactoryMode, UtvGetMessageString( rStatus ) );

        g_pUtvDiagStats->utvLastError.usError = rStatus;
        g_pUtvDiagStats->utvLastError.tTime = UtvTime( NULL );
        g_pUtvDiagStats->utvLastError.usCount++;

        /* if this is a file delivery and a download or command were not found then try the next file in updatelogic.txt */
        if ( bDeliveryFile && (UTV_OK != rStatus) && (UTV_COMMANDS_DETECTED != rStatus) )
            continue;

        /* we found a schedule that has a compatible update */
        break;
    }
    
    if(UTV_COMMANDS_DETECTED == rStatus){
        lStatus = UtvImageProcessUSBCommands();
    }

    UtvMessage( UTV_LEVEL_INFO, "unlock in" );

    /* release access mutex */
    if ( UTV_OK != ( lStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( lStatus ) );
    }

    UtvMessage( UTV_LEVEL_INFO, "unlock out" );

    return rStatus;
}

/* s_UtvDownloadThread
    *** PORTING STEP: None, UNLESS you would like to change the order
                      of the internet/broadcast delivery mode ***
*/
static void *s_UtvDownloadThread( void *pArg )
{
    UTV_RESULT                      rStatus;
    UTV_PROJECT_DOWNLOAD_REQUEST   *pReq = (UTV_PROJECT_DOWNLOAD_REQUEST *)pArg;
    UTV_BOOL                        bFactory = pReq->bFactoryMode;
    UTV_PUBLIC_UPDATE_SUMMARY         *pUpdate = pReq->pUpdate;
    UTV_PUBLIC_RESULT_OBJECT          *pResult;

    UtvDebugSetThreadName( pReq->pszThreadName );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        UTV_THREAD_EXIT;
    }

    /* error exit do while false */
    do
    {
        /* Look for aborts (power ons) issued while we're sleeping
           If found skip the rest of the update mechanism
        */
        if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        {
            UtvMessage( UTV_LEVEL_INFO, "Aborting from sleep" );
            rStatus = UTV_ABORT;
            break;
        }

        /* Download now */
        rStatus = UtvPublicDownloadComponents( pUpdate, bFactory, NULL, 0 );
        if ( UTV_DOWNLOAD_COMPLETE != rStatus && UTV_UPD_INSTALL_COMPLETE != rStatus )
        {
            UtvMessage( UTV_LEVEL_ERROR,
                        "UtvPublicDownloadComponents( pUpdate, %d, NULL, 0 ) \"%s\"",
                        bFactory, UtvGetMessageString( rStatus ) );
            break;
        } else
        {
            UtvMessage( UTV_LEVEL_INFO, "UtvPublicDownloadComponents( &pSchedule->tUpdates[ 0 ], UTV_FALSE, NULL, 0 ) success" );
        }

    } while ( UTV_FALSE );

    pResult = UtvGetAsyncResultObject();

    /* set the event type. */
    pResult->iToken  = UTV_PROJECT_SYSEVENT_DOWNLOAD;

    /* set the result from above */
    pResult->rOperationResult = rStatus;

    g_pUtvDiagStats->utvLastError.usError = rStatus;
    g_pUtvDiagStats->utvLastError.tTime = UtvTime( NULL );
    g_pUtvDiagStats->utvLastError.usCount++;

    /* if a callback function has been provided then call it. */
    if ( NULL != s_fCallbackFunction )
        (s_fCallbackFunction)( pResult );
    else
        UtvMessage( UTV_LEVEL_WARN, "NO CALLBACK REGISTERED!" );

    /* release access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
    }

    UTV_THREAD_EXIT;
}

#ifdef UTV_DELIVERY_FILE
/* s_UtvInstallFromMediaThread
    *** PORTING STEP: None***
*/
static void *s_UtvInstallFromMediaThread( void *pArg )
{
    UTV_RESULT                  rStatus;
    UTV_PUBLIC_UPDATE_SUMMARY  *pUpdate = (UTV_PUBLIC_UPDATE_SUMMARY  *)pArg;
    UTV_UINT32                    uiDeliveryMode;
    UTV_PUBLIC_RESULT_OBJECT   *pResult;

    UtvDebugSetThreadName( "install" );

    /* attempt to take access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexLock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexLock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
        UTV_THREAD_EXIT;
    }

    uiDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_FILE;

    /* error exit do while false */
    do
    {
        /* setup delivery configuration */
        if ( UTV_OK != ( rStatus = UtvConfigureDeliveryModes( 1, &uiDeliveryMode )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvConfigureDeliveryModes() \"%s\"", UtvGetMessageString( rStatus ) );
            break;
        }

        UtvMessage( UTV_LEVEL_INFO, "delivery config ok" );

        /* Approve all components for install */
        if ( UTV_OK != ( rStatus = UtvPublicApproveForInstall( pUpdate )))
        {
            UtvMessage( UTV_LEVEL_ERROR,
                        "UtvPublicApproveForInstall( pUpdate ] ) \"%s\"",
                        UtvGetMessageString( rStatus ) );
            break;
        }

        UtvMessage( UTV_LEVEL_INFO, "approved" );

        /* UtvDownloadEnded for USB tells NOC how we got the update */
        if ( UTV_DOWNLOAD_COMPLETE != ( rStatus = UtvPublicInstallComponents( pUpdate, UTV_FALSE, NULL, 0 ) ))
        {
            UtvMessage( UTV_LEVEL_ERROR,
                        "UtvPublicInstallComponents( pUpdate, UTV_FALSE, NULL, 0 ) \"%s\"",
                        UtvGetMessageString( rStatus ) );
        } else
        {
            UtvMessage( UTV_LEVEL_INFO,
                        "UtvPublicInstallComponents( pUpdate, UTV_FALSE, NULL, 0 ) success" );
        }
    } while ( UTV_FALSE );

    UtvMessage( UTV_LEVEL_INFO, "installed" );

    pResult = UtvGetAsyncResultObject();

    /* set the event type. */
    pResult->iToken  = UTV_PROJECT_SYSEVENT_MEDIA_INSTALL;
    /* set the result from above */
    pResult->rOperationResult = rStatus;

    g_pUtvDiagStats->utvLastError.usError = rStatus;
    g_pUtvDiagStats->utvLastError.tTime = UtvTime( NULL );
    g_pUtvDiagStats->utvLastError.usCount++;

    /* if a callback function has been provided then call it. */
    if ( NULL != s_fCallbackFunction )
        (s_fCallbackFunction)( pResult );
    else
        UtvMessage( UTV_LEVEL_WARN, "NO CALLBACK REGISTERED!" );

    /* release access mutex */
    if ( UTV_OK != ( rStatus = UtvMutexUnlock( s_AccessMutex )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMutexUnlock( s_AccessMutex ) \"%s\"", UtvGetMessageString( rStatus ) );
    }

    UTV_THREAD_EXIT;
}
#endif

#if defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT)
/**
 **********************************************************************************************
 *
 * Provides general device information used by Local and Live Support functionality.
 *
 * Example general device information:
 *
 *     {"Make" : "Acme",
 *      "Serial Number" : "ABC123",
 *      "Model" : "HTP-612",
 *      "Software Version" : "1.78.2.14A",
 *      "For Support Call" : "1-800-555-1212"}
 *
 * The default implementation provides phony device information for the expected fields as an
 * example.
 *
 * @intg
 *
 * A replacement implementation should return corrected device information for the expected
 * fields and can append additional device-specific fields as desired.
 * 
 * @param   pDeviceInfo   Device information is added to this buffer.
 *
 * @return  UTV_OK if device information was provided successfully.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvCEMGetDeviceInfo( cJSON* pDeviceInfo )
{
  UTV_INT8 pszSerialNumber[48];

  /*
   * Validate the parameters.
   */

  if ( NULL == pDeviceInfo ||
       cJSON_Object != pDeviceInfo->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO RETRIEVE THE EXPECTED DEVICE
  ** INFORMATION AND ADD THE VALUES TO THE JSON OBJECT.
  */

  UtvCEMGetSerialNumber( pszSerialNumber, 25 );

  cJSON_AddStringToObject( pDeviceInfo, "Make", "Acme" );
  cJSON_AddStringToObject( pDeviceInfo, "Serial Number", pszSerialNumber );
  cJSON_AddStringToObject( pDeviceInfo, "Model", "HTP-612" );
  cJSON_AddStringToObject( pDeviceInfo, "Software Version", "1.78.2.14A" );
  cJSON_AddStringToObject( pDeviceInfo, "For Support Call", "1-800-555-1212" );

  return ( UTV_OK );
}

/**
 **********************************************************************************************
 *
 * Provides device network information used by Local and Live Support functionality.
 *
 * Example device network information:
 *
 *    @verbatim
 *     {"Interfaces" : [{"NAME_1" : {"Type" : "Wired/Wireless",
 *                                   "Status" : "Disabled/Connected/Disconnected",
 *                                   "Network Name" : "MyNet",
 *                                   "Security Key" : "theFamilyNet!",
 *                                   "DHCP" : "Enabled/Disabled",
 *                                   "IP Address" : "192.169.10.100",
 *                                   "Gateway" : "192.168.2.1",
 *                                   "Subnet Mask" : "255.255.255.0",
 *                                   "Physical Address" : "AB:CD:EF:01:23:45",
 *                                   "First DNS Server" : "192.168.2.1",
 *                                   "Second DNS Server" : "68.87.72.134",
 *                                   "Third DNS Server" : "68.87.72.135",
 *                                   "Broadcast Address" : "0.0.0.0",
 *                                   "Metrics" : {"Received Bytes" : 1234,
 *                                                "Recieved Packets" : 1234,
 *                                                "Received Compressed Packets" : 1234,
 *                                                "Received Multicast Packets" : 1234,
 *                                                "Received Errors" : 1234,
 *                                                "Received Packets Dropped" : 1234,
 *                                                "Received FIFO Errors" : 1234,
 *                                                "Received Frame Errors" : 1234,
 *                                                "Transmitted Bytes" : 1234,
 *                                                "Transmitted Packets" : 1234,
 *                                                "Transmitted Errors" : 1234,
 *                                                "Transmitted Packets Dropped" : 1234,
 *                                                "Transmitted FIFO Errors" : 1234,
 *                                                "Transmitted Collisions" : 1234,
 *                                                "Transmitted Carrier Errors" : 1234}}},
 *                      {"NAME_2" : { SAME AS ABOVE }}],
 *      "Routes" : [{"Route to IP_ADDRESS_1" : {"Gateway" : "192.168.2.1",
 *                                              "Subnet Mask" : "255.255.255.255",
 *                                              "Metric" : 1234,
 *                                              "Reference Count" : 1234,
 *                                              "Use" : 1234,
 *                                              "Interface" : 1234},
 *                  {"Route to IP_ADDRESS_2" : { SAME AS ABOVE }}]}}
 *    @endverbatim
 *
 * The default implementation leverages UtvPlatformGetNetworkInfo().
 *
 * @intg
 *
 * Customizations to the functionality of this routine can be made in this function or, more
 * generally, within UtvPlatformGetNetworkInfo().
 * 
 * @param   pNetworkInfo  Device network information is added to this buffer.
 *
 * @return  UTV_OK if device network information was provided successfully.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvCEMGetNetworkInfo( cJSON* pNetworkInfo )
{
  UTV_RESULT result = UTV_OK;

  /*
   * Validate the parameters.
   */

  if ( NULL == pNetworkInfo ||
       cJSON_Object != pNetworkInfo->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** REVIEW THE IMPLEMENTATION OF THE PLATFORM ADAPTATION METHOD, UtvPlatformGetNetworkInfo,
  ** AND IF NECESSARY REPLACE THAT WITH CALLS TO THE APPROPRIATE DEVICE SPECIFIC PROCESSING
  ** ROUTINES TO RETRIEVE THE EXPECTED NETWORK INFORMATION AND ADD THE VALUES TO THE JSON
  ** OBJECT.
  */

  result = UtvPlatformGetNetworkInfo( pNetworkInfo );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Performs and reports the results of network diagnostics for the purposes of Local and Live
 * Support functionality.
 *
 * Example network diagnostic results:
 *
 *     {"Local Ping" : "Pass/Fail",
 *      "Internet Ping" : "Pass/Fail",
 *      "DNS Lookup" : "Pass/Fail",
 *      "Speed (Up)" : "1.2",
 *      "Speed (Down)" : "8.5",
 *      "Latency" : "550",
 *      "Wireless Signal" : "-42",
 *      "Wireless Speed" : "54"}
 *
 * The default implementation leverages platform routines and also provides phony results
 * as an example.
 *
 *
 * @intg
 *
 * A replacement implementation should perform the network diagnostics and report the actual
 * results in the same format as the example.
 * 
 * @param   pNetworkDiagnostics  Network diagnostic results are added to this buffer.
 *
 * @return  UTV_OK if network diagnostic results were provided successfully.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvCEMNetworkDiagnostics( cJSON* pNetworkDiagnostics )
{
  UTV_RESULT result = UTV_OK;
  UTV_UINT32 uiAddress = 0;
  UTV_INT8 szAddress[32];

  /*
   * Validate the parameters.
   */

  if ( NULL == pNetworkDiagnostics ||
       cJSON_Object != pNetworkDiagnostics->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A LOCAL PING
  ** DIAGNOSTIC TEST AND ADD "Pass" or "Fail" TO THE JSON OBJECT.
  **
  ** Execute a Local Ping diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Local Ping", "Pass" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE AN INTERNET PING
  ** DIAGNOSTIC TEST AND ADD "Pass" or "Fail" TO THE JSON OBJECT.
  **
  ** Execute an Internet Ping diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Internet Ping", "Pass" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A DNS LOOKUP
  ** DIAGNOSTIC TEST AND ADD "Pass" or "Fail" TO THE JSON OBJECT.
  **
  ** Execute a DNS Lookup diagnostic test.
  */

  memset( szAddress, 0x00, 32 );
  result = UtvPlatformInternetCommonDNS( "www.updatelogic.com", &uiAddress );

  cJSON_AddStringToObject(
    pNetworkDiagnostics,
    "DNS Lookup",
    (result == UTV_OK) ? "Pass" : "Fail" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A NETWORK UPLOAD
  ** SPEED DIAGNOSTIC TEST AND ADD THE RESULT TO THE JSON OBJECT.
  **
  ** Execute a Network Speed (Up) diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Speed (Up)", "1.2" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A NETWORK DOWNLOAD
  ** SPEED DIAGNOSTIC TEST AND ADD THE RESULT TO THE JSON OBJECT.
  **
  ** Execute a Network Speed (Down) diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Speed (Down)", "8.5" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A NETWORK LATENCY
  ** DIAGNOSTIC TEST AND ADD THE RESULT TO THE JSON OBJECT.
  **
  ** Execute a Latency diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Latency", "550" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A WIRELESS SIGNAL
  ** DIAGNOSTIC TEST AND ADD THE RESULT TO THE JSON OBJECT.
  **
  ** Execute a Wireless Signal diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Wireless Signal", "-42" );

  /*
  ** PORTING STEP:
  ** CALL THE APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES TO EXECUTE A WIRELESS SPEED
  ** DIAGNOSTIC TEST AND ADD THE RESULT TO THE JSON OBJECT.
  **
  ** Execute a Wireless Speed diagnostic test.
  */

  cJSON_AddStringToObject( pNetworkDiagnostics, "Wireless Speed", "54" );

  return ( result );
}
#endif /* defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT) */

#if defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT)
/* UtvCEMRemoteButton
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC HANDLING.
   ***

   pRemoteButtons is a pointer to a cJSON object containing the remote control buttons to
    process.  For this command the inputs consist of a single item, "BUTTONS".  The "BUTTONS"
    item contains an array of button values to process where the button values can be one of
    the defines with the "UTV_REMOTE_BUTTON_" prefix found in utv-platform-cem.h.

      {"BUTTONS" : [22, 23, 22, 23]}

   Returns UTV_OK if the processing executed successfully
   Returns other UTV_RESULT value if errors occur.
*/
UTV_RESULT UtvCEMRemoteButton( cJSON* pRemoteButtons )
{
  UTV_RESULT result = UTV_OK;
  UTV_RESULT eventResult;
  UTV_INT32 idx;
  cJSON* buttons = NULL;
  cJSON* button = NULL;
  cJSON* event = NULL;
  cJSON* eventButtons = NULL;
  UTV_BYTE sessionCode[SESSION_CODE_SIZE];

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == pRemoteButtons )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  buttons = cJSON_GetObjectItem( pRemoteButtons, "BUTTONS" );

  /*
   * Validate the inputs, there must be a "BUTTONS" item, it must be an array and it must
   * contain at least one item.
   */

  if ( NULL == buttons ||
       cJSON_Array != buttons->type ||
       0 == cJSON_GetArraySize( buttons ) )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
   * Get the session code for this Live Support session to be used in the remote button
   * events.
   */

  result = UtvLiveSupportGetSessionCode( sessionCode, SESSION_CODE_SIZE );

  if ( UTV_OK != result )
  {
    return ( result );
  }

  /*
   * The inputs to a Remote Button command is an array of buttons.  Loop through the array
   * and process each button.
   */
  for ( idx = 0; idx < cJSON_GetArraySize( buttons ); idx++ )
  {
    button = cJSON_GetArrayItem( buttons, idx );

    /*
    ** PORTING STEP:
    ** PROCESS THE INDIVIDUAL BUTTON HERE BY CONVERTING IT FROM THE "UTV_REMOTE_BUTTON_"
    ** VALUE TO A DEVICE SPECIFIC VALUE AND CALLING THE APPROPRIATE DEVICE SPECIFIC PROCESSING
    ** ROUTINES.
    */

    UtvMessage( UTV_LEVEL_INFO,
                "%s, Processed Remote Button: 0x%8.8x",
                __FUNCTION__,
                button->valueint );

    /*
     * If the remote button processing was successful then create an event for the remote
     * button.
     *
     * NOTE: This code is generating one event per remote button in the array of buttons.
     *       The code could be written to send a single event with the array of buttons as
     *       the event data by moving the event creation outside of the loop.
     */

    event = cJSON_CreateObject();
    eventButtons = cJSON_CreateArray();

    if ( NULL != event && NULL != eventButtons )
    {
      cJSON_AddItemToArray( eventButtons, cJSON_CreateNumber( button->valueint ) );

      cJSON_AddStringToObject( event, "SESSION_CODE", (char*)sessionCode );
      cJSON_AddItemToObject( event, "BUTTONS", eventButtons );

      eventResult =
        UtvEventsAdd( EVENT_TYPE_LIVESUPPORT, LIVESUPPORT_EVENT_REMOTE_BUTTON, event );

      if ( UTV_OK != eventResult )
      {
        UtvMessage(
          UTV_LEVEL_ERROR,
          "%s, Failed to add remote button event, %s.",
          __FUNCTION__,
          UtvGetMessageString( eventResult ) );
      }
    }
    else
    {
      cJSON_Delete( event );
      cJSON_Delete( eventButtons );

      UtvMessage(
        UTV_LEVEL_ERROR,
        "%s, Failed to create event objects.",
        __FUNCTION__ );
    }
  }

  /*
   * If the remote button processing was successful then create an event for the set of
   * processed remote buttons.
   *
   * NOTE: If a single event is desired for the remote buttons then this code should be used
   *       instead of the code inside the loop that generates an event per remote button.
   */

  /*
  event = cJSON_CreateObject();

  if ( NULL != event )
  {
    char* szButtons = cJSON_PrintUnformatted( buttons );

    cJSON_AddStringToObject( event, "SESSION_CODE", (char*)sessionCode );

    cJSON_AddItemToObject(
      event,
      "BUTTONS",
      cJSON_Parse( szButtons ) );

    free( szButtons );

    eventResult =
      UtvEventsAdd( EVENT_TYPE_LIVESUPPORT, LIVESUPPORT_EVENT_REMOTE_BUTTON, event );

    if ( UTV_OK != eventResult )
    {
      UtvMessage(
        UTV_LEVEL_ERROR,
        "%s, Failed to add remote button event, %s.",
        __FUNCTION__,
        UtvGetMessageString( eventResult ) );
    }
  }
  else
  {
    cJSON_Delete( event );
    cJSON_Delete( eventButtons );

    UtvMessage(
      UTV_LEVEL_ERROR,
      "%s, Failed to create event objects.",
      __FUNCTION__ );
  }
  */

  return ( result );
}
#endif /* defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT) */

#ifdef UTV_LAN_REMOTE_CONTROL
/* UtvCEMLanRemoteControlInitialize
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC
   *** INITIALIZATION INCLUDING THE SETUP OF COMMAND HANDLERS.  IF CERTAIN COMMANDS
   *** ARE NOT HANDLED ON A GIVEN PLATFORM THEY SHOULD NOT BE REGISTERED.
   ***

   Command requests can include the following request names:
      LAN_REMOTE_CONTROL_REMOTE_BUTTON
*/
void UtvCEMLanRemoteControlInitialize( void )
{
  #ifdef UTV_LAN_REMOTE_CONTROL_WEB_SERVER
  /*
   * Register post command handlers with the web server to allow these commands to be
   * processed.
   */

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LAN_REMOTE_CONTROL_REMOTE_BUTTON",
                                             s_LanRemoteControlRemoteButton );
  #endif /* UTV_LAN_REMOTE_CONTROL_WEB_SERVER */
}
#endif /* UTV_LAN_REMOTE_CONTROL */

#ifdef UTV_LAN_REMOTE_CONTROL_WEB_SERVER
/* s_LanRemoteControlRemoteButton
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to process a remote button for
    the device.
*/
static UTV_RESULT s_LanRemoteControlRemoteButton( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;

  result = UtvCEMRemoteButton( inputs );

  return ( result );
}
#endif /* UTV_LAN_REMOTE_CONTROL_WEB_SERVER */

#ifdef UTV_LOCAL_SUPPORT
/* UtvCEMLocalSupportInitialize
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC
   *** INITIALIZATION INCLUDING THE SETUP OF COMMAND HANDLERS.  IF CERTAIN COMMANDS
   *** ARE NOT HANDLED ON A GIVEN PLATFORM THEY SHOULD NOT BE REGISTERED.
   ***

   Command requests can include the following request names:
      LOCAL_SUPPORT_GET_DEVICE_INFO
      LOCAL_SUPPORT_GET_NETWORK_INFO
      LOCAL_SUPPORT_NETWORK_DIAGNOSTICS
*/
void UtvCEMLocalSupportInitialize( void )
{
  #ifdef UTV_LOCAL_SUPPORT_WEB_SERVER
  /*
   * Register post command handlers with the web server to allow these commands to be
   * processed.
   */

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LOCAL_SUPPORT_GET_DEVICE_INFO",
                                             s_LocalSupportGetDeviceInfo );

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LOCAL_SUPPORT_GET_NETWORK_INFO",
                                             s_LocalSupportGetNetworkInfo );

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LOCAL_SUPPORT_NETWORK_DIAGNOSTICS",
                                             s_LocalSupportNetworkDiagnostics );
  #endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */
}
#endif /* UTV_LOCAL_SUPPORT */

#ifdef UTV_LOCAL_SUPPORT_WEB_SERVER
/* s_LocalSupportGetDeviceInfo
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to retrieve the Device Information.
*/
static UTV_RESULT s_LocalSupportGetDeviceInfo( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;

  result = UtvCEMGetDeviceInfo( outputs );

  return ( result );
}
#endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */

#ifdef UTV_LOCAL_SUPPORT_WEB_SERVER
/* s_LocalSupportGetNetworkInfo
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to retrieve the Network Information.
*/
static UTV_RESULT s_LocalSupportGetNetworkInfo( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;

  result = UtvCEMGetNetworkInfo( outputs );

  return ( result );
}
#endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */

#ifdef UTV_LOCAL_SUPPORT_WEB_SERVER
/* s_LocalSupportNetworkDiagnostics
    *** PORTING STEP:  NONE. ***
    You should not have to modify this code.  Used to execute Network Diagnostics.
*/
static UTV_RESULT s_LocalSupportNetworkDiagnostics( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;

  result = UtvCEMNetworkDiagnostics( outputs );

  return ( result );
}
#endif /* UTV_LOCAL_SUPPORT_WEB_SERVER */

#ifdef UTV_LIVE_SUPPORT
/**
 **********************************************************************************************
 *
 * Platform specific Live Support initialization.
 *
 * Initialization typically includes registering handlers for supported Live Support commands.
 * Registering a handler for a command causes it to be published as "supported" to Live
 * Support client applications when they establish a Live Support session to the device, and
 * they subsequently display it as available for use to a support technician.
 *
 * The default implementation registers handlers for many ULI commands as well as some custom
 * commands.
 *
 * @intg
 *
 * A replacement implementation should register handlers for the Live Support commands
 * supported by the platform.  Live Support commands *not* supported by the platform should
 * *not* be registered.
 * 
 **********************************************************************************************
 */

void UtvCEMLiveSupportInitialize( void )
{
  /*
   * Register the command handlers for the commands that are initiated through the Live
   * Support task mechanism.
   *
   * NOTE: The second parameter for the registration of these handlers, which are UpdateLogic
   * defined commands, is UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION indicating that there is
   * not a command description.
   */

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_TV_REMOTE_BUTTON",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportProcessRemoteButton );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_DEVICE_INFO",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportGetDeviceInfo );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_CHECK_FOR_UPDATES",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportCheckForUpdates );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_UPDATE_SOFTWARE",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportUpdateSoftware );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_CHECK_PRODUCT_REGISTERED",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportCheckProductRegistered );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_PRODUCT_RETURN_PREPROCESS",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportProductReturnPreProcess );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_NETWORK_INFO",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportGetNetworkInfo );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_NETWORK_DIAGNOSTICS",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportNetworkDiagnostics );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_HARDWARE_DIAG_QUICK",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportHardwareDiagQuick );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_HARDWARE_DIAG_FULL",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportHardwareDiagFull );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_CHECK_INPUTS",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportCheckInputs );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_PARTS_LIST",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportGetPartsList );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_SYSTEM_STATISTICS",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportGetSystemStatistics );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_RESET_FACTORY_DEFAULTS",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportResetFactoryDefaults );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_SET_ASPECT_RATIO",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportSetAspectRatio );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_APPLY_VIDEO_PATTERN",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportApplyVideoPattern );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_DEVICE_CONFIG",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportGetDeviceConfig );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_SET_DEVICE_CONFIG",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportSetDeviceConfig );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_SET_DATE_TIME",
                                            UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION,
                                            s_LiveSupportSetDateTime );

  /*
   * Register the command handler for the Register Product command.
   *
   * NOTE: The registration for this command handler takes place in a separate method to allow
   *       customization of the input and output fields.
   */

  s_LiveSupportRegisterCommandHandler_RegisterProduct();

  /*
   * Register the command handlers for custom commands defined and implemented by the CEM
   * and/or Platform developers.
   *
   * NOTE: The second parameter for the registration of these handlers is a string containing
   *       a description of the command in JSON format.  This command description contains
   *       information such as a user friendly name, a description, definition of the inputs
   *       and definition of the outputs.
   */

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_CUSTOM_DIAGNOSTIC_SAMPLE",
                                            (UTV_BYTE*) pszLiveSupportCustomDiagnosticSampleDescription,
                                            s_LiveSupportCustomDiagnosticSample );

  UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_CUSTOM_TOOL_SAMPLE",
                                            (UTV_BYTE*) pszLiveSupportCustomToolSampleDescription,
                                            s_LiveSupportCustomToolSample );
}

/* UtvCEMLiveSupportShutdown
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC
   *** SHUTDOWN.
   ***
*/
void UtvCEMLiveSupportShutdown( void )
{
}

/* UtvCEMLiveSupportShutdown
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC
   *** SESSION INITIALIZATION.
   ***
*/
void UtvCEMLiveSupportSessionInitialize( void )
{
}

/* UtvCEMLiveSupportSessionShutdown
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT PERFORMS PLATFORM-SPECIFIC
   *** SESSION SHUTDOWN.
   ***
*/
void UtvCEMLiveSupportSessionShutdown( void )
{
}

/* s_LiveSupportRegisterCommandHandler_RegisterProduct
   ***
   *** PORTING STEP:
   *** REPLACE THE BODY OF THIS FUNCTION WITH CODE THAT CONSTRUCTS THE DESCRIPTION FOR
   *** THE REGISTER PRODUCT COMMAND AND THEN REGISTERS THAT COMMAND WITH LIVE SUPPORT.
   ***
   *** IF THIS DEVICE DOES NOT SUPPORT PRODUCT REGISTRATION THIS FUNCTION CAN BE REMOVED.
   ***
*/
static void s_LiveSupportRegisterCommandHandler_RegisterProduct( void )
{
  cJSON* description;
  cJSON* inputs;
  cJSON* input;
  cJSON* values;
  cJSON* outputs;
  char* pszDescription;
  UTV_INT8 pszSerialNumber[48];

  /*
  ** PORTING STEP:
  ** REPLACE THE CODE HERE WITH DEVICE SPECIFIC PROCESSING THAT CONSTRUCTS THE DESCRIPTION FOR
  ** THE REGISTER PRODUCT COMMAND.  THE DESCRIPTION PROVIDES THE COMMAND INPUTS AND OUTPUTS
  ** ALONG WITH DEFAULT VALUES.
  **
  ** SEE THE DEVICE AGENT INTEGRATION INSRUCTIONS DOCUMENT FOR ADDITIONAL INFORMATION ON THE
  ** COMMAND DESCRIPTION.
  */

  UtvCEMGetSerialNumber( pszSerialNumber, 25 );

  inputs = cJSON_CreateArray();

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "SERIAL_NUMBER" );
  cJSON_AddStringToObject( input, "NAME", "Serial Number" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Serial number of the deivce being registered." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", pszSerialNumber );
  cJSON_AddTrueToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddTrueToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "FIRST_NAME" );
  cJSON_AddStringToObject( input, "NAME", "First Name" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "First name of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddTrueToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "LAST_NAME" );
  cJSON_AddStringToObject( input, "NAME", "Last Name" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Last name of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddTrueToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "ADDRESS" );
  cJSON_AddStringToObject( input, "NAME", "Address" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Address of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "CITY" );
  cJSON_AddStringToObject( input, "NAME", "City" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "City of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  values = cJSON_CreateArray();
  cJSON_AddItemToArray( values, cJSON_CreateString( "" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "AL" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "AK" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "AS" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "AZ" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "AR" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "CA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "CO" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "CT" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "DE" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "DC" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "FM" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "FL" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "GA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "GU" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "HI" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "ID" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "IL" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "IN" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "IA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "KS" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "KY" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "LA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "ME" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MH" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MD" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MI" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MN" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MS" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MO" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MT" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NE" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NV" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NH" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NJ" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NM" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NY" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "NC" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "ND" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "MP" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "OH" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "OK" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "OR" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "PW" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "PA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "PR" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "RI" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "SC" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "SD" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "TN" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "TX" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "UT" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "VT" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "VI" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "VA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "WA" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "WV" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "WI" ) );
  cJSON_AddItemToArray( values, cJSON_CreateString( "WY" ) );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "STATE" );
  cJSON_AddStringToObject( input, "NAME", "State" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "State of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "LIST" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddItemToObject( input, "VALUES", values );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "POSTAL_CODE" );
  cJSON_AddStringToObject( input, "NAME", "Postal Code" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Postal code of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "PHONE_NUMBER" );
  cJSON_AddStringToObject( input, "NAME", "Phone Number" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Phone number of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  input = cJSON_CreateObject();
  cJSON_AddStringToObject( input, "ID", "EMAIL" );
  cJSON_AddStringToObject( input, "NAME", "Email Address" );
  cJSON_AddStringToObject( input, "DESCRIPTION", "Email address of the person registering the product." );
  cJSON_AddNumberToObject( input, "ORDER", -1 );
  cJSON_AddStringToObject( input, "TYPE", "STRING" );
  cJSON_AddStringToObject( input, "DEFAULT", "" );
  cJSON_AddFalseToObject( input, "REQUIRED" );
  cJSON_AddTrueToObject( input, "VISIBILITY" );
  cJSON_AddFalseToObject( input, "READONLY" );
  cJSON_AddItemToArray( inputs, input );

  outputs = cJSON_CreateArray();

  /*
  ** PORTING STEP:
  ** END OF THE PORTING STEP, THE ABOVE CODE BUILT UP THE INPUT AND OUTPUT OBJECTS WHICH WILL
  ** BE USED BELOW TO CREATE THE OVERALL DESCRIPTION.
  */

  /*
  ** Build up the overall description object which will include the inputs and outputs created
  ** by the above code.
  */

  description = cJSON_CreateObject();
  cJSON_AddStringToObject( description, "TYPE", "TOOL_COMMANDS" );
  cJSON_AddStringToObject( description, "ID", "FFFFFF_LIVE_SUPPORT_REGISTER_PRODUCT" );
  cJSON_AddStringToObject( description, "NAME", "Register Product" );
  cJSON_AddStringToObject( description, "DESCRIPTION", "Register a product." );
  cJSON_AddNumberToObject( description, "ORDER", -1 );
  cJSON_AddStringToObject( description, "PERMISSIONS", "" );
  cJSON_AddItemToObject( description, "INPUTS", inputs );
  cJSON_AddItemToObject( description, "OUTPUTS", outputs );

  /*
  ** The command description has now been built, convert it to the string representation
  ** which is used for the registration function.
  */

  pszDescription = cJSON_PrintUnformatted( description );

  /*
  ** Register the command handler with the given description.
  */

  UtvLiveSupportTaskRegisterCommandHandler(
    (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_REGISTER_PRODUCT", 
    (UTV_BYTE*) pszDescription,
    s_LiveSupportRegisterProduct );

  /*
  ** Free up the temporary variables now that the registration is complete.
  */

  free( pszDescription );
  cJSON_Delete( description );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to provide one or more TV Remote Buttons
 * to be processed by the device.
 *
 * @intg
 *
 * Example implementation loops through the TV Remote Button inputs.  Replace the processing
 * of the individual button within the loop to perform device specific conversion and
 * processing of the button.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the inputs consist of a single item, "BUTTONS".  The "BUTTONS" item
 *    contains an array of button values to process where the button values can be one of
 *    the defines with the "UTV_REMOTE_BUTTON_" prefix found in utv-core.h.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportProcessRemoteButton( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;

  result = UtvCEMRemoteButton( inputs );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to retrieve device specific information
 * from the device.
 *
 * @intg
 *
 * Example implementation gathers device information and adds the results to the output
 * data.  Replace the body of this command handler function with a device specific
 * implementation that retrieves the device information and adds the results to the output
 * data.  The example implementation contains the expected fields; additional device specific
 * fields can be added.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object the output for this command handler.  For this command
 *    there is a single output which consists of device specific information.  In this
 *    example the output contains a set of values and timestamps for the device.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"OUI" : "",
 *                 "MODEL_GROUP" : "A12345",
 *                 "SERIAL_NUMBER" : "B123",
 *                 "CURRENT_DATE" : "YYYY-MM-DDTHH:MM:SSZ",
 *                 "MANUFACTURED_DATE" : "YYYY-MM-DDTHH:MM:SSZ",
 *                 "INITIAL_POWER_ON_DATE" : "YYYY-MM-DDTHH:MM:SSZ",
 *                 "UPTIME_SECONDS" : 12345}}
 *
 *      NOTE: The date formats used below are ISO 8601 compliant that specify a UTC time.
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetDeviceInfo( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;
  UTV_INT8 pszOUI[7];
  UTV_INT8 pszModelGroup[5];
  UTV_INT8 pszSerialNumber[48];
  UTV_TIME tCurrentTime;
  struct tm* ptmTime;
  char pszCurrentTime[21];
  UTV_TIME tUpTime = 0;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvMemFormat( (UTV_BYTE*)pszOUI, 7, "%06X", UtvCEMGetPlatformOUI() );
  UtvMemFormat( (UTV_BYTE*)pszModelGroup, 5, "%04X", UtvCEMGetPlatformModelGroup() );

  UtvCEMGetSerialNumber( pszSerialNumber, 25 );

  tCurrentTime = UtvTime( NULL );
  ptmTime = UtvGmtime( &tCurrentTime );
  strftime( pszCurrentTime, 21, "%Y-%m-%dT%H:%M:%SZ", ptmTime );

  tUpTime = UtvUpTime();

  responseData = cJSON_CreateObject();
  cJSON_AddStringToObject( responseData, "OUI", pszOUI );
  cJSON_AddStringToObject( responseData, "MODEL_GROUP", pszModelGroup );
  cJSON_AddStringToObject( responseData, "SERIAL_NUMBER", pszSerialNumber );
  cJSON_AddStringToObject( responseData, "CURRENT_DATE", pszCurrentTime );
  cJSON_AddStringToObject( responseData, "MANUFACTURED_DATE", "2009-01-01T13:45:03Z" );
  cJSON_AddStringToObject( responseData, "INITIAL_POWER_ON_DATE", "2010-05-28T08:23:05Z" );
  cJSON_AddNumberToObject( responseData, "UPTIME_SECONDS", (UTV_UINT32) tUpTime );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to determine if there are any pending
 * software updates available.
 *
 * @intg
 *
 * Example implementation utilizes the UpdateLogic UpdateTV functionality to determine if
 * software updates are available.  Replace the body of this command handler function with a
 * device specific implementation if the device does not utilize the UpdateLogic UpdateTV
 * functionality.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information.  In this
 *    example the output contains information about the state of software updates.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"SOFTWARE_CURRENT_FLAG" : true}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportCheckForUpdates( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  UtvInternetUpdateQueryResponse* putvUQR = NULL;
  UTV_BOOL bUpdatesAvailable = UTV_FALSE;
  cJSON* output;
  cJSON* responseData;
  cJSON* softwareCurrent;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
  ** PORTING STEP:
  ** IF THIS DEVICE DOES NOT UTILIZE THE UPDATELOGIC UPDATETV FUNCTIONALITY THEN REPLACE THE
  ** CODE HERE WITH DEVICE SPECIFIC PROCESSING THAT DETERMINES IF SOFTWARE UPDATES ARE
  ** AVAILABLE.  SET THE "bUpdatesAvailable" FLAG TO TRUE IF SOFTWARE UPDATES ARE AVAIALBLE.
  ** SET THE FLAG TO FALSE IF SOFTWARE UPDATES ARE NOT AVAIALBLE.
  **
  ** IF THE DEVICE IS UNABLE TO DETERMINE IF SOFTWARE UPDATES ARE AVAILABLE SET THE "result"
  ** VALUE TO AN APPROPRIATE ERROR CODE.
  */

  UtvProjectOnForceNOCContact();

  result = UtvInternetCheckForUpdates( &putvUQR );

  if ( UTV_OK == result || UTV_NO_DOWNLOAD == result )
  {
    /*
     * Both results are acceptable at this point so make sure the result reflects it.
     */

    result = UTV_OK;

    bUpdatesAvailable = ( putvUQR->uiNumEntries > 0 );
  }

  /*
   * Now that the flag indicating if software updates are available has been set add it to the
   * response data.
   */

  responseData = cJSON_CreateObject();
  softwareCurrent = cJSON_CreateBool( !bUpdatesAvailable );
  cJSON_AddItemToObject( responseData, "SOFTWARE_CURRENT_FLAG", softwareCurrent );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to initiate a software update.
 *
 * @intg
 *
 * Example implementation initiates a software update utilizing the UpdateLogic UpdateTV
 * functionality.  If the device does not utilize the UpdateLogic UpdateTV functionality then
 * replace the code with device specific processing that initites the software update
 * processing.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information.  In this
 *    example the output contains information about the state of software update processing.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"STATUS" : "UNKNOWN" | "NO_UPDATE_REQUIRED" | "INITIATED" | "COMPLETED"}}
 *
 *      Status value definitions:
 *        UNKNOWN - Unable to determine the state of the download.
 *        NO_UPDATE_REQUIRED - There is not a software update available.
 *        INITIATED - The software update process has been initiated.  The device will pause
 *                    and resume the session.
 *        COMPLETED - The software update processing has completed.
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 * @note
 *    The software update processing must occur in a separate thread to allow the device to
 *    send the response for this comand back to the NetReady Services.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportUpdateSoftware( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output = NULL;
  cJSON* responseData = NULL;
  UtvInternetUpdateQueryResponse* putvUQR = NULL;
  char* status = "UNKNOWN";

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
  ** PORTING STEP:
  ** IF THIS DEVICE DOES NOT UTILIZE THE UPDATELOGIC UPDATETV FUNCTIONALITY THEN REPLACE THE
  ** CODE HERE WITH DEVICE SPECIFIC PROCESSING THAT INITIATES THE SOFTWARE UPDATE PROCESSING.
  **
  ** NOTE: THIS PROCESSING MUST BE DONE IN A SEPARATE THREAD TO ALLOW THE DEVICE TO SEND THE
  **       RESPONSE FOR THIS COMMAND BACK TO THE NOC.
  **
  ** IF THE DEVICE IS UNABLE TO INITIATE THE SOFTWARE UPDATE PROCESSING SET THE "result" VALUE
  ** TO AN APPROPRIATE ERROR CODE.
  */

  UtvProjectOnForceNOCContact();

  result = UtvInternetCheckForUpdates( &putvUQR );

  if ( UTV_OK == result || UTV_NO_DOWNLOAD == result )
  {
    /*
     * Both results are acceptable at this point so make sure the result reflects it.
     */

    result = UTV_OK;

    if ( putvUQR->uiNumEntries > 0 )
    {
      result = UtvProjectOnScanForNetUpdate( NULL );

      if ( UTV_OK == result )
      {
        status = "INITIATED";
      }
    }
    else
    {
      status = "NO_UPDATE_REQUIRED";
    }
  }

  responseData = cJSON_CreateObject();
  cJSON_AddStringToObject( responseData, "STATUS", status );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to check if the product has already been
 * registered.
 *
 * @intg
 *
 * Example implementation utilizes a static variable to indicate its product registration
 * status.  The static variable is updated during a successful product registration using the
 * s_LiveSupportRegisterProduct function.  Replace the body of this command handler function
 * with a device specific implementation that utilizes persistent storage to determine the
 * product registration status and return that information in the output of this command
 * handler.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information.  In this
 *    example the output contains information about the state of the product registration.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"PRODUCT_REGISTERED_FLAG" : true}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportCheckProductRegistered( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  UTV_BOOL bProductRegistered = UTV_FALSE;
  cJSON* output;
  cJSON* responseData;
  cJSON* productRegistered;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs ||
       cJSON_Array != outputs->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** THE RESPONSE INFORMATION CONTAINS DATA INDICATING WHETHER THE DEIVCE HAS BEEN REGISTERED
  ** OR NOT.  SET THE "bProductRegistered" FLAG TO TRUE IF THE PRODUCT HAS BEEN REGISTERED.
  ** SET THE FLAG TO FALSE IF THE PRODUCT HAS NOT BEEN REGISTERED.
  **
  ** THE SAMPLE IMPLEMENTATION USES A STATIC VARIABLE TO DETERMINE THE REGISTRATION STATE.
  ** THE FLAG IS INITIALIZED TO UTV_FALSE AND UPON SUCCESSFUL REGISTRATION IT WILL BE SET TO
  ** UTV_TRUE.  THE FLAG WILL BE RESET EACH TIME THE SUPERVISORY APPLICATION IS RESTARTED.
  **
  ** IF THE DEVICE IS UNABLE TO DETERMINE THE REGISTRATION STATE SET THE "result" VALUE TO AN
  ** APPROPRIATE ERROR CODE.
  **
  ** IF THE DEVICE DOES NOT SUPPORT THE NOTION OF PRODUCT REGISTRATION THEN THIS COMMAND
  ** HANDLER SHOULD BE REMOVED AND IT SHOULD NOT BE REGISTERED AS ONE OF THE SUPPORTED
  ** COMMANDS.
  */

  bProductRegistered = s_bProductRegistered;

  /*
   * Now that the flag indicating if software updates are available has been set add it to the
   * response data.
   */

  responseData = cJSON_CreateObject();
  productRegistered = cJSON_CreateBool( bProductRegistered );
  cJSON_AddItemToObject( responseData, "PRODUCT_REGISTERED_FLAG", productRegistered );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to process a product registration that
 * is initiated using the SupportTV commands.
 *
 * @intg
 *
 * Example implementation generates the string representation of the input JSON information
 * and uses it as the registration information.  Replace the body of this command handler
 * function with a device specific implementation that processes the input information and
 * generates the desired registration information.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of the fields identified in the command description used
 *    when the command handler was registered.  The fields are defined by the CEM; the sample
 *    implementation is constructed with the following input fields.
 *    @verbatim
 *      {"SERIAL_NUMBER" : "AP001",
 *       "FIRST_NAME"    : "John",
 *       "LAST_NAME"     : "Smith",
 *       "ADDRESS"       : "123 Oak Street",
 *       "CITY"          : "New York",
 *       "STATE"         : "NY",
 *       "POSTAL_CODE"   : "10001",
 *       "PHONE_NUMBER"  : "555-555-55555",
 *       "EMAIL"         : "john@smith.com"}
 *    @endverbatim
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportRegisterProduct( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;
  char* registrationInfo;
  UtvStoreDeviceData_MetaData_t metaData;

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    UtvMessage( UTV_LEVEL_INFO, "%s inputs is NULL",  __FUNCTION__);
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** PROVIDE DEVICE SPECIFIC PROCESSING TO CONVERT THE DATA AVAILABLE IN THE INPUTS INTO
  ** THE APPROPRIATE DEVICE DATA TO STORE WITH THE NETREADY SERVICE.  THE SAMPLE
  ** IMPLEMENTATION CONVERTS THE INPUTS INTO THE STRING REPRESENTATION OF THE JSON DATA
  ** AND USES THAT AS THE DEVICE DATA TO STORE.
  */

  registrationInfo = cJSON_PrintUnformatted( inputs );
  if ( NULL == registrationInfo )
  {
    UtvMessage( UTV_LEVEL_INFO, "%s cJSON_PrintUnformatted failed",  __FUNCTION__);
    return ( UTV_NO_MEMORY );
  }

  /* the streamID field can be ignored */
  UtvMemset( &metaData, 0x00, sizeof( metaData ) );
  metaData.version = UTV_METADATA_VERSION_1;
  metaData.type = ProductRegistration;
  metaData.encryptOnHost = UTV_TRUE;
  metaData.compressOnHost = UTV_TRUE;
  metaData.createTime = UtvTime( NULL );

  result = UtvInternetStoreDeviceData_BlockSend(&metaData, 
                                                (UTV_BYTE*) registrationInfo,
                                                strlen( registrationInfo ) );
  free( registrationInfo );

  if ( UTV_OK == result )
  {
    /*
    ** PORTING STEP:
    ** THE DATA HAS SUCCESSFULLY BEEN STORED WITH THE NETREADY SERVICE, UPDATE THE DEVICE
    ** REGISTRATION STATE SO FUTURE REQUESTS FOR THE PRODUCT REGISTRATION STATE WILL INDICATE
    ** THE CORRECT STATUS.
    **
    ** THE SAMPLE IMPLEMENTATION USES A STATIC VARIABLE TO DETERMINE THE REGISTRATION STATE.
    ** THE FLAG IS INITIALIZED TO UTV_FALSE AND UPON SUCCESSFUL REGISTRATION IT WILL BE SET TO
    ** UTV_TRUE.  THE FLAG WILL BE RESET EACH TIME THE SUPERVISORY APPLICATION IS RESTARTED.
    */

    s_bProductRegistered = UTV_TRUE;
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to allow device specific processing
 * required prior to running the return processing for the device.
 *
 * @intg
 *
 * Example implementation does nothing.  Replace the body of this command handler function
 * with a device specific implementation for processing required prior to running the return
 * processing for the device..
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportProductReturnPreProcess( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;

  /*
  ** PORTING STEP:
  ** PROVIDE DEVICE SPECIFIC PROCESSING THAT NEEDS TO HAPPEN PRIOR TO THE PRODUCT RETURN
  ** PROCESSING.  THE SAMPLE IMPLEMENTATION HAS NO ADDITIONAL PROCESSING.
  */

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to retrieve network information from the
 * device.
 *
 * @intg
 *
 * Example implementation gathers network information and adds the following values to the
 * cJSON output object using UtvPlatformGetNetworkInfo.  Replace the body of this command
 * handler function with a device specific implementation that retrieves the network
 * information and adds the results to the output data.  The example imple-mentation contains
 * the expected fields; additional device specific fields can be added.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific network information.
 *    In this example the output contains the interfaces and routing information available on
 *    a Linux platform.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {{"Interfaces" : [{"NAME_1" : {"Type" : "Wired/Wireless",
 *                                               "Status" : "Disabled/Connected/Disconnected",
 *                                               "Network Name" : "MyNet",
 *                                               "Security Key" : "theFamilyNet!",
 *                                               "DHCP" : "Enabled/Disabled",
 *                                               "IP Address" : "192.169.10.100",
 *                                               "Gateway" : "192.168.2.1",
 *                                               "Subnet Mask" : "255.255.255.0",
 *                                               "Physical Address" : "AB:CD:EF:01:23:45",
 *                                               "First DNS Server" : "192.168.2.1",
 *                                               "Second DNS Server" : "68.87.72.134",
 *                                               "Third DNS Server" : "68.87.72.135",
 *                                               "Broadcast Address" : "0.0.0.0",
 *                                               "Metrics" : {"Received Bytes" : 1234,
 *                                                            "Recieved Packets" : 1234,
 *                                                            "Received Compressed Packets" : 1234,
 *                                                            "Received Multicast Packets" : 1234,
 *                                                            "Received Errors" : 1234,
 *                                                            "Received Packets Dropped" : 1234,
 *                                                            "Received FIFO Errors" : 1234,
 *                                                            "Received Frame Errors" : 1234,
 *                                                            "Transmitted Bytes" : 1234,
 *                                                            "Transmitted Packets" : 1234,
 *                                                            "Transmitted Errors" : 1234,
 *                                                            "Transmitted Packets Dropped" : 1234,
 *                                                            "Transmitted FIFO Errors" : 1234,
 *                                                            "Transmitted Collisions" : 1234,
 *                                                            "Transmitted Carrier Errors" : 1234}}},
 *                                  {"NAME_2" : { SAME AS ABOVE }}]},
 *                 {"Routes" : [{"Route to IP_ADDRESS_1" : {"Gateway" : "192.168.2.1",
 *                                                          "Subnet Mask" : "255.255.255.255",
 *                                                          "Metric" : 1234,
 *                                                          "Reference Count" : 1234,
 *                                                          "Use" : 1234,
 *                                                          "Interface" : 1234},
 *                              {"Route to IP_ADDRESS_1" : { SAME AS ABOVE }}]}}
 *
 *      NOTES:
 *        - The default implementation of this implementation only retrieves network
 *          information for active/enabled network interfaces.  If the device or platform
 *          supports gathering additional information please adjust the implementation
 *          accordingly.
 *        - The default implementation of this implementation is not able to retrieve all of
 *          the desired information.  Missing information includes:
 *            - Type
 *            - Status
 *            - Network Name
 *            - Security Key
 *            - DHCP
 *            - First DNS Server
 *            - Second DNS Server
 *            - Third DNS Server
 *          If the device or platform has access to this information please adjust the
 *          implementation accordingly.
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetNetworkInfo( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  responseData = cJSON_CreateObject();

  if ( NULL == responseData )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvCEMGetNetworkInfo( responseData );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to execute a set of network diagnostic
 * tests on the device.
 *
 * @intg
 *
 * Example implementation utilizes the common UtvCEMNetworkDiagnostics function to execute
 * the diagnostic tests.  The common routine is utilized because there are multiple external
 * entry points that can initiate the network diagnostic functionality.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of the results of device specific
 *    network diagnostic tests.  In this example the output contains example output from
 *    expected network diagnostic tests.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"Local Ping" : "Pass/Fail",
 *                 "Internet Ping" : "Pass/Fail",
 *                 "DNS Lookup" : "Pass/Fail",
 *                 "Speed (Up)" : "1.2",
 *                 "Speed (Down)" : "8.5",
 *                 "Latency" : "550",
 *                 "Wireless Signal" : "-42",
 *                 "Wireless Speed" : "54"}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 * @sa      UtvCEMNetworkDiagnostics
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportNetworkDiagnostics( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  responseData = cJSON_CreateObject();

  if ( NULL == responseData )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  result = UtvCEMNetworkDiagnostics( responseData );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to request the device execute a quick
 * hardware diagnostic test and return the results.
 *
 * @intg
 *
 * Example implementation returns hard-coded sample data in JSON format. Replace the body of
 * this command handler function with a device specific implementation that executes a quick
 * hardware diagnostic check and adds the results to the output data.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information resulting
 *    from the quick hardware diagnostics run on the device.  In this example the output
 *    contains a set of diagnostic areas and results from within those areas.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"Summary" : "Pass" | "Fail" | "Warning",
 *                 "Hardware Detect" : {"Panel Driver" : "Found",
 *                                      "RTC" : "Found",
 *                                      "Flash" : "Found",
 *                                      "ROM" : "Found",
 *                                      "UART" : "Found",
 *                                      "802.3" : "Found",
 *                                      "802.11" : "Found",
 *                                      "I2C" : "Found"},
 *                 "Hardware" : {"CPU" : "Passed",
 *                               "Controller 0" : "Passed",
 *                               "Controller 1" : "Passed",
 *                               "Timer 1" : "Passed",
 *                               "Timer 2" : "Passed",
 *                               "Flash" : "Passed"},
 *                 "Memory" : {"Stress Test" : "Passed",
 *                             "Ground Bounce" : "Passed",
 *                             "Walking 1s/0s" : "Passed",
 *                             "March B" : "Passed"},
 *                 "File System" : {"Broken Links" : 0,
 *                                  "Corrupt iNodes" : 0,
 *                                  "Bad Clusters" : 0}}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportHardwareDiagQuick( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;
  cJSON* hardwareDiag;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  responseData = cJSON_CreateObject();

  if ( NULL == responseData )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  cJSON_AddStringToObject( responseData, "Summary", "Pass" );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "Panel Driver", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "RTC", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "Flash", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "ROM", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "UART", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "802.3", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "802.11", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "I2C", "Found" );
  cJSON_AddItemToObject( responseData, "Hardware Detect", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "CPU", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Controller 0", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Controller 1", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Timer 1", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Timer 2", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Flash", "Passed" );
  cJSON_AddItemToObject( responseData, "Hardware", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "Stress Test", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Ground Bounce", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Walking 1s/0s", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "March B", "Passed" );
  cJSON_AddItemToObject( responseData, "Memory", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddNumberToObject( hardwareDiag, "Broken Links", 0 );
  cJSON_AddNumberToObject( hardwareDiag, "Corrupt iNodes", 0 );
  cJSON_AddNumberToObject( hardwareDiag, "Bad Clusters", 0 );
  cJSON_AddItemToObject( responseData, "File System", hardwareDiag );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to request the device execute a full
 * hardware diagnostic test and return the results.
 *
 * @intg
 *
 * Example implementation returns hard-coded sample data in JSON format.  Replace the body of
 * this command handler function with a device specific implementation that executes a full
 * hardware diagnostic check and adds the results to the output data.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"Summary" : "Pass" | "Fail" | "Warning",
 *                 "Hardware Detect" : {"Panel Driver" : "Found",
 *                                      "RTC" : "Found",
 *                                      "Flash" : "Found",
 *                                      "ROM" : "Found",
 *                                      "UART" : "Found",
 *                                      "802.3" : "Found",
 *                                      "802.11" : "Found",
 *                                      "I2C" : "Found"},
 *                 "Hardware" : {"CPU Load" : "Passed",
 *                               "CPU Temperature" : "Passed",
 *                               "Controller 0" : "Passed",
 *                               "Controller 1" : "Passed",
 *                               "Timer 1" : "Passed",
 *                               "Timer 2" : "Passed",
 *                               "Flash Sequential Read" : "Passed",
 *                               "Flash Sequential Write" : "Passed",
 *                               "Flash Random Read" : "Passed",
 *                               "Flash Random Write" : "Passed"},
 *                 "Memory" : {"Zero-One", "Passed",
 *                             "Checkerboard", "Passed",
 *                             "Sliding Diagonal", "Passed",
 *                             "Stress Test" : "Passed",
 *                             "Ground Bounce" : "Passed",
 *                             "Walking 1s/0s" : "Passed",
 *                             "March A" : "Passed",
 *                             "March B" : "Passed",
 *                             "March Y" : "Passed"},
 *                 "File System" : {"Broken Links" : 0,
 *                                  "Corrupt iNodes" : 0,
 *                                  "Bad Clusters" : 0}}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportHardwareDiagFull( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;
  cJSON* hardwareDiag;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  responseData = cJSON_CreateObject();

  if ( NULL == responseData )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  cJSON_AddStringToObject( responseData, "Summary", "Pass" );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "Panel Driver", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "RTC", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "Flash", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "ROM", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "UART", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "802.3", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "802.11", "Found" );
  cJSON_AddStringToObject( hardwareDiag, "I2C", "Found" );
  cJSON_AddItemToObject( responseData, "Hardware Detect", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "CPU Load", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "CPU Temperature", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Controller 0", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Controller 1", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Timer 1", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Timer 2", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Flash Sequential Read", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Flash Sequential Write", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Flash Random Read", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Flash Random Write", "Passed" );
  cJSON_AddItemToObject( responseData, "Hardware", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddStringToObject( hardwareDiag, "Zero-One", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Checkerboard", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Sliding Diagonal", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Stress Test", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Ground Bounce", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "Walking 1s/0s", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "March A", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "March B", "Passed" );
  cJSON_AddStringToObject( hardwareDiag, "March Y", "Passed" );
  cJSON_AddItemToObject( responseData, "Memory", hardwareDiag );

  hardwareDiag = cJSON_CreateObject();
  cJSON_AddNumberToObject( hardwareDiag, "Broken Links", 0 );
  cJSON_AddNumberToObject( hardwareDiag, "Corrupt iNodes", 0 );
  cJSON_AddNumberToObject( hardwareDiag, "Bad Clusters", 0 );
  cJSON_AddItemToObject( responseData, "File System", hardwareDiag );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to check the status of the device input
 * ports (i.e. TV, AVI, HDMI, RGB, etc.).
 *
 * @intg
 *
 * Example implementation returns hard-coded sample data in JSON format.  Replace the body of
 * this command handler function with a device specific implementation that checks the status
 * of the device input ports and adds the results to the output data.  The results should
 * include a list of the inputs available and a true/false indicator for their connected
 * status.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information resulting
 *    from the input check on the device.  In this example the output contains the set of
 *    inputs for the device with a true/false value representing their connected state.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : [{"TV" :     {"Connected Status" : false,
 *                              "Type" : "Coaxial",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}},
 *                 {"AV-1" :   {"Connected Status" : true,
 *                              "Type" : "Component",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}},
 *                 {"AV-2" :   {"Connected Status" : null,
 *                              "Type" : "Composite",
 *                              "Location" : "Left Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}},
 *                 {"AV-3" :   {"Connected Status" : null,
 *                              "Type" : "S-Video",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}},
 *                 {"HDMI-1" : {"Connected Status" : true,
 *                              "Type" : "HDMI",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Interface Attributes" : [{"EDID" : {"Name" : "EPI EnVision EN-775e",
 *                                                                   "ID" : "EPID775",
 *                                                                   "Model" : "EN-775e",
 *                                                                   "Manufacture Date" : "Week 26 / 2002",
 *                                                                   "Serial Number" : "1226764172",
 *                                                                   "Max. Visible Display Size" : "32 cm x 24 cm (15.7 in)",
 *                                                                   "Picture Aspect Ratio" : "4:3",
 *                                                                   "Horizontal Frequency" : "30 - 72 kHz",
 *                                                                   "Vertical Frequency" : "50 - 160 Hz",
 *                                                                   "Maximum Resolution" : "1280 x 1024",
 *                                                                   "Gamma" : "2.20",
 *                                                                   "DPMS Mode Support" : "Active-Off",
 *                                                                   "Supported Video Modes" : ["640 x 480 140 Hz",
 *                                                                                              "800 x 600 110 Hz",
 *                                                                                              "1024 x 768 85 Hz",
 *                                                                                              "1152 x 864 75 Hz",
 *                                                                                              "1280 x 1024 65 Hz"],
 *                                                                   "Manufacturer Company" : "Envision, Inc."},
 *                                                         "CEC Physical Address" : "0.0.0.0",
 *                                                         "CEC Logical Addresses" : [{"Address" : 0,
 *                                                                                     "Description" : "TV"}],
 *                              "Device Attributes" :    [{"EDID" : NULL,
 *                                                         "CEC Physical Address" : "1.0.0.0",
 *                                                         "CEC Logical Addresses" : [{"Address" : 4,
 *                                                                                     "Name" : "DVD",
 *                                                                                     "Description" : "Playback Device 1"}]}},
 *                 {"HDMI-2" : {"Connected Status" : false,
 *                              "Type" : "HDMI",
 *                              "Location" : "Left Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Interface Attributes" : [{"EDID" : {"Name" : "EPI EnVision EN-775e",
 *                                                                   "ID" : "EPID775",
 *                                                                   "Model" : "EN-775e",
 *                                                                   "Manufacture Date" : "Week 26 / 2002",
 *                                                                   "Serial Number" : "1226764172",
 *                                                                   "Max. Visible Display Size" : "32 cm x 24 cm (15.7 in)",
 *                                                                   "Picture Aspect Ratio" : "4:3",
 *                                                                   "Horizontal Frequency" : "30 - 72 kHz",
 *                                                                   "Vertical Frequency" : "50 - 160 Hz",
 *                                                                   "Maximum Resolution" : "1280 x 1024",
 *                                                                   "Gamma" : "2.20",
 *                                                                   "DPMS Mode Support" : "Active-Off",
 *                                                                   "Supported Video Modes" : ["640 x 480 140 Hz",
 *                                                                                              "800 x 600 110 Hz",
 *                                                                                              "1024 x 768 85 Hz",
 *                                                                                              "1152 x 864 75 Hz",
 *                                                                                              "1280 x 1024 65 Hz"],
 *                                                                   "Manufacturer Company" : "Envision, Inc."},
 *                                                         "CEC Physical Address" : "0.0.0.0",
 *                                                         "CEC Logical Addresses" : [{"Address" : 0,
 *                                                                                     "Description" : "TV"}],
 *                              "Device Attributes" :    [{"EDID" : NULL,
 *                                                         "CEC Physical Address" : "2.0.0.0",
 *                                                         "CEC Logical Addresses" : [{"Address" : 8,
 *                                                                                     "Name" : "DVD",
 *                                                                                     "Description" : "Playback Device 2"}]}},
 *                 {"PC-1" :   {"Connected Status" : false,
 *                              "Type" : "DVI",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}},
 *                 {"PC-2" :   {"Connected Status" : null,
 *                              "Type" : "RGB",
 *                              "Location" : "Back Panel",
 *                              "Notes" : "Sample Notes",
 *                              "Device Attributes" : NULL,
 *                              "Interface Attributes" : NULL}}]}
 *
 *      Each input has the following fields:
 *        Connected Status - true, false or null
 *        Type - Unknown, Coaxial, Component, Composite, S-Video, HDMI, DVI, RGB
 *        Location - Optional field containing integrator string describing the location of the input
 *        Notes - Optional field containing integrator string with additional information for the input
 *        Interface Attributes - Optional field containing information gathered for the input interface
 *        Device Attributes - Optional field containing information gathered for an attached device
 *        
 *        The integrator has the ability to also add an arbitrary list of key/value pairs to the existing
 *        set of fields.
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportCheckInputs( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseItems;
  cJSON* responseItem;
  cJSON* responseItemAttrs;
  cJSON* attrs;
  cJSON* attr;
  cJSON* edid;
  cJSON* videoModes;
  char* szEdid;
  cJSON* cecLogicals;
  cJSON* cecLogical;
  UTV_INT8 pszSerialNumber[48];

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** CHECK THE INPUTS AND REPORT BACK TRUE/FALSE FOR THE CONNECTED STATUS, SOME NOTES
  ** REGARDING THE INPUT AND ANY ADDITIONAL INFORMATION BY CALLING THE APPROPRIATE DEVICE
  ** SPECIFIC PROCESSING ROUTINES.
  */

  UtvMessage( UTV_LEVEL_INFO,
              "%s - Stubbed Implementation checking inputs",
              __FUNCTION__ );

  UtvCEMGetSerialNumber( pszSerialNumber, 25 );

  //
  // Create the EDID information, save it in string format so it can be parsed to create
  // objects for each input in which it is needed.
  //

  videoModes = cJSON_CreateArray();
  cJSON_AddItemToArray( videoModes, cJSON_CreateString( "640 x 480 140 Hz" ) );
  cJSON_AddItemToArray( videoModes, cJSON_CreateString( "800 x 600 110 Hz" ) );
  cJSON_AddItemToArray( videoModes, cJSON_CreateString( "1024 x 768 85 Hz" ) );
  cJSON_AddItemToArray( videoModes, cJSON_CreateString( "1152 x 864 75 Hz" ) );
  cJSON_AddItemToArray( videoModes, cJSON_CreateString( "1280 x 1024 65 Hz" ) );

  edid = cJSON_CreateObject();
  cJSON_AddStringToObject( edid, "Name", "ACME A-001" );
  cJSON_AddStringToObject( edid, "ID", "A-001" );
  cJSON_AddStringToObject( edid, "Model", "A-001" );
  cJSON_AddStringToObject( edid, "Manufacture Date", "Week 6 / 2011" );
  cJSON_AddStringToObject( edid, "Serial Number", pszSerialNumber );
  cJSON_AddStringToObject( edid, "Max. Visible Display Size", "32 cm x 24 cm" );
  cJSON_AddStringToObject( edid, "Picture Aspect Ratio", "4:3" );
  cJSON_AddStringToObject( edid, "Horizontal Frequency", "30 - 72 kHz" );
  cJSON_AddStringToObject( edid, "Vertical Frequency", "50 - 160 Hz" );
  cJSON_AddStringToObject( edid, "Maximum Resolution", "1280 x 1024" );
  cJSON_AddStringToObject( edid, "Gamma", "2.20" );
  cJSON_AddStringToObject( edid, "DPMS Mode Support", "Active-Off" );
  cJSON_AddItemToObject( edid, "Supported Video Modes", videoModes );
  cJSON_AddStringToObject( edid, "Manufacturer Company", "Acme, Inc." );

  szEdid = cJSON_PrintUnformatted( edid );
  cJSON_Delete( edid );

  //
  // Create the array to hold the information for each of the inputs.
  //

  responseItems = cJSON_CreateArray();

  //
  // Add the TV Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddFalseToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "Coaxial" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding the TV Coaxial input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "TV", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the AV-1 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddTrueToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "Component" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding AV-1 Component input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "AV-1", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the AV-2 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();

  cJSON_AddNullToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "Composite" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Left Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding AV-2 Composite input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "AV-2", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the AV-3 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddNullToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "S-Video" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding AV-3 S-Video input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "AV-3", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the HDMI-1 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();

  cJSON_AddTrueToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "HDMI" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding HDMI-1 input." );

  // Create and add the interface attributes

  cecLogicals = cJSON_CreateArray();
  cecLogical = cJSON_CreateObject();
  cJSON_AddNumberToObject( cecLogical, "Address", 0 );
  cJSON_AddStringToObject( cecLogical, "Description", "TV" );
  cJSON_AddItemToArray( cecLogicals, cecLogical );

  attrs = cJSON_CreateArray();
  attr = cJSON_CreateObject();
  cJSON_AddItemToObject( attr, "EDID", cJSON_Parse( szEdid ) );
  cJSON_AddStringToObject( attr, "CEC Physical Address", "0.0.0.0" );
  cJSON_AddItemToObject( attr, "CEC Logical Addresses", cecLogicals );
  cJSON_AddItemToArray( attrs, attr );

  cJSON_AddItemToObject( responseItemAttrs, "Interface Attributes", attrs );

  // Create and add the device attributes

  cecLogicals = cJSON_CreateArray();
  cecLogical = cJSON_CreateObject();
  cJSON_AddNumberToObject( cecLogical, "Address", 4 );
  cJSON_AddStringToObject( cecLogical, "Name", "DVD" );
  cJSON_AddStringToObject( cecLogical, "Description", "Playback Device 1" );
  cJSON_AddItemToArray( cecLogicals, cecLogical );

  attrs = cJSON_CreateArray();
  attr = cJSON_CreateObject();
  cJSON_AddNullToObject( attr, "EDID" );
  cJSON_AddStringToObject( attr, "CEC Physical Address", "1.0.0.0" );
  cJSON_AddItemToObject( attr, "CEC Logical Addresses", cecLogicals );
  cJSON_AddItemToArray( attrs, attr );

  cJSON_AddItemToObject( responseItemAttrs, "Device Attributes", attrs );

  cJSON_AddItemToObject( responseItem, "HDMI-1", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the HDMI-2 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();

  cJSON_AddFalseToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "HDMI" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Left Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding HDMI-2 input." );

  // Create and add the interface attributes

  cecLogicals = cJSON_CreateArray();
  cecLogical = cJSON_CreateObject();
  cJSON_AddNumberToObject( cecLogical, "Address", 0 );
  cJSON_AddStringToObject( cecLogical, "Description", "TV" );
  cJSON_AddItemToArray( cecLogicals, cecLogical );

  attrs = cJSON_CreateArray();
  attr = cJSON_CreateObject();
  cJSON_AddItemToObject( attr, "EDID", cJSON_Parse( szEdid ) );
  cJSON_AddStringToObject( attr, "CEC Physical Address", "0.0.0.0" );
  cJSON_AddItemToObject( attr, "CEC Logical Addresses", cecLogicals );
  cJSON_AddItemToArray( attrs, attr );

  cJSON_AddItemToObject( responseItemAttrs, "Interface Attributes", attrs );

  // Create and add the device attributes (NOT CONNECTED, NULL ADDED)

  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );

  cJSON_AddItemToObject( responseItem, "HDMI-2", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the PC-1 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddFalseToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "DVI" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding PC-1 DVI input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "PC-1", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Add the PC-2 Input
  //

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddFalseToObject( responseItemAttrs, "Connected Status" );
  cJSON_AddStringToObject( responseItemAttrs, "Type", "RGB" );
  cJSON_AddStringToObject( responseItemAttrs, "Location", "Back Panel" );
  cJSON_AddStringToObject( responseItemAttrs, "Notes", "Additional notes regarding PC-2 RGB input." );
  cJSON_AddNullToObject( responseItemAttrs, "Interface Attributes" );
  cJSON_AddNullToObject( responseItemAttrs, "Device Attributes" );
  cJSON_AddItemToObject( responseItem, "PC-2", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  //
  // Construct the outputs that contain the response items.
  //

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseItems );

  cJSON_AddItemToArray( outputs, output );

  //
  // Make sure the EDID string is freed up now that it is no longer needed.
  //

  free( szEdid );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to retrieve a list of the parts that make
 * up the device.  This information can include additional information such as part revision
 * or manufacturer.
 *
 * @intg
 *
 * Example implementation returns hard-coded sample data in JSON format.  Replace the body of
 * this command handler function with a device specific implementation that retrieves the
 * parts list for the device and adds the results to the output data.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information
 *    containing the parts list.  In this example the output contains an array with the part
 *    names listed.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : [{"PART A" : {"Number" : "ABC123", "Revision" : "1.1"}},
 *                 {"PART B" : {"Number" : "DEF456", "Revision" : "2.1"}},
 *                 {"PART C" : {"Number" : "GHI789", "Revision" : "3.1"}},
 *                 {"PART D" : {"Number" : "XYZ000", "Revision" : "4.1"}}]}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetPartsList( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseItems;
  cJSON* responseItem;
  cJSON* responseItemAttrs;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** ADD THE DEVICE SPECIFIC PARTS INFORMATION TO THE BY CALLING THE APPROPRIATE DEVICE
  ** SPECIFIC PROCESSING ROUTINES.
  */

  UtvMessage( UTV_LEVEL_INFO,
              "%s - Stubbed Implementation retrieving parts information",
              __FUNCTION__ );

  responseItems = cJSON_CreateArray();

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddStringToObject( responseItemAttrs, "Number", "ABC123" );
  cJSON_AddStringToObject( responseItemAttrs, "Revision", "1.1" );
  cJSON_AddItemToObject( responseItem, "PART A", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddStringToObject( responseItemAttrs, "Number", "DEF456" );
  cJSON_AddStringToObject( responseItemAttrs, "Revision", "2.1" );
  cJSON_AddItemToObject( responseItem, "PART B", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddStringToObject( responseItemAttrs, "Number", "GHI789" );
  cJSON_AddStringToObject( responseItemAttrs, "Revision", "3.1" );
  cJSON_AddItemToObject( responseItem, "PART C", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  responseItem = cJSON_CreateObject();
  responseItemAttrs = cJSON_CreateObject();
  cJSON_AddStringToObject( responseItemAttrs, "Number", "XYZ000" );
  cJSON_AddStringToObject( responseItemAttrs, "Revision", "4.1" );
  cJSON_AddItemToObject( responseItem, "PART D", responseItemAttrs );
  cJSON_AddItemToArray( responseItems, responseItem );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseItems );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to retrieve network configuration
 * information from the device.
 *
 * @intg
 *
 * Example implementation gathers statistical information for the system and adds the
 * following items to the cJSON output object using UtvPlatformGetSystemStats.  Replace the
 * body of this command handler function with a device specific implementation that retrieves
 * statistical information for the device and adds the results to the output data.  The
 * example implementation contains the expected fields; additional device specific fields can
 * be added.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific system statistics
 *    information. In this example the output contains the memory and CPU information
 *    available on a Linux platform.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"Memory" : {"Percent Used" : 50,
 *                             "Free" : 2000000,
 *                             "Used" : 2000000,
 *                             "Total" : 4000000},
 *                 "CPU" : {"Total Percent Used" : 50,
 *                          "User Percent Used" : 25,
 *                          "System Percent Used" : 25}}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 * @sa      UtvPlatformGetSystemStats
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetSystemStatistics( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  responseData = cJSON_CreateObject();
  result = UtvPlatformGetSystemStats( responseData );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to request that the device reset to
 * factory defaults.
 *
 * @intg
 *
 * Example implementation does nothing.  Replace the body of this command handler function
 * with a device specific implementation that restores the factory settings of the device.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportResetFactoryDefaults( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;

  /*
  ** PORTING STEP:
  ** RESET THE DEVICE TO FACTORY DEFAULTS BY CALLING THE APPROPRIATE DEVICE SPECIFIC
  ** PROCESSING ROUTINES.
  */

  UtvMessage( UTV_LEVEL_INFO,
              "%s - Stubbed Implementation reset to factory defaults",
              __FUNCTION__ );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to modify the aspect ratio of the device.
 *
 * @intg
 *
 * Example implementation retrieves the new aspect ratio to apply from the input information.
 * Replace the body of this command handler function with a device specific implementation
 * that adjusts the aspect ratio using the command inputs.  The command inputs include a
 * ratio in the form of WIDTH by HEIGHT (“WxH”) or one of the special values “Zoom” or
 *“Stretch”.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of a single item, "RATIO" which represents the new
 *    setting for the TV.  This values can be "x" separated width and height values or a
 *    string indicating a special setting such as ZOOM or STRETCH.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportSetAspectRatio( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* ratio;

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  ratio = cJSON_GetObjectItem( inputs, "RATIO" );

  /*
   * Validate the inputs, there must be a "RATIO" item, it must be a string.
   */

  if ( NULL == ratio || cJSON_String != ratio->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** SET THE ASPECT RATION FOR THE DEVICE GIVEN THE "RATIO" VALUE BY CALLING THE
  ** APPROPRIATE DEVICE SPECIFIC PROCESSING ROUTINES.  THE "RATIO" VALUE IS A STRING
  ** CONTAINING:
  **  - ASPECT RATIO IN THE FORM OF "NUMxNUM", i.e. "16x9"
  **  - ONE OF THE SPECIAL VALUES: "STRETCH", "ZOOM"
  */

  UtvMessage( UTV_LEVEL_INFO,
              "%s - Stubbed Implementation setting the aspect ratio %s",
              __FUNCTION__,
              ratio->valuestring );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to request that the device display a
 * specific video pattern.
 *
 * @intg
 *
 * Example implementation retrieves the video pattern to apply from the input information.
 * Replace the body of this command handler function with a device specific implementation
 * that applies the video pattern from the command inputs to the device.  The command inputs
 * include the type of pattern to apply “SIMPLE” or “NONE” and the pattern value specified
 * as hexadecimal ARGB string (i.e. “FFFF0000” for red).
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of a 2 items, "TYPE" and "VALUE".  The "TYPE" item is
 *    "SIMPLE" or "NONE" for the initial version of Live Support. When "NONE" is specified,
 *    no pattern should be displayed. If "SIMPLE" is specified, a flat color corresponding
 *    to the color specified by the "VALUE" string should be presented as the whole frame on
 *    the screen. The "VALUE" is the RGBA encoded hex value to display. 
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportApplyVideoPattern( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* patternType = NULL;
  cJSON* patternValue = NULL;
  UTV_BYTE pszA[3];
  UTV_BYTE pszR[3];
  UTV_BYTE pszG[3];
  UTV_BYTE pszB[3];
  UTV_UINT32 ulA = 0;
  UTV_UINT32 ulR = 0;
  UTV_UINT32 ulG = 0;
  UTV_UINT32 ulB = 0;

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  patternType = cJSON_GetObjectItem( inputs, "TYPE" );
  patternValue = cJSON_GetObjectItem( inputs, "VALUE" );

  /*
   * Validate the inputs, there must be a "TYPE" and "VALUE" item.
   */

  if ( ( NULL == patternType ) ||
       ( NULL == patternValue ) ||
       ( cJSON_String != patternType->type ) ||
       ( cJSON_String != patternValue->type ) ||
       ( 8 != UtvStrlen( (UTV_BYTE *) patternValue->valuestring ) ) )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
   * Validate the pattern is of a simple or none type since that is what is
   * currently supported. More pattern types will be supported in the future.
   */
 
  if ( 0 == strncmp( "SIMPLE", patternType->valuestring, 6 ) )
  {
    /*
     * Parse the returned simple pattern type
     */

    UtvStrnCopy( pszA, 3, (UTV_BYTE *) patternValue->valuestring, 2 );
    UtvStrnCopy( pszR, 3, (UTV_BYTE *) &( patternValue->valuestring[2] ), 2 );
    UtvStrnCopy( pszG, 3, (UTV_BYTE *) &( patternValue->valuestring[4] ), 2 );
    UtvStrnCopy( pszB, 3, (UTV_BYTE *) &( patternValue->valuestring[6] ), 2 );

    ulA = UtvStrtoul( (UTV_INT8 *) pszA, NULL, 16 );
    ulR = UtvStrtoul( (UTV_INT8 *) pszR, NULL, 16 );
    ulG = UtvStrtoul( (UTV_INT8 *) pszG, NULL, 16 );
    ulB = UtvStrtoul( (UTV_INT8 *) pszB, NULL, 16 );

    UtvMessage( UTV_LEVEL_INFO,
                "%s - Stubbed Implementation applying video pattern type %s %i,%i,%i,%i",
                __FUNCTION__,
                patternType->valuestring,
                ulA,
                ulR,
                ulG,
                ulB );

    /*
    ** PORTING STEP:
    ** APPLY THE VIDEO PATTERN TO THE DEVICE HERE BY FIRST DETERMINING THE PATTERN TYPE,
    ** INITIALLY A SIMPLE TYPE, AND THEN DISPLAY THE VALUE AS A SOLID COLOR ON THE SCREEN.
    ** THE COLOR IS SPECIFIED IN ARGB HEXADECIMAL STRING FORMAT.
    */
  }
  else if ( 0 == strncmp( "NONE", patternType->valuestring, 4 ) )
  {
    UtvMessage( UTV_LEVEL_INFO,
                "%s - Stubbed Implementation clearing video pattern.",
                __FUNCTION__ );

    /*
    ** PORTING STEP:
    ** CLEAR ANY DISPLAYED PATTERN ON THE SCREEN.
    */
  }
  else
  {
    result = UTV_INVALID_PARAMETER;
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to retrieve device configuration.
 *
 * @intg
 *
 * Example implementation reads the contents of the /etc/services file, Base64 encodes the
 * data and return it in the output data.  Replace the body of this command handler function
 * with a device specific implementation that retrieves the device configuration information.
 * The information must be Base64 encoded and added to the output data.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command the output consists of a text based parameter. The output consists of Base 64
 *    encoded configuration file data that will be downloaded to the technician's computer.  A
 *    method to encode a file to a Base 64 value is demonstrated below. 
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "TEXT",
 *       "DATA" : "BASE 64 ENCODED STRING"}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetDeviceConfig( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;
  cJSON* output;
  UTV_BYTE* pBuffer = NULL;
  UTV_BYTE* pFileReadBuffer = NULL;
  UTV_BYTE* pBase64Buffer = NULL;
  UTV_UINT32 uiFilesize = 0;
  UTV_UINT32 uiBytesRead;
  UTV_UINT32 uiTotalBytesRead = 0;
  UTV_UINT32 uiBase64BufferSize;
  UTV_UINT32 hFile;
  UTV_INT8* pszFilename = "/etc/services";

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs ||
       cJSON_Array != outputs->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** PROVIDE THE CUSTOM FILE/BUFFER IN BASE-64 USING UtvBase64Encode ROUTINE
  */

  /*
   * Get file size
   */

  result = UtvPlatformFileGetSize( pszFilename, &uiFilesize );

  if ( UTV_OK != result )
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Allocate buffer that will contain the config file
   */

  pBuffer = UtvMalloc( uiFilesize + 1 );

  if ( NULL == pBuffer )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Open config file
   */

  result = UtvPlatformFileOpen( pszFilename, "r", &hFile );

  if ( UTV_OK != result )
  {
    UtvFree( pBuffer );
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Read config file
   */

  UtvMemset( pBuffer, 0x00, uiFilesize + 1 );

  pFileReadBuffer = pBuffer;

  while ( uiTotalBytesRead < uiFilesize )
  {
    result =
      UtvPlatformFileReadBinary(
        hFile,
        pFileReadBuffer,
        uiFilesize,
        &uiBytesRead );
          
    uiTotalBytesRead += uiBytesRead;

    if ( UTV_FILE_EOF == result )
    {
      result = UTV_OK;
      break;
    }

    pFileReadBuffer += uiBytesRead;
  }
        
  if ( UTV_OK == result )
  {
    /*
     * Convert config file to Base-64
     */

    result =
      UtvBase64Encode(
        pBuffer,
        uiFilesize,
        &pBase64Buffer,
        &uiBase64BufferSize );

    if ( UTV_OK != result )
    {
      UtvMessage(
        UTV_LEVEL_ERROR,
        "%s() Failed to Base 64 encode device configuration data, \"%s\"",
        __FUNCTION__,
        UtvGetMessageString( result ) );
    }
  }
  else
  {
    UtvMessage(
      UTV_LEVEL_ERROR,
      "%s() Failed to read the device configuration file (%s), \"%s\"",
      __FUNCTION__,
      pszFilename,
      UtvGetMessageString( result ) );
  }

  UtvPlatformFileClose( hFile );
  UtvFree( pBuffer );

  /*
   * Send buffer if everything was successful
   */

  if ( UTV_OK == result )
  {
    output = cJSON_CreateObject();
    cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
    cJSON_AddStringToObject( output, "TYPE", "TEXT" );
    cJSON_AddStringToObject( output, "DATA", (char*) pBase64Buffer );

    cJSON_AddItemToArray( outputs, output );

    UtvMessage( UTV_LEVEL_INFO,
                "%s - Stubbed Implementation getting device configuration.",
                __FUNCTION__);
    
  }

  if ( NULL != pBase64Buffer )
  {
    UtvFree( pBase64Buffer );
  }

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to update the device configuration.
 *
 * @intg
 *
 * Example implementation Base64 decodes the input information and writes the data to the
 * tmpConfig file.  Replace the body of this command handler function with a device specific
 * implementation that Base64 decodes the input data, validates the decoded data and writes
 * the data to the device con-figuration.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of a single item, "CONFIG".  The "CONFIG" item consists
 *    of a Base 64 encoded string. The device should apply the specified config file to the
 *    device specific configuration area. The method to decode the Base 64 encoded config file
 *    is demonstrated below.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportSetDeviceConfig( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* config;
  UTV_BYTE* pBuffer = NULL;
  UTV_BYTE* pFileWriteBuffer = NULL;
  UTV_UINT32 uiBufferSize;
  UTV_UINT32 uiBytesWritten;
  UTV_UINT32 uiTotalBytesWritten = 0;
  UTV_UINT32 hFile;
  UTV_UINT32 uiBase64BufferSize;
  char* pszFilename = "tmpConfig";

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  config = cJSON_GetObjectItem( inputs, "CONFIG" );

  /*
   * Validate the inputs, there must be a "CONFIG" item
   */

  if ( ( NULL == config ) ||
       ( cJSON_String != config->type ) )
  {
    result = UTV_INVALID_PARAMETER;
  }
  else
  {
    /*
     * Decode Base-64 buffer
     */
 
    uiBase64BufferSize = UtvStrlen( (UTV_BYTE *) config->valuestring );

    if ( 0 == uiBase64BufferSize )
    {
      result = UTV_INVALID_PARAMETER;
    }
    else
    {
      result =
        UtvBase64Decode(
          (UTV_BYTE *) config->valuestring,
          uiBase64BufferSize,
          &pBuffer,
          &uiBufferSize );

      if ( UTV_OK == result )
      {
        /*
        ** PORTING STEP:
        ** TAKE THE BINARY BUFFER RETURNED FROM THE UtvBase64Decode FUNCTION AND
        ** APPLY ANY DEVICE SPECIFIC CONFIGURATION LOGIC
        */

        result = UtvPlatformFileOpen( pszFilename, "w", &hFile );

        if ( UTV_OK == result )
        {
          /*
           * Write config file
           */

          pFileWriteBuffer = pBuffer;

          while ( uiTotalBytesWritten < uiBufferSize )
          {
            result = UtvPlatformFileWriteBinary( hFile, pFileWriteBuffer, uiBufferSize,
                                                 &uiBytesWritten );
          
            uiTotalBytesWritten += uiBytesWritten;

            if ( UTV_FILE_EOF == result )
            {
              result = UTV_OK;
              break;
            }

            pFileWriteBuffer += uiBytesWritten;
          }
        
          /*
           * Clean up
           */

          UtvPlatformFileClose( hFile );
        }
        
        UtvFree( pBuffer );
      }
    }
  }

  if ( UTV_OK == result )
  {
    UtvMessage( UTV_LEVEL_INFO,
                "%s - Stubbed Implementation applying config.",
                __FUNCTION__);
  }
  else
  {
    
    UtvMessage( UTV_LEVEL_ERROR,
                "%s, Failed to write configuration file",
                __FUNCTION__); 
  }
  
  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to set the date and time of the device.
 *
 * @intg
 *
 * Example implementation utilizes the UtvPlatformSetSystemTime function to set the date and
 * time of the device.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of a single item, "DATETIME".  The "DATETIME" item
 *    consists of "YYYY-MM-DDTHH:MM:SSZ" formatted string.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.
 *    For this command the output is empty.
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 * @sa      UtvPlatformSetSystemTime
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportSetDateTime( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* responseData;
  cJSON* pDateTime;
  UTV_TIME tNewTime;
  UTV_TIME tCurrentTime;
  struct tm* ptmTime;
  char pszCurrentTime[21];

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  pDateTime = cJSON_GetObjectItem( inputs, "DATETIME" );

  /*
   * Validate the inputs, there must be a "DATETIME" item
   */

  if ( ( NULL == pDateTime ) ||
       ( cJSON_String != pDateTime->type ) )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  result =
    UtvInternetDeviceWebServiceLiveSupportStringToDateTime(
      pDateTime->valuestring,
      &tNewTime );

  if ( UTV_OK == result )
  {
    result = UtvPlatformSetSystemTime( tNewTime );
  }

  tCurrentTime = UtvTime( NULL );
  ptmTime = UtvGmtime( &tCurrentTime );
  strftime( pszCurrentTime, 21, "%Y-%m-%dT%H:%M:%SZ", ptmTime );

  responseData = cJSON_CreateObject();
  cJSON_AddStringToObject( responseData, "CURRENT_DATE", pszCurrentTime );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to call the custom diagnostic sample.
 *
 * @intg
 *
 * Example implementation creates sample HTML and adds it to the outputs.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.
 *    For this command the input is empty.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information.  In this
 *    example the output contains the textual output consisting of the function name.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "TEXT",
 *       "DATA" : "<p><b>This is a custom HTML response</b></p>"}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportCustomDiagnosticSample( cJSON* inputs, cJSON* outputs )
{
  cJSON* output;

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "HTML" );
  cJSON_AddStringToObject( output,
                           "DATA",
                           "<p><b>This is a custom html response</b></p>"
                           "<p>A <em>limited</em> subset of HTML is supported</p>" );

  cJSON_AddItemToArray( outputs, output );

  UtvMessage( UTV_LEVEL_INFO, "Processed %s", __FUNCTION__ );

  return ( UTV_OK );
}

/**
 **********************************************************************************************
 *
 * Command handler called by the ULI SupportTV code to call the custom tool sample.
 *
 * @intg
 *
 * Example implementation validates the input information, displays the input information and
 * copies the input JSON data to the output JSON data.
 *
 * @param inputs
 *    A cJSON object pointer containing the input information for this command handler.  For
 *    this command the inputs consist of a several items, "STATIC_INPUT_STRING",
 *    "TEST_INPUT_STRING", "TEST_INPUT_NUMERIC", "TEST_INPUT_BOOL" and TEST_INPUT_LIST". The
 *    items contain a value of the type indicated by their name.
 *
 * @param outputs
 *    A pointer to a cJSON object used to hold any output for this command handler.  For this
 *    command there is a single output which consists of device specific information.  In this
 *    example the output contains the textual output consisting of the function name.
 *    @verbatim
 *      The output object contains an identifer ("ID"), the type of the data ("TYPE") and the
 *      data itself ("DATA").
 *
 *      {"ID" : "RESPONSE_DATA",
 *       "TYPE" : "JSON",
 *       "DATA" : {"Static String Input" : Value from STATIC_INPUT_STRING,
 *                 "String Input" : Value from TEST_INPUT_STRING,
 *                 "Numeric Input" : Value from TEST_INPUT_NUMERIC,
 *                 "Bool Input" : Value from TEST_INPUT_BOOL,
 *                 "List Input" : Value from TEST_INPUT_LIST}}
 *    @endverbatim
 *
 * @return  UTV_OK if the processing executed successfully
 * @return  UTV_RESULT value if errors occurs.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportCustomToolSample( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* staticString;
  cJSON* inputString;
  cJSON* inputNumeric;
  cJSON* inputBool;
  cJSON* inputList;
  cJSON* output;
  cJSON* responseData;

  /*
   * Validate that the inputs exists before attempt to extract items from it.
   */

  if ( NULL == inputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
   * Validate that the outputs exists before attempting to add data to it.
   */

  if ( NULL == outputs )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  staticString = cJSON_GetObjectItem( inputs, "STATIC_INPUT_STRING" );
  inputString = cJSON_GetObjectItem( inputs, "TEST_INPUT_STRING" );
  inputNumeric = cJSON_GetObjectItem( inputs, "TEST_INPUT_NUMERIC" );
  inputBool = cJSON_GetObjectItem( inputs, "TEST_INPUT_BOOL" );
  inputList = cJSON_GetObjectItem( inputs, "TEST_INPUT_LIST" );

  /*
   * Validate the inputs, there must be the four inputs expected with the expected types:
   *  - STATIC_INPUT_STRING has type cJSON_String
   *  - TEST_INPUT_STRING has type cJSON_String
   *  - TEST_INPUT_NUMERIC has type cJSON_Number
   *  - TEST_INPUT_BOOL has types cJSON_True or cJSON_False
   *  - TEST_INPUT_LIST has type cJSON_String, NOTE that the input type is described as
   *    a list input which is different that the type of value expected to be in the
   *    input sent from client to NOC to device.  In this case the input data is a string.
   */

  if ( NULL == staticString ||
       cJSON_String != staticString->type ||
       NULL == inputString ||
       cJSON_String != inputString->type ||
       NULL == inputNumeric ||
       cJSON_Number != inputNumeric->type ||
       NULL == inputBool ||
       ( cJSON_True != inputBool->type && cJSON_False != inputBool->type ) ||
       NULL == inputList ||
       cJSON_String != inputList->type )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /*
  ** PORTING STEP:
  ** THIS IS A SAMPLE CUSTOM COMMAND, THE IMPLEMENTATION IS COMPLETE CEM/PLATFORM SPECIFIC.
  */

  UtvMessage( UTV_LEVEL_INFO,
              "%s - Stubbed Implementation of Custom Command Sample",
              __FUNCTION__ );

  UtvMessage( UTV_LEVEL_INFO, "Custom Input Information:" );
  UtvMessage( UTV_LEVEL_INFO, "\tStatic String Input: %s", staticString->valuestring );
  UtvMessage( UTV_LEVEL_INFO, "\tString Input: %s", inputString->valuestring );
  UtvMessage( UTV_LEVEL_INFO, "\tNumeric Input: %d", inputNumeric->valueint );
  UtvMessage( UTV_LEVEL_INFO, "\tBool Input: %d", inputBool->valueint );
  UtvMessage( UTV_LEVEL_INFO, "\tList Input: %s", inputList->valuestring );

  responseData = cJSON_CreateObject();

  cJSON_AddStringToObject( responseData, "Static String Input", staticString->valuestring );
  cJSON_AddStringToObject( responseData, "String Input", inputString->valuestring );
  cJSON_AddNumberToObject( responseData, "Numeric Input", inputNumeric->valueint );
  cJSON_AddItemToObject( responseData, "Bool Input", cJSON_CreateBool( inputBool->valueint ) );
  cJSON_AddStringToObject( responseData, "List Input", inputList->valuestring );

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", responseData );

  cJSON_AddItemToArray( outputs, output );

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT */

#ifdef UTV_LIVE_SUPPORT_VIDEO
/**
 **********************************************************************************************
 *
 * Called by the Device Agent Live Support video code to determine if the device supports
 * securing the media output stream.  This capability is reported to and may be invoked by the
 * NetReady Services during a Live Support session.
 * 
 * The default implementation basis its return value off of UTV_LIVE_SUPPORT_VIDEO_SECURE.
 *
 * @intg
 *
 * A replacement implementation may base its return value off of a run-time indicator.
 * 
 * @return  UTV_TRUE if the device supports a secure media output stream.
 *
 * @return  UTV_FALSE if the device does NOT support a secure media output stream.
 *
 * @note
 *
 * Securing the media stream requires customizations to the GStreamer TCP base plug-in (a
 * third-party package); contact UpdateLogic for additional information regarding securing the
 * media stream.
 *
 **********************************************************************************************
 */

UTV_BOOL UtvPlatformLiveSupportVideoSecureMediaStream( void )
{
#ifdef UTV_LIVE_SUPPORT_VIDEO_SECURE
  return ( UTV_TRUE );
#else
  return ( UTV_FALSE );
#endif /* UTV_LIVE_SUPPORT_VIDEO_SECURE */
}

/**
 **********************************************************************************************
 *
 * Called by the Device Agent Live Support video code to retreive the path to the GStreamer
 * libraries (example: "/usr/lib/gstreamer-0.10").
 *
 * The default implementation returns the standard GStreamer library installation path.
 *
 * @intg
 *
 * A replacement implementation could return a custom path.
 * 
 * @param   pubBuffer     Buffer in which to place the library path string.
 *
 * @param   uiBufferSize  The size of pubBuffer (maximum number of bytes to place).
 *
 * @return  UTV_OK if the library path string was successfully set.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformLiveSupportVideoGetLibraryPath(
  UTV_BYTE* pubBuffer,
  UTV_UINT32 uiBufferSize )
{
  UTV_RESULT result = UTV_OK;

  if ( NULL == pubBuffer )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvMemset( pubBuffer, 0x00, uiBufferSize );
  UtvMemFormat( pubBuffer, uiBufferSize, "%s", "/usr/lib/gstreamer-0.10" );

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Called by the Device Agent Live Support video code to retreive the dimensions (width,
 * height, and pitch) of the video frame.  These values are used to calculate the size of the
 * buffer used in the UtvPlatformLiveSupportVideoFrameBuffer() function and also to inform
 * downstream consumers of the media output stream.
 *
 * The default implementation returns 960 x 1080 x 4 (Width x Height x Pitch).
 *
 * @intg
 *
 * A replacement implementation should return the correct dimensions for the current platform.
 *
 * @param   piWidth       Width buffer.
 *
 * @param   piHeight      Height buffer.
 *
 * @param   piPitch       Pitch buffer.
 *
 * @return  UTV_OK if the the video frame dimensions were succesfully set.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformLiveSupportVideoGetVideoFrameSpecs(
  UTV_INT32* piWidth,
  UTV_INT32* piHeight,
  UTV_INT32* piPitch )
{
  UTV_RESULT result = UTV_OK;

  if ( NULL == piWidth || NULL == piHeight || NULL == piPitch )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  *piWidth = 960;
  *piHeight = 1080;
  *piPitch = 4;

  return ( result );
}

/**
 **********************************************************************************************
 *
 * Called by the Device Agent Live Support video code to retreive the next video frame buffer.
 *
 * The frame rate established when the media output stream begins controls the rate at which
 * this function is called.  The default frame rate is 2 frames-per-second, which would cause
 * this function to be called approximately twice per second.
 *
 * The default implementation generates two solid colored buffers (both shades of gray) that
 * alternate every 5th call to this function.  This is useful for development purposes.
 *
 * @intg
 *
 * A replacement implementation should return an actual video frame for the platform.
 * 
 * @param   pubBuffer     Buffer in which to place the next video frame.
 *
 * @param   uiBufferSize  The size of pubBuffer (maximum number of bytes to place).
 *
 * @return  UTV_OK if the buffer was successfully filled.
 *
 * @return  UTV_RESULT value if an error occurs.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvPlatformLiveSupportVideoFillVideoFrameBuffer(
  UTV_BYTE* pubBuffer,
  UTV_UINT32 uiBufferSize )
{
  static UTV_UINT32 uiRequestCounter = 0;
  static UTV_BYTE ubFillValue = 0xAA;

  UTV_RESULT result = UTV_OK;

  if ( NULL == pubBuffer )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  uiRequestCounter++;

  if ( 0 == (uiRequestCounter % 5) )
  {
    ubFillValue = ( ubFillValue == 0xAA ? 0x33 : 0xAA );
  }

  UtvMemset( pubBuffer, ubFillValue, uiBufferSize );

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT_VIDEO */


/* Android specific helper functions */

void UtvProjectSetPeristentPath(UTV_INT8 * path){
    if (path == NULL)
        path = DEFAULT_PERSISTENT_PATH;
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_PERSISTENT_PATH, 
                  sizeof(UTV_PLATFORM_PERSISTENT_PATH), 
                  "%s", path );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_ULID_FNAME, 
                  sizeof(UTV_PLATFORM_INTERNET_ULID_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "ulid.dat" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_PROVISIONER_PATH, 
                  sizeof(UTV_PLATFORM_INTERNET_PROVISIONER_PATH), 
                  "%s", UTV_PLATFORM_PERSISTENT_PATH );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_UPDATE_STORE_0_FNAME, 
                  sizeof(UTV_PLATFORM_UPDATE_STORE_0_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "update-0.store" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_UPDATE_STORE_1_FNAME, 
                  sizeof(UTV_PLATFORM_UPDATE_STORE_1_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "update-1.store" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME, 
                  sizeof(UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "component.manifest" ); 
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_PERSISTENT_STORAGE_FNAME, 
                  sizeof(UTV_PLATFORM_PERSISTENT_STORAGE_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "persistent-storage.dat" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_REGSTORE_STORE_AND_FWD_FNAME, 
                  sizeof(UTV_PLATFORM_REGSTORE_STORE_AND_FWD_FNAME), 
                  "%s%s", UTV_PLATFORM_PERSISTENT_PATH, "regstore.txt" );
}

void UtvProjectSetFactoryPath(UTV_INT8 * path){
    if (path == NULL)
        path = DEFAULT_FACTORY_PATH;
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_FACTORY_PATH, 
                  sizeof(UTV_PLATFORM_INTERNET_FACTORY_PATH), 
                  "%s", path );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_ULPK_FNAME, 
                  sizeof(UTV_PLATFORM_INTERNET_ULPK_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_FACTORY_PATH, "ulpk.dat" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_SERIAL_NUMBER_FNAME, 
                  sizeof(UTV_PLATFORM_SERIAL_NUMBER_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_FACTORY_PATH, "snum.txt" );
}

void UtvProjectSetReadOnlyPath(UTV_INT8 * path){
    if (path == NULL)
        path = DEFAULT_READ_ONLY_PATH;
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_READ_ONLY_PATH, 
                  sizeof(UTV_PLATFORM_INTERNET_READ_ONLY_PATH), 
                  "%s", path );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_SSL_CA_CERT_FNAME, 
                  sizeof(UTV_PLATFORM_INTERNET_SSL_CA_CERT_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_READ_ONLY_PATH, "uli.crt" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME, 
                  sizeof(UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_READ_ONLY_PATH, "component.manifest" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_KEYSTORE_FNAME, 
                  sizeof(UTV_PLATFORM_KEYSTORE_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_READ_ONLY_PATH, "keystore.dat" );
    UtvMemFormat( (UTV_BYTE *) UTV_PLATFORM_INTERNET_READ_ONLY_ULPK_FNAME, 
                  sizeof(UTV_PLATFORM_INTERNET_READ_ONLY_ULPK_FNAME), 
                  "%s%s", UTV_PLATFORM_INTERNET_READ_ONLY_PATH, "ulpk-0.dat" );
}

void UtvProjectSetUpdateInstallPath(UTV_INT8 * path){ 
    if (path == NULL)
        path = DEFAULT_INSTALL_PATH;
    UtvMemFormat( (UTV_BYTE *) s_installPath, sizeof(s_installPath), "%s", path );
}

UTV_INT8 *UtvProjectGetUpdateInstallPath(){
    return s_installPath;
}

void UtvProjectSetUpdateReadyToInstall(UTV_BOOL updateAv){
    s_updateReadyToInstall = updateAv;
}

UTV_BOOL UtvProjectGetUpdateReadyToInstall(){
    return s_updateReadyToInstall;
}

void UtvProjectSetUpdateSchedule(UTV_PUBLIC_SCHEDULE *pCurrentUpdateScheduled){
    if ( NULL != s_pCurrentUpdateScheduled )
    {
        UtvImageClose( s_pCurrentUpdateScheduled->tUpdates[ 0 ].hImage );
        s_pCurrentUpdateScheduled = NULL;
    }
    s_pCurrentUpdateScheduled = pCurrentUpdateScheduled;
}

UTV_PUBLIC_SCHEDULE *UtvProjectGetUpdateSchedule(){
    return s_pCurrentUpdateScheduled;
}

