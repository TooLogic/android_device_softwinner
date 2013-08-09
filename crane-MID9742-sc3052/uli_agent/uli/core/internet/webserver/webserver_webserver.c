/*
	webserver.c

	Example stand-alone gSOAP Web server based on the gSOAP HTTP GET plugin.
	This is a small but fully functional (embedded) Web server for serving
	static and dynamic pages and SOAP/XML responses.

--------------------------------------------------------------------------------
gSOAP XML Web services tools
Copyright (C) 2001-2009, Robert van Engelen, Genivia, Inc. All Rights Reserved.
This software is released under one of the following two licenses:
GPL or Genivia's license for commercial use.
--------------------------------------------------------------------------------
GPL license.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA

Author contact information:
engelen@genivia.com / engelen@acm.org
--------------------------------------------------------------------------------
A commercial use license is available from Genivia, Inc., contact@genivia.com
--------------------------------------------------------------------------------

	The Web server handles HTTP GET requests to serve pages and HTTP POST
	reguests to handle SOAP/XML messages. This example only implements
	a simple calculator XML Web service for demonstration purposes (the
	service responds with SOAP/XML).

	This application requires Zlib and Pthreads (you can replace Pthreads
	with another thread library, but you need to study the OpenSSL thread
	changes in the OpenSSL documentation).

	On Unix/Linux, please enable SIGPIPE handling, see main function below.
	SIGPIPE handling will avoid your server from termination when sockets
	are disconnected by clients before the transaction was completed
	(aka broken pipe).

	Use (HTTP GET):
	Compile the web server as explained above
	Start the web server on an even numbered port (e.g. 8080):
	> webserver 8080 &
	Start a web browser and open a (localhost) location:
	http://127.0.0.1:8080
	and type userid 'admin' and passwd 'guest' to gain access
	Open the location:
	http://127.0.0.1:8080/calc.html
	then enter an expression
	Open the locations:
	http://127.0.0.1:8080/test.html
	http://127.0.0.1:8081/webserver.wsdl

	Use (HTTPS GET):
	Create the SSL certificate (see samples/ssl README and scripts)
	Compile the web server with OpenSSL as explained above
	Start the web server on an odd numbered port (e.g. 8081)
	> webserver 8081 &
	Actually, you can start two servers, one on 8080 and a secure one on
	8081
	Start a web browser and open a (localhost) location:
	https://127.0.0.1:8081
	and type userid 'admin' and passwd 'guest' to gain access
	Open the location:
	https://127.0.0.1:8081/calc.html
	and enter an expression
	Open the locations:
	https://127.0.0.1:8081/test.html
	https://127.0.0.1:8081/webserver.wsdl

	Use (HTTP POST):
	Serves SOAP/XML calculation requests
*/

#include "utv.h"
#include "webserver_webserviceH.h"
#include "webserver_webservice.nsmap"
#include "httpget.h"
#include "httpform.h"
#include "logging.h"
#include "threads.h"
#include "webserver_webserver.h"
#include "httpda.h" 	/* optionally enable HTTP Digest Authentication */
#include <signal.h>	/* defines SIGPIPE */

#define BACKLOG (100)

#define AUTH_REALM "gSOAP Web Server Admin"
#define AUTH_USERID "admin"	/* user ID to access admin pages */
#define AUTH_PASSWD "guest"	/* user pw to access admin pages */


/*
 **********************************************************************************************
 *
 * STATIC DEFINES
 *
 **********************************************************************************************
 */

#define MAX_THR (100)
#define MAX_QUEUE (1000)

struct PostHandler
{
  struct PostHandler* next;
  UTV_BYTE* end_point;
  int (*post_handler)(struct soap* soap);
};

struct option
{
  const char *name;             /* name */
  const char *selections;       /* NULL (option does not require argument),
                                   or one-word description,
                                   or selection of possible values (separated by spaces) */
  int selected;                 /* >=0: current selection in selections list */
  char *value;                  /* parsed value (when 'selections' is one name) */
};

#define OPTION_z        0
#define OPTION_c        1
#define OPTION_k        2
#define OPTION_i        3
#define OPTION_v        4
#define OPTION_o        5
#define OPTION_t        6
#define OPTION_s        7
#define OPTION_l        8 
#define OPTION_port     9



/*
 **********************************************************************************************
 *
 * STATIC VARS
 *
 **********************************************************************************************
 */

static int poolsize = 0;

static int queue[MAX_QUEUE];
static int head = 0, tail = 0;

static MUTEX_TYPE queue_cs;
static COND_TYPE queue_cv;

static const struct option *g_options = NULL;
static int secure = 0;		/* =0: no SSL, =1: support SSL */

static struct PostHandler* s_handler_list = NULL;

static const struct option default_options[] =
  { /* "key.fullname", "unit or selection list", text field width, "default value"  */
    { "z.compress", NULL, 0},
    { "c.chunking", NULL, 0},
    { "k.keepalive", NULL, 0},
    { "i.iterative", NULL, 0},
    { "v.verbose", NULL, 0},
    { "o.pool", "threads", 6, "none"},
    { "t.ioTimeout", "seconds", 6, "5"},
    { "s.serverTimeout", "seconds", 6, "0"},
    { "l.logging", "none inbound outbound both", },
    { "", "port", },/* takes the rest of command line args */
    { NULL },/* must be NULL terminated */
  };

static UTV_THREAD_HANDLE s_webserver_worker_thread = 0;



/*
 **********************************************************************************************
 *
 * STATIC FUNCTIONS
 *
 **********************************************************************************************
 */

void* webserver_worker( void* pArg );
int webserver_run( const struct option* options );
void server_loop(struct soap* soap);
void* process_request(void* soap);	/* multi-threaded request handler */
void* process_queue(void* soap);	/* multi-threaded request handler for pool */
int enqueue(SOAP_SOCKET sock);
SOAP_SOCKET dequeue();
int http_get_handler(struct soap* soap);	/* HTTP get handler */
int http_form_handler(struct soap* soap);	/* HTTP form handler */
int check_authentication(struct soap* soap);	/* HTTP authentication check */
int copy_file(struct soap* soap, const char* name, const char* type);	/* copy file as HTTP response */



/**
 **********************************************************************************************
 *
 * Initialize the web server and start the web server worker thread.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerInit( void )
{
  if ( 0 == s_webserver_worker_thread )
  {
    UtvThreadCreate( &s_webserver_worker_thread, &webserver_worker, NULL );
  }
}



/**
 **********************************************************************************************
 *
 * Stop the web server worker thread and shutdown the web server.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerShutdown( void )
{
  /* TODO: Fill in the necessary shutdown activities */
}



/**
 **********************************************************************************************
 *
 * Register a POST handler for a web server endpoint.  This method will look in the existing
 * handler list to determine if this endpoint is already registered.
 *
 * @param     end_point - Name of the web server endpoint.
 *
 * @param     post_handler - Function pointer to execute for the given command.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvWebServerRegisterPostHandler(
  UTV_BYTE* end_point,
  int (*post_handler)(struct soap* soap) )
{
  struct PostHandler* handler;
  UTV_UINT32 end_point_length = UtvStrlen( (UTV_BYTE*)end_point );

  /*
   * Search through the existing handler list for the given endpoint.  If the endpoint is
   * found then change the function pointer if it is different.  If the endpoint is not found
   * then add it to the list.
   */

  handler = s_handler_list;

  while ( handler != NULL )
  {
    if ( !UtvMemcmp( end_point, handler->end_point, UtvStrlen( handler->end_point ) ) )
    {
      break;
    }

    handler = handler->next;
  }

  if ( handler != NULL )
  {
    /*
     * The endpoint is already registered, check if the handler is different.
     */

    if ( handler->post_handler != post_handler )
    {
      UtvMessage( UTV_LEVEL_INFO,
                  "%s: Endpoint %s already registered, updating function pointer",
                  __FUNCTION__,
                  end_point );

      handler->post_handler = post_handler;
    }
    else
    {
      UtvMessage( UTV_LEVEL_INFO,
                  "%s: Endpoint %s already registered",
                  __FUNCTION__,
                  end_point );
    }
  }
  else
  {
    UtvMessage( UTV_LEVEL_INFO,
                "%s: Registering endpoint %s",
                __FUNCTION__,
                end_point );

    handler = UtvMalloc( sizeof(struct PostHandler) );

    handler->end_point = UtvMalloc( end_point_length );
    UtvStrnCopy( handler->end_point,
                 end_point_length,
                 end_point,
                 end_point_length );

    handler->post_handler = post_handler;

    handler->next = s_handler_list;
    s_handler_list = handler;
  }
}



/**
 **********************************************************************************************
 *
 * Worker thread to poll and process tasks from the NetReady Services while a live support
 * session is active.
 *
 * @param     pArg - Thread arguments (none expected).
 *
 * @return    NULL
 *
 * @warning   Thread calling webserver_run() doesn't return until the web server is stopped.
 *
 **********************************************************************************************
 */

void* webserver_worker( void* pArg )
{
  webserver_run( default_options );
  return ( NULL );
}



/**
 **********************************************************************************************
 *
 * Run the web server.
 *
 * @param     options - Startup options for the web server.
 *
 * @warning   Thread calling run_webserver() doesn't return until the web server is stopped.
 *
 * webserver_run() parameter is an array of struct option objects.
 *
 * The objects in the array are according to the order of the OPTION_ defines
 * below the struct definition.
 *
 * Options are:
 *
 *       -z              enables compression
 *       -c              enables chunking
 *       -k              enables keep-alive
 *       -i              enables non-threaded iterative server
 *       -v              enables verbose mode
 *       -o<num>         pool of <num> threads (cannot be used with option -i)
 *                       Note: interactive chunking/keep-alive settings cannot be
 *                       changed, unless the number of threads is interactively
 *                       changed to restart the pool
 *                       Note: <num>=0 specifies unlimited threads
 *       -t<num>         sets I/O timeout value (seconds)
 *       -s<num>         sets server timeout value (seconds)
 *       -l[none inbound outbound both]
 *                       enables logging
 *
 **********************************************************************************************
 */

int webserver_run( const struct option* options )
{
  struct soap soap;
  SOAP_SOCKET master;
  int port = 0;

  g_options = options;

  if (g_options[OPTION_port].value)
    port = atol(g_options[OPTION_port].value);
  if (!port)
    port = 8080;
  fprintf(stderr, "Starting Web server on port %d\n", port);
  /* if the port is an odd number, the Web server uses HTTPS only */
  if (port % 2)
    secure = 1;
  if (secure)
    fprintf(stderr, "[Note: https://127.0.0.1:%d/test.html to test the server from browser]\n", port);
  else
    fprintf(stderr, "[Note: http://127.0.0.1:%d/test.html to test the server from browser]\n", port);
  fprintf(stderr, "[Note: http://127.0.0.1:%d for settings, login: '"AUTH_USERID"' and '"AUTH_PASSWD"']\n", port);
  fprintf(stderr, "[Note: you should enable Linux/Unix SIGPIPE handlers to avoid broken pipe]\n");

  soap_init2(&soap, SOAP_IO_KEEPALIVE, SOAP_IO_DEFAULT);

  /*
   * gSOAP generation was done WITH_NONAMESPACES so they need to be setup by hand.  The web
   * server uses "webservice_namespaces" defined in webservice.nsmap.
   */

  soap_set_namespaces( &soap, webserver_webservice_namespaces );

#ifdef WITH_OPENSSL
  /* SSL (to enable: compile all sources with -DWITH_OPENSSL) */
  if (secure && soap_ssl_server_context(&soap,
    SOAP_SSL_DEFAULT,
    "server.pem",	/* keyfile: see SSL docs on how to obtain this file */
    "password",		/* password to read the key file */
    NULL, 		/* cacert */
    NULL,		/* capath */
    "dh512.pem",	/* DH file, if NULL use RSA */
    NULL,		/* if randfile!=NULL: use a file with random data to seed randomness */ 
    "webserver"		/* server identification for SSL session cache (must be a unique name) */
  ))
  {
    soap_print_fault(&soap, stderr);
    exit(1);
  }
#endif
  /* Register HTTP GET plugin */
  if (soap_register_plugin_arg(&soap, http_get, (void*)http_get_handler))
    soap_print_fault(&soap, stderr);
  /* Register HTTP POST plugin */
  if (soap_register_plugin_arg(&soap, http_form, (void*)http_form_handler))
    soap_print_fault(&soap, stderr);
  /* Register logging plugin */
  if (soap_register_plugin(&soap, logging))
    soap_print_fault(&soap, stderr);
#ifdef HTTPDA_H
  /* Register HTTP Digest Authentication plugin */
  if (soap_register_plugin(&soap, http_da))
    soap_print_fault(&soap, stderr);
#endif
  /* Unix SIGPIPE, this is OS dependent (win does not need this) */
  /* soap.accept_flags = SO_NOSIGPIPE; */ 	/* some systems like this */
  /* soap.socket_flags = MSG_NOSIGNAL; */	/* others need this */
  /* and some older Unix systems may require a sigpipe handler */
  master = soap_bind(&soap, NULL, port, BACKLOG);
  if (!soap_valid_socket(master))
  {
    soap_print_fault(&soap, stderr);
    exit(1);
  }
  fprintf(stderr, "Port bind successful: master socket = %d\n", master);
  MUTEX_SETUP(queue_cs);
  COND_SETUP(queue_cv);
  server_loop(&soap);
  MUTEX_CLEANUP(queue_cs);
  COND_CLEANUP(queue_cv);
  soap_end(&soap);
  soap_done(&soap);
  THREAD_EXIT;
  return 0;
}



/**
 **********************************************************************************************
 *
 *
 *
 **********************************************************************************************
 */

void server_loop(struct soap *soap)
{
  struct soap *soap_thr[MAX_THR];
  THREAD_TYPE tid, tids[MAX_THR];
  int req;

  for (req = 1; ; req++)
  {
    SOAP_SOCKET sock;
    int newpoolsize;
    
    if (g_options[OPTION_c].selected)
      soap_set_omode(soap, SOAP_IO_CHUNK); /* use chunked HTTP content (fast) */
    if (g_options[OPTION_k].selected)
      soap_set_omode(soap, SOAP_IO_KEEPALIVE);
    if (g_options[OPTION_t].value)
      soap->send_timeout = soap->recv_timeout = atol(g_options[OPTION_t].value);
    if (g_options[OPTION_s].value)
      soap->accept_timeout = atol(g_options[OPTION_s].value);
    if (g_options[OPTION_l].selected == 1 || g_options[OPTION_l].selected == 3)
      soap_set_logging_inbound(soap, stdout);
    else
      soap_set_logging_inbound(soap, NULL);
    if (g_options[OPTION_l].selected == 2 || g_options[OPTION_l].selected == 3)
      soap_set_logging_outbound(soap, stdout);
    else
      soap_set_logging_outbound(soap, NULL);

    newpoolsize = atol(g_options[OPTION_o].value);

    if (newpoolsize < 0)
      newpoolsize = 0;
    else if (newpoolsize > MAX_THR)
      newpoolsize = MAX_THR;

    if (poolsize > newpoolsize)
    {
      int job;

      for (job = 0; job < poolsize; job++)
      {
        while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
          sleep(1);
      }

      for (job = 0; job < poolsize; job++)
      {
        fprintf(stderr, "Waiting for thread %d to terminate...\n", job);
        THREAD_JOIN(tids[job]);
        fprintf(stderr, "Thread %d has stopped\n", job);
        soap_done(soap_thr[job]);
        free(soap_thr[job]);
      }

      poolsize = 0;
    }

    if (poolsize < newpoolsize)
    {
      int job;

      for (job = poolsize; job < newpoolsize; job++)
      {
        soap_thr[job] = soap_copy(soap);

        if (!soap_thr[job])
          break;

        soap_thr[job]->user = (void*)job;
	
        fprintf(stderr, "Starting thread %d\n", job);
        THREAD_CREATE(&tids[job], (void*(*)(void*))process_queue, (void*)soap_thr[job]);
      }

      poolsize = job;
    }

    /* to control the close behavior and wait states, use the following:
    soap->accept_flags |= SO_LINGER;
    soap->linger_time = 60;
    */
    /* the time resolution of linger_time is system dependent, though according
     * to some experts only zero and nonzero values matter.
    */

    sock = soap_accept(soap);
    if (!soap_valid_socket(sock))
    {
      if (soap->errnum)
      {
        soap_print_fault(soap, stderr);
        fprintf(stderr, "Retry...\n");
        continue;
      }
      fprintf(stderr, "gSOAP Web server timed out\n");
      break;
    }

    if (g_options[OPTION_v].selected)
      fprintf(stderr, "Request #%d accepted on socket %d connected from IP %d.%d.%d.%d\n", req, sock, (int)(soap->ip>>24)&0xFF, (int)(soap->ip>>16)&0xFF, (int)(soap->ip>>8)&0xFF, (int)soap->ip&0xFF);

    if (poolsize > 0)
    {
      while (enqueue(sock) == SOAP_EOM)
        sleep(1);
    }
    else
    {
      struct soap *tsoap = NULL;

      if (!g_options[OPTION_i].selected)
        tsoap = soap_copy(soap);

      if (tsoap)
      {
#ifdef WITH_OPENSSL
        if (secure && soap_ssl_accept(tsoap))
        {
          soap_print_fault(tsoap, stderr);
          fprintf(stderr, "SSL request failed, continue with next call...\n");
          soap_end(tsoap);
          soap_done(tsoap);
          free(tsoap);
          continue;
        }
#endif
        tsoap->user = (void*)req;
        THREAD_CREATE(&tid, (void*(*)(void*))process_request, (void*)tsoap);
      }
      else
      {
#ifdef WITH_OPENSSL
        if (secure && soap_ssl_accept(soap))
        {
          soap_print_fault(soap, stderr);
          fprintf(stderr, "SSL request failed, continue with next call...\n");
          soap_end(soap);
          continue;
        }
#endif
	/* Keep-alive: frequent EOF faults occur related to KA-timeouts */
        if (webserver_webservice_serve(soap) &&
            (soap->error != SOAP_EOF ||
             (soap->errnum != 0 && !(soap->omode & SOAP_IO_KEEPALIVE))))
        {
          fprintf(stderr, "Request #%d completed with failure %d\n", req, soap->error);
          soap_print_fault(soap, stderr);
        }
        else if (g_options[OPTION_v].selected)
          fprintf(stderr, "Request #%d completed\n", req);

        soap_destroy(soap);
        soap_end(soap);
      }
    }
  }

  if (poolsize > 0)
  {
    int job;

    for (job = 0; job < poolsize; job++)
    {
      while (enqueue(SOAP_INVALID_SOCKET) == SOAP_EOM)
      {
        sleep(1);
      }
    }

    for (job = 0; job < poolsize; job++)
    {
      fprintf(stderr, "Waiting for thread %d to terminate... ", job);
      THREAD_JOIN(tids[job]);
      fprintf(stderr, "terminated\n");
      soap_done(soap_thr[job]);
      free(soap_thr[job]);
    }
  }
}



/**
 **********************************************************************************************
 *
 * Process dispatcher.
 *
 **********************************************************************************************
 */

void *process_request(void *soap)
{
  struct soap *tsoap = (struct soap*)soap;

  THREAD_DETACH(THREAD_ID);

  if (webserver_webservice_serve(tsoap) && (tsoap->error != SOAP_EOF || (tsoap->errnum != 0 && !(tsoap->omode & SOAP_IO_KEEPALIVE))))
  {
    fprintf(stderr, "Thread %d completed with failure %d\n", (int)tsoap->user, tsoap->error);
    soap_print_fault(tsoap, stderr);
  }
  else if (g_options[OPTION_v].selected)
    fprintf(stderr, "Thread %d completed\n", (int)tsoap->user);
  soap_destroy((struct soap*)soap);
  soap_end(tsoap);
  soap_done(tsoap);
  free(soap);

  return NULL;
}



/**
 **********************************************************************************************
 *
 * Thread pool (enabled with option -o<num>).
 *
 **********************************************************************************************
 */

void *process_queue(void *soap)
{
  struct soap *tsoap = (struct soap*)soap;

  for (;;)
  {
    tsoap->socket = dequeue();
    if (!soap_valid_socket(tsoap->socket))
    {
      if (g_options[OPTION_v].selected)
        fprintf(stderr, "Thread %d terminating\n", (int)tsoap->user);
      break;
    }

#ifdef WITH_OPENSSL
    if (secure && soap_ssl_accept(tsoap))
    {
      soap_print_fault(tsoap, stderr);
      fprintf(stderr, "SSL request failed, continue with next call...\n");
      soap_end(tsoap);
      continue;
    }
#endif

    if (g_options[OPTION_v].selected)
      fprintf(stderr, "Thread %d accepted a request\n", (int)tsoap->user);
    if (webserver_webservice_serve(tsoap) && (tsoap->error != SOAP_EOF || (tsoap->errnum != 0 && !(tsoap->omode & SOAP_IO_KEEPALIVE))))
    {
      fprintf(stderr, "Thread %d finished serving request with failure %d\n", (int)tsoap->user, tsoap->error);
      soap_print_fault(tsoap, stderr);
    }
    else if (g_options[OPTION_v].selected)
      fprintf(stderr, "Thread %d finished serving request\n", (int)tsoap->user);
    soap_destroy(tsoap);
    soap_end(tsoap);
  }

  soap_destroy(tsoap);
  soap_end(tsoap);
  soap_done(tsoap);

  return NULL;
}



/**
 **********************************************************************************************
 *
 *
 *
 **********************************************************************************************
 */

int enqueue(SOAP_SOCKET sock)
{
  int status = SOAP_OK;
  int next;
  int ret;

  if ((ret = MUTEX_LOCK(queue_cs)))
    fprintf(stderr, "MUTEX_LOCK error %d\n", ret);

  next = (tail + 1) % MAX_QUEUE;
  if (head == next)
  {
    /* don't block on full queue, return SOAP_EOM */
    status = SOAP_EOM;
  }
  else
  {
    queue[tail] = sock;
    tail = next;

    if (g_options[OPTION_v].selected)
      fprintf(stderr, "enqueue(%d)\n", sock);

    if ((ret = COND_SIGNAL(queue_cv)))
      fprintf(stderr, "COND_SIGNAL error %d\n", ret);
  }

  if ((ret = MUTEX_UNLOCK(queue_cs)))
    fprintf(stderr, "MUTEX_UNLOCK error %d\n", ret);

  return status;
}



/**
 **********************************************************************************************
 *
 *
 *
 **********************************************************************************************
 */

SOAP_SOCKET dequeue()
{
  SOAP_SOCKET sock;
  int ret;

  if ((ret = MUTEX_LOCK(queue_cs)))
    fprintf(stderr, "MUTEX_LOCK error %d\n", ret);

  while (head == tail)
    if ((ret = COND_WAIT(queue_cv, queue_cs)))
      fprintf(stderr, "COND_WAIT error %d\n", ret);

  sock = queue[head];
  
  head = (head + 1) % MAX_QUEUE;

  if (g_options[OPTION_v].selected)
    fprintf(stderr, "dequeue(%d)\n", sock);

  if ((ret = MUTEX_UNLOCK(queue_cs)))
    fprintf(stderr, "MUTEX_UNLOCK error %d\n", ret);

  return sock;
}



/**
 **********************************************************************************************
 *
 * HTTP GET handler for plugin
 *
 **********************************************************************************************
 */

int http_get_handler(struct soap *soap)
{
  /* gSOAP >=2.5 soap_response() will do this automatically for us, when sending SOAP_HTML or SOAP_FILE:
  if ((soap->omode & SOAP_IO) != SOAP_IO_CHUNK)
    soap_set_omode(soap, SOAP_IO_STORE); */ /* if not chunking we MUST buffer entire content when returning HTML pages to determine content length */
#ifdef WITH_ZLIB
  if (g_options[OPTION_z].selected && soap->zlib_out == SOAP_ZLIB_GZIP) /* client accepts gzip */
  { soap_set_omode(soap, SOAP_ENC_ZLIB); /* so we can compress content (gzip) */
    soap->z_level = 9; /* best compression */
  }
  else
    soap_clr_omode(soap, SOAP_ENC_ZLIB); /* so we can compress content (gzip) */
#endif
  /* Use soap->path (from request URL) to determine request: */
  if (g_options[OPTION_v].selected)
    fprintf(stderr, "HTTP GET Request: %s\n", soap->endpoint);
  /* Note: soap->path always starts with '/' */
  if (strchr(soap->path + 1, '/') || strchr(soap->path + 1, '\\'))	/* we don't like snooping in dirs */
    return 403; /* HTTP forbidden */
  if (!soap_tag_cmp(soap->path, "*.html"))
    return copy_file(soap, soap->path + 1, "text/html");
  if (!soap_tag_cmp(soap->path, "*.xml")
   || !soap_tag_cmp(soap->path, "*.xsd")
   || !soap_tag_cmp(soap->path, "*.wsdl"))
    return copy_file(soap, soap->path + 1, "text/xml");
  if (!soap_tag_cmp(soap->path, "*.jpg"))
    return copy_file(soap, soap->path + 1, "image/jpeg");
  if (!soap_tag_cmp(soap->path, "*.gif"))
    return copy_file(soap, soap->path + 1, "image/gif");
  if (!soap_tag_cmp(soap->path, "*.png"))
    return copy_file(soap, soap->path + 1, "image/png");
  if (!soap_tag_cmp(soap->path, "*.ico"))
    return copy_file(soap, soap->path + 1, "image/ico");

  if (!strncmp(soap->path, "/genivia", 8))
  { strcpy(soap->endpoint, "http://genivia.com"); /* redirect */
    strcat(soap->endpoint, soap->path + 8);
    return 307; /* Temporary Redirect */
  }
  /* Check requestor's authentication: */
  if (check_authentication(soap))
    return 401; /* HTTP not authorized */
  return 404; /* HTTP not found */
}



/**
 **********************************************************************************************
 *
 *
 *
 **********************************************************************************************
 */

int check_authentication(struct soap *soap)
{
  if (soap->userid && soap->passwd)
  {
    if (!strcmp(soap->userid, AUTH_USERID) && !strcmp(soap->passwd, AUTH_PASSWD))
    {
      return SOAP_OK;
    }
  }
#ifdef HTTPDA_H
  else if (soap->authrealm && soap->userid)
  {
    if (!strcmp(soap->authrealm, AUTH_REALM) && !strcmp(soap->userid, AUTH_USERID))
    {
      if (!http_da_verify_get(soap, AUTH_PASSWD))
      {
        return SOAP_OK;
      }
    }
  }
#endif

  soap->authrealm = AUTH_REALM;
  return ( 401 );
}



/**
 **********************************************************************************************
 *
 * HTTP POST application/x-www-form-urlencoded handler for plugin
 *
 **********************************************************************************************
 */

int http_form_handler(struct soap *soap)
{
  struct PostHandler* handler;

#ifdef WITH_ZLIB
  if (g_options[OPTION_z].selected && soap->zlib_out == SOAP_ZLIB_GZIP) /* client accepts gzip */
  {
    soap_set_omode(soap, SOAP_ENC_ZLIB); /* so we can compress content (gzip) */
  }

  soap->z_level = 9; /* best compression */
#endif

  /* Use soap->path (from request URL) to determine request: */
  if (g_options[OPTION_v].selected)
  {
    fprintf(stderr, "HTTP POST Request: %s\n", soap->endpoint);
  }

  /* Note: soap->path always starts with '/' */

  handler = s_handler_list;

  while ( handler != NULL )
  {
    if ( !UtvMemcmp( soap->path, handler->end_point, UtvStrlen( handler->end_point ) ) )
    {
      return ( handler->post_handler( soap ) );
    }

    handler = handler->next;
  }

  return 404; /* HTTP not found */
}



/**
 **********************************************************************************************
 *
 * Copy static page
 *
 **********************************************************************************************
 */

int copy_file(struct soap *soap, const char *name, const char *type)
{ FILE *fd;
  size_t r;
  fd = fopen(name, "rb"); /* open file to copy */
  if (!fd)
    return 404; /* return HTTP not found */
  soap->http_content = type;
  if (soap_response(soap, SOAP_FILE)) /* OK HTTP response header */
  { soap_end_send(soap);
    fclose(fd);
    return soap->error;
  }
  for (;;)
  { r = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);
    if (!r)
      break;
    if (soap_send_raw(soap, soap->tmpbuf, r))
    { soap_end_send(soap);
      fclose(fd);
      return soap->error;
    }
  }
  fclose(fd);
  return soap_end_send(soap);
}
