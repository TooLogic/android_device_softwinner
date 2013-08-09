/* coreparse.c: UpdateTV Common Core - UpdateTV Section Data Parse Functions

  This module implements common UpdateTV transport stream parsing support.

  In order to ensure UpdateTV network compatibility, customers must not 
  change UpdateTV Common Core code.

  Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who              Date        Edit
  Bob Taylor       08/06/2008  Converted from C++ to C.
  Bob Taylor       10/26/2007  Moved UtvCrc32() to common-utility.cpp.
  Chris Nevin      05/12/2006  Change cast in UtvConvertLEIntToInt to platform specific UTV_UINT32
  Chris Nevin      12/13/2005  Added functions to perform byte swapping for endian agnostic support
  Greg Scott       10/20/2004  Creation.
*/

#include "utv.h"

/* UtvStartParsePointer
    Start out a section pointer, assumes section filter has checked syntax
   Removes size of CRC32 field from the size as we don't look at it
*/
UTV_TID UtvStartSectionPointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    UTV_TID iTableId = (UTV_TID)*(*ppData);
    (*ppData)++;
    *piSize = *(*ppData) << 8;
    (*ppData)++;
    *piSize |= *(*ppData);
    *piSize &= 0xFFF;
    (*ppData)++;
    return iTableId;
}

/* UtvGetUint16Pointer
   Return 16 bit number from the stream of bytes, incrementing pointer
*/
UTV_UINT8 UtvGetUint8Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    UTV_UINT8 rValue;

    if ( *piSize == 0 ) 
        return 0;

    if ( *piSize > 0 )
        *piSize -= 1;

    rValue = **(UTV_UINT8 **)ppData;
    *ppData += 1;
    return rValue;
}

/* UtvGetUint16Pointer
   Return 16 bit number from the stream of bytes, incrementing pointer
*/
UTV_UINT16 UtvGetUint16Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    UTV_UINT16 rValue = UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    return rValue;
}

/* UtvGetUint32Pointer
   Return 32 bit number from the stream of bytes, incrementing pointer
*/
UTV_UINT32 UtvGetUint32Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    UTV_UINT32 rValue = UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    return rValue;
}

/* UtvGetUint32Pointer
   Return 24 bit number in a 32 bit field, incrementing pointer
*/
UTV_UINT32 UtvGetUint24Pointer( UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    UTV_UINT32 rValue = UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    rValue = ( rValue << 8 ) | UtvGetUint8Pointer( ppData, piSize );
    return rValue;
}

/* UtvSkipBytesPointer
    Skip the number of bytes specified
*/
void UtvSkipBytesPointer( UTV_UINT32 iSkip, UTV_BYTE * * ppData, UTV_UINT32 * piSize )
{
    if ( iSkip == 0 )
        return;
    if ( iSkip > *piSize )
        *piSize = 0;
    else 
        *piSize -= iSkip;
    if ( *piSize > 0 ) 
        *ppData += iSkip;
}
