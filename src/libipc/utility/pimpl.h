#pragma once

#include <new>
#include <utility>

#include "libipc/platform/detail.h"
#include "libipc/utility/concept.h"
#include "libipc/pool_alloc.h"

namespace ipc {

// pimpl small object optimization helpers

template <typename T, typename R = T*>
using IsImplComfortable = ipc::require<(sizeof(T) <= sizeof(T*)), R>;

template <typename T, typename R = T*>
using IsImplUncomfortable = ipc::require<(sizeof(T) > sizeof(T*)), R>;

template <typename T, typename... P>
IPC_CONSTEXPR_ auto make_impl(P&&... params) -> IsImplComfortable<T> {
    T* buf {};
    ::new (&buf) T { std::forward<P>(params)... };
    return buf;
}

template <typename T>
IPC_CONSTEXPR_ auto impl(T* const (& p)) -> IsImplComfortable<T> {
    return reinterpret_cast<T*>(&const_cast<char &>(reinterpret_cast<char const &>(p)));
}

template <typename T>
IPC_CONSTEXPR_ auto clear_impl(T* p) -> IsImplComfortable<T, void> {
    if (p != nullptr) impl(p)->~T();
}

template <typename T, typename... P>
IPC_CONSTEXPR_ auto make_impl(P&&... params) -> IsImplUncomfortable<T> {
    return mem::alloc<T>(std::forward<P>(params)...);
}

template <typename T>
IPC_CONSTEXPR_ auto clear_impl(T* p) -> IsImplUncomfortable<T, void> {
    mem::free(p);
}

template <typename T>
IPC_CONSTEXPR_ auto impl(T* const (& p)) -> IsImplUncomfortable<T> {
    return p;
}

template <typename T>
struct pimpl {
    template <typename... P>
    IPC_CONSTEXPR_ static T* make(P&&... params) {
        return make_impl<T>(std::forward<P>(params)...);
    }

    IPC_CONSTEXPR_ void clear() {
        clear_impl(static_cast<T*>(const_cast<pimpl*>(this)));
    }
};

} // namespace ipc
