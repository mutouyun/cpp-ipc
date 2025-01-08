
#include "libipc/imp/detect_plat.h"
#if defined(LIBIPC_OS_WIN)
# include "libipc/platform/win/system.h"
#else
# include "libipc/platform/posix/system.h"
#endif
