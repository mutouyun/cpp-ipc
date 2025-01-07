
#include "libipc/imp/detect_plat.h"
#if defined(LIBIPC_CC_GNUC)
# include "libipc/platform/gnuc/demangle.h"
#else
# include "libipc/platform/win/demangle.h"
#endif
