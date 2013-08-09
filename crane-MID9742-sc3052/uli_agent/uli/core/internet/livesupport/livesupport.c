/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Implementation of LiveSupport interface.
 *
 * @file      livesupport.c
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: $
 *            $Author: $
 *            $Date: 2011-12-19 23:11:54 -0500 (Mon, 19 Dec 2011) $
 *
 **********************************************************************************************
 */

#include "utv.h"

/*
 **********************************************************************************************
 *
 * STATIC DEFINES
 *
 **********************************************************************************************
 */

/*
 * This is the amount of time that can have elapsed since a session was paused and have it
 * still be allowed to be resumed.
 */
#define SESSION_RESUME_LIMIT_SECONDS 60 * 60



/*
 **********************************************************************************************
 *
 * STATIC VARS
 *
 **********************************************************************************************
 */

static UTV_BYTE* s_sessionId = NULL;
static UTV_BYTE s_sessionCode[SESSION_CODE_SIZE];
static UtvLiveSupportSessionState_t s_sessionState = UTV_LIVE_SUPPORT_SESSION_STATE_NONE;


/*
 **********************************************************************************************
 *
 * STATIC FUNCTIONS
 *
 **********************************************************************************************
 */

#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
static UTV_RESULT s_LiveSupportInitiateSession( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportTerminateSession( cJSON* inputs, cJSON* outputs );
static UTV_RESULT s_LiveSupportGetSessionState( cJSON* inputs, cJSON* outputs );
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */



/**
 **********************************************************************************************
 *
 * Initialize the Live Support sub-system as part of Agent initialization.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportAgentInit( void )
{
#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
  /*
   * Register post command handlers with the web server to allow these commands to be
   * processed.
   */

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LIVE_SUPPORT_INITIATE_SESSION",
                                             s_LiveSupportInitiateSession );

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LIVE_SUPPORT_TERMINATE_SESSION",
                                             s_LiveSupportTerminateSession );

  UtvWebServerUliRegisterPostCommandHandler( (UTV_BYTE*) "LIVE_SUPPORT_GET_SESSION_STATE",
                                             s_LiveSupportGetSessionState );
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */

  /*
   * Initialize the task processing used during a Live Support session.
   */

  UtvLiveSupportTaskAgentInit();

#ifdef UTV_LIVE_SUPPORT_VIDEO
  /*
   * Initialize the video support used during a Live Support session.
   */

  UtvLiveSupportVideoAgentInit();
#endif /* UTV_LIVE_SUPPORT_VIDEO */

  /*
   * Call the CEM specific initialization routines for Live Support.
   *
   * NOTE: This call must come after UtvLiveSupportTaskAgentInit since it depends on
   *       items being initialized.
   */

  UtvCEMLiveSupportInitialize();
}



/**
 **********************************************************************************************
 *
 * Shutdown the Live Support sub-system as part of Agent shutdown.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportAgentShutdown( void )
{
  if ( UtvLiveSupportIsSessionActive() )
  {
    /*
     * Shutdown the session processing with the indication that this shutdown is internally
     * initiated.  The UTV_TRUE indicates the internal initiation.
     */

    UtvLiveSupportSessionShutdown( UTV_TRUE );
  }

  UtvLiveSupportTaskAgentShutdown();

#ifdef UTV_LIVE_SUPPORT_VIDEO
  UtvLiveSupportVideoAgentShutdown();
#endif /* UTV_LIVE_SUPPORT_VIDEO */

  /*
   * Call the CEM specific shutdown routines for Live Support.
   */

  UtvCEMLiveSupportShutdown();
}



/**
 **********************************************************************************************
 *
 * Initialize a Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportSessionInit( void )
{
  /*
   * Startup the task handling for the Live Support session.
   */

  UtvLiveSupportTaskSessionInit();

  /*
   * Call the CEM specific session initialization routines for Live Support.
   */

  UtvCEMLiveSupportSessionInitialize();
}



/**
 **********************************************************************************************
 *
 * Shutdown the Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportSessionShutdown( UTV_BOOL bInternallyInitiated )
{
  UTV_RESULT result;
  UTV_BYTE* sessionId = NULL;
  UTV_BYTE sessionCode[SESSION_CODE_SIZE];

  if ( !UtvLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvLiveSupportGetSessionCode( sessionCode, SESSION_CODE_SIZE );

  sessionId = s_sessionId;
  s_sessionId = NULL;

#ifdef UTV_LIVE_SUPPORT_VIDEO
  /*
   * Shutdown the video processing, it is only needed while a session is active.
   */

  UtvLiveSupportVideoSessionShutdown();
#endif

  /*
   * Shutdown the task handling, it is only needed while a session is active.
   */

  UtvLiveSupportTaskSessionShutdown();

  UtvMemset( s_sessionCode, 0x00, SESSION_CODE_SIZE );

  /*
   * If the session is not currently paused there might be additional processing needed.
   *
   * Basically the NetReady Services should not be notified that the session should be
   * terminated if it is in the paused state.
   */

  if ( UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED != s_sessionState )
  {
    UtvLiveSupportSetSessionState( UTV_LIVE_SUPPORT_SESSION_STATE_NONE );

    /*
     * If the shutdown was initiated within the Device Agent code then attempt to send the
     * terminate command to the NetReady Services.
     */

    result =
      UtvInternetDeviceWebServiceLiveSupportTerminateSession(
        sessionId,
        bInternallyInitiated );

    if ( UTV_OK != result )
    {
      UtvMessage(
        UTV_LEVEL_ERROR,
        "%s() \"%s\"",
        __FUNCTION__,
        UtvGetMessageString( result ) );
    }
  }

  /*
   * Call the CEM specific session termination routines for Live Support.
   */

  UtvCEMLiveSupportSessionShutdown();

  /*
   * Free up the session ID since the session has been terminated.
   */

  UtvFree( sessionId );

  if ( UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED != s_sessionState )
  {
    UtvMessage( UTV_LEVEL_INFO,
                "Live Support session (%c%c%c-%c%c%c-%c%c%c) terminated",
                sessionCode[0],
                sessionCode[1],
                sessionCode[2],
                sessionCode[3],
                sessionCode[4],
                sessionCode[5],
                sessionCode[6],
                sessionCode[7],
                sessionCode[8] );
  }

  return ( UTV_OK );
}



/**
 **********************************************************************************************
 *
 * Pause a Live Support session.
 *
 * This method saves the session information to persistent memory to allow the session to be
 * resumed later.  It then ends the device side of the Live Support processing.
 *
 * @return    UTV_OK if the session was successfully paused, otherwise one of the other
 *            UTV_RESULT values.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportSessionPause( void )
{
  UTV_RESULT result = UTV_OK;

  if ( !UtvLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvTime( &g_utvLiveSupportInfo.savedDate );

  UtvStrnCopy(
    g_utvLiveSupportInfo.sessionInfo.id,
    SESSION_ID_SIZE,
    s_sessionId,
    SESSION_ID_SIZE );

  UtvStrnCopy(
    g_utvLiveSupportInfo.sessionInfo.code,
    SESSION_CODE_SIZE,
    s_sessionCode,
    SESSION_CODE_SIZE );

  result =
    UtvInternetDeviceWebServiceLiveSupportGetSessionData1(
      g_utvLiveSupportInfo.sessionInfo.data,
      SESSION_DATA_SIZE );

  result = UtvPlatformPersistWrite();

  if ( UTV_OK == result )
  {
    /*
     * Set the session state to paused, this will ensure that the session is not terminated
     * to the NRS during the shutdown processing.
     */

    UtvLiveSupportSetSessionState( UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED );

    /*
     * Shutdown the session processing with the indication that this shutdown is internally
     * initiated.  The UTV_TRUE indicates the internal initiation.
     */

    UtvLiveSupportSessionShutdown( UTV_TRUE );
  }
  else
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );

    UtvMemset( &g_utvLiveSupportInfo, 0x00, sizeof( g_utvLiveSupportInfo ) );
    g_utvLiveSupportInfo.header.usSigBytes = UTV_LIVE_SUPPORT_INFO_SIG;
    g_utvLiveSupportInfo.header.usStructSize = sizeof ( g_utvLiveSupportInfo );
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Resume a Live Support session.
 *
 * This method attempts to resume a Live Support session.
 *
 * @return    UTV_OK if the session was successfully resumed, otherwise one of the other
 *            UTV_RESULT values.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportSessionResume( void )
{
  UTV_RESULT result = UTV_OK;
  UTV_TIME resumedDate;

  if ( UtvLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_ALREADY_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Check if the saved date in the session information is valid, if not there
   * is nothing to resume.
   */

  if ( g_utvLiveSupportInfo.savedDate == 0 )
  {
    result = UTV_LIVE_SUPPORT_SESSION_RESUME_NOT_AVAILABLE;
    UtvMessage( UTV_LEVEL_INFO, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Check if the saved session is too old to resume.
   */

  UtvTime( &resumedDate );

  if ( difftime( resumedDate, g_utvLiveSupportInfo.savedDate ) <= SESSION_RESUME_LIMIT_SECONDS )
  {
    result = UtvLiveSupportTestConnection();

    if ( UTV_OK == result )
    {
      UtvLiveSupportSessionInitiateInfo1 sessionInitiateInfo;
      UtvLiveSupportSessionInfo1 sessionInfo;

      UtvMemset( &sessionInitiateInfo, 0x00, sizeof( sessionInitiateInfo ) );

      sessionInitiateInfo.allowSessionCodeBypass = UTV_FALSE;
      sessionInitiateInfo.secureMediaStream = UTV_FALSE;
      sessionInitiateInfo.resumeExistingSession = UTV_TRUE;

      UtvByteCopy(
        (UTV_BYTE*)&sessionInfo,
        (UTV_BYTE*)&g_utvLiveSupportInfo.sessionInfo,
        sizeof( sessionInfo ) );

      result = UtvLiveSupportInitiateSession1( &sessionInitiateInfo, &sessionInfo );
    }

    if ( UTV_OK != result )
    {
      UtvMessage(
        UTV_LEVEL_ERROR,
        "%s() \"%s\"",
        __FUNCTION__,
        UtvGetMessageString( result ) );
    }
  }
  else
  {
    result = UTV_LIVE_SUPPORT_SESSION_RESUME_TOO_OLD;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
  }

  /*
   * Regardless of whether there was a failure or not the Live Support information will be
   * cleared and the persistent storage will be updated.
   */

  UtvMemset( &g_utvLiveSupportInfo, 0x00, sizeof( g_utvLiveSupportInfo ) );
  g_utvLiveSupportInfo.header.usSigBytes = UTV_LIVE_SUPPORT_INFO_SIG;
  g_utvLiveSupportInfo.header.usStructSize = sizeof ( g_utvLiveSupportInfo );

  if ( UTV_OK != UtvPlatformPersistWrite() )
  {
    UtvMessage(
      UTV_LEVEL_WARN,
      "%s() failed to clear and save persistent session information",
      __FUNCTION__ );
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Test Live Support connection to the NetReady Services.
 *
 * @return    UTV_OK if the Echo1 request is successfully sent to the NetReady Services.
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportTestConnection( void )
{
  /*
   * If the network isn't up yet then don't allow the session to be initiated.
   */

  if ( !UtvPlatformNetworkUp() )
  {
    UTV_RESULT result = UTV_NETWORK_NOT_UP;
    UtvMessage( UTV_LEVEL_WARN, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  return ( UtvInternetDeviceWebServiceEcho() );
}



/**
 **********************************************************************************************
 *
 * Checks if a LiveSupport session with a NetReady Services is active.
 *
 * @return    UTV_TRUE if there is an active LiveSupport session.
 * @return    UTV_FALSE if there is not an active LiveSupport session.
 *
 **********************************************************************************************
 */

UTV_BOOL UtvLiveSupportIsSessionActive( void )
{
  return ( NULL != s_sessionId );
}



/**
 **********************************************************************************************
 *
 * Initiate a LiveSupport session with the NetReady Services.
 *
 * @param       sessionCode - Buffer to hold the session code of the LiveSupport session.
 * @param       sessionCodeSize - Size of the session code buffer.
 *
 * @return      UTV_OK if the processing executed successfully
 * @return      Other UTV_RESULT value if errors occur.
 *
 * @deprecated  Use UtvLiveSupportInitiateSession1
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportInitiateSession( UTV_BYTE* sessionCode, UTV_UINT32 sessionCodeSize )
{
  UTV_RESULT result;
  UtvLiveSupportSessionInitiateInfo1 sessionInitiateInfo;
  UtvLiveSupportSessionInfo1 sessionInfo;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == sessionCode || SESSION_CODE_SIZE > sessionCodeSize )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvMemset( &sessionInitiateInfo, 0x00, sizeof( sessionInitiateInfo ) );

  /*
   * Initialize the 'allowSessionCodeBypass' and 'secureMediaStream' flags to UTV_FALSE to
   * ensure the correct default values.
   */

  sessionInitiateInfo.allowSessionCodeBypass = UTV_FALSE;
  sessionInitiateInfo.secureMediaStream = UTV_FALSE;

  result = UtvLiveSupportInitiateSession1( &sessionInitiateInfo, &sessionInfo );

  if ( UTV_OK == result )
  {
    UtvStrnCopy(
      sessionCode,
      sessionCodeSize,
      sessionInfo.code,
      UtvStrlen( sessionInfo.code ) );
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Initiate a LiveSupport session with the NetReady Services.
 *
 * @param     sessionInitiateInfo - Session initiation information.
 * @param     sessionInfo - Session information.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportInitiateSession1(
  UtvLiveSupportSessionInitiateInfo1* sessionInitiateInfo,
  UtvLiveSupportSessionInfo1* sessionInfo )
{
  UTV_RESULT result;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == sessionInitiateInfo || NULL == sessionInfo )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * If the network isn't up yet then don't allow the session to be initiated.
   */

  if ( !UtvPlatformNetworkUp() )
  {
    result = UTV_NETWORK_NOT_UP;
    UtvMessage( UTV_LEVEL_WARN, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  if ( UtvLiveSupportIsSessionActive() )
  {
    UtvMessage(
      UTV_LEVEL_ERROR,
      "%s: LiveSupport session (%s) already active",
      __FUNCTION__,
      s_sessionId );

    result = UTV_LIVE_SUPPORT_SESSION_INIT_ALREADY_ACTIVE;
    return ( result );
  }

#ifdef UTV_LIVE_SUPPORT_VIDEO_SECURE
  if ( UTV_TRUE == UtvLiveSupportVideoSecureMediaStream() &&
       UTV_TRUE == UtvPlatformLiveSupportVideoSecureMediaStream() )
  {
    sessionInitiateInfo->secureMediaStream = UTV_TRUE;
  }
#endif

  result =
    UtvInternetDeviceWebServiceLiveSupportInitiateSession1(
      sessionInitiateInfo,
      sessionInfo );

  if ( UTV_OK == result )
  {
    s_sessionId = UtvMalloc( SESSION_ID_SIZE );

    UtvStrnCopy(
      s_sessionId,
      SESSION_ID_SIZE,
      sessionInfo->id,
      UtvStrlen( sessionInfo->id ) );

    UtvStrnCopy(
      s_sessionCode,
      SESSION_CODE_SIZE,
      sessionInfo->code,
      UtvStrlen( sessionInfo->code ) );

    UtvLiveSupportSessionInit();

    UtvMessage( UTV_LEVEL_INFO,
                "Live Support session (%c%c%c-%c%c%c-%c%c%c) %s",
                sessionInfo->code[0],
                sessionInfo->code[1],
                sessionInfo->code[2],
                sessionInfo->code[3],
                sessionInfo->code[4],
                sessionInfo->code[5],
                sessionInfo->code[6],
                sessionInfo->code[7],
                sessionInfo->code[8],
                (g_utvLiveSupportInfo.savedDate == 0) ? "initiated" : "resumed" );
  }
  else
  {
    UtvMessage( UTV_LEVEL_ERROR,
                "%s: Failed to %s LiveSupport session \"%s\"",
                __FUNCTION__,
                (g_utvLiveSupportInfo.savedDate == 0) ? "initiate" : "resume",
                UtvGetMessageString( result ));

    result = UTV_LIVE_SUPPORT_SESSION_INIT_FAILED;
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Terminate a LiveSupport session with the NetReady Services.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportTerminateSession()
{
  UTV_RESULT result = UTV_OK;

  if ( UtvLiveSupportIsSessionActive() )
  {
    /*
     * Shutdown the session processing with the indication that this shutdown is internally
     * initiated.  The UTV_TRUE indicates the internal initiation.
     */

    UtvLiveSupportSessionShutdown( UTV_TRUE );
  }
  else
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s: no LiveSupport session active", __FUNCTION__ );

    result = UTV_LIVE_SUPPORT_SESSION_TERMINATE_NOT_ACTIVE;
  }
  
  return ( result );
}



/**
 **********************************************************************************************
 *
 * Retrieves the current Live Support session identifier.
 *
 * @param     pszSessionId - pointer to a buffer to hold the session identifier.
 * @param     uiSessionIdSize - Size of the session idenifier buffer.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportGetSessionId( UTV_BYTE* pszSessionId, UTV_UINT32 uiSessionIdSize )
{
  UTV_RESULT result = UTV_OK;

  if ( NULL == pszSessionId || SESSION_ID_SIZE > uiSessionIdSize )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  if ( !UtvLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvStrnCopy( pszSessionId, uiSessionIdSize, s_sessionId, UtvStrlen( s_sessionId ) );

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Sets the Live Support session identifier.
 *
 * @param     pszSessionId - Session identifier.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportSetSessionId( UTV_BYTE* pszSessionId )
{
  UTV_RESULT result = UTV_INVALID_PARAMETER;

  if ( NULL != pszSessionId )
  {
    if ( NULL == s_sessionId )
    {
      s_sessionId = UtvMalloc( SESSION_ID_SIZE );
    }

    UtvStrnCopy( s_sessionId, SESSION_ID_SIZE, pszSessionId, UtvStrlen( pszSessionId ) );

    result = UTV_OK;
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Checks if a LiveSupport session with the NetReady Services is active.
 *
 * @param     pSessionState - Buffer to hold the session state.
 *              - UTV_LIVE_SUPPORT_SESSION_STATE_NONE if the session is not active
 *              - UTV_LIVE_SUPPORT_SESSION_STATE_INITIALIZING if the session is initializing
 *              - UTV_LIVE_SUPPORT_SESSION_STATE_JOINING if the session is waiting for a
 *                technical support representative to join the session.
 *              - UTV_LIVE_SUPPORT_SESSION_STATE_RUNNING if the session is active and a
 *                technical support representative has joined the session.
 *              - UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED if the session is paused.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportGetSessionState( UtvLiveSupportSessionState_t* pSessionState )
{
  UTV_RESULT result = UTV_INVALID_PARAMETER;

  if ( pSessionState != NULL )
  {
    *pSessionState = s_sessionState;
    result = UTV_OK;
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Sets the Live Support session state.
 *
 * @param     sessionState - Session state with possible values of:
 *
 *                - UTV_LIVE_SUPPORT_SESSION_STATE_NONE
 *                - UTV_LIVE_SUPPORT_SESSION_STATE_INITIALIZING
 *                - UTV_LIVE_SUPPORT_SESSION_STATE_JOINING
 *                - UTV_LIVE_SUPPORT_SESSION_STATE_RUNNING
 *                - UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportSetSessionState( UtvLiveSupportSessionState_t sessionState )
{
  s_sessionState = sessionState;

  return ( UTV_OK );
}



/**
 **********************************************************************************************
 *
 * Retrieve a copy of the LiveSupport session code.
 *
 * @param     pbSessionCode - Buffer to hold the session code of the LiveSupport session.
 * @param     uiSessionCodeSize - Size of the session code buffer.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportGetSessionCode(
  UTV_BYTE* pbSessionCode,
  UTV_UINT32 uiSessionCodeSize )
{
  UTV_RESULT result = UTV_OK;

  if ( NULL == pbSessionCode || SESSION_CODE_SIZE > uiSessionCodeSize )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  if ( !UtvLiveSupportIsSessionActive() )
  {
    result = UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvStrnCopy( pbSessionCode, uiSessionCodeSize, s_sessionCode, UtvStrlen( s_sessionCode ) );

  return ( result );
}



#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
/**
 **********************************************************************************************
 *
 * Post handler for the LIVE_SUPPORT_INITIATE_SESSION command.
 *
 * @param     inputs -
 *
 * @param     outputs -
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportInitiateSession( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;
  UtvLiveSupportSessionInitiateInfo1 sessionInitiateInfo;
  UtvLiveSupportSessionInfo1 sessionInfo;
  cJSON* input = NULL;

  if ( NULL == outputs || cJSON_Object != outputs->type )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  UtvMemset( &sessionInitiateInfo, 0x00, sizeof( sessionInitiateInfo ) );

  /*
   * Initialize the 'allowSessionCodeBypass' flag to UTV_FALSE to ensure the correct
   * default value if the 'ALLOW_SESSION_CODE_BYPASS" option is not part of the inputs
   * information.
   */

  sessionInitiateInfo.allowSessionCodeBypass = UTV_FALSE;

  /*
   * There are optional inputs allowed for the initiate session request.  This is the check
   * to see if the user has passed any inputs and so then parses them for the optional values.
   */

  if ( NULL != inputs && cJSON_Object == inputs->type )
  {
    /*
     * The 'Allow Session Code Bypass' flag is an optional parameter, check if it exists
     * and if so then attempt to get the correct value from it check to make sure the value
     * is either cJSON_False or cJSON_True.
     */

    if ( NULL != ( input = cJSON_GetObjectItem( inputs, "ALLOW_SESSION_CODE_BYPASS" ) ) &&
         ( cJSON_False == input->type || cJSON_True == input->type ) )
    {

      sessionInitiateInfo.allowSessionCodeBypass = ( cJSON_True == input->type );
    }
  }

  result = UtvLiveSupportInitiateSession1( &sessionInitiateInfo, &sessionInfo );

  if ( UTV_OK == result )
  {
    cJSON_AddStringToObject( outputs, "SESSION_CODE", (char*)sessionInfo.code );
    UtvLiveSupportSetSessionState( UTV_LIVE_SUPPORT_SESSION_STATE_JOINING );
  }

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */



#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
/**
 **********************************************************************************************
 *
 * Post handler for the LIVE_SUPPORT_TERMINATE_SESSION command.
 *
 * @param     inputs -
 *
 * @param     outputs -
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportTerminateSession( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result;

  result = UtvLiveSupportTerminateSession();

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */



#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
/**
 **********************************************************************************************
 *
 * Post handler for the LIVE_SUPPORT_GET_SESSION_STATE command.
 *
 * @param     inputs -
 *
 * @param     outputs -
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

static UTV_RESULT s_LiveSupportGetSessionState( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  UTV_BYTE session_code[SESSION_CODE_SIZE];
  char* state = "";

  if ( NULL == outputs || cJSON_Object != outputs->type )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  switch ( s_sessionState )
  {
    case UTV_LIVE_SUPPORT_SESSION_STATE_NONE:
      state = "NONE";
      break;

    case UTV_LIVE_SUPPORT_SESSION_STATE_INITIALIZING:
      state = "INITIALIZING";
      break;

    case UTV_LIVE_SUPPORT_SESSION_STATE_JOINING:
      state = "JOIN";
      break;

    case UTV_LIVE_SUPPORT_SESSION_STATE_RUNNING:
      state = "RUNNING";
      break;

    case UTV_LIVE_SUPPORT_SESSION_STATE_PAUSED:
      state = "PAUSED";
      break;

    default:
      state = "UNKNOWN";
      break;
  }

  cJSON_AddStringToObject( outputs, "STATE", state );

  if ( UtvLiveSupportIsSessionActive() )
  {
    UtvLiveSupportGetSessionCode( session_code, SESSION_CODE_SIZE );
    cJSON_AddStringToObject( outputs, "SESSION_CODE", (char*)session_code );
  }
  else
  {
    cJSON_AddStringToObject( outputs, "SESSION_CODE", "" );
  }

  return ( result );
}
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */
