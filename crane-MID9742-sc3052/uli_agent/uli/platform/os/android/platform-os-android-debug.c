/* linux-debug.cpp: UpdateTV Personality Map - Debug and Logging support for Linux

   Each of these calls map into a C runtime or native operating system call.

   Customers should not need to modify this file when porting.
   Everything in this source file is under conditional compilation when
   UTV_TEST_MESSAGES is defined.

   This file is normally NOT included in the RELEASE build of the Agent.

   Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who            Date        Edit
   Jim Muchow      08/20/2010  Create a means to open/close log file just once (previous code retained)
   Bob Taylor      07/02/2010  UTVDataDump() modified to print indefinite length buffers.
   Jim Muchow      04/06/2010  Add UTVDataDump() function
   Bob Taylor      03/30/2010  Fixed test log issue.
   Bob Taylor      08/06/2008  Converted from C++ to C.
   Bob Taylor      07/25/2007  Mac compiler warning.
   Bob Taylor      07/12/2007  Cleaned up logging mechanism.  Thread display on ifdef.
   Bob Taylor      04/30/2007  Derived from persdebug.cpp for version 1.9.
*/

#undef __STRICT_ANSI__            /* to enable vsnprintf */
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "utv.h"
#include <android/log.h>
/*#include <jni.h>*/
#include <nativehelper/jni.h>

/*
JavaVM *svm = NULL; 
 jobject io; 
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env; 
    jclass cls; 
    jmethodID constr; 
    svm = vm; 
    
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) 
        return -1; 
    cls = (*env)->FindClass(env, "com/example/uliAgent/AgentUI");
    __android_log_print(ANDROID_LOG_ERROR, "JNI_OnLoad", "FindClass -> %p", cls); 
    constr = (*env)->GetMethodID(env, cls, "<init>", "()V"); 
    __android_log_print(ANDROID_LOG_ERROR, "JNI_OnLoad", "GetMethodID -> %p", constr); 
    io = (*env)->NewGlobalRef(env, (*env)->NewObject(env, cls, constr)); 
    __android_log_print(ANDROID_LOG_ERROR, "JNI_OnLoad", "io -> %p", io); 
    return JNI_VERSION_1_6; 
}

void JNI_OnUnload(JavaVM *vm, void *reserved) { 
    JNIEnv *env;
    (*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6); 
    (*env)->DeleteGlobalRef(env, io); 
} 

 JNIEnv *JNU_GetEnv()
 {
     JNIEnv *env;
     (*svm)->GetEnv(svm,(void **)&env,JNI_VERSION_1_6);
     return env;
 }


void Java_com_example_uliAgent_AgentUI_DebugOutput(char *s) { 
    jclass cls; 
    jmethodID mid; 
    jstring js;
    JNIEnv* env;

    if (svm == NULL)
        return;
    
    env = JNU_GetEnv();


    cls = (*env)->FindClass(env, "com/example/uliAgent/AgentUI"); 
    __android_log_print(ANDROID_LOG_ERROR, "DebugOutput", "FindClass -> %p", cls); 
    if (cls == 0) 
        return;

    js = (*env)->NewStringUTF(env, s);

    mid = (*env)->GetMethodID(env, cls, "NativeDebugOutput", "(Ljava/lang/String;)V"); 
    __android_log_print(ANDROID_LOG_ERROR, "DebugOutput", "GetMethodID -> %p", mid);
    if (mid == 0) 
        return; 

    (*env)->CallVoidMethod(env, io, mid, js);
}
*/
#define UTV_MESSAGE_MAX 2400

/* All debug message related functions under UTV_TEST_MESSAGES
   undefine UTV_TEST_MESSAGES to remove all message output
*/
#ifdef UTV_TEST_MESSAGES

#define UTV_THREAD_DISPLAY

/* Global storage */
UTV_UINT32 g_uiMessageLevel = UTV_LEVEL_MESSAGES;

static char s_szMessage[ UTV_MESSAGE_MAX ];
static char s_szLog[ UTV_MESSAGE_MAX  * 2 ];
static char s_shortLog[ UTV_MESSAGE_MAX  * 2 ];

/* Change order of these to activate or not */
#undef OPEN_LOGFILE_JUST_ONCE
#define OPEN_LOGFILE_JUST_ONCE

#ifdef UTV_TEST_LOG_FNAME
#ifdef OPEN_LOGFILE_JUST_ONCE
/* ifdef prevents compiler clucking */
static FILE *logfile = NULL;
#endif

void UtvCloseLogFile( void )
{
#ifdef OPEN_LOGFILE_JUST_ONCE
    if ( NULL == logfile )
        return;

    fclose( logfile );
    logfile = NULL;
#endif
}
#endif

/* UtvMessage
    Main routine for logging information
    This code should NOT need to be changed when porting to new operating systems
*/
void UtvMessage( UTV_UINT32 iLevel, const char * pszFormat, ... )
{
    va_list     va;
    char      * pszType = "";
    UTV_UINT32  i;
 

    /* See if we need to say anything */
    if ( ! ( iLevel & g_uiMessageLevel ) )
        return;

    /* Format the data */
    va_start( va, pszFormat );
    vsnprintf( s_szMessage, sizeof(s_szMessage), pszFormat, va );
    va_end( va );

    /* zero terminate at any non-ASCII characters */
    for ( i = 0; s_szMessage[ i ] != 0; i++ )
    {
        if ( (UTV_BYTE) s_szMessage[ i ] >= 0x80 )
            break;
        if ( s_szMessage[ i ] < ' ' && s_szMessage[ i ] != '\r' && s_szMessage[ i ] != '\n' && s_szMessage[ i ] != '\t' )
            break;
    }

    s_szMessage[ i ] = 0;

    /* Determine the error type */
    if ( iLevel & UTV_LEVEL_ERROR )
        pszType = "error: ";
    else if ( iLevel & UTV_LEVEL_WARN )
        pszType = "warn: ";
    else if ( iLevel & UTV_LEVEL_PERF )
        pszType = "perf: ";
    else if ( iLevel & UTV_LEVEL_OOB_TEST )
        pszType = "++++ OOB Milestone ++++: ";

    /* format the debug msg with time, date, and/or other optional information here. */
    {
        UTV_TIME tNow;
        UTV_INT8 cTime[32];
        tNow = UtvTime( NULL );
        UtvCTime( tNow, cTime, sizeof(cTime) );

        /* kill year */
        cTime[19] = 0;

#ifdef UTV_THREAD_DISPLAY
        if ( iLevel & UTV_LEVEL_PERF )
        {
            sprintf( s_szLog, "UTV %s (%s) %s%s\n", cTime, UtvDebugGetThreadName(), pszType, s_szMessage );
            sprintf( s_shortLog, "(%s)%s%s\n", UtvDebugGetThreadName(), pszType, s_szMessage );
        }
        else
        {
            sprintf( s_szLog, "UTV %s (%s) %s%s\n", cTime, UtvDebugGetThreadName(), pszType, s_szMessage );
            sprintf( s_shortLog, "(%s)%s%s\n", UtvDebugGetThreadName(), pszType, s_szMessage );
        }
#else
        if ( iLevel & UTV_LEVEL_PERF )
        {
            sprintf( s_szLog, "UTV %s %s%s\n", cTime, pszType, s_szMessage );
            sprintf( s_shortLog, "%s%s\n", pszType, s_szMessage );
        }
        else
        {
            sprintf( s_szLog, "UTV %s %s%s\n", cTime, pszType, s_szMessage );
            sprintf( s_shortLog, "%s%s\n" pszType, s_szMessage );
        }
#endif

    }
#ifndef UTV_OPERATE_QUIETLY
    /* Log to console 
    printf( "%s", s_szLog );*/
    #ifdef COMMAND_LINE_ONLY_TEST_AGENT
        printf( "%s", s_szLog );
    #else
        if ( iLevel & UTV_LEVEL_ERROR )
            __android_log_print(ANDROID_LOG_ERROR,"UPDATELOGIC",  "%s", s_szLog );
        else if ( iLevel & UTV_LEVEL_WARN )
            __android_log_print(ANDROID_LOG_WARN,"UPDATELOGIC",  "%s", s_szLog );
        else
            __android_log_print(ANDROID_LOG_INFO,"UPDATELOGIC",  "%s", s_szLog );
    #endif
    

    /*interfaceDebugOutput(s_shortLog);*/
    /*interfaceDebugOutput(s_szLog);*/
#endif

#ifdef UTV_TEST_LOG_FNAME
#ifdef OPEN_LOGFILE_JUST_ONCE
    /* Once things are set up, the fast path is the write to file */
    if ( NULL != logfile )
    {
        fwrite( s_szLog, UtvStrlen( ( UTV_BYTE * )s_szLog ), 1, logfile );
    }
    /* The setup involves a few steps, but under normal circumstances is only performed once. */
    else if ( UtvStrlen( ( UTV_BYTE *) UTV_TEST_LOG_FNAME ) > 0 )
    {
        /* Send a copy to the log file */
        logfile = fopen( UTV_TEST_LOG_FNAME, "a" );
        if ( NULL != logfile )
        {
            fwrite( s_szLog, UtvStrlen( ( UTV_BYTE * )s_szLog ), 1, logfile );
        }
    }
#else
    /* Copy to the log file */
    if ( UtvStrlen( ( UTV_BYTE *) UTV_TEST_LOG_FNAME ) > 0 )
    {
        /* Send a copy to the log file */
        FILE * f = fopen( UTV_TEST_LOG_FNAME, "a" );
        if ( f )
        {
            fwrite( s_szLog, UtvStrlen( ( UTV_BYTE * )s_szLog ), 1, f );
            fclose( f );
        }
    }
#endif
#endif
}

void
UtvDataDump(void *dataIn, int dataInSize)
{
    /* Dumps dataInSize bytes of dataIn to theDump. Each line the following format:
     * 0020  74 65 3A 20 54 75 65 2C 20 32 33 20 4D 61 72 20   te: Tue, 23 Mar-0
     * 1234567890123456789012345678901234567890123456789012345678901234567890123
     *          1         2         3         4         5         6         7 ||
     *                                                                        |- '\0'
     *                                                                        - '\n'
     */

    unsigned char *p = dataIn;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};

    for(n = 1; n <= dataInSize; n++) {

        if ((n % 16) == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)dataIn) );
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);
        bytestr[0] = 0;

        /* store char str (for right side) */
        c = isprint(*p) ? *p : '.';
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);
        bytestr[0] = 0;

        if ((n % 16) == 0) { /* line completed */
            UtvMessage( UTV_LEVEL_INFO, "[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);

            hexstr[0] = 0;
            charstr[0] = 0;
        } else if ( n % 8 == 0 ) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }

        p++; /* next byte */
    }

    if (strlen(hexstr) > 0)
    {
        /* process rest of buffer if not empty */
        UtvMessage( UTV_LEVEL_INFO, "[%4.4s]   %-50.50s  %s", addrstr, hexstr, charstr);
    }

}

#endif /* UTV_TEST_MESSAGES */
