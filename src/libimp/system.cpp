
#include "libimp/system.h"
#include "libimp/fmt.h"
#include "libimp/detect_plat.h"
#if defined(LIBIMP_OS_WIN)
# include "libimp/platform/win/system.h"
#else
# include "libimp/platform/posix/system.h"
#endif
