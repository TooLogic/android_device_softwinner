/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 - 2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * utv-core.h: UpdateTV Common Core -  Main Common Core header file
 * 
 * In order to ensure UpdateTV network compatibility, customers must not
 * change UpdateTV Common Core code.
 *
 * These defines and typdefs are shared with the UpdateTV tools.
 *
 * @file      utv-core.h
 *
 * @author    Bob Taylor
 *
 * @version   $Revision: 5806 $
 *            $Author: btaylor $
 *            $Date: 2011-11-21 13:19:49 -0500 (Mon, 21 Nov 2011) $
 *
 **********************************************************************************************
 */

/* 
 Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      11/03/2011  Moved UTV_MAX_UQR_SIZE back to core-internet-msg-protocol.c.
  Jim Muchow      11/02/2011  Download Resume Support (update DownloadTracker structs)
  Bob Taylor      07/07/2011  TKTest support.
  Bob Taylor      06/15/2011  UtvPlatformOnProvisioned() operation types added.
                              Reverted UTV_NO_DOWNLOAD to old message.  Supervisor taking
                              care of pretty happy messages.
  Bob Taylor      06/04/2011  Changed UTV_INTERNET_PERS_FAILURE to "internet connection failure"
                              Added UTV_MANIFEST_OPEN_FAILS.
                              Changed UTV_NO_DOWNLOAD to "Your system is currently up to date."
  Bob Taylor      04/28/2011  Added UtvManifestOptional() proto.
  Bob Taylor      12/16/2010  Added UtvPlatformProvisionerGetObjectStatus() proto.  Removed provisioning-related structures.
  Jim Muchow      10/04/2010  Add a struct prototype used to track when socket selects (or other timed-out blocking calls) are started and for how long
  Bob Taylor      10/01/2010  Added UQR caching struct for Query Pacing.
  Jim Muchow      09/15/2010  Add in prototypes for Query Pacing
  Jim Muchow      08/20/2010  Add in prototypes for Utv[Activate|Deactivate]SelectMonitor() and UtvInternetReset_UpdateQueryDisabled()
  Nathan Ford     07/01/2010  Added prototype for UtvCommonDecryptCommonFile(), removed UtvCommonDecryptExtFile()
  Bob Taylor      06/22/2010  Loop test support.
  Bob Taylor      06/09/2010  Removed UtvInternetProvisionerPutEntry().
  Bob Taylor      05/05/2010  Added UTV_INTERNET_INCOMPLETE_DOWNLOAD and UTV_RANGE_REQUEST_FAILURE.
  Bob Taylor      04/29/2010  Added UTV_NETWORK_NOT_UP.
  Bob Taylor      04/23/2010  Added UTV_PLATFORM_PART_READ.
  Bob Taylor      04/09/2010  Added UtvImageProcessUSBCommands() proto.  New USB command target failure error.
  Jim Muchow      04/07/2010  Add declarations to [Clear|SSL]SessionCreate() functions
  Jim Muchow      03/19/2010  Modified UTV_ERRORLIST to use explicit enumerations;
                              Added three new TIMEOUT error types for encrypted channels
  Bob Taylor      03/11/2010  Security model two support added.
  Bob Taylor      11/05/2009  Added UTV_UQS_DISABLED.
  Chris Nigbur    02/02/2010  UtvImageInit() proto added.
  Bob Taylor      01/12/2010  Added new error states and download end tracking.
  Bob Taylor      11/05/2009  Added UtvBroadcastDownloadGenerateSchedule() proto.
                              Renamed UtvImageValidatePartitionHashes() to
                              UtvImageValidateHashes().
                              Added UtvInternetRegistered().
                              Added UtvImageCloseAll() and UtvManifestClose() protos.
  Bob Taylor      06/30/2009  New download tracker struct and provisioner encrypt types.
  Bob Taylor      03/05/2009  All delivery modes defined all the time.
                              Many changes related to Publisher.
                              Additional error codes.
  Bob Taylor      02/23/2009  Moved image-related structures to utvimage.h
  Bob Taylor      01/26/2009  Created from version 1.9.21
*/

#ifndef __UTVCORE_H__
#define __UTVCORE_H__

/* Global UpdateTV status values
    Add new values to the bottom of the list
    Each value is added by macro definition
    This allows presdebug to optionally translate error numbers to strings
*/
#define UTV_ERRORLIST \
    UTV_ERR( UTV_OK,                0, "normal (success) return" )  \
    UTV_ERR( UTV_UNKNOWN,           1, "generic unknown error code" )  \
    UTV_ERR( UTV_TIMEOUT,           2, "operation timed out" )      \
    UTV_ERR( UTV_VCT_TIMEOUT,       3, "time out scanning for VCT" )  \
    UTV_ERR( UTV_PAT_TIMEOUT,       4, "time out scanning for PAT" )  \
    UTV_ERR( UTV_PMT_TIMEOUT,       5, "time out scanning for PMT" )  \
    UTV_ERR( UTV_DSI_TIMEOUT,       6, "time out scanning for DSI" )  \
    UTV_ERR( UTV_DII_TIMEOUT,       7, "time out scanning for DII" )  \
    UTV_ERR( UTV_DDB_TIMEOUT,       8, "time out scanning for DDBs" )  \
    UTV_ERR( UTV_ABORT,             9, "operation was aborted" )    \
    UTV_ERR( UTV_FUTURE,            10, "operation is too far in the future" )  \
    UTV_ERR( UTV_PAST,              11, "operation is too far in the past" )  \
    UTV_ERR( UTV_NO_DOWNLOAD,       12, "no download matched the selection criteria" )  \
    UTV_ERR( UTV_NO_DATA,           13, "expected data was not found" )  \
    UTV_ERR( UTV_NO_HARDWARE,       14, "some hardware was not ready" )  \
    UTV_ERR( UTV_NO_TUNER,          15, "tuner hardware not available" )  \
    UTV_ERR( UTV_NO_FREQUENCY_LIST, 16, "frequency list is empty" )  \
    UTV_ERR( UTV_NO_SECTION_FILTER, 17, "section filter hardware not ready" )  \
    UTV_ERR( UTV_NO_MEMORY,         18, "not enough memory was available" )  \
    UTV_ERR( UTV_NO_THREAD,         19, "thread was not created" )  \
    UTV_ERR( UTV_NO_MUTEX,          20, "mutex was not available" )  \
    UTV_ERR( UTV_NO_LOCK,           21, "mutex did not create or did not lock" )  \
    UTV_ERR( UTV_NO_WAIT,           22, "condition did not create or did not wait" )  \
    UTV_ERR( UTV_NO_STATE,          23, "persisted state not set" )  \
    UTV_ERR( UTV_BAD_SIZE,          24, "attempt to set or use bad size" )  \
    UTV_ERR( UTV_BAD_HANDLE,        25, "attempt to set or use bad handle" )  \
    UTV_ERR( UTV_BAD_RETURN,        26, "attempt to set or use bad return value" )  \
    UTV_ERR( UTV_BAD_LOCK,          27, "serious error with getting signal lock" )  \
    UTV_ERR( UTV_BAD_FREQUENCY,     28, "serious error with a frequency" )  \
    UTV_ERR( UTV_BAD_STATE,         29, "attempt to set or use bad UpdateTV state" )  \
    UTV_ERR( UTV_BAD_THREAD,        30, "serious error accessing or shutting down a thread" )  \
    UTV_ERR( UTV_BAD_CHECKSUM,      31, "bad CRC32 checksum" )      \
    UTV_ERR( UTV_BAD_WAIT_HANDLE,   32, "attempt to use invalid wait handle" )  \
    UTV_ERR( UTV_BAD_MUTEX_HANDLE,  33, "attempt to use invalid mutex handle" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_NULL_DATA,      34, "section filter can't deliver null section pointer" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_ZERO_DATA,      35, "section filter can't deliver zero length section" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_ZERO_RETURN,    36, "section filter thought it had data but had null pointer" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_NOT_RUNNING,    37, "section filter not in running state" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_NOT_STOPPED,    38, "section filter not in stopped state" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_LOCK_RUN,       39, "section can't be set in RUNNING state" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_LOCK_OPEN,      40, "section filter can't lock for opening" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_LOCK_DELIVER,   41, "section filter can't lock for delivery of data" )  \
    UTV_ERR( UTV_BAD_SECTION_FILTER_DELIVER_TID_MATCH, 42, "section filter data didn't match table id" )  \
    UTV_ERR( UTV_BAD_PID,           43, "attempt to set or use bad pid" )  \
    UTV_ERR( UTV_BAD_VCT_TID,       44, "VCT was on wrong table id" )  \
    UTV_ERR( UTV_BAD_VCT_PRV,       45, "VCT had incorrect protocol version" )  \
    UTV_ERR( UTV_BAD_VCT_CIS,       46, "VCT had zero channels listed" )  \
    UTV_ERR( UTV_BAD_PAT_TID,       47, "PAT was on wrong table id" )  \
    UTV_ERR( UTV_BAD_PAT_CCT,       48, "PAT has zero channel count" )  \
    UTV_ERR( UTV_BAD_PAT_NCF,       49, "PAT has no channels parsed" )  \
    UTV_ERR( UTV_BAD_PMT_TID,       50, "PMT on wrong table id" )   \
    UTV_ERR( UTV_BAD_DSI_TID,       51, "DSI has wrong section table id" )  \
    UTV_ERR( UTV_BAD_DSI_DPD,       52, "DSI did not have DSM-CC protocol discriminator" )  \
    UTV_ERR( UTV_BAD_DSI_UNM,       53, "DSI not DSM-CC User-Network Message" )  \
    UTV_ERR( UTV_BAD_DSI_MID,       54, "DSI has incorrect DSM-CC Message Id" )  \
    UTV_ERR( UTV_BAD_DSI_ADP,       55, "DSI has nonzero adaptation length" )  \
    UTV_ERR( UTV_BAD_DSI_DLN,       56, "DSI has bad DSM-CC message length" )  \
    UTV_ERR( UTV_BAD_DSI_CDL,       57, "DSI has nonzero compatibility descriptor length" )  \
    UTV_ERR( UTV_BAD_DSI_PDL,       58, "DSI has zero private data length" )  \
    UTV_ERR( UTV_BAD_DSI_BDC,       59, "DSI is missing BDC network id" )  \
    UTV_ERR( UTV_BAD_DSI_VER,       60, "DSI has incorrect network version" )  \
    UTV_ERR( UTV_BAD_DSI_NOG,       61, "DSI has zero number of groups" )  \
    UTV_ERR( UTV_BAD_DSI_STT,       62, "DSI has missing system time" )  \
    UTV_ERR( UTV_BAD_DSI_STV,       63, "DSI has invalid time protocol version" )  \
    UTV_ERR( UTV_BAD_DII_TID,       64, "DII has wrong section table id" )  \
    UTV_ERR( UTV_BAD_DII_DPD,       65, "DII did not have DSM-CC protocol discriminator" )  \
    UTV_ERR( UTV_BAD_DII_UNM,       66, "DII not DSM-CC User-Network Message" )  \
    UTV_ERR( UTV_BAD_DII_MID,       67, "DII has incorrect DSM-CC Message Id" )  \
    UTV_ERR( UTV_BAD_DII_RSV,       68, "DII reserved byte has wrong value" )  \
    UTV_ERR( UTV_BAD_DII_ADP,       69, "DII has nonzero adaptation length" )  \
    UTV_ERR( UTV_BAD_DII_DLN,       70, "DII has bad DSM-CC message length" )  \
    UTV_ERR( UTV_BAD_DII_CDL,       71, "DII has zero compatibility descriptor length" )  \
    UTV_ERR( UTV_BAD_DDB_TID,       72, "DDB has wrong section table id" )  \
    UTV_ERR( UTV_BAD_DDB_DPD,       73, "DDB did not have DSM-CC protocol discriminator" )  \
    UTV_ERR( UTV_BAD_DDB_UNM,       74, "DDB not DSM-CC User-Network Message" )  \
    UTV_ERR( UTV_BAD_DDB_MID,       75, "DDB has incorrect DSM-CC Message Id" )  \
    UTV_ERR( UTV_BAD_DDB_ADP,       76, "DDB has nonzero adaptation length" )  \
    UTV_ERR( UTV_BAD_DDB_DLN,       77, "DDB has bad DSM-CC message length" )  \
    UTV_ERR( UTV_BAD_DDB_BTL,       78, "DDB has block number too large" )  \
    UTV_ERR( UTV_NO_ENCRYPTION,         79, "module is not encrypted" )  \
    UTV_ERR( UTV_NO_CRYPT_HEADER,       80, "module is missing encryption signature" )  \
    UTV_ERR( UTV_BAD_CLEAR_VERSION,     81, "module clear header is wrong version" )  \
    UTV_ERR( UTV_BAD_KEY_VERSION,       82, "module key header is wrong version" )  \
    UTV_ERR( UTV_BAD_KEY_TYPE,          83, "module key header is wrong type" )  \
    UTV_ERR( UTV_NO_CRYPT_KEY,          84, "no decryption key found" )  \
    UTV_ERR( UTV_BAD_CRYPT_HANDLE,      85, "attempt to set or use bad decryption handle" )  \
    UTV_ERR( UTV_BAD_CRYPT_VERSION,     86, "wrong version of decryption header" )  \
    UTV_ERR( UTV_BAD_CRYPT_OUI,         87, "incorrect OUI in decrypted image" )  \
    UTV_ERR( UTV_BAD_CRYPT_MODEL,       88, "incorrect model group in decrypted image" )  \
    UTV_ERR( UTV_BAD_CRYPT_HEADSIZE,    89, "module is too small" )  \
    UTV_ERR( UTV_BAD_CRYPT_TRAILSIZE,   90, "decryption trailer bytes are missing or too small" )  \
    UTV_ERR( UTV_BAD_CRYPT_HEADER,      91, "decrypted header doesn't match header signature" )  \
    UTV_ERR( UTV_BAD_DECRYPTION,        92, "failure to validate decrypted data" )  \
    UTV_ERR( UTV_BAD_CRYPT_DATA,        93, "decryption data value invalid" )  \
    UTV_ERR( UTV_NO_DOWLOAD_SERVICE,    94, "no download service present in the VCT" )  \
    UTV_ERR( UTV_INCOMPATIBLE_OUI,      95, "OUI not compatible with this platform" )  \
    UTV_ERR( UTV_INCOMPATIBLE_MGROUP,   96, "model group not compatible with this platform" )  \
    UTV_ERR( UTV_INCOMPATIBLE_OUIMGROUP,97, "OUI/model group pair not compatible with this platform" ) \
    UTV_ERR( UTV_INCOMPATIBLE_SWVER,    98, "software version not compatible with this platform" ) \
    UTV_ERR( UTV_INCOMPATIBLE_HWMODEL,  99, "hardware model not compatible with this platform" )  \
    UTV_ERR( UTV_INCOMPATIBLE_FTEST,    100, "field test attribute doesn't match platform configuration" )  \
    UTV_ERR( UTV_INCOMPATIBLE_FACT_TEST, 101, "factory test attribute doesn't match platform configuration" ) \
    UTV_ERR( UTV_INCOMPATIBLE_DEPEND,   102, "dependency is incompatible" ) \
    UTV_ERR( UTV_DEPENDENCY_MISSING,    103, "dependency missing" )  \
    UTV_ERR( UTV_NEED_COMPDIR,          104, "need component directory module before other modules" )  \
    UTV_ERR( UTV_ALREADY_HAVE_COMPDIR,  105, "already have component directory" )  \
    UTV_ERR( UTV_UNSUPPORTED_MODULE,    106, "module name does not exist in any supported download group" )  \
    UTV_ERR( UTV_BAD_DOWNLOAD_STATUS,   107, "download status memory block invalid" )  \
    UTV_ERR( UTV_BAD_MODULE_SIZE,       108, "module size not as expected" )  \
    UTV_ERR( UTV_BAD_MODVER_DOWNLOADED, 109, "downloaded module version is same or more recent - don't update" )  \
    UTV_ERR( UTV_BAD_MODVER_CURRENT,    110, "current module version is same or more recent - ALREADY HAVE, DON'T UPDATE" )  \
    UTV_ERR( UTV_BAD_TUNER_MODE,        111, "attempt to set or use an unsupported tuner mode" )  \
    UTV_ERR( UTV_BAD_CRYPT_MODULE_SIZE, 112, "module size invalid - encrypt size unexpected" )  \
    UTV_ERR( UTV_BAD_HASH_VALUE,        113, "bad SHA message digest" )  \
    UTV_ERR( UTV_DOWNLOAD_COMPLETE,     114, "download complete" )  \
    UTV_ERR( UTV_NO_INSTALL,            115, "no components were installed" )  \
    UTV_ERR( UTV_COMP_INSTALL_COMPLETE, 116, "component install complete" )  \
    UTV_ERR( UTV_UPD_INSTALL_COMPLETE,  117, "update install complete" )  \
    UTV_ERR( UTV_SECTION_QUEUE_OVERFLOW, 118, "section queue overflow" ) \
    UTV_ERR( UTV_ILLEGAL_DDM,           119, "specified DDM is invalid" ) \
    UTV_ERR( UTV_ALREADY_OPEN,          120, "sub-system already opened" )  \
    UTV_ERR( UTV_MODULE_STORE_FAIL,     121, "module store fails" )  \
    UTV_ERR( UTV_PERSIST_FAILS,         122, "peristent memory unavailable or failed - reinit" )  \
    UTV_ERR( UTV_BAD_DII_TTE,           123, "slot time too early" )  \
    UTV_ERR( UTV_BAD_DII_TTL,           124, "slot time too late" )  \
    UTV_ERR( UTV_BAD_DII_LAT,           125, "slot time later than an accepted slot" )  \
    UTV_ERR( UTV_BAD_MODVER_SIGNALED,   126, "signaled module version is more recent" )  \
    UTV_ERR( UTV_BAD_DII_NCD,           127, "DII has no compatibility descriptors" )  \
    UTV_ERR( UTV_BAD_DII_NSD,           128, "DII has no schedule descriptors" )  \
    UTV_ERR( UTV_EXECUTE_FAILED,        129, "new image execute failed" )  \
    UTV_ERR( UTV_BAD_MAGIC,             130, "bad magic code in header" )  \
    UTV_ERR( UTV_BAD_VERSION,           131, "bad version in header" )  \
    UTV_ERR( UTV_STRUCT_SIZE,           132, "bad struct size" )    \
    UTV_ERR( UTV_NOT_INITIALIZED,       133, "sub-system has not been initialized" )  \
    UTV_ERR( UTV_COUNTS_DISAGREE,       134, "internal and actual counts disagree" )  \
    UTV_ERR( UTV_BAD_DSI_MRD,           135, "DSI contains unknown MRD" )  \
    UTV_ERR( UTV_BAD_DDB_MSZ,           136, "DDB has bad module size" )  \
    UTV_ERR( UTV_NO_ULI_DATA,           137, "channel does not contain ULI data" )  \
    UTV_ERR( UTV_OPEN_FAILS,            138, "file open fails" )    \
    UTV_ERR( UTV_WRITE_FAILS,           139, "file write fails, disk full?" )  \
    UTV_ERR( UTV_READ_FAILS,            140, "file read fails" )    \
    UTV_ERR( UTV_SEEK_FAILS,            141, "file seek fails" )    \
    UTV_ERR( UTV_CLOSE_FAILS,           142, "file close fails" )   \
    UTV_ERR( UTV_DELETE_FAILS,          143, "file delete fails" )  \
    UTV_ERR( UTV_CREATE_DIR_FAILS,      144, "create directory fails" )  \
    UTV_ERR( UTV_REG_OPEN_FAILS,        145, "registry key open fails" )  \
    UTV_ERR( UTV_BAD_CRYPT_PAYLOAD_TYPE, 146, "decryption payload type is unknown" ) \
    UTV_ERR( UTV_BAD_CRYPT_SMALL,       147, "input data is too small" ) \
    UTV_ERR( UTV_BAD_CRYPT_SIZE,        148, "input data is wrong size" )  \
    UTV_ERR( UTV_BAD_CRYPT_TRAILER,     149, "decrypted module doesn't match trailer signature" )  \
    UTV_ERR( UTV_NULL_PTR,              150, "null ptr encountered" )  \
    UTV_ERR( UTV_BAD_DELIVERY_MODE,     151, "bad delivery mode" )  \
    UTV_ERR( UTV_BAD_DELIVERY_COUNT,    152, "bad delivery count" )  \
    UTV_ERR( UTV_DELIVERY_DUPLICATE,    153, "delivery mode duplicate" )  \
    UTV_ERR( UTV_NO_DELIVERY_MODES,     154, "no delivery modes found" )  \
    UTV_ERR( UTV_INTERNET_PERS_FAILURE, 155, "internet connection failure" )  \
    UTV_ERR( UTV_INTERNET_SRVR_FAILURE, 156, "internet server failure" )  \
    UTV_ERR( UTV_INTERNET_DLOAD_DONE,   157, "internet download complete" )  \
    UTV_ERR( UTV_INTERNET_DLOAD_CONT,   158, "internet download continues" )  \
    UTV_ERR( UTV_INTERNET_ULID_ACQUIRED, 159, "ULID has already been acquired" ) \
    UTV_ERR( UTV_INTERNET_SNUM_SIZE,    160, "serial number is too long" ) \
    UTV_ERR( UTV_INTERNET_SNUM_RANGE,   161, "serial number out of range" )  \
    UTV_ERR( UTV_INTERNET_SNUM_ALREADY, 162, "serial number already registered" )  \
    UTV_ERR( UTV_INTERNET_UNKNOWN,      163, "unknown internet server error" )  \
    UTV_ERR( UTV_INTERNET_BAD_ULID,     164, "bad ULID received from server" )  \
    UTV_ERR( UTV_INTERNET_NO_COMPAT,    165, "no Compatibility info in UQR" )  \
    UTV_ERR( UTV_INTERNET_PARSE,        166, "server response parse error" )  \
    UTV_ERR( UTV_INTERNET_DNS_FAILS,    167, "name resolution fails" )  \
    UTV_ERR( UTV_INTERNET_CNCT_TIMEOUT, 168, "internet connect timeout" )  \
    UTV_ERR( UTV_INTERNET_READ_TIMEOUT, 169, "internet read timeout" )  \
    UTV_ERR( UTV_INTERNET_WRITE_TIMEOUT, 170, "internet write timeout" ) \
    UTV_ERR( UTV_INTERNET_CONN_CLOSED,  171, "socket closed" ) \
    UTV_ERR( UTV_FILE_OPEN_FAILS,       172, "file open fails" )    \
    UTV_ERR( UTV_FILE_UNKNOWN_LOCATION, 173, "unknown file location" )  \
    UTV_ERR( UTV_FILE_BAD_HANDLE,       174, "bad file handle" )    \
    UTV_ERR( UTV_FILE_EOF,              175, "end of file encountered" )  \
    UTV_ERR( UTV_FILE_DELETE_FAILS,     176, "file delete fails" )  \
    UTV_ERR( UTV_FILE_CLOSE_FAILS,      177, "file close fails" )   \
    UTV_ERR( UTV_BAD_DIR_SIG,           178, "bad component directory signature" )  \
    UTV_ERR( UTV_BAD_DIR_VER,           179, "bad component directory version" )  \
    UTV_ERR( UTV_BAD_CLR_HDR,           180, "bad clear header" )   \
    UTV_ERR( UTV_BAD_MOD_SIG,           181, "bad module header signature" )  \
    UTV_ERR( UTV_BAD_MOD_VER,           182, "bad module header version" )  \
    UTV_ERR( UTV_MOD_SIZES_DISAGREE,    183, "module header size disagrees with decrypted size" )  \
    UTV_ERR( UTV_BAD_INDEX,             184, "bad index" )          \
    UTV_ERR( UTV_MOD_ID_OVERFLOW,       185, "module ID tracking overflow" )  \
    UTV_ERR( UTV_NOT_OPEN,              186, "not open" )           \
    UTV_ERR( UTV_NO_OVERWRITE,          187, "no overwrite permission" )  \
    UTV_ERR( UTV_TOO_BIG,               188, "request to store something that is too big" )  \
    UTV_ERR( UTV_BAD_ACCESS,            189, "no or illegal access requested" )  \
    UTV_ERR( UTV_UNKNOWN_COMPONENT,     190, "unknown component" )  \
    UTV_ERR( UTV_MISSING_OBJECTS,       191, "missing objects in set" )  \
    UTV_ERR( UTV_UNKNOWN_HW_MODEL,      192, "unknown hardware model" )  \
    UTV_ERR( UTV_DOWNLOAD_DEFERRED,     193, "download deferred to later time" )  \
    UTV_ERR( UTV_BAD_OFFSET,            194, "bad offset" )         \
    UTV_ERR( UTV_NO_IMAGES_ENTRIES,     195, "out of image storage array entries" )  \
    UTV_ERR( UTV_BAD_COMPONENT_INDEX,   196, "bad component index" )  \
    UTV_ERR( UTV_SIGNALING_DISAGREES,   197, "signaling disagrees with component directroy" )  \
    UTV_ERR( UTV_TEXT_DEF_NOT_FOUND,    198, "text definition not found" )  \
    UTV_ERR( UTV_BAD_TEXT_DEF_INDEX,    199, "bad definition text index" )  \
    UTV_ERR( UTV_NO_OPEN_STORES,        200, "out of open update storage" )  \
    UTV_ERR( UTV_STORE_EMPTY,           201, "store is empty" )     \
    UTV_ERR( UTV_DATA_VERIFY_FAILS,     202, "data verify fails" )  \
    UTV_ERR( UTV_DOWNLOAD_NOT_SUPPORTED, 203, "download not supported for file delivery mode" ) \
    UTV_ERR( UTV_SSL_INIT_FAILS,        204, "ssl init failed" ) \
    UTV_ERR( UTV_SSL_CERT_LOAD_FAILS,   205, "ssl verification cert load failed" )  \
    UTV_ERR( UTV_SSL_CERT_NOT_FOUND,    206, "ssl verification cert not found" )  \
    UTV_ERR( UTV_SSL_CONNECT_FAILS,     207, "ssl connect fails" )  \
    UTV_ERR( UTV_SSL_HOST_CERT_FAILS,   208, "ssl host certificate verify failed" )  \
    UTV_ERR( UTV_SSL_NO_HOST_CERT,      209, "ssl host did not provide a certificate" )  \
    UTV_ERR( UTV_SSL_NO_ALT_NAMES,      210, "ssl alt names field not found" )  \
    UTV_ERR( UTV_SSL_NO_CN,             211, "ssl common name lookup failed" )  \
    UTV_ERR( UTV_NO_OBJECT,             212, "no object in provision payload" )  \
    UTV_ERR( UTV_OBJECT_NOT_FOUND,      213, "object not found" )   \
    UTV_ERR( UTV_BUFFER_TOO_SMALL,      214, "buffer too small" )   \
    UTV_ERR( UTV_PLATFORM_INSTALL_FAILS, 215, "platform-specific installation fails" ) \
    UTV_ERR( UTV_TIME_ROLLBACK,         216, "time has rolled back" ) \
    UTV_ERR( UTV_NO_NETWORK_TIME,       217, "network time not set" )  \
    UTV_ERR( UTV_MODVER_TRANSPLANT,     218, "db mod ver greater than system mod ver" )  \
    UTV_ERR( UTV_GUID_NOT_FOUND,        219, "object guid not found" )  \
    UTV_ERR( UTV_OWNER_NOT_FOUND,       220, "object owner not found" )  \
    UTV_ERR( UTV_NAME_NOT_FOUND,        221, "object name not found" )  \
    UTV_ERR( UTV_MAX_CMD_RESPONSE_TIME, 222, "max cmd response time" )  \
    UTV_ERR( UTV_MIN_CMD_RESPONSE_TIME, 223, "min cmd response time" )  \
    UTV_ERR( UTV_MAX_DLD_RESPONSE_TIME, 224, "max download response time" )  \
    UTV_ERR( UTV_MIN_DLD_RESPONSE_TIME, 225, "min download response time" )  \
    UTV_ERR( UTV_INIT_PRO_TIME,         226, "initial provisioning duration" )  \
    UTV_ERR( UTV_PERSIST_WRITE,         227, "persistent memory write count" )  \
    UTV_ERR( UTV_PLATFORM_COMP_COMP,    228, "platform component complete error" )  \
    UTV_ERR( UTV_PLATFORM_UPDATE_COMP,  229, "platform update complete error" )  \
    UTV_ERR( UTV_PLATFORM_PART_OPEN,    230, "platform partition open error" )  \
    UTV_ERR( UTV_PLATFORM_PART_WRITE,   231, "platform partition write error" )  \
    UTV_ERR( UTV_PLATFORM_PART_CLOSE,   232, "platform partition close error" )  \
    UTV_ERR( UTV_ETHER_MAC_ADDRESS,     233, "Ethernet MAC Address" )  \
    UTV_ERR( UTV_WIFI_MAC_ADDRESS,      234, "Wifi MAC Address" )   \
    UTV_ERR( UTV_TUNE_INFO,             235, "Tune Info" )          \
    UTV_ERR( UTV_COMMANDS_DETECTED,     236, "Embedded commands detected in update" )  \
    UTV_ERR( UTV_FILE_LOCK_FAILS,       237, "file lock fails" )    \
    UTV_ERR( UTV_PART_VALIDATE_FAILS,   238, "partition validate fails" )  \
    UTV_ERR( UTV_INTERNET_DENIED,       239, "internet request denied" )  \
    UTV_ERR( UTV_REGS_DISABLED,         240, "registrations disabled" )  \
    UTV_ERR( UTV_UQS_DISABLED,          241, "update queries disabled" )  \
    UTV_ERR( UTV_BAD_SECURITY_MODEL,    242, "bad security model" )  \
    UTV_ERR( UTV_INTERNET_CRYPT_CNCT_TIMEOUT, 243, "internet encrypted connect timeout" )  \
    UTV_ERR( UTV_INTERNET_CRYPT_READ_TIMEOUT, 244, "internet encrypted read timeout" )  \
    UTV_ERR( UTV_INTERNET_CRYPT_WRITE_TIMEOUT, 245, "internet encrypted write timeout" )  \
    UTV_ERR( UTV_SNUM_TARGETING_FAILS,  246, "serial number targeting fails" )  \
    UTV_ERR( UTV_PLATFORM_PART_READ,    247, "platform partition read error" )  \
    UTV_ERR( UTV_NETWORK_NOT_UP,        248, "network not up" )  \
    UTV_ERR( UTV_INTERNET_INCOMPLETE_DOWNLOAD, 249, "download completed before all data received" )  \
    UTV_ERR( UTV_RANGE_REQUEST_FAILURE, 250, "range request failure" )  \
    UTV_ERR( UTV_LOOP_TEST_MODE,        251, "device is in loop test mode" )  \
    UTV_ERR( UTV_BUSY,                  252, "presently busy or active" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_COMMAND_UNKNOWN, 253, "live support command unknown" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_COMMAND_NOT_PROCESSED, 254, "live support command not processed" )  \
    UTV_ERR( UTV_WEB_SERVICE_CONTROL_FAILURE, 255, "web service control failure" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_INIT_ALREADY_ACTIVE, 256, "unable to inititate live support session because session already active" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_INIT_FAILED, 257, "live support session initialization failed" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_TERMINATE_NOT_ACTIVE, 258, "unable to terminate live support session because session not active" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_TERMINATE_FAILED, 259, "live support session termination failed" )  \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_NOT_ACTIVE, 260, "live support session not active" )  \
    UTV_ERR( UTV_INVALID_PARAMETER, 261, "invalid parameter" )  \
    UTV_ERR( UTV_SOAP_CTX_CREATE_FAILED, 262, "failed to create soap context" ) \
    UTV_ERR( UTV_BAD_ULID_GUID, 263, "ULID GUID field invalid" ) \
    UTV_ERR( UTV_MANIFEST_OPEN_FAILS, 264, "manifest open fails" ) \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_ALREADY_ACTIVE, 265, "live support session already active" ) \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_RESUME_NOT_AVAILABLE, 266, "live support session not available to resume" ) \
    UTV_ERR( UTV_LIVE_SUPPORT_SESSION_RESUME_TOO_OLD, 267, "live support session saved too old to resume" ) \
    UTV_ERR( UTV_SCHEDULE_MISSING, 268, "The update schedule is missing" ) \

/* Define the global error codes
*/
#define UTV_ERR( code, value, text ) code=value,
typedef enum {
    UTV_ERRORLIST
    UTV_RETURN_COUNT
} UTV_RETURN;

#undef UTV_ERR

/* event text unavailable flag */
#define UTV_TEXT_OFFSET_INVALID 0xFFFFFFFF

/* delivery tracking structs */
#define UTV_DELIVERY_MAX_MODES        3

typedef struct {
    UTV_UINT32    uiType;
    UTV_BOOL    bAccess;
} UtvDeliveryMode;

typedef struct {
     UTV_UINT32            uiNumModes;
    UtvDeliveryMode        Mode[ UTV_DELIVERY_MAX_MODES ];
} UtvDeliveryModes;

#define UTV_CORE_IMAGE_INVALID    0xFFFFFFFF
/* Define internet protocol related structures here.
 */

/* the port we download from */
#define UTV_INTERNET_DOWNLOAD_PORT      80

/* types for UtvInternetDownloadEnded() */
#define UTV_IP_DOWNLOAD_TYPE_UNKNOWN    0
#define UTV_IP_DOWNLOAD_TYPE_BROADCAST  1
#define UTV_IP_DOWNLOAD_TYPE_INTERNET   2
#define UTV_IP_DOWNLOAD_TYPE_FILE       3

/* 16 plus zero */
#define UTV_IP_MAX_ULID_CHARS     17
#define UTV_IP_MAX_DEVID_CHARS    24

/* maximum numbeer of compatibility entries in a update query response */
#define UTV_IP_MAX_UQ_COMPAT    8

typedef struct {
    UTV_UINT32              uiOUI;
    UTV_UINT16              usModelGroup;
    UTV_UINT16              usModuleVersion;
    UTV_UINT32              uiAttributes;
    UTV_UINT32              uiNumCompatEntries;
    UtvRange                utvHWMRange[UTV_IP_MAX_UQ_COMPAT];
    UtvRange                utvSWVRange[UTV_IP_MAX_UQ_COMPAT];
} UtvInternetUpdateQueryEntry;

typedef struct {
    UTV_UINT32                     uiNextQuerySeconds;
    UTV_UINT32                     uiTime;
    UTV_UINT32                     uiNumEntries;
    UtvInternetUpdateQueryEntry    aUtvUQE[UTV_PLATFORM_MAX_UPDATES];
} UtvInternetUpdateQueryResponse;

/* provisioned object encryption types */
#define UTV_PROVISIONER_ENCRYPTION_NONE     0x0000
#define UTV_PROVISIONER_ENCRYPTION_3DES     0x0001
#define UTV_PROVISIONER_ENCRYPTION_AES      0x0002
#define UTV_PROVISIONER_ENCRYPTION_RSA      0x0004
#define UTV_PROVISIONER_ENCRYPTION_CUSTOM   0x0100
#define UTV_PROVISIONER_ENCRYPTION_3DES_UR  0x8001
#define UTV_PROVISIONER_ENCRYPTION_AES_UR   0x8002
#define UTV_PROVISIONER_ENCRYPTION_RSA_UR   0x8004
#define UTV_PROVISIONER_ENCRYPTION_CUST_UR  0x8100

/* provisioned object status mask defines */
#define UTV_PROVISION_STATUS_MASK_CHANGED   0x00000001

/* UtvPlatformOnProvisioned() operation types */
#define UTV_PROVISION_OP_INSTALL    0
#define UTV_PROVISION_OP_DELETE     1

#define UTV_MAX_COMPONENT_NAME_CHARS    64

typedef struct {
    UTV_UINT32        uiDownloadInSeconds;
    UTV_UINT32        uiOUI;
    UTV_UINT16        usModelGroup;
    UTV_UINT16        usModuleVersion;
    UTV_UINT32        uiAttributes;
    UTV_INT8          szHostName[UTV_PUBLIC_IP_MAX_HOST_NAME_CHARS];
    UTV_INT8          szFilePath[UTV_PUBLIC_IP_MAX_FILE_PATH_CHARS];
    UTV_UINT32        uiFileSize;
} UtvInternetScheduleDownloadResponse;

/* Device hint bit mask */
#define UTV_DEVICE_HINT_REVOKE      0x00000001
#define UTV_DEVICE_HINT_PROVISION   0x00000002
#define UTV_DEVICE_HINT_CEM_STATUS  0x00000004
#define UTV_DEVICE_HINT_OAD         0x00000008
#define UTV_DEVICE_HINT_CMD_MASK    0xFFFF0000
#define UTV_DEVICE_HINT_REREG_CMD   0x00010000
#define UTV_DEVICE_HINT_NETDIAG_CMD 0x00020000
extern  UTV_UINT32 g_uiDevHints;

#define UTV_DEVICE_HINT_DEFAULT     (UTV_DEVICE_HINT_CEM_STATUS | UTV_DEVICE_HINT_OAD)

extern  UTV_UINT32 g_uiStatusMask;
/* by default we don't send anything to the NOC */
#define UTV_STATUS_MASK_DEFAULT     0

extern  UTV_UINT32 g_uiCmdMask;
/* by default we don't respond to netDiag cmds */
#define UTV_CMD_MASK_DEFAULT        0

#define UTV_DEVICE_HINT_CEM_STATUS_ENABLED ((g_uiDevHints & UTV_DEVICE_HINT_CEM_STATUS) != 0)
#define UTV_DEVICE_HINT_OAD_ENABLED        ((g_uiDevHints & UTV_DEVICE_HINT_OAD) != 0)
#define UTV_DEVICE_HINT_PROVISIONING       ((g_uiDevHints & UTV_DEVICE_HINT_PROVISION)||(g_uiDevHints & UTV_DEVICE_HINT_REVOKE))
#define UTV_DEVICE_HINT_CMD_PRESENT        ((g_uiDevHints & UTV_DEVICE_HINT_CMD_MASK) != 0)
#define UTV_DEVICE_HINT_CMD                 (g_uiDevHints & UTV_DEVICE_HINT_CMD_MASK)

/* Section Filter States
*/
typedef enum {
    UTV_SECTION_FILTER_UNKNOWN,     /* new or uninitialized filter */
    UTV_SECTION_FILTER_STOPPED,     /* not gathering data */
    UTV_SECTION_FILTER_RUNNING,     /* gathering data and sending it to the core callback */
    UTV_SECTION_FILTER_COUNT
} UTV_SECTION_FILTER_STATUS;

/* Define wildcard OUI, model group, hw/sw versions
*/
#define UTV_CORE_OUI_ALL 0xffffff
#define UTV_CORE_MODELGROUP_ALL 0xffff

/* Define MPEG-2 Transport Stream packet size in bytes (do not change) */
#define UTV_MPEG_PACKET_SIZE 188

/* Define MPEG-2 Transport Stream max section size (do not change) */
#define UTV_MPEG_MAX_SECTION_SIZE 4096

/* UpdateTV Network Timeouts (in milliseconds) */
/* [CAUTION: THESE ARE UPDATE TV NETWORK PARAMETERS DO NOT CHANGE] */
#define UTV_WAIT_FOR_VCT                     (1000)
#define UTV_WAIT_FOR_PAT                     (1000)
#define UTV_WAIT_FOR_STT                (3 * 1000)
#define UTV_WAIT_FOR_PMT                (1 * 1000)
#define UTV_WAIT_FOR_DSI               (40 * 1000)
#define UTV_WAIT_FOR_DII           (4 * 60 * 1000)

/* Define the hardwired PIDs
*/
enum UTV_PID {
    UTV_PID_PSIP = 0x1FFB,      /* PSIP PID */
    UTV_PID_PAT = 0x0           /* PAT PID */
};

/* Define the hardwired TIDs
*/
#define UTV_TID_CVCT 0xC9       /* Cable Virtual Channel Table on PSIP PID */
#define UTV_TID_TVCT 0xC8       /* Terrestrial Virtual Channel Table on PSIP PID */
#define UTV_TID_STT  0xCD       /* STT table id on PSIP PID */
#define UTV_TID_PMT  0x02       /* PMT table id on PMT PID */
#define UTV_TID_PAT  0x00       /* PAT table id on PID 0 */
#define UTV_TID_UNM  0x3B       /* DSM-CC userNetworkMessage on Carousel PID */
#define UTV_TID_DDM  0x3C       /* DSM-CC downloadDataMessage on Carousel PID */
#define UTV_TID_ANY     0xFFFFFFFF /* Return all tables on this PID. */
#define UTV_TID UTV_BYTE

/* Service type (found in VCT)
*/
#define UTV_SERVICE_TYPE_DATA 0x04  /*  ATSC A/90 Data */
#define UTV_SERVICE_TYPE_DOWNLOAD 0x05  /*  ATSC A/97 Softwre Download Data Service */

/* Stream type (found in PMT)  DSM-CC type B, ATSC A/90 Download Protocol
*/
#define UTV_STREAM_TYPE_DOWNLOAD 0x0B
#define UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_TAG 0x05 /* PMT's MPEG-2 Registration Descriptor */
#define UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_LENGTH 10 /* Max length of registration descriptor */

/* DSM-CC message types
*/
#define UTV_DSMCC_PROTOCOL_DISCRIMINATOR 0x11
#define UTV_DSMCC_UNM 0x03
#define UTV_DSMCC_DSI 0x1006
#define UTV_DSMCC_DII 0x1002
#define UTV_DSMCC_DDB 0x1003

/* Descriptors found in the DII
*/
#define UTV_DII_SCHEDULE_DESCRIPTOR_TAG         0xBA
#define UTV_DII_MODULEINFO_DESCRIPTOR_TAG       0xB7
#define UTV_DII_COMPATABILITY_DESCRIPTOR_TAG    0x82

/* A/90 32-bit Transaction ID Definition
   10vvvvvvvvvvvvvvviiiiiiiiiiiiiiit
   |||-------------||-------------||
   ||       |              |       Control Message Update Bit - must toggle when control message is updated.
   ||       |              15 bit Control Message ID.
   ||       14 bit Control Message Version - modified everytime control message is updated.
    2 bit Control Message Originator - always 10.

   Define a mask that is sensitive to the Control Message ID only.  For Update TV networks this is the Group ID.
   A Group is a logical collection of modules belonging to a single OUI and Model Group pair.
*/
#define UTV_TRANSACTION_ID_MASK    0x0000FFFE

/* Subdescriptors found in the DSI
*/
#define UTV_DSI_NETWORK_DESCRIPTOR          0
#define UTV_DSI_TIME_DESCRIPTOR             3
#define UTV_DSI_SRVR_VERSION_DESCRIPTOR    33
#define UTV_DSI_SRVR_ID_DESCRIPTOR         34
#define UTV_DSI_FACTORY_MODE_DESCRIPTOR    35

/* Version check parameters
*/
#define UTV_NETWORK_ID                        0xBDC
#define UTV_NETWORK_VERSION                    3

/* Update Logic PMT Registration Descriptor
*/
#define UTV_ULI_PMT_REG_DESC_TOT_LEN     4
#define UTV_ULI_PMT_REG_DESC_BASE        "BDC"
#define UTV_ULI_PMT_REG_DESC_BASE_LEN    3
#define UTV_ULI_PMT_REG_DESC_ORG         '0'
#define UTV_ULI_PMT_REG_DESC_END         '1'

#define UTV_SERVER_VER_MAX_LEN           16
#define UTV_SERVER_ID_MAX_LEN            32

/* Module name total length with XXofYY
*/
#define UTV_MAX_NAME_BYTES 31

/* Module name base length only
*/
#define UTV_MOD_BASE_NAME_BYTES (UTV_MAX_NAME_BYTES - 5)

/* ANSI Time Conversion time buffer
*/
#define UTV_MAX_ASCII_TIME_BYTES    26

/* Signature of the peristent memory structure
*/
#define UTV_PERSISTENT_MEMORY_STRUCT_SIG        0xBDC1
#define UTV_PERSISTENT_MEMORY_TRACKER_SIG       0xFACEFEED
#define UTV_PERSISTENT_MEMORY_PROVISIONER_SIG   0xCAFEBABE

/* Max length of a query host
*/
#define UTV_MAX_QUERY_HOST_LEN 64

/* Depth of error history in diag stats structure
*/
#define UTV_ERROR_COUNT 5

/* UtvCarouselCandidate
    Each instance holds information gathered about a carousel.
*/
typedef struct
{
    UTV_UINT32 iFrequency;                  /* Tuner frequency */
    UTV_UINT32 iProgramNumber;              /* Program number from frequency's VCT with service_type 0x05 */
    UTV_UINT32 iPmtPid;                     /* PMT PID of this program number */
    UTV_UINT32 iCarouselPid;                /* Elementary PID of carousel data from the PMT */
    UTV_BOOL   bDiscoveredInVCT;            /* flag to indicate whether this candidate came from the VCT */

} UtvCarouselCandidate;

/* UtvGroupCandidate
    Each carousel may signal a number of groups we are interested in.
*/
typedef struct
{
    UTV_UINT32 iFrequency;                  /* Tuner frequency */
    UTV_UINT32 iCarouselPid;                /* Elementary PID of carousel data from the PMT */
    UTV_UINT32 iTransactionId;              /* Transaction ID for each compatible download */
    UTV_UINT32 iOui;                        /* OUI for this download */
    UTV_UINT32 iModelGroup;                 /* Model group for this download */
    UTV_UINT32 iSeenCount;                  /* Number of times group has been seen */
} UtvGroupCandidate;

/* UtvDownloadInfo
    A structure that contains information related to prospective download candidates, the best candidate so far, and
    module downloads that have been we are interested in.
*/
typedef struct
{
    UTV_PUBLIC_UPDATE_SUMMARY *pUpdate;     /* update struct associated with the download */
    UTV_UINT32 iFrequency;                  /* Tuner frequency */
    UTV_UINT32 iTransactionId;              /* Transaction ID for this compatible download from DSI */
    UTV_UINT32 iOui;                        /* OUI for this download from the DII (GII) */
    UTV_UINT32 iModelGroup;                 /* Model group from the DII (GII) */
    UTV_BYTE   szModuleName[ UTV_MAX_NAME_BYTES + 1 ]; /* Up to UTV_MAX_NAME_BYTES bytes of module name followed by 0 */
    UTV_UINT32 iCarouselPid;                /* Elementary PID of carousel data from the PMT */
    UTV_UINT32 iHardwareModelBegin;         /* Begin of hardware version range from the GII */
    UTV_UINT32 iHardwareModelEnd;           /* End of hardware version range from the GII */
    UTV_UINT32 iSoftwareVersionBegin;       /* Begin of software version range from the GII */
    UTV_UINT32 iSoftwareVersionEnd;         /* End of software version range from the GII */
    UTV_UINT32 iModuleId;                   /* Module id for this download */
    UTV_UINT32 iModulePriority;             /* Priority of this download */
    UTV_UINT32 iModuleVersion;              /* Version of module */
    UTV_UINT32 iModuleSize;                 /* Size of module in bytes */
    UTV_UINT32 iDecryptedModuleSize;        /* Size of module after decryption */
    UTV_UINT32 iModuleBlockSize;            /* size of each module's block in the DDB */
    UTV_UINT32 iModuleNameLength;           /* Length of module name (0 to UTV_MAX_NAME_BYTES) */
    UTV_UINT32 iNumberOfModules;            /* Number of modules in the module's home DII */
    UTV_UINT32 iBroadcastSeconds;           /* Number of seconds that the download will be broadcast */
    UTV_UINT32 iMillisecondsToStart;        /* Number of milliseconds until the download is to start */
    UTV_TIME   dtDownload;                  /* Start time for this download */
    UTV_BYTE   ubReserved[16];              /* for future expansion */
}  UtvDownloadInfo;

/* UtvDownloadTracker
    A structure that tracks downloaded modules downloads across pseudo-power cycles.
*/
typedef struct
{
    UTV_UINT16          usModId;            /* Module ID */
    UTV_UINT16          usCompIndex;        /* component descriptor index */
}  UtvModRecord_v1;

typedef struct
{
    UTV_UINT16          usModId;        /* Module ID */
    UTV_UINT16          usCompIndex;    /* owning component's index */
    UTV_UINT32          ulSize;         /* module's length - without header */
    UTV_UINT32          ulOffset;       /* module's offset - without header */
    UTV_UINT32          ulCRC;          /* module's CRC - without header */
}  UtvModRecord_v2;

typedef UtvModRecord_v2 UtvModRecord;

typedef struct
{
    UTV_UINT16          usModuleVersion;           /* module version of these modules */
    UTV_UINT16          usCurDownloadCount;        /* number of modules in the set that have been downloaded in the current update */
    UtvModRecord_v1     ausModIdArray[UTV_PLATFORM_MAX_MODULES]; /* record of modules downloaded. */
}  UtvDownloadTracker_v1;

typedef struct
{
    UTV_UINT16   usModuleVersion;           /* module version of these modules */
    UTV_UINT16   usCurDownloadCount;        /* number of modules in the set that have been downloaded in the current update */
    UtvModRecord        ausModIdArray[UTV_PLATFORM_MAX_MODULES];
}  UtvDownloadTracker_v2;

typedef UtvDownloadTracker_v2 UtvDownloadTracker;

/* The following structures are used to store data that must persist across
   power cycles.  See common-persist.c for more information.
*/

/* Persistent data header
*/
typedef struct
{
    UTV_UINT16  usSigBytes;                        /* Signature bytes that must 0xBDC1. */
    UTV_UINT16  usStructSize;                      /* Size of Global Storage structure including this header. */
} UtvAgentGlobalStorageHeader;

/* The state of the Agent is stored here
*/
typedef struct
{
    UTV_PUBLIC_STATE   utvState;                   /* Next (or current) state */
    UTV_PUBLIC_STATE   utvStateWhenAborted;        /* State system was in when it was aborted. */
    UTV_UINT32         iDelayMilliseconds;         /* Number of seconds until next event */
    UTV_BOOL           bReserved;                  /* reserved */
} UtvAgentState;

/* Presistent query pacing struct
*/
typedef struct
{
    UTV_BOOL b_ValidQueryResponseCached;
    time_t l_QueryPacingTimeToWait;
    time_t l_ValidQueryRspTimestamp;
    UtvInternetUpdateQueryResponse UtvUQR;
} UtvQueryCache;

/* Event argument types
*/
typedef enum
{
    UTV_EARG_LINE,
    UTV_EARG_DECNUM,
    UTV_EARG_HEXNUM,
    UTV_EARG_STRING_PTR,
    UTV_EARG_STRING_OFFSET,
    UTV_EARG_TIME,
    UTV_EARG_NOT_USED,
    UTV_EARG_MAX
} UTV_EARG_TYPE;

/* Event argument multi-type abstration
*/
typedef struct
{
    UTV_EARG_TYPE   eArgType;
    UTV_UINT32      uiArg;
} UTV_EARG;

/* Event storage struct
*/
typedef struct
{
    UTV_CATEGORY    rCat;                /* the event category */
    UTV_RESULT      rMsg;                /* the error/status code */
    UTV_UINT32      uiCount;             /* the number of them that has taken place */
    UTV_TIME        tTime;               /* the time of the last occurrence */
    UTV_EARG        eArg[UTV_PLATFORM_ARGS_PER_EVENT]; /* additional args */
} UTV_EVENT_ENTRY;

typedef struct
{
    UTV_UINT32      uiClassID;           /* event class identifier */
    UTV_UINT32      uiCount;             /* count of active event slots */
    UTV_UINT32      uiNext;              /* index of next available event slot */
    UTV_EVENT_ENTRY aEvent[UTV_PLATFORM_EVENTS_PER_CLASS]; /* event array */
} UTV_EVENT_CLASS;

/* Diagnostic stats
*/
typedef struct
{
    UTV_PUBLIC_SSTATE  utvSubState;         /* sub-state */
    UTV_UINT32  iSubStateWait;              /* sub-state wait interval */
    UTV_UINT32  iScanGood;                  /* Good scans count */
    UTV_UINT32  iScanAborted;               /* Scans aborted count */
    UTV_UINT32  iScanBad;                   /* Bad scans count */
    UTV_UINT32  iDownloadComplete;          /* Complete downloads count */
    UTV_UINT32  iDownloadPartial;           /* Partial downloads count */
    UTV_UINT32  iDownloadAborted;           /* Downloads aborted count */
    UTV_UINT32  iDownloadBad;               /* Bad downloads count */
    UTV_UINT32  iModCount;                  /* Count of modules downloaded */
    UTV_UINT32  iFreqListSize;              /* Count of total channels to scan */
    UTV_UINT32  iLastFreqTuned;             /* Last frequency tuned */
    UTV_UINT32  iChannelIndex;              /* Current channel index */
    UTV_UINT32  iBadChannelCount;           /* Count of channels with tune problems */
    UTV_UINT32  iNeedBlockCount;            /* Count of blocks needed for this download */
    UTV_UINT32  iReceivedBlockCount;        /* Count of blocks received during this download */
    UTV_UINT32  iUnreceivedBlockCount;      /* Count of blocks unreceived during last bad download */
    UTV_UINT32  iAlreadyHaveCount;          /* Count of blocks already encountered during download */
    UTV_BYTE    aubLastScanID[UTV_SERVER_ID_MAX_LEN]; /* Server ID where last scan came from */
    UTV_BYTE    aubLastDownloadID[UTV_SERVER_ID_MAX_LEN]; /* Server ID where last download came from */
    UTV_TIME    tLastScanTime;              /* Time of last scan */
    UTV_TIME    tLastCompatibleScanTime;    /* Time of last compatible scan */
    UTV_UINT32  uiTKTestResult;             /* Freq of last compatible scan */
    UTV_UINT16  iLastCompatiblePid;         /* PID of last compatible scan */
    UTV_TIME    tLastModuleTime;            /* Time of last module download */
    UTV_UINT32  iLastModuleFreq;            /* Freq last module download happened on */
    UTV_UINT16  iLastModulePid;             /* PID last module download happened on */
    UTV_TIME    tLastDownloadTime;          /* Time of last complete download */
    UTV_UINT32  iSectionsDelivered;         /* Count of sections delivered */
    UTV_UINT32  iSectionsDiscarded;         /* and discarded */
    UTV_EVENT_CLASS utvEvents[UTV_PLATFORM_EVENT_CLASS_COUNT]; /* inited by UtvPlatformPersistRead() */
    UTV_UINT32    uiTextOffset;             /* current offset into text store */
    UTV_INT8      cTextStore[UTV_PLATFORM_EVENT_TEXT_STORE_SIZE]; /* diagnostic text store */
    UTV_PUBLIC_ERR_STAT utvLastError;       /* last error encountered */
    UTV_UINT32    uiTotalReceivedByteCount; /* Count of total bytes received during this download */
    UTV_UINT32    uiLastBroadcastDiscovery;
    UTV_UINT32    uiNumBroadcastDiscoveries;
    UTV_UINT32    uiLastInternetDiscovery;
    UTV_UINT32    uiNumInternetDiscoveries;
    UTV_UINT32    uiLastFileDiscovery;
    UTV_UINT32    uiNumFileDiscoveries;
    UTV_UINT32    uiCmdCount;
    UTV_UINT32    uiCmdResponseMax;
    UTV_UINT32    uiCmdResponseMin;
    UTV_UINT32    uiDownloadResponseMax;
    UTV_UINT32    uiDownloadResponseMin;
    UTV_BOOL      bRegStoreSent;
    UTV_UINT32    uiReserved[ 16 ];
    UTV_RESULT    rLastDownloadStatus;
    UTV_RESULT    rLastError;
    UTV_TIME      tLastErrorTime;
    UTV_UINT32    uiErrorCount;
    UTV_UINT16    usLastDownloadEndedMV;
    UTV_UINT16    usLastDownloadEndedUnreportedMV;
    UTV_UINT16    usLastDownloadEndedType;
    UTV_UINT16    usLastDownloadEndedStatus;
    UTV_TIME      tLastDownloadEndedEvent;
#ifdef UTV_TEST_ABORT
    UTV_UINT32  iAbortCount;                /* abort testing - number of aborts */
    UTV_UINT32  iHighestAbortTime;          /* abort testing - highest (longest) abort time */
    UTV_UINT32  iAvgAbortTime;              /* abort testing - average abort time */
#endif

}  UtvAgentDiagStats;

/* Version structure
*/
typedef struct
{
    UTV_BYTE    ubMajor;
    UTV_BYTE    ubMinor;
    UTV_BYTE    ubRevision;
    UTV_BYTE    ubReserved;
    UTV_UINT16  usLastUpdateVersionBooted;
} UtvVersion;

/* UtvVarLenHeader
    A structure that contains descriptors of the variable length parts of perisistent memory.
*/
typedef struct
{
    UTV_UINT32  uiTrackerSig;           /* signature of tracker portion of variable length header */
    UTV_UINT16  usTrackerCount;         /* number of modules that have been downloaded in the current update */
    UTV_UINT16  usTrackerStructSize;    /* structure size of the tracker entries */
    UTV_UINT16  usTrackerModVer;        /* Module version of tracked modules */
    UTV_UINT32  uiProvisionerSig;       /* NO LONGER USED, left here for backwards compatibility */
    UTV_UINT16  usProvisionerStructSize;/* NO LONGER USED, left here for backwards compatibility */
    UTV_UINT16  usProvisionerCount;     /* NO LONGER USED, left here for backwards compatibility */
}  UtvVarLenHeader;

/* Global extern references to persistent structures.
    All of these pointers are declared in platform-persist.c
    They are accessed by routines in all aspects of the Agent.
*/
extern UtvAgentState                *g_pUtvState;
extern UtvAgentDiagStats            *g_pUtvDiagStats;
extern UtvDownloadInfo              *g_pUtvBestCandidate;
extern UTV_UINT32                    g_hManifestImage;
extern UtvDownloadTracker            g_utvDownloadTracker;

typedef struct
{
    UTV_UINT32    uiDownloadID;
    UTV_BOOL    bDownload;
} UtvDownloadEntry;

/* Image Access Interface Definition */

typedef UTV_RESULT UtvIAIOpenFunc ( UTV_INT8 *, UTV_INT8 *, UTV_UINT32 * );
typedef UTV_RESULT UtvIAIReadFunc ( UTV_UINT32, UTV_UINT32, UTV_UINT32, UTV_BYTE * );
typedef UTV_RESULT UtvIAICloseFunc( UTV_UINT32 );

typedef struct {
    UtvIAIOpenFunc    *pfOpen;
    UtvIAIReadFunc    *pfRead;
    UtvIAICloseFunc    *pfClose;
} UtvImageAccessInterface;

/* func type def used to retrieve partition information */
typedef UTV_RESULT UtvPartReadFunc (char *pszPartName, int bIndexable, unsigned char *pData, int iBytesToRead, int iOffset );

/* Core Prototypes common to all delivery methods follow
*/

/* core-common-Compatibility.c: compatibility functions
*/
UTV_RESULT UtvCoreCommonCompatibilityUpdate(
    UTV_UINT32 hImage,
    UTV_UINT32 uiOui,
    UTV_UINT16 usModelGroup,
    UTV_UINT16 usModuleVersion,
    UTV_UINT32 uiAttributes,
    UTV_UINT32 uiNumHMWEntries,
    UtvRange  *pHWMRange,
    UTV_UINT32 uiNumSWVEntries,
    UtvRange  *pSWVRange,
    UTV_UINT32 uiNumSWDeps,
    UtvDependency *pSWDeps );
UTV_RESULT
UtvCoreCommonCompatibilityUpdateConvert(UTV_UINT32 hImage, UTV_UINT32 uiOui, UTV_UINT16 usModelGroup,
                                        UTV_UINT16 usModuleVersion, UTV_UINT32 uiAttributes,
                                        UTV_UINT32 uiNumHMWEntries, UtvRange *pHWMRange,
                                        UTV_UINT32 uiNumSWVEntries, UtvRange *pSWVRange,
                                        UTV_UINT32 uiNumSWDeps, UtvDependency *pSWDeps );
UTV_RESULT UtvCoreCommonCompatibilityComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc );
UTV_RESULT UtvCoreCommonCompatibilityDownload( UTV_UINT32 iOui, UTV_UINT32 iModelGroupId );
UTV_RESULT UtvCoreCommonCompatibilityCountMods( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT32 *puiModulesWritten );
#ifdef UTV_DELIVERY_BROADCAST
UTV_RESULT UtvCoreCommonCompatibilityModule( UTV_UINT32 iOui, UTV_UINT32 iModelGroupId, UTV_UINT32 iModulePriority,
                                            UTV_UINT32 iModuleNameLength, UTV_BYTE * pModuleName, UTV_UINT32 iModuleVersion,
                                            UTV_UINT32 iHardwareModelBegin, UTV_UINT32 iHardwareModelEnd, UTV_UINT32 iSoftwareVersionBegin,
                                            UTV_UINT32 iSoftwareVersionEnd, UTV_UINT32 iModuleSize, UTV_UINT16 usModuleId,
                                            UTV_BOOL  *pbNewModuleVersionDetected );
#endif
void UtvCoreCommonCompatibilityDownloadInit( void );
void UtvCoreCommonCompatibilityDownloadStart( UTV_UINT32 hImage, UTV_UINT16 usModuleVersion );
UTV_RESULT UtvCoreCommonCompatibilityRecordModuleId( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT16 usModuleId );
void UtvCoreCommonCompatibilityDownloadClear( void );

/* core-common-control.c: application thread, control, dispatch
*/
void UtvAgentTask( void );
UTV_RESULT UtvScanEvent( void );
UTV_RESULT UtvDownloadEvent( void );
UTV_PUBLIC_RESULT_OBJECT * UtvGetAsyncResultObject( void );
void UtvSetCallbackFunction( UTV_PUBLIC_CALLBACK pCallback );
UTV_PUBLIC_CALLBACK UtvGetCallbackFunction( void );
void UtvBroadcastDownloadModulesWorker( void );
UTV_RESULT UtvConfigureDeliveryModes( UTV_UINT32 uiNumMethods, UTV_UINT32 *pMethods );
UTV_BOOL UtvCheckDeliveryMode( UTV_UINT32 uiDeliveryMode );

/* core-common-state.c: state routines
*/
UTV_RESULT UtvExit( void );
UTV_RESULT UtvAbort( void );
UTV_RESULT UtvActivateSelectMonitor( void *timeval, int socketFd );
UTV_RESULT UtvAbortSelectMonitor( void );
UTV_RESULT UtvDeactivateSelectMonitor( void );
UTV_PUBLIC_STATE UtvGetState( void );
UTV_RESULT UtvClearState( void );
UTV_RESULT UtvSetState( UTV_PUBLIC_STATE iNewState );
void UtvSetSubState( UTV_PUBLIC_SSTATE iSubState, UTV_UINT32 iWait );
UTV_UINT32 UtvGetSleepTime( void );
UTV_RESULT UtvSetSleepTime( UTV_UINT32 iMilliseconds );
UTV_RESULT UtvSetStateTime( UTV_PUBLIC_STATE iNewState, UTV_UINT32 iMilliseconds );
void UtvSetExitRequest( void );
int UtvGetExitRequest( void );
void UtvSaveServerIDScan( void );
void UtvSaveServerIDModule( void );
void UtvSaveLastScanTime( void );
void UtvSaveLastCompatibleScanTime( void );
void UtvSaveLastModuleTime( void );
void UtvSaveLastDownloadTime( void );
void UtvIncModuleCount( void );
void UtvIncChannelCount( void );
void UtvIncBadChannelCount( void );
UTV_UINT32 UtvGetDataDeliveryMode( void );
UTV_RESULT UtvSetDataDeliveryMode( UTV_UINT32 uiDDM );

/* The following is used to track when a socket select() call is started
   and when it completed. It is used to determine how long to wait before
   starting the final shutdown of the agent - especially under aborts
*/
#include <sys/time.h>
typedef struct
{
    int                 active;
    int                 socketFd;
    struct timeval      startTime;
    struct timeval      waitLength;
} selectMonitorControlBlock_t;

extern selectMonitorControlBlock_t g_selectMonitorControlBlock;

/* core-common-decrypt.c - module decryption routines
*/
UTV_RESULT UtvDecryptModule( UTV_BYTE * pInputModule, UTV_UINT32 iLengthIn, UTV_UINT32 * pLengthOut,
                             UTV_UINT32 uiOUI, UTV_UINT32 uiModelGroup, UtvKeyHdr *pUtvKeyHeaderIn, UtvKeyHdr *pUtvKeyHeaderOut );
UTV_RESULT UtvFixupKeyHeader( UtvKeyHdr *pKeyHeader, UTV_UINT32 uiModuleSize, UTV_UINT32 uiEncryptedModuleSize, UTV_BYTE *pubMessageDigest );
UTV_INT8 *UtvPrivateLibraryVersion( void );
void UtvGenerateSHA256Open( void );
void UtvGenerateSHA256Close( UTV_BYTE * pMessageDigest );
UTV_RESULT UtvGenerateSHA256Process( UTV_BYTE * pBuffer, UTV_UINT32 iBufferSize );
UTV_RESULT UtvGenerateSHA256( UTV_BYTE * pBuffer, UTV_UINT32 iBufferSize, UTV_BYTE * pMessageDigest );

#ifdef UTV_DELIVERY_INTERNET
UTV_RESULT UtvCommonDecryptInit( void );
UTV_RESULT UtvCommonDecryptGenerateUliInfo( UTV_BYTE *pszSerialNumber, UTV_BYTE *pubUliInfo );
UTV_RESULT UtvCommonDecryptGetPKID( UTV_UINT32 *uiPKID );
UTV_RESULT UtvCommonDecryptULIDRead( void );
UTV_RESULT UtvCommonDecryptULIDWrite( void );
UTV_RESULT UtvCommonDecryptGetSecurityModelUniqueKey( UTV_BYTE *pubKeyBuffer, UTV_UINT32 *puiKeyBufferSize  );
UTV_RESULT UtvCommonDecryptGetSecurityModelCommonKey( UTV_BYTE *pubKeyBuffer, UTV_UINT32 *puiKeyBufferSize  );
UTV_RESULT UtvCommonDecryptCommonFile( UTV_INT8 *pszFName, UTV_BYTE **pData, UTV_UINT32 *puiDataLen );
UTV_RESULT UtvCommonDecryptAES( UTV_BYTE *pInputData, UTV_BYTE *pOutputBuffer, UTV_UINT32 uiLengthIn, UTV_BYTE *pKey );
UTV_RESULT UtvCommonDecryptSplitULPK( UTV_BYTE *pubUlpk, UTV_UINT32 puiUlpkSize, UTV_BYTE *pubKeyBuffer, UTV_UINT32 *puiKeyBufferSize  );
#endif

#ifdef UTV_DELIVERY_INTERNET
/* core-internet-msg-protocol.c: msg functions.
 */
UTV_RESULT UtvInternetMsgDownloadRange( UTV_UINT32 hSession, UTV_INT8 *pszFileName, UTV_INT8 *pDownloadHost, UTV_UINT32 uiStartOffset, UTV_UINT32 uiEndOffset, UTV_BYTE *pubBuffer, UTV_UINT32 uiBufferSize );

/* core-internet-provisioner.c: Provisioned objects retrieval functions.
*/
UTV_RESULT UtvInternetProvisionerClose( void );
UTV_RESULT UtvInternetProvisionerRefresh( void );
UTV_RESULT UtvInternetProvisionerGetCount( UTV_UINT32 *puiObjectCount );
#ifndef UTV_FILE_BASED_PROVISIONING
UTV_RESULT UtvPlatformProvisionerGetObjectStatus( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                                  UTV_UINT32 *puiStatus, UTV_UINT32 *puiRequestCount );
UTV_RESULT UtvPlatformProvisionerGetObjectSize( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                                UTV_UINT32 *puiObjectSize );
UTV_RESULT UtvPlatformProvisionerGetObject( UTV_INT8 *pszOwner, UTV_INT8 *pszName,
                                            UTV_INT8 *pszObjectIDBuffer, UTV_UINT32 uiObjectIDBufferSize,
                                            UTV_BYTE *pubBuffer, UTV_UINT32 uiBufferSize,
                                            UTV_UINT32 *puiObjectSize, UTV_UINT32 *puiEncryptionType );
#endif

#ifdef UTV_DEBUG
void UtvDisplayProvisionedObjects( void );
#endif

/* core-internet-certs.c: deliver access to in-line certs
*/
char * UtvDeliverMrsCAFile(void);
#endif

/* core-common-manifest.c - component management routines
 */
UTV_RESULT UtvManifestOpen( void );
UTV_RESULT UtvManifestClose( void );
UTV_UINT16 UtvManifestGetSoftwareVersion( void );
UTV_UINT16 UtvManifestGetModuleVersion( void );
UTV_BOOL UtvManifestGetFieldTestStatus( void );
UTV_BOOL UtvManifestGetFactoryTestStatus( void );
UTV_BOOL UtvManifestGetLoopTestStatus( void );
UTV_BOOL UtvManifestAllowAdditions( void );
UTV_BOOL UtvManifestOptional( UTV_UINT32 uiImage );
UTV_RESULT UtvManifestCommit( UTV_UINT32 hImage );
UTV_INT8 *UtvManifestGetQueryHost( void );
UTV_UINT16 UtvManifestGetModelGroup( void );

/* core-common-image.c - image management routines
 */
UTV_RESULT UtvImageInit( void );
UTV_RESULT UtvImageOpen( UTV_INT8 *pszLocation, UTV_INT8 *pszFileName, UtvImageAccessInterface *pIAI, UTV_UINT32 *phImage, UTV_UINT32 hStore );
UTV_RESULT UtvImageGetCompDirHdr( UTV_UINT32 hImage, UtvCompDirHdr **ppCompDirHdr, UtvRange **ppHWMRanges, UtvRange **ppSWVRanges,
                                  UtvDependency **ppSWVDeps, UtvTextDef **ppTextDefs );
UTV_RESULT UtvImageGetTotalModuleCount( UTV_UINT32 hImage, UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_UINT32 *puiTotalModuleCount );
UTV_RESULT UtvImageGetCompDescByIndex( UTV_UINT32 hImage, UTV_UINT32 c, UtvCompDirCompDesc **ppCompDescUtvRange );
UTV_RESULT UtvImageGetCompDescByName( UTV_UINT32 hImage, UTV_INT8 *pszCompName, UtvCompDirCompDesc **ppCompDesc );
UTV_RESULT UtvImageGetCompIndex( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT16 *pusCompIndex  );
UTV_RESULT UtvImageGetCompDescInfo( UtvCompDirCompDesc *pCompDesc, UtvRange **ppHWMRanges, UtvRange **ppSWVRanges,
                                    UtvDependency **ppSWVDeps, UtvTextDef **ppTextDefs, UtvCompDirModDesc **ppModDesc );
UTV_RESULT UtvImageInstallModule( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UtvCompDirModDesc *pModDesc, UTV_BYTE *pBuffer, UtvModRecord *pModRecord );
UTV_RESULT UtvImageInstallComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc );
UTV_RESULT UtvImageTest( UTV_INT8 *pszLocation, UTV_INT8 *pszFileName, UtvImageAccessInterface *pIAI, UTV_BOOL bDecrypt );
UTV_RESULT UtvImageGetEncryptedCompDir( UTV_UINT32 hImage, UTV_BYTE **pCompDir, UTV_UINT32 *uiCompDirSize );
UTV_RESULT UtvImageGetStore( UTV_UINT32 hImage, UTV_UINT32 *puiStore );
UTV_RESULT UtvImageClose( UTV_UINT32 hImage );
UTV_RESULT UtvImageCloseAll( void );
UTV_RESULT UtvImageCheckFileLocks( UTV_UINT32 hImage );
UTV_RESULT UtvImageValidateHashes( UTV_UINT32 hImage );
UTV_RESULT UtvImageProcessUSBCommands( void );
UTV_RESULT UtvImageProcessPreBootCommands( void );
UTV_RESULT UtvImageProcessPostBootCommands( void );
UTV_RESULT UtvImageProcessPostEveryBootCommands( void );

/* core-common-interactive.c - interactive control routines
 */
UTV_RESULT UtvInstallUpdateComplete( UTV_UINT32 hImage, UTV_UINT16 usModuleVersion );
UTV_BOOL UtvGetInstallWithoutStoreStatus( void );

#ifdef UTV_INTERACTIVE_MODE
void UtvSetFactoryMode( UTV_BOOL bFactoryMode );
UTV_BOOL UtvGetFactoryMode( void );
UTV_RESULT UtvClearDownloadTimes ( void );
UTV_RESULT UtvSortSchedule( void );
void UtvGetDownloadScheduleWorker( void );
void *UtvGetDownloadScheduleWorkerAsThread( void * );
extern UTV_PUBLIC_SCHEDULE g_sSchedule;
#ifdef UTV_FACTORY_DECRYPT_SUPPORT
extern UTV_BOOL g_bDSIContainsFactoryDescriptor;
#endif
#ifdef UTV_DELIVERY_BROADCAST
extern UTV_BOOL     g_bAccumulateDownloadInfo;
#endif
#endif

#ifdef UTV_FACTORY_DECRYPT_SUPPORT
void UtvEnableFactoryDecrypt( UTV_BOOL bFactoryDecryptEnable );
UTV_BOOL UtvFactoryDecryptPermission( void );
#endif

/* core-common-base64.c - Base64 encode and decode routines
 */
UTV_RESULT UtvBase64Encode( UTV_BYTE* pBuffer, UTV_UINT32 uiBufferSize, UTV_BYTE** pBase64Buffer, UTV_UINT32* puiBase64BufferSize );
UTV_RESULT UtvBase64Decode( UTV_BYTE* pBase64Buffer, UTV_UINT32 uiBase64BufferSize, UTV_BYTE** pBuffer, UTV_UINT32* puiBufferSize );

/* Core Protos for the Broadcast delivery method only follow
 */
#ifdef UTV_DELIVERY_BROADCAST
/* core-broadcast-channel.c: Channel Scan
*/
UTV_RESULT UtvParsePmt( UtvCarouselCandidate * pUtvCarouselCandidate, UTV_BYTE * pSectionData );
UTV_RESULT UtvClearDownloadInfo( UtvDownloadInfo * pUtvBestCandidate );
UTV_RESULT UtvChannelScan( UtvDownloadInfo * pUtvBestCandidate );
void UtvClearCarouselCandidate(void);
UTV_RESULT UtvScanOneFrequency( UtvDownloadInfo * pUtvBestCandidate );
void UtvTunedChannelHasDownloadDataWorker( void );
void *UtvTunedChannelHasDownloadDataWorkerAsThread( void * );

/* core-broadcast-carousel.c: Carousel Scan
*/
UTV_RESULT UtvCarouselScan( UTV_UINT32 iCarouselPid, UtvDownloadInfo * pUtvBestCandidate );
UTV_RESULT UtvCalcTimeToDownload( UtvDownloadInfo * pUtvPossibleCandidate );

void UtvClearGroupCandidate(void);
void UtvClearDownloadCandidate(void);
char *UtvGetServerVersion( void );
char *UtvGetServerID( void );
void UtvSetChannelScanTime( UTV_UINT32 uiChannelScanTime );
UTV_UINT32 UtvGetChannelScanTime( void );
void UtvSetAttributeMatchCriteria( UTV_UINT32 uiAttributeBitMask, UTV_UINT32 uiAttributeSenseMask );

/* core-broadcast-parse.c: section data parse routines
*/
UTV_TID UtvStartSectionPointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize );
void UtvSkipBytesPointer( UTV_UINT32 iSkip, UTV_BYTE * * ppData, UTV_UINT32 * piSize );
UTV_UINT8 UtvGetUint8Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize );
UTV_UINT16 UtvGetUint16Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize );
UTV_UINT32 UtvGetUint24Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize );
UTV_UINT32 UtvGetUint32Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize );

/* core-broadcast-time.c: date and time routines
*/
UTV_RESULT UtvSetBroadcastTime( UTV_TIME dtBroadcastTime );
UTV_TIME UtvGetBroadcastTime( void );
UTV_RESULT UtvSetBroadcastTime( UTV_TIME dtBroadcastTime );
UTV_UINT32 UtvGetGpsUtcOffset( void );
UTV_RESULT UtvSetGpsUtcOffset( UTV_UINT32 iGpsUtcOffset );
UTV_UINT32 UtvGetMillisecondsToEvent( UTV_TIME dtEvent );
const char * UtvConvertGpsTime( UTV_TIME dtGps );

/* core-broadcast-sectionfilter.c: section filter
*/
UTV_RESULT UtvCreateSectionFilter( void );
UTV_RESULT UtvOpenSectionFilter( UTV_UINT32 iPid, UTV_UINT32 iTableId, UTV_UINT32 iTableIdExt, UTV_BOOL bEnableTableIdExt );
UTV_RESULT UtvGetSectionData( UTV_UINT32 * piTimer, UTV_BYTE * * ppSectionData );
UTV_RESULT UtvCloseSectionFilter( void );
UTV_RESULT UtvDestroySectionFilter( void );
UTV_RESULT UtvDeliverSectionFilterData( UTV_UINT32 iSectionDataLength, UTV_BYTE * pSectionData );
UTV_RESULT UtvSectionQueueInit( void );
UTV_RESULT UtvClearSectionQueue( void );
UTV_UINT32 UtvGetSectionsOverflowed( void );
UTV_UINT32 UtvGetSectionsHighWater( void );
void   UtvClearSectionsHighWater( void );
void   UtvOverrideSectionFilterStatus( UTV_SECTION_FILTER_STATUS eStatus );

/* core-broadcast-packetfilter.c: packet filter (optional)
*/
#ifdef UTV_CORE_PACKET_FILTER
void UtvDeliverTransportStreamPacket( UTV_UINT32 iDesiredPid, UTV_UINT32 iDesiredTableId, UTV_UINT32 iDesiredTableIdExt, UTV_BYTE * pTransportStreamPacket );
#endif /* UTV_CORE_PACKET_FILTER */

/* core-broadcast-download.c: Download Module
*/
UTV_RESULT UtvBroadcastDownloadComponentDirectory( UtvDownloadInfo * pUtvDownloadInfo );
UTV_RESULT UtvBroadcastDownloadGenerateSchedule( UtvDownloadInfo * pUtvDownloadInfo );
UTV_RESULT UtvBroadcastDownloadModule( UtvDownloadInfo * pUtvBroadcastDownloadModule );
UTV_RESULT UtvBroadcastDownloadCarousel( UtvDownloadInfo * pUtvDownloadInfo, UTV_BYTE * pModuleBuffer );

/* core-broadcast-image.c - Image Access Interface for broadcast.
 */
UTV_RESULT UtvBroadcastImageAccessInterfaceGet( UtvImageAccessInterface *pIAI );
UTV_RESULT UtvBroadcastImageAccessInterfaceOpen( UTV_INT8 *pszHost, UTV_INT8 *pszFileName, UTV_UINT32 *phImage );
UTV_RESULT UtvBroadcastImageAccessInterfaceRead( UTV_UINT32 hImage, UTV_UINT32 uiOffset, UTV_UINT32 uiLength, UTV_BYTE *pubBuffer );
UTV_RESULT UtvBroadcastImageAccessInterfaceClose( UTV_UINT32 hImage );

#endif /* UTV_DELIVERY_BROADCAST */

/* Core Protos for the Internet delivery method only follow
 */
#ifdef UTV_DELIVERY_INTERNET
/* core-internet-manage.c - Internet download management routines
 */
void UtvInternetReset_UpdateQueryDisabled(void);
UTV_RESULT UtvInternetRegister( UTV_BOOL bForceRegister, UTV_BOOL *pbJustRegistered );
UTV_RESULT UtvInternetMakeReRegisterRequest( void );
/* UtvInternetCheckForUpdates() pacing functions */
UTV_RESULT UtvInternetUpdateQueryPacingControl( void );
UTV_BOOL UtvInternetIsValidQueryResponseCached( void );
UTV_RESULT UtvInternetOverrideValidQueryResponseCached( UTV_BOOL newState );
time_t UtvInternetGetQueryPacingWaitTime( void );
UTV_RESULT UtvInternetSetQueryPacingWaitTime( time_t timeToWait );
time_t UtvInternetGetQueryRspTimestamp( void );
UTV_RESULT UtvInternetCheckForUpdates( UtvInternetUpdateQueryResponse **pputvUQR );
UTV_RESULT UtvInternetScheduleDownload( UtvInternetUpdateQueryEntry *putvUQE, UTV_BOOL bDownloadNow,
                                        UtvInternetScheduleDownloadResponse **pputvSDR );
UTV_RESULT UtvInternetDownload( UtvInternetScheduleDownloadResponse *putvSDR, UTV_UINT32 uiStoreIndex );
UTV_RESULT UtvInternetDownloadStart( UTV_UINT32 *hSession, UTV_INT8 *szHost );
UTV_RESULT UtvInternetDownloadRange( UTV_UINT32 *phSession, UTV_INT8 *pszHost, UTV_INT8 *pszFilePath, UTV_UINT32 uiStartOffset, UTV_UINT32 uiEndOffset,
                                     UTV_BYTE *pubBuffer, UTV_UINT32 uiBufferLen );
UTV_RESULT UtvInternetDownloadStop( UTV_UINT32 hSession );
UTV_RESULT UtvInternetDownloadEnded( UtvInternetScheduleDownloadResponse *putvSDR, UTV_UINT32 uiDownloadType, UTV_UINT32 uiStatus  );
UTV_RESULT UtvInternetManageCEMStatus( UTV_UINT32 uiNumStrings, UTV_INT8 *pszCategory, UTV_INT8 *apszNameStrings[], UTV_INT8 *apszValueStrings[] );
UTV_RESULT UtvInternetManageAvailMask( void );
UTV_RESULT UtvInternetStatusChange( UTV_UINT32 uiOUI, UTV_UINT16 usModelGroup, UTV_UINT16 usSoftwareVersion,
                                    UTV_UINT32 uiNumCEMVars, UTV_INT8 *pszCategory, UTV_INT8 *apszCEMVarName[], UTV_INT8 *apszCEMVarValue[],
                                    UTV_UINT32 uiNumCEMPrivateVars, UTV_INT8 *apszCEMPrivateVarValue[] );
UTV_RESULT UtvInternetProvision( UTV_UINT32 uiReadTimeout );
UTV_RESULT UtvInternetDownloadCompDir( UtvInternetScheduleDownloadResponse *putvSDR, UTV_BYTE *pBuffer, UTV_UINT32 uiBufferSize );
void UtvInternetLogEventFiltered( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_EARG aArg[] );
void UtvInternetLogEvent( UTV_UINT32 uiLevel, UTV_CATEGORY rCat, UTV_RESULT rMsg, UTV_TIME eTime, UTV_EARG aArg[], UTV_UINT32 uiCount );
UTV_RESULT UtvInternetRetrieveDiagInfo( UTV_UINT32 uiLevel );
UTV_BOOL UtvInternetRegistered( void );

/* core-internet-image.c - Image Access Interface for the internet.
 */
UTV_RESULT UtvInternetImageAccessInterfaceGet( UtvImageAccessInterface *pIAI );
UTV_RESULT UtvInternetImageAccessInterfaceInfo( UTV_INT8 **pszHostName, UTV_INT8 **pszFileName, UTV_UINT32 hImage );
UTV_RESULT UtvInternetImageAccessInterfaceOpen( UTV_INT8 *pszHost, UTV_INT8 *pszFileName, UTV_UINT32 *phImage );
UTV_RESULT UtvInternetImageAccessInterfaceRead( UTV_UINT32 hImage, UTV_UINT32 uiOffset, UTV_UINT32 uiLength, UTV_BYTE *pubBuffer );
UTV_RESULT UtvInternetImageAccessInterfaceClose( UTV_UINT32 hImage );
#endif /* UTV_DELIVERY_INTERNET */

/*
 * This section declares defines and functions used with the Device Web Service to
 * communicate with the NRS.
 */
#ifdef UTV_DEVICE_WEB_SERVICE
#define SESSION_ID_SIZE                             37
#define SESSION_CODE_SIZE                           11
#define SESSION_DATA_SIZE                           512

/*
 * UtvLiveSupportSessionInitiateInfo1 - Structure containing the required information to
 * initiate a Live Support sessions.
 */
typedef struct
{
  /*
   * NOTE: Setting the allowSessionCodeBypass flag to UTV_TRUE allows support technicians
   *       to lookup this session without using the generated session code.  Use this only
   *       when the user will not be able to view the session code and read it to the support
   *       technician.
   */
  UTV_BOOL                allowSessionCodeBypass;

  /*
   * Variables below are used internally by the Device Agent core code.  They do not need to
   * be set by CEM adaptation methods.
   */

  UTV_BOOL                secureMediaStream;
  UTV_BOOL                resumeExistingSession;
} UtvLiveSupportSessionInitiateInfo1;

/*
 * UtvLiveSupportSessionInfo1 - Structure containing information returned for a Live Support
 * session.
 */
typedef struct
{
  UTV_BYTE                id[SESSION_ID_SIZE];
  UTV_BYTE                code[SESSION_CODE_SIZE];
  UTV_BYTE                data[SESSION_DATA_SIZE];
} UtvLiveSupportSessionInfo1;


/* core-internet-devicewebservice.c - Helper methods used to communicate with the NRS
 * using the Device Web Service.  These methods are used internally by the Device Agent
 * and should not be called.
 */

void UtvInternetDeviceWebServiceInit( void );
void UtvInternetDeviceWebServiceShutdown( void );
UTV_RESULT UtvInternetDeviceWebServiceEcho( void );

/* DEPRECATED - use UtvInternetDeviceWebServiceLiveSupportInitiateSession1 */
UTV_RESULT UtvInternetDeviceWebServiceLiveSupportInitiateSession(
  UTV_BYTE* sessionId,
                                                                  UTV_UINT32 sessionIdSize,
                                                                  UTV_BYTE* sessionCode,
                                                                  UTV_UINT32 sessionCodeSize,
                                                                  void* capabilities );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportInitiateSession1(
  UtvLiveSupportSessionInitiateInfo1* sessionInitiateInfo,
  UtvLiveSupportSessionInfo1* sessionInfo );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportGetSessionData1(
  UTV_BYTE* data,
  UTV_UINT32 dataSize );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportTerminateSession(
  UTV_BYTE* sessionId,
  UTV_BOOL bInternallyInitiated );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportGetNextTask(
  UTV_BYTE* sessionId,
                                                              UTV_INT32 timeoutInSeconds,
                                                              void** task );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportCompleteTask(
  UTV_BYTE* sessionId,
                                                               UTV_INT32 taskId,
                                                               void* output );

UTV_RESULT UtvInternetDeviceWebServiceLiveSupportStringToDateTime(
  UTV_INT8* szDateTime,
  time_t* pTime );
#endif /* UTV_DEVICE_WEB_SERVICE */

#ifdef UTV_TEST_ABORT
/* abort.c - abort test support
*/
void UtvAbortTestInit( void );
void UtvAbortTestShutdown( void );
void UtvAbortComplete( void );
void UtvAbortAcquireHardware( void );
void UtvAbortReleaseHardware( void );
void UtvAbortEnterSysCall( void );
void UtvAbortExitSysCall( void );
#endif  /* UTV_TEST_ABORT */

#ifdef UTV_LIVE_SUPPORT
#define UTV_LIVE_SUPPORT_COMMAND_TYPE_UNKNOWN     0x00
#define UTV_LIVE_SUPPORT_COMMAND_TYPE_CONTROL     0x01
#define UTV_LIVE_SUPPORT_COMMAND_TYPE_DIAGNOSTIC  0x02
#define UTV_LIVE_SUPPORT_COMMAND_TYPE_TOOL        0x03
#endif /* UTV_LIVE_SUPPORT */

#if defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT)
#define UTV_REMOTE_BUTTON_UNKNOWN               0
#define UTV_REMOTE_BUTTON_POWER                 1
#define UTV_REMOTE_BUTTON_INPUT                 2
#define UTV_REMOTE_BUTTON_YELLOW                3
#define UTV_REMOTE_BUTTON_BLUE                  4
#define UTV_REMOTE_BUTTON_RED                   5
#define UTV_REMOTE_BUTTON_GREEN                 6
#define UTV_REMOTE_BUTTON_EXIT                  7
#define UTV_REMOTE_BUTTON_MENU                  8
#define UTV_REMOTE_BUTTON_GUIDE                 9
#define UTV_REMOTE_BUTTON_BACK                 10
#define UTV_REMOTE_BUTTON_UP                   11
#define UTV_REMOTE_BUTTON_DOWN                 12
#define UTV_REMOTE_BUTTON_LEFT                 13
#define UTV_REMOTE_BUTTON_RIGHT                14
#define UTV_REMOTE_BUTTON_ENTER                15
#define UTV_REMOTE_BUTTON_VOL_UP               16
#define UTV_REMOTE_BUTTON_VOL_DOWN             17
#define UTV_REMOTE_BUTTON_VOL_MUTE             18
#define UTV_REMOTE_BUTTON_CHANNEL_UP           19
#define UTV_REMOTE_BUTTON_CHANNEL_DOWN         20
#define UTV_REMOTE_BUTTON_CHANNEL_RECALL       21
#define UTV_REMOTE_BUTTON_INTERNET_APPS        22
#define UTV_REMOTE_BUTTON_PIP                  23
#define UTV_REMOTE_BUTTON_ASPECT               24
#define UTV_REMOTE_BUTTON_NUM1                 25
#define UTV_REMOTE_BUTTON_NUM2                 26
#define UTV_REMOTE_BUTTON_NUM3                 27
#define UTV_REMOTE_BUTTON_NUM4                 28
#define UTV_REMOTE_BUTTON_NUM5                 29
#define UTV_REMOTE_BUTTON_NUM6                 30
#define UTV_REMOTE_BUTTON_NUM7                 31
#define UTV_REMOTE_BUTTON_NUM8                 32
#define UTV_REMOTE_BUTTON_NUM9                 33
#define UTV_REMOTE_BUTTON_NUM0                 34
#define UTV_REMOTE_BUTTON_DOT                  35
#endif /* defined(UTV_LAN_REMOTE_CONTROL) || defined(UTV_LIVE_SUPPORT) */

#endif /* __UTVCORE_H__ */
