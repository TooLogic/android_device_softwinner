/* data-src-internet-common.c: UpdateTV Internet Personality Map
                               Common hardware access functions.

   Copyright (c) 2009 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformInternetCommonAcquireHardware( UTV_UINT32 uiEventType );
   UTV_RESULT UtvPlatformInternetCommonReleaseHardware( void );
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Jim Muchow      07/21/2011  Many changes, thread creation is now local, better error 
                               checking and comments, WIN32 conditional removed
   Bob Taylor      03/18/2009  Created.
*/

#include "utv.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
extern int h_errno;
#include <stdio.h>
#include <pthread.h>

/* local dns cache tracking structs */

#define _NUM_DNS_CACHE_ENTRIES        2
#define _HOST_NAME_MAX_LEN            128

typedef struct 
{
    UTV_INT8      szHostName[_HOST_NAME_MAX_LEN];
    UTV_UINT32    uiAddress;
} _DNS_CACHE_ENTRY;

static _DNS_CACHE_ENTRY s_DNSCacheEntries[ _NUM_DNS_CACHE_ENTRIES ];
static UTV_BOOL s_bInitialized = UTV_FALSE;

/* Both of these routines do nothing as shipped, but may be modified to take care of
   internet network hardware arbitration.

   When the Aegent is operating in interactive mode these routines often are just stubbed out
   because the supervising program already knows that the Agent will be run and has already
   pre-empted access to the hardware.  In unattended mode these routines become important because
   the Agent will wake up and want to access the hardware.  These routines allow the Agent to
   be denied or granted that access.
*/

/* UtvPlatformInternetAcquireHardware
    Called by the core to give the CEM with an opportunity to establish access to the socket
    hardware and arbitrate conflicts before a series of socket open/close calls.
*/
UTV_RESULT UtvPlatformInternetCommonAcquireHardware( UTV_UINT32 uiEventType )
{
    return UTV_OK;
}

/* UtvPlatformInternetReleaseHardware
    Called by the core to indicate that the socket resource is being freed until the next call
    to UtvPlatformInternetAcquireHardware().
*/
UTV_RESULT UtvPlatformInternetCommonReleaseHardware()
{
    return UTV_OK;
}

/* *******************************************************************************
  Creating a thread to wait for gethostbyname() to complete is a great idea, but
  gethostbyname() has been found empirically to be essentially impervious to the
  typical means of "stopping" a thread short of a kill signal (which will also kill
  the agent). It is necessary to understand this better.

  First of all, in Linux, the primary means of controlling gethostbyname() is by 
  using the /etc/resolv.conf config file. In addition to setting up DNS server
  addresses, it can also modify default retry counts and timeouts.

  Second, the UtvThread code presumes full control of the threads registered there.
  This is not useful with a gethostbyname() thread, so it has been decided to 
  perform threading manually in this case. In addition, the thread created will be
  configured as detached so that when it exits, it will clean up after itself and
  exit.
*/
static UTV_WAIT_HANDLE      s_hWait; /* for gethostbyname timeout */
static UTV_UINT32           s_uiAddress;
static UTV_BOOL             s_bDNSRequestProcessed;
static pthread_t            s_dnsThread;
static pthread_attr_t       s_dnsThreadAttrttr;
static int                  s_herrno_copy;

static void *getHostByNameThread( void *pszHostAsVoid )
{
    UTV_INT8 *pszHost;
    struct hostent      *pHE;

    pszHost = (UTV_INT8 *)pszHostAsVoid;
    pHE = gethostbyname( pszHost ); /* resolve the host name */
    if ( NULL == pHE )
        s_herrno_copy = h_errno; /* h_error seems to be pretty 
                                    volatile, save it for later use */
    else if ( pHE->h_addrtype == AF_INET &&
              pHE->h_length == sizeof( UTV_UINT32 ) )
    {
        s_uiAddress = *(int*)( pHE -> h_addr_list[0] );
    }
    else /* a non-V4 IP address, will be processed like NULL == pHE */
        s_herrno_copy = NO_ADDRESS; /* see netdb.h */

    /* let the caller know we exited so it can look for false exit signals */
    s_bDNSRequestProcessed = UTV_TRUE;
    UtvWaitSignal( s_hWait );

#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD EXIT: getHostByNameThread" );
#endif
    s_dnsThread = 0; /* reset */
    return NULL; /* necessary to exit a thread */
}


UTV_RESULT UtvPlatformInternetCommonDNS( char *pszHost, void *puiAddress )
{
    UTV_UINT32            uiTimer, index, uiLen;
    UTV_RESULT            rStatus;
    int result; /* pthread_create() is an int return */

    /* if the local cache isn't inited yet do so */
    if ( !s_bInitialized )
    {
        for ( index = 0; index < _NUM_DNS_CACHE_ENTRIES; index++ )
            s_DNSCacheEntries[ index ].uiAddress = 0;
        s_bInitialized = UTV_TRUE;
    }

    /* search for an entry in the local cache for the host name first so we can avoid going to DNS too often */
    for ( index = 0; index < _NUM_DNS_CACHE_ENTRIES; index++ )
    {
        if ( 0 != s_DNSCacheEntries[ index ].uiAddress )
        {
            if (( uiLen = UtvStrlen((UTV_BYTE *) pszHost)) == UtvStrlen((UTV_BYTE *) s_DNSCacheEntries[ index ].szHostName ) )
                if ( 0 == UtvMemcmp( (UTV_BYTE *) pszHost, (UTV_BYTE *) s_DNSCacheEntries[ index ].szHostName, uiLen ))
                    break;
        }
    }
    
    if ( index < _NUM_DNS_CACHE_ENTRIES )
    {
        *(UTV_UINT32 *)puiAddress = s_DNSCacheEntries[ index ].uiAddress;
        return UTV_OK;
    }

    UtvWaitInit( &s_hWait );
    s_uiAddress = 0;
    s_bDNSRequestProcessed = UTV_FALSE;
#ifdef THREADS_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "THREAD CREATE: getHostByNameThread");
#endif
    /* create a thread that will clean itself up */
    (void)pthread_attr_init(&s_dnsThreadAttrttr); 
    (void)pthread_attr_setdetachstate(&s_dnsThreadAttrttr, PTHREAD_CREATE_DETACHED);
    result =  pthread_create( &s_dnsThread, &s_dnsThreadAttrttr, getHostByNameThread, pszHost );
    (void)pthread_attr_destroy(&s_dnsThreadAttrttr); /* does nothing, could, looks more symmetric */
    if ( 0 != result )
    {
#ifdef THREADS_DEBUG
        UtvMessage( UTV_LEVEL_ERROR, "THREAD CREATE: failed, %d (%s)", 
                    result, strerror( result ) );
#endif
        return UTV_NO_THREAD;
    }

    /* thread created and started, start up the timer to wait for completion */
    uiTimer = UTV_PLATFORM_INTERNET_DNS_TIMEOUT_SEC * 1000;
    rStatus = UtvWaitTimed( s_hWait, &uiTimer );
    UtvWaitDestroy( s_hWait );

    if ( ( UTV_OK != rStatus ) && ( UTV_TIMEOUT != rStatus ) )
    {
        /* something weird, UtvWaitTimed() failed in some odd way */
        UtvMessage( UTV_LEVEL_ERROR, "DNS failed, unexpected error: %s",
                    UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* ( ( UTV_OK == rStatus ) || ( UTV_TIMEOUT == rStatus ) ) */

    /* if s_bDNSRequestProcessed is (still) false, it really   */
    /* can't be anything other than UTV_TIMEOUT == rStatus     */
    if ( UTV_FALSE == s_bDNSRequestProcessed )
    {
        UtvMessage( UTV_LEVEL_ERROR, "DNS query did not complete: %s",
                    UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* gethostbyname() error, note this means UTV_OK == rStatus */
    if ( 0 == s_uiAddress ) 
    {
        UtvMessage( UTV_LEVEL_ERROR, "DNS returned no host address: %d (%s)",
                    s_herrno_copy, hstrerror( s_herrno_copy ) );
        return UTV_INTERNET_DNS_FAILS;
    }

    /* still ( ( UTV_OK == rStatus ) || ( UTV_TIMEOUT == rStatus ) ): */
    /* it is neither illegal nor impossible have a query timeout that */
    /* manages to get an address before being processed               */

    /* set return value */
    *(UTV_UINT32 *)puiAddress = s_uiAddress;

    /* create entry in local cache for this address */
    for ( index = 0; index < _NUM_DNS_CACHE_ENTRIES; index++ )
    {
        if ( 0 == s_DNSCacheEntries[ index ].uiAddress )
            break;
    }
    
    /* if space found use it otherwise take the second entry because the first is probably the query server */
    if ( index >= _NUM_DNS_CACHE_ENTRIES )
        index = _NUM_DNS_CACHE_ENTRIES-1;

    UtvStrnCopy( (UTV_BYTE *) s_DNSCacheEntries[ index ].szHostName, _HOST_NAME_MAX_LEN, (UTV_BYTE *) pszHost, UtvStrlen((UTV_BYTE *) pszHost ));
    s_DNSCacheEntries[ index ].uiAddress = s_uiAddress;

    return UTV_OK;
}
