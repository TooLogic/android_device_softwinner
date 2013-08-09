/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010-2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Declaration of LiveSupport interface.
 *
 * @file      utv-livesupport.h
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: $
 *            $Author: $
 *            $Date: 2011-12-19 23:11:54 -0500 (Mon, 19 Dec 2011) $
 *
 **********************************************************************************************
 */

#ifndef LIVESUPPORT_H
#define LIVESUPPORT_H

#include "cJSON.h"



/*
 **********************************************************************************************
 *
 * DEFINES
 *
 **********************************************************************************************
 */

typedef enum sessionState
{
  UTV_LIVE_SUPPORT_SESSION_STATE_NONE = 0x00,
  UTV_LIVE_SUPPORT_SESSION_STATE_INITIALIZING = 0x01,
  UTV_LIVE_SUPPORT_SESSION_STATE_JOINING = 0x02,
  UTV_LIVE_SUPPORT_SESSION_STATE_RUNNING = 0x03,
  UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED = 0x04,
} UtvLiveSupportSessionState_t;



/*
 **********************************************************************************************
 *
 * FUNCTIONS
 *
 **********************************************************************************************
 */

/* Private methods, should only be used within Live Support code */
UTV_RESULT UtvLiveSupportTestConnection( void );
void UtvLiveSupportAgentInit( void );
void UtvLiveSupportAgentShutdown( void );
void UtvLiveSupportSessionInit( void );
UTV_RESULT UtvLiveSupportSessionShutdown( UTV_BOOL bInternallyInitiated );
UTV_RESULT UtvLiveSupportSessionPause();
UTV_RESULT UtvLiveSupportSessionResume();

UTV_BOOL UtvLiveSupportIsSessionActive( void );

/* DEPRECATED - use UtvLiveSupportInitiateSession1 */
UTV_RESULT UtvLiveSupportInitiateSession( UTV_BYTE* pbSessionCode,
                                          UTV_UINT32 uiSessionCodeSize );

UTV_RESULT UtvLiveSupportInitiateSession1(
  UtvLiveSupportSessionInitiateInfo1* pSessionInitiateInfo,
  UtvLiveSupportSessionInfo1* pSessionInfo );

UTV_RESULT UtvLiveSupportTerminateSession( void );
UTV_RESULT UtvLiveSupportGetSessionId( UTV_BYTE* pszSessionId, UTV_UINT32 uiSessionIdSize );
UTV_RESULT UtvLiveSupportSetSessionId( UTV_BYTE* pszSessionId );
UTV_RESULT UtvLiveSupportGetSessionState( UtvLiveSupportSessionState_t* pSessionState );
UTV_RESULT UtvLiveSupportSetSessionState( UtvLiveSupportSessionState_t sessionState );
UTV_RESULT UtvLiveSupportGetSessionCode( UTV_BYTE* pbSessionCode,
                                         UTV_UINT32 uiSessionCodeSize );
#endif /* LIVESUPPORT_H */
