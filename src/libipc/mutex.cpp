
#include "libimp/log.h"

#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
# include "libipc/platform/win/mutex_impl.h"
#else
#endif

#if !defined(LIBIMP_OS_LINUX)
LIBIPC_NAMESPACE_BEG_

/// \brief C style mutex access interface implementation.

/// \brief The mutex object.

LIBIPC_NAMESPACE_END_
#endif
