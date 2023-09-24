
#include "libipc/semaphore.h"

#include "libipc/utility/pimpl.h"
#include "libipc/utility/log.h"
#include "libipc/memory/resource.h"
#include "libipc/platform/detail.h"
#if defined(IPC_OS_WINDOWS_)
#include "libipc/platform/win/semaphore.h"
#elif defined(IPC_OS_LINUX_) || defined(IPC_OS_QNX_)
#include "libipc/platform/posix/semaphore_impl.h"
#else/*IPC_OS*/
#   error "Unsupported platform."
#endif

namespace ipc {
namespace sync {

class semaphore::semaphore_ : public ipc::pimpl<semaphore_> {
public:
    ipc::detail::sync::semaphore sem_;
};

semaphore::semaphore()
    : p_(p_->make()) {
}

semaphore::semaphore(char const * name, std::uint32_t count)
    : semaphore() {
    open(name, count);
}

semaphore::~semaphore() {
    close();
    p_->clear();
}

void const *semaphore::native() const noexcept {
    return impl(p_)->sem_.native();
}

void *semaphore::native() noexcept {
    return impl(p_)->sem_.native();
}

bool semaphore::valid() const noexcept {
    return impl(p_)->sem_.valid();
}

bool semaphore::open(char const *name, std::uint32_t count) noexcept {
    if (!is_valid_string(name)) {
        ipc::error("fail semaphore open: name is empty\n");
        return false;
    }
    return impl(p_)->sem_.open(name, count);
}

void semaphore::close() noexcept {
    impl(p_)->sem_.close();
}

bool semaphore::wait(std::uint64_t tm) noexcept {
    return impl(p_)->sem_.wait(tm);
}

bool semaphore::post(std::uint32_t count) noexcept {
    return impl(p_)->sem_.post(count);
}

} // namespace sync
} // namespace ipc
