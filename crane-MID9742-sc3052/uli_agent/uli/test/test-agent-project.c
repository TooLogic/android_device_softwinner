/* test-agent-project.c : A test program that exercies the Agent via the project interfaces.

   This test emulates simple TV events and the corresponding Agent functions including:

       1.  On boot.
       2.  On network up.
       3.  On USB insert.
       4.  On DVD insert.
       5.  On security key request.
       6.  On request to check for update.
       7.  On request to check for stored update.
       8.  On request for status.
       9.  On shutdown.
      10.  On reset to initial component manifest.
      11.  On switch to factory mode.

   Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who            Date        Edit
   Bob Taylor     12/26/2011  Added s_insertULPKTest().
   Bob Taylor     12/20/2011  Added UTV_FILE_BASED_PROVISIONING ifdef to eliminate object request mechanism
                              when using UtvPlatformOnProvisioned().
   Bob Taylor     11/30/2011  Early boot logic added.
   Jim Muchow     06/29/2011  Add in examples showing how to initialize OpenSSL external to the agent
   Bob Taylor     02/23/2011  New security unit test().
   Bob Taylor     12/30/2010  Added UtvProjectOnObjectRequestStatus() in the provisioned object request example.
   Chris Nigbur   10/21/2010  Added s_liveSupport test.
   Bob Taylor     10/01/2010  Added UtvProjectOnForceNOCContact() test.    Added s_waitForAgent() test.
   Bob Taylor     07/02/2010  Added example of UtvProjectOnObjectRequestSize() use in s_requestSecurityKey().
   Bob Taylor     06/30/2010  Added support for UtvProjectOnDebugInfo().
   Bob Taylor     06/23/2010  Added loop mode status display and UtvImageClose() to s_displayCommands().
   Bob Taylor     06/14/2010  UtvClearState() added in response to abort.
   Bob Taylor     06/07/2010  Fixed getch() for cygwin 1.7 issue.
   Bob Taylor     05/03/2010  Replaced UtvPlatformProvisionerGetObject() with UtvProjectOnObjectRequest().
   Bob Taylor     04/09/2010  Added support for USB command processing.
   Bob Taylor     12/13/2009  Changed UtvProjectOnUpdate() to UtvProjectOnScanForUpdate()
                              in power down handler.  UtvProjectOnUpdate() is called within
                              the event handler.  Reorganized events into scan and then download.
                              Added network up and get key simulation.
                              Added scan for stored images.
   Bob Taylor     11/13/2009  Created.
*/

#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "utv.h"
#include UTV_TARGET_PROJECT

/*
*****************************************************************************************************
**
** STATIC DEFINES
**
*****************************************************************************************************
*/
/* if defined each component is individually approved by s_approveOptionalComponents()
   otherwise all components are approved */
/* #define S_APPROVE_EACH_COMPONENT */

/* example security key object owner */
#define S_SECURITY_KEY_OWNER            "test-owner"

/* example security key name */
#define S_SECURITY_KEY_NAME             "test-object"

/* example timeout to wait for Agent to complete a task */
#define S_WAIT_LEN_IN_MSECS             10000

#define S_TEST_ULPK_FNAME               "ulpk.transit"
#define S_ULPK_SIZE						96

/*
*****************************************************************************************************
**
** STATIC VARS
**
*****************************************************************************************************
*/
static UTV_BOOL s_bExitRequest = UTV_FALSE;

/*
*****************************************************************************************************
**
** STATIC FUNCTIONS
**
*****************************************************************************************************
*/
static void s_eventCallback( UTV_PUBLIC_RESULT_OBJECT *pResultObject );
static void s_SIGINT_handler (int signum);
static void s_SIGPIPE_handler (int signum);
static void s_errorExit( UTV_UINT32 rVal );
static void s_usage( void );
static void s_displayStatus( void );
static void s_displayUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
static void s_displayUpdateAlreadyHave( void );
static void s_displayUpdateIncompatible( UTV_RESULT rStatus );
static void s_displayCommands( UTV_RESULT rStatus );
static void s_displayUnknown( UTV_RESULT rStatus );
static void s_insertULPKTest( void );
#ifdef S_APPROVE_EACH_COMPONENT
static void s_approveOptionalComponents( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
#endif
#ifndef UTV_FILE_BASED_PROVISIONING
static void s_requestSecurityKey( void );
#endif
static void s_nonblock( void );
static int  s_kbhit( void );
static void s_waitForAgent( void );
static void s_securityUnitTest( void );
static void s_regStoreTest( void );
#ifdef UTV_DEVICE_WEB_SERVICE
static void s_storeDeviceDataTest( void );
static void s_storeDeviceDataTest_Fixed( UtvStoreDeviceData_MetaData_t* pMetaData, UTV_UINT32 uiDataLength );
static void s_storeDeviceDataTest_Random( UtvStoreDeviceData_MetaData_t* pMetaData, UTV_UINT32 numBytes );
#endif

#ifdef UTV_LIVE_SUPPORT
static void s_liveSupport( void );
static void s_liveSupportTestConnection( void );
#endif /* UTV_LIVE_SUPPORT */

/* main
    Main body of test program.
*/

#undef ABORT_TEST
/* #define ABORT_TEST */

int main( int argc, char * argv[] )
{
#ifdef ABORT_TEST
    int cnt=0;
    int wait_time=0;
#endif
    /* Setup graceful exit in case of control-c/kill */
    signal( SIGINT, s_SIGINT_handler );
    signal( SIGTERM, s_SIGINT_handler );
    signal( SIGABRT, s_SIGINT_handler );

    /* Setup graceful handler for SIGPIPE */
    signal( SIGPIPE, s_SIGPIPE_handler );

	/* Android path setup */
    UtvProjectSetUpdateInstallPath(NULL);
    UtvProjectSetPeristentPath(NULL);
    UtvProjectSetFactoryPath(NULL);
    UtvProjectSetReadOnlyPath(NULL);

    /* lightweight early boot.  called once */
    UtvProjectOnEarlyBoot();

    /* Name this thread.  Can be issued before UtvPublicAgentInit() */
    UtvDebugSetThreadName( "main" );

    /* Announce program name */
    UtvMessage( UTV_LEVEL_INFO, UTV_AGENT_VERSION " test-agent-project" );

    /* display usage */
    s_usage();

    /*** PORTING STEP:  REPLACE THIS PLACEHOLDER CALLBACK WITH YOUR OWN ***/
    UtvProjectOnRegisterCallback( s_eventCallback );

#ifndef UTV_INTERNAL_SSL_INIT 
    /*** PORTING STEP: REPLACE THIS PLACEHOLDER FUNCTION WITH YOUR OWN ***/
    /* OpenSSL needs to be initialized for use. Under this conditional,  */
    /* an example is given showing how to initialize OpenSSL. In this    */
    /* case, the function called is the same as that called by the agent */
    /* when it performs the intialization.                               */
    UtvPlatformInternetSSL_Init();
#endif
#ifdef ABORT_TEST
    /* Processing loop to test what happens when "a user" aborts
       a download at various times */
    UtvProjectOnNetworkUp();
    while (1) /*for(loop=0;loop<10;loop++)*/
    {
        printf("\n\n");
        printf(" Count: %u \n", cnt);
        system ("date");
        sleep(3);
        srand ( time(NULL) );
        wait_time = 1000 *(rand() % 90);/*(int)(1000 * rand() / (RAND_MAX + 1.0));*/
        if (!wait_time)
            wait_time = 1000;
        sleep(1);
        UtvProjectOnScanForNetUpdate( NULL );
        printf("\n##################### start wait %d usec #####################\n\n", wait_time);
        UtvSleep(wait_time);
        printf("\n##################### end wait %d usec #####################\n\n", wait_time);
        UtvProjectOnAbort();
        printf("Aborted !!!!!\n");
        sleep(1);
        UtvProjectOnShutdown();
        sleep(1);
        ++cnt;
    }
    return 0;
#endif
    /* turn off kb blocking */
    s_nonblock();

    while ( !s_bExitRequest )
    {
        /* wait for a key */
        while (!s_kbhit() )
            usleep(1);

        /* process it */
        switch( toupper(getchar()) )
        {
        case 'A': /* simulate user abort */
            UtvProjectOnAbort();
            break;
        case 'B': /* scan for stored updates */
#ifndef UTV_INTERNAL_SSL_INIT
            /*** PORTING STEP: REPLACE THIS PLACEHOLDER FUNCTION WITH YOUR OWN ***/
            /* OpenSSL needs to be initialized for use. Under this conditional,  */
            /* an example is given showing how to initialize OpenSSL. In this    */
            /* case, the function called is the same as that called by the agent */
            /* when it performs the intialization.                               */
            UtvPlatformInternetSSL_Init();
#endif
            break;
        case 'C': /* scan for stored updates */
            UtvProjectOnScanForStoredUpdate( NULL );
            break;
        case 'D': /* display the status of the Agent */
            s_displayStatus();
            break;
        case 'F': /* invalidate the update query cache */
            UtvProjectOnForceNOCContact();
            break;
        case 'G': /* Test RegStore functionality */
            s_regStoreTest();
            break;
#ifdef UTV_DEVICE_WEB_SERVICE
        case 'H': /* Test StoreDeviceData functionality */
            s_storeDeviceDataTest();
            break;
#endif
        case 'I': /* Test StoreDeviceData functionality */
            s_insertULPKTest();
            break;
#ifndef UTV_FILE_BASED_PROVISIONING
        case 'K': /* simulate a streaming client asking for a security key */
            s_requestSecurityKey();
            break;
#endif
#ifdef UTV_LIVE_SUPPORT
        case 'L': /* simulate a live support session */
            s_liveSupport();
            break;
#endif /* UTV_LIVE_SUPPORT */
        case 'M': /* make the factory mode component manifest the active component manifest */
            UtvProjectOnFactoryMode();
            break;
        case 'N': /* simulate network up which refreshes the netProvision local cache */
            UtvProjectOnNetworkUp();
            break;
        case 'P': /* simulate power off update scan */
            UtvProjectOnScanForNetUpdate( NULL );
            break;
        case 'Q': /* quit simulation */
            s_errorExit( 0 );
            break;
        case 'R': /* make the backup component manifestt the active component manmifest */
            UtvProjectOnResetComponentManifest();
            break;
        case 'S': /* simulate system shutdown */
            UtvProjectOnShutdown();
#ifndef UTV_INTERNAL_SSL_INIT
            /*** PORTING STEP: REPLACE THIS PLACEHOLDER FUNCTION WITH YOUR OWN ***/
            /* OpenSSL needs to be initialized for use, but it may also need to  */
            /* be released or de-initialized when shutting down the agent. Under */
            /* this conditional, an example is given showing how to release the  */
            /* basic OpenSSL structures. In this case, the function called is    */
            /* the same as that called by the agent when it performs the release */
            UtvPlatformInternetSSL_Deinit();
#endif
            break;
#ifdef UTV_LIVE_SUPPORT
        case 'T': /* test Live Support connection to the NOC */
            s_liveSupportTestConnection();
            break;
#endif /* UTV_LIVE_SUPPORT */
        case 'U': /* simulate USB insertion */
            UtvProjectOnUSBInsertionBlocking( "/mnt/sda1" );
            break;
        case 'V': /* simulate DVD insertion */
            UtvProjectOnDVDInsertion();
            break;
#ifndef UTV_FILE_BASED_PROVISIONING
        case 'X': /* display debug info */
            UtvProjectOnDebugInfo();
            break;
#endif
        case 'W': /* wait for Agent to finish what it's doing */
            s_waitForAgent();
            break;
        case 'Z': /* wait for Agent to finish what it's doing */
            s_securityUnitTest();
            break;
        case '?':
        default:
            s_usage();
            break;
        }

    }

    s_errorExit( 0 );

    return 0;
}

/* s_eventCallback
    *** PORTING STEP:  REPLACE THIS ENTIRE FUNCTION WITH YOUR OWN EVENT HANDLER ***
    This code is an example of a event handling callback.
    Your real event handling callback will react to events by displaying UI
    information, etc.
*/
static void s_eventCallback( UTV_PUBLIC_RESULT_OBJECT *pResultObject )
{
    UTV_PUBLIC_SCHEDULE *pSchedule;
    UTV_RESULT             rStatus;

    /* clear abort whenever it's encountered */
    if ( UTV_ABORT == pResultObject->rOperationResult )
        UtvClearState();

    switch ( pResultObject->iToken )
    {
    case UTV_PROJECT_SYSEVENT_USB_SCAN_RESULT:
        UtvMessage( UTV_LEVEL_INFO, "   UTV_PROJECT_SYSEVENT_USB_SCAN_RESULT: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        /* fall through, complicate debug statementes a bit */
    case UTV_PROJECT_SYSEVENT_DVD_SCAN_RESULT:
        if ( pResultObject->iToken != UTV_PROJECT_SYSEVENT_USB_SCAN_RESULT )
            UtvMessage( UTV_LEVEL_INFO, "   UTV_PROJECT_SYSEVENT_DVD_SCAN_RESULT: \"%s\"",
                        UtvGetMessageString( pResultObject->rOperationResult ) );
        /* Inform the user about the event */
        switch( pResultObject->rOperationResult )
        {
        case UTV_OK:
            /* media contains a valid update, inform user device is being updated */
            pSchedule = (UTV_PUBLIC_SCHEDULE *) pResultObject->pOperationResultData;
            s_displayUpdateInfo( &pSchedule->tUpdates[ 0 ] );
            /* It's possible to check optional updates or other Publisher
               provided attributes here by looking at the tUpdate[ 0 ] structure.
               This example assumes the update should be executed. */
            /* Initiate media update.  Result will come through on
               UTV_PROJECT_SYSEVENT_FILE_DOWNLOAD event below. */
            UtvProjectOnInstallFromMedia( &pSchedule->tUpdates[ 0 ] );
            break;
        case UTV_BAD_MODVER_CURRENT:
            /* media contains the same software that is already loaded */
            s_displayUpdateAlreadyHave();
            break;
        case UTV_INCOMPATIBLE_OUI:
        case UTV_INCOMPATIBLE_MGROUP:
        case UTV_INCOMPATIBLE_HWMODEL:
        case UTV_INCOMPATIBLE_FACT_TEST:
        case UTV_INCOMPATIBLE_SWVER:
        case UTV_DEPENDENCY_MISSING:
        case UTV_INCOMPATIBLE_DEPEND:
        case UTV_INCOMPATIBLE_FTEST:
            /* media contains an update that is incompatible */
            s_displayUpdateIncompatible( pResultObject->rOperationResult );
            break;
        case UTV_COMMANDS_DETECTED:
            /* execute the detected commands */
            rStatus = UtvImageProcessUSBCommands();
            /* let user know this has happened */
            s_displayCommands( rStatus );
            break;
        default:
            /* unknown result */
            s_displayUnknown( pResultObject->rOperationResult );
            break;
        }
        break;

    case UTV_PROJECT_SYSEVENT_NETWORK_SCAN_RESULT:
        UtvMessage( UTV_LEVEL_INFO, "   UTV_PROJECT_SYSEVENT_NETWORK_SCAN_RESULT: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        if ( UTV_OK != pResultObject->rOperationResult )
            break;

        /* received a schedule event. */
        {
            UTV_PUBLIC_SCHEDULE *pSchedule = (UTV_PUBLIC_SCHEDULE *)pResultObject->pOperationResultData;
            /* arbitrarily select the first update.  Multiple update handler could be inserted here. */
            UTV_PUBLIC_UPDATE_SUMMARY *pUpdate = &pSchedule->tUpdates[ 0 ];
            UTV_RESULT rStatus;

#ifdef S_APPROVE_EACH_COMPONENT
            /* each component can be approved individually as shown here */
            s_approveOptionalComponents{ pUpdate );
#else
            /* or they can all be approved all at once using these two calls */
            UtvMessage( UTV_LEVEL_INFO, "   approving all components" );

            /* direct the download to install while it is downloading and to not convert optional components to required */
            UtvPublicSetUpdatePolicy( UTV_TRUE, UTV_FALSE );

            /* Abitrarily approve ALL of the components in the returned schedule. */
            if ( UTV_OK != ( rStatus = UtvPublicApproveForDownload( pUpdate, UTV_PLATFORM_UPDATE_STORE_0_INDEX )))
            {
                UtvMessage( UTV_LEVEL_ERROR,
                            "UtvPublicApproveUpdate( pUpdate, UTV_PLATFORM_UPDATE_STORE_0_INDEX ) \"%s\"",
                            UtvGetMessageString( rStatus ) );
                break;
            }
#endif

            /* now that approval is complete launch the download */
            UtvProjectOnUpdate( pUpdate );
        }
        break;

    case UTV_PROJECT_SYSEVENT_DOWNLOAD:
        UtvMessage( UTV_LEVEL_INFO, "   UTV_PROJECT_SYSEVENT_DOWNLOAD: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        break;

    case UTV_PROJECT_SYSEVENT_MEDIA_INSTALL:
        UtvMessage( UTV_LEVEL_INFO, "   UTV_PROJECT_SYSEVENT_MEDIA_INSTALL: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        break;

    default:
        UtvMessage( UTV_LEVEL_INFO, "   UNKNOWN EVENT: %u: \"%s\"", pResultObject->iToken,
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        return;
    }
}

/* s_displayStatus, Etc...
    *** PORTING STEP:  REPLACE THE FOLLOWING DISPLAY ROUTINES WITH FUNCTIONS AND DISPLAY INFO
                       THAT IS APPROPRIATE TO YOUR DEVICE ***
    The display functions below are primitive examples of the kind of UI information that you
    may wish to display in response to various events.
*/
static void s_displayStatus( void )
{
    UTV_RESULT                     rStatus;
    UTV_PROJECT_STATUS_STRUCT    tAgentStatus;
    UTV_INT8                    cESN[48];
    UTV_INT8                    cQH[32];
    UTV_INT8                    cVersion[32];
    UTV_INT8                    cInfo[128];
    UTV_INT8                    cError[128];
    UTV_INT8                    cTime[48];

    /* setup return strings */
    tAgentStatus.pszESN = cESN;
    tAgentStatus.uiESNBufferLen = sizeof(cESN);
    tAgentStatus.pszQueryHost = cQH;
    tAgentStatus.uiQueryHostBufferLen = sizeof(cQH);
    tAgentStatus.pszVersion = cVersion;
    tAgentStatus.uiVersionBufferLen = sizeof(cVersion);
    tAgentStatus.pszInfo = cInfo;
    tAgentStatus.uiInfoBufferLen = sizeof(cInfo);
    tAgentStatus.pszLastError = cError;
    tAgentStatus.uiLastErrorBufferLen = sizeof(cError);

    /* get status */
    if ( UTV_OK != ( rStatus = UtvProjectOnStatusRequested( &tAgentStatus )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvOnProjectStatusRequested() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* display it */
    UtvMessage( UTV_LEVEL_INFO,    "ESN: %s", tAgentStatus.pszESN );
    UtvMessage( UTV_LEVEL_INFO,    "UID: %u", tAgentStatus.uiULPKIndex );
    UtvMessage( UTV_LEVEL_INFO,    "QH:  %s", tAgentStatus.pszQueryHost );
    UtvMessage( UTV_LEVEL_INFO,    "VER: %s", tAgentStatus.pszVersion );
    UtvMessage( UTV_LEVEL_INFO,    "INF: %s", tAgentStatus.pszInfo );
    UtvMessage( UTV_LEVEL_INFO,    "OUI: 0x%06X", tAgentStatus.uiOUI );
    UtvMessage( UTV_LEVEL_INFO,    "MG:  0x%04X", tAgentStatus.usModelGroup );
    UtvMessage( UTV_LEVEL_INFO,    "HM:  0x%04X", tAgentStatus.usHardwareModel );
    UtvMessage( UTV_LEVEL_INFO,    "SV:  0x%04X", tAgentStatus.usSoftwareVersion );
    UtvMessage( UTV_LEVEL_INFO,    "MV:  0x%04X", tAgentStatus.usModuleVersion );
    UtvMessage( UTV_LEVEL_INFO,    "NP:  %u", tAgentStatus.usNumObjectsProvisioned );
    UtvMessage( UTV_LEVEL_INFO,    "REG: %u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_REGISTERED) );
    UtvMessage( UTV_LEVEL_INFO,    "FCT: %u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_FACTORY_TEST_MODE) );
    UtvMessage( UTV_LEVEL_INFO,    "FET: %u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_FIELD_TEST_MODE) );
    UtvMessage( UTV_LEVEL_INFO,    "LP:  %u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_LOOP_TEST_MODE) );
    UtvMessage( UTV_LEVEL_INFO,    "EC:  %u", tAgentStatus.uiErrorCount );
    UtvMessage( UTV_LEVEL_INFO,    "LE:  %u", tAgentStatus.uiLastErrorCode );
    UtvMessage( UTV_LEVEL_INFO,    "LES: %s", tAgentStatus.pszLastError );
    UtvMessage( UTV_LEVEL_INFO,    "LET: %s", UtvCTime( tAgentStatus.tLastErrorTime, cTime, sizeof(cTime) )  );
}

static void s_displayUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_RESULT                     rStatus;
    UTV_PROJECT_UPDATE_INFO     tUpdateInfo;
    UTV_INT8                    cVersion[32];
    UTV_INT8                    cInfo[128];

    /* setup return strings */
    tUpdateInfo.pszVersion = cVersion;
    tUpdateInfo.uiVersionBufferLen = sizeof(cVersion);
    tUpdateInfo.pszInfo = cInfo;
    tUpdateInfo.uiInfoBufferLen = sizeof(cInfo);

    /* get update info */
    if ( UTV_OK != ( rStatus = UtvProjectOnUpdateInfo( pUpdate, &tUpdateInfo )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvOnProjectUpdateInfo() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* display update message */
    UtvMessage( UTV_LEVEL_INFO,    "MEDIA CONTAINS THE FOLLOWING UPDATE:" );
    UtvMessage( UTV_LEVEL_INFO,    "   VER: %s", tUpdateInfo.pszVersion );
    UtvMessage( UTV_LEVEL_INFO,    "   INF: %s", tUpdateInfo.pszInfo );
    UtvMessage( UTV_LEVEL_INFO,    "YOUR DEVICE IS BEING UPDATED." );
    UtvMessage( UTV_LEVEL_INFO,    "PLEASE DO NOT TURN IT OFF." );
    UtvMessage( UTV_LEVEL_INFO,    "YOUR DEVICE WILL REBOOT WHEN THE UPDATE IS COMPLETE." );
}

static void s_displayUpdateAlreadyHave( void )
{
    /* display already have message */
    UtvMessage( UTV_LEVEL_INFO,    "MEDIA CONTAINS UPDATE THAT DEVICE ALREADY HAS." );
}

static void s_displayUpdateIncompatible( UTV_RESULT rStatus )
{
    /* display already incompatiblity message */
    UtvMessage( UTV_LEVEL_INFO,    "MEDIA UPDATE IS INCOMPATIBLE." );
    UtvMessage( UTV_LEVEL_INFO,    "REASON: %s.", UtvGetMessageString( rStatus ) );
}

static void s_displayCommands( UTV_RESULT cStatus )
{
    UTV_RESULT                  rStatus;
    UTV_PROJECT_UPDATE_INFO     tUpdateInfo;
    UTV_INT8                    cVersion[32];
    UTV_INT8                    cInfo[128];
    UTV_PUBLIC_UPDATE_SUMMARY   tUpdate;
    UTV_UINT32                  hStore, hImage;
    UtvImageAccessInterface     utvIAI;

    rStatus = UTV_OK;
    /* if the status isn't OK then report the error */
    switch ( cStatus )
    {
    case UTV_SNUM_TARGETING_FAILS:
        UtvMessage( UTV_LEVEL_INFO,    "MEDIA ESN TARGETING FAILS!" );
        UtvMessage( UTV_LEVEL_INFO,    "YOU MAY REMOVE THE UPDATE MEDIA NOW." );
        return;
    case UTV_OK:
        break;
    default:
        UtvMessage( UTV_LEVEL_INFO,    "COMMAND STICK FAILS: %s", UtvGetMessageString( rStatus ) );
        UtvMessage( UTV_LEVEL_INFO,    "YOU MAY REMOVE THE UPDATE MEDIA NOW." );
        return;
    }

    /* setup return strings */
    tUpdateInfo.pszVersion = cVersion;
    tUpdateInfo.uiVersionBufferLen = sizeof(cVersion);
    tUpdateInfo.pszInfo = cInfo;
    tUpdateInfo.uiInfoBufferLen = sizeof(cInfo);

    /* Open the USB store */
    if ( UTV_OK != ( rStatus = UtvPlatformStoreOpen( UTV_PLATFORM_UPDATE_USB_INDEX, UTV_PLATFORM_UPDATE_STORE_FLAG_READ,
                                                     UTV_MAX_COMP_DIR_SIZE, &hStore, NULL, NULL )))
    {
        /* tolerate already open */
        if ( UTV_ALREADY_OPEN != rStatus )
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformStoreOpen() \"%s\"", UtvGetMessageString( rStatus ) );
            return;
        }
    }

    /* get the file version of the image access interface */
    if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvFileImageAccessInterfaceGet() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* attempt to open this store which will return its image location */
    if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)hStore, &utvIAI, &hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvFileImageAccessInterfaceGet() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* stuff the image location into the update struct */
    tUpdate.hImage = hImage;

    /* get command info */
    if ( UTV_OK != ( rStatus = UtvProjectOnUpdateInfo( &tUpdate, &tUpdateInfo )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvOnProjectUpdateInfo() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* display command message */
    UtvMessage( UTV_LEVEL_INFO,    "MEDIA CONTAINS COMMANDS THAT HAVE BEEN EXECUTED:" );
    UtvMessage( UTV_LEVEL_INFO,    "   VER: %s", tUpdateInfo.pszVersion );
    UtvMessage( UTV_LEVEL_INFO,    "   INF: %s", tUpdateInfo.pszInfo );
    UtvMessage( UTV_LEVEL_INFO,    "YOU MAY REMOVE THE UPDATE MEDIA NOW." );

    UtvImageClose( hImage );
}

static void s_displayUnknown( UTV_RESULT rStatus )
{
    /* display unknown update result */
    UtvMessage( UTV_LEVEL_INFO,    "MEDIA UPDATE RESULT UNKNOWN." );
    UtvMessage( UTV_LEVEL_INFO,    "RESULT: %s.", UtvGetMessageString( rStatus ) );
}

static void s_nonblock( void )
{
    struct termios ttystate;

    tcgetattr(STDIN_FILENO, &ttystate);
    ttystate.c_lflag &= ~ICANON;
    ttystate.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

}

static int s_kbhit( void )
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

/* SIGINT_handler
    Included for demonstrating orderly shutdown of entire system *for test purposes*.
    A production system would NOT support this.
*/
static void s_SIGINT_handler (int signum)
{
    s_bExitRequest = UTV_TRUE;
}

/* SIGPIPE_handler
    The application using the Device Agent should catch and handle (or scuttle) expected
    signals.  Because the Device Agent uses network communication, and most often SSL
    network communication, the SIGPIPE signal can be raised.  For example, by an underlying
    secure network communication library such as OpenSSL.  If the application does not catch
    SIGPIPE it may be terminated by the run-time environment for expected occurrences of SIGPIPE.
*/
static void s_SIGPIPE_handler (int signum)
{
}

/* errorExit
    A helper function that releases the hardware, cleans up the Agent, and foces an application exit.
*/
static void s_errorExit( UTV_UINT32 rVal )
{
    exit( rVal );
}

/* s_usage
    Dusplay simulation key press options
*/
static void s_usage( void )
{
    UtvMessage( UTV_LEVEL_INFO, "a - abort." );
    UtvMessage( UTV_LEVEL_INFO, "b - boot system." );
    UtvMessage( UTV_LEVEL_INFO, "c - scan for stored updates." );
    UtvMessage( UTV_LEVEL_INFO, "d - display status." );
    UtvMessage( UTV_LEVEL_INFO, "f - force NOC contact." );
    UtvMessage( UTV_LEVEL_INFO, "g - test RegStore functionality." );
    UtvMessage( UTV_LEVEL_INFO, "h - test Store Device Data functionality." );
    UtvMessage( UTV_LEVEL_INFO, "i - test ULPK insertion." );
    UtvMessage( UTV_LEVEL_INFO, "k - get provisioned object." );
    UtvMessage( UTV_LEVEL_INFO, "m - factory mode." );

#ifdef UTV_LIVE_SUPPORT
    UtvMessage( UTV_LEVEL_INFO, "l - simulate Live Support session." );
#endif /* UTV_LIVE_SUPPORT */

    UtvMessage( UTV_LEVEL_INFO, "n - simulate network up." );
    UtvMessage( UTV_LEVEL_INFO, "p - simulate power off update check." );
    UtvMessage( UTV_LEVEL_INFO, "q - quit simulation." );
    UtvMessage( UTV_LEVEL_INFO, "r - reset component manifest." );
    UtvMessage( UTV_LEVEL_INFO, "s - shutdown." );

#ifdef UTV_LIVE_SUPPORT
    UtvMessage( UTV_LEVEL_INFO, "t - test Live Support connection to the NOC." );
#endif /* UTV_LIVE_SUPPORT */

    UtvMessage( UTV_LEVEL_INFO, "u - simulate USB insertion." );
    UtvMessage( UTV_LEVEL_INFO, "v - simulate DVD insertion." );
    UtvMessage( UTV_LEVEL_INFO, "w - wait for Agent to complete a task." );
    UtvMessage( UTV_LEVEL_INFO, "x - display debug info." );
    UtvMessage( UTV_LEVEL_INFO, "? - prints this help." );
    UtvMessage( UTV_LEVEL_INFO, "Press one of the above keys..." );
}

#ifdef S_APPROVE_EACH_COMPONENT
/* s_approveOptionalComponents
    An example of how to approve the components of an update for insallation.
*/
static void s_approveOptionalComponents( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_UINT32 i;

    UtvMessage( UTV_LEVEL_INFO, "   approving optional components", );

    for ( i = 0; i < pUpdate->uiNumComponents; i++ )
    {
        if ( Utv_32letoh( pUpdate->tComponents[ i ].pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_OPTIONAL )
        {
            /* Optional component found. Ask use whether it's wanted or not and set install approval */
            /* insert UI query here */
            /* set approval here.  by default the component is not approved for install */
            pUpdate->tComponents[ i ].bApprovedForInstall = UTV_TRUE;
        } else
        {
            /* always approve required components */
            pUpdate->tComponents[ i ].bApprovedForInstall = UTV_TRUE;
        }
    }
}
#endif

#ifndef UTV_FILE_BASED_PROVISIONING
/* s_requestSecurityKey
    An example of how to request a security key from the netProvision local cache.
*/
static void s_requestSecurityKey( void )
{
    UTV_RESULT    rStatus;
    UTV_UINT32    uiObjectSize, uiEncryptionType, uiStatus, uiRequestCount;
    UTV_BYTE     *pbProvisionedObject;
    UTV_INT8      szObjectIDBuffer[ 64 ]; /* length depends on objects serial number length */

    UtvMessage( UTV_LEVEL_INFO, "UtvProjectOnObjectRequest( \"%s\", \"%s\" )",
                S_SECURITY_KEY_OWNER, S_SECURITY_KEY_NAME );

    /* get the object's status */
    if ( UTV_OK != ( rStatus = UtvProjectOnObjectRequestStatus( S_SECURITY_KEY_OWNER, S_SECURITY_KEY_NAME,
                                                                &uiStatus, &uiRequestCount )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvProjectOnObjectRequestStatus() fails with error \"%s\"",
                    UtvGetMessageString( rStatus ) );
        return;
    }

    /* if status has changed say so */
    if ( UTV_PROVISION_STATUS_MASK_CHANGED & uiStatus )
        UtvMessage( UTV_LEVEL_INFO, "Object has changed since it was last requested." );
    else
        UtvMessage( UTV_LEVEL_INFO, "Object has NOT changed since it was last requested." );

    /* get the object's length */
    if ( UTV_OK != ( rStatus = UtvProjectOnObjectRequestSize( S_SECURITY_KEY_OWNER, S_SECURITY_KEY_NAME,
                                                              &uiObjectSize )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvProjectOnObjectRequestSize() fails with error \"%s\"",
                    UtvGetMessageString( rStatus ) );
        return;
    }
    
    /* allocate space for it */
    if ( NULL == ( pbProvisionedObject = UtvMalloc( uiObjectSize )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc( %u bytes ) fails", uiObjectSize );
        return;
    }
    
    /* get the object itself. */
    if ( UTV_OK != ( rStatus = UtvProjectOnObjectRequest( S_SECURITY_KEY_OWNER, S_SECURITY_KEY_NAME,
                                                          szObjectIDBuffer, sizeof( szObjectIDBuffer ),
                                                          pbProvisionedObject, uiObjectSize,
                                                          &uiObjectSize, &uiEncryptionType )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvInternetProvisionerGetObject() fails with error \"%s\"",
                    UtvGetMessageString( rStatus ) );
        return;
    }

    UtvMessage( UTV_LEVEL_INFO, "Provisioned object is %d bytes in length, has an object ID of \"%s\", and an encryption type of 0x%04X",
                uiObjectSize, szObjectIDBuffer, uiEncryptionType );

    /* dump object */
    UtvDataDump( pbProvisionedObject, uiObjectSize );

    /* The key could then be returned to the caller.  Because it's stored on the heap the buffer could be freed by the caller
       or a FreeObject() function could be provided. */
    UtvFree( pbProvisionedObject );
}
#endif

/* s_waitForAgent
    An example of how to use UtvProjectOnTestAgentActive() to wait for the Agent to complete what it's doing.
*/
static void s_waitForAgent( void )
{
    UTV_RESULT    rStatus;

    UtvMessage( UTV_LEVEL_INFO, "UtvProjectOnTestAgentActive( %d msec timeout )", S_WAIT_LEN_IN_MSECS );

    /* wait using the canned duration defined by S_WAIT_LEN_IN_MSECS */
    rStatus = UtvProjectOnTestAgentActive( S_WAIT_LEN_IN_MSECS );

    UtvMessage( UTV_LEVEL_INFO, "UtvProjectOnTestAgentActive() returns: %s",
                    UtvGetMessageString( rStatus ) );

}

#ifdef UTV_LIVE_SUPPORT
/* s_liveSupport
    Simulates a live support session.
*/
static void s_liveSupport( void )
{
  UTV_RESULT result;

  if ( !UtvProjectOnLiveSupportIsSessionActive() )
  {
    UtvLiveSupportSessionInitiateInfo1 sessionInitiateInfo;
    UtvLiveSupportSessionInfo1 sessionInfo;

    UtvMemset( &sessionInitiateInfo, 0x00, sizeof( sessionInitiateInfo ) );

    /*
     * NOTE: Setting the allowSessionCodeBypass flag to UTV_TRUE allows support technicians
     *       to lookup this session without using the generated session code.  Use this only
     *       when the user will not be able to view the session code and read it to the support
     *       technician.
     */

    sessionInitiateInfo.allowSessionCodeBypass = UTV_FALSE;

    result = 
      UtvProjectOnLiveSupportInitiateSession1(
        &sessionInitiateInfo,
        &sessionInfo );
  }
  else
  {
    result = UtvProjectOnLiveSupportTerminateSession();
  }
}

/* s_liveSupportTestConnection
    Test Live Support connection to the NOC.
*/
static void s_liveSupportTestConnection( void )
{
  UTV_RESULT result;

  result = UtvProjectOnLiveSupportTestConnection();
}
#endif /* UTV_LIVE_SUPPORT */

static void s_securityUnitTest( void )
{
    UtvMessage( UTV_LEVEL_INFO, "Security Unit Test Result: %s", UtvGetMessageString( UtvPlatformSecureWriteReadTest() ));
}

static void s_insertULPKTest( void )
{
	UTV_UINT32 uiFileHandle, rStatus, uiBytesRead;
	UTV_BYTE   ubULPK[ S_ULPK_SIZE ];
	
	/* check file size */
    if ( UTV_OK != ( rStatus = UtvPlatformFileGetSize( S_TEST_ULPK_FNAME, &uiBytesRead )))
    {
		UtvMessage( UTV_LEVEL_ERROR,
					"UtvPlatformFileGetSize( %s ) fails: ", S_TEST_ULPK_FNAME, UtvGetMessageString( rStatus ) );
        return;
    }

	if ( S_ULPK_SIZE != uiBytesRead )
	{
		UtvMessage( UTV_LEVEL_ERROR,
					"File \"%s\" is not %d bytes in length, it is %d bytes long", S_TEST_ULPK_FNAME, uiBytesRead );
		return;
	}

    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( S_TEST_ULPK_FNAME, "rb", &uiFileHandle )))
    {
		UtvMessage( UTV_LEVEL_ERROR,
					"UtvPlatformFileOpen( %s ) fails: ", S_TEST_ULPK_FNAME, UtvGetMessageString( rStatus ) );
		return;
    }

    /* attempt to read ULPK */
    if ( UTV_OK != ( rStatus = UtvPlatformFileReadBinary( uiFileHandle, 
                                                          ubULPK, S_ULPK_SIZE, &uiBytesRead )))
    {
		UtvMessage( UTV_LEVEL_ERROR,
					"UtvPlatformFileReadBinary( %s ) fails: ", S_TEST_ULPK_FNAME, UtvGetMessageString( rStatus ) );
		return;
    } 

    UtvPlatformFileClose( uiFileHandle );

	if ( UTV_OK != ( rStatus = UtvPlatformSecureInsertULPK( ubULPK )))
	{
		UtvMessage( UTV_LEVEL_ERROR,
					"UtvPlatformSecureInsertULPK( %s ) fails: ", S_TEST_ULPK_FNAME, UtvGetMessageString( rStatus ) );
		return;		
	}

	UtvMessage( UTV_LEVEL_INFO, "UtvPlatformSecureInsertULPK() OK!" );

	return;
}

static void s_regStoreTest( void )
{
  UTV_RESULT result;
  UTV_INT8* apszRegStoreValues[ 10 ];

  /* mock up some CEM status values */
  apszRegStoreValues[ 0 ] = "1";
  apszRegStoreValues[ 1 ] = "random stuff";
  apszRegStoreValues[ 2 ] = "like error code 14 from Rhapsody";
  apszRegStoreValues[ 3 ] = "or libcurl error 58 from Amazon";
  apszRegStoreValues[ 4 ] = "or owner-name not found";
  apszRegStoreValues[ 5 ] = "how about network connection error";
  apszRegStoreValues[ 6 ] = "or even more and more things";
  apszRegStoreValues[ 7 ] = "like number 1 2 3 4 5 6 7 8 9";
  apszRegStoreValues[ 8 ] = "or more and more fields: this is the ninth field";
  apszRegStoreValues[ 9 ] = "this is the tenth";
  apszRegStoreValues[ 10 ] = "this is the eleventh";

  /*
   * Inform the NOC of our status change.
   */

  result = UtvPublicInternetManageRegStore( 10, apszRegStoreValues );

  if ( UTV_OK != result )
  {
    UtvMessage(
      UTV_LEVEL_ERROR,
      "UtvPublicInternetManageRegStore() fails with error \"%s\"",
      UtvGetMessageString( result ) );
  }
}

#ifdef UTV_DEVICE_WEB_SERVICE
static void s_storeDeviceDataTest( void )
{
  UtvStoreDeviceData_MetaData_t metaData;
  time_t tCurrentTime;
  struct tm* ptmTime;
  UTV_BYTE szCurrentTime[21];
  int idxRandomDigits;
  char* szRandomDigits = NULL;
  int randomDigits;
  int numBytes;

  tCurrentTime = UtvTime( NULL );
  ptmTime = UtvGmtime( &tCurrentTime );
  strftime( (char*)szCurrentTime, 21, "%Y-%m-%dT%H:%M:%SZ", ptmTime );

  randomDigits = rand() % 7;

  szRandomDigits = (char*) calloc( randomDigits + 1, 1 );

  for ( idxRandomDigits = 0; idxRandomDigits < randomDigits; idxRandomDigits++ )
  {
    szRandomDigits[idxRandomDigits] = rand() % 9 + 48;
  }

  numBytes = atoi( szRandomDigits );

  free( szRandomDigits );

  /* ignore the streamID field - othwerwise, change these values as needed for testing */
  UtvMemset( &metaData, 0x00, sizeof( metaData ) );
  metaData.version = UTV_METADATA_VERSION_1;
  metaData.type = Generic;
  metaData.encryptOnHost = UTV_FALSE;
  metaData.compressOnHost = UTV_FALSE;
  metaData.createTime = tCurrentTime;

  /* Test with empty name and notes */
  s_storeDeviceDataTest_Fixed( &metaData, 253 );

  /* Test with a long name and note */
  UtvMemset( metaData.name, 'A', sizeof( metaData.name ) - 1 );
  UtvMemset( metaData.note, 'B', sizeof( metaData.note ) - 1 );
  s_storeDeviceDataTest_Fixed( &metaData, 253 );

  /* Setup generic name and notes for the remaining tests */
  UtvMemset( metaData.name, 0x00, sizeof( metaData.name ) );
  UtvMemset( metaData.note, 0x00, sizeof( metaData.note ) );
  UtvStrnCopy(
    metaData.name,
    sizeof( metaData.name ),
    (UTV_BYTE*)__FUNCTION__,
    UtvStrlen( (UTV_BYTE*)__FUNCTION__ ) );
  UtvStrnCopy(
    metaData.note,
    sizeof( metaData.note ),
    szCurrentTime,
    UtvStrlen( szCurrentTime ) );

  /* Run some tests that are expected to be successful. */
  s_storeDeviceDataTest_Fixed( &metaData, 253 );
  s_storeDeviceDataTest_Fixed( &metaData, UTV_STOREDEVICEDATA_APPEND_SIZE_MAX - 1 );
  s_storeDeviceDataTest_Fixed( &metaData, UTV_STOREDEVICEDATA_APPEND_SIZE_MAX );
  s_storeDeviceDataTest_Fixed( &metaData, UTV_STOREDEVICEDATA_APPEND_SIZE_MAX + 1 );
  s_storeDeviceDataTest_Random( &metaData, 100 * 1024 );

  /* Depending on value in "numBytes" this may or may not cause failure */
  s_storeDeviceDataTest_Random( &metaData, numBytes );

  /* These should fail */
  s_storeDeviceDataTest_Random( &metaData, 100 * 1024 + 1 );
}

/**
 **********************************************************************************************
 *
 * This function is used to send a specified number of bytes.
 *
 * This function differs primarily from s_storeDeviceDataTest_Random() in that it uses
 * the UtvInternetStoreDeviceData_BlockSend function rather than invoking each transfer
 * function individually
 *
 * @param   pMetaData       Holds description of transfer to take place 
 *
 * @param   numBytes        Number of bytes to transfer
 *
 *****************************************************************************************************
*/

static void s_storeDeviceDataTest_Fixed(
  UtvStoreDeviceData_MetaData_t* pMetaData,
  UTV_UINT32 uiDataLength )
{
  UTV_RESULT result;
  UTV_BYTE *dataToSend;
  UTV_TIME tCurrentTime;
  int index, indexMax;

  dataToSend = (UTV_BYTE *)UtvMalloc( uiDataLength );
  if ( NULL == dataToSend )
  {
    UtvMessage( UTV_LEVEL_INFO, "%s Malloc failed",  __FUNCTION__);
    return;
  }
 
  index = 0;
  indexMax = uiDataLength - 1; /* make room for a null terminator */
  tCurrentTime = UtvTime( NULL );

  /* if there's room, squirrel away the create time in the data */
  if ( uiDataLength > 21 ) /* if data block can hold time format */
  {
    struct tm* ptmTime;

    ptmTime = UtvGmtime( &tCurrentTime );
    strftime( (char *)dataToSend, 21, "%Y-%m-%dT%H:%M:%SZ", ptmTime );
    index = 20;
  }

  for ( ; index < indexMax; index++)
  {
    char offset;

    offset = index % 128;
    if ((offset < 32) || (offset == 127))
      dataToSend[index] = '.';
    else
      dataToSend[index] = offset;
  }

  dataToSend[index] = '\0';

  result = UtvInternetStoreDeviceData_BlockSend( pMetaData, dataToSend, uiDataLength );

  if ( UTV_OK == result )
  {
    UtvMessage(
      UTV_LEVEL_INFO,
      "%s, Stored Device Data at (%s) with %d bytes",
      __FUNCTION__,
      pMetaData->streamID,
      uiDataLength );
  }
  else
  {
    UtvMessage(
      UTV_LEVEL_INFO,
      "%s, Failed to store device data with %d bytes, %s",
      __FUNCTION__,
      uiDataLength,
      UtvGetMessageString( result ) );
  }

  UtvFree( dataToSend );
}

/**
 **********************************************************************************************
 *
 * This function is used to send a specified number of bytes under various processing
 * options. The present options are defined but not limited to the four compression and
 * encryption possibilities.
 *
 * This function differs primarily from s_storeDeviceDataTest_Fixed() in that it does not use
 * the UtvInternetStoreDeviceData_BlockSend function but rather invokes each transfer
 * function individually
 *
 * @param   pMetaData       Holds description of transfer to take place 
 *
 * @param   numBytes        Number of bytes to transfer
 *
 *****************************************************************************************************
*/

static void s_storeDeviceDataTest_Random(
  UtvStoreDeviceData_MetaData_t* pMetaData,
  UTV_UINT32 numBytes )
{
  UTV_RESULT result = UTV_OK;
  UTV_BOOL abort = UTV_FALSE;
  int idxDeviceData;
  UTV_UINT32 sentBytes;
  UTV_UINT32 sendBytes;
  int randomByte;
  time_t tCurrentTime;
  struct tm* ptmTime;
  char szCurrentTime[22];
  char* deviceData;
  int idxEntry;

  tCurrentTime = UtvTime( NULL );
  ptmTime = UtvGmtime( &tCurrentTime );
  strftime( szCurrentTime, 22, "%Y-%m-%dT%H:%M:%SZ - ", ptmTime );

  deviceData = (char*)calloc( numBytes, 1 );

  if ( numBytes >= 22 )
  {
    strftime( deviceData, numBytes, "%Y-%m-%dT%H:%M:%SZ - ", ptmTime );
  }

  for ( idxDeviceData = strlen( deviceData ); idxDeviceData < numBytes; idxDeviceData++ )
  {
    randomByte = rand() % 93 + 33;
    deviceData[idxDeviceData] = randomByte;
  }

  for ( idxEntry = 0; idxEntry < 4; idxEntry++ )
  {
    switch ( idxEntry )
    {
      case 0:
        pMetaData->compressOnHost = UTV_FALSE;
        pMetaData->encryptOnHost = UTV_FALSE;
        break;
       
      case 1:
        pMetaData->compressOnHost = UTV_TRUE;
        pMetaData->encryptOnHost = UTV_FALSE;
        break;
       
      case 2:
        pMetaData->compressOnHost = UTV_FALSE;
        pMetaData->encryptOnHost = UTV_TRUE;
        break;
       
      case 3:
        pMetaData->compressOnHost = UTV_TRUE;
        pMetaData->encryptOnHost = UTV_TRUE;
        break;
    }
/* There are two types of store data operations.  File and block. 
   By default test the file interface.  Define _STORE_DATA_BLOCK_SEND to 
   test the block-like instead. */
#ifndef _STORE_DATA_BLOCK_SEND
    result = UtvInternetStoreDeviceData_Open( pMetaData );
    if ( UTV_OK == result )
    {
        sentBytes = 0;
        sendBytes = 0;

        while ( UTV_OK == result )
        {
            sentBytes += sendBytes;

            if ( numBytes == sentBytes )
            {
                break;
            }

            sendBytes = numBytes - sentBytes;

            if ( sendBytes > UTV_STOREDEVICEDATA_APPEND_SIZE_MAX - 1 )
            {
                sendBytes = UTV_STOREDEVICEDATA_APPEND_SIZE_MAX - 1;
            }

            result =
                UtvInternetStoreDeviceData_Append(
                                                  pMetaData,
                                                  (UTV_BYTE*)(deviceData + sentBytes),
                                                  sendBytes );
        }

        if ( UTV_OK == result )
        {
            result = UtvInternetStoreDeviceData_Close( pMetaData );

            if ( UTV_OK != result )
            {
                abort = UTV_TRUE;
            }
            else
            {
                UtvMessage(
                           UTV_LEVEL_INFO,
                           "%s, Stored Device Data at (%s) with %d bytes",
                           __FUNCTION__,
                           pMetaData->streamID,
                           numBytes );
            }
        }
        else
        {
            abort = UTV_TRUE;
        }
    }
    else
    {
        abort = UTV_TRUE;
    }

    if ( UTV_OK != result )
    {
        UtvMessage(
                   UTV_LEVEL_ERROR,
                   "%s() \"%s\"",
                   __FUNCTION__,
                   UtvGetMessageString( result ) );
    }

    if ( UTV_TRUE == abort )
    {
        result = UtvInternetStoreDeviceData_Abort( pMetaData );

        UtvMessage(
                   UTV_LEVEL_INFO,
                   "%s, Failed to store device data with %d bytes...",
                   __FUNCTION__,
                   numBytes );
    }
#else
    result = UtvInternetStoreDeviceData_BlockSend( pMetaData,
                                                   (UTV_BYTE*)deviceData,
                                                   numBytes );
    if ( UTV_OK == result )
    {
        UtvMessage(
                   UTV_LEVEL_INFO,
                   "%s, Stored Device Data at (%s) with %d bytes",
                   __FUNCTION__,
                   pMetaData->streamID,
                   numBytes );
    }
    else
    {
        UtvMessage(
                   UTV_LEVEL_INFO,
                   "%s, Failed to store device data with %d bytes, %s",
                   __FUNCTION__,
                   numBytes,
                   UtvGetMessageString( result ) );
    }
#endif
  }

  free( deviceData );

  /*
   * Reset the compression and encryption values to a known starting state.
   */

  pMetaData->compressOnHost = UTV_FALSE;
  pMetaData->encryptOnHost = UTV_FALSE;
}
#endif
