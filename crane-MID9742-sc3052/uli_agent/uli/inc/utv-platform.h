/* utv-platform.h: UpdateTV Platform Adaptation Layer - Main Platform header file

 The UpdateTV Platform Adapatation Layer virtualize the interface from the UpdateTV code to each platform.

 Copyright (c) 2004-2011 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who           Date        Edit
  Bob Taylor    01/17/2012  Added object ID to UtvPlatformOnProvisioned().
  Bob Taylor    12/26/2011  Added new uiClearObjectSize arg to UtvPlatformOnProvisioned().
  Bob Taylor    12/20/2011  Added new args to UtvPlatformOnProvisioned().
  Bob Taylor    12/02/2011  Added UtvConvertHexToBin().
  Jim Muchow    11/02/2011  Download Resume Support (add function decls)
  Jim Muchow    06/29/2011  Add declarations for the explicit SSL Init/Deinit functions
  Bob Taylor    06/13/2011  Added UtvPlatformOnProvisioned().
  Bob Taylor    06/09/2011  New uiLinesToSkip arg to UtvPlatformUtilityAssignFileToStore().
  Bob Taylor    06/04/2011  Added new UtvThreadRemove() function to handle Android case where
                            pthread_cancel isn't available.
  Jim Muchow    04/27/2011  Add versioning to persistent storage structure 
  Bob Taylor    02/23/2011  Added UtvPlatformSecureWriteReadTest() and UtvSleepNoAbort() protos. 
  Bob Taylor    02/08/2011  Removed hImage argument from UtvPlatformInstallCleanup() proto.
  Jim Muchow    02/04/2011  UtvPlatformSecureGetGUID() as part of ULID Copy Protection scheme implementation
  Bob Taylor    12/16/2010  Removed pre-207 storage definitions.  Removed UtvDisplayProvisionedObjects().
  Chris Nigbur  11/02/2010  Added UtvUpTime().
  Bob Taylor    10/01/2010  Added UQR caching struct into peristent memory struct.
  Jim Muchow    08/20/2010  Add in UtvCloseLogFile() prototypes
  Jim Muchow    07/16/2010  Add in threads accounting declarations
  Nathan Ford   07/01/2010  Prototype for UtvPlatformSecureRead() changed.
  Bob Taylor    06/23/2010  Added UtvPlatformUpdateUnassignRemote().
  Bob Taylor    05/10/2010  Added UtvPlatformInstallInit() proto.
  Bob Taylor    04/29/2010  Added UtvPlatformNetworkUp().
  Jim Muchow    04/06/2010  Add globally useful defines for UtvDataDump().
  Bob Taylor    02/09/2010  Added UtvPlatformStoreInit().
  Bob Taylor    02/03/2010  Added UtvPlatformInstallCleanup() and UtvPlatformSystemTimeSet().
  Bob Taylor    11/05/2009  New UtvPlatformInstallCopyComponent() proto.
  Bob Taylor    06/30/2009  New download tracking struct added to peristent memory.
  Bob Taylor    01/26/2009  Created from version 1.9.21
*/

#ifndef __UTV_PLATFORM_H__
#define __UTV_PLATFORM_H__

/* Platform Macros
*/

/* Handles 8-bit rollover of module version
   with a guard band from 40-CF
*/
#define ROLL_GUARD_LOWER_LIMIT    0x40
#define ROLL_GUARD_UPPER_LIMIT    0xBF

/* Rollover handling comparison operators.
   Returns true when A <= B.
*/
#define UTV_PERS_MOD_VER_LTE(A,B) ( A <  ROLL_GUARD_LOWER_LIMIT && B >  ROLL_GUARD_UPPER_LIMIT ? UTV_FALSE : \
                                    A >  ROLL_GUARD_UPPER_LIMIT && B <  ROLL_GUARD_LOWER_LIMIT ? UTV_TRUE  : \
                                    A <= B )

/* Returns true when A < B.
*/
#define UTV_PERS_MOD_VER_LT(A,B)  ( A <  ROLL_GUARD_LOWER_LIMIT && B >  ROLL_GUARD_UPPER_LIMIT ? UTV_FALSE  : \
                                    A >  ROLL_GUARD_UPPER_LIMIT && B <  ROLL_GUARD_LOWER_LIMIT ? UTV_TRUE : \
                                    A < B )

/* Platform Typedefs
*/

/* Typedef for single thread pump function. */
typedef UTV_RESULT (*UTV_SINGLE_THREAD_PUMP_FUNC)(UTV_UINT32 *);

/* Main persistent storage structure:
   This is the "good to have, but not a catastrophe if not available" persistent 
   data storage - the other kind is used for provisioning. The Base- and
   DefaultUtvAgentPersistentStorage structures below are based on the original form
   of this feature, but it became necessary to define and document the means by
   which new objects could be added as needed so as to avoid re-inventing an 
   alternate/parallel universe. 

   The BaseUtvAgentPersistentStorage defines the fixed portion of the original
   or version 1 structre (DefaultUtvAgentPersistentStorage). It exists and will so
   in all versions. It forms the first block read from persistent storage and the
   first block written back on an update.

   DefaultUtvAgentPersistentStorage is the version 1 UtvAgentPersistentStorage 
   structure and defines both the fixed and variable portions of the structure. It
   is assumed by all existing code (as of this writing) and will continue so.

   New additions, new versions will build on the tail of the Base structure - that is,
   between the fixed portion and variable portion. New additions are appended to the
   tail of the fixed portion of the newest version. A new global signature needs to be
   defined each time a new object is added. See version 2 below for an example.
*/
typedef struct
{
    UtvAgentGlobalStorageHeader utvHeader;
    UtvAgentState               utvState;
    UtvAgentDiagStats           utvDiagStats;
    UtvDownloadInfo             utvBestCandidate;
    UtvVersion                  utvVersion;
    UtvDeliveryModes            utvDeliveryConfiguration;
    UtvQueryCache               utvQueryCache;
} BaseUtvAgentPersistentStorage;

typedef struct
{
    UtvAgentGlobalStorageHeader utvHeader;
    /* normally this would be defined here, like this, but it exists elsewhere
#define UTV_PERSISTENT_MEMORY_STRUCT_SIG_V1     0xBDC1
#define UTV_PERSISTENT_MEMORY_STRUCT_SIG        UTV_PERSISTENT_MEMORY_STRUCT_SIG_V1
    */
    UtvAgentState                               utvState;
    UtvAgentDiagStats                           utvDiagStats;
    UtvDownloadInfo                             utvBestCandidate;
    UtvVersion                                  utvVersion;
    UtvDeliveryModes                            utvDeliveryConfiguration;
    UtvQueryCache                               utvQueryCache;
    UtvVarLenHeader             utvVarLenHeader;
} UtvAgentPersistentStorage_V1;

typedef UtvAgentPersistentStorage_V1 UtvAgentPersistentStorage;

#ifdef UTV_LIVE_SUPPORT
/* Version 2:  addition of LiveSupportSessionInfo object, change of signature */
typedef struct liveSupportInfo
{
    UtvAgentGlobalStorageHeader                 header;
#define UTV_LIVE_SUPPORT_INFO_SIG               0x5511
    UTV_TIME                                    savedDate;
    UtvLiveSupportSessionInfo1                  sessionInfo;
} UtvLiveSupportInfo_t;

typedef struct utvAgentPersistentStorage_V2
{
    UtvAgentGlobalStorageHeader                 utvHeader;
#define UTV_PERSISTENT_MEMORY_STRUCT_SIG_V2     0xBDC2
    UtvAgentState                               utvState;
    UtvAgentDiagStats                           utvDiagStats;
    UtvDownloadInfo                             utvBestCandidate;
    UtvVersion                                  utvVersion;
    UtvDeliveryModes                            utvDeliveryConfiguration;
    UtvQueryCache                               utvQueryCache;
    UtvLiveSupportInfo_t                        utvLiveSupportInfo;
    UtvVarLenHeader                             utvVarLenHeader;
} UtvAgentPersistentStorage_V2;
#endif

/* Global extern reference to entire persistent storage structure
    for debug summary only.
*/
extern UtvAgentPersistentStorage g_utvAgentPersistentStorage;

#ifdef UTV_LIVE_SUPPORT
/* Global extern reference to the Live Support persistent storage
   structure.
*/
extern UtvLiveSupportInfo_t g_utvLiveSupportInfo;
#endif

/* Example clear header for hardware specific encrypt/decrypt follows */

typedef struct
{
    unsigned char  ucMagic;
    unsigned char  ucHeaderVersion;
    unsigned short usModelGroup;
    unsigned long  uiOUI;
    unsigned short usEncryptType;
    unsigned short usKeySize;
    unsigned long  ulPayloadSize;
} UtvExtClrHdr;

/* Example external clear header values follow */

#define PLATFORM_SPECIFIC_EXT_ENCRYPT_MAGIC     'B'
#define PLATFORM_SPECIFIC_EXT_ENCRYPT_VERSION   1
#define PLATFORM_SPECIFIC_EXT_ENCRYPT_TYPE      UTV_PROVISIONER_ENCRYPTION_CUST_UR
#define PLATFORM_SPECIFIC_EXT_ENCRYPT_TYPE_BE   (UTV_PROVISIONER_ENCRYPTION_CUST_UR+0x10)
#define PLATFORM_SPECIFIC_EXT_ENCRYPT_KEY_SIZE  32

/* Platform Function Prototypes
 */

/* Main Agent entry points
 */
void UtvApplication( UTV_UINT32 uiNumDeliveryModes, UTV_UINT32 *pauiDeliveryModes );
void UtvUserAbort( void );
void UtvUserExit( void );
#ifdef  UTV_OS_SINGLE_THREAD
void UtvYieldCPU( void );
#endif

/* Partition/File install functions
    Called by common core to install module data
*/
void UtvPlatformInstallInit( void );
UTV_RESULT UtvPlatformInstallCleanup();
UTV_RESULT UtvPlatformInstallModuleData( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_BYTE *pModuleBuffer );
void * UtvPlatformInstallAllocateModuleBuffer( UTV_UINT32 iModuleSize );
UTV_RESULT UtvPlatformInstallFreeModuleBuffer( UTV_UINT32 iModuleSize, void * pModuleBuffer );
UTV_RESULT UtvPlatformInstallComponentComplete( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc );
UTV_RESULT UtvPlatformInstallCopyComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc );
UTV_RESULT UtvPlatformInstallUpdateComplete( UTV_UINT32 hImage );

UTV_RESULT UtvPlatformInstallPartitionOpenWrite( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle );
UTV_RESULT UtvPlatformInstallPartitionOpenAppend( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle );
UTV_RESULT UtvPlatformInstallPartitionOpenRead( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_BOOL bActive, UTV_UINT32 *puiHandle  );
UTV_RESULT UtvPlatformInstallPartitionRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize, UTV_UINT32 uiOffset );
UTV_RESULT UtvPlatformInstallPartitionWrite( UTV_UINT32 uiHandle, UTV_BYTE *pData, UTV_UINT32 uiDataLen, UTV_UINT32 uiDataOffset );
UTV_UINT32 UtvPlatformInstallPartitionClose( UTV_UINT32 uiHandle );

UTV_RESULT UtvPlatformInstallFileOpenWrite( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle );
UTV_RESULT UtvPlatformInstallFileOpenAppend( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvModSigHdr *pModSigHdr, UTV_UINT32 *puiHandle );
UTV_RESULT UtvPlatformInstallFileOpenRead( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_BOOL bActive, UTV_UINT32 *puiHandle );
UTV_RESULT UtvPlatformInstallFileRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize );
UTV_RESULT UtvPlatformInstallFileSeekRead( UTV_UINT32 uiHandle, UTV_BYTE *pBuff, UTV_UINT32 uiSize, UTV_UINT32 uiOffset );
UTV_RESULT UtvPlatformInstallFileWrite( UTV_UINT32 uiHandle, UTV_BYTE *pData, UTV_UINT32 uiDataLen, UTV_UINT32 uiDataOffset );
UTV_RESULT UtvPlatformInstallFileClose( UTV_UINT32 uiHandle );
UTV_BOOL UtvFilesOpen( void );
void UtvCloseAllFiles( void );

/* CEM-specific common utility functions
    Called by the common core to perform utility functions.
*/
UTV_RESULT UtvPlatformCommonAcquireHardware( UTV_UINT32 iEventType );
UTV_RESULT UtvPlatformCommonReleaseHardware( void );
void UtvModOrgInit( UTV_BOOL bPersistMemReadSuccess );
void UtvShutdown( void );
UTV_RESULT UtvGetTunerFrequencyList( UTV_UINT32 * piFrequencyListSize, UTV_UINT32 * * ppFrequencyList );
UTV_RESULT UtvPlatformFactoryModeNotice( UTV_BOOL bFactoryMode );
UTV_RESULT UtvPlatformCommandInterface( UTV_INT8 *pszCommand );
UTV_BOOL UtvPlatformNetworkUp( void );
UTV_BOOL UtvPlatformSystemTimeSet( void );
UTV_RESULT UtvPlatformSetSystemTime( UTV_UINT32 uiTime );
UTV_RESULT UtvGetPid ( UTV_UINT32 * piPID );
UTV_UINT32 UtvCrc32( const UTV_BYTE * pBuffer, UTV_UINT32 uiSize );
UTV_UINT32 UtvCrc32Continue( const UTV_BYTE * pBuffer, UTV_UINT32 uiSize, UTV_UINT32 uiCrc );
void UtvSetRestartRequest( void );
UTV_BOOL UtvGetRestartRequest( void );
UTV_RESULT UtvStoreModuleAsFile( UTV_BYTE *pModuleData, UTV_UINT32 iModuleSize, char *pszModuleName, UTV_UINT32 iModuleId );
UTV_RESULT UtvConcatModules( char *pszModuleBaseName, UTV_UINT32 iSignalledCount, UTV_UINT32 iModVer );
void UtvSetErrorCount( UTV_RESULT rError, UTV_UINT32 usInfo );
UTV_UINT32 UtvLogGetStoredEventClassMask( void );
void UtvLogEventNoLine( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg );
void UtvLogEventOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1 );
void UtvLogEventTwoDecNums( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1, UTV_UINT32 uiV2 );
void UtvLogEventString( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString );
void UtvLogEventStringOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString, UTV_UINT32 uiV1 );
void UtvLogEventStringOneHexNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_INT8 *pszString, UTV_UINT32 uiV1 );
void UtvLogEventOneHexNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1 );
void UtvLogEventTwoHexNums( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_UINT32 uiV1, UTV_UINT32 uiV2 );
void UtvLogEvent( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine );
void UtvLogEventTimes( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiLine, UTV_TIME tT1, UTV_TIME tT2 );
void UtvLogEventNoLineOneDecNum( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_UINT32 uiV1 );
void UtvLogClearEventLog( UTV_UINT32 uiLevel );
UTV_UINT32 UtvLogGetEventClassCount( UTV_UINT32 uiLevel, UTV_UINT32 *puiFirst );
UTV_EVENT_ENTRY *UtvLogGetEvent( UTV_UINT32 uiLevel, UTV_UINT32 uiIndex );
UTV_INT8 *UtvLogConvertTextOffset( UTV_UINT32 uiOffset );
void UtvConvertHexToBin( void *bin, UTV_UINT32 size, UTV_INT8 *hex );

void UtvSetDSIIncompatStatus( UTV_RESULT utvStatus );
void UtvSetDIIIncompatStatus( UTV_RESULT utvStatus );
#ifdef UTV_DELIVERY_FILE
UTV_RESULT UtvPlatformUtilityAssignFileToStore( UTV_UINT32 uiLinesToSkip );
#endif
UTV_BOOL UtvPlatformUtilityFileOnly( void );
UTV_INT8 *UtvCTime( UTV_TIME utvTime, UTV_INT8 *pBuffer, UTV_UINT32 uiBufferLen );
void UtvDumpEvents( void );
UTV_RESULT UtvConvertToIpV4String( UTV_UINT32 uiAddress, UTV_INT8* pszBuffer, UTV_UINT32 uiBufferLen );

/* Secure read/write functions
*/
UTV_RESULT UtvPlatformSecureOpen( void );
UTV_RESULT UtvPlatformSecureClose( void );
UTV_RESULT UtvPlatformSecureRead( UTV_INT8 *pszFName, UTV_BYTE **pBuff, UTV_UINT32 *puiDataLen );
UTV_RESULT UtvPlatformSecureWrite( UTV_INT8 *pszFileName, UTV_BYTE *pubData, UTV_UINT32 uiSize );
UTV_RESULT UtvPlatformSecureDeliverKeys( void *pKeys );
UTV_RESULT UtvPlatformSecureBlockSize( UTV_UINT32 *puiBlockSize );
UTV_RESULT UtvPlatformSecureDecrypt( UTV_UINT32 uiOUI, UTV_UINT32 uiModelGroup, UTV_BYTE *pInBuff, UTV_BYTE *pOutBuff, UTV_UINT32 uiDataLen );
UTV_RESULT UtvPlatformSecureDecryptDirect( UTV_BYTE *pInBuff, UTV_UINT32 *puiDataLen );
UTV_RESULT UtvPlatformSecureGetULPK( UTV_BYTE **pULPK, UTV_UINT32 *puiULPKLength );
UTV_UINT32 UtvPlatformSecureGetULPKIndex( void );
UTV_RESULT UtvPlatformSecureInsertULPK( UTV_BYTE *pbULPK );
UTV_RESULT UtvPlatformSecureGetGUID( UTV_BYTE *pGUID, UTV_UINT16 *puiGUIDLength );
UTV_RESULT UtvPlatformSecureWriteReadTest( void );

/* Persistent memory management functions
*/
UTV_RESULT UtvPlatformPersistRead( void );
UTV_RESULT UtvPlatformPersistWrite( void );

/* Provisioning related functions
 */
#ifdef UTV_FILE_BASED_PROVISIONING
void UtvPlatformOnProvisioned(UTV_BYTE *pubObject, UTV_UINT32 uiObjectSize, UTV_UINT32 uiClearObjectSize,
                              UTV_INT8 *pszName, UTV_INT8 *pszOwner, UTV_INT8 *pszObjectID, UTV_UINT32 uiEncryptionType, 
                              UTV_INT8 *pszDeliveryInfo, UTV_UINT32 uiOperation );
#endif

/* Test message functions
    If messages are turned off, just define all these calls as nothing
*/
char * UtvGetMessageString( UTV_RESULT rReturn );

#ifndef UTV_TEST_MESSAGES
#define UtvMessage( ... )
#define UtvIsMessageLevelSet( ... )
#define UtvGetCategoryString( ... )
#define UtvStatsSummary( ... )
#define UtvStatsDisplay( ... )
#define UtvCompatibilitySummary( ... )
#define UtvAnnouceConfiguration( ... )
#define UtvDisplayComponentDirectory( ... )
#define UtvDebugSetThreadName( ... )
#define UtvDebugGetThreadName( ... )
#define UtvDisplaySchedule( ... )
#define UtvDataDump( ... )
#define UtvCloseLogFile( ... )
#else
void UtvMessage( UTV_UINT32 iLevel, const char * pszMessage, ... );
UTV_BOOL UtvIsMessageLevelSet( UTV_UINT32 iMessageLevel );
char * UtvGetCategoryString( UTV_CATEGORY rCat );
void UtvStatsSummary( void );
void UtvStatsDisplay( UtvAgentPersistentStorage *pStats );
void UtvDisplayComponentManifest( void );
UTV_INT8 *UtvTranslateDeliveryMode( UTV_UINT32 uiDeliveryMode );
void UtvAnnouceConfiguration( void );
UTV_RESULT UtvDisplayComponentDirectory( UTV_UINT32 hImage );
void UtvDisplaySchedule(UTV_PUBLIC_SCHEDULE *pSchedule);
void UtvDebugSetThreadName( UTV_INT8 *pName );
UTV_INT8 *UtvDebugGetThreadName( void );
#ifdef UTV_TEST_LOG_FNAME
void UtvCloseLogFile( void );
#endif
/* dataDump() defines */
#define DATA_CHUNK      16 /* dataDump works best on data chunks of 16 bytes */
#define EACHLINE_SIZE   75 /* any buffer provided via dataOut must comprise
                              at least this amount for each line */
#define OUT_ALLOCATE( BYTES_TO_DUMP ) ( ( ( BYTES_TO_DUMP ) / DATA_CHUNK ) * EACHLINE_SIZE )
void UtvDataDump( void *dataIn, int dataInSize );
#endif

/* Update storage access functions
*/
#define UTV_SEEK_SET    0
#define UTV_SEEK_CUR    1
#define UTV_SEEK_END    2
UTV_RESULT UtvPlatformStoreInit( void );
UTV_RESULT UtvPlatformStoreOpen( UTV_UINT32 uiStoreIndex, UTV_UINT32 uiAccessFlags, UTV_UINT32 uiUpdateSize, UTV_UINT32 *phStoreHandle,
                                       UTV_INT8 *pszHostName, UTV_INT8 *pszFilePath );
UTV_RESULT UtvPlatformStoreWrite( UTV_UINT32 hStoreHandle, UTV_BYTE *pDataBuffer, UTV_UINT32 uiBytesToWrite, UTV_UINT32 *puiBytesWritten );
UTV_RESULT UtvPlatformStoreClose( UTV_UINT32 hStoreHandle );
UTV_RESULT UtvPlatformStoreDelete( UTV_UINT32 hStoreHandle );
UTV_RESULT UtvPlatformStoreAttributes( UTV_UINT32 uiStoreIndex, UTV_BOOL *pbOpen, UTV_BOOL *pbPrimed, UTV_BOOL *pbAssignable,
                                       UTV_BOOL *pbManifest, UTV_UINT32 *puiBytesWritten );
UTV_RESULT UtvPlatformStoreSetCompStatus( UTV_UINT32 uiStoreIndex, UTV_UINT32 uiCompIndex, UTV_BOOL bStatus );
UTV_RESULT UtvPlatformUpdateAssignRemote( UTV_INT8 *pszFileName, UTV_UINT32 *phStoreHandle );
UTV_RESULT UtvPlatformUpdateUnassignRemote( void );
UTV_RESULT UtvFileImageAccessInterfaceGet( UtvImageAccessInterface *pIAI );
void UtvInternetImageAccessInterfaceReinit( void );

/* xxxx-memory.c: Memory management functions
 */
void * UtvMalloc( UTV_UINT32 iSize );
void UtvFree ( void * pMemoryBlock );
void UtvByteCopy( UTV_BYTE * pStore, UTV_BYTE * pData, UTV_UINT32 iSize );
void UtvMemset( void * pData, UTV_BYTE bData, UTV_UINT32 iSize );
UTV_INT32 UtvMemcmp( void * pData1, void * pData2, UTV_UINT32 iSize );
UTV_BYTE * UtvStrnCopy( UTV_BYTE * pStore, UTV_UINT32 iBuffSize, UTV_BYTE * pSource, UTV_UINT32 iCount );
UTV_UINT32 UtvStrlen( UTV_BYTE * pData );
UTV_BYTE *UtvStrcat( UTV_BYTE * pDst, UTV_BYTE * pSrc );
UTV_UINT32 UtvStrtoul( UTV_INT8 * pString, UTV_INT8 ** ppEndPtr, UTV_UINT32 uiBase );
UTV_UINT32 UtvMemFormat( UTV_BYTE * pszData, UTV_UINT32 iBuffSize, char * pszFormat, ... );

/* xxxx-thread.c: Thread management functions (optional)
 */
#ifdef UTV_THREAD_START_ROUTINE
/* undefine by default, unless Makefile VERSION is DEBUG */
#undef THREADS_DEBUG
#if VERSION == DEBUG
/* define - or not - THREADS_DEBUG */
#define THREADS_DEBUG
#undef THREADS_DEBUG
#endif
UTV_BOOL UtvThreadsActive( void );
UTV_THREAD_HANDLE UtvThreadSelf( void );
void UtvThreadKillAllThreads( void );
void *UtvThreadExit( void );
void UtvThreadRemove( UTV_THREAD_HANDLE hThread );
UTV_RESULT UtvThreadCreate( UTV_THREAD_HANDLE *phThread, UTV_THREAD_START_ROUTINE pRoutine, void * pArg );
UTV_RESULT UtvThreadDestroy( UTV_THREAD_HANDLE *phThread );
UTV_RESULT UtvThreadSetCancelState( UTV_INT32 iNewState, UTV_INT32 *pIOldState );

/* Mockup of the pthread cancel defines.  Has to be equal to the ones found in pthread.h */
enum {
  UTV_PTHREAD_CANCEL_ENABLE,
#define UTV_PTHREAD_CANCEL_ENABLE UTV_PTHREAD_CANCEL_ENABLE
  UTV_PTHREAD_CANCEL_DISABLE
#define UTV_PTHREAD_CANCEL_DISABLE UTV_PTHREAD_CANCEL_DISABLE
};

#endif

/* xxxx-execute.c: File Execution management functions (optional)
*/
UTV_RESULT    UtvExecute( char *szExeFileName, UTV_UINT32 *iSystemErr );

/* xxxx-mutex.c: Handles for mutex support
*/
#define UTV_MUTEX_HANDLE UTV_UINT32
#define UTV_WAIT_HANDLE UTV_UINT32

/* xxxx-mutex.c: Mutex functions
*/
UTV_RESULT UtvMutexInit( UTV_MUTEX_HANDLE * phMutex );
UTV_RESULT UtvMutexDestroy( UTV_MUTEX_HANDLE hMutex );
UTV_RESULT UtvMutexLock( UTV_MUTEX_HANDLE hMutex );
UTV_RESULT UtvMutexUnlock( UTV_MUTEX_HANDLE hMutex );
UTV_RESULT UtvInitMutexList( void );
void UtvDestroyMutexList( void );
UTV_BOOL UtvMutexHandles( void );
UTV_RESULT UtvInitWaitList( void );
void UtvDestroyWaitList( void );
UTV_RESULT UtvWaitInit( UTV_WAIT_HANDLE * phWait );
UTV_RESULT UtvWaitDestroy( UTV_WAIT_HANDLE hWait );
UTV_RESULT UtvWaitTimed( UTV_WAIT_HANDLE hWait, UTV_UINT32 * piTimer );
UTV_RESULT UtvWaitSignal( UTV_WAIT_HANDLE hWait );
void UtvWakeAllThreads( void );
UTV_BOOL UtvWaitHandles( void );

/* xxxx-time.c: Time conversion functions
*/
UTV_UINT32 UtvGetTickCount( void );
void UtvSleep( UTV_UINT32 iMilliseconds );
void UtvSleepNoAbort( UTV_UINT32 iMilliseconds );
#ifdef UTV_OS_SINGLE_THREAD
UTV_BOOL UtvSleeping( void );
#endif
struct tm * UtvGmtime( time_t * pTime_t );
UTV_INT8 * UtvAsctime( struct tm * ptmTime );
time_t UtvMktime( struct tm * ptmTime );
time_t UtvTime( time_t * pTime_t );
UTV_TIME UtvUpTime( void );

/* xxxx-file.c File access functions
*/
#define UTV_SEEK_SET    0
#define UTV_SEEK_CUR    1
#define UTV_SEEK_END    2
UTV_RESULT UtvPlatformFileOpen( char *pszFileName, char *pszAttributes, UTV_UINT32 *phFile );
UTV_RESULT UtvPlatformFileReadBinary( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiReadRequestLen, UTV_UINT32 *puiBytesRead );
UTV_RESULT UtvPlatformFileWriteBinary( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiWriteRequestLen, UTV_UINT32 *puiBytesWritten );
UTV_RESULT UtvPlatformFileReadText( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiReadMax );
UTV_RESULT UtvPlatformFileWriteText( UTV_UINT32 hFile, UTV_BYTE *pBuffer );
UTV_RESULT UtvPlatformFileSeek( UTV_UINT32 hFile, UTV_UINT32 uiSeekOffset, UTV_UINT32 uiSeekType );
UTV_RESULT UtvPlatformFileTell( UTV_UINT32 hFile, UTV_UINT32 *uiTellOffset );
UTV_RESULT UtvPlatformFileDelete( char *pszFileName );
UTV_RESULT UtvPlatformFileClose( UTV_UINT32 hFile );
UTV_RESULT UtvPlatformFileGetSize( UTV_INT8 *pszFName, UTV_UINT32 *uiDataLen );
UTV_BOOL UtvPlatformFilePathExists( UTV_INT8 *pszPFName );
UTV_RESULT UtvPlatformMakeDirectory( UTV_INT8 *pszPath );
UTV_RESULT UtvPlatformFileCopy( UTV_INT8 *pszFromFName, UTV_INT8 *pszToFName );

/* xxxx-getnetwork.c Get Network Info functions
*/
#if defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT)
UTV_RESULT UtvPlatformGetNetworkInfo( void *pVoidResponseData );
#endif

/* xxxx-getsystemstats.c Get System Stat functions
*/
#ifdef UTV_LIVE_SUPPORT
UTV_RESULT UtvPlatformGetSystemStats( void *pVoidResponseData );
#endif

#ifdef UTV_DELIVERY_BROADCAST
/* data-src-broadcast-xxxx.c: Broadcast section filter personality functions
*/
UTV_RESULT UtvPlatformDataBroadcastAcquireHardware( UTV_UINT32 iEventType );
UTV_RESULT UtvPlatformDataBroadcastReleaseHardware( void );
UTV_RESULT UtvPlatformDataBroadcastSetTunerState( UTV_UINT32 iFrequency );
UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( void );
UTV_RESULT UtvPlatformDataBroadcastGetTuneInfo( UTV_UINT32 *puiPhysicalFrequency, UTV_UINT32 *puiModulationType );
UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pSingleThreadPumpFunc );
UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( UTV_UINT32 iPid, UTV_UINT32 iTableId, UTV_UINT32 iTableIdExt, UTV_BOOL bEnableTableIdExt );
UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( void );
UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( void );

/* data-src-xxxx.c: Packet filter personality functions (optional)
*/
#ifdef UTV_CORE_PACKET_FILTER
UTV_BYTE * UtvPlatformGetTransportStreamPacket( );
void UtvDeliverTransportStreamPacket( UTV_UINT32 iDesiredPid, UTV_UINT32 iDesiredTableId, UTV_UINT32 iDesiredTableIdExt, UTV_BYTE * pTransportStreamPacket );
UTV_RESULT UtvVerifySectionData( UTV_BYTE * pSectionData, UTV_UINT32 iSectionLength );
#endif /* UTV_CORE_PACKET_FILTER */
#endif /* UTV_DELIVERY_BROADCAST */

#ifdef UTV_DELIVERY_INTERNET
/* data-src-internet-common.c:  Internet common connection functions
 */
UTV_RESULT UtvPlatformInternetCommonAcquireHardware( UTV_UINT32 iEventType );
UTV_RESULT UtvPlatformInternetCommonReleaseHardware( void );
UTV_RESULT UtvPlatformInternetCommonDNS( char *pszHost, void * );

/* data-src-internet-ssl-xxxx.c:  Internet SSL connection functions
 */
UTV_RESULT UtvPlatformInternetSSLAddCerts( void *pCertStore, UTV_BYTE *pubData, UTV_UINT32 uiSize );
UTV_RESULT UtvPlatformInternetSSLAddCertsFromFile( void *pCertStore, UTV_INT8 *filename );
UTV_RESULT UtvPlatformInternetSSLOpen( char *pszHost, UTV_UINT32 iPort, UTV_UINT32 uiReadTimeout );
UTV_RESULT UtvPlatformInternetSSLSessionOpen( UTV_UINT32 *phSession );
UTV_RESULT UtvPlatformInternetSSLSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen );
UTV_RESULT UtvPlatformInternetSSLGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned );
UTV_RESULT UtvPlatformInternetSSLSessionClose( UTV_UINT32 hSession );
UTV_RESULT UtvPlatformInternetSSLClose( void );
UTV_RESULT UtvPlatformInternetSSL_Init( void );
UTV_RESULT UtvPlatformInternetSSL_Deinit( void );

/* data-src-internet-clear-xxxx.c:  Internet clear connection functions
 */
UTV_RESULT UtvPlatformInternetClearOpen( char *pszHost, UTV_UINT32 iPort );
UTV_RESULT UtvPlatformInternetClearSessionOpen( UTV_UINT32 *phSession );
UTV_RESULT UtvPlatformInternetClearSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen );
UTV_RESULT UtvPlatformInternetClearGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned );
UTV_RESULT UtvPlatformInternetClearSessionClose( UTV_UINT32 hSession );
UTV_RESULT UtvPlatformInternetClearClose( void );
#endif /* UTV_DELIVERY_INTERNET */

/* platform-data-file-common.c:  File I/O functions
 */
UTV_RESULT UtvPlatformFileAcquireHardware( UTV_UINT32 iEventType );
UTV_RESULT UtvPlatformFileReleaseHardware( void );

/* platform-os-<specific>-timestamp.c:  Access structures and functions */
#ifdef UTV_TEST_RSPTIME_LOG_FNAME
typedef
enum { 
    ACTIVTY_TYPE_UNSPECIFIED,           /* testing */

    /* the next 8 values are supposed to be the numeric equivalent of */
    /* the UTV_INET_ACTION_ values                                    */
    ACTIVTY_TYPE_REQUEST_REGISTERIDD,   /* eg, UTV_INET_ACTION_REQUEST_REGISTERIDD */
    ACTIVTY_TYPE_REQUEST_IDDREGISTERED,
    ACTIVTY_TYPE_REQUEST_UPDATE_QUERY,
    ACTIVTY_TYPE_REQUEST_SCHEDULE_DLOAD,
    ACTIVTY_TYPE_REQUEST_DLOAD_ENDED,
    ACTIVTY_TYPE_REQUEST_STATUS_CHANGE,
    ACTIVTY_TYPE_REQUEST_PROVISIONIDD,
    ACTIVTY_TYPE_REQUEST_IDDPROVISIONED,

    ACTIVTY_TYPE_REQUEST_DOWNLOAD, /* this is a new entry */
    
    /* add new entries here */

    ACTIVTY_TYPE_INVALID = 0xFFFF /* this should be plenty */
} UtvTimeStampActivityType_t;

extern
void UtvCloseRspTimeLogFile( void );

extern void
UtvStartActivityTimer( UtvTimeStampActivityType_t activity );

extern void
UtvStopActivityTimer( UtvTimeStampActivityType_t activity );

#else

/* "undefine" */
#define UtvStartActivityTimer( ... )
#define UtvStopActivityTimer( ... )

#endif /* UTV_TEST_RSPTIME_LOG_FNAME */

#endif /* __UTV_PLATFORM_H__ */
