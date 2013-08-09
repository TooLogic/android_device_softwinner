/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010-2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Declaration of the video support for the LiveSupport interface.
 *
 * @file      utv-livesupport-video.h
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: 5954 $
 *            $Author: btaylor $
 *            $Date: 2011-12-19 23:11:54 -0500 (Mon, 19 Dec 2011) $
 *
 **********************************************************************************************
 */

#ifndef LIVESUPPORT_VIDEO_H
#define LIVESUPPORT_VIDEO_H

#define MAX_MRS_ADDRESS_LEN 128

typedef struct
{
  UTV_BOOL                    secureMediaStream;
  UTV_BYTE                    mrsAddress[MAX_MRS_ADDRESS_LEN];
  UTV_INT32                   mrsPort;
  UTV_INT32                   frameWidth;
  UTV_INT32                   frameHeight;
  UTV_INT32                   frameRateNumerator;
  UTV_INT32                   frameRateDenominator;
} UtvLiveSupportMediaStreamInfo1;

void UtvLiveSupportVideoAgentInit( void );
void UtvLiveSupportVideoAgentShutdown( void );
void UtvLiveSupportVideoSessionInit( void );
void UtvLiveSupportVideoSessionShutdown( void );
UTV_RESULT UtvLiveSupportVideoSessionStart1( UtvLiveSupportMediaStreamInfo1* mediaStreamInfo );
UTV_BOOL UtvLiveSupportVideoSecureMediaStream( void );

#endif /* LIVESUPPORT_VIDEO_H */
