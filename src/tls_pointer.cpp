#include "tls_pointer.h"

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
#include "platform/tls_pointer_win.h"
#else /*!WIN*/
#include "platform/tls_pointer_linux.h"
#endif/*!WIN*/
