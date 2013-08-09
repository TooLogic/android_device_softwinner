/* corepacketfilter.c: UpdateTV Personality Map - MPEG-2 Packet Filter

  This module contains code that assembles Transport Stream packets into section data.

  In order to ensure UpdateTV network compatibility, customers must not 
  change UpdateTV Common Core code.

  Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

  Revision History (newest edits added to the top)

  Who             Date        Edit
  Bob Taylor      08/06/2008  Converted from C++ to C.
  Bob Taylor      06/17/2008  Added UTV_TID_ANY support.
  Bob Taylor      06/21/2007  Fixed extra free error.
  Bob Taylor      05/01/2007  Removed unused variables.
  Greg Scott      09/29/2005  Fix packet assembler's occasional trait of ending sections early.
  Greg Scott      08/01/2005  Remove unused core debugging code.
  Greg Scott      06/10/2005  Move CRC32 support to coreparse.cpp.
  Greg Scott      01/15/2005  Add CRC32 support.
  Greg Scott      11/22/2004  Creation for SDK 1.1.
*/

#include "utv.h"

#ifdef UTV_CORE_PACKET_FILTER

void UtvAssembleTransportStreamPacket( UTV_UINT32 iDesiredPid, UTV_UINT32 iDesiredTableId, UTV_UINT32 iDesiredTableIdExt, UTV_BYTE * pTransportStreamPacket );

/* Local storage for the packet assembler
*/
typedef struct
{
    UTV_BYTE * pSectionData;
    UTV_UINT32 iSectionData;
    UTV_UINT32 iSectionLength;
    UTV_UINT32 iPidContinuity;
}  UtvPacketAssembler;

UtvPacketAssembler gUtvPacketAssembler = { 0 };

/* UtvDeliverTransportStreamPacket
    Get one MPEG-2 Transport Stream packet from the Personality Map.
    Checks for the MPEG-2 Transport Stream sync byte.
   Returns NULL or a pointer to the 188 byte buffer
*/
void UtvDeliverTransportStreamPacket( UTV_UINT32 iDesiredPid, UTV_UINT32 iDesiredTableId, UTV_UINT32 iDesiredTableIdExt, UTV_BYTE * pTransportStreamPacket )
{
    /* Return if sync byte not present or bad pointer */
    if ( pTransportStreamPacket != NULL )
    {
        /* We have a packet is this a sync byte at the beginning */
        if ( pTransportStreamPacket[ 0 ] != 0x47 )
        {
            /* Discard this packet */
            UtvMessage( UTV_LEVEL_TRACE, "    missing sync byte on delivery of transport stream packet" );
            return;
        }
    }

    /* Deliver the 188 byte packet */
    UtvAssembleTransportStreamPacket( iDesiredPid, iDesiredTableId, iDesiredTableIdExt, pTransportStreamPacket );
}

/* UtvDeliverTransportStreamReset
    Resets the data so that assembly of a new packet will begin
*/
void UtvDeliverTransportStreamReset( void )
{
    /* Now forget about that data */
    gUtvPacketAssembler.iSectionLength = 0;
    gUtvPacketAssembler.iSectionData = 0;
    if ( NULL != gUtvPacketAssembler.pSectionData )
    {
        UtvFree( gUtvPacketAssembler.pSectionData );
        gUtvPacketAssembler.pSectionData = NULL;
    }
}

/* UtvAssembleTransportStreamPacket
    Get one section worth of data from the input packet stream given a PID, table id, table id ext.
    Calls UtvGetTransportStreamPacket to get 188 bytes of data at a time
   Returns pointer to data or NULL if errors
*/
void UtvAssembleTransportStreamPacket( UTV_UINT32 iDesiredPid, UTV_UINT32 iDesiredTableId, UTV_UINT32 iDesiredTableIdExt, UTV_BYTE * pData )
{
    UTV_UINT32 iSize = UTV_MPEG_PACKET_SIZE;
    UTV_UINT16 iHeader;
    UTV_BOOL   bTransportError;
    UTV_BOOL   bPayloadUnitStartIndicator;
    UTV_UINT32 iPid;
    UTV_UINT8  iHeader2;
    UTV_BOOL   bAdaptationFieldPresent;
    UTV_BOOL   bPayloadPresent;
    UTV_UINT32 iContinuity;
    UTV_UINT32 iPointerField;
    UTV_BYTE * pPacketPayload;
    UTV_UINT32 iPacketPayload;
    UTV_UINT32 iTableId;
    UTV_UINT16 iSectionHeader;
    UTV_UINT32 iTableIdExt;

    /* Check the incoming pointer, get size */
    if ( pData == NULL )
        return;

    /* Check sync byte again */
    if ( UtvGetUint8Pointer( &pData, &iSize ) != 0x47 )
    {
        /* Dump the section in progress */
        UtvDeliverTransportStreamReset();
        UtvMessage( UTV_LEVEL_TRACE, "    missing sync byte when assembling transport stream packet" );
    }

    /* Still have sync, get 2 bytes of header */
    iHeader = UtvGetUint16Pointer( &pData, &iSize );
    bTransportError = ( iHeader & 0x8000 ) == 0 ? UTV_FALSE : UTV_TRUE;
    bPayloadUnitStartIndicator = ( iHeader & 0x4000 ) == 0 ? UTV_FALSE : UTV_TRUE;
    iPid = iHeader & 0x1FFF;

    /* If the pid had a transport error, discard it */
    if ( bTransportError )
    {
        /* UtvMessage( UTV_LEVEL_TRACE, "    packet transport error seen on pid 0x%X", iPid ); */
        return;
    }

    /* If wrong pid, discard it */
    if ( iPid != iDesiredPid )
    {
        /* UtvMessage( UTV_LEVEL_TRACE, "    wrong pid seen, got 0x%X when expecting 0x%X", iPid, iDesiredPid ); */
        return;
    }

    /* Yes good data, PID matches, continue to get the scramble, adaptation, continuity counter */
    iHeader2 = UtvGetUint8Pointer( &pData, &iSize );
    bAdaptationFieldPresent = ( iHeader2 & 0x20 ) == 0 ? UTV_FALSE : UTV_TRUE;
    bPayloadPresent  = ( iHeader2 & 0x10 ) == 0 ? UTV_FALSE : UTV_TRUE;
    iContinuity = iHeader2 & 0x0F;

    /* Now see if the adaptation field is present, if so skip over it */
    if ( bAdaptationFieldPresent ) 
    {
        /* Next byte is the adaptation_field_length */
        UtvSkipBytesPointer( UtvGetUint8Pointer( &pData, &iSize ), &pData, &iSize );
    }

    /* This packet has a payload? */
    if ( ! bPayloadPresent )
        return;

    /* Now see if this is the first packet with this table */
    iPointerField = 0;

    /* Is this payload the start of a section? */
    if ( bPayloadUnitStartIndicator )
    {
        /* Yes, get the offset to the start of the data in this packet, reset continuity */
        iPointerField = UtvGetUint8Pointer( &pData, &iSize );
        gUtvPacketAssembler.iPidContinuity = iContinuity;
    } 
    else 
    {
        /* No PUSI, continuation from a previous packet, so check the continuity */
        if ( ( ( gUtvPacketAssembler.iPidContinuity + 1 ) & 0xF ) == iContinuity )
        {
            /* Continuity was good, packet is ok, remember this packet's continuity for next time */
            gUtvPacketAssembler.iPidContinuity = iContinuity;
        }
        else
        {
            /* Packet damaged or out of continuity, it is ok to dump the section in progress */
            UtvDeliverTransportStreamReset();
            /* UtvMessage( UTV_LEVEL_TRACE, "    continuity error in transport stream for pid 0x%X got 0x%X expected 0x%X", iDesiredPid, ( ( gUtvPacketAssembler.iPidContinuity + 1 ) & 0xF ), iContinuity ); */
            return;
        }
    }

    /* If the payload checked out so far, we need to check the payload */
    pPacketPayload = pData + iPointerField;
    iPacketPayload = iSize;

    /* If first packet in the section we need to check out the contents of the section */
    if ( bPayloadUnitStartIndicator && gUtvPacketAssembler.pSectionData == NULL )
    {
        /* Get important data from the section but only if enough section data present */
        if ( iSize >= 5 )
        {
            /* First byte is the table id, see if it is one we are looking for */
            iTableId = UtvGetUint8Pointer( &pData, &iSize );

            /* See if the table id matches */
            if ( UTV_TID_ANY == iDesiredTableId || iTableId == iDesiredTableId ) 
            {
                /* PID and TID match, now we are really interested in this packet, get section length */
                iSectionHeader = UtvGetUint16Pointer( &pData, &iSize );
                /*BOOL bSectionSyntaxIndicator = ( iSectionHeader & 0x8000 ) == 0 ? false : true; */
                gUtvPacketAssembler.iSectionLength = (iSectionHeader & 0xFFF) + 3;
                
                /* Get the table id extension */
                iTableIdExt = UtvGetUint16Pointer( &pData, &iSize );

                /* See if we want this table id extension */
                if ( ( 0 == iDesiredTableIdExt ) || ( iDesiredTableIdExt != 0 && ( iTableIdExt == iDesiredTableIdExt ) ) )
                {
                    /* Now we know how much data is in this section, so get memory for the whole section */
                    gUtvPacketAssembler.pSectionData = (UTV_BYTE *)UtvMalloc( gUtvPacketAssembler.iSectionLength );
                    if ( gUtvPacketAssembler.pSectionData != NULL )
                    {
                        /* Copy the remainder of this packet as the data for the section */
                        if ( gUtvPacketAssembler.iSectionLength < iPacketPayload )
                        {
                            /* All data is entirely inside this packet */
                            UtvByteCopy( gUtvPacketAssembler.pSectionData, pPacketPayload, gUtvPacketAssembler.iSectionLength );
                            gUtvPacketAssembler.iSectionData = gUtvPacketAssembler.iSectionLength;
                        } 
                        else 
                        {
                            /* Only the first portion was inside this packet */
                            UtvByteCopy( gUtvPacketAssembler.pSectionData, pPacketPayload, iPacketPayload );
                            gUtvPacketAssembler.iSectionData = iPacketPayload;
                        }
                    }
                }
            }
        }
    } 
    else 
    {
        /* Are we building a section right now? */
        if ( gUtvPacketAssembler.pSectionData != NULL )
        {
            /* Appending to section, see if last packet of payload for this section */
            if ( ( gUtvPacketAssembler.iSectionData + iPacketPayload ) <= gUtvPacketAssembler.iSectionLength )
            {
                /* Entire packet has data for this section */
                UtvByteCopy( gUtvPacketAssembler.pSectionData + gUtvPacketAssembler.iSectionData, pPacketPayload, iPacketPayload );
                gUtvPacketAssembler.iSectionData += iPacketPayload;
            } 
            else 
            {
                /* Only a portion of this packet has data for this section */
                UtvByteCopy( gUtvPacketAssembler.pSectionData + gUtvPacketAssembler.iSectionData, pPacketPayload, ( gUtvPacketAssembler.iSectionLength - gUtvPacketAssembler.iSectionData ) );
                gUtvPacketAssembler.iSectionData += ( gUtvPacketAssembler.iSectionLength - gUtvPacketAssembler.iSectionData );
            }
        }
    }

    /* Is it time to stand up and deliver? */
    if ( gUtvPacketAssembler.iSectionData > 0 && gUtvPacketAssembler.iSectionData == gUtvPacketAssembler.iSectionLength )
    {
        /* Yes, deliver the assembled packets as one section of data to the caller */
        UtvDeliverSectionFilterData( gUtvPacketAssembler.iSectionData, gUtvPacketAssembler.pSectionData );

        /* Get ready for the next section */
        UtvDeliverTransportStreamReset();
    }
}

/* UtvVerifySectionData
    Verify the CRC32 checksum at the end of each block of section data
   Returns UTV_OK if ok, error code if error
*/
UTV_RESULT UtvVerifySectionData( UTV_BYTE * pSectionData, UTV_UINT32 iSectionLength )
{
    /* Get CRC-32 for this section(last 4 bytes) */
    UTV_BYTE * pData = pSectionData;
    UTV_UINT32 iSize = iSectionLength;
    UTV_UINT32 iCrc32Section;

    UtvStartSectionPointer( &pData, &iSize );
    UtvSkipBytesPointer( iSize - 4, &pData, &iSize );
    iCrc32Section = UtvGetUint32Pointer( &pData, &iSize );

    /* If the CRC32 is zero then there is no CRC32 for this section */
    if ( iCrc32Section == 0 )
        return UTV_OK;

    /* Try forward CRC32, DSM-CC, PSIP, PSI compatible */
    if ( UtvCrc32( pSectionData, iSectionLength - 4 ) == iCrc32Section )
        return UTV_OK;

    /* Bad CRC32, return errors */
    UtvMessage( UTV_LEVEL_TRACE, "    bad CRC32 tid 0x%X, size %d", pSectionData[ 0 ], iSectionLength );
    return UTV_BAD_CHECKSUM;
}

#endif /* UTV_CORE_PACKET_FILTER */
