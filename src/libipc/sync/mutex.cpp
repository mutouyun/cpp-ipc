
#include "libipc/mutex.h"

#include "libipc/utility/pimpl.h"
#include "libipc/memory/resource.h"
#include "libipc/platform/detail.h"
#if defined(IPC_OS_WINDOWS_)
#include "libipc/platform/win/mutex.h"
#elif defined(IPC_OS_LINUX_)
#include "libipc/platform/linux/mutex.h"
#elif defined(IPC_OS_QNX_)
#include "libipc/platform/posix/mutex.h"
#else/*IPC_OS*/
#   error "Unsupported platform."
#endif

namespace ipc {
namespace sync {

class mutex::mutex_ : public ipc::pimpl<mutex_> {
public:
    ipc::detail::sync::mutex lock_;
};

mutex::mutex()
    : p_(p_->make()) {
}

mutex::mutex(char const * name)
    : mutex() {
    open(name);
}

mutex::~mutex() {
    close();
    p_->clear();
}

void const *mutex::native() const noexcept {
    return impl(p_)->lock_.native();
}

void *mutex::native() noexcept {
    return impl(p_)->lock_.native();
}

bool mutex::valid() const noexcept {
    return impl(p_)->lock_.valid();
}

bool mutex::open(char const *name) noexcept {
    return impl(p_)->lock_.open(name);
}

void mutex::close() noexcept {
    impl(p_)->lock_.close();
}

bool mutex::lock(std::uint64_t tm) noexcept {
    return impl(p_)->lock_.lock(tm);
}

bool mutex::try_lock() noexcept(false) {
    return impl(p_)->lock_.try_lock();
}

bool mutex::unlock() noexcept {
    return impl(p_)->lock_.unlock();
}

} // namespace sync
} // namespace ipc
