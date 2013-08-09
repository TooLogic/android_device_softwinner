/* persmemory.cpp: UpdateTV Personality Map - Operating System Memory Management

   Each of these calls map into a C runtime library call.

   Porting this module to another operating system should be easy if it supports
   the standard C runtime library calls. Therefore, this file will probably
   not be modified by customers.

   Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Bob Taylor      08/06/2008  Converted from C++ to C, unused code eliminated.
   Greg Scott      10/20/2004  Creation.
*/

#undef __STRICT_ANSI__            /* to enable vsnprintf */
#include <stdio.h>
#include <memory.h>
#include <stdlib.h>               /*for malloc */
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

#include "utv.h"

/* UtvMalloc
    Defaults to use the regular C runtime malloc() library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns pointer to newly allocated storage or NULL if no memory available
*/
void * UtvMalloc( UTV_UINT32 iSize )
{
    /* Grab the memory */
    void * pMemoryBlock = malloc( iSize );

    /* Return null in the unlikely event that we didn't get any memory */
    if ( pMemoryBlock == NULL )
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvMalloc failed to allocate a memory block of 0x%X bytes", iSize );
        return NULL;
    }

    /* Now clear the memory we just got */
    memset( pMemoryBlock, 0, iSize );

    /* Return a pointer to the memory */
    return pMemoryBlock;
}

/* UtvFree
    Defaults to use the regular C runtime free() library call.
    This code should NOT need to be changed when porting to new operating systems
   Returns the memory block allocated by UtvMalloc.
*/
void UtvFree( void * pMemoryBlock )
{
    if ( pMemoryBlock != NULL )
        free( pMemoryBlock );
}

/* UtvByteCopy
    Virtualizes memcpy
    This code should NOT need to be changed when porting to new operating systems
*/
void UtvByteCopy( UTV_BYTE * pStore, UTV_BYTE * pData, UTV_UINT32 iSize )
{
    /* Just do the memcpy */
    memcpy( pStore, pData, iSize );
}

/* UtvMemset
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
*/
void UtvMemset( void * pData, UTV_BYTE bData, UTV_UINT32 iSize )
{
    /* Just do the memset */
    memset(pData, bData, iSize);
}

/* UtvMemcmp
    Uses the regular C runtime library call.
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_INT32 UtvMemcmp( void * pData1, void * pData2, UTV_UINT32 iSize )
{
    /* Just do the memcmp */
    return memcmp(pData1, pData2, iSize);
}

/* UtvStrnCopy
    Virtualizes the standard C runtime library call, strncpy
    Will always NUL terminate the destination string
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_BYTE * UtvStrnCopy( UTV_BYTE * pStore, UTV_UINT32 iBuffSize, UTV_BYTE * pSource, UTV_UINT32 iCount )
{
    iBuffSize --;

    if ( iCount > iBuffSize )
        iCount = iBuffSize;

    strncpy( ( char * )pStore, ( const char * )pSource, iCount );
    pStore[ iCount ] = '\0';

    return pStore;
}

/* UtvStrlen
    Virtualizes the standard C runtime library call, strlen
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_UINT32 UtvStrlen( UTV_BYTE * pData )
{

    return ( (UTV_UINT32)strlen( ( const char * )pData ) );
}

/* UtvStrcat
    Virtualizes the standard C runtime library call, strcat
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_BYTE *UtvStrcat( UTV_BYTE * pDst, UTV_BYTE * pSrc )
{
    return ( (UTV_BYTE *)strcat( (char * )pDst, (char *)pSrc ) );
}

/* UtvStrtoul
    Virtualizes the standard C runtime library call, stroul
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_UINT32 UtvStrtoul( UTV_INT8 * pString, UTV_INT8 ** ppEndPtr, UTV_UINT32 uiBase )
{
    return ( (UTV_UINT32)strtoul( (char * )pString, (char **)ppEndPtr, uiBase ) );
}

/* UtvMemFormat
    Takes a format string and parameters and generates them into the callers buffer
    This code should NOT need to be changed when porting to new operating systems
*/
UTV_UINT32 UtvMemFormat( UTV_BYTE * pszData, UTV_UINT32 iBuffSize, char * pszFormat, ... )
{
    UTV_UINT32 iCount = 0;
    va_list va;

    /* Format the data */
    va_start( va, pszFormat );
    iCount = vsnprintf( (char *)pszData, iBuffSize, pszFormat, va );
    va_end( va );

    /* see of the output is truncated */
    if ( iCount >= iBuffSize )
	{
        UtvMessage( UTV_LEVEL_ERROR, "UtvMemFormat() TRUNCATED output.  Buffer should have been %u bytes long, but is %u bytes long", iCount, iBuffSize );
		iCount = iBuffSize;
	}
	
    return iCount;
}
