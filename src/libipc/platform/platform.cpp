
#include "libipc/platform/detail.h"
#if defined(LIBIPC_OS_WIN)
#include "libipc/platform/win/shm_win.cpp"
#elif defined(LIBIPC_OS_LINUX) || defined(LIBIPC_OS_QNX)
#include "libipc/platform/posix/shm_posix.cpp"
#else/*IPC_OS*/
#   error "Unsupported platform."
#endif
