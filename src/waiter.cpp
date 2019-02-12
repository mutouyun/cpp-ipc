#include "waiter.h"

#include <string>

#include "def.h"
#include "platform/waiter_wrapper.h"

namespace ipc {

class waiter::waiter_ : public pimpl<waiter_> {
public:
    std::string n_;

    detail::waiter_wrapper w_ { new detail::waiter };
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
    return impl(p_)->n_.c_str();
}

bool waiter::open(char const * name) {
    if (impl(p_)->w_.open(name)) {
        impl(p_)->n_ = name;
        return true;
    }
    return false;
}

void waiter::close() {
    impl(p_)->w_.close();
    impl(p_)->n_.clear();
}

bool waiter::wait() {
    return impl(p_)->w_.wait_if([] { return true; });
}

bool waiter::pred() {
    return true;
}

bool waiter::wait_if_pred() {
    return impl(p_)->w_.wait_if([this] { return pred(); });
}

bool waiter::notify() {
    return impl(p_)->w_.notify();
}

bool waiter::broadcast() {
    return impl(p_)->w_.broadcast();
}

} // namespace ipc
