/* platform-os-linux-getnetwork.c: UpdateTV Personality Map - Linux Get Network functions

   Porting this module to another operating system should be easy if it supports
   the standard C runtime library calls. Therefore, this file will probably
   not be modified by customers.

   Copyright (c) 2007 - 2010 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Mac Stevens     11/02/2010  Created.
*/

#if defined(UTV_LOCAL_SUPPORT) || defined(UTV_LIVE_SUPPORT)

#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/netdevice.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "utv.h"



/*
  Parse route line into struct.

  Adds the following information to the routes information:

    {"Route to IP_ADDRESS_1" : {"Gateway" :         "192.168.10.101",
                                "Subnet Mask" :     "255.255.255.255",
                                "Metric" :          1234,
                                "Reference Count" : 1234,
                                "Use" :             1234,
                                "Interface" :       1234}}
*/

static UTV_RESULT s_ParseRoute( char* pubBuf, cJSON* routes )
{ 
  UTV_RESULT result = UTV_INVALID_PARAMETER;
  UTV_INT32 iCnt;
  UTV_UINT32 uiDestination;
  UTV_UINT32 uiGateway;
  UTV_UINT32 uiMask;
  UTV_UINT32 uiMetric;
  UTV_UINT32 uiRefCnt;
  UTV_UINT32 uiUse;
  UTV_INT8 sInterface[128];
  UTV_INT8 sRouteInfo[512];
  UTV_INT8 sIpAddr[32];
  cJSON* routeObject = NULL;
  cJSON* routeAttrs = NULL;
  
  if ( NULL != routes && NULL != pubBuf )
  {
    /* Parse /proc/net/route format */
    iCnt = sscanf( pubBuf,
                   "%s %lX %lX %*u %lu %lu %lu %lX",
                   sInterface,
                   &uiDestination,
                   &uiGateway,
                   &uiRefCnt,
                   &uiUse,
                   &uiMetric,
                   &uiMask);

    if ( 7 == iCnt )
    {
      /* Create JSON sub node */
      routeAttrs = cJSON_CreateObject();

      /* Add route info to JSON node */
      memset( sIpAddr, 0x00, 32 );

      UtvConvertToIpV4String( uiGateway, sIpAddr, 32 );
      cJSON_AddStringToObject( routeAttrs, "Gateway", sIpAddr );
      UtvConvertToIpV4String( uiMask, sIpAddr, 32 );
      cJSON_AddStringToObject( routeAttrs, "Subnet Mask", sIpAddr );
      cJSON_AddStringToObject( routeAttrs, "Interface", sInterface );
      cJSON_AddNumberToObject( routeAttrs, "Metric", uiMetric );
      cJSON_AddNumberToObject( routeAttrs, "Reference Count", uiRefCnt );
      cJSON_AddNumberToObject( routeAttrs, "Use", uiUse );
      UtvConvertToIpV4String( uiDestination, sIpAddr, 32 );

      routeObject = cJSON_CreateObject();

      /* Add sub node to parent */
      sprintf( sRouteInfo, "Route to %s", sIpAddr );
      cJSON_AddItemToObject( routeObject, sRouteInfo, routeAttrs );

      cJSON_AddItemToArray( routes, routeObject );
  
      result = UTV_OK;
    }
  }

  return ( result );
}



/* 
  Get routes.

  Adds the following information to the routes information:

    {"Route to IP_ADDRESS_1" : { SEE s_ParseRoute },
     "Route to IP_ADDRESS_2" : { SEE s_ParseRoute }}
*/

static UTV_INT32 s_GetRoutesInfo( cJSON* routes )
{
  FILE* fp;
  UTV_RESULT result = UTV_OK;
  UTV_RESULT parseResult = UTV_INVALID_PARAMETER;
  UTV_INT8 ubReadBuffer[2048];

  memset( ubReadBuffer, 0x00, sizeof(ubReadBuffer) );

  fp = fopen( "/proc/net/route", "r" );

  if ( !fp )
  {
    result = UTV_INVALID_PARAMETER;
  }
  else
  {
    /* Initial line contains heading info */
    fgets( ubReadBuffer, sizeof(ubReadBuffer), fp );

    /* Parse lines */
    while ( fgets( ubReadBuffer, sizeof(ubReadBuffer), fp ) )
    {
      parseResult = s_ParseRoute( ubReadBuffer, routes );

      if ( UTV_OK != parseResult )
      {
        result = parseResult;
      }
    }

    fclose( fp );
  }

  return ( result );
}



/* 
  Get the interface stats for the specified interface.  This can be improved by fetching an
  array of the stats to avoid multiple file open/closes.

  Adds the following information to the interface object:

    {"Metrics" : {"Received Bytes" :              1234,
                  "Recieved Packets" :            1234,
                  "Received Compressed Packets" : 1234,
                  "Received Multicast Packets" :  1234,
                  "Received Errors" :             1234,
                  "Received Packets Dropped" :    1234,
                  "Received FIFO Errors" :        1234,
                  "Received Frame Errors" :       1234,
                  "Transmitted Bytes" :           1234,
                  "Transmitted Packets" :         1234,
                  "Transmitted Errors" :          1234,
                  "Transmitted Packets Dropped" : 1234,
                  "Transmitted FIFO Errors" :     1234,
                  "Transmitted Collisions" :      1234,
                  "Transmitted Carrier Errors" :  1234}}
*/

static UTV_BOOL s_GetInterfaceMetrics( UTV_INT8* interfaceName, cJSON* interfaceObject )
{
  FILE* fp;
  UTV_RESULT result = UTV_UNKNOWN;
  UTV_INT8 ubReadBuffer[512];
  UTV_INT8 pszDeviceName[128];
  UTV_UINT32 uiRxBytes;
  UTV_UINT32 uiRxPackets;
  UTV_UINT32 uiRxErr;
  UTV_UINT32 uiRxDropped;
  UTV_UINT32 uiRxFifoErr;
  UTV_UINT32 uiRxFrameErr;
  UTV_UINT32 uiRxCompressed;
  UTV_UINT32 uiRxMulticast;
  UTV_UINT32 uiTxBytes;
  UTV_UINT32 uiTxPackets;
  UTV_UINT32 uiTxErr;
  UTV_UINT32 uiTxDropped;
  UTV_UINT32 uiTxFifoErr;
  UTV_UINT32 uiTxCollisions;
  UTV_UINT32 uiTxCarrierErr;
  UTV_UINT32 uiTxCompressed;
  UTV_INT32 iCnt;
  UTV_INT32 iFirstChar;
  UTV_INT32 iLastChar;
  cJSON* metrics;

  if ( NULL == interfaceName || NULL == interfaceObject )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  metrics = cJSON_CreateObject();

  memset( ubReadBuffer, 0x00, sizeof(ubReadBuffer) );

  fp = fopen( "/proc/net/dev", "r" );

  if ( !fp )
  {
    result = UTV_FILE_OPEN_FAILS;
  }
  else
  {
    /* Initial lines contain heading info */
    fgets( ubReadBuffer, sizeof(ubReadBuffer), fp );
    fgets( ubReadBuffer, sizeof(ubReadBuffer), fp );
     
    /* Parse lines */
    while ( fgets( ubReadBuffer, sizeof(ubReadBuffer), fp ) )
    {
      iFirstChar = 0;

      /* Skip any leading spaces */
      while ( ubReadBuffer[ iFirstChar ] == ' ' )
      {
        iFirstChar++;
      }

      iLastChar = iFirstChar + 1;

      /* Find the separator character, ':' */
      while ( ubReadBuffer[ iLastChar ] != ':' )
      {
        iLastChar++;
      }

      /* Copy the device name out of the buffer and NULL terminate it */
      strncpy( pszDeviceName, &ubReadBuffer[iFirstChar], iLastChar - iFirstChar );
      pszDeviceName[ iLastChar - iFirstChar ] = '\0';

      /* Parse /proc/net/dev format */
      iCnt = sscanf( &ubReadBuffer[ iLastChar + 1 ],
                     "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                     &uiRxBytes,
                     &uiRxPackets,
                     &uiRxErr,
                     &uiRxDropped,
                     &uiRxFifoErr,
                     &uiRxFrameErr,
                     &uiRxCompressed,
                     &uiRxMulticast,
                     &uiTxBytes,
                     &uiTxPackets,
                     &uiTxErr,
                     &uiTxDropped,
                     &uiTxFifoErr,
                     &uiTxCollisions,
                     &uiTxCarrierErr,
                     &uiTxCompressed);

      if ( 16 == iCnt )
      {
        /* Check to see if this is the correct interface */
        if ( 0 == strncmp( interfaceName, pszDeviceName, sizeof(interfaceName) ) )
        {
          cJSON_AddNumberToObject( metrics, "Received Bytes", uiRxBytes );
          cJSON_AddNumberToObject( metrics, "Received Packets", uiRxPackets );
          cJSON_AddNumberToObject( metrics, "Received Compressed Packets", uiRxCompressed );
          cJSON_AddNumberToObject( metrics, "Received Multicast Packets", uiRxMulticast );
          cJSON_AddNumberToObject( metrics, "Received Errors", uiRxErr );
          cJSON_AddNumberToObject( metrics, "Received Packets Dropped", uiRxDropped );
          cJSON_AddNumberToObject( metrics, "Received FIFO Errors", uiRxFifoErr );
          cJSON_AddNumberToObject( metrics, "Received Frame Errors", uiRxFrameErr );
          cJSON_AddNumberToObject( metrics, "Transmitted Bytes", uiTxBytes );
          cJSON_AddNumberToObject( metrics, "Transmitted Packets", uiTxPackets );
          cJSON_AddNumberToObject( metrics, "Transmitted Compressed Packets", uiTxCompressed );
          cJSON_AddNumberToObject( metrics, "Transmitted Errors", uiTxErr );
          cJSON_AddNumberToObject( metrics, "Transmitted Packets Dropped", uiTxDropped );
          cJSON_AddNumberToObject( metrics, "Transmitted FIFO Errors", uiTxFifoErr );
          cJSON_AddNumberToObject( metrics, "Transmitted Collisions", uiTxCollisions );
          cJSON_AddNumberToObject( metrics, "Transmitted Carrier Errors", uiTxCarrierErr );

          result = UTV_OK;

          break;
        }
      }
    }

    fclose( fp );
  }

  cJSON_AddItemToObject( interfaceObject, "Metrics", metrics );

  return ( result );
}



/*
  Get network interface data

  Add the following information to the interfaces object:

    {"NAME_1" : {"Type" :               "Wired/Wireless",
                 "Status" :             "Disabled/Connected/Disconnected",
                 "Network Name" :       "MyNet",
                 "Security Key" :       "theFamilyNet!",
                 "DHCP" :               "Enabled/Disabled",
                 "IP Address" :         "192.169.10.100",
                 "Subnet Mask" :        "255.255.255.0",
                 "Gateway" :            "192.168.10.101",
                 "Physical Address" :   "AB:CD:EF:01:23:45",
                 "First DNS Server" :   "192.168.2.1",
                 "Second DNS Server" :  "68.87.72.134",
                 "Third DNS Server" :   "68.87.72.135",
                 "Broadcast Address" :  "0.0.0.0",
                 "Metrics" :            { SEE s_GetInterfaceMetrics }},
     "NAME_2" : { SAME AS ABOVE WITH VALUE FOR ANOTHER INTERFACE }}

        NOTES:
          - The default implementation of this implementation only retrieves network
            information for active/enabled network interfaces.  If the device or platform
            supports gathering additional information please adjust the implementation
            accordingly.
          - The default implementation of this implementation is not able to retrieve all of
            the desired information.  Missing information includes:
              - Type
              - Status
              - Network Name
              - Security Key
              - DHCP
              - First DNS Server
              - Second DNS Server
              - Third DNS Server
            If the device or platform has access to this information please adjust the
            implementation accordingly.
*/
static UTV_RESULT s_GetInterfacesData( cJSON* interfaces, cJSON* routes )
{
  UTV_RESULT result = UTV_INVALID_PARAMETER;
  UTV_INT8 buf[1024];
  UTV_INT8 sMacAddr[32];
  UTV_INT32 sck;
  UTV_INT32 nInterfaces;
  UTV_INT32 i;
  cJSON* interfaceObject;
  cJSON* interfaceAttrs;
  struct ifconf ifc;
  struct ifreq *ifr;

  if ( NULL == interfaces )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  /* Get a socket handle. */
  sck = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( sck < 0 )
  {
    result = UTV_UNKNOWN;
  }
  else
  {
    /* Query available interfaces. */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;

    if ( ioctl( sck, SIOCGIFCONF, &ifc ) < 0 )
    {
      result = UTV_READ_FAILS;
    }
    else
    {
      /* Iterate through the list of interfaces. */
      ifr = ifc.ifc_req;
      nInterfaces = ifc.ifc_len / sizeof(struct ifreq);

      for ( i = 0; i < nInterfaces; i++ )
      {
        interfaceAttrs = cJSON_CreateObject();

        cJSON_AddStringToObject( interfaceAttrs, "Type", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "Status", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "Network Name", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "Security Key", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "DHCP", "Not Available" );

        struct ifreq* item = &ifr[i];

        /* Output the device name and IP address */

        cJSON_AddStringToObject(
          interfaceAttrs,
          "IP Address", 
          inet_ntoa(((struct sockaddr_in *)&item->ifr_addr)->sin_addr) );

        /* Get the subnet mask */
        if ( ioctl( sck, SIOCGIFNETMASK, item ) >= 0 )
        {
          cJSON_AddStringToObject(
            interfaceAttrs,
            "Subnet Mask", 
            inet_ntoa(((struct sockaddr_in *)&item->ifr_netmask)->sin_addr) );
        }
        else
        {
          cJSON_AddStringToObject( interfaceAttrs, "Subnet Mask", "Not Available" );
        }

        /* Attempt to find the gateway for this interface. */
        if ( NULL != routes )
        {
          cJSON* routeGateway = NULL;
          cJSON* routeItem = routes->child;

          while ( NULL != routeItem && NULL == routeGateway )
          {
            cJSON* route = routeItem->child;
            if ( 0 == strncmp( "Route to 0.0.0.0", route->string, strlen("Route to 0.0.0.0") ) )
            {
              cJSON* routeIf = cJSON_GetObjectItem( route, "Interface" );

              if ( 0 == strncmp( item->ifr_name, routeIf->valuestring, sizeof(item->ifr_name) ) )
              {
                routeGateway = cJSON_GetObjectItem( route, "Gateway" );
                break;
              }
            }

            routeItem = routeItem->next;
          }

          /*
           * If a gateway was found then add its value to the interface object, otherwise add
           * the "N/A" value indicating a gateway could not be determined.
           */

          cJSON_AddStringToObject(
            interfaceAttrs,
            "Gateway",
            (routeGateway != NULL ? routeGateway->valuestring : "Not Available") );
        }

        /* Get the physical address */
        if ( ioctl( sck, SIOCGIFHWADDR, item ) >= 0 )
        {
          sprintf( sMacAddr,
                   "%02lx:%02lx:%02lx:%02lx:%02lx:%02lx",
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[0],
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[1],
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[2],
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[3],
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[4],
                   (UTV_INT32) ((UTV_UINT8*) &item->ifr_hwaddr.sa_data)[5]); 
        
          cJSON_AddStringToObject( interfaceAttrs, "Physical Address", sMacAddr );
        }
        else
        {
          cJSON_AddStringToObject( interfaceAttrs, "Physical Address", "Not Available" );
        }

        cJSON_AddStringToObject( interfaceAttrs, "First DNS Server", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "Second DNS Server", "Not Available" );
        cJSON_AddStringToObject( interfaceAttrs, "Third DNS Server", "Not Available" );

        /* Get the broadcast address */
        if ( ioctl( sck, SIOCGIFBRDADDR, item ) >= 0 )
        {
          cJSON_AddStringToObject(
            interfaceAttrs,
            "Broadcast Address", 
            inet_ntoa( ((struct sockaddr_in*)&item->ifr_broadaddr)->sin_addr) );
        }
        else
        {
          cJSON_AddStringToObject( interfaceAttrs, "Broadcast Address", "Not Available" );
        }

        /* Get interface statistics */
        s_GetInterfaceMetrics( item->ifr_name, interfaceAttrs );

        interfaceObject = cJSON_CreateObject();
        cJSON_AddItemToObject( interfaceObject, item->ifr_name, interfaceAttrs );

        cJSON_AddItemToArray( interfaces, interfaceObject );

        result = UTV_OK;
      }
    }
  }

  return ( result );
}



/* UtvPlatformGetNetworkInfo
   Populates a CJSON struct with relevant network information

   Add the following information to the response data:

      {"Interfaces" : [{"NAME_1" : {"Type" : "Wired/Wireless",
                                    "Status" : "Disabled/Connected/Disconnected",
                                    "Network Name" : "MyNet",
                                    "Security Key" : "theFamilyNet!",
                                    "DHCP" : "Enabled/Disabled",
                                    "IP Address" : "192.169.10.100",
                                    "Gateway" : "192.168.2.1",
                                    "Subnet Mask" : "255.255.255.0",
                                    "Physical Address" : "AB:CD:EF:01:23:45",
                                    "First DNS Server" : "192.168.2.1",
                                    "Second DNS Server" : "68.87.72.134",
                                    "Third DNS Server" : "68.87.72.135",
                                    "Broadcast Address" : "0.0.0.0",
                                    "Metrics" : {"Received Bytes" : 1234,
                                                 "Recieved Packets" : 1234,
                                                 "Received Compressed Packets" : 1234,
                                                 "Received Multicast Packets" : 1234,
                                                 "Received Errors" : 1234,
                                                 "Received Packets Dropped" : 1234,
                                                 "Received FIFO Errors" : 1234,
                                                 "Received Frame Errors" : 1234,
                                                 "Transmitted Bytes" : 1234,
                                                 "Transmitted Packets" : 1234,
                                                 "Transmitted Errors" : 1234,
                                                 "Transmitted Packets Dropped" : 1234,
                                                 "Transmitted FIFO Errors" : 1234,
                                                 "Transmitted Collisions" : 1234,
                                                 "Transmitted Carrier Errors" : 1234}}},
                       {"NAME_2" : { SAME AS ABOVE }}],
       "Routes" : [{"Route to IP_ADDRESS_1" : {"Gateway" : "192.168.2.1",
                                               "Subnet Mask" : "255.255.255.255",
                                               "Metric" : 1234,
                                               "Reference Count" : 1234,
                                               "Use" : 1234,
                                               "Interface" : 1234},
                   {"Route to IP_ADDRESS_1" : { SAME AS ABOVE }}]}}

   NOTES:
     - The default implementation of this implementation only retrieves network
       information for active/enabled network interfaces.  If the device or platform
       supports gathering additional information please adjust the implementation
       accordingly.
     - The default implementation of this implementation is not able to retrieve all of
       the desired information.  Missing information includes:
         - Type
         - Status
         - Network Name
         - Security Key
         - DHCP
         - First DNS Server
         - Second DNS Server
         - Third DNS Server
       If the device or platform has access to this information please adjust the
       implementation accordingly.
*/
UTV_RESULT UtvPlatformGetNetworkInfo( void* pVoidResponseData )
{
  UTV_RESULT resultInterfaces = UTV_OK;
  UTV_RESULT resultRoutes = UTV_OK;
  cJSON* responseData = (cJSON*)pVoidResponseData;
  cJSON* interfaces = cJSON_CreateArray();
  cJSON* routes = cJSON_CreateArray();

  if ( NULL == pVoidResponseData )
  {
    return ( UTV_INVALID_PARAMETER );
  }

  resultRoutes = s_GetRoutesInfo( routes );
  resultInterfaces = s_GetInterfacesData( interfaces, routes );

  cJSON_AddItemToObject( responseData, "Interfaces", interfaces );
  cJSON_AddItemToObject( responseData, "Routes", routes );

  /*
   * If there was an issue retrieving the results information return that error, otherwise
   * return the result from the routes retrieval.
   */

  return ( ( UTV_OK != resultInterfaces ) ? resultInterfaces : resultRoutes );
}

#endif /* UTV_LOCAL_SUPPORT || UTV_LIVE_SUPPORT */
