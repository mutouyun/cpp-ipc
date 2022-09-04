#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libipc/platform/win/mmap.h"
#else
#include "libipc/platform/posix/mmap.h"
#endif

LIBIPC_NAMESPACE_BEG_



LIBIPC_NAMESPACE_END_
