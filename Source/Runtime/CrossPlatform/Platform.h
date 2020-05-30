#pragma once



#if( PLATFORM == PLATFORM_WINDOWS )
// enable CRT debug.
#if !defined(RELEASE) && !defined(_RELEASE)
#define _CRTDBG_MAP_ALLOC 
#include <stdlib.h>
#include <crtdbg.h>
#endif 

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>  
#else

#endif