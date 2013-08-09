/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Declaration of the task handling for the LiveSupport interface.
 *
 * @file      utv-livesupport-task.h
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: $
 *            $Author: $
 *            $Date: 2011-12-19 23:11:54 -0500 (Mon, 19 Dec 2011) $
 *
 **********************************************************************************************
 */

#ifndef LIVESUPPORT_TASK_H
#define LIVESUPPORT_TASK_H

#include "cJSON.h"

#define UTV_LIVE_SUPPORT_ULI_COMMAND_DESCRIPTION    NULL

void UtvLiveSupportTaskAgentInit( void );
void UtvLiveSupportTaskAgentShutdown( void );
void UtvLiveSupportTaskSessionInit( void );
void UtvLiveSupportTaskSessionShutdown( void );

void UtvLiveSupportTaskRegisterCommandHandler(
  UTV_BYTE* command_name,
  UTV_BYTE* command_description,
  UTV_RESULT (*command_handler)( cJSON* inputs, cJSON* outputs ) );

#endif /* LIVESUPPORT_TASK_H */
