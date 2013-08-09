/* linux-execute.cpp: UpdateTV Personality Map - File Execution Routines for Linux

   This module is responsible for the implementation of the routine UtvExecute() which 
   executes a named file. Customers should not have to modify this file.

  Copyright (c) 2007 - 2008 UpdateLogic Incorporated. All rights reserved.

   Revision History

   Who          Date        Edit
   Bob Taylor   08/06/2008  Converted from C++ to C.
   Bob Taylor   07/25/2007  Added UTV_EXECUTE_FAILED error and iSysErr return.
   Bob Taylor   04/30/2007  Creation.
*/

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "utv.h"

/* UtvExecute
    Executes the named file.
    PLEASE NOTE:  this function is provided for simple testing purposes only.
    In a production environment the Agent will need to write the update to flash,
    inform the boot ROM that a new image is available, consult with the user whether
    they wat to accept the update, and then cause a reboot.
   Returns OK if execution success.
   Returns UTV_UNKNOWN otherwise.
*/
UTV_RESULT UtvExecute( char *szExecutable, UTV_UINT32 *iSystemErr )
{
    /* Make the file executable */
    if ( -1 == chmod( szExecutable, S_IRWXU | S_IRWXG | S_IRWXO ) )
    {
        *iSystemErr = errno;
        UtvMessage( UTV_LEVEL_ERROR, "  Cannot change file attribute of \"%s\" to make executable", szExecutable );
        return UTV_EXECUTE_FAILED;
    }

    /* execute it */
    if ( -1 == execl( szExecutable, szExecutable, (char *) NULL ) )
    {
        *iSystemErr = errno;
        UtvMessage( UTV_LEVEL_ERROR, "Launch of program \"%s\" failed: %d", szExecutable, errno );
        return UTV_EXECUTE_FAILED;
    }

    return UTV_OK;
}

