#include "libipc/waiter.h"

#include "libipc/platform/detail.h"
#if defined(LIBIPC_OS_WIN)
#include "libipc/platform/win/mutex.h"
#elif defined(LIBIPC_OS_LINUX)
#include "libipc/platform/linux/mutex.h"
#elif defined(LIBIPC_OS_QNX)
#include "libipc/platform/posix/mutex.h"
#else/*IPC_OS*/
#   error "Unsupported platform."
#endif

namespace ipc {
namespace detail {

void waiter::init() {
    ipc::detail::sync::mutex::init();
}

} // namespace detail
} // namespace ipc
