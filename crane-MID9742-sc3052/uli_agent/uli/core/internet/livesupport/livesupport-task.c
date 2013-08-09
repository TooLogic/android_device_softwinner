/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Implementation of the task handling for the LiveSupport interface.
 *
 * @file      livesupport-task.c
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
 * Define the number of seconds that the Get Next Task request is allowed to wait for a task
 * before it returns with no task.  This value should be slightly less than the recv_timeout
 * for the SOAP context.  By default the SOAP context recv_timeout is setup to be 10 seconds.
 */
#define GET_NEXT_TASK_TIMEOUT_SECONDS 8

struct TaskCommandHandler
{
  struct TaskCommandHandler* next;
  UTV_BYTE* command_name;
  UTV_BYTE* command_description;
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs );
};



/*
 **********************************************************************************************
 *
 * STATIC VARS
 *
 **********************************************************************************************
 */

static struct TaskCommandHandler* s_handler_list = NULL;
static UTV_THREAD_HANDLE s_taskWorkerThread = 0;



/*
 **********************************************************************************************
 *
 * STATIC FUNCTIONS
 *
 **********************************************************************************************
 */

static void* task_worker( void* pArg );
static cJSON* process_task( cJSON* input );
static UTV_RESULT process_get_available_commands( cJSON* inputs, cJSON* outputs );



/**
 **********************************************************************************************
 *
 * Initialize the task handling for the Live Support sub-system as part of Agent
 * initialization.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportTaskAgentInit( void )
{
  /*
   * Register the command handlers for the commands that are initiated through the task
   * mechanism and handled by UpdateLogic implementations.
   */

  UtvLiveSupportTaskRegisterCommandHandler(
    (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_GET_AVAILABLE_COMMANDS",
    NULL,
    process_get_available_commands );
}



/**
 **********************************************************************************************
 *
 * Shutdown the task handling for the Live Support sub-system as part of Agent shutdown.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportTaskAgentShutdown( void )
{
  struct TaskCommandHandler* handler;

  while ( NULL != s_handler_list )
  {
    handler = s_handler_list;
    s_handler_list = handler->next;

    if ( NULL != handler->command_description )
    {
      UtvFree( handler->command_description );
    }

    UtvFree( handler->command_name );
    UtvFree( handler );
  }
}



/**
 **********************************************************************************************
 *
 * Startup the task handling for a Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportTaskSessionInit( void )
{
  if ( 0 == s_taskWorkerThread )
  {
    UtvThreadCreate( &s_taskWorkerThread, task_worker, NULL );
  }
}



/**
 **********************************************************************************************
 *
 * Shutdown the task handling for a Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportTaskSessionShutdown( void )
{
  if ( 0 != s_taskWorkerThread )
  {
    UtvThreadKill( &s_taskWorkerThread );
    s_taskWorkerThread = 0;
  }
}



/**
 **********************************************************************************************
 *
 * Register a command handler for a task command.
 *
 * @param     command_name - Name of the command to handle.
 *
 * @param     command_description - Description of the command to handle.  This is a string in
 *                                  JSON format.
 *
 * @param     command_handler - Function pointer to execute for the given command.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportTaskRegisterCommandHandler(
  UTV_BYTE* command_name,
  UTV_BYTE* command_description,
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs ) )
{
  UTV_UINT32 length;
  struct TaskCommandHandler* handler = UtvMalloc( sizeof(struct TaskCommandHandler) );

  length = UtvStrlen( command_name );

  handler->command_name = UtvMalloc( length + 1 );
  UtvStrnCopy( handler->command_name,
               length + 1,
               command_name,
               length );

  if ( NULL != command_description )
  {
    length = UtvStrlen( command_description );
    handler->command_description = UtvMalloc( length + 1 );
    UtvStrnCopy( handler->command_description,
                 length + 1,
                 command_description,
                 length );
  }

  handler->command_handler = command_handler;

  handler->next = s_handler_list;
  s_handler_list = handler;
}



/**
 **********************************************************************************************
 *
 * Worker thread to poll and process tasks from the NetReady Services while a live support
 * session is active.
 *
 * @param     pArg - Thread arguments, points to a soap context.
 *
 * @return    NULL
 *
 **********************************************************************************************
 */

static void* task_worker( void* pArg )
{
  UTV_RESULT result;
  cJSON* task;
  cJSON* taskId;
  cJSON* output;
  UTV_BYTE* sessionId;

  sessionId = UtvMalloc( SESSION_ID_SIZE );
  UtvLiveSupportGetSessionId( sessionId, SESSION_ID_SIZE );

  while ( UtvLiveSupportIsSessionActive() )
  {
    result =
      UtvInternetDeviceWebServiceLiveSupportGetNextTask(
        sessionId,
        GET_NEXT_TASK_TIMEOUT_SECONDS,
        (void**)&task );

    if ( UTV_OK == result )
    {
      if ( task )
      {
        taskId = cJSON_GetObjectItem( task, "TASK_ID" );

        /*
         * If the taskId is -1 then the session has been terminated.
         */

        if ( -1 == taskId->valueint )
        {
          /*
           * This is an early exit so make sure we free up the task object.
           */

          cJSON_Delete( task );
          break;
        }
      
        UtvLiveSupportSetSessionState( UTV_LIVE_SUPPORT_SESSION_STATE_RUNNING );

        output = process_task( task );

        UtvInternetDeviceWebServiceLiveSupportCompleteTask(
          sessionId,
          taskId->valueint,
          output );

        cJSON_Delete( task );
      }
    }
    else
    {
      UtvMessage( UTV_LEVEL_ERROR,
                  "%s: Error occured getting next task, terminating task worker \"%s\"",
                  __FUNCTION__,
                  UtvGetMessageString( result ) );
      break;
    }
  }

  UtvFree( sessionId );

  s_taskWorkerThread = 0;

  /*
   * If the session is terminated by the Agent instead of the NetReady Services then the
   * session ID is already zero and there is no need to print anything or set the session
   * ID to zero.
   */

  if ( UtvLiveSupportIsSessionActive() )
  {
    /*
     * If the result from the get next task request is UTV_OK it means the session was
     * terminated from the NetReady Services and not internally initiated so pass FALSE to
     * the session shutdown processing.
     *
     * If the result from the get next task request is NOT UTV_OK it means an error occurred
     * within this processing and that the shutdown is being initiated internally so pass
     * TRUE to the session shutdown processing.
     */

    UtvLiveSupportSessionShutdown( ( result != UTV_OK ) );
  }

  UTV_THREAD_EXIT;
}



/**
 **********************************************************************************************
 *
 * Process a task received from the NetReady Services.
 *
 * @param     input - JSON object containing the task information.
 *
 * @return    Output for the given task input.
 *
 **********************************************************************************************
 */

static cJSON* process_task( cJSON* input )
{
  struct TaskCommandHandler* handler;
  UTV_RESULT result;
  UTV_BOOL failedRequest = UTV_FALSE;
  int idx;
  cJSON* requests;
  cJSON* request;
  cJSON* command;
  cJSON* inputs;
  cJSON* commandName;
  UTV_UINT32 commandNameLength;
  cJSON* output;
  cJSON* responses;
  cJSON* response;
  cJSON* outputs;

  responses = cJSON_CreateArray();

  requests = cJSON_GetObjectItem( input, "REQUESTS" );

  for ( idx = 0; idx < cJSON_GetArraySize( requests ); idx++ )
  {
    request = cJSON_GetArrayItem( requests, idx );
    command = cJSON_DetachItemFromObject( request, "COMMAND" );
    inputs = cJSON_GetObjectItem( request, "INPUTS" );
    commandName = cJSON_GetObjectItem( command, "NAME" );
    commandNameLength = UtvStrlen( (UTV_BYTE*) commandName->valuestring );

    response = cJSON_CreateObject();
    outputs = cJSON_CreateArray();

    cJSON_AddItemToObject( response, "COMMAND", command );

    if ( !failedRequest )
    {
      handler = s_handler_list;

      while ( handler != NULL )
      {
        if ( commandNameLength == UtvStrlen( handler->command_name ) )
        {
          if ( !UtvMemcmp( commandName->valuestring, handler->command_name, commandNameLength ) )
          {
            result = handler->command_handler( inputs, outputs );
            break;
          }
        }

        handler = handler->next;
      }

      if ( NULL == handler )
      {
        UtvMessage( UTV_LEVEL_WARN,
                    "%s: Unknown command \"%s\"",
                    __FUNCTION__,
                    commandName->valuestring );

        result = UTV_LIVE_SUPPORT_COMMAND_UNKNOWN;
      }

      failedRequest = ( result != UTV_OK );
    }
    else
    {
        UtvMessage( UTV_LEVEL_WARN,
                    "%s: Command \"%s\" not processed",
                    __FUNCTION__,
                    commandName->valuestring );

      result = UTV_LIVE_SUPPORT_COMMAND_NOT_PROCESSED;
    }

    cJSON_AddNumberToObject( response, "RESULT", result );
    cJSON_AddItemToObject( response, "OUTPUTS", outputs );
    cJSON_AddItemToArray( responses, response );
  }

  output = cJSON_CreateObject();
  cJSON_AddItemToObject( output, "RESPONSES", responses );

  return ( output );
}



/**
 **********************************************************************************************
 *
 * Retrieves the avaialble commands supported by the Agent.
 *
 * @param     inputs - JSON objects containing the input for the command.
 * @param     outputs - JSON objects containing the results for the command.
 *
 * @return    UTV_OK if the processing executed successfully
 * @return    Other UTV_RESULT value if errors occur.
 *
 **********************************************************************************************
 */

static UTV_RESULT process_get_available_commands( cJSON* inputs, cJSON* outputs )
{
  struct TaskCommandHandler* handler;
  cJSON* output;
  cJSON* commands;
  cJSON* command;

  commands = cJSON_CreateArray();

  handler = s_handler_list;

  while ( handler != NULL )
  {
    if ( NULL == handler->command_description )
    {
      command = cJSON_CreateObject();
      cJSON_AddStringToObject( command, "ID", (const char*) handler->command_name );
    }
    else
    {
      command = cJSON_Parse( (const char*) handler->command_description );

      if ( NULL == command )
      {
        /* TODO: Is this what we want for default behavior? */
        /*
         * If the processing of the command description was not successful just add the
         * command name.
         */

        UtvMessage( UTV_LEVEL_ERROR,
                    "%s: Failed to parse command description \"%s\"",
                    __FUNCTION__,
                    handler->command_description );

        command = cJSON_CreateObject();
        cJSON_AddStringToObject( command, "ID", (const char*) handler->command_name );
      }
    }

    cJSON_AddItemToArray( commands, command );
    handler = handler->next;
  }

  output = cJSON_CreateObject();
  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", commands );

  cJSON_AddItemToArray( outputs, output );

  return ( UTV_OK );
}
