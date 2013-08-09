/* platform-config.h: UpdateTV Platform Adaptation Layer - Platform-specific defines.

 Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      07/05/2011  Added WTPSP category.
  Bob Taylor      06/04/2011  Added UTV_MANIFEST_OPEN_RETRY_COUNT.
  Bob Taylor      05/29/2011  Moved UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL and UTV_TEST_NO_MOD_VERSION_UPDATE '                                                  outside of UTV_DEBUG because they control 
  Jim Muchow      06/29/2011  Moved UTV_INTERNAL_SSL_INIT from here to Makefile
  Bob Taylor      06/22/2010  Loop test support.
  Bob Taylor      06/16/2010  Removed UTV_DEBUG define and defined fallback model group unconditionally.
  Seb Emonet      06/14/2010  Updated retry counts and timeouts.
  Seb Emonet      05/12/2010  UTV_TEST_FREQUENCY defined by default
  Bob Taylor      04/14/2010  Minimum updates per query set to 16.  Max stores check removed.  
                              UTV_TEST_FREQUENCY defined by default.
  Seb Emonet      04/09/2010  Added USB store index define.
  Jim Muchow      04/06/2010  Add some retry logic defines
  Seb Emonet      03/15/2010  OOB Milestone support.
  Bob Taylor      11/10/2009  Added UTV_PLATFORM_FALLBACK_QUERY_HOST.
  Bob Taylor      06/30/2009  Undefined UTV_TEST_VERIFY_PATTERN by default and added new
                              define called UTV_TEST_EXAMPLE_INSTALL_TEXT_DEFS (undefined by default)
                              which controls whether platform-install-file.c looks for the example
                              text definitions or not.
  Bob Taylor      01/26/2009  Created from version 1.9.21 for version 2.0
*/

#ifndef __PLATFORM_CONFIG_H__
#define __PLATFORM_CONFIG_H__

/* If this file is included outside of the standard Make environment
   then these macros are required to be defined here.  (Normally they're
   defined in the Makefile).  IF YOU USE A MAKEFILE SET THESE DEFINES IN THE
   MAKEFILE.
*/

/* ================== begin non-Makefile configuration section ==================== */
#ifndef UTV_STD_MAKE
/* Defines the main mode conditional compile controls
*/
#define UTV_INTERACTIVE_MODE
#define UTV_DELIVERY_BROADCAST
#define UTV_DELIVERY_INTERNET
#define UTV_DELIVERY_FILE
#define UTV_PLATFORM_PROVISIONER

/* The data source defines control which strings are displayed for debug only
   define one for each type of delivery mode used
*/

#define UTV_PLATFORM_DATA_BROADCAST_TS_FILE
#if 0
#define UTV_PLATFORM_DATA_BROADCAST_CUST
#define UTV_PLATFORM_DATA_BROADCAST_LINUX_DVBAPI
#define UTV_PLATFORM_DATA_BROADCAST_STATIC
#endif

#define UTV_PLATFORM_DATA_INTERNET_CLEAR_WINSOCK
#if 0
#define UTV_PLATFORM_DATA_INTERNET_CLEAR_SOCKET
#define UTV_PLATFORM_DATA_INTERNET_CLEAR_CUST
#endif

#define UTV_PLATFORM_DATA_INTERNET_SSL_WINSOCK
#if 0
#define UTV_PLATFORM_DATA_INTERNET_SSL_OPENSSL
#define UTV_PLATFORM_DATA_INTERNET_SSL_SOCKET
#define UTV_PLATFORM_DATA_INTERNET_SSL_CUST
#endif

#define UTV_PLATFORM_DATA_FILE_STDIO
#if 0
#define UTV_PLATFORM_DATA_FILE_CUST
#endif

/* Define UTV_CORE_PACKET_FILTER to enable packet filter as opposed to section filter interface.
   Used for test purposes for the broadcast TS file and Static data sources.
   Modern tuner hardware delivers ULI data in sections, not packets.
*/
#if defined(UTV_PLATFORM_DATA_BROADCAST_TS_FILE) || defined(UTV_PLATFORM_DATA_BROADCAST_STATIC)
#define UTV_CORE_PACKET_FILTER
#endif

/* Define UTV_OS_SINGLE_THREAD to enable single threaded mode.
   Used for single threaded OS implementation.
   Used for test purposes for the broadcast TS file data source to speed up testing.
 */
#ifdef UTV_PLATFORM_DATA_BROADCAST_TS_FILE
#define UTV_OS_SINGLE_THREAD
#endif

#endif /* non-Makefile config */
/* ================== end non-Makefile configuration section ==================== */

/* Define the maximum number of update candidates that we're willing to review at any given time */
#define UTV_PLATFORM_MAX_UPDATES                    16

/* Define the maximum number of time slots that we're tracking for brodacast downloads
*/
#define UTV_PLATFORM_MAX_SCHEDULE_SLOTS             1

/* Define the maximum number of components that we can expect in a given update
*/
#define UTV_PLATFORM_MAX_COMPONENTS                 100

/* Define the maximum number of modules that we can expect in a given download.
*/
#define UTV_PLATFORM_MAX_MODULES                    128

#define UTV_PLATFORM_MAX_SERIAL_NUMBER_SIZE         32

/* Update store related defines
*/
#define UTV_PLATFORM_UPDATE_STORE_MAX_STORES      4

#define UTV_PLATFORM_UPDATE_STORE_INVALID           0xFFFFFFFF
#define UTV_PLATFORM_UPDATE_STORE_FLAG_READ         0x00000001
#define UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE        0x00000002
#define UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE    0x00000004
#define UTV_PLATFORM_UPDATE_STORE_FLAG_APPEND       0x00000008

#define UTV_PLATFORM_UPDATE_STORE_0_INDEX    0
#define UTV_PLATFORM_UPDATE_STORE_0_SIZE     0x10000000
 
#define UTV_PLATFORM_UPDATE_STORE_1_INDEX    1
#define UTV_PLATFORM_UPDATE_STORE_1_SIZE     0x10000000

/* store 2 is the assignable store for USB downloads */
#define UTV_PLATFORM_UPDATE_USB_INDEX        2

/* component manifest index */
#define UTV_PLATFORM_UPDATE_MANIFEST_INDEX   3

/* FALLBACK control defines.  These values go into effect if the component manifest can't be read.

   UTV_PLATFORM_FALLBACK_SW_VER should be left set to zero to indicate loss of component manifest.

   Please note that if the value of UTV_PLATFORM_FALLBACK_ALLOW_ADDS is set to UTV_FALSE then
   the system will not allow updates if it loses its component manifest.
*/
#define UTV_PLATFORM_FALLBACK_MODEL_GROUP   1
#define UTV_PLATFORM_FALLBACK_SW_VER        1
#define UTV_PLATFORM_FALLBACK_MOD_VER       0
#define UTV_PLATFORM_FALLBACK_FTEST         0
#define UTV_PLATFORM_FALLBACK_FACT_TEST     0
#define UTV_PLATFORM_FALLBACK_LOOP_TEST     0
#define UTV_PLATFORM_FALLBACK_ALLOW_ADDS    UTV_TRUE
#define UTV_PLATFORM_FALLBACK_QUERY_HOST    "extdev.updatelogic.com"

#define UTV_MANIFEST_OPEN_RETRY_COUNT 		1

#ifdef UTV_DELIVERY_INTERNET
/* Specify connect and read timeouts for the internet connection
*/
#define UTV_PLATFORM_INTERNET_DNS_TIMEOUT_SEC           10

/* controls both crypt and clear DNS retry count */
#define UTV_PLATFORM_INTERNET_CONNECT_RETRY_COUNT       2

/* these control the clear timeout connects */
#define UTV_PLATFORM_INTERNET_CONNECT_TIMEOUT_SEC       10


#define UTV_PLATFORM_INTERNET_READ_TIMEOUT_INITIAL_SEC  30
#define UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC          10
#define UTV_PLATFORM_INTERNET_READ_RETRY_COUNT          5

#define UTV_PLATFORM_INTERNET_WRITE_TIMEOUT_SEC          UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC
#define UTV_PLATFORM_INTERNET_WRITE_RETRY_COUNT          UTV_PLATFORM_INTERNET_READ_RETRY_COUNT

/* controls update query retry count and retyr only.  it's deliberately small to keep the user from waiting */
#define UTV_PLATFORM_INTERNET_UQ_OPEN_RETRY             2
#define UTV_PLATFORM_INTERNET_UQ_READ_TIMEOUT_SEC       5

#define UTV_PLATFORM_INTERNET_DOWNLOAD_RETRY            5

#define UTV_PLATFORM_INTERNET_MODULE_RETRY              5

#endif

#ifdef UTV_DELIVERY_BROADCAST
/* Specify whether VCT scan is disabled or not 
*/
#define UTV_DISABLE_VCT_SCAN
#endif

/* Interactive mode must be enabled if factory decryption is supported. */
#ifdef UTV_FACTORY_DECRYPT_SUPPORT
#ifndef UTV_INTERACTIVE_MODE
#error UTV_INTERACTIVE_MODE  must be defined if factory UTV_FACTORY_DECRYPT_SUPPORT is defined.
#endif
#endif

/* Moved UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL and UTV_TEST_NO_MOD_VERSION_UPDATE outside of 
   UTV_DEBUG because they control behavior of the device that should persist in 
   both debug and release builds. */

/* Define this in order to prevent the NOC from being notified that a download has ended.
   This is useful during development when you are testing downloads and want to keep
   downloading the same update over and over again.  In production this value is NOT set so the NOC
   only sends a download once.
*/
 #define UTV_TEST_NO_DOWNLOAD_ENDED_SIGNAL 

/* Define a value that controls whether the module version is updated once a module is downloaded.
   When uncommented (and therefore defined) this allows the continuous download of an image for
   test purposes.
*/
 #define UTV_TEST_NO_MOD_VERSION_UPDATE

/* Test Definitions that allow you to test the system in the lab while UTV_DEBUG is defined.
*/

#ifdef UTV_DEBUG

/* Platform Data Source Dbg Define Values and Strings
*/
#define UTV_PLATFORM_DATA_BROADCAST_STATIC_STR            "BROADCAST_STATIC"
#define UTV_PLATFORM_DATA_BROADCAST_TS_FILE_STR           "BROADCAST_TS_FILE"
#define UTV_PLATFORM_DATA_BROADCAST_LINUX_DVBAPI_STR      "BROADCAST_LINUX_DVB_API"
#define UTV_PLATFORM_DATA_BROADCAST_CUST_STR              "BROADCAST_CUST"
#define UTV_PLATFORM_DATA_INTERNET_CLEAR_SOCKET_STR       "INTERNET_CLEAR_SOCKET"
#define UTV_PLATFORM_DATA_INTERNET_CLEAR_WINSOCK_STR      "INTERNET_CLEAR_WINSOCK"
#define UTV_PLATFORM_DATA_INTERNET_CLEAR_CUST_STR         "INTERNET_CLEAR_CUST"
#define UTV_PLATFORM_DATA_INTERNET_SSL_SOCKET_STR         "INTERNET_SSL_SOCKET"
#define UTV_PLATFORM_DATA_INTERNET_SSL_WINSOCK_STR        "INTERNET_SSL_WINSOCK"
#define UTV_PLATFORM_DATA_INTERNET_SSL_OPENSSL_STR        "INTERNET_SSL_OPENSSL"
#define UTV_PLATFORM_DATA_INTERNET_SSL_CUST_STR           "INTERNET_SSL_CUST"
#define UTV_PLATFORM_DATA_FILE_STDIO_STR                  "FILE_STDIO"
#define UTV_PLATFORM_DATA_FILE_CUST_STR                   "FILE_CUST"

/* Platform Delivery Mode Dbg Strings
*/
#define UTV_DELIVERY_MODE_BROADCAST_STR                   "BROADCAST"
#define UTV_DELIVERY_MODE_INTERNET_STR                    "INTERNET"
#define UTV_DELIVERY_MODE_FILE_STR                        "FILE"

/* Platform Installation Type Dbg Values and Strings
*/
#define UTV_PLATFORM_INSTALL_FILE_STR                     "FILE"
#define UTV_PLATFORM_INSTALL_CUST_STR                     "CUST"

#define UTV_PLATFORM_INSTALL_FILE                         1
#define UTV_PLATFORM_INSTALL_CUST                         2

/* This define controls the module storage type which is displayed for debug only. */
#define UTV_PLATFORM_INSTALL                             UTV_PLATFORM_INSTALL_FILE

/* When debugging define UTV_DDB_DEBUG to get messages when DDBs are received.
   This should only be necessary for initial debugging.
   For most debugging it is best to turn off these messages
   because the DDBs arrive so quickly it slows down the system.
*/
#define UTV_DDB_DEBUG

/* Define UTV_TEST_ABORT to turn on the abort testing mechanism.
*/
/* #define UTV_TEST_ABORT */

/* Allow unencrypted modules to be downloaded when compiled in DEBUG mode
*/
#define UTV_TEST_ALLOW_UNENCRYPTED

/* When UTV_TEST_FREQUENCY is defined the frequencies in g_uiFreqList[] are replaced
   with this single frequency. */
 #define UTV_TEST_FREQUENCY    729000000

/* Define this to use the example text definitions that come with the example script files
   shipped with the tools.
*/
/* #define UTV_TEST_EXAMPLE_INSTALL_TEXT_DEFS */

/* Define this in order to perform test pattern verification in addition to installation of components.
*/
/* #define UTV_TEST_VERIFY_PATTERN */

/* Define this to enable example text definitions for file and partition installation.
*/
/* #define UTV_TEST_EXAMPLE_INSTALL_TEXT_DEFS */

/* Define this in order to force the system to quit after a specified number of frequencies. */
/* #define UTV_TEST_ITERATION_LIMIT    10 */

/* Define to make wait times shorter for testing and demonstration */
/* #define UTV_TEST_DISCARD_LONG_WAITS */

/* Define this in order to prevent the NOC from being notified that a provisioned object has been downloaded.
   This is useful during development when you are testing provisioning and want to keep
   downloading the same object over and over again.  In production this value is NOT set so the NOC
   only sends a provisioned object once.
*/
/* #define UTV_TEST_NO_PROVISION_ENDED_SIGNAL */

/* Define a value that controls whether the module version is updated once a module is downloaded.
   When uncommented (and therefore defined) this allows the continuous download of an image for
   test purposes.
*/
/* #define UTV_TEST_NO_MOD_VERSION_UPDATE */

/* Define for execute testing.  Please note this mode is not designed for actual production execution testing which
   will update flash, cause the system to reboot, etc.  It is just for basic executable image testing.
*/
/* #define UTV_TEST_EXECUTE_IMAGE */

/* Define for enabling the TS file read looping.  By default a TS file is just read through once and then the Agent
   quits.  By defining UTV_TEST_TS_FILE_LOOP the file will be read over and over.
*/
/* #define UTV_TEST_TS_FILE_LOOP */

/* Enable output of debug messages */
#define UTV_TEST_MESSAGES

/* Define a value to be used by the TS_FILE data source
    which controls how fast the bits are delivered in the multi-threaded
    configuration.

   90000 is the default bit rate for StreamGenerator created streams.
*/
#define UTV_TEST_TS_FILE_BITRATE 90000

/* Abort test time definitions
*/
#define UTV_TEST_MINIMUM_ABORT_TIME 1000
#define UTV_TEST_MAXIMUM_ABORT_TIME 180000

/* High water reporting define
*/
/*#define UTV_TEST_REPORT_HIGH_WATER */

#endif /* UTV_DEBUG */

/* netDiag category values
    Add new values to the bottom of the list
    Each value is added by macro definition
    This allows the event logger to optionally translate categories to strings 
*/
#define UTV_CATLIST \
    UTV_CAT( UTV_CAT_PROVISIONER,       "Provisioner" ) \
    UTV_CAT( UTV_CAT_SECURITY,          "Security" ) \
    UTV_CAT( UTV_CAT_WTPSP,             "WTPSP" ) \
    UTV_CAT( UTV_CAT_INTERNET,          "Internet" ) \
    UTV_CAT( UTV_CAT_SSL,               "SSL" ) \
    UTV_CAT( UTV_CAT_PERSIST,           "Persist" ) \
    UTV_CAT( UTV_CAT_INSTALL,           "Install" ) \
    UTV_CAT( UTV_CAT_MANIFEST,          "Manifest" ) \
    UTV_CAT( UTV_CAT_PLATFORM,          "Platform" ) \

/* Define the global category codes
*/
#define UTV_CAT( code, text ) code,
typedef enum {
    UTV_CATLIST
    UTV_CATEGORY_COUNT
} UTV_CATEGORY;

#undef UTV_CAT

/* Event level definitions
*/
#define UTV_LEVEL_INFO      0x00000001
#define UTV_LEVEL_WARN      0x00000002
#define UTV_LEVEL_ERROR     0x00000004
#define UTV_LEVEL_TRACE     0x00000008
#define UTV_LEVEL_PERF      0x00000010
#define UTV_LEVEL_TEST      0x00000020
#define UTV_LEVEL_COMPAT    0x00000040
#define UTV_LEVEL_NETWORK   0x00000080
#define UTV_LEVEL_OOB_TEST  0x00000100
#define UTV_LEVEL_COUNT     9

/* Select the type of messages that are output to the serial console */
#define UTV_LEVEL_MESSAGES (UTV_LEVEL_INFO | UTV_LEVEL_WARN | UTV_LEVEL_ERROR | UTV_LEVEL_TEST | UTV_LEVEL_TRACE | UTV_LEVEL_OOB_TEST)

/* Select the type of messages that are sent to the NOC */
#define UTV_LEVEL_NOC_MESSAGES UTV_LEVEL_ERROR

/* events logging defines,  the initialization in UtvPlatformPersistRead() MUST match this */
#define UTV_PLATFORM_EVENT_CLASS_MASK           (UTV_LEVEL_WARN | UTV_LEVEL_ERROR | UTV_LEVEL_COMPAT | UTV_LEVEL_NETWORK)
#define UTV_PLATFORM_EVENT_CLASS_COUNT          4
#define UTV_PLATFORM_EVENTS_PER_CLASS           32
#define UTV_PLATFORM_ARGS_PER_EVENT             3
#define UTV_PLATFORM_EVENT_TEXT_STORE_SIZE      1024
#define UTV_PLATFORM_EVENT_TEXT_MAX_STR_SIZE    63

/* Special Definitions that effect system operation and tuning

   Define maximum size of the candidate tables.  These arrays of structures are used during the
   the best candidate discovery process.
   These values were chosen to allow the UpdateTV application to work in many different environments
   Caution: Do not make these any smaller without contacting UpdateLogic, Inc.
*/

/* Maximum number of frequency/PID structures that define a carousel candidate
   that we will track during DSI discovery.
*/
#define UTV_CAROUSEL_CANDIDATE_MAX 32

/* Maximum number of OUI/Model Group structures that define a group candidate
   that we will track during DSI parsing.
*/
#define UTV_GROUP_CANDIDATE_MAX 32

/* Maximum number of Module Information structures that define module candidates that we will
   track during DII parsing.  This value MUST be greater than or equal to the maximum number of
   modules in your largest component.
*/
#define UTV_DOWNLOAD_CANDIDATE_MAX UTV_PLATFORM_MAX_MODULES

/* Define single threaded support which includes a hook out to a yield during lengthy operations (decryption, for example)
   and switches the STATIC and TS_FILE data sources between a single threaded and multi-threaded pump.
*/
/*#define UTV_OS_SINGLE_THREAD */

/* Define the depth of the section queue
*/
#define UTV_SECTION_QUEUE_DEPTH    8

/* UpdateTV Network Timeouts (in milliseconds)
   [CAUTION: These values have been tested.  Change with caution (if neccessary) to support your platform]
*/
#define UTV_WAIT_SLEEP_LONG  (24 * 60 * 60 * 1000)
#define UTV_WAIT_SLEEP_SHORT      (30 * 60 * 1000)
#define UTV_WAIT_UP_EARLY          (5 * 60 * 1000)
#define UTV_DOWNLOAD_TOO_SOON                   0   /* Filter modules during download event - this will test the download time of all by default */
#define UTV_DOWNLOAD_TOO_FAR (25 * 60 * 60 * 1000)
#define UTV_WAIT_DOWNLOAD_DONE_DELAY    0
#ifdef  UTV_TEST_DISCARD_LONG_WAITS
#define UTV_MAX_CHANNEL_SCAN      0
#else
#define UTV_MAX_CHANNEL_SCAN      (100 * 1000)
#endif
#endif /* __PLATFORM_CONFIG_H__ */
