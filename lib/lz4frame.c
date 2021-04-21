/*
   LZ4 - Fast LZ compression algorithm
   Copyright (C) 2011-2016, Yann Collet.

   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
    - LZ4 homepage : http://www.lz4.org
    - LZ4 source repository : https://github.com/Cyan4973/lz4
*/

/*-************************************
*  Compiler Options
**************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  define FORCE_INLINE static __forceinline
#  include <intrin.h>
#  pragma warning(disable : 4127)        /* disable: C4127: conditional expression is constant */
#  pragma warning(disable : 4293)        /* disable: C4293: too large shift (32-bits) */
#else
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
#    if defined(__GNUC__) || defined(__clang__)
#      define FORCE_INLINE static inline __attribute__((always_inline))
#    else
#      define FORCE_INLINE static inline
#    endif
#  else
#    define FORCE_INLINE static
#  endif   /* __STDC_VERSION__ */
#endif  /* _MSC_VER */

/* LZ4_GCC_VERSION is defined into lz4.h */
#if (LZ4_GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)


/*-************************************
*  Memory routines
**************************************/
#include <stdlib.h>   /* malloc, calloc, free */
#define ALLOCATOR(n,s) calloc(n,s)
#define ALLOCATORF(s)   calloc(1,s)
#define FREEMEM        free
#include <string.h>   /* memset, memcpy, memmove */
#define MEM_INIT       memset


/*-************************************
*  Includes
**************************************/
#include "lz4frame.h"
//#include "lz4.h"
//#include "lz4hc.h"
#define XXH_STATIC_LINKING_ONLY
#include "xxhash.h"


/*-************************************
*  Basic Types
**************************************/
#if !defined (__VMS) && (defined (__cplusplus) || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
# include <stdint.h>
  typedef  uint8_t BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif


  /*-************************************
  *  Tuning parameters
  **************************************/
  /*
  * HEAPMODE :
  * Select how default compression functions will allocate memory for their hash table,
  * in memory stack (0:default, fastest), or in memory heap (1:requires malloc()).
  */
#define HEAPMODE 0
#define LZ4HC_HEAPMODE 0
  /*
  * ACCELERATION_DEFAULT :
  * Select "acceleration" for LZ4_compress_fast() when parameter value <= 0
  */
#define ACCELERATION_DEFAULT 1


  /*-************************************
  *  CPU Feature Detection
  **************************************/
  /* LZ4_FORCE_MEMORY_ACCESS
  * By default, access to unaligned memory is controlled by `memcpy()`, which is safe and portable.
  * Unfortunately, on some target/compiler combinations, the generated assembly is sub-optimal.
  * The below switch allow to select different access method for improved performance.
  * Method 0 (default) : use `memcpy()`. Safe and portable.
  * Method 1 : `__packed` statement. It depends on compiler extension (ie, not portable).
  *            This method is safe if your compiler supports it, and *generally* as fast or faster than `memcpy`.
  * Method 2 : direct access. This method is portable but violate C standard.
  *            It can generate buggy code on targets which generate assembly depending on alignment.
  *            But in some circumstances, it's the only known way to get the most performance (ie GCC + ARMv6)
  * See https://fastcompression.blogspot.fr/2015/08/accessing-unaligned-memory.html for details.
  * Prefer these methods in priority order (0 > 1 > 2)
  */
#ifndef LZ4_FORCE_MEMORY_ACCESS   /* can be defined externally, on command line for example */
#  if defined(__GNUC__) && ( defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) )
#    define LZ4_FORCE_MEMORY_ACCESS 2
#  elif defined(__INTEL_COMPILER) || \
  (defined(__GNUC__) && ( defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) ))
#    define LZ4_FORCE_MEMORY_ACCESS 1
#  endif
#endif

  /*
  * LZ4_FORCE_SW_BITCOUNT
  * Define this parameter if your target system or compiler does not support hardware bit count
  */
#if defined(_MSC_VER) && defined(_WIN32_WCE)   /* Visual Studio for Windows CE does not support Hardware bit count */
#  define LZ4_FORCE_SW_BITCOUNT
#endif


  /*-************************************
  *  Reading and writing into memory
  **************************************/
#define STEPSIZE sizeof(size_t)

  static unsigned LZ4_64bits( void ) { return sizeof( void* ) == 8; }

  static unsigned LZ4_isLittleEndian( void )
  {
	  const union { U32 i; BYTE c[4]; } one = { 1 };   /* don't use static : performance detrimental */
	  return one.c[0];
  }


  static U16 LZ4_read16( const void* memPtr )
  {
	  U16 val; memcpy( &val, memPtr, sizeof( val ) ); return val;
  }

  static U32 LZ4_read32( const void* memPtr )
  {
	  U32 val; memcpy( &val, memPtr, sizeof( val ) ); return val;
  }

  static size_t LZ4_read_ARCH( const void* memPtr )
  {
	  size_t val; memcpy( &val, memPtr, sizeof( val ) ); return val;
  }

  static void LZ4_write16( void* memPtr, U16 value )
  {
	  memcpy( memPtr, &value, sizeof( value ) );
  }

  static void LZ4_write32( void* memPtr, U32 value )
  {
	  memcpy( memPtr, &value, sizeof( value ) );
  }

  static U16 LZ4_readLE16( const void* memPtr )
  {
	  if ( LZ4_isLittleEndian() ) {
		  return LZ4_read16( memPtr );
	  } else {
		  const BYTE* p = (const BYTE*)memPtr;
		  return (U16)((U16)p[0] + (p[1] << 8));
	  }
  }

  static void LZ4_writeLE16( void* memPtr, U16 value )
  {
	  if ( LZ4_isLittleEndian() ) {
		  LZ4_write16( memPtr, value );
	  } else {
		  BYTE* p = (BYTE*)memPtr;
		  p[0] = (BYTE)value;
		  p[1] = (BYTE)(value >> 8);
	  }
  }

  static void LZ4_copy8( void* dst, const void* src )
  {
	  memcpy( dst, src, 8 );
  }

  /* customized variant of memcpy, which can overwrite up to 7 bytes beyond dstEnd */
  static void LZ4_wildCopy( void* dstPtr, const void* srcPtr, void* dstEnd )
  {
	  BYTE* d = (BYTE*)dstPtr;
	  const BYTE* s = (const BYTE*)srcPtr;
	  BYTE* const e = (BYTE*)dstEnd;

#if 0
	  const size_t l2 = 8 - (((size_t)d) & (sizeof( void* ) - 1));
	  LZ4_copy8( d, s ); if ( d>e - 9 ) return;
	  d += l2; s += l2;
#endif /* join to align */

	  do { LZ4_copy8( d, s ); d += 8; s += 8; } while ( d<e );
  }


  /* unoptimized version; solves endianess & alignment issues */
  static U32 LZ4F_readLE32( const void* src )
  {
	  const BYTE* const srcPtr = (const BYTE*)src;
	  U32 value32 = srcPtr[0];
	  value32 += (srcPtr[1] << 8);
	  value32 += (srcPtr[2] << 16);
	  value32 += ((U32)srcPtr[3]) << 24;
	  return value32;
  }

  static void LZ4F_writeLE32( BYTE* dstPtr, U32 value32 )
  {
	  dstPtr[0] = (BYTE)value32;
	  dstPtr[1] = (BYTE)(value32 >> 8);
	  dstPtr[2] = (BYTE)(value32 >> 16);
	  dstPtr[3] = (BYTE)(value32 >> 24);
  }

  static U64 LZ4F_readLE64( const BYTE* srcPtr )
  {
	  U64 value64 = srcPtr[0];
	  value64 += ((U64)srcPtr[1] << 8);
	  value64 += ((U64)srcPtr[2] << 16);
	  value64 += ((U64)srcPtr[3] << 24);
	  value64 += ((U64)srcPtr[4] << 32);
	  value64 += ((U64)srcPtr[5] << 40);
	  value64 += ((U64)srcPtr[6] << 48);
	  value64 += ((U64)srcPtr[7] << 56);
	  return value64;
  }

  static void LZ4F_writeLE64( BYTE* dstPtr, U64 value64 )
  {
	  dstPtr[0] = (BYTE)value64;
	  dstPtr[1] = (BYTE)(value64 >> 8);
	  dstPtr[2] = (BYTE)(value64 >> 16);
	  dstPtr[3] = (BYTE)(value64 >> 24);
	  dstPtr[4] = (BYTE)(value64 >> 32);
	  dstPtr[5] = (BYTE)(value64 >> 40);
	  dstPtr[6] = (BYTE)(value64 >> 48);
	  dstPtr[7] = (BYTE)(value64 >> 56);
  }



  /*-************************************
  *  Common Constants
  **************************************/
#define MINMATCH 4

#define WILDCOPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (WILDCOPYLENGTH+MINMATCH)
  static const int LZ4_minLength = (MFLIMIT + 1);

#define KB *(1 <<10)
#define MB *(1 <<20)
#define GB *(1U<<30)

#define MAXD_LOG 16
#define MAX_DISTANCE ((1 << MAXD_LOG) - 1)

#define ML_BITS  4
#define ML_MASK  ((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)


  /*-************************************
  *  Common Utils
  **************************************/
#define LZ4_STATIC_ASSERT(c)    { enum { LZ4_static_assert = 1/(int)(!!(c)) }; }   /* use only *after* variable declarations */


  /*-************************************
  *  Common functions
  **************************************/
  static unsigned LZ4_NbCommonBytes( register size_t val )
  {
	  if ( LZ4_isLittleEndian() ) {
		  if ( LZ4_64bits() ) {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  unsigned long r = 0;
			  _BitScanForward64( &r, (U64)val );
			  return (int)(r >> 3);
#       elif (defined(__clang__) || (LZ4_GCC_VERSION >= 304)) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  return (__builtin_ctzll( (U64)val ) >> 3);
#       else
			  static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
			  return DeBruijnBytePos[((U64)((val & -(long long)val) * 0x0218A392CDABBD3FULL)) >> 58];
#       endif
		  } else /* 32 bits */ {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  unsigned long r;
			  _BitScanForward( &r, (U32)val );
			  return (int)(r >> 3);
#       elif (defined(__clang__) || (LZ4_GCC_VERSION >= 304)) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  return (__builtin_ctz( (U32)val ) >> 3);
#       else
			  static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
			  return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#       endif
		  }
	  } else   /* Big Endian CPU */ {
		  if ( LZ4_64bits() ) {
#       if defined(_MSC_VER) && defined(_WIN64) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  unsigned long r = 0;
			  _BitScanReverse64( &r, val );
			  return (unsigned)(r >> 3);
#       elif (defined(__clang__) || (LZ4_GCC_VERSION >= 304)) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  return (__builtin_clzll( (U64)val ) >> 3);
#       else
			  unsigned r;
			  if ( !(val >> 32) ) { r = 4; } else { r = 0; val >>= 32; }
			  if ( !(val >> 16) ) { r += 2; val >>= 8; } else { val >>= 24; }
			  r += (!val);
			  return r;
#       endif
		  } else /* 32 bits */ {
#       if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  unsigned long r = 0;
			  _BitScanReverse( &r, (unsigned long)val );
			  return (unsigned)(r >> 3);
#       elif (defined(__clang__) || (LZ4_GCC_VERSION >= 304)) && !defined(LZ4_FORCE_SW_BITCOUNT)
			  return (__builtin_clz( (U32)val ) >> 3);
#       else
			  unsigned r;
			  if ( !(val >> 16) ) { r = 2; val >>= 8; } else { r = 0; val >>= 24; }
			  r += (!val);
			  return r;
#       endif
		  }
	  }
  }

  static unsigned LZ4_count( const BYTE* pIn, const BYTE* pMatch, const BYTE* pInLimit )
  {
	  const BYTE* const pStart = pIn;

	  while ( likely( pIn<pInLimit - (STEPSIZE - 1) ) ) {
		  size_t diff = LZ4_read_ARCH( pMatch ) ^ LZ4_read_ARCH( pIn );
		  if ( !diff ) { pIn += STEPSIZE; pMatch += STEPSIZE; continue; }
		  pIn += LZ4_NbCommonBytes( diff );
		  return (unsigned)(pIn - pStart);
	  }

	  if ( LZ4_64bits() ) if ( (pIn<(pInLimit - 3)) && (LZ4_read32( pMatch ) == LZ4_read32( pIn )) ) { pIn += 4; pMatch += 4; }
	  if ( (pIn<(pInLimit - 1)) && (LZ4_read16( pMatch ) == LZ4_read16( pIn )) ) { pIn += 2; pMatch += 2; }
	  if ( (pIn<pInLimit) && (*pMatch == *pIn) ) pIn++;
	  return (unsigned)(pIn - pStart);
  }


  /*-************************************
  *  Local Constants
  **************************************/
#define LZ4_HASHLOG   (LZ4_MEMORY_USAGE-2)
#define HASHTABLESIZE (1 << LZ4_MEMORY_USAGE)
#define HASH_SIZE_U32 (1 << LZ4_HASHLOG)       /* required as macro for static allocation */

  static const int LZ4_64Klimit = ((64 KB) + (MFLIMIT - 1));
  static const U32 LZ4_skipTrigger = 6;  /* Increase this value ==> compression run slower on incompressible data */


										 /*-************************************
										 *  Local Structures and types
										 **************************************/
  typedef struct
  {
	  U32 hashTable[HASH_SIZE_U32];
	  U32 currentOffset;
	  U32 initCheck;
	  const BYTE* dictionary;
	  BYTE* bufferStart;   /* obsolete, used for slideInputBuffer */
	  U32 dictSize;
  } LZ4_stream_t_internal;

  typedef enum { notLimited = 0, limitedOutput = 1 } limitedOutput_directive;
  typedef enum { byPtr, byU32, byU16 } tableType_t;

  typedef enum { noDict = 0, withPrefix64k, usingExtDict } dict_directive;
  typedef enum { noDictIssue = 0, dictSmall } dictIssue_directive;

  typedef enum { endOnOutputSize = 0, endOnInputSize = 1 } endCondition_directive;
  typedef enum { full = 0, partial = 1 } earlyEnd_directive;


  /*-************************************
  *  Local Utils
  **************************************/
  int LZ4_versionNumber( void ) { return LZ4_VERSION_NUMBER; }
  int LZ4_compressBound( int isize ) { return LZ4_COMPRESSBOUND( isize ); }
  int LZ4_sizeofState() { return LZ4_STREAMSIZE; }


  /*-******************************
  *  Compression functions
  ********************************/
  static U32 LZ4_hashSequence( U32 sequence, tableType_t const tableType )
  {
	  if ( tableType == byU16 )
		  return (((sequence) * 2654435761U) >> ((MINMATCH * 8) - (LZ4_HASHLOG + 1)));
	  else
		  return (((sequence) * 2654435761U) >> ((MINMATCH * 8) - LZ4_HASHLOG));
  }

  static const U64 prime5bytes = 889523592379ULL;
  static U32 LZ4_hashSequence64( size_t sequence, tableType_t const tableType )
  {
	  const U32 hashLog = (tableType == byU16) ? LZ4_HASHLOG + 1 : LZ4_HASHLOG;
	  const U32 hashMask = (1 << hashLog) - 1;
	  return ((sequence * prime5bytes) >> (40 - hashLog)) & hashMask;
  }

  static U32 LZ4_hashSequenceT( size_t sequence, tableType_t const tableType )
  {
	  if ( LZ4_64bits() )
		  return LZ4_hashSequence64( sequence, tableType );
	  return LZ4_hashSequence( (U32)sequence, tableType );
  }

  static U32 LZ4_hashPosition( const void* p, tableType_t tableType ) { return LZ4_hashSequenceT( LZ4_read_ARCH( p ), tableType ); }

  static void LZ4_putPositionOnHash( const BYTE* p, U32 h, void* tableBase, tableType_t const tableType, const BYTE* srcBase )
  {
	  switch ( tableType ) {
	  case byPtr: { const BYTE** hashTable = (const BYTE**)tableBase; hashTable[h] = p; return; }
	  case byU32: { U32* hashTable = (U32*)tableBase; hashTable[h] = (U32)(p - srcBase); return; }
	  case byU16: { U16* hashTable = (U16*)tableBase; hashTable[h] = (U16)(p - srcBase); return; }
	  }
  }

  static void LZ4_putPosition( const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase )
  {
	  U32 const h = LZ4_hashPosition( p, tableType );
	  LZ4_putPositionOnHash( p, h, tableBase, tableType, srcBase );
  }

  static const BYTE* LZ4_getPositionOnHash( U32 h, void* tableBase, tableType_t tableType, const BYTE* srcBase )
  {
	  if ( tableType == byPtr ) { const BYTE** hashTable = (const BYTE**)tableBase; return hashTable[h]; }
	  if ( tableType == byU32 ) { const U32* const hashTable = (U32*)tableBase; return hashTable[h] + srcBase; }
	  { const U16* const hashTable = (U16*)tableBase; return hashTable[h] + srcBase; }   /* default, to ensure a return */
  }

  static const BYTE* LZ4_getPosition( const BYTE* p, void* tableBase, tableType_t tableType, const BYTE* srcBase )
  {
	  U32 const h = LZ4_hashPosition( p, tableType );
	  return LZ4_getPositionOnHash( h, tableBase, tableType, srcBase );
  }


  /** LZ4_compress_generic() :
  inlined, to ensure branches are decided at compilation time */
  FORCE_INLINE int LZ4_compress_generic(
	  void* const ctx,
	  const char* const source,
	  char* const dest,
	  const int inputSize,
	  const int maxOutputSize,
	  const limitedOutput_directive outputLimited,
	  const tableType_t tableType,
	  const dict_directive dict,
	  const dictIssue_directive dictIssue,
	  const U32 acceleration )
  {
	  LZ4_stream_t_internal* const dictPtr = (LZ4_stream_t_internal*)ctx;

	  const BYTE* ip = (const BYTE*)source;
	  const BYTE* base;
	  const BYTE* lowLimit;
	  const BYTE* const lowRefLimit = ip - dictPtr->dictSize;
	  const BYTE* const dictionary = dictPtr->dictionary;
	  const BYTE* const dictEnd = dictionary + dictPtr->dictSize;
	  const size_t dictDelta = dictEnd - (const BYTE*)source;
	  const BYTE* anchor = (const BYTE*)source;
	  const BYTE* const iend = ip + inputSize;
	  const BYTE* const mflimit = iend - MFLIMIT;
	  const BYTE* const matchlimit = iend - LASTLITERALS;

	  BYTE* op = (BYTE*)dest;
	  BYTE* const olimit = op + maxOutputSize;

	  U32 forwardH;
	  size_t refDelta = 0;

	  /* Init conditions */
	  if ( (U32)inputSize > (U32)LZ4_MAX_INPUT_SIZE ) return 0;   /* Unsupported inputSize, too large (or negative) */
	  switch ( dict ) {
	  case noDict:
	  default:
		  base = (const BYTE*)source;
		  lowLimit = (const BYTE*)source;
		  break;
	  case withPrefix64k:
		  base = (const BYTE*)source - dictPtr->currentOffset;
		  lowLimit = (const BYTE*)source - dictPtr->dictSize;
		  break;
	  case usingExtDict:
		  base = (const BYTE*)source - dictPtr->currentOffset;
		  lowLimit = (const BYTE*)source;
		  break;
	  }
	  if ( (tableType == byU16) && (inputSize >= LZ4_64Klimit) ) return 0;   /* Size too large (not within 64K limit) */
	  if ( inputSize<LZ4_minLength ) goto _last_literals;                  /* Input too small, no compression (all literals) */

																		   /* First Byte */
	  LZ4_putPosition( ip, ctx, tableType, base );
	  ip++; forwardH = LZ4_hashPosition( ip, tableType );

	  /* Main Loop */
	  for ( ; ; ) {
		  const BYTE* match;
		  BYTE* token;

		  /* Find a match */
		  {
			  const BYTE* forwardIp = ip;
			  unsigned step = 1;
			  unsigned searchMatchNb = acceleration << LZ4_skipTrigger;
			  do {
				  U32 const h = forwardH;
				  ip = forwardIp;
				  forwardIp += step;
				  step = (searchMatchNb++ >> LZ4_skipTrigger);

				  if ( unlikely( forwardIp > mflimit ) ) goto _last_literals;

				  match = LZ4_getPositionOnHash( h, ctx, tableType, base );
				  if ( dict == usingExtDict ) {
					  if ( match < (const BYTE*)source ) {
						  refDelta = dictDelta;
						  lowLimit = dictionary;
					  } else {
						  refDelta = 0;
						  lowLimit = (const BYTE*)source;
					  }
				  }
				  forwardH = LZ4_hashPosition( forwardIp, tableType );
				  LZ4_putPositionOnHash( ip, h, ctx, tableType, base );

			  } while ( ((dictIssue == dictSmall) ? (match < lowRefLimit) : 0)
						|| ((tableType == byU16) ? 0 : (match + MAX_DISTANCE < ip))
						|| (LZ4_read32( match + refDelta ) != LZ4_read32( ip )) );
		  }

		  /* Catch up */
		  while ( ((ip>anchor) & (match + refDelta > lowLimit)) && (unlikely( ip[-1] == match[refDelta - 1] )) ) { ip--; match--; }

		  /* Encode Literals */
		  {
			  unsigned const litLength = (unsigned)(ip - anchor);
			  token = op++;
			  if ( (outputLimited) &&  /* Check output buffer overflow */
				  (unlikely( op + litLength + (2 + 1 + LASTLITERALS) + (litLength / 255) > olimit )) )
				  return 0;
			  if ( litLength >= RUN_MASK ) {
				  int len = (int)litLength - RUN_MASK;
				  *token = (RUN_MASK << ML_BITS);
				  for ( ; len >= 255; len -= 255 ) *op++ = 255;
				  *op++ = (BYTE)len;
			  } else *token = (BYTE)(litLength << ML_BITS);

			  /* Copy Literals */
			  LZ4_wildCopy( op, anchor, op + litLength );
			  op += litLength;
		  }

	  _next_match:
		  /* Encode Offset */
		  LZ4_writeLE16( op, (U16)(ip - match) ); op += 2;

		  /* Encode MatchLength */
		  {
			  unsigned matchCode;

			  if ( (dict == usingExtDict) && (lowLimit == dictionary) ) {
				  const BYTE* limit;
				  match += refDelta;
				  limit = ip + (dictEnd - match);
				  if ( limit > matchlimit ) limit = matchlimit;
				  matchCode = LZ4_count( ip + MINMATCH, match + MINMATCH, limit );
				  ip += MINMATCH + matchCode;
				  if ( ip == limit ) {
					  unsigned const more = LZ4_count( ip, (const BYTE*)source, matchlimit );
					  matchCode += more;
					  ip += more;
				  }
			  } else {
				  matchCode = LZ4_count( ip + MINMATCH, match + MINMATCH, matchlimit );
				  ip += MINMATCH + matchCode;
			  }

			  if ( outputLimited &&    /* Check output buffer overflow */
				  (unlikely( op + (1 + LASTLITERALS) + (matchCode >> 8) > olimit )) )
				  return 0;
			  if ( matchCode >= ML_MASK ) {
				  *token += ML_MASK;
				  matchCode -= ML_MASK;
				  LZ4_write32( op, 0xFFFFFFFF );
				  while ( matchCode >= 4 * 255 ) op += 4, LZ4_write32( op, 0xFFFFFFFF ), matchCode -= 4 * 255;
				  op += matchCode / 255;
				  *op++ = (BYTE)(matchCode % 255);
			  } else
				  *token += (BYTE)(matchCode);
		  }

		  anchor = ip;

		  /* Test end of chunk */
		  if ( ip > mflimit ) break;

		  /* Fill table */
		  LZ4_putPosition( ip - 2, ctx, tableType, base );

		  /* Test next position */
		  match = LZ4_getPosition( ip, ctx, tableType, base );
		  if ( dict == usingExtDict ) {
			  if ( match < (const BYTE*)source ) {
				  refDelta = dictDelta;
				  lowLimit = dictionary;
			  } else {
				  refDelta = 0;
				  lowLimit = (const BYTE*)source;
			  }
		  }
		  LZ4_putPosition( ip, ctx, tableType, base );
		  if ( ((dictIssue == dictSmall) ? (match >= lowRefLimit) : 1)
			   && (match + MAX_DISTANCE >= ip)
			   && (LZ4_read32( match + refDelta ) == LZ4_read32( ip )) ) {
			  token = op++; *token = 0; goto _next_match;
		  }

		  /* Prepare next loop */
		  forwardH = LZ4_hashPosition( ++ip, tableType );
	  }

  _last_literals:
	  /* Encode Last Literals */
	  {
		  const size_t lastRun = (size_t)(iend - anchor);
		  if ( (outputLimited) &&  /* Check output buffer overflow */
			  ((op - (BYTE*)dest) + lastRun + 1 + ((lastRun + 255 - RUN_MASK) / 255) > (U32)maxOutputSize) )
			  return 0;
		  if ( lastRun >= RUN_MASK ) {
			  size_t accumulator = lastRun - RUN_MASK;
			  *op++ = RUN_MASK << ML_BITS;
			  for ( ; accumulator >= 255; accumulator -= 255 ) *op++ = 255;
			  *op++ = (BYTE)accumulator;
		  } else {
			  *op++ = (BYTE)(lastRun << ML_BITS);
		  }
		  memcpy( op, anchor, lastRun );
		  op += lastRun;
	  }

	  /* End */
	  return (int)(((char*)op) - dest);
  }


  int LZ4_compress_fast_extState( void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration )
  {
	  LZ4_resetStream( (LZ4_stream_t*)state );
	  if ( acceleration < 1 ) acceleration = ACCELERATION_DEFAULT;

	  if ( maxOutputSize >= LZ4_compressBound( inputSize ) ) {
		  if ( inputSize < LZ4_64Klimit )
			  return LZ4_compress_generic( state, source, dest, inputSize, 0, notLimited, byU16, noDict, noDictIssue, acceleration );
		  else
			  return LZ4_compress_generic( state, source, dest, inputSize, 0, notLimited, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue, acceleration );
	  } else {
		  if ( inputSize < LZ4_64Klimit )
			  return LZ4_compress_generic( state, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue, acceleration );
		  else
			  return LZ4_compress_generic( state, source, dest, inputSize, maxOutputSize, limitedOutput, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue, acceleration );
	  }
  }


  int LZ4_compress_fast( const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration )
  {
#if (HEAPMODE)
	  void* ctxPtr = ALLOCATOR( 1, sizeof( LZ4_stream_t ) );   /* malloc-calloc always properly aligned */
#else
	  LZ4_stream_t ctx;
	  void* ctxPtr = &ctx;
#endif

	  int result = LZ4_compress_fast_extState( ctxPtr, source, dest, inputSize, maxOutputSize, acceleration );

#if (HEAPMODE)
	  FREEMEM( ctxPtr );
#endif
	  return result;
  }


  int LZ4_compress_default( const char* source, char* dest, int inputSize, int maxOutputSize )
  {
	  return LZ4_compress_fast( source, dest, inputSize, maxOutputSize, 1 );
  }


  /* hidden debug function */
  /* strangely enough, gcc generates faster code when this function is uncommented, even if unused */
  int LZ4_compress_fast_force( const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration )
  {
	  LZ4_stream_t ctx;
	  LZ4_resetStream( &ctx );

	  if ( inputSize < LZ4_64Klimit )
		  return LZ4_compress_generic( &ctx, source, dest, inputSize, maxOutputSize, limitedOutput, byU16, noDict, noDictIssue, acceleration );
	  else
		  return LZ4_compress_generic( &ctx, source, dest, inputSize, maxOutputSize, limitedOutput, LZ4_64bits() ? byU32 : byPtr, noDict, noDictIssue, acceleration );
  }


  /*-******************************
  *  *_destSize() variant
  ********************************/

  static int LZ4_compress_destSize_generic(
	  void* const ctx,
	  const char* const src,
	  char* const dst,
	  int*  const srcSizePtr,
	  const int targetDstSize,
	  const tableType_t tableType )
  {
	  const BYTE* ip = (const BYTE*)src;
	  const BYTE* base = (const BYTE*)src;
	  const BYTE* lowLimit = (const BYTE*)src;
	  const BYTE* anchor = ip;
	  const BYTE* const iend = ip + *srcSizePtr;
	  const BYTE* const mflimit = iend - MFLIMIT;
	  const BYTE* const matchlimit = iend - LASTLITERALS;

	  BYTE* op = (BYTE*)dst;
	  BYTE* const oend = op + targetDstSize;
	  BYTE* const oMaxLit = op + targetDstSize - 2 /* offset */ - 8 /* because 8+MINMATCH==MFLIMIT */ - 1 /* token */;
	  BYTE* const oMaxMatch = op + targetDstSize - (LASTLITERALS + 1 /* token */);
	  BYTE* const oMaxSeq = oMaxLit - 1 /* token */;

	  U32 forwardH;


	  /* Init conditions */
	  if ( targetDstSize < 1 ) return 0;                                     /* Impossible to store anything */
	  if ( (U32)*srcSizePtr >( U32 )LZ4_MAX_INPUT_SIZE ) return 0;            /* Unsupported input size, too large (or negative) */
	  if ( (tableType == byU16) && (*srcSizePtr >= LZ4_64Klimit) ) return 0;   /* Size too large (not within 64K limit) */
	  if ( *srcSizePtr<LZ4_minLength ) goto _last_literals;                  /* Input too small, no compression (all literals) */

																			 /* First Byte */
	  *srcSizePtr = 0;
	  LZ4_putPosition( ip, ctx, tableType, base );
	  ip++; forwardH = LZ4_hashPosition( ip, tableType );

	  /* Main Loop */
	  for ( ; ; ) {
		  const BYTE* match;
		  BYTE* token;

		  /* Find a match */
		  {
			  const BYTE* forwardIp = ip;
			  unsigned step = 1;
			  unsigned searchMatchNb = 1 << LZ4_skipTrigger;

			  do {
				  U32 h = forwardH;
				  ip = forwardIp;
				  forwardIp += step;
				  step = (searchMatchNb++ >> LZ4_skipTrigger);

				  if ( unlikely( forwardIp > mflimit ) ) goto _last_literals;

				  match = LZ4_getPositionOnHash( h, ctx, tableType, base );
				  forwardH = LZ4_hashPosition( forwardIp, tableType );
				  LZ4_putPositionOnHash( ip, h, ctx, tableType, base );

			  } while ( ((tableType == byU16) ? 0 : (match + MAX_DISTANCE < ip))
						|| (LZ4_read32( match ) != LZ4_read32( ip )) );
		  }

		  /* Catch up */
		  while ( (ip>anchor) && (match > lowLimit) && (unlikely( ip[-1] == match[-1] )) ) { ip--; match--; }

		  /* Encode Literal length */
		  {
			  unsigned litLength = (unsigned)(ip - anchor);
			  token = op++;
			  if ( op + ((litLength + 240) / 255) + litLength > oMaxLit ) {
				  /* Not enough space for a last match */
				  op--;
				  goto _last_literals;
			  }
			  if ( litLength >= RUN_MASK ) {
				  unsigned len = litLength - RUN_MASK;
				  *token = (RUN_MASK << ML_BITS);
				  for ( ; len >= 255; len -= 255 ) *op++ = 255;
				  *op++ = (BYTE)len;
			  } else *token = (BYTE)(litLength << ML_BITS);

			  /* Copy Literals */
			  LZ4_wildCopy( op, anchor, op + litLength );
			  op += litLength;
		  }

	  _next_match:
		  /* Encode Offset */
		  LZ4_writeLE16( op, (U16)(ip - match) ); op += 2;

		  /* Encode MatchLength */
		  {
			  size_t matchLength = LZ4_count( ip + MINMATCH, match + MINMATCH, matchlimit );

			  if ( op + ((matchLength + 240) / 255) > oMaxMatch ) {
				  /* Match description too long : reduce it */
				  matchLength = (15 - 1) + (oMaxMatch - op) * 255;
			  }
			  ip += MINMATCH + matchLength;

			  if ( matchLength >= ML_MASK ) {
				  *token += ML_MASK;
				  matchLength -= ML_MASK;
				  while ( matchLength >= 255 ) { matchLength -= 255; *op++ = 255; }
				  *op++ = (BYTE)matchLength;
			  } else *token += (BYTE)(matchLength);
		  }

		  anchor = ip;

		  /* Test end of block */
		  if ( ip > mflimit ) break;
		  if ( op > oMaxSeq ) break;

		  /* Fill table */
		  LZ4_putPosition( ip - 2, ctx, tableType, base );

		  /* Test next position */
		  match = LZ4_getPosition( ip, ctx, tableType, base );
		  LZ4_putPosition( ip, ctx, tableType, base );
		  if ( (match + MAX_DISTANCE >= ip)
			   && (LZ4_read32( match ) == LZ4_read32( ip )) ) {
			  token = op++; *token = 0; goto _next_match;
		  }

		  /* Prepare next loop */
		  forwardH = LZ4_hashPosition( ++ip, tableType );
	  }

  _last_literals:
	  /* Encode Last Literals */
	  {
		  size_t lastRunSize = (size_t)(iend - anchor);
		  if ( op + 1 /* token */ + ((lastRunSize + 240) / 255) /* litLength */ + lastRunSize /* literals */ > oend ) {
			  /* adapt lastRunSize to fill 'dst' */
			  lastRunSize = (oend - op) - 1;
			  lastRunSize -= (lastRunSize + 240) / 255;
		  }
		  ip = anchor + lastRunSize;

		  if ( lastRunSize >= RUN_MASK ) {
			  size_t accumulator = lastRunSize - RUN_MASK;
			  *op++ = RUN_MASK << ML_BITS;
			  for ( ; accumulator >= 255; accumulator -= 255 ) *op++ = 255;
			  *op++ = (BYTE)accumulator;
		  } else {
			  *op++ = (BYTE)(lastRunSize << ML_BITS);
		  }
		  memcpy( op, anchor, lastRunSize );
		  op += lastRunSize;
	  }

	  /* End */
	  *srcSizePtr = (int)(((const char*)ip) - src);
	  return (int)(((char*)op) - dst);
  }


  static int LZ4_compress_destSize_extState( void* state, const char* src, char* dst, int* srcSizePtr, int targetDstSize )
  {
	  LZ4_resetStream( (LZ4_stream_t*)state );

	  if ( targetDstSize >= LZ4_compressBound( *srcSizePtr ) ) {  /* compression success is guaranteed */
		  return LZ4_compress_fast_extState( state, src, dst, *srcSizePtr, targetDstSize, 1 );
	  } else {
		  if ( *srcSizePtr < LZ4_64Klimit )
			  return LZ4_compress_destSize_generic( state, src, dst, srcSizePtr, targetDstSize, byU16 );
		  else
			  return LZ4_compress_destSize_generic( state, src, dst, srcSizePtr, targetDstSize, LZ4_64bits() ? byU32 : byPtr );
	  }
  }


  int LZ4_compress_destSize( const char* src, char* dst, int* srcSizePtr, int targetDstSize )
  {
#if (HEAPMODE)
	  void* ctx = ALLOCATOR( 1, sizeof( LZ4_stream_t ) );   /* malloc-calloc always properly aligned */
#else
	  LZ4_stream_t ctxBody;
	  void* ctx = &ctxBody;
#endif

	  int result = LZ4_compress_destSize_extState( ctx, src, dst, srcSizePtr, targetDstSize );

#if (HEAPMODE)
	  FREEMEM( ctx );
#endif
	  return result;
  }



  /*-******************************
  *  Streaming functions
  ********************************/

  LZ4_stream_t* LZ4_createStream( void )
  {
	  LZ4_stream_t* lz4s = (LZ4_stream_t*)ALLOCATOR( 8, LZ4_STREAMSIZE_U64 );
	  LZ4_STATIC_ASSERT( LZ4_STREAMSIZE >= sizeof( LZ4_stream_t_internal ) );    /* A compilation error here means LZ4_STREAMSIZE is not large enough */
	  LZ4_resetStream( lz4s );
	  return lz4s;
  }

  void LZ4_resetStream( LZ4_stream_t* LZ4_stream )
  {
	  MEM_INIT( LZ4_stream, 0, sizeof( LZ4_stream_t ) );
  }

  int LZ4_freeStream( LZ4_stream_t* LZ4_stream )
  {
	  FREEMEM( LZ4_stream );
	  return (0);
  }


#define HASH_UNIT sizeof(size_t)
  int LZ4_loadDict( LZ4_stream_t* LZ4_dict, const char* dictionary, int dictSize )
  {
	  LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*)LZ4_dict;
	  const BYTE* p = (const BYTE*)dictionary;
	  const BYTE* const dictEnd = p + dictSize;
	  const BYTE* base;

	  if ( (dict->initCheck) || (dict->currentOffset > 1 GB) )  /* Uninitialized structure, or reuse overflow */
		  LZ4_resetStream( LZ4_dict );

	  if ( dictSize < (int)HASH_UNIT ) {
		  dict->dictionary = NULL;
		  dict->dictSize = 0;
		  return 0;
	  }

	  if ( (dictEnd - p) > 64 KB ) p = dictEnd - 64 KB;
	  dict->currentOffset += 64 KB;
	  base = p - dict->currentOffset;
	  dict->dictionary = p;
	  dict->dictSize = (U32)(dictEnd - p);
	  dict->currentOffset += dict->dictSize;

	  while ( p <= dictEnd - HASH_UNIT ) {
		  LZ4_putPosition( p, dict->hashTable, byU32, base );
		  p += 3;
	  }

	  return dict->dictSize;
  }


  static void LZ4_renormDictT( LZ4_stream_t_internal* LZ4_dict, const BYTE* src )
  {
	  if ( (LZ4_dict->currentOffset > 0x80000000) ||
		  ((size_t)LZ4_dict->currentOffset > (size_t)src) ) {   /* address space overflow */
																/* rescale hash table */
		  U32 const delta = LZ4_dict->currentOffset - 64 KB;
		  const BYTE* dictEnd = LZ4_dict->dictionary + LZ4_dict->dictSize;
		  int i;
		  for ( i = 0; i<HASH_SIZE_U32; i++ ) {
			  if ( LZ4_dict->hashTable[i] < delta ) LZ4_dict->hashTable[i] = 0;
			  else LZ4_dict->hashTable[i] -= delta;
		  }
		  LZ4_dict->currentOffset = 64 KB;
		  if ( LZ4_dict->dictSize > 64 KB ) LZ4_dict->dictSize = 64 KB;
		  LZ4_dict->dictionary = dictEnd - LZ4_dict->dictSize;
	  }
  }


  int LZ4_compress_fast_continue( LZ4_stream_t* LZ4_stream, const char* source, char* dest, int inputSize, int maxOutputSize, int acceleration )
  {
	  LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_stream;
	  const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

	  const BYTE* smallest = (const BYTE*)source;
	  if ( streamPtr->initCheck ) return 0;   /* Uninitialized structure detected */
	  if ( (streamPtr->dictSize>0) && (smallest>dictEnd) ) smallest = dictEnd;
	  LZ4_renormDictT( streamPtr, smallest );
	  if ( acceleration < 1 ) acceleration = ACCELERATION_DEFAULT;

	  /* Check overlapping input/dictionary space */
	  {
		  const BYTE* sourceEnd = (const BYTE*)source + inputSize;
		  if ( (sourceEnd > streamPtr->dictionary) && (sourceEnd < dictEnd) ) {
			  streamPtr->dictSize = (U32)(dictEnd - sourceEnd);
			  if ( streamPtr->dictSize > 64 KB ) streamPtr->dictSize = 64 KB;
			  if ( streamPtr->dictSize < 4 ) streamPtr->dictSize = 0;
			  streamPtr->dictionary = dictEnd - streamPtr->dictSize;
		  }
	  }

	  /* prefix mode : source data follows dictionary */
	  if ( dictEnd == (const BYTE*)source ) {
		  int result;
		  if ( (streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset) )
			  result = LZ4_compress_generic( LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput, byU32, withPrefix64k, dictSmall, acceleration );
		  else
			  result = LZ4_compress_generic( LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput, byU32, withPrefix64k, noDictIssue, acceleration );
		  streamPtr->dictSize += (U32)inputSize;
		  streamPtr->currentOffset += (U32)inputSize;
		  return result;
	  }

	  /* external dictionary mode */
	  {
		  int result;
		  if ( (streamPtr->dictSize < 64 KB) && (streamPtr->dictSize < streamPtr->currentOffset) )
			  result = LZ4_compress_generic( LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput, byU32, usingExtDict, dictSmall, acceleration );
		  else
			  result = LZ4_compress_generic( LZ4_stream, source, dest, inputSize, maxOutputSize, limitedOutput, byU32, usingExtDict, noDictIssue, acceleration );
		  streamPtr->dictionary = (const BYTE*)source;
		  streamPtr->dictSize = (U32)inputSize;
		  streamPtr->currentOffset += (U32)inputSize;
		  return result;
	  }
  }


  /* Hidden debug function, to force external dictionary mode */
  int LZ4_compress_forceExtDict( LZ4_stream_t* LZ4_dict, const char* source, char* dest, int inputSize )
  {
	  LZ4_stream_t_internal* streamPtr = (LZ4_stream_t_internal*)LZ4_dict;
	  int result;
	  const BYTE* const dictEnd = streamPtr->dictionary + streamPtr->dictSize;

	  const BYTE* smallest = dictEnd;
	  if ( smallest > (const BYTE*)source ) smallest = (const BYTE*)source;
	  LZ4_renormDictT( (LZ4_stream_t_internal*)LZ4_dict, smallest );

	  result = LZ4_compress_generic( LZ4_dict, source, dest, inputSize, 0, notLimited, byU32, usingExtDict, noDictIssue, 1 );

	  streamPtr->dictionary = (const BYTE*)source;
	  streamPtr->dictSize = (U32)inputSize;
	  streamPtr->currentOffset += (U32)inputSize;

	  return result;
  }


  int LZ4_saveDict( LZ4_stream_t* LZ4_dict, char* safeBuffer, int dictSize )
  {
	  LZ4_stream_t_internal* dict = (LZ4_stream_t_internal*)LZ4_dict;
	  const BYTE* previousDictEnd = dict->dictionary + dict->dictSize;

	  if ( (U32)dictSize > 64 KB ) dictSize = 64 KB;   /* useless to define a dictionary > 64 KB */
	  if ( (U32)dictSize > dict->dictSize ) dictSize = dict->dictSize;

	  memmove( safeBuffer, previousDictEnd - dictSize, dictSize );

	  dict->dictionary = (const BYTE*)safeBuffer;
	  dict->dictSize = (U32)dictSize;

	  return dictSize;
  }



  /*-*****************************
  *  Decompression functions
  *******************************/
  /*! LZ4_decompress_generic() :
  *  This generic decompression function cover all use cases.
  *  It shall be instantiated several times, using different sets of directives
  *  Note that it is important this generic function is really inlined,
  *  in order to remove useless branches during compilation optimization.
  */
  FORCE_INLINE int LZ4_decompress_generic(
	  const char* const source,
	  char* const dest,
	  int inputSize,
	  int outputSize,         /* If endOnInput==endOnInputSize, this value is the max size of Output Buffer. */

	  int endOnInput,         /* endOnOutputSize, endOnInputSize */
	  int partialDecoding,    /* full, partial */
	  int targetOutputSize,   /* only used if partialDecoding==partial */
	  int dict,               /* noDict, withPrefix64k, usingExtDict */
	  const BYTE* const lowPrefix,  /* == dest when no prefix */
	  const BYTE* const dictStart,  /* only if dict==usingExtDict */
	  const size_t dictSize         /* note : = 0 if noDict */
  )
  {
	  /* Local Variables */
	  const BYTE* ip = (const BYTE*)source;
	  const BYTE* const iend = ip + inputSize;

	  BYTE* op = (BYTE*)dest;
	  BYTE* const oend = op + outputSize;
	  BYTE* cpy;
	  BYTE* oexit = op + targetOutputSize;
	  const BYTE* const lowLimit = lowPrefix - dictSize;

	  const BYTE* const dictEnd = (const BYTE*)dictStart + dictSize;
	  const unsigned dec32table[] = { 4, 1, 2, 1, 4, 4, 4, 4 };
	  const int dec64table[] = { 0, 0, 0, -1, 0, 1, 2, 3 };

	  const int safeDecode = (endOnInput == endOnInputSize);
	  const int checkOffset = ((safeDecode) && (dictSize < (int)(64 KB)));


	  /* Special cases */
	  if ( (partialDecoding) && (oexit > oend - MFLIMIT) ) oexit = oend - MFLIMIT;                        /* targetOutputSize too high => decode everything */
	  if ( (endOnInput) && (unlikely( outputSize == 0 )) ) return ((inputSize == 1) && (*ip == 0)) ? 0 : -1;  /* Empty output buffer */
	  if ( (!endOnInput) && (unlikely( outputSize == 0 )) ) return (*ip == 0 ? 1 : -1);

	  /* Main Loop : decode sequences */
	  while ( 1 ) {
		  unsigned token;
		  size_t length;
		  const BYTE* match;
		  size_t offset;

		  /* get literal length */
		  token = *ip++;
		  if ( (length = (token >> ML_BITS)) == RUN_MASK ) {
			  unsigned s;
			  do {
				  s = *ip++;
				  length += s;
			  } while ( likely( endOnInput ? ip<iend - RUN_MASK : 1 ) & (s == 255) );
			  if ( (safeDecode) && unlikely( (size_t)(op + length)<(size_t)(op) ) ) goto _output_error;   /* overflow detection */
			  if ( (safeDecode) && unlikely( (size_t)(ip + length)<(size_t)(ip) ) ) goto _output_error;   /* overflow detection */
		  }

		  /* copy literals */
		  cpy = op + length;
		  if ( ((endOnInput) && ((cpy>(partialDecoding ? oexit : oend - MFLIMIT)) || (ip + length>iend - (2 + 1 + LASTLITERALS))))
			   || ((!endOnInput) && (cpy>oend - WILDCOPYLENGTH)) ) {
			  if ( partialDecoding ) {
				  if ( cpy > oend ) goto _output_error;                           /* Error : write attempt beyond end of output buffer */
				  if ( (endOnInput) && (ip + length > iend) ) goto _output_error;   /* Error : read attempt beyond end of input buffer */
			  } else {
				  if ( (!endOnInput) && (cpy != oend) ) goto _output_error;       /* Error : block decoding must stop exactly there */
				  if ( (endOnInput) && ((ip + length != iend) || (cpy > oend)) ) goto _output_error;   /* Error : input must be consumed */
			  }
			  memcpy( op, ip, length );
			  ip += length;
			  op += length;
			  break;     /* Necessarily EOF, due to parsing restrictions */
		  }
		  LZ4_wildCopy( op, ip, cpy );
		  ip += length; op = cpy;

		  /* get offset */
		  offset = LZ4_readLE16( ip ); ip += 2;
		  match = op - offset;
		  if ( (checkOffset) && (unlikely( match < lowLimit )) ) goto _output_error;   /* Error : offset outside buffers */

																					   /* get matchlength */
		  length = token & ML_MASK;
		  if ( length == ML_MASK ) {
			  unsigned s;
			  do {
				  s = *ip++;
				  if ( (endOnInput) && (ip > iend - LASTLITERALS) ) goto _output_error;
				  length += s;
			  } while ( s == 255 );
			  if ( (safeDecode) && unlikely( (size_t)(op + length)<(size_t)op ) ) goto _output_error;   /* overflow detection */
		  }
		  length += MINMATCH;

		  /* check external dictionary */
		  if ( (dict == usingExtDict) && (match < lowPrefix) ) {
			  if ( unlikely( op + length > oend - LASTLITERALS ) ) goto _output_error;   /* doesn't respect parsing restriction */

			  if ( length <= (size_t)(lowPrefix - match) ) {
				  /* match can be copied as a single segment from external dictionary */
				  memmove( op, dictEnd - (lowPrefix - match), length );
				  op += length;
			  } else {
				  /* match encompass external dictionary and current block */
				  size_t const copySize = (size_t)(lowPrefix - match);
				  size_t const restSize = length - copySize;
				  memcpy( op, dictEnd - copySize, copySize );
				  op += copySize;
				  if ( restSize > (size_t)(op - lowPrefix) ) {  /* overlap copy */
					  BYTE* const endOfMatch = op + restSize;
					  const BYTE* copyFrom = lowPrefix;
					  while ( op < endOfMatch ) *op++ = *copyFrom++;
				  } else {
					  memcpy( op, lowPrefix, restSize );
					  op += restSize;
				  }
			  }
			  continue;
		  }

		  /* copy match within block */
		  cpy = op + length;
		  if ( unlikely( offset<8 ) ) {
			  const int dec64 = dec64table[offset];
			  op[0] = match[0];
			  op[1] = match[1];
			  op[2] = match[2];
			  op[3] = match[3];
			  match += dec32table[offset];
			  memcpy( op + 4, match, 4 );
			  match -= dec64;
		  } else { LZ4_copy8( op, match ); match += 8; }
		  op += 8;

		  if ( unlikely( cpy>oend - 12 ) ) {
			  BYTE* const oCopyLimit = oend - (WILDCOPYLENGTH - 1);
			  if ( cpy > oend - LASTLITERALS ) goto _output_error;    /* Error : last LASTLITERALS bytes must be literals (uncompressed) */
			  if ( op < oCopyLimit ) {
				  LZ4_wildCopy( op, match, oCopyLimit );
				  match += oCopyLimit - op;
				  op = oCopyLimit;
			  }
			  while ( op<cpy ) *op++ = *match++;
		  } else {
			  LZ4_copy8( op, match );
			  if ( length>16 ) LZ4_wildCopy( op + 8, match + 8, cpy );
		  }
		  op = cpy;   /* correction */
	  }

	  /* end of decoding */
	  if ( endOnInput )
		  return (int)(((char*)op) - dest);     /* Nb of output bytes decoded */
	  else
		  return (int)(((const char*)ip) - source);   /* Nb of input bytes read */

													  /* Overflow error detected */
  _output_error:
	  return (int)(-(((const char*)ip) - source)) - 1;
  }


  int LZ4_decompress_safe( const char* source, char* dest, int compressedSize, int maxDecompressedSize )
  {
	  return LZ4_decompress_generic( source, dest, compressedSize, maxDecompressedSize, endOnInputSize, full, 0, noDict, (BYTE*)dest, NULL, 0 );
  }

  int LZ4_decompress_safe_partial( const char* source, char* dest, int compressedSize, int targetOutputSize, int maxDecompressedSize )
  {
	  return LZ4_decompress_generic( source, dest, compressedSize, maxDecompressedSize, endOnInputSize, partial, targetOutputSize, noDict, (BYTE*)dest, NULL, 0 );
  }

  int LZ4_decompress_fast( const char* source, char* dest, int originalSize )
  {
	  return LZ4_decompress_generic( source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, (BYTE*)(dest - 64 KB), NULL, 64 KB );
  }


  /*===== streaming decompression functions =====*/

  typedef struct
  {
	  const BYTE* externalDict;
	  size_t extDictSize;
	  const BYTE* prefixEnd;
	  size_t prefixSize;
  } LZ4_streamDecode_t_internal;

  /*
  * If you prefer dynamic allocation methods,
  * LZ4_createStreamDecode()
  * provides a pointer (void*) towards an initialized LZ4_streamDecode_t structure.
  */
  LZ4_streamDecode_t* LZ4_createStreamDecode( void )
  {
	  LZ4_streamDecode_t* lz4s = (LZ4_streamDecode_t*)ALLOCATOR( 1, sizeof( LZ4_streamDecode_t ) );
	  return lz4s;
  }

  int LZ4_freeStreamDecode( LZ4_streamDecode_t* LZ4_stream )
  {
	  FREEMEM( LZ4_stream );
	  return 0;
  }

  /*!
  * LZ4_setStreamDecode() :
  * Use this function to instruct where to find the dictionary.
  * This function is not necessary if previous data is still available where it was decoded.
  * Loading a size of 0 is allowed (same effect as no dictionary).
  * Return : 1 if OK, 0 if error
  */
  int LZ4_setStreamDecode( LZ4_streamDecode_t* LZ4_streamDecode, const char* dictionary, int dictSize )
  {
	  LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*)LZ4_streamDecode;
	  lz4sd->prefixSize = (size_t)dictSize;
	  lz4sd->prefixEnd = (const BYTE*)dictionary + dictSize;
	  lz4sd->externalDict = NULL;
	  lz4sd->extDictSize = 0;
	  return 1;
  }

  /*
  *_continue() :
  These decoding functions allow decompression of multiple blocks in "streaming" mode.
  Previously decoded blocks must still be available at the memory position where they were decoded.
  If it's not possible, save the relevant part of decoded data into a safe buffer,
  and indicate where it stands using LZ4_setStreamDecode()
  */
  int LZ4_decompress_safe_continue( LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxOutputSize )
  {
	  LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*)LZ4_streamDecode;
	  int result;

	  if ( lz4sd->prefixEnd == (BYTE*)dest ) {
		  result = LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize,
										   endOnInputSize, full, 0,
										   usingExtDict, lz4sd->prefixEnd - lz4sd->prefixSize, lz4sd->externalDict, lz4sd->extDictSize );
		  if ( result <= 0 ) return result;
		  lz4sd->prefixSize += result;
		  lz4sd->prefixEnd += result;
	  } else {
		  lz4sd->extDictSize = lz4sd->prefixSize;
		  lz4sd->externalDict = lz4sd->prefixEnd - lz4sd->extDictSize;
		  result = LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize,
										   endOnInputSize, full, 0,
										   usingExtDict, (BYTE*)dest, lz4sd->externalDict, lz4sd->extDictSize );
		  if ( result <= 0 ) return result;
		  lz4sd->prefixSize = result;
		  lz4sd->prefixEnd = (BYTE*)dest + result;
	  }

	  return result;
  }

  int LZ4_decompress_fast_continue( LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int originalSize )
  {
	  LZ4_streamDecode_t_internal* lz4sd = (LZ4_streamDecode_t_internal*)LZ4_streamDecode;
	  int result;

	  if ( lz4sd->prefixEnd == (BYTE*)dest ) {
		  result = LZ4_decompress_generic( source, dest, 0, originalSize,
										   endOnOutputSize, full, 0,
										   usingExtDict, lz4sd->prefixEnd - lz4sd->prefixSize, lz4sd->externalDict, lz4sd->extDictSize );
		  if ( result <= 0 ) return result;
		  lz4sd->prefixSize += originalSize;
		  lz4sd->prefixEnd += originalSize;
	  } else {
		  lz4sd->extDictSize = lz4sd->prefixSize;
		  lz4sd->externalDict = (BYTE*)dest - lz4sd->extDictSize;
		  result = LZ4_decompress_generic( source, dest, 0, originalSize,
										   endOnOutputSize, full, 0,
										   usingExtDict, (BYTE*)dest, lz4sd->externalDict, lz4sd->extDictSize );
		  if ( result <= 0 ) return result;
		  lz4sd->prefixSize = originalSize;
		  lz4sd->prefixEnd = (BYTE*)dest + originalSize;
	  }

	  return result;
  }


  /*
  Advanced decoding functions :
  *_usingDict() :
  These decoding functions work the same as "_continue" ones,
  the dictionary must be explicitly provided within parameters
  */

  FORCE_INLINE int LZ4_decompress_usingDict_generic( const char* source, char* dest, int compressedSize, int maxOutputSize, int safe, const char* dictStart, int dictSize )
  {
	  if ( dictSize == 0 )
		  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, safe, full, 0, noDict, (BYTE*)dest, NULL, 0 );
	  if ( dictStart + dictSize == dest ) {
		  if ( dictSize >= (int)(64 KB - 1) )
			  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, safe, full, 0, withPrefix64k, (BYTE*)dest - 64 KB, NULL, 0 );
		  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, safe, full, 0, noDict, (BYTE*)dest - dictSize, NULL, 0 );
	  }
	  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, safe, full, 0, usingExtDict, (BYTE*)dest, (const BYTE*)dictStart, dictSize );
  }

  int LZ4_decompress_safe_usingDict( const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize )
  {
	  return LZ4_decompress_usingDict_generic( source, dest, compressedSize, maxOutputSize, 1, dictStart, dictSize );
  }

  int LZ4_decompress_fast_usingDict( const char* source, char* dest, int originalSize, const char* dictStart, int dictSize )
  {
	  return LZ4_decompress_usingDict_generic( source, dest, 0, originalSize, 0, dictStart, dictSize );
  }

  /* debug function */
  int LZ4_decompress_safe_forceExtDict( const char* source, char* dest, int compressedSize, int maxOutputSize, const char* dictStart, int dictSize )
  {
	  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, usingExtDict, (BYTE*)dest, (const BYTE*)dictStart, dictSize );
  }


  /*=*************************************************
  *  Obsolete Functions
  ***************************************************/
  /* obsolete compression functions */
  int LZ4_compress_limitedOutput( const char* source, char* dest, int inputSize, int maxOutputSize ) { return LZ4_compress_default( source, dest, inputSize, maxOutputSize ); }
  int LZ4_compress( const char* source, char* dest, int inputSize ) { return LZ4_compress_default( source, dest, inputSize, LZ4_compressBound( inputSize ) ); }
  int LZ4_compress_limitedOutput_withState( void* state, const char* src, char* dst, int srcSize, int dstSize ) { return LZ4_compress_fast_extState( state, src, dst, srcSize, dstSize, 1 ); }
  int LZ4_compress_withState( void* state, const char* src, char* dst, int srcSize ) { return LZ4_compress_fast_extState( state, src, dst, srcSize, LZ4_compressBound( srcSize ), 1 ); }
  int LZ4_compress_limitedOutput_continue( LZ4_stream_t* LZ4_stream, const char* src, char* dst, int srcSize, int maxDstSize ) { return LZ4_compress_fast_continue( LZ4_stream, src, dst, srcSize, maxDstSize, 1 ); }
  int LZ4_compress_continue( LZ4_stream_t* LZ4_stream, const char* source, char* dest, int inputSize ) { return LZ4_compress_fast_continue( LZ4_stream, source, dest, inputSize, LZ4_compressBound( inputSize ), 1 ); }

  /*
  These function names are deprecated and should no longer be used.
  They are only provided here for compatibility with older user programs.
  - LZ4_uncompress is totally equivalent to LZ4_decompress_fast
  - LZ4_uncompress_unknownOutputSize is totally equivalent to LZ4_decompress_safe
  */
  int LZ4_uncompress( const char* source, char* dest, int outputSize ) { return LZ4_decompress_fast( source, dest, outputSize ); }
  int LZ4_uncompress_unknownOutputSize( const char* source, char* dest, int isize, int maxOutputSize ) { return LZ4_decompress_safe( source, dest, isize, maxOutputSize ); }


  /* Obsolete Streaming functions */

  int LZ4_sizeofStreamState() { return LZ4_STREAMSIZE; }

  static void LZ4_init( LZ4_stream_t_internal* lz4ds, BYTE* base )
  {
	  MEM_INIT( lz4ds, 0, LZ4_STREAMSIZE );
	  lz4ds->bufferStart = base;
  }

  int LZ4_resetStreamState( void* state, char* inputBuffer )
  {
	  if ( (((size_t)state) & 3) != 0 ) return 1;   /* Error : pointer is not aligned on 4-bytes boundary */
	  LZ4_init( (LZ4_stream_t_internal*)state, (BYTE*)inputBuffer );
	  return 0;
  }

  void* LZ4_create( char* inputBuffer )
  {
	  void* lz4ds = ALLOCATOR( 8, LZ4_STREAMSIZE_U64 );
	  LZ4_init( (LZ4_stream_t_internal*)lz4ds, (BYTE*)inputBuffer );
	  return lz4ds;
  }

  char* LZ4_slideInputBuffer( void* LZ4_Data )
  {
	  LZ4_stream_t_internal* ctx = (LZ4_stream_t_internal*)LZ4_Data;
	  int dictSize = LZ4_saveDict( (LZ4_stream_t*)LZ4_Data, (char*)ctx->bufferStart, 64 KB );
	  return (char*)(ctx->bufferStart + dictSize);
  }

  /* Obsolete streaming decompression functions */

  int LZ4_decompress_safe_withPrefix64k( const char* source, char* dest, int compressedSize, int maxOutputSize )
  {
	  return LZ4_decompress_generic( source, dest, compressedSize, maxOutputSize, endOnInputSize, full, 0, withPrefix64k, (BYTE*)dest - 64 KB, NULL, 64 KB );
  }

  int LZ4_decompress_fast_withPrefix64k( const char* source, char* dest, int originalSize )
  {
	  return LZ4_decompress_generic( source, dest, 0, originalSize, endOnOutputSize, full, 0, withPrefix64k, (BYTE*)dest - 64 KB, NULL, 64 KB );
  }


/*
    LZ4 HC - High Compression Mode of LZ4
    Copyright (C) 2011-2015, Yann Collet.

    BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the
    distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    You can contact the author at :
       - LZ4 source repository : https://github.com/Cyan4973/lz4
       - LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
*/

  /* *************************************
  *  Local Constants
  ***************************************/
#define DICTIONARY_LOGSIZE 16
#define MAXD (1<<DICTIONARY_LOGSIZE)
#define MAXD_MASK (MAXD - 1)

#define HASH_LOG (DICTIONARY_LOGSIZE-1)
#define HASHTABLESIZEHC (1 << HASH_LOG)
#define HASH_MASK (HASHTABLESIZEHC - 1)

#define OPTIMAL_ML (int)((ML_MASK-1)+MINMATCH)


  /**************************************
  *  Local Types
  **************************************/
  typedef struct
  {
	  U32   hashTable[HASHTABLESIZEHC];
	  U16   chainTable[MAXD];
	  const BYTE* end;        /* next block here to continue on current prefix */
	  const BYTE* base;       /* All index relative to this position */
	  const BYTE* dictBase;   /* alternate base for extDict */
	  BYTE* inputBuffer;      /* deprecated */
	  U32   dictLimit;        /* below that point, need extDict */
	  U32   lowLimit;         /* below that point, no more dict */
	  U32   nextToUpdate;     /* index from which to continue dictionary update */
	  U32   compressionLevel;
  } LZ4HC_Data_Structure;


  /**************************************
  *  Local Macros
  **************************************/
#define HASH_FUNCTION(i)       (((i) * 2654435761U) >> ((MINMATCH*8)-HASH_LOG))
  //#define DELTANEXTU16(p)        chainTable[(p) & MAXD_MASK]   /* flexible, MAXD dependent */
#define DELTANEXTU16(p)        chainTable[(U16)(p)]   /* faster */

  static U32 LZ4HC_hashPtr( const void* ptr ) { return HASH_FUNCTION( LZ4_read32( ptr ) ); }



  /**************************************
  *  HC Compression
  **************************************/
  static void LZ4HC_init( LZ4HC_Data_Structure* hc4, const BYTE* start )
  {
	  MEM_INIT( (void*)hc4->hashTable, 0, sizeof( hc4->hashTable ) );
	  MEM_INIT( hc4->chainTable, 0xFF, sizeof( hc4->chainTable ) );
	  hc4->nextToUpdate = 64 KB;
	  hc4->base = start - 64 KB;
	  hc4->end = start;
	  hc4->dictBase = start - 64 KB;
	  hc4->dictLimit = 64 KB;
	  hc4->lowLimit = 64 KB;
  }


  /* Update chains up to ip (excluded) */
  FORCE_INLINE void LZ4HC_Insert( LZ4HC_Data_Structure* hc4, const BYTE* ip )
  {
	  U16* const chainTable = hc4->chainTable;
	  U32* const hashTable = hc4->hashTable;
	  const BYTE* const base = hc4->base;
	  U32 const target = (U32)(ip - base);
	  U32 idx = hc4->nextToUpdate;

	  while ( idx < target ) {
		  U32 const h = LZ4HC_hashPtr( base + idx );
		  size_t delta = idx - hashTable[h];
		  if ( delta>MAX_DISTANCE ) delta = MAX_DISTANCE;
		  DELTANEXTU16( idx ) = (U16)delta;
		  hashTable[h] = idx;
		  idx++;
	  }

	  hc4->nextToUpdate = target;
  }


  FORCE_INLINE int LZ4HC_InsertAndFindBestMatch( LZ4HC_Data_Structure* hc4,   /* Index table will be updated */
												 const BYTE* ip, const BYTE* const iLimit,
												 const BYTE** matchpos,
												 const int maxNbAttempts )
  {
	  U16* const chainTable = hc4->chainTable;
	  U32* const HashTable = hc4->hashTable;
	  const BYTE* const base = hc4->base;
	  const BYTE* const dictBase = hc4->dictBase;
	  const U32 dictLimit = hc4->dictLimit;
	  const U32 lowLimit = (hc4->lowLimit + 64 KB > (U32)(ip - base)) ? hc4->lowLimit : (U32)(ip - base) - (64 KB - 1);
	  U32 matchIndex;
	  const BYTE* match;
	  int nbAttempts = maxNbAttempts;
	  size_t ml = 0;

	  /* HC4 match finder */
	  LZ4HC_Insert( hc4, ip );
	  matchIndex = HashTable[LZ4HC_hashPtr( ip )];

	  while ( (matchIndex >= lowLimit) && (nbAttempts) ) {
		  nbAttempts--;
		  if ( matchIndex >= dictLimit ) {
			  match = base + matchIndex;
			  if ( *(match + ml) == *(ip + ml)
				   && (LZ4_read32( match ) == LZ4_read32( ip )) ) {
				  size_t const mlt = LZ4_count( ip + MINMATCH, match + MINMATCH, iLimit ) + MINMATCH;
				  if ( mlt > ml ) { ml = mlt; *matchpos = match; }
			  }
		  } else {
			  match = dictBase + matchIndex;
			  if ( LZ4_read32( match ) == LZ4_read32( ip ) ) {
				  size_t mlt;
				  const BYTE* vLimit = ip + (dictLimit - matchIndex);
				  if ( vLimit > iLimit ) vLimit = iLimit;
				  mlt = LZ4_count( ip + MINMATCH, match + MINMATCH, vLimit ) + MINMATCH;
				  if ( (ip + mlt == vLimit) && (vLimit < iLimit) )
					  mlt += LZ4_count( ip + mlt, base + dictLimit, iLimit );
				  if ( mlt > ml ) { ml = mlt; *matchpos = base + matchIndex; }   /* virtual matchpos */
			  }
		  }
		  matchIndex -= DELTANEXTU16( matchIndex );
	  }

	  return (int)ml;
  }


  FORCE_INLINE int LZ4HC_InsertAndGetWiderMatch(
	  LZ4HC_Data_Structure* hc4,
	  const BYTE* const ip,
	  const BYTE* const iLowLimit,
	  const BYTE* const iHighLimit,
	  int longest,
	  const BYTE** matchpos,
	  const BYTE** startpos,
	  const int maxNbAttempts )
  {
	  U16* const chainTable = hc4->chainTable;
	  U32* const HashTable = hc4->hashTable;
	  const BYTE* const base = hc4->base;
	  const U32 dictLimit = hc4->dictLimit;
	  const BYTE* const lowPrefixPtr = base + dictLimit;
	  const U32 lowLimit = (hc4->lowLimit + 64 KB > (U32)(ip - base)) ? hc4->lowLimit : (U32)(ip - base) - (64 KB - 1);
	  const BYTE* const dictBase = hc4->dictBase;
	  U32   matchIndex;
	  int nbAttempts = maxNbAttempts;
	  int delta = (int)(ip - iLowLimit);


	  /* First Match */
	  LZ4HC_Insert( hc4, ip );
	  matchIndex = HashTable[LZ4HC_hashPtr( ip )];

	  while ( (matchIndex >= lowLimit) && (nbAttempts) ) {
		  nbAttempts--;
		  if ( matchIndex >= dictLimit ) {
			  const BYTE* matchPtr = base + matchIndex;
			  if ( *(iLowLimit + longest) == *(matchPtr - delta + longest) ) {
				  if ( LZ4_read32( matchPtr ) == LZ4_read32( ip ) ) {
					  int mlt = MINMATCH + LZ4_count( ip + MINMATCH, matchPtr + MINMATCH, iHighLimit );
					  int back = 0;

					  while ( (ip + back > iLowLimit)
							  && (matchPtr + back > lowPrefixPtr)
							  && (ip[back - 1] == matchPtr[back - 1]) )
						  back--;

					  mlt -= back;

					  if ( mlt > longest ) {
						  longest = (int)mlt;
						  *matchpos = matchPtr + back;
						  *startpos = ip + back;
					  }
				  }
			  }
		  } else {
			  const BYTE* matchPtr = dictBase + matchIndex;
			  if ( LZ4_read32( matchPtr ) == LZ4_read32( ip ) ) {
				  size_t mlt;
				  int back = 0;
				  const BYTE* vLimit = ip + (dictLimit - matchIndex);
				  if ( vLimit > iHighLimit ) vLimit = iHighLimit;
				  mlt = LZ4_count( ip + MINMATCH, matchPtr + MINMATCH, vLimit ) + MINMATCH;
				  if ( (ip + mlt == vLimit) && (vLimit < iHighLimit) )
					  mlt += LZ4_count( ip + mlt, base + dictLimit, iHighLimit );
				  while ( (ip + back > iLowLimit) && (matchIndex + back > lowLimit) && (ip[back - 1] == matchPtr[back - 1]) ) back--;
				  mlt -= back;
				  if ( (int)mlt > longest ) { longest = (int)mlt; *matchpos = base + matchIndex + back; *startpos = ip + back; }
			  }
		  }
		  matchIndex -= DELTANEXTU16( matchIndex );
	  }

	  return longest;
  }


#define LZ4HC_DEBUG 0
#if LZ4HC_DEBUG
  static unsigned debug = 0;
#endif

  FORCE_INLINE int LZ4HC_encodeSequence(
	  const BYTE** ip,
	  BYTE** op,
	  const BYTE** anchor,
	  int matchLength,
	  const BYTE* const match,
	  limitedOutput_directive limitedOutputBuffer,
	  BYTE* oend )
  {
	  int length;
	  BYTE* token;

#if LZ4HC_DEBUG
	  if ( debug ) printf( "literal : %u  --  match : %u  --  offset : %u\n", (U32)(*ip - *anchor), (U32)matchLength, (U32)(*ip - match) );
#endif

	  /* Encode Literal length */
	  length = (int)(*ip - *anchor);
	  token = (*op)++;
	  if ( (limitedOutputBuffer) && ((*op + (length >> 8) + length + (2 + 1 + LASTLITERALS)) > oend) ) return 1;   /* Check output limit */
	  if ( length >= (int)RUN_MASK ) { int len; *token = (RUN_MASK << ML_BITS); len = length - RUN_MASK; for ( ; len > 254; len -= 255 ) *(*op)++ = 255;  *(*op)++ = (BYTE)len; } else *token = (BYTE)(length << ML_BITS);

	  /* Copy Literals */
	  LZ4_wildCopy( *op, *anchor, (*op) + length );
	  *op += length;

	  /* Encode Offset */
	  LZ4_writeLE16( *op, (U16)(*ip - match) ); *op += 2;

	  /* Encode MatchLength */
	  length = (int)(matchLength - MINMATCH);
	  if ( (limitedOutputBuffer) && (*op + (length >> 8) + (1 + LASTLITERALS) > oend) ) return 1;   /* Check output limit */
	  if ( length >= (int)ML_MASK ) {
		  *token += ML_MASK;
		  length -= ML_MASK;
		  for ( ; length > 509; length -= 510 ) { *(*op)++ = 255; *(*op)++ = 255; }
		  if ( length > 254 ) { length -= 255; *(*op)++ = 255; }
		  *(*op)++ = (BYTE)length;
	  } else {
		  *token += (BYTE)(length);
	  }

	  /* Prepare next loop */
	  *ip += matchLength;
	  *anchor = *ip;

	  return 0;
  }


  static int LZ4HC_compress_generic(
	  void* ctxvoid,
	  const char* source,
	  char* dest,
	  int inputSize,
	  int maxOutputSize,
	  int compressionLevel,
	  limitedOutput_directive limit
  )
  {
	  LZ4HC_Data_Structure* ctx = (LZ4HC_Data_Structure*)ctxvoid;
	  const BYTE* ip = (const BYTE*)source;
	  const BYTE* anchor = ip;
	  const BYTE* const iend = ip + inputSize;
	  const BYTE* const mflimit = iend - MFLIMIT;
	  const BYTE* const matchlimit = (iend - LASTLITERALS);

	  BYTE* op = (BYTE*)dest;
	  BYTE* const oend = op + maxOutputSize;

	  unsigned maxNbAttempts;
	  int   ml, ml2, ml3, ml0;
	  const BYTE* ref = NULL;
	  const BYTE* start2 = NULL;
	  const BYTE* ref2 = NULL;
	  const BYTE* start3 = NULL;
	  const BYTE* ref3 = NULL;
	  const BYTE* start0;
	  const BYTE* ref0;

	  /* init */
	  if ( compressionLevel > LZ4HC_MAX_CLEVEL ) compressionLevel = LZ4HC_MAX_CLEVEL;
	  if ( compressionLevel < 1 ) compressionLevel = LZ4HC_DEFAULT_CLEVEL;
	  maxNbAttempts = 1 << (compressionLevel - 1);
	  ctx->end += inputSize;

	  ip++;

	  /* Main Loop */
	  while ( ip < mflimit ) {
		  ml = LZ4HC_InsertAndFindBestMatch( ctx, ip, matchlimit, (&ref), maxNbAttempts );
		  if ( !ml ) { ip++; continue; }

		  /* saved, in case we would skip too much */
		  start0 = ip;
		  ref0 = ref;
		  ml0 = ml;

	  _Search2:
		  if ( ip + ml < mflimit )
			  ml2 = LZ4HC_InsertAndGetWiderMatch( ctx, ip + ml - 2, ip + 1, matchlimit, ml, &ref2, &start2, maxNbAttempts );
		  else ml2 = ml;

		  if ( ml2 == ml ) { /* No better match */
			  if ( LZ4HC_encodeSequence( &ip, &op, &anchor, ml, ref, limit, oend ) ) return 0;
			  continue;
		  }

		  if ( start0 < ip ) {
			  if ( start2 < ip + ml0 ) {  /* empirical */
				  ip = start0;
				  ref = ref0;
				  ml = ml0;
			  }
		  }

		  /* Here, start0==ip */
		  if ( (start2 - ip) < 3 ) {  /* First Match too small : removed */
			  ml = ml2;
			  ip = start2;
			  ref = ref2;
			  goto _Search2;
		  }

	  _Search3:
		  /*
		  * Currently we have :
		  * ml2 > ml1, and
		  * ip1+3 <= ip2 (usually < ip1+ml1)
		  */
		  if ( (start2 - ip) < OPTIMAL_ML ) {
			  int correction;
			  int new_ml = ml;
			  if ( new_ml > OPTIMAL_ML ) new_ml = OPTIMAL_ML;
			  if ( ip + new_ml > start2 + ml2 - MINMATCH ) new_ml = (int)(start2 - ip) + ml2 - MINMATCH;
			  correction = new_ml - (int)(start2 - ip);
			  if ( correction > 0 ) {
				  start2 += correction;
				  ref2 += correction;
				  ml2 -= correction;
			  }
		  }
		  /* Now, we have start2 = ip+new_ml, with new_ml = min(ml, OPTIMAL_ML=18) */

		  if ( start2 + ml2 < mflimit )
			  ml3 = LZ4HC_InsertAndGetWiderMatch( ctx, start2 + ml2 - 3, start2, matchlimit, ml2, &ref3, &start3, maxNbAttempts );
		  else ml3 = ml2;

		  if ( ml3 == ml2 ) {  /* No better match : 2 sequences to encode */
							   /* ip & ref are known; Now for ml */
			  if ( start2 < ip + ml )  ml = (int)(start2 - ip);
			  /* Now, encode 2 sequences */
			  if ( LZ4HC_encodeSequence( &ip, &op, &anchor, ml, ref, limit, oend ) ) return 0;
			  ip = start2;
			  if ( LZ4HC_encodeSequence( &ip, &op, &anchor, ml2, ref2, limit, oend ) ) return 0;
			  continue;
		  }

		  if ( start3 < ip + ml + 3 ) {  /* Not enough space for match 2 : remove it */
			  if ( start3 >= (ip + ml) ) {  /* can write Seq1 immediately ==> Seq2 is removed, so Seq3 becomes Seq1 */
				  if ( start2 < ip + ml ) {
					  int correction = (int)(ip + ml - start2);
					  start2 += correction;
					  ref2 += correction;
					  ml2 -= correction;
					  if ( ml2 < MINMATCH ) {
						  start2 = start3;
						  ref2 = ref3;
						  ml2 = ml3;
					  }
				  }

				  if ( LZ4HC_encodeSequence( &ip, &op, &anchor, ml, ref, limit, oend ) ) return 0;
				  ip = start3;
				  ref = ref3;
				  ml = ml3;

				  start0 = start2;
				  ref0 = ref2;
				  ml0 = ml2;
				  goto _Search2;
			  }

			  start2 = start3;
			  ref2 = ref3;
			  ml2 = ml3;
			  goto _Search3;
		  }

		  /*
		  * OK, now we have 3 ascending matches; let's write at least the first one
		  * ip & ref are known; Now for ml
		  */
		  if ( start2 < ip + ml ) {
			  if ( (start2 - ip) < (int)ML_MASK ) {
				  int correction;
				  if ( ml > OPTIMAL_ML ) ml = OPTIMAL_ML;
				  if ( ip + ml > start2 + ml2 - MINMATCH ) ml = (int)(start2 - ip) + ml2 - MINMATCH;
				  correction = ml - (int)(start2 - ip);
				  if ( correction > 0 ) {
					  start2 += correction;
					  ref2 += correction;
					  ml2 -= correction;
				  }
			  } else {
				  ml = (int)(start2 - ip);
			  }
		  }
		  if ( LZ4HC_encodeSequence( &ip, &op, &anchor, ml, ref, limit, oend ) ) return 0;

		  ip = start2;
		  ref = ref2;
		  ml = ml2;

		  start2 = start3;
		  ref2 = ref3;
		  ml2 = ml3;

		  goto _Search3;
	  }

	  /* Encode Last Literals */
	  {   int lastRun = (int)(iend - anchor);
	  if ( (limit) && (((char*)op - dest) + lastRun + 1 + ((lastRun + 255 - RUN_MASK) / 255) > (U32)maxOutputSize) ) return 0;  /* Check output limit */
	  if ( lastRun >= (int)RUN_MASK ) { *op++ = (RUN_MASK << ML_BITS); lastRun -= RUN_MASK; for ( ; lastRun > 254; lastRun -= 255 ) *op++ = 255; *op++ = (BYTE)lastRun; } else *op++ = (BYTE)(lastRun << ML_BITS);
	  memcpy( op, anchor, iend - anchor );
	  op += iend - anchor;
	  }

	  /* End */
	  return (int)(((char*)op) - dest);
  }


  int LZ4_sizeofStateHC( void ) { return sizeof( LZ4HC_Data_Structure ); }

  int LZ4_compress_HC_extStateHC( void* state, const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel )
  {
	  if ( ((size_t)(state)&(sizeof( void* ) - 1)) != 0 ) return 0;   /* Error : state is not aligned for pointers (32 or 64 bits) */
	  LZ4HC_init( (LZ4HC_Data_Structure*)state, (const BYTE*)src );
	  if ( maxDstSize < LZ4_compressBound( srcSize ) )
		  return LZ4HC_compress_generic( state, src, dst, srcSize, maxDstSize, compressionLevel, limitedOutput );
	  else
		  return LZ4HC_compress_generic( state, src, dst, srcSize, maxDstSize, compressionLevel, notLimited );
  }

  int LZ4_compress_HC( const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel )
  {
#if LZ4HC_HEAPMODE==1
	  LZ4HC_Data_Structure* statePtr = malloc( sizeof( LZ4HC_Data_Structure ) );
#else
	  LZ4HC_Data_Structure state;
	  LZ4HC_Data_Structure* const statePtr = &state;
#endif
	  int cSize = LZ4_compress_HC_extStateHC( statePtr, src, dst, srcSize, maxDstSize, compressionLevel );
#if LZ4HC_HEAPMODE==1
	  free( statePtr );
#endif
	  return cSize;
  }



  /**************************************
  *  Streaming Functions
  **************************************/
  /* allocation */
  LZ4_streamHC_t* LZ4_createStreamHC( void ) { return (LZ4_streamHC_t*)malloc( sizeof( LZ4_streamHC_t ) ); }
  int             LZ4_freeStreamHC( LZ4_streamHC_t* LZ4_streamHCPtr ) { free( LZ4_streamHCPtr ); return 0; }


  /* initialization */
  void LZ4_resetStreamHC( LZ4_streamHC_t* LZ4_streamHCPtr, int compressionLevel )
  {
	  LZ4_STATIC_ASSERT( sizeof( LZ4HC_Data_Structure ) <= sizeof( LZ4_streamHC_t ) );   /* if compilation fails here, LZ4_STREAMHCSIZE must be increased */
	  ((LZ4HC_Data_Structure*)LZ4_streamHCPtr)->base = NULL;
	  ((LZ4HC_Data_Structure*)LZ4_streamHCPtr)->compressionLevel = (unsigned)compressionLevel;
  }

  int LZ4_loadDictHC( LZ4_streamHC_t* LZ4_streamHCPtr, const char* dictionary, int dictSize )
  {
	  LZ4HC_Data_Structure* ctxPtr = (LZ4HC_Data_Structure*)LZ4_streamHCPtr;
	  if ( dictSize > 64 KB ) {
		  dictionary += dictSize - 64 KB;
		  dictSize = 64 KB;
	  }
	  LZ4HC_init( ctxPtr, (const BYTE*)dictionary );
	  if ( dictSize >= 4 ) LZ4HC_Insert( ctxPtr, (const BYTE*)dictionary + (dictSize - 3) );
	  ctxPtr->end = (const BYTE*)dictionary + dictSize;
	  return dictSize;
  }


  /* compression */

  static void LZ4HC_setExternalDict( LZ4HC_Data_Structure* ctxPtr, const BYTE* newBlock )
  {
	  if ( ctxPtr->end >= ctxPtr->base + 4 ) LZ4HC_Insert( ctxPtr, ctxPtr->end - 3 );   /* Referencing remaining dictionary content */
																						/* Only one memory segment for extDict, so any previous extDict is lost at this stage */
	  ctxPtr->lowLimit = ctxPtr->dictLimit;
	  ctxPtr->dictLimit = (U32)(ctxPtr->end - ctxPtr->base);
	  ctxPtr->dictBase = ctxPtr->base;
	  ctxPtr->base = newBlock - ctxPtr->dictLimit;
	  ctxPtr->end = newBlock;
	  ctxPtr->nextToUpdate = ctxPtr->dictLimit;   /* match referencing will resume from there */
  }

  static int LZ4_compressHC_continue_generic( LZ4HC_Data_Structure* ctxPtr,
											  const char* source, char* dest,
											  int inputSize, int maxOutputSize, limitedOutput_directive limit )
  {
	  /* auto-init if forgotten */
	  if ( ctxPtr->base == NULL ) LZ4HC_init( ctxPtr, (const BYTE*)source );

	  /* Check overflow */
	  if ( (size_t)(ctxPtr->end - ctxPtr->base) > 2 GB ) {
		  size_t dictSize = (size_t)(ctxPtr->end - ctxPtr->base) - ctxPtr->dictLimit;
		  if ( dictSize > 64 KB ) dictSize = 64 KB;
		  LZ4_loadDictHC( (LZ4_streamHC_t*)ctxPtr, (const char*)(ctxPtr->end) - dictSize, (int)dictSize );
	  }

	  /* Check if blocks follow each other */
	  if ( (const BYTE*)source != ctxPtr->end ) LZ4HC_setExternalDict( ctxPtr, (const BYTE*)source );

	  /* Check overlapping input/dictionary space */
	  {
		  const BYTE* sourceEnd = (const BYTE*)source + inputSize;
		  const BYTE* const dictBegin = ctxPtr->dictBase + ctxPtr->lowLimit;
		  const BYTE* const dictEnd = ctxPtr->dictBase + ctxPtr->dictLimit;
		  if ( (sourceEnd > dictBegin) && ((const BYTE*)source < dictEnd) ) {
			  if ( sourceEnd > dictEnd ) sourceEnd = dictEnd;
			  ctxPtr->lowLimit = (U32)(sourceEnd - ctxPtr->dictBase);
			  if ( ctxPtr->dictLimit - ctxPtr->lowLimit < 4 ) ctxPtr->lowLimit = ctxPtr->dictLimit;
		  }
	  }

	  return LZ4HC_compress_generic( ctxPtr, source, dest, inputSize, maxOutputSize, ctxPtr->compressionLevel, limit );
  }

  int LZ4_compress_HC_continue( LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize )
  {
	  if ( maxOutputSize < LZ4_compressBound( inputSize ) )
		  return LZ4_compressHC_continue_generic( (LZ4HC_Data_Structure*)LZ4_streamHCPtr, source, dest, inputSize, maxOutputSize, limitedOutput );
	  else
		  return LZ4_compressHC_continue_generic( (LZ4HC_Data_Structure*)LZ4_streamHCPtr, source, dest, inputSize, maxOutputSize, notLimited );
  }


  /* dictionary saving */

  int LZ4_saveDictHC( LZ4_streamHC_t* LZ4_streamHCPtr, char* safeBuffer, int dictSize )
  {
	  LZ4HC_Data_Structure* const streamPtr = (LZ4HC_Data_Structure*)LZ4_streamHCPtr;
	  int const prefixSize = (int)(streamPtr->end - (streamPtr->base + streamPtr->dictLimit));
	  if ( dictSize > 64 KB ) dictSize = 64 KB;
	  if ( dictSize < 4 ) dictSize = 0;
	  if ( dictSize > prefixSize ) dictSize = prefixSize;
	  memmove( safeBuffer, streamPtr->end - dictSize, dictSize );
	  {
		  U32 const endIndex = (U32)(streamPtr->end - streamPtr->base);
		  streamPtr->end = (const BYTE*)safeBuffer + dictSize;
		  streamPtr->base = streamPtr->end - endIndex;
		  streamPtr->dictLimit = endIndex - dictSize;
		  streamPtr->lowLimit = endIndex - dictSize;
		  if ( streamPtr->nextToUpdate < streamPtr->dictLimit ) streamPtr->nextToUpdate = streamPtr->dictLimit;
	  }
	  return dictSize;
  }


  /***********************************
  *  Deprecated Functions
  ***********************************/
  /* Deprecated compression functions */
  /* These functions are planned to start generate warnings by r131 approximately */
  int LZ4_compressHC( const char* src, char* dst, int srcSize ) { return LZ4_compress_HC( src, dst, srcSize, LZ4_compressBound( srcSize ), 0 ); }
  int LZ4_compressHC_limitedOutput( const char* src, char* dst, int srcSize, int maxDstSize ) { return LZ4_compress_HC( src, dst, srcSize, maxDstSize, 0 ); }
  int LZ4_compressHC2( const char* src, char* dst, int srcSize, int cLevel ) { return LZ4_compress_HC( src, dst, srcSize, LZ4_compressBound( srcSize ), cLevel ); }
  int LZ4_compressHC2_limitedOutput( const char* src, char* dst, int srcSize, int maxDstSize, int cLevel ) { return LZ4_compress_HC( src, dst, srcSize, maxDstSize, cLevel ); }
  int LZ4_compressHC_withStateHC( void* state, const char* src, char* dst, int srcSize ) { return LZ4_compress_HC_extStateHC( state, src, dst, srcSize, LZ4_compressBound( srcSize ), 0 ); }
  int LZ4_compressHC_limitedOutput_withStateHC( void* state, const char* src, char* dst, int srcSize, int maxDstSize ) { return LZ4_compress_HC_extStateHC( state, src, dst, srcSize, maxDstSize, 0 ); }
  int LZ4_compressHC2_withStateHC( void* state, const char* src, char* dst, int srcSize, int cLevel ) { return LZ4_compress_HC_extStateHC( state, src, dst, srcSize, LZ4_compressBound( srcSize ), cLevel ); }
  int LZ4_compressHC2_limitedOutput_withStateHC( void* state, const char* src, char* dst, int srcSize, int maxDstSize, int cLevel ) { return LZ4_compress_HC_extStateHC( state, src, dst, srcSize, maxDstSize, cLevel ); }
  int LZ4_compressHC_continue( LZ4_streamHC_t* ctx, const char* src, char* dst, int srcSize ) { return LZ4_compress_HC_continue( ctx, src, dst, srcSize, LZ4_compressBound( srcSize ) ); }
  int LZ4_compressHC_limitedOutput_continue( LZ4_streamHC_t* ctx, const char* src, char* dst, int srcSize, int maxDstSize ) { return LZ4_compress_HC_continue( ctx, src, dst, srcSize, maxDstSize ); }


  /* Deprecated streaming functions */
  /* These functions currently generate deprecation warnings */
  int LZ4_sizeofStreamStateHC( void ) { return LZ4_STREAMHCSIZE; }

  int LZ4_resetStreamStateHC( void* state, char* inputBuffer )
  {
	  if ( (((size_t)state) & (sizeof( void* ) - 1)) != 0 ) return 1;   /* Error : pointer is not aligned for pointer (32 or 64 bits) */
	  LZ4HC_init( (LZ4HC_Data_Structure*)state, (const BYTE*)inputBuffer );
	  ((LZ4HC_Data_Structure*)state)->inputBuffer = (BYTE*)inputBuffer;
	  return 0;
  }

  void* LZ4_createHC( char* inputBuffer )
  {
	  void* hc4 = ALLOCATOR( 1, sizeof( LZ4HC_Data_Structure ) );
	  if ( hc4 == NULL ) return NULL;   /* not enough memory */
	  LZ4HC_init( (LZ4HC_Data_Structure*)hc4, (const BYTE*)inputBuffer );
	  ((LZ4HC_Data_Structure*)hc4)->inputBuffer = (BYTE*)inputBuffer;
	  return hc4;
  }

  int LZ4_freeHC( void* LZ4HC_Data )
  {
	  FREEMEM( LZ4HC_Data );
	  return (0);
  }

  int LZ4_compressHC2_continue( void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel )
  {
	  return LZ4HC_compress_generic( LZ4HC_Data, source, dest, inputSize, 0, compressionLevel, notLimited );
  }

  int LZ4_compressHC2_limitedOutput_continue( void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel )
  {
	  return LZ4HC_compress_generic( LZ4HC_Data, source, dest, inputSize, maxOutputSize, compressionLevel, limitedOutput );
  }

  char* LZ4_slideInputBufferHC( void* LZ4HC_Data )
  {
	  LZ4HC_Data_Structure* hc4 = (LZ4HC_Data_Structure*)LZ4HC_Data;
	  int dictSize = LZ4_saveDictHC( (LZ4_streamHC_t*)LZ4HC_Data, (char*)(hc4->inputBuffer), 64 KB );
	  return (char*)(hc4->inputBuffer + dictSize);
  }

/*
LZ4 auto-framing library
Copyright (C) 2011-2016, Yann Collet.

BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You can contact the author at :
- LZ4 homepage : http://www.lz4.org
- LZ4 source repository : https://github.com/Cyan4973/lz4
*/

/* LZ4F is a stand-alone API to create LZ4-compressed Frames
*  in full conformance with specification v1.5.0
*  All related operations, including memory management, are handled by the library.
* */

/*-************************************
*  Constants
**************************************/

#define _1BIT  0x01
#define _2BITS 0x03
#define _3BITS 0x07
#define _4BITS 0x0F
#define _8BITS 0xFF

#define LZ4F_MAGIC_SKIPPABLE_START 0x184D2A50U
#define LZ4F_MAGICNUMBER 0x184D2204U
#define LZ4F_BLOCKUNCOMPRESSED_FLAG 0x80000000U
#define LZ4F_BLOCKSIZEID_DEFAULT LZ4F_max64KB

static const size_t minFHSize = 7;
static const size_t maxFHSize = 15;
static const size_t BHSize = 4;


/*-************************************
*  Structures and local types
**************************************/
typedef struct LZ4F_cctx_s
{
    LZ4F_preferences_t prefs;
    U32    version;
    U32    cStage;
    size_t maxBlockSize;
    size_t maxBufferSize;
    BYTE*  tmpBuff;
    BYTE*  tmpIn;
    size_t tmpInSize;
    U64    totalInSize;
    XXH32_state_t xxh;
    void*  lz4CtxPtr;
    U32    lz4CtxLevel;     /* 0: unallocated;  1: LZ4_stream_t;  3: LZ4_streamHC_t */
} LZ4F_cctx_t;

typedef struct LZ4F_dctx_s
{
    LZ4F_frameInfo_t frameInfo;
    U32    version;
    U32    dStage;
    U64    frameRemainingSize;
    size_t maxBlockSize;
    size_t maxBufferSize;
    const BYTE* srcExpect;
    BYTE*  tmpIn;
    size_t tmpInSize;
    size_t tmpInTarget;
    BYTE*  tmpOutBuffer;
    const BYTE*  dict;
    size_t dictSize;
    BYTE*  tmpOut;
    size_t tmpOutSize;
    size_t tmpOutStart;
    XXH32_state_t xxh;
    BYTE   header[16];
} LZ4F_dctx_t;


/*-************************************
*  Error management
**************************************/
#define LZ4F_GENERATE_STRING(STRING) #STRING,
static const char* LZ4F_errorStrings[] = { LZ4F_LIST_ERRORS(LZ4F_GENERATE_STRING) };


unsigned LZ4F_isError(LZ4F_errorCode_t code)
{
    return (code > (LZ4F_errorCode_t)(-LZ4F_ERROR_maxCode));
}

const char* LZ4F_getErrorName(LZ4F_errorCode_t code)
{
    static const char* codeError = "Unspecified error code";
    if (LZ4F_isError(code)) return LZ4F_errorStrings[-(int)(code)];
    return codeError;
}


/*-************************************
*  Private functions
**************************************/
static size_t LZ4F_getBlockSize(unsigned blockSizeID)
{
    static const size_t blockSizes[4] = { 64 KB, 256 KB, 1 MB, 4 MB };

    if (blockSizeID == 0) blockSizeID = LZ4F_BLOCKSIZEID_DEFAULT;
    blockSizeID -= 4;
    if (blockSizeID > 3) return (size_t)-LZ4F_ERROR_maxBlockSize_invalid;
    return blockSizes[blockSizeID];
}


static BYTE LZ4F_headerChecksum (const void* header, size_t length)
{
    U32 xxh = XXH32(header, length, 0);
    return (BYTE)(xxh >> 8);
}


/*-************************************
*  Simple compression functions
**************************************/
static LZ4F_blockSizeID_t LZ4F_optimalBSID(const LZ4F_blockSizeID_t requestedBSID, const size_t srcSize)
{
    LZ4F_blockSizeID_t proposedBSID = LZ4F_max64KB;
    size_t maxBlockSize = 64 KB;
    while (requestedBSID > proposedBSID)
    {
        if (srcSize <= maxBlockSize)
            return proposedBSID;
        proposedBSID = (LZ4F_blockSizeID_t)((int)proposedBSID + 1);
        maxBlockSize <<= 2;
    }
    return requestedBSID;
}


size_t LZ4F_compressFrameBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefs;
    size_t headerSize;
    size_t streamSize;

    if (preferencesPtr!=NULL) prefs = *preferencesPtr;
    else memset(&prefs, 0, sizeof(prefs));

    prefs.frameInfo.blockSizeID = LZ4F_optimalBSID(prefs.frameInfo.blockSizeID, srcSize);
    prefs.autoFlush = 1;

    headerSize = maxFHSize;      /* header size, including magic number and frame content size*/
    streamSize = LZ4F_compressBound(srcSize, &prefs);

    return headerSize + streamSize;
}


/*! LZ4F_compressFrame() :
* Compress an entire srcBuffer into a valid LZ4 frame, as defined by specification v1.5.0, in a single step.
* The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
* You can get the minimum value of dstMaxSize by using LZ4F_compressFrameBound()
* If this condition is not respected, LZ4F_compressFrame() will fail (result is an errorCode)
* The LZ4F_preferences_t structure is optional : you can provide NULL as argument. All preferences will then be set to default.
* The result of the function is the number of bytes written into dstBuffer.
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_cctx_t cctxI;
    LZ4_stream_t lz4ctx;
    LZ4F_preferences_t prefs;
    LZ4F_compressOptions_t options;
    LZ4F_errorCode_t errorCode;
    BYTE* const dstStart = (BYTE*) dstBuffer;
    BYTE* dstPtr = dstStart;
    BYTE* const dstEnd = dstStart + dstMaxSize;

    memset(&cctxI, 0, sizeof(cctxI));   /* works because no allocation */
    memset(&options, 0, sizeof(options));

    cctxI.version = LZ4F_VERSION;
    cctxI.maxBufferSize = 5 MB;   /* mess with real buffer size to prevent allocation; works because autoflush==1 & stableSrc==1 */

    if (preferencesPtr!=NULL)
        prefs = *preferencesPtr;
    else
        memset(&prefs, 0, sizeof(prefs));
    if (prefs.frameInfo.contentSize != 0)
        prefs.frameInfo.contentSize = (U64)srcSize;   /* auto-correct content size if selected (!=0) */

	if (prefs.compressionLevel < LZ4HC_MIN_CLEVEL) {
		cctxI.lz4CtxPtr = &lz4ctx;
		cctxI.lz4CtxLevel = 1;
	}

    prefs.frameInfo.blockSizeID = LZ4F_optimalBSID(prefs.frameInfo.blockSizeID, srcSize);
    prefs.autoFlush = 1;
    if (srcSize <= LZ4F_getBlockSize(prefs.frameInfo.blockSizeID))
        prefs.frameInfo.blockMode = LZ4F_blockIndependent;   /* no need for linked blocks */

    options.stableSrc = 1;

    if (dstMaxSize < LZ4F_compressFrameBound(srcSize, &prefs))
        return (size_t)-LZ4F_ERROR_dstMaxSize_tooSmall;

    errorCode = LZ4F_compressBegin(&cctxI, dstBuffer, dstMaxSize, &prefs);  /* write header */
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;   /* header size */

    errorCode = LZ4F_compressUpdate(&cctxI, dstPtr, dstEnd-dstPtr, srcBuffer, srcSize, &options);
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    errorCode = LZ4F_compressEnd(&cctxI, dstPtr, dstEnd-dstPtr, &options);   /* flush last block, and generate suffix */
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    if (prefs.compressionLevel >= LZ4HC_MIN_CLEVEL)   /* no allocation necessary with lz4 fast */
        FREEMEM(cctxI.lz4CtxPtr);

    return (dstPtr - dstStart);
}


/*-*********************************
*  Advanced compression functions
***********************************/

/* LZ4F_createCompressionContext() :
* The first thing to do is to create a compressionContext object, which will be used in all compression operations.
* This is achieved using LZ4F_createCompressionContext(), which takes as argument a version and an LZ4F_preferences_t structure.
* The version provided MUST be LZ4F_VERSION. It is intended to track potential version differences between different binaries.
* The function will provide a pointer to an allocated LZ4F_compressionContext_t object.
* If the result LZ4F_errorCode_t is not OK_NoError, there was an error during context creation.
* Object can release its memory using LZ4F_freeCompressionContext();
*/
LZ4F_errorCode_t LZ4F_createCompressionContext(LZ4F_compressionContext_t* LZ4F_compressionContextPtr, unsigned version)
{
    LZ4F_cctx_t* cctxPtr;

    cctxPtr = (LZ4F_cctx_t*)ALLOCATORF(sizeof(LZ4F_cctx_t));
    if (cctxPtr==NULL) return (LZ4F_errorCode_t)(-LZ4F_ERROR_allocation_failed);

    cctxPtr->version = version;
    cctxPtr->cStage = 0;   /* Next stage : write header */

    *LZ4F_compressionContextPtr = (LZ4F_compressionContext_t)cctxPtr;

    return LZ4F_OK_NoError;
}


LZ4F_errorCode_t LZ4F_freeCompressionContext(LZ4F_compressionContext_t LZ4F_compressionContext)
{
    LZ4F_cctx_t* cctxPtr = (LZ4F_cctx_t*)LZ4F_compressionContext;

    if (cctxPtr != NULL) {  /* null pointers can be safely provided to this function, like free() */
       FREEMEM(cctxPtr->lz4CtxPtr);
       FREEMEM(cctxPtr->tmpBuff);
       FREEMEM(LZ4F_compressionContext);
    }

    return LZ4F_OK_NoError;
}


/*! LZ4F_compressBegin() :
* will write the frame header into dstBuffer.
* dstBuffer must be large enough to accommodate a header (dstMaxSize). Maximum header size is LZ4F_MAXHEADERFRAME_SIZE bytes.
* The result of the function is the number of bytes written into dstBuffer for the header
* or an error code (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressBegin(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefNull;
    LZ4F_cctx_t* cctxPtr = (LZ4F_cctx_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    BYTE* headerStart;
    size_t requiredBuffSize;

    if (dstMaxSize < maxFHSize) return (size_t)-LZ4F_ERROR_dstMaxSize_tooSmall;
    if (cctxPtr->cStage != 0) return (size_t)-LZ4F_ERROR_GENERIC;
    memset(&prefNull, 0, sizeof(prefNull));
    if (preferencesPtr == NULL) preferencesPtr = &prefNull;
    cctxPtr->prefs = *preferencesPtr;

    /* ctx Management */
    {   U32 const tableID = (cctxPtr->prefs.compressionLevel < LZ4HC_MIN_CLEVEL) ? 1 : 2;  /* 0:nothing ; 1:LZ4 table ; 2:HC tables */
        if (cctxPtr->lz4CtxLevel < tableID) {
            FREEMEM(cctxPtr->lz4CtxPtr);
            if (cctxPtr->prefs.compressionLevel < LZ4HC_MIN_CLEVEL)
                cctxPtr->lz4CtxPtr = (void*)LZ4_createStream();
            else
                cctxPtr->lz4CtxPtr = (void*)LZ4_createStreamHC();
            cctxPtr->lz4CtxLevel = tableID;
        }
    }

    /* Buffer Management */
    if (cctxPtr->prefs.frameInfo.blockSizeID == 0) cctxPtr->prefs.frameInfo.blockSizeID = LZ4F_BLOCKSIZEID_DEFAULT;
    cctxPtr->maxBlockSize = LZ4F_getBlockSize(cctxPtr->prefs.frameInfo.blockSizeID);

    requiredBuffSize = cctxPtr->maxBlockSize + ((cctxPtr->prefs.frameInfo.blockMode == LZ4F_blockLinked) * 128 KB);
    if (preferencesPtr->autoFlush)
        requiredBuffSize = (cctxPtr->prefs.frameInfo.blockMode == LZ4F_blockLinked) * 64 KB;   /* just needs dict */

    if (cctxPtr->maxBufferSize < requiredBuffSize) {
        cctxPtr->maxBufferSize = requiredBuffSize;
        FREEMEM(cctxPtr->tmpBuff);
        cctxPtr->tmpBuff = (BYTE*)ALLOCATORF(requiredBuffSize);
        if (cctxPtr->tmpBuff == NULL) return (size_t)-LZ4F_ERROR_allocation_failed;
    }
    cctxPtr->tmpIn = cctxPtr->tmpBuff;
    cctxPtr->tmpInSize = 0;
	XXH32_reset( &(cctxPtr->xxh), 0 );
	if ( cctxPtr->prefs.compressionLevel < LZ4HC_MIN_CLEVEL )
		LZ4_resetStream( (LZ4_stream_t*)(cctxPtr->lz4CtxPtr) );
	else
		LZ4_resetStreamHC( (LZ4_streamHC_t*)(cctxPtr->lz4CtxPtr), cctxPtr->prefs.compressionLevel );

    /* Magic Number */
    LZ4F_writeLE32(dstPtr, LZ4F_MAGICNUMBER);
    dstPtr += 4;
    headerStart = dstPtr;

    /* FLG Byte */
    *dstPtr++ = (BYTE)(((1 & _2BITS) << 6)    /* Version('01') */
        + ((cctxPtr->prefs.frameInfo.blockMode & _1BIT ) << 5)    /* Block mode */
        + ((cctxPtr->prefs.frameInfo.contentChecksumFlag & _1BIT ) << 2)   /* Frame checksum */
        + ((cctxPtr->prefs.frameInfo.contentSize > 0) << 3));   /* Frame content size */
    /* BD Byte */
    *dstPtr++ = (BYTE)((cctxPtr->prefs.frameInfo.blockSizeID & _3BITS) << 4);
    /* Optional Frame content size field */
    if (cctxPtr->prefs.frameInfo.contentSize) {
        LZ4F_writeLE64(dstPtr, cctxPtr->prefs.frameInfo.contentSize);
        dstPtr += 8;
        cctxPtr->totalInSize = 0;
    }
    /* CRC Byte */
    *dstPtr = LZ4F_headerChecksum(headerStart, dstPtr - headerStart);
    dstPtr++;

    cctxPtr->cStage = 1;   /* header written, now request input data block */

    return (dstPtr - dstStart);
}


/* LZ4F_compressBound() : gives the size of Dst buffer given a srcSize to handle worst case situations.
*                        The LZ4F_frameInfo_t structure is optional :
*                        you can provide NULL as argument, preferences will then be set to cover worst case situations.
* */
size_t LZ4F_compressBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr)
{
    LZ4F_preferences_t prefsNull;
    memset(&prefsNull, 0, sizeof(prefsNull));
    prefsNull.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;   /* worst case */
    {   const LZ4F_preferences_t* prefsPtr = (preferencesPtr==NULL) ? &prefsNull : preferencesPtr;
        LZ4F_blockSizeID_t bid = prefsPtr->frameInfo.blockSizeID;
        size_t blockSize = LZ4F_getBlockSize(bid);
        unsigned nbBlocks = (unsigned)(srcSize / blockSize) + 1;
        size_t lastBlockSize = prefsPtr->autoFlush ? srcSize % blockSize : blockSize;
        size_t blockInfo = 4;   /* default, without block CRC option */
        size_t frameEnd = 4 + (prefsPtr->frameInfo.contentChecksumFlag*4);

        return (blockInfo * nbBlocks) + (blockSize * (nbBlocks-1)) + lastBlockSize + frameEnd;;
    }
}


typedef int (*compressFunc_t)(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level);

static size_t LZ4F_compressBlock(void* dst, const void* src, size_t srcSize, compressFunc_t compress, void* lz4ctx, int level)
{
    /* compress one block */
    BYTE* cSizePtr = (BYTE*)dst;
    U32 cSize;
    cSize = (U32)compress(lz4ctx, (const char*)src, (char*)(cSizePtr+4), (int)(srcSize), (int)(srcSize-1), level);
    LZ4F_writeLE32(cSizePtr, cSize);
    if (cSize == 0) {  /* compression failed */
        cSize = (U32)srcSize;
        LZ4F_writeLE32(cSizePtr, cSize + LZ4F_BLOCKUNCOMPRESSED_FLAG);
        memcpy(cSizePtr+4, src, srcSize);
    }
    return cSize + 4;
}


static int LZ4F_localLZ4_compress_limitedOutput_withState(void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level)
{
    (void) level;
    return LZ4_compress_limitedOutput_withState(ctx, src, dst, srcSize, dstSize);
}

static int LZ4F_localLZ4_compress_limitedOutput_continue( void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level )
{
	(void)level;
	return LZ4_compress_limitedOutput_continue( (LZ4_stream_t*)ctx, src, dst, srcSize, dstSize );
}

static int LZ4F_localLZ4_compressHC_limitedOutput_continue( void* ctx, const char* src, char* dst, int srcSize, int dstSize, int level )
{
	(void)level;
	return LZ4_compress_HC_continue( (LZ4_streamHC_t*)ctx, src, dst, srcSize, dstSize );
}

static compressFunc_t LZ4F_selectCompression( LZ4F_blockMode_t blockMode, int level )
{
	if ( level < LZ4HC_MIN_CLEVEL ) {
		if ( blockMode == LZ4F_blockIndependent ) return LZ4F_localLZ4_compress_limitedOutput_withState;
		return LZ4F_localLZ4_compress_limitedOutput_continue;
	}
	if ( blockMode == LZ4F_blockIndependent ) return LZ4_compress_HC_extStateHC;
	return LZ4F_localLZ4_compressHC_limitedOutput_continue;
}

static int LZ4F_localSaveDict( LZ4F_cctx_t* cctxPtr )
{
	if ( cctxPtr->prefs.compressionLevel < LZ4HC_MIN_CLEVEL )
		return LZ4_saveDict( (LZ4_stream_t*)(cctxPtr->lz4CtxPtr), (char*)(cctxPtr->tmpBuff), 64 KB );
	return LZ4_saveDictHC( (LZ4_streamHC_t*)(cctxPtr->lz4CtxPtr), (char*)(cctxPtr->tmpBuff), 64 KB );
}

typedef enum { notDone, fromTmpBuffer, fromSrcBuffer } LZ4F_lastBlockStatus;

/*! LZ4F_compressUpdate() :
* LZ4F_compressUpdate() can be called repetitively to compress as much data as necessary.
* The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
* If this condition is not respected, LZ4F_compress() will fail (result is an errorCode)
* You can get the minimum value of dstMaxSize by using LZ4F_compressBound()
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
* The result of the function is the number of bytes written into dstBuffer : it can be zero, meaning input data was just buffered.
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
*/
size_t LZ4F_compressUpdate(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_compressOptions_t cOptionsNull;
    LZ4F_cctx_t* cctxPtr = (LZ4F_cctx_t*)compressionContext;
    size_t blockSize = cctxPtr->maxBlockSize;
    const BYTE* srcPtr = (const BYTE*)srcBuffer;
    const BYTE* const srcEnd = srcPtr + srcSize;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    LZ4F_lastBlockStatus lastBlockCompressed = notDone;
    compressFunc_t compress;


    if (cctxPtr->cStage != 1) return (size_t)-LZ4F_ERROR_GENERIC;
    if (dstMaxSize < LZ4F_compressBound(srcSize, &(cctxPtr->prefs))) return (size_t)-LZ4F_ERROR_dstMaxSize_tooSmall;
    memset(&cOptionsNull, 0, sizeof(cOptionsNull));
    if (compressOptionsPtr == NULL) compressOptionsPtr = &cOptionsNull;

    /* select compression function */
    compress = LZ4F_selectCompression(cctxPtr->prefs.frameInfo.blockMode, cctxPtr->prefs.compressionLevel);

    /* complete tmp buffer */
    if (cctxPtr->tmpInSize > 0) {   /* some data already within tmp buffer */
        size_t sizeToCopy = blockSize - cctxPtr->tmpInSize;
        if (sizeToCopy > srcSize) {
            /* add src to tmpIn buffer */
            memcpy(cctxPtr->tmpIn + cctxPtr->tmpInSize, srcBuffer, srcSize);
            srcPtr = srcEnd;
            cctxPtr->tmpInSize += srcSize;
            /* still needs some CRC */
        } else {
            /* complete tmpIn block and then compress it */
            lastBlockCompressed = fromTmpBuffer;
            memcpy(cctxPtr->tmpIn + cctxPtr->tmpInSize, srcBuffer, sizeToCopy);
            srcPtr += sizeToCopy;

            dstPtr += LZ4F_compressBlock(dstPtr, cctxPtr->tmpIn, blockSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);

            if (cctxPtr->prefs.frameInfo.blockMode==LZ4F_blockLinked) cctxPtr->tmpIn += blockSize;
            cctxPtr->tmpInSize = 0;
        }
    }

    while ((size_t)(srcEnd - srcPtr) >= blockSize) {
        /* compress full block */
        lastBlockCompressed = fromSrcBuffer;
        dstPtr += LZ4F_compressBlock(dstPtr, srcPtr, blockSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
        srcPtr += blockSize;
    }

    if ((cctxPtr->prefs.autoFlush) && (srcPtr < srcEnd)) {
        /* compress remaining input < blockSize */
        lastBlockCompressed = fromSrcBuffer;
        dstPtr += LZ4F_compressBlock(dstPtr, srcPtr, srcEnd - srcPtr, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
        srcPtr  = srcEnd;
    }

    /* preserve dictionary if necessary */
    if ((cctxPtr->prefs.frameInfo.blockMode==LZ4F_blockLinked) && (lastBlockCompressed==fromSrcBuffer)) {
        if (compressOptionsPtr->stableSrc) {
            cctxPtr->tmpIn = cctxPtr->tmpBuff;
        } else {
            int realDictSize = LZ4F_localSaveDict(cctxPtr);
            if (realDictSize==0) return (size_t)-LZ4F_ERROR_GENERIC;
            cctxPtr->tmpIn = cctxPtr->tmpBuff + realDictSize;
        }
    }

    /* keep tmpIn within limits */
    if ((cctxPtr->tmpIn + blockSize) > (cctxPtr->tmpBuff + cctxPtr->maxBufferSize)   /* necessarily LZ4F_blockLinked && lastBlockCompressed==fromTmpBuffer */
        && !(cctxPtr->prefs.autoFlush))
    {
        int realDictSize = LZ4F_localSaveDict(cctxPtr);
        cctxPtr->tmpIn = cctxPtr->tmpBuff + realDictSize;
    }

    /* some input data left, necessarily < blockSize */
    if (srcPtr < srcEnd) {
        /* fill tmp buffer */
        size_t sizeToCopy = srcEnd - srcPtr;
        memcpy(cctxPtr->tmpIn, srcPtr, sizeToCopy);
        cctxPtr->tmpInSize = sizeToCopy;
    }

    if (cctxPtr->prefs.frameInfo.contentChecksumFlag == LZ4F_contentChecksumEnabled)
        XXH32_update(&(cctxPtr->xxh), srcBuffer, srcSize);

    cctxPtr->totalInSize += srcSize;
    return dstPtr - dstStart;
}


/*! LZ4F_flush() :
* Should you need to create compressed data immediately, without waiting for a block to be filled,
* you can call LZ4_flush(), which will immediately compress any remaining data stored within compressionContext.
* The result of the function is the number of bytes written into dstBuffer
* (it can be zero, this means there was no data left within compressionContext)
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
*/
size_t LZ4F_flush(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_cctx_t* cctxPtr = (LZ4F_cctx_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    compressFunc_t compress;


    if (cctxPtr->tmpInSize == 0) return 0;   /* nothing to flush */
    if (cctxPtr->cStage != 1) return (size_t)-LZ4F_ERROR_GENERIC;
    if (dstMaxSize < (cctxPtr->tmpInSize + 8)) return (size_t)-LZ4F_ERROR_dstMaxSize_tooSmall;   /* +8 : block header(4) + block checksum(4) */
    (void)compressOptionsPtr;   /* not yet useful */

    /* select compression function */
    compress = LZ4F_selectCompression(cctxPtr->prefs.frameInfo.blockMode, cctxPtr->prefs.compressionLevel);

    /* compress tmp buffer */
    dstPtr += LZ4F_compressBlock(dstPtr, cctxPtr->tmpIn, cctxPtr->tmpInSize, compress, cctxPtr->lz4CtxPtr, cctxPtr->prefs.compressionLevel);
    if (cctxPtr->prefs.frameInfo.blockMode==LZ4F_blockLinked) cctxPtr->tmpIn += cctxPtr->tmpInSize;
    cctxPtr->tmpInSize = 0;

    /* keep tmpIn within limits */
    if ((cctxPtr->tmpIn + cctxPtr->maxBlockSize) > (cctxPtr->tmpBuff + cctxPtr->maxBufferSize)) {  /* necessarily LZ4F_blockLinked */
        int realDictSize = LZ4F_localSaveDict(cctxPtr);
        cctxPtr->tmpIn = cctxPtr->tmpBuff + realDictSize;
    }

    return dstPtr - dstStart;
}


/*! LZ4F_compressEnd() :
* When you want to properly finish the compressed frame, just call LZ4F_compressEnd().
* It will flush whatever data remained within compressionContext (like LZ4_flush())
* but also properly finalize the frame, with an endMark and a checksum.
* The result of the function is the number of bytes written into dstBuffer (necessarily >= 4 (endMark size))
* The function outputs an error code if it fails (can be tested using LZ4F_isError())
* The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
* compressionContext can then be used again, starting with LZ4F_compressBegin(). The preferences will remain the same.
*/
size_t LZ4F_compressEnd(LZ4F_compressionContext_t compressionContext, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* compressOptionsPtr)
{
    LZ4F_cctx_t* cctxPtr = (LZ4F_cctx_t*)compressionContext;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* dstPtr = dstStart;
    size_t errorCode;

    errorCode = LZ4F_flush(compressionContext, dstBuffer, dstMaxSize, compressOptionsPtr);
    if (LZ4F_isError(errorCode)) return errorCode;
    dstPtr += errorCode;

    LZ4F_writeLE32(dstPtr, 0);
    dstPtr+=4;   /* endMark */

    if (cctxPtr->prefs.frameInfo.contentChecksumFlag == LZ4F_contentChecksumEnabled) {
        U32 xxh = XXH32_digest(&(cctxPtr->xxh));
        LZ4F_writeLE32(dstPtr, xxh);
        dstPtr+=4;   /* content Checksum */
    }

    cctxPtr->cStage = 0;   /* state is now re-usable (with identical preferences) */
    cctxPtr->maxBufferSize = 0;  /* reuse HC context */

    if (cctxPtr->prefs.frameInfo.contentSize) {
        if (cctxPtr->prefs.frameInfo.contentSize != cctxPtr->totalInSize)
            return (size_t)-LZ4F_ERROR_frameSize_wrong;
    }

    return dstPtr - dstStart;
}


/*-***************************************************
*   Frame Decompression
*****************************************************/

/* Resource management */

/*! LZ4F_createDecompressionContext() :
*   Create a decompressionContext object, which will track all decompression operations.
*   Provides a pointer to a fully allocated and initialized LZ4F_decompressionContext object.
*   Object can later be released using LZ4F_freeDecompressionContext().
*   @return : if != 0, there was an error during context creation.
*/
LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_decompressionContext_t* LZ4F_decompressionContextPtr, unsigned versionNumber)
{
    LZ4F_dctx_t* const dctxPtr = (LZ4F_dctx_t*)ALLOCATORF(sizeof(LZ4F_dctx_t));
    if (dctxPtr==NULL) return (LZ4F_errorCode_t)-LZ4F_ERROR_GENERIC;

    dctxPtr->version = versionNumber;
    *LZ4F_decompressionContextPtr = (LZ4F_decompressionContext_t)dctxPtr;
    return LZ4F_OK_NoError;
}

LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_decompressionContext_t LZ4F_decompressionContext)
{
    LZ4F_errorCode_t result = LZ4F_OK_NoError;
    LZ4F_dctx_t* const dctxPtr = (LZ4F_dctx_t*)LZ4F_decompressionContext;
    if (dctxPtr != NULL) {   /* can accept NULL input, like free() */
      result = (LZ4F_errorCode_t)dctxPtr->dStage;
      FREEMEM(dctxPtr->tmpIn);
      FREEMEM(dctxPtr->tmpOutBuffer);
      FREEMEM(dctxPtr);
    }
    return result;
}


/* ******************************************************************** */
/* ********************* Decompression ******************************** */
/* ******************************************************************** */

typedef enum { dstage_getHeader=0, dstage_storeHeader,
    dstage_getCBlockSize, dstage_storeCBlockSize,
    dstage_copyDirect,
    dstage_getCBlock, dstage_storeCBlock,
    dstage_decodeCBlock, dstage_decodeCBlock_intoDst,
    dstage_decodeCBlock_intoTmp, dstage_flushOut,
    dstage_getSuffix, dstage_storeSuffix,
    dstage_getSFrameSize, dstage_storeSFrameSize,
    dstage_skipSkippable
} dStage_t;


/*! LZ4F_headerSize() :
*   @return : size of frame header
*             or an error code, which can be tested using LZ4F_isError()
*/
static size_t LZ4F_headerSize(const void* src, size_t srcSize)
{
    /* minimal srcSize to determine header size */
    if (srcSize < 5) return (size_t)-LZ4F_ERROR_frameHeader_incomplete;

    /* special case : skippable frames */
    if ((LZ4F_readLE32(src) & 0xFFFFFFF0U) == LZ4F_MAGIC_SKIPPABLE_START) return 8;

    /* control magic number */
    if (LZ4F_readLE32(src) != LZ4F_MAGICNUMBER) return (size_t)-LZ4F_ERROR_frameType_unknown;

    /* Frame Header Size */
    {   BYTE const FLG = ((const BYTE*)src)[4];
        U32 const contentSizeFlag = (FLG>>3) & _1BIT;
        return contentSizeFlag ? maxFHSize : minFHSize;
    }
}


/*! LZ4F_decodeHeader() :
   input   : `srcVoidPtr` points at the **beginning of the frame**
   output  : set internal values of dctx, such as
             dctxPtr->frameInfo and dctxPtr->dStage.
             Also allocates internal buffers.
   @return : nb Bytes read from srcVoidPtr (necessarily <= srcSize)
             or an error code (testable with LZ4F_isError())
*/
static size_t LZ4F_decodeHeader(LZ4F_dctx_t* dctxPtr, const void* srcVoidPtr, size_t srcSize)
{
    BYTE FLG, BD, HC;
    unsigned version, blockMode, blockChecksumFlag, contentSizeFlag, contentChecksumFlag, blockSizeID;
    size_t bufferNeeded;
    size_t frameHeaderSize;
    const BYTE* srcPtr = (const BYTE*)srcVoidPtr;

    /* need to decode header to get frameInfo */
    if (srcSize < minFHSize) return (size_t)-LZ4F_ERROR_frameHeader_incomplete;   /* minimal frame header size */
    memset(&(dctxPtr->frameInfo), 0, sizeof(dctxPtr->frameInfo));

    /* special case : skippable frames */
    if ((LZ4F_readLE32(srcPtr) & 0xFFFFFFF0U) == LZ4F_MAGIC_SKIPPABLE_START) {
        dctxPtr->frameInfo.frameType = LZ4F_skippableFrame;
        if (srcVoidPtr == (void*)(dctxPtr->header)) {
            dctxPtr->tmpInSize = srcSize;
            dctxPtr->tmpInTarget = 8;
            dctxPtr->dStage = dstage_storeSFrameSize;
            return srcSize;
        } else {
            dctxPtr->dStage = dstage_getSFrameSize;
            return 4;
        }
    }

    /* control magic number */
    if (LZ4F_readLE32(srcPtr) != LZ4F_MAGICNUMBER) return (size_t)-LZ4F_ERROR_frameType_unknown;
    dctxPtr->frameInfo.frameType = LZ4F_frame;

    /* Flags */
    FLG = srcPtr[4];
    version = (FLG>>6) & _2BITS;
    blockMode = (FLG>>5) & _1BIT;
    blockChecksumFlag = (FLG>>4) & _1BIT;
    contentSizeFlag = (FLG>>3) & _1BIT;
    contentChecksumFlag = (FLG>>2) & _1BIT;

    /* Frame Header Size */
    frameHeaderSize = contentSizeFlag ? maxFHSize : minFHSize;

    if (srcSize < frameHeaderSize) {
        /* not enough input to fully decode frame header */
        if (srcPtr != dctxPtr->header)
            memcpy(dctxPtr->header, srcPtr, srcSize);
        dctxPtr->tmpInSize = srcSize;
        dctxPtr->tmpInTarget = frameHeaderSize;
        dctxPtr->dStage = dstage_storeHeader;
        return srcSize;
    }

    BD = srcPtr[5];
    blockSizeID = (BD>>4) & _3BITS;

    /* validate */
    if (version != 1) return (size_t)-LZ4F_ERROR_headerVersion_wrong;        /* Version Number, only supported value */
    if (blockChecksumFlag != 0) return (size_t)-LZ4F_ERROR_blockChecksum_unsupported; /* Not supported for the time being */
    if (((FLG>>0)&_2BITS) != 0) return (size_t)-LZ4F_ERROR_reservedFlag_set; /* Reserved bits */
    if (((BD>>7)&_1BIT) != 0) return (size_t)-LZ4F_ERROR_reservedFlag_set;   /* Reserved bit */
    if (blockSizeID < 4) return (size_t)-LZ4F_ERROR_maxBlockSize_invalid;    /* 4-7 only supported values for the time being */
    if (((BD>>0)&_4BITS) != 0) return (size_t)-LZ4F_ERROR_reservedFlag_set;  /* Reserved bits */

    /* check */
    HC = LZ4F_headerChecksum(srcPtr+4, frameHeaderSize-5);
    if (HC != srcPtr[frameHeaderSize-1]) return (size_t)-LZ4F_ERROR_headerChecksum_invalid;   /* Bad header checksum error */

    /* save */
    dctxPtr->frameInfo.blockMode = (LZ4F_blockMode_t)blockMode;
    dctxPtr->frameInfo.contentChecksumFlag = (LZ4F_contentChecksum_t)contentChecksumFlag;
    dctxPtr->frameInfo.blockSizeID = (LZ4F_blockSizeID_t)blockSizeID;
    dctxPtr->maxBlockSize = LZ4F_getBlockSize(blockSizeID);
    if (contentSizeFlag)
        dctxPtr->frameRemainingSize = dctxPtr->frameInfo.contentSize = LZ4F_readLE64(srcPtr+6);

    /* init */
    if (contentChecksumFlag) XXH32_reset(&(dctxPtr->xxh), 0);

    /* alloc */
    bufferNeeded = dctxPtr->maxBlockSize + ((dctxPtr->frameInfo.blockMode==LZ4F_blockLinked) * 128 KB);
    if (bufferNeeded > dctxPtr->maxBufferSize) {   /* tmp buffers too small */
        FREEMEM(dctxPtr->tmpIn);
        FREEMEM(dctxPtr->tmpOutBuffer);
        dctxPtr->maxBufferSize = bufferNeeded;
        dctxPtr->tmpIn = (BYTE*)ALLOCATORF(dctxPtr->maxBlockSize);
        if (dctxPtr->tmpIn == NULL) return (size_t)-LZ4F_ERROR_GENERIC;
        dctxPtr->tmpOutBuffer= (BYTE*)ALLOCATORF(dctxPtr->maxBufferSize);
        if (dctxPtr->tmpOutBuffer== NULL) return (size_t)-LZ4F_ERROR_GENERIC;
    }
    dctxPtr->tmpInSize = 0;
    dctxPtr->tmpInTarget = 0;
    dctxPtr->dict = dctxPtr->tmpOutBuffer;
    dctxPtr->dictSize = 0;
    dctxPtr->tmpOut = dctxPtr->tmpOutBuffer;
    dctxPtr->tmpOutStart = 0;
    dctxPtr->tmpOutSize = 0;

    dctxPtr->dStage = dstage_getCBlockSize;

    return frameHeaderSize;
}


/*! LZ4F_getFrameInfo() :
*   Decodes frame header information, such as blockSize.
*   It is optional : you could start by calling directly LZ4F_decompress() instead.
*   The objective is to extract header information without starting decompression, typically for allocation purposes.
*   LZ4F_getFrameInfo() can also be used *after* starting decompression, on a valid LZ4F_decompressionContext_t.
*   The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
*   You are expected to resume decompression from where it stopped (srcBuffer + *srcSizePtr)
*   @return : hint of the better `srcSize` to use for next call to LZ4F_decompress,
*             or an error code which can be tested using LZ4F_isError().
*/
LZ4F_errorCode_t LZ4F_getFrameInfo(LZ4F_decompressionContext_t dCtx, LZ4F_frameInfo_t* frameInfoPtr,
                                   const void* srcBuffer, size_t* srcSizePtr)
{
    LZ4F_dctx_t* dctxPtr = (LZ4F_dctx_t*)dCtx;

    if (dctxPtr->dStage > dstage_storeHeader) {  /* note : requires dstage_* header related to be at beginning of enum */
        /* frameInfo already decoded */
        size_t o=0, i=0;
        *srcSizePtr = 0;
        *frameInfoPtr = dctxPtr->frameInfo;
        return LZ4F_decompress(dCtx, NULL, &o, NULL, &i, NULL);  /* returns : recommended nb of bytes for LZ4F_decompress() */
    } else {
        size_t nextSrcSize, o=0;
        size_t const hSize = LZ4F_headerSize(srcBuffer, *srcSizePtr);
        if (LZ4F_isError(hSize)) { *srcSizePtr=0; return hSize; }
        if (*srcSizePtr < hSize) { *srcSizePtr=0; return (size_t)-LZ4F_ERROR_frameHeader_incomplete; }

        *srcSizePtr = hSize;
        nextSrcSize = LZ4F_decompress(dCtx, NULL, &o, srcBuffer, srcSizePtr, NULL);
        if (dctxPtr->dStage <= dstage_storeHeader) return (size_t)-LZ4F_ERROR_frameHeader_incomplete; /* should not happen, already checked */
        *frameInfoPtr = dctxPtr->frameInfo;
        return nextSrcSize;
    }
}


/* trivial redirector, for common prototype */
static int LZ4F_decompress_safe (const char* source, char* dest, int compressedSize, int maxDecompressedSize, const char* dictStart, int dictSize)
{
    (void)dictStart; (void)dictSize;
    return LZ4_decompress_safe (source, dest, compressedSize, maxDecompressedSize);
}


static void LZ4F_updateDict(LZ4F_dctx_t* dctxPtr, const BYTE* dstPtr, size_t dstSize, const BYTE* dstPtr0, unsigned withinTmp)
{
    if (dctxPtr->dictSize==0)
        dctxPtr->dict = (const BYTE*)dstPtr;   /* priority to dictionary continuity */

    if (dctxPtr->dict + dctxPtr->dictSize == dstPtr) {  /* dictionary continuity */
        dctxPtr->dictSize += dstSize;
        return;
    }

    if (dstPtr - dstPtr0 + dstSize >= 64 KB) {  /* dstBuffer large enough to become dictionary */
        dctxPtr->dict = (const BYTE*)dstPtr0;
        dctxPtr->dictSize = dstPtr - dstPtr0 + dstSize;
        return;
    }

    if ((withinTmp) && (dctxPtr->dict == dctxPtr->tmpOutBuffer)) {
        /* assumption : dctxPtr->dict + dctxPtr->dictSize == dctxPtr->tmpOut + dctxPtr->tmpOutStart */
        dctxPtr->dictSize += dstSize;
        return;
    }

    if (withinTmp) { /* copy relevant dict portion in front of tmpOut within tmpOutBuffer */
        size_t preserveSize = dctxPtr->tmpOut - dctxPtr->tmpOutBuffer;
        size_t copySize = 64 KB - dctxPtr->tmpOutSize;
        const BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize - dctxPtr->tmpOutStart;
        if (dctxPtr->tmpOutSize > 64 KB) copySize = 0;
        if (copySize > preserveSize) copySize = preserveSize;

        memcpy(dctxPtr->tmpOutBuffer + preserveSize - copySize, oldDictEnd - copySize, copySize);

        dctxPtr->dict = dctxPtr->tmpOutBuffer;
        dctxPtr->dictSize = preserveSize + dctxPtr->tmpOutStart + dstSize;
        return;
    }

    if (dctxPtr->dict == dctxPtr->tmpOutBuffer) {    /* copy dst into tmp to complete dict */
        if (dctxPtr->dictSize + dstSize > dctxPtr->maxBufferSize) {  /* tmp buffer not large enough */
            size_t preserveSize = 64 KB - dstSize;   /* note : dstSize < 64 KB */
            memcpy(dctxPtr->tmpOutBuffer, dctxPtr->dict + dctxPtr->dictSize - preserveSize, preserveSize);
            dctxPtr->dictSize = preserveSize;
        }
        memcpy(dctxPtr->tmpOutBuffer + dctxPtr->dictSize, dstPtr, dstSize);
        dctxPtr->dictSize += dstSize;
        return;
    }

    /* join dict & dest into tmp */
    {   size_t preserveSize = 64 KB - dstSize;   /* note : dstSize < 64 KB */
        if (preserveSize > dctxPtr->dictSize) preserveSize = dctxPtr->dictSize;
        memcpy(dctxPtr->tmpOutBuffer, dctxPtr->dict + dctxPtr->dictSize - preserveSize, preserveSize);
        memcpy(dctxPtr->tmpOutBuffer + preserveSize, dstPtr, dstSize);
        dctxPtr->dict = dctxPtr->tmpOutBuffer;
        dctxPtr->dictSize = preserveSize + dstSize;
    }
}



/*! LZ4F_decompress() :
* Call this function repetitively to regenerate data compressed within srcBuffer.
* The function will attempt to decode *srcSizePtr from srcBuffer, into dstBuffer of maximum size *dstSizePtr.
*
* The number of bytes regenerated into dstBuffer will be provided within *dstSizePtr (necessarily <= original value).
*
* The number of bytes effectively read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
* If the number of bytes read is < number of bytes provided, then the decompression operation is not complete.
* You will have to call it again, continuing from where it stopped.
*
* The function result is an hint of the better srcSize to use for next call to LZ4F_decompress.
* Basically, it's the size of the current (or remaining) compressed block + header of next block.
* Respecting the hint provides some boost to performance, since it allows less buffer shuffling.
* Note that this is just a hint, you can always provide any srcSize you want.
* When a frame is fully decoded, the function result will be 0.
* If decompression failed, function result is an error code which can be tested using LZ4F_isError().
*/
size_t LZ4F_decompress(LZ4F_decompressionContext_t decompressionContext,
                       void* dstBuffer, size_t* dstSizePtr,
                       const void* srcBuffer, size_t* srcSizePtr,
                       const LZ4F_decompressOptions_t* decompressOptionsPtr)
{
    LZ4F_dctx_t* dctxPtr = (LZ4F_dctx_t*)decompressionContext;
    LZ4F_decompressOptions_t optionsNull;
    const BYTE* const srcStart = (const BYTE*)srcBuffer;
    const BYTE* const srcEnd = srcStart + *srcSizePtr;
    const BYTE* srcPtr = srcStart;
    BYTE* const dstStart = (BYTE*)dstBuffer;
    BYTE* const dstEnd = dstStart + *dstSizePtr;
    BYTE* dstPtr = dstStart;
    const BYTE* selectedIn = NULL;
    unsigned doAnotherStage = 1;
    size_t nextSrcSizeHint = 1;


    memset(&optionsNull, 0, sizeof(optionsNull));
    if (decompressOptionsPtr==NULL) decompressOptionsPtr = &optionsNull;
    *srcSizePtr = 0;
    *dstSizePtr = 0;

    /* expect to continue decoding src buffer where it left previously */
    if (dctxPtr->srcExpect != NULL) {
        if (srcStart != dctxPtr->srcExpect) return (size_t)-LZ4F_ERROR_srcPtr_wrong;
    }

    /* programmed as a state machine */

    while (doAnotherStage) {

        switch(dctxPtr->dStage)
        {

        case dstage_getHeader:
            if ((size_t)(srcEnd-srcPtr) >= maxFHSize) {  /* enough to decode - shortcut */
                LZ4F_errorCode_t const hSize = LZ4F_decodeHeader(dctxPtr, srcPtr, srcEnd-srcPtr);
                if (LZ4F_isError(hSize)) return hSize;
                srcPtr += hSize;
                break;
            }
            dctxPtr->tmpInSize = 0;
            dctxPtr->tmpInTarget = minFHSize;   /* minimum to attempt decode */
            dctxPtr->dStage = dstage_storeHeader;
            /* pass-through */

        case dstage_storeHeader:
            {   size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy =  srcEnd - srcPtr;
                memcpy(dctxPtr->header + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                dctxPtr->tmpInSize += sizeToCopy;
                srcPtr += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget) {
                    nextSrcSizeHint = (dctxPtr->tmpInTarget - dctxPtr->tmpInSize) + BHSize;   /* rest of header + nextBlockHeader */
                    doAnotherStage = 0;   /* not enough src data, ask for some more */
                    break;
                }
                {   LZ4F_errorCode_t const hSize = LZ4F_decodeHeader(dctxPtr, dctxPtr->header, dctxPtr->tmpInTarget);
                    if (LZ4F_isError(hSize)) return hSize;
                }
                break;
            }

        case dstage_getCBlockSize:
            if ((size_t)(srcEnd - srcPtr) >= BHSize) {
                selectedIn = srcPtr;
                srcPtr += BHSize;
            } else {
                /* not enough input to read cBlockSize field */
                dctxPtr->tmpInSize = 0;
                dctxPtr->dStage = dstage_storeCBlockSize;
            }

            if (dctxPtr->dStage == dstage_storeCBlockSize)   /* can be skipped */
        case dstage_storeCBlockSize:
            {
                size_t sizeToCopy = BHSize - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < BHSize) {   /* not enough input to get full cBlockSize; wait for more */
                    nextSrcSizeHint = BHSize - dctxPtr->tmpInSize;
                    doAnotherStage  = 0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
            }

        /* case dstage_decodeCBlockSize: */   /* no more direct access, to prevent scan-build warning */
            {   size_t const nextCBlockSize = LZ4F_readLE32(selectedIn) & 0x7FFFFFFFU;
                if (nextCBlockSize==0) {  /* frameEnd signal, no more CBlock */
                    dctxPtr->dStage = dstage_getSuffix;
                    break;
                }
                if (nextCBlockSize > dctxPtr->maxBlockSize) return (size_t)-LZ4F_ERROR_GENERIC;   /* invalid cBlockSize */
                dctxPtr->tmpInTarget = nextCBlockSize;
                if (LZ4F_readLE32(selectedIn) & LZ4F_BLOCKUNCOMPRESSED_FLAG) {
                    dctxPtr->dStage = dstage_copyDirect;
                    break;
                }
                dctxPtr->dStage = dstage_getCBlock;
                if (dstPtr==dstEnd) {
                    nextSrcSizeHint = nextCBlockSize + BHSize;
                    doAnotherStage = 0;
                }
                break;
            }

        case dstage_copyDirect:   /* uncompressed block */
            {   size_t sizeToCopy = dctxPtr->tmpInTarget;
                if ((size_t)(srcEnd-srcPtr) < sizeToCopy) sizeToCopy = srcEnd - srcPtr;  /* not enough input to read full block */
                if ((size_t)(dstEnd-dstPtr) < sizeToCopy) sizeToCopy = dstEnd - dstPtr;
                memcpy(dstPtr, srcPtr, sizeToCopy);
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), srcPtr, sizeToCopy);
                if (dctxPtr->frameInfo.contentSize) dctxPtr->frameRemainingSize -= sizeToCopy;

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==LZ4F_blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, sizeToCopy, dstStart, 0);

                srcPtr += sizeToCopy;
                dstPtr += sizeToCopy;
                if (sizeToCopy == dctxPtr->tmpInTarget) {  /* all copied */
                    dctxPtr->dStage = dstage_getCBlockSize;
                    break;
                }
                dctxPtr->tmpInTarget -= sizeToCopy;   /* still need to copy more */
                nextSrcSizeHint = dctxPtr->tmpInTarget + BHSize;
                doAnotherStage = 0;
                break;
            }

        case dstage_getCBlock:   /* entry from dstage_decodeCBlockSize */
            if ((size_t)(srcEnd-srcPtr) < dctxPtr->tmpInTarget) {
                dctxPtr->tmpInSize = 0;
                dctxPtr->dStage = dstage_storeCBlock;
                break;
            }
            selectedIn = srcPtr;
            srcPtr += dctxPtr->tmpInTarget;
            dctxPtr->dStage = dstage_decodeCBlock;
            break;

        case dstage_storeCBlock:
            {   size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd-srcPtr)) sizeToCopy = srcEnd-srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                dctxPtr->tmpInSize += sizeToCopy;
                srcPtr += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget) { /* need more input */
                    nextSrcSizeHint = (dctxPtr->tmpInTarget - dctxPtr->tmpInSize) + BHSize;
                    doAnotherStage=0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
                dctxPtr->dStage = dstage_decodeCBlock;
                /* pass-through */
            }

        case dstage_decodeCBlock:
            if ((size_t)(dstEnd-dstPtr) < dctxPtr->maxBlockSize)   /* not enough place into dst : decode into tmpOut */
                dctxPtr->dStage = dstage_decodeCBlock_intoTmp;
            else
                dctxPtr->dStage = dstage_decodeCBlock_intoDst;
            break;

        case dstage_decodeCBlock_intoDst:
            {   int (*decoder)(const char*, char*, int, int, const char*, int);
                int decodedSize;

                if (dctxPtr->frameInfo.blockMode == LZ4F_blockLinked)
                    decoder = LZ4_decompress_safe_usingDict;
                else
                    decoder = LZ4F_decompress_safe;

                decodedSize = decoder((const char*)selectedIn, (char*)dstPtr, (int)dctxPtr->tmpInTarget, (int)dctxPtr->maxBlockSize, (const char*)dctxPtr->dict, (int)dctxPtr->dictSize);
                if (decodedSize < 0) return (size_t)-LZ4F_ERROR_GENERIC;   /* decompression failed */
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), dstPtr, decodedSize);
                if (dctxPtr->frameInfo.contentSize) dctxPtr->frameRemainingSize -= decodedSize;

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==LZ4F_blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, decodedSize, dstStart, 0);

                dstPtr += decodedSize;
                dctxPtr->dStage = dstage_getCBlockSize;
                break;
            }

        case dstage_decodeCBlock_intoTmp:
            /* not enough place into dst : decode into tmpOut */
            {   int (*decoder)(const char*, char*, int, int, const char*, int);
                int decodedSize;

                if (dctxPtr->frameInfo.blockMode == LZ4F_blockLinked)
                    decoder = LZ4_decompress_safe_usingDict;
                else
                    decoder = LZ4F_decompress_safe;

                /* ensure enough place for tmpOut */
                if (dctxPtr->frameInfo.blockMode == LZ4F_blockLinked) {
                    if (dctxPtr->dict == dctxPtr->tmpOutBuffer) {
                        if (dctxPtr->dictSize > 128 KB) {
                            memcpy(dctxPtr->tmpOutBuffer, dctxPtr->dict + dctxPtr->dictSize - 64 KB, 64 KB);
                            dctxPtr->dictSize = 64 KB;
                        }
                        dctxPtr->tmpOut = dctxPtr->tmpOutBuffer + dctxPtr->dictSize;
                    } else {  /* dict not within tmp */
                        size_t reservedDictSpace = dctxPtr->dictSize;
                        if (reservedDictSpace > 64 KB) reservedDictSpace = 64 KB;
                        dctxPtr->tmpOut = dctxPtr->tmpOutBuffer + reservedDictSpace;
                    }
                }

                /* Decode */
                decodedSize = decoder((const char*)selectedIn, (char*)dctxPtr->tmpOut, (int)dctxPtr->tmpInTarget, (int)dctxPtr->maxBlockSize, (const char*)dctxPtr->dict, (int)dctxPtr->dictSize);
                if (decodedSize < 0) return (size_t)-LZ4F_ERROR_decompressionFailed;   /* decompression failed */
                if (dctxPtr->frameInfo.contentChecksumFlag) XXH32_update(&(dctxPtr->xxh), dctxPtr->tmpOut, decodedSize);
                if (dctxPtr->frameInfo.contentSize) dctxPtr->frameRemainingSize -= decodedSize;
                dctxPtr->tmpOutSize = decodedSize;
                dctxPtr->tmpOutStart = 0;
                dctxPtr->dStage = dstage_flushOut;
                break;
            }

        case dstage_flushOut:  /* flush decoded data from tmpOut to dstBuffer */
            {   size_t sizeToCopy = dctxPtr->tmpOutSize - dctxPtr->tmpOutStart;
                if (sizeToCopy > (size_t)(dstEnd-dstPtr)) sizeToCopy = dstEnd-dstPtr;
                memcpy(dstPtr, dctxPtr->tmpOut + dctxPtr->tmpOutStart, sizeToCopy);

                /* dictionary management */
                if (dctxPtr->frameInfo.blockMode==LZ4F_blockLinked)
                    LZ4F_updateDict(dctxPtr, dstPtr, sizeToCopy, dstStart, 1);

                dctxPtr->tmpOutStart += sizeToCopy;
                dstPtr += sizeToCopy;

                /* end of flush ? */
                if (dctxPtr->tmpOutStart == dctxPtr->tmpOutSize) {
                    dctxPtr->dStage = dstage_getCBlockSize;
                    break;
                }
                nextSrcSizeHint = BHSize;
                doAnotherStage = 0;   /* still some data to flush */
                break;
            }

        case dstage_getSuffix:
            {   size_t const suffixSize = dctxPtr->frameInfo.contentChecksumFlag * 4;
                if (dctxPtr->frameRemainingSize) return (size_t)-LZ4F_ERROR_frameSize_wrong;   /* incorrect frame size decoded */
                if (suffixSize == 0) {  /* frame completed */
                    nextSrcSizeHint = 0;
                    dctxPtr->dStage = dstage_getHeader;
                    doAnotherStage = 0;
                    break;
                }
                if ((srcEnd - srcPtr) < 4) {  /* not enough size for entire CRC */
                    dctxPtr->tmpInSize = 0;
                    dctxPtr->dStage = dstage_storeSuffix;
                } else {
                    selectedIn = srcPtr;
                    srcPtr += 4;
                }
            }

            if (dctxPtr->dStage == dstage_storeSuffix)   /* can be skipped */
        case dstage_storeSuffix:
            {
                size_t sizeToCopy = 4 - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->tmpIn + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < 4) { /* not enough input to read complete suffix */
                    nextSrcSizeHint = 4 - dctxPtr->tmpInSize;
                    doAnotherStage=0;
                    break;
                }
                selectedIn = dctxPtr->tmpIn;
            }

        /* case dstage_checkSuffix: */   /* no direct call, to avoid scan-build warning */
            {   U32 const readCRC = LZ4F_readLE32(selectedIn);
                U32 const resultCRC = XXH32_digest(&(dctxPtr->xxh));
                if (readCRC != resultCRC) return (size_t)-LZ4F_ERROR_contentChecksum_invalid;
                nextSrcSizeHint = 0;
                dctxPtr->dStage = dstage_getHeader;
                doAnotherStage = 0;
                break;
            }

        case dstage_getSFrameSize:
            if ((srcEnd - srcPtr) >= 4) {
                selectedIn = srcPtr;
                srcPtr += 4;
            } else {
                /* not enough input to read cBlockSize field */
                dctxPtr->tmpInSize = 4;
                dctxPtr->tmpInTarget = 8;
                dctxPtr->dStage = dstage_storeSFrameSize;
            }

            if (dctxPtr->dStage == dstage_storeSFrameSize)
        case dstage_storeSFrameSize:
            {
                size_t sizeToCopy = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                if (sizeToCopy > (size_t)(srcEnd - srcPtr)) sizeToCopy = srcEnd - srcPtr;
                memcpy(dctxPtr->header + dctxPtr->tmpInSize, srcPtr, sizeToCopy);
                srcPtr += sizeToCopy;
                dctxPtr->tmpInSize += sizeToCopy;
                if (dctxPtr->tmpInSize < dctxPtr->tmpInTarget) { /* not enough input to get full sBlockSize; wait for more */
                    nextSrcSizeHint = dctxPtr->tmpInTarget - dctxPtr->tmpInSize;
                    doAnotherStage = 0;
                    break;
                }
                selectedIn = dctxPtr->header + 4;
            }

        /* case dstage_decodeSFrameSize: */   /* no direct access */
            {   size_t const SFrameSize = LZ4F_readLE32(selectedIn);
                dctxPtr->frameInfo.contentSize = SFrameSize;
                dctxPtr->tmpInTarget = SFrameSize;
                dctxPtr->dStage = dstage_skipSkippable;
                break;
            }

        case dstage_skipSkippable:
            {   size_t skipSize = dctxPtr->tmpInTarget;
                if (skipSize > (size_t)(srcEnd-srcPtr)) skipSize = srcEnd-srcPtr;
                srcPtr += skipSize;
                dctxPtr->tmpInTarget -= skipSize;
                doAnotherStage = 0;
                nextSrcSizeHint = dctxPtr->tmpInTarget;
                if (nextSrcSizeHint) break;
                dctxPtr->dStage = dstage_getHeader;
                break;
            }
        }
    }

    /* preserve dictionary within tmp if necessary */
    if ( (dctxPtr->frameInfo.blockMode==LZ4F_blockLinked)
        &&(dctxPtr->dict != dctxPtr->tmpOutBuffer)
        &&(!decompressOptionsPtr->stableDst)
        &&((unsigned)(dctxPtr->dStage-1) < (unsigned)(dstage_getSuffix-1))
        )
    {
        if (dctxPtr->dStage == dstage_flushOut) {
            size_t preserveSize = dctxPtr->tmpOut - dctxPtr->tmpOutBuffer;
            size_t copySize = 64 KB - dctxPtr->tmpOutSize;
            const BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize - dctxPtr->tmpOutStart;
            if (dctxPtr->tmpOutSize > 64 KB) copySize = 0;
            if (copySize > preserveSize) copySize = preserveSize;

            memcpy(dctxPtr->tmpOutBuffer + preserveSize - copySize, oldDictEnd - copySize, copySize);

            dctxPtr->dict = dctxPtr->tmpOutBuffer;
            dctxPtr->dictSize = preserveSize + dctxPtr->tmpOutStart;
        } else {
            size_t newDictSize = dctxPtr->dictSize;
            const BYTE* oldDictEnd = dctxPtr->dict + dctxPtr->dictSize;
            if ((newDictSize) > 64 KB) newDictSize = 64 KB;

            memcpy(dctxPtr->tmpOutBuffer, oldDictEnd - newDictSize, newDictSize);

            dctxPtr->dict = dctxPtr->tmpOutBuffer;
            dctxPtr->dictSize = newDictSize;
            dctxPtr->tmpOut = dctxPtr->tmpOutBuffer + newDictSize;
        }
    }

    /* require function to be called again from position where it stopped */
    if (srcPtr<srcEnd)
        dctxPtr->srcExpect = srcPtr;
    else
        dctxPtr->srcExpect = NULL;

    *srcSizePtr = (srcPtr - srcStart);
    *dstSizePtr = (dstPtr - dstStart);
    return nextSrcSizeHint;
}
