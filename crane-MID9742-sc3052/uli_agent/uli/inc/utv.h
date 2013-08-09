/* utv.h: Main include file for UpdateTV SDK modules

 Copyright (c) 2004 - 2011 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

 Who           Date        Edit
 Bob Taylor    01/16/2012  Version to 3.2.3.3
                           Fixed UtvPlatformOnProvisioned() fixed to return encryption type determined from checkSecurityModel().
 Bob Taylor    12/26/2011  Version to 3.2.3.2
                           Fixed checkSecurityModel() args in non-file-based configuration.
 Bob Taylor    12/26/2011  Version to 3.2.3.1
                           Added uiClearObjectSize arg.
 Bob Taylor    11/21/2011  Version to 3.2.1
                           Merged with ULI 3.2.1
 Seb Emonet    07/28/2011  Version to 1.6.31.
                           Added an interface to retrieve the update size from the supervisor
                           This will be used to check if it's necessary to clean the cache partition
 Seb Emonet    07/28/2011  Version to 1.6.30.
                           Fixed the "downloaded ended type unknown bug"
 Bob Taylor    07/25/2011  Version to 1.6.29.
                           TKT speedup.
 Bob Taylor    07/25/2011  Version to 1.6.28.
                           TKT export.  Released to Marvell only.
 Bob Taylor    07/21/2011  Version to 1.6.27.
                           Wtpsp hang recovery.
                           Removed sleep from download thread just in case it was ever non-zero.
                           Added mutex in/out debugging to try and catch the hang.
 Bob Taylor    07/18/2011  Version to 1.6.26.
                           New early boot initialization logic.
 Bob Taylor    07/14/2011  Version to 1.6.25.
                           PR delete dev and tmp dirs plus permission setting.  First working PR build.
 Bob Taylor    07/12/2011  Version to 1.6.24.
                           Early persistent storage initialization to fix ULID not present log error.
 Bob Taylor    07/08/2011  Version to 1.6.23.
                           Initialization streamlining.
 Bob Taylor    07/06/2011  Version to 1.6.22.
                           ULPK insertion fail when transit key is already burned.
                           Disabled call to UtvPlatformSetSystemTime().
                           Added TKTest to validate transit key.
 Bob Taylor    07/06/2011  Version to 1.6.21.
                           SSUT auto-reboot.
 Bob Taylor    07/05/2011  Version to 1.6.20.
                           SSUT added.  Error logging on for all secure store functions.
 Bob Taylor    06/27/2011  Version to 1.6.19.
                           Removed setting currentDeliveryMode to zero when there is no schedule
                           available in a given delivery mode.  Reentrancy was causing problems.
 Bob Taylor    06/26/2011  Version to 1.6.18.
                           Fixed PR conf read and path names.
                           Changed manifest access from persistent ("active") to read-only ("backup").
                           Remove percent complete debug message.
 Bob Taylor    06/21/2011  Version to 1.6.17.
                           Open image in UtvDownloadScheduledUpdate() so that download retry doesn't
                           have to re-download the component directory.  Add better 0xA8 handling.
 Seb Emonet    06/15/2011  Version to 1.6.16.
                           Set the update schedule to null before calling UtvProjectOnScanForOTAUpdate() (fixes zombie update bug)
                           UtvDownloadScheduledUpdate returns the true error code if the pSchedule structure is null
                           call UtvImageClose in setUpdateSchedule if there's an active schedule
 Bob Taylor    06/13/2011  Version to 1.6.15.
                           Changed UtvProjectOnProvisioned() to UtvPlatformOnProvisioned() and added
                           operation arg to it for revocation.  PR Support in init.
                           Added transit key OTP check to GetULPK.
 Bob Taylor    06/11/2011  Version to 1.6.14.
                           Added TSR to display info.
                           Separated ULPK factory insert from ULPK registration access paths.
 Bob Taylor    06/09/2011  Version to 1.6.13.
                           Added multi-line updatelogic.txt support and build variant reporting do ulpk.dat
                           can come directly from read-only when in user and eng mode.
 Seb Emonet    06/06/2011  Version to 1.6.12.
                           Added UtvSetIsUpdateReadyToInstall in the JNI interface to be able to reset 
                           this state from the java supervisor
 Bob Taylor    06/05/2011  Version to 1.6.11.  
                           Changed UTV_INTERNET_PERS_FAILURE to "internet connection failure".
                           Added ThreadRemove to UtvPlatformInternetCommonDNS() to unable to create thread problem.
                           DNS retry count to 2 x 10 seconds.
                           Fixed bad logic in core-common-interactive.c when in RELEASE mode.
                           Fixed UtvManifestOpen() retry to be dependent on platform-config.h 
                           (new retry count is 1)
                           Bail from download ended and status change in update query.
                           Removed provisioning thread from network up because get schedule does the same thing.
                           This prevents twice the amount of work from being done.
 Bob Taylor    05/29/2011  Version to 1.6.10.  Moved UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL and 
                           UTV_TEST_NO_MOD_VERSION_UPDATE outside of UTV_DEBUG because they control 
                           behavior of the device that should persist in both debug and release builds.
 Bob Taylor    05/22/2011  Version to 1.6.9.  AES_UR support, UQ connect retry to 2.  UQ connect timeout to 5.
                           UQ disable logic ifdef'd out.
 Chris Nigbur  10/21/2010  Live support includes added.
 Bob Taylor    02/09/2010  Removed platform-file.h.
 Bob Taylor    02/23/2009  Added utv-image.h
 Bob Taylor    01/26/2009  2.0

*/

#ifndef __UTV_H__
#define __UTV_H__

/* Version
 */
#define UTV_AGENT_VERSION "UpdateLogic Agent 3.2.3.3"
#define UTV_AGENT_VERSION_MAJOR        3
#define UTV_AGENT_VERSION_MINOR        2
#define UTV_AGENT_VERSION_REVISION     3

/* Additional includes
 */
#include "platform-arch-types.h"
#include "platform-os-defines.h"
#include "platform-config.h"
#include "utv-image.h"
#include "utv-platform-public.h"
#include "utv-core.h"
#include "utv-platform.h"
#include "utv-platform-cem.h"
#include "utv-strings.h"

#ifdef UTV_JSON
#include "cJSON.h"
#include "utv-events.h"
#endif

#ifdef UTV_DEVICE_WEB_SERVICE
#include "stdsoap2.h"
#include <uuid/uuid.h>
#include "utv-storeDeviceData.h"
#endif

#ifdef UTV_WEBSERVER
#include "webserver_webserviceH.h"
#include "webserver_webserver.h"
#include "webserver_uli_post_handler.h"
#include "webserver_uli_events.h"
#include "httpform.h"
#endif

#ifdef UTV_LIVE_SUPPORT
#include "utv-livesupport.h"
#include "utv-livesupport-task.h"
#endif

#ifdef UTV_LIVE_SUPPORT_WEB_SERVER
#include "webserver_uli_post_handler.h"
#endif /* UTV_LIVE_SUPPORT_WEB_SERVER */

#ifdef UTV_LIVE_SUPPORT_VIDEO
#include "gst/gst.h"
#include "gst/app/gstappbuffer.h"
#include "gst/app/gstappsrc.h"
#include "utv-livesupport-video.h"
#endif

#endif /* __UTV_H__ */
