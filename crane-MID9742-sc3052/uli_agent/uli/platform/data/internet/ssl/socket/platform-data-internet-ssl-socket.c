/* data-src-internet-ssl-socket.c: UpdateTV Internet Personality Map
                                   SSL Clear-Dummy Interface Example

   Copyright (c) 2009 UpdateLogic Incorporated. All rights reserved.

   This version of the SSL implementation is for testing the SSL entry points
   without actually using SSL.

   Contains the following UpdateTV Agent Personality Map Entry Points
   ------------------------------------------------------------------
   UTV_RESULT UtvPlatformInternetSSLOpen( char *pszHost, UTV_UINT32 iPort );
   UTV_RESULT UtvPlatformInternetSSLSessionOpen( UTV_UINT32 *phSession );
   UTV_RESULT UtvPlatformInternetSSLSessionClose( UTV_UINT32 hSession );
   UTV_RESULT UtvPlatformInternetSSLSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen );
   UTV_RESULT UtvPlatformInternetSSLGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned );
   UTV_RESULT UtvPlatformInternetSSLClose( void  );
   ------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Jim Muchow      07/5/2011   Provide stubbed out SSL Init/De-init functions
   Jim Muchow      03/19/2010  Replace clear channel TIMEOUT errors with new encrypted channel errors
   Bob Taylor      01/26/2009  Created from version 1.9.21
*/

#include "utv.h"
#include <stdio.h>                /* perror() */
#include <string.h>               /* strlen() */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>               /* read()   */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

/* Local static vars */
static UTV_BOOL             s_bInternetOpen = UTV_FALSE;
static struct sockaddr_in    s_tServerName = { 0 };

/* stub versions of real init/de-init functions to make linker happy */

UTV_RESULT UtvPlatformInternetSSL_Init( void )
{
    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSL_Deinit( void )
{
    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSLOpen( char *pszHost, UTV_UINT32 iPort, UTV_UINT32 uiReadTimeout )
{
    UTV_RESULT          rStatus;

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

UTV_RESULT UtvPlatformInternetSSLSessionOpen( UTV_UINT32 *phSession )
{
    int fd = -1;
    fd_set fds;
    struct timeval tv;
    int ret;
    socklen_t len = sizeof( ret );

    if ( !phSession )
        return UTV_INTERNET_PERS_FAILURE;
    *phSession = 0;

    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSendRequest() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* allocate a new socket */
    if ( -1 == (fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
    {
        UtvMessage( UTV_LEVEL_ERROR, " socket() fails!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* Set socket to non-blocking */
    if ( -1 == fcntl( fd, F_SETFL, O_NONBLOCK ))
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, " fcntl() fails!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* Initiate the connection process, this will return immediately */
    if ( -1 == connect( fd, (struct sockaddr*) &s_tServerName, sizeof(s_tServerName)))
    {
        if ( errno != EINPROGRESS )
        {
            close( fd );
            UtvMessage( UTV_LEVEL_ERROR, " connect() fails!" );
            return UTV_INTERNET_PERS_FAILURE;
        }
    }

	if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
	{
		close( fd );
		return UTV_ABORT;
	}

	/* Wait for writeability of the socket to determine success or failure on connect */
    FD_ZERO( &fds );
    FD_SET( fd, &fds );
    tv.tv_sec = UTV_PLATFORM_INTERNET_CONNECT_TIMEOUT_SEC;
    tv.tv_usec = 0;
    ret = select( fd+1, NULL, &fds, NULL, &tv );
    if ( ret < 1 )
    {
        close( fd );
        if ( ret < 0 )
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionOpen() select() error" );
        else
            UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionOpen() timeout waiting for data" );

        return UTV_INTERNET_CRYPT_CNCT_TIMEOUT;
    }

    /* At this point the connect has completed, but we still need to check for success */
    if ( -1 == getsockopt( fd, SOL_SOCKET, SO_ERROR, &ret, &len ))
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionOpen() getsockopt() failed" );
        return UTV_INTERNET_PERS_FAILURE;
    }
    if ( ret )
    {
        close( fd );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionOpen() failed to connect to host" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    *phSession = fd;
	return ( UtvGetState() == UTV_PUBLIC_STATE_ABORT ) ? UTV_ABORT : UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSLSessionClose( UTV_UINT32 hSession )
{
    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionClose() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* close the specified socket */
    close( hSession );

    return UTV_OK;
}


UTV_RESULT UtvPlatformInternetSSLSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen )
{
    int n;

#ifdef UTV_TRACE_INTERNET_TRANSACTION
    UtvMessage( UTV_LEVEL_INFO, " INTERNET REQUEST: %s", pbTxData );
#endif

    if (0 > (n = write(hSession, pbTxData, uiDataLen )))
    {
        UtvMessage( UTV_LEVEL_ERROR, " socket write() fails!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    return UTV_OK;
}
    
UTV_RESULT UtvPlatformInternetSSLGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned )
{
    struct timeval tv;
    fd_set fds;
    UTV_UINT32    iBytesRead;
    int ret;
    UTV_INT32      retry_count;

    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLGetRequest() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

	if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
		return UTV_ABORT;

	FD_ZERO( &fds );
    FD_SET( hSession, &fds );

    for (retry_count = 0; retry_count < UTV_PLATFORM_INTERNET_READ_RETRY_COUNT; retry_count++)
    {
    tv.tv_sec = UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC;
    tv.tv_usec = 0;
    ret = select( hSession+1, &fds, NULL, NULL, &tv );

    if ( ret < 0 )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLGetRequest() select() error" );
        return UTV_INTERNET_PERS_FAILURE;
    }

        if ( ret > 0 )
        {
            break;
        }

        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLGetRequest() retrying" );
    }

    if ( ret == 0 ) /* multiple timeout exit */
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLGetRequest() timeout waiting for data" );
        return UTV_INTERNET_CRYPT_READ_TIMEOUT;
    }

    /* otherwise we need to actually get the data and parse the HTTP header. */
    iBytesRead = read( hSession, pcbResponseBuffer, iBufferLen );
	if ( iBytesRead < 0 )
    {
        UtvMessage( UTV_LEVEL_ERROR, " UtvPlatformInternetSSLGetResponse socket read() fails!" );
        return UTV_INTERNET_PERS_FAILURE;
    }
	else if ( iBytesRead == 0 )
	{
		return UTV_INTERNET_CONN_CLOSED;
	}
    *piLenReturned = (UTV_UINT32)iBytesRead;    
    
	return ( UtvGetState() == UTV_PUBLIC_STATE_ABORT ) ? UTV_ABORT : UTV_OK;
}



/* UtvPlatformInternetSSLClose()
    Currently this routine just clears a flag which tells the system 
    that it needs to resolve the host name when it communicates next.
*/
UTV_RESULT UtvPlatformInternetSSLClose( )
{
    s_bInternetOpen = UTV_FALSE;    
    return UTV_OK;
}

