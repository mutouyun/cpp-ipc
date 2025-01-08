
#include "libipc/platform/detail.h"
#if defined(LIBIPC_OS_WIN)
#elif defined(LIBIPC_OS_LINUX)
#include "libipc/platform/linux/a0/err.c"
#include "libipc/platform/linux/a0/mtx.c"
#include "libipc/platform/linux/a0/strconv.c"
#include "libipc/platform/linux/a0/tid.c"
#include "libipc/platform/linux/a0/time.c"
#elif defined(LIBIPC_OS_QNX)
#else/*IPC_OS*/
#   error "Unsupported platform."
#endif
