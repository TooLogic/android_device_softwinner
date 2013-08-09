/* platform-data-broadcast-linux-dvbapi.c: UpdateTV Linux DVB API Platform Adaptation Layer
                                           Tuner Interface Example based on Linux DVB API.

   Copyright (c) 2007-2010 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
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
   Bob Taylor      04/15/2010  Addition of full frequency lists as examples and fix of typo.
   Bob Taylor      11/05/2009  Added UtvPlatformDataBroadcastGetTuneInfo().
   Bob Taylor      02/02/2009  Created from 1.9.21 for version 2.0.0.
*/

#include "utv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <poll.h>

/* These files are typically found in /usr/include/linux/dvb */
#include "frontend.h"
#include "dmx.h"

/* Main hardware interface structure
*/
struct HWInterface
{
    UTV_BOOL   bInitialized;     /* UTV_BOOL to keep track of whether we're opened or not */
    int    frontend_fd;          /* file handle for front end (tuner) */
    int    demux_fd;             /* file handle for back end (demux) */
    int    dvr_fd;               /* file handle for data delivery */
    int    vid_fd;               /* DEMO ONLY: file handle for DVR tap video */
    int    aud_fd;               /* DEMO ONLY: file handle for DVR tap audio */
} g_HWInterface;

/* Section filter state machine structure
*/
struct HWSectionFilter
{
    UTV_UINT32 iPid;                 /* MPEG-2 virtual program id  */
    UTV_UINT32 iTableId;             /* MPEG-2 private section table id */
    UTV_UINT32 iTableIdExt;          /* MPEG-2 private section table id extension */
    UTV_BOOL   bEnableTableIdExt;    /* true if filtering on table id extension */
    UTV_BOOL   bSectionFilterOpen;   /* true when thread is supposed to be running */
    UTV_UINT32 uiSectionsDelivered;  /* Number of sections deleivered */
    UTV_UINT32 uiSectionsDiscarded;  /* Number of sections discarded */
    UTV_BYTE * pbSectionBuffer;      /* Buffer for storing sections read from device */
    UTV_THREAD_HANDLE hThread;       /* Handle for thread */
    UTV_BOOL   bThreadRunning;       /* True when thread is running */
} g_HWSectionFilter;

/* Adapter interface definitions
*/
static char FRONTEND_DEV [80];
static char DEMUX_DEV [80];
static char DVR_DEV [80];

#define FRONTEND_FMT "/dev/dvb/adapter%i/frontend%i"
#define DEMUX_FMT    "/dev/dvb/adapter%i/demux%i"
#define DVR_FMT      "/dev/dvb/adapter%i/dvr%i"

#define ADAPTER_INDEX  0
#define FRONTEND_INDEX 0
#define DEMUX_INDEX    0
#define DVR_INDEX      0

/* Modulation type definitions
*/
#define NUM_MOD_TYPES    3
fe_modulation_t g_femtModulationTypes[NUM_MOD_TYPES] = { VSB_8, QAM_64, QAM_256 };
int g_iModulationTypeIndex = 0;    /* choose VSB to start. */

/* Driver internal buffer size
*/
#define DVR_BUFFER_SIZE (1024 * 1024)

/* FOR DEMONSTRATION PURPOSES ONLY - define LINUX_DVR_TAP to turn on a "media tap" mechanism 
   that media players like MPlayer that can take advantage of the Linux DVB API to play media 
   at the same time that PSIP data is being delivered to the UpdateTV agent.
   #define LINUX_DVR_TAP
*/
#ifdef LINUX_DVR_TAP
/* FOR DEMONSTRATION PURPOSES ONLY - specify the video and audio pids to be used for the media tap.
   proto that allows setup of demo DVR tap. 
*/
#define LINUX_DVR_TAP_VIDEO_PID    0x41
#define LINUX_DVR_TAP_AUDIO_PID    0x44
#endif

/* Local static function protos
*/
#ifdef UTV_OS_SINGLE_THREAD
UTV_RESULT GetSection( UTV_UINT32 * piTimer );
#else
static UTV_THREAD_ROUTINE PumpThread( void );
static void LocalSleep( UTV_UINT32 iMilliseconds );
#endif
#ifdef LINUX_DVR_TAP
static int set_pesfilter (int fd, int pid, dmx_pes_type_t type, int dvr);
#endif

/* Public Tuner Interface Functions Required By the UpdateTV Core follow...
 */

/* UtvGetTunerFrequencyList
   For LINUX_DVBAPI only a single test frequency is returned.
*/
#ifdef UTV_TEST_FREQUENCY
UTV_UINT32  g_uiFreqList[] = { UTV_TEST_FREQUENCY };
#else
UTV_UINT32  g_uiFreqList[] =
    { 57000000,   63000000,  69000000,  79000000,  85000000, 177000000, 183000000, 189000000, 195000000, 201000000, 207000000, 213000000, 473000000,
      479000000, 485000000, 491000000, 497000000, 503000000, 509000000, 515000000, 521000000, 527000000, 533000000, 539000000, 545000000, 551000000,
      557000000, 563000000, 569000000, 575000000, 581000000, 587000000, 593000000, 599000000, 605000000, 611000000, 617000000, 623000000, 629000000,
      635000000, 641000000, 647000000, 653000000, 659000000, 665000000, 671000000, 677000000, 683000000, 689000000, 695000000, 701000000, 707000000,
      713000000, 719000000, 725000000, 731000000, 737000000, 743000000, 749000000, 755000000, 761000000, 767000000, 773000000, 779000000, 785000000,
      791000000, 797000000, 803000000
    };
#endif

/* The following frequency lists are provided as examples.  The CEM is responsible for defining their own frequency list
that is depenent on the deployment and use of the hardware platform. 
*/


/* EXAMPLE_FREQUENCY_LISTS */
#if 0

/* Standard ATSC Frequencies
 */
*/
    {  57000000,  63000000,  69000000,  79000000,  85000000, 177000000, 183000000, 189000000, 195000000, 201000000, 207000000, 213000000, 473000000,
      479000000, 485000000, 491000000, 497000000, 503000000, 509000000, 515000000, 521000000, 527000000, 533000000, 539000000, 545000000, 551000000,
      557000000, 563000000, 569000000, 575000000, 581000000, 587000000, 593000000, 599000000, 605000000, 611000000, 617000000, 623000000, 629000000,
      635000000, 641000000, 647000000, 653000000, 659000000, 665000000, 671000000, 677000000, 683000000, 689000000, 695000000, 701000000, 707000000,
      713000000, 719000000, 725000000, 731000000, 737000000, 743000000, 749000000, 755000000, 761000000, 767000000, 773000000, 779000000, 785000000,
      791000000, 797000000, 803000000
    };

/* QAM Frequencies
*/
    {  57000000,  63000000,  69000000,  75000000,  79000000,  81000000,  85000000,  87000000,  93000000,  99000000, 105000000, 111000000, 117000000,
      123000000, 129000000, 135000000, 141000000, 147000000, 153000000, 159000000, 165000000, 171000000, 177000000, 183000000, 189000000, 195000000,
      201000000, 207000000, 213000000, 219000000, 225000000, 231000000, 237000000, 243000000, 249000000, 255000000, 261000000, 267000000, 273000000,
      279000000, 285000000, 291000000, 297000000, 303000000, 309000000, 315000000, 321000000, 327000000, 333000000, 339000000, 345000000, 351000000,
      357000000, 363000000, 369000000, 375000000, 381000000, 387000000, 393000000, 399000000, 405000000, 411000000, 417000000, 423000000, 429000000,
      435000000, 441000000, 447000000, 453000000, 459000000, 465000000, 471000000, 483000000, 489000000, 495000000, 501000000, 507000000, 513000000,
      519000000, 525000000, 531000000, 537000000, 543000000, 549000000, 555000000, 561000000, 567000000, 573000000, 579000000, 585000000, 591000000,
      597000000, 603000000, 609000000, 615000000, 621000000, 627000000, 633000000, 639000000, 645000000, 651000000, 657000000, 663000000, 669000000,
      675000000, 681000000, 687000000, 693000000, 699000000, 705000000, 711000000, 717000000, 723000000, 729000000, 735000000, 741000000, 747000000,
      753000000, 759000000, 765000000, 771000000, 777000000, 783000000, 789000000, 795000000, 801000000
    };

#endif
/* END OF EXAMPLE FREQUENCY LISTS */

UTV_RESULT UtvGetTunerFrequencyList (
    UTV_UINT32 * piFrequencyListSize,
    UTV_UINT32 * * ppFrequencyList )
{
    *piFrequencyListSize = sizeof( s_uiFreqList ) / sizeof ( UTV_UINT32 );
    *ppFrequencyList = s_uiFreqList;

    return UTV_OK;
}

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
    /* Only need to do this once. */
    if ( g_HWInterface.bInitialized )
        return UTV_OK;

    /* Open FRONT END device */
    /* format front end device path depending on adapter and front end index */
    snprintf (FRONTEND_DEV, sizeof(FRONTEND_DEV),
              FRONTEND_FMT, ADAPTER_INDEX, FRONTEND_INDEX);

    if ((g_HWInterface.frontend_fd = open(FRONTEND_DEV, O_RDWR)) < 0) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "Can't open front end device: %s\n", FRONTEND_DEV );
        return UTV_NO_TUNER;
    }

    /* Open DEMUX device */
    /* format demux device path depending on adapter and demux index */
    snprintf (DEMUX_DEV, sizeof(DEMUX_DEV),
              DEMUX_FMT, ADAPTER_INDEX, DEMUX_INDEX);

    if ((g_HWInterface.demux_fd = open(DEMUX_DEV, O_RDWR)) < 0) 
    {
      UtvMessage( UTV_LEVEL_ERROR, "Can't open demux device: %s\n", DEMUX_DEV );
      return UTV_NO_TUNER;
    }

    /* Open DVR device */
    /* format dvr device path depending on adapter and dvr index */
    /* this is done here and re-used in the pump thread control functions. */
    snprintf (DVR_DEV, sizeof(DVR_DEV),
              DVR_FMT, ADAPTER_INDEX, DVR_INDEX);

#ifdef LINUX_DVR_TAP
    /* */
    /* FOR DEMONSTRATION PURPOSES ONLY - create filters to allow and audio/video player */
    /* like MPLayer to get its media from the same transport stream that the UpdateTV PSIP data is */
    /* arriving on. */
    if ((g_HWInterface.vid_fd = open(DEMUX_DEV, O_RDWR)) < 0) 
    {
      UtvMessage( UTV_LEVEL_ERROR, "Can't open demux device fro experimental video: %s\n", DEMUX_DEV );
      return UTV_NO_TUNER;
    }

    if (set_pesfilter (g_HWInterface.vid_fd, LINUX_DVR_TAP_VIDEO_PID, DMX_PES_VIDEO, 1) < 0)
    {
      UtvMessage( UTV_LEVEL_ERROR, "set_pesfilter() fails for video: %s\n", DEMUX_DEV );
      return UTV_NO_TUNER;
    }

    if ((g_HWInterface.aud_fd = open(DEMUX_DEV, O_RDWR)) < 0)
    {
      UtvMessage( UTV_LEVEL_ERROR, "Can't open demux device fro experimental audio: %s\n", DEMUX_DEV );
      return UTV_NO_TUNER;
    }

    if (set_pesfilter (g_HWInterface.aud_fd, LINUX_DVR_TAP_AUDIO_PID, DMX_PES_AUDIO, 1) < 0)
    {
      UtvMessage( UTV_LEVEL_ERROR, "set_pesfilter() fails for audio: %s\n", DEMUX_DEV );
      return UTV_NO_TUNER;
    }
#endif

    /* Mark interface as open */
    g_HWInterface.bInitialized = UTV_TRUE;

    return UTV_OK;
}

/* UtvPlatformDataBroadcastReleaseHardware
    This call releases the hardware reserved by the UtvAcquireHardware function 
   Returns true if resources released, false if resources were not acquired
*/
UTV_RESULT UtvPlatformDataBroadcastReleaseHardware()
{
    UTV_RESULT rStatus = UTV_OK;

    /* If already released then just return OK. */
    if ( !g_HWInterface.bInitialized )
        return UTV_OK;

#ifdef LINUX_DVR_TAP
    /* When the DVR tap is running don't give up the hardware because it */
    /* will shutdown the media stream. */
    return rStatus;
#endif

    /* close the section filter pump if it happens to be running. */
    UtvPlatformDataBroadcastCloseSectionFilter();

    /* release the section filter memory if it's still allocated. */
    UtvPlatformDataBroadcastDestroySectionFilter();

    if ( 0 == g_HWInterface.frontend_fd )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Can't close front end device: %s\n", FRONTEND_DEV );
        rStatus = UTV_NO_TUNER;
    }
    close( g_HWInterface.frontend_fd );

    if ( 0 == g_HWInterface.demux_fd )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Can't close demux device: %s\n", DEMUX_DEV );
        rStatus = UTV_NO_TUNER;
    }
    close( g_HWInterface.demux_fd );

    g_HWInterface.bInitialized = UTV_FALSE;

    return rStatus;
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
    int                             i, iModTypeCount;
    struct dvb_frontend_parameters     p;
    fe_status_t                     s;

    /* if the interface isn't opened or the front end isn't */
    /* available then complain. */
    if ( !g_HWInterface.bInitialized || 0 == g_HWInterface.frontend_fd )
        return UTV_NO_TUNER;

#ifdef UTV_TEST_ABORT
    UtvAbortEnterSysCall();
#endif  /* UTV_TEST_ABORT */

    /* cycle through modulation types if lock fails */
    for ( iModTypeCount = 0; iModTypeCount < NUM_MOD_TYPES; iModTypeCount++ )
    {
        /* set frequency and modulation type in argument struct */
        p.frequency = iFrequency;
        p.u.vsb.modulation = g_femtModulationTypes[ g_iModulationTypeIndex ];

        if (ioctl(g_HWInterface.frontend_fd, FE_SET_FRONTEND, &p) == -1) 
        {
            UtvMessage( UTV_LEVEL_ERROR, "Can't set FE status.\n" );
            return UTV_NO_TUNER;
        }

        for (i = 0; i < 10; i++) 
        {
            /* Check abort state, get out now if aborting */
            if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
                return UTV_ABORT;

            usleep (200000);

            if (ioctl(g_HWInterface.frontend_fd, FE_READ_STATUS, &s) == -1) 
            {
                UtvMessage( UTV_LEVEL_ERROR, "Can't set FE status.\n" );
                return UTV_NO_TUNER;
            }
            
            if (s & FE_HAS_LOCK) 
                break;
        }

        if (s & FE_HAS_LOCK) 
            break;

        /* couldn't get lock, attempt next modulation type */
        if ( ++g_iModulationTypeIndex >= NUM_MOD_TYPES )
            g_iModulationTypeIndex = 0;
    }

#ifdef UTV_TEST_ABORT
    UtvAbortExitSysCall();
#endif  /* UTV_TEST_ABORT */

    /* Did the tuner get lock? */
    if ( !(s & FE_HAS_LOCK) )
    {
        UtvMessage( UTV_LEVEL_ERROR, " failed to tune to %d ", iFrequency );
        if (UtvGetState() == UTV_PUBLIC_STATE_ABORT)
            return UTV_ABORT;
        else
            return UTV_BAD_LOCK;
    }

    /* Indicate we tuned */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastGetTunerFrequency
    This call sets the tuner frequency, state is ignored.
    Returns frequency or 0 if unknown
*/
UTV_UINT32 UtvPlatformDataBroadcastGetTunerFrequency( )
{
    struct dvb_frontend_parameters p;

    /* if the interface isn't opened or the front end isn't */
    /* available then complain. */
    if ( !g_HWInterface.bInitialized || 0 == g_HWInterface.frontend_fd )
        return 0;

    if (ioctl(g_HWInterface.frontend_fd, FE_GET_FRONTEND, &p) < 0)
    {
      UtvMessage( UTV_LEVEL_ERROR, "Can't read FE info.\n" );
      return 0;
    }

    /* Return frequency in hz */
    return p.frequency;
}

/* UtvPlatformDataBroadcastGetTuneInfo
    This call returns the physical frequency and modulation type of the current tune.
*/
UTV_RESULT UtvPlatformDataBroadcastGetTuneInfo( UTV_UINT32 *puiPhysicalFrequency, UTV_UINT32 *puiModulationType )
{
    struct dvb_frontend_parameters p;

    /* if the interface isn't opened or the front end isn't */
    /* available then complain. */
    if ( !g_HWInterface.bInitialized || 0 == g_HWInterface.frontend_fd )
        return UTV_UNKNOWN;

    if (ioctl(g_HWInterface.frontend_fd, FE_GET_FRONTEND, &p) < 0)
    {
      UtvMessage( UTV_LEVEL_ERROR, "Can't read FE info.\n" );
      return UTV_UNKNOWN;
    }

    *puiPhysicalFrequency = p.frequency;
    *puiModulationType = 0;
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCreateSectionFilter
    Called after Mutex and Wait handles established with state set to STOPPED  
    First step in using a section filter
    Sets up the section filter context
    Next step is to open the section filter
   Returns OK if the section filter has been created ok
*/
UTV_RESULT UtvPlatformDataBroadcastCreateSectionFilter( UTV_SINGLE_THREAD_PUMP_FUNC *pPumpFunc )
{
    /* Allocate section-sized buffer */
    g_HWSectionFilter.pbSectionBuffer = (UTV_BYTE *)UtvMalloc( UTV_MPEG_MAX_SECTION_SIZE );

    if ( g_HWSectionFilter.pbSectionBuffer == NULL )
        return UTV_NO_MEMORY;

#ifndef UTV_OS_SINGLE_THREAD
    /* In the multi-threaded version no pump function is used. */
    *pPumpFunc = NULL;
#else
    /* In the single-threaded version we use a derivative of the  */
    /* pump thread that returns a single section. */
    *pPumpFunc = GetSection;
#endif

    /* Return the status to caller */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastOpenSectionFilter
    Called here from UtvPlatformDataBroadcastOpenSectionFilter to start the section filter with the state set to READY
    Next step is to get section data
   Returns UTV_OK if section filter opened and ready
*/
UTV_RESULT UtvPlatformDataBroadcastOpenSectionFilter( 
    IN UTV_UINT32 iPid, 
    IN UTV_UINT32 iTableId, 
    IN UTV_UINT32 iTableIdExt, 
    IN UTV_BOOL bEnableTableIdExt )
{
    /* Set the section PID we're interested in. */
    struct dmx_sct_filter_params flt;

    /* Make sure we aren't already open first */
    if ( g_HWSectionFilter.bSectionFilterOpen == UTV_TRUE )
      return UTV_NO_SECTION_FILTER;

    /* We need to copy the data for local use */
    g_HWSectionFilter.iPid = iPid;
    g_HWSectionFilter.iTableId = iTableId;
    g_HWSectionFilter.iTableIdExt = iTableIdExt;
    g_HWSectionFilter.bEnableTableIdExt = bEnableTableIdExt;

    /* Get the data control now */
    if ( 0 == g_HWInterface.demux_fd )
      return UTV_NO_SECTION_FILTER;

    /* we'll do all the table parsing ourselves so zero the filter. */
    flt.pid = iPid;
    flt.pid = iPid;
    memset(&flt.filter, 0, sizeof(struct dmx_filter));

    flt.filter.filter[ 0 ] = (UTV_BYTE) iTableId;
    flt.filter.mask[ 0 ] = 0xFF;

    if ( bEnableTableIdExt )
    {
        flt.filter.filter[ 1 ] = (UTV_BYTE) (iTableIdExt >> 8 );
        flt.filter.filter[ 2 ] = (UTV_BYTE) (iTableIdExt);
        flt.filter.mask[ 1 ] = 0xFF;
        flt.filter.mask[ 2 ] = 0xFF;
    }

    flt.timeout = 0;
    flt.flags = DMX_CHECK_CRC;
        
    if (ioctl( g_HWInterface.demux_fd, DMX_SET_FILTER, &flt ) < 0) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "  data control error adding pid 0x%X via section filter ", iPid);
        return UTV_NO_SECTION_FILTER;
    }

    /* setup a meg input buffer on the data input path */
    if (ioctl( g_HWInterface.demux_fd, DMX_SET_BUFFER_SIZE, DVR_BUFFER_SIZE) < 0 )
    {
        UtvMessage( UTV_LEVEL_ERROR, "Can't setup data delivery buffer size to %d bytes.\n", DVR_BUFFER_SIZE );
        return UTV_NO_MEMORY;
    }

    /* Start the flow of data. */
    if (ioctl( g_HWInterface.demux_fd, DMX_START, 0 ) < 0) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "  unable to start section filter");
        return UTV_NO_SECTION_FILTER;
    }

    /* we are officially pumping data */
    g_HWSectionFilter.bSectionFilterOpen = UTV_TRUE;

    /* Only start the section pumpt thread in the multi-threaded version */
#ifndef UTV_OS_SINGLE_THREAD
    g_HWSectionFilter.hThread = 0;

    /* Start the data pump thread */
    if ( UTV_OK != UtvThreadCreate( &g_HWSectionFilter.hThread, 
                                    (UTV_THREAD_START_ROUTINE)&PumpThread, 
                                    (void *)&g_HWSectionFilter ) ) 
    {
        /* Thread create error */
        UtvMessage( UTV_LEVEL_ERROR, "  unable to create thread.  %s ", strerror(errno)); 

        /* Stop data pump */
        if (ioctl( g_HWInterface.demux_fd, DMX_STOP, 0 ) < 0 ) 
        {
            UtvMessage( UTV_LEVEL_ERROR, "  unable to stop section filter after create fails");
            return UTV_NO_SECTION_FILTER;
        }

        return UTV_NO_THREAD;
    }
#endif

    /* Return status */
    return UTV_OK;
}

/* UtvPlatformDataBroadcastCloseSectionFilter
    Called to stop delivery of section filter data with mutex locked and start changed to STOPPED
    Next step is to destroy the section filter or be opened on another PID/TID
   Returns OK if everything closed normally
*/
UTV_RESULT UtvPlatformDataBroadcastCloseSectionFilter( )
{
    UTV_RESULT rStatus = UTV_OK;
    UTV_UINT32 iCount;

    /* If the pump thread is already shutdown then OK */
    if ( !g_HWSectionFilter.bSectionFilterOpen )
        return rStatus;

    /* Ask the pump thread to shut down */
    g_HWSectionFilter.bSectionFilterOpen = UTV_FALSE;

    /* stop the stream after the shutdown otherwise the read hangs. */
    if (ioctl( g_HWInterface.demux_fd, DMX_STOP, 0 ) < 0) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "  unable to stop section filter on close");
        return UTV_NO_SECTION_FILTER;
    }

    /* Only stop the section pumpt thread in the multi-threaded version */
#ifndef UTV_OS_SINGLE_THREAD
    /*  Thread should be shutting down.  Give it at least as much time as the poll timeout. */
    for ( iCount = 0; iCount < 1000; iCount++ )
    {
        /* Sleep for a bit to give the thread time to exit.   */
        /* Use LocalSleep that sleeps even when in abort state. */
        LocalSleep( 10 );
        
        /* If the other thread is gone then go ahead, otherwise wait a bit longer */
        if ( g_HWSectionFilter.bThreadRunning == UTV_FALSE ) 
            break;
    }

    /* Is the thread still running? */
    if ( g_HWSectionFilter.bThreadRunning == UTV_TRUE ) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "  unable to stop thread  "); 
        rStatus = UTV_BAD_THREAD;
    }

    /* Ask for its destruction...  clean up the residue */
    if ( UTV_OK == rStatus )
    {
        /* Clean house */
        rStatus = UtvThreadDestroy( &g_HWSectionFilter.hThread );
        if ( UTV_OK != rStatus )
            UtvMessage( UTV_LEVEL_ERROR, "  unable to destroy thread, error %s (%s) ", UtvGetMessageString(rStatus), strerror( errno ) ); 
    }
#endif

    /* Return status */
    return rStatus;
}

/* UtvPlatformDataBroadcastDestroySectionFilter
    Destroy the context created by the UtvOpenSectionFilter call
    Called with mutex locked and state changed to UNKNOWN
    Any hardware context is released
    All memory allocated by the given section filter is freed
   Returns OK if everything done normally
*/
UTV_RESULT UtvPlatformDataBroadcastDestroySectionFilter( )
{
    /* Dump the buffer */
    if ( g_HWSectionFilter.pbSectionBuffer != NULL )
    {
        UtvFree( g_HWSectionFilter.pbSectionBuffer );
        g_HWSectionFilter.pbSectionBuffer = NULL;

        /* Return status */
        UtvMessage( UTV_LEVEL_INFO, " total %d sections delivered, %d sections discarded", g_HWSectionFilter.uiSectionsDelivered, g_HWSectionFilter.uiSectionsDiscarded );
    }
    
    return UTV_OK;
}

/* Static Functions local to this hardware suppport module follow...
*/

#ifndef UTV_OS_SINGLE_THREAD
/* LocalSleep
    Sleep for the given number of milliseconds
    Unlike UtvSleep, This does not use a timed wait
    and is not woken up during abort.
*/
#define MIN_SLEEP_DELAY 5
static void LocalSleep( UTV_UINT32 iMilliseconds )
{
    struct timespec sleeptime;

/* nanosleep for small values (2 msec on linux 2.4.18) will busywait. */
/* make sure sleep is long enough to do a context switch */

    if ( iMilliseconds < MIN_SLEEP_DELAY )
        iMilliseconds = MIN_SLEEP_DELAY;

    sleeptime.tv_nsec = (iMilliseconds % 1000) * 1000 * 1000;
    sleeptime.tv_sec = (iMilliseconds / 1000);
    
    nanosleep( &sleeptime, NULL);
}

/* PumpThread
    Thread that delivers sections from the hardware to the UpdateTV Core
    via the Linux DVB API when configured in multi-threaded mode.
*/
static UTV_THREAD_ROUTINE PumpThread( )
{
    UTV_RESULT rStatus;
    int iBytesRead;
    struct pollfd pfd[1];

    /* Setup the fd to poll for data available. */
    pfd[0].fd = g_HWInterface.demux_fd;
    pfd[0].events = POLLIN;

    /* Remember thread is running */
    g_HWSectionFilter.bThreadRunning = UTV_TRUE;

    /* Data pump to other thread */
    while ( g_HWSectionFilter.bSectionFilterOpen )
    {
        /* wait here until we know we have data */
        /* timeout after a second to allow us to check for section close request. */
        if ( 0 == poll(pfd,1,1000) )
        {
            /* timeout, just continue. */
            continue;
        }

        if ( !(pfd[0].revents & POLLIN) )
        {
            /* tolerate periodic invalid polling requests for some reason */
            if ( !(pfd[0].revents & POLLNVAL) )
                UtvMessage( UTV_LEVEL_ERROR, "  PumpThread() unknown poll event: %x ", pfd[0].revents ); 
            continue;
        }

        /* We now have an entire section of data. */
        if ((iBytesRead = read( g_HWInterface.demux_fd, 
                                g_HWSectionFilter.pbSectionBuffer, UTV_MPEG_MAX_SECTION_SIZE )) <= 0) 
        {
            UtvMessage( UTV_LEVEL_ERROR, "section read fails" );
            continue;
        }
        
        if ( 0 != iBytesRead )
        {
            /* Deliver this section to the core */
            if ( UTV_OK != (rStatus = UtvDeliverSectionFilterData( iBytesRead, g_HWSectionFilter.pbSectionBuffer )))
                g_HWSectionFilter.uiSectionsDiscarded++;
            else
                g_HWSectionFilter.uiSectionsDelivered++;
            
        } else 
        {
            /* Data reception error of some sort */
            UtvMessage( UTV_LEVEL_ERROR, "  zero bytes read" ); 
        }
    } /* while section filter open */

    /* When we get here we need to terminate this thread now, just return to exit */
    g_HWSectionFilter.bThreadRunning = UTV_FALSE;
    return (void *) 0;
}
#else

/* GetSection
    Function that gets the next available section from the section filter if there is one.

    Define the number of milliseconds to wait for section data
    This shouldn't be too long otherwise aborts will not be responsive enough.
*/
#define SECTION_DATA_WAIT   100
UTV_RESULT GetSection( UTV_UINT32 * piTimer )
{
    UTV_RESULT rStatus;
    int iBytesRead;
    struct pollfd pfd[1];

    /* Setup the fd to poll for data available. */
    pfd[0].fd = g_HWInterface.demux_fd;
    pfd[0].events = POLLIN;

    /* If the section isn't open then complain. */
    if ( !g_HWSectionFilter.bSectionFilterOpen )
    {
      return UTV_BAD_SECTION_FILTER_NOT_RUNNING;        
    }
    
    /* wait here for SECTION_DATA_WAIT milliseconds */
    /* if we timeout decrement polling timer. */
    if ( 0 == poll(pfd,1,SECTION_DATA_WAIT) )
    {
        /* timeout, decrement timer and return */
        if ( *piTimer < SECTION_DATA_WAIT )
            *piTimer = 0;
        else
            *piTimer -= SECTION_DATA_WAIT;
        return UTV_NO_DATA;
    }

    if ( !(pfd[0].revents & POLLIN) )
    {
        /* tolerate periodic invalid polling requests for some reason */
        if ( !(pfd[0].revents & POLLNVAL) )
        {
            UtvMessage( UTV_LEVEL_ERROR, "  PumpThread() unknown poll event: %x ", pfd[0].revents ); 
            if ( *piTimer < SECTION_DATA_WAIT )
                *piTimer = 0;
            else
                *piTimer -= SECTION_DATA_WAIT;
            return UTV_NO_DATA;
        }
    }

    /* We now have an entire section of data. */
    if ((iBytesRead = read( g_HWInterface.demux_fd, 
                            g_HWSectionFilter.pbSectionBuffer, UTV_MPEG_MAX_SECTION_SIZE )) <= 0) 
    {   
        UtvMessage( UTV_LEVEL_ERROR, "section read fails" );
        if ( *piTimer < SECTION_DATA_WAIT )
            *piTimer = 0;
        else
            *piTimer -= SECTION_DATA_WAIT;
        return UTV_NO_DATA;
    }
        
    if ( 0 != iBytesRead )
    {
        /* Deliver this section to the core */
        if ( UTV_OK != (rStatus = UtvDeliverSectionFilterData( iBytesRead, g_HWSectionFilter.pbSectionBuffer )))
            g_HWSectionFilter.uiSectionsDiscarded++;
        else
            g_HWSectionFilter.uiSectionsDelivered++;
    } else 
    {
        /* Data reception error of some sort */
        UtvMessage( UTV_LEVEL_ERROR, "  zero bytes read" ); 
        if ( *piTimer < SECTION_DATA_WAIT )
            *piTimer = 0;
        else
            *piTimer -= SECTION_DATA_WAIT;
        return UTV_NO_DATA;
    }
    
    /* data recieved. */
    return UTV_OK;
}

#endif

#ifdef LINUX_DVR_TAP
/* FOR DEMONSTRATION PURPOSES ONLY */
/* set_pesfilter
    A local static routine that sets up a tap filter that allows an A/V player like
    MPlayer which uses the Linux DVB API to receive its video and audio data from the
    same stream that the UpdateTV PSIP information is streaming on.
   Returns 0 if successful, -1 otherwise.
*/
static
int set_pesfilter (int fd, int pid, dmx_pes_type_t type, int dvr)
{
  struct dmx_pes_filter_params pesfilter;

  if (pid <= 0 || pid >= 0x1fff)
    return 0;

  pesfilter.pid = pid;
  pesfilter.input = DMX_IN_FRONTEND;
  pesfilter.output = dvr ? DMX_OUT_TS_TAP : DMX_OUT_DECODER;
  pesfilter.pes_type = type;
  pesfilter.flags = DMX_IMMEDIATE_START;

  if (ioctl(fd, DMX_SET_PES_FILTER, &pesfilter) < 0) 
  {
    UtvMessage( UTV_LEVEL_ERROR, "  set_pesfilter() fails adding pid 0x%X via PES filter", pid);
    return -1;
  }

  return 0;
}

#endif


