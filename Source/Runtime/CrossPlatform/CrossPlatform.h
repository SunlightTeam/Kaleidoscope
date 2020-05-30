#pragma once


/*****************************************************************************************
* Initial platform/compiler-related stuff to set.
*****************************************************************************************/
/**
* Platform definition
*/
#define PLATFORM_WINDOWS   1
#define PLATFORM_MAC       2
#define PLATFORM_LINUX     3
#define PLATFORM_CYGWIN    4
#define PLATFORM_XBOX360   5
#define PLATFORM_PS3       6
#define PLATFORM_WIIU      7
#define PLATFORM_IOS       8
#define PLATFORM_ANDROID   9
#define PLATFORM_3DS       10
#define PLATFORM_PSVITA    11
#define PLATFORM_CHROME    12
#define PLATFORM_WII       13

/**
* CPU
*/
#define CPU_X86  1
#define CPU_PPC  2
#define CPU_ARM  3
#define CPU_X64  4


/**
* Compiler definition
*/
#define COMPILER_MSVC 1   // Microsoft visual c++ 
#define COMPILER_GNUC 2   // GNU c++
#define COMPILER_CW   3   // CodeWarrior


/**
* Endian definition
*/
#define ENDIAN_LITTLE 1
#define ENDIAN_BIG    2


/**
* Architecture definition
*/
#define ARCHITECTURE_32 1
#define ARCHITECTURE_64 2



/*****************************************************************************************
* Finds the compiler type and version.
*****************************************************************************************/
#if defined( _MSC_VER )
#   define COMPILER COMPILER_MSVC
#   define COMP_VER _MSC_VER

#elif defined( __GNUC__ )
#   define COMPILER COMPILER_GNUC
#   define COMP_VER (((__GNUC__)*100) + \
        (__GNUC_MINOR__*10) + \
        __GNUC_PATCHLEVEL__)

#elif defined( __MWERKS__ )
#   define COMPILER COMPILER_CW
#   define COMP_VER 0

#else
#error "Unknown compiler. Abort! Abort!"
#endif



/*****************************************************************************************
* Finds the current platform
*****************************************************************************************/
#if defined( __WIN32__ ) || defined( _WIN32 )
#   define PLATFORM PLATFORM_WINDOWS
#   define PLAT_VERSION WINVER  

#elif defined( __APPLE_CC__ ) || defined( APPLE_CC )
#   define PLATFORM PLATFORM_MAC
#   define PLAT_VERSION 0

#elif defined( __LINUX__ ) || defined( linux )
#   define PLATFORM PLATFORM_LINUX
#   define PLAT_VERSION 0

#elif defined(__CYGWIN__)
#   define PLATFORM PLATFORM_CYGWIN
#   define PLAT_VERSION 0

#else
#error "Unknown Platform. Abort! Abort!"
#endif



/*****************************************************************************************
* Finds the CPU
*****************************************************************************************/
#if (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))) || \
        (defined(__GNUC__) && (defined(__i386__) ))
#    define CPU CPU_X86

#elif PLATFORM == PLATFORM_MAC && defined(__BIG_ENDIAN__)
#    define CPU CPU_PPC
#elif PLATFORM == PLATFORM_MAC
#       define CPU CPU_X86

#else
#error "Unknown CPU. Abort! Abort!"
#endif


#if defined(__x86_64__) || defined(_M_X64) || defined(__powerpc64__)   \
    || defined(__alpha__) || defined(__ia64__) || defined(__s390__)    \
    || defined(__s390x__)
#   define ARCH_TYPE ARCHITECTURE_64
#else
#   define ARCH_TYPE ARCHITECTURE_32
#endif



/*****************************************************************************************
* Finds the endian
*****************************************************************************************/

#if defined(__sparc) || defined(__sparc__) \
  || defined(_POWER) || defined(__powerpc__) \
	|| defined(__ppc__) || defined(__hpux) || defined(__hppa) \
	|| defined(_MIPSEB) || defined(_POWER) \
	|| defined(__s390__)
# define _BIG_ENDIAN_
# undef _LITTLE_ENDIAN_
#define ENDIAN ENDIAN_BIG


#elif defined(__i386__) || defined(__alpha__) \
	|| defined(__ia64) || defined(__ia64__) \
	|| defined(_M_IX86) || defined(_M_IA64) \
	|| defined(_M_ALPHA) || defined(__amd64) \
	|| defined(__amd64__) || defined(_M_AMD64) \
	|| defined(__x86_64) || defined(__x86_64__) \
	|| defined(_M_X64) || defined(__bfin__)

# define _LITTLE_ENDIAN_
# undef _BIG_ENDIAN_
#define ENDIAN ENDIAN_LITTLE

#else
# error Needs to be set up for your CPU type.
#endif



/*****************************************************************************************
* Define inline and forceinline
*****************************************************************************************/
#define INLINE __inline

#if( COMPILER == COMPILER_MSVC )
#   if COMP_VER >= 1200
#       define FORCEINLINE __forceinline
#   endif
#endif

#ifndef FORCEINLINE
#define FORCEINLINE INLINE
#endif



/*****************************************************************************************
* Define function description
*****************************************************************************************/
#if( PLATFORM == PLATFORM_WINDOWS )
#   define DLL_IMPORT  __declspec( dllimport )
#   define DLL_EXPORT  __declspec( dllexport )
#   define DLL_PRIVATE 

#else
#   define DLL_IMPORT
#   define DLL_EXPORT __attribute__ ((visibility("default")))
#   define DLL_PRIVATE __attribute__ ((visibility("hidden")))
#endif


#ifndef __NAMESPACE
#define __NAMESPACE Kaleidoscope
#define __BEGIN_NAMESPACE   namespace __NAMESPACE {
#define __END_NAMESPACE     }
#define __USING_NAMESPACE   using namespace __NAMESPACE;
#endif



/*****************************************************************************************
* Define programming version
*****************************************************************************************/
#if defined(DEBUG) || defined(_DEBUG)         // for detail debug
#undef  DEBUG
#undef  _DEBUG
#define DEBUG
#define _DEBUG

#elif defined(RELEASE) || defined(_RELEASE)   // for distribute
#ifndef NDEBUG
#define NDEBUG
#endif
#undef  RELEASE
#undef  _RELEASE
#define RELEASE
#define _RELEASE

#else                                         // for profile
#undef  PROFILE
#undef  _PROFILE
#define PROFILE
#define _PROFILE
#endif
