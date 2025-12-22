#pragma once

#if   defined(_M_X64)\
   || defined(__amd64__)\
   || defined(__amd64)\
   || defined(_WIN64)\
   || defined(_WIN64_)\
   || defined(WIN64)\
   || defined(__LP64__)\
   || defined(_LP64)\
   || defined(__x86_64__)\
   || defined(__ppc64__)\
   || defined(__aarch64__)
    #define GTX64  
#else
    #define GTX32  
#endif

#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x) CONCATENATE(x, __COUNTER__)

////////////////////////////////////////////////////////////////////////////////
//WINDOWS specific definitions & types
#if defined(_WIN32) || defined(_WIN64)

    #define G_OS_WINDOWS
    
    #if !defined(_WINSOCKAPI_)
        #include <winsock2.h>
    #endif
    #include <windows.h>

    //Text marco, allow to use wchar_t automatically
    #define TM(i_pStr)         L##i_pStr

    #define XCHAR              wchar_t
    typedef wchar_t            tWCHAR;

    #define WND_HANDLE         HWND

    #define SHARED_EXT        L"dll"

    #define P7_EXPORT __declspec(dllexport)

    #define OS_PATH_SEPARATOR TM("\\")

////////////////////////////////////////////////////////////////////////////////
//LINUX specific definitions & types
#elif defined(__linux__)
    #define UTF8_ENCODING
    #define G_OS_LINUX

    //Text marco, allow to use char automatically
    #define TM(i_pStr)    i_pStr

    #define XCHAR          char
    typedef unsigned short tWCHAR;
    #define WND_HANDLE     void*

    #define SHARED_EXT    "so"

    typedef struct _GUID
    {
        unsigned int   Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char  Data4[ 8 ];
    } GUID;

    #define __stdcall
    #define __cdecl

    #ifndef __forceinline
        #if defined(GTX64) || defined(__PIC__)
            #define __forceinline  
        #else
            #define __forceinline  __attribute__((always_inline))
        #endif
    #endif

    #define P7_EXPORT __attribute__ ((visibility ("default")))

    #define OS_PATH_SEPARATOR TM("/")
#endif

#ifdef _MSC_VER
    #define PRAGMA_PACK_ENTER(x)  __pragma(pack(push, x))
    #define PRAGMA_PACK_EXIT()   __pragma(pack(pop))
    #define ATTR_PACK(x)
    #define ATTR_ALIGN(x)
    #define UNUSED_FUNC
#else
    #define PRAGMA_PACK_ENTER(x) 
    #define PRAGMA_PACK_EXIT(x) 
    #define ATTR_PACK(x) __attribute__ ((aligned(x), packed))
    #define ATTR_ALIGN(x) __attribute__ ((aligned(x)))
    #define UNUSED_FUNC __attribute__ ((unused))
#endif


#define UNUSED_ARG(x)        (void)(x)

#define STR_HELPER(x)        #x
#define TOSTR(x)             STR_HELPER(x)

#define TMM(i_pStr)          TM(i_pStr)

//platfrorm specific char, Windows - wchar_t, Linix - char,
//XCHAR defined in PTypes.hpp specific for each platform or project.
#define tXCHAR               XCHAR  
