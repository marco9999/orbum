#pragma once

/*
Platform macros, to make sure they are universal.
*/
#if defined(__WINDOWS__) || defined(_WIN32) || defined(_WIN64)
#define ENV_WINDOWS
#elif defined(__UNIX) || defined(__UNIX__) || defined(__APPLE__) || defined(__MACH__) || defined(__LINUX__)
#define ENV_UNIX
#endif

/*
Enable the BUILD_DEBUG token if a debug build configuration is detected.
*/
#if defined(_DEBUG) || defined(_DEBUG_BUILD)
#define BUILD_DEBUG
#endif

/*
Inlineing of functions.
*/
#if defined(ENV_WINDOWS)
 #if defined(BUILD_DEBUG)
  #define INLINE 
 #else
  #define INLINE __declspec(inline)
 #endif
 #define NO_INLINE __declspec(noinline)
#elif defined(ENV_UNIX)
 #if defined(BUILD_DEBUG)
  #define INLINE 
 #else
  #define INLINE __attribute__((inline))
 #endif
 #define NO_INLINE __attribute__((noinline))
#endif