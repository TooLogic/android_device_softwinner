/* utv-public.h: UpdateTV Public Entry Points and Definitions

 Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who           Date        Edit
  Bob Taylor    07/18/2011  UtvPublicAgentEarlyInit() added.
  Bob Taylor    11/05/2009  New bCompatible flag in UTV_PUBLIC_COMPONENT_SUMMARY and
                            UtvPublicAgentInit() proto.
  Bob Taylor    04/01/2009  Created.
*/

#ifndef __UTV_PUBLIC_H__
#define __UTV_PUBLIC_H__

#define UTV_PUBLIC_DELIVERY_MODE_BROADCAST   1
#define UTV_PUBLIC_DELIVERY_MODE_INTERNET    2
#define UTV_PUBLIC_DELIVERY_MODE_FILE        3

/* Typedef for returning operation results via a callback function. */
typedef struct 
{
   UTV_UINT32 iToken;
   UTV_RESULT rOperationResult;
   void *     pOperationResultData;
} UTV_PUBLIC_RESULT_OBJECT;

/* Typedef for callback function pointers. */
typedef void (*UTV_PUBLIC_CALLBACK)(UTV_PUBLIC_RESULT_OBJECT *);

#define UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS        32
#define UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS        128

/* Component identifying structure. */
typedef struct {
    UtvCompDirCompDesc          *pCompDesc;
    UTV_BOOL                     bNew;
    UTV_BOOL                     bStored;
    UTV_BOOL                     bApprovedForStorage;
    UTV_BOOL                     bApprovedForInstall;
    UTV_BOOL                     bCompatible;
    UTV_BOOL                     bCopied;
} UTV_PUBLIC_COMPONENT_SUMMARY;

/* Update identifying structure. */
typedef struct {
    UTV_UINT16                   usModuleVersion;
    UTV_UINT32                   uiDeliveryMode;
    UTV_UINT32                   hStore;
    UTV_UINT32                   hImage;
    UTV_UINT32                   uiNumTimeSlots;
    UTV_UINT32                   uiSecondsToStart[UTV_PLATFORM_MAX_SCHEDULE_SLOTS];
    UTV_UINT32                   uiNextQuerySeconds;
    UTV_INT8                     szHostName[UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS];
    UTV_INT8                     szFilePath[UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS];
    UTV_UINT32                   uiBroadcastDuration;
    UTV_UINT32                   uiDownloadSize;
    UTV_UINT32                   uiBlockSize;
    UTV_BYTE                     aubModuleEncounteredFlag[UTV_PLATFORM_MAX_MODULES];
    UTV_UINT32                   uiFrequency;
    UTV_UINT32                   uiPid;
    UTV_UINT32                   uiNumComponents;
    UTV_PUBLIC_COMPONENT_SUMMARY tComponents[UTV_PLATFORM_MAX_COMPONENTS];
} UTV_PUBLIC_UPDATE_SUMMARY;

/* Master schedule containing multiple updates */
typedef struct {
    UTV_UINT32                   uiNumUpdates;
    UTV_PUBLIC_UPDATE_SUMMARY    tUpdates[UTV_PLATFORM_MAX_UPDATES];
} UTV_PUBLIC_SCHEDULE;

/* Define the global UpdateTV application states
*/
typedef enum {
    UTV_PUBLIC_STATE_SCAN,         /* the current (or next) state is Scan Event */
    UTV_PUBLIC_STATE_DOWNLOAD,     /* the current (or next) state is Download Event */
    UTV_PUBLIC_STATE_DOWNLOAD_DONE,/* the current (or next) state is a Download Complete Event */
    UTV_PUBLIC_STATE_ABORT,        /* abort the current operation and return to sleep */
    UTV_PUBLIC_STATE_EXIT,         /* exit the UpdateTV application (system shutdown) */
    UTV_PUBLIC_STATE_COUNT
} UTV_PUBLIC_STATE;

/* Define the global UpdateTV application diagnostic sub-states
*/
typedef enum {
    UTV_PUBLIC_SSTATE_SLEEPING,        /* sleeping for some period of time */
    UTV_PUBLIC_SSTATE_SCANNING_DSI,    /* scanning for a DSI */
    UTV_PUBLIC_SSTATE_SCANNING_DII,    /* scanning for a DII */
    UTV_PUBLIC_SSTATE_SCANNING_DDB,    /* scanning for DDBs */
    UTV_PUBLIC_SSTATE_DOWNLOADING,     /* downloading DDBs */
    UTV_PUBLIC_SSTATE_DECRYPTING,      /* decrypting */
    UTV_PUBLIC_SSTATE_STORING_MODULE,  /* storing a module */
    UTV_PUBLIC_SSTATE_STORING_IMAGE,   /* storing an image as a result of a download complete */
    UTV_PUBLIC_SSTATE_SHUTDOWN,        /* agent is shutdown */
    UTV_PUBLIC_SSTATE_COUNT
} UTV_PUBLIC_SSTATE;

/* Error helper structure for diag stats
*/
typedef struct
{
    UTV_UINT16 usError;                     /* the error/status code */
    UTV_UINT16 usCount;                     /* the number of them that has taken place */
    UTV_UINT32 iInfo;                       /* additional information about the error */
    UTV_TIME   tTime;                       /* the time of the last occurrence */
} UTV_PUBLIC_ERR_STAT;

void UtvPublicAgentEarlyInit( void );
UTV_RESULT UtvPublicAgentInit( void );
void UtvPublicAgentShutdown( void );

UTV_RESULT UtvPublicImageGetText( UTV_UINT32 hImage, UtvTextOffset toText, UTV_INT8 **ppText );
UTV_RESULT UtvPublicImageGetUpdateTextId( UTV_UINT32 hImage, UTV_UINT32 uiIndex, UTV_INT8 **ppszTextIdentifier );
UTV_RESULT UtvPublicImageGetCompTextId( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT32 uiIndex, UTV_INT8 **ppszTextIdentifier );
UTV_RESULT UtvPublicImageGetUpdateTextDef( UTV_UINT32 hImage, UTV_INT8 *pszTextID, UTV_INT8 **ppszTextDef );
UTV_RESULT UtvPublicImageGetUpdateTextDefPartial( UTV_UINT32 hImage, UTV_INT8 *pszMatchText, UTV_INT8 **ppszTextID, UTV_INT8 **ppszTextDef );
UTV_RESULT UtvPublicImageGetCompTextDef( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_INT8 *pszTextID, UTV_INT8 **ppszTextDef );

#ifdef UTV_DELIVERY_INTERNET
UTV_RESULT UtvPublicInternetManageRegStore( UTV_UINT32 uiNumStrings, UTV_INT8 *apszString[] );
#endif

#ifdef UTV_INTERACTIVE_MODE
UTV_RESULT UtvPublicTunedChannelHasDownloadData( UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken );
UTV_RESULT UtvPublicGetDownloadSchedule(UTV_BOOL bStoredOnly, UTV_BOOL bFactoryMode, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken, 
                                        UTV_PUBLIC_SCHEDULE **pSchedule, UTV_BOOL bCoordinateDownloadScheduleAndDownloadImage );
UTV_RESULT UtvPublicDownloadComponents( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_BOOL bFactoryDecryptEnable, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken );
UTV_RESULT UtvPublicInstallComponents( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_BOOL bFactoryDecryptEnable, UTV_PUBLIC_CALLBACK pCallback, UTV_UINT32 iToken );
UTV_RESULT UtvPublicGetStatus( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_PUBLIC_STATE *eState, UTV_PUBLIC_SSTATE *eSubState, UTV_PUBLIC_ERR_STAT **ptError, UTV_UINT32 *puiPercentComplete );
UTV_RESULT UtvPublicApproveForDownload( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_UINT32 uiStore );
UTV_RESULT UtvPublicApproveForInstall( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );
UTV_RESULT UtvPublicSetUpdatePolicy( UTV_BOOL bInstallWithoutStore, UTV_BOOL bConvertOptional );
#endif

#endif
