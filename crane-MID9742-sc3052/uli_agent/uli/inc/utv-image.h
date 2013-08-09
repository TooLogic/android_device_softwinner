/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 - 2011 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * UpdateTV Common Core -  Common Image Structure Definitions
 *
 * In order to ensure UpdateTV network compatibility, customers must not
 * change UpdateTV Common Core code.
 *
 * These defines and typdefs are shared with the UpdateTV tools.
 *
 * @file      utv-image.h
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
  Bob Taylor      06/22/2009  Loop test bit added.
  Bob Taylor      03/01/2009  Extended for modules.
  Bob Taylor      02/24/2009  Cleanup for agent/tools share.
  Bob Taylor      02/17/2009  Created.
*/

#ifndef __UTVIMAGE_H__
#define __UTVIMAGE_H__

/* ============================================================== */
/* ================= Common Definitions ========================= */
/* ============================================================== */

#define UTV_FILE_NAME_FMT    "%06lX-%04X.utv"

#define UTV_SIGNATURE_SIZE 16

/* macro to convert text offset into text pointer */
#define UTV_CONVERT_TEXT_OFFSET_TO_PTR( S_PTR, T_OFFSET ) (((UTV_INT8 *) S_PTR) + T_OFFSET )

/* common typedefs */
typedef unsigned short UtvTextOffset;

/* ============================================================== */
/* ============= Encryption Header Definition =================== */
/* ============================================================== */

#define UTV_HEAD_SIGNATURE          "BDC\tCDB\tBDC\tCDB\t\0"
#define UTV_TRAIL_SIGNATURE        "CDB\tBDC\tCDB\tBDC\t\0"

/* The UtvKeyHdr version is broken down into 4 bytes as follows:
     byte 0 - version #
     byte 1 - hash type (0==SHA,FF==None)
     byte 2 - payload encryption type, 0 = full 3DES, FF = clear/not encrypted
     byte 3 - always 0 to ensure proper decryption results
*/
#define UTV_ENCRYPT_VER_VERSION                         0x02
#define UTV_ENCRYPT_VER_PAYLOAD_3DES                    0x00
#define UTV_ENCRYPT_VER_PAYLOAD_AES                     0x01
#define UTV_ENCRYPT_VER_PAYLOAD_EXT                     0x02
#define UTV_ENCRYPT_VER_PAYLOAD_CLEAR                   0xFF
#define UTV_ENCRYPT_VER_HEADERAD_HEADER_CLEAR           0xFE
#define UTV_ENCRYPT_VER_PAYLOAD_HEADER_NO_HASH_CLEAR    0xFD

#define UTV_ENCRYPT_HASH_SHA                            0x00
#define UTV_ENCRYPT_HASH_NONE                           0xFF

/* Decryption key info
*/
#define UTV_KEY_SIZE_IN_BITS    2048
#define UTV_ASCII_KEY_SIZE      UTV_KEY_SIZE_IN_BITS / 4

/* Hash algorithm message settings
*/
#define UTV_SHA_MESSAGE_BLOCK_SIZE       64
#define UTV_SHA_MESSAGE_DIGEST_LENGTH    32

#define UTV_CLEAR_HDR_VERSION                1
#define UTV_KEY_HDR_VERSION                  1
#define UTV_KEY_HDR_ENCRYPT_TYPE_NONE        0
#define UTV_KEY_HDR_ENCRYPT_TYPE_RSA         1
#define UTV_KEY_HDR_ENCRYPT_TYPE_RSA_OAEP    2
#define UTV_KEY_HDR_ENCRYPT_TYPE_RSA_PRIVATE 3

/* Structure for clear header portion of encrypted module */
typedef struct
{
    unsigned char   ubUTVSignature[UTV_SIGNATURE_SIZE];
    unsigned char   ubClearHeaderVersion;
    unsigned char   ubKeyHeaderVersion;
    unsigned char   ubKeyHeaderEncryptType;
    unsigned char   ubReserved[1];
    unsigned long   uiOUI;
    unsigned long   uiModelGroup;
    unsigned long   uiEncryptedImageSize;
} UtvClearHdr;

/* Structure for AES decrypt keys */
typedef struct
{
    unsigned char   ubKey1[8];
    unsigned char   ubKey2[8];
    unsigned char   ubKey3[8];
} Utv3DESKeys;

/* Structure for AES decrypt keys */
#define UTV_MAX_AES_KEY_LENGTH_IN_BITS    256
#define UTV_MAX_AES_KEY_LENGTH_IN_BYTES    (UTV_MAX_AES_KEY_LENGTH_IN_BITS/8)
typedef struct
{
    unsigned char   ubKeyLengthInBytes;
    unsigned char   ubReserved[3];
    unsigned char   ubKey[UTV_MAX_AES_KEY_LENGTH_IN_BYTES];
} UtvAESKey;

/* Structure for Ext decrypt keys */
#define UTV_MAX_EXT_KEY_LENGTH_IN_BYTES    16
typedef struct
{
    unsigned char   ubKey1[UTV_MAX_EXT_KEY_LENGTH_IN_BYTES];
    unsigned char   ubKey2[UTV_MAX_EXT_KEY_LENGTH_IN_BYTES];
} UtvExtKey;

/* union of the key types that we currently support */
typedef union
{
    Utv3DESKeys     DES;
    UtvAESKey       AES;
    UtvExtKey       EXT;
} UtvDecryptKeys;

/* Structure for RSA encrypted header portion of encrypted module */
typedef struct
{
    unsigned char    ubVersion;
    unsigned char    ubHashType;
    unsigned char    ubPayloadEncryptionType;
    unsigned char    ubReserved;
    UtvDecryptKeys   utvDecryptKeys;
    unsigned char    ubMessageDigest[UTV_SHA_MESSAGE_DIGEST_LENGTH];
    unsigned long    uiModuleLength;
    unsigned long    uiEncryptBlocks;
    unsigned char    ubUTVSignature[UTV_SIGNATURE_SIZE];
    /* keysize (256) minus struct size (96) and zero byte (1) */
    unsigned char    ubPadding[159];
    unsigned char    ubZerobyte;
} UtvKeyHdr;

/* ============================================================== */
/* ============= Component Directory Definition ================= */
/* ============================================================== */

/* current component directory structure version */
#define UTV_COMP_DIR_VERSION          1

/* Estimated maximum size of a component directory.  Used to
   sanity check a potenitally "spoofed" clear header. */
#define UTV_MAX_COMP_DIR_SIZE        8000

#define UTV_COMP_DIR_SIGNATURE      "BDC\tCOM\tDIR\tSTR\t\0"
#define UTV_COMPRESSION_TYPE_NONE    0

/* component directory module name */
#define UTV_COMP_DIR_MOD_NAME        "compdir-01of01"
#define UTV_COMP_DIR_ENC_NAME        "compdir-01of01.enc"

typedef struct
{
    /* signature to allow the Agent to detect whether this is a
       version 2.0 Directory Component */
    unsigned char    ubCompDirSignature[UTV_SIGNATURE_SIZE];
    unsigned char    ubStructureVersion;
    unsigned char    ubCreateAppID;
    unsigned char    ubCreateVerMajor;
    unsigned char    ubCreateVerMinor;
    unsigned char    ubCreateVerRevision;
    unsigned char    ubBroadcastDistribution;
    unsigned char    ubReserved[ 2 ];
    unsigned short   toCreateUserName;
    unsigned long    uiPlatformOUI;
    unsigned short   usPlatformModelGroup;
    unsigned short   usUpdateCompressionType;
    /* reserved for now, global attributes */
    unsigned long    uiUpdateAttributes;
    /* global module version is the only module version used. */
    unsigned short   usUpdateModuleVersion;
    /* global software version is only used to tie an this group of components to
       a named update (submitted CAB file) on the NOC.  It's never checked by the Agent. */
    unsigned short   usUpdateSoftwareVersion;
    unsigned short   usNumUpdateHardwareModelRanges;
    unsigned short   usNumUpdateSoftwareVersionRanges;
    unsigned short   usNumUpdateSoftwareDependencies;
    unsigned short   usNumUpdateTextDefs;
    unsigned short   usNumComponents;
} UtvCompDirHdr;

/* The Component Directory Header is followed by usNumComponents number of unsigned short
   offsets to an array of UtvCompDirCompDesc structures followed by their variable
   length dependency arrays.
   The offsets are from the end of the UtvCompDirHdr structure
*/

/* unsigned short OFFSET to UtvCompDirComponentEntry zero */
/* unsigned short OFFSET to UtvCompDirComponentEntry one  */
/* unsigned short OFFSET to UtvCompDirComponentEntry two...  */

/* The Offsets are followed by usNumUpdateHardwareModelRanges
   Update-Level UtvRange structures that contain the hardware model Compatibility info
   for the entire update. */

typedef struct
{
    unsigned short usStart;
    unsigned short usEnd;
} UtvRange;

/* The Hardware Model Compatibility is followed by usNumUpdateSoftwareVersionRanges
   Update-Level UtvRange structures containing the software version Compatibility info for
   the entire update. */

/* The Software Version Compatibility Ranges are followed by UPdate-Level usNumUpdateSoftwareDependencies
   UtvDependency structures containing the software component dependencies for the entire update */

#pragma pack(push,r1,2)
typedef struct
{
    UtvTextOffset   toComponentName;
    UtvRange        utvRange;
} UtvDependency;
#pragma pack(pop,r1)

/* The Software Dependencies are followed by usNumUpdateTextDirectives number of Update-Level
   UtvTextDirective structures for the entire update.
*/
typedef struct
{
    UtvTextOffset    toIdentifier;
    UtvTextOffset    toText;
} UtvTextDef;

/* The Update-Level Text Directives are followed by Component Descriptors */

/* the download decision attributes should be in the lower eight bits
   of the attribute field.  Currently there are four:  loop test, factory test,
   field test, and whether the component is a component directory.
   The reason the download decision attributes are in the lower 8 bits
   is because the 8-bit priority field of the DII is where they get
   transmitted in the broadcast delivery mode. */

/* this bit doesn't have a corresponding publisher directive */
#define UTV_COMPONENT_ATTR_CDIR        0x00000001

/* -field_test means that the component is enabled for public field testing */
#define UTV_COMPONENT_ATTR_FTEST       0x00000002

/* -factory_test means that the component is enabled for factory testing */
#define UTV_COMPONENT_ATTR_FACT_TEST   0x00000004

/* -loop_test means that the component is enabled for loop testing */
#define UTV_COMPONENT_ATTR_LOOP_TEST   0x00000008

/* The rest of these flags are not used to make download decisions so
   therefore they live outside of the lower 8 bits. */

/* -optional means user will be given a choice whether to accept it or not
   during a user download query */
#define UTV_COMPONENT_ATTR_OPTIONAL    0x00000100

/* -reveal means it is displayed during a user download query */
#define UTV_COMPONENT_ATTR_REVEAL      0x00000200

/* -store means the Agent will store and not install the component
   after download. */
#define UTV_COMPONENT_ATTR_STORE       0x00000400

/* -reboot means system will reboot after install */
#define UTV_COMPONENT_ATTR_REBOOT      0x00000800

/* -agent means this component contains the UTV Agent */
#define UTV_COMPONENT_ATTR_AGENT       0x00001000

/* -partition means this component is installed as a partition
   if not set then it is assumed this component is stored as a file */
#define UTV_COMPONENT_ATTR_PARTITION   0x00002000

/* -prevent_adds means that no new components are allowed to be added */
#define UTV_COMPONENT_ATTR_NO_ADDS     0x00004000

/* -command means that commands are embedded into the update */
#define UTV_COMPONENT_ATTR_COMMAND     0x00008000

/* -cem1-16 sets flags for a component that may be interpreted by the CEM in any way they choose */
#define UTV_COMPONENT_ATTR_CEM1        0x00010000
#define UTV_COMPONENT_ATTR_CEM2        0x00020000
#define UTV_COMPONENT_ATTR_CEM3        0x00040000
#define UTV_COMPONENT_ATTR_CEM4        0x00080000
#define UTV_COMPONENT_ATTR_CEM5        0x00100000
#define UTV_COMPONENT_ATTR_CEM6        0x00200000
#define UTV_COMPONENT_ATTR_CEM7        0x00400000
#define UTV_COMPONENT_ATTR_CEM8        0x00800000
#define UTV_COMPONENT_ATTR_CEM9        0x01000000
#define UTV_COMPONENT_ATTR_CEM10       0x02000000
#define UTV_COMPONENT_ATTR_CEM11       0x04000000
#define UTV_COMPONENT_ATTR_CEM12       0x08000000
#define UTV_COMPONENT_ATTR_CEM13       0x10000000
#define UTV_COMPONENT_ATTR_CEM14       0x20000000
#define UTV_COMPONENT_ATTR_CEM15       0x40000000
#define UTV_COMPONENT_ATTR_CEM16       0x80000000

typedef struct
{
    unsigned long    uiComponentAttributes;
    unsigned long    uiOUI;
    unsigned short   usModelGroup;
    UtvTextOffset    toName;
    /* for possible future expansion. Locked to the update MV for now. */
    unsigned short   usModuleVersion;
    unsigned short   usSoftwareVersion;
    unsigned long    uiImageSize;
    unsigned short   usNumHardwareModelRanges;
    unsigned short   usNumSoftwareVersionRanges;
    unsigned short   usNumSoftwareDependencies;
    unsigned short   usNumTextDefs;
    unsigned short   usModuleCount;
} UtvCompDirCompDesc;

/* Each UtvCompDirCompDesc is followed by usNumHardwareModelRanges
   UtvRange structures that contain the hardware model Compatibility info. */

/* The Hardware Model Compatibility is followed by usNumSoftwareVersionRanges
   UtvRange structures containing the software version Compatibility info for
   this component */

/* The Software Version Compatibility Ranges are followed by usNumSoftwareDependencies
   UtvDependency structures containing the software component dependencies */

/* The Software Dependencies are followed by usNumtestDirectives number of UtvTextDirective
   structures.
*/

/* The Text Directives are followed by usModuleCount number of UtvCompDirModDesc
   structures.
*/

typedef struct
{
    unsigned long    uiModuleOffset; /* offset to module start from bottom of UtvCompDirHdr struct */
    unsigned long    uiModuleSize;     /* CEM module plus Module Signaling Header */
    unsigned long    uiEncryptedModuleSize;
    unsigned char    ubMessageDigest[UTV_SHA_MESSAGE_DIGEST_LENGTH];
} UtvCompDirModDesc;

/* The UtvCompDirModDesc structures are followed by the text block.  The text block is
   a variable length blob of zero terminated strings. The offsets to these strings
   are stored in the structures above. */

/* "example component name", "example component display name" "example user", etc. */

/* ============================================================== */
/* =========== Module Signaling Header Definition =============== */
/* ============================================================== */

#define UTV_MOD_SIG_HDR_SIGNATURE "BDC\tMOD\tSIG\tHDR\t\0"

/* current module signaling header structure version */
#define UTV_MOD_SIG_HDR_VERSION                1

typedef struct
{
    unsigned char    ubModSigHdrSignature[UTV_SIGNATURE_SIZE];
    unsigned char    ubStructureVersion;
    /* size of this header PLUS all variable length elements that
       follow it. */
    unsigned short   usHeaderSize;
    unsigned char    ubReserved;
    UtvTextOffset    toComponentName;
    unsigned short   usModuleVersion;
    unsigned short   usModuleIndex;
    unsigned short   usModuleCount;
    /* size of the CEM component data in this module */
    unsigned long    uiCEMModuleSize;
    /* offset into CEM component data of this module */
    unsigned long    uiCEMModuleOffset;
} UtvModSigHdr;

/* Each UtvModSigHdr is followed by a text block whic currently only contains the component name */

#endif /* __UTVIMAGE_H__ */
