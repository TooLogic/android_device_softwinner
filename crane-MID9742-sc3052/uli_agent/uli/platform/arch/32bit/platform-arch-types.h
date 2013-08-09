/* arch-types.h: UpdateTV Personality Map - 32-bit type definitions
 ***************************************************************************


 Copyright (c) 2004 - 2008 UpdateLogic Incorporated. All rights reserved.

 Revision History (newest edits added to the top)

  Who             Date        Edit
  Jim Muchow      09/21/2010  BE support.
  Bob Taylor      08/05/2008  Converted from C++ to C.
  Bob Taylor      01/28/2008  Added UTV_TRUE and UTV_FALSE to prevent win32 warnings about new int bool compares.
  Bob Taylor      10/16/2007  UTV_BOOL changed from bool to int.
  Bob Taylor      04/30/2007  Original.
*/

/* Primitive Data Type Definitions for 32-bit architectures
   NOTE: long long and unsigned long long are 64 bit integer types specified in ANSI C
   Microsoft compilers call this type __int64 
*/
#ifndef WIN32
#define __int64 long long
#endif

#ifndef UTV_INT64
#define UTV_INT64 __int64
#endif

#ifndef UTV_UINT64
#define UTV_UINT64 unsigned __int64
#endif

#ifndef UTV_INT32
#define UTV_INT32 long
#endif

#ifndef UTV_UINT16
#define UTV_UINT16 unsigned short
#endif 

#ifndef UTV_UINT8
#define UTV_UINT8 unsigned char
#endif 

#ifndef UTV_INT8
#define UTV_INT8 char
#endif 

#ifndef UTV_UINT32
#define UTV_UINT32 unsigned long 
#endif 

#ifndef UTV_BYTE
#define UTV_BYTE unsigned char 
#endif 

#ifndef UTV_BOOL
#define UTV_BOOL int
#endif 

#ifndef IN
#define IN
#endif 

#ifndef OUT
#define OUT
#endif 

#ifndef UTV_TRUE
#define UTV_TRUE (int) 1
#endif

#ifndef UTV_FALSE
#define UTV_FALSE (int) 0
#endif

#ifndef UTV_RESULT
#define UTV_RESULT UTV_UINT32
#endif

#ifndef UTV_ENDIANNESS
#define UTV_ENDIANNESS

/* Handling endianness in the agent covers two areas. Networking and "network order" is
   handled transparently and correctly and is not a concern here. Reading and writing files
   is another matter. Any file info that is not a byte stream is defined to exist in little
   endian format.

   It is thus necessary to handle endianness transparently such that neither little- nor
   big-endian machines can detect the different (like the networking mentioned above). To
   do so a set of conversion functions is defined:

   UTV_INT32 Utv_32letoh(x) - convert a 32-bit Little Endian value to host order
   UTV_INT16 Utv_16letoh(x) - convert a 16-bit Little Endian value to host order

   These are the only two needed at present. More can defined as needed.

   The ultimate fallback is to simple define the swapping macros by hand, but it is
   possible with GCC to use a complete library of byte-swapping functions/macros.
*/

#if defined(__APPLE__)

#warning "Apple compile"
/* The following include file will handle both PPC as well as "Intel" machines by
   including the appropriate byte-order specific OSByteOrder file. 
*/
#include <libkern/OSByteOrder.h>

/* Unfortunately, Apple also has it's own names for the enddiannes conversion routines.
   Make them avialable for use here. */
#define Utv_32letoh(x)      OSSwapLittleToHostConstInt32(x)
#define Utv_16letoh(x)      OSSwapLittleToHostConstInt16(x)

#elif defined __GNUC__ /* a good fallback if using GCC */

#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define Utv_16letoh(x) __bswap_16 (x)
#define Utv_32letoh(x) __bswap_32 (x)

#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define Utv_16letoh(x) (x)
#define Utv_32letoh(x) (x)
#else
#error "BYTE_ORDER not defined"
#endif

#else /* the ulitmate fallback, just define the conversions here */

#warning "fallback compile"
#include <endian.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define Utv_16letoh(x) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#define Utv_32letoh(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
                    (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define Utv_16letoh(x) (x)
#define Utv_32letoh(x) (x)
#else
#error "BYTE_ORDER not defined"
#endif

#endif

#endif /* UTV_ENDIANNESS */
