#include "waiter.h"

#include <string>

#include "def.h"
#include "platform/waiter.h"

namespace ipc {

class waiter::waiter_ : public pimpl<waiter_> {
public:
    detail::waiter_impl w_ { new detail::waiter };
    ~waiter_() { delete w_.waiter(); }
};

waiter::waiter()
    : p_(p_->make()) {
}

waiter::waiter(char const * name)
    : waiter() {
    open(name);
}

waiter::waiter(waiter&& rhs)
    : waiter() {
    swap(rhs);
}

waiter::~waiter() {
    p_->clear();
}

void waiter::swap(waiter& rhs) {
    std::swap(p_, rhs.p_);
}

waiter& waiter::operator=(waiter rhs) {
    swap(rhs);
    return *this;
}

bool waiter::valid() const {
    return impl(p_)->w_.valid();
}

char const * waiter::name() const {
    return impl(p_)->w_.name();
}

bool waiter::open(char const * name) {
    return impl(p_)->w_.open(name);
}

void waiter::close() {
    impl(p_)->w_.close();
}

bool waiter::wait() {
    return impl(p_)->w_.wait();
}

bool waiter::notify() {
    return impl(p_)->w_.notify();
}

bool waiter::broadcast() {
    return impl(p_)->w_.broadcast();
}

} // namespace ipc
