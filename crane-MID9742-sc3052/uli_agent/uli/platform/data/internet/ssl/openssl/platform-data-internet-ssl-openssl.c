/* platform-data-src-internet-ssl-openssl.c: Platform adaptation layer
                                             SSl to openSSL mapping

   Copyright (c) 2009-2011 UpdateLogic Incorporated. All rights reserved.

   Contains the following entry points
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
   Bob Taylor      12/02/2011  Replaced pthread_setcancelstate with UtvThreadSetCancelState().
   Jim Muchow      07/05/2011  Add OpenSSL locking mechanisms code
   Jim Muchow      06/29/2011  Add explicit SSL Init/Deinit functions
   Jim Muchow      12/29/2010  Change processing of UtvVerifyCertificateHostName() to allow certs
                               with no alt names
   Jim Muchow      09/03/2010  Add code to close s_sslCtx on UtvPlatformInternetSSLOpen() error exits
   Jim Muchow      08/23/2010  Add code to use the selectMonitor when receiving data
   Jim Muchow      08/23/2010  Add code using pthread_cancelstate() when opening/configuring sockets
   Nathan Ford     07/01/2010  Updated to call UtvCommonDecryptCommonFile to get uli.crt
   Bob Taylor      06/10/2010  Remove superfluous select retry loops.
   Nate Ford       04/28/2010  Removed secure save of root CA cert.  Loaded directly from memory.
   Jim Muchow      04/06/2010  Add retry logic to UtvPlatformInternetSSLSessionOpen() and UtvSSLWait()
   Jim Muchow      03/19/2010  Replace clear channel TIMEOUT errors with new encrypted channel errors
   Seb Emonet      03/15/2010  Added wildcard subdomains support in UtvVerifyCertificateHostName()
   Bob Taylor      11/13/2009  Assume 0 len returned by UtvPlatformInternetSSLGetResponse()
   Bob Taylor      09/14/2009  SSL connect timeout uses variable read timeout.
   Bob Taylor      06/30/2009  Improved SSL cert verify debug.
   Bob Taylor      03/18/2009  Created.
*/

#include "utv.h"
#include <stdio.h>                /* perror() */
#undef __STRICT_ANSI__            /* to enable strcasecmp */
#include <string.h>               /* strcasecmp() */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>               /* read() */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <pthread.h>

/* Local static vars */
static UTV_BOOL             s_bInternetOpen = UTV_FALSE;
/* define SSL initialized states */
#define SSL_INITIALIZED_INTERNALLY      1
#define SSL_UNINITIALIZED               0
#define SSL_INITIALIZED_EXTERNALLY      -1
/* If SSL is not to be initialized internally by the agent, it is defined as  */
/* externally initialized. An external entity may use its own code or the     */
/* initialize and/or de-initialize functions defined here. Note, however, if  */
/* these initialize and/or de-initialize functions are used, that the agent   */
/* will not (it cannot) validate or transition the intialized state variable: */
/* these actions are the responsibility of the external entity.               */
#ifdef UTV_INTERNAL_SSL_INIT
static UTV_INT32            s_iSSL_Initialized = SSL_UNINITIALIZED;
#else
static UTV_INT32            s_iSSL_Initialized = SSL_INITIALIZED_EXTERNALLY;
#endif

/* locally defined variants of the OpenSSL static locking mechanisms */
/* a place to hold the array of mutex locks */
static pthread_mutex_t      *s_sSSL_MutexList = NULL;
/* forward declaration of callbacks for use by OpenSSL */
static void s_SSL_locking_function(int mode, int n, const char *file, int line);
static unsigned long s_SSL_id_function(void);

static struct sockaddr_in   s_tServerName = { 0 };
static SSL_CTX *            s_sslCtx = NULL;
static char                 s_host[ 512 ];
static UTV_UINT32           s_uiReadTimeout = UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC;

static UTV_RESULT UtvVerifyCertificateHostName( X509 *cert );
static UTV_RESULT UtvSSLWait( SSL *ssl, int err );
static int verifySSLCert_callback(int ok, X509_STORE_CTX *ctx);

/* #define UTV_TEST_SSL_VERIFY_TRACE */
static int verifySSLCert_callback(int ok, X509_STORE_CTX *ctx)
{
    int err = X509_STORE_CTX_get_error(ctx);

#ifdef UTV_TEST_SSL_VERIFY_TRACE
    X509 *cert;

    cert = X509_STORE_CTX_get_current_cert(ctx);

    UtvMessage( UTV_LEVEL_INFO, " SSL verifying ok: %d", ok );

    UtvMessage( UTV_LEVEL_INFO, " SSL verifying: %s", X509_NAME_oneline(X509_get_subject_name(cert), 0, 0));
    if ( 0 != err )
        UtvMessage( UTV_LEVEL_ERROR, " SLL ERROR: %d, \"%s\"", err, X509_verify_cert_error_string(err));
    else
        UtvMessage( UTV_LEVEL_INFO, " SLL OK" );
#endif

    /* allow X509_V_ERR_CERT_NOT_YET_VALID error to handle case of no system time */
    if ( X509_V_ERR_CERT_NOT_YET_VALID == err )
        ok = 1;

    return ok;
}

UTV_RESULT UtvPlatformInternetSSLAddCerts( void *pCertStore, UTV_BYTE *pubData, UTV_UINT32 uiSize )
{
    X509_STORE *cert_store = (X509_STORE*)pCertStore;
    UTV_RESULT rStatus = UTV_OK;
    BIO *in = NULL;
    STACK_OF(X509_INFO) *inf = NULL;
    int i = 0;
    int storeAddResult;
    X509_INFO *item = NULL;

    in = BIO_new_mem_buf( pubData, uiSize );
    inf = PEM_X509_INFO_read_bio( in, NULL, NULL, NULL );

    BIO_free( in );

    if ( NULL == inf )
    {
      UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, ( rStatus = UTV_SSL_CERT_LOAD_FAILS ), __LINE__ );
      return rStatus;
    }

    for( i = 0; i < sk_X509_INFO_num( inf ); i++ )
    {
      item = sk_X509_INFO_value( inf, i );

      if ( item->x509 )
      {
        storeAddResult = X509_STORE_add_cert( cert_store, item->x509 );
      }

      if ( item->crl )
      {
        storeAddResult = X509_STORE_add_crl( cert_store, item->crl );
      }
    }

    sk_X509_INFO_pop_free( inf, X509_INFO_free );

    return rStatus;
}

UTV_RESULT UtvPlatformInternetSSLAddCertsFromFile( void *pCertStore, UTV_INT8 *filename )
{
  X509_STORE *cert_store = (X509_STORE*)pCertStore;
  UTV_RESULT rStatus;
  UTV_BYTE *pubData = NULL;
  UTV_UINT32 uiSize = 0;

  /* decrypt into a buffer */
  rStatus = UtvCommonDecryptCommonFile( UtvCEMSSLGetCertFileName(), &pubData, &uiSize );

  if ( UTV_OK != rStatus )
  {
    UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, filename );
    return rStatus;
  }

  rStatus = UtvPlatformInternetSSLAddCerts( cert_store, pubData, uiSize );

  if ( UTV_OK != rStatus )
  {
    UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, filename );
    return rStatus;
  }

  UtvFree( pubData );

  return rStatus;
}

/* functions to initialize and also release (aka de-initialize) OpenSSL */

/* swap around the order of these to enable or not SSL_INIT debugging   */
#define SSL_INIT_DEBUG
#undef SSL_INIT_DEBUG

static void s_SSL_locking_function(int mode, int n, const char *file, int line)
{ 
    if (mode & CRYPTO_LOCK) /* defined in OpenSSL */
        pthread_mutex_lock(&s_sSSL_MutexList[n]);
    else
        pthread_mutex_unlock(&s_sSSL_MutexList[n]);
}

static unsigned long s_SSL_id_function(void)
{ 
    return (unsigned long)pthread_self();
}

UTV_RESULT UtvPlatformInternetSSL_Init( void )
{
    int index;
#ifdef SSL_INIT_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "%s: entry state %d", 
                __FUNCTION__, s_iSSL_Initialized );
#endif
    /* test if OpenSSL is already initialized in/via the agent */
    if ( SSL_INITIALIZED_INTERNALLY == s_iSSL_Initialized )
    {
#ifdef SSL_INIT_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "%s: early exit, already initialized", 
                    __FUNCTION__);
#endif
        return UTV_OK; /* no problem, silently ignore */
    }

    s_sSSL_MutexList = (pthread_mutex_t *)malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    if ( NULL == s_sSSL_MutexList )
    {
        UtvMessage( UTV_LEVEL_ERROR, "%s: unable to allocate s_sSSL_MutexList", 
                    __FUNCTION__ );
        return UTV_NO_MEMORY;
    }

    /* these don't fail */
    SSL_library_init();
    SSL_load_error_strings();

    /* From the book, Network Security with OpenSSL: A common security    */
    /* pitfall is the incorrect seeding of the OpenSSL PRNG.              */
    /*                                                                    */
    /* To set up a good seed, first check the kernel's entropy pool file  */
    /* availability and then that it contains the needed entropy (do a    */
    /* "man urandom" for more details). If these tests fail, use the RAND */
    /* code to RYO (roll your own) entropy.                               */
    /*                                                                    */
    /* Note that the entropy pool file can be used up; provide a way for  */
    /* a developer to just use RYO everytime - or define a different pool */
    /* file.                                                              */

#define UTV_USE_URANDOM_TO_SEED_SSL
#ifdef UTV_USE_URANDOM_TO_SEED_SSL
#define RANDOM_POOL     "/dev/urandom" /* use the file that won't block */
#else
#warning "RANDOM_POOL is undefined"
#define RANDOM_POOL     NULL /* force the RYO option */
#endif
    if ( 0 != RAND_load_file( RANDOM_POOL, 1024 ))
    { 
        char buf[1024];

        /* no file exist or it doesn't contain enough entropy,
           need to roll our own */

        RAND_seed(buf, sizeof(buf));
        while (!RAND_status())
        { 
            int r = rand();
            RAND_seed(&r, sizeof(int));
        }
    }

    for (index = 0; index < CRYPTO_num_locks(); index++)
        pthread_mutex_init(&s_sSSL_MutexList[index], NULL);

    CRYPTO_set_id_callback(s_SSL_id_function);

    CRYPTO_set_locking_callback(s_SSL_locking_function);

    /* SSL now intialized, update state from SSL_UNINITIALIZED */
    /* do not modify SSL_INITIALIZED_EXTERNALLY state          */
    if ( SSL_UNINITIALIZED == s_iSSL_Initialized )
        s_iSSL_Initialized = SSL_INITIALIZED_INTERNALLY;
#ifdef SSL_INIT_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "%s: exit state %d", 
                __FUNCTION__, s_iSSL_Initialized);
#endif
    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSL_Deinit( void )
{
    int index;
#ifdef SSL_INIT_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "%s: entry state %d", 
                __FUNCTION__, s_iSSL_Initialized );
#endif
    /* don't allow the rug to be pulled out from under an active Context */
    if ( s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "%s: s_bInternetOpen is true", 
                    __FUNCTION__ );
        return UTV_ALREADY_OPEN;
    }

    /* test if OpenSSL was already de-initialized in/via the agent */
    if ( SSL_UNINITIALIZED == s_iSSL_Initialized )
    {
#ifdef SSL_INIT_DEBUG
        UtvMessage( UTV_LEVEL_INFO, "%s: early exit, already uninitialized", 
                    __FUNCTION__);
#endif
        return UTV_OK; /* no problem, silently ignore */
    }

    /* start cleaning up */
    for (index = 0; index < CRYPTO_num_locks(); index++)
        pthread_mutex_destroy(&s_sSSL_MutexList[index]);

    free(s_sSSL_MutexList);

    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);

    RAND_cleanup();
    ERR_free_strings();

    /* SSL now de-intialized, update state from SSL_INITIALIZED_INTERNALLY */
    /* do not modify SSL_INITIALIZED_EXTERNALLY state                      */
    if ( SSL_INITIALIZED_INTERNALLY == s_iSSL_Initialized )
        s_iSSL_Initialized = SSL_UNINITIALIZED;
#ifdef SSL_INIT_DEBUG
    UtvMessage( UTV_LEVEL_INFO, "%s: exit state %d", 
                __FUNCTION__, s_iSSL_Initialized);
#endif
    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSLOpen( char *pszHost, UTV_UINT32 iPort, UTV_UINT32 uiReadTimeout )
{
    SSL_METHOD *sslMethod = NULL;
    UTV_RESULT  rStatus;

    /* we only need to so this once */
    if ( s_bInternetOpen )
    {
        return UTV_OK;
    }

    /* need a SSL context, ensure that OpenSSL is even initialized */
    if ( SSL_UNINITIALIZED == s_iSSL_Initialized )
    {
        rStatus = UTV_SSL_INIT_FAILS;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        return rStatus;
    }

    if ( !( sslMethod = TLSv1_client_method() ))
    {
        rStatus = UTV_SSL_INIT_FAILS;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        return rStatus;
    }

    if ( !(s_sslCtx = SSL_CTX_new( sslMethod ) ))
    {
        rStatus = UTV_SSL_INIT_FAILS;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        return rStatus;
    }

    rStatus = UtvPlatformInternetSSLAddCertsFromFile( s_sslCtx->cert_store, UtvCEMSSLGetCertFileName() );

    if ( UTV_OK != rStatus )
    {
        SSL_CTX_free( s_sslCtx );
        s_sslCtx = NULL;
        return rStatus;
    }

    SSL_CTX_set_verify( s_sslCtx, SSL_VERIFY_PEER, verifySSLCert_callback );

    strncpy( s_host, pszHost, sizeof( s_host ));

    if ( UTV_OK != ( rStatus = UtvPlatformInternetCommonDNS( pszHost, &s_tServerName.sin_addr )))
    {
        SSL_CTX_free( s_sslCtx );
        s_sslCtx = NULL;
        UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, pszHost );
        return rStatus;
    }

    s_tServerName.sin_family = AF_INET;
    s_tServerName.sin_port = htons((int)iPort);

    /* set the passed in read timeout */
    s_uiReadTimeout = uiReadTimeout;

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
    SSL *ssl = NULL;
    X509 *server_cert = NULL;
    UTV_RESULT rStatus = UTV_OK;
    UTV_INT32 old_cancel_state;

    if ( !phSession )
        return UTV_INTERNET_PERS_FAILURE;

    if ( !s_bInternetOpen )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformInternetSSLSessionOpen() connection not open!" );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* allocate a new socket */
    if ( -1 == (fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)))
        return UTV_INTERNET_PERS_FAILURE;

    UtvThreadSetCancelState(UTV_PTHREAD_CANCEL_DISABLE, &old_cancel_state); 

    /* Set socket to non-blocking */
    if ( -1 == fcntl( fd, F_SETFL, O_NONBLOCK ))
    {
        close( fd );
        rStatus = UTV_INTERNET_PERS_FAILURE;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return rStatus;
    }

    /* Initiate the connection process, this will return immediately */
    if ( -1 == connect( fd, (struct sockaddr*) &s_tServerName, sizeof(s_tServerName)))
    {
        if ( errno != EINPROGRESS )
        {
            close( fd );
            rStatus = UTV_INTERNET_PERS_FAILURE;
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
            UtvThreadSetCancelState(old_cancel_state,  NULL); 
            return rStatus;
        }
    }

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
    {
        close( fd );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_ABORT;
    }

    /* Wait for writeability of the socket to determine success or failure on connect */
    FD_ZERO( &fds );
    FD_SET( fd, &fds );

    /* connect timeout borrows read timeout so that it is large during initial connection and tighter during normal operation */
    tv.tv_sec = s_uiReadTimeout;
    tv.tv_usec = 0;

    UtvActivateSelectMonitor(&tv, fd);
    ret = select( fd+1, NULL, &fds, NULL, &tv );
    UtvDeactivateSelectMonitor();

    if ( ret < 0 )
    {
        close( fd );
        rStatus = UTV_INTERNET_PERS_FAILURE;
        UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, errno );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return rStatus;
    }

    if ( ret == 0 ) /* timeout exit */
    {
        close( fd );
        rStatus = UTV_INTERNET_CRYPT_CNCT_TIMEOUT;
        UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, ret );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_CRYPT_CNCT_TIMEOUT;
    }

    /* At this point the connect has completed, but we still need to check for success */
    if ( -1 == getsockopt( fd, SOL_SOCKET, SO_ERROR, &ret, &len ))
    {
        close( fd );
        rStatus = UTV_INTERNET_PERS_FAILURE;
        UtvLogEventOneDecNum( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, errno );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return rStatus;
    }
    if ( ret )
    {
        close( fd );
        rStatus = UTV_INTERNET_PERS_FAILURE;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* Create a new SSL session */
    if ( NULL == (ssl = SSL_new( s_sslCtx )) )
    {
        close( fd );
        rStatus = UTV_INTERNET_PERS_FAILURE;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 

        return UTV_SSL_INIT_FAILS;
    }

    /* Connect SSL */
    SSL_set_fd( ssl, fd );
    while (( ret = SSL_connect( ssl )) < 0 )
    {
        rStatus = UtvSSLWait( ssl, ret );
        if ( rStatus != UTV_OK )
        {
            SSL_free( ssl );
            close( fd );
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
            UtvThreadSetCancelState(old_cancel_state,  NULL); 
            return rStatus;
        }
    }
    if ( ret == 0 )
    {
        SSL_free( ssl );
        close( fd );
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_SSL_CONNECT_FAILS;
    }

    /* Because we set SSL_CTX_set_verify to SSL_VERIFY_PEER, we will probably never get this far if it really did fail */
    if ( X509_V_OK != (ret = SSL_get_verify_result( ssl )))
    {
        /* tolerate cert not valid yet because system time may not be set */
        if ( X509_V_ERR_CERT_NOT_YET_VALID != ret )
        {
            UtvLogEventString( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__, (UTV_INT8 *) X509_verify_cert_error_string(ret) );
            close( fd );
            SSL_free( ssl );
            UtvThreadSetCancelState(old_cancel_state,  NULL); 
            return UTV_SSL_HOST_CERT_FAILS;
        }
    }

    /* Snag the servers certificate so we can check that it is for who we think it should be for */
    server_cert = SSL_get_peer_certificate( ssl );
    if ( !server_cert )
    {
        close( fd );
        SSL_free( ssl );
        rStatus = UTV_SSL_NO_HOST_CERT;
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_SSL_NO_HOST_CERT;
    }

    rStatus = UtvVerifyCertificateHostName( server_cert );
    X509_free (server_cert);

    if ( rStatus != UTV_OK )
    {
        close( fd );
        SSL_free( ssl );
        /* event logging is in UtvVerifyCertificateHostName */
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return rStatus;
    }

    if ( UTV_PUBLIC_STATE_ABORT == UtvGetState() )
    {
        close( fd );
        SSL_free( ssl );
        UtvThreadSetCancelState(old_cancel_state,  NULL); 
        return UTV_ABORT;
    }

    *phSession = (UTV_UINT32)ssl;
    UtvThreadSetCancelState(old_cancel_state,  NULL); 
    return UTV_OK;
}

UTV_RESULT UtvVerifyCertificateHostName( X509 *cert )
{
    char commonName[ 256 ];
    STACK_OF(GENERAL_NAME) *altNames = NULL;
    const GENERAL_NAME *name = NULL;
    const char *asn = NULL;
    UTV_BOOL matched = UTV_FALSE;
    int count = 0;
    int i, length, lengthDomain = 0;
    UTV_RESULT  rStatus;
    char *s_hostDomain = NULL;
    int crit;

    if ( !cert )
        return UTV_INTERNET_PERS_FAILURE;

    altNames = X509_get_ext_d2i( cert, NID_subject_alt_name, &crit, NULL );
    if ( !altNames )
    {
        rStatus = UTV_SSL_NO_ALT_NAMES;
        UtvLogEventOneHexNum( UTV_LEVEL_INFO, UTV_CAT_SSL, rStatus, __LINE__, crit );
#undef UTV_SSL_NO_ALT_NAMES_IS_FATAL
#ifdef UTV_SSL_NO_ALT_NAMES_IS_FATAL
        return rStatus;
#endif
    }

    length = (int)strlen( s_host );
    /* extract domain name*/
    s_hostDomain = strchr( s_host, '.' );
    if( s_hostDomain )
    {
        lengthDomain = (int)strlen( ++s_hostDomain );
    }
#ifdef UTV_TEST_SSL_VERIFY_TRACE
    UtvMessage( UTV_LEVEL_INFO,"s_host = %s",s_host );
    UtvMessage( UTV_LEVEL_INFO,"s_hostDomain = %s",s_hostDomain );
#endif
    if ( NULL != altNames ) /* count is preset above to 0 */
      count = sk_GENERAL_NAME_num( altNames );
    for ( i=0; i < count; i++ )
    {
        name = sk_GENERAL_NAME_value( altNames, i );
#ifdef UTV_TEST_SSL_VERIFY_TRACE
        UtvMessage( UTV_LEVEL_INFO,"altNames[%d] = %s", i, (char *)ASN1_STRING_data( name->d.ia5 ) );
#endif
        if ( name->type != GEN_DNS )
        {
#ifdef UTV_TEST_SSL_VERIFY_TRACE
            UtvMessage( UTV_LEVEL_ERROR, "name->type != GEN_DNS" );
#endif
            continue;
        }

        if ( (length != ASN1_STRING_length( name->d.ia5 )) && (lengthDomain != ASN1_STRING_length( name->d.ia5 )) )
        {

#ifdef UTV_TEST_SSL_VERIFY_TRACE
            UtvMessage( UTV_LEVEL_ERROR, "length != ASN1_STRING_length( name->d.ia5 ): %d != %d", length, ASN1_STRING_length( name->d.ia5 ) );
#endif
            continue;
        }

        asn = (char *)ASN1_STRING_data( name->d.ia5 );
        if ( (0 == strcasecmp( s_host, asn )) || (0 == strcasecmp( s_hostDomain, asn )) )
        {
            matched = UTV_TRUE;
            break;
        }

#ifdef UTV_TEST_SSL_VERIFY_TRACE
        UtvMessage( UTV_LEVEL_ERROR, "compare: %s, %s", s_host, asn );
#endif
    }

    if ( NULL != altNames ) /* not sure this matters, but... */
      GENERAL_NAMES_free( altNames );

    if ( matched )
    {
#ifdef UTV_TEST_SSL_VERIFY_TRACE
        UtvMessage( UTV_LEVEL_INFO, "alt names OK: %s", s_host );
#endif
        return UTV_OK;
    }

    /* Check the common name next if we didn't find it in the alt names */
    if ( length == X509_NAME_get_text_by_NID( X509_get_subject_name( cert ), NID_commonName, commonName, sizeof( commonName )))
    {
        if ( 0 == strcasecmp( commonName, s_host ))
            return UTV_OK;
    }

    rStatus = UTV_SSL_NO_CN;
    UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
    return rStatus;
}

UTV_RESULT UtvPlatformInternetSSLSessionClose( UTV_UINT32 hSession )
{
    SSL *ssl = (SSL *)hSession;

    if ( !s_bInternetOpen )
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, UTV_INTERNET_PERS_FAILURE, __LINE__ );
        return UTV_INTERNET_PERS_FAILURE;
    }

    /* close the specified socket */
    close( SSL_get_fd( ssl ) );
    SSL_free( ssl );

    return UTV_OK;
}


UTV_RESULT UtvPlatformInternetSSLSendRequest( UTV_UINT32 hSession, char *pbTxData, UTV_UINT32 uiDataLen )
{
    UTV_RESULT rStatus;
    int ret;

#ifdef UTV_TRACE_INTERNET_TRANSACTION
    UtvMessage( UTV_LEVEL_INFO, " INTERNET REQUEST: %s", pbTxData );
#endif

    while (( ret = SSL_write((SSL *)hSession, pbTxData, uiDataLen )) < 0 )
    {
        rStatus = UtvSSLWait( (SSL *)hSession, ret );
        if ( rStatus != UTV_OK )
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
            return rStatus;
        }
    }
    if ( ret != uiDataLen )
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, UTV_INTERNET_PERS_FAILURE, __LINE__ );
        return UTV_INTERNET_PERS_FAILURE;
    }

    return UTV_OK;
}

UTV_RESULT UtvPlatformInternetSSLGetResponse( UTV_UINT32 hSession, char *pcbResponseBuffer, UTV_UINT32 iBufferLen, UTV_UINT32 *piLenReturned )
{
    UTV_RESULT rStatus;
    int ret;

    /* assume no data returned */
    *piLenReturned = 0;

    if ( !s_bInternetOpen )
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, UTV_INTERNET_PERS_FAILURE, __LINE__ );
        return UTV_INTERNET_PERS_FAILURE;
    }

    while (( ret = SSL_read((SSL *)hSession, pcbResponseBuffer, iBufferLen )) < 0 )
    {
        rStatus = UtvSSLWait( (SSL *)hSession, ret );
        if ( rStatus != UTV_OK )
        {
            UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, rStatus, __LINE__ );
            return rStatus;
        }
    }
    if ( ret == 0 )
    {
        UtvLogEvent( UTV_LEVEL_ERROR, UTV_CAT_SSL, UTV_INTERNET_CONN_CLOSED, __LINE__ );
        return UTV_INTERNET_CONN_CLOSED;
    }

    *piLenReturned = ret;
    return UTV_OK;
}

/* UtvPlatformInternetSSLClose()
    Currently this routine just clears a flag which tells the system
    that it needs to resolve the host name when it communicates next.
*/
UTV_RESULT UtvPlatformInternetSSLClose( )
{
    if ( s_sslCtx )
    {
        SSL_CTX_free( s_sslCtx );
        s_sslCtx = NULL;
    }

    s_bInternetOpen = UTV_FALSE;
    return UTV_OK;
}

UTV_RESULT UtvSSLWait( SSL *ssl, int err )
{
    struct timeval tv;
    fd_set fds;
    int fd = SSL_get_fd( ssl );
    int ret, eType;

    if ( UtvGetState() == UTV_PUBLIC_STATE_ABORT )
        return UTV_ABORT;

    eType = SSL_get_error( ssl, err );
    if ( SSL_ERROR_NONE == eType )
        return UTV_OK; /* this is slightly different from the original in that it doesn't
                          recheck UtvGetState() - how much different could it be after the
                          SSL_get_error() call? */
    if (SSL_ERROR_SSL == eType )
        return UTV_SSL_HOST_CERT_FAILS;
    if (( SSL_ERROR_WANT_READ != eType) &&
        ( SSL_ERROR_WANT_WRITE != eType))
        return UTV_INTERNET_PERS_FAILURE;

    FD_ZERO( &fds );
    FD_SET( fd, &fds );

    tv.tv_sec = s_uiReadTimeout; /* actually set to UTV_PLATFORM_INTERNET_READ_TIMEOUT_SEC */
    tv.tv_usec = 0;

    UtvActivateSelectMonitor(&tv, fd);
    ret = select( fd+1, &fds, NULL, NULL, &tv );
    UtvDeactivateSelectMonitor( );

    if ( ret < 0 )
        return UTV_INTERNET_PERS_FAILURE;

    if ( 0 == ret )
    {
        if (SSL_ERROR_WANT_WRITE == eType )
            return UTV_INTERNET_CRYPT_WRITE_TIMEOUT;

        return UTV_INTERNET_CRYPT_READ_TIMEOUT;
    }

    return ( UtvGetState() == UTV_PUBLIC_STATE_ABORT ) ? UTV_ABORT : UTV_OK;
}
