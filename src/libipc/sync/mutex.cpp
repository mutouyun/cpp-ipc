
#include "libipc/mutex.h"

#include "libipc/utility/pimpl.h"
#include "libipc/utility/log.h"
#include "libipc/memory/resource.h"
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
    if (!is_valid_string(name)) {
        ipc::error("fail mutex open: name is empty\n");
        return false;
    }
    return impl(p_)->lock_.open(name);
}

void mutex::close() noexcept {
    impl(p_)->lock_.close();
}

void mutex::clear() noexcept {
    impl(p_)->lock_.clear();
}

void mutex::clear_storage(char const * name) noexcept {
    ipc::detail::sync::mutex::clear_storage(name);
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
