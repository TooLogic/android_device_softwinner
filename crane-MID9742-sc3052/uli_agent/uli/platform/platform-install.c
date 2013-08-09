/* platform-install.c : UpdateTV "File" Implemetation of the Module Install Functions

   Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformInstallCleanup( UTV_UINT32 hImage );
   UTV_RESULT UtvPlatformInstallModuleData( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc,
                                            UtvModSigHdr *pModSigHdr, UTV_BYTE *pModuleBuffer );
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Jim Muchow      11/02/2011  Download Resume Support
   Bob Taylor      02/08/2011  Fixed UtvPlatformInstallCleanup().
   Jim Muchow      09/21/2010  BE support.
   Bob Taylor      05/10/2010  Added UtvPlatformInstallInit().
   Bob Taylor      02/08/2010  Adapted from platform-install-file.c
   Bob Taylor      02/03/2010  Added UtvPlatformInstallCleanup() to close files and partitions
                               when the Agent needs to abort download for any reason.
*/

#include "utv.h"

static UtvCompDirCompDesc *s_pCurrentCompDesc = NULL;
static UTV_UINT32 s_uiModulesWritten = 0;
static UTV_UINT32 s_uiModulesToWrite = 0;
static UTV_UINT32 s_hPartHandle = UTV_CORE_IMAGE_INVALID;
static UTV_UINT32 s_hFileHandle = UTV_CORE_IMAGE_INVALID;

/* UtvPlatformInstallInit
    This routine is called by UtvPublicAgentInit() to make sure that the tracking variables are initialized
    on subsequent invocations of the agent when the program that calls it is not re-loaded between instances.
*/
void UtvPlatformInstallInit( void )
{
    s_pCurrentCompDesc = NULL;
    s_uiModulesWritten = 0;
    s_uiModulesToWrite = 0;
    s_hPartHandle = UTV_CORE_IMAGE_INVALID;
    s_hFileHandle = UTV_CORE_IMAGE_INVALID;
}

/* UtvPlatformInstallCleanup
    This routine is called when a download is done for any reason to make sure that any
    open partitions/files are closed.
*/
UTV_RESULT UtvPlatformInstallCleanup( void )
{
    UTV_RESULT  rStatus = UTV_OK;

    /* if this isn't the current component or the current component is closed there's
       nothing to do here. 
    */
    if ( s_pCurrentCompDesc == NULL )
    {
        UtvMessage( UTV_LEVEL_INFO, "UtvPlatformInstallCleanup(): nothing to do." );
        return rStatus;
    }
    
    /* if this was a file then close it */
    if ( UTV_CORE_IMAGE_INVALID != s_hFileHandle )
    {
        UtvMessage( UTV_LEVEL_INFO, "UtvPlatformInstallCleanup(): closing currently open file." );
        UtvPlatformFileClose( s_hFileHandle );
        s_hFileHandle = UTV_CORE_IMAGE_INVALID;
    } 

    /* if this was a partition then close it */
    if ( UTV_CORE_IMAGE_INVALID != s_hPartHandle )
    {
        UtvMessage( UTV_LEVEL_INFO, "UtvPlatformInstallCleanup(): closing currently open partition." );
        /* it's a partition. */
        if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionClose( s_hPartHandle ) ))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionClose(): \"%s\"", UtvGetMessageString( rStatus ) );
        }
        s_hPartHandle = UTV_CORE_IMAGE_INVALID;
    } 

    s_pCurrentCompDesc = NULL;

    return rStatus;
}

/* UtvPlatformInstallModuleData
    This routine is called after the module has been decrypted and needs to be installed.
    The UtvPlatformInstallModuleData routine is responsible for the disposition of downloaded modules.
    Each manufacturer may implement additional module authentication steps in this module.
    If the module data is the TV Application or system image, the call may not return (TV rebooted).

    PLEASE NOTE:  This function is the only function that will be called where the memory
                  that was allocated in UtvPlatformInstallAllocateModuleBuffer() and filled by the core
                  is intact.  Outside of this function that memory will have been freed
                  and is therefore not available to access.
*/
UTV_RESULT UtvPlatformInstallModuleData( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc,
                                         UtvModSigHdr *pModSigHdr, UTV_BYTE *pModuleBuffer )
{
    UTV_RESULT           rStatus = UTV_OK;
    UTV_UINT32           uiPreviouslyWritten;
    UTV_INT8               *pszCompName, *pszCurrentCompName;
    UTV_BOOL            appendingToAlreadyReceivedData;

    /* The variable that controls whether data will be appended to that */
    /* already received from a previous power-cycle download            */
    appendingToAlreadyReceivedData = UTV_FALSE;

    /* See if an abort happened, and return now if it is set */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    /* get this component's name for debug display */
    if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), 
                              &pszCompName )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetText(): \"%s\"", UtvGetMessageString( rStatus ) );
        pszCompName = "";
    }

    /* check to see if we've started writing this component yet */
    if ( s_pCurrentCompDesc != pCompDesc )
    {
        /* are we in the middle of a write? */
        if ( NULL != s_pCurrentCompDesc )
        {
            if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
                                  Utv_16letoh( s_pCurrentCompDesc->toName ),
                                  &pszCurrentCompName )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPublicImageGetText(): \"%s\"", UtvGetMessageString( rStatus ) );
                pszCurrentCompName = "";
            }

            UtvMessage( UTV_LEVEL_WARN, "Abandoning write of component %s after %d out of %d modules written!",
                        pszCurrentCompName, s_uiModulesWritten, s_uiModulesToWrite );

            /* if this was a file then close it */
            if ( UTV_CORE_IMAGE_INVALID != s_hFileHandle )
            {
                if ( UTV_OK != ( rStatus = UtvPlatformInstallFileClose( s_hFileHandle )))
                {
                    UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileClose(): \"%s\"", UtvGetMessageString( rStatus ) );
                    return rStatus;
                }
                s_hFileHandle = UTV_CORE_IMAGE_INVALID;
            } else
            {
                /* it's a partition. */
                if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionClose( s_hPartHandle ) ))
                {
                    UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionClose(): \"%s\"", UtvGetMessageString( rStatus ) );
                }
                s_hPartHandle = UTV_CORE_IMAGE_INVALID;
            }

            s_uiModulesWritten = 0;
            s_uiModulesToWrite = Utv_16letoh( pCompDesc->usModuleCount );

        } /* abandoned write of another component closed */
        else
        {
            /* are we returning to a previously abandoned write? */
            (void)UtvCoreCommonCompatibilityCountMods( hImage, pCompDesc, &uiPreviouslyWritten );

            if ( 0 != uiPreviouslyWritten )
            {
                s_uiModulesWritten = uiPreviouslyWritten;

                /* check to make sure nothing fishy is going on */
                if ( uiPreviouslyWritten >= Utv_16letoh( pCompDesc->usModuleCount ) )
                    UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallModuleData(): uiPreviouslyWritten: %d, pCompDesc->usModuleCount: %d", uiPreviouslyWritten, Utv_16letoh( pCompDesc->usModuleCount ) );

                s_uiModulesToWrite = Utv_16letoh( pCompDesc->usModuleCount );
                appendingToAlreadyReceivedData = UTV_TRUE;
                UtvMessage( UTV_LEVEL_INFO, "Returning to previously abandoned write of component %s.  %d modules already written. Looking for %d more.",
                            pszCompName, s_uiModulesWritten, s_uiModulesToWrite - s_uiModulesWritten );
            } else
            {
                s_uiModulesWritten = 0;
                s_uiModulesToWrite = Utv_16letoh( pCompDesc->usModuleCount );
                UtvMessage( UTV_LEVEL_INFO, "Encountering first component %s.  %d modules to write.",
                            pszCompName, s_uiModulesToWrite );                
            }
        }
   
        /* open it depending on whether it's a file or a partition */
        if ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION )
        {
            /* In this example code there isn't anything we can do because partition writing is
               platform-specific.  Just issue the placeholder command UtvPlatformInstallPartitionOpen() */
            if (appendingToAlreadyReceivedData == UTV_TRUE)
                rStatus = UtvPlatformInstallPartitionOpenAppend( hImage, pCompDesc, pModSigHdr, &s_hPartHandle );
            else
                rStatus = UtvPlatformInstallPartitionOpenWrite( hImage, pCompDesc, pModSigHdr, &s_hPartHandle );
            if ( UTV_OK != rStatus )  
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionOpenWrite(): \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }
        } else
        {
            if (appendingToAlreadyReceivedData == UTV_TRUE)
                rStatus = UtvPlatformInstallFileOpenAppend( hImage, pCompDesc, pModSigHdr, &s_hFileHandle );
            else
                rStatus = UtvPlatformInstallFileOpenWrite( hImage, pCompDesc, pModSigHdr, &s_hFileHandle );
            if ( UTV_OK != rStatus )
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileOpenWrite(): \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }
        } /* file or partition are ready to go */

        /* set component descriptor and module count tracker */
        s_pCurrentCompDesc = pCompDesc;
    }
    
    /* ready for write, perform it depending on whether this is a partition or file */
    if ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION )
    {
        /* In this example code there isn't anything we can do because partition writing is
           platform-specific.  Just issue the placeholder command UtvPlatformInstallPartitionWrite() */
        if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionWrite( s_hPartHandle, pModuleBuffer, 
                                     Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                                     Utv_32letoh( pModSigHdr->uiCEMModuleOffset ) )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionWrite( %d, %d ): \"%s\"",
                        Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                        Utv_32letoh( pModSigHdr->uiCEMModuleOffset ),
                        UtvGetMessageString( rStatus ) );
            return rStatus;
        }
    } else
    {
        if ( UTV_OK != ( rStatus = UtvPlatformInstallFileWrite( s_hFileHandle, pModuleBuffer, 
                                Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                                Utv_32letoh( pModSigHdr->uiCEMModuleOffset ) )))
        {
            UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileWrite( %d, %d ): \"%s\"",
                        Utv_32letoh( pModSigHdr->uiCEMModuleSize ), 
                        Utv_32letoh( pModSigHdr->uiCEMModuleOffset ),
                        UtvGetMessageString( rStatus ) );
            return rStatus;
        }
    }

    /* check the module accounting to see if we're done. */
    if ( ++s_uiModulesWritten == s_uiModulesToWrite )
    {
        UtvMessage( UTV_LEVEL_INFO, " All %d modules written for component \"%s\".  Closing.", s_uiModulesToWrite, pszCompName );
        if ( Utv_32letoh( pCompDesc->uiComponentAttributes ) & UTV_COMPONENT_ATTR_PARTITION )
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallPartitionClose( s_hPartHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallPartitionClose(): \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            s_hPartHandle = UTV_CORE_IMAGE_INVALID;
        }
        else
        {
            if ( UTV_OK != ( rStatus = UtvPlatformInstallFileClose( s_hFileHandle )))
            {
                UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInstallFileClose(): \"%s\"", UtvGetMessageString( rStatus ) );
                return rStatus;
            }

            s_hFileHandle = UTV_CORE_IMAGE_INVALID;
        }

        /* setting s_pCurrentCompDesc to NULL indicates we're done */
        s_pCurrentCompDesc = NULL;
        rStatus = UTV_COMP_INSTALL_COMPLETE;
    }

    UtvMessage( UTV_LEVEL_INFO, " [DBG]%d/%d modules written for component \"%s\" (%s)", s_uiModulesWritten, s_uiModulesToWrite, pszCompName, UtvGetMessageString( rStatus ) );

    return rStatus;
}
