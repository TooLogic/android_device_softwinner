/* platform-data-broadcast-static-section.c: UpdateTV Core - Creates Static Section Data

   This code is used to create MPEG-2 section data when none is available.

   Customers should not need to modify this file when porting except to adjust the fake data defines below.

   Copyright (c) 2004 - 2009 UpdateLogic Incorporated. All rights reserved.

   Revision History (newest edits added to the top)

   Who              Date        Edit
   Bob Taylor       08/06/2008  Converted from C++ to C.
   Bob Taylor       11/26/2007  Added version MRD in DSI support and added defines that track UTV_NETWORK_VERSION and make
                                private data reformatting a little easier.
   Bob Taylor       07/21/2007  Replaced UtvLocaltime with UtvGmtime use.
   Bob Taylor       05/31/2007  Removed dependency on UTV_TEST_CAROUSEL_PID.
   Bob Taylor       05/02/2007  Made Module Base Name follow persconfig.h.
   Bob Taylor       04/30/2007  Ported from corecreate.cpp.  Made OUI and Model Group follow persconfig.h.
   Bob Taylor       08/30/2006  Network version changed to 2.  Fixes bug #135.
   Chris Nevin      06/28/2006  Changed calls to time, mktime, and localtime to UtvTime, UtvMktime, and UtvLocaltime
   Chris Nevin      01/31/2006  Added checks for out of memory conditions
   Greg Scott       06/29/2005  Support of new descriptors 
   Greg Scott       02/11/2005  Creation from persdebug.cpp.
*/

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "utv.h"

/* Fake Storage */
static UTV_UINT32 iUtvFakeBlockNumber = 0;

/* Faked information can be redefined here */

#define UTV_FAKE_TRANSACTION_ID    0xDEADBEEF
#define UTV_FAKE_MODULE_ID         0xBEEF
#define UTV_FAKE_PMT_PID           0xB00
#define UTV_FAKE_CAROUSEL_PID      0x6A
#define UTV_FAKE_PROGRAM_NUMBER    0x00B
#define UTV_FAKE_DDB_BLOCKSIZE     128
#define UTV_FAKE_DDB_BLOCKCOUNT    8
#define UTV_FAKE_BROADCAST_SECONDS 600
#define UTV_FAKE_OUI               0xFFFFFF
#define UTV_FAKE_MODEL_GROUP       0xFFFF
#define UTV_FAKE_MOD_NAME          "compdir-01of01"
#define UTV_FAKE_SMALLSECTION      188
#define UTV_FAKE_DATASECTION       188

/* UtvFakeNewPacket
  Allocates memory for a new fake section
*/
UTV_BYTE * UtvFakeNewPacket()
{
    UTV_BYTE * pFake = (UTV_BYTE *)UtvMalloc( UTV_FAKE_SMALLSECTION );
    if ( pFake != NULL )
        UtvMemset( pFake, 0xff, UTV_FAKE_SMALLSECTION );
    return pFake;
}

/* GPS-UTC conversion
    Add seconds from Jan 1, 1970 to Jan 6, 1980 (10 years, 2 leap days, 5 days from Jan 1 to 6
*/
#define UTV_GPS_CONVERSION ( ( 60 * 60 * 24 * 365 * 10 ) + ( 60 * 60 * 24 * 7 ) )
#define UTV_GPS_UTC_OFFSET 13

#define GROUPS_INFO_PRIVATE_DATA_LEN    22

/* UtvFakeDsi
    Generates a canned DSI section based on UTV_FAKE_OUI and UTV_FAKE_MODEL_GROUP
*/
UTV_BYTE * UtvFakeDsi( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    time_t tTime = (time_t)( (UTV_UINT32)UtvTime( NULL ) - ( UTV_GPS_CONVERSION + UTV_GPS_UTC_OFFSET ) );    
    struct tm * ptmGmt = UtvGmtime( &tTime );
    tTime = UtvMktime( ptmGmt );

    if ( (pFake = UtvFakeNewPacket()) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] = 0;
    pData++[0] = 1 + (2 + 1 + 1 + 1) + 2 + 2 + 4 + 1 + 2 + (20 + 2) + 2 + 2 + (4 + 4 + 2 + 13) + 2 + (2 + GROUPS_INFO_PRIVATE_DATA_LEN) + 4;  /* section_length */
    pData += 2;                 /* table id ext */
    pData++[0] = 0;             /* version  */
    pData++[0] = 0;             /* section number */
    pData++[0] = 0;             /* last section number */
    pData++[0] = UTV_DSMCC_PROTOCOL_DISCRIMINATOR;  /* dsmcc protocol */
    pData++[0] = UTV_DSMCC_UNM;         /* dsmcc user-network message */
    pData++[0] = UTV_DSMCC_DSI  >> 8;   /* dsmcc message type */
    pData++[0] = UTV_DSMCC_DSI & 0xff;  
    pData++[0] = 0x80;  /* transactionId */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0x33; 
    pData++[0] = 0xFF;  /* reserved */
    pData++[0] = 0;     /* adaptationLength */
    pData++[0] = 0;     /* messageLength */
    pData++[0] = (20 + 2) + 2 + 2 + (4 + 4 + 2 + 13) + 2 + (2 + GROUPS_INFO_PRIVATE_DATA_LEN);
    pData += 20;        /* serverId */
    pData++[0] = 0;     /* compatibilityDescriptorLength */
    pData++[0] = 0;
    pData++[0] = 0;     /* privateDataLength */
    pData++[0] = 2 + (4 + 4 + 2 + 15) + (2 + GROUPS_INFO_PRIVATE_DATA_LEN); 
    pData++[0] = 0;     /* numberOfGroups */
    pData++[0] = 1;
    pData++[0] = (UTV_FAKE_TRANSACTION_ID >> 24) & 0xFF;    /* groupId will be transaction id of DII  */
    pData++[0] = (UTV_FAKE_TRANSACTION_ID >> 16) & 0xFF;
    pData++[0] = (UTV_FAKE_TRANSACTION_ID >>  8) & 0xFF;
    pData++[0] = (UTV_FAKE_TRANSACTION_ID >>  0) & 0xFF;
    pData++[0] = 0;     /* groupSize */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;     /* groupCompatibilityDescriptorLength */
    pData++[0] = 13;
                        /* Start groupCompatibilityDescriptor */
    pData++[0] = 0;     /* descriptorCount */
    pData++[0] = 1;
    pData++[0] = 0x01;                          /* descriptorType */
    pData++[0] = 0x08;                          /* descriptorLength */
    pData++[0] = 0x01;                          /* specifierType (oui) */
    pData++[0] = (UTV_FAKE_OUI >> 16) & 0xFF;   /* specifierData (oui) */
    pData++[0] = (UTV_FAKE_OUI >>  8) & 0xFF;
    pData++[0] = (UTV_FAKE_OUI >>  0) & 0xFF;
    pData++[0] = (UTV_FAKE_MODEL_GROUP >> 8) & 0xFF;    /* model (model group) */
    pData++[0] = (UTV_FAKE_MODEL_GROUP >> 0) & 0xFF;
    pData++[0] = 0;     /* version */
    pData++[0] = 0;
    pData++[0] = 0;     /* subDescriptorCount */
                        /* End groupCompatibilityDescriptor */
    pData++[0] = 0;     /* groupInfoLength  (not used) */
    pData++[0] = 0;

    pData++[0] = 0;     /* groupsInfoPrivateDataLength  */
    pData++[0] = GROUPS_INFO_PRIVATE_DATA_LEN;

    pData++[0] = UTV_STREAM_PMT_REGISTATION_DESCRIPTOR_TAG;     /* reg desc. tag */
    pData++[0] = GROUPS_INFO_PRIVATE_DATA_LEN - 2;    /* length including embedded descriptors */
    pData++[0] = 'B';   /* ULI MRD... */
    pData++[0] = 'D';
    pData++[0] = 'C';
    pData++[0] = '0';

    pData++[0] = 3;     /* tag 3 - time tag */
    pData++[0] = 8;     /* length  */
    pData++[0] = 0;     /* STT protocol version */
    pData++[0] = (UTV_BYTE)((tTime >> 24) & 0xff);  /* STT GPS seconds */
    pData++[0] = (UTV_BYTE)((tTime >> 16) & 0xff);
    pData++[0] = (UTV_BYTE)((tTime >>  8) & 0xff);
    pData++[0] = (UTV_BYTE)((tTime      ) & 0xff);
    pData++[0] = UTV_GPS_UTC_OFFSET; /* STT UTC offset */
    pData++[0] = 0;     /* STT DS status */
    pData++[0] = 0;     /* STT DS hour */    


    pData++[0] = 0;     /* tag 0 - network tag */
    pData++[0] = 4;     /* length 4 */
    pData++[0] = 0xb;   /* networkId */
    pData++[0] = 0xdc;
    pData++[0] = 0;     /* networkVersion */
    pData++[0] = UTV_NETWORK_VERSION;

    pData++[0] = 0;     /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}

/* UtvFakeDii
    Generates a canned DII section based on the UTV_FAKE... parameters
*/
UTV_BYTE * UtvFakeDii( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    time_t tTime = (time_t)( (UTV_UINT32)UtvTime( NULL ) - ( UTV_GPS_CONVERSION + UTV_GPS_UTC_OFFSET ) + ( UTV_WAIT_UP_EARLY / 1000 ) + 5 );
    struct tm * ptmGmt = UtvGmtime( &tTime );    
    int     i, n;

    tTime = UtvMktime( ptmGmt );    

    n = (int) strlen(UTV_FAKE_MOD_NAME);         /* module name length */

    if ( ( pFake = UtvFakeNewPacket() ) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] = 0;
    pData++[0] = (2 + 1 + 1 + 1) + (2 + 2 + 4 + 1 + 1) + 2 + (4 + 2 + (1 + 1 + 4 + 4) + 2 + 2 + 2 + 4 + 1 + 1 + (2 + 8) + (2 + n + 8 + 1 + 7)) + 2 + 4;  /* section_length */
    pData += 2;                 /* table id ext */
    pData++[0] = 0;             /* version  */
    pData++[0] = 0;             /* section number */
    pData++[0] = 0;             /* last section number */
    pData++[0] = UTV_DSMCC_PROTOCOL_DISCRIMINATOR; 
    pData++[0] = UTV_DSMCC_UNM;  
    pData++[0] = ( UTV_DSMCC_DII >> 8 ) & 0xFF;  
    pData++[0] = ( UTV_DSMCC_DII      ) & 0xFF;  
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 24 ) & 0xFF;  /* transactionId */
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 16 ) & 0xFF;
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >>  8 ) & 0xFF;
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID       ) & 0xFF;
    pData++[0] = 0xFF;  /* reserved */
    pData++[0] = 0;     /* adaptation length */
    pData++[0] = 0;     /* message length */
    pData++[0] = 4 + 2 + (1 + 1 + 4 + 4) + 2 + 2 + 2 + 4 + 1 + 1 + (2 + 8) + (2 + n + 8 + 1 + 7);
    pData++[0] = 0;     /* download id */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = ( UTV_FAKE_DDB_BLOCKSIZE >> 8 ) & 0xFF;        /* block size */
    pData++[0] = ( UTV_FAKE_DDB_BLOCKSIZE >> 0 ) & 0xFF; 
    pData += (1 + 1 + 4 + 4); /* window size, ack period, download window, download scenario */
    pData++[0] = 0;     /* compat descr size must be zero */
    pData++[0] = 0;
    pData++[0] = 0;     /* number of modules */
    pData++[0] = 1;
    pData++[0] = ( UTV_FAKE_MODULE_ID >> 8 ) & 0xff;        /* module id */
    pData++[0] = ( UTV_FAKE_MODULE_ID      ) & 0xff;
    pData++[0] = 0;     /* module size in bytes */
    pData++[0] = 0;
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKCOUNT * UTV_FAKE_DDB_BLOCKSIZE ) >> 8 ) & 0xFF;
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKCOUNT * UTV_FAKE_DDB_BLOCKSIZE ) >> 0 ) & 0xFF;
    pData++[0] = 10;    /* module version */
    pData++[0] = (2 + 8) + (2 + n + 8 + 1 + 7);    /* module info length */
    pData++[0] = 0xBA;      /* descriptor tag */
    pData++[0] = 8;         /* descriptor length */
    pData++[0] = (UTV_BYTE)(( tTime >> 24 ) & 0xff);  /* start time */
    pData++[0] = (UTV_BYTE)(( tTime >> 16 ) & 0xff);
    pData++[0] = (UTV_BYTE)(( tTime >>  8 ) & 0xff);
    pData++[0] = (UTV_BYTE)(( tTime       ) & 0xff);
    pData++[0] = ( UTV_FAKE_BROADCAST_SECONDS >> 24 ) & 0xff;   /* broadcast seconds */
    pData++[0] = ( UTV_FAKE_BROADCAST_SECONDS >> 16 ) & 0xff;
    pData++[0] = ( UTV_FAKE_BROADCAST_SECONDS >>  8 ) & 0xff;
    pData++[0] = ( UTV_FAKE_BROADCAST_SECONDS       ) & 0xff;
    pData++[0] = 0xB7;      /* moduleInfoDescriptor */
    pData++[0] = n + 8 + 1 + 7; /* size */
    pData++[0] = 0;         /* encoding */
    pData++[0] = n;         /* module name length */
    for ( i = 0; i < n; i++ )                            /* module name */
        pData++[0] = UTV_FAKE_MOD_NAME[ i ];
    pData++[0] = 0;         /* signature type */
    pData++[0] = 4;         /* signature length */
    pData++[0] = 'g';
    pData++[0] = 's';
    pData++[0] = 'i';
    pData++[0] = 'g';
    pData++[0] = 7;         /* privateModuleLength */
                            /* UpdateTV */
    pData++[0] = 0x80;      /* modulePriority */
    pData++[0] = UTV_DII_COMPATABILITY_DESCRIPTOR_TAG; 
    pData++[0] = 4;         /* descriptor length */
    pData++[0] = 0x0;       /* hardware model begin */
    pData++[0] = 0xff;      /* hardware model end */
    pData++[0] = 0x0;       /* software version begin */
    pData++[0] = 0xff;      /* software version end */
                            /* End UpdateTV */

    pData++[0] = 0;         /* privateDataLength */
    pData++[0] = 0;
    pData++[0] = 0;         /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}

/* UtvFakeDdb
    Generates a canned DDB section based on the UTV_FAKE... parameters
*/
UTV_BYTE * UtvFakeDdb( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    UTV_UINT32 iFill;
    
    if ( ( pFake = UtvFakeNewPacket() ) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKSIZE + (2 + 1 + 1 + 1) + ( 2 + 2 + 4 + 1 + 1 + 2 ) + ( 2 + 1 + 1 + 2) + 4 ) >> 8 ) & 0xff;  
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKSIZE + (2 + 1 + 1 + 1) + ( 2 + 2 + 4 + 1 + 1 + 2 ) + ( 2 + 1 + 1 + 2) + 4 ) >> 0 ) & 0xff;  
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 8 ) & 0xFF;   /* Low order 16 bits transaction id */
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 0 ) & 0xFF;
    pData++[0] = 0;                                     /* version */
    pData++[0] = (UTV_BYTE)( iUtvFakeBlockNumber & 0x7 );   /* section number */
    pData++[0] = 0xf;                                   /* last section number */
    pData++[0] = UTV_DSMCC_PROTOCOL_DISCRIMINATOR; 
    pData++[0] = UTV_DSMCC_UNM;  
    pData++[0] = UTV_DSMCC_DDB >> 8;  
    pData++[0] = UTV_DSMCC_DDB & 0xff;  
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 24 ) & 0xFF;  /* transactionId */
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >> 16 ) & 0xFF;
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >>  8 ) & 0xFF;
    pData++[0] = ( UTV_FAKE_TRANSACTION_ID >>  0 ) & 0xFF;
    pData++[0] = 0xFF;                                  /* reserved */
    pData++[0] = 0;                                     /* adaptation length */
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKSIZE + 6 ) >> 8 ) & 0xFF; /* message length (16 bits) */
    pData++[0] = ( ( UTV_FAKE_DDB_BLOCKSIZE + 6 ) >> 0 ) & 0xFF;
    pData++[0] = ( UTV_FAKE_MODULE_ID >> 8 ) & 0xff;    /* module id */
    pData++[0] = ( UTV_FAKE_MODULE_ID      ) & 0xff;
    pData++[0] = 10;        /* module version */
    pData++[0] = 0xFF;      /* reserved */
    pData++[0] = 0;         /* block number */
    pData++[0] = (UTV_BYTE)( iUtvFakeBlockNumber & 0x7 );
    for ( iFill = 0; iFill < UTV_FAKE_DDB_BLOCKSIZE; iFill++ )
        pData++[0] = (UTV_BYTE)(iFill & 0xFF);
    iUtvFakeBlockNumber += 3;
    pData++[0] = 0;     /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}

/* UtvFakePat
    Generates a canned PAT section
*/
UTV_BYTE * UtvFakePat( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    
    if ( ( pFake = UtvFakeNewPacket() ) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] =  0xf0;     /* section_syntax_indicator private_indicator reserved */
    pData++[0] = (2 + 1 + 1 + 1) + 4 + 4;   /* section_length */
    pData += 2;                 /* tsid  */
    pData++[0] = 0;             /* version  */
    pData++[0] = 0;             /* section number */
    pData++[0] = 0;             /* last section number */
    pData++[0] = 0;             /* program_number */
    pData++[0] = UTV_FAKE_PROGRAM_NUMBER;  
    pData++[0] = ( UTV_FAKE_PMT_PID >> 8 ) & 0xFF;  /* PMT PID */
    pData++[0] = ( UTV_FAKE_PMT_PID >> 0 ) & 0xFF;
    pData++[0] = 0;             /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}

/* UtvFakeVct
    Generates a canned VCT section
*/
UTV_BYTE * UtvFakeVct( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    
    if ( ( pFake = UtvFakeNewPacket() ) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] = 0;
    pData++[0] = (2 + 1 + 1 + 1) + 2 + (14 + 3 + 1 + 4 + 2) + 2 + 2 + (2 + 2) + 4;  /* section_length */
    pData += 2;                 /* tsid  */
    pData++[0] = 0;             /* version  */
    pData++[0] = 0;             /* section number */
    pData++[0] = 0;             /* last section number */
    pData++[0] = 0;         /* protocol version */
    pData++[0] = 1;         /* num channels in section */
    pData += (14 + 3 + 1 + 4 + 2); /* channel name, channel number, modulation mode, carrier frequency, TSID */
    pData++[0] = 0;
    pData++[0] = UTV_FAKE_PROGRAM_NUMBER;       /* program_number */
    pData++[0] = 0;
    pData++[0] = UTV_SERVICE_TYPE_DOWNLOAD;  /* service_type */
    pData += (2 + 2);   /* descriptors length, additional descr len */
    pData++[0] = 0;     /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}

/* UtvFakePmt
    Generates a canned PMT section
*/
UTV_BYTE * UtvFakePmt( UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake, * pData;
    
    if ( ( pFake = UtvFakeNewPacket() ) == NULL )
        return NULL;

    pData = pFake;
    pData++[0] = (UTV_BYTE)iTableId;
    pData++[0] = 0;
    pData++[0] = 2 + (1 + 1 + 1 + 2 + 2) + 1 + 2 + 4;   /* section_length */
    pData++[0] = 0;         
    pData++[0] = UTV_FAKE_PROGRAM_NUMBER;   /* program_number */
    pData++[0] = 0;             /* version  */
    pData++[0] = 0;             /* section number */
    pData++[0] = 0;             /* last section number */
    pData++[0] = 0;             /* PCR PID */
    pData++[0] = 0; 
    pData++[0] = 0;             /* program info length */
    pData++[0] = 0; 
    pData++[0] = UTV_STREAM_TYPE_DOWNLOAD;
    pData++[0] = ( UTV_FAKE_CAROUSEL_PID >> 8 ) & 0xFF;  /* Elementary/Carousel PID */
    pData++[0] = ( UTV_FAKE_CAROUSEL_PID >> 0 ) & 0xFF;
    pData++[0] = 0;     /* CRC32  */
    pData++[0] = 0;
    pData++[0] = 0;
    pData++[0] = 0;
    return pFake;
}
static UTV_UINT32 iUtvFakeCount = 0;

/* UtvGetFakeSectionData
    Returns fake section data given the pid and tid
*/
UTV_BYTE * UtvGetFakeSectionData( UTV_UINT32 iPid, UTV_UINT32 iTableId )
{
    UTV_BYTE * pFake = NULL;
    UTV_UINT32 iCrcBytes;
    UTV_UINT32 uiCrc32;
    UTV_BYTE * pCrc32;

    /* Look at the settings and make up some appropriate data */
    switch ( iPid )
    {
    case UTV_PID_PAT: /* PID 0 */
        pFake = UtvFakePat( iTableId );
        break;

    case UTV_PID_PSIP:  /* PID 8187 = 0x1ffb */
        pFake = UtvFakeVct( iTableId );
        break;

    case UTV_FAKE_PMT_PID: /* PID 0x77 Fake PMT */
        pFake = UtvFakePmt( iTableId );
        break;

    case UTV_FAKE_CAROUSEL_PID: /* Default Carousel Primary PID  */
        switch ( iTableId )
        {
            case UTV_TID_UNM:
                /* Could be DII or DSI */
                if ( iUtvFakeCount++ == 0 )
                {
                    /* Send the DSI */
                    pFake = UtvFakeDsi( iTableId );
                } 
                else 
                {
                    /* Send the one DII we fake */
                    iUtvFakeCount = 0;
                    pFake = UtvFakeDii( iTableId );
                }
                break;
            case UTV_TID_DDM:
                /* Send a DDB  */
                pFake = UtvFakeDdb( iTableId );
                break;
        }
        break;
    }

    if ( pFake != NULL )
    {
        /* Calculate CRC32 for packet */
        iCrcBytes = pFake[ 2 ] + 3 - 4;
        uiCrc32 = UtvCrc32( pFake, iCrcBytes );
        pCrc32 = pFake + iCrcBytes;
        *pCrc32++ = (UTV_BYTE)( uiCrc32 >> 24 ) & 0xFF;
        *pCrc32++ = (UTV_BYTE)( uiCrc32 >> 16 ) & 0xFF;
        *pCrc32++ = (UTV_BYTE)( uiCrc32 >>  8 ) & 0xFF;
        *pCrc32++ = (UTV_BYTE)( uiCrc32       ) & 0xFF;
    }

    /* return pointer to fake data if available */
    return pFake;
}
