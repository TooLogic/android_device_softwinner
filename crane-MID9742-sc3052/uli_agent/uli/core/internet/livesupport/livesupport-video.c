/**********************************************************************************************
 *                                CONFIDENTIAL AND PROPRIETARY
 *                 Copyright (C) 2010 UpdateLogic, Inc., All Rights Reserved
 *********************************************************************************************/

/**
 **********************************************************************************************
 *
 * Implementation of the video suppport for the LiveSupport interface.
 *
 * @file      livesupport-video.c
 *
 * @author    Chris Nigbur
 *
 * @version   $Revision: 5954 $
 *            $Author: btaylor $
 *            $Date: 2011-12-19 23:11:54 -0500 (Mon, 19 Dec 2011) $
 *
 **********************************************************************************************
 */

#include "pthread.h" /* for the mutex */

#include "utv.h"



/*
 **********************************************************************************************
 *
 * STATIC DEFINES
 *
 **********************************************************************************************
 */

typedef struct
{
  gboolean secureMediaStream;
  gchar relayServerAddress[128];
  gint relayServerPort;
  gint sourceFrameWidth;
  gint sourceFrameHeight;
  gint sourceFramePitch;
  guint sourceBufferSize; /* Result of width * height * pitch */
  gint frameWidth;
  gint frameHeight;
  gint frameRateNumerator;
  gint frameRateDenominator;
  guint pushBufferInterval;
  GMainLoop* loop;
  GstElement* pipeline;
  GstElement* source;
  guint source_id;
  guint num_frame;
} LiveSupportVideoWorkerData;



/*
 **********************************************************************************************
 *
 * STATIC VARS
 *
 **********************************************************************************************
 */

static UTV_THREAD_HANDLE s_videoWorkerThread = 0;
static UTV_BOOL s_videoCapable = UTV_FALSE;
static LiveSupportVideoWorkerData* s_videoWorkerData = NULL;
static UTV_BYTE s_libraryPath[PATH_MAX];

/* to make sure the children play well together: */
static pthread_mutex_t          s_videoEnableMutex;
static UTV_BOOL                 s_videoEnabled;

/*
 **********************************************************************************************
 *
 * STATIC FUNCTIONS
 *
 **********************************************************************************************
 */

static UTV_BOOL validate_gst_functionality( void );
static UTV_BOOL load_gst_plugins( GList** list );
static UTV_BOOL load_gst_plugin( UTV_BYTE* path, UTV_BYTE* fileName, GList** list );
static UTV_BOOL find_gst_factories( GList** list );
static UTV_BOOL find_gst_factory( const char* factoryName, GList** list );
static void* VideoWorker( void* pArg );
static UTV_RESULT live_support_video_start( cJSON* inputs, cJSON* outputs );
static UTV_RESULT live_support_video_end( cJSON* inputs, cJSON* outputs );



/**
 **********************************************************************************************
 *
 * Initialize the video handling for the Live Support sub-system as part of Agent
 * initialization.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportVideoAgentInit()
{
  /*
   * Initialize the static variables associated with video just in case they were left in a
   * bad state during shutdown.
   */

  s_videoCapable = UTV_FALSE;

  /* initialize mutex control structures */
  pthread_mutex_init(&s_videoEnableMutex, NULL);
  s_videoEnabled = UTV_FALSE;

  UtvPlatformLiveSupportVideoGetLibraryPath( s_libraryPath, PATH_MAX );

  /*
   * Initialize GStreamer
   */

  if ( TRUE == gst_init_check( NULL, NULL, NULL ) )
  {
    /*
     * Validate that capabilities of the GStreamer installation to determine if the Agent will
     * be capable of sending video or not.  If the Agent will not be capable of sending video
     * the command handlers for starting video should not be enabled.
     */

    s_videoCapable = validate_gst_functionality();
  }

  if ( UTV_TRUE == s_videoCapable )
  {
    /*
     * Register the command handlers for the commands that are initiated through the task
     * mechanism.
     */

    UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_VIDEO_START",
                                              NULL,
                                              live_support_video_start );

    UtvLiveSupportTaskRegisterCommandHandler( (UTV_BYTE*) "FFFFFF_LIVE_SUPPORT_VIDEO_END",
                                              NULL,
                                              live_support_video_end );
  }
  else
  {
    UtvMessage( UTV_LEVEL_ERROR,
                "%s: Agent is not able to support video functionality, GStreamer not "
                "sufficiently configured.",
                __FUNCTION__ );
  }
}



/**
 **********************************************************************************************
 *
 * Shutdown the video handling for the Live Support sub-system as part of Agent shutdown.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportVideoAgentShutdown( void )
{
  s_videoCapable = UTV_FALSE;

  pthread_mutex_lock(&s_videoEnableMutex);
  s_videoEnabled = UTV_FALSE;
  pthread_mutex_unlock(&s_videoEnableMutex);
  pthread_mutex_destroy(&s_videoEnableMutex);

  if ( NULL != s_videoWorkerData )
  {
    g_free( s_videoWorkerData );
    s_videoWorkerData = NULL;
  }
}



/**
 **********************************************************************************************
 *
 * Initialize the video handling for a Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportVideoSessionInit( void )
{
}



/**
 **********************************************************************************************
 *
 * Shutdown the video handling for a Live Support session.
 *
 * @return    void
 *
 **********************************************************************************************
 */

void UtvLiveSupportVideoSessionShutdown( void )
{
  live_support_video_end( NULL, NULL );
}



/**
 **********************************************************************************************
 *
 * Start the video processing for a Live Support Session.
 *
 * @param     mediaStreamInfo - Information about the media stream to start.
 *
 * @return    UTV_OK if the media stream was successfully started.
 * @return    Other UTV_RESULT if an error occurred.
 *
 **********************************************************************************************
 */

UTV_RESULT UtvLiveSupportVideoSessionStart1( UtvLiveSupportMediaStreamInfo1* mediaStreamInfo )
{
  UTV_RESULT result = UTV_OK;
  double pushBufferInterval;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == mediaStreamInfo )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Grab the lock for phase 1 processing
   */

  pthread_mutex_lock( &s_videoEnableMutex );

  /*
   * Check if video is already running.
   */

  if ( UTV_TRUE == s_videoEnabled )
  {
    /*
     * Video is already enabled at the agent, return UTV_OK since this is not an error.
     */

    UtvMessage( UTV_LEVEL_INFO, "%s: Video already enabled", __FUNCTION__ );
    pthread_mutex_unlock(&s_videoEnableMutex);
    return ( result );
  }

  s_videoEnabled = UTV_TRUE;

  UtvMessage(
    UTV_LEVEL_INFO,
    "%s: Starting %s video using MRS %s:%d",
    __FUNCTION__,
    ( mediaStreamInfo->secureMediaStream ? "secure" : "non-secure" ),
    mediaStreamInfo->mrsAddress,
    mediaStreamInfo->mrsPort );

  s_videoWorkerData = g_new0( LiveSupportVideoWorkerData, 1 );

  s_videoWorkerData->secureMediaStream = mediaStreamInfo->secureMediaStream;

  UtvStrnCopy(
    (UTV_BYTE*) s_videoWorkerData->relayServerAddress,
    MAX_MRS_ADDRESS_LEN,
    mediaStreamInfo->mrsAddress,
    UtvStrlen( mediaStreamInfo->mrsAddress ) );

  s_videoWorkerData->relayServerPort = mediaStreamInfo->mrsPort;
  s_videoWorkerData->frameWidth = mediaStreamInfo->frameWidth;
  s_videoWorkerData->frameHeight = mediaStreamInfo->frameHeight;
  s_videoWorkerData->frameRateNumerator = mediaStreamInfo->frameRateNumerator;
  s_videoWorkerData->frameRateDenominator = mediaStreamInfo->frameRateDenominator;

  /*
   * Calculate the timeout interval when pushing buffers into the source.  This interval
   * should be close to the desired framerate if the time to capture a buffer is not a
   * factor.
   */

  pushBufferInterval = ((double)s_videoWorkerData->frameRateDenominator /
                        (double)s_videoWorkerData->frameRateNumerator);
  pushBufferInterval *= 1000;
  pushBufferInterval -= 5;
  s_videoWorkerData->pushBufferInterval = (guint)pushBufferInterval;

  result = UtvThreadCreate( &s_videoWorkerThread, &VideoWorker, NULL );

  if ( UTV_OK != result )
  {
    UtvMessage(
      UTV_LEVEL_ERROR,
      "%s() Failed to create video worker thread (%d) \"%s\"",
      __FUNCTION__,
      result,
      UtvGetMessageString( result ) );

    g_free( s_videoWorkerData );
    s_videoWorkerData = NULL;
    s_videoEnabled = UTV_FALSE;
  }

  pthread_mutex_unlock( &s_videoEnableMutex );

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Retrieves an indicator as to whether the media stream can be secured.
 *
 * @return    UTV_TRUE if the secure stream is supported.
 * @return    UTV_FALSE if the secure stream is NOT supported.
 *
 **********************************************************************************************
 */

UTV_BOOL UtvLiveSupportVideoSecureMediaStream( void )
{
#ifdef UTV_LIVE_SUPPORT_VIDEO_SECURE
  return ( UTV_TRUE );
#else
  return ( UTV_FALSE );
#endif /* UTV_LIVE_SUPPORT_VIDEO_SECURE */
}



/**
 **********************************************************************************************
 *
 * Post handler for the LIVE_SUPPORT_INITIATE_SESSION command.
 *
 * @param     inputs -
 *
 * @param     outputs -
 *
 * @return    UTV_OK if the command was successful.
 * @return    UTV_UNKNOWN if the command was not successful.
 *
 **********************************************************************************************
 */

static UTV_RESULT live_support_video_start( cJSON* inputs, cJSON* outputs )
{
  UTV_RESULT result = UTV_OK;
  cJSON* output;
  cJSON* secureMediaStream;
  cJSON* relayServerAddress;
  cJSON* relayServerPort;
  cJSON* frameWidth;
  cJSON* frameHeight;
  cJSON* frameRateNumerator;
  cJSON* frameRateDenominator;
  UtvLiveSupportMediaStreamInfo1 mediaStreamInfo;

  /*
   * Validate the parameters and return an error if necessary.
   */

  if ( NULL == inputs || NULL == outputs )
  {
    result = UTV_INVALID_PARAMETER;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  output = cJSON_CreateObject();

  if ( NULL == output )
  {
    result = UTV_NO_MEMORY;
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  cJSON_AddStringToObject( output, "ID", "RESPONSE_DATA" );
  cJSON_AddStringToObject( output, "TYPE", "JSON" );
  cJSON_AddItemToObject( output, "DATA", cJSON_CreateObject() );
  cJSON_AddItemToArray( outputs, output );

  /*
   * Retrieve the flag indicating if the media stream should be secure.
   *
   * NOTE: This can be a missing field and if so by default the stream will default to
   *       unsecured.
   */

  secureMediaStream = cJSON_GetObjectItem( inputs, "SECURE_MEDIA_STREAM" );

  /*
   * Get the rest of the required settings for the stream.
   */

  relayServerAddress = cJSON_GetObjectItem( inputs, "RELAY_SERVER_ADDRESS" );
  relayServerPort = cJSON_GetObjectItem( inputs, "RELAY_SERVER_PORT" );
  frameWidth = cJSON_GetObjectItem( inputs, "FRAME_WIDTH" );
  frameHeight = cJSON_GetObjectItem( inputs, "FRAME_HEIGHT" );
  frameRateNumerator = cJSON_GetObjectItem( inputs, "FRAME_RATE_NUMERATOR" );
  frameRateDenominator = cJSON_GetObjectItem( inputs, "FRAME_RATE_DENOMINATOR" );

  if ( NULL == relayServerAddress ||
       cJSON_String != relayServerAddress->type ||
       MAX_MRS_ADDRESS_LEN < UtvStrlen( (UTV_BYTE*) relayServerAddress->valuestring ) ||
       NULL == relayServerPort ||
       cJSON_Number != relayServerPort->type ||
       NULL == frameWidth ||
       cJSON_Number != frameWidth->type ||
       NULL == frameHeight ||
       cJSON_Number != frameHeight->type ||
       NULL == frameRateNumerator ||
       cJSON_Number != frameRateNumerator->type ||
       NULL == frameRateDenominator ||
       cJSON_Number != frameRateDenominator->type )
  {
    UtvMessage( UTV_LEVEL_ERROR, "%s() \"%s\"", __FUNCTION__, UtvGetMessageString( result ) );
    return ( result );
  }

  /*
   * Copy the information from the 'inputs' into the media stream information structure.
   */

  if ( NULL != secureMediaStream && secureMediaStream->type == cJSON_True )
  {
    mediaStreamInfo.secureMediaStream = UTV_TRUE;
  }
  else
  {
    mediaStreamInfo.secureMediaStream = UTV_FALSE;
  }

  UtvStrnCopy(
    mediaStreamInfo.mrsAddress,
    MAX_MRS_ADDRESS_LEN,
    (UTV_BYTE*) relayServerAddress->valuestring,
    UtvStrlen( (UTV_BYTE*) relayServerAddress->valuestring ) );

  mediaStreamInfo.mrsPort = relayServerPort->valueint;
  mediaStreamInfo.frameWidth = frameWidth->valueint;
  mediaStreamInfo.frameHeight = frameHeight->valueint;
  mediaStreamInfo.frameRateNumerator = frameRateNumerator->valueint;
  mediaStreamInfo.frameRateDenominator = frameRateDenominator->valueint;

  result = UtvLiveSupportVideoSessionStart1( &mediaStreamInfo );

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Post handler for the LIVE_SUPPORT_INITIATE_SESSION command.
 *
 * @param     inputs -
 *
 * @param     outputs -
 *
 * @return    UTV_OK if the command was successful.
 * @return    UTV_UNKNOWN if the command was not successful.
 *
 **********************************************************************************************
 */

static UTV_RESULT live_support_video_end( cJSON* inputs, cJSON* outputs )
{
  GstFlowReturn ret;

  pthread_mutex_lock( &s_videoEnableMutex );

  if ( UTV_TRUE == s_videoEnabled )
  {
    s_videoEnabled = UTV_FALSE;
    g_signal_emit_by_name( GST_APP_SRC( s_videoWorkerData->source ), "end-of-stream", &ret);
  }
  else
  {
    /*
     * Video is not enabled at the agent, return UTV_OK since this is not an error.
     */
  }

  pthread_mutex_unlock( &s_videoEnableMutex );

  if ( 0 != s_videoWorkerThread )
  {
    UtvThreadDestroy( &s_videoWorkerThread );
    s_videoWorkerThread = 0;
  }

  return ( UTV_OK );
}



/**
 **********************************************************************************************
 *
 * Validates the capabilities of the GStreamer installation within the Agent.
 *
 * @return    UTV_TRUE if the validation matches the necessary capabilities and the Agent should
 *            be able to support sending video.
 * @return    UTV_FALSE if the Agent will not be able to support sending video.
 *
 **********************************************************************************************
 */

static UTV_BOOL validate_gst_functionality( void )
{
  UTV_BOOL result = UTV_TRUE;
  const gchar* nano_str;
  guint major;
  guint minor;
  guint micro;
  guint nano;
  GList* listElement = NULL;
  GList* pluginList = NULL;
  GstPlugin* plugin = NULL;;

  gst_version( &major, &minor, &micro, &nano );

  if ( nano == 1 )
  {
    nano_str = "(CVS)";
  }
  else if ( nano == 2 )
  {
    nano_str = "(Pre-release)";
  }
  else
  {
    nano_str = "";
  }

  UtvMessage( UTV_LEVEL_INFO,
              "%s: This program is linked against GStreamer %d.%d.%d %s",
              __FUNCTION__,
              major,
              minor,
              micro,
              nano_str );

  /* TODO: Add GStreamer version checking if desired. */

  if ( UTV_FALSE == load_gst_plugins( &pluginList ) ||
       UTV_FALSE == find_gst_factories( NULL ) )
  {
    result = UTV_FALSE;
  }

  while ( NULL != ( listElement = g_list_first( pluginList ) ) )
  {
    plugin = listElement->data;
    pluginList = g_list_delete_link( pluginList, listElement );
    gst_object_unref( plugin );
  }

  g_list_free( pluginList );

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Loads the required GStreamer plugins.
 *
 * @param     list - Pointer to a pointer to a list to hold the plugin objects.  If this is
 *                   NULL then the object is unreferenced automatically.
 *
 * @return    UTV_TRUE if the plugins were loaded.
 * @return    UTV_FALSE if the plugins were not loaded.
 *
 **********************************************************************************************
 */

static UTV_BOOL load_gst_plugins( GList** list )
{
  UTV_BOOL result = UTV_TRUE;

  if ( UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgstcoreelements.so", list ) ||
       UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgstapp.so", list ) ||
       UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgstvideoscale.so", list ) ||
       UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgstffmpegcolorspace.so", list ) ||
       UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgstjpeg.so", list ) ||
       UTV_FALSE == load_gst_plugin( s_libraryPath, (UTV_BYTE*) "libgsttcp.so", list ) )
  {
    result = UTV_FALSE;
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Loads the given GStreamer plugin.  Caller is responsible for cleanup of the plugin object
 * (gst_object_unref).
 *
 * @param     path - Path to use to find the plugin.
 * @param     fileName - Name of the file representing the plugin to load.
 * @param     list - Pointer to a pointer to a list to hold the plugin objects.  If this is
 *                   NULL then the object is unreferenced automatically.
 *
 * @return    UTV_TRUE if the plugin was loaded.
 * @return    UTV_FALSE if the plugin was not loaded.
 *
 **********************************************************************************************
 */

static UTV_BOOL load_gst_plugin( UTV_BYTE* path, UTV_BYTE* fileName, GList** list )
{
  UTV_BOOL result = UTV_TRUE;
  GstPlugin* plugin;
  GError* error;
  char* buffer;
  UTV_UINT32 bufferSize;

  bufferSize = UtvStrlen( path ) + UtvStrlen( fileName ) + 2;
  buffer = UtvMalloc( bufferSize );

  UtvMemFormat( (UTV_BYTE*) buffer, bufferSize, "%s/%s", path, fileName );

  plugin = gst_plugin_load_file( buffer, &error);
  
  if ( NULL == plugin )
  {
    UtvMessage( UTV_LEVEL_ERROR,
                "%s: Failed to load plugin %s: %s",
                __FUNCTION__,
                buffer,
                error->message );

    result = UTV_FALSE;
  }
  else
  {
    if ( NULL != list )
    {
      *list = g_list_prepend( *list, plugin );
    }
    else
    {
      gst_object_unref( plugin );
    }
  }

  UtvFree( buffer );

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Attempts to locate the required GStreamer factories by name.
 *
 * @param     list - Pointer to a pointer to a list to hold the plugin objects.  If this is
 *                   NULL then the object is unreferenced automatically.
 *
 * @return    UTV_TRUE if the plugins were loaded.
 * @return    UTV_FALSE if the plugins were not loaded.
 *
 **********************************************************************************************
 */

static UTV_BOOL find_gst_factories( GList** list )
{
  UTV_BOOL result = UTV_TRUE;

  if ( UTV_FALSE == find_gst_factory( "pipeline", NULL ) ||
       UTV_FALSE == find_gst_factory( "appsrc", NULL ) ||
       UTV_FALSE == find_gst_factory( "videoscale", NULL ) ||
       UTV_FALSE == find_gst_factory( "ffmpegcolorspace", NULL ) ||
       UTV_FALSE == find_gst_factory( "jpegenc", NULL ) ||
       UTV_FALSE == find_gst_factory( "tcpclientsink", NULL ) )
  {
    result = UTV_FALSE;
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Attempts to locate the given GStreamer factory by name.  Caller is responsible for cleanup
 * of the element factory object (gst_object_unref).
 *
 * @param     factoryName - Name of the factory to locate.
 * @param     list - Pointer to a pointer to a list to hold the factory objects.  If this is
 *                   NULL then the object is unreferenced automatically.
 *
 * @return    UTV_TRUE if the factory was found.
 * @return    UTV_FALSE if the factory was not found.
 *
 **********************************************************************************************
 */

static UTV_BOOL find_gst_factory( const char* factoryName, GList** list )
{
  UTV_BOOL result = UTV_TRUE;
  GstElementFactory* factory = NULL;

  factory = gst_element_factory_find( factoryName );

  if ( !factory )
  {
    UtvMessage( UTV_LEVEL_ERROR, "Element factory %s not found.", factoryName );
    result = UTV_FALSE;
  }
  else
  {
    if ( NULL != list )
    {
      *list = g_list_prepend( *list, factory );
    }
    else
    {
      gst_object_unref( factory );
    }
  }

  return ( result );
}



/**
 **********************************************************************************************
 *
 * Callback handler for the GStreamer bus.
 *
 * @param     bus - Bus that initiated the callback.
 *
 * @param     message - Message for the callback.
 *
 * @param     data - User defined data for the callback.
 *
 * @return    NULL
 *
 **********************************************************************************************
 */

static gboolean VideoWorkerBusCallback( GstBus* bus, GstMessage* message, gpointer data )
{
  LiveSupportVideoWorkerData* workerData = (LiveSupportVideoWorkerData*) data;

  switch (GST_MESSAGE_TYPE (message))
  {
    case GST_MESSAGE_ERROR:
    {
      GError* err;
      gchar* debug;

      gst_message_parse_error( message, &err, &debug );

      UtvMessage( UTV_LEVEL_ERROR, "%s: Error: %s", __FUNCTION__, err->message );

      g_error_free( err );
      g_free( debug );

      g_main_loop_quit( workerData->loop );
      break;
    }

    case GST_MESSAGE_EOS:
    {
      g_main_loop_quit( workerData->loop );
      break;
    }

    default:
    {
      break;
    }
  }

  /*
   * Since we want another message notification, return TRUE. FALSE will abort message
   * handling.
   */

  return ( TRUE );
}



/**
 **********************************************************************************************
 *
 * Method to push data into the appsrc.
 *
 * @param     data - User defined data for the handler to use.
 *
 * @return    TRUE
 *
 **********************************************************************************************
 */

static gboolean VideoWorkerPushBuffer( LiveSupportVideoWorkerData* data )
{
  GstFlowReturn result;
  GstBuffer* buffer;

  data->num_frame++;
 
  buffer = gst_buffer_new_and_alloc( data->sourceBufferSize );

  UtvPlatformLiveSupportVideoFillVideoFrameBuffer( GST_BUFFER_DATA( buffer ),
                                                   data->sourceBufferSize );

  GST_BUFFER_TIMESTAMP(buffer) = (GstClockTime)((data->num_frame / 2.0) * 1e9);

  result = gst_app_src_push_buffer( GST_APP_SRC(data->source), buffer );

  if ( GST_FLOW_OK != result )
  {
    if ( UTV_TRUE == s_videoEnabled )
    {
      UtvMessage(
        UTV_LEVEL_WARN,
        "%s: Non-OK flow (%d) response when pushing Push the buffer into appsrc",
        __FUNCTION__,
        result );
    }
  }

  return ( TRUE );
}



/**
 **********************************************************************************************
 *
 * Handler for the "need_data" signal from the GStreamer appsrc.
 *
 * @param     pipeline - pipeline that has enough data.
 *
 * @param     size - Size needed by the pipeline.
 *
 * @param     data - User defined data for the handler to use.
 *
 * @return    none
 *
 **********************************************************************************************
 */

static void VideoWorkerNeedData(
  GstElement* pipeline,
  guint size,
  LiveSupportVideoWorkerData* data )
{
  GstFlowReturn ret;

  if ( s_videoEnabled )
  {
    if ( data->source_id == 0 )
    {
      data->source_id =
        g_timeout_add( data->pushBufferInterval,
                       (GSourceFunc) VideoWorkerPushBuffer,
                       data );
    }
  }
  else
  {
    if ( data->source_id != 0 )
    {
      g_source_remove( data->source_id );
      data->source_id = 0;
    }

    g_signal_emit_by_name( GST_APP_SRC(data->source), "end-of-stream", &ret);
  }
}



/**
 **********************************************************************************************
 *
 * Handler for the "enough_data" signal from the GStreamer appsrc.
 *
 * @param     pipeline - pipeline that has enough data.
 *
 * @param     data - User defined data for the handler to use.
 *
 * @return    none
 *
 **********************************************************************************************
 */

static void VideoWorkerEnoughData( GstElement* pipeline, LiveSupportVideoWorkerData* data )
{
  if ( data->source_id != 0)
  {
    g_source_remove( data->source_id );
    data->source_id = 0;
  }
}



/**
 **********************************************************************************************
 *
 * Worker thread to send video to the NetReady Services.
 *
 * @param     pArg - Thread arguments (none expected).
 *
 * @return    NULL
 *
 **********************************************************************************************
 */

static void* VideoWorker( void* pArg )
{
  GList* pluginList = NULL;
  GstPlugin* plugin = NULL;
  GList* factoryList = NULL;
  GstElementFactory* factory = NULL;
  GList* listElement = NULL;
  GstElement* pipeline = NULL;
  GstElement* source = NULL;
  GstElement* videoscale = NULL;
  GstElement* colorspace = NULL;
  GstElement* encoder = NULL;
  GstElement* sink = NULL;
  GstCaps* capsSrc = NULL;
  GstCaps* caps = NULL;
  GstBus* bus = NULL;
  UTV_INT32 sourceFrameWidth;
  UTV_INT32 sourceFrameHeight;
  UTV_INT32 sourceFramePitch;
  UTV_RESULT result;
  UtvLiveSupportSessionState_t sessionState;

  /* Grab the lock for phase 2 processing */
  pthread_mutex_lock(&s_videoEnableMutex);

  /* Check if video state was changed while starting up the thread */
  if ( UTV_TRUE != s_videoEnabled )
  {
    UtvMessage( UTV_LEVEL_INFO, "%s: Video was disabled", __FUNCTION__ );
    pthread_mutex_unlock(&s_videoEnableMutex);
    UTV_THREAD_EXIT;
  }

  load_gst_plugins( &pluginList );
  find_gst_factories( &factoryList );

  s_videoWorkerData->source_id = 0;
  s_videoWorkerData->num_frame = 0;

  UtvPlatformLiveSupportVideoGetVideoFrameSpecs( &sourceFrameWidth,
                                                 &sourceFrameHeight,
                                                 &sourceFramePitch );

  s_videoWorkerData->sourceFrameWidth = sourceFrameWidth;
  s_videoWorkerData->sourceFrameHeight = sourceFrameHeight;
  s_videoWorkerData->sourceFramePitch = sourceFramePitch;

  s_videoWorkerData->sourceBufferSize = s_videoWorkerData->sourceFrameWidth *
                                        s_videoWorkerData->sourceFrameHeight *
                                        s_videoWorkerData->sourceFramePitch;

  do
  {
    pipeline = gst_pipeline_new( "streamPipeline" );

    if ( NULL == pipeline )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create pipeline.", __FUNCTION__ );
      break;
    }

    source = gst_element_factory_make( "appsrc", "source" );

    if ( NULL == source )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create appsrc.", __FUNCTION__ );
      break;
    }

    videoscale = gst_element_factory_make( "videoscale", "videoscale" );

    if ( NULL == videoscale )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create videoscale.", __FUNCTION__ );
      break;
    }

    colorspace = gst_element_factory_make( "ffmpegcolorspace", "colorspace" );

    if ( NULL == colorspace )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create ffmpegcolorspace.", __FUNCTION__ );
      break;
    }

    encoder = gst_element_factory_make( "jpegenc", "encoder" );

    if ( NULL == encoder )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create jpegenc.", __FUNCTION__ );
      break;
    }

    sink = gst_element_factory_make( "tcpclientsink", "sink" );

    if ( NULL == sink )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create tcpclientsink.", __FUNCTION__ );
      break;
    }

    capsSrc = gst_caps_new_simple("video/x-raw-rgb",
                                  "bpp", G_TYPE_INT, 32,
                                  "depth", G_TYPE_INT, 32,
                                  "endianness", G_TYPE_INT, 4321,
                                  "red_mask", G_TYPE_INT, 0xff00,
                                  "green_mask", G_TYPE_INT, 0xff0000,
                                  "blue_mask", G_TYPE_INT, 0xff000000,
                                  "width", G_TYPE_INT, s_videoWorkerData->sourceFrameWidth,
                                  "height", G_TYPE_INT, s_videoWorkerData->sourceFrameHeight,
                                  "framerate",
                                    GST_TYPE_FRACTION,
                                    s_videoWorkerData->frameRateNumerator,
                                    s_videoWorkerData->frameRateDenominator,
                                  NULL);

    if ( NULL == capsSrc )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create capsSrc", __FUNCTION__ );
      break;
    }

    caps = gst_caps_new_simple( "video/x-raw-rgb",
                                "width", G_TYPE_INT, s_videoWorkerData->frameWidth,
                                "height", G_TYPE_INT, s_videoWorkerData->frameHeight,
                                "framerate",
                                  GST_TYPE_FRACTION,
                                  s_videoWorkerData->frameRateNumerator,
                                  s_videoWorkerData->frameRateDenominator,
                                NULL );

    if ( NULL == caps )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to create caps.", __FUNCTION__ );
      break;
    }

    s_videoWorkerData->pipeline = pipeline;
    s_videoWorkerData->source = source;

    g_object_set( G_OBJECT( source ), "block", FALSE, NULL );
    g_object_set( G_OBJECT( source ), "is-live", TRUE, NULL );
    g_object_set( G_OBJECT( source ), "min-percent", 50, NULL );
    g_object_set( G_OBJECT( encoder ), "quality", 85, NULL );
    g_object_set( G_OBJECT( videoscale ), "method", 0, NULL);
    g_object_set( G_OBJECT( sink ), "host", s_videoWorkerData->relayServerAddress, NULL );
    g_object_set( G_OBJECT( sink ), "port", s_videoWorkerData->relayServerPort, NULL );

#ifdef UTV_LIVE_SUPPORT_VIDEO_SECURE
    if ( TRUE == s_videoWorkerData->secureMediaStream )
    {
      g_object_set( G_OBJECT( sink ), "encryption", s_videoWorkerData->secureMediaStream, NULL );
      g_object_set( G_OBJECT( sink ), "cafile", UtvDeliverMrsCAFile(), NULL );
    }
#endif /* UTV_LIVE_SUPPORT_VIDEO_SECURE */

    gst_bin_add_many( GST_BIN( pipeline ),
                      source,
                      videoscale,
                      colorspace,
                      encoder,
                      sink,
                      NULL );

    if ( !gst_element_link_filtered( source, videoscale, capsSrc ) )
    {
      UtvMessage( UTV_LEVEL_ERROR, "Error linking source to videoscale!" );
      break;
    }

    if ( !gst_element_link_filtered( videoscale, colorspace, caps ) )
    {
      UtvMessage( UTV_LEVEL_ERROR, "Error linking videoscale to colorspace!" );
      break;
    }

    if ( !gst_element_link_pads( colorspace, "src", encoder, "sink" ) )
    {
      UtvMessage( UTV_LEVEL_ERROR, "Error linking colorspace to overlay!" );
      break;
    }

    if ( !gst_element_link_pads( encoder, "src", sink, "sink" ) )
    {
      UtvMessage( UTV_LEVEL_ERROR, "Error linking encoder to queue!" );
      break;
    }

    bus = gst_pipeline_get_bus( GST_PIPELINE( pipeline ) );

    if ( NULL == bus )
    {
      UtvMessage( UTV_LEVEL_ERROR, "%s: Failed to get bus", __FUNCTION__ );
      break;
    }

    gst_bus_add_watch( bus, VideoWorkerBusCallback, s_videoWorkerData );

    g_signal_connect( source,
                      "need-data",
                      G_CALLBACK( VideoWorkerNeedData ),
                      s_videoWorkerData );

    g_signal_connect( source,
                      "enough-data",
                      G_CALLBACK( VideoWorkerEnoughData ),
                      s_videoWorkerData);

    gst_element_set_state( pipeline, GST_STATE_PLAYING );

    s_videoWorkerData->loop = g_main_loop_new( NULL, FALSE );

    /* unlock mutex to allow state change command */
    pthread_mutex_unlock(&s_videoEnableMutex);

    g_main_loop_run( s_videoWorkerData->loop );

    /* lock mutex to allow permit cleanup */
    pthread_mutex_lock(&s_videoEnableMutex);

    result = UtvLiveSupportGetSessionState( &sessionState );

    gst_element_set_state( pipeline, GST_STATE_NULL );
  }
  while ( 0 );

  if ( NULL != pipeline )
  {
    gst_object_unref( pipeline );
    pipeline = NULL;
  }

  if ( NULL != capsSrc )
  {
    gst_caps_unref( capsSrc );
    capsSrc = NULL;
  }

  if ( NULL != caps )
  {
    gst_caps_unref( caps );
    caps = NULL;
  }

  if ( NULL != bus )
  {
    gst_object_unref( bus );
    bus = NULL;
  }

  if ( NULL != s_videoWorkerData && NULL != s_videoWorkerData->loop )
  {
    g_main_loop_unref( s_videoWorkerData->loop );
    s_videoWorkerData->loop = NULL;
  }

  if ( NULL != s_videoWorkerData )
  {
    g_free( s_videoWorkerData );
    s_videoWorkerData = NULL;
  }

  if ( NULL != factoryList )
  {
    while ( NULL != ( listElement = g_list_first( factoryList ) ) )
    {
      factory = listElement->data;
      factoryList = g_list_delete_link( factoryList, listElement );
      gst_object_unref( factory );
    }

    g_list_free( factoryList );
  }

  if ( NULL != pluginList )
  {
    while ( NULL != ( listElement = g_list_first( pluginList ) ) )
    {
      plugin = listElement->data;
      pluginList = g_list_delete_link( pluginList, listElement );
      gst_object_unref( plugin );
    }

    g_list_free( pluginList );
  }

  s_videoEnabled = UTV_FALSE;
  s_videoWorkerThread = 0;
  pthread_mutex_unlock(&s_videoEnableMutex);

  UTV_THREAD_EXIT;
}
