/* platform-os-linux-getsystemstats.c: UpdateTV Personality Map - Linux Get System Stats

   Porting this module to another operating system should be easy if it supports
   the standard C runtime library calls. Therefore, this file will probably
   not be modified by customers.

   Copyright (c) 2007 - 2010 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Mac Stevens     11/02/2010  Created.
*/

#ifdef UTV_LIVE_SUPPORT

#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "utv.h"



/* UtvGetSystemStats
   Populates a CJSON struct with relevant system stats information
*/
UTV_RESULT UtvPlatformGetSystemStats( void* pVoidResponseData )
{
  UTV_RESULT result = UTV_OK;
  UTV_INT8 ubReadBuffer[1024];
  FILE* fpVmStat;
  UTV_UINT32 uiVmStatVer = 0;
  FILE* fp;
  UTV_UINT32 uiScanCount = 0;
  UTV_UINT32 uiMemFree = 0;
  UTV_UINT32 uiMemUsed = 0;
  UTV_UINT32 uiMemTotal = 0;
  UTV_UINT32 uiMemPercentUsed = 0;
  UTV_UINT32 uiCpuUser = 0;
  UTV_UINT32 uiCpuSystem = 0;
  UTV_UINT32 uiCpuTotalUsed = 0;
  cJSON* responseData = (cJSON*)pVoidResponseData;
  cJSON* memObject = NULL;
  cJSON* cpuObject = NULL;

  if (NULL != responseData)
  {
    memset(ubReadBuffer, 0x00, sizeof(ubReadBuffer));

    /* Get stats */
    fpVmStat = popen( "vmstat", "r" );

    if ( !fpVmStat )
    {
      result = UTV_FILE_OPEN_FAILS;
    }
    else
    {
      /* Parse first 2 headers... ignore this data */
      fgets( ubReadBuffer, sizeof(ubReadBuffer), fpVmStat );
      fgets( ubReadBuffer, sizeof(ubReadBuffer), fpVmStat );

      if ( 0 == strncmp( ubReadBuffer, " r  b  w", strlen( " r  b  w" ) ) )
      {
        uiVmStatVer = 2;
      }
      else if ( 0 == strncmp( ubReadBuffer, " r  b   swpd", strlen( " r  b   swpd" ) ) )
      {
        uiVmStatVer = 3;
      }

      /* Parse actual stats */
      fgets( ubReadBuffer, sizeof(ubReadBuffer), fpVmStat );

      if ( 2 == uiVmStatVer )
      {
        uiScanCount = sscanf( ubReadBuffer,
                              "%*u %*u %*u %*u %lu %*u %*u %*u %*u %*u %*u %*u %*u %lu %lu",
                              &uiMemFree,
                              &uiCpuUser,
                              &uiCpuSystem );
      }
      else if ( 3 == uiVmStatVer )
      {
        uiScanCount = sscanf( ubReadBuffer,
                              "%*u %*u %*u %lu %*u %*u %*u %*u %*u %*u %*u %*u %lu %lu",
                              &uiMemFree,
                              &uiCpuUser,
                              &uiCpuSystem );
      }

      if ( 3 != uiScanCount )
      {
        result = UTV_READ_FAILS;
      }
      else
      {
        /* Calculate total used CPU */
        uiCpuTotalUsed = uiCpuUser + uiCpuSystem;
      }
                      
      pclose( fpVmStat );
      fpVmStat = NULL;

      fp = fopen( "/proc/meminfo", "r" );

      if ( !fp )
      {
        result = UTV_FILE_OPEN_FAILS;
      }
      else
      {
        /* Parse lines */
        fgets( ubReadBuffer, sizeof(ubReadBuffer), fp );

        if ( 0 == sscanf( ubReadBuffer, "MemTotal: %lu", &uiMemTotal ) )
        {
          result = UTV_READ_FAILS;
        }
        else
        {
          /* Calculate memory percentage */
          if ( 0 != uiMemTotal )
          {
            uiMemUsed = uiMemTotal - uiMemFree;
            uiMemPercentUsed = round( ((float)uiMemUsed / (float)uiMemTotal) * 100 );
  
            /* Populate JSON object */
            memObject = cJSON_CreateObject();
            cpuObject = cJSON_CreateObject();

            cJSON_AddNumberToObject( memObject, "Percent Used", uiMemPercentUsed );
            cJSON_AddNumberToObject( memObject, "Free (MB)", uiMemFree );
            cJSON_AddNumberToObject( memObject, "Used (MB)", uiMemUsed );
            cJSON_AddNumberToObject( memObject, "Total (MB)", uiMemTotal );
            
            cJSON_AddNumberToObject( cpuObject, "Total Percent Used", uiCpuTotalUsed );
            cJSON_AddNumberToObject( cpuObject, "User Percent Used", uiCpuUser );
            cJSON_AddNumberToObject( cpuObject, "System Percent Used", uiCpuSystem );

            cJSON_AddItemToObject( responseData, "Memory", memObject );
            cJSON_AddItemToObject( responseData, "CPU", cpuObject );
          }
        }

        fclose( fp );
      }
    }
  }
  else
  {
    result = UTV_INVALID_PARAMETER;
  }
 
  return ( result );
}
#endif /* UTV_LIVE_SUPPORT */
