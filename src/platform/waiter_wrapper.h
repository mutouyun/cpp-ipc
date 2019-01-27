#pragma once

#include <string>
#include <tuple>
#include <type_traits>

#include "pool_alloc.h"

#if defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || \
    defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || \
    defined(WINCE) || defined(_WIN32_WCE)
#include "platform/waiter_win.h"
#else
#include "platform/waiter_linux.h"
#endif
#include "platform/detail.h"

namespace ipc {
namespace detail {

class waiter_wrapper {
public:
    using waiter_t = detail::waiter;

private:
    waiter_t* w_ = nullptr;
    waiter_t::handle_t h_ = waiter_t::invalid();

    auto to_w_info() {
        return std::make_tuple(w_, h_);
    }

public:
    waiter_wrapper() = default;
    explicit waiter_wrapper(waiter_t* w) {
        attach(w);
    }
    waiter_wrapper(const waiter_wrapper&) = delete;
    waiter_wrapper& operator=(const waiter_wrapper&) = delete;

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

    bool wait_all(waiter_wrapper * all, std::size_t size) {
        if (all == nullptr || size == 0) {
            return false;
        }
        using tp_t = decltype(std::declval<waiter_wrapper>().to_w_info());
        auto hs = static_cast<tp_t*>(mem::alloc(sizeof(tp_t) * size));
        IPC_UNUSED_ auto guard = unique_ptr(hs, [size](void* p) { mem::free(p, sizeof(tp_t) * size); });
        std::size_t i = 0;
        for (; i < size; ++i) {
            auto& w = all[i];
            if (!w.valid()) continue;
            hs[i] = w.to_w_info();
        }
        return waiter_t::wait_all(hs, i);
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
