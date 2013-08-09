/* platform-data-internet-clear-socket.c: ULI Device Agent Platform Adadptation Layer

   Clear Sockets Interface Example

   Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformInternetClearOpen( char *pszHost, UTV_UINT32 iPort );
   UTV_RESULT UtvPlatformInternetClearSessionOpen( UTV_UINT32 *phSession );
   UTV_RESULT UtvPlatformInternetClearSessionClose( UTV_UINT32 hSession );
   UTV_RESULT UtvPlatformInternetClearSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen );
   UTV_RESULT UtvPlatformInternetClearGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned );
   UTV_RESULT UtvPlatformInternetClearClose( void  );
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      12/19/2011  Fixed old_cancel_state bug.
   Bob Taylor      12/02/2011  Replaced pthread_setcancelstate with UtvThreadSetCancelState().
   Jim Muchow      08/24/2010  Add code to prevent inadvertent close of stdin
   Jim Muchow      08/23/2010  Add code to use the selectMonitor when receiving data
   Jim Muchow      08/23/2010  Add code using pthread_cancelstate() when opening/configuring sockets
   Bob Taylor      06/14/2010  Remove superfluous select retry loops.
   Jim Muchow      04/06/2010  Add retry logic to UtvPlatformInternetClearSessionOpen() and
                               UtvPlatformInternetClearGetResponse(); correct some log message strings
   Bob Taylor      11/13/2009  Assume 0 len returned by UtvPlatformInternetClearGetResponse()
   Bob Taylor      03/18/2009  Created.
*/

#include "utv.h"
#include <stdio.h>                /* perror() */
#include <string.h>               /* strlen() */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>               /* read() */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

/* Local static vars */
static UTV_BOOL             s_bInternetOpen = UTV_FALSE;
static struct sockaddr_in    s_tServerName = { 0 };

UTV_RESULT UtvPlatformInternetClearOpen( char *pszHost, UTV_UINT32 iPort )
{
    UTV_RESULT rStatus;

    /* we only need to so this once */
    if ( s_bInternetOpen )
    {
        return UTV_OK;
    }

    if ( UTV_OK != ( rStatus = UtvPlatformInternetCommonDNS( pszHost, &s_tServerName.sin_addr )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInternetCommonDNS() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    s_tServerName.sin_family = AF_INET;
    s_tServerName.sin_port = htons((int)iPort);

    /* once the host name has been resolved the connection is considered to be open. */
    s_bInternetOpen = UTV_TRUE;

    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetClearSessionOpen( UTV_UINT32 *phSession )
{
    int fd = -1;
    fd_set fds;
    struct timeval tv;
    int ret;
    socklen_t len = sizeof( ret );
    UTV_INT32 old_cancel_state;

    if ( !phSession )
        return UTV_INTERNET_PERS_FAILURE;

    *phSession = 0;

    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* allocate a new socket */
    if ( -1 == (fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
    {
        UtvMessage( UTV_LEVEL_ERROR, " socket() fails!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    UtvThreadSetCancelState(UTV_PTHREAD_CANCEL_DISABLE, &old_cancel_state); 

    /* Set socket to non-blocking */
    if ( -1 == fcntl( fd, F_SETFL, O_NONBLOCK ))
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, " fcntl() fails!" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* Initiate the connection process, this will return immediately */
    if ( -1 == connect( fd, (struct sockaddr*) &s_tServerName, sizeof(s_tServerName)))
    {
        if ( errno != EINPROGRESS )
        {
            close( fd );
            UtvMessage( UTV_LEVEL_ERROR, " connect() fails!" );
            UtvThreadSetCancelState(old_cancel_state,  NULL); 
            return UTV_INTERNET_PERS_FAILURE;
        }
    }

    /* if we have been signaled to abort, bail now before we do somethign that could take awhile */
    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
    {
        close( fd );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_ABORT;
    }

    /* Wait for writeability of the socket to determine success or failure on connect */
    FD_ZERO( &fds );
    FD_SET( fd, &fds );

    tv.tv_sec = UTV_PLATFORM_INTERNET_CONNECT_TIMEOUT_SEC;
    tv.tv_usec = 0;

    UtvActivateSelectMonitor(&tv, fd);
    ret = select( fd+1, NULL, &fds, NULL, &tv );
    UtvDeactivateSelectMonitor( );

    if ( UTV_PUBLIC_STATE_ABORT == UtvGetState() )
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() SELECT => abort" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_ABORT;
    }

    if ( ret < 0 )
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() select() error" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_PERS_FAILURE;
    }

    if ( ret == 0 ) /* timeout exit */
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() timeout waiting for connect" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_CNCT_TIMEOUT;
    }

    /* At this point the connect has completed, but we still need to check for success */
    if ( -1 == getsockopt( fd, SOL_SOCKET, SO_ERROR, &ret, &len ))
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() getsockopt() failed" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_PERS_FAILURE;
    }
    if ( ret )
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionOpen() failed to connect to host" );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_PERS_FAILURE;
    }

    *phSession = ( UTV_UINT32 )fd;
    UtvThreadSetCancelState(old_cancel_state,  NULL); 
    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetClearSessionClose( UTV_UINT32 hSession )
{
    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearSessionClose() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* close the specified socket
       Note that hSession may be zero, the same value as STDIN, if this close occurs 
       after a failed re-open - see for example, UtvInternetImageAccessInterfaceRead() */
    if ( hSession )
        close( hSession );

    return UTV_OK;
}


UTV_RESULT UtvPlatformInternetClearSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen )
{
    int n;

#ifdef UTV_TRACE_INTERNET_TRANSACTION
    UtvMessage( UTV_LEVEL_INFO, " INTERNET REQUEST: %s", pbTxData );
#endif

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    if (0 > (n = write(hSession, pbTxData, uiDataLen )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " socket write() fails! %d", errno );
        return UTV_INTERNET_PERS_FAILURE;
    }

    return UTV_OK;
}
    
UTV_RESULT UtvPlatformInternetClearGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned )
{
    struct timeval tv;
    fd_set         fds;
    UTV_INT32      iBytesRead;
    UTV_INT32      ret;

    /* assume no data returned */
    *piLenReturned = 0;

    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    FD_ZERO( &fds );
    FD_SET( hSession, &fds );
    
    tv.tv_sec = UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC;
    tv.tv_usec = 0;

    UtvActivateSelectMonitor(&tv, hSession);
    ret = select( hSession+1, &fds, NULL, NULL, &tv );
    UtvDeactivateSelectMonitor( );

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() SELECT => abort" );
        return UTV_ABORT;
    }

    if ( ret < 0 )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() select() error (%d)", 
                    errno );
        return UTV_INTERNET_PERS_FAILURE;
    }

    if ( ret == 0 ) /* timeout exit */
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() timeout waiting for data" );
        return UTV_INTERNET_READ_TIMEOUT;
    }

    /* otherwise we need to actually get the data and parse the HTTP header. */
    iBytesRead = read( hSession, pcbResponseBuffer, iBufferLen );

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() READ => abort" );
        return UTV_ABORT;
    }

    if ( iBytesRead < 0 )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetClearGetResponse() socket read() fails (%d)",
                    errno );
        return UTV_INTERNET_PERS_FAILURE;
    }
    else if ( iBytesRead == 0 )
    {
        return UTV_INTERNET_CONN_CLOSED;
    }

    *piLenReturned = (UTV_UINT32)iBytesRead;    

    return ( UtvGetState() == UTV_PUBLIC_STATE_ABORT ) ? UTV_ABORT : UTV_OK;
}

/* UtvPlatformInternetClearClose()
    Currently this routine just clears a flag which tells the system 
    that it needs to resolve the host name when it communicates next.
*/
UTV_RESULT UtvPlatformInternetClearClose( )
{
    s_bInternetOpen = UTV_FALSE;    
    return UTV_OK;
}

