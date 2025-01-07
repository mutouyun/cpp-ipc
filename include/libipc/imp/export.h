/**
 * \file libipc/export.h
 * \author mutouyun (orz@orzz.org)
 * \brief Define the symbol export interfaces.
 */
#pragma once

#include "libipc/imp/detect_plat.h"

#if defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

# define IPC_DECL_EXPORT Q_DECL_EXPORT
# define IPC_DECL_IMPORT Q_DECL_IMPORT

#else // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Compiler & system detection for IPC_DECL_EXPORT & IPC_DECL_IMPORT.
 * Not using QtCore cause it shouldn't depend on Qt.
 */
# if defined(LIBIPC_CC_MSVC) || defined(LIBIPC_OS_WIN)
#   define IPC_DECL_EXPORT __declspec(dllexport)
#   define IPC_DECL_IMPORT __declspec(dllimport)
# elif defined(LIBIPC_OS_ANDROID) || defined(LIBIPC_OS_LINUX) || defined(LIBIPC_CC_GNUC)
#   define IPC_DECL_EXPORT __attribute__((visibility("default")))
#   define IPC_DECL_IMPORT __attribute__((visibility("default")))
# else
#   define IPC_DECL_EXPORT __attribute__((visibility("default")))
#   define IPC_DECL_IMPORT __attribute__((visibility("default")))
# endif

#endif // defined(Q_DECL_EXPORT) && defined(Q_DECL_IMPORT)

/**
 * \brief Define IPC_EXPORT for exporting function & class.
 */
#ifndef IPC_EXPORT
# if defined(LIBIPC_LIBRARY_SHARED_BUILDING__)
#   define IPC_EXPORT IPC_DECL_EXPORT
# elif defined(LIBIPC_LIBRARY_SHARED_USING__)
#   define IPC_EXPORT IPC_DECL_IMPORT
# else
#   define IPC_EXPORT
# endif
#endif /*IPC_EXPORT*/
