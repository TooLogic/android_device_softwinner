/* platform-os-linux-file.c: UpdateTV Platform Adaptation Layer
                             Linux File Interfaces

   Copyright (c) 2007 - 2009 UpdateLogic Incorporated. All rights reserved.

   Contains the following UpdateTV Agent Platform Adapatation Entry Points
   -----------------------------------------------------------------------
   -----------------------------------------------------------------------

   Revision History (newest edits added to the top)

   Who             Date        Edit
   Jim Muchow      08/20/2010  Added file handle accounting infrastructure and action to prevent loss when agent aborts.
   Bob Taylor      11/12/2009  Added missing file closes to UtvPlatformFileGetSize()
   Bob Taylor      07/07/2009  Created
*/

#include "utv.h"
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/* File handles accounting: maintain a list of file file handles created and 
   removed them when they are closed. The purpose is to ensure that when a
   UtvProjectOnShutdown( ) occurs, there are no unclosed file handles "waiting
   around" that basically become lost.

   Under normal circumstances there is seldom more than a single open file in 
   operation; there can, however, be two. Define for three.
*/

#define HANDLE_COUNT    3
static UTV_UINT32 handleExists[HANDLE_COUNT] = { 0 };

UTV_BOOL UtvFilesOpen( void )
{
    int index;

    for ( index = 0; index < HANDLE_COUNT; index++ )
    {
        if ( handleExists[index] )
        {
            return UTV_TRUE;
        }
    }

    return UTV_FALSE;
}

void UtvCloseAllFiles( void )
{
    int index;

    for ( index = 0; index < HANDLE_COUNT; index++ )
    {
        if ( handleExists[index] == 0 )
        {
            continue;
        }

        (void)fclose( (FILE *)(handleExists[index]) );

        handleExists[index] = 0;
    }
}

/* UtvPlatformFileOpen
   Attempt to open the named file in the given location.
   Return a file handle to be used in the UtvPlatformFileRead() and Utv call below.
*/
UTV_RESULT UtvPlatformFileOpen( char *pszFileName, char *pszAttributes, UTV_UINT32 *phFile )
{
    int index;

    /* find the first open spot */
    for (index = 0; index < HANDLE_COUNT; index++)
    {
        if (handleExists[index] == 0)
            break;
    }

    if (index == HANDLE_COUNT)
    {
        return UTV_FILE_OPEN_FAILS;
    }

    /* open it for read binary */
    if ( 0 == ( *phFile  = (UTV_UINT32) (void *) fopen( pszFileName, pszAttributes )))
    {
        return UTV_FILE_OPEN_FAILS;
    }

    handleExists[index] = *phFile;

    return UTV_OK;
}

/* UtvPlatformFileReadBinary
    Read X number of bytes into a buffer from the specified file previously opened
    with UtvPlatformFileOpen().
   Returns bytes actually read.  If they differ from the amount requested UTV_FILE_EOF is returned
   otherwise UTV_OK.
*/
UTV_RESULT UtvPlatformFileReadBinary( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiReadRequestLen, UTV_UINT32 *puiBytesRead )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

    /* attemp the read request */
    *puiBytesRead = fread( pBuffer, 1, uiReadRequestLen, (FILE *) hFile );

    if ( *puiBytesRead != uiReadRequestLen )
        return UTV_FILE_EOF;

    return UTV_OK;
}

/* UtvPlatformFileWriteBinary
    Attempt to write X number of bytes from the buffer into the specified file previously opened
    with UtvPlatformFileOpen().
   Returns bytes actually written.  If they differ from the amount requested UTV_WRITE_FAILS is returned
   otherwise UTV_OK.
*/
UTV_RESULT UtvPlatformFileWriteBinary( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiWriteRequestLen, UTV_UINT32 *puiBytesWritten )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

    /* attemp the read request */
    *puiBytesWritten = fwrite( pBuffer, 1, uiWriteRequestLen, (FILE *) hFile );

    if ( *puiBytesWritten != uiWriteRequestLen )
        return UTV_WRITE_FAILS;

    return UTV_OK;
}

/* UtvPlatformFileReadText
    Read X number of bytes into a buffer from the specified file previously opened
    with UtvPlatformFileOpen().
   Returns bytes actually read.  If they differ from the amount requested UTV_FILE_EOF is returned
   otherwise UTV_OK.
*/
UTV_RESULT UtvPlatformFileReadText( UTV_UINT32 hFile, UTV_BYTE *pBuffer, UTV_UINT32 uiReadMax )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

    /* attemp the read request */
    if ( NULL == fgets( (UTV_INT8 *) pBuffer, uiReadMax, (FILE *) hFile ))
        return UTV_FILE_EOF;

    return UTV_OK;
}

/* UtvPlatformFileWriteText
    Write X number of bytes into a buffer from the specified file previously opened
    with UtvPlatformFileOpen().
   Returns bytes actually written.  If they differ from the amount requested UTV_FILE_EOF is returned
   otherwise UTV_OK.
*/
UTV_RESULT UtvPlatformFileWriteText( UTV_UINT32 hFile, UTV_BYTE *pBuffer )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

    /* attemp the write request */
    if ( EOF == fputs( (UTV_INT8 *) pBuffer, (FILE *) hFile ))
        return UTV_FILE_EOF;

    return UTV_OK;
}

/* UtvPlatformFileSeek
    Seek to the specified offset into the currently open file.
   Returns OK or UTV_FILE_EOF if error.
*/
UTV_RESULT UtvPlatformFileSeek( UTV_UINT32 hFile, UTV_UINT32 uiSeekOffset, UTV_UINT32 uiSeekType )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

	switch ( uiSeekType )
	{
	case UTV_SEEK_SET: uiSeekType = SEEK_SET; break;
	case UTV_SEEK_CUR: uiSeekType = SEEK_CUR; break;
	case UTV_SEEK_END: uiSeekType = SEEK_END; break;
	default: return UTV_UNKNOWN;
	}

    if ( 0 != fseek( (FILE *) hFile, uiSeekOffset, uiSeekType ))
        return UTV_FILE_EOF;

    return UTV_OK;
}

/* UtvPlatformFileTell
    Tell the offset into the currently open file.
   Returns OK or UTV_FILE_EOF if error.
*/
UTV_RESULT UtvPlatformFileTell( UTV_UINT32 hFile, UTV_UINT32 *puiTellOffset )
{
    if ( 0 == hFile )
        return UTV_FILE_BAD_HANDLE;

    *puiTellOffset = ftell( (FILE *) hFile );
    return UTV_OK;
}

/* UtvPlatformFileDelete
    Attempt to delete the named file in the given location.
   Returns OK or error status.
*/
UTV_RESULT UtvPlatformFileDelete( char *pszFileName )
{
    UTV_INT32   res;

    /* open it for read binary */
    if ( 0 != ( res = unlink( pszFileName )))
        return UTV_FILE_DELETE_FAILS;

    return UTV_OK;
}

/* UtvPlatformFileClose
    Closes the file specified.
*/
UTV_RESULT UtvPlatformFileClose( UTV_UINT32 hFile )
{
    int index;

    /* find the first open spot */
    for (index = 0; index < HANDLE_COUNT; index++)
    {
        if (handleExists[index] == hFile)
            break;
    }

    if (index == HANDLE_COUNT)
    {
        UtvMessage( UTV_LEVEL_WARN, "UtvPlatformFileClose: hFile %d not found", hFile );
        return UTV_FILE_CLOSE_FAILS;
    }

    if ( 0 != fclose( (FILE *) hFile ) )
        return UTV_FILE_CLOSE_FAILS;

    handleExists[index] = 0;

    return UTV_OK;
}

UTV_RESULT UtvPlatformFileGetSize( UTV_INT8 *pszFName, UTV_UINT32 *uiDataLen )
{
    UTV_RESULT        rStatus;
    UTV_UINT32        uiFileHandle;

    /* open the file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszFName, "rb", &uiFileHandle )))
    {
        UtvMessage( UTV_LEVEL_WARN, "UtvPlatformFileOpen(%s) fails: \"%s\"", pszFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* seek to the end */
    if ( UTV_OK != ( rStatus = UtvPlatformFileSeek( uiFileHandle, 0, UTV_SEEK_END )))
    {
        UtvPlatformFileClose( uiFileHandle );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileSeek() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* get the size */
    if ( UTV_OK != ( rStatus = UtvPlatformFileTell( uiFileHandle, uiDataLen )))
    {
        UtvPlatformFileClose( uiFileHandle );
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileTell() fails: \"%s\"", UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    UtvPlatformFileClose( uiFileHandle );

    return rStatus;
}

UTV_BOOL UtvPlatformFilePathExists( UTV_INT8 *pszPFName )
{
	struct stat     sDirInfo;
	return ( 0 == stat( pszPFName, &sDirInfo ));
}

UTV_RESULT UtvPlatformMakeDirectory( UTV_INT8 *pszPath )
{
    if ( UTV_FALSE == UtvPlatformFilePathExists( pszPath ))
    {
        if ( -1 == mkdir( pszPath, 0700 ))
            return UTV_CREATE_DIR_FAILS;
    }
    return UTV_OK;
}

UTV_RESULT UtvPlatformFileCopy( UTV_INT8 *pszFromFName, UTV_INT8 *pszToFName )
{
    UTV_RESULT        	rStatus;
    UTV_UINT32        	uiFromFileHandle, uiToFileHandle, uiBytesRead, uiBytesWritten;
	UTV_BYTE			b;

    /* open the from file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszFromFName, "rb", &uiFromFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen(%s) fails: \"%s\"", pszFromFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

    /* open the to file */
    if ( UTV_OK != ( rStatus = UtvPlatformFileOpen( pszToFName, "wb", &uiToFileHandle )))
    {
        UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileOpen(%s) fails: \"%s\"", pszToFName, UtvGetMessageString( rStatus ) );
        return rStatus;
    }

	while ( UTV_TRUE )
	{
		if ( UTV_OK != ( rStatus = UtvPlatformFileReadBinary( uiFromFileHandle, &b, 1, &uiBytesRead )))
		{
			if ( UTV_FILE_EOF != rStatus )
			{
				UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileReadBinary(): \"%s\"", UtvGetMessageString( rStatus ) );
			} else
				rStatus = UTV_OK;

			UtvPlatformFileClose( uiFromFileHandle );
			UtvPlatformFileClose( uiToFileHandle );
			return rStatus;
		}

		if ( UTV_OK != ( rStatus = UtvPlatformFileWriteBinary( uiToFileHandle, &b, 1, &uiBytesWritten )))
		{
			UtvMessage( UTV_LEVEL_ERROR, "UtvPlatformFileWriteBinary(): \"%s\"", UtvGetMessageString( rStatus ) );
			UtvPlatformFileClose( uiFromFileHandle );
			UtvPlatformFileClose( uiToFileHandle );
			return rStatus;
		}
	}

}
