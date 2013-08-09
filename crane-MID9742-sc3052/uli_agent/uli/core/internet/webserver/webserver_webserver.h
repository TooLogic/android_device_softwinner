/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
**********************************************************************************************
*
* Declaration of the web server interface.
*
* @file      webserver_webserver.h
*
* @author    Chris Nigbur
*
* @version   $Revision: $
*            $Author: $
*            $Date: $
*
**********************************************************************************************
*/

#ifndef __WEBSERVER_WEBSERVER_H__
#define __WEBSERVER_WEBSERVER_H__

#include "webserver_webserviceH.h"

void UtvWebServerInit( void );
void UtvWebServerShutdown( void );

void UtvWebServerRegisterPostHandler(
  UTV_BYTE* end_point,
  int (*post_handler)(struct soap* soap) );

#endif /* __WEBSERVER_WEBSERVER_H__ */
