/* core-common-image.c: UpdateTV Common Core
                        Image Access Manager

 This module implements understands how images are put together and
 allows calling routines to gain access to them via "Image Access Interfaces"
 which hides whether the image is in a file or on the internet somewhere.

 In order to ensure UpdateTV network compatibility, customers must not
 change UpdateTV Core code including this file.

 Copyright (c) 2009-2010 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Jim Muchow      11/02/2011  Download Resume Support
  Nathan Ford     02/21/2011  Encrypted version of component manifest is now held in memory so it can be written during
                              component manifest commit.
  Jim Muchow      09/21/2010  BE support.
  Bob Taylor      06/14/2010  Command debug consolidation.
  Nathan Ford     05/20/2010  Added UtvImageClose() calls to UtvImageProcessCommands for proper cleanup.
  Bob Taylor      04/09/2010  UtvImageProcessUSBCommands() added.
  Chris Nigbur    02/02/2010  UtvImageInit() added.
  Bob Taylor      11/05/2009  Tolerate not open on aux write platform close to eliminate spurious warning.
                              Renamed UtvImageValidatePartitionHashes() to UtvImageValidateHashes() and
                              added file hash functionality.  File handle fix to file lock.  Added UtvImageCloseAll().
  Bob Taylor      06/30/2009  UtvImageGetTotalModuleCount() modified to help with mod tracker functionality.
  Bob Taylor      03/08/2009  Created.
*/

#include "utv.h"

/* define to debug command processing */
/* #define UTV_CMD_DEBUG */

#define UTV_IAM_MAX_IMAGES    6

typedef struct
{
    UtvImageAccessInterface utvIAI;
    UTV_UINT32              hRemoteStore;
    UTV_BYTE               *pClearCompDir;      /* Unencrypted conponent manifest */
    UTV_BYTE               *pEncryptedCompDir;  /* Encrypted version of the component manifest */
    UTV_UINT32              uiCompDirSize;      /* Size of the encrypted component manifest */
    UtvKeyHdr               utvKeyHeader;
    UTV_BOOL                bOpen;
} UtvImageAccessEntry;

static UtvImageAccessEntry UtvImageAccessArray[ UTV_IAM_MAX_IMAGES ] = {
    { {0}, 0, 0, 0, 0, {0}, 0 },
    { {0}, 0, 0, 0, 0, {0}, 0 },
    { {0}, 0, 0, 0, 0, {0}, 0 },
    { {0}, 0, 0, 0, 0, {0}, 0 },
    { {0}, 0, 0, 0, 0, {0}, 0 },
    { {0}, 0, 0, 0, 0, {0}, 0 }
};

/* static protos */
static UTV_RESULT UtvImageProcessCommands( UTV_UINT32 uiCommandFmtString, UTV_UINT32 uiStoreIndex );
static UTV_RESULT UtvImageCheckSerialNumberTargeting( UTV_UINT32 uiStoreIndex );

UTV_RESULT UtvImageInit()
{
  UTV_UINT32 i;

  for ( i = 0; i < UTV_IAM_MAX_IMAGES; i++)
  {
    UtvImageAccessArray[ i ].bOpen = UTV_FALSE;
  }

  return UTV_OK;
}

UTV_RESULT UtvImageOpen( UTV_INT8 *pszLocation, UTV_INT8 *pszFileName, UtvImageAccessInterface *pIAI, UTV_UINT32 *phImage, UTV_UINT32 hAuxWriteStore )
{
    UTV_UINT32    i, uiLengthOut, uiBytesWritten, scratch;
    UTV_UINT16    usModelGroup;
    UTV_RESULT    rStatus;
    UtvClearHdr  *pUtvClearHeader;

    /* store the image acces info in an available storage entry if there is one. */
    for ( i = 0; i < UTV_IAM_MAX_IMAGES; i++ )
        if ( !UtvImageAccessArray[ i ].bOpen )
            break;

    if ( UTV_IAM_MAX_IMAGES <= i )
        return UTV_NO_IMAGES_ENTRIES;

    /* entry available, call the access open function to see if we can at least get that far. */
    rStatus = pIAI->pfOpen( pszLocation, pszFileName, &UtvImageAccessArray[ i ].hRemoteStore );
    if ( UTV_OK != rStatus && UTV_ALREADY_OPEN != rStatus )
    {
        /* don't report errors which are allowed */
        if ( UTV_STORE_EMPTY != rStatus )
            UtvMessage( UTV_LEVEL_ERROR, "pIAI->pfOpen() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* open worked, save IAI */
    UtvImageAccessArray[ i ].utvIAI = *pIAI;

    /* snag the component directory */

    /* allocate space for it */
    if ( NULL == ( UtvImageAccessArray[ i ].pEncryptedCompDir = UtvMalloc( UTV_MAX_COMP_DIR_SIZE )))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc( UTV_MAX_COMP_DIR_SIZE ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        pIAI->pfClose( UtvImageAccessArray[ i ].hRemoteStore );
        return rStatus;
    }
    if ( NULL == ( UtvImageAccessArray[ i ].pClearCompDir = UtvMalloc( UTV_MAX_COMP_DIR_SIZE )))
    {
        rStatus = UTV_NO_MEMORY;
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc( UTV_MAX_COMP_DIR_SIZE ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        pIAI->pfClose( UtvImageAccessArray[ i ].hRemoteStore );
        UtvFree( UtvImageAccessArray[ i ].pEncryptedCompDir );
        return rStatus;
    }

    /* all of the following checks result in the same error exit, thus the while false trick */
    do
    {
        /* get its clear header */
        if ( UTV_OK != ( rStatus = pIAI->pfRead( UtvImageAccessArray[ i ].hRemoteStore, 0, sizeof( UtvClearHdr ), UtvImageAccessArray[ i ].pEncryptedCompDir )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "pIAI->pfRead() \"%s\"", UtvGetMessageString( rStatus ) );
            break;
        }

        pUtvClearHeader = (UtvClearHdr *) UtvImageAccessArray[ i ].pEncryptedCompDir;

        /* signature first */
        if ( UtvMemcmp( pUtvClearHeader->ubUTVSignature, (void *) UTV_HEAD_SIGNATURE, UTV_SIGNATURE_SIZE ) )
        {
            rStatus = UTV_NO_CRYPT_HEADER;
            break;
        }

        /* then clear header version */
        if ( UTV_CLEAR_HDR_VERSION != pUtvClearHeader->ubClearHeaderVersion )
        {
            rStatus = UTV_BAD_CLEAR_VERSION;
            break;
        }

        /* then key header version */
        if ( UTV_KEY_HDR_VERSION != pUtvClearHeader->ubKeyHeaderVersion )
        {
            rStatus = UTV_BAD_KEY_VERSION;
            break;
        }

        /* then size */
        UtvImageAccessArray[ i ].uiCompDirSize = pUtvClearHeader->uiEncryptedImageSize;
        if ( Utv_32letoh( UtvImageAccessArray[ i ].uiCompDirSize ) > UTV_MAX_COMP_DIR_SIZE )
        {
            rStatus = UTV_BAD_CRYPT_MODULE_SIZE;
            break;
        }

        /* then OUI */
        if ( UtvCEMGetPlatformOUI() != Utv_32letoh(pUtvClearHeader->uiOUI) )
        {
            rStatus = UTV_INCOMPATIBLE_OUI;
            break;
        }

        /* allow all model groups to work with this component manifest if not in lock mode */
#ifdef UTV_MODEL_GROUP_LOCK
        /* model group */
        if ( UtvCEMGetPlatformModelGroup() != Utv_32letoh(pUtvClearHeader->uiModelGroup) )
        {
            rStatus = UTV_INCOMPATIBLE_MGROUP;
            break;
        }
        usModelGroup = (UTV_UINT16) UtvCEMGetPlatformModelGroup();
#else
        usModelGroup = (UTV_UINT16) Utv_32letoh(pUtvClearHeader->uiModelGroup);
#endif

        scratch = Utv_32letoh( UtvImageAccessArray[ i ].uiCompDirSize );

        /* read the rest of the component directory in. */
        if ( UTV_OK != ( rStatus = pIAI->pfRead( UtvImageAccessArray[ i ].hRemoteStore,
                                                 sizeof( UtvClearHdr ),
                                                 scratch - sizeof( UtvClearHdr ),
                                                &UtvImageAccessArray[ i ].pEncryptedCompDir[ sizeof( UtvClearHdr ) ] )))
            break;

        /* if the hAuxWriteStore parameter has a legal storage value in it then write the component directory out before it's decrypted */
        if ( UTV_PLATFORM_UPDATE_STORE_INVALID != hAuxWriteStore )
        {
            UTV_UINT32 hStore;

            /* close the store which may be open for reading.  Tolerate errors */
            if ( UTV_OK != ( rStatus = UtvPlatformStoreClose( hAuxWriteStore )))
            {
                /* tolerate not open */
                if ( UTV_NOT_OPEN != rStatus )
                    UtvMessage( UTV_LEVEL_WARN, "UtvPlatformStoreClose( hAuxWriteStore ) fails with \"%s\"", UtvGetMessageString( rStatus ) );
            }

            /* Open the aux store */
            if ( UTV_OK != ( rStatus = UtvPlatformStoreOpen( hAuxWriteStore, UTV_PLATFORM_UPDATE_STORE_FLAG_WRITE | UTV_PLATFORM_UPDATE_STORE_FLAG_OVERWRITE,
                                                             UTV_MAX_COMP_DIR_SIZE, &hStore, NULL, NULL )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformStoreOpen( hAuxWriteStore ) fails with error \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            /* write to the store */
            if ( UTV_OK != ( rStatus = UtvPlatformStoreWrite( hAuxWriteStore, UtvImageAccessArray[ i ].pEncryptedCompDir,
                                                              Utv_32letoh( UtvImageAccessArray[ i ].uiCompDirSize ), &uiBytesWritten )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformStoreWrite( %d, %d ) fails: %s", 
                            Utv_32letoh( UtvImageAccessArray[ i ].uiCompDirSize ),
                            uiBytesWritten, UtvGetMessageString( rStatus ) );
                return rStatus;
            }
        }

        /* copy the encrypted component directory to pClearCompDir to do an inplace decrypt */
        UtvByteCopy( UtvImageAccessArray[ i ].pClearCompDir, UtvImageAccessArray[ i ].pEncryptedCompDir, UTV_MAX_COMP_DIR_SIZE );

        /* decrypt it */
        if ( UTV_OK != ( rStatus = UtvDecryptModule( UtvImageAccessArray[ i ].pClearCompDir, scratch, &uiLengthOut,
                                                     UtvCEMGetPlatformOUI(), usModelGroup, NULL, &UtvImageAccessArray[ i ].utvKeyHeader )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvDecryptModule( cd ) fails: %s", UtvGetMessageString( rStatus ) );
            break;
        }

        /* Check signature */
        if ( UtvMemcmp( ((UtvCompDirHdr *) UtvImageAccessArray[ i ].pClearCompDir)->ubCompDirSignature, (void *) UTV_COMP_DIR_SIGNATURE, UTV_SIGNATURE_SIZE ) )
        {
            rStatus = UTV_BAD_DIR_SIG;
            break;
        }

        /* Check structure revision */
        if ( UTV_COMP_DIR_VERSION != ((UtvCompDirHdr *) UtvImageAccessArray[ i ].pClearCompDir)->ubStructureVersion )
        {
            rStatus = UTV_BAD_DIR_VER;
            break;
        }

    } while (UTV_FALSE);

    if ( UTV_OK != rStatus )
    {
        UtvFree( UtvImageAccessArray[ i ].pEncryptedCompDir );
        UtvFree( UtvImageAccessArray[ i ].pClearCompDir );
        pIAI->pfClose( UtvImageAccessArray[ i ].hRemoteStore );
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageAccessManagerOpen() fails: %s", UtvGetMessageString( rStatus ) );
    } else
    {
        /* looks like everything worked OK, we're open for business */
        UtvImageAccessArray[ i ].bOpen = UTV_TRUE;
        *phImage = i;

#ifdef UTV_CMD_DEBUG
        {
            UTV_INT8 *pszTextID, *pszTextDef;
            UTV_UINT32 n;
    
            for ( n = 0; UTV_OK == UtvPublicImageGetUpdateTextId( i, n, &pszTextID); n++ )
            {
                if ( UTV_OK != ( rStatus = UtvPublicImageGetUpdateTextDef( i, pszTextID, &pszTextDef )))
                {
                    UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetCompTextDef() fails: \"%s\"", UtvGetMessageString( rStatus ) );
                    return rStatus;
                }
                UtvMessage( UTV_LEVEL_INFO, "Text Def %d: %s => %s", n, pszTextID, pszTextDef );
            }
        }
#endif
    }
    
    return rStatus;                
}

UTV_RESULT UtvImageClose( UTV_UINT32 hImage )
{
    UTV_RESULT    rStatus;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    rStatus = UtvImageAccessArray[ hImage ].utvIAI.pfClose( UtvImageAccessArray[ hImage ].hRemoteStore );
    if ( UTV_OK != rStatus )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageClose: hImage %d close of hRemoteStore %d returned \"%s\"", 
                    hImage, UtvImageAccessArray[ hImage ].hRemoteStore,
                    UtvGetMessageString( rStatus ) );
        /* inform user, but continue */
    }

    /* free the comp dir space */
    UtvFree( UtvImageAccessArray[ hImage ].pClearCompDir );
    UtvFree( UtvImageAccessArray[ hImage ].pEncryptedCompDir );

    UtvImageAccessArray[ hImage ].bOpen = UTV_FALSE;

    return UTV_OK;
}

UTV_RESULT UtvImageCloseAll( void )
{
    UTV_UINT32    hImage;

    for ( hImage = 0; hImage < UTV_IAM_MAX_IMAGES; hImage++ )
    {
        if ( !UtvImageAccessArray[ hImage ].bOpen )
            continue;

        UtvImageClose( hImage );
    }

    return UTV_OK;
}

UTV_RESULT UtvImageGetEncryptedCompDir( UTV_UINT32 hImage, UTV_BYTE **pCompDir, UTV_UINT32 *uiCompDirSize )
{
    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    *pCompDir = UtvImageAccessArray[ hImage ].pEncryptedCompDir;
    *uiCompDirSize = UtvImageAccessArray[ hImage ].uiCompDirSize;

    return UTV_OK;
}

UTV_RESULT UtvImageGetStore( UTV_UINT32 hImage, UTV_UINT32 *puiStore )
{
    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    *puiStore = UtvImageAccessArray[ hImage ].hRemoteStore;

    return UTV_OK;
}

/* UtvImageGetCompDirHdr
    Return a pointer to the component directory header and its sub-parts.
*/
UTV_RESULT UtvImageGetCompDirHdr( UTV_UINT32 hImage, UtvCompDirHdr **ppCompDirHdr, UtvRange **ppHWMRanges, UtvRange **ppSWVRanges,
                                  UtvDependency **ppSWVDeps, UtvTextDef **ppTextDefs )
{
    UtvCompDirHdr    *pCompDirHdr;
    UtvRange        *pRange;
    UtvDependency    *pDependency;
    UTV_UINT16        *pTempPtr;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pCompDirHdr = *ppCompDirHdr = (UtvCompDirHdr *) (UtvImageAccessArray[ hImage ].pClearCompDir);

    /* skip past comp desc offsets to ranges */
    pTempPtr = (UTV_UINT16 *) (pCompDirHdr + 1);
    pTempPtr += Utv_16letoh( pCompDirHdr->usNumComponents );
    pRange = (UtvRange *) pTempPtr;

    *ppHWMRanges = pRange;
    pRange += Utv_16letoh( pCompDirHdr->usNumUpdateHardwareModelRanges );
    *ppSWVRanges = pRange;
    pRange += Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareVersionRanges );
    pDependency = (UtvDependency *) pRange;
    *ppSWVDeps = pDependency;
    pDependency += Utv_16letoh( pCompDirHdr->usNumUpdateSoftwareDependencies );
    *ppTextDefs = (UtvTextDef *) pDependency;

    return UTV_OK;
}

/* UtvImageGetTotalModuleCount
    Return the total number of modules in all components that have been approved for download
    minus the ones that have already been downloaded.
*/
UTV_RESULT UtvImageGetTotalModuleCount( UTV_UINT32 hImage, UTV_PUBLIC_UPDATE_SUMMARY *pUpdate, UTV_UINT32 *puiTotalModuleCount )
{
    UTV_RESULT             rStatus;
    UtvCompDirHdr         *pCompDirHdr;
    UtvCompDirCompDesc    *pCompDesc;
    UTV_UINT32             c, uiModCount;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pCompDirHdr = (UtvCompDirHdr *) UtvImageAccessArray[ hImage ].pClearCompDir;

    /* look through component list */
    for ( uiModCount = c = 0; c < Utv_16letoh( pCompDirHdr->usNumComponents ); c++ )
    {
        if ( !pUpdate->tComponents[ c ].bApprovedForStorage &&
             !pUpdate->tComponents[ c ].bApprovedForInstall )
             continue;

        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, c, &pCompDesc )))
            return rStatus;

        uiModCount += Utv_16letoh( pCompDesc->usModuleCount );
    }

    /* subtract any modules that have already been received. */
    uiModCount -= g_utvDownloadTracker.usCurDownloadCount;

    *puiTotalModuleCount = uiModCount;

    return UTV_OK;
}


/* UtvImageGetCompDescByIndex
    Return a pointer to a component descriptor based on its index.
*/
UTV_RESULT UtvImageGetCompDescByIndex( UTV_UINT32 hImage, UTV_UINT32 c, UtvCompDirCompDesc **ppCompDesc )
{
    UtvCompDirHdr            *pCompDirHdr;
    UTV_UINT16               *pusCompSummaryOffset;
    unsigned short theOffset;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pCompDirHdr = (UtvCompDirHdr *) UtvImageAccessArray[ hImage ].pClearCompDir;

    /* check that component is in bounds */
    if ( c >= Utv_16letoh( pCompDirHdr->usNumComponents ) )
        return UTV_BAD_COMPONENT_INDEX;

    pusCompSummaryOffset = (UTV_UINT16 *) (pCompDirHdr + 1);
    pusCompSummaryOffset += c;
    theOffset = Utv_16letoh( *pusCompSummaryOffset );
    *ppCompDesc = (UtvCompDirCompDesc *) ((UTV_BYTE *) (pCompDirHdr + 1) + theOffset);

    return UTV_OK;
}

/* UtvImageGetCompDescByName
    Return a pointer to a component descriptor based on its name.
*/
UTV_RESULT UtvImageGetCompDescByName( UTV_UINT32 hImage, UTV_INT8 *pszCompName, UtvCompDirCompDesc **ppCompDesc )
{
    UTV_RESULT            rStatus;
    UtvCompDirHdr         *pCompDirHdr;
    UTV_UINT32            c, uiTargetStrLen = UtvStrlen( (UTV_BYTE *) pszCompName );
    UTV_INT8            *pszPossibleName;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pCompDirHdr = (UtvCompDirHdr *) UtvImageAccessArray[ hImage ].pClearCompDir;

    /* look through component list */
    for ( c = 0; c < Utv_16letoh( pCompDirHdr->usNumComponents ); c++ )
    {
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, c, ppCompDesc )))
            return rStatus;

        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                                          Utv_16letoh( (*ppCompDesc)->toName ), 
                                                          &pszPossibleName )))
            return rStatus;

        if ( uiTargetStrLen != UtvStrlen( (UTV_BYTE *) pszPossibleName ) )
            continue;

        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszCompName, (UTV_BYTE *) pszPossibleName, uiTargetStrLen ))
            continue;

        /* found it */
        break;
    }

    if ( c >= Utv_16letoh( pCompDirHdr->usNumComponents ) )
        return UTV_UNKNOWN_COMPONENT;

    return UTV_OK;
}

UTV_RESULT UtvImageGetCompIndex( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT16 *pusCompIndex  )
{
    UtvCompDirHdr            *pCompDirHdr;
    UTV_UINT16               *pusCompSummaryOffset, i;
    UtvCompDirCompDesc       *pCandidateCompDesc;
    unsigned short theOffset;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pCompDirHdr = (UtvCompDirHdr *) UtvImageAccessArray[ hImage ].pClearCompDir;

    /* spin around hte component descriptor list looking for this component descriptor */
    for ( pusCompSummaryOffset = (UTV_UINT16 *) (pCompDirHdr + 1), i = 0;
          i < Utv_16letoh( pCompDirHdr->usNumComponents );
          i++, pusCompSummaryOffset++ )
    {
        theOffset = Utv_16letoh( *pusCompSummaryOffset );
        pCandidateCompDesc = (UtvCompDirCompDesc *) ((UTV_BYTE *) (pCompDirHdr + 1) + theOffset );
        if ( pCandidateCompDesc == pCompDesc )
        {
            *pusCompIndex = (UTV_UINT16) i;
            return UTV_OK;
        }
    }

    return UTV_BAD_COMPONENT_INDEX;
}

/* UtvImageGetCompDescInfo
    Return info related to the specified component descriptor.
*/
UTV_RESULT UtvImageGetCompDescInfo( UtvCompDirCompDesc *pCompDesc, UtvRange **ppHWMRanges, UtvRange **ppSWVRanges,
                                    UtvDependency **ppSWVDeps, UtvTextDef **ppTextDefs, UtvCompDirModDesc **ppModDesc )
{
    UtvRange               *pRange;
    UtvDependency          *pDependency;
    UtvTextDef               *pTextDef;

    pRange = (UtvRange *) (pCompDesc + 1);

    *ppHWMRanges = pRange;
    pRange += Utv_16letoh( pCompDesc->usNumHardwareModelRanges );
    *ppSWVRanges = pRange;
    pRange += Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges );
    pDependency = (UtvDependency *) pRange;
    *ppSWVDeps = pDependency;
    pDependency += Utv_16letoh( pCompDesc->usNumSoftwareDependencies );
    pTextDef = *ppTextDefs = (UtvTextDef *) pDependency;
    pTextDef += Utv_16letoh( pCompDesc->usNumTextDefs );
    *ppModDesc = (UtvCompDirModDesc *) pTextDef;

    return UTV_OK;
}

/* UtvPublicImageGetText
    Convert a text offset to a string for a given update.
*/
UTV_RESULT UtvPublicImageGetText( UTV_UINT32 hImage, UtvTextOffset toText, UTV_INT8 **ppText )
{
    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    *ppText = UTV_CONVERT_TEXT_OFFSET_TO_PTR( UtvImageAccessArray[ hImage ].pClearCompDir, toText );

    return UTV_OK;
}

/* UtvPublicImageGetUpdateTextId
    Look up an update-level text identifier by index.
*/
UTV_RESULT UtvPublicImageGetUpdateTextId( UTV_UINT32 hImage, UTV_UINT32 uiIndex, UTV_INT8 **ppszTextIdentifier )
{
    UTV_RESULT                rStatus;
    UtvRange               *pRange;
    UtvDependency           *pSWDeps;
    UtvTextDef               *pTextDef;
    UtvCompDirHdr           *pCompDirHdr;

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
        return rStatus;

    /* reject illegal indices */
    if ( uiIndex >= Utv_16letoh( pCompDirHdr->usNumUpdateTextDefs ) )
        return UTV_BAD_TEXT_DEF_INDEX;

    pTextDef += uiIndex;

    *ppszTextIdentifier = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, pTextDef->toIdentifier );
    return UTV_OK;
}

/* UtvPublicImageGetCompTextId
    Look up a component-level text identifier by index.
*/
UTV_RESULT UtvPublicImageGetCompTextId( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT32 uiIndex, UTV_INT8 **ppszTextIdentifier )
{
    UtvRange               *pRange;
    UtvDependency           *pSWDeps;
    UtvTextDef               *pTextDef;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    /* reject illegal indices */
    if ( uiIndex >= Utv_16letoh( pCompDesc->usNumTextDefs ) )
        return UTV_BAD_TEXT_DEF_INDEX;

    pRange = (UtvRange *) (pCompDesc + 1);
    pRange += Utv_16letoh( pCompDesc->usNumHardwareModelRanges );
    pRange += Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges );
    pSWDeps = (UtvDependency *) pRange;
    pSWDeps += Utv_16letoh( pCompDesc->usNumSoftwareDependencies );
    pTextDef = (UtvTextDef *) pSWDeps;
    pTextDef += uiIndex;
    *ppszTextIdentifier = UTV_CONVERT_TEXT_OFFSET_TO_PTR( UtvImageAccessArray[ hImage ].pClearCompDir, 
                                                          Utv_16letoh( pTextDef->toIdentifier ) );
    return UTV_OK;
}

/* UtvPublicImageGetUpdateTextDef
    Look up a update-level text definition by text id string.
*/
UTV_RESULT UtvPublicImageGetUpdateTextDef( UTV_UINT32 hImage, UTV_INT8 *pszTextID, UTV_INT8 **ppszTextDef )
{
    UTV_RESULT                rStatus;
    UTV_UINT32                i;
    UTV_INT8               *pszCandidateID;
    UtvRange               *pRange;
    UtvDependency           *pSWDeps;
    UtvTextDef               *pTextDef;
    UtvCompDirHdr           *pCompDirHdr;

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
        return rStatus;

    /* spin through the text defs looking for this one */
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateTextDefs ); i++, pTextDef++ )
    {
        pszCandidateID = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, Utv_16letoh( pTextDef->toIdentifier ) );

        if ( UtvStrlen( (UTV_BYTE *) pszTextID ) != UtvStrlen( (UTV_BYTE *) pszCandidateID ) )
            continue;

        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszTextID, (UTV_BYTE *) pszCandidateID, UtvStrlen( (UTV_BYTE *) pszCandidateID ) ))
            continue;

        /* got it */
        *ppszTextDef = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, Utv_16letoh( pTextDef->toText ) );
        return UTV_OK;
    }

    return UTV_TEXT_DEF_NOT_FOUND;
}

/* UtvPublicImageGetUpdateTextDefPartial
    Look up a update-level text definition and id by match string, allow partial match.
*/
UTV_RESULT UtvPublicImageGetUpdateTextDefPartial( UTV_UINT32 hImage, UTV_INT8 *pszMatchText, UTV_INT8 **ppszTextID, UTV_INT8 **ppszTextDef )
{
    UTV_RESULT                rStatus;
    UTV_UINT32                i;
    UTV_INT8               *pszCandidateID;
    UtvRange               *pRange;
    UtvDependency           *pSWDeps;
    UtvTextDef               *pTextDef;
    UtvCompDirHdr           *pCompDirHdr;

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
        return rStatus;

    /* spin through the text defs looking for this one */
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumUpdateTextDefs ); i++, pTextDef++ )
    {
        pszCandidateID = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, Utv_16letoh( pTextDef->toIdentifier ) );

        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszMatchText, (UTV_BYTE *) pszCandidateID, UtvStrlen( (UTV_BYTE *) pszMatchText ) ))
            continue;

        /* got it */
        *ppszTextID  = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, Utv_16letoh( pTextDef->toIdentifier ) );
        *ppszTextDef = UTV_CONVERT_TEXT_OFFSET_TO_PTR( pCompDirHdr, Utv_16letoh( pTextDef->toText ) );
        return UTV_OK;
    }

    return UTV_TEXT_DEF_NOT_FOUND;
}

/* UtvPublicImageGetCompTextDef
    Look up a component-level text definition by text id string.
*/
UTV_RESULT UtvPublicImageGetCompTextDef( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_INT8 *pszTextID, UTV_INT8 **ppszTextDef )
{
    UtvRange               *pRange;
    UtvDependency          *pSWDeps;
    UtvTextDef             *pTextDef;
    UTV_UINT32              i;
    UTV_INT8               *pszCandidateID;

    /* check that handle is OK */
    if ( UTV_IAM_MAX_IMAGES <= hImage )
        return UTV_BAD_HANDLE;

    if ( !UtvImageAccessArray[ hImage ].bOpen )
        return UTV_NOT_OPEN;

    pRange = (UtvRange *) (pCompDesc + 1);
    pRange += Utv_16letoh( pCompDesc->usNumHardwareModelRanges );
    pRange += Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges );
    pSWDeps = (UtvDependency *) pRange;
    pSWDeps += Utv_16letoh( pCompDesc->usNumSoftwareDependencies );

    /* spin through the text defs looking for this one */
    for ( i = 0, pTextDef = (UtvTextDef *) pSWDeps; i < Utv_16letoh( pCompDesc->usNumTextDefs ); i++, pTextDef++ )
    {

        pszCandidateID = UTV_CONVERT_TEXT_OFFSET_TO_PTR( UtvImageAccessArray[ hImage ].pClearCompDir, 
                                                         Utv_16letoh( pTextDef->toIdentifier ) );

        if ( UtvStrlen( (UTV_BYTE *) pszTextID ) != UtvStrlen( (UTV_BYTE *) pszCandidateID ) )
            continue;

        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszTextID, (UTV_BYTE *) pszCandidateID, UtvStrlen( (UTV_BYTE *) pszCandidateID ) ))
            continue;

        /* got it */
        *ppszTextDef = UTV_CONVERT_TEXT_OFFSET_TO_PTR( UtvImageAccessArray[ hImage ].pClearCompDir, 
                                                       Utv_16letoh( pTextDef->toText ) );
        return UTV_OK;
    }

    return UTV_TEXT_DEF_NOT_FOUND;
}

/* UtvImageInstallModule()
    Install the specified module.
*/
UTV_RESULT UtvImageInstallModule( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, 
                                  UtvCompDirModDesc *pModDesc, UTV_BYTE *pBuffer,
                                  UtvModRecord *pModRecord)
{
    UTV_RESULT              rStatus;
    UtvCompDirHdr          *pCompDirHdr;
    UtvRange               *pRange;
    UtvDependency          *pSWDeps;
    UtvTextDef             *pTextDef;
    UtvModSigHdr           *pModSigHdr;
    UTV_UINT32              uiLengthOut;

    /* get the component directory header to check the embedded keys */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* If the component has the same OUI and Model Group as the platform then try to decrypt it using the
       component directory key and the message digest stored in the module description. */
    if ( ( Utv_32letoh( pCompDirHdr->uiPlatformOUI ) == Utv_32letoh( pCompDesc->uiOUI ) ) && 
         ( Utv_16letoh( pCompDirHdr->usPlatformModelGroup) == Utv_16letoh( pCompDesc->usModelGroup ) ) )
    {
        /* decrypt it using the component directory's 3DES keys after fixing up the local scratch key header with the
           module size, encrypted size, and message digest for this module */
        if ( UTV_OK != ( rStatus = UtvFixupKeyHeader( &UtvImageAccessArray[ hImage ].utvKeyHeader, 
                                                      Utv_32letoh( pModDesc->uiModuleSize ), 
                                                      Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                      pModDesc->ubMessageDigest )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvFixupKeyHeader( ) fails: %s", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        UtvMessage( UTV_LEVEL_INFO, " FAST DECRYPT START" );
        /* decrypt using the comp dir key */
        if ( UTV_OK != ( rStatus = UtvDecryptModule( pBuffer, 
                                                     Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                     &uiLengthOut,
                                                     Utv_32letoh( pCompDirHdr->uiPlatformOUI ), 
                                                     Utv_16letoh( pCompDirHdr->usPlatformModelGroup ),
                                                     &UtvImageAccessArray[ hImage ].utvKeyHeader, NULL )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvDecryptModule( comp dir header ) fails: %s", UtvGetMessageString( rStatus ) );
            return rStatus;
        }
        UtvMessage( UTV_LEVEL_INFO, " FAST DECRYPT DONE" );

    } else
    {
        UtvMessage( UTV_LEVEL_INFO, " FULL DECRYPT START" );
        /* the OUI or MG is different than the platform OUI/MG.  Attempt a decrypt using the module's key header */
        if ( UTV_OK != ( rStatus = UtvDecryptModule( pBuffer, Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                     &uiLengthOut,
                                                     Utv_32letoh( pCompDesc->uiOUI ), 
                                                     Utv_16letoh( pCompDesc->usModelGroup ), NULL, NULL )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvDecryptModule( native header ) fails: %s", UtvGetMessageString( rStatus ) );
            return rStatus;
        }
        UtvMessage( UTV_LEVEL_INFO, " FULL DECRYPT DONE" );
    }

    /* check out the module signaling header that sits at the top of the module */
    pModSigHdr = (UtvModSigHdr *) pBuffer;

    /* Check signature */
    if ( UtvMemcmp( pModSigHdr->ubModSigHdrSignature, (void *) UTV_MOD_SIG_HDR_SIGNATURE, UTV_SIGNATURE_SIZE ) )
    {
        rStatus = UTV_BAD_MOD_SIG;
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageInstallComponentWorker fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* Check structure revision */
    if ( UTV_MOD_SIG_HDR_VERSION != pModSigHdr->ubStructureVersion )
    {
        rStatus = UTV_BAD_MOD_VER;
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageInstallComponentWorker fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* display mod hdr info */
    UtvMessage( UTV_LEVEL_INFO, "            Module Header Info:  Component name: %s, MV: 0x%X, Index: %d, Count: %d",
                UTV_CONVERT_TEXT_OFFSET_TO_PTR( pModSigHdr, Utv_16letoh( pModSigHdr->toComponentName )), 
                Utv_16letoh( pModSigHdr->usModuleVersion ), 
                Utv_16letoh( pModSigHdr->usModuleIndex ),
                Utv_16letoh( pModSigHdr->usModuleCount ) );

    UtvMessage( UTV_LEVEL_INFO, "                                 Header size: %d, CEM module size: %d, CEM module offset: %d",
                Utv_16letoh( pModSigHdr->usHeaderSize ), 
                Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                Utv_32letoh( pModSigHdr->uiCEMModuleOffset ) );

    UtvMessage( UTV_LEVEL_INFO, " PLATFORM MODULE INSTALL START" );
    /* install this module */
    rStatus = UtvPlatformInstallModuleData( hImage, pCompDesc, pModSigHdr, 
                                            pBuffer + Utv_16letoh( pModSigHdr->usHeaderSize ) );
    if ( UTV_OK != rStatus && UTV_COMP_INSTALL_COMPLETE != rStatus )
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallModuleData() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }
    UtvMessage( UTV_LEVEL_INFO, " PLATFORM MODULE INSTALL DONE" );

    if ( NULL != pModRecord )
    {
        pModRecord->ulSize = Utv_32letoh( pModSigHdr->uiCEMModuleSize);
        pModRecord->ulOffset = Utv_32letoh( pModSigHdr->uiCEMModuleOffset);
        pModRecord->ulCRC = UtvCrc32(pBuffer + Utv_16letoh( pModSigHdr->usHeaderSize ),
                                     Utv_32letoh( pModSigHdr->uiCEMModuleSize));
    }

    return rStatus;
}

/* UtvImageInstallComponent()
    Install the specified component from the image specified.
    Assumes hImage is open.
*/
UTV_RESULT UtvImageInstallComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc )
{
    UTV_RESULT              rStatus;
    UTV_UINT32              n;
    UtvCompDirHdr          *pCompDirHdr;
    UtvRange               *pRange;
    UtvDependency          *pSWDeps;
    UtvTextDef             *pTextDef;
    UtvCompDirModDesc      *pModDesc;
    UTV_BYTE               *pBuffer;

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* get the module descriptor for this component */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDescInfo( pCompDesc, &pRange, &pRange, &pSWDeps, &pTextDef, &pModDesc )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescInfo() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    for ( n = 0; n < Utv_16letoh( pCompDesc->usModuleCount ); n++, pModDesc++ )
    {
        UtvMessage( UTV_LEVEL_INFO, "            Module %d of %d, size: %lu, encrypted size: %lu, offset: %lu",
                    n + 1, Utv_16letoh( pCompDesc->usModuleCount ),
                    Utv_32letoh( pModDesc->uiModuleSize ), 
                    Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                    Utv_32letoh( pModDesc->uiModuleOffset ) );
                    
        g_pUtvDiagStats->uiTotalReceivedByteCount += Utv_32letoh( pModDesc->uiModuleSize );

        /* allocate memory for the entire encrypted module */
        if ( NULL == ( pBuffer = UtvPlatformInstallAllocateModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ) )))
        {
            rStatus = UTV_NO_MEMORY;
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallAllocateModuleBuffer( %d ) fails: %s", 
                        Utv_16letoh( pModDesc->uiEncryptedModuleSize ), UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        /* read it in */
        if ( UTV_OK != ( rStatus = UtvImageAccessArray[ hImage ].utvIAI.pfRead( UtvImageAccessArray[ hImage ].hRemoteStore, 
                                                                                Utv_32letoh( pModDesc->uiModuleOffset ),
                                                                                Utv_32letoh( pModDesc->uiEncryptedModuleSize ),
                                                                                pBuffer )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageAccessArray[ hImage ].utvIAI.pfRead() \"%s\"", UtvGetMessageString( rStatus ) );
            UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
            return rStatus;
        }

        rStatus = UtvImageInstallModule( hImage, pCompDesc, pModDesc, pBuffer, NULL );
        if ( UTV_OK != rStatus && UTV_COMP_INSTALL_COMPLETE != rStatus )
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageInstallComponentWorker() \"%s\"", UtvGetMessageString( rStatus ) );
            UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
            return rStatus;
        }

        /* Free the buffer that we allocated for this module */
        UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );

    } /* for each module in this component */

    /* if successful notify the platform that this component install is complete */
    if ( UTV_COMP_INSTALL_COMPLETE == rStatus )
    {
        if ( UTV_COMP_INSTALL_COMPLETE != ( rStatus = UtvPlatformInstallComponentComplete( hImage, pCompDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInstallComponentComplete() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        }
    }

    return rStatus;
}

UTV_RESULT UtvImageValidateHashes( UTV_UINT32 hImage )
{
    UTV_RESULT              rStatus = UTV_OK;
    UTV_UINT32              n, i, uiBufferLen, uiIndexable, uiPartSize, uiOffset;
    UTV_UINT32              uiReadThisTime, uiAmountToProcess, uiTotalLeft, uiHandle;
    UTV_INT8               *pszTextID, *pszTextDef, cMessageDigestString[256], cNum[3];
    UTV_INT8                szDescBuff[ 256 ], *pszCompName, *pszIndexable, *pszSize;
    UTV_BYTE                ubMessageDigest[UTV_SHA_MESSAGE_BLOCK_SIZE], *pBuff;
    UtvCompDirCompDesc     *pCompDesc;

    /* Allocate a reasonable sized chunk of memory to process the hashes in.
       Use 2MB in UTV_SHA_MESSAGE_BLOCK_SIZE units.
    */
    uiBufferLen = ( UTV_SHA_MESSAGE_BLOCK_SIZE * 32678 );

    if ( NULL == (pBuff = UtvMalloc( uiBufferLen )))
    {
        UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, UTV_NO_MEMORY, __LINE__, uiBufferLen );
        return UTV_NO_MEMORY;
    }

    /* if there are any hashes check them */
    for ( i = 0; UTV_TRUE; i++ )
    {
        UTV_INT8 cMatch[ 32 ];
        UtvMemFormat( (UTV_BYTE *) cMatch, 32, "_validp%d", i );
        if ( UTV_OK != UtvPublicImageGetUpdateTextDefPartial( hImage, cMatch, &pszTextID, &pszTextDef ))
        {
            rStatus = UTV_OK;
            break; /* can't find one?  we're done. */
        }

        /* spin past lock directive */
        pszTextID++;
        while ( '_' != *pszTextID )
            pszTextID++;
        pszTextID++;

        UtvStrnCopy( (UTV_BYTE *) szDescBuff, sizeof( szDescBuff ), (UTV_BYTE *) pszTextID, UtvStrlen( (UTV_BYTE *) pszTextID ));
        pszTextID = szDescBuff;

        /* isolate the partition name, indexable, and size parameters in the string */
        pszCompName = pszTextID;

        /* terminate the part name */
        while ( '$' != *pszTextID )
            pszTextID++;
        *pszTextID++ = 0;

        pszIndexable = pszTextID;
        uiIndexable = UtvStrtoul( pszIndexable, NULL, 0 );

        while ( '$' != *pszTextID )
            pszTextID++;
        *pszTextID++ = 0;

        pszSize = pszTextID;
        uiPartSize = UtvStrtoul( pszSize, NULL, 0 );

        /* look up the component */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByName( hImage, pszCompName, &pCompDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvImageGetCompDescByName( %s ): %s", pszCompName, UtvGetMessageString( rStatus ) );
            break;
        }

        /* open the currentlty inactive file/partition */
        if ( 0 == ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION ) )
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallFileOpenRead( hImage, pCompDesc, UTV_FALSE, &uiHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileOpenRead(%s): %s", pszCompName, UtvGetMessageString( rStatus ) );
                break;
            }
        } else
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionOpenRead( hImage, pCompDesc, UTV_FALSE, &uiHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionOpenRead(%s): %s", pszCompName, UtvGetMessageString( rStatus ) );
                break;
            }
        }

        /* initialize the SHA256 generator */
        UtvGenerateSHA256Open();

        uiOffset = 0;
        uiTotalLeft = uiPartSize;

        /* while there is data to be processed */
        while ( 0 != uiTotalLeft )
        {
            /* abort check before part read */
            if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            {
                rStatus = UTV_ABORT;
                break;
            }

            if ( uiTotalLeft >= uiBufferLen )
                uiReadThisTime = uiBufferLen;
            else
                uiReadThisTime = uiTotalLeft;

            /* attempt to read in a chunk of data from the file/partition */
            if ( 0 == ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION ) )
            {
                if ( UTV_OK != ( rStatus = UtvPlatformInstallFileRead( uiHandle, pBuff, uiReadThisTime )))
                {
                    UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, UTV_UNKNOWN, __LINE__, rStatus );
                    break;
                }
            } else
            {
                if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionRead( uiHandle, pBuff, uiReadThisTime, uiOffset )))
                {
                    UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, UTV_UNKNOWN, __LINE__, rStatus );
                    break;
                }
            }

            uiTotalLeft -= uiReadThisTime;
            uiOffset += uiReadThisTime;

            /* check for an amount that is not an integral of UTV_SHA_MESSAGE_BLOCK_SIZE bytes */
            if (uiReadThisTime % UTV_SHA_MESSAGE_BLOCK_SIZE)
                uiAmountToProcess = uiReadThisTime + (UTV_SHA_MESSAGE_BLOCK_SIZE - (uiReadThisTime % UTV_SHA_MESSAGE_BLOCK_SIZE));
            else
                uiAmountToProcess = uiReadThisTime;

            /* pad it out if necessary */
            UtvMemset( &pBuff[ uiReadThisTime ], 0x55, uiAmountToProcess - uiReadThisTime );

            /* abort check before hash calc */
            if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
            {
                rStatus = UTV_ABORT;
                break;
            }

            /* continue to process the hash for this section of the file */
            UtvGenerateSHA256Process( pBuff, uiAmountToProcess );
        }

        /* close the file/partition */
        if ( 0 == ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION ) )
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallFileClose( uiHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileClose(%s): %s", pszCompName, UtvGetMessageString( rStatus ) );
                break;
            }
        } else
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionClose( uiHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionClose(%s): %s", pszCompName, UtvGetMessageString( rStatus ) );
                break;
            }
        }

        /* if inner loop punted, punt here too */
        if ( UTV_OK != rStatus )
            break;

        /* get the hash from the processed data */
        UtvGenerateSHA256Close( ubMessageDigest );

        /* convert hash */
        cMessageDigestString[ 0 ] = 0;
        for ( n = 0; n < UTV_SHA_MESSAGE_DIGEST_LENGTH; n++ )
        {
            UtvMemFormat( (UTV_BYTE *) cNum, 3, "%02X", ubMessageDigest[ n ] );
            UtvStrcat( (UTV_BYTE *) cMessageDigestString, (UTV_BYTE *) cNum );
        }

        /* compare to the previously calculated value */
        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszTextDef, (UTV_BYTE *) cMessageDigestString, UTV_SHA_MESSAGE_DIGEST_LENGTH * 2 ))
        {
            rStatus = UTV_PART_VALIDATE_FAILS;
            UtvMessage( UTV_LEVEL_ERROR, "Component \"%s\" fails validation", pszCompName );
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        UtvMessage( UTV_LEVEL_INFO, "Component \"%s\" validated", pszCompName );
    }

    /* free the hash buffer */
    UtvFree( pBuff );

    return rStatus;
}

UTV_RESULT UtvImageCheckFileLocks( UTV_UINT32 hImage )
{
    UTV_RESULT              rStatus = UTV_OK;
    UTV_UINT32              n, i, uiFileHandle, uiFileLen, uiBufferLen, uiBytesRead;
    UTV_INT8               *pszTextID, *pszTextDef, cMessageDigestString[256], cNum[3];
    UTV_BYTE                ubMessageDigest[UTV_SHA_MESSAGE_BLOCK_SIZE], *pBuff;

    /* if there are any lock hashes check them */
    for ( uiFileHandle = 0, i = 0; UTV_TRUE; i++ )
    {
        UTV_INT8 cMatch[ 32 ];
        UtvMemFormat( (UTV_BYTE *) cMatch, 32, "_lockf%d", i );
        if ( UTV_OK != UtvPublicImageGetUpdateTextDefPartial( hImage, cMatch, &pszTextID, &pszTextDef ))
        {
            rStatus = UTV_OK;
            break; /* can't find one?  we're done. */
        }

        /* spin past lock directive */
        pszTextID++;
        while ( '_' != *pszTextID )
            pszTextID++;
        pszTextID++;

        uiFileHandle = 0;
        /* attempt to access the file */
        if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszTextID, "rb", &uiFileHandle )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        /* seek to the end */
        if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiFileHandle, 0, UTV_SEEK_END )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        /* get the size */
        if ( UTV_OK != ( rStatus = UtvPlatformFileTell( uiFileHandle, &uiFileLen )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        /* seek to the start */
        if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiFileHandle, 0, UTV_SEEK_SET )))
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        if (uiFileLen % UTV_SHA_MESSAGE_BLOCK_SIZE)
            uiBufferLen = uiFileLen + (UTV_SHA_MESSAGE_BLOCK_SIZE - (uiFileLen % UTV_SHA_MESSAGE_BLOCK_SIZE));
        else
            uiBufferLen = uiFileLen;

        /* allocate memory for the entire file */
        if ( NULL == (pBuff = UtvMalloc( uiBufferLen )))
        {
            UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, UTV_NO_MEMORY, __LINE__, uiBufferLen );
            break;
        }

        /* attempt to read in the entire file */
        rStatus = UtvPlatformFileReadBinary( uiFileHandle, pBuff, uiFileLen, &uiBytesRead );
        if ( rStatus != UTV_OK )
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            UtvFree( pBuff );
            break;
        }

        UtvMemset( &pBuff[ uiFileLen ], 0x55, uiBufferLen - uiFileLen );

        /* get the hash for this file */
        UtvGenerateSHA256( pBuff, uiBufferLen, ubMessageDigest );

        /* free the buffer */
        UtvFree( pBuff );

        /* convert hash */
        cMessageDigestString[ 0 ] = 0;
        for ( n = 0; n < UTV_SHA_MESSAGE_DIGEST_LENGTH; n++ )
        {
            UtvMemFormat( (UTV_BYTE *) cNum, 3, "%02X", ubMessageDigest[ n ] );
            UtvStrcat( (UTV_BYTE *) cMessageDigestString, (UTV_BYTE *) cNum );
        }

        /* compare to the previously calculated value */
        if ( 0 != UtvMemcmp( (UTV_BYTE *) pszTextDef, (UTV_BYTE *) cMessageDigestString, UTV_SHA_MESSAGE_DIGEST_LENGTH * 2 ))
        {
            rStatus = UTV_FILE_LOCK_FAILS;
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__, pszTextID );
            break;
        }

        UtvMessage( UTV_LEVEL_INFO, "\"%s\" OK", pszTextID );
    }

    if( uiFileHandle != 0 )
    {
        UtvPlatformFileClose( uiFileHandle );
    }

    return rStatus;
}

static UTV_RESULT UtvImageProcessCommands( UTV_UINT32 uiCommandFmtString, UTV_UINT32 uiStoreIndex )
{
    UTV_RESULT              rStatus = UTV_OK;
    UTV_UINT32              iCmd;
    UTV_UINT32              hStore, hImage;
    UtvImageAccessInterface utvIAI;

    /* Open the specified store */
    if ( UTV_OK != ( rStatus = UtvPlatformStoreOpen( uiStoreIndex, UTV_PLATFORM_UPDATE_STORE_FLAG_READ,
                                                     UTV_MAX_COMP_DIR_SIZE, &hStore, NULL, NULL )))
    {
        /* tolerate already open */
        if ( UTV_ALREADY_OPEN != rStatus )
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
            return rStatus;
        }
    }

    /* get the file version of the image access interface */
    if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
        return rStatus;
    }

    /* attempt to open this image which will access its component directory */
    if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)hStore, &utvIAI, &hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
        return rStatus;
    }

    /* look for commands */
    for ( iCmd = 0; UTV_TRUE; iCmd++ )
    {
        UTV_INT8 cCmd[ 64 ], *pszCmdString;

        /* check for command string */
        UtvMemFormat( (UTV_BYTE *) cCmd, sizeof(cCmd), UtvStringsLookup( uiCommandFmtString ), iCmd );

#ifdef UTV_CMD_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "LOOKING FOR COMMAND STRING: %s", cCmd );
#endif

        if ( UTV_OK != ( rStatus = UtvPublicImageGetUpdateTextDef( hImage, cCmd, &pszCmdString )))
        {
            UtvMessage( UTV_LEVEL_INFO, "DONE PROCESSING COMMANDS" );
            rStatus = UTV_OK;
            break;
        }

#ifdef UTV_CMD_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "EXECUTING CMD %u: %s", iCmd, pszCmdString );
#endif

        /* send the command to the platform specific layer for processing */
        UtvPlatformCommandInterface( pszCmdString );
    }

    UtvImageClose( hImage );
    return rStatus;
}

static UTV_RESULT UtvImageCheckSerialNumberTargeting( UTV_UINT32 uiStoreIndex )
{
    UTV_RESULT              rStatus = UTV_OK;
    UTV_UINT32              i;
    UTV_UINT32              hStore, hImage;
    UtvImageAccessInterface utvIAI;
    UTV_INT8                 cESN[ 64 ], *pszTarget;
    UTV_BOOL                bFound = UTV_FALSE;

    /* Open the specified store */
    if ( UTV_OK != ( rStatus = UtvPlatformStoreOpen( uiStoreIndex, UTV_PLATFORM_UPDATE_STORE_FLAG_READ,
                                                     UTV_MAX_COMP_DIR_SIZE, &hStore, NULL, NULL )))
    {
        /* tolerate already open */
        if ( UTV_ALREADY_OPEN != rStatus )
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
            return rStatus;
        }
    }

    /* get the file version of the image access interface */
    if ( UTV_OK != ( rStatus = UtvFileImageAccessInterfaceGet( &utvIAI )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
        return rStatus;
    }

    /* attempt to open this image which will access its component directory */
    if ( UTV_OK != ( rStatus = UtvImageOpen( NULL, (UTV_INT8 *)hStore, &utvIAI, &hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
        return rStatus;
    }

    /* get the targeting strings */
    if ( UTV_OK != ( rStatus = UtvPublicImageGetUpdateTextDef( hImage, UtvStringsLookup( UTV_CMD_TARGET ), &pszTarget )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPublicImageGetCompTextDef() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != ( rStatus = UtvCEMGetSerialNumber( cESN, sizeof( cESN ) )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvCEMGetSerialNumber() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

#ifdef UTV_CMD_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "TARGETING: checking target \"%s\" against ESN \"%s\"", pszTarget, cESN );
#endif

    /* match logic */
    for ( i = 0; i < UtvStrlen( (UTV_BYTE *) pszTarget); i++ )
    {
        /* if '*' encountered we're done */
        if ( '*' == pszTarget[ i ] )
        {
            bFound = UTV_TRUE;
            break;
        }

        /* if '?' encountered accept current position */
        if ( '?' == pszTarget[ i ] )
            continue;

        /* otherwise it needs to be a match */
        if ( cESN[ i ] != pszTarget[ i ] )
        {
            break;
        }
    }

    /* if found or we're out of string then it's a match */
    if ( bFound || i >= UtvStrlen( (UTV_BYTE *) pszTarget) )
        rStatus = UTV_OK;
    else
        rStatus = UTV_SNUM_TARGETING_FAILS;

    return rStatus;
}

UTV_RESULT UtvImageProcessUSBCommands( void )
{
    UTV_RESULT    rStatus;

    /* check serial number targeting */
    if ( UTV_OK != ( rStatus = UtvImageCheckSerialNumberTargeting( UTV_PLATFORM_UPDATE_USB_INDEX )))
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SECURITY, rStatus, __LINE__ );
        return rStatus;
    }

    return UtvImageProcessCommands( UTV_USB_CMD, UTV_PLATFORM_UPDATE_USB_INDEX );
}

UTV_RESULT UtvImageProcessPreBootCommands( void )
{
    return UtvImageProcessCommands( UTV_PRE_BOOT_CMD, UTV_PLATFORM_UPDATE_MANIFEST_INDEX );
}

UTV_RESULT UtvImageProcessPostBootCommands( void )
{
    return UtvImageProcessCommands( UTV_POST_BOOT_CMD, UTV_PLATFORM_UPDATE_MANIFEST_INDEX );
}

UTV_RESULT UtvImageProcessPostEveryBootCommands( void )
{
    return UtvImageProcessCommands( UTV_POST_EVERY_BOOT_CMD, UTV_PLATFORM_UPDATE_MANIFEST_INDEX );
}

#ifdef UTV_DEBUG
/* UtvImageTest()
    Test that a given image is properly formatted and optionally
    that all of it's components can be decrypted.
*/
UTV_RESULT UtvImageTest( UTV_INT8 *pszLocation, UTV_INT8 *pszFileName, UtvImageAccessInterface *pIAI, UTV_BOOL bDecrypt )
{
    UTV_RESULT                rStatus;
    UTV_UINT32                hImage;
    UTV_UINT32                uiTotalEncryptedLength = 0, uiTotalCEMDataLength = 0, uiTotalModCount = 0;
    UTV_UINT32                i, n;
    UtvCompDirHdr            *pCompDirHdr;
    UtvCompDirCompDesc       *pCompDesc;
    UtvRange                 *pRange;
    UtvDependency            *pSWDeps;
    UtvTextDef               *pTextDef;
    UtvCompDirModDesc        *pModDesc;
    UTV_BYTE                 *pBuffer;
    UtvModSigHdr             *pModSigHdr;
    UTV_UINT32                uiLengthOut;

    if ( UTV_OK != ( rStatus = UtvImageOpen( pszLocation, pszFileName, pIAI, &hImage, UTV_PLATFORM_UPDATE_STORE_INVALID )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageOpen() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* display the component directory */
    if ( UTV_OK != ( rStatus = UtvDisplayComponentDirectory( hImage )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvDisplayComponentDirectory() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    if ( UTV_OK != ( rStatus = UtvImageGetCompDirHdr( hImage, &pCompDirHdr, &pRange, &pRange, &pSWDeps, &pTextDef )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDirHdr() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "    === Component Module Walk === " );

    /* spin through each module of each component, decrypting as we go and checking the resulting module descriptors */
    for ( i = 0; i < Utv_16letoh( pCompDirHdr->usNumComponents ); i++ )
    {
        /* Get the component descriptor for this component */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByIndex( hImage, i, &pCompDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescByIndex() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        /* get the module descriptor for this component */
        if ( UTV_OK != ( rStatus = UtvImageGetCompDescInfo( pCompDesc, &pRange, &pRange, &pSWDeps, &pTextDef, &pModDesc )))
        {
            UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescInfo() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
            return rStatus;
        }

        uiTotalModCount += Utv_16letoh( pCompDesc->usModuleCount );

        for ( n = 0; n < Utv_16letoh( pCompDesc->usModuleCount ); n++, pModDesc++ )
        {
            UtvMessage( UTV_LEVEL_INFO, "            Module %d of %d, size: %lu, encrypted size: %lu, offset: %lu",
                        n + 1, Utv_16letoh( pCompDesc->usModuleCount ),
                        Utv_32letoh( pModDesc->uiModuleSize ), 
                        Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                        Utv_32letoh( pModDesc->uiModuleOffset ) );

            uiTotalEncryptedLength += Utv_32letoh( pModDesc->uiEncryptedModuleSize );

            /* allocate memory for the entire encrypted module */
            if ( NULL == ( pBuffer = UtvPlatformInstallAllocateModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ) )))
            {
                rStatus = UTV_NO_MEMORY;
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallAllocateModuleBuffer( %d ) fails: %s", 
                            Utv_32letoh( pModDesc->uiEncryptedModuleSize ), UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            /* read it in */
            if ( UTV_OK != ( rStatus = UtvImageAccessArray[ hImage ].utvIAI.pfRead( UtvImageAccessArray[ hImage ].hRemoteStore, 
                                                                                    Utv_32letoh( pModDesc->uiModuleOffset ),
                                                                                    Utv_32letoh( pModDesc->uiEncryptedModuleSize ),
                                                                                    pBuffer )))
            {
                UtvMessage( UTV_LEVEL_ERROR, "UtvImageAccessArray[ hImage ].utvIAI.pfRead() \"%s\"", UtvGetMessageString( rStatus ) );
                UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                return rStatus;
            }

            /* decrypt the module if requested and display its module signaling header */
            if ( bDecrypt )
            {
                /* If the component has the same OUI and Model Group as the platform then try to decrypt it using the
                   component directory key and the message digest stored in the module description. */
                if ( Utv_32letoh( pCompDirHdr->uiPlatformOUI) == Utv_32letoh( pCompDesc->uiOUI ) && 
                     Utv_16letoh( pCompDirHdr->usPlatformModelGroup ) == Utv_16letoh( pCompDesc->usModelGroup ) )
                {
                    /* decrypt it using the component directory's 3DES keys after fixing up the local scratch key header with the
                       module size, encrypted size, and message digest for this module */
                    if ( UTV_OK != ( rStatus = UtvFixupKeyHeader( &UtvImageAccessArray[ hImage ].utvKeyHeader, 
                                                                  Utv_32letoh( pModDesc->uiModuleSize ),
                                                                  Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                                  pModDesc->ubMessageDigest )))
                    {
                        UtvMessage( UTV_LEVEL_ERROR, " UtvFixupKeyHeader( ) fails: %s", UtvGetMessageString( rStatus ) );
                        UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                        return rStatus;
                    }

                    /* decrypt using the comp dir key */
                    if ( UTV_OK != ( rStatus = UtvDecryptModule( pBuffer, 
                                                                 Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                                 &uiLengthOut,
                                                                 Utv_32letoh( pCompDirHdr->uiPlatformOUI ), 
                                                                 Utv_16letoh( pCompDirHdr->usPlatformModelGroup ),
                                                                 &UtvImageAccessArray[ hImage ].utvKeyHeader, NULL )))
                    {
                        UtvMessage( UTV_LEVEL_ERROR, " UtvDecryptModule( comp dir header ) fails: %s", UtvGetMessageString( rStatus ) );
                        UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                        return rStatus;
                    }

                    UtvMessage( UTV_LEVEL_INFO, "            Fast decrypt OK" );

                } else
                {
                    /* the OUI or MG is different than the platform OUI/MG.  Attempt a decrypt using the module's key header */
                    if ( UTV_OK != ( rStatus = UtvDecryptModule( pBuffer, 
                                                                 Utv_32letoh( pModDesc->uiEncryptedModuleSize ), 
                                                                 &uiLengthOut,
                                                                 Utv_32letoh( pCompDesc->uiOUI ), 
                                                                 Utv_16letoh( pCompDesc->usModelGroup ), NULL, NULL )))
                    {
                        UtvMessage( UTV_LEVEL_ERROR, " UtvDecryptModule( native header ) fails: %s", UtvGetMessageString( rStatus ) );
                        UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );
                        return rStatus;
                    }

                    UtvMessage( UTV_LEVEL_INFO, "            Native decrypt OK" );
                }

                /* check out the module signaling header that sits at the top of the module */
                pModSigHdr = (UtvModSigHdr *) pBuffer;

                /* Check signature */
                if ( UtvMemcmp( pModSigHdr->ubModSigHdrSignature, (void *) UTV_MOD_SIG_HDR_SIGNATURE, UTV_SIGNATURE_SIZE ) )
                {
                    rStatus = UTV_BAD_MOD_SIG;
                    UtvMessage( UTV_LEVEL_ERROR, " UtvFileTest fails: %s", UtvGetMessageString( rStatus ) );
                    UtvFree( pBuffer );
                    return rStatus;
                }

                /* Check structure revision */
                if ( UTV_MOD_SIG_HDR_VERSION != pModSigHdr->ubStructureVersion )
                {
                    rStatus = UTV_BAD_MOD_VER;
                    UtvMessage( UTV_LEVEL_ERROR, " UtvFileTest fails: %s", UtvGetMessageString( rStatus ) );
                    UtvFree( pBuffer );
                    return rStatus;
                }

                /* display mod hdr info */
                UtvMessage( UTV_LEVEL_INFO, "            Module Header Info:  Component name: %s, MV: 0x%X, Index: %d, Count: %d",
                            UTV_CONVERT_TEXT_OFFSET_TO_PTR( pModSigHdr, pModSigHdr->toComponentName), pModSigHdr->usModuleVersion, pModSigHdr->usModuleIndex,
                            pModSigHdr->usModuleCount );
                UtvMessage( UTV_LEVEL_INFO, "                                 Header size: %d, CEM module size: %d, CEM module offset: %d",
                            pModSigHdr->usHeaderSize, pModSigHdr->uiCEMModuleSize, pModSigHdr->uiCEMModuleOffset );

                uiTotalCEMDataLength += pModSigHdr->uiCEMModuleSize;
            } /* if bDecrypt */

            /* Free the buffer that we allocated for this module */
            UtvPlatformInstallFreeModuleBuffer( Utv_32letoh( pModDesc->uiEncryptedModuleSize ), pBuffer );

        } /* for each module in each component */

    } /* for each component */

    /* close the image */
    if ( UTV_OK != ( rStatus = UtvImageClose( hImage )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvImageClose() fails: %s", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvMessage( UTV_LEVEL_INFO, "" );

    if ( bDecrypt )
        UtvMessage( UTV_LEVEL_INFO, "Summary stats:  Total Modules: %d.  Total Encrypted Size: %d bytes.  Total CEM Data Size: %d, Overhead: %d",
                    uiTotalModCount, uiTotalEncryptedLength, uiTotalCEMDataLength, uiTotalEncryptedLength - uiTotalCEMDataLength  );
    else
        UtvMessage( UTV_LEVEL_INFO, "Summary stats:  Total Modules: %d.  Total Encrypted Size: %d bytes.",
                    uiTotalModCount, uiTotalEncryptedLength );
    return UTV_OK;
}
#endif
