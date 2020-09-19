#pragma once

#include <utility>      // std::forward, std::move
#include <algorithm>    // std::swap
#include <type_traits>  // std::decay

namespace ipc {

////////////////////////////////////////////////////////////////
/// Execute guard function when the enclosing scope exits
////////////////////////////////////////////////////////////////

template <typename F>
class scope_guard {
    F destructor_;
    mutable bool dismiss_;

public:
    template <typename D>
    scope_guard(D && destructor)
        : destructor_(std::forward<D>(destructor))
        , dismiss_(false) {
    }

    scope_guard(scope_guard&& rhs)
        : destructor_(std::move(rhs.destructor_))
        , dismiss_(true) /* dismiss rhs */ {
        std::swap(dismiss_, rhs.dismiss_);
    }

    ~scope_guard() {
        try { do_exit(); }
        /**
         * In the realm of exceptions, it is fundamental that you can do nothing
         * if your "undo/recover" action fails.
        */
        catch (...) { /* Do nothing */ }
    }

    void swap(scope_guard & rhs) {
        std::swap(destructor_, rhs.destructor_);
        std::swap(dismiss_   , rhs.dismiss_);
    }

    void dismiss() const noexcept {
        dismiss_ = true;
    }

    void do_exit() {
        if (!dismiss_) {
            dismiss_ = true;
            destructor_();
        }
    }
};

template <typename D>
constexpr auto guard(D && destructor) noexcept {
    return scope_guard<std::decay_t<D>> {
        std::forward<D>(destructor)
    };
}

} // namespace ipc