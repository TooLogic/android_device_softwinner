/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *               Copyright (C) 2009-2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Primary Device Agent adaptation header file.
 *
 * @file      project-generic.h
 *
 * @author    Bob Taylor
 *
 * @version   $Revision: 5186 $
 *            $Author: ryan.melville $
 *            $Date: 2011-07-11 14:17:44 -0500 (Mon, 11 Jul 2011) $
 *
 **********************************************************************************************
 */

/*
 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      12/20/2011  Added UTV_FILE_BASED_PROVISIONING ifdef to eliminate object request mechanism
                              when using UtvPlatformOnProvisioned().
  Bob Taylor      11/30/2011  Introduced UtvProjectOnEarlyBoot() proto.  Removed UtvProjectOnBoot() proto.
  Bob Taylor      12/16/2010  Added proto for UtvProjectOnObjectRequestStatus().
  Bob Taylor      10/01/2010  Added proto for UtvProjectOnForceNOCContact().
  Bob Taylor      06/30/2010  Added proto for UtvProjectOnDebugInfo().
  Bob Taylor      02/09/2010  Moved file name defines to here from platform-file.h
  Bob Taylor      12/07/2009  New UTV_PROJECT_SYSEVENT_GET_SCHEDULE event.
  Bob Taylor      11/13/2009  Created.
*/

#ifndef __UTV_PROJECT_H__
#define __UTV_PROJECT_H__

/*
*****************************************************************************************************
**
** PROJECT DEFINES
**
*****************************************************************************************************
*/

/* Define the TS test broadcast input file here. */
#define UTV_PLATFORM_TEST_TS_FNAME    "Stream.ts"

/* Specify the name of the redirect file that contains the name of the actual
   UTV file containing a USB update. */
#define UTV_PLATFORM_UPDATE_REDIRECT_FNAME "updatelogic.txt"

/* Specify a path where files can be stored in a secure fashion.  Typically tmpfs. */
#define UTV_PLATFORM_SECURE_SAVE_FNAME    "tmp.secure"

#define UTV_PLATFORM_COMP_MANIFEST_FNAME     "component.manifest"

#define UTV_PLATFORM_UPDATE_FILE_PATH        ""

extern UTV_INT8 UTV_PLATFORM_PERSISTENT_PATH[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_ULID_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_FACTORY_PATH[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_ULPK_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_SERIAL_NUMBER_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_PROVISIONER_PATH[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_READ_ONLY_PATH[256];
extern UTV_INT8 UTV_PLATFORM_INTERNET_SSL_CA_CERT_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_UPDATE_STORE_0_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_UPDATE_STORE_1_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_ACTIVE_COMP_MANIFEST_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_BACKUP_COMP_MANIFEST_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_PERSISTENT_STORAGE_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_KEYSTORE_FNAME[256];
extern UTV_INT8 UTV_PLATFORM_REGSTORE_STORE_AND_FWD_FNAME[256];

#define DEFAULT_PERSISTENT_PATH "/sdcard/ULI/persistent/"
//#define DEFAULT_FACTORY_PATH "/sdcard/ULI/factory/"
//#define DEFAULT_READ_ONLY_PATH "/sdcard/ULI/read-only/"
#define DEFAULT_FACTORY_PATH "/mnt/private/ULI/factory/"
#define DEFAULT_READ_ONLY_PATH "/mnt/private/ULI/read-only/"
#define DEFAULT_INSTALL_PATH "/sdcard/"

/* END PATH AND FILE NAME DEFINITIONS */

/* system events */
#define UTV_PROJECT_SYSEVENT_USB_SCAN_RESULT        0
#define UTV_PROJECT_SYSEVENT_DVD_SCAN_RESULT        1
#define UTV_PROJECT_SYSEVENT_STORE_SCAN_RESULT        2
#define UTV_PROJECT_SYSEVENT_NETWORK_SCAN_RESULT    3

#define UTV_PROJECT_SYSEVENT_DOWNLOAD                10
#define UTV_PROJECT_SYSEVENT_MEDIA_INSTALL            11

/* status mask flags */
#define UTV_PROJECT_STATUS_MASK_REGISTERED        0x0001
#define UTV_PROJECT_STATUS_MASK_FACTORY_TEST_MODE 0x0002
#define UTV_PROJECT_STATUS_MASK_FIELD_TEST_MODE   0x0004
#define UTV_PROJECT_STATUS_MASK_LOOP_TEST_MODE    0x0008

/*
*****************************************************************************************************
**
** PROJECT TYPDEFS
**
*****************************************************************************************************
*/
typedef struct
{
    UTV_INT8  *pszESN;
    UTV_UINT32 uiESNBufferLen;
    UTV_UINT32 uiULPKIndex;
    UTV_INT8  *pszQueryHost;
    UTV_UINT32 uiQueryHostBufferLen;
    UTV_INT8  *pszVersion;
    UTV_UINT32 uiVersionBufferLen;
    UTV_INT8  *pszInfo;
    UTV_UINT32 uiInfoBufferLen;
    UTV_UINT32 uiOUI;
    UTV_UINT16 usModelGroup;
    UTV_UINT16 usHardwareModel;
    UTV_UINT16 usSoftwareVersion;
    UTV_UINT16 usModuleVersion;
    UTV_UINT16 usNumObjectsProvisioned;
    UTV_UINT16 usStatusMask;
    UTV_UINT32 uiErrorCount;
    UTV_UINT32 uiLastErrorCode;
    UTV_INT8  *pszLastError;
    UTV_UINT32 uiLastErrorBufferLen;
    UTV_TIME   tLastErrorTime;
} UTV_PROJECT_STATUS_STRUCT;

typedef struct
{
    UTV_INT8  *pszVersion;
    UTV_UINT32 uiVersionBufferLen;
    UTV_INT8  *pszInfo;
    UTV_UINT32 uiInfoBufferLen;
} UTV_PROJECT_UPDATE_INFO;

typedef struct
{
    UTV_INT8   *pszThreadName;
    UTV_UINT32  uiEventType;
    UTV_BOOL    bDeliveryFile;
    UTV_BOOL    bDeliveryInternet;
    UTV_BOOL    bDeliveryBroadcast;
    UTV_BOOL    bStored;
    UTV_BOOL    bFactoryMode;
} UTV_PROJECT_GET_SCHEDULE_REQUEST;

typedef struct
{
    UTV_INT8                      *pszThreadName;
    UTV_PUBLIC_UPDATE_SUMMARY  *pUpdate;
    UTV_BOOL                    bFactoryMode;
} UTV_PROJECT_DOWNLOAD_REQUEST;

/*
*****************************************************************************************************
**
** PROJECT PROTOS
**
*****************************************************************************************************
*/

void UtvProjectOnEarlyBoot( void );
void UtvProjectOnShutdown( void );
UTV_RESULT UtvProjectOnTestAgentActive( UTV_UINT32 msecsToWait );
UTV_RESULT UtvProjectOnScanForNetUpdate( UTV_PUBLIC_SCHEDULE *pSchedule );
UTV_RESULT UtvProjectOnScanForStoredUpdate( UTV_PUBLIC_SCHEDULE *pSchedule );
UTV_RESULT UtvProjectOnUpdate( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
void UtvProjectOnAbort( void );
UTV_RESULT UtvProjectOnUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_PROJECT_UPDATE_INFO *pInfo );
UTV_RESULT UtvProjectOnStatusRequested( UTV_PROJECT_STATUS_STRUCT *pStatus );
void UtvProjectOnDebugInfo( void );
void UtvProjectOnRegisterCallback( UTV_PUBLIC_CALLBACK pCallback );
UTV_RESULT UtvProjectOnResetComponentManifest( void );
UTV_RESULT UtvProjectOnFactoryMode( void );
UTV_RESULT UtvProjectManageRegStore( UTV_UINT32 uiNumStrings, UTV_INT8 *apszString[] );

#ifdef UTV_DELIVERY_FILE
UTV_RESULT UtvProjectOnUSBInsertionBlocking( UTV_INT8 *pszPath );
UTV_RESULT UtvProjectOnDVDInsertion( void );
UTV_RESULT UtvProjectOnInstallFromMedia( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
#endif

#ifdef UTV_DELIVERY_INTERNET
void UtvProjectOnNetworkUp( void );
void UtvProjectOnForceNOCContact( void );
#ifndef UTV_FILE_BASED_PROVISIONING
UTV_RESULT UtvProjectOnObjectRequestStatus( UTV_INT8 *pszOwner, UTV_INT8 *pszName, 
                                            UTV_UINT32 *puiStatus, UTV_UINT32 *puiRequestCount );
UTV_RESULT UtvProjectOnObjectRequestSize( UTV_INT8 *pszOwner, UTV_INT8 *pszName, 
                                          UTV_UINT32 *puiObjectSize);
UTV_RESULT UtvProjectOnObjectRequest( UTV_INT8 *pszOwner, UTV_INT8 *pszName, 
                                      UTV_INT8 *pszObjectIDBuffer, UTV_UINT32 uiObjectIDBufferSize, 
                                      UTV_BYTE *pubBuffer, UTV_UINT32 uiBufferSize, 
                                      UTV_UINT32 *puiObjectSize, UTV_UINT32 *puiEncryptionType );
#endif
UTV_RESULT UtvPlatformGetEtherMAC( UTV_INT8 *pszMAC, UTV_UINT32 uiBufferSize );
UTV_RESULT UtvPlatformGetWiFiMAC( UTV_INT8 *pszMAC, UTV_UINT32 uiBufferSize );
#endif

UTV_INT8 *UtvPlatformGetPersistentPath( void );

/* Android specific helper functions */
void UtvProjectSetPeristentPath(UTV_INT8 * path);
void UtvProjectSetFactoryPath(UTV_INT8 * path);
void UtvProjectSetReadOnlyPath(UTV_INT8 * path);
void UtvProjectSetUpdateInstallPath(UTV_INT8 * path);
UTV_INT8 *UtvProjectGetUpdateInstallPath(void);
void UtvProjectSetUpdateReadyToInstall(UTV_BOOL updateAv);
UTV_BOOL UtvProjectGetUpdateReadyToInstall(void);
UTV_PUBLIC_SCHEDULE *UtvProjectGetUpdateSchedule(void);
void UtvProjectSetUpdateSchedule(UTV_PUBLIC_SCHEDULE *pCurrentUpdateScheduled);

#ifdef UTV_LOCAL_SUPPORT
UTV_RESULT UtvProjectOnLocalSupportGetDeviceInfo( UTV_BYTE** ppszDeviceInfo );
UTV_RESULT UtvProjectOnLocalSupportGetNetworkInfo( UTV_BYTE** ppszNetworkInfo );
UTV_RESULT UtvProjectOnLocalSupportNetworkDiagnostics( UTV_BYTE** ppszNetworkDiagnostics );
#endif /* UTV_LOCAL_SUPPORT */

#ifdef UTV_LIVE_SUPPORT
static const UTV_BYTE pszLiveSupportCustomDiagnosticSampleDescription[] =
  "{"
  "  \"TYPE\" :         \"DIAGNOSTIC_COMMANDS\","
  "  \"ID\" :           \"FFFFFF_CUSTOM_DIAGNOSTIC_SAMPLE\","
  "  \"NAME\" :         \"Custom Diagnostic Sample\","
  "  \"DESCRIPTION\" :  \"This is a sample custom diagnostic\","
  "  \"ORDER\" :        -1,"
  "  \"PERMISSIONS\" :  \"\","
  "  \"INPUTS\" :       [],"
  "  \"OUTPUTS\" : "
  "  ["
  "    {"
  "      \"ID\" :           \"RESPONSE_DATA\","
  "      \"NAME\" :         \"Result\","
  "      \"DESCRIPTION\" :  \"Result of command\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"HTML\""
  "    }"
  "  ]"
  "}";

static const UTV_BYTE pszLiveSupportCustomToolSampleDescription[] =
  "{"
  "  \"TYPE\" :         \"TOOL_COMMANDS\","
  "  \"ID\" :           \"FFFFFF_CUSTOM_TOOL_SAMPLE\","
  "  \"NAME\" :         \"Custom Tool Sample\","
  "  \"DESCRIPTION\" :  \"This is a sample custom tool\","
  "  \"ORDER\" :        -1,"
  "  \"PERMISSIONS\" :  \"\","
  "  \"INPUTS\" : "
  "  ["
  "    {"
  "      \"ID\" :           \"STATIC_INPUT_STRING\","
  "      \"NAME\" :         \"Static String Input\","
  "      \"DESCRIPTION\" :  \"Test input for a read-only string.\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"STRING\","
  "      \"DEFAULT\" :      \"Test Default (Read-Only)\","
  "      \"REQUIRED\" :     true,"
  "      \"VISIBILITY\" :   true,"
  "      \"READONLY\" :     true"
  "    },"
  "    {"
  "      \"ID\" :           \"TEST_INPUT_STRING\","
  "      \"NAME\" :         \"String Input\","
  "      \"DESCRIPTION\" :  \"Test input for a string.\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"STRING\","
  "      \"DEFAULT\" :      \"Test Default\","
  "      \"REQUIRED\" :     true,"
  "      \"VISIBILITY\" :   true,"
  "      \"READONLY\" :     false"
  "    },"
  "    {"
  "      \"ID\" :           \"TEST_INPUT_NUMERIC\","
  "      \"NAME\" :         \"Numeric Input\","
  "      \"DESCRIPTION\" :  \"Test input for a numeric value.\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"NUMERIC\","
  "      \"REQUIRED\" :     true,"
  "      \"VISIBILITY\" :   true,"
  "      \"READONLY\" :     false"
  "    },"
  "    {"
  "      \"ID\" :           \"TEST_INPUT_BOOL\","
  "      \"NAME\" :         \"Boolean Input\","
  "      \"DESCRIPTION\" :  \"Test input for a boolean value.\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"BOOL\","
  "      \"REQUIRED\" :     true,"
  "      \"VISIBILITY\" :   true,"
  "      \"READONLY\" :     false"
  "    },"
  "    {"
  "      \"ID\" :           \"TEST_INPUT_LIST\","
  "      \"NAME\" :         \"List Input\","
  "      \"DESCRIPTION\" :  \"Test input for a list value.\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"LIST\","
  "      \"DEFAULT\" :      \"Test Value 2\","
  "      \"VALUES\" :       [\"Test Value 1\", \"Test Value 2\", \"Test Value 3\"],"
  "      \"REQUIRED\" :     true,"
  "      \"VISIBILITY\" :   true,"
  "      \"READONLY\" :     false"
  "    }"
  "  ],"
  "  \"OUTPUTS\" : "
  "  ["
  "    {"
  "      \"ID\" :           \"RESPONSE_DATA\","
  "      \"NAME\" :         \"Result\","
  "      \"DESCRIPTION\" :  \"Result of command\","
  "      \"ORDER\" :        -1,"
  "      \"TYPE\" :         \"JSON\""
  "    }"
  "  ]"
  "}";

UTV_RESULT UtvProjectOnLiveSupportTestConnection( void );
UTV_BOOL UtvProjectOnLiveSupportIsSessionActive( void );

/* DEPRECATED - use UtvProjectOnLiveSupportInitiateSession1 */
UTV_RESULT UtvProjectOnLiveSupportInitiateSession( UTV_BYTE* pbSessionCode,
                                                   UTV_UINT32 uiSessionCodeSize );

UTV_RESULT UtvProjectOnLiveSupportInitiateSession1(
  UtvLiveSupportSessionInitiateInfo1* sessionInitiateInfo,
  UtvLiveSupportSessionInfo1* sessionInfo );

UTV_RESULT UtvProjectOnLiveSupportTerminateSession( void );
UTV_RESULT UtvProjectOnLiveSupportGetSessionCode( UTV_BYTE* pbSessionCode,
                                                  UTV_UINT32 uiSessionCodeSize );
#endif /* UTV_LIVE_SUPPORT */

#endif /* __UTV_PROJECT_H__ */
