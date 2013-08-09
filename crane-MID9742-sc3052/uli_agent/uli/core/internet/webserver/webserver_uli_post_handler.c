/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Implementation of the ULI web server endpoint post handler.
 *
 * @file      webserver_uli_post_handler.c
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: $
 *            $Author: $
 *            $Date: $
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

struct CommandHandler
{
  struct CommandHandler* next;
  UTV_BYTE* command_name;
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs );
};



/*
 **********************************************************************************************
 *
 * STATIC VARS
 *
 **********************************************************************************************
 */

static struct CommandHandler* s_handler_list = NULL;
static char* s_last_http_post_response = NULL;



/*
 **********************************************************************************************
 *
 * STATIC FUNCTIONS
 *
 **********************************************************************************************
 */

static int handle_uli_post(struct soap *soap);
static const char* process_uli_post( const char* form_data );



/**
 **********************************************************************************************
 *
 * Initialize the ULI web server endpoint.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerUliEndPointInit( void )
{
  UtvWebServerRegisterPostHandler( (UTV_BYTE*)"/Uli", handle_uli_post );
}



/**
 **********************************************************************************************
 *
 * Shutdown the ULI web server endpoint.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerUliEndPointShutdown( void )
{
  /* TODO: Fill in the necessary shutdown activities */
}



/**
 **********************************************************************************************
 *
 * Register a command handler for a command to the ULI POST handler.  This method will look in
 * the existing handler list to determine if this command is already registered.
 *
 * @param     command_name - Name of the command to handle.
 *
 * @param     command_handler - Function pointer to execute for the given command.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerUliRegisterPostCommandHandler(
  UTV_BYTE* command_name,
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs ) )
{
  struct CommandHandler* handler;
  UTV_UINT32 command_name_length = UtvStrlen( (UTV_BYTE*)command_name );

  /*
   * Search through the existing handler list for the given command.  If the command is
   * found then change the function pointer if it is different.  If the command is not found
   * then add it to the list.
   */

  handler = s_handler_list;

  while ( handler != NULL )
  {
    if ( !UtvMemcmp( command_name, handler->command_name, UtvStrlen( handler->command_name ) ) )
    {
      break;
    }

    handler = handler->next;
  }

  if ( handler != NULL )
  {
    /*
     * The command is already registered, check if the handler is different.
     */

    if ( handler->command_handler != command_handler )
    {
      UtvMessage( UTV_LEVEL_INFO,
                  "%s: Command %s already registered, updating function pointer",
                  __FUNCTION__,
                  command_name );

      handler->command_handler = command_handler;
    }
    else
    {
      UtvMessage( UTV_LEVEL_INFO,
                  "%s: Command %s already registered",
                  __FUNCTION__,
                  command_name );
    }
  }
  else
  {
    UtvMessage( UTV_LEVEL_INFO,
                "%s: Registering command %s",
                __FUNCTION__,
                command_name );

    handler = UtvMalloc( sizeof(struct CommandHandler) );
    handler->command_name = UtvMalloc( command_name_length + 1 );
    UtvStrnCopy( handler->command_name,
                 command_name_length + 1,
                 command_name,
                 command_name_length );

    handler->command_handler = command_handler;

    handler->next = s_handler_list;
    s_handler_list = handler;
  }
}



/**
 **********************************************************************************************
 *
 * Handle HTTP POST to UI URL.
 *
 * @param     soap - Pointer to the soap context.
 *
 * @return    SOAP_OK on successful handling of the POST.
 *
 **********************************************************************************************
 */

static int handle_uli_post( struct soap *soap )
{
  const char* response_data;
  char* form_data = form(soap);

  /* fprintf(stderr, "HTTP POST to ULI URL:\n%s\n", form_data); */

  /* chop off the leading '?' */
  if(form_data && *form_data != '\0')
    form_data = &(form_data[1]);

  response_data = process_uli_post(form_data);

  soap->http_content = "application/json";
  soap_response(soap, SOAP_FILE);
  soap_send(soap, response_data);
  soap_end_send(soap);

  return SOAP_OK;
}



/**
 **********************************************************************************************
 *
 * Process a ULI HTTP POST recieved by the web server.
 *
 * @param     form_data - Message posted to the web server.
 *
 * @return    Response to the posted message.
 *
 * @todo      Add additional documentation for the parameters and return values indicating
 *            expected content.  Both are JSON formatted strings and have specific formatting.
 *
 **********************************************************************************************
 */

static const char* process_uli_post( const char* form_data )
{
  struct CommandHandler* handler;
  UTV_RESULT result = UTV_UNKNOWN;
  cJSON* input = NULL;
  cJSON* requests = NULL;
  cJSON* request = NULL;
  cJSON* command = NULL;
  cJSON* command_name = NULL;
  cJSON* inputs = NULL;
  cJSON* responses = NULL;
  cJSON* response = NULL;
  cJSON* outputs = NULL;
  cJSON* output = NULL;
  char* command_name_string = NULL;
  int idx = 0;

  /* treat missing input the same as empty input */
  if ( !form_data )
  {
    form_data = "";
  }

  /* process requests and form responses */

  /*
   * TODO: Write a validation routine to verify the contents of the form_data.  If it does
   *       not match the expected input then fail the request.
   */

  responses = cJSON_CreateArray();

  input = cJSON_Parse( form_data );

  if ( NULL != input )
  {
    requests = cJSON_GetObjectItem( input, "REQUESTS" );
  }

  if ( NULL != input && NULL != requests )
  {
    for ( idx = 0; idx < cJSON_GetArraySize( requests ); idx++ )
    {
      request = cJSON_GetArrayItem( requests, idx );
      command = cJSON_DetachItemFromObject( request, "COMMAND" );
      command_name = cJSON_GetObjectItem( command, "NAME" );
      command_name_string = command_name->valuestring ? command_name->valuestring : "";
      inputs = cJSON_GetObjectItem( request, "INPUTS" );

      outputs = cJSON_CreateObject();

      handler = s_handler_list;

      while ( handler != NULL )
      {
        if ( !UtvMemcmp( command_name_string, handler->command_name, UtvStrlen( handler->command_name ) ) )
        {
          result = handler->command_handler( inputs, outputs );
          break;
        }

        handler = handler->next;
      }

      response = cJSON_CreateObject();
      cJSON_AddItemToObject( response, "COMMAND", command );
      cJSON_AddNumberToObject( response, "RESULT", result );
      cJSON_AddItemToObject( response, "OUTPUTS", outputs );
      cJSON_AddItemToArray( responses, response );
    }

    /* create final output */
    output = cJSON_CreateObject();
    cJSON_AddItemToObject( output, "RESPONSES", responses );

    /* turn into string */
    if ( s_last_http_post_response )
    {
      UtvFree( s_last_http_post_response );
    }

    s_last_http_post_response = cJSON_PrintUnformatted( output );
  }
  else
  {
    UtvMessage( UTV_LEVEL_INFO,
                "%s: Failed to parse the form_data or failed to find REQUESTS",
                __FUNCTION__ );

    /*
     * Failed to parse the form_data or failed to find the necessary information in the JSON
     * objects.  Either way, report the error.
     */

    if( s_last_http_post_response )
    {
      UtvFree( s_last_http_post_response );
    }

    s_last_http_post_response = UtvMalloc( 2 );
    UtvStrcat( (UTV_BYTE*)s_last_http_post_response, (UTV_BYTE*)"" );
  }

  cJSON_Delete( input );
  cJSON_Delete( output );

  return ( s_last_http_post_response );
}
