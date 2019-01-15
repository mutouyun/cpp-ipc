#pragma once

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
#include "platform/waiter_win.h"
#else
#include "platform/waiter_linux.h"
#endif

namespace ipc {
namespace detail {

class waiter_impl {
public:
    using waiter_t = detail::waiter;

private:
    waiter_t* w_ = nullptr;
    waiter_t::handle_t h_ = waiter_t::invalid();

public:
    waiter_impl() = default;
    explicit waiter_impl(waiter_t* w) {
        attach(w);
    }
    waiter_impl(const waiter_impl&) = delete;
    waiter_impl& operator=(const waiter_impl&) = delete;

    waiter_t       * waiter()       { return w_; }
    waiter_t const * waiter() const { return w_; }

    void attach(waiter_t* w) {
        w_ = w;
    }

    bool valid() const {
        return (w_ != nullptr) && (h_ != waiter_t::invalid());
    }

    bool open(char const * name) {
        if (w_ == nullptr) return false;
        close();
        h_ = w_->open(name);
        return valid();
    }

    void close() {
        if (!valid()) return;
        w_->close(h_);
        h_ = waiter_t::invalid();
    }

    bool wait() {
        if (!valid()) return false;
        return w_->wait(h_);
    }

    bool notify() {
        if (!valid()) return false;
        w_->notify(h_);
        return true;
    }

    bool broadcast() {
        if (!valid()) return false;
        w_->broadcast(h_);
        return true;
    }
};

} // namespace detail
} // namespace ipc
