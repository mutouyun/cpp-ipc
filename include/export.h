#pragma once

#if defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

#   define IPC_DECL_EXPORT Q_DECL_EXPORT
#   define IPC_DECL_IMPORT Q_DECL_IMPORT

#else // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/*
 * Compiler & system detection for IPC_DECL_EXPORT & IPC_DECL_IMPORT.
 * Not using QtCore cause it shouldn't depend on Qt.
*/

#if defined(_MSC_VER)
#   define IPC_DECL_EXPORT      __declspec(dllexport)
#   define IPC_DECL_IMPORT      __declspec(dllimport)
#elif defined(__ARMCC__) || defined(__CC_ARM)
#   if defined(ANDROID) || defined(__linux__) || defined(__linux)
#       define IPC_DECL_EXPORT  __attribute__((visibility("default")))
#       define IPC_DECL_IMPORT  __attribute__((visibility("default")))
#   else
#       define IPC_DECL_EXPORT  __declspec(dllexport)
#       define IPC_DECL_IMPORT  __declspec(dllimport)
#   endif
#elif defined(__GNUC__)
#   if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
       defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#       define IPC_DECL_EXPORT  __declspec(dllexport)
#       define IPC_DECL_IMPORT  __declspec(dllimport)
#   else
#       define IPC_DECL_EXPORT  __attribute__((visibility("default")))
#       define IPC_DECL_IMPORT  __attribute__((visibility("default")))
#   endif
#else
#   define IPC_DECL_EXPORT      __attribute__((visibility("default")))
#   define IPC_DECL_IMPORT      __attribute__((visibility("default")))
#endif

#endif // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/*
 * Define IPC_EXPORT for exporting function & class.
*/

#ifndef IPC_EXPORT
#if defined(__IPC_LIBRARY__)
#  define IPC_EXPORT IPC_DECL_EXPORT
#else
#  define IPC_EXPORT IPC_DECL_IMPORT
#endif
#endif /*IPC_EXPORT*/
