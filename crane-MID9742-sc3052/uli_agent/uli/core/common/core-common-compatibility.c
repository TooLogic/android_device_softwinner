/* core-common-compatibility.c : UpdateTV Compatibility Functions

   Copyright (c) 2007-2010 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
    UTV_RESULT UtvCoreCommonCompatibilityUpdate(    UTV_UINT32 hImage, UTV_UINT32 uiOui, 
                                                    UTV_UINT16 usModelGroup, UTV_UINT16 usModuleVersion,
                                                    UTV_UINT32 uiAttributes,UTV_UINT32 uiNumHMWEntries,
                                                    UtvRange  *pHWMRange, UTV_UINT32 uiNumSWVEntries,
                                                    UtvRange  *pSWVRange, UTV_UINT32 uiNumSWDeps,
                                                    UtvDependency *pSWDeps );
    UTV_RESULT UtvCoreCommonCompatibilityComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc );
    UTV_RESULT UtvCoreCommonCompatibilityDownload(  UTV_UINT32 iOui, UTV_UINT32 iModelGroupId );
    UTV_RESULT UtvCoreCommonCompatibilityModule(    UTV_UINT32 iOui, UTV_UINT32 iModelGroupId, UTV_UINT32 iModulePriority,
                                                    UTV_UINT32 iModuleNameLength, UTV_BYTE * pModuleName,
                                                    UTV_UINT32 iModuleVersion, UTV_UINT32 iHardwareModelBegin,
                                                    UTV_UINT32 iHardwareModelEnd,  UTV_UINT32 iSoftwareVersionBegin,
                                                    UTV_UINT32 iSoftwareVersionEnd, UTV_UINT32 iModuleSize );
   ------------------------------------------------------------------
 
   Revision History (newest edits added to the top)

   Who             Date        Edit
   Jim Muchow      11/18/2011  Removed Broadcast support.
   Jim Muchow      11/02/2011  Download Resume support.
   Jim Muchow      09/21/2010  BE support.
   Bob Taylor      06/23/2010  Made command mode check hw model sensitive.
   Bob Taylor      06/22/2010  Factory mode test coherenet with field mode test.  Loop test mode support.
   Bob Taylor      04/15/2010  Command sticks are now model group sensitive. 
                               Factory and Field directives are now completely analgous to normal update 
                               compatiblity checking.
                               Factory directive is now software version sensitive.
   Bob Taylor      01/11/2010  Fixed OOB already received bug.  Added persist write after tracker clear.
   Bob Taylor      06/30/2009  Module tracking moved to persistent memory.
   Bob Taylor      03/05/2009  Created from mod-org-xxxx for version 2.0.

   ------------------------------------------------------------------

   Compatibility checking moves through a series of steps.  The details of these steps are 
   dependent on the delivery mode, but the conceptual framework for each of them is the same.

   STEP 1:  -*- UPDATE QUALIFICATION SCAN -*-
            (same as version 1.9 except with the addition of internet and USB)
            Determine whether an update (a collection of components) is compatible
            with this platform.  Nothing is downloaded to do this.  It is done entriely with
            signaling.  The Compatibility parameters at this level are:

                Platform OUI
                Platform Model Group
                Platform Module Version
                Platform Attributes (field test is currently the only attribute at this level)
                System Software Compatibility
                Hardware Model Compatibility

            The result of this step is either:
            
                A)  The update is compatible and its component directory will be downloaded.
                B)  The update is incompatible and it is ignored.

            The Broadcast delivery mode parses DSI and DIIs to collect the pertinent Compatibility information.
            The Internet delivery mode gets this information via an UpdateQuery command.
            The File delivery mode assumes that any file that has the name OUI-ModelGroup.utv in a specified 
            directory is pre-qualified.  There is no signaling in the File case and package qualification is
            presumed.

            Please note that note that no progress information is available during this step except via the
            system sub-state variable.  This step is initiated either automatically via UtvApplication() or programatically
            via UtvPublicGetDownloadSchedule().  This step either returns UTV_NO_DOWNLOAD or transitions to step 2 without 
            notification.

   STEP 2:  -*- COMPONENT QUALIFICATION SCAN -*-
            (new to version 2.0) 
            In this stage a component directory is acquired and each of the Compatibility entries for each of the
            components in it are checked.  The Compatibility parameters at this level are:

                Component OUI
                Component Model Group
                Component Name
                Component Attributes (reserved for future expansion, none used now during this phase)
                Software Version Dependency
                Hardware Model Compatibility
                Distribution Percentage (broadcast only - check serial number against distribution hash for phased rollout)

            Note that module version does not play a role at this level.  It has already been established
            in step 1.  Note that at this level components may have dependency on each other in addition to
            themselves.  New components can be added at this level as well if the system has been given permission
            to do so.  The component directory may contain deletion notifications which the Agent will act upon if
            if the system has been given permission to do so.

   STEP 3:  -*- DOWNLOAD DECISION -*-
            (same as version 1.9, but with the addition of components and their attributes)
            The results of the above steps depends on the following.  What's different from version 1.9 is that an upload
            package contains multiple components and they don't all have to be accepted for download.  Additionally components
            carry attributes that inform the download decision.  By defaults all components are "REQUIRED" (no user option to decline) 
            and "HIDDEN" (not displayed in a query list whether they are required or not) unless they are qualified by the following:

                Publisher component command:  "-optional"
                Component attribute:          UTV_COMPONENT_ATTR_OPTIONAL 
                                              The download of the component is optional and appears in a query list that is displayed
                                              to the user who is allowed to decline or defer the download

                Publisher component command:  "-reveal"
                Component attribute:          UTV_COMPONENT_ATTR_REVEAL
                                              The component is required and the user won't have a choice whether to download it or not
                                              but for whatever reason the TV user interface would like to display what is about to be
                                              downloaded.

                It should be noted that component attributes only come into play when a scan is initiated via 
                UtvPublicGetDownloadSchedule():

                A.  If the scan was initiated via UtvApplication() automatically then the UtvDownloadOrder will be created
                    automatically and download will be initiated.  Component attributes have no effect in this case.  Everything
                    that is compatible will be downloaded.
                B.  If the scan was initiated by UtvPublicGetDownloadSchedule() then the compatible components are returned via 
                    call back or directly (depending on the UtvPublicGetDownloadSchedule() call mode) in a UTV_PUBLIC_SCHEDULE structure.
                    The supervising program then sets the download flags in the schedule either automatically or via user
                    interaction and calls UtvDownloadPackage() which will fill out a UtvDownloadOrder structure and execute the
                    download.  PLEASE NOTE:  The Agent NEVER interprets component attributes in the "user query" class (the ones
                    shown above).  It simply conveys them to the supervising program to make a decision.


   STEP 4:  -*- COMPONENT DOWNLOAD -*-
            (components are new to version 2.0, by default the Agent downloads all components making it behave like version 1.9)
            All components in the UtvComponentDownloadOrder are downloaded.  Depending on their attributes one of a few different 
            things can happen during download:

                Publisher component command:  "-store"
                Component attribute:          UTV_COMPONENT_ATTR_STORE
                                              The component is downloaded, but not decrypted or installed.  
                                              A stored component can be accessed later by a call to UtvGetStoredComponent() and
                                              may be installed later or deleted.  By default the Agent does not
                                              store components.  By default the Agent installs them, meaning they are handed off to
                                              the supervising program and are not kept track of in any way except to notify the 
                                              NOC (when a aware of itsoptional and appears in a query list that is displayed
                                              to the user who is allowed to decline or defer the download

            When a download is complete and an internet connection is available the call DownloadEnded is made telling the NOC
            about the success or failure of the download.  A call to UtvPlatformDownloadComplete() is made at the Agent level so
            that the supervising application is informed.

   STEP 5:  -*- STORAGE -*-

            Stored components can be accessed and presented to the user for installation at a later date.  This step is optional.
            Before Issuing a UtvPublicGetDownloadSchedule() the supervising program should spin through the stored components via the 
            UtvStorageGetComponent() call and present them to the user at supervisor-defined intervals.
            A common scenario is that this happens when the TV is turned on.  The user can also say "not interested" and stored component(s)
            are deleted via the UtvStorageDeleteComponent().  They can also say "remind me later".  Note that this user query is different than
            the decision to download user query.  It's up to the CEM whether they want to ask to download and then install unconditionally or
            store unconditionally and ask to install.  The CEM can define arbitrary strings on a package or pre-component basis in Publisher 
            that can be used as friendly names, version strings, release notes, or help messages.

   STEP 6:  -*- INSTALLATION -*-

            When the user decides to install stored components they are decrypted and presented to the supervisor module by module.
            The Agent provides example implementation for assembling the modules and putting them where they belong.  Strings that are defined
            in Publisher can be accessed to tell the installation where to install.  These strings can contain file paths or offsets into a boot 
            image.  They are defined entirely by the CEM.

*/

#include "utv.h"

/* UtvCoreCommonCompatibilityUpdate
    The function is check the compatibility of an update which is under consideration
    for download.  This is the first step in update Compatibility checking.  
   Returns UTV_OK to signal compatibility; otherwise component is not compatible.
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
    UtvDependency *pSWDeps )
{
    UTV_RESULT               rStatus;
    UTV_UINT32               n;
    UTV_INT8                *pszDepName;
    UtvCompDirCompDesc      *pManifestDesc;

    /* This function must be called with the values passed in above already in Host Order!!!!! */

    /* UPDATE COMPATABILITY CHECK #1
       Platform OUI/Model Group 
    */
    UtvMessage( UTV_LEVEL_INFO, " Update-level Compatibility OUI/MG: 0x%06X == 0x%06X and 0x%04X == 0x%04X?",
                uiOui, UtvCEMGetPlatformOUI(), usModelGroup, UtvCEMGetPlatformModelGroup() );

    if ( uiOui != UtvCEMGetPlatformOUI() )
        return UTV_INCOMPATIBLE_OUI;

    if ( usModelGroup != UtvCEMGetPlatformModelGroup() )
        return UTV_INCOMPATIBLE_MGROUP;
    
  /* UPDATE COMPATABILITY CHECK #2
       Check hardware model which is component independent 
    */
    for ( n = 0; n < uiNumHMWEntries; n++, pHWMRange++ )
    {
        UtvMessage( UTV_LEVEL_INFO, " Update-level Compatibility hardware model range check: 0x%X <= 0x%X <= 0x%X?",
                    Utv_16letoh( pHWMRange->usStart ), UtvCEMTranslateHardwareModel(), Utv_16letoh( pHWMRange->usEnd ) );
        /* if any range matches punt */
        if ( Utv_16letoh( pHWMRange->usStart ) <= UtvCEMTranslateHardwareModel() && \
             Utv_16letoh( pHWMRange->usEnd ) >= UtvCEMTranslateHardwareModel() )
            break;
    }

    if ( n >= uiNumHMWEntries )
        return UTV_INCOMPATIBLE_HWMODEL;

    /* See if the update has the command attribute set.  If so punt right here. */
    if ( 0 != ( uiAttributes & UTV_COMPONENT_ATTR_COMMAND ))
        return UTV_COMMANDS_DETECTED;

    /* UPDATE COMPATABILITY CHECK #3
       Check module version.  Skip if in loop test mode.
    */
	if ( UtvManifestGetLoopTestStatus() )
	{
		UtvMessage( UTV_LEVEL_INFO, " LOOP TEST: Skipping update-level module version check: current 0x%X < proposed 0x%X?",
					UtvManifestGetModuleVersion(), usModuleVersion );
	} else
	{
		UtvMessage( UTV_LEVEL_INFO, " Update-level module version check: current 0x%X < proposed 0x%X?",
					UtvManifestGetModuleVersion(), usModuleVersion );
		if (  UtvManifestGetModuleVersion() >= usModuleVersion )
			return UTV_BAD_MODVER_CURRENT;
	}

    /* UPDATE COMPATABILITY CHECK #4
       Check SW Compatibility against system software version
    */
    for ( n = 0; n < uiNumSWVEntries; n++, pSWVRange++ )
    {
        UtvMessage( UTV_LEVEL_INFO, " Update-level Compatibility software version range check: 0x%X <= 0x%X <= 0x%X?",
                    Utv_16letoh( pSWVRange->usStart ), UtvManifestGetSoftwareVersion(), Utv_16letoh( pSWVRange->usEnd ) );
        /* if any range matches punt */
        if ( Utv_16letoh( pSWVRange->usStart ) <= UtvManifestGetSoftwareVersion() && \
             Utv_16letoh( pSWVRange->usEnd ) >= UtvManifestGetSoftwareVersion() )
            break;
    }

    /* none matched, complain */
    if ( n >= uiNumSWVEntries )
        return UTV_INCOMPATIBLE_SWVER;
    
    /* COMPONENT COMPATABILITY CHECK #5
       Check SW dependencies
    */
    for ( n = 0; n < uiNumSWDeps; n++, pSWDeps++ )
    {
        /* get this dependency's name */
        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pSWDeps->toComponentName ), &pszDepName )))
            return rStatus;

        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByName( g_hManifestImage, pszDepName, &pManifestDesc )))
        {
            UtvMessage( UTV_LEVEL_INFO, " Update-level Compatibility software dependency check: \"%s\" is missing", pszDepName );
            return UTV_DEPENDENCY_MISSING;
        }

        /* check range, if no match then we're done */
        UtvMessage( UTV_LEVEL_INFO, " Update-level Compatibility software dependency check: \"%s\" 0x%X <= 0x%X <= 0x%X?",
                    pszDepName, Utv_16letoh( pSWDeps->utvRange.usStart ), 
                    Utv_16letoh( pManifestDesc->usSoftwareVersion ), 
                    Utv_16letoh( pSWDeps->utvRange.usEnd ) );

        if ( Utv_16letoh( pSWDeps->utvRange.usStart ) > Utv_16letoh( pManifestDesc->usSoftwareVersion ) || 
             Utv_16letoh( pSWDeps->utvRange.usEnd ) < Utv_16letoh( pManifestDesc->usSoftwareVersion ) )
            return UTV_INCOMPATIBLE_DEPEND;
    }

    /* check field test bit */
    UtvMessage( UTV_LEVEL_INFO, " Update-level field test check: %d == %d?",
                ((uiAttributes & UTV_COMPONENT_ATTR_FTEST) != 0), UtvManifestGetFieldTestStatus() );
    if ( ((uiAttributes & UTV_COMPONENT_ATTR_FTEST) != 0) !=  UtvManifestGetFieldTestStatus() )
        return UTV_INCOMPATIBLE_FTEST;
        
   /* check factory test bit */
    UtvMessage( UTV_LEVEL_INFO, " Update-level factory test check: %d == %d?",
                ((uiAttributes & UTV_COMPONENT_ATTR_FACT_TEST) != 0), UtvManifestGetFactoryTestStatus() );
    if ( ((uiAttributes & UTV_COMPONENT_ATTR_FACT_TEST) != 0) !=  UtvManifestGetFactoryTestStatus() )
        return UTV_INCOMPATIBLE_FACT_TEST;

    /* This update is compatible. */
    return UTV_OK;
}

/* Endianness concerns in UtvCoreCommonCompatibilityUpdateConvert() presumed to be resolved
   for all incoming variables except for the Range and Dependency arrays. These are left and
   handled as needed for all incoming images. Unforunately, when checking the Update Query
   Response (s_utvUQR) from getDownloadScheduleWorkerInternet(), all variables are manufactured
   and thus everything is in host order. This is fine with a LE machine, not so good with BE.
   Convert each of the arrays as they exist in place before checking Compatibility and then
   convert back to the original format.
*/
UTV_RESULT 
UtvCoreCommonCompatibilityUpdateConvert(UTV_UINT32 hImage, UTV_UINT32 uiOui, UTV_UINT16 usModelGroup, 
                                        UTV_UINT16 usModuleVersion, UTV_UINT32 uiAttributes, 
                                        UTV_UINT32 uiNumHMWEntries, UtvRange *pHWMRange, 
                                        UTV_UINT32 uiNumSWVEntries, UtvRange *pSWVRange,
                                        UTV_UINT32 uiNumSWDeps, UtvDependency *pSWDeps )
{
    UTV_RESULT rStatus;
    int index;
    UtvRange *pRange;
    UtvDependency *pDep;

    for ( index = 0, pRange = pHWMRange; index < uiNumHMWEntries; index++, pRange++ )
    {
        pRange->usStart = Utv_16letoh( pRange->usStart );
        pRange->usEnd = Utv_16letoh( pRange->usEnd );
    }

    for ( index = 0, pRange = pSWVRange; index < uiNumSWVEntries; index++, pRange++ )
    {
        pRange->usStart = Utv_16letoh( pRange->usStart );
        pRange->usEnd = Utv_16letoh( pRange->usEnd );
    }

    for ( index = 0, pDep = pSWDeps; index < uiNumSWDeps; index++, pDep++ )
    {
        pDep->toComponentName = Utv_16letoh( pDep->toComponentName );
        pDep->utvRange.usStart = Utv_16letoh( pDep->utvRange.usStart );
        pDep->utvRange.usEnd = Utv_16letoh( pDep->utvRange.usEnd );
    }

    rStatus = UtvCoreCommonCompatibilityUpdate( hImage, uiOui, usModelGroup, usModuleVersion,
                                                uiAttributes, 
                                                uiNumHMWEntries, pHWMRange,
                                                uiNumSWVEntries, pSWVRange, 
                                                uiNumSWDeps, pSWDeps );

    for ( index = 0, pRange = pHWMRange; index < uiNumHMWEntries; index++, pRange++ )
    {
        pRange->usStart = Utv_16letoh( pRange->usStart );
        pRange->usEnd = Utv_16letoh( pRange->usEnd );
    }

    for ( index = 0, pRange = pSWVRange; index < uiNumSWVEntries; index++, pRange++ )
    {
        pRange->usStart = Utv_16letoh( pRange->usStart );
        pRange->usEnd = Utv_16letoh( pRange->usEnd );
    }

    for ( index = 0, pDep = pSWDeps; index < uiNumSWDeps; index++, pDep++ )
    {
        pDep->toComponentName = Utv_16letoh( pDep->toComponentName );
        pDep->utvRange.usStart = Utv_16letoh( pDep->utvRange.usStart );
        pDep->utvRange.usEnd = Utv_16letoh( pDep->utvRange.usEnd );
    }

    return rStatus;

}

/* UtvCoreCommonCompatibilityComponent
    The function is check the compatibility of each component in an update package.
   Returns UTV_OK to signal compatibility; otherwise component is not compatible.
*/
UTV_RESULT UtvCoreCommonCompatibilityComponent( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc )
{
    UTV_RESULT              rStatus;
    UTV_UINT32              n;
    UTV_INT8               *pszCompName;
    UtvRange               *pHWMRanges, *pSWVRanges;
    UtvDependency          *pSWDeps;
    UtvTextDef             *pTextDefs;
    UtvCompDirModDesc      *pModDesc;
    UtvCompDirCompDesc     *pManifestDesc;

    /* get all component info */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDescInfo( pCompDesc, &pHWMRanges, &pSWVRanges, &pSWDeps, &pTextDefs, &pModDesc )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvImageGetCompDescInfo() fails with error \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* COMPONENT COMPATABILITY CHECK #1
       Check hardware model which is component independent and optional
    */
    if ( 0 != Utv_16letoh( pCompDesc->usNumHardwareModelRanges ) )
    {
        for ( n = 0; n < Utv_16letoh( pCompDesc->usNumHardwareModelRanges ); n++, pHWMRanges++ )
        {
            UtvMessage( UTV_LEVEL_INFO, " Component-level Compatibility hardware model range check: 0x%X <= 0x%X <= 0x%X?",
                        Utv_16letoh( pHWMRanges->usStart ), 
			UtvCEMTranslateHardwareModel(), 
			Utv_16letoh( pHWMRanges->usEnd ) );

            /* we just need one match */
            if ( Utv_16letoh( pHWMRanges->usStart ) <= UtvCEMTranslateHardwareModel() &&    \
                 Utv_16letoh( pHWMRanges->usEnd ) >= UtvCEMTranslateHardwareModel() )
                break;
        }

        /* if none then no HW compatibility */
        if ( n >= Utv_16letoh( pCompDesc->usNumHardwareModelRanges ) )
            return UTV_INCOMPATIBLE_HWMODEL;
    }

    /* COMPONENT COMPATABILITY CHECK #2
       Check the software version ranges on this component
    */

    /* get this component's name */
    if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, Utv_16letoh( pCompDesc->toName ), &pszCompName )))
        return rStatus;

    /* Find it in the component manifest. */
    if ( UTV_OK != ( rStatus = UtvImageGetCompDescByName( g_hManifestImage, pszCompName, &pManifestDesc )))
    {
        UtvMessage( UTV_LEVEL_INFO, " Component-level Compatibility software version check: \"%s\" is missing", pszCompName );
        return UTV_UNKNOWN_COMPONENT;
    }

    for ( n = 0; n < Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges ); n++, pSWVRanges++ )
    {
        UtvMessage( UTV_LEVEL_INFO, " Component-level Compatibility software version range check: 0x%X <= 0x%X <= 0x%X?",
                    Utv_16letoh( pSWVRanges->usStart ), 
                    Utv_16letoh( pManifestDesc->usSoftwareVersion ), 
                    Utv_16letoh( pSWVRanges->usEnd ) );

        /* we just need one match */
        if ( ( Utv_16letoh( pSWVRanges->usStart ) <= Utv_16letoh( pManifestDesc->usSoftwareVersion)) && \
             ( Utv_16letoh( pSWVRanges->usEnd ) >= Utv_16letoh( pManifestDesc->usSoftwareVersion ) ) )
            break;
    }

    /* if none then no SW compatibility */
    if ( n >= Utv_16letoh( pCompDesc->usNumSoftwareVersionRanges ) )
        return UTV_INCOMPATIBLE_SWVER;
    
    /* COMPONENT COMPATABILITY CHECK #3
       Check SW dependencies which are optional
    */
    for ( n = 0; n < Utv_16letoh( pCompDesc->usNumSoftwareDependencies ); n++, pSWDeps++ )
    {
        /* get this dependency's name */
        if ( UTV_OK != ( rStatus = UtvPublicImageGetText( hImage, 
							  Utv_16letoh( pSWDeps->toComponentName ), 
							  &pszCompName )))
            return rStatus;

        if ( UTV_OK != ( rStatus = UtvImageGetCompDescByName( g_hManifestImage, pszCompName, &pManifestDesc )))
        {
            UtvMessage( UTV_LEVEL_INFO, " Component-level Compatibility software dependency check: \"%s\" is missing", pszCompName );
            return UTV_DEPENDENCY_MISSING;
        }

        /* check range, if no match then we're done */
        UtvMessage( UTV_LEVEL_INFO, " Component-level Compatibility software dependency check: \"%s\" 0x%X <= 0x%X <= 0x%X?",
                    pszCompName, 
		    Utv_16letoh( pSWDeps->utvRange.usStart ), 
		    Utv_16letoh( pManifestDesc->usSoftwareVersion ), 
		    Utv_16letoh( pSWDeps->utvRange.usEnd ) );

        if ( ( Utv_16letoh( pSWDeps->utvRange.usStart ) > Utv_16letoh( pManifestDesc->usSoftwareVersion ) ) || 
	     ( Utv_16letoh( pSWDeps->utvRange.usEnd ) < Utv_16letoh( pManifestDesc->usSoftwareVersion ) ) )
            return UTV_INCOMPATIBLE_DEPEND;
    }

    /* This component is compatible */
    return UTV_OK;
}

UTV_RESULT UtvCoreCommonCompatibilityCountMods( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT32 *puiModulesWritten )
{
    UTV_UINT32 i, uiFound;
    UTV_UINT16 usCompIndex;

    if ( g_utvDownloadTracker.usCurDownloadCount >= UTV_PLATFORM_MAX_MODULES )
        return UTV_MOD_ID_OVERFLOW;

    if ( NULL == pCompDesc || (UTV_OK != UtvImageGetCompIndex( hImage, pCompDesc, &usCompIndex  )))
        usCompIndex = 0xFFFF;

    for( uiFound = i = 0; i < g_utvDownloadTracker.usCurDownloadCount; i++ )
    {
        if ( g_utvDownloadTracker.ausModIdArray[ i ].usCompIndex == usCompIndex )
            uiFound++;
    }

    *puiModulesWritten = uiFound;

    return UTV_OK;
}

/* Download status tracking structure
 */
typedef struct
{
    UTV_UINT32  hImage;                     /* image where this component is to be stored */
    UtvCompDirCompDesc *pCompDesc;          /* the component descruptor associated with this download */
    UTV_UINT8   ubMaxMods;                  /* The maximum number of modules expected for this image */
/*  --------------------------------------------------------------------------------------------------------------------------
    The following members of this structure are used for scratch purposes during the download process and do NOT have to
    persist across power cycles.
    -------------------------------------------------------------------------------------------------------------------------- */
    UTV_BOOL    bDownloadInProgress;        /* indicates if a multi-module download is in progress */
    UTV_UINT8   ubDownloadModVersion;       /* Module Version of the download in progress */
    UTV_UINT8   ubHighestSignaledModVer;    /* Highest signalled mod ver detected so far */
    UTV_UINT32  iCountModulesSignaled;      /* number of modules in this in the current update for this download group */
    UTV_UINT32  uiModSize;                  /* module size of the modules being downloaded. */
} UtvDownloadStatus;

static UtvDownloadStatus s_UtvDownloadStatus;
static UtvDownloadStatus *s_pUtvDownloadStatus = &s_UtvDownloadStatus;

/* UtvCoreCommonCompatibilityDownload
    The function is called for each DSI (downloadServerInitiate) message with information
    extracted from the ATSC A/97 compaibilityDescriptor as extended by UTV for UpdateTV.
   Returns UTV_OK to signal compatibility; otherwise the OUI and model group are not compatible.
*/
UTV_RESULT UtvCoreCommonCompatibilityDownload( 
    UTV_UINT32 iOui,
    UTV_UINT32 iModelGroupId )
{
    /* Check OUI */
    if ( UtvCEMGetPlatformOUI() != iOui )
    {
        /* OUI does not match */
        return UTV_INCOMPATIBLE_OUI;
    }

    /* Check Model Group */
    if ( UtvCEMGetPlatformModelGroup() != iModelGroupId )
    {
        /* Model Group does not match */
        return UTV_INCOMPATIBLE_MGROUP;
    }

    /* The OUI and Model Group were acceptable */
    return UTV_OK;
}

/* UtvCoreCommonCompatibilityDownloadStart
    This routine is called when UtvBroadcastDownloadComponentDirectory() downloads a new component
    directory.  The new component directory should always fail the module version check because
    otherwise a component directory will have been needlessly downloaded.
*/
void UtvCoreCommonCompatibilityDownloadStart( UTV_UINT32 hImage, UTV_UINT16 usModuleVersion )
{
    UTV_UINT32    i;
        
    s_pUtvDownloadStatus->hImage = hImage;
    s_pUtvDownloadStatus->ubDownloadModVersion = (UTV_BYTE) usModuleVersion;
    s_pUtvDownloadStatus->pCompDesc = NULL;
    s_pUtvDownloadStatus->ubHighestSignaledModVer = 0;
    s_pUtvDownloadStatus->iCountModulesSignaled = 0;

    /* If current module version is different then one in download tracker then re-init download tracker.
       Otherwise assume it contains valid data. */
    if ( g_utvDownloadTracker.usModuleVersion != usModuleVersion )
    {
        UtvMessage( UTV_LEVEL_INFO, " TRACKER THROWING OUT old download tracker info, beginning to track MV 0x%02X", usModuleVersion );
        g_utvDownloadTracker.usCurDownloadCount = 0;
        for ( i = 0; i < UTV_PLATFORM_MAX_MODULES; i++ )
        {
            g_utvDownloadTracker.ausModIdArray[ i ].usModId = 0;
            g_utvDownloadTracker.ausModIdArray[ i ].usCompIndex = 0;
            /* these are still really the only important fields to clear */
        }

        g_utvDownloadTracker.usModuleVersion = usModuleVersion;

    } else
    {
        UtvMessage( UTV_LEVEL_INFO, " TRACKER RETAINING download tracker info.  %d modules already downloaded for MV 0x%02X",
                    g_utvDownloadTracker.usCurDownloadCount, usModuleVersion );

    }
}

UTV_RESULT UtvCoreCommonCompatibilityRecordModuleId( UTV_UINT32 hImage, UtvCompDirCompDesc *pCompDesc, UTV_UINT16 usModuleId )
{
    UTV_RESULT rStatus;
    UTV_UINT16 usCompIndex;

    if ( g_utvDownloadTracker.usCurDownloadCount >= UTV_PLATFORM_MAX_MODULES )
        return UTV_MOD_ID_OVERFLOW;

    if ( NULL == pCompDesc || (UTV_OK != (rStatus = UtvImageGetCompIndex( hImage, pCompDesc, &usCompIndex  ))))
        usCompIndex = 0xFFFF;

    g_utvDownloadTracker.ausModIdArray[ g_utvDownloadTracker.usCurDownloadCount   ].usCompIndex = usCompIndex;
    g_utvDownloadTracker.ausModIdArray[ g_utvDownloadTracker.usCurDownloadCount++ ].usModId = usModuleId;
    /* ulCRC and friends are handled in UtvImageInstallModule() */

    UtvMessage( UTV_LEVEL_INFO, " TRACKER STORING MODULE ID: %d-%d", usModuleId, usCompIndex );

    return UTV_OK;
}

void UtvCoreCommonCompatibilityDownloadClear( void )
{
   UTV_UINT32    i;

   s_pUtvDownloadStatus->hImage = UTV_CORE_IMAGE_INVALID;

    /* blow away download tracker */
    g_utvDownloadTracker.usModuleVersion = 0;
    g_utvDownloadTracker.usCurDownloadCount = 0;
    for ( i = 0; i < UTV_PLATFORM_MAX_MODULES; i++ )
    {
        g_utvDownloadTracker.ausModIdArray[ i ].usModId = 0;
        g_utvDownloadTracker.ausModIdArray[ i ].usCompIndex = 0;
        /* these are still really the only important fields to clear */
    }

    UtvMessage( UTV_LEVEL_INFO, " TRACKER CLEAR" );

    UtvPlatformPersistWrite();
}

/* UtvCoreCommonCompatibilityDownloadInit
    Called by cem-config.c on every boot to make sure we download a component directory first.
*/
void UtvCoreCommonCompatibilityDownloadInit( void )
{
   s_pUtvDownloadStatus->hImage = UTV_CORE_IMAGE_INVALID;
}
