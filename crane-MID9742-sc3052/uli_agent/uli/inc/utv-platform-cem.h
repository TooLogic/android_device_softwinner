/* utv-platform-cem.h: UpdateTV Consumer Electronics Manufacturer header file

 Contains prototypes for functions that configure the UTV Agent to CEM-specific values.

 Copyright (c) 2009-2010 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who           Date        Edit
  Chris Nigbur  10/21/2010  Added Live Support APIs.
  Bob Taylor    02/09/2010  Added UtvPlatformGetActiveCompManifestName(),
                            UtvPlatformGetBackupCompManifestName(),
                            UtvPlatformGetUpdateRedirectName(),
							UtvPlatformGetStore0Name(),
							UtvPlatformGetStore1Name(),
							UtvPlatformGetPersistName(),
							UtvPlatformGetRegstoreName(),
							UtvPlatformGetTSName().
  Bob Taylor    03/31/2009  Created
*/

#ifndef __UTV_PLATFORM_CEM_H__
#define __UTV_PLATFORM_CEM_H__

#if defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT) || defined(UTV_LOCAL_SUPPORT)
#include "cJSON.h"
#endif

/* All of these functions are found in project/project-xxxx.c
*/
UTV_RESULT UtvCEMConfigure( void );
UTV_UINT32 UtvCEMGetPlatformOUI( void );
UTV_UINT16 UtvCEMGetPlatformModelGroup( void );
UTV_RESULT UtvCEMGetSerialNumber( UTV_INT8 *pubBuffer, UTV_UINT32 uiBufferSize );
UTV_UINT16 UtvCEMTranslateHardwareModel( void );
UTV_INT8 *UtvCEMGetKeystoreFile( void );
UTV_INT8 *UtvPlatformGetActiveCompManifestName( void );
UTV_INT8 *UtvPlatformGetBackupCompManifestName( void );
UTV_INT8 *UtvPlatformGetUpdateRedirectName( void );
UTV_INT8 *UtvPlatformGetStore0Name( void );
UTV_INT8 *UtvPlatformGetStore1Name( void );
UTV_INT8 *UtvPlatformGetPersistName( void );
UTV_INT8 *UtvPlatformGetRegstoreName( void );
UTV_INT8 *UtvPlatformGetTSName( void );
#ifdef UTV_DELIVERY_FILE
UTV_RESULT UtvCEMGetUpdateFilePath( UTV_INT8 *pszFilePath, UTV_UINT32 uiBufferLen );
#endif
#ifdef UTV_DELIVERY_INTERNET
UTV_BOOL UtvCEMProductionNOC( void );
UTV_INT8 *UtvCEMGetInternetQueryHost( void );
UTV_INT8 *UtvCEMSSLGetCertFileName( void );
UTV_INT8 *UtvCEMInternetGetULIDFileName( void );
UTV_INT8 *UtvCEMInternetGetULPKFileName( void );
UTV_INT8 *UtvCEMInternetGetProvisionerPath( void );
#endif

#if defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT)
UTV_RESULT UtvCEMGetDeviceInfo( cJSON* pDeviceInfo );
UTV_RESULT UtvCEMGetNetworkInfo( cJSON* pNetworkInfo );
UTV_RESULT UtvCEMNetworkDiagnostics( cJSON* pNetworkDiagnostics );
#endif /* defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT) */

#if defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT)
UTV_RESULT UtvCEMRemoteButton( cJSON* pRemoteButtons );
#endif /* defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT) */

#ifdef UTV_LAN_REMOTE_CONTROL
void UtvCEMLanRemoteControlInitialize( void );
#endif /* UTV_LAN_REMOTE_CONTROL */

#ifdef UTV_LOCAL_SUPPORT
void UtvCEMLocalSupportInitialize( void );
#endif /* UTV_LOCAL_SUPPORT */

#ifdef UTV_LIVE_SUPPORT
void UtvCEMLiveSupportInitialize( void );
void UtvCEMLiveSupportShutdown( void );
void UtvCEMLiveSupportSessionInitialize( void );
void UtvCEMLiveSupportSessionShutdown( void );
#endif /* UTV_LIVE_SUPPORT */

#ifdef UTV_LIVE_SUPPORT_VIDEO
UTV_BOOL UtvPlatformLiveSupportVideoSecureMediaStream( void );

UTV_RESULT UtvPlatformLiveSupportVideoGetLibraryPath(
  UTV_BYTE* pubBuffer,
  UTV_UINT32 uiBufferSize );

UTV_RESULT UtvPlatformLiveSupportVideoGetVideoFrameSpecs(
  UTV_INT32* piWidth,
  UTV_INT32* piHeight,
  UTV_INT32* piPitch );

UTV_RESULT UtvPlatformLiveSupportVideoFillVideoFrameBuffer(
  UTV_BYTE* pubBuffer,
  UTV_UINT32 uiBufferSize );
#endif /* UTV_LIVE_SUPPORT_VIDEO */

#endif /* __UTV_PLATFORM_CEM_H__ */
