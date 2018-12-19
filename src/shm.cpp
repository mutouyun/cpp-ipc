#include "shm.h"

#include <string>
#include <utility>

#include "def.h"

namespace ipc {
namespace shm {

class handle::handle_ : public pimpl<handle_> {
public:
    void* m_ = nullptr;

    std::string n_;
    std::size_t s_ = 0;
};

handle::handle()
    : p_(p_->make()) {
}

handle::handle(char const * name, std::size_t size)
    : handle() {
    acquire(name, size);
}

handle::handle(handle&& rhs)
    : handle() {
    swap(rhs);
}

handle::~handle() {
    release();
    p_->clear();
}

void handle::swap(handle& rhs) {
    std::swap(p_, rhs.p_);
}

handle& handle::operator=(handle rhs) {
    swap(rhs);
    return *this;
}

bool handle::valid() const {
    return impl(p_)->m_ != nullptr;
}

std::size_t handle::size() const {
    return impl(p_)->s_;
}

char const * handle::name() const {
    return impl(p_)->n_.c_str();
}

bool handle::acquire(char const * name, std::size_t size) {
    release();
    impl(p_)->m_ = shm::acquire((impl(p_)->n_ = name).c_str(),
                                 impl(p_)->s_ = size);
    return valid();
}

void handle::release() {
    if (!valid()) return;
    shm::release(impl(p_)->m_, impl(p_)->s_);
    impl(p_)->m_ = nullptr;
    impl(p_)->s_ = 0;
    impl(p_)->n_.clear();
}

void* handle::get() const {
    if (!valid()) return nullptr;
    return impl(p_)->m_;
}

} // namespace shm
} // namespace ipc
