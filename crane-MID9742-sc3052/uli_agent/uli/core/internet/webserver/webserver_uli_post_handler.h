/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Declaration of the ULI web server endpoint post handler.
 *
 * @file      webserver_uli_post_handler.h
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: $
 *            $Author: $
 *            $Date: $
 *
 **********************************************************************************************
 */

#ifndef WEBSERVER_ULI_POST_HANDLER_H
#define WEBSERVER_ULI_POST_HANDLER_H

#include "utv.h"

void UtvWebServerUliEndPointInit( void );
void UtvWebServerUliEndPointShutdown( void );

void UtvWebServerUliRegisterPostCommandHandler(
  UTV_BYTE* command_name,
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs ) );

#endif /* WEBSERVER_ULI_POST_HANDLER_H */
