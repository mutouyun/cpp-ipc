
#include "libimp/detect_plat.h"
#if defined(LIBIMP_CC_GNUC)
# include "libimp/platform/gnuc/demangle.h"
#else
# include "libimp/platform/win/demangle.h"
#endif
