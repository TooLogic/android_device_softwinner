/* os-defines.h: Os-specific definitions for Linux

 Copyright (c) 2007 - 2008 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who           Date        Edit
  Jim Muchow    07/16/2010  Update thread exit macros for new threads accounting
  Bob Taylor    08/05/2008  Converted from C++ to C.
  Bob Taylor    09/06/2007  Moved time.h include to here from utv.h.
  Bob Taylor    05/14/2007  Original.
*/

#include <time.h>
#define UTV_TIME time_t

typedef unsigned long UTV_THREAD_HANDLE;
#define UTV_THREAD_ROUTINE void *
typedef void *(*PTHREAD_START_ROUTINE)(void *);
#define UTV_THREAD_START_ROUTINE PTHREAD_START_ROUTINE
#define UTV_THREAD_EXIT    return(UtvThreadExit());
