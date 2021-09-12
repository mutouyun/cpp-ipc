
#include "libipc/semaphore.h"

#include "libipc/utility/pimpl.h"
#include "libipc/memory/resource.h"
#include "libipc/platform/detail.h"
#if defined(IPC_OS_WINDOWS_)
#include "libipc/platform/semaphore_win.h"
#elif defined(IPC_OS_LINUX_)
#include "libipc/platform/semaphore_linux.h"
#else/*linux*/
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

semaphore::semaphore(char const * name, std::uint32_t count, std::uint32_t limit)
    : semaphore() {
    open(name, count, limit);
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

bool semaphore::open(char const *name, std::uint32_t count, std::uint32_t limit) noexcept {
    return impl(p_)->sem_.open(name, count, limit);
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
