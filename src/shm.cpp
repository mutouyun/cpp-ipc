#include "shm.h"

#include <string>
#include <utility>

#include "def.h"

namespace ipc {
namespace shm {

class handle::handle_ : public pimpl<handle_> {
public:
    handle*  t_ = nullptr;
    handle_t h_ = nullptr;
    void*    m_ = nullptr;

    std::string n_ {};
    std::size_t s_ = 0;

    handle_() = default;
    handle_(handle* t) : t_{t} {}

    ~handle_(void) {
        t_->close();
        t_->release();
    }
};

handle::handle(void)
    : p_(p_->make(this)) {
}

handle::handle(char const * name, std::size_t size)
    : handle() {
    acquire(name, size);
}

handle::handle(handle&& rhs)
    : handle() {
    swap(rhs);
}

handle::~handle(void) {
    p_->clear();
}

void handle::swap(handle& rhs) {
    std::swap(p_, rhs.p_);
}

handle& handle::operator=(handle rhs) {
    swap(rhs);
    return *this;
}

bool handle::valid(void) const {
    return impl(p_)->h_ != nullptr;
}

std::size_t handle::size(void) const {
    return impl(p_)->s_;
}

char const * handle::name(void) const {
    return impl(p_)->n_.c_str();
}

bool handle::acquire(char const * name, std::size_t size) {
    close();
    release();
    impl(p_)->h_ = shm::acquire((impl(p_)->n_ = name).c_str(),
                                 impl(p_)->s_ = size);
    return valid();
}

void handle::release(void) {
    if (!valid()) return;
    shm::release(impl(p_)->h_, impl(p_)->s_);
    impl(p_)->h_ = nullptr;
    impl(p_)->s_ = 0;
    impl(p_)->n_.clear();
}

void* handle::get(void) {
    if (!valid()) return nullptr;
    if (impl(p_)->m_ == nullptr) {
        return impl(p_)->m_ = shm::open(impl(p_)->h_);
    }
    else return impl(p_)->m_;
}

void handle::close(void) {
    if (!valid()) return;
    shm::close(impl(p_)->m_);
    impl(p_)->m_ = nullptr;
}

} // namespace shm
} // namespace ipc
