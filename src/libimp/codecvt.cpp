#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
#include "libimp/platform/win/codecvt.h"
#else
#include "libimp/platform/posix/codecvt.h"
#endif