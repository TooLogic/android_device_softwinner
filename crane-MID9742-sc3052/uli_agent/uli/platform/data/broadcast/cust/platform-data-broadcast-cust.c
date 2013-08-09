/* platform-data-broadcast-cust.c: UpdateTV Custom Platform Adaptation Layer
                                   Generic Tuner Interface Stub to be filled in
                                   by porting engineers with platform-specific tuner
                                   and section filter code.

   Copyright (c) 2007 - 2009 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Platform Adadptation Layer Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformDataBroadcastAcquireHardware( UTV_UINT32 iEventType )
   UTV_RESULT UtvPlatformDataBroadcastReleaseHardware( void )
   UTV_RESULT UtvPlatformDataBroadcastSetTunerState( UTV_UINT32 iFrequency )
   UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( void )
   UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pFunc )
   UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( UTV_UINT32 iPid, UTV_UINT32 iTableId, 
                                        UTV_UINT32 iTableIdExt, UTV_BOOL bEnableTableIdExt )
   UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( void )
   UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( void )
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      11/05/2009  Added UtvPlatformDataBroadcastGetTuneInfo().
   Bob Taylor      02/02/2009  Created from 1.9.21 for version 2.0.0.
*/

#include "utv.h"

/* Public Tuner Interface Functions Required By the UpdateTV Core follow...
*/

/* UtvPlatformDataBroadcastAcquireHardware
    This call checks to see if the TV hardware is idle (consumer is not watching TV) and
    then reserves the hardware for UpdateTV use. The hardware resources needed include
     - Tuner
     - Hardware section filters
     - Module download memory
   Returns true if TV is idle, false if TV is busy
*/
UTV_RESULT UtvPlatformDataBroadcastAcquireHardware( UTV_UINT32 iEventType )
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastReleaseHardware
    This call releases the hardware reserved by the UtvPlatformDataBroadcastAcquireHardware function
   Returns true if resources released, false if resources were not acquired
*/
UTV_RESULT UtvPlatformDataBroadcastReleaseHardware()
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastSetTunerState
    This call sets the tuner state and frequency.
    Cycles through modulation types when lock fails attempting what worked
    last time first and then the other two:  VSB, QAM_64, QAM_256.
   Returns OK if tuner set
*/
UTV_RESULT UtvPlatformDataBroadcastSetTunerState( 
    UTV_UINT32 iFrequency )
{
    return UTV_OK;
}

/* UtvPlatformDataBroadcastGetTunerFrequency
    This call returns the current tuner frequency, state is ignored.
   Returns frequency or 0 if unknown
*/
UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( )
{
    return 0;
}

/* UtvPlatformDataBroadcastGetTuneInfo
    This call returns the physical frequency and modulation type of the current tune.
*/
UTV_RESULT UtvPlatformDataBroadcastGetTuneInfo( UTV_UINT32 *puiPhysicalFrequency, UTV_UINT32 *puiModulationType )
{
    *puiPhysicalFrequency = 0;
    *puiModulationType = 0;
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCreateSectionFilter
    Called after Mutex and Wait handles initialized.
    Established with state set to STOPPED
    First step in using a section filter
    Sets up the section filter context
    Next step is to open the section filter
   Returns OK if the section filter has been created
*/
UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pPumpFunc )
{
    /* Indicate that the section filter hardware has been initialized and  */
    /* a single threaded or multi-threaded section delivery mechanism has  */
    /* been initialized, BUT not yet started. */
    
    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastOpenSectionFilter
    Called here from UtvPlatformDataBroadcastPersOpenSectionFilter to start the section filter with the state set to READY
    Should start data delivery even though main thread may not be ready for data yet
    Next step is to get section data 
   Returns UTV_OK if section filter opened and ready
*/
UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( 
    IN UTV_UINT32 iPid, 
    IN UTV_UINT32 iTableId, 
    IN UTV_UINT32 iTableIdExt, 
    IN UTV_BOOL bEnableTableIdExt )
{
    /* Indicate that we setup the section filter as desired and have begun to deliver section data. */

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCloseSectionFilter
    Called to stop delivery of section filter data with mutex locked and start changed to STOPPED
    Next step is to destroy the section filter or be opened on another PID/TID
    This code should leave resources available for the next section filter open operation
   Returns OK if everything closed normally
*/
UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( )
{
    /* Indicate that we have shut down the stream of section data */
    /* that was started with UtvPersOpenSectionFilter(). */

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastDestroySectionFilter
    Destroy the context created by the UtvPlatformDataBroadcastOpenSectionFilter call
    Called with mutex locked and state changed to UNKNOWN
    Any hardware context is released
    All memory allocated by the given section filter is freed
   Returns OK if everything done normally
*/
UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( )
{
   /* Indicate that we have released any resources allocated */
   /* by UtvPersCreateSectionFilter. */

    /* Return status */
    return UTV_OK;
}
