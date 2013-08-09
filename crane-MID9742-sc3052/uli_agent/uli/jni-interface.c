/* jni-interface.c : JNU interface to the ULI Agent shared lib.

   Copyright (c) 2011 UpdateLogic Incorporated. All rights reserved.

   This file contains all of the project and platform related functions
   for an entire UpdateTV port.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      12/21/2011  Isolated Android helper functions into project layer.
   Bob Taylor      07/18/2011  Switched to calling UtvProjectOnEarlyBoot() to minimize boot overhead.
                               Calling UtvProjectManageRegStore() instead of UtvPublicInternetManageRegStore().
   Bob Taylor      07/08/2011  JNI_OnLoad() is now the only place where UtvProjectOnBoot() is called.
   Bob Taylor      07/07/2011  Add SSUT support.
   Bob Taylor      06/27/2011  Remove percent complete debug message.  
                               Removed setting currentDeliveryMode to zero when there is no schedule
							   available in a given deivery mode.  Reentrancy was causing problems.
   Bob Taylor      06/21/2011  Download schedule collision fix.
                               Error out of DUS handled so thread will see it.
                               Re-open of previously downloaded schedule to fix stuck at 0% prob.
*/

#include <string.h> 
#include <nativehelper/jni.h>
#include <android/log.h>
#include <signal.h>
#include "utv.h" 
#include UTV_TARGET_PROJECT

/* local statics */
static void s_eventCallback( UTV_PUBLIC_RESULT_OBJECT *pResultObject );
static void s_displayUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate );

static UTV_INT8             sdCardPath[256];
static UTV_UINT32 currentUpdateTotalSize = 0;
static UTV_UINT32 currentUpdateStatus = UTV_OK;
static JavaVM *s_jvm = NULL;
static UTV_UINT32           currentDeliveryMode = 0;
UTV_PROJECT_UPDATE_INFO     tUpdateInfo;
UTV_INT8                    cVersion[32];
UTV_INT8                    cInfo[128];
            
jint Java_com_uli_supervisor_ULIService_UtvSetUpdateInstallPath( JNIEnv* env, jobject thiz, jstring jPath)
{
    const char *pszPath = (*env)->GetStringUTFChars(env, jPath, 0);
    UtvProjectSetUpdateInstallPath(pszPath);
    (*env)->ReleaseStringUTFChars(env, jPath, pszPath);
    return UTV_OK;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetUpdateInstallPath( JNIEnv* env, jobject thiz )
{
    return (*env)->NewStringUTF(env, UtvProjectGetUpdateInstallPath());
}

jint Java_com_uli_supervisor_ULIService_UtvSetPeristentPath( JNIEnv* env, jobject thiz, jstring jPath)
{
    const char *pszPath = (*env)->GetStringUTFChars(env, jPath, 0);
    UtvProjectSetPeristentPath(pszPath);
    (*env)->ReleaseStringUTFChars(env, jPath, pszPath);
    return UTV_OK;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetPeristentPath( JNIEnv* env, jobject thiz )
{
    return (*env)->NewStringUTF(env, UTV_PLATFORM_PERSISTENT_PATH);
}

jint Java_com_uli_supervisor_ULIService_UtvSetFactoryPath( JNIEnv* env, jobject thiz, jstring jPath)
{
    const char *pszPath = (*env)->GetStringUTFChars(env, jPath, 0);
    UtvProjectSetFactoryPath(pszPath);
    (*env)->ReleaseStringUTFChars(env, jPath, pszPath);
    return UTV_OK;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetFactoryPath( JNIEnv* env, jobject thiz )
{
    return (*env)->NewStringUTF(env, UTV_PLATFORM_INTERNET_FACTORY_PATH);
}

jint Java_com_uli_supervisor_ULIService_UtvSetReadOnlyPath( JNIEnv* env, jobject thiz, jstring jPath)
{
    const char *pszPath = (*env)->GetStringUTFChars(env, jPath, 0);
    UtvProjectSetReadOnlyPath(pszPath);
    (*env)->ReleaseStringUTFChars(env, jPath, pszPath);
    return UTV_OK;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetReadOnlyPath( JNIEnv* env, jobject thiz )
{
    return (*env)->NewStringUTF(env, UTV_PLATFORM_INTERNET_READ_ONLY_PATH);
}

jboolean Java_com_uli_supervisor_ULIService_UtvIsUpdateReadyToInstall(JNIEnv* env, jobject thiz){
    return UtvProjectGetUpdateReadyToInstall();
}

jint Java_com_uli_supervisor_ULIService_UtvSetIsUpdateReadyToInstall(JNIEnv* env, jobject thiz, int value){
    UtvProjectSetUpdateReadyToInstall((UTV_BOOL)value);
    return UTV_OK;
}

jboolean Java_com_uli_supervisor_ULIService_UtvManifestGetLoopTestStatus(JNIEnv* env, jobject thiz){
    return UtvManifestGetLoopTestStatus();
}

void JNI_AttachCurrentThread(){
    JNIEnv *env;
    if (s_jvm){
        (*s_jvm)->AttachCurrentThread (s_jvm, (void **) &env, NULL);
    }
}

void JNI_DetachCurrentThread(){
    JNIEnv *env;
    if (s_jvm){
        (*s_jvm)->DetachCurrentThread (s_jvm);
    }
}

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    s_jvm = vm;
    UtvProjectSetUpdateInstallPath(NULL);
    UtvProjectSetPeristentPath(NULL);
    UtvProjectSetFactoryPath(NULL);
    UtvProjectSetReadOnlyPath(NULL);
    UtvMemFormat( (UTV_BYTE *) cVersion, sizeof(cVersion), "%s", "" );
    UtvMemFormat( (UTV_BYTE *) cInfo, sizeof(cInfo), "%s", "" );
    UtvProjectOnRegisterCallback( s_eventCallback );
	UtvProjectOnEarlyBoot();
    return JNI_VERSION_1_6; 
}


jint Java_com_uli_supervisor_ULIService_SetUpdateInstallPath( JNIEnv* env, jobject thiz, jstring jName)
{
    UTV_RESULT    rStatus;

    const char *pszName = (*env)->GetStringUTFChars(env, jName, 0);
    UtvProjectSetUpdateInstallPath( pszName );
    (*env)->ReleaseStringUTFChars(env, jName, pszName);
    
    return UTV_OK;
}

jint Java_com_uli_supervisor_ULIService_UtvDebugSetThreadName( JNIEnv* env, jobject thiz, jstring jName)
{
    UTV_RESULT    rStatus;
    static char txt[128];

    const char *pszName = (*env)->GetStringUTFChars(env, jName, 0);

    UtvMemFormat( (UTV_BYTE *) txt, sizeof(txt), "%s", pszName);
    UtvMessage( UTV_LEVEL_INFO, "UtvDebugSetThreadName(%s)", txt);
    
    /* only works when UTV_DEBUG is defined */
#ifdef UTV_DEBUG
    UtvDebugSetThreadName( txt );
#endif
    
    (*env)->ReleaseStringUTFChars(env, jName, pszName);
    
    return UTV_OK;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetBuildVariant( JNIEnv* env, jobject thiz )
{
    char txt[256];
    UtvMemFormat( (UTV_BYTE *) txt, sizeof(txt), "%s", TARGET_BUILD_VARIANT );
    return (*env)->NewStringUTF(env, txt);
}

jstring Java_com_uli_supervisor_ULIService_UtvGetSharedLibVersion( JNIEnv* env, jobject thiz )
{
    char txt[256];
    UtvMemFormat( (UTV_BYTE *) txt, sizeof(txt), "%s", UTV_AGENT_VERSION);
    return (*env)->NewStringUTF(env, txt);
}

jstring Java_com_uli_supervisor_ULIService_UtvGetPrivateLibVersion( JNIEnv* env, jobject thiz )
{
    char txt[256];
    UtvMemFormat( (UTV_BYTE *) txt, sizeof(txt), "%s", UtvPrivateLibraryVersion());
    return (*env)->NewStringUTF(env, txt);
}

jint Java_com_uli_supervisor_ULIService_UtvProjectOnNetworkUp( JNIEnv* env, jobject thiz )
{
    UtvProjectOnNetworkUp();
    return UTV_OK;
}

#ifdef UTV_DEBUG
jint Java_com_uli_supervisor_ULIService_UtvProjectOnResetComponentManifest( JNIEnv* env, jobject thiz )
{
    return UtvProjectOnResetComponentManifest();
}
#endif

jint Java_com_uli_supervisor_ULIService_UtvProjectOnShutdown( JNIEnv* env, jobject thiz )
{
    UtvProjectOnShutdown();
    return UTV_OK;
}

jint Java_com_uli_supervisor_ULIService_UtvProjectOnAbort( JNIEnv* env, jobject thiz )
{
    UtvProjectOnAbort();
    return UTV_OK;
}

jint Java_com_uli_supervisor_ULIService_UtvProjectOnScanForStoredUpdate( JNIEnv* env, jobject thiz )
{
    UtvProjectOnScanForStoredUpdate( NULL );
    return UTV_OK;
}


jint Java_com_uli_supervisor_ULIService_UtvProjectOnForceNOCContact( JNIEnv* env, jobject thiz )
{
    UtvProjectOnForceNOCContact();
    return UTV_OK;
}


jint Java_com_uli_supervisor_ULIService_UtvProjectOnDebugInfo( JNIEnv* env, jobject thiz )
{
    UtvProjectOnDebugInfo();
    return UTV_OK;
}

void removeSpecialCharFromString(UTV_INT8* str){
    UTV_UINT32 stringSize, i;
    stringSize = UtvStrlen(str);
    for (i = 0; i < stringSize; i++)
    {
        if ((str[i] >= '0' && str[i] <= '9') || (str[i] >= 'A' && str[i] <= 'Z')|| (str[i] >= 'a' && str[i] <= 'z') || (str[i] == '.' || str[i] == '_' || str[i] == '\''|| str[i] == ' ' || str[i] == '@'|| str[i] == '-' ))
        {
        }
        else{
            str[i] = '_';
        }
    }
}


jint Java_com_uli_supervisor_ULIService_UtvPublicInternetManageRegStore(  JNIEnv* env, jobject thiz, jobjectArray stringArray ){
    int i;
    UTV_RESULT                     rStatus;
    UTV_INT8** nativeStringArray;
    jstring *jStringArray;
   jsize len = (*env)->GetArrayLength(env, stringArray);
   
   nativeStringArray = (UTV_INT8**)UtvMalloc(len * sizeof(UTV_INT8*));
   jStringArray = (jstring *)UtvMalloc(len * sizeof(jstring));

   for(i=0; i < len; i++)
   {
      jStringArray[i] = (*env)->GetObjectArrayElement(env, stringArray, i);
      nativeStringArray[i] = (*env)->GetStringUTFChars(env, jStringArray[i], 0);
      /* UtvMessage( UTV_LEVEL_INFO, "UtvPublicInternetManageRegStore() index = %d, raw string = %s", i, nativeStringArray[i]); */
      removeSpecialCharFromString(nativeStringArray[i]);
      /* UtvMessage( UTV_LEVEL_INFO, "UtvPublicInternetManageRegStore() index = %d, cleaned string = %s", i, nativeStringArray[i]); */

   }
   
   rStatus = UtvProjectManageRegStore( len, nativeStringArray );
   
   for(i=0; i < len; i++)
   {
      (*env)->ReleaseStringUTFChars(env, jStringArray[i], nativeStringArray[i]);
      (*env)->DeleteLocalRef(env, jStringArray[i]); 
   }
   
   UtvFree(nativeStringArray);
   UtvFree(jStringArray);
   
   return rStatus;
}


jint Java_com_uli_supervisor_ULIService_UtvPlatformInsertULPK( JNIEnv* env, jobject thiz,  jbyteArray ulpk ){
    UTV_RESULT rStatus;
    UTV_BYTE *pbULPK = NULL;
    UTV_BYTE *pbClearULPK = NULL;
    UTV_BYTE   *pDataCheck;
    UTV_UINT32  i, uiSizeCheck, clearUlpkSize;
    jsize ulpkSize = (*env)->GetArrayLength(env, ulpk);
    pbULPK = (*env)->GetByteArrayElements( env, ulpk, 0);
    
    /* insert the ULPK */
    if ( UTV_OK != ( rStatus = UtvPlatformSecureInsertULPK( pbULPK )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformSecureInsertULPK fails: %s", UtvGetMessageString( rStatus ));
    }
    
    (*env)->ReleaseByteArrayElements( env, ulpk, (jbyte *)pbULPK, 0);
    return rStatus;
}  

jint Java_com_uli_supervisor_ULIService_UtvPlatformGetULPKIndex( JNIEnv* env, jobject thiz ){

    /* return the installed ULPK index */
    return UtvPlatformSecureGetULPKIndex();
}  

jint Java_com_uli_supervisor_ULIService_UtvProjectOTAUpdateAvailable(JNIEnv* env, jobject thiz)
{
    UtvProjectSetUpdateSchedule(NULL);
    UtvProjectOnScanForOTAUpdate();
    UTV_PUBLIC_SCHEDULE *pSchedule = UtvProjectGetUpdateSchedule();
    if (NULL == pSchedule){
        /* usError value is set by s_GetScheduleBlocking */
        currentUpdateStatus = g_pUtvDiagStats->utvLastError.usError;
        return 0;
    }
    UTV_PUBLIC_UPDATE_SUMMARY *pUpdate = &pSchedule->tUpdates[ 0 ];
    
    if (pUpdate->uiDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_INTERNET)
	{
		currentDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_INTERNET;
        return 1;
	}

        return 0;
}

jint Java_com_uli_supervisor_ULIService_UtvProjectMediaUpdateAvailable(JNIEnv* env, jobject thiz, jstring jPath )
{
    const char *pszPath = (*env)->GetStringUTFChars(env, jPath, 0);
	/* save a copy of the path for error recovery */
	UtvStrnCopy( (UTV_BYTE *) sdCardPath, sizeof(sdCardPath), 
				 (UTV_BYTE *) pszPath, UtvStrlen( (UTV_BYTE *) pszPath ));
    UtvProjectOnUSBInsertionBlocking(pszPath);
    (*env)->ReleaseStringUTFChars(env, jPath, pszPath);

    UTV_PUBLIC_SCHEDULE *pSchedule = UtvProjectGetUpdateSchedule();
    if (NULL == pSchedule)
        return 0;
    UTV_PUBLIC_UPDATE_SUMMARY *pUpdate = &pSchedule->tUpdates[ 0 ];
    
    if (pUpdate->uiDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_FILE)
	{
		currentDeliveryMode = UTV_PUBLIC_DELIVERY_MODE_FILE;		
        return 1;
	}

        return 0;
}

jobjectArray Java_com_uli_supervisor_ULIService_UtvProjectGetUpdateInfo(JNIEnv* env, jobject thiz)
{
    UTV_RESULT rStatus;
    UTV_INT8 pszInfo[256];
    UTV_INT8 pszVersion[256];
    UTV_INT8 pszDesc[512];
    UTV_INT8 pszType[64];
    jstring str = NULL;
    UtvMemFormat( (UTV_BYTE *) pszInfo, sizeof(pszInfo), "%s", "" );
    UtvMemFormat( (UTV_BYTE *) pszVersion, sizeof(pszVersion), "%s", "" );
    UtvMemFormat( (UTV_BYTE *) pszDesc, sizeof(pszDesc), "%s", "" );
    UtvMemFormat( (UTV_BYTE *) pszType, sizeof(pszType), "%s", "" );
    
    jclass strCls = (*env)->FindClass(env,"java/lang/String");
    jobjectArray strarray = (*env)->NewObjectArray(env,4,strCls,NULL);

    if ( UTV_OK != ( rStatus = UtvProjectGetAvailableUpdateInfo( pszInfo, sizeof(pszInfo), pszVersion, sizeof(pszVersion), pszDesc, sizeof(pszDesc), pszType,sizeof(pszType) )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvProjectGetAvailableUpdateInfo: %s", UtvGetMessageString( rStatus ));
    }

    str = (*env)->NewStringUTF(env,pszInfo);
    (*env)->SetObjectArrayElement(env,strarray,0,str);
    (*env)->DeleteLocalRef(env,str);

    str = (*env)->NewStringUTF(env,pszVersion);
    (*env)->SetObjectArrayElement(env,strarray,1,str);
    (*env)->DeleteLocalRef(env,str);

    str = (*env)->NewStringUTF(env,pszDesc);
    (*env)->SetObjectArrayElement(env,strarray,2,str);
    (*env)->DeleteLocalRef(env,str);
    
    str = (*env)->NewStringUTF(env,pszType);
    (*env)->SetObjectArrayElement(env,strarray,3,str);
    (*env)->DeleteLocalRef(env,str);

    return strarray;
}



jint Java_com_uli_supervisor_ULIService_UtvProjectDownloadPercentComplete(JNIEnv* env, jobject thiz)
{
    UTV_UINT32 percentComplete = 0;
    UtvPublicGetPercentComplete( currentUpdateTotalSize, &percentComplete );
    return percentComplete;
}


UTV_INT32 UtvDownloadScheduledUpdate()
{
    UTV_RESULT                     rStatus;
    UTV_PUBLIC_SCHEDULE           *pSchedule;
    UTV_UINT32                     uiBytesWritten, hStore, hImage;
	UTV_BOOL                       bOpen, bPrimed, bAssignable, bManifest, bAlreadyHave;
    UTV_PUBLIC_UPDATE_SUMMARY 	  *pUpdate;

    /* Is a downloaded component directroy already open? */
    if ( UTV_OK != ( rStatus = UtvPlatformStoreAttributes( UTV_PLATFORM_UPDATE_STORE_0_INDEX, 
                                                           &bOpen, &bPrimed, &bAssignable, 
														   &bManifest, &uiBytesWritten )))
    {
        UtvMessage( UTV_LEVEL_ERROR, 
                    "UtvPlatformStoreAttributes( %d ) fails with error \"%s\"", 
                    UTV_PLATFORM_UPDATE_STORE_0_INDEX, UtvGetMessageString( rStatus ) );
        currentUpdateStatus = rStatus;
        return currentUpdateStatus;
    }

	UtvProjectSetUpdateReadyToInstall(UTV_FALSE);

	/* clean up any left over accumlated installation */
	UtvPlatformInstallCleanup();
	UtvPlatformInstallInit();

    if (currentDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_INTERNET)
    {
		/* if not open refresh schedule */
		if ( !bOpen )
{
			UtvProjectSetUpdateSchedule(NULL);				
			UtvProjectOnScanForOTAUpdate();
		}
    
		pSchedule = UtvProjectGetUpdateSchedule();
		if (NULL == pSchedule){
			/* usError value is set by s_GetScheduleBlocking */
			if ( UTV_OK != g_pUtvDiagStats->utvLastError.usError )
				currentUpdateStatus = g_pUtvDiagStats->utvLastError.usError;
			else
				currentUpdateStatus = UTV_SCHEDULE_MISSING;
        return currentUpdateStatus;
    }
           
		currentUpdateStatus = UTV_OK;
		pUpdate = &pSchedule->tUpdates[ 0 ];
    
            /* or they can all be approved all at once using these two calls */
            UtvMessage( UTV_LEVEL_INFO, "   approving all components" );

            /* direct the download to install while it is downloading and to not convert optional components to required */
            UtvPublicSetUpdatePolicy( UTV_TRUE, UTV_FALSE );

            /* Abitrarily approve ALL of the components in the returned schedule. */
            if ( UTV_OK != ( rStatus = UtvPublicApproveForDownload( pUpdate, UTV_PLATFORM_UPDATE_STORE_0_INDEX )))
            {
                UtvMessage( UTV_LEVEL_ERROR,
                            "UtvPublicApproveUpdate( pUpdate, UTV_PLATFORM_UPDATE_STORE_0_INDEX ) \"%s\"",
                            UtvGetMessageString( rStatus ) );
			currentUpdateStatus = rStatus;
                return rStatus;
            }
            s_displayUpdateInfo(pUpdate);
            UtvPublicGetUpdateTotalSize(pUpdate,&currentUpdateTotalSize);
            /* now that approval is complete launch the download */
            UtvProjectOnUpdate( pUpdate );
    }
    else if (currentDeliveryMode == UTV_PUBLIC_DELIVERY_MODE_FILE)
    {
		/* if not open refresh schedule */
		if ( !bOpen )
    {
			UtvProjectSetUpdateSchedule(NULL);				
			UtvProjectOnUSBInsertionBlocking(sdCardPath);
            UtvMessage( UTV_LEVEL_WARN,"Store not open yet" );
		}

		pSchedule = UtvProjectGetUpdateSchedule();

		if ( pSchedule == NULL ){
			if ( UTV_OK != g_pUtvDiagStats->utvLastError.usError )
				currentUpdateStatus = g_pUtvDiagStats->utvLastError.usError;
			else
				currentUpdateStatus = UTV_SCHEDULE_MISSING;
			return currentUpdateStatus;
		}

		currentUpdateStatus = UTV_OK;
		pUpdate = &pSchedule->tUpdates[ 0 ];
            s_displayUpdateInfo( pUpdate );
            UtvPublicGetUpdateTotalSize(pUpdate,&currentUpdateTotalSize);
            UtvProjectOnInstallFromMedia( pUpdate );    
    }
    else
    {
        currentUpdateStatus = UTV_BAD_DELIVERY_MODE;;        
        return UTV_BAD_DELIVERY_MODE;
    }
            
    /* note that currentUpdateStatus is not refreshed when the launch goes ok.
       either UtvProjectOnUpdate() or UtvProjectOnInstallFromMedia() will do that. */
    return UTV_OK;
}

jint Java_com_uli_supervisor_ULIService_UtvProjectGetCurrentUpdateStatus(JNIEnv* env, jobject thiz)
{
    return currentUpdateStatus;
}

jstring Java_com_uli_supervisor_ULIService_UtvGetMessageString( JNIEnv* env, jobject thiz, int errorCode )
{
    return (*env)->NewStringUTF(env, UtvGetMessageString(errorCode));
}

jint Java_com_uli_supervisor_ULIService_UtvDownloadScheduledUpdate(JNIEnv* env, jobject thiz)
{
    return UtvDownloadScheduledUpdate();
}

static void s_displayUpdateInfo( UTV_PUBLIC_UPDATE_SUMMARY *pUpdate )
{
    UTV_RESULT                     rStatus;

    /* setup return strings */
    tUpdateInfo.pszVersion = cVersion;
    tUpdateInfo.uiVersionBufferLen = sizeof(cVersion);
    tUpdateInfo.pszInfo = cInfo;
    tUpdateInfo.uiInfoBufferLen = sizeof(cInfo);

    /* get update info */
    if ( UTV_OK != ( rStatus = UtvProjectOnUpdateInfo( pUpdate, &tUpdateInfo )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvOnProjectUpdateInfo() \"%s\"", UtvGetMessageString( rStatus ) );
        return;
    }

    /* display update message */
    UtvMessage( UTV_LEVEL_INFO,    "UPDATE INFO:" );
    UtvMessage( UTV_LEVEL_INFO,    "   VER: %s", tUpdateInfo.pszVersion );
    UtvMessage( UTV_LEVEL_INFO,    "   INF: %s", tUpdateInfo.pszInfo );

}


static void s_eventCallback( UTV_PUBLIC_RESULT_OBJECT *pResultObject )
{
    /* clear abort whenever it's encountered */
    if ( UTV_ABORT == pResultObject->rOperationResult )
        UtvClearState();

    switch ( pResultObject->iToken )
    {

    case UTV_PROJECT_SYSEVENT_DOWNLOAD:
        currentUpdateStatus = pResultObject->rOperationResult;
        UtvMessage( UTV_LEVEL_INFO, "UTV_PROJECT_SYSEVENT_DOWNLOAD: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );    
        //UtvProjectOnShutdown();   
        //UtvProjectOnNetworkUp();    
        //UtvProjectOnScanForOTAUpdate();        
        break;

    case UTV_PROJECT_SYSEVENT_MEDIA_INSTALL:
        currentUpdateStatus = pResultObject->rOperationResult;
        UtvMessage( UTV_LEVEL_INFO, "UTV_PROJECT_SYSEVENT_MEDIA_INSTALL: \"%s\"",
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        break;

    default:
        UtvMessage( UTV_LEVEL_INFO, "UNKNOWN EVENT: %u: \"%s\"", pResultObject->iToken,
                    UtvGetMessageString( pResultObject->rOperationResult ) );
        return;
    }
}

jobjectArray Java_com_uli_supervisor_ULIService_UtvPublicGetAgentInfo(  JNIEnv* env, jobject thiz, jobjectArray stringArray ){
    int i;
    UTV_RESULT                     rStatus;
        UTV_PROJECT_STATUS_STRUCT    tAgentStatus;
    UTV_INT8                    cESN[48];
    UTV_INT8                    cQH[32];
    UTV_INT8                    cVersion[32];
    UTV_INT8                    cInfo[128];
    UTV_INT8                    cError[128];
    UTV_INT8                    cTime[48];
    UTV_INT8 tmpString[256];
    UTV_INT8** nativeStringArray;
    jstring *jStringArray;
    jstring str = NULL;
    jsize len = (*env)->GetArrayLength(env, stringArray);
    jclass strCls = (*env)->FindClass(env,"java/lang/String");
    jobjectArray strarray = (*env)->NewObjectArray(env,len,strCls,NULL);


    /* setup return strings */
    tAgentStatus.pszESN = cESN;
    tAgentStatus.uiESNBufferLen = sizeof(cESN);
    tAgentStatus.pszQueryHost = cQH;
    tAgentStatus.uiQueryHostBufferLen = sizeof(cQH);
    tAgentStatus.pszVersion = cVersion;
    tAgentStatus.uiVersionBufferLen = sizeof(cVersion);
    tAgentStatus.pszInfo = cInfo;
    tAgentStatus.uiInfoBufferLen = sizeof(cInfo);
    tAgentStatus.pszLastError = cError;
    tAgentStatus.uiLastErrorBufferLen = sizeof(cError);

    /* get status */
    if ( UTV_OK != ( rStatus = UtvProjectOnStatusRequested( &tAgentStatus )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvOnProjectStatusRequested() \"%s\"", UtvGetMessageString( rStatus ) );
    }
  
   nativeStringArray = (UTV_INT8**)UtvMalloc(len * sizeof(UTV_INT8*));
   jStringArray = (jstring *)UtvMalloc(len * sizeof(jstring));

   for(i=0; i < len; i++)
   {
      jStringArray[i] = (*env)->GetObjectArrayElement(env, stringArray, i);
      nativeStringArray[i] = (*env)->GetStringUTFChars(env, jStringArray[i], 0);
      /*UtvMessage( UTV_LEVEL_INFO, "UtvPublicGetAgentInfo() index = %d, string = %s", i, nativeStringArray[i]);*/

   }
   
   for(i=0; i < len; i++)
   {
        if (!strcmp(nativeStringArray[i], "ESN"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", tAgentStatus.pszESN );
        } else if (!strcmp(nativeStringArray[i], "UID"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", tAgentStatus.uiULPKIndex );
        }  else if (!strcmp(nativeStringArray[i], "QH"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", tAgentStatus.pszQueryHost );
        }  else if (!strcmp(nativeStringArray[i], "VER"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", tAgentStatus.pszVersion );
        }  else if (!strcmp(nativeStringArray[i], "INF"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", tAgentStatus.pszInfo );
        }  else if (!strcmp(nativeStringArray[i], "OUI"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "0x%06X", tAgentStatus.uiOUI );
        }  else if (!strcmp(nativeStringArray[i], "MG"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "0x%04X", tAgentStatus.usModelGroup );
        } else if (!strcmp(nativeStringArray[i], "HM"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "0x%04X", tAgentStatus.usHardwareModel );
        }  else if (!strcmp(nativeStringArray[i], "SV"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "0x%04X", tAgentStatus.usSoftwareVersion );
        }  else if (!strcmp(nativeStringArray[i], "MV"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "0x%04X", tAgentStatus.usModuleVersion );
        }  else if (!strcmp(nativeStringArray[i], "NP"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", tAgentStatus.usNumObjectsProvisioned );
        }  else if (!strcmp(nativeStringArray[i], "REG"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_REGISTERED) );
        }  else if (!strcmp(nativeStringArray[i], "LP"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", 0 != (tAgentStatus.usStatusMask & UTV_PROJECT_STATUS_MASK_LOOP_TEST_MODE) );
        }  else if (!strcmp(nativeStringArray[i], "EC"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", tAgentStatus.uiErrorCount );
        }  else if (!strcmp(nativeStringArray[i], "LE"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%u", tAgentStatus.uiLastErrorCode );
        }  else if (!strcmp(nativeStringArray[i], "LES"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", tAgentStatus.pszLastError );
        }  else if (!strcmp(nativeStringArray[i], "LET"))
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", UtvCTime( tAgentStatus.tLastErrorTime, cTime, sizeof(cTime) )  );
        } else if (!strcmp(nativeStringArray[i], "currentUpdateTotalSize"))
        {
            UTV_PUBLIC_SCHEDULE *pSchedule = UtvProjectGetUpdateSchedule();
            if (NULL != pSchedule){
                UTV_UINT32 updateTotalSize = 0;
                UTV_PUBLIC_UPDATE_SUMMARY *pUpdate = &pSchedule->tUpdates[ 0 ];
                UtvPublicGetUpdateTotalSize(pUpdate,&updateTotalSize);
                UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%d", updateTotalSize );
            }
            else{
                UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%d", 0 );
            }
        } else 
        {
            UtvMemFormat( (UTV_BYTE *) tmpString, sizeof(tmpString), "%s", "Unknown"  );
        }
        
        str = (*env)->NewStringUTF(env,tmpString);
        (*env)->SetObjectArrayElement(env,strarray,i,str);
        (*env)->DeleteLocalRef(env,str);
    }
   
   for(i=0; i < len; i++)
   {
      (*env)->ReleaseStringUTFChars(env, jStringArray[i], nativeStringArray[i]);
      (*env)->DeleteLocalRef(env, jStringArray[i]); 
   }
   
   UtvFree(nativeStringArray);
   UtvFree(jStringArray);
   
   return strarray;
}

jboolean Java_com_uli_supervisor_ULIService_UtvIsCurrentUpdateOptional(JNIEnv* env, jobject thiz){

    UTV_PUBLIC_SCHEDULE *pSchedule = UtvProjectGetUpdateSchedule();
    if (NULL == pSchedule)
        return JNI_FALSE;
    return UtvManifestOptional( pSchedule->tUpdates[ 0 ].hImage );
}

