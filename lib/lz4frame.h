/*
   LZ4 - Fast LZ compression algorithm
   Header File
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

#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

/*-************************************
*  Includes
**************************************/
#include <stddef.h>   /* size_t */

/*-***************************************************************
*  Export parameters
*****************************************************************/
/*!
*  LZ4_DLL_EXPORT :
*  Enable exporting of functions when building a Windows DLL
*/
#ifndef LZ4_STATIC
#if defined(_WIN32)
#  if defined(LZ4_DLL_EXPORT) && (LZ4_DLL_EXPORT==1)
#    define LZ4FLIB_API __declspec(dllexport)
#  else
#    define LZ4FLIB_API __declspec(dllimport)
#  endif
#endif
#else
#  define LZ4FLIB_API
#endif



/**************************************
* Error management
* ************************************/
#define LZ4F_LIST_ERRORS(ITEM) \
        ITEM(OK_NoError) ITEM(ERROR_GENERIC) \
        ITEM(ERROR_maxBlockSize_invalid) ITEM(ERROR_blockMode_invalid) ITEM(ERROR_contentChecksumFlag_invalid) \
        ITEM(ERROR_compressionLevel_invalid) \
        ITEM(ERROR_headerVersion_wrong) ITEM(ERROR_blockChecksum_unsupported) ITEM(ERROR_reservedFlag_set) \
        ITEM(ERROR_allocation_failed) \
        ITEM(ERROR_srcSize_tooLarge) ITEM(ERROR_dstMaxSize_tooSmall) \
        ITEM(ERROR_frameHeader_incomplete) ITEM(ERROR_frameType_unknown) ITEM(ERROR_frameSize_wrong) \
        ITEM(ERROR_srcPtr_wrong) \
        ITEM(ERROR_decompressionFailed) \
        ITEM(ERROR_headerChecksum_invalid) ITEM(ERROR_contentChecksum_invalid) \
        ITEM(ERROR_maxCode)

//#define LZ4F_DISABLE_OLD_ENUMS
#ifndef LZ4F_DISABLE_OLD_ENUMS
#define LZ4F_GENERATE_ENUM(ENUM) LZ4F_##ENUM, ENUM = LZ4F_##ENUM,
#else
#define LZ4F_GENERATE_ENUM(ENUM) LZ4F_##ENUM,
#endif
	typedef enum { LZ4F_LIST_ERRORS( LZ4F_GENERATE_ENUM ) } LZ4F_errorCodes;  /* enum is exposed, to handle specific errors; compare function result to -enum value */



/*-************************************
*  Version
**************************************/
#define LZ4_VERSION_MAJOR    1    /* for breaking interface changes  */
#define LZ4_VERSION_MINOR    7    /* for new (non-breaking) interface capabilities */
#define LZ4_VERSION_RELEASE  2    /* for tweaks, bug-fixes, or development */

#define LZ4_VERSION_NUMBER (LZ4_VERSION_MAJOR *100*100 + LZ4_VERSION_MINOR *100 + LZ4_VERSION_RELEASE)
	LZ4FLIB_API int LZ4_versionNumber( void );

#define LZ4_LIB_VERSION LZ4_VERSION_MAJOR.LZ4_VERSION_MINOR.LZ4_VERSION_RELEASE
#define LZ4_QUOTE(str) #str
#define LZ4_EXPAND_AND_QUOTE(str) LZ4_QUOTE(str)
#define LZ4_VERSION_STRING LZ4_EXPAND_AND_QUOTE(LZ4_LIB_VERSION)
	LZ4FLIB_API const char* LZ4_versionString( void );


	/*-************************************
	*  Tuning parameter
	**************************************/
	/*
	* LZ4_MEMORY_USAGE :
	* Memory usage formula : N->2^N Bytes (examples : 10 -> 1KB; 12 -> 4KB ; 16 -> 64KB; 20 -> 1MB; etc.)
	* Increasing memory usage improves compression ratio
	* Reduced memory usage can improve speed, due to cache effect
	* Default value is 14, for 16KB, which nicely fits into Intel x86 L1 cache
	*/
#define LZ4_MEMORY_USAGE 14


	/*-************************************
	*  Simple Functions
	**************************************/

	LZ4FLIB_API int LZ4_compress_default( const char* source, char* dest, int sourceSize, int maxDestSize );
	LZ4FLIB_API int LZ4_decompress_safe( const char* source, char* dest, int compressedSize, int maxDecompressedSize );

	/*
	LZ4_compress_default() :
	Compresses 'sourceSize' bytes from buffer 'source'
	into already allocated 'dest' buffer of size 'maxDestSize'.
	Compression is guaranteed to succeed if 'maxDestSize' >= LZ4_compressBound(sourceSize).
	It also runs faster, so it's a recommended setting.
	If the function cannot compress 'source' into a more limited 'dest' budget,
	compression stops *immediately*, and the function result is zero.
	As a consequence, 'dest' content is not valid.
	This function never writes outside 'dest' buffer, nor read outside 'source' buffer.
	sourceSize  : Max supported value is LZ4_MAX_INPUT_VALUE
	maxDestSize : full or partial size of buffer 'dest' (which must be already allocated)
	return : the number of bytes written into buffer 'dest' (necessarily <= maxOutputSize)
	or 0 if compression fails

	LZ4_decompress_safe() :
	compressedSize : is the precise full size of the compressed block.
	maxDecompressedSize : is the size of destination buffer, which must be already allocated.
	return : the number of bytes decompressed into destination buffer (necessarily <= maxDecompressedSize)
	If destination buffer is not large enough, decoding will stop and output an error code (<0).
	If the source stream is detected malformed, the function will stop decoding and return a negative result.
	This function is protected against buffer overflow exploits, including malicious data packets.
	It never writes outside output buffer, nor reads outside input buffer.
	*/


	/*-************************************
	*  Advanced Functions
	**************************************/
#define LZ4_MAX_INPUT_SIZE        0x7E000000   /* 2 113 929 216 bytes */
#define LZ4_COMPRESSBOUND(isize)  ((unsigned)(isize) > (unsigned)LZ4_MAX_INPUT_SIZE ? 0 : (isize) + ((isize)/255) + 16)

	/*!
	LZ4_compressBound() :
	Provides the maximum size that LZ4 compression may output in a "worst case" scenario (input data not compressible)
	This function is primarily useful for memory allocation purposes (destination buffer size).
	Macro LZ4_COMPRESSBOUND() is also provided for compilation-time evaluation (stack memory allocation for example).
	Note that LZ4_compress_default() compress faster when dest buffer size is >= LZ4_compressBound(srcSize)
	inputSize  : max supported value is LZ4_MAX_INPUT_SIZE
	return : maximum output size in a "worst case" scenario
	or 0, if input size is too large ( > LZ4_MAX_INPUT_SIZE)
	*/
	LZ4FLIB_API int LZ4_compressBound( int inputSize );

	/*!
	LZ4_compress_fast() :
	Same as LZ4_compress_default(), but allows to select an "acceleration" factor.
	The larger the acceleration value, the faster the algorithm, but also the lesser the compression.
	It's a trade-off. It can be fine tuned, with each successive value providing roughly +~3% to speed.
	An acceleration value of "1" is the same as regular LZ4_compress_default()
	Values <= 0 will be replaced by ACCELERATION_DEFAULT (see lz4.c), which is 1.
	*/
	LZ4FLIB_API int LZ4_compress_fast( const char* source, char* dest, int sourceSize, int maxDestSize, int acceleration );


	/*!
	LZ4_compress_fast_extState() :
	Same compression function, just using an externally allocated memory space to store compression state.
	Use LZ4_sizeofState() to know how much memory must be allocated,
	and allocate it on 8-bytes boundaries (using malloc() typically).
	Then, provide it as 'void* state' to compression function.
	*/
	LZ4FLIB_API int LZ4_sizeofState( void );
	LZ4FLIB_API int LZ4_compress_fast_extState( void* state, const char* source, char* dest, int inputSize, int maxDestSize, int acceleration );


	/*!
	LZ4_compress_destSize() :
	Reverse the logic, by compressing as much data as possible from 'source' buffer
	into already allocated buffer 'dest' of size 'targetDestSize'.
	This function either compresses the entire 'source' content into 'dest' if it's large enough,
	or fill 'dest' buffer completely with as much data as possible from 'source'.
	*sourceSizePtr : will be modified to indicate how many bytes where read from 'source' to fill 'dest'.
	New value is necessarily <= old value.
	return : Nb bytes written into 'dest' (necessarily <= targetDestSize)
	or 0 if compression fails
	*/
	LZ4FLIB_API int LZ4_compress_destSize( const char* source, char* dest, int* sourceSizePtr, int targetDestSize );


	/*!
	LZ4_decompress_fast() :
	originalSize : is the original and therefore uncompressed size
	return : the number of bytes read from the source buffer (in other words, the compressed size)
	If the source stream is detected malformed, the function will stop decoding and return a negative result.
	Destination buffer must be already allocated. Its size must be a minimum of 'originalSize' bytes.
	note : This function fully respect memory boundaries for properly formed compressed data.
	It is a bit faster than LZ4_decompress_safe().
	However, it does not provide any protection against intentionally modified data stream (malicious input).
	Use this function in trusted environment only (data to decode comes from a trusted source).
	*/
	LZ4FLIB_API int LZ4_decompress_fast( const char* source, char* dest, int originalSize );

	/*!
	LZ4_decompress_safe_partial() :
	This function decompress a compressed block of size 'compressedSize' at position 'source'
	into destination buffer 'dest' of size 'maxDecompressedSize'.
	The function tries to stop decompressing operation as soon as 'targetOutputSize' has been reached,
	reducing decompression time.
	return : the number of bytes decoded in the destination buffer (necessarily <= maxDecompressedSize)
	Note : this number can be < 'targetOutputSize' should the compressed block to decode be smaller.
	Always control how many bytes were decoded.
	If the source stream is detected malformed, the function will stop decoding and return a negative result.
	This function never writes outside of output buffer, and never reads outside of input buffer. It is therefore protected against malicious data packets
	*/
	LZ4FLIB_API int LZ4_decompress_safe_partial( const char* source, char* dest, int compressedSize, int targetOutputSize, int maxDecompressedSize );


	/*-*********************************************
	*  Streaming Compression Functions
	***********************************************/
#define LZ4_STREAMSIZE_U64 ((1 << (LZ4_MEMORY_USAGE-3)) + 4)
#define LZ4_STREAMSIZE     (LZ4_STREAMSIZE_U64 * sizeof(long long))
	/*
	* LZ4_stream_t :
	* information structure to track an LZ4 stream.
	* important : init this structure content before first use !
	* note : only allocated directly the structure if you are statically linking LZ4
	*        If you are using liblz4 as a DLL, please use below construction methods instead.
	*/
	typedef struct { long long table[LZ4_STREAMSIZE_U64]; } LZ4_stream_t;

	/*! LZ4_resetStream() :
	*  Use this function to init an allocated `LZ4_stream_t` structure
	*/
	LZ4FLIB_API void LZ4_resetStream( LZ4_stream_t* streamPtr );

	/*! LZ4_createStream() will allocate and initialize an `LZ4_stream_t` structure.
	*  LZ4_freeStream() releases its memory.
	*  In the context of a DLL (liblz4), please use these methods rather than the static struct.
	*  They are more future proof, in case of a change of `LZ4_stream_t` size.
	*/
	LZ4FLIB_API LZ4_stream_t* LZ4_createStream( void );
	LZ4FLIB_API int           LZ4_freeStream( LZ4_stream_t* streamPtr );

	/*! LZ4_loadDict() :
	*  Use this function to load a static dictionary into LZ4_stream.
	*  Any previous data will be forgotten, only 'dictionary' will remain in memory.
	*  Loading a size of 0 is allowed.
	*  Return : dictionary size, in bytes (necessarily <= 64 KB)
	*/
	LZ4FLIB_API int LZ4_loadDict( LZ4_stream_t* streamPtr, const char* dictionary, int dictSize );

	/*! LZ4_compress_fast_continue() :
	*  Compress buffer content 'src', using data from previously compressed blocks as dictionary to improve compression ratio.
	*  Important : Previous data blocks are assumed to still be present and unmodified !
	*  'dst' buffer must be already allocated.
	*  If maxDstSize >= LZ4_compressBound(srcSize), compression is guaranteed to succeed, and runs faster.
	*  If not, and if compressed data cannot fit into 'dst' buffer size, compression stops, and function returns a zero.
	*/
	LZ4FLIB_API int LZ4_compress_fast_continue( LZ4_stream_t* streamPtr, const char* src, char* dst, int srcSize, int maxDstSize, int acceleration );

	/*! LZ4_saveDict() :
	*  If previously compressed data block is not guaranteed to remain available at its memory location,
	*  save it into a safer place (char* safeBuffer).
	*  Note : you don't need to call LZ4_loadDict() afterwards,
	*         dictionary is immediately usable, you can therefore call LZ4_compress_fast_continue().
	*  Return : saved dictionary size in bytes (necessarily <= dictSize), or 0 if error.
	*/
	LZ4FLIB_API int LZ4_saveDict( LZ4_stream_t* streamPtr, char* safeBuffer, int dictSize );


	/*-**********************************************
	*  Streaming Decompression Functions
	************************************************/

#define LZ4_STREAMDECODESIZE_U64  4
#define LZ4_STREAMDECODESIZE     (LZ4_STREAMDECODESIZE_U64 * sizeof(unsigned long long))
	typedef struct { unsigned long long table[LZ4_STREAMDECODESIZE_U64]; } LZ4_streamDecode_t;
	/*
	* LZ4_streamDecode_t
	* information structure to track an LZ4 stream.
	* init this structure content using LZ4_setStreamDecode or memset() before first use !
	*
	* In the context of a DLL (liblz4) please prefer usage of construction methods below.
	* They are more future proof, in case of a change of LZ4_streamDecode_t size in the future.
	* LZ4_createStreamDecode will allocate and initialize an LZ4_streamDecode_t structure
	* LZ4_freeStreamDecode releases its memory.
	*/
	LZ4FLIB_API LZ4_streamDecode_t* LZ4_createStreamDecode( void );
	LZ4FLIB_API int                 LZ4_freeStreamDecode( LZ4_streamDecode_t* LZ4_stream );

	/*! LZ4_setStreamDecode() :
	*  Use this function to instruct where to find the dictionary.
	*  Setting a size of 0 is allowed (same effect as reset).
	*  @return : 1 if OK, 0 if error
	*/
	LZ4FLIB_API int LZ4_setStreamDecode( LZ4_streamDecode_t* LZ4_streamDecode, const char* dictionary, int dictSize );

	/*
	*_continue() :
	These decoding functions allow decompression of multiple blocks in "streaming" mode.
	Previously decoded blocks *must* remain available at the memory position where they were decoded (up to 64 KB)
	In the case of a ring buffers, decoding buffer must be either :
	- Exactly same size as encoding buffer, with same update rule (block boundaries at same positions)
	In which case, the decoding & encoding ring buffer can have any size, including very small ones ( < 64 KB).
	- Larger than encoding buffer, by a minimum of maxBlockSize more bytes.
	maxBlockSize is implementation dependent. It's the maximum size you intend to compress into a single block.
	In which case, encoding and decoding buffers do not need to be synchronized,
	and encoding ring buffer can have any size, including small ones ( < 64 KB).
	- _At least_ 64 KB + 8 bytes + maxBlockSize.
	In which case, encoding and decoding buffers do not need to be synchronized,
	and encoding ring buffer can have any size, including larger than decoding buffer.
	Whenever these conditions are not possible, save the last 64KB of decoded data into a safe buffer,
	and indicate where it is saved using LZ4_setStreamDecode()
	*/
	LZ4FLIB_API int LZ4_decompress_safe_continue( LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int compressedSize, int maxDecompressedSize );
	LZ4FLIB_API int LZ4_decompress_fast_continue( LZ4_streamDecode_t* LZ4_streamDecode, const char* source, char* dest, int originalSize );


	/*
	Advanced decoding functions :
	*_usingDict() :
	These decoding functions work the same as
	a combination of LZ4_setStreamDecode() followed by LZ4_decompress_x_continue()
	They are stand-alone. They don't need nor update an LZ4_streamDecode_t structure.
	*/
	LZ4FLIB_API int LZ4_decompress_safe_usingDict( const char* source, char* dest, int compressedSize, int maxDecompressedSize, const char* dictStart, int dictSize );
	LZ4FLIB_API int LZ4_decompress_fast_usingDict( const char* source, char* dest, int originalSize, const char* dictStart, int dictSize );


	/*=************************************
	*  Obsolete Functions
	**************************************/
	/* Deprecate Warnings */
	/* Should these warnings messages be a problem,
	it is generally possible to disable them,
	with -Wno-deprecated-declarations for gcc
	or _CRT_SECURE_NO_WARNINGS in Visual for example.
	Otherwise, you can also define LZ4_DISABLE_DEPRECATE_WARNINGS */
#define LZ4_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#ifdef LZ4_DISABLE_DEPRECATE_WARNINGS
#  define LZ4_DEPRECATED()   /* disable deprecation warnings */
#else
#  if (LZ4_GCC_VERSION >= 405) || defined(__clang__)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated(message)))
#  elif (LZ4_GCC_VERSION >= 301)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define LZ4_DEPRECATED(message) __declspec(deprecated(message))
#  else
#    pragma message("WARNING: You need to implement LZ4_DEPRECATED for this compiler")
#    define LZ4_DEPRECATED(message)
#  endif
#endif /* LZ4_DISABLE_DEPRECATE_WARNINGS */

	/* Obsolete compression functions */
	/* These functions will generate warnings in a future release */
	LZ4FLIB_API int LZ4_compress( const char* source, char* dest, int sourceSize );
	LZ4FLIB_API int LZ4_compress_limitedOutput( const char* source, char* dest, int sourceSize, int maxOutputSize );
	LZ4FLIB_API int LZ4_compress_withState( void* state, const char* source, char* dest, int inputSize );
	LZ4FLIB_API int LZ4_compress_limitedOutput_withState( void* state, const char* source, char* dest, int inputSize, int maxOutputSize );
	LZ4FLIB_API int LZ4_compress_continue( LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize );
	LZ4FLIB_API int LZ4_compress_limitedOutput_continue( LZ4_stream_t* LZ4_streamPtr, const char* source, char* dest, int inputSize, int maxOutputSize );

	/* Obsolete decompression functions */
	/* These function names are completely deprecated and must no longer be used.
	They are only provided in lz4.c for compatibility with older programs.
	- LZ4_uncompress is the same as LZ4_decompress_fast
	- LZ4_uncompress_unknownOutputSize is the same as LZ4_decompress_safe
	These function prototypes are now disabled; uncomment them only if you really need them.
	It is highly recommended to stop using these prototypes and migrate to maintained ones */
	/* int LZ4_uncompress (const char* source, char* dest, int outputSize); */
	/* int LZ4_uncompress_unknownOutputSize (const char* source, char* dest, int isize, int maxOutputSize); */

	/* Obsolete streaming functions; use new streaming interface whenever possible */
	LZ4_DEPRECATED( "use LZ4_createStream() instead" ) void* LZ4_create( char* inputBuffer );
	LZ4_DEPRECATED( "use LZ4_createStream() instead" ) int   LZ4_sizeofStreamState( void );
	LZ4_DEPRECATED( "use LZ4_resetStream() instead" )  int   LZ4_resetStreamState( void* state, char* inputBuffer );
	LZ4_DEPRECATED( "use LZ4_saveDict() instead" )     char* LZ4_slideInputBuffer( void* state );

	/* Obsolete streaming decoding functions */
	LZ4_DEPRECATED( "use LZ4_decompress_safe_usingDict() instead" ) int LZ4_decompress_safe_withPrefix64k( const char* src, char* dst, int compressedSize, int maxDstSize );
	LZ4_DEPRECATED( "use LZ4_decompress_fast_usingDict() instead" ) int LZ4_decompress_fast_withPrefix64k( const char* src, char* dst, int originalSize );

/*
   LZ4 HC - High Compression Mode of LZ4
   Header File
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

#define LZ4HC_MIN_CLEVEL        3
#define LZ4HC_DEFAULT_CLEVEL    9
#define LZ4HC_MAX_CLEVEL        16

	/*-************************************
	*  Block Compression
	**************************************/
	/*!
	LZ4_compress_HC() :
	Destination buffer `dst` must be already allocated.
	Compression success is guaranteed if `dst` buffer is sized to handle worst circumstances (data not compressible)
	Worst size evaluation is provided by function LZ4_compressBound() (see "lz4.h")
	`srcSize`  : Max supported value is LZ4_MAX_INPUT_SIZE (see "lz4.h")
	`compressionLevel` : Recommended values are between 4 and 9, although any value between 0 and LZ4HC_MAX_CLEVEL will work.
	0 means "use default value" (see lz4hc.c).
	Values >LZ4HC_MAX_CLEVEL behave the same as 16.
	@return : the number of bytes written into buffer 'dst'
	or 0 if compression fails.
	*/
	LZ4FLIB_API int LZ4_compress_HC( const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel );


	/* Note :
	Decompression functions are provided within "lz4.h" (BSD license)
	*/


	/*!
	LZ4_compress_HC_extStateHC() :
	Use this function if you prefer to manually allocate memory for compression tables.
	To know how much memory must be allocated for the compression tables, use :
	int LZ4_sizeofStateHC();

	Allocated memory must be aligned on 8-bytes boundaries (which a normal malloc() will do properly).

	The allocated memory can then be provided to the compression functions using 'void* state' parameter.
	LZ4_compress_HC_extStateHC() is equivalent to previously described function.
	It just uses externally allocated memory for stateHC.
	*/
	LZ4FLIB_API int LZ4_compress_HC_extStateHC( void* state, const char* src, char* dst, int srcSize, int maxDstSize, int compressionLevel );
	LZ4FLIB_API int LZ4_sizeofStateHC( void );


	/*-************************************
	*  Streaming Compression
	**************************************/
#define LZ4_STREAMHCSIZE        262192
#define LZ4_STREAMHCSIZE_SIZET (LZ4_STREAMHCSIZE / sizeof(size_t))
	typedef struct { size_t table[LZ4_STREAMHCSIZE_SIZET]; } LZ4_streamHC_t;
	/*
	LZ4_streamHC_t
	This structure allows static allocation of LZ4 HC streaming state.
	State must then be initialized using LZ4_resetStreamHC() before first use.

	Static allocation should only be used in combination with static linking.
	If you want to use LZ4 as a DLL, please use construction functions below, which are future-proof.
	*/


	/*! LZ4_createStreamHC() and LZ4_freeStreamHC() :
	These functions create and release memory for LZ4 HC streaming state.
	Newly created states are already initialized.
	Existing state space can be re-used anytime using LZ4_resetStreamHC().
	If you use LZ4 as a DLL, use these functions instead of static structure allocation,
	to avoid size mismatch between different versions.
	*/
	LZ4FLIB_API LZ4_streamHC_t* LZ4_createStreamHC( void );
	LZ4FLIB_API int             LZ4_freeStreamHC( LZ4_streamHC_t* streamHCPtr );

	LZ4FLIB_API void LZ4_resetStreamHC( LZ4_streamHC_t* streamHCPtr, int compressionLevel );
	LZ4FLIB_API int  LZ4_loadDictHC( LZ4_streamHC_t* streamHCPtr, const char* dictionary, int dictSize );

	LZ4FLIB_API int LZ4_compress_HC_continue( LZ4_streamHC_t* streamHCPtr, const char* src, char* dst, int srcSize, int maxDstSize );

	LZ4FLIB_API int LZ4_saveDictHC( LZ4_streamHC_t* streamHCPtr, char* safeBuffer, int maxDictSize );

	/*
	These functions compress data in successive blocks of any size, using previous blocks as dictionary.
	One key assumption is that previous blocks (up to 64 KB) remain read-accessible while compressing next blocks.
	There is an exception for ring buffers, which can be smaller than 64 KB.
	Such case is automatically detected and correctly handled by LZ4_compress_HC_continue().

	Before starting compression, state must be properly initialized, using LZ4_resetStreamHC().
	A first "fictional block" can then be designated as initial dictionary, using LZ4_loadDictHC() (Optional).

	Then, use LZ4_compress_HC_continue() to compress each successive block.
	It works like LZ4_compress_HC(), but use previous memory blocks as dictionary to improve compression.
	Previous memory blocks (including initial dictionary when present) must remain accessible and unmodified during compression.
	As a reminder, size 'dst' buffer to handle worst cases, using LZ4_compressBound(), to ensure success of compression operation.

	If, for any reason, previous data blocks can't be preserved unmodified in memory during next compression block,
	you must save it to a safer memory space, using LZ4_saveDictHC().
	Return value of LZ4_saveDictHC() is the size of dictionary effectively saved into 'safeBuffer'.
	*/



	/*-************************************
	*  Deprecated Functions
	**************************************/
	/* Deprecate Warnings */
	/* Should these warnings messages be a problem,
	it is generally possible to disable them,
	with -Wno-deprecated-declarations for gcc
	or _CRT_SECURE_NO_WARNINGS in Visual for example.
	You can also define LZ4_DEPRECATE_WARNING_DEFBLOCK. */
#ifndef LZ4_DEPRECATE_WARNING_DEFBLOCK
#  define LZ4_DEPRECATE_WARNING_DEFBLOCK
#  define LZ4_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#  if (LZ4_GCC_VERSION >= 405) || defined(__clang__)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated(message)))
#  elif (LZ4_GCC_VERSION >= 301)
#    define LZ4_DEPRECATED(message) __attribute__((deprecated))
#  elif defined(_MSC_VER)
#    define LZ4_DEPRECATED(message) __declspec(deprecated(message))
#  else
#    pragma message("WARNING: You need to implement LZ4_DEPRECATED for this compiler")
#    define LZ4_DEPRECATED(message)
#  endif
#endif /* LZ4_DEPRECATE_WARNING_DEFBLOCK */

	/* deprecated compression functions */
	/* these functions will trigger warning messages in future releases */
	LZ4FLIB_API int LZ4_compressHC( const char* source, char* dest, int inputSize );
	LZ4FLIB_API int LZ4_compressHC_limitedOutput( const char* source, char* dest, int inputSize, int maxOutputSize );
	LZ4_DEPRECATED( "use LZ4_compress_HC() instead" ) int LZ4_compressHC2( const char* source, char* dest, int inputSize, int compressionLevel );
	LZ4_DEPRECATED( "use LZ4_compress_HC() instead" ) int LZ4_compressHC2_limitedOutput( const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel );
	LZ4FLIB_API int LZ4_compressHC_withStateHC( void* state, const char* source, char* dest, int inputSize );
	LZ4FLIB_API int LZ4_compressHC_limitedOutput_withStateHC( void* state, const char* source, char* dest, int inputSize, int maxOutputSize );
	LZ4_DEPRECATED( "use LZ4_compress_HC_extStateHC() instead" ) int LZ4_compressHC2_withStateHC( void* state, const char* source, char* dest, int inputSize, int compressionLevel );
	LZ4_DEPRECATED( "use LZ4_compress_HC_extStateHC() instead" ) int LZ4_compressHC2_limitedOutput_withStateHC( void* state, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel );
	LZ4FLIB_API int LZ4_compressHC_continue( LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize );
	LZ4FLIB_API int LZ4_compressHC_limitedOutput_continue( LZ4_streamHC_t* LZ4_streamHCPtr, const char* source, char* dest, int inputSize, int maxOutputSize );

	/* Deprecated Streaming functions using older model; should no longer be used */
	LZ4_DEPRECATED( "use LZ4_createStreamHC() instead" ) void* LZ4_createHC( char* inputBuffer );
	LZ4_DEPRECATED( "use LZ4_saveDictHC() instead" )     char* LZ4_slideInputBufferHC( void* LZ4HC_Data );
	LZ4_DEPRECATED( "use LZ4_freeStreamHC() instead" )   int   LZ4_freeHC( void* LZ4HC_Data );
	LZ4_DEPRECATED( "use LZ4_compress_HC_continue() instead" ) int   LZ4_compressHC2_continue( void* LZ4HC_Data, const char* source, char* dest, int inputSize, int compressionLevel );
	LZ4_DEPRECATED( "use LZ4_compress_HC_continue() instead" ) int   LZ4_compressHC2_limitedOutput_continue( void* LZ4HC_Data, const char* source, char* dest, int inputSize, int maxOutputSize, int compressionLevel );
	LZ4_DEPRECATED( "use LZ4_createStreamHC() instead" ) int   LZ4_sizeofStreamStateHC( void );
	LZ4_DEPRECATED( "use LZ4_resetStreamHC() instead" )  int   LZ4_resetStreamStateHC( void* state, char* inputBuffer );


/*
   LZ4 auto-framing library
   Header File
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

/* LZ4F is a stand-alone API to create LZ4-compressed frames
 * conformant with specification v1.5.1.
 * All related operations, including memory management, are handled internally by the library.
 * You don't need lz4.h when using lz4frame.h.
 * */
/*-************************************
*  Error management
**************************************/
typedef size_t LZ4F_errorCode_t;

LZ4FLIB_API unsigned    LZ4F_isError(LZ4F_errorCode_t code);
LZ4FLIB_API const char* LZ4F_getErrorName(LZ4F_errorCode_t code);   /* return error code string; useful for debugging */


/*-************************************
*  Frame compression types
**************************************/
//#define LZ4F_DISABLE_OBSOLETE_ENUMS
#ifndef LZ4F_DISABLE_OBSOLETE_ENUMS
#  define LZ4F_OBSOLETE_ENUM(x) ,x
#else
#  define LZ4F_OBSOLETE_ENUM(x)
#endif

typedef enum {
    LZ4F_default=0,
    LZ4F_max64KB=4,
    LZ4F_max256KB=5,
    LZ4F_max1MB=6,
    LZ4F_max4MB=7
    LZ4F_OBSOLETE_ENUM(max64KB = LZ4F_max64KB)
    LZ4F_OBSOLETE_ENUM(max256KB = LZ4F_max256KB)
    LZ4F_OBSOLETE_ENUM(max1MB = LZ4F_max1MB)
    LZ4F_OBSOLETE_ENUM(max4MB = LZ4F_max4MB)
} LZ4F_blockSizeID_t;

typedef enum {
    LZ4F_blockLinked=0,
    LZ4F_blockIndependent
    LZ4F_OBSOLETE_ENUM(blockLinked = LZ4F_blockLinked)
    LZ4F_OBSOLETE_ENUM(blockIndependent = LZ4F_blockIndependent)
} LZ4F_blockMode_t;

typedef enum {
    LZ4F_noContentChecksum=0,
    LZ4F_contentChecksumEnabled
    LZ4F_OBSOLETE_ENUM(noContentChecksum = LZ4F_noContentChecksum)
    LZ4F_OBSOLETE_ENUM(contentChecksumEnabled = LZ4F_contentChecksumEnabled)
} LZ4F_contentChecksum_t;

typedef enum {
    LZ4F_frame=0,
    LZ4F_skippableFrame
    LZ4F_OBSOLETE_ENUM(skippableFrame = LZ4F_skippableFrame)
} LZ4F_frameType_t;

#ifndef LZ4F_DISABLE_OBSOLETE_ENUMS
typedef LZ4F_blockSizeID_t blockSizeID_t;
typedef LZ4F_blockMode_t blockMode_t;
typedef LZ4F_frameType_t frameType_t;
typedef LZ4F_contentChecksum_t contentChecksum_t;
#endif

typedef struct {
  LZ4F_blockSizeID_t     blockSizeID;           /* max64KB, max256KB, max1MB, max4MB ; 0 == default */
  LZ4F_blockMode_t       blockMode;             /* blockLinked, blockIndependent ; 0 == default */
  LZ4F_contentChecksum_t contentChecksumFlag;   /* noContentChecksum, contentChecksumEnabled ; 0 == default  */
  LZ4F_frameType_t       frameType;             /* LZ4F_frame, skippableFrame ; 0 == default */
  unsigned long long     contentSize;           /* Size of uncompressed (original) content ; 0 == unknown */
  unsigned               reserved[2];           /* must be zero for forward compatibility */
} LZ4F_frameInfo_t;

typedef struct {
  LZ4F_frameInfo_t frameInfo;
  int      compressionLevel;       /* 0 == default (fast mode); values above 16 count as 16; values below 0 count as 0 */
  unsigned autoFlush;              /* 1 == always flush (reduce need for tmp buffer) */
  unsigned reserved[4];            /* must be zero for forward compatibility */
} LZ4F_preferences_t;


/*-*********************************
*  Simple compression function
***********************************/
LZ4FLIB_API size_t LZ4F_compressFrameBound(size_t srcSize, const LZ4F_preferences_t* preferencesPtr);

/*!LZ4F_compressFrame() :
 * Compress an entire srcBuffer into a valid LZ4 frame, as defined by specification v1.5.1
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * You can get the minimum value of dstMaxSize by using LZ4F_compressFrameBound()
 * If this condition is not respected, LZ4F_compressFrame() will fail (result is an errorCode)
 * The LZ4F_preferences_t structure is optional : you can provide NULL as argument. All preferences will be set to default.
 * The result of the function is the number of bytes written into dstBuffer.
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */
LZ4FLIB_API size_t LZ4F_compressFrame(void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_preferences_t* preferencesPtr);



/*-***********************************
*  Advanced compression functions
*************************************/
typedef struct LZ4F_cctx_s* LZ4F_compressionContext_t;   /* must be aligned on 8-bytes */

typedef struct {
  unsigned stableSrc;    /* 1 == src content will remain available on future calls to LZ4F_compress(); avoid saving src content within tmp buffer as future dictionary */
  unsigned reserved[3];
} LZ4F_compressOptions_t;

/* Resource Management */

#define LZ4F_VERSION 100
LZ4FLIB_API LZ4F_errorCode_t LZ4F_createCompressionContext(LZ4F_compressionContext_t* cctxPtr, unsigned version);
LZ4FLIB_API LZ4F_errorCode_t LZ4F_freeCompressionContext(LZ4F_compressionContext_t cctx);
/* LZ4F_createCompressionContext() :
 * The first thing to do is to create a compressionContext object, which will be used in all compression operations.
 * This is achieved using LZ4F_createCompressionContext(), which takes as argument a version and an LZ4F_preferences_t structure.
 * The version provided MUST be LZ4F_VERSION. It is intended to track potential version differences between different binaries.
 * The function will provide a pointer to a fully allocated LZ4F_compressionContext_t object.
 * If the result LZ4F_errorCode_t is not zero, there was an error during context creation.
 * Object can release its memory using LZ4F_freeCompressionContext();
 */


/* Compression */

LZ4FLIB_API size_t LZ4F_compressBegin(LZ4F_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LZ4F_preferences_t* prefsPtr);
/* LZ4F_compressBegin() :
 * will write the frame header into dstBuffer.
 * dstBuffer must be large enough to accommodate a header (dstMaxSize). Maximum header size is 15 bytes.
 * The LZ4F_preferences_t structure is optional : you can provide NULL as argument, all preferences will then be set to default.
 * The result of the function is the number of bytes written into dstBuffer for the header
 * or an error code (can be tested using LZ4F_isError())
 */

LZ4FLIB_API size_t LZ4F_compressBound(size_t srcSize, const LZ4F_preferences_t* prefsPtr);
/* LZ4F_compressBound() :
 * Provides the minimum size of Dst buffer given srcSize to handle worst case situations.
 * Different preferences can produce different results.
 * prefsPtr is optional : you can provide NULL as argument, all preferences will then be set to cover worst case.
 * This function includes frame termination cost (4 bytes, or 8 if frame checksum is enabled)
 */

LZ4FLIB_API size_t LZ4F_compressUpdate(LZ4F_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const void* srcBuffer, size_t srcSize, const LZ4F_compressOptions_t* cOptPtr);
/* LZ4F_compressUpdate()
 * LZ4F_compressUpdate() can be called repetitively to compress as much data as necessary.
 * The most important rule is that dstBuffer MUST be large enough (dstMaxSize) to ensure compression completion even in worst case.
 * You can get the minimum value of dstMaxSize by using LZ4F_compressBound().
 * If this condition is not respected, LZ4F_compress() will fail (result is an errorCode).
 * LZ4F_compressUpdate() doesn't guarantee error recovery, so you have to reset compression context when an error occurs.
 * The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer : it can be zero, meaning input data was just buffered.
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */

LZ4FLIB_API size_t LZ4F_flush(LZ4F_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* cOptPtr);
/* LZ4F_flush()
 * Should you need to generate compressed data immediately, without waiting for the current block to be filled,
 * you can call LZ4_flush(), which will immediately compress any remaining data buffered within cctx.
 * Note that dstMaxSize must be large enough to ensure the operation will be successful.
 * LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * The result of the function is the number of bytes written into dstBuffer
 * (it can be zero, this means there was no data left within cctx)
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 */

LZ4FLIB_API size_t LZ4F_compressEnd(LZ4F_compressionContext_t cctx, void* dstBuffer, size_t dstMaxSize, const LZ4F_compressOptions_t* cOptPtr);
/* LZ4F_compressEnd()
 * When you want to properly finish the compressed frame, just call LZ4F_compressEnd().
 * It will flush whatever data remained within compressionContext (like LZ4_flush())
 * but also properly finalize the frame, with an endMark and a checksum.
 * The result of the function is the number of bytes written into dstBuffer (necessarily >= 4 (endMark), or 8 if optional frame checksum is enabled)
 * The function outputs an error code if it fails (can be tested using LZ4F_isError())
 * The LZ4F_compressOptions_t structure is optional : you can provide NULL as argument.
 * A successful call to LZ4F_compressEnd() makes cctx available again for next compression task.
 */


/*-*********************************
*  Decompression functions
***********************************/

typedef struct LZ4F_dctx_s* LZ4F_decompressionContext_t;   /* must be aligned on 8-bytes */

typedef struct {
  unsigned stableDst;       /* guarantee that decompressed data will still be there on next function calls (avoid storage into tmp buffers) */
  unsigned reserved[3];
} LZ4F_decompressOptions_t;


/* Resource management */

/*!LZ4F_createDecompressionContext() :
 * Create an LZ4F_decompressionContext_t object, which will be used to track all decompression operations.
 * The version provided MUST be LZ4F_VERSION. It is intended to track potential breaking differences between different versions.
 * The function will provide a pointer to a fully allocated and initialized LZ4F_decompressionContext_t object.
 * The result is an errorCode, which can be tested using LZ4F_isError().
 * dctx memory can be released using LZ4F_freeDecompressionContext();
 * The result of LZ4F_freeDecompressionContext() is indicative of the current state of decompressionContext when being released.
 * That is, it should be == 0 if decompression has been completed fully and correctly.
 */
LZ4FLIB_API LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_decompressionContext_t* dctxPtr, unsigned version);
LZ4FLIB_API LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_decompressionContext_t dctx);


/*======   Decompression   ======*/

/*!LZ4F_getFrameInfo() :
 * This function decodes frame header information (such as max blockSize, frame checksum, etc.).
 * Its usage is optional. The objective is to extract frame header information, typically for allocation purposes.
 * A header size is variable and can be from 7 to 15 bytes. It's also possible to input more bytes than that.
 * The number of bytes read from srcBuffer will be updated within *srcSizePtr (necessarily <= original value).
 * (note that LZ4F_getFrameInfo() can also be used anytime *after* starting decompression, in this case 0 input byte is enough)
 * Frame header info is *copied into* an already allocated LZ4F_frameInfo_t structure.
 * The function result is an hint about how many srcSize bytes LZ4F_decompress() expects for next call,
 *                        or an error code which can be tested using LZ4F_isError()
 *                        (typically, when there is not enough src bytes to fully decode the frame header)
 * Decompression is expected to resume from where it stopped (srcBuffer + *srcSizePtr)
 */
LZ4FLIB_API size_t LZ4F_getFrameInfo(LZ4F_decompressionContext_t dctx,
                                     LZ4F_frameInfo_t* frameInfoPtr,
                                     const void* srcBuffer, size_t* srcSizePtr);

/*!LZ4F_decompress() :
 * Call this function repetitively to regenerate data compressed within srcBuffer.
 * The function will attempt to decode *srcSizePtr bytes from srcBuffer, into dstBuffer of maximum size *dstSizePtr.
 *
 * The number of bytes regenerated into dstBuffer will be provided within *dstSizePtr (necessarily <= original value).
 *
 * The number of bytes read from srcBuffer will be provided within *srcSizePtr (necessarily <= original value).
 * If number of bytes read is < number of bytes provided, then decompression operation is not completed.
 * It typically happens when dstBuffer is not large enough to contain all decoded data.
 * LZ4F_decompress() must be called again, starting from where it stopped (srcBuffer + *srcSizePtr)
 * The function will check this condition, and refuse to continue if it is not respected.
 *
 * `dstBuffer` is expected to be flushed between each call to the function, its content will be overwritten.
 * `dst` arguments can be changed at will at each consecutive call to the function.
 *
 * The function result is an hint of how many `srcSize` bytes LZ4F_decompress() expects for next call.
 * Schematically, it's the size of the current (or remaining) compressed block + header of next block.
 * Respecting the hint provides some boost to performance, since it does skip intermediate buffers.
 * This is just a hint though, it's always possible to provide any srcSize.
 * When a frame is fully decoded, the function result will be 0 (no more data expected).
 * If decompression failed, function result is an error code, which can be tested using LZ4F_isError().
 *
 * After a frame is fully decoded, dctx can be used again to decompress another frame.
 */
LZ4FLIB_API size_t LZ4F_decompress(LZ4F_decompressionContext_t dctx,
                                   void* dstBuffer, size_t* dstSizePtr,
                                   const void* srcBuffer, size_t* srcSizePtr,
                                   const LZ4F_decompressOptions_t* dOptPtr);



#if defined (__cplusplus)
}
#endif
